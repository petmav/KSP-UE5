// WP-54 slice 2: the Tests.* classes that WP-55 deferred because they need the slice-2 types (Expansions/Missions/Tests/*.cs):
// TestVesselSituation (+ Flying / Landed / Splashed), TestVesselStateCrashed, TestVesselVelocity, TestGoTo, TestFlyThrough, TestPlantFlag,
// TestKerbalKilled / Recovery / Rescue / EVA / Boarded, TestTakeKerbal, TestCrewAssignment, TestOrbit, TestOrbitParams, TestScienceExperiment,
// TestGrapple, TestPartDocking. TestDistance (875 lines of editor dropdown plumbing) is not ported.
// Conventions: see node_library.hpp. The editor-control callbacks (OnXControlCreated, OnXValueChange, ParameterSetupComplete, ...) drive MEGUI
// widgets and are not ported; the data they keep in sync is the data the tests read.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/events/game_events.hpp"
#include "ksp/missions/mission_actions.hpp"
#include "ksp/missions/mission_awards.hpp"
#include "ksp/missions/mission_data.hpp"
#include "ksp/missions/mission_tests.hpp"

namespace ksp {
class KerbalEVA;
class EventReport;
class Part;
}  // namespace ksp

namespace ksp::Expansions::Missions {

// What the tests read from the flight world that kspcore has no object for (unset = the stated default).
struct MissionTestsEnvironment {
    // vessel.crashObjectName?.objectName (CrashObjectName component). Unset: nullopt (no component).
    static inline std::function<std::optional<std::string>(Vessel*)> vesselCrashObjectName;
    // ScienceUtil.GetExperimentBiome(body, lat, lon). Unset: ScienceUtil::GetExperimentBiome.
    static inline std::function<std::string(CelestialBody*, double lat, double lon)> experimentBiome;
    // FlightGlobals.ship_obtVelocity. Unset: zero.
    static inline std::function<Vector3d()> shipObtVelocity;
    // FlagSite.vessel (PartModule.vessel of the flag part). Unset: nullptr.
    static inline std::function<Vessel*(FlagSite*)> flagSiteVessel;
    // FlightGlobals.currentMainBody. Unset: nullptr.
    static inline std::function<CelestialBody*()> currentMainBody;
    // EventReport.sender (GameEvents.onCrewKilled). Unset: "".
    static inline std::function<std::string(EventReport*)> eventReportSender;
    // FlightGlobals.PersistentLoadedPartIds[id] / PersistentUnloadedPartIds[id].protoModuleCrew (TestCrewAssignment). Unset: not found.
    static inline std::function<Part*(uint32_t partId)> persistentLoadedPart;
    struct UnloadedPart {
        std::vector<std::shared_ptr<ProtoCrewMember>> protoModuleCrew;
        uint32_t vesselPersistentId = 0;   // pVesselRef.persistentId
    };
    static inline std::function<std::optional<UnloadedPart>(uint32_t partId)> persistentUnloadedPart;
    // KerbalEVA.part.protoModuleCrew (TestKerbalBoarded.OnSeated): the crew of the KerbalEVA's part and its vessel. Unset: nothing.
    // FlightGlobals.FindUnloadedPart(partId, out snapshot).pVesselRef.persistentId (TestPartDocking). Unset: not found.
    static inline std::function<std::optional<uint32_t>(uint32_t partId)> findUnloadedPartVessel;
    // vessel.PartsContain / part.FindModuleImplementing<ModuleAsteroid>() != null || ...<ModuleComet>() != null (TestGrapple). Unset: false.
    static inline std::function<bool(Part*)> partIsAsteroidOrComet;
    // part.FindModuleImplementing<ModuleGrappleNode>().state (TestGrapple). Unset: nullopt (no grapple node).
    static inline std::function<std::optional<std::string>(Part*)> grappleNodeState;
    // KerbalEVA.part.protoModuleCrew and KerbalEVA.vessel (TestKerbalBoarded.OnSeated). Unset: no crew, no vessel.
    static inline std::function<std::pair<std::vector<std::shared_ptr<ProtoCrewMember>>, Vessel*>(KerbalEVA*)> kerbalEvaCrew;
    // VesselUtilities.VesselAtOrbit(orbit, deviation, vessel) / VesselAtOrbitAccuracy (FinePrint, gated by WP-38). Unset: Contracts::Lite::VesselAtOrbit*.
    static inline std::function<bool(Orbit*, double, Vessel*)> vesselAtOrbit;
    static inline std::function<double(Orbit*, double, Vessel*)> vesselAtOrbitAccuracy;
    // TestOrbit.CreateOrbitRenderer / MissionOrbitRenderer.Setup (UI): unset: not created.
    static inline std::function<bool(MENode*, Vessel*, Orbit*)> setupOrbitRenderer;
    static void Reset();
};

namespace Tests {

// TestVesselSituation.cs:19 (abstract)
class TestVesselSituation : public TestVessel, public IScoreableObjective, public INodeWaypoint, public ITestNodeLabel, public INodeBody {
public:
    explicit TestVesselSituation(std::string typeName);                          // :36 Awake
    bool IsA(std::string_view n) const override { return n == "TestVesselSituation" || TestVessel::IsA(n); }
    Vessel::Situations situation;                                                // :25
    virtual double SurfaceVelocity() = 0;                                        // :34
    virtual bool IgnoreSurfaceVelocity() = 0;                                    // :36
    virtual ParamChoices_CelestialBodySurface* LocationSituation() = 0;          // :38
    void Initialized() override;                                                 // :47
    bool Test() override;                                                        // :55
    bool HasNodeWaypoint() override;                                             // :126
    FinePrint::Waypoint* GetNodeWaypoint() override;                             // :131
    bool HasNodeLabel() override;                                                // :141
    bool HasWorldPosition() override;                                            // :146
    Vector3f GetWorldPosition() override;                                        // :151
    std::string GetExtraText() override;                                         // :156
    MENode* node() override { return TestModule::node(); }
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :161
    bool HasNodeBody() override;                                                 // :175
    CelestialBody* GetNodeBody() override;                                       // :185
    std::string GetInfo() override;                                              // :195
    void Save(ConfigNode& node) const override;                                  // :200
    void Load(const ConfigNode& node) override;                                  // :206
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :212
    double stabilizationTime = 1.0;                                              // :21

protected:
    std::string currentBiome;                                                    // :29
    bool speedSuccess = false;                                                   // :31
    bool situationSuccess = false;                                               // :33
    bool locationSuccess = false;                                                // :35
    double successStartTime = -1.0;                                              // :37
    std::unique_ptr<FinePrint::Waypoint> nodeWaypoint_;
    void InitLocationSituation(ParamChoices_CelestialBodySurface& p);            // the Awake block of the three subclasses
};

// TestVesselSituationFlying.cs:13
class TestVesselSituationFlying : public TestVesselSituation {
public:
    static constexpr const char* kTypeName = "TestVesselSituationFlying";
    TestVesselSituationFlying();                                                 // :29 Awake
    double SurfaceVelocity() override { return surfaceVelocity; }                // :19
    bool IgnoreSurfaceVelocity() override { return ignoreSurfaceVelocity; }      // :23
    ParamChoices_CelestialBodySurface* LocationSituation() override { return locationSituation.get(); }  // :27
    void Save(ConfigNode& node) const override;                                  // :48
    void Load(const ConfigNode& node) override;                                  // :56
    std::string GetInfo() override;                                              // :66
    double surfaceVelocity = 0.10000000149011612;                                // :11
    bool ignoreSurfaceVelocity = true;                                           // :14
    std::unique_ptr<ParamChoices_CelestialBodySurface> locationSituation;        // :17
};
// TestVesselSituationLanded.cs:13
class TestVesselSituationLanded : public TestVesselSituation {
public:
    static constexpr const char* kTypeName = "TestVesselSituationLanded";
    TestVesselSituationLanded();                                                 // :29
    double SurfaceVelocity() override { return surfaceVelocity; }
    bool IgnoreSurfaceVelocity() override { return ignoreSurfaceVelocity; }
    ParamChoices_CelestialBodySurface* LocationSituation() override { return locationSituation.get(); }
    void Save(ConfigNode& node) const override;                                  // :47
    void Load(const ConfigNode& node) override;                                  // :55
    std::string GetInfo() override;                                              // :63
    double surfaceVelocity = 0.10000000149011612;                                // :11
    bool ignoreSurfaceVelocity = false;                                          // :14
    std::unique_ptr<ParamChoices_CelestialBodySurface> locationSituation;        // :17
};
// TestVesselSituationSplashed.cs:13
class TestVesselSituationSplashed : public TestVesselSituation {
public:
    static constexpr const char* kTypeName = "TestVesselSituationSplashed";
    TestVesselSituationSplashed();                                               // :29
    double SurfaceVelocity() override { return surfaceVelocity; }
    bool IgnoreSurfaceVelocity() override { return ignoreSurfaceVelocity; }
    ParamChoices_CelestialBodySurface* LocationSituation() override { return locationSituation.get(); }
    void Save(ConfigNode& node) const override;                                  // :47
    void Load(const ConfigNode& node) override;                                  // :55
    double surfaceVelocity = 0.10000000149011612;                                // :11
    bool ignoreSurfaceVelocity = true;                                           // :14
    std::unique_ptr<ParamChoices_CelestialBodySurface> locationSituation;        // :17
};

// TestVesselStateCrashed.cs:13 (internal)
class TestVesselStateCrashed : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestVesselStateCrashed";
    TestVesselStateCrashed();                                                    // :23 Awake
    ~TestVesselStateCrashed() override;
    void Initialized() override;                                                 // :35
    void Cleared() override;                                                     // :41
    bool Test() override;                                                        // :64
    std::string GetInfo() override;                                              // :95
    void Save(ConfigNode& node) const override;                                  // :100
    void Load(const ConfigNode& node) override;                                  // :106
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :112
    std::unique_ptr<ParamChoices_CelestialBodySurface> locationSituation;        // :20

private:
    void OnVesselToBeDestroyed(Vessel* vesselDestroyed);                         // :46
    bool TestVesselLocation();                                                   // :66
    bool eventFound = false;                                                     // :16
    bool subscribed = false;
};

// TestVesselVelocity.cs:14
class TestVesselVelocity : public TestVessel {
public:
    enum class SpeedType { SurfaceVelocity = 0, OrbitalVelocity = 1 };           // :16
    enum class VelocityReferenceFrame { Default = 0, Asteroid = 1, Comet = 2, Kerbal = 3, Vessel = 4 };  // :24
    static constexpr const char* kTypeName = "TestVesselVelocity";
    TestVesselVelocity();                                                        // :80 Awake
    float velocity = 0.0f;                                                       // :39
    TestComparisonLessGreaterOnly comparisonOperator = TestComparisonLessGreaterOnly::GreaterThan;  // :42
    VelocityReferenceFrame referenceBody = VelocityReferenceFrame::Default;      // :45
    SpeedType speedType = SpeedType::SurfaceVelocity;                            // :51
    std::unique_ptr<MissionKerbal> velocityKerbal;                               // :55
    uint32_t velocityVesselID = 0;                                               // :61
    uint32_t velocityAsteroidID = 0;                                             // :67
    uint32_t velocityCometID = 0;                                                // :70
    void Initialize(TestGroup* testGroup, const std::string& title = "") override;  // :178
    bool Test() override;                                                        // :191
    std::string GetInfo() override;                                              // :273
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :278
    void Save(ConfigNode& node) const override;                                  // :318
    void Load(const ConfigNode& node) override;                                  // :334

private:
    bool CheckCrew(Vessel* v, MissionKerbal& missionKerbal);                     // :260
    Vessel* vesselRefBody = nullptr;                                             // :91
};

// TestGoTo.cs:13 (internal)
class TestGoTo : public TestVessel, public IScoreableObjective, public INodeWaypoint, public ITestNodeLabel {
public:
    static constexpr const char* kTypeName = "TestGoTo";
    TestGoTo();                                                                  // :35 Awake
    std::unique_ptr<SurfaceArea> areaData;                                       // :20
    double surfaceVelocity = 0.10000000149011612;                                // :23
    bool ignoreSurfaceVelocity = false;                                          // :26
    bool HasNodeWaypoint() override;                                             // :43
    FinePrint::Waypoint* GetNodeWaypoint() override;                             // :48
    bool Test() override;                                                        // :55
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :95
    MENode* node() override { return TestModule::node(); }
    bool HasNodeLabel() override;                                                // :104
    bool HasWorldPosition() override;                                            // :109
    Vector3f GetWorldPosition() override;                                        // :114
    std::string GetExtraText() override;                                         // :119
    std::string GetInfo() override;                                              // :124
    void Save(ConfigNode& node) const override;                                  // :129
    void Load(const ConfigNode& node) override;                                  // :137
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :146

private:
    bool speedSuccess = false;                                                   // :33
    bool passed = false;                                                         // :35
    std::unique_ptr<FinePrint::Waypoint> nodeWaypoint_;
};

// TestFlyThrough.cs:13 (internal)
class TestFlyThrough : public TestVessel, public IScoreableObjective, public INodeWaypoint, public ITestNodeLabel {
public:
    static constexpr const char* kTypeName = "TestFlyThrough";
    TestFlyThrough();                                                            // :29 Awake
    std::unique_ptr<SurfaceVolume> volumeData;                                   // :17
    bool showNodeLabel = false;                                                  // :20
    bool showMapNode = true;                                                     // :23
    void OnShowLabelChanged(bool value);                                         // :37
    bool HasNodeLabel() override;                                                // :42
    bool HasWorldPosition() override;                                            // :47
    Vector3f GetWorldPosition() override;                                        // :52
    std::string GetExtraText() override;                                         // :57
    bool HasNodeWaypoint() override;                                             // :62
    FinePrint::Waypoint* GetNodeWaypoint() override;                             // :67
    MENode* node() override { return TestModule::node(); }
    bool Test() override;                                                        // :80
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :95
    std::string GetInfo() override;                                              // :104
    void Save(ConfigNode& node) const override;                                  // :109
    void Load(const ConfigNode& node) override;                                  // :117
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :127

private:
    bool passed = false;                                                         // :26
    std::unique_ptr<FinePrint::Waypoint> nodeWaypoint_;
};

// TestPlantFlag.cs:14
class TestPlantFlag : public TestModule, public IScoreableObjective, public INodeWaypoint, public ITestNodeLabel, public INodeBody {
public:
    static constexpr const char* kTypeName = "TestPlantFlag";
    TestPlantFlag();                                                             // :27 Awake
    ~TestPlantFlag() override;
    std::unique_ptr<ParamChoices_CelestialBodySurface> situation;                // :20
    void Initialized() override;                                                 // :41
    void Cleared() override;                                                     // :47
    bool Test() override;                                                        // :53
    bool HasNodeWaypoint() override;                                             // :89
    FinePrint::Waypoint* GetNodeWaypoint() override;                             // :94
    bool HasNodeBody() override { return true; }                                 // :104
    CelestialBody* GetNodeBody() override;                                       // :109
    bool HasNodeLabel() override;                                                // :114
    bool HasWorldPosition() override;                                            // :119
    Vector3f GetWorldPosition() override;                                        // :124
    std::string GetExtraText() override;                                         // :129
    MENode* node() override { return TestModule::node(); }
    std::string GetInfo() override;                                              // :134
    void Save(ConfigNode& node) const override;                                  // :139
    void Load(const ConfigNode& node) override;                                  // :146
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :153
    void AfterFlagPlanted(FlagSite* flagSite);                                   // :58

private:
    bool eventFound = false;                                                     // :22
    Vessel* v = nullptr;                                                         // :24
    bool subscribed = false;
    std::unique_ptr<FinePrint::Waypoint> nodeWaypoint_;
};

// The roster-aware tests share the "missionKerbal" field text (TestKerbal*.GetNodeBodyParameterString) and the missionKerbal RunValidation.
// TestKerbalKilled.cs:6 (internal)
class TestKerbalKilled : public TestModule {
public:
    static constexpr const char* kTypeName = "TestKerbalKilled";
    TestKerbalKilled();                                                          // :13 Awake
    ~TestKerbalKilled() override;
    std::unique_ptr<MissionKerbal> missionKerbal;                                // :10
    void Initialize(TestGroup* testGroup, const std::string& title = "") override;  // :19
    void Initialized() override;                                                 // :25
    void Cleared() override;                                                     // :41
    void OnKilled(EventReport* report);                                          // :46
    bool Test() override;                                                        // :64
    std::string GetInfo() override;                                              // :69
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :74
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :83
    void Save(ConfigNode& node) const override;                                  // :91
    void Load(const ConfigNode& node) override;                                  // :97

private:
    bool eventFound = false;                                                     // :12
    bool subscribed = false;
};

// TestKerbalRecovery.cs:10 (internal)
class TestKerbalRecovery : public TestModule, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestKerbalRecovery";
    TestKerbalRecovery();                                                        // :20 Awake
    ~TestKerbalRecovery() override;
    std::unique_ptr<MissionKerbal> missionKerbal;                                // :15
    void Initialize(TestGroup* testGroup, const std::string& title = "") override;  // :26
    void Initialized() override;                                                 // :32
    void Cleared() override;                                                     // :43
    void OnRecovered(ProtoVessel* pv, bool enabled);                             // :58
    bool Test() override;                                                        // :84
    std::string GetInfo() override;                                              // :89
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :94
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :103
    void Save(ConfigNode& node) const override;                                  // :111
    void Load(const ConfigNode& node) override;                                  // :117
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :130

private:
    void checkAlreadyRecovered();                                                // :48
    bool eventFound = false;                                                     // :18
    bool subscribed = false;
};

