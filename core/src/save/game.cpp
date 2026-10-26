// Game, FlightState, ProtoScenarioModule, ScenarioModule (data side), GameBackup; see include/ksp/save/game.hpp.
#include "ksp/save/game.hpp"

#include <algorithm>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/modules/base_field.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/ksp_util.hpp"
#include "save_util.hpp"

namespace ksp {

using save_detail::AddNullableValue;
using save_detail::EnumName;
using save_detail::ParseBool;
using save_detail::ParseDouble;
using save_detail::ParseEnum;
using save_detail::ParseInt;
using save_detail::ParseUInt;
using GE = events::GameEvents;

namespace {
// VersionCompareResult.ToString()
const char* VersionCompareResultName(VersionCompareResult r) {
    switch (r) {
    case VersionCompareResult::INCOMPATIBLE_TOO_EARLY: return "INCOMPATIBLE_TOO_EARLY";
    case VersionCompareResult::COMPATIBLE: return "COMPATIBLE";
    case VersionCompareResult::INCOMPATIBLE_TOO_LATE: return "INCOMPATIBLE_TOO_LATE";
    default: return "INVALID";
    }
}
// "(int)scene, (int)scene, ..." (ScenarioModule.cs:58 / Game.cs:655 / ProtoScenarioModule.cs:54)
std::string ScenesText(const std::vector<GameScenes>& scenes) {
    std::string text;
    for (size_t i = 0; i < scenes.size(); ++i) {
        text += dotnet::FormatInt64(static_cast<int>(scenes[i]));
        if (i < scenes.size() - 1) text += ", ";
    }
    return text;
}
}  // namespace

// ================================================= ScenarioModule ==================================================

ScenarioModule::ScenarioModule(std::string className) : className_(std::move(className)) {
    classID_ = MonoStringHash(className_);
}
ScenarioModule::~ScenarioModule() = default;

// ScenarioModule.cs:41 Awake + :35 ModularSetup (className/classID are set by the constructor here)
void ScenarioModule::Awake() {
    targetScenes.clear();
    if (const ModuleClassInfo* cls = GetScenarioModuleClass(className_)) {
        events_ = std::make_unique<BaseEventList>(*cls);
        store_ = std::make_unique<FieldStore>(*cls);
        fields_ = std::make_unique<BaseFieldList>(*store_);
    }
    OnAwake();
}

void ScenarioModule::SaveFields(ConfigNode& node) {
    if (fields_) fields_->Save(node);
}
void ScenarioModule::LoadFields(ConfigNode& node) {
    if (fields_) fields_->Load(node);
}

// ScenarioModule.cs:55
void ScenarioModule::Save(ConfigNode& node) {
    node.AddValue("name", ClassName());
    node.AddValue("scene", ScenesText(targetScenes));
    SaveFields(node);
    OnSave(node);
}

// ScenarioModule.cs:72
void ScenarioModule::Load(ConfigNode& node) {
    LoadFields(node);
    OnLoad(node);
}

// =============================================== ProtoScenarioModule ===============================================

// ProtoScenarioModule.cs:13
ProtoScenarioModule::ProtoScenarioModule(ScenarioModule& module) {
    moduleRef = &module;
    moduleName = module.ClassName();
    targetScenes = module.targetScenes;
    moduleValues_ = std::make_shared<ConfigNode>("SCENARIO");
    module.Save(*moduleValues_);
    module.snapshot = this;
}

// ProtoScenarioModule.cs:23
ProtoScenarioModule::ProtoScenarioModule(const ConfigNode& node) {
    moduleValues_ = std::make_shared<ConfigNode>("SCENARIO");
    node.CopyTo(*moduleValues_);
    for (const ConfigNode::Value& value : node.values()) {
        if (value.name == "name") {
            moduleName = value.value;
        } else if (value.name == "scene") {
            for (const std::string& s : dotnet::Split(value.value, ",", false))
                targetScenes.push_back(static_cast<GameScenes>(ParseInt(dotnet::Trim(s))));
        }
    }
}

// ProtoScenarioModule.cs:51
void ProtoScenarioModule::SetTargetScenes(const std::vector<GameScenes>& scenes) {
    targetScenes = scenes;
    const std::string text = ScenesText(scenes);
    if (moduleValues_->HasValue("scene")) moduleValues_->SetValue("scene", text);
    else moduleValues_->AddValue("scene", text);
}

// ProtoScenarioModule.cs:73
void ProtoScenarioModule::Save(ConfigNode& node) const { moduleValues_->CopyTo(node); }

// =================================================== FlightState ===================================================

// FlightState.cs:33 FlightState() and :164 Load: the live half, src/vessel/flight_state_live.cpp (WP-19c)

// FinePrint/Utilities/CompatibilityUtilities.cs:588
void CompatibilityUtilities_CleanUpUnsanitaryEVAKerbals(ConfigNode& flightState) {
    const std::string* av = flightState.GetValuePtr("activeVessel");
    if (av == nullptr) throw ArgumentNullException("String reference not set to an instance of a String.");
    int num = ParseInt(*av);
    const std::vector<ConfigNode*> nodes = flightState.GetNodes("VESSEL");
    int num2 = static_cast<int>(nodes.size());
    while (num2-- > 0) {
        ConfigNode* configNode = nodes[static_cast<size_t>(num2)];
        const std::string* type = configNode->GetValuePtr("type");
        if (type == nullptr || *type != "EVA") continue;
        for (ConfigNode* part : configNode->GetNodes("PART")) {
            const std::string* name = part->GetValuePtr("name");
            if (name != nullptr && *name == "kerbalEVA" && part->GetValuePtr("crew") == nullptr) {
                flightState.RemoveNode(configNode);
                if (num2 > num) --num;
                break;
            }
        }
    }
    flightState.SetValue("activeVessel", dotnet::FormatInt64(num));
}

// FlightState.cs:92
FlightState::FlightState(ConfigNode& rootNode, Game* game) {
    CompatibilityUtilities_CleanUpUnsanitaryEVAKerbals(rootNode);
    SaveEnvironment& env = SaveEnvironment::Current();
    for (int num = 0; num < rootNode.values().Count(); ++num) {
        const ConfigNode::Value& value = rootNode.values()[num];
        const std::string& n = value.name;
        if (n == "commNetUIModeFlight") env.commNetUIModeFlightMap = ParseEnum<CommNetUI_DisplayMode>(value.value);
        else if (n == "commNetUIModeTracking")
            env.commNetUIModeTrackingStation = ParseEnum<CommNetUI_DisplayMode>(value.value);
        else if (n == "mapViewFiltering") mapViewFilterState = ParseInt(value.value);
        else if (n == "activeVessel") activeVesselIdx = ParseInt(value.value);
        else if (n == "UT") universalTime = ParseDouble(value.value);
        else if (n == "version") {
            const std::vector<std::string> array = dotnet::Split(value.value, ".", false);
            file_version_major = ParseInt(array.at(0));
            file_version_minor = ParseInt(array.at(1));
            file_version_revision = ParseInt(array.at(2));
            const VersionCompareResult r = KSPUtil::CheckVersion(value.value, 0, 18, 0);
            compatible = r == VersionCompareResult::COMPATIBLE;
            if (!compatible) return;
        }
    }
    for (int i = 0; i < rootNode.nodes().Count(); ++i) {
        const ConfigNode& child = *rootNode.nodes()[i];
        if (child.name == "VESSEL") protoVessels.push_back(std::make_unique<ProtoVessel>(child, game));
    }
}

// FlightState.cs:153
void FlightState::Save(ConfigNode& rootNode) {
    const SaveEnvironment& env = SaveEnvironment::Current();
    rootNode.AddValue("version", dotnet::FormatInt64(file_version_major) + "." + dotnet::FormatInt64(file_version_minor) +
                                     "." + dotnet::FormatInt64(file_version_revision));
    rootNode.AddValue("UT", universalTime);
    rootNode.AddValue("activeVessel", static_cast<int32_t>(activeVesselIdx));
    rootNode.AddValue("mapViewFiltering", static_cast<int32_t>(mapViewFilterState));
    rootNode.AddValue("commNetUIModeTracking", EnumName(env.commNetUIModeTrackingStation));
    rootNode.AddValue("commNetUIModeFlight", EnumName(env.commNetUIModeFlightMap));
    for (auto& pv : protoVessels) pv->Save(*rootNode.AddNode("VESSEL"));
}

// FlightState.cs:182
bool FlightState::ContainsFlightID(uint32_t id) const {
    for (const auto& pv : protoVessels)
        for (const auto& pps : pv->protoPartSnapshots)
            if (pps->flightID == id) return true;
    return false;
}

// ======================================================= Game ======================================================

// Game.cs:146
Game::Game() {
    SaveEnvironment& env = SaveEnvironment::Current();
    Title = "";
    Description = "";
    linkURL = "";
    linkCaption = "";
    modded = env.gameDatabaseModded;
    UpdateLoaderInfo(loaderInfo);
    flagURL = "";
    Mode = Modes::SANDBOX;
    Status = GameStatus::ONGOING;
    PopulateGameSeed();
    if (env.IsExpansionInstalled("Serenity")) PopulateROCGameSeed();
    startScene = GameScenes::SPACECENTER;
    editorFacility = EditorFacility::None;
    Parameters = std::make_shared<GameParameters>();
    scenarios.clear();
    scenariosIsNull = false;
    CrewRoster = std::make_shared<KerbalRoster>(Modes::SANDBOX);
    file_version_major = Versioning::version_major;
    file_version_minor = Versioning::version_minor;
    file_version_revision = Versioning::Revision;
    compatible = true;
    versionCreated = versionFull = Versioning_GetVersionStringFull();
    versionFull = Versioning_GetVersionStringFull();
    log::Info("Game State Created.");
    GE::onGameStateCreated.Fire(this);
}

// Game.cs:181
Game::Game(ConfigNode* root) {
    SaveEnvironment& env = SaveEnvironment::Current();
    if (root != nullptr && root->HasNode("GAME")) {
        linkCaption = "";
        linkURL = "";
        ConfigNode::NodePtr nodePtr = save_detail::FirstChildShared(*root, "GAME");
        ConfigNode& node = *nodePtr;
        for (int i = 0; i < node.values().Count(); ++i) {
            const ConfigNode::Value& value = node.values()[i];
            const std::string& n = value.name;
            const std::string& v = value.value;
            if (n == "launchID") launchID = ParseUInt(v);
            else if (n == "Status") Status = static_cast<GameStatus>(ParseInt(v));
            else if (n == "scene") startScene = static_cast<GameScenes>(ParseInt(v));
            else if (n == "ROCSeed") ROCSeed = env.IsExpansionInstalled("Serenity") ? ParseInt(v) : -1;
            else if (n == "version") {
                const std::vector<std::string> array = dotnet::Split(v, ".", false);
                file_version_major = ParseInt(array.at(0));
                file_version_minor = ParseInt(array.at(1));
                file_version_revision = ParseInt(array.at(2));
                const VersionCompareResult r = KSPUtil::CheckVersion(v, 0, 21, 0);
                compatible = r == VersionCompareResult::COMPATIBLE;
                if (!compatible) {
                    log::Info("Save compatibility check failed: " + v + " vs 0.21.0: " + VersionCompareResultName(r));
                    return;
                }
            } else if (n == "Title") Title = v;
            else if (n == "modded") {
                bool result2 = false;
                if (dotnet::TryParseBool(v, result2)) modded |= result2;
            } else if (n == "editor") editorFacility = ParseEnum<EditorFacility>(v);
            else if (n == "Mode") {
                int32_t result = 0;
                if (dotnet::TryParseInt32(v, result)) {
                    switch (result) {
                    case 1: Mode = Modes::CAREER; break;
                    case 2: Mode = Modes::SCENARIO; break;
                    case 3: Mode = Modes::SCENARIO_NON_RESUMABLE; break;
                    default: Mode = Modes::SANDBOX; break;
                    }
                } else {
                    Mode = ParseEnum<Modes>(v);
                }
            } else if (n == "versionCreated") versionCreated = v;
            else if (n == "Description") Description = dotnet::Replace(v, "^", "\n");
            else if (n == "flag") flagURL = v;
            else if (n == "linkURL") linkURL = v;
            else if (n == "defaultSPHLaunchSite") defaultSPHLaunchSite = v;
            else if (n == "Seed") Seed = ParseInt(v);
            else if (n == "versionFull") versionFull = v;
            else if (n == "linkCaption") linkCaption = v;
            else if (n == "defaultVABLaunchSite") defaultVABLaunchSite = v;
        }
        if (Mode != Modes::SANDBOX && Mode != Modes::SCIENCE_SANDBOX) {
            defaultVABLaunchSite = "LaunchPad";
            defaultSPHLaunchSite = "Runway";
        }
        PopulateGameSeed();
        if (!node.HasNode("ROSTER")) {
            log::Error("[GamePersistence Error]: No ROSTER node found in save state: " + Title.value_or(""));
            return;
        }
        CrewRoster = std::make_shared<KerbalRoster>(*node.GetNode("ROSTER"), Mode);
        scenarios.clear();
        scenariosIsNull = false;
        for (int i = 0; i < node.nodes().Count(); ++i) {
            const ConfigNode::NodePtr& node2 = node.nodes().Shared(i);
            const std::string& n = node2->name;
            if (n == "SCENARIO") {
                auto psm = std::make_shared<ProtoScenarioModule>(*node2);
                if (psm->moduleName == "DeployedScience") {
                    if (env.IsExpansionInstalled("Serenity")) scenarios.push_back(psm);
                    else
                        log::Warning("[GamePersistence]: Game contains DeployedScience Data but Breaking Ground DLC is "
                                     "not installed, skipping.");
                } else {
                    scenarios.push_back(psm);
                }
            } else if (n == "CometNames") {
                LoadCometNames(*node2);
            } else if (n == "LoaderInfo") {
                loaderInfo.Clear();
                LoadLoaderInfo(*node2, loaderInfo);
            } else if (n == "PARAMETERS") {
                Parameters = std::make_shared<GameParameters>(*node2);
            } else if (n == "FLIGHTSTATE") {
                flightState = std::make_shared<FlightState>(*node2, this);
            } else if (n == "RemovedROCs") {
                if (env.IsExpansionInstalled("Serenity") && env.rocManager != nullptr) env.rocManager->LoadRemovedROCs(*node2);
            } else if (n == "MISSIONTOSTART") {
                missionToStart = node2->CreateCopy();
            }
        }
        if (!Parameters) Parameters = std::make_shared<GameParameters>();
        config = nodePtr;
    } else {
        Title = "";
        Mode = Modes::SANDBOX;
        Status = GameStatus::ONGOING;
        PopulateGameSeed();
        startScene = GameScenes::SPACECENTER;
        editorFacility = EditorFacility::None;
        flagURL = "";
        linkCaption = "";
        linkURL = "";
        defaultVABLaunchSite = "LaunchPad";
        defaultSPHLaunchSite = "Runway";
        modded = env.gameDatabaseModded;
        file_version_major = Versioning::version_major;
        file_version_minor = Versioning::version_minor;
        file_version_revision = Versioning::Revision;
        if (root == nullptr) {
            log::Error("[GamePersistence]: Game state root config node is null, this should never happen. Initializing "
                       "with default game and flight states.");
            Description = "Defaulted from corrupt or missing SFS file";
            flightState = std::make_shared<FlightState>();
        } else {
            log::Error("[GamePersistence]: Game state from older save file was missing. Initializing with default game "
                       "states.");
            Description = "Upgraded from old SFS file";
            flightState = std::make_shared<FlightState>(*root, this);
        }
        compatible = flightState->compatible;
        scenarios.clear();
        scenariosIsNull = false;
        Parameters = std::make_shared<GameParameters>();
        CrewRoster = std::make_shared<KerbalRoster>(Mode);
    }
    GE::onGameStateCreated.Fire(this);
}

// Game.cs:372
Game& Game::Updated() {
    const Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr) throw NullReferenceException("Game.Updated: HighLogic.CurrentGame is null");
    if (g->Mode != Modes::MISSION && g->Mode != Modes::MISSION_BUILDER) return Updated(GameScenes::SPACECENTER);
    return Updated(GameScenes::LOADING);
}

