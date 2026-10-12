// WP-20d: the SoA aerodynamics kernel; see include/ksp/physics/aero_soa.hpp. C# paths are relative to
// decompiled/KSP-Decompiled/Assembly-CSharp/. Every float expression is written with aero::FE<MonoFloat> exactly as the WP-21 port
// (src/aero/flight_integrator_aero.cpp, src/aero/drag_cube_list.cpp) writes it, so both evaluate the same IEEE operations.
#include "ksp/physics/aero_soa.hpp"

#include <cmath>
#include <cstring>
#include <limits>

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/aero/float_eval.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp::physics {

using aero::FE;
using aero::MonoFloat;
using aero::S;
using E = FE<MonoFloat>;
using U = aero::U<MonoFloat>;

namespace {

// Part.DragModel (Part.cs:29)
constexpr int32_t kDragDefault = 0, kDragCube = 4;

template <class T>
bool Same(const T& a, const T& b) {
    return std::memcmp(&a, &b, sizeof(T)) == 0;
}
bool Same(const Vector3f& a, const Vector3f& b) { return Same(a.x, b.x) && Same(a.y, b.y) && Same(a.z, b.z); }

// The cube model of CalculateDragValue (FlightIntegrator.cs:2391-2412) and CalculateAerodynamicArea / CalculateAreaExposed reading the
// cube list (:2434-2484): DEFAULT always, CUBE without DragCubesUseSpherical
bool CubeModel(const FlightIntegratorAero& fi, const Part& p) {
    const int32_t m = p.fields.dragModel;
    return m == kDragDefault || (m == kDragCube && !fi.cacheDragCubesUseSpherical);
}

bool Covered(const FlightIntegratorAero& fi, const AeroMachCurves& m, const Part& p) {
    const DragCubeList& d = p.DragCubes;
    return !d.None() && CubeModel(fi, p) && !d.RotateDragVector() && d.DragCurveCd == m.cCd && d.DragCurveCdPower == m.cCdPower &&
           d.DragCurveMultiplier == m.cDragMultiplier && d.SurfaceCurves.dragCurveTail == m.cTail &&
           d.SurfaceCurves.dragCurveSurface == m.cSurface && d.SurfaceCurves.dragCurveTip == m.cTip &&
           d.SurfaceCurves.dragCurveMultiplier == m.cMultiplier && d.BodyLiftCurve != nullptr && m.cCd != nullptr;
}

}  // namespace

// ===================================================================================== outputs

AeroPartOutputs AeroPartOutputs::Capture(const Part& part, const AeroForces& f) {
    AeroPartOutputs o;
    const PartFields& p = part.fields;
    o.aerodynamicArea = p.aerodynamicArea;
    o.exposedArea = p.exposedArea;
    o.submergedDynamicPressurekPa = p.submergedDynamicPressurekPa;
    o.dynamicPressurekPa = p.dynamicPressurekPa;
    o.dragVector = p.dragVector;
    o.dragVectorDir = p.dragVectorDir;
    o.dragVectorDirLocal = p.dragVectorDirLocal;
    o.dragVectorSqrMag = p.dragVectorSqrMag;
    o.dragVectorMag = p.dragVectorMag;
    o.dragScalar = p.dragScalar;
    o.bodyLiftScalar = p.bodyLiftScalar;
    o.bodyLiftLocalPosition = p.bodyLiftLocalPosition;
    o.bodyLiftLocalVector = p.bodyLiftLocalVector;
    o.cubeData = part.DragCubes.GetCubeData();
    o.forces = f;
    return o;
}

bool AeroPartOutputs::BitEqual(const AeroPartOutputs& o, std::string* firstDiff) const {
    auto fail = [firstDiff](const char* what) {
        if (firstDiff != nullptr) *firstDiff = what;
        return false;
    };
#define KSP_SOA_CMP(m) \
    if (!Same(m, o.m)) return fail(#m);
    KSP_SOA_CMP(aerodynamicArea)
    KSP_SOA_CMP(exposedArea)
    KSP_SOA_CMP(submergedDynamicPressurekPa)
    KSP_SOA_CMP(dynamicPressurekPa)
    KSP_SOA_CMP(dragVector)
    KSP_SOA_CMP(dragVectorDir)
    KSP_SOA_CMP(dragVectorDirLocal)
    KSP_SOA_CMP(dragVectorSqrMag)
    KSP_SOA_CMP(dragVectorMag)
    KSP_SOA_CMP(dragScalar)
    KSP_SOA_CMP(bodyLiftScalar)
    KSP_SOA_CMP(bodyLiftLocalPosition)
    KSP_SOA_CMP(bodyLiftLocalVector)
    KSP_SOA_CMP(cubeData.dragVector)
    KSP_SOA_CMP(cubeData.liftForce)
    KSP_SOA_CMP(cubeData.area)
    KSP_SOA_CMP(cubeData.areaDrag)
    KSP_SOA_CMP(cubeData.depth)
    KSP_SOA_CMP(cubeData.crossSectionalArea)
    KSP_SOA_CMP(cubeData.exposedArea)
    KSP_SOA_CMP(cubeData.dragCoeff)
    KSP_SOA_CMP(cubeData.taperDot)
    KSP_SOA_CMP(forces.applyDrag)
    KSP_SOA_CMP(forces.dragForce)
    KSP_SOA_CMP(forces.dragPosition)
    KSP_SOA_CMP(forces.applyLift)
    KSP_SOA_CMP(forces.liftForce)
    KSP_SOA_CMP(forces.liftPosition)
    KSP_SOA_CMP(forces.mode)
    KSP_SOA_CMP(forces.setAngularDrag)
    KSP_SOA_CMP(forces.angularDrag)
#undef KSP_SOA_CMP
    return true;
}

// ===================================================================================== hoisted curves

AeroMachCurves AeroMachCurves::Evaluate(float mach) {
    AeroMachCurves m;
    m.mach = mach;
    // DragCubeList.SetPart (DragCubeList.cs:204) gives every list PhysicsGlobals' curves; the kernel covers the lists that hold them
    const PhysicsGlobals::SurfaceCurvesList& sc = PhysicsGlobals::SurfaceCurves;
    m.cTail = sc.dragCurveTail;
    m.cSurface = sc.dragCurveSurface;
    m.cTip = sc.dragCurveTip;
    m.cMultiplier = sc.dragCurveMultiplier;
    if (PhysicsGlobals::Instance() == nullptr) return m;  // no curves: nothing is covered (m.cCd stays null)
    m.cCd = &PhysicsGlobals::DragCurveCd();
    m.cCdPower = &PhysicsGlobals::DragCurveCdPower();
    m.cDragMultiplier = &PhysicsGlobals::DragCurveMultiplier();
    if (m.cTail == nullptr || m.cSurface == nullptr || m.cTip == nullptr || m.cMultiplier == nullptr) {
        m.cCd = nullptr;  // nothing is covered
        return m;
    }
    // PhysicsGlobals.cs:2692 DragCurveValue: the curves it evaluates per face, all at `mach`
    m.tail = m.cTail->Evaluate(mach);
    m.surface = m.cSurface->Evaluate(mach);
    m.tip = m.cTip->Evaluate(mach);
    m.multiplier = m.cMultiplier->Evaluate(mach);
    // DragCubeList.cs:395 DragCurveCdPower.Evaluate(machNumber), :402 DragCurveMultiplier.Evaluate(machNumber)
    m.cdPower = m.cCdPower->Evaluate(mach);
    m.exposedDiv = m.cDragMultiplier->Evaluate(mach);
    // FlightIntegrator.cs:2383 PhysicsGlobals.BodyLiftCurve.liftMachCurve.Evaluate((float)mach)
    m.liftMach = PhysicsGlobals::BodyLiftCurve()->liftMachCurve.Evaluate(mach);
    return m;
}

// ===================================================================================== rows

void AeroFaceRow::Build(const DragCubeList& cubes) {
    areaOccluded = cubes.AreaOccluded();
    weightedDrag = cubes.WeightedDrag();
    weightedDepth = cubes.WeightedDepth();
    for (int i = 0; i < 6; i++) {
        const float num5 = weightedDrag[static_cast<size_t>(i)];
        // DragCubeList.cs:394-395: num6 = num5 < 1 ? Mathf.Pow(DragCurveCd.Evaluate(num5), cdPower) : num5 (the Evaluate half)
        cd[static_cast<size_t>(i)] = num5 < 1.0f ? cubes.DragCurveCd->Evaluate(num5) : num5;
        // DragCubeList.cs:401: num5 = (!(num5 < 1f) || !(num5 > 0.01f)) ? 1f : (1f / num5)
        invDrag[static_cast<size_t>(i)] = (!(num5 < 1.0f) || !(num5 > 0.01f)) ? 1.0f : S(E(1.0f) / E(num5));
    }
}

bool AeroFaceRow::Matches(const DragCubeList& cubes) const {
    return std::memcmp(areaOccluded.data(), cubes.AreaOccluded().data(), sizeof(float) * 6) == 0 &&
           std::memcmp(weightedDrag.data(), cubes.WeightedDrag().data(), sizeof(float) * 6) == 0 &&
           std::memcmp(weightedDepth.data(), cubes.WeightedDepth().data(), sizeof(float) * 6) == 0;
}

// ===================================================================================== the face loop

// DragCubeList.cs:352 SetDrag(vector, mach) -> :367 AddSurfaceDragDirection(-vector, mach, ref cubeData), with the Mach curves hoisted
DragCubeList::CubeData AeroSoaKernel::SurfaceDrag(const AeroMachCurves& m, const AeroFaceRow& row, const FloatCurve& liftCurve,
                                                  const Vector3f& dragVectorDirLocal) {
    DragCubeList::CubeData retData;
    const Vector3f direction = -dragVectorDirLocal;
    float num = 0.0f;
    retData.Reset();
    retData.dragVector = direction;
    const auto& faces = DragCubeList::FaceDirections();
    for (int i = 0; i < 6; i++) {
        const size_t k = static_cast<size_t>(i);
        const Vector3f& vector = faces[k];
        const float num2 = U::Dot(direction, vector);
        const float dotNormalized = S((E(num2) + E(1.0f)) * E(0.5f));
        // PhysicsGlobals.cs:2692 DragCurveValue with the hoisted curve values
        const float num3 = dotNormalized <= 0.5f ? S(E(U::Lerp(m.tail, m.surface, S(E(dotNormalized) * E(2.0f)))) * E(m.multiplier))
                                                 : S(E(U::Lerp(m.surface, m.tip, S((E(dotNormalized) - E(0.5f)) * E(2.0f)))) * E(m.multiplier));
        const float num4 = S(E(row.areaOccluded[k]) * E(num3));
        const float num5 = row.weightedDrag[k];
        retData.area = S(E(retData.area) + E(num4));
        const float num6 = num5 < 1.0f ? Mathf::Pow(row.cd[k], m.cdPower) : num5;
        retData.areaDrag = S(E(retData.areaDrag) + E(num4) * E(num6));
        retData.crossSectionalArea = S(E(retData.crossSectionalArea) + E(row.areaOccluded[k]) * E(Mathf::Clamp01(num2)));
        const float inv = row.invDrag[k];
        retData.exposedArea = S(E(retData.exposedArea) + E(num4) / E(m.exposedDiv) * E(inv));
        if (num2 > 0.0f) {
            num = S(E(num) + E(num2));
            const double num7 = static_cast<double>(liftCurve.Evaluate(num2));
            if (!std::isnan(num7)) {
                const float scale = S(E(num2) * E(row.areaOccluded[k]) * E(row.weightedDrag[k]) * E(static_cast<float>(num7)));
                retData.liftForce += -vector * scale;
            }
            retData.depth = S(E(retData.depth) + E(num2) * E(row.weightedDepth[k]));
            retData.taperDot = S(E(retData.taperDot) + E(num2) * E(inv));
        }
    }
    if (num > 0.0f) {
        const float num8 = S(E(1.0f) / E(num));
        retData.depth = S(E(retData.depth) * E(num8));
        retData.taperDot = S(E(retData.taperDot) * E(num8));
    }
    if (retData.area > 0.0f) {
        retData.dragCoeff = S(E(retData.areaDrag) / E(retData.area));
        retData.areaDrag = S(E(retData.area) * E(retData.dragCoeff));
    } else {
        retData.areaDrag = 0.0f;
        retData.dragCoeff = 0.0f;
    }
    return retData;
}

// FlightIntegrator.cs:2371-2380 (cube model)
float AeroSoaKernel::DragScalar(const FlightIntegratorAero& fi, float areaDrag, double q, double num2, double& num6) {
    num6 = aero::D(E(areaDrag) * E(fi.cacheDragCubeMultiplier)) * fi.pseudoReDragMult;
    if (!std::isnan(num6) && num6 != 0.0) return S(E(static_cast<float>(q * num6 * num2)) * E(fi.cacheDragMultiplier));
    return 0.0f;
}

// FlightIntegrator.cs:2383
float AeroSoaKernel::BodyLiftScalar(const FlightIntegratorAero& fi, const AeroMachCurves& m, float bodyLiftMultiplier, double q) {
    return S(E(static_cast<float>(q)) * E(bodyLiftMultiplier) * E(fi.cacheBodyLiftMultiplier) * E(m.liftMach));
}

// FlightIntegrator.cs:2309-2389 for a covered part (the cube list is not None and the drag model is the cube model)
void AeroSoaKernel::PartScalars(const FlightIntegratorAero& fi, const AeroMachCurves& m, Part& part, const AeroRigidbody& rigidbody,
                                bool flag3, AeroForces& out) {
    PartFields& f = part.fields;
    const bool flag = rigidbody.partHasRb;
    const double submergedPortion = f.submergedPortion;
    // :2309 CalculateAerodynamicArea: !DragCubes.None -> DragCubes.Area
    f.aerodynamicArea = static_cast<double>(part.DragCubes.Area());
    if (!(fi.cacheApplyDrag && flag3) || (!rigidbody.isPartRb && !fi.cacheApplyDragToNonPhysicsParts)) return;
    double num2 = 1.0;
    bool flag4 = false;
    double num4;
    const CelestialBody* body = fi.currentMainBody;
    if (body != nullptr && body->ocean) {
        if (submergedPortion > 0.0) {
            flag4 = true;
            const double num3 = body->oceanDensity * 1000.0;
            if (submergedPortion >= 1.0) {
                num2 = 0.0;
                f.submergedDynamicPressurekPa = num3;
                num4 = num3 * fi.cacheBuoyancyWaterAngularDragScalar * static_cast<double>(f.waterAngularDragMultiplier);
            } else {
                num2 = 1.0 - submergedPortion;
                f.submergedDynamicPressurekPa = num3;
                num4 = f.staticPressureAtm * num2 +
                       submergedPortion * num3 * fi.cacheBuoyancyWaterAngularDragScalar * static_cast<double>(f.waterAngularDragMultiplier);
            }
        } else {
            f.dynamicPressurekPa = f.atmDensity;
            num4 = f.staticPressureAtm;
        }
    } else {
        if (body == nullptr) throw NullReferenceException();
        f.dynamicPressurekPa = f.atmDensity;
        num4 = f.staticPressureAtm;
    }
    const double num5 = 0.0005 * static_cast<double>(f.dragVectorSqrMag);
    f.dynamicPressurekPa *= num5;
    f.submergedDynamicPressurekPa *= num5;
    if (flag && f.angularDragByFI) {
        if (flag4) {
            num4 += f.dynamicPressurekPa * 0.009869232667160128 * num2;
            num4 += f.submergedDynamicPressurekPa * 0.009869232667160128 * fi.cacheBuoyancyWaterAngularDragScalar *
                    static_cast<double>(f.waterAngularDragMultiplier) * submergedPortion;
        } else {
            num4 = f.dynamicPressurekPa * 0.009869232667160128;
        }
        if (num4 <= 0.0) num4 = 0.0;
        out.setAngularDrag = true;
        out.angularDrag = S(E(f.angularDrag) * E(static_cast<float>(num4)) * E(fi.cacheAngularDragMultiplier));
    }
    out.mode = fi.cacheDragUsesAcceleration ? AeroForceMode::Acceleration : AeroForceMode::Force;
    // :2371 CalculateDragValue -> CalculateDragValue_Cube (:2429): AreaDrag * cacheDragCubeMultiplier (float product widened)
    double num6 = 0.0;
    const float dragScalar = DragScalar(fi, part.DragCubes.AreaDrag(), f.dynamicPressurekPa, num2, num6);
    if (!std::isnan(num6) && num6 != 0.0) {
        f.dragScalar = dragScalar;
        out.applyDrag = true;  // ApplyAeroDrag (:2243)
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
        // :2383 (BodyLiftScalar with the hoisted liftMach)
        f.bodyLiftScalar = BodyLiftScalar(fi, m, f.bodyLiftMultiplier, q);
        if (f.bodyLiftScalar != 0.0f && part.DragCubes.LiftForce() != Vector3f::zero() && !VectorExtensions::IsInvalid(part.DragCubes.LiftForce())) {
            // ApplyAeroLift (:2249)
            const Vector3f zero = (rigidbody.isPartRb || !PhysicsGlobals::ApplyDragToNonPhysicsPartsAtParentCoM())
                                      ? part.partTransform.TransformPoint(f.CoLOffset)
                                      : rigidbody.worldCenterOfMass;
            Vector3f vector = part.partTransform.rotation * (f.bodyLiftScalar * part.DragCubes.LiftForce());
            vector = U::ProjectOnPlane(vector, -f.dragVectorDir);
            f.bodyLiftLocalPosition = part.partTransform.InverseTransformPoint(zero);
            f.bodyLiftLocalVector = part.partTransform.InverseTransformDirection(vector);
            out.applyLift = true;
            out.liftForce = vector;
            out.liftPosition = zero;
        }
    }
}

void AeroSoaKernel::Clear() {
    slots_.clear();
    fallback_.clear();
    covered_ = 0;
}

// FlightIntegrator.cs:2260 UpdateAerodynamics over a batch of parts
void AeroSoaKernel::Run(const FlightIntegratorAero& fi, const std::vector<Part*>& parts, const std::vector<AeroRigidbody>& rbs,
                        std::vector<AeroForces>& out) {
    const size_t n = parts.size();
    out.assign(n, AeroForces{});
    fallback_.clear();
    covered_ = 0;
    // the rows follow the part list (rebuilt from scratch when the list itself changes)
    bool same = slots_.size() == n;
    for (size_t i = 0; same && i < n; ++i) same = slots_[i].part == parts[i];
    if (!same) {
        slots_.assign(n, Slot{});
        for (size_t i = 0; i < n; ++i) slots_[i].part = parts[i];
    }
    const AeroMachCurves m = AeroMachCurves::Evaluate(static_cast<float>(fi.mach));
    for (size_t i = 0; i < n; ++i) {
        Part& part = *parts[i];
        const AeroRigidbody& rigidbody = rbs[i];
        if (!Covered(fi, m, part)) {
            fallback_.push_back(&part);
            out[i] = fi.UpdateAerodynamics<MonoFloat>(part, rigidbody);
            continue;
        }
        ++covered_;
        Slot& slot = slots_[i];
        if (!slot.built || !slot.row.Matches(part.DragCubes)) {
            slot.row.Build(part.DragCubes);
            slot.built = true;
            ++rebuilds_;
        }
        AeroForces& o = out[i];
        PartFields& f = part.fields;
        // :2262-2299 (the rigidbody, the angular-drag reset, the drag vector)
        if (!rigidbody.present) continue;
        const bool flag = rigidbody.partHasRb;
        const bool flag2 = rigidbody.partHasServoRb;
        f.aerodynamicArea = 0.0;
        f.exposedArea = static_cast<double>(part.DragCubes.ExposedArea());  // :2270 CalculateAreaExposed (the previous SetDrag's)
        f.submergedDynamicPressurekPa = 0.0;
        f.dynamicPressurekPa = 0.0;
        if ((flag && f.angularDragByFI) || (flag2 && f.angularDragByFI)) {
            o.setAngularDrag = true;
            o.angularDrag = 0.0f;
        }
        f.dragVector = (Vector3d(rigidbody.velocity) + rigidbody.krakensbaneFrameVelocity).ToVector3f();
        f.dragVectorSqrMag = U::SqrMagnitude(f.dragVector);
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
        if (part.ShieldedFromAirstream() || (!(f.atmDensity > 0.0) && !(submergedPortion > 0.0))) continue;
        // :2307 SetDrag (the face loop over the row)
        part.DragCubes.SetCubeData(SurfaceDrag(m, slot.row, part.DragCubes.BodyLiftCurve->liftCurve, f.dragVectorDirLocal));
        PartScalars(fi, m, part, rigidbody, flag3, o);
    }
}

}  // namespace ksp::physics
