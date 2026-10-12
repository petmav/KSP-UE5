// WP-54 slice 2: VesselSituation.cs (the stand-in of slice 1 is replaced). See mission.hpp.
#include <algorithm>

#include "../save/save_util.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"

namespace ksp::Expansions::Missions {

using KSP::Localization::Localizer;

// VesselSituation.cs:57
VesselSituation::VesselSituation() {
    persistentId = 0;
    vesselCrew.clear();
    launched = false;
    readyToLaunch = false;
    focusonSpawn = false;
    requiredParts.clear();
}

// VesselSituation.cs:67 (partFilter.SetUpdatePartnerExcludedPartFilterCallback is the part picker's)
VesselSituation::VesselSituation(Mission* m, MENode* n) : VesselSituation() {
    persistentId = MissionDataEnvironment::UniquePersistentId();
    vesselName = Localizer::Format("#autoLOC_8100086");
    location = std::make_unique<VesselLocation>(n);
    mission = m;
    node = n;
    partFilter = std::make_unique<MissionPartFilter>(m, this);
    vesselRestrictionList = std::make_unique<VesselRestrictionList>(n);
}

VesselSituation::~VesselSituation() = default;

// VesselSituation.cs:80 (MissionEditorLogic.Instance.Unlock("missionBuilder_SetVesselCrew") is an editor input lock: UI)
void VesselSituation::SetVesselCrew(KerbalRoster& crewRoster, VesselCrewManifest& newCrew) {
    std::vector<std::string> list;
    for (size_t i = 0; i < vesselCrew.size(); ++i)
        for (size_t j = 0; j < vesselCrew[i].crewNames.size(); ++j) list.push_back(vesselCrew[i].crewNames[j]);
    std::vector<Crew> list2;
    auto& manifests = newCrew.PartManifests();
    for (size_t k = 0; k < manifests.size(); ++k) {
        PartCrewManifest& partCrewManifest = *manifests[k];
        if (partCrewManifest.partCrew.empty()) continue;
        Crew crew;
        crew.crewNames.clear();
        for (size_t l = 0; l < partCrewManifest.partCrew.size(); ++l) {
            if (partCrewManifest.partCrew[l] != "") {
                crew.crewNames.push_back(partCrewManifest.partCrew[l]);
                auto it = std::find(list.begin(), list.end(), partCrewManifest.partCrew[l]);  // List.Remove: first match
                if (it != list.end()) list.erase(it);
                std::shared_ptr<ProtoCrewMember> protoCrewMember = crewRoster[partCrewManifest.partCrew[l]];
                if (protoCrewMember != nullptr) {
                    protoCrewMember->seatIdx = partCrewManifest.GetCrewSeat(*protoCrewMember);
                    protoCrewMember->set_rosterStatus(ProtoCrewMember::RosterStatus::Assigned);
                }
            }
        }
        if (!crew.crewNames.empty()) {
            crew.partPersistentID = partCrewManifest.PartID();
            crew.vesselPersistentID = persistentId;
            list2.push_back(std::move(crew));
        }
    }
    for (size_t m = 0; m < list.size(); ++m) {
        std::shared_ptr<ProtoCrewMember> protoCrewMember2 = crewRoster[list[m]];
        if (protoCrewMember2 != nullptr) {
            protoCrewMember2->seatIdx = -1;
            protoCrewMember2->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
        }
    }
    vesselCrew = std::move(list2);
}

// VesselSituation.cs:137
void VesselSituation::SetCrewAvailable(KerbalRoster& crewRoster) {
    for (size_t i = 0; i < vesselCrew.size(); ++i) {
        for (size_t j = 0; j < vesselCrew[i].crewNames.size(); ++j) {
            const std::string& name = vesselCrew[i].crewNames[j];
            if (crewRoster.Exists(name)) crewRoster[name]->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
        }
    }
}

// VesselSituation.cs:152 (the Dictionary<string, List<string>> in insertion order)
std::vector<std::pair<std::string, std::vector<std::string>>> VesselSituation::GetRequiredExcludedParts() {
    return {{Localizer::Format("#autoLOC_8000261"), mission->situation->partFilter->GetExcludedParts()},
            {Localizer::Format("#autoLOC_8000262"), partFilter->GetExcludedParts()}};
}

// VesselSituation.cs:167 / :182
void VesselSituation::StartEditorEvents() { vesselRestrictionList->StartAppEvents(); }
void VesselSituation::ClearEditorEvents() { vesselRestrictionList->ClearAppEvents(); }

// VesselSituation.cs:207
bool VesselSituation::OnRequieredPartsCheckpointValidator(const std::vector<std::string>& value) {
    return MissionCheckpointValidator::CompareObjectLists(&requiredParts, &value);
}

// VesselSituation.cs:212
void VesselSituation::Load(const ConfigNode& node_) {
    const auto& values = node_.values();
    for (int i = 0; i < values.Count(); ++i) {
        const ConfigNode::Value& value = values[i];
        const std::string& n = value.name;
        if (n == "persistentId") {
            persistentId = save_detail::ParseUInt(value.value);
        } else if (n == "focusonSpawn") {
            focusonSpawn = save_detail::ParseBool(value.value);
        } else if (n == "ship") {
            vesselName = value.value;
        } else if (n == "autoGenerateCrew") {
            autoGenerateCrew = save_detail::ParseBool(value.value);
        } else if (n == "playerCreated") {
            playerCreated = save_detail::ParseBool(value.value);
        } else if (n == "vesselDescription") {
            vesselDescription = value.value;
        } else if (n == "craftFile") {
            craftFile = value.value;
        } else if (n == "launched") {
            launched = save_detail::ParseBool(value.value);
        } else if (n == "readyToLaunch") {
            readyToLaunch = save_detail::ParseBool(value.value);
        }
    }
    const auto& nodes = node_.nodes();
    for (int j = 0; j < nodes.Count(); ++j) {
        const ConfigNode* configNode = nodes[j];
        const std::string& name = configNode->name;
        if (name != "LOCATION") {
            if (name == "PARTCREW") {
                Crew crew;
                crew.Load(*configNode);
                vesselCrew.push_back(std::move(crew));
            }
        } else {
            if (location == nullptr) throw NullReferenceException();
            location->Load(*configNode);
        }
    }
    partFilter->Load(node_);
    requiredParts = node_.GetValuesList("requiredParts");
    MissionsUtils::UpdatePartNames(requiredParts);
    if (playerCreated) {
        vesselRestrictionList->Load(node_);
        if (HighLogic::CurrentGame() != nullptr && HighLogic::CurrentGame()->Mode == Game_Modes::MISSION && !requiredParts.empty()) {
            vesselRestrictionList->activeModules.push_back(std::make_unique<VesselRestriction_RequiredParts>());
        }
    }
}

// VesselSituation.cs:279
void VesselSituation::Save(ConfigNode& node_) {
    ConfigNode* configNode = node_.AddNode("VESSELSITUATION");
    configNode->AddValue("ship", vesselName);
    std::string text;
    for (char c : vesselDescription) {  // vesselDescription.Replace("\n", "\\n").Replace("\t", "\\t")
        if (c == '\n') {
            text += "\\n";
        } else if (c == '\t') {
            text += "\\t";
        } else {
            text += c;
        }
    }
    configNode->AddValue("vesselDescription", text);
    configNode->AddValue("playerCreated", playerCreated);
    configNode->AddValue("persistentId", persistentId);
    configNode->AddValue("craftFile", craftFile);
    configNode->AddValue("focusonSpawn", focusonSpawn);
    configNode->AddValue("launched", launched);
    configNode->AddValue("readyToLaunch", readyToLaunch);
    configNode->AddValue("autoGenerateCrew", autoGenerateCrew);
    location->Save(*configNode->AddNode("LOCATION"));
    for (size_t i = 0; i < vesselCrew.size(); ++i) vesselCrew[i].Save(*configNode->AddNode("PARTCREW"));
    if (playerCreated) vesselRestrictionList->Save(*configNode);
    partFilter->Save(*configNode);
    for (size_t j = 0; j < requiredParts.size(); ++j) configNode->AddValue("requiredParts", requiredParts[j]);
}

}  // namespace ksp::Expansions::Missions
