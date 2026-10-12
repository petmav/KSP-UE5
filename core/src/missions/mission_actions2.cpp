// WP-54 slice 2: Assembly-CSharp/Expansions/Missions/Actions/ActionCreate{Vessel,Kerbal,Asteroid,Comet,Flag,LaunchSite}.cs (the create actions WP-55
// deferred). See mission_actions2.hpp. Cite placeholders `File.cs:@Name[#k]` are resolved by tools/wp55/check_cites.py --resolve.
#include "ksp/missions/mission_actions2.hpp"

#include <algorithm>

#include "../save/save_util.hpp"
#include "ksp/career/format.hpp"
#include "ksp/career/game_variables.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/sentinel.hpp"
#include "ksp/inventory/inventory.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_awards.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/save/game.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/discovery_info.hpp"

namespace ksp::Expansions::Missions {

void ActionCreateEnvironment::Reset() {
    createVesselNode = nullptr;
    createPartNode = nullptr;
    createDiscoveryNode = nullptr;
    addVessel = nullptr;
    addedVesselNodes.clear();
    hasPQS = nullptr;
    surfaceHeight = nullptr;
    uniqueFlightID = nullptr;
    createRandomOrbitAround = nullptr;
    fromToRotation = nullptr;
    angleAxis = nullptr;
    lookRotation = nullptr;
    cometVesselNode = nullptr;
    launchSiteWorldPosition = nullptr;
    spawnCraftVessel = nullptr;
    createPlaceholderCraft = nullptr;
    craftIsIncompatible = nullptr;
    loadedSceneIsMissionBuilder = nullptr;
    validateCraft = nullptr;
    updateLauncherButtonPlayAnim = nullptr;
    evaDefaultHelmetOn = nullptr;
    evaDefaultNeckRingOn = nullptr;
}

namespace Actions {
namespace {

using Env = ActionCreateEnvironment;
using KSP::Localization::Localizer;
using PCM = std::shared_ptr<ProtoCrewMember>;

std::string Tag(const char* t) { return Localizer::Format(t); }
std::string D2h(double v) { return career::FormatNumber(v, std::string_view("0.##")); }
std::string N0(double v) { return career::FormatNumber(v, std::string_view("N0")); }
std::string N0f(float v) { return career::FormatNumber(v, std::string_view("N0")); }
std::string RemoveGender(std::string input) {
    const size_t num = input.rfind('^');
    if (num != std::string::npos) input = input.substr(0, num);
    return input;
}
CelestialBody* Home() { return NodeLibraryEnvironment::HomeBody(); }
KerbalRoster& Roster() { return *HighLogic::CurrentGame()->CrewRoster; }
double UT() { return HighLogic::CurrentGame() != nullptr ? HighLogic::CurrentGame()->UniversalTime() : 0.0; }

Vector3f ToV3(const Vector3d& v) { return v.ToVector3f(); }

ConfigNode::NodePtr CreateVesselNode(const std::string& name, VesselType type, const Orbit& orbit, int rootPartIndex, const std::vector<ConfigNode::NodePtr>& partNodes,
                                     const std::vector<ConfigNode::NodePtr>& additionalNodes = {}) {
    if (Env::createVesselNode) return Env::createVesselNode(name, type, orbit, rootPartIndex, partNodes, additionalNodes);
    std::vector<ConfigNode::NodePtr> all = partNodes;
    all.insert(all.end(), additionalNodes.begin(), additionalNodes.end());
    return ProtoVesselNodes::CreateVesselNode(name, type, orbit, rootPartIndex, all, NodeLibraryEnvironment::Bodies());
}
ConfigNode::NodePtr CreatePartNode(const std::string& partName, uint32_t id, const std::vector<PCM>& crew) {
    if (Env::createPartNode) return Env::createPartNode(partName, id, crew);
    throw std::runtime_error("[VesselSpawner Error]: No part in loader found with name: " + partName + ".");
}
ConfigNode::NodePtr CreateDiscoveryNode(DiscoveryLevels level, UntrackedObjectClass size, double life, double maxLife) {
    if (Env::createDiscoveryNode) return Env::createDiscoveryNode(level, size, life, maxLife);
    auto n = std::make_shared<ConfigNode>("DISCOVERY");
    n->AddValue("level", static_cast<int>(level));
    n->AddValue("size", static_cast<int>(size));
    n->AddValue("lifetime", life);
    n->AddValue("maxlifetime", maxLife);
    return n;
}
Vessel* AddVessel(const ConfigNode& node) {
    if (Env::addVessel) return Env::addVessel(node);
    Env::addedVesselNodes.push_back(node.CreateCopy());
    return nullptr;
}
bool HasPQS(CelestialBody* b) { return Env::hasPQS ? Env::hasPQS(b) : b->pqsController != nullptr; }
double SurfaceHeight(CelestialBody* b, const Vector3d& n) {
    if (Env::surfaceHeight) return Env::surfaceHeight(b, n);
    if (b->pqsController == nullptr) throw NullReferenceException();
    return b->pqsController->GetSurfaceHeight(n);
}
uint32_t UniqueFlightID() {
    if (Env::uniqueFlightID) return Env::uniqueFlightID();
    return ShipConstruction::GetUniqueFlightID(HighLogic::CurrentGame()->flightState.get());
}
Orbit RandomOrbitAround(CelestialBody* b) { return Env::createRandomOrbitAround ? Env::createRandomOrbitAround(b) : Orbit::CreateRandomOrbitAround(b); }
Quaternionf FromTo(const Vector3f& a, const Vector3f& b) { return Env::fromToRotation ? Env::fromToRotation(a, b) : UnityNative::FromToRotation(a, b); }
Quaternionf AngleAxisQ(float angle, const Vector3f& axis) { return Env::angleAxis ? Env::angleAxis(angle, axis) : UnityNative::AngleAxis(angle, axis); }
Quaternionf LookRot(const Vector3f& f, const Vector3f& u) { return Env::lookRotation ? Env::lookRotation(f, u) : UnityNative::LookRotation(f, u); }
constexpr Vector3f kUp{0.0f, 1.0f, 0.0f};

std::string SituationName(Vessel::Situations s) { return dotnet::EnumToString(s); }

// the Save/Load of INodeOrbit.GetNodeOrbit: `orbit = location.orbit.Orbit; case landed: orbit.referenceBody = location.landed.targetBody`
Orbit* NodeOrbit(ParamChoices_VesselSimpleLocation& location) {
    Orbit* orbit = location.orbit->orbit();
    if (location.locationChoice == ParamChoices_VesselSimpleLocation::Choices::landed) orbit->referenceBody = location.landed->targetBody;
    return orbit;
}
Vector3f LocationActionLocation(ParamChoices_VesselSimpleLocation& location) {
    switch (location.locationChoice) {
        default: return Vector3f();
        case ParamChoices_VesselSimpleLocation::Choices::orbit: return ToV3(location.orbit->orbit()->getPositionAtUT(UT()));
        case ParamChoices_VesselSimpleLocation::Choices::landed: return location.landed->GetWorldPosition();
    }
}
std::unique_ptr<ParamChoices_VesselSimpleLocation> NewLocation(VesselGroundLocation::GizmoIcon icon, MENode* node, ParamChoices_VesselSimpleLocation::Choices choice) {
    auto location = std::make_unique<ParamChoices_VesselSimpleLocation>();
    location->orbit = std::make_unique<MissionOrbit>(Home());
    location->landed = std::make_unique<VesselGroundLocation>(node, icon);
    location->locationChoice = choice;
    return location;
}
}  // namespace

// ---- ActionCreateVessel ----------------------------------------------------------------------------------------------------------

// ActionCreateVessel.cs:23
ActionCreateVessel::ActionCreateVessel() : ActionModule(kTypeName) { title = Tag("#autoLOC_8000028"); }
// ActionCreateVessel.cs:29
void ActionCreateVessel::Initialize(MENode* n) {
    ActionModule::Initialize(n);
    vesselSituation = std::make_unique<VesselSituation>(n->mission, n);
}
// ActionCreateVessel.cs:35
Enumerator ActionCreateVessel::Fire() {
    if (vesselSituation->launched) co_return;
    if (vesselSituation->playerCreated) {
        node->mission->situation->AddNodeWithVesselToBuild(node);
        CreateVesselScreenMessage();
    } else if (!vesselSituation->craftFile.empty()) {
        for (size_t i = 0; i < node->fromNodes.size(); ++i) {
            if (node->fromNodes[i]->isStartNode) {
                co_yield YieldInstruction::WaitForSeconds(1.0f);
                break;
            }
        }
        // the PRELAUNCH recovery of vessels at the site, MissionSystem.ConstructShip, the camera / active-vessel switch and the revert flag
        // are the flight scene's (ActionCreateEnvironment::spawnCraftVessel)
        node->mission->situation->AddNodeWithVesselToBuild(node);
        if (Env::spawnCraftVessel) Env::spawnCraftVessel(*this);
        node->mission->situation->RemoveLaunchedVessels();
    } else {
        log::Warning("[Missions]: Unable to spawn Vessel :" + vesselSituation->vesselName + " Craftfile is blank");
    }
    co_return;
}
// ActionCreateVessel.cs:133
void ActionCreateVessel::CreateVesselScreenMessage() {
    if (!screenMessageActive) {
        const std::string message = Localizer::Format("#autoLOC_8000302", {vesselSituation->vesselName});
        NodeLibraryEnvironment::ScreenMessage m;
        m.text = message;
        m.duration = std::numeric_limits<float>::max();
        m.style = ScreenMessageStyle::UPPER_CENTER;
        m.persist = true;
        if (NodeLibraryEnvironment::postScreenMessage) {
            NodeLibraryEnvironment::postScreenMessage(m);
        } else {
            NodeLibraryEnvironment::screenMessages.push_back(m);
        }
        screenMessageActive = true;
    }
    if (Env::updateLauncherButtonPlayAnim) Env::updateLauncherButtonPlayAnim();
}
// ActionCreateVessel.cs:146
Vector3f ActionCreateVessel::ActionLocation() {
    switch (vesselSituation->location->situation) {
        case VesselLocation::VesselStartSituations::LANDED: return vesselSituation->location->vesselGroundLocation->GetWorldPosition();
        case VesselLocation::VesselStartSituations::ORBITING: return ToV3(vesselSituation->location->orbitSnapShot->orbit()->getPositionAtUT(UT()));
        case VesselLocation::VesselStartSituations::PRELAUNCH:
            if (Env::launchSiteWorldPosition) {
                if (std::optional<Vector3f> p = Env::launchSiteWorldPosition(vesselSituation->location->launchSite); p.has_value()) return *p;
            }
            break;
    }
    return Vector3f();
}
// ActionCreateVessel.cs:176
void ActionCreateVessel::OnCloned(ActionModule*& actionModuleBase) {
    ActionModule::OnCloned(actionModuleBase);
    vesselSituation->persistentId = MissionDataEnvironment::UniquePersistentId();
}
// ActionCreateVessel.cs:187
Orbit* ActionCreateVessel::GetNodeOrbit() {
    Orbit* orbit = vesselSituation->location->orbitSnapShot->orbit();
    switch (vesselSituation->location->situation) {
        case VesselLocation::VesselStartSituations::LANDED:
        case VesselLocation::VesselStartSituations::PRELAUNCH: orbit->referenceBody = vesselSituation->location->vesselGroundLocation->targetBody; break;
        default: break;
    }
    return orbit;
}
// ActionCreateVessel.cs:201
void ActionCreateVessel::CreatePlaceholderCraft() {
    if (Env::createPlaceholderCraft) Env::createPlaceholderCraft(*this);
}
// ActionCreateVessel.cs:243
std::string ActionCreateVessel::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "vesselSituation") {
        std::string text;
        if (vesselSituation == nullptr) {
            text = Tag("#autoLOC_8000068");
        } else {
            std::string text2 = vesselSituation->vesselName;
            if (Env::loadedSceneIsMissionBuilder && Env::loadedSceneIsMissionBuilder()) {
                if (Env::craftIsIncompatible && Env::craftIsIncompatible(vesselSituation.get())) text2 = Localizer::Format("#autoLOC_8004245", {text2});
            }
            text = Localizer::Format("#autoLOC_8000069", {text2});
        }
        if (!vesselSituation->vesselCrew.empty()) {
            std::vector<std::string> list;
            for (size_t i = 0; i < vesselSituation->vesselCrew.size(); ++i)
                for (size_t j = 0; j < vesselSituation->vesselCrew[i].crewNames.size(); ++j)
                    list.push_back(vesselSituation->vesselCrew[i].LocalizeCrewMember(static_cast<int>(j)));
            if (!list.empty()) {
                text += Tag("#autoLOC_8000059");
                text += Contracts::Lite::StringUtilities::ThisThisAndThat(list);
                text += "\n";
            }
        }
        VesselLocation& loc = *vesselSituation->location;
        switch (loc.situation) {
            case VesselLocation::VesselStartSituations::ORBITING:
                if (loc.orbitSnapShot->Body() != nullptr) {
                    MissionOrbit& o = *loc.orbitSnapShot;
                    text = text + Localizer::Format("#autoLOC_8100316", {field.guiName, RemoveGender(o.Body()->displayName())}) + "\n";
                    text = text + Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100058"), N0(o.SemiMajorAxis())}) + "\n";
                    text = text + Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100059"), N0(o.Apoapsis())}) + "\n";
                    text = text + Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100060"), N0(o.Periapsis())}) + "\n";
                    if (o.eccentricity > 0.0001) text = text + Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100061"), D2h(o.Eccentricity())}) + "\n";
                    if (o.inclination != 0.0)
                        text = text + Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100062"), D2h(o.Inclination()) + "\xC2\xB0"}) + "\n";
                    if (o.Lan() != 0.0) text = text + Localizer::Format("#autoLOC_8004190", {Tag("#autoLOC_8100063"), D2h(o.LAN)}) + "\n";
                }
                break;
            case VesselLocation::VesselStartSituations::PRELAUNCH:
                if (!loc.launchSite.empty()) {
                    text += Localizer::Format("#autoLOC_8000283", {MissionDataEnvironment::launchSiteDisplayName ? MissionDataEnvironment::launchSiteDisplayName(loc.launchSite) : loc.launchSite});
                }
                break;
            case VesselLocation::VesselStartSituations::LANDED:
                if (loc.vesselGroundLocation->targetBody != nullptr) {
                    text += Localizer::Format("#autoLOC_8000284", {RemoveGender(loc.vesselGroundLocation->targetBody->displayName())});
                    text += Localizer::Format("#autoLOC_8000285", {D2h(loc.vesselGroundLocation->longitude), D2h(loc.vesselGroundLocation->latitude)});
                }
                break;
        }
        return text;
    }
    return ActionModule::GetNodeBodyParameterString(field);
}
// ActionCreateVessel.cs:336
std::string ActionCreateVessel::GetInfo() { return Tag("#autoLOC_8004010"); }
// ActionCreateVessel.cs:341
void ActionCreateVessel::RunValidation(Editor::MissionEditorValidator* validator) {
    VesselLocation& loc = *vesselSituation->location;
    auto displayName = [&](const std::string& site) { return MissionDataEnvironment::launchSiteDisplayName ? MissionDataEnvironment::launchSiteDisplayName(site) : site; };
    if (!vesselSituation->playerCreated && vesselSituation->craftFile.empty()) validator->AddNodeFail(node, Tag("#autoLOC_8000293"));
    if (loc.situation == VesselLocation::VesselStartSituations::PRELAUNCH && displayName(loc.launchSite).empty() && !node->mission->MissionHasLaunchSite(loc.launchSite))
        validator->AddNodeFail(node, Tag("#autoLOC_8000294"));
    if (!vesselSituation->playerCreated && !vesselSituation->craftFile.empty()) {
        if (Env::validateCraft) Env::validateCraft(*this, validator);  // GetCraftBySituationsVesselID / LoadVessel / NoControlSources / MissionCheckLaunchClamps
    }
    if (vesselSituation->playerCreated) {
        std::vector<VesselRestriction*> activeRestrictions = vesselSituation->vesselRestrictionList->ActiveRestrictions();
        if (!activeRestrictions.empty()) {
            auto& params = node->mission->situation->gameParameters;
            for (size_t i = 0; i < activeRestrictions.size(); ++i) {
                auto* mass = dynamic_cast<VesselRestriction_Mass*>(activeRestrictions[i]);
                auto* size = dynamic_cast<VesselRestriction_Size*>(activeRestrictions[i]);
                auto* partCount = dynamic_cast<VesselRestriction_PartCount*>(activeRestrictions[i]);
                const bool padSite = loc.launchSite == "Runway" || loc.launchSite == "LaunchPad";
                if (mass != nullptr && mass->targetMass > 0.0f && padSite) {
                    float num = 0.0f;
                    if (loc.launchSite == "Runway") {
                        const float editorNormLevel = f32store((fx(static_cast<float>(params->CustomParams<MissionParamsFacilities>().facilityLevelRunway)) - 1.0) / 2.0);
                        num = GameVariables::Instance->GetCraftMassLimit(editorNormLevel, false);
                    } else {
                        const float editorNormLevel2 = f32store((fx(static_cast<float>(params->CustomParams<MissionParamsFacilities>().facilityLevelLaunchpad)) - 1.0) / 2.0);
                        num = GameVariables::Instance->GetCraftMassLimit(editorNormLevel2, true);
                    }
                    if (num > 0.0f) {
                        if (mass->comparisonOperator == TestComparisonLessGreaterEqual::GreaterOrEqual) {
                            if (mass->targetMass >= num) validator->AddNodeWarn(node, Localizer::Format("#autoLOC_8000296", {loc.launchSite, N0f(num)}));
                        } else if (mass->targetMass > num) {
                            validator->AddNodeWarn(node, Localizer::Format("#autoLOC_8000296", {loc.launchSite, N0f(num)}));
                        }
                    }
                }
                if (size != nullptr && (size->tempInputX > 0.0f || size->tempInputY > 0.0f || (size->tempInputZ > 0.0f && padSite))) {
                    Vector3f zero;
                    if (loc.launchSite == "Runway") {
                        const float editorNormLevel3 = f32store((fx(static_cast<float>(params->CustomParams<MissionParamsFacilities>().facilityLevelRunway)) - 1.0) / 2.0);
                        zero = GameVariables::Instance->GetCraftSizeLimit(editorNormLevel3, false);
                    } else {
                        const float editorNormLevel4 = f32store((fx(static_cast<float>(params->CustomParams<MissionParamsFacilities>().facilityLevelLaunchpad)) - 1.0) / 2.0);
                        zero = GameVariables::Instance->GetCraftSizeLimit(editorNormLevel4, true);
                    }
                    const bool ge = size->comparisonOperator == TestComparisonLessGreaterEqual::GreaterOrEqual;
                    if (zero.x > 0.0f) {
                        if (ge ? size->tempInputX >= zero.x : size->tempInputX > zero.x)
                            validator->AddNodeWarn(node, Localizer::Format("#autoLOC_8000298", {loc.launchSite, N0f(zero.x)}));
                    }
                    if (zero.y > 0.0f) {
                        if (ge ? size->tempInputY >= zero.y : size->tempInputY > zero.y)
                            validator->AddNodeWarn(node, Localizer::Format("#autoLOC_8000299", {loc.launchSite, N0f(zero.y)}));
                    }
                    if (zero.z > 0.0f) {
                        if (ge ? size->tempInputZ >= zero.z : size->tempInputZ > zero.z)
                            validator->AddNodeWarn(node, Localizer::Format("#autoLOC_8000300", {loc.launchSite, N0f(zero.z)}));
                    }
                }
                if (partCount != nullptr && partCount->targetParts > 0) {
                    int num2 = 0;
                    if (loc.facility == EditorFacility::SPH) {
                        const float editorNormLevel5 = f32store((fx(static_cast<float>(params->CustomParams<MissionParamsFacilities>().facilityLevelSPH)) - 1.0) / 2.0);
                        num2 = GameVariables::Instance->GetPartCountLimit(editorNormLevel5, false);
                    } else {
                        const float editorNormLevel6 = f32store((fx(static_cast<float>(params->CustomParams<MissionParamsFacilities>().facilityLevelVAB)) - 1.0) / 2.0);
                        num2 = GameVariables::Instance->GetPartCountLimit(editorNormLevel6, true);
                    }
                    if (num2 > 0) {
                        const bool ge = partCount->comparisonOperator == TestComparisonLessGreaterEqual::GreaterOrEqual;
                        if (ge ? partCount->targetParts >= num2 : partCount->targetParts > num2) {
                            // num2.ToString(Localizer.Format("#autoLOC_8000301")): the localized custom number format
                            const std::string fmt301 = Tag("#autoLOC_8000301");
                            validator->AddNodeWarn(node, Localizer::Format("#autoLOC_8000275", {Localizer::Format(EnumDescription(loc.facility == EditorFacility::SPH ? EditorFacility::SPH : EditorFacility::VAB)),
                                                                                                career::FormatNumber(static_cast<double>(num2), std::string_view(fmt301))}));
                        }
                    }
                }
                for (size_t j = 0; j < activeRestrictions.size(); ++j) {
                    if (i != j && activeRestrictions[i]->SameComparatorWrapper(*activeRestrictions[j]))
                        validator->AddNodeFail(node, Localizer::Format("#autoLOC_8003110", {activeRestrictions[i]->GetDisplayName()}));
                }
            }
        }
    }
    if (loc.situation != VesselLocation::VesselStartSituations::PRELAUNCH || loc.launchSite.empty()) return;
    std::vector<VesselSituation*> allVesselSituations = vesselSituation->mission->GetAllVesselSituations();
    for (size_t k = 0; k < allVesselSituations.size(); ++k) {
        if (allVesselSituations[k]->location->situation == VesselLocation::VesselStartSituations::PRELAUNCH && allVesselSituations[k]->location->launchSite == loc.launchSite &&
            allVesselSituations[k]->persistentId != vesselSituation->persistentId) {
            validator->AddNodeWarn(node, Localizer::Format("#autoLOC_8002019", {allVesselSituations[k]->vesselName, loc.launchSite}));
        }
    }
}
// ActionCreateVessel.cs:523
void ActionCreateVessel::Save(ConfigNode& cfg) const {
    ActionModule::Save(cfg);
    vesselSituation->Save(cfg);
}
// ActionCreateVessel.cs:529
void ActionCreateVessel::Load(const ConfigNode& cfg) {
    ActionModule::Load(cfg);
    ConfigNode* configNode = cfg.GetNode("VESSELSITUATION");
    if (configNode != nullptr) vesselSituation->Load(*configNode);
}

