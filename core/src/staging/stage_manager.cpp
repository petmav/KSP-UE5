// WP-16: StageManager.cs staging model (see include/ksp/staging/stage_manager.hpp). Every function cites
// KSP/UI/Screens/StageManager.cs:<line> (decompiled/KSP-Decompiled/Assembly-CSharp/) unless it names another file.
#include "ksp/staging/stage_manager.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/vessel/flight_logger.hpp"
#include "ksp/vessel/vessel.hpp"

// int.CompareTo(int): -1 / 0 / 1
static inline int IntCompareTo(int a, int b) { return a < b ? -1 : (a > b ? 1 : 0); }

namespace ksp {

using GE = GameEvents;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
int ClampMin0(int v) { return v < 0 ? 0 : v; }  // Mathf.Clamp(v, 0, int.MaxValue)

template <class T>
bool EraseOne(std::vector<T>& v, const T& x) {  // List<T>.Remove
    auto it = std::find(v.begin(), v.end(), x);
    if (it == v.end()) return false;
    v.erase(it);
    return true;
}
template <class T>
bool Contains(const std::vector<T>& v, const T& x) { return std::find(v.begin(), v.end(), x) != v.end(); }

bool HasModuleClass(Part& p, std::string_view cls) {
    return p.FindModuleImplementing([cls](PartModule& pm) { return pm.Class().Is(cls); }) != nullptr;
}

// part.FindModulesImplementing<IStageSeparator>()
std::vector<IStageSeparator*> FindStageSeparators(Part& p) {
    std::vector<IStageSeparator*> out;
    for (size_t i = 0; i < p.Modules().Count(); i++)
        if (auto* s = dynamic_cast<IStageSeparator*>(p.Modules()[i])) out.push_back(s);
    return out;
}
IStageSeparatorChild* FindStageSeparatorChild(Part& p) {
    for (size_t i = 0; i < p.Modules().Count(); i++)
        if (auto* s = dynamic_cast<IStageSeparatorChild*>(p.Modules()[i])) return s;
    return nullptr;
}
}  // namespace

// ============================================================================================ hosts

Part* FlightStagingHost::RootPart() { return FlightRegistry::ActiveVessel != nullptr ? FlightRegistry::ActiveVessel->rootPart : nullptr; }
Vessel* FlightStagingHost::ActiveVessel() { return FlightRegistry::ActiveVessel; }

// ============================================================================================ StageIcon (StageIcon.cs)

// StageIcon.cs:136
bool StageIcon::isMainIcon() const {
    if (groupLead != this) {
        if (groupLead == nullptr) return stage != nullptr;
        return false;
    }
    return true;
}

// StageIcon.cs:152
bool StageIcon::isSymmetryCounterPart() const { return part != nullptr && !part->symmetryCounterparts.empty(); }
// StageIcon.cs:164
int StageIcon::InverseStage() const { return part != nullptr ? part->fields.inverseStage : 0; }
// StageIcon.cs:176
int StageIcon::InStageIndex() const { return part != nullptr ? part->fields.inStageIndex : -1; }
// StageIcon.cs:188
int StageIcon::DefaultSequenceIndex() const { return part != nullptr ? part->fields.defaultInverseStage : 0; }
// StageIcon.cs:200
int StageIcon::OriginalStageIndex() const { return part != nullptr ? part->fields.originalStage : 0; }

// StageIcon.cs:236 (Part.cs:195 `stackIconGrouping = SYM_COUNTERPARTS`; ModuleRCS.cs:436 sets SAME_TYPE in its OnStart)
StackIconGrouping StageIcon::IconGroupingRule() const {
    if (part == nullptr) return StackIconGrouping::NONE;
    if (HasModuleClass(*part, "ModuleRCS")) return StackIconGrouping::SAME_TYPE;
    return StackIconGrouping::SYM_COUNTERPARTS;
}

// StageIcon.cs:771
void StageIcon::SetInverseSequenceIndex(int inverseIndex, int inStageIndex, bool seqOverride) {
    if (part == nullptr) return;
    part->fields.inverseStage = inverseIndex;
    if (seqOverride) {
        part->fields.manualStageOffset = inverseIndex;
        if (HasModuleClass(*part, "ModuleDecouple") || HasModuleClass(*part, "ModuleAnchoredDecoupler") || part->isEngine()) {
            part->fields.separationIndex = inverseIndex;
            StageManager::SetSeparationIndices();
        }
    }
    if (groupLead != nullptr && groupLead == this) {
        for (size_t i = 0; i < groupedIcons.size(); i++) {
            groupedIcons[i]->part->fields.inverseStage = inverseIndex;
            if (seqOverride) groupedIcons[i]->part->fields.manualStageOffset = inverseIndex;
        }
    }
    if (stage != nullptr) SetInStageIndex(inStageIndex);
}

// StageIcon.cs:810
void StageIcon::SetManualStageOffset(int inverseIndex) {
    if (part == nullptr) return;
    part->fields.manualStageOffset = inverseIndex;
    if (groupLead != nullptr && groupLead == this)
        for (size_t i = 0; i < groupedIcons.size(); i++) groupedIcons[i]->part->fields.manualStageOffset = inverseIndex;
}

// StageIcon.cs:832
void StageIcon::SetInStageIndex(int index) { part->fields.inStageIndex = index; }

// StageIcon.cs:837 (the UI sibling reparenting is not modelled)
void StageIcon::SetInStageIndexOFGroupedIcons(bool /*setSiblingIndex*/) {
    StageGroup* stageGroup = groupLead != nullptr ? groupLead->stage : stage;
    if (stageGroup == nullptr) return;
    int count = static_cast<int>(groupedIcons.size());
    while (count-- > 0) groupedIcons[static_cast<size_t>(count)]->SetInStageIndex(InStageIndex());
}

// StageIcon.cs:857 (List.Sort is the unstable introsort, ported as dotnet::ListSort; the keys are distinct child indices)
void StageIcon::SortGroupedIcons() {
    StageGroup* stageGroup = groupLead != nullptr ? groupLead->stage : stage;
    if (stageGroup == nullptr) return;
    auto idx = [](StageIcon* a) {
        const auto& ch = a->part->parent->children;
        return static_cast<int>(std::find(ch.begin(), ch.end(), a->part) - ch.begin());
    };
    // StageIcon.cs:863: a.Part.parent.children.IndexOf(a.Part).CompareTo(b...)
    dotnet::ListSort(groupedIcons, [&](StageIcon* a, StageIcon* b) { return IntCompareTo(idx(a), idx(b)); });
    int count = static_cast<int>(groupedIcons.size());
    while (count-- > 0) groupedIcons[static_cast<size_t>(count)]->SetInStageIndex(InStageIndex());
}

// StageIcon.cs:889
void StageIcon::Reset() {
    if (part == nullptr) return;
    if (groupLead != nullptr) groupLead->ResetStageGroup();
    else if (groupLead == nullptr && stage != nullptr) ResetStageGroup();
}

// StageIcon.cs:904
void StageIcon::ResetStageGroup() {
    if (groupLead == this || (groupLead == nullptr && stage != nullptr)) {
        EraseOne(StageManager::Selection(), this);
        const int inverseStage = part->fields.inverseStage;
        part->fields.inverseStage = part->fields.defaultInverseStage;
        if (inverseStage != part->fields.inverseStage) GE::onPartPriorityChanged.Fire(part);
        part->fields.manualStageOffset = part->fields.defaultInverseStage;
        if (!frozen) {
            if (stage != nullptr) stage->RemoveIcon(this);
            StageManager::Instance->HoldIcon(this, false);
        }
        for (size_t i = 0, num = groupedIcons.size(); i < num; i++) {
            if (groupedIcons[i] == nullptr) {
                groupedIcons.erase(groupedIcons.begin() + static_cast<std::ptrdiff_t>(i--));
                num--;
                continue;
            }
            groupedIcons[i]->active = true;
            EraseOne(StageManager::Selection(), groupedIcons[i]);
            if (!groupedIcons[i]->frozen) StageManager::Instance->HoldIcon(groupedIcons[i], false);
            groupedIcons[i]->part->fields.inverseStage = groupedIcons[i]->part->fields.defaultInverseStage;
            groupedIcons[i]->part->fields.manualStageOffset = groupedIcons[i]->part->fields.defaultInverseStage;
            groupedIcons[i]->groupLead = nullptr;
            groupedIcons[i]->stage = nullptr;
            groupedIcons[i]->grouped = false;
            groupedIcons[i]->expanded = false;
        }
        groupedIcons.clear();
        groupLead = nullptr;
        stage = nullptr;
        grouped = false;
        expanded = false;
    }
}

