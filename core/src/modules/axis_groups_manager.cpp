// WP-13: AxisGroupsManager (AxisGroupsManager.cs).
#include "ksp/modules/axis_groups_manager.hpp"

#include "ksp/modules/base_field.hpp"
#include "ksp/modules/part_module.hpp"

namespace ksp {

// AxisGroupsManager.cs:7
void AxisGroupsManager::BuildBaseAxisFields(PartModule& pm) {
    int count = static_cast<int>(pm.Fields().Count());
    while (count-- > 0) {
        BaseField* baseField = pm.Fields()[static_cast<size_t>(count)];
        if (baseField->Attribute().isAxis && dynamic_cast<BaseAxisField*>(baseField) == nullptr) {
            pm.Fields().Set(static_cast<size_t>(count),
                            std::make_shared<BaseAxisField>(baseField->Attribute(), baseField->FieldInfo(), baseField->host(),
                                                            static_cast<size_t>(count)));
        }
    }
}

// AxisGroupsManager.cs:19
void AxisGroupsManager::LoadAxisFieldNodes(PartModule& pm, const ConfigNode& node) {
    ConfigNode* node2 = node.GetNode("AXISGROUPS");
    if (node2 == nullptr) return;
    for (int i = 0; i < node2->nodes().Count(); i++) {
        ConfigNode& configNode = *node2->nodes().Shared(i);
        if (auto* baseAxisField = dynamic_cast<BaseAxisField*>(pm.Fields()[configNode.name])) baseAxisField->OnLoad(configNode);
    }
}

// AxisGroupsManager.cs:36
void AxisGroupsManager::SaveAxisFieldNodes(PartModule& pm, ConfigNode& node) {
    bool flag = false;
    int count = static_cast<int>(pm.Fields().Count());
    while (count-- > 0) {
        if (dynamic_cast<BaseAxisField*>(pm.Fields()[static_cast<size_t>(count)]) != nullptr) {
            flag = true;
            break;
        }
    }
    if (!flag) return;
    ConfigNode* configNode = node.AddNode("AXISGROUPS");
    for (size_t i = 0; i < pm.Fields().Count(); i++)
        if (auto* a = dynamic_cast<BaseAxisField*>(pm.Fields()[i])) a->OnSave(*configNode->AddNode(a->name()));
}

}  // namespace ksp
