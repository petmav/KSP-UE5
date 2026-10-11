// WP-19a: VesselPrecalculate (VesselPrecalculate.cs) over the vessel registry; see include/ksp/vessel/vessel_precalculate.hpp.
// C# paths are relative to decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/vessel/vessel_precalculate.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/physics/reference_frame.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace M = dotnet::Math;
using GE = events::GameEvents;

namespace PrecalcMath {

// FlightGlobals.cs:1347
Vector3d GeeForceAtPosition(const Vector3d& posIn, const Vector3d& bodyPosition, double gMagnitudeAtCenter) {
    Vector3d pos = posIn - bodyPosition;
    double sqrMagnitude = pos.sqrMagnitude();
    sqrMagnitude *= M::Sqrt(sqrMagnitude);
    return pos * (0.0 - gMagnitudeAtCenter) / sqrMagnitude;
}

// VesselPrecalculate.cs:278-290
DriftCheck CheckDrift(bool driftAlwaysPassThreshChecks, const Vector3d& cposMinusCoM, const Vector3d& cvelRel, double sqrThreshold,
                      int framesToWait, int framesUntilCorrect) {
    DriftCheck r;
    r.framesUntilCorrect = framesUntilCorrect;
    if (!driftAlwaysPassThreshChecks && (!(cposMinusCoM.sqrMagnitude() < sqrThreshold) || cvelRel.sqrMagnitude() >= sqrThreshold)) {
        r.framesUntilCorrect = framesToWait;
        r.action = DriftAction::ResetWait;
    } else if (framesUntilCorrect > 0) {
        r.framesUntilCorrect = framesUntilCorrect - 1;
        r.action = DriftAction::CountDown;
    } else {
        r.action = DriftAction::Correct;
    }
    return r;
}

// VesselPrecalculate.cs:293-314
DriftResult ApplyDriftCompensation(const DriftInputs& in) {
    DriftResult r;
    r.preIntegrationVelocityOffset =
        (in.railsPosNext + in.referenceBodyPosition - in.CoMD - in.velocityD * in.fDeltaTime) * in.fDeltaTimeRecip;
    r.postIntegrationVelocityCorrection = in.railsVelNext - in.velocityD - in.rotFrameNext - r.preIntegrationVelocityOffset;
    const double k = in.GraviticForceMultiplier * in.gravityMultiplier;
    r.preIntegrationVelocityOffset *= k;
    r.postIntegrationVelocityCorrection *= k;
    r.integrationAccel.Zero();
    r.gAccel = (in.railsVelNext - in.velocityD - in.curRotFrameVel) * in.fDeltaTimeRecip;
    r.gravityForPos = r.preIntegrationVelocityOffset * in.fDeltaTimeRecip;
    if (VectorExtensions::IsInvalid(r.preIntegrationVelocityOffset) || VectorExtensions::IsInvalid(r.postIntegrationVelocityCorrection)) {
        r.invalid = true;
        r.preIntegrationVelocityOffset.Zero();
        r.postIntegrationVelocityCorrection.Zero();
    }
    return r;
}

// VesselPrecalculate.cs:317-343
GravityResult IntegrationGravity(const GravityInputs& in, const std::function<Vector3d(const Vector3d&)>& rotFrameVelAtPos) {
    GravityResult r;
    r.integrationAccel = in.gAccelTrue;
    const double k = in.GraviticForceMultiplier * in.gravityMultiplier;
    if (in.inverseRotation) {
        r.integrationAccel += in.coriolis + in.centrifugal;
        r.integrationAccel *= k;
        r.gravityForPos = r.integrationAccel;
        const Vector3d vector3d2 = in.velocityD + r.integrationAccel * in.fDeltaTime;
        Vector3d refPos = in.CoMD + vector3d2 * in.fDeltaTime - in.mainBodyPosition;
        refPos.Swizzle();
        Vector3d rotFrameVel = in.rotFrameVel;
        Vector3d rotFrameVelAtPosV = rotFrameVelAtPos(refPos);
        rotFrameVelAtPosV.Swizzle();
        r.gAccel = vector3d2 + rotFrameVelAtPosV - (in.velocityD + rotFrameVel);
        r.gAccel *= 1.0 / in.fDeltaTime;
    } else {
        r.integrationAccel *= k;
        r.gAccel = r.integrationAccel;
        r.gravityForPos = r.gAccel;
    }
    return r;
}

}  // namespace PrecalcMath

// ============================================================ the component

