// WP-42: FlightDriver.cs, EditorDriver.cs.
#include "ksp/space_center/scene_drivers.hpp"

#include "ksp/career/progress.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/physics/floating_origin.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/game_persistence.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/psystem_setup.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

// GamePersistence.cs:20 SaveGame(name, folder, mode, scene): SaveGame(HighLogic.CurrentGame.Updated(scene), name, folder, mode)
void SaveCurrentGameAs(const char* name, GameScenes scene) {
    Game* game = HighLogic::CurrentGame();
    if (game == nullptr) throw NullReferenceException();
    GamePersistence::SaveGame(game->Updated(scene), name, HighLogic::SaveFolder(), SaveMode::OVERWRITE);
}

void FlightDriver::ResetStatics() {
    host = Host{};
    newShipToLoadPath = "";
    StartupBehaviour = StartupBehaviours::NEW_FROM_FILE;
    resumeCacheUsed = false;
    newShipFlagURL = "";
    newShipManifest = nullptr;
    StateFileToLoad = "persistent";
    FlightStateCache.reset();
    FocusVesselAfterLoad = 0;
    LaunchSiteName = "LaunchPad";
    flightStarted = false;
    CanRevertToPostInit = false;
    CanRevertToPrelaunch = false;
    PostInitState.reset();
    PreLaunchState.reset();
    fetch = nullptr;
    pause = false;
}

// FlightDriver.cs:108
void FlightDriver::Awake() {
    fetch = this;
    flightStarted = false;
    framesAtStartup = host.frameCount ? host.frameCount() : 0;
    if (!bypassLoadingEnforce && host.partsLoaded && !host.partsLoaded()) {
        log::Error("[Flight Driver]: Game does not seem to be loaded.");
        canRun = false;
    }
}

