// WP-54c: Assembly-CSharp/Expansions/Missions/Tests/TestDistance.cs. See mission_tests4.hpp.
// Cite placeholders `File.cs:@Name[#k]` are resolved by tools/wp55/check_cites.py --resolve.
#include "ksp/missions/mission_tests4.hpp"

#include <cmath>

#include "../save/save_util.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_actions2.hpp"
#include "ksp/missions/mission_system.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Missions::Tests {

namespace {

using KSP::Localization::Localizer;
using NLE = NodeLibraryEnvironment;
using PCM = std::shared_ptr<ProtoCrewMember>;

std::string Tag(const char* t) { return Localizer::Format(t); }
std::string Fmt(const char* tag, std::initializer_list<std::string> args) { return Localizer::Format(tag, std::vector<std::string>(args)); }
CelestialBody* Home() { return NLE::HomeBody(); }
// FlightGlobals.PersistentVesselIds.ContainsKey(id)
bool PersistentContains(uint32_t id) { return AwardEnvironment::VesselByPersistentId(id) != nullptr; }
Vector3d V3d(const Vector3d& v) { return v; }

}  // namespace

// TestDistance.cs:143
TestDistance::TestDistance() : TestModule(kTypeName) {
    title = Tag("#autoLOC_8000176");
    distanceFromKerbal = std::make_unique<MissionKerbal>(nullptr, node(), [this] { UpdateNodeBodyUI(); }, ProtoCrewMember::KerbalType::Crew, false, true);
    distanceToKerbal = std::make_unique<MissionKerbal>(nullptr, node(), [this] { UpdateNodeBodyUI(); }, ProtoCrewMember::KerbalType::Crew, false);
    distanceToLocation = std::make_unique<VesselGroundLocation>(node(), VesselGroundLocation::GizmoIcon::Flag);
    distanceToCelestialBody = std::make_unique<MissionCelestialBody>(Home());
    if (node() != nullptr) node()->UpdateInfoOnRefresh = true;
}

// TestDistance.cs:157
void TestDistance::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    if (distanceFromVesselID == oldId) distanceFromVesselID = newId;
    if (distanceToVesselID == oldId) distanceToVesselID = newId;
    if (distanceToAsteroidID == oldId) distanceToAsteroidID = newId;
    if (distanceToCometID == oldId) distanceToCometID = newId;
    if (distanceToFlagID == oldId) distanceToFlagID = newId;
}

// TestDistance.cs:196
void TestDistance::OnDistanceFromValueChange(Editor::IMEGUIDropdownList* sender, int) {
    const std::optional<int> selected = sender->SelectedValueInt();
    if (selected.has_value() && node() != nullptr) {
        switch (static_cast<DistanceFromChoices>(*selected)) {
            case DistanceFromChoices::Vessel:
                if (distanceFromKerbalParameterReference != nullptr) distanceFromKerbalParameterReference->SetActive(false);
                if (distanceFromVesselParameterReference != nullptr) distanceFromVesselParameterReference->SetActive(true);
                break;
            case DistanceFromChoices::Kerbal:
                if (distanceFromKerbalParameterReference != nullptr) distanceFromKerbalParameterReference->SetActive(true);
                if (distanceFromVesselParameterReference != nullptr) distanceFromVesselParameterReference->SetActive(false);
                break;
        }
    }
    UpdateNodeBodyUI();
}

// TestDistance.cs:286
void TestDistance::OnDistanceToValueChange(Editor::IMEGUIDropdownList* sender, int) {
    for (Editor::IMEGUIControl* c : {distanceToKerbalParameterReference, distanceToVesselParameterReference, distanceToLocationParameterReference,
                                     distanceToCelestialBodyParameterReference, distanceToAsteroidParameterReference, distanceToCometParameterReference,
                                     distanceToFlagParameterReference, distanceToLaunchSiteParameterReference, distanceToNodeParameterReference})
        if (c != nullptr) c->SetActive(false);
    const std::optional<int> selected = sender->SelectedValueInt();
    if (selected.has_value() && node() != nullptr) {
        Editor::IMEGUIControl* shown = nullptr;
        switch (static_cast<DistanceToChoices>(*selected)) {
            case DistanceToChoices::Asteroid: shown = distanceToAsteroidParameterReference; break;
            case DistanceToChoices::Comet: shown = distanceToCometParameterReference; break;
            case DistanceToChoices::CelestialBody: shown = distanceToCelestialBodyParameterReference; break;
            case DistanceToChoices::Flag: shown = distanceToFlagParameterReference; break;
            case DistanceToChoices::Kerbal: shown = distanceToKerbalParameterReference; break;
            case DistanceToChoices::LaunchSite: shown = distanceToLaunchSiteParameterReference; break;
            case DistanceToChoices::Location: shown = distanceToLocationParameterReference; break;
            case DistanceToChoices::NodeLabelNode: shown = distanceToNodeParameterReference; break;
            case DistanceToChoices::Vessel: shown = distanceToVesselParameterReference; break;
        }
        if (shown != nullptr) shown->SetActive(true);
    }
    UpdateNodeBodyUI();
}

