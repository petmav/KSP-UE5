// WP-29b: ModuleWheelBase.cs (see ksp/wheels/wheel_modules.hpp for the scope and the seams). Every function cites File.cs:line of
// decompiled/KSP-Decompiled/Assembly-CSharp (tools/wp28/check_cites.py verifies the citations). Float32 follows the Mono rule.
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/core/coroutine_host.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/wheels/wheel_modules.hpp"

namespace ksp {

using namespace VehiclePhysics;
using namespace ModuleWheels;

namespace {
Enumerator DelayedSeconds(float s, std::function<void()> cb) {  // CallbackUtil.DelayedCallback<T>(float, Callback<T>, T): the argument is bound by the caller
    co_yield YieldInstruction::WaitForSeconds(s);
    cb();
}
Enumerator WaitUntilCoroutine(std::function<bool()> condition, std::function<void()> cb) {  // CallbackUtil.cs:106
    while (!condition()) co_yield YieldInstruction::Null();
    cb();
}
constexpr int kForceHeaviest = 5;  // Part.AutoStrutMode.ForceHeaviest (Part.cs:79)
}  // namespace

Part* ModuleWheelBase::PartPtr() const { return dynamic_cast<Part*>(part); }
Vessel* ModuleWheelBase::VesselPtr() const {
    Part* p = PartPtr();
    return p ? p->vessel : nullptr;
}

ModuleWheelBase::ModuleWheelBase(const ModuleClassInfo& cls) : PartModule(cls) { detail::NoteLiveWheelBase(this, true); }

ModuleWheelBase::~ModuleWheelBase() {
    detail::NoteLiveWheelBase(this, false);
    UnsubscribeEvents();
    if (WheelModuleEnvironment::scheduler != nullptr)
        for (auto& c : coroutines_) WheelModuleEnvironment::scheduler->StopCoroutine(c);
}

void ModuleWheelBase::SubscribeEvents() {
    GameEvents::onPartPack.Add(this, &ModuleWheelBase::OnPartPack);
    GameEvents::onPartUnpack.Add(this, &ModuleWheelBase::OnPartUnpack);
    GameEvents::onVesselSOIChanged.Add(this, &ModuleWheelBase::SOIChangeEvent);
    GameEvents::onVesselWasModified.Add(this, &ModuleWheelBase::OnVesselModified);
    GameEvents::onDockingComplete.Add(this, &ModuleWheelBase::OnDockingCompleteEvent);
    GameEvents::onVesselsUndocking.Add(this, &ModuleWheelBase::OnVesselUndocking);
    subscribed_ = true;
}

void ModuleWheelBase::UnsubscribeEvents() {
    if (!subscribed_) return;
    GameEvents::onVesselWasModified.Remove(this, &ModuleWheelBase::OnVesselModified);
    GameEvents::onPartPack.Remove(this, &ModuleWheelBase::OnPartPack);
    GameEvents::onPartUnpack.Remove(this, &ModuleWheelBase::OnPartUnpack);
    GameEvents::onVesselSOIChanged.Remove(this, &ModuleWheelBase::SOIChangeEvent);
    GameEvents::onDockingComplete.Remove(this, &ModuleWheelBase::OnDockingCompleteEvent);
    GameEvents::onVesselsUndocking.Remove(this, &ModuleWheelBase::OnVesselUndocking);
    subscribed_ = false;
}

// ModuleWheelBase.cs:202
void ModuleWheelBase::OnAwake() {
    PartModule::OnAwake();
    subModules_.clear();
    inopSystems = WheelSubsystems();
    Events().Bind("EvtAutoFrictionToggle", BaseEvent::Handler([this] { EvtAutoFrictionToggle(); }));
    Actions().Bind("ActAutoFrictionToggle", [this](KSPActionParam& p) { ActAutoFrictionToggle(p); });
}

// ModuleWheelBase.cs:208
void ModuleWheelBase::OnStart(StartState) {
    suspensionEnabled = true;
    Part* p = PartPtr();
    const std::string wheelTransformName = Get<std::string>("wheelTransformName");
    if (!wheelTransformName.empty()) {
        wheelTransform = WheelModuleEnvironment::findModelTransform ? WheelModuleEnvironment::findModelTransform(*p, wheelTransformName) : nullptr;
        if (wheelTransform == nullptr) {
            log::Error("[ModuleWheelBase]: No transform called " + wheelTransformName + " found in " + p->gameObjectName + " hierarchy");
            return;
        }
    }
    p->fields.autoStrutMode = kForceHeaviest;
    const std::string colliderName = Get<std::string>("wheelColliderTransformName");
    wheelColliderHost = WheelModuleEnvironment::findModelTransform ? WheelModuleEnvironment::findModelTransform(*p, colliderName) : nullptr;
    if (wheelColliderHost == nullptr) {
        log::Error("[ModuleWheelBase]: No transform called " + colliderName + " found in " + p->gameObjectName + " hierarchy");
        return;
    }
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        auto c = WheelModuleEnvironment::scheduler != nullptr
                     ? WheelModuleEnvironment::scheduler->StartCoroutine(
                           WaitUntilCoroutine([this] { return PartPtr()->fields.started && !PartPtr()->fields.packed; }, [this] { WheelSetup(); }))
                     : nullptr;
        if (c) coroutines_.push_back(c);
    }
    SubscribeEvents();
    inopOnRails_ = WheelSubsystem("Part is on-rails", WheelSubsystem::WheelCollider, this);
    inopSuspension_ = WheelSubsystem("Suspension is disabled", WheelSubsystem::Suspension, this);
    ActionUIUpdate();
    if (Get<int>("activeSubsteps") < 0) Set<int>("activeSubsteps", GameSettings::WHEEL_SUBSTEPS_ACTIVE);
    if (Get<int>("inactiveSubsteps") < 0) Set<int>("inactiveSubsteps", GameSettings::WHEEL_SUBSTEPS_INACTIVE);
    wheelDamageSubmodule_ = dynamic_cast<ModuleWheelDamage*>(p->FindModuleOfClass("ModuleWheelDamage"));
    brakesSubmodule_ = dynamic_cast<ModuleWheelBrakes*>(p->FindModuleOfClass("ModuleWheelBrakes"));
    wheelLockSubmodule_ = dynamic_cast<ModuleWheelLock*>(p->FindModuleOfClass("ModuleWheelLock"));
    Vessel* v = p->vessel;
    if (wheelLockSubmodule_ != nullptr && GetWheelType() == WheelType::LEG && v != nullptr && v->mainBody()->GeeASL > 1.100000023841858) ki = 1000.0f;
    if (GetWheelType() == WheelType::LEG) Set<float>("minimumDownforce", 10.0f);
}

