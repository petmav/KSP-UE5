// WP-54: MENode.cs, TestGroup.cs, TestModule.cs, ActionModule.cs, Editor/MEBasicNode.cs (Load), Editor/DisplayParameter.cs. See mission_node.hpp.
#include "ksp/missions/mission_node.hpp"
#include "ksp/missions/mission_data.hpp"

#include <algorithm>
#include <map>

#include "../save/save_util.hpp"
#include "ksp/config/parse_extensions.hpp"
#include "ksp/contracts/waypoints.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_flow.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/missions/node_library.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/collections.hpp"

namespace ksp::Expansions::Missions {

namespace {
// ColorUtility.ToHtmlStringRGB (Unity/UnityEngine.CoreModule/UnityEngine/ColorUtility.cs:21)
std::string ToHtmlStringRGB(const Color& c) {
    auto ch = [](float v) { return static_cast<unsigned>(Mathf::Clamp(Mathf::RoundToInt(f32store(fx(v) * 255.0)), 0, 255)); };
    static const char* hex = "0123456789ABCDEF";
    std::string s;
    for (unsigned v : {ch(c.r), ch(c.g), ch(c.b)}) {
        s.push_back(hex[(v >> 4) & 15]);
        s.push_back(hex[v & 15]);
    }
    return s;
}

std::vector<std::unique_ptr<MENode>>& NodePool() {
    static std::vector<std::unique_ptr<MENode>> pool;
    return pool;
}

std::map<std::string, TestModuleType>& TestTypes() {
    static std::map<std::string, TestModuleType> m;
    return m;
}
std::map<std::string, ActionModuleType>& ActionTypes() {
    static std::map<std::string, ActionModuleType> m;
    return m;
}

template <class T>
void AddUniqueTo(std::vector<T>& list, const T& item) {
    if (std::find(list.begin(), list.end(), item) == list.end()) list.push_back(item);
}
}  // namespace

// ---- DisplayParameter --------------------------------------------------------------------------------------------------------

// Editor/DisplayParameter.cs:12
void Editor::DisplayParameter::Load(const ConfigNode& node) {
    node.TryGetValue("module", module);
    node.TryGetValue("parameter", parameter);
}

// ---- registries --------------------------------------------------------------------------------------------------------------

void TestModuleRegistry::Register(TestModuleType type) { TestTypes()[type.name] = std::move(type); }
const TestModuleType* TestModuleRegistry::Find(const std::string& name) {
    auto it = TestTypes().find(name);
    return it == TestTypes().end() ? nullptr : &it->second;
}
void TestModuleRegistry::ResetForTests() {
    TestTypes().clear();
    fallback = nullptr;
}
void ActionModuleRegistry::Register(ActionModuleType type) { ActionTypes()[type.name] = std::move(type); }
const ActionModuleType* ActionModuleRegistry::Find(const std::string& name) {
    auto it = ActionTypes().find(name);
    return it == ActionTypes().end() ? nullptr : &it->second;
}
void ActionModuleRegistry::ResetForTests() {
    ActionTypes().clear();
    fallback = nullptr;
}

// ---- TestModule --------------------------------------------------------------------------------------------------------------

// TestModule.cs:9 Awake (+ :87 SetupPersistentIdEvents)
TestModule::TestModule(std::string typeName) : name_(std::move(typeName)) {
    GameEvents::onPartPersistentIdChanged.Add(this, &TestModule::PartPersistentIdChangedWrapper);
    GameEvents::onVesselPersistentIdChanged.Add(this, &TestModule::VesselPersistentIdChangedWrapper);
    GameEvents::onVesselDocking.Add(this, &TestModule::VesselDockingWrapper);
    GameEvents::onVesselsUndocking.Add(this, &TestModule::VesselsUndockingWrapper);
}

// TestModule.cs:66
TestModule::~TestModule() {
    GameEvents::onPartPersistentIdChanged.Remove(this, &TestModule::PartPersistentIdChangedWrapper);
    GameEvents::onVesselPersistentIdChanged.Remove(this, &TestModule::VesselPersistentIdChangedWrapper);
    GameEvents::onVesselDocking.Remove(this, &TestModule::VesselDockingWrapper);
    GameEvents::onVesselsUndocking.Remove(this, &TestModule::VesselsUndockingWrapper);
}

// TestModule.cs:82
void TestModule::VesselPersistentIdChangedWrapper(uint32_t oldId, uint32_t newId) {
    if (oldId != 0) OnVesselPersistentIdChanged(oldId, newId);
}
// TestModule.cs:94
void TestModule::PartPersistentIdChangedWrapper(uint32_t vesselID, uint32_t oldId, uint32_t newId) {
    if (vesselID != 0 && oldId != 0) OnPartPersistentIdChanged(vesselID, oldId, newId);
}
// TestModule.cs:106
void TestModule::VesselDockingWrapper(uint32_t oldId, uint32_t newId) { OnVesselDocking(oldId, newId); }
// TestModule.cs:115
void TestModule::VesselsUndockingWrapper(Vessel* oldVessel, Vessel* newVessel) { OnVesselsUndocking(oldVessel, newVessel); }

// TestModule.cs:31
void TestModule::Initialize(TestGroup* group, const std::string& newTitle) {
    if (!newTitle.empty()) title = newTitle;
    testGroup_ = group;
    node_ = group->node();
    hasWaypoint = dynamic_cast<INodeWaypoint*>(this) != nullptr;
    hasNodeLabel_ = dynamic_cast<ITestNodeLabel*>(this) != nullptr;
    hasOrbit = dynamic_cast<INodeOrbit*>(this) != nullptr;
}

// TestModule.cs:124
TestModule* TestModule::InitializeTest() {
    GameEvents::Mission::onTestInitialized.Fire(this);
    if (hasWaypoint) {
        if (auto* nodeWaypoint = dynamic_cast<INodeWaypoint*>(this); nodeWaypoint != nullptr && nodeWaypoint->HasNodeWaypoint()) {
            waypoint = nodeWaypoint->GetNodeWaypoint();
            if (waypoint != nullptr) ScenarioCustomWaypoints::AddWaypoint(waypoint, true);
        }
    }
    if (hasNodeLabel_) {
        if (MissionsEnvironment::addNodeLabel) nodeLabel_ = MissionsEnvironment::addNodeLabel(dynamic_cast<ITestNodeLabel*>(this));
    }
    Initialized();
    return this;
}

// TestModule.cs:143
TestModule* TestModule::ClearTest() {
    GameEvents::Mission::onTestCleared.Fire(this);
    if (hasWaypoint && waypoint != nullptr) ScenarioCustomWaypoints::RemoveWaypoint(waypoint);
    if (hasNodeLabel_ && nodeLabel_ != nullptr) {
        if (MissionsEnvironment::removeNodeLabel) MissionsEnvironment::removeNodeLabel(nodeLabel_);
    }
    Cleared();
    return this;
}

// TestModule.cs:176
std::string TestModule::GetAppObjectiveInfo() { return ""; }

// TestModule.cs:192
void TestModule::RunValidationWrapper(Editor::MissionEditorValidator* validator) { RunValidation(validator); }

// TestModule.cs:201
std::string TestModule::GetName() { return name_; }

// TestModule.cs:206
std::string TestModule::GetDisplayName() { return KSP::Localization::Localizer::Format(title); }

// TestModule.cs:211
void TestModule::AddParameterToSAP(const std::string& parameter) { AddUniqueTo(parametersDisplayedInSAP, parameter); }

// TestModule.cs:216
void TestModule::RemoveParameterFromSAP(const std::string& parameter) {
    auto it = std::find(parametersDisplayedInSAP.begin(), parametersDisplayedInSAP.end(), parameter);
    if (it != parametersDisplayedInSAP.end()) parametersDisplayedInSAP.erase(it);
    if (node_->HasNodeBodyParameter(name_, parameter)) {
        node_->RemoveParameterFromNodeBody(name_, parameter);
        UpdateNodeBodyUI();
    }
}

// TestModule.cs:226
void TestModule::AddParameterToNodeBody(const std::string& parameter) { node_->AddParameterToNodeBody(name_, parameter); }

// TestModule.cs:231
void TestModule::AddParameterToNodeBodyAndUpdateUI(const std::string& parameter) {
    node_->AddParameterToNodeBody(name_, parameter);
    UpdateNodeBodyUI();
}

// TestModule.cs:237
void TestModule::RemoveParameterFromNodeBody(const std::string& parameter) { node_->RemoveParameterFromNodeBody(name_, parameter); }

// TestModule.cs:242
void TestModule::RemoveParameterFromNodeBodyAndUpdateUI(const std::string& parameter) {
    node_->RemoveParameterFromNodeBody(name_, parameter);
    UpdateNodeBodyUI();
}

// TestModule.cs:248
bool TestModule::HasNodeBodyParameter(const std::string& parameter) { return node_->HasNodeBodyParameter(name_, parameter); }

// TestModule.cs:253
bool TestModule::HasSAPParameter(const std::string& parameter) {
    return std::find(parametersDisplayedInSAP.begin(), parametersDisplayedInSAP.end(), parameter) != parametersDisplayedInSAP.end();
}

// TestModule.cs:258
std::string TestModule::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    // ParamChoices_CelestialBodySurface / MissionKerbal fields: the caller passes the text of their branch
    if (field.paramChoices != nullptr) return field.paramChoices->NodeBodyParameterString();
    if (field.missionKerbal != nullptr) return KSP::Localization::Localizer::Format("#autoLOC_8004190", {field.guiName, field.missionKerbal->Name()});
    if (field.typedText.has_value()) return *field.typedText;
    if (field.celestialBody != nullptr) {
        return KSP::Localization::Localizer::Format("#autoLOC_8004190", {field.guiName, field.celestialBody->DisplayName()});
    }
    std::string text = KSP::Localization::Localizer::Format("#autoLOC_6003000");
    if (field.hasValue) text = field.value;
    return KSP::Localization::Localizer::Format("#autoLOC_8004190", {field.guiName, text});
}

