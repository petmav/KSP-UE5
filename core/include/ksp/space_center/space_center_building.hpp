// WP-42: SpaceCenterBuilding.cs (the data side: damage / repair / upgrade / demolish of a KSC facility) and the facility entry
// logic of its subclasses (VehicleAssemblyBuilding, SpacePlaneHangarBuilding, TrackingStationBuilding, AstronautComplexFacility,
// AdministrationFacility, MissionControlBuilding, RnDBuilding, FlagPoleFacility, LaunchSiteFacility).
//
// Seams (WP-46 UI / WP-47 scene): hover / highlight / tooltips / camera view (MonoBehaviour GUI), InputLockManager (`SpaceCenterUI::controlLock`),
// PopupDialog (`SpaceCenterUI::popup`), ScreenMessages (`SpaceCenterUI::screenMessage`), GamePersistence.SaveGame (`SpaceCenterUI::saveGame`),
// the Unity hierarchy (`destructibles` = buildingTransform.GetComponentsInChildren<DestructibleBuilding>, the facility's tags).
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/career/currency.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/space_center/destructible_building.hpp"
#include "ksp/editor/preflight_tests.hpp"
#include "ksp/space_center/psystem_setup.hpp"

namespace ksp {

class ProtoVessel;
namespace KSP::UI::Screens {
class MissionRecoveryDialog;
}
using MissionRecoveryDialogHandle = KSP::UI::Screens::MissionRecoveryDialog;
class FlightState;
class VesselCrewManifest;
namespace Upgradeables {
class UpgradeableFacility;
}

// the GUI / input seams the buildings call (all unset = dropped)
struct SpaceCenterUI {
    // InputLockManager.SetControlLock(ControlTypes mask, id) / RemoveControlLock(id): `set` true = lock
    static inline std::function<void(const std::string& lockId, bool set)> controlLock;
    // PopupDialog.SpawnPopupDialog(MultiOptionDialog(id, message, title, ...))
    static inline std::function<void(const std::string& dialogId, const std::string& title, const std::string& message)> popup;
    // ScreenMessages.PostScreenMessage(text, 4f, LOWER_CENTER)
    static inline std::function<void(const std::string& text)> screenMessage;
    // GamePersistence.SaveGame("persistent", HighLogic.SaveFolder, SaveMode.OVERWRITE)
    static inline std::function<void()> saveGame;
    static void SetLock(const std::string& id) { if (controlLock) controlLock(id, true); }
    static void RemoveLock(const std::string& id) { if (controlLock) controlLock(id, false); }
    static void Reset() {
        controlLock = nullptr;
        popup = nullptr;
        screenMessage = nullptr;
        saveGame = nullptr;
    }
};

// SpaceCenterBuilding.cs:13
class SpaceCenterBuilding {
public:
    virtual ~SpaceCenterBuilding();

    std::string facilityName;                      // :21
    std::string buildingInfoName;                  // :31
    std::string buildingDescription;               // :33
    std::vector<DestructibleBuilding*> destructibles;   // :35
    int owner = 0;                                 // StartCoroutine owner id
    Upgradeables::UpgradeableFacility* upgradeableFacility = nullptr;   // :27 (the UpgradeableFacility on the facility transform)
    // buildingTransform.GetComponentsInChildren<DestructibleBuilding>(true) / GetComponent<UpgradeableFacility>() after a rebuild
    std::function<std::vector<DestructibleBuilding*>()> findDestructibles;
    std::function<Upgradeables::UpgradeableFacility*()> findUpgradeableFacility;
    // KSPUtil.FindTagsInChildren(list, transform) of the facility root / of the grounds parent
    std::function<std::vector<std::string>()> facilityRootTags;
    std::function<std::vector<std::string>()> groundTags;
    // facility neighbours of UpgradeableSlave (NeighbourIDs of the slave on the facility transform)
    std::vector<std::string> neighbourIDs;
    bool hasUpgradeableSlave = false;
    PSystemSetup::SpaceCenterFacility* spaceCenterFacility = nullptr;   // :19

    bool Operational() const { return operational_; }          // :79
    float StructuralDamage() const { return structuralDamage_; }   // :81
    Upgradeables::UpgradeableFacility* Facility() { return upgradeableFacility; }   // :77