// FlightDriver.cs:120
void FlightDriver::Start() {
    flightStarted = false;
    if (!canRun) return;
    pause = false;
    if (host.setTimeScalePause) host.setTimeScalePause(false);
    if (StartupBehaviour != StartupBehaviours::RESUME_SAVED_CACHE && !bypassPersistence) {
        FlightStateCache = GamePersistence::LoadGame(StateFileToLoad, HighLogic::SaveFolder(), true, true);
        if (FlightStateCache == nullptr) {
            bypassPersistence = true;
            if (StartupBehaviour == StartupBehaviours::RESUME_SAVED_FILE) StartupBehaviour = StartupBehaviours::NEW_FROM_FILE;
        } else if (!FlightStateCache->compatible) {
            if (host.popup) host.popup("Load Failed", Localizer::Format("#autoLOC_6002422"));
            HighLogic::LoadScene(GameScenes::SPACECENTER);
            return;
        }
    }
    if (FlightStateCache == nullptr) throw NullReferenceException();   // FlightStateCache.flightState with no cache
    if (FlightStateCache->flightState == nullptr) FlightStateCache->flightState = std::make_shared<FlightState>();
    if (!bypassPersistence) {
        if (StartupBehaviour == StartupBehaviours::NEW_FROM_FILE) {
            PreLaunchState = std::make_shared<GameBackup>(*FlightStateCache);
            std::vector<ProtoVessel*> list = ShipConstruction::FindVesselsLandedAt(FlightStateCache->flightState.get(), LaunchSiteName);
            const int count = static_cast<int>(list.size());
            for (int i = 0; i < count; i++) ShipConstruction::RecoverVesselFromFlight(*list[static_cast<size_t>(i)], *FlightStateCache->flightState);
        }
        // FlightDriver.cs:169-174 (WP-19c): the clock, then Game.Load (HighLogic.CurrentGame = the cache, the scenario modules, the roster,
        // FlightState.Load: every proto vessel becomes an unloaded Vessel in FlightGlobals)
        if (Planetarium::fetch != nullptr) {
            Planetarium::SetUniversalTime(FlightStateCache->UniversalTime());
            Planetarium::fetch->UpdateCBs();
        }
        FlightStateCache->Load();
    }
    bool started = false;
    switch (StartupBehaviour) {
        case StartupBehaviours::NEW_FROM_FILE:
            if (!(host.fileExists ? host.fileExists(newShipToLoadPath) : false)) break;
            log::Info("Loading ship from file: " + newShipToLoadPath);
            setStartupNewVessel();
            started = true;
            break;
        case StartupBehaviours::RESUME_SAVED_FILE:
        case StartupBehaviours::RESUME_SAVED_CACHE: {
            // unset seams: FlightGlobals' own registry (WP-19c), which FlightStateCache.Load filled
            const int vesselCount = host.vesselCount ? host.vesselCount() : static_cast<int>(FlightGlobals::Vessels.size());
            // Mathf.Clamp(FocusVesselAfterLoad, 0, Vessels.Count - 1) (Mathf.Clamp(int): min > max returns min... the C# clamp is min-then-max)
            int focus = FocusVesselAfterLoad;
            if (focus < 0) focus = 0;
            if (focus > vesselCount - 1) focus = vesselCount - 1;
            FocusVesselAfterLoad = focus;
            if (vesselCount == 0) {
                log::Warning("Flight State contains no vessels. Returning to KSC");
                HighLogic::LoadScene(GameScenes::SPACECENTER);
                return;
            }
            log::Info("Target vessel index: " + std::to_string(FocusVesselAfterLoad) + "  vessel count: " + std::to_string(vesselCount));
            CanRevertToPostInit = host.vesselIsPrelaunch ? host.vesselIsPrelaunch(FocusVesselAfterLoad)
                                                         : FlightGlobals::Vessels[static_cast<size_t>(FocusVesselAfterLoad)]->situation ==
                                                               Vessel::Situations::PRELAUNCH;
            if (CanRevertToPostInit && PostInitState != nullptr &&
                FlightStateCache->flightState->protoVessels[static_cast<size_t>(FocusVesselAfterLoad)]->vesselID == PostInitState->ActiveVesselID &&
                ShipConstruction::ShipConfig != nullptr && PreLaunchState != nullptr) {
                CanRevertToPrelaunch = true;
            } else {
                CanRevertToPrelaunch = false;
                if (ShipConstruction::ShipConfig != nullptr) ShipConstruction::ShipConfig = nullptr;
            }
            if (host.positionAllVessels) {
                host.positionAllVessels();
            } else {
                // FlightDriver.cs:209-219
                for (size_t j = 0; j < FlightGlobals::Vessels.size(); j++) {
                    Vessel* vessel2 = FlightGlobals::Vessels[j];
                    if (vessel2->LandedOrSplashed()) {
                        vessel2->SetPosition(vessel2->mainBody()->GetWorldSurfacePosition(vessel2->latitude, vessel2->longitude, vessel2->altitude));
                        continue;
                    }
                    vessel2->orbitDriver->orbit->Init();
                    vessel2->orbitDriver->updateFromParameters(true);
                }
            }
            if (host.focusVessel) {
                host.focusVessel(FocusVesselAfterLoad);
            } else {
                // FlightDriver.cs:220-221 (the flight camera / PQS targeting of :222-230 are the host's)
                Vessel* vessel = FlightGlobals::Vessels[static_cast<size_t>(FocusVesselAfterLoad)];
                FloatingOrigin::SetOffset(Vector3d(vessel->vesselTransform.position));
                FlightGlobals::SetActiveVessel(vessel);
            }
            if (StartupBehaviour == StartupBehaviours::RESUME_SAVED_CACHE) resumeCacheUsed = true;
            started = true;
            break;
        }
        case StartupBehaviours::NEW_FROM_CRAFT_NODE:
            if (ShipConstruction::ShipConfig == nullptr) break;
            log::Info("Loading ship from config");
            setStartupNewVessel();
            started = true;
            break;
    }
    if (started) {
        log::Info("all systems started");
        flightStarted = true;
        HighLogic::Scheduler().StartCoroutine(PostInit(), owner);
        return;
    }
    log::Info("No ship in ShipAssembly or no such filename exists! cannot start flight mode!");
    log::Info("try building a ship first, that could help somewhat");
    canRun = false;
    HighLogic::LoadScene(GameScenes::SPACECENTER);
}

