// WP-55: Assembly-CSharp/Expansions/Missions/Tests/*.cs. See mission_tests.hpp and node_library.hpp.
// Float rule (README "Mono float32 rule"): managed float expressions are f32store(fx(a) op fx(b)).
// Number text: `x.ToString("0")` / `("0.00")` / `("F1")` are rendered with dotnet::FormatDoubleF / FormatSingleF (UNVERIFIED for exact decimal
// ties and negative values that round to zero, which the Mono custom-format engine may print differently; the gate avoids them).
#include "ksp/missions/mission_tests.hpp"

#include <algorithm>
#include <cmath>

#include "ksp/career/funding.hpp"
#include "ksp/career/scenario_misc.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "../save/save_util.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_awards.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/discovery_info.hpp"

namespace ksp::Expansions::Missions::Tests {

using KSP::Localization::Localizer;

namespace {
double epsilon = 1E-15;  // TestMissionScore.cs:12 / TestMissionFunds.cs:12 / TestVesselResource.cs:20 / TestFundsRecovery.cs:17

std::string F0(double v) { return dotnet::FormatDoubleF(v, 0); }
std::string F0(float v) { return dotnet::FormatSingleF(v, 0); }
std::string F2(double v) { return dotnet::FormatDoubleF(v, 2); }
std::string G(double v) { return dotnet::FormatDoubleG(v); }
std::string I(int v) { return dotnet::FormatInt64(v); }

// GetNodeBodyParameterString of a LessGreaterOnly operator: "< " / "> " prefix + the compact date delta (TestMETTime.cs:46 and copies)
std::string TimeBody(TestComparisonLessGreaterOnly op, double time) {
    return std::string(op == TestComparisonLessGreaterOnly::LessThan ? "< " : "> ") + KSPUtil::PrintDateDeltaCompact(time, true, true);
}

std::string Tag(const char* t) { return Localizer::Format(t); }

Vessel* PersistentVessel(uint32_t id) { return AwardEnvironment::VesselByPersistentId(id); }
}  // namespace

// "<" "<=" "=" ">=" ">" (TestVesselPartCount.cs:84 and copies)
std::string OperatorString(TestComparisonOperator op) {
    switch (op) {
        case TestComparisonOperator::LessThan: return "<";
        case TestComparisonOperator::LessThanorEqual: return "<=";
        case TestComparisonOperator::Equal: return "=";
        case TestComparisonOperator::GreaterThanorEqual: return ">=";
        case TestComparisonOperator::GreaterThan: return ">";
    }
    return "";
}

// ---- TestEmptyTrue / TestEmptyFalse -----------------------------------------------------------------------------------------

// TestEmptyTrue.cs:7 Awake
TestEmptyTrue::TestEmptyTrue() : TestModule(kTypeName) { title = Tag("#autoLOC_8003077"); }
// TestEmptyTrue.cs:13
bool TestEmptyTrue::Test() { return true; }
// TestEmptyTrue.cs:18
std::string TestEmptyTrue::GetInfo() { return Tag("#autoLOC_8003078"); }

// TestEmptyFalse.cs:7 Awake
TestEmptyFalse::TestEmptyFalse() : TestModule(kTypeName) { title = Tag("#autoLOC_8003075"); }
// TestEmptyFalse.cs:13
bool TestEmptyFalse::Test() { return false; }
// TestEmptyFalse.cs:18
std::string TestEmptyFalse::GetInfo() { return Tag("#autoLOC_8003076"); }

// ---- TestVessel --------------------------------------------------------------------------------------------------------------

// TestVessel.cs:23 Awake
TestVessel::TestVessel(std::string typeName) : TestModule(std::move(typeName)) {}

// TestVessel.cs:29
void TestVessel::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    if (vesselID == oldId) vesselID = newId;
}

// TestVessel.cs:37
void TestVessel::OnVesselDocking(uint32_t oldId, uint32_t newId) {
    (void)newId;
    if (oldId == vesselID) {
        Vessel* v = nullptr;
        if (AwardEnvironment::findVessel) {
            AwardEnvironment::findVessel(oldId, v);
        } else {
            v = AwardEnvironment::VesselByPersistentId(oldId);
        }
        if (v != nullptr) oldRootPart = v->rootPart->fields.persistentId;
    }
}

// TestVessel.cs:50
void TestVessel::OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) {
    if (oldVessel->persistentId == vesselID || (newVessel->persistentId == vesselID && oldRootPart != 0)) {
        // FlightGlobals.FindLoadedPart(oldRootPart, out partout) -> partout.vessel.persistentId, else FindUnloadedPart -> pVesselRef
        uint32_t vesselOfPart = 0;
        MissionsEnvironment::PartLookup r = MissionsEnvironment::PartLookup::NotFound;
        if (MissionsEnvironment::partVessel) r = MissionsEnvironment::partVessel(oldRootPart, true, vesselOfPart);
        if (r != MissionsEnvironment::PartLookup::NotFound) {
            if (r == MissionsEnvironment::PartLookup::FoundNoVessel) throw NullReferenceException();
            vesselID = vesselOfPart;
        } else {
            if (MissionsEnvironment::partVessel) r = MissionsEnvironment::partVessel(oldRootPart, false, vesselOfPart);
            if (r != MissionsEnvironment::PartLookup::NotFound) {
                if (r == MissionsEnvironment::PartLookup::FoundNoVessel) throw NullReferenceException();
                vesselID = vesselOfPart;
            }
        }
    }
}

// TestVessel.cs:67
std::string TestVessel::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (node() != nullptr) node()->UpdateInfoOnRefresh = true;
    if (field.name == "vesselID" && node()->mission != nullptr) {
        Mission* mission = node()->mission;
        VesselSituation* vesselSituation = mission->GetVesselSituationByVesselID(vesselID);
        std::string vesselName;
        bool flag = false;
        if (vesselSituation != nullptr) {
            vesselName = vesselSituation->vesselName;
        } else {
            // Mission.UpdateFromMappedVesselIDs (Mission.cs:1375)
            const int num = MappedVessels::IndexMappedVesselId(mission->mappedVessels, vesselID);
            if (num != -1) {
                vesselName = mission->mappedVessels[static_cast<size_t>(num)].partVesselName;
                vesselSituation = mission->GetVesselSituationByVesselID(mission->mappedVessels[static_cast<size_t>(num)].currentVesselPersistentId);
                flag = true;
            }
        }
        if (vesselSituation == nullptr && !flag) {
            return Localizer::Format("#autoLOC_8000069", {useActiveVessel ? Tag("#autoLOC_8004217") : Tag("#autoLOC_8001004")});
        }
        // HighLogic.LoadedSceneIsMissionBuilder: the MissionEditorLogic incompatible-craft test is the editor's (MissionCraft: slice 2)
        std::string text = Localizer::Format("#autoLOC_8000069", {vesselName});
        if (flag && vesselSituation != nullptr) {
            text = text + "\n" + Localizer::Format("#autoLOC_6002491", {vesselSituation->vesselName}) + "\n";
        }
        return text;
    }
    if (field.name == "resourceName") {
        PartResourceLibrary* lib = PartResourceLibrary::Instance();
        if (lib == nullptr) throw NullReferenceException();
        const PartResourceDefinition* definition = lib->GetDefinition(field.value);
        if (definition == nullptr) throw NullReferenceException();
        return Localizer::Format("#autoLOC_8004190", {field.guiName, definition->displayName()});
    }
    return TestModule::GetNodeBodyParameterString(field);
}

