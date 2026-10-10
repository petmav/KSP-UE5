// WP-28: ModuleGenerator, ModuleAlternator, ModuleFuelJettison, ModuleResourceDrain. See ksp/modules/resource_processing.hpp.
#include "ksp/modules/resource_processing.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
Part* PartOf(PartModule& m) { return dynamic_cast<Part*>(m.part); }
template <class T>
void AddUnique(std::vector<T>& v, const T& x) {
    if (std::find(v.begin(), v.end(), x) == v.end()) v.push_back(x);
}
}  // namespace

// ================================================== ModuleGenerator ============================================================

// binds the [KSPEvent] / [KSPAction] methods (the C# attribute scan does this)
void ModuleGenerator::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("Activate", BaseEvent::Handler([this] { Activate(); }));
    Events().Bind("Shutdown", BaseEvent::Handler([this] { Shutdown(); }));
    Actions().Bind("ToggleAction", [this](KSPActionParam& p) { ToggleAction(p); });
    Actions().Bind("ActivateAction", [this](KSPActionParam& p) { ActivateAction(p); });
    Actions().Bind("ShutdownAction", [this](KSPActionParam& p) { ShutdownAction(p); });
}

// ModuleGenerator.cs:65 (KSPActionType: Activate 0, Deactivate 1, Toggle 2)
void ModuleGenerator::ToggleAction(KSPActionParam& param) {
    if (param.type != 0 && (param.type != static_cast<int>(KSPActionType::Toggle) || Get<bool>("generatorIsActive"))) Shutdown();
    else Activate();
}
// ModuleGenerator.cs:78
void ModuleGenerator::ActivateAction(KSPActionParam&) { Activate(); }
// ModuleGenerator.cs:84
void ModuleGenerator::ShutdownAction(KSPActionParam&) { Shutdown(); }

// ModuleGenerator.cs:90
void ModuleGenerator::Activate() {
    if (!Get<bool>("isAlwaysActive")) {
        Set<bool>("generatorIsActive", true);
        Events()["Shutdown"]->active = true;
        Events()["Activate"]->active = false;
    }
}
// ModuleGenerator.cs:95
void ModuleGenerator::Shutdown() {
    if (!Get<bool>("isAlwaysActive")) {
        Set<bool>("generatorIsActive", false);
        Events()["Shutdown"]->active = false;
        Events()["Activate"]->active = true;
    }
}

// ModuleGenerator.cs:111
void ModuleGenerator::OnStart(StartState state) {
    const bool moduleIsEnabled = Get<bool>("moduleIsEnabled");
    const bool isAlwaysActive = Get<bool>("isAlwaysActive");
    Events()["Activate"]->active = moduleIsEnabled && !isAlwaysActive && !Get<bool>("generatorIsActive");
    Events()["Shutdown"]->active = moduleIsEnabled && !isAlwaysActive && Get<bool>("generatorIsActive");
    Actions()["ToggleAction"]->active = moduleIsEnabled && !isAlwaysActive;
    Actions()["ActivateAction"]->active = moduleIsEnabled && !isAlwaysActive;
    Actions()["ShutdownAction"]->active = moduleIsEnabled && !isAlwaysActive;
    Fields()["efficiency"]->guiActive = moduleIsEnabled;
    Fields()["displayStatus"]->guiActive = moduleIsEnabled;
    Fields()["efficiency"]->guiName = Get<std::string>("efficiencyGUIName");
    Fields()["displayStatus"]->guiName = Localizer::Format(Get<std::string>("statusGUIName"));
    const std::string shutdownGUIName = Get<std::string>("shutdownGUIName");
    Actions()["ShutdownAction"]->SetGuiName(shutdownGUIName);
    Events()["Shutdown"]->SetGuiName(shutdownGUIName);
    const std::string activateGUIName = Get<std::string>("activateGUIName");
    Actions()["ActivateAction"]->SetGuiName(activateGUIName);
    Events()["Activate"]->SetGuiName(activateGUIName);
    Actions()["ToggleAction"]->SetGuiName(Get<std::string>("toggleGUIName"));
    if (state != StartState::Editor && isAlwaysActive) Set<bool>("generatorIsActive", moduleIsEnabled);
}

