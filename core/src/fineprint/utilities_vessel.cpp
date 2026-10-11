// WP-38 slice 2: FinePrint/Utilities/VesselUtilities.cs (the members not in WP-37's fineprint_lite), SystemUtilities.cs (list / side-request helpers),
// ProgressUtilities.cs (tech checks, ExperimentPossibleAt, CurrentTrackRecord), the IContractObjectiveModule table and SurveyDefinition*.cs.
#include <algorithm>
#include <cmath>

#include "ksp/career/career.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/contracts.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/career/progress.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/science/science.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp::Contracts::Lite {

namespace M = ksp::dotnet::Math;
using KSP::Localization::Localizer;

namespace {
std::string DotName(std::string s) {
    for (char& c : s)
        if (c == '_') c = '.';
    return s;
}
std::vector<CelestialBody*>& Bodies() { return const_cast<std::vector<CelestialBody*>&>(FlightRegistry::Bodies()); }
CelestialBody* ReferenceBodyOf(Vessel* v) {
    return v->loaded ? v->orbit().referenceBody : Bodies().at(static_cast<size_t>(v->protoVessel->orbitSnapShot->ReferenceBodyIndex));
}
double DefD(const char* sec, const char* key) { return ContractDefsLite::D(sec, "", key); }
float DefF(const char* sec, const char* sub, const char* key) { return ContractDefsLite::F(sec, sub, key); }
Orbit OrbitOf(Vessel* v) { return v->loaded ? v->orbit() : v->protoVessel->orbitSnapShot->Load(FlightRegistry::Bodies()); }
int PartCountOf(Vessel* v) { return v->loaded ? static_cast<int>(v->Parts().size()) : static_cast<int>(v->protoVessel->protoPartSnapshots.size()); }
}  // namespace

// ------------------------------------------------------------------------------------------------------------------ SystemUtilities

namespace SystemUtilities {

// SystemUtilities.cs:181 (LoadNode<string> of the packed value, then UnpackDelimitedString)
void LoadNodeList(ConfigNode* node, const char* className, const char* valueName, std::vector<std::string>& list) {
    if (node != nullptr) {
        std::string value;
        LoadNode(node, className, valueName, value, std::string(), false);
        list = StringUtilities::UnpackDelimitedStrings(value);
    }
}
void LoadNodeList(ConfigNode* node, const char* className, const char* valueName, std::vector<int32_t>& list) {
    if (node != nullptr) {
        std::string value;
        LoadNode(node, className, valueName, value, std::string(), false);
        list = StringUtilities::UnpackDelimitedInt32(value);
    }
}
void LoadNodeList(ConfigNode* node, const char* className, const char* valueName, std::vector<uint32_t>& list) {
    if (node != nullptr) {
        std::string value;
        LoadNode(node, className, valueName, value, std::string(), false);
        list = StringUtilities::UnpackDelimitedUInt32(value);
    }
}
// SystemUtilities.cs:191
void SaveNodeList(ConfigNode* node, const char* listName, const std::vector<std::string>& list) {
    if (node != nullptr && !list.empty()) node->AddValue(listName, StringUtilities::PackDelimitedString(list));
}
void SaveNodeList(ConfigNode* node, const char* listName, const std::vector<int32_t>& list) {
    if (node != nullptr && !list.empty()) node->AddValue(listName, StringUtilities::PackDelimitedString(list));
}
void SaveNodeList(ConfigNode* node, const char* listName, const std::vector<uint32_t>& list) {
    if (node != nullptr && !list.empty()) node->AddValue(listName, StringUtilities::PackDelimitedString(list));
}

// SystemUtilities.cs:578
std::string RandomSplitChoice(std::vector<std::vector<std::string>*>& listOfLists, KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(ContractsEnvironment::Current().NewRandomSeed());
        generator = own.get();
    }
    int num = 0;
    int count = static_cast<int>(listOfLists.size());
    while (count-- > 0) {
        std::vector<std::string>* list = listOfLists[static_cast<size_t>(count)];
        if (list == nullptr)
            listOfLists.erase(listOfLists.begin() + count);
        else
            num += static_cast<int>(list->size());
    }
    if (listOfLists.empty()) return std::string();
    int num2 = generator->Next(num);
    int count2 = static_cast<int>(listOfLists.size());
    while (count2-- > 0) {
        const int count3 = static_cast<int>(listOfLists[static_cast<size_t>(count2)]->size());
        if (num2 >= count3) {
            num2 -= count3;
            continue;
        }
        num2 = count2;
        break;
    }
    std::vector<std::string>* list2 = listOfLists[static_cast<size_t>(num2)];
    return (*list2)[static_cast<size_t>(generator->Next(static_cast<int>(list2->size())))];
}

// SystemUtilities.cs:292
void ProcessSideRequests(Contract& contract, ConfigNode& contractNode, CelestialBody* targetBody, const std::string& vesselName, float& fundsMultiplier,
                         float& scienceMultiplier, float& reputationMultiplier, Vessel* vessel) {
    KSPRandom kSPRandom(contract.MissionSeed());
    int num;
    std::string name;
    switch (contract.Prestige()) {
        case Contract::ContractPrestige::Exceptional:
            num = kSPRandom.Next(2, 4);
            name = "Exceptional";
            break;
        default:
            num = kSPRandom.Next(0, 2);
            name = "Trivial";
            break;
        case Contract::ContractPrestige::Significant:
            num = kSPRandom.Next(1, 3);
            name = "Significant";
            break;
    }
    std::vector<ConfigNode*> nodes = contractNode.GetNodes();
    std::vector<std::pair<ConfigNode*, int>> list;
    int num2 = static_cast<int>(nodes.size());
    while (num2-- > 0) {
        ConfigNode* n = nodes[static_cast<size_t>(num2)];
        if (n->name != "PART_REQUEST" && n->name != "RESOURCE_REQUEST" && n->name != "CREW_REQUEST") continue;
        std::vector<std::string> list2 = n->GetValuesList("Part");
        std::vector<std::string> list3 = n->GetValuesList("Module");
        if (!ProgressUtilities::HaveAnyTech(&list2, &list3, false)) continue;
        if (n->name == "CREW_REQUEST") {
            std::string value = "Pilot";
            LoadNode(n, "ProcessSideRequests", "Trait", value, std::string("Pilot"), false);
            const int num3 = CrewTraitMissionAvailability(value, targetBody, vessel);
            const int num4 = vessel != nullptr ? VesselCrewWithTraitCount(value, vessel) : 0;
            const int num5 = static_cast<int>(contract.Prestige()) + 1;
            if (num5 > num3) continue;
            if (vessel != nullptr && list2.size() + list3.size() > 0) {
                int value2 = 1;
                LoadNode(n, "ProcessSideRequests", "Crew", value2, 1, false);
                int num6 = ActualCrewCapacity(vessel) / 2 / static_cast<int>(SaveEnvironment::Current().experienceConfigs->Categories().size());
                num6 += VesselPartAndModuleCount(list2, list3, vessel) * value2;
                if (num4 + num5 > num6) continue;
            }
        }
        int value3 = 1;
        LoadNode(n->GetNode(name), "ProcessSideRequests", "Weight", value3, 1, false);
        list.emplace_back(n, value3);
    }
    ShuffleList(list, &kSPRandom);
    int num7 = num;
    while (num7-- > 0 && !list.empty()) {
        ConfigNode* configNode = nullptr;
        int num8 = 0;
        int count = static_cast<int>(list.size());
        while (count-- > 0) num8 += list[static_cast<size_t>(count)].second;
        if (num8 <= 0) continue;
        int num9 = kSPRandom.Next(num8);
        int count2 = static_cast<int>(list.size());
        while (count2-- > 0) {
            num9 -= list[static_cast<size_t>(count2)].second;
            if (num9 <= 0) {
                configNode = list[static_cast<size_t>(count2)].first;
                list.erase(list.begin() + count2);
                break;
            }
        }
        if (configNode == nullptr) continue;
        float value4 = 1.0f, value5 = 1.0f, value6 = 1.0f, value7 = 0.0f, value8 = 0.0f, value9 = 0.0f;
        std::string value10;
        ConfigNode* node = configNode->GetNode(name);
        LoadNode(node, "ProcessSideRequests", "FundsMultiplier", value4, 1.0f, false);
        LoadNode(node, "ProcessSideRequests", "ScienceMultiplier", value5, 1.0f, false);
        LoadNode(node, "ProcessSideRequests", "ReputationMultiplier", value6, 1.0f, false);
        LoadNode(configNode, "ProcessSideRequests", "Keyword", value10, std::string(), false);
        LoadNode(configNode, "ProcessSideRequests", "MinimumFunds", value7, 0.0f, false);
        LoadNode(configNode, "ProcessSideRequests", "MinimumScience", value8, 0.0f, false);
        LoadNode(configNode, "ProcessSideRequests", "MinimumReputation", value9, 0.0f, false);
        fundsMultiplier = f32store(fx(fundsMultiplier) * fx(value4));
        scienceMultiplier = f32store(fx(scienceMultiplier) * fx(value5));
        reputationMultiplier = f32store(fx(reputationMultiplier) * fx(value6));
        contract.FundsCompletion = M::Max(contract.FundsCompletion, static_cast<double>(value7));
        contract.ScienceCompletion = M::Max(contract.ScienceCompletion, value8);
        contract.ReputationCompletion = M::Max(contract.ReputationCompletion, value9);
        if (!value10.empty() && std::find(contract.Keywords().begin(), contract.Keywords().end(), value10) == contract.Keywords().end())
            contract.Keywords().push_back(value10);
        if (configNode->name == "CREW_REQUEST") {
            std::string value14 = "Pilot";
            LoadNode(configNode, "ProcessSideRequests", "Trait", value14, std::string("Pilot"), false);
            int num10 = vessel != nullptr ? VesselCrewWithTraitCount(value14, vessel) : 0;
            num10 = num10 + (static_cast<int>(contract.Prestige()) + 1);
            contract.AddParameter(std::make_unique<ksp::Contracts::Parameters::CrewTraitParameter>(value14, num10, vesselName));
        } else if (configNode->name == "RESOURCE_REQUEST") {
            std::string value11 = "Matter";
            std::string value12 = "matter";
            float value13 = 1000.0f;
            LoadNode(configNode, "ProcessSideRequests", "Name", value11, std::string("Matter"), false);
            LoadNode(configNode, "ProcessSideRequests", "Title", value12, std::string("matter"), false);
            LoadNode(node, "ProcessSideRequests", "Amount", value13, 1000.0f, false);
            if (vessel != nullptr) value13 = f32store(fx(value13) + VesselResourceAmount(value11, vessel));
            contract.AddParameter(std::make_unique<ksp::Contracts::Parameters::ResourcePossessionParameter>(value11, value12, vesselName, static_cast<double>(value13)));
        } else if (configNode->name == "PART_REQUEST") {
            contract.AddParameter(std::make_unique<ksp::Contracts::Parameters::PartRequestParameter>(*configNode, vessel));
        }
    }
}

}  // namespace SystemUtilities