    // SpaceCenterBuilding.Start (the data part; the renderer / collider / camera setup is WP-47)
    bool Start();                                         // :103
    void OnDestroy();                                          // :122
    void OnGameSceneChange(GameScenes scene);                  // :130
    void OnLeftClick();                                        // :373
    void OnRightClick();                                       // :387
    void EnterBuilding();                                      // :458
    virtual bool IsOpen() { return true; }                     // :470
    std::vector<ProtoVessel*> FindVesselsAtFacility(const FlightState& st, const std::vector<DestructibleBuilding*>& structuresToCheck);   // :475
    std::vector<ProtoVessel*> FindVesselsAtGrounds(const FlightState& st);                                          // :488
    bool WarnOfObstructingVessels(bool includeGrounds, bool onlyDestroyed);              // :501
    void SetupDestructibles();                                 // :542
    void OnStructureCollapsing(DestructibleBuilding* dBld);    // :553
    void OnStructureCollapsed(DestructibleBuilding* dBld);     // :572
    void OnStructureRepairing(DestructibleBuilding* dBld);     // :578
    void OnStructureRepaired(DestructibleBuilding* dBld);      // :592
    float GetStructureDamage() const;                          // :615
    bool IsFacilityOperational(float damage) const;            // :629
    static std::string GetStructureDamageLevel(float lvl);     // :634
    float GetRepairsCost() const;                              // :655
    float GetCollapseReputationHit() const;                    // :669
    void RepairFacility(bool deduceFunds);                     // :683
    void DemolishFacility();                                   // :697
    void RepairStructures();                                   // :714
    void ResetStructures();                                    // :736
    void UnregisterDestructibles();                            // :745
    void OnFacilityUpgrading(Upgradeables::UpgradeableFacility* facility, int level);   // :769
    void OnFacilityUpgraded(Upgradeables::UpgradeableFacility* facility, int level);    // :777
    void UnregisterUpgradeables();                             // :822
    void UpgradeFacility(bool deduceFunds);                    // :828
    void DowngradeFacility(bool deduceFunds);                  // :849

    // the dismiss actions of KSCFacilityContextMenu (OnContextMenuDismissed :412)
    enum class DismissAction { Enter, Repair, Upgrade, Demolish, Downgrade, Cancel };
    void OnContextMenuDismissed(DismissAction dma, bool demolishConfirmed = true);   // :412

protected:
    virtual void OnStart() {}                                  // :393
    virtual void OnClicked() {}                                // :401
    virtual void OnOnDestroy() {}                              // :397
    bool operational_ = false;                                 // :49
    float structuralDamage_ = 0.0f;                            // :51
    bool eventsAdded_ = false;
    bool structureEventsAdded_ = false;
    bool upgradeEventsAdded_ = false;
};

// VehicleAssemblyBuilding.cs:5
class VehicleAssemblyBuilding : public SpaceCenterBuilding {
public:
    bool IsOpen() override;            // :7
    // EditorDriver / MissionSystem state read by OnClicked: ShipConstruction.ShipConfig != null / ShipType; a mission with pending vessels
    static inline std::function<bool()> missionVesselsArePending;
    void OnClicked() override;         // :12
};

// SpacePlaneHangarBuilding.cs:6
class SpacePlaneHangarBuilding : public SpaceCenterBuilding {
public:
    bool IsOpen() override;            // :8
    void OnClicked() override;         // :13
};

// TrackingStationBuilding.cs:5
class TrackingStationBuilding : public SpaceCenterBuilding {
public:
    bool IsOpen() override;            // :7
    void OnClicked() override;         // :12
};

// AstronautComplexFacility.cs:6
class AstronautComplexFacility : public SpaceCenterBuilding {
public:
    bool IsOpen() override;            // :9
    void OnClicked() override;         // :14
    void onAstronautComplexDialogClose();   // :48
    void OnOnDestroy() override;       // :55
private:
    bool dialogIsUp_ = false;          // :8
};

// AdministrationFacility.cs:5 (StrategySystem.Instance != null is `strategySystemPresent`)
class AdministrationFacility : public SpaceCenterBuilding {
public:
    static inline std::function<bool()> strategySystemPresent;
    bool IsOpen() override;            // :8
    void OnClicked() override;         // :13
    void onDialogClose();              // :43
    void OnOnDestroy() override;       // :50
private:
    bool dialogIsUp_ = false;
};

// MissionControlBuilding.cs:5 (ContractSystem.Instance != null is `contractSystemPresent`)
class MissionControlBuilding : public SpaceCenterBuilding {
public:
    static inline std::function<bool()> contractSystemPresent;
    bool IsOpen() override;            // :8
    void OnClicked() override;         // :13
    void onDialogClose();              // :40
    void OnOnDestroy() override;       // :47
private:
    bool dialogIsUp_ = false;
};

// RnDBuilding.cs:5 (ResearchAndDevelopment.Instance != null is `rndPresent`)
class RnDBuilding : public SpaceCenterBuilding {
public:
    static inline std::function<bool()> rndPresent;
    bool IsOpen() override;            // :8
    void OnClicked() override;         // :13
    void onDialogClose();              // :40
    void OnOnDestroy() override;       // :47
private:
    bool dialogIsUp_ = false;
};

// FlagPoleFacility.cs:5
class FlagPoleFacility : public SpaceCenterBuilding {
public:
    void OnClicked() override;         // :12
    void OnFlagSelect(const std::string& textureName);   // :35 (FlagBrowser.FlagEntry.textureInfo.name)
    void OnFlagCancel();               // :47
    static inline std::function<void()> spawnFlagBrowser;   // Object.Instantiate(FlagBrowserPrefab)
};

// LaunchSiteFacility.cs:12 (the launch pad / runway building: ship selection, pre-flight checks and the launch call)
class LaunchSiteFacility : public SpaceCenterBuilding {
public:
    std::string launchSiteName = "LaunchPad";                        // :14
    std::string SCFacilityName = "VAB";                              // :16
    std::string craftSubfolder = "VAB";                              // :18
    EditorFacility facilityType = EditorFacility::VAB;               // :20
    std::string vehicleName = "Rocket";                              // :22
    std::string constructionFacilityName = "Vehicle Assembly Building";   // :24

