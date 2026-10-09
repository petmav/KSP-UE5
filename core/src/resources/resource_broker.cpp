// ResourceBroker.cs, ResourceConverter.cs, ResourceSetup.cs (WP-15): see ksp/resources/resource_broker.hpp.
#include "ksp/resources/resource_broker.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/resource_flow_host.hpp"

namespace ksp {

namespace {
NullReferenceException Nre() { return NullReferenceException("Object reference not set to an instance of an object"); }

ResourceFlowMode DefaultFlowOf(int resID) {
    const PartResourceLibrary* lib = PartResourceLibrary::Instance();
    const PartResourceDefinition* d = lib ? lib->GetDefinition(resID) : nullptr;
    if (d == nullptr) throw Nre();
    return AsFlowMode(d->resourceFlowMode());
}

// ResourceSetup.Instance.ResConfig.ECMinScale
int ECMinScale() {
    ResourceSetup::ResourceConfig* c = ResourceSetup::Instance().ResConfig();
    if (c == nullptr) throw Nre();
    return c->ECMinScale;
}
}  // namespace

// ---- ResourceSetup ---------------------------------------------------------------------------------------------------------

// ResourceSetup.cs:20
ResourceSetup& ResourceSetup::Instance() {
    if (!instance_) instance_.reset(new ResourceSetup());
    return *instance_;
}
void ResourceSetup::DestroyInstance() { instance_.reset(); }
void ResourceSetup::ResetConfig() {
    resConfig_.reset();
    resConfigLoaded_ = false;
}

// ResourceSetup.cs:22
ResourceSetup::ResourceConfig* ResourceSetup::ResConfig() {
    if (!resConfigLoaded_) {
        resConfig_.reset(LoadResourceConfig());
        resConfigLoaded_ = resConfig_ != nullptr;  // `_resConfig ?? (_resConfig = ...)`: a null result is looked up again
    }
    return resConfig_.get();
}

// ResourceSetup.cs:33 (ResourceUtilities.LoadNodeProperties<ResourceConfig>: Convert.ChangeType per property; unknown names ignored)
ResourceSetup::ResourceConfig* ResourceSetup::LoadResourceConfig() {
    const GameDatabase* db = database != nullptr ? database : GameDatabase::Instance();
    if (db == nullptr) throw Nre();
    std::vector<ConfigNode*> nodes = db->GetConfigNodes("RESOURCE_CONFIGURATION");
    if (nodes.empty()) return nullptr;
    auto* cfg = new ResourceConfig();
    for (const ConfigNode::Value& v : nodes[0]->values()) {
        if (v.name == "HeatEnabled") cfg->HeatEnabled = cs::BoolParse(v.value);
        else if (v.name == "ECMinScale") cfg->ECMinScale = dotnet::ParseInt32(v.value);
        else if (v.name == "OverlayStyle") cfg->OverlayStyle = dotnet::ParseInt32(v.value);
        else if (v.name == "ShowDebugOptions") cfg->ShowDebugOptions = cs::BoolParse(v.value);
    }
    return cfg;
}

// ---- ResourceBroker --------------------------------------------------------------------------------------------------------

// ResourceBroker.cs:3
double ResourceBroker::AmountAvailable(Part* part, const std::string& resName, double deltaTime, ResourceFlowMode flowMode) {
    return AmountAvailable(part, MonoStringHash(resName), deltaTime, flowMode);
}

// ResourceBroker.cs:8
double ResourceBroker::AmountAvailable(Part* part, int resID, double deltaTime, ResourceFlowMode flowMode) {
    if (flowMode == ResourceFlowMode::NULL_) flowMode = DefaultFlowOf(resID);
    double amount = 0.0, maxAmount = 0.0;
    part->GetConnectedResourceTotals(resID, flowMode, amount, maxAmount);
    if (resID == PartResourceLibrary::ElectricityHashcode() && deltaTime > 0.02 * static_cast<double>(ECMinScale())) {
        double num = deltaTime / 0.02;
        return amount * num;
    }
    return amount;
}

// ResourceBroker.cs:23
double ResourceBroker::RequestResource(Part* part, const std::string& resName, double resAmount, double deltaTime, ResourceFlowMode flowMode) {
    return RequestResource(part, MonoStringHash(resName), resAmount, deltaTime, flowMode);
}

// ResourceBroker.cs:28
double ResourceBroker::RequestResource(Part* part, int resID, double resAmount, double deltaTime, ResourceFlowMode flowMode) {
    if (flowMode == ResourceFlowMode::NULL_) flowMode = DefaultFlowOf(resID);
    double num = resAmount;
    if (resID == PartResourceLibrary::ElectricityHashcode() && deltaTime > 0.02 * static_cast<double>(ECMinScale())) {
        double num2 = deltaTime / 0.02;
        num /= num2;
    }
    return part->RequestResource(resID, num, flowMode);
}

// ResourceBroker.cs:43
double ResourceBroker::StorageAvailable(Part* part, const std::string& resName, double deltaTime, ResourceFlowMode flowMode, double FillAmount) {
    return StorageAvailable(part, MonoStringHash(resName), deltaTime, flowMode, FillAmount);
}

// ResourceBroker.cs:48
double ResourceBroker::StorageAvailable(Part* part, int resID, double /*deltaTime*/, ResourceFlowMode flowMode, double FillAmount) {
    if (flowMode == ResourceFlowMode::NULL_) flowMode = DefaultFlowOf(resID);
    double amount = 0.0, maxAmount = 0.0;
    part->GetConnectedResourceTotals(resID, flowMode, amount, maxAmount, FillAmount, false);
    return amount;
}

// ResourceBroker.cs:58
double ResourceBroker::StoreResource(Part* part, const std::string& resName, double resAmount, double deltaTime, ResourceFlowMode flowMode) {
    return StoreResource(part, MonoStringHash(resName), resAmount, deltaTime, flowMode);
}

// ResourceBroker.cs:63
double ResourceBroker::StoreResource(Part* part, int resID, double resAmount, double /*deltaTime*/, ResourceFlowMode flowMode) {
    if (flowMode == ResourceFlowMode::NULL_) flowMode = DefaultFlowOf(resID);
    return part->RequestResource(resID, 0.0 - resAmount, flowMode);
}

// ---- ResourceConverter -----------------------------------------------------------------------------------------------------

// ResourceConverter.cs:23 (the UIPartActionController status texts are shown only while the part's PAW is open:
// ResourceFlowEnvironment::partActionUIContains, absent = never)
ConverterResults ResourceConverter::ProcessRecipe(double deltaTime, ConversionRecipe& recipe, Part* resPart, PartModule* resModule, float efficiencyBonus) {
    ConverterResults result;
    auto uiOpen = [&]() { return ResourceFlowEnvironment::partActionUIContains && ResourceFlowEnvironment::partActionUIContains(resPart); };
    auto defName = [](const std::string& n) {
        const PartResourceLibrary* lib = PartResourceLibrary::Instance();
        const PartResourceDefinition* d = lib ? lib->GetDefinition(n) : nullptr;
        return d != nullptr ? d->displayName() : n;
    };
    if (uiOpen()) result.Status = cacheAutoLOC_6002353;
    double num = 1.0;
    int count = static_cast<int>(recipe.Requirements().size());
    int num2 = 0;
    ResourceRatio resourceRatio;
    while (true) {
        if (num2 < count) {
            resourceRatio = recipe.Requirements()[static_cast<size_t>(num2)];
            PartResource* r = resPart->Resources.Get(resourceRatio.ResourceName);
            if (r == nullptr) throw Nre();
            double num3 = r->amount / dotnet::Math::Abs(resourceRatio.Ratio);
            if (resourceRatio.Ratio < 0.0) num3 = 1.0 - num3;
            if (num3 < num) num = num3;
            if (!(num > 1E-09)) break;
            num2++;
            continue;
        }
        num *= static_cast<double>(efficiencyBonus);
        if (num <= 1E-09) {
            if (uiOpen()) result.Status = cacheAutoLOC_261274;
            result.TimeFactor = 0.0;
            return result;
        }
        double num4 = deltaTime;
        if (!ResourceFlowEnvironment::InfinitePropellant) {
            int count2 = static_cast<int>(recipe.Inputs().size());
            for (int i = 0; i < count2; i++) {
                ResourceRatio resourceRatio2 = recipe.Inputs()[static_cast<size_t>(i)];
                if (ResourceFlowEnvironment::InfiniteElectricity && resourceRatio2.ResourceName == "ElectricCharge") continue;
                double num5 = broker_->AmountAvailable(resPart, resourceRatio2.ResourceName, deltaTime, resourceRatio2.FlowMode) *
                              static_cast<double>(recipe.TakeAmount);
                double num6 = resourceRatio2.Ratio * num4 * num;
                if (!(num5 < num6)) continue;
                num4 *= num5 / num6;
                if (!(num4 > 1E-09)) {
                    if (uiOpen()) result.Status = Localizer::Format("#autoLOC_261304", {defName(resourceRatio2.ResourceName)});
                    result.TimeFactor = 0.0;
                    return result;
                }
            }
        }
        int count3 = static_cast<int>(recipe.Outputs().size());
        int num7 = 0;
        ResourceRatio resourceRatio3;
        bool outputBlocked = false;
        while (true) {
            if (num7 < count3) {
                resourceRatio3 = recipe.Outputs()[static_cast<size_t>(num7)];
                if (!resourceRatio3.DumpExcess) {
                    double num8 = broker_->StorageAvailable(resPart, resourceRatio3.ResourceName, deltaTime, resourceRatio3.FlowMode,
                                                            static_cast<double>(recipe.FillAmount));
                    double num9 = resourceRatio3.Ratio * num4 * num;
                    if (num8 < num9) {
                        num4 *= num8 / num9;
                        if (!(num4 > 1E-09)) {
                            outputBlocked = true;
                            break;
                        }
                    }
                }
                num7++;
                continue;
            }
            int count4 = static_cast<int>(recipe.Inputs().size());
            for (int j = 0; j < count4; j++) {
                ResourceRatio resourceRatio4 = recipe.Inputs()[static_cast<size_t>(j)];
                double resAmount = resourceRatio4.Ratio * num4 * num;
                broker_->RequestResource(resPart, resourceRatio4.ResourceName, resAmount, deltaTime, resourceRatio4.FlowMode);
            }
            int count5 = static_cast<int>(recipe.Outputs().size());
            for (int k = 0; k < count5; k++) {
                ResourceRatio resourceRatio5 = recipe.Outputs()[static_cast<size_t>(k)];
                double num10 = resourceRatio5.Ratio * num4 * num;
                broker_->StoreResource(resPart, resourceRatio5.ResourceName, num10, deltaTime, resourceRatio5.FlowMode);
                events::GameEvents::OnResourceConverterOutput.Fire(resModule, resourceRatio5.ResourceName, num10);
            }
            result.TimeFactor = num4 * num;
            return result;
        }
        (void)outputBlocked;
        if (recipe.FillAmount < 1.0f) {
            if (uiOpen()) result.Status = Localizer::Format("#autoLOC_261332", {dotnet::FormatSingleF(recipe.FillAmount * 100.0f, 0) + "%"});
        } else {
            if (uiOpen()) result.Status = Localizer::Format("#autoLOC_261334", {defName(resourceRatio3.ResourceName)});
        }
        result.TimeFactor = 0.0;
        return result;
    }
    // a requirement is not met (the `break` out of the requirement loop)
    if (uiOpen()) result.Status = Localizer::Format("#autoLOC_261263", {defName(resourceRatio.ResourceName)});
    result.TimeFactor = 0.0;
    return result;
}

// ResourceConverter.cs:163
void ResourceConverter::CacheLocalStrings() {
    cacheAutoLOC_6002353 = Localizer::Format("#autoLOC_6002353");
    cacheAutoLOC_261274 = Localizer::Format("#autoLOC_261274");
}

}  // namespace ksp