// ------------------------------------------------------------------------------------------------------------------ ProgressUtilities

namespace ProgressUtilities {

// ProgressUtilities.cs:11
bool HavePartTech(std::string partName, bool logging) {
    partName = DotName(partName);
    const AvailablePart* partInfoByName = SaveEnvironment::Current().getPartInfoByName ? SaveEnvironment::Current().getPartInfoByName(partName) : nullptr;
    if (partInfoByName != nullptr) {
        if (ResearchAndDevelopment::PartTechAvailable(partInfoByName)) return true;
    } else if (logging) {
        log::Warning("Contract Log: Attempted to check for nonexistent technology: \"" + partName + "\".");
    }
    return false;
}
// ProgressUtilities.cs:29
bool HaveModuleTech(const std::string& moduleName, const std::string* excludeModule) {
    PartLoader* loader = PartLoader::Instance();
    if (loader == nullptr) return false;
    int count = static_cast<int>(loader->loadedParts.size());
    while (true) {
        if (count-- > 0) {
            AvailablePart* availablePart = loader->loadedParts[static_cast<size_t>(count)].get();
            if (availablePart->partPrefab == nullptr || !ResearchAndDevelopment::PartTechAvailable(availablePart)) continue;
            bool flag = false;
            bool flag2 = false;
            int count2 = static_cast<int>(availablePart->partPrefab->Modules().Count());
            while (count2-- > 0) {
                PartModule* partModule = availablePart->partPrefab->Modules()[static_cast<size_t>(count2)];
                if (partModule->moduleName == moduleName) flag = true;
                if (excludeModule != nullptr && partModule->moduleName == *excludeModule) flag2 = true;
            }
            if (flag && !flag2) break;
            continue;
        }
        return false;
    }
    return true;
}
// ProgressUtilities.cs:67
bool HaveAnyTech(const std::vector<std::string>* partNames, const std::vector<std::string>* moduleNames, bool logging) {
    if (partNames != nullptr) {
        int count = static_cast<int>(partNames->size());
        while (count-- > 0)
            if (HavePartTech((*partNames)[static_cast<size_t>(count)], logging)) return true;
    }
    if (moduleNames != nullptr) {
        int count2 = static_cast<int>(moduleNames->size());
        while (count2-- > 0)
            if (HaveModuleTech((*moduleNames)[static_cast<size_t>(count2)])) return true;
    }
    return false;
}
// ProgressUtilities.cs:559
bool ExperimentPossibleAt(const std::string* experimentID, CelestialBody* body, double latitude, double longitude, double altitude, double terrainHeight) {
    (void)latitude;
    (void)longitude;
    if (experimentID == nullptr) return false;
    ScienceExperiment* experiment = ResearchAndDevelopment::GetExperiment(*experimentID);
    if (experiment == nullptr) return false;
    ExperimentSituations situation;
    if (terrainHeight < 0.0 && body->ocean && altitude <= 0.0)
        situation = ExperimentSituations::SrfSplashed;
    else if (altitude <= terrainHeight)
        situation = ExperimentSituations::SrfLanded;
    else if (!body->atmosphere || !(altitude < body->atmosphereDepth))
        situation = altitude < static_cast<double>(body->scienceValues.spaceAltitudeThreshold) ? ExperimentSituations::InSpaceLow : ExperimentSituations::InSpaceHigh;
    else
        situation = altitude < static_cast<double>(body->scienceValues.flyingAltitudeThreshold) ? ExperimentSituations::FlyingLow : ExperimentSituations::FlyingHigh;
    return experiment->IsAvailableWhile(situation, *body);
}
// ProgressUtilities.cs:273
double CurrentTrackRecord(RecordTrackType type) {
    ProgressTracking* pt = ProgressTracking::Instance();
    if (pt == nullptr) return 0.0;
    switch (type) {
        case RecordTrackType::ALTITUDE:
            if (auto* r = dynamic_cast<KSPAchievements::RecordsAltitude*>(pt->FindNode({"RecordsAltitude"}))) return r->record;
            break;
        case RecordTrackType::DEPTH:
            if (auto* r = dynamic_cast<KSPAchievements::RecordsDepth*>(pt->FindNode({"RecordsDepth"}))) return r->record;
            break;
        case RecordTrackType::DISTANCE:
            if (auto* r = dynamic_cast<KSPAchievements::RecordsDistance*>(pt->FindNode({"RecordsDistance"}))) return r->record;
            break;
        case RecordTrackType::SPEED:
            if (auto* r = dynamic_cast<KSPAchievements::RecordsSpeed*>(pt->FindNode({"RecordsSpeed"}))) return r->record;
            break;
        default: break;
    }
    return 0.0;
}

}  // namespace ProgressUtilities

// ------------------------------------------------------------------------------------------------------------------ contract objectives

