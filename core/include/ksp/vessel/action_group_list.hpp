// ActionGroupList.cs (146 lines). WP-16 (staging, action groups) owns the class; WP-14 ported the part the vessel model and the
// save need (the fields, the constructor, the indexer, Load / Save / CopyFrom / CooldownTimeRemaining); WP-16 added ToggleGroup /
// SetGroup (BaseAction.CreateActionList over the vessel's parts, src/modules/base_action_lists.cpp).
#pragma once

#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/save/save_enums.hpp"

namespace ksp {

class Vessel;

class ActionGroupList {
public:
    std::vector<std::vector<bool>> groupStates;  // List<bool>[Vessel.NumOverrideGroups]
    std::vector<bool> groups;
    std::vector<double> cooldownTimes;
    Vessel* v = nullptr;

    explicit ActionGroupList(Vessel* vessel);  // ActionGroupList.cs:24

    // ActionGroupList.cs:12 (BaseAction.GetGroupIndex: log2 of the flag)
    bool operator[](KSPActionGroup group) const;
    void Set(KSPActionGroup group, bool value);

    void Load(const ConfigNode& node);  // ActionGroupList.cs:96
    void Save(ConfigNode& node) const;  // ActionGroupList.cs:118
    double CooldownTimeRemaining(KSPActionGroup group) const;  // ActionGroupList.cs:127
    void CopyFrom(const ActionGroupList& acl);                 // ActionGroupList.cs:137

    // ActionGroupList.cs:50 (WP-16): flips a group (with the ADDITIONAL_ACTION_GROUPS override-group bookkeeping), invokes every
    // matching action of the vessel's parts with a KSPActionParam(group, Activate|Deactivate) and starts the cooldown the actions
    // requested. A group still on cooldown only logs.
    void ToggleGroup(KSPActionGroup group);
    // ActionGroupList.cs:87: toggles only when the group's current state differs
    void SetGroup(KSPActionGroup group, bool active);
};

}  // namespace ksp
