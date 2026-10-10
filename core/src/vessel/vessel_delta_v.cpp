// WP-17: VesselDeltaV.cs and DeltaVEngineStageSet.cs.
#include <algorithm>
#include <climits>
#include <cmath>

#include "ksp/dotnet/math.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/vessel/delta_v.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {

bool Contains(const std::vector<int>& v, int x) { return std::find(v.begin(), v.end(), x) != v.end(); }
void AddUnique(std::vector<int>& v, int x) {
    if (!Contains(v, x)) v.push_back(x);
}
bool PartsContain(const std::vector<Part*>& parts, const Part* p) { return std::find(parts.begin(), parts.end(), p) != parts.end(); }

template <class T>
T* FindModuleOf(Part& part) {
    for (size_t i = 0; i < part.Modules().Count(); i++)
        if (auto* m = dynamic_cast<T*>(part.Modules().GetModule(i))) return m;
    return nullptr;
}

}  // namespace

// ============================================================================================================================
// DeltaVEngineStageSet.cs
// ============================================================================================================================

// DeltaVEngineStageSet.cs:205
DeltaVEngineStageSet::DeltaVEngineStageSet(VesselDeltaV* v) : vesselDeltaV(v) {}

// DeltaVEngineStageSet.cs:69 WorkingStageInfo: the instance of `workingIndex`, sorted descending by stage
std::vector<DeltaVStageInfoPtr> DeltaVEngineStageSet::WorkingStageInfo() {
    std::vector<DeltaVStageInfoPtr> list;
    for (const auto& e : stageInfoEntries) {
        const DeltaVStageInfoPtr& s = workingIndex == 0 ? e.stageInfo_InstanceOne : e.stageInfo_InstanceTwo;
        if (s != nullptr) list.push_back(s);
    }
    std::stable_sort(list.begin(), list.end(), [](const DeltaVStageInfoPtr& a, const DeltaVStageInfoPtr& b) { return a->stage > b->stage; });
    return list;
}

// DeltaVEngineStageSet.cs:98 OperatingStageInfo
std::vector<DeltaVStageInfoPtr> DeltaVEngineStageSet::OperatingStageInfo() {
    std::vector<DeltaVStageInfoPtr> list;
    auto byStage = [](const DeltaVStageInfoPtr& a, const DeltaVStageInfoPtr& b) { return a->stage > b->stage; };
    if (DeltaVGlobals::LoadedSceneIsFlight()) {
        for (const auto& e : stageInfoEntries) {
            if (flightSceneOperatingIndex == 0) {
                if (e.stageInfo_InstanceOne != nullptr && e.stageInfo_InstanceOne->stage < StageManager::CurrentStage()) list.push_back(e.stageInfo_InstanceOne);
            } else if (e.stageInfo_InstanceTwo != nullptr && e.stageInfo_InstanceTwo->stage < StageManager::CurrentStage()) {
                list.push_back(e.stageInfo_InstanceTwo);
            }
            if (operatingIndex == 0) {
                if (e.stageInfo_InstanceOne != nullptr && e.stageInfo_InstanceOne->stage == StageManager::CurrentStage()) list.push_back(e.stageInfo_InstanceOne);
            } else if (e.stageInfo_InstanceTwo != nullptr && e.stageInfo_InstanceTwo->stage == StageManager::CurrentStage()) {
                list.push_back(e.stageInfo_InstanceTwo);
            }
        }
        std::stable_sort(list.begin(), list.end(), byStage);
        return list;
    }
    for (const auto& e : stageInfoEntries) {
        const DeltaVStageInfoPtr& s = operatingIndex == 0 ? e.stageInfo_InstanceOne : e.stageInfo_InstanceTwo;
        if (s != nullptr) list.push_back(s);
    }
    std::stable_sort(list.begin(), list.end(), byStage);
    return list;
}

// DeltaVEngineStageSet.cs:157 WorkingEngineInfo
std::vector<DeltaVEngineInfoPtr>& DeltaVEngineStageSet::WorkingEngineInfo() {
    if (vesselDeltaV->SimulationRunning()) return workingIndex == 0 ? engineInfo_InstanceOne : engineInfo_InstanceTwo;
    return operatingIndex == 0 ? engineInfo_InstanceOne : engineInfo_InstanceTwo;
}

// DeltaVEngineStageSet.cs:185 OperatingEngineInfo
std::vector<DeltaVEngineInfoPtr>& DeltaVEngineStageSet::OperatingEngineInfo() {
    return operatingIndex == 0 ? engineInfo_InstanceOne : engineInfo_InstanceTwo;
}

// DeltaVEngineStageSet.cs:217
void DeltaVEngineStageSet::AddStageInfo(DeltaVStageInfoPtr stageInfo) {
    SetPayload(stageInfo.get());
    stageInfoEntries.emplace_back(stageInfo->vesselDeltaV);
    DeltaVStageInfoEntry& entry = stageInfoEntries.back();
    if (vesselDeltaV->SimulationRunning()) {
        if (workingIndex == 0) entry.stageInfo_InstanceOne = stageInfo;
        else entry.stageInfo_InstanceTwo = stageInfo;
    } else if (operatingIndex == 0) {
        entry.stageInfo_InstanceOne = stageInfo;
    } else {
        entry.stageInfo_InstanceTwo = stageInfo;
    }
}