// ---- ActionCreateKerbal ----------------------------------------------------------------------------------------------------------

// ActionCreateKerbal.cs:38
ActionCreateKerbal::ActionCreateKerbal() : ActionModule(kTypeName) {
    isHelmetEnabled = Env::evaDefaultHelmetOn ? Env::evaDefaultHelmetOn() : false;
    isNeckRingEnabled = Env::evaDefaultNeckRingOn ? Env::evaDefaultNeckRingOn() : false;
    title = Tag("#autoLOC_8000240");
    kerbalName = "";
    missionKerbal = std::make_unique<MissionKerbal>(nullptr, node, [this] { UpdateNodeBodyUI(); });
    location = std::make_unique<ParamChoices_VesselSimpleLocation>();
    location->orbit = std::make_unique<MissionOrbit>(Home());
    location->landed = std::make_unique<VesselGroundLocation>(node, VesselGroundLocation::GizmoIcon::Kerbal);
    location->landed->splashed = true;
    persistentId = MissionDataEnvironment::UniquePersistentId();
    location->locationChoice = ParamChoices_VesselSimpleLocation::Choices::landed;
}
// ActionCreateKerbal.cs:52
void ActionCreateKerbal::Initialize(MENode* n) {
    ActionModule::Initialize(n);
    missionKerbal->Initialize(nullptr, n);
}
// ActionCreateKerbal.cs:58
Enumerator ActionCreateKerbal::Fire() {
    if (!kerbalName.empty()) {
        missionKerbal->set_Kerbal(nullptr);
        const std::vector<PCM> crew = node->mission->situation->crewRoster->Crew();
        for (size_t i = 0; i < crew.size(); ++i) {
            if (kerbalName == crew[i]->name().value_or("")) missionKerbal->set_Kerbal(crew[i]);
        }
    }
    SpawnKerbal();
    co_return;
}
// ActionCreateKerbal.cs:83
ConfigNode::NodePtr ActionCreateKerbal::SpawnKerbal(bool addtoCurrentGame) {
    if (missionKerbal->Kerbal() == nullptr || missionKerbal->Kerbal()->name().value_or("").empty()) {
        log::Info("Unable to find kerbal " + kerbalName + " in mission roster, generating a random Kerbal of type " + dotnet::EnumToString(missionKerbal->TypeToShow()));
        missionKerbal->set_Kerbal(Roster().GetNewKerbal(missionKerbal->TypeToShow()));
        missionKerbal->set_Kerbal(missionKerbal->Kerbal());
        kerbalName = missionKerbal->Kerbal()->name().value_or("");
    }
    PCM k = missionKerbal->Kerbal();
    k->set_rosterStatus(ProtoCrewMember::RosterStatus::Assigned);
    k->seatIdx = -1;
    k->hasHelmetOn = isHelmetEnabled;
    k->hasNeckRingOn = isNeckRingEnabled;
    k->completedFirstEVA = true;
    ConfigNode::NodePtr protoVesselNode = GetProtoVesselNode();
    if (addtoCurrentGame) AddVessel(*protoVesselNode);
    return protoVesselNode;
}
// ActionCreateKerbal.cs:115
ConfigNode::NodePtr ActionCreateKerbal::GetProtoVesselNode() { return GetProtoVesselNode(defaultKerbalFeetToPivotDist); }
// ActionCreateKerbal.cs:191
ConfigNode::NodePtr ActionCreateKerbal::CreateKerbalPartNode(uint32_t id, const PCM& crew) {
    ConfigNode::NodePtr configNode = CreatePartNode(crew->GetKerbalEVAPartName(), id, {crew});
    configNode->SetValue("flag", node->mission->flagURL, true);
    return configNode;
}
// ActionCreateKerbal.cs:120
ConfigNode::NodePtr ActionCreateKerbal::GetProtoVesselNode(float feetToPivotDistance) {
    const uint32_t uniqueFlightID = UniqueFlightID();
    bool flag = location->locationChoice == ParamChoices_VesselSimpleLocation::Choices::landed;
    bool flag2 = flag && location->landed->splashed;
    const Orbit orbit = flag ? RandomOrbitAround(location->landed->targetBody) : location->orbit->RelativeOrbit(node->mission);
    ConfigNode::NodePtr configNode;
    if (!isStranded) {
        configNode = CreateVesselNode(missionKerbal->Kerbal()->name().value_or(""), VesselType::EVA, orbit, 0, {CreateKerbalPartNode(uniqueFlightID, missionKerbal->Kerbal())});
    } else {
        configNode = CreateVesselNode(missionKerbal->Kerbal()->name().value_or(""), VesselType::EVA, orbit, 0, {CreateKerbalPartNode(uniqueFlightID, missionKerbal->Kerbal())},
                                      {CreateDiscoveryNode(DiscoveryLevels::Unowned, UntrackedObjectClass::A, TimeDeadline * 2.0, TimeDeadline * 2.0)});
    }
    Vessel::Situations situations = flag ? Vessel::Situations::LANDED : Vessel::Situations::ORBITING;
    VesselGroundLocation& landed = *location->landed;
    configNode->SetValue("landedAt", landed.targetBody->name());
    configNode->SetValue("lat", landed.latitude);
    configNode->SetValue("lon", landed.longitude);
    double num = landed.altitude;
    const Vector3d relSurfaceNVector = landed.targetBody->GetRelSurfaceNVector(landed.latitude, landed.longitude);
    if (flag2) {
        num = SurfaceHeight(landed.targetBody, relSurfaceNVector) - landed.targetBody->Radius;
        if (num < 0.0) {
            num = 0.0;
            flag = false;
            situations = Vessel::Situations::SPLASHED;
        } else {
            flag2 = false;
        }
    }
    if (flag && !flag2) num += static_cast<double>(feetToPivotDistance);
    configNode->SetValue("sit", SituationName(situations));
    configNode->SetValue("alt", num);
    configNode->SetValue("landed", flag);
    configNode->SetValue("splashed", flag2);
    configNode->SetValue("skipGroundPositioning", false);
    configNode->SetValue("vesselSpawning", true);
    configNode->AddValue("prst", true);
    const Vector3f vector = landed.targetBody->GetRelSurfacePosition(landed.latitude, landed.longitude, num).ToVector3f();
    Quaternionf identity = Quaternionf::identity();
    const Vector3f normalized = vector.normalized();
    Vector3f normalized2;
    if (landed.longitude > 89.9000015258789) {
        const Vector3f other = landed.targetBody->GetRelSurfacePosition(landed.latitude - 1E-05, landed.longitude, num).ToVector3f();
        normalized2 = Vector3f::Cross((other - vector).normalized(), normalized).normalized();
    } else {
        const Vector3f vector2 = landed.targetBody->GetRelSurfacePosition(landed.latitude + 1E-05, landed.longitude, num).ToVector3f();
        normalized2 = Vector3f::Cross(normalized, (vector2 - vector).normalized()).normalized();
    }
    identity = LookRot(normalized2, normalized);
    identity = AngleAxisQ(f32store(fx(kerbalRotationOffset) + fx(landed.rotation->eulerZ())), normalized) * identity;
    configNode->AddValue("rot", identity);
    if (flag) {
        configNode->SetValue("skipGroundPositioning", false);
    } else {
        configNode->SetValue("skipGroundPositioning", true);
    }
    if (HasPQS(landed.targetBody)) {
        configNode->SetValue("PQSMin", 0, true);
        configNode->SetValue("PQSMax", 0, true);
    }
    return configNode;
}
// ActionCreateKerbal.cs:198
Vector3f ActionCreateKerbal::ActionLocation() { return LocationActionLocation(*location); }
// ActionCreateKerbal.cs:212
void ActionCreateKerbal::OnCloned(ActionModule*& actionModuleBase) {
    ActionModule::OnCloned(actionModuleBase);
    persistentId = MissionDataEnvironment::UniquePersistentId();
    missionKerbal->set_Kerbal(nullptr);
}
// ActionCreateKerbal.cs:224
Orbit* ActionCreateKerbal::GetNodeOrbit() { return NodeOrbit(*location); }
// ActionCreateKerbal.cs:262
std::string ActionCreateKerbal::GetInfo() { return Tag("#autoLOC_8004008"); }
// ActionCreateKerbal.cs:267
void ActionCreateKerbal::NodeDeleted() {
    if (missionKerbal != nullptr) missionKerbal->setcurrentKerbalAvailable();
}
// ActionCreateKerbal.cs:275
void ActionCreateKerbal::KerbalRosterStatusChange(const PCM& kerbal, ProtoCrewMember::RosterStatus oldStatus, ProtoCrewMember::RosterStatus newStatus) {
    if (missionKerbal != nullptr) missionKerbal->onKerbalStatusChange(kerbal, oldStatus, newStatus);
}
// ActionCreateKerbal.cs:283
void ActionCreateKerbal::KerbalAdded(const PCM& kerbal) {
    if (missionKerbal != nullptr) missionKerbal->onKerbalAdded(kerbal);
}
// ActionCreateKerbal.cs:291
void ActionCreateKerbal::KerbalTypeChange(const PCM& kerbal, ProtoCrewMember::KerbalType oldType, ProtoCrewMember::KerbalType newType) {
    if (missionKerbal != nullptr) missionKerbal->onKerbalTypeChange(kerbal, oldType, newType);
}
// ActionCreateKerbal.cs:299
void ActionCreateKerbal::KerbalNameChange(const PCM& kerbal, const std::string& oldName, const std::string& newName) {
    if (missionKerbal != nullptr) missionKerbal->onKerbalNameChange(kerbal, oldName, newName);
}
// ActionCreateKerbal.cs:307
void ActionCreateKerbal::KerbalRemoved(const PCM& kerbal) {
    if (missionKerbal != nullptr) missionKerbal->onKerbalRemoved(kerbal);
}
// ActionCreateKerbal.cs:315
std::string ActionCreateKerbal::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "location") return location->GetNodeBodyParameterString();
    if (field.name == "missionKerbal") return missionKerbal->GetNodeBodyParameterString();
    return ActionModule::GetNodeBodyParameterString(field);
}
// ActionCreateKerbal.cs:328
void ActionCreateKerbal::Save(ConfigNode& cfg) const {
    ActionModule::Save(cfg);
    missionKerbal->Save(cfg);
    cfg.AddValue("isStranded", isStranded);
    cfg.AddValue("isHelmetEnabled", isHelmetEnabled);
    cfg.AddValue("isNeckRingEnabled", isNeckRingEnabled);
    cfg.AddValue("persistentId", persistentId);
    location->Save(cfg);
}
// ActionCreateKerbal.cs:339
void ActionCreateKerbal::Load(const ConfigNode& cfg) {
    ActionModule::Load(cfg);
    location->Load(cfg);
    if (missionKerbal == nullptr) missionKerbal = std::make_unique<MissionKerbal>(nullptr, node, [this] { UpdateNodeBodyUI(); });
    missionKerbal->Load(cfg);
    cfg.TryGetValue("isStranded", isStranded);
    cfg.TryGetValue("isHelmetEnabled", isHelmetEnabled);
    cfg.TryGetValue("isNeckRingEnabled", isNeckRingEnabled);
    cfg.TryGetValue("persistentId", persistentId);
}

