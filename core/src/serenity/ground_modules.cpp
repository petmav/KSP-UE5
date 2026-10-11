// WP-53: ModuleGroundPart.cs, ModuleGroundSciencePart.cs, ModuleGroundCommsPart.cs, ModuleGroundExpControl.cs, ModuleGroundExperiment.cs
// (see include/ksp/serenity/ground_modules.hpp for the model and the seams).
#include "ksp/serenity/ground_modules.hpp"

#include <algorithm>
#include <limits>

#include "ksp/career/format.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/commnet/comm_net_vessel.hpp"
#include "ksp/commnet/commnet_scene.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/animation_modules.hpp"
#include "ksp/modules/science_modules.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/science/science.hpp"
#include "ksp/serenity/deployed_science.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {
namespace {

using GE = events::GameEvents;
using KSP::Localization::Localizer;
using Expansions::Serenity::SerenityEnvironment;
namespace RT = Expansions::Serenity::DeployedScience::Runtime;
using RT::DeployedScience;

constexpr float kFloatMax = std::numeric_limits<float>::max();

bool SerenityInstalled() { return SaveEnvironment::Current().IsExpansionInstalled("Serenity"); }           // ExpansionsLoader.IsExpansionInstalled
bool LoadedSceneIsFlight() { return SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT; }          // HighLogic.LoadedSceneIsFlight
bool LoadedSceneIsGame() { return SaveEnvironment::Current().loadedSceneIsGame; }                            // HighLogic.LoadedSceneIsGame
Part* PartOf(const PartModule& m) { return dynamic_cast<Part*>(m.part); }
Part& NeedPart(const PartModule& m) {
    Part* p = PartOf(m);
    if (p == nullptr) throw NullReferenceException();
    return *p;
}
Vessel* VesselOf(const PartModule& m) {
    Part* p = PartOf(m);
    return p ? p->vessel : nullptr;
}
bool ActiveVesselIsEVA() { return FlightRegistry::ActiveVessel != nullptr && FlightRegistry::ActiveVessel->isEVA(); }

void RecordPhysics(Part& p, const std::string& what) {
    SerenityEnvironment::physicsActions.push_back(what);
    if (SerenityEnvironment::physicsAction) SerenityEnvironment::physicsAction(p, what);
}

// FlightGlobals.fetch.persistentLoadedPartIds / PersistentLoadedPartIds (FlightGlobals.cs:441)
Part* FindLoadedPart(uint32_t id) {
    SaveEnvironment& env = SaveEnvironment::Current();
    if (id == 0 || !env.persistentIds.fetch) return nullptr;
    const void* p = nullptr;
    if (env.persistentIds.persistentLoadedPartIds.TryGetValue(id, p)) return const_cast<Part*>(static_cast<const Part*>(p));
    return nullptr;
}

void RefreshVesselRename(Vessel* v) {
    GE::onVesselRename.Fire(GE::HostedFromToAction<Vessel*, std::string>(v, v->vesselName, v->vesselName));
    ++SerenityEnvironment::vesselMarkerRefreshes;
}

bool IsGroundSciencePart(PartModule& m) { return dynamic_cast<ModuleGroundSciencePart*>(&m) != nullptr; }
template <class T>
T* FindImpl(Part& p) {
    return static_cast<T*>(p.FindModuleImplementing([](PartModule& m) { return dynamic_cast<T*>(&m) != nullptr; }));
}

// the object is Unity-null once destroyed
template <class T>
bool UnityNull(const T* o) {
    return o == nullptr || o->destroyed;
}
inline bool UnityNull(const Part* p) { return p == nullptr; }

}  // namespace

// =============================================================================================================================
// ModuleGroundPart

ModuleGroundPart::~ModuleGroundPart() {
    GE::onVesselChange.Remove(this, &ModuleGroundPart::OnVesselChange);
    GE::OnAnimationGroupRetractComplete.Remove(this, &ModuleGroundPart::OnRetractCompleted);
    GE::onPartWillDie.Remove(this, &ModuleGroundPart::OnPartWillDie);
    GE::onSceneConfirmExit.Remove(this, &ModuleGroundPart::OnLeavingScene);
}

void ModuleGroundPart::OnAwake() {
    ModuleCargoPart::OnAwake();
    Events().Bind("RetrievePart", BaseEvent::Handler([this] { RetrievePart(); }));
}

// ModuleGroundPart.cs:48
void ModuleGroundPart::RetrievePart() {
    Part& p = NeedPart(*this);
    if (!HasChildParts()) {
        SerenityEnvironment::PostScreenMessage(Localizer::Format("#autoLOC_6006111"));
        return;
    }
    if (!CanBeStored()) return;
    beingRetrieved = true;
    Set<bool>("deployedOnGround", false);
    const AttachRules rules = p.fields.attachRules.value_or(AttachRules{});
    if (rules.allowStack || rules.allowSrfAttach) {
        if (p.partInfo == nullptr || !p.partInfo->partPrefab) throw NullReferenceException();
        p.fields.mass = p.partInfo->partPrefab->fields.mass;
        p.fields.prefabMass = p.partInfo->partPrefab->fields.prefabMass;
    }
    if (p.vessel == nullptr) throw NullReferenceException();
    p.vessel->vesselType = VesselType::DeployedSciencePart;
    if (kerbalInventoryModule == nullptr) OnVesselChange(FlightRegistry::ActiveVessel);
    if (kerbalInventoryModule != nullptr) {
        kerbalInventoryModule->partBeingRetrieved = true;
        if (kerbalInventoryModule->storeCargoPartAtSlot && kerbalInventoryModule->storeCargoPartAtSlot(p, -1) && SerenityInstalled() &&
            groundScienceModule != nullptr) {
            GE::onGroundSciencePartRemoved.Fire(groundScienceModule);
        }
        PlayRetractAnimation();
    }
}

// ModuleGroundPart.cs:84
bool ModuleGroundPart::CanBeStored() {
    bool result = true;
    if (LoadedSceneIsFlight() && FlightRegistry::ActiveVessel != nullptr && ActiveVesselIsEVA()) {
        auto* inv = SerenityEnvironment::activeKerbalInventory.get();
        if (inv != nullptr) {
            if (!(inv->hasCapacity ? inv->hasCapacity(NeedPart(*this)) : true)) {
                SerenityEnvironment::PostScreenMessage(Localizer::Format("#autoLOC_6006116"));
                result = false;
            }
        } else {
            result = false;
        }
    }
    return result;
}

// ModuleGroundPart.cs:106
bool ModuleGroundPart::HasChildParts() {
    bool flag = true;
    Part& p = NeedPart(*this);
    if (p.vessel != nullptr && p.vessel->parts.size() > 1) flag = false;
    if (flag) {
        for (size_t i = 0; i < p.attachNodes.size(); i++) {
            if (p.attachNodes[i].attachedPart != nullptr) {
                flag = false;
                break;
            }
        }
    }
    return flag;
}

// ModuleGroundPart.cs:127
void ModuleGroundPart::RetrieveScienceData() {
    if (kerbalInventoryModule == nullptr || !SerenityInstalled() || groundScienceModule == nullptr) return;
    if (FlightRegistry::ActiveVessel == nullptr) throw NullReferenceException();
    auto* container = dynamic_cast<ModuleScienceContainer*>(FlightRegistry::ActiveVessel->FindPartModuleImplementingClass("ModuleScienceContainer"));
    if (container != nullptr && groundScienceModule->ScienceClusterData() != nullptr) {
        RT::DeployedScienceExperiment* exp = RT::DeployedScienceExtensions::GetExperimentByPartID(
            groundScienceModule->ScienceClusterData()->DeployedScienceParts, NeedPart(*this).fields.persistentId);
        std::shared_ptr<ScienceData> data;
        if (exp != nullptr && exp->GatherScienceData(data)) container->AddData(data);
    }
}