// ModuleGenerator.cs:147. cacheAutoLOC_219034 / cacheAutoLOC_220477 are Localizer.Format of the two ids, taken once.
void ModuleGenerator::FixedUpdate() {
    if (Get<bool>("generatorIsActive") && Get<bool>("moduleIsEnabled")) {
        inputRecieved = 1.0;
        lackingResources = false;
        Set<float>("efficiency", 1.0f);
        if (Get<std::string>("status") != "Nominal") {
            Set<std::string>("displayStatus", Localizer::Format("#autoLOC_219034"));
            Set<std::string>("status", "Nominal");
        }
        if (Get<bool>("isThrottleControlled")) {
            Part* p = PartOf(*this);
            const ProcessingVessel* v = ResourceProcessingEnvironment::VesselOf(p);
            if (v != nullptr && v->isControllable) Set<float>("throttle", v->mainThrottle);  // part.isControllable = vessel.IsControllable
        } else {
            Set<float>("throttle", 1.0f);
        }
        const float throttle = Get<float>("throttle");
        const int count = static_cast<int>(resHandler.inputResources.size());
        if (count > 0) {
            std::string status = Get<std::string>("status");
            inputRecieved = resHandler.UpdateModuleResourceInputs(status, static_cast<double>(throttle), static_cast<double>(Get<float>("resourceThreshold")), false, true, true);
            Set<std::string>("status", status);
            if (status == "Nominal") Set<std::string>("displayStatus", Localizer::Format("#autoLOC_219034"));
            else Set<std::string>("displayStatus", Localizer::Format("#autoLOC_220477"));
            lackingResources = false;
            int index = count;
            while (index-- > 0) {
                currentResource = resHandler.inputResources[static_cast<size_t>(index)].get();
                if (!currentResource->available) {
                    lackingResources = true;
                    break;
                }
            }
            if (Get<bool>("requiresAllInputs") && lackingResources) Set<float>("efficiency", 0.0f);
            else if (Get<bool>("isThrottleControlled")) Set<float>("efficiency", static_cast<float>(inputRecieved * static_cast<double>(throttle)));
            else Set<float>("efficiency", static_cast<float>(inputRecieved));
        } else if (Get<bool>("isThrottleControlled")) {
            Set<float>("efficiency", throttle);
        }
        Set<float>("efficiency", ApplyEfficiencyAdjustments(Get<float>("efficiency")));
        resHandler.UpdateModuleResourceOutputs(static_cast<double>(Get<float>("efficiency")));
    } else {
        Set<float>("efficiency", 0.0f);
        if (Get<std::string>("status") != "Off") {
            Set<std::string>("displayStatus", Localizer::Format("#autoLOC_220477"));
            Set<std::string>("status", "Off");
        }
    }
}

// ModuleGenerator.cs:234
std::string ModuleGenerator::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003042"); }

// ModuleGenerator.cs:255
float ModuleGenerator::ApplyEfficiencyAdjustments(float efficiency) {
    for (const auto& a : efficiencyAdjusters) efficiency = a(efficiency);
    return efficiency;
}

// ================================================== ModuleAlternator ===========================================================

// ModuleAlternator.cs:44 (fld = Fields["outputRate"]; the events of this class: none)
void ModuleAlternator::OnAwake() { resHandler.moduleResourceBasedPrimaryIsInput = false; }

// ModuleAlternator.cs:50 (does not call base.OnLoad)
void ModuleAlternator::OnLoad(const ConfigNode&) {
    BaseField* fld = Fields()["outputRate"];
    fld->guiName = Get<std::string>("outputName");
    fld->guiUnits = Localizer::Format(Get<std::string>("outputUnits"));
    fld->guiFormat = Get<std::string>("outputFormat");
}