// ModuleWheelBase.cs:277
void ModuleWheelBase::WheelSetup() {
    Part* p = PartPtr();
    Vessel* v = p->vessel;
    // part.UpdateAutoStrut(): joint bookkeeping, not modelled
    wheelHost_ = partRigidbodyTransform;
    const Vector3f velocity = partRigidbody->velocity;
    wheel = KSPWheelController::Create(partRigidbody, wheelHost_, wheelColliderHost);
    wheel->gravity.Value(v->gravityForPos.ToVector3f());
    wheel->minimumdownForce = Get<float>("minimumDownforce");
    if (wheel->wheelCollider != nullptr) wheel->wheelCollider->springSlerpRate = Get<float>("springSlerpRate");
    // wheel.wcTransform.SetParent(model.parent); wheel.wcTransform.localScale *= 1f / rescaleFactor: transform bookkeeping (scale is not modelled)
    int num = ~8519681;                  // ~LayerUtil.DefaultEquivalent
    num = ~(num | 0x20000) | 0x8000;
    if (wheel->wheelCollider != nullptr) {
        wheel->wheelCollider->layerMask = num;
        wheel->wheelCollider->mass = Get<float>("mass");
        CheckSubsteps();
        wheel->wheelCollider->suspensionDistance = 0.02f;
        wheel->wheelCollider->suspensionAnchor = 0.5f;
        wheel->wheelCollider->springRate = 10000.0f;
        wheel->wheelCollider->damperRate = 1.0f;
        wheel->wheelCollider->groundPenetration = 0.02f;
    }
    wheel->tireForceSmoothing = true;
    wheel->tireForceSharpness = Get<float>("tireForceSharpness");
    wheel->suspensionForceSmoothing = true;
    wheel->suspensionForceSharpness = Get<float>("suspensionForceSharpness");
    if (Get<float>("frictionSharpness") > 0.0f) {
        wheel->tireSideDeflection = true;
        wheel->tireSideDeflectionRate = Get<float>("frictionSharpness");
    }
    wheel->wheelDamping = f32store(1.0 - fx(Get<float>("wheelDamping")));
    wheel->maxRpm = WheelUtil::SpeedToRpm(Get<float>("wheelMaxSpeed"), Get<float>("radius"));
    wheel->OnShouldIgnoreForces = [this](const VehicleBase::WheelState& st) { return IgnoreForcesOnSameVesselContact(st); };
    if (wheel->wheelCollider != nullptr) {
        if (Get<bool>("FitWheelColliderToMesh") && wheelTransform != nullptr) {
            WheelModuleEnvironment::Trace("AdjustToWheelMesh");
        } else {
            wheel->wheelCollider->radius = f32store(fx(Get<float>("radius")) * fx(p->fields.rescaleFactor));
            wheel->wheelCollider->center = Get<Vector3f>("center") * p->fields.rescaleFactor;
        }
    }
    if (wheelTransform != nullptr) {
        WheelOrgPosR = p->partTransform.InverseTransformPoint(wheelTransform->position);
        WheelOrgRotR = UnityNative::Inverse(p->partTransform.rotation) * wheelTransform->rotation;
    } else {
        WheelOrgPosR = p->partTransform.InverseTransformPoint(wheelColliderHost->position);
        WheelOrgRotR = UnityNative::Inverse(p->partTransform.rotation) * wheelColliderHost->rotation;
    }
    inopSystems.SetOnModified(this, [this](WheelSubsystems* s) { OnSubsystemsModified(s); });
    OnWheelSetup(wheel.get());
    if (ResourceFlowEnvironment::LoadedSceneIsFlight() && v->mainBody() != nullptr) {
        ApplyGeeBias(f32store(v->mainBody()->GeeASL));
    } else if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        ApplyGeeBias(1.0f);
    }
    wheel->tireFriction->model = TireFriction::Model::Lineal;
    setup = true;
    InopUpdate(true);
    if (!wheel->enabled()) wheel->Initialize();
    wheel->rb()->velocity = velocity;
    if (Get<bool>("ApplyForcesToParent") && p->parent != nullptr) {
        wheel->tgtRb = parentRigidbody;
    }
}

