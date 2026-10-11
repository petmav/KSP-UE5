// WP-54: AwardDefinition.cs, Awards.cs, AwardModule*.cs, MissionAwards.cs. See mission_awards.hpp.
#include "ksp/missions/mission_awards.hpp"

#include <algorithm>

#include "../save/save_util.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/psystem_setup.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Missions {

namespace {
template <class T>
void AddUniqueTo(std::vector<T>& list, const T& item) {
    if (std::find(list.begin(), list.end(), item) == list.end()) list.push_back(item);
}
}  // namespace

// ---- AwardEnvironment --------------------------------------------------------------------------------------------------------

int AwardEnvironment::ResourceDefinitionId(const std::string& resourceName) {
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    if (lib == nullptr) return -1;
    const auto* def = lib->GetDefinition(resourceName);
    return def != nullptr ? def->id() : -1;
}

std::vector<Vessel*> AwardEnvironment::Vessels() {
    if (vessels) return vessels();
    return FlightRegistry::Vessels;
}

Vessel* AwardEnvironment::ActiveVessel() {
    if (activeVessel) return activeVessel();
    return FlightRegistry::ActiveVessel;
}

Vessel* AwardEnvironment::VesselByPersistentId(uint32_t persistentId) {
    if (vesselByPersistentId) return vesselByPersistentId(persistentId);
    for (Vessel* v : FlightRegistry::Vessels)
        if (v != nullptr && v->persistentId == persistentId) return v;
    return nullptr;
}

bool AwardEnvironment::ConnectedResourceTotal(Vessel* vessel, const std::string& resourceName, double& amount) {
    amount = 0.0;
    if (connectedResourceTotal) return connectedResourceTotal(vessel, resourceName, amount);
    return false;
}

bool AwardEnvironment::CurrentGameIsMission() {
    if (currentGameIsMission) return currentGameIsMission();
    Game* g = HighLogic::CurrentGame();
    return g != nullptr && g->Mode == Game::Modes::MISSION;
}

void AwardEnvironment::Reset() {
    awardDefinitions = nullptr;
    vessels = nullptr;
    activeVessel = nullptr;
    vesselByPersistentId = nullptr;
    findVessel = nullptr;
    connectedResourceTotal = nullptr;
    currentGameIsMission = nullptr;
    crewRosterStatuses = nullptr;
    launchPad = nullptr;
    getBodyByName = nullptr;
}

// ---- AwardDefinition / Awards ------------------------------------------------------------------------------------------------

// AwardDefinition.cs:19
void AwardDefinition::Load(const ConfigNode& node) {
    node.TryGetValue("id", id);
    node.TryGetValue("awardName", name);
    node.TryGetValue("displayName", displayName);
    node.TryGetValue("description", description);
}

// AwardDefinition.cs:36
void AwardDefinition::Save(ConfigNode& node) const {
    node.AddValue("id", id);
    node.AddValue("awardName", name);
    node.AddValue("displayName", displayName);
    node.AddValue("description", description);
}

// Awards.cs:24
void Awards::SetupAwards() {
    awardsDictionary_.clear();
    AwardDefinitions.clear();
    for (AwardDefinition& d : internalAwards) AwardDefinitions.push_back(&d);
    int i = 0;
    for (int count = static_cast<int>(internalAwards.size()); i < count; ++i) {
        const std::string& key = internalAwards[static_cast<size_t>(i)].id;
        if (awardsDictionary_.count(key)) throw ArgumentException("An item with the same key has already been added.");
        awardsDictionary_[key] = &internalAwards[static_cast<size_t>(i)];
    }
}

// Awards.cs:35
AwardDefinition* Awards::GetAwardDefinition(const std::string& id) {
    auto it = awardsDictionary_.find(id);
    if (it != awardsDictionary_.end()) return it->second;
    return nullptr;
}

// ---- AwardModule -------------------------------------------------------------------------------------------------------------

// AwardModule.cs:37
AwardModule::AwardModule(std::string typeName, MENode* node) : DynamicModule(std::move(typeName), node) { enabled = false; }

// AwardModule.cs:43
AwardModule::AwardModule(std::string typeName, MENode* node, AwardDefinition* definition) : AwardModule(std::move(typeName), node) { awardID = definition->id; }