// FlightDriver.cs:272 (the spawn transform positions and the vessel assembly are `Host`: placeAtSpawn / beginNewVessel)
void FlightDriver::setStartupNewVessel() {
    PSystemSetup::SpaceCenterFacility* spaceCenterFacility = nullptr;
    LaunchSite* launchSite = nullptr;
    LaunchSite::SpawnPoint* spawnPoint = nullptr;
    PSystemSetup::SpaceCenterFacility::SpawnPoint* spawnPoint2 = nullptr;
    std::optional<Vector3d> transform;
    newVesselOwned = ShipConstruction::LoadShip(newShipToLoadPath);
    newVessel = newVesselOwned.get();
    if (newVessel == nullptr) {
        HighLogic::LoadScene(GameScenes::SPACECENTER);
        return;
    }
    ShipConstruction::ShipManifest = newShipManifest;
    try {
        if (PSystemSetup::Instance == nullptr) throw NullReferenceException();
        spaceCenterFacility = PSystemSetup::Instance->GetSpaceCenterFacility(LaunchSiteName);
        if (spaceCenterFacility != nullptr) {
            spawnPoint2 = spaceCenterFacility->GetSpawnPoint(LaunchSiteName);
            if (spawnPoint2 != nullptr) {
                const auto t = spawnPoint2->GetSpawnPointTransform();
                if (!t) throw NullReferenceException();
                if (host.placeAtSpawn) host.placeAtSpawn(*t);
                PSystemSetup::Instance->SetPQSActive(spaceCenterFacility->facilityPQS);
                newVessel->shipFacility = spaceCenterFacility->editorFacility;
                ShipConstruction::CreateBackup(*newVessel);
                transform = t;
            }
        } else {
            launchSite = PSystemSetup::Instance->GetLaunchSite(LaunchSiteName);
            if (launchSite != nullptr) {
                spawnPoint = launchSite->GetSpawnPoint(LaunchSiteName);
                if (spawnPoint != nullptr) {
                    const auto t = spawnPoint->GetSpawnPointTransform();
                    if (!t) throw NullReferenceException();
                    if (host.placeAtSpawn) host.placeAtSpawn(*t);
                    PSystemSetup::Instance->SetPQSActive(launchSite->launchsitePQS);
                    newVessel->shipFacility = launchSite->editorFacility;
                    ShipConstruction::CreateBackup(*newVessel);
                    transform = t;
                }
            }
        }
    } catch (const std::exception& e) {
        log::Error(std::string("[FlightDriver] ") + e.what());
    }
    if (!transform) {
        if (host.popup) host.popup("Launch Failed", "Launch of Vessel " + newVessel->shipName + " failed.");
        HighLogic::LoadScene(GameScenes::SPACECENTER);
        return;
    }
    if (host.beginNewVessel) host.beginNewVessel(*newVessel);
    CanRevertToPostInit = true;
    CanRevertToPrelaunch = true;
}

// FlightDriver.cs:376
Enumerator FlightDriver::PostInit() {
    auto frame = [] { return host.frameCount ? host.frameCount() : 0; };
    while (frame() - framesAtStartup < framesBeforeInitialSave) co_yield YieldInstruction::Null();
    if (StartupBehaviour == StartupBehaviours::NEW_FROM_FILE && newVessel != nullptr) GE::OnVesselRollout.Fire(newVessel);
    PostInitState = std::make_shared<GameBackup>(HighLogic::CurrentGame()->Updated());
    if (!host.clearToSave || host.clearToSave()) GamePersistence::SaveGame(*PostInitState, "persistent", HighLogic::SaveFolder(), SaveMode::OVERWRITE);
    GE::onFlightReady.Fire();
}

// FlightDriver.cs:395
void FlightDriver::FocusTargetVessel() {
    if (host.setActiveVessel) host.setActiveVessel(targetVessel);
}

