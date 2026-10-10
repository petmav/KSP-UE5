// WP-17: DeltaVEngineInfo.cs (the per-engine isp / thrust / burn simulation record).
#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/resources/part_set.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/delta_v.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {

// FlightGlobals.GetHomeBody().atmDensityASL (null home body: 0, the editor tools that never set one)
float HomeAtmDensityASL() {
    return DeltaVGlobals::homeBody != nullptr ? f32store(DeltaVGlobals::homeBody->atmDensityASL) : 0.0f;
}

}  // namespace

// DeltaVEngineInfo.cs:56 partInfo
DeltaVPartInfo* DeltaVEngineInfo::partInfo() {
    if (partInfo_ == nullptr && vesselDeltaV != nullptr && engine != nullptr) partInfo_ = deltav_ext::Get(vesselDeltaV->PartInfo(), engine->part ? static_cast<Part*>(engine->part) : nullptr);
    return partInfo_;
}

// DeltaVEngineInfo.cs:68
DeltaVEngineInfo::DeltaVEngineInfo(VesselDeltaV* inVesselDeltaV, ModuleEngines* inEngine, MultiModeEngine* inMultiModeEngine) {
    vesselDeltaV = inVesselDeltaV;
    if (inMultiModeEngine != nullptr) {
        multiModeEngine = inMultiModeEngine;
        if (multiModeEngine->runningPrimary()) engine = multiModeEngine->PrimaryEngine();
        else engine = multiModeEngine->SecondaryEngine();
    } else {
        multiModeEngine = nullptr;
        engine = inEngine;
    }
    startBurnStage = static_cast<Part*>(engine->part)->fields.inverseStage;
    requiresAir = false;
    for (size_t i = 0; i < engine->propellants.size(); i++) {
        if (engine->propellants[i].name == "IntakeAir") requiresAir = true;
        propellantInfo.emplace_back(&engine->propellants[i]);
    }
    CalculateISP();
    CalculateThrustVector();
}

// DeltaVEngineInfo.cs:120
void DeltaVEngineInfo::Reset() {
    startBurnStage = static_cast<Part*>(engine->part)->fields.inverseStage;
    stageBurnTotals.clear();
    if (multiModeEngine != nullptr) {
        if (multiModeEngine->runningPrimary()) engine = multiModeEngine->PrimaryEngine();
        else engine = multiModeEngine->SecondaryEngine();
    }
    ispActual = 0.0;
    ispASL = 0.0;
    ispVac = 0.0;
    thrustASL = 0.0f;
    thrustActual = 0.0f;
    thrustVac = 0.0f;
    partInfo_ = nullptr;
    CalculateISP();
    CalculateThrustVector();
    deprived = false;
}

// DeltaVEngineInfo.cs:147
bool DeltaVEngineInfo::PropellantStarved() {
    bool result = false;
    for (size_t i = 0; i < propellantInfo.size(); i++) {
        double maxAmount = 0.0;
        DeltaVPropellantInfo& pi = propellantInfo[i];
        if (!DeltaVGlobals::PropellantIgnored(pi.propellant->id)) {
            static_cast<Part*>(engine->part)
                ->GetConnectedResourceTotals(pi.propellant->id, static_cast<ResourceFlowMode>(pi.propellant->GetFlowMode()), true, pi.amountAvailable, maxAmount);
            if (pi.amountAvailable <= 0.0) result = true;
        }
    }
    return result;
}