// TestVessel.cs:113
bool TestVessel::Test() {
    if (vesselID == 0) {
        vessel = AwardEnvironment::ActiveVessel();
    } else {
        uint32_t num = node()->mission->CurrentVesselID(node(), vesselID);
        if (Vessel* found = PersistentVessel(num); found != nullptr) {
            vessel = found;
        } else if (vessel != nullptr) {
            if (std::find(missingVessels.begin(), missingVessels.end(), num) == missingVessels.end()) {
                missingVessels.push_back(num);
                log::Error("[TestVessel] Unable to find VesselID (" + std::to_string(num) + ") from Node (" + node()->Title() + ") in FlightGlobals.");
            }
            vessel = nullptr;
        }
    }
    return true;
}

// TestVessel.cs:144
void TestVessel::Save(ConfigNode& node) const {
    TestModule::Save(node);
    node.AddValue("vesselID", vesselID);
    if (oldRootPart != 0) node.AddValue("oldRootPart", oldRootPart);
}

// TestVessel.cs:154
void TestVessel::Load(const ConfigNode& node) {
    TestModule::Load(node);
    node.TryGetValue("vesselID", vesselID);
    node.TryGetValue("oldRootPart", oldRootPart);
}

// the common `GetScoreModifier` body for ScoreModule_Resource (TestVesselMass.cs:78 and copies)
Vessel* TestVessel::ScoreModifierVessel(const char* className) {
    if (vesselID == 0) return AwardEnvironment::ActiveVessel();
    uint32_t num = node()->mission->CurrentVesselID(node(), vesselID);
    if (Vessel* found = PersistentVessel(num); found != nullptr) return found;
    log::Error(std::string("[") + className + "] Unable to find VesselID (" + std::to_string(num) + ") for Score Modifier.");
    return nullptr;
}

// ---- TestVesselActive --------------------------------------------------------------------------------------------------------

// TestVesselActive.cs:9 Awake
TestVesselActive::TestVesselActive() : TestVessel(kTypeName) {
    title = "#autoLOC_8004217";
    useActiveVessel = true;
}
// TestVesselActive.cs:16
bool TestVesselActive::Test() {
    TestVessel::Test();
    return vessel == AwardEnvironment::ActiveVessel();
}
// TestVesselActive.cs:22
std::string TestVesselActive::GetInfo() { return Tag("#autoLOC_8006116"); }

// ---- TestMETTime -------------------------------------------------------------------------------------------------------------

// TestMETTime.cs:20 Awake
TestMETTime::TestMETTime() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8004046");
    time = 300.0;
    useActiveVessel = true;
}
// TestMETTime.cs:28
bool TestMETTime::Test() {
    TestVessel::Test();
    if (vessel != nullptr) {
        switch (comparisonOperator) {
            default: return false;
            case TestComparisonLessGreaterOnly::GreaterThan: return vessel->missionTime > time;
            case TestComparisonLessGreaterOnly::LessThan: return vessel->missionTime < time;
        }
    }
    return false;
}
// TestMETTime.cs:46
std::string TestMETTime::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "time") return TimeBody(comparisonOperator, time);
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestMETTime.cs:55
std::string TestMETTime::GetInfo() { return Tag("#autoLOC_8004036"); }
// TestMETTime.cs:60
void TestMETTime::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("time", time);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestMETTime.cs:67
void TestMETTime::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    node.TryGetValue("time", time);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterOnly::LessThan);
}
// TestMETTime.cs:74
ScoreModifier TestMETTime::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = AwardEnvironment::ActiveVessel();
    return r;
}

// ---- TestMissionTime ---------------------------------------------------------------------------------------------------------

// TestMissionTime.cs:22 Awake
TestMissionTime::TestMissionTime() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000080");
    time = 300.0;
}
// TestMissionTime.cs:29
bool TestMissionTime::Test() {
    missionTime = node()->mission->startedUT + time;
    switch (comparisonOperator) {
        default: return false;
        case TestComparisonLessGreaterOnly::GreaterThan: return MissionsEnvironment::UniversalTime() > missionTime;
        case TestComparisonLessGreaterOnly::LessThan: return MissionsEnvironment::UniversalTime() < missionTime;
    }
}
// TestMissionTime.cs:43
std::string TestMissionTime::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "time") return TimeBody(comparisonOperator, time);
    return TestModule::GetNodeBodyParameterString(field);
}
// TestMissionTime.cs:52
std::string TestMissionTime::GetInfo() { return Tag("#autoLOC_8004037"); }
// TestMissionTime.cs:57
void TestMissionTime::Save(ConfigNode& node) const {
    TestModule::Save(node);
    node.AddValue("time", time);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestMissionTime.cs:64
void TestMissionTime::Load(const ConfigNode& node) {
    TestModule::Load(node);
    node.TryGetValue("time", time);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterOnly::LessThan);
}
// TestMissionTime.cs:71
ScoreModifier TestMissionTime::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = AwardEnvironment::ActiveVessel();
    return r;
}

// ---- TestTimeSinceNode -------------------------------------------------------------------------------------------------------

// TestTimeSinceNode.cs:25 Awake
TestTimeSinceNode::TestTimeSinceNode() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000099");
    time = 300.0;
}
// TestTimeSinceNode.cs:32
void TestTimeSinceNode::Initialize(TestGroup* testGroup, const std::string& newTitle) {
    TestModule::Initialize(testGroup, newTitle.empty() ? title : newTitle);
    if (testGroup->node() != nullptr && testGroup->node()->mission != nullptr && testGroup->node()->mission->startNode() != nullptr) {
        nodeID = testGroup->node()->mission->startNode()->id();
    }
}
// TestTimeSinceNode.cs:41
bool TestTimeSinceNode::Test() {
    if (!node()->mission->nodes.ContainsKey(nodeID)) return false;
    nodeCompareTime = node()->mission->nodes[nodeID]->activatedUT + time;
    if (node() != nullptr && node()->mission->nodes[nodeID]->HasBeenActivated()) {
        switch (comparisonOperator) {
            default: return false;
            case TestComparisonLessGreaterOnly::GreaterThan: return MissionsEnvironment::UniversalTime() > nodeCompareTime;
            case TestComparisonLessGreaterOnly::LessThan: return MissionsEnvironment::UniversalTime() < nodeCompareTime;
        }
    }
    return false;
}
// TestTimeSinceNode.cs:63
std::string TestTimeSinceNode::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "nodeID") {
        if (node()->mission->nodes.ContainsKey(nodeID)) {
            std::string objectiveString = node()->mission->GetNodeById(nodeID)->ObjectiveString();
            std::string text = Localizer::Format(objectiveString);
            if (!text.empty()) return text;
            return objectiveString;
        }
        return Tag("#autoLOC_8007310");
    }
    if (field.name == "time") return TimeBody(comparisonOperator, time);
    return TestModule::GetNodeBodyParameterString(field);
}
// TestTimeSinceNode.cs:86
std::string TestTimeSinceNode::GetInfo() { return Tag("#autoLOC_8004038"); }
// TestTimeSinceNode.cs:91
void TestTimeSinceNode::Save(ConfigNode& node) const {
    TestModule::Save(node);
    node.AddValue("nodeID", nodeID.ToString());
    node.AddValue("time", time);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestTimeSinceNode.cs:99
void TestTimeSinceNode::Load(const ConfigNode& node) {
    TestModule::Load(node);
    if (nodeID == Guid::Empty() && testGroup()->node() != nullptr && testGroup()->node()->mission != nullptr &&
        testGroup()->node()->mission->startNode() != nullptr) {
        nodeID = testGroup()->node()->mission->startNode()->id();
    }
    node.TryGetValue("nodeID", nodeID);
    node.TryGetValue("time", time);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterOnly::LessThan);
}
// TestTimeSinceNode.cs:111
void TestTimeSinceNode::RunValidation(Editor::MissionEditorValidator* validator) {
    // MissionEditorLogic.Instance.EditorMission: the editor's mission (the node's own mission in play mode)
    Mission* editorMission = MissionsEnvironment::editorMission ? MissionsEnvironment::editorMission() : node()->mission;
    if (!editorMission->nodes.ContainsKey(nodeID)) validator->AddNodeFail(node(), Tag("#autoLOC_8007311"));
}
// TestTimeSinceNode.cs:119
ScoreModifier TestTimeSinceNode::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = AwardEnvironment::ActiveVessel();
    return r;
}

