// WP-38 slice 3: FinePrint/Utilities/CompatibilityUtilities.cs. See include/ksp/fineprint/compatibility.hpp.
#include "ksp/fineprint/compatibility.hpp"

#include <algorithm>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/contract.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/contracts/module_test_subject.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Contracts::Lite {

namespace {
std::vector<std::string> SplitChar(const std::string& s, char c) {
    std::vector<std::string> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == c) {
            out.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    return out;
}
std::string Join(const std::vector<std::string>& v, char c) {
    std::string out;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i != 0) out += c;
        out += v[i];
    }
    return out;
}
}  // namespace

// CompatibilityUtilities.cs:15
bool CompatibilityUtilities::OldCareerSave(Game::Modes mode, int saveMajor, int saveMinor, int saveRevision) {
    majorVersion = saveMajor;
    minorVersion = saveMinor;
    revisionVersion = saveRevision;
    if (mode != Game::Modes::CAREER) return false;
    if (Versioning::version_major > saveMajor) return true;
    if (Versioning::version_major == saveMajor) {
        if (Versioning::version_minor > saveMinor) return true;
        if (Versioning::version_minor == saveMinor && Versioning::Revision > saveRevision) return true;
    }
    return false;
}

// CompatibilityUtilities.cs:42
bool CompatibilityUtilities::OldCareerSave(Game::Modes mode, const std::string& versionString) {
    std::vector<std::string> array = SplitChar(versionString, '.');
    if (array.size() != 3) return true;
    int result = 0;
    if (!dotnet::TryParseInt32(array[0], result)) return true;
    int result2 = 0;
    if (!dotnet::TryParseInt32(array[1], result2)) return true;
    int result3 = 0;
    if (!dotnet::TryParseInt32(array[2], result3)) return true;
    return OldCareerSave(mode, result, result2, result3);
}

