// WP-42: SpaceCenterBuilding.cs and its subclasses, LaunchSiteFacility.cs.
#include "ksp/space_center/space_center_building.hpp"

#include <algorithm>
#include <cmath>

#include "ksp/career/currency.hpp"
#include "ksp/career/funding.hpp"
#include "ksp/career/game_variables.hpp"
#include "ksp/career/scenario_misc.hpp"
#include "ksp/career/upgradeables.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/ship/ship_template.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
Game& CurrentGameChecked() {
    Game* game = HighLogic::CurrentGame();
    if (game == nullptr) throw NullReferenceException();
    return *game;
}
void Popup(const std::string& id, const std::string& title, const std::string& message) {
    if (SpaceCenterUI::popup) SpaceCenterUI::popup(id, title, message);
}
void SaveGameOverwrite() {
    if (SpaceCenterUI::saveGame) SpaceCenterUI::saveGame();
}
bool Contains(const std::vector<DestructibleBuilding*>& v, DestructibleBuilding* d) { return std::find(v.begin(), v.end(), d) != v.end(); }
}  // namespace

// ---- SpaceCenterBuilding ----------------------------------------------------------------------------------------------------

SpaceCenterBuilding::~SpaceCenterBuilding() = default;

// SpaceCenterBuilding.cs:103 (the Start coroutine's data part; SetupRenderers / SetupColliders / highlight are WP-47)
bool SpaceCenterBuilding::Start() {
    if (PSystemSetup::Instance == nullptr) throw NullReferenceException();
    spaceCenterFacility = PSystemSetup::Instance->GetSpaceCenterFacility(facilityName);
    if (spaceCenterFacility == nullptr) {
        log::Error("SpaceCenterBuilding: Cannot find a facility of name '" + facilityName + "'");
        return false;
    }
    GE::onGameSceneLoadRequested.Add(this, &SpaceCenterBuilding::OnGameSceneChange);
    eventsAdded_ = true;
    // SetupFacility (:758)
    if (findUpgradeableFacility) upgradeableFacility = findUpgradeableFacility();
    GE::OnKSCFacilityUpgrading.Add(this, &SpaceCenterBuilding::OnFacilityUpgrading);
    GE::OnKSCFacilityUpgraded.Add(this, &SpaceCenterBuilding::OnFacilityUpgraded);
    upgradeEventsAdded_ = true;
    SetupDestructibles();
    OnStart();
    return true;
}

// SpaceCenterBuilding.cs:122
void SpaceCenterBuilding::OnDestroy() {
    if (eventsAdded_) {
        GE::onGameSceneLoadRequested.Remove(this, &SpaceCenterBuilding::OnGameSceneChange);
        eventsAdded_ = false;
    }
    SpaceCenterUI::RemoveLock(buildingInfoName + "_ContextMenu");
    OnOnDestroy();
}

// SpaceCenterBuilding.cs:130 (Object.Destroy(gameObject) afterwards is the host's)
void SpaceCenterBuilding::OnGameSceneChange(GameScenes scene) {
    (void)scene;
    UnregisterDestructibles();
    UnregisterUpgradeables();
}

// SpaceCenterBuilding.cs:373
void SpaceCenterBuilding::OnLeftClick() {
    if (GetStructureDamage() < 70.0f) {
        EnterBuilding();
    } else {
        log::Info(buildingInfoName + " is not operational");
    }
}

// SpaceCenterBuilding.cs:387 (the context menu itself is UI: KSCFacilityContextMenu.Create -> OnContextMenuDismissed)
void SpaceCenterBuilding::OnRightClick() {}

// SpaceCenterBuilding.cs:458
void SpaceCenterBuilding::EnterBuilding() {
    if (HighLogic::LoadedSceneIsGame())
        OnClicked();
    else
        log::Info(buildingInfoName + " is up-and-running");
}

// SpaceCenterBuilding.cs:475
std::vector<ProtoVessel*> SpaceCenterBuilding::FindVesselsAtFacility(const FlightState& st, const std::vector<DestructibleBuilding*>& structuresToCheck) {
    std::vector<ProtoVessel*> list;
    for (DestructibleBuilding* item : structuresToCheck) {
        for (ProtoVessel* item2 : ShipConstruction::FindVesselsLandedAt(&st, item->tag)) {
            if (std::find(list.begin(), list.end(), item2) == list.end()) list.push_back(item2);   // AddUnique
        }
    }
    return list;
}

// SpaceCenterBuilding.cs:488
std::vector<ProtoVessel*> SpaceCenterBuilding::FindVesselsAtGrounds(const FlightState& st) {
    std::vector<std::string> list = facilityRootTags ? facilityRootTags() : std::vector<std::string>();
    std::vector<ProtoVessel*> list2;
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        for (ProtoVessel* pv : ShipConstruction::FindVesselsLandedAt(&st, list[static_cast<size_t>(count)]))
            if (std::find(list2.begin(), list2.end(), pv) == list2.end()) list2.push_back(pv);   // AddUniqueRange
    }
    return list2;
}

