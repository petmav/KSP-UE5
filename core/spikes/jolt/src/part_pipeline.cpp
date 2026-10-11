#include "part_pipeline.hpp"

#include <chrono>
#include <cmath>
#include <cstring>
#include <stdexcept>

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/aero/float_eval.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp_context.hpp"

namespace spike {

using Clock = std::chrono::steady_clock;
using ksp::aero::FE;
using ksp::aero::MonoFloat;
using ksp::aero::S;

namespace {
double Ms(Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); }

template <class T>
bool BitEq(const T& a, const T& b) {
    return std::memcmp(&a, &b, sizeof(T)) == 0;
}
bool BitEq(const ksp::Vector3f& a, const ksp::Vector3f& b) { return BitEq(a.x, b.x) && BitEq(a.y, b.y) && BitEq(a.z, b.z); }

// Part.DragModel (Part.cs:29)
constexpr int kDragDefault = 0, kDragCube = 4;
}  // namespace

PartPipeline::PartPipeline(KspContext& ctx, CraftModel& craft) : ctx_(ctx), craft_(craft) {
    for (const PartSpec& s : craft.parts) parts_.push_back(s.part);
    atm_.currentMainBody = &ctx.Kerbin();
    atm_.situation = ksp::VesselSituations::FLYING;
    fi_.CacheGlobals();
    fi_.currentMainBody = &ctx.Kerbin();
    exactOut_.resize(parts_.size());
    soaOut_.resize(parts_.size());
}

void PartPipeline::SetTransforms(VesselPhysics& model) {
    // vessel-local float frame: origin = the vessel CoM of this tick (KSP's floating origin sits near the vessel too, so the
    // float values the ports see have the same magnitude as in the game; only the arbitrary origin offset differs)
    refPoint_ = model.com;
    for (size_t i = 0; i < parts_.size(); i++) {
        ksp::PartTransform& t = parts_[i]->partTransform;
        t.position = ToK(JPH::Vec3(model.partPos[i] - refPoint_));
        t.rotation = ToK(model.partRot[i]);
    }
}

void PartPipeline::ApplyAero(VesselPhysics& model, int part, const ksp::AeroForces& f) {
    // positions come back in the vessel-local float frame: add the double reference point
    if (f.applyDrag) model.AddForceAtPosition(part, ToJ(f.dragForce), refPoint_ + JPH::RVec3(ToJ(f.dragPosition)));
    if (f.applyLift) model.AddForceAtPosition(part, ToJ(f.liftForce), refPoint_ + JPH::RVec3(ToJ(f.liftPosition)));
    if (f.setAngularDrag) model.SetAngularDrag(part, f.angularDrag);
}

ksp::AeroForces PartPipeline::AeroExactPart(VesselPhysics& model, size_t i) {
    const PartSpec& s = craft_.parts[i];
    ksp::AeroRigidbody rb;
    rb.present = true;
    rb.isPartRb = s.physical;
    rb.partHasRb = s.physical;
    // part.Rigidbody.velocity: the part's own body (its CoM velocity), or its owner's for a physicsless part
    rb.velocity = ToK(model.partVel[s.physical ? static_cast<int>(i) : s.rbOwner]);
    const int o = s.rbOwner;
    rb.worldCenterOfMass = ToK(JPH::Vec3(model.partPos[o] + model.partRot[o] * craft_.parts[o].comLocal - refPoint_));
    rb.krakensbaneFrameVelocity = ksp::Vector3d();
    return fi_.UpdateAerodynamics<MonoFloat>(*parts_[i], rb);
}

void PartPipeline::AeroExact(VesselPhysics& model, std::vector<ksp::AeroForces>* out) {
    for (size_t i = 0; i < parts_.size(); i++) {
        const ksp::AeroForces f = AeroExactPart(model, i);
        if (out) (*out)[i] = f;
        else ApplyAero(model, static_cast<int>(i), f);
    }
}

