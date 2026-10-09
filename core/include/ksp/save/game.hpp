// The save game (persistent.sfs / quicksave .sfs) data model: Game (Game.cs), FlightState (FlightState.cs),
// ProtoScenarioModule (ProtoScenarioModule.cs), the data side of ScenarioModule (ScenarioModule.cs) and GameBackup
// (GameBackup.cs). Load = the C# constructors over a ConfigNode, Save = the C# Save methods, field names, order and
// number formatting as KSP writes them (ConfigNode.AddValue: double "G17", float "G9", bool "True"/"False").
//
// Values KSP rewrites on every save, independent of what was loaded (STATUS.md WP-04 lists them all):
//  * GAME: "modded" = (modded || GameDatabase.Modded), "envInfo" = GameDatabase.EnvironmentInfo of the running game,
//    LoaderInfo = the loaded entries re-flagged by GameDatabase.loaderInfo (+ its new entries), ROCSeed and RemovedROCs
//    only with Breaking Ground (Serenity) installed, additional systems (MESSAGESYSTEM, mods' nodes) come from
//    GameEvents.onGameStateSave listeners, not from the loaded file, Description loses its line breaks ('^' is read as
//    '\n' and AddValue's CleanupInput strips '\n'), the version stamp is only renewed by Updated().
//  * FLIGHTSTATE: commNetUIModeTracking / commNetUIModeFlight are the CommNetUI statics (set by loading).
//  * Game(ConfigNode) mutates the tree it reads: FlightState removes EVA vessels without crew
//    (CompatibilityUtilities.CleanUpUnsanitaryEVAKerbals) and rewrites activeVessel, FlightLog.Load removes "flight".
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/util/collections.hpp"

namespace ksp {

class Game;
class ProtoScenarioModule;

class FieldStore;
class BaseFieldList;
class BaseEventList;
class ScenarioRunner;

// ScenarioModule.cs. WP-04 holds the data side (Save/Load/targetScenes); WP-06 adds the MonoBehaviour host: Awake builds the
// BaseFieldList / BaseEventList of the module class from the generated host schema (host_schema.inc), so a module that does
// not override SaveFields / LoadFields saves and loads its [KSPField]s exactly as KSP does (Fields.Save / Fields.Load). A
// subclass with C++ members for its fields overrides SaveFields/LoadFields (or reads the typed store: Store()).
class ScenarioModule {
public:
    explicit ScenarioModule(std::string className);  // the C# constructor: className = GetType().Name
    virtual ~ScenarioModule();
    ScenarioModule(const ScenarioModule&) = delete;
    ScenarioModule& operator=(const ScenarioModule&) = delete;
    ProtoScenarioModule* snapshot = nullptr;
    ScenarioRunner* runner = nullptr;
    std::vector<GameScenes> targetScenes;
    const std::string& ClassName() const { return className_; }
    int ClassID() const { return classID_; }

    // ScenarioModule.cs:41 Awake: targetScenes = new List; ModularSetup (events, fields); OnAwake()
    void Awake();
    // null until Awake / for a class outside the generated schema
    FieldStore* Store() { return store_.get(); }
    BaseFieldList* Fields() { return fields_.get(); }
    BaseEventList* Events() { return events_.get(); }

    // ScenarioModule.cs:55 (name, "scene" = (int)scenes joined with ", ", Fields.Save, OnSave)
    void Save(ConfigNode& node);
    // ScenarioModule.cs:72
    void Load(ConfigNode& node);
    virtual void OnAwake() {}
    virtual void OnSave(ConfigNode& node) { (void)node; }
    virtual void OnLoad(ConfigNode& node) { (void)node; }
    virtual void SaveFields(ConfigNode& node);  // BaseFieldList.Save (default: the generic field list)
    virtual void LoadFields(ConfigNode& node);  // BaseFieldList.Load

private:
    std::string className_;
    int classID_ = 0;
    std::unique_ptr<FieldStore> store_;
    std::unique_ptr<BaseFieldList> fields_;
    std::unique_ptr<BaseEventList> events_;
};

// ProtoScenarioModule.cs
class ProtoScenarioModule {
public:
    std::optional<std::string> moduleName;
    std::vector<GameScenes> targetScenes;
    ScenarioModule* moduleRef = nullptr;

    // ProtoScenarioModule.cs:13 (snapshot of a live module: module.Save into a SCENARIO node)
    explicit ProtoScenarioModule(ScenarioModule& module);
    // ProtoScenarioModule.cs:23 ("scene" = comma-separated ints, int.Parse after Trim)
    explicit ProtoScenarioModule(const ConfigNode& node);
    // ProtoScenarioModule.cs:51
    void SetTargetScenes(const std::vector<GameScenes>& scenes);
    // ProtoScenarioModule.cs:73
    void Save(ConfigNode& node) const;
    // ProtoScenarioModule.cs:66 (WP-06: defined with the runner, modules/scenario_runner.cpp)
    ScenarioModule* Load(ScenarioRunner& host);
    // ProtoScenarioModule.cs:91
    ConfigNode& GetData() { return *moduleValues_; }
    const ConfigNode& GetData() const { return *moduleValues_; }

private:
    ConfigNode::NodePtr moduleValues_;
};

// FlightState.cs
class FlightState {
public:
    static constexpr int lastCompatibleMajor = 0;
    static constexpr int lastCompatibleMinor = 18;
    static constexpr int lastCompatibleRev = 0;

    bool compatible = false;
    std::vector<std::unique_ptr<ProtoVessel>> protoVessels;
    KSPParseableDictionary sceneStateValues;
    int file_version_major = 0;
    int file_version_minor = 0;
    int file_version_revision = 0;
    double universalTime = 0.0;
    int activeVesselIdx = 0;
    int mapViewFilterState = 0;