// ---- TestMissionScore / TestMissionFunds -------------------------------------------------------------------------------------

// TestMissionScore.cs:21 Awake
TestMissionScore::TestMissionScore() : TestModule(kTypeName) { title = Tag("#autoLOC_8000049"); }
// TestMissionScore.cs:27
bool TestMissionScore::Test() {
    if (node() != nullptr) {
        const float currentScore = node()->mission->currentScore;
        switch (comparisonOperator) {
            default: return false;
            case TestComparisonOperator::LessThan: return currentScore < score;
            case TestComparisonOperator::LessThanorEqual: return currentScore <= score;
            case TestComparisonOperator::Equal: return std::fabs(fx(f32store(fx(score) - fx(currentScore)))) < epsilon;
            case TestComparisonOperator::GreaterThanorEqual: return currentScore >= score;
            case TestComparisonOperator::GreaterThan: return currentScore > score;
        }
    }
    return false;
}
// TestMissionScore.cs:50
std::string TestMissionScore::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "score") {
        std::string bodyScore = OperatorString(comparisonOperator);
        bodyScore = Localizer::Format("#autoLOC_8006046", {bodyScore, F0(score)});
        return bodyScore;
    }
    return TestModule::GetNodeBodyParameterString(field);
}
// TestMissionScore.cs:79
std::string TestMissionScore::GetInfo() { return Tag("#autoLOC_8004022"); }
// TestMissionScore.cs:84
void TestMissionScore::Save(ConfigNode& node) const {
    TestModule::Save(node);
    node.AddValue("score", score);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestMissionScore.cs:91
void TestMissionScore::Load(const ConfigNode& node) {
    TestModule::Load(node);
    node.TryGetValue("score", score);
    if (!save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonOperator::Equal)) {
        log::Error("Failed to parse comparisonOperator from " + title);
    }
}
// TestMissionScore.cs:101
ScoreModifier TestMissionScore::GetScoreModifier(const std::string&) { return {}; }

// TestMissionFunds.cs:21 Awake
TestMissionFunds::TestMissionFunds() : TestModule(kTypeName) { title = Tag("#autoLOC_8003129"); }
// TestMissionFunds.cs:27
bool TestMissionFunds::Test() {
    if (Funding::Instance == nullptr) return true;
    if (node() != nullptr) {
        const double current = Funding::Instance->Funds();
        switch (comparisonOperator) {
            default: return false;
            case TestComparisonOperator::LessThan: return current < fx(funds);
            case TestComparisonOperator::LessThanorEqual: return current <= fx(funds);
            case TestComparisonOperator::Equal: return std::fabs(fx(funds) - current) < epsilon;
            case TestComparisonOperator::GreaterThanorEqual: return current >= fx(funds);
            case TestComparisonOperator::GreaterThan: return current > fx(funds);
        }
    }
    return false;
}
// TestMissionFunds.cs:54
std::string TestMissionFunds::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "funds") {
        std::string bodyFunds = OperatorString(comparisonOperator);
        bodyFunds = Localizer::Format("#autoLOC_8003131", {bodyFunds, F0(funds)});
        return bodyFunds;
    }
    return TestModule::GetNodeBodyParameterString(field);
}
// TestMissionFunds.cs:83
std::string TestMissionFunds::GetInfo() { return Tag("#autoLOC_8003130"); }
// TestMissionFunds.cs:88
void TestMissionFunds::Save(ConfigNode& node) const {
    TestModule::Save(node);
    node.AddValue("funds", funds);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestMissionFunds.cs:95
void TestMissionFunds::Load(const ConfigNode& node) {
    TestModule::Load(node);
    node.TryGetValue("funds", funds);
    if (!save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonOperator::Equal)) {
        log::Error("Failed to parse comparisonOperator from " + title);
    }
}
// TestMissionFunds.cs:105
ScoreModifier TestMissionFunds::GetScoreModifier(const std::string&) { return {}; }

// ---- TestAccuracy ------------------------------------------------------------------------------------------------------------

// TestAccuracy.cs:17 Awake
TestAccuracy::TestAccuracy() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8001008");
    accuracy = 0.0;
}
// TestAccuracy.cs:24
bool TestAccuracy::Test() {
    if (scoreableObjective == nullptr && node()->dockParentNode != nullptr && node()->dockParentNode->HasTestModules()) {
        scoreableObjective = dynamic_cast<IScoreableObjective*>(node()->dockParentNode->testGroups[0]->testModules[0].get());
    }
    if (scoreableObjective != nullptr) {
        ScoreModifier scoreModifier = scoreableObjective->GetScoreModifier("ScoreModule_Accuracy");
        double num = scoreModifier.number.has_value() ? *scoreModifier.number * 100.0 : 100.0;
        switch (comparisonOperator) {
            default: return false;
            case TestComparisonLessGreaterOnly::GreaterThan: return num > accuracy;
            case TestComparisonLessGreaterOnly::LessThan: return num < accuracy;
        }
    }
    return false;
}
// TestAccuracy.cs:49
std::string TestAccuracy::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "accuracy") {
        return Localizer::Format("#autoLOC_8001012", {comparisonOperator == TestComparisonLessGreaterOnly::LessThan ? "<" : ">", G(accuracy)});
    }
    return TestModule::GetNodeBodyParameterString(field);
}
// TestAccuracy.cs:58
std::string TestAccuracy::GetInfo() { return Tag("#autoLOC_8001009"); }
// TestAccuracy.cs:63
void TestAccuracy::Save(ConfigNode& node) const {
    TestModule::Save(node);
    node.AddValue("accuracy", accuracy);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestAccuracy.cs:70
void TestAccuracy::Load(const ConfigNode& node) {
    TestModule::Load(node);
    node.TryGetValue("accuracy", accuracy);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterOnly::GreaterThan);
}

// ---- TestGameEvent -----------------------------------------------------------------------------------------------------------

