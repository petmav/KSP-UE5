// WP-20a: the physics world. A thin owner of one Jolt PhysicsSystem (Jolt Physics v5.6.0, vendored in third_party/jolt, built
// with JPH_DOUBLE_PRECISION) and the rules KSP's PhysX scene had that do not depend on vessels:
//
//   * Replaces the PhysX scene Unity steps at FixedUpdate.PhysicsFixedUpdate (PlayerLoop::physicsStep, WP-00). Gravity is zero
//     in the world itself: KSP sets Physics.gravity = 0 and applies gravity as a per-vessel force (VesselPrecalculate,
//     Assembly-CSharp/VesselPrecalculate.cs:281-368), so the seam for it is AddForce / AddForceAtPosition.
//   * Two object layers, as in the spike (PHYSICS-SPIKE.md 5.2): Static (terrain quads, KSC, WP-12/WP-42) and Moving (parts and
//     vessel clusters, WP-20b). Static bodies only collide with Moving ones.
//   * Same-vessel collision suppression is a Jolt GroupFilter keyed on the vessel id: O(1) per candidate pair and nothing to
//     rebuild on a structural event. It replaces CollisionManager's O(C^2) Physics.IgnoreCollision rebuild
//     (Assembly-CSharp/CollisionManager.cs:126-168); sub-group flag 1 on BOTH bodies is Part.sameVesselCollision.
//   * Positions are doubles in the PHYSICS FRAME (the main body's body-fixed frame below the inverse-rotation threshold, its
//     inertial frame above: ksp/physics/reference_frame.hpp). The world never shifts; the Galilean frame velocity is a velocity
//     offset only (ksp/physics/krakensbane.hpp): velocities stored here are physics-frame velocities, i.e. the true velocity
//     minus Krakensbane::GetFrameVelocity().
//
// WP-20c added the static-contact record of the last step (StaticContacts) for the ground supports of the load solve.
// Not here (documented seams): vessel rigid bodies, clusters, joints and structural loads (WP-20b / WP-20c), ray / overlap
// queries and the static mesh interface AddStaticMesh(id, centre, vertices, triangles) of WP-12b, the contact listener that
// feeds Part.HandleCollision (WP-20d), EVA kerbals and wheels (WP-20e). CollisionEnhancer.cs (anti-tunnelling) is replaced by
// Jolt's motion quality (LinearCast) on those bodies.
//
// The header includes no Jolt header: the implementation is a pimpl, so consumers need no Jolt include paths or ISA flags.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"

namespace ksp {
class PlayerLoop;
}

