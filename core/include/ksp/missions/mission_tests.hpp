// WP-55: Assembly-CSharp/Expansions/Missions/Tests/*.cs (Expansions.Missions.Tests). See node_library.hpp for the conventions.
// The line numbers on the members are those of the C# file named in the class comment (the .cpp cites are what check_cites verifies).
//
// Ported here (the engine-agnostic tests): TestEmptyTrue, TestEmptyFalse, TestVessel, TestVesselActive, TestMETTime, TestMissionTime,
// TestTimeSinceNode, TestMissionScore, TestMissionFunds, TestAccuracy, TestGameEvent, TestVesselMass, TestVesselPartCount,
// TestVesselCrewCount, TestVesselAltitude, TestVesselActiveCount, TestVesselResource, TestVesselStage, TestVesselStageActivated,
// TestVesselState, TestVesselRecovery, TestFundsRecovery, TestResourcesRecovery, TestSOILeft, TestSOIReached.
#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ksp/events/game_events.hpp"
#include "ksp/missions/mission_node.hpp"
#include "ksp/missions/node_library.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Missions::Tests {

// "<" "<=" "=" ">=" ">" (the operatorString switch repeated by the TestComparisonOperator tests; "" for an undefined value)
std::string OperatorString(TestComparisonOperator op);

// TestEmptyTrue.cs:6
class TestEmptyTrue : public TestModule {
public:
    static constexpr const char* kTypeName = "TestEmptyTrue";
    TestEmptyTrue();                          // :8 Awake
    bool Test() override;                     // :14
    std::string GetInfo() override;           // :19
};

// TestEmptyFalse.cs:5
class TestEmptyFalse : public TestModule {
public:
    static constexpr const char* kTypeName = "TestEmptyFalse";
    TestEmptyFalse();                         // :7 Awake
    bool Test() override;                     // :13
    std::string GetInfo() override;           // :18
};

// TestVessel.cs:9
class TestVessel : public TestModule {
public:
    explicit TestVessel(std::string typeName);                                   // :23 Awake
    uint32_t vesselID = 0;                                                       // :13
    bool IsA(std::string_view n) const override { return n == "TestVessel" || TestModule::IsA(n); }
    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) override;   // :29
    void OnVesselDocking(uint32_t oldId, uint32_t newId) override;               // :37
    void OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) override;      // :49
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :67
    bool Test() override;                                                        // :104
    void Save(ConfigNode& node) const override;                                  // :134
    void Load(const ConfigNode& node) override;                                  // :145

protected:
    // the shared body of every `GetScoreModifier` for ScoreModule_Resource (TestVesselMass.cs:88 and 16 copies): ActiveVessel, the vessel
    // of the (mapped) id, or null (the C# logs "[<class>] Unable to find VesselID" and returns null)
    Vessel* ScoreModifierVessel(const char* className);
    bool useActiveVessel = false;                                                // :15
    Vessel* vessel = nullptr;                                                    // :17

private:
    std::vector<uint32_t> missingVessels;                                        // :19
    uint32_t oldRootPart = 0;                                                    // :21
};

// TestVesselActive.cs:9 (not scoreable; [MEScoreModule({})])
class TestVesselActive : public TestVessel {
public:
    static constexpr const char* kTypeName = "TestVesselActive";
    TestVesselActive();                       // :11 Awake
    bool Test() override;                     // :18
    std::string GetInfo() override;           // :24
};

// TestMETTime.cs:13
class TestMETTime : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestMETTime";
    TestMETTime();                                                               // :21 Awake
    double time = 300.0;                                                         // :16
    TestComparisonLessGreaterOnly comparisonOperator = TestComparisonLessGreaterOnly::GreaterThan;  // :19
    bool Test() override;                                                        // :29
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :46
    std::string GetInfo() override;                                              // :55
    void Save(ConfigNode& node) const override;                                  // :60
    void Load(const ConfigNode& node) override;                                  // :67
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :75
};