// StageIcon.cs:961
bool StageIcon::ResetAvailable() const {
    if (part == nullptr) return false;
    if (groupLead != nullptr) return groupLead->ResetStageGroupAvailable();
    if (groupLead == nullptr && stage != nullptr) return ResetStageGroupAvailable();
    return false;
}

// StageIcon.cs:978
bool StageIcon::ResetStageGroupAvailable() const {
    if (groupLead != this && (groupLead != nullptr || stage == nullptr)) return false;
    bool result = false;
    if (part->fields.manualStageOffset != -1 && part->fields.inverseStage != part->fields.defaultInverseStage) result = true;
    for (size_t i = 0; i < groupedIcons.size(); i++)
        if (groupedIcons[i]->part->fields.manualStageOffset != -1 && groupedIcons[i]->part->fields.inverseStage != groupedIcons[i]->part->fields.defaultInverseStage)
            result = true;
    return result;
}

// StageIcon.cs:1000
void StageIcon::AddToGroup(StageIcon* icon, bool /*setParent*/) {
    if (!Contains(groupedIcons, icon)) {
        groupedIcons.push_back(icon);
        icon->grouped = true;
        icon->groupLead = this;
        grouped = true;
        groupLead = this;
        if (stage == nullptr) throw Nre();
        const int inverseStage = icon->part->fields.inverseStage;
        icon->part->fields.inverseStage = stage->inverseStageIndex;
        if (inverseStage != stage->inverseStageIndex) GE::onPartPriorityChanged.Fire(icon->part);
        icon->SetInStageIndex(InStageIndex());
        icon->part->fields.manualStageOffset = stage->inverseStageIndex;
    }
}

// StageIcon.cs:1028
void StageIcon::RemoveFromGroup(StageIcon* icon, bool resetGroupLeadIfempty) {
    if (groupLead != icon) {
        if (Contains(groupedIcons, icon)) EraseOne(groupedIcons, icon);
        if (groupedIcons.empty() && resetGroupLeadIfempty) {
            grouped = false;
            groupLead = nullptr;
        }
        icon->grouped = false;
        icon->groupLead = nullptr;
    }
}

// StageIcon.cs:1049
bool StageIcon::RemoveFromGroupAndReshuffle(StageGroup*& foundInGroup) {
    bool result = false;
    StageIcon* stageIcon = nullptr;
    foundInGroup = nullptr;
    if (groupLead == this && !groupedIcons.empty()) {
        foundInGroup = stage;
        stageIcon = groupedIcons[0];
        stageIcon->groupLead = nullptr;
        foundInGroup->RemoveIcon(this);
        foundInGroup->AddIconAt(stageIcon, InStageIndex(), SiblingIndex, false);
        std::vector<StageIcon*> list(groupedIcons);
        for (size_t i = 1; i < list.size(); i++) {
            StageIcon* stageIcon2 = list[i];
            RemoveFromGroup(stageIcon2);
            stageIcon->AddToGroup(stageIcon2);
        }
        result = true;
    } else if (groupLead != nullptr) {
        foundInGroup = groupLead->stage;
        stageIcon = groupLead;
        groupLead->RemoveFromGroup(this);
        result = true;
    }
    if (stageIcon != nullptr) stageIcon->SetInStageIndexOFGroupedIcons(false);
    if (foundInGroup == nullptr) {
        std::vector<StageGroup*> st = StageManager::Instance->Stages();
        for (size_t j = 0; j < st.size(); j++) {
            if (Contains(st[j]->Icons(), this)) {
                foundInGroup = st[j];
                foundInGroup->RemoveIcon(this, false);
                result = true;
                break;
            }
        }
    }
    return result;
}

// StageIcon.cs:1100
void StageIcon::HoldGroupedIcons(bool alertStagingSequencer) {
    if (groupLead != this) return;
    for (size_t i = 0, num = groupedIcons.size(); i < num; i++) {
        if (groupedIcons[i] == nullptr) {
            groupedIcons.erase(groupedIcons.begin() + static_cast<std::ptrdiff_t>(i--));
            num--;
            continue;
        }
        groupedIcons[i]->active = true;
        groupedIcons[i]->grouped = false;
        groupedIcons[i]->groupLead = nullptr;
        groupedIcons[i]->expanded = false;
        EraseOne(StageManager::Selection(), groupedIcons[i]);
        if (!groupedIcons[i]->frozen) StageManager::Instance->HoldIcon(groupedIcons[i], true, alertStagingSequencer);
    }
    groupedIcons.clear();
    grouped = false;
    groupLead = nullptr;
    expanded = false;
    EraseOne(StageManager::Selection(), this);
    if (!frozen) {
        if (stage != nullptr) stage->RemoveIcon(this);
        StageManager::Instance->HoldIcon(this, true, alertStagingSequencer);
    }
}

// StageIcon.cs:1145
void StageIcon::ExpandGroup() {
    if (groupLead != this) return;
    StageManager::Selection().push_back(this);
    for (size_t i = 0, num = groupedIcons.size(); i < num; i++) {
        if (groupedIcons[i] == nullptr) {
            groupedIcons.erase(groupedIcons.begin() + static_cast<std::ptrdiff_t>(i--));
            num--;
            continue;
        }
        groupedIcons[i]->active = true;
        groupedIcons[i]->expanded = true;
        StageManager::Selection().push_back(groupedIcons[i]);
    }
    expanded = true;
}

// StageIcon.cs:1195
void StageIcon::CollapseGroup() {
    if (groupLead != this) return;
    expanded = false;
    EraseOne(StageManager::Selection(), this);
    for (size_t i = 0, num = groupedIcons.size(); i < num; i++) {
        if (groupedIcons[i] == nullptr) {
            groupedIcons.erase(groupedIcons.begin() + static_cast<std::ptrdiff_t>(i--));
            num--;
            continue;
        }
        groupedIcons[i]->active = false;
        groupedIcons[i]->expanded = false;
        EraseOne(StageManager::Selection(), groupedIcons[i]);
    }
}

// StageIcon.cs:1222
void StageIcon::DisableGroup(StageIcon* iconToDisable) {
    if (iconToDisable->groupLead == iconToDisable) {
        StageGroup* stageGroup = iconToDisable->stage;
        int num = iconToDisable->InStageIndex();
        const int last = static_cast<int>(stageGroup->Icons().size()) - 1;
        if (num < 0 || num > last) num = std::min(std::max(num, 0), std::max(last, 0));  // Mathf.Clamp(num, 0, Count - 1)
        stageGroup->RemoveIcon(iconToDisable, false);
        for (size_t i = 0; i < iconToDisable->groupedIcons.size(); i++) {
            if (iconToDisable->groupedIcons[i] == nullptr) {
                log::Error("[StageIcon] DisableGroup: Null icon in grouped icons.");
                iconToDisable->groupedIcons.erase(iconToDisable->groupedIcons.begin() + static_cast<std::ptrdiff_t>(i--));
            } else if (i == 0) {
                stageGroup->AddIconAt(iconToDisable->groupedIcons[0], num, num);
                iconToDisable->groupedIcons[0]->grouped = false;
                iconToDisable->groupedIcons[0]->groupLead = nullptr;
            } else {
                iconToDisable->groupedIcons[0]->AddToGroup(iconToDisable->groupedIcons[i], false);
            }
        }
    } else {
        iconToDisable->groupLead->RemoveFromGroup(iconToDisable);
    }
    iconToDisable->active = false;
    iconToDisable->grouped = false;
    iconToDisable->groupLead = nullptr;
    EraseOne(StageManager::Selection(), iconToDisable);
}