// TestKerbalRescue.cs:10 (internal)
class TestKerbalRescue : public TestModule, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestKerbalRescue";
    TestKerbalRescue();                                                          // :22 Awake
    ~TestKerbalRescue() override;
    std::unique_ptr<MissionKerbal> missionKerbal;                                // :15
    bool mustBoardVessel = false;                                                // :18
    void Initialize(TestGroup* testGroup, const std::string& title = "") override;  // :28
    void Initialized() override;                                                 // :35
    void Cleared() override;                                                     // :77
    bool Test() override;                                                        // :127
    std::string GetInfo() override;                                              // :132
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :137
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :146
    void Save(ConfigNode& node) const override;                                  // :153
    void Load(const ConfigNode& node) override;                                  // :160
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :173
    void OnCrewBoardVessel(GameEvents::FromToAction<Part*, Part*> action);       // :86
    void OnCrewTransferred(GameEvents::HostedFromToAction<ProtoCrewMember*, Part*> action);  // :91
    void onKnowledgeChanged(GameEvents::HostedFromToAction<IDiscoverable*, DiscoveryLevels> kChg);  // :109

private:
    void CheckNewKerbal(const std::string& rescuedName);                         // :96
    bool eventFound = false;                                                     // :20
    bool subscribedBoard = false, subscribedKnowledge = false;
};