// ---- ActionCreateAsteroid / ActionCreateComet ------------------------------------------------------------------------------------

// ActionCreateAsteroid.cs:18
ActionCreateAsteroid::ActionCreateAsteroid() : ActionModule(kTypeName) {
    title = Tag("#autoLOC_8000045");
    asteroid = std::make_unique<Asteroid>();
    location = NewLocation(VesselGroundLocation::GizmoIcon::Asteroid, node, ParamChoices_VesselSimpleLocation::Choices::orbit);
    location->landed->splashed = true;
}
// ActionCreateAsteroid.cs:30
Enumerator ActionCreateAsteroid::Fire() {
    SpawnAsteroid();
    co_return;
}
// ActionCreateAsteroid.cs:36
Vector3f ActionCreateAsteroid::ActionLocation() { return LocationActionLocation(*location); }
// ActionCreateAsteroid.cs:50
void ActionCreateAsteroid::OnCloned(ActionModule*& actionModuleBase) {
    ActionModule::OnCloned(actionModuleBase);
    asteroid->seed = Asteroid::GetAsteroidSeed();
    asteroid->persistentId = MissionDataEnvironment::UniquePersistentId();
}
// ActionCreateAsteroid.cs:62
Orbit* ActionCreateAsteroid::GetNodeOrbit() { return NodeOrbit(*location); }
// ActionCreateAsteroid.cs:75
std::string ActionCreateAsteroid::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.fieldType == "Asteroid") {
        return Localizer::Format("#autoLOC_8000139", {field.guiName, Localizer::Format(EnumDescription(asteroid->asteroidType)), Localizer::Format(EnumDescription(asteroid->asteroidClass))});
    }
    if (field.name == "location") return location->GetNodeBodyParameterString();
    return ActionModule::GetNodeBodyParameterString(field);
}
// ActionCreateAsteroid.cs:88
std::string ActionCreateAsteroid::GetInfo() { return Tag("#autoLOC_8004007"); }
// ActionCreateAsteroid.cs:93
void ActionCreateAsteroid::Save(ConfigNode& cfg) const {
    ActionModule::Save(cfg);
    asteroid->Save(cfg);
    location->Save(cfg);
}
// ActionCreateAsteroid.cs:100
void ActionCreateAsteroid::Load(const ConfigNode& cfg) {
    ActionModule::Load(cfg);
    asteroid->Load(cfg);
    location->Load(cfg);
}
// ActionCreateAsteroid.cs:107
void ActionCreateAsteroid::SpawnAsteroid() {
    ConfigNode::NodePtr protoVesselNode = GetProtoVesselNode();
    Vessel* vesselRef = AddVessel(*protoVesselNode);
    GameEvents::onAsteroidSpawned.Fire(vesselRef);
}