// ModuleGroundPart.cs:144
void ModuleGroundPart::OnPartPack() {
    if (part != nullptr) RecordPhysics(NeedPart(*this), "constraints=None");
    if (hasJoint) {
        hasJoint = false;
        RecordPhysics(NeedPart(*this), "destroyJoint");
    }
}

// ModuleGroundPart.cs:156
void ModuleGroundPart::OnPartUnpack() {
    Part& p = NeedPart(*this);
    if (animationGroup == nullptr) animationGroup = dynamic_cast<ModuleAnimationGroup*>(p.FindModuleOfClass("ModuleAnimationGroup"));
    if (beingDeployed() && animationGroup != nullptr) animationGroup->isEnabled = true;
    if (beingAttached()) {
        Set<bool>("beingAttached", false);
    } else {
        Vessel* v = p.vessel;
        if (v == nullptr) throw NullReferenceException();
        if (beingSettled() || beingDeployed() || deployedOnGround() ||
            (v->parts.size() == 1 && (v->situation == Vessel::Situations::LANDED || v->situation == Vessel::Situations::PRELAUNCH))) {
            kinematicCoroutine_ = StartCoroutine(MakePartKinematic());
        }
    }
    OnVesselChange(nullptr);
}

// ModuleGroundPart.cs:177
void ModuleGroundPart::OnStart(StartState) {
    Part& p = NeedPart(*this);
    animationGroup = dynamic_cast<ModuleAnimationGroup*>(p.FindModuleOfClass("ModuleAnimationGroup"));
    groundScienceModule = FindImpl<ModuleGroundSciencePart>(p);
    groundExpConModule = FindImpl<ModuleGroundExpControl>(p);
    if (p.vessel != nullptr && p.vessel->vesselType == VesselType::DroppedPart) {
        isEnabled = false;
        if (animationGroup != nullptr) animationGroup->isEnabled = false;
        if (groundScienceModule != nullptr) groundScienceModule->SetEnabled(false);
        if (groundExpConModule != nullptr) groundExpConModule->SetEnabled(false);
    }
    UpdateGroundModuleUI();
    GE::onVesselChange.Add(this, &ModuleGroundPart::OnVesselChange);
    GE::onSceneConfirmExit.Add(this, &ModuleGroundPart::OnLeavingScene);
    if (LoadedSceneIsFlight()) GE::onPartWillDie.Add(this, &ModuleGroundPart::OnPartWillDie);
    inConstructionMode = false;  // EVAConstructionModeController (WP-33)
}

// ModuleGroundPart.cs:210
void ModuleGroundPart::OnDestroy() {
    StopCoroutineRef(kinematicCoroutine_);
    ModuleCargoPart::OnDestroy();
    GE::onVesselChange.Remove(this, &ModuleGroundPart::OnVesselChange);
    GE::OnAnimationGroupRetractComplete.Remove(this, &ModuleGroundPart::OnRetractCompleted);
    GE::onPartWillDie.Remove(this, &ModuleGroundPart::OnPartWillDie);
    GE::onSceneConfirmExit.Remove(this, &ModuleGroundPart::OnLeavingScene);
    if (hasJoint) {
        hasJoint = false;
        RecordPhysics(NeedPart(*this), "destroyJoint");
    }
}

// ModuleGroundPart.cs:227
void ModuleGroundPart::OnUpdate() {
    ModuleCargoPart::OnUpdate();
    if (partActionWindowOpen) UpdateGroundModuleUI();
}

// ModuleGroundPart.cs:236
Enumerator ModuleGroundPart::MakePartKinematic() {
    Part& p = NeedPart(*this);
    if (beingDeployed() && kinematicDelay() < 1.0f) Set<float>("kinematicDelay", 3.0f);
    if (p.vessel != nullptr && p.vessel->rootPart != nullptr && p.vessel->rootPart->fields.persistentId == p.fields.persistentId && p.vessel->parts.size() > 1) {
        childMass = DictionaryValueList<uint32_t, float>();
        childMassValid = true;
        for (size_t i = 0; i < p.vessel->parts.size(); i++) {
            Part* q = p.vessel->parts[i];
            if (q->fields.persistentId != p.vessel->rootPart->fields.persistentId) {
                childMass.Add(q->fields.persistentId, q->fields.prefabMass);
                q->fields.prefabMass = 0.01f;
                q->fields.mass = 0.01f;
                if (q->hasRigidbody) RecordPhysics(*q, "rb.mass=0.01");
            }
        }
    }
    co_yield YieldInstruction::Wait(StartCoroutine(MakePartSettle(deployedOnGround() || (beingDeployed() && kinematicDelay() > 1.0f))));
    if (deployedOnGround() || beingDeployed()) {
        const bool groundContact = SerenityEnvironment::groundContact ? SerenityEnvironment::groundContact(p) : true;
        if (!groundContact && kinematicDelay() >= 1.0f) {
            log::Info("[ModuleCargoPart]: Part " + p.partInfo->title + " cannot deploy unless on the ground.");
            SerenityEnvironment::PostScreenMessage(Localizer::Format("#autoLOC_8002326"));
            EnableModule();
            co_return;
        }
        p.fields.PermanentGroundContact = true;
        RecordPhysics(p, "constraints=FreezeAll");
        p.fields.autoStrutMode = static_cast<int32_t>(Part_AutoStrutMode::ForceGrandparent);
        PlayDeployAnimation();
        SetDeployedOnGroundFromCoRoutine();  // SendMessage("SetDeployedOnGroundFromCoRoutine")
    }
}

// ModuleGroundPart.cs:278
void ModuleGroundPart::SetDeployedOnGroundFromCoRoutine() {
    Part& p = NeedPart(*this);
    Set<bool>("deployedOnGround", true);
    Set<bool>("beingDeployed", false);
    const AttachRules rules = p.fields.attachRules.value_or(AttachRules{});
    if (rules.allowStack || rules.allowSrfAttach) p.fields.prefabMass = 2.0f;
    if (p.vessel->vesselType == VesselType::Unknown) p.vessel->vesselType = VesselType::DeployedGroundPart;
    if (SerenityInstalled() && groundScienceModule != nullptr) groundScienceModule->SetDeployedOnGround();
    if (hasJoint) RecordPhysics(p, "destroyJoint");
    hasJoint = true;  // AddComponent<FixedJoint>(): breakForce / breakTorque = +Infinity, enablePreprocessing = false
    RecordPhysics(p, "addFixedJoint(breakForce=inf,breakTorque=inf,enablePreprocessing=false)");
    if (childMassValid && childMass.Count() > 0) {
        for (size_t i = 0; i < p.vessel->parts.size(); i++) {
            Part* q = p.vessel->parts[i];
            if (childMass.ContainsKey(q->fields.persistentId)) {
                q->fields.prefabMass = childMass[q->fields.persistentId];
                q->fields.mass = 0.01f;
                if (q->hasRigidbody) RecordPhysics(*q, "rb.mass=0.01");
            }
        }
        childMass.Clear();
    }
    UpdateGroundModuleUI();
}

// ModuleGroundPart.cs:321
void ModuleGroundPart::OnLeavingScene(GameScenes) {
    if (beingRetrieved) {
        Vessel* v = VesselOf(*this);
        if (v == nullptr) throw NullReferenceException();
        v->state = Vessel::State::DEAD;
    }
}

// ModuleGroundPart.cs:329
void ModuleGroundPart::OnPartWillDie(Part* p) {
    Part* self = PartOf(*this);
    if (self != nullptr && p->fields.persistentId == self->fields.persistentId && self->hasRigidbody) {
        RecordPhysics(*self, "constraints=None");
        RecordPhysics(*self, "ResetInertiaTensor");
    }
}

// ModuleGroundPart.cs:338
void ModuleGroundPart::PlayDeployAnimation() {
    Part& p = NeedPart(*this);
    if (animationGroup != nullptr && animationGroup->isEnabled && !animationGroup->IsDeployed()) animationGroup->DeployModule();
    const std::string fx = Get<std::string>("fxGroupDeploy");
    if (!fx.empty() && p.findFxGroup(fx) != nullptr && beingDeployed()) RecordPhysics(p, "fxBurst:" + fx);
}