// DeltaVEngineStageSet.cs:243
void DeltaVEngineStageSet::UpdateStageInfo(int stage) {
    if (StageManager::Instance == nullptr) return;
    int num = -1;
    for (size_t i = 0; i < stageInfoEntries.size(); i++) {
        if (stageInfoEntries[i].stageInfo_InstanceOne == nullptr || stageInfoEntries[i].stageInfo_InstanceOne->stage != stage) {
            if (stageInfoEntries[i].stageInfo_InstanceTwo != nullptr && stageInfoEntries[i].stageInfo_InstanceTwo->stage == stage) {
                num = static_cast<int>(i);
                break;
            }
            continue;
        }
        num = static_cast<int>(i);
        break;
    }
    auto make = [&]() {
        if (vesselDeltaV->GetVessel() != nullptr) return std::make_shared<DeltaVStageInfo>(vesselDeltaV->GetVessel(), stage, vesselDeltaV);
        return std::make_shared<DeltaVStageInfo>(vesselDeltaV->Ship(), stage, vesselDeltaV);
    };
    if (num != -1) {
        DeltaVStageInfoEntry& entry = stageInfoEntries[static_cast<size_t>(num)];
        const bool useOne = vesselDeltaV->SimulationRunning() ? workingIndex == 0 : operatingIndex == 0;
        DeltaVStageInfoPtr& slot = useOne ? entry.stageInfo_InstanceOne : entry.stageInfo_InstanceTwo;
        if (slot != nullptr) slot->Reset(stage, vesselDeltaV);
        else slot = make();
        SetPayload(slot.get());
    } else {
        AddStageInfo(make());
    }
}

// DeltaVEngineStageSet.cs:344
void DeltaVEngineStageSet::UpdateStageInfo() {
    if (StageManager::Instance != nullptr) {
        RemoveStaleStages();
        for (int num = StageManager::LastStage(); num >= 0; num--) UpdateStageInfo(num);
        RemoveStagedStages();
    }
    SortStages();
    payloadStages.clear();
}

// DeltaVEngineStageSet.cs:359
void DeltaVEngineStageSet::RemoveStaleStages() {
    if (StageManager::Instance == nullptr) return;
    const int lastStage = StageManager::LastStage();
    int count = static_cast<int>(stageInfoEntries.size());
    while (count-- > 0) {
        DeltaVStageInfoEntry& e = stageInfoEntries[static_cast<size_t>(count)];
        if (e.stageInfo_InstanceOne != nullptr) {
            if (e.stageInfo_InstanceOne->stage > lastStage) stageInfoEntries.erase(stageInfoEntries.begin() + count);
        } else if (e.stageInfo_InstanceTwo != nullptr) {
            if (e.stageInfo_InstanceTwo->stage > lastStage) stageInfoEntries.erase(stageInfoEntries.begin() + count);
        } else {
            stageInfoEntries.erase(stageInfoEntries.begin() + count);
        }
    }
}

// DeltaVEngineStageSet.cs:390
void DeltaVEngineStageSet::SetPayload(DeltaVStageInfo* stageInfo) {
    if (stageInfo != nullptr) stageInfo->payloadStage = std::find(payloadStages.begin(), payloadStages.end(), stageInfo->stage) != payloadStages.end();
}

// DeltaVEngineStageSet.cs:405
void DeltaVEngineStageSet::RemoveStagedStages(bool sortStages) {
    if (StageManager::Instance != nullptr) {
        for (int num = static_cast<int>(stageInfoEntries.size()) - 1; num >= 0; num--) {
            DeltaVStageInfoEntry& e = stageInfoEntries[static_cast<size_t>(num)];
            const bool useOne = vesselDeltaV->SimulationRunning() ? workingIndex == 0 : operatingIndex == 0;
            DeltaVStageInfoPtr& slot = useOne ? e.stageInfo_InstanceOne : e.stageInfo_InstanceTwo;
            if (slot != nullptr && slot->stage > StageManager::LastStage()) slot = nullptr;
        }
    }
    if (sortStages) SortStages();
}

// DeltaVEngineStageSet.cs:444 (StageSortingInstanceOne: by InstanceOne.stage descending, a null InstanceOne last)
void DeltaVEngineStageSet::SortStages() {
    std::stable_sort(stageInfoEntries.begin(), stageInfoEntries.end(), [](const DeltaVStageInfoEntry& x, const DeltaVStageInfoEntry& y) {
        if (x.stageInfo_InstanceOne == nullptr) return false;
        if (y.stageInfo_InstanceOne == nullptr) return true;
        return x.stageInfo_InstanceOne->stage > y.stageInfo_InstanceOne->stage;
    });
}

// DeltaVEngineStageSet.cs:450
void DeltaVEngineStageSet::AddEngineWorkingSet(DeltaVEngineInfoPtr engineInfo) {
    if (workingIndex == 0) engineInfo_InstanceOne.push_back(std::move(engineInfo));
    else engineInfo_InstanceTwo.push_back(std::move(engineInfo));
}

