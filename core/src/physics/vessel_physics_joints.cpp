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
#include "ksp/serenity/base_servo.hpp"
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
    const double* fixedTime = nullptr;  // Time.fixedTime for GetMassivePart's per-part cache (null: no cache)
};

bool IsDockingNode(PartModule& m) { return dynamic_cast<ModuleDockingNode*>(&m) != nullptr; }

// The Part.cs / PartJoint.cs members ported here, as static functions of one file-local class (the C# instance is the first argument)
struct PartJointRules {
    static bool HasFreePivot(Part& p);
    static AttachNode* AttachedPartNode(Part* previousPart, Part* currentPart, bool& srfAttached);
    static bool HasFreePivotBetweenRoot(Part& self, Part*& lastPart, AttachNode*& nodeToParent, AttachNode*& nodeFromParent, bool& srfAttached);
    static bool HasFreePivotBetweenParentPart(Part& self, Part* endPart, Part*& lastPart, AttachNode*& nodeToParent, AttachNode*& nodeFromParent,
                                              bool& srfAttached);
    static Part* GetAutoStrutAnchor(Vessel& vessel, Part& self, AttachNode*& nodeToParent, AttachNode*& nodeFromParent, bool& srfAttached,
                                    const BuildCtx& ctx);
    static Part* GetMassivePart(Part& self, Part& start, Part*& lastPart, AttachNode*& nodeToParent, AttachNode*& nodeFromParent, bool& srfAttached,
                                const std::vector<Part*>& excluded, const BuildCtx& ctx);
    static void GetMassiveParentPart(Part& original, Part* child, Part*& highestPart, float& highestMass, Part*& lastPart, AttachNode*& nodeToParent,
                                     AttachNode*& nodeFromParent, bool& srfAttached, const std::vector<Part*>& excluded, const BuildCtx& ctx);
    static void GetMassiveChildPart(Part& original, Part& parent, const std::vector<Part*>& children, Part*& highestPart, float& highestMass,
                                    Part*& lastPart, AttachNode*& nodeToParent, AttachNode*& nodeFromParent, bool& srfAttached, const Part* ignoredChild,
                                    const std::vector<Part*>& excluded, const BuildCtx& ctx);
    static void MassivePartCheck(Part& original, Part& p, Part*& highestPart, float& highestMass, const std::vector<Part*>& excluded, const BuildCtx& ctx);
    static bool SetupJoint(PartJointSpec& j, const Vector3f& jointPos, int size, const BuildCtx& ctx);
    static bool Create(PartJointSpec& j, Part& owner, Part& parent, AttachNode* nodeToParent, AttachNode* nodeFromParent, int mode, const BuildCtx& ctx);
    static void SetUnbreakable(PartJointSpec& j);
    static bool SecureAutoStrut(PartJointSpec& j, Part& self, Part& anchor, AttachNode* nodeToParent, AttachNode* nodeFromParent, bool srfAttached,
                                const BuildCtx& ctx, AttachNode& storage);
};

