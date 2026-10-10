// ModuleResource.cs and ModuleResourceHandler.cs (WP-15): see ksp/modules/module_resource.hpp.
#include "ksp/modules/module_resource.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/modules/resource_modules.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }
// partModule.part as the live Part (the C# field is a Part)
Part* ModulePart(PartModule* m) {
    Part* p = m ? dynamic_cast<Part*>(m->part) : nullptr;
    if (p == nullptr) throw Nre();
    return p;
}
}  // namespace

// ---- ModuleResource ---------------------------------------------------------------------------------------------------------

// ModuleResource.cs:53 resourceDef
const PartResourceDefinition* ModuleResource::resourceDef() {
    if (cachedResourceDef_ == nullptr) {
        const PartResourceLibrary* lib = PartResourceLibrary::Instance();
        if (lib == nullptr) throw Nre();
        cachedResourceDef_ = lib->GetDefinition(id);
    }
    return cachedResourceDef_;
}

// Unity's Instantiate copy of a [Serializable] ModuleResource (ModuleResource.cs:4): all serialised members, not the OnAwake-created helper
std::shared_ptr<ModuleResource> ModuleResource::CloneSerialized() const {
    auto c = std::make_shared<ModuleResource>(*this);
    c->autoStateShifter = nullptr;
    c->ownedShifter_.reset();
    return c;
}

// ModuleResource.cs:65
void ModuleResource::OnAwake() {
    if (autoStateShifter == nullptr) {
        ownedShifter_ = std::make_shared<ModuleResourceAutoShiftState>();
        autoStateShifter = ownedShifter_.get();
    }
}

// ModuleResource.cs:73
void ModuleResource::Load(const ConfigNode& node) {
    const auto n = node.GetValue("name");
    if (!n) throw Nre();  // name.GetHashCode()
    name = *n;
    title = KSPUtil::PrintLocalizedModuleName(name);
    id = MonoStringHash(name);
    if (node.HasValue("resourceFlowMode")) {
        flowMode = AsFlowMode(static_cast<int32_t>(kEnum_ResourceFlowMode.Parse(*node.GetValue("resourceFlowMode"))));
    } else if (resourceDef() != nullptr) {
        flowMode = AsFlowMode(resourceDef()->resourceFlowMode());
    }
    if (node.HasValue("rate")) rate = cs::DoubleParse(*node.GetValue("rate"));
    if (node.HasValue("amount")) amount = cs::DoubleParse(*node.GetValue("amount"));
    if (node.HasValue("varyTime")) varyTime = cs::BoolParse(*node.GetValue("varyTime"));
    if (node.HasValue("useSI")) useSI = cs::BoolParse(*node.GetValue("useSI"));
    if (node.HasValue("displayUnitMult")) displayUnitMult = cs::DoubleParse(*node.GetValue("displayUnitMult"));
    if (node.HasValue("unitName")) unitName = *node.GetValue("unitName");
}

// ModuleResource.cs:112 (`_cachedResourceDef` is the field, not the property: null until resourceDef was read)
void ModuleResource::Save(ConfigNode& node) const {
    node.AddValue("name", name);
    node.AddValue("rate", rate);
    node.AddValue("amount", amount);
    if (cachedResourceDef_ == nullptr) throw Nre();
    if (static_cast<int32_t>(flowMode) != cachedResourceDef_->resourceFlowMode())
        node.AddValue("resourceFlowMode", kEnum_ResourceFlowMode.ToString(static_cast<int32_t>(flowMode)));
}

// ModuleResource.cs:143
bool ModuleResource::IsDeprived(float requestThreshold) const { return currentAmount < currentRequest * static_cast<double>(requestThreshold); }

// ---- ModuleResourceHandler --------------------------------------------------------------------------------------------------

// ModuleResourceHandler.cs:18
void ModuleResourceHandler::OnAwake() {}  // the lists are never null here

// ModuleResourceHandler.cs:30
void ModuleResourceHandler::OnLoad(const ConfigNode& node) {
    bool flag = true;
    bool flag2 = true;
    int count = node.nodes().Count();
    for (int i = 0; i < count; i++) {
        ConfigNode* configNode = node.nodes()[i];
        const std::string& nm = configNode->name;
        if (nm == "OUTPUT_RESOURCE") {
            const auto v = configNode->GetValue("name");
            if (v && !v->empty()) {
                if (flag2) {
                    outputResources.clear();
                    flag2 = false;
                }
                auto moduleResource = std::make_shared<ModuleResource>();
                moduleResource->Load(*configNode);
                outputResources.push_back(moduleResource);
            }
        } else if (nm == "INPUT_RESOURCE") {
            const auto v = configNode->GetValue("name");
            if (v && !v->empty()) {
                if (flag) {
                    inputResources.clear();
                    flag = false;
                }
                auto moduleResource = std::make_shared<ModuleResource>();
                moduleResource->Load(*configNode);
                inputResources.push_back(moduleResource);
            }
        } else if (nm == "RESOURCE") {
            LoadResList(moduleResourceBasedPrimaryIsInput ? inputResources : outputResources, node);
            return;
        }
    }
}

