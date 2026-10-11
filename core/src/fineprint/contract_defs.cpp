// WP-38 slice 3: FinePrint.ContractDefs (the hand-written part: registry lookup, Awake, LoadDefinitions). The writer, the reader and the
// typed members are generated (contract_defs_gen.cpp, contract_defs_members.inc).
#include "ksp/fineprint/contract_defs.hpp"

#include <algorithm>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_cargo_part.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::FinePrint {

using KSP::Localization::Localizer;

const ContractDefs::Entry* ContractDefs::Find(const std::string& section, const std::string& sub, const std::string& key) {
    for (const Entry& e : Registry()) {
        if (e.serenityOnly) continue;
        if (section == e.section && sub == e.sub && key == e.key) return &e;
    }
    return nullptr;
}

// FinePrint/ContractDefs.cs:1387
void ContractDefs::LoadDefinitions() {
    PolarOrbitName = Localizer::Format("#autoLOC_7001202");
    EquatorialOrbitName = Localizer::Format("#autoLOC_7001201");
    TundraOrbitName = Localizer::Format("#autoLOC_7001204");
    SunStationaryName = Localizer::Format("#autoLOC_7001205");
    HomeStationaryName = Localizer::Format("#autoLOC_7001206");
    OtherStationaryName = Localizer::Format("#autoLOC_7001207");
    SunSynchronousName = Localizer::Format("#autoLOC_7001208");
    HomeSynchronousName = Localizer::Format("#autoLOC_7001209");
    OtherSynchronousName = Localizer::Format("#autoLOC_7001210");
    MolniyaName = Localizer::Format("#autoLOC_7001203");
}

// FinePrint/ContractDefs.cs:1401
void ContractDefs::Awake(const std::vector<const ConfigNode*>& contractsNodes, bool cfgFileExists) {
    config = std::make_shared<ConfigNode>();
    if (!cfgFileExists) CreateDefaultConfig();
    int num = static_cast<int>(contractsNodes.size());
    while (num-- > 0) contractsNodes[static_cast<size_t>(num)]->CopyTo(*config);
    LoadConfig();
    LoadDefinitions();
}

// ---- the pre-built craft data ----------------------------------------------------------------------------------------------------------------------

namespace {
std::vector<ConfigNode*> DatabaseNodes(const std::string& type) {
    if (ContractDefs::gameDatabaseConfigNodes) return ContractDefs::gameDatabaseConfigNodes(type);
    if (GameDatabase::Instance() != nullptr) return GameDatabase::Instance()->GetConfigNodes(type);
    return {};
}
}  // namespace

bool ContractDefs::PositionDictionary::ContainsKey(const std::string& key) const {
    for (const auto& kv : items)
        if (kv.first == key) return true;
    return false;
}
ContractDefs::PositionList& ContractDefs::PositionDictionary::operator[](const std::string& key) {
    for (auto& kv : items)
        if (kv.first == key) return kv.second;
    throw KeyNotFoundException("The given key was not present in the dictionary.");
}
ContractDefs::PositionList* ContractDefs::PositionDictionary::TryGetValue(const std::string& key) {
    for (auto& kv : items)
        if (kv.first == key) return &kv.second;
    return nullptr;
}

// FinePrint/ContractDefs.cs:1432
void ContractDefs::TechResearched() {
    loadedContractCraftDefs = false;
    LoadContractCraftDefs();
}

// FinePrint/ContractDefs.cs:1438
void ContractDefs::LoadContractCraftDefs() {
    if (!loadedContractCraftDefs) {
        std::vector<ConfigNode*> configNodes = DatabaseNodes("CONTRACTCRAFT");
        ConfigNode node;
        int num = static_cast<int>(configNodes.size());
        while (num-- > 0) configNodes[static_cast<size_t>(num)]->CopyTo(node);
        PreBuiltCraftPositions = GetPreBuiltCraftPositions(node);
        PreBuiltCraftDefs = GetPreBuiltCraft("ContractDefs", node);
        loadedContractCraftDefs = true;
    }
}

