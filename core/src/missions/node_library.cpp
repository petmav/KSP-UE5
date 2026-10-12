// WP-55: shared pieces of the Making History node library (see node_library.hpp): MissionCelestialBody, NodeLibraryEnvironment, the
// [MEScoreModule] table and RegisterNodeLibrary.
#include "ksp/missions/node_library.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission_actions.hpp"
#include "ksp/missions/mission_adjusters.hpp"
#include "ksp/missions/mission_awards.hpp"
#include "ksp/missions/mission_score.hpp"
#include "ksp/missions/mission_tests.hpp"
#include "ksp/missions/mission_actions2.hpp"
#include "ksp/missions/mission_tests2.hpp"
#include "ksp/missions/vessel_restrictions.hpp"
#include "ksp/save/save_environment.hpp"

namespace ksp::Expansions::Missions {

using KSP::Localization::Localizer;

// ---- NodeLibraryEnvironment --------------------------------------------------------------------------------------------------

void NodeLibraryEnvironment::Reset() {
    homeBody = nullptr;
    bodies = nullptr;
    bodyByName = nullptr;
    localizedTraitName = nullptr;
    postScreenMessage = nullptr;
    screenMessages.clear();
    spawnMessageDialog = nullptr;
    dialogMessages.clear();
    addMessageSystemMessage = nullptr;
    systemMessages.clear();
    explodePart = nullptr;
    explodedParts.clear();
    activateNextStage = nullptr;
    stagesActivated = 0;
    findLoadedPart = nullptr;
    findNearestControllableVessel = nullptr;
    forceSetActiveVessel = nullptr;
    randomRange = nullptr;
    craftPartIds = nullptr;
    vesselRequestResource = nullptr;
    moduleIsA = nullptr;
    moduleCall = nullptr;
    moduleCalls.clear();
    moduleQuery = nullptr;
    moduleText = nullptr;
    reactionWheelAppliedTorque = nullptr;
    adjusterAttach = nullptr;
    removePartModuleAdjuster = nullptr;
    partModuleSubclasses = nullptr;
    randomRangeFloat = nullptr;
    randomValue = nullptr;
    deltaTime = nullptr;
}

// FlightGlobals.GetHomeBody() (FlightGlobals.cs:1813)
CelestialBody* NodeLibraryEnvironment::HomeBody() {
    if (homeBody) return homeBody();
    for (CelestialBody* b : SaveEnvironment::Current().localBodies)
        if (b != nullptr && b->isHomeWorld) return b;
    return nullptr;
}

// FlightGlobals.GetBodyByName(name)
CelestialBody* NodeLibraryEnvironment::BodyByName(const std::string& name) {
    if (bodyByName) return bodyByName(name);
    if (AwardEnvironment::getBodyByName) return AwardEnvironment::getBodyByName(name);
    for (CelestialBody* b : SaveEnvironment::Current().localBodies)
        if (b != nullptr && b->bodyName == name) return b;
    return nullptr;
}

// ---- MissionCelestialBody ----------------------------------------------------------------------------------------------------

// MissionCelestialBody.cs:62
MissionCelestialBody::MissionCelestialBody() : body_(NodeLibraryEnvironment::HomeBody()) {}
// MissionCelestialBody.cs:67
MissionCelestialBody::MissionCelestialBody(CelestialBody* body) : body_(body) {}

// MissionCelestialBody.cs:14 (LingoonaGrammarExtensions.cs:35 LocalizeRemoveGender: cut at the last '^')
std::string MissionCelestialBody::DisplayName() const {
    if (body_ != nullptr) {
        std::string input = body_->displayName();
        const size_t num = input.rfind('^');
        if (num != std::string::npos) input = input.substr(0, num);
        return input;
    }
    return Localizer::Format("#autoLOC_8000273");
}

// MissionCelestialBody.cs:26
std::string MissionCelestialBody::Name() const {
    if (body_ != nullptr) return body_->name();
    return "AnyValid";
}

// MissionCelestialBody.cs:72
bool MissionCelestialBody::IsValid(CelestialBody* targetbody) const {
    bool result = true;
    if (!AnyValid()) result = body_ == targetbody;
    return result;
}

// MissionCelestialBody.cs:93
std::string MissionCelestialBody::GetNodeBodyParameterString() const { return Localizer::Format("#autoLOC_8000267", {DisplayName()}); }

// MissionCelestialBody.cs:98
void MissionCelestialBody::Save(ConfigNode& node) const { node.AddValue("bodyName", Name()); }

// MissionCelestialBody.cs:103
void MissionCelestialBody::Load(const ConfigNode& node) {
    std::string value;
    node.TryGetValue("bodyName", value);
    if (value != "") {
        if (value == "AnyValid") {
            body_ = nullptr;
        } else {
            body_ = NodeLibraryEnvironment::BodyByName(value);
        }
    }
}

// ---- Library helpers ---------------------------------------------------------------------------------------------------------

namespace Library {
// MissionsUtils.GetFieldValueForDisplay (MissionsUtils.cs, line 571)
std::string BoolFieldText(bool v) { return Localizer::Format(v ? "#autoLOC_439839" : "#autoLOC_439840"); }
}  // namespace Library

// ---- [MEScoreModule] table ---------------------------------------------------------------------------------------------------

// The own [MEScoreModule(new Type[] { ... })] attribute of each class that has one (the Tests/ files)
const std::map<std::string, std::vector<std::string>>& ScoreModuleAttributes() {
    static const std::map<std::string, std::vector<std::string>> table = {
        {"TestMETTime", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestMissionTime", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestTimeSinceNode", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestMissionScore", {}},
        {"TestMissionFunds", {}},
        {"TestGameEvent", {}},
        {"TestVesselActive", {}},
        {"TestVesselMass", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestVesselPartCount", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestVesselCrewCount", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestVesselAltitude", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestVesselResource", {}},
        {"TestVesselStage", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestVesselStageActivated", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestVesselState", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestFundsRecovery", {}},
        {"TestResourcesRecovery", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestSOILeft", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestSOIReached", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestVesselSituationFlying", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestVesselSituationLanded", {"ScoreModule_Resource", "ScoreModule_Time", "ScoreModule_Accuracy"}},
        {"TestVesselSituationSplashed", {"ScoreModule_Resource", "ScoreModule_Time", "ScoreModule_Accuracy"}},
        {"TestVesselStateCrashed", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestVesselVelocity", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestGoTo", {"ScoreModule_Resource", "ScoreModule_Time", "ScoreModule_Accuracy"}},
        {"TestFlyThrough", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestPlantFlag", {"ScoreModule_Resource", "ScoreModule_Time", "ScoreModule_Accuracy"}},
        {"TestKerbalRecovery", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestKerbalRescue", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestKerbalEVA", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestKerbalBoarded", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestTakeKerbal", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestCrewAssignment", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestOrbit", {"ScoreModule_Resource", "ScoreModule_Time", "ScoreModule_Accuracy"}},
        {"TestOrbitParams", {"ScoreModule_Resource", "ScoreModule_Time", "ScoreModule_Accuracy"}},
        {"TestScienceExperiment", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestGrapple", {"ScoreModule_Resource", "ScoreModule_Time"}},
        {"TestPartDocking", {"ScoreModule_Resource", "ScoreModule_Time"}},
    };
    return table;
}

// ---- registration ------------------------------------------------------------------------------------------------------------

namespace {
template <class T>
void RegisterTest() {
    TestModuleRegistry::Register({T::kTypeName, [] { return std::make_unique<T>(); }});
}
template <class T>
void RegisterAction() {
    ActionModuleRegistry::Register({T::kTypeName, [] { return std::make_unique<T>(); }});
}
}  // namespace

void RegisterTest_TestVessel() {
    // TestVessel is a concrete public class: Type.GetType("Expansions.Missions.Tests.TestVessel") resolves
    TestModuleRegistry::Register({"TestVessel", [] { return std::make_unique<Tests::TestVessel>("TestVessel"); }});
}

void RegisterNodeLibrary() {
    using namespace Tests;
    RegisterTest<TestEmptyTrue>();
    RegisterTest<TestEmptyFalse>();
    RegisterTest<TestVesselActive>();
    RegisterTest<TestMETTime>();
    RegisterTest<TestMissionTime>();
    RegisterTest<TestTimeSinceNode>();
    RegisterTest<TestMissionScore>();
    RegisterTest<TestMissionFunds>();
    RegisterTest<TestAccuracy>();
    RegisterTest<TestGameEvent>();
    RegisterTest<TestVesselMass>();
    RegisterTest<TestVesselPartCount>();
    RegisterTest<TestVesselCrewCount>();
    RegisterTest<TestVesselAltitude>();
    RegisterTest<TestVesselActiveCount>();
    RegisterTest<TestVesselResource>();
    RegisterTest<TestVesselStage>();
    RegisterTest<TestVesselStageActivated>();
    RegisterTest<TestVesselState>();
    RegisterTest<TestVesselRecovery>();
    RegisterTest<TestFundsRecovery>();
    RegisterTest<TestResourcesRecovery>();
    RegisterTest<TestSOILeft>();
    RegisterTest<TestSOIReached>();
    RegisterTest_TestVessel();
    RegisterNodeLibrarySlice2Tests();
    RegisterNodeLibrarySlice2Actions();
    RegisterVesselRestrictions();
    using namespace Actions;
    RegisterAction<ActionMessage>();
    RegisterAction<ActionDialogMessage>();
    RegisterAction<ActionModifierFunds>();
    RegisterAction<ActionMissionScore>();
    RegisterAction<ActionModifierScore>();
    ActionModuleRegistry::Register({"ActionVessel", [] { return std::make_unique<ActionVessel>("ActionVessel"); }});
    RegisterAction<ActionVesselActionGroup>();
    RegisterAction<ActionVesselExplode>();
    RegisterAction<ActionVesselStage>();
    RegisterAction<ActionSetCamera>();
    RegisterAction<ActionPartResourceAmount>();
    RegisterAction<ActionPartExplode>();
    Adjusters::RegisterAdjusters();
    // MissionScore.GetSupportedTypes reads [MEScoreModule] of the objective's own class (inherit: false)
    MissionScore::allowedSystemsOf = [](IScoreableObjective* objective) -> std::vector<std::string> {
        auto* module = dynamic_cast<TestModule*>(objective);
        if (module == nullptr) return {};
        auto it = ScoreModuleAttributes().find(module->TypeName());
        if (it == ScoreModuleAttributes().end()) return {};
        return it->second;
    };
}

}  // namespace ksp::Expansions::Missions