// TestGameEvent.cs:12 Awake
TestGameEvent::TestGameEvent() : TestModule(kTypeName) { title = "Game Event Test"; }
// kspcore: the C# delegate keeps the destroyed component alive; the C++ listener must not outlive the object
TestGameEvent::~TestGameEvent() {
    if (eventSuscribed) GameEvents::onStageActivate.Remove(this, &TestGameEvent::EventCallback_StageSeparation);
}
// TestGameEvent.cs:18
bool TestGameEvent::Test() {
    if (!eventSuscribed) SuscribeToEvent();
    return eventTriggered;
}
// TestGameEvent.cs:27
void TestGameEvent::Save(ConfigNode& node) const {
    TestModule::Save(node);
    node.AddValue("eventTriggered", eventTriggered);
}
// TestGameEvent.cs:33
void TestGameEvent::Load(const ConfigNode& node) {
    TestModule::Load(node);
    node.TryGetValue("eventTriggered", eventTriggered);
}
// TestGameEvent.cs:39
void TestGameEvent::SuscribeToEvent() {
    GameEvents::onStageActivate.Add(this, &TestGameEvent::EventCallback_StageSeparation);
    eventSuscribed = true;
}
// TestGameEvent.cs:45
void TestGameEvent::EventCallback_StageSeparation(int) { eventTriggered = true; }
// TestGameEvent.cs:50
ScoreModifier TestGameEvent::GetScoreModifier(const std::string&) { return {}; }

// ---- TestVesselMass ----------------------------------------------------------------------------------------------------------

// TestVesselMass.cs:23 Awake
TestVesselMass::TestVesselMass() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000113");
    mass = 25.0;
    useActiveVessel = true;
}
// TestVesselMass.cs:31
bool TestVesselMass::Test() {
    TestVessel::Test();
    if (vessel != nullptr) {
        massCompare = fx(vessel->GetTotalMass());
        switch (comparisonOperator) {
            default: return false;
            case TestComparisonLessGreaterOnly::GreaterThan: return massCompare > mass;
            case TestComparisonLessGreaterOnly::LessThan: return massCompare < mass;
        }
    }
    return false;
}
// TestVesselMass.cs:50
std::string TestVesselMass::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "mass") {
        return Localizer::Format("#autoLOC_8100175", {comparisonOperator == TestComparisonLessGreaterOnly::LessThan ? "<" : ">", F2(mass)});
    }
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestVesselMass.cs:59
std::string TestVesselMass::GetInfo() { return Tag("#autoLOC_8004042"); }
// TestVesselMass.cs:64
void TestVesselMass::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("mass", mass);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestVesselMass.cs:71
void TestVesselMass::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    node.TryGetValue("mass", mass);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterOnly::GreaterThan);
}
// TestVesselMass.cs:78
ScoreModifier TestVesselMass::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestVesselMass");
    return r;
}

// ---- TestVesselPartCount -----------------------------------------------------------------------------------------------------

// TestVesselPartCount.cs:25 Awake
TestVesselPartCount::TestVesselPartCount() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8100293");
    parts = 1;
    useActiveVessel = true;
}
// TestVesselPartCount.cs:33
bool TestVesselPartCount::Test() {
    TestVessel::Test();
    if (vessel != nullptr) {
        if (vessel->loaded) {
            partsCompare = static_cast<int>(vessel->Parts().size());
        } else {
            partsCompare = static_cast<int>(vessel->protoVessel->protoPartSnapshots.size());
        }
        switch (comparisonOperator) {
            default: return false;
            case TestComparisonOperator::LessThan: return partsCompare < parts;
            case TestComparisonOperator::LessThanorEqual: return partsCompare <= parts;
            case TestComparisonOperator::Equal: return partsCompare == parts;
            case TestComparisonOperator::GreaterThanorEqual: return partsCompare >= parts;
            case TestComparisonOperator::GreaterThan: return partsCompare > parts;
        }
    }
    CheckForUnloadedVessel();
    return false;
}
// TestVesselPartCount.cs:66 (C# bug kept: the loop runs to FlightGlobals.Vessels.Count but indexes VesselsUnloaded)
void TestVesselPartCount::CheckForUnloadedVessel() {
    if (FlightRegistry::VesselsUnloaded.size() <= 0) return;
    for (size_t i = 0; i < FlightRegistry::Vessels.size(); ++i) {
        if (i >= FlightRegistry::VesselsUnloaded.size()) throw ArgumentException("Index was out of range (FlightGlobals.VesselsUnloaded)");
        if (FlightRegistry::VesselsUnloaded[i]->persistentId == vesselID) vessel = FlightRegistry::VesselsUnloaded[i];
    }
}
// TestVesselPartCount.cs:81
std::string TestVesselPartCount::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "parts") {
        return Localizer::Format("#autoLOC_8100154", {field.guiName, OperatorString(comparisonOperator), I(parts)});
    }
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestVesselPartCount.cs:109
std::string TestVesselPartCount::GetInfo() { return Tag("#autoLOC_8000305"); }
// TestVesselPartCount.cs:114
void TestVesselPartCount::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("numberOfParts", parts);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestVesselPartCount.cs:121
void TestVesselPartCount::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    node.TryGetValue("numberOfParts", parts);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonOperator::Equal);
}
// TestVesselPartCount.cs:128
ScoreModifier TestVesselPartCount::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestVesselPartCount");
    return r;
}

// ---- TestVesselCrewCount -----------------------------------------------------------------------------------------------------

// TestVesselCrewCount.cs:31 Awake
TestVesselCrewCount::TestVesselCrewCount() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000106");
    traitName = "";
    useActiveVessel = true;
}
// TestVesselCrewCount.cs:40
bool TestVesselCrewCount::Test() {
    TestVessel::Test();
    if (vessel != nullptr) {
        currentCrewCounter = 0;
        if (traitName.empty()) {
            currentCrewCounter = static_cast<int>(vessel->GetVesselCrew().size());
        } else {
            const auto& pcmList = vessel->GetVesselCrew();
            for (size_t i = 0; i < pcmList.size(); ++i) {
                if ((pcmList[i]->trait.has_value() && *pcmList[i]->trait == traitName) || traitName == "Any Role") ++currentCrewCounter;
            }
        }
        switch (comparisonOperator) {
            default: return false;
            case TestComparisonOperator::LessThan: return currentCrewCounter < crewAmount;
            case TestComparisonOperator::LessThanorEqual: return currentCrewCounter <= crewAmount;
            case TestComparisonOperator::Equal: return currentCrewCounter == crewAmount;
            case TestComparisonOperator::GreaterThanorEqual: return currentCrewCounter >= crewAmount;
            case TestComparisonOperator::GreaterThan: return currentCrewCounter > crewAmount;
        }
    }
    return false;
}
// TestVesselCrewCount.cs:91
std::string TestVesselCrewCount::GetInfo() { return Tag("#autoLOC_8004041"); }
// TestVesselCrewCount.cs:96
std::string TestVesselCrewCount::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "traitName") {
        if (field.value != "" && field.value != "Any Role") {
            // KerbalRoster.GetLocalizedExperienceTraitName(name): the localized trait title (host seam)
            std::string traitTitle = NodeLibraryEnvironment::localizedTraitName ? NodeLibraryEnvironment::localizedTraitName(field.value) : field.value;
            return Localizer::Format("#autoLOC_8004190", {field.guiName, traitTitle});
        }
        return Localizer::Format("#autoLOC_8004190", {field.guiName, "#autoLOC_8002020"});
    }
    if (field.name == "crewAmount") {
        return Localizer::Format("#autoLOC_8100154", {field.guiName, OperatorString(comparisonOperator), I(crewAmount)});
    }
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestVesselCrewCount.cs:132
void TestVesselCrewCount::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("crewAmount", crewAmount);
    node.AddValue("traitName", traitName);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestVesselCrewCount.cs:140