// ModuleWheelBase.cs:376
float ModuleWheelBase::ApplyGeeBias(float gee) {
    if (Get<bool>("autoFriction")) {
        if (Get<bool>("useNewFrictionModel")) {
            Set<float>("frictionMultiplier", Mathf::Clamp(f32store(fx(gee) * fx(Get<float>("geeBias")) + fx(0.4f)), 0.4f, 10.0f));
        } else {
            Set<float>("minimumDownforce", 10.0f);
            if (Get<float>("geeBias") == 1.6f) Set<float>("geeBias", 1.0f);
            Set<float>("frictionMultiplier", Mathf::Clamp(f32store(1.0 / fx(gee) * fx(Get<float>("geeBias")) + 1.0), 0.5f, 10.0f));
        }
    }
    if (Wheel() != nullptr && wheel->tireFriction != nullptr) {
        wheel->tireFriction->settings->adherent.x = Get<float>("adherentStart");
        wheel->tireFriction->settings->peak.x = Get<float>("peakStart");
        wheel->tireFriction->settings->limit.x = Get<float>("limitStart");
        UpdateFriction();
    }
    return Get<float>("frictionMultiplier");
}

// ModuleWheelBase.cs:404
void ModuleWheelBase::UpdateFriction() {
    const float fm = Get<float>("frictionMultiplier");
    wheel->tireFriction->settings->adherent.y = f32store(fx(Get<float>("frictionAdherent")) * fx(fm));
    wheel->tireFriction->settings->peak.y = f32store(fx(Get<float>("frictionPeak")) * fx(fm));
    wheel->tireFriction->settings->limit.y = f32store(fx(Get<float>("frictionLimit")) * fx(fm));
}

// ModuleWheelBase.cs:411
void ModuleWheelBase::OnDestroy() { UnsubscribeEvents(); }

// ModuleWheelBase.cs:421
void ModuleWheelBase::FixedUpdate() {
    Part* p = PartPtr();
    if (!setup || !ResourceFlowEnvironment::LoadedSceneIsFlight() || p->fields.packed || p->State() == PartStates::DEAD) return;
    CheckSuspensionToggle();
    CheckSubsteps();
    VehicleBase::WheelState& cs = *wheel->currentState();
    slipDisplacement.x = cs.localWheelVelocity.x;
    slipDisplacement.y = f32store(fx(cs.angularVelocity) * fx(Get<float>("radius")) - fx(cs.localWheelVelocity.y));
    slipDisplacement = slipDisplacement * TimeWarp::deltaTime();
    if (wheelDamageSubmodule_ != nullptr && wheel->IsGrounded() && !Get<bool>("isGrounded")) {
        wheelDamageSubmodule_->ResetImmunity();
        if (GameSettings::WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED) {
            Vector3f velocity = partRigidbody->velocity;
            const Collider* collider = cs.hit.collider;
            if (collider->attachedRigidbody != nullptr) velocity -= collider->attachedRigidbody->velocity;
            wheelDamageSubmodule_->HandleCollisionVelocity(collider, velocity, cs.hit.normal);
        }
    }
    Set<bool>("isGrounded", wheel->IsGrounded());
    if (!Get<bool>("autoFriction")) UpdateFriction();
    wheel->gravity.Value(VesselPtr()->gravityForPos.ToVector3f());
    LandingDetectionUpdate();
    wheel->maxRpm = WheelUtil::SpeedToRpm(Get<float>("wheelMaxSpeed"), Get<float>("radius"));
    wheel->wheelDamping = f32store(1.0 - fx(Get<float>("wheelDamping")));
    if (driftCorrection) UpdateDriftFix();
    if (GetWheelType() != WheelType::LEG) LSchloomphaVPPProcessing();
}