namespace {
// the shared tail of ActionCreateAsteroid / ActionCreateComet.GetProtoVesselNode (identical but for `alt` and the comet extras)
ConfigNode::NodePtr SpaceObjectNode(ParamChoices_VesselSimpleLocation& location, Mission* mission, const std::string& name, uint32_t persistentId, double radiusForAlt,
                                    const std::vector<ConfigNode::NodePtr>& partNodes) {
    bool flag = location.locationChoice == ParamChoices_VesselSimpleLocation::Choices::landed;
    bool flag2 = flag && location.landed->splashed;
    Vessel::Situations situations = flag ? Vessel::Situations::LANDED : Vessel::Situations::ORBITING;
    const Orbit orbit = flag ? RandomOrbitAround(location.landed->targetBody) : location.orbit->RelativeOrbit(mission);
    ConfigNode::NodePtr configNode4 = CreateVesselNode(name, VesselType::SpaceObject, orbit, 0, partNodes);
    VesselGroundLocation& landed = *location.landed;
    double num = landed.altitude;
    const Vector3d relSurfaceNVector = landed.targetBody->GetRelSurfaceNVector(landed.latitude, landed.longitude);
    if (flag2) {
        num = SurfaceHeight(landed.targetBody, relSurfaceNVector) - landed.targetBody->Radius;
        if (num < 0.0) {
            num = 0.0;
            flag = false;
            situations = Vessel::Situations::SPLASHED;
        } else {
            flag2 = false;
        }
    }
    configNode4->SetValue("persistentId", persistentId);
    configNode4->SetValue("sit", SituationName(situations));
    configNode4->SetValue("landedAt", landed.targetBody->name());
    configNode4->SetValue("lat", landed.latitude);
    configNode4->SetValue("lon", landed.longitude);
    configNode4->SetValue("alt", num + radiusForAlt);
    configNode4->SetValue("landed", flag);
    configNode4->SetValue("splashed", flag2);
    configNode4->AddValue("rot", FromTo(kUp, relSurfaceNVector.ToVector3f()) * AngleAxisQ(landed.rotation->eulerZ(), kUp));
    configNode4->SetValue("vesselSpawning", true);
    if (flag) {
        configNode4->SetValue("skipGroundPositioning", false);
        if (HasPQS(landed.targetBody)) {
            configNode4->SetValue("PQSMin", 0);
            configNode4->SetValue("PQSMax", 0);
        }
    } else {
        configNode4->SetValue("skipGroundPositioning", true);
    }
    return configNode4;
}
}  // namespace