// TestKerbalEVA.cs:12 (internal)
class TestKerbalEVA : public TestModule, public IScoreableObjective, public INodeWaypoint, public ITestNodeLabel {
public:
    static constexpr const char* kTypeName = "TestKerbalEVA";
    TestKerbalEVA();                                                             // :27 Awake
    ~TestKerbalEVA() override;
    std::unique_ptr<MissionKerbal> missionKerbal;                                // :16
    std::unique_ptr<ParamChoices_CelestialBodySurface> situation;                // :19
    uint32_t vesselID = 0;                                                       // :22
    void Initialize(TestGroup* testGroup, const std::string& title = "") override;  // :43
    void Initialized() override;                                                 // :49
    void Cleared() override;                                                     // :55
    void OnEVA(GameEvents::FromToAction<Part*, Part*> fromTo);                   // :60
    bool Test() override;                                                        // :104
    bool ShouldCreateCheckpoint() override { return false; }                     // :109
    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) override;   // :114
    bool HasNodeWaypoint() override;                                             // :122
    FinePrint::Waypoint* GetNodeWaypoint() override;                             // :127
    bool HasNodeLabel() override;                                                // :137
    bool HasWorldPosition() override;                                            // :142
    Vector3f GetWorldPosition() override;                                        // :147
    std::string GetExtraText() override;                                         // :152
    MENode* node() override { return TestModule::node(); }
    std::string GetInfo() override;                                              // :157
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :162
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :195
    void Save(ConfigNode& node) const override;                                  // :203
    void Load(const ConfigNode& node) override;                                  // :211
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :226

