// WP-20b: vessel rigid bodies on the REDUCED rigid-body model (core/PHYSICS-SPIKE.md sections 3-5, adopted 2026-10-12).
//
// One Jolt body per rigid CLUSTER of parts instead of KSP's one PhysX Rigidbody per part plus PartJoint springs. What decides
// the clusters, the masses and the forces is KSP's own rules (C# citations relative to decompiled/KSP-Decompiled/Assembly-CSharp):
//
//   rigid bodies   Part.Start (Part.cs:1788-1834, 1868-1895): a part has a Rigidbody when its physicalSignificance is FULL
//                  (PhysicsSignificance <= 0, the vessel root, a child of a robotic part); a physicsless part rides on its
//                  RigidBodyPart (Part.cs:1184) and its mass is folded into that part's physicsMass (FlightIntegrator.cs:700).
//   joints         PartJoint.Create / create / SetupJoint (PartJoint.cs:96-313) for the attach joint of every FULL, attached part
//                  (Part.CreateAttachJoint, Part.cs:2197), the autostruts (Part.SecureAutoStruts / SecureAutoStrut, Part.cs:8061-8126)
//                  and the struts (CModuleStrut.OnTargetSet, CompoundParts/CModuleStrut.cs:26): host / target bodies, the node,
//                  the sub-joint count (3 on a ring for size >= 2 nodes without a docking port), stiffness and break limits.
//                  PartJointSpec is that data: the reduced model WELDS the bodies a joint connects (wobble is dropped by decision);
//                  WP-20c uses the specs for the structural load graph and breaking.
//   clusters       the bodies welded through attach joints form one cluster, except across an ARTICULATION POINT: a decoupler
//                  in the SplitAtDecouplers policy (PHYSICS-SPIKE.md "reduced-split"), a robotic part or anything the host marks
//                  flexible (`isArticulation`). Articulation points become Jolt fixed constraints between clusters (WP-52 / WP-20c
//                  give them motors / compliance later). A cut joint (decoupled, broken) separates the clusters for good.
//   mass           rb.mass = Max(partInfo.MinimumRBMass, physicsMass) (FlightIntegrator.cs:476-489, after the WP-14 port of
//                  UpdateMassStats), rb.centerOfMass = CoMOffset; cluster mass / centre of mass / inertia are the double sums of
//                  these over the cluster's parts in KSP's loop order (last part first, VesselPrecalculate.cs:404-415).
//   inertia        rb.inertiaTensor = inertiaTensor * Max(1, rb.mass) (Part.ValidateInertiaTensor, Part.cs:4263), inertiaTensor
//                  = the collider tensor per unit mass (Part.cs:1890). Colliders are PROXIES until the model reader supplies
//                  the .mu colliders (PHYSICS-SPIKE.md R6): the box of the part's first drag cube (the spike's choice).
//   forces         every force keeps its point of application: Part.AddForce / AddForceAtPosition / AddTorque / AddImpulse
//                  (Part.cs:8663-8688) and Rigidbody.AddForce* with a ForceMode go to the owning cluster as force + torque about
//                  its centre of mass; FlightIntegrator.Integrate (FlightIntegrator.cs:715-745) is IntegratePartBuffers().
//   state          ReadState() is VesselPrecalculate.CalculatePhysicsStats' per-part sums (VesselPrecalculate.cs:388-460) over the
//                  parts' rigid-body states, evaluated in the DOUBLE physics frame (the VesselPhysicsState seam of WP-19a).
//
// Frames (PHYSICS-SPIKE.md 5.1, WP-20a): positions are doubles in the physics frame of the PhysicsWorld (the main body's body-fixed
// or inertial frame, no per-tick shift). The WORLD position of a physics-frame point p is
//     world = frameOriginWorld + ReferenceFrame::TruePosition(p)        (TruePosition adds FloatingOrigin::FrameOffset())
// with frameOriginWorld the world position of the physics frame's origin (the main body's `position`; the host keeps it). The
// float Unity-style fields the ported code reads (Part.partTransform, Rigidbody.worldCenterOfMass, Part.AddForceAtPosition's
// position) are written / read in the TRANSFORM frame:  transform = (float)(world - floatOrigin). floatOrigin = 0 is KSP's own
// convention (part transforms, Vessel.CoM and CelestialBody.position share one frame); a host that keeps the world origin far away
// sets floatOrigin near the vessel (the spike's vessel-local frame) and must then hand the ported code consistent references.
//
// Units are KSP's: tonnes, metres, seconds, kN. Not here: the structural load solve and breaking (WP-20c), the per-part
// FlightIntegrator pipeline (WP-20d), contacts / landing / water / the landed anchor (WP-20e), frames and the physics bubble
// (WP-19b), wheels (WP-29), robotic motors (WP-52).
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/physics/physics_world.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {
struct AeroForces;          // ksp/aero/flight_integrator_aero.hpp (WP-21)
struct SurfaceForces;       // ksp/aero/lifting_surface.hpp (WP-21)
struct EngineThrustResult;  // ksp/modules/module_engines.hpp (WP-23)
struct RCSForce;            // ksp/modules/module_rcs.hpp (WP-23)
}  // namespace ksp