// AwardModule.cs:19
AwardDefinition* AwardModule::Definition() {
    if (definition_ == nullptr && !awardID.empty() && AwardEnvironment::awardDefinitions) {
        Awards* awards = AwardEnvironment::awardDefinitions();
        if (awards != nullptr) definition_ = awards->GetAwardDefinition(awardID);
    }
    return definition_;
}

// AwardModule.cs:48
std::string AwardModule::GetDisplayName() {
    if (Definition() != nullptr) return Definition()->displayName;
    return DynamicModule::GetDisplayName();
}

// AwardModule.cs:57
bool AwardModule::EvaluateCondition(Mission*) { return false; }

// AwardModule.cs:63
void AwardModule::EvaluateAward(Mission* mission, std::vector<std::string>& awards) {
    if (EvaluateCondition(mission) && Definition() != nullptr) {
        awards.push_back(Definition()->id);
        log::Info("[MissionAwards]: <b><color=green> Award earned " + KSP::Localization::Localizer::Format(Definition()->displayName) + ". </color></b>");
    }
}

// AwardModule.cs:85
void AwardModule::Load(const ConfigNode& node) {
    DynamicModule::Load(node);
    node.TryGetValue("enabled", enabled);
    node.TryGetValue("score", score);
    node.TryGetValue("awardID", awardID);
}

// AwardModule.cs:93
void AwardModule::Save(ConfigNode& node) const {
    DynamicModule::Save(node);
    node.AddValue("awardID", awardID);
    node.AddValue("enabled", enabled);
    node.AddValue("score", score);
}

// ---- AwardModule_Fuel --------------------------------------------------------------------------------------------------------

// AwardModule_Fuel.cs:19
AwardModule_Fuel::AwardModule_Fuel(MENode* node) : DynamicModuleT(kTypeName, node) {}

// AwardModule_Fuel.cs:24
AwardModule_Fuel::AwardModule_Fuel(MENode* node, AwardDefinition* definition) : DynamicModuleT(kTypeName, node, definition) {}

AwardModule_Fuel::~AwardModule_Fuel() {
    if (subscribed_) StopTracking();
}

// AwardModule_Fuel.cs:29
bool AwardModule_Fuel::EvaluateCondition(Mission*) {
    int i = 0;
    for (int count = vesselResource.Count(); i < count; ++i) {
        Vessel* v = vesselResource.KeyAt(i);
        totalSpentResource += vesselResource.At(i) - GetCurrentResourceFromVessel(v);
    }
    return totalSpentResource <= resourceAmount;
}

// AwardModule_Fuel.cs:37
void AwardModule_Fuel::StartTracking() {
    resourceDefinitionId = AwardEnvironment::ResourceDefinitionId(resourceName);
    if (resourceDefinitionId != -1) {
        GameEvents::onNewVesselCreated.Add(this, &AwardModule_Fuel::OnVesselCreated);
        GameEvents::onVesselWillDestroy.Add(this, &AwardModule_Fuel::OnVesselWillDestroy);
        subscribed_ = true;
    }
}

// AwardModule_Fuel.cs:47
void AwardModule_Fuel::StopTracking() {
    GameEvents::onNewVesselCreated.Remove(this, &AwardModule_Fuel::OnVesselCreated);
    GameEvents::onVesselWillDestroy.Remove(this, &AwardModule_Fuel::OnVesselWillDestroy);
    subscribed_ = false;
}

// AwardModule_Fuel.cs:55
double AwardModule_Fuel::GetCurrentResourceFromVessel(Vessel* v) {
    double amount = 0.0;
    if (resourceDefinitionId != -1) AwardEnvironment::ConnectedResourceTotal(v, resourceName, amount);
    return amount;
}

// AwardModule_Fuel.cs:66
void AwardModule_Fuel::OnVesselWillDestroy(Vessel* data) {
    if (vesselResource.ContainsKey(data)) {
        totalSpentResource += vesselResource[data] - GetCurrentResourceFromVessel(data);
        vesselResource.Remove(data);
    }
}

// AwardModule_Fuel.cs:75
void AwardModule_Fuel::OnVesselCreated(Vessel* data) {
    if (!vesselResource.ContainsKey(data)) vesselResource.Add(data, GetCurrentResourceFromVessel(data));
}