// TestDistance.cs:387
void TestDistance::Initialize(TestGroup* testGroup, const std::string& newTitle) {
    TestModule::Initialize(testGroup, newTitle);
    distanceFromKerbal->Initialize(nullptr, testGroup->node());
    distanceToKerbal->Initialize(nullptr, testGroup->node());
}

// TestDistance.cs:394
void TestDistance::ParameterSetupComplete() {
    if (distanceFromTargetParameterReference == nullptr || distanceToTargetParameterReference == nullptr) throw NullReferenceException();
    OnDistanceFromValueChange(distanceFromTargetParameterReference, distanceFromTargetParameterReference->GetItemIndex(distanceFromTargetParameterReference->SelectedValueInt()));
    OnDistanceToValueChange(distanceToTargetParameterReference, distanceToTargetParameterReference->GetItemIndex(distanceToTargetParameterReference->SelectedValueInt()));
}

// TestDistance.cs:400
bool TestDistance::Test() {
    bool result = false;
    if (AwardEnvironment::ActiveVessel() != nullptr) {
        Vector3d location = Vector3d::zero();
        bool flag = false;
        CelestialBody* celestialBody = nullptr;
        switch (distanceFromTarget) {
            case DistanceFromChoices::Vessel:
                flag = GetVesselLocation(distanceFromVesselID, location, celestialBody);
                break;
            case DistanceFromChoices::Kerbal:
                flag = GetKerbalLocation(*distanceFromKerbal, distanceFromKerbalVessel, location, celestialBody);
                break;
        }
        Vector3d location2 = Vector3d::zero();
        bool flag2 = false;
        CelestialBody* celestialBody2 = nullptr;
        switch (distanceToTarget) {
            case DistanceToChoices::Asteroid:
                if (distanceToAsteroidID != 0) flag2 = GetVesselLocation(distanceToAsteroidID, location2, celestialBody2);
                break;
            case DistanceToChoices::Comet:
                if (distanceToCometID != 0) flag2 = GetVesselLocation(distanceToCometID, location2, celestialBody2);
                break;
            case DistanceToChoices::CelestialBody: {
                CelestialBody* celestialBody3 = distanceToCelestialBody->Body();
                if (celestialBody3 == nullptr) celestialBody3 = celestialBody;
                if (celestialBody3 == nullptr) throw NullReferenceException();
                location2 = celestialBody3->position + (location - celestialBody3->position).normalized() * celestialBody3->Radius;
                flag2 = true;
                break;
            }
            case DistanceToChoices::Flag:
                if (distanceToFlagID != 0) flag2 = GetVesselLocation(distanceToFlagID, location2, celestialBody2);
                break;
            case DistanceToChoices::Kerbal:
                flag2 = GetKerbalLocation(*distanceToKerbal, distanceToKerbalVessel, location2, celestialBody2);
                break;
            case DistanceToChoices::LaunchSite:
                if (!distanceToFacility && !distanceToLaunchSite) distanceToFacility = NLE::getSpaceCenterFacility ? NLE::getSpaceCenterFacility(distanceToLaunchSiteName) : std::function<Vector3d()>();
                if (distanceToFacility) {
                    location2 = distanceToFacility();
                    flag2 = true;
                }
                if (!flag2) {
                    if (!distanceToFacility && !distanceToLaunchSite) distanceToLaunchSite = NLE::getLaunchSite ? NLE::getLaunchSite(distanceToLaunchSiteName) : std::function<Vector3d()>();
                    if (distanceToLaunchSite) {
                        location2 = distanceToLaunchSite();
                        flag2 = true;
                    }
                }
                break;
            case DistanceToChoices::Location:
                location2 = Vector3d(distanceToLocation->GetWorldPosition());
                flag2 = true;
                break;
            case DistanceToChoices::NodeLabelNode:
                if (distanceToNodeLabelInterface == nullptr && !(distanceToNode == Guid::Empty()) && !Runtime::MissionSystem::missions.empty() &&
                    Runtime::MissionSystem::missions[0]->nodes.Contains(distanceToNode)) {
                    MENode* mENode = Runtime::MissionSystem::missions[0]->nodes[distanceToNode];
                    for (size_t i = 0; i < mENode->testGroups.size(); ++i) {
                        if (distanceToNodeLabelInterface != nullptr) break;
                        for (size_t j = 0; j < mENode->testGroups[i]->testModules.size(); ++j) {
                            if (distanceToNodeLabelInterface != nullptr) break;
                            distanceToNodeLabelInterface = dynamic_cast<ITestNodeLabel*>(mENode->testGroups[i]->testModules[j].get());
                        }
                    }
                }
                if (distanceToNodeLabelInterface != nullptr && distanceToNodeLabelInterface->HasWorldPosition()) {
                    location2 = Vector3d(distanceToNodeLabelInterface->GetWorldPosition());
                    flag2 = true;
                }
                break;
            case DistanceToChoices::Vessel:
                flag2 = GetVesselLocation(distanceToVesselID, location2, celestialBody2);
                break;
        }
        if (!flag || !flag2) return false;
        result = TestPointsDistance(location, location2, celestialBody);
    }
    return result;
}