// DeltaVEngineStageSet.cs:470
void DeltaVEngineStageSet::RemoveInvalidEnginesWorkingSet() {
    std::vector<DeltaVEngineInfoPtr>& list = workingIndex == 0 ? engineInfo_InstanceOne : engineInfo_InstanceTwo;
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        DeltaVEngineInfo& e = *list[static_cast<size_t>(count)];
        Part* p = e.engine == nullptr ? nullptr : static_cast<Part*>(e.engine->part);
        if (vesselDeltaV->ActiveMode() == VesselDeltaV::Mode::Vessel && vesselDeltaV->GetVessel() != nullptr) {
            if (e.engine == nullptr || p == nullptr || !PartsContain(vesselDeltaV->GetVessel()->parts, p)) list.erase(list.begin() + count);
        } else if (vesselDeltaV->ActiveMode() == VesselDeltaV::Mode::Ship && vesselDeltaV->Ship() != nullptr &&
                   (e.engine == nullptr || p == nullptr || !PartsContain(vesselDeltaV->Ship()->liveParts, p))) {
            list.erase(list.begin() + count);
        }
    }
}

// ============================================================================================================================
// VesselDeltaV.cs
// ============================================================================================================================

VesselDeltaV::~VesselDeltaV() = default;

// VesselDeltaV.cs:154 Create(Vessel)
std::unique_ptr<VesselDeltaV> VesselDeltaV::Create(Vessel* vesselRef) {
    std::unique_ptr<VesselDeltaV> v(new VesselDeltaV());
    v->_vessel = vesselRef;
    v->_ship = nullptr;
    v->engineStageSet = std::make_unique<DeltaVEngineStageSet>(v.get());
    v->_activeMode = Mode::Vessel;
    return v;
}

// VesselDeltaV.cs:170 Create(ShipConstruct)
std::unique_ptr<VesselDeltaV> VesselDeltaV::Create(ShipConstruct* shipRef) {
    std::unique_ptr<VesselDeltaV> v(new VesselDeltaV());
    v->_ship = shipRef;
    v->_vessel = nullptr;
    v->engineStageSet = std::make_unique<DeltaVEngineStageSet>(v.get());
    for (size_t i = 0; i < shipRef->liveParts.size(); i++) shipRef->liveParts[i]->ship = shipRef;
    v->_activeMode = Mode::Ship;
    return v;
}

// VesselDeltaV.cs:132
bool VesselDeltaV::EnableStockSimluation() {
    if (!DeltaVGameSettings::DELTAV_CALCULATIONS_ENABLED) return false;
    _doStockSimulation = true;
    return true;
}

// VesselDeltaV.cs:143
bool VesselDeltaV::DisableStockSimluation() {
    _doStockSimulation = false;
    return true;
}

const std::vector<Part*>& VesselDeltaV::PartsOfActive() const { return _activeMode == Mode::Vessel ? _vessel->parts : _ship->liveParts; }

// VesselDeltaV.cs:187 Start (without the GameEvents subscriptions)
void VesselDeltaV::Start() {
    if (ActiveMode() == Mode::Vessel && _vessel != nullptr) {
        CalculateSeparationStageIndexes(_vessel->parts);
        ResetPartInfo(_vessel->parts);
    } else if (ActiveMode() == Mode::Ship && _ship != nullptr) {
        CalculateSeparationStageIndexes(_ship->liveParts);
        ResetPartInfo(_ship->liveParts);
    }
    calcsDirty_ = true;
    _doStockSimulation = DeltaVGlobals::DoStockSimulations();
}

// VesselDeltaV.cs:247 CheckDirtyAndRun, editor branch: a run starts while the calculations are dirty or the instances must be synced
void VesselDeltaV::DriveToCompletion() {
    int guard = 0;
    while (_doStockSimulation && (calcsDirty_ || syncListInstances_) && guard++ < 16) RunCalculations();
}

// VesselDeltaV.cs:471
void VesselDeltaV::SetCalcsDirty(bool resetPartCaches, bool syncListInstances) {
    calcsDirty_ = true;
    resetPartCaches_ = resetPartCaches_ || resetPartCaches;
    syncListInstances_ = syncListInstances_ || syncListInstances;
}

// VesselDeltaV.cs:555
void VesselDeltaV::CalculateSeparationStageIndexes(const std::vector<Part*>& parts) {
    _separationStageIndexes.clear();
    for (size_t i = 0; i < parts.size(); i++) {
        Part& p = *parts[i];
        if (FindModuleOf<ModuleDecouple>(p) != nullptr || FindModuleOf<ModuleAnchoredDecoupler>(p) != nullptr) {
            IStageSeparatorChild* stageSeparatorChild = FindModuleOf<IStageSeparatorChild>(p);
            std::vector<Part*> decoupledChildParts;
            if (stageSeparatorChild != nullptr && !stageSeparatorChild->PartDetaches(decoupledChildParts)) AddUnique(_separationStageIndexes, p.fields.inverseStage);
            AddUnique(_separationStageIndexes, p.fields.separationIndex);
        }
    }
}

// VesselDeltaV.cs:573
void VesselDeltaV::UpdateStageInfo() {
    if (_vessel != nullptr) {
        CalculateSeparationStageIndexes(_vessel->parts);
        ResetPartInfo(_vessel->parts);
    } else if (_ship != nullptr) {
        CalculateSeparationStageIndexes(_ship->liveParts);
        ResetPartInfo(_ship->liveParts);
    }
    engineStageSet->UpdateStageInfo();
    ProcessPayloadStages();
}