// SpaceCenterBuilding.cs:501
bool SpaceCenterBuilding::WarnOfObstructingVessels(bool includeGrounds, bool onlyDestroyed) {
    std::vector<ProtoVessel*> list;
    std::vector<std::string> list2;
    int num = static_cast<int>(destructibles.size());
    while (num-- > 0) {
        DestructibleBuilding* d = destructibles[static_cast<size_t>(num)];
        if (d != nullptr && (!onlyDestroyed || (d->IsDestroyed() && !d->tag.empty()))) {
            if (std::find(list2.begin(), list2.end(), d->tag) == list2.end()) list2.push_back(d->tag);   // AddUnique
        }
    }
    if (includeGrounds && groundTags) {
        for (const std::string& t : groundTags())
            if (std::find(list2.begin(), list2.end(), t) == list2.end()) list2.push_back(t);   // KSPUtil.FindTagsInChildren (adds unique)
    }
    FlightState& st = *CurrentGameChecked().flightState;
    const int count = static_cast<int>(st.protoVessels.size());
    for (int i = 0; i < count; i++) {
        ProtoVessel* protoVessel = st.protoVessels[static_cast<size_t>(i)].get();
        int count2 = static_cast<int>(list2.size());
        while (count2-- > 0) {
            if (protoVessel->landedAt.find(list2[static_cast<size_t>(count2)]) != std::string::npos) {
                list.push_back(protoVessel);
                break;
            }
        }
    }
    if (!list.empty()) {
        Popup("VesselObstruction", Localizer::Format(buildingInfoName) + " " + Localizer::Format("#autoLOC_6002254"),
              Localizer::Format("#autoLOC_6002252", {buildingInfoName}) + KSPUtil::PrintCollection(list, "\n", [](ProtoVessel* p) { return p->vesselName; }) +
                  Localizer::Format("#autoLOC_6002253"));
        return true;
    }
    return false;
}

// SpaceCenterBuilding.cs:542
void SpaceCenterBuilding::SetupDestructibles() {
    if (findDestructibles) destructibles = findDestructibles();
    structuralDamage_ = GetStructureDamage();
    operational_ = IsFacilityOperational(structuralDamage_);
    GE::OnKSCStructureCollapsing.Add(this, &SpaceCenterBuilding::OnStructureCollapsing);
    GE::OnKSCStructureCollapsed.Add(this, &SpaceCenterBuilding::OnStructureCollapsed);
    GE::OnKSCStructureRepairing.Add(this, &SpaceCenterBuilding::OnStructureRepairing);
    GE::OnKSCStructureRepaired.Add(this, &SpaceCenterBuilding::OnStructureRepaired);
    structureEventsAdded_ = true;
}

// SpaceCenterBuilding.cs:553
void SpaceCenterBuilding::OnStructureCollapsing(DestructibleBuilding* dBld) {
    if (!(ScenarioDestructibles::Instance != nullptr) && HighLogic::LoadedSceneIsGame()) return;
    int num = static_cast<int>(destructibles.size());
    do {
        if (num-- <= 0) return;
    } while (destructibles[static_cast<size_t>(num)] != dBld);
    structuralDamage_ = GetStructureDamage();
    operational_ = IsFacilityOperational(structuralDamage_);
}

// SpaceCenterBuilding.cs:572 (SetupRenderers / SetupColliders: WP-47)
void SpaceCenterBuilding::OnStructureCollapsed(DestructibleBuilding* dBld) { (void)dBld; }

// SpaceCenterBuilding.cs:578
void SpaceCenterBuilding::OnStructureRepairing(DestructibleBuilding* dBld) {
    int num = static_cast<int>(destructibles.size());
    do {
        if (num-- <= 0) return;
    } while (destructibles[static_cast<size_t>(num)] != dBld);
    SpaceCenterUI::SetLock(facilityName + "_" + dBld->id + "_Repairs");
}

// SpaceCenterBuilding.cs:592
void SpaceCenterBuilding::OnStructureRepaired(DestructibleBuilding* dBld) {
    bool flag = false;
    int num = static_cast<int>(destructibles.size());
    while (num-- > 0) {
        if (destructibles[static_cast<size_t>(num)] == dBld) {
            flag = true;
            break;
        }
    }
    if ((ScenarioDestructibles::Instance != nullptr || !HighLogic::LoadedSceneIsGame()) && flag) {
        structuralDamage_ = GetStructureDamage();
        operational_ = IsFacilityOperational(structuralDamage_);
    }
    if (flag) SpaceCenterUI::RemoveLock(facilityName + "_" + dBld->id + "_Repairs");
}

// SpaceCenterBuilding.cs:615
float SpaceCenterBuilding::GetStructureDamage() const {
    float num = 0.0f;
    int num2 = static_cast<int>(destructibles.size());
    while (num2-- > 0) {
        if (!destructibles[static_cast<size_t>(num2)]->IsIntact()) num = f32store(fx(num) + fx(destructibles[static_cast<size_t>(num2)]->FacilityDamageFraction()));
    }
    return num;
}

// SpaceCenterBuilding.cs:629
bool SpaceCenterBuilding::IsFacilityOperational(float damage) const { return damage < 100.0f; }

// SpaceCenterBuilding.cs:634
std::string SpaceCenterBuilding::GetStructureDamageLevel(float lvl) {
    if (lvl <= 0.0f) return Localizer::Format("#autoLOC_7003217");
    if (lvl <= 20.0f) return Localizer::Format("#autoLOC_7003218");
    if (lvl <= 50.0f) return Localizer::Format("#autoLOC_7003219");
    if (lvl <= 80.0f) return Localizer::Format("#autoLOC_7003220");
    return Localizer::Format("#autoLOC_7003221");
}