// TestDistance.cs:520
bool TestDistance::TestPointsDistance(const Vector3d& pointA, const Vector3d& pointB, CelestialBody* body) {
    double num = 0.0;
    switch (calculationType) {
        case DistanceCalculationType::GreatCircle:
            if (body == nullptr) return false;
            num = Contracts::Lite::CelestialUtilities::GreatCircleDistance(body, pointA, pointB);
            break;
        case DistanceCalculationType::StraightLine:
            num = Vector3d::Distance(pointA, pointB);
            break;
    }
    bool result = false;
    switch (comparisonOperator) {
        case TestComparisonLessGreaterOnly::GreaterThan:
            result = num >= distance;
            break;
        case TestComparisonLessGreaterOnly::LessThan:
            result = num <= distance;
            break;
    }
    return result;
}

// TestDistance.cs:549
bool TestDistance::GetVesselLocation(uint32_t vesselID, Vector3d& location, CelestialBody*& celestialBody) {
    if (vesselID != 0) {
        vesselID = node()->mission->CurrentVesselID(node(), vesselID);
        if (!PersistentContains(vesselID)) return false;
    }
    Vessel* vessel = (vesselID == 0) ? AwardEnvironment::ActiveVessel() : AwardEnvironment::VesselByPersistentId(vesselID);
    if (vessel != nullptr) {
        location = vessel->GetWorldPos3D();
        celestialBody = vessel->mainBody();
        return true;
    }
    return false;
}

// TestDistance.cs:569
bool TestDistance::GetKerbalLocation(MissionKerbal& missionKerbal, Vessel*& currentVessel, Vector3d& location, CelestialBody*& celestialBody) {
    if (!CheckCrew(currentVessel, missionKerbal) || (missionKerbal.Kerbal() == nullptr && currentVessel != AwardEnvironment::ActiveVessel())) currentVessel = nullptr;
    if (currentVessel == nullptr) {
        if (missionKerbal.Kerbal() == nullptr) {
            Vessel* activeVessel = AwardEnvironment::ActiveVessel();
            if (CheckCrew(activeVessel, missionKerbal)) currentVessel = activeVessel;
        } else {
            std::vector<Vessel*> vessels = AwardEnvironment::Vessels();
            for (size_t i = 0; i < vessels.size(); ++i) {
                if (!(currentVessel == nullptr)) break;
                if (CheckCrew(vessels[i], missionKerbal)) currentVessel = vessels[i];
            }
        }
    }
    if (currentVessel != nullptr) {
        location = currentVessel->GetWorldPos3D();
        celestialBody = currentVessel->mainBody();
        return true;
    }
    return false;
}

// TestDistance.cs:571
bool TestDistance::CheckCrew(Vessel* v, MissionKerbal& missionKerbal) {
    if (v == nullptr) return false;
    const std::vector<PCM>& vesselCrew = v->GetVesselCrew();
    int count = static_cast<int>(vesselCrew.size());
    do {
        if (count-- <= 0) return false;
    } while (!missionKerbal.IsValid(*vesselCrew[static_cast<size_t>(count)]));
    return true;
}