// ModuleAlternator.cs:57
void ModuleAlternator::OnStart(StartState) {
    BaseField* fld = Fields()["outputRate"];
    fld->guiName = Get<std::string>("outputName");
    fld->guiUnits = Localizer::Format(Get<std::string>("outputUnits"));
    fld->guiFormat = Get<std::string>("outputFormat");
    fld->guiActive = Get<bool>("moduleIsEnabled");
    if (Get<bool>("roboticRotorMode")) {
        if (!serenityInstalled) {
            Set<bool>("roboticRotorMode", false);
            return;
        }
        Part* p = PartOf(*this);
        rotor = p != nullptr ? p->FindModuleOfClass("ModuleRoboticServoRotor") : nullptr;
        if (rotor == nullptr) {
            Set<bool>("roboticRotorMode", false);
        } else {
            for (size_t i = 0; i < rotor->resHandler.inputResources.size(); i++) {
                const PartResourceDefinition* d = rotor->resHandler.inputResources[i]->resourceDef();
                if (d != nullptr && d->name() == "ElectricCharge") Set<bool>("roboticRotorMode", false);
            }
        }
    }
    // else: engine = part.Modules.FindEngineNearby(engineName, engineIndex, preferMultiMode): the engine package binds
    // engineNormalizedOutput
}

// ModuleAlternator.cs:110
void ModuleAlternator::FixedUpdate() {
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight() || !Get<bool>("moduleIsEnabled")) return;
    float num = 0.0f;
    if (Get<bool>("roboticRotorMode")) {
        if (rotor != nullptr && rotorNormalizedOutput) num = ApplyOutputAdjustments(rotorNormalizedOutput());
    } else if (engineNormalizedOutput) {
        num = ApplyOutputAdjustments(engineNormalizedOutput());
    }
    if (num > 0.0f) Set<float>("outputRate", static_cast<float>(resHandler.UpdateModuleResourceOutputs(static_cast<double>(num), Get<double>("singleTickThreshold"))));
}

// ModuleAlternator.cs:150
float ModuleAlternator::ApplyOutputAdjustments(float output) {
    for (const auto& a : outputAdjusters) output = a(output);
    return output;
}

// ModuleAlternator.cs:159
std::string ModuleAlternator::GetModuleDisplayName() { return Localizer::Format("#autoLoc_6003026"); }

// ================================================== ModuleFuelJettison =========================================================

void ModuleFuelJettison::OnAwake() {
    PartModule::OnAwake();
    Events().Bind("FuelJettison", BaseEvent::Handler([this] { FuelJettison(); }));
    Actions().Bind("FuelJettisonAction", [this](KSPActionParam& p) { FuelJettisonAction(p); });
}

// ModuleFuelJettison.cs:9. A runtime module is a Unity clone of its prefab component, and Unity serialises a null string field as
// "": a ResourceName the cfg never sets is string.Empty here (the prefab component's field stays null in kspcore's schema).
void ModuleFuelJettison::FuelJettison() {
    Part* p = PartOf(*this);
    if (p == nullptr) throw Nre();
    const std::string ResourceName = IsNull("ResourceName") ? std::string() : Get<std::string>("ResourceName");
    if (ResourceName.empty()) {
        const int count = p->ResourceList().Count();
        for (int i = 0; i < count; i++) Dump(p->ResourceList()[static_cast<size_t>(i)]);
    } else {
        PartResource* partResource = p->GetResource(ResourceName);
        if (partResource != nullptr) Dump(*partResource);
    }
}