void TestVesselCrewCount::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    node.TryGetValue("crewAmount", crewAmount);
    node.TryGetValue("traitName", traitName);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonOperator::Equal);
}
// TestVesselCrewCount.cs:148
ScoreModifier TestVesselCrewCount::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestVesselCrewCount");
    return r;
}

// ---- TestVesselAltitude ------------------------------------------------------------------------------------------------------

// TestVesselAltitude.cs:26 Awake
TestVesselAltitude::TestVesselAltitude() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000092");
    altitude = 0.0;
    useRadarAltimiter = false;
    useActiveVessel = true;
}
// TestVesselAltitude.cs:35
bool TestVesselAltitude::Test() {
    TestVessel::Test();
    if (vessel != nullptr) {
        altitudeCompare = useRadarAltimiter ? vessel->radarAltitude : vessel->altitude;
        switch (comparisonOperator) {
            default: return false;
            case TestComparisonLessGreaterOnly::GreaterThan: return altitudeCompare > altitude;
            case TestComparisonLessGreaterOnly::LessThan: return altitudeCompare < altitude;
        }
    }
    return false;
}
// TestVesselAltitude.cs:61
std::string TestVesselAltitude::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "altitude") {
        return Localizer::Format("#autoLOC_8100035", {comparisonOperator == TestComparisonLessGreaterOnly::LessThan ? "<" : ">", G(altitude)});
    }
    if (field.name == "useRadarAltimiter") {
        if (!useRadarAltimiter) return Tag("#autoLOC_8100167");
        return Tag("#autoLOC_8100036");
    }
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestVesselAltitude.cs:78
std::string TestVesselAltitude::GetInfo() { return Tag("#autoLOC_8004040"); }
// TestVesselAltitude.cs:83
void TestVesselAltitude::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("altitude", altitude);
    node.AddValue("useRadarAltimiter", useRadarAltimiter);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestVesselAltitude.cs:91
void TestVesselAltitude::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    node.TryGetValue("altitude", altitude);
    node.TryGetValue("useRadarAltimiter", useRadarAltimiter);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterOnly::GreaterThan);
}
// TestVesselAltitude.cs:99
ScoreModifier TestVesselAltitude::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestVesselAltitude");
    return r;
}

// ---- TestVesselActiveCount ---------------------------------------------------------------------------------------------------

// TestVesselActiveCount.cs:36 Awake
TestVesselActiveCount::TestVesselActiveCount() : TestModule(kTypeName) { title = "#autoLOC_8001038"; }
// kspcore: remove every listener the (C# bug) Cleared() added
TestVesselActiveCount::~TestVesselActiveCount() {
    for (; addSubscriptions > 0; --addSubscriptions) GameEvents::onFlightGlobalsAddVessel.Remove(this, &TestVesselActiveCount::OnAddVessel);
    for (; removeSubscriptions > 0; --removeSubscriptions) GameEvents::onFlightGlobalsRemoveVessel.Remove(this, &TestVesselActiveCount::OnRemoveVessel);
}
// TestVesselActiveCount.cs:42
void TestVesselActiveCount::Initialized() {
    TestModule::Initialized();
    countCompare = 0;
    GameEvents::onFlightGlobalsAddVessel.Add(this, &TestVesselActiveCount::OnAddVessel);
    ++addSubscriptions;
    GameEvents::onFlightGlobalsRemoveVessel.Add(this, &TestVesselActiveCount::OnRemoveVessel);
    ++removeSubscriptions;
    std::vector<Vessel*> vessels = AwardEnvironment::Vessels();
    int i = 0;
    for (int count = static_cast<int>(vessels.size()); i < count; i++) UpdateVesselCount(vessels[static_cast<size_t>(i)], 1);
}
// TestVesselActiveCount.cs:55 (C# bug kept: Cleared() ADDS the handlers again instead of removing them)
void TestVesselActiveCount::Cleared() {
    TestModule::Cleared();
    GameEvents::onFlightGlobalsAddVessel.Add(this, &TestVesselActiveCount::OnAddVessel);
    ++addSubscriptions;
    GameEvents::onFlightGlobalsRemoveVessel.Add(this, &TestVesselActiveCount::OnRemoveVessel);
    ++removeSubscriptions;
}
// TestVesselActiveCount.cs:62
void TestVesselActiveCount::OnAddVessel(Vessel* v) { UpdateVesselCount(v, 1); }
// TestVesselActiveCount.cs:67
void TestVesselActiveCount::OnRemoveVessel(Vessel* v) { UpdateVesselCount(v, -1); }
// TestVesselActiveCount.cs:72
void TestVesselActiveCount::UpdateVesselCount(Vessel* v, int value) {
    if (v->vesselType == ksp::VesselType::EVA || v->vesselType == ksp::VesselType::Flag || v->vesselType == ksp::VesselType::DeployedSciencePart ||
        v->vesselType == ksp::VesselType::DeployedScienceController) {
        return;
    }
    switch (vesselType) {
        case VesselType::All: countCompare += value; break;
        case VesselType::PlayerControlled:
            if (v->GetDiscoveryInfo().Level() == DiscoveryLevels::Owned) countCompare += value;
            break;
        case VesselType::Asteroids:
            if (v->FindPartModuleImplementingClass("ModuleAsteroid") != nullptr) countCompare += value;
            break;
        case VesselType::Comets:
            if (v->FindPartModuleImplementingClass("ModuleComet") != nullptr) countCompare += value;
            break;
        case VesselType::Unowned:
            if (v->GetDiscoveryInfo().Level() == DiscoveryLevels::Unowned) countCompare += value;
            break;
    }
}
// TestVesselActiveCount.cs:110
bool TestVesselActiveCount::Test() {
    TestModule::Test();
    switch (comparisonOperator) {
        default: return false;
        case TestComparisonOperator::LessThan: return countCompare < vesselCount;
        case TestComparisonOperator::LessThanorEqual: return countCompare <= vesselCount;
        case TestComparisonOperator::Equal: return countCompare == vesselCount;
        case TestComparisonOperator::GreaterThanorEqual: return countCompare >= vesselCount;
        case TestComparisonOperator::GreaterThan: return countCompare > vesselCount;
    }
}
// TestVesselActiveCount.cs:130
std::string TestVesselActiveCount::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "vesselCount") {
        return Localizer::Format("#autoLOC_8100154", {field.guiName, OperatorString(comparisonOperator), I(vesselCount)});
    }
    return TestModule::GetNodeBodyParameterString(field);
}
// TestVesselActiveCount.cs:158
std::string TestVesselActiveCount::GetInfo() { return Tag("#autoLOC_8001039"); }
// TestVesselActiveCount.cs:163
void TestVesselActiveCount::Save(ConfigNode& node) const {
    TestModule::Save(node);
    node.AddValue("vesselType", dotnet::EnumToString(vesselType));
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
    node.AddValue("vesselCount", vesselCount);
}
// TestVesselActiveCount.cs:171
void TestVesselActiveCount::Load(const ConfigNode& node) {
    TestModule::Load(node);
    save_detail::TryGetEnum(node, "vesselType", vesselType, VesselType::All);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonOperator::GreaterThan);
    node.TryGetValue("vesselCount", vesselCount);
}