namespace ksp::physics {

// ===================================================================================== Part.Start rigid body rules

// Part.Start's flight-scene block for every part of `vessel` (Part.cs:1788-1834 and 1868-1895): physicalSignificance from
// PhysicsSignificance (Max(0, PhysicsSignificance); a child of a robotic part is promoted; the vessel root is always FULL) and
// `rb != null` (Part::hasRigidbody) for the FULL parts. Run once when the parts enter the flight scene (KSP creates the
// Rigidbody once per Part and keeps it through pack / unpack); VesselPhysics only READS hasRigidbody.
void StartPartRigidbodies(Vessel& vessel);

// FlightIntegrator.cs:483 `num2 = Mathf.Max(part.partInfo.MinimumRBMass, num)` with num = part.physicsMass (the WP-14
// Vessel::UpdateMassStats output): rb.mass of a part with a Rigidbody. 0 for a part without one.
float PartRigidbodyMass(const Part& part);

// Part.ValidateInertiaTensor (Part.cs:4263): rb.inertiaTensor = inertiaTensor * Mathf.Max(1f, rb.mass), applied only when every
// component of the per-unit-mass tensor exceeds 1e-6 (else PhysX's own tensor stays: here the per-unit tensor * rb.mass).
// `floor == false` drops the Max(1, m) floor (PHYSICS-ENGINE-REVIEW.md 10 lists it as droppable; kept by default).
Vector3f PartInertiaTensor(const Vector3f& perUnitMass, float rbMass, bool floor = true);

// Part.RigidBodyPart (Part.cs:1184): the part itself when it has a Rigidbody, else its parent's RigidBodyPart; nullptr at the root.
Part* RigidBodyPart(Part* part);

// ===================================================================================== collider proxies

// One collider of a part in PART space (the transform's local frame). Until the model reader exists: boxes.
struct ColliderProxy {
    Vector3f center;
    Vector3f halfExtents{0.25f, 0.25f, 0.25f};
    Quaternionf rotation = Quaternionf::identity();
};
// The spike's proxy (core/spikes/jolt/src/craft_model.cpp): the box of the part's first drag cube (DragCube center / size), half
// extents at least 0.05 m; a part without drag cubes gets a 0.5 m cube at its origin.
std::vector<ColliderProxy> DragCubeColliderProxy(const Part& part);
// Solid-box inertia tensor per unit mass of a set of proxies (the stand-in for PhysX's collider tensor at rb.mass 1, Part.cs:1890):
// one box: ((hy^2 + hz^2) / 3, (hx^2 + hz^2) / 3, (hx^2 + hy^2) / 3) about its centre, principal axes = the part axes.
// Several boxes: volume-weighted, about their common volume centre, diagonal kept (the off-diagonal part is dropped, as PhysX
// keeps only the principal moments in rb.inertiaTensor and the rotation in rb.inertiaTensorRotation; proxies are axis aligned).
Vector3f ProxyInertiaPerUnitMass(const std::vector<ColliderProxy>& colliders);

// ===================================================================================== PartJoint data

enum class JointKind : uint8_t { Attach, AutoStrut, Strut };

// What PartJoint.Create (PartJoint.cs:96) builds for one joint, as data. Positions / directions are in the CHILD part's space
// (PartJoint.create converts the node into child space, :170-172).
struct PartJointSpec {
    JointKind kind = JointKind::Attach;
    Part* child = nullptr;       // PartJoint.child  (the owner: Part.CreateAttachJoint's `this`, the autostrutted part, the strut's root body)
    Part* parent = nullptr;      // PartJoint.parent (the parent, the autostrut anchor, the strut's target body)
    Part* host = nullptr;        // PartJoint.host   (:145-161: the child when both are FULL)
    Part* target = nullptr;      // PartJoint.target
    Part* hostBody = nullptr;    // the Rigidbody the ConfigurableJoints sit on (host.gameObject; host is FULL)
    Part* targetBody = nullptr;  // the connected body (SetupJoint :244-263: target.Rigidbody, else its parent's / grandparent's)
    int mode = 0;                // AttachModes: 0 STACK, 1 SRF_ATTACH
    int nodeSize = 0;
    bool rigid = false;          // PartJoint.rigid (node rigidity, :103-133)
    bool forceRigid = false;     // Part.rigidAttachment of the child, the forceRigid of SetUnbreakable (Part.ResetJoints)
    int internalJoints = 1;      // ConfigurableJoints made (:176-205)
    std::array<Vector3f, 3> anchors{};  // jointPos of each sub-joint, child space
    Vector3f axis{0.0f, 1.0f, 0.0f};    // nodeOrt / nodeOrt2 after OrthoNormalize, child space
    Vector3f secAxis{1.0f, 0.0f, 0.0f};
    float stiffness = 0.0f;      // PartJoint.stiffness = rigidity * num3 * Stack/SrfAttachStiffNess (:290-298)
    float breakingForce = 0.0f;  // PartJoint.breakingForce / breakingTorque (per sub-joint, :302-303)
    float breakingTorque = 0.0f;
    float breakForce = 0.0f;     // ConfigurableJoint.breakForce / breakTorque in flight: SetUnbreakable(false, forceRigid) (:332-334)
    float breakTorque = 0.0f;    // for attach joints; +infinity for autostruts and struts (SetupJoint's value, never made breakable)
};

// FlightGlobals.StackAttachStiffNess / SrfAttachStiffNess are scene-serialized fields; both are 10 in the shipped flight scene
// [runtime, PROFILE.md section 6]. Used by PartJointSpec.stiffness.
struct JointGlobals {
    float stackAttachStiffness = 10.0f;
    float srfAttachStiffness = 10.0f;
};

// The joints of a vessel in KSP's creation order: the attach joint of every FULL attached part with a parent (vessel part order),
// then the autostruts (Part.SecureAutoStruts for every part with autoStrutMode != Off), then the struts (CModuleStrut with a target).
// Joints SetupJoint cannot connect (both ends on one Rigidbody, :255-260) are not created (as in KSP).
// Autostrut anchors: Root / Grandparent (and Force*) as Part.GetAutoStrutAnchor (Part.cs:8290-8338) with HasFreePivotBetween*
// (:8525-8615), the symmetry-counterpart loop of SecureAutoStruts (:8068-8086); Heaviest is the spike's rule (heaviest rb.mass,
// excluding the part itself and, with autoStrutExcludeParent, its parent): the GetMassivePart tree walk is WP-20c's.
// `transformOf` gives the Transform each part is measured in (default: Part::partTransform); only relative poses matter, so
// VesselPhysics passes the vessel-frame transforms (orgPos / orgRot), which stay precise wherever the vessel is.
std::vector<PartJointSpec> BuildPartJoints(Vessel& vessel, const JointGlobals& globals = {},
                                           const std::function<const PartTransform&(const Part&)>& transformOf = nullptr);

// ===================================================================================== force accumulator

// Unity's ForceMode (what Rigidbody.AddForce* is called with).
enum class ForceMode : uint8_t { Force, Acceleration, Impulse, VelocityChange };

// Where the per-part force producers (WP-21 aero, WP-23 engines / RCS, WP-24 reaction wheels, WP-25 docking, WP-16 decouplers,
// WP-28 drains ...) hand their forces. Every call buffers; Flush() pushes the sum into the solver once per tick. Positions are in
// the TRANSFORM frame (see the header comment: what Part.partTransform and Rigidbody.worldCenterOfMass are written in).
class ForceAccumulator {
public:
    virtual ~ForceAccumulator() = default;
    // Part.AddForce (Part.cs:8663): RigidBodyPart.force += vec; Integrate applies it with rb.AddForce, i.e. at that body's centre of mass
    virtual void AddForce(Part& part, const Vector3d& force) = 0;
    // Part.AddImpulse (Part.cs:8671): RigidBodyPart.force += vec / TimeWarp.fixedDeltaTime
    virtual void AddImpulse(Part& part, const Vector3d& impulse) = 0;
    // Part.AddForceAtPosition (Part.cs:8679): RigidBodyPart.forces.Add (applied by rb.AddForceAtPosition)
    virtual void AddForceAtPosition(Part& part, const Vector3d& force, const Vector3d& position) = 0;
    // Part.AddTorque (Part.cs:8684): RigidBodyPart.torque += vec (Vessel.ResetRBAnchor is WP-20e's)
    virtual void AddTorque(Part& part, const Vector3d& torque) = 0;
    // part.Rigidbody.AddForce(force, mode) / AddForceAtPosition(force, position, mode): Acceleration and VelocityChange scale by
    // that Rigidbody's mass (part.Rigidbody = the RigidBodyPart's rb), Impulse and VelocityChange are spread over the tick.
    virtual void AddRigidbodyForce(Part& part, const Vector3d& force, ForceMode mode) = 0;
    virtual void AddRigidbodyForceAtPosition(Part& part, const Vector3d& force, const Vector3d& position, ForceMode mode) = 0;
    // part.rb.angularDrag (FlightIntegrator.UpdateAerodynamics :2276 / :2365, PartBuoyancy): parts without a Rigidbody are ignored
    virtual void SetAngularDrag(Part& part, float angularDrag) = 0;
    // rb.AddForce(a, ForceMode.Acceleration) on every Rigidbody of the vessel (FlightIntegrator.Integrate :719, integrationAccel)
    virtual void AddUniformAcceleration(const Vector3d& acceleration) = 0;
};

// The force records of the ported modules, applied as the C# does (positions in the transform frame):
// FlightIntegrator.ApplyAeroDrag / ApplyAeroLift (FlightIntegrator.cs:2243-2258: part.Rigidbody.AddForceAtPosition with the
// drag mode) and rb.angularDrag; ModuleEngines.ThrustUpdate (part.AddForceAtPosition per thrust transform); ModuleRCS
// (part.AddForceAtPosition); ModuleLiftingSurface / ModuleControlSurface (part.AddForceAtPosition, lift then drag).
void ApplyAeroForces(ForceAccumulator& acc, Part& part, const AeroForces& f);
void ApplyEngineForces(ForceAccumulator& acc, Part& part, const EngineThrustResult& r);
void ApplyRcsForces(ForceAccumulator& acc, Part& part, const std::vector<RCSForce>& forces);
void ApplySurfaceForces(ForceAccumulator& acc, Part& part, const SurfaceForces& f);

// ===================================================================================== the vessel binding

// How rb.angularDrag of the parts acts on a cluster.
enum class AngularDragModel : uint8_t {
    // PhysX damps every part body's own angular velocity (w_i *= max(0, 1 - dt d_i)); in a rigid assembly that removes
    // sum_i (1 - f_i) I_i w of angular momentum per step (I_i the part's own tensor, the orbital m r x v of a part is not damped).
    // VesselPhysics applies exactly that as a torque (default: KSP's behaviour).
    PerPart,
    // The spike's model: Jolt's damping of the whole cluster with the rb.mass-weighted mean drag (damps the whole I_c w; much
    // stronger for long vessels). Kept to reproduce the spike's numbers.
    ClusterMassAverage,
};

enum class ClusterPolicy : uint8_t {
    Whole,              // one body per connected vessel; decouplers are welded until they fire (the spike's "reduced-whole", the default)
    SplitAtDecouplers,  // one body per piece between decouplers, pieces welded by fixed constraints (the spike's "reduced-split")
};

struct VesselPhysicsOptions {
    ClusterPolicy policy = ClusterPolicy::Whole;
    // extra articulation points (robotics, "flexible" joints): the attach joint of `child` is a fixed constraint between clusters
    // instead of a weld. Unset: none (robotic parts are welded until WP-52 gives them motors).
    std::function<bool(const PartJointSpec&)> isArticulation;
    // colliders of a part (part space); unset = DragCubeColliderProxy
    std::function<std::vector<ColliderProxy>(const Part&)> colliders;
    JointGlobals jointGlobals;
    bool inertiaFloor = true;     // Part.ValidateInertiaTensor's Max(1, rb.mass)
    float friction = 0.6f;        // Unity's default PhysicMaterial (dynamic / static friction 0.6, bounciness 0)
    float restitution = 0.0f;
    float defaultAngularDrag = 0.05f;  // Rigidbody.angularDrag of a new Rigidbody (Unity default) until FlightIntegrator sets it
    AngularDragModel angularDragModel = AngularDragModel::PerPart;
};

// The pose and motion of one part as the reduced model sees it (physics frame).
struct PartPhysicsState {
    Vector3d position;          // part transform origin
    QuaternionD rotation;       // part transform rotation
    Vector3d centerOfMass;      // rb.worldCenterOfMass (the part's own; for a physicsless part its own CoMOffset point too)
    Vector3d velocity;          // velocity of `centerOfMass` (rb.velocity for a part with a Rigidbody)
    Vector3d angularVelocity;   // the cluster's
    int cluster = -1;
};

// The vessel pose a VesselPhysics is built at: the vessel transform (= root part transform) in the physics frame.
struct VesselPose {
    Vector3d position;
    QuaternionD rotation = QuaternionD::identity();
};

class VesselPhysics final : public ForceAccumulator {
public:
    // Builds the clusters of `vessel` at `pose` (parts placed by orgPos / orgRot relative to the root: the pristine coordinates of
    // Vessel.SetPosition / SetRotation, Vessel.cs:3683-3740). Bodies start at rest. `group` is the same-vessel collision group
    // (0: the vessel's persistentId, or a fresh id when that is 0). The PhysicsWorld must outlive the VesselPhysics.
    VesselPhysics(PhysicsWorld& world, Vessel& vessel, const VesselPose& pose, const VesselPhysicsOptions& options = {},
                  uint32_t group = 0);
    // Builds from given part states (one per vessel part, physics frame): the general path of re-clustering, splits and merges.
    VesselPhysics(PhysicsWorld& world, Vessel& vessel, const std::vector<PartPhysicsState>& partStates,
                  const VesselPhysicsOptions& options = {}, uint32_t group = 0);
    // Removes the bodies and constraints from the world. Touches neither the Vessel nor its Parts (they may be gone already).
    ~VesselPhysics() override;
    VesselPhysics(const VesselPhysics&) = delete;
    VesselPhysics& operator=(const VesselPhysics&) = delete;