// SpaceCenterBuilding.cs:655
float SpaceCenterBuilding::GetRepairsCost() const {
    float num = 0.0f;
    int num2 = static_cast<int>(destructibles.size());
    while (num2-- > 0) {
        if (destructibles[static_cast<size_t>(num2)]->IsDestroyed()) num = f32store(fx(num) + fx(destructibles[static_cast<size_t>(num2)]->RepairCost));
    }
    return f32store(fx(num) * fx(CurrentGameChecked().Parameters->Career.FundsLossMultiplier));
}

// SpaceCenterBuilding.cs:669
float SpaceCenterBuilding::GetCollapseReputationHit() const {
    float num = 0.0f;
    int num2 = static_cast<int>(destructibles.size());
    while (num2-- > 0) {
        if (!destructibles[static_cast<size_t>(num2)]->IsIntact()) num = f32store(fx(num) + fx(destructibles[static_cast<size_t>(num2)]->CollapseReputationHit));
    }
    return f32store(fx(num) * fx(CurrentGameChecked().Parameters->Career.RepLossMultiplier));
}

// SpaceCenterBuilding.cs:683
void SpaceCenterBuilding::RepairFacility(bool deduceFunds) {
    if (deduceFunds) {
        if (!CurrencyModifierQuery::RunQuery(TransactionReasons::StructureRepair, f32store(0.0 - fx(GetRepairsCost())), 0.0f, 0.0f).CanAfford()) {
            if (SpaceCenterUI::screenMessage) SpaceCenterUI::screenMessage(Localizer::Format("#autoLOC_7003210"));
            return;
        }
        if (Funding::Instance == nullptr) throw NullReferenceException();
        Funding::Instance->AddFunds(0.0 - std::fabs(static_cast<double>(GetRepairsCost())), TransactionReasons::StructureRepair);
    }
    RepairStructures();
}

// SpaceCenterBuilding.cs:697
void SpaceCenterBuilding::DemolishFacility() {
    if (destructibles.empty()) {
        if (SpaceCenterUI::screenMessage) SpaceCenterUI::screenMessage("<color=orange><b>" + Localizer::Format("#autoLOC_7003213") + "</b></color>");
    } else {
        for (DestructibleBuilding* d : destructibles) d->Demolish();
    }
    SaveGameOverwrite();
}

// SpaceCenterBuilding.cs:714 (the delayed save is a coroutine on the shared scheduler)
void SpaceCenterBuilding::RepairStructures() {
    float num = 0.0f;
    if (destructibles.empty()) {
        if (SpaceCenterUI::screenMessage) SpaceCenterUI::screenMessage("<color=orange><b>" + Localizer::Format("#autoLOC_7003214") + "</b></color>");
    } else {
        for (DestructibleBuilding* d : destructibles) {
            d->Repair();
            num = std::max(num, d->GetRepairSequenceDuration());
        }
    }
    struct Delayed {
        static Enumerator Run(float delay) {
            co_yield YieldInstruction::WaitForSeconds(delay);
            SaveGameOverwrite();
        }
    };
    HighLogic::Scheduler().StartCoroutine(Delayed::Run(f32store(fx(num) + 0.1)), owner);
}

// SpaceCenterBuilding.cs:736
void SpaceCenterBuilding::ResetStructures() {
    for (DestructibleBuilding* d : destructibles) d->Reset();
}

// SpaceCenterBuilding.cs:745
void SpaceCenterBuilding::UnregisterDestructibles() {
    if (structureEventsAdded_) {
        GE::OnKSCStructureCollapsing.Remove(this, &SpaceCenterBuilding::OnStructureCollapsing);
        GE::OnKSCStructureCollapsed.Remove(this, &SpaceCenterBuilding::OnStructureCollapsed);
        GE::OnKSCStructureRepairing.Remove(this, &SpaceCenterBuilding::OnStructureRepairing);
        GE::OnKSCStructureRepaired.Remove(this, &SpaceCenterBuilding::OnStructureRepaired);
        structureEventsAdded_ = false;
    }
    for (DestructibleBuilding* d : destructibles) SpaceCenterUI::RemoveLock(facilityName + "_" + d->id + "_Repairs");
}

// SpaceCenterBuilding.cs:769
void SpaceCenterBuilding::OnFacilityUpgrading(Upgradeables::UpgradeableFacility* facility, int level) {
    (void)level;
    if (facility == upgradeableFacility) UnregisterDestructibles();
}

// SpaceCenterBuilding.cs:777
void SpaceCenterBuilding::OnFacilityUpgraded(Upgradeables::UpgradeableFacility* facility, int level) {
    (void)level;
    if (facility == upgradeableFacility) {
        if (findUpgradeableFacility) upgradeableFacility = findUpgradeableFacility();
        SetupDestructibles();
    } else {
        if (!hasUpgradeableSlave) return;
        int num = static_cast<int>(neighbourIDs.size());
        do {
            if (num-- <= 0) return;
        } while (neighbourIDs[static_cast<size_t>(num)] != facility->id);
        if (findUpgradeableFacility) upgradeableFacility = findUpgradeableFacility();
        SetupDestructibles();
    }
}

// SpaceCenterBuilding.cs:822
void SpaceCenterBuilding::UnregisterUpgradeables() {
    if (upgradeEventsAdded_) {
        GE::OnKSCFacilityUpgraded.Remove(this, &SpaceCenterBuilding::OnFacilityUpgraded);
        GE::OnKSCFacilityUpgrading.Remove(this, &SpaceCenterBuilding::OnFacilityUpgrading);
        upgradeEventsAdded_ = false;
    }
}

