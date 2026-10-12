// WP-54c: Assembly-CSharp/Expansions/Missions/Actions/{ActionPartFailure,ActionPartRepair,ActionPartResourceDrain}.cs. See mission_actions3.hpp.
// Float rule (README "Mono float32 rule"): managed float expressions are f32store(fx(a) op fx(b)).
// Cite placeholders `File.cs:@Name[#k]` are resolved by tools/wp55/check_cites.py --resolve.
#include "ksp/missions/mission_actions3.hpp"

#include <algorithm>

#include "../save/save_util.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_awards.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Missions::Actions {

using KSP::Localization::Localizer;
using NLE = NodeLibraryEnvironment;

namespace {

double epsilonDrain = 1E-15;           // ActionPartResourceDrain.cs:@epsilon
const float drainEveryXSecs = 0.5f;    // ActionPartResourceDrain.cs:@drainEveryXSecs

std::string Tag(const char* t) { return Localizer::Format(t); }
std::string Fmt(const char* tag, std::initializer_list<std::string> args) { return Localizer::Format(tag, std::vector<std::string>(args)); }

// FlightGlobals.FindVessel(id, out vessel)
bool FindVesselById(uint32_t id, Vessel*& vessel) {
    vessel = nullptr;
    if (AwardEnvironment::findVessel) return AwardEnvironment::findVessel(id, vessel);
    vessel = AwardEnvironment::VesselByPersistentId(id);
    return vessel != nullptr;
}
// FlightGlobals.FindLoadedPart(id, out part)
Part* FindLoadedPart(uint32_t id) { return NLE::findLoadedPart ? NLE::findLoadedPart(id) : nullptr; }
// FlightGlobals.FindUnloadedPart(id, out part)
ProtoPartSnapshot* FindUnloadedPart(uint32_t id) {
    if (NLE::findUnloadedPart) return NLE::findUnloadedPart(id);
    ProtoPartSnapshot* p = nullptr;
    SaveEnvironment::Current().persistentIds.persistentUnloadedPartIds.TryGetValue(id, p);
    return p;
}
// part.Modules[i].GetType().Name for every module
std::vector<std::string> ModuleNames(Part* part) {
    if (NLE::partModuleNames) return NLE::partModuleNames(part);
    std::vector<std::string> names;
    for (size_t i = 0; i < part->Modules().Count(); ++i) names.push_back(part->Modules()[i]->ClassName());
    return names;
}
// MissionsUtils.adjusterTypesSupportedByPartModule[name] (nullptr = ContainsKey is false)
const std::vector<std::string>* AdjustersFor(const std::string& partModuleName) {
    for (const auto& kv : Adjusters::GetAdjusterTypeTables().adjusterTypesSupportedByPartModule)
        if (kv.first == partModuleName) return &kv.second;
    return nullptr;
}
bool Contains(const std::vector<std::string>& list, const std::string& v) { return std::find(list.begin(), list.end(), v) != list.end(); }

// ProtoPartModuleSnapshot.cs:92 + :@AddPartModuleAdjusterList + ProtoPartSnapshot.cs:@AddProtoPartModuleAdjusters
void ProtoAddAdjusters(ProtoPartSnapshot* part, const std::vector<Adjusters::AdjusterPartModuleBase*>& adjusters) {
    if (!MissionsEnvironment::MakingHistoryInstalled()) return;
    for (size_t m = 0; m < part->modules.size(); ++m) {
        ProtoPartModuleSnapshot* module = part->modules[m].get();
        // ProtoPartModuleSnapshot.cs, MakingHistoryInstalled check (line 81)
        if (!MissionsEnvironment::MakingHistoryInstalled()) continue;
        for (size_t i = 0; i < adjusters.size(); ++i) {
            // ProtoPartModuleSnapshot.cs:92
            if (!module->moduleName.has_value()) throw ArgumentNullException("key");
            const std::vector<std::string>* list = AdjustersFor(*module->moduleName);
            if (list == nullptr) continue;
            Adjusters::AdjusterPartModuleBase* newAdjuster = adjusters[i];
            if (newAdjuster != nullptr && Contains(*list, newAdjuster->TypeName()))
                module->AddPartModuleAdjusterNode([newAdjuster](ConfigNode& n) { newAdjuster->Save(n); });
        }
    }
}

// the vessel-id remap shared by the three part actions' OnVesselPersistentIdChanged / OnPartPersistentIdChanged / OnVesselsUndocking
void RemapUndocking(VesselPartIDPair& ids, Vessel* oldVessel, Vessel* newVessel) {
    if (ids.VesselID != oldVessel->persistentId && ids.VesselID != newVessel->persistentId) return;
    size_t num = 0;
    while (true) {
        if (num < newVessel->parts.size()) {
            if (ids.partID == newVessel->parts[num]->fields.persistentId) break;
            num++;
            continue;
        }
        return;
    }
    ids.VesselID = newVessel->persistentId;
}

// The vessel-name block shared by the GetNodeBodyParameterString of ActionPartFailure and ActionPartResourceDrain
std::string VesselPartText(MENode* node, const VesselPartIDPair& ids, const Editor::BaseAPField& field) {
    std::string text;
    std::string vesselName;
    bool flag = false;
    VesselSituation* vesselSituation = node->mission->GetVesselSituationByVesselID(ids.VesselID);
    if (vesselSituation != nullptr) {
        vesselName = vesselSituation->vesselName;
    } else if (node->mission->UpdateFromMappedVesselIDs(ids.VesselID, vesselName, vesselSituation)) {
        flag = true;
        if (vesselSituation == nullptr) text = Fmt("#autoLOC_8004190", {Tag("#autoLOC_8000001"), vesselName}) + "\n";
    }
    if (vesselSituation != nullptr) {
        text = Fmt("#autoLOC_8004190", {Tag("#autoLOC_8000001"), vesselName}) + "\n";
        if (flag) text = text + Fmt("#autoLOC_6002491", {vesselSituation->vesselName}) + "\n";
    }
    return text + Fmt("#autoLOC_8004190", {field.guiName, Localizer::Format(ids.partName)});
}

}  // namespace

