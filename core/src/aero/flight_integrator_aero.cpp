// WP-21: FlightIntegrator's aerodynamic members (FlightIntegrator.cs:2243-2543), see ksp/aero/flight_integrator_aero.hpp.
#include "ksp/aero/flight_integrator_aero.hpp"

#include <cmath>

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"

namespace ksp {

using aero::FE;
using aero::MonoFloat;
using aero::NetFloat;

namespace {
// Part.DragModel (Part.cs:29)
enum DragModel : int32_t { DEFAULT = 0, CONIC = 1, CYLINDRICAL = 2, SPHERICAL = 3, CUBE = 4, NONE = 5 };
}  // namespace

// FlightIntegrator.cs:565-585 (aero subset of FixedUpdate's cache block)
void FlightIntegratorAero::CacheGlobals() {
    cacheDragCubeMultiplier = PhysicsGlobals::DragCubeMultiplier();
    cacheDragCubesUseSpherical = PhysicsGlobals::DragCubesUseSpherical();
    cacheApplyDrag = PhysicsGlobals::ApplyDrag();
    cacheApplyDragToNonPhysicsParts = PhysicsGlobals::ApplyDragToNonPhysicsParts();
    cacheDragUsesAcceleration = PhysicsGlobals::DragUsesAcceleration();
    cacheDragMultiplier = PhysicsGlobals::DragMultiplier();
    cacheBuoyancyWaterAngularDragScalar = PhysicsGlobals::BuoyancyWaterAngularDragScalar();
    cacheAngularDragMultiplier = PhysicsGlobals::AngularDragMultiplier();
    cacheBodyLiftMultiplier = PhysicsGlobals::BodyLiftMultiplier();
}

// FlightIntegrator.cs:2391
template <class P>
double FlightIntegratorAero::CalculateDragValue(const Part& part) const {
    switch (part.fields.dragModel) {
        case DEFAULT: return CalculateDragValue_Cube<P>(part);
        case CONIC: return CalculateDragValue_Conic<P>(part);
        case CYLINDRICAL: return CalculateDragValue_Cylindrical<P>(part);
        case SPHERICAL: return CalculateDragValue_Spherical(part);
        case CUBE:
            if (!cacheDragCubesUseSpherical && !part.DragCubes.None()) return CalculateDragValue_Cube<P>(part);
            return CalculateDragValue_Spherical(part);
        default: return 0.0;
    }
}

// FlightIntegrator.cs:2414
double FlightIntegratorAero::CalculateDragValue_Spherical(const Part& part) const { return static_cast<double>(part.fields.maximum_drag); }

// FlightIntegrator.cs:2419: Mathf.Lerp(min, max, Mathf.Abs(Vector3.Dot(partTransform.TransformDirection(dragReferenceVector), dragVectorDir)))
template <class P>
double FlightIntegratorAero::CalculateDragValue_Cylindrical(const Part& part) const {
    const Vector3f refDir = part.partTransform.TransformDirection(part.fields.dragReferenceVector);
    const float t = Mathf::Abs(aero::U<P>::Dot(refDir, part.fields.dragVectorDir));
    return static_cast<double>(aero::U<P>::Lerp(part.fields.minimum_drag, part.fields.maximum_drag, t));
}

// FlightIntegrator.cs:2424: Mathf.Lerp(min, max, Vector3.Angle(TransformDirection(dragReferenceVector), dragVectorDir) / 180f)
template <class P>
double FlightIntegratorAero::CalculateDragValue_Conic(const Part& part) const {
    using E = FE<P>;
    const Vector3f refDir = part.partTransform.TransformDirection(part.fields.dragReferenceVector);
    const float t = aero::S(E(aero::U<P>::Angle(refDir, part.fields.dragVectorDir)) / E(180.0f));
    return static_cast<double>(aero::U<P>::Lerp(part.fields.minimum_drag, part.fields.maximum_drag, t));
}

// FlightIntegrator.cs:2429: return part.DragCubes.AreaDrag * cacheDragCubeMultiplier (a float product widened to double:
// exact under Mono, rounded to float32 first under .NET)
template <class P>
double FlightIntegratorAero::CalculateDragValue_Cube(const Part& part) const {
    using E = FE<P>;
    return aero::D(E(part.DragCubes.AreaDrag()) * E(cacheDragCubeMultiplier));
}

// FlightIntegrator.cs:2434
double FlightIntegratorAero::CalculateAerodynamicArea(const Part& part) const {
    if (!part.DragCubes.None()) return static_cast<double>(part.DragCubes.Area());
    switch (part.fields.dragModel) {
        case CONIC:
        case CYLINDRICAL:
        case SPHERICAL: return static_cast<double>(part.fields.maximum_drag);
        case DEFAULT:
        case CUBE:
            if (!cacheDragCubesUseSpherical && !part.DragCubes.None()) return static_cast<double>(part.DragCubes.Area());
            return static_cast<double>(part.fields.maximum_drag);
        default: return static_cast<double>(part.fields.maximum_drag);
    }
}

// FlightIntegrator.cs:2460
double FlightIntegratorAero::CalculateAreaExposed(const Part& part) const {
    if (!part.DragCubes.None()) return static_cast<double>(part.DragCubes.ExposedArea());
    switch (part.fields.dragModel) {
        case CONIC:
        case CYLINDRICAL:
        case SPHERICAL: return static_cast<double>(part.fields.maximum_drag);
        case DEFAULT:
        case CUBE:
            if (!cacheDragCubesUseSpherical && !part.DragCubes.None()) return static_cast<double>(part.DragCubes.ExposedArea());
            return static_cast<double>(part.fields.maximum_drag);
        default: return static_cast<double>(part.fields.maximum_drag);
    }
}

// FlightIntegrator.cs:2486
double FlightIntegratorAero::CalculateAreaRadiative(const Part& part) const {
    if (!part.DragCubes.None()) return static_cast<double>(part.DragCubes.PostOcclusionArea());
    switch (part.fields.dragModel) {
        case CONIC:
        case CYLINDRICAL:
        case SPHERICAL: return static_cast<double>(part.fields.maximum_drag);
        case DEFAULT:
        case CUBE:
            if (!cacheDragCubesUseSpherical && !part.DragCubes.None()) return static_cast<double>(part.DragCubes.PostOcclusionArea());
            return static_cast<double>(part.fields.maximum_drag);
        default: return static_cast<double>(part.fields.maximum_drag);
    }
}

// FlightIntegrator.cs:2359-2366: num6 = CalculateDragValue(part) * pseudoReDragMult;
// dragScalar = (float)(part.dynamicPressurekPa * num6 * num2) * cacheDragMultiplier, or 0 when num6 is NaN or 0
template <class P>
float FlightIntegratorAero::DragScalar(const Part& part, double dynamicPressurekPa, double num2) const {
    using E = FE<P>;
    const double num6 = CalculateDragValue<P>(part) * pseudoReDragMult;
    if (!std::isnan(num6) && num6 != 0.0)
        return aero::S(E(static_cast<float>(dynamicPressurekPa * num6 * num2)) * E(cacheDragMultiplier));
    return 0.0f;
}

// FlightIntegrator.cs:2369: (float)(q-term) * part.bodyLiftMultiplier * cacheBodyLiftMultiplier
//                          * PhysicsGlobals.BodyLiftCurve.liftMachCurve.Evaluate((float)mach)
template <class P>
float FlightIntegratorAero::BodyLiftScalar(const Part& part, double liftPressure) const {
    using E = FE<P>;
    const float liftMach = PhysicsGlobals::BodyLiftCurve()->liftMachCurve.Evaluate(static_cast<float>(mach));
    return aero::S(E(static_cast<float>(liftPressure)) * E(part.fields.bodyLiftMultiplier) * E(cacheBodyLiftMultiplier) * E(liftMach));
}

// FlightIntegrator.cs:2260 UpdateAerodynamics (with ApplyAeroDrag :2243 and ApplyAeroLift :2249 returning the forces)
template <class P>
AeroForces FlightIntegratorAero::UpdateAerodynamics(Part& part, const AeroRigidbody& rigidbody) const {
    using E = FE<P>;
    AeroForces out;
    PartFields& f = part.fields;
    if (!rigidbody.present) return out;
    const bool flag = rigidbody.partHasRb;
    const bool flag2 = rigidbody.partHasServoRb;
    f.aerodynamicArea = 0.0;
    f.exposedArea = CalculateAreaExposed(part);
    f.submergedDynamicPressurekPa = 0.0;
    f.dynamicPressurekPa = 0.0;
    if ((flag && f.angularDragByFI) || (flag2 && f.angularDragByFI)) {
        out.setAngularDrag = true;
        out.angularDrag = 0.0f;
    }
    // part.dragVector = rigidbody.velocity + Krakensbane.GetFrameVelocity() (Vector3 + Vector3d -> Vector3d -> Vector3)
    f.dragVector = (Vector3d(rigidbody.velocity) + rigidbody.krakensbaneFrameVelocity).ToVector3f();
    f.dragVectorSqrMag = aero::U<P>::SqrMagnitude(f.dragVector);
    bool flag3 = false;
    if (f.dragVectorSqrMag != 0.0f) {
        flag3 = true;
        f.dragVectorMag = Mathf::Sqrt(f.dragVectorSqrMag);
        f.dragVectorDir = f.dragVector / f.dragVectorMag;
        f.dragVectorDirLocal = -part.partTransform.InverseTransformDirection(f.dragVectorDir);
        f.dragScalar = 0.0f;
    } else {
        f.dragVectorMag = 0.0f;
        f.dragVectorDir = Vector3f::zero();
        f.dragVectorDirLocal = Vector3f::zero();
        f.dragScalar = 0.0f;
    }
    const double submergedPortion = f.submergedPortion;
    if (part.ShieldedFromAirstream() || (!(f.atmDensity > 0.0) && !(submergedPortion > 0.0))) return out;
    if (!part.DragCubes.None()) part.DragCubes.SetDrag<P>(f.dragVectorDirLocal, static_cast<float>(mach));
    f.aerodynamicArea = CalculateAerodynamicArea(part);
    if (!(cacheApplyDrag && flag3) || (!rigidbody.isPartRb && !cacheApplyDragToNonPhysicsParts)) return out;
    double num2 = 1.0;
    bool flag4 = false;
    double num4;
    if (currentMainBody != nullptr && currentMainBody->ocean) {
        if (submergedPortion > 0.0) {
            flag4 = true;
            const double num3 = currentMainBody->oceanDensity * 1000.0;
            if (submergedPortion >= 1.0) {
                num2 = 0.0;
                f.submergedDynamicPressurekPa = num3;
                num4 = num3 * cacheBuoyancyWaterAngularDragScalar * static_cast<double>(f.waterAngularDragMultiplier);
            } else {
                num2 = 1.0 - submergedPortion;
                f.submergedDynamicPressurekPa = num3;
                num4 = f.staticPressureAtm * num2 +
                       submergedPortion * num3 * cacheBuoyancyWaterAngularDragScalar * static_cast<double>(f.waterAngularDragMultiplier);
            }
        } else {
            f.dynamicPressurekPa = f.atmDensity;
            num4 = f.staticPressureAtm;
        }
    } else {
        if (currentMainBody == nullptr) throw NullReferenceException();
        f.dynamicPressurekPa = f.atmDensity;
        num4 = f.staticPressureAtm;
    }
    const double num5 = 0.0005 * static_cast<double>(f.dragVectorSqrMag);
    f.dynamicPressurekPa *= num5;
    f.submergedDynamicPressurekPa *= num5;
    if (flag && f.angularDragByFI) {
        if (flag4) {
            num4 += f.dynamicPressurekPa * 0.009869232667160128 * num2;
            num4 += f.submergedDynamicPressurekPa * 0.009869232667160128 * cacheBuoyancyWaterAngularDragScalar *
                    static_cast<double>(f.waterAngularDragMultiplier) * submergedPortion;
        } else {
            num4 = f.dynamicPressurekPa * 0.009869232667160128;
        }
        if (num4 <= 0.0) num4 = 0.0;
        out.setAngularDrag = true;
        out.angularDrag = aero::S(E(f.angularDrag) * E(static_cast<float>(num4)) * E(cacheAngularDragMultiplier));
    }
    out.mode = cacheDragUsesAcceleration ? AeroForceMode::Acceleration : AeroForceMode::Force;
    const double num6 = CalculateDragValue<P>(part) * pseudoReDragMult;
    if (!std::isnan(num6) && num6 != 0.0) {
        f.dragScalar = aero::S(E(static_cast<float>(f.dynamicPressurekPa * num6 * num2)) * E(cacheDragMultiplier));
        // ApplyAeroDrag (:2243)
        out.applyDrag = true;
        out.dragPosition = (rigidbody.isPartRb || !PhysicsGlobals::ApplyDragToNonPhysicsPartsAtParentCoM())
                               ? part.partTransform.TransformPoint(f.CoPOffset)
                               : rigidbody.worldCenterOfMass;
        out.dragForce = -f.dragVectorDir * f.dragScalar;
    } else {
        f.dragScalar = 0.0f;
    }
    if (!f.hasLiftModule &&
        (!f.bodyLiftOnlyUnattachedLiftActual || part.bodyLiftOnlyProvider == nullptr || !part.bodyLiftOnlyProvider->IsLifting())) {
        const double q = flag4 ? (f.dynamicPressurekPa * num2 + f.submergedDynamicPressurekPa * f.submergedLiftScalar * submergedPortion)
                               : f.dynamicPressurekPa;
        f.bodyLiftScalar = BodyLiftScalar<P>(part, q);
        if (f.bodyLiftScalar != 0.0f && part.DragCubes.LiftForce() != Vector3f::zero() && !VectorExtensions::IsInvalid(part.DragCubes.LiftForce())) {
            // ApplyAeroLift (:2249)
            const Vector3f zero = (rigidbody.isPartRb || !PhysicsGlobals::ApplyDragToNonPhysicsPartsAtParentCoM())
                                      ? part.partTransform.TransformPoint(f.CoLOffset)
                                      : rigidbody.worldCenterOfMass;
            Vector3f vector = part.partTransform.rotation * (f.bodyLiftScalar * part.DragCubes.LiftForce());
            vector = aero::U<P>::ProjectOnPlane(vector, -f.dragVectorDir);
            f.bodyLiftLocalPosition = part.partTransform.InverseTransformPoint(zero);
            f.bodyLiftLocalVector = part.partTransform.InverseTransformDirection(vector);
            out.applyLift = true;
            out.liftForce = vector;
            out.liftPosition = zero;
        }
    }
    return out;
}

// FlightIntegrator.cs:2512
template <class P>
void FlightIntegratorAero::DragCubeSetupAndPartAeroStats(const std::vector<Part*>& parts, double gravityTrueMagnitude,
                                                         const std::function<double(const Part&)>& altitudeAtPos) const {
    const size_t partCount = parts.size();
    for (size_t i = 0; i < partCount; i++)
        if (!parts[i]->DragCubes.None()) parts[i]->DragCubes.SetDragWeights<P>();
    for (size_t j = 0; j < partCount; j++)
        if (!parts[j]->DragCubes.None()) parts[j]->DragCubes.SetPartOcclusion<P>();
    const double magnitude = gravityTrueMagnitude;
    for (size_t k = 0; k < partCount; k++) {
        Part& part = *parts[k];
        part.fields.radiativeArea = CalculateAreaRadiative(part);
        const double altitudeAtPos_ = altitudeAtPos ? altitudeAtPos(part) : 0.0;
        if (currentMainBody == nullptr) throw NullReferenceException();
        part.fields.staticPressureAtm = currentMainBody->GetPressure(altitudeAtPos_);
        part.fields.atmDensity = density;
        if (currentMainBody->ocean && altitudeAtPos_ < 0.0)
            part.fields.staticPressureAtm += magnitude * (-altitudeAtPos_) * currentMainBody->oceanDensity;
        part.fields.staticPressureAtm *= 0.009869232667160128;
        part.fields.machNumber = mach;
    }
}

#define KSP_FIAERO_INSTANTIATE(P)                                                                                       \
    template double FlightIntegratorAero::CalculateDragValue<P>(const Part&) const;                                     \
    template double FlightIntegratorAero::CalculateDragValue_Cylindrical<P>(const Part&) const;                         \
    template double FlightIntegratorAero::CalculateDragValue_Conic<P>(const Part&) const;                               \
    template double FlightIntegratorAero::CalculateDragValue_Cube<P>(const Part&) const;                                \
    template AeroForces FlightIntegratorAero::UpdateAerodynamics<P>(Part&, const AeroRigidbody&) const;                 \
    template float FlightIntegratorAero::DragScalar<P>(const Part&, double, double) const;                              \
    template float FlightIntegratorAero::BodyLiftScalar<P>(const Part&, double) const;                                  \
    template void FlightIntegratorAero::DragCubeSetupAndPartAeroStats<P>(const std::vector<Part*>&, double,             \
                                                                         const std::function<double(const Part&)>&) const;
KSP_FIAERO_INSTANTIATE(MonoFloat)
KSP_FIAERO_INSTANTIATE(NetFloat)
#undef KSP_FIAERO_INSTANTIATE

}  // namespace ksp