// TestMissionTime.cs:13
class TestMissionTime : public TestModule, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestMissionTime";
    TestMissionTime();                                                           // :23 Awake
    double time = 300.0;                                                         // :16
    TestComparisonLessGreaterOnly comparisonOperator = TestComparisonLessGreaterOnly::GreaterThan;  // :21
    bool Test() override;                                                        // :30
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :44
    std::string GetInfo() override;                                              // :53
    void Save(ConfigNode& node) const override;                                  // :58
    void Load(const ConfigNode& node) override;                                  // :65
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :73

private:
    double missionTime = 0.0;                                                    // :18
};

// TestTimeSinceNode.cs:13
class TestTimeSinceNode : public TestModule, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestTimeSinceNode";
    TestTimeSinceNode();                                                         // :23 Awake
    Guid nodeID;                                                                 // :16
    double time = 300.0;                                                         // :19
    TestComparisonLessGreaterOnly comparisonOperator = TestComparisonLessGreaterOnly::GreaterThan;  // :24
    void Initialize(TestGroup* testGroup, const std::string& title = "") override;  // :30
    bool Test() override;                                                        // :38
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :63
    std::string GetInfo() override;                                              // :87
    void Save(ConfigNode& node) const override;                                  // :92
    void Load(const ConfigNode& node) override;                                  // :99
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :112
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :120

private:
    double nodeCompareTime = 0.0;                                                // :21
};

// TestMissionScore.cs:10
class TestMissionScore : public TestModule, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestMissionScore";
    TestMissionScore();                                                          // :22 Awake
    float score = 0.0f;                                                          // :13
    TestComparisonOperator comparisonOperator = TestComparisonOperator::LessThan;  // :16
    bool Test() override;                                                        // :28
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :50
    std::string GetInfo() override;                                              // :84
    void Save(ConfigNode& node) const override;                                  // :89
    void Load(const ConfigNode& node) override;                                  // :95
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :105
};

// TestMissionFunds.cs:10
class TestMissionFunds : public TestModule, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestMissionFunds";
    TestMissionFunds();                                                          // :22 Awake
    float funds = 0.0f;                                                          // :13
    TestComparisonOperator comparisonOperator = TestComparisonOperator::LessThan;  // :16
    bool Test() override;                                                        // :28
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :58
    std::string GetInfo() override;                                              // :88
    void Save(ConfigNode& node) const override;                                  // :93
    void Load(const ConfigNode& node) override;                                  // :99
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :109
};

// TestAccuracy.cs:7
class TestAccuracy : public TestModule {
public:
    static constexpr const char* kTypeName = "TestAccuracy";
    TestAccuracy();                                                              // :18 Awake
    double accuracy = 0.0;                                                       // :11
    TestComparisonLessGreaterOnly comparisonOperator = TestComparisonLessGreaterOnly::GreaterThan;  // :14
    bool Test() override;                                                        // :25
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :45
    std::string GetInfo() override;                                              // :54
    void Save(ConfigNode& node) const override;                                  // :59
    void Load(const ConfigNode& node) override;                                  // :65

protected:
    IScoreableObjective* scoreableObjective = nullptr;                           // :16
};

// TestGameEvent.cs:6
class TestGameEvent : public TestModule, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestGameEvent";
    TestGameEvent();                                                             // :11 Awake
    ~TestGameEvent() override;
    bool Test() override;                                                        // :17
    void Save(ConfigNode& node) const override;                                  // :27
    void Load(const ConfigNode& node) override;                                  // :33
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :50

private:
    void SuscribeToEvent();                                                      // :39
    void EventCallback_StageSeparation(int eventReport);                         // :45
    bool eventTriggered = false;                                                 // :8
    bool eventSuscribed = false;                                                 // :9
};

// TestVesselMass.cs:13
class TestVesselMass : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestVesselMass";
    TestVesselMass();                                                            // :23 Awake
    double mass = 25.0;                                                          // :16
    TestComparisonLessGreaterOnly comparisonOperator = TestComparisonLessGreaterOnly::GreaterThan;  // :21
    bool Test() override;                                                        // :31
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :51
    std::string GetInfo() override;                                              // :60
    void Save(ConfigNode& node) const override;                                  // :65
    void Load(const ConfigNode& node) override;                                  // :72
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :79

