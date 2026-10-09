// ModuleResource.cs and ModuleResourceHandler.cs (WP-15): the per-module input/output resource lists every PartModule carries
// (PartModule.resHandler, PartModule.cs:175; Awake -> resHandler.SetPartModule/OnAwake, Load -> resHandler.OnLoad).
//
// Units the C# works in: rates are per second, `currentRequest = rate * rateMultiplier * TimeWarp.fixedDeltaTime`; requests go
// through Part.RequestResource(id, amount, flowMode) so they obey the flow mode / PartSet rules of WP-15's solver.
//
// Not modelled: the PAW text of PrintRate / PrintModuleResources (PartModuleUtil.PrintResourceRequirements, Localizer texts
// for the part info panel). The error-string Localizer calls of UpdateModuleResourceInputs are kept (they are state).
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/parts/part_data.hpp"
#include "ksp/resources/resource_types.hpp"

namespace ksp {

class PartModule;
class ModuleResourceAutoShiftState;
class PartResourceDefinition;

// ModuleResource.cs:5
class ModuleResource {
public:
    std::string name;                                   // :8
    std::string title;                                  // :10
    int id = 0;                                         // :12
    double amount = 0.0;                                // :14
    double rate = 0.0;                                  // :16
    double currentRequest = 0.0;                        // :18
    double currentAmount = 0.0;                         // :20
    bool varyTime = true;                               // :22
    ResourceFlowMode flowMode = ResourceFlowMode::NULL_;  // :24
    bool useSI = false;                                 // :28
    double displayUnitMult = 1.0;                       // :30
    std::string unitName;                               // :32
    bool available = false;                             // :34
    bool isDeprived = false;                            // :36
    bool shutOffStartUpUsePercent = false;              // :38
    bool shutOffHandler = false;                        // :40
    bool startUpHandler = false;                        // :42
    float shutOffAmount = 0.0f;                         // :44
    float shutOffPercent = 0.0f;                        // :46
    float startUpAmount = 0.0f;                         // :48
    float startUpPercent = 0.0f;                        // :50
    ModuleResourceAutoShiftState* autoStateShifter = nullptr;  // :52 (the module that owns the auto start/stop fields)
    std::shared_ptr<ModuleResourceAutoShiftState> ownedShifter_;  // the object OnAwake creates (`new ModuleResourceAutoShiftState()`)

    const PartResourceDefinition* resourceDef();        // :54 (cached lookup by id)
    virtual ~ModuleResource() = default;
    virtual void OnAwake();                             // :66
    void Load(const ConfigNode& node);                  // :74
    void Save(ConfigNode& node) const;                  // :105
    bool IsDeprived(float requestThreshold = 0.9f) const;  // :140

private:
    const PartResourceDefinition* cachedResourceDef_ = nullptr;
};

// ModuleResourceHandler.cs:7
class ModuleResourceHandler {
public:
    std::vector<std::shared_ptr<ModuleResource>> inputResources;   // :9
    std::vector<std::shared_ptr<ModuleResource>> outputResources;  // :11
    bool moduleResourceBasedPrimaryIsInput = true;                 // :13
    bool currentResourceLowerThanLayoff = false;                   // :15
    PartModule* partModule = nullptr;                              // :17

    virtual ~ModuleResourceHandler() = default;
    virtual void OnAwake();                                        // :19
    virtual void OnLoad(const ConfigNode& node);                   // :30
    void SetPartModule(PartModule* p) { partModule = p; }          // :80

    // :85 / :104 (error: the C# `ref string error`)
    virtual double GetAverageInput();                              // :133
    // :150 / :155: returns the satisfied fraction (average = the mean over the inputs), `error` receives the missing resources
    virtual double UpdateModuleResourceInputs(std::string& error, double rateMultiplier, double threshold, bool returnOnFirstLack, bool average,
                                              bool stringOps);
    virtual double UpdateModuleResourceInputs(std::string& error, bool useFlowMode, double rateMultiplier, double threshold, bool returnOnFirstLack,
                                              bool average, bool stringOps);
    // (C# gives `stringOps` a default of true on every overload; with the bool-returning overloads below present a call that omits
    // it binds to those, so the double-returning ones take it explicitly here)
    virtual bool UpdateModuleResourceInputs(std::string& error, bool useFlowMode, double rateMultiplier, double threshold, bool returnOnFirstLack,
                                            bool stringOps = true);                       // :296
    virtual bool UpdateModuleResourceInputs(std::string& error, double rateMultiplier, double threshold, bool returnOnFirstLack, bool stringOps = true);  // :301
    virtual double UpdateModuleResourceOutputs(double rateMultiplier = 1.0, double minAbsValue = 0.0);  // :306
    bool HasEnoughResourcesToAutoStart();                          // :331
    bool IsResourceBelowShutOffLimit(PartResource& partRes);  // :369

protected:
    void LoadResList(std::vector<std::shared_ptr<ModuleResource>>& list, const ConfigNode& node);  // :86
};

}  // namespace ksp
