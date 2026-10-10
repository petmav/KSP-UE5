// PartModule's staging queries (PartModule.cs:272-345) and the IsStageable / StagingEnabled / StagingToggleEnabled* overrides of the
// stageable modules that their own packages (WP-25 docking, WP-30 science, ...) have not ported yet.
// Part.UpdateStageability (Part.cs:7777) asks every module of a part with a stagingIcon, so the staging sequence of a craft depends
// on these answers. Each rule cites its C# source; a ported subclass overrides the virtuals and the rule here is not reached.
#include "ksp/modules/part_module.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/parts/part.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

namespace ksp {

namespace {
// ReflectedAttributes.isStageable (PartModule.cs:27-50): a class at or below PartModule declares a public OnActive. The classes that
// declare one are generated (tools/wp16/gen_stageable.py -> stageable_classes.inc).
bool ReflectedIsStageable(const ModuleClassInfo& cls) {
    static const char* const kDeclaring[] = {
#define KSP_STAGEABLE(c, f, l) c,
#include "ksp/modules/stageable_classes.inc"
#undef KSP_STAGEABLE
    };
    for (const ModuleClassInfo* c = &cls; c != nullptr; c = c->base) {
        if (std::string_view(c->name) == "PartModule") break;
        for (const char* n : kDeclaring)
            if (std::string_view(n) == c->name) return true;
    }
    return false;
}

// PartModule.cs:746 GetReflectedAttributes(GetType()): one ReflectedAttributes per module type, built at the first query (the
// constructor evaluates the OnActive rule above) and shared by every instance; ModuleDecouplerBase.LateUpdate overwrites its
// `isStageable`. Keyed by the module class name, like C#'s per-Type cache.
std::unordered_map<std::string, bool>& ReflectedAttributesIsStageable() {
    static std::unordered_map<std::string, bool> cache;
    return cache;
}
bool& ModuleAttributesIsStageable(const ModuleClassInfo& cls) {
    auto& cache = ReflectedAttributesIsStageable();
    auto it = cache.find(cls.name);
    if (it == cache.end()) it = cache.emplace(cls.name, ReflectedIsStageable(cls)).first;
    return it->second;
}
}  // namespace

// The `part.stagingIcon = ...` default inside the OnStart / Setup of modules whose package has not ported the class yet:
// `if (part.stagingIcon == string.Empty && overrideStagingIconIfBlank) part.stagingIcon = <icon>` (ModuleParachute.cs:310, LaunchClamp.cs:159, ModuleServiceModule.cs:174, ModuleProceduralFairing.cs:402; ModuleScienceExperiment.cs:263
// additionally needs useStaging). The decouplers' own (ModuleDecouple / ModuleAnchoredDecoupler) are ported in module_decouplers.cpp.
void PartModule::OnStart(StartState) {
    const ModuleClassInfo& cls = Class();
    const char* icon = nullptr;
    if (cls.Is("ModuleParachute")) icon = "PARACHUTES";
    else if (cls.Is("LaunchClamp")) icon = "STRUT";
    else if (cls.Is("ModuleServiceModule") || cls.Is("ModuleProceduralFairing")) icon = "FUEL_TANK";
    else if (cls.Is("ModuleScienceExperiment") && Get<bool>("useStaging")) icon = "SCIENCE_GENERIC";
    if (icon == nullptr) return;
    if (Part* p = dynamic_cast<Part*>(part))
        if (p->fields.stagingIcon && p->fields.stagingIcon->empty() && Get<bool>("overrideStagingIconIfBlank")) p->fields.stagingIcon = std::string(icon);
}

// PartModule.cs:272 `return ModuleAttributes.isStageable` (the per-class ReflectedAttributes flag, evaluated at the class's first query)
bool PartModule::IsStageable() {
    const ModuleClassInfo& cls = Class();
    // ModuleJettison.cs:641: `if (!stagingEnabled && !stagingToggleEnabledEditor) return stagingToggleEnabledFlight; return true;`
    // (ModuleRCS.cs:746 is ModuleRCS::IsStageable, WP-23)
    if (cls.Is("ModuleJettison")) {
        if (!Get<bool>("stagingEnabled") && !Get<bool>("stagingToggleEnabledEditor")) return Get<bool>("stagingToggleEnabledFlight");
        return true;
    }
    // ModuleEngines.cs:2137 / ModuleGimbal.cs:326 are ModuleEngines::IsStageable / ModuleGimbal::IsStageable (WP-23)
    if (cls.Is("ModuleTestSubject") || cls.Is("ModuleEvaChute")) return false;  // ModuleTestSubject.cs:200, ModuleEvaChute.cs:136
    if (cls.Is("ModuleDockingNode")) return Get<bool>("staged");               // ModuleDockingNode.cs:1949
    if (cls.Is("ModuleScienceExperiment")) return Get<bool>("useStaging");      // ModuleScienceExperiment.cs:1183
    return ModuleAttributesIsStageable(cls);
}

// ModuleDecouplerBase.cs:116 `base.ModuleAttributes.isStageable = value`
void PartModule::SetModuleAttributesIsStageable(bool value) { ModuleAttributesIsStageable(Class()) = value; }

// PartModule.cs:277
bool PartModule::StagingEnabled() {
    const ModuleClassInfo& cls = Class();
    if (cls.Is("ModuleDockingNode")) return Get<bool>("stagingEnabled") ? Get<bool>("staged") : false;  // ModuleDockingNode.cs:1954
    if (cls.Is("ModuleScienceExperiment")) return Get<bool>("useStaging") ? Get<bool>("stagingEnabled") : false;  // ModuleScienceExperiment.cs:1188
    return Get<bool>("stagingEnabled");
}

// PartModule.cs:333
bool PartModule::StagingToggleEnabledEditor() {
    if (Class().Is("ModuleDockingNode")) return Get<bool>("staged");  // ModuleDockingNode.cs:1963
    return Get<bool>("stagingToggleEnabledEditor");
}

// PartModule.cs:338
bool PartModule::StagingToggleEnabledFlight() {
    if (Class().Is("ModuleDockingNode")) return Get<bool>("staged");  // ModuleDockingNode.cs:1968
    return Get<bool>("stagingToggleEnabledFlight");
}

// PartModule.cs:303 (the symmetry counterparts follow only through their ModuleDecouplerBase / ModuleProceduralFairing)
void PartModule::ToggleStaging() {
    const bool stagingState = !Get<bool>("stagingEnabled");
    SetStagingState(stagingState);
    // C# throws NullReferenceException for a module without a part (`part.symmetryCounterparts`); kspcore's WP-06 tests Send events to
    // part-less modules, so the part-dependent half is skipped there
    Part* p = dynamic_cast<Part*>(part);
    if (p == nullptr) return;
    for (size_t i = 0; i < p->symmetryCounterparts.size(); i++) {
        Part* sym = p->symmetryCounterparts[i];
        if (sym == nullptr) continue;
        PartModule* obj = sym->FindModuleImplementing([](PartModule& pm) { return pm.Class().Is("ModuleDecouplerBase"); });
        if (obj == nullptr)
            obj = sym->FindModuleImplementing([](PartModule& pm) { return pm.Class().Is("ModuleProceduralFairing"); });
        if (obj != nullptr) obj->SetStagingState(stagingState);
    }
}

// PartModule.cs:326
void PartModule::SetStagingState(bool newState) {
    Set<bool>("stagingEnabled", newState);
    UpdateStagingToggle();
    Part* p = dynamic_cast<Part*>(part);
    if (p == nullptr) return;  // see ToggleStaging
    p->UpdateStageability(true, true);
}

}  // namespace ksp