// CompatibilityUtilities.cs:64
void CompatibilityUtilities::UpdateCareerSave(ConfigNode& node) {
    if (!node.HasValue("name") || node.GetValue("name") != std::optional<std::string>("ContractSystem")) return;
    ConfigNode* node2 = node.GetNode("CONTRACTS");
    if (node2 == nullptr) return;
    bool flag = false;
    int count = node2->nodes().Count();
    while (count-- > 0) {
        ConfigNode* configNode = node2->nodes()[count];
        if ((configNode->name != "CONTRACT" && configNode->name != "CONTRACT_FINISHED") || !configNode->HasValue("type")) continue;
        const std::string value = *configNode->GetValue("type");
        if (value == "BaseContract") {
            if (!configNode->HasValue("contextual")) {
                configNode->AddValue("contextual", false);
                flag = true;
            }
        } else if (value == "StationContract") {
            if (!configNode->HasValue("contextual")) {
                configNode->AddValue("contextual", false);
                flag = true;
            }
        } else if (value == "PartTest") {
            if (!configNode->HasValue("haul")) {
                configNode->AddValue("haul", false);
                flag = true;
            }
        } else if (value == "SurveyContract") {
            if (!configNode->HasValue("contextual")) {
                configNode->AddValue("contextual", false);
                flag = true;
            }
            if (!configNode->HasValue("targetBand")) {
                configNode->AddValue("targetBand", dotnet::EnumToString(FlightBand::NONE));
                flag = true;
            }
            if (!configNode->HasValue("locationName")) {
                CelestialBody* value5 = Planetarium::fetch->Home;
                LoadNode(configNode, "CompatibilityUtilities", "targetBody", value5, Planetarium::fetch->Home, false);
                configNode->AddValue("locationName", value5->displayName());
                flag = true;
            }
            if (!configNode->HasValue("dataName") || !configNode->HasValue("anomalyName") || !configNode->HasValue("reportName")) {
                std::unique_ptr<SurveyDefinition> surveyDefinition;
                int count2 = configNode->nodes().Count();
                while (count2-- > 0) {
                    ConfigNode* configNode3 = configNode->nodes()[count2];
                    if (configNode3->HasValue("name") && !(configNode3->GetValue("name") != std::optional<std::string>("SurveyWaypointParameter"))) {
                        if (configNode3->HasValue("experiment")) surveyDefinition = FindSurveyDefinition(*configNode3->GetValue("experiment"));
                        if (surveyDefinition != nullptr) break;
                    }
                }
                if (surveyDefinition != nullptr) {
                    if (!configNode->HasValue("dataName")) configNode->AddValue("dataName", surveyDefinition->DataName);
                    if (!configNode->HasValue("anomalyName")) configNode->AddValue("anomalyName", surveyDefinition->AnomalyName);
                    if (!configNode->HasValue("reportName")) configNode->AddValue("reportName", surveyDefinition->ResultName);
                    flag = true;
                }
            }
            configNode->RemoveValues("surveyTitle");
            configNode->RemoveValues("surveyBriefing");
            configNode->RemoveValues("surveyDebriefing");
        } else if (value == "RecordTrackContract") {
            node2->nodes().Remove(configNode);
            flag = true;
        } else if (value == "ISRUContract") {
            if (configNode->HasValue("deliverySituation") && SavePriorTo(1, 0, 6)) {
                const std::string newValue = UpgradeVesselSituation(*configNode->GetValue("deliverySituation"));
                configNode->SetValue("deliverySituation", newValue);
                flag = true;
            }
        } else if (value == "SatelliteContract") {
            if (!configNode->HasValue("placementMode")) {
                configNode->AddValue("placementMode", "0");
                flag = true;
            }
        } else if (value == "TourismContract") {
            if (!configNode->HasValue("preposition")) {
                Contract::ContractPrestige value2 = Contract::ContractPrestige::Trivial;
                LoadNode(configNode, "CompatibilityUtilities", "prestige", value2, Contract::ContractPrestige::Trivial, false);
                std::string value3;
                if (static_cast<int>(value2) <= static_cast<int>(Contract::ContractPrestige::Significant)) {
                    CelestialBody* value4 = nullptr;
                    std::vector<ConfigNode*> nodes = configNode->GetNodes("PARAM");
                    ConfigNode* configNode2 = nullptr;
                    int num = static_cast<int>(nodes.size());
                    while (num-- > 0) {
                        if (nodes[static_cast<size_t>(num)]->HasValue("name") && nodes[static_cast<size_t>(num)]->GetValue("name") == std::optional<std::string>("KerbalTourParameter")) {
                            configNode2 = nodes[static_cast<size_t>(num)];
                            break;
                        }
                    }
                    if (configNode2 != nullptr) {
                        std::vector<ConfigNode*> nodes2 = configNode2->GetNodes("PARAM");
                        int num2 = static_cast<int>(nodes2.size()) - 1;
                        while (num2 >= 0) {
                            if (!nodes2[static_cast<size_t>(num2)]->HasValue("name") || !(nodes2[static_cast<size_t>(num2)]->GetValue("name") == std::optional<std::string>("KerbalDestinationParameter"))) {
                                num2--;
                                continue;
                            }
                            // sic: the C# loads from `node` (the ContractSystem node), not from the contract node
                            LoadNode(&node, "CompatibilityUtilities", "targetBody", value4, Planetarium::fetch->Home, false);
                            break;
                        }
                    }
                    if (value4 != nullptr) value3 = value4->isHomeWorld ? ("right here on " + value4->displayName()) : ("around " + value4->displayName());
                } else {
                    value3 = "of the solar system";
                }
                configNode->AddValue("preposition", value3);
                flag = true;
            }
            if (!configNode->HasValue("homeDestinations")) {
                configNode->AddValue("homeDestinations", false);
                flag = true;
            }
            if (!configNode->GetValues("kerbalName").empty()) {
                SystemUtilities::CondenseNodeList(configNode, "kerbalName", "tourists");
                flag = true;
            }
        }
        std::vector<ConfigNode*> nodes3 = configNode->GetNodes("PARAM");
        int num3 = static_cast<int>(nodes3.size());
        while (num3-- > 0) {
            ConfigNode* configNode4 = nodes3[static_cast<size_t>(num3)];
            if (!configNode4->HasValue("name")) continue;
            const std::string value6 = *configNode4->GetValue("name");
            if (value6 == "VesselSystemsParameter") {
                if (!configNode4->HasValue("checkModuleTypes") || !SavePriorTo(1, 1, 4)) continue;
                std::vector<std::string> array = SplitChar(*configNode4->GetValue("checkModuleTypes"), '|');
                for (size_t i = 0; i < array.size(); ++i) {
                    if (array[i] == "Dock")
                        array[i] = "Dock";
                    else if (array[i] == "Power")
                        array[i] = "Generator";
                    else if (array[i] == "Antenna")
                        array[i] = "Antenna";
                }
                configNode4->SetValue("checkModuleTypes", Join(array, '|'));
                flag = true;
            } else if (value6 == "SpecificOrbitParameter") {
                if (!configNode4->HasValue("TargetBody")) {
                    std::string value15 = "0";
                    LoadNode(configNode4, "CompatibilityUtilities", "targetBody", value15, std::string("0"), false);
                    configNode4->RemoveValues("targetBody");
                    configNode4->AddValue("TargetBody", value15);
                    flag = true;
                }
            } else if (value6 == "ProbeSystemsParameter" || value6 == "FacilitySystemsParameter") {
                configNode4->SetValue("name", "VesselSystemsParameter", true);
                flag = true;
                if (!configNode4->HasValue("typeString")) {
                    std::string value19 = "vessel";
                    if (value == "BaseContract")
                        value19 = "outpost";
                    else if (value == "StationContract")
                        value19 = "station";
                    else if (value == "SatelliteContract")
                        value19 = "satellite";
                    configNode4->AddValue("typeString", value19);
                }
                if (!configNode4->HasValue("mannedStatus"))
                    configNode4->AddValue("mannedStatus", dotnet::EnumToString((value6 == "ProbeSystemsParameter") ? MannedStatus::UNMANNED : MannedStatus::ANY));
                if (!configNode4->HasValue("requireNew")) configNode4->AddValue("requireNew", true);
                if (!configNode4->HasValue("checkModuleTypes"))
                    configNode4->AddValue("checkModuleTypes", (value6 == "ProbeSystemsParameter") ? "Antenna|Power" : "Antenna|Power|Dock");
                if (!configNode4->HasValue("checkModuleDescriptions"))
                    configNode4->AddValue("checkModuleDescriptions", (value6 == "ProbeSystemsParameter") ? "has an antenna|can generate power" : "has an antenna|has a docking port|can generate power");
                configNode4->RemoveValues("hasAntenna");
                configNode4->RemoveValues("hasPowerGenerator");
                configNode4->RemoveValues("hasDockingPort");
            } else if (value6 == "LocationAndSituationParameter") {
                if (configNode4->HasValue("targetSituation") && SavePriorTo(1, 0, 6)) {
                    const std::string newValue2 = UpgradeVesselSituation(*configNode4->GetValue("targetSituation"));
                    configNode4->SetValue("targetSituation", newValue2);
                    flag = true;
                }
            } else if (value6 == "SurveyWaypointParameter") {
                if (!configNode4->HasValue("contextual")) {
                    configNode4->AddValue("contextual", false);
                    flag = true;
                }
            } else if (value6 == "PartTest") {
                if (!configNode4->HasValue("body")) {
                    CelestialBody* value16 = nullptr;
                    LoadNode(configNode, "CompatibilityUtilities", "dest", value16, static_cast<CelestialBody*>(nullptr), false);
                    if (value16 != nullptr) {
                        configNode4->AddValue("body", value16->bodyName);
                        flag = true;
                    }
                }
                if (!configNode4->HasValue("situation")) {
                    std::string value17 = "ORBITING";
                    LoadNode(configNode, "CompatibilityUtilities", "sit", value17, std::string("ORBITING"), false);
                    configNode4->AddValue("situation", value17);
                    flag = true;
                }
                if (!configNode4->HasValue("uniqueID")) {
                    int value18 = 0;
                    LoadNode(configNode, "CompatibilityUtilities", "seed", value18, 0, false);
                    KSPRandom kSPRandom(value18);
                    configNode4->AddValue("uniqueID", dotnet::FormatDoubleG(kSPRandom.NextDouble(), 17));
                    flag = true;
                }
                if (!configNode4->HasValue("haul")) {
                    configNode4->AddValue("haul", false);
                    flag = true;
                }
                if (!configNode4->HasValue("repeatability")) {
                    configNode4->AddValue("repeatability", dotnet::EnumToString(PartTestConstraint::TestRepeatability::ONCEPERPART));
                    flag = true;
                }
            } else if (value6 == "RecordTrackingParameter") {
                configNode->nodes().Remove(configNode4);
                flag = true;
            } else if (value6 == "ResourceExtractionParameter") {
                if (!configNode4->HasValue("modules")) {
                    SystemUtilities::SaveNodeList(configNode4, "modules", std::vector<std::string>{"ModuleResourceHarvester"});
                    flag = true;
                }
            } else if (value6 == "StationaryPointParameter") {
                if (!configNode4->HasValue("deviation")) {
                    configNode4->AddValue("deviation", 12.5);
                    flag = true;
                }
            } else if (value6 == "PartRequestParameter") {
                if (configNode4->HasValue("partNames")) {
                    const std::string value7 = *configNode4->GetValue("partNames");
                    if (value7.find(',') != std::string::npos) {
                        std::vector<std::string> list = SplitChar(value7, ',');
                        configNode4->RemoveValues("partNames");
                        SystemUtilities::SaveNodeList(configNode4, "partNames", list);
                        flag = true;
                    }
                }
                if (configNode4->HasValue("moduleNames")) {
                    const std::string value8 = *configNode4->GetValue("moduleNames");
                    if (value8.find(',') != std::string::npos) {
                        std::vector<std::string> list2 = SplitChar(value8, ',');
                        configNode4->RemoveValues("moduleNames");
                        SystemUtilities::SaveNodeList(configNode4, "moduleNames", list2);
                        flag = true;
                    }
                }
                if (!configNode4->HasValue("partDescription")) {
                    std::string value9 = "part";
                    std::optional<std::string> value10 = configNode4->GetValue("partNames");
                    std::optional<std::string> value11 = configNode4->GetValue("moduleNames");
                    if (!value10.has_value() || !value11.has_value()) throw NullReferenceException("Object reference not set to an instance of an object.");
                    std::vector<std::string> partNames = SplitChar(*value10, '|');
                    std::vector<std::string> moduleNames = SplitChar(*value11, '|');
                    Part* part = FindFirstPartOrModuleName(&partNames, &moduleNames);
                    if (part != nullptr) value9 = part->partInfo->title;
                    configNode4->AddValue("partDescription", value9);
                }
                if (!configNode4->HasValue("article")) {
                    std::string value12 = "a";
                    const std::string value13 = *configNode4->GetValue("partDescription");
                    if (value13.size() > 0 && StringUtilities::IsVowel(value13[0])) value12 = "an";
                    configNode4->AddValue("article", value12);
                }
                if (!configNode4->HasValue("vesselDescription")) {
                    std::string value14 = "vessel";
                    if (value == "SatelliteContract")
                        value14 = "satellite";
                    else if (value == "BaseContract")
                        value14 = "outpost";
                    else if (value == "StationContract")
                        value14 = "station";
                    configNode4->AddValue("vesselDescription", value14);
                }
                configNode4->RemoveValues("title");
            }
        }
    }
    if (flag) log::Info("Contract Log: Outdated save file detected! All contracts in save file were updated!");
}

