// WP-15 part modules of the resource system:
//   ModuleResourceAutoShiftState.cs, ModuleToggleCrossfeed.cs (+ IToggleCrossfeed.cs), CompoundParts/CModuleFuelLine.cs,
//   EVAResourceContainer.cs.
// Registered with the ModuleFactory by RegisterBuiltinModules (src/modules/module_behaviours.cpp); the schema entries
// (KSPFields, KSPEvents, KSPActions) come from the generated module_schema.inc / module_members.inc.
//
// Not modelled (UI / Unity): UI_FloatRange limits and SetSceneVisibility of the auto-shift fields, guiName rebuilding for the PAW,
// the stack icon colour of the fuel line, CModuleFuelLine's EVA-construction check, EVAResourceContainer's coroutine and screen
// messages (the transfer loop is `TransferStep`, one iteration per call).
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "ksp/modules/module_resource.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/modules/part_structural_modules.hpp"
#include "ksp/resources/resource_types.hpp"

namespace ksp {

class Part;
struct PartResource;

// ModuleResourceAutoShiftState.cs:4
class ModuleResourceAutoShiftState : public PartModule {
public:
    explicit ModuleResourceAutoShiftState(const ModuleClassInfo& cls) : PartModule(cls) {}
    ModuleResourceAutoShiftState();  // `new ModuleResourceAutoShiftState()` (ModuleResource.OnAwake): no Awake, no field lists
    PartResource* partResource = nullptr;  // :52
    ModuleResource* resource = nullptr;    // :54
    bool destroyed = false;                // Object.Destroy(this) was called
    bool warningVisible = false;           // the last ToggleWarningVisibility state (the UI_Label visibility)

    void OnStart(StartState state) override;                // :56
    void ModifyStartUpPercent();                            // :163
    void ModifyShutOffAmount();                             // :175
    void ModifyStartUpAmount();                             // :187
    void ModifyShutOffPercent();                            // :199
    void ModifyAmountPercent();                             // :211
    void ToggleWarningVisibility(bool state);               // :233
};

// ModuleToggleCrossfeed.cs:3
class ModuleToggleCrossfeed : public PartModule, public IToggleCrossfeed {
public:
    using PartModule::PartModule;
    bool defaultCrossfeedStatus = false;                    // :40 (public, serialized: Instantiate copies it)
    // technology state hook (ResearchAndDevelopment.GetTechnologyState(tech) == Available); unset: available
    static inline std::function<bool(const std::string&)> technologyAvailable;

    void ToggleEvent();                                     // :43
    void ToggleAction(KSPActionParam& param);               // :67 [KSPAction]
    void ToggleAction(int actionType);                      // :78 ToggleAction(KSPActionType): 1 = Deactivate, otherwise Activate
    void EnableAction(KSPActionParam& param);               // :91
    void DisableAction(KSPActionParam& param);              // :103
    void Start();                                           // :114
    void OnAwake() override;                                // :119
    void OnLoad(const ConfigNode& node) override;           // :132
    void OnStart(StartState state) override;                // :138
    void UpdateCrossfeed(bool fireEvent);                   // :155
    bool CrossfeedToggleableEditor() override;              // :176
    bool CrossfeedToggleableFlight() override;              // :181
    bool CrossfeedRequiresTech() override;                  // :186
    std::string CrossfeedTech() override;                   // :191
    bool CrossfeedHasTech() override;                       // :196
    std::string GetModuleDisplayName() override;            // :205
    void CopySerializedFrom(const PartModule& from) override;

private:
    void Activate();                                        // :96
    void Deactivate();                                      // :108
    BaseEvent* toggleE = nullptr;
    BaseAction* toggleA = nullptr;
    BaseAction* enableA = nullptr;
    BaseAction* disableA = nullptr;
};

// CompoundParts/CModuleFuelLine.cs:7: a fuel line (CompoundPart) adds itself to its target's fuelLookupTargets
class CModuleFuelLine : public CompoundPartModule, public IModuleInfo {
public:
    using CompoundPartModule::CompoundPartModule;
    void OnTargetSet(Part* target) override;                // :12
    void OnTargetLost() override;                           // :19
    void CloseFuelLine();                                   // :24
    std::string GetInfo() override;                         // :43
    std::string GetModuleTitle() override { return "Fuel Line"; }  // :48
    std::string GetPrimaryField() override { return ""; }          // :58
    std::string GetModuleDisplayName() override;            // :63
    IModuleInfo* AsModuleInfo() override { return this; }
    static void CacheLocalStrings();                        // :68
private:
    static inline std::string cacheAutoLOC_216750, cacheAutoLOC_216757;
};

// EVAResourceContainer.cs:7
class EVAResourceContainer : public PartModule {
public:
    using PartModule::PartModule;
    PartResource* resource = nullptr;                       // :26
    void OnStart(StartState state) override;                // :28
    void OnDestroy();                                       // :35
    void GetResourceExternalEvent(Part* activeVesselRoot);  // :62 [KSPEvent] (the active vessel's root part is passed in)
    void PutResourceExternalEvent(Part* activeVesselRoot);  // :78
    void StopTransfer();                                    // :93
    // one iteration of the transferResources coroutine (:99); false when the loop ends
    static bool TransferStep(PartResource& src, PartResource& dest, double& transferRate, float maxDist, double sqrDistance);
    bool transferring = false;                              // the coroutine is running (Events["StopTransfer"].active)
    PartResource* transferSrc = nullptr;
    PartResource* transferDest = nullptr;
    void updateModuleUI();                                  // :48
};

namespace detail {
void RegisterResourceModules();  // called by RegisterBuiltinModules
}

}  // namespace ksp
