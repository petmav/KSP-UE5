// ActionGroupList.cs (see include/ksp/vessel/action_group_list.hpp)
#include "ksp/vessel/action_group_list.hpp"

#include <algorithm>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {
// BaseAction.GetGroupIndex (BaseAction.cs:45): the bit position of a single flag
int GroupIndex(KSPActionGroup group) {
    int v = static_cast<int>(group);
    int i = 0;
    while (v > 1) {
        v >>= 1;
        i++;
    }
    return i;
}
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
