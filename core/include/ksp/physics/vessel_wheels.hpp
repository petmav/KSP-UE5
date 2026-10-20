// WP-29c: wheels and landing legs on the reduced rigid-body model (WHEEL-SPIKE.md section 2, option A). Every ModuleWheelBase part of a
// loaded, unpacked vessel is a suspension element of its WP-20b cluster body:
//
//   managed   (C#, ported 1:1 in WP-29a / WP-29b) SuspensionLoadBalancer.FixedUpdate, then per wheel part its ModuleWheel* FixedUpdates in module
//             order and its KSPWheelController (VehicleBase.FixedUpdate: reads the WheelHit of the LAST physics step, integrates the wheel
//             spin, applies the tyre force at the contact point, writes the WheelCollider's spring / damper / target for this step). It runs
//             in the part-module slot of the tick (FlightScene::hooks.beforeFlightIntegrator, after the WP-20d module forces), as in KSP
//             (vessel modules, part modules and the runtime-added controller all run between seq 22 and 276, PHYSICS-ENGINE-REVIEW.md 1.2).
//   native    (UnityEngine.WheelCollider = PhysX 4.1, ksp/wheels/native_wheel_collider.hpp) in the physics step, before the solver: one ray per
//             wheel against the PhysicsWorld without the vessel's own bodies (terrain colliders of WP-12b, other vessels), the suspension
//             law, the travel-limit row (projected Gauss-Seidel on the cluster's predicted velocity), the WheelHit for the next tick.
//   forces    the tyre force (rb.AddForceAtPosition), the suspension force and the limit impulses act on the cluster at their points, booked on
//             the wheel PART (VesselPhysics::AddForceAtPhysicsPoint), so WP-20c's structural solve sees them as external loads on the wheel
//             part's joint; the reactions on another vessel (VPWheelCollider.ApplyForce's -F clamp01(m_other / m), PhysX's hit-actor force) go
//             to that vessel's cluster.
//   mass      the wheel part is welded into its cluster with its rb.mass and inertia; it has no collision shape (VesselPhysicsOptions::massOnly:
//             its tyre is the ray). The WheelCollider's sprungMass is the wheel part's rb.mass (one collider per Rigidbody in Unity).
//
// PHYSICS WARP (decision of 2026-10-13, WHEEL-SPIKE.md 2.3 / R3): a tick longer than `substepDt` (Time.fixedDeltaTime = 0.02 x rate,
// TimeWarp.cs:423) with a wheeled vessel in the bubble runs as n = ceil(dt / substepDt) physics substeps of dt / n: the managed wheel tick
// and the native step run every substep with Time.fixedDeltaTime = Time.deltaTime = dt / n, the forces the rest of the tick buffered
// (FlightIntegrator, engines, gravity) act unchanged in every substep (VesselPhysics::BufferedForces), each substep is one
// VesselPhysicsHost::Step. A deliberate change: KSP integrates the whole tick in one step of 0.02 x rate, where the explicit suspension and
// VPP's brake / tyre model are unstable (the spike: a braked Bug-E lifts off at 4x, a lander at 3x).
//
// TYRE CURVE (UNVERIFIED pending capture C11b): ModuleWheelBase.cs:361 assigns `tireFriction.model = Lineal` after the controller's
// TireFriction built its SmoothFriction curve, and nothing rebuilds the curve (TireFriction.SetupFrictionCurves runs only from the constructor
// and OnAfterDeserialize). `TyreCurve::AsDecompiled` (default) keeps the C# reading: the Smooth curve over the Lineal settings; `Lineal`
// rebuilds the curve after the set-up, the behaviour the assignment intends. The difference is ~3 % of acceleration at 0.5-3 m/s of slip.
//
// GEOMETRY (UNVERIFIED, WHEEL-SPIKE.md R4): the wheel collider transforms are in the parts' .mu models (decompiled/KSP-dev; the model reader).
// `WheelPhysicsOptions::geometry` supplies them; unset, the spike's proxy: the tyre bottom at full extension is the bottom face of the part's
// drag cube ("Deployed" cube when the part has one), the suspension along the vessel's up, the wheel rolling along the vessel's forward.
//
// Not here (deferred): the "schloompa" rigid-body freeze (ModuleWheelBase.ToggleRbConstraints: a 2 s anchor of a braked vessel at rest, traced
// by WP-29b), the leg stand-in spheres and piston colliders of ModuleWheelDeployment, the visual suspension / wheel transforms, the impact
// collider of ModuleWheelDamage, KSC scenery tags of landedAt (terrain reports layer 15, "Untagged").
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <optional>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/parts/part_model_rig.hpp"
#include "ksp/unity/animation.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/physics/physics_world.hpp"
#include "ksp/physics/vessel_physics.hpp"
#include "ksp/physics/wheel_models.hpp"
#include "ksp/wheels/native_wheel_collider.hpp"
#include "ksp/wheels/wheel_modules.hpp"