// ModuleFuelJettison.cs:28 (the screen message `Localizer.Format("#autoLOC_6001051", amount.ToString("0.00"), info.displayName)` is recorded)
void ModuleFuelJettison::Dump(PartResource& res) {
    Part* p = PartOf(*this);
    ResourceProcessingEnvironment::screenMessages.push_back(
        Localizer::Format("#autoLOC_6001051", {dotnet::FormatDoubleF(res.amount, 2), res.info != nullptr ? res.info->displayName() : std::string()}));
    p->TransferResource(res, std::numeric_limits<double>::lowest(), p);  // double.MinValue
}

// ModuleFuelJettison.cs:35
void ModuleFuelJettison::FuelJettisonAction(KSPActionParam&) { FuelJettison(); }

// ModuleFuelJettison.cs:40
void ModuleFuelJettison::OnStart(StartState) {
    const std::string ResourceName = IsNull("ResourceName") ? std::string() : Get<std::string>("ResourceName");
    if (!ResourceName.empty()) {
        Actions()["FuelJettisonAction"]->SetGuiName(Localizer::Format("#autoLOC_6001888", {ResourceName}));
        Events()["FuelJettison"]->SetGuiName(Localizer::Format("#autoLOC_6001888", {ResourceName}));
    }
}

// ================================================== ModuleResourceDrain ========================================================

// ModuleResourceDrain.cs:110
void ModuleResourceDrain::OnAwake() {
    Actions()["ToggleResourceDrainAction"]->SetGuiName(Localizer::Format("#autoLOC_6006002"));
    Actions()["ToggleResourceDrainFlowAction"]->SetGuiName(Localizer::Format("#autoLOC_6006016"));
    Fields()["flowMode"]->guiName = Localizer::Format("#autoLOC_6006015");
    resourcesAvailable.clear();
    resourceDrainStatus.clear();
    drainFXGroups.clear();
    Actions().Bind("StartResourceDrainAction", [this](KSPActionParam& p) { StartResourceDrainAction(p); });
    Actions().Bind("StopResourceDrainAction", [this](KSPActionParam& p) { StopResourceDrainAction(p); });
    Actions().Bind("ToggleResourceDrainAction", [this](KSPActionParam& p) { ToggleResourceDrainAction(p); });
    Actions().Bind("ToggleResourceDrainFlowAction", [this](KSPActionParam& p) { ToggleResourceDrainFlowAction(p); });
}

// ModuleResourceDrain.cs:121 (the GameEvents / OnValueModified subscriptions are the host's: onPartResourceListChange -> OnPartResourceListChanged)
void ModuleResourceDrain::OnStart(StartState) {
    Set<float>("drainRate", Mathf::Clamp(Get<float>("drainRate"), minDrainRate, maxDrainRate));
    Part* p = PartOf(*this);
    if (p != nullptr && p->parent != nullptr) {
        PartResourceList& pr = p->parent->ResourceList();
        for (int i = 0; i < pr.Count(); i++) {
            const PartResourceDefinition* info = pr[static_cast<size_t>(i)].info;
            if (info->partResourceDrainDefinition().isDrainable()) {
                const std::string& fxName = info->partResourceDrainDefinition().drainFXDefinition();
                // Part.findFxGroup(name): the part's FX group of that name
                if (std::find(p->fxGroupNames.begin(), p->fxGroupNames.end(), fxName) != p->fxGroupNames.end()) AddUnique(drainFXGroups, fxName);
            }
        }
    }
    Initialize();
}

// ModuleResourceDrain.cs:147
void ModuleResourceDrain::OnStartFinished(StartState) {
    if (IsDraining()) CheckDrainingForEffects();
}

// ModuleResourceDrain.cs:157
void ModuleResourceDrain::FixedUpdate() {
    if (isDraining() && snapshot != nullptr) DrainPart();
}

// ModuleResourceDrain.cs:165
void ModuleResourceDrain::OnDestroy() {}

