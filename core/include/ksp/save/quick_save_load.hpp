// WP-19c: QuickSaveLoad (QuickSaveLoad.cs, 494 lines), the flight scene's F5 / F9 component: quicksave (GamePersistence.SaveGame of
// HighLogic.CurrentGame.Updated() as "quicksave" with a BACKUP, start scene FLIGHT, then optionally "persistent"), quickload (the upgrade pipeline,
// GamePersistence.LoadGameCfg, FlightDriver.StartAndFocusVessel or the Space Center), the "save as" / "load" file dialogs and the hold-to-load key.
//
// Ported as written: every decision, file name, save mode, event, the screen messages' localisation tags and durations, the pause and input-lock
// handling. Seams (`QuickSaveLoad::env`, all optional; unset = the neutral answer): the popup dialogs (PopupDialog / MultiOptionDialog /
// LoadGameDialog objects, their skins, MenuNavigation) are spawned through hooks and answered by calling the public handlers the C# delegates
// are (OnSaveAsTextInput, ConfirmDialog, OnSaveAsClose, ConfirmOverwrite / CancelOverwrite, InvalidNameOk, OnLoadClose);
// UIMasterController.CameraMode, EventSystem.current's selection, ScreenMessages and the mission objects of Making History are hooks.
// Keys: GameSettings.QUICKSAVE / QUICKLOAD / MODIFIER_KEY (KeyBinding through ExtendedInput) and UnityEngine.Input's Escape / Return /
// KeypadEnter (InputEnvironment). MonoBehaviour.Invoke("onHoldComplete", holdTime) is a pending call at Time.time + holdTime that the host runs
// with RunDelayedCalls(Time.time) after the frame's Update (Unity runs Invokes between Update and the coroutines).
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/version.hpp"
#include "ksp/save/game_persistence.hpp"
#include "ksp/save/upgrade_pipeline.hpp"

namespace ksp {

class Game;

class QuickSaveLoad {
public:
    using FinishedSaveLoadDialogCallback = std::function<void(const std::string& saveName)>;  // :13

    // QuickSaveLoad.loadDialogParameters (:15)
    struct loadDialogParameters {
        bool pauseGame = false;
        FinishedSaveLoadDialogCallback onLoadCloseReturn;
        loadDialogParameters(bool pauseGame_, FinishedSaveLoadDialogCallback onLoadCloseReturn_)  // :21
            : pauseGame(pauseGame_), onLoadCloseReturn(std::move(onLoadCloseReturn_)) {}
    };
    using FilenameCheckResults = GamePersistence::FilenameCheckResults;  // :28

    struct Environment {
        // UIMasterController.Instance.CameraMode (Update returns at once while the UI is hidden for screenshots); unset = false
        std::function<bool()> cameraMode;
        // EventSystem.current != null && EventSystem.current.currentSelectedGameObject == null (Return confirms the save-as dialog); unset = true
        std::function<bool()> nothingSelected;
        // ScreenMessages.PostScreenMessage(message, duration) of the component's screenMessage (UPPER_CENTER) and of the plain string form
        std::function<void(const std::string& message, float duration)> postScreenMessage;
        // PopupDialog "Quicksave" (SpawnSaveAsDialog: the text field starts with quicksaveFilename) / its Dismiss / gameObject.SetActive
        std::function<void(const std::string& initialName)> spawnSaveDialog;
        std::function<void()> dismissSaveDialog;
        std::function<void(bool active)> setSaveDialogActive;
        // PopupDialog "OverwriteSave" (#autoLOC_417256 with the name) / "InvalidSaveName" (#autoLOC_417273) / their Dismiss
        std::function<void(const std::string& message)> spawnConfirmDialog;
        std::function<void()> dismissConfirmDialog;
        // LoadGameDialog.Create(OnLoadClose, HighLogic.SaveFolder, persistent: false, skin)
        std::function<void(const std::string& saveFolder)> spawnLoadBrowser;
        std::function<void()> destroyLoadDialog;  // loadDialog.Destroy() (Escape)
        // Expansions.Missions: HighLogic.CurrentGame.missionToStart.InitMission() of a mission quicksave; unset = nothing
        std::function<void(Game&)> initMission;
        // KSPUpgradePipeline's failure dialog ("Cancel" / "Attempt to Load Anyway"); unset = Cancel (KSPUpgradePipeline::Process's default)
        std::function<KSPUpgradePipeline::UpgradeFailOption(const std::string& runInfo)> upgradeFailDialog;
    };
    static inline Environment env;