void PartPipeline::BuildSoa() {
    soa_.clear();
    soaEligible_.assign(parts_.size(), 0);
    for (size_t i = 0; i < parts_.size(); i++) {
        ksp::Part& p = *parts_[i];
        const ksp::PartFields& f = p.fields;
        const bool cubeModel = f.dragModel == kDragDefault || (f.dragModel == kDragCube && !fi_.cacheDragCubesUseSpherical);
        if (p.DragCubes.None() || !cubeModel || p.DragCubes.RotateDragVector()) continue;
        if (!(!f.hasLiftModule && !f.bodyLiftOnlyUnattachedLiftActual)) continue;  // the spike's SoA path covers plain body lift
        SoaPart sp;
        sp.part = static_cast<int>(i);
        sp.areaOccluded = p.DragCubes.AreaOccluded();
        sp.weightedDrag = p.DragCubes.WeightedDrag();
        sp.weightedDepth = p.DragCubes.WeightedDepth();
        for (int k = 0; k < 6; k++) {
            const float num5 = sp.weightedDrag[k];
            sp.cdCurve[k] = num5 < 1.0f ? p.DragCubes.DragCurveCd->Evaluate(num5) : num5;
            sp.invDrag[k] = (!(num5 < 1.0f) || !(num5 > 0.01f)) ? 1.0f : S(FE<MonoFloat>(1.0f) / FE<MonoFloat>(num5));
        }
        soa_.push_back(sp);
        soaEligible_[i] = 1;
    }
}

