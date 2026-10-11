// WP-54: MENode.cs, TestGroup.cs, TestModule.cs, ActionModule.cs, ITestModule.cs, IActionModule.cs, IMEFlowBlock.cs, the node
// interfaces (INodeBody / INodeOrbit / INodeWaypoint / ITestNodeLabel / IScoreableObjective / IMissionKerbal) and Editor/MEBasicNode.cs
// (Load only) of Assembly-CSharp/Expansions/Missions.
//
// C# -> C++ mapping used by the whole Missions port:
//   * MonoBehaviour components become plain objects. MENode (a MonoBehaviour on the node's GameObject, parent = the mission's) is created by
//     MENode::Spawn(mission) and owned by the mission (Mission::AdoptNode); TestModule / ActionModule (components added with
//     gameObject.AddComponent(type)) are owned by their TestGroup / MENode. Awake() is the constructor; Start()'s ExpansionsLoader check and
//     the OnDestroy event removal are the destructor / not modelled (see "Not ported").
//   * `Type.GetType("Expansions.Missions.Tests." + name)` and `AssemblyLoader.GetClassByName(typeof(TestModule), name)` are one
//     registry: TestModuleRegistry / ActionModuleRegistry. WP-55 registers the stock node library there. A name that is not registered is
//     logged and the module dropped exactly like the C# "type was not found" branch (TestGroup.cs:119, MENode.cs:727); the optional
//     `fallback` hook of the registry lets a host keep unknown modules as opaque ConfigNodes (not in KSP).
//   * `x is ActionCreateVessel` / `GetType() == typeof(T)`: IsA(name) / TypeName() on the module (the registered class names).
//   * Coroutines: ActionModule::Fire() returns an Enumerator; MENode::ActivateNode hands it to MissionsEnvironment::StartCoroutine.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/missions/mission_display.hpp"
#include "ksp/missions/mission_types.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {
class CelestialBody;
class Orbit;
namespace FinePrint {
class Waypoint;
}
}  // namespace ksp

