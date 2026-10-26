// WP-19c: VesselRetrieval (VesselRetrieval.cs); see include/ksp/vessel/vessel_retrieval.hpp.
#include "ksp/vessel/vessel_retrieval.hpp"

#include <algorithm>

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/save/game_persistence.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

// VesselRetrieval.cs:13
VesselRetrieval::VesselRetrieval() { GE::OnVesselRecoveryRequested.Add(this, &VesselRetrieval::onVesselRecoveryRequested); }

// VesselRetrieval.cs:20
VesselRetrieval::~VesselRetrieval() { GE::OnVesselRecoveryRequested.Remove(this, &VesselRetrieval::onVesselRecoveryRequested); }

// VesselRetrieval.cs:26
void VesselRetrieval::OnSceneLoaded(int buildIndex) {
    HighLogic::Scheduler().StartCoroutine(OnLevelLoaded(HighLogic::GetLoadedGameSceneFromBuildIndex(buildIndex)));
}

// VesselRetrieval.cs:31
Enumerator VesselRetrieval::OnLevelLoaded(GameScenes scene) {
    if (scene == GameScenes::SPACECENTER && !ids_.empty()) {
        int i = 0;
        while (i < KSCFrameDelay) {
            co_yield YieldInstruction::Null();
            i = i + 1;
        }
        recoverVessels();
    }
}

// VesselRetrieval.cs:51
void VesselRetrieval::onVesselRecoveryRequested(Vessel* v) {
    if (std::find(ids_.begin(), ids_.end(), v->id) == ids_.end()) ids_.push_back(v->id);
    GamePersistence::SaveGame("persistent", HighLogic::SaveFolder(), SaveMode::OVERWRITE);
    HighLogic::LoadScene(GameScenes::SPACECENTER);
}

// VesselRetrieval.cs:58
void VesselRetrieval::recoverVessels() {
    if (ids_.empty()) return;
    std::vector<Vessel*> vesselsToRecover = getVesselsToRecover(ids_);
    const int count = static_cast<int>(vesselsToRecover.size());
    for (int i = 0; i < count; i++) recoverVessel(vesselsToRecover[static_cast<size_t>(i)]);
    GamePersistence::SaveGame("persistent", HighLogic::SaveFolder(), SaveMode::OVERWRITE);
    ids_.clear();
}

// VesselRetrieval.cs:73
std::vector<Vessel*> VesselRetrieval::getVesselsToRecover(const std::vector<Guid>& vesselIDs) const {
    std::vector<Vessel*> list;
    const int count = static_cast<int>(FlightGlobals::Vessels.size());
    for (int i = 0; i < count; i++) {
        Vessel* vessel = FlightGlobals::Vessels[static_cast<size_t>(i)];
        if (std::find(vesselIDs.begin(), vesselIDs.end(), vessel->id) != vesselIDs.end()) list.push_back(vessel);
    }
    return list;
}

// VesselRetrieval.cs:88 (UnityEngine.Object.DestroyImmediate(v.gameObject): the vessel leaves FlightGlobals and is freed when the scene owns it)
void VesselRetrieval::recoverVessel(Vessel* v) {
    GE::onVesselRecovered.Fire(v->protoVessel, false);
    Vessel::DestroyImmediate(v);
}

}  // namespace ksp