private:
    double massCompare = 0.0;                                                    // :18
};

// TestVesselPartCount.cs:13
class TestVesselPartCount : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestVesselPartCount";
    TestVesselPartCount();                                                       // :23 Awake
    int parts = 25;                                                              // :16
    int partsCompare = 0;                                                        // :18
    TestComparisonOperator comparisonOperator = TestComparisonOperator::Equal;   // :21
    bool Test() override;                                                        // :31
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :76
    std::string GetInfo() override;                                              // :105
    void Save(ConfigNode& node) const override;                                  // :110
    void Load(const ConfigNode& node) override;                                  // :116
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :122

private:
    void CheckForUnloadedVessel();                                               // :62
};

// TestVesselCrewCount.cs:11
class TestVesselCrewCount : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestVesselCrewCount";
    TestVesselCrewCount();                                                       // :29 Awake
    int crewAmount = 0;                                                          // :14
    TestComparisonOperator comparisonOperator = TestComparisonOperator::LessThan;  // :17
    std::string traitName;                                                       // :20
    bool Test() override;                                                        // :38
    std::string GetInfo() override;                                              // :100
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :105
    void Save(ConfigNode& node) const override;                                  // :150
    void Load(const ConfigNode& node) override;                                  // :158
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :166

private:
    int currentCrewCounter = 0;                                                  // :22
};

// TestVesselAltitude.cs:13
class TestVesselAltitude : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestVesselAltitude";
    TestVesselAltitude();                                                        // :26 Awake
    double altitude = 0.0;                                                       // :16
    bool useRadarAltimiter = false;                                              // :21
    TestComparisonLessGreaterOnly comparisonOperator = TestComparisonLessGreaterOnly::GreaterThan;  // :24
    bool Test() override;                                                        // :36
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :63
    std::string GetInfo() override;                                              // :80
    void Save(ConfigNode& node) const override;                                  // :85
    void Load(const ConfigNode& node) override;                                  // :92
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :100

private:
    double altitudeCompare = 0.0;                                                // :18
};

// TestVesselActiveCount.cs:7
class TestVesselActiveCount : public TestModule {
public:
    // :9 enum VesselType
    enum class VesselType { All = 0, PlayerControlled = 1, Asteroids = 2, Comets = 3, Unowned = 4 };
    static constexpr const char* kTypeName = "TestVesselActiveCount";
    TestVesselActiveCount();                                                     // :39 Awake
    ~TestVesselActiveCount() override;
    VesselType vesselType = VesselType::All;                                     // :24
    TestComparisonOperator comparisonOperator = TestComparisonOperator::LessThan;  // :27
    int vesselCount = 0;                                                         // :30
    void Initialized() override;                                                 // :44
    void Cleared() override;                                                     // :55
    bool Test() override;                                                        // :114
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :134
    std::string GetInfo() override;                                              // :159
    void Save(ConfigNode& node) const override;                                  // :164
    void Load(const ConfigNode& node) override;                                  // :171

protected:
    int countCompare = 0;                                                        // :33

private:
    int addSubscriptions = 0;     // kspcore: onFlightGlobalsAddVessel registrations (Cleared() registers again, a C# bug)
    int removeSubscriptions = 0;  // kspcore: onFlightGlobalsRemoveVessel registrations
    void OnAddVessel(Vessel* v);                                                 // :62
    void OnRemoveVessel(Vessel* v);                                              // :67
    void UpdateVesselCount(Vessel* v, int value);                                // :72
};

// TestVesselResource.cs:9
class TestVesselResource : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestVesselResource";
    TestVesselResource();                                                        // :26 Awake
    std::string resourceName = "LiquidFuel";                                     // :12
    double resourceAmount = 0.0;                                                 // :15
    TestComparisonOperator comparisonOperator = TestComparisonOperator::LessThan;  // :18
    bool Test() override;                                                        // :33
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :92
    std::string GetInfo() override;                                              // :132
    void Save(ConfigNode& node) const override;                                  // :137
    void Load(const ConfigNode& node) override;                                  // :144
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :155