namespace ksp::Expansions::Missions {

class MissionOrbitRenderer;
class Asteroid;
class Comet;
namespace Editor {
class MissionEditorValidator;
}
namespace Flow {
class MEFlowParser;
}

// ---------------------------------------------------------------------------------------------------------------------------
// IMEFlowBlock.cs:5. MENode and the flow blocks (MEFlowBlock / MEFlowThenBlock / MEFlowOrBlock) are the three implementations; `FlowKind`
// replaces the `is MENode` / `is MEFlowOrBlock` / `is MEFlowThenBlock` tests of the flow code.
namespace Flow {
class IMEFlowBlock {
public:
    enum class Kind { Node, Block, Then, Or };
    virtual ~IMEFlowBlock() = default;
    virtual Kind FlowKind() const = 0;
    virtual bool HasVisibleChildren() const = 0;      // :7
    virtual bool HasReachableObjectives() const = 0;  // :9
    virtual bool HasObjectives() const = 0;           // :11
    virtual int CountObjectives() const = 0;          // :13
    virtual void UpdateMissionFlowUI(MEFlowParser* parser) = 0;  // :15
};
}  // namespace Flow

// ---- the optional node-module interfaces ------------------------------------------------------------------------------------
// INodeBody.cs:3 / INodeOrbit.cs:3 / INodeWaypoint.cs:5
class INodeBody {
public:
    virtual ~INodeBody() = default;
    virtual bool HasNodeBody() = 0;
    virtual CelestialBody* GetNodeBody() = 0;
};
class INodeOrbit {
public:
    virtual ~INodeOrbit() = default;
    virtual bool HasNodeOrbit() = 0;
    virtual Orbit* GetNodeOrbit() = 0;
};
class INodeWaypoint {
public:
    virtual ~INodeWaypoint() = default;
    virtual bool HasNodeWaypoint() = 0;
    virtual FinePrint::Waypoint* GetNodeWaypoint() = 0;
};
// ITestNodeLabel.cs:5
class ITestNodeLabel {
public:
    virtual ~ITestNodeLabel() = default;
    virtual MENode* node() = 0;
    virtual bool HasNodeLabel() = 0;
    virtual bool HasWorldPosition() = 0;
    virtual Vector3f GetWorldPosition() = 0;
    virtual std::string GetExtraText() = 0;
};
// IScoreableObjective.cs:5: `object GetScoreModifier(Type scoreModule)`: a boxed double (ScoreModule_Accuracy casts it) or a Vessel
// (ScoreModule_Resource uses `as Vessel`).
struct ScoreModifier {
    std::optional<double> number;
    Vessel* vessel = nullptr;
};
class IScoreableObjective {
public:
    virtual ~IScoreableObjective() = default;
    virtual ScoreModifier GetScoreModifier(const std::string& scoreModuleTypeName) = 0;
};
// IMissionKerbal.cs:5 (node deleted / roster events: implemented by the kerbal-aware modules of WP-55)
class IMissionKerbal {
public:
    virtual ~IMissionKerbal() = default;
    virtual void NodeDeleted() = 0;
};

// ---- views of the WP-55 create actions that Mission / MissionSituation / MENode read -------------------------------------------
// (the C# reads ActionCreateVessel.vesselSituation, ActionCreateKerbal.persistentId, ActionCreateAsteroid.asteroid, ActionCreateComet.comet,
// ActionCreateFlag.persistentID, ActionCreateLaunchSite.launchSiteSituation.launchSiteName and ActionMissionScore's descriptions; WP-55
// derives its action classes from these)
class IActionCreateVessel {
public:
    virtual ~IActionCreateVessel() = default;
    virtual VesselSituation* GetVesselSituation() = 0;
};
class IActionCreateKerbal {
public:
    virtual ~IActionCreateKerbal() = default;
    virtual uint32_t GetPersistentId() = 0;
};
class IActionCreateAsteroid {
public:
    virtual ~IActionCreateAsteroid() = default;
    virtual Asteroid* GetAsteroid() = 0;
    virtual uint32_t GetPersistentID() = 0;
};
class IActionCreateComet {
public:
    virtual ~IActionCreateComet() = default;
    virtual Comet* GetComet() = 0;
    virtual uint32_t GetPersistentID() = 0;
};
class IActionCreateFlag {
public:
    virtual ~IActionCreateFlag() = default;
    virtual uint32_t GetPersistentID() = 0;
};
class IActionCreateLaunchSite {
public:
    virtual ~IActionCreateLaunchSite() = default;
    virtual std::string GetLaunchSiteName() = 0;
};
class IActionMissionScore {
public:
    virtual ~IActionMissionScore() = default;
    virtual std::string GetScoreDescription() = 0;
    virtual std::string GetAwardedScoreDescription() = 0;
};

// ---------------------------------------------------------------------------------------------------------------------------
// ITestModule.cs:5 / IActionModule.cs:7
class ITestModule : public IMENodeDisplay {
public:
    virtual bool Test() = 0;
    virtual bool ShouldCreateCheckpoint() = 0;
    virtual void RunValidationWrapper(Editor::MissionEditorValidator* validator) = 0;
    virtual void Initialize(TestGroup* testGroup, const std::string& title = "") = 0;
    virtual TestModule* InitializeTest() = 0;
    virtual TestModule* ClearTest() = 0;
    virtual std::string GetAppObjectiveInfo() = 0;
    virtual void Load(const ConfigNode& node) = 0;
    virtual void Save(ConfigNode& node) const = 0;
};
class ActionModule;
class IActionModule : public IMENodeDisplay {
public:
    virtual void RunValidationWrapper(Editor::MissionEditorValidator* validator) = 0;
    virtual void Initialize(MENode* node) = 0;
    virtual Enumerator Fire() = 0;
    virtual std::string GetAppObjectiveInfo() = 0;
    virtual Vector3f ActionLocation() = 0;
    virtual void OnCloned(ActionModule*& actionModule) = 0;
    virtual void Load(const ConfigNode& node) = 0;
    virtual void Save(ConfigNode& node) const = 0;
    // GetType().Name / `is T`: the registered class name and its chain (nearest class first)
    virtual std::string TypeName() const = 0;
    virtual bool IsA(std::string_view className) const = 0;
};

// ---------------------------------------------------------------------------------------------------------------------------
// TestModule.cs:9 (a MonoBehaviour: `name = GetType().Name` is the constructor argument)
class TestModule : public ITestModule {
public:
    explicit TestModule(std::string typeName);          // :40 Awake + :87 SetupPersistentIdEvents
    ~TestModule() override;                              // :80 OnDestroy
    TestModule(const TestModule&) = delete;
    TestModule& operator=(const TestModule&) = delete;

