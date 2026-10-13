// Vessel: the MonoBehaviour tick (WP-19a): FixedUpdate / Update / LateUpdate, CheckKill, OffsetVelocity.
// C# paths are relative to decompiled/KSP-Decompiled/Assembly-CSharp/. The rigid-body parts (AddRBAnchor, Part.explode, rigid-body
// velocities) go through the VesselEnvironment seams (vessel.hpp), see PHYSICS-SPIKE.md 5.3 (`VesselPhysics`, WP-20b).
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_autopilot.hpp"

namespace ksp {

using GE = events::GameEvents;

// Vessel.cs:2730
void Vessel::FixedUpdate() {
    if (state != State::DEAD) {
        UpdateCaches();
        if (ignoreCollisionsFrames > 0) ignoreCollisionsFrames--;
        if (ignoreSpeedFrames_ > 0) ignoreSpeedFrames_--;
    }
}

// Vessel.cs:2746
void Vessel::CheckKill() {
    if (FlightGlobals::ready && packed && this != FlightGlobals::ActiveVessel && !FlightGlobals::overrideOrbit && !LandedOrSplashed() &&
        mainBody()->GetPressure(altitude) > 1.0) {
        bool flag = true;
        if (env.LoadedSceneIsFlight && FlightGlobals::ActiveVessel != nullptr) {
            float pack = vesselRanges.GetSituationRanges(BestSituation()).pack;
            pack *= pack;
            if ((vesselTransform.position - FlightGlobals::ActiveVessel->vesselTransform.position).sqrMagnitude() < pack) flag = false;
        }
        if (flag) {
            log::Info("[F: " + std::to_string(env.frameCount) + "]: Vessel " + GetDisplayName() + " was on-rails at " +
                      std::to_string(FlightGlobals::getStaticPressure(Vector3d(vesselTransform.position), *mainBody())) + " kPa pressure and was destroyed.");
            MurderCrew();
            Die();
            return;
        }
    }
    if (!LandedOrSplashed() && PhysicsHoldExpired() && altitude < ((terrainAltitude != -1.0) ? terrainAltitude : (loaded ? (-250.0) : (-0.1)))) {
        log::Warning("[F: " + std::to_string(env.frameCount) + "]: Vessel " + GetDisplayName() + " crashed through terrain on " + mainBody()->bodyName + ".");
        GE::onVesselExplodeGroundCollision.Fire(this);
        if (loaded) {
            EventReport report(FlightEvents::CRASH, rootPart, GetDisplayName(), mainBody()->displayName());
            GE::onCrash.Fire(&report);
            const float offset = static_cast<float>(terrainAltitude - altitude);
            if (env.explodePart && rootPart != nullptr) env.explodePart(*rootPart, offset);
        } else {
            MurderCrew();
            Die();
        }
    }
}

// Vessel.cs:2991 (the camera-dependent FloatingOrigin.ResetTerrainShaderOffset branch for the active vessel is rendering)
void Vessel::Update() {
    if (state == State::DEAD) return;
    if (situation == Situations::PRELAUNCH) launchTime = Planetarium::GetUniversalTime();
    missionTime = Planetarium::GetUniversalTime() - launchTime;
    UpdateVesselModuleActivation();
    if (autoClean_ && !loaded && (!env.currenciesAvailable || env.currenciesAvailable())) {
        Clean(autoCleanReason_);
    } else {
        if (!FlightGlobals::ready) return;
        if (this != FlightGlobals::ActiveVessel) {
            Vessel& active = *FlightGlobals::ActiveVessel;
            if (loaded && Vector3f::Distance(vesselTransform.position, active.vesselTransform.position) > vesselRanges.GetSituationRanges(situation).unload)
                Unload();
            if (!loaded && Vector3f::Distance(vesselTransform.position, active.vesselTransform.position) < vesselRanges.GetSituationRanges(situation).load)
                Load();
        }
        if (IsControllable() && !packed) autopilot_->Update();
    }
}

// Vessel.cs:3055
void Vessel::LateUpdate() {
    if (!FlightGlobals::ready || state == State::DEAD) return;
    if (!bouncedOffCheck_ && env.timeSinceLevelLoad > 5.0f) {
        bouncedOffCheck_ = true;
        UpdateLandedSplashed();
    }
    CheckControllable();
    if (env.timeSinceLevelLoad > 0.5f) updateSituation();
    if (loaded) {
        Game* game = SaveEnvironment::Current().currentGame;
        const bool gLimits = game != nullptr && game->Parameters->CustomParams<GameParameters::AdvancedParams>().GPartLimits;
        const bool presLimits = game != nullptr && game->Parameters->CustomParams<GameParameters::AdvancedParams>().PressurePartLimits;
        if (partMaxGThresh > warningThresholdG && gLimits) {
            // ScreenMessages.PostScreenMessage(gMessage) when this is the active vessel: UI
            gThresh = static_cast<float>((partMaxGThresh - warningThresholdG) * (1.0 / (1.0 - warningThresholdG)));
        } else {
            gThresh = 0.0f;
        }
        if (partMaxPresThresh > warningThresholdPres && presLimits) {
            presThresh = static_cast<float>((partMaxPresThresh - warningThresholdPres) * (1.0 / (1.0 - warningThresholdPres)));
        } else {
            presThresh = 0.0f;
        }
        AddRBAnchor();  // WP-20e: the landed FixedJoint anchor (src/vessel/vessel_ground.cpp; the joint itself through Vessel::env.rbAnchor*)
    }
    partMaxPresThresh = 0.0;
    partMaxGThresh = 0.0;
}

// Vessel.cs:3742 (the loop over the parts' rigid bodies and servo rigid bodies is the seam)
void Vessel::OffsetVelocity(const Vector3d& correction) {
    if (env.offsetVelocity) env.offsetVelocity(*this, correction);
}

// Vessel.cs:3197 (WP-19b). The part loop (rb and servoRb of every part that is not DEAD) is the offsetVelocity seam: the reduced model
// moves every cluster body by velOffset, which is the same rigid-body change for every part (VesselPhysics::ChangeWorldVelocity).
void Vessel::ChangeWorldVelocity(const Vector3d& velOffset) {
    if (state == State::DEAD || packed) return;
    if (env.offsetVelocity) env.offsetVelocity(*this, velOffset);
    krakensbaneAcc += velOffset * (1.0 / static_cast<double>(TimeWarp::fixedDeltaTime()));
}

// Vessel.cs:3219 (WP-19b). Every part rb (and servoRb) = vel: the reduced model sets every cluster's centre-of-mass velocity
// (VesselPhysics::SetLinearVelocity; angular velocities kept).
void Vessel::SetWorldVelocity(const Vector3d& vel) {
    if (state == State::DEAD || packed) return;
    if (env.setWorldVelocity) env.setWorldVelocity(*this, vel);
}

}  // namespace ksp