// The WP-21 UpdateAerodynamics + DragCubeList.SetDrag / AddSurfaceDragDirection statements over SoA data (see the header).
void PartPipeline::AeroSoA(VesselPhysics& model, std::vector<ksp::AeroForces>* out) {
    using E = FE<MonoFloat>;
    using U = ksp::aero::U<MonoFloat>;
    if (soa_.empty() && soaEligible_.empty()) BuildSoa();
    if (!out)  // parts outside the SoA kernel's scope take the exact path
        for (size_t i = 0; i < parts_.size(); i++)
            if (!soaEligible_[i]) ApplyAero(model, static_cast<int>(i), AeroExactPart(model, i));
    if (soa_.empty()) return;
    const ksp::DragCubeList& d0 = parts_[soa_.front().part]->DragCubes;
    const float machF = static_cast<float>(fi_.mach);
    // hoisted: functions of the vessel's Mach only (identical inputs -> identical values to the per-face evaluations)
    const float tail = d0.SurfaceCurves.dragCurveTail->Evaluate(machF);
    const float surf = d0.SurfaceCurves.dragCurveSurface->Evaluate(machF);
    const float tip = d0.SurfaceCurves.dragCurveTip->Evaluate(machF);
    const float mult = d0.SurfaceCurves.dragCurveMultiplier->Evaluate(machF);
    const float cdPow = d0.DragCurveCdPower->Evaluate(machF);
    const float exposedDiv = d0.DragCurveMultiplier->Evaluate(machF);
    const float liftMach = ksp::PhysicsGlobals::BodyLiftCurve()->liftMachCurve.Evaluate(machF);
    const ksp::FloatCurve& liftCurve = d0.BodyLiftCurve->liftCurve;
    const auto& faces = ksp::DragCubeList::FaceDirections();
    const bool ocean = fi_.currentMainBody->ocean;
    for (const SoaPart& sp : soa_) {
        const int i = sp.part;
        const PartSpec& s = craft_.parts[i];
        ksp::Part& p = *parts_[i];
        const ksp::PartFields& f = p.fields;
        ksp::AeroForces o;
        if ((s.physical && f.angularDragByFI)) {
            o.setAngularDrag = true;
            o.angularDrag = 0.0f;
        }
        const ksp::Vector3f vel = ToK(model.partVel[s.physical ? i : s.rbOwner]);
        const ksp::Vector3f dragVector = (ksp::Vector3d(vel) + ksp::Vector3d()).ToVector3f();
        const float sqrMag = U::SqrMagnitude(dragVector);
        ksp::Vector3f dir, dirLocal;
        const bool flag3 = sqrMag != 0.0f;
        if (flag3) {
            const float mag = ksp::Mathf::Sqrt(sqrMag);
            dir = dragVector / mag;
            dirLocal = -p.partTransform.InverseTransformDirection(dir);
        }
        const double submerged = f.submergedPortion;
        if (p.ShieldedFromAirstream() || (!(f.atmDensity > 0.0) && !(submerged > 0.0))) {
            if (out) (*out)[i] = o;
            else ApplyAero(model, i, o);
            continue;
        }
        // ---- AddSurfaceDragDirection(-dirLocal, mach)
        const ksp::Vector3f direction = -dirLocal;
        float num = 0.0f, area = 0.0f, areaDrag = 0.0f, cross = 0.0f, exposed = 0.0f, depth = 0.0f, taper = 0.0f;
        ksp::Vector3f liftForce{};
        for (int k = 0; k < 6; k++) {
            const float num2 = U::Dot(direction, faces[k]);
            const float dotN = S((E(num2) + E(1.0f)) * E(0.5f));
            const float num3 = dotN <= 0.5f ? S(E(U::Lerp(tail, surf, S(E(dotN) * E(2.0f)))) * E(mult))
                                            : S(E(U::Lerp(surf, tip, S((E(dotN) - E(0.5f)) * E(2.0f)))) * E(mult));
            const float num4 = S(E(sp.areaOccluded[k]) * E(num3));
            const float wd = sp.weightedDrag[k];
            area = S(E(area) + E(num4));
            const float num6 = wd < 1.0f ? ksp::Mathf::Pow(sp.cdCurve[k], cdPow) : wd;
            areaDrag = S(E(areaDrag) + E(num4) * E(num6));
            cross = S(E(cross) + E(sp.areaOccluded[k]) * E(ksp::Mathf::Clamp01(num2)));
            const float num5 = sp.invDrag[k];
            exposed = S(E(exposed) + E(num4) / E(exposedDiv) * E(num5));
            if (num2 > 0.0f) {
                num = S(E(num) + E(num2));
                const double num7 = static_cast<double>(liftCurve.Evaluate(num2));
                if (!std::isnan(num7)) {
                    const float scale = S(E(num2) * E(sp.areaOccluded[k]) * E(wd) * E(static_cast<float>(num7)));
                    liftForce += -faces[k] * scale;
                }
                depth = S(E(depth) + E(num2) * E(sp.weightedDepth[k]));
                taper = S(E(taper) + E(num2) * E(num5));
            }
        }
        if (num > 0.0f) {
            const float num8 = S(E(1.0f) / E(num));
            depth = S(E(depth) * E(num8));
            taper = S(E(taper) * E(num8));
        }
        float dragCoeff = 0.0f;
        if (area > 0.0f) {
            dragCoeff = S(E(areaDrag) / E(area));
            areaDrag = S(E(area) * E(dragCoeff));
        } else {
            areaDrag = 0.0f;
        }
        (void)depth;
        (void)taper;
        (void)cross;
        (void)exposed;
        // ---- the rest of UpdateAerodynamics (FlightIntegrator.cs:2320-2389)
        if (!(fi_.cacheApplyDrag && flag3) || (!s.physical && !fi_.cacheApplyDragToNonPhysicsParts)) {
            if (out) (*out)[i] = o;
            else ApplyAero(model, i, o);
            continue;
        }
        double num2d = 1.0, num4d;
        double q = 0.0;
        if (ocean && submerged > 0.0) throw std::logic_error("SoA aero: submerged parts are not covered by the spike kernel");
        q = f.atmDensity;
        num4d = f.staticPressureAtm;
        q *= 0.0005 * static_cast<double>(sqrMag);
        if (s.physical && f.angularDragByFI) {
            num4d = q * 0.009869232667160128;
            if (num4d <= 0.0) num4d = 0.0;
            o.setAngularDrag = true;
            o.angularDrag = S(E(f.angularDrag) * E(static_cast<float>(num4d)) * E(fi_.cacheAngularDragMultiplier));
        }
        o.mode = fi_.cacheDragUsesAcceleration ? ksp::AeroForceMode::Acceleration : ksp::AeroForceMode::Force;
        const double num6 = ksp::aero::D(E(areaDrag) * E(fi_.cacheDragCubeMultiplier)) * fi_.pseudoReDragMult;
        if (!std::isnan(num6) && num6 != 0.0) {
            const float dragScalar = S(E(static_cast<float>(q * num6 * num2d)) * E(fi_.cacheDragMultiplier));
            o.applyDrag = true;
            o.dragPosition = (s.physical || !ksp::PhysicsGlobals::ApplyDragToNonPhysicsPartsAtParentCoM())
                                 ? p.partTransform.TransformPoint(f.CoPOffset)
                                 : ToK(JPH::Vec3(model.partPos[s.rbOwner] + model.partRot[s.rbOwner] * craft_.parts[s.rbOwner].comLocal - refPoint_));
            o.dragForce = -dir * dragScalar;
        }
        const float bodyLiftScalar = S(E(static_cast<float>(q)) * E(f.bodyLiftMultiplier) * E(fi_.cacheBodyLiftMultiplier) * E(liftMach));
        if (bodyLiftScalar != 0.0f && liftForce != ksp::Vector3f::zero() && !ksp::VectorExtensions::IsInvalid(liftForce)) {
            const ksp::Vector3f zero = (s.physical || !ksp::PhysicsGlobals::ApplyDragToNonPhysicsPartsAtParentCoM())
                                           ? p.partTransform.TransformPoint(f.CoLOffset)
                                           : ToK(JPH::Vec3(model.partPos[s.rbOwner] + model.partRot[s.rbOwner] * craft_.parts[s.rbOwner].comLocal - refPoint_));
            ksp::Vector3f v = p.partTransform.rotation * (bodyLiftScalar * liftForce);
            v = U::ProjectOnPlane(v, -dir);
            o.applyLift = true;
            o.liftForce = v;
            o.liftPosition = zero;
        }
        if (out) (*out)[i] = o;
        else ApplyAero(model, i, o);
    }
}