// Game.cs:381
Game& Game::Updated(GameScenes startSceneOverride) {
    SaveEnvironment& env = SaveEnvironment::Current();
    if (env.loadedSceneHasPlanetarium)
        flightState = env.captureFlightState ? env.captureFlightState() : std::make_shared<FlightState>();
    bool flag = false;
    if (env.currentGame != nullptr) {
        Parameters = env.currentGame->Parameters;
        CrewRoster = env.currentGame->CrewRoster;
        flag = env.currentGame->Mode == Modes::MISSION || env.currentGame->Mode == Modes::MISSION_BUILDER;
        if (!CrewRoster) throw NullReferenceException("Game.Updated: CrewRoster is null");
        CrewRoster->ValidateAssignments(*this);
    }
    if (env.loadedSceneIsGame && env.getUpdatedProtoModules) {
        scenarios = env.getUpdatedProtoModules();
        scenariosIsNull = false;
    }
    file_version_major = Versioning::version_major;
    file_version_minor = Versioning::version_minor;
    file_version_revision = Versioning::Revision;
    compatible = true;
    versionFull = Versioning_GetVersionStringFull();
    if (Parameters && Parameters->Flight.CanLeaveToSpaceCenter && !flag) startScene = GameScenes::SPACECENTER;
    else if (flag) startScene = startSceneOverride == GameScenes::LOADING ? env.loadedScene : startSceneOverride;
    additionalSystems = std::make_shared<ConfigNode>();
    GE::onGameStateSave.Fire(additionalSystems.get());
    return *this;
}