// PartModule.cs:964 IsValidContractObjective (the six IContractObjectiveModule implementations)
bool IsValidContractObjective(PartModule& module, const std::string& objectiveType) {
    std::string type;
    bool valid = false;
    if (module.IsA("ModuleDataTransmitter")) {
        type = "Antenna";  // ModuleDataTransmitter.cs:696
        valid = module.EnumName("antennaType") != "INTERNAL";  // :701
    } else if (module.IsA("ModuleDeployableSolarPanel")) {
        type = "Generator";  // ModuleDeployableSolarPanel.cs:336
        valid = true;        // :341
    } else if (module.IsA("ModuleDockingNode")) {
        type = "Dock";  // ModuleDockingNode.cs:2001
        valid = true;   // :2006
    } else if (module.IsA("ModuleGenerator")) {
        type = "Generator";  // ModuleGenerator.cs:224
        valid = !module.Get<bool>("isGroundFixture");  // :229
    } else if (module.IsA("ModuleGrappleNode")) {
        type = "Grapple";  // ModuleGrappleNode.cs:918
        valid = true;      // :923
    } else if (module.IsA("ModuleWheelBase")) {
        type = "Wheel";  // ModuleWheelBase.cs:1116
        valid = module.EnumName("wheelType") == "MOTORIZED";  // :1121
    } else {
        return false;
    }
    return !(type != objectiveType) && valid;
}
// Part.cs:8716
bool HasValidContractObjective(Part& part, const std::string& objectiveType) {
    int count = static_cast<int>(part.Modules().Count());
    do {
        if (count-- <= 0) return false;
    } while (!IsValidContractObjective(*part.Modules()[static_cast<size_t>(count)], objectiveType));
    return true;
}
// Vessel.cs:4313
bool HasValidContractObjectives(Vessel& vessel, std::vector<std::string> objectiveTypes, bool copy) {
    (void)copy;
    if (!objectiveTypes.empty()) {
        std::vector<std::string>& list = objectiveTypes;
        if (vessel.loaded) {
            int count = static_cast<int>(vessel.Parts().size());
            while (count-- > 0) {
                for (int num = static_cast<int>(list.size()) - 1; num >= 0; num--) {
                    if (HasValidContractObjective(*vessel.Parts()[static_cast<size_t>(count)], list[static_cast<size_t>(num)])) {
                        list.erase(list.begin() + num);
                        if (list.empty()) return true;
                    }
                }
            }
        } else {
            int count2 = static_cast<int>(vessel.protoVessel->protoPartSnapshots.size());
            while (count2-- > 0) {
                const AvailablePart* info = vessel.protoVessel->protoPartSnapshots[static_cast<size_t>(count2)]->partInfo;
                Part* partPrefab = info != nullptr ? info->partPrefab.get() : nullptr;
                if (partPrefab == nullptr) continue;
                for (int num2 = static_cast<int>(list.size()) - 1; num2 >= 0; num2--) {
                    if (HasValidContractObjective(*partPrefab, list[static_cast<size_t>(num2)])) {
                        list.erase(list.begin() + num2);
                        if (list.empty()) return true;
                    }
                }
            }
        }
        return false;
    }
    return false;
}
// ResearchAndDevelopment.cs:1177
bool ResearchedValidContractObjectives(std::vector<std::string> objectiveTypes, bool copy) {
    (void)copy;
    if (!objectiveTypes.empty()) {
        std::vector<std::string>& list = objectiveTypes;
        PartLoader* loader = PartLoader::Instance();
        if (loader == nullptr) return false;
        int count = static_cast<int>(loader->loadedParts.size());
        while (count-- > 0) {
            AvailablePart* availablePart = loader->loadedParts[static_cast<size_t>(count)].get();
            if (availablePart->partPrefab == nullptr || !ResearchAndDevelopment::PartTechAvailable(availablePart)) continue;
            int count2 = static_cast<int>(availablePart->partPrefab->Modules().Count());
            while (count2-- > 0) {
                for (int num = static_cast<int>(list.size()) - 1; num >= 0; num--) {
                    if (IsValidContractObjective(*availablePart->partPrefab->Modules()[static_cast<size_t>(count2)], list[static_cast<size_t>(num)])) {
                        list.erase(list.begin() + num);
                        if (list.empty()) return true;
                    }
                }
            }
        }
        return false;
    }
    return false;
}

// ------------------------------------------------------------------------------------------------------------------ VesselUtilities

namespace {
int highestPodCapacity = INT32_MIN;
}

// VesselUtilities.cs:15 HighestPodCapacity
int HighestPodCapacity() {
    if (highestPodCapacity != INT32_MIN) return highestPodCapacity;
    PartLoader* loader = PartLoader::Instance();
    if (loader == nullptr) return highestPodCapacity;
    int count = static_cast<int>(loader->loadedParts.size());
    while (count-- > 0) {
        Part* partPrefab = loader->loadedParts[static_cast<size_t>(count)]->partPrefab.get();
        if (partPrefab != nullptr && static_cast<int>(partPrefab->fields.isControlSource) > static_cast<int>(ControlLevel::NONE))
            highestPodCapacity = partPrefab->fields.CrewCapacity > highestPodCapacity ? partPrefab->fields.CrewCapacity : highestPodCapacity;
    }
    return highestPodCapacity;
}

