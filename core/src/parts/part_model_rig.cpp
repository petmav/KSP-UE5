// WP-29d: the transform hierarchy of a part model and its legacy Animation components. See ksp/parts/part_model_rig.hpp.
#include "ksp/parts/part_model_rig.hpp"

#include <algorithm>
#include <cmath>
#include <functional>

#include "ksp/unity/quaternion_native.hpp"

namespace ksp::mu {

namespace {

// property name of a Transform curve -> component index (PartReader.cs:848 AnimationClip.SetCurve(path, typeof(Transform), property, curve))
int ComponentOf(const std::string& p) {
    static const char* names[10] = {"m_LocalPosition.x", "m_LocalPosition.y", "m_LocalPosition.z", "m_LocalRotation.x", "m_LocalRotation.y",
                                    "m_LocalRotation.z", "m_LocalRotation.w", "m_LocalScale.x", "m_LocalScale.y", "m_LocalScale.z"};
    for (int i = 0; i < 10; ++i)
        if (p == names[i]) return i;
    return -1;
}

}  // namespace

float RigAnimation::Length(const std::string& clip) const {
    const int i = FindClip(clip);
    return i < 0 ? 0.0f : chunk->clips[static_cast<size_t>(i)].Length();
}

int RigAnimation::FindClip(const std::string& clip) const {
    if (chunk == nullptr) return -1;
    for (size_t i = 0; i < chunk->clips.size(); ++i)
        if (chunk->clips[i].name == clip) return static_cast<int>(i);
    return -1;
}

// CompileModel placement (PartLoader.cs:1765), the same rules as BuildPartColliders (parts/part_colliders.cpp)
PartModelRig PartModelRig::Build(float rescaleFactor, const std::vector<ModelInstance>& instances) {
    PartModelRig rig;
    std::function<int(const Node&, int)> add = [&](const Node& n, int parent) {
        const int idx = static_cast<int>(rig.nodes_.size());
        rig.nodes_.emplace_back();
        {
            RigNode& r = rig.nodes_.back();
            r.name = n.name;
            r.parent = parent;
            r.lp = n.position;
            r.lr = n.rotation;
            r.ls = n.scale;
            r.src = &n;
        }
        if (parent >= 0) rig.nodes_[static_cast<size_t>(parent)].children.push_back(idx);
        for (const Node& c : n.children) add(c, idx);
        return idx;
    };
    bool any = false;
    for (const ModelInstance& mi : instances) {
        if (!mi.model) continue;
        if (!mi.fromModelNode) {
            // PartLoader.cs:1914-1927: the instance is renamed "model", position 0, rotation identity, scale rescaleFactor
            rig.nodes_.clear();
            const int m = add(mi.model->root, -1);
            RigNode& r = rig.nodes_[static_cast<size_t>(m)];
            r.name = "model";
            r.lp = Vector3f();
            r.lr = Quaternionf::identity();
            r.ls = Vector3f(rescaleFactor, rescaleFactor, rescaleFactor);
            any = true;
            break;
        }
        if (rig.nodes_.empty()) {
            RigNode model;  // gameObject2 = new GameObject("model"), localScale = Vector3.one * scaleFactor
            model.name = "model";
            model.ls = Vector3f(rescaleFactor, rescaleFactor, rescaleFactor);
            rig.nodes_.push_back(model);
        }
        int host = 0;
        if (!mi.parent.empty()) {
            const int f = rig.FindPath(0, mi.parent);
            if (f >= 0) host = f;
        }
        const int inst = add(mi.model->root, host);
        RigNode& r = rig.nodes_[static_cast<size_t>(inst)];
        // NestToParent: position 0, rotation identity (TransformExtension.cs), then the cfg values
        r.lp = Vector3f();
        r.lr = Quaternionf::identity();
        r.ls = mi.scale ? *mi.scale : Vector3f(1.0f, 1.0f, 1.0f);
        if (mi.name) r.name = *mi.name;
        if (mi.position) r.lp = *mi.position;
        if (mi.rotationEuler) r.lr = UnityNative::Euler(*mi.rotationEuler);
        any = true;
    }
    if (!any) {
        rig.nodes_.clear();
        return rig;
    }
    rig.prefab_ = rig.nodes_;
    rig.CollectAnimations(0);
    rig.ResolveTargets();
    return rig;
}

void PartModelRig::CollectAnimations(int node) {
    const RigNode& n = nodes_[static_cast<size_t>(node)];
    if (n.src != nullptr)
        for (const AnimationChunk& a : n.src->animations) {
            RigAnimation ra;
            ra.owner = node;
            ra.chunk = &a;
            animations_.push_back(std::move(ra));
        }
    for (int c : n.children) CollectAnimations(c);
}

void PartModelRig::ResolveTargets() {
    for (RigAnimation& ra : animations_) {
        ra.clipTargets.resize(ra.chunk->clips.size());
        for (size_t ci = 0; ci < ra.chunk->clips.size(); ++ci) {
            for (const AnimCurve& c : ra.chunk->clips[ci].curves) {
                if (c.type != static_cast<int32_t>(AnimationType::Transform)) continue;
                const int comp = ComponentOf(c.property);
                const int node = c.path.empty() ? ra.owner : FindPath(ra.owner, c.path);
                if (comp < 0 || node < 0) continue;  // a curve addressing nothing is ignored (Unity warns once)
                RigAnimation::Target t;
                t.node = node;
                t.component = comp;
                std::vector<Keyframe> keys;
                keys.reserve(c.keys.size());
                for (const AnimKey& k : c.keys) keys.emplace_back(k.time, k.value, k.inTangent, k.outTangent);
                t.curve = AnimationCurve(keys);
                t.curve.SetPreWrapMode(WrapMode::ClampForever);
                t.curve.SetPostWrapMode(WrapMode::ClampForever);
                ra.clipTargets[ci].push_back(std::move(t));
            }
        }
    }
}

// Part.FindHeirarchyTransform (Part.cs:5084): the node itself, else the first match among the children's subtrees (pre-order)
int PartModelRig::Find(const std::string& name) const {
    if (nodes_.empty()) return -1;
    std::function<int(int)> rec = [&](int i) {
        const RigNode& n = nodes_[static_cast<size_t>(i)];
        if (n.name == name) return i;
        for (int c : n.children) {
            const int r = rec(c);
            if (r >= 0) return r;
        }
        return -1;
    };
    return rec(0);
}

// Part.FindHeirarchyTransforms (Part.cs:5161)
std::vector<int> PartModelRig::FindAll(const std::string& name) const {
    std::vector<int> out;
    if (nodes_.empty()) return out;
    std::function<void(int)> rec = [&](int i) {
        const RigNode& n = nodes_[static_cast<size_t>(i)];
        if (n.name == name) out.push_back(i);
        for (int c : n.children) rec(c);
    };
    rec(0);
    return out;
}

int PartModelRig::FindPath(int from, const std::string& path) const {
    int cur = from;
    size_t i = 0;
    while (cur >= 0 && i <= path.size()) {
        size_t j = path.find('/', i);
        if (j == std::string::npos) j = path.size();
        const std::string part = path.substr(i, j - i);
        int next = -1;
        for (int c : nodes_[static_cast<size_t>(cur)].children)
            if (nodes_[static_cast<size_t>(c)].name == part) {
                next = c;
                break;
            }
        cur = next;
        i = j + 1;
        if (j == path.size()) break;
    }
    return cur;
}

RigPose PartModelRig::PartSpace(int node) const {
    std::vector<int> chain;
    for (int i = node; i >= 0; i = nodes_[static_cast<size_t>(i)].parent) chain.push_back(i);
    RigPose w;
    for (size_t k = chain.size(); k-- > 0;) {
        const RigNode& n = nodes_[static_cast<size_t>(chain[k])];
        const Vector3f scaled(w.scale.x * n.lp.x, w.scale.y * n.lp.y, w.scale.z * n.lp.z);
        const Vector3f r = RotateNative(w.rotation, scaled);
        w.position = Vector3f(w.position.x + r.x, w.position.y + r.y, w.position.z + r.z);
        w.rotation = UnityNative::Multiply(w.rotation, n.lr);
        w.scale = Vector3f(w.scale.x * n.ls.x, w.scale.y * n.ls.y, w.scale.z * n.ls.z);
    }
    return w;
}

void PartModelRig::SetLocal(int node, const Vector3f& position, const Quaternionf& rotation) {
    RigNode& n = nodes_[static_cast<size_t>(node)];
    n.lp = position;
    n.lr = rotation;
}

void PartModelRig::SetLocalPosition(int node, const Vector3f& position) { nodes_[static_cast<size_t>(node)].lp = position; }

// Transform.position setter (WP-29e): UnityNative::Inverse / RotateNative are the native maths of PartSpace's chain, one rounding per operation
void PartModelRig::SetPartSpacePosition(int node, const Vector3f& p) {
    RigNode& n = nodes_[static_cast<size_t>(node)];
    if (n.parent < 0) {
        n.lp = p;
        return;
    }
    const RigPose pp = PartSpace(n.parent);
    const Vector3f d = RotateNative(UnityNative::Inverse(pp.rotation), Vector3f(p.x - pp.position.x, p.y - pp.position.y, p.z - pp.position.z));
    n.lp = Vector3f(d.x / pp.scale.x, d.y / pp.scale.y, d.z / pp.scale.z);
}

bool PartModelRig::IsChildOf(int node, int ancestor) const {
    for (int i = node; i >= 0; i = nodes_[static_cast<size_t>(i)].parent)
        if (i == ancestor) return true;
    return false;
}

void PartModelRig::ResetLocal() {
    for (size_t i = 0; i < nodes_.size(); ++i) {
        nodes_[i].lp = prefab_[i].lp;
        nodes_[i].lr = prefab_[i].lr;
        nodes_[i].ls = prefab_[i].ls;
    }
}

std::vector<PartColliderShape> PartModelRig::Colliders() const {
    std::vector<PartColliderShape> out;
    if (nodes_.empty()) return out;
    std::function<void(int, const std::string&)> rec = [&](int i, const std::string& parentPath) {
        const RigNode& n = nodes_[static_cast<size_t>(i)];
        const std::string path = parentPath.empty() ? n.name : parentPath + "/" + n.name;
        if (n.src != nullptr) {
            const RigPose w = PartSpace(i);
            AppendNodeColliderShapes(*n.src, path, w.position, w.rotation, w.scale, out);
        }
        for (int c : n.children) rec(c, path);
    };
    rec(0, std::string());
    return out;
}

int PartModelRig::FindAnimator(const std::string& transformName, const std::string& clipName) const {
    for (size_t i = 0; i < animations_.size(); ++i)
        if (nodes_[static_cast<size_t>(animations_[i].owner)].name == transformName && animations_[i].FindClip(clipName) >= 0) return static_cast<int>(i);
    return -1;
}

void PartModelRig::SampleClip(int animation, int clip, float time) {
    if (animation < 0 || clip < 0) return;
    RigAnimation& ra = animations_[static_cast<size_t>(animation)];
    if (static_cast<size_t>(clip) >= ra.clipTargets.size()) return;
    // gather per node the components the clip writes, so a rotation is normalised once after all four components are in
    std::vector<int> touchedRot;
    for (const RigAnimation::Target& t : ra.clipTargets[static_cast<size_t>(clip)]) {
        RigNode& n = nodes_[static_cast<size_t>(t.node)];
        const float v = t.curve.Evaluate(time);
        switch (t.component) {
            case 0: n.lp.x = v; break;
            case 1: n.lp.y = v; break;
            case 2: n.lp.z = v; break;
            case 3: n.lr.x = v; touchedRot.push_back(t.node); break;
            case 4: n.lr.y = v; touchedRot.push_back(t.node); break;
            case 5: n.lr.z = v; touchedRot.push_back(t.node); break;
            case 6: n.lr.w = v; touchedRot.push_back(t.node); break;
            case 7: n.ls.x = v; break;
            case 8: n.ls.y = v; break;
            default: n.ls.z = v; break;
        }
    }
    std::sort(touchedRot.begin(), touchedRot.end());
    touchedRot.erase(std::unique(touchedRot.begin(), touchedRot.end()), touchedRot.end());
    for (int ni : touchedRot) {
        Quaternionf& q = nodes_[static_cast<size_t>(ni)].lr;
        const float m = std::sqrt(((q.x * q.x + q.y * q.y) + q.z * q.z) + q.w * q.w);  // std-math-ok: native Unity Animation rotation blend normalisation (IEEE sqrt, correctly rounded; not managed code)
        if (m > 1e-6f) {
            q.x /= m;
            q.y /= m;
            q.z /= m;
            q.w /= m;
        }
    }
}

}  // namespace ksp::mu