// ---- TestVesselResource ------------------------------------------------------------------------------------------------------

// TestVesselResource.cs:29 Awake
TestVesselResource::TestVesselResource() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000085");
    useActiveVessel = true;
}
// TestVesselResource.cs:36
bool TestVesselResource::Test() {
    TestVessel::Test();
    if (vessel != nullptr) {
        PartResourceLibrary* lib = PartResourceLibrary::Instance();
        if (lib == nullptr) throw NullReferenceException();
        const PartResourceDefinition* definition = lib->GetDefinition(resourceName);
        if (definition == nullptr) throw NullReferenceException();
        if (definition->id() != -1) {
            if (vessel->loaded) {
                // vessel.GetConnectedResourceTotals(definition.id, out current, out max): the total through the AwardEnvironment seam
                // (maxResourceAmount is never read)
                AwardEnvironment::ConnectedResourceTotal(vessel, resourceName, currentResourceAmount);
            } else {
                currentResourceAmount = 0.0;
                maxResourceAmount = 0.0;
                for (size_t i = 0; i < vessel->protoVessel->protoPartSnapshots.size(); ++i) {
                    ProtoPartSnapshot* protoPartSnapshot = vessel->protoVessel->protoPartSnapshots[i].get();
                    for (size_t j = 0; j < protoPartSnapshot->resources.size(); ++j) {
                        const auto& res = protoPartSnapshot->resources[j];
                        if (res->resourceName.has_value() && *res->resourceName == resourceName) {
                            currentResourceAmount += res->amount;
                            maxResourceAmount += res->maxAmount;
                        }
                    }
                }
            }
            switch (comparisonOperator) {
                default: return false;
                case TestComparisonOperator::LessThan: return currentResourceAmount < resourceAmount;
                case TestComparisonOperator::LessThanorEqual: return currentResourceAmount <= resourceAmount;
                case TestComparisonOperator::Equal: return std::fabs(currentResourceAmount - resourceAmount) < epsilon;
                case TestComparisonOperator::GreaterThanorEqual: return currentResourceAmount >= resourceAmount;
                case TestComparisonOperator::GreaterThan: return currentResourceAmount > resourceAmount;
            }
        }
        return false;
    }
    return false;
}
// TestVesselResource.cs:98
std::string TestVesselResource::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "resourceAmount") {
        switch (comparisonOperator) {
            case TestComparisonOperator::LessThan: resourceDisplay = "< "; break;
            case TestComparisonOperator::LessThanorEqual: resourceDisplay = "<= "; break;
            case TestComparisonOperator::Equal: resourceDisplay = "= "; break;
            case TestComparisonOperator::GreaterThanorEqual: resourceDisplay = ">= "; break;
            default: resourceDisplay = "> "; break;
        }
        if (resourceAmount < 1000.0) return resourceDisplay + dotnet::FormatDoubleF(resourceAmount, 1);
        return resourceDisplay + dotnet::FormatDoubleF(resourceAmount, 0);
    }
    if (field.name == "resourceName") {
        PartResourceLibrary* lib = PartResourceLibrary::Instance();
        if (lib == nullptr) throw NullReferenceException();
        const PartResourceDefinition* definition = lib->GetDefinition(resourceName);
        if (definition == nullptr) throw NullReferenceException();
        if (definition->id() != -1) return definition->displayName();
        return field.value;  // MissionsUtils.GetFieldValueForDisplay(field)
    }
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestVesselResource.cs:138
std::string TestVesselResource::GetInfo() { return Tag("#autoLOC_8004028"); }
// TestVesselResource.cs:143
void TestVesselResource::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("resourceName", resourceName);
    node.AddValue("resourceAmount", resourceAmount);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestVesselResource.cs:151
void TestVesselResource::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    node.TryGetValue("resourceName", resourceName);
    node.TryGetValue("resourceAmount", resourceAmount);
    if (!save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonOperator::Equal)) {
        log::Error("Failed to parse comparisonOperator from " + title);
    }
}
// TestVesselResource.cs:162
ScoreModifier TestVesselResource::GetScoreModifier(const std::string&) { return {}; }

// ---- TestVesselStage / TestVesselStageActivated ------------------------------------------------------------------------------

// TestVesselStage.cs:23 Awake
TestVesselStage::TestVesselStage() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000124");
    useActiveVessel = true;
}
// TestVesselStage.cs:30
bool TestVesselStage::Test() {
    TestVessel::Test();
    if (vessel != nullptr) {
        const int currentStage = vessel->currentStage;
        switch (comparisonOperator) {
            default: return false;
            case TestComparisonOperator::LessThan: return currentStage < stage;
            case TestComparisonOperator::LessThanorEqual: return currentStage <= stage;
            case TestComparisonOperator::Equal: return currentStage == stage;
            case TestComparisonOperator::GreaterThanorEqual: return currentStage >= stage;
            case TestComparisonOperator::GreaterThan: return currentStage > stage;
        }
    }
    return false;
}
// TestVesselStage.cs:55
std::string TestVesselStage::GetInfo() { return Tag("#autoLOC_8004043"); }
// TestVesselStage.cs:60
std::string TestVesselStage::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "stage") {
        return Localizer::Format("#autoLOC_8100154", {field.guiName, OperatorString(comparisonOperator), I(stage)});
    }
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestVesselStage.cs:88
void TestVesselStage::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("stage", stage);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
}
// TestVesselStage.cs:95
void TestVesselStage::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    node.TryGetValue("stage", stage);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonOperator::GreaterThan);
}
// TestVesselStage.cs:102
ScoreModifier TestVesselStage::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestVesselStage");
    return r;
}

// TestVesselStageActivated.cs:20 Awake
TestVesselStageActivated::TestVesselStageActivated() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000129");
    useActiveVessel = true;
}
// kspcore: the listener must not outlive the object
TestVesselStageActivated::~TestVesselStageActivated() {
    if (subscribed) GameEvents::onStageActivate.Remove(this, &TestVesselStageActivated::StageActivated);
}
// TestVesselStageActivated.cs:27
bool TestVesselStageActivated::Test() { return eventFound; }
// TestVesselStageActivated.cs:32
void TestVesselStageActivated::Initialized() {
    eventFound = false;
    GameEvents::onStageActivate.Add(this, &TestVesselStageActivated::StageActivated);
    subscribed = true;
}
// TestVesselStageActivated.cs:38
void TestVesselStageActivated::Cleared() {
    GameEvents::onStageActivate.Remove(this, &TestVesselStageActivated::StageActivated);
    subscribed = false;
}
// TestVesselStageActivated.cs:43
void TestVesselStageActivated::StageActivated(int act) {
    TestVessel::Test();
    if (vessel != nullptr && vessel == AwardEnvironment::ActiveVessel() && act == activation) eventFound = true;
}
// TestVesselStageActivated.cs:52
std::string TestVesselStageActivated::GetInfo() { return Tag("#autoLOC_8004032"); }
// TestVesselStageActivated.cs:57
void TestVesselStageActivated::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("activation", activation);
}
// TestVesselStageActivated.cs:63
void TestVesselStageActivated::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    node.TryGetValue("activation", activation);
}
// TestVesselStageActivated.cs:69
ScoreModifier TestVesselStageActivated::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestVesselStageActivated");
    return r;
}