// ---- ActionPartFailure -------------------------------------------------------------------------------------------------------

// ActionPartFailure.cs:101
ActionPartFailure::ActionPartFailure() : ActionModule(kTypeName) {
    title = Tag("#autoLOC_8000002");
    vesselPartIDs = VesselPartIDPair();
    if (HighLogic::LoadedSceneIsMissionBuilder) {
        GameEvents::Mission::onVesselSituationChanged.Add(this, &ActionPartFailure::onVesselSituationChanged);
        GameEvents::Mission::onMissionLoaded.Add(this, &ActionPartFailure::onVesselSituationChanged);
    }
}

// ActionPartFailure.cs:123
ActionPartFailure::~ActionPartFailure() {
    GameEvents::Mission::onVesselSituationChanged.Remove(this, &ActionPartFailure::onVesselSituationChanged);
    GameEvents::Mission::onMissionLoaded.Remove(this, &ActionPartFailure::onVesselSituationChanged);
}

// ActionPartFailure.cs:23
void ActionPartFailure::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    if (vesselPartIDs.VesselID == oldId) vesselPartIDs.VesselID = newId;
}

// ActionPartFailure.cs:32
void ActionPartFailure::OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) {
    if (vesselPartIDs.VesselID == vesselID && vesselPartIDs.partID == oldId) vesselPartIDs.partID = newId;
}

// ActionPartFailure.cs:45
void ActionPartFailure::OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) { RemapUndocking(vesselPartIDs, oldVessel, newVessel); }

// ActionPartFailure.cs:69
void ActionPartFailure::VesselPartSelectorControlCreated(Editor::IMEGUIVesselPartSelector* sender) {
    partSelectorRef = sender;
    OnPartIDChanged(vesselPartIDs);
}

// ActionPartFailure.cs:75
void ActionPartFailure::DynamicModuleListControlCreated(Editor::IMEGUIDynamicModuleListControl* sender) {
    dynamicModuleListRef = sender;
    dynamicModuleListRef->SetAddButtonText(Tag("#autoLOC_8002056"));
    OnPartIDChanged(vesselPartIDs);
}

// ActionPartFailure.cs:82
void ActionPartFailure::OnAdjusterListModified(DynamicModuleList*) { GameEvents::Mission::onFailureListChanged.Fire(); }

// ActionPartFailure.cs:87
void ActionPartFailure::onVesselSituationChanged() {
    if (node != nullptr && node->mission != nullptr && vesselPartIDs.partID != 0 && node->mission->GetSituationsIndexByPart(vesselPartIDs.partID) < 0) {
        vesselPartIDs = VesselPartIDPair();
        Modules = std::make_unique<DynamicModuleList>(node);
    }
    UpdateNodeBodyUI();
    if (partSelectorRef != nullptr) partSelectorRef->RefreshUI();
}

// ActionPartFailure.cs:113
void ActionPartFailure::Initialize(MENode* n) {
    ActionModule::Initialize(n);
    Modules = std::make_unique<DynamicModuleList>(n);
    if (n != nullptr) n->UpdateInfoOnRefresh = true;
}

// ActionPartFailure.cs:130
void ActionPartFailure::OnPartIDChanged(const VesselPartIDPair&) {
    if (partSelectorRef == nullptr || dynamicModuleListRef == nullptr) return;
    Part* selectedPart = partSelectorRef->GetSelectedPart();
    std::vector<std::string> list;
    if (selectedPart != nullptr) {
        const std::vector<std::string> names = ModuleNames(selectedPart);
        for (size_t i = 0; i < names.size(); ++i) {
            if (const std::vector<std::string>* collection = AdjustersFor(names[i])) list.insert(list.end(), collection->begin(), collection->end());
        }
    } else {
        for (const auto& kv : Adjusters::GetAdjusterTypeTables().adjusterTypesSupportedByPartModule) list.insert(list.end(), kv.second.begin(), kv.second.end());
    }
    if (Modules == nullptr) throw NullReferenceException();
    Modules->SetSupportedTypes(list);
    dynamicModuleListRef->RefreshUI();
}

// ActionPartFailure.cs:160
std::vector<IMENodeDisplay*> ActionPartFailure::GetInternalParametersToDisplay() {
    if (Modules == nullptr) throw NullReferenceException();
    return Modules->GetInternalParameters();
}

