// WP-53: Expansions/Serenity/DeployedScience/Runtime/*.cs (see include/ksp/serenity/deployed_science.hpp for the component model).
#include "ksp/serenity/deployed_science.hpp"

#include <algorithm>
#include <limits>

#include "ksp/career/career_environment.hpp"
#include "ksp/career/format.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/commnet/comm_net_vessel.hpp"
#include "ksp/commnet/commnet_scene.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/core/coroutine_host.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Serenity::DeployedScience::Runtime {
namespace {

using GE = events::GameEvents;
using KSP::Localization::Localizer;

bool SerenityInstalled() { return SaveEnvironment::Current().IsExpansionInstalled("Serenity"); }
double UT() { return ScienceEnvironment::UniversalTime(); }

Part* FindLoadedPart(uint32_t id) {  // FlightGlobals.FindLoadedPart (FlightGlobals.cs:441)
    SaveEnvironment& env = SaveEnvironment::Current();
    if (id == 0 || !env.persistentIds.fetch) return nullptr;
    const void* p = nullptr;
    if (env.persistentIds.persistentLoadedPartIds.TryGetValue(id, p)) return const_cast<Part*>(static_cast<const Part*>(p));
    return nullptr;
}
ProtoPartSnapshot* FindUnloadedPart(uint32_t id) {  // FlightGlobals.FindUnloadedPart (FlightGlobals.cs:457)
    SaveEnvironment& env = SaveEnvironment::Current();
    if (!env.persistentIds.fetch) return nullptr;
    ProtoPartSnapshot* p = nullptr;
    if (env.persistentIds.persistentUnloadedPartIds.TryGetValue(id, p)) return p;
    return nullptr;
}

template <class T>
T* FindImpl(Part& p) {
    return static_cast<T*>(p.FindModuleImplementing([](PartModule& m) { return dynamic_cast<T*>(&m) != nullptr; }));
}
template <class T>
bool UnityNull(const T* o) {
    return o == nullptr || o->destroyed;
}
CelestialBody* BodyByName(const std::string& name) {  // FlightGlobals.GetBodyByName (FlightGlobals.cs:1736; searches from the end)
    const std::vector<CelestialBody*>& bodies = FlightRegistry::Bodies();
    for (size_t i = bodies.size(); i-- > 0;)
        if (bodies[i]->name() == name) return bodies[i];
    return nullptr;
}
CelestialBody* HomeBody() { return CareerEnvironment::Current().HomeBody(); }

void PostMsg(const std::string& text) { SerenityEnvironment::PostScreenMessage(text); }

template <class T>
void AddUnique(std::vector<T>& v, const T& x) {  // ListExtensions.AddUnique
    if (std::find(v.begin(), v.end(), x) == v.end()) v.push_back(x);
}
template <class T>
bool Contains(const std::vector<T>& v, const T& x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}
template <class T>
void RemoveFirst(std::vector<T>& v, const T& x) {
    auto it = std::find(v.begin(), v.end(), x);
    if (it != v.end()) v.erase(it);
}

}  // namespace

// =============================================================================================================================
// SerenityBehaviour

std::vector<std::shared_ptr<SerenityBehaviour>>& SerenityBehaviour::live() {
    static std::vector<std::shared_ptr<SerenityBehaviour>> v;
    return v;
}

void SerenityBehaviour::DestroyGameObject() { destroyed = true; }

bool SerenityBehaviour::IsLive(const SerenityBehaviour* b) {
    for (const auto& p : live())
        if (p.get() == b) return true;
    return false;
}

void SerenityBehaviour::RunPendingStarts() {
    for (size_t i = 0; i < live().size(); i++) {
        std::shared_ptr<SerenityBehaviour> b = live()[i];
        if (!b->started && !b->destroyed) {
            b->started = true;
            b->Start();
        }
    }
}

void SerenityBehaviour::FlushDestroyed() {
    if (running_) return;
    running_ = true;
    std::vector<std::shared_ptr<SerenityBehaviour>> graveyard;
    for (;;) {
        std::vector<std::shared_ptr<SerenityBehaviour>> batch;
        for (size_t i = 0; i < live().size();) {
            if (live()[i]->destroyed) {
                batch.push_back(live()[i]);
                live().erase(live().begin() + static_cast<std::ptrdiff_t>(i));
            } else {
                ++i;
            }
        }
        if (batch.empty()) break;
        for (auto& b : batch) b->OnDestroy();  // may destroy children (they land in live() with destroyed set)
        graveyard.insert(graveyard.end(), batch.begin(), batch.end());
    }
    running_ = false;
}

void SerenityBehaviour::DestroyAll() {
    for (auto& b : live()) b->destroyed = true;
    FlushDestroyed();
}

// =============================================================================================================================
// DeployedScienceExtensions.cs

namespace DeployedScienceExtensions {

// :7
DeployedSciencePart* Get(const std::vector<DeployedSciencePart*>& list, uint32_t persistentId) {
    size_t num = 0;
    while (true) {
        if (num < list.size()) {
            if (list[num]->PartId() == persistentId) break;
            num++;
            continue;
        }
        return nullptr;
    }
    return list[num];
}

// :26
DeployedSciencePart* GetExperiment(const std::vector<DeployedSciencePart*>& list, const std::string& experimentId) {
    size_t num = 0;
    while (true) {
        if (num < list.size()) {
            if (list[num]->Experiment != nullptr && list[num]->Experiment->ExperimentId() == experimentId) break;
            num++;
            continue;
        }
        return nullptr;
    }
    return list[num];
}

// :45
std::vector<std::string> GetExperiments(const std::vector<DeployedSciencePart*>& list) {
    std::vector<std::string> list2;
    for (size_t i = 0; i < list.size(); i++)
        if (list[i]->Experiment != nullptr) AddUnique(list2, list[i]->Experiment->ExperimentId());
    return list2;
}

// :58
ModuleGroundSciencePart* Get(const std::vector<ModuleGroundSciencePart*>& list, uint32_t persistentId) {
    size_t num = 0;
    while (true) {
        if (num < list.size()) {
            if (list[num]->part != nullptr && list[num]->part->fields.persistentId == persistentId) break;
            num++;
            continue;
        }
        return nullptr;
    }
    return list[num];
}

// :77
DeployedScienceExperiment* GetExperimentByPartID(const std::vector<DeployedSciencePart*>& list, uint32_t persistentId) {
    size_t num = 0;
    while (true) {
        if (num < list.size()) {
            if (list[num]->Experiment != nullptr && list[num]->PartId() == persistentId) break;
            num++;
            continue;
        }
        return nullptr;
    }
    return list[num]->Experiment;
}

}  // namespace DeployedScienceExtensions

// =============================================================================================================================
// DeployedSciencePart

// DeployedSciencePart.cs:89
DeployedSciencePart* DeployedSciencePart::Spawn() {
    DeployedSciencePart* p = SerenityBehaviour::Add(std::shared_ptr<DeployedSciencePart>(new DeployedSciencePart()));
    p->Awake();
    return p;
}

// DeployedSciencePart.cs:96
DeployedSciencePart* DeployedSciencePart::Spawn(ModuleGroundSciencePart* part, ModuleGroundExpControl* controlUnit, DeployedScienceCluster* cluster) {
    DeployedSciencePart* dsp = Spawn();
    if (cluster != nullptr) dsp->Cluster = cluster;
    if (controlUnit != nullptr) dsp->controllerId = controlUnit->part->fields.persistentId;
    if (part != nullptr) {
        dsp->partId = part->part->fields.persistentId;
        dsp->UpdateSciencePart(part);
    }
    return dsp;
}

// DeployedSciencePart.cs:117
DeployedSciencePart* DeployedSciencePart::SpawnandLoad(const ConfigNode& node, DeployedScienceCluster* cluster) {
    DeployedSciencePart* dsp = Spawn();
    if (cluster != nullptr) dsp->Cluster = cluster;
    dsp->Load(node);
    if (dsp->Experiment != nullptr && cluster != nullptr) {
        dsp->Experiment->Cluster = cluster;
        dsp->Experiment->sciencePart = dsp;
    }
    dsp->Cluster = cluster;
    return dsp;
}

DeployedSciencePart::~DeployedSciencePart() {
    GE::onGroundSciencePartChanged.Remove(this, &DeployedSciencePart::UpdateSciencePart);
    GE::onGroundSciencePartRemoved.Remove(this, &DeployedSciencePart::OnGroundSciencePartRemoved);
    GE::onGroundSciencePartEnabledStateChanged.Remove(this, &DeployedSciencePart::UpdateSciencePart);
}

// DeployedSciencePart.cs:137
void DeployedSciencePart::Awake() {
    if (!SerenityInstalled()) destroyed = true;  // Object.Destroy(this)
}

// DeployedSciencePart.cs:145
void DeployedSciencePart::Start() {
    GE::onGroundSciencePartChanged.Add(this, &DeployedSciencePart::UpdateSciencePart);
    GE::onGroundSciencePartRemoved.Add(this, &DeployedSciencePart::OnGroundSciencePartRemoved);
    GE::onGroundSciencePartEnabledStateChanged.Add(this, &DeployedSciencePart::UpdateSciencePart);
}

// DeployedSciencePart.cs:152
void DeployedSciencePart::OnDestroy() {
    GE::onGroundSciencePartChanged.Remove(this, &DeployedSciencePart::UpdateSciencePart);
    GE::onGroundSciencePartRemoved.Remove(this, &DeployedSciencePart::OnGroundSciencePartRemoved);
    GE::onGroundSciencePartEnabledStateChanged.Remove(this, &DeployedSciencePart::UpdateSciencePart);
    if (Experiment != nullptr) Experiment->DestroyGameObject();
}

// DeployedSciencePart.cs:163
void DeployedSciencePart::OnGroundSciencePartRemoved(ModuleGroundSciencePart* sciencePartRemoved) {
    if (ControllerId() == sciencePartRemoved->ControlUnitId() && sciencePartRemoved->part->fields.persistentId != sciencePartRemoved->ControlUnitId())
        controllerId = 0u;
}

// DeployedSciencePart.cs:47 getter
bool DeployedSciencePart::Enabled() { return moduleEnabled; }

