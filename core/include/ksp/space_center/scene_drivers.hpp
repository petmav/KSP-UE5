// WP-42: FlightDriver.cs and EditorDriver.cs (the scene entry state machines: startup behaviours, revert data, launch-site validity).
//
// Seams (WP-47 / WP-19): everything that touches live Unity / flight objects is `FlightDriverHost` (the live vessel list and their
// positioning, FloatingOrigin, PQS targeting, the flight camera, StageManager, ShipConstruction.PutShipToGround on the spawn transform,
// the popup dialogs) or `EditorDriverHost` (cameras, the editor scenery, UI masks, EditorLogic.launchSiteName). The persistence /
// revert / startup-mode logic and the launch-site validity filter are ported as written.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/core/coroutine.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_enums.hpp"

namespace ksp {

class ShipConstruct;
class VesselCrewManifest;
class FlightState;

// GamePersistence.SaveGame(name, HighLogic.SaveFolder, SaveMode.OVERWRITE, scene) = SaveGame(HighLogic.CurrentGame.Updated(scene), ...)
void SaveCurrentGameAs(const char* name, GameScenes scene);

// FlightDriver.cs:9
class FlightDriver {
public:
    enum class StartupBehaviours : int { NEW_FROM_FILE, RESUME_SAVED_FILE, RESUME_SAVED_CACHE, NEW_FROM_CRAFT_NODE };   // :11

    // what the Start / setStartupNewVessel paths need from the live game (unset members: the neutral answer)
    struct Host {
        std::function<bool()> partsLoaded;                           // PartLoader.LoadedPartsList != null (unset: true)
        std::function<int()> vesselCount;                            // FlightGlobals.Vessels.Count (unset: 0)
        std::function<bool(int idx)> vesselIsPrelaunch;              // Vessels[idx].situation == PRELAUNCH
        std::function<void()> positionAllVessels;                    // the per-vessel SetPosition / orbit Init / updateFromParameters loop
        std::function<void(int idx)> focusVessel;                    // FloatingOrigin.SetOffset + SetActiveVessel + PQS targeting (:223-234)
        std::function<bool(const std::string& path)> fileExists;     // File.Exists(newShipToLoadPath)
        std::function<void(const std::string& title, const std::string& message)> popup;   // PopupDialog.SpawnPopupDialog
        std::function<void(ShipConstruct&)> beginNewVessel;          // ShipConstruction.PutShipToGround + AssembleForLaunch + PSystemSetup.SetPQSActive + StageManager.BeginFlight
        // the spawn transform of a launch site / facility: set by FlightDriver from PSystemSetup; the host positions the camera / floating origin
        std::function<void(const Vector3d& spawnPosition)> placeAtSpawn;
        std::function<bool()> clearToSave;                           // FlightGlobals.ClearToSave() == CLEAR (unset: true)
        std::function<int()> frameCount;                             // Time.frameCount (unset: 0)
        std::function<void(bool paused)> setTimeScalePause;          // Time.timeScale / InputLock "gamePause" / TimeWarp.SetRate on unpause
        std::function<bool(int idx)> setActiveVessel;                // FlightGlobals.SetActiveVessel(Vessels[idx]) (FocusTargetVessel)
    };
    static inline Host host;

    static inline std::string newShipToLoadPath = "";          // :36
    static inline StartupBehaviours StartupBehaviour = StartupBehaviours::NEW_FROM_FILE;   // :38
    static inline bool resumeCacheUsed = false;                // :40
    static inline std::string newShipFlagURL = "";             // :42
    static inline VesselCrewManifest* newShipManifest = nullptr;   // :44
    static inline std::string StateFileToLoad = "persistent";  // :46
    static inline std::shared_ptr<Game> FlightStateCache;      // :48
    static inline int FocusVesselAfterLoad = 0;                // :50
    static inline std::string LaunchSiteName = "LaunchPad";    // :52
    static inline bool flightStarted = false;                  // :54
    static inline bool CanRevertToPostInit = false;            // :60
    static inline bool CanRevertToPrelaunch = false;           // :62
    static inline std::shared_ptr<GameBackup> PostInitState;   // :64
    static inline std::shared_ptr<GameBackup> PreLaunchState;  // :66
    static inline FlightDriver* fetch = nullptr;               // :68
    static inline bool pause = false;                          // :82