// ---- TestVesselState ---------------------------------------------------------------------------------------------------------

// TestVesselState.cs:18 Awake
TestVesselState::TestVesselState() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000121");
    vesselState = Vessel::State::INACTIVE;
    useActiveVessel = true;
}
// TestVesselState.cs:26
bool TestVesselState::Test() {
    TestVessel::Test();
    if (vessel != nullptr) return vessel->state == vesselState;
    return false;
}
// TestVesselState.cs:36
void TestVesselState::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("state", dotnet::EnumToString(vesselState));
}
// TestVesselState.cs:42
void TestVesselState::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    if (!save_detail::TryGetEnum(node, "state", vesselState, Vessel::State::INACTIVE)) log::Error("Failed to parse vesselstate from " + title);
}
// TestVesselState.cs:51
ScoreModifier TestVesselState::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestVesselState");
    return r;
}

// ---- TestVesselRecovery / TestFundsRecovery / TestResourcesRecovery -----------------------------------------------------------

// TestVesselRecovery.cs:10 Awake
TestVesselRecovery::TestVesselRecovery() : TestVessel(kTypeName) { title = "#autoLOC_8001046"; }
// kspcore: the listener must not outlive the object
TestVesselRecovery::~TestVesselRecovery() {
    if (subscribed) GameEvents::onVesselRecoveryProcessingComplete.Remove(this, &TestVesselRecovery::Recovered);
}
// TestVesselRecovery.cs:16
void TestVesselRecovery::Initialized() {
    GameEvents::onVesselRecoveryProcessingComplete.Add(this, &TestVesselRecovery::Recovered);
    subscribed = true;
}
// TestVesselRecovery.cs:21
void TestVesselRecovery::Cleared() {
    GameEvents::onVesselRecoveryProcessingComplete.Remove(this, &TestVesselRecovery::Recovered);
    subscribed = false;
}
// TestVesselRecovery.cs:26
void TestVesselRecovery::Recovered(ProtoVessel* v, KSP::UI::Screens::MissionRecoveryDialog*, float) {
    if (vesselID == v->persistentId || vesselID == 0) eventFound = true;
}
// TestVesselRecovery.cs:34
bool TestVesselRecovery::Test() { return eventFound; }
// TestVesselRecovery.cs:39
std::string TestVesselRecovery::GetInfo() { return Tag("#autoLOC_8001047"); }

// TestFundsRecovery.cs:25 Awake
TestFundsRecovery::TestFundsRecovery() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000233");
    eventFound = false;
}
// kspcore: the listener must not outlive the object
TestFundsRecovery::~TestFundsRecovery() {
    if (subscribed) GameEvents::onVesselRecoveryProcessingComplete.Remove(this, &TestFundsRecovery::Recovered);
}
// TestFundsRecovery.cs:32
void TestFundsRecovery::Initialized() {
    GameEvents::onVesselRecoveryProcessingComplete.Add(this, &TestFundsRecovery::Recovered);
    subscribed = true;
}
// TestFundsRecovery.cs:37
void TestFundsRecovery::Cleared() {
    GameEvents::onVesselRecoveryProcessingComplete.Remove(this, &TestFundsRecovery::Recovered);
    subscribed = false;
}
// TestFundsRecovery.cs:42 (C# bug kept: LessThanorEqual compares with <)
void TestFundsRecovery::Recovered(ProtoVessel*, KSP::UI::Screens::MissionRecoveryDialog* mrd, float) {
    if (mrd == nullptr) {
        float f = 0.0f;
        float s = 0.0f;
        float r = 0.0f;
        f = CurrencyModifierQuery::RunQuery(TransactionReasons::VesselRecovery, f, s, r).GetEffectDelta(Currency::Funds);
        fundsCollected = fx(f);
    } else {
        fundsCollected = mrd->fundsEarned;
    }
    switch (comparisonOperator) {
        case TestComparisonOperator::LessThan: eventFound = fundsCollected < fundsToRecover; break;
        case TestComparisonOperator::LessThanorEqual: eventFound = fundsCollected < fundsToRecover; break;
        case TestComparisonOperator::Equal:
            if (std::fabs(fundsCollected - fundsToRecover) < epsilon) eventFound = true;
            break;
        case TestComparisonOperator::GreaterThanorEqual: eventFound = fundsCollected >= fundsToRecover; break;
        case TestComparisonOperator::GreaterThan: eventFound = fundsCollected > fundsToRecover; break;
    }
}
// TestFundsRecovery.cs:79
bool TestFundsRecovery::Test() { return eventFound; }
// TestFundsRecovery.cs:84
std::string TestFundsRecovery::GetInfo() { return Tag("#autoLOC_8004016"); }
// TestFundsRecovery.cs:89
std::string TestFundsRecovery::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "fundsToRecover") {
        return Localizer::Format("#autoLOC_8100154", {field.guiName, OperatorString(comparisonOperator), F0(fundsToRecover)});
    }
    return TestModule::GetNodeBodyParameterString(field);
}
// TestFundsRecovery.cs:117
void TestFundsRecovery::Save(ConfigNode& node) const {
    TestModule::Save(node);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
    node.AddValue("fundsRecovered", fundsToRecover);
}
// TestFundsRecovery.cs:124
void TestFundsRecovery::Load(const ConfigNode& node) {
    TestModule::Load(node);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonOperator::GreaterThan);
    node.TryGetValue("fundsRecovered", fundsToRecover);
}
// TestFundsRecovery.cs:131
ScoreModifier TestFundsRecovery::GetScoreModifier(const std::string&) { return {}; }