// DeployedSciencePart.cs:47 setter
void DeployedSciencePart::SetEnabled(bool value) {
    if (UnityNull(loadedPartModule)) {
        Part* partout = FindLoadedPart(partId);
        if (partout != nullptr) loadedPartModule = FindImpl<ModuleGroundSciencePart>(*partout);
    }
    moduleEnabled = value;
    if (!UnityNull(loadedPartModule) && loadedPartModule->Enabled() != moduleEnabled)
        loadedPartModule->SetEnabled(moduleEnabled);
    else if (!UnityNull(Cluster))
        Cluster->UpdatePowerState();
}

// DeployedSciencePart.cs:171
void DeployedSciencePart::UpdateSciencePart(ModuleGroundSciencePart* part) {
    if (PartId() != part->part->fields.persistentId) return;
    moduleEnabled = part->Enabled();
    deployedOnGround = part->DeployedOnGround();
    powerUnitsProduced = part->PowerUnitsProduced();
    actualPowerUnitsProduced = part->ActualPowerUnitsProduced();
    powerUnitsRequired = part->PowerUnitsRequired();
    isSolarPanel = part->IsSolarPanel();
    ModuleGroundExperiment* component = FindImpl<ModuleGroundExperiment>(static_cast<Part&>(*part->part));
    if (component != nullptr) {
        if (UnityNull(Experiment)) {
            if (!UnityNull(Cluster)) {
                Experiment = DeployedScienceExperiment::Spawn(component, Cluster);
                Experiment->sciencePart = this;
            }
        } else {
            Experiment->UpdateScienceExperiment(component);
        }
    }
    isAntenna = false;
    antennaBoosterPower = 0.0;
    ModuleGroundCommsPart* component2 = FindImpl<ModuleGroundCommsPart>(static_cast<Part&>(*part->part));
    if (component2 != nullptr) {
        isAntenna = true;
        antennaBoosterPower = component2->antennaPower();
    }
    if (!UnityNull(Cluster)) {
        if (isSolarPanel) AddUnique(Cluster->solarPanelParts, this);
        if (isAntenna) AddUnique(Cluster->antennaParts, this);
        Cluster->UpdatePowerState();
    }
}

// DeployedSciencePart.cs:222
Part* DeployedSciencePart::PartIsLoaded() {
    Part* partout = nullptr;
    if (partId != 0) {
        partout = FindLoadedPart(partId);
        return partout;
    }
    return partout;
}

// DeployedSciencePart.cs:233
void DeployedSciencePart::Load(const ConfigNode& node) {
    node.TryGetValue("PartId", partId);
    node.TryGetValue("Enabled", moduleEnabled);
    node.TryGetValue("DeployedOnGround", deployedOnGround);
    node.TryGetValue("PowerUnitsProduced", powerUnitsProduced);
    node.TryGetValue("ActualPowerUnitsProduced", actualPowerUnitsProduced);
    node.TryGetValue("PowerUnitsRequired", powerUnitsRequired);
    node.TryGetValue("IsSolarPanel", isSolarPanel);
    node.TryGetValue("IsAntenna", isAntenna);
    node.TryGetValue("AntennaBoosterPower", antennaBoosterPower);
    ConfigNode* node2 = nullptr;
    if (node.TryGetNode("EXPERIMENT", node2)) Experiment = DeployedScienceExperiment::SpawnandLoad(*node2);
}

// DeployedSciencePart.cs:251
void DeployedSciencePart::Save(ConfigNode& node) {
    node.AddValue("PartId", PartId());
    node.AddValue("Enabled", Enabled());
    node.AddValue("DeployedOnGround", DeployedOnGround());
    node.AddValue("PowerUnitsProduced", PowerUnitsProduced());
    node.AddValue("ActualPowerUnitsProduced", actualPowerUnitsProduced);
    node.AddValue("PowerUnitsRequired", PowerUnitsRequired());
    node.AddValue("IsSolarPanel", IsSolarPanel());
    node.AddValue("IsAntenna", IsAntenna());
    node.AddValue("AntennaBoosterPower", AntennaBoosterPower());
    if (Experiment != nullptr) {
        ConfigNode* node2 = node.AddNode("EXPERIMENT");
        Experiment->Save(*node2);
    }
}

// =============================================================================================================================
// DeployedScienceExperiment

// DeployedScienceExperiment.cs:167
DeployedScienceExperiment* DeployedScienceExperiment::Spawn() {
    DeployedScienceExperiment* e = SerenityBehaviour::Add(std::shared_ptr<DeployedScienceExperiment>(new DeployedScienceExperiment()));
    e->Awake();
    e->experimentName = "";
    e->experimentId = "";
    e->scienceDiminishingModifierRate = 1.0f;
    e->LastScienceGeneratedUT = UT();
    e->distanceCurve = FloatCurve();
    return e;
}

// DeployedScienceExperiment.cs:180
DeployedScienceExperiment* DeployedScienceExperiment::Spawn(ModuleGroundExperiment* mge, DeployedScienceCluster* cluster) {
    DeployedScienceExperiment* e = Spawn();
    if (mge != nullptr) {
        e->Cluster = cluster;
        e->partId = mge->part->fields.persistentId;
        e->moduleGroundExperiment = mge;
        e->experimentName = mge->ClassName();
        e->experimentId = mge->experimentId();
        e->ExperimentVessel = static_cast<Part&>(*mge->part).vessel;
        e->UpdateScienceExperiment(mge);
        const FloatCurve* c = mge->DistanceCurve();
        if (c != nullptr) e->distanceCurve = *c;  // `distanceCurve = moduleGroundExperiment.distanceCurve` (a reference in C#; the module's curve is not mutated here)
    }
    return e;
}

// DeployedScienceExperiment.cs:202
DeployedScienceExperiment* DeployedScienceExperiment::SpawnandLoad(const ConfigNode& node) {
    DeployedScienceExperiment* e = Spawn();
    e->Load(node);
    return e;
}

DeployedScienceExperiment::~DeployedScienceExperiment() {
    GE::onGroundScienceExperimentScienceValueChanged.Remove(this, &DeployedScienceExperiment::UpdateScienceValueExperiment);
    GE::onGroundScienceExperimentScienceLimitChanged.Remove(this, &DeployedScienceExperiment::UpdateScienceLimitExperiment);
    GE::onGroundSciencePartEnabledStateChanged.Remove(this, &DeployedScienceExperiment::GroundSciencePartEnabledStateChanged);
    GE::onGroundScienceClusterPowerStateChanged.Remove(this, &DeployedScienceExperiment::GroundScienceClusterPowerStateChanged);
    StopAllCoroutines();
}

std::shared_ptr<Coroutine> DeployedScienceExperiment::StartCoroutine(Enumerator e) {
    return ScienceEnvironment::Scheduler().StartCoroutine(std::move(e), CoroutineOwner());
}
void DeployedScienceExperiment::StopAllCoroutines() {
    if (ScienceEnvironment::scheduler != nullptr) ScienceEnvironment::scheduler->StopAllCoroutines(CoroutineOwner());
}

// DeployedScienceExperiment.cs:210
void DeployedScienceExperiment::Awake() {
    if (!SerenityInstalled()) destroyed = true;
}

// DeployedScienceExperiment.cs:146
ModuleGroundExperiment* DeployedScienceExperiment::GroundExperimentModule() {
    if (!UnityNull(moduleGroundExperiment)) return moduleGroundExperiment;
    moduleGroundExperiment = nullptr;
    Part* partout = FindLoadedPart(partId);
    if (partout != nullptr) moduleGroundExperiment = FindImpl<ModuleGroundExperiment>(*partout);
    return moduleGroundExperiment;
}

// DeployedScienceExperiment.cs:218
void DeployedScienceExperiment::Start() {
    GE::onGroundScienceExperimentScienceValueChanged.Add(this, &DeployedScienceExperiment::UpdateScienceValueExperiment);
    GE::onGroundScienceExperimentScienceLimitChanged.Add(this, &DeployedScienceExperiment::UpdateScienceLimitExperiment);
    GE::onGroundSciencePartEnabledStateChanged.Add(this, &DeployedScienceExperiment::GroundSciencePartEnabledStateChanged);
    GE::onGroundScienceClusterPowerStateChanged.Add(this, &DeployedScienceExperiment::GroundScienceClusterPowerStateChanged);
    Experiment = ResearchAndDevelopment::GetExperiment(ExperimentId());
    if (ExperimentVessel == nullptr) {
        ExperimentVessel = FlightRegistry::FindVessel(vesselID);
        if (ExperimentVessel == nullptr) {
            Part* partout = FindLoadedPart(partId);
            if (partout != nullptr) {
                ExperimentVessel = partout->vessel;
            } else {
                ProtoPartSnapshot* partout2 = FindUnloadedPart(partId);
                if (partout2 != nullptr && partout2->pVesselRef != nullptr) ExperimentVessel = partout2->pVesselRef->vesselRef;
            }
        }
    }
    if (ExperimentVessel != nullptr) {
        if (Experiment == nullptr) throw NullReferenceException();
        const ExperimentSituations experimentSituation = ScienceUtil::GetExperimentSituation(*ExperimentVessel);
        if (Experiment->IsAvailableWhile(experimentSituation, *ExperimentVessel->mainBody())) {
            const std::string biome;
            const std::string empty;
            subject = ResearchAndDevelopment::GetExperimentSubject(*Experiment, experimentSituation, *ExperimentVessel->mainBody(), biome, empty);
            subject->applyScienceScale = Experiment->applyScienceScale;
            scienceValue = f32store(fx(Experiment->baseValue) * fx(Experiment->dataScale) * fx(subject->subjectValue) * (fx(ScienceModifierRate()) / 100.0));
            scienceLimit = subject->scienceCap;
            if (GroundExperimentModule() != nullptr) {
                moduleGroundExperiment->SetScienceValue(scienceValue);
                moduleGroundExperiment->SetScienceLimit(scienceLimit);
            }
            experimentSituationValid = true;
        } else {
            experimentSituationValid = false;
        }
    } else {
        log::Warning("[DeployedScienceExperiment]: Failed to find Vessel for Experiment " + experimentId + " " + std::to_string(partId));
    }
}