private:
    double currentResourceAmount = 0.0;                                          // :23
    double maxResourceAmount = 0.0;                                              // :25
    std::string resourceDisplay;                                                 // :27
};

// TestVesselStage.cs:10
class TestVesselStage : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestVesselStage";
    TestVesselStage();                                                           // :20 Awake
    int stage = 0;                                                               // :13
    TestComparisonOperator comparisonOperator = TestComparisonOperator::GreaterThan;  // :16
    bool Test() override;                                                        // :27
    std::string GetInfo() override;                                              // :56
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :61
    void Save(ConfigNode& node) const override;                                  // :89
    void Load(const ConfigNode& node) override;                                  // :95
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :102
};

// TestVesselStageActivated.cs:10
class TestVesselStageActivated : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestVesselStageActivated";
    TestVesselStageActivated();                                                  // :18 Awake
    ~TestVesselStageActivated() override;
    int activation = 0;                                                          // :13
    bool Test() override;                                                        // :25
    void Initialized() override;                                                 // :30
    void Cleared() override;                                                     // :36
    std::string GetInfo() override;                                              // :53
    void Save(ConfigNode& node) const override;                                  // :58
    void Load(const ConfigNode& node) override;                                  // :64
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :71

private:
    void StageActivated(int act);                                                // :41
    bool eventFound = false;                                                     // :15
    bool subscribed = false;                                                     // kspcore: the listener is registered
};

// TestVesselState.cs:9
class TestVesselState : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestVesselState";
    TestVesselState();                                                           // :14 Awake
    Vessel::State vesselState = Vessel::State::INACTIVE;                         // :12
    bool Test() override;                                                        // :22
    void Save(ConfigNode& node) const override;                                  // :32
    void Load(const ConfigNode& node) override;                                  // :38
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :47
};

// TestVesselRecovery.cs:5
class TestVesselRecovery : public TestVessel {
public:
    static constexpr const char* kTypeName = "TestVesselRecovery";
    TestVesselRecovery();                                                        // :9 Awake
    ~TestVesselRecovery() override;
    void Initialized() override;                                                 // :15
    void Cleared() override;                                                     // :20
    void Recovered(ProtoVessel* v, KSP::UI::Screens::MissionRecoveryDialog* mrd, float x);  // :25
    bool Test() override;                                                        // :33
    std::string GetInfo() override;                                              // :38

private:
    bool eventFound = false;                                                     // :7
    bool subscribed = false;                                                     // kspcore: the listener is registered
};

// TestFundsRecovery.cs:9
class TestFundsRecovery : public TestModule, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestFundsRecovery";
    TestFundsRecovery();                                                         // :23 Awake
    ~TestFundsRecovery() override;
    double fundsToRecover = 0.0;                                                 // :12
    TestComparisonOperator comparisonOperator = TestComparisonOperator::LessThan;  // :15
    void Initialized() override;                                                 // :29
    void Cleared() override;                                                     // :33
    void Recovered(ProtoVessel* v, KSP::UI::Screens::MissionRecoveryDialog* mrd, float x);  // :37
    bool Test() override;                                                        // :73
    std::string GetInfo() override;                                              // :78
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :83
    void Save(ConfigNode& node) const override;                                  // :111
    void Load(const ConfigNode& node) override;                                  // :117
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :126

private:
    bool eventFound = false;                                                     // :20
    double fundsCollected = 0.0;                                                 // :22
    bool subscribed = false;                                                     // kspcore: the listener is registered
};