// AwardModule_Fuel.cs:82
void AwardModule_Fuel::Load(const ConfigNode& node) {
    AwardModule::Load(node);
    node.TryGetValue("resourceName", resourceName);
    node.TryGetValue("resourceAmount", resourceAmount);
    ConfigNode* configNode = nullptr;
    if (!node.TryGetNode("PROGRESS", configNode)) return;
    configNode->TryGetValue("totalSpentResource", totalSpentResource);
    const std::vector<ConfigNode*> nodes = configNode->GetNodes("VESSELRESOURCE");
    uint32_t value = 0u;
    double value2 = 0.0;
    int i = 0;
    for (int num = static_cast<int>(nodes.size()); i < num; ++i) {
        Vessel* vessel = nullptr;
        if (nodes[static_cast<size_t>(i)]->TryGetValue("vesselID", value) &&
            (AwardEnvironment::findVessel ? AwardEnvironment::findVessel(value, vessel) : ((vessel = AwardEnvironment::VesselByPersistentId(value)) != nullptr))) {
            nodes[static_cast<size_t>(i)]->TryGetValue("currentResource", value2);
            vesselResource.Add(vessel, value2);
        }
    }
}

// AwardModule_Fuel.cs:107
void AwardModule_Fuel::Save(ConfigNode& node) const {
    AwardModule::Save(node);
    node.AddValue("resourceName", resourceName);
    node.AddValue("resourceAmount", resourceAmount);
    if (!AwardEnvironment::CurrentGameIsMission()) return;
    ConfigNode* configNode = node.AddNode("PROGRESS");
    configNode->AddValue("totalSpentResource", totalSpentResource);
    int i = 0;
    for (int count = vesselResource.Count(); i < count; ++i) {
        Vessel* vessel = vesselResource.KeyAt(i);
        if (vessel != nullptr) {
            ConfigNode* configNode2 = configNode->AddNode("VESSELRESOURCE");
            configNode2->AddValue("vesselID", vessel->persistentId);
            configNode2->AddValue("currentResource", vesselResource[vessel]);
        }
    }
}

// ---- AwardModule_KerbalsDied -------------------------------------------------------------------------------------------------

// AwardModule_KerbalsDied.cs:5
AwardModule_KerbalsDied::AwardModule_KerbalsDied(MENode* node) : DynamicModuleT(kTypeName, node) {}

// AwardModule_KerbalsDied.cs:9
AwardModule_KerbalsDied::AwardModule_KerbalsDied(MENode* node, AwardDefinition* definition) : DynamicModuleT(kTypeName, node, definition) {}

// AwardModule_KerbalsDied.cs:14
bool AwardModule_KerbalsDied::EvaluateCondition(Mission*) {
    std::vector<int> statuses;
    if (AwardEnvironment::crewRosterStatuses) {
        statuses = AwardEnvironment::crewRosterStatuses();
    } else {
        KerbalRoster& crewRoster = *HighLogic::CurrentGame()->CrewRoster;
        for (int i = 0; i < crewRoster.Count(); ++i) statuses.push_back(static_cast<int>(crewRoster[i]->rosterStatus()));
    }
    int count = static_cast<int>(statuses.size());
    do {
        if (count-- <= 0) return true;
    } while (statuses[static_cast<size_t>(count)] != static_cast<int>(ProtoCrewMember_RosterStatus::Assigned));
    return false;
}

// ---- AwardModule_KerbalsSurvived ---------------------------------------------------------------------------------------------

// AwardModule_KerbalsSurvived.cs:7
AwardModule_KerbalsSurvived::AwardModule_KerbalsSurvived(MENode* node) : DynamicModuleT(kTypeName, node) {}

// AwardModule_KerbalsSurvived.cs:11
AwardModule_KerbalsSurvived::AwardModule_KerbalsSurvived(MENode* node, AwardDefinition* definition) : DynamicModuleT(kTypeName, node, definition) {}

AwardModule_KerbalsSurvived::~AwardModule_KerbalsSurvived() {
    if (subscribed_) StopTracking();
}

// AwardModule_KerbalsSurvived.cs:19
bool AwardModule_KerbalsSurvived::EvaluateCondition(Mission*) { return kerbalsAreAlive; }

// AwardModule_KerbalsSurvived.cs:24
void AwardModule_KerbalsSurvived::StartTracking() {
    GameEvents::onCrewKilled.Add(this, &AwardModule_KerbalsSurvived::OnCrewKilled);
    subscribed_ = true;
}