// TestModule.cs:280
void TestModule::UpdateNodeBodyUI() {
    // node.guiNode is dereferenced without a null test (NullReferenceException in C#)
    if (node_ == nullptr || node_->guiNode == nullptr) throw NullReferenceException();
    node_->guiNode->DisplayNodeBodyParameters();
}

// TestModule.cs:285
std::vector<IMENodeDisplay*> TestModule::GetInternalParametersToDisplay() { return {}; }

// TestModule.cs:290
MENode* TestModule::GetNode() { return node_; }

// TestModule.cs:295
std::string TestModule::GetInfo() { return TypeName(); }

// TestModule.cs:304
void TestModule::Load(const ConfigNode& node) {
    node.TryGetValue("title", title);
    parametersDisplayedInSAP = node.GetValuesList("parametersDisplayedInSAP");
}

// TestModule.cs:310
void TestModule::Save(ConfigNode& node) const {
    node.AddValue("name", name_);
    node.AddValue("title", title);
    for (size_t i = 0; i < parametersDisplayedInSAP.size(); ++i) node.AddValue("parametersDisplayedInSAP", parametersDisplayedInSAP[i]);
}

// ---- ActionModule ------------------------------------------------------------------------------------------------------------

// ActionModule.cs:113 Awake
ActionModule::ActionModule(std::string typeName) : name_(std::move(typeName)) {
    GameEvents::onPartPersistentIdChanged.Add(this, &ActionModule::PartPersistentIdChangedWrapper);
    GameEvents::onVesselPersistentIdChanged.Add(this, &ActionModule::VesselPersistentIdChangedWrapper);
    GameEvents::onVesselsUndocking.Add(this, &ActionModule::VesselsUndockingWrapper);
    GameEvents::onVesselDocking.Add(this, &ActionModule::VesselDockingWrapper);
}

