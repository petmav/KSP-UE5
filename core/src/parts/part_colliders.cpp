// WP-mucol: part-space collider shapes from .mu models. See ksp/parts/part_colliders.hpp.
#include "ksp/parts/part_colliders.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

#include "ksp/unity/quaternion_native.hpp"

namespace ksp::mu {

namespace {

struct TNode {
    std::string name;
    Vector3f lp;
    Quaternionf lr = Quaternionf::identity();
    Vector3f ls{1.0f, 1.0f, 1.0f};
    const Node* src = nullptr;
    std::vector<std::unique_ptr<TNode>> children;
};

std::unique_ptr<TNode> FromMu(const Node& n) {
    auto t = std::make_unique<TNode>();
    t->name = n.name;
    t->lp = n.position;
    t->lr = n.rotation;
    t->ls = n.scale;
    t->src = &n;
    for (const Node& c : n.children) t->children.push_back(FromMu(c));
    return t;
}

// Transform.Find(path): '/'-separated, direct children by name, first match
TNode* Find(TNode* root, const std::string& path) {
    TNode* cur = root;
    size_t i = 0;
    while (cur && i <= path.size()) {
        size_t j = path.find('/', i);
        if (j == std::string::npos) j = path.size();
        const std::string part = path.substr(i, j - i);
        TNode* next = nullptr;
        for (auto& c : cur->children)
            if (c->name == part) { next = c.get(); break; }
        cur = next;
        i = j + 1;
        if (j == path.size()) break;
    }
    return cur;
}

struct World {
    Vector3f pos;
    Quaternionf rot = Quaternionf::identity();
    Vector3f scl{1.0f, 1.0f, 1.0f};
};

World Compose(const World& p, const TNode& n) {
    World w;
    const Vector3f scaled(p.scl.x * n.lp.x, p.scl.y * n.lp.y, p.scl.z * n.lp.z);
    const Vector3f r = RotateNative(p.rot, scaled);
    w.pos = Vector3f(p.pos.x + r.x, p.pos.y + r.y, p.pos.z + r.z);
    w.rot = UnityNative::Multiply(p.rot, n.lr);
    w.scl = Vector3f(p.scl.x * n.ls.x, p.scl.y * n.ls.y, p.scl.z * n.ls.z);
    return w;
}

Vector3f Scaled(const Vector3f& s, const Vector3f& v) { return Vector3f(s.x * v.x, s.y * v.y, s.z * v.z); }
Vector3f Placed(const World& w, const Vector3f& local) {
    const Vector3f r = RotateNative(w.rot, Scaled(w.scl, local));
    return Vector3f(w.pos.x + r.x, w.pos.y + r.y, w.pos.z + r.z);
}

void Walk(const TNode& n, const World& parent, const std::string& parentPath, std::vector<PartColliderShape>& out) {
    const World w = Compose(parent, n);
    const std::string path = parentPath.empty() ? n.name : parentPath + "/" + n.name;
    if (n.src) {
        for (const ColliderChunk& c : n.src->colliders) {
            if (c.kind == ColliderKind::Wheel) continue;
            PartColliderShape s;
            s.path = path;
            s.entry = c.entry;
            s.kind = c.kind;
            s.trigger = c.trigger;
            s.tag = n.src->tag;
            s.layer = n.src->layer;
            s.nodePosition = w.pos;
            s.nodeRotation = w.rot;
            s.nodeScale = w.scl;
            const float ax = std::fabs(w.scl.x), ay = std::fabs(w.scl.y), az = std::fabs(w.scl.z);
            const float a[3] = {ax, ay, az};
            switch (c.kind) {
                case ColliderKind::Box: {
                    s.center = Placed(w, c.center);
                    const Vector3f sz = Scaled(w.scl, c.size);
                    s.halfExtents = Vector3f(std::fabs(sz.x) * 0.5f, std::fabs(sz.y) * 0.5f, std::fabs(sz.z) * 0.5f);
                    break;
                }
                case ColliderKind::Sphere:
                    s.center = Placed(w, c.center);
                    s.radius = c.radius * std::max(ax, std::max(ay, az));
                    break;
                case ColliderKind::Capsule: {
                    s.center = Placed(w, c.center);
                    const int d = c.direction;
                    s.axis = d;
                    const int di = (d < 0 || d > 2) ? 1 : d;
                    float other = 0.0f;
                    for (int i = 0; i < 3; ++i)
                        if (i != di) other = std::max(other, a[i]);
                    s.radius = c.radius * other;
                    s.height = c.height * a[di];
                    s.halfCylinder = std::max(0.0f, s.height * 0.5f - s.radius);
                    break;
                }
                case ColliderKind::Mesh: {
                    s.vertices.reserve(c.mesh.vertices.size());
                    for (const Vector3f& v : c.mesh.vertices) s.vertices.push_back(Placed(w, v));
                    for (const auto& t : c.mesh.triangleLists) s.triangles.insert(s.triangles.end(), t.begin(), t.end());
                    break;
                }
                default:
                    break;
            }
            out.push_back(std::move(s));
        }
    }
    for (const auto& ch : n.children) Walk(*ch, w, path, out);
}

}  // namespace

Vector3f RotateNative(const Quaternionf& q, const Vector3f& v) {
    // Quaternion.operator*(Quaternion, Vector3) (Quaternion.cs), float per operation
    const float num = q.x * 2.0f, num2 = q.y * 2.0f, num3 = q.z * 2.0f;
    const float num4 = q.x * num, num5 = q.y * num2, num6 = q.z * num3;
    const float num7 = q.x * num2, num8 = q.x * num3, num9 = q.y * num3;
    const float num10 = q.w * num, num11 = q.w * num2, num12 = q.w * num3;
    float t[3];
    t[0] = (1.0f - (num5 + num6)) * v.x;
    t[1] = (num7 - num12) * v.y;
    t[2] = (num8 + num11) * v.z;
    const float rx = (t[0] + t[1]) + t[2];
    t[0] = (num7 + num12) * v.x;
    t[1] = (1.0f - (num4 + num6)) * v.y;
    t[2] = (num9 - num10) * v.z;
    const float ry = (t[0] + t[1]) + t[2];
    t[0] = (num8 - num11) * v.x;
    t[1] = (num9 + num10) * v.y;
    t[2] = (1.0f - (num4 + num5)) * v.z;
    const float rz = (t[0] + t[1]) + t[2];
    return Vector3f(rx, ry, rz);
}

std::vector<PartColliderShape> BuildPartColliders(float rescaleFactor, const std::vector<ModelInstance>& instances) {
    std::unique_ptr<TNode> model;
    bool any = false;
    for (const ModelInstance& mi : instances) {
        if (!mi.model) continue;
        if (!mi.fromModelNode) {
            // PartLoader.cs:1914-1927: the instance is renamed "model", position 0, rotation identity, scale rescaleFactor
            model = FromMu(mi.model->root);
            model->name = "model";
            model->lp = Vector3f();
            model->lr = Quaternionf::identity();
            model->ls = Vector3f(rescaleFactor, rescaleFactor, rescaleFactor);
            any = true;
            break;
        }
        if (!model) {
            model = std::make_unique<TNode>();  // gameObject2 = new GameObject("model"), localScale = Vector3.one * scaleFactor
            model->name = "model";
            model->ls = Vector3f(rescaleFactor, rescaleFactor, rescaleFactor);
        }
        auto inst = FromMu(mi.model->root);
        // NestToParent: position 0, rotation identity (TransformExtension.cs), then the cfg values
        inst->lp = Vector3f();
        inst->lr = Quaternionf::identity();
        inst->ls = mi.scale ? *mi.scale : Vector3f(1.0f, 1.0f, 1.0f);
        if (mi.name) inst->name = *mi.name;
        if (mi.position) inst->lp = *mi.position;
        if (mi.rotationEuler) inst->lr = UnityNative::Euler(*mi.rotationEuler);
        TNode* host = model.get();
        if (!mi.parent.empty()) {
            if (TNode* f = Find(model.get(), mi.parent)) host = f;
        }
        host->children.push_back(std::move(inst));
        any = true;
    }
    std::vector<PartColliderShape> out;
    if (!any || !model) return out;
    // the part transform is the identity in the prefab
    Walk(*model, World{}, std::string(), out);
    return out;
}

}  // namespace ksp::mu
