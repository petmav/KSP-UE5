// WP-21: the aerodynamic members of FlightIntegrator (FlightIntegrator.cs) as pure functions on the part data model, for
// the physics tick (WP-20) and the thermal model (WP-22) to call. The atmosphere state (pressure, density, Mach, q,
// pseudo-Reynolds) is WP-09's FlightIntegratorAtmosphere (ksp/celestial/flight_atmosphere.hpp): not duplicated here.
//
// Ported (FlightIntegrator.cs):
//   :544-588   FixedUpdate's cache block, aero subset                  -> FlightIntegratorAero::CacheGlobals
//   :2243-2247 ApplyAeroDrag (force, position, mode: returned, not applied)  -> AeroForces::drag*
//   :2249-2258 ApplyAeroLift (bodyLiftLocalPosition / Vector set on the part; force returned)  -> AeroForces::lift*
//   :2260-2389 UpdateAerodynamics (every statement; the Rigidbody is the AeroRigidbody input, its AddForceAtPosition
//              calls are the returned AeroForces, rb.angularDrag is AeroForces::angularDrag)
//   :2391-2510 CalculateDragValue (+ _Spherical, _Cylindrical, _Conic, _Cube), CalculateAerodynamicArea,
//              CalculateAreaExposed, CalculateAreaRadiative
//   :2512-2543 DragCubeSetupAndPartAeroStats (altitude of a part's position through a hook: FlightGlobals.getAltitudeAtPos)
// Not ported here (and who owns it): :1-543 fields / Setup / events / UpdateMassStats / SmoothVelocity and :589-713
// the rest of FixedUpdate / Update / ThermoPrecalculate (the physics tick: WP-20, with the drag-cube root-part reload of
// Setup :281-348 left to it); :715-783 Integrate / IntegratePhysicalObjects (rigidbody forces, Krakensbane: WP-20);
// :784-985 atmosphere helpers (WP-09, flight_atmosphere.hpp); :974-2242 thermal model (WP-22); :2545-2770 thermal
// occlusion passes UpdateOcclusion* (WP-22; their geometry, OcclusionData/Cone/Cylinder, is ported in occlusion.hpp);
// :2771 OnDrawGizmosSelected.
#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include "ksp/aero/float_eval.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class CelestialBody;
class Part;

namespace aero {

// PhysicsGlobals.DragCurveValue under policy P (PhysicsGlobals.cs:2692); MonoFloat equals PhysicsGlobals::DragCurveValue.
template <class P>
float DragCurveValue(const PhysicsGlobals::SurfaceCurvesList& curves, float dotNormalized, float mach);

}  // namespace aero

// UnityEngine.ForceMode values FlightIntegrator uses
enum class AeroForceMode : int32_t { Force = 0, Acceleration = 5 };

// The rigidbody UpdateAerodynamics reads (part.Rigidbody: the part's own rb, or the parent's for a physicsless part).
struct AeroRigidbody {
    bool present = false;              // part.Rigidbody != null
    bool isPartRb = true;              // part.Rigidbody == part.rb (false: a physicsless part using its parent's body)
    bool partHasRb = true;             // part.rb != null
    bool partHasServoRb = false;       // part.servoRb != null
    Vector3f velocity{};               // rigidbody.velocity
    Vector3f worldCenterOfMass{};      // rigidbody.worldCenterOfMass
    Vector3d krakensbaneFrameVelocity{};  // Krakensbane.GetFrameVelocity()
};

// What UpdateAerodynamics hands to the physics engine (instead of calling Rigidbody.AddForceAtPosition).
struct AeroForces {
    bool applyDrag = false;
    Vector3f dragForce{};     // -part.dragVectorDir * part.dragScalar
    Vector3f dragPosition{};  // TransformPoint(CoPOffset), or rb.worldCenterOfMass (ApplyDragToNonPhysicsPartsAtParentCoM)
    bool applyLift = false;
    Vector3f liftForce{};
    Vector3f liftPosition{};
    AeroForceMode mode = AeroForceMode::Force;
    bool setAngularDrag = false;  // rb.angularDrag (and servoRb.angularDrag) was assigned
    float angularDrag = 0.0f;
};

// FlightIntegrator's aero state: the PhysicsGlobals caches and the atmosphere values UpdateAerodynamics reads.
struct FlightIntegratorAero {
    // FixedUpdate cache block (FlightIntegrator.cs:565-585), aero subset
    float cacheDragCubeMultiplier = 0.0f;
    bool cacheDragCubesUseSpherical = false;
    bool cacheApplyDrag = false;
    bool cacheApplyDragToNonPhysicsParts = false;
    bool cacheDragUsesAcceleration = false;
    float cacheDragMultiplier = 0.0f;
    double cacheBuoyancyWaterAngularDragScalar = 0.0;
    float cacheAngularDragMultiplier = 0.0f;
    float cacheBodyLiftMultiplier = 0.0f;
    // FlightIntegrator fields read by the aero members
    const CelestialBody* currentMainBody = nullptr;
    double mach = 0.0;
    double density = 0.0;
    double pseudoReDragMult = 1.0;

    void CacheGlobals();  // FlightIntegrator.cs:565-585 (reads PhysicsGlobals)

    // FlightIntegrator.cs:2391 CalculateDragValue and the per-model helpers (:2414-2432)
    template <class P = aero::MonoFloat>
    double CalculateDragValue(const Part& part) const;
    double CalculateDragValue_Spherical(const Part& part) const;  // :2414
    template <class P = aero::MonoFloat>
    double CalculateDragValue_Cylindrical(const Part& part) const;  // :2419
    template <class P = aero::MonoFloat>
    double CalculateDragValue_Conic(const Part& part) const;  // :2424
    template <class P = aero::MonoFloat>
    double CalculateDragValue_Cube(const Part& part) const;  // :2429
    double CalculateAerodynamicArea(const Part& part) const;  // :2434
    double CalculateAreaExposed(const Part& part) const;      // :2460
    double CalculateAreaRadiative(const Part& part) const;    // :2486

    // FlightIntegrator.cs:2260 UpdateAerodynamics. `rb` is part.Rigidbody; the part's aero fields are written as in C#.
    template <class P = aero::MonoFloat>
    AeroForces UpdateAerodynamics(Part& part, const AeroRigidbody& rb) const;

    // The force-scalar statements of UpdateAerodynamics on their own (FlightIntegrator.cs:2359-2375) for callers that
    // already hold the part's dynamic pressure (the gates; WP-20 uses UpdateAerodynamics): num2 = 1 - submerged share.
    template <class P = aero::MonoFloat>
    float DragScalar(const Part& part, double dynamicPressurekPa, double num2) const;  // :2359-2366 (NaN / 0 -> 0)
    template <class P = aero::MonoFloat>
    float BodyLiftScalar(const Part& part, double liftPressure) const;  // :2369 (liftPressure: the q term in parentheses)

    // FlightIntegrator.cs:2512 DragCubeSetupAndPartAeroStats over vessel.parts. altitudeAtPos = FlightGlobals.getAltitudeAtPos
    // of the part's partTransform.position; gravityTrueMagnitude = vessel.gravityTrue.magnitude.
    template <class P = aero::MonoFloat>
    void DragCubeSetupAndPartAeroStats(const std::vector<Part*>& parts, double gravityTrueMagnitude,
                                       const std::function<double(const Part&)>& altitudeAtPos) const;
};

}  // namespace ksp