    Vessel& GetVessel() const { return *vessel_; }
    PhysicsWorld& World() const { return *world_; }
    const VesselPhysicsOptions& Options() const { return options_; }

    // ---- frames (see the header comment) --------------------------------------------------------------------------------
    Vector3d frameOriginWorld;  // world position of the physics frame's origin (default 0)
    Vector3d floatOrigin;       // origin of the transform frame (default 0)
    Vector3d WorldFromPhysics(const Vector3d& p) const;
    Vector3d PhysicsFromWorld(const Vector3d& w) const;
    Vector3d PhysicsFromTransform(const Vector3d& t) const { return PhysicsFromWorld(t + floatOrigin); }
    Vector3f TransformFromPhysics(const Vector3d& p) const { return (WorldFromPhysics(p) - floatOrigin).ToVector3f(); }

    // ---- structure ------------------------------------------------------------------------------------------------------
    const std::vector<PartJointSpec>& Joints() const { return joints_; }
    // joints still welded / constrained (not cut), in Joints() order
    bool JointCut(size_t joint) const { return cut_[joint]; }
    size_t ClusterCount() const;
    size_t ConstraintCount() const { return constraints_.size(); }  // inter-cluster fixed constraints
    int ClusterOf(const Part& part) const;
    BodyId ClusterBody(size_t cluster) const;
    const std::vector<int>& ClusterParts(size_t cluster) const;  // vessel part indices (vessel order at build)
    // the cluster's mass properties as given to the solver: mass (sum of rb.mass), centre of mass (physics frame, now) and the
    // inertia tensor about it (physics axes, now)
    double ClusterMass(size_t cluster) const;
    Vector3d ClusterCenterOfMass(size_t cluster) const;
    void ClusterInertia(size_t cluster, double out[3][3]) const;
    Vector3d ClusterVelocity(size_t cluster) const;
    Vector3d ClusterAngularVelocity(size_t cluster) const;
    // rb.mass and rb.inertiaTensor (principal moments, part axes) of a part as the cluster sums use them; 0 without a Rigidbody
    float PartMass(const Part& part) const;
    Vector3f PartInertia(const Part& part) const;
    int PartIndex(const Part& part) const;  // index in vessel.parts at build time (-1 when not a part of this vessel)

