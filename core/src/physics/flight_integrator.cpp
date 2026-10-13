// WP-20d: FlightIntegrator over the rigid bodies; see include/ksp/physics/flight_integrator.hpp. C# paths are relative to
// decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/physics/flight_integrator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/aero/float_eval.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "ksp/physics/physical_object.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/unity/matrix_native.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::physics {

using aero::MonoFloat;

void AeroKernelStats::Add(const AeroKernelStats& o) {
    ticks += o.ticks;
    partTicks += o.partTicks;
    compared += o.compared;
    mismatches += o.mismatches;
    covered += o.covered;
    if (firstMismatch.empty()) firstMismatch = o.firstMismatch;
}

// Part.cs:1663 Start, its flight-scene statements the tick depends on
void StartPartsInFlight(Vessel& vessel) {
    for (Part* p : vessel.parts) {
        Part::StartContext ctx;
        ctx.flight = true;
        ctx.standardSpecificHeatCapacity = PhysicsGlobals::StandardSpecificHeatCapacity();
        ctx.isRootOfVessel = vessel.rootPart == p;
        // :1825-1828 FlightGlobals.getExternalTemperature(getAltitudeAtPos(partTransform.position, mainBody), mainBody)
        if (CelestialBody* body = vessel.mainBody()) {
            const double alt = FlightGlobals::getAltitudeAtPos(Vector3d(p->partTransform.position), body);
            ctx.externalTemperature = FlightGlobals::getExternalTemperature(alt, body);
        }
        p->StartData(ctx);
        // :1857-1866
        p->fields.hasLiftModule = false;
        size_t count2 = p->Modules().Count();
        while (count2-- > 0) {
            const auto* lift = dynamic_cast<const ILiftProvider*>(p->Modules().GetModule(count2));
            if (lift != nullptr && lift->DisableBodyLift()) {
                p->fields.hasLiftModule = true;
                break;
            }
        }
        p->CheckBodyLiftAttachment();  // :1867
    }
    StartPartRigidbodies(vessel);  // :1868-1885
}

FlightIntegrator::FlightIntegrator(Vessel& v) {
    vessel = &v;
    // FlightIntegrator.cs:400 HookVesselEvents
    using GE = events::GameEvents;
    GE::onPartAttach.Add(this, &FlightIntegrator::OnPartEventTargetAction);
    GE::onPartRemove.Add(this, &FlightIntegrator::OnPartEventTargetAction);
    GE::onPartCouple.Add(this, &FlightIntegrator::OnPartEventFromToAction);
    GE::onPartDie.Add(this, &FlightIntegrator::OnPartEvent);
    GE::onPartUndock.Add(this, &FlightIntegrator::OnPartEvent);
    GE::onVesselWasModified.Add(this, &FlightIntegrator::OnVesselEvent);
    // part.partTransform.localToWorldMatrix of the occlusion passes: the part transform is a world transform here (Unity composes the
    // hierarchy's local matrices natively; the TRS of the world pose is that up to float rounding)
    localToWorld = [](const Part& p) {
        return UnityNative::TRS(p.partTransform.position, p.partTransform.rotation, p.partTransform.localScale);
    };
}

// FlightIntegrator.cs:410 UnhookVesselEvents
FlightIntegrator::~FlightIntegrator() {
    using GE = events::GameEvents;
    GE::onPartAttach.Remove(this, &FlightIntegrator::OnPartEventTargetAction);
    GE::onPartRemove.Remove(this, &FlightIntegrator::OnPartEventTargetAction);
    GE::onPartCouple.Remove(this, &FlightIntegrator::OnPartEventFromToAction);
    GE::onPartDie.Remove(this, &FlightIntegrator::OnPartEvent);
    GE::onPartUndock.Remove(this, &FlightIntegrator::OnPartEvent);
    GE::onVesselWasModified.Remove(this, &FlightIntegrator::OnVesselEvent);
    if (ActiveVesselFI == this) ActiveVesselFI = nullptr;
}

// FlightIntegrator.cs:420
void FlightIntegrator::OnPartEvent(Part* part) {
    if (part != nullptr && part->vessel == vessel) recreateThermalGraph = true;
}