// ModuleWheelBase.cs:465
void ModuleWheelBase::CheckSubsteps() {
    Vessel* v = VesselPtr();
    if (wheel != nullptr && v != nullptr) {
        const bool active = WheelModuleEnvironment::isActiveVessel ? WheelModuleEnvironment::isActiveVessel(v) : true;
        wheel->integrationSteps = active ? Get<int>("activeSubsteps") : Get<int>("inactiveSubsteps");
    }
}

// ModuleWheelBase.cs:473
void ModuleWheelBase::LandingDetectionUpdate() {
    Part* p = PartPtr();
    Vessel* vessel = p->vessel;
    VehicleBase::WheelState& cs = *wheel->currentState();
    if (!LandedDetectionNeedsUpdate(cs.hit.collider, gColliderPrev, vSrfContact, wheel->IsGrounded())) return;
    gColliderPrev = gCollider;
    gCollider = cs.hit.collider;
    p->fields.GroundContact = false;
    {
        auto it = std::find(vessel->GroundContacts.begin(), vessel->GroundContacts.end(), p);
        if (it != vessel->GroundContacts.end()) vessel->GroundContacts.erase(it);
    }
    vSrfContact = nullptr;
    crashObjectName.clear();
    WheelModuleEnvironment::Trace("crash:null");
    if (wheel->IsGrounded()) {
        if (gCollider == nullptr) throw std::runtime_error("InvalidOperationException: [ModuleWheelBase]: wheel says it's grounded but hit.collider is null! this can't possibly be right.");
        const WheelColliderInfo info = WheelModuleEnvironment::describeCollider ? WheelModuleEnvironment::describeCollider(gCollider) : WheelColliderInfo{0, "Untagged"};
        if (info.layer == 15) {
            if (info.tag != std::string()) {
                vLandedAt = info.tag;
                if (info.tag != "Untagged") {
                    if (info.rocTerrain && info.tag == "ROC") vLandedAt = info.rocTerrainTag;
                    WheelModuleEnvironment::Trace("SetLandedAt:" + vLandedAt + "|" + info.tag + "|");
                    vessel->SetLandedAt(vLandedAt, info.tag);
                } else {
                    vLandedAt = "";
                    WheelModuleEnvironment::Trace("SetLandedAt:|-|");
                    vessel->SetLandedAt(vLandedAt);
                }
            } else {
                vLandedAt = "";
                WheelModuleEnvironment::Trace("SetLandedAt:|-|");
                vessel->SetLandedAt(vLandedAt);
            }
            p->fields.GroundContact = true;
            if (std::find(vessel->GroundContacts.begin(), vessel->GroundContacts.end(), p) == vessel->GroundContacts.end()) vessel->GroundContacts.push_back(p);
            if (info.hasCrashObjectName) {
                crashObjectName = "set";
                WheelModuleEnvironment::Trace("crash:set");
            }
        } else if (info.layer == 0) {
            Part* other = info.part;
            if (other != nullptr && std::find(vessel->parts.begin(), vessel->parts.end(), other) == vessel->parts.end() && other->vessel->LandedOrSplashed()) {
                p->fields.GroundContact = true;
                if (std::find(vessel->GroundContacts.begin(), vessel->GroundContacts.end(), p) == vessel->GroundContacts.end()) vessel->GroundContacts.push_back(p);
                vSrfContact = other->vessel;
                vLandedAt = vSrfContact->landedAt;
                WheelModuleEnvironment::Trace("SetLandedAt:" + vLandedAt + "|-|" + other->vessel->displaylandedAt);
                vessel->SetLandedAt(vLandedAt, "", other->vessel->displaylandedAt);
            }
        }
    } else {
        WheelModuleEnvironment::Trace("ResetCollisions");
    }
    WheelModuleEnvironment::Trace("checkLanded");
    vessel->checkLanded();
    if (!vessel->Landed) WheelModuleEnvironment::Trace("SetLandedAt:|-|");
}

