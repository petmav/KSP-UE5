// WP-38 slice 3: the pre-built craft data model. See include/ksp/fineprint/prebuilt.hpp.
#include "ksp/fineprint/prebuilt.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/save/save_environment.hpp"
#include "../save/save_util.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/ship/ship_template.hpp"
#include "ksp/science/science.hpp"

namespace ksp::Contracts::Lite {

namespace {
void AddUnique(std::vector<std::string>& list, const std::string& v) {
    if (std::find(list.begin(), list.end(), v) == list.end()) list.push_back(v);
}
const AvailablePart* PartInfoByName(const std::string& name) {
    return SaveEnvironment::Current().getPartInfoByName ? SaveEnvironment::Current().getPartInfoByName(name) : nullptr;
}
bool TechAvailable(const AvailablePart* p) {
    if (PreBuiltEnvironment::partTechAvailable) return PreBuiltEnvironment::partTechAvailable(p);
    return ResearchAndDevelopment::PartTechAvailable(p);
}
std::vector<std::string> SplitComma(const std::string& s) {
    std::vector<std::string> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == ',') {
            out.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    return out;
}
}  // namespace

// CraftProfileInfo.cs:76
CraftProfileInfo& CraftProfileInfo::LoadDetailsFromCraftFile(const ConfigNode& root, const std::string& filePath, bool brokenParts, bool bypassTechCheck) {
    if (root.TryGetValue("version", version))
        compatibility = KSPUtil::CheckVersion(version, ShipConstruct::lastCompatibleMajor, ShipConstruct::lastCompatibleMinor, ShipConstruct::lastCompatibleRev);
    if (root.HasValue("description")) description = root.GetValue("description").value_or("");
    save_detail::TryGetEnum(root, "type", shipFacility, EditorFacility::VAB);
    root.TryGetValue("steamPublishedFileId", steamPublishedFileId);
    if (steamPublishedFileId == 0)
        steamPublishedFileId = ShipConstruction::env.steamIdFromSteamFolder ? ShipConstruction::env.steamIdFromSteamFolder(filePath) : 0;
    // ShipTemplate.LoadShip(root) (the C# loads the node itself: the template keeps the reference)
    ShipTemplate shipTemplate;
    shipTemplate.LoadShip(root.CreateCopy());
    shipName = shipTemplate.shipName;
    partCount = shipTemplate.partCount;
    stageCount = shipTemplate.stageCount;
    totalCost = shipTemplate.totalCost;
    totalMass = shipTemplate.totalMass;
    shipSize = shipTemplate.GetShipSize();
    shipPartsExperimental = shipTemplate.shipPartsExperimental;
    duplicatedParts = shipTemplate.duplicatedParts;
    partNames.clear();
    partModules.clear();
    std::vector<ConfigNode*> nodes = root.GetNodes("PART");
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::optional<std::string> value = nodes[i]->GetValue("part");
        if (!value.has_value()) continue;
        const size_t us = value->find('_');
        if (us == std::string::npos) throw std::out_of_range("Length cannot be less than zero. (Parameter 'length')");
        const std::string partName = value->substr(0, us);
        AddUnique(partNames, partName);
        const AvailablePart* partInfoByName = PartInfoByName(partName);
        if (partInfoByName == nullptr || (!bypassTechCheck && !TechAvailable(partInfoByName))) AddUnique(UnavailableShipParts, partName);  // excludedParts is empty
        std::vector<ConfigNode*> nodes2 = nodes[i]->GetNodes("MODULE");
        for (size_t j = 0; j < nodes2.size(); ++j) {
            std::optional<std::string> value2 = nodes2[j]->GetValue("name");
            if (!value2.has_value()) continue;
            AddUnique(partModules, *value2);
            if (GetPartModuleClass(*value2) == nullptr) AddUnique(UnavailableShipPartModules, *value2);
            if (*value2 == "ModuleInventoryPart") {
                std::string value3;
                if (nodes2[j]->TryGetValue("inventory", value3)) {
                    std::vector<std::string> array = SplitComma(value3);
                    for (size_t k = 0; k < array.size(); ++k) {
                        AddUnique(partNames, array[k]);
                        const AvailablePart* partInfoByName2 = PartInfoByName(array[k]);
                        if (partInfoByName2 == nullptr || (!bypassTechCheck && !TechAvailable(partInfoByName2))) AddUnique(UnavailableShipParts, array[k]);
                    }
                }
            }
            if (brokenParts) {
                std::string value4;
                if (nodes2[j]->TryGetValue("deployState", value4) && value4 == "BROKEN") AddUnique(BrokenShipParts, partName);
            }
        }
    }
    return *this;
}