// Game.cs:419
void Game::Save(ConfigNode& rootNode) {
    SaveEnvironment& env = SaveEnvironment::Current();
    ConfigNode& configNode = *rootNode.AddNode("GAME");
    configNode.AddValue("version", dotnet::FormatInt64(file_version_major) + "." + dotnet::FormatInt64(file_version_minor) +
                                       "." + dotnet::FormatInt64(file_version_revision));
    AddNullableValue(configNode, "Title", Title);
    AddNullableValue(configNode, "Description", Description);
    configNode.AddValue("linkURL", linkURL);
    configNode.AddValue("linkCaption", linkCaption);
    configNode.AddValue("Mode", EnumName(Mode));
    configNode.AddValue("Status", static_cast<int32_t>(Status));
    configNode.AddValue("Seed", static_cast<int32_t>(Seed));
    if (env.IsExpansionInstalled("Serenity")) configNode.AddValue("ROCSeed", static_cast<int32_t>(ROCSeed));
    if (env.IsExpansionInstalled("Serenity") && env.rocManager != nullptr)
        env.rocManager->SaveRemoveROCs(*configNode.AddNode("RemovedROCs"));
    SaveCometNames(*configNode.AddNode("CometNames"));
    configNode.AddValue("scene", static_cast<int32_t>(startScene));
    configNode.AddValue("editor", EnumName(editorFacility));
    AddNullableValue(configNode, "flag", flagURL);
    configNode.AddValue("launchID", launchID);
    configNode.AddValue("defaultVABLaunchSite", defaultVABLaunchSite);
    configNode.AddValue("defaultSPHLaunchSite", defaultSPHLaunchSite);
    if (!Parameters) throw NullReferenceException("Game.Save: Parameters is null");
    Parameters->Save(*configNode.AddNode("PARAMETERS"));
    if (scenariosIsNull) throw NullReferenceException("Game.Save: scenarios is null");
    for (const auto& scenario : scenarios) scenario->Save(*configNode.AddNode("SCENARIO"));
    if (flightState) flightState->Save(*configNode.AddNode("FLIGHTSTATE"));
    if (missionToStart) missionToStart->CopyTo(*configNode.AddNode("MISSIONTOSTART"));
    configNode.AddValue("modded", dotnet::FormatBool(modded || env.gameDatabaseModded));
    configNode.AddValue("envInfo", env.gameDatabaseEnvironmentInfo);
    configNode.AddValue("versionFull", versionFull);
    configNode.AddValue("versionCreated", versionCreated);
    SaveLoaderInfo(*configNode.AddNode("LoaderInfo"), loaderInfo);
    if (!CrewRoster) throw NullReferenceException("Game.Save: CrewRoster is null");
    CrewRoster->Save(*configNode.AddNode("ROSTER"));
    if (!additionalSystems) {
        additionalSystems = std::make_shared<ConfigNode>();
        GE::onGameStateSave.Fire(additionalSystems.get());
    }
    configNode.AddData(*additionalSystems);
    GE::onGameStateSaved.Fire(this);
}