// VesselDeltaV.cs:589
void VesselDeltaV::ProcessPayloadStages() {
    engineStageSet->payloadStages.clear();
    std::vector<DeltaVStageInfoPtr> workingStageInfo = WorkingStageInfo();
    for (size_t i = 0; i < workingStageInfo.size(); i++) {
        DeltaVStageInfo& s = *workingStageInfo[i];
        if (s.stage == StageManager::CurrentStage()) continue;
        bool flag = false;
        for (size_t j = 0; j < s.enginesInStage.size(); j++) {
            DeltaVEngineInfo& e = *s.enginesInStage[j];
            if (e.partInfo() != nullptr && e.engine != nullptr && e.engine->part != nullptr && e.partInfo()->decoupleBeforeBurn &&
                e.partInfo()->part->fields.inverseStage == s.stage &&
                (!e.engine->Get<bool>("nonThrustMotor") || (e.engine->Get<bool>("nonThrustMotor") && e.engine->Get<bool>("includeinDVCalcs")))) {
                flag = true;
            }
        }
        if (flag) AddUnique(engineStageSet->payloadStages, s.stage);
    }
}

// VesselDeltaV.cs:624
void VesselDeltaV::ResetPartInfo(const std::vector<Part*>& parts) {
    int count = static_cast<int>(_partInfo.size());
    while (count-- > 0) {
        if (_partInfo[static_cast<size_t>(count)]->part == nullptr || !PartsContain(parts, _partInfo[static_cast<size_t>(count)]->part))
            _partInfo.erase(_partInfo.begin() + count);
    }
    for (size_t i = 0; i < parts.size(); i++) {
        if (parts[i]->isLaunchClamp()) continue;
        bool flag = false;
        for (size_t j = 0; j < _partInfo.size(); j++) {
            if (_partInfo[j]->part == parts[i]) {
                _partInfo[j]->decoupleBeforeBurn = false;
                _partInfo[j]->CalculateMassValues();
                _partInfo[j]->CalculateStagingValues();
                flag = true;
                break;
            }
        }
        if (!flag) _partInfo.push_back(std::make_shared<DeltaVPartInfo>(parts[i], this));
    }
    for (size_t k = 0; k < _partInfo.size(); k++) {
        DeltaVPartInfo* deltaVPartInfo = _partInfo[k].get();
        if (deltaVPartInfo->isEngine && !deltaVPartInfo->engines.empty()) {
            AttachNode* attachNode = deltaVPartInfo->part->FindAttachNode("bottom");
            if (attachNode != nullptr && attachNode->attachedPart != nullptr) {
                DeltaVEngineInfo* wei = deltav_ext::Get(WorkingEngineInfo(), deltaVPartInfo->part);
                const int num = wei != nullptr ? wei->startBurnStage : deltaVPartInfo->activationStage;
                DeltaVPartInfo* deltaVPartInfo2 = deltav_ext::Get(_partInfo, attachNode->attachedPart);
                if (deltaVPartInfo2 != nullptr && !deltaVPartInfo->engines[0]->Get<bool>("nonThrustMotor") && deltaVPartInfo2->decoupleStage <= num) {
                    deltaVPartInfo2->decoupleStage = num;
                    if (deltaVPartInfo2->activationStage < deltaVPartInfo2->decoupleStage) deltaVPartInfo2->activationStage = deltaVPartInfo2->decoupleStage;
                    DeltaVEngineInfo* wei2 = deltav_ext::Get(WorkingEngineInfo(), deltaVPartInfo2->part);
                    if (deltaVPartInfo2->isEngine && (wei2 != nullptr ? wei2->startBurnStage : deltaVPartInfo2->activationStage) <= num)
                        deltaVPartInfo2->decoupleBeforeBurn = true;
                    ProcessEnginePartInfoChildren(deltaVPartInfo2);
                }
            }
        }
        if (deltaVPartInfo->isDockingPort && deltaVPartInfo->moduleDockingNode != nullptr && deltaVPartInfo->moduleDockingNode->StagingEnabled() &&
            (!DeltaVGlobals::LoadedSceneIsFlight() || (DeltaVGlobals::LoadedSceneIsFlight() && deltaVPartInfo->activationStage > 0) ||
             (DeltaVGlobals::LoadedSceneIsFlight() && deltaVPartInfo->activationStage == StageManager::CurrentStage()))) {
            DeltaVPartInfo* deltaVPartInfo3 = nullptr;
            if (deltaVPartInfo->part != nullptr && deltaVPartInfo->part->parent != nullptr) deltaVPartInfo3 = deltav_ext::Get(_partInfo, deltaVPartInfo->part->parent);
            if (deltaVPartInfo3 != nullptr && deltaVPartInfo->decoupleStage != deltaVPartInfo3->decoupleStage && !deltaVPartInfo3->isEngine)
                deltaVPartInfo->decoupleStage = deltaVPartInfo3->decoupleStage;
            bool setToParentDecoupleStage = false;
            bool bypassFairings = false;
            if ((DeltaVGlobals::LoadedSceneIsFlight() && deltaVPartInfo->activationStage == StageManager::CurrentStage()) || GetActivatedEngines() > 0) {
                deltaVPartInfo->activationStage--;
                setToParentDecoupleStage = true;
                bypassFairings = true;
            }
            ProcessDecouplePartInfoChildren(deltaVPartInfo, PartStageComparisonOperator::NotEqual, deltaVPartInfo->decoupleStage, deltaVPartInfo->activationStage,
                                            bypassFairings, true, setToParentDecoupleStage);
        }
        if (deltaVPartInfo->isDockingPort && deltaVPartInfo->moduleDockingNode != nullptr && !deltaVPartInfo->moduleDockingNode->StagingEnabled()) {
            ProcessDecouplePartInfoChildren(deltaVPartInfo, PartStageComparisonOperator::NotEqual, deltaVPartInfo->decoupleStage, deltaVPartInfo->activationStage,
                                            true, true, false);
        }
        if ((deltaVPartInfo->isDecoupler && ((deltaVPartInfo->moduleDecoupler != nullptr && deltaVPartInfo->moduleDecoupler->Get<bool>("stagingEnabled")) ||
                                             (deltaVPartInfo->moduleAnchoredDecoupler != nullptr && deltaVPartInfo->moduleAnchoredDecoupler->Get<bool>("stagingEnabled")))) ||
            deltaVPartInfo->isStageSeparator) {
            bool flag2 = false;
            bool bypassFairings2 = false;
            if (DeltaVGlobals::LoadedSceneIsFlight() && (deltaVPartInfo->activationStage == StageManager::CurrentStage() || GetActivatedEngines() > 0)) {
                deltaVPartInfo->activationStage--;
                flag2 = true;
                bypassFairings2 = true;
            }
            std::vector<Part*> decoupledChildParts;
            IStageSeparatorChild* stageSeparatorChild = FindModuleOf<IStageSeparatorChild>(*deltaVPartInfo->part);
            if (stageSeparatorChild != nullptr) {
                flag2 = stageSeparatorChild->IsEnginePlate() || flag2;
                if (!stageSeparatorChild->PartDetaches(decoupledChildParts)) {
                    if (!decoupledChildParts.empty()) {
                        for (size_t l = 0; l < decoupledChildParts.size(); l++) {
                            DeltaVPartInfo* deltaVPartInfo4 = deltav_ext::Get(_partInfo, decoupledChildParts[l]);
                            if (deltaVPartInfo4 != nullptr && deltaVPartInfo4->decoupleStage < deltaVPartInfo->activationStage)
                                changePartInfoDecoupleStage(deltaVPartInfo, deltaVPartInfo4);
                            ProcessDecouplePartInfoChildren(deltaVPartInfo4, PartStageComparisonOperator::LessThan, deltaVPartInfo->decoupleStage,
                                                            deltaVPartInfo->activationStage, bypassFairings2, false, flag2);
                        }
                    } else if (stageSeparatorChild->IsEnginePlate()) {
                        ProcessDecouplePartInfoChildren(deltaVPartInfo, PartStageComparisonOperator::Equal, deltaVPartInfo->decoupleStage,
                                                        deltaVPartInfo->activationStage, bypassFairings2, false, flag2);
                    } else {
                        ProcessDecouplePartInfoChildren(deltaVPartInfo, PartStageComparisonOperator::LessThan, deltaVPartInfo->decoupleStage,
                                                        deltaVPartInfo->activationStage, bypassFairings2, false, flag2);
                    }
                } else {
                    ProcessDecouplePartInfoChildren(deltaVPartInfo, PartStageComparisonOperator::LessThan, deltaVPartInfo->decoupleStage,
                                                    deltaVPartInfo->activationStage, bypassFairings2, true, flag2);
                }
            }
        }
        if (deltaVPartInfo->isDecoupler && ((deltaVPartInfo->moduleDecoupler != nullptr && !deltaVPartInfo->moduleDecoupler->Get<bool>("stagingEnabled")) ||
                                            (deltaVPartInfo->moduleAnchoredDecoupler != nullptr && !deltaVPartInfo->moduleAnchoredDecoupler->Get<bool>("stagingEnabled")))) {
            ProcessDecouplePartInfoChildren(deltaVPartInfo, PartStageComparisonOperator::LessThan, deltaVPartInfo->decoupleStage, deltaVPartInfo->activationStage,
                                            false, true, false);
        }
    }
}

