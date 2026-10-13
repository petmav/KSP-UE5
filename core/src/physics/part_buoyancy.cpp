// WP-20e: PartBuoyancy.cs (see ksp/physics/part_buoyancy.hpp). C# paths are relative to decompiled/KSP-Decompiled/Assembly-CSharp/.
// Every float statement follows the Mono rule: fx() widens a float operand, f32store() rounds where the C# stores / passes / returns a
// float; Vector3 / Vector3d operators are the managed ones of ksp/unity/vector.hpp and ksp/math/vector3d.hpp (Vector3 + Vector3d is
// Vector3d's mixed operator, a Vector3d assigned to a Vector3 is the implicit per-component float conversion).
#include "ksp/physics/part_buoyancy.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/matrix_native.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;
using PG = PhysicsGlobals;

namespace {
// `Vector3 a = (Vector3d)expr`: the implicit Vector3d -> Vector3 conversion
Vector3f F3(const Vector3d& v) { return v.ToVector3f(); }
// Bounds(center, size).min / .max (managed: extents = size * 0.5f, min = center - extents, max = center + extents)
Vector3f BoundsMin(const Vector3f& c, const Vector3f& size) { return c - size * 0.5f; }
Vector3f BoundsMax(const Vector3f& c, const Vector3f& size) { return c + size * 0.5f; }
}  // namespace

// PartBuoyancy.cs:98
void PartBuoyancy::Start(Part& p, const Env& env) {
    part = &p;
    canForceUpdate = true;
    centerOfBuoyancy = part->partTransform.position + part->partTransform.rotation * part->fields.CenterOfBuoyancy;
    centerOfDisplacement = part->partTransform.position + part->partTransform.rotation * part->fields.CenterOfDisplacement;
    pLast = centerOfBuoyancy;
    isKerbal = part->FindModuleOfClass("KerbalEVA") != nullptr;
    if (overrideCubeOnChutesIfUnspecified && (!part->fields.buoyancyUseCubeNamed.has_value() || part->fields.buoyancyUseCubeNamed->empty()) &&
        part->Modules().Contains("ModuleParachute")) {
        for (const DragCube& c : part->DragCubes.Cubes())
            if (c.Name() == "PACKED") {
                part->fields.buoyancyUseCubeNamed = std::string("PACKED");
                break;
            }
    }
    part->fields.submergedDragScalar = (dragScalar = PG::BuoyancyWaterDragScalar());
    part->fields.submergedPortion = 0.0;
    submergedPortion = 0.0;
    liftScalar = 0.0;
    FindOverrideCube();
    UpdateDisplacement(env);
    lastBuoyantForce = Vector3f();
    lastForcePosition = centerOfBuoyancy;
}

// PartBuoyancy.cs:147
void PartBuoyancy::FindOverrideCube() {
    overrideCube = nullptr;
    if (!part->fields.buoyancyUseCubeNamed.has_value() || part->fields.buoyancyUseCubeNamed->empty()) return;
    for (const DragCube& c : part->DragCubes.Cubes())
        if (c.Name() == *part->fields.buoyancyUseCubeNamed) {
            overrideCube = &c;
            return;
        }
}

// PartBuoyancy.cs:171 (`displacement = vector.x * vector.y * vector.z`: a float expression stored to a double field, not rounded to float)
void PartBuoyancy::UpdateDisplacement(const Env& env) {
    displacement = PG::BuoyancyDefaultVolume();
    maxDimension = 1.0f;
    if (!part->DragCubes.None()) {
        if (part->DragCubes.WeightedSize().x == 0.0f && canForceUpdate && overrideCube == nullptr) {
            part->DragCubes.ForceUpdate(true, true, true);
            canForceUpdate = false;
            return;
        }
        const std::array<float, 6>& array = overrideCube != nullptr ? overrideCube->Area() : part->DragCubes.WeightedArea();
        const Vector3f vector = overrideCube != nullptr ? overrideCube->Size() : part->DragCubes.WeightedSize();
        displacement = fx(vector.x) * fx(vector.y) * fx(vector.z);
        maxDimension = vector.x;
        if (vector.y > maxDimension) maxDimension = vector.y;
        if (vector.y > maxDimension) maxDimension = vector.z;  // sic: y twice (the C#)
        xPortion = fx(array[0]) / (fx(vector.y) * fx(vector.z));
        yPortion = fx(array[2]) / (fx(vector.x) * fx(vector.z));
        zPortion = fx(array[4]) / (fx(vector.x) * fx(vector.y));
        const double num = xPortion + yPortion + zPortion;
        if (!std::isnan(num) && num > 0.0) {
            xzPortion = (std::min(xPortion, zPortion) + 2.0 * (xPortion * zPortion)) * (1.0 / 3.0);
            displacement *= xzPortion * yPortion;
        }
    } else if (env.colliderBoundsSize) {
        const Vector3f size = env.colliderBoundsSize(*part);
        displacement = (fx(size.x) * fx(size.y) * fx(size.z)) * 0.75;
        maxDimension = size.x;
        if (size.y > maxDimension) maxDimension = size.y;
        if (size.y > maxDimension) maxDimension = size.z;
    }
}