// ActionModule.cs:95
ActionModule::~ActionModule() {
    GameEvents::onPartPersistentIdChanged.Remove(this, &ActionModule::PartPersistentIdChangedWrapper);
    GameEvents::onVesselPersistentIdChanged.Remove(this, &ActionModule::VesselPersistentIdChangedWrapper);
    GameEvents::onVesselsUndocking.Remove(this, &ActionModule::VesselsUndockingWrapper);
    GameEvents::onVesselDocking.Remove(this, &ActionModule::VesselDockingWrapper);
}

// ActionModule.cs:23
void ActionModule::VesselPersistentIdChangedWrapper(uint32_t oldId, uint32_t newId) {
    if (oldId != 0) OnVesselPersistentIdChanged(oldId, newId);
}
// ActionModule.cs:35
void ActionModule::PartPersistentIdChangedWrapper(uint32_t vesselID, uint32_t oldId, uint32_t newId) {
    if (vesselID != 0 && oldId != 0) OnPartPersistentIdChanged(vesselID, oldId, newId);
}
// ActionModule.cs:47
void ActionModule::VesselsUndockingWrapper(Vessel* oldVessel, Vessel* newVessel) { OnVesselsUndocking(oldVessel, newVessel); }
// ActionModule.cs:52
void ActionModule::VesselDockingWrapper(uint32_t oldId, uint32_t newId) { OnVesselsDocking(oldId, newId); }

// ActionModule.cs:65
void ActionModule::Initialize(MENode* n) { node = n; }

// ActionModule.cs:103
Enumerator ActionModule::Fire() { co_yield YieldInstruction::Null(); }

// ActionModule.cs:117
void ActionModule::RunValidationWrapper(Editor::MissionEditorValidator* validator) { RunValidation(validator); }

// ActionModule.cs:126
std::string ActionModule::GetName() { return name_; }

// ActionModule.cs:131
std::string ActionModule::GetDisplayName() { return KSP::Localization::Localizer::Format(title); }

// ActionModule.cs:136
void ActionModule::AddParameterToSAP(const std::string& parameter) { AddUniqueTo(parametersDisplayedInSAP, parameter); }

// ActionModule.cs:141
void ActionModule::RemoveParameterFromSAP(const std::string& parameter) {
    auto it = std::find(parametersDisplayedInSAP.begin(), parametersDisplayedInSAP.end(), parameter);
    if (it != parametersDisplayedInSAP.end()) parametersDisplayedInSAP.erase(it);
    if (node->HasNodeBodyParameter(name_, parameter)) {
        node->RemoveParameterFromNodeBody(name_, parameter);
        UpdateNodeBodyUI();
    }
}

// ActionModule.cs:151
void ActionModule::AddParameterToNodeBody(const std::string& parameter) { node->AddParameterToNodeBody(name_, parameter); }

// ActionModule.cs:156
void ActionModule::AddParameterToNodeBodyAndUpdateUI(const std::string& parameter) {
    node->AddParameterToNodeBody(name_, parameter);
    UpdateNodeBodyUI();
}

// ActionModule.cs:162
void ActionModule::RemoveParameterFromNodeBody(const std::string& parameter) { node->RemoveParameterFromNodeBody(name_, parameter); }

// ActionModule.cs:167
void ActionModule::RemoveParameterFromNodeBodyAndUpdateUI(const std::string& parameter) {
    node->RemoveParameterFromNodeBody(name_, parameter);
    UpdateNodeBodyUI();
}

// ActionModule.cs:173
bool ActionModule::HasNodeBodyParameter(const std::string& parameter) { return node->HasNodeBodyParameter(name_, parameter); }

// ActionModule.cs:178
bool ActionModule::HasSAPParameter(const std::string& parameter) {
    return std::find(parametersDisplayedInSAP.begin(), parametersDisplayedInSAP.end(), parameter) != parametersDisplayedInSAP.end();
}

// ActionModule.cs:183
std::string ActionModule::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    std::string text = KSP::Localization::Localizer::Format("#autoLOC_6003083");
    if (field.hasValue) text = field.value;
    return KSP::Localization::Localizer::Format("#autoLOC_8004190", {field.guiName, text});
}

// ActionModule.cs:193
void ActionModule::UpdateNodeBodyUI() {
    if (node->guiNode != nullptr) node->guiNode->DisplayNodeBodyParameters();
}

// ActionModule.cs:201
std::vector<IMENodeDisplay*> ActionModule::GetInternalParametersToDisplay() { return {}; }

// ActionModule.cs:206
MENode* ActionModule::GetNode() { return node; }

// ActionModule.cs:211
std::string ActionModule::GetInfo() { return name_; }

// ActionModule.cs:216
std::string ActionModule::GetAppObjectiveInfo() { return ""; }

// ActionModule.cs:236
void ActionModule::Load(const ConfigNode& cfg) {
    cfg.TryGetValue("title", title);
    parametersDisplayedInSAP = cfg.GetValuesList("parametersDisplayedInSAP");
    cfg.TryGetValue("restartOnSceneLoad", restartOnSceneLoad);
    cfg.TryGetValue("isRunning", isRunning);
}

// ActionModule.cs:244
void ActionModule::Save(ConfigNode& cfg) const {
    cfg.AddValue("name", name_);
    cfg.AddValue("title", title);
    for (size_t i = 0; i < parametersDisplayedInSAP.size(); ++i) cfg.AddValue("parametersDisplayedInSAP", parametersDisplayedInSAP[i]);
    cfg.AddValue("restartOnSceneLoad", restartOnSceneLoad);
    cfg.AddValue("isRunning", isRunning);
}

// ---- TestGroup ---------------------------------------------------------------------------------------------------------------

// TestGroup.cs:26
TestGroup::TestGroup(MENode* n) : node_(n) {}

// TestGroup.cs:32
TestGroup::TestGroup(MENode* n, std::string t) : title(std::move(t)), node_(n) {}

// TestGroup.cs:38
bool TestGroup::Test() {
    for (size_t num = 0; num < testModules.size(); ++num) {
        if (!testModules[num]->Test()) return false;
    }
    return true;
}

// TestGroup.cs:57
TestGroup* TestGroup::initializeGroup() {
    GameEvents::Mission::onTestGroupInitialized.Fire(this);
    Initialized();
    InitializeTestModules();
    isActive_ = true;
    return this;
}

