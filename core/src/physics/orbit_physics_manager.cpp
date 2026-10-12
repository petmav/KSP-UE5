// WP-19a: OrbitPhysicsManager (pack / unpack decisions, hold counter); see include/ksp/physics/orbit_physics_manager.hpp.
// C# paths are relative to decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/physics/orbit_physics_manager.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

// OrbitPhysicsManager.cs:33
CelestialBody* OrbitPhysicsManager::DominantBody() { return fetch_ != nullptr ? fetch_->dominantBody : nullptr; }

// Awake (OrbitPhysicsManager.cs:45)
OrbitPhysicsManager::OrbitPhysicsManager() {
    if (fetch_ != nullptr) alive_ = false;  // UnityEngine.Object.Destroy(this)
    else fetch_ = this;
}

// OnDestroy (OrbitPhysicsManager.cs:57)
OrbitPhysicsManager::~OrbitPhysicsManager() {
    if (fetch_ != nullptr && fetch_ == this) fetch_ = nullptr;
}

// OrbitPhysicsManager.cs:65 Start (the coroutine's `while (!FlightGlobals.ready) yield WaitForSeconds(0.5f)` is the caller polling)
bool OrbitPhysicsManager::Start() {
    if (!started_ && FlightGlobals::ready) {
        dominantBody = FlightGlobals::currentMainBody;
        started_ = true;
    }
    return started_;
}

// OrbitPhysicsManager.cs:75
void OrbitPhysicsManager::HoldVesselUnpack(int releaseAfter) {
    if (fetch_ != nullptr) {
        fetch_->holdVesselUnpack = true;
        fetch_->releaseUnpackIn_ = releaseAfter;
    }
}

// OrbitPhysicsManager.cs:84
void OrbitPhysicsManager::FixedUpdate() {
    if (!FlightGlobals::ready || !started_ || FlightGlobals::overrideOrbit) return;
    if (!degub) checkReferenceFrame();
    if (holdVesselUnpack) {
        if (releaseUnpackIn_ > 0) releaseUnpackIn_--;
        else holdVesselUnpack = false;
    }
}

// OrbitPhysicsManager.cs:107
void OrbitPhysicsManager::LateUpdate() {
    if (!FlightGlobals::ready || !started_ || FlightGlobals::overrideOrbit) return;
    const bool flag = TimeWarp::WarpMode() == TimeWarp::Modes::HIGH && TimeWarp::CurrentRate() > TimeWarp::MaxPhysicsRate();
    Vessel* activeVessel = FlightGlobals::ActiveVessel;
    if (!FlightGlobals::warpDriveActive) {
        if (flag) {
            if (!activeVessel->packed) activeVessel->GoOnRails();
        } else if (activeVessel->packed && !holdVesselUnpack) {
            activeVessel->GoOffRails();
        }
    }
    for (size_t i = 0; i < FlightGlobals::Vessels.size(); i++) {
        Vessel* vessel = FlightGlobals::Vessels[i];
        if (vessel == activeVessel || vessel == nullptr || vessel->orbitDriver == nullptr) continue;
        if (!FlightGlobals::warpDriveActive && flag && !vessel->packed) vessel->GoOnRails();
        const float num = Vector3f::Distance(vessel->vesselTransform.position, activeVessel->vesselTransform.position);
        if (num > vessel->vesselRanges.GetSituationRanges(vessel->BestSituation()).pack) {
            if (!vessel->packed) vessel->GoOnRails();
        } else if (num < vessel->vesselRanges.GetSituationRanges(vessel->situation).unpack && vessel->packed &&
                   TimeWarp::CurrentRate() <= TimeWarp::MaxPhysicsRate() && !holdVesselUnpack) {
            vessel->GoOffRails();
        }
    }
}

// OrbitPhysicsManager.cs:155 (state part; the velocity resume of the loaded vessels and physical objects is WP-19b's seam)
void OrbitPhysicsManager::setDominantBody(CelestialBody* body) {
    if (env.setDominantBody) {
        env.setDominantBody(*this, body);
        return;
    }
    dominantBody = body;
    GE::onDominantBodyChange.Fire(GE::FromToAction<CelestialBody*, CelestialBody*>(dominantBody, body));
}

// OrbitPhysicsManager.cs:235 (state part; the rigid-body velocity change is WP-19b's seam)
void OrbitPhysicsManager::setRotatingFrame(bool rotatingFrameState) {
    if (env.setRotatingFrame) {
        env.setRotatingFrame(*this, rotatingFrameState);
        return;
    }
    dominantBody->inverseRotation = rotatingFrameState;
    GE::onRotatingFrameTransition.Fire(GE::HostTargetAction<CelestialBody*, bool>(dominantBody, rotatingFrameState));
}

// OrbitPhysicsManager.cs:322
void OrbitPhysicsManager::checkReferenceFrame() {
    if (dominantBody != FlightGlobals::ActiveVessel->orbit().referenceBody) setDominantBody(FlightGlobals::ActiveVessel->orbit().referenceBody);
    if (!fetch_->dominantBody->rotates) return;
    const double num = FlightGlobals::getAltitudeAtPos(Vector3d(FlightGlobals::ActiveVessel->vesselTransform.position), dominantBody);
    if (num < static_cast<double>(dominantBody->inverseRotThresholdAltitude)) {
        if (!dominantBody->inverseRotation) setRotatingFrame(true);
    } else if (num > static_cast<double>(dominantBody->inverseRotThresholdAltitude) && dominantBody->inverseRotation) {
        setRotatingFrame(false);
    }
}

// OrbitPhysicsManager.cs:218
void OrbitPhysicsManager::SetDominantBody(CelestialBody* body) {
    if (fetch_ != nullptr) fetch_->setDominantBody(body);
}

// OrbitPhysicsManager.cs:314
void OrbitPhysicsManager::SetRotatingFrame(bool rotating) {
    if (fetch_ != nullptr) fetch_->setRotatingFrame(rotating);
}

// OrbitPhysicsManager.cs:346
void OrbitPhysicsManager::CheckReferenceFrame() {
    if (fetch_ != nullptr) fetch_->checkReferenceFrame();
}

}  // namespace ksp