// SpaceCenterBuilding.cs:828
void SpaceCenterBuilding::UpgradeFacility(bool deduceFunds) {
    if (upgradeableFacility == nullptr) throw NullReferenceException();
    if (upgradeableFacility->FacilityLevel() == upgradeableFacility->MaxLevel()) {
        if (SpaceCenterUI::screenMessage) SpaceCenterUI::screenMessage(Localizer::Format("#autoLOC_7003215"));
        return;
    }
    if (deduceFunds) {
        if (!CurrencyModifierQuery::RunQuery(TransactionReasons::StructureConstruction, f32store(0.0 - fx(upgradeableFacility->GetUpgradeCost())), 0.0f, 0.0f).CanAfford()) {
            if (SpaceCenterUI::screenMessage) SpaceCenterUI::screenMessage(Localizer::Format("#autoLOC_7003211"));
            return;
        }
        if (Funding::Instance == nullptr) throw NullReferenceException();
        Funding::Instance->AddFunds(0.0 - std::fabs(static_cast<double>(upgradeableFacility->GetUpgradeCost())), TransactionReasons::StructureConstruction);
    }
    ResetStructures();
    upgradeableFacility->SetLevel(upgradeableFacility->FacilityLevel() + 1);
    SaveGameOverwrite();
}

// SpaceCenterBuilding.cs:849
void SpaceCenterBuilding::DowngradeFacility(bool deduceFunds) {
    if (upgradeableFacility == nullptr) throw NullReferenceException();
    if (upgradeableFacility->FacilityLevel() == 0) {
        if (SpaceCenterUI::screenMessage) SpaceCenterUI::screenMessage(Localizer::Format("#autoLOC_7003216"));
        return;
    }
    if (deduceFunds) {
        if (!CurrencyModifierQuery::RunQuery(TransactionReasons::StructureConstruction, f32store(0.0 - fx(upgradeableFacility->GetDowngradeCost())), 0.0f, 0.0f).CanAfford()) {
            if (SpaceCenterUI::screenMessage) SpaceCenterUI::screenMessage(Localizer::Format("#autoLOC_7003212"));
            return;
        }
        if (Funding::Instance == nullptr) throw NullReferenceException();
        Funding::Instance->AddFunds(0.0 - std::fabs(static_cast<double>(upgradeableFacility->GetDowngradeCost())), TransactionReasons::StructureConstruction);
    }
    ResetStructures();
    upgradeableFacility->SetLevel(upgradeableFacility->FacilityLevel() - 1);
    SaveGameOverwrite();
}

// SpaceCenterBuilding.cs:412 (the Demolish confirmation dialog is UI: `demolishConfirmed` is the user's answer)
void SpaceCenterBuilding::OnContextMenuDismissed(DismissAction dma, bool demolishConfirmed) {
    const bool funding = Funding::Instance != nullptr;
    switch (dma) {
        case DismissAction::Enter: EnterBuilding(); break;
        case DismissAction::Repair:
            if (!WarnOfObstructingVessels(false, true)) RepairFacility(funding);
            break;
        case DismissAction::Upgrade:
            if (!WarnOfObstructingVessels(true, false)) UpgradeFacility(funding);
            break;
        case DismissAction::Demolish:
            if (!WarnOfObstructingVessels(true, false)) {
                if (demolishConfirmed) DemolishFacility();
            }
            break;
        case DismissAction::Downgrade:
            if (!WarnOfObstructingVessels(true, false)) DowngradeFacility(funding);
            break;
        case DismissAction::Cancel: break;
    }
    SpaceCenterUI::RemoveLock(buildingInfoName + "_ContextMenu");
}

// ---- subclasses -------------------------------------------------------------------------------------------------------------------

namespace {
bool ModeIsNonResumable() { return CurrentGameChecked().Mode == Game_Modes::SCENARIO_NON_RESUMABLE; }
}  // namespace

// VehicleAssemblyBuilding.cs:7
bool VehicleAssemblyBuilding::IsOpen() { return !ModeIsNonResumable(); }

// VehicleAssemblyBuilding.cs:12
void VehicleAssemblyBuilding::OnClicked() {
    Game& game = CurrentGameChecked();
    if (game.Mode == Game_Modes::SCENARIO_NON_RESUMABLE) return;
    bool flag = false;
    if (!game.Parameters->SpaceCenter.CanGoInVAB && game.Mode == Game_Modes::MISSION && missionVesselsArePending && missionVesselsArePending()) flag = true;
    if (game.Parameters->SpaceCenter.CanGoInVAB || flag) {
        EditorFacility editorFacility = EditorFacility::None;
        if (ShipConstruction::ShipConfig != nullptr) editorFacility = ShipConstruction::ShipType;
        EditorDriver::StartupBehaviour = editorFacility == EditorFacility::VAB ? EditorDriver::StartupBehaviours::LOAD_FROM_CACHE : EditorDriver::StartupBehaviours::START_CLEAN;
        SaveGameOverwrite();
        EditorDriver::StartEditor(EditorFacility::VAB);
    } else {
        SpaceCenterUI::SetLock("VABFacility");
        Popup("FacilityLocked", Localizer::Format("#autoLOC_418766"), Localizer::Format("#autoLOC_7003201"));
    }
}

// SpacePlaneHangarBuilding.cs:7
bool SpacePlaneHangarBuilding::IsOpen() { return !ModeIsNonResumable(); }