// ActionPartFailure.cs:165
Enumerator ActionPartFailure::Fire() {
    if (!MissionsEnvironment::MakingHistoryInstalled()) co_yield YieldInstruction::Null();
    if (vesselPartIDs.partID == 0) {
        const uint32_t key = node->mission->CurrentVesselID(node, vesselPartIDs.VesselID);
        Vessel* vessel = AwardEnvironment::VesselByPersistentId(key);
        if (vessel != nullptr) {
            addAdjustersVessel(vessel);
            co_yield YieldInstruction::Null();
        }
        // Debug.LogWarning("Unable to perform action on entire vessel! Vessel Not found!")
        co_yield YieldInstruction::Null();
    }
    Part* partout = FindLoadedPart(vesselPartIDs.partID);
    ProtoPartSnapshot* partout2 = nullptr;
    if (partout != nullptr) {
        if (Modules == nullptr) throw NullReferenceException();
        if (Modules->activeModules.size() > 0) {
            std::vector<Adjusters::AdjusterPartModuleBase*> list;
            for (auto& activeModule : Modules->activeModules) list.push_back(dynamic_cast<Adjusters::AdjusterPartModuleBase*>(activeModule.get()));
            if (NLE::partAddModuleAdjusters) {
                NLE::partAddModuleAdjusters(partout, list);
            } else {
                std::string ids;
                for (auto* a : list) ids += (ids.empty() ? "" : ",") + (a != nullptr ? a->adjusterID.ToString() : std::string("null"));
                NLE::adjusterCalls.push_back("AddPartModuleAdjusterList(" + std::to_string(partout->fields.persistentId) + ",[" + ids + "])");
            }
        } else {
            // Debug.LogWarning("Unable to perform action on part! No adjuster types were specified!")
        }
    } else if ((partout2 = FindUnloadedPart(vesselPartIDs.partID)) != nullptr) {
        if (Modules == nullptr) throw NullReferenceException();
        if (Modules->activeModules.size() > 0) {
            std::vector<Adjusters::AdjusterPartModuleBase*> list2;
            for (auto& activeModule2 : Modules->activeModules) list2.push_back(dynamic_cast<Adjusters::AdjusterPartModuleBase*>(activeModule2.get()));
            ProtoAddAdjusters(partout2, list2);
        } else {
            // Debug.LogWarning("Unable to perform action on part! No adjuster types were specified!")
        }
    } else {
        // Debug.LogWarning("Unable to perform action, part was not found!")
    }
    co_yield YieldInstruction::Null();
}

// ActionPartFailure.cs:227
void ActionPartFailure::addAdjustersVessel(Vessel* vessel) {
    for (size_t i = 0; i < vessel->parts.size(); ++i) {
        std::vector<Adjusters::AdjusterPartModuleBase*> list;
        Part* part = vessel->parts[i];
        const std::vector<std::string> names = ModuleNames(part);
        for (size_t j = 0; j < names.size(); ++j) {
            const std::vector<std::string>* list2 = AdjustersFor(names[j]);
            if (list2 == nullptr) continue;
            for (size_t k = 0; k < Modules->activeModules.size(); ++k) {
                if (Contains(*list2, Modules->activeModules[k]->TypeName())) list.push_back(dynamic_cast<Adjusters::AdjusterPartModuleBase*>(Modules->activeModules[k].get()));
            }
        }
        if (NLE::partAddModuleAdjusters) {
            NLE::partAddModuleAdjusters(part, list);
        } else {
            std::string ids;
            for (auto* a : list) ids += (ids.empty() ? "" : ",") + (a != nullptr ? a->adjusterID.ToString() : std::string("null"));
            NLE::adjusterCalls.push_back("AddPartModuleAdjusterList(" + std::to_string(part->fields.persistentId) + ",[" + ids + "])");
        }
    }
}

// ActionPartFailure.cs:253
std::string ActionPartFailure::GetInfo() { return Tag("#autoLOC_8004024"); }

// ActionPartFailure.cs:258
std::string ActionPartFailure::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (node != nullptr) node->UpdateInfoOnRefresh = true;
    std::string text;
    if (field.name == "vesselPartIDs") {
        if (node == nullptr) throw NullReferenceException();
        if (node->mission != nullptr) return VesselPartText(node, vesselPartIDs, field);
    }
    if (field.name == "Modules") {
        // `field.GetValue() is DynamicModuleList`: the field's host is this action, so the value is Modules
        if (Modules != nullptr) {
            DynamicModuleList& dynamicModuleList = *Modules;
            int num = 0;
            for (size_t i = 0; i < dynamicModuleList.activeModules.size(); ++i) {
                text += dynamicModuleList.activeModules[i]->GetDisplayName();
                num++;
                if (i != dynamicModuleList.activeModules.size() - 1) text += "\n";
            }
            if (num > 0) return Fmt("#autoLOC_8004190", {field.guiName, text});
        }
        return Fmt("#autoLOC_8004190", {field.guiName, Tag("#autoLOC_8100310")});
    }
    return ActionModule::GetNodeBodyParameterString(field);
}

// ActionPartFailure.cs:316
void ActionPartFailure::RunValidation(Editor::MissionEditorValidator* validator) { vesselPartIDs.ValidatePartAgainstCraft(node, validator); }

// ActionPartFailure.cs:324
void ActionPartFailure::Save(ConfigNode& cfg) const {
    ActionModule::Save(cfg);
    vesselPartIDs.Save(cfg);
    if (Modules == nullptr) throw NullReferenceException();
    Modules->Save(cfg);
}

// ActionPartFailure.cs:331
void ActionPartFailure::Load(const ConfigNode& cfg) {
    ActionModule::Load(cfg);
    vesselPartIDs.Load(cfg);
    if (Modules == nullptr) throw NullReferenceException();
    Modules->Load(cfg);
}

// ---- ActionPartRepair --------------------------------------------------------------------------------------------------------

// ActionPartRepair.cs:48
ActionPartRepair::ActionPartRepair() : ActionModule(kTypeName) {
    title = Tag("#autoLOC_8000063");
    vesselPartIDs = VesselPartIDPair();
    partModule = "";
    failureID = Guid::Empty();
    if (HighLogic::LoadedSceneIsMissionBuilder) {
        GameEvents::Mission::onFailureListChanged.Add(this, &ActionPartRepair::onFailureListChanged);
        GameEvents::Mission::onVesselSituationChanged.Add(this, &ActionPartRepair::onVesselSituationChanged);
        GameEvents::Mission::onMissionLoaded.Add(this, &ActionPartRepair::onVesselSituationChanged);
    }
}