// ModuleGroundPart.cs:350
void ModuleGroundPart::PlayRetractAnimation() {
    Part& p = NeedPart(*this);
    if (animationGroup != nullptr && animationGroup->isEnabled && (animationGroup->IsDeployed() || p.vessel != nullptr)) {
        GE::OnAnimationGroupRetractComplete.Add(this, &ModuleGroundPart::OnRetractCompleted);
        animationGroup->RetractModule();
    } else {
        OnRetractCompleted(animationGroup);
    }
}

// ModuleGroundPart.cs:363
void ModuleGroundPart::OnRetractCompleted(ModuleAnimationGroup* aGroup) {
    if (aGroup != animationGroup) return;
    GE::OnAnimationGroupRetractComplete.Remove(this, &ModuleGroundPart::OnRetractCompleted);
    if (SerenityInstalled()) {
        if (groundScienceModule != nullptr) GE::onGroundSciencePartRemoved.Fire(groundScienceModule);
        if (groundExpConModule != nullptr) GE::onGroundScienceDeregisterCluster.Fire(NeedPart(*this).fields.persistentId);
    }
    if (kerbalInventoryModule != nullptr) kerbalInventoryModule->partBeingRetrieved = false;
    Vessel* v = VesselOf(*this);
    if (v == nullptr) throw NullReferenceException();
    v->Die();
}

// ModuleGroundPart.cs:388 / :397 (UIPartActionController raising / dismissing the part's window)
void ModuleGroundPart::OnPartActionUIShown(Part* p, bool open) {
    if (p != PartOf(*this)) return;
    partActionWindowOpen = open;
    if (open) UpdateGroundModuleUI();
}

// ModuleGroundPart.cs:405
void ModuleGroundPart::OnVesselChange(Vessel*) {
    if (LoadedSceneIsFlight() && FlightRegistry::ActiveVessel != nullptr && ActiveVesselIsEVA())
        kerbalInventoryModule = SerenityEnvironment::activeKerbalInventory.get();
}

// ModuleGroundPart.cs:413
void ModuleGroundPart::UpdateGroundModuleUI() {
    Events()["RetrievePart"]->active = false;
    if (LoadedSceneIsFlight() && FlightRegistry::ActiveVessel != nullptr && ActiveVesselIsEVA() && kerbalInventoryModule != nullptr) {
        firstEmptyInvSlot = kerbalInventoryModule->firstEmptySlot ? kerbalInventoryModule->firstEmptySlot() : -1;
        if (firstEmptyInvSlot > -1) Events()["RetrievePart"]->active = true;
    }
}

// ModuleGroundPart.cs:426
std::string ModuleGroundPart::GetModuleDisplayName() { return Localizer::Format("#autoLOC_6012033"); }

// ModuleGroundPart.cs:431
void ModuleGroundPart::EnableModule() {
    Vessel* v = VesselOf(*this);
    if (v != nullptr && v->vesselType != VesselType::DroppedPart) isEnabled = true;
}

// ModuleGroundPart.cs:439
void ModuleGroundPart::DisableModule() { isEnabled = false; }

// ModuleGroundPart.cs:444
bool ModuleGroundPart::ModuleIsActive() {
    if (groundScienceModule != nullptr) return isEnabled ? groundScienceModule->Enabled() : false;
    if (groundExpConModule != nullptr) return isEnabled ? groundExpConModule->Enabled() : false;
    return isEnabled;
}

// ModuleGroundPart.cs:465
bool ModuleGroundPart::IsSituationValid() {
    if (SaveEnvironment::Current().loadedScene != GameScenes::FLIGHT) return false;
    return NeedPart(*this).State() != PartStates::PLACEMENT;
}

// =============================================================================================================================
// ModuleGroundSciencePart

ModuleGroundSciencePart::~ModuleGroundSciencePart() {
    GE::onGroundScienceDeregisterCluster.Remove(this, &ModuleGroundSciencePart::OnGroundScienceDeregisterCluster);
    GE::onGroundScienceClusterUpdated.Remove(this, &ModuleGroundSciencePart::OnGroundScienceClusterUpdated);
    GE::onGroundScienceClusterPowerStateChanged.Remove(this, &ModuleGroundSciencePart::OnGroundScienceClusterPowerStateChanged);
}

// ModuleGroundSciencePart.cs:111-185
void ModuleGroundSciencePart::SetPowerUnitsProduced(int v) {
    if (v != PowerUnitsProduced()) {
        Set<int32_t>("powerUnitsProduced", v);
        if (!beingRetrieved) GE::onGroundSciencePartChanged.Fire(this);
    }
}
void ModuleGroundSciencePart::SetActualPowerUnitsProduced(int v) {
    if (v != ActualPowerUnitsProduced()) {
        Set<int32_t>("actualPowerUnitsProduced", v);
        if (!beingRetrieved) GE::onGroundSciencePartChanged.Fire(this);
    }
}
void ModuleGroundSciencePart::SetPowerUnitsRequired(int v) {
    if (v != PowerUnitsRequired()) {
        Set<int32_t>("powerUnitsRequired", v);
        if (!beingRetrieved) GE::onGroundSciencePartChanged.Fire(this);
    }
}
void ModuleGroundSciencePart::SetIsSolarPanel(bool v) {
    if (v != IsSolarPanel()) {
        Set<bool>("isSolarPanel", v);
        if (!beingRetrieved) GE::onGroundSciencePartChanged.Fire(this);
    }
}
// ModuleGroundSciencePart.cs:187
void ModuleGroundSciencePart::SetEnabled(bool v) {
    if (v != Enabled()) {
        Set<bool>("moduleEnabled", v);
        if (!beingRetrieved) {
            GE::onGroundSciencePartEnabledStateChanged.Fire(this);
            UpdateModuleUI();
        }
    }
}

// ModuleGroundSciencePart.cs:209
ModuleGroundSciencePart::Cluster* ModuleGroundSciencePart::ScienceClusterData() {
    if (UnityNull(scienceClusterData) && DeployedScience::Instance != nullptr && SerenityInstalled()) {
        Cluster* found = nullptr;
        if (!DeployedScience::Instance->DeployedScienceClusters().TryGetValue(ControlUnitId(), found)) found = nullptr;
        scienceClusterData = found;
    }
    return scienceClusterData;
}

// ModuleGroundSciencePart.cs:222
void ModuleGroundSciencePart::ToggleActive() {
    if (deployedOnGround()) {
        SetEnabled(!Enabled());
        UpdateModuleUI();
    }
}

// ModuleGroundSciencePart.cs:231
void ModuleGroundSciencePart::ToggleActiveAction() { ToggleActive(); }

// ModuleGroundSciencePart.cs:237
void ModuleGroundSciencePart::Start() {
    if (!SerenityInstalled() && LoadedSceneIsGame()) {
        behaviourEnabled = false;
        destroyed = true;
    }
}

// ModuleGroundSciencePart.cs:246
void ModuleGroundSciencePart::OnAwake() {
    ModuleGroundPart::OnAwake();
    SetDistanceToController(kFloatMax);
    Events().Bind("ToggleActive", BaseEvent::Handler([this] { ToggleActive(); }));
    Actions().Bind("ToggleActiveAction", [this](KSPActionParam&) { ToggleActiveAction(); });
}