private:
    bool eventFound = false;                                                     // :24
    Vessel* vessel = nullptr;                                                    // :26
    bool subscribed = false;
    std::unique_ptr<FinePrint::Waypoint> nodeWaypoint_;
};

// TestKerbalBoarded.cs:13 (internal)
class TestKerbalBoarded : public TestModule, public IScoreableObjective, public INodeWaypoint, public ITestNodeLabel {
public:
    static constexpr const char* kTypeName = "TestKerbalBoarded";
    TestKerbalBoarded();                                                         // :26 Awake
    ~TestKerbalBoarded() override;
    std::unique_ptr<MissionKerbal> missionKerbal;                                // :16
    std::unique_ptr<ParamChoices_CelestialBodySurface> situation;                // :19
    uint32_t vesselID = 0;                                                       // :22
    void Initialize(TestGroup* testGroup, const std::string& title = "") override;  // :38
    void Initialized() override;                                                 // :44
    void Cleared() override;                                                     // :51
    void OnBoarded(GameEvents::HostedFromToAction<ProtoCrewMember*, Part*> hostedFromTo);  // :57
    // OnSeated(KerbalEVA eva, bool entered): `eva.part.protoModuleCrew` and `eva.vessel` are passed by the caller (the KerbalEVA is the flight scene's)
    void OnSeated(const std::vector<std::shared_ptr<ProtoCrewMember>>& evaPartCrew, Vessel* evaVessel);  // :66
    bool Test() override;                                                        // :111
    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) override;   // :116
    bool HasNodeWaypoint() override;                                             // :124
    FinePrint::Waypoint* GetNodeWaypoint() override;                             // :129
    bool HasNodeLabel() override;                                                // :139
    bool HasWorldPosition() override;                                            // :144
    Vector3f GetWorldPosition() override;                                        // :149
    std::string GetExtraText() override;                                         // :154
    MENode* node() override { return TestModule::node(); }
    std::string GetInfo() override;                                              // :159
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :164
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :196
    void Save(ConfigNode& node) const override;                                  // :204
    void Load(const ConfigNode& node) override;                                  // :212
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :226