// ActionPartRepair.cs:63
ActionPartRepair::~ActionPartRepair() {
    GameEvents::Mission::onFailureListChanged.Remove(this, &ActionPartRepair::onFailureListChanged);
    GameEvents::Mission::onVesselSituationChanged.Remove(this, &ActionPartRepair::onVesselSituationChanged);
    GameEvents::Mission::onMissionLoaded.Remove(this, &ActionPartRepair::onVesselSituationChanged);
}

// ActionPartRepair.cs:71
void ActionPartRepair::VesselPartSelectorControlCreated(Editor::IMEGUIVesselPartSelector*) { OnPartIDChanged(&vesselPartIDs); }

// ActionPartRepair.cs:76
void ActionPartRepair::OnRepairTypeControlCreated(Editor::IMEGUIDropdownList* parameter) { RepairTypeDropdown = parameter; }

// ActionPartRepair.cs:81
void ActionPartRepair::OnPartModuleDropdownControlCreated(Editor::IMEGUIDropdownList* parameter) { PartModuleDropdown = parameter; }

// ActionPartRepair.cs:86
void ActionPartRepair::OnFailureDropdownControlCreated(Editor::IMEGUIDropdownList* parameter) { FailureDropdown = parameter; }

// ActionPartRepair.cs:91
std::vector<Editor::MEGUIDropDownItem> ActionPartRepair::SetRepairTypeDropdown() {
    std::vector<Editor::MEGUIDropDownItem> list;
    vesselSituation = node->mission->GetVesselSituationByVesselID(vesselPartIDs.VesselID);
    playerCreated = true;
    if (vesselSituation != nullptr) playerCreated = vesselSituation->playerCreated;
    for (RepairChoices e : {RepairChoices::entireVessel, RepairChoices::onePart, RepairChoices::partModule, RepairChoices::failureID}) {
        const std::string name = dotnet::EnumToString(e);
        if ((playerCreated || vesselPartIDs.partID != 0 || !(name == "onePart")) && (!playerCreated || (!(name == "onePart") && !(name == "partModule"))))
            list.push_back({name, name, Library::EnumFieldText(e)});
    }
    return list;
}

// ActionPartRepair.cs:113
std::vector<Editor::MEGUIDropDownItem> ActionPartRepair::SetPartModuleNamesForDropdown() {
    std::vector<std::string> list;
    MissionCraft* craftBySituationsVesselID = node->mission->GetCraftBySituationsVesselID(vesselPartIDs.VesselID);
    if (craftBySituationsVesselID != nullptr) {
        if (craftBySituationsVesselID->CraftNode() == nullptr) throw NullReferenceException();
        const std::vector<ConfigNode*> nodes = craftBySituationsVesselID->CraftNode()->GetNodes("PART");
        for (size_t i = 0; i < nodes.size(); ++i) {
            uint32_t value = 0;
            nodes[i]->TryGetValue("persistentId", value);
            const std::vector<ConfigNode*> nodes2 = nodes[i]->GetNodes("MODULE");
            for (size_t j = 0; j < nodes2.size(); ++j) {
                if (vesselPartIDs.partID == 0 || (vesselPartIDs.partID != 0 && vesselPartIDs.partID == value)) {
                    const std::string* n = nodes2[j]->GetValuePtr("name");
                    if (n != nullptr && !Contains(list, *n)) list.push_back(*n);  // List.AddUnique(string)
                    if (n == nullptr) list.push_back(std::string());              // AddUnique(null): never matched below (ContainsKey(null) throws)
                }
            }
        }
    }
    std::vector<Editor::MEGUIDropDownItem> list2;
    for (size_t k = 0; k < list.size(); ++k) {
        if (AdjustersFor(list[k]) != nullptr) {
            // AssemblyLoader.GetClassByName(typeof(PartModule), list[k]) + AddComponent + GetModuleDisplayName() on a discarded GameObject
            std::optional<std::string> displayName = NLE::partModuleDisplayName ? NLE::partModuleDisplayName(list[k]) : std::nullopt;
            if (!displayName.has_value()) throw NullReferenceException();
            list2.push_back({list[k], list[k], *displayName});
        }
    }
    if (list2.size() == 0) list2.push_back({"None", "None Available", Tag("#autoLOC_8100310")});
    return list2;
}

// ActionPartRepair.cs:160
std::vector<Editor::MEGUIDropDownItem> ActionPartRepair::SetFailureNamesForDropdown() {
    std::vector<Editor::MEGUIDropDownItem> list;
    std::vector<ActionPartFailure*> allActionModules = node->mission->GetAllActionModules<ActionPartFailure>();
    for (size_t i = 0; i < allActionModules.size(); ++i) {
        if (allActionModules[i]->vesselPartIDs.VesselID != vesselPartIDs.VesselID) continue;
        if (allActionModules[i]->Modules == nullptr) throw NullReferenceException();
        for (size_t j = 0; j < allActionModules[i]->Modules->activeModules.size(); ++j) {
            if (auto* adjuster = dynamic_cast<Adjusters::AdjusterPartModuleBase*>(allActionModules[i]->Modules->activeModules[j].get()))
                list.push_back({adjuster->adjusterID.ToString(), adjuster->adjusterID.ToString(), adjuster->GetDisplayName()});
        }
    }
    if (list.size() == 0) list.push_back({"None Available", Guid::Empty().ToString(), Tag("#autoLOC_8100310")});
    return list;
}