namespace ksp {
class Part;
class Vessel;
class PartModule;
}  // namespace ksp

namespace ksp::physics {

class PhysicsBubble;

enum class TyreCurve : uint8_t {
    AsDecompiled,  // the TireFriction constructor's SmoothFriction stays (the C# reading of ModuleWheelBase.cs:361); UNVERIFIED, capture C11b
    Lineal,        // the curve rebuilt for the Lineal model ModuleWheelBase assigns
};

// Where the wheel collider transform (ModuleWheelBase.wheelColliderTransformName) of a wheel part sits in PART space. Every other model
// transform the wheel modules look up (wheel, suspension, steering, caliper, bogey ...) is given the same pose until the model reader exists.
struct WheelPartGeometry {
    Vector3f position;
    Quaternionf rotation = Quaternionf::identity();
};

struct WheelPhysicsOptions {
    bool substeps = true;          // physics-warp substeps of wheeled vessels
    float substepDt = 0.02f;
    TyreCurve tyreCurve = TyreCurve::AsDecompiled;
    int limitIterations = 4;       // projected Gauss-Seidel sweeps of the travel-limit rows
    float limitBaumgarte = 0.2f;   // fraction of the over-compression corrected per step (PhysX corrects it in its position iterations)
    bool reactOnOtherVessels = true;
    bool startModules = true;      // run OnStart of the wheel modules (Part.ModulesOnStart's subset) when a vessel's parts have started
    // WP-29d: the model data of the wheel parts (transforms by name, animations, hub colliders): the stock library (core/data/wheel_models)
    // or any source of PartModelRigs (the real GameData's .mu files). With neither, the WP-29c drag-cube proxy below.
    std::shared_ptr<const WheelModelLibrary> models;
    std::function<std::shared_ptr<mu::PartModelRig>(const Part&)> modelRig;
    // WP-29d: the tag of the terrain collider a wheel reports (ModuleWheelBase.LandingDetectionUpdate reads collider.tag: a KSC runway
    // collider is tagged Runway). The default is the PQS quads: Untagged. A FIXTURE until WP-12b / WP-42 register the scenery colliders.
    std::string groundTag = "Untagged";
    bool modelColliders = true;    // the hub colliders from the models weld into the cluster (else the wheel parts stay mass-only)
    // the geometry of a wheel part (true: answered); unset or false: the model rig, then the drag-cube proxy below
    std::function<bool(const Part&, const ModuleWheelBase&, WheelPartGeometry&)> geometry;
    // the proxy's up and forward in the VESSEL frame (the root part's frame: orgPos / orgRot). Unset: up = the local vertical (vessel.upAxis)
    // in the vessel frame when the wheel is set up (the vessel stands on its wheels), +Y without one; forward = the root part's up projected
    // on the plane normal to up (the spike's rule)
    std::function<Vector3f(const Vessel&)> proxyUp;
    std::function<Vector3f(const Vessel&)> proxyForward;
};

// One wheel part and the Unity objects behind it
struct WheelPart {
    Part* part = nullptr;
    ModuleWheelBase* base = nullptr;
    std::vector<PartModule*> modules;                    // ModuleWheelBase and the ModuleWheel* submodules, module order
    VehiclePhysics::NativeRigidbody rb;                  // part.Rigidbody (the cluster's motion at the part)
    VehiclePhysics::NativeTransform rbTransform;         // part.Rigidbody.transform (the part transform)
    VehiclePhysics::NativeTransform colliderHost;        // the wheel collider transform (and every other model transform of the part)
    std::map<std::string, std::unique_ptr<VehiclePhysics::NativeTransform>> named;  // the other names (same pose as colliderHost)
    WheelPartGeometry geometry;                          // part space
    bool geometryFromProxy = false;
    // WP-29d: the model rig (null: proxy geometry) and the Unity objects built on it
    std::shared_ptr<mu::PartModelRig> rig;
    int colliderNode = -1;                                // rig node of wheelColliderTransformName
    std::map<int, std::unique_ptr<VehiclePhysics::NativeTransform>> rigTransforms;  // FindModelTransform results by rig node
    struct AnimBinding {
        int rigAnimation = -1;
        std::unique_ptr<Animation> anim;
    };
    std::vector<AnimBinding> animations;
    VehiclePhysics::NativeTransform colliderSet;          // the pose the host last gave the collider host (a module write is a difference)
    bool standInBuilt = false;                            // the cluster compound holds the part stand-in sphere
    Vector3f standInCenter;                               // ... at this part-space point
    bool geometryLocked = false;                          // ModuleWheelBase.wheelSetup re-parented the collider transform to the part
    bool started = false;                                // the wheel modules' OnStart ran
    bool nativeOn = false;                               // the PhysX wheel exists (collider enabled)
    VehiclePhysics::NativeWheelStep last;                // the last native step (diagnostics)
    VehiclePhysics::NativeWheelGeometry lastGeometry;
    VehiclePhysics::KSPWheelController* Controller() const;
};

class WheelPhysicsHost;

// The wheel parts of one vessel and its SuspensionLoadBalancer (a VesselModule in KSP)
class VesselWheels {
public:
    VesselWheels(WheelPhysicsHost& host, Vessel& vessel);
    ~VesselWheels();
    Vessel& GetVessel() const { return *vessel_; }
    std::vector<std::unique_ptr<WheelPart>>& Wheels() { return wheels_; }
    const std::vector<std::unique_ptr<WheelPart>>& Wheels() const { return wheels_; }
    SuspensionLoadBalancer* LoadBalancer() const { return balancer_.get(); }
    WheelPart* Find(const Part& p) const;
    int SetUpCount() const;     // wheels whose ModuleWheelBase finished wheelSetup
    int GroundedCount() const;  // wheels on the ground after the last native step

private:
    friend class WheelPhysicsHost;
    WheelPhysicsHost* host_;
    Vessel* vessel_;
    std::vector<std::unique_ptr<WheelPart>> wheels_;
    std::unique_ptr<SuspensionLoadBalancer> balancer_;
    bool balancerStarted_ = false;
    std::vector<Part*> lastParts_;  // the wheel parts at the last refresh (a change re-collects the balancer's suspensions)
};

class WheelPhysicsHost {
public:
    explicit WheelPhysicsHost(PhysicsBubble& bubble, const WheelPhysicsOptions& options = {});
    ~WheelPhysicsHost();  // Uninstall
    WheelPhysicsHost(const WheelPhysicsHost&) = delete;
    WheelPhysicsHost& operator=(const WheelPhysicsHost&) = delete;