// AwardModule_KerbalsSurvived.cs:29
void AwardModule_KerbalsSurvived::StopTracking() {
    GameEvents::onCrewKilled.Remove(this, &AwardModule_KerbalsSurvived::OnCrewKilled);
    subscribed_ = false;
}

// AwardModule_KerbalsSurvived.cs:34
void AwardModule_KerbalsSurvived::OnCrewKilled(EventReport*) {
    kerbalsAreAlive = false;
    StopTracking();
}

// AwardModule_KerbalsSurvived.cs:40
void AwardModule_KerbalsSurvived::Load(const ConfigNode& node) {
    AwardModule::Load(node);
    ConfigNode* configNode = nullptr;
    if (node.TryGetNode("PROGRESS", configNode)) configNode->TryGetValue("kerbalsAreAlive", kerbalsAreAlive);
}

// AwardModule_KerbalsSurvived.cs:49
void AwardModule_KerbalsSurvived::Save(ConfigNode& node) const {
    AwardModule::Save(node);
    if (AwardEnvironment::CurrentGameIsMission()) node.AddNode("PROGRESS")->AddValue("kerbalsAreAlive", kerbalsAreAlive);
}

// ---- AwardModule_LandedOn ----------------------------------------------------------------------------------------------------

// AwardModule_LandedOn.cs:16
AwardModule_LandedOn::AwardModule_LandedOn(MENode* node) : DynamicModuleT(kTypeName, node) {}

// AwardModule_LandedOn.cs:20
AwardModule_LandedOn::AwardModule_LandedOn(MENode* node, AwardDefinition* definition) : DynamicModuleT(kTypeName, node, definition) {}

// AwardModule_LandedOn.cs:25
bool AwardModule_LandedOn::EvaluateCondition(Mission*) {
    Vessel* vessel = GetVessel();
    if (vessel != nullptr && vessel->situation == Vessel::Situations::LANDED) {
        std::optional<AwardEnvironment::LaunchPad> pad;
        if (AwardEnvironment::launchPad) {
            pad = AwardEnvironment::launchPad();
        } else if (PSystemSetup::Instance != nullptr) {
            if (PSystemSetup::SpaceCenterFacility* f = PSystemSetup::Instance->GetSpaceCenterFacility("LaunchPad")) {
                AwardEnvironment::LaunchPad lp;
                lp.pqsName = f->pqsName;
                lp.hasFacilityPQS = f->facilityPQS != nullptr;
                lp.position = f->facilityTransformPosition;
                pad = lp;
            }
        }
        if (pad.has_value() && !pad->pqsName.empty() && pad->hasFacilityPQS) {
            CelestialBody* bodyByName = nullptr;
            if (AwardEnvironment::getBodyByName) {
                bodyByName = AwardEnvironment::getBodyByName(pad->pqsName);
            } else {
                for (CelestialBody* b : SaveEnvironment::Current().localBodies)
                    if (b != nullptr && b->bodyName == pad->pqsName) bodyByName = b;
            }
            if (bodyByName != nullptr) {
                Editor::SurfaceLocation surfaceLocation;
                bodyByName->GetLatLonAlt(pad->position, surfaceLocation.latitude, surfaceLocation.longitude, surfaceLocation.altitude);
                return Contracts::Lite::CelestialUtilities::GreatCircleDistance(bodyByName, vessel->latitude, vessel->longitude, surfaceLocation.latitude,
                                                                                surfaceLocation.longitude) <= distance;
            }
        }
    }
    return false;
}

// AwardModule_LandedOn.cs:40
Vessel* AwardModule_LandedOn::GetVessel() {
    if (vesselID == 0) return AwardEnvironment::ActiveVessel();
    const uint32_t key = node->mission->CurrentVesselID(node, vesselID);
    if (Vessel* v = AwardEnvironment::VesselByPersistentId(key)) return v;
    log::Error("[AwardModule] Unable to find VesselID (" + std::to_string(vesselID) + ") from award " + GetDisplayName() + " in FlightGlobals.");
    return nullptr;
}

// AwardModule_LandedOn.cs:56
void AwardModule_LandedOn::Load(const ConfigNode& node) {
    AwardModule::Load(node);
    node.TryGetValue("vesselID", vesselID);
    node.TryGetValue("distance", distance);
}

// AwardModule_LandedOn.cs:63
void AwardModule_LandedOn::Save(ConfigNode& node) const {
    AwardModule::Save(node);
    node.AddValue("vesselID", vesselID);
    node.AddValue("distance", distance);
}