// Awake (VesselPrecalculate.cs:104: GetComponent<Vessel>, the onDockingComplete subscription)
VesselPrecalculate::VesselPrecalculate(Vessel* v) : vessel(v) {
    GE::onDockingComplete.Add(this, &VesselPrecalculate::onDockingComplete);
}

// The destructor is the delegate removal of OnDestroy (VesselPrecalculate.cs:113)
VesselPrecalculate::~VesselPrecalculate() { GE::onDockingComplete.Remove(this, &VesselPrecalculate::onDockingComplete); }

// VesselPrecalculate.cs:113 OnDestroy (called by ~Vessel, while the vessel and its orbit driver are still alive)
void VesselPrecalculate::OnDestroy() {
    StopEasing();
    GE::onDockingComplete.Remove(this, &VesselPrecalculate::onDockingComplete);
    if (vessel != nullptr && vessel->orbitDriver != nullptr) vessel->orbitDriver->QueueOnce = false;
}

// The isEasingGravity setter (VesselPrecalculate.cs:103-124)
void VesselPrecalculate::set_isEasingGravity(bool value) {
    if (easing_) {
        if (!value) StopEasing();
    } else if (value) {
        StartEasing();
    }
    easing_ = value;
}

// VesselPrecalculate.cs:123
void VesselPrecalculate::RunFirst() {
    if (vessel == nullptr || vessel->parts.empty()) return;
    int count = static_cast<int>(vessel->parts.size());
    do {
        if (count-- <= 0) {
            const bool num = allowDriftCompensation;
            allowDriftCompensation = false;
            MainPhysics(false);
            allowDriftCompensation = num;
            break;
        }
    } while (vessel->parts[static_cast<size_t>(count)] != nullptr);
}

// VesselPrecalculate.cs:144
void VesselPrecalculate::FixedUpdate() { MainPhysics(true); }

// VesselPrecalculate.cs:149
void VesselPrecalculate::MainPhysics(bool doKillChecks) {
    Vessel& v = *vessel;
    packedInFixed = !v.loaded || v.packed;
    fDeltaTime = static_cast<double>(TimeWarp::fixedDeltaTime());
    fDeltaTimeRecip_ = 1.0 / fDeltaTime;
    v.orbitDriver->QueueOnce = !v.orbitDriver->QueuedUpdate;
    lastMode = v.orbitDriver->lastMode;
    if (v.orbitDriver->updateMode == OrbitDriver::UpdateMode::UPDATE && updateOrbit) v.orbitDriver->UpdateOrbit();
    else SetLandedPosRot();
    if (!v.packed) ApplyVelocityCorrection();
    wasCorrectingDrift = false;
    correctDriftThisFrame_ = false;
    postIntegrationVelocityCorrection.Zero();
    preIntegrationVelocityOffset.Zero();
    if (!v.packed && !physStatsNotDoneInUpdate) {
        if (calculateGravity) {
            CalculateGravity();
            v.graviticAcceleration = gAccel;
            ranFixedThisFrame = false;
        }
    } else {
        CalculatePhysicsStats();
        if (updateOrbit && v.orbitDriver->updateMode != OrbitDriver::UpdateMode::UPDATE) v.orbitDriver->UpdateOrbit();
        v.UpdatePosVel();
        if (calculateGravity && v.loaded) CalculateGravity();
        v.UpdateAcceleration(fDeltaTimeRecip_, false);
        if (calculateGravity) v.graviticAcceleration = gAccel;
        ranFixedThisFrame = true;
    }
    if (doKillChecks) v.CheckKill();
    physStatsNotDoneInUpdate = true;
}

// VesselPrecalculate.cs:207
void VesselPrecalculate::Update() {
    if (!packedInFixed && !disableRunInUpdate) {
        const double universalTime = Planetarium::GetUniversalTime();
        if (lastUT != universalTime) {
            lastUT = universalTime;
            physStatsNotDoneInUpdate = false;
            ApplyVelocityCorrection();
            preIntegrationVelocityOffset.Zero();
            CalculatePhysicsStats();
            vessel->orbitDriver->UpdateOrbit(false);
            vessel->UpdatePosVel();
            vessel->UpdateAcceleration(fDeltaTimeRecip_, true);
        }
    }
}

// VesselPrecalculate.cs:226
void VesselPrecalculate::ApplyVelocityCorrection() {
    if (correctDriftThisFrame_) vessel->OffsetVelocity(postIntegrationVelocityCorrection);
    correctDriftThisFrame_ = false;
    postIntegrationVelocityCorrection.Zero();
}