// SpacePlaneHangarBuilding.cs:12
void SpacePlaneHangarBuilding::OnClicked() {
    Game& game = CurrentGameChecked();
    if (game.Mode == Game_Modes::SCENARIO_NON_RESUMABLE) return;
    bool flag = false;
    if (!game.Parameters->SpaceCenter.CanGoInSPH && game.Mode == Game_Modes::MISSION && VehicleAssemblyBuilding::missionVesselsArePending &&
        VehicleAssemblyBuilding::missionVesselsArePending())
        flag = true;
    if (game.Parameters->SpaceCenter.CanGoInSPH || flag) {
        SaveGameOverwrite();
        EditorFacility editorFacility = EditorFacility::None;
        if (ShipConstruction::ShipConfig != nullptr) editorFacility = ShipConstruction::ShipType;
        log::Info("Going to Hangar! CachedShipType: " + dotnet::EnumToString(editorFacility));
        EditorDriver::StartupBehaviour = editorFacility == EditorFacility::SPH ? EditorDriver::StartupBehaviours::LOAD_FROM_CACHE : EditorDriver::StartupBehaviours::START_CLEAN;
        EditorDriver::StartEditor(EditorFacility::SPH);
    } else {
        SpaceCenterUI::SetLock("SPHFacility");
        Popup("FacilityLocked", Localizer::Format("#autoLOC_6001661"), Localizer::Format("#autoLOC_7003201"));
    }
}

// TrackingStationBuilding.cs:6
bool TrackingStationBuilding::IsOpen() { return !ModeIsNonResumable(); }

// TrackingStationBuilding.cs:11
void TrackingStationBuilding::OnClicked() {
    Game& game = CurrentGameChecked();
    if (game.Mode != Game_Modes::SCENARIO_NON_RESUMABLE) {
        if (game.Parameters->SpaceCenter.CanGoInTrackingStation) {
            SaveGameOverwrite();
            HighLogic::LoadScene(GameScenes::TRACKSTATION);
        } else {
            SpaceCenterUI::SetLock("TSFacility");
            Popup("FacilityLocked", Localizer::Format("#autoLOC_6002126"), Localizer::Format("#autoLOC_7003201"));
        }
    }
}

// AstronautComplexFacility.cs:8
bool AstronautComplexFacility::IsOpen() { return !ModeIsNonResumable(); }

// AstronautComplexFacility.cs:13 (CrewRoster.CheckRosterRespawn is subscribed to onGUIAstronautComplexSpawn: WP-46's roster call; the
// event wiring below is the same)
void AstronautComplexFacility::OnClicked() {
    Game& game = CurrentGameChecked();
    if (game.Parameters->SpaceCenter.CanGoToAstronautC) {
        if (game.Mode != Game_Modes::SCENARIO_NON_RESUMABLE) {
            GE::onGUIAstronautComplexSpawn.Fire();
            SpaceCenterUI::SetLock("astronautComplexFacility");
            dialogIsUp_ = true;
        } else {
            SpaceCenterUI::SetLock("ACFacility");
            Popup("FacilityClosed", Localizer::Format("#autoLOC_6001643"), Localizer::Format("#autoLOC_7003200"));
        }
    } else {
        SpaceCenterUI::SetLock("ACFacility");
        Popup("FacilityLocked", Localizer::Format("#autoLOC_6001643"), Localizer::Format("#autoLOC_7003201"));
    }
}

// AstronautComplexFacility.cs:46
void AstronautComplexFacility::onAstronautComplexDialogClose() {
    dialogIsUp_ = false;
    SpaceCenterUI::RemoveLock("astronautComplexFacility");
}

// AstronautComplexFacility.cs:54
void AstronautComplexFacility::OnOnDestroy() { dialogIsUp_ = false; }

// AdministrationFacility.cs:9
bool AdministrationFacility::IsOpen() { return strategySystemPresent && strategySystemPresent(); }

// AdministrationFacility.cs:14
void AdministrationFacility::OnClicked() {
    SpaceCenterUI::SetLock("administrationFacility");
    if (CurrentGameChecked().Parameters->SpaceCenter.CanGoToAdmin) {
        if (strategySystemPresent && strategySystemPresent()) {
            GE::onGUIAdministrationFacilitySpawn.Fire();
            dialogIsUp_ = true;
        } else {
            Popup("FacilityClosed", Localizer::Format("#autoLOC_6001644"), Localizer::Format("#autoLOC_7003200"));
        }
    } else {
        Popup("FacilityLocked", Localizer::Format("#autoLOC_6001644"), Localizer::Format("#autoLOC_7003201"));
    }
}

// AdministrationFacility.cs:44
void AdministrationFacility::onDialogClose() {
    dialogIsUp_ = false;
    SpaceCenterUI::RemoveLock("administrationFacility");
}

// AdministrationFacility.cs:51
void AdministrationFacility::OnOnDestroy() { dialogIsUp_ = false; }

// MissionControlBuilding.cs:9
bool MissionControlBuilding::IsOpen() { return contractSystemPresent && contractSystemPresent(); }

// MissionControlBuilding.cs:14
void MissionControlBuilding::OnClicked() {
    SpaceCenterUI::SetLock("missionControlFacility");
    if (CurrentGameChecked().Parameters->SpaceCenter.CanGoToMissionControl) {
        if (contractSystemPresent && contractSystemPresent()) {
            GE::onGUIMissionControlSpawn.Fire();
            dialogIsUp_ = true;
        } else {
            Popup("FacilityClosed", Localizer::Format("#autoLOC_6001645"), Localizer::Format("#autoLOC_7003200"));
        }
    } else {
        Popup("FacilityLocked", Localizer::Format("#autoLOC_6001645"), Localizer::Format("#autoLOC_7003201"));
    }
}