    // ---- state ------------------------------------------------------------------------------------------------------------
    PartPhysicsState PartState(const Part& part) const;
    std::vector<PartPhysicsState> PartStates() const;  // vessel part order
    // VesselPrecalculate.CalculatePhysicsStats' sums (VesselPrecalculate.cs:388-460) in the double frame: totalMass and CoMD
    // weighted by part.physicsMass over the parts with a Rigidbody, last part first; velocity = the rb velocity at each part's
    // centre of mass, angular velocity in the reference transform's frame; MOI from rb.inertiaTensor, the part rotation and the
    // parallel-axis terms about CoMD in float exactly as the C# (via Vessel::MOIFromRigidbodies). CoMD is a WORLD position.
    VesselPhysicsState ReadState() const;
    // Part.rb as the float Rigidbody KSP would report (the oracle of the partRigidbody seam): transform-frame floats.
    PartRigidbodyState RigidbodyState(const Part& part) const;
    // rootPart.rb.GetPointVelocity(point) / rb.angularVelocity (VesselPrecalculate.cs:491-497); `point` in the transform frame
    bool RootRigidbody(const Vector3f& point, Vector3f& pointVelocity, Vector3f& angularVelocity) const;
    // Writes every part's partTransform (position in the transform frame, rotation) and the vessel transform (= the root part's):
    // what Unity's transform sync did after each PhysX step.
    void SyncPartTransforms();