// VesselDeltaV.cs:767
void VesselDeltaV::ProcessEnginePartInfoChildren(DeltaVPartInfo* partInfoItem) {
    for (size_t i = 0; i < partInfoItem->part->children.size(); i++) {
        DeltaVPartInfo* deltaVPartInfo = deltav_ext::Get(_partInfo, partInfoItem->part->children[i]);
        if (deltaVPartInfo != nullptr && deltaVPartInfo->decoupleStage < partInfoItem->activationStage) {
            deltaVPartInfo->decoupleStage = partInfoItem->activationStage;
            if (deltaVPartInfo->activationStage < deltaVPartInfo->decoupleStage) deltaVPartInfo->activationStage = deltaVPartInfo->decoupleStage;
            DeltaVEngineInfo* wei = deltav_ext::Get(WorkingEngineInfo(), deltaVPartInfo->part);
            if (deltaVPartInfo->isEngine && (wei != nullptr ? wei->startBurnStage : deltaVPartInfo->activationStage) <= partInfoItem->activationStage)
                deltaVPartInfo->decoupleBeforeBurn = true;
            ProcessEnginePartInfoChildren(deltaVPartInfo);
        }
    }
}

// VesselDeltaV.cs:788
void VesselDeltaV::ProcessDecouplePartInfoChildren(DeltaVPartInfo* partInfoItem, PartStageComparisonOperator comparisonOp, int decoupleStage, int activationStage,
                                                   bool bypassFairings, bool bypassJettison, bool setToParentDecoupleStage) {
    for (size_t i = 0; i < partInfoItem->part->children.size(); i++) {
        DeltaVPartInfo* deltaVPartInfo = deltav_ext::Get(_partInfo, partInfoItem->part->children[i]);
        if (deltaVPartInfo == nullptr || (!bypassJettison && partInfoItem->jettisonPart != nullptr && deltaVPartInfo->part != partInfoItem->jettisonPart) ||
            (deltaVPartInfo->isFairing && !bypassFairings && deltaVPartInfo->decoupleStage > decoupleStage) || deltaVPartInfo->isDecoupler ||
            (deltaVPartInfo->isDockingPort && partInfoItem->moduleDockingNode != nullptr && partInfoItem->moduleDockingNode->StagingEnabled() &&
             deltaVPartInfo->decoupleStage > decoupleStage)) {
            continue;
        }
        switch (comparisonOp) {
            case PartStageComparisonOperator::LessThan:
                if (deltaVPartInfo->decoupleStage < activationStage) changePartInfoDecoupleStage(partInfoItem, deltaVPartInfo, setToParentDecoupleStage);
                break;
            case PartStageComparisonOperator::NotEqual:
                if (partInfoItem->isFairing) changePartInfoDecoupleStage(partInfoItem, deltaVPartInfo, true);
                else if (deltaVPartInfo->decoupleStage != activationStage) changePartInfoDecoupleStage(partInfoItem, deltaVPartInfo, setToParentDecoupleStage);
                break;
            case PartStageComparisonOperator::Equal:
                if (deltaVPartInfo->decoupleStage == activationStage) changePartInfoDecoupleStage(partInfoItem, deltaVPartInfo, setToParentDecoupleStage);
                break;
        }
        ProcessDecouplePartInfoChildren(deltaVPartInfo, comparisonOp, decoupleStage, activationStage, bypassFairings, bypassJettison, setToParentDecoupleStage);
    }
}

