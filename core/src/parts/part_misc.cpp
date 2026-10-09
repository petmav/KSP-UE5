// WP-13: FlagNameManager.cs and AvailableVariantTheme.cs.
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/available_variant_theme.hpp"
#include "ksp/parts/flag_name_manager.hpp"

namespace ksp {

// FlagNameManager.cs:25
void FlagNameManager::Initialize(const std::vector<FlagMesh>& flagMeshes) {
    for (const FlagMesh& flagMesh : flagMeshes) {
        created_ = true;
        auto& byOrientation = flagNames_[flagMesh.flagOrientation];
        auto& byIndex = byOrientation[flagMesh.indexOffset];
        byIndex[NameType::SizeName];     // ContainsKey / Add on demand
        byIndex[NameType::DisplayName];
        byIndex[NameType::SizeName].push_back(flagMesh.name);
        byIndex[NameType::DisplayName].push_back(flagMesh.displayName);
    }
}

// FlagNameManager.cs:55
std::vector<std::string> FlagNameManager::SizeNames(FlagOrientation flagOrientation, int structureType) const {
    auto o = flagNames_.find(flagOrientation);
    if (created_ && o != flagNames_.end()) {
        auto i = o->second.find(structureType);
        if (i != o->second.end()) return i->second.at(NameType::SizeName);
    }
    return {};
}

// FlagNameManager.cs:64
bool FlagNameManager::HasSizeNames(FlagOrientation flagOrientation, int structureType) const {
    auto o = flagNames_.find(flagOrientation);
    if (created_ && o != flagNames_.end()) {
        auto i = o->second.find(structureType);
        if (i != o->second.end()) {
            auto n = i->second.find(NameType::SizeName);
            if (n != i->second.end()) return !n->second.empty();
        }
    }
    return false;
}

// FlagNameManager.cs:73
std::vector<std::string> FlagNameManager::DisplayNames(FlagOrientation flagOrientation, int structureType) const {
    auto o = flagNames_.find(flagOrientation);
    if (created_ && o != flagNames_.end()) {
        auto i = o->second.find(structureType);
        if (i != o->second.end()) return i->second.at(NameType::DisplayName);
    }
    return {};
}

// AvailableVariantTheme.cs:26
AvailableVariantTheme AvailableVariantTheme::CreateVariantTheme() {
    using KSP::Localization::Localizer;
    AvailableVariantTheme t;
    t.name = Localizer::Format("#autoLOC_168872");
    t.displayName = Localizer::Format("#autoLOC_168872");
    t.description = Localizer::Format("#autoLOC_168872");
    t.primaryColor = "#ffffff";
    t.secondaryColor = "#f0f0f0";
    return t;
}

// AvailableVariantTheme.cs:39
void AvailableVariantTheme::Load(const ConfigNode& node) {
    node.TryGetValue("name", name);
    node.TryGetValue("displayName", displayName);
    node.TryGetValue("description", description);
    node.TryGetValue("primaryColor", primaryColor);
    node.TryGetValue("secondaryColor", secondaryColor);
}

// AvailableVariantTheme.cs:48
void AvailableVariantTheme::Save(ConfigNode& node) const {
    node.AddValue("name", name);
    node.AddValue("displayName", displayName);
    node.AddValue("description", description);
    node.AddValue("primaryColor", primaryColor);
    node.AddValue("secondaryColor", secondaryColor);
}

}  // namespace ksp
