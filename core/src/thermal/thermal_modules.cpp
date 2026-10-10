// TemperatureModel.cs, RadiatorUtilities.cs, ModuleConductionMultiplier.cs, ModuleAblator.cs, ModuleCoreHeat.cs and
// ModuleActiveRadiator.cs, operation for operation (paths relative to decompiled/KSP-Decompiled/Assembly-CSharp/). See
// include/ksp/thermal/thermal_modules.hpp for the seams and what is not ported.
#include "ksp/thermal/thermal_modules.hpp"

#include <cmath>
#include <cstdio>
#include <limits>

#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace M = dotnet::Math;

// ---------------------------------------------------------------------------------------------------------------------
// Radiators/RadiatorUtilities.cs:7
Radiators::RadiatorData RadiatorUtilities::GetThermalData(Part& part) {
    Radiators::RadiatorData result;
    result.Energy = part.fields.thermalMass * part.fields.temperature;
    result.MaxEnergy = part.fields.thermalMass * part.fields.maxTemp * part.fields.radiatorMax;
    result.EnergyCap = part.fields.thermalMass * part.fields.maxTemp * part.fields.radiatorHeadroom;
    result.part = &part;
    return result;
}

// Radiators/RadiatorUtilities.cs:17 (RadiatorData is a struct: the Part references are shared)
void RadiatorUtilities::TransferEnergy(const Radiators::RadiatorData& src, const Radiators::RadiatorData& tgt) {
    const double val = tgt.EnergyCap - tgt.Energy;
    const double val2 = src.Energy - src.MaxEnergy;
    const double num = M::Min(val, val2);
    src.part->fields.thermalInternalFlux -= num;
    tgt.part->fields.thermalInternalFlux += num;
}

// ---------------------------------------------------------------------------------------------------------------------
// TemperatureModel.cs:33
TemperatureModel::Linear::Linear(float temp1, float alt1, float temp2, float alt2) {
    m = f32store((fx(temp1) - fx(temp2)) / (fx(alt1) - fx(alt2)));
    b = f32store(fx(temp1) - fx(m) * fx(alt1));
}
// :31
float TemperatureModel::Linear::getTemperatureAtAltitude(float altitude) { return f32store(fx(m) * fx(altitude) + fx(b)); }
// :55
float TemperatureModel::Quadratic::getTemperatureAtAltitude(float altitude) {
    return f32store(fx(A) * fx(altitude) * fx(altitude) + fx(B) * fx(altitude) + fx(C));
}

