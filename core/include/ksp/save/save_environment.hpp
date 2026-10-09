// SaveEnvironment: the process-wide state that KSP's save model reads from other subsystems while it loads and saves
// (GameDatabase, ExpansionsLoader, HighLogic, FlightGlobals' persistentId registry, GameSettings, Versioning, ROCManager,
// PartLoader, Guid/TickCount randomness). Those subsystems belong to other work packages (WP-05 GameDatabase /
// ExpansionsLoader / PartLoader, WP-19 FlightGlobals, WP-42 HighLogic, WP-50 GameSettings); WP-04 keeps exactly the
// pieces it needs here, under one object, so an integration can wire the real systems in one place.
//
// Defaults = a stock KSP 1.12.5 (build 3190) Windows x64 install without DLC, english, not modded.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/save/save_enums.hpp"
#include "ksp/util/collections.hpp"

namespace ksp {

class ConfigNode;
class Game;
class ProtoPartSnapshot;
class Vessel;
class Part;
class CelestialBody;
class FlightState;
class ProtoScenarioModule;
namespace Experience {
class ExperienceSystemConfig;
}

// ROCManager.cs:359-399: the removed-ROC list that Game.Save / Game(ConfigNode) persist ("RemovedROCs" node).
class ROCManagerRemovedROCs {
public:
    std::vector<int> removedROCs;
    bool hasList = false;  // C# removedROCs != null

    // ROCManager.cs:359 SaveRemoveROCs
    void SaveRemoveROCs(ConfigNode& node) const;
    // ROCManager.cs:370 LoadRemovedROCs (int.TryParse of every ROCId; AddUnique)
    void LoadRemovedROCs(const ConfigNode& node);
    // ROCManager.cs:384 ROCRemoved
    bool ROCRemoved(int rocId);
};

// FlightGlobals.cs:178-182 / 473-666: the persistentId dictionaries and their check functions as used by the save
// model (ProtoVessel / ProtoPartSnapshot / ShipConstruct). `fetch` is FlightGlobals' instance being alive.
class PersistentIdRegistry {
public:
    bool fetch = true;
    DictionaryValueList<uint32_t, const void*> persistentVesselIds;          // uint -> Vessel (null for protos)
    DictionaryValueList<uint32_t, const void*> persistentLoadedPartIds;      // uint -> Part
    DictionaryValueList<uint32_t, ProtoPartSnapshot*> persistentUnloadedPartIds;

    // FlightGlobals.cs:473 GetUniquepersistentId ((uint)Guid.NewGuid().GetHashCode(), never 0 or in use)
    uint32_t GetUniquepersistentId() const;
    // FlightGlobals.cs:487 CheckVesselpersistentId (vessel = the live Vessel, nullptr for ProtoVessel/ShipConstruct)
    uint32_t CheckVesselpersistentId(uint32_t persistentId, const void* vessel, bool removeOldId, bool addNewId);
    // FlightGlobals.cs:604 CheckProtoPartSnapShotpersistentId (vesselPersistentId: the snapshot's pVesselRef id)
    uint32_t CheckProtoPartSnapShotpersistentId(uint32_t persistentId, ProtoPartSnapshot* partSnapshot, bool removeOldId,
                                                bool addNewId, uint32_t vesselPersistentId);
    // FlightGlobals.cs:536 CheckPartpersistentId for parts without a vessel (craft loading: part.vessel == null)
    uint32_t CheckPartpersistentId(uint32_t persistentId, const void* part, bool removeOldId, bool addNewId,
                                   uint32_t vesselId = 0);
    // FlightGlobals.cs:655 ClearpersistentIdDictionaries
    bool ClearpersistentIdDictionaries();
};

struct SaveEnvironment {
    // ---- GameDatabase (GameDatabase.cs:124-156, 1044-1085, 1136-1200) -------------------------------------------------
    bool gameDatabaseModded = false;          // GameDatabase.Modded
    std::string gameDatabaseEnvironmentInfo;  // GameDatabase.EnvironmentInfo (the "envInfo" save value)
    std::vector<std::string> gameDatabaseLoaderInfo;  // GameDatabase.loaderInfo (HashSet<string>, insertion order)

    // ---- ExpansionsLoader.expansionsInfo (ExpansionsLoader.cs:328-400) ------------------------------------------------
    struct ExpansionInfo {
        std::string name;
        bool installed = false;
        std::vector<ProtoCrewMember_KerbalSuit> kerbalSuits;
    };
    std::vector<ExpansionInfo> expansions;
    // the two stock DLCs with their suits: MakingHistory (Vintage), Serenity (Future)
    void SetStockExpansionInstalled(std::string_view name, bool installed);
    bool IsExpansionInstalled(std::string_view name) const;                          // ExpansionsLoader.cs:328
    bool IsExpansionAnyKerbalSuitInstalled() const;                                  // ExpansionsLoader.cs:350
    bool IsExpansionKerbalSuitInstalled(ProtoCrewMember_KerbalSuit suit) const;      // ExpansionsLoader.cs:371

