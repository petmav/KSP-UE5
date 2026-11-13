// WP-UE-02: the game host. The product-side composition of the flight scene that the test fixtures built by hand until now
// (tests/flight_integrator_fixture.hpp PadScene, physics_bubble_fixture.hpp BubbleScene, contact_fixture.hpp KerbinScene, the InputScene
// of tests/test_flight_input_feed.cpp, the VAB launch of tests/test_vab_launch.cpp), behind the C ABI v2 (kspcore/kspcore_game.h and
// kspcore_frame.h). There is no C# counterpart of the host as a whole: it is the Unity scene objects' wiring (FlightDriver, the flight
// scene's FlightInputHandler, TimeWarp, OrbitPhysicsManager, the PQS sphere, the KSC) done by code, in the order STATUS.md WP-20g
// "Host contract" lists. Every statement that stands for KSP code cites it in game_host.cpp.
//
// Two levels, as in the game:
//   * GameProcess: once per process (KSP's start-up scene): the game root, the GameDatabase and PartLoader data (GameData), the terrain
//     maps. Survives scenes and sims.
//   * GameHost: one per kspcore_sim. Owns everything a scene creates (the celestial system, the settings instance, HighLogic, the
//     scene composition) and puts every process-wide static it touched back when it goes (the way tests/test_hygiene.cpp does), so a
//     new GameHost can be made afterwards.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "kspcore/kspcore_frame.h"
#include "kspcore/kspcore_game.h"

namespace ksp::host {

struct GameConfig {
    std::string gameRoot;
    std::string dataDir;
    std::string extractionRoot;
    std::string savesDir;
    std::string physicsCfg;
    std::string reducedModelDir;
    int pqsHeightThreads = 0;
    int physicsThreads = 0;
};

using ProgressFn = std::function<void(float fraction, const char* stage)>;

class GameProcess {
public:
    // false (and `error`) when already initialised or the paths are unusable
    static bool Init(const GameConfig& config, std::string& error);
    static void Shutdown();
    static bool Initialised();
    static const GameConfig& Config();
    // GameDatabase + PartLoader + Physics.cfg + localisation
    static bool LoadDatabase(const ProgressFn& progress, std::string& error);
    static bool DatabaseLoaded();
    static int PartCount();
    static int ResourceCount();
    // model ids of snapshot parts (stable for the process)
    static uint32_t ModelId(const std::string& partName);
    static const char* ModelName(uint32_t id);
};

class GameHost {
public:
    GameHost();
    ~GameHost();
    GameHost(const GameHost&) = delete;
    GameHost& operator=(const GameHost&) = delete;

    // ---- game (kspcore_game.h) ----
    bool NewGame(int mode, const std::string& saveName);
    bool LoadGame(const std::string& saveName, const std::string& file);
    bool SaveGame(const std::string& saveName, const std::string& file);
    size_t ListSaves(const std::string& saveName, char* buffer, size_t capacity);
    bool QuickSave();
    bool QuickLoad();
    bool HasQuickSave();
    bool LaunchCraft(const std::string& craftPath, const std::string& launchSite);
    bool RevertToLaunch();
    bool LoadScene(int scene);
    bool SceneLoaded(int scene);
    int CurrentScene() const;

    // ---- frame (kspcore_frame.h) ----
    void SetInput(const kspcore_input_frame& input);
    size_t Submit(const kspcore_command* commands, size_t count);
    bool Frame(double dtReal);
    const kspcore_snapshot_view& Snapshot();

    const std::string& Error() const { return error_; }

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
    std::string error_;
};

// GameSettings by field name (kspcore_settings_*)
bool SettingsLoad(const std::string& path);
bool SettingsSave(const std::string& path);
bool SettingsGet(const std::string& name, std::string& value);
bool SettingsSet(const std::string& name, const std::string& value);

// FNV-1a 64 over the snapshot as documented in kspcore_frame.h (kspcore_snapshot_hash)
uint64_t SnapshotHash(const kspcore_snapshot_view& snapshot);

}  // namespace ksp::host