// ActionPartRepair.cs:185
void ActionPartRepair::OnRepairTypeValueChange(Editor::IMEGUIDropdownList* sender, int) {
    std::optional<int> selected = sender->SelectedValueInt();
    if (selected.has_value() && node != nullptr) {
        switch (static_cast<RepairChoices>(*selected)) {
            case RepairChoices::entireVessel:
                if (FailureDropdown != nullptr) FailureDropdown->SetActive(false);
                if (PartModuleDropdown != nullptr) PartModuleDropdown->SetActive(false);
                break;
            case RepairChoices::onePart:
                if (FailureDropdown != nullptr) FailureDropdown->SetActive(false);
                if (PartModuleDropdown != nullptr) PartModuleDropdown->SetActive(false);
                break;
            case RepairChoices::partModule:
                if (FailureDropdown != nullptr) FailureDropdown->SetActive(false);
                if (PartModuleDropdown != nullptr) {
                    PartModuleDropdown->SetActive(true);
                    PartModuleDropdown->RebuildDropDown();
                }
                break;
            case RepairChoices::failureID:
                if (FailureDropdown != nullptr) {
                    FailureDropdown->SetActive(true);
                    FailureDropdown->RebuildDropDown();
                }
                if (PartModuleDropdown != nullptr) PartModuleDropdown->SetActive(false);
                break;
        }
    }
    UpdateNodeBodyUI();
}

// ActionPartRepair.cs:238
void ActionPartRepair::onFailureListChanged() {
    if (FailureDropdown != nullptr) FailureDropdown->RebuildDropDown();
}

// ActionPartRepair.cs:246
void ActionPartRepair::OnPartIDChanged(const VesselPartIDPair* newVesselPartIDs) {
    if (newVesselPartIDs != nullptr) {
        vesselPartIDs = *newVesselPartIDs;
        onVesselSituationChanged();
    }
}

// ActionPartRepair.cs:255
void ActionPartRepair::onVesselSituationChanged() {
    if (node != nullptr && node->mission != nullptr) {
        vesselSituation = node->mission->GetVesselSituationByVesselID(vesselPartIDs.VesselID);
        if (RepairTypeDropdown != nullptr) {
            RepairTypeDropdown->RebuildDropDown();
            OnRepairTypeValueChange(RepairTypeDropdown, RepairTypeDropdown->GetItemIndex(RepairTypeDropdown->SelectedValueInt()));
        }
        if (PartModuleDropdown != nullptr) PartModuleDropdown->RebuildDropDown();
        if (FailureDropdown != nullptr) FailureDropdown->RebuildDropDown();
        UpdateNodeBodyUI();
    }
}

// ActionPartRepair.cs:277
Enumerator ActionPartRepair::Fire() {
    if (!MissionsEnvironment::MakingHistoryInstalled()) co_yield YieldInstruction::Null();
    Vessel* vessel = nullptr;
    if (FindVesselById(vesselPartIDs.VesselID, vessel)) {
        if (vessel->loaded) {
            switch (repairType) {
                case RepairChoices::entireVessel:
                    RepairFailures(vessel);
                    break;
                case RepairChoices::onePart: {
                    Part* partout = FindLoadedPart(vesselPartIDs.partID);
                    if (partout != nullptr) RepairFailures(partout);
                    break;
                }
                case RepairChoices::partModule:
                    RepairFailures(vessel, partModule);
                    break;
                case RepairChoices::failureID:
                    RepairFailures(vessel, "", true);
                    break;
            }
        } else {
            switch (repairType) {
                case RepairChoices::entireVessel:
                    RepairFailures(vessel->protoVessel);
                    break;
                case RepairChoices::onePart: {
                    ProtoPartSnapshot* partout2 = FindUnloadedPart(vesselPartIDs.partID);
                    if (partout2 != nullptr) partout2->ProtoPartRepair();
                    break;
                }
                case RepairChoices::partModule:
                    RepairFailures(vessel->protoVessel, partModule);
                    break;
                case RepairChoices::failureID:
                    RepairFailures(vessel->protoVessel, "", true);
                    break;
            }
        }
    } else {
        // Debug.LogWarning("Unable to perform action, vessel was not found!")
    }
    co_yield YieldInstruction::Null();
}

// ActionPartRepair.cs:342
void ActionPartRepair::RepairFailures(Vessel* vessel, const std::string& repairpartModule, bool useGuid) {
    for (size_t i = 0; i < vessel->Parts().size(); ++i) RepairFailures(vessel->Parts()[i], repairpartModule, useGuid);
}

// ActionPartRepair.cs:350
void ActionPartRepair::RepairFailures(Part* part, const std::string& repairpartModule, bool useGuid) {
    const std::vector<std::string> names = ModuleNames(part);
    for (size_t i = 0; i < names.size(); ++i) {
        if (!(repairpartModule == "") && !(repairpartModule == names[i])) continue;
        const std::vector<Guid> ids = NLE::partModuleAdjusterIds ? NLE::partModuleAdjusterIds(part, static_cast<int>(i)) : std::vector<Guid>();
        for (int num = static_cast<int>(ids.size()) - 1; num >= 0; --num) {
            if (!useGuid || failureID == ids[static_cast<size_t>(num)]) {
                if (NLE::partRemoveModuleAdjuster) {
                    NLE::partRemoveModuleAdjuster(part, static_cast<int>(i), ids[static_cast<size_t>(num)]);
                } else {
                    NLE::adjusterCalls.push_back("RemovePartModuleAdjuster(" + std::to_string(part->fields.persistentId) + "," + std::to_string(i) + "," + ids[static_cast<size_t>(num)].ToString() + ")");
                }
            }
        }
    }
}

// ActionPartRepair.cs:369
void ActionPartRepair::RepairFailures(ProtoVessel* vessel, const std::string& repairpartModule, bool useGuid) {
    for (size_t i = 0; i < vessel->protoPartSnapshots.size(); ++i) RepairFailures(vessel->protoPartSnapshots[i].get(), repairpartModule, useGuid);
}

