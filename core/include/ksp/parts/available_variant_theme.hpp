// WP-13: AvailableVariantTheme (AvailableVariantTheme.cs): a VARIANTTHEME record (name, display name, colours) and the parts that
// use it. PartLoader.CompileVariantThemes / AddVariants (which build the theme list from the GameDatabase and the variants'
// themeName) are part of the PartLoader data side that WP-05 left uncompiled (STATUS.md WP-05).
#pragma once

#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"

namespace ksp {

struct AvailablePart;

struct AvailableVariantTheme {
    std::string name, displayName, description, primaryColor, secondaryColor;  // [Persistent]
    std::vector<AvailablePart*> parts;                                         // [Persistent] List<AvailablePart>

    // :26 CreateVariantTheme: every text is Localizer.Format("#autoLOC_168872"), colours "#ffffff" / "#f0f0f0"
    static AvailableVariantTheme CreateVariantTheme();
    void Load(const ConfigNode& node);   // :39 (TryGetValue: missing values keep the current ones)
    void Save(ConfigNode& node) const;   // :48
};

}  // namespace ksp