// FlightDriver.cs:415 (Time.timeScale, InputLockManager and TimeWarp.SetRate are `host.setTimeScalePause`)
void FlightDriver::SetPause(bool pauseState, bool postScreenMessage) {
    (void)postScreenMessage;
    GE::onShowUI.Fire();
    if (pauseState) {
        pause = true;
        log::Info("Game Paused!");
        GE::onGamePause.Fire();
        if (host.setTimeScalePause) host.setTimeScalePause(true);
    } else {
        pause = false;
        log::Info("Game Unpaused!");
        GE::onGameUnpause.Fire();
        if (host.setTimeScalePause) host.setTimeScalePause(false);
    }
}

// FlightDriver.cs:440
void FlightDriver::OnDestroy() {
    pause = false;
    if (StartupBehaviour == StartupBehaviours::RESUME_SAVED_CACHE && resumeCacheUsed) {
        FlightStateCache.reset();
        resumeCacheUsed = false;
    }
    if (fetch != nullptr && fetch == this) fetch = nullptr;
}

// FlightDriver.cs:461
void FlightDriver::StartWithNewLaunch(const std::string& fullFilePath, const std::string& missionFlagURL, const std::string& launchSiteName, VesselCrewManifest* manifest) {
    log::Info("Launching vessel from " + launchSiteName + ". Craft file: " + fullFilePath);
    SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
    SaveCurrentGameAs("persistent", GameScenes::FLIGHT);
    StartupBehaviour = StartupBehaviours::NEW_FROM_FILE;
    newShipToLoadPath = fullFilePath;
    StateFileToLoad = "persistent";
    newShipFlagURL = missionFlagURL;
    newShipManifest = manifest;
    LaunchSiteName = launchSiteName;
    HighLogic::LoadScene(GameScenes::FLIGHT);
}

// FlightDriver.cs:475
void FlightDriver::RevertToLaunch() {
    SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
    StartupBehaviour = StartupBehaviours::RESUME_SAVED_CACHE;
    std::shared_ptr<GameParameters> parameters = FlightStateCache->Parameters;
    FlightStateCache = std::make_shared<Game>(PostInitState->Config.get());
    FlightStateCache->Parameters = parameters;
    resumeCacheUsed = false;
    FocusVesselAfterLoad = PostInitState->ActiveVessel;
    GE::onGameStatePostLoad.Fire(FlightStateCache->config.get());
    GE::OnRevertToLaunchFlightState.Fire(HighLogic::CurrentGame()->flightState.get());
    HighLogic::LoadScene(GameScenes::FLIGHT);
}

// FlightDriver.cs:489
void FlightDriver::RevertToPrelaunch(EditorFacility facility) {
    log::Info("[FlightDriver]: Flight State Reverted to Prelaunch.");
    SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
    EditorDriver::StartupBehaviour = EditorDriver::StartupBehaviours::LOAD_FROM_CACHE;
    GamePersistence::SaveGame(*PreLaunchState, "persistent", HighLogic::SaveFolder(), SaveMode::OVERWRITE);
    GE::onGameStatePostLoad.Fire(FlightStateCache->config.get());
    GE::OnRevertToPrelaunchFlightState.Fire(HighLogic::CurrentGame()->flightState.get());
    EditorDriver::StartEditor(facility);
}

// FlightDriver.cs:500
void FlightDriver::ReturnToEditor(EditorFacility facility) {
    log::Info("[FlightDriver]: Returning to Editor.");
    SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
    EditorDriver::StartupBehaviour = EditorDriver::StartupBehaviours::LOAD_FROM_CACHE;
    SaveCurrentGameAs("persistent", GameScenes::EDITOR);
    EditorDriver::StartEditor(facility);
}

// FlightDriver.cs:509
void FlightDriver::StartAndFocusVessel(const std::string& stateFileToLoad, int vesselToFocusIdx) {
    StartupBehaviour = StartupBehaviours::RESUME_SAVED_FILE;
    StateFileToLoad = stateFileToLoad;
    FocusVesselAfterLoad = vesselToFocusIdx;
    HighLogic::LoadScene(GameScenes::FLIGHT);
}