private:
    void OnSeatedEvent(KerbalEVA* eva, bool entered);                            // GameEvents.onCommandSeatInteraction handler
    void CheckSituation();                                                       // :84
    bool eventFound = false;                                                     // :24
    Vessel* vessel = nullptr;                                                    // :26
    bool subscribed = false;
    std::unique_ptr<FinePrint::Waypoint> nodeWaypoint_;
};

// TestTakeKerbal.cs:14 (internal)
class TestTakeKerbal : public TestModule, public IScoreableObjective, public INodeWaypoint, public ITestNodeLabel {
public:
    enum class TestType { AnyVesselAnyKerbal = 0, AnyVesselSpecificKerbal = 1, SpecificVesselAnyKerbal = 2, SpecificVesselSpecificKerbal = 3 };  // :16
    static constexpr const char* kTypeName = "TestTakeKerbal";
    TestTakeKerbal();                                                            // :34 Awake
    ~TestTakeKerbal() override;
    std::unique_ptr<MissionKerbal> missionKerbal;                                // :24
    std::unique_ptr<ParamChoices_CelestialBodySurface> locationSituation;        // :27
    uint32_t vesselID = 0;                                                       // :30
    void Initialize(TestGroup* testGroup, const std::string& title = "") override;  // :46
    void Initialized() override;                                                 // :52
    void Cleared() override;                                                     // :107
    bool Test() override;                                                        // :170
    std::string GetInfo() override;                                              // :183
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :188
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :221
    bool HasNodeWaypoint() override;                                             // :229
    FinePrint::Waypoint* GetNodeWaypoint() override;                             // :234
    bool HasNodeLabel() override;                                                // :244
    bool HasWorldPosition() override;                                            // :249
    Vector3f GetWorldPosition() override;                                        // :254
    std::string GetExtraText() override;                                         // :259
    MENode* node() override { return TestModule::node(); }
    void Save(ConfigNode& node) const override;                                  // :264
    void Load(const ConfigNode& node) override;                                  // :272
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :288
    // the event handlers (public for the tests that drive the events)
    void onVesselChange(Vessel* v);                                              // :114
    void onVesselCrewWasModified(Vessel* v);                                     // :132

private:
    void scanVessels();                                                          // :119
    bool CheckCrew(Vessel* v);                                                   // :137
    bool targetVesselStillValid(Vessel* vessel);                                 // :161
    void testLocation(Vessel* vessel);                                           // :166
    bool testPassed = false;                                                     // :36
    Vessel* targetVessel = nullptr;                                              // :39
    std::string currentBiome;                                                    // :41
    bool subscribedCrew = false, subscribedChange = false;
    std::unique_ptr<FinePrint::Waypoint> nodeWaypoint_;
};