namespace ksp::physics {

// Jolt body id (index | sequence number << 24). 0xFFFFFFFF is invalid.
using BodyId = uint32_t;
constexpr BodyId kInvalidBody = 0xFFFFFFFFu;

enum class MotionType : uint8_t { Static, Kinematic, Dynamic };

struct ShapeDesc {
    enum class Kind : uint8_t { Box, Sphere, Capsule, ConvexHull };
    Kind kind = Kind::Box;
    Vector3f halfExtents{0.5f, 0.5f, 0.5f};  // Box
    float radius = 0.5f;                     // Sphere, Capsule
    float halfHeight = 0.5f;                 // Capsule (half of the cylinder part, along local Y)
    // ConvexHull (WP-mucol: a Unity convex MeshCollider, the hull of the vertices; shared so ShapeDesc stays cheap to copy)
    std::shared_ptr<const std::vector<Vector3f>> hullPoints;
    static ShapeDesc ConvexHull(std::shared_ptr<const std::vector<Vector3f>> points) { ShapeDesc s; s.kind = Kind::ConvexHull; s.hullPoints = std::move(points); return s; }
    static ShapeDesc Box(Vector3f halfExtents_) { ShapeDesc s; s.kind = Kind::Box; s.halfExtents = halfExtents_; return s; }
    static ShapeDesc Sphere(float radius_) { ShapeDesc s; s.kind = Kind::Sphere; s.radius = radius_; return s; }
    static ShapeDesc Capsule(float halfHeight_, float radius_) { ShapeDesc s; s.kind = Kind::Capsule; s.halfHeight = halfHeight_; s.radius = radius_; return s; }
};

// WP-mucol: solid properties of the convex hull of a point set, as Jolt builds it (ConvexHullShape: its volume, centre of mass and
// inertia integrals; the hull tolerance is Jolt's default 1e-3 m). Inertia is about the centroid, per unit mass, in the point set's axes.
struct HullMassProps {
    bool valid = false;  // false: fewer than 4 non-coplanar points
    double volume = 0.0;
    Vector3d centroid;
    double inertia[3][3] = {};
};
HullMassProps HullMassProperties(const std::vector<Vector3f>& points);

struct BodyDesc {
    ShapeDesc shape;
    MotionType motion = MotionType::Dynamic;       // Static bodies go to the Static layer, the others to Moving
    Vector3d position;                             // physics frame, double
    QuaternionD rotation = QuaternionD::identity();
    Vector3d linearVelocity;                       // physics-frame velocity (true velocity - frame velocity)
    Vector3d angularVelocity;
    float mass = 1.0f;                             // Dynamic only; the inertia tensor is derived from the shape
    float friction = 0.5f;
    float restitution = 0.0f;
    float linearDamping = 0.0f;                    // KSP applies its own drag; Jolt's default 0.05/s is not wanted
    float angularDamping = 0.0f;
    // Jolt's default linear speed cap is 500 m/s; orbital speeds need it lifted (spike: 1e5). With a frame velocity the
    // physics-frame speed is small anyway. Angular: PhysicsGlobals.MaxAngularVelocity (Physics.cfg, 50 rad/s); callers with a
    // PhysicsGlobals instance pass PhysicsGlobals::MaxAngularVelocity().
    float maxLinearVelocity = 1.0e5f;
    float maxAngularVelocity = 50.0f;
    bool allowSleeping = false;                    // KSP never lets a part rigidbody sleep
    uint32_t vesselGroup = 0;                      // Vessel id for the same-vessel GroupFilter; 0 = ungrouped (collides with everything)
    bool sameVesselCollision = false;              // Part.sameVesselCollision: collide with the same vessel's bodies if both set
    uint64_t userData = 0;                         // opaque to the world (the part / quad id of the caller)
};

// WP-20b: a rigid body made of several collider shapes (one vessel cluster: the colliders of every part welded into it).
// The body's ORIGIN is a reference point the caller chooses (the cluster's centre of mass at creation, PHYSICS-SPIKE.md 5.3);
// children are placed relative to it in body space. Mass, inertia (about the centre of mass, body axes) and the centre of mass
// (body space, relative to the origin) are given explicitly: they are KSP's per-part sums, not the volumes of the proxy colliders.
struct CompoundChild {
    ShapeDesc shape;
    Vector3f position;                              // body space, relative to the body origin
    Quaternionf rotation = Quaternionf::identity();
    uint32_t userData = 0;                          // sub-shape user data (the part index of the caller)
};

struct CompoundBodyDesc {
    std::vector<CompoundChild> children;            // at least one
    MotionType motion = MotionType::Dynamic;
    Vector3d position;                              // physics-frame position of the body origin, double
    QuaternionD rotation = QuaternionD::identity();
    Vector3d linearVelocity;                        // physics-frame velocity of the centre of mass
    Vector3d angularVelocity;
    double mass = 1.0;
    double inertia[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};  // about the centre of mass, body axes (row-major, symmetric)
    Vector3f centerOfMass;                          // body space, relative to the origin (0: the origin is the centre of mass)
    float friction = 0.6f;
    float restitution = 0.0f;
    float maxLinearVelocity = 1.0e5f;
    float maxAngularVelocity = 50.0f;
    bool allowSleeping = false;
    uint32_t vesselGroup = 0;
    bool sameVesselCollision = false;
    uint64_t userData = 0;
    // Jolt's manifold reduction (merging the contact manifolds of different sub-shapes of a body pair that share a normal; on by default).
    // StaticContacts() reports every touching sub-shape either way.
    bool useManifoldReduction = true;
};

// Jolt constraint id (index into the world's constraint table); kInvalidConstraint when absent.
using ConstraintId = uint32_t;
constexpr ConstraintId kInvalidConstraint = 0xFFFFFFFFu;

struct PhysicsWorldConfig {
    // Jolt's default is 10 velocity steps; PhysX in KSP ran 50 position iterations / 1 velocity iteration ([runtime], PHYSICS-SPIKE.md).
    int velocitySteps = 10;
    int positionSteps = 2;
    int threads = 1;  // total threads including the calling one (the pool has threads - 1 workers); results do not depend on it
    unsigned maxBodies = 65536;
    unsigned maxBodyPairs = 65536;
    unsigned maxContacts = 65536;
    unsigned tempAllocatorBytes = 32u * 1024u * 1024u;
};

class PhysicsWorld {
public:
    explicit PhysicsWorld(const PhysicsWorldConfig& cfg = {});
    ~PhysicsWorld();
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    const PhysicsWorldConfig& Config() const;

