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
// WP-20c adds the structural layer (ksp/physics/structure_graph.hpp): every tick AfterStep turns the clusters' motion and the parts'
// forces into the load of every PartJoint sub-joint (tree solve, or the indeterminate graph solve where struts / autostruts / several
// ground supports share the load), checks it against KSP's capacity (the PhysX break limits PartJoint.SetUnbreakable gives each
// ConfigurableJoint, PartJoint.cs:340-341) and reports the sub-joints over capacity; the host turns a break into PartJoint.OnJointBreak
// (PartJoint.cs:397) -> Part.OnPartJointBreak -> Part.decouple, and every structural game event (decouple, undock, couple / dock, part
// death, vessel modified) into one deferred Rebind of the bindings concerned (momentum conserved per new cluster). Autostruts follow
// Part.CycleAutoStrut: released at the event, secured after the next physics step (the SecureAutoStruts coroutine's WaitForFixedUpdate),
// Heaviest anchors from the GetMassivePart walk (Part.cs:8342).
//
// Units are KSP's: tonnes, metres, seconds, kN. Not here: the per-part FlightIntegrator pipeline (WP-20d), contacts / landing / water /
// the landed anchor (WP-20e), frames and the physics bubble (WP-19b), wheels (WP-29), robotic motors (WP-52).
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <cstddef>
#include <vector>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/physics/physics_world.hpp"
#include "ksp/physics/structure_graph.hpp"
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
// Autostrut anchors: Part.GetAutoStrutAnchor (Part.cs:8280): Root / Grandparent (and Force*) with HasFreePivotBetween* (:8525-8615),
// Heaviest / ForceHeaviest through GetMassivePart (:8342: the walk up the parents and through every subtree, stopping at free pivots,
// MassivePartCheck's rb.mass weights and tie rules, autoStrutExcludeParent), and the symmetry-counterpart loop of SecureAutoStruts
// (:8068-8086). `transformOf` gives the Transform each part is measured in (default: Part::partTransform); only relative poses matter,
// so VesselPhysics passes the vessel-frame transforms (orgPos / orgRot), which stay precise wherever the vessel is.
std::vector<PartJointSpec> BuildPartJoints(Vessel& vessel, const JointGlobals& globals = {},
                                           const std::function<const PartTransform&(const Part&)>& transformOf = nullptr);
// The three groups separately (BuildPartJoints = attach + autostruts + struts). BuildAutoStruts is one round of SecureAutoStruts over the
// vessel: `fixedTime` is Time.fixedTime, the key of GetMassivePart's per-part cache (Part.cs:8349-8376: a part walking twice in one fixed
// step starts from its last result); null: no cache (read or written).
std::vector<PartJointSpec> BuildAttachJoints(Vessel& vessel, const JointGlobals& globals = {},
                                             const std::function<const PartTransform&(const Part&)>& transformOf = nullptr);
std::vector<PartJointSpec> BuildAutoStruts(Vessel& vessel, const JointGlobals& globals = {},
                                           const std::function<const PartTransform&(const Part&)>& transformOf = nullptr,
                                           const double* fixedTime = nullptr);
std::vector<PartJointSpec> BuildStrutJoints(Vessel& vessel, const JointGlobals& globals = {},
                                            const std::function<const PartTransform&(const Part&)>& transformOf = nullptr);
// Part.GetAutoStrutAnchor (Part.cs:8280) of one part: the part its autostrut goes to (nullptr: none). Throws NullReferenceException
// where the C# does (MassivePartCheck after a cache hit, Part.cs:8462).
Part* GetAutoStrutAnchor(Vessel& vessel, Part& part, const std::function<const PartTransform&(const Part&)>& transformOf = nullptr,
                         const double* fixedTime = nullptr);

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

