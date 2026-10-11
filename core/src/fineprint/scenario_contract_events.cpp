// WP-38 slice 2: FinePrint/ScenarioContractEvents.cs. See include/ksp/fineprint/scenario_contract_events.hpp.
#include "ksp/fineprint/scenario_contract_events.hpp"

#include "ksp/career/career.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/fineprint/contracts.hpp"
#include "ksp/fineprint/utilities.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp::FinePrint {

using GE = events::GameEvents;
namespace L = ksp::Contracts::Lite;

namespace {
std::vector<uint8_t> Key(const Guid& g) { return std::vector<uint8_t>(g.bytes, g.bytes + 16); }
Guid FromKey(const std::vector<uint8_t>& k) {
    Guid g;
    for (size_t i = 0; i < 16; ++i) g.bytes[i] = k[i];
    return g;
}
}  // namespace

ScenarioContractEvents::ScenarioContractEvents() : ScenarioModule("ScenarioContractEvents") {}
ScenarioContractEvents::~ScenarioContractEvents() { OnDestroy(); }

// ScenarioContractEvents.cs:28
void ScenarioContractEvents::OnAwake() {
    GE::onVesselWasModified.Add(this, &ScenarioContractEvents::VesselWasModifiedEvent);
    GE::onPartCouple.Add(this, &ScenarioContractEvents::PartCoupleEvent);
    GE::onVesselWillDestroy.Add(this, &ScenarioContractEvents::VesselWillDestroyEvent);
    GE::onVesselTerminated.Add(this, &ScenarioContractEvents::VesselTerminatedEvent);
    GE::onVesselRecovered.Add(this, &ScenarioContractEvents::VesselRecoveredEvent);
    GE::onVesselRename.Add(this, &ScenarioContractEvents::VesselRenameEvent);
    specificVesselCache.clear();
    awake_ = true;
}
// ScenarioContractEvents.cs:38
void ScenarioContractEvents::OnDestroy() {
    if (!awake_) return;
    awake_ = false;
    GE::onVesselWasModified.Remove(this, &ScenarioContractEvents::VesselWasModifiedEvent);
    GE::onPartCouple.Remove(this, &ScenarioContractEvents::PartCoupleEvent);
    GE::onVesselWillDestroy.Remove(this, &ScenarioContractEvents::VesselWillDestroyEvent);
    GE::onVesselTerminated.Remove(this, &ScenarioContractEvents::VesselTerminatedEvent);
    GE::onVesselRecovered.Remove(this, &ScenarioContractEvents::VesselRecoveredEvent);
    GE::onVesselRename.Remove(this, &ScenarioContractEvents::VesselRenameEvent);
    if (IsInvoking()) invoking_ = false;
}
// ScenarioContractEvents.cs:52
void ScenarioContractEvents::OnSave(ConfigNode& node) { node.AddValue("id", static_cast<int32_t>(gameSeed)); }
// ScenarioContractEvents.cs:57
void ScenarioContractEvents::OnLoad(ConfigNode& node) {
    L::LoadNode(node, "ScenarioContractEvents", "id", gameSeed, -1, false);
    ProcessGameSeed();
}
// ScenarioContractEvents.cs:63
void ScenarioContractEvents::ProcessGameSeed() {
    if (gameSeed == -1) {
        // new KSPRandom(Environment.TickCount ^ Guid.NewGuid().GetHashCode()); Guid.GetHashCode() = _a ^ ((_b << 16) | (ushort)_c) ^ ((_f << 24) | _k)
        const Guid g = ksp::Contracts::ContractsEnvironment::Current().NewGuid();
        const uint32_t a = static_cast<uint32_t>(g.bytes[0]) | (static_cast<uint32_t>(g.bytes[1]) << 8) | (static_cast<uint32_t>(g.bytes[2]) << 16) |
                           (static_cast<uint32_t>(g.bytes[3]) << 24);
        const uint32_t b = static_cast<uint32_t>(g.bytes[4]) | (static_cast<uint32_t>(g.bytes[5]) << 8);
        const uint32_t c = static_cast<uint32_t>(g.bytes[6]) | (static_cast<uint32_t>(g.bytes[7]) << 8);
        const int32_t h = static_cast<int32_t>(a ^ ((b << 16) | (c & 0xFFFFu)) ^ ((static_cast<uint32_t>(g.bytes[10]) << 24) | g.bytes[15]));
        KSPRandom kSPRandom(ksp::Contracts::ContractsEnvironment::Current().NewRandomSeed() ^ h);
        while (gameSeed == -1) gameSeed = kSPRandom.Next(INT32_MIN, INT32_MAX);
    }
}
// ScenarioContractEvents.cs:74
void ScenarioContractEvents::VesselWasModifiedEvent(Vessel* v) { InvokeCheckSpecificVessels({v->id}); }
// ScenarioContractEvents.cs:79
void ScenarioContractEvents::PartCoupleEvent(GE::FromToAction<Part*, Part*> action) {
    InvokeCheckSpecificVessels({action.from->vessel->id, action.to->vessel->id});
}
// ScenarioContractEvents.cs:84
void ScenarioContractEvents::VesselWillDestroyEvent(Vessel* v) { InvokeCheckSpecificVessels({v->id}); }
// ScenarioContractEvents.cs:89
void ScenarioContractEvents::VesselTerminatedEvent(ProtoVessel* pv) { InvokeCheckSpecificVessels({pv->vesselID}); }
// ScenarioContractEvents.cs:94
void ScenarioContractEvents::VesselRecoveredEvent(ProtoVessel* pv, bool quick) {
    (void)quick;
    InvokeCheckSpecificVessels({pv->vesselID});
    TouristDepartures(eventDelay, pv);
}
// ScenarioContractEvents.cs:100
void ScenarioContractEvents::VesselRenameEvent(GE::HostedFromToAction<Vessel*, std::string> action) {
    Game* game = SaveEnvironment::Current().currentGame;
    if (game->Mode != Game::Modes::CAREER || ksp::Contracts::ContractSystem::Instance == nullptr) return;
    auto& contracts = ksp::Contracts::ContractSystem::Instance->Contracts();
    int count = static_cast<int>(contracts.size());
    while (count-- > 0) {
        ksp::Contracts::Contract* contract = contracts[static_cast<size_t>(count)].get();
        if (dynamic_cast<ICheckSpecificVessels*>(contract) == nullptr) continue;
        std::vector<ksp::Contracts::ContractParameter*> all;
        contract->AllParameters(all);
        for (auto* p : all)
            if (auto* sv = dynamic_cast<ksp::Contracts::Parameters::SpecificVesselParameter*>(p)) sv->SpecificVesselRename(action.host->id, action.to);
    }
}
// ScenarioContractEvents.cs:124 (the iterator takes its crew snapshot at once and acts after WaitForSeconds(delay))
void ScenarioContractEvents::TouristDepartures(float delay, ProtoVessel* pv) {
    Pending p;
    p.remaining = delay;
    p.crew = pv->GetVesselCrew();
    p.pv = pv;
    departures_.push_back(std::move(p));
}
// ScenarioContractEvents.cs:138
void ScenarioContractEvents::InvokeCheckSpecificVessels(const std::vector<Guid>& guids) {
    if (IsInvoking()) invoking_ = false;
    int num = static_cast<int>(guids.size());
    while (num-- > 0) specificVesselCache.insert(Key(guids[static_cast<size_t>(num)]));
    invoking_ = true;
    invokeRemaining_ = eventDelay;
}
// ScenarioContractEvents.cs:151
void ScenarioContractEvents::CheckSpecificVessels() {
    Game* game = SaveEnvironment::Current().currentGame;
    if (game->Mode == Game::Modes::CAREER && ksp::Contracts::ContractSystem::Instance != nullptr) {
        std::vector<Guid> ids;
        for (const auto& k : specificVesselCache) ids.push_back(FromKey(k));
        auto& contracts = ksp::Contracts::ContractSystem::Instance->Contracts();
        int count = static_cast<int>(contracts.size());
        while (count-- > 0) {
            ksp::Contracts::Contract* contract = contracts[static_cast<size_t>(count)].get();
            if (dynamic_cast<ICheckSpecificVessels*>(contract) == nullptr) continue;
            std::vector<ksp::Contracts::ContractParameter*> all;
            contract->AllParameters(all);
            for (auto* p : all)
                if (auto* sv = dynamic_cast<ksp::Contracts::Parameters::SpecificVesselParameter*>(p)) sv->SpecificVesselScan(ids);
        }
        specificVesselCache.clear();
    } else {
        specificVesselCache.clear();
    }
}
// the Unity timers
void ScenarioContractEvents::AdvanceTime(float dt) {
    if (invoking_) {
        invokeRemaining_ -= dt;
        if (invokeRemaining_ <= 0.0f) {
            invoking_ = false;
            CheckSpecificVessels();
        }
    }
    std::vector<Pending> keep;
    std::vector<Pending> due;
    for (auto& p : departures_) {
        p.remaining -= dt;
        (p.remaining <= 0.0f ? due : keep).push_back(std::move(p));
    }
    departures_ = std::move(keep);
    for (auto& p : due) {
        int count = static_cast<int>(p.crew.size());
        while (count-- > 0) {
            auto& protoCrewMember = p.crew[static_cast<size_t>(count)];
            if (protoCrewMember->type() == ProtoCrewMember_KerbalType::Tourist && protoCrewMember->hasToured) {
                if (p.pv != nullptr) p.pv->RemoveCrew(protoCrewMember);
                SaveEnvironment::Current().currentGame->CrewRoster->Remove(protoCrewMember);
            }
        }
    }
}
// ScenarioModuleFactory registration ([KSPScenario])
void ScenarioContractEvents::Register() {
    ScenarioModuleFactory::Register("ScenarioContractEvents", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new ScenarioContractEvents()); });
}

}  // namespace ksp::FinePrint
