// WP-20b: the PartJoints of a vessel as data (PartJoint.cs, Part.cs autostruts, CModuleStrut.cs). See ksp/physics/vessel_physics.hpp.
//
// Ported KSP code: Mono float rule (fx / f32store) on the float statements, the Unity shims (Vector3f / Quaternionf operators)
// for the managed vector maths, UnityNative for the native ones (Quaternion.AngleAxis, Vector3.OrthoNormalize); no std:: maths.
// The Transform maths goes through PartTransform (TransformPoint / InverseTransformDirection ...): the native matrix rounding of
// Unity is not reproduced (WP-13 note); the joint anchors are therefore within float rounding of KSP's, not bit-exact.
#include <algorithm>
#include <limits>

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/modules/module_docking_node.hpp"
#include "ksp/modules/module_grapple_node.hpp"
#include "ksp/modules/module_strut.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/physics/vessel_physics.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/matrix_native.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp::physics {

namespace {

// PartJoint.cs:62-68 (private fields with initialisers)
constexpr float kRigidity = 1500.0f;
constexpr float kStackNodeFactor = 2.0f;
constexpr float kSrfNodeFactor = 0.8f;
constexpr float kBreakingForceModifier = 1.0f;
constexpr float kBreakingTorqueModifier = 1.0f;
constexpr int kStack = 0;      // AttachModes.STACK
constexpr int kSrfAttach = 1;  // AttachModes.SRF_ATTACH
constexpr int kFull = 0;       // Part.PhysicalSignificance.FULL

using TransformOf = std::function<const PartTransform&(const Part&)>;

// what one BuildPartJoints call reads besides the parts
struct BuildCtx {
    JointGlobals globals;
    TransformOf tf;
};

bool IsDockingNode(PartModule& m) { return dynamic_cast<ModuleDockingNode*>(&m) != nullptr; }

// The Part.cs / PartJoint.cs members ported here, as static functions of one file-local class (the C# instance is the first argument)
struct PartJointRules {
    static bool HasFreePivot(Part& p);
    static AttachNode* AttachedPartNode(Part* previousPart, Part* currentPart, bool& srfAttached);
    static bool HasFreePivotBetweenRoot(Part& self, Part*& lastPart, AttachNode*& nodeToParent, AttachNode*& nodeFromParent, bool& srfAttached);
    static bool HasFreePivotBetweenParentPart(Part& self, Part* endPart, Part*& lastPart, AttachNode*& nodeToParent, AttachNode*& nodeFromParent,
                                              bool& srfAttached);
    static Part* GetAutoStrutAnchor(Vessel& vessel, Part& self, AttachNode*& nodeToParent, AttachNode*& nodeFromParent, bool& srfAttached);
    static bool SetupJoint(PartJointSpec& j, const Vector3f& jointPos, int size, const BuildCtx& ctx);
    static bool Create(PartJointSpec& j, Part& owner, Part& parent, AttachNode* nodeToParent, AttachNode* nodeFromParent, int mode, const BuildCtx& ctx);
    static void SetUnbreakable(PartJointSpec& j);
    static bool SecureAutoStrut(PartJointSpec& j, Part& self, Part& anchor, AttachNode* nodeToParent, AttachNode* nodeFromParent, bool srfAttached,
                                const BuildCtx& ctx, AttachNode& storage);
};

// Part.cs:8511 HasFreePivot: a module that is an IJointLockState with IsJointUnlocked(). kspcore's ported implementers are
// ModuleDockingNode and ModuleGrappleNode (BaseServo / ModuleJointMotor, the robotics, are not ported: WP-52).
bool PartJointRules::HasFreePivot(Part& p) {
    PartModuleList& modules = p.Modules();
    size_t count = modules.Count();
    while (count-- > 0) {
        PartModule* m = modules.GetModule(count);
        if (auto* d = dynamic_cast<ModuleDockingNode*>(m)) {
            if (d->IsJointUnlocked()) return true;
        } else if (auto* g = dynamic_cast<ModuleGrappleNode*>(m)) {
            if (g->IsJointUnlocked()) return true;
        }
    }
    return false;
}

// Part.cs:8551 AttachedPartNode
AttachNode* PartJointRules::AttachedPartNode(Part* previousPart, Part* currentPart, bool& srfAttached) {
    AttachNode* result = nullptr;
    srfAttached = false;
    if (previousPart == nullptr || currentPart == nullptr) return result;
    for (size_t i = 0; i < currentPart->attachNodes.size(); i++) {
        AttachNode& n = currentPart->attachNodes[i];
        if (n.attachedPart != nullptr && n.attachedPart->fields.persistentId == previousPart->fields.persistentId) {
            result = &n;
            break;
        }
    }
    if (currentPart->srfAttachNode && currentPart->srfAttachNode->attachedPart != nullptr &&
        currentPart->srfAttachNode->attachedPart->fields.persistentId == previousPart->fields.persistentId) {
        result = currentPart->srfAttachNode.get();
        srfAttached = true;
    } else if (previousPart->srfAttachNode && previousPart->srfAttachNode->attachedPart != nullptr &&
               previousPart->srfAttachNode->attachedPart->fields.persistentId == currentPart->fields.persistentId) {
        result = previousPart->srfAttachNode.get();
        srfAttached = true;
    }
    return result;
}

// Part.cs:8525 HasFreePivotBetweenRoot
bool PartJointRules::HasFreePivotBetweenRoot(Part& self, Part*& lastPart, AttachNode*& nodeToParent, AttachNode*& nodeFromParent, bool& srfAttached) {
    Part* previousPart = &self;
    srfAttached = false;
    nodeToParent = nodeFromParent = nullptr;
    lastPart = &self;
    nodeToParent = AttachedPartNode(self.parent, &self, srfAttached);
    Part* part = self.parent;
    while (part != nullptr && part->parent != nullptr) {
        if (!HasFreePivot(*part)) {
            previousPart = part;
            part = part->parent;
            continue;
        }
        if (part->isRobotic()) {
            nodeFromParent = AttachedPartNode(previousPart, part, srfAttached);
            lastPart = part;
        }
        return true;
    }
    return false;
}

// Part.cs:8580 HasFreePivotBetweenParentPart
bool PartJointRules::HasFreePivotBetweenParentPart(Part& self, Part* endPart, Part*& lastPart, AttachNode*& nodeToParent, AttachNode*& nodeFromParent,
                                   bool& srfAttached) {
    Part* previousPart = &self;
    srfAttached = false;
    nodeToParent = nodeFromParent = nullptr;
    lastPart = &self;
    nodeToParent = AttachedPartNode(self.parent, &self, srfAttached);
    Part* part = self.parent;
    while (part != nullptr && part->parent != nullptr) {
        if (!HasFreePivot(*part)) {
            if (part->parent->fields.persistentId != endPart->fields.persistentId) {
                previousPart = part;
                part = part->parent;
                continue;
            }
            if (!HasFreePivot(*part->parent)) break;
            if (part->parent->isRobotic()) {
                nodeFromParent = AttachedPartNode(part, part->parent, srfAttached);
                lastPart = part->parent;
            }
            return true;
        }
        if (part->isRobotic()) {
            nodeFromParent = AttachedPartNode(previousPart, part, srfAttached);
            lastPart = part;
        }
        return true;
    }
    return false;
}

// Part.cs:8280 GetAutoStrutAnchor. Heaviest / ForceHeaviest: the spike's rule (see the header), not GetMassivePart.
Part* PartJointRules::GetAutoStrutAnchor(Vessel& vessel, Part& self, AttachNode*& nodeToParent, AttachNode*& nodeFromParent, bool& srfAttached) {
    nodeToParent = nodeFromParent = nullptr;
    srfAttached = false;
    Part* lastPart = nullptr;
    bool flag = false;
    switch (self.fields.autoStrutMode) {
        default:
            return nullptr;
        case 1:    // Root
        case 4: {  // ForceRoot
            Part* part = vessel.rootPart;  // HighLogic.LoadedSceneIsFlight
            flag = HasFreePivotBetweenRoot(self, lastPart, nodeToParent, nodeFromParent, srfAttached);
            if (!((self.fields.autoStrutExcludeParent && self.parent != nullptr && self.parent == part) || flag)) return part;
            return lastPart;
        }
        case 2:    // Heaviest
        case 5: {  // ForceHeaviest: the heaviest rigid body (rb.mass, MassivePartCheck's in-flight weight), first in vessel order on a tie
            Part* best = nullptr;
            float bestMass = 0.0f;
            for (Part* p : vessel.parts) {
                if (p == &self || !p->hasRigidbody) continue;
                if (self.fields.autoStrutExcludeParent && p == self.parent) continue;
                const float m = PartRigidbodyMass(*p);
                if (best == nullptr || m > bestMass) {
                    best = p;
                    bestMass = m;
                }
            }
            return best;
        }
        case 3:    // Grandparent
        case 6: {  // ForceGrandparent
            if (self.parent != nullptr && self.parent->parent != nullptr) {
                if ((flag = HasFreePivotBetweenParentPart(self, self.parent->parent, lastPart, nodeToParent, nodeFromParent, srfAttached)))
                    return lastPart;
                return self.parent->parent;
            }
            if (self.parent != nullptr) {
                if ((flag = HasFreePivotBetweenParentPart(self, self.parent, lastPart, nodeToParent, nodeFromParent, srfAttached)))
                    return lastPart;
                return self.parent;
            }
            return nullptr;
        }
    }
}

// PartJoint.cs:215 SetupJoint (to :305): the connected body and the joint numbers. Returns false where the C# returns null (no
// ConfigurableJoint). Robotic servo rigid bodies (:220-243) are WP-52's: kspcore has no servo, the target's Rigidbody is used.
bool PartJointRules::SetupJoint(PartJointSpec& j, const Vector3f& jointPos, int size, const BuildCtx& ctx) {
    Part* rigidbody = RigidBodyPart(j.target);  // target.Rigidbody
    Part* hostRb = RigidBodyPart(j.host);
    Part* connected = nullptr;
    if (rigidbody != hostRb) {
        connected = rigidbody;
    } else if (j.target->parent != nullptr && RigidBodyPart(j.target->parent) != nullptr && RigidBodyPart(j.target->parent) != rigidbody) {
        connected = RigidBodyPart(j.target->parent);
    } else {
        if (!(j.target->parent != nullptr && j.target->parent->parent != nullptr && RigidBodyPart(j.target->parent->parent) != rigidbody))
            return false;
        connected = RigidBodyPart(j.target->parent->parent);
    }
    if (connected == nullptr) return false;
    j.hostBody = j.host;  // host.gameObject (the host is FULL: PartJoint.create picks it so)
    j.targetBody = connected;
    float num3 = 0.0f;
    if (j.mode == kStack) {
        num3 = f32store((fx(static_cast<float>(size)) + fx(1.0f)) * fx(kStackNodeFactor));
        j.stiffness = f32store(fx(kRigidity) * fx(num3) * fx(ctx.globals.stackAttachStiffness));
    } else if (j.mode == kSrfAttach) {
        num3 = f32store((fx(static_cast<float>(size)) + fx(1.0f)) * fx(kSrfNodeFactor));
        j.stiffness = f32store(fx(kRigidity) * fx(num3) * fx(ctx.globals.srfAttachStiffness));
    }
    j.breakingForce = f32store(fx(Mathf::Min(j.host->fields.breakingForce, j.target->fields.breakingForce)) * fx(kBreakingForceModifier) *
                               fx(num3) / fx(static_cast<float>(j.internalJoints)));
    j.breakingTorque = f32store(fx(Mathf::Min(j.host->fields.breakingTorque, j.target->fields.breakingTorque)) * fx(kBreakingTorqueModifier) *
                                fx(num3) / fx(static_cast<float>(j.internalJoints)));
    (void)jointPos;
    return true;
}

// PartJoint.cs:96 Create, with create (:139) inlined. Returns false when no joint results (create's error branch, or no
// ConfigurableJoint could be set up).
bool PartJointRules::Create(PartJointSpec& j, Part& owner, Part& parent, AttachNode* nodeToParent, AttachNode* nodeFromParent, int mode,
                     const BuildCtx& ctx) {
    const TransformOf& tf = ctx.tf;
    AttachNode* attachNode = nodeToParent;
    const Part* nodeSpace = &owner;
    bool flag = false;
    if (nodeToParent == nullptr) {
        attachNode = nodeFromParent;
        nodeSpace = &parent;
        if (attachNode != nullptr) flag = attachNode->rigid;
    } else if (nodeFromParent == nullptr) {
        attachNode = nodeToParent;
        nodeSpace = &owner;
        flag = attachNode->rigid;
    } else {
        if (nodeToParent->size <= nodeFromParent->size) {
            attachNode = nodeToParent;
            nodeSpace = &owner;
        } else {
            attachNode = nodeFromParent;
            nodeSpace = &parent;
        }
        flag = nodeToParent->rigid || nodeFromParent->rigid;
    }
    if (attachNode == nullptr) {
        attachNode = parent.srfAttachNode.get();
        nodeSpace = &parent;
        if (attachNode == nullptr) return false;  // the C# dereferences a null srfAttachNode (NullReferenceException)
        flag = attachNode->rigid;
    }
    // ---- create (:138)
    Part& child = owner;
    Part* host = nullptr;
    if (child.fields.physicalSignificance == kFull && parent.fields.physicalSignificance == kFull) {
        host = &child;
    } else if (child.fields.physicalSignificance != kFull) {
        host = &parent;
    } else {
        if (parent.fields.physicalSignificance == kFull) return false;  // "[PartJoint]: Cannot create a PartJoint between two physicsless parts"
        host = &child;
    }
    j.child = &child;
    j.parent = &parent;
    j.host = host;
    j.target = host == &child ? &parent : &child;
    const PartTransform& childT = tf(child);
    const PartTransform& spaceT = tf(*nodeSpace);
    const Vector3f nodePos = childT.InverseTransformPoint(spaceT.TransformPoint(attachNode->position));
    Vector3f nodeOrt = childT.InverseTransformDirection(spaceT.TransformDirection(attachNode->orientation));
    Vector3f nodeOrt2 = childT.InverseTransformDirection(spaceT.TransformDirection(attachNode->secondaryAxis));
    j.mode = mode;
    j.rigid = flag;
    j.nodeSize = attachNode->size;
    if (VectorExtensions::IsZero(nodeOrt2)) UnityNative::OrthoNormalize(nodeOrt, nodeOrt2);
    j.axis = nodeOrt;
    j.secAxis = nodeOrt2;
    const int nodeSize = attachNode->size;
    const bool dockingEitherSide = child.HasModuleImplementing(IsDockingNode) || parent.HasModuleImplementing(IsDockingNode);
    if (nodeSize >= 2 && !dockingEitherSide) {
        j.internalJoints = 3;
        int made = 0;
        for (int i = 0; i < 3; i++) {
            const float radius = f32store(fx(static_cast<float>(nodeSize)) - fx(1.0f));
            const Vector3f pos = nodePos + UnityNative::AngleAxis(f32store(fx(120.0f) * fx(static_cast<float>(i))), nodeOrt) * (nodeOrt2 * radius);
            if (SetupJoint(j, pos, nodeSize, ctx)) j.anchors[static_cast<size_t>(made++)] = pos;
            else j.internalJoints--;
        }
    } else {
        j.internalJoints = 1;
        if (SetupJoint(j, nodePos, nodeSize, ctx)) j.anchors[0] = nodePos;
        else j.internalJoints = 0;
    }
    return j.internalJoints > 0;  // goodSetup; a PartJoint without ConfigurableJoints holds nothing
}

// PartJoint.cs:315 SetUnbreakable(false, forceRigid) (the branch :332-342): the PhysX break limits in flight (Part.ResetJoints, Part.cs:3815)
void PartJointRules::SetUnbreakable(PartJointSpec& j) {
    const bool flag = j.rigid || j.forceRigid;
    if (PhysicsGlobals::Instance() == nullptr) {  // no Physics.cfg loaded: the factors' C# defaults are not known here
        j.breakForce = j.breakingForce;
        j.breakTorque = j.breakingTorque;
        return;
    }
    j.breakForce = f32store(fx(j.breakingForce) * fx(PhysicsGlobals::JointBreakForceFactor()) *
                            fx(flag ? PhysicsGlobals::RigidJointBreakForceFactor() : 1.0f));
    j.breakTorque = f32store(fx(j.breakingTorque) * fx(PhysicsGlobals::JointBreakTorqueFactor()) *
                             fx(flag ? PhysicsGlobals::RigidJointBreakTorqueFactor() : 1.0f));
}

// Part.cs:8098 SecureAutoStrut: the AutoStrut node and PartJoint.Create(this, anchor, node, nodeFromParent, SRF_ATTACH)
bool PartJointRules::SecureAutoStrut(PartJointSpec& j, Part& self, Part& anchor, AttachNode* nodeToParent, AttachNode* nodeFromParent, bool srfAttached,
                     const BuildCtx& ctx, AttachNode& storage) {
    const TransformOf& tf = ctx.tf;
    AttachNode attachNode;
    attachNode.id = "AutoStrut";
    attachNode.attachedPart = &anchor;
    attachNode.nodeType = AttachNodeType::Stack;
    if (nodeToParent != nullptr) {
        if (srfAttached) {
            attachNode.nodeType = AttachNodeType::Surface;
            attachNode.srfAttachMeshName = nodeToParent->srfAttachMeshName;
        } else {
            attachNode.id = nodeToParent->id;
        }
    }
    attachNode.attachMethod = AttachNodeMethod::FIXED_JOINT;
    attachNode.breakingForce = std::numeric_limits<float>::max();
    attachNode.breakingTorque = std::numeric_limits<float>::max();
    attachNode.position = self.fields.strutOffset;
    attachNode.orientation = tf(anchor).InverseTransformDirection((tf(self).position - tf(anchor).position).normalized());
    attachNode.ResourceXFeed = false;
    attachNode.overrideDragArea = 0.0f;
    attachNode.owner = &self;
    attachNode.size = 1;
    storage = attachNode;
    return Create(j, self, anchor, &storage, nodeFromParent, kSrfAttach, ctx);
}

}  // namespace