// TestGroup.cs:66
TestGroup* TestGroup::ClearGroup() {
    GameEvents::Mission::onTestGroupCleared.Fire(this);
    Cleared();
    ClearTestModules();
    isActive_ = false;
    return this;
}

// TestGroup.cs:75
void TestGroup::InitializeTestModules() {
    for (size_t i = 0; i < testModules.size(); ++i) testModules[i]->InitializeTest();
}

// TestGroup.cs:83
void TestGroup::ClearTestModules() {
    for (size_t i = 0; i < testModules.size(); ++i) testModules[i]->ClearTest();
}

// TestGroup.cs:91
void TestGroup::AddTestModule(const std::string& testName, const ConfigNode* cfg) {
    const TestModuleType* type = TestModuleRegistry::Find(testName);
    std::unique_ptr<TestModule> testModule;
    if (type != nullptr) {
        testModule = type->create();
    } else if (TestModuleRegistry::fallback) {
        testModule = TestModuleRegistry::fallback(testName, cfg);
    }
    if (testModule) {
        testModule->Initialize(this);
        if (cfg != nullptr) testModule->Load(*cfg);
        testModules.push_back(std::move(testModule));
    } else {
        log::Info("Error! Test module " + testName + " type was not found in Assembly");
    }
}

// TestGroup.cs:114
void TestGroup::RunValidationWrapper(Editor::MissionEditorValidator* validator) {
    RunValidation(validator);
    for (size_t i = 0; i < testModules.size(); ++i) testModules[i]->RunValidationWrapper(validator);
}

// TestGroup.cs:139
void TestGroup::Load(const ConfigNode& cfg) {
    cfg.TryGetValue("title", title);
    const std::vector<ConfigNode*> nodes = cfg.GetNodes("TESTMODULE");
    for (const ConfigNode* configNode : nodes) {
        // GetValue("name") of a node without `name` is null: AddTestModule(null) logs and drops
        const auto nm = configNode->GetValue("name");
        AddTestModule(nm ? *nm : std::string(), configNode);
    }
}

// TestGroup.cs:149
void TestGroup::Save(ConfigNode& cfg) const {
    cfg.AddValue("title", title);
    for (const auto& testModule : testModules) {
        ConfigNode* configNode = cfg.AddNode("TESTMODULE");
        testModule->Save(*configNode);
    }
}

// ---- MEBasicNode -------------------------------------------------------------------------------------------------------------

// Editor/MEBasicNode.cs:55
MEBasicNode::MEBasicNode() {
    name = KSP::Localization::Localizer::Format("#autoLOC_8100159");
    description = KSP::Localization::Localizer::Format("#autoLOC_8200061");
    headerColor = Color(0.5f, 0.5f, 0.5f, 1.0f);
}

// Editor/MEBasicNode.cs:144
void MEBasicNode::Load(const ConfigNode& node) {
    node.TryGetValue("name", name);
    node.TryGetValue("displayName", displayName);
    node.TryGetValue("description", description);
    node.TryGetValue("tooltipDescription", tooltipDescription);
    node.TryGetValue("category", category);
    node.TryGetValue("categoryDisplayName", categoryDisplayName);
    node.TryGetValue("iconURL", iconURL);
    node.TryGetValue("isLogicNode", isLogicNode);
    if (!node.TryGetValue("isObjective", isObjective)) {
        if (isLogicNode) {
            isObjective = false;
        } else {
            isObjective = true;
        }
    }
    std::string value;
    if (node.TryGetValue("headerColor", value)) {
        ColorUtility::TryParseHtmlString(value, headerColor);
    } else {
        headerColor = MissionsEnvironment::categoryColor ? MissionsEnvironment::categoryColor(category) : Color(0.5f, 0.5f, 0.5f, 1.0f);
    }
    tests = node.GetValuesList("tests");
    actions = node.GetValuesList("actions");
    if (node.HasNode("SAP_PARAMETERS")) {
        ConfigNode* node2 = nullptr;
        if (node.TryGetNode("SAP_PARAMETERS", node2)) {
            for (const ConfigNode* n : node2->GetNodes("MODULE_PARAMETER")) {
                Editor::DisplayParameter item;
                item.Load(*n);
                AddUniqueTo(defaultSAPParameters, item);
            }
        }
    }
    if (!node.HasNode("NODEBODY_PARAMETERS")) return;
    ConfigNode* node3 = nullptr;
    if (node.TryGetNode("NODEBODY_PARAMETERS", node3)) {
        for (const ConfigNode* n : node3->GetNodes("MODULE_PARAMETER")) {
            Editor::DisplayParameter item2;
            item2.Load(*n);
            AddUniqueTo(defaultNodeBodyParameters, item2);
        }
    }
}

// ---- MENode ------------------------------------------------------------------------------------------------------------------

// MENode.cs:424 Awake (+ :456 Spawn(mission) assigns the mission)
MENode::MENode(Mission* m) : nodeColor(0.5f, 0.5f, 0.5f, 1.0f), mission(m) {
    id_ = MissionsEnvironment::NewGuid();
    editorPosition = Vector2f::zero();
    category_ = "";
}

MENode::~MENode() = default;

// MENode.cs:456
MENode* MENode::Spawn(Mission* mission) {
    auto node = std::make_unique<MENode>(mission);
    MENode* raw = node.get();
    if (mission != nullptr) {
        mission->AdoptNode(std::move(node));
    } else {
        NodePool().push_back(std::move(node));
    }
    return raw;
}

// MENode.cs:468
MENode* MENode::Spawn(Mission* mission, const std::string& title) {
    MENode* mENode = Spawn(mission);
    mENode->SetTitleNoGui(title);
    return mENode;
}

