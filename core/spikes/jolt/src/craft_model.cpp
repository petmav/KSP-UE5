#include "craft_model.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <unordered_map>

#include "ksp_context.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/physics/physics_globals.hpp"

namespace spike {

using JPH::DVec3;
using JPH::Quat;
using JPH::Vec3;

int CraftModel::SubJointCount() const {
    int n = 0;
    for (const JointSpec& j : joints) n += j.internalJoints;
    return n;
}

namespace {

DVec3 ToD(const ksp::Vector3f& v) { return DVec3(v.x, v.y, v.z); }

// PartJoint.cs:62-68
constexpr float kRigidity = 1500.0f;
constexpr float kStackNodeFactor = 2.0f;
constexpr float kSrfNodeFactor = 0.8f;
// FlightGlobals.StackAttachStiffNess / SrfAttachStiffNess: scene-serialized, 10 / 10 [runtime] (PROFILE.md section 6)
constexpr float kStackAttachStiffness = 10.0f;
constexpr float kSrfAttachStiffness = 10.0f;

// Unity Vector3.OrthoNormalize(ref normal, ref tangent) for a zero tangent: any unit vector perpendicular to normal.
Vec3 AnyPerpendicular(Vec3 n) {
    Vec3 t = std::abs(n.GetY()) < 0.9f ? Vec3::sAxisY().Cross(n) : Vec3::sAxisX().Cross(n);
    return t.Normalized();
}

}  // namespace

CraftModel BuildCraftModel(KspContext& ctx, const std::string& craftPath, const std::string& name) {
    CraftModel m;
    m.name = name;
    m.vessel = ctx.LoadCraft(craftPath);
    ksp::Vessel& v = *m.vessel;
    const size_t n = v.parts.size();
    std::unordered_map<const ksp::Part*, int> index;
    for (size_t i = 0; i < n; i++) index[v.parts[i]] = static_cast<int>(i);
    m.parts.resize(n);

    // ---- parts: tree, physical significance (Part.cs:1788-1834), transforms, collider proxy, engines
    for (size_t i = 0; i < n; i++) {
        ksp::Part* p = v.parts[i];
        PartSpec& s = m.parts[i];
        s.part = p;
        s.name = p->partInfo ? p->partInfo->name : p->gameObjectName;
        s.parent = p->parent ? index.at(p->parent) : -1;
        if (s.parent < 0) m.root = static_cast<int>(i);
        const int sig = std::max(0, p->fields.PhysicsSignificance);
        s.physical = (sig == 0) || p->parent == nullptr;  // the root is always FULL (Part.cs:1830-1834)
        p->fields.physicalSignificance = s.physical ? 0 : 1;
        p->hasRigidbody = s.physical;
        s.pos = ToD(p->partTransform.position);
        s.rot = ToJ(p->partTransform.rotation);
        s.comLocal = ToJ(p->fields.CoMOffset);
        s.autoStrutMode = p->fields.autoStrutMode;
        s.isDecoupler = p->isDecoupler() || p->isAnchoredDecoupler();
        if (!p->DragCubes.None() && !p->DragCubes.Cubes().empty()) {
            const ksp::DragCube& c = p->DragCubes.Cubes().front();
            s.boxCenter = ToJ(c.Center());
            const Vec3 size = ToJ(c.Size());
            s.boxHalf = Vec3::sMax(size * 0.5f, Vec3::sReplicate(0.05f));
        }
        const Vec3 h = s.boxHalf;
        // solid box tensor per unit mass: (hy^2 + hz^2) / 3 ...
        s.inertiaPerMass = Vec3((h.GetY() * h.GetY() + h.GetZ() * h.GetZ()) / 3.0f, (h.GetX() * h.GetX() + h.GetZ() * h.GetZ()) / 3.0f,
                                (h.GetX() * h.GetX() + h.GetY() * h.GetY()) / 3.0f);
        if (auto* e = dynamic_cast<ksp::ModuleEngines*>(
                p->FindModuleImplementing([](ksp::PartModule& pm) { return dynamic_cast<ksp::ModuleEngines*>(&pm) != nullptr; }))) {
            s.isEngine = true;
            s.maxThrust = e->maxThrust;
        }
    }
    for (size_t i = 0; i < n; i++)
        if (m.parts[i].parent >= 0) m.parts[m.parts[i].parent].children.push_back(static_cast<int>(i));
    for (size_t i = 0; i < n; i++) {
        int o = static_cast<int>(i);
        while (!m.parts[o].physical) o = m.parts[o].parent;
        m.parts[i].rbOwner = o;
    }

    // ---- mass: FlightIntegrator.UpdateMassStats through kspcore (WP-14 Vessel::UpdateMassStats, exact port)
    v.UpdateMassStats();
    const float rbMin = ksp::PhysicsGlobals::PartRBMassMin();
    for (size_t i = 0; i < n; i++) {
        PartSpec& s = m.parts[i];
        s.ownMass = s.part->fields.mass + s.part->GetResourceMass();
        s.physicsMass = s.part->fields.physicsMass;
        s.rbMass = s.physical ? std::max(rbMin, static_cast<float>(s.physicsMass)) : 0.0f;
        m.totalMass += s.ownMass;
        if (s.physical) m.physicalCount++;
    }

    // ---- attach joints: Part.CreateAttachJoint -> PartJoint.Create (PartJoint.cs:96-213), SetupJoint (:215-313)
    const float breakF = ksp::PhysicsGlobals::JointBreakForceFactor();
    const float breakT = ksp::PhysicsGlobals::JointBreakTorqueFactor();
    auto makeJoint = [&](int child, int parent, const ksp::AttachNode* nodeToParent, const ksp::AttachNode* nodeFromParent, int mode,
                         bool autostrut) -> JointSpec {
        const PartSpec& cs = m.parts[child];
        const PartSpec& ps = m.parts[parent];
        const ksp::AttachNode* node = nodeToParent;
        const ksp::Part* nodeSpace = cs.part;
        bool rigid = false;
        if (nodeToParent == nullptr) {
            node = nodeFromParent;
            nodeSpace = ps.part;
            if (node) rigid = node->rigid;
        } else if (nodeFromParent == nullptr) {
            rigid = node->rigid;
        } else {
            if (nodeToParent->size <= nodeFromParent->size) {
                node = nodeToParent;
                nodeSpace = cs.part;
            } else {
                node = nodeFromParent;
                nodeSpace = ps.part;
            }
            rigid = nodeToParent->rigid || nodeFromParent->rigid;
        }
        if (node == nullptr) {
            node = ps.part->srfAttachNode.get();
            nodeSpace = ps.part;
            if (node == nullptr) throw std::runtime_error("joint without a node: " + cs.name + " -> " + ps.name);
            rigid = node->rigid;
        }
        rigid = rigid || cs.part->fields.rigidAttachment;
        JointSpec j;
        j.child = child;
        j.parent = parent;
        j.autostrut = autostrut;
        j.mode = mode;
        j.rigid = rigid;
        j.nodeSize = node->size;
        // host: the child when both are FULL (PartJoint.cs:142-159); bodies are the rigidbody owners
        const bool childFull = cs.physical, parentFull = ps.physical;
        int host = child, target = parent;
        if (!(childFull && parentFull) && !childFull) std::swap(host, target);
        j.host = m.parts[host].rbOwner;
        j.target = m.parts[target].rbOwner;
        const ksp::PartTransform& nt = nodeSpace->partTransform;
        const DVec3 nodePos = ToD(nt.TransformPoint(node->position));
        Vec3 ort = ToJ(nt.TransformDirection(node->orientation));
        Vec3 ort2 = ToJ(nt.TransformDirection(node->secondaryAxis));
        ort = ort.IsNearZero() ? Vec3::sAxisY() : ort.Normalized();
        if (ort2.IsNearZero()) ort2 = AnyPerpendicular(ort);
        ort2 = (ort2 - ort * ort.Dot(ort2)).Normalized();
        j.axis = ort;
        j.secAxis = ort2;
        const bool docking = cs.part->isDockingPort() || ps.part->isDockingPort();
        if (node->size >= 2 && !docking) {
            j.internalJoints = 3;
            j.ringRadius = static_cast<float>(node->size) - 1.0f;
            for (int k = 0; k < 3; k++) {
                const Vec3 off = Quat::sRotation(ort, JPH::DegreesToRadians(120.0f * k)) * (ort2 * j.ringRadius);
                j.anchors[k] = nodePos + DVec3(off.GetX(), off.GetY(), off.GetZ());
            }
        } else {
            j.internalJoints = 1;
            j.anchors[0] = nodePos;
        }
        float num3 = 0.0f;
        if (mode == 0) {
            num3 = (static_cast<float>(node->size) + 1.0f) * kStackNodeFactor;
            j.stiffness = kRigidity * num3 * kStackAttachStiffness;
        } else {
            num3 = (static_cast<float>(node->size) + 1.0f) * kSrfNodeFactor;
            j.stiffness = kRigidity * num3 * kSrfAttachStiffness;
        }
        const ksp::Part* hp = m.parts[j.host].part;
        const ksp::Part* tp = m.parts[j.target].part;
        float bf = std::min(hp->fields.breakingForce, tp->fields.breakingForce) * num3 / static_cast<float>(j.internalJoints);
        float bt = std::min(hp->fields.breakingTorque, tp->fields.breakingTorque) * num3 / static_cast<float>(j.internalJoints);
        if (autostrut) {  // SecureAutoStrut: node breakingForce = float.MaxValue, and autostruts are never made breakable (Part.cs:8098)
            bf = bt = std::numeric_limits<float>::infinity();
        }
        j.breakForceSub = bf * breakF * (rigid ? ksp::PhysicsGlobals::RigidJointBreakForceFactor() : 1.0f);
        j.breakTorqueSub = bt * breakT * (rigid ? ksp::PhysicsGlobals::RigidJointBreakTorqueFactor() : 1.0f);
        return j;
    };

    for (size_t i = 0; i < n; i++) {
        PartSpec& s = m.parts[i];
        if (!s.physical || s.parent < 0) continue;
        ksp::Part* p = s.part;
        const ksp::AttachNode* toParent = p->FindAttachNodeByPart(p->parent);
        const ksp::AttachNode* fromParent = p->parent->FindAttachNodeByPart(p);
        JointSpec j = makeJoint(static_cast<int>(i), s.parent, toParent, fromParent, p->fields.attachMode, false);
        if (j.host == j.target) continue;  // both ends on the same body (physicsless chain)
        s.attachJoint = static_cast<int>(m.joints.size());
        m.joints.push_back(j);
    }
    m.attachJointCount = static_cast<int>(m.joints.size());

    // ---- autostruts: GetAutoStrutAnchor (Part.cs:8290-8338) for Grandparent / Root; SecureAutoStrut (:8098)
    for (size_t i = 0; i < n; i++) {
        PartSpec& s = m.parts[i];
        if (!s.physical || s.parent < 0 || s.autoStrutMode == 0) continue;
        int anchor = -1;
        switch (s.autoStrutMode) {
            case 1: case 4: anchor = m.root; break;                                                         // Root / ForceRoot
            case 3: case 6: anchor = m.parts[s.parent].parent >= 0 ? m.parts[s.parent].parent : s.parent; break;  // Grandparent
            case 2: case 5: {                                                                              // Heaviest
                float best = -1.0f;
                for (size_t k = 0; k < n; k++)
                    if (k != i && m.parts[k].physical && m.parts[k].rbMass > best) { best = m.parts[k].rbMass; anchor = static_cast<int>(k); }
                break;
            }
            default: break;
        }
        if (anchor < 0 || anchor == static_cast<int>(i)) continue;
        anchor = m.parts[anchor].rbOwner;
        ksp::AttachNode strutNode;  // Part.cs:8100-8125: size 1, position strutOffset (0), orientation towards the part
        strutNode.id = "AutoStrut";
        strutNode.size = 1;
        strutNode.position = ksp::Vector3f();
        const ksp::Vector3f d = s.part->partTransform.position - m.parts[anchor].part->partTransform.position;
        strutNode.orientation = m.parts[anchor].part->partTransform.InverseTransformDirection(d.normalized());
        JointSpec j = makeJoint(static_cast<int>(i), anchor, &strutNode, nullptr, 1, true);
        if (j.host == j.target) continue;
        m.joints.push_back(j);
    }
    m.autostrutCount = static_cast<int>(m.joints.size()) - m.attachJointCount;
    return m;
}

}  // namespace spike