// FlightDriver.cs:517
void FlightDriver::StartAndFocusVessel(std::shared_ptr<Game> stateToLoad, int vesselToFocusIdx) {
    FlightStateCache = std::move(stateToLoad);
    resumeCacheUsed = false;
    FocusVesselAfterLoad = vesselToFocusIdx;
    StartupBehaviour = StartupBehaviours::RESUME_SAVED_CACHE;
    HighLogic::LoadScene(GameScenes::FLIGHT);
}

// FlightDriver.cs:526
std::unique_ptr<Game> FlightDriver::RemoveSavedVessel(Game& original, int vesselIndex) {
    if (vesselIndex < 0 || static_cast<size_t>(vesselIndex) >= original.flightState->protoVessels.size() || original.flightState->protoVessels[static_cast<size_t>(vesselIndex)] == nullptr)
        throw std::invalid_argument("Flight Driver Error: Cannot Generate Prelaunch State from given FlightState, as it does not contain a vessel at index " + std::to_string(vesselIndex));
    const auto vesselCrew = original.flightState->protoVessels[static_cast<size_t>(vesselIndex)]->GetVesselCrew();
    for (size_t i = 0; i < vesselCrew.size(); i++) vesselCrew[i]->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
    std::unique_ptr<Game> cloneOf = Game::GetCloneOf(original);
    cloneOf->flightState->protoVessels.erase(cloneOf->flightState->protoVessels.begin() + vesselIndex);
    return cloneOf;
}

// ---- EditorDriver -------------------------------------------------------------------------------------------------------------

void EditorDriver::ResetStatics() {
    host = Host{};
    fetch = nullptr;
    StartupBehaviour = StartupBehaviours::LOAD_FROM_CACHE;
    filePathToLoad = "";
    editorFacility = EditorFacility::None;
    DefaultCraftSavePath = "";
    validLaunchSites_.clear();
    validLaunchSitesNull_ = true;
    selectedlaunchSiteName_() = "";
}

std::string& EditorDriver::selectedlaunchSiteName_() {
    static std::string s;
    return s;
}

// EditorDriver.cs:56
std::vector<std::string>& EditorDriver::ValidLaunchSites() {
    validLaunchSitesNull_ = false;
    return validLaunchSites_;
}

// EditorDriver.cs:195
uint64_t EditorDriver::SetInputLockFromGameParameters() {
    Game* game = HighLogic::CurrentGame();
    if (game == nullptr) return 0;
    uint64_t controlTypes = 0;
    if (game->Mode == Game_Modes::MISSION_BUILDER) {
        controlTypes |= EDITOR_LAUNCH;
        controlTypes |= EDITOR_CREW;
        return controlTypes;
    }
    if (game->Mode == Game_Modes::MISSION) return controlTypes;
    const auto& ed = game->Parameters->Editor;
    if (!ed.CanLaunch) controlTypes |= EDITOR_LAUNCH;
    if (!ed.CanLoad) controlTypes |= EDITOR_LOAD;
    if (!ed.CanStartNew) controlTypes |= EDITOR_NEW;
    if (!ed.CanSave) controlTypes |= EDITOR_SAVE;
    if (!ed.CanLeaveToMainMenu && !ed.CanLeaveToSpaceCenter) controlTypes |= EDITOR_EXIT;
    return controlTypes;
}

// EditorDriver.cs:265
void EditorDriver::StartAndLoadVessel(const std::string& fullFilePath, EditorFacility facility) {
    StartupBehaviour = StartupBehaviours::LOAD_FROM_FILE;
    filePathToLoad = fullFilePath;
    editorFacility = facility;
    HighLogic::LoadScene(GameScenes::EDITOR);
}

// EditorDriver.cs:273
void EditorDriver::StartEditor(EditorFacility facility) {
    editorFacility = facility;
    HighLogic::LoadScene(GameScenes::EDITOR);
}

// EditorDriver.cs:442
bool EditorDriver::setLaunchSite(const std::string& siteName) {
    if (ValidLaunchSite(siteName)) {
        if (host.editorLogicPresent && host.editorLogicPresent() && host.setEditorLogicLaunchSite) host.setEditorLogicLaunchSite(siteName);
        selectedlaunchSiteName_() = siteName;
        return true;
    }
    setDefaultLaunchSite();
    return false;
}