    // ---- HighLogic.CurrentGame / LoadedScene / LoadedSceneHasPlanetarium / LoadedSceneIsGame --------------------------
    Game* currentGame = nullptr;
    GameScenes loadedScene = GameScenes::MAINMENU;
    bool loadedSceneHasPlanetarium = false;
    bool loadedSceneIsGame = false;

    // Game.Updated(): new FlightState() captures the live vessels (WP-14) and ScenarioRunner.GetUpdatedProtoModules()
    // the running scenario modules (WP-06); unset: an empty FlightState / the game's current scenario list
    std::function<std::shared_ptr<FlightState>()> captureFlightState;
    std::function<std::vector<std::shared_ptr<ProtoScenarioModule>>()> getUpdatedProtoModules;

    // ---- CommNet.CommNetUI statics persisted by FlightState (CommNetUI.cs:46-48) ---------------------------------------
    CommNetUI_DisplayMode commNetUIModeTrackingStation = CommNetUI_DisplayMode::Network;
    CommNetUI_DisplayMode commNetUIModeFlightMap = CommNetUI_DisplayMode::Path;
    // ---- MapViewFiltering.vesselTypeFilter (MapViewFiltering.cs:83, GetFilterState / LoadFilterState) -----------------
    int mapViewFilteringVesselTypeFilter = 0;

    // ---- ROCManager.Instance (only its removed-ROC list; nullptr = no ROCManager in the scene) ------------------------
    ROCManagerRemovedROCs* rocManager = nullptr;

    // ---- GameSettings values the save model reads (GameSettings.cs SetDefaultValues) -----------------------------------
    double DEFAULT_KERBAL_RESPAWN_TIMER = 7200.0;  // GameSettings.cs:1217
    int SAVE_BACKUPS = 5;                          // GameSettings.cs:1052
    int MAX_VESSELS_BUDGET = 250;                  // GameSettings.cs:996
    bool DECLUTTER_KSC = true;                     // GameSettings.cs:997

    // ---- Versioning (Versioning.cs:138; version numbers are ksp::Versioning in ksp/util/ksp_util.hpp) ------------------
    std::string applicationPlatform = "WindowsPlayer";  // Application.platform.ToString()
    std::string architecture = " x64";                  // Versioning.architecture
    int experimental = 0;
    bool beta = false;
    bool prerelease = false;

    // ---- GameDatabase.Instance.ExperienceConfigs ----------------------------------------------------------------------
    std::shared_ptr<Experience::ExperienceSystemConfig> experienceConfigs;

    // ---- PartLoader queries (PartLoader.getPartInfoByName(...).partPrefab ...) -----------------------------------------
    // partInfo != null for a part name (ProtoPartSnapshot.partInfo); default: unknown
    std::function<bool(std::string_view partName)> partExists;
    // partPrefab.CrewCapacity (ProtoVessel.RebuildCrewCounts); called only when partExists says true
    std::function<int(std::string_view partName)> partCrewCapacity;
    // a float field of the prefab's first module of a class (v180 upgrade scripts: ModuleControlSurface /
    // ModuleAeroSurface ctrlSurfaceRange); nullopt = part or module not found (KSP: NullReferenceException)
    std::function<std::optional<float>(std::string_view partName, std::string_view moduleClass, std::string_view field)>
        prefabModuleFloat;
    // PartLoader.Instance.AddPartReplacement(old, new) calls made by the PartReplace upgrade scripts (in order)
    std::vector<std::pair<std::string, std::string>> partReplacements;
    // ModuleInventoryPart.SaveDefault of the EVA part's prefab (ProtoCrewMember.SetDefaultInventory); unset: the
    // INVENTORY node stays empty
    std::function<void(std::string_view evaPartName, ConfigNode& inventory)> saveDefaultKerbalInventory;

    // ---- PSystemManager.Instance.localBodies (KerbalRoster experience: name, displayName, isHomeWorld, RecoveryValue) --
    std::vector<CelestialBody*> localBodies;

    // ---- FlightGlobals persistentId registry ------------------------------------------------------------------------
    PersistentIdRegistry persistentIds;

    // ---- randomness KSP draws while loading (Guid.NewGuid().GetHashCode(), Environment.TickCount) ---------------------
    std::function<int32_t()> newGuidHashCode;
    std::function<int32_t()> tickCount;
    int32_t NewGuidHashCode() const;
    int32_t TickCount() const;

    // the current environment (process-wide, like the C# statics)
    static SaveEnvironment& Current();
    // restore the defaults (keeps nothing; tests)
    static void Reset();
};

// Planetarium.GetUniversalTime(): Planetarium.fetch.time, else HighLogic.CurrentGame.UniversalTime (NullReferenceException
// without either, as in KSP)
double SaveUniversalTime();

// Versioning.cs:138 GetVersionStringFull ("1.12.5.3190 (WindowsPlayer x64)")
std::string Versioning_GetVersionStringFull();
// Versioning.cs:123 GetVersion ("1.12.5")
std::string Versioning_GetVersion();

}  // namespace ksp