// TestDistance.cs:629
void TestDistance::RunValidation(Editor::MissionEditorValidator* validator) {
    if (distanceFromTarget == DistanceFromChoices::Kerbal && distanceToTarget == DistanceToChoices::Kerbal && distanceFromKerbal->Name() == distanceToKerbal->Name())
        validator->AddNodeFail(node(), Tag("#autoLOC_8004219"));
    if (distanceFromTarget == DistanceFromChoices::Vessel && distanceToTarget == DistanceToChoices::Vessel && distanceFromVesselID == distanceToVesselID)
        validator->AddNodeFail(node(), Tag("#autoLOC_8004220"));
    if (distanceFromTarget == DistanceFromChoices::Kerbal) {
        if (distanceFromKerbal->Kerbal() != nullptr) {
            PCM p = (*node()->mission->situation->crewRoster)[distanceFromKerbal->Kerbal()->name().value_or("")];
            if (p == nullptr) throw NullReferenceException();
            if (p->rosterStatus() != ProtoCrewMember::RosterStatus::Assigned) validator->AddNodeWarn(node(), Fmt("#autoLOC_8002033", {distanceFromKerbal->Name()}));
        }
        // `distanceFromKerbal == null` (the MissionKerbal object, never null) && TypeToShow == Tourist
        if (distanceFromKerbal == nullptr && distanceFromKerbal->TypeToShow() == ProtoCrewMember::KerbalType::Tourist) validator->AddNodeFail(node(), Tag("#autoLOC_8004221"));
    }
    if (distanceToTarget == DistanceToChoices::Kerbal) {
        if (distanceToKerbal->Kerbal() != nullptr) {
            PCM p = (*node()->mission->situation->crewRoster)[distanceToKerbal->Kerbal()->name().value_or("")];
            if (p == nullptr) throw NullReferenceException();
            if (p->rosterStatus() != ProtoCrewMember::RosterStatus::Assigned) validator->AddNodeWarn(node(), Fmt("#autoLOC_8002033", {distanceToKerbal->Name()}));
        }
        if (distanceToKerbal == nullptr) validator->AddNodeFail(node(), Tag("#autoLOC_8004222"));
    }
    if (distanceToTarget == DistanceToChoices::Asteroid && distanceToAsteroidID == 0) validator->AddNodeFail(node(), Tag("#autoLOC_8004223"));
    if (distanceToTarget == DistanceToChoices::Comet && distanceToCometID == 0) validator->AddNodeFail(node(), Tag("#autoLOC_8005448"));
    if (distanceToTarget == DistanceToChoices::Flag && distanceToFlagID == 0) validator->AddNodeFail(node(), Tag("#autoLOC_8004224"));
    if (distanceToTarget == DistanceToChoices::NodeLabelNode) {
        Mission* editorMission = MissionsEnvironment::editorMission ? MissionsEnvironment::editorMission() : nullptr;
        if (editorMission == nullptr) throw NullReferenceException();  // MissionEditorLogic.Instance.EditorMission
        if (!editorMission->nodes.ContainsKey(distanceToNode)) validator->AddNodeFail(node(), Tag("#autoLOC_8004225"));
    }
}

