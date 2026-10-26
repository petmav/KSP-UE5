// WP-19c test fixture: a flight scene on the Kerbol system (flight_fixture.hpp) with the part database of the GameData mirror, a HighLogic
// instance, a scenario runner and a scratch save folder (KSPUtil.ApplicationRootPath = <tmp>/data/../, HighLogic.SaveFolder = "wp19c").
#pragma once

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>

#include "flight_fixture.hpp"
#include "game_root.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/physics/flight_integrator.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/game_persistence.hpp"
#include "ksp/save/quick_save_load.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/save/upgrade_pipeline.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/util/ksp_util.hpp"

namespace wp19c {

using namespace ksp;
namespace fs = std::filesystem;

inline const std::string kSaves = std::string(KSPCORE_TESTDATA_DIR) + "/saves";

inline std::string ReadText(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

struct World : flighttest::FlightEnv {
    GameData data;
    HighLogic hl;
    fs::path tmp;
    fs::path folder;
    std::unique_ptr<ScenarioRunner> runner;
    World() {
        GameData::Options o;
        o.gameRoot = ksp::testing::GameRoot();
        data.Load(o);
        SaveEnvironment& env = SaveEnvironment::Current();
        env.getPartInfoByName = [this](std::string_view n) { return data.parts.getPartInfoByName(n); };
        env.partExists = [this](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
        env.newGuidHashCode = [] {
            static uint32_t s = 1919u;
            s = s * 1664525u + 1013904223u;
            return static_cast<int32_t>(s);
        };
        tmp = fs::temp_directory_path() / ("kspcore_wp19c_" + std::to_string(std::random_device{}()));
        folder = tmp / "saves" / "wp19c";
        fs::create_directories(folder);
        fs::create_directories(tmp / "data");
        KSPUtil::ApplicationDataPath = (tmp / "data").generic_string();
        hl.Awake();
        HighLogic::sceneLoader = [this](int buildIndex, bool) { loadedScenes.push_back(static_cast<GameScenes>(buildIndex)); };
        HighLogic::SaveFolder("wp19c");
        GameParameters::GetDefaultParameters(Game_Modes::SANDBOX, GameParameters::Preset::Normal);  // MainMenu.Start's first instance
        KSPUpgradePipeline::Reset();
        FlightDriver::ResetStatics();
        QuickSaveLoad::ResetStatics();
    }
    ~World() {
        if (extraHooked) events::GameEvents::onGameStateSave.Remove(&extra, &Extra::OnSave);
        EndFlight();
        runner.reset();
        FlightGlobals::Clear();
        Vessel::SpawnedVessels().clear();
        FlightDriver::ResetStatics();
        QuickSaveLoad::ResetStatics();
        HighLogic::currentGameOwner = nullptr;
        SaveEnvironment::Current().currentGame = nullptr;
        hl.OnDestroy();
        HighLogic::ResetStatics();
        KSPUtil::ApplicationDataPath.clear();
        std::error_code ec;
        fs::remove_all(tmp, ec);
    }
    std::vector<GameScenes> loadedScenes;

    // what Unity does on a scene load: the old scene's vessels go, the scene flags are set (HighLogic.LoadScene set them already), the
    // ScenarioRunner of the new scene starts
    void NewScene(GameScenes scene) {
        EndFlight();
        runner.reset();
        FlightGlobals::Clear();
        Vessel::SpawnedVessels().clear();
        HighLogic::setLevelFlags(scene);
        SaveEnvironment::Current().loadedScene = scene;
        runner = std::make_unique<ScenarioRunner>();
    }
    // the flight scene's FlightDriver.Awake / Start (FlightDriver.cs:108 / :120), then one FlightGlobals.FixedUpdate (ready)
    std::unique_ptr<FlightDriver> driver;
    void StartFlight() {
        driver = std::make_unique<FlightDriver>();
        driver->owner = 1919;
        driver->Awake();
        driver->Start();
        // the host contract (STATUS WP-20g): a loaded vessel entering the scene gets Part.Start with the module lifecycle before the first tick
        for (Vessel* v : FlightGlobals::Vessels)
            if (v->loaded) physics::StartVesselInFlight(*v);
        FlightGlobals::FixedUpdate();
    }
    // `frames` flight-scene frames without a physics world (the vessels stay packed: no rigid bodies) and without advancing the clock
    // (a quicksave of the next scene must see the UT the previous one saved)
    void Frames(int frames) {
        for (int i = 0; i < frames; i++) {
            FlightScene::FixedUpdate();
            FlightScene::Update();
            FlightScene::LateUpdate();
        }
    }
    void EndFlight() {
        if (!driver) return;
        HighLogic::Scheduler().StopAllCoroutines(driver->owner);  // the PostInit routine dies with its MonoBehaviour
        driver->OnDestroy();
        driver.reset();
    }
    // the environment that wrote a save (as tools/save_corpus does): GameDatabase.EnvironmentInfo from the raw line, the LoaderInfo flags, and
    // the GAME nodes other systems own (MessageSystem's MESSAGESYSTEM) given back through GameEvents.onGameStateSave
    struct Extra {
        std::vector<ConfigNode::NodePtr> nodes;
        void OnSave(ConfigNode* n) {
            for (const auto& c : nodes) n->AddNode(c->CreateCopy());
        }
    } extra;
    bool extraHooked = false;
    void WriterEnvironment(const std::string& raw) {
        SaveEnvironment& env = SaveEnvironment::Current();
        const std::string key = "\n\tenvInfo = ";
        const size_t at = raw.find(key);
        if (at != std::string::npos) {
            const size_t start = at + key.size();
            env.gameDatabaseEnvironmentInfo = raw.substr(start, raw.find_first_of("\r\n", start) - start);
        }
        auto root = ConfigNode::Parse(raw);
        const ConfigNode* game = root->GetNode("GAME");
        REQUIRE(game != nullptr);
        if (const ConfigNode* li = game->GetNode("LoaderInfo"))
            for (const auto& v : li->values())
                if (v.value == "True") env.gameDatabaseLoaderInfo.push_back(v.name);
        if (const ConfigNode* m = game->GetNode("MESSAGESYSTEM")) extra.nodes.push_back(m->CreateCopy());
        events::GameEvents::onGameStateSave.Add(&extra, &Extra::OnSave);
        extraHooked = true;
    }
    void CopySave(const std::string& from, const std::string& name) { fs::copy_file(from, folder / (name + ".sfs"), fs::copy_options::overwrite_existing); }
    std::string SaveText(const std::string& name) const { return ReadText(folder / (name + ".sfs")); }
};

}  // namespace wp19c