// MissionControlBuilding.cs:42
void MissionControlBuilding::onDialogClose() {
    dialogIsUp_ = false;
    SpaceCenterUI::RemoveLock("missionControlFacility");
}

// MissionControlBuilding.cs:49
void MissionControlBuilding::OnOnDestroy() { dialogIsUp_ = false; }

// RnDBuilding.cs:8
bool RnDBuilding::IsOpen() { return rndPresent && rndPresent(); }

// RnDBuilding.cs:13
void RnDBuilding::OnClicked() {
    SpaceCenterUI::SetLock("RnDFacility");
    if (CurrentGameChecked().Parameters->SpaceCenter.CanGoToRnD) {
        if (rndPresent && rndPresent()) {
            GE::onGUIRnDComplexSpawn.Fire();
            dialogIsUp_ = true;
        } else {
            Popup("FacilityClosed", Localizer::Format("#autoLOC_6001646"), Localizer::Format("#autoLOC_7003200"));
        }
    } else {
        Popup("FacilityLocked", Localizer::Format("#autoLOC_6001646"), Localizer::Format("#autoLOC_7003201"));
    }
}

// RnDBuilding.cs:41
void RnDBuilding::onDialogClose() {
    dialogIsUp_ = false;
    SpaceCenterUI::RemoveLock("RnDFacility");
}

// RnDBuilding.cs:48
void RnDBuilding::OnOnDestroy() { dialogIsUp_ = false; }

// FlagPoleFacility.cs:10
void FlagPoleFacility::OnClicked() {
    if (CurrentGameChecked().Parameters->SpaceCenter.CanSelectFlag) {
        if (spawnFlagBrowser) spawnFlagBrowser();
        SpaceCenterUI::SetLock("flagPoleFacility");
        return;
    }
    SpaceCenterUI::SetLock("flagPoleFacility");
    Popup("FacilityLocked", Localizer::Format("#autoLOC_6002147"), Localizer::Format("#autoLOC_7003201"));
}

// FlagPoleFacility.cs:34
void FlagPoleFacility::OnFlagSelect(const std::string& textureName) {
    Game* game = HighLogic::CurrentGame();
    if (game != nullptr) {
        game->flagURL = textureName;
        GE::onFlagSelect.Fire(*game->flagURL);
    }
    GE::onFlagSelect.Fire(textureName);
    SpaceCenterUI::RemoveLock("flagPoleFacility");
}

// FlagPoleFacility.cs:45
void FlagPoleFacility::OnFlagCancel() { SpaceCenterUI::RemoveLock("flagPoleFacility"); }

// ---- LaunchSiteFacility -------------------------------------------------------------------------------------------------------

// LaunchSiteFacility.cs:48
void LaunchSiteFacility::OnStart() {
    missionDialogs.clear();
    constructionFacilityName = Localizer::Format(constructionFacilityName);
    GE::onGUILaunchScreenDespawn.Add(this, &LaunchSiteFacility::Cancel);
    dialogEventsAdded_ = true;
}

// LaunchSiteFacility.cs:60
void LaunchSiteFacility::OnOnDestroy() {
    SpaceCenterUI::RemoveLock("launchSiteFacility");
    if (dialogEventsAdded_) {
        GE::onGUILaunchScreenDespawn.Remove(this, &LaunchSiteFacility::Cancel);
        dialogEventsAdded_ = false;
    }
    GE::onGUIRecoveryDialogSpawn.Remove(this, &LaunchSiteFacility::onMissionDialogUp);
    GE::onGUIRecoveryDialogDespawn.Remove(this, &LaunchSiteFacility::onMissionDialogDismiss);
}

// LaunchSiteFacility.cs:68
bool LaunchSiteFacility::IsOpen() { return !ModeIsNonResumable(); }

// the launchSiteDisplayName switch of :92 and :304
std::string LaunchSiteFacility::LaunchSiteDisplayName() const {
    std::string launchSiteDisplayName = launchSiteName;
    if (launchSiteName != "LaunchPad" && launchSiteName != "Runway") {
        if (PSystemSetup::Instance == nullptr) throw NullReferenceException();
        launchSiteDisplayName = PSystemSetup::Instance->GetLaunchSiteDisplayName(launchSiteName);
    } else {
        switch (facilityType) {
            case EditorFacility::SPH: launchSiteDisplayName = ScenarioUpgradeableFacilities::GetFacilityName(SpaceCenterFacility::Runway); break;
            case EditorFacility::VAB: launchSiteDisplayName = ScenarioUpgradeableFacilities::GetFacilityName(SpaceCenterFacility::LaunchPad); break;
            default: break;
        }
    }
    return launchSiteDisplayName;
}