// ---- AwardModule_MissionTime -------------------------------------------------------------------------------------------------

// AwardModule_MissionTime.cs:10
AwardModule_MissionTime::AwardModule_MissionTime(MENode* node) : DynamicModuleT(kTypeName, node) {}

// AwardModule_MissionTime.cs:14
AwardModule_MissionTime::AwardModule_MissionTime(MENode* node, AwardDefinition* definition) : DynamicModuleT(kTypeName, node, definition) {}

// AwardModule_MissionTime.cs:19
bool AwardModule_MissionTime::EvaluateCondition(Mission*) { return MissionsEnvironment::GameUniversalTime() <= missionTime; }

// AwardModule_MissionTime.cs:24
void AwardModule_MissionTime::Load(const ConfigNode& node) {
    AwardModule::Load(node);
    node.TryGetValue("missionTime", missionTime);
}

// AwardModule_MissionTime.cs:30
void AwardModule_MissionTime::Save(ConfigNode& node) const {
    AwardModule::Save(node);
    node.AddValue("missionTime", missionTime);
}

// ---- AwardModule_PlanetsLanded -----------------------------------------------------------------------------------------------

// AwardModule_PlanetsLanded.cs:14
AwardModule_PlanetsLanded::AwardModule_PlanetsLanded(MENode* node) : DynamicModuleT(kTypeName, node) {}

// AwardModule_PlanetsLanded.cs:19
AwardModule_PlanetsLanded::AwardModule_PlanetsLanded(MENode* node, AwardDefinition* definition) : DynamicModuleT(kTypeName, node, definition) {}

AwardModule_PlanetsLanded::~AwardModule_PlanetsLanded() {
    if (subscribed_) StopTracking();
}

// AwardModule_PlanetsLanded.cs:24
bool AwardModule_PlanetsLanded::EvaluateCondition(Mission*) { return static_cast<int>(landedOn.size()) >= planetsLanded; }

// AwardModule_PlanetsLanded.cs:29
void AwardModule_PlanetsLanded::StartTracking() {
    GameEvents::onVesselSituationChange.Add(this, &AwardModule_PlanetsLanded::OnVesselSitutationChange);
    subscribed_ = true;
}

// AwardModule_PlanetsLanded.cs:34
void AwardModule_PlanetsLanded::StopTracking() {
    GameEvents::onVesselSituationChange.Remove(this, &AwardModule_PlanetsLanded::OnVesselSitutationChange);
    subscribed_ = false;
}

// AwardModule_PlanetsLanded.cs:39
void AwardModule_PlanetsLanded::OnVesselSitutationChange(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> data) {
    if (data.to == Vessel_Situations::LANDED) AddUniqueTo(landedOn, data.host->lastBody->displayName());
}

// AwardModule_PlanetsLanded.cs:47
void AwardModule_PlanetsLanded::Load(const ConfigNode& node) {
    AwardModule::Load(node);
    node.TryGetValue("planetsLanded", planetsLanded);
    ConfigNode* configNode = nullptr;
    if (node.TryGetNode("PROGRESS", configNode)) landedOn = configNode->GetValuesList("landedOn");
}

// AwardModule_PlanetsLanded.cs:58
void AwardModule_PlanetsLanded::Save(ConfigNode& node) const {
    AwardModule::Save(node);
    node.AddValue("planetsLanded", planetsLanded);
    if (AwardEnvironment::CurrentGameIsMission()) {
        ConfigNode* configNode = node.AddNode("PROGRESS");
        for (size_t i = 0; i < landedOn.size(); ++i) configNode->AddValue("landedOn", landedOn[i]);
    }
}

// ---- AwardModule_Score -------------------------------------------------------------------------------------------------------

// AwardModule_Score.cs:7
AwardModule_Score::AwardModule_Score(MENode* node) : DynamicModuleT(kTypeName, node) { enabled = true; }

// AwardModule_Score.cs:13
AwardModule_Score::AwardModule_Score(MENode* node, AwardDefinition* definition) : DynamicModuleT(kTypeName, node, definition) { enabled = true; }

// AwardModule_Score.cs:19
bool AwardModule_Score::EvaluateCondition(Mission* mission) {
    if (score > 0.0f) return mission->currentScore >= score;
    return false;
}

