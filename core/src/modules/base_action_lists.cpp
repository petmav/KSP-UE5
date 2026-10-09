// BaseAction.cs statics over parts (WP-16): GetGroupIndex, CreateActionList, CreateGroupList, ContainsNonDefaultActions, FireAction.
// The declarations are in modules/base_event.hpp (class BaseAction); WP-06 ported the instance side in base_event.cpp.
#include "ksp/modules/base_event.hpp"

#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {

// BaseAction.cs:217 AddAction (private static)
void AddAction(std::vector<BaseAction*>& actionList, int64_t group, int overrideGroup, bool overrideDefault, bool include, BaseAction* action) {
    if (action->active && (GameSettings::ADVANCED_TWEAKABLES || !action->advancedTweakable)) {
        bool flag = false;
        if (overrideGroup > 0 && overrideGroup <= static_cast<int>(action->overrideGroups.size()))
            flag = (action->overrideGroups[static_cast<size_t>(overrideGroup - 1)] & group) != 0;
        if (overrideGroup == 0 || (!overrideDefault && include)) flag |= (action->actionGroup & group) != 0;
        if (include == flag) actionList.push_back(action);
    }
}

}  // namespace

// BaseAction.cs:394
int BaseAction::GetGroupIndex(int64_t group) {
    for (int num = 0; num < ActionGroupsLength(); num++)
        if ((int64_t{1} << num) == group) return num;
    return 0;
}

// BaseAction.cs:205
std::vector<BaseAction*> BaseAction::CreateActionList(const std::vector<Part*>& parts, int64_t group, int overrideGroup, bool overrideDefault, bool include) {
    std::vector<BaseAction*> list;
    const size_t count = parts.size();
    for (size_t i = 0; i < count; i++) {
        std::vector<BaseAction*> sub = CreateActionList(*parts[i], group, overrideGroup, overrideDefault, include);
        list.insert(list.end(), sub.begin(), sub.end());
    }
    return list;
}

// BaseAction.cs:236 (the Part's own p.Actions list is not modelled: its three [KSPAction]s only appear once a craft assigns them)
std::vector<BaseAction*> BaseAction::CreateActionList(Part& p, int64_t group, int overrideGroup, bool overrideDefault, bool include) {
    std::vector<BaseAction*> list;
    const size_t count2 = p.Modules().Count();
    for (size_t j = 0; j < count2; j++) {
        PartModule* partModule = p.Modules()[j];
        if (partModule->isEnabled) {
            const size_t count3 = partModule->Actions().Count();
            for (size_t k = 0; k < count3; k++) AddAction(list, group, overrideGroup, overrideDefault, include, partModule->Actions()[k]);
        }
    }
    return list;
}

// BaseAction.cs:267 (sic: only Part.Actions is searched, not the modules'; Part.Actions is not modelled, so every flag is false)
std::vector<bool> BaseAction::CreateGroupList(Part& /*p*/, int /*overrideGroup*/) {
    const int num = ActionGroupsLength();
    return std::vector<bool>(static_cast<size_t>(num), false);
}

// BaseAction.cs:308
std::vector<bool> BaseAction::CreateGroupList(const std::vector<Part*>& parts, int groupOverride) {
    const int num = ActionGroupsLength();
    std::vector<bool> list;
    list.reserve(static_cast<size_t>(num));
    for (int i = 0; i < num; i++) {
        bool flag = false;
        const int64_t group = int64_t{1} << i;
        const size_t count = parts.size();
        for (size_t j = 0; j < count; j++) {
            Part* part = parts[j];
            // part.Actions.Contains(group, groupOverride) is false (not modelled), then the modules
            const size_t count2 = part->Modules().Count();
            for (size_t k = 0; k < count2; k++) {
                if (part->Modules()[k]->Actions().Contains(group, groupOverride)) {
                    flag = true;
                    break;
                }
            }
            if (flag) break;
        }
        list.push_back(flag);
    }
    return list;
}

// BaseAction.cs:346
bool BaseAction::ContainsNonDefaultActions(Part& p) {
    const size_t count2 = p.Modules().Count();
    for (size_t i = 0; i < count2; i++) {
        PartModule* partModule = p.Modules()[i];
        const size_t count3 = partModule->Actions().Count();
        for (size_t j = 0; j < count3; j++)
            if (partModule->Actions()[j]->ContainsNonDefaultActions()) return true;
    }
    return false;
}

// BaseAction.cs:379
void BaseAction::FireAction(const std::vector<Part*>& parts, int64_t group, int overrideGroup, KSPActionType type, bool fullOnlyLocked, bool editorScene) {
    KSPActionParam param(group, type);
    std::vector<BaseAction*> list = CreateActionList(parts, group, overrideGroup, false, true);
    const size_t count = list.size();
    for (size_t i = 0; i < count; i++) {
        BaseAction* baseAction = list[i];
        if (baseAction->active && (!baseAction->requireFullControl || !fullOnlyLocked) && (baseAction->activeEditor || !editorScene))
            baseAction->Invoke(param);
    }
}

}  // namespace ksp