// ModuleResourceDrain.cs:173
void ModuleResourceDrain::StartResourceDrainAction(KSPActionParam&) { TurnOnDrain(); }
// ModuleResourceDrain.cs:179
void ModuleResourceDrain::StopResourceDrainAction(KSPActionParam&) { TurnOffDrain(); }
// ModuleResourceDrain.cs:185
void ModuleResourceDrain::ToggleResourceDrainAction(KSPActionParam&) { ToggleDrain(); }
// ModuleResourceDrain.cs:191
void ModuleResourceDrain::ToggleResourceDrainFlowAction(KSPActionParam&) { TogglePartFlowMode(); }
// ModuleResourceDrain.cs:202
void ModuleResourceDrain::TogglePartFlowMode() { Set<bool>("flowMode", !Get<bool>("flowMode")); }

// ModuleResourceDrain.cs:207
void ModuleResourceDrain::OnPartResourceListChanged(Part* hostPart) {
    Part* p = PartOf(*this);
    if (p == nullptr || p->parent == nullptr) throw Nre();
    if (hostPart->fields.persistentId == p->parent->fields.persistentId) LoadResources();
}

// ModuleResourceDrain.cs:223 (UI_Resources lookups are UI; the action names and the axis field limits are state)
void ModuleResourceDrain::Initialize() {
    LoadResources();
    Actions()["StartResourceDrainAction"]->SetGuiName(Localizer::Format("#autoLOC_6006003"));
    Actions()["StopResourceDrainAction"]->SetGuiName(Localizer::Format("#autoLOC_6006004"));
    if (auto* axis = dynamic_cast<BaseAxisField*>(Fields()["drainRate"])) {
        axis->minValue = minDrainRate;
        axis->maxValue = maxDrainRate;
    }
    Set<std::string>("noResourceAvailable", Localizer::Format("#autoLOC_6006031"));
    Fields()["noResourceAvailable"]->guiActive = resourcesAvailable.size() <= 0;
    Fields()["noResourceAvailable"]->guiActiveEditor = resourcesAvailable.size() <= 0;
}

// ModuleResourceDrain.cs:254
void ModuleResourceDrain::LoadResources() {
    Part* p = PartOf(*this);
    if (p == nullptr) throw Nre();
    if (p->parent == nullptr) {
        isInactive = true;
        enabledBehaviour = false;
    } else {
        resourcesAvailable.clear();
        LoadResourcesAvailable();
        if (resourcesAvailable.empty()) {
            isInactive = true;
            enabledBehaviour = false;
        }
    }
    if (loadedResources && !loadedResources->empty()) UpdateResourcesToRelease(*loadedResources);
}

// ModuleResourceDrain.cs:277
void ModuleResourceDrain::LoadResourcesAvailable() {
    Part* p = PartOf(*this);
    if (p->parent != nullptr) {
        PartResourceList& pr = p->parent->ResourceList();
        for (int i = 0; i < pr.Count(); i++)
            if (pr[static_cast<size_t>(i)].info->partResourceDrainDefinition().isDrainable()) AddUnique(resourcesAvailable, &pr[static_cast<size_t>(i)]);
    }
    for (size_t j = 0; j < resourcesAvailable.size(); j++) {
        ResourceDrainStatus s;
        const PartResourceDefinition* info = resourcesAvailable[j]->info;
        s.resource = info;
        s.isDraining = loadedResources && std::find(loadedResources->begin(), loadedResources->end(), resourcesAvailable[j]->resourceName) != loadedResources->end();
        s.showDrainFX = info->partResourceDrainDefinition().showDrainFX();
        s.drainFXName = info->partResourceDrainDefinition().drainFXDefinition();
        s.fxPriority = info->partResourceDrainDefinition().drainFXPriority();
        s.drainISP = info->partResourceDrainDefinition().drainForceISP();
        // List<ResourceDrainStatus>.AddUnique: reference equality of a fresh object: always added
        resourceDrainStatus.push_back(s);
    }
}