// DeltaVEngineInfo.cs:167
void DeltaVEngineInfo::CalculateBurn(DeltaVStageInfo*, std::vector<DeltaVEngineInfoPtr>& enginesStillActive, float deltaTime, bool runningActive,
                                     bool logMsgs, bool infiniteFuel, bool, bool& checkDeprived, float& minTimeStep) {
    (void)logMsgs;
    maxTimeStep = (minTimeStep = deltaTime);
    if (engine != nullptr && !engine->IsEngineDead()) {
        runningActive = runningActive && engine->EngineIgnited() && FlightRegistry::ActiveVessel != nullptr &&
                        FlightRegistry::ActiveVessel->ctrlState.mainThrottle > 0.0f;
        checkDeprived = false;
        double maxAmount = 0.0;
        const int count = static_cast<int>(propellantInfo.size());
        Part* enginePart = static_cast<Part*>(engine->part);
        for (int num = 0; num < count; num++) {
            DeltaVPropellantInfo& dp = propellantInfo[static_cast<size_t>(num)];
            dp.pendingDemand = 0.0;
            dp.timeLeftSetThrottle = 0.0f;
            dp.timeLeftCurrentThrottle = 0.0f;
            dp.amountPerSecondMaxThrottle = engine->getMaxFuelFlow(*dp.propellant);
            const float tp = engine->thrustPercentage();
            float throttle;
            if (engine->Get<bool>("throttleLocked")) {
                throttle = f32store(fx(tp) * fx(0.01f));
            } else if (engine->Get<bool>("independentThrottle")) {
                throttle = f32store(fx(engine->Get<float>("independentThrottlePercentage")) * fx(0.01f) * (fx(tp) * fx(0.01f)));
            } else {
                throttle = f32store(fx(tp) * fx(0.01f));
            }
            throttle = engine->ApplyThrottleAdjustments(throttle);
            dp.amountPerSecondSetThrottle = engine->getFuelFlow(*dp.propellant, f32store(fx(engine->maxFuelFlow) * fx(throttle)));
            dp.amountPerSecondCurrentThrottle = engine->getFuelFlow(*dp.propellant, engine->requestedMassFlow);
            enginePart->GetConnectedResourceTotals(dp.propellant->id, static_cast<ResourceFlowMode>(dp.propellant->GetFlowMode()), true, dp.amountAvailable, maxAmount);
            if (runningActive) dp.pendingDemand = dp.amountPerSecondCurrentThrottle * static_cast<double>(deltaTime);
            else dp.pendingDemand = dp.amountPerSecondSetThrottle * static_cast<double>(deltaTime);
            if (!(DeltaVGlobals::PropellantIgnored(dp.propellant->id) || infiniteFuel)) {
                dp.timeLeftSetThrottle = f32store(dp.amountAvailable / dp.amountPerSecondSetThrottle);
                dp.timeLeftCurrentThrottle = f32store(dp.amountAvailable / dp.amountPerSecondCurrentThrottle);
                const float num2 = f32store(dotnet::Math::Round(dp.amountAvailable, 5, dotnet::MidpointRounding::ToEven));
                const float num3 = f32store(dotnet::Math::Round(dp.pendingDemand, 5, dotnet::MidpointRounding::ToEven));
                if (!(num2 > 0.0f) || !(deltaTime <= 0.0f)) {
                    if (!(num2 < num3) && (!(num2 <= 0.0f) || deltaTime > 0.0f)) {
                        if (static_cast<double>(dotnet::Math::Abs(f32store(fx(num2) - fx(num3)))) < epsilon) {
                            // "Exactly Enough" (the C# `break` out of the loop)
                            lastStep = true;
                            checkDeprived = lastStep;
                            return;
                        }
                    } else {
                        lastStep = true;
                        if (num2 <= 0.0f && deltaTime <= 0.0f) deprived = true;
                    }
                }
            }
        }
        if (lastStep) {
            maxTimeStep = deltaTime;
            for (int i = 0; i < count; i++) {
                DeltaVPropellantInfo& dp = propellantInfo[static_cast<size_t>(i)];
                if (!DeltaVGlobals::PropellantIgnored(dp.propellant->id))
                    maxTimeStep = Mathf::Min(maxTimeStep, runningActive ? dp.timeLeftCurrentThrottle : dp.timeLeftSetThrottle);
            }
            for (int j = 0; j < count; j++) {
                DeltaVPropellantInfo& dp = propellantInfo[static_cast<size_t>(j)];
                if (!DeltaVGlobals::PropellantIgnored(dp.propellant->id) && enginesStillActive.size() <= 1) {
                    dp.pendingDemand = fx(runningActive ? f32store(dp.amountPerSecondCurrentThrottle) : f32store(dp.amountPerSecondSetThrottle)) * fx(maxTimeStep);
                }
            }
            minTimeStep = maxTimeStep;
        }
        checkDeprived = lastStep;
    } else {
        minTimeStep = 0.0f;
        checkDeprived = true;
        deprived = true;
    }
}

