// Jolt spike: the physical description of a real craft, built from kspcore's live Parts (WP-13/14/18) with KSP's rules:
//   rigid bodies   Part.Start (Assembly-CSharp/Part.cs:1868-1895): a body per FULL part; physicsless parts fold into the first
//                  ancestor with a body (FlightIntegrator.GetPhysicslessChildMass, FlightIntegrator.cs:700-713)
//   mass           FlightIntegrator.UpdateMassStats (FlightIntegrator.cs:452-493) through kspcore's Vessel::UpdateMassStats:
//                  physicsMass, rb.mass = max(MinimumRBMass, physicsMass), rb.centerOfMass = CoMOffset
//   inertia        Part.ValidateInertiaTensor (Part.cs:4263-4273): collider tensor per unit mass * max(1, rb.mass)
//   attach joints  PartJoint.Create / create / SetupJoint (PartJoint.cs:96-313): smaller node, 3 sub-joints on a ring of radius
//                  size-1 for size >= 2 nodes, stiffness 1500 * (size+1) * {2 stack, 0.8 srf} * 10 [runtime], rigid flag,
//                  break force min(host, target).breakingForce * (size+1)*factor / joints * JointBreakForceFactor (PartJoint.cs:307, :340)
//   autostruts     Part.SecureAutoStrut (Part.cs:8098-8126): one size-1 SRF_ATTACH joint to the anchor (Grandparent / Root / Heaviest)
// The collider of a part is a proxy: the box of its first drag cube (DragCube center / size), a stand-in for the .mu colliders
// the model reader (UE import) will provide.
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Math/DVec3.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "ksp/parts/part.hpp"
#include "ksp/vessel/vessel.hpp"

namespace spike {

class KspContext;

struct JointSpec {
    int child = -1, parent = -1;   // PartJoint.child / parent (part indices)
    int host = -1, target = -1;    // rigidbody owners (PartJoint.host / target after the physicsless walk)
    int mode = 0;                  // AttachModes: 0 STACK, 1 SRF_ATTACH
    int nodeSize = 0;
    bool rigid = false;
    bool autostrut = false;
    int internalJoints = 1;
    float stiffness = 0.0f;        // angular drive positionSpring per sub-joint (kN m / rad)
    float breakForceSub = 0.0f;    // per sub-joint, PhysX breakForce after SetUnbreakable(false) (kN)
    float breakTorqueSub = 0.0f;   // kN m
    float ringRadius = 0.0f;       // size - 1 for 3-joint attachments
    std::array<JPH::DVec3, 3> anchors{};  // sub-joint anchors, craft frame
    JPH::Vec3 axis = JPH::Vec3::sAxisY(), secAxis = JPH::Vec3::sAxisX();  // craft frame, orthonormal
};

struct PartSpec {
    ksp::Part* part = nullptr;
    std::string name;
    int parent = -1;
    std::vector<int> children;
    bool physical = true;       // physicalSignificance == FULL
    int rbOwner = -1;           // the part whose rigidbody carries this part
    bool isDecoupler = false;
    bool isEngine = false;
    float maxThrust = 0.0f;     // ModuleEngines.maxThrust (kN), 0 for non-engines
    JPH::DVec3 pos;             // partTransform.position, craft frame (Unity axes, root at the origin)
    JPH::Quat rot = JPH::Quat::sIdentity();
    JPH::Vec3 comLocal = JPH::Vec3::sZero();     // CoMOffset (part space)
    JPH::Vec3 boxCenter = JPH::Vec3::sZero();    // collider proxy (part space)
    JPH::Vec3 boxHalf = JPH::Vec3::sReplicate(0.25f);
    float ownMass = 0.0f;       // mass + resourceMass of this part alone (t)
    double physicsMass = 0.0;   // FlightIntegrator: incl. physicsless children
    float rbMass = 0.0f;        // max(MinimumRBMass, physicsMass) for parts with a body
    JPH::Vec3 inertiaPerMass = JPH::Vec3::sReplicate(1.0f);  // collider tensor at rb.mass 1 (Part.cs:1889)
    int autoStrutMode = 0;      // Part.AutoStrutMode
    int attachJoint = -1;       // index into CraftModel::joints of the part's attach joint
};

struct CraftModel {
    std::string name;
    std::unique_ptr<ksp::Vessel> vessel;
    std::vector<PartSpec> parts;
    std::vector<JointSpec> joints;  // attach joints first, then autostruts
    int attachJointCount = 0;
    int autostrutCount = 0;
    int root = 0;
    double totalMass = 0.0;
    int physicalCount = 0;
    int SubJointCount() const;
};

// Load a .craft through kspcore (WP-18 LoadShip + WP-14 AssembleVesselFromCraft) and derive the physical description.
CraftModel BuildCraftModel(KspContext& ctx, const std::string& craftPath, const std::string& name);

// Unity <-> Jolt value conversions (no handedness change: the spike simulates the scene as KSP's numbers describe it)
inline JPH::Vec3 ToJ(const ksp::Vector3f& v) { return JPH::Vec3(v.x, v.y, v.z); }
inline JPH::Quat ToJ(const ksp::Quaternionf& q) { return JPH::Quat(q.x, q.y, q.z, q.w).Normalized(); }
inline ksp::Vector3f ToK(JPH::Vec3Arg v) { return ksp::Vector3f(v.GetX(), v.GetY(), v.GetZ()); }
inline ksp::Quaternionf ToK(JPH::QuatArg q) { return ksp::Quaternionf(q.GetX(), q.GetY(), q.GetZ(), q.GetW()); }

}  // namespace spike