    // ---- stepping -------------------------------------------------------------------------------------------------------
    // One simulation step of `dt` seconds (Jolt PhysicsSystem::Update with `collisionSteps` collision steps).
    void Step(float dt, int collisionSteps = 1);
    // Step(Time.fixedDeltaTime): what Unity's physics step did once per FixedUpdate (ksp::Time::fixedDeltaTime, WP-00/WP-08).
    void FixedStep();
    // Steps taken so far.
    uint64_t StepCount() const;
    // Hooks FixedStep into FixedUpdate.PhysicsFixedUpdate of `loop` (after Timing5; the number of steps per frame, bounded by
    // PHYSICS_FRAME_DT_LIMIT, is the loop's). Detach removes it; the destructor does not touch the loop, so detach first.
    void Attach(PlayerLoop& loop);
    void Detach(PlayerLoop& loop);

    // ---- bodies ---------------------------------------------------------------------------------------------------------
    BodyId CreateBody(const BodyDesc& desc);  // kInvalidBody when the body limit is reached
    void RemoveBody(BodyId id);               // removes and destroys; unknown ids are ignored
    size_t BodyCount() const;
    // Ids of every live body in creation order (the order all aggregate operations use: determinism).
    const std::vector<BodyId>& Bodies() const;
    bool IsValid(BodyId id) const;
    uint64_t UserData(BodyId id) const;

    Vector3d GetPosition(BodyId id) const;  // body origin == centre of mass for the shapes of this API, double
    QuaternionD GetRotation(BodyId id) const;
    Vector3d GetLinearVelocity(BodyId id) const;   // physics-frame velocity (float precision inside Jolt)
    Vector3d GetAngularVelocity(BodyId id) const;
    void SetPosition(BodyId id, const Vector3d& position);
    void SetLinearVelocity(BodyId id, const Vector3d& v);
    void SetAngularVelocity(BodyId id, const Vector3d& w);
    float GetMass(BodyId id) const;  // 0 for non-dynamic bodies

    // ---- forces: accumulated until the next Step, then cleared by Jolt (Rigidbody.AddForce(ForceMode.Force) semantics) --------
    void AddForce(BodyId id, const Vector3d& force);
    void AddForceAtPosition(BodyId id, const Vector3d& force, const Vector3d& point);  // point: physics frame, double
    void AddTorque(BodyId id, const Vector3d& torque);
    void AddImpulse(BodyId id, const Vector3d& impulse);  // ForceMode.Impulse
    // ForceMode.VelocityChange on every non-static body, in creation order: the whole of Krakensbane.AddExcess / Zero's velocity
    // work for bodies owned by the world ("one velocity write per body", Krakensbane.cs:121-173). Returns the bodies touched.
    size_t AddVelocityChangeToAllMoving(const Vector3d& dv);