// AwardModule_Score.cs:28
void AwardModule_Score::Load(const ConfigNode& node) { AwardModule::Load(node); }

// AwardModule_Score.cs:33
void AwardModule_Score::Save(ConfigNode& node) const { AwardModule::Save(node); }

// ---- AwardModule_TraveledLessThan / MoreThan ---------------------------------------------------------------------------------

// AwardModule_TraveledLessThan.cs:13
AwardModule_TraveledLessThan::AwardModule_TraveledLessThan(MENode* node) : DynamicModuleT(kTypeName, node) {}

// AwardModule_TraveledLessThan.cs:17
AwardModule_TraveledLessThan::AwardModule_TraveledLessThan(MENode* node, AwardDefinition* definition) : DynamicModuleT(kTypeName, node, definition) {}

AwardModule_TraveledLessThan::~AwardModule_TraveledLessThan() {
    if (subscribed_) StopTracking();
}

// AwardModule_TraveledLessThan.cs:22
bool AwardModule_TraveledLessThan::EvaluateCondition(Mission*) {
    const std::vector<Vessel*> vessels = AwardEnvironment::Vessels();
    int i = 0;
    for (int count = static_cast<int>(vessels.size()); i < count; ++i) distanceTraveled += vessels[static_cast<size_t>(i)]->distanceTraveled;
    return distanceTraveled / 1000.0 < distance;
}

// AwardModule_TraveledLessThan.cs:34
void AwardModule_TraveledLessThan::StartTracking() {
    GameEvents::onVesselWillDestroy.Add(this, &AwardModule_TraveledLessThan::OnVesselWillDestroy);
    subscribed_ = true;
}

// AwardModule_TraveledLessThan.cs:39
void AwardModule_TraveledLessThan::StopTracking() {
    GameEvents::onVesselWillDestroy.Remove(this, &AwardModule_TraveledLessThan::OnVesselWillDestroy);
    subscribed_ = false;
}

// AwardModule_TraveledLessThan.cs:44
void AwardModule_TraveledLessThan::OnVesselWillDestroy(Vessel* data) { distanceTraveled += data->distanceTraveled; }

// AwardModule_TraveledLessThan.cs:49
void AwardModule_TraveledLessThan::Load(const ConfigNode& node) {
    AwardModule::Load(node);
    node.TryGetValue("distance", distance);
    ConfigNode* configNode = nullptr;
    if (node.TryGetNode("PROGRESS", configNode)) configNode->TryGetValue("distanceTraveled", distanceTraveled);
}

// AwardModule_TraveledLessThan.cs:59
void AwardModule_TraveledLessThan::Save(ConfigNode& node) const {
    AwardModule::Save(node);
    node.AddValue("distance", distance);
    if (AwardEnvironment::CurrentGameIsMission()) node.AddNode("PROGRESS")->AddValue("distanceTraveled", distanceTraveled);
}

// AwardModule_TraveledMoreThan.cs:13
AwardModule_TraveledMoreThan::AwardModule_TraveledMoreThan(MENode* node) : DynamicModuleT(kTypeName, node) {}

// AwardModule_TraveledMoreThan.cs:17
AwardModule_TraveledMoreThan::AwardModule_TraveledMoreThan(MENode* node, AwardDefinition* definition) : DynamicModuleT(kTypeName, node, definition) {}

AwardModule_TraveledMoreThan::~AwardModule_TraveledMoreThan() {
    if (subscribed_) StopTracking();
}

// AwardModule_TraveledMoreThan.cs:22
bool AwardModule_TraveledMoreThan::EvaluateCondition(Mission*) {
    const std::vector<Vessel*> vessels = AwardEnvironment::Vessels();
    int i = 0;
    for (int count = static_cast<int>(vessels.size()); i < count; ++i) distanceTraveled += vessels[static_cast<size_t>(i)]->distanceTraveled;
    return distanceTraveled / 1000.0 > distance;
}

// AwardModule_TraveledMoreThan.cs:34
void AwardModule_TraveledMoreThan::StartTracking() {
    GameEvents::onVesselWillDestroy.Add(this, &AwardModule_TraveledMoreThan::OnVesselWillDestroy);
    subscribed_ = true;
}

// AwardModule_TraveledMoreThan.cs:39
void AwardModule_TraveledMoreThan::StopTracking() {
    GameEvents::onVesselWillDestroy.Remove(this, &AwardModule_TraveledMoreThan::OnVesselWillDestroy);
    subscribed_ = false;
}