// DeltaVEngineInfo.cs:289
void DeltaVEngineInfo::ApplyBurn(DeltaVStageInfo*, float deltaTime, bool, bool, bool infiniteFuel) {
    const int count = static_cast<int>(propellantInfo.size());
    Part* enginePart = static_cast<Part*>(engine->part);
    for (int num = 0; num < count; num++) {
        DeltaVPropellantInfo& dp = propellantInfo[static_cast<size_t>(num)];
        if (DeltaVGlobals::PropellantIgnored(dp.propellant->id) || infiniteFuel) {
            dp.amountBurnt = dp.pendingDemand;
            dp.pendingDemand = 0.0;
        } else {
            const double num2 = enginePart->RequestResource(dp.propellant->id, dp.pendingDemand, static_cast<ResourceFlowMode>(dp.propellant->GetFlowMode()), true);
            dp.amountBurnt += num2;
            if (num2 <= 0.0 && deltaTime <= 0.0f && !lastStep) {
                dp.pendingDemand = 0.0;
            } else {
                if (!(dotnet::Math::Abs(num2 - dp.pendingDemand) <= epsilon)) {
                    deprived = true;
                    dp.pendingDemand = 0.0;
                    return;
                }
                dp.pendingDemand = 0.0;
            }
        }
    }
    if (lastStep && !deprived) deprived = true;
}

// DeltaVEngineInfo.cs:347
void DeltaVEngineInfo::ResetCalcVariables() {
    ResetResourceTotals();
    ResetStageInfo();
    ResetPropellantBurnValues();
}

// DeltaVEngineInfo.cs:354
void DeltaVEngineInfo::SwitchEngine() {
    if (multiModeEngine->runningPrimary()) engine = multiModeEngine->PrimaryEngine();
    else engine = multiModeEngine->SecondaryEngine();
    requiresAir = false;
    propellantInfo.clear();
    for (size_t i = 0; i < engine->propellants.size(); i++) {
        if (engine->propellants[i].name == "IntakeAir") requiresAir = true;
        propellantInfo.emplace_back(&engine->propellants[i]);
    }
    CalculateISP();
    CalculateThrustVector();
}