// DeployedScienceExperiment.cs:274
void DeployedScienceExperiment::FixedUpdate() {
    if (FlightRegistry::ready && !ExperimentSituationValid() && !UnityNull(sciencePart) && sciencePart->Enabled()) sciencePart->SetEnabled(false);
}

// DeployedScienceExperiment.cs:282
void DeployedScienceExperiment::OnDestroy() {
    GE::onGroundScienceExperimentScienceValueChanged.Remove(this, &DeployedScienceExperiment::UpdateScienceValueExperiment);
    GE::onGroundScienceExperimentScienceLimitChanged.Remove(this, &DeployedScienceExperiment::UpdateScienceLimitExperiment);
    GE::onGroundSciencePartEnabledStateChanged.Remove(this, &DeployedScienceExperiment::GroundSciencePartEnabledStateChanged);
    GE::onGroundScienceClusterPowerStateChanged.Remove(this, &DeployedScienceExperiment::GroundScienceClusterPowerStateChanged);
    StopAllCoroutines();
}

// DeployedScienceExperiment.cs:291
void DeployedScienceExperiment::SeismicDataCalcOverrideAdd(const SeismicDataCalc& method) {
    if (seismicDataCalcOverride.IsNull())
        seismicDataCalcOverride = method;
    else
        seismicDataCalcOverride = seismicDataCalcOverride + method;
}

// DeployedScienceExperiment.cs:303
void DeployedScienceExperiment::SeismicDataCalcOverrideRemove(const SeismicDataCalc& method) {
    if (seismicDataCalcOverride == method)
        seismicDataCalcOverride = SeismicDataCalc();
    else
        seismicDataCalcOverride = seismicDataCalcOverride - method;
}

// DeployedScienceExperiment.cs:315
void DeployedScienceExperiment::GroundScienceClusterPowerStateChanged(DeployedScienceCluster* cluster) {
    if (UnityNull(Cluster) || cluster->ControlModulePartId() != Cluster->ControlModulePartId()) return;
    noPowerMsgPosted = false;
    if (cluster->IsPowered()) {
        if (SeismicEventDataSendPending()) ProcessSeismicEvent();
        LastScienceGeneratedUT = UT();
    } else if (!UnityNull(sciencePart) && sciencePart->Enabled() && experimentId != DeployedScience::SeismicExperimentId) {
        StartCoroutine(CalcScience());
    }
}

// DeployedScienceExperiment.cs:336
void DeployedScienceExperiment::GroundSciencePartEnabledStateChanged(ModuleGroundSciencePart* sciencePartModule) {
    if (UnityNull(Cluster) || sciencePartModule->part->fields.persistentId != PartId()) return;
    if (sciencePartModule->Enabled() && sciencePartModule->DeployedOnGround()) {
        if (SeismicEventDataSendPending()) ProcessSeismicEvent();
        LastScienceGeneratedUT = UT();
    } else if (experimentId != DeployedScience::SeismicExperimentId) {
        StartCoroutine(CalcScience());
    }
}

// DeployedScienceExperiment.cs:356
Enumerator DeployedScienceExperiment::CalcScience() {
    while (Cluster->UpdatingScience()) co_yield YieldInstruction::Null();
    CalculateScience(Cluster->PartialPowerMultiplier());
    if (ExperimentSituationValid() && TimeToSendStoredData()) SendDataToComms();
    co_yield YieldInstruction::Null();
}

// DeployedScienceExperiment.cs:370
void DeployedScienceExperiment::UpdateScienceLimitExperiment(ModuleGroundExperiment* mge) {
    if (!UnityNull(mge) && PartId() == mge->part->fields.persistentId) {
        moduleGroundExperiment = mge;
        scienceLimit = mge->ScienceLimit();
    }
}

// DeployedScienceExperiment.cs:379
void DeployedScienceExperiment::UpdateScienceValueExperiment(ModuleGroundExperiment* mge) {
    if (!UnityNull(mge) && PartId() == mge->part->fields.persistentId) {
        moduleGroundExperiment = mge;
        scienceValue = mge->ScienceValue();
    }
}

// DeployedScienceExperiment.cs:388
void DeployedScienceExperiment::UpdateScienceExperiment(ModuleGroundExperiment* mge) {
    if (!UnityNull(mge) && PartId() == mge->part->fields.persistentId) {
        moduleGroundExperiment = mge;
        scienceLimit = mge->ScienceLimit();
        scienceValue = mge->ScienceValue();
        scienceModifierRate = mge->ScienceModifierRate();
    }
}

// DeployedScienceExperiment.cs:399
void DeployedScienceExperiment::CalculateScience(float solarMultiplierArg) {
    if (!UnityNull(sciencePart) && sciencePart->Enabled() && !(TotalScienceGenerated() >= ScienceLimit()) && ExperimentSituationValid() && !UnityNull(Cluster) &&
        Cluster->IsPowered()) {
        if (experimentId == DeployedScience::SeismicExperimentId) {
            LastScienceGeneratedUT = UT();
            return;
        }
        if (ExperimentVessel != nullptr && Experiment != nullptr && subject != nullptr) {
            // float num = ScienceValue / 60f / 60f * (float)(UT - LastScienceGeneratedUT) * ScienceDiminishingModifierRate * solarMultiplier;
            float num = f32store(fx(ScienceValue()) / 60.0 / 60.0 * fx(f32store(UT() - LastScienceGeneratedUT)) * fx(ScienceDiminishingModifierRate()) *
                                 fx(solarMultiplierArg));
            if (fx(TotalScienceGenerated()) + fx(num) > fx(ScienceLimit())) num = f32store(fx(ScienceLimit()) - fx(TotalScienceGenerated()));
            totalScienceGenerated = f32store(fx(totalScienceGenerated) + fx(num));
            storedScienceData = f32store(fx(storedScienceData) + fx(num) / fx(subject->subjectValue));
            GE::onGroundScienceGenerated.Fire(this, sciencePart, Cluster, num);
        }
        LastScienceGeneratedUT = UT();
    } else {
        if (!UnityNull(sciencePart) && TotalScienceGenerated() >= ScienceLimit() && sciencePart->Enabled()) sciencePart->SetEnabled(false);
        LastScienceGeneratedUT = UT();
    }
}

// DeployedScienceExperiment.cs:431
void DeployedScienceExperiment::CalcSendSeismicScience(Vessel* vessel, Part* part, float solarMultiplierArg) {
    if (!UnityNull(Cluster) && vessel != nullptr && !(experimentId != DeployedScience::SeismicExperimentId)) {
        solarMultiplier = solarMultiplierArg;
        if (Cluster->IsPowered() && !UnityNull(sciencePart) && sciencePart->Enabled()) {
            if (ExperimentVessel != nullptr && Experiment != nullptr && subject != nullptr) {
                seismicEventVelocity = vessel->loaded ? vessel->rb_velocity.magnitude() : static_cast<float>(vessel->srfSpeed);
                if (!seismicEventDataSendPending) {
                    if (!seismicPartListValid) {
                        seismicPartList.clear();
                        seismicPartListValid = true;
                    }
                    seismicPartList.clear();
                }
                if (vessel->loaded) {
                    if (part == nullptr) {
                        for (size_t i = 0; i < vessel->parts.size(); i++) processImpactVesselPart(vessel, vessel->parts[i]);
                    } else {
                        processImpactVesselPart(vessel, part);
                    }
                } else {
                    for (size_t j = 0; j < vessel->protoVessel->protoPartSnapshots.size(); j++) {
                        ProtoPartSnapshot& pps = *vessel->protoVessel->protoPartSnapshots[j];
                        if (SeismicListContains(pps.persistentId)) continue;
                        auto d = std::make_shared<SeismicPartData>();
                        d->partId = pps.persistentId;
                        d->mass = pps.mass;
                        for (size_t k = 0; k < pps.resources.size(); k++) {
                            ProtoPartResourceSnapshot* prs = pps.resources[k].get();
                            if (prs != nullptr && prs->definition != nullptr)
                                d->resourceMass = f32store(fx(d->resourceMass) + fx(f32store(prs->amount * static_cast<double>(prs->definition->density()))));
                        }
                        d->mass = f32store(fx(d->mass) + fx(d->resourceMass));
                        d->velocity = seismicEventVelocity;
                        SetControllerVessel();
                        if (ControllerVessel != nullptr) {
                            d->impactDistance = Contracts::Lite::CelestialUtilities::GreatCircleDistance(vessel->mainBody(), vessel->latitude, vessel->longitude,
                                                                                      ControllerVessel->latitude, ControllerVessel->longitude);
                            const double num3 = d->impactDistance / vessel->mainBody()->Radius;
                            d->impactDistanceMultiplier = distanceCurve.Evaluate(static_cast<float>(num3));
                        } else {
                            d->impactDistance = vessel->mainBody()->Radius;
                        }
                        seismicPartList.push_back(d);
                    }
                }
                if (!seismicEventDataSendPending) {
                    seismicEventDataSendPending = true;
                    StartCoroutine(CallbackUtil::DelayedCallback(DeployedScience::Instance->SeismicScienceProcessingDelay,
                                                                 Callback<>::Lambda(this, [this] { ProcessSeismicEvent(); })));
                }
            }
            LastScienceGeneratedUT = UT();
        } else {
            const std::string bodyText = Cluster->DeployedBody != nullptr ? Cluster->DeployedBody->bodyDisplayName : Localizer::Format("#autoLOC_8100159");
            if (!Cluster->IsPowered()) PostMsg(Localizer::Format("#autoLOC_8002371", {bodyText}));
            if (UnityNull(sciencePart) || !sciencePart->Enabled()) PostMsg(Localizer::Format("#autoLOC_8002372", {bodyText}));
        }
    } else {
        log::Info("[DeployedScienceExperiment]: Cluster Cannot calculate and send Seismic Data event - Invalid Call Parameters.");
    }
}