// ModuleWheelBase.cs:553
bool ModuleWheelBase::LandedDetectionNeedsUpdate(const Collider* hitCollider, const Collider* hitColliderPrev, Vessel* vContact, bool isGrounded) {
    Vessel* vessel = VesselPtr();
    if (isGrounded && vessel != nullptr && !vessel->Landed) return true;
    if (hitColliderPrev != hitCollider) return true;
    if (hitCollider != nullptr && !isGrounded) return true;
    if (hitCollider == nullptr && isGrounded) return true;
    if (vContact != nullptr && !vContact->Landed) return true;
    return false;
}

// ModuleWheelBase.cs:578
bool ModuleWheelBase::IgnoreForcesOnSameVesselContact(const VehicleBase::WheelState& st) {
    const Collider* collider = st.hit.collider;
    if (collider == nullptr) return false;
    if (collider->attachedRigidbody == nullptr) return false;
    const WheelColliderInfo info = WheelModuleEnvironment::describeCollider ? WheelModuleEnvironment::describeCollider(collider) : WheelColliderInfo{};
    if (info.part == nullptr) return false;
    return info.part->vessel == VesselPtr();
}

// ModuleWheelBase.cs:602
void ModuleWheelBase::LSchloomphaVPPProcessing() {
    Vessel* vessel = VesselPtr();
    const float timeSinceLevelLoad = WheelModuleEnvironment::timeSinceLevelLoad ? WheelModuleEnvironment::timeSinceLevelLoad() : 0.0f;
    if (!(timeSinceLevelLoad > 3.0f) || !vessel->loaded || !vessel->Landed || !Get<bool>("isGrounded") || !(wheel != nullptr) ||
        (!(brakesSubmodule_ != nullptr) && !(wheelLockSubmodule_ != nullptr)))
        return;
    if ((brakesSubmodule_ != nullptr && brakesSubmodule_->Get<float>("brakeInput") >= 1.0f) || (wheelLockSubmodule_ != nullptr && wheel->brakeInput >= 1.0f)) {
        if (((GetWheelType() == WheelType::LEG) ? (vessel->srfSpeed < 0.10000000149011612) : (vessel->srfSpeed < 9.999999747378752E-06)) && !rbBrakeConstraints) {
            ToggleRbConstraints(true);
            if (WheelModuleEnvironment::scheduler != nullptr) {
                coroutines_.push_back(WheelModuleEnvironment::scheduler->StartCoroutine(DelayedSeconds(schloompaTime, [this] { ToggleRbConstraints(false); })));
            }
            rbBrakeConstraints = true;
        }
    } else {
        rbBrakeConstraints = false;
    }
}

// ModuleWheelBase.cs:623
void ModuleWheelBase::EnableWheelCollider() {
    WheelModuleEnvironment::Trace("EnableWheelCollider");
    wheel->wheelCollider->activeInHierarchy = true;
    wheel->wheelCollider->ResetWheelCollider();
    wheel->wheelCollider->SetEnabled(true);
}

// ModuleWheelBase.cs:631
void ModuleWheelBase::DisableWheelCollider(bool immediate) {
    wheel->wheelCollider->SetEnabled(false);
    WheelModuleEnvironment::Trace(std::string("DisableWheelCollider:") + (immediate ? "1" : "0"));
    wheel->wheelCollider->activeInHierarchy = false;
}

// ModuleWheelBase.cs:649
void ModuleWheelBase::OnVesselModified(Vessel* v) {
    Part* p = PartPtr();
    if (!(v == nullptr) && v == p->vessel && Get<bool>("ApplyForcesToParent")) {
        if (p->parent == nullptr) {
            wheel->tgtRb = partRigidbody;
        } else {
            wheel->tgtRb = parentRigidbody;
        }
    }
}

// ModuleWheelBase.cs:666
void ModuleWheelBase::ResetWheelGroundCheck() { gColliderPrev = nullptr; }

// ModuleWheelBase.cs:671
void ModuleWheelBase::OnPutToGround(PartHeightQuery& qry) {
    Part* p = PartPtr();
    if (Get<float>("groundHeightOffset") != 0.0f) {
        PartModule* dep = p->FindModuleOfClass("ModuleWheelDeployment");
        bool flag = false;
        if (dep != nullptr && dep->Get<std::string>("stateString") == "Retracted") {
            qry.lowestOnParts.at(p) = Mathf::Min(qry.lowestOnParts.at(p), f32store(fx(p->partTransform.TransformPoint(WheelOrgPosR).y) - fx(Get<float>("radius"))));
            flag = true;
        }
        if (!flag) {
            WheelModuleEnvironment::Trace("PutToGround:manualOffset");
            qry.lowestOnParts.at(p) = Mathf::Min(qry.lowestOnParts.at(p), f32store(fx(p->partTransform.position.y) - fx(Get<float>("groundHeightOffset"))));
        }
    } else {
        qry.lowestOnParts.at(p) = Mathf::Min(qry.lowestOnParts.at(p), f32store(fx(p->partTransform.TransformPoint(WheelOrgPosR).y) - fx(Get<float>("radius"))));
    }
    qry.lowestPoint = Mathf::Min(qry.lowestPoint, qry.lowestOnParts.at(p));
}

