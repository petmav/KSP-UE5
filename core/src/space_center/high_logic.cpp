// WP-42: HighLogic.cs.
#include "ksp/space_center/high_logic.hpp"

#include "ksp/career/career_environment.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/psystem_setup.hpp"

namespace ksp {

using GE = events::GameEvents;

CoroutineScheduler& HighLogic::Scheduler() { return scheduler != nullptr ? *scheduler : CareerEnvironment::Current().Scheduler(); }

void HighLogic::ResetStatics() {
    fetch = nullptr;
    gameRunTimeAdditive = 0.0;
    TimeSceneLoaded = 0.0;
    FastEditorLoading = false;
    LoadedSceneIsEditor = false;
    LoadedSceneIsFlight = false;
    LoadedSceneIsMissionBuilder = false;
    sceneLoader = nullptr;
    sceneLoaderByName = nullptr;
    removeMissionObjects = nullptr;
    removeMissionToStartObject = nullptr;
    timeNow = nullptr;
    scheduler = nullptr;
}

GameScenes HighLogic::LoadedScene() { return SaveEnvironment::Current().loadedScene; }
bool HighLogic::LoadedSceneHasPlanetarium() { return SaveEnvironment::Current().loadedSceneHasPlanetarium; }
bool HighLogic::LoadedSceneIsGame() { return SaveEnvironment::Current().loadedSceneIsGame; }

// HighLogic.cs:74
std::string HighLogic::SaveFolder() { return fetch == nullptr ? std::string() : fetch->GameSaveFolder; }
bool HighLogic::HasSaveFolder() { return fetch != nullptr; }

// HighLogic.cs:84
void HighLogic::SetSaveFolder(const std::string& value) {
    if (fetch != nullptr) fetch->GameSaveFolder = value;
}

// HighLogic.cs:93
Game* HighLogic::CurrentGame() { return fetch == nullptr ? nullptr : SaveEnvironment::Current().currentGame; }

// HighLogic.cs:103
void HighLogic::SetCurrentGame(Game* game) {
    if (fetch != nullptr) SaveEnvironment::Current().currentGame = game;
}

// HighLogic.cs:112 (the Destroy of a second instance, GameSettings, the culture and the analytics are the host's)
void HighLogic::Awake() {
    if (fetch != nullptr) return;
    fetch = this;
    gameRunTimeAdditive = 0.0;
}

// HighLogic.cs:164
void HighLogic::OnDestroy() {
    if (fetch != nullptr && fetch == this) fetch = nullptr;
}

// HighLogic.cs:173
void HighLogic::Update(float unscaledDeltaTime) {
    if (unscaledDeltaTime < gameRunTimeFrameLimit) gameRunTimeAdditive += static_cast<double>(unscaledDeltaTime);
}

// HighLogic.cs:202
void HighLogic::OnApplicationFocus(bool focus) {
    GE::OnAppFocus.Fire(focus);
    if (!focus) GE::onTooltipDestroyRequested.Fire();
}

// HighLogic.cs:211
void HighLogic::LoadScene(GameScenes scene) {
    if (LoadedSceneIsGame()) {
        if (scene == GameScenes::MAINMENU && PSystemSetup::Instance != nullptr) PSystemSetup::Instance->RemoveNonStockLaunchSites();
        if (scene == GameScenes::MAINMENU) {
            if (removeMissionObjects) removeMissionObjects();
            Game* game = CurrentGame();
            if (game != nullptr && game->missionToStart != nullptr && removeMissionToStartObject) removeMissionToStartObject();
        }
    }
    if (fetch == nullptr) throw NullReferenceException();
    const bool transitionValue = fetch->sceneBufferTransitionMatrix.GetTransitionValue(LoadedScene(), scene);
    SetLoadSceneEventsAndFlags(scene, transitionValue);
    if (transitionValue) {
        Scheduler().StartCoroutine(bufferedLoad(static_cast<int>(scene), true));
    } else if (sceneLoader) {
        sceneLoader(static_cast<int>(scene), false);
    }
}

// HighLogic.cs:240
void HighLogic::LoadSceneFromBundle(GameScenes scene, const std::string& sceneName) {
    SetLoadSceneEventsAndFlags(scene, true);
    if (LoadedSceneIsGame() && scene == GameScenes::MAINMENU && PSystemSetup::Instance != nullptr) PSystemSetup::Instance->RemoveNonStockLaunchSites();
    if (scene == GameScenes::MAINMENU) {
        if (removeMissionObjects) removeMissionObjects();
        Game* game = CurrentGame();
        if (game != nullptr && game->missionToStart != nullptr && removeMissionToStartObject) removeMissionToStartObject();
    }
    if (sceneLoaderByName) sceneLoaderByName(sceneName);
}

// HighLogic.cs:258
void HighLogic::SetLoadSceneEventsAndFlags(GameScenes scene, bool useAsyncBufferedLoad) {
    GE::onGameSceneLoadRequested.Fire(scene);
    GE::onGameSceneSwitchRequested.Fire(GE::FromToAction<GameScenes, GameScenes>(LoadedScene(), scene));
    log::Info("[HighLogic]: =========================== Scene Change : From " + dotnet::EnumToString(LoadedScene()) + " to " + dotnet::EnumToString(scene) +
              (useAsyncBufferedLoad ? " (Async) " : " ") + "=====================");
    setLevelFlags(scene);
}

// HighLogic.cs:266 (Application.backgroundLoadingPriority, Resources.UnloadUnusedAssets, GC.Collect and the AsyncOperation progress
// are the engine's: `sceneLoader(index, true)` performs the loadingBuffer + async load; the wait yields are kept)
Enumerator HighLogic::bufferedLoad(int sceneToBeLoaded, bool loadAsync) {
    if (loadAsync) {
        if (sceneLoader) sceneLoader(sceneToBeLoaded, true);
        co_yield YieldInstruction::WaitForEndOfFrame();
    } else {
        if (sceneLoader) sceneLoader(sceneToBeLoaded, true);
        co_yield YieldInstruction::WaitForSeconds(0.1f);
    }
}

// HighLogic.cs:300
void HighLogic::setLevelFlags(GameScenes scene) {
    SaveEnvironment& env = SaveEnvironment::Current();
    LoadedSceneIsEditor = scene == GameScenes::EDITOR;
    env.loadedSceneHasPlanetarium = scene == GameScenes::FLIGHT || scene == GameScenes::TRACKSTATION || scene == GameScenes::SPACECENTER;
    LoadedSceneIsFlight = scene == GameScenes::FLIGHT;
    env.loadedSceneIsGame = scene == GameScenes::SPACECENTER || scene == GameScenes::EDITOR || scene == GameScenes::FLIGHT || scene == GameScenes::TRACKSTATION;
    LoadedSceneIsMissionBuilder = scene == GameScenes::MISSIONBUILDER;
    env.loadedScene = scene;
}

// HighLogic.cs:310
void HighLogic::OnSceneLoaded(int buildIndex, bool additive) {
    if (!additive) OnLevelLoaded(GetLoadedGameSceneFromBuildIndex(buildIndex));
}

// HighLogic.cs:318
void HighLogic::OnLevelLoaded(GameScenes level) {
    if (level != GameScenes::LOADINGBUFFER) {
        Scheduler().StartCoroutine(FireLoadedEvent(level));
        Scheduler().StartCoroutine(FireLoadedEventGUIReady(level));
    }
}

// HighLogic.cs:327
Enumerator HighLogic::FireLoadedEvent(GameScenes scene) {
    if (Planetarium::fetch == nullptr && CurrentGame() == nullptr) {
        TimeSceneLoaded = timeNow ? timeNow() : 0.0;
    } else {
        TimeSceneLoaded = SaveUniversalTime();
    }
    co_yield YieldInstruction::Null();
    GE::onLevelWasLoaded.Fire(scene);
}

// HighLogic.cs:341
Enumerator HighLogic::FireLoadedEventGUIReady(GameScenes scene) {
    co_yield YieldInstruction::Null();
    GE::onLevelWasLoadedGUIReady.Fire(scene);
}

// HighLogic.cs:347
GameScenes HighLogic::GetLoadedGameSceneFromBuildIndex(int loadedSceneBuildIndex) {
    if (loadedSceneBuildIndex == -1) return LoadedScene();
    return static_cast<GameScenes>(loadedSceneBuildIndex);
}

}  // namespace ksp