// FlightIntegrator.cs:428
void FlightIntegrator::OnVesselEvent(Vessel* v) {
    if (vessel == v) recreateThermalGraph = true;
}

// FlightIntegrator.cs:436 (the C#'s second test reads data.target.vessel behind a host null check: a null target is skipped here)
void FlightIntegrator::OnPartEventTargetAction(events::GameEvents::HostTargetAction<Part*, Part*> data) {
    if ((data.host != nullptr && data.host->vessel == vessel) || (data.host != nullptr && data.target != nullptr && data.target->vessel == vessel))
        recreateThermalGraph = true;
}

// FlightIntegrator.cs:444
void FlightIntegrator::OnPartEventFromToAction(events::GameEvents::FromToAction<Part*, Part*> data) {
    if ((data.from != nullptr && data.from->vessel == vessel) || (data.to != nullptr && data.to->vessel == vessel)) recreateThermalGraph = true;
}

// ===================================================================================== Setup

// FlightIntegrator.cs:235
void FlightIntegrator::Setup() {
    partRef = vessel != nullptr ? vessel->rootPart : nullptr;  // :238-239 GetComponent<Vessel>() / GetComponent<Part>()
    atm.staticPressureAtm = 1.0;
    atm.staticPressurekPa = 101.325;
    atm.dynamicPressurekPa = 0.0;
    atm.altitude = 0.0;
    atm.currentMainBody = nullptr;
    atm.atmosphericTemperature = 0.0;
    atm.externalTemperature = 0.0;
    // :247-254 FlightIntegrator.sunBody (static)
    sunBody = (Planetarium::fetch != nullptr && Planetarium::fetch->Sun != nullptr)
                  ? Planetarium::fetch->Sun
                  : (FlightGlobals::Bodies().empty() ? nullptr : FlightGlobals::Bodies()[0]);
    if (vessel == nullptr || partRef == nullptr || !partRef->fields.started) return;  // :255
    setupRun = true;
    VelSmoothLenRecip = 1.0 / static_cast<double>(VelSmoothLen);
    int velSmoothLen = VelSmoothLen;
    while (velSmoothLen-- > 0) VelSmoother[static_cast<size_t>(velSmoothLen)] = Vector3f::zero();
    isKerbal = partRef->FindModuleOfClass("KerbalEVA") != nullptr;  // :267 Modules.Contains("KerbalEVA")
    partCount = static_cast<int>(vessel->parts.size());
    int index = partCount;
    while (index-- > 0) {
        Part& part = *vessel->parts[static_cast<size_t>(index)];
        part.fields.resourceMass = part.GetResourceMass(part.fields.resourceThermalMass);
        // :277-294 asteroids and comets (ModuleAsteroid / ModuleComet with forceProceduralDrag)
        for (const char* cls : {"ModuleAsteroid", "ModuleComet"}) {
            if (PartModule* m = part.FindModuleOfClass(cls)) {
                if (m->Fields()["forceProceduralDrag"] != nullptr && m->Get<bool>("forceProceduralDrag")) part.DragCubes.SetProcedural(true);
                part.DragCubes.ForceUpdate(true, true, true);
                part.DragCubes.SetDragWeights<MonoFloat>();
            }
        }
        if (part.DragCubes.None()) continue;
        if (&part == vessel->rootPart) SetupReloadRootCubes(part);
        part.fields.radiativeArea = static_cast<double>(part.DragCubes.PostOcclusionArea());
    }
    cacheStandardSpecificHeatCapacity = PhysicsGlobals::StandardSpecificHeatCapacity();
    UpdateMassStats();
    firstFrame = true;
    needOcclusion = true;
    recreateThermalGraph = true;
}