    std::string title;                                   // :11
    std::vector<std::string> parametersDisplayedInSAP;   // :13
    bool hasWaypoint = false;                            // :15
    bool hasOrbit = false;                               // :25
    FinePrint::Waypoint* waypoint = nullptr;             // :17 (private)

    const std::string& name() const { return name_; }    // :27
    TestGroup* testGroup() const { return testGroup_; }  // :29
    MENode* node() const { return node_; }               // :31

    // GetType().Name / `is T` (see IActionModule)
    virtual std::string TypeName() const { return name_; }
    virtual bool IsA(std::string_view className) const { return className == "TestModule" || className == name_; }

    void Initialize(TestGroup* testGroup, const std::string& title = "") override;  // :33
    TestModule* InitializeTest() override;                                          // :129
    TestModule* ClearTest() override;                                               // :152
    bool ShouldCreateCheckpoint() override { return true; }                         // :168
    virtual void Initialized() {}                                                   // :173
    virtual void Cleared() {}                                                       // :177
    bool Test() override { return true; }                                           // :181
    std::string GetAppObjectiveInfo() override;                                     // :186 (BaseAPFieldList reflection: not ported, "")
    void RunValidationWrapper(Editor::MissionEditorValidator* validator) override;  // :203
    virtual void RunValidation(Editor::MissionEditorValidator* validator) { (void)validator; }  // :208
    virtual void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) { (void)oldId; (void)newId; }  // :100
    virtual void OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) { (void)vesselID; (void)oldId; (void)newId; }  // :111
    virtual void OnVesselDocking(uint32_t oldId, uint32_t newId) { (void)oldId; (void)newId; }  // :121
    virtual void OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) { (void)oldVessel; (void)newVessel; }  // :126

    std::string GetName() override;                                                  // :212
    std::string GetDisplayName() override;                                           // :217
    void AddParameterToSAP(const std::string& parameter) override;                   // :222
    void RemoveParameterFromSAP(const std::string& parameter) override;              // :227
    void AddParameterToNodeBody(const std::string& parameter) override;              // :237
    void AddParameterToNodeBodyAndUpdateUI(const std::string& parameter) override;   // :242
    void RemoveParameterFromNodeBody(const std::string& parameter) override;         // :248
    void RemoveParameterFromNodeBodyAndUpdateUI(const std::string& parameter) override;  // :253
    bool HasNodeBodyParameter(const std::string& parameter) override;                // :259
    bool HasSAPParameter(const std::string& parameter) override;                     // :264
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :269 (editor reflection: not ported, "")
    void UpdateNodeBodyUI() override;                                                // :289
    std::vector<IMENodeDisplay*> GetInternalParametersToDisplay() override;          // :294
    MENode* GetNode() override;                                                      // :299
    std::string GetInfo() override;                                                  // :304
    void ParameterSetupComplete() override {}                                        // :309
    void Load(const ConfigNode& node) override;                                      // :314
    void Save(ConfigNode& node) const override;                                      // :320