// TestCrewAssignment.cs:12 (internal)
class TestCrewAssignment : public TestModule, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestCrewAssignment";
    TestCrewAssignment();                                                        // :21 Awake
    std::unique_ptr<VesselPartIDPair> vesselPartIDs;                             // :15
    std::unique_ptr<MissionKerbal> missionKerbal;                                // :18
    void Initialize(TestGroup* testGroup, const std::string& title = "") override;  // :28
    bool Test() override;                                                        // :34
    void OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) override;  // :97
    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) override;   // :107
    void OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) override;      // :116
    std::string GetInfo() override;                                              // :140
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :145
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :183
    void Save(ConfigNode& node) const override;                                  // :195
    void Load(const ConfigNode& node) override;                                  // :203
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :217

private:
    bool partContainsKerbal(const std::vector<std::shared_ptr<ProtoCrewMember>>& crew);  // :84
    Vessel* vessel = nullptr;                                                    // :17
};

// TestOrbit.cs:15
class TestOrbit : public TestVessel, public IScoreableObjective, public INodeOrbit {
public:
    static constexpr const char* kTypeName = "TestOrbit";
    TestOrbit();                                                                 // :40 Awake
    double orbitAccuracy = 0.0;                                                  // :18
    std::unique_ptr<MissionOrbit> missionOrbit;                                  // :27 (private)
    std::unique_ptr<Orbit> relativeOrbit;                                        // :29
    double stabilizationTime = 5.0;                                              // :21
    bool underThrust = false;                                                    // :24
    double OrbitDeviation() const;                                               // :36
    void Initialized() override;                                                 // :48
    void OnOrbitAccuracyChanged(double newValue);                                // :55
    void DrawOrbit();                                                            // :69
    void Cleared() override;                                                     // :79
    bool Test() override;                                                        // :124
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :163
    bool HasNodeOrbit() override { return true; }                                // :173
    Orbit* GetNodeOrbit() override;                                              // :178
    std::string GetInfo() override;                                              // :183
    void Save(ConfigNode& node) const override;                                  // :188
    void Load(const ConfigNode& node) override;                                  // :198
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :216

protected:
    bool situationSuccess = false;                                               // :31
    bool orbitSuccess = false;                                                   // :33
    double successStartTime = -1.0;                                              // :35
};

