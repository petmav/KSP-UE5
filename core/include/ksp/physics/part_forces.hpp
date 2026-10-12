// WP-20d: the part force buffers and the part-module FixedUpdate slot of the flight tick.
//
// KSP's per-part force producers never touch a Rigidbody: they write the three buffers of the part that carries the Rigidbody
// (Part.AddForce / AddImpulse / AddForceAtPosition / AddTorque, Part.cs:8663-8688, into RigidBodyPart.force / .forces / .torque), and
// FlightIntegrator.Integrate (FlightIntegrator.cs:715-745) applies and clears them in the same tick (PHYSICS-ENGINE-REVIEW.md 1.3:
// the module FixedUpdates run before FlightIntegrator.FixedUpdate in the measured script order, seq 22-276 vs 278).
//
// The modules ported in WP-21 / WP-23 / WP-24 return their forces as records instead of calling Part.AddForceAtPosition / AddTorque
// (EngineThrustResult, RCSForce, SurfaceForces, ModuleReactionWheel::lastTorque). PartModuleForces is the slot that runs those
// modules' FixedUpdate with the Unity state they read (built from the rigid bodies of the WP-20b binding) and hands every record to
// the part buffers exactly as the C# would have:
//   ModuleEngines / ModuleEnginesFX  FixedUpdate (ModuleEngines.cs:1587: UpdateThrottle, UpdatePropellantStatus, ThrustUpdate :1540):
//                     part.AddForceAtPosition(-transform.forward * finalThrust * multiplier, transform.position) per thrust transform,
//                     part.AddThermalFlux; ModuleEnginesFX.FXUpdate (the spool value). The thrust transforms follow the part every tick
//                     from their PART-SPACE pose (`Hooks::engineThrustTransforms`: the model reader; the WP-17b data in tests).
//   ModuleRCS         FixedUpdate (ModuleRCS.cs:277; thrusters from `Hooks::rcsThrusters`, part space) -> part.AddForceAtPosition
//                     (:357); the per-frame input mapping ModuleRCS.Update (:211) runs in Update().
//   ModuleLiftingSurface / ModuleControlSurface / ModuleAeroSurface  FixedUpdate (ModuleLiftingSurface.cs:170 / ModuleControlSurface.cs:366)
//                     with baseTransform = the part transform (no model transforms) -> part.AddForceAtPosition lift, then drag.
//   ModuleReactionWheel  FixedUpdate (ModuleReactionWheel.cs:158) -> part.AddTorque(lastTorque).
// Not run here: Part.FixedUpdate itself (temperature / g / pressure limits, crew, ModulesOnFixedUpdate of ACTIVE parts) and every
// module without a force record (they belong to their packages); ModuleGimbal actuation (the thrust transforms stay at their
// neutral pose); exhaust damage raycasts.
#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "ksp/math/vector3d.hpp"
#include "ksp/physics/vessel_physics.hpp"

namespace ksp {
class Part;
class Vessel;
class ModuleEngines;
class ModuleRCS;
struct PartTransform;
struct RCSThruster;
}  // namespace ksp

namespace ksp::physics {

// ---- Part.cs:8663-8688 -------------------------------------------------------------------------------------------------------
void PartAddForce(Part& part, const Vector3d& vec);                                // :8663 RigidBodyPart.force += vec
void PartAddImpulse(Part& part, const Vector3d& vec);                              // :8671 force += vec / TimeWarp.fixedDeltaTime
void PartAddForceAtPosition(Part& part, const Vector3d& vec, const Vector3d& pos); // :8679 RigidBodyPart.forces.Add
void PartAddTorque(Part& part, const Vector3d& vec);                               // :8684 RigidBodyPart.torque += vec

// A ForceAccumulator over the part buffers: the Part.AddForce* calls buffer (applied by FlightIntegrator.Integrate), the
// Rigidbody.AddForce* calls, angular drag and the uniform acceleration go straight to the binding (they are Rigidbody calls in the C#).
class PartBufferForces final : public ForceAccumulator {
public:
    explicit PartBufferForces(VesselPhysics* binding = nullptr) : binding_(binding) {}
    void AddForce(Part& part, const Vector3d& force) override { PartAddForce(part, force); }
    void AddImpulse(Part& part, const Vector3d& impulse) override { PartAddImpulse(part, impulse); }
    void AddForceAtPosition(Part& part, const Vector3d& force, const Vector3d& position) override { PartAddForceAtPosition(part, force, position); }
    void AddTorque(Part& part, const Vector3d& torque) override { PartAddTorque(part, torque); }
    void AddRigidbodyForce(Part& part, const Vector3d& force, ForceMode mode) override;
    void AddRigidbodyForceAtPosition(Part& part, const Vector3d& force, const Vector3d& position, ForceMode mode) override;
    void SetAngularDrag(Part& part, float angularDrag) override;
    void AddUniformAcceleration(const Vector3d& acceleration) override;

private:
    VesselPhysics* binding_;
};

// The Unity transform of a child of the part (a model transform) placed on the part's current transform: position =
// partTransform.TransformPoint(local.position), rotation = partTransform.rotation * local.rotation (native quaternion product).
PartTransform ChildTransform(const PartTransform& part, const PartTransform& local);

class PartModuleForces {
public:
    struct Hooks {
        // Part-space thrust transforms of an engine module (position / rotation relative to the part, FindModelTransforms order).
        // Unset or empty: the module's thrustTransforms are left as they are (they then do not follow the part).
        std::function<std::vector<PartTransform>(Part&, const ModuleEngines&)> engineThrustTransforms;
        // Part-space RCS thrusters (position, up, forward relative to the part). Unset: the module's thrusterTransforms are left alone.
        std::function<std::vector<RCSThruster>(Part&, const ModuleRCS&)> rcsThrusters;
        // FlightInputHandler.fetch.precisionMode (ModuleRCS)
        bool precisionMode = false;
    };
    Hooks hooks;
    bool engines = true, rcs = true, liftingSurfaces = true, reactionWheels = true;

    // seq 22-276: every loaded vessel of the registry (FlightGlobals.Vessels order), its parts in vessel order, each part's modules in
    // module order. `host` gives the vessels' bindings (a vessel without one runs its modules' packed / no-Rigidbody branches).
    void FixedUpdate(VesselPhysicsHost& host);
    void FixedUpdateVessel(Vessel& vessel, VesselPhysics* binding);
    // the frame-rate half (ModuleRCS.Update: the control input mapping)
    void Update();

    // what the last FixedUpdate handed to the part buffers (counts)
    size_t engineForces = 0, rcsForces = 0, surfaceForces = 0, wheelTorques = 0;

private:
    void EngineFixedUpdate(Part& part, ModuleEngines& e, Vessel& v, VesselPhysics* vp);
    void RcsFixedUpdate(Part& part, ModuleRCS& r, Vessel& v, VesselPhysics* vp);
};

}  // namespace ksp::physics