// DeltaVEngineInfo.cs:379
void DeltaVEngineInfo::CalculateFuelTime(int stage) {
    if (engine == nullptr) return;
    auto it = stageBurnTotals.find(stage);
    if (it != stageBurnTotals.end()) {
        it->second.propellantInfo.clear();
    } else {
        it = stageBurnTotals.emplace(stage, DeltaVEngineBurnTotals()).first;
    }
    DeltaVEngineBurnTotals& totals = it->second;
    DeltaVPartInfo* pi = partInfo();
    if (pi != nullptr && pi->decoupleBeforeBurn) return;
    totals.maxTotalBurnTime = -1.0;
    totals.setFuelFlowTotalBurnTime = -1.0;
    totals.currentFuelFlowTotalBurnTime = -1.0;
    Part* enginePart = static_cast<Part*>(engine->part);
    const int count = static_cast<int>(propellantInfo.size());
    for (int i = 0; i < count; i++) {
        DeltaVPropellantInfo& dp = propellantInfo[static_cast<size_t>(i)];
        float throttle = f32store(fx(engine->thrustPercentage()) * fx(0.01f));
        throttle = engine->ApplyThrottleAdjustments(throttle);
        if (engine->IsEngineDead()) {
            dp.amountPerSecondCurrentThrottle = 0.0;
            dp.amountPerSecondSetThrottle = 0.0;
            dp.amountPerSecondMaxThrottle = 0.0;
        } else {
            dp.amountPerSecondMaxThrottle = engine->getMaxFuelFlow(*dp.propellant);
            dp.amountPerSecondSetThrottle = engine->getFuelFlow(*dp.propellant, f32store(fx(engine->maxFuelFlow) * fx(throttle)));
            if (engine->requestedMassFlow > 0.0f) {
                dp.amountPerSecondCurrentThrottle = engine->getFuelFlow(*dp.propellant, engine->requestedMassFlow);
            } else if (DeltaVGlobals::LoadedSceneIsFlight() && vesselDeltaV->GetVessel() != nullptr) {
                dp.amountPerSecondCurrentThrottle = engine->getFuelFlow(
                    *dp.propellant, f32store(fx(engine->maxFuelFlow) * fx(f32store(fx(vesselDeltaV->GetVessel()->ctrlState.mainThrottle) * fx(throttle)))));
            } else {
                dp.amountPerSecondCurrentThrottle = 0.0;
            }
        }
        double maxAmount = 0.0;
        if (dp.propellant->name == "IntakeAir") {
            dp.currentBurnTime = 0.0;
            dp.maxBurnTime = 0.0;
            dp.setThrottleBurnTime = 0.0;
            dp.amountAvailable = 0.0;
            dp.pendingDemand = 0.0;
        } else {
            enginePart->GetConnectedResourceTotals(dp.propellant->id, static_cast<ResourceFlowMode>(dp.propellant->GetFlowMode()), true, dp.amountAvailable, maxAmount);
            dp.currentBurnTime = 0.0;
            dp.maxBurnTime = 0.0;
            dp.setThrottleBurnTime = 0.0;
            if (dp.amountPerSecondMaxThrottle != 0.0) dp.maxBurnTime = dp.amountAvailable / dp.amountPerSecondMaxThrottle;
            if (dp.amountPerSecondSetThrottle != 0.0) dp.setThrottleBurnTime = dp.amountAvailable / dp.amountPerSecondSetThrottle;
            if (dp.amountPerSecondCurrentThrottle != 0.0) dp.currentBurnTime = dp.amountAvailable / dp.amountPerSecondCurrentThrottle;
            if (totals.maxTotalBurnTime < 0.0) totals.maxTotalBurnTime = dp.maxBurnTime;
            else totals.maxTotalBurnTime = dotnet::Math::Min(totals.maxTotalBurnTime, dp.maxBurnTime);
            if (totals.setFuelFlowTotalBurnTime < 0.0) totals.setFuelFlowTotalBurnTime = dp.setThrottleBurnTime;
            else totals.setFuelFlowTotalBurnTime = dotnet::Math::Min(totals.setFuelFlowTotalBurnTime, dp.setThrottleBurnTime);
            if (totals.currentFuelFlowTotalBurnTime < 0.0) totals.currentFuelFlowTotalBurnTime = dp.currentBurnTime;
            else totals.currentFuelFlowTotalBurnTime = dotnet::Math::Min(totals.currentFuelFlowTotalBurnTime, dp.currentBurnTime);
        }
        totals.propellantInfo.push_back(dp.Copy());
    }
}

// DeltaVEngineInfo.cs:506
void DeltaVEngineInfo::ResetPropellantBurnValues() {
    for (size_t i = 0; i < propellantInfo.size(); i++) propellantInfo[i].amountBurnt = 0.0;
}

// DeltaVEngineInfo.cs:514
void DeltaVEngineInfo::ResetResourceTotals() {
    double maxAmount = 0.0;
    for (size_t i = 0; i < propellantInfo.size(); i++) {
        DeltaVPropellantInfo& dp = propellantInfo[i];
        dp.amountAvailable = 0.0;
        if (engine != nullptr && engine->part != nullptr)
            static_cast<Part*>(engine->part)->GetConnectedResourceTotals(dp.propellant->id, static_cast<ResourceFlowMode>(dp.propellant->GetFlowMode()), true,
                                                                         dp.amountAvailable, maxAmount);
    }
}

// DeltaVEngineInfo.cs:529
void DeltaVEngineInfo::ResetStageInfo() {
    maxTimeStep = 0.0f;
    lastStep = false;
}

// DeltaVEngineInfo.cs:535
void DeltaVEngineInfo::CalcThrustActual(bool engineIngitedCheck) {
    Part* enginePart = static_cast<Part*>(engine->part);
    if (DeltaVGlobals::LoadedSceneIsFlight() && enginePart != nullptr) {
        if (engine != nullptr && !engine->EngineIgnited() && engineIngitedCheck) {
            thrustActual = 0.0f;
            return;
        }
        if (engine != nullptr && engine->EngineIgnited() && engine->requestedMassFlow > 0.0f) {
            thrustActual = engine->finalThrust();
            return;
        }
        const float num = f32store(enginePart->fields.staticPressureAtm);
        if (num > 0.0f) thrustActual = engine->MaxThrustOutputAtm(false, true, num, enginePart->vessel->atmosphericTemperature, enginePart->vessel->atmDensity);
        else thrustActual = engine->MaxThrustOutputVac();
    } else if (DeltaVGlobals::LoadedSceneIsEditor() && enginePart != nullptr && DeltaVGlobals::DeltaVAppValuesPtr() != nullptr) {
        DeltaVAppValues* v = DeltaVGlobals::DeltaVAppValuesPtr();
        thrustActual = engine->MaxThrustOutputAtm(false, true, v->atmPressure(), 310.0, v->atmDensity());
    } else {
        thrustActual = engine->MaxThrustOutputAtm();
    }
}