// StageIcon.cs:1264
void StageIcon::ConsolidateMembers() {
    if (groupLead != this) {
        groupLead = nullptr;
        grouped = false;
        return;
    }
    for (size_t i = 0, count = groupedIcons.size(); i < count; i++) {
        if (groupedIcons[i]->groupLead != this) {
            groupedIcons.erase(groupedIcons.begin() + static_cast<std::ptrdiff_t>(i));
            count = groupedIcons.size();
        }
    }
    if (groupedIcons.empty()) {
        groupLead = nullptr;
        grouped = false;
    }
}

// ============================================================================================ StageGroup (StageGroup.cs)

// StageGroup.cs:542
void StageGroup::AddIcons(const std::vector<StageIcon*>& iconsToAdd) {
    for (size_t i = 0; i < iconsToAdd.size(); i++) AddIcon(iconsToAdd[i]);
}

// StageGroup.cs:551
void StageGroup::AddIcon(StageIcon* icon, bool /*setParent*/) {
    if (!Contains(icons, icon)) {
        icons.push_back(icon);
        icon->stage = this;
        icon->SetInverseSequenceIndex(inverseStageIndex, static_cast<int>(icons.size()) - 1);
        icon->SetInStageIndex(static_cast<int>(icons.size()) - 1);
    }
}

// StageGroup.cs:570
void StageGroup::AddIconAt(StageIcon* icon, int index, int /*forcedSiblingIndex*/, bool /*setParent*/) {
    if (!Contains(icons, icon)) {
        if (index < 0 || index > static_cast<int>(icons.size())) throw std::out_of_range("Index was out of range");  // List.Insert
        icons.insert(icons.begin() + index, icon);
        icon->active = true;
        icon->stage = this;
        icon->SetInverseSequenceIndex(inverseStageIndex, index);
        icon->SetInStageIndex(index);
    } else {
        log::Warning("[StageGroup] AddIconAt(" + std::to_string(index) + ") failed (already in group) ->");
    }
}

// StageGroup.cs:602
void StageGroup::RemoveIcon(StageIcon* icon, bool /*changeParents*/) {
    if (Contains(icons, icon)) {
        EraseOne(icons, icon);
        icon->stage = nullptr;
    } else {
        log::Warning("[StageGroup] RemoveIcon() failed (not in group) ->");
    }
}

// StageGroup.cs:619
void StageGroup::Delete() {
    if (icons.empty()) {
        StageManager::Instance->DecrementCurrentStage();
        const int num = inverseStageIndex;
        StageManager::Instance->DeleteStage(this, StageManager::Instance->Visible);
        StageManager::Instance->SetManualStageOffset(num);
        StageManager::SetSeparationIndices();
        GE::StageManager::OnGUIStageRemoved.Fire(num);
    }
}

// StageGroup.cs:632
void StageGroup::AddStageAfter() {
    StageManager::Instance->IncrementCurrentStage();
    const int data = inverseStageIndex + 1;
    StageManager::Instance->AddStageAt(inverseStageIndex + 1);
    StageManager::Instance->SetManualStageOffset(inverseStageIndex + 1);
    StageManager::SetSeparationIndices();
    GE::StageManager::OnGUIStageAdded.Fire(data);
}

// StageGroup.cs:642
void StageGroup::UpdateInStageIndexes() {
    for (size_t i = 0; i < icons.size(); i++) {
        icons[i]->SetInStageIndex(static_cast<int>(i));
        icons[i]->SetInStageIndexOFGroupedIcons();
    }
}

// StageGroup.cs:657
void StageGroup::SetPartIndices(bool seqOverride) {
    for (size_t i = 0; i < icons.size(); i++) icons[i]->SetInverseSequenceIndex(inverseStageIndex, static_cast<int>(i), seqOverride);
}

// StageGroup.cs:666
void StageGroup::SetManualStageOffset() {
    for (size_t i = 0; i < icons.size(); i++) icons[i]->SetManualStageOffset(inverseStageIndex);
}

// StageGroup.cs:675
void StageGroup::SetSiblingIndexes() {
    int num = 0;
    for (size_t i = 0; i < icons.size(); i++) {
        StageIcon* stageIcon = icons[i];
        stageIcon->SiblingIndex = num++;
        if (stageIcon->groupLead != nullptr && stageIcon->groupLead == stageIcon)
            for (size_t j = 0; j < stageIcon->groupedIcons.size(); j++) stageIcon->groupedIcons[j]->SiblingIndex = num++;
    }
}

// StageGroup.cs:694
void StageGroup::Reset() {
    std::vector<StageIcon*> list(icons);
    for (size_t i = 0; i < list.size(); i++) list[i]->Reset();
    icons.clear();
}

// StageGroup.cs:705
bool StageGroup::ResetAvailable() const {
    for (size_t i = 0; i < icons.size(); i++)
        if (icons[i]->ResetAvailable()) return true;
    return false;
}

// StageGroup.cs:793
StageIcon* StageGroup::FindSymmetryGroupleader(StageIcon* icon) {
    for (size_t num = 0; num < icons.size(); num++) {
        if (icons[num]->IconGroupingRule() == icon->IconGroupingRule()) {
            const StackIconGrouping rule = icons[num]->IconGroupingRule();
            if (rule == StackIconGrouping::SYM_COUNTERPARTS && icons[num]->part->isSymmetryCounterPart(icon->part)) return icons[num];
        }
    }
    return nullptr;
}

// StageGroup.cs:817
void StageGroup::MoveAllIconsInto(StageGroup* moveInto) {
    int count = static_cast<int>(icons.size());
    while (count-- > 0) {
        StageIcon* stageIcon = icons[static_cast<size_t>(count)];
        if (stageIcon->groupLead != nullptr && stageIcon->groupLead == stageIcon) {
            std::vector<StageIcon*> list(stageIcon->groupedIcons);
            list.insert(list.begin(), stageIcon);
            moveInto->MoveIconsInto(stageIcon->groupLead->stage, stageIcon, list, nullptr, {}, 0, 0);
        } else if (stageIcon->isSymmetryCounterPart()) {
            moveInto->MoveIconsInto(stageIcon->stage, stageIcon, std::vector<StageIcon*>{stageIcon}, nullptr, {}, 0, 0);
        } else {
            RemoveIcon(stageIcon);
            moveInto->AddIconAt(stageIcon, 0, 0);
        }
    }
}