// DeployedScienceExperiment.cs:534
void DeployedScienceExperiment::processImpactVesselPart(Vessel* vessel, Part* part) {
    if (!SeismicListContains(part->fields.persistentId)) {
        auto d = std::make_shared<SeismicPartData>();
        d->partId = part->fields.persistentId;
        std::optional<float> rbv;
        if (part->hasRigidbody && SerenityEnvironment::partRbVelocityMagnitude) rbv = SerenityEnvironment::partRbVelocityMagnitude(*part);
        d->velocity = rbv ? *rbv : seismicEventVelocity;
        d->resourceMass = part->GetResourceMass();
        d->mass = f32store(fx(part->fields.mass) + fx(d->resourceMass));
        SetControllerVessel();
        if (ControllerVessel != nullptr) {
            d->impactDistance = Contracts::Lite::CelestialUtilities::GreatCircleDistance(vessel->mainBody(), vessel->latitude, vessel->longitude, ControllerVessel->latitude,
                                                                      ControllerVessel->longitude);
            const double num = d->impactDistance / vessel->mainBody()->Radius;
            d->impactDistanceMultiplier = Mathf::Clamp01(distanceCurve.Evaluate(static_cast<float>(num)));
        } else {
            d->impactDistance = vessel->mainBody()->Radius;
        }
        seismicPartList.push_back(d);
    }
}

// DeployedScienceExperiment.cs:558
bool DeployedScienceExperiment::SeismicListContains(uint32_t pid) {
    size_t num = 0;
    while (true) {
        if (num < seismicPartList.size()) {
            if (seismicPartList[num]->partId == pid) break;
            num++;
            continue;
        }
        return false;
    }
    return true;
}

// DeployedScienceExperiment.cs:577
void DeployedScienceExperiment::ProcessSeismicEvent() {
    float num = 0.0f;
    totalJoules = 0.0;
    double num2 = 0.0;
    distanceMultiplier = 1.0;
    std::vector<std::shared_ptr<SeismicPartData>> list(seismicPartList);
    for (size_t i = 0; i < list.size(); i++) {
        totalJoules += 0.5 * (fx(list[i]->mass) * 1000.0) * dotnet::Math::Pow(static_cast<double>(list[i]->velocity), 2.0);
        num = f32store(fx(num) + fx(list[i]->mass));
        num2 = list[i]->impactDistance;
        distanceMultiplier = static_cast<double>(list[i]->impactDistanceMultiplier);
    }
    (void)num2;
    impactRate = 0.0f;
    float rate = 0.0f;
    if (!DeployedScience::Instance->SeismicEnergyRates().TryGetValue(Cluster->DeployedBody->name(), rate))
        impactRate = std::numeric_limits<float>::max();
    else
        impactRate = rate;
    if (totalJoules < static_cast<double>(DeployedScience::Instance->MinimumSeismicEnergyRequired)) {
        log::Info("[DeployedScienceExperiment]: Ignoring Impact less than " + dotnet::FormatSingleG(DeployedScience::Instance->MinimumSeismicEnergyRequired) +
                  "Joules.");
        return;
    }
    // float num3 = DimRate * solarMultiplier * dataScale * scienceLimit * ((float)totalJoules / impactRate) * (ScienceModifierRate / 100f);
    float num3 = f32store(fx(ScienceDiminishingModifierRate()) * fx(solarMultiplier) * fx(subject->dataScale) * fx(scienceLimit) *
                          (fx(f32store(totalJoules)) / fx(impactRate)) * (fx(ScienceModifierRate()) / 100.0));
    num3 = f32store(fx(num3) * fx(f32store(distanceMultiplier)));
    if (!seismicDataCalcOverride.IsNull()) {
        const float num4 = seismicDataCalcOverride(this, f32store(totalJoules), impactRate, seismicPartList);
        num3 = num4;
    }
    if (fx(TotalScienceGenerated()) + fx(num3) > fx(scienceLimit)) num3 = f32store(fx(scienceLimit) - fx(totalScienceGenerated));
    totalScienceGenerated = f32store(fx(totalScienceGenerated) + fx(num3));
    storedScienceData = f32store(fx(storedScienceData) + fx(num3) / fx(subject->subjectValue));
    GE::onGroundScienceGenerated.Fire(this, sciencePart, Cluster, num3);
    const bool flag = SendDataToComms();
    for (size_t j = 0; j < list.size(); j++) RemoveFirst(seismicPartList, list[j]);
    if (flag && seismicPartList.empty()) seismicEventDataSendPending = false;
    if (!seismicPartList.empty()) {
        StartCoroutine(CallbackUtil::DelayedCallback(DeployedScience::Instance->SeismicScienceProcessingDelay,
                                                     Callback<>::Lambda(this, [this] { ProcessSeismicEvent(); })));
    }
}

// DeployedScienceExperiment.cs:638
bool DeployedScienceExperiment::TimeToSendStoredData() {
    if (experimentId == DeployedScience::SeismicExperimentId) return true;
    if (subject == nullptr) return false;
    if (totalScienceGenerated >= scienceLimit) return true;
    if (fx(StoredScienceData()) * fx(subject->subjectValue) / fx(ScienceLimit()) * 100.0 >= 10.0) return true;
    return false;
}

// DeployedScienceExperiment.cs:659
bool DeployedScienceExperiment::GatherScienceData(std::shared_ptr<ScienceData>& experimentData) {
    experimentData = nullptr;
    if (Experiment != nullptr && ExperimentVessel != nullptr && subject != nullptr && !UnityNull(Cluster) && !(storedScienceData <= 0.0f) &&
        ExperimentSituationValid() && !UnityNull(sciencePart)) {
        experimentData = std::make_shared<ScienceData>(StoredScienceData(), 1.0f, xmitDataScalar, 0.0f, subject->id, subject->title);
        storedScienceData = 0.0f;
        return true;
    }
    if (Experiment == nullptr || UnityNull(sciencePart) || (ExperimentVessel == nullptr && ExperimentSituationValid()))
        log::Warning("[DeployedScienceExperiment]: Unable to take data from experiment " + ExperimentName() + " part Id " + std::to_string(PartId()) +
                     " as Experiment, Science Part or Vessel reference are null.");
    return false;
}

// DeployedScienceExperiment.cs:675
bool DeployedScienceExperiment::SendDataToComms() {
    if (Experiment != nullptr && ExperimentVessel != nullptr && subject != nullptr && !UnityNull(Cluster) && sciencePart->Enabled() &&
        !(storedScienceData <= 0.0f) && ExperimentSituationValid()) {
        const bool commNetEnabled = CommNet::CommNetScenario::CommNetEnabled();
        if (commNetEnabled && ControllerVessel == nullptr && Cluster != nullptr) SetControllerVessel();
        const bool commOk = ControllerVessel != nullptr && ControllerVessel->connection != nullptr && ControllerVessel->connection->SignalStrengthValue() > 0.0 &&
                            ControllerVessel->connection->ControlPath().IsLastHopHome();
        if ((Cluster->IsPowered() && !commNetEnabled) || commOk) {
            float num = 0.0f;
            const std::string homeName = HomeBody()->bodyDisplayName;
            const auto seismicText = [&](const char* tag, float shown) {
                return Localizer::Format(tag, {subject->title, dotnet::FormatSingleF(shown, 3),
                                               impactRate > 0.0f ? dotnet::FormatDoubleF(totalJoules / static_cast<double>(impactRate) * 100.0, 0) : std::string("0"),
                                               dotnet::FormatDoubleF(distanceMultiplier * 100.0, 0), dotnet::FormatSingleF(scienceModifierRate, 0),
                                               dotnet::FormatSingleF(shown, 3), homeName});
            };
            if (ResearchAndDevelopment::Instance != nullptr) {
                num = ResearchAndDevelopment::Instance->SubmitScienceData(storedScienceData, *subject);
                if (num > 0.0f) {
                    if (ExperimentId() == DeployedScience::SeismicExperimentId) {
                        PostMsg(seismicText("#autoLOC_8002337", num));
                    } else {
                        PostMsg(Localizer::Format("#autoLOC_8002254", {subject->title, dotnet::FormatSingleF(num, 3), homeName}));
                    }
                    transmittedScienceData = f32store(fx(transmittedScienceData) + fx(storedScienceData) * fx(subject->subjectValue));
                }
            } else if (ExperimentId() == DeployedScience::SeismicExperimentId) {
                PostMsg(seismicText("#autoLOC_8002373", num));
            }
            storedScienceData = 0.0f;
            totalJoules = 0.0;
            impactRate = 0.0f;
            noCommNetMsgPosted = false;
            GE::onGroundScienceTransmitted.Fire(this, sciencePart, Cluster, num);
            return true;
        }
        const float num2 = ResearchAndDevelopment::GetScienceValue(StoredScienceData(), *subject);
        if (num2 > 0.0f) {
            if (Cluster->IsPowered()) {
                if (!noCommNetMsgPosted) {
                    if (ExperimentId() == DeployedScience::SeismicExperimentId) {
                        PostMsg(Localizer::Format(
                            "#autoLOC_8002338",
                            {subject->title, dotnet::FormatSingleF(num2, 1),
                             impactRate > 0.0f ? dotnet::FormatDoubleF(totalJoules / static_cast<double>(impactRate) * 100.0, 0) : std::string("0"),
                             dotnet::FormatDoubleF(distanceMultiplier * 100.0, 0), dotnet::FormatSingleF(scienceModifierRate, 0), dotnet::FormatSingleF(num2, 3)}));
                    } else {
                        PostMsg(Localizer::Format("#autoLOC_8002255", {ExperimentVessel->mainBody()->displayName()}));
                    }
                    noCommNetMsgPosted = true;
                }
            } else if (!noPowerMsgPosted) {
                PostMsg(Localizer::Format("#autoLOC_8002256", {ExperimentVessel->mainBody()->displayName()}));
                noPowerMsgPosted = true;
            }
        }
        return false;
    }
    if (Experiment == nullptr || (ExperimentVessel == nullptr && ExperimentSituationValid()))
        log::Warning("[DeployedScienceExperiment]: Unable to send data from experiment " + ExperimentName() + " part Id " + std::to_string(PartId()) +
                     " as Experiment or Vessel reference are null.");
    return false;
}