    WheelPhysicsOptions options;
    PhysicsBubble& Bubble() const { return *bubble_; }

    // WheelModuleEnvironment's hooks, VesselPhysicsOptions::massOnly of the bubble's host (before any vessel unpacks), the managed slot after
    // the module forces (FlightScene::hooks.beforeFlightIntegrator), the frame (FlightScene::hooks.beforeVesselUpdate) and the bubble's
    // physicsStep. Uninstall (or the destructor) restores them; destroy this host before the bubble.
    void Install();
    void Uninstall();
    bool Installed() const { return installed_; }

    // ---- per tick (Install wires them; a host without a bubble tick may call them itself) ------------------------------------------
    // the managed wheel tick of every wheeled vessel (substep 0 of a warp tick, with the substep's Time.fixedDeltaTime)
    void FixedUpdateManaged();
    // the physics step of a tick of `dt`: substeps, native steps, VesselPhysicsHost::Step per substep
    void PhysicsStep(float dt);
    // the frame: the controllers' Update (steer angle into the collider, VehicleBase.Update), the modules' Update / LateUpdate, the coroutine
    // pass of the wheel modules (wheelSetup waits there for an unpacked part)
    void Update();

    // the substeps of a tick of `dt` (1: none): options.substeps, dt > substepDt and a loaded unpacked vessel with a set-up wheel
    int SubstepCount(float dt) const;
    int LastSubsteps() const { return lastSubsteps_; }