// CraftProfileInfo.cs:172 (the part and module names are checked against the part database and the module classes like the validation half)
void CraftProfileInfo::Load(const ConfigNode& node) {
    node.TryGetValue("shipName", shipName);
    {
        // description = node.GetValue("description").Replace(U+00A8, LF): a missing value is a NullReferenceException
        const std::optional<std::string> d = node.GetValue("description");
        if (!d.has_value()) throw NullReferenceException("description");
        std::string t = *d;
        size_t pos;
        while ((pos = t.find("\xC2\xA8")) != std::string::npos) t.replace(pos, 2, "\n");  // U+00A8 in UTF-8
        description = t;
    }
    node.TryGetValue("version", version);
    compatibility = KSPUtil::CheckVersion(version, ShipConstruct::lastCompatibleMajor, ShipConstruct::lastCompatibleMinor, ShipConstruct::lastCompatibleRev);
    node.TryGetValue("partCount", partCount);
    node.TryGetValue("stageCount", stageCount);
    node.TryGetValue("totalCost", totalCost);
    node.TryGetValue("totalMass", totalMass);
    node.TryGetValue("shipSize", shipSize);
    node.TryGetValue("steamPublishedFileId", steamPublishedFileId);
    save_detail::TryGetEnum(node, "type", shipFacility, EditorFacility::VAB);
    node.TryGetValue("duplicatedParts", duplicatedParts);
    if (node.HasValue("partNames")) partNames = node.GetValuesList("partNames");
    if (shipPartsUnlocked()) {
        for (size_t num = 0; shipPartsUnlocked() && num < partNames.size(); ++num) {
            const AvailablePart* p = PartInfoByName(partNames[num]);
            if (p == nullptr || !TechAvailable(p)) AddUnique(UnavailableShipParts, partNames[num]);  // excludedParts is empty
        }
    }
    if (node.HasValue("partModules")) partModules = node.GetValuesList("partModules");
    if (shipPartsUnlocked()) {
        for (size_t num2 = 0; shipPartsUnlocked() && num2 < partModules.size(); ++num2)
            if (GetPartModuleClass(partModules[num2]) == nullptr) AddUnique(UnavailableShipPartModules, partModules[num2]);
    }
    node.TryGetValue("saveMD5", saveMD5);
    node.TryGetValue("lastWriteTime", lastWriteTime);
}

// CraftProfileInfo.cs:223
void CraftProfileInfo::Save(ConfigNode& node) const {
    node.AddValue("shipName", shipName);
    std::string d = description;
    size_t pos = 0;
    while ((pos = d.find('\n', pos)) != std::string::npos) {
        d.replace(pos, 1, "\xC2\xA8");
        pos += 2;
    }
    node.AddValue("description", d);
    node.AddValue("version", version);
    node.AddValue("partCount", static_cast<int32_t>(partCount));
    node.AddValue("stageCount", static_cast<int32_t>(stageCount));
    node.AddValue("totalCost", totalCost);
    node.AddValue("totalMass", totalMass);
    node.AddValue("shipSize", shipSize);
    node.AddValue("type", dotnet::EnumToString(shipFacility));
    node.AddValue("duplicatedParts", duplicatedParts);
    node.AddValue("steamPublishedFileId", steamPublishedFileId);
    for (size_t i = 0; i < partNames.size(); ++i) node.AddValue("partNames", partNames[i]);
    for (size_t j = 0; j < partModules.size(); ++j) node.AddValue("partModules", partModules[j]);
    node.AddValue("saveMD5", saveMD5);
    node.AddValue("lastWriteTime", lastWriteTime);
}

// CraftProfileInfo.cs:375
std::string CraftProfileInfo::GetErrorMessage() const {
    using KSP::Localization::Localizer;
    std::string text;
    if (compatibility != VersionCompareResult::COMPATIBLE) text += Localizer::Format("#autoLOC_8004244", {shipName});
    for (size_t i = 0; i < UnavailableShipParts.size(); ++i) text += Localizer::Format("#autoLOC_8004243", {shipName, UnavailableShipParts[i]});
    for (size_t j = 0; j < UnavailableShipPartModules.size(); ++j) text += Localizer::Format("#autoLOC_8004255", {shipName, UnavailableShipPartModules[j]});
    // string.Trim()
    const size_t a = text.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return std::string();
    const size_t b = text.find_last_not_of(" \t\r\n");
    return text.substr(a, b - a + 1);
}

// CraftProfileInfo.cs:393
bool CraftProfileInfo::CraftHasErrors() const {
    if (compatibility != VersionCompareResult::COMPATIBLE) return true;
    if (!UnavailableShipParts.empty()) return true;
    if (!UnavailableShipPartModules.empty()) return true;
    return false;
}

// PreBuiltCraftDefinition.cs:30
PreBuiltCraftDefinition::PreBuiltCraftDefinition(const std::string& url, ConfigNode::NodePtr node) : craftURL(url), craftNode(std::move(node)) {}

// PreBuiltCraft.cs:15
PreBuiltCraft::PreBuiltCraft(List preBuiltDefinitions) : preBuiltDefinitions_(std::move(preBuiltDefinitions)) {}