// DeployedScienceExperiment.cs:762
void DeployedScienceExperiment::SetControllerVessel() {
    Part* partout = FindLoadedPart(Cluster->ControlModulePartId());
    if (partout != nullptr) {
        ControllerVessel = partout->vessel;
        return;
    }
    ProtoPartSnapshot* partout2 = FindUnloadedPart(Cluster->ControlModulePartId());
    if (partout2 != nullptr && partout2->pVesselRef != nullptr) ControllerVessel = partout2->pVesselRef->vesselRef;
}

// DeployedScienceExperiment.cs:777
void DeployedScienceExperiment::Load(const ConfigNode& node) {
    node.TryGetValue("PartId", partId);
    node.TryGetValue("ExperimentName", experimentName);
    node.TryGetValue("ExperimentId", experimentId);
    node.TryGetValue("TotalScienceGenerated", totalScienceGenerated);
    node.TryGetValue("StoredScienceData", storedScienceData);
    node.TryGetValue("TransmittedScienceData", transmittedScienceData);
    node.TryGetValue("ScienceLimit", scienceLimit);
    node.TryGetValue("ScienceValue", scienceValue);
    node.TryGetValue("ScienceModifierRate", scienceModifierRate);
    node.TryGetValue("ScienceDiminishingModifierRate", scienceDiminishingModifierRate);
    node.TryGetValue("LastScienceGeneratedUT", LastScienceGeneratedUT);
    vesselID = Guid::Empty();
    node.TryGetValue("VesselId", vesselID);
    node.TryGetValue("noPowerMsgPosted", noPowerMsgPosted);
    node.TryGetValue("noCommNetMsgPosted", noCommNetMsgPosted);
    node.TryGetValue("totalJoules", totalJoules);
    node.TryGetValue("impactRate", impactRate);
    if (node.HasNode("distanceCurve")) distanceCurve.Load(*node.GetNode("distanceCurve"));
}

// DeployedScienceExperiment.cs:802
void DeployedScienceExperiment::Save(ConfigNode& node) {
    node.AddValue("PartId", PartId());
    if (!ExperimentName().empty()) node.AddValue("ExperimentName", ExperimentName());
    if (!ExperimentId().empty()) node.AddValue("ExperimentId", ExperimentId());
    node.AddValue("TotalScienceGenerated", TotalScienceGenerated());
    node.AddValue("StoredScienceData", StoredScienceData());
    node.AddValue("TransmittedScienceData", TransmittedScienceData());
    node.AddValue("ScienceLimit", ScienceLimit());
    node.AddValue("ScienceValue", ScienceValue());
    node.AddValue("ScienceModifierRate", ScienceModifierRate());
    node.AddValue("ScienceDiminishingModifierRate", ScienceDiminishingModifierRate());
    node.AddValue("LastScienceGeneratedUT", LastScienceGeneratedUT);
    node.AddValue("noPowerMsgPosted", noPowerMsgPosted);
    node.AddValue("noCommNetMsgPosted", noCommNetMsgPosted);
    if (totalJoules > 0.0) node.AddValue("totalJoules", totalJoules);
    if (impactRate > 0.0f) node.AddValue("impactRate", impactRate);
    if (ExperimentVessel != nullptr) node.AddValue("VesselId", ExperimentVessel->id.ToString());
    ConfigNode* node2 = node.AddNode("distanceCurve");
    distanceCurve.Save(*node2);
}

// =============================================================================================================================
// DeployedScienceCluster

// DeployedScienceCluster.cs:100
DeployedScienceCluster* DeployedScienceCluster::Spawn() {
    DeployedScienceCluster* c = SerenityBehaviour::Add(std::shared_ptr<DeployedScienceCluster>(new DeployedScienceCluster()));
    c->Awake();
    c->lastScienceGeneratedUT = UT();
    c->controllerPartEnabled = true;
    return c;
}

// DeployedScienceCluster.cs:113
DeployedScienceCluster* DeployedScienceCluster::Spawn(ModuleGroundExpControl* controlUnit) {
    if (controlUnit == nullptr) return nullptr;
    DeployedScienceCluster* cluster = Spawn();
    cluster->controlModulePartId = controlUnit->ControlUnitId();
    Part& cp = static_cast<Part&>(*controlUnit->part);
    if (cp.vessel != nullptr) cluster->DeployedBody = cp.vessel->mainBody();
    ModuleGroundSciencePart* component = FindImpl<ModuleGroundSciencePart>(cp);
    if (component != nullptr) {
        DeployedSciencePart* dsp = DeployedSciencePart::Spawn(component, controlUnit, cluster);
        cluster->DeployedScienceParts.push_back(dsp);
        if (component->IsSolarPanel()) cluster->solarPanelParts.push_back(dsp);
        if (dsp->IsAntenna()) AddUnique(cluster->antennaParts, dsp);
    }
    return cluster;
}

// DeployedScienceCluster.cs:143
DeployedScienceCluster* DeployedScienceCluster::SpawnandLoad(const ConfigNode& node) {
    DeployedScienceCluster* c = Spawn();
    c->Load(node);
    c->UpdatePowerState();
    return c;
}

DeployedScienceCluster::~DeployedScienceCluster() {
    GE::onGroundScienceControllerChanged.Remove(this, &DeployedScienceCluster::UpdateCluster);
    GE::onGroundSciencePartRemoved.Remove(this, &DeployedScienceCluster::GroundScienceModuleRemoved);
    GE::onGroundSciencePartEnabledStateChanged.Remove(this, &DeployedScienceCluster::GroundSciencePartEnabledStateChanged);
}

// DeployedScienceCluster.cs:152
void DeployedScienceCluster::Awake() {
    if (!SerenityInstalled()) destroyed = true;
}

// DeployedScienceCluster.cs:160
void DeployedScienceCluster::Start() {
    GE::onGroundScienceControllerChanged.Add(this, &DeployedScienceCluster::UpdateCluster);
    GE::onGroundSciencePartRemoved.Add(this, &DeployedScienceCluster::GroundScienceModuleRemoved);
    GE::onGroundSciencePartEnabledStateChanged.Add(this, &DeployedScienceCluster::GroundSciencePartEnabledStateChanged);
}

// DeployedScienceCluster.cs:167
void DeployedScienceCluster::OnDestroy() {
    GE::onGroundScienceControllerChanged.Remove(this, &DeployedScienceCluster::UpdateCluster);
    GE::onGroundSciencePartRemoved.Remove(this, &DeployedScienceCluster::GroundScienceModuleRemoved);
    GE::onGroundSciencePartEnabledStateChanged.Remove(this, &DeployedScienceCluster::GroundSciencePartEnabledStateChanged);
    for (size_t i = 0; i < DeployedScienceParts.size(); i++) DeployedScienceParts[i]->DestroyGameObject();
}

// DeployedScienceCluster.cs:178
int DeployedScienceCluster::SolarPanelUnitsProduced() {
    int num = 0;
    for (size_t i = 0; i < solarPanelParts.size(); i++) num += solarPanelParts[i]->PowerUnitsProduced();
    return num;
}

// DeployedScienceCluster.cs:188
void DeployedScienceCluster::UpdatePowerState() {
    powerRequired = 0;
    powerAvailable = 0;
    bool flag = false;
    bool flag2 = isPowered;
    for (size_t i = 0; i < DeployedScienceParts.size(); i++) {
        DeployedSciencePart* dsp = DeployedScienceParts[i];
        if (dsp->Enabled() && dsp->DeployedOnGround()) {
            Part* part = dsp->PartIsLoaded();
            if (part != nullptr) flag = true;
            if (dsp->IsSolarPanel()) {
                if (part != nullptr)
                    powerAvailable += dsp->ActualPowerUnitsProduced();
                else
                    powerAvailable += dsp->PowerUnitsProduced();
            } else {
                powerAvailable += dsp->PowerUnitsProduced();
            }
            powerRequired += dsp->PowerUnitsRequired();
        }
        if (dsp->PartId() == ControlModulePartId()) controllerPartEnabled = dsp->Enabled();
    }
    flag2 = controllerPartEnabled;
    if (flag) {
        if (powerAvailable >= powerRequired) {
            partialPower = false;
            partialPowerMultiplier = 1.0f;
        } else {
            partialPower = true;
            partialPowerMultiplier = 0.0f;
            flag2 = false;
        }
    } else if (PowerAvailable() < PowerRequired()) {
        partialPower = true;
        partialPowerMultiplier = 0.0f;
        flag2 = false;
    } else if (PowerAvailable() - SolarPanelUnitsProduced() < PowerRequired()) {
        partialPower = true;
        partialPowerMultiplier = 0.5f;
    } else {
        partialPower = false;
        partialPowerMultiplier = 1.0f;
    }
    if (flag2 != IsPowered()) {
        isPowered = flag2;
        GE::onGroundScienceClusterPowerStateChanged.Fire(this);
    }
}

// DeployedScienceCluster.cs:263
void DeployedScienceCluster::UpdateCluster(ModuleGroundExpControl* controlUnit, bool replaceList, std::vector<ModuleGroundSciencePart*>* scienceParts) {
    if (ControlModulePartId() != controlUnit->part->fields.persistentId) return;
    std::vector<ModuleGroundSciencePart*>& sp = *scienceParts;
    bool flag = false;
    for (size_t i = 0; i < sp.size(); i++) {
        if (sp[i]->part->fields.persistentId == controlUnit->part->fields.persistentId) {
            flag = true;
            break;
        }
    }
    if (!flag) sp.push_back(controlUnit);
    bool flag2 = false;
    for (size_t j = 0; j < sp.size(); j++) {
        DeployedSciencePart* dsp = DeployedScienceExtensions::Get(DeployedScienceParts, sp[j]->part->fields.persistentId);
        if (dsp == nullptr) {
            dsp = DeployedSciencePart::Spawn(sp[j], controlUnit, this);
            DeployedScienceParts.push_back(dsp);
            if (dsp->IsSolarPanel()) solarPanelParts.push_back(dsp);
            if (dsp->Experiment != nullptr) flag2 = true;
            if (dsp->IsAntenna()) AddUnique(antennaParts, dsp);
        } else {
            dsp->UpdateSciencePart(sp[j]);
        }
    }
    bool flag3 = false;
    if (replaceList) {
        int count = static_cast<int>(DeployedScienceParts.size());
        while (count-- > 0) {
            DeployedSciencePart* dsp = DeployedScienceParts[static_cast<size_t>(count)];
            if (DeployedScienceExtensions::Get(sp, dsp->PartId()) == nullptr) {
                if (Contains(solarPanelParts, dsp)) RemoveFirst(solarPanelParts, dsp);
                if (Contains(antennaParts, dsp)) RemoveFirst(antennaParts, dsp);
                if (dsp->Experiment != nullptr) flag3 = true;
                dsp->DestroyGameObject();
                DeployedScienceParts.erase(DeployedScienceParts.begin() + count);
            }
        }
    }
    UpdatePowerState();
    if (flag2 || flag3) UpdateExperimentDiminishReturns();
    GE::onGroundScienceClusterUpdated.Fire(controlUnit, this);
}