// ModuleResourceDrain.cs:302
void ModuleResourceDrain::UpdateVFXToUse() {
    if (highestPriorityFXGroup) fxGroupActive[*highestPriorityFXGroup] = false;
    if (!resourceDrainStatus.empty()) {
        UpdatePriorityIndices();
        highestPriorityFXGroup.reset();
        const std::string& n = resourceDrainStatus[static_cast<size_t>(highestIndex)].drainFXName;
        for (const std::string& g : drainFXGroups)
            if (g == n) {
                highestPriorityFXGroup = g;
                break;
            }
    } else {
        highestPriorityFXGroup.reset();
    }
    if (highestPriorityFXGroup && IsDraining() && !ResourceFlowEnvironment::LoadedSceneIsEditor()) fxGroupActive[*highestPriorityFXGroup] = true;
}

// ModuleResourceDrain.cs:337
void ModuleResourceDrain::UpdatePriorityIndices() {
    highestValue = 0;
    for (size_t i = 0; i < resourceDrainStatus.size(); i++) {
        if (resourceDrainStatus[i].showDrainFX && resourceDrainStatus[i].isDraining && DrainResourceNotFinished(static_cast<int>(i)) &&
            resourceDrainStatus[i].fxPriority > highestValue) {
            highestIndex = static_cast<int>(i);
            highestValue = resourceDrainStatus[i].fxPriority;
        }
    }
}

// ModuleResourceDrain.cs:350 (`maxAmount / (double)(100f / drainRate) * (double)TimeWarp.deltaTime`)
bool ModuleResourceDrain::HasEnoughDrainingResources() {
    bool result = false;
    Part* p = PartOf(*this);
    Part* parent = p->parent;
    double amount = 0.0, maxAmount = 0.0, num = 0.0;
    if (parent != nullptr) {
        for (size_t i = 0; i < resourceDrainStatus.size(); i++) {
            if (resourceDrainStatus[i].isDraining) {
                parent->GetConnectedResourceTotals(resourceDrainStatus[i].resource->id(),
                                                   Get<bool>("flowMode") ? AsFlowMode(resourceDrainStatus[i].resource->resourceFlowMode()) : ResourceFlowMode::NO_FLOW,
                                                   false, amount, maxAmount);
                const float per = f32store(100.0 / fx(Get<float>("drainRate")));
                num = maxAmount / static_cast<double>(per) * static_cast<double>(TimeWarp::deltaTime());
                if (!(amount < num)) {
                    result = true;
                    break;
                }
            }
        }
    }
    return result;
}

// ModuleResourceDrain.cs:376
bool ModuleResourceDrain::DrainResourceNotFinished(int rdsIndex) {
    Part* parent = PartOf(*this)->parent;
    double amount = 0.0, maxAmount = 0.0;
    if (parent != nullptr) {
        const auto& r = resourceDrainStatus[static_cast<size_t>(rdsIndex)];
        parent->GetConnectedResourceTotals(r.resource->id(), Get<bool>("flowMode") ? AsFlowMode(r.resource->resourceFlowMode()) : ResourceFlowMode::NO_FLOW, false, amount, maxAmount);
        if (amount > 0.0) return true;
    }
    return false;
}

// ModuleResourceDrain.cs:392
void ModuleResourceDrain::UpdateResourcesToRelease(const std::vector<std::string>& resourceNames, bool exclude) {
    if (exclude) {
        for (size_t i = 0; i < resourceNames.size(); i++)
            for (size_t j = 0; j < resourcesAvailable.size(); j++)
                if (resourcesAvailable[j]->resourceName == resourceNames[i]) UpdateResourceDrainStatus(resourcesAvailable[j]->info->id(), false);
    } else {
        for (size_t k = 0; k < resourceNames.size(); k++)
            if (PartContainsResource(resourceNames[k])) UpdateResourceDrainStatus(GetPartResource(resourceNames[k])->info->id(), true);
        for (size_t l = 0; l < resourceDrainStatus.size(); l++)
            if (std::find(resourceNames.begin(), resourceNames.end(), resourceDrainStatus[l].resource->name()) == resourceNames.end()) resourceDrainStatus[l].isDraining = false;
    }
    if (ResourcesToDrain() <= 0) set_IsDraining(false);
}

