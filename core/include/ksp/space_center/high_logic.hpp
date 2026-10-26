// WP-42: HighLogic.cs (scene flow: LoadScene, the LoadedScene flags, the scene-loaded events), SceneTransitionMatrix.cs and the
// scene-driver statics of FlightDriver.cs / EditorDriver.cs (flight_driver.hpp / editor_driver.hpp).
//
// Storage: HighLogic.LoadedScene / LoadedSceneHasPlanetarium / LoadedSceneIsGame and CurrentGame are the SaveEnvironment fields WP-04
// introduced (other packages read them there); the remaining flags and GameSaveFolder live here.
// Seams: SceneManager.LoadScene / LoadSceneAsync / the loading-buffer scene (`sceneLoader`: WP-47), Time.time, MissionSystem
// (Expansions, unported), the MonoBehaviour scheduler.
#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>

#include "ksp/core/coroutine.hpp"
#include "ksp/events/game_event_types.hpp"
#include "ksp/save/save_enums.hpp"

namespace ksp {

class Game;

// SceneTransitionMatrix.cs:4 (a serialized matrix of which scene changes go through the loading buffer)
class SceneTransitionMatrix {
public:
    // :7
    class SceneTransitions {
    public:
        std::array<bool, 30> To{};                                         // :10
        bool operator[](GameScenes scn) const { return To[static_cast<size_t>(scn)]; }   // :12
    };
    std::array<SceneTransitions, 30> From{};                                // :16
    const SceneTransitions& operator[](GameScenes scn) const { return From[static_cast<size_t>(scn)]; }   // :18
    bool GetTransitionValue(GameScenes from, GameScenes to) const { return (*this)[from][to]; }          // :20
};

// HighLogic.cs:11
class HighLogic {
public:
    static inline HighLogic* fetch = nullptr;                 // :36
    static inline double gameRunTimeAdditive = 0.0;           // :27
    float gameRunTimeFrameLimit = 10.0f;                      // :30
    std::string GameSaveFolder = "default";                   // :38
    SceneTransitionMatrix sceneBufferTransitionMatrix;        // :42
    static inline double TimeSceneLoaded = 0.0;               // :44
    static inline bool FastEditorLoading = false;             // :46
    static inline bool LoadedSceneIsEditor = false;           // :48
    static inline bool LoadedSceneIsFlight = false;           // :50
    static inline bool LoadedSceneIsMissionBuilder = false;   // :56

    // HighLogic.LoadedScene / LoadedSceneHasPlanetarium / LoadedSceneIsGame (SaveEnvironment storage)
    static GameScenes LoadedScene();                          // :58
    static bool LoadedSceneHasPlanetarium();                  // :52
    static bool LoadedSceneIsGame();                          // :54
    static std::string SaveFolder();                          // :74 (null without fetch: "")
    static bool HasSaveFolder();
    static void SaveFolder(const std::string& value);         // :84 (the property setter)
    static Game* CurrentGame();                               // :93 (null without fetch)
    static void CurrentGame(Game* game);                      // :103 (the property setter)
    // WP-19c: the C# reference of fetch.currentGame keeps the game alive; Game.Load of a shared-owned game (a loaded save) stores it here
    static inline std::shared_ptr<Game> currentGameOwner;

    // seams ------------------------------------------------------------------------------------------------------------------
    // SceneManager.LoadScene(buildIndex) (buffered = false) / the loadingBuffer + LoadSceneAsync routine (buffered = true)
    static inline std::function<void(int buildIndex, bool buffered)> sceneLoader;
    // SceneManager.LoadSceneAsync(sceneName) (LoadSceneFromBundle)
    static inline std::function<void(const std::string& sceneName)> sceneLoaderByName;
    // MissionSystem.RemoveMissionObjects(removeAll: true) / RemoveMissonObject(CurrentGame.missionToStart.MissionInfo)
    static inline std::function<void()> removeMissionObjects;
    static inline std::function<void()> removeMissionToStartObject;
    static inline std::function<double()> timeNow;            // Time.time (TimeSceneLoaded without a Planetarium / game)
    static inline CoroutineScheduler* scheduler = nullptr;    // fetch.StartCoroutine target (unset: an internal scheduler, see Scheduler())
    static CoroutineScheduler& Scheduler();
    static void ResetStatics();                               // tests

    void Awake();                                             // :112
    void OnDestroy();                                         // :164
    void Update(float unscaledDeltaTime);                     // :173 (the joystick half is input: WP-47)
    void OnApplicationFocus(bool focus);                      // :202

    static void LoadScene(GameScenes scene);                  // :211
    static void LoadSceneFromBundle(GameScenes scene, const std::string& sceneName);   // :240
    static void SetLoadSceneEventsAndFlags(GameScenes scene, bool useAsyncBufferedLoad);   // :258
    static Enumerator bufferedLoad(int sceneToBeLoaded, bool loadAsync);   // :266
    static void setLevelFlags(GameScenes scene);              // :300
    void OnSceneLoaded(int buildIndex, bool additive);        // :310
    void OnLevelLoaded(GameScenes level);                     // :318
    Enumerator FireLoadedEvent(GameScenes scene);             // :327
    Enumerator FireLoadedEventGUIReady(GameScenes scene);     // :341
    static GameScenes GetLoadedGameSceneFromBuildIndex(int loadedSceneBuildIndex);   // :347
};

}  // namespace ksp