// ActionPartRepair.cs:377
void ActionPartRepair::RepairFailures(ProtoPartSnapshot* part, const std::string& repairpartModule, bool useGuid) {
    for (size_t i = 0; i < part->modules.size(); ++i) {
        ProtoPartModuleSnapshot* protoPartModuleSnapshot = part->modules[i].get();
        if (repairpartModule == "" || (protoPartModuleSnapshot->moduleName.has_value() && repairpartModule == *protoPartModuleSnapshot->moduleName)) {
            if (useGuid) {
                protoPartModuleSnapshot->RemovePartModuleAdjuster(failureID);
            } else {
                protoPartModuleSnapshot->ProtoPartModuleRepair();
            }
        }
    }
}

// ActionPartRepair.cs:396
std::string ActionPartRepair::GetInfo() { return Tag("#autoLOC_8004025"); }

// ActionPartRepair.cs:401
std::string ActionPartRepair::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (node != nullptr) node->UpdateInfoOnRefresh = true;
    if (field.name == "vesselPartIDs") {
        if (node == nullptr) throw NullReferenceException();
        if (node->mission != nullptr) {
            std::string text;
            std::string vesselName;
            bool flag = false;
            VesselSituation* vesselSituationByVesselID = node->mission->GetVesselSituationByVesselID(vesselPartIDs.VesselID);
            if (vesselSituationByVesselID != nullptr) {
                vesselName = vesselSituationByVesselID->vesselName;
            } else if (node->mission->UpdateFromMappedVesselIDs(vesselPartIDs.VesselID, vesselName, vesselSituationByVesselID)) {
                flag = true;
                if (vesselSituationByVesselID == nullptr) text = Tag("#autoLOC_8000001") + ": " + Localizer::Format(vesselName) + "\n";
            }
            if (vesselSituationByVesselID != nullptr) {
                text = Tag("#autoLOC_8000001") + ": " + Localizer::Format(vesselName) + "\n";
                if (flag) text = text + Fmt("#autoLOC_6002491", {vesselSituationByVesselID->vesselName}) + "\n";
            }
            return text + field.guiName + ": " + Localizer::Format(vesselPartIDs.partName);
        }
    }
    if (field.name == "repairType") {
        switch (repairType) {
            default:
                return Fmt("#autoLOC_8004190", {Tag("#autoLOC_8100122"), Tag("#autoLOC_8100115")});
            case RepairChoices::onePart:
                return Fmt("#autoLOC_8004190", {Library::EnumFieldText(RepairChoices::onePart), Localizer::Format(vesselPartIDs.partName)});
            case RepairChoices::partModule:
                if (node == nullptr) throw NullReferenceException();
                if (node->mission != nullptr) return Fmt("#autoLOC_8004190", {Library::EnumFieldText(RepairChoices::partModule), partModule});
                break;
            case RepairChoices::failureID: {
                if (node == nullptr) throw NullReferenceException();
                if (!(node->mission != nullptr)) break;
                std::vector<ActionPartFailure*> allActionModules = node->mission->GetAllActionModules<ActionPartFailure>();
                for (size_t i = 0; i < allActionModules.size(); ++i) {
                    if (allActionModules[i]->vesselPartIDs.VesselID != vesselPartIDs.VesselID) continue;
                    if (allActionModules[i]->Modules == nullptr) throw NullReferenceException();
                    for (size_t j = 0; j < allActionModules[i]->Modules->activeModules.size(); ++j) {
                        auto* adjuster = dynamic_cast<Adjusters::AdjusterPartModuleBase*>(allActionModules[i]->Modules->activeModules[j].get());
                        if (adjuster != nullptr && adjuster->adjusterID == failureID) return Fmt("#autoLOC_8004190", {Tag("#autoLOC_8100122"), adjuster->GetDisplayName()});
                    }
                }
                break;
            }
        }
    }
    if (!(field.name == "partModule") && !(field.name == "failureID")) return ActionModule::GetNodeBodyParameterString(field);
    return "";
}

// ActionPartRepair.cs:481
void ActionPartRepair::RunValidation(Editor::MissionEditorValidator* validator) { vesselPartIDs.ValidatePartAgainstCraft(node, validator); }

// ActionPartRepair.cs:489
void ActionPartRepair::Save(ConfigNode& cfg) const {
    ActionModule::Save(cfg);
    vesselPartIDs.Save(cfg);
    const_cast<VesselPartIDPair&>(vesselPartIDs).Load(cfg);  // the C# calls vesselPartIDs.Load(node) right after Save(node), on the node just written
    cfg.AddValue("repairType", dotnet::EnumToString(repairType));
    if (partModule != "None Available") cfg.AddValue("partModule", partModule);
    if (!(failureID == Guid::Empty())) cfg.AddValue("failureID", failureID.ToString());
}

// ActionPartRepair.cs:505
void ActionPartRepair::Load(const ConfigNode& cfg) {
    ActionModule::Load(cfg);
    vesselPartIDs.Load(cfg);
    save_detail::TryGetEnum(cfg, "repairType", repairType, RepairChoices::entireVessel);
    if (cfg.HasValue("partModule")) partModule = *cfg.GetValue("partModule");
    if (cfg.HasValue("failureID")) {
        failureID = Guid::Parse(*cfg.GetValue("failureID"));
    } else {
        failureID = MissionsEnvironment::NewGuid();
    }
}

// ---- ActionPartResourceDrain -------------------------------------------------------------------------------------------------

// ActionPartResourceDrain.cs:49
ActionPartResourceDrain::ActionPartResourceDrain() : ActionModule(kTypeName) {
    title = Tag("#autoLOC_8000007");
    vesselPartIDs = VesselPartIDPair();
    restartOnSceneLoad = true;
    leftToDrain = amountToDrain;
}

