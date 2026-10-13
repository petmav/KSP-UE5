// WP-54c: Assembly-CSharp/Expansions/Missions/Tests/TestDistance.cs (Expansions.Missions.Tests.TestDistance, 875 lines of which about 400 are the editor's drop-down
// plumbing). Ported: the test logic (Test, TestPointsDistance, GetVesselLocation, GetKerbalLocation, CheckCrew), Awake, Load / Save, RunValidation,
// GetNodeBodyParameterString, GetScoreModifier, OnVesselPersistentIdChanged and the visibility rules of OnDistanceFromValueChange /
// OnDistanceToValueChange (on IMEGUIDropdownList-like seams). Conventions: see node_library.hpp.
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ksp/missions/mission_data.hpp"
#include "ksp/missions/mission_tests.hpp"
#include "ksp/missions/mission_tests2.hpp"

namespace ksp::Expansions::Missions::Tests {

// TestDistance.cs:20 (internal)
class TestDistance : public TestModule, public IScoreableObjective {
public:
    enum class DistanceFromChoices { Kerbal = 0, Vessel = 1 };  // :22
    enum class DistanceToChoices { Asteroid = 0, Comet = 1, CelestialBody = 2, Flag = 3, Kerbal = 4, LaunchSite = 5, Location = 6, NodeLabelNode = 7, Vessel = 8 };  // :30
    enum class DistanceCalculationType { StraightLine = 0, GreatCircle = 1 };  // :56

    static constexpr const char* kTypeName = "TestDistance";
    TestDistance();                                                              // :143 Awake
    DistanceFromChoices distanceFromTarget = DistanceFromChoices::Vessel;        // :64
    std::unique_ptr<MissionKerbal> distanceFromKerbal;                           // :70
    uint32_t distanceFromVesselID = 0;                                           // :79
    DistanceToChoices distanceToTarget = DistanceToChoices::Vessel;              // :86
    std::unique_ptr<MissionKerbal> distanceToKerbal;                             // :92
    uint32_t distanceToVesselID = 0;                                             // :98
    std::unique_ptr<VesselGroundLocation> distanceToLocation;                    // :104
    std::unique_ptr<MissionCelestialBody> distanceToCelestialBody;               // :110
    uint32_t distanceToAsteroidID = 0;                                           // :113
    uint32_t distanceToCometID = 0;                                              // :116
    uint32_t distanceToFlagID = 0;                                               // :124
    std::string distanceToLaunchSiteName = "LaunchPad";                          // :130
    Guid distanceToNode;                                                         // :140
    DistanceCalculationType calculationType = DistanceCalculationType::StraightLine;  // :148
    TestComparisonLessGreaterOnly comparisonOperator = TestComparisonLessGreaterOnly::GreaterThan;  // :151
    double distance = 0.0;                                                       // :154

    // editor control references (OnXControlCreated); null = no control
    Editor::IMEGUIDropdownList* distanceFromTargetParameterReference = nullptr;  // :66
    Editor::IMEGUIControl* distanceFromKerbalParameterReference = nullptr;       // :69 (MEGUIParameterMissionKerbal)
    Editor::IMEGUIControl* distanceFromVesselParameterReference = nullptr;       // :81 (MEGUIParameterVesselDropdownList)
    Editor::IMEGUIDropdownList* distanceToTargetParameterReference = nullptr;    // :88
    Editor::IMEGUIControl* distanceToKerbalParameterReference = nullptr;         // :95
    Editor::IMEGUIControl* distanceToVesselParameterReference = nullptr;         // :101
    Editor::IMEGUIControl* distanceToLocationParameterReference = nullptr;       // :107
    Editor::IMEGUIControl* distanceToCelestialBodyParameterReference = nullptr;  // :113
    Editor::IMEGUIControl* distanceToAsteroidParameterReference = nullptr;       // :119
    Editor::IMEGUIControl* distanceToCometParameterReference = nullptr;          // :121
    Editor::IMEGUIControl* distanceToFlagParameterReference = nullptr;           // :127
    Editor::IMEGUIControl* distanceToLaunchSiteParameterReference = nullptr;     // :135
    Editor::IMEGUIControl* distanceToNodeParameterReference = nullptr;           // :145

    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) override;   // :157
    void Initialize(TestGroup* testGroup, const std::string& title = "") override;  // :387
    bool Test() override;                                                        // :400
    void RunValidation(Editor::MissionEditorValidator* validator) override;      // :629
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :679
    std::string GetInfo() override;                                              // :791
    void Save(ConfigNode& node) const override;                                  // :796
    void Load(const ConfigNode& node) override;                                  // :824
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override;     // :866