// ModuleGroundSciencePart.cs:252
void ModuleGroundSciencePart::OnStart(StartState state) {
    ModuleGroundPart::OnStart(state);
    SetActualPowerUnitsProduced(PowerUnitsProduced());
    // trackingBody = Planetarium.fetch.Sun, layer masks, FindModelTransform lookups: the engine's (SerenityEnvironment::hasModelTransform)
    Part& p = NeedPart(*this);
    if (p.vessel != nullptr && LoadedSceneIsFlight() && (static_cast<int>(state) & static_cast<int>(StartState::Landed)) != 0 &&
        p.vessel->vesselType != VesselType::DroppedPart) {
        GE::onGroundScienceDeregisterCluster.Add(this, &ModuleGroundSciencePart::OnGroundScienceDeregisterCluster);
        GE::onGroundScienceClusterUpdated.Add(this, &ModuleGroundSciencePart::OnGroundScienceClusterUpdated);
        GE::onGroundScienceClusterPowerStateChanged.Add(this, &ModuleGroundSciencePart::OnGroundScienceClusterPowerStateChanged);
        if (p.FindModuleOfClass("ModuleGroundExpControl") == nullptr && DeployedScience::Instance != nullptr && ControlUnitId() == 0) {
            p.vessel->vesselType = VesselType::DeployedScienceController;
            ++SerenityEnvironment::mapObjectRefreshes;
            RefreshVesselRename(p.vessel);
            GE::onGroundSciencePartDeployed.Fire(this);
        }
    }
    Fields()["powerUnitsProduced"]->guiActive = IsSolarPanel();
}

// ModuleGroundSciencePart.cs:303
void ModuleGroundSciencePart::OnDestroy() {
    SetActualPowerUnitsProduced(PowerUnitsProduced());
    ModuleGroundPart::OnDestroy();
    GE::onGroundScienceDeregisterCluster.Remove(this, &ModuleGroundSciencePart::OnGroundScienceDeregisterCluster);
    GE::onGroundScienceClusterUpdated.Remove(this, &ModuleGroundSciencePart::OnGroundScienceClusterUpdated);
    GE::onGroundScienceClusterPowerStateChanged.Remove(this, &ModuleGroundSciencePart::OnGroundScienceClusterPowerStateChanged);
}

// ModuleGroundSciencePart.cs:314
void ModuleGroundSciencePart::OnUpdate() {
    ModuleGroundPart::OnUpdate();
    if (partActionWindowOpen) UpdateModuleUI();
}

// ModuleGroundSciencePart.cs:323
void ModuleGroundSciencePart::FixedUpdate() {
    if (!TrackSun()) return;
    Part& p = NeedPart(*this);
    const std::string secondary = Get<std::string>("secondaryTransformName");
    const bool secondaryTransform = !secondary.empty() && SerenityEnvironment::hasModelTransform && SerenityEnvironment::hasModelTransform(p, secondary);
    const bool trackingBodyPresent = Planetarium::fetch != nullptr;
    if (deployedOnGround() && isEnabled && behaviourEnabled && secondaryTransform && trackingBodyPresent && TimeWarp::CurrentRate() == 1.0f) {
        trackingLOS = CalculateTrackingLOS(blockingObject);
        if (trackingLOS) {
            if (IsSolarPanel() && ActualPowerUnitsProduced() != PowerUnitsProduced()) SetActualPowerUnitsProduced(PowerUnitsProduced());
            // the pivot's Atan2 / Quaternion.Lerp rotation toward the sun is the engine's (Transform.localRotation)
        } else if (IsSolarPanel() && ActualPowerUnitsProduced() > 0) {
            SetActualPowerUnitsProduced(0);
        }
    } else if (!deployedOnGround() || !isEnabled || !behaviourEnabled) {
        SetActualPowerUnitsProduced(0);
    }
}

// ModuleGroundSciencePart.cs:370 (the two Physics.Raycasts and the blocker name are the engine's)
bool ModuleGroundSciencePart::CalculateTrackingLOS(std::string& blocker) {
    if (SerenityEnvironment::trackingLineOfSight) return SerenityEnvironment::trackingLineOfSight(NeedPart(*this), blocker);
    return true;
}

// ModuleGroundSciencePart.cs:415
void ModuleGroundSciencePart::SetDeployedOnGround() {
    if (!beingRetrieved) GE::onGroundSciencePartEnabledStateChanged.Fire(this);
}

// ModuleGroundSciencePart.cs:423
void ModuleGroundSciencePart::OnLoad(const ConfigNode& node) {
    ModuleGroundPart::OnLoad(node);
    if (DeployedScience::Instance != nullptr && DeployedScience::IsActive && SerenityInstalled() && ScienceClusterData() != nullptr) {
        RT::DeployedSciencePart* dsp = RT::DeployedScienceExtensions::Get(scienceClusterData->DeployedScienceParts, NeedPart(*this).fields.persistentId);
        if (dsp != nullptr) {
            Set<int32_t>("powerUnitsProduced", dsp->PowerUnitsProduced());
            Set<int32_t>("powerUnitsRequired", dsp->PowerUnitsRequired());
            Set<bool>("isSolarPanel", dsp->IsSolarPanel());
            Set<bool>("moduleEnabled", dsp->Enabled());
        }
    }
}

// ModuleGroundSciencePart.cs:439
void ModuleGroundSciencePart::RetrievePart() {
    if (!CanBeStored()) return;
    beingRetrieved = true;
    Set<bool>("deployedOnGround", false);
    SetControlUnitId(0);
    SetDistanceToController(kFloatMax);
    Part& p = NeedPart(*this);
    const AvailablePart* availablePart = p.partInfo;
    if (availablePart != nullptr) {
        auto* prefabModule = FindImpl<ModuleGroundSciencePart>(*availablePart->partPrefab);
        if (prefabModule != nullptr) Set<int32_t>("powerUnitsProduced", prefabModule->PowerUnitsProduced());
    }
    if (TrackSun()) {
        sunTrackerRunning = true;
        StartCoroutine(SunTrackerToOrigin());
        return;
    }
    RetrieveScienceData();
    ModuleGroundPart::RetrievePart();
}

// ModuleGroundSciencePart.cs:467 (Quaternion.RotateTowards on the pivot: the engine step `panelAtOrigin`, one call per frame)
Enumerator ModuleGroundSciencePart::SunTrackerToOrigin() {
    for (;;) {
        const bool atOrigin = SerenityEnvironment::panelAtOrigin ? SerenityEnvironment::panelAtOrigin(NeedPart(*this)) : true;
        if (atOrigin) break;
        co_yield YieldInstruction::Null();
    }
    RetrieveScienceData();
    ModuleGroundPart::RetrievePart();
    sunTrackerRunning = false;
}

// ModuleGroundSciencePart.cs:480
void ModuleGroundSciencePart::OnGroundScienceClusterUpdated(ModuleGroundExpControl* controlUnit, Cluster*) {
    if (ControlUnitId() == controlUnit->ControlUnitId()) UpdateModuleUI();
}

// ModuleGroundSciencePart.cs:488
void ModuleGroundSciencePart::OnGroundScienceClusterPowerStateChanged(Cluster* scienceCluster) {
    if (ControlUnitId() == scienceCluster->ControlModulePartId()) UpdateModuleUI();
}

// ModuleGroundSciencePart.cs:496
void ModuleGroundSciencePart::OnGroundScienceDeregisterCluster(uint32_t controlUnitId) {
    if (controlUnitId != ControlUnitId()) return;
    SetControlUnitId(0);
    SetDistanceToController(kFloatMax);
    Part& p = NeedPart(*this);
    if (p.vessel != nullptr && p.fields.persistentId != controlUnitId) {
        p.vessel->vesselType = VesselType::DeployedScienceController;
        RefreshVesselRename(p.vessel);
        ++SerenityEnvironment::mapObjectRefreshes;
    }
    if (!beingRetrieved) GE::onGroundSciencePartDeployed.Fire(this);
}

// ModuleGroundSciencePart.cs:540
void ModuleGroundSciencePart::UpdateModuleUI() {
    Set<std::string>("PowerState", Localizer::Format("#autoLOC_7003285"));
    Events()["ToggleActive"]->guiActive = deployedOnGround();
    if (Enabled())
        Events()["ToggleActive"]->SetGuiName(Localizer::Format("#autoLOC_8002238"));
    else
        Events()["ToggleActive"]->SetGuiName(Localizer::Format("#autoLOC_8002239"));
    if (ScienceClusterData() != nullptr) {
        Set<std::string>("ConnectionState", Localizer::Format("#autoLOC_8002240"));
        if (Enabled() && DeployedOnGround()) {
            if (scienceClusterData->IsPowered())
                Set<std::string>("PowerState", Localizer::Format("#autoLOC_8002241"));
            else if (scienceClusterData->ControllerPartEnabled())
                Set<std::string>("PowerState", Localizer::Format("#autoLOC_8002242"));
            else
                Set<std::string>("PowerState", Localizer::Format("#autoLOC_8002253"));
        } else {
            Set<std::string>("PowerState", Localizer::Format("#autoLOC_8002243"));
        }
    } else {
        Set<std::string>("ConnectionState", Localizer::Format("#autoLOC_8002244"));
    }
}