// StageGroup.cs:841 (radio buttons, sibling indices and the info-box display test are UI; `isDisplayingInfoInGroup` is false)
void StageGroup::MoveIconsInto(StageGroup* oldStageGroup, StageIcon* selectedLeader, std::vector<StageIcon*> selectedIcons,
                               StageIcon* unSelectedLeader, std::vector<StageIcon*> unSelectedIcons, int insertAt, int modifiedSiblingIndex) {
    auto firstOf = [](const std::vector<StageIcon*>& set) -> StageIcon* {
        const auto& children = set[0]->part->parent->children;
        for (Part* a : children) {
            StageIcon* ic = StageManager::Instance->IconOf(a);
            if (ic != nullptr && Contains(set, ic)) return ic;
        }
        throw Nre();  // List.Find returns null, `.stackIcon` throws
    };
    StageIcon* stageIcon = selectedLeader != nullptr ? selectedLeader : firstOf(selectedIcons);
    StageIcon* stageIcon2 = FindSymmetryGroupleader(stageIcon);
    const int inStageIndex = stageIcon->InStageIndex();
    if (selectedLeader == nullptr) {
        if (stageIcon->groupLead != nullptr) stageIcon->groupLead->RemoveFromGroup(stageIcon);
    } else if (selectedLeader->stage != nullptr) {
        selectedLeader->stage->RemoveIcon(selectedLeader, false);
    } else {
        selectedLeader->groupLead->RemoveFromGroup(selectedLeader);
    }
    auto indexOf = [](StageIcon* ic) {
        const auto& ch = ic->part->parent->children;
        return static_cast<int>(std::find(ch.begin(), ch.end(), ic->part) - ch.begin());
    };
    if (stageIcon2 == nullptr) {
        AddIconAt(stageIcon, insertAt, modifiedSiblingIndex);
        for (size_t i = 0; i < selectedIcons.size(); i++) {
            StageIcon* stageIcon3 = selectedIcons[i];
            if (stageIcon3 != stageIcon) {
                if (stageIcon3->groupLead != nullptr) stageIcon3->groupLead->RemoveFromGroup(stageIcon3);
                stageIcon->AddToGroup(stageIcon3);
            }
        }
    } else if (indexOf(stageIcon2) < indexOf(stageIcon)) {
        std::vector<StageIcon*> list;
        list.push_back(stageIcon);
        for (size_t j = 0; j < selectedIcons.size(); j++)
            if (selectedIcons[j] != stageIcon) list.push_back(selectedIcons[j]);
        stageIcon->groupedIcons.clear();
        for (size_t k = 0; k < list.size(); k++) stageIcon2->AddToGroup(list[k]);
    } else {
        AddIconAt(stageIcon, insertAt, modifiedSiblingIndex);
        for (size_t l = 0; l < selectedIcons.size(); l++) {
            StageIcon* stageIcon4 = selectedIcons[l];
            if (stageIcon4 != stageIcon) {
                stageIcon4->groupLead->RemoveFromGroup(stageIcon4);
                stageIcon->AddToGroup(stageIcon4);
            }
        }
        std::vector<StageIcon*> list2;
        list2.push_back(stageIcon2);
        list2.insert(list2.end(), stageIcon2->groupedIcons.begin(), stageIcon2->groupedIcons.end());
        stageIcon2->groupedIcons.clear();
        stageIcon2->stage->RemoveIcon(stageIcon2, false);
        for (size_t m = 0; m < list2.size(); m++) stageIcon->AddToGroup(list2[m]);
        stageIcon2 = stageIcon;
    }
    if (unSelectedLeader != nullptr) {
        unSelectedLeader->ConsolidateMembers();
        unSelectedLeader->CollapseGroup();
    } else if (!unSelectedIcons.empty()) {
        StageIcon* stageIcon5 = firstOf(unSelectedIcons);
        oldStageGroup->AddIconAt(stageIcon5, inStageIndex, 0, false);
        for (size_t n = 0; n < unSelectedIcons.size(); n++) {
            StageIcon* stageIcon6 = unSelectedIcons[n];
            stageIcon6->groupLead->RemoveFromGroup(stageIcon6, false);
            if (stageIcon6 != stageIcon5) stageIcon5->AddToGroup(stageIcon6, false);
        }
        stageIcon5->ConsolidateMembers();
        stageIcon5->CollapseGroup();
        stageIcon5->SetInStageIndexOFGroupedIcons();
    }
    StageIcon* stageIcon7 = stageIcon2 != nullptr ? stageIcon2 : stageIcon;
    stageIcon7->ConsolidateMembers();
    stageIcon7->SortGroupedIcons();
    if (this != oldStageGroup) stageIcon7->SetManualStageOffset(stageIcon7->stage->inverseStageIndex);
}

// ============================================================================================ StageManager

StageManager* StageManager::Instance = nullptr;

StageManager::StageManager(IStagingHost* host) : host_(host) {}

StageManager::~StageManager() {
    if (awake_) {
        // OnDestroy (:230)
        if (Instance == this) Instance = nullptr;
        for (auto& f : subscriptions_) f();
        subscriptions_.clear();
        RemovePartHooks();
    }
}

// PhysicsGlobals.StagingCooldownTimer (PhysicsGlobals.cs:2652); the Instance is absent in tests that never loaded the database
double StageManager::StagingCooldownTimer() {
    return PhysicsGlobals::Instance() != nullptr ? static_cast<double>(PhysicsGlobals::StagingCooldownTimer()) : 0.5;
}

// :184
bool StageManager::Awake() {
    if (Instance != nullptr) {
        log::Error("StageManager: Only one instance of this class may exist per scene. Destroying potential usurper.");
        return false;
    }
    Instance = this;
    awake_ = true;
    Visible = true;
    auto sub = [this](auto& event, auto method) {
        event.Add(this, method);
        subscriptions_.push_back([this, &event, method] { event.Remove(this, method); });
    };
    sub(GE::StageManager::SortIcons, &StageManager::SortIconsEvent);
    sub(GE::onVesselClearStaging, &StageManager::OnVesselClearStaging);
    sub(GE::onVesselResumeStaging, &StageManager::OnVesselResumeStaging);
    sub(GE::onEditorStarted, &StageManager::OnEditorStarted);
    sub(GE::onEditorPodPicked, &StageManager::OnEditorPodSelected);
    sub(GE::onEditorPartPicked, &StageManager::OnEditorPartPicked);
    sub(GE::onEditorPartPlaced, &StageManager::OnEditorPartPlaced);
    sub(GE::onEditorPodDeleted, &StageManager::OnEditorPodDeleted);
    sub(GE::onEditorPartDeleted, &StageManager::OnEditorPartDeleted);
    sub(GE::onEditorRestoreState, &StageManager::OnEditorRestoreState);
    sub(GE::StageManager::OnPartUpdateStageability, &StageManager::OnPartUpdateStageability);
    sub(GE::onVesselSwitching, &StageManager::OnVesselSwitching);
    sub(GE::onEditorLoad, &StageManager::OnEditorLoadEvent);
    InstallPartHooks();
    return true;
}

// ProtoStageIcon.cs:57 / :101 and StageManager.StageCount as seen by Part
void StageManager::InstallPartHooks() {
    Part::stagingHooks.stageCount = [] { return StageManager::StageCount(); };
    Part::stagingHooks.createIcon = [](Part& p, bool alert) { StageManager::CreateIcon(p, alert); };
    Part::stagingHooks.removeIcon = [](Part& p, bool alert) {
        StageManager* sm = StageManager::Instance;
        if (sm == nullptr) return;
        StageIcon* icon = sm->IconOf(&p);
        if (icon == nullptr) return;
        sm->byPart_.erase(&p);  // ProtoStageIcon.RemoveIcon: stageIcon = null
        StageManager::RemoveIcon(icon, true, true, alert);
    };
    Part::stagingHooks.hasIcon = [](const Part& p) { return StageManager::Instance != nullptr && StageManager::Instance->IconOf(&p) != nullptr; };
}
void StageManager::RemovePartHooks() { Part::stagingHooks = Part::StagingHooks{}; }

StageIcon* StageManager::IconOf(const Part* part) const {
    auto it = byPart_.find(part);
    return it == byPart_.end() ? nullptr : it->second;
}

std::vector<StageGroup*> StageManager::Stages() {
    std::vector<StageGroup*> out;
    for (auto& g : stages_) out.push_back(g.get());
    return out;
}

// :146
int StageManager::LastStage() {
    if (Instance == nullptr) throw Nre();
    if (static_cast<int>(Instance->stages_.size()) - 1 < 0) return 0;
    return static_cast<int>(Instance->stages_.size()) - 1;
}
// :158
int StageManager::StageCount() {
    if (Instance == nullptr) throw Nre();
    return static_cast<int>(Instance->stages_.size());
}
// :160
int StageManager::CurrentStage() {
    if (Instance == nullptr) throw Nre();
    return Instance->currentStage_;
}
// :162
std::vector<StageIcon*>& StageManager::Selection() {
    static std::vector<StageIcon*> none;
    if (Instance == nullptr) {
        none.clear();
        return none;
    }
    return Instance->selection;
}
// :174
bool StageManager::CanSeparate() {
    if (Instance == nullptr) throw Nre();
    return Instance->canSeparate_;
}

// ---- GameEvents handlers

// :281
void StageManager::OnVesselClearStaging(Vessel*) { SortIcons(false); }

// :286
void StageManager::OnVesselResumeStaging(Vessel* vessel) {
    if (vessel == host_->ActiveVessel()) ResumeFlight(vessel->currentStage);
}

// :294
void StageManager::OnPartUpdateStageability(Part* data) {
    rebuildIndexes_ = true;
    data->fields.inverseStage = ClampMin0(data->fields.inverseStage);
    SortIcons(true);
}

// :301 (DeltaVCalcsCompleted is the stack's delta-v readout: UI)
void StageManager::OnEditorStarted() { SortIcons(false, true); }

// :307
void StageManager::OnEditorPodSelected(Part* rootPart) {
    SetSeparationIndices(rootPart, -1);
    SortIcons(false);
}

// :313
void StageManager::OnEditorPodDeleted() { SortIcons(false); }