private:
    void VesselPersistentIdChangedWrapper(uint32_t oldId, uint32_t newId);                  // :93
    void PartPersistentIdChangedWrapper(uint32_t vesselID, uint32_t oldId, uint32_t newId);  // :104
    void VesselDockingWrapper(uint32_t oldId, uint32_t newId);                              // :116
    void VesselsUndockingWrapper(Vessel* oldVessel, Vessel* newVessel);                     // :121

    std::string name_;
    TestGroup* testGroup_ = nullptr;
    MENode* node_ = nullptr;
    bool hasNodeLabel_ = false;  // :23
    void* nodeLabel_ = nullptr;  // :25 (private NodeLabel)
};

// ---------------------------------------------------------------------------------------------------------------------------
// ActionModule.cs:9
class ActionModule : public IActionModule {
public:
    explicit ActionModule(std::string typeName);         // :99 Awake
    ~ActionModule() override;                             // :124 OnDestroy
    ActionModule(const ActionModule&) = delete;
    ActionModule& operator=(const ActionModule&) = delete;

    std::string title;                                   // :11
    std::vector<std::string> parametersDisplayedInSAP;   // :13
    bool restartOnSceneLoad = false;                     // :15
    bool isRunning = false;                              // :17
    MENode* node = nullptr;                              // :19

    const std::string& name() const { return name_; }    // :21

    std::string TypeName() const override { return name_; }
    bool IsA(std::string_view className) const override { return className == "ActionModule" || className == name_; }

    void Initialize(MENode* n) override;                 // :93
    Enumerator Fire() override;                          // :133
    Vector3f ActionLocation() override { return Vector3f(); }  // :138
    void OnCloned(ActionModule*& actionModule) override { (void)actionModule; }  // :143
    void RunValidationWrapper(Editor::MissionEditorValidator* validator) override;  // :147
    virtual void RunValidation(Editor::MissionEditorValidator* validator) { (void)validator; }  // :152
    virtual void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) { (void)oldId; (void)newId; }  // :31
    virtual void OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) { (void)vesselID; (void)oldId; (void)newId; }  // :42
    virtual void OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) { (void)oldVessel; (void)newVessel; }  // :63
    virtual void OnVesselsDocking(uint32_t oldId, uint32_t newId) { (void)oldId; (void)newId; }  // :67

    std::string GetName() override;                                                  // :156
    std::string GetDisplayName() override;                                           // :161
    void AddParameterToSAP(const std::string& parameter) override;                   // :166
    void RemoveParameterFromSAP(const std::string& parameter) override;              // :171
    void AddParameterToNodeBody(const std::string& parameter) override;              // :181
    void AddParameterToNodeBodyAndUpdateUI(const std::string& parameter) override;   // :186
    void RemoveParameterFromNodeBody(const std::string& parameter) override;         // :192
    void RemoveParameterFromNodeBodyAndUpdateUI(const std::string& parameter) override;  // :197
    bool HasNodeBodyParameter(const std::string& parameter) override;                // :203
    bool HasSAPParameter(const std::string& parameter) override;                     // :208
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :213 (editor reflection: not ported, "")
    void UpdateNodeBodyUI() override;                                                // :223
    std::vector<IMENodeDisplay*> GetInternalParametersToDisplay() override;          // :231
    MENode* GetNode() override;                                                      // :236
    std::string GetInfo() override;                                                  // :241
    std::string GetAppObjectiveInfo() override;                                      // :246 (BaseAPFieldList reflection: not ported, "")
    void ParameterSetupComplete() override {}                                        // :262
    void Load(const ConfigNode& node) override;                                      // :267
    void Save(ConfigNode& node) const override;                                      // :276

private:
    void VesselPersistentIdChangedWrapper(uint32_t oldId, uint32_t newId);                  // :23
    void PartPersistentIdChangedWrapper(uint32_t vesselID, uint32_t oldId, uint32_t newId);  // :34
    void VesselsUndockingWrapper(Vessel* oldVessel, Vessel* newVessel);                     // :55
    void VesselDockingWrapper(uint32_t oldId, uint32_t newId);                              // :59

    std::string name_;
};