// ModuleGroundSciencePart.cs:581
std::string ModuleGroundSciencePart::GetInfo() {
    std::string text = ModuleCargoPart::GetInfo();  // ModuleGroundPart has no GetInfo: base.GetInfo() is ModuleCargoPart's
    if (packedVolume() > 0.0f || stackableQuantity() > 1) text += "\n";
    text += Localizer::Format("<color=#00ffff>" + Localizer::Format("#autoLOC_8002222") + "</color>") + "\n";
    const int required = Get<int32_t>("powerUnitsRequired");
    const int produced = Get<int32_t>("powerUnitsProduced");
    if (required > 0)
        text += Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8002236"), std::to_string(required)}) + "\n";
    if (produced > 0) {
        text += "\n";
        text += Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8002235"), ""}) + "\n";
        text += Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_6006033"), std::to_string(produced)}) + "\n";
        std::shared_ptr<Experience::ExperienceSystemConfig> cfgs = SaveEnvironment::Current().experienceConfigs;
        if (cfgs) {
            const std::vector<std::string> traitsNamesWithEffect = cfgs->GetTraitsNamesWithEffect("DeployedSciencePowerSkill");
            Experience::ExperienceTrait trait;
            Experience::Effects::DeployedSciencePowerSkill skill(&trait);
            for (size_t i = 0; i < traitsNamesWithEffect.size(); i++) {
                Experience::ExperienceTraitConfig* traitConfig = nullptr;
                KerbalRoster::TryGetExperienceTraitConfig(traitsNamesWithEffect[i], traitConfig);
                if (traitConfig == nullptr) continue;
                for (size_t j = 0; j < traitConfig->Effects().size(); j++) {
                    if (traitConfig->Effects()[j]->Name() != "DeployedSciencePowerSkill") continue;
                    skill.LoadFromConfig(*traitConfig->Effects()[j]->Config());
                    if (skill.HasLevelModifiers()) {
                        for (size_t k = 1; k < skill.LevelModifiers().size(); k++) {
                            const std::string title = Localizer::Format(traitConfig->Title().value_or(""));
                            text += Localizer::Format("#autoLOC_8004190",
                                                       {Localizer::Format("#autoLOC_6006034", {title, std::to_string(k)}),
                                                        dotnet::FormatSingleG(f32store(fx(static_cast<float>(produced)) + fx(skill.GetTraitBonus(static_cast<int>(k)))))}) +
                                    "\n";
                        }
                    }
                }
            }
        }
    }
    return text + "\n";
}

// ModuleGroundSciencePart.cs:632
std::string ModuleGroundSciencePart::GetModuleDisplayName() { return Localizer::Format("#autoLOC_8002223"); }

// ModuleGroundSciencePart.cs:637
std::string ModuleGroundSciencePart::GetTooltip() {
    if (Get<std::string>("inventoryTooltip").empty()) return GetInfo();
    return Localizer::Format(Get<std::string>("inventoryTooltip"), {std::to_string(PowerUnitsRequired())});
}

// =============================================================================================================================
// ModuleGroundCommsPart

AntennaType ModuleGroundCommsPart::antennaType() const {
    const ModuleValue* v = Value("antennaType");
    const EnumValue* e = v ? std::get_if<EnumValue>(v) : nullptr;
    return e ? static_cast<AntennaType>(e->value) : AntennaType::DIRECT;
}

const DoubleCurve* ModuleGroundCommsPart::CommRangeCurve() const {
    const ModuleValue* v = Value("rangeCurve");
    return v ? std::get_if<DoubleCurve>(v) : nullptr;
}
const DoubleCurve* ModuleGroundCommsPart::CommScienceCurve() const {
    const ModuleValue* v = Value("scienceCurve");
    return v ? std::get_if<DoubleCurve>(v) : nullptr;
}

// ModuleGroundCommsPart.cs:44
bool ModuleGroundCommsPart::CanScienceTo(bool, double, double) { return false; }

// ModuleGroundCommsPart.cs:49
bool ModuleGroundCommsPart::CanComm() {
    Cluster* cluster = ScienceClusterData();
    if (cluster != nullptr) {
        if (UnityNull(controllerModule_)) {
            if (UnityNull(loadedPart_)) loadedPart_ = FindLoadedPart(cluster->ControlModulePartId());
            if (loadedPart_ != nullptr) controllerModule_ = FindImpl<ModuleGroundExpControl>(*loadedPart_);
        }
        if (cluster->ControlModulePartId() != NeedPart(*this).fields.persistentId && loadedPart_ != nullptr && controllerModule_ != nullptr) {
            if (isEnabled && deployedOnGround() && cluster->IsPowered() && Enabled()) return controllerModule_->Enabled();
            return false;
        }
        if (isEnabled && deployedOnGround() && cluster->IsPowered()) return Enabled();
        return false;
    }
    return false;
}

// ModuleGroundCommsPart.cs:81
bool ModuleGroundCommsPart::CanCommUnloaded(ProtoPartModuleSnapshot* mSnap) {
    if (mSnap == nullptr) return true;
    int count = static_cast<int>(mSnap->moduleValues->values().Count());
    do {
        if (count-- <= 0) return true;
    } while (!(mSnap->moduleValues->values()[static_cast<size_t>(count)].name == "canComm"));
    return mSnap->moduleValues->values()[static_cast<size_t>(count)].value != "False";
}

// ModuleGroundCommsPart.cs:99
double ModuleGroundCommsPart::CommPowerUnloaded(ProtoPartModuleSnapshot* mSnap) {
    double value = antennaPower();
    ConfigNode* node = nullptr;
    if (mSnap != nullptr && (node = mSnap->moduleValues->GetNode("UPGRADESAPPLIED")) != nullptr) {
        // LoadUpgradesApplied / ApplyUpgradeNode(doLoad: false): the part-upgrade system (PartUpgradeManager) is not ported; no upgrade
        // changes antennaPower (WP-06: PartModule.FindUpgrades)
    }
    return value;
}

// ModuleGroundCommsPart.cs:113
std::string ModuleGroundCommsPart::GetInfo() {
    std::string antType = antennaType() == AntennaType::INTERNAL ? "Internal" : antennaType() == AntennaType::DIRECT ? "Direct" : "Relay";
    std::string info = ModuleGroundSciencePart::GetInfo();
    info += "<color=#00ffff>" + Localizer::Format("#autoLOC_8002327") + "</color>\n";
    const std::string unitName = Localizer::Format("#autoLOC_7001411");
    info += Localizer::Format("#autoLOC_7001005", {antType});
    const std::string text2 = KSPUtil::PrintSI(CommPowerValue(), "") + (antennaCombinable() ? (" " + Localizer::Format("#autoLOC_236248")) : std::string());
    info += Localizer::Format("#autoLOC_7001006", {text2});
    CommNet::CommRangeModel model;
    info += Localizer::Format("#autoLOC_236834") + " " + KSPUtil::PrintSI(model.GetMaximumRange(antennaPower(), CommNet::DSN::GetDSNRange(0.0f)), unitName);
    info += Localizer::Format("#autoLOC_236835") + " " + KSPUtil::PrintSI(model.GetMaximumRange(antennaPower(), CommNet::DSN::GetDSNRange(0.5f)), unitName);
    info += Localizer::Format("#autoLOC_236836") + " " + KSPUtil::PrintSI(model.GetMaximumRange(antennaPower(), CommNet::DSN::GetDSNRange(1.0f)), unitName);
    info += "\n";
    if (!isEnabled) info += Localizer::Format("#autoLOC_236849");
    return info;
}