// WP-20c: which structural solve AfterStep runs (structure_graph.hpp).
enum class LoadSolve : uint8_t {
    None,   // no loads, no breaking
    Tree,   // the statically determinate solve over the attach-joint tree (struts / autostruts ignored: conservative); supports share the
            // residual equally (the spike's rule)
    Graph,  // the indeterminate solve over every live sub-joint and ground support
    Auto,   // Graph when a live strut / autostrut joins two rigid bodies or two or more parts touch static geometry, else Tree (default)
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
    // ---- WP-20c: the structural layer ------------------------------------------------------------------------------------
    LoadSolve loadSolve = LoadSolve::Auto;
    bool breakJoints = true;          // report the sub-joints over capacity (PhysX's joint break); false: loads only
    bool unbreakableJoints = false;   // CheatOptions.UnbreakableJoints: Part.ResetJoints makes every attach joint unbreakable (Part.cs:3815)
    // kN/m: the sub-joints' linear drive. PhysX's is PhysicsGlobals.JointForce (rigid in practice); a finite quasi-rigid value keeps the
    // elastic model well conditioned against the angular drives (PartJoint.stiffness, 2.4e4..1.4e5 kN m/rad). The spike's value.
    double kLinear = 1.0e8;
    double kSupportAngular = 1.0e8;   // kN m/rad: a ground support's rotational stiffness (kLinear x 1 m^2: a contact patch carries moments)
    // Part.CycleAutoStrut releases the autostruts at once and SecureAutoStruts re-creates them after `yield return new WaitForFixedUpdate()`
    // (Part.cs:8043-8067): the steps a new or modified binding runs without autostruts. 0: secured at once (no GetMassivePart cache).
    int autoStrutDelaySteps = 1;
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

namespace detail {
// Part* -> index: open addressing over a power-of-two table (the per-part calls of a tick look parts up several times each)
class PartIndexMap {
public:
    void Clear() {
        slots_.clear();
        size_ = 0;
    }
    void Reserve(size_t n) {
        size_t cap = 16;
        while (cap < 2 * n) cap <<= 1;
        slots_.assign(cap, Slot{});
        mask_ = cap - 1;
        size_ = 0;
    }
    void Insert(const Part* p, int index) {
        if (slots_.empty() || 2 * (size_ + 1) > slots_.size()) Grow();
        size_t h = Hash(p) & mask_;
        while (slots_[h].key != nullptr && slots_[h].key != p) h = (h + 1) & mask_;
        if (slots_[h].key == nullptr) ++size_;
        slots_[h] = Slot{p, index};
    }
    int Find(const Part* p) const {
        if (slots_.empty() || p == nullptr) return -1;
        size_t h = Hash(p) & mask_;
        while (slots_[h].key != nullptr) {
            if (slots_[h].key == p) return slots_[h].index;
            h = (h + 1) & mask_;
        }
        return -1;
    }

private:
    struct Slot {
        const Part* key = nullptr;
        int index = -1;
    };
    static size_t Hash(const Part* p) {
        const uint64_t x = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(p)) >> 4;
        return static_cast<size_t>((x * 0x9E3779B97F4A7C15ull) >> 20);
    }
    void Grow() {
        std::vector<Slot> old;
        old.swap(slots_);
        Reserve(old.empty() ? 8 : old.size());
        for (const Slot& s : old)
            if (s.key != nullptr) Insert(s.key, s.index);
    }
    std::vector<Slot> slots_;
    size_t mask_ = 0, size_ = 0;
};
}  // namespace detail