// MENode.cs:475
MENode* MENode::Spawn(Mission* mission, const MEBasicNode& basicNode) {
    MENode* mENode = Spawn(mission);
    mENode->SetTitleNoGui(basicNode.displayName);
    mENode->nodeColor = basicNode.headerColor;
    mENode->description = basicNode.description;
    mENode->basicNodeSource = basicNode.name;
    mENode->basicNodeIconURL = basicNode.iconURL;
    mENode->category_ = basicNode.category;
    mENode->testGroups.clear();
    auto testGroup = std::make_unique<TestGroup>(mENode);
    for (size_t i = 0; i < basicNode.tests.size(); ++i) testGroup->AddTestModule(basicNode.tests[i], nullptr);
    mENode->testGroups.push_back(std::move(testGroup));
    if (!mENode->testGroups[0]->testModules.empty()) {
        mENode->SetCatchAllNode(true);
        if (mission != nullptr) mission->UpdateOrphanNodeState(mENode, true);
    }
    mENode->actionModules.clear();
    for (size_t j = 0; j < basicNode.actions.size(); ++j) mENode->AddAction(basicNode.actions[j], nullptr);
    mENode->isLogicNode_ = basicNode.isLogicNode;
    mENode->isObjective = basicNode.isObjective;
    mENode->AddParametersDisplayedInNodeBody(basicNode.defaultNodeBodyParameters);
    mENode->AddParametersDisplayedInSAP(basicNode.defaultSAPParameters);
    return mENode;
}

// MENode.cs:511
MENode* MENode::Clone(MENode* nodeRef, const Vector2f& nodePos) {
    MENode* mENode = Spawn(nodeRef->mission);
    mENode->SetTitleNoGui(nodeRef->Title());
    mENode->nodeColor = nodeRef->nodeColor;
    mENode->description = nodeRef->description;
    mENode->basicNodeSource = nodeRef->basicNodeSource;
    mENode->basicNodeIconURL = nodeRef->basicNodeIconURL;
    mENode->editorPosition = nodePos;
    for (size_t i = 0; i < nodeRef->testGroups.size(); ++i) {
        auto testGroup = std::make_unique<TestGroup>(mENode);
        ConfigNode node;
        nodeRef->testGroups[i]->Save(node);
        testGroup->Load(node);
        TestGroup* tg = testGroup.get();
        mENode->testGroups.push_back(std::move(testGroup));
        tg->OnCloned(nodeRef->testGroups[i].get());
    }
    if (nodeRef->isCatchAll_) {
        mENode->SetCatchAllNode(true);
        if (mENode->mission != nullptr) mENode->mission->UpdateOrphanNodeState(mENode, true);
    }
    // GetComponents<ActionModule>(): the node's action modules in the order they were added
    std::vector<ActionModule*> components;
    for (auto& am : nodeRef->actionModules) components.push_back(dynamic_cast<ActionModule*>(am.get()));
    for (size_t j = 0; j < components.size(); ++j) {
        ConfigNode configNode;
        components[j]->Save(configNode);
        if (IActionModule* added = mENode->AddAction(components[j]->TypeName(), &configNode)) added->OnCloned(components[j]);
    }
    mENode->isLogicNode_ = nodeRef->IsLogicNode();
    mENode->isObjective = nodeRef->isObjective;
    mENode->AddParametersDisplayedInNodeBody(nodeRef->nodeBodyParameters);
    mENode->AddParametersDisplayedInSAP(nodeRef->nodeBodyParameters);
    return mENode;
}

void MENode::ResetPool() { NodePool().clear(); }

// MENode.cs:126 (title setter: :135 renames the GameObject)
void MENode::SetTitleNoGui(const std::string& value) {
    title_ = value;
    gameObjectName = "Node (" + KSP::Localization::Localizer::Format(value) + ")";
}

// MENode.cs:126
void MENode::SetTitle(const std::string& value) {
    SetTitleNoGui(value);
    if (guiNode != nullptr) guiNode->SetTitleText(value);
}

// MENode.cs:158
bool MENode::HasBeenActivated() const {
    if (!(activatedUT > 0.0) && !isStartNode) return IsDockedToStartNode();
    return true;
}

// MENode.cs:170
bool MENode::IsActiveNode() const { return mission->activeNode() == this; }

// MENode.cs:172
bool MENode::IsReachable() const {
    if (mission->isStarted() && mission->flow != nullptr && mission->flow->NodeReversePaths.ContainsKey(const_cast<MENode*>(this))) {
        const auto& info = mission->flow->NodeReversePaths.At(const_cast<MENode*>(this));
        int num = 0;
        while (true) {
            if (num < static_cast<int>(info->paths.size())) {
                const auto& nodes = info->paths[static_cast<size_t>(num)]->Nodes;
                if (std::find(nodes.begin(), nodes.end(), mission->activeNode()) != nodes.end()) break;
                ++num;
                continue;
            }
            return false;
        }
        return true;
    }
    return true;
}

// MENode.cs:199
bool MENode::IsHiddenByEvent() const {
    if (mission->flow != nullptr && mission->flow->NodeReversePaths.ContainsKey(const_cast<MENode*>(this))) {
        const auto& info = mission->flow->NodeReversePaths.At(const_cast<MENode*>(this));
        for (size_t i = 0; i < info->paths.size(); ++i) {
            const auto& path = info->paths[i];
            for (size_t j = 0; j < path->Nodes.size(); ++j) {
                MENode* mENode = path->Nodes[j];
                if (!mENode->IsActiveNode()) {
                    if (mENode->isEvent && !mENode->HasBeenActivated()) return true;
                    continue;
                }
                return false;
            }
        }
    }
    return false;
}

// MENode.cs:230
bool MENode::IsDockedToStartNode() const {
    if (dockParentNode == nullptr) return false;
    return dockParentNode->id() == mission->startNode()->id();
}

// MENode.cs:242
bool MENode::IsOrphanNode() const {
    if (static_cast<int>(fromNodeIDs.size()) < 1 && !isStartNode && isCatchAll_) return !IsDocked();
    return false;
}

// MENode.cs:254
bool MENode::isNextObjective() const { return mission->IsNextObjective(const_cast<MENode*>(this)); }