// DeltaVEngineInfo.cs:569
float DeltaVEngineInfo::PropellantMassBurnt() const {
    float num = 0.0f;
    for (size_t i = 0; i < propellantInfo.size(); i++) num = f32store(fx(num) + fx(propellantInfo[i].MassBurnt()));
    return num;
}

// DeltaVEngineInfo.cs:579
double DeltaVEngineInfo::GetFuelTimeMaxThrottle(int stage) const {
    auto it = stageBurnTotals.find(stage);
    return it != stageBurnTotals.end() ? it->second.maxTotalBurnTime : 0.0;
}

// DeltaVEngineInfo.cs:588
double DeltaVEngineInfo::GetFuelTimeAtThrottle(int stage) const {
    auto it = stageBurnTotals.find(stage);
    return it != stageBurnTotals.end() ? it->second.setFuelFlowTotalBurnTime : 0.0;
}

// DeltaVEngineInfo.cs:597
double DeltaVEngineInfo::GetFuelTimeAtActiveThrottle(int stage) const {
    auto it = stageBurnTotals.find(stage);
    if (it != stageBurnTotals.end()) {
        if (!(it->second.currentFuelFlowTotalBurnTime <= 0.0) && !std::isinf(it->second.currentFuelFlowTotalBurnTime))
            return it->second.currentFuelFlowTotalBurnTime;
        return GetFuelTimeAtThrottle(stage);
    }
    return 0.0;
}

// DeltaVEngineInfo.cs:610
void DeltaVEngineInfo::CalculateISP(bool engineIngitedCheck) {
    const float time = 0.0f;
    float time2 = 1.0f;
    float time3 = HomeAtmDensityASL();
    Part* enginePart = static_cast<Part*>(engine->part);
    if (DeltaVGlobals::LoadedSceneIsFlight() && enginePart != nullptr) {
        time2 = f32store(enginePart->vessel->mainBody()->atmPressureASL);
        time3 = f32store(enginePart->vessel->mainBody()->atmDensityASL);
    } else if (DeltaVGlobals::LoadedSceneIsEditor() && DeltaVGlobals::DeltaVAppValuesPtr() != nullptr && DeltaVGlobals::DeltaVAppValuesPtr()->body != nullptr) {
        time2 = f32store(DeltaVGlobals::DeltaVAppValuesPtr()->body->atmPressureASL);
        time3 = f32store(DeltaVGlobals::DeltaVAppValuesPtr()->body->atmDensityASL);
    }
    const FloatCurve atmosphereCurve = engine->Get<FloatCurve>("atmosphereCurve");
    const bool useAtmCurveIsp = engine->Get<bool>("useAtmCurveIsp");
    const FloatCurve atmCurveIsp = engine->Get<FloatCurve>("atmCurveIsp");
    ispASL = atmosphereCurve.Evaluate(time2);
    if (useAtmCurveIsp) ispASL *= static_cast<double>(atmCurveIsp.Evaluate(time3));
    ispVac = atmosphereCurve.Evaluate(time);
    ispActual = ispVac;
    if (DeltaVGlobals::LoadedSceneIsFlight() && enginePart != nullptr) {
        if (engine != nullptr && !engine->EngineIgnited() && engineIngitedCheck) {
            ispActual = 0.0;
            return;
        }
        DeltaVPartInfo* pi = partInfo();
        if (pi != nullptr && pi->activationStage >= StageManager::CurrentStage() && engine->EngineIgnited() && engine->realIsp() > 0.0f) {
            ispActual = engine->realIsp();
            return;
        }
        const float num = f32store(enginePart->fields.staticPressureAtm);
        ispActual = atmosphereCurve.Evaluate(num);
        if (engine->Get<bool>("useThrottleIspCurve")) ispActual *= static_cast<double>(engine->GetThrottlingMult(num, engine->currentThrottle()));
        if (useAtmCurveIsp) ispActual *= static_cast<double>(atmCurveIsp.Evaluate(f32store(enginePart->fields.atmDensity * 0.8163265306122448)));
        if (engine->Get<bool>("useVelCurveIsp"))
            ispActual *= static_cast<double>(engine->Get<FloatCurve>("velCurveIsp").Evaluate(f32store(enginePart->fields.machNumber)));
    } else if (DeltaVGlobals::LoadedSceneIsEditor() && enginePart != nullptr && DeltaVGlobals::DeltaVAppValuesPtr() != nullptr) {
        DeltaVAppValues* v = DeltaVGlobals::DeltaVAppValuesPtr();
        ispActual = atmosphereCurve.Evaluate(v->atmPressure());
        if (useAtmCurveIsp) ispActual *= static_cast<double>(atmCurveIsp.Evaluate(f32store(v->atmDensity() * 0.8163265306122448)));
    }
}