class VesselPhysics final : public ForceAccumulator {
public:
    // Builds the clusters of `vessel` at `pose` (parts placed by orgPos / orgRot relative to the root: the pristine coordinates of
    // Vessel.SetPosition / SetRotation, Vessel.cs:3683-3740). Bodies start at rest. `group` is the same-vessel collision group
    // (0: the vessel's persistentId, or a fresh id when that is 0). The PhysicsWorld must outlive the VesselPhysics. Both constructors
    // first run Vessel::UpdateMassStats (FlightIntegrator.UpdateMassStats runs every FixedUpdate in KSP, packed or not): rb.mass
    // follows from physicsMass.
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
    Vessel* VesselPtr() const { return vessel_; }  // nullptr after DetachVessel
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
    size_t PartCount() const { return parts_.size(); }
    bool PartHasRigidbody(size_t index) const;  // part.rb != null as the binding was built

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
    // Runs UpdateMassProperties first (FlightIntegrator.FixedUpdate runs UpdateMassStats before Integrate).
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
    // The vessel's part list or a part's physical significance changed (Part.PromoteToPhysicalPart / DemoteToPhysicslessPart, parts
    // added or removed by the host): re-read the parts and their joints and rebuild the clusters from the current part states
    // (momentum-conserving per new cluster; parts new to the binding start at their vessel-frame pose relative to the root and at rest).
    void Rebuild();
    // Part.Couple / docking: `merged` now holds the parts of a and b (the WP-16 / WP-25 tree operation is done); a new VesselPhysics
    // over `merged` from the part states of both, joints rebuilt; momentum of both conserved (an inelastic weld).
    static std::unique_ptr<VesselPhysics> Merge(VesselPhysics& a, VesselPhysics& b, Vessel& merged);
    // WP-20c, the general form of SplitOff / Merge / Rebuild: the parts of the `sources` bindings now belong to the `targets` vessels
    // (any mix of split, merge, part death, demotion). One new binding per target vessel that holds at least one part the sources know,
    // built from the current states of its parts (each new cluster takes the linear and angular momentum of its parts); parts no source
    // knows are placed at their vessel-frame pose relative to a known part of their vessel, at that part's rigid motion. Parts of the
    // sources that are in no target (destroyed) leave with their momentum. The sources are left as they are (the caller destroys them);
    // their Part pointers are only used as keys (a part may be gone).
    static std::vector<std::unique_ptr<VesselPhysics>> Rebind(PhysicsWorld& world, const std::vector<VesselPhysics*>& sources,
                                                              const std::vector<Vessel*>& targets, const VesselPhysicsOptions& options);
    // The Vessel is about to go (Part.Couple destroys the source vessel, Part.cs:3201): the binding never touches it again (it is then
    // only a Rebind source and must be destroyed).
    void DetachVessel() { vessel_ = nullptr; }