// ModuleResourceDrain.cs:455
void ModuleResourceDrain::DrainPart() {
    drainAmount = 0.0;
    amtReceived = 0.0;
    drainedResources = 0;
    bool flag = false;
    double num = 0.0;
    Part* p = PartOf(*this);
    for (size_t i = 0; i < resourcesAvailable.size(); i++) {
        PartResource& r = *resourcesAvailable[i];
        if (ResourceFlowEnvironment::InfinitePropellant || GetResourceDrainStatus(r)) {
            const float per = f32store(100.0 / fx(Get<float>("drainRate")));
            drainAmount = r.maxAmount / static_cast<double>(per) * static_cast<double>(TimeWarp::deltaTime());
            if (ResourceFlowEnvironment::InfinitePropellant) amtReceived = drainAmount;
            else if (Get<bool>("flowMode")) amtReceived = p->parent->RequestResource(r.info->id(), drainAmount, AsFlowMode(r.info->resourceFlowMode()));
            else amtReceived = p->parent->RequestResource(r.info->id(), drainAmount, ResourceFlowMode::NO_FLOW);
            if (amtReceived <= epsilon) {
                drainedResources++;
                flag = true;
                continue;
            }
            num = amtReceived * static_cast<double>(r.info->density());
            perResourceExhaustVel = f32store(fx(r.info->partResourceDrainDefinition().drainForceISP()) * fx(9.80665f));
            perResourceThrust = f32store(fx(perResourceExhaustVel) * fx(static_cast<float>(num)) / fx(Time::fixedDeltaTime));
            addedForces.push_back(perResourceThrust);  // Part.AddForce(part.transform.forward * perResourceThrust)
        }
    }
    if (drainedResources == ResourcesToDrain()) set_IsDraining(false);
    if (flag) UpdateVFXToUse();
}

// ModuleResourceDrain.cs:501
bool ModuleResourceDrain::PartContainsResource(const std::string& partResource) {
    Part* p = PartOf(*this);
    if (p->parent == nullptr) return false;
    return p->parent->ResourceList().Contains(partResource);
}

// ModuleResourceDrain.cs:510
void ModuleResourceDrain::TurnOnDrain() {
    if (ResourcesToDrain() > 0 && HasEnoughDrainingResources()) set_IsDraining(true);
}

// ModuleResourceDrain.cs:518
void ModuleResourceDrain::CheckDrainingForEffects() {
    if (!ResourceFlowEnvironment::LoadedSceneIsEditor() && highestPriorityFXGroup) {
        if (IsDraining() && ResourcesToDrain() > 0 && ResourcesToDrainHaveFX() && HasEnoughDrainingResources()) {
            fxGroupActive[*highestPriorityFXGroup] = true;
            fxGroupPower[*highestPriorityFXGroup] = f32store(fx(Get<float>("drainRate")) * fx(0.05f));  // UpdateResourceDrainRate: SetPower
        } else {
            fxGroupActive[*highestPriorityFXGroup] = false;
        }
    }
}

// ModuleResourceDrain.cs:534
void ModuleResourceDrain::TurnOffDrain() { set_IsDraining(false); }
// ModuleResourceDrain.cs:539
void ModuleResourceDrain::TurnOffDrain(const std::vector<std::string>& resourcesToStopDraining) { UpdateResourcesToRelease(resourcesToStopDraining, true); }

// ModuleResourceDrain.cs:544
void ModuleResourceDrain::ToggleDrain() {
    set_IsDraining(!IsDraining());
    if (IsDraining()) TurnOnDrain();
    else TurnOffDrain();
}

// ModuleResourceDrain.cs:557
bool ModuleResourceDrain::IsResourceDraining(PartResource& pR) { return GetResourceDrainStatus(pR); }

