// WP-19c: QuickSaveLoad (QuickSaveLoad.cs); see include/ksp/save/quick_save_load.hpp for the seams.
#include "ksp/save/quick_save_load.hpp"

#include <filesystem>
#include <system_error>

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_binding.hpp"
#include "ksp/input/input_device.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/serenity/roc.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
// File.Exists(KSPUtil.ApplicationRootPath + "saves/" + HighLogic.SaveFolder + "/" + name + ".sfs")
bool SaveFileExists(const std::string& name) {
    std::error_code ec;
    return std::filesystem::is_regular_file(std::filesystem::path(dotnet::Utf8ToWide(KSPUtil::ApplicationRootPath() + "saves/" + HighLogic::SaveFolder() + "/" + name + ".sfs")), ec);
}
Game& Current() {
    Game* g = HighLogic::CurrentGame();
    if (g == nullptr) throw NullReferenceException("HighLogic.CurrentGame is null");
    return *g;
}
const GameParameters& Params(Game& g) {
    if (!g.Parameters) throw NullReferenceException("Game.Parameters is null");
    return *g.Parameters;
}
bool KeyDown(const std::shared_ptr<KeyBinding>& k) { return k != nullptr && k->GetKeyDown(); }
bool KeyUp(const std::shared_ptr<KeyBinding>& k) { return k != nullptr && k->GetKeyUp(); }
bool KeyHeld(const std::shared_ptr<KeyBinding>& k) { return k != nullptr && k->GetKey(); }
}  // namespace

void QuickSaveLoad::ResetStatics() {
    fetch = nullptr;
    env = Environment{};
    cacheAutoLOC_174575.clear();
    cacheAutoLOC_174610.clear();
}

void QuickSaveLoad::Post(const std::string& message, float duration) {
    if (env.postScreenMessage) env.postScreenMessage(message, duration);
}

// QuickSaveLoad.cs:76
QuickSaveLoad::QuickSaveLoad() {
    if (fetch != nullptr) return;  // UnityEngine.Object.Destroy(this)
    fetch = this;
    alive_ = true;
    screenMessageText = "";
    screenMessageDuration = 3.0f;
}

// QuickSaveLoad.cs:87
QuickSaveLoad::~QuickSaveLoad() {
    if (fetch != nullptr && fetch == this) fetch = nullptr;
}

// QuickSaveLoad.cs:95 (UISkinManager.GetSkin("MiniSettingsSkin") is the dialogs' skin)
void QuickSaveLoad::start() { blockedSubmitOnStart_ = false; }