    void OnDistanceFromValueChange(Editor::IMEGUIDropdownList* sender, int newIndex);  // :196
    void OnDistanceToValueChange(Editor::IMEGUIDropdownList* sender, int newIndex);    // :286
    void ParameterSetupComplete() override;                                      // :394

private:
    bool TestPointsDistance(const Vector3d& pointA, const Vector3d& pointB, CelestialBody* body);  // :520
    bool GetVesselLocation(uint32_t vesselID, Vector3d& location, CelestialBody*& celestialBody);   // :549
    bool GetKerbalLocation(MissionKerbal& missionKerbal, Vessel*& currentVessel, Vector3d& location, CelestialBody*& celestialBody);  // :569
    bool CheckCrew(Vessel* v, MissionKerbal& missionKerbal);                    // :610

    Vessel* distanceFromKerbalVessel = nullptr;                                  // :72
    Vessel* distanceToKerbalVessel = nullptr;                                    // :84
    std::function<Vector3d()> distanceToFacility;                                // :132 (PSystemSetup.SpaceCenterFacility: facilityTransform.position)
    std::function<Vector3d()> distanceToLaunchSite;                              // :134 (LaunchSite: GetWorldPos().position)
    ITestNodeLabel* distanceToNodeLabelInterface = nullptr;                      // :142
};

}  // namespace ksp::Expansions::Missions::Tests

template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Tests::TestDistance::DistanceFromChoices> {
    using E = ksp::Expansions::Missions::Tests::TestDistance::DistanceFromChoices;
    static constexpr bool flags = false;
    // TestDistance.cs:22
    static constexpr EnumMember<E> members[] = {{E::Kerbal, "Kerbal", "#autoLOC_7000020"}, {E::Vessel, "Vessel", "#autoLOC_8000001"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Tests::TestDistance::DistanceToChoices> {
    using E = ksp::Expansions::Missions::Tests::TestDistance::DistanceToChoices;
    static constexpr bool flags = false;
    // TestDistance.cs:30
    static constexpr EnumMember<E> members[] = {{E::Asteroid, "Asteroid", "#autoLOC_8000046"},     {E::Comet, "Comet", "#autoLOC_6005065"},
                                                {E::CelestialBody, "CelestialBody", "#autoLOC_8000263"}, {E::Flag, "Flag", "#autoLoc_6002179"},
                                                {E::Kerbal, "Kerbal", "#autoLOC_7000020"},         {E::LaunchSite, "LaunchSite", "#autoLOC_8000067"},
                                                {E::Location, "Location", "#autoLOC_8004145"},     {E::NodeLabelNode, "NodeLabelNode", "#autoLOC_8004199"},
                                                {E::Vessel, "Vessel", "#autoLOC_8000001"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Tests::TestDistance::DistanceCalculationType> {
    using E = ksp::Expansions::Missions::Tests::TestDistance::DistanceCalculationType;
    static constexpr bool flags = false;
    // TestDistance.cs:56
    static constexpr EnumMember<E> members[] = {{E::StraightLine, "StraightLine", "#autoLOC_8004207"}, {E::GreatCircle, "GreatCircle", "#autoLOC_8004208"}};
};