// TestOrbitParams.cs:13
class TestOrbitParams : public TestVessel, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestOrbitParams";
    TestOrbitParams();                                                           // :89 Awake
    bool checkApoapsis = false;                                                  // :16
    double apMinValue = 0.0, apMaxValue = 0.0;                                   // :19 / :22
    bool checkPeriapsis = false;                                                 // :25
    double peMinValue = 0.0, peMaxValue = 0.0;                                   // :28 / :31
    bool checkInclination = false;                                               // :34
    double inclination = 0.0;                                                    // :37
    double inclinationAccuracy = 0.0;                                            // :40
    bool checkLAN = false;                                                       // :43
    double LAN = 0.0;                                                            // :46
    double LANAccuracy = 0.0;                                                    // :49
    std::unique_ptr<MissionCelestialBody> body;                                  // :52
    double stabilizationTime = 5.0;                                              // :55
    bool underThrust = false;                                                    // :58
    double InclinationDeviation() const;                                         // :80
    double LANDeviation() const;                                                 // :82
    bool Test() override;                                                        // :362
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :476
    std::string GetInfo() override;                                              // :536
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :541
    void Save(ConfigNode& node) const override;                                  // :557
    void Load(const ConfigNode& node) override;                                  // :578
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :601
    // the accuracy value of GetScoreModifier(ScoreModule_Accuracy) (the C# returns a boxed int 1 or double)
    static double GetMaxAp(CelestialBody* body);                                 // :313

protected:
    bool situationSuccess = false;                                               // :61
    bool orbitSuccess = false;                                                   // :63
    double successStartTime = -1.0;                                              // :65
};

// TestScienceExperiment.cs:13
class TestScienceExperiment : public TestModule, public IScoreableObjective, public INodeBody {
public:
    static constexpr const char* kTypeName = "TestScienceExperiment";
    TestScienceExperiment();                                                     // :28 Awake
    ~TestScienceExperiment() override;
    std::string experimentID;                                                    // :16
    std::string experimentSituation;                                             // :19
    std::unique_ptr<Editor::MissionBiome> biomeData;                             // :22
    std::string DeployorReceived;                                                // :28
    void Initialized() override;                                                 // :39
    void Cleared() override;                                                     // :46
    void OnScienceReceived(float scienceValue, ScienceSubject* scienceSubjectIn, ProtoVessel* vessel, bool reverseEngineered);  // :53
    void OnExperimentDeployed(ScienceData* scienceDataIn);                       // :61
    bool Test() override;                                                        // :69
    bool HasNodeBody() override;                                                 // :119
    CelestialBody* GetNodeBody() override;                                       // :128
    std::string GetInfo() override;                                              // :138
    void Save(ConfigNode& node) const override;                                  // :143
    void Load(const ConfigNode& node) override;                                  // :163
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :181
    std::string GetAppObjectiveInfo() override;                                  // :224
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :243
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :270
    // ConductScienceTest(subject id) :74 (the two handlers pass ScienceSubject.id / ScienceData.subjectID)
    void ConductScienceTest(const std::string& inputExperimentId);

private:
    bool testsSuccessful = false;                                                // :24
    bool passedTests = false;                                                    // :31
    bool subscribed = false;
};