// ModuleGroundCommsPart.cs:133
std::string ModuleGroundCommsPart::GetModuleDisplayName() { return Localizer::Format("#autoLOC_6012035"); }

// =============================================================================================================================
// ModuleGroundExpControl

ModuleGroundExpControl::~ModuleGroundExpControl() {
    StopCoroutineRef(scanCoroutine_);
    GE::onGroundSciencePartDeployed.Remove(this, &ModuleGroundExpControl::OnGroundSciencePartDeployed);
    GE::onGroundScienceClusterUpdated.Remove(this, &ModuleGroundExpControl::OnGroundScienceClusterUpdated);
    GE::onGroundSciencePartEnabledStateChanged.Remove(this, &ModuleGroundExpControl::OnGroundSciencePartEnabledStateChanged);
    GE::onGroundSciencePartRemoved.Remove(this, &ModuleGroundExpControl::OnGroundScienceModuleRemoved);
    GE::onGroundScienceClusterPowerStateChanged.Remove(this, &ModuleGroundExpControl::OnGroundScienceClusterPowerStateChanged);
}

void ModuleGroundExpControl::SetVesselNaming() { ++vesselNamingCalls; }  // ModuleGroundExpControl.cs:42 (Part.SetVesselNaming: naming dialog)

// ModuleGroundExpControl.cs:47
void ModuleGroundExpControl::Start() {
    if (!SerenityInstalled() && LoadedSceneIsGame()) {
        behaviourEnabled = false;
        destroyed = true;
    }
}

// ModuleGroundExpControl.cs:56
void ModuleGroundExpControl::OnAwake() {
    ModuleGroundCommsPart::OnAwake();
    const DoubleCurve* range = CommRangeCurve();
    if (range == nullptr || range->keys.size() < 2) {
        DoubleCurve c;
        c.Add(0.0, 0.0, 0.0, 0.0);
        c.Add(1.0, 1.0, 0.0, 0.0);
        Set<DoubleCurve>("rangeCurve", std::move(c));
    }
    const DoubleCurve* science = CommScienceCurve();
    if (science == nullptr || science->keys.size() < 2) {
        DoubleCurve c;
        c.Add(0.0, 0.0, 0.0, 0.0);
        c.Add(0.44999998807907104, 0.07999999821186066, 0.4951019, 0.4951019);
        c.Add(0.8100000023841858, 0.3499999940395355, 0.8020515, 0.8020515);
        c.Add(1.0, 0.4000000059604645, 0.0, 0.0);
        Set<DoubleCurve>("scienceCurve", std::move(c));
    }
    SetDistanceToController(0.0f);
    Events().Bind("SetVesselNaming", BaseEvent::Handler([this] { SetVesselNaming(); }));
}

// ModuleGroundExpControl.cs:76
void ModuleGroundExpControl::OnDestroy() {
    GE::onGroundSciencePartDeployed.Remove(this, &ModuleGroundExpControl::OnGroundSciencePartDeployed);
    GE::onGroundScienceClusterUpdated.Remove(this, &ModuleGroundExpControl::OnGroundScienceClusterUpdated);
    GE::onGroundSciencePartEnabledStateChanged.Remove(this, &ModuleGroundExpControl::OnGroundSciencePartEnabledStateChanged);
    GE::onGroundSciencePartRemoved.Remove(this, &ModuleGroundExpControl::OnGroundScienceModuleRemoved);
    GE::onGroundScienceClusterPowerStateChanged.Remove(this, &ModuleGroundExpControl::OnGroundScienceClusterPowerStateChanged);
}

// ModuleGroundExpControl.cs:87
void ModuleGroundExpControl::OnStart(StartState state) {
    ModuleGroundCommsPart::OnStart(state);
    Part& p = NeedPart(*this);
    if (LoadedSceneIsFlight() && (static_cast<int>(state) & static_cast<int>(StartState::Landed)) != 0 && p.vessel != nullptr &&
        p.vessel->vesselType != VesselType::DroppedPart) {
        SetControlUnitId(p.fields.persistentId);
        if (DeployedScience::Instance != nullptr) scanCoroutine_ = StartCoroutine(ScanAndAddControlUnit());
        GE::onGroundSciencePartDeployed.Add(this, &ModuleGroundExpControl::OnGroundSciencePartDeployed);
    }
    if (p.vessel != nullptr && p.vessel->vesselType != VesselType::DroppedPart) {
        GE::onGroundScienceClusterUpdated.Add(this, &ModuleGroundExpControl::OnGroundScienceClusterUpdated);
        GE::onGroundSciencePartEnabledStateChanged.Add(this, &ModuleGroundExpControl::OnGroundSciencePartEnabledStateChanged);
        GE::onGroundSciencePartRemoved.Add(this, &ModuleGroundExpControl::OnGroundScienceModuleRemoved);
        GE::onGroundScienceClusterPowerStateChanged.Add(this, &ModuleGroundExpControl::OnGroundScienceClusterPowerStateChanged);
    }
    Fields()["PowerState"]->guiName = Localizer::Format("#autoLOC_8002247");
}

// ModuleGroundExpControl.cs:111
void ModuleGroundExpControl::OnSave(ConfigNode& node) {
    if (LoadedSceneIsFlight()) node.AddValue("canComm", CanComm());
}

// ModuleGroundExpControl.cs:119
void ModuleGroundExpControl::OnUpdate() {
    ModuleGroundCommsPart::OnUpdate();
    if (partActionWindowOpen) UpdateModuleUI();
}

// ModuleGroundExpControl.cs:128
std::string ModuleGroundExpControl::GetInfo() {
    std::string text = Localizer::Format("#autoLOC_8002220");
    text += "\n\n";
    Part& p = NeedPart(*this);
    const float prefabMass = p.partInfo->partPrefab->fields.mass;
    text += Localizer::Format("#autoLOC_443357") + " " + dotnet::FormatSingleF(f32store(fx(prefabMass) + fx(p.GetModuleMass(prefabMass))), 3) + " t\\n";
    if (packedVolume() > 0.0f)
        text += Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8003414"),
                                                         Localizer::Format("<<1>><<2>>", {dotnet::FormatSingleF(packedVolume(), 1), "L"})}) + "\n";
    if (stackableQuantity() > 1)
        text += Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8003418"), std::to_string(stackableQuantity())}) + "\n";
    if (packedVolume() > 0.0f || stackableQuantity() > 1) text += "\n";
    text += "<color=#00ffff>" + Localizer::Format("#autoLOC_8002227") + "</color>\n";
    text += Localizer::Format("#autoLOC_8002328", {std::to_string(controlUnitRange())}) + "\n";
    text += "\n";
    return text + ModuleGroundCommsPart::GetInfo();
}

// ModuleGroundExpControl.cs:151
void ModuleGroundExpControl::OnGroundScienceClusterPowerStateChanged(Cluster* cluster) {
    if (ControlUnitId() == cluster->ControlModulePartId()) UpdateModuleUI();
}

// ModuleGroundExpControl.cs:159
void ModuleGroundExpControl::OnGroundScienceModuleRemoved(ModuleGroundSciencePart* sciencePart) {
    if (ControlUnitId() == sciencePart->ControlUnitId()) {
        if (NeedPart(*sciencePart).fields.persistentId == ControlUnitId())
            beingDestroyed = true;
        else if (dynamic_cast<ModuleGroundCommsPart*>(sciencePart) != nullptr)
            RecalcBoosterPower(sciencePart);
    }
}

// ModuleGroundExpControl.cs:174
void ModuleGroundExpControl::OnGroundSciencePartEnabledStateChanged(ModuleGroundSciencePart* sciencePart) {
    if (ControlUnitId() == sciencePart->ControlUnitId() && dynamic_cast<ModuleGroundCommsPart*>(sciencePart) != nullptr) RecalcBoosterPower(sciencePart);
}