    static inline QuickSaveLoad* fetch = nullptr;  // :36

    bool AutoSaveOnQuickSave = false;  // :38
    bool holdToLoad = false;           // :40
    float holdTime = 1.0f;             // :42
    std::string quicksaveFilename;     // :66
    // screenMessage (:48, new ScreenMessage("", 3f, UPPER_CENTER) in Awake): its last text and duration
    std::string screenMessageText;
    float screenMessageDuration = 3.0f;

    QuickSaveLoad();   // Awake (:76): the singleton; a second instance destroys itself (Alive() == false)
    ~QuickSaveLoad();  // OnDestroy (:87)
    QuickSaveLoad(const QuickSaveLoad&) = delete;
    QuickSaveLoad& operator=(const QuickSaveLoad&) = delete;
    bool Alive() const { return alive_; }

    void start();   // :95 (the lower-case Unity message name of the C#: never called by Unity)
    void Update();  // :101
    void RunDelayedCalls(float time);  // the pending Invoke("onHoldComplete") whose time has come
    bool IsInvokingHoldComplete() const { return invokePending_; }

    static void QuickSave();                  // :189
    void quickSave(bool saveAs);              // :194
    static bool QuickSaveClearToSave();       // :226
    void doSave(const std::string& filename, const std::string& screenMsg = "#autoLOC_174669");  // :236
    void quickLoad(const std::string& filename, const std::string& folder);                      // :252
    void onQuickloadPipelineFailed(KSPUpgradePipeline::UpgradeFailOption opt, const ConfigNode::NodePtr& n, const std::string& folder,
                                   const dotnet::Version& originalVersion);                                               // :292
    void onQuickloadPipelineFinished(const ConfigNode::NodePtr& node, const std::string& saveName, const dotnet::Version& originalVersion);  // :305

    // the dialogs (the C# delegates of the dialog buttons are these public calls)
    void SpawnSaveAsDialog();                                                        // :331
    void SpawnSaveAsDialog(bool pauseGame, FinishedSaveLoadDialogCallback onSaveAsCloseReturn);  // :336
    std::string OnSaveAsTextInput(const std::string& n);                             // :352 the DialogGUITextInput callback
    void ConfirmDialog(bool pauseGame = true, FinishedSaveLoadDialogCallback onSaveAsCloseReturn = nullptr);  // :374 (the "Save" button)
    void ConfirmOverwrite();                                                         // :379 the "Overwrite" button of OverwriteSave
    void CancelOverwrite();                                                          // :383 its "Cancel" button / OnDismiss
    void InvalidNameOk();                                                            // :396 the button / OnDismiss of InvalidSaveName
    void OnSaveAsClose(const std::string& saveName, bool pauseGame = true, FinishedSaveLoadDialogCallback onSaveAsCloseReturn = nullptr);  // :413
    void SpawnQuickLoadDialog();                                                     // :429
    void SpawnQuickLoadDialog(bool pauseGame, FinishedSaveLoadDialogCallback onLoadCloseReturn);  // :434
    void OnLoadClose(const std::string& saveName);                                   // :448 (LoadGameDialog's callback)
    static void CacheLocalStrings();                                                 // :489

    bool SaveDialogOpen() const { return saveDialog_; }
    bool ConfirmDialogOpen() const { return confirmDialog_; }
    bool LoadBrowserOpen() const { return loadBrowser_; }

    static void ResetStatics();  // tests: fetch, env, the cached strings

private:
    void onHoldComplete();  // :184
    void Post(const std::string& message, float duration);

    bool alive_ = false;
    bool blockedSubmitOnStart_ = false;  // :64
    bool pauseGame_ = false;             // :58
    FinishedSaveLoadDialogCallback onSaveAsCloseReturn_;  // :56
    bool overwritePause_ = true;                      // ConfirmDialog's arguments, captured by the OverwriteSave delegate (:381)
    FinishedSaveLoadDialogCallback overwriteReturn_;
    bool saveDialog_ = false;     // saveDialog != null (:52)
    bool confirmDialog_ = false;  // confirmDialog != null (:54)
    bool loadDialog_ = false;     // loadDialog != null (:50: never assigned by the C#, so the Escape branch for it never runs)
    bool loadBrowser_ = false;    // loadBrowser != null (:68)
    std::unique_ptr<loadDialogParameters> loadDialogParams_;  // :70
    bool invokePending_ = false;
    float invokeAt_ = 0.0f;
    static inline std::string cacheAutoLOC_174575;  // :72
    static inline std::string cacheAutoLOC_174610;  // :74
};

}  // namespace ksp