// Game.cs:572
bool Game::IsResumable() const {
    if (Mode == Modes::SCENARIO_NON_RESUMABLE) {
        log::Info("Scenario " + Title.value_or("") + " is non-resumable.");
        return false;
    }
    if (Status == GameStatus::FAILED_OR_ABORTED) {
        log::Info("Scenario " + Title.value_or("") + " is not resumable because it was failed or aborted.");
        return false;
    }
    if (Status == GameStatus::COMPLETED) {
        log::Info("Scenario " + Title.value_or("") + " is not resumable because it was already completed.");
        return false;
    }
    if (startScene == GameScenes::FLIGHT) {
        if (!flightState) {
            log::Info("Scenario " + Title.value_or("") +
                      " cannot be resumed because it has a null flight state and starts at the flight scene.");
            return false;
        }
        if (flightState->protoVessels.empty()) {
            log::Info("Scenario " + Title.value_or("") +
                      " cannot be resumed because it starts at the flight scene, but there are no live vessels.");
            return false;
        }
    }
    return true;
}

// Game.cs:600
std::unique_ptr<Game> Game::GetCloneOf(Game& g) {
    ConfigNode configNode;
    g.Save(configNode);
    return std::make_unique<Game>(&configNode);
}

// Game.cs:607
ProtoScenarioModule* Game::AddProtoScenarioModule(const std::string& typeName, const std::vector<GameScenes>& scenes) {
    return AddProtoScenarioModule(scenarios, true, true, typeName, scenes).get();
}