    // ---- velocities -------------------------------------------------------------------------------------------------------
    // Part.ResumeVelocity (Part.cs:3827) for every part of the vessel: v = orbit.GetVel() - (rotating frame ? body.getRFrmVel(p) : 0)
    // - Krakensbane.GetFrameVelocity(), zero when LandedOrSplashed; the reduced model evaluates getRFrmVel at each cluster's
    // centre of mass (KSP: at each part position; the -omega x r field it gives is not a rigid motion of zero angular velocity).
    void ResumeVelocity();
    void ResumePartVelocity(Part& part);  // the same for the part's cluster
    // every body's velocity += dv: Vessel.OffsetVelocity (Vessel.cs:3742), Vessel.ChangeWorldVelocity (:3197), Krakensbane
    void ChangeWorldVelocity(const Vector3d& dv);
    void SetLinearVelocity(const Vector3d& v);  // every cluster (centre of mass), angular velocities unchanged

    // ---- forces (ForceAccumulator) ----------------------------------------------------------------------------------------
    void AddForce(Part& part, const Vector3d& force) override;
    void AddImpulse(Part& part, const Vector3d& impulse) override;
    void AddForceAtPosition(Part& part, const Vector3d& force, const Vector3d& position) override;
    void AddTorque(Part& part, const Vector3d& torque) override;
    void AddRigidbodyForce(Part& part, const Vector3d& force, ForceMode mode) override;
    void AddRigidbodyForceAtPosition(Part& part, const Vector3d& force, const Vector3d& position, ForceMode mode) override;
    void SetAngularDrag(Part& part, float angularDrag) override;
    void AddUniformAcceleration(const Vector3d& acceleration) override;
    // physics-frame variants (no transform frame involved)
    void AddForceAtPhysicsPoint(Part& part, const Vector3d& force, const Vector3d& point);
    float AngularDrag(const Part& part) const;
    // The force half of FlightIntegrator.Integrate (FlightIntegrator.cs:715-745), in its order (depth first from the root over
    // attached children): for each part with a Rigidbody integrationAccel (ForceMode.Acceleration), part.force, part.torque and
    // part.forces (last first); then every part's buffers are cleared. `integrationAccel` is vessel.precalc.integrationAccel.
    void IntegratePartBuffers(const Vector3d& integrationAccel);
    // External force and moment buffered for each part this tick (force, moment about the part's centre of mass), for WP-20c's
    // structural solve; cleared by Flush.
    const std::vector<Vector3d>& PartExternalForces() const { return partForce_; }
    const std::vector<Vector3d>& PartExternalMoments() const { return partMoment_; }