// AwardModule_TraveledMoreThan.cs:44
void AwardModule_TraveledMoreThan::OnVesselWillDestroy(Vessel* data) { distanceTraveled += data->distanceTraveled; }

// AwardModule_TraveledMoreThan.cs:49
void AwardModule_TraveledMoreThan::Load(const ConfigNode& node) {
    AwardModule::Load(node);
    node.TryGetValue("distance", distance);
    ConfigNode* configNode = nullptr;
    if (node.TryGetNode("PROGRESS", configNode)) configNode->TryGetValue("distanceTraveled", distanceTraveled);
}

// AwardModule_TraveledMoreThan.cs:59
void AwardModule_TraveledMoreThan::Save(ConfigNode& node) const {
    AwardModule::Save(node);
    node.AddValue("distance", distance);
    if (AwardEnvironment::CurrentGameIsMission()) node.AddNode("PROGRESS")->AddValue("distanceTraveled", distanceTraveled);
}

// ---- AwardModule_VesselsUsed -------------------------------------------------------------------------------------------------

// AwardModule_VesselsUsed.cs:14
AwardModule_VesselsUsed::AwardModule_VesselsUsed(MENode* node) : DynamicModuleT(kTypeName, node) {}

// AwardModule_VesselsUsed.cs:19
AwardModule_VesselsUsed::AwardModule_VesselsUsed(MENode* node, AwardDefinition* definition) : DynamicModuleT(kTypeName, node, definition) {}

AwardModule_VesselsUsed::~AwardModule_VesselsUsed() {
    if (subscribed_) StopTracking();
}

// AwardModule_VesselsUsed.cs:24
bool AwardModule_VesselsUsed::EvaluateCondition(Mission*) { return vesselsUsed == static_cast<int>(vesselsUsedList.size()); }

// AwardModule_VesselsUsed.cs:29
void AwardModule_VesselsUsed::StartTracking() {
    GameEvents::onVesselChange.Add(this, &AwardModule_VesselsUsed::OnVesselChange);
    subscribed_ = true;
}

// AwardModule_VesselsUsed.cs:34
void AwardModule_VesselsUsed::StopTracking() {
    GameEvents::onVesselChange.Remove(this, &AwardModule_VesselsUsed::OnVesselChange);
    subscribed_ = false;
}

// AwardModule_VesselsUsed.cs:39
void AwardModule_VesselsUsed::OnVesselChange(Vessel* data) { AddUniqueTo(vesselsUsedList, data->persistentId); }

// AwardModule_VesselsUsed.cs:44
void AwardModule_VesselsUsed::Load(const ConfigNode& node) {
    AwardModule::Load(node);
    node.TryGetValue("vesselsUsed", vesselsUsed);
    ConfigNode* configNode = nullptr;
    if (node.TryGetNode("PROGRESS", configNode)) {
        const std::vector<std::string> valuesList = configNode->GetValuesList("vesselUsedID");
        int i = 0;
        for (int count = static_cast<int>(valuesList.size()); i < count; ++i) vesselsUsedList.push_back(save_detail::ParseUInt(valuesList[static_cast<size_t>(i)]));
    }
}

// AwardModule_VesselsUsed.cs:60
void AwardModule_VesselsUsed::Save(ConfigNode& node) const {
    AwardModule::Save(node);
    node.AddValue("vesselsUsed", vesselsUsed);
    if (AwardEnvironment::CurrentGameIsMission()) {
        ConfigNode* configNode = node.AddNode("PROGRESS");
        for (size_t i = 0; i < vesselsUsedList.size(); ++i) configNode->AddValue("vesselUsedID", vesselsUsedList[i]);
    }
}

// ---- MissionAwards -----------------------------------------------------------------------------------------------------------

// MissionAwards.cs:15
MissionAwards::MissionAwards(MENode* n) : DynamicModuleList(n) {
    rootNodeName = "AWARDMODULES";
    moduleNodeName = "AWARDMODULE";
}