// VesselDeltaV.cs:826
void VesselDeltaV::changePartInfoDecoupleStage(DeltaVPartInfo* partInfoItem, DeltaVPartInfo* childPart, bool setToParentDecoupleStage) {
    if (setToParentDecoupleStage) childPart->decoupleStage = partInfoItem->decoupleStage;
    else childPart->decoupleStage = partInfoItem->activationStage;
    if (childPart->activationStage < childPart->decoupleStage) childPart->activationStage = childPart->decoupleStage;
    if (childPart->isEngine) {
        if (childPart->activationStage == childPart->decoupleStage) childPart->decoupleBeforeBurn = true;
        else childPart->decoupleBeforeBurn = false;
    }
}

// VesselDeltaV.cs:853 RunCalculations (the coroutine; its yields wait frames, so it runs to the end)
void VesselDeltaV::RunCalculations() {
    runCount_++;
    simulationRunning_ = true;  // `simulation = StartCoroutine(...)` was set by CheckDirtyAndRun
    lowestStageWithDeltaV = INT_MAX;
    if (useMultipleInfoLists) {
        engineStageSet->workingIndex = engineStageSet->operatingIndex == 0 ? 1 : 0;
    } else {
        engineStageSet->operatingIndex = 0;
        engineStageSet->workingIndex = 0;
    }
    if (resetPartCaches_) {
        ResetStagePartCaches();
        resetPartCaches_ = false;
    }
    calcsDirty_ = false;
    UpdateModuleEngines();
    UpdateStageInfo();
    if (WorkingEngineInfo().empty()) {
        ReCalculateVesselTotals();
        if (useMultipleInfoLists) engineStageSet->operatingIndex = engineStageSet->workingIndex;
        if (syncListInstances_) {
            syncListInstances_ = false;
            SetCalcsDirty(true);
        }
        simulationRunning_ = false;
        completedEvents_++;  // GameEvents.onDeltaVCalcsCompleted.Fire()
        return;
    }
    _totalDeltaVVac = 0.0;
    _totalDeltaVASL = 0.0;
    _totalDeltaVActual = 0.0;
    _totalBurnTime = 0.0;
    ResetSimulationResources();
    std::vector<DeltaVStageInfoPtr> stages = WorkingStageInfo();
    for (size_t i = 0; i < stages.size(); i++) {
        stages[i]->SimulateDeltaV(ActiveMode() == Mode::Vessel, false, DeltaVGameSettings::DELTAV_CALCULATIONS_TIMESTEP, false);
        if (!std::isnan(stages[i]->deltaVinVac)) _totalDeltaVVac += stages[i]->deltaVinVac;
        if (!std::isnan(stages[i]->deltaVatASL)) _totalDeltaVASL += stages[i]->deltaVatASL;
        if (!std::isnan(stageDVActual(stages[i].get()))) _totalDeltaVActual += stageDVActual(stages[i].get());
        if (!std::isnan(stages[i]->stageBurnTime)) _totalBurnTime += stages[i]->stageBurnTime;
        if ((stages[i]->deltaVinVac > 0.0f || stages[i]->deltaVatASL > 0.0f) && stages[i]->stage < lowestStageWithDeltaV) lowestStageWithDeltaV = stages[i]->stage;
    }
    ResetSimulationResources();
    if (useMultipleInfoLists) {
        engineStageSet->operatingIndex = engineStageSet->workingIndex;
        engineStageSet->flightSceneOperatingIndex = engineStageSet->operatingIndex;
    }
    if (syncListInstances_) {
        syncListInstances_ = false;
        SetCalcsDirty(true);
    } else {
        _isReady = true;
        completedEvents_++;  // GameEvents.onDeltaVCalcsCompleted.Fire()
    }
    simulationRunning_ = false;
}

