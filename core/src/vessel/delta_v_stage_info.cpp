// WP-17: DeltaVStageInfo.cs (the per-stage burn simulation).
#include <algorithm>
#include <cmath>

#include "ksp/aero/aero_interfaces.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/modules/module_resource_intake.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/part_set.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/delta_v.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {

template <class T>
bool ListContains(const std::vector<std::shared_ptr<T>>& list, const T* item) {
    for (const auto& e : list)
        if (e.get() == item) return true;
    return false;
}

template <class T>
void ListRemove(std::vector<std::shared_ptr<T>>& list, const T* item) {
    for (size_t i = 0; i < list.size(); i++)
        if (list[i].get() == item) {
            list.erase(list.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
}

}  // namespace

// DeltaVStageInfo.cs:110
DeltaVStageInfo::DeltaVStageInfo(ShipConstruct* ship, int inStage, VesselDeltaV* v) : vesselDeltaV(v) {
    stage = inStage;
    if (ship != nullptr) ProcessParts(inStage);
}

// DeltaVStageInfo.cs:126
DeltaVStageInfo::DeltaVStageInfo(Vessel* vessel, int inStage, VesselDeltaV* v) : vesselDeltaV(v) {
    stage = inStage;
    if (vessel != nullptr) ProcessParts(inStage);
}

// DeltaVStageInfo.cs:177
void DeltaVStageInfo::Reset(int inStage, VesselDeltaV* v) {
    vesselDeltaV = v;
    stage = inStage;
    payloadStage = false;
    deltaVinVac = 0.0f;
    deltaVatASL = 0.0f;
    deltaVActual = 0.0f;
    stageMass = 0.0f;
    endMass = 0.0f;
    dryMass = 0.0f;
    fuelMass = 0.0f;
    decoupledMass = 0.0f;
    TWRActual = 0.0f;
    TWRASL = 0.0f;
    TWRVac = 0.0f;
    ispActual = 0.0;
    ispASL = 0.0;
    ispVac = 0.0;
    thrustActual = 0.0f;
    thrustASL = 0.0f;
    thrustVac = 0.0f;
    stageBurnTime = 0.0;
    vectoredThrustVac = 0.0f;
    vectoredThrustASL = 0.0f;
    vectoredThrustActual = 0.0f;
    totalExhaustVelocityVAC = 0.0f;
    totalExhaustVelocityASL = 0.0f;
    totalExhaustVelocityActual = 0.0f;
    ProcessParts(inStage);
}

// DeltaVStageInfo.cs:230
void DeltaVStageInfo::ProcessParts(int inStage) {
    stageContainsOnlyLaunchClamps = false;
    airIntakeParts.clear();
    bool flag = false;
    int count = static_cast<int>(parts.size());
    while (count-- > 0) {
        bool flag2 = false;
        if (vesselDeltaV->ActiveMode() == VesselDeltaV::Mode::Ship && vesselDeltaV->Ship() != nullptr) {
            const auto& shipParts = vesselDeltaV->Ship()->liveParts;
            for (size_t i = 0; i < shipParts.size(); i++) {
                if (shipParts[i]->fields.persistentId == parts[static_cast<size_t>(count)]->part->fields.persistentId) {
                    flag2 = true;
                    break;
                }
            }
        } else if (vesselDeltaV->ActiveMode() == VesselDeltaV::Mode::Vessel) {
            const auto& vparts = vesselDeltaV->GetVessel()->parts;
            for (size_t j = 0; j < vparts.size(); j++) {
                if (vparts[j]->fields.persistentId == parts[static_cast<size_t>(count)]->part->fields.persistentId) {
                    flag2 = true;
                    break;
                }
            }
        }
        if (!flag2) {
            partInfoDictionary.erase(parts[static_cast<size_t>(count)]->part->fields.persistentId);
            parts.erase(parts.begin() + count);
            flag = true;
        }
    }
    std::vector<DeltaVPartInfoPtr>& infos = vesselDeltaV->PartInfo();
    for (size_t k = 0; k < infos.size(); k++) {
        DeltaVPartInfoPtr deltaVPartInfo = infos[k];
        if (deltaVPartInfo->decoupleStage <= inStage) {
            if (partInfoDictionary.find(deltaVPartInfo->part->fields.persistentId) == partInfoDictionary.end()) {
                partInfoDictionary.emplace(deltaVPartInfo->part->fields.persistentId, deltaVPartInfo);
                parts.push_back(deltaVPartInfo);
                flag = true;
            }
            if (deltaVPartInfo->isIntake && deltaVPartInfo->moduleResourceIntake != nullptr) airIntakeParts.push_back(deltaVPartInfo->moduleResourceIntake);
            separationIndex = dotnet::Math::Max(separationIndex, deltaVPartInfo->decoupleStage);
        } else if (partInfoDictionary.find(infos[k]->part->fields.persistentId) != partInfoDictionary.end()) {
            partInfoDictionary.erase(infos[k]->part->fields.persistentId);
            ListRemove(parts, infos[k].get());
            flag = true;
        }
    }
    ProcessActiveEngines();
    if (parts.empty()) separationIndex = vesselDeltaV->GetHighestSeparationStage(inStage);
    if (flag) ResetPartCaches();
    PartsActivateInStage(cachedActivatedParts);
    bool flag3 = true;
    for (size_t l = 0; l < cachedActivatedParts.size(); l++) {
        if (!cachedActivatedParts[l]->part->isLaunchClamp()) {
            flag3 = false;
            break;
        }
    }
    stageContainsOnlyLaunchClamps = flag3;
}

// DeltaVStageInfo.cs:318
void DeltaVStageInfo::CalculateStartMass() {
    float decoupledDryMass = 0.0f;
    float decoupledFuelMass = 0.0f;
    float jettisonedDryMass = 0.0f;
    int decoupledPartCount = 0;
    stageMass = GetStageMass(dryMass, fuelMass, decoupledDryMass, decoupledFuelMass, jettisonedDryMass, decoupledPartCount);
    decoupledMass = f32store(fx(decoupledDryMass) + fx(decoupledFuelMass) + fx(jettisonedDryMass));
    dryMass = f32store(fx(dryMass) - fx(decoupledDryMass) - fx(jettisonedDryMass));
    startMass = f32store(fx(stageMass) - fx(decoupledMass));
    StoreStartFuelMass();
}

// DeltaVStageInfo.cs:331
void DeltaVStageInfo::StoreInterimDeltaV(std::vector<DeltaVEngineInfoPtr>& engines, std::vector<DeltaVCalc>& deltaVCalcsList, float startingMass,
                                         float currentMass, double simulationTime, bool) {
    enginesThrustVac = Vector3f::zero();
    enginesThrustASL = Vector3f::zero();
    enginesThrustActual = Vector3f::zero();
    for (size_t i = 0; i < engines.size(); i++) {
        engines[i]->CalculateThrustVector(stage == StageManager::CurrentStage());
        enginesThrustVac += engines[i]->thrustVectorVac;
        enginesThrustASL += engines[i]->thrustVectorASL;
        enginesThrustActual += engines[i]->thrustVectorActual;
    }
    const double g = PhysicsGlobals::GravitationalAcceleration;
    const double logRatio = dotnet::Math::Log(fx(startingMass) / fx(currentMass));
    Vector3f vector = enginesThrustVac * f32store(logRatio * ispVac * g / static_cast<double>(thrustVac));
    Vector3f vector2 = enginesThrustASL * f32store(dotnet::Math::Log(fx(startingMass) / fx(currentMass)) * ispASL * g / static_cast<double>(thrustASL));
    Vector3f vector3 = enginesThrustActual * f32store(dotnet::Math::Log(fx(startingMass) / fx(currentMass)) * ispActual * g / static_cast<double>(thrustActual));
    CalculateTWR(startingMass);
    DeltaVCalc c;
    c.dVinVac = vector.magnitude();
    c.dVatASL = vector2.magnitude();
    c.dVActual = vector3.magnitude();
    c.time = simulationTime;
    c.activeEngines = engines;
    c.ispVAC = ispVac;
    c.ispASL = ispASL;
    c.ispActual = ispActual;
    c.TWRVac = TWRVac;
    c.TWRASL = TWRASL;
    c.TWRActual = TWRActual;
    c.startMass = startingMass;
    c.endMass = currentMass;
    c.thrustVac = thrustVac;
    c.thrustASL = thrustASL;
    c.thrustActual = thrustActual;
    deltaVCalcsList.push_back(std::move(c));
}

// DeltaVStageInfo.cs:354
int DeltaVStageInfo::GetHighestSeparationIndex(std::vector<DeltaVEngineInfoPtr>& engines) {
    int num = INT32_MIN;
    for (size_t i = 0; i < engines.size(); i++) num = dotnet::Math::Max(num, engines[i]->partInfo()->decoupleStage);
    return num;
}

// DeltaVStageInfo.cs:364
std::vector<DeltaVEngineInfoPtr> DeltaVStageInfo::MatchingSeparationIndex(std::vector<DeltaVEngineInfoPtr>& engines, int index) {
    std::vector<DeltaVEngineInfoPtr> list;
    for (size_t i = 0; i < engines.size(); i++)
        if (engines[i]->partInfo()->decoupleStage == index) list.push_back(engines[i]);
    if (list.empty()) return engines;
    return list;
}

// DeltaVStageInfo.cs:381
std::vector<DeltaVEngineResourcePartInfo> DeltaVStageInfo::GetEngineResourceParts(std::vector<DeltaVEngineInfoPtr>& engines) {
    std::vector<DeltaVEngineResourcePartInfo> list;
    for (size_t i = 0; i < engines.size(); i++) {
        PartHashSetPtr set = engines[i]->partInfo()->part->simulationCrossfeedPartSet->GetParts();
        for (auto it = set->begin(); it != set->end(); ++it) {
            Part* current = *it;
            auto found = partInfoDictionary.find(current->fields.persistentId);
            if (found == partInfoDictionary.end()) continue;
            DeltaVPartInfoPtr value = found->second;
            if (value->decoupleStage == stage) {
                for (int j = 0; j < value->part->SimulationResources().Count(); j++) value->part->SimulationResources()[static_cast<size_t>(j)].amount = 0.0;
                continue;
            }
            for (size_t k = 0; k < engines[i]->propellantInfo.size(); k++) {
                if (value->part->Resources.Contains(engines[i]->propellantInfo[k].propellant->id)) {
                    DeltaVEngineResourcePartInfo* e = deltav_ext::Get(list, *value);
                    if (e == nullptr) {
                        list.emplace_back(value);
                        e = &list.back();
                    }
                    const int pid = engines[i]->propellantInfo[k].propellant->id;
                    if (std::find(e->resourceIdsUsed.begin(), e->resourceIdsUsed.end(), pid) == e->resourceIdsUsed.end()) e->resourceIdsUsed.push_back(pid);
                }
            }
        }
    }
    return list;
}

// DeltaVStageInfo.cs:420
void DeltaVStageInfo::CalculateEngineResourceFuelMass(std::vector<DeltaVEngineResourcePartInfo>& resourceParts) {
    for (size_t i = 0; i < resourceParts.size(); i++) {
        DeltaVEngineResourcePartInfo& rp = resourceParts[i];
        for (size_t j = 0; j < rp.resourceIdsUsed.size(); j++) {
            PartResource* partResource = rp.resourcePart->part->SimulationResources().Get(rp.resourceIdsUsed[j]);
            if (partResource != nullptr) rp.fuelMassUsed = f32store(fx(rp.fuelMassUsed) + fx(partResource->info->density()) * fx(f32store(partResource->amount)));
        }
    }
}

// DeltaVStageInfo.cs:436
bool DeltaVStageInfo::EnginesDeprived(std::vector<DeltaVEngineInfoPtr>& enginesToStageNext, bool allEngines) {
    int num = 0;
    if (enginesToStageNext.empty()) return true;
    for (size_t i = 0; i < enginesToStageNext.size(); i++) {
        if (enginesToStageNext[i]->deprived) {
            num++;
            if (!allEngines) return true;
        }
    }
    if (allEngines && num == static_cast<int>(enginesToStageNext.size())) return true;
    return false;
}

// DeltaVStageInfo.cs:468
void DeltaVStageInfo::StoreStartFuelMass() {
    for (size_t i = 0; i < parts.size(); i++) parts[i]->StageStartFuelMass(stage);
}

// DeltaVStageInfo.cs:476
void DeltaVStageInfo::StoreEndFuelMass() {
    for (size_t i = 0; i < parts.size(); i++) parts[i]->StageEndFuelMass(stage);
}

// DeltaVStageInfo.cs:484
float DeltaVStageInfo::CheckTimeStep(float timeStep, float simulationBurnTime, float fuelMassRemoved, std::vector<DeltaVEngineInfoPtr>& enginesStillActive,
                                     std::vector<DeltaVEngineResourcePartInfo>& resourcePartsToStageNext, bool) {
    float num = 0.0f;
    float num2 = 0.0f;
    if (resourcePartsToStageNext.empty()) return timeStep;
    for (size_t i = 0; i < resourcePartsToStageNext.size(); i++) num = f32store(fx(num) + fx(resourcePartsToStageNext[i].fuelMassUsed));
    for (size_t j = 0; j < enginesStillActive.size(); j++) {
        bool flag = false;
        for (size_t k = 0; k < resourcePartsToStageNext.size(); k++) {
            if (enginesStillActive[j]->partInfo()->part->simulationCrossfeedPartSet->ContainsPart(resourcePartsToStageNext[k].resourcePart->part)) {
                flag = true;
                break;
            }
        }
        if (!(enginesStillActive[j]->maxTimeStep <= 0.0f) && flag) {
            float throttle = f32store(fx(enginesStillActive[j]->engine->thrustPercentage()) * fx(0.01f));
            throttle = enginesStillActive[j]->engine->ApplyThrottleAdjustments(throttle);
            num2 = f32store(fx(num2) + fx(f32store(static_cast<double>(enginesStillActive[j]->thrustActual) / enginesStillActive[j]->ispActual /
                                                  PhysicsGlobals::GravitationalAcceleration * static_cast<double>(throttle))));
        }
    }
    const float num3 = f32store((fx(fuelMassRemoved) + fx(num)) / fx(num2));
    const float num4 = f32store(fx(num3) - fx(simulationBurnTime));
    if (num4 >= timeStep) return timeStep;
    return num4;
}

// DeltaVStageInfo.cs:531
void DeltaVStageInfo::SimulateDeltaV(bool runningActive, bool infiniteFuel, float timeStep, bool logMsgs, bool thisStageActive) {
    CalculateStartMass();
    if (!enginesActiveInStage.empty() && (thisStageActive || !stageContainsOnlyLaunchClamps)) {
        deltaVCalcs.clear();
        if (!cachedResourcePartSetValid_) {
            cachedResourcePartSetParts = deltav_ext::PartsInStage(parts, stage);
            cachedResourcePartSetValid_ = true;
            cachedFlowGraph = std::make_unique<RUI::Algorithms::SCCFlowGraph>(cachedResourcePartSetParts);
            PartSet::BuildPartSimulationSets(cachedResourcePartSetParts, cachedFlowGraph.get());
        }
        if (DeltaVGlobals::LoadedSceneIsEditor() && vesselDeltaV != nullptr && vesselDeltaV->Ship() != nullptr)
            vesselDeltaV->Ship()->UpdateResourceSets(&cachedResourcePartSetParts, cachedFlowGraph.get());
        int count = static_cast<int>(enginesActiveInStage.size());
        while (count-- > 0) {
            DeltaVEngineInfoPtr& ea = enginesActiveInStage[static_cast<size_t>(count)];
            if (!ea->deprived && !ea->PropellantStarved()) {
                if (ea->partInfo()->decoupleBeforeBurn) {
                    enginesActiveInStage.erase(enginesActiveInStage.begin() + count);
                } else if (DeltaVGlobals::LoadedSceneIsFlight() && StageManager::CurrentStage() == stage && ea->RequiresAir() &&
                           static_cast<Part*>(ea->engine->part)->fields.staticPressureAtm <= 0.0) {
                    enginesActiveInStage.erase(enginesActiveInStage.begin() + count);
                }
            } else {
                enginesActiveInStage.erase(enginesActiveInStage.begin() + count);
            }
        }
        if (payloadStage && !enginesActiveInStage.empty()) payloadStage = false;
        int highestSeparationIndex = GetHighestSeparationIndex(enginesActiveInStage);
        std::vector<DeltaVEngineInfoPtr> list = MatchingSeparationIndex(enginesActiveInStage, highestSeparationIndex);
        std::vector<DeltaVEngineResourcePartInfo> engineResourceParts = GetEngineResourceParts(list);
        CalculateEngineResourceFuelMass(engineResourceParts);
        int highestPartSeparationIndex = deltav_ext::GetHighestPartSeparationIndex(engineResourceParts, stage);
        std::vector<DeltaVEngineResourcePartInfo> list2 = deltav_ext::PartsMatchingSeparationIndex(engineResourceParts, highestPartSeparationIndex);
        for (size_t k = 0; k < enginesActiveInStage.size(); k++) {
            enginesActiveInStage[k]->ResetCalcVariables();
            enginesActiveInStage[k]->CalculateFuelTime(stage);
            if (enginesActiveInStage[k]->requiresAir) timeStep = DeltaVGameSettings::DELTAV_CALCULATIONS_BIGTIMESTEP;
        }
        std::vector<DeltaVEngineInfoPtr> list3(enginesActiveInStage);
        CalculateISP();
        CalculateTWR();
        float startingMass = startMass;
        float num = 0.0f;
        removedFuelMass = 0.0f;
        std::vector<DeltaVEngineInfoPtr> list4;
        do {
            bool flag = false;
            bool flag2 = false;
            float num2 = timeStep;
            for (size_t l = 0; l < list3.size(); l++) {
                if (!list3[l]->deprived) {
                    bool checkDeprived = false;
                    float minTimeStep = timeStep;
                    list3[l]->CalculateBurn(this, list3, timeStep, runningActive, logMsgs, infiniteFuel, false, checkDeprived, minTimeStep);
                    flag = flag || checkDeprived;
                    if (flag) num2 = Mathf::Min(num2, minTimeStep);
                }
            }
            if (!list3.empty() || list2.size() > 1) {
                const float num3 = CheckTimeStep(timeStep, num, removedFuelMass, list3, list2, logMsgs);
                if (num3 < num2) {
                    flag = true;
                    num2 = num3;
                }
                if (num2 <= 0.0f) {
                    for (size_t m = 0; m < list.size(); m++) {
                        list[m]->deprived = true;
                        list4.push_back(list[m]);
                    }
                }
            }
            if (flag && !list3.empty()) {
                for (size_t n = 0; n < list3.size(); n++) {
                    if (!list3[n]->deprived) {
                        bool checkDeprived2 = false;
                        float minTimeStep2 = num2;
                        list3[n]->CalculateBurn(this, list3, num2, runningActive, logMsgs, infiniteFuel, true, checkDeprived2, minTimeStep2);
                    }
                }
            }
            if (!EnginesDeprived(list, true)) {
                int count2 = static_cast<int>(list3.size());
                while (count2-- > 0) {
                    if (list3[static_cast<size_t>(count2)]->deprived) {
                        list4.push_back(list3[static_cast<size_t>(count2)]);
                        continue;
                    }
                    list3[static_cast<size_t>(count2)]->ApplyBurn(this, num2, runningActive, logMsgs, infiniteFuel);
                    if (list3[static_cast<size_t>(count2)]->deprived) list4.push_back(list3[static_cast<size_t>(count2)]);
                }
            }
            if (!list2.empty()) {
                int count3 = static_cast<int>(list2.size());
                while (count3-- > 0) {
                    bool flag3 = false;
                    DeltaVEngineResourcePartInfo& rp = list2[static_cast<size_t>(count3)];
                    for (int num4 = 0; num4 < rp.resourcePart->part->SimulationResources().Count(); num4++) {
                        PartResource& sr = rp.resourcePart->part->SimulationResources()[static_cast<size_t>(num4)];
                        flag3 = (std::find(rp.resourceIdsUsed.begin(), rp.resourceIdsUsed.end(), sr.info->id()) != rp.resourceIdsUsed.end() && sr.amount <= 0.0) ? true : false;
                    }
                    if (flag3) {
                        removedFuelMass = f32store(fx(removedFuelMass) + fx(rp.fuelMassUsed));
                        list2.erase(list2.begin() + count3);
                    }
                }
                if (list2.empty()) flag2 = true;
            }
            if (!list4.empty() || num2 < timeStep || flag2) {
                CalculateISP(&list3);
                const float currentStageMass = GetCurrentStageMass();
                StoreInterimDeltaV(list3, deltaVCalcs, startingMass, currentStageMass, fx(num) + fx(num2), logMsgs);
                startingMass = currentStageMass;
                for (size_t num5 = 0; num5 < list4.size(); num5++) ListRemove(list3, list4[num5].get());
                list4.clear();
            }
            if (EnginesDeprived(list, true) || flag2) list3.clear();
            num = f32store(fx(num) + fx(num2));
            if (num > 1000.0f && timeStep < DeltaVGameSettings::DELTAV_CALCULATIONS_BIGTIMESTEP) timeStep = DeltaVGameSettings::DELTAV_CALCULATIONS_BIGTIMESTEP;
        } while (!list3.empty() && num < 100000.0f);
        for (size_t num6 = 0; num6 < list.size(); num6++)
            if (list[num6]->partInfo()->decoupleStage < stage) list[num6]->deprived = false;
        stageBurnTime = num;
        endMass = GetCurrentStageMass();
        StoreEndFuelMass();
        CalculateISP();
        CalculateTWR();
        deltaVinVac = 0.0f;
        deltaVatASL = 0.0f;
        deltaVActual = 0.0f;
        for (size_t num7 = 0; num7 < deltaVCalcs.size(); num7++) {
            if (!std::isnan(deltaVCalcs[num7].dVinVac)) deltaVinVac = f32store(fx(deltaVinVac) + fx(f32store(deltaVCalcs[num7].dVinVac)));
            if (!std::isnan(deltaVCalcs[num7].dVatASL)) deltaVatASL = f32store(fx(deltaVatASL) + fx(f32store(deltaVCalcs[num7].dVatASL)));
            if (!std::isnan(deltaVCalcs[num7].dVActual)) deltaVActual = f32store(fx(deltaVActual) + fx(f32store(deltaVCalcs[num7].dVActual)));
        }
        totalExhaustVelocityActual = 0.0f;
        totalExhaustVelocityASL = 0.0f;
        totalExhaustVelocityVAC = 0.0f;
        vectoredExhaustVelocityVAC = vectoredExhaustVelocityASL = vectoredExhaustVelocityActual = Vector3f::zero();
        vectoredThrustActual = 0.0f;
        vectoredThrustASL = 0.0f;
        vectoredThrustVac = 0.0f;
        enginesThrustVac = enginesThrustASL = enginesThrustActual = Vector3f::zero();
        const float gF = f32store(PhysicsGlobals::GravitationalAcceleration);
        for (size_t e = 0; e < enginesActiveInStage.size(); e++) {
            DeltaVEngineInfo& ei = *enginesActiveInStage[e];
            if (ei.thrustVac > 0.0f) {
                const Vector3f v = ei.thrustVectorVac * f32store(fx(f32store(ei.ispVac)) * fx(gF)) / ei.thrustVac;
                totalExhaustVelocityVAC = f32store(fx(totalExhaustVelocityVAC) + fx(v.magnitude()));
                vectoredExhaustVelocityVAC += v;
            }
            if (ei.thrustASL > 0.0f) vectoredExhaustVelocityASL += ei.thrustVectorASL * f32store(fx(f32store(ei.ispASL)) * fx(gF)) / ei.thrustASL;
            if (ei.thrustActual > 0.0f) vectoredExhaustVelocityActual += ei.thrustVectorActual * f32store(fx(f32store(ei.ispActual)) * fx(gF)) / ei.thrustActual;
            enginesThrustVac += ei.thrustVectorVac;
            enginesThrustASL += ei.thrustVectorASL;
            enginesThrustActual += ei.thrustVectorActual;
        }
        vectoredThrustVac = enginesThrustVac.magnitude();
        vectoredThrustASL = enginesThrustASL.magnitude();
        vectoredThrustActual = enginesThrustActual.magnitude();
        return;
    }
    endMass = startMass;
    StoreEndFuelMass();
}

// DeltaVStageInfo.cs:871 (the time to burn `deltaVRequested`; the debug log is dropped)
double DeltaVStageInfo::CalculateTimeRequiredDV(bool runningActive, float deltaVRequested) {
    Vessel* active = FlightRegistry::ActiveVessel;
    const float num2 = runningActive ? vectoredThrustActual : (active == nullptr ? vectoredThrustVac : (active->atmDensity > 0.0 ? vectoredThrustASL : vectoredThrustVac));
    const float num3 = runningActive ? static_cast<float>(ispActual) : (active == nullptr ? static_cast<float>(ispVac) : (active->atmDensity > 0.0 ? static_cast<float>(ispASL) : static_cast<float>(ispVac)));
    const float num4 = runningActive ? vectoredExhaustVelocityActual.magnitude()
                                     : (active == nullptr ? vectoredExhaustVelocityVAC.magnitude()
                                                          : (active->atmDensity > 0.0 ? vectoredExhaustVelocityASL.magnitude() : vectoredExhaustVelocityVAC.magnitude()));
    const float num5 = runningActive ? totalExhaustVelocityActual : (active == nullptr ? totalExhaustVelocityVAC : (active->atmDensity > 0.0 ? totalExhaustVelocityASL : totalExhaustVelocityVAC));
    float num6 = 1.0f;
    if (active != nullptr && num5 > 0.0f) num6 = f32store(fx(num4) / fx(num5));
    const float gF = f32store(PhysicsGlobals::GravitationalAcceleration);
    const float num7 = f32store(fx(startMass) / fx(Mathf::Exp(f32store(fx(deltaVRequested) / fx(gF) / fx(num3) / fx(num6)))));
    const float num8 = f32store(fx(num2) / fx(startMass));
    const float num9 = f32store(fx(num2) / fx(num7));
    return fx(f32store(fx(deltaVRequested) / fx(Mathf::Sqrt(f32store(fx(num8) * fx(num9))))));
}

// DeltaVStageInfo.cs:895
void DeltaVStageInfo::CalcLerpDeltaV() {
    for (size_t i = 0; i < enginesActiveInStage.size(); i++) {
        enginesActiveInStage[i]->CalcThrustActual();
        enginesActiveInStage[i]->CalculateISP();
    }
    CalculateISP();
    CalculateTWR();
    if (vesselDeltaV->GetVessel() != nullptr) {
        float num = f32store(vesselDeltaV->GetVessel()->rootPart->fields.staticPressureAtm);
        if (num > 0.0f) num = f32store(fx(num) / fx(f32store(vesselDeltaV->GetVessel()->mainBody()->atmPressureASL)));
        deltaVActual = Mathf::Lerp(deltaVinVac, deltaVatASL, num);
    }
}

// DeltaVStageInfo.cs:961
void DeltaVStageInfo::ProcessActiveEngines() {
    enginesActiveInStage.clear();
    enginesInStage.clear();
    std::vector<DeltaVEngineInfoPtr>& workingEngineInfo = vesselDeltaV->WorkingEngineInfo();
    for (size_t i = 0; i < workingEngineInfo.size(); i++) {
        if (workingEngineInfo[i]->partInfo() == nullptr) {
            vesselDeltaV->SetCalcsDirty(true);
        } else if (stage <= workingEngineInfo[i]->startBurnStage) {
            enginesInStage.push_back(workingEngineInfo[i]);
            if (partInfoDictionary.find(static_cast<Part*>(workingEngineInfo[i]->engine->part)->fields.persistentId) != partInfoDictionary.end() &&
                workingEngineInfo[i]->partInfo()->decoupleStage < stage &&
                (!DeltaVGlobals::LoadedSceneIsFlight() || StageManager::CurrentStage() != stage || workingEngineInfo[i]->engine->EngineIgnited())) {
                enginesActiveInStage.push_back(workingEngineInfo[i]);
            }
        }
    }
}

// DeltaVStageInfo.cs:983
void DeltaVStageInfo::ResetPartCaches() {
    cachedResourcePartSetValid_ = false;
    cachedResourcePartSetParts.clear();
    cachedFlowGraph.reset();
}

// DeltaVStageInfo.cs:989
float DeltaVStageInfo::GetStageMass(float& dryMass_, float& fuelMass_, float& decoupledDryMass, float& decoupledFuelMass, float& jettisonedDryMass,
                                    int& decoupledPartCount) {
    dryMass_ = 0.0f;
    fuelMass_ = 0.0f;
    decoupledDryMass = 0.0f;
    decoupledFuelMass = 0.0f;
    jettisonedDryMass = 0.0f;
    decoupledPartCount = 0;
    int count = static_cast<int>(parts.size());
    while (count-- > 0) {
        DeltaVPartInfo& p = *parts[static_cast<size_t>(count)];
        dryMass_ = f32store(fx(dryMass_) + fx(p.dryMass));
        fuelMass_ = f32store(fx(fuelMass_) + fx(p.GetCurrentFuelMass()));
        if (p.decoupleStage == stage) {
            decoupledPartCount++;
            decoupledDryMass = f32store(fx(decoupledDryMass) + fx(p.dryMass));
            decoupledFuelMass = f32store(fx(decoupledFuelMass) + fx(p.GetCurrentFuelMass()));
        }
        if (p.JettisonInStage(stage)) jettisonedDryMass = f32store(fx(jettisonedDryMass) + fx(p.jettisonMass));
    }
    return f32store(fx(dryMass_) + fx(fuelMass_));
}

// DeltaVStageInfo.cs:1017
float DeltaVStageInfo::GetCurrentStageMass() {
    float num = startMass;
    float num2 = 0.0f;
    for (size_t i = 0; i < enginesActiveInStage.size(); i++) num2 = f32store(fx(num2) + fx(enginesActiveInStage[i]->PropellantMassBurnt()));
    num = f32store(fx(num) - fx(num2));
    return num;
}

// DeltaVStageInfo.cs:1033
bool DeltaVStageInfo::ContainsAnchoredDecoupler() const {
    for (size_t i = 0; i < parts.size(); i++)
        if (parts[i]->moduleAnchoredDecoupler != nullptr) return true;
    return false;
}

// DeltaVStageInfo.cs:1052
bool DeltaVStageInfo::ContainsDecoupler() const {
    for (size_t i = 0; i < parts.size(); i++)
        if (parts[i]->isDecoupler) return true;
    return false;
}

// DeltaVStageInfo.cs:1099
void DeltaVStageInfo::CalculateISP(std::vector<DeltaVEngineInfoPtr>* enginesIn) {
    bool flag = true;
    ispVac = 0.0;
    ispASL = 0.0;
    ispActual = 0.0;
    thrustVac = 0.0f;
    thrustASL = 0.0f;
    thrustActual = 0.0f;
    std::vector<DeltaVEngineInfoPtr>& engines = enginesIn == nullptr ? enginesActiveInStage : *enginesIn;
    for (size_t i = 0; i < engines.size(); i++) {
        DeltaVEngineInfo& e = *engines[i];
        e.CalculateThrustVector(stage == StageManager::CurrentStage());
        thrustVac = f32store(fx(thrustVac) + fx(e.thrustVac));
        thrustASL = f32store(fx(thrustASL) + fx(e.thrustASL));
        thrustActual = f32store(fx(thrustActual) + fx(e.thrustActual));
        e.CalculateISP(stage == StageManager::CurrentStage());
        if (e.ispVac != ispVac) {
            if (ispVac == 0.0) {
                ispVac = e.ispVac;
                ispASL = e.ispASL;
                ispActual = e.ispActual;
            } else {
                flag = false;
            }
        }
    }
    if (!flag) {
        double num = 0.0, num2 = 0.0, num3 = 0.0;
        for (size_t j = 0; j < engines.size(); j++) {
            DeltaVEngineInfo& e2 = *engines[j];
            num += static_cast<double>(e2.thrustVac) / e2.ispVac;
            num2 += static_cast<double>(e2.thrustASL) / e2.ispASL;
            num3 += static_cast<double>(e2.thrustActual) / e2.ispActual;
        }
        ispVac = static_cast<double>(thrustVac) / num;
        ispASL = static_cast<double>(thrustASL) / num2;
        ispActual = static_cast<double>(thrustActual) / num3;
    }
}

// DeltaVStageInfo.cs:1152
void DeltaVStageInfo::CalculateTWR(float mass) {
    float num = f32store(DeltaVGlobals::homeBody != nullptr ? DeltaVGlobals::homeBody->GeeASL : 0.0);
    if (DeltaVGlobals::LoadedSceneIsFlight()) {
        if (vesselDeltaV != nullptr && vesselDeltaV->GetVessel() != nullptr && vesselDeltaV->GetVessel()->mainBody() != nullptr)
            num = f32store(vesselDeltaV->GetVessel()->mainBody()->GeeASL);
    } else if (DeltaVGlobals::LoadedSceneIsEditor()) {
        DeltaVAppValues* v = DeltaVGlobals::DeltaVAppValuesPtr();
        if (v != nullptr && v->body != nullptr) num = f32store(v->body->GeeASL);
    }
    const float gF = f32store(PhysicsGlobals::GravitationalAcceleration);
    TWRVac = f32store(fx(thrustVac) / (fx(startMass) * (fx(gF) * fx(num))));
    TWRASL = f32store(fx(thrustASL) / (fx(startMass) * (fx(gF) * fx(num))));
    TWRActual = f32store(fx(thrustActual) / (fx(mass == -1.0f ? startMass : mass) * (fx(gF) * fx(num))));
}

// DeltaVStageInfo.cs:1175
int DeltaVStageInfo::PartsActiveInStage() const {
    int num = 0;
    for (size_t i = 0; i < parts.size(); i++)
        if (parts[i]->decoupleStage != stage) num++;
    return num;
}

// DeltaVStageInfo.cs:1188
int DeltaVStageInfo::PartsActivateInStage(std::vector<DeltaVPartInfoPtr>& activeParts) {
    int num = 0;
    activeParts.clear();
    for (size_t i = 0; i < parts.size(); i++) {
        if (parts[i]->activationStage == stage) {
            num++;
            activeParts.push_back(parts[i]);
        } else if (parts[i]->isEngine && parts[i]->activationStage > stage) {
            DeltaVEngineInfo* e = deltav_ext::Get(enginesActiveInStage, parts[i]->part);
            if (e != nullptr && !e->deprived) {
                num++;
                activeParts.push_back(parts[i]);
            }
        }
    }
    return num;
}

// DeltaVStageInfo.cs:1212
int DeltaVStageInfo::PartsDecoupledInStage() const {
    int num = 0;
    for (size_t i = 0; i < parts.size(); i++)
        if (parts[i]->decoupleStage == stage) num++;
    return num;
}

}  // namespace ksp