// ModuleWheelBase.cs:695
void ModuleWheelBase::RegisterSubmodule(ModuleWheelSubmodule* m) {
    if (std::find(subModules_.begin(), subModules_.end(), m) == subModules_.end()) subModules_.push_back(m);
    if (setup) m->OnWheelInit(wheel.get());
}

// ModuleWheelBase.cs:704
void ModuleWheelBase::UnregisterSubmodule(ModuleWheelSubmodule* m) {
    auto it = std::find(subModules_.begin(), subModules_.end(), m);
    if (it != subModules_.end()) subModules_.erase(it);
}

// ModuleWheelBase.cs:709
void ModuleWheelBase::OnWheelSetup(KSPWheelController* w) {
    int count = static_cast<int>(subModules_.size());
    while (count-- > 0) subModules_[static_cast<size_t>(count)]->OnWheelInit(w);
}

// ModuleWheelBase.cs:718
void ModuleWheelBase::OnPartPack(Part* p) {
    if (p == PartPtr()) InopSystems()->AddSubsystem(&inopOnRails_);
}

// ModuleWheelBase.cs:726
void ModuleWheelBase::OnPartUnpack(Part* p) {
    if (p == PartPtr()) InopSystems()->RemoveSubsystem(&inopOnRails_);
}

// ModuleWheelBase.cs:734
void ModuleWheelBase::OnDockingComplete(Part* from, Part* to) {
    Vessel* vessel = VesselPtr();
    if (from->vessel == vessel || to->vessel == vessel) {
        HoldWheelDamage();
        WheelModuleEnvironment::Trace("ignoreColliders");
        suspensionEnabled = false;
        CheckSuspensionToggle();
        ToggleRbConstraints(false);
        InopWheelCollider(true, true);
        InopWheelCollider(false, true);
        wheel->wheelCollider->springRate = 0.0f;
        suspensionEnabled = true;
        CheckSuspensionToggle();
        if (brakesSubmodule_ != nullptr) {
            const float brakeInput = brakesSubmodule_->Get<float>("brakeInput");
            brakesSubmodule_->Set<float>("brakeInput", 1.0f);
            if (WheelModuleEnvironment::scheduler != nullptr) {
                ModuleWheelBrakes* b = brakesSubmodule_;
                coroutines_.push_back(WheelModuleEnvironment::scheduler->StartCoroutine(DelayedSeconds(1.0f, [b, brakeInput] { b->Set<float>("brakeInput", brakeInput); })));
            }
        }
        rbBrakeConstraints = false;
        if (GetWheelType() != WheelType::LEG) LSchloomphaVPPProcessing();
    }
}

// ModuleWheelBase.cs:767
void ModuleWheelBase::OnVesselUndocking(Vessel* fromVessel, Vessel* toVessel) {
    Vessel* vessel = VesselPtr();
    if ((fromVessel == vessel || toVessel == vessel) && !fromVessel->isEVA() && !toVessel->isEVA()) {
        HoldWheelDamage();
        WheelModuleEnvironment::Trace("ignoreColliders");
        suspensionEnabled = false;
        CheckSuspensionToggle();
        ToggleRbConstraints(false);
        InopWheelCollider(true, true);
        InopWheelCollider(false, true);
        wheel->wheelCollider->springRate = 0.0f;
        suspensionEnabled = true;
        CheckSuspensionToggle();
        rbBrakeConstraints = false;
    }
}

// ModuleWheelBase.cs:785
void ModuleWheelBase::ToggleRbConstraints(bool freeze) {
    if (freeze) {
        WheelModuleEnvironment::Trace("lock:set");
        InputLockManager::SetControlLock(ControlTypes::GROUP_BRAKES | ControlTypes::WHEEL_STEER | ControlTypes::WHEEL_THROTTLE, "WheelRBConstraints");
        HoldWheelDamage(f32store(fx(schloompaTime) * 1.5));
        WheelModuleEnvironment::Trace("ignoreColliders");
        WheelModuleEnvironment::Trace("rb:FreezeAll");
    } else {
        WheelModuleEnvironment::Trace("rb:None");
        InputLockManager::RemoveControlLock("WheelRBConstraints");
        WheelModuleEnvironment::Trace("lock:remove");
    }
}