// :318
void StageManager::OnEditorPartPicked(Part* part) {
    std::vector<StageIcon*> list = AddPartsWithStageiconsToListRecursive(part);
    for (size_t i = 0; i < list.size(); i++) HoldIcon(list[i]);
}

// :328
void StageManager::OnEditorPartPlaced(Part* part) { SortIcons(true, part, false, false); }

// :333
void StageManager::OnEditorPartDeleted(Part*) { SortIcons(false); }

// :338
void StageManager::OnEditorRestoreState() {
    holdEditorSelectedIcons = true;
    SortIcons(false);
}

// :344
void StageManager::OnVesselSwitching(Vessel* from, Vessel*) {
    if (from != nullptr) from->currentStage = currentStage_;
}

// :352
void StageManager::OnEditorLoad() { rebuildIndexes_ = true; }

// ---- :373
void StageManager::AddNewStageGroupsIfNeeded(Part* selectedPart) {
    if (selectedPart != nullptr && selectedPart->partInfo != nullptr &&
        ((selectedPart->partInfo->preferredStage == static_cast<int>(PreferredInitialStage::LAST) && !stages_.empty()) ||
         (selectedPart->partInfo->preferredStage == static_cast<int>(PreferredInitialStage::PARACHUTESTAGE) && !stages_.empty())))
        return;
    int num = 0;
    for (size_t i = 0; i < stages_.size(); i++) {
        StageGroup* stageGroup = stages_[i].get();
        for (size_t j = 0; j < stageGroup->Icons().size(); j++) num = std::max(num, stageGroup->Icons()[j]->InverseStage());
    }
    for (size_t k = 0; k < iconsInHold.size(); k++) num = std::max(num, iconsInHold[k]->InverseStage());
    int num2 = 0;
    if (num > 0) num2 = num;
    if (num2 < static_cast<int>(stages_.size())) return;
    const int num3 = num2 - static_cast<int>(stages_.size()) + 1;
    if (num3 > 0) {
        for (int l = 0; l < num3; l++) {
            StageGroup* g = NewStageGroup();
            g->defaultStage = static_cast<int>(stages_.size());
            g->inverseStageIndex = static_cast<int>(stages_.size());
            stages_.emplace_back(g);
        }
    }
}

StageGroup* StageManager::NewStageGroup() { return new StageGroup(); }

// :416
void StageManager::AddHeldIconsToStages(Part* selectedPart, bool switchingVessels) {
    const bool flight = host_->LoadedSceneIsFlight();
    // the editor branch of the placement of one icon (:473-523 for the symmetry groups, :568-618 for the singles)
    auto placeEditor = [&](StageIcon* ic, bool single) {
        if (selectedPart != nullptr) {
            if (!selectedPart->fields.editorFirstTimePlaced) {
                const int pref = selectedPart->partInfo != nullptr ? selectedPart->partInfo->preferredStage : static_cast<int>(PreferredInitialStage::DEFAULT);
                switch (static_cast<PreferredInitialStage>(pref)) {
                    case PreferredInitialStage::FIRST:
                        stages_[0]->AddIcon(ic);
                        break;
                    case PreferredInitialStage::LAST:
                        stages_[stages_.size() - 1]->AddIcon(ic);
                        break;
                    case PreferredInitialStage::PARACHUTESTAGE: {
                        const int parachuteStageIndex = GetParachuteStageIndex(ic->InverseStage());
                        stages_[static_cast<size_t>(parachuteStageIndex)]->AddIcon(ic);
                        break;
                    }
                    default:  // DEFAULT and any other value
                        stages_[static_cast<size_t>(ic->InverseStage())]->AddIcon(ic);
                        break;
                }
                selectedPart->fields.editorFirstTimePlaced = true;
            } else if (single && ic->InverseStage() >= static_cast<int>(stages_.size())) {
                stages_[stages_.size() - 1]->AddIcon(ic);
            } else {
                stages_[static_cast<size_t>(ic->InverseStage())]->AddIcon(ic);
            }
        } else if (single && ic->InverseStage() >= static_cast<int>(stages_.size())) {
            stages_[stages_.size() - 1]->AddIcon(ic);
        } else {
            stages_[static_cast<size_t>(ic->InverseStage())]->AddIcon(ic);
        }
    };
    if (!iconsInHold.empty()) {
        std::vector<std::vector<StageIcon*>> list;
        std::vector<StageIcon*> list2;
        int num;
        for (num = static_cast<int>(iconsInHold.size()) - 1; num >= 0; num = static_cast<int>(iconsInHold.size()) - 1) {
            StageIcon* stageIcon = iconsInHold[0];
            stageIcon->part->fields.inverseStage = ClampMin0(stageIcon->part->fields.inverseStage);
            std::vector<StageIcon*> list3;
            int num2 = 0;
            for (int num3 = num; num3 >= 0; num3--) {
                StageIcon* stageIcon2 = iconsInHold[static_cast<size_t>(num3)];
                if (stageIcon2 != stageIcon && stageIcon2->InverseStage() == stageIcon->InverseStage() &&
                    stageIcon2->part->isSymmetryCounterPart(stageIcon->part)) {
                    list3.push_back(stageIcon2);
                    num2++;
                }
            }
            if (num2 > 0) {
                list3.push_back(stageIcon);
                list.push_back(list3);
                for (int num4 = num; num4 >= 0; num4--)
                    if (Contains(list3, iconsInHold[static_cast<size_t>(num4)])) iconsInHold.erase(iconsInHold.begin() + num4);
            } else {
                list2.push_back(stageIcon);
                EraseOne(iconsInHold, stageIcon);
            }
        }
        num = static_cast<int>(list.size());
        for (int i = 0; i < num; i++) {
            std::vector<StageIcon*>& iteratedIcons = list[static_cast<size_t>(i)];
            StageIcon* stageIcon3;
            if (iteratedIcons[0]->groupLead != nullptr) {
                stageIcon3 = iteratedIcons[0]->groupLead;
            } else if (selectedPart != nullptr && IconOf(selectedPart) != nullptr && Contains(iteratedIcons, IconOf(selectedPart))) {
                stageIcon3 = IconOf(selectedPart);
            } else if (iteratedIcons[0]->part->parent == nullptr) {
                stageIcon3 = iteratedIcons[0];
            } else {
                StageIcon* found = nullptr;
                for (Part* a : iteratedIcons[0]->part->parent->children) {
                    StageIcon* ic = IconOf(a);
                    if (ic != nullptr && Contains(iteratedIcons, ic)) {
                        found = ic;
                        break;
                    }
                }
                if (found == nullptr) throw Nre();
                stageIcon3 = found;
            }
            if (flight) {
                stages_[static_cast<size_t>(std::min(stageIcon3->InverseStage(), currentStage_))]->AddIcon(stageIcon3);
            } else {
                placeEditor(stageIcon3, false);
            }
            for (size_t j = 0; j < iteratedIcons.size(); j++)
                if (iteratedIcons[j] != stageIcon3) stageIcon3->AddToGroup(iteratedIcons[j]);
            stageIcon3->CollapseGroup();
        }
        num = static_cast<int>(list2.size());
        for (int k = 0; k < num; k++) {
            StageIcon* stageIcon4 = nullptr;
            if (list2[static_cast<size_t>(k)]->InverseStage() < static_cast<int>(stages_.size()))
                stageIcon4 = stages_[static_cast<size_t>(list2[static_cast<size_t>(k)]->InverseStage())]->FindSymmetryGroupleader(list2[static_cast<size_t>(k)]);
            if (stageIcon4 != nullptr) {
                stageIcon4->AddToGroup(list2[static_cast<size_t>(k)]);
                stageIcon4->CollapseGroup();
            } else if (flight) {
                stages_[static_cast<size_t>(std::min(list2[static_cast<size_t>(k)]->InverseStage(), currentStage_))]->AddIcon(list2[static_cast<size_t>(k)]);
            } else {
                placeEditor(list2[static_cast<size_t>(k)], true);
            }
        }
    }
    if (flight) {
        for (int l = 0, num5 = static_cast<int>(stages_.size()); l < num5; l++) {
            if (stages_[static_cast<size_t>(l)]->Icons().empty() &&
                (stages_[static_cast<size_t>(l)]->defaultStage != -1 || stages_[static_cast<size_t>(l)]->inverseStageIndex >= currentStage_)) {
                DeleteStage(stages_[static_cast<size_t>(l--)].get(), Visible && !switchingVessels);
                num5--;
            }
        }
    }
    int count2 = static_cast<int>(stages_.size());
    while (count2-- > 0) UpdateStageGroup(stages_[static_cast<size_t>(count2)].get(), count2, false);
    iconsInHold.clear();
}