// TestResourcesRecovery.cs:12
class TestResourcesRecovery : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestResourcesRecovery";
    TestResourcesRecovery();                                                     // :32 Awake
    ~TestResourcesRecovery() override;
    std::string resourceName = "LiquidFuel";                                     // :15
    double resourcesToRecover = 0.0;                                             // :18
    TestComparisonOperator comparisonOperator = TestComparisonOperator::LessThan;  // :21
    void Initialized() override;                                                 // :37
    void Cleared() override;                                                     // :41
    void Recovered(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrd, float x);  // :45
    bool Test() override;                                                        // :114
    void ParameterSetupComplete() override;                                      // :119
    std::string GetInfo() override;                                              // :123
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :128
    void Save(ConfigNode& node) const override;                                  // :156
    void Load(const ConfigNode& node) override;                                  // :163
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :171

private:
    bool CompareResource(double resourcesRecovered, double resourcesToRecover, TestComparisonOperator comparisonOperator);  // :78
    bool eventFound = false;                                                     // :24
    double resourcesRecovered = 0.0;                                             // :26
    bool subscribed = false;                                                     // kspcore: the listener is registered
};

// TestSOILeft.cs:12 (INodeBody: the node's map body)
class TestSOILeft : public TestVessel, public IScoreableObjective, public INodeBody {
public:
    static constexpr const char* kTypeName = "TestSOILeft";
    TestSOILeft();                                                               // :20 Awake
    ~TestSOILeft() override;
    MissionCelestialBody missionBody;                                            // :15
    void Initialized() override;                                                 // :27
    void Cleared() override;                                                     // :33
    bool Test() override;                                                        // :48
    bool HasNodeBody() override;                                                 // :53
    CelestialBody* GetNodeBody() override;                                       // :58
    std::string GetInfo() override;                                              // :63
    void Save(ConfigNode& node) const override;                                  // :68
    void Load(const ConfigNode& node) override;                                  // :74
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :80

private:
    void OnEscapedSOI(GameEvents::HostedFromToAction<Vessel*, CelestialBody*> fromTo);  // :38
    bool eventFound = false;                                                     // :17
    bool subscribed = false;                                                     // kspcore: the listener is registered
};

// TestSOIReached.cs:12
class TestSOIReached : public TestVessel, public IScoreableObjective, public INodeBody {
public:
    static constexpr const char* kTypeName = "TestSOIReached";
    TestSOIReached();                                                            // :19 Awake
    ~TestSOIReached() override;
    MissionCelestialBody missionBody;                                            // :15
    void Initialized() override;                                                 // :25
    void Cleared() override;                                                     // :31
    bool Test() override;                                                        // :46
    bool HasNodeBody() override;                                                 // :51
    CelestialBody* GetNodeBody() override;                                       // :56
    std::string GetInfo() override;                                              // :61
    void Save(ConfigNode& node) const override;                                  // :66
    void Load(const ConfigNode& node) override;                                  // :72
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :78

private:
    void OnReachedSOI(GameEvents::HostedFromToAction<Vessel*, CelestialBody*> fromTo);  // :36
    bool eventFound = false;                                                     // :17
    bool subscribed = false;                                                     // kspcore: the listener is registered
};

}  // namespace ksp::Expansions::Missions::Tests

// ---- enum metadata (declaration order, [Description] text) ---------------------------------------------------------------------
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Tests::TestVesselActiveCount::VesselType> {
    using E = ksp::Expansions::Missions::Tests::TestVesselActiveCount::VesselType;
    static constexpr bool flags = false;
    // TestVesselActiveCount.cs:9
    static constexpr EnumMember<E> members[] = {{E::All, "All", "#autoLOC_900712"},
                                                {E::PlayerControlled, "PlayerControlled", "#autoLOC_8001049"},
                                                {E::Asteroids, "Asteroids", "#autoLOC_308682"},
                                                {E::Comets, "Comets", "#autoLOC_6006050"},
                                                {E::Unowned, "Unowned", "#autoLOC_901070"}};
};
// Vessel.State (Vessel.cs:12): TestVesselState parses / writes the member names
template <>
struct ksp::dotnet::EnumTraits<ksp::VesselState> {
    using E = ksp::VesselState;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::INACTIVE, "INACTIVE"}, {E::ACTIVE, "ACTIVE"}, {E::DEAD, "DEAD"}};
};