// TestDistance.cs:679
std::string TestDistance::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (node() != nullptr) node()->UpdateInfoOnRefresh = true;
    if (field.name == "distanceFromTarget") {
        if (node() == nullptr) throw NullReferenceException();
        if (node()->mission != nullptr) {
            std::string text = TestModule::GetNodeBodyParameterString(field) + "\n";
            switch (distanceFromTarget) {
                case DistanceFromChoices::Vessel: {
                    std::string text2;
                    std::string vesselName;
                    bool flag = false;
                    VesselSituation* vesselSituation = node()->mission->GetVesselSituationByVesselID(distanceFromVesselID);
                    if (vesselSituation != nullptr) {
                        vesselName = vesselSituation->vesselName;
                    } else if (node()->mission->UpdateFromMappedVesselIDs(distanceFromVesselID, vesselName, vesselSituation)) {
                        flag = true;
                    }
                    text2 = (vesselSituation != nullptr || flag) ? vesselName : ((distanceFromVesselID != 0) ? Tag("#autoLOC_8100159") : Tag("#autoLOC_8004217"));
                    std::string text3 = text + Fmt("#autoLOC_8004190", {Tag("#autoLOC_8000001"), text2});
                    if (flag && vesselSituation != nullptr) text3 = text3 + "\n" + Fmt("#autoLOC_6002491", {vesselSituation->vesselName}) + "\n";
                    return text3;
                }
                case DistanceFromChoices::Kerbal:
                    return text + distanceFromKerbal->GetNodeBodyParameterString();
            }
        }
    }
    if (field.name == "distanceToTarget") {
        if (node() == nullptr) throw NullReferenceException();
        std::string text4 = TestModule::GetNodeBodyParameterString(field) + "\n";
        if (node()->mission == nullptr) throw NullReferenceException();
        switch (distanceToTarget) {
            case DistanceToChoices::Asteroid: {
                std::string text10;
                Asteroid* asteroidByPersistentID = node()->mission->GetAsteroidByPersistentID(distanceToAsteroidID);
                text10 = (asteroidByPersistentID != nullptr) ? asteroidByPersistentID->name : Tag("#autoLOC_6003000");
                return text4 + Fmt("#autoLOC_8004190", {Tag("#autoLOC_8000046"), text10});
            }
            case DistanceToChoices::Comet: {
                std::string text9;
                Comet* cometByPersistentID = node()->mission->GetCometByPersistentID(distanceToCometID);
                text9 = (cometByPersistentID != nullptr) ? Localizer::Format(cometByPersistentID->name) : Tag("#autoLOC_6003000");
                return text4 + Fmt("#autoLOC_8004190", {Tag("#autoLOC_6005065"), text9});
            }
            case DistanceToChoices::CelestialBody:
                return text4 + distanceToCelestialBody->GetNodeBodyParameterString();
            case DistanceToChoices::Flag: {
                std::string text7;
                auto* actionCreateFlagByPersistentID = dynamic_cast<Actions::ActionCreateFlag*>(node()->mission->GetActionCreateFlagByPersistentID(distanceToFlagID));
                text7 = (actionCreateFlagByPersistentID != nullptr) ? actionCreateFlagByPersistentID->siteName : Tag("#autoLOC_6003000");
                return text4 + Fmt("#autoLOC_8004190", {Tag("#autoLoc_6002179"), text7});
            }
            case DistanceToChoices::Kerbal:
                return text4 + distanceToKerbal->GetNodeBodyParameterString();
            case DistanceToChoices::LaunchSite: {
                std::string display = NLE::launchSiteDisplayName ? NLE::launchSiteDisplayName(distanceToLaunchSiteName)
                                      : (MissionDataEnvironment::launchSiteDisplayName ? MissionDataEnvironment::launchSiteDisplayName(distanceToLaunchSiteName) : distanceToLaunchSiteName);
                return text4 + Fmt("#autoLOC_8004190", {Tag("#autoLOC_8000067"), display});
            }
            case DistanceToChoices::Location:
                return text4 + distanceToLocation->GetNodeBodyParameterString();
            case DistanceToChoices::NodeLabelNode: {
                std::string text8;
                text8 = (!node()->mission->nodes.ContainsKey(distanceToNode)) ? Tag("#autoLOC_6003000") : node()->mission->nodes[distanceToNode]->Title();
                return text4 + Fmt("#autoLOC_8004190", {Tag("#autoLOC_8004199"), text8});
            }
            case DistanceToChoices::Vessel: {
                std::string text5;
                std::string vesselName2;
                bool flag2 = false;
                VesselSituation* vesselSituation2 = node()->mission->GetVesselSituationByVesselID(distanceToVesselID);
                if (vesselSituation2 != nullptr) {
                    vesselName2 = vesselSituation2->vesselName;
                } else if (node()->mission->UpdateFromMappedVesselIDs(distanceFromVesselID, vesselName2, vesselSituation2)) {  // sic: distanceFromVesselID
                    flag2 = true;
                }
                text5 = (vesselSituation2 != nullptr || flag2) ? vesselName2 : Tag("#autoLOC_8100159");
                std::string text6 = text4 + Fmt("#autoLOC_8004190", {Tag("#autoLOC_8000001"), text5});
                if (flag2 && vesselSituation2 != nullptr) text6 = text6 + Fmt("#autoLOC_6002491", {vesselSituation2->vesselName}) + "\n";
                return text6;
            }
        }
    }
    if (field.name == "distance") return Fmt("#autoLOC_8004226", {(comparisonOperator == TestComparisonLessGreaterOnly::LessThan) ? "<" : ">", dotnet::FormatDoubleG(distance)});
    if (field.name == "calculationType") return TestModule::GetNodeBodyParameterString(field);
    return "";
}