// VesselUtilities.cs:96
bool VesselHasModuleName(const std::string& moduleName, Vessel* v) {
    if (!ActiveVesselFallback(v)) return false;
    if (v->loaded) {
        int count = static_cast<int>(v->Parts().size());
        while (count-- > 0) {
            int count2 = static_cast<int>(v->Parts()[static_cast<size_t>(count)]->Modules().Count());
            while (count2-- > 0)
                if (v->Parts()[static_cast<size_t>(count)]->Modules()[static_cast<size_t>(count2)]->moduleName == moduleName) return true;
        }
    } else {
        int count3 = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
        while (count3-- > 0) {
            int count4 = static_cast<int>(v->protoVessel->protoPartSnapshots[static_cast<size_t>(count3)]->modules.size());
            while (count4-- > 0)
                if (v->protoVessel->protoPartSnapshots[static_cast<size_t>(count3)]->modules[static_cast<size_t>(count4)]->moduleName.value_or("") == moduleName) return true;
        }
    }
    return false;
}
// VesselUtilities.cs:135
double VesselResourceAmount(const std::string& resourceName, Vessel* v) {
    double num = 0.0;
    if (!ActiveVesselFallback(v)) return num;
    if (v->loaded) {
        int count = static_cast<int>(v->Parts().size());
        while (count-- > 0) {
            PartResource* partResource = v->Parts()[static_cast<size_t>(count)]->ResourceList().Get(resourceName);
            if (partResource != nullptr) num += partResource->amount;
        }
    } else {
        int count2 = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
        while (count2-- > 0) {
            ProtoPartSnapshot* pps = v->protoVessel->protoPartSnapshots[static_cast<size_t>(count2)].get();
            for (int num2 = static_cast<int>(pps->resources.size()) - 1; num2 >= 0; num2--) {
                if (!(pps->resources[static_cast<size_t>(num2)]->resourceName.value_or("") != resourceName)) num += pps->resources[static_cast<size_t>(num2)]->amount;
            }
        }
    }
    return num;
}
// VesselUtilities.cs:171
bool VesselHasAnyParts(const std::vector<std::string>* partList, Vessel* v) {
    if (!ActiveVesselFallback(v)) return false;
    if (partList != nullptr && !partList->empty()) {
        if (v->loaded) {
            int count = static_cast<int>(v->Parts().size());
            while (count-- > 0) {
                int count2 = static_cast<int>(partList->size());
                while (count2-- > 0) {
                    const std::string text = DotName((*partList)[static_cast<size_t>(count2)]);
                    if (GetPartName(v->Parts()[static_cast<size_t>(count)]) == text) return true;
                }
            }
        } else {
            int count3 = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
            while (count3-- > 0) {
                int count4 = static_cast<int>(partList->size());
                while (count4-- > 0) {
                    const std::string text2 = DotName((*partList)[static_cast<size_t>(count4)]);
                    if (v->protoVessel->protoPartSnapshots[static_cast<size_t>(count3)]->partName.value_or("") == text2) return true;
                }
            }
        }
        return false;
    }
    return false;
}
// VesselUtilities.cs:216
bool VesselHasAnyModules(const std::vector<std::string>* moduleList, Vessel* v) {
    if (!ActiveVesselFallback(v)) return false;
    if (moduleList != nullptr && !moduleList->empty()) {
        if (v->loaded) {
            int count = static_cast<int>(v->Parts().size());
            while (count-- > 0) {
                int count2 = static_cast<int>(v->Parts()[static_cast<size_t>(count)]->Modules().Count());
                while (count2-- > 0) {
                    int count3 = static_cast<int>(moduleList->size());
                    while (count3-- > 0)
                        if (v->Parts()[static_cast<size_t>(count)]->Modules()[static_cast<size_t>(count2)]->moduleName == (*moduleList)[static_cast<size_t>(count3)]) return true;
                }
            }
        } else {
            int count4 = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
            while (count4-- > 0) {
                int count5 = static_cast<int>(v->protoVessel->protoPartSnapshots[static_cast<size_t>(count4)]->modules.size());
                while (count5-- > 0) {
                    int count6 = static_cast<int>(moduleList->size());
                    while (count6-- > 0)
                        if (v->protoVessel->protoPartSnapshots[static_cast<size_t>(count4)]->modules[static_cast<size_t>(count5)]->moduleName.value_or("") ==
                            (*moduleList)[static_cast<size_t>(count6)])
                            return true;
                }
            }
        }
        return false;
    }
    return false;
}
// VesselUtilities.cs:267
bool VesselHasAnyPartsOrModules(const std::vector<std::string>* partList, const std::vector<std::string>* moduleList, Vessel* v) {
    if (!ActiveVesselFallback(v)) return false;
    const bool flag = partList == nullptr || partList->empty();
    const bool flag2 = moduleList == nullptr || moduleList->empty();
    if (flag && flag2) return false;
    if (flag2) return VesselHasAnyParts(partList, v);
    if (flag) return VesselHasAnyModules(moduleList, v);
    if (v->loaded) {
        int count = static_cast<int>(v->Parts().size());
        while (count-- > 0) {
            int count2 = static_cast<int>(partList->size());
            while (count2-- > 0) {
                const std::string text = DotName((*partList)[static_cast<size_t>(count2)]);
                if (GetPartName(v->Parts()[static_cast<size_t>(count)]) == text) return true;
            }
            int count3 = static_cast<int>(v->Parts()[static_cast<size_t>(count)]->Modules().Count());
            while (count3-- > 0) {
                int count4 = static_cast<int>(moduleList->size());
                while (count4-- > 0)
                    if (v->Parts()[static_cast<size_t>(count)]->Modules()[static_cast<size_t>(count3)]->moduleName == (*moduleList)[static_cast<size_t>(count4)]) return true;
            }
        }
    } else {
        int count5 = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
        while (count5-- > 0) {
            ProtoPartSnapshot* pps = v->protoVessel->protoPartSnapshots[static_cast<size_t>(count5)].get();
            int count6 = static_cast<int>(partList->size());
            while (count6-- > 0) {
                const std::string text2 = DotName((*partList)[static_cast<size_t>(count6)]);
                if (pps->partName.value_or("") == text2) return true;
            }
            int count7 = static_cast<int>(pps->modules.size());
            while (count7-- > 0) {
                int count8 = static_cast<int>(moduleList->size());
                while (count8-- > 0)
                    if (pps->modules[static_cast<size_t>(count7)]->moduleName.value_or("") == (*moduleList)[static_cast<size_t>(count8)]) return true;
            }
        }
    }
    return false;
}
// VesselUtilities.cs:346
int VesselPartAndModuleCount(const std::vector<std::string>& partList, const std::vector<std::string>& moduleList, Vessel* v) {
    if (!ActiveVesselFallback(v)) return 0;
    int num = 0;
    if (v->loaded) {
        int count = static_cast<int>(v->Parts().size());
        while (count-- > 0) {
            int count2 = static_cast<int>(partList.size());
            while (count2-- > 0) {
                const std::string text = DotName(partList[static_cast<size_t>(count2)]);
                if (GetPartName(v->Parts()[static_cast<size_t>(count)]) == text) num++;
            }
            int count3 = static_cast<int>(v->Parts()[static_cast<size_t>(count)]->Modules().Count());
            while (count3-- > 0) {
                int count4 = static_cast<int>(moduleList.size());
                while (count4-- > 0) {
                    if (v->Parts()[static_cast<size_t>(count)]->Modules()[static_cast<size_t>(count3)]->moduleName == moduleList[static_cast<size_t>(count4)]) {
                        num++;
                        break;
                    }
                }
            }
        }
    } else {
        int count5 = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
        while (count5-- > 0) {
            ProtoPartSnapshot* pps = v->protoVessel->protoPartSnapshots[static_cast<size_t>(count5)].get();
            int count6 = static_cast<int>(partList.size());
            while (count6-- > 0) {
                const std::string text2 = DotName(partList[static_cast<size_t>(count6)]);
                if (pps->partName.value_or("") == text2) num++;
            }
            int count7 = static_cast<int>(pps->modules.size());
            while (count7-- > 0) {
                int count8 = static_cast<int>(moduleList.size());
                while (count8-- > 0) {
                    if (pps->modules[static_cast<size_t>(count7)]->moduleName.value_or("") == moduleList[static_cast<size_t>(count8)]) {
                        num++;
                        break;
                    }
                }
            }
        }
    }
    return num;
}
// VesselUtilities.cs:414
std::vector<uint32_t> GetPartIDList(Vessel* v) {
    std::vector<uint32_t> list;
    if (!ActiveVesselFallback(v)) return list;
    if (v->loaded) {
        int count = static_cast<int>(v->Parts().size());
        while (count-- > 0) list.push_back(v->Parts()[static_cast<size_t>(count)]->fields.flightID);
    } else {
        int count2 = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
        while (count2-- > 0) list.push_back(v->protoVessel->protoPartSnapshots[static_cast<size_t>(count2)]->flightID);
    }
    return list;
}
// VesselUtilities.cs:440
Vessel* FindVesselWithPartIDs(const std::vector<uint32_t>* partIDs) {
    if (partIDs != nullptr && !partIDs->empty()) {
        int num = 0;
        Vessel* result = nullptr;
        int count = static_cast<int>(FlightRegistry::Vessels.size());
        auto contains = [&](uint32_t id) { return std::find(partIDs->begin(), partIDs->end(), id) != partIDs->end(); };
        while (count-- > 0) {
            Vessel* vessel = FlightRegistry::Vessels[static_cast<size_t>(count)];
            int num2 = 0;
            if (vessel->loaded) {
                int count2 = static_cast<int>(vessel->Parts().size());
                while (count2-- > 0)
                    if (contains(vessel->Parts()[static_cast<size_t>(count2)]->fields.flightID)) num2++;
            } else {
                int count3 = static_cast<int>(vessel->protoVessel->protoPartSnapshots.size());
                while (count3-- > 0)
                    if (contains(vessel->protoVessel->protoPartSnapshots[static_cast<size_t>(count3)]->flightID)) num2++;
            }
            if (num2 > num) {
                num = num2;
                result = vessel;
            }
        }
        return result;
    }
    return nullptr;
}
// VesselUtilities.cs:484
VesselType ClassifyVesselType(Vessel* v, bool useVesselType) {
    if (!ActiveVesselFallback(v)) return VesselType::Unknown;
    int count;
    int num;
    Vessel::Situations situation;
    VesselType vesselType;
    if (v->loaded) {
        count = static_cast<int>(v->Parts().size());
        num = v->GetCrewCount();
        situation = v->situation;
        vesselType = v->vesselType;
    } else {
        count = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
        num = static_cast<int>(v->protoVessel->GetVesselCrew().size());
        situation = v->protoVessel->situation;
        vesselType = v->protoVessel->vesselType;
    }
    if (count > 1 && VesselHasModuleName("ModuleCommand", v)) {
        const int num2 = ActualCrewCapacity(v);
        if (num2 <= HighestPodCapacity() && VesselHasWheelsOnGround(v, {WheelType::MOTORIZED})) return VesselType::Rover;
        if (HasValidContractObjectives(*v, {"Generator", "Antenna"})) {
            if (num2 > HighestPodCapacity() && HasValidContractObjectives(*v, {"Dock"})) {
                switch (situation) {
                    case Vessel::Situations::ORBITING: return VesselType::Station;
                    case Vessel::Situations::LANDED: return VesselType::Base;
                    default: break;
                }
            }
            if (num + num2 <= 0) return VesselType::Probe;
        }
    }
    if (!useVesselType) return VesselType::Unknown;
    return vesselType;
}
// VesselUtilities.cs:540
std::vector<std::pair<Vessel*, VesselType>> ClassifyAllVesselsAt(CelestialBody* body, bool useVesselType) {
    std::vector<std::pair<Vessel*, VesselType>> dictionary;
    if (body == nullptr) return dictionary;
    int count = static_cast<int>(FlightRegistry::Vessels.size());
    while (count-- > 0) {
        Vessel* vessel = FlightRegistry::Vessels[static_cast<size_t>(count)];
        if (ReferenceBodyOf(vessel) == body) dictionary.emplace_back(vessel, ClassifyVesselType(vessel, useVesselType));
    }
    return dictionary;
}
// VesselUtilities.cs:559
std::vector<Vessel*> SpecificVesselClassAt(CelestialBody* body, VesselType vesselType, bool requireOwned, bool excludeActive, bool useVesselType) {
    std::vector<Vessel*> list;
    if (body == nullptr) return list;
    int count = static_cast<int>(FlightRegistry::Vessels.size());
    while (count-- > 0) {
        Vessel* vessel = FlightRegistry::Vessels[static_cast<size_t>(count)];
        if ((!excludeActive || SaveEnvironment::Current().loadedScene != GameScenes::FLIGHT || !FlightRegistry::ready || vessel != FlightRegistry::ActiveVessel) &&
            !(ReferenceBodyOf(vessel) != body) && (!requireOwned || VesselIsOwned(vessel)) && ClassifyVesselType(vessel, useVesselType) == vesselType)
            list.push_back(vessel);
    }
    return list;
}
// VesselUtilities.cs:578
std::vector<Vessel*> SpecificVesselClassAt(Vessel::Situations situation, CelestialBody* body, VesselType vesselType, bool requireOwned, bool excludeActive,
                                           bool useVesselType) {
    std::vector<Vessel*> list;
    if (body == nullptr) return list;
    int count = static_cast<int>(FlightRegistry::Vessels.size());
    while (count-- > 0) {
        Vessel* vessel = FlightRegistry::Vessels[static_cast<size_t>(count)];
        if ((!excludeActive || SaveEnvironment::Current().loadedScene != GameScenes::FLIGHT || !FlightRegistry::ready || vessel != FlightRegistry::ActiveVessel) &&
            (vessel->loaded ? vessel->situation : vessel->protoVessel->situation) == situation && !(ReferenceBodyOf(vessel) != body) &&
            (!requireOwned || VesselIsOwned(vessel)) && ClassifyVesselType(vessel, useVesselType) == vesselType)
            list.push_back(vessel);
    }
    return list;
}
// VesselUtilities.cs:597
bool VesselHasWheelsOnGround(Vessel* v, const std::vector<WheelType>& validWheelTypes) {
    if (!ActiveVesselFallback(v)) return false;
    const Vessel::Situations sit = v->loaded ? v->situation : v->protoVessel->situation;
    if (sit != Vessel::Situations::LANDED && sit != Vessel::Situations::PRELAUNCH) return false;
    static const char* wheelNames[3] = {"FREE", "MOTORIZED", "LEG"};
    if (v->loaded) {
        int count = static_cast<int>(v->Parts().size());
        while (count-- > 0) {
            int count2 = static_cast<int>(v->Parts()[static_cast<size_t>(count)]->Modules().Count());
            while (count2-- > 0) {
                PartModule* moduleWheelBase = v->Parts()[static_cast<size_t>(count)]->Modules()[static_cast<size_t>(count2)];
                if (!moduleWheelBase->IsA("ModuleWheelBase") || !moduleWheelBase->Get<bool>("isGrounded")) continue;
                int num2 = static_cast<int>(validWheelTypes.size());
                while (num2-- > 0)
                    if (moduleWheelBase->EnumName("wheelType") == wheelNames[static_cast<int>(validWheelTypes[static_cast<size_t>(num2)])]) return true;
            }
        }
    } else {
        int count3 = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
        while (count3-- > 0) {
            int count4 = static_cast<int>(v->protoVessel->protoPartSnapshots[static_cast<size_t>(count3)]->modules.size());
            while (count4-- > 0) {
                ProtoPartModuleSnapshot* pm = v->protoVessel->protoPartSnapshots[static_cast<size_t>(count3)]->modules[static_cast<size_t>(count4)].get();
                if (pm->moduleName.value_or("") != "ModuleWheelBase") continue;
                bool result = false;
                if (pm->moduleValues->HasValue("isGrounded")) dotnet::TryParseBool(*pm->moduleValues->GetValue("isGrounded"), result);
                if (!result || !pm->moduleValues->HasValue("wheelType")) continue;
                const std::string value = *pm->moduleValues->GetValue("wheelType");
                int num3 = static_cast<int>(validWheelTypes.size());
                while (num3-- > 0)
                    if (value == wheelNames[static_cast<int>(validWheelTypes[static_cast<size_t>(num3)])]) return true;
            }
        }
    }
    return false;
}
// VesselUtilities.cs:686
FlightBand GetFlightBand(double threshold, Vessel* v) {
    if (!ActiveVesselFallback(v)) return FlightBand::NONE;
    if (v->loaded) {
        if (v->LandedOrSplashed()) return FlightBand::GROUND;
        if (v->altitude > threshold) return FlightBand::HIGH;
        if (v->altitude <= threshold) return FlightBand::LOW;
    } else {
        if (v->protoVessel->situation == Vessel::Situations::LANDED || v->protoVessel->situation == Vessel::Situations::SPLASHED) return FlightBand::GROUND;
        Orbit orbit = v->protoVessel->orbitSnapShot->Load(FlightRegistry::Bodies());
        orbit.UpdateFromUT(Planetarium::GetUniversalTime());
        if (orbit.altitude > threshold) return FlightBand::HIGH;
        if (orbit.altitude <= threshold) return FlightBand::LOW;
    }
    return FlightBand::NONE;
}
// VesselUtilities.cs:727
int ActualCrewCapacity(Vessel* v) {
    if (!ActiveVesselFallback(v)) return 0;
    if (v->loaded) return v->GetCrewCapacity();
    int num = 0;
    int count = static_cast<int>(v->protoVessel->protoPartSnapshots.size());
    while (count-- > 0) {
        const AvailablePart* info = v->protoVessel->protoPartSnapshots[static_cast<size_t>(count)]->partInfo;
        if (info != nullptr) {
            Part* partPrefab = info->partPrefab.get();
            if (partPrefab != nullptr) num += partPrefab->fields.CrewCapacity;
        }
    }
    return num;
}
// VesselUtilities.cs:753
bool VesselLaunchedAfterID(uint32_t launchID, Vessel* v, std::vector<std::string> ignore) {
    if (v == nullptr) return false;
    int num = static_cast<int>(ignore.size());
    while (num-- > 0) ignore[static_cast<size_t>(num)] = DotName(ignore[static_cast<size_t>(num)]);
    if (v->loaded) {
        std::vector<Part*>& parts = v->Parts();
        if (parts.empty()) return false;
        int count = static_cast<int>(parts.size());
        while (count-- > 0) {
            Part* part = parts[static_cast<size_t>(count)];
            bool flag = false;
            int num2 = static_cast<int>(ignore.size());
            while (num2-- > 0) {
                if (!(GetPartName(part) != ignore[static_cast<size_t>(num2)])) {
                    flag = true;
                    break;
                }
            }
            if (!flag && part->fields.launchID < launchID) return false;
        }
    } else {
        auto& protoPartSnapshots = v->protoVessel->protoPartSnapshots;
        if (protoPartSnapshots.empty()) return false;
        int count2 = static_cast<int>(protoPartSnapshots.size());
        while (count2-- > 0) {
            ProtoPartSnapshot* pps = protoPartSnapshots[static_cast<size_t>(count2)].get();
            bool flag2 = false;
            int num3 = static_cast<int>(ignore.size());
            while (num3-- > 0) {
                if (!(pps->partName.value_or("") != ignore[static_cast<size_t>(num3)])) {
                    flag2 = true;
                    break;
                }
            }
            if (!flag2 && pps->launchID < launchID) return false;
        }
    }
    return true;
}
// VesselUtilities.cs:821
bool VesselIsOwned(Vessel* v) {
    if (!ActiveVesselFallback(v)) return false;
    if (v->loaded) return v->GetDiscoveryInfo().Level() == DiscoveryLevels::Owned;
    ProtoVessel* protoVessel = v->protoVessel;
    DiscoveryLevels discoveryLevels = DiscoveryLevels::None;
    if (protoVessel->discoveryInfo->HasValue("state")) discoveryLevels = static_cast<DiscoveryLevels>(dotnet::ParseInt32(*protoVessel->discoveryInfo->GetValue("state")));
    return discoveryLevels == DiscoveryLevels::Owned;
}
// VesselUtilities.cs:840
void DiscoverVessel(Vessel* v) {
    if (!ActiveVesselFallback(v)) return;
    if (v->loaded) {
        if (v->GetDiscoveryInfo().Level() != DiscoveryLevels::Owned) v->GetDiscoveryInfo().SetLevel(DiscoveryLevels::Owned);
        return;
    }
    ProtoVessel* protoVessel = v->protoVessel;
    DiscoveryLevels discoveryLevels = DiscoveryLevels::None;
    if (protoVessel->discoveryInfo->HasValue("state")) discoveryLevels = static_cast<DiscoveryLevels>(dotnet::ParseInt32(*protoVessel->discoveryInfo->GetValue("state")));
    if (discoveryLevels != DiscoveryLevels::Owned) protoVessel->discoveryInfo->SetValue("state", "-1", true);
}
// VesselUtilities.cs:866
uint32_t VesselID(Vessel* v) {
    if (!ActiveVesselFallback(v)) return 0u;
    if (v->loaded) return v->rootPart->fields.flightID;
    if (v->protoVessel->protoPartSnapshots.size() < 0) return 0u;
    return v->protoVessel->protoPartSnapshots[0]->flightID;
}
// VesselUtilities.cs:883
std::vector<std::shared_ptr<ProtoCrewMember>> VesselCrewWithTrait(const std::string& trait, Vessel* v) {
    if (!ActiveVesselFallback(v)) return {};
    std::vector<std::shared_ptr<ProtoCrewMember>> list(v->GetVesselCrew());
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        if (list[static_cast<size_t>(count)]->experienceTrait->TypeName() != trait) {
            // List.Remove(list[count]) removes the first element equal to it
            const auto target = list[static_cast<size_t>(count)];
            list.erase(std::find(list.begin(), list.end(), target));
        }
    }
    return list;
}
// VesselUtilities.cs:901
int VesselCrewWithTraitCount(const std::string& trait, Vessel* v) {
    if (!ActiveVesselFallback(v)) return 0;
    int num = 0;
    const auto vesselCrew = v->GetVesselCrew();
    int count = static_cast<int>(vesselCrew.size());
    while (count-- > 0)
        if (vesselCrew[static_cast<size_t>(count)]->experienceTrait->TypeName() == trait) num++;
    return num;
}
// VesselUtilities.cs:920
int CrewTraitMissionAvailability(const std::string& trait, CelestialBody* targetBody, Vessel* excludeVessel) {
    int num = 0;
    Game* game = SaveEnvironment::Current().currentGame;
    if (trait == "Tourist") {
        if (targetBody == nullptr) {
            num = static_cast<int>(game->CrewRoster->Kerbals(ProtoCrewMember_KerbalType::Tourist).size());
            return num;
        }
        auto currentContracts = ksp::Contracts::ContractSystem::Instance->GetCurrentContracts<ksp::FinePrint::Contracts::TourismContract>();
        int num2 = static_cast<int>(currentContracts.size());
        while (num2-- > 0) {
            std::vector<ContractParameter*> all;
            currentContracts[static_cast<size_t>(num2)]->AllParameters(all);
            for (ContractParameter* p : all) {
                auto* tour = dynamic_cast<ksp::Contracts::Parameters::KerbalTourParameter*>(p);
                if (tour == nullptr) continue;
                std::vector<ContractParameter*> sub;
                tour->AllParameters(sub);
                for (ContractParameter* q : sub) {
                    auto* dest = dynamic_cast<ksp::Contracts::Parameters::KerbalDestinationParameter*>(q);
                    if (dest != nullptr && CelestialUtilities::GetHostPlanet(dest->targetBody) == targetBody) {
                        num++;
                        break;
                    }
                }
            }
        }
        return num;
    }
    for (const auto& k : game->CrewRoster->Kerbals(ProtoCrewMember_KerbalType::Crew))
        if (k != nullptr && k->experienceTrait->TypeName() == trait) num++;
    if (targetBody == nullptr) return num;
    CelestialBody* hostPlanet = CelestialUtilities::GetHostPlanet(targetBody);
    int count = static_cast<int>(FlightRegistry::Vessels.size());
    while (count-- > 0) {
        Vessel* vessel = FlightRegistry::Vessels[static_cast<size_t>(count)];
        if (vessel == nullptr || vessel == excludeVessel) continue;
        const auto vesselCrew = vessel->GetVesselCrew();
        if (vesselCrew.empty()) continue;
        CelestialBody* hostPlanet2 = CelestialUtilities::GetHostPlanet(vessel->loaded ? vessel->mainBody() : ReferenceBodyOf(vessel));
        if (hostPlanet != hostPlanet2) continue;
        int count2 = static_cast<int>(vesselCrew.size());
        while (count2-- > 0) {
            const auto& c = vesselCrew[static_cast<size_t>(count2)];
            if (c->type() == ProtoCrewMember_KerbalType::Crew && c->rosterStatus() == ProtoCrewMember_RosterStatus::Assigned && c->experienceTrait->TypeName() == trait) num++;
        }
    }
    return num;
}
// VesselUtilities.cs:1001
Orbit GenerateAdjustedVesselOrbit(double minimumDeviation, double modificationLength, int numberOfModifications, KSPRandom* generator, Vessel* v) {
    if (!ActiveVesselFallback(v)) return Orbit();
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(ContractsEnvironment::Current().NewRandomSeed());
        generator = own.get();
    }
    KSPRandom* kSPRandom = generator;
    Orbit orbit = OrbitOf(v);
    if (!(orbit.eccentricity >= 1.0) && orbit.ApR() < orbit.referenceBody->sphereOfInfluence) {
        Orbit orbit2(orbit.inclination, orbit.eccentricity, orbit.semiMajorAxis, orbit.LAN, orbit.argumentOfPeriapsis, orbit.meanAnomalyAtEpoch, orbit.epoch,
                     orbit.referenceBody);
        std::vector<OrbitalProperties> list{OrbitalProperties::INC, OrbitalProperties::PER, OrbitalProperties::APO};
        if (std::fmod(M::Abs(orbit.inclination), 180.0) >= 1.0) list.push_back(OrbitalProperties::LAN);
        if (orbit.eccentricity > 0.05) list.push_back(OrbitalProperties::ARG);
        SystemUtilities::ShuffleList(list, kSPRandom);
        while (static_cast<int>(list.size()) > numberOfModifications) list.erase(list.begin());
        int count = static_cast<int>(list.size());
        while (count-- > 0) {
            const double num = (minimumDeviation + modificationLength + (modificationLength * 1.5 - modificationLength) * kSPRandom->NextDouble()) / 100.0;
            const double num2 = !CoinFlip(kSPRandom) ? 1 : -1;
            switch (list[static_cast<size_t>(count)]) {
                case OrbitalProperties::INC:
                    orbit2.inclination += num2 * num * 90.0;
                    orbit2.inclination = UtilMath::WrapAround(orbit2.inclination, -180.0, 180.0);
                    if (orbit2.inclination < 0.0) {
                        orbit2.inclination = M::Abs(orbit2.inclination);
                        orbit2.LAN += 180.0;
                        orbit2.argumentOfPeriapsis -= 180.0;
                        orbit2.LAN = UtilMath::WrapAround(orbit2.LAN, 0.0, 360.0);
                        orbit2.argumentOfPeriapsis = UtilMath::WrapAround(orbit2.argumentOfPeriapsis, 0.0, 360.0);
                    }
                    break;
                case OrbitalProperties::ARG:
                    orbit2.argumentOfPeriapsis += num2 * num * 360.0;
                    orbit2.argumentOfPeriapsis = UtilMath::WrapAround(orbit2.argumentOfPeriapsis, 0.0, 360.0);
                    break;
                case OrbitalProperties::LAN:
                    orbit2.LAN += num2 * num * 360.0;
                    orbit2.argumentOfPeriapsis -= num2 * num * 360.0;
                    orbit2.LAN = UtilMath::WrapAround(orbit2.LAN, 0.0, 360.0);
                    orbit2.argumentOfPeriapsis = UtilMath::WrapAround(orbit2.argumentOfPeriapsis, 0.0, 360.0);
                    break;
                case OrbitalProperties::SMA:
                case OrbitalProperties::PER:
                case OrbitalProperties::APO: {
                    double num3 = orbit2.semiMajorAxis * (1.0 - orbit2.eccentricity);
                    double num4 = orbit2.semiMajorAxis * (1.0 + orbit2.eccentricity);
                    std::vector<double> list2;
                    if (list[static_cast<size_t>(count)] == OrbitalProperties::PER || list[static_cast<size_t>(count)] == OrbitalProperties::SMA) {
                        const double num5 = num3 * num;
                        if (num3 + num5 <= orbit2.referenceBody->sphereOfInfluence) list2.push_back(num3 + num5);
                        if (num3 - num5 >= orbit2.referenceBody->minOrbitalDistance * 1.05) list2.push_back(num3 - num5);
                        if (!list2.empty()) num3 = list2[static_cast<size_t>(kSPRandom->Next(0, static_cast<int>(list2.size())))];
                    }
                    if (list[static_cast<size_t>(count)] == OrbitalProperties::APO || list[static_cast<size_t>(count)] == OrbitalProperties::SMA) {
                        const double num6 = num4 * num;
                        if (num4 + num6 <= orbit2.referenceBody->sphereOfInfluence) list2.push_back(num4 + num6);
                        if (num4 - num6 >= orbit2.referenceBody->minOrbitalDistance * 1.05) list2.push_back(num4 - num6);
                        if (!list2.empty()) num4 = list2[static_cast<size_t>(kSPRandom->Next(0, static_cast<int>(list2.size())))];
                    }
                    const double val = num3;
                    const double val2 = num4;
                    num3 = M::Min(val, val2);
                    num4 = M::Max(val, val2);
                    orbit2.semiMajorAxis = (num4 + num3) / 2.0;
                    orbit2.eccentricity = (num4 - num3) / (num4 + num3);
                    break;
                }
                default: break;
            }
        }
        return orbit2;
    }
    return orbit;
}
// VesselUtilities.cs:1114
bool VesselAtOrbit(Orbit* o, double deviationWindow, Vessel* v) {
    if (!ActiveVesselFallback(v)) return false;
    if (o == nullptr) return false;
    if ((v->loaded ? v->situation : v->protoVessel->situation) != Vessel::Situations::ORBITING) return false;
    Orbit orbit = OrbitOf(v);
    if (orbit.referenceBody != o->referenceBody) return false;
    if (!SystemUtilities::WithinDeviation(orbit.PeA(), o->PeA(), deviationWindow) && M::Abs(orbit.PeA() - o->PeA()) > DefD("Satellite", "MinimumDeviationWindow")) return false;
    if (!SystemUtilities::WithinDeviation(orbit.ApA(), o->ApA(), deviationWindow) && M::Abs(orbit.ApA() - o->ApA()) > DefD("Satellite", "MinimumDeviationWindow")) return false;
    if (!SystemUtilities::WithinDeviationByValue(M::Abs(orbit.inclination), M::Abs(o->inclination), deviationWindow, 90.0)) return false;
    float num = 0.0f;
    if (std::fmod(M::Abs(o->inclination), 180.0) < 1.0) {
        double num2 = 389.0;
        double num3 = 32.0;
        if (std::fmod(M::Abs(o->inclination), 360.0) < 1.0) {
            num2 = orbit.LAN + orbit.argumentOfPeriapsis;
            num3 = o->LAN + o->argumentOfPeriapsis;
        } else {
            num2 = orbit.LAN - orbit.argumentOfPeriapsis;
            num3 = o->LAN - o->argumentOfPeriapsis;
        }
        if (num2 > 360.0)
            num2 -= 360.0;
        else if (num2 < 0.0)
            num2 += 360.0;
        if (num3 > 360.0)
            num2 -= 360.0;
        else if (num3 < 0.0)
            num2 += 360.0;
        num = static_cast<float>(std::fmod(static_cast<double>(static_cast<float>(M::Abs(num2 - num3))), 360.0));
    } else {
        double num4 = orbit.LAN;
        double num5 = o->LAN;
        if (orbit.inclination < 0.0) num4 = std::fmod(num4 + 180.0, 360.0);
        if (o->inclination < 0.0) num5 = std::fmod(num5 + 180.0, 360.0);
        float num6 = static_cast<float>(std::fmod(static_cast<double>(static_cast<float>(M::Abs(num4 - num5))), 360.0));
        if (num6 > 180.0f) num6 = f32store(360.0 - fx(num6));
        if (static_cast<double>(num6) > deviationWindow / 100.0 * 360.0) return false;
        num = static_cast<float>(std::fmod(static_cast<double>(static_cast<float>(M::Abs(orbit.argumentOfPeriapsis - o->argumentOfPeriapsis))), 360.0));
    }
    if (num > 180.0f) num = f32store(360.0 - fx(num));
    if (o->eccentricity > 0.05 && static_cast<double>(num) > deviationWindow / 100.0 * 360.0) return false;
    return true;
}
// VesselUtilities.cs:1212
double VesselAtOrbitAccuracy(Orbit* o, double deviationWindow, Vessel* v) {
    if (!VesselAtOrbit(o, deviationWindow, v)) return 0.0;
    Orbit obj = OrbitOf(v);
    const double num = SystemUtilities::WithinDeviationAccuracy(obj.PeA(), o->PeA(), deviationWindow);
    const double num2 = SystemUtilities::WithinDeviationAccuracy(obj.ApA(), o->ApA(), deviationWindow);
    const double num3 = SystemUtilities::WithinDeviationByReferenceAccuracy(obj.inclination, o->inclination, 180.0, deviationWindow);
    return (num + num2 + num3) / 3.0;
}
// VesselUtilities.cs:1248
Part* FindFirstPartOrModuleName(const std::vector<std::string>* partNames, const std::vector<std::string>* moduleNames) {
    PartLoader* loader = PartLoader::Instance();
    if (loader == nullptr) return nullptr;
    int count = static_cast<int>(loader->loadedParts.size());
    while (count-- > 0) {
        Part* partPrefab = loader->loadedParts[static_cast<size_t>(count)]->partPrefab.get();
        if (partNames != nullptr) {
            int count2 = static_cast<int>(partNames->size());
            while (count2-- > 0)
                if (GetPartName(partPrefab) == DotName((*partNames)[static_cast<size_t>(count2)])) return partPrefab;
        }
        if (moduleNames == nullptr || moduleNames->empty()) continue;
        int count3 = static_cast<int>(partPrefab->Modules().Count());
        while (count3-- > 0) {
            PartModule* partModule = partPrefab->Modules()[static_cast<size_t>(count3)];
            if (std::find(moduleNames->begin(), moduleNames->end(), partModule->moduleName) != moduleNames->end()) return partPrefab;
        }
    }
    return nullptr;
}

