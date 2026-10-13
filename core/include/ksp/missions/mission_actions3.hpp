// WP-54c: Assembly-CSharp/Expansions/Missions/Actions/{ActionPartFailure,ActionPartRepair,ActionPartResourceDrain}.cs (Expansions.Missions.Actions).
// Conventions: see node_library.hpp. The editor-control callbacks (OnXControlCreated, OnRepairTypeValueChange, the Set*Dropdown item builders) drive
// MEGUI widgets through the seams of mission_display.hpp; the data they keep in sync is the data the actions read and is ported.
// The loaded-part adjuster calls (Part.AddPartModuleAdjusterList, PartModule.CurrentModuleAdjusterList / RemovePartModuleAdjuster) go through
// NodeLibraryEnvironment (the PartModule adjuster list is the part-module port's); the unloaded-part calls act on the real ProtoPartSnapshot /
// ProtoPartModuleSnapshot config nodes (ADJUSTERS / ADJUSTERSTOREMOVE).
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/missions/dynamic_module.hpp"
#include "ksp/missions/mission_actions.hpp"
#include "ksp/missions/mission_adjusters.hpp"

namespace ksp {
class PartResourceDefinition;
}

namespace ksp::Expansions::Missions::Actions {

// ActionPartFailure.cs:11
class ActionPartFailure : public ActionModule {
public:
    static constexpr const char* kTypeName = "ActionPartFailure";
    ActionPartFailure();                                     // :88 Awake
    ~ActionPartFailure() override;                           // :119 OnDestroy
    VesselPartIDPair vesselPartIDs;                          // :14
    std::unique_ptr<DynamicModuleList> Modules;              // :19
    Editor::IMEGUIVesselPartSelector* partSelectorRef = nullptr;           // :16
    Editor::IMEGUIDynamicModuleListControl* dynamicModuleListRef = nullptr;  // :21
    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) override;  // :23
    void OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) override;  // :32
    void OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) override;     // :44
    void VesselPartSelectorControlCreated(Editor::IMEGUIVesselPartSelector* sender);   // :66
    void DynamicModuleListControlCreated(Editor::IMEGUIDynamicModuleListControl* sender);  // :72
    void OnAdjusterListModified(DynamicModuleList* newModuleList);  // :79
    void Initialize(MENode* node) override;                  // :109
    std::vector<IMENodeDisplay*> GetInternalParametersToDisplay() override;  // :152
    Enumerator Fire() override;                              // :157
    std::string GetInfo() override;                          // :260
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :265
    void RunValidation(Editor::MissionEditorValidator* validator) override;  // :322
    void Save(ConfigNode& node) const override;              // :331
    void Load(const ConfigNode& node) override;              // :339
    void onVesselSituationChanged();                         // :84
    void OnPartIDChanged(const VesselPartIDPair& newVesselPartIDs);  // :127

private:
    void addAdjustersVessel(Vessel* vessel);                 // :235
};