// FlightIntegrator.cs:235 Setup, its lines 299-363: a root part whose six weighted areas are all zero gets its cubes reloaded (the EVA
// cube for a kerbal, the prefab's cubes otherwise)
void FlightIntegrator::SetupReloadRootCubes(Part& part) {
    auto zeroed = [&part] {
        bool flag = true;
        int num = 5;
        while (num >= 0) {
            if (part.DragCubes.WeightedArea()[static_cast<size_t>(num)] <= 0.0f) {
                num--;
                continue;
            }
            flag = false;
            break;
        }
        return flag;
    };
    if (!zeroed() || part.DragCubes.Procedural()) return;
    if (part.FindModuleOfClass("KerbalEVA") != nullptr) {
        if (auto node = PhysicsGlobals::KerbalEVADragCube()) part.DragCubes.LoadCubes(*node);
        return;
    }
    if (part.partInfo == nullptr || !part.partInfo->partPrefab) return;
    Part& partPrefab = *part.partInfo->partPrefab;
    if (!partPrefab.DragCubes.None()) {
        while (part.DragCubes.Cubes().size() < partPrefab.DragCubes.Cubes().size()) part.DragCubes.Cubes().emplace_back();
        for (size_t i = 0; i < partPrefab.DragCubes.Cubes().size(); i++)
            part.DragCubes.Cubes()[i].Load(dotnet::Split(partPrefab.DragCubes.Cubes()[i].SaveToString(), ", ", true));
        part.DragCubes.ForceUpdate(true, true);
        part.DragCubes.SetDragWeights<MonoFloat>();
        part.DragCubes.SetPartOcclusion<MonoFloat>();
        size_t count = part.children.size();
        while (count-- > 0)
            if (!part.children[count]->DragCubes.None()) part.children[count]->DragCubes.SetPartOcclusion<MonoFloat>();
        // Debug.Log("[FlightIntegrator]: Reloaded drag cube for zeroed cube root part ...")
    }
    // :345-360: still zeroed -> Debug.LogError (no behaviour)
}

// FlightIntegrator.cs:381
void FlightIntegrator::OnLoadVessel() {
    partRef = vessel != nullptr ? vessel->rootPart : nullptr;
    if (!setupRun && partRef != nullptr && partRef->fields.started) Setup();
}

// ===================================================================================== mass and velocity

// FlightIntegrator.cs:452: the thermal loop (WP-22), the physicsMass loop (WP-14's Vessel::UpdateMassStats: the same statements; it
// recomputes resourceMass, which the thermal loop just set to the same value), then rb.mass / rb.centerOfMass (:474-486, not while
// packed) through the binding
void FlightIntegrator::UpdateMassStats() {
    UpdateThermalMassStats();
    vessel->UpdateMassStats();
    if (vp_ != nullptr && !vessel->packed) vp_->UpdateMassProperties();
}

// FlightIntegrator.cs:700
float FlightIntegrator::GetPhysicslessChildMass(const Part& part) const {
    using E = aero::FE<MonoFloat>;
    float num = 0.0f;
    size_t count = part.children.size();
    while (count-- > 0) {
        const Part& part2 = *part.children[count];
        if (!part2.hasRigidbody)
            num = aero::S(E(num) + (E(part2.fields.mass) + E(part2.fields.resourceMass) + E(GetPhysicslessChildMass(part2))));
    }
    return num;
}

// FlightIntegrator.cs:495
void FlightIntegrator::SmoothVelocity() {
    VelUnsmoothed = Vel;
    int num = VelIndex - 1;
    if (num < 0) num = VelSmoothLen - 1;
    const Vector3f vector = VelSmoother[static_cast<size_t>(num)];
    if (static_cast<double>((Vel - vector).sqrMagnitude()) > maxVelDeltaSqr * fDeltaTime * fDeltaTime &&
        Vel.sqrMagnitude() > vector.sqrMagnitude()) {
        if (VelSpiking) {
            VelSmoother[static_cast<size_t>(VelIndex++)] = Vel;
        } else if (lastVelProvisional) {
            VelSmoother[static_cast<size_t>(VelIndex++)] = Vel;
            VelSmoother[static_cast<size_t>(lastVelIndex)] = lastVel;
            VelSpiking = true;
        } else {
            lastVelProvisional = true;
            lastVel = Vel;
            lastVelIndex = VelIndex;
            VelSmoother[static_cast<size_t>(VelIndex++)] = vector;
        }
    } else {
        VelSmoother[static_cast<size_t>(VelIndex++)] = Vel;
        VelSpiking = false;
        lastVelProvisional = false;
    }
    if (VelIndex >= VelSmoothLen) VelIndex = 0;
    Vector3d zero;
    int velSmoothLen = VelSmoothLen;
    while (velSmoothLen-- > 0) zero += Vector3d(VelSmoother[static_cast<size_t>(velSmoothLen)]);
    zero *= VelSmoothLenRecip;
    Vel = zero.ToVector3f();
}

