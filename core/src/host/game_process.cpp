// WP-UE-02: GameProcess, the once-per-process half of the game host (include/ksp/host/game_host.hpp). Host glue with no C# counterpart:
// it stands for KSP's start-up scene (the LOADING scene: GameDatabase.Load, PartLoader, Localizer, PhysicsGlobals) done once before any
// game scene exists. The calls it makes are the same ones tests/vessel_physics_fixture.hpp World and tests/wp19c_fixture.hpp World made.
#include <filesystem>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "ksp/database/game_data.hpp"
#include "ksp/host/game_host.hpp"
#include "host_internal.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp::host {

namespace {

struct ProcessState {
    GameConfig config;
    bool initialised = false;
    bool databaseLoaded = false;
    std::unique_ptr<GameData> data;
    std::unique_ptr<terrain::MapLibrary> maps;
    std::vector<std::string> modelNames{""};  // id 0 = unknown
    std::map<std::string, uint32_t> modelIds;
};

ProcessState& State() {
    static ProcessState s;
    return s;
}

}  // namespace

// host glue (no C#): the process-level objects the sims share
GameData& ProcessGameData() { return *State().data; }
terrain::MapLibrary& ProcessMaps() {
    ProcessState& s = State();
    if (!s.maps) s.maps = std::make_unique<terrain::MapLibrary>(s.config.dataDir + "/terrain");
    return *s.maps;
}

bool GameProcess::Init(const GameConfig& config, std::string& error) {
    ProcessState& s = State();
    if (s.initialised) {
        error = "kspcore_game_init: already initialised";
        return false;
    }
    namespace fs = std::filesystem;
    std::error_code ec;
    if (config.gameRoot.empty() || !fs::exists(fs::path(config.gameRoot) / "GameData", ec)) {
        error = "kspcore_game_init: game_root has no GameData folder: " + config.gameRoot;
        return false;
    }
    if (config.dataDir.empty() || !fs::exists(fs::path(config.dataDir) / "terrain", ec)) {
        error = "kspcore_game_init: data_dir has no terrain folder: " + config.dataDir;
        return false;
    }
    if (config.savesDir.empty()) {
        error = "kspcore_game_init: saves_dir is empty";
        return false;
    }
    fs::create_directories(fs::path(config.savesDir), ec);
    s.config = config;
    if (s.config.physicsCfg.empty()) s.config.physicsCfg = (fs::path(config.gameRoot) / "Physics.cfg").generic_string();
    s.initialised = true;
    return true;
}

void GameProcess::Shutdown() {
    ProcessState& s = State();
    s.maps.reset();
    s.data.reset();
    s.databaseLoaded = false;
    s.initialised = false;
    s.modelNames.assign(1, "");
    s.modelIds.clear();
}

bool GameProcess::Initialised() { return State().initialised; }
const GameConfig& GameProcess::Config() { return State().config; }
bool GameProcess::DatabaseLoaded() { return State().databaseLoaded; }

bool GameProcess::LoadDatabase(const ProgressFn& progress, std::string& error) {
    ProcessState& s = State();
    if (!s.initialised) {
        error = "kspcore_game_load_database: kspcore_game_init has not run";
        return false;
    }
    if (s.databaseLoaded) return true;
    auto report = [&](float f, const char* stage) {
        if (progress) progress(f, stage);
    };
    report(0.0f, "GameDatabase");
    // KSPUtil.ApplicationRootPath = Application.dataPath + "/../" (KSPUtil.cs:795): the saves folder's parent is the root, so
    // <saves_dir> = <root>/saves (GamePersistence reads ApplicationRootPath + "saves/")
    {
        namespace fs = std::filesystem;
        const fs::path saves = fs::path(s.config.savesDir).lexically_normal();
        const fs::path root = saves.has_filename() ? saves.parent_path() : saves.parent_path().parent_path();
        fs::create_directories(root / "data");
        KSPUtil::ApplicationDataPath = (root / "data").generic_string();
    }
    s.data = std::make_unique<GameData>();
    GameData::Options o;
    o.gameRoot = s.config.gameRoot;
    s.data->Load(o);  // GameDatabase.Load, PhysicsGlobals / Physics.cfg, resources, PhysicMaterials, PartLoader.CompileAll
    report(0.9f, "PartLoader");
    for (const auto& up : s.data->parts.loadedParts) {
        const AvailablePart* p = up.get();
        if (p == nullptr) continue;
        if (s.modelIds.find(p->name) == s.modelIds.end()) {
            s.modelIds[p->name] = static_cast<uint32_t>(s.modelNames.size());
            s.modelNames.push_back(p->name);
        }
    }
    s.databaseLoaded = true;
    report(1.0f, "done");
    return true;
}

int GameProcess::PartCount() { return State().data ? static_cast<int>(State().data->parts.loadedParts.size()) : 0; }
int GameProcess::ResourceCount() { return State().data ? static_cast<int>(std::distance(State().data->resources.resourceDefinitions.begin(), State().data->resources.resourceDefinitions.end())) : 0; }

uint32_t GameProcess::ModelId(const std::string& partName) {
    ProcessState& s = State();
    const auto it = s.modelIds.find(partName);
    if (it != s.modelIds.end()) return it->second;
    s.modelIds[partName] = static_cast<uint32_t>(s.modelNames.size());
    s.modelNames.push_back(partName);
    return s.modelIds[partName];
}

const char* GameProcess::ModelName(uint32_t id) {
    ProcessState& s = State();
    return id < s.modelNames.size() ? s.modelNames[id].c_str() : "";
}

}  // namespace ksp::host
