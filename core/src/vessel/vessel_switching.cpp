// VesselSwitching (VesselSwitching.cs): which loaded vessel the next / previous key switches to; see include/ksp/vessel/vessel_switching.hpp.
// C# paths are relative to decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/vessel/vessel_switching.hpp"

#include <algorithm>

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/input/flight_input_handler.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using Localizer = KSP::Localization::Localizer;

// Awake (VesselSwitching.cs:15)
VesselSwitching::VesselSwitching() {
    if (fetch_ != nullptr) alive_ = false;  // UnityEngine.Object.Destroy(this)
    else fetch_ = this;
}

// OnDestroy (VesselSwitching.cs:27)
VesselSwitching::~VesselSwitching() {
    if (fetch_ != nullptr && fetch_ == this) fetch_ = nullptr;
}

// the do { ... } while (v != FlightGlobals.ActiveVessel) loop of Update (VesselSwitching.cs:60-92)
std::vector<Vessel*> VesselSwitching::FindCandidates(int direction, bool modifierKey, bool& skippedUncontrollable) const {
    std::vector<Vessel*> candidates;
    skippedUncontrollable = false;
    const auto& vessels = FlightGlobals::Vessels;
    int nextVessel = static_cast<int>(std::find(vessels.begin(), vessels.end(), FlightGlobals::ActiveVessel) - vessels.begin());
    // (an active vessel that is not in the list makes the C# loop forever: nothing to switch to here)
    if (FlightGlobals::ActiveVessel == nullptr || nextVessel == static_cast<int>(vessels.size())) return candidates;
    const int count = static_cast<int>(vessels.size());
    Vessel* v = nullptr;
    do {
        nextVessel += direction;
        if (nextVessel >= 0) {
            if (nextVessel >= count) nextVessel -= count;
        } else {
            nextVessel += count;
        }
        v = vessels.at(static_cast<size_t>(nextVessel));
        if (v->loaded && v != FlightGlobals::ActiveVessel && v->vesselType != VesselType::DeployedSciencePart) {
            if (static_cast<int>(v->vesselType) <= static_cast<int>(VesselType::SpaceObject) && !modifierKey) {
                skippedUncontrollable = true;
                continue;  // the `while` condition is evaluated
            }
            candidates.push_back(v);
        }
    } while (v != FlightGlobals::ActiveVessel);
    return candidates;
}

// VesselSwitching.cs:35
Vessel* VesselSwitching::Update() {
    const bool mapEnabled = env.mapIsEnabled && env.mapIsEnabled();
    const bool paused = env.paused && env.paused();
    if (mapEnabled || InputLockManager::IsLocked(ControlTypes::VESSEL_SWITCHING) || paused) return nullptr;
    nextVessel_ = -1;
    bool flag = false;
    const bool prev = env.focusPrevKeyDown && env.focusPrevKeyDown();
    const bool next = env.focusNextKeyDown && env.focusNextKeyDown();
    if (prev || next) {
        Game* game = SaveEnvironment::Current().currentGame;
        const bool sceneIsFlight = !env.loadedSceneIsFlight || env.loadedSceneIsFlight();
        if (game == nullptr || !sceneIsFlight) return nullptr;
        if (game != nullptr && game->Parameters && !game->Parameters->Flight.CanSwitchVesselsNear) {
            if (env.postScreenMessage) env.postScreenMessage(Localizer::Format("#autoLOC_137562"));
            return nullptr;
        }
        if (sceneIsFlight && env.setCameraFlight) env.setCameraFlight();
        nextVesselCandidates_ = FindCandidates(prev ? -1 : 1, env.modifierKeyHeld && env.modifierKeyHeld(), flag);
        nextVessel_ = 0;  // (the index of the active vessel after the loop; only `!= -1` matters below)
    }
    if (nextVessel_ == -1) return nullptr;
    if (nextVesselCandidates_.empty()) {
        if (env.postScreenMessage) env.postScreenMessage(Localizer::Format(flag ? "#autoLOC_8003367" : "#autoLOC_137597"));
        return nullptr;
    }
    const ClearToSaveStatus clearToSaveStatus = FlightGlobals::ClearToSave(true);
    for (Vessel* vessel : nextVesselCandidates_) {
        if (vessel->packed && clearToSaveStatus != ClearToSaveStatus::CLEAR) continue;
        FlightGlobals::ForceSetActiveVessel(vessel);
        if (env.resumeVesselCtrlState) env.resumeVesselCtrlState(*vessel);
        else FlightInputHandler::ResumeVesselCtrlState(vessel);
        return vessel;
    }
    FlightGlobals::SetActiveVessel(nextVesselCandidates_[0]);  // (the C# logs an error if this succeeds: "Should not have been possible to get here")
    return nullptr;
}

}  // namespace ksp