// ===================================================================================== the sun (ThermoPrecalculate)

// The scene state of CalculateSunBodyFlux (:798-835) without Unity's scaled space: positions are the world positions times
// ScaledSpace.InverseScaleFactor (ScaledSpace.cs:120 LocalToScaledSpace with a zero totalOffset; only differences enter), the scaled
// bodies' transforms the bodies' positions / rotations, and Physics.Raycast(ray, float.MaxValue, "Scaled Scenery") an analytic ray /
// sphere test against every body but the sun (a hit on the sun's own collider gives the same distance as no hit, :822 vs :829).
SunBodyFluxInput FlightIntegrator::DefaultSunBodyFlux() const {
    SunBodyFluxInput in;
    const float scale = 6000.0f;  // ScaledSpace.scaleFactor (ScaledSpace.cs:6)
    const float inv = 1.0f / scale;
    in.frame.scaleFactor = scale;
    const CelestialBody* body = atm.currentMainBody;
    if (vessel == nullptr || sunBody == nullptr || body == nullptr) return in;
    in.integratorScaledPos = Vector3d(vessel->vesselTransform.position) * static_cast<double>(inv);
    in.frame.sunScaledPos = (sunBody->position * static_cast<double>(inv)).ToVector3f();
    in.frame.scaledBodyPos = (body->position * static_cast<double>(inv)).ToVector3f();
    in.frame.bodyUp = (body->rotation * Vector3d(0.0, 1.0, 0.0)).ToVector3f();
    const Vector3d o = in.integratorScaledPos;
    const Vector3d toSun = Vector3d(in.frame.sunScaledPos) - o;
    const double dist = toSun.magnitude();
    if (!(dist > 0.0)) return in;
    const Vector3d d = toSun / dist;
    for (const CelestialBody* b : FlightGlobals::Bodies()) {
        if (b == nullptr || b == sunBody) continue;
        const Vector3d c = b->position * static_cast<double>(inv) - o;
        const double r = b->Radius * static_cast<double>(inv);
        const double t = Vector3d::Dot(c, d);
        const double h2 = c.sqrMagnitude() - t * t;
        if (h2 > r * r) continue;
        const double th = std::sqrt(r * r - h2);
        const double t0 = t - th;
        if (t0 > 0.0 && t0 < dist) {
            in.raycastHit = true;
            in.hitIsSunBody = false;
            in.hitDistance = static_cast<float>(t0);
            break;
        }
    }
    return in;
}

// FlightIntegrator.cs:695
void FlightIntegrator::ThermoPrecalculate() { ThermoPrecalculate(sunBodyFlux ? sunBodyFlux(*this) : DefaultSunBodyFlux()); }

// ===================================================================================== FixedUpdate