void PartPipeline::Tick(VesselPhysics& model, const JPH::DVec3& frameOrigin, bool aero, AeroKernel kernel, float throttle,
                        PipelineTimes& t) {
    frameOrigin_ = frameOrigin;
    auto t0 = Clock::now();
    SetTransforms(model);
    auto t1 = Clock::now();
    // FlightIntegrator.UpdateMassStats (exact WP-14 port)
    craft_.vessel->UpdateMassStats();
    auto t2 = Clock::now();
    // FlightIntegrator: CalculatePressure / CalculateConstantsAtmosphere (exact WP-09 port) at the vessel CoM
    const JPH::DVec3 comBody = frameOrigin + JPH::DVec3(model.com);
    const double R = ctx_.Kerbin().Radius;
    altitude = comBody.Length() - R;
    atm_.altitude = altitude;
    atm_.spd = model.comVel.Length();
    atm_.CalculatePressure();
    if (atm_.staticPressurekPa > 0.0) atm_.CalculateConstantsAtmosphere();
    else atm_.CalculateConstantsVacuum();
    fi_.mach = atm_.mach;
    fi_.density = atm_.density;
    fi_.pseudoReDragMult = atm_.pseudoReDragMult;
    mach = atm_.mach;
    dynamicPressure = atm_.dynamicPressurekPa;
    auto t3 = Clock::now();
    if (aero) {
        const JPH::RVec3 ref = refPoint_;
        const JPH::DVec3 fo = frameOrigin;
        const double gMag = ctx_.Kerbin().gMagnitudeAtCenter / (comBody.LengthSq());
        fi_.DragCubeSetupAndPartAeroStats<MonoFloat>(parts_, gMag, [&](const ksp::Part& p) {
            const ksp::Vector3f lp = p.partTransform.position;
            const JPH::DVec3 w = fo + JPH::DVec3(ref) + JPH::DVec3(lp.x, lp.y, lp.z);
            return w.Length() - R;
        });
    }
    auto t4 = Clock::now();
    if (aero) {
        switch (kernel) {
            case AeroKernel::Exact: AeroExact(model, nullptr); break;
            case AeroKernel::SoA: AeroSoA(model, nullptr); break;
            case AeroKernel::Both: {
                AeroExact(model, &exactOut_);
                AeroSoA(model, &soaOut_);
                for (const SoaPart& sp : soa_) {
                    const ksp::AeroForces& a = exactOut_[sp.part];
                    const ksp::AeroForces& b = soaOut_[sp.part];
                    const bool eq = a.applyDrag == b.applyDrag && a.applyLift == b.applyLift && a.setAngularDrag == b.setAngularDrag &&
                                    BitEq(a.dragForce, b.dragForce) && BitEq(a.dragPosition, b.dragPosition) &&
                                    BitEq(a.liftForce, b.liftForce) && BitEq(a.liftPosition, b.liftPosition) &&
                                    BitEq(a.angularDrag, b.angularDrag);
                    soaCompared++;
                    if (!eq) soaMismatches++;
                }
                for (size_t i = 0; i < parts_.size(); i++) ApplyAero(model, static_cast<int>(i), exactOut_[i]);
                break;
            }
        }
    }
    auto t5 = Clock::now();
    // engines: full thrust along the part's axis at the part (Mainsail / LV-T45 thrust transforms point down the part's -Y)
    if (throttle > 0.0f)
        for (size_t i = 0; i < craft_.parts.size(); i++) {
            const PartSpec& s = craft_.parts[i];
            if (!s.isEngine) continue;
            const JPH::Vec3 up = model.partRot[i] * JPH::Vec3::sAxisY();
            model.AddForceAtPosition(static_cast<int>(i), up * (s.maxThrust * throttle), model.partPos[i]);
        }
    auto t6 = Clock::now();
    t.transformsMs += Ms(t0, t1);
    t.massMs += Ms(t1, t2);
    t.atmosphereMs += Ms(t2, t3);
    t.aeroSetupMs += Ms(t3, t4);
    t.aeroMs += Ms(t4, t5);
    t.enginesMs += Ms(t5, t6);
}

}  // namespace spike