// ------------------------------------------------------------------------------------------------------------------ Survey definitions

// SurveyDefinitionParameter.cs:40
SurveyDefinitionParameter::SurveyDefinitionParameter() { Definition = nullptr; }
// SurveyDefinitionParameter.cs:47
SurveyDefinitionParameter::SurveyDefinitionParameter(const SurveyDefinitionParameter& that)
    : Experiment(that.Experiment), Description(that.Description), Texture(that.Texture), AllowGround(that.AllowGround), AllowLow(that.AllowLow),
      AllowHigh(that.AllowHigh), AllowWater(that.AllowWater), AllowVacuum(that.AllowVacuum), CrewRequired(that.CrewRequired), EVARequired(that.EVARequired),
      FundsMultiplier(that.FundsMultiplier), ScienceMultiplier(that.ScienceMultiplier), ReputationMultiplier(that.ReputationMultiplier), Node(that.Node),
      Definition(that.Definition) {
    int count = static_cast<int>(that.Tech.size());
    while (count-- > 0) Tech.push_back(that.Tech[static_cast<size_t>(count)]);
    if (EVARequired) CrewRequired = true;
}
// SurveyDefinitionParameter.cs:73
SurveyDefinitionParameter::SurveyDefinitionParameter(SurveyDefinition* definition, const ConfigNode* node) {
    if (definition != nullptr && node != nullptr && !(node->name != "PARAM")) {
        ConfigNode* n = const_cast<ConfigNode*>(node);
        Node = node;
        Definition = definition;
        LoadNode(n, "SurveyContract", "Experiment", Experiment, std::string("experiment"), false);
        LoadNode(n, "SurveyContract", "Description", Description, std::string("Perform a survey"), false);
        LoadNode(n, "SurveyContract", "Texture", Texture, std::string("default"), false);
        LoadNode(n, "SurveyContract", "AllowGround", AllowGround, true, false);
        LoadNode(n, "SurveyContract", "AllowLow", AllowLow, true, false);
        LoadNode(n, "SurveyContract", "AllowHigh", AllowHigh, true, false);
        LoadNode(n, "SurveyContract", "AllowWater", AllowWater, true, false);
        LoadNode(n, "SurveyContract", "AllowVacuum", AllowVacuum, true, false);
        LoadNode(n, "SurveyContract", "CrewRequired", CrewRequired, false, false);
        LoadNode(n, "SurveyContract", "EVARequired", EVARequired, false, false);
        LoadNode(n, "SurveyContract", "FundsMultiplier", FundsMultiplier, 1.0f, false);
        LoadNode(n, "SurveyContract", "ScienceMultiplier", ScienceMultiplier, 1.0f, false);
        LoadNode(n, "SurveyContract", "ReputationMultiplier", ReputationMultiplier, 1.0f, false);
        Tech = node->GetValuesList("Tech");
        if (EVARequired) CrewRequired = true;
    }
}