// VesselDeltaV.cs:943
void VesselDeltaV::SimulateFlightScene() {
    if (StageManager::StageCount() <= 0) return;
    updateFlightScene_ = true;
    if (useMultipleInfoLists) {
        engineStageSet->workingIndex = engineStageSet->operatingIndex == 0 ? 1 : 0;
    } else {
        engineStageSet->operatingIndex = 0;
        engineStageSet->workingIndex = 0;
    }
    SimulateLastStage(false);
    std::vector<DeltaVStageInfoPtr> workingStageInfo = WorkingStageInfo();
    for (size_t i = 0; i < workingStageInfo.size(); i++) {
        if (workingStageInfo[i]->stage != StageManager::LastStage()) {
            workingStageInfo[i]->CalcLerpDeltaV();
            if ((workingStageInfo[i]->deltaVinVac > 0.0f || workingStageInfo[i]->deltaVatASL > 0.0f) && workingStageInfo[i]->stage < lowestStageWithDeltaV)
                lowestStageWithDeltaV = workingStageInfo[i]->stage;
        }
    }
    ReCalculateVesselTotals();
    if (useMultipleInfoLists) engineStageSet->operatingIndex = engineStageSet->workingIndex;
    updateFlightScene_ = false;
}

// VesselDeltaV.cs:982
void VesselDeltaV::SimulateLastStage(bool recalcVesselTotals) {
    DeltaVStageInfo* workingStage = GetWorkingStage(StageManager::CurrentStage());
    if (workingStage == nullptr && GetActivatedEngines() > 0) workingStage = GetWorkingStage(StageManager::LastStage());
    currentStageActivated = false;
    if (workingStage != nullptr) {
        ProcessActiveUpdateEngines(workingStage);
        ResetSimulationResources();
        for (size_t i = 0; i < _partInfo.size(); i++) _partInfo[i]->CalculateMassValues();
        if (recalcVesselTotals) ReCalculateVesselTotals();
        ResetSimulationResources();
        currentStageActivated = true;
        if (workingStage->deltaVinVac > 0.0f || workingStage->deltaVatASL > 0.0f) lowestStageWithDeltaV = workingStage->stage;
    }
}

// VesselDeltaV.cs:1012
void VesselDeltaV::ReCalculateVesselTotals() {
    _totalDeltaVVac = 0.0;
    _totalDeltaVASL = 0.0;
    _totalDeltaVActual = 0.0;
    _totalBurnTime = 0.0;
    std::vector<DeltaVStageInfoPtr> w = WorkingStageInfo();
    for (size_t i = 0; i < w.size(); i++) {
        if (!std::isnan(w[i]->deltaVinVac)) _totalDeltaVVac += w[i]->deltaVinVac;
        if (!std::isnan(w[i]->deltaVatASL)) _totalDeltaVASL += w[i]->deltaVatASL;
        if (!std::isnan(stageDVActual(w[i].get()))) _totalDeltaVActual += stageDVActual(w[i].get());
        if (!std::isnan(w[i]->stageBurnTime)) _totalBurnTime += w[i]->stageBurnTime;
    }
}

// VesselDeltaV.cs:1040
double VesselDeltaV::stageDVActual(DeltaVStageInfo* stageInfo) {
    if (DeltaVGlobals::LoadedSceneIsFlight() && FlightRegistry::ActiveVessel != nullptr) return stageInfo->deltaVActual;
    if (DeltaVGlobals::LoadedSceneIsEditor() && DeltaVGlobals::DeltaVAppValuesPtr() != nullptr) return stageInfo->deltaVActual;
    return stageInfo->deltaVinVac;
}

// VesselDeltaV.cs:1053
void VesselDeltaV::ResetSimulationResources() {
    if (ActiveMode() == Mode::Ship && _ship != nullptr)
        for (size_t i = 0; i < _ship->liveParts.size(); i++) _ship->liveParts[i]->ResetSimulation();
    if (ActiveMode() == Mode::Vessel && _vessel != nullptr)
        for (size_t j = 0; j < _vessel->parts.size(); j++) _vessel->parts[j]->ResetSimulation();
}

// VesselDeltaV.cs:1071
void VesselDeltaV::ResetStagePartCaches() {
    std::vector<DeltaVStageInfoPtr> w = WorkingStageInfo();
    for (size_t i = 0; i < w.size(); i++) w[i]->ResetPartCaches();
    std::vector<DeltaVStageInfoPtr> o = OperatingStageInfo();
    for (size_t j = 0; j < o.size(); j++) o[j]->ResetPartCaches();
}