    bool canRun = true;                                        // :30
    bool bypassPersistence = false;                            // :56
    bool bypassLoadingEnforce = false;                         // :70
    int framesBeforeInitialSave = 15;                          // :72
    int targetVessel = 0;                                      // :34
    int framesAtStartup = 0;                                   // :74
    ShipConstruct* newVessel = nullptr;                        // :76 (owned by the host after beginNewVessel)
    std::unique_ptr<ShipConstruct> newVesselOwned;
    int owner = 0;

    static bool BypassPersistence() { return fetch->bypassPersistence; }   // :92
    static bool CanRevert() { return CanRevertToPrelaunch || CanRevertToPostInit; }   // :94
    static bool Pause() { return pause; }                      // :106

    void Awake();                                              // :108
    void Start();                                              // :120
    void setStartupNewVessel();                                // :272
    Enumerator PostInit();                                     // :376
    void FocusTargetVessel();                                  // :395
    static void SetPause(bool pauseState, bool postScreenMessage = true);   // :415
    void OnDestroy();                                          // :440
    static void StartWithNewLaunch(const std::string& fullFilePath, const std::string& missionFlagURL, const std::string& launchSiteName,
                                   VesselCrewManifest* manifest);   // :461
    static void RevertToLaunch();                              // :475
    static void RevertToPrelaunch(EditorFacility facility);    // :489
    static void ReturnToEditor(EditorFacility facility);       // :500
    static void StartAndFocusVessel(const std::string& stateFileToLoad, int vesselToFocusIdx);   // :509
    static void StartAndFocusVessel(std::shared_ptr<Game> stateToLoad, int vesselToFocusIdx);    // :517
    static std::unique_ptr<Game> RemoveSavedVessel(Game& original, int vesselIndex);              // :526
    static void ResetStatics();
};

// EditorDriver.cs:14
class EditorDriver {
public:
    enum class StartupBehaviours : int { START_CLEAN, LOAD_FROM_CACHE, LOAD_FROM_FILE };   // :16

    struct Host {
        std::function<void(const std::string& siteName)> setEditorLogicLaunchSite;   // EditorLogic.fetch.launchSiteName = siteName (unset: no EditorLogic)
        std::function<bool()> editorLogicPresent;                    // EditorLogic.fetch != null
        // Expansions.Missions: MissionSystem.Instance != null && missions.Count > 0 && missions[0].situation.VesselsArePending
        std::function<bool()> missionVesselsPending;
        std::function<bool(const std::string& path)> directoryExists;   // new DirectoryInfo(path).Exists
    };
    static inline Host host;

    static inline EditorDriver* fetch = nullptr;               // :27
    static inline StartupBehaviours StartupBehaviour = StartupBehaviours::LOAD_FROM_CACHE;   // :29
    static inline std::string filePathToLoad = "";             // :31
    static inline EditorFacility editorFacility = EditorFacility::None;   // :33
    static inline std::string DefaultCraftSavePath;            // :68
    bool canRun = true;
    bool restartingEditor = true;                              // :22

    static bool CanRun() { return fetch->canRun; }             // :52
    static const std::string& SelectedLaunchSiteName() { return selectedlaunchSiteName_(); }   // :54
    static std::vector<std::string>& ValidLaunchSites();       // :56
    static void StartAndLoadVessel(const std::string& fullFilePath, EditorFacility facility);   // :265
    static void StartEditor(EditorFacility facility);          // :273
    static bool setLaunchSite(const std::string& siteName);    // :442
    static void saveselectedLaunchSite();                      // :457
    static void setDefaultLaunchSite();                        // :477
    static bool ValidLaunchSite(const std::string& siteName);  // :497
    static void setupValidLaunchSites();                       // :516
    static bool OtherLaunchSitesValid();                       // :558
    static bool SetDefaultSaveFolder(const std::string& path); // :578
    static void ResetStatics();

    // ControlTypes.cs:40-44, :71 (the bits EditorDriver.SetInputLockFromGameParameters ORs together; InputLockManager itself is a WP-47 seam)
    enum : uint64_t {
        EDITOR_SAVE = 0x100000000ull, EDITOR_LOAD = 0x200000000ull, EDITOR_EXIT = 0x400000000ull, EDITOR_NEW = 0x800000000ull,
        EDITOR_LAUNCH = 0x1000000000ull, EDITOR_CREW = 9223372036854775808ull
    };
    static uint64_t SetInputLockFromGameParameters();             // :195 (the ControlTypes mask passed to SetControlLock("EditorDriver_GameParameters"))

private:
    static std::string& selectedlaunchSiteName_();
    static inline std::vector<std::string> validLaunchSites_;
    static inline bool validLaunchSitesNull_ = true;
};

}  // namespace ksp
