// WP-06 module host, scenario side: ScenarioRunner (ScenarioRunner.cs), ScenarioModuleFactory (AssemblyLoader.GetClassByName
// (typeof(ScenarioModule), name) + AddComponent) over the WP-04 data model (ScenarioModule / ProtoScenarioModule in
// ksp/save/game.hpp, KSPScenarioType in ksp/save/game_persistence.hpp).
//
// The runner is the GameObject component that owns the live ScenarioModules of the current scene: SetProtoModules loads the
// ProtoScenarioModules whose target scenes contain the loaded scene (MissionSystem first), a scene load request destroys the
// modules, GetUpdatedProtoModules re-snapshots the live ones for Game.Updated().
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/save/game.hpp"

namespace ksp {

class ScenarioModuleFactory {
public:
    using Creator = std::function<std::unique_ptr<ScenarioModule>(const std::string&)>;
    // Register the C++ implementation of a ScenarioModule type (exact Type.Name). Types without one are instantiated as the
    // generic ScenarioModule (host-schema [KSPField]s, no behaviour).
    static void Register(const std::string& className, Creator creator);
    // null when no ScenarioModule type of that name exists (the schema or a registration)
    static std::unique_ptr<ScenarioModule> Create(std::string_view className);
};

class ScenarioRunner {
public:
    ScenarioRunner();   // Awake: becomes the Instance (a second runner is destroyed), subscribes the scene events
    virtual ~ScenarioRunner();
    static ScenarioRunner* Instance() { return instance_; }

    // :122 AddModule(string) / :140 AddModule(ConfigNode) -> Load(node); the node's `name` selects the type
    ScenarioModule* AddModule(std::string_view moduleName);
    ScenarioModule* AddModule(const ConfigNode& node);

    // :183 / :190 / :198 / :208 (static wrappers over Instance)
    static std::vector<std::unique_ptr<ScenarioModule>>& GetLoadedModules();
    static std::vector<std::shared_ptr<ProtoScenarioModule>>& GetUpdatedProtoModules();
    static void SetProtoModules(std::vector<std::shared_ptr<ProtoScenarioModule>> protoModules);
    static void SetProtoModules(std::shared_ptr<ProtoScenarioModule> protoModule);
    static void RemoveModule(ScenarioModule* module);

    // ProtoScenarioModule.cs:66 Load(ScenarioRunner host)
    ScenarioModule* LoadProto(ProtoScenarioModule& proto);

    // HighLogic.LoadedScene as the runner sees it (SaveEnvironment::Current().loadedScene by default)
    std::function<GameScenes()> loadedScene;

protected:
    virtual void OnGameSceneLoadRequested(GameScenes scene);  // :39
    virtual void onLevelWasLoaded(GameScenes scene);           // :54
    virtual void LoadModules(std::vector<std::shared_ptr<ProtoScenarioModule>> protoModules);  // :119
    virtual void ClearModules();                               // :145
    virtual void UpdateModules();                              // :158

private:
    void AddMainMenuScenarios();  // :64
    static ScenarioRunner* instance_;
    std::vector<std::shared_ptr<ProtoScenarioModule>> protoModules_;
    std::vector<std::unique_ptr<ScenarioModule>> modules_;
    bool subscribed_ = false;
    bool hookInstalled_ = false;
};

}  // namespace ksp