    // ---- static geometry (WP-12b terrain colliders, WP-42 KSC / PQSCity objects, WP-20b's pad terrain patch) ----------------
    // Static bodies live in the Static layer (they only collide with Moving ones) and are addressed by a caller-chosen 64-bit
    // key (a quad collider, a city object ...); adding a key that exists replaces the body (the MeshCollider.sharedMesh
    // re-assignment of an OnQuadBuilt on an already enabled collider). The key is also the body's user data. Positions are
    // doubles in the physics frame; mesh vertices are floats relative to `centre`, the layout Jolt's MeshShape keeps
    // (PHYSICS-SPIKE.md 5.2 AddStaticMesh(id, centre, vertices, triangles)). `triangles` are index triples into `vertices`,
    // wound as the Unity mesh (cacheIndices): the numeric normal cross(b - a, c - a) points to the front face in both Unity and
    // Jolt (counter-clockwise seen from the front), so no flip is made. RemoveBody removes a static body too (and its key).
    // Returns kInvalidBody when the shape cannot be cooked (no valid triangle, logged) or the body limit is reached.
    BodyId AddStaticMesh(uint64_t key, const Vector3d& centre, const std::vector<Vector3f>& vertices, const std::vector<int>& triangles,
                         float friction = 0.6f, float restitution = 0.0f);
    // A primitive static collider (BoxCollider / SphereCollider / CapsuleCollider of a scenery object).
    BodyId AddStaticShape(uint64_t key, const ShapeDesc& shape, const Vector3d& position, const QuaternionD& rotation,
                          float friction = 0.6f, float restitution = 0.0f);
    bool RemoveStatic(uint64_t key);              // false when the key is unknown
    BodyId StaticBody(uint64_t key) const;        // kInvalidBody when unknown
    size_t StaticCount() const;
    std::vector<uint64_t> StaticKeys() const;     // every static collider's key, ascending
    // Rebalances the broad phase after a batch of insertions (Jolt keeps newly added bodies in an unbalanced part of the tree
    // until then); no effect on results, only on query / step speed.
    void OptimizeBroadPhase();

    // ---- queries (WP-12b; the general RayCast / LineCast / OverlapSphere users are WP-20d and the vessel ground height) ---------
    // Layer bits for the `layers` argument (the two object layers of this world).
    static constexpr uint32_t kLayerStatic = 1u, kLayerMoving = 2u, kLayerAll = 3u;
    struct RayHit {
        bool hit = false;
        BodyId body = kInvalidBody;
        uint64_t userData = 0;     // BodyDesc::userData of the body hit (static colliders: their key)
        double distance = 0.0;     // from the origin along the (normalised) direction
        Vector3d point;            // hit point (physics frame, double)
        Vector3d normal;           // unit surface normal on the side of the ray origin
    };
    // Closest hit of origin + t * direction, 0 <= t <= maxDistance (direction is normalised; both sides of a triangle are hit,
    // so a ray started under the terrain finds it too). Precision: Jolt returns a float fraction of maxDistance, so the point
    // is good to about 6e-8 * maxDistance.
    RayHit RayCast(const Vector3d& origin, const Vector3d& direction, double maxDistance, uint32_t layers = kLayerAll) const;
    // Every body whose shape overlaps the sphere, ascending by body id.
    size_t OverlapSphere(const Vector3d& centre, double radius, std::vector<BodyId>& out, uint32_t layers = kLayerAll) const;

