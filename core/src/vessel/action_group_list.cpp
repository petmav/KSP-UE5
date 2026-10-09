// ActionGroupList.cs (see include/ksp/vessel/action_group_list.hpp)
#include "ksp/vessel/action_group_list.hpp"

#include <algorithm>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {
int GroupIndex(KSPActionGroup group) { return BaseAction::GetGroupIndex(static_cast<int64_t>(static_cast<int>(group))); }  // BaseAction.cs:394
}  // namespace

// ActionGroupList.cs:24
ActionGroupList::ActionGroupList(Vessel* vessel) : v(vessel) {
    const int actionGroupsLength = BaseAction::ActionGroupsLength();
    groupStates.assign(static_cast<size_t>(Vessel::NumOverrideGroups), std::vector<bool>(static_cast<size_t>(actionGroupsLength), false));
    cooldownTimes.assign(static_cast<size_t>(actionGroupsLength), 0.0);
    groups.assign(static_cast<size_t>(actionGroupsLength), false);
}

bool ActionGroupList::operator[](KSPActionGroup group) const { return groups.at(static_cast<size_t>(GroupIndex(group))); }
void ActionGroupList::Set(KSPActionGroup group, bool value) { groups.at(static_cast<size_t>(GroupIndex(group))) = value; }

// ActionGroupList.cs:50
void ActionGroupList::ToggleGroup(KSPActionGroup group) {
    const int groupIndex = GroupIndex(group);
    if (Planetarium::GetUniversalTime() < cooldownTimes[static_cast<size_t>(groupIndex)]) {
        log::Info("Cannot toggle " + dotnet::EnumToString(group) + ". " +
                  dotnet::FormatDoubleF(cooldownTimes[static_cast<size_t>(groupIndex)] - Planetarium::GetUniversalTime(), 1) +
                  " seconds until it becomes available again.");
        return;
    }
    std::vector<bool>* list = &groups;
    int num = 0;
    bool flag = false;
    bool flag2 = false;
    if (GameSettings::ADDITIONAL_ACTION_GROUPS) {
        num = v->GroupOverride;
        if (num > 0 && num <= static_cast<int>(v->OverrideDefault.size())) {
            flag = v->OverrideDefault[static_cast<size_t>(num - 1)];
            flag2 = (static_cast<int>(v->OverrideActionControl[static_cast<size_t>(num - 1)]) & static_cast<int>(group)) != 0;
            list = &groupStates[static_cast<size_t>(num - 1)];
        }
    }
    const size_t gi = static_cast<size_t>(groupIndex);
    (*list)[gi] = !(*list)[gi];
    if (num > 0 && !(flag || flag2)) groups[gi] = !groups[gi];
    KSPActionParam kSPActionParam(static_cast<int64_t>(static_cast<int>(group)), (*list)[gi] ? KSPActionType::Activate : KSPActionType::Deactivate);
    std::vector<BaseAction*> list2 = BaseAction::CreateActionList(v->parts, kSPActionParam.group, num, flag, true);
    const size_t count = list2.size();
    for (size_t i = 0; i < count; i++) list2[i]->Invoke(kSPActionParam);
    cooldownTimes[gi] = Planetarium::GetUniversalTime() + static_cast<double>(kSPActionParam.Cooldown());
}

// ActionGroupList.cs:87
void ActionGroupList::SetGroup(KSPActionGroup group, bool active) {
    const int groupIndex = GroupIndex(group);
    if (groups[static_cast<size_t>(groupIndex)] != active) ToggleGroup(group);
}

// ActionGroupList.cs:96
void ActionGroupList::Load(const ConfigNode& node) {
    const int actionGroupsLength = BaseAction::ActionGroupsLength();
    for (int i = 0; i < actionGroupsLength; i++) {
        const std::string name = dotnet::EnumToString(static_cast<KSPActionGroup>(1 << i));
        std::optional<std::string> value = node.GetValue(name);
        if (!value) continue;
        std::vector<std::string> array = dotnet::Split(*value, ",", false);
        if (array.size() == 2) {
            groups[static_cast<size_t>(i)] = cs::BoolParse(dotnet::Trim(array[0]));
            cooldownTimes[static_cast<size_t>(i)] = dotnet::ParseDouble(dotnet::Trim(array[1]));
        } else {
            groups[static_cast<size_t>(i)] = cs::BoolParse(*value);
        }
    }
}

// ActionGroupList.cs:118 (`groups[i] + ", " + cooldownTimes[i]`: bool.ToString() "True"/"False", double.ToString())
void ActionGroupList::Save(ConfigNode& node) const {
    const int actionGroupsLength = BaseAction::ActionGroupsLength();
    for (int i = 0; i < actionGroupsLength; i++)
        node.AddValue(dotnet::EnumToString(static_cast<KSPActionGroup>(1 << i)),
                      dotnet::FormatBool(groups[static_cast<size_t>(i)]) + ", " + dotnet::FormatDoubleG(cooldownTimes[static_cast<size_t>(i)]));
}

// ActionGroupList.cs:127
double ActionGroupList::CooldownTimeRemaining(KSPActionGroup group) const {
    const int groupIndex = GroupIndex(group);
    if (Planetarium::GetUniversalTime() < cooldownTimes[static_cast<size_t>(groupIndex)])
        return cooldownTimes[static_cast<size_t>(groupIndex)] - Planetarium::GetUniversalTime();
    return 0.0;
}

// ActionGroupList.cs:137
void ActionGroupList::CopyFrom(const ActionGroupList& acl) {
    const size_t count = groups.size();
    for (size_t i = 0; i < count; i++) {
        groups[i] = acl.groups[i];
        cooldownTimes[i] = acl.cooldownTimes[i];
    }
}

}  // namespace ksp