std::vector<PartJointSpec> BuildPartJoints(Vessel& vessel, const JointGlobals& globals,
                                           const std::function<const PartTransform&(const Part&)>& transformOf) {
    BuildCtx ctx;
    ctx.globals = globals;
    ctx.tf = transformOf ? transformOf : TransformOf([](const Part& p) -> const PartTransform& { return p.partTransform; });
    std::vector<PartJointSpec> out;
    // ---- attach joints: Part.Start -> CreateAttachJoint(attachMode) for FULL parts (Part.cs:1893-1896, 2197-2209)
    for (Part* p : vessel.parts) {
        if (p->fields.physicalSignificance != kFull || p->parent == nullptr || !p->isAttached()) continue;
        AttachNode* nodeToParent = p->FindAttachNodeByPart(p->parent);
        AttachNode* nodeFromParent = p->parent->FindAttachNodeByPart(p);
        PartJointSpec j;
        j.kind = JointKind::Attach;
        if (!PartJointRules::Create(j, *p, *p->parent, nodeToParent, nodeFromParent, p->fields.attachMode, ctx)) continue;
        j.forceRigid = p->fields.rigidAttachment;
        PartJointRules::SetUnbreakable(j);
        out.push_back(j);
    }
    // ---- autostruts: Part.CycleAutoStrut -> SecureAutoStruts (Part.cs:8044-8091) for every part with autoStrutMode != Off
    // (the EVA-construction exclusion of :8066 does not apply outside construction mode)
    for (Part* p : vessel.parts) {
        if (p->fields.autoStrutMode == 0) continue;
        AttachNode* nodeToParent = nullptr;
        AttachNode* nodeFromParent = nullptr;
        bool srfAttached = false;
        Part* anchor = PartJointRules::GetAutoStrutAnchor(vessel, *p, nodeToParent, nodeFromParent, srfAttached);
        if (anchor == nullptr || anchor == p) continue;
        AttachNode node;
        PartJointSpec j;
        j.kind = JointKind::AutoStrut;
        if (PartJointRules::SecureAutoStrut(j, *p, *anchor, nodeToParent, nodeFromParent, srfAttached, ctx, node)) {
            j.breakForce = j.breakTorque = std::numeric_limits<float>::infinity();  // SetupJoint's +inf; autostruts are never SetUnbreakable'd
            out.push_back(j);
        }
        if (!p->isRobotic() && !anchor->isRobotic() &&
            std::find(anchor->symmetryCounterparts.begin(), anchor->symmetryCounterparts.end(), p) == anchor->symmetryCounterparts.end()) {
            size_t count = anchor->symmetryCounterparts.size();
            while (count-- > 0) {
                Part* cp = anchor->symmetryCounterparts[count];
                Part* lastPart = nullptr;
                if (!PartJointRules::HasFreePivotBetweenParentPart(*p, cp, lastPart, nodeToParent, nodeFromParent, srfAttached)) {
                    if (p->symmetryCounterparts.size() >= count && cp->isRobotic() && count < p->symmetryCounterparts.size())
                        nodeToParent = PartJointRules::AttachedPartNode(p->symmetryCounterparts[count], cp, srfAttached);
                    AttachNode node2;
                    PartJointSpec j2;
                    j2.kind = JointKind::AutoStrut;
                    if (PartJointRules::SecureAutoStrut(j2, *p, *cp, nodeToParent, nodeFromParent, srfAttached, ctx, node2)) {
                        j2.breakForce = j2.breakTorque = std::numeric_limits<float>::infinity();
                        out.push_back(j2);
                    }
                }
            }
        }
    }
    // ---- struts: CModuleStrut.OnTargetSet (CompoundParts/CModuleStrut.cs:26-49): PartJoint.Create(jointRoot, jointTarget,
    // targetNode, null, SRF_ATTACH) when both sides are different rigid bodies (WP-25 fills jointRoot / jointTarget / targetNode)
    for (Part* p : vessel.parts) {
        for (size_t m = 0; m < p->Modules().Count(); m++) {
            auto* strut = dynamic_cast<CModuleStrut*>(p->Modules().GetModule(m));
            if (strut == nullptr || strut->jointRoot == nullptr || strut->jointTarget == nullptr || strut->jointRoot == strut->jointTarget) continue;
            AttachNode* node = const_cast<AttachNode*>(strut->TargetNode());
            if (node == nullptr) continue;
            PartJointSpec j;
            j.kind = JointKind::Strut;
            if (PartJointRules::Create(j, *strut->jointRoot, *strut->jointTarget, node, nullptr, kSrfAttach, ctx)) {
                j.breakForce = j.breakTorque = std::numeric_limits<float>::infinity();
                out.push_back(j);
            }
        }
    }
    return out;
}

}  // namespace ksp::physics