// ---- the registries that stand for Type.GetType(...) / AssemblyLoader.GetClassByName ------------------------------------------
struct TestModuleType {
    std::string name;
    std::function<std::unique_ptr<TestModule>()> create;  // AddComponent(type): Awake runs inside the constructor
};
class TestModuleRegistry {
public:
    static void Register(TestModuleType type);
    static const TestModuleType* Find(const std::string& name);
    // host hook for names that are not registered (unset: KSP's behaviour, drop the module)
    static inline std::function<std::unique_ptr<TestModule>(const std::string& name, const ConfigNode* cfg)> fallback;
    static void ResetForTests();
};
struct ActionModuleType {
    std::string name;
    std::function<std::unique_ptr<ActionModule>()> create;
};
class ActionModuleRegistry {
public:
    static void Register(ActionModuleType type);
    static const ActionModuleType* Find(const std::string& name);
    static inline std::function<std::unique_ptr<ActionModule>(const std::string& name, const ConfigNode* cfg)> fallback;
    static void ResetForTests();
};

// ---------------------------------------------------------------------------------------------------------------------------
// TestGroup.cs:9
class TestGroup {
public:
    explicit TestGroup(MENode* node);                      // :26
    TestGroup(MENode* node, std::string title);            // :31
    TestGroup(const TestGroup&) = delete;
    TestGroup& operator=(const TestGroup&) = delete;

    std::string title;                                     // :11
    std::string description;                               // :13
    std::vector<std::unique_ptr<ITestModule>> testModules;  // :15
    MENode* node() const { return node_; }                 // :17
    bool isActive() const { return isActive_; }            // :19

    bool Test();                                           // :36
    TestGroup* initializeGroup();                          // :54 (internal)
    TestGroup* ClearGroup();                               // :64 (internal)
    void InitializeTestModules();                          // :74
    void ClearTestModules();                               // :82
    void AddTestModule(const std::string& testName, const ConfigNode* cfg);  // :90
    void RunValidationWrapper(Editor::MissionEditorValidator* validator);    // :117
    virtual ~TestGroup() = default;
    virtual void RunValidation(Editor::MissionEditorValidator* validator) { (void)validator; }  // :126
    virtual void Initialized() {}                          // :130
    virtual void Cleared() {}                              // :134
    virtual void OnCloned(TestGroup* testGroupBase) { (void)testGroupBase; }  // :138
    void Load(const ConfigNode& node);                     // :142
    void Save(ConfigNode& node) const;                     // :152

private:
    MENode* node_ = nullptr;
    bool isActive_ = false;
};

// ---------------------------------------------------------------------------------------------------------------------------
// Editor/MEBasicNode.cs:10 (the node library entry; Initialize() builds a GUI node and is not ported)
class MEBasicNode {
public:
    struct ExtendedInfo {
        std::string name;
        std::string information;
        std::vector<ExtendedInfo> scoringInformation;
    };
    MEBasicNode();                                          // :53
    std::string name = "Unknown";                           // :19
    std::string displayName = "#autoLOC_260145";            // :21
    std::string description = "#autoLOC_260145";            // :23
    std::string tooltipDescription = "#autoLOC_260145";     // :25
    std::string categoryShortDescription = "Unknown";       // :27
    bool isObjective = false;                               // :29
    bool isLogicNode = false;                               // :31
    Color headerColor;                                      // :33
    std::string category = "#autoLOC_8100159";              // :35
    std::string categoryDisplayName = "#autoLOC_260145";    // :37
    std::string iconURL;                                    // :39
    std::vector<std::string> tests;                         // :41
    std::vector<std::string> actions;                       // :43
    std::vector<Editor::DisplayParameter> defaultSAPParameters;   // :45
    std::vector<Editor::DisplayParameter> defaultNodeBodyParameters;  // :47
    std::vector<ExtendedInfo> extInfoActionModules;         // :49
    std::vector<ExtendedInfo> extInfoTestModules;           // :51
    void Save(ConfigNode& node) const {}                    // :147
    void Load(const ConfigNode& node);                      // :151
};