// :637 (sic: the recursion at the end discards its result)
int StageManager::GetParachuteStageIndex(int currentIndex) {
    if (currentIndex == 0) {
        if (stages_[static_cast<size_t>(currentIndex)]->Icons().empty()) return currentIndex;
        bool foundParachute = false;
        bool foundDecouple = false;
        for (size_t i = 0; i < stages_[static_cast<size_t>(currentIndex)]->Icons().size(); i++) {
            Part* p = stages_[static_cast<size_t>(currentIndex)]->Icons()[i]->part;
            foundParachute = HasModuleClass(*p, "ModuleParachute");
            foundDecouple = false;
            if (!foundParachute) foundDecouple = HasModuleClass(*p, "ModuleDecouple");
            if (foundParachute || foundDecouple) return currentIndex;
        }
        if (!foundParachute) {
            StageGroup* g = NewStageGroup();
            g->defaultStage = static_cast<int>(stages_.size());
            g->inverseStageIndex = static_cast<int>(stages_.size());
            stages_.emplace_back(g);
            for (int num = static_cast<int>(stages_.size()) - 2; num >= 0; num--)
                stages_[static_cast<size_t>(num)]->MoveAllIconsInto(stages_[static_cast<size_t>(num) + 1].get());
            return currentIndex;
        }
    } else {
        if (!stages_.empty() && currentIndex >= static_cast<int>(stages_.size())) currentIndex = static_cast<int>(stages_.size()) - 1;
        if (!stages_[static_cast<size_t>(currentIndex)]->Icons().empty()) {
            bool foundDecouple = false;
            for (size_t j = 0; j < stages_[static_cast<size_t>(currentIndex)]->Icons().size(); j++) {
                foundDecouple = HasModuleClass(*stages_[static_cast<size_t>(currentIndex)]->Icons()[j]->part, "ModuleDecouple");
                if (foundDecouple) return currentIndex;
            }
            if (!foundDecouple && currentIndex > 0) {
                currentIndex--;
                GetParachuteStageIndex(currentIndex);
            }
        }
    }
    return currentIndex;
}

// ---- icons

// :822
void StageManager::HoldIcon(StageIcon* stageIcon, bool removeIcon, bool alertStagingSequencer) {
    if (!Contains(iconsInHold, stageIcon)) {
        if (removeIcon) RemoveIcon(stageIcon, false, false, alertStagingSequencer);
        iconsInHold.push_back(stageIcon);
    }
}

// :835
void StageManager::UnHoldIcon(StageIcon* stageIcon, bool /*changeParents*/) { EraseOne(iconsInHold, stageIcon); }

// :843 + ProtoStageIcon.cs:57 (CreateIcon)
StageIcon* StageManager::CreateIcon(Part& part, bool alertStagingSequencer) {
    if (!part.fields.stagingIcon || part.fields.stagingIcon->empty()) return nullptr;
    StageManager* sm = Instance;
    IStagingHost* host = sm != nullptr ? sm->host_ : nullptr;
    if (host != nullptr && host->LoadedSceneIsFlight() && (part.State() == PartStates::DEAD || part.vessel != host->ActiveVessel())) return nullptr;
    if (sm == nullptr) return nullptr;
    if (StageIcon* existing = sm->IconOf(&part)) return existing;
    sm->iconCache_.push_back(std::make_unique<StageIcon>(&part));
    StageIcon* newStageIcon = sm->iconCache_.back().get();
    sm->HoldIcon(newStageIcon, true, alertStagingSequencer);
    sm->byPart_[&part] = newStageIcon;
    return newStageIcon;
}

// :881
void StageManager::RemoveIcon(StageIcon* stageIcon, bool destroyIcon, bool removeSelection, bool alertStagingSequencer) {
    StageManager* sm = Instance;
    if (sm == nullptr || stageIcon == nullptr) return;
    if (sm->host_->LoadedSceneIsFlight()) {
        StageGroup* foundInGroup = nullptr;
        stageIcon->RemoveFromGroupAndReshuffle(foundInGroup);
        if (foundInGroup != nullptr) foundInGroup->UpdateInStageIndexes();
    } else {
        if (stageIcon->groupLead != nullptr) stageIcon->groupLead->HoldGroupedIcons(alertStagingSequencer);
        for (size_t i = 0; i < sm->stages_.size(); i++)
            if (Contains(sm->stages_[i]->Icons(), stageIcon)) sm->stages_[i]->RemoveIcon(stageIcon);
    }
    if (Contains(sm->iconsInHold, stageIcon)) EraseOne(sm->iconsInHold, stageIcon);
    if (removeSelection) EraseOne(Selection(), stageIcon);
    if (destroyIcon) {
        stageIcon->active = false;
        stageIcon->groupLead = nullptr;
        stageIcon->groupedIcons.clear();
    }
    if (sm->host_->LoadedSceneIsFlight() && alertStagingSequencer) sm->SortIcons(false);
}

// :932
void StageManager::DisableIcon(StageIcon* iconToDisable) {
    if (iconToDisable == nullptr) return;
    if (iconToDisable->groupLead != nullptr) {
        iconToDisable->groupLead->DisableGroup(iconToDisable);
    } else if (iconToDisable->stage != nullptr) {
        iconToDisable->stage->RemoveIcon(iconToDisable, false);
        iconToDisable->active = false;
        EraseOne(Selection(), iconToDisable);
    }
}

// ---- stage groups

// :949
void StageManager::AddStageAt(int index) {
    if (index < 0 || index > static_cast<int>(stages_.size())) throw std::out_of_range("Index must be within the bounds of the List.");
    stages_.insert(stages_.begin() + index, std::unique_ptr<StageGroup>(NewStageGroup()));
    UpdateStageGroups();
}

