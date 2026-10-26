// WP-19c: VesselRetrieval (VesselRetrieval.cs, 93 lines): the "recover vessel" request of the flight scene. GameEvents.OnVesselRecoveryRequested
// remembers the vessel id, saves "persistent" (OVERWRITE) and loads the Space Center; once that scene is loaded and KSCFrameDelay (8) frames have
// passed, every remembered vessel of FlightGlobals.Vessels is recovered (GameEvents.onVesselRecovered(protoVessel, false): the funds / science /
// reputation of the recovery are the listeners', e.g. WP-36's VesselRecovery scenario; then DestroyImmediate of the vessel) and "persistent" is
// saved again, whose new FlightState() no longer holds them.
//
// kspcore reading: the component lives across scenes (it sits on the persistent game object, as in KSP); SceneManager.sceneLoaded is the
// host's call of OnSceneLoaded(buildIndex) (HighLogic::OnSceneLoaded's order); the coroutine runs on HighLogic::Scheduler().
#pragma once

#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/events/game_event_types.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/config/config_node.hpp"  // Guid

namespace ksp {

class Vessel;

class VesselRetrieval {
public:
    VesselRetrieval();   // Awake (VesselRetrieval.cs:13)
    ~VesselRetrieval();  // OnDestroy (:20)
    VesselRetrieval(const VesselRetrieval&) = delete;
    VesselRetrieval& operator=(const VesselRetrieval&) = delete;

    void OnSceneLoaded(int buildIndex);              // :26 (SceneManager.sceneLoaded)
    Enumerator OnLevelLoaded(GameScenes scene);       // :31
    void onVesselRecoveryRequested(Vessel* v);        // :51
    void recoverVessels();                            // :58
    std::vector<Vessel*> getVesselsToRecover(const std::vector<Guid>& vesselIDs) const;  // :73
    void recoverVessel(Vessel* v);                    // :88

    const std::vector<Guid>& IDsOfVesselsToRecover() const { return ids_; }
    int KSCFrameDelay = 8;  // :9

private:
    std::vector<Guid> ids_;  // HashSet<Guid> (:11): Add ignores a duplicate; the order is never observed
};

}  // namespace ksp