// FlightIntegrator.cs:544
void FlightIntegrator::FixedUpdate(VesselPhysics* binding) {
    vp_ = binding;
    if (!setupRun) {
        Setup();
        if (!setupRun) return;
    }
    fDeltaTime = static_cast<double>(TimeWarp::fixedDeltaTime());
    fDeltaTimeRecip = 1.0 / fDeltaTime;
    if (vessel == FlightGlobals::ActiveVessel) ActiveVesselFI = this;
    isRunning = false;
    if (!FlightGlobals::ready) return;
    if (partRef == nullptr) partRef = vessel->rootPart;
    if (partRef == nullptr || !partRef->hasRigidbody) return;  // :569 !partRef.rb
    // :573-593
    aero.CacheGlobals();
    CacheGlobals();
    isRunning = true;
    const int count = static_cast<int>(vessel->parts.size());
    if (partCount != count) {
        partCount = count;
        recreateThermalGraph = true;
    }
    universalTime = Planetarium::GetUniversalTime();
    timeWarpRate = TimeWarp::CurrentRate();
    rbVeld = vessel->rb_velocityD;
    Veld = vessel->velocityD;
    Vel = Veld.ToVector3f();
    CoMd = vessel->CoMD;
    CoM = vessel->CoM;
    UpdateMassStats();
    SmoothVelocity();
    atm.spd = vessel->IgnoreSpeedActive() ? 0.0 : static_cast<double>(Vel.magnitude());
    nVel = (atm.spd != 0.0) ? (Vel / static_cast<float>(atm.spd)) : Vector3f::zero();
    atm.currentMainBody = vessel->mainBody();
    atm.situation = vessel->situation;
    GraviticAcceleration = vessel->graviticAcceleration.ToVector3f();
    atm.altitude = FlightGlobals::getAltitudeAtPos(CoMd, const_cast<CelestialBody*>(atm.currentMainBody));
    atm.CalculatePressure();
    vessel->speed = atm.spd;
    vessel->staticPressurekPa = atm.staticPressurekPa;
    vessel->indicatedAirSpeed = atm.spd * atm.staticPressureAtm;
    if (sunBody != nullptr)
        vessel->distanceToSun = static_cast<double>((vessel->vesselTransform.position - sunBody->position.ToVector3f()).magnitude());
    if (firstFrame) {
        firstFrame = false;
        atm.density = vessel->atmDensity;
        ThermoPrecalculate();
    }
    if (atm.staticPressurekPa > 0.0) {
        // :893 CalculateConstantsAtmosphere and its vessel mirrors
        atm.CalculateConstantsAtmosphere();
        vessel->solarFlux = atm.solarFlux;
        vessel->atmosphericTemperature = atm.atmosphericTemperature;
        vessel->atmDensity = atm.density;
        vessel->dynamicPressurekPa = atm.dynamicPressurekPa;
        vessel->speedOfSound = atm.speedOfSound;
        vessel->mach = atm.mach;
        vessel->externalTemperature = atm.externalTemperature;
        vessel->convectiveCoefficient = atm.convectiveCoefficient;
    } else {
        // :861 CalculateConstantsVacuum and its vessel mirrors
        atm.CalculateConstantsVacuum();
        vessel->atmosphericTemperature = atm.atmosphericTemperature;
        vessel->externalTemperature = atm.externalTemperature;
        vessel->convectiveCoefficient = 0.0;
        vessel->convectiveMachFlux = 0.0;
        vessel->atmDensity = 0.0;
        vessel->dynamicPressurekPa = 0.0;
        vessel->mach = 0.0;
        vessel->speedOfSound = 0.0;
        vessel->solarFlux = atm.solarFlux;
    }
    atm.densityThermalLerp = atm.CalculateDensityThermalLerp();
    backgroundRadiationTemp = atm.CalculateBackgroundRadiationTemperature(atm.atmosphericTemperature);
    backgroundRadiationTempExposed = atm.CalculateBackgroundRadiationTemperature(atm.externalTemperature);
    // :642 DragCubeSetupAndPartAeroStats(vessel): the aero half reads the FlightIntegrator's mach / density
    aero.currentMainBody = atm.currentMainBody;
    aero.mach = atm.mach;
    aero.density = atm.density;
    aero.pseudoReDragMult = atm.pseudoReDragMult;
    {
        std::vector<Part*> parts(vessel->parts.begin(), vessel->parts.begin() + std::min<size_t>(static_cast<size_t>(partCount), vessel->parts.size()));
        CelestialBody* body = const_cast<CelestialBody*>(atm.currentMainBody);
        aero.DragCubeSetupAndPartAeroStats<MonoFloat>(parts, vessel->gravityTrue.magnitude(), [body](const Part& p) {
            return FlightGlobals::getAltitudeAtPos(Vector3d(p.partTransform.position), body);
        });
    }
    CheckThermalGraph();  // :643
    if (needOcclusion) UpdateOcclusion(true);
    UpdateThermodynamics();
    if (vessel->packed || vp_ == nullptr) {  // :649 partRef.packed (a loaded vessel without bodies cannot integrate either)
        ClearPartBuffers();
        return;
    }
    if (FlightGlobals::ActiveVessel == vessel && !FlightGlobals::physicalObjects.empty())
        IntegratePhysicalObjects(FlightGlobals::physicalObjects, atm.density);  // :661
    if (!vessel->precalc.preIntegrationVelocityOffset.IsZero()) vessel->OffsetVelocity(vessel->precalc.preIntegrationVelocityOffset);
    Integrate(*vp_);  // :669
    ++fixedUpdates_;
}