// ModuleGroundExpControl.cs:182
void ModuleGroundExpControl::OnGroundScienceClusterUpdated(ModuleGroundExpControl* controlUnit, Cluster*) {
    if (ControlUnitId() == controlUnit->ControlUnitId()) {
        UpdateModuleUI();
        RecalcBoosterPower(nullptr);
    }
}

// ModuleGroundExpControl.cs:191
Enumerator ModuleGroundExpControl::ScanAndAddControlUnit() {
    co_yield YieldInstruction::WaitForSeconds(2.0f);
    std::vector<ModuleGroundSciencePart*> connectedUnits;
    Part& self = NeedPart(*this);
    const std::vector<const void*> loadedParts = SaveEnvironment::Current().persistentIds.persistentLoadedPartIds.Values();
    for (size_t i = 0; i < loadedParts.size(); i++) {
        Part* lp = const_cast<Part*>(static_cast<const Part*>(loadedParts[i]));
        ModuleGroundSciencePart* m = FindImpl<ModuleGroundSciencePart>(*lp);
        if (m != nullptr && lp->fields.persistentId != self.fields.persistentId) {
            const float num = Vector3f::Distance(self.partTransform.position, lp->partTransform.position);
            if (m->ControlUnitId() == 0 && fx(num) <= static_cast<double>(controlUnitRange()) && num < m->DistanceToController()) {
                m->SetControlUnitId(self.fields.persistentId);
                SetDeployedSciencePartVesselType(m);
                connectedUnits.push_back(m);
            }
        }
        co_yield YieldInstruction::Null();
    }
    if (DeployedScience::Instance->DeployedScienceClusters().ContainsKey(self.fields.persistentId))
        GE::onGroundScienceControllerChanged.Fire(this, false, &connectedUnits);
    else
        GE::onGroundScienceRegisterCluster.Fire(this, &connectedUnits);
}

// ModuleGroundExpControl.cs:222
void ModuleGroundExpControl::OnGroundSciencePartDeployed(ModuleGroundSciencePart* sciencePart) {
    if (beingDestroyed) return;
    Part& self = NeedPart(*this);
    const float num = Vector3f::Distance(self.partTransform.position, NeedPart(*sciencePart).partTransform.position);
    if (fx(num) <= static_cast<double>(controlUnitRange()) && num < sciencePart->DistanceToController() && ScienceClusterData() != nullptr) {
        auto* component = dynamic_cast<ModuleGroundExperiment*>(sciencePart);
        if (component != nullptr && RT::DeployedScienceExtensions::GetExperiment(ScienceClusterData()->DeployedScienceParts, component->experimentId()) != nullptr) {
            return;  // Unable to connect: the Science Cluster already has this Experiment Connected
        }
        sciencePart->SetDistanceToController(num);
        std::vector<ModuleGroundSciencePart*> list;
        list.push_back(sciencePart);
        sciencePart->SetControlUnitId(self.fields.persistentId);
        SetDeployedSciencePartVesselType(sciencePart);
        GE::onGroundScienceControllerChanged.Fire(this, false, &list);
    }
}

// ModuleGroundExpControl.cs:247
void ModuleGroundExpControl::SetDeployedSciencePartVesselType(ModuleGroundSciencePart* sciencePart) {
    Vessel* v = VesselOf(*sciencePart);
    if (NeedPart(*sciencePart).fields.persistentId != NeedPart(*this).fields.persistentId && v != nullptr) {
        v->vesselType = VesselType::DeployedSciencePart;
        ++SerenityEnvironment::mapObjectRefreshes;
        RefreshVesselRename(v);
    }
}

// ModuleGroundExpControl.cs:281
void ModuleGroundExpControl::UpdateModuleUI() {
    ModuleGroundCommsPart::UpdateModuleUI();
    Fields()["ConnectionState"]->guiActive = false;
    Cluster* cluster = ScienceClusterData();
    if (cluster != nullptr) {
        Set<std::string>("PowerState", std::to_string(cluster->PowerAvailable()));
        if (!cluster->IsPowered())
            Set<std::string>("powerNeeded", Localizer::Format("#autoLOC_6002375", {std::to_string(cluster->PowerRequired()), Localizer::Format("#autoLOC_8002233")}));
        else
            Set<std::string>("powerNeeded", std::to_string(cluster->PowerRequired()));
    }
    if (cluster == nullptr) return;
    int32_t connected = 0;
    for (size_t i = 0; i < cluster->DeployedScienceParts.size(); i++)
        if (cluster->DeployedScienceParts[i]->Enabled() && cluster->DeployedScienceParts[i]->Experiment != nullptr) connected++;
    Set<int32_t>("experimentsConnected", connected);
}

// ModuleGroundExpControl.cs:311
void ModuleGroundExpControl::RecalcBoosterPower(ModuleGroundSciencePart* sciencePart) {
    Set<double>("boostedAntennaPower", 0.0);
    Cluster* cluster = ScienceClusterData();
    if (cluster == nullptr) return;
    for (size_t i = 0; i < cluster->AntennaParts().size(); i++) {
        bool flag = cluster->antennaParts[i]->Enabled();
        if (sciencePart != nullptr && cluster->antennaParts[i]->PartId() == NeedPart(*sciencePart).fields.persistentId)
            flag = sciencePart->Enabled() && sciencePart->DeployedOnGround();
        if (flag && cluster->antennaParts[i]->PartId() != NeedPart(*this).fields.persistentId)
            Set<double>("boostedAntennaPower", boostedAntennaPower() + cluster->antennaParts[i]->AntennaBoosterPower());
    }
}

// ModuleGroundExpControl.cs:332
double ModuleGroundExpControl::CommPowerUnloaded(ProtoPartModuleSnapshot* mSnap) {
    double num = ModuleGroundCommsPart::CommPowerUnloaded(mSnap);
    double value = 0.0;
    if (mSnap != nullptr && mSnap->moduleValues->TryGetValue("boostedAntennaPower", value)) num += value;
    return num;
}

// ModuleGroundExpControl.cs:343
std::string ModuleGroundExpControl::GetModuleDisplayName() { return Localizer::Format("#autoLOC_8002227"); }

// ModuleGroundExpControl.cs:348
std::string ModuleGroundExpControl::GetTooltip() {
    if (Get<std::string>("inventoryTooltip").empty()) return GetInfo();
    return Localizer::Format(Get<std::string>("inventoryTooltip"), {std::to_string(controlUnitRange())});
}

// ModuleGroundExpControl.cs:357
VesselControlState ModuleGroundExpControl::GetControlSourceState() const {
    auto* self = const_cast<ModuleGroundExpControl*>(this);
    Cluster* cluster = self->ScienceClusterData();
    if (cluster != nullptr) {
        if (cluster->IsPowered() && Enabled() && DeployedOnGround()) return VesselControlState::ProbeFull;
        return VesselControlState::None;
    }
    return VesselControlState::None;
}

// ModuleGroundExpControl.cs:370
bool ModuleGroundExpControl::IsCommCapable() const {
    auto* self = const_cast<ModuleGroundExpControl*>(this);
    Cluster* cluster = self->ScienceClusterData();
    if (cluster != nullptr) return cluster->IsPowered() && Enabled() && DeployedOnGround();
    return false;
}

// ModuleGroundExpControl.cs:383
void ModuleGroundExpControl::UpdateNetwork() {
    Vessel* v = VesselOf(*this);
    CommNet::CommNetVessel* connection = v ? v->connection : nullptr;
    if (partActionWindowOpen && connection != nullptr) {
        Set<std::string>("commNetSignal", KSPUtil::LocalizeNumber(connection->SignalStrengthValue(), "F2"));
        if (connection->IsConnected && connection->ControlPath().First())
            Set<std::string>("commNetFirstHopDistance", KSPUtil::PrintSI(connection->ControlPath().First()->cost, Localizer::Format("#autoLOC_7001411")));
        else
            Set<std::string>("commNetFirstHopDistance", "-");
    }
}