// DeltaVEngineInfo.cs:687
void DeltaVEngineInfo::CalculateThrustVector(bool engineIngitedCheck) {
    Part* enginePart = static_cast<Part*>(engine->part);
    thrustVac = engine->MaxThrustOutputVac();
    if (DeltaVGlobals::LoadedSceneIsFlight() && enginePart != nullptr) {
        CelestialBody* mb = enginePart->vessel->mainBody();
        thrustASL = engine->MaxThrustOutputAtm(engineIngitedCheck, true, f32store(mb->atmPressureASL), mb->atmosphereTemperatureSeaLevel, mb->atmDensityASL);
    } else if (DeltaVGlobals::LoadedSceneIsEditor() && DeltaVGlobals::DeltaVAppValuesPtr() != nullptr && DeltaVGlobals::DeltaVAppValuesPtr()->body != nullptr) {
        const CelestialBody* b = DeltaVGlobals::DeltaVAppValuesPtr()->body;
        thrustASL = engine->MaxThrustOutputAtm(false, true, f32store(b->atmPressureASL), b->atmosphereTemperatureSeaLevel, b->atmDensityASL);
    } else {
        thrustASL = engine->MaxThrustOutputAtm();
    }
    CalcThrustActual(engineIngitedCheck);
    thrustVectorVac = Vector3f::zero();
    thrustVectorASL = Vector3f::zero();
    thrustVectorActual = Vector3f::zero();
    CenterOfThrustQuery centerOfThrustQuery;
    centerOfThrustQuery.Reset();
    engine->OnCenterOfThrustQuery(centerOfThrustQuery);
    thrustVectorVac = thrustVac * centerOfThrustQuery.dir;
    thrustVectorASL = thrustASL * centerOfThrustQuery.dir;
    thrustVectorActual = thrustActual * centerOfThrustQuery.dir;
}

// DeltaVEngineInfo.cs:750
bool DeltaVEngineInfo::RequiresAir() {
    for (size_t i = 0; i < propellantInfo.size(); i++) {
        if (propellantInfo[i].propellant->name == "IntakeAir") {
            requiresAir = true;
            return true;
        }
    }
    requiresAir = false;
    return false;
}

// DeltaVEngineInfo.cs:771
bool DeltaVEngineInfo::ThrottleLimited() {
    if (engine != nullptr && engine->thrustPercentage() < 100.0f) {
        throttleLimited = true;
        return true;
    }
    throttleLimited = false;
    return false;
}

// DeltaVEngineInfo.cs:782
double DeltaVEngineInfo::PropellantDemand(int id, bool runningActive) {
    double result = 0.0;
    runningActive = runningActive && engine->EngineIgnited() && FlightRegistry::ActiveVessel != nullptr && FlightRegistry::ActiveVessel->ctrlState.mainThrottle > 0.0f;
    for (size_t i = 0; i < propellantInfo.size(); i++) {
        if (propellantInfo[i].propellant->id == id) {
            result = !runningActive ? propellantInfo[i].amountPerSecondSetThrottle : propellantInfo[i].amountPerSecondCurrentThrottle;
            break;
        }
    }
    return result;
}

}  // namespace ksp
