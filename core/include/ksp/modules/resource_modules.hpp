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
    PartResource* partResource = nullptr;  // :46
    ModuleResource* resource = nullptr;    // :48
    bool destroyed = false;                // Object.Destroy(this) was called
    bool warningVisible = false;           // the last ToggleWarningVisibility state (the UI_Label visibility)

    void OnStart(StartState state) override;                // :50
    void ModifyStartUpPercent();                            // :122
    void ModifyShutOffAmount();                             // :132
    void ModifyStartUpAmount();                             // :142
    void ModifyShutOffPercent();                            // :152
    void ModifyAmountPercent();                             // :162
    void ToggleWarningVisibility(bool state);               // :183
};

// ModuleToggleCrossfeed.cs:3
class ModuleToggleCrossfeed : public PartModule, public IToggleCrossfeed {
public:
    using PartModule::PartModule;
    bool defaultCrossfeedStatus = false;                    // :27 (public, serialized: Instantiate copies it)
    // technology state hook (ResearchAndDevelopment.GetTechnologyState(tech) == Available); unset: available
    static inline std::function<bool(const std::string&)> technologyAvailable;

    void ToggleEvent();                                     // :31
    void ToggleAction(KSPActionParam& param);               // :53 [KSPAction]
    void ToggleAction(int actionType);                      // :65 ToggleAction(KSPActionType): 1 = Deactivate, otherwise Activate
    void EnableAction(KSPActionParam& param);               // :75
    void DisableAction(KSPActionParam& param);              // :89
    void Start();                                           // :107
    void OnAwake() override;                                // :116
    void OnLoad(const ConfigNode& node) override;           // :127
    void OnStart(StartState state) override;                // :134
    void UpdateCrossfeed(bool fireEvent);                   // :155
    bool CrossfeedToggleableEditor() override;              // :178
    bool CrossfeedToggleableFlight() override;              // :183
    bool CrossfeedRequiresTech() override;                  // :188
    std::string CrossfeedTech() override;                   // :193
    bool CrossfeedHasTech() override;                       // :198
    std::string GetModuleDisplayName() override;            // :208
    void CopySerializedFrom(const PartModule& from) override;

private:
    void Activate();                                        // :80
    void Deactivate();                                      // :94
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
    std::string GetInfo() override;                         // :46
    std::string GetModuleTitle() override { return "Fuel Line"; }  // :51
    std::string GetPrimaryField() override { return ""; }          // :61
    std::string GetModuleDisplayName() override;            // :66
    IModuleInfo* AsModuleInfo() override { return this; }
    static void CacheLocalStrings();                        // :71
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
    void PutResourceExternalEvent(Part* activeVesselRoot);  // :80
    void StopTransfer();                                    // :98
    // one iteration of the transferResources coroutine (:105); false when the loop ends
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