// =============================================================================================================================
// ModuleGroundExperiment

ModuleGroundExperiment::~ModuleGroundExperiment() {
    GE::onGroundScienceGenerated.Remove(this, &ModuleGroundExperiment::OnGroundScienceGenerated);
    GE::onGroundScienceTransmitted.Remove(this, &ModuleGroundExperiment::OnGroundScienceTransmitted);
}

const FloatCurve* ModuleGroundExperiment::DistanceCurve() const {
    const ModuleValue* v = Value("distanceCurve");
    return v ? std::get_if<FloatCurve>(v) : nullptr;
}

// ModuleGroundExperiment.cs:42 / :58
void ModuleGroundExperiment::SetScienceLimit(float v) {
    if (v != ScienceLimit()) {
        Set<float>("scienceLimit", v);
        GE::onGroundScienceExperimentScienceLimitChanged.Fire(this);
    }
}
void ModuleGroundExperiment::SetScienceValue(float v) {
    if (v != scienceValue) {
        scienceValue = v;
        GE::onGroundScienceExperimentScienceValueChanged.Fire(this);
    }
}

// ModuleGroundExperiment.cs:74
void ModuleGroundExperiment::Start() {
    if (!SerenityInstalled() && LoadedSceneIsGame()) {
        behaviourEnabled = false;
        destroyed = true;
    }
    experiment = ResearchAndDevelopment::GetExperiment(experimentId());
}

// ModuleGroundExperiment.cs:84
void ModuleGroundExperiment::OnStart(StartState state) {
    ModuleGroundSciencePart::OnStart(state);
    GE::onGroundScienceGenerated.Add(this, &ModuleGroundExperiment::OnGroundScienceGenerated);
    GE::onGroundScienceTransmitted.Add(this, &ModuleGroundExperiment::OnGroundScienceTransmitted);
    if (experimentId() == DeployedScience::SeismicExperimentId) Fields()["scienceValueDisplay"]->guiActive = false;
    Set<float>("scienceValueDisplay", scienceValue);
}

// ModuleGroundExperiment.cs:96 (sic: no base.OnDestroy call)
void ModuleGroundExperiment::OnDestroy() {
    GE::onGroundScienceGenerated.Remove(this, &ModuleGroundExperiment::OnGroundScienceGenerated);
    GE::onGroundScienceTransmitted.Remove(this, &ModuleGroundExperiment::OnGroundScienceTransmitted);
}

// ModuleGroundExperiment.cs:102
void ModuleGroundExperiment::OnLoad(const ConfigNode& node) {
    ModuleGroundSciencePart::OnLoad(node);
    if (DeployedScience::Instance != nullptr && DeployedScience::IsActive && SerenityInstalled() && ScienceClusterData() != nullptr) {
        deployedSciencePart = RT::DeployedScienceExtensions::Get(ScienceClusterData()->DeployedScienceParts, NeedPart(*this).fields.persistentId);
        if (deployedSciencePart != nullptr && deployedSciencePart->Experiment != nullptr) {
            Set<float>("scienceLimit", deployedSciencePart->Experiment->ScienceLimit());
            scienceValue = deployedSciencePart->Experiment->ScienceValue();
            SetScienceModifierRate(deployedSciencePart->Experiment->ScienceModifierRate());
        }
    }
}

// ModuleGroundExperiment.cs:117
void ModuleGroundExperiment::OnGroundScienceGenerated(DSExperiment*, DSPart* sciencePart, Cluster*, float) {
    if (sciencePart->PartId() == NeedPart(*this).fields.persistentId) UpdateModuleUI();
}

// ModuleGroundExperiment.cs:125
void ModuleGroundExperiment::OnGroundScienceTransmitted(DSExperiment*, DSPart* sciencePart, Cluster*, float) {
    if (sciencePart->PartId() == NeedPart(*this).fields.persistentId) UpdateModuleUI();
}

// ModuleGroundExperiment.cs:133
void ModuleGroundExperiment::UpdateModuleUI() {
    ModuleGroundSciencePart::UpdateModuleUI();
    if (UnityNull(scienceExperiment) && ScienceClusterData() != nullptr) {
        scienceExperiment = nullptr;
        DSPart* dsp = RT::DeployedScienceExtensions::GetExperiment(ScienceClusterData()->DeployedScienceParts, experimentId());
        if (dsp != nullptr) scienceExperiment = dsp->Experiment;
    }
    if (!UnityNull(scienceExperiment)) {
        const float limit = Get<float>("scienceLimit");
        Set<float>("ScienceCompletedPercentage",
                   (limit > 0.0f) ? f32store(fx(scienceExperiment->TotalScienceGenerated()) / fx(scienceExperiment->ScienceLimit()) * 100.0) : 0.0f);
        Set<float>("ScienceTransmittedPercentage",
                   (scienceExperiment->TransmittedScienceData() > 0.0f)
                       ? f32store(fx(scienceExperiment->TransmittedScienceData()) / fx(scienceExperiment->ScienceLimit()) * 100.0)
                       : 0.0f);
    } else {
        Set<float>("ScienceCompletedPercentage", 0.0f);
        Set<float>("ScienceTransmittedPercentage", 0.0f);
    }
    if ((!Enabled() || !DeployedOnGround()) && !UnityNull(scienceExperiment) && !scienceExperiment->ExperimentSituationValid())
        Set<std::string>("PowerState", Localizer::Format("#autoLOC_8002251"));
    if (Enabled() && DeployedOnGround() && !UnityNull(scienceExperiment) && scienceExperiment->ExperimentSituationValid() &&
        (ScienceClusterData() == nullptr || scienceClusterData->IsPowered()))
        Set<float>("scienceValueDisplay", scienceValue);
    else
        Set<float>("scienceValueDisplay", 0.0f);
}

// ModuleGroundExperiment.cs:168
void ModuleGroundExperiment::RetrievePart() {
    if (!CanBeStored()) return;
    Part& p = NeedPart(*this);
    if (p.partInfo != nullptr) {
        auto* prefabModule = FindImpl<ModuleGroundExperiment>(*p.partInfo->partPrefab);
        if (prefabModule != nullptr) SetScienceModifierRate(prefabModule->ScienceModifierRate());
    }
    ModuleGroundSciencePart::RetrievePart();
}

// ModuleGroundExperiment.cs:186
std::string ModuleGroundExperiment::GetInfo() {
    std::string text;
    text += ModuleGroundSciencePart::GetInfo();
    if (experiment == nullptr) experiment = ResearchAndDevelopment::GetExperiment(experimentId());
    if (experiment != nullptr) {
        text += "<color=#00ffff>" + experiment->experimentTitle + "</color>\n";
        const auto yesNo = [](bool b) { return b ? std::string("<color=#15b01a>V</color>") : std::string("<color=#e50000>X</color>"); };  // RUIutils.GetYesNoUIString (:117)
        text += Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8002224"), yesNo(experiment->requireAtmosphere)}) + "\n";
        text += Localizer::Format("#autoLOC_8004190", {Localizer::Format("#autoLOC_8002226"), yesNo(experiment->requireNoAtmosphere)}) + "\n";
    }
    return text;
}

// ModuleGroundExperiment.cs:203
std::string ModuleGroundExperiment::GetModuleDisplayName() { return Localizer::Format("#autoLOC_8002225"); }

// =============================================================================================================================

namespace detail {
void RegisterSerenityModules() {
    const auto reg = [](const char* name, ModuleFactory::Creator c) { ModuleFactory::Register(name, std::move(c)); };
    reg("ModuleGroundPart", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleGroundPart>(c)); });
    reg("ModuleGroundSciencePart", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleGroundSciencePart>(c)); });
    reg("ModuleGroundCommsPart", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleGroundCommsPart>(c)); });
    reg("ModuleGroundExpControl", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleGroundExpControl>(c)); });
    reg("ModuleGroundExperiment", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleGroundExperiment>(c)); });
}
}  // namespace detail

}  // namespace ksp