// MENode.cs:258
bool MENode::HasPendingVesselLaunch() const {
    for (size_t num = 0; num < actionModules.size(); ++num) {
        auto* actionCreateVessel = dynamic_cast<IActionCreateVessel*>(actionModules[num].get());
        if (actionCreateVessel != nullptr && !actionCreateVessel->GetVesselSituation()->launched) return true;
    }
    return false;
}

// MENode.cs:281
bool MENode::IsActiveAndPendingVesselLaunch() const {
    if (IsVesselNode() && HasBeenActivated()) return HasPendingVesselLaunch();
    return false;
}

// MENode.cs:293
bool MENode::HasConnectedOutput() const {
    if (static_cast<int>(toNodes.size()) < 1) return false;
    for (size_t num = 0; num < toNodes.size(); ++num) {
        const auto& ids = toNodes[num]->fromNodeIDs;
        if (std::find(ids.begin(), ids.end(), id_) != ids.end()) return true;
    }
    return false;
}

// MENode.cs:551
bool MENode::ActionsContain(std::string_view className) const {
    for (size_t num = 0; num < actionModules.size(); ++num) {
        if (actionModules[num]->IsA(className)) return true;
    }
    return false;
}

// MENode.cs:319
bool MENode::IsVesselNode() const {
    if (ActionsContain("ActionCreateVessel")) return true;
    if (ActionsContain("ActionCreateAsteroid")) return true;
    if (ActionsContain("ActionCreateKerbal")) return true;
    if (ActionsContain("ActionCreateFlag")) return true;
    if (ActionsContain("ActionCreateComet")) return true;
    return false;
}

// MENode.cs:349
bool MENode::IsScoringNode() const { return ActionsContain("ActionMissionScore"); }

// MENode.cs:361
bool MENode::IsLaunchPadNode() const { return ActionsContain("ActionCreateLaunchSite"); }

// MENode.cs:373
bool MENode::IsNodeLabelNode() const {
    for (size_t i = 0; i < testGroups.size(); ++i) {
        for (size_t j = 0; j < testGroups[i]->testModules.size(); ++j) {
            if (dynamic_cast<ITestNodeLabel*>(testGroups[i]->testModules[j].get()) != nullptr) return true;
        }
    }
    return false;
}

// MENode.cs:391
bool MENode::HasTestModules() const {
    if (!testGroups.empty()) return !testGroups[0]->testModules.empty();
    return false;
}

// MENode.cs:575
void MENode::SetEndNode(bool value) {
    if (guiNode != nullptr) guiNode->SetEndNode(value);
}

// MENode.cs:592
void MENode::SetCatchAllNode(bool newCatchAll) {
    isCatchAll_ = newCatchAll;
    if (guiNode != nullptr) guiNode->UpdateInputConnectorImage();
}

// MENode.cs:821
void MENode::SetNodeColor(const Color& c) {
    if (guiNode != nullptr) guiNode->SetNodeColor(c);
}

// MENode.cs:605
MENode* MENode::ActivateNode(bool Loading) {
    if (!Loading) {
        activatedUT = MissionsEnvironment::UniversalTime();
        GameEvents::Mission::onNodeActivated.Fire(this);
        Activated();
    }
    if (HasTestModules() || !IsDocked()) mission->SetNextObjectives(this);
    if (isEvent) mission->RemoveInactiveEvent(this);
    for (size_t i = 0; i < toNodes.size(); ++i) {
        MENode* mENode = toNodes[i];
        if (!mENode->IsOrphanNode()) mENode->InitializeTestGroups();
    }
    if (!Loading) {
        int j = 0;
        for (int count = static_cast<int>(actionModules.size()); j < count; ++j) MissionsEnvironment::StartCoroutine(actionModules[static_cast<size_t>(j)]->Fire());
        for (size_t k = 0; k < toNodes.size(); ++k) {
            MENode* mENode2 = toNodes[k];
            if (mENode2->showScreenMessage && !mENode2->description.empty() && !mENode2->isStartNode && !mENode2->IsOrphanNode() && !mENode2->IsDocked()) {
                MissionsEnvironment::PostScreenMessage(mENode2->description, static_cast<float>(mENode2->msgTime));
            }
        }
    }
    return this;
}

// MENode.cs:648
MENode* MENode::DeactivateNode() {
    GameEvents::Mission::onNodeDeactivated.Fire(this);
    Deactivated();
    for (size_t i = 0; i < toNodes.size(); ++i) {
        MENode* mENode = toNodes[i];
        if (!mENode->IsOrphanNode()) mENode->ClearTestGroups();
    }
    ClearTestGroups();
    if (IsOrphanNode()) InitializeTestGroups();
    return this;
}

// MENode.cs:668
void MENode::InitializeTestGroups() {
    for (size_t i = 0; i < testGroups.size(); ++i) testGroups[i]->initializeGroup();
}

// MENode.cs:676
void MENode::ClearTestGroups() {
    for (size_t i = 0; i < testGroups.size(); ++i) testGroups[i]->ClearGroup();
}

// MENode.cs:684
bool MENode::RunTests() {
    if (activateOnceOnly && activatedUT > 0.0) return false;
    for (size_t num = 0; num < testGroups.size(); ++num) {
        if (testGroups[num]->Test()) return true;
    }
    return false;
}

// MENode.cs:707
IActionModule* MENode::AddAction(const std::string& actionName, const ConfigNode* cfg) {
    const ActionModuleType* type = ActionModuleRegistry::Find(actionName);
    std::unique_ptr<ActionModule> created;
    if (type != nullptr) {
        created = type->create();
    } else if (ActionModuleRegistry::fallback) {
        created = ActionModuleRegistry::fallback(actionName, cfg);
    }
    if (created) {
        IActionModule* actionModule = created.get();
        actionModule->Initialize(this);
        if (cfg != nullptr) actionModule->Load(*cfg);
        actionModules.push_back(std::move(created));
        return actionModule;
    }
    log::Info("Error! Action module " + actionName + " type was not found in Assembly");
    return nullptr;
}