// :90
bool TemperatureModel::Stepped::AddInitialLinearStepEquation(float min, float max, float m, float b) {
    if (!Steps.empty()) return false;
    TempRange item;
    item.minAlt = min;
    item.maxAlt = max;
    item.modelName = TemperatureModels::LINEAR;
    item.modelData = std::make_shared<Linear>(m, b);
    Steps.push_back(item);
    return true;
}
// :105
bool TemperatureModel::Stepped::AddInitialLinearStep(float temp1, float alt1, float temp2, float alt2) {
    if (!Steps.empty()) return false;
    TempRange item;
    item.minAlt = (temp1 < temp2) ? temp1 : temp2;
    item.maxAlt = (temp1 > temp2) ? temp1 : temp2;
    item.modelName = TemperatureModels::LINEAR;
    item.modelData = std::make_shared<Linear>(temp1, alt1, temp2, alt2);
    Steps.push_back(item);
    return true;
}
// :121
bool TemperatureModel::Stepped::AddInitialQuadraticStep(float min, float max, float A, float B, float C) {
    if (!Steps.empty()) return false;
    TempRange item;
    item.minAlt = min;
    item.maxAlt = max;
    item.modelName = TemperatureModels::QUADRATIC;
    item.modelData = std::make_shared<Quadratic>(A, B, C);
    Steps.push_back(item);
    return true;
}
// :137 (modelName LINEAR is what the decompile stores for the constant initial step)
bool TemperatureModel::Stepped::AddInitialConstantStep(float min, float max, float K) {
    if (!Steps.empty()) return false;
    TempRange item;
    item.minAlt = min;
    item.maxAlt = max;
    item.modelName = TemperatureModels::LINEAR;
    item.modelData = std::make_shared<Constant>(K);
    Steps.push_back(item);
    return true;
}
// :153
bool TemperatureModel::Stepped::AddConstantStep(float limit, float K) {
    if (Steps.empty()) return AddInitialConstantStep((limit < 0.0f) ? limit : 0.0f, (limit < 0.0f) ? 0.0f : limit, K);
    const float num = (limit < 0.0f) ? 0.0f : limit;
    int num2 = 0;
    const int count = static_cast<int>(Steps.size());
    while (true) {
        if (num2 < count) {
            if (!(num <= Steps[num2].maxAlt)) break;
            num2++;
            continue;
        }
        return false;
    }
    TempRange item;
    item.minAlt = Steps[num2].maxAlt;
    item.maxAlt = num;
    item.modelName = TemperatureModels::CONSTANT;
    item.modelData = std::make_shared<Constant>(K);
    Steps.push_back(item);
    return true;
}
// :185
bool TemperatureModel::Stepped::AddLinearStep(float limit, float temp2) {
    if (Steps.empty())
        return AddInitialLinearStep((limit < 0.0f) ? temp2 : 0.0f, (limit < 0.0f) ? limit : 0.0f, (limit < 0.0f) ? 0.0f : temp2,
                                    (limit < 0.0f) ? 0.0f : limit);
    const float num = (limit < 0.0f) ? 0.0f : limit;
    int num2 = 0;
    const int count = static_cast<int>(Steps.size());
    while (true) {
        if (num2 < count) {
            if (!(num <= Steps[num2].maxAlt)) break;
            num2++;
            continue;
        }
        return false;
    }
    TempRange item;
    item.minAlt = Steps[num2].maxAlt;
    item.maxAlt = num;
    item.modelName = TemperatureModels::LINEAR;
    item.modelData = std::make_shared<Linear>(Steps[num2].modelData->getTemperatureAtAltitude(Steps[num2].maxAlt), Steps[num2].maxAlt,
                                              temp2, limit);
    Steps.push_back(item);
    return true;
}
// :215
bool TemperatureModel::Stepped::AddQuadraticStep(float limit, float A, float B, float C) {
    if (Steps.empty())
        return AddInitialQuadraticStep((limit < 0.0f) ? limit : 0.0f, (limit < 0.0f) ? 0.0f : limit, A, B, C);
    const float num = (limit < 0.0f) ? 0.0f : limit;
    int num2 = 0;
    const int count = static_cast<int>(Steps.size());
    while (true) {
        if (num2 < count) {
            if (!(num <= Steps[num2].maxAlt)) break;
            num2++;
            continue;
        }
        return false;
    }
    TempRange item;
    item.minAlt = Steps[num2].maxAlt;
    item.maxAlt = num;
    item.modelName = TemperatureModels::QUADRATIC;
    item.modelData = std::make_shared<Quadratic>(A, B, C);
    Steps.push_back(item);
    return true;
}
// :245
float TemperatureModel::Stepped::getTemperatureAtAltitude(float altitude) {
    int num = 0;
    const int count = static_cast<int>(Steps.size());
    while (true) {
        if (num < count) {
            if (static_cast<double>(altitude) < static_cast<double>(Steps[num].maxAlt) + 0.0001 &&
                !(static_cast<double>(altitude) <= static_cast<double>(Steps[num].minAlt) - 0.0001))
                break;
            num++;
            continue;
        }
        return 0.0f;
    }
    return Steps[num].modelData->getTemperatureAtAltitude(altitude);
}