// SurveyDefinition.cs:30
SurveyDefinition::SurveyDefinition()
    : FundsAdvance(DefF("Survey", "Funds", "DefaultAdvance")), FundsReward(DefF("Survey", "Funds", "DefaultReward")),
      FundsFailure(DefF("Survey", "Funds", "DefaultFailure")), ScienceReward(DefF("Survey", "Science", "DefaultReward")),
      ReputationReward(DefF("Survey", "Reputation", "DefaultReward")), ReputationFailure(DefF("Survey", "Reputation", "DefaultFailure")) {}
// SurveyDefinition.cs:36
SurveyDefinition::SurveyDefinition(const SurveyDefinition& that)
    : DataName(that.DataName), AnomalyName(that.AnomalyName), ResultName(that.ResultName), FundsAdvance(that.FundsAdvance), FundsReward(that.FundsReward),
      FundsFailure(that.FundsFailure), ScienceReward(that.ScienceReward), ReputationReward(that.ReputationReward), ReputationFailure(that.ReputationFailure),
      Node(that.Node) {
    int count = static_cast<int>(that.Parameters.size());
    while (count-- > 0) {
        Parameters.push_back(std::make_unique<SurveyDefinitionParameter>(*that.Parameters[static_cast<size_t>(count)]));
    }
}
// SurveyDefinition.cs:56
SurveyDefinition::SurveyDefinition(const ConfigNode* node)
    : FundsAdvance(DefF("Survey", "Funds", "DefaultAdvance")), FundsReward(DefF("Survey", "Funds", "DefaultReward")),
      FundsFailure(DefF("Survey", "Funds", "DefaultFailure")), ScienceReward(DefF("Survey", "Science", "DefaultReward")),
      ReputationReward(DefF("Survey", "Reputation", "DefaultReward")), ReputationFailure(DefF("Survey", "Reputation", "DefaultFailure")) {
    if (node != nullptr && !(node->name != "SURVEY_DEFINITION")) {
        ConfigNode* n = const_cast<ConfigNode*>(node);
        Node = node;
        LoadNode(n, "SurveyContract", "DataName", DataName, std::string("generic"), false);
        LoadNode(n, "SurveyContract", "AnomalyName", AnomalyName, std::string("anomaly"), false);
        LoadNode(n, "SurveyContract", "ResultName", ResultName, std::string("report"), false);
        LoadNode(n, "SurveyContract", "FundsAdvance", FundsAdvance, DefF("Survey", "Funds", "DefaultAdvance"), false);
        LoadNode(n, "SurveyContract", "FundsReward", FundsReward, DefF("Survey", "Funds", "DefaultReward"), false);
        LoadNode(n, "SurveyContract", "FundsFailure", FundsFailure, DefF("Survey", "Funds", "DefaultFailure"), false);
        LoadNode(n, "SurveyContract", "ScienceReward", ScienceReward, DefF("Survey", "Science", "DefaultReward"), false);
        LoadNode(n, "SurveyContract", "ReputationReward", ReputationReward, DefF("Survey", "Reputation", "DefaultReward"), false);
        LoadNode(n, "SurveyContract", "ReputationFailure", ReputationFailure, DefF("Survey", "Reputation", "DefaultFailure"), false);
        std::vector<ConfigNode*> nodes = n->GetNodes("PARAM");
        int num = static_cast<int>(nodes.size());
        while (num-- > 0) Parameters.push_back(std::make_unique<SurveyDefinitionParameter>(this, nodes[static_cast<size_t>(num)]));
    }
}