// VesselPrecalculate.cs:236
void VesselPrecalculate::GoOnRails() {
    if (vessel->orbitDriver->updateMode == OrbitDriver::UpdateMode::TRACK_Phys && !postIntegrationVelocityCorrection.IsZero()) {
        ApplyVelocityCorrection();
        CalculatePhysicsStats();
        vessel->orbitDriver->TrackRigidbody(vessel->mainBody(), 0.0);
    }
    correctDriftThisFrame_ = false;
    wasCorrectingDrift = false;
    physStatsNotDoneInUpdate = true;
    postIntegrationVelocityCorrection.Zero();
}

// VesselPrecalculate.cs:250
void VesselPrecalculate::GoOffRails() {}

// VesselPrecalculate.cs:254
void VesselPrecalculate::StartEasing() {
    vessel->gravityMultiplier = 0.0;
    easing_ = true;
    GE::onPhysicsEaseStart.Fire(vessel);
    if (vessel == FlightGlobals::ActiveVessel) {
        easingLockOn_ = true;
        easingLockOn = true;
        InputLockManager::SetControlLock(ControlTypes::ALL_SHIP_CONTROLS, "PhysicsEase");
        // ScreenMessages.PostScreenMessage(#autoLOC_6003101, (float)(1.0 / easingFrameIncrease * 0.02 + 0.5), UPPER_RIGHT): the host shows it
        easingMessageSeconds = static_cast<float>(1.0 / easingFrameIncrease * 0.02 + 0.5);
    }
}

// VesselPrecalculate.cs:267
void VesselPrecalculate::StopEasing() {
    easing_ = false;
    if (vessel != nullptr) vessel->gravityMultiplier = 1.0;
    GE::onPhysicsEaseStop.Fire(vessel);
    if (easingLockOn_) InputLockManager::RemoveControlLock("PhysicsEase");
}