// FlightIntegrator.cs:544 FixedUpdate, its packed branch (lines 649-660)
void FlightIntegrator::ClearPartBuffers() {
    size_t count2 = vessel->parts.size();
    while (count2-- > 0) {
        Part& part = *vessel->parts[count2];
        part.forces.clear();
        part.fields.force = Vector3d();
        part.fields.torque = Vector3d();
    }
}

// ===================================================================================== Integrate

AeroRigidbody FlightIntegrator::RigidbodyOf(Part& part) const {
    AeroRigidbody rb;
    Part* owner = RigidBodyPart(&part);  // part.Rigidbody: the part's rb, or its parent's for a physicsless part (Part.cs:1184)
    rb.present = owner != nullptr && vp_ != nullptr && vp_->PartIndex(*owner) >= 0;
    rb.isPartRb = part.hasRigidbody;
    rb.partHasRb = part.hasRigidbody;
    rb.partHasServoRb = false;
    rb.krakensbaneFrameVelocity = Krakensbane::GetFrameVelocity();
    if (rb.present) {
        const PartRigidbodyState s = vp_->RigidbodyState(*owner);
        rb.velocity = s.velocity;
        rb.worldCenterOfMass = s.worldCenterOfMass;
    }
    return rb;
}

// SoA / Both: UpdateAerodynamics of every part of the Integrate order before the recursion
void FlightIntegrator::AeroBatch() {
    const size_t n = order_.size();
    if (kernel == AeroKernel::SoA) {
        soa_.UpdateAerodynamics(aero, order_, rbs_, aeroOut_);
        aeroStats.covered += soa_.CoveredParts();
        return;
    }
    // Both: the exact port on the previous tick's cube data, then the SoA kernel on the same inputs
    prevCube_.resize(n);
    exactOut_.resize(n);
    exactSnap_.resize(n);
    for (size_t i = 0; i < n; ++i) prevCube_[i] = order_[i]->DragCubes.GetCubeData();
    for (size_t i = 0; i < n; ++i) {
        exactOut_[i] = aero.UpdateAerodynamics<MonoFloat>(*order_[i], rbs_[i]);
        exactSnap_[i] = AeroPartOutputs::Capture(*order_[i], exactOut_[i]);
    }
    for (size_t i = 0; i < n; ++i) order_[i]->DragCubes.SetCubeData(prevCube_[i]);
    soa_.UpdateAerodynamics(aero, order_, rbs_, aeroOut_);
    aeroStats.covered += soa_.CoveredParts();
    for (size_t i = 0; i < n; ++i) {
        const AeroPartOutputs s = AeroPartOutputs::Capture(*order_[i], aeroOut_[i]);
        ++aeroStats.compared;
        std::string what;
        if (!exactSnap_[i].BitEqual(s, &what)) {
            ++aeroStats.mismatches;
            if (aeroStats.firstMismatch.empty())
                aeroStats.firstMismatch = order_[i]->partInfo != nullptr ? order_[i]->partInfo->name : std::string("?");
            if (aeroStats.firstMismatch.find(" tick ") == std::string::npos)
                aeroStats.firstMismatch += " tick " + std::to_string(aeroStats.ticks) + ": " + what;
        }
    }
    aeroOut_ = exactOut_;  // the exact results go to the bodies
}