void PartBuoyancy::TickInvoke(double dt) {
    if (invokeIn < 0.0) return;
    invokeIn -= dt;
    if (invokeIn <= 0.0) {
        invokeIn = -1.0;
        ReportSplashDown();
    }
}

// PartBuoyancy.cs:672
void PartBuoyancy::ReportSplashDown() { part->OnSplashDown(); }

// PartBuoyancy.cs:231
void PartBuoyancy::FixedUpdate(const Env& env) {
    lastApplied = lastCrash = lastSplashFx = false;
    dead = false;
    if (!enabled) return;
    if (!FlightGlobals::ready || part->vessel == nullptr || part->vessel->mainBody() == nullptr || !(body = part->vessel->mainBody())->ocean ||
        !env.hasRigidbody(*part))
        return;
    Vessel& vessel = *part->vessel;
    const bool shieldedFromAirstream = part->ShieldedFromAirstream();
    const double num = vessel.waterOffset + waterLevel;
    const float fixedDeltaTime = TimeWarp::fixedDeltaTime();
    centerOfBuoyancy = part->partTransform.position + part->partTransform.rotation * part->fields.CenterOfBuoyancy;
    centerOfDisplacement = part->partTransform.position + part->partTransform.rotation * part->fields.CenterOfDisplacement;
    depth = 0.0 - FlightGlobals::getAltitudeAtPos(Vector3d(centerOfDisplacement), body) + num;
    if (!shieldedFromAirstream && !part->DragCubes.None() && depth > 0.0 - PG::BuoyancyRange() && vessel.terrainAltitude < 20.0 && !isKerbal) {
        minDepth = std::numeric_limits<double>::max();
        maxDepth = std::numeric_limits<double>::lowest();
        FindOverrideCube();
        if (overrideCube != nullptr) {
            boundsCenterLocal = overrideCube->Center();
            boundsSize = overrideCube->Size();
        } else {
            boundsCenterLocal = part->DragCubes.WeightedCenter();
            boundsSize = part->DragCubes.WeightedSize();
        }
        boundsCenter = boundsCenterLocal;
        const Vector3d vector3d(BoundsMin(boundsCenterLocal, boundsSize));
        const Vector3d vector3d2(BoundsMax(boundsCenterLocal, boundsSize));
        const Matrix4x4f localToWorldMatrix = env.localToWorld(*part);
        boundsCenter = MultiplyPoint3x4(localToWorldMatrix, boundsCenter);
        // :263-366, the 8 corners in the C#'s order (x min / max outermost, then y, then z)
        for (int i = 0; i < 8; ++i) {
            const Vector3d c((i & 4) ? vector3d2.x : vector3d.x, (i & 2) ? vector3d2.y : vector3d.y, (i & 1) ? vector3d2.z : vector3d.z);
            boundCoords[static_cast<size_t>(i)] = F3(c);
            boundCoords[static_cast<size_t>(i)] = MultiplyPoint3x4(localToWorldMatrix, boundCoords[static_cast<size_t>(i)]);
            // (double)(0f - getAltitudeAtPos(Vector3, body)) + num: the float overload, negated in double
            depths[static_cast<size_t>(i)] = (0.0 - fx(FlightGlobals::getAltitudeAtPos(boundCoords[static_cast<size_t>(i)], body))) + num;
            if (depths[static_cast<size_t>(i)] > maxDepth) {
                maxDepth = depths[static_cast<size_t>(i)];
                maxCoords = Vector3d(boundCoords[static_cast<size_t>(i)]);
            }
            if (depths[static_cast<size_t>(i)] < minDepth) {
                minDepth = depths[static_cast<size_t>(i)];
                minCoords = Vector3d(boundCoords[static_cast<size_t>(i)]);
            }
        }
    } else {
        minDepth = (maxDepth = depth);
        minCoords = Vector3d(centerOfBuoyancy);
        maxCoords = Vector3d(centerOfBuoyancy);
    }
    part->fields.depth = depth;
    part->fields.minDepth = minDepth;
    part->fields.maxDepth = maxDepth;
    const Vector3f rbVelocity = env.rbVelocity(*part);
    const Vector3d frameVelocity = env.frameVelocity ? env.frameVelocity() : Vector3d();
    localVelocityMag = fx(rbVelocity.magnitude()) + frameVelocity.magnitude();
    downAxis = F3(-FlightGlobals::getUpAxis(*body, Vector3d(centerOfBuoyancy)));
    // (rb.velocity + frameVelocity): Vector3d's mixed operator; / (float)localVelocityMag: Vector3d / double; Vector3.Dot of the implicit Vector3
    const float num2 = (localVelocityMag == 0.0)
                           ? 1.0f
                           : Vector3f::Dot(F3((Vector3d(rbVelocity) + frameVelocity) / fx(static_cast<float>(localVelocityMag))), downAxis);
    const double num3 = VectorExtensions::IsZero(vessel.srf_vel_direction.ToVector3f())
                            ? 1.0
                            : fx(std::max(Mathf::Abs(num2), Mathf::Abs(Vector3f::Dot(F3(vessel.srf_vel_direction), downAxis))));
    const double num4 = PG::BuoyancyWaterDragScalarLerpDotMultBase() - num3 * PG::BuoyancyWaterDragScalarLerpDotMult();
    early = splashedCounter < PG::BuoyancyWaterDragTimer();
    allSplashed = splashed || vessel.Splashed;
    slow = localVelocityMag < PG::BuoyancyWaterDragMinVel() * PG::BuoyancyWaterDragMinVelMult();
    double num5 = PG::BuoyancyWaterDragScalar();
    if (early) num5 *= 0.5 + num3;
    if (splashed && !slow) {
        if (!early) {
            if (dragScalar > PG::BuoyancyWaterDragScalarEnd()) {
                dragScalar += (PG::BuoyancyWaterDragScalarEnd() - dragScalar) * PG::BuoyancyWaterDragScalarLerp() * num4 * fx(fixedDeltaTime);
                if (dragScalar < PG::BuoyancyWaterDragScalarEnd()) dragScalar = PG::BuoyancyWaterDragScalarEnd();
            } else {
                dragScalar = PG::BuoyancyWaterDragScalarEnd();
            }
            if (liftScalar < PG::BuoyancyWaterLiftScalarEnd()) {
                if (early) {
                    liftScalar = 0.0;
                } else {
                    liftScalar += (PG::BuoyancyWaterLiftScalarEnd() - liftScalar) * PG::BuoyancyWaterDragScalarLerp() * fx(fixedDeltaTime);
                    if (liftScalar > PG::BuoyancyWaterLiftScalarEnd()) liftScalar = PG::BuoyancyWaterLiftScalarEnd();
                }
            } else {
                liftScalar = PG::BuoyancyWaterLiftScalarEnd();
            }
        }
    } else {
        if (dragScalar < num5) {
            dragScalar += (num5 - dragScalar) * PG::BuoyancyWaterDragScalarLerp() * fx(fixedDeltaTime);
            if (dragScalar > num5) dragScalar = num5;
        } else {
            dragScalar = num5;
        }
        if (liftScalar > 0.0) {
            liftScalar -= liftScalar * PG::BuoyancyWaterDragScalarLerp() * fx(fixedDeltaTime);
            if (liftScalar < 0.0) liftScalar = 0.0;
        }
    }
    if (splashed) {
        if (early) splashedCounter += fx(fixedDeltaTime);
    } else {
        splashedCounter = 0.0;
    }
    if (!shieldedFromAirstream && maxDepth > 0.0 && part->State() != PartStates::DEAD) {
        UpdateDisplacement(env);
        if (!splashed) {
            pLast = centerOfBuoyancy - (env.frameVelocity ? env.frameVelocity() : Vector3d()).ToVector3f() * fixedDeltaTime;
            V = centerOfBuoyancy - pLast;
            dH = Vector3f::Dot(V, downAxis);
            const float num6 = f32store(fx(num2) * fx(static_cast<float>(localVelocityMag)));
            const Vector3d camera = env.cameraPosition ? env.cameraPosition() : Vector3d(1.0e30, 0.0, 0.0);
            if (localVelocityMag > 2.0 && Vector3f::Distance(part->partTransform.position, camera.ToVector3f()) < 5000.0f) {
                const double num7 = env.waterLevelOffset ? env.waterLevelOffset(*body) : 0.0;
                lastSplashFx = true;
                lastSplashFxPosition = centerOfBuoyancy + F3(Vector3d(downAxis) * (maxDepth - depth - fx(dH) + num7));
                lastSplashFxPower = f32store(fx(num6) * fx(0.1f) * fx(maxDimension));
                if (env.splashFx) env.splashFx(lastSplashFxPosition, lastSplashFxPower);
            }
            splashed = true;
            const float crashSpeed = Mathf::Max(f32store(fx(PG::BuoyancyMinCrashMult()) * fx(static_cast<float>(localVelocityMag))), num6);
            if (fx(crashSpeed) > fx(part->fields.crashTolerance) * fx(PG::BuoyancyCrashToleranceMult())) {
                EventReport r(FlightEvents::SPLASHDOWN_CRASH, part, part->partInfo != nullptr ? part->partInfo->title : std::string());
                GE::onCrashSplashdown.Fire(&r);
                lastCrash = true;
                log::Info("[PartBuoyancy]: Part " + r.sender + " splashed at " + std::to_string(localVelocityMag) + " m/s total, " + std::to_string(num6) +
                          " down");
                dead = true;
            } else {
                invokeIn = 2.0;  // if (IsInvoking()) CancelInvoke(); Invoke("ReportSplashDown", 2f)
            }
        }
        part->fields.WaterContact = true;
        vessel.Splashed = true;
        submergedPortion = 1.0;
        Vector3f vector = centerOfBuoyancy;
        if (minDepth < 0.0 && minDepth < maxDepth) {
            if (depth <= 0.0 && depth != maxDepth) submergedPortion = maxDepth / (maxDepth - depth) * 0.5;
            else submergedPortion = 0.5 + depth / (depth - minDepth) * 0.5;
            if (part->fields.buoyancyUseSine) {
                submergedPortion *= 3.14159265358979323846;
                submergedPortion -= 3.14159265358979323846 / 2.0;
                submergedPortion = dotnet::Math::Sin(submergedPortion) * 0.5 + 0.5;
            }
            if (PG::BuoyancyUseCoBOffset()) {
                Vector3f zero;
                float num8 = 0.0f;
                for (int num9 = 7; num9 >= 0; num9--) {
                    const float num10 = static_cast<float>(depths[static_cast<size_t>(num9)]);
                    if (num10 > 0.0f) {
                        zero += boundCoords[static_cast<size_t>(num9)] * num10;
                        num8 = f32store(fx(num8) + fx(num10));
                    }
                }
                zero /= num8;
                vector = centerOfBuoyancy + (zero + part->partTransform.rotation * part->fields.CenterOfBuoyancy - centerOfBuoyancy) *
                                                static_cast<float>(1.0 - submergedPortion);
            }
        }
        part->fields.submergedPortion = submergedPortion;
        buoyantGeeForce = displacement * submergedPortion * body->oceanDensity *
                          ((maxDepth < PG::BuoyancyScaleAboveDepth()) ? (maxDepth / PG::BuoyancyScaleAboveDepth()) : 1.0) * PG::BuoyancyScalar() *
                          fx(part->fields.buoyancy);
        effectiveForce = F3(-FlightGlobals::getGeeForceAtPosition(Vector3d(vector)) * buoyantGeeForce * vessel.gravityMultiplier *
                            PG::GraviticForceMultiplier);
        if (early && !wasSplashed)
            dragScalar = std::min(dragScalar, (PG::BuoyancyWaterDragMultMinForMinDot() + num3 * (1.0 - PG::BuoyancyWaterDragMultMinForMinDot())) *
                                                      (num5 - PG::BuoyancyWaterDragScalarEnd()) +
                                                  PG::BuoyancyWaterDragScalarEnd());
        if (localVelocityMag < PG::BuoyancyWaterDragMinVel() * PG::BuoyancyWaterDragMinVelMultCOBOff())
            vector = Vector3f::Lerp(lastForcePosition + V, vector, PG::BuoyancyForceOffsetLerp());
        if (!(localVelocityMag < PG::BuoyancyWaterDragMinVel()) && localVelocityMag <= vessel.srfSpeed * PG::BuoyancyWaterDragPartVelGreaterVesselMult()) {
            if (!isKerbal) {
                part->fields.angularDragByFI = true;
                env.setRbAngularDrag(*part, 0.0f);
                const double num11 = part->DragCubes.None() ? 1.0 : fx(part->DragCubes.AreaDrag());
                const double num12 = 0.5 * body->oceanDensity * localVelocityMag * num11 * fx(PG::DragCubeMultiplier()) * dragScalar * submergedPortion *
                                     fx(PG::DragMultiplier());
                const float mass = env.rbMass(*part);
                env.setRbDrag(*part, static_cast<float>(num12 / fx(mass != 0.0f ? mass : 0.1f)));
            } else {
                env.setRbDrag(*part, PG::BuoyancyWaterDragSlow());
            }
        } else {
            if (!isKerbal) {
                const double num13 = vessel.ctrlState.pitch;
                const double num14 = vessel.ctrlState.yaw;
                const double num15 = vessel.ctrlState.roll;
                if (num13 * num13 + num14 * num14 + num15 * num15 < PG::BuoyancyAngularDragMinControlSqrMag()) {
                    part->fields.angularDragByFI = false;
                    env.setRbAngularDrag(*part, PG::BuoyancyWaterAngularDragSlow());
                } else {
                    part->fields.angularDragByFI = true;
                    env.setRbAngularDrag(*part, 0.0f);
                }
            }
            env.setRbDrag(*part, PG::BuoyancyWaterDragSlow());
        }
        physWarp = TimeWarp::CurrentRate() > 1.0f && TimeWarp::WarpMode() == TimeWarp::Modes::LOW;
        if (isKerbal && physWarp) {
            env.setRbDrag(*part, f32store(fx(env.rbDrag(*part)) + fx(fixedDeltaTime) * 6.25));
            if (TimeWarp::CurrentRate() > 2.0f)
                effectiveForce = Vector3f::Lerp(lastBuoyantForce, effectiveForce, f32store(1.25 - fx(TimeWarp::CurrentRate()) * 0.25));
        }
        if (early) {
            if (physWarp && !isKerbal) {
                env.setRbDrag(*part, f32store(fx(env.rbDrag(*part)) +
                                              fx(static_cast<float>((dragScalar - PG::BuoyancyWaterDragScalarEnd()) * fx(fixedDeltaTime) * 50.0))));
                if (TimeWarp::CurrentRate() > 2.0f)
                    effectiveForce = Vector3f::Lerp(lastBuoyantForce, effectiveForce, f32store(1.25 - fx(TimeWarp::CurrentRate()) * 0.25));
            } else if (num3 > PG::BuoyancyWaterDragExtraRBDragAboveDot()) {
                env.setRbDrag(*part, f32store(fx(env.rbDrag(*part)) + fx(static_cast<float>(num3 - PG::BuoyancyWaterDragExtraRBDragAboveDot()))));
            }
        }
        if (VectorExtensions::IsInvalid(effectiveForce)) effectiveForce = Vector3f();
        lastBuoyantForce = effectiveForce;
        lastForcePosition = vector;
        if (dead) {
            if (PG::BuoyancyApplyForceOnDie() && part->parent != nullptr) {
                env.addForceAtPosition(*part->parent, Vector3d(effectiveForce), Vector3d(vector));
                lastApplied = true;
                lastAppliedForce = Vector3d(effectiveForce);
                lastAppliedPosition = Vector3d(vector);
            }
            part->Die();
        } else {
            env.addForceAtPosition(*part, Vector3d(effectiveForce), Vector3d(vector));
            lastApplied = true;
            lastAppliedForce = Vector3d(effectiveForce);
            lastAppliedPosition = Vector3d(vector);
        }
    } else {
        invokeIn = -1.0;  // if (IsInvoking()) CancelInvoke()
        part->fields.WaterContact = false;
        if (splashed) {
            vessel.checkSplashed();
            splashed = false;
            env.setRbDrag(*part, 0.0f);  // GetComponentCached(ref _rigidbody).drag = 0f
        }
        part->fields.submergedPortion = 0.0;
        submergedPortion = 0.0;
        drag = 0.0;
        lastBuoyantForce = Vector3f();
        lastForcePosition = centerOfBuoyancy;
    }
    part->fields.submergedDragScalar = dragScalar;
    part->fields.submergedLiftScalar = liftScalar;
    wasSplashed = splashed;
}

}  // namespace ksp