    // ---- WP-20b: compound (cluster) bodies -----------------------------------------------------------------------------------
    // A compound body (see CompoundBodyDesc); kInvalidBody when the limit is reached or the description is invalid.
    BodyId CreateCompoundBody(const CompoundBodyDesc& desc);
    // Physics-frame centre of mass (Jolt keeps it in double: Body::GetCenterOfMassPosition).
    Vector3d GetCenterOfMassPosition(BodyId id) const;
    // New mass properties of a compound body: mass, inertia about the centre of mass (body axes) and the centre of mass in body
    // space. The body origin keeps its place (Jolt moves its centre of mass, Body::SetShapeInternal); the velocities are left as
    // they are (the caller adds omega x shift when the centre of mass moves).
    void SetMassProperties(BodyId id, double mass, const double inertia[3][3], const Vector3f& centerOfMass);
    // Jolt's own angular damping of a body (w *= max(0, 1 - c dt) each step, after the forces): the spike's cluster damping model
    void SetAngularDamping(BodyId id, float damping);
    // A rigid weld between two bodies at a physics-frame point (Jolt FixedConstraint): the inter-cluster joint of the reduced
    // model (decouplers in the split clustering, robotics / flexible joints until WP-20c / WP-52 give them their own constraints).
    ConstraintId CreateFixedConstraint(BodyId a, BodyId b, const Vector3d& point);
    void RemoveConstraint(ConstraintId id);   // unknown ids are ignored
    size_t ConstraintCount() const;
    // Total impulse of the last step on a fixed constraint: position (linear, kN s) and rotation (angular, kN m s) parts,
    // applied to body b (and the opposite to a). Zero for unknown ids. (The input of WP-20c's load solve between clusters.)
    void GetConstraintImpulse(ConstraintId id, Vector3d& linear, Vector3d& angular) const;

    // ---- WP-20c: contacts with static geometry (the ground supports of the structural load solve) ----------------------------
    // With tracking on, a Jolt contact listener records every (dynamic body, sub-shape user data) that touched (within 1 mm) a
    // Static-layer body during the last Step, from the per-sub-shape validation callback (so manifold reduction hides none); StaticContacts()
    // lists them sorted by (body id, user data), without duplicates: the same set whatever the thread count. Off by default (one mutex per
    // contact callback). The full contact events (Part.OnCollisionEnter / HandleCollision) are WP-20e's.
    struct StaticContact {
        BodyId body = kInvalidBody;
        uint32_t subShapeUserData = 0;
        bool operator==(const StaticContact& o) const { return body == o.body && subShapeUserData == o.subShapeUserData; }
        bool operator<(const StaticContact& o) const { return body != o.body ? body < o.body : subShapeUserData < o.subShapeUserData; }
    };
    void SetStaticContactTracking(bool on);
    bool StaticContactTracking() const;
    const std::vector<StaticContact>& StaticContacts() const;
    // ---- WP-19b: frames of the physics bubble -----------------------------------------------------------------------------
    bool IsStatic(BodyId id) const;  // false for unknown ids
    // Moves every body of the chosen kinds by `delta` (double, in creation order; SetPosition, no activation): the fold of
    // Krakensbane's frame drift into the dynamic bodies, the rare re-centring of the physics origin. Returns the bodies moved.
    size_t TranslateBodies(const Vector3d& delta, bool moving, bool statics);
    // Static bodies collide with moving ones only while this is on (default). The physics bubble switches it off while the moving
    // bodies are in a Galilean frame that has drifted away from the body frame the static geometry lives in (FrameOffset != 0).
    void SetStaticCollisions(bool enabled);
    bool StaticCollisions() const;

    // ---- determinism ----------------------------------------------------------------------------------------------------
    // FNV-1a 64 over the raw bits of every body's position, rotation and both velocities, in creation order.
    uint64_t StateHash() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<void> loopHook_;  // the Delegate registered by Attach (type-erased: this header and physics_world.cpp are RTTI-free)
};

}  // namespace ksp::physics
