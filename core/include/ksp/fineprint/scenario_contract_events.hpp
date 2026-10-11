// WP-38 slice 2: FinePrint/ScenarioContractEvents.cs ([KSPScenario(96, FLIGHT, TRACKSTATION, SPACECENTER)]): the per-game contract seed (id) and the
// vessel event relay that re-scans the SpecificVesselParameters (Invoke after 0.2 s) and removes toured tourists after recovery.
//
// Seam: the Unity timers (Invoke / StartCoroutine(WaitForSeconds)) are driven by AdvanceTime(dt) (the host calls it with the frame time).
#pragma once

#include <memory>
#include <set>
#include <vector>

#include "ksp/events/game_events.hpp"
#include "ksp/fineprint/fineprint.hpp"
#include "ksp/save/game.hpp"

namespace ksp::FinePrint {

class ScenarioContractEvents : public ScenarioModule {
public:
    ScenarioContractEvents();
    ~ScenarioContractEvents() override;
    static constexpr float eventDelay = 0.2f;                 // :18
    static inline int gameSeed = -1;                          // :20
    static int GameSeed() { return gameSeed; }                // :24
    // :26 (HashSet<Guid>: the 16 bytes of each Guid)
    std::set<std::vector<uint8_t>> specificVesselCache;
    void OnAwake() override;                                  // :28
    void OnDestroy();                                         // :38
    void OnSave(ConfigNode& node) override;                   // :52
    void OnLoad(ConfigNode& node) override;                   // :57
    void ProcessGameSeed();                                   // :63
    void VesselWasModifiedEvent(Vessel* v);                   // :74
    void PartCoupleEvent(events::GameEvents::FromToAction<Part*, Part*> action);  // :79
    void VesselWillDestroyEvent(Vessel* v);                   // :84
    void VesselTerminatedEvent(ProtoVessel* pv);              // :89
    void VesselRecoveredEvent(ProtoVessel* pv, bool quick);   // :94
    void VesselRenameEvent(events::GameEvents::HostedFromToAction<Vessel*, std::string> action);  // :100
    void TouristDepartures(float delay, ProtoVessel* pv);     // :124
    void InvokeCheckSpecificVessels(const std::vector<Guid>& guids);  // :138
    void CheckSpecificVessels();                              // :151
    // the Unity timers (Invoke("CheckSpecificVessels", 0.2) / WaitForSeconds(0.2)): advance by dt seconds
    void AdvanceTime(float dt);
    bool IsInvoking() const { return invoking_; }
    static void Register();

private:
    bool invoking_ = false;
    float invokeRemaining_ = 0.0f;
    struct Pending {
        float remaining;
        std::vector<std::shared_ptr<ProtoCrewMember>> crew;
        ProtoVessel* pv;
    };
    std::vector<Pending> departures_;
    bool awake_ = false;
};

}  // namespace ksp::FinePrint