// FinePrint/ContractDefs.cs:1455
void ContractDefs::LoadConstructionPartsList() {
    if (loadedConstructionPartsList) return;
    Contracts::Lite::ConstructionParts::List list;
    std::vector<ConfigNode*> configNodes = DatabaseNodes("CONTRACT_CONSTRUCTION_PARTS");
    ConfigNode configNode;
    int num = static_cast<int>(configNodes.size());
    while (num-- > 0) configNodes[static_cast<size_t>(num)]->CopyTo(configNode);
    std::vector<ConfigNode*> nodes = configNode.GetNodes("CONSTRUCTION_PART");
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::string value;
        nodes[i]->TryGetValue("name", value);
        std::vector<std::string> values = nodes[i]->GetValues("contract");
        if (value.empty() || values.empty()) continue;
        std::replace(value.begin(), value.end(), '_', '.');
        const AvailablePart* partInfoByName = SaveEnvironment::Current().getPartInfoByName ? SaveEnvironment::Current().getPartInfoByName(value) : nullptr;
        bool flag = false;
        if (partInfoByName != nullptr && partInfoByName->partPrefab != nullptr) {
            PartModule* m = partInfoByName->partPrefab->FindModuleImplementing([](PartModule& pm) { return dynamic_cast<ModuleCargoPart*>(&pm) != nullptr; });
            auto* moduleCargoPart = dynamic_cast<ModuleCargoPart*>(m);
            if (moduleCargoPart != nullptr && moduleCargoPart->packedVolume() >= 0.0f) {
                if (!Contracts::Lite::ContainsPart(list, value)) list.push_back(std::make_shared<Contracts::Lite::ConstructionPart>(value, partInfoByName, values));
                flag = true;
            }
        }
        if (!flag) log::Warning("[ContractDefs]: Ignored invalid construction contract part: " + value + ",");
    }
    constructionParts = std::make_shared<Contracts::Lite::ConstructionParts>(std::move(list));
    loadedConstructionPartsList = true;
}

// FinePrint/ContractDefs.cs:1503
std::shared_ptr<Contracts::Lite::PreBuiltCraft> ContractDefs::GetPreBuiltCraft(const std::string& className, const ConfigNode& node) {
    std::vector<ConfigNode*> nodes = node.GetNodes("PREBUILTCRAFT");
    Contracts::Lite::PreBuiltCraft::List list;
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::string value;
        bool value2 = false;
        if (!nodes[i]->TryGetValue("url", value)) continue;
        value = applicationRootPath + value;
        nodes[i]->TryGetValue("brokenParts", value2);
        std::shared_ptr<Contracts::Lite::PreBuiltCraftDefinition> preBuiltCraftDefinition = Contracts::Lite::SystemUtilities::ValidateLoadCraftNode(value, className, value2, true);
        if (preBuiltCraftDefinition != nullptr) {
            nodes[i]->TryGetValue("AllowGround", preBuiltCraftDefinition->allowGround);
            nodes[i]->TryGetValue("AllowOrbit", preBuiltCraftDefinition->allowOrbit);
            nodes[i]->TryGetValue("AllowWater", preBuiltCraftDefinition->allowWater);
            nodes[i]->TryGetValue("UsePreBuiltPos", preBuiltCraftDefinition->usePreBuiltPositions);
            std::vector<std::string> values = nodes[i]->GetValues("ContractType");
            if (!values.empty()) {
                preBuiltCraftDefinition->contractTypes = values;
                list.push_back(preBuiltCraftDefinition);
            }
        }
    }
    return std::make_shared<Contracts::Lite::PreBuiltCraft>(std::move(list));
}

// FinePrint/ContractDefs.cs:1535
ContractDefs::PositionDictionary ContractDefs::GetPreBuiltCraftPositions(const ConfigNode& node) {
    PositionDictionary dictionaryValueList;
    std::vector<ConfigNode*> nodes = node.GetNodes("PREBUILTPOSITIONS");
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::string value;
        CelestialBody* body = nullptr;
        if (nodes[i]->TryGetValue("Body", value)) {
            for (CelestialBody* b : FlightRegistry::Bodies())
                if (b != nullptr && b->bodyName == value) {
                    body = b;
                    break;
                }
        }
        if (body != nullptr) {
            Contracts::Lite::PreBuiltCraftPosition preBuiltCraftPosition;
            nodes[i]->TryGetValue("CenterLatitude", preBuiltCraftPosition.CenterLatitude);
            nodes[i]->TryGetValue("CenterLongitude", preBuiltCraftPosition.CenterLongitude);
            nodes[i]->TryGetValue("SearchRadius", preBuiltCraftPosition.SearchRadius);
            if (!dictionaryValueList.ContainsKey(value)) dictionaryValueList.items.emplace_back(value, PositionList{});
            if (PositionList* val = dictionaryValueList.TryGetValue(value)) val->push_back(preBuiltCraftPosition);
        }
    }
    return dictionaryValueList;
}

}  // namespace ksp::FinePrint