// QuickSaveLoad.cs:101
void QuickSaveLoad::Update() {
    if (env.cameraMode && env.cameraMode()) return;
    const IInputDevice& input = InputEnvironment::Input();
    if (input.GetKeyUp(KeyCode::Escape)) {
        if (saveDialog_) {
            OnSaveAsClose("");
            if (confirmDialog_) {
                if (env.dismissConfirmDialog) env.dismissConfirmDialog();
                confirmDialog_ = false;
            }
        } else if (loadDialog_) {
            OnLoadClose("");
            if (env.destroyLoadDialog) env.destroyLoadDialog();
            loadDialog_ = false;
        }
    }
    if ((input.GetKeyUp(KeyCode::Return) || input.GetKeyUp(KeyCode::KeypadEnter)) && saveDialog_) {
        if (blockedSubmitOnStart_) {
            blockedSubmitOnStart_ = false;
        } else if (!env.nothingSelected || env.nothingSelected()) {
            ConfirmDialog(pauseGame_, onSaveAsCloseReturn_);
        }
    }
    if (InputLockManager::IsUnlocked(ControlTypes::QUICKSAVE) && KeyDown(GameSettings::QUICKSAVE)) quickSave(KeyHeld(GameSettings::MODIFIER_KEY));
    if (InputLockManager::IsUnlocked(ControlTypes::QUICKLOAD) && KeyDown(GameSettings::QUICKLOAD)) {
        if (!Params(Current()).Flight.CanQuickLoad) {
            screenMessageText = Localizer::Format("#autoLOC_174575");  // cacheAutoLOC_174575
            screenMessageDuration = 2.0f;
            Post(screenMessageText, screenMessageDuration);
            return;
        }
        if (holdToLoad && !FlightDriver::Pause()) {
            if (!KeyHeld(GameSettings::MODIFIER_KEY)) {
                screenMessageText = Localizer::Format("#autoLOC_174589", {GameSettings::QUICKLOAD->name()});
                screenMessageDuration = holdTime;
                Post(screenMessageText, screenMessageDuration);
            }
            if (!invokePending_) {  // Invoke("onHoldComplete", holdTime) unless IsInvoking
                invokePending_ = true;
                invokeAt_ = Time::time + holdTime;
            }
        } else {
            quickLoad("quicksave", HighLogic::SaveFolder());
        }
    }
    if (KeyUp(GameSettings::QUICKLOAD) && invokePending_) invokePending_ = false;  // CancelInvoke("onHoldComplete")
    if (InputLockManager::IsUnlocked(ControlTypes::QUICKLOAD) && KeyHeld(GameSettings::MODIFIER_KEY) && KeyDown(GameSettings::QUICKLOAD)) {
        if (!Params(Current()).Flight.CanQuickLoad) {
            screenMessageText = Localizer::Format("#autoLOC_174610");  // cacheAutoLOC_174610
            screenMessageDuration = 2.0f;
            Post(screenMessageText, screenMessageDuration);
        } else {
            SpawnQuickLoadDialog();
        }
    }
}

void QuickSaveLoad::RunDelayedCalls(float time) {
    if (invokePending_ && time >= invokeAt_) {
        invokePending_ = false;
        onHoldComplete();
    }
}

// QuickSaveLoad.cs:184
void QuickSaveLoad::onHoldComplete() { quickLoad("quicksave", HighLogic::SaveFolder()); }

// QuickSaveLoad.cs:189
void QuickSaveLoad::QuickSave() {
    if (fetch == nullptr) throw NullReferenceException("QuickSaveLoad.fetch is null");
    fetch->quickSave(false);
}

// QuickSaveLoad.cs:194
void QuickSaveLoad::quickSave(bool saveAs) {
    if (!Params(Current()).Flight.CanQuickSave) {
        screenMessageText = Localizer::Format("#autoLOC_174631");
        screenMessageDuration = 2.0f;
        Post(screenMessageText, screenMessageDuration);
        return;
    }
    if (FlightGlobals::ActiveVessel == nullptr) throw NullReferenceException("FlightGlobals.ActiveVessel is null");
    if (FlightGlobals::ActiveVessel->state != Vessel::State::DEAD) {
        if (QuickSaveClearToSave()) {
            if (saveAs) SpawnSaveAsDialog();
            else doSave("quicksave");
        } else {
            Post(FlightGlobals::GetNotClearToSaveStatusReason(FlightGlobals::ClearToSave(), Localizer::Format("#autoLOC_174654")), 2.0f);
        }
    } else {
        Post(Localizer::Format("#autoLOC_174659"), 2.0f);
    }
}

// QuickSaveLoad.cs:226
bool QuickSaveLoad::QuickSaveClearToSave() {
    const ClearToSaveStatus s = FlightGlobals::ClearToSave(false);
    if (s != ClearToSaveStatus::CLEAR && !GameSettings::CAN_ALWAYS_QUICKSAVE &&
        (s == ClearToSaveStatus::NOT_WHILE_ON_A_LADDER || s == ClearToSaveStatus::NOT_WHILE_MOVING_OVER_SURFACE ||
         s == ClearToSaveStatus::NOT_WHILE_ABOUT_TO_CRASH))
        return false;
    return true;
}

