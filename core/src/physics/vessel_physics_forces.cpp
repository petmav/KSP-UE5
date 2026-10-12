// WP-20b: the force records of the ported part modules handed to a ForceAccumulator as the C# hands them to PhysX.
// See ksp/physics/vessel_physics.hpp. Positions and forces are the records' transform-frame floats.
#include "ksp/aero/flight_integrator_aero.hpp"
#include "ksp/aero/lifting_surface.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/modules/module_rcs.hpp"
#include "ksp/physics/vessel_physics.hpp"

namespace ksp::physics {

// FlightIntegrator.cs:2243-2258 (ApplyAeroDrag / ApplyAeroLift: part.Rigidbody.AddForceAtPosition(force, position, mode)) and the
// rb.angularDrag assignments of UpdateAerodynamics (:2274-2281, :2350-2369), in UpdateAerodynamics' order: angular drag, drag, lift.
void ApplyAeroForces(ForceAccumulator& acc, Part& part, const AeroForces& f) {
    if (f.setAngularDrag) acc.SetAngularDrag(part, f.angularDrag);
    const ForceMode mode = f.mode == AeroForceMode::Acceleration ? ForceMode::Acceleration : ForceMode::Force;
    if (f.applyDrag) acc.AddRigidbodyForceAtPosition(part, Vector3d(f.dragForce), Vector3d(f.dragPosition), mode);
    if (f.applyLift) acc.AddRigidbodyForceAtPosition(part, Vector3d(f.liftForce), Vector3d(f.liftPosition), mode);
}

// ModuleEngines.ThrustUpdate (ModuleEngines.cs:1540): part.AddForceAtPosition(-transform.forward * thrust, transform.position) per
// thrust transform, in the record's (transform) order
void ApplyEngineForces(ForceAccumulator& acc, Part& part, const EngineThrustResult& r) {
    for (const EngineForce& f : r.forces) acc.AddForceAtPosition(part, Vector3d(f.force), Vector3d(f.position));
}

// ModuleRCS.cs:357: base.part.AddForceAtPosition(-vector * currentThrustForce, currentThruster.transform.position)
void ApplyRcsForces(ForceAccumulator& acc, Part& part, const std::vector<RCSForce>& forces) {
    for (const RCSForce& f : forces) acc.AddForceAtPosition(part, Vector3d(f.force), Vector3d(f.position));
}

// ModuleLiftingSurface.cs:188-192: base.part.AddForceAtPosition(liftForce, CoL point), then the drag force at the CoP point
void ApplySurfaceForces(ForceAccumulator& acc, Part& part, const SurfaceForces& f) {
    if (!f.applied) return;
    acc.AddForceAtPosition(part, Vector3d(f.liftForce), Vector3d(f.liftPosition));
    if (f.applyDrag) acc.AddForceAtPosition(part, Vector3d(f.dragForce), Vector3d(f.dragPosition));
}

}  // namespace ksp::physics