// ActionCreateAsteroid.cs:114
ConfigNode::NodePtr ActionCreateAsteroid::GetProtoVesselNode() {
    ConfigNode::NodePtr configNode = CreatePartNode("PotatoRoid", asteroid->seed, {});
    auto configNode2 = std::make_shared<ConfigNode>("MODULE");
    configNode2->AddValue("name", "ModuleAsteroid");
    configNode2->AddValue("seed", asteroid->seed);
    const int value = (asteroid->asteroidType != Asteroid::AsteroidType::Glimmeroid) ? 1 : 2;
    configNode2->AddValue("currentState", value);
    configNode->AddNode(configNode2);
    auto configNode3 = std::make_shared<ConfigNode>("DISCOVERY");
    configNode3->AddValue("size", static_cast<int>(asteroid->asteroidClass));
    // (the DiscoveryInfo.GetClassRadius value is read after the orbit and the node are built; both are pure functions of the seed)
    const float classRadius = DiscoveryInfo::GetClassRadius(asteroid->asteroidClass, static_cast<int>(asteroid->seed));
    return SpaceObjectNode(*location, node->mission, asteroid->name, PersistentID(), static_cast<double>(classRadius), {configNode, configNode3});
}

// ActionCreateComet.cs:18
ActionCreateComet::ActionCreateComet() : ActionModule(kTypeName) {
    title = Tag("#autoLOC_8005445");
    comet = std::make_unique<Comet>();
    location = NewLocation(VesselGroundLocation::GizmoIcon::Comet, node, ParamChoices_VesselSimpleLocation::Choices::orbit);
    location->landed->splashed = true;
}
// ActionCreateComet.cs:30
Enumerator ActionCreateComet::Fire() {
    SpawnComet();
    co_return;
}
// ActionCreateComet.cs:36
Vector3f ActionCreateComet::ActionLocation() { return LocationActionLocation(*location); }
// ActionCreateComet.cs:50
void ActionCreateComet::OnCloned(ActionModule*& actionModuleBase) {
    ActionModule::OnCloned(actionModuleBase);
    comet->seed = Comet::GetRandomCometSeed();
    comet->persistentId = MissionDataEnvironment::UniquePersistentId();
}
// ActionCreateComet.cs:62
Orbit* ActionCreateComet::GetNodeOrbit() { return NodeOrbit(*location); }
// ActionCreateComet.cs:75
std::string ActionCreateComet::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.fieldType == "Comet") return Localizer::Format("#autoLOC_8000139", {field.guiName, comet->cometDisplayType, Localizer::Format(EnumDescription(comet->cometClass))});
    if (field.name == "location") return location->GetNodeBodyParameterString();
    return ActionModule::GetNodeBodyParameterString(field);
}
// ActionCreateComet.cs:88
std::string ActionCreateComet::GetInfo() { return Tag("#autoLOC_8005446"); }
// ActionCreateComet.cs:93
void ActionCreateComet::Save(ConfigNode& cfg) const {
    ActionModule::Save(cfg);
    comet->Save(cfg);
    location->Save(cfg);
}
// ActionCreateComet.cs:100
void ActionCreateComet::Load(const ConfigNode& cfg) {
    ActionModule::Load(cfg);
    comet->Load(cfg);
    location->Load(cfg);
}
// ActionCreateComet.cs:107
void ActionCreateComet::SpawnComet() {
    ConfigNode::NodePtr protoVesselNode = GetProtoVesselNode();
    Vessel* vesselRef = AddVessel(*protoVesselNode);
    GameEvents::onCometSpawned.Fire(vesselRef);
}
// ActionCreateComet.cs:114
ConfigNode::NodePtr ActionCreateComet::GetProtoVesselNode() {
    ConfigNode::NodePtr configNode = CreatePartNode("PotatoComet", comet->seed, {});
    std::string pidText;
    configNode->TryGetValue("persistentId", pidText);
    const uint32_t value = save_detail::ParseUInt(pidText);
    auto configNode2 = std::make_shared<ConfigNode>("MODULE");
    configNode2->AddValue("name", "ModuleComet");
    configNode2->AddValue("seed", comet->seed);
    configNode2->AddValue("type", comet->cometType);
    if (comet->seed == 1633779195u) configNode2->AddValue("prefabBaseURL", "Comets/Rosetta");
    configNode->AddNode(configNode2);
    auto configNode3 = std::make_shared<ConfigNode>("DISCOVERY");
    configNode3->AddValue("size", static_cast<int>(comet->cometClass));
    ConfigNode::NodePtr configNode4 = SpaceObjectNode(*location, node->mission, comet->name, PersistentID(), static_cast<double>(comet->radius), {configNode, configNode3});
    // CometManager.GenerateDefinition(...).CreateVesselNode(optimizedCollider: false, 0f, hasName: true)
    ConfigNode::NodePtr configNode6 = Env::cometVesselNode ? Env::cometVesselNode(comet->cometType, comet->cometClass, static_cast<int>(comet->seed)) : std::make_shared<ConfigNode>("COMET");
    auto configNode5 = std::make_shared<ConfigNode>("VESSELMODULES");
    configNode6->AddValue("cometPartId", value);
    configNode5->AddNode(configNode6);
    configNode4->AddNode(configNode5);
    return configNode4;
}

