// WP-19b: physicalObject (Assembly-CSharp/physicalObject.cs, 98 lines): a free rigid body of the flight scene that is not a part
// (debris pieces, ejected fairing / shroud meshes: physicalObject.ConvertToPhysicalObject). FlightGlobals.physicalObjects is the
// list Krakensbane (AddExcess / Zero), OrbitPhysicsManager (setRotatingFrame / setDominantBody), FloatingOrigin and
// FlightIntegrator.IntegratePhysicalObjects walk. Here the Rigidbody is a body of the bubble's PhysicsWorld; its Transform position is
// the physics bubble's WorldFromPhysics of the body position (ksp/physics/physics_bubble.hpp).
//
// Ported: the fields, Awake (FlightGlobals.addPhysicalObject), OnDestroy (removePhysicalObject; the body goes with the GameObject),
// the Update decision (destroyed at high time warp or farther than maxDistance from the active vessel). Not ported: the 0.05 s
// collider delay of Start (rb.detectCollisions false for colliderDelay), renderer / material cleanup (the shell's).
#pragma once

#include "ksp/math/vector3d.hpp"
#include "ksp/physics/physics_world.hpp"

namespace ksp {

class physicalObject {
public:
    float maxDistance = 1000.0f;  // physicalObject.cs:7
    float origDrag = 1.0f;        // :9
    physics::PhysicsWorld* world = nullptr;
    physics::BodyId rb = physics::kInvalidBody;  // :11 (Rigidbody)

    // Awake (physicalObject.cs:17): registers with FlightGlobals. `body` is owned from now on (removed by the destructor).
    physicalObject(physics::PhysicsWorld& w, physics::BodyId body);
    // OnDestroy (physicalObject.cs:42): unregisters and removes the body (UnityEngine.Object.Destroy of the GameObject)
    ~physicalObject();
    physicalObject(const physicalObject&) = delete;
    physicalObject& operator=(const physicalObject&) = delete;

    bool HasRigidbody() const { return world != nullptr && world->IsValid(rb); }
    // physicalObject.cs:48 Update, the decision only: true when the object destroys itself (TimeWarp.CurrentRate > 1 in HIGH mode, or
    // Vector3.Distance(transform.position, ActiveVessel.transform.position) > maxDistance; float positions, Mono rule).
    // `hasActiveVessel` = FlightGlobals.ActiveVessel != null.
    bool UpdateWantsDestroy(const Vector3f& position, bool hasActiveVessel, const Vector3f& activeVesselPosition) const;
};

}  // namespace ksp