// ActionPartResourceDrain.cs:58
void ActionPartResourceDrain::OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) {
    if (vesselPartIDs.VesselID == oldId) vesselPartIDs.VesselID = newId;
}

// ActionPartResourceDrain.cs:67
void ActionPartResourceDrain::OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) {
    if (vesselPartIDs.VesselID == vesselID && vesselPartIDs.partID == oldId) vesselPartIDs.partID = newId;
}

// ActionPartResourceDrain.cs:76
void ActionPartResourceDrain::OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) { RemapUndocking(vesselPartIDs, oldVessel, newVessel); }

// ActionPartResourceDrain.cs:100
Enumerator ActionPartResourceDrain::Fire() {
    isRunning = true;
    if (std::getenv("KSP_DBG")) std::fprintf(stderr, "DRAIN fire setup=%d\n", (int)setup);
    if (!setup) {
        if (!Setup()) {
            isRunning = false;
            co_yield YieldInstruction::Null();
        }
        setup = true;
    }
    while (true) {
        if (vesselDrain) {
            if (vessel == nullptr) {
                vesselFound = findVessel();
                if (!vesselFound) {
                    // Debug.LogWarning("[MissionExpansions] ActionPartResourceDrain failed to find vessel id")
                    co_return;
                }
            }
            drainVessel();
        } else {
            if ((partLoaded && part == nullptr) || (!partLoaded && protopart == nullptr)) {
                partFound = findPart();
                if (!partFound) break;
            }
            drainPart();
        }
        if (fx(leftToDrain) > epsilonDrain) co_yield YieldInstruction::WaitForSeconds(drainEveryXSecs);
        if (!(fx(leftToDrain) > epsilonDrain)) {
            isRunning = false;
            co_return;
        }
    }
    // Debug.LogWarning("[MissionExpansions] ActionPartResourceDrain failed to find part id")
}

// ActionPartResourceDrain.cs:105
bool ActionPartResourceDrain::Setup() {
    if (std::getenv("KSP_DBG")) std::fprintf(stderr, "DRAIN Setup enter\n");
    if (vesselPartIDs.VesselID != 0 && amountToDrain != 0.0f && !(resourceName == "") && SaveEnvironment::Current().persistentIds.fetch) {
        if (vesselPartIDs.VesselID != 0 && vesselPartIDs.partID == 0) {
            vesselDrain = true;
            vesselFound = findVessel();
            if (!vesselFound) return false;  // Debug.LogWarning("[ActionPartResourceDrain] Failed to find vessel id")
        } else {
            vesselDrain = false;
            partFound = findPart();
            if (!partFound) return false;  // Debug.LogWarning("[ActionPartResourceDrain] Failed to find part id")
        }
        PartResourceLibrary* lib = PartResourceLibrary::Instance();
        if (lib == nullptr) throw NullReferenceException();
        resourcedef = lib->GetDefinition(resourceName);
        if (std::getenv("KSP_DBG")) std::fprintf(stderr, "DRAIN got def\n");
        if (resourcedef == nullptr) return false;  // Debug.LogWarning("[MissionExpansions] ActionPartResourceDrain failed to find resource")
        if (timePeriod <= 0.0f) {
            drainPerSecond = amountToDrain;
        } else {
            drainPerSecond = f32store(fx(amountToDrain) / fx(timePeriod));
        }
        leftToDrain = amountToDrain;
        if (std::getenv("KSP_DBG")) std::fprintf(stderr, "DRAIN setup done\n");
        return true;
    }
    // Debug.LogWarning("[ActionPartResourceDrain] Failed to initialize ...")
    return false;
}

// ActionPartResourceDrain.cs:197
bool ActionPartResourceDrain::findVessel() {
    if (std::getenv("KSP_DBG")) std::fprintf(stderr, "DRAIN findVessel enter\n");
    vesselFound = false;
    const uint32_t key = node->mission->CurrentVesselID(node, vesselPartIDs.VesselID);
    Vessel* v = AwardEnvironment::VesselByPersistentId(key);
    if (v != nullptr) {
        vesselFound = true;
        vessel = v;
    }
    return vesselFound;
}

// ActionPartResourceDrain.cs:209
bool ActionPartResourceDrain::findPart() {
    if (std::getenv("KSP_DBG")) std::fprintf(stderr, "DRAIN findPart enter\n");
    part = FindLoadedPart(vesselPartIDs.partID);
    if (std::getenv("KSP_DBG")) std::fprintf(stderr, "DRAIN found loaded %p id=%u\n", (void*)part, vesselPartIDs.partID);
    partLoaded = part != nullptr;
    if (!partLoaded) {
        protopart = FindUnloadedPart(vesselPartIDs.partID);
        if (std::getenv("KSP_DBG")) std::fprintf(stderr, "DRAIN found proto %p\n", (void*)protopart);
        return protopart != nullptr;
    }
    return true;
}