// QuickSaveLoad.cs:236
void QuickSaveLoad::doSave(const std::string& filename, const std::string& screenMsg) {
    GE::onGameAboutToQuicksave.Fire();
    Game& game = Current().Updated();
    game.startScene = GameScenes::FLIGHT;
    GamePersistence::SaveGame(game, filename, HighLogic::SaveFolder(), SaveMode::BACKUP);
    screenMessageText = Localizer::Format(screenMsg);
    screenMessageDuration = 2.0f;
    Post(screenMessageText, screenMessageDuration);
    game.startScene = GameScenes::SPACECENTER;
    if (FlightGlobals::ClearToSave() == ClearToSaveStatus::CLEAR && AutoSaveOnQuickSave)
        GamePersistence::SaveGame(game, "persistent", HighLogic::SaveFolder(), SaveMode::OVERWRITE);
}

// QuickSaveLoad.cs:252
void QuickSaveLoad::quickLoad(const std::string& filename, const std::string& folder) {
    if (ROCManager::Instance != nullptr) ROCManager::Instance->ClearROCsCache();
    ConfigNode::NodePtr configNode = GamePersistence::LoadSFSFile(filename, folder);
    if (configNode == nullptr) {
        screenMessageText = Localizer::Format("#autoLOC_174686", {std::string("#autoLOC_6002266")});
        screenMessageDuration = 2.0f;
        Post(screenMessageText, screenMessageDuration);
        return;
    }
    Game* current = HighLogic::CurrentGame();
    if (current != nullptr && current->Mode == Game::Modes::MISSION && configNode->HasNode("GAME") &&
        configNode->GetNode("GAME")->HasNode("MISSIONTOSTART")) {
        ConfigNode::NodePtr configNode2 = configNode;
        std::shared_ptr<Game> loaded(GamePersistence::LoadGameCfg(configNode2, filename, true, false));
        HighLogic::CurrentGame(loaded.get());
        if (HighLogic::fetch != nullptr) HighLogic::currentGameOwner = loaded;
        HighLogic::SaveFolder(folder);
        Game& g = Current();  // a null (incompatible) game: GamePersistence.UpdateScenarioModules(null) throws
        GamePersistence::UpdateScenarioModules(g, KSPScenarioType::GetAllScenarioTypesInAssemblies());
        if (configNode2 != nullptr) GE::onGameStatePostLoad.Fire(configNode2.get());
        if (env.initMission) env.initMission(g);  // HighLogic.CurrentGame.missionToStart.InitMission()
        GamePersistence::SaveGame(g, "persistent", folder, SaveMode::OVERWRITE);
        g.Start();
        return;
    }
    const dotnet::Version originalVersion = SaveUpgradePipeline::NodeUtil::GetCfgVersion(*configNode, SaveUpgradePipeline::LoadContext::SFS);
    KSPUpgradePipeline::Process(
        configNode, folder, SaveUpgradePipeline::LoadContext::SFS,
        [this, folder, originalVersion](const ConfigNode::NodePtr& n) { onQuickloadPipelineFinished(n, folder, originalVersion); },
        [this, folder, originalVersion](KSPUpgradePipeline::UpgradeFailOption opt, const ConfigNode::NodePtr& n) {
            onQuickloadPipelineFailed(opt, n, folder, originalVersion);
        },
        env.upgradeFailDialog);
}

// QuickSaveLoad.cs:292
void QuickSaveLoad::onQuickloadPipelineFailed(KSPUpgradePipeline::UpgradeFailOption opt, const ConfigNode::NodePtr& n, const std::string& folder,
                                              const dotnet::Version& originalVersion) {
    switch (opt) {
        case KSPUpgradePipeline::UpgradeFailOption::LoadAnyway: onQuickloadPipelineFinished(n, folder, originalVersion); break;
        case KSPUpgradePipeline::UpgradeFailOption::Cancel: FlightDriver::SetPause(false); break;
    }
}