// TestDistance.cs:791
std::string TestDistance::GetInfo() { return Tag("#autoLOC_8004141"); }

// TestDistance.cs:796
void TestDistance::Save(ConfigNode& cfg) const {
    TestModule::Save(cfg);
    cfg.AddValue("distanceFromTarget", dotnet::EnumToString(distanceFromTarget));
    cfg.AddValue("distanceFromVesselID", distanceFromVesselID);
    ConfigNode* configNode = cfg.AddNode("DISTANCEFROMKERBAL");
    distanceFromKerbal->Save(*configNode);
    cfg.AddValue("distanceToTarget", dotnet::EnumToString(distanceToTarget));
    cfg.AddValue("distanceToVesselID", distanceToVesselID);
    distanceToKerbal->Save(cfg);
    ConfigNode* configNode2 = cfg.AddNode("DISTANCETOKERBAL");
    distanceToKerbal->Save(*configNode2);
    ConfigNode* configNode3 = cfg.AddNode("DISTANCETOLOCATION");
    distanceToLocation->Save(*configNode3);
    distanceToCelestialBody->Save(cfg);
    cfg.AddValue("distanceToAsteroidID", distanceToAsteroidID);
    cfg.AddValue("distanceToCometID", distanceToCometID);
    cfg.AddValue("distanceToFlagID", distanceToFlagID);
    cfg.AddValue("distanceToLaunchSiteName", distanceToLaunchSiteName);
    cfg.AddValue("distanceToNode", distanceToNode.ToString());
    cfg.AddValue("calculationType", dotnet::EnumToString(calculationType));
    cfg.AddValue("comparisonOperator", dotnet::EnumToString(comparisonOperator));
    cfg.AddValue("distance", distance);
}

// TestDistance.cs:824
void TestDistance::Load(const ConfigNode& cfg) {
    TestModule::Load(cfg);
    save_detail::TryGetEnum(cfg, "distanceFromTarget", distanceFromTarget, DistanceFromChoices::Vessel);
    cfg.TryGetValue("distanceFromVesselID", distanceFromVesselID);
    ConfigNode* configNode = nullptr;
    cfg.TryGetNode("DISTANCEFROMKERBAL", configNode);
    if (configNode != nullptr) distanceFromKerbal->Load(*configNode);
    save_detail::TryGetEnum(cfg, "distanceToTarget", distanceToTarget, DistanceToChoices::Vessel);
    cfg.TryGetValue("distanceToVesselID", distanceToVesselID);
    cfg.TryGetValue("distanceToLaunchSiteName", distanceToLaunchSiteName);
    ConfigNode* configNode2 = nullptr;
    cfg.TryGetNode("DISTANCETOKERBAL", configNode2);
    if (configNode2 != nullptr) distanceToKerbal->Load(*configNode2);
    ConfigNode* configNode3 = nullptr;
    cfg.TryGetNode("DISTANCETOLOCATION", configNode3);
    if (configNode2 != nullptr) {  // sic: tests configNode2, not configNode3 (NullReferenceException when only DISTANCETOKERBAL is present)
        if (configNode3 == nullptr) throw NullReferenceException();
        distanceToLocation->Load(*configNode3);
    }
    distanceToCelestialBody->Load(cfg);
    cfg.TryGetValue("distanceToAsteroidID", distanceToAsteroidID);
    cfg.TryGetValue("distanceToCometID", distanceToCometID);
    cfg.TryGetValue("distanceToFlagID", distanceToFlagID);
    cfg.TryGetValue("distanceToNode", distanceToNode);
    save_detail::TryGetEnum(cfg, "calculationType", calculationType, DistanceCalculationType::StraightLine);
    save_detail::TryGetEnum(cfg, "comparisonOperator", comparisonOperator, TestComparisonLessGreaterOnly::GreaterThan);
    cfg.TryGetValue("distance", distance);
}

// TestDistance.cs:866
ScoreModifier TestDistance::GetScoreModifier(const std::string& scoreModule) {
    ScoreModifier r;
    if (scoreModule == "ScoreModule_Resource") r.vessel = AwardEnvironment::ActiveVessel();
    return r;
}

}  // namespace ksp::Expansions::Missions::Tests