    // FlightState.cs:33: the state of the running game. Live vessels (FlightGlobals.Vessels -> BackupVessel) are
    // WP-14; kspcore captures none, UT = Planetarium.GetUniversalTime() (0 without a Planetarium / current game).
    FlightState();
    // FlightState.cs:92 (rootNode is modified: CleanUpUnsanitaryEVAKerbals)
    FlightState(ConfigNode& rootNode, Game* game);
    FlightState(const FlightState&) = delete;
    FlightState& operator=(const FlightState&) = delete;
    // FlightState.cs:153
    void Save(ConfigNode& rootNode);
    // FlightState.cs:182
    bool ContainsFlightID(uint32_t id) const;
};

// FinePrint/Utilities/CompatibilityUtilities.cs:588 (int.Parse(activeVessel): ArgumentNullException when missing;
// the index fix-up compares the wrong way round -- sic)
void CompatibilityUtilities_CleanUpUnsanitaryEVAKerbals(ConfigNode& flightState);

// Game.cs
class Game {
public:
    using Modes = Game_Modes;
    using GameStatus = Game_GameStatus;
    static constexpr int lastCompatibleMajor = 0;
    static constexpr int lastCompatibleMinor = 21;
    static constexpr int lastCompatibleRev = 0;

    std::optional<std::string> Title;
    std::optional<std::string> Description;
    std::string linkURL;
    std::string linkCaption;
    bool modded = false;
    dotnet::Dictionary<std::string, bool> loaderInfo;
    DictionaryValueList<std::string, int> cometNames;
    Modes Mode = Modes::SANDBOX;
    GameStatus Status = GameStatus::UNSTARTED;
    int Seed = -1;
    int ROCSeed = -1;
    GameScenes startScene = GameScenes::LOADING;
    EditorFacility editorFacility = EditorFacility::None;
    std::shared_ptr<FlightState> flightState;
    std::shared_ptr<GameParameters> Parameters;
    std::shared_ptr<KerbalRoster> CrewRoster;
    // MISSIONTOSTART (Expansions.Missions.Mission.Load/Save, MakingHistory): kept as the loaded node and written back
    // unchanged (the mission system is not ported)
    ConfigNode::NodePtr missionToStart;
    std::vector<std::shared_ptr<ProtoScenarioModule>> scenarios;
    bool scenariosIsNull = true;  // C# scenarios == null (Game(ConfigNode) returning early)
    ConfigNode::NodePtr additionalSystems;
    ConfigNode::NodePtr config;
    bool compatible = false;
    int file_version_major = 0;
    int file_version_minor = 0;
    int file_version_revision = 0;
    std::string versionFull = "";
    std::string versionCreated = "unknown";
    std::optional<std::string> flagURL;
    uint32_t launchID = 1;
    std::string defaultVABLaunchSite = "LaunchPad";
    std::string defaultSPHLaunchSite = "Runway";

    // Game.cs:146 (a new game: current version, random seeds, sandbox defaults)
    Game();
    // Game.cs:181 (root = the file's root node; nullptr is allowed. The tree is modified, see the file comment)
    explicit Game(ConfigNode* root);
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    bool IsMissionMode() const { return Mode == Modes::MISSION || Mode == Modes::MISSION_BUILDER; }  // Game.cs:110
    double UniversalTime() const { return flightState ? flightState->universalTime : 0.0; }        // Game.cs:122

    // Game.cs:372 / 381 (HighLogic statics from SaveEnvironment; live flight state / scenario modules through
    // SaveEnvironment hooks)
    Game& Updated();
    Game& Updated(GameScenes startSceneOverride);
    // Game.cs:419
    void Save(ConfigNode& rootNode);
    // Game.cs:572
    bool IsResumable() const;
    // Game.cs:600 (Save into a fresh root, construct from it)
    static std::unique_ptr<Game> GetCloneOf(Game& g);
    // Game.cs:607 / 612 (typeName = the ScenarioModule subclass name; nullptr = unchanged / skipped)
    ProtoScenarioModule* AddProtoScenarioModule(const std::string& typeName, const std::vector<GameScenes>& scenes);
    static std::shared_ptr<ProtoScenarioModule> AddProtoScenarioModule(
        std::vector<std::shared_ptr<ProtoScenarioModule>>& scenarioList, bool returnNullIfUnchanged,
        bool addScenarioToScenarioListIfNew, const std::string& typeName, const std::vector<GameScenes>& scenes);
    // Game.cs:666
    bool RemoveProtoScenarioModule(const std::string& typeName);
    // Game.cs:752 (ToString: Save into a root and print it)
    std::string ToString();
    // Game.cs:759
    std::string GetCometNumberedName(const std::string& name);
    // Game.cs:784
    void LoadCometNames(const ConfigNode& node);

    // GameDatabase.cs:1044 / 1066 / 1075 (the loader-info half of GameDatabase the save model uses)
    static void UpdateLoaderInfo(dotnet::Dictionary<std::string, bool>& dest);
    static void SaveLoaderInfo(ConfigNode& node, dotnet::Dictionary<std::string, bool>& dest);
    static void LoadLoaderInfo(const ConfigNode& node, dotnet::Dictionary<std::string, bool>& dest);

private:
    void PopulateGameSeed();     // Game.cs:738
    void PopulateROCGameSeed();  // Game.cs:747
    void SaveCometNames(ConfigNode& node) const;  // Game.cs:772
};

// GameBackup.cs
class GameBackup {
public:
    explicit GameBackup(Game& game);
    double UniversalTime = 0.0;
    ConfigNode::NodePtr Config;
    int ActiveVessel = 0;
    Guid ActiveVesselID;
};

}  // namespace ksp
