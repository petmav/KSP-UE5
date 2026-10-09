// WP-13: AxisGroupsManager (AxisGroupsManager.cs): the AXISGROUPS node of a PartModule and the conversion of its
// [KSPAxisField] members into BaseAxisFields. KSPAxisGroup.cs / KSPAxisMode.cs are enums (save/save_enums.hpp,
// parts/part_types.hpp); AxisFieldLimit / IAxisFieldLimits are in parts/part_types.hpp.
#pragma once

#include "ksp/config/config_node.hpp"

namespace ksp {

class PartModule;

class AxisGroupsManager {
public:
    // AxisGroupsManager.cs:7: every field whose attribute is a KSPAxisField becomes a BaseAxisField (BaseFieldList's
    // constructor already creates them, so this finds nothing left to convert; it is idempotent)
    static void BuildBaseAxisFields(PartModule& pm);
    // :19 LoadAxisFieldNodes: one child node per BaseAxisField of the AXISGROUPS node
    static void LoadAxisFieldNodes(PartModule& pm, const ConfigNode& node);
    // :36 SaveAxisFieldNodes: an AXISGROUPS node is added only when the module has a BaseAxisField
    static void SaveAxisFieldNodes(PartModule& pm, ConfigNode& node);
};

}  // namespace ksp
