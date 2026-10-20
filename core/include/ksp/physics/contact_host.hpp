// WP-20e: contact, landing and water in the physics bubble. The host of everything KSP did through Unity's collision callbacks, its
// per-part MonoBehaviours and its raycasts, on the reduced model (one Jolt body per rigid cluster of parts):
//
//   collision messages  PhysicsWorld::ContactEvents (per collider pair: a part's proxy collider against a static collider or another part's)
//                       -> a Collision record (ksp/physics/collision.hpp) -> Part.OnCollisionEnter / OnCollisionStay / OnCollisionExit
//                       (src/parts/part_contact.cpp: currentCollisions, LandedCollisionChecks -> GroundContact / vessel.Landed / landedAt,
//                       CheckCollision -> HandleCollision's crash tolerance -> Part.explode -> Part.Die) on the part whose Rigidbody owns
//                       the touching collider; between two vessels both sides get theirs (Unity sends OnCollisionEnter to both bodies).
//                       Collision.relativeVelocity: the two parts' rigid-body velocities (at their centres of mass) when Jolt found the
//                       contact, before the contact solve (PhysX's pre-solver velocities). Unity's dispatch order is PhysX's report order;
//                       here: the new contacts (Enter), the kept ones (Stay), then the lost ones (Exit), each sorted (deterministic; new
//                       contacts first, so a part sliding from one terrain quad to the next never reads as airborne for a moment).
//                       Several world steps in one FixedUpdate (WP-29c's physics-warp substeps): every step's Enter / Exit in step order,
//                       OnCollisionStay once per pair (PhysicsWorld::TakeContactEventLog).
//   static colliders    every static body of the world is a collider with a GameObject: layer (15 Local Scenery for terrain and KSC, the
//                       default), tag ("Untagged", or LaunchPad / Runway ... of KSC objects: Vessel.SetLandedAt), name, a PQ (terrain
//                       quad: the body), CrashObjectName. RegisterStatic / RegisterStaticRange; TerrainColliders hosted per body register
//                       their key range as PQ quads.
//   PartBuoyancy        one per part with a Rigidbody (Part.cs:1885), FixedUpdate after the part-module slot (seq 13), enabled while the
//                       vessel is unpacked; its rb.drag / angularDrag on the binding, its force through Part.AddForceAtPosition.
//   CollisionEnhancer   one per part with a Rigidbody (Part.cs:1884), FixedUpdate at FlightScene::hooks.afterPrecalc (seq 7, after
//                       VesselPrecalculate / OrbitDriver), the terrain linecast through PhysicsWorld::RayCast.
//   RB anchor           Vessel.AddRBAnchor's FixedJoint (Vessel.cs:1162) as a fixed-to-world weld of the root part's cluster
//                       (PhysicsWorld::CreateFixedToWorldConstraint); its force read after every step (joint.currentForce), broken as
//                       PhysX broke it (force > breakForce or torque > breakTorque -> Vessel.OnJointBreak). The rocking of a vessel resting
//                       on many contacts (WP-20b R2) ends when it anchors (1.5 s still and landed: KSP's rule).
//   ground probes       Vessel::env.heightFromTerrainProbe (GetHeightFromTerrain's raycast, layer 15), groundRaycast and
//                       colliderClosestPointsZ (CheckGroundCollision / getLowestPoint over the parts' proxy colliders), checkGroundCollision
//                       = Vessel::CheckGroundCollision (the placement of a landed vessel at GoOffRails).
//   terrain per body    HostTerrain(body, sphere): a TerrainColliders (WP-12b) for the PQS sphere of a body, its quads placed in the bubble's
//                       static frame (origin = StaticPhysicsFromWorld(body.position), rotation = the body's), valid while the body is the
//                       dominant body in its rotating frame: on onRotatingFrameTransition / onDominantBodyChange they are rebuilt or removed.
//
// The whole host is optional per piece (ContactHostOptions); PhysicsBubble owns one and runs it by default.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "ksp/physics/collision.hpp"
#include "ksp/physics/collision_enhancer.hpp"
#include "ksp/physics/part_buoyancy.hpp"
#include "ksp/physics/physics_world.hpp"
#include "ksp/physics/vessel_physics.hpp"
#include "ksp/terrain/terrain_colliders.hpp"

namespace ksp {
class CelestialBody;
class PQS;
class Vessel;
class Part;
}  // namespace ksp