// TestGrapple.cs:12
class TestGrapple : public TestModule {
public:
    enum class SpaceObjectChoices { Asteroid = 0, Comet = 1, Vessel = 2 };       // :14
    static constexpr const char* kTypeName = "TestGrapple";
    TestGrapple();                                                               // :53 Awake
    ~TestGrapple() override;
    uint32_t grapplingVesselID = 0;                                              // :25
    SpaceObjectChoices grappledSpaceObject = SpaceObjectChoices::Asteroid;       // :31
    uint32_t grappleAsteroidID = 0;                                              // :36
    uint32_t grappleCometID = 0;                                                 // :39
    uint32_t grappleVesselID = 0;                                                // :42
    std::string dockedUndocked;                                                  // :51
    void Initialized() override;                                                 // :109
    void Cleared() override;                                                     // :124
    bool Test() override;                                                        // :245
    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) override;   // :257
    void OnVesselDocking(uint32_t oldId, uint32_t newId) override;               // :281
    std::string GetInfo() override;                                              // :291
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :296
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :372
    void Save(ConfigNode& node) const override;                                  // :398
    void Load(const ConfigNode& node) override;                                  // :409
    void onPartCouple(GameEvents::FromToAction<Part*, Part*> partAction);        // :135
    void onPartUndock(Part* part);                                               // :140

private:
    void testPartVessels();                                                      // :145
    uint32_t grappleSOPartID = 0;                                                // :46
    Part* grabbingPart_ = nullptr;                                               // :20 grabbingUnit (the part holding the ModuleGrappleNode)
    bool firstRunTest = true;                                                    // :53
    bool testSuccess = false;                                                    // :55
    bool subscribed = false;
};

// TestPartDocking.cs:12
class TestPartDocking : public TestModule {
public:
    static constexpr const char* kTypeName = "TestPartDocking";
    TestPartDocking();                                                           // :29 Awake
    ~TestPartDocking() override;
    std::unique_ptr<VesselPartIDPair> partOnevesselPartIDs;                      // :16
    std::unique_ptr<VesselPartIDPair> partTwovesselPartIDs;                      // :19
    std::string dockedUndocked;                                                  // :22
    void Initialized() override;                                                 // :46
    void Cleared() override;                                                     // :114
    bool Test() override;                                                        // :172
    void OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) override;  // :183
    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) override;   // :197
    void OnVesselDocking(uint32_t oldId, uint32_t newId) override;               // :211
    std::string GetInfo() override;                                              // :219
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :224
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :300
    void Save(ConfigNode& node) const override;                                  // :335
    void Load(const ConfigNode& node) override;                                  // :347
    void onPartCouple(GameEvents::FromToAction<Part*, Part*> partAction);        // :124
    void onPartUndock(Part* part);                                               // :129

private:
    void setPlayerCreatedRootParts();                                            // :74
    void testPartVessels();                                                      // :134
    bool findPartVesselId(uint32_t partId, uint32_t& vesselId);                  // :155
    bool firstRunTest = true;                                                    // :24
    bool testSuccess = false;                                                    // :26
    bool subscribed = false;
};

// Registers the tests of this header in TestModuleRegistry (called by RegisterNodeLibrary()).
void RegisterNodeLibrarySlice2Tests();

}  // namespace Tests
}  // namespace ksp::Expansions::Missions

template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Tests::TestVesselVelocity::SpeedType> {
    using E = ksp::Expansions::Missions::Tests::TestVesselVelocity::SpeedType;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::SurfaceVelocity, "SurfaceVelocity", "#autoLOC_8004167"}, {E::OrbitalVelocity, "OrbitalVelocity", "#autoLOC_8004168"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Tests::TestVesselVelocity::VelocityReferenceFrame> {
    using E = ksp::Expansions::Missions::Tests::TestVesselVelocity::VelocityReferenceFrame;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Default, "Default", "#autoLOC_8005455"}, {E::Asteroid, "Asteroid", "#autoLOC_8000046"},
                                                {E::Comet, "Comet", "#autoLOC_6005065"},   {E::Kerbal, "Kerbal", "#autoLOC_7000020"},
                                                {E::Vessel, "Vessel", "#autoLOC_8000001"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Tests::TestGrapple::SpaceObjectChoices> {
    using E = ksp::Expansions::Missions::Tests::TestGrapple::SpaceObjectChoices;
    static constexpr bool flags = false;
    static constexpr EnumMember<E> members[] = {{E::Asteroid, "Asteroid", "#autoLOC_8000046"}, {E::Comet, "Comet", "#autoLOC_6005065"}, {E::Vessel, "Vessel", "#autoLOC_8000001"}};
};