// ---------------------------------------------------------------------------------------------------------------------
// ModuleConductionMultiplier.cs:15
void ModuleConductionMultiplier::OnAwake() {
    if (Part* p = dynamic_cast<Part*>(part)) originalConduction = p->fields.heatConductivity;
}
// :19
void ModuleConductionMultiplier::Start() { threshRecip = 1.0 / Get<double>("convectionFluxThreshold"); }
// :24
void ModuleConductionMultiplier::FixedUpdate() {
    if (!ThermalModuleEnvironment::loadedSceneIsFlight) return;
    Part& p = *dynamic_cast<Part*>(part);
    const double modifiedConductionFactor = Get<double>("modifiedConductionFactor");
    const double convectionFluxThreshold = Get<double>("convectionFluxThreshold");
    if (p.fields.exposedArea != 0.0 && convectionFluxThreshold != 0.0) {
        const double num = p.fields.thermalConvectionFlux / p.fields.exposedArea;
        currentConduction = UtilMath::Lerp(originalConduction, originalConduction * modifiedConductionFactor,
                                           (num - convectionFluxThreshold) * threshRecip);
        p.fields.heatConductivity = currentConduction;
    } else {
        p.fields.heatConductivity = originalConduction;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
Part* ModuleAblator::PartOf() const { return dynamic_cast<Part*>(part); }

// ModuleAblator.cs:106 (the char module and the node lookup; PartResourceLibrary / GetConnectedResourceTotals as in C#)
void ModuleAblator::Start() {
    Part& p = *PartOf();
    if (Get<bool>("useChar") && !charModuleFound) Set<bool>("useChar", false);  // charModule == null -> useChar = false
    if (Get<bool>("useNode")) occNode = p.FindAttachNode(Get<std::string>("nodeName"));
    useOutput = false;
    useAblator = false;
    if (!ThermalModuleEnvironment::loadedSceneIsFlight) return;
    const std::string ablativeResource = Get<std::string>("ablativeResource");
    if (!ablativeResource.empty()) {
        const PartResourceDefinition* definition = nullptr;
        if (Get<double>("lossExp") < 0.0 && Get<bool>("moduleIsEnabled") &&
            (definition = PartResourceLibrary::Instance()->GetDefinition(ablativeResource)) != nullptr) {
            pyrolysisLoss = Get<double>("pyrolysisLossFactor") * static_cast<double>(definition->specificHeatCapacity());
            density = static_cast<double>(definition->density());
            useAblator = true;
            ablativeID = definition->id();
            ablativeFlowMode = definition->resourceFlowMode();
            double amount = 0.0, maxAmount = 0.0;
            p.GetConnectedResourceTotals(ablativeID, static_cast<ResourceFlowMode>(ablativeFlowMode), amount, maxAmount);
            if (amount > 0.0) {
                if (Get<double>("nominalAmountRecip") < 0.0) Set<double>("nominalAmountRecip", 1.0 / amount);
                ablativeAmount = amount;
            } else {
                const double num = 1.0;
                Set<double>("nominalAmountRecip", 1.0);
                ablativeAmount = num;
            }
            const std::string outputResource = Get<std::string>("outputResource");
            if (!outputResource.empty() && (definition = PartResourceLibrary::Instance()->GetDefinition(outputResource)) != nullptr) {
                outputID = definition->id();
                outputFlowMode = definition->resourceFlowMode();
                useOutput = true;
            }
        }
    } else {
        Set<double>("nominalAmountRecip", 1.0);
    }
    origConductivity = p.fields.heatConductivity;
}

// ModuleAblator.cs:168
void ModuleAblator::FixedUpdate() {
    if (!ThermalModuleEnvironment::loadedSceneIsFlight) return;
    Part& p = *PartOf();
    double flux = 0.0, loss = 0.0;
    Set<double>("flux", flux);
    Set<double>("loss", loss);
    const bool useNode = Get<bool>("useNode");
    const bool flag = (!useNode) ? (static_cast<double>(p.DragCubes.AreaOccluded()[downDir]) <= 0.1 * static_cast<double>(p.DragCubes.WeightedArea()[downDir]))
                                 : (occNode == nullptr || ((occNode->attachedPart != nullptr) ? true : false));
    if (flag)
        p.fields.heatConductivity = origConductivity;
    else
        p.fields.heatConductivity = Get<double>("reentryConductivity");
    if (!useAblator || !Get<bool>("moduleIsEnabled") || !(p.fields.skinTemperature > Get<double>("ablationTempThresh"))) return;
    p.GetConnectedResourceTotals(ablativeID, static_cast<ResourceFlowMode>(ablativeFlowMode), ablativeAmount, ablativeMaxAmount);
    if (!(ablativeAmount > 0.0)) return;
    loss = Get<double>("lossConst") * M::Exp(Get<double>("lossExp") / p.fields.skinTemperature);
    if (!(loss > 0.0)) {
        Set<double>("loss", loss);
        return;
    }
    loss *= ablativeMaxAmount;
    const double num = loss * static_cast<double>(TimeWarp::fixedDeltaTime());
    const double outputMult = Get<double>("outputMult");
    if (num > ablativeAmount) {
        loss = ablativeAmount / static_cast<double>(TimeWarp::fixedDeltaTime());
        p.RequestResource(ablativeID, num, static_cast<ResourceFlowMode>(ablativeFlowMode));
        if (useOutput) p.RequestResource(outputID, (0.0 - ablativeAmount) * outputMult, static_cast<ResourceFlowMode>(outputFlowMode));
    } else {
        p.RequestResource(ablativeID, num, static_cast<ResourceFlowMode>(ablativeFlowMode));
        if (useOutput) p.RequestResource(outputID, (0.0 - num) * outputMult, static_cast<ResourceFlowMode>(outputFlowMode));
    }
    loss *= density;
    flux = pyrolysisLoss * loss;
    p.AddExposedThermalFlux(0.0 - flux);
    loss *= 1000.0;
    Set<double>("flux", flux);
    Set<double>("loss", loss);
}

// ModuleAblator.cs:236 UpdateColor: `float num = (float)(ablativeAmount * nominalAmountRecip); if (num > 1f) num = 1f;
// float num2 = charMax - charMin; float scalar = charMin + num2 * num;`
float ModuleAblator::CharScalar() const {
    float num = f32store(ablativeAmount * Get<double>("nominalAmountRecip"));
    if (num > 1.0f) num = 1.0f;
    const float charMax = Get<float>("charMax"), charMin = Get<float>("charMin");
    const float num2 = f32store(fx(charMax) - fx(charMin));
    return f32store(fx(charMin) + fx(num2) * fx(num));
}
void ModuleAblator::UpdateColor() {}

// ---------------------------------------------------------------------------------------------------------------------
Part* ModuleActiveRadiator::PartOf() const { return dynamic_cast<Part*>(part); }

// ModuleActiveRadiator.cs:206 (the GameEvents subscription is the caller's: set partCachesDirty on onVesselStandardModification)
void ModuleActiveRadiator::OnAwake() {}

// :152
void ModuleActiveRadiator::Start() {
    originalMaxEnergyTranfer = Get<double>("maxEnergyTransfer");
    if (deployableExtended) Set<bool>("IsCooling", Get<bool>("IsCooling"));  // CheckIfDeployable: the deployable decides in FixedUpdate
}

// :485
void ModuleActiveRadiator::CheckPartCaches() {
    if (!partCachesDirty) return;
    activeRadiatorParts.clear();
    nonRadiatorParts.clear();
    Vessel* v = PartOf()->vessel;
    int count = static_cast<int>(v->parts.size());
    while (count-- > 0) {
        Part* part_ = v->parts[count];
        if (part_->FindModuleOfClass("ModuleActiveRadiator") != nullptr)
            activeRadiatorParts.push_back(part_);
        else
            nonRadiatorParts.push_back(part_);
    }
    partCachesDirty = false;
}

// :353
bool ModuleActiveRadiator::IsSibling(Part& targetPart) {
    Part* parent = PartOf()->parent;
    if (&targetPart == parent) return true;
    if (parent->parent != nullptr && &targetPart == parent->parent) return true;
    if (targetPart.parent != nullptr && targetPart.parent == parent) return true;
    return false;
}

// :216 (the adjuster block and the status text are not ported: status keeps the percentage)
void ModuleActiveRadiator::FixedUpdate() {
    Part& p = *PartOf();
    if (!ThermalModuleEnvironment::loadedSceneIsFlight || p.vessel == nullptr) return;
    CheckPartCaches();
    if (deployableExtended) Set<bool>("IsCooling", deployableExtended());
    if (!Get<bool>("IsCooling")) {
        statusValue = -1.0;
        return;
    }
    if (checkResources && !checkResources()) return;
    Set<double>("maxEnergyTransfer", originalMaxEnergyTranfer);  // ApplyMaxEnergyTransferAdjustments with no adjusters
    const double num = M::Round(100.0 * p.fields.temperature / (p.fields.maxTemp * p.fields.radiatorHeadroom), 2, dotnet::MidpointRounding::ToEven);
    if (num != statusValue) {
        char buf[64];
        std::snprintf(buf, sizeof buf, "%.2f%%", num);
        Set<std::string>("status", std::string(buf));
        statusValue = num;
    }
    bool extended = false;
    if (isRadiator && isRadiator(p, extended) && !extended) return;
    int num2 = 0;
    int count = static_cast<int>(activeRadiatorParts.size());
    while (count-- > 0) {
        bool ext2 = false;
        if (isRadiator && isRadiator(*activeRadiatorParts[count], ext2)) {
            if (ext2) num2++;
        } else {
            num2++;
        }
    }
    if (num2 > 0) {
        Radiators::RadiatorData thermalData = RadiatorUtilities::GetThermalData(p);
        InternalCooling(thermalData, num2);
    }
}

// :276
void ModuleActiveRadiator::InternalCooling(Radiators::RadiatorData& rad, int radCount) {
    Part& p = *PartOf();
    coolParts.clear();
    hotParts.clear();
    int count = static_cast<int>(nonRadiatorParts.size());
    while (count-- > 0) {
        Part* part_ = nonRadiatorParts[count];
        if (part_->fields.temperature > part_->fields.maxTemp * part_->fields.radiatorMax &&
            part_->fields.temperature > p.vessel->externalTemperature)
            hotParts.push_back(part_);
    }
    const int count2 = static_cast<int>(hotParts.size());
    const bool parentCoolingOnly = Get<bool>("parentCoolingOnly");
    for (int i = 0; i < count2; i++) {
        Part* part2 = hotParts[i];
        bool flag = true;
        if (parentCoolingOnly) flag = IsSibling(*part2);
        if (!flag) continue;
        const double num = p.fields.temperature * Get<double>("overcoolFactor");
        if (part2->fields.temperature > num) {
            Radiators::RadiatorData thermalData = RadiatorUtilities::GetThermalData(*part2);
            if (thermalData.Energy - thermalData.MaxEnergy > 0.0) coolParts.push_back(thermalData);
        }
    }
    const int count3 = static_cast<int>(coolParts.size());
    for (int j = 0; j < count3; j++) {
        Radiators::RadiatorData radiatorData = coolParts[j];
        double num2 = (radiatorData.Energy - radiatorData.MaxEnergy) / static_cast<double>(TimeWarp::fixedDeltaTime());
        num2 /= static_cast<double>(radCount + count3);
        const double maxEnergyTransfer = Get<double>("maxEnergyTransfer");
        const double val = M::Min(rad.EnergyCap - rad.Energy, maxEnergyTransfer) / static_cast<double>(TimeWarp::fixedDeltaTime());
        double num3 = M::Min(val, num2);
        num3 *= M::Min(1.0, Get<double>("energyTransferScale"));
        if (num3 > 0.0 && !p.vessel->IsFirstFrame()) {
            radiatorData.part->AddThermalFlux(0.0 - num3);
            p.AddThermalFlux(num3);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------
Part* ModuleCoreHeat::PartOf() const { return dynamic_cast<Part*>(part); }

// ModuleCoreHeat.cs:416
void ModuleCoreHeat::OnAwake() {
    if (IsNull("PassiveEnergy")) {
        FloatCurve c;
        c.Add(0.0f, 0.0f);
        Set<FloatCurve>("PassiveEnergy", c);
    }
}

// :107 CoreTemperature
double ModuleCoreHeat::CoreTemperature() const {
    ModuleCoreHeat* self = const_cast<ModuleCoreHeat*>(this);
    const double e = Get<double>("CoreThermalEnergy");
    if (e != e) self->CheckStartingTemperature();
    return Get<double>("CoreThermalEnergy") / PartOf()->fields.thermalMass;
}

// :255 (ScreenMessages.PostScreenMessage is the shell's)
void ModuleCoreHeat::CheckCoreShutdown() {
    if (CoreTemperature() <= Get<double>("CoreShutdownTemp")) return;
    int count = static_cast<int>(converters.size());
    while (count-- > 0) {
        if (!converters[count]->autoShutdown && converters[count]->stopResourceConverter) converters[count]->stopResourceConverter();
    }
}

// :353
void ModuleCoreHeat::ResolveNetEnergy() {
    Set<double>("CoreThermalEnergy", Get<double>("CoreThermalEnergy") + energyTemp);
    energyTemp = 0.0;
}

// :359
void ModuleCoreHeat::ResolveConverterEnergy(double deltaTime) {
    const int count = static_cast<int>(converters.size());
    for (int i = 0; i < count; i++) {
        ConverterState* baseConverter = converters[i];
        if (baseConverter->isActivated && baseConverter->generatesHeat && !(baseConverter->lastTimeFactor < 1E-09)) {
            double num = static_cast<double>(baseConverter->temperatureModifier->Evaluate(static_cast<float>(CoreTemperature()))) * deltaTime;
            if (baseConverter->lastTimeFactor > 1.0) num /= baseConverter->lastTimeFactor;
            if (CoreTemperature() > Get<double>("CoreTempGoal") + Get<double>("CoreTempGoalAdjustment"))
                num -= num * baseConverter->crewHeatBonus;
            AddEnergyToCore(num);
            baseConverter->lastHeatFlux = GetLastFlux();
        }
    }
}

// :382
bool ModuleCoreHeat::HasActiveConverters() const {
    int count = static_cast<int>(converters.size());
    while (count-- > 0)
        if (converters[count]->isActivated && !(converters[count]->lastTimeFactor <= 1E-09)) return true;
    return false;
}
// :399
bool ModuleCoreHeat::HasEnabledConverters() const {
    int count = static_cast<int>(converters.size());
    while (count-- > 0)
        if (converters[count]->isActivated) return true;
    return false;
}

// :562
void ModuleCoreHeat::CheckStartingTemperature() {
    Part& p = *PartOf();
    if (Get<double>("CoreThermalEnergy") != Get<double>("CoreThermalEnergy"))
        Set<double>("CoreThermalEnergy", p.fields.thermalMass * p.fields.temperature);
    if (Get<double>("CoreThermalEnergy") / p.fields.thermalMass < 5.0)
        Set<double>("CoreThermalEnergy", p.fields.thermalMass * p.fields.temperature);
}

// :579
double ModuleCoreHeat::GetDeltaTime() {
    if (!(ThermalModuleEnvironment::timeSinceLevelLoad < 1.0f) && ThermalModuleEnvironment::flightGlobalsReady) {
        if (M::Abs(lastUpdateTime) < 1E-09) {
            lastUpdateTime = ThermalModuleEnvironment::universalTime;
            return -1.0;
        }
        if (M::Abs(lastCoreTemp - CoreTemperature()) < 1E-09) {
            lastUpdateTime = ThermalModuleEnvironment::universalTime;
            return static_cast<double>(TimeWarp::fixedDeltaTime());
        }
        lastCoreTemp = CoreTemperature();
        const double num = M::Min(ThermalModuleEnvironment::universalTime - lastUpdateTime, Get<double>("MaxCalculationWarp") / 50.0);
        lastUpdateTime += num;
        return num;
    }
    return -1.0;
}

// :677
void ModuleCoreHeat::CheckPartCaches() {
    if (!partCachesDirty) return;
    activeRadiatorParts.clear();
    coreHeatParts.clear();
    Vessel* v = PartOf()->vessel;
    int count = static_cast<int>(v->parts.size());
    while (count-- > 0) {
        Part* part_ = v->parts[count];
        if (part_->FindModuleOfClass("ModuleActiveRadiator") != nullptr) activeRadiatorParts.push_back(part_);
        if (part_->FindModuleOfClass("ModuleCoreHeat") != nullptr) coreHeatParts.push_back(part_);
    }
    partCachesDirty = false;
}

// :554 (TimeWarp.fixedDeltaTime is float)
void ModuleCoreHeat::AddFluxToRadiator(Part& rad, double xferToPart, double deltatime) {
    Vessel* v = PartOf()->vessel;
    if ((!(v->lastUT > 1E-09) || !(deltatime > static_cast<double>(TimeWarp::fixedDeltaTime()) - 1E-09)) && !v->IsFirstFrame())
        rad.AddThermalFlux(xferToPart);
}

// :431
void ModuleCoreHeat::MoveCoreEnergyToRadiators(double excess, double deltaTime) {
    vRadList.clear();
    pRadList.clear();
    double num = 0.0;
    int count = static_cast<int>(activeRadiatorParts.size());
    for (int i = 0; i < count; i++) {
        Part* part_ = activeRadiatorParts[i];
        bool extended = false;
        if (ModuleActiveRadiator::isRadiator && ModuleActiveRadiator::isRadiator(*part_, extended) && !extended) continue;
        ModuleActiveRadiator* mar = dynamic_cast<ModuleActiveRadiator*>(part_->FindModuleOfClass("ModuleActiveRadiator"));
        if (mar->Get<bool>("IsCooling") && (!mar->Get<bool>("parentCoolingOnly") || mar->IsSibling(*PartOf()))) {
            num += mar->Get<double>("maxEnergyTransfer");
            if (mar->Get<bool>("parentCoolingOnly"))
                pRadList.push_back(part_);
            else
                vRadList.push_back(part_);
        }
    }
    const int num2 = static_cast<int>(pRadList.size() + vRadList.size());
    if (num2 == 0) return;
    double num3 = 0.0;
    count = static_cast<int>(coreHeatParts.size());
    for (int j = 0; j < count; j++) {
        Part* part2 = coreHeatParts[j];
        ModuleCoreHeat* mch = dynamic_cast<ModuleCoreHeat*>(part2->FindModuleOfClass("ModuleCoreHeat"));
        double num4 = mch->Get<double>("MaxCoolant") * mch->Get<double>("radiatorHeatingFactor");
        if (mch->CoreTemperature() <= part2->fields.temperature) continue;
        int num5 = static_cast<int>(vRadList.size());
        const int count2 = static_cast<int>(pRadList.size());
        for (int k = 0; k < count2; k++) {
            ModuleActiveRadiator* pr = dynamic_cast<ModuleActiveRadiator*>(pRadList[k]->FindModuleOfClass("ModuleActiveRadiator"));
            if (pr->IsSibling(*part2)) num5++;
        }
        num4 *= static_cast<double>(num5 / num2);  // int / int
        if (mch->CoreTemperature() < mch->Get<double>("CoreTempGoal") + mch->Get<double>("CoreTempGoalAdjustment")) num4 = 0.0;
        num3 += num4;
    }
    if (num3 < 1E-09) return;
    const double radiatorHeatingFactor = Get<double>("radiatorHeatingFactor");
    const double radiatorCoolingFactor = Get<double>("radiatorCoolingFactor");
    const double MaxCoolant = Get<double>("MaxCoolant");
    double num6 = MaxCoolant * radiatorHeatingFactor;
    if (CoreTemperature() < Get<double>("CoreTempGoal") + Get<double>("CoreTempGoalAdjustment")) num6 = 0.0;
    const double num7 = num6 / num3;
    double val = num * num7 * deltaTime;
    val = M::Min(val, MaxCoolant * 50.0 * deltaTime);
    const double num8 = M::Min(val, excess);
    const double num9 = num8 * radiatorCoolingFactor;
    const double num10 = num8 * radiatorHeatingFactor;
    AddEnergyToCore(0.0 - num9);
    vRadList.insert(vRadList.end(), pRadList.begin(), pRadList.end());
    count = static_cast<int>(vRadList.size());
    for (int l = 0; l < count; l++) {
        Part* part3 = vRadList[l];
        ModuleActiveRadiator* mar = dynamic_cast<ModuleActiveRadiator*>(part3->FindModuleOfClass("ModuleActiveRadiator"));
        const double num11 = mar->Get<double>("maxEnergyTransfer") / 50.0 / num;
        const double xferToPart = num10 * num11 / deltaTime;
        AddFluxToRadiator(*part3, xferToPart, deltaTime);
    }
}

// :273
void ModuleCoreHeat::FixedUpdate() {
    const double deltaTime = GetDeltaTime();
    Part& p = *PartOf();
    if (!ThermalModuleEnvironment::loadedSceneIsFlight || p.vessel == nullptr) return;
    CheckPartCaches();
    CheckCoreShutdown();
    CheckStartingTemperature();
    if (!(deltaTime < 1E-09)) {
        AddEnergyToCore(static_cast<double>(Get<FloatCurve>("PassiveEnergy").Evaluate(static_cast<float>(CoreTemperature()))) * deltaTime);
        const double num = p.fields.thermalMass * p.fields.temperature;
        const double num2 = p.fields.thermalMass * (Get<double>("CoreTempGoal") + Get<double>("CoreTempGoalAdjustment"));
        double num3 = 0.0;
        double num4 = 0.0;
        double energy = 0.0;
        const double CoreThermalEnergy = Get<double>("CoreThermalEnergy");
        if (CoreThermalEnergy > num) {
            energy = (CoreThermalEnergy - num) * Get<double>("CoreEnergyMultiplier") * Get<double>("CoreToPartRatio");
            num3 = GetTransferAmount(energy, Get<double>("HeatRadiantMultiplier"));
            const double num5 = CoreThermalEnergy * Get<double>("CoreEnergyMultiplier") * Get<double>("CoreToPartRatio") *
                                Get<double>("CoolantTransferMultiplier");
            const double val = CoreThermalEnergy - num2;
            num4 = (!HasActiveConverters()) ? (0.0 - num5) : (0.0 - M::Min(num5, val));
        } else if (CoreThermalEnergy < num) {
            energy = (num - CoreThermalEnergy) * Get<double>("CoreEnergyMultiplier") * Get<double>("CoreToPartRatio");
            num3 = 0.0 - GetTransferAmount(energy, Get<double>("CoolingRadiantMultiplier"));
            num4 = CoreThermalEnergy * Get<double>("CoreEnergyMultiplier") * Get<double>("CoreToPartRatio") * Get<double>("HeatTransferMultiplier");
        }
        lastFlux = num3;
        Set<double>("lastFlux", lastFlux);
        if (!p.vessel->IsFirstFrame()) p.AddThermalFlux(lastFlux);
        num4 *= deltaTime;
        AddEnergyToCore(num4);
        ResolveConverterEnergy(deltaTime);
        ResolveNetEnergy();
        if (Get<double>("CoreThermalEnergy") > num2) {
            const double excess = Get<double>("CoreThermalEnergy") - num2;
            MoveCoreEnergyToRadiators(excess, deltaTime);
        } else if (Get<double>("CoreThermalEnergy") > num && !HasEnabledConverters()) {
            const double excess2 = Get<double>("CoreThermalEnergy") - num;
            MoveCoreEnergyToRadiators(excess2, deltaTime);
        }
        ResolveNetEnergy();
    }
}

// ---------------------------------------------------------------------------------------------------------------------
namespace detail {
void RegisterThermalModules() {
    ModuleFactory::Register("ModuleConductionMultiplier", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<ModuleConductionMultiplier>(c));
    });
    ModuleFactory::Register("ModuleAblator", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<ModuleAblator>(c));
    });
    ModuleFactory::Register("ModuleActiveRadiator", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<ModuleActiveRadiator>(c));
    });
    ModuleFactory::Register("ModuleCoreHeat", [](const ModuleClassInfo& c) {
        return std::static_pointer_cast<PartModule>(std::make_shared<ModuleCoreHeat>(c));
    });
}
}  // namespace detail

}  // namespace ksp