namespace ksp::physics {

class PhysicsBubble;

// The GameObject of a static collider, as Part.OnCollisionEnter / Vessel.SetLandedAt / HandleCollision read it
struct StaticColliderInfo {
    int layer = 15;                  // Local Scenery
    std::string tag = "Untagged";
    std::string name = "Terrain";
    bool isPQ = false;               // a PQ (terrain quad) component on the GameObject
    CelestialBody* pqBody = nullptr; // its sphere's body
    std::string crashObjectName;     // CrashObjectName.objectName ("" = none)
    bool isMobileLaunchPad = false;  // PositionMobileLaunchPad in its parents (CheckGroundCollision)
};

struct ContactHostOptions {
    bool collisions = true;         // Part.OnCollisionEnter / Stay / Exit from the contact events
    bool buoyancy = true;           // PartBuoyancy
    bool collisionEnhancer = true;  // CollisionEnhancer
    bool rbAnchor = true;           // Vessel.AddRBAnchor's joint (Vessel::env.rbAnchor*)
    bool groundProbes = true;       // Vessel::env.heightFromTerrainProbe / groundRaycast / colliderClosestPointsZ / checkGroundCollision
    float contactOffset = 0.01f;    // Physics.defaultContactOffset (Unity's default)
};

class ContactHost {
public:
    ContactHost(PhysicsBubble& bubble, const ContactHostOptions& options = {});
    ~ContactHost();
    ContactHost(const ContactHost&) = delete;
    ContactHost& operator=(const ContactHost&) = delete;

    ContactHostOptions options;

    // Installs the Vessel::env hooks (saving what they replace) and the contact-event tracking; Uninstall restores them.
    void Install();
    void Uninstall();
    bool Installed() const { return installed_; }

    // ---- static colliders ----------------------------------------------------------------------------------------------------------
    void RegisterStatic(uint64_t key, const StaticColliderInfo& info);
    void RegisterStaticRange(uint64_t keyBase, uint64_t count, const StaticColliderInfo& info);  // keys [keyBase, keyBase + count)
    void UnregisterStatic(uint64_t key);
    const StaticColliderInfo& StaticInfo(uint64_t key) const;  // the default (terrain, untagged) for unregistered keys

    // ---- terrain colliders per celestial body (WP-12b's seam 3) ----------------------------------------------------------------
    // A TerrainColliders for `sphere` (the PQS of `body`) feeding the bubble's world: keys keyBase + quad id (a per-body base when 0),
    // registered as PQ quads of `body`. The quads are placed with the body's current frame (origin / rotation); the host rebuilds them on
    // frame changes (Refresh). Returns the manager (owned by the host; DropTerrain before the sphere goes), null while the body is not in
    // its rotating frame (no colliders then; RefreshTerrain builds them when it is).
    TerrainColliders* HostTerrain(CelestialBody& body, PQS& sphere, TerrainCollidersOptions opt = {});
    void DropTerrain(CelestialBody& body);
    TerrainColliders* Terrain(const CelestialBody& body) const;
    // the frame of a body's quads: physics = origin + rotation * (PQS local)
    Vector3d TerrainOrigin(const CelestialBody& body) const;
    QuaternionD TerrainRotation(const CelestialBody& body) const;
    // re-places every hosted body's colliders in its current frame (onRotatingFrameTransition / onDominantBodyChange call it; hosts call it
    // after moving a body's frame themselves). A body that is not the dominant body in its rotating frame has its colliders removed.
    void RefreshTerrain();
    size_t TerrainRefreshCount() const { return terrainRefreshes_; }
    // WP-12c: other static colliders that live in a body frame (the KSC scenery, SceneryCityHost) re-place themselves with RefreshTerrain
    uint64_t AddRefreshHook(std::function<void()> hook);
    void RemoveRefreshHook(uint64_t id);

    // ---- per tick (PhysicsBubble calls these) ----------------------------------------------------------------------------------
    void CollisionEnhancerFixedUpdate();  // FlightScene::hooks.afterPrecalc (seq 7)
    void BuoyancyFixedUpdate();           // after the part-module slot (seq 13)
    void AfterPhysicsStep(float dt);      // after PhysicsWorld::Step and the bindings' AfterStep: anchors, then the collision messages
    // the world shift the bubble applied (ShiftWorld) and the Galilean frame drift of the tick (CollisionEnhancer's FloatingOrigin.Offset)
    void AddWorldShift(const Vector3d& shift) { pendingShift_ += shift; }