// FlightIntegrator.cs:715 Integrate(part), from partRef: depth first, children in order, attached children only
void FlightIntegrator::Integrate(VesselPhysics& binding) {
    vp_ = &binding;
    if (partRef == nullptr) partRef = vessel->rootPart;
    if (partRef == nullptr) return;
    order_.clear();
    std::vector<Part*> stack{partRef};
    while (!stack.empty()) {
        Part* part = stack.back();
        stack.pop_back();
        order_.push_back(part);
        for (auto it = part->children.rbegin(); it != part->children.rend(); ++it)
            if ((*it)->isAttached()) stack.push_back(*it);
    }
    const size_t n = order_.size();
    rbs_.resize(n);
    for (size_t i = 0; i < n; ++i) rbs_[i] = RigidbodyOf(*order_[i]);
    ++aeroStats.ticks;
    aeroStats.partTicks += n;
    if (kernel != AeroKernel::Exact) AeroBatch();
    // :719 / :730 rb.AddForce(integrationAccel, ForceMode.Acceleration) on every Rigidbody: the uniform acceleration of the bodies
    bool any = false;
    for (Part* p : order_) any = any || p->hasRigidbody;
    if (any) vp_->AddUniformAcceleration(vessel->precalc.integrationAccel);
    for (size_t i = 0; i < n; ++i) {
        Part& part = *order_[i];
        if (part.hasRigidbody) {
            vp_->AddRigidbodyForce(part, part.fields.force, ForceMode::Force);  // :720 rb.AddForce(part.force)
            vp_->AddTorque(part, part.fields.torque);                           // :721 rb.AddTorque(part.torque)
            size_t count = part.forces.size();
            while (count-- > 0)  // :722-726 rb.AddForceAtPosition(forces[count].force, forces[count].pos)
                vp_->AddForceAtPosition(part, part.forces[count].force, part.forces[count].pos);
        }
        part.forces.clear();  // :732-734
        part.fields.force = Vector3d();
        part.fields.torque = Vector3d();
        // :735 UpdateAerodynamics(part): drag / lift on part.Rigidbody, rb.angularDrag
        const AeroForces f = kernel == AeroKernel::Exact ? aero.UpdateAerodynamics<MonoFloat>(part, rbs_[i]) : aeroOut_[i];
        ApplyAeroForces(*vp_, part, f);
    }
}

// FlightIntegrator.cs:747 IntegratePhysicalObjects, its acceleration (line 752)
Vector3f FlightIntegrator::PhysicalObjectGravity() const {
    const CelestialBody& body = *atm.currentMainBody;
    return ((FlightGlobals::getGeeForceAtPosition(CoMd, body) + FlightGlobals::getCoriolisAcc(Veld, body) +
             FlightGlobals::getCentrifugalAcc(CoMd, body)) *
            PhysicsGlobals::GraviticForceMultiplier)
        .ToVector3f();
}

// FlightIntegrator.cs:747 IntegratePhysicalObjects, its drag (lines 767-780; `rb.velocity + Krakensbane.GetFrameVelocity()`: a float
// velocity plus the double frame velocity)
bool FlightIntegrator::PhysicalObjectDrag(const Vector3f& rbVelocity, const Vector3d& frameVelocity, float origDrag, double atmDensity,
                                          Vector3f& drag) const {
    if (!PhysicsGlobals::ApplyDrag() || !(atmDensity > 0.0)) return false;
    const Vector3d v = Vector3d(rbVelocity) + frameVelocity;
    const double num = static_cast<double>(origDrag);
    const double num2 = 0.0005 * atm.pseudoReDragMult * atmDensity * num * v.sqrMagnitude() * static_cast<double>(PhysicsGlobals::DragMultiplier());
    if (std::isnan(num2) || std::isinf(num2)) return false;
    drag = (-v.normalized() * num2).ToVector3f();
    return true;
}