// CompatibilityUtilities.cs:479
std::unique_ptr<SurveyDefinition> CompatibilityUtilities::FindSurveyDefinition(const std::string& experiment) {
    ConfigNode* configNode = nullptr;
    std::vector<ConfigNode*> configNodes;
    if (CompatibilityEnvironment::contractsConfigNodes)
        configNodes = CompatibilityEnvironment::contractsConfigNodes();
    else if (GameDatabase::Instance() != nullptr)
        configNodes = GameDatabase::Instance()->GetConfigNodes("Contracts");
    else if (ConfigNode* loaded = ContractDefsLite::Loaded() ? ContractDefsLite::Node("") : nullptr)
        configNodes.push_back(loaded);
    int num = static_cast<int>(configNodes.size());
    while (num-- > 0) configNode = configNodes[static_cast<size_t>(num)];
    if (configNode == nullptr) {
        log::Warning("Contract Log: Failed to update outdated survey contract - invalid configuration!");
        return nullptr;
    }
    ConfigNode* node = configNode->GetNode("Survey");
    if (node == nullptr) {
        log::Warning("Contract Log: Failed to update outdated survey contract - invalid configuration!");
        return nullptr;
    }
    std::vector<ConfigNode*> nodes = node->GetNodes("SURVEY_DEFINITION");
    int num2 = static_cast<int>(nodes.size());
    while (num2-- > 0) {
        ConfigNode* configNode2 = nodes[static_cast<size_t>(num2)];
        std::vector<ConfigNode*> nodes2 = configNode2->GetNodes("PARAM");
        int num3 = static_cast<int>(nodes2.size());
        while (num3-- > 0) {
            ConfigNode* configNode3 = nodes2[static_cast<size_t>(num3)];
            if (configNode3->HasValue("Experiment") && configNode3->GetValue("Experiment") == std::optional<std::string>(experiment))
                return std::make_unique<SurveyDefinition>(configNode2);
        }
    }
    log::Warning("Contract Log: Failed to update outdated survey contract - survey definition not found!");
    return nullptr;
}