// ModuleWheelBase.cs:801
void ModuleWheelBase::HoldWheelDamage(float seconds) {
    ModuleWheelDamage* d = dynamic_cast<ModuleWheelDamage*>(PartPtr()->FindModuleOfClass("ModuleWheelDamage"));
    if (d != nullptr) d->startupTime = seconds;
    WheelModuleEnvironment::Trace("hold:" + std::to_string(seconds));
}

// ModuleWheelBase.cs:813 -- `FromTo.from == base.vessel` compares a CelestialBody with a Vessel (UnityEngine.Object.operator==): never equal, so
// the handler never applies the gee bias
void ModuleWheelBase::SOIChange(Vessel* host, CelestialBody* from, CelestialBody* to) {
    (void)host;
    (void)to;
    if (static_cast<const void*>(from) == static_cast<const void*>(VesselPtr())) ApplyGeeBias(f32store(to->GeeASL));
}

// ModuleWheelBase.cs:821
void ModuleWheelBase::OnInventoryModeDisable() {
    if (wheel != nullptr) {
        OnPartPack(PartPtr());
        DisableWheelCollider(true);
        WheelModuleEnvironment::Trace("DestroyVPWheelCollider");
        WheelModuleEnvironment::Trace("DestroyWheelController");
    }
    if (partRigidbody != nullptr) partRigidbody->isKinematic = true;
}

// ModuleWheelBase.cs:840
void ModuleWheelBase::OnInventoryModeEnable() {
    if (partRigidbody != nullptr) partRigidbody->isKinematic = false;
    OnPartUnpack(PartPtr());
}

// ModuleWheelBase.cs:849
void ModuleWheelBase::DemoteToPhysicslessPart() { OnInventoryModeDisable(); }

// ModuleWheelBase.cs:854
void ModuleWheelBase::PromoteToPhysicalPart() {
    if (partRigidbody != nullptr) partRigidbody->isKinematic = false;
    if (wheelColliderHost == nullptr && WheelModuleEnvironment::findModelTransform)
        wheelColliderHost = WheelModuleEnvironment::findModelTransform(*PartPtr(), Get<std::string>("wheelColliderTransformName"));
    if (wheel == nullptr) {
        WheelSetup();
        wheel->wheelCollider->activeInHierarchy = true;
    }
    if (wheel != nullptr && wheel->wheelCollider == nullptr) WheelModuleEnvironment::Trace("AddVPWheelCollider");
    OnPartUnpack(PartPtr());
}

// ModuleWheelBase.cs:919
void ModuleWheelBase::OnSubsystemsModified(WheelSubsystems* sub) {
    if (sub == &inopSystems) InopUpdate();
}

// ModuleWheelBase.cs:927
void ModuleWheelBase::InopUpdate(bool force) {
    InopWheelCollider(inopSystems.HasType(WheelSubsystem::WheelCollider), force);
    InopWheelTransform(inopSystems.HasType(WheelSubsystem::Tire));
}

// ModuleWheelBase.cs:933
void ModuleWheelBase::InopWheelCollider(bool inop, bool force) {
    if (wheel == nullptr) return;
    if (inop && (force || wheel->enabled()) && setup) {
        DisableWheelCollider(force);
        wheel->enabled(false);
        wheel->IsGrounded(false);
    }
    if (!inop && (force || !wheel->enabled()) && setup) {
        const Vector3f velocity = partRigidbody->velocity;
        EnableWheelCollider();
        wheel->enabled(true);
        wheel->rb()->velocity = velocity;
    }
}

// ModuleWheelBase.cs:963
void ModuleWheelBase::InopWheelTransform(bool disable) { wheel->wheelCollider->updateWheel = !disable; }

// ModuleWheelBase.cs:978
void ModuleWheelBase::CheckSuspensionToggle() {
    if (!suspensionEnabled && !InopSystems()->Contains(&inopSuspension_)) {
        InopSystems()->AddSubsystem(&inopSuspension_);
    } else if (suspensionEnabled && InopSystems()->Contains(&inopSuspension_)) {
        InopSystems()->RemoveSubsystem(&inopSuspension_);
    }
}

