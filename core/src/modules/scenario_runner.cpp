#include "ksp/modules/scenario_runner.hpp"

#include <algorithm>
#include <map>

#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/save/game_persistence.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp {

// ---- ScenarioModuleFactory ----------------------------------------------------------------------------------------------

namespace {
std::map<std::string, ScenarioModuleFactory::Creator, std::less<>>& Creators() {
    static std::map<std::string, ScenarioModuleFactory::Creator, std::less<>> m;
    return m;
}
}  // namespace

void ScenarioModuleFactory::Register(const std::string& className, Creator creator) { Creators()[className] = std::move(creator); }

std::unique_ptr<ScenarioModule> ScenarioModuleFactory::Create(std::string_view className) {
    auto it = Creators().find(className);
    if (it != Creators().end()) return it->second(std::string(className));
    if (!GetScenarioModuleClass(className)) return nullptr;  // AssemblyLoader.GetClassByName found no type
    return std::make_unique<ScenarioModule>(std::string(className));
}

// ---- ScenarioRunner -----------------------------------------------------------------------------------------------------

ScenarioRunner* ScenarioRunner::instance_ = nullptr;

// ScenarioRunner.cs:20 Awake (a second runner would be Destroy'ed: here it simply does not become the Instance)
ScenarioRunner::ScenarioRunner() {
    loadedScene = [] { return SaveEnvironment::Current().loadedScene; };
    if (instance_ != nullptr && instance_ != this) return;
    instance_ = this;
    GameEvents::onGameSceneLoadRequested.Add(this, &ScenarioRunner::OnGameSceneLoadRequested);
    GameEvents::onLevelWasLoaded.Add(this, &ScenarioRunner::onLevelWasLoaded);
    subscribed_ = true;
    // Game.Updated() reads ScenarioRunner.GetUpdatedProtoModules() (Game.cs:~) through the environment hook
    SaveEnvironment& env = SaveEnvironment::Current();
    if (!env.getUpdatedProtoModules) {
        env.getUpdatedProtoModules = [] { return ScenarioRunner::GetUpdatedProtoModules(); };
        hookInstalled_ = true;
    }
}

// ScenarioRunner.cs:32 OnDestroy
ScenarioRunner::~ScenarioRunner() {
    if (subscribed_) {
        GameEvents::onGameSceneLoadRequested.Remove(this, &ScenarioRunner::OnGameSceneLoadRequested);
        GameEvents::onLevelWasLoaded.Remove(this, &ScenarioRunner::onLevelWasLoaded);
    }
    if (hookInstalled_) SaveEnvironment::Current().getUpdatedProtoModules = nullptr;
    if (instance_ == this) instance_ = nullptr;
}

// ScenarioRunner.cs:39
void ScenarioRunner::OnGameSceneLoadRequested(GameScenes) {
    for (auto& m : modules_)
        if (m) m->runner = nullptr;  // DestroyImmediate
    modules_.clear();
}

// ScenarioRunner.cs:54
void ScenarioRunner::onLevelWasLoaded(GameScenes scene) {
    if (scene == GameScenes::MAINMENU) AddMainMenuScenarios();
}

// ScenarioRunner.cs:64
void ScenarioRunner::AddMainMenuScenarios() {
    std::vector<std::shared_ptr<ProtoScenarioModule>> list;
    for (const KSPScenarioType& t : KSPScenarioType::GetAllScenarioTypesInAssemblies()) {
        if (std::find(t.targetScenes.begin(), t.targetScenes.end(), GameScenes::MAINMENU) == t.targetScenes.end()) continue;
        ConfigNode node("SCENARIO");
        node.AddValue("name", t.moduleTypeName);
        std::string text;
        for (size_t j = 0; j < t.targetScenes.size(); j++) {
            text += std::to_string(static_cast<int>(t.targetScenes[j]));
            if (j < t.targetScenes.size() - 1) text += ", ";
        }
        node.AddValue("scene", text);
        list.push_back(std::make_shared<ProtoScenarioModule>(node));
    }
    LoadModules(std::move(list));
}