// ActionPartRepair.cs:12
class ActionPartRepair : public ActionModule {
public:
    enum class RepairChoices { entireVessel = 0, onePart = 1, partModule = 2, failureID = 3 };  // :14
    static constexpr const char* kTypeName = "ActionPartRepair";
    ActionPartRepair();                                      // :54 Awake
    ~ActionPartRepair() override;                            // :72 OnDestroy
    VesselPartIDPair vesselPartIDs;                          // :29
    RepairChoices repairType = RepairChoices::entireVessel;  // :32
    std::string partModule;                                  // :37
    Guid failureID;                                          // :42
    Editor::IMEGUIDropdownList* RepairTypeDropdown = nullptr;   // :34
    Editor::IMEGUIDropdownList* FailureDropdown = nullptr;      // :44
    Editor::IMEGUIDropdownList* PartModuleDropdown = nullptr;   // :50
    void VesselPartSelectorControlCreated(Editor::IMEGUIVesselPartSelector* sender);  // :81
    void OnRepairTypeControlCreated(Editor::IMEGUIDropdownList* parameter);  // :86
    void OnPartModuleDropdownControlCreated(Editor::IMEGUIDropdownList* parameter);  // :91
    void OnFailureDropdownControlCreated(Editor::IMEGUIDropdownList* parameter);  // :96
    std::vector<Editor::MEGUIDropDownItem> SetRepairTypeDropdown();     // :101
    std::vector<Editor::MEGUIDropDownItem> SetPartModuleNamesForDropdown();  // :126
    std::vector<Editor::MEGUIDropDownItem> SetFailureNamesForDropdown();     // :170
    void OnRepairTypeValueChange(Editor::IMEGUIDropdownList* sender, int newIndex);  // :192
    void onFailureListChanged();                             // :250
    void OnPartIDChanged(const VesselPartIDPair* newVesselPartIDs);  // :259
    void onVesselSituationChanged();                         // :268
    Enumerator Fire() override;                              // :295
    std::string GetInfo() override;                          // :445
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :450
    void RunValidation(Editor::MissionEditorValidator* validator) override;  // :522
    void Save(ConfigNode& node) const override;              // :531
    void Load(const ConfigNode& node) override;              // :547

private:
    void RepairFailures(Vessel* vessel, const std::string& repairpartModule = "", bool useGuid = false);  // :396
    void RepairFailures(Part* part, const std::string& repairpartModule = "", bool useGuid = false);     // :404
    void RepairFailures(ProtoVessel* vessel, const std::string& repairpartModule = "", bool useGuid = false);  // :422
    void RepairFailures(ProtoPartSnapshot* part, const std::string& repairpartModule = "", bool useGuid = false);  // :430
    VesselSituation* vesselSituation = nullptr;              // :46
    bool playerCreated = false;                              // :48
};

// ActionPartResourceDrain.cs:9
class ActionPartResourceDrain : public ActionModule {
public:
    static constexpr const char* kTypeName = "ActionPartResourceDrain";
    ActionPartResourceDrain();                               // :46 Awake
    float timePeriod = 0.0f;                                 // :12
    float amountToDrain = 0.0f;                              // :15
    VesselPartIDPair vesselPartIDs;                          // :18
    std::string resourceName;                                // :21
    void OnVesselPersistentIdChanged(uint32_t oldId, uint32_t newId) override;  // :54
    void OnPartPersistentIdChanged(uint32_t vesselID, uint32_t oldId, uint32_t newId) override;  // :63
    void OnVesselsUndocking(Vessel* oldVessel, Vessel* newVessel) override;     // :72
    Enumerator Fire() override;                              // :95
    std::vector<Editor::MEGUIDropDownItem> APRD_SetDropDownValues();  // :302
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :313
    std::string GetInfo() override;                          // :362
    void RunValidation(Editor::MissionEditorValidator* validator) override;  // :367
    void Save(ConfigNode& node) const override;              // :375
    void Load(const ConfigNode& node) override;              // :386
    // the private run-time state the C# keeps in fields (read by the gate)
    float LeftToDrain() const { return leftToDrain; }
    bool VesselDrain() const { return vesselDrain; }

private:
    bool Setup();                                            // :147
    bool findVessel();                                       // :186
    bool findPart();                                         // :198
    void drainVessel();                                      // :208
    void drainPart();                                        // :243
    float drainUnloadedPart(ProtoPartSnapshot* InProtoPart);  // :275

    Part* part = nullptr;                                    // :23
    ProtoPartSnapshot* protopart = nullptr;                  // :25
    const PartResourceDefinition* resourcedef = nullptr;     // :27
    bool vesselDrain = false;                                // :29
    Vessel* vessel = nullptr;                                // :31
    bool vesselFound = false;                                // :33
    bool partLoaded = false;                                 // :35
    bool partFound = false;                                  // :37
    bool setup = false;                                      // :39
    float drainPerSecond = 0.0f;                             // :41
    float leftToDrain = 0.0f;                                // :47
};

}  // namespace ksp::Expansions::Missions::Actions

template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Actions::ActionPartRepair::RepairChoices> {
    using E = ksp::Expansions::Missions::Actions::ActionPartRepair::RepairChoices;
    static constexpr bool flags = false;
    // ActionPartRepair.cs:14
    static constexpr EnumMember<E> members[] = {{E::entireVessel, "entireVessel", "#autoLOC_8100115"},
                                                {E::onePart, "onePart", "#autoLOC_8100116"},
                                                {E::partModule, "partModule", "#autoLOC_8100117"},
                                                {E::failureID, "failureID", "#autoLOC_8100118"}};
};