// ---- ActionCreateFlag ------------------------------------------------------------------------------------------------------------

// ActionCreateFlag.cs:23
ActionCreateFlag::ActionCreateFlag() : ActionModule(kTypeName) {
    title = Tag("#autoLOC_8004174");
    siteName = Tag("#autoLOC_8100159");
    plaqueText = Tag("#autoLOC_8100159");
    location = std::make_unique<VesselGroundLocation>(node, VesselGroundLocation::GizmoIcon::Flag);
    persistentID = MissionDataEnvironment::UniquePersistentId();
}
// ActionCreateFlag.cs:33
Enumerator ActionCreateFlag::Fire() {
    ConfigNode::NodePtr protoVesselNode = GetProtoVesselNode();
    AddVessel(*protoVesselNode);
    co_return;
}
// ActionCreateFlag.cs:71
ConfigNode::NodePtr ActionCreateFlag::CreateFlagPartNode(uint32_t id) {
    ConfigNode::NodePtr configNode = CreatePartNode("flag", id, {});
    configNode->SetValue("flag", node->mission->flagURL, true);
    auto configNode2 = std::make_shared<ConfigNode>("MODULE");
    configNode2->AddValue("name", "FlagSite");
    configNode2->AddValue("state", "Placed");
    configNode2->AddValue("PlaqueText", plaqueText);
    configNode->AddNode(configNode2);
    return configNode;
}
// ActionCreateFlag.cs:40
ConfigNode::NodePtr ActionCreateFlag::GetProtoVesselNode() {
    if (siteName.empty()) siteName = Tag("#autoLOC_8100159");
    const uint32_t uniqueFlightID = UniqueFlightID();
    const Orbit orbit = RandomOrbitAround(location->targetBody);
    ConfigNode::NodePtr configNode = CreateVesselNode(siteName, VesselType::Flag, orbit, 0, {CreateFlagPartNode(uniqueFlightID)});
    configNode->SetValue("persistentId", persistentID);
    configNode->SetValue("landedAt", location->targetBody->name());
    configNode->SetValue("lat", location->latitude);
    configNode->SetValue("lon", location->longitude);
    configNode->SetValue("sit", "LANDED");
    configNode->SetValue("alt", location->altitude);
    configNode->SetValue("skipGroundPositioning", false);
    configNode->SetValue("landed", true);
    configNode->SetValue("splashed", false);
    configNode->AddValue("prst", false);
    const Vector3d relSurfaceNVector = location->targetBody->GetRelSurfaceNVector(location->latitude, location->longitude);
    configNode->AddValue("nrm", kUp);
    configNode->AddValue("rot", FromTo(kUp, relSurfaceNVector.ToVector3f()) * AngleAxisQ(f32store(fx(location->rotation->eulerZ()) + fx(flagRotationOffset)), kUp));
    configNode->SetValue("vesselSpawning", true);
    if (HasPQS(location->targetBody)) {
        configNode->SetValue("PQSMin", 0);
        configNode->SetValue("PQSMax", 0);
    }
    return configNode;
}
// ActionCreateFlag.cs:83
Vector3f ActionCreateFlag::ActionLocation() { return location->GetWorldPosition(); }
// ActionCreateFlag.cs:88
void ActionCreateFlag::OnCloned(ActionModule*& actionModuleBase) {
    ActionModule::OnCloned(actionModuleBase);
    persistentID = MissionDataEnvironment::UniquePersistentId();
}
// ActionCreateFlag.cs:94
std::string ActionCreateFlag::GetInfo() { return Tag("#autoLOC_8004175"); }
// ActionCreateFlag.cs:99
std::string ActionCreateFlag::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "location") return location->GetNodeBodyParameterString();
    return ActionModule::GetNodeBodyParameterString(field);
}
// ActionCreateFlag.cs:108
void ActionCreateFlag::Save(ConfigNode& cfg) const {
    ActionModule::Save(cfg);
    cfg.AddValue("siteName", siteName);
    cfg.AddValue("plaqueText", plaqueText);
    cfg.AddValue("persistentId", persistentID);
    location->Save(cfg);
}
// ActionCreateFlag.cs:117
void ActionCreateFlag::Load(const ConfigNode& cfg) {
    ActionModule::Load(cfg);
    cfg.TryGetValue("siteName", siteName);
    cfg.TryGetValue("plaqueText", plaqueText);
    cfg.TryGetValue("persistentId", persistentID);
    location->Load(cfg);
}