// VesselPrecalculate.cs:281 CalculateGravity
void VesselPrecalculate::CalculateGravity() {
    Vessel& v = *vessel;
    v.gravityTrue = (gAccelTrue = PrecalcMath::GeeForceAtPosition(v.CoMD, v.mainBody()->position, v.mainBody()->gMagnitudeAtCenter));
    if (!v.packed) {
        if (lastMode != OrbitDriver::UpdateMode::TRACK_Phys) v.IgnoreGForces(10);
        if ((GameSettings::PHYSICS_EASE && !easing_ && lastMode == OrbitDriver::UpdateMode::IDLE &&
             v.orbitDriver->updateMode == OrbitDriver::UpdateMode::TRACK_Phys) ||
            easeDocking) {
            StartEasing();
            easeDocking = false;
        }
        if (easing_) {
            v.gravityMultiplier += easingFrameIncrease;
            if (v.gravityMultiplier >= 1.0) StopEasing();
        }
        if (GameSettings::ORBIT_DRIFT_COMPENSATION && allowDriftCompensation && v.loaded && !v.packed && !v.LandedOrSplashed() &&
            v.mainBody() == v.lastBody && v.staticPressurekPa == 0.0 &&
            (!v.mainBody()->atmosphere || v.altitude > v.mainBody()->atmosphereDepth) &&
            v.orbit().radius < PhysicsGlobals::OrbitDriftAltThreshold()) {
            const Vector3d vector3d = v.CoMD - v.orbit().referenceBody->position;
            v.orbit().GetOrbitalStateVectorsAtUT(v.orbitDriver->lastTrackUT, cpos_, cvel_);
            curRotFrameVel_ = v.orbit().GetRotFrameVelAtPos(*v.mainBody(), cpos_);
            cpos_.Swizzle();
            cvel_.Swizzle();
            curRotFrameVel_.Swizzle();
            const PrecalcMath::DriftCheck check = PrecalcMath::CheckDrift(
                driftAlwaysPassThreshChecks, cpos_ - vector3d, cvel_ - curRotFrameVel_ - v.velocityD,
                PhysicsGlobals::OrbitDriftSqrThreshold(), PhysicsGlobals::OrbitDriftFramesToWait(), framesUntilCorrect_);
            framesUntilCorrect_ = check.framesUntilCorrect;
            if (check.action == PrecalcMath::DriftAction::Correct) {
                wasCorrectingDrift = true;
                correctDriftThisFrame_ = true;
                const double uT = v.orbitDriver->lastTrackUT + fDeltaTime;
                v.orbit().GetOrbitalStateVectorsAtUT(uT, railsPosNext_, railsVelNext_);
                rotFrameNext_ = v.orbit().GetRotFrameVelAtPos(*v.orbit().referenceBody, railsPosNext_);
                railsPosNext_.Swizzle();
                railsVelNext_.Swizzle();
                rotFrameNext_.Swizzle();
                PrecalcMath::DriftInputs in;
                in.railsPosNext = railsPosNext_;
                in.railsVelNext = railsVelNext_;
                in.rotFrameNext = rotFrameNext_;
                in.curRotFrameVel = curRotFrameVel_;
                in.referenceBodyPosition = v.orbit().referenceBody->position;
                in.CoMD = v.CoMD;
                in.velocityD = v.velocityD;
                in.fDeltaTime = fDeltaTime;
                in.fDeltaTimeRecip = fDeltaTimeRecip_;
                in.GraviticForceMultiplier = PhysicsGlobals::GraviticForceMultiplier;
                in.gravityMultiplier = v.gravityMultiplier;
                const PrecalcMath::DriftResult r = PrecalcMath::ApplyDriftCompensation(in);
                preIntegrationVelocityOffset = r.preIntegrationVelocityOffset;
                postIntegrationVelocityCorrection = r.postIntegrationVelocityCorrection;
                integrationAccel = r.integrationAccel;
                gAccel = r.gAccel;
                v.gravityForPos = r.gravityForPos;
                if (r.invalid) wasCorrectingDrift = false;
            }
        }
        if (!wasCorrectingDrift) {
            PrecalcMath::GravityInputs in;
            in.gAccelTrue = gAccelTrue;
            in.inverseRotation = v.mainBody()->inverseRotation;
            in.GraviticForceMultiplier = PhysicsGlobals::GraviticForceMultiplier;
            in.gravityMultiplier = v.gravityMultiplier;
            in.CoMD = v.CoMD;
            in.velocityD = v.velocityD;
            in.mainBodyPosition = v.mainBody()->position;
            in.fDeltaTime = fDeltaTime;
            if (in.inverseRotation) {
                coriolis = ReferenceFrame::getCoriolisAcc(v.velocityD, *v.mainBody());
                centrifugal = ReferenceFrame::getCentrifugalAcc(v.CoMD, *v.mainBody());
                in.coriolis = coriolis;
                in.centrifugal = centrifugal;
                Vector3d rotFrameVel = v.orbit().GetRotFrameVel(*v.mainBody());
                rotFrameVel.Swizzle();
                in.rotFrameVel = rotFrameVel;
            }
            const PrecalcMath::GravityResult r = PrecalcMath::IntegrationGravity(
                in, [&](const Vector3d& refPos) { return v.orbit().GetRotFrameVelAtPos(*v.mainBody(), refPos); });
            integrationAccel = r.integrationAccel;
            gAccel = r.gAccel;
            v.gravityForPos = r.gravityForPos;
        }
    } else if (easing_) {
        StopEasing();
    }
}

// VesselPrecalculate.cs:377
void VesselPrecalculate::SetLandedPosRot() {
    Vessel& v = *vessel;
    if (v.LandedOrSplashed() && v.packed && railsSetPosRot) {
        // `bodyTransform.rotation * srfRelRotation` is Unity float quaternion maths; bodyTransform.rotation = (Quaternion)rotation (CelestialBody.cs:558)
        worldSurfaceRot = QuaternionD(v.mainBody()->rotation.ToQuaternionf() * v.srfRelRotation);
        v.SetRotation(worldSurfaceRot.ToQuaternionf(), false);
        worldSurfacePos = v.mainBody()->GetWorldSurfacePosition(v.latitude, v.longitude, v.altitude);
        v.SetPosition(worldSurfacePos);
    }
}

// VesselPrecalculate.cs:388 (the arithmetic is Vessel::CalculatePhysicsStats, WP-14, which reads worldSurfacePos / worldSurfaceRot from here)
void VesselPrecalculate::CalculatePhysicsStats() { vessel->CalculatePhysicsStats(); }

// VesselPrecalculate.cs:513
void VesselPrecalculate::onDockingComplete(GE::FromToAction<Part*, Part*> fromTo) {
    if (vessel == nullptr) return;
    if (fromTo.from->vessel == vessel || fromTo.to->vessel == vessel) easeDocking = true;
}

}  // namespace ksp
