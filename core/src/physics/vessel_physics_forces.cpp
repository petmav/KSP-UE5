// WP-20b: the force records of the ported part modules handed to a ForceAccumulator as the C# hands them to PhysX.
// See ksp/physics/vessel_physics.hpp. Positions and forces are the records' transform-frame floats.
#include "ksp/aero/flight_integrator_aero.hpp"
#include "ksp/aero/lifting_surface.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/modules/module_rcs.hpp"
#include "ksp/physics/vessel_physics.hpp"

namespace ksp {

// FlightIntegrator.cs:2243 ApplyAeroDrag / :2249 ApplyAeroLift (part.Rigidbody.AddForceAtPosition(force, position, mode)) and the
// rb.angularDrag assignments of UpdateAerodynamics (:2274-2281, :2350-2369), in UpdateAerodynamics' order: angular drag, drag, lift.
void physics::ApplyAeroForces(physics::ForceAccumulator& acc, Part& part, const AeroForces& f) {
    if (f.setAngularDrag) acc.SetAngularDrag(part, f.angularDrag);
    const physics::ForceMode mode = f.mode == AeroForceMode::Acceleration ? physics::ForceMode::Acceleration : physics::ForceMode::Force;
    if (f.applyDrag) acc.AddRigidbodyForceAtPosition(part, Vector3d(f.dragForce), Vector3d(f.dragPosition), mode);
    if (f.applyLift) acc.AddRigidbodyForceAtPosition(part, Vector3d(f.liftForce), Vector3d(f.liftPosition), mode);
}

// ModuleEngines.cs:1540 ThrustUpdate: part.AddForceAtPosition(-transform.forward * thrust, transform.position) per thrust transform,
// in the record's (transform) order
void physics::ApplyEngineForces(physics::ForceAccumulator& acc, Part& part, const EngineThrustResult& r) {
    for (const EngineForce& f : r.forces) acc.AddForceAtPosition(part, Vector3d(f.force), Vector3d(f.position));
}

// ModuleRCS.cs:277 FixedUpdate (:357 base.part.AddForceAtPosition(-vector * currentThrustForce, currentThruster.transform.position))
void physics::ApplyRcsForces(physics::ForceAccumulator& acc, Part& part, const std::vector<RCSForce>& forces) {
    for (const RCSForce& f : forces) acc.AddForceAtPosition(part, Vector3d(f.force), Vector3d(f.position));
}

// ModuleLiftingSurface.cs:170 FixedUpdate (:188-192: base.part.AddForceAtPosition(liftForce, CoL point), then the drag at the CoP point)
void physics::ApplySurfaceForces(physics::ForceAccumulator& acc, Part& part, const SurfaceForces& f) {
    if (!f.applied) return;
    acc.AddForceAtPosition(part, Vector3d(f.liftForce), Vector3d(f.liftPosition));
    if (f.applyDrag) acc.AddForceAtPosition(part, Vector3d(f.dragForce), Vector3d(f.dragPosition));
}

}  // namespace ksp