// Game.cs:612
std::shared_ptr<ProtoScenarioModule> Game::AddProtoScenarioModule(
    std::vector<std::shared_ptr<ProtoScenarioModule>>& scenarioList, bool returnNullIfUnchanged,
    bool addScenarioToScenarioListIfNew, const std::string& typeName, const std::vector<GameScenes>& scenes) {
    if (typeName == "DeployedScience" && !SaveEnvironment::Current().IsExpansionInstalled("Serenity")) {
        log::Warning("[GamePersistence]: Cannot add DeployedScience Scenario, Breaking Ground DLC is not installed, "
                     "skipping.");
        return nullptr;
    }
    auto it = std::find_if(scenarioList.begin(), scenarioList.end(),
                           [&](const std::shared_ptr<ProtoScenarioModule>& s) { return s->moduleName == typeName; });
    if (it != scenarioList.end()) {
        std::shared_ptr<ProtoScenarioModule> psm = *it;
        const size_t count = psm->targetScenes.size();
        bool flag = count == scenes.size();
        if (flag) {
            size_t num = count;
            while (num-- > 0)
                if (psm->targetScenes[num] != scenes[num]) {
                    flag = false;
                    break;
                }
        }
        if (flag) return returnNullIfUnchanged ? nullptr : psm;
        log::Info("[Game]: ScenarioModule " + typeName +
                  " already exists in save, but saved target scenes do not match module definition. Updating...");
        psm->SetTargetScenes(scenes);
        return psm;
    }
    ConfigNode configNode("SCENARIO");
    configNode.AddValue("name", typeName);
    configNode.AddValue("scene", ScenesText(scenes));
    auto psm2 = std::make_shared<ProtoScenarioModule>(configNode);
    if (addScenarioToScenarioListIfNew) scenarioList.push_back(psm2);
    return psm2;
}