// QuickSaveLoad.cs:305 (saveName is the folder: quickLoad passes it so -- the upgrade pipeline's saveName argument)
void QuickSaveLoad::onQuickloadPipelineFinished(const ConfigNode::NodePtr& node, const std::string& saveName, const dotnet::Version& originalVersion) {
    std::shared_ptr<Game> game(GamePersistence::LoadGameCfg(node, saveName, true, false));
    if (game != nullptr && game->flightState != nullptr && game->compatible) {
        GamePersistence::UpdateScenarioModules(*game, KSPScenarioType::GetAllScenarioTypesInAssemblies());
        if (node != nullptr) GE::onGameStatePostLoad.Fire(node.get());
        if (game->startScene != GameScenes::FLIGHT && !(originalVersion < dotnet::Version(0, 24, 0))) {
            GamePersistence::SaveGame(*game, "persistent", HighLogic::SaveFolder(), SaveMode::OVERWRITE);
            HighLogic::LoadScene(GameScenes::SPACECENTER);
        } else {
            FlightDriver::StartAndFocusVessel(game, game->flightState->activeVesselIdx);
        }
    } else {
        FlightDriver::SetPause(false);
    }
}

// QuickSaveLoad.cs:331
void QuickSaveLoad::SpawnSaveAsDialog() { SpawnSaveAsDialog(true, nullptr); }

// QuickSaveLoad.cs:336
void QuickSaveLoad::SpawnSaveAsDialog(bool pauseGame, FinishedSaveLoadDialogCallback onSaveAsCloseReturn) {
    const IInputDevice& input = InputEnvironment::Input();
    blockedSubmitOnStart_ = input.GetKey(KeyCode::Return) || input.GetKey(KeyCode::KeypadEnter);
    pauseGame_ = pauseGame;
    onSaveAsCloseReturn_ = onSaveAsCloseReturn;
    if (pauseGame) FlightDriver::SetPause(true);
    InputLockManager::SetControlLock("SaveAsDialog");
    quicksaveFilename = GamePersistence::NextQuicksaveFilename(HighLogic::SaveFolder());  // :346-351 ("quicksave", "quicksave #1", ...)
    saveDialog_ = true;
    if (env.spawnSaveDialog) env.spawnSaveDialog(quicksaveFilename);
}

// QuickSaveLoad.cs:352 (the text field's callback: quicksaveFilename = EscapeString(n))
std::string QuickSaveLoad::OnSaveAsTextInput(const std::string& n) {
    quicksaveFilename = GamePersistence::EscapeString(n);
    return quicksaveFilename;
}

// QuickSaveLoad.cs:374
void QuickSaveLoad::ConfirmDialog(bool pauseGame, FinishedSaveLoadDialogCallback onSaveAsCloseReturn) {
    if (SaveFileExists(quicksaveFilename)) {
        if (env.setSaveDialogActive) env.setSaveDialogActive(false);
        confirmDialog_ = true;
        overwritePause_ = pauseGame;  // the "Overwrite" delegate captures ConfirmDialog's arguments
        overwriteReturn_ = onSaveAsCloseReturn;
        if (env.spawnConfirmDialog) env.spawnConfirmDialog(Localizer::Format("#autoLOC_417256", {quicksaveFilename}));
    } else if (GamePersistence::CheckFilename(quicksaveFilename) != FilenameCheckResults::Good) {
        confirmDialog_ = true;
        if (env.spawnConfirmDialog) env.spawnConfirmDialog(Localizer::Format("#autoLOC_417273"));
    } else {
        OnSaveAsClose(quicksaveFilename, pauseGame, onSaveAsCloseReturn);
    }
}

// QuickSaveLoad.cs:379-382 (OverwriteSave, "#autoLOC_174798")
void QuickSaveLoad::ConfirmOverwrite() {
    OnSaveAsClose(quicksaveFilename, overwritePause_, overwriteReturn_);
    if (env.dismissConfirmDialog) env.dismissConfirmDialog();
    confirmDialog_ = false;
}