// LaunchSiteFacility.cs:73
void LaunchSiteFacility::OnClicked() {
    Game& game = CurrentGameChecked();
    if (game.Mode == Game_Modes::SCENARIO_NON_RESUMABLE) return;
    if (game.Mode != Game_Modes::MISSION && game.Parameters->SpaceCenter.CanLaunchAtPad) {
        showShipSelection();
    } else if (game.Mode == Game_Modes::MISSION) {
        if (!game.Parameters->CustomParams<Expansions::Missions::MissionParamsExtras>().facilityOpenEditor) {
            showFacilityLocked();
            return;
        }
        if (game.Parameters->CustomParams<Expansions::Missions::MissionParamsGeneral>().preventVesselRecovery) {
            const std::string launchSiteDisplayName = LaunchSiteDisplayName();
            launchSiteClearTest_ = std::make_unique<PreFlightTests::LaunchSiteClear>(launchSiteName, launchSiteDisplayName);
            if (!launchSiteClearTest_->Test()) {
                Popup("LaunchSiteWarning", launchSiteClearTest_->GetWarningTitle(), launchSiteClearTest_->GetWarningDescription() + "\n" + Localizer::Format("#autoLOC_8002108"));
                SpaceCenterUI::SetLock("launchSiteFacility");
                return;
            }
            showShipSelection();
        } else {
            showShipSelection();
        }
    } else {
        showFacilityLocked();
    }
}

// LaunchSiteFacility.cs:136
void LaunchSiteFacility::showFacilityLocked() {
    SpaceCenterUI::SetLock("launchSiteFacility");
    Popup("FacilityLocked", Localizer::Format(buildingInfoName), Localizer::Format("#autoLOC_7003201"));
}

// LaunchSiteFacility.cs:147
void LaunchSiteFacility::ResumeFlightOnSite() {
    GE::onGUIRecoveryDialogSpawn.Remove(this, &LaunchSiteFacility::onMissionDialogUp);
    log::Info("Resuming Flight on " + launchSiteName);
    Cancel();
    SaveCurrentGameAs("persistent", GameScenes::FLIGHT);
    FlightDriver::StartAndFocusVessel("persistent", launchSiteClearTest_->GetObstructingVesselIndex());
}

// LaunchSiteFacility.cs:156
void LaunchSiteFacility::ClearSite() {
    GE::onGUIRecoveryDialogSpawn.Add(this, &LaunchSiteFacility::onMissionDialogUp);
    FlightState& st = *CurrentGameChecked().flightState;
    // launchSiteClearTest.GetObstructingVessels(): the proto vessels landed at the site
    for (ProtoVessel* obstructing : ShipConstruction::FindVesselsLandedAt(&st, launchSiteName)) ShipConstruction::RecoverVesselFromFlight(*obstructing, st);
}

// LaunchSiteFacility.cs:165
void LaunchSiteFacility::onMissionDialogUp(MissionRecoveryDialogHandle* dialog) {
    if (missionDialogs.empty()) GE::onGUIRecoveryDialogDespawn.Add(this, &LaunchSiteFacility::onMissionDialogDismiss);
    missionDialogs.push_back(dialog);
}

// LaunchSiteFacility.cs:174
void LaunchSiteFacility::onMissionDialogDismiss(MissionRecoveryDialogHandle* dialog) {
    missionDialogs.erase(std::remove(missionDialogs.begin(), missionDialogs.end(), dialog), missionDialogs.end());
    if (missionDialogs.empty()) {
        GE::onGUIRecoveryDialogDespawn.Remove(this, &LaunchSiteFacility::onMissionDialogDismiss);
        if (awaitingLaunchClear_)
            launchChecks();
        else
            showShipSelection();
    }
}

// LaunchSiteFacility.cs:191
void LaunchSiteFacility::Cancel() {
    GE::onGUIRecoveryDialogSpawn.Remove(this, &LaunchSiteFacility::onMissionDialogUp);
    if (dialogEventsAdded_) GE::onGUILaunchScreenDespawn.Remove(this, &LaunchSiteFacility::Cancel);
    GE::onGUILaunchScreenDespawn.Fire();
    if (dialogEventsAdded_) GE::onGUILaunchScreenDespawn.Add(this, &LaunchSiteFacility::Cancel);
    SpaceCenterUI::RemoveLock("launchSiteFacility");
}

// LaunchSiteFacility.cs:198
void LaunchSiteFacility::showShipSelection() {
    Game& game = CurrentGameChecked();
    EditorDriver::editorFacility = facilityType;
    EditorDriver::setupValidLaunchSites();
    if (static_cast<uint32_t>(facilityType) > 1u && facilityType == EditorFacility::SPH)
        EditorDriver::setLaunchSite(game.defaultSPHLaunchSite);
    else
        EditorDriver::setLaunchSite(game.defaultVABLaunchSite);
    launchSiteName = EditorDriver::SelectedLaunchSiteName();
    int craftFiles = 0;
    if (craftFileCount) {
        const bool stock = game.Parameters->Difficulty.AllowStockVessels;
        bool anyExpansion = false;
        for (const auto& e : SaveEnvironment::Current().expansions) anyExpansion = anyExpansion || e.installed;   // ExpansionsLoader.IsAnyExpansionInstalled
        const bool expansions = stock && anyExpansion;
        craftFiles = craftFileCount(craftSubfolder, stock, expansions);
    }
    if (craftFiles > 0) {
        GE::onGUILaunchScreenSpawn.Fire(GE::VesselSpawnInfo(craftSubfolder, HighLogic::SaveFolder(),
                                                            Delegate<void(std::string, std::string, VesselCrewManifest*)>::Bind(this, &LaunchSiteFacility::shipSelected), nullptr));
    } else {
        NothingToLaunch();
    }
    SpaceCenterUI::SetLock("launchSiteFacility");
}