// DeployedScienceCluster.cs:342
void DeployedScienceCluster::UpdateExperimentDiminishReturns() {
    for (size_t i = 0; i < DeployedScienceParts.size(); i++)
        if (DeployedScienceParts[i]->Experiment != nullptr)
            DeployedScience::Instance->SetDiminishingScienceRate(DeployedScienceParts[i]->Experiment->ExperimentId(), DeployedBody->name());
}

// DeployedScienceCluster.cs:353
void DeployedScienceCluster::GroundSciencePartEnabledStateChanged(ModuleGroundSciencePart* sciencePart) {
    if (sciencePart->ControlUnitId() == ControlModulePartId()) {
        if (sciencePart->part->fields.persistentId == ControlModulePartId()) {
            controllerPartEnabled = sciencePart->Enabled() && sciencePart->DeployedOnGround();
            UpdatePowerState();
        }
        ModuleGroundExperiment* component = FindImpl<ModuleGroundExperiment>(static_cast<Part&>(*sciencePart->part));
        if (component != nullptr)
            DeployedScience::Instance->SetDiminishingScienceRate(component->experimentId(), static_cast<Part&>(*sciencePart->part).vessel->mainBody()->name());
    }
}

// DeployedScienceCluster.cs:370
void DeployedScienceCluster::GroundScienceModuleRemoved(ModuleGroundSciencePart* part) {
    DeployedSciencePart* dsp = DeployedScienceExtensions::Get(DeployedScienceParts, part->part->fields.persistentId);
    if (dsp != nullptr) {
        if (Contains(solarPanelParts, dsp)) RemoveFirst(solarPanelParts, dsp);
        if (Contains(antennaParts, dsp)) RemoveFirst(antennaParts, dsp);
        std::string text;
        if (dsp->Experiment != nullptr) text = dsp->Experiment->ExperimentId();
        RemoveFirst(DeployedScienceParts, dsp);
        dsp->DestroyGameObject();
        UpdatePowerState();
        if (!text.empty()) DeployedScience::Instance->SetDiminishingScienceRate(text, DeployedBody->name());
    }
}

// DeployedScienceCluster.cs:398
Enumerator DeployedScienceCluster::UpdateScience() {
    if (IsPowered()) {
        for (size_t i = 0; i < DeployedScienceParts.size(); i++) {
            if (DeployedScienceParts[i]->Experiment != nullptr) {
                if (DeployedScienceParts[i]->Enabled() && DeployedScienceParts[i]->Experiment->ExperimentId() != DeployedScience::SeismicExperimentId)
                    DeployedScienceParts[i]->Experiment->CalculateScience(partialPowerMultiplier);
                if (DeployedScienceParts[i]->Experiment->StoredScienceData() > 0.0f && DeployedScienceParts[i]->Experiment->TimeToSendStoredData())
                    dataSendFailed = !DeployedScienceParts[i]->Experiment->SendDataToComms();
                lastScienceTransmittedUT = UT();
                co_yield YieldInstruction::Null();
            }
        }
    } else {
        const double universalTime = UT();
        for (size_t j = 0; j < DeployedScienceParts.size(); j++)
            if (DeployedScienceParts[j]->Experiment != nullptr) DeployedScienceParts[j]->Experiment->LastScienceGeneratedUT = universalTime;
    }
    lastScienceGeneratedUT = UT();
    updatingScience = false;
    co_yield YieldInstruction::Null();
}

// DeployedScienceCluster.cs:435
void DeployedScienceCluster::Load(const ConfigNode& node) {
    node.TryGetValue("ControlModulePartId", controlModulePartId);
    std::string value;
    node.TryGetValue("DeployedBody", value);
    if (!value.empty()) DeployedBody = BodyByName(value);
    node.TryGetValue("ControlModuleEnabled", controllerPartEnabled);
    node.TryGetValue("LastScienceGeneratedUT", lastScienceGeneratedUT);
    ConfigNode* node2 = nullptr;
    if (!node.TryGetNode("MANNEDSCIENCEPARTS", node2)) return;
    std::vector<ConfigNode*> nodes = node2->GetNodes("MANNEDSCIENCEPART");
    for (size_t i = 0; i < nodes.size(); i++) {
        DeployedSciencePart* dsp = DeployedSciencePart::SpawnandLoad(*nodes[i], this);
        DeployedScienceParts.push_back(dsp);
        if (dsp->IsSolarPanel()) solarPanelParts.push_back(dsp);
        if (dsp->IsAntenna() && dsp->PartId() != ControlModulePartId()) AddUnique(antennaParts, dsp);
    }
}

// DeployedScienceCluster.cs:467
void DeployedScienceCluster::Save(ConfigNode& node) {
    node.AddValue("ControlModulePartId", ControlModulePartId());
    if (DeployedBody != nullptr) node.AddValue("DeployedBody", DeployedBody->name());
    node.AddValue("ControlModuleEnabled", ControllerPartEnabled());
    node.AddValue("LastScienceGeneratedUT", LastScienceGeneratedUT());
    ConfigNode* configNode = node.AddNode("MANNEDSCIENCEPARTS");
    for (size_t i = 0; i < DeployedScienceParts.size(); i++) {
        ConfigNode* node2 = configNode->AddNode("MANNEDSCIENCEPART");
        DeployedScienceParts[i]->Save(*node2);
    }
}

// =============================================================================================================================
// DeployedScience (the ScenarioModule)

DeployedScience::DeployedScience() : ScenarioModule("DeployedScience") {}

DeployedScience::~DeployedScience() {
    // OnDestroy (:89)
    if (Instance != nullptr && Instance == this) Instance = nullptr;
    GE::onGroundScienceRegisterCluster.Remove(this, &DeployedScience::RegisterCluster);
    GE::onGroundScienceDeregisterCluster.Remove(this, &DeployedScience::DeRegisterCluster);
    GE::onGroundScienceControllerChanged.Remove(this, &DeployedScience::UpdateCluster);
    GE::onVesselRecovered.Remove(this, &DeployedScience::OnVesselRecovered);
    GE::onVesselTerminated.Remove(this, &DeployedScience::OnVesselTerminated);
    GE::onVesselWillDestroy.Remove(this, &DeployedScience::OnVesselWillDestroy);
    GE::onVesselExplodeGroundCollision.Remove(this, &DeployedScience::OnVesselExplodeGroundCollision);
    GE::onGameStatePostLoad.Remove(this, &DeployedScience::OnGameStateLoad);
    GE::onGameStateLoad.Remove(this, &DeployedScience::OnGameStateLoad);
    GE::onPartExplodeGroundCollision.Remove(this, &DeployedScience::OnPartExplodeGroundCollision);
    RemoveMannedScienceObjects();
}

void DeployedScience::Register() {
    ScenarioModuleFactory::Register("DeployedScience", [](const std::string&) { return std::unique_ptr<ScenarioModule>(new DeployedScience()); });
}

// DeployedScience.cs:57
void DeployedScience::OnAwake() {
    if (!SerenityInstalled()) {
        if (Instance != nullptr) Instance->destroyed = true;  // Object.Destroy(Instance)
        destroyed = true;                                    // Object.Destroy(this)
        return;
    }
    if (Instance != nullptr && Instance != this) {
        log::Error("[DeployedScience Module]: Instance already exists!");
        destroyed = true;
        return;
    }
    Instance = this;
    IsActive = false;
    deployedScienceClusters = DictionaryValueList<uint32_t, DeployedScienceCluster*>();
    GE::onGroundScienceRegisterCluster.Add(this, &DeployedScience::RegisterCluster);
    GE::onGroundScienceDeregisterCluster.Add(this, &DeployedScience::DeRegisterCluster);
    GE::onGroundScienceControllerChanged.Add(this, &DeployedScience::UpdateCluster);
    GE::onVesselRecovered.Add(this, &DeployedScience::OnVesselRecovered);
    GE::onVesselTerminated.Add(this, &DeployedScience::OnVesselTerminated);
    GE::onVesselWillDestroy.Add(this, &DeployedScience::OnVesselWillDestroy);
    GE::onPartExplodeGroundCollision.Add(this, &DeployedScience::OnPartExplodeGroundCollision);
    GE::onVesselExplodeGroundCollision.Add(this, &DeployedScience::OnVesselExplodeGroundCollision);
    GE::onGameStatePostLoad.Add(this, &DeployedScience::OnGameStateLoad);
    GE::onGameStateLoad.Add(this, &DeployedScience::OnGameStateLoad);
}

// DeployedScience.cs:451
void DeployedScience::RemoveMannedScienceObjects() {
    // the children of DeployedScienceGameObject that carry a DeployedScienceCluster are destroyed
    // (C# walks the live children of the game object: a cluster whose Object.Destroy already ran is no longer a child, here it is gone from the registry)
    for (int i = deployedScienceClusters.Count() - 1; i >= 0; i--) {
        DeployedScienceCluster* cluster = deployedScienceClusters.At(i);
        if (SerenityBehaviour::IsLive(cluster)) cluster->DestroyGameObject();
    }
}