    // ---- per tick ---------------------------------------------------------------------------------------------------------
    // FlightIntegrator.UpdateMassStats changed rb.mass (resource drain): new cluster mass, centre of mass (the body origin stays,
    // the centre of mass moves inside the body and the velocity follows omega x shift) and inertia. Returns the clusters changed.
    size_t UpdateMassProperties();
    // Pushes the buffered forces and torques (about each cluster's centre of mass), the uniform acceleration and the angular-drag
    // torques into the solver, then clears the buffers. dt = the step about to be taken (Impulse / VelocityChange, angular drag).
    void Flush(float dt);
    // After the world step: caches the cluster states, syncs the part transforms.
    void AfterStep();

    // ---- structural seams (WP-20c / WP-16 / WP-25 call them; WP-20b provides the mechanism) ------------------------------
    // Cut the joints whose child is one of `children` (Part.decouple, a broken PartJoint, a fired decoupler): attach joints of
    // those parts, and the struts / autostruts that no longer connect parts of one piece of the tree. Clusters are rebuilt from the
    // current part states; each new cluster takes the linear and angular momentum of its parts (momentum is conserved).
    // Returns the cluster count afterwards. The parts stay in this Vessel (the vessel split is SplitOff).
    size_t Cut(const std::vector<Part*>& children);
    // The current part states of the parts of `other` (a Vessel created by Part.decouple / Undock from parts of this one) leave this
    // binding: a new VesselPhysics for `other` is built from them (momentum-conserving per cluster), this one is rebuilt over the
    // parts it still has. The caller (VesselPhysicsHost::OnVesselSplit) owns the result.
    std::unique_ptr<VesselPhysics> SplitOff(Vessel& other);
    // Part.Couple / docking: `merged` now holds the parts of a and b (the WP-16 / WP-25 tree operation is done); a new VesselPhysics
    // over `merged` from the part states of both, joints rebuilt; momentum of both conserved (an inelastic weld).
    static std::unique_ptr<VesselPhysics> Merge(VesselPhysics& a, VesselPhysics& b, Vessel& merged);

private:
    struct PartRec;
    struct ClusterRec;
    void Init(const std::vector<PartPhysicsState>& states, uint32_t group);
    void IndexParts();
    void BuildClusters(const std::vector<PartPhysicsState>& states);
    void DestroyBodies();
    void CacheClusterStates();
    void RefreshPartPoses();
    void AccumulateAt(int partIndex, const Vector3d& force, const Vector3d& pointPhysics);
    int OwnerIndex(const Part& part) const;  // index of the part's RigidBodyPart, -1 without one
    Vector3d PartPositionNow(int i) const;
    QuaternionD PartRotationNow(int i) const;
    Vector3d PartComNow(int i) const;