    // ---- WP-20c: structural loads and breaking ------------------------------------------------------------------------------
    struct JointLoad {
        bool valid = false;        // the joint is live and was part of the last solve
        Vector3d force, moment;    // resultant on the joint's host side (physics frame; moment about the anchor centroid)
        float maxForce = 0.0f;     // largest sub-joint linear force (kN) and angular-drive torque (kN m) magnitudes
        float maxTorque = 0.0f;
        float ratio = 0.0f;        // max over the live sub-joints of max(|f| / breakForce, |t| / breakTorque) (0 for unbreakable joints)
        int critical = -1;         // the sub-joint of `ratio`
    };
    // a sub-joint over its PhysX limit: |f| > breakForce or |t| > breakTorque (PartJoint.cs:340-341)
    struct JointBreak {
        size_t joint = 0;
        uint8_t mask = 0;          // the sub-joints that broke (bit k = sub-joint k)
        float force = 0.0f, torque = 0.0f;  // the worst of them
    };
    // per joint (Joints() order), from the last AfterStep that followed a Flush
    const std::vector<JointLoad>& JointLoads() const { return jointLoads_; }
    // the element loads of the last solve: per joint, per live sub-joint (physics frame, on the host side)
    const std::vector<std::array<ElementLoad, 3>>& SubJointLoads() const { return subLoads_; }
    // the sub-joints over capacity at the last AfterStep (options.breakJoints); the host acts on them (VesselPhysicsHost::AfterStepAll)
    const std::vector<JointBreak>& Breaks() const { return breaks_; }
    LoadSolve LastSolve() const { return lastSolve_; }  // the solve that ran (None when no loads were computed)
    // options.breakJoints at run time (a host's grace period: the spike skipped breaks for the 60 ticks of Vessel.ignoreCollisionsFrames
    // after its approximate ground placement, which drops the vessel a few cm onto the pad)
    void SetBreakJoints(bool on) { options_.breakJoints = on; }
    float MaxLoadRatio() const { return maxRatio_; }
    int MaxLoadJoint() const { return maxRatioJoint_; }
    // the parts touching static geometry in the last step (vessel part indices of the nodes that got a ground support)
    const std::vector<int>& SupportParts() const { return supportParts_; }
    double LastLoadSolveMs() const { return lastSolveMs_; }
    double LastFactorMs() const { return lastFactorMs_; }
    size_t FactorCount() const { return factorCount_; }  // graph factorisations so far
    // live sub-joints of a joint (bit k = sub-joint k); all bits of internalJoints while the PartJoint stands
    uint8_t SubJointMask(size_t joint) const { return subAlive_[joint]; }
    // Unity removed these ConfigurableJoints (a PhysX break): the rest of the PartJoint carries on; when none is left the joint is cut
    // (Cut of its child: the clusters split, the vessel does not - PartJoint.OnJointBreak returned early, PartJoint.cs:399)
    void DestroySubJoints(size_t joint, uint8_t mask);
    // Part.CycleAutoStrut for every part (Part.cs:8043, :8217): the autostruts go now, SecureAutoStruts re-creates them after
    // options.autoStrutDelaySteps steps (at once when 0)
    void CycleAutoStruts();
    size_t AutoStrutJointCount() const;
    bool AutoStrutsPending() const { return autoStrutCountdown_ > 0; }

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
    PartPhysicsState StateAt(int i) const;
    void MapVesselParts() const;  // scratchIdx_[k] = record of vessel.parts[k] (identity while the part list is unchanged)
    Vector3d PartPositionNow(int i) const;
    QuaternionD PartRotationNow(int i) const;
    Vector3d PartComNow(int i) const;
    // WP-20c (src/physics/vessel_physics_loads.cpp)
    std::function<const PartTransform&(const Part&)> VesselFrameTransforms();  // orgPos / orgRot of the binding's parts
    void SecureAutoStruts();
    void ReleaseAutoStruts();
    void RebuildWelds();  // the inter-cluster fixed constraints from the live joints
    void ComputeLoads();
    void BuildLoadModel(const std::vector<int>& supports);
    Vector3d FactorFrameOrigin() const;