// MissionAwards.cs:22
void MissionAwards::FillAwardsList() {
    Awards* awards = AwardEnvironment::awardDefinitions ? AwardEnvironment::awardDefinitions() : nullptr;
    if (!(awards != nullptr)) return;
    int i = 0;
    for (int count = static_cast<int>(awards->AwardDefinitions.size()); i < count; ++i) {
        AwardDefinition* def = awards->AwardDefinitions[static_cast<size_t>(i)];
        const DynamicModuleType* classByName = DynamicModuleRegistry::GetClassByName(def->name);
        if (classByName != nullptr && !ContainsDefinition(*def)) {
            std::unique_ptr<DynamicModule> item = classByName->createWithDefinition(node(), def);
            activeModules.push_back(std::move(item));
        }
    }
}

// MissionAwards.cs:41
bool MissionAwards::ContainsDefinition(const AwardDefinition& definition) {
    int count = static_cast<int>(activeModules.size());
    do {
        if (count-- <= 0) return false;
        auto* awardModule = dynamic_cast<AwardModule*>(activeModules[static_cast<size_t>(count)].get());
        if (awardModule != nullptr && awardModule->Definition() != nullptr && awardModule->Definition()->id == definition.id) return true;
    } while (true);
}

// MissionAwards.cs:54
std::vector<std::string> MissionAwards::GetSupportedTypes() {
    if (supportedTypes.empty()) supportedTypes = DynamicModuleRegistry::SubclassesOf("AwardModule");
    return supportedTypes;
}

// MissionAwards.cs:75
void MissionAwards::EvaluateAwards(Mission* mission) {
    StopTracking();
    int i = 0;
    for (int count = static_cast<int>(activeModules.size()); i < count; ++i) {
        if (auto* awardModule = dynamic_cast<AwardModule*>(activeModules[static_cast<size_t>(i)].get()); awardModule != nullptr && awardModule->enabled)
            awardModule->EvaluateAward(mission, awardedAwards);
    }
}

// MissionAwards.cs:87
void MissionAwards::StartTracking() {
    int i = 0;
    for (int count = static_cast<int>(activeModules.size()); i < count; ++i) {
        if (auto* awardModule = dynamic_cast<AwardModule*>(activeModules[static_cast<size_t>(i)].get()); awardModule != nullptr && awardModule->enabled)
            awardModule->StartTracking();
    }
    log::Info("[MissionAwards]: Awards tracking started.");
}

// MissionAwards.cs:101
void MissionAwards::StopTracking() {
    int i = 0;
    for (int count = static_cast<int>(activeModules.size()); i < count; ++i) {
        if (auto* awardModule = dynamic_cast<AwardModule*>(activeModules[static_cast<size_t>(i)].get()); awardModule != nullptr && awardModule->enabled)
            awardModule->StopTracking();
    }
    log::Info("[MissionAwards]: Awards tracking stopped.");
}

// DynamicModuleList.cs:140 Clone
std::unique_ptr<DynamicModuleList> MissionAwards::Clone() const {
    auto list = std::make_unique<MissionAwards>(node());
    list->activeModules.clear();
    for (size_t i = 0; i < activeModules.size(); ++i) list->activeModules.push_back(activeModules[i]->Clone());
    list->supportedTypes = supportedTypes;
    list->awardedAwards = awardedAwards;
    return list;
}

// ---- registration ------------------------------------------------------------------------------------------------------------

namespace {
template <class T>
void RegisterAward() {
    DynamicModuleType t;
    t.name = T::kTypeName;
    t.bases = {"AwardModule", "DynamicModule"};
    t.create = [](MENode* node) -> std::unique_ptr<DynamicModule> { return std::make_unique<T>(node); };
    t.createWithDefinition = [](MENode* node, AwardDefinition* d) -> std::unique_ptr<DynamicModule> { return std::make_unique<T>(node, d); };
    DynamicModuleRegistry::Register(std::move(t));
}
}  // namespace

void RegisterAwardModules() {
    RegisterAward<AwardModule_Fuel>();
    RegisterAward<AwardModule_KerbalsDied>();
    RegisterAward<AwardModule_KerbalsSurvived>();
    RegisterAward<AwardModule_LandedOn>();
    RegisterAward<AwardModule_MissionTime>();
    RegisterAward<AwardModule_PlanetsLanded>();
    RegisterAward<AwardModule_Score>();
    RegisterAward<AwardModule_TraveledLessThan>();
    RegisterAward<AwardModule_TraveledMoreThan>();
    RegisterAward<AwardModule_VesselsUsed>();
}

}  // namespace ksp::Expansions::Missions