// FlightIntegrator.cs:747
void FlightIntegrator::IntegratePhysicalObjects(const std::vector<physicalObject*>& pObjs, double atmDensity) {
    const Vector3f force = PhysicalObjectGravity();
    size_t count = pObjs.size();
    while (count-- > 0) {
        physicalObject* o = pObjs[count];
        if (o == nullptr || !o->HasRigidbody()) continue;
        const double mass = static_cast<double>(o->world->GetMass(o->rb));
        o->world->AddForce(o->rb, Vector3d(force) * mass);  // :762 rb.AddForce(force, ForceMode.Acceleration)
        Vector3f zero;
        if (!PhysicalObjectDrag(o->world->GetLinearVelocity(o->rb).ToVector3f(), Krakensbane::GetFrameVelocity(), o->origDrag, atmDensity, zero))
            continue;
        if (PhysicsGlobals::DragUsesAcceleration()) o->world->AddForce(o->rb, Vector3d(zero) * mass);  // :774 Acceleration
        else o->world->AddForce(o->rb, Vector3d(zero));                                             // :778 Force
    }
}

// ===================================================================================== Update

// FlightIntegrator.cs:672
void FlightIntegrator::Update() {
    if (!FlightGlobals::ready) return;
    if (!setupRun) {
        Setup();
        if (!setupRun) return;
    }
    ThermoPrecalculate();
    deltaTime += static_cast<double>(TimeWarp::deltaTime());
    // (kspcore: not while the thermal graph waits for its rebuild: a part that died since the last FixedUpdate is freed memory here, a
    // destroyed but live object in the C#; the round-robin pass resumes after the next CheckThermalGraph)
    if (deltaTime > PhysicsGlobals::OcclusionMinStep() && !recreateThermalGraph) {
        UpdateOcclusion(false);
        deltaTime = 0.0;
    }
}

// ===================================================================================== the host

FlightIntegrator& FlightIntegratorHost::For(Vessel& vessel) {
    if (FlightIntegrator* fi = Find(vessel)) return *fi;
    auto fi = std::make_unique<FlightIntegrator>(vessel);
    fi->kernel = kernel;
    fi->sunBodyFlux = sunBodyFlux;
    if (configure) configure(*fi);
    fis_.emplace_back(&vessel, std::move(fi));
    return *fis_.back().second;
}

FlightIntegrator* FlightIntegratorHost::Find(const Vessel& vessel) const {
    for (const auto& e : fis_)
        if (e.first == &vessel) return e.second.get();
    return nullptr;
}

void FlightIntegratorHost::Remove(const Vessel& vessel) {
    for (auto it = fis_.begin(); it != fis_.end(); ++it)
        if (it->first == &vessel) {
            if (FlightIntegrator::ActiveVesselFI == it->second.get()) FlightIntegrator::ActiveVesselFI = nullptr;
            fis_.erase(it);
            return;
        }
}

void FlightIntegratorHost::Clear() {
    for (const auto& e : fis_)
        if (FlightIntegrator::ActiveVesselFI == e.second.get()) FlightIntegrator::ActiveVesselFI = nullptr;
    fis_.clear();
}

void FlightIntegratorHost::OnVesselUnloaded(const Vessel& vessel) {
    if (FlightIntegrator* fi = Find(vessel)) fi->OnUnloadVessel();
}

namespace {
// a vessel destroyed by an earlier vessel's step of the same pass gets no further callbacks (as FlightScene)
bool Registered(const Vessel* v) { return std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), v) != FlightGlobals::Vessels.end(); }
}  // namespace

void FlightIntegratorHost::FixedUpdate(VesselPhysicsHost& host) {
    const std::vector<Vessel*> vessels = FlightGlobals::Vessels;
    for (Vessel* v : vessels) {
        if (v == nullptr || !Registered(v) || !v->loaded || v->state == Vessel::State::DEAD) continue;  // FlightIntegrator.ShouldBeActive: vessel.loaded
        For(*v).FixedUpdate(v->packed ? nullptr : host.Find(*v));
    }
}

void FlightIntegratorHost::Update() {
    const std::vector<Vessel*> vessels = FlightGlobals::Vessels;
    for (Vessel* v : vessels) {
        if (v == nullptr || !Registered(v) || !v->loaded || v->state == Vessel::State::DEAD) continue;
        For(*v).Update();
    }
}

AeroKernelStats FlightIntegratorHost::TotalAeroStats() const {
    AeroKernelStats s;
    for (const auto& e : fis_) s.Add(e.second->aeroStats);
    return s;
}

}  // namespace ksp::physics