// ActionPartResourceDrain.cs:220
void ActionPartResourceDrain::drainVessel() {
    if (std::getenv("KSP_DBG")) std::fprintf(stderr, "DRAIN drainVessel loaded=%d proto=%p\n", (int)vessel->loaded, (void*)vessel->protoVessel);
    float num = f32store(fx(drainPerSecond) * fx(drainEveryXSecs));
    if (vessel->loaded) {
        if (resourcedef == nullptr) throw NullReferenceException();  // resourcedef.id
        double num2 = 0.0;
        if (NLE::vesselRequestResource) num2 = NLE::vesselRequestResource(vessel, vessel->rootPart, resourcedef->id(), fx(num), false);
        if (num2 <= 0.0) {
            leftToDrain = 0.0f;
        } else {
            GameEvents::Mission::onNodeChangedVesselResources.Fire(node, vessel, vessel->rootPart, nullptr);
        }
        leftToDrain = f32store(fx(leftToDrain) - fx(f32store(num2)));
        return;
    }
    float num3 = 0.0f;
    for (size_t i = 0; i < vessel->protoVessel->protoPartSnapshots.size(); ++i) {
        float num4 = drainUnloadedPart(vessel->protoVessel->protoPartSnapshots[i].get());
        if (!(num4 <= 0.0f)) {
            num3 = f32store(fx(num3) + fx(num4));
            break;
        }
    }
    if (num3 <= 0.0f) leftToDrain = 0.0f;
    leftToDrain = f32store(fx(leftToDrain) - fx(num3));
}

// ActionPartResourceDrain.cs:254
void ActionPartResourceDrain::drainPart() {
    if (std::getenv("KSP_DBG")) std::fprintf(stderr, "DRAIN drainPart\n");
    float num = f32store(fx(drainPerSecond) * fx(drainEveryXSecs));
    if (partLoaded) {
        if (resourcedef == nullptr) throw NullReferenceException();  // resourcedef.id
        double num2 = 0.0;
        if (NLE::partRequestResource) {
            num2 = NLE::partRequestResource(part, resourcedef->id(), fx(num));
        } else {
            num2 = part->RequestResource(resourcedef->id(), fx(num), ResourceFlowMode::NO_FLOW);
        }
        if (num2 <= 0.0) {
            leftToDrain = 0.0f;
        } else {
            GameEvents::Mission::onNodeChangedVesselResources.Fire(node, vessel, part, nullptr);
        }
        leftToDrain = f32store(fx(leftToDrain) - fx(f32store(num2)));
    } else {
        float num3 = drainUnloadedPart(protopart);
        if (num3 <= 0.0f) {
            leftToDrain = 0.0f;
        } else {
            GameEvents::Mission::onNodeChangedVesselResources.Fire(node, vessel, nullptr, protopart);
        }
        leftToDrain = f32store(fx(leftToDrain) - fx(num3));
    }
}

// ActionPartResourceDrain.cs:285
float ActionPartResourceDrain::drainUnloadedPart(ProtoPartSnapshot* InProtoPart) {
    float num = f32store(fx(drainPerSecond) * fx(drainEveryXSecs));
    size_t num2 = 0;
    while (true) {
        if (num2 < InProtoPart->resources.size()) {
            const auto& rn = InProtoPart->resources[num2]->resourceName;
            if (rn.has_value() && *rn == resourceName) break;
            num2++;
            continue;
        }
        return 0.0f;
    }
    if (InProtoPart->resources[num2]->amount >= fx(num)) {
        InProtoPart->resources[num2]->amount -= fx(num);
        return num;
    }
    return 0.0f;
}

// ActionPartResourceDrain.cs:310
std::vector<Editor::MEGUIDropDownItem> ActionPartResourceDrain::APRD_SetDropDownValues() {
    std::vector<Editor::MEGUIDropDownItem> list;
    PartResourceLibrary* lib = PartResourceLibrary::Instance();
    if (lib == nullptr) throw NullReferenceException();
    for (const auto& current : lib->resourceDefinitions) list.push_back({current->name(), current->name(), current->displayName()});
    return list;
}

// ActionPartResourceDrain.cs:323
std::string ActionPartResourceDrain::GetNodeBodyParameterString(const Editor::BaseAPField& field) {
    if (node != nullptr) node->UpdateInfoOnRefresh = true;
    if (field.name == "vesselPartIDs") {
        if (node == nullptr) throw NullReferenceException();
        if (node->mission != nullptr) return VesselPartText(node, vesselPartIDs, field);
    }
    if (field.name == "resourceName") {
        std::string text2 = Tag("#autoLOC_6003083");
        if (!resourceName.empty()) {
            PartResourceLibrary* lib = PartResourceLibrary::Instance();
            if (lib == nullptr) throw NullReferenceException();
            resourcedef = lib->GetDefinition(resourceName);
            if (resourcedef != nullptr) text2 = resourcedef->displayName();
        }
        return Fmt("#autoLOC_8004190", {field.guiName, text2});
    }
    return ActionModule::GetNodeBodyParameterString(field);
}

// ActionPartResourceDrain.cs:373
std::string ActionPartResourceDrain::GetInfo() { return Tag("#autoLOC_8004004"); }

// ActionPartResourceDrain.cs:378
void ActionPartResourceDrain::RunValidation(Editor::MissionEditorValidator* validator) { vesselPartIDs.ValidatePartAgainstCraft(node, validator); }

// ActionPartResourceDrain.cs:386
void ActionPartResourceDrain::Save(ConfigNode& cfg) const {
    ActionModule::Save(cfg);
    cfg.AddValue("timePeriod", timePeriod);
    cfg.AddValue("amountToDrain", amountToDrain);
    cfg.AddValue("resourceName", resourceName);
    cfg.AddValue("leftToDrain", leftToDrain);
    vesselPartIDs.Save(cfg);
}

// ActionPartResourceDrain.cs:396
void ActionPartResourceDrain::Load(const ConfigNode& cfg) {
    ActionModule::Load(cfg);
    cfg.TryGetValue("timePeriod", timePeriod);
    cfg.TryGetValue("amountToDrain", amountToDrain);
    cfg.TryGetValue("resourceName", resourceName);
    cfg.TryGetValue("leftToDrain", leftToDrain);
    vesselPartIDs.Load(cfg);
}

}  // namespace ksp::Expansions::Missions::Actions