// :958
void StageManager::InsertStageAt(StageGroup* stageGroup, int index, int /*forcedSiblingIndex*/) {
    for (auto& g : stages_)
        if (g.get() == stageGroup) return;
    std::unique_ptr<StageGroup> owned;
    for (size_t i = 0; i < deletedGroups_.size(); i++)
        if (deletedGroups_[i].get() == stageGroup) {
            owned = std::move(deletedGroups_[i]);
            deletedGroups_.erase(deletedGroups_.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    if (!owned) owned.reset(stageGroup);
    stages_.insert(stages_.begin() + index, std::move(owned));
    UpdateStageGroups();
}

// :972 (the tween-out animation is UI)
void StageManager::DeleteStage(StageGroup* stageToDelete, bool /*tweenOut*/) {
    for (size_t i = 0; i < stages_.size(); i++)
        if (stages_[i].get() == stageToDelete) {
            deletedGroups_.push_back(std::move(stages_[i]));
            stages_.erase(stages_.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    UpdateStageGroups();
}

// :1005
void StageManager::DeleteEmptyStages() { DeleteEmptyStages(false); }

// :1010
void StageManager::DeleteEmptyStages(bool deleteManualStages) {
    bool flag = false;
    int count = static_cast<int>(stages_.size());
    while (count-- > 0) {
        StageGroup* stageGroup = stages_[static_cast<size_t>(count)].get();
        if (stageGroup->Icons().empty() && (stageGroup->defaultStage != -1 || deleteManualStages)) {
            deletedGroups_.push_back(std::move(stages_[static_cast<size_t>(count)]));
            stages_.erase(stages_.begin() + count);
            DecrementCurrentStage();
            flag = true;
        }
    }
    if (flag) {
        UpdateStageGroups(false);
        SetSeparationIndices(host_->LoadedSceneIsEditor() ? host_->RootPart() : nullptr, -1);
    }
}

// :1032
void StageManager::RemoveStage(StageGroup* stageGroup) {
    for (size_t i = 0; i < stages_.size(); i++)
        if (stages_[i].get() == stageGroup) {
            deletedGroups_.push_back(std::move(stages_[i]));
            stages_.erase(stages_.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    UpdateStageGroups();
}

// :1038
int StageManager::GetStageCount(const std::vector<Part*>& parts) {
    int num = 0;
    int count = static_cast<int>(parts.size());
    while (count-- > 0) num = std::max(num, parts[static_cast<size_t>(count)]->fields.inverseStage);
    return num;
}

// :1049
void StageManager::SetStageCount(const std::vector<Part*>& ship) {
    int num = 0;
    int count = static_cast<int>(ship.size());
    while (count-- > 0) {
        const int inverseStage = ship[static_cast<size_t>(count)]->fields.inverseStage;
        if (inverseStage > num) num = inverseStage;
    }
}

// :1063
std::vector<Part*> StageManager::FindPartsWithModuleSeparatingBeforeOtherPartsWithModule(const std::vector<Part*>& parts,
                                                                                         const std::function<bool(Part&)>& isBefore,
                                                                                         const std::function<bool(Part&)>& isAfter) {
    std::vector<Part*> list;
    std::vector<Part*> list2(parts);
    // StageManager.cs:1067: List.Sort is the unstable introsort and the keys tie on separationIndex, so the tie order is
    // the introsort's, reproduced bit-for-bit by dotnet::ListSort
    dotnet::ListSort(list2, [](Part* a, Part* b) { return IntCompareTo(a->fields.separationIndex, b->fields.separationIndex); });
    Part* part = nullptr;
    int num = 0;
    bool flag = false;
    bool flag2 = false;
    std::vector<Part*> list3;
    for (size_t i = 0, count = list2.size(); i < count; i++) {
        part = list2[i];
        if (num != part->fields.separationIndex) {
            if (flag) break;
            if (flag2) {
                list.insert(list.end(), list3.begin(), list3.end());
                list3.clear();
            }
            num = part->fields.separationIndex;
            flag2 = false;
        }
        if (isBefore(*part)) flag = true;
        if (!flag && isAfter(*part)) {
            flag2 = true;
            list3.push_back(part);
        }
    }
    return list;
}

// :1104
void StageManager::SetManualStageOffset(int startIndex) {
    if (startIndex >= 0 && startIndex < static_cast<int>(stages_.size()))
        for (size_t i = static_cast<size_t>(startIndex); i < stages_.size(); i++) stages_[i]->SetManualStageOffset();
}

// ---- SortIcons

// :1116
void StageManager::SortIconsDefault() { SortIcons(true, nullptr, false, false); }
// :1121
void StageManager::SortIcons(bool instant) { SortIcons(instant, nullptr, false, false); }
// :1126
void StageManager::SortIcons(bool instant, bool deleteManualStages) { SortIcons(instant, nullptr, false, deleteManualStages); }

// :1131
void StageManager::SortIcons(bool instant, Part* p, bool switchingVessels, bool deleteManualStages) {
    if (instant) {
        if (!sortRoutine_) SortIconsSequence(p, switchingVessels, deleteManualStages);
    } else if (!sortRoutine_) {
        // StartCoroutine(SortIconsSequence(p, 2, ...)): runs to its first yield at once
        sortRoutine_ = true;
        pendingPart_ = p;
        pendingSwitching_ = switchingVessels;
        pendingDeleteManual_ = deleteManualStages;
        coroutineFramesToWait_ = 2;
    } else {
        coroutineFramesToWait_ = 2;
    }
}

// :1150 (the coroutine body) / :1160
void StageManager::SortIconsSequence(Part* p, bool switchingVessels, bool deleteManualStages) {
    AddNewStageGroupsIfNeeded(p);
    AddHeldIconsToStages(p, switchingVessels);
    SetCurrentStage((currentStage_ < static_cast<int>(stages_.size())) ? (static_cast<int>(stages_.size()) - 1) : static_cast<int>(stages_.size()));
    if (holdEditorSelectedIcons && host_->SelectedPart() != nullptr) {
        std::vector<StageIcon*> list = AddPartsWithStageiconsToListRecursive(host_->SelectedPart());
        for (size_t i = 0; i < list.size(); i++) HoldIcon(list[i]);
    }
    holdEditorSelectedIcons = false;
    if (EnableDeleteEmptyStages) DeleteEmptyStages(deleteManualStages);
    if (rebuildIndexes_) {
        SetSeparationIndices();
        GE::StageManager::OnGUIStageSequenceModified.Fire();
        rebuildIndexes_ = false;
    }
}

// one frame: the SortIcons coroutine (`for (cfw = frames; cfw >= 0; cfw--) yield`) and cooldownSeparation (:1308)
void StageManager::Tick(double dt) {
    if (cooldown_ > 0.0) {
        cooldown_ -= dt;
        if (cooldown_ <= 0.0) {
            cooldown_ = 0.0;
            canSeparate_ = true;
        }
    }
    if (sortRoutine_) {
        coroutineFramesToWait_--;
        if (coroutineFramesToWait_ < 0) {
            Part* p = pendingPart_;
            const bool sw = pendingSwitching_, dm = pendingDeleteManual_;
            SortIconsSequence(p, sw, dm);
            sortRoutine_ = false;
        }
    }
}

void StageManager::FlushSort() {
    int guard = 0;
    while (sortRoutine_ && guard++ < 1000) Tick();
}

// :1193
void StageManager::UpdateStageGroups(bool seqOverride) {
    for (size_t i = 0, count = stages_.size(); i < count; i++) UpdateStageGroup(stages_[i].get(), static_cast<int>(i), seqOverride);
}

// :1202
void StageManager::UpdateStageGroup(StageGroup* group, int index, bool seqOverride) {
    group->SetInverseStageIndex(index);
    group->SetPartIndices(seqOverride);
}

// ---- flight

// :1209
void StageManager::BeginFlight() {
    Instance->rebuildIndexes_ = true;
    Instance->SortIcons(false);
    int num = 0;
    Vessel* av = Instance->host_->ActiveVessel();
    int count = static_cast<int>(av->parts.size());
    while (count-- > 0) {
        const int inverseStage = av->parts[static_cast<size_t>(count)]->fields.inverseStage;
        if (inverseStage > num) num = inverseStage;
    }
    Instance->SetCurrentStage(num + 1);
    Instance->canSeparate_ = true;
}

// :1227
void StageManager::ResumeFlight(int currStage) {
    Instance->SetCurrentStage(currStage);
    Instance->canSeparate_ = true;
    Instance->rebuildIndexes_ = true;
    Instance->SortIcons(false, nullptr, true, false);
}

// :1235
void StageManager::ActivateNextStage() { ActivateStage(Instance->currentStage_ - 1); }

// :1244
void StageManager::ActivateStage(int stage) {
    StageManager* sm = Instance;
    Vessel* activeVessel = sm->host_->ActiveVessel();
    stage = std::min(std::max(0, stage), static_cast<int>(sm->stages_.size()) - 1);
    bool flag = !CanSeparate() || sm->currentStage_ == 0;
    if (!flag) {
        int count = static_cast<int>(sm->stages_.size());
        while (count-- > 0) {
            if (sm->stages_[static_cast<size_t>(count)]->IsBeingDragged) {
                flag = true;
                break;
            }
        }
    }
    if (flag) return;  // cannotSeparateClip
    GE::onStageActivate.Fire(stage);
    activeVessel->updateResourcesOnEvent = false;
    std::vector<Part*> list;
    int count2 = static_cast<int>(activeVessel->parts.size());
    while (count2-- > 0) {
        Part* part = activeVessel->parts[static_cast<size_t>(count2)];
        if (part->fields.inverseStage == stage) list.push_back(part);
    }
    for (size_t i = 0; i < list.size(); i++) list[i]->force_activate();
    if (!FlightLogger::LiftOff()) {
        EventReport rep(FlightEvents::LAUNCH, nullptr);
        GE::onLaunch.Fire(&rep);
    }
    if (stage != static_cast<int>(sm->stages_.size()) - 1) sm->stages_[static_cast<size_t>(stage)]->MoveAllIconsInto(sm->stages_.back().get());
    sm->SetCurrentStage(stage);
    sm->canSeparate_ = false;
    sm->cooldown_ = StagingCooldownTimer();  // StartCoroutine(cooldownSeparation(PhysicsGlobals.StagingCooldownTimer))
    sm->SortIcons(true);
    activeVessel->updateResourcesOnEvent = true;
    activeVessel->UpdateResourceSetsIfDirty();
}

// :1314
void StageManager::IncrementCurrentStage() {
    currentStage_++;
    if (host_->LoadedSceneIsFlight()) host_->ActiveVessel()->currentStage = currentStage_;
}

// :1323
void StageManager::DecrementCurrentStage() {
    if (currentStage_ > 0) {
        currentStage_--;
        if (host_->LoadedSceneIsFlight()) host_->ActiveVessel()->currentStage = currentStage_;
    }
}

// :1335
void StageManager::SetCurrentStage(int cStage) {
    if (currentStage_ >= 0) {
        currentStage_ = cStage;
        if (host_->LoadedSceneIsFlight()) host_->ActiveVessel()->currentStage = currentStage_;
    }
}

// :1347
void StageManager::StepBackCurrentStage() { Instance->SetCurrentStage(static_cast<int>(Instance->stages_.size())); }

// :1376
std::vector<StageIcon*> StageManager::AddPartsWithStageiconsToListRecursive(Part* part) {
    std::vector<StageIcon*> partsWithIcons;
    AddPartsWithStageiconsToListRecursive(partsWithIcons, part);
    return partsWithIcons;
}

// :1383
void StageManager::AddPartsWithStageiconsToListRecursive(std::vector<StageIcon*>& partsWithIcons, Part* part) {
    if (IconOf(part) != nullptr) {
        partsWithIcons.push_back(IconOf(part));
        for (size_t i = 0; i < part->children.size(); i++) AddPartsWithStageiconsToListRecursive(partsWithIcons, part->children[i]);
    } else {
        for (size_t j = 0; j < part->children.size(); j++) AddPartsWithStageiconsToListRecursive(partsWithIcons, part->children[j]);
    }
}

// ---- the staging sequence

// :1402
void StageManager::GenerateStagingSequence(Part* root) {
    FindStageIndices(root, -1, -1);
    SetSeparationIndices(root, -1);
}

// :1408 (the goto-structured body flattened: `num` is "the part shows a stage icon")
void StageManager::FindStageIndices(Part* part, int thisStage, int carriedInverseStage) {
    const bool hasIcon = part->hasStagingIcon() || (Instance != nullptr ? Instance->IconOf(part) != nullptr : part->HasStageIcon());
    const int num3 = hasIcon ? part->fields.stageOffset : 0;
    thisStage = std::max(0, thisStage + num3);
    if (hasIcon && part->fields.stageBefore) thisStage++;
    part->fields.defaultInverseStage = thisStage;
    const int inverseStage = part->fields.inverseStage;
    if (hasIcon) {
        part->fields.inverseStage = (part->fields.manualStageOffset != -1) ? part->fields.manualStageOffset : thisStage;
        if (part->fields.inverseStageCarryover) carriedInverseStage = part->fields.inverseStage;
    } else {
        part->fields.inverseStage = carriedInverseStage;
    }
    if (inverseStage != part->fields.inverseStage) GE::onPartPriorityChanged.Fire(part);
    if (hasIcon && part->fields.stageAfter) thisStage++;
    const int num4 = hasIcon ? part->fields.childStageOffset : 0;
    thisStage = std::max(0, thisStage + num4);
    for (size_t i = 0, count = part->children.size(); i < count; i++) FindStageIndices(part->children[i], thisStage, carriedInverseStage);
}

// :1477
int StageManager::RecalculateVesselStaging(Vessel* v) {
    // FindLastInverseStage (:1484)
    std::function<int(Part*, int)> findLast = [&](Part* part, int lastStg) {
        lastStg = std::max(part->fields.inverseStage, lastStg);
        for (size_t i = 0; i < part->children.size(); i++) lastStg = findLast(part->children[i], lastStg);
        return lastStg;
    };
    const int result = findLast(v->rootPart, 0);
    SetSeparationIndices(v->rootPart, -1);
    return result;
}

// :1494
void StageManager::SetSeparationIndices() {
    if (Instance == nullptr) throw Nre();
    if (Instance->host_->LoadedSceneIsFlight()) SetSeparationIndices(Instance->host_->ActiveVessel()->rootPart, -1);
    else if (Instance->host_->LoadedSceneIsEditor()) SetSeparationIndices(Instance->host_->RootPart(), -1);
}

// :1506
void StageManager::SetSeparationIndices(Part* p, int /*sepIndex*/) {
    setSeparationIndices(p, -1, -1);
    GE::StageManager::OnStagingSeparationIndices.Fire();
}

// :1512
void StageManager::setSeparationIndices(Part* part, int sepIndex, int carriedInverseStage) {
    if (part == nullptr) return;
    std::vector<IStageSeparator*> list = FindStageSeparators(*part);
    IStageSeparatorChild* stageSeparatorChild = FindStageSeparatorChild(*part);
    bool flag = true;
    int num = sepIndex;
    std::vector<Part*> decoupledChildParts;
    int count = static_cast<int>(list.size());
    while (count-- > 0) num = list[static_cast<size_t>(count)]->GetStageIndex(sepIndex);
    if (stageSeparatorChild != nullptr) flag = stageSeparatorChild->PartDetaches(decoupledChildParts);
    if (flag) {
        sepIndex = num;
        part->fields.separationIndex = sepIndex;
    } else {
        part->fields.separationIndex = sepIndex;
        sepIndex = num;
    }
    if ((part->hasStagingIcon() || part->HasStageIcon()) && part->fields.stagingOn) {
        if (part->fields.inverseStageCarryover) carriedInverseStage = part->fields.inverseStage;
    } else {
        part->fields.inverseStage = carriedInverseStage;
    }
    for (size_t i = 0, count2 = part->children.size(); i < count2; i++) {
        if (!flag && !decoupledChildParts.empty()) {
            bool flag2 = false;
            for (size_t j = 0; j < decoupledChildParts.size(); j++) {
                if (part->children[i]->fields.persistentId == decoupledChildParts[j]->fields.persistentId) {
                    flag2 = true;
                    setSeparationIndices(part->children[i], sepIndex, carriedInverseStage);
                }
            }
            if (!flag2) setSeparationIndices(part->children[i], part->fields.separationIndex, carriedInverseStage);
        } else {
            setSeparationIndices(part->children[i], sepIndex, carriedInverseStage);
        }
    }
}

// :1579 (the confirmation ScreenMessages are UI)
void StageManager::Reset() {
    bool flag = false;
    int count = static_cast<int>(stages_.size());
    while (count-- > 0) {
        if (stages_[static_cast<size_t>(count)]->ResetAvailable()) {
            flag = true;
            break;
        }
    }
    if (flag) {
        count = static_cast<int>(stages_.size());
        while (count-- > 0) {
            stages_[static_cast<size_t>(count)]->Reset();
            deletedGroups_.push_back(std::move(stages_[static_cast<size_t>(count)]));
        }
        stages_.clear();
        SetCurrentStage(0);
        SortIcons(false);
        SetSeparationIndices(host_->RootPart(), -1);
    }
}

// :1631
bool StageManager::CheckNullrefsInStageIconList() const {
    bool result = false;
    for (size_t i = 0; i < stages_.size(); i++)
        for (size_t j = 0; j < stages_[i]->Icons().size(); j++)
            if (stages_[i]->Icons()[j] == nullptr) {
                log::Error("StageGroup has null icon ->");
                result = true;
            }
    return result;
}

bool IsDefaultIconName(std::string_view name) {
    // DefaultIcons.cs
    static const char* const kNames[] = {"CUSTOM", "MYSTERY_PART", "LIQUID_ENGINE", "SOLID_BOOSTER", "COMMAND_POD", "DECOUPLER_VERT",
                                         "DECOUPLER_HOR", "FUEL_TANK", "PARACHUTES", "WINGLETS", "SAS", "STRUT", "STRUT_CONNECTOR",
                                         "ADV_SAS", "RCS_TANK", "RCS_MODULE", "FUEL_LINE", "LANDING_LEG", "REACTION_WHEEL", "WHEEL",
                                         "LANDING_GEAR", "PROBE", "SCIENCE_GENERIC"};
    for (const char* n : kNames)
        if (name == n) return true;
    return false;
}

}  // namespace ksp