    PhysicsWorld* world_;
    Vessel* vessel_;
    VesselPhysicsOptions options_;
    uint32_t group_ = 0;
    bool sameVesselCollisionAll_ = false;
    std::vector<std::unique_ptr<PartRec>> parts_;  // vessel part order at build
    std::unordered_map<const Part*, int> index_;
    std::vector<PartJointSpec> joints_;
    std::vector<bool> cut_;
    std::vector<ClusterRec> clusters_;
    std::vector<ConstraintId> constraints_;
    std::vector<Vector3d> partForce_, partMoment_;
    Vector3d uniformAccel_;
};

// ===================================================================================== the host: Vessel::env wiring

// One VesselPhysics per loaded, unpacked vessel in one PhysicsWorld, wired into the WP-19a / WP-14 seams of Vessel::env:
//   unpackPart          (Vessel.GoOffRails -> Part.Unpack) builds the vessel's bodies on the first part of a vessel
//   resumePartVelocity  (Part.ResumeVelocity) sets the part's cluster velocity
//   packPart            (Vessel.GoOnRails -> Part.Pack) destroys the vessel's bodies (KSP keeps kinematic bodies; nothing
//                       needs them on rails, PHYSICS-SPIKE.md 5.4)
//   readPhysicsState    VesselPhysics::ReadState (CalculatePhysicsStats in the double frame)
//   partRigidbody       VesselPhysics::RigidbodyState (the float oracle); FromPart for vessels without bodies
//   rootRigidbody       VesselPhysics::RootRigidbody
//   offsetVelocity      VesselPhysics::ChangeWorldVelocity (Vessel.OffsetVelocity)
// Install() saves the hooks it replaces and Uninstall() (or the destructor) restores them.
class VesselPhysicsHost {
public:
    explicit VesselPhysicsHost(PhysicsWorld& world, const VesselPhysicsOptions& options = {});
    ~VesselPhysicsHost();
    VesselPhysicsHost(const VesselPhysicsHost&) = delete;
    VesselPhysicsHost& operator=(const VesselPhysicsHost&) = delete;