// MENode.cs:732
void MENode::AddParametersDisplayedInSAP(const std::vector<Editor::DisplayParameter>& parameters) {
    for (size_t i = 0; i < testGroups.size(); ++i) {
        for (size_t j = 0; j < testGroups[i]->testModules.size(); ++j) {
            for (size_t k = 0; k < parameters.size(); ++k) {
                if (testGroups[i]->testModules[j]->GetName() == parameters[k].module) testGroups[i]->testModules[j]->AddParameterToSAP(parameters[k].parameter);
            }
        }
    }
    for (size_t l = 0; l < actionModules.size(); ++l) {
        for (size_t m = 0; m < parameters.size(); ++m) {
            if (actionModules[l]->GetName() == parameters[m].module) actionModules[l]->AddParameterToSAP(parameters[m].parameter);
        }
    }
    if (!isStartNode) return;
    for (size_t n = 0; n < parameters.size(); ++n) {
        if (mission->situation->GetName() == parameters[n].module) mission->situation->AddParameterToSAP(parameters[n].parameter);
    }
}

// MENode.cs:770
void MENode::AddParametersDisplayedInNodeBody(const std::vector<Editor::DisplayParameter>& parameters) { nodeBodyParameters = parameters; }

// MENode.cs:775
bool MENode::HasNodeBodyParameter(const std::string& module, const std::string& parameter) const {
    for (size_t num = 0; num < nodeBodyParameters.size(); ++num) {
        if (nodeBodyParameters[num].module == module && nodeBodyParameters[num].parameter == parameter) return true;
    }
    return false;
}

// MENode.cs:794
void MENode::AddParameterToNodeBody(const std::string& module, const std::string& parameter) {
    Editor::DisplayParameter item;
    item.module = module;
    item.parameter = parameter;
    AddUniqueTo(nodeBodyParameters, item);
}

// MENode.cs:802
void MENode::RemoveParameterFromNodeBody(const std::string& module, const std::string& parameter) {
    for (int num = static_cast<int>(nodeBodyParameters.size()) - 1; num >= 0; --num) {
        if (nodeBodyParameters[static_cast<size_t>(num)].module == module && nodeBodyParameters[static_cast<size_t>(num)].parameter == parameter) {
            nodeBodyParameters.erase(nodeBodyParameters.begin() + num);
            return;
        }
    }
}

// MENode.cs:943
void MENode::RunValidationWrapper(Editor::MissionEditorValidator* validator) {
    if (!isEndNode && !IsDockedToStartNode() && !ActionsContain("ActionCreateLaunchSite") && (!IsDocked() || !IsScoringNode()) && !HasConnectedOutput()) {
        validator->AddNodeFail(this, KSP::Localization::Localizer::Format("#autoLOC_8006040"));
    }
    RunValidation(validator);
    for (size_t i = 0; i < testGroups.size(); ++i) testGroups[i]->RunValidationWrapper(validator);
    for (size_t j = 0; j < actionModules.size(); ++j) actionModules[j]->RunValidationWrapper(validator);
}

// MENode.cs:972
void MENode::UpdateMissionFlowUI(Flow::MEFlowParser* parser) {
    if (OnUpdateFlowUI) OnUpdateFlowUI(parser);
}

// MENode.cs:1015
void MENode::Load(const ConfigNode& node) {
    if (node.HasValue("id")) id_ = Guid::Parse(*node.GetValue("id"));
    std::string value;
    node.TryGetValue("title", value);
    SetTitleNoGui(value);
    node.TryGetValue("description", description);
    node.TryGetValue("showScreenMessage", showScreenMessage);
    node.TryGetValue("msgTime", msgTime);
    if (node.HasValue("category")) category_ = *node.GetValue("category");
    node.TryGetValue("editorPosition", editorPosition);
    node.TryGetValue("isStartNode", isStartNode);
    if (category_.empty() && isStartNode) category_ = "Start";
    node.TryGetValue("isEndNode", isEndNode);
    node.TryGetValue("message", message);
    node.TryGetValue("dockedIndex", dockedIndex);
    node.TryGetValue("isObjective", isObjective);
    node.TryGetValue("isCatchAll", isCatchAll_);
    node.TryGetValue("isEvent", isEvent);
    node.TryGetValue("activateOnceOnly", activateOnceOnly);
    if (node.HasValue("activatedUT")) {
        activatedUT = dotnet::ParseDouble(*node.GetValue("activatedUT"));
    } else {
        activatedUT = 0.0;
    }
    node.TryGetValue("isLogicNode", isLogicNode_);
    node.TryGetValue("basicNodeSource", basicNodeSource);
    node.TryGetValue("basicNodeIconURL", basicNodeIconURL);
    if (((basicNodeIconURL == "") & (basicNodeSource != "")) && MissionsEnvironment::basicNodeIconURL) {
        // MissionEditorLogic.Instance.basicNodes[i].name == basicNodeSource -> iconURL (the editor's list; "" when no entry matches)
        const std::string found = MissionsEnvironment::basicNodeIconURL(basicNodeSource);
        if (!found.empty()) basicNodeIconURL = found;
    }
    std::string value2;
    if (node.TryGetValue("nodeColor", value2) && ColorUtility::TryParseHtmlString(value2, nodeColor)) SetNodeColor(nodeColor);
    node.TryGetValue("dockParentNodeID", dockParentNodeID);
    if (node.HasNode("FROMNODES")) {
        const std::vector<ConfigNode*> nodes = node.GetNode("FROMNODES")->GetNodes();
        for (size_t j = 0; j < nodes.size(); ++j) {
            std::string value3;
            if (nodes[j]->TryGetValue("nodeID", value3)) {
                const Guid item = Guid::Parse(value3);
                if (std::find(fromNodeIDs.begin(), fromNodeIDs.end(), item) == fromNodeIDs.end()) fromNodeIDs.push_back(item);
            }
            std::string value4;
            Color color;
            if (nodes[j]->TryGetValue("connectorColor", value4) && ColorUtility::TryParseHtmlString(value4, color)) fromNodesConnectionColor.push_back(color);
        }
    }
    if (node.HasNode("TONODEIDS")) {
        const std::vector<std::string> values = node.GetNode("TONODEIDS")->GetValuesList("nodeID");
        for (size_t k = 0; k < values.size(); ++k) {
            const Guid item2 = Guid::Parse(values[k]);
            if (std::find(toNodeIDs.begin(), toNodeIDs.end(), item2) == toNodeIDs.end()) toNodeIDs.push_back(item2);
        }
    }
    ConfigNode* node2 = nullptr;
    if (node.TryGetNode("TESTGROUPS", node2)) {
        for (const ConfigNode* tg : node2->GetNodes("TESTGROUP")) {
            auto testGroup = std::make_unique<TestGroup>(this);
            testGroup->Load(*tg);
            testGroups.push_back(std::move(testGroup));
        }
    }
    ConfigNode* node3 = nullptr;
    if (node.TryGetNode("ACTIONS", node3)) {
        for (const ConfigNode* am : node3->GetNodes("ACTIONMODULE")) {
            const auto nm = am->GetValue("name");
            AddAction(nm ? *nm : std::string(), am);
        }
    }
    ConfigNode* node4 = nullptr;
    if (node.TryGetNode("NODEBODY_PARAMETERS", node4)) {
        for (const ConfigNode* mp : node4->GetNodes("MODULE_PARAMETER")) {
            Editor::DisplayParameter item3;
            item3.Load(*mp);
            AddUniqueTo(nodeBodyParameters, item3);
        }
    }
    if (node.HasNode("DOCKEDNODEIDS")) {
        const std::vector<std::string> values2 = node.GetNode("DOCKEDNODEIDS")->GetValuesList("nodeID");
        for (size_t num = 0; num < values2.size(); ++num) dockedNodesIDsOnLoad.push_back(Guid::Parse(values2[num]));
    }
    save_detail::TryGetEnum(node, "missionEndOptions", missionEndOptions, MissionEndOptions::Fail);
}