// DeployedScience.cs:108
void DeployedScience::FixedUpdate() {
    SerenityBehaviour::RunPendingStarts();
    for (size_t i = 0; i < deployedScienceClusters.ValuesList().size(); i++) {
        DeployedScienceCluster* cluster = deployedScienceClusters.ValuesList()[i];
        if (UnityNull(cluster)) continue;
        bool flag = false;
        if (cluster->UpdatingScience()) continue;
        const double rate = static_cast<double>(TimeWarp::CurrentRate());
        if (cluster->DataSendFailed()) {
            if ((UT() - cluster->LastScienceTransmittedUT()) * rate > DataSendFailedTimeDelay * rate) flag = true;
        } else if ((UT() - cluster->LastScienceGeneratedUT()) * rate > ScienceTimeDelay * rate) {
            flag = true;
        }
        if (flag && !cluster->UpdatingScience()) {
            cluster->SetUpdatingScience(true);
            ScienceEnvironment::Scheduler().StartCoroutine(cluster->UpdateScience(), static_cast<int>(reinterpret_cast<uintptr_t>(this) >> 4));
        }
    }
    // the DeployedScienceExperiment.FixedUpdate messages of the live components
    for (size_t i = 0; i < deployedScienceClusters.ValuesList().size(); i++) {
        DeployedScienceCluster* cluster = deployedScienceClusters.ValuesList()[i];
        if (UnityNull(cluster)) continue;
        for (size_t j = 0; j < cluster->DeployedScienceParts.size(); j++) {
            DeployedSciencePart* dsp = cluster->DeployedScienceParts[j];
            if (!UnityNull(dsp) && !UnityNull(dsp->Experiment)) dsp->Experiment->FixedUpdate();
        }
    }
}

// DeployedScience.cs:141
void DeployedScience::UpdateSciencePart(ModuleGroundSciencePart* sciencePart) {
    ModuleGroundExperiment* component = FindImpl<ModuleGroundExperiment>(static_cast<Part&>(*sciencePart->part));
    if (component != nullptr) SetDiminishingScienceRate(component->experimentId(), static_cast<Part&>(*sciencePart->part).vessel->mainBody()->name());
}

// DeployedScience.cs:150
void DeployedScience::UpdateCluster(ModuleGroundExpControl* controlUnit, bool, std::vector<ModuleGroundSciencePart*>* deployableScienceParts) {
    if (!deployedScienceClusters.ContainsKey(controlUnit->part->fields.persistentId)) RegisterCluster(controlUnit, deployableScienceParts);
}

// DeployedScience.cs:158
void DeployedScience::OnVesselRecovered(ProtoVessel* vessel, bool) {
    for (size_t i = 0; i < vessel->protoPartSnapshots.size(); i++)
        if (deployedScienceClusters.ContainsKey(vessel->protoPartSnapshots[i]->persistentId)) DeRegisterCluster(vessel->protoPartSnapshots[i]->persistentId);
}

// DeployedScience.cs:169
void DeployedScience::OnVesselTerminated(ProtoVessel* vessel) { OnVesselRecovered(vessel, false); }

// DeployedScience.cs:174
void DeployedScience::OnVesselWillDestroy(Vessel* vessel) {
    for (size_t i = 0; i < vessel->parts.size(); i++)
        if (deployedScienceClusters.ContainsKey(vessel->parts[i]->fields.persistentId)) DeRegisterCluster(vessel->parts[i]->fields.persistentId);
}

// DeployedScience.cs:185
void DeployedScience::OnPartExplodeGroundCollision(Part* part) {
    if (part->vessel != nullptr) {
        if (part->vessel->parts.empty()) part->vessel->parts.push_back(part);
        ScanAndProcessSeismicEvent(part->vessel, part);
    } else {
        log::Info("[DeployedScience]: Part " + part->partInfo->title + " " + std::to_string(part->fields.persistentId) +
                  " Exploded on Ground collision, but Vessel was null. Unable to process.");
    }
}

// DeployedScience.cs:201
void DeployedScience::OnVesselExplodeGroundCollision(Vessel* vessel) {
    if (vessel != nullptr && vessel->mainBody() != nullptr)
        ScanAndProcessSeismicEvent(vessel, nullptr);
    else
        log::Info("[DeployedScience]: Seismic Event Cannot be processed - no Vessel data.");
}

// DeployedScience.cs:213
void DeployedScience::ScanAndProcessSeismicEvent(Vessel* vessel, Part* part) {
    if (vessel != nullptr && vessel->mainBody() != nullptr) {
        for (size_t i = 0; i < deployedScienceClusters.ValuesList().size(); i++) {
            DeployedScienceCluster* cluster = deployedScienceClusters.ValuesList()[i];
            if (cluster->DeployedBody->name() == vessel->mainBody()->name()) {
                DeployedSciencePart* experiment = DeployedScienceExtensions::GetExperiment(cluster->DeployedScienceParts, SeismicExperimentId);
                if (experiment != nullptr && experiment->Experiment != nullptr)
                    experiment->Experiment->CalcSendSeismicScience(vessel, part, cluster->PartialPowerMultiplier());
                else if (experiment != nullptr)
                    log::Warning("[DeployedScience]: CalcSeismicScience - unable to calculate science. DeployedSciencePart Experiment is null.");
            }
        }
    } else {
        log::Info("[DeployedScience]: Seismic Event Cannot be processed - no Vessel data.");
    }
}

// DeployedScience.cs:239
void DeployedScience::OnGameStateLoad(ConfigNode*) {
    for (int num = deployedScienceClusters.Count() - 1; num >= 0; num--) {
        DeployedScienceCluster* cluster = deployedScienceClusters.ValuesList()[static_cast<size_t>(num)];
        if (cluster != nullptr) {
            if (FindLoadedPart(cluster->ControlModulePartId()) == nullptr && FindUnloadedPart(cluster->ControlModulePartId()) == nullptr) {
                log::Info("[DeployedScience]: Unable to find Manned Science controller with Id " + std::to_string(cluster->ControlModulePartId()) +
                          " in current Game. Removed.");
                cluster->DestroyGameObject();
                deployedScienceClusters.Remove(deployedScienceClusters.KeysList()[static_cast<size_t>(num)]);
            } else {
                for (int num2 = static_cast<int>(cluster->DeployedScienceParts.size()) - 1; num2 >= 0; num2--) {
                    DeployedSciencePart* dsp = cluster->DeployedScienceParts[static_cast<size_t>(num2)];
                    if (FindLoadedPart(dsp->PartId()) == nullptr && FindUnloadedPart(dsp->PartId()) == nullptr) {
                        if (Contains(cluster->SolarPanelParts(), dsp)) RemoveFirst(cluster->solarPanelParts, dsp);
                        if (Contains(cluster->AntennaParts(), dsp)) RemoveFirst(cluster->antennaParts, dsp);
                        dsp->DestroyGameObject();
                        cluster->DeployedScienceParts.erase(cluster->DeployedScienceParts.begin() + num2);
                    }
                }
            }
        }
    }
    IsActive = true;
}

// DeployedScience.cs:277
float DeployedScience::ExpDimReturnsRate(const std::string& experimentId, const std::string& celestialBodyName) {
    int num = 0;
    for (size_t i = 0; i < DeployedScienceClusters().ValuesList().size(); i++) {
        DeployedScienceCluster* cluster = DeployedScienceClusters().ValuesList()[i];
        if (!cluster->IsPowered()) continue;
        for (size_t j = 0; j < cluster->DeployedScienceParts.size(); j++) {
            DeployedSciencePart* dsp = cluster->DeployedScienceParts[j];
            if (dsp->Experiment != nullptr && dsp->Experiment->ExperimentVessel != nullptr && dsp->Experiment->ExperimentVessel->mainBody()->name() == celestialBodyName &&
                dsp->Experiment->ExperimentId() == experimentId)
                num++;
        }
    }
    return GetDiminishingReturnsMultiplier(num);
}

// DeployedScience.cs:299
float DeployedScience::GetDiminishingReturnsMultiplier(int numExperiments) {
    float result = 1.0f;
    bool flag = false;
    for (size_t i = 0; i < diminishingReturns.KeysList().size(); i++) {
        if (diminishingReturns.KeysList()[i] <= numExperiments) {
            if (diminishingReturns.KeysList()[i] == numExperiments) {
                flag = true;
                result = diminishingReturns.ValuesList()[i];
            } else if (!flag) {
                result = diminishingReturns.ValuesList()[i];
            }
        }
    }
    return result;
}

// DeployedScience.cs:321
DeployedSciencePart* DeployedScience::ExpMaxScienceMultiplier(const std::string& experimentId, const std::string& celestialBodyName) {
    float num = 0.0f;
    DeployedSciencePart* result = nullptr;
    for (size_t i = 0; i < DeployedScienceClusters().ValuesList().size(); i++) {
        DeployedScienceCluster* cluster = DeployedScienceClusters().ValuesList()[i];
        if (!cluster->IsPowered()) continue;
        for (size_t j = 0; j < cluster->DeployedScienceParts.size(); j++) {
            DeployedSciencePart* dsp = cluster->DeployedScienceParts[j];
            if (dsp->Experiment != nullptr && dsp->Experiment->ExperimentVessel != nullptr && dsp->Experiment->ExperimentVessel->mainBody()->name() == celestialBodyName &&
                dsp->Experiment->ExperimentId() == experimentId && dsp->Experiment->ScienceModifierRate() > num) {
                num = dsp->Experiment->ScienceModifierRate();
                result = dsp;
            }
        }
    }
    return result;
}

// DeployedScience.cs:345
void DeployedScience::SetDiminishingScienceRate(const std::string& experimentId, const std::string& celestialBodyName) {
    const float scienceDiminishingModifierRate = ExpDimReturnsRate(experimentId, celestialBodyName);
    DeployedSciencePart* best = ExpMaxScienceMultiplier(experimentId, celestialBodyName);
    if (best == nullptr) return;
    for (size_t i = 0; i < DeployedScienceClusters().ValuesList().size(); i++) {
        DeployedScienceCluster* cluster = DeployedScienceClusters().ValuesList()[i];
        if (!cluster->IsPowered()) continue;
        for (size_t j = 0; j < cluster->DeployedScienceParts.size(); j++) {
            DeployedSciencePart* dsp2 = cluster->DeployedScienceParts[j];
            if (dsp2->PartId() == best->PartId()) {
                if (dsp2->Experiment != nullptr) dsp2->Experiment->SetScienceDiminishingModifierRate(1.0f);
            } else if (dsp2->Experiment != nullptr && dsp2->Experiment->ExperimentVessel != nullptr &&
                       dsp2->Experiment->ExperimentVessel->mainBody()->name() == celestialBodyName && dsp2->Experiment->ExperimentId() == experimentId) {
                dsp2->Experiment->SetScienceDiminishingModifierRate(scienceDiminishingModifierRate);
            }
        }
    }
}