    PhysicsWorld& World() const { return *world_; }
    VesselPhysicsOptions options;
    // world position of the physics frame's origin for a vessel (WP-19b: the bubble's dominant body); unset = vessel.mainBody()->position
    std::function<Vector3d(const Vessel&)> frameOriginWorld;
    Vector3d floatOrigin;  // the transform frame origin given to every VesselPhysics (default 0, KSP's convention)
    // the pose a vessel is built at on unpack; unset = the rails state: vessel transform = CoMD - rotation * localCoM
    // (OrbitDriver.updateFromParameters, OrbitDriver.cs:283-284; CoMD is the packed CalculatePhysicsStats', VesselPrecalculate.cs:469-480)
    std::function<VesselPose(Vessel&)> unpackPose;

    void Install();
    void Uninstall();
    bool Installed() const { return installed_; }

    VesselPhysics* Find(const Vessel& vessel) const;
    VesselPhysics& Create(Vessel& vessel, const VesselPose& pose);  // replaces an existing one
    VesselPhysics& Adopt(std::unique_ptr<VesselPhysics> physics);  // a binding built elsewhere (SplitOff / Merge)
    void Destroy(const Vessel& vessel);
    void DestroyAll();
    size_t Count() const { return vessels_.size(); }
    // vessel split / merge (the seams of WP-16's Part.decouple / Undock and WP-25's Couple): see VesselPhysics::SplitOff / Merge
    VesselPhysics* OnVesselSplit(Vessel& original, Vessel& split);
    VesselPhysics* OnVesselMerge(Vessel& a, Vessel& b, Vessel& merged);

    // ---- per tick, in creation order (deterministic) ----------------------------------------------------------------------
    void RefreshFrames();     // frameOriginWorld / floatOrigin of every binding
    void IntegrateAll();      // IntegratePartBuffers(vessel.precalc.integrationAccel) for every binding
    void FlushAll(float dt);
    void AfterStepAll();
    // FlushAll, PhysicsWorld::Step(dt), AfterStepAll (the tick's physics half for hosts and tests)
    void Step(float dt);

private:
    PhysicsWorld* world_;
    std::vector<std::unique_ptr<VesselPhysics>> vessels_;  // creation order
    bool installed_ = false;
    VesselEnvironment saved_;
    VesselPose DefaultUnpackPose(Vessel& v) const;
    void ApplyFrame(VesselPhysics& vp) const;
};

}  // namespace ksp::physics