// ModuleResourceDrain.cs:561
PartResource* ModuleResourceDrain::GetPartResource(const std::string& resourceName) {
    Part* p = PartOf(*this);
    if (p->parent == nullptr) return nullptr;
    PartResourceList& pr = p->parent->ResourceList();
    for (int i = 0; i < pr.Count(); i++)
        if (pr[static_cast<size_t>(i)].info->partResourceDrainDefinition().isDrainable() && pr[static_cast<size_t>(i)].info->name() == resourceName) return &pr[static_cast<size_t>(i)];
    return nullptr;
}

// ModuleResourceDrain.cs:585
void ModuleResourceDrain::TogglePartResource(PartResource* resource, bool status) {
    if (resource != nullptr) UpdateResourceDrainStatus(resource->info->id(), status);
}

// ModuleResourceDrain.cs:595
void ModuleResourceDrain::UpdateResourceDrainStatus(int resourceID, bool status) {
    for (auto& s : resourceDrainStatus)
        if (s.resource->id() == resourceID) s.isDraining = status;
    UpdateVFXToUse();
}

// ModuleResourceDrain.cs:607 (writes the `resourcesDraining` field)
int ModuleResourceDrain::ResourcesToDrain() {
    int n = 0;
    for (const auto& s : resourceDrainStatus)
        if (ResourceFlowEnvironment::InfinitePropellant || s.isDraining) n++;
    Set<int32_t>("resourcesDraining", n);
    return n;
}

// ModuleResourceDrain.cs:618
bool ModuleResourceDrain::ResourcesToDrainHaveFX() {
    for (const auto& s : resourceDrainStatus)
        if ((ResourceFlowEnvironment::InfinitePropellant || s.isDraining) && s.showDrainFX) return true;
    return false;
}

// ModuleResourceDrain.cs:636
bool ModuleResourceDrain::GetResourceDrainStatus(PartResource& resource) {
    size_t num = 0;
    while (true) {
        if (num < resourceDrainStatus.size()) {
            if (resourceDrainStatus[num].resource->id() == resource.info->id()) break;
            num++;
            continue;
        }
        return false;
    }
    if (!ResourceFlowEnvironment::InfinitePropellant) return resourceDrainStatus[num].isDraining;
    return true;
}

// ModuleResourceDrain.cs:94 / :95 (the property setter)
void ModuleResourceDrain::set_IsDraining(bool v) {
    Set<bool>("isDraining", v);
    CheckDrainingForEffects();
    // UpdateResourceDrainRate(null): highestPriorityFXGroup.SetPower(drainRate * 0.05f)
    if (highestPriorityFXGroup) fxGroupPower[*highestPriorityFXGroup] = f32store(fx(Get<float>("drainRate")) * fx(0.05f));
}

// ModuleResourceDrain.cs:675
void ModuleResourceDrain::OnSave(ConfigNode& node) {
    PartModule::OnSave(node);
    if (!resourcesAvailable.empty()) {
        std::string text;
        for (size_t i = 0; i < resourcesAvailable.size(); i++)
            if (GetResourceDrainStatus(*resourcesAvailable[i])) text = !text.empty() ? text + "," + resourcesAvailable[i]->resourceName : text + resourcesAvailable[i]->resourceName;
        node.AddValue("resourcesToRelease", text);
    } else {
        node.AddValue("resourcesToRelease", loadedSavedResources);
    }
}

// ModuleResourceDrain.cs:696
void ModuleResourceDrain::OnLoad(const ConfigNode& node) {
    PartModule::OnLoad(node);
    node.TryGetValue("resourcesToRelease", loadedSavedResources);
    if (loadedSavedResources.empty()) return;
    loadedResources = std::vector<std::string>();
    for (const std::string& s : dotnet::Split(loadedSavedResources, ",", false))
        if (!s.empty()) loadedResources->push_back(s);
}

}  // namespace ksp