// TestResourcesRecovery.cs:36 Awake
TestResourcesRecovery::TestResourcesRecovery() : TestVessel(kTypeName) {
    title = Tag("#autoLOC_8000167");
    useActiveVessel = true;
}
// kspcore: the listener must not outlive the object
TestResourcesRecovery::~TestResourcesRecovery() {
    if (subscribed) GameEvents::onVesselRecoveryProcessing.Remove(this, &TestResourcesRecovery::Recovered);
}
// TestResourcesRecovery.cs:43
void TestResourcesRecovery::Initialized() {
    GameEvents::onVesselRecoveryProcessing.Add(this, &TestResourcesRecovery::Recovered);
    subscribed = true;
}
// TestResourcesRecovery.cs:48
void TestResourcesRecovery::Cleared() {
    GameEvents::onVesselRecoveryProcessing.Remove(this, &TestResourcesRecovery::Recovered);
    subscribed = false;
}
// TestResourcesRecovery.cs:53
void TestResourcesRecovery::Recovered(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog*, float) {
    TestVessel::Test();
    if ((!(vessel != nullptr) || !(vessel->id == pv->vesselID)) && vesselID != 0) return;
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    if (lib == nullptr) throw NullReferenceException();
    const PartResourceDefinition* definition = lib->GetDefinition(resourceName);
    if (definition == nullptr) throw NullReferenceException();
    resourcesRecovered = 0.0;
    if (definition->id() == -1) return;
    std::vector<ProtoPartSnapshot*> allProtoPartsIncludingCargo = pv->GetAllProtoPartsIncludingCargo();
    for (size_t i = 0; i < allProtoPartsIncludingCargo.size(); ++i) {
        ProtoPartSnapshot* protoPartSnapshot = allProtoPartsIncludingCargo[i];
        for (size_t j = 0; j < protoPartSnapshot->resources.size(); ++j) {
            const auto& protoPartResourceSnapshot = protoPartSnapshot->resources[j];
            if (!protoPartResourceSnapshot->resourceName.has_value()) throw NullReferenceException();  // GetDefinition(null).GetHashCode()
            const PartResourceDefinition* definition2 = lib->GetDefinition(*protoPartResourceSnapshot->resourceName);
            if (definition2 != nullptr && definition2->id() == definition->id()) resourcesRecovered += protoPartResourceSnapshot->amount;
        }
    }
    eventFound = CompareResource(resourcesRecovered, resourcesToRecover, comparisonOperator);
}
// TestResourcesRecovery.cs:83
bool TestResourcesRecovery::CompareResource(double resourcesRecovered_, double resourcesToRecover_, TestComparisonOperator op) {
    bool result = false;
    switch (op) {
        default: result = false; break;
        case TestComparisonOperator::LessThan: result = resourcesRecovered_ < resourcesToRecover_; break;
        case TestComparisonOperator::LessThanorEqual: result = resourcesRecovered_ <= resourcesToRecover_; break;
        case TestComparisonOperator::Equal:
            if (std::fabs(resourcesRecovered_ - resourcesToRecover_) < epsilon) result = true;
            break;
        case TestComparisonOperator::GreaterThanorEqual: result = resourcesRecovered_ >= resourcesToRecover_; break;
        case TestComparisonOperator::GreaterThan: result = resourcesRecovered_ > resourcesToRecover_; break;
    }
    return result;
}
// TestResourcesRecovery.cs:125
bool TestResourcesRecovery::Test() { return eventFound; }
// TestResourcesRecovery.cs:130
void TestResourcesRecovery::ParameterSetupComplete() { TestVessel::ParameterSetupComplete(); }
// TestResourcesRecovery.cs:135
std::string TestResourcesRecovery::GetInfo() { return Tag("#autoLOC_8004029"); }
// TestResourcesRecovery.cs:140
std::string TestResourcesRecovery::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "resourcesToRecover") {
        return Localizer::Format("#autoLOC_8100154", {field.guiName, OperatorString(comparisonOperator), F0(resourcesToRecover)});
    }
    return TestVessel::GetNodeBodyParameterString(field);
}
// TestResourcesRecovery.cs:168
void TestResourcesRecovery::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    node.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
    node.AddValue("resourcesToRecover", resourcesToRecover);
    node.AddValue("resourceName", resourceName);
}
// TestResourcesRecovery.cs:176
void TestResourcesRecovery::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    save_detail::TryGetEnum(node, "comparisonOperator", comparisonOperator, TestComparisonOperator::GreaterThan);
    node.TryGetValue("resourcesToRecover", resourcesToRecover);
    node.TryGetValue("resourceName", resourceName);
}
// TestResourcesRecovery.cs:184
ScoreModifier TestResourcesRecovery::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestResourceRecovery");
    return r;
}

// ---- TestSOILeft / TestSOIReached --------------------------------------------------------------------------------------------

// TestSOILeft.cs:20 Awake
TestSOILeft::TestSOILeft() : TestVessel(kTypeName), missionBody(NodeLibraryEnvironment::HomeBody()) {
    title = Tag("#autoLOC_8000155");
    useActiveVessel = true;
}
// kspcore: the listener must not outlive the object
TestSOILeft::~TestSOILeft() {
    if (subscribed) GameEvents::onVesselSOIChanged.Remove(this, &TestSOILeft::OnEscapedSOI);
}
// TestSOILeft.cs:28
void TestSOILeft::Initialized() {
    eventFound = false;
    GameEvents::onVesselSOIChanged.Add(this, &TestSOILeft::OnEscapedSOI);
    subscribed = true;
}
// TestSOILeft.cs:34
void TestSOILeft::Cleared() {
    GameEvents::onVesselSOIChanged.Remove(this, &TestSOILeft::OnEscapedSOI);
    subscribed = false;
}
// TestSOILeft.cs:39
void TestSOILeft::OnEscapedSOI(GameEvents::HostedFromToAction<Vessel*, CelestialBody*> fromTo) {
    TestVessel::Test();
    if (vessel == fromTo.host && missionBody.IsValid(fromTo.from)) eventFound = true;
}
// TestSOILeft.cs:48
bool TestSOILeft::Test() { return eventFound; }
// TestSOILeft.cs:53
bool TestSOILeft::HasNodeBody() { return true; }
// TestSOILeft.cs:58
CelestialBody* TestSOILeft::GetNodeBody() { return missionBody.Body(); }
// TestSOILeft.cs:63
std::string TestSOILeft::GetInfo() { return Tag("#autoLOC_8004013"); }
// TestSOILeft.cs:68
void TestSOILeft::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    missionBody.Save(node);
}
// TestSOILeft.cs:74
void TestSOILeft::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    missionBody.Load(node);
}
// TestSOILeft.cs:80
ScoreModifier TestSOILeft::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestSOILeft");
    return r;
}

// TestSOIReached.cs:20 Awake
TestSOIReached::TestSOIReached() : TestVessel(kTypeName), missionBody(NodeLibraryEnvironment::HomeBody()) { title = Tag("#autoLOC_8000164"); }
// kspcore: the listener must not outlive the object
TestSOIReached::~TestSOIReached() {
    if (subscribed) GameEvents::onVesselSOIChanged.Remove(this, &TestSOIReached::OnReachedSOI);
}
// TestSOIReached.cs:27
void TestSOIReached::Initialized() {
    eventFound = false;
    GameEvents::onVesselSOIChanged.Add(this, &TestSOIReached::OnReachedSOI);
    subscribed = true;
}
// TestSOIReached.cs:33
void TestSOIReached::Cleared() {
    GameEvents::onVesselSOIChanged.Remove(this, &TestSOIReached::OnReachedSOI);
    subscribed = false;
}
// TestSOIReached.cs:38
void TestSOIReached::OnReachedSOI(GameEvents::HostedFromToAction<Vessel*, CelestialBody*> fromTo) {
    TestVessel::Test();
    if (vessel == fromTo.host && missionBody.IsValid(fromTo.to)) eventFound = true;
}
// TestSOIReached.cs:47
bool TestSOIReached::Test() { return eventFound; }
// TestSOIReached.cs:52
bool TestSOIReached::HasNodeBody() { return true; }
// TestSOIReached.cs:57
CelestialBody* TestSOIReached::GetNodeBody() { return missionBody.Body(); }
// TestSOIReached.cs:62
std::string TestSOIReached::GetInfo() { return Tag("#autoLOC_8004027"); }
// TestSOIReached.cs:67
void TestSOIReached::Save(ConfigNode& node) const {
    TestVessel::Save(node);
    missionBody.Save(node);
}
// TestSOIReached.cs:73
void TestSOIReached::Load(const ConfigNode& node) {
    TestVessel::Load(node);
    missionBody.Load(node);
}
// TestSOIReached.cs:79
ScoreModifier TestSOIReached::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = ScoreModifierVessel("TestSOIReached");
    return r;
}

}  // namespace ksp::Expansions::Missions::Tests