// ---------------------------------------------------------------------------------------------------------------------------
// MENode.cs:14
class MENode : public Flow::IMEFlowBlock {
public:
    explicit MENode(Mission* mission);                      // :413 Awake (+ MENode.Spawn(mission) :456 sets mission)
    ~MENode() override;
    MENode(const MENode&) = delete;
    MENode& operator=(const MENode&) = delete;

    // MENode.Spawn(mission) :456 (the node is owned by the mission; a null mission parks it in a process-wide pool)
    static MENode* Spawn(Mission* mission);
    static MENode* Spawn(Mission* mission, const std::string& title);          // :468
    static MENode* Spawn(Mission* mission, const MEBasicNode& basicNode);      // :475
    static MENode* Clone(MENode* nodeRef, const Vector2f& nodePos);            // :511
    static void ResetPool();                                                    // tests: frees the nodes spawned with a null mission

    // ---- fields ----
    Color nodeColor;                                        // :20
    std::string description;                                // :23
    std::string basicNodeSource;                            // :25
    std::string basicNodeIconURL;                           // :27 (internal)
    Vector2f editorPosition;                                // :29
    bool isStartNode = false;                               // :32 (internal)
    bool isPaused = false;                                  // :35 (internal)
    int dockedIndex = 0;                                    // :37
    bool isObjective = false;                               // :40
    bool showScreenMessage = false;                         // :48
    int msgTime = 5;                                        // :51
    bool isEvent = false;                                   // :54
    bool isEndNode = false;                                 // :57
    MissionEndOptions missionEndOptions = MissionEndOptions::Success;  // :60
    std::string message;                                    // :62
    std::vector<MENode*> toNodes;                           // :65
    bool activateOnceOnly = true;                           // :68
    std::vector<Color> fromNodesConnectionColor;            // :70
    std::vector<Guid> fromNodeIDs;                          // :73 (internal)
    std::vector<MENode*> fromNodes;                         // :75
    std::vector<Guid> toNodeIDs;                            // :78 (internal)
    Mission* mission = nullptr;                             // :80
    std::vector<std::unique_ptr<TestGroup>> testGroups;     // :82
    std::vector<std::unique_ptr<IActionModule>> actionModules;  // :84
    std::vector<Editor::DisplayParameter> nodeBodyParameters;   // :86
    double activatedUT = 0.0;                               // :88
    Editor::IMEGUINode* guiNode = nullptr;                  // :90
    std::vector<MENode*> dockedNodes;                       // :95
    std::vector<Guid> dockedNodesIDsOnLoad;                 // :97
    MENode* dockParentNode = nullptr;                       // :99
    Guid dockParentNodeID;                                  // :102 (internal)
    bool UpdateInfoOnRefresh = false;                       // :118
    std::function<void(Flow::MEFlowParser*)> OnUpdateFlowUI;      // :120 (internal)
    std::string gameObjectName = "Node";                    // gameObject.name (:135 title setter)

    // ---- properties ----
    const Guid& id() const { return id_; }                  // :124
    void SetIdForTests(const Guid& g) { id_ = g; }
    const std::string& Title() const { return title_; }     // :140
    void SetTitle(const std::string& value);                // :144
    void SetTitleNoGui(const std::string& value);           // :135 (the private `title` setter)
    const std::string& ObjectiveString() const { return title_; }  // :156
    bool HasBeenActivated() const;                          // :158
    bool IsActiveNode() const;                              // :170
    bool IsReachable() const;                               // :172
    bool IsHiddenByEvent() const;                           // :199
    const std::string& category() const { return category_; }  // :228
    void SetCategory(const std::string& c) { category_ = c; }
    bool IsDockedToStartNode() const;                       // :230
    bool IsOrphanNode() const;                              // :242
    bool isNextObjective() const;                           // :254
    bool IsDocked() const { return dockParentNode != nullptr; }  // :256
    bool HasPendingVesselLaunch() const;                    // :258
    bool IsActiveAndPendingVesselLaunch() const;            // :281
    bool HasConnectedOutput() const;                        // :293 (internal)
    bool IsVesselNode() const;                              // :319
    bool IsLogicNode() const { return isLogicNode_; }       // :347
    bool IsScoringNode() const;                             // :349
    bool IsLaunchPadNode() const;                           // :361
    bool IsNodeLabelNode() const;                           // :373
    bool HasTestModules() const;                            // :391
    bool isCatchAll() const { return isCatchAll_; }         // :43 (private)