// LaunchSiteFacility.cs:289
void LaunchSiteFacility::NothingToLaunch() { Popup("NoVesselForLaunch", Localizer::Format("#autoLOC_350955", {constructionFacilityName}), Localizer::Format("#autoLOC_350954")); }

// LaunchSiteFacility.cs:294
void LaunchSiteFacility::shipSelected(const std::string& path, const std::string& flag, VesselCrewManifest* manifest) {
    SpaceCenterUI::RemoveLock("launchSiteFacility");
    if (path.empty()) return;
    path_ = path;
    flag_ = flag;
    manifest_ = manifest;
    const std::string launchSiteDisplayName = LaunchSiteDisplayName();
    launchSiteClearTest_ = std::make_unique<PreFlightTests::LaunchSiteClear>(launchSiteName, launchSiteDisplayName);
    if (launchSiteClearTest_->Test()) {
        awaitingLaunchClear_ = true;
        launchChecks();
        return;
    }
    Popup("LaunchSiteWarning", launchSiteClearTest_->GetWarningTitle(), launchSiteClearTest_->GetWarningDescription());
    SpaceCenterUI::SetLock("launchSiteFacility");
}

// LaunchSiteFacility.cs:352
void LaunchSiteFacility::launchChecks() {
    if (!missionDialogs.empty() || !awaitingLaunchClear_) return;
    GE::onGUIRecoveryDialogSpawn.Remove(this, &LaunchSiteFacility::onMissionDialogUp);
    GE::onGUIRecoveryDialogDespawn.Remove(this, &LaunchSiteFacility::onMissionDialogDismiss);
    awaitingLaunchClear_ = false;
    fileName_ = path_;
    flagURL_ = flag_;
    crewManifest_ = manifest_;
    ShipTemplate shipTemplate;
    ConfigNode::NodePtr root = ConfigNode::Load(path_);
    shipTemplate.LoadShip(root);
    preFlightCheck_ = std::make_unique<PreFlightCheck>([this] { launchVessel(); }, [this] { ReturnToDialog(); });
    GameVariables* gv = GameVariables::Instance;
    if (gv == nullptr) throw NullReferenceException();
    switch (facilityType) {
        case EditorFacility::SPH:
            preFlightCheck_->AddTest(std::make_unique<PreFlightTests::CraftWithinPartCountLimit>(
                shipTemplate, SpaceCenterFacility::SpaceplaneHangar, gv->GetPartCountLimit(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::SpaceplaneHangar), false)));
            if (launchSiteName == "Runway") {
                preFlightCheck_->AddTest(std::make_unique<PreFlightTests::CraftWithinSizeLimits>(
                    shipTemplate, SpaceCenterFacility::Runway, gv->GetCraftSizeLimit(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::Runway), false)));
                preFlightCheck_->AddTest(std::make_unique<PreFlightTests::CraftWithinMassLimits>(
                    shipTemplate, SpaceCenterFacility::Runway, gv->GetCraftMassLimit(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::Runway), false)));
            }
            break;
        case EditorFacility::VAB:
            preFlightCheck_->AddTest(std::make_unique<PreFlightTests::CraftWithinPartCountLimit>(
                shipTemplate, SpaceCenterFacility::VehicleAssemblyBuilding, gv->GetPartCountLimit(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::VehicleAssemblyBuilding), true)));
            if (launchSiteName == "LaunchPad") {
                preFlightCheck_->AddTest(std::make_unique<PreFlightTests::CraftWithinSizeLimits>(
                    shipTemplate, SpaceCenterFacility::LaunchPad, gv->GetCraftSizeLimit(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::LaunchPad), true)));
                preFlightCheck_->AddTest(std::make_unique<PreFlightTests::CraftWithinMassLimits>(
                    shipTemplate, SpaceCenterFacility::LaunchPad, gv->GetCraftMassLimit(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::LaunchPad), true)));
            }
            break;
        default: break;
    }
    preFlightCheck_->AddTest(std::make_unique<PreFlightTests::ExperimentalPartsAvailable>(*manifest_));
    std::optional<double> funds;
    if (Funding::Instance != nullptr) funds = Funding::Instance->Funds();
    preFlightCheck_->AddTest(std::make_unique<PreFlightTests::CanAffordLaunchTest>(shipTemplate, funds));
    if (PSystemSetup::Instance != nullptr && PSystemSetup::Instance->GetSpaceCenterFacility(launchSiteName) != nullptr)
        preFlightCheck_->AddTest(std::make_unique<PreFlightTests::FacilityOperational>(launchSiteName, launchSiteName));
    preFlightCheck_->AddTest(std::make_unique<PreFlightTests::FacilityOperational>(SCFacilityName, constructionFacilityName));
    preFlightCheck_->AddTest(std::make_unique<PreFlightTests::NoControlSources>(manifest_));
    preFlightCheck_->AddTest(std::make_unique<PreFlightTests::WrongVesselTypeForLaunchSite>(facilityType, fileName_, vehicleName, launchSiteName));
    if (!preFlightCheck_->RunTests()) SpaceCenterUI::SetLock("launchSiteFacility");
}

// LaunchSiteFacility.cs:401
void LaunchSiteFacility::ReturnToDialog() {
    Cancel();
    showShipSelection();
}

// LaunchSiteFacility.cs:407
void LaunchSiteFacility::launchVessel() {
    SpaceCenterUI::RemoveLock("launchSiteFacility");
    FlightDriver::StartWithNewLaunch(fileName_, flagURL_, launchSiteName, crewManifest_);
}

}  // namespace ksp