    PhysicsWorld* world_;
    Vessel* vessel_;
    VesselPhysicsOptions options_;
    uint32_t group_ = 0;
    bool sameVesselCollisionAll_ = false;
    std::vector<std::unique_ptr<PartRec>> parts_;  // vessel part order at build
    detail::PartIndexMap index_;
    std::vector<PartJointSpec> joints_;  // attach joints, struts, then the secured autostruts (at the end)
    std::vector<bool> cut_;
    std::vector<uint8_t> subAlive_;      // live sub-joints per joint
    std::vector<ClusterRec> clusters_;
    struct Weld {
        ConstraintId id = kInvalidConstraint;
        int a = -1, b = -1;              // clusters (the impulse applies to b, its opposite to a)
        Vector3d pointBodyA;             // the weld point in cluster a's body frame (relative to its origin at creation)
    };
    std::vector<Weld> welds_;
    std::vector<ConstraintId> constraints_;
    std::vector<Vector3d> partForce_, partMoment_;
    Vector3d uniformAccel_;
    // ---- WP-20c state
    int autoStrutCountdown_ = 0;         // steps until SecureAutoStruts (0: none pending)
    size_t autoStrutBegin_ = 0;          // index of the first autostrut in joints_ (== joints_.size() without autostruts)
    bool stepValid_ = false;             // Flush captured a step (AfterStep may compute loads)
    float stepDt_ = 0.0f;
    Vector3d stepAccel_;
    std::vector<Vector3d> stepForce_, stepMoment_;  // per part: the external force / moment (about its centre of mass) of the step
    std::vector<Vector3d> clusterVBefore_, clusterWBefore_;
    std::vector<JointLoad> jointLoads_;
    std::vector<std::array<ElementLoad, 3>> subLoads_;
    std::vector<JointBreak> breaks_;
    LoadSolve lastSolve_ = LoadSolve::None;
    float maxRatio_ = 0.0f;
    int maxRatioJoint_ = -1;
    double lastSolveMs_ = 0.0, lastFactorMs_ = 0.0;
    size_t factorCount_ = 0;
    std::vector<int> supportParts_;
    // the load model, rebuilt when the structure or the support set changes (structureDirty_)
    bool structureDirty_ = true;
    std::vector<int> modelSupports_;     // the supports the model was built for
    bool modelGraph_ = false;            // the model's solve (graph or tree)
    bool modelWantGraph_ = false;        // the solve the model was built for (a failed factorisation falls back to the tree)
    std::vector<std::array<Vector3d, 3>> anchor0_;  // per joint, per sub-joint: the anchor in the factor frame
    std::vector<int> nodeOfPart_;        // part index -> node (its RigidBodyPart's), -1 without one
    std::vector<int> partOfNode_;
    std::vector<int> treeParentNode_, treeParentJoint_;
    std::vector<Vector3d> nodeCom0_;     // node centres of mass in the factor frame (the clusters' creation frame, FactorFrameOrigin())
    std::vector<Vector3d> centroid0_;    // joint anchor centroids, factor frame
    std::vector<JointSplit> splits_;     // per joint (tree solve)
    std::vector<std::array<int, 3>> splitSub_;  // per joint: the sub-joint index of each JointSplit element
    StructuralGraph graph_;
    std::vector<StructElement> elements_;
    std::vector<ElementLoad> elementLoads_;
    std::vector<NodeDemand> demand_;
    // scratch of ReadState (not thread-safe: one reader at a time per binding)
    mutable std::vector<int> scratchIdx_;
    mutable std::vector<PartRigidbodyState> scratchStates_;
    mutable std::vector<Vector3d> scratchRel_;
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
//
// WP-20c: Install() also adds GameEvents listeners and the module force hooks, and AfterStepAll acts on joint breaks:
//   onPartDeCoupleNewVesselComplete (Part.decouple, Part.cs:2833)   the original and the new vessel are rebound
//   onVesselsUndocking (Part.Undock, :3352)                          the same
//   onPartCouple (Part.Couple / docking, :3173)                      the source vessel's binding lets go of its Vessel (Couple destroys it);
//                                                                    both bindings are rebound into the target vessel
//   onPartDie (Part.Die, :4464)                                      the part's vessel is rebound (the dead part leaves with its momentum)
//   onVesselWasModified (:2613, 2708, 2831, 3209, 3350, 4451)        Part.VesselModified -> CycleAutoStrut (Part.cs:7214); a vessel
//                                                                    whose part list or rigid bodies changed is rebound
//   ModuleDecouplerBase::addForce / addForceAtPosition, DockingEnvironment::addTorque / partVelocity: Part.AddForce* / rb.velocity
// Rebinds are DEFERRED: the events only collect the bindings and vessels concerned; ProcessStructuralChanges() rebinds them all at once
// from the part states (every tick method and every env hook runs it first, so staging N decouplers in one frame costs one rebind).
// A joint break (VesselPhysics::Breaks) is PartJoint.OnJointBreak (PartJoint.cs:397): the broken ConfigurableJoints are gone; unless the
// host vessel ignores collisions (IgnoreCollisionsFrames > 0) GameEvents.onPartJointBreak fires and the child part's
// OnPartJointBreak (Part.cs:3159) decouples it - the decouple event then rebinds.
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
    // the same with the bindings themselves (Part.Couple destroys the source Vessel: look its binding up before the tree operation)
    VesselPhysics* OnVesselMerge(VesselPhysics* a, VesselPhysics* b, Vessel& merged);