// Part.cs:8511 HasFreePivot: a module that is an IJointLockState with IsJointUnlocked(). kspcore's ported implementers are
// ModuleDockingNode, ModuleGrappleNode and BaseServo (the robotic servos, WP-52; ModuleJointMotor is the only other C# implementer, not ported).
bool PartJointRules::HasFreePivot(Part& p) {
    PartModuleList& modules = p.Modules();
    size_t count = modules.Count();
    while (count-- > 0) {
        PartModule* m = modules.GetModule(count);
        if (auto* d = dynamic_cast<ModuleDockingNode*>(m)) {
            if (d->IsJointUnlocked()) return true;
        } else if (auto* g = dynamic_cast<ModuleGrappleNode*>(m)) {
            if (g->IsJointUnlocked()) return true;
        } else if (auto* servo = dynamic_cast<Expansions::Serenity::BaseServo*>(m)) {
            if (servo->IsJointUnlocked()) return true;  // BaseServo.IsJointUnlocked: !servoIsLocked (BaseServo.cs:1790)
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

// Part.cs:8456 MassivePartCheck. The weight is rb.mass in flight (p.mass + p.GetResourceMass() without a Rigidbody); an approximately
// equal weight goes to the part nearer to `original`, and between equidistant ones (0.05 m) to the smaller Dot(original.up, lhs x position).
// highestMass = -1 (the new part wins) is set BEFORE the exclusion test, as in the C#: an excluded part that ties and wins leaves -1, so
// the next part of any weight is taken. A cache hit (GetMassivePart) with highestPart still null throws, as the C# does
// (highestPart.transform): the SecureAutoStruts coroutine dies and the part gets no autostrut this cycle.
void PartJointRules::MassivePartCheck(Part& original, Part& p, Part*& highestPart, float& highestMass, const std::vector<Part*>& excluded,
                                      const BuildCtx& ctx) {
    const TransformOf& tf = ctx.tf;
    float num = !p.hasRigidbody ? f32store(fx(p.fields.mass) + fx(p.GetResourceMass())) : PartRigidbodyMass(p);  // !LoadedSceneIsEditor
    if (Mathf::Approximately(num, highestMass)) {
        if (highestPart == nullptr) throw NullReferenceException("Object reference not set to an instance of an object");
        const Vector3f position = tf(original).position;
        const Vector3f position2 = tf(*highestPart).position;
        const Vector3f position3 = tf(p).position;
        const float num2 = Vector3f::Distance(position2, position);
        const float num3 = Vector3f::Distance(position3, position);
        if (static_cast<double>(Mathf::Abs(f32store(fx(num2) - fx(num3)))) < 0.05 && original.parent != nullptr) {
            const Vector3f lhs = position - tf(*original.parent).position;
            const Vector3f rhs = Vector3f::Cross(lhs, position2);
            const Vector3f rhs2 = Vector3f::Cross(lhs, position3);
            const float num4 = Vector3f::Dot(tf(original).up(), rhs);
            if (Vector3f::Dot(tf(original).up(), rhs2) < num4) highestMass = -1.0f;
            else num = -1.0f;
        } else if (num3 < num2) {
            highestMass = -1.0f;
        } else {
            num = -1.0f;
        }
    }
    if (!(num > highestMass)) return;
    bool flag = false;
    size_t num5 = excluded.size();
    while (num5-- > 0) {
        if (excluded[num5] == &p) {
            flag = true;
            break;
        }
    }
    if (!flag) {
        highestMass = num;
        highestPart = &p;
    }
}

// Part.cs:8421 GetMassiveChildPart (children in reverse order, the subtrees below a free pivot are not entered)
void PartJointRules::GetMassiveChildPart(Part& original, Part& parent, const std::vector<Part*>& children, Part*& highestPart, float& highestMass,
                                         Part*& lastPart, AttachNode*& nodeToParent, AttachNode*& nodeFromParent, bool& srfAttached,
                                         const Part* ignoredChild, const std::vector<Part*>& excluded, const BuildCtx& ctx) {
    if (children.empty()) return;
    size_t count = children.size();
    while (count-- > 0) {
        Part* part = children[count];
        if (original.isRobotic() && parent.fields.persistentId == original.fields.persistentId)
            nodeFromParent = AttachedPartNode(&original, part, srfAttached);
        if (part->fields.persistentId == original.fields.persistentId || part == ignoredChild) continue;
        MassivePartCheck(original, *part, highestPart, highestMass, excluded, ctx);
        if (HasFreePivot(*part)) {
            if (part->isRobotic()) {
                nodeFromParent = AttachedPartNode(part->parent, part, srfAttached);
                lastPart = part;
            }
        } else {
            GetMassiveChildPart(original, *part, part->children, highestPart, highestMass, lastPart, nodeToParent, nodeFromParent, srfAttached, nullptr,
                                excluded, ctx);
        }
    }
}

// Part.cs:8380 GetMassiveParentPart (up to the root, each ancestor's other children walked too; a free pivot ends the walk)
void PartJointRules::GetMassiveParentPart(Part& original, Part* child, Part*& highestPart, float& highestMass, Part*& lastPart, AttachNode*& nodeToParent,
                                          AttachNode*& nodeFromParent, bool& srfAttached, const std::vector<Part*>& excluded, const BuildCtx& ctx) {
    if (child == nullptr || child->parent == nullptr) return;
    MassivePartCheck(original, *child->parent, highestPart, highestMass, excluded, ctx);
    if (HasFreePivot(*child->parent)) {
        if (child->parent->isRobotic()) {
            nodeFromParent = AttachedPartNode(child, child->parent, srfAttached);
            lastPart = child->parent;
        }
        return;
    }
    GetMassiveParentPart(original, child->parent, highestPart, highestMass, lastPart, nodeToParent, nodeFromParent, srfAttached, excluded, ctx);
    Part* highestPart2 = highestPart;
    float highestMass2 = highestMass;
    Part* lastPart2 = nullptr;
    AttachNode* nodeToParent2 = nullptr;
    AttachNode* nodeFromParent2 = nullptr;
    bool srfAttached2 = false;
    GetMassiveChildPart(original, *child->parent, child->parent->children, highestPart2, highestMass2, lastPart2, nodeToParent2, nodeFromParent2,
                        srfAttached2, child, excluded, ctx);
    if ((highestPart2 != nullptr && highestPart == nullptr) ||
        (highestPart != nullptr && highestPart2->fields.persistentId != highestPart->fields.persistentId)) {
        highestPart = highestPart2;
        highestMass = highestMass2;
        lastPart = lastPart2;
        if (nodeToParent2 != nullptr) nodeToParent = nodeToParent2;
        if (nodeFromParent2 != nullptr) nodeFromParent = nodeFromParent2;
        srfAttached = srfAttached2;
    }
}

// Part.cs:8342 GetMassivePart (`self` is the C# `this`, whose cache fields are read and written; start == this in every call of the C#).
// The cache (:8349) is used only when the builder has a Time.fixedTime (ctx.fixedTime).
Part* PartJointRules::GetMassivePart(Part& self, Part& start, Part*& lastPart, AttachNode*& nodeToParent, AttachNode*& nodeFromParent, bool& srfAttached,
                                     const std::vector<Part*>& excluded, const BuildCtx& ctx) {
    float highestMass = -std::numeric_limits<float>::max();  // float.MinValue
    Part* highestPart = nullptr;
    lastPart = nullptr;
    nodeToParent = nodeFromParent = nullptr;
    srfAttached = false;
    if (ctx.fixedTime != nullptr && self.autoStrutCacheVessel == start.vessel && self.autoStrutCacheTick == *ctx.fixedTime)
        highestMass = self.autoStrutHighMass;
    if (start.isRobotic() && self.parent != nullptr) nodeToParent = AttachedPartNode(self.parent, &start, srfAttached);
    GetMassiveParentPart(start, &start, highestPart, highestMass, lastPart, nodeToParent, nodeFromParent, srfAttached, excluded, ctx);
    Part* highestPart2 = highestPart;
    float highestMass2 = highestMass;
    Part* lastPart2 = nullptr;
    AttachNode* nodeToParent2 = nullptr;
    AttachNode* nodeFromParent2 = nullptr;
    bool srfAttached2 = false;
    GetMassiveChildPart(start, start, start.children, highestPart2, highestMass2, lastPart2, nodeToParent2, nodeFromParent2, srfAttached2, nullptr,
                        excluded, ctx);
    if ((highestPart2 != nullptr && highestPart == nullptr) ||
        (highestPart != nullptr && highestPart2->fields.persistentId != highestPart->fields.persistentId)) {
        highestPart = highestPart2;
        highestMass = highestMass2;
        lastPart = lastPart2;
        nodeToParent = nodeToParent2;
        nodeFromParent = nodeFromParent2;
        srfAttached = srfAttached2;
    }
    if (ctx.fixedTime != nullptr) {
        self.autoStrutHighMass = highestMass;
        self.autoStrutCacheTick = *ctx.fixedTime;
        self.autoStrutCacheVessel = start.vessel;
    }
    return highestPart;
}

// Part.cs:8280 GetAutoStrutAnchor
Part* PartJointRules::GetAutoStrutAnchor(Vessel& vessel, Part& self, AttachNode*& nodeToParent, AttachNode*& nodeFromParent, bool& srfAttached,
                                         const BuildCtx& ctx) {
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
        case 5: {  // ForceHeaviest
            if (!self.fields.autoStrutExcludeParent) return GetMassivePart(self, self, lastPart, nodeToParent, nodeFromParent, srfAttached, {}, ctx);
            return GetMassivePart(self, self, lastPart, nodeToParent, nodeFromParent, srfAttached, {self.parent}, ctx);
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

namespace {
BuildCtx MakeCtx(const JointGlobals& globals, const std::function<const PartTransform&(const Part&)>& transformOf, const double* fixedTime) {
    BuildCtx ctx;
    ctx.globals = globals;
    ctx.tf = transformOf ? transformOf : TransformOf([](const Part& p) -> const PartTransform& { return p.partTransform; });
    ctx.fixedTime = fixedTime;
    return ctx;
}
}  // namespace

Part* GetAutoStrutAnchor(Vessel& vessel, Part& part, const std::function<const PartTransform&(const Part&)>& transformOf, const double* fixedTime) {
    const BuildCtx ctx = MakeCtx({}, transformOf, fixedTime);
    AttachNode* nodeToParent = nullptr;
    AttachNode* nodeFromParent = nullptr;
    bool srfAttached = false;
    return PartJointRules::GetAutoStrutAnchor(vessel, part, nodeToParent, nodeFromParent, srfAttached, ctx);
}

std::vector<PartJointSpec> BuildPartJoints(Vessel& vessel, const JointGlobals& globals,
                                           const std::function<const PartTransform&(const Part&)>& transformOf) {
    std::vector<PartJointSpec> out = BuildAttachJoints(vessel, globals, transformOf);
    std::vector<PartJointSpec> autostruts = BuildAutoStruts(vessel, globals, transformOf);
    std::vector<PartJointSpec> struts = BuildStrutJoints(vessel, globals, transformOf);
    out.insert(out.end(), autostruts.begin(), autostruts.end());
    out.insert(out.end(), struts.begin(), struts.end());
    return out;
}

std::vector<PartJointSpec> BuildAttachJoints(Vessel& vessel, const JointGlobals& globals,
                                             const std::function<const PartTransform&(const Part&)>& transformOf) {
    const BuildCtx ctx = MakeCtx(globals, transformOf, nullptr);
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
    return out;
}

std::vector<PartJointSpec> BuildAutoStruts(Vessel& vessel, const JointGlobals& globals,
                                           const std::function<const PartTransform&(const Part&)>& transformOf, const double* fixedTime) {
    const BuildCtx ctx = MakeCtx(globals, transformOf, fixedTime);
    std::vector<PartJointSpec> out;
    // ---- autostruts: Part.CycleAutoStrut -> SecureAutoStruts (Part.cs:8044-8091) for every part with autoStrutMode != Off
    // (the EVA-construction exclusion of :8066 does not apply outside construction mode)
    for (Part* p : vessel.parts) {
        if (p->fields.autoStrutMode == 0) continue;
        AttachNode* nodeToParent = nullptr;
        AttachNode* nodeFromParent = nullptr;
        bool srfAttached = false;
        Part* anchor = nullptr;
        try {
            anchor = PartJointRules::GetAutoStrutAnchor(vessel, *p, nodeToParent, nodeFromParent, srfAttached, ctx);
        } catch (const NullReferenceException&) {
            continue;  // MassivePartCheck's null highestPart on a cache hit: the coroutine of this part dies, no autostrut this cycle
        }
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
    return out;
}

std::vector<PartJointSpec> BuildStrutJoints(Vessel& vessel, const JointGlobals& globals,
                                            const std::function<const PartTransform&(const Part&)>& transformOf) {
    const BuildCtx ctx = MakeCtx(globals, transformOf, nullptr);
    std::vector<PartJointSpec> out;
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
