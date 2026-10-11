// WP-20a: the physics frame model; see ksp/physics/reference_frame.hpp.
#include "ksp/physics/reference_frame.hpp"

#include "ksp/events/game_events.hpp"

namespace ksp::ReferenceFrame {

// OrbitPhysicsManager.cs:322-343 (the body switch `setDominantBody` in :324-327 is WP-19b)
int RotatingFrameTransition(const CelestialBody& body, double altitude) {
    if (!body.rotates) return 0;
    if (altitude < static_cast<double>(body.inverseRotThresholdAltitude)) {
        if (!body.inverseRotation) return +1;
    } else if (altitude > static_cast<double>(body.inverseRotThresholdAltitude) && body.inverseRotation) {
        return -1;
    }
    return 0;
}

// OrbitPhysicsManager.cs:234-304
void SetRotatingFrame(CelestialBody& body, bool rotatingFrameState, physics::PhysicsWorld& world,
                      const std::vector<FrameRebaseGroup>& groups, const std::function<void(float)>& onIgnoreGeeForces) {
    if (onIgnoreGeeForces) onIgnoreGeeForces(0.5f);  // FlightLogger.IgnoreGeeForces(0.5f)
    body.inverseRotation = rotatingFrameState;
    for (const FrameRebaseGroup& g : groups) {
        Vector3d rFrmVel = body.getRFrmVel(g.position);
        for (physics::BodyId id : g.bodies) {
            if (!world.IsValid(id)) continue;
            // rb.velocity = (Vector3d)rb.velocity -/+ rFrmVel (the world holds the physics-frame velocity)
            if (rotatingFrameState) world.SetLinearVelocity(id, world.GetLinearVelocity(id) - rFrmVel);
            else world.SetLinearVelocity(id, world.GetLinearVelocity(id) + rFrmVel);
        }
        if (g.krakensbaneAcc != nullptr) {
            if (rotatingFrameState) *g.krakensbaneAcc = *g.krakensbaneAcc - rFrmVel;
            else *g.krakensbaneAcc = *g.krakensbaneAcc + rFrmVel;
        }
    }
    GameEvents::onRotatingFrameTransition.Fire(GameEvents::HostTargetAction<CelestialBody*, bool>(&body, rotatingFrameState));
}

int CheckReferenceFrame(CelestialBody& body, double altitude, physics::PhysicsWorld& world,
                        const std::vector<FrameRebaseGroup>& groups, const std::function<void(float)>& onIgnoreGeeForces) {
    int t = RotatingFrameTransition(body, altitude);
    if (t != 0) SetRotatingFrame(body, t > 0, world, groups, onIgnoreGeeForces);
    return t;
}

// FlightGlobals.cs:1356
Vector3d getCentrifugalAcc(const Vector3d& pos, const CelestialBody& body) {
    if (!body.inverseRotation) return Vector3d::zero();
    Vector3d p = body.position - pos;
    return Vector3d::Cross(body.angularVelocity, Vector3d::Cross(body.angularVelocity, p));
}

// FlightGlobals.cs:1366
Vector3d getCoriolisAcc(const Vector3d& vel, const CelestialBody& body) {
    if (!body.inverseRotation) return Vector3d::zero();
    return -2.0 * Vector3d::Cross(body.angularVelocity, vel);
}

}  // namespace ksp::ReferenceFrame