    // ---- per tick, in creation order (deterministic) ----------------------------------------------------------------------
    void RefreshFrames();     // frameOriginWorld / floatOrigin of every binding
    void IntegrateAll();      // IntegratePartBuffers(vessel.precalc.integrationAccel) for every binding
    void FlushAll(float dt);
    // AfterStep of every binding, then the joint breaks (see the class comment), then ProcessStructuralChanges
    void AfterStepAll();
    // FlushAll, PhysicsWorld::Step(dt), AfterStepAll (the tick's physics half for hosts and tests)
    void Step(float dt);

    // ---- WP-20c: structural events -------------------------------------------------------------------------------------------
    // the deferred rebinds of the events since the last call (see the class comment); no-op when none is pending
    void ProcessStructuralChanges();
    bool StructuralChangesPending() const { return !pendingSources_.empty() || !pendingTargets_.empty(); }
    // a change the events do not report (Part.DemoteToPhysicslessPart / PromoteToPhysicalPart, parts added by the host): `vessel` is rebound
    void RequestRebind(Vessel& vessel);
    struct BreakRecord {
        Part* child = nullptr;     // PartJoint.child (the part that decouples)
        Part* parent = nullptr;
        size_t joint = 0;
        uint8_t mask = 0;
        float breakForce = 0.0f;   // what OnJointBreak received (the broken sub-joint's breakForce)
        float force = 0.0f, torque = 0.0f;  // the sub-joint load that broke it
        bool decoupled = false;    // false: IgnoreCollisionsFrames kept the part in its vessel
    };
    const std::vector<BreakRecord>& LastBreaks() const { return lastBreaks_; }  // the breaks of the last AfterStepAll
    size_t RebindCount() const { return rebinds_; }
    double LastRebindMs() const { return lastRebindMs_; }

private:
    PhysicsWorld* world_;
    std::vector<std::unique_ptr<VesselPhysics>> vessels_;  // creation order
    bool installed_ = false;
    VesselEnvironment saved_;
    struct SavedHooks;
    std::unique_ptr<SavedHooks> savedHooks_;
    std::vector<VesselPhysics*> pendingSources_;  // event order
    std::vector<Vessel*> pendingTargets_;
    std::vector<BreakRecord> lastBreaks_;
    size_t rebinds_ = 0;
    double lastRebindMs_ = 0.0;
    VesselPose DefaultUnpackPose(Vessel& v) const;
    void ApplyFrame(VesselPhysics& vp) const;
    VesselPhysics* BindingForPart(Part& p);  // ProcessStructuralChanges, then the binding of p.vessel
    void MarkRebind(VesselPhysics* source, Vessel* target);
    void Forget(const VesselPhysics* vp);  // a binding goes: no longer a pending source
    void HandleBreak(VesselPhysics& vp, const VesselPhysics::JointBreak& b);
    // GameEvents listeners
    void OnDecoupleNewVessel(Vessel* original, Vessel* created);
    void OnVesselsUndocking(Vessel* original, Vessel* created);
    void OnPartCouple(events::GameEvents::FromToAction<Part*, Part*> action);
    void OnPartDie(Part* part);
    void OnVesselWasModified(Vessel* vessel);
};

}  // namespace ksp::physics

namespace ksp {

// PartJoint.cs as the payload of GameEvents.onPartJointBreak (PartJoint.cs:407): kspcore keeps the joints as data (physics::PartJointSpec)
// in the vessel's binding, so the host hands the listeners a PartJoint made from that record for the duration of the event.
class PartJoint {
public:
    explicit PartJoint(const physics::PartJointSpec& spec) : spec_(spec) {}
    const physics::PartJointSpec& Spec() const { return spec_; }
    Part* Child() const { return spec_.child; }    // PartJoint.Child
    Part* Parent() const { return spec_.parent; }  // PartJoint.Parent
    Part* Host() const { return spec_.host; }      // PartJoint.Host
    Part* Target() const { return spec_.target; }  // PartJoint.Target
    float stiffness() const { return spec_.stiffness; }

private:
    physics::PartJointSpec spec_;
};

}  // namespace ksp

namespace ksp::physics {

}  // namespace ksp::physics