// CompatibilityUtilities.cs:519
bool CompatibilityUtilities::SavePriorTo(int major, int minor, int revision) {
    return KSPUtil::CheckVersion(majorVersion, minorVersion, revisionVersion, major, minor, revision) == VersionCompareResult::INCOMPATIBLE_TOO_EARLY;
}

// CompatibilityUtilities.cs:524
std::string CompatibilityUtilities::UpgradeVesselSituation(const std::string& situation) {
    int result = -1;
    if (!dotnet::TryParseInt32(situation, result)) return situation;
    if (SavePriorTo(1, 0, 5)) {
        switch (result) {
            case 0: return dotnet::EnumToString(Vessel::Situations::LANDED);
            case 1: return dotnet::EnumToString(Vessel::Situations::SPLASHED);
            case 2: return dotnet::EnumToString(Vessel::Situations::PRELAUNCH);
            case 3: return dotnet::EnumToString(Vessel::Situations::FLYING);
            case 4: return dotnet::EnumToString(Vessel::Situations::SUB_ORBITAL);
            case 5: return dotnet::EnumToString(Vessel::Situations::ORBITING);
            case 6: return dotnet::EnumToString(Vessel::Situations::ESCAPING);
            case 7: return dotnet::EnumToString(Vessel::Situations::DOCKED);
            default: break;
        }
    } else if (SavePriorTo(1, 0, 6)) {
        switch (result) {
            case 32: return dotnet::EnumToString(Vessel::Situations::ORBITING);
            case 0: return dotnet::EnumToString(Vessel::Situations::LANDED);
            case 1: return dotnet::EnumToString(Vessel::Situations::LANDED);
            case 2: return dotnet::EnumToString(Vessel::Situations::SPLASHED);
            case 3: return dotnet::EnumToString(Vessel::Situations::FLYING);
            case 4: return dotnet::EnumToString(Vessel::Situations::PRELAUNCH);
            case 5: return dotnet::EnumToString(Vessel::Situations::ORBITING);
            case 6: return dotnet::EnumToString(Vessel::Situations::ESCAPING);
            case 7: return dotnet::EnumToString(Vessel::Situations::DOCKED);
            case 8: return dotnet::EnumToString(Vessel::Situations::FLYING);
            case 16: return dotnet::EnumToString(Vessel::Situations::SUB_ORBITAL);
            case 128: return dotnet::EnumToString(Vessel::Situations::DOCKED);
            case 64: return dotnet::EnumToString(Vessel::Situations::ESCAPING);
            default: break;
        }
    }
    return situation;
}

// CompatibilityUtilities.cs:588
void CompatibilityUtilities::CleanUpUnsanitaryEVAKerbals(ConfigNode& flightState) {
    int num = dotnet::ParseInt32(*flightState.GetValue("activeVessel"));
    std::vector<ConfigNode*> nodes = flightState.GetNodes("VESSEL");
    int num2 = static_cast<int>(nodes.size());
    while (num2-- > 0) {
        ConfigNode* configNode = nodes[static_cast<size_t>(num2)];
        if (!(configNode->GetValue("type") == std::optional<std::string>("EVA"))) continue;
        std::vector<ConfigNode*> nodes2 = configNode->GetNodes("PART");
        int i = 0;
        for (int num3 = static_cast<int>(nodes2.size()); i < num3; i++) {
            ConfigNode* configNode2 = nodes2[static_cast<size_t>(i)];
            if (configNode2->GetValue("name") == std::optional<std::string>("kerbalEVA") && !configNode2->GetValue("crew").has_value()) {
                flightState.RemoveNode(configNode);
                if (num2 > num) num--;
                break;
            }
        }
    }
    flightState.SetValue("activeVessel", dotnet::FormatInt64(num));
}

}  // namespace ksp::Contracts::Lite