    // seams: Directory / FileInfo enumeration of the *.craft files (Ships/<subfolder> of the game, the installed expansions and
    // saves/<folder>/Ships/<subfolder>; AllowStockVessels / expansions logic of showShipSelection stays here): the number found
    static inline std::function<int(const std::string& craftSubfolder, bool includeStock, bool includeExpansions)> craftFileCount;
    // MissionSystem (Expansions): the launch-site-clear check for a mission game (CustomParams<MissionParamsGeneral>.preventVesselRecovery)
    // is read from the game parameters; a recovery dialog (MissionRecoveryDialog) is the UI's
    std::vector<MissionRecoveryDialogHandle*> missionDialogs;        // :40
    std::function<void(const std::string& path, std::string& error)> loadCraft;   // ConfigNode.Load(path) failure is the host's (unset: ConfigNode::Load)

    void OnStart() override;                                         // :48
    void OnOnDestroy() override;                                     // :60
    bool IsOpen() override;                                          // :68
    void OnClicked() override;                                       // :73
    void showFacilityLocked();                                       // :136
    void ResumeFlightOnSite();                                       // :147
    void ClearSite();                                                // :156
    void onMissionDialogUp(MissionRecoveryDialogHandle* dialog);     // :165
    void onMissionDialogDismiss(MissionRecoveryDialogHandle* dialog);   // :174
    void Cancel();                                                   // :191
    void showShipSelection();                                        // :198
    void NothingToLaunch();                                          // :289
    void shipSelected(const std::string& path, const std::string& flag, VesselCrewManifest* manifest);   // :294
    void launchChecks();                                             // :352
    void ReturnToDialog();                                           // :401
    void launchVessel();                                             // :407
    std::string LaunchSiteDisplayName() const;                       // the launchSiteDisplayName switch shared by :92 and :304
    PreFlightCheck* CurrentPreFlightCheck() { return preFlightCheck_.get(); }
    PreFlightTests::LaunchSiteClear* LaunchSiteClearTest() { return launchSiteClearTest_.get(); }

private:
    std::unique_ptr<PreFlightTests::LaunchSiteClear> launchSiteClearTest_;   // :30
    bool awaitingLaunchClear_ = false;                               // :32
    VesselCrewManifest* crewManifest_ = nullptr;                     // :34
    std::string fileName_;                                           // :36
    std::string flagURL_;                                            // :38
    std::string path_;                                               // :42
    std::string flag_;                                               // :44
    VesselCrewManifest* manifest_ = nullptr;                         // :46
    std::unique_ptr<PreFlightCheck> preFlightCheck_;
    bool dialogEventsAdded_ = false;
};

}  // namespace ksp