// ModuleResourceHandler.cs:78
void ModuleResourceHandler::LoadResList(std::vector<std::shared_ptr<ModuleResource>>& list, const ConfigNode& node) {
    bool flag = true;
    int count = node.nodes().Count();
    for (int i = 0; i < count; i++) {
        ConfigNode* configNode = node.nodes()[i];
        if (configNode->name == "RESOURCE") {
            if (flag) {
                list.clear();
                flag = false;
            }
            auto moduleResource = std::make_shared<ModuleResource>();
            moduleResource->Load(*configNode);
            list.push_back(moduleResource);
        }
    }
}

// ModuleResourceHandler.cs:121
double ModuleResourceHandler::GetAverageInput() {
    int count = static_cast<int>(inputResources.size());
    if (count == 0) return 0.0;
    double num = 0.0;
    int index = count;
    while (index-- > 0) num += inputResources[static_cast<size_t>(index)]->rate;
    return num / static_cast<double>(count);
}

// ModuleResourceHandler.cs:137
double ModuleResourceHandler::UpdateModuleResourceInputs(std::string& error, double rateMultiplier, double threshold, bool returnOnFirstLack,
                                                         bool average, bool stringOps) {
    return UpdateModuleResourceInputs(error, false, rateMultiplier, threshold, returnOnFirstLack, average, stringOps);
}

// ModuleResourceHandler.cs:142
double ModuleResourceHandler::UpdateModuleResourceInputs(std::string& error, bool useFlowMode, double rateMultiplier, double threshold,
                                                         bool returnOnFirstLack, bool average, bool stringOps) {
    int count = static_cast<int>(inputResources.size());
    if (count == 0) return 1.0;
    bool flag = true;
    double num = average ? 0.0 : 1.0;
    double num2 = static_cast<double>(TimeWarp::fixedDeltaTime());
    int num3 = 0;
    ModuleResource* moduleResource = nullptr;
    double num4 = 0.0;
    while (true) {
        if (num3 < count) {
            moduleResource = inputResources[static_cast<size_t>(num3)].get();
            num4 = moduleResource->rate * rateMultiplier;
            moduleResource->currentRequest = num4 * num2;
            if (ResourceFlowEnvironment::InfinitePropellant) {
                moduleResource->currentAmount = moduleResource->currentRequest;
                moduleResource->available = true;
                if (average) num += 1.0;
            } else {
                Part* p = ModulePart(partModule);
                double amount = 0.0, maxAmount = 0.0;
                IResourceVessel* h = ResourceFlowEnvironment::Host(p->vessel);
                if (h == nullptr) throw Nre();
                h->GetConnectedResourceTotals(moduleResource->id, false, amount, maxAmount, true);
                if (useFlowMode) moduleResource->currentAmount = p->RequestResource(moduleResource->id, moduleResource->currentRequest, moduleResource->flowMode);
                else moduleResource->currentAmount = p->RequestResource(moduleResource->id, moduleResource->currentRequest);
                if (moduleResource->shutOffHandler) {
                    if (moduleResource->shutOffStartUpUsePercent) {
                        double num5 = amount / maxAmount * 100.0;
                        if (moduleResource->shutOffPercent >= static_cast<float>(num5)) {
                            threshold = 1.1;
                            currentResourceLowerThanLayoff = true;
                            if (!moduleResource->autoStateShifter) throw Nre();
                            moduleResource->autoStateShifter->ToggleWarningVisibility(true);
                            events::GameEvents::onPartResourceNonemptyEmpty.Fire(moduleResource->autoStateShifter->partResource);
                        } else {
                            threshold = dotnet::Math::Max(threshold, 0.001);
                            currentResourceLowerThanLayoff = false;
                            if (!moduleResource->autoStateShifter) throw Nre();
                            moduleResource->autoStateShifter->ToggleWarningVisibility(false);
                        }
                    } else if (static_cast<double>(moduleResource->shutOffAmount) >= amount) {
                        threshold = 1.1;
                        currentResourceLowerThanLayoff = true;
                        if (!moduleResource->autoStateShifter) throw Nre();
                        moduleResource->autoStateShifter->ToggleWarningVisibility(true);
                        events::GameEvents::onPartResourceNonemptyEmpty.Fire(moduleResource->autoStateShifter->partResource);
                    } else {
                        threshold = dotnet::Math::Max(threshold, 0.001);
                        currentResourceLowerThanLayoff = false;
                        if (!moduleResource->autoStateShifter) throw Nre();
                        moduleResource->autoStateShifter->ToggleWarningVisibility(false);
                    }
                } else {
                    threshold = dotnet::Math::Max(threshold, 0.001);
                }
                if (moduleResource->currentAmount < moduleResource->currentRequest * threshold) {
                    moduleResource->available = false;
                    if (returnOnFirstLack) break;
                    if (flag) {
                        flag = false;
                        if (stringOps) error = Localizer::Format("#autoLOC_244419", {KSPUtil::PrintLocalizedModuleName(moduleResource->name)});
                        num = !moduleResource->shutOffHandler
                                  ? (!average ? moduleResource->currentAmount / moduleResource->currentRequest
                                              : num + moduleResource->currentAmount / moduleResource->currentRequest)
                                  : (!average ? 0.0 : num + amount / maxAmount);
                    } else {
                        if (stringOps) error = error + ", " + KSPUtil::PrintLocalizedModuleName(moduleResource->name);
                        num = !moduleResource->shutOffHandler
                                  ? (!average ? moduleResource->currentAmount / moduleResource->currentRequest
                                              : num + UtilMath::Clamp01(moduleResource->currentAmount / moduleResource->currentRequest))
                                  : (!average ? 0.0 : num + amount / maxAmount);
                    }
                } else {
                    moduleResource->available = true;
                    if (average) num += 1.0;
                }
            }
            num3++;
            continue;
        }
        if (average) num /= static_cast<double>(count);
        return num;
    }
    if (stringOps) {
        error = Localizer::Format("#autoLOC_6001043", {KSPUtil::PrintLocalizedModuleName(moduleResource->name),
                                               dotnet::FormatDoubleF(moduleResource->currentAmount / num2, 1), dotnet::FormatDoubleF(num4, 1)});
    }
    return 0.0;
}