    VesselWheels* Find(const Vessel& v) const;
    const std::vector<std::unique_ptr<VesselWheels>>& Vessels() const { return vessels_; }
    CoroutineScheduler& Scheduler() { return scheduler_; }
    // WP-29d: the wheel parts whose Rigidbody is frozen (ModuleWheelBase.toggleRbConstraints, FreezeAll) and the welds that hold them
    size_t FrozenPartCount() const { return frozen_.size(); }
    size_t FreezeWeldCount() const { return freezes_.size(); }
    // the collider the terrain / static geometry hits report (layer 15 "Local Scenery", tag "Untagged", no rigidbody)
    VehiclePhysics::Collider& TerrainCollider() { return terrain_; }

    // the drag-cube proxy of WheelPhysicsOptions::geometry (public: tests and the bench place vessels with it)
    static WheelPartGeometry ProxyGeometry(const Part& part, const ModuleWheelBase& base, const Vector3f& upVessel, const Vector3f& forwardVessel);

private:
    friend class VesselWheels;
    struct OtherPart {  // the Unity objects of another vessel's part a wheel stands on
        Part* part = nullptr;
        Vessel* vessel = nullptr;
        VehiclePhysics::NativeRigidbody rb;
        VehiclePhysics::Collider collider;
    };
    struct PendingForce {  // a managed force in the physics frame, applied in the physics step
        Vessel* vessel;
        Part* part;
        Vector3d force, point;
    };
    void Refresh();                                   // a VesselWheels per loaded vessel with wheel parts; drops the gone ones
    void StartWheelModules(VesselWheels& vw);
    void SyncNative(VesselWheels& vw, VesselPhysics& vp);
    void ManagedTick(VesselWheels& vw, VesselPhysics& vp);
    void CollectManagedForces(VesselWheels& vw, VesselPhysics& vp);
    void ApplyPendingForces();
    void NativeStep(float h);
    void NativeStepVessel(VesselWheels& vw, VesselPhysics& vp, float h);
    void ManagedSubstep(float h);
    WheelPartGeometry GeometryOf(const Part& part, const ModuleWheelBase& base) const;
    std::shared_ptr<mu::PartModelRig> RigFor(const Part& part) const;      // WP-29d: a fresh rig of the part, null without model data
    void SampleAnimations(WheelPart& w);                                    // Animation.Sample of every enabled state onto the rig
    void TickAnimations(float h);                                           // the animatePhysics update of the wheel parts' animations
    void WriteBackGeometry(WheelPart& w);                                   // a module moved the collider transform (SetWheelTransform): keep it
    Animation* FindAnimator(Part& p, const std::string& trf, const std::string& state);
    void SampleAnimation(Animation& a);
    std::vector<ColliderProxy> HubColliders(const Part& part);
    std::optional<ColliderProxy> StandInProxy(const Part& part) const;      // the retracted gear sphere (ModuleWheelDeployment.standInCollider)
    void UpdateStandIns();
    void ApplyFreezes();   // the brake "schloompa": a frozen wheel part holds its cluster (a weld to the world) for the freeze time
                                                  // rebuild the cluster shapes when a stand-in appears, goes or moves
    std::shared_ptr<mu::PartModelRig> SampledRig(const Part& part) const;   // the rig posed at the part ModuleWheelDeployment state
    // getLowestPoint collider scan for a wheel part (Vessel.cs:2589-2637, VesselEnvironment::colliderClosestPoints)
    std::vector<VesselEnvironment::ColliderPoint> PlacementPoints(Part& part, const Vector3f& measure);
    OtherPart* OtherPartFor(VesselPhysics& owner, uint32_t partIndex);
    bool AnyWheelSetUp() const;
    VehiclePhysics::NativeTransform* FindModelTransform(Part& p, const std::string& name);

    PhysicsBubble* bubble_;
    std::vector<std::unique_ptr<VesselWheels>> vessels_;
    std::vector<std::unique_ptr<WheelPart>> dormant_;  // records of wheel parts in no loaded vessel (kept while the Part lives)
    std::map<const Part*, std::unique_ptr<OtherPart>> others_;
    std::vector<PendingForce> pending_;
    std::set<const Part*> frozen_;
    struct Freeze {
        ConstraintId id = kInvalidConstraint;
        BodyId body = kInvalidBody;
    };
    std::map<VesselPhysics*, Freeze> freezes_;
    VehiclePhysics::Collider terrain_;
    CoroutineScheduler scheduler_;
    bool installed_ = false;
    int lastSubsteps_ = 1;
    float timeSinceLevelLoad_ = 0.0f;
    struct Saved;
    std::unique_ptr<Saved> saved_;
};

}  // namespace ksp::physics