    // ---- results (tests) -------------------------------------------------------------------------------------------------------
    struct DispatchRecord {
        PhysicsWorld::ContactEvent::Kind kind = PhysicsWorld::ContactEvent::Kind::Enter;
        Part* receiver = nullptr;     // the part OnCollision* was called on
        Part* colliderPart = nullptr; // its collider's part
        UnityEngine::Collision collision;
    };
    const std::vector<DispatchRecord>& LastDispatch() const { return lastDispatch_; }
    bool recordDispatch = false;  // keep LastDispatch (off: no copies)
    uint64_t EnterCount() const { return enters_; }
    uint64_t ExitCount() const { return exits_; }
    uint64_t StayCount() const { return stays_; }
    // world steps whose contact events the last dispatch delivered (1 per FixedUpdate; WP-29c's physics-warp substeps: more)
    size_t SubstepsDispatched() const { return substepsDispatched_; }
    PartBuoyancy* Buoyancy(const Part& part) const;
    CollisionEnhancer* Enhancer(const Part& part) const;
    size_t AnchorCount() const { return anchors_.size(); }
    // the anchor's force of the last step (joint.currentForce) of a vessel; zero without one
    Vector3f AnchorForce(const Vessel& v) const;
    // FX hooks (FXMonger.Splash); unset: recorded only (PartBuoyancy::lastSplashFx*)
    std::function<void(const Vector3f& position, float power)> splashFx;

private:
    struct PairMemory {
        Part* colliderPart1 = nullptr;
        Part* colliderPart2 = nullptr;   // null for a static collider
        uint64_t otherKey = 0;
        bool otherStatic = false;
    };
    struct Anchor {
        ConstraintId id = kInvalidConstraint;
        BodyId body = kInvalidBody;
        float breakForce = 0.0f, breakTorque = 0.0f;
        Vector3f currentForce;
    };
    struct HostedTerrain {
        CelestialBody* body = nullptr;
        PQS* sphere = nullptr;
        TerrainCollidersOptions opt;
        std::unique_ptr<TerrainColliders> colliders;
    };
    VesselPhysics* BindingOf(const Vessel& v) const;
    VesselPhysics* BindingOfBody(BodyId body, int& cluster) const;
    Vector3d PartRbVelocity(VesselPhysics& vp, Part& part) const;  // physics-frame velocity of the part's rb (centre of mass)
    UnityEngine::Collision MakeCollision(const PhysicsWorld::ContactEvent& e, bool mirrored, Part* thisColliderPart, Part* otherColliderPart,
                            VesselPhysics* vp1, VesselPhysics* vp2) const;
    void DispatchContacts();
    void DispatchPass(const std::vector<PhysicsWorld::ContactEvent>& events, PhysicsWorld::ContactEvent::Kind pass, size_t step,
                      const std::map<std::tuple<BodyId, uint32_t, BodyId, uint64_t>, size_t>& lastStay);
    void UpdateAnchors(float dt);
    PartBuoyancy::Env BuoyancyEnv();
    CollisionEnhancer::Env EnhancerEnv();
    void BuildTerrain(HostedTerrain& t);
    void OnRotatingFrameTransition(events::GameEvents::HostTargetAction<CelestialBody*, bool> data);
    void OnDominantBodyChange(events::GameEvents::FromToAction<CelestialBody*, CelestialBody*> data);

    PhysicsBubble* bubble_;
    bool installed_ = false;
    VesselEnvironment saved_;
    std::map<uint64_t, StaticColliderInfo> statics_;
    struct StaticRange {
        uint64_t base = 0, count = 0;
        StaticColliderInfo info;
    };
    std::vector<StaticRange> ranges_;
    StaticColliderInfo defaultInfo_;
    std::map<std::tuple<BodyId, uint32_t, BodyId, uint64_t>, PairMemory> pairs_;
    std::map<const Part*, std::pair<uint32_t, std::unique_ptr<PartBuoyancy>>> buoyancy_;   // keyed by part, with its flightID
    std::map<const Part*, std::pair<uint32_t, std::unique_ptr<CollisionEnhancer>>> enhancers_;
    std::map<const Vessel*, Anchor> anchors_;
    std::vector<HostedTerrain> terrain_;
    std::map<uint64_t, std::function<void()>> refreshHooks_;
    uint64_t nextHook_ = 1;
    std::vector<DispatchRecord> lastDispatch_;
    uint64_t enters_ = 0, exits_ = 0, stays_ = 0;
    size_t substepsDispatched_ = 0;
    Vector3d pendingShift_;
    size_t terrainRefreshes_ = 0;
};

}  // namespace ksp::physics