// ModuleResourceHandler.cs:269
bool ModuleResourceHandler::UpdateModuleResourceInputs(std::string& error, bool useFlowMode, double rateMultiplier, double threshold,
                                                       bool returnOnFirstLack, bool stringOps) {
    return UpdateModuleResourceInputs(error, useFlowMode, rateMultiplier, threshold, returnOnFirstLack, false, stringOps) > 0.0;
}

// ModuleResourceHandler.cs:274
bool ModuleResourceHandler::UpdateModuleResourceInputs(std::string& error, double rateMultiplier, double threshold, bool returnOnFirstLack,
                                                       bool stringOps) {
    return UpdateModuleResourceInputs(error, rateMultiplier, threshold, returnOnFirstLack, false, stringOps) > 0.0;
}

// ModuleResourceHandler.cs:279
double ModuleResourceHandler::UpdateModuleResourceOutputs(double rateMultiplier, double minAbsValue) {
    int count = static_cast<int>(outputResources.size());
    if (static_cast<double>(count) == 0.0) return 0.0;
    double num = static_cast<double>(TimeWarp::fixedDeltaTime());
    double num2 = 0.0;
    for (int i = 0; i < count; i++) {
        ModuleResource* moduleResource = outputResources[static_cast<size_t>(i)].get();
        double num3 = moduleResource->rate * rateMultiplier;
        moduleResource->currentRequest = num3 * num;
        if (moduleResource->currentRequest > minAbsValue) {
            moduleResource->currentAmount = dotnet::Math::Abs(ModulePart(partModule)->RequestResource(moduleResource->id, 0.0 - moduleResource->currentRequest));
            if (num3 > num2) num2 = num3;
        }
    }
    return num2;
}

// ModuleResourceHandler.cs:305
bool ModuleResourceHandler::HasEnoughResourcesToAutoStart() {
    int count = static_cast<int>(inputResources.size());
    if (count == 0) return false;
    if (ResourceFlowEnvironment::InfinitePropellant) return true;
    int num = 0;
    while (true) {
        if (num < count) {
            ModuleResource* moduleResource = inputResources[static_cast<size_t>(num)].get();
            Part* p = ModulePart(partModule);
            double amount = 0.0, maxAmount = 0.0;
            IResourceVessel* h = ResourceFlowEnvironment::Host(p->vessel);
            if (h == nullptr) throw Nre();
            h->GetConnectedResourceTotals(moduleResource->id, false, amount, maxAmount, true);
            if (moduleResource->startUpHandler) {
                if (moduleResource->shutOffStartUpUsePercent) {
                    double num2 = amount / maxAmount * 100.0;
                    if (moduleResource->startUpPercent <= static_cast<float>(num2)) return true;
                } else if (!(amount < static_cast<double>(moduleResource->startUpAmount))) {
                    break;
                }
                num++;
                continue;
            }
            return false;
        }
        return false;
    }
    return true;
}

// ModuleResourceHandler.cs:348
bool ModuleResourceHandler::IsResourceBelowShutOffLimit(PartResource& partRes) {
    int count = static_cast<int>(inputResources.size());
    if (count == 0) return false;
    int num = 0;
    while (true) {
        if (num < count) {
            ModuleResource* moduleResource = inputResources[static_cast<size_t>(num)].get();
            if (moduleResource->shutOffHandler) {
                if (moduleResource->name == partRes.resourceName && moduleResource->currentAmount > static_cast<double>(moduleResource->shutOffAmount))
                    return true;
            } else if (!(moduleResource->currentAmount < 0.009999999776482582)) {
                break;
            }
            num++;
            continue;
        }
        return false;
    }
    return true;
}

}  // namespace ksp