// Game.cs:666
bool Game::RemoveProtoScenarioModule(const std::string& typeName) {
    auto it = std::find_if(scenarios.begin(), scenarios.end(),
                           [&](const std::shared_ptr<ProtoScenarioModule>& s) { return s->moduleName == typeName; });
    if (it == scenarios.end()) return false;
    scenarios.erase(it);
    return true;
}

// Game.cs:738
void Game::PopulateGameSeed() {
    if (Seed == -1) {
        const SaveEnvironment& env = SaveEnvironment::Current();
        KSPRandom r(env.TickCount() ^ env.NewGuidHashCode());
        Seed = r.Next();
    }
}

// Game.cs:747
void Game::PopulateROCGameSeed() {
    if (ROCSeed <= -1) {
        const SaveEnvironment& env = SaveEnvironment::Current();
        KSPRandom r(env.TickCount() ^ env.NewGuidHashCode());
        ROCSeed = r.Next();
    }
}

// Game.cs:752
std::string Game::ToString() {
    ConfigNode configNode;
    Save(configNode);
    return configNode.ToString();
}

// Game.cs:745 (uses HighLogic.CurrentGame's dictionary, not this game's -- sic)
std::string Game::GetCometNumberedName(const std::string& nameArg) {
    // C# strings are values: `name` may be an element of cometNames.KeysList (DiscoverableObjectsUtil.GenerateCometName passes one), which the indexer set below
    // erases and re-appends. A reference parameter would dangle (WP-38d), so the text is copied first.
    const std::string name = nameArg;
    Game* g = SaveEnvironment::Current().currentGame;
    if (g == nullptr) throw NullReferenceException("Game.GetCometNumberedName: HighLogic.CurrentGame is null");
    if (g->cometNames.ContainsKey(name)) g->cometNames.Set(name, g->cometNames[name] + 1);
    else g->cometNames.Add(name, 1);
    return name + " " + dotnet::FormatInt64(g->cometNames[name]);
}