namespace {
std::vector<std::unique_ptr<SurveyDefinition>>& LoadedSurveyDefinitions() {
    static std::vector<std::unique_ptr<SurveyDefinition>> v;
    return v;
}
bool& SurveyDefinitionsLoaded() {
    static bool b = false;
    return b;
}
}  // namespace

// ContractDefs.cs:1359
std::vector<std::unique_ptr<SurveyDefinition>> SurveyDefinitions() {
    if (!SurveyDefinitionsLoaded()) {
        SurveyDefinitionsLoaded() = true;
        ConfigNode* node = ContractDefsLite::Node("Survey");
        if (node != nullptr) {
            std::vector<ConfigNode*> nodes = node->GetNodes("SURVEY_DEFINITION");
            int num = static_cast<int>(nodes.size());
            while (num-- > 0) LoadedSurveyDefinitions().push_back(std::make_unique<SurveyDefinition>(nodes[static_cast<size_t>(num)]));
        }
    }
    std::vector<std::unique_ptr<SurveyDefinition>> list;
    int count = static_cast<int>(LoadedSurveyDefinitions().size());
    while (count-- > 0) list.push_back(std::make_unique<SurveyDefinition>(*LoadedSurveyDefinitions()[static_cast<size_t>(count)]));
    return list;
}
void ResetSurveyDefinitions() {
    LoadedSurveyDefinitions().clear();
    SurveyDefinitionsLoaded() = false;
}

}  // namespace ksp::Contracts::Lite