// VesselDeltaV.cs:1085
void VesselDeltaV::UpdateModuleEngines() {
    std::vector<DeltaVEngineInfoPtr>& workingEngineInfo = WorkingEngineInfo();
    std::vector<MultiModeEngine*> list;
    auto multiOf = [](Part* p) -> MultiModeEngine* { return FindModuleOf<MultiModeEngine>(*p); };
    if (ActiveMode() == Mode::Vessel && _vessel != nullptr) {
        std::vector<PartModule*> list2 = _vessel->FindPartModulesImplementing([](PartModule& m) { return dynamic_cast<ModuleEngines*>(&m) != nullptr; });
        for (size_t i = 0; i < list2.size(); i++) {
            auto* me = static_cast<ModuleEngines*>(list2[i]);
            Part* p = static_cast<Part*>(me->part);
            if ((me->Get<bool>("nonThrustMotor") && !me->Get<bool>("includeinDVCalcs")) || p == nullptr) continue;
            MultiModeEngine* component = multiOf(p);
            if (component != nullptr) {
                if (std::find(list.begin(), list.end(), component) != list.end()) continue;
                list.push_back(component);
            }
            DeltaVEngineInfo* info = deltav_ext::Get(workingEngineInfo, p);
            if (info == nullptr) {
                auto created = std::make_shared<DeltaVEngineInfo>(this, me, component);
                info = created.get();
                engineStageSet->AddEngineWorkingSet(created);
            } else {
                info->Reset();
            }
            if (me->EngineIgnited() && info->startBurnStage < StageManager::LastStage()) info->startBurnStage = StageManager::LastStage();
        }
    } else if (ActiveMode() == Mode::Ship && _ship != nullptr) {
        for (size_t j = 0; j < _ship->liveParts.size(); j++) {
            std::vector<ModuleEngines*> engines;  // Part.isEngine(out engines)
            for (size_t m = 0; m < _ship->liveParts[j]->Modules().Count(); m++)
                if (auto* e = dynamic_cast<ModuleEngines*>(_ship->liveParts[j]->Modules().GetModule(m))) engines.push_back(e);
            if (engines.empty()) continue;
            for (size_t k = 0; k < engines.size(); k++) {
                Part* p = static_cast<Part*>(engines[k]->part);
                if ((engines[k]->Get<bool>("nonThrustMotor") && !engines[k]->Get<bool>("includeinDVCalcs")) || p == nullptr) continue;
                MultiModeEngine* component2 = multiOf(p);
                if (component2 != nullptr) {
                    if (std::find(list.begin(), list.end(), component2) != list.end()) continue;
                    list.push_back(component2);
                }
                DeltaVEngineInfo* info = deltav_ext::Get(workingEngineInfo, p);
                if (info == nullptr) {
                    auto created = std::make_shared<DeltaVEngineInfo>(this, engines[k], component2);
                    engineStageSet->AddEngineWorkingSet(created);
                } else {
                    info->Reset();
                }
            }
        }
    }
    engineStageSet->RemoveInvalidEnginesWorkingSet();
}

// VesselDeltaV.cs:1163
void VesselDeltaV::ProcessActiveUpdateEngines(DeltaVStageInfo* currentDeltaVStage) {
    std::vector<DeltaVEngineInfoPtr>& workingEngineInfo = WorkingEngineInfo();
    for (size_t i = 0; i < workingEngineInfo.size(); i++) {
        workingEngineInfo[i]->deprived = false;
        if (currentDeltaVStage != nullptr) workingEngineInfo[i]->stageBurnTotals.erase(currentDeltaVStage->stage);
    }
    std::vector<DeltaVStageInfoPtr> w = WorkingStageInfo();
    for (size_t j = 0; j < w.size(); j++) w[j]->ProcessActiveEngines();
}

// VesselDeltaV.cs:1181
DeltaVStageInfo* VesselDeltaV::GetWorkingStage(int stage) {
    static thread_local std::vector<DeltaVStageInfoPtr> keep;
    keep = WorkingStageInfo();
    for (auto& s : keep)
        if (s->stage == stage) return s.get();
    return nullptr;
}

// VesselDeltaV.cs:1201
DeltaVStageInfo* VesselDeltaV::GetStage(int stage) {
    static thread_local std::vector<DeltaVStageInfoPtr> keep;
    keep = OperatingStageInfo();
    for (auto& s : keep)
        if (s->stage == stage) return s.get();
    return nullptr;
}

// VesselDeltaV.cs:1221
int VesselDeltaV::GetActivatedEngines() {
    int num = 0;
    std::vector<DeltaVEngineInfoPtr>& w = WorkingEngineInfo();
    for (size_t i = 0; i < w.size(); i++)
        if (w[i]->engine != nullptr && w[i]->engine->isOperational()) num++;
    return num;
}

// VesselDeltaV.cs:1235
int VesselDeltaV::GetHighestSeparationStage(int inStage) const {
    int num = 0;
    for (size_t i = 0; i < _separationStageIndexes.size(); i++) {
        if (_separationStageIndexes[i] == inStage) return inStage;
        if (_separationStageIndexes[i] < inStage) num = dotnet::Math::Max(num, _separationStageIndexes[i]);
    }
    return num;
}

// VesselDeltaV.cs:1259
void VesselDeltaV::ResetSeparationIndexes() {
    if (ActiveMode() == Mode::Vessel && _vessel != nullptr) CalculateSeparationStageIndexes(_vessel->parts);
    else if (ActiveMode() == Mode::Ship && _ship != nullptr) CalculateSeparationStageIndexes(_ship->liveParts);
}

}  // namespace ksp