// MENode.cs:1148
void MENode::Save(ConfigNode& node) const {
    node.AddValue("title", title_);
    node.AddValue("id", id_.ToString());
    std::string text = dotnet::Replace(description, "\n", "\\n");
    text = dotnet::Replace(text, "\t", "\\t");
    node.AddValue("description", text);
    if (!category_.empty()) node.AddValue("category", category_);
    node.AddValue("showScreenMessage", showScreenMessage);
    node.AddValue("msgTime", msgTime);
    node.AddValue("editorPosition", editorPosition);
    node.AddValue("isStartNode", isStartNode);
    node.AddValue("isEndNode", isEndNode);
    std::string text2 = dotnet::Replace(message, "\n", "\\n");
    text2 = dotnet::Replace(text2, "\t", "\\t");
    node.AddValue("message", text2);
    node.AddValue("dockedIndex", dockedIndex);
    node.AddValue("isObjective", isObjective);
    node.AddValue("isCatchAll", isCatchAll_);
    node.AddValue("isEvent", isEvent);
    node.AddValue("activateOnceOnly", activateOnceOnly);
    node.AddValue("activatedUT", activatedUT);
    node.AddValue("isLogicNode", isLogicNode_);
    node.AddValue("basicNodeSource", basicNodeSource);
    node.AddValue("basicNodeIconURL", basicNodeIconURL);
    node.AddValue("nodeColor", "#" + ToHtmlStringRGB(nodeColor));
    if (dockParentNode != nullptr) node.AddValue("dockParentNodeID", dockParentNode->id().ToString());
    if (!fromNodes.empty()) {
        ConfigNode* configNode = node.AddNode("FROMNODES");
        for (size_t i = 0; i < fromNodes.size(); ++i) {
            MENode* mENode = fromNodes[i];
            ConfigNode* configNode2 = configNode->AddNode("FROMNODE");
            configNode2->AddValue("nodeID", mENode->id().ToString());
            if (!(mENode->guiNode != nullptr)) continue;
            auto connectors = mENode->guiNode->OutputConnectors();
            int j = 0;
            for (int count = static_cast<int>(connectors.size()); j < count; ++j) {
                if (connectors[static_cast<size_t>(j)]->IsConnectedToNode(guiNode)) {
                    configNode2->AddValue("connectorColor", "#" + ToHtmlStringRGB(connectors[static_cast<size_t>(j)]->LineColour()));
                    break;
                }
            }
        }
    }
    if (!toNodes.empty()) {
        ConfigNode* configNode3 = node.AddNode("TONODEIDS");
        for (size_t k = 0; k < toNodes.size(); ++k) {
            if (!toNodes[k]->IsOrphanNode()) configNode3->AddValue("nodeID", toNodes[k]->id().ToString());
        }
    }
    if (!testGroups.empty()) {
        ConfigNode* configNode4 = node.AddNode("TESTGROUPS");
        for (size_t l = 0; l < testGroups.size(); ++l) {
            ConfigNode* node2 = configNode4->AddNode("TESTGROUP");
            testGroups[l]->Save(*node2);
        }
    }
    if (!actionModules.empty()) {
        ConfigNode* configNode5 = node.AddNode("ACTIONS");
        for (size_t m = 0; m < actionModules.size(); ++m) {
            ConfigNode* node3 = configNode5->AddNode("ACTIONMODULE");
            actionModules[m]->Save(*node3);
        }
    }
    if (!nodeBodyParameters.empty()) {
        ConfigNode* configNode6 = node.AddNode("NODEBODY_PARAMETERS");
        for (size_t n = 0; n < nodeBodyParameters.size(); ++n) {
            ConfigNode* configNode7 = configNode6->AddNode("MODULE_PARAMETER");
            configNode7->AddValue("module", nodeBodyParameters[n].module);
            configNode7->AddValue("parameter", nodeBodyParameters[n].parameter);
        }
    }
    if (!dockedNodes.empty()) {
        ConfigNode* configNode8 = node.AddNode("DOCKEDNODEIDS");
        for (size_t num = 0; num < dockedNodes.size(); ++num) configNode8->AddValue("nodeID", dockedNodes[num]->id().ToString());
    }
    node.AddValue("missionEndOptions", dotnet::EnumToString(missionEndOptions));
}

}  // namespace ksp::Expansions::Missions