    // IMEFlowBlock (:403-409)
    Kind FlowKind() const override { return Kind::Node; }
    bool HasVisibleChildren() const override { return false; }
    bool HasReachableObjectives() const override { return false; }
    bool HasObjectives() const override { return false; }
    int CountObjectives() const override { return 0; }
    void UpdateMissionFlowUI(Flow::MEFlowParser* parser) override;  // :972

    // ---- methods ----
    void SetCatchAllNode(bool newCatchAll);                 // :592
    MENode* ActivateNode(bool Loading = false);             // :605 (internal)
    MENode* DeactivateNode();                               // :648 (internal)
    void InitializeTestGroups();                            // :668 (internal)
    void ClearTestGroups();                                 // :676 (internal)
    bool RunTests();                                        // :684
    IActionModule* AddAction(const std::string& actionName, const ConfigNode* cfg);  // :707
    void AddParametersDisplayedInSAP(const std::vector<Editor::DisplayParameter>& parameters);  // :732
    void AddParametersDisplayedInNodeBody(const std::vector<Editor::DisplayParameter>& parameters);  // :770
    bool HasNodeBodyParameter(const std::string& module, const std::string& parameter) const;  // :775
    void AddParameterToNodeBody(const std::string& module, const std::string& parameter);     // :794
    void RemoveParameterFromNodeBody(const std::string& module, const std::string& parameter);  // :802
    bool HasTestOrbit(Orbit*& testOrbit, MissionOrbitRenderer*& testOrbitRenderer, bool drawOrbit = false);  // :829
    // :922 GetAllActionModules<T>(): modules whose exact type is T (GetType() == typeof(T)) viewed as T
    template <class T>
    std::vector<T*> GetAllActionModules() {
        std::vector<T*> list;
        for (auto& am : actionModules)
            if (am->TypeName() == T::kTypeName)
                if (T* t = dynamic_cast<T*>(am.get())) list.push_back(t);
        return list;
    }
    void RunValidationWrapper(Editor::MissionEditorValidator* validator);  // :943 (internal)
    virtual void RunValidation(Editor::MissionEditorValidator* validator) { (void)validator; }  // :960
    virtual void Activated() {}                             // :964
    virtual void Deactivated() {}                           // :968
    // IDiscoverable (:980-1013)
    std::string RevealName() const { return title_; }
    std::string RevealDisplayName() const { return title_; }
    double RevealSpeed() const { return 0.0; }
    double RevealAltitude() const { return 0.0; }
    std::string RevealSituationString() const { return title_; }
    std::string RevealType() const { return description; }
    float RevealMass() const { return 0.0f; }

    void Load(const ConfigNode& node);                      // :1015
    void Save(ConfigNode& node) const;                      // :1148

private:
    friend class Mission;
    friend class Flow::MEFlowParser;
    bool ActionsContain(std::string_view className) const;  // :551 ActionsContainModule<T>
    void SetEndNode(bool value);                            // :575
    void SetNodeColor(const Color& c);                      // :821

    Guid id_;
    std::string title_;
    std::string category_;
    bool isCatchAll_ = false;                               // :43
    bool isLogicNode_ = false;                              // :93
};

}  // namespace ksp::Expansions::Missions