// PreBuiltCraft.cs:21
PreBuiltCraft::List& PreBuiltCraft::CraftForContractType(const std::string& contractType) {
    auto it = cachedLists_.find(contractType);
    if (it != cachedLists_.end()) return it->second;
    List list;
    for (size_t i = 0; i < preBuiltDefinitions_.size(); ++i) {
        const auto& types = preBuiltDefinitions_[i]->contractTypes;
        if (std::find(types.begin(), types.end(), contractType) != types.end()) list.push_back(preBuiltDefinitions_[i]);
    }
    return cachedLists_.emplace(contractType, std::move(list)).first->second;
}

// PreBuiltCraft.cs:39
std::shared_ptr<PreBuiltCraftDefinition> PreBuiltCraft::CraftByURL(std::string url) {
    const size_t startIndex = url.rfind("GameData");
    if (startIndex == std::string::npos) throw std::out_of_range("StartIndex cannot be less than zero. (Parameter 'startIndex')");
    url = url.substr(startIndex);
    for (size_t num = 0; num < preBuiltDefinitions_.size(); ++num) {
        const std::string& craft = preBuiltDefinitions_[num]->craftURL;
        const size_t startIndex2 = craft.rfind("GameData");
        if (startIndex2 == std::string::npos) throw std::out_of_range("StartIndex cannot be less than zero. (Parameter 'startIndex')");
        if (craft.substr(startIndex2) == url) return preBuiltDefinitions_[num];
    }
    return nullptr;
}

// ConstructionPart.cs:13
ConstructionPart::ConstructionPart(const std::string& name, const AvailablePart* aPart, std::vector<std::string> contractTypes_)
    : partName(name), availablePart(aPart), contractTypes(std::move(contractTypes_)) {}

// ConstructionParts.cs:15
ConstructionParts::ConstructionParts(List constructionPartDefinitions) : constructionPartDefinitions_(std::move(constructionPartDefinitions)) {}

// ConstructionParts.cs:21
ConstructionParts::List& ConstructionParts::PartsForContractType(const std::string& contractType) {
    auto it = cachedLists_.find(contractType);
    if (it != cachedLists_.end()) return it->second;
    List list;
    for (size_t i = 0; i < constructionPartDefinitions_.size(); ++i) {
        const auto& types = constructionPartDefinitions_[i]->contractTypes;
        if (std::find(types.begin(), types.end(), contractType) != types.end()) list.push_back(constructionPartDefinitions_[i]);
    }
    return cachedLists_.emplace(contractType, std::move(list)).first->second;
}

// ConstructionExtensions.cs:7
bool ContainsPart(const ConstructionParts::List& list, const std::string& partName) {
    for (size_t num = 0; num < list.size(); ++num)
        if (list[num]->partName == partName) return true;
    return false;
}

namespace SystemUtilities {

// SystemUtilities.cs:79
std::shared_ptr<PreBuiltCraftDefinition> ValidateLoadCraftNode(const std::string& url, const std::string& className, bool brokenParts) {
    return ValidateLoadCraftNode(url, className, brokenParts, false);
}

// SystemUtilities.cs:84
std::shared_ptr<PreBuiltCraftDefinition> ValidateLoadCraftNode(const std::string& url, const std::string& className, bool brokenParts, bool bypassTechCheck) {
    const bool exists = PreBuiltEnvironment::fileExists ? PreBuiltEnvironment::fileExists(url) : std::filesystem::exists(std::filesystem::path(url));
    if (exists) {
        ConfigNode::NodePtr configNode = PreBuiltEnvironment::loadConfigNode ? PreBuiltEnvironment::loadConfigNode(url) : ConfigNode::Load(url);
        if (configNode == nullptr) {
            log::Warning("Contract Log: " + className + " prebuilt craft at " + url + " is invalid.");
            return nullptr;
        }
        auto preBuiltCraftDefinition = std::make_shared<PreBuiltCraftDefinition>(url, configNode);
        preBuiltCraftDefinition->craftInfo.LoadDetailsFromCraftFile(*configNode, url, brokenParts, bypassTechCheck);
        if (preBuiltCraftDefinition->craftInfo.CraftHasErrors()) {
            log::Warning("Contract Log: " + className + " prebuilt craft at " + url + " invalid.");
            log::Warning(preBuiltCraftDefinition->craftInfo.GetErrorMessage());
            return nullptr;
        }
        if (brokenParts && preBuiltCraftDefinition->craftInfo.BrokenShipParts.size() < 1) {
            log::Warning("Contract Log: " + className + " prebuilt craft at " + url + " is supposed to contain broken parts, but it does not. So craft is invalid.");
            return nullptr;
        }
        return preBuiltCraftDefinition;
    }
    log::Warning("Contract Log: " + className + " unable to find a prebuilt craft at " + url + " Craft will not be added to contract possibilities.");
    return nullptr;
}

}  // namespace SystemUtilities

}  // namespace ksp::Contracts::Lite