// EditorDriver.cs:457
void EditorDriver::saveselectedLaunchSite() {
    Game* game = HighLogic::CurrentGame();
    if (game != nullptr) {
        if (editorFacility == EditorFacility::SPH)
            game->defaultSPHLaunchSite = selectedlaunchSiteName_();
        else if (editorFacility == EditorFacility::VAB)
            game->defaultVABLaunchSite = selectedlaunchSiteName_();
    }
}

// EditorDriver.cs:477
void EditorDriver::setDefaultLaunchSite() {
    const char* site = editorFacility == EditorFacility::SPH ? "Runway" : "LaunchPad";
    if (host.editorLogicPresent && host.editorLogicPresent() && host.setEditorLogicLaunchSite) host.setEditorLogicLaunchSite(site);
    selectedlaunchSiteName_() = site;
}

// EditorDriver.cs:497
bool EditorDriver::ValidLaunchSite(const std::string& siteName) {
    if (validLaunchSitesNull_) throw NullReferenceException();
    for (const std::string& s : validLaunchSites_)
        if (s == siteName) return true;
    return false;
}

// EditorDriver.cs:516
void EditorDriver::setupValidLaunchSites() {
    if (validLaunchSitesNull_) {
        validLaunchSitesNull_ = false;
        validLaunchSites_.clear();
    } else {
        validLaunchSites_.clear();
    }
    PSystemSetup* setup = PSystemSetup::Instance;
    if (setup == nullptr) throw NullReferenceException();
    for (size_t i = 0; i < setup->facilities.size(); i++) {
        if (setup->facilities[i].editorFacility == editorFacility) validLaunchSites_.push_back(setup->facilities[i].name);
    }
    const bool flag = OtherLaunchSitesValid();
    Game* game = HighLogic::CurrentGame();
    if (game == nullptr) throw NullReferenceException();
    const bool flag2 = game->Mode == Game_Modes::SANDBOX || game->Mode == Game_Modes::SCIENCE_SANDBOX || game->Mode == Game_Modes::CAREER;
    for (size_t j = 0; j < setup->launchsites.size(); j++) {
        LaunchSite* ls = setup->launchsites[j];
        if (ls->editorFacility == editorFacility) {
            bool flag3 = false;
            if (flag && ls->BundleName == "makinghistory_assets") {
                flag3 = true;
            } else if ((flag2 && ls->BundleName.empty()) || ls->BundleName == "stock") {
                flag3 = !ls->requiresPOIVisit || ProgressTracking::Instance() == nullptr || ProgressTracking::Instance()->NodeComplete({ls->poiName});
            }
            if (flag3) validLaunchSites_.push_back(ls->name);
        }
    }
}

// EditorDriver.cs:558
bool EditorDriver::OtherLaunchSitesValid() {
    Game* game = HighLogic::CurrentGame();
    if (SaveEnvironment::Current().IsExpansionInstalled("MakingHistory") && game != nullptr) {
        if (game->Mode == Game_Modes::MISSION_BUILDER) return false;
        if (game->Mode == Game_Modes::MISSION && host.missionVesselsPending && host.missionVesselsPending()) return false;
        if (((game->Mode == Game_Modes::SANDBOX || game->Mode == Game_Modes::SCIENCE_SANDBOX || game->Mode == Game_Modes::CAREER) && game->Parameters->Difficulty.AllowOtherLaunchSites) ||
            (game->Mode == Game_Modes::MISSION && game->Parameters->CustomParams<Expansions::Missions::MissionParamsExtras>().facilityOpenEditor &&
             game->Parameters->CustomParams<Expansions::Missions::MissionParamsExtras>().launchSitesOpen))
            return true;
    }
    return false;
}

// EditorDriver.cs:578
bool EditorDriver::SetDefaultSaveFolder(const std::string& path) {
    const bool exists = host.directoryExists ? host.directoryExists(path) : false;
    if (!exists) return false;
    DefaultCraftSavePath = path;
    return true;
}

}  // namespace ksp