// ModuleWheelBase.cs:990
void ModuleWheelBase::UpdateDriftFix() {
    switch (driftCorrectionState) {
    case DriftCorrectionState::Idle:
        if (wheel->IsGrounded()) driftCorrectionState = DriftCorrectionState::Acquire;
        break;
    case DriftCorrectionState::Acquire:
        if (wheel->IsGrounded()) {
            if (wheel->currentState()->localWheelVelocity.sqrMagnitude() < Mathf::Pow(acquireMaxSpeed, 2.0f)) {
                fixFwd = GetFixFwd();
                error = Vector3f::zero();
                driftCorrectionState = DriftCorrectionState::Fix;
            }
        } else {
            driftCorrectionState = DriftCorrectionState::Idle;
        }
        break;
    case DriftCorrectionState::Fix:
        if (wheel->IsGrounded()) {
            if (wheel->currentState()->localWheelVelocity.sqrMagnitude() < Mathf::Pow(acquireMaxSpeed, 2.0f) && wheel->IsGrounded()) {
                GetFixTorque(fixFwd, GetFixFwd());
                const Vector3f vector = error * ki;
                const Vector3f vector2 = (error - errorLast) * kd;
                const Vector3f t = vector + vector2;
                // wheel.RbTgt.AddTorque(vector + vector2, ForceMode.Acceleration)
                char buf[160];
                std::snprintf(buf, sizeof buf, "torque:%.17g,%.17g,%.17g", static_cast<double>(t.x), static_cast<double>(t.y), static_cast<double>(t.z));
                WheelModuleEnvironment::Trace(buf);
            } else {
                driftCorrectionState = DriftCorrectionState::Acquire;
            }
        } else {
            driftCorrectionState = DriftCorrectionState::Idle;
        }
        break;
    }
}

// ModuleWheelBase.cs:1038
Vector3f ModuleWheelBase::GetFixFwd() {
    if (GetWheelType() != WheelType::LEG) return wheelHost_->forward();
    return wheelHost_->right();
}

// ModuleWheelBase.cs:1047
void ModuleWheelBase::GetFixTorque(const Vector3f& fixOrt, const Vector3f& refOrt) {
    const Vector3f vector = Vector3f::Cross(refOrt, fixOrt) * f32store(VesselPtr()->mainBody()->GeeASL);
    errorLast = error;
    error += vector / Time::deltaTime;
}

// ModuleWheelBase.cs:1055
void ModuleWheelBase::EvtAutoFrictionToggle() {
    Set<bool>("autoFriction", !Get<bool>("autoFriction"));
    ActionUIUpdate();
    ATsymPartUpdate();
}

// ModuleWheelBase.cs:1063
void ModuleWheelBase::ActAutoFrictionToggle(KSPActionParam& act) {
    const bool af = Get<bool>("autoFriction");
    Set<bool>("autoFriction", act.type != 0 || (act.type == static_cast<int>(KSPActionType::Toggle) && !af));
    ActionUIUpdate();
}

// ModuleWheelBase.cs:1069
void ModuleWheelBase::ActionUIUpdate() {
    Part* p = PartPtr();
    Vessel* v = p->vessel;
    BaseEvent* evt = Events()["EvtAutoFrictionToggle"];
    BaseField* fld = Fields()["frictionMultiplier"];
    if (Get<bool>("autoFrictionAvailable")) {
        evt->SetGuiName(Localizer::Format("#autoLOC_7001003", std::vector<std::string>{std::to_string(Get<bool>("autoFriction") ? 1 : 0)}));
        fld->guiActive = !Get<bool>("autoFriction");
        fld->guiActiveEditor = !Get<bool>("autoFriction");
        if (ResourceFlowEnvironment::LoadedSceneIsFlight() && v != nullptr && v->mainBody() != nullptr) {
            ApplyGeeBias(f32store(v->mainBody()->GeeASL));
        } else if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
            ApplyGeeBias(1.0f);
        }
        return;
    }
    evt->guiActive = false;
    evt->guiActiveEditor = false;
    fld->guiActive = false;
    fld->guiActiveEditor = false;
    Set<bool>("autoFriction", false);
    Set<float>("frictionMultiplier", 1.0f);
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        if (v != nullptr && v->mainBody() != nullptr) {
            ApplyGeeBias(f32store(v->mainBody()->GeeASL));
        } else {
            ApplyGeeBias(1.0f);
        }
    }
}

// ModuleWheelBase.cs:1105 (the scripted world has no symmetry counterparts; with some, each counterpart's module takes the flag and refreshes its UI)
void ModuleWheelBase::ATsymPartUpdate() {
    Part* p = PartPtr();
    for (size_t i = 0, count = p->symmetryCounterparts.size(); i < count; i++) {
        PartModule* m = p->symmetryCounterparts[i]->Modules()[static_cast<size_t>(p->Modules().IndexOf(this))];
        auto* obj = dynamic_cast<ModuleWheelBase*>(m);
        obj->Set<bool>("autoFriction", Get<bool>("autoFriction"));
        obj->ActionUIUpdate();
    }
}

}  // namespace ksp