// Game.cs:772
void Game::SaveCometNames(ConfigNode& node) const {
    for (int i = 0; i < cometNames.Count(); ++i) {
        const std::string& key = cometNames.KeysList()[static_cast<size_t>(i)];
        node.AddValue("CometName", key + "," + dotnet::FormatInt64(cometNames[key]));
    }
}

// Game.cs:784
void Game::LoadCometNames(const ConfigNode& node) {
    cometNames.Clear();
    for (const std::string& line : node.GetValuesList("CometName")) {
        if (dotnet::Trim(line).empty()) continue;
        std::vector<std::string> array = dotnet::Split(line, ",", false);
        if (array.empty() || dotnet::Trim(array[0]).empty() || cometNames.ContainsKey(array[0])) continue;
        if (array.size() < 2) {
            array[0] = dotnet::Replace(dotnet::Replace(dotnet::Replace(array[0], "0", ""), "1", ""), "2", "");
            cometNames.Add(array[0], 1);
            continue;
        }
        int32_t v = 0;
        if (dotnet::TryParseInt32(array[1], v)) cometNames.Add(array[0], v);
        else log::Warning("Unable to restore comet name. Skipping line " + line);
    }
}

// GameDatabase.cs:1044
void Game::UpdateLoaderInfo(dotnet::Dictionary<std::string, bool>& dest) {
    const std::vector<std::string>& loaderInfo = SaveEnvironment::Current().gameDatabaseLoaderInfo;
    for (const std::string& item : dest.Keys())
        dest.Set(item, std::find(loaderInfo.begin(), loaderInfo.end(), item) != loaderInfo.end());
    for (const std::string& item2 : loaderInfo)
        if (!dest.ContainsKey(item2)) dest.Set(item2, true);
}

// GameDatabase.cs:1066
void Game::SaveLoaderInfo(ConfigNode& node, dotnet::Dictionary<std::string, bool>& dest) {
    UpdateLoaderInfo(dest);
    for (const auto& [key, value] : dest) node.AddValue(key, dotnet::FormatBool(value));
}

// GameDatabase.cs:1075
void Game::LoadLoaderInfo(const ConfigNode& node, dotnet::Dictionary<std::string, bool>& dest) {
    for (const ConfigNode::Value& v : node.values())
        if (!dest.ContainsKey(v.name)) dest.Set(v.name, false);
    UpdateLoaderInfo(dest);
}

// ===================================================== GameBackup ==================================================

// GameBackup.cs:15
GameBackup::GameBackup(Game& game) {
    UniversalTime = game.UniversalTime();
    if (!game.flightState) throw NullReferenceException("GameBackup: flightState is null");
    ActiveVessel = game.flightState->activeVesselIdx;
    if (ActiveVessel >= 0 && ActiveVessel < static_cast<int>(game.flightState->protoVessels.size())) {
        ActiveVesselID = game.flightState->protoVessels[static_cast<size_t>(ActiveVessel)]->vesselID;
    } else {
        ActiveVessel = 0;
        ActiveVesselID = Guid::Empty();
    }
    Config = std::make_shared<ConfigNode>();
    game.Save(*Config);
}

}  // namespace ksp