// ScenarioRunner.cs:122
ScenarioModule* ScenarioRunner::AddModule(std::string_view moduleName) {
    std::unique_ptr<ScenarioModule> m = ScenarioModuleFactory::Create(moduleName);
    if (!m) {
        log::Error("Cannot find a Module of typename '" + std::string(moduleName) + "'");
        return nullptr;
    }
    ScenarioRunner* host = instance_ ? instance_ : this;
    m->runner = host;
    m->Awake();
    host->modules_.push_back(std::move(m));
    return host->modules_.back().get();
}

// ScenarioRunner.cs:140
ScenarioModule* ScenarioRunner::AddModule(const ConfigNode& node) {
    if (!node.HasValue("name")) {
        log::Error("Cannot add a Module because ConfigNode contains no module name");
        return nullptr;
    }
    const std::string name = *node.GetValue("name");
    ScenarioModule* m = AddModule(name);
    if (!m) return nullptr;
    try {
        ConfigNode copy;
        node.CopyTo(copy);
        m->Load(copy);
    } catch (const std::exception& e) {
        log::Error("Exception loading ScenarioModule " + name + ": " + e.what());
    }
    return m;
}

// ProtoScenarioModule.cs:66
ScenarioModule* ProtoScenarioModule::Load(ScenarioRunner& host) {
    ScenarioModule* m = host.AddModule(*moduleValues_);
    if (!m) {
        log::Error("ScenarioModule is null.");
        return nullptr;
    }
    m->snapshot = this;
    m->targetScenes = targetScenes;
    moduleRef = m;
    return m;
}

ScenarioModule* ScenarioRunner::LoadProto(ProtoScenarioModule& proto) { return proto.Load(*this); }

// ScenarioRunner.cs:98 / :104 (MissionSystem first, then the rest)
void ScenarioRunner::LoadModules(std::vector<std::shared_ptr<ProtoScenarioModule>> protoModules) {
    protoModules_ = std::move(protoModules);
    const GameScenes scene = loadedScene();
    auto inScene = [&](const ProtoScenarioModule& p) {
        return std::find(p.targetScenes.begin(), p.targetScenes.end(), scene) != p.targetScenes.end();
    };
    for (size_t i = 0; i < protoModules_.size(); i++) {
        auto& p = protoModules_[i];
        if (p->moduleName == "MissionSystem" && inScene(*p)) p->Load(*this);
    }
    for (size_t i = 0; i < protoModules_.size(); i++) {
        auto& p = protoModules_[i];
        if (p->moduleName != "MissionSystem" && inScene(*p)) p->Load(*this);
    }
}

void ScenarioRunner::ClearModules() {
    modules_.clear();
}

// ScenarioRunner.cs:158
void ScenarioRunner::UpdateModules() {
    for (auto& p : protoModules_)
        if (p->moduleRef != nullptr) p = std::make_shared<ProtoScenarioModule>(*p->moduleRef);
}

std::vector<std::unique_ptr<ScenarioModule>>& ScenarioRunner::GetLoadedModules() { return instance_->modules_; }

std::vector<std::shared_ptr<ProtoScenarioModule>>& ScenarioRunner::GetUpdatedProtoModules() {
    instance_->UpdateModules();
    return instance_->protoModules_;
}

void ScenarioRunner::SetProtoModules(std::vector<std::shared_ptr<ProtoScenarioModule>> protoModules) {
    instance_->LoadModules(std::move(protoModules));
}

void ScenarioRunner::SetProtoModules(std::shared_ptr<ProtoScenarioModule> protoModule) {
    std::vector<std::shared_ptr<ProtoScenarioModule>> one;
    one.push_back(std::move(protoModule));
    instance_->LoadModules(std::move(one));
}

// ScenarioRunner.cs:213
void ScenarioRunner::RemoveModule(ScenarioModule* module) {
    if (module->runner != instance_) {
        log::Error("Cannot destroy a module which does not belong to this runner");
        return;
    }
    auto& mods = instance_->modules_;
    auto it = std::find_if(mods.begin(), mods.end(), [&](const auto& m) { return m.get() == module; });
    if (it != mods.end()) mods.erase(it);
}

}  // namespace ksp
