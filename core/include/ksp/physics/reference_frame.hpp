// WP-20a: the physics frame model (PHYSICS-SPIKE.md 5.1).
//
//   * Positions: doubles in the main body's frame. Below CelestialBody.inverseRotThresholdAltitude the frame is BODY-FIXED
//     (`inverseRotation == true`: KSP's "rotating frame", in which the planet does not turn and the surface is static, so terrain
//     colliders never move); above it the frame is the body's INERTIAL frame. KSP's own switch decides, unchanged:
//     OrbitPhysicsManager.checkReferenceFrame (OrbitPhysicsManager.cs:322-343) and setRotatingFrame (:314, :234-304).
//   * No per-tick shift of anything (floating_origin.hpp). Krakensbane's frame velocity is a Galilean velocity offset only
//     (krakensbane.hpp): the physics-frame velocity of a body is its true velocity minus Krakensbane::GetFrameVelocity(),
//     and its true position is the physics-frame position plus FloatingOrigin::FrameOffset().
//   * In the body-fixed frame the fictitious accelerations are applied per vessel as KSP does:
//     FlightGlobals.getCentrifugalAcc / getCoriolisAcc (FlightGlobals.cs:1356-1373), used by VesselPrecalculate (WP-19a).
//
// This header is the WP-20a half of "frame switch": the decision, the velocity re-expression of all bodies at the switch
// (+-omega x r, as KSP does), the fictitious accelerations and the physics-frame <-> true conversions. The rest of
// OrbitPhysicsManager (setDominantBody on SOI change, vessel pack / unpack, HoldVesselUnpack) is WP-19a / WP-19b; with vessel rigid bodies
// the physics bubble (ksp/physics/physics_bubble.hpp) ports setRotatingFrame itself (velocity changes through the VesselPhysics bindings).
#pragma once

#include <functional>
#include <vector>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/physics/floating_origin.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "ksp/physics/physics_world.hpp"

namespace ksp {

// The bodies that move together with one position, as OrbitPhysicsManager.setRotatingFrame sees them: a vessel (all its part
// rigidbodies share `position` = vessel.transform.position, so they all get the same +-omega x r) or one free physical object.
struct FrameRebaseGroup {
    Vector3d position;                       // vessel.transform.position / physicalObject.transform.position (body.position frame)
    std::vector<physics::BodyId> bodies;     // part rigidbodies (and servo rigidbodies) of the group
    Vector3d* krakensbaneAcc = nullptr;      // Vessel.krakensbaneAcc of a vessel (-= / += rFrmVel); null for physical objects
};

namespace ReferenceFrame {

// OrbitPhysicsManager.cs:322 checkReferenceFrame, the decision part: the rotating-frame state `body` should be in at
// `altitude` (FlightGlobals.getAltitudeAtPos of the active vessel above `body`), or no change. `altitude` and the float threshold
// are compared in double like the C# (`num < (double)inverseRotThresholdAltitude`). Bodies that do not rotate never change.
// Returns +1 = switch to rotating (body-fixed), -1 = switch to inertial, 0 = stay.
int RotatingFrameTransition(const CelestialBody& body, double altitude);

// OrbitPhysicsManager.cs:314 / :234 setRotatingFrame: body.inverseRotation = rotating; every group's bodies get
// velocity -= rFrmVel (entering the rotating frame) or += rFrmVel (leaving it), rFrmVel = body.getRFrmVel(group.position);
// vessels' krakensbaneAcc likewise; onRotatingFrameTransition fires. `onIgnoreGeeForces` is FlightLogger.IgnoreGeeForces(0.5f).
// The velocity write is one SetLinearVelocity per body (O(bodies in the bubble), PHYSICS-SPIKE.md 5.1 "rebase").
void SetRotatingFrame(CelestialBody& body, bool rotating, physics::PhysicsWorld& world, const std::vector<FrameRebaseGroup>& groups,
                      const std::function<void(float)>& onIgnoreGeeForces = {});

// checkReferenceFrame with its effect: RotatingFrameTransition then SetRotatingFrame. Returns the transition (+1 / -1 / 0).
int CheckReferenceFrame(CelestialBody& body, double altitude, physics::PhysicsWorld& world, const std::vector<FrameRebaseGroup>& groups,
                        const std::function<void(float)>& onIgnoreGeeForces = {});

// FlightGlobals.cs:1356 getCentrifugalAcc(pos, body): zero unless the body is in the rotating frame; omega x (omega x (body.position - pos)).
Vector3d getCentrifugalAcc(const Vector3d& pos, const CelestialBody& body);
// FlightGlobals.cs:1366 getCoriolisAcc(vel, body): zero unless rotating; -2 omega x vel.
Vector3d getCoriolisAcc(const Vector3d& vel, const CelestialBody& body);

// Physics-frame <-> true (Galilean frame removed) conversions, kspcore's reading of
// `velocityD = rb_velocityD + Krakensbane.GetFrameVelocity()` (Vessel.cs UpdatePosVel) and of the shift the FloatingOrigin used to apply.
inline Vector3d TrueVelocity(const Vector3d& physicsVelocity) { return physicsVelocity + Krakensbane::GetFrameVelocity(); }
inline Vector3d PhysicsVelocity(const Vector3d& trueVelocity) { return trueVelocity - Krakensbane::GetFrameVelocity(); }
inline Vector3d TruePosition(const Vector3d& physicsPosition) {
    const FloatingOrigin* fo = FloatingOrigin::Instance();
    return fo ? physicsPosition + fo->FrameOffset() : physicsPosition;
}
inline Vector3d PhysicsPosition(const Vector3d& truePosition) {
    const FloatingOrigin* fo = FloatingOrigin::Instance();
    return fo ? truePosition - fo->FrameOffset() : truePosition;
}

}  // namespace ReferenceFrame
}  // namespace ksp