// DeployedScience.cs:378
void DeployedScience::RegisterCluster(ModuleGroundExpControl* controlUnit, std::vector<ModuleGroundSciencePart*>* deployableScienceParts) {
    if (deployedScienceClusters.ContainsKey(controlUnit->part->fields.persistentId)) {
        log::Info("[DeployedScience]: Already contains Science Cluster " + std::to_string(controlUnit->part->fields.persistentId));
        return;
    }
    DeployedScienceCluster* cluster = DeployedScienceCluster::Spawn(controlUnit);
    deployedScienceClusters.Add(controlUnit->part->fields.persistentId, cluster);
    for (size_t i = 0; i < deployableScienceParts->size(); i++)
        cluster->DeployedScienceParts.push_back(DeployedSciencePart::Spawn((*deployableScienceParts)[i], controlUnit, cluster));
    cluster->UpdatePowerState();
    cluster->UpdateExperimentDiminishReturns();
    GE::onGroundScienceClusterRegistered.Fire(controlUnit, cluster);
    GE::onGroundScienceClusterUpdated.Fire(controlUnit, cluster);
}

// DeployedScience.cs:397
void DeployedScience::DeRegisterCluster(uint32_t controlUnitId) {
    DeployedScienceCluster* val = nullptr;
    if (!deployedScienceClusters.TryGetValue(controlUnitId, val)) return;
    std::vector<std::string> experiments = DeployedScienceExtensions::GetExperiments(val->DeployedScienceParts);
    const std::string celestialBodyName = val->DeployedBody->name();
    for (size_t i = 0; i < val->DeployedScienceParts.size(); i++) {
        if (val->DeployedScienceParts[i]->PartId() == val->ControlModulePartId()) continue;
        if (val->DeployedScienceParts[i]->PartId() == 0) continue;
        Part* partout = FindLoadedPart(val->DeployedScienceParts[i]->PartId());
        if (partout != nullptr) {
            if (partout->vessel != nullptr) SetDeployedSciencePartVesselType(partout->vessel);
        } else {
            ProtoPartSnapshot* partout2 = FindUnloadedPart(val->DeployedScienceParts[i]->PartId());
            if (partout2 != nullptr && partout2->pVesselRef != nullptr && partout2->pVesselRef->vesselRef != nullptr)
                SetDeployedSciencePartVesselType(partout2->pVesselRef->vesselRef);
        }
    }
    val->DestroyGameObject();
    deployedScienceClusters.Remove(controlUnitId);
    for (size_t j = 0; j < experiments.size(); j++) SetDiminishingScienceRate(experiments[j], celestialBodyName);
}

// DeployedScience.cs:437
void DeployedScience::SetDeployedSciencePartVesselType(Vessel* vessel) {
    vessel->vesselType = VesselType::DeployedScienceController;
    GE::onVesselRename.Fire(GE::HostedFromToAction<Vessel*, std::string>(vessel, vessel->vesselName, vessel->vesselName));
    ++SerenityEnvironment::vesselMarkerRefreshes;
    ++SerenityEnvironment::mapObjectRefreshes;
}

// DeployedScience.cs:467
void DeployedScience::OnLoad(ConfigNode& node) {
    node.TryGetValue("ScienceTimeDelay", ScienceTimeDelay);
    node.TryGetValue("DataSendFailedTimeDelay", DataSendFailedTimeDelay);
    ConfigNode* node2 = nullptr;
    if (node.TryGetNode("SCIENCECLUSTERS", node2)) {
        std::vector<ConfigNode*> nodes = node2->GetNodes("SCIENCECLUSTER");
        for (size_t i = 0; i < nodes.size(); i++) {
            DeployedScienceCluster* cluster = DeployedScienceCluster::SpawnandLoad(*nodes[i]);
            deployedScienceClusters.Add(cluster->ControlModulePartId(), cluster);
        }
    }
    const auto dbNodes = [] {
        return ScienceEnvironment::getConfigNodes ? ScienceEnvironment::getConfigNodes("DEPLOYEDSCIENCE") : std::vector<ConfigNode*>();
    };
    ConfigNode* node3 = nullptr;
    if (node.TryGetNode("DIMINISHINGRETURNS", node3)) {
        SetupDiminishingReturns(node3);
    } else {
        std::vector<ConfigNode*> configNodes = dbNodes();
        if (!configNodes.empty()) {
            if (configNodes[0]->TryGetNode("DIMINISHINGRETURNS", node3))
                SetupDiminishingReturns(node3);
            else
                SetupDiminishingReturns(nullptr);
        } else {
            SetupDiminishingReturns(nullptr);
        }
    }
    ConfigNode* node4 = nullptr;
    if (node.TryGetNode("SEISMICENERGY", node4)) {
        SetupSeismicEnergy(node4);
        return;
    }
    std::vector<ConfigNode*> configNodes2 = dbNodes();
    if (!configNodes2.empty()) {
        if (configNodes2[0]->TryGetNode("SEISMICENERGY", node4))
            SetupSeismicEnergy(node4);
        else
            SetupSeismicEnergy(nullptr);
    } else {
        SetupSeismicEnergy(nullptr);
    }
}

// DeployedScience.cs:529
void DeployedScience::SetupDiminishingReturns(const ConfigNode* node) {
    diminishingReturns = DictionaryValueList<int32_t, float>();
    if (node != nullptr) {
        std::vector<ConfigNode*> nodes = node->GetNodes("ENTRY");
        for (size_t i = 0; i < nodes.size(); i++) {
            int32_t value = 0;
            float value2 = 1.0f;
            nodes[i]->TryGetValue("ExpCount", value);
            nodes[i]->TryGetValue("Rate", value2);
            if (!diminishingReturns.ContainsKey(value)) diminishingReturns.Add(value, value2);
        }
    } else {
        diminishingReturns.Add(1, 1.0f);
        diminishingReturns.Add(2, 0.2f);
        diminishingReturns.Add(3, 0.35f);
        diminishingReturns.Add(4, 0.45f);
        diminishingReturns.Add(5, 0.5f);
    }
}

// DeployedScience.cs:561
void DeployedScience::SetupSeismicEnergy(const ConfigNode* node) {
    seismicEnergyRates = DictionaryValueList<std::string, float>();
    if (node != nullptr) {
        node->TryGetValue("MinimumEnergyRequired", MinimumSeismicEnergyRequired);
        int32_t delay = SeismicScienceProcessingDelay;
        node->TryGetValue("SeismicScienceProcessingDelay", delay);
        SeismicScienceProcessingDelay = delay;
        std::vector<ConfigNode*> nodes = node->GetNodes("ENTRY");
        for (size_t i = 0; i < nodes.size(); i++) {
            std::string value;
            float value2 = 1.0f;
            nodes[i]->TryGetValue("BodyName", value);
            nodes[i]->TryGetValue("Energy", value2);
            if (!seismicEnergyRates.ContainsKey(value)) seismicEnergyRates.Add(value, value2);
        }
    } else {
        seismicEnergyRates.Add("Moho", 2.02E+09f);
        seismicEnergyRates.Add("Eve", 9.78E+10f);
        seismicEnergyRates.Add("Gilly", 1000000.0f);
        seismicEnergyRates.Add("Kerbin", 4.184E+10f);
        seismicEnergyRates.Add("Mun", 782000000.0f);
        seismicEnergyRates.Add("Minmus", 21200000.0f);
        seismicEnergyRates.Add("Duna", 3.62E+09f);
        seismicEnergyRates.Add("Ike", 223000000.0f);
        seismicEnergyRates.Add("Dres", 258000000.0f);
        seismicEnergyRates.Add("Laythe", 2.036E+10f);
        seismicEnergyRates.Add("Vall", 2.49E+09f);
        seismicEnergyRates.Add("Tylo", 3.39E+10f);
        seismicEnergyRates.Add("Bop", 29900000.0f);
        seismicEnergyRates.Add("Poll", 866000000.0f);
        seismicEnergyRates.Add("Eeloo", 894000000.0f);
    }
}

// DeployedScience.cs:605
void DeployedScience::OnSave(ConfigNode& node) {
    node.AddValue("ScienceTimeDelay", ScienceTimeDelay);
    node.AddValue("DataSendFailedTimeDelay", DataSendFailedTimeDelay);
    ConfigNode* configNode = node.AddNode("SCIENCECLUSTERS");
    for (size_t i = 0; i < deployedScienceClusters.ValuesList().size(); i++) {
        auto node2 = std::make_shared<ConfigNode>("SCIENCECLUSTER");
        deployedScienceClusters.ValuesList()[i]->Save(*node2);
        configNode->AddNode(node2);
    }
    ConfigNode* configNode2 = node.AddNode("DIMINISHINGRETURNS");
    for (size_t j = 0; j < diminishingReturns.KeysList().size(); j++) {
        ConfigNode* configNode3 = configNode2->AddNode("ENTRY");
        configNode3->AddValue("ExpCount", diminishingReturns.KeysList()[j]);
        configNode3->AddValue("Rate", diminishingReturns.ValuesList()[j]);
    }
    ConfigNode* configNode4 = node.AddNode("SEISMICENERGY");
    configNode4->AddValue("MinimumEnergyRequired", MinimumSeismicEnergyRequired);
    configNode4->AddValue("SeismicScienceProcessingDelay", SeismicScienceProcessingDelay);
    for (size_t k = 0; k < seismicEnergyRates.KeysList().size(); k++) {
        ConfigNode* configNode5 = configNode4->AddNode("ENTRY");
        configNode5->AddValue("BodyName", seismicEnergyRates.KeysList()[k]);
        configNode5->AddValue("Energy", seismicEnergyRates.ValuesList()[k]);
    }
}

}  // namespace ksp::Expansions::Serenity::DeployedScience::Runtime