// ---- ActionCreateLaunchSite ------------------------------------------------------------------------------------------------------

// ActionCreateLaunchSite.cs:14
ActionCreateLaunchSite::ActionCreateLaunchSite() : ActionModule(kTypeName) { title = Tag("#autoLOC_8000066"); }
// ActionCreateLaunchSite.cs:20
void ActionCreateLaunchSite::Initialize(MENode* n) {
    ActionModule::Initialize(n);
    launchSiteSituation = std::make_unique<LaunchSiteSituation>(n);
}
// ActionCreateLaunchSite.cs:26
Enumerator ActionCreateLaunchSite::Fire() {
    if (!node->IsDockedToStartNode()) {
        log::Warning("[MissionSystem]: Cannot spawn LaunchSites if they are not docked to the start node.");
        co_yield YieldInstruction::Null();
    }
    co_return;
}
// ActionCreateLaunchSite.cs:35
std::string ActionCreateLaunchSite::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (field.name == "launchSiteSituation") {
        std::string empty;
        if (launchSiteSituation == nullptr) return empty + Localizer::Format("autoLOC_8006028");
        empty += Localizer::Format("#autoLOC_8000283", {launchSiteSituation->launchSiteName});
        return empty + launchSiteSituation->launchSiteGroundLocation->GetNodeBodyParameterString();
    }
    return ActionModule::GetNodeBodyParameterString(field);
}
// ActionCreateLaunchSite.cs:50
void ActionCreateLaunchSite::RunValidation(Editor::MissionEditorValidator* validator) {
    if (!node->IsDockedToStartNode()) validator->AddNodeFail(node, Tag("#autoLOC_8006041"));
    if (launchSiteSituation->launchSiteName == "Unnamed LaunchSite") validator->AddNodeWarn(node, Tag("#autoLOC_8006042"));
    for (MENode* n : node->mission->nodes.GetListEnumerator()) {
        if (n->IsDockedToStartNode() && n->IsLaunchPadNode()) {
            auto* actionCreateLaunchSite = n->actionModules.empty() ? nullptr : dynamic_cast<ActionCreateLaunchSite*>(n->actionModules[0].get());
            if (actionCreateLaunchSite != nullptr && actionCreateLaunchSite->launchSiteSituation->launchSiteName == launchSiteSituation->launchSiteName && n != node)
                validator->AddNodeFail(node, Tag("#autoLOC_8006043"));
        }
    }
    if (launchSiteSituation->launchSiteGroundLocation->targetBody == Home() && launchSiteSituation->launchSiteGroundLocation->latitude == 0.0 &&
        launchSiteSituation->launchSiteGroundLocation->longitude == 0.0)
        validator->AddNodeFail(node, Tag("#autoLOC_8006044"));
}
// ActionCreateLaunchSite.cs:78
std::string ActionCreateLaunchSite::GetInfo() { return Tag("#autoLOC_8004009"); }
// ActionCreateLaunchSite.cs:83
void ActionCreateLaunchSite::Save(ConfigNode& cfg) const {
    ActionModule::Save(cfg);
    launchSiteSituation->Save(cfg);
}
// ActionCreateLaunchSite.cs:89
void ActionCreateLaunchSite::Load(const ConfigNode& cfg) {
    ActionModule::Load(cfg);
    ConfigNode* configNode = cfg.GetNode("LAUNCHSITESITUATION");
    if (configNode != nullptr) launchSiteSituation->Load(*configNode);
}

}  // namespace Actions

// ---- registration ----------------------------------------------------------------------------------------------------------------

void RegisterNodeLibrarySlice2Actions() {
    using namespace Actions;
    ActionModuleRegistry::Register({ActionCreateVessel::kTypeName, [] { return std::make_unique<ActionCreateVessel>(); }});
    ActionModuleRegistry::Register({ActionCreateKerbal::kTypeName, [] { return std::make_unique<ActionCreateKerbal>(); }});
    ActionModuleRegistry::Register({ActionCreateAsteroid::kTypeName, [] { return std::make_unique<ActionCreateAsteroid>(); }});
    ActionModuleRegistry::Register({ActionCreateComet::kTypeName, [] { return std::make_unique<ActionCreateComet>(); }});
    ActionModuleRegistry::Register({ActionCreateFlag::kTypeName, [] { return std::make_unique<ActionCreateFlag>(); }});
    ActionModuleRegistry::Register({ActionCreateLaunchSite::kTypeName, [] { return std::make_unique<ActionCreateLaunchSite>(); }});
}

}  // namespace ksp::Expansions::Missions