// QuickSaveLoad.cs:383-392 (OverwriteSave "#autoLOC_174804" and OnDismiss)
void QuickSaveLoad::CancelOverwrite() {
    if (env.dismissConfirmDialog) env.dismissConfirmDialog();
    confirmDialog_ = false;
    if (env.setSaveDialogActive) env.setSaveDialogActive(true);
}

// QuickSaveLoad.cs:396-405 (InvalidSaveName "#autoLOC_174814" and OnDismiss)
void QuickSaveLoad::InvalidNameOk() { CancelOverwrite(); }

// QuickSaveLoad.cs:413 (doSave(quicksaveFilename), not saveName -- the two are equal from every caller)
void QuickSaveLoad::OnSaveAsClose(const std::string& saveName, bool pauseGame, FinishedSaveLoadDialogCallback onSaveAsCloseReturn) {
    if (!saveDialog_) throw NullReferenceException("QuickSaveLoad.saveDialog is null");  // saveDialog.Dismiss()
    if (env.dismissSaveDialog) env.dismissSaveDialog();
    saveDialog_ = false;
    InputLockManager::RemoveControlLock("SaveAsDialog");
    if (!saveName.empty() && GamePersistence::CheckFilename(saveName) == FilenameCheckResults::Good) doSave(quicksaveFilename);
    if (pauseGame) FlightDriver::SetPause(false);
    if (onSaveAsCloseReturn) onSaveAsCloseReturn(saveName);
    blockedSubmitOnStart_ = false;
}

// QuickSaveLoad.cs:429
void QuickSaveLoad::SpawnQuickLoadDialog() { SpawnQuickLoadDialog(true, nullptr); }

// QuickSaveLoad.cs:434
void QuickSaveLoad::SpawnQuickLoadDialog(bool pauseGame, FinishedSaveLoadDialogCallback onLoadCloseReturn) {
    loadDialogParams_ = std::make_unique<loadDialogParameters>(pauseGame, std::move(onLoadCloseReturn));
    if (pauseGame) FlightDriver::SetPause(true);
    InputLockManager::SetControlLock("LoadFileDialog");
    if (!loadBrowser_) {
        loadBrowser_ = true;
        if (env.spawnLoadBrowser) env.spawnLoadBrowser(HighLogic::SaveFolder());
    }
}

// QuickSaveLoad.cs:448
void QuickSaveLoad::OnLoadClose(const std::string& saveName) {
    loadBrowser_ = false;
    InputLockManager::RemoveControlLock("LoadFileDialog");
    if (!saveName.empty()) {
        // Path.GetFileNameWithoutExtension(saveName)
        std::string name = saveName;
        const size_t slash = name.find_last_of("/\\");
        if (slash != std::string::npos) name = name.substr(slash + 1);
        const size_t dot = name.find_last_of('.');
        if (dot != std::string::npos) name = name.substr(0, dot);
        quickLoad(name, HighLogic::SaveFolder());
    }
    if (loadDialogParams_ == nullptr) throw NullReferenceException("QuickSaveLoad.loadDialogParams is null");
    if (loadDialogParams_->pauseGame) FlightDriver::SetPause(false);
    if (loadDialogParams_->onLoadCloseReturn) loadDialogParams_->onLoadCloseReturn(saveName);
    loadDialogParams_.reset();
}

// QuickSaveLoad.cs:489 (Localizer._CacheLocalStrings calls it at start-up and on a language switch; kspcore formats the two tags where they are
// shown, the same text once the Localizer is up)
void QuickSaveLoad::CacheLocalStrings() {
    cacheAutoLOC_174575 = Localizer::Format("#autoLOC_174575");
    cacheAutoLOC_174610 = Localizer::Format("#autoLOC_174610");
}

}  // namespace ksp
