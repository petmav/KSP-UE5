// WP-29b: ModuleWheels/{ModuleWheelSubmodule, ModuleWheelMotor, ModuleWheelMotorSteering, ModuleWheelBrakes, ModuleWheelLock, ModuleWheelSuspension,
// ModuleWheelSteering, ModuleWheelDamage, ModuleWheelDeployment, ModuleWheelBogey}.cs (see ksp/wheels/wheel_modules.hpp for the scope and the
// seams). Every function cites File.cs:line of decompiled/KSP-Decompiled/Assembly-CSharp (tools/wp28/check_cites.py verifies the citations).
// Float32 follows the Mono rule (ksp/dotnet/mono_float.hpp).
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/flight_ctrl_state.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/wheels/wheel_modules.hpp"

namespace ksp {

using namespace VehiclePhysics;

namespace {
// the live ModuleWheelBase objects: a submodule destroyed after its base (C#: the managed object stays valid) must not touch it
std::set<const ModuleWheelBase*>& LiveBases() {
    static std::set<const ModuleWheelBase*> s;
    return s;
}
Enumerator DelayedFrames(int frames, std::function<void()> cb) {  // CallbackUtil.cs:32
    for (int i = 0; i < frames; i++) co_yield YieldInstruction::Null();
    cb();
}
Part* AsPart(PartPrefab* p) { return dynamic_cast<Part*>(p); }
}  // namespace

void detail::NoteLiveWheelBase(const ModuleWheelBase* b, bool alive) {
    if (alive) LiveBases().insert(b);
    else LiveBases().erase(b);
}

namespace ModuleWheels {

// ---- ModuleWheelSubmodule ------------------------------------------------------------------------------------------------------------------
Part* ModuleWheelSubmodule::PartPtr() const { return AsPart(part); }
Vessel* ModuleWheelSubmodule::VesselPtr() const {
    Part* p = PartPtr();
    return p ? p->vessel : nullptr;
}

std::shared_ptr<Coroutine> ModuleWheelSubmodule::StartCoroutine(Enumerator e) {
    if (WheelModuleEnvironment::scheduler == nullptr) return nullptr;
    auto c = WheelModuleEnvironment::scheduler->StartCoroutine(std::move(e));
    coroutines_.push_back(c);
    return c;
}

ModuleWheelSubmodule::~ModuleWheelSubmodule() {
    OnDestroy();
    if (WheelModuleEnvironment::scheduler != nullptr)
        for (auto& c : coroutines_) WheelModuleEnvironment::scheduler->StopCoroutine(c);
}

// ModuleWheelSubmodule.cs:17
void ModuleWheelSubmodule::OnAwake() {
    PartModule::OnAwake();
    baseSetup = false;
    SetWheelBase();
    if (wheelBase == nullptr) throw NullReferenceException();
    wheelBase->RegisterSubmodule(this);
}

// ModuleWheelSubmodule.cs:24
void ModuleWheelSubmodule::SetWheelBase() {
    const int baseModuleIndex = Get<int>("baseModuleIndex");
    Part* p = PartPtr();
    if (baseModuleIndex == -1) {
        wheelBase = dynamic_cast<ModuleWheelBase*>(p->FindModuleOfClass("ModuleWheelBase"));
        return;
    }
    auto* m = dynamic_cast<ModuleWheelBase*>(p->Modules()[static_cast<size_t>(baseModuleIndex)]);
    if (m == nullptr) {
        log::Error("[ModuleWheelSubmodule]: Module at index " + std::to_string(baseModuleIndex) + " is not a ModuleWheelBase type");
    } else {
        wheelBase = m;
    }
}

// ModuleWheelSubmodule.cs:42
void ModuleWheelSubmodule::OnDestroy() {
    if (wheelBase == nullptr) return;
    if (LiveBases().count(wheelBase) == 0) {
        wheelBase = nullptr;
        return;
    }
    wheelBase->UnregisterSubmodule(this);
    if (wheelBase->InopSystems() != nullptr && wheelBase->InopSystems()->HasOnModified()) wheelBase->InopSystems()->RemoveOnModified(this);
    wheelBase = nullptr;
}

// ModuleWheelSubmodule.cs:55
void ModuleWheelSubmodule::OnWheelInit(KSPWheelController* w) {
    wheel = w;
    baseSetup = true;
    OnWheelSetup();
    wheelBase->InopSystems()->CombineOnModified(this, [this](WheelSubsystems* s) { OnSubsystemsModified(s); });
}

// ---- ModuleWheelMotor ----------------------------------------------------------------------------------------------------------------------
// ModuleWheelMotor.cs:89
void ModuleWheelMotor::OnAwake() {
    if (IsNull("torqueCurve")) Set<FloatCurve>("torqueCurve", FloatCurve{});
    consumedResources.clear();
    for (size_t i = 0, count = resHandler.inputResources.size(); i < count; i++) {
        const PartResourceLibrary* lib = PartResourceLibrary::Instance();
        consumedResources.push_back(lib ? lib->GetDefinition(std::string_view(resHandler.inputResources[i]->name)) : nullptr);
    }
    ModuleWheelSubmodule::OnAwake();
    Events().Bind("EvtAutoTorqueToggle", BaseEvent::Handler([this] { EvtAutoTorqueToggle(); }));
    Actions().Bind("MotorToggle", [this](KSPActionParam& p) { MotorToggle(p); });
    Actions().Bind("MotorEnable", [this](KSPActionParam& p) { MotorEnable(p); });
    Actions().Bind("MotorDisable", [this](KSPActionParam& p) { MotorDisable(p); });
    Actions().Bind("ActAutoTorqueToggle", [this](KSPActionParam& p) { ActAutoTorqueToggle(p); });
}

void ModuleWheelMotor::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* m = dynamic_cast<const ModuleWheelMotor*>(&from)) {
        maxTorque = m->maxTorque;
        state = m->state;
    }
}

// ModuleWheelMotor.cs:111
void ModuleWheelMotor::OnStart(StartState) {
    avgResRate = resHandler.GetAverageInput();
    Events()["EvtAutoTorqueToggle"]->SetGuiName(Localizer::Format("#autoLOC_7001004", std::vector<std::string>{std::to_string(Get<bool>("autoTorque") ? 1 : 0)}));
    ActionUIUpdate();
}

// ModuleWheelMotor.cs:122
void ModuleWheelMotor::OnWheelSetup() {
    float mn = 0.0f;
    Get<FloatCurve>("torqueCurve").FindMinMaxValue(mn, maxTorque);
    wheel->maxDriveTorque = maxTorque;
    wheel->maxDriveTorque = f32store(dotnet::Math::Round(fx(wheel->maxDriveTorque) * 100.0) / 100.0);
    wheel->maxRpm = WheelUtil::SpeedToRpm(Get<float>("wheelSpeedMax"), wheelBase->Get<float>("radius"));
    wheel->driveResponse = Get<float>("driveResponse");
}

// ModuleWheelMotor.cs:131
void ModuleWheelMotor::FixedUpdate() {
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight() || !baseSetup) return;
    Part* p = PartPtr();
    Vessel* vessel = p->vessel;
    if (Get<bool>("motorEnabled") && !p->fields.packed) {
        driveInput = Mathf::Clamp(f32store(fx(vessel->ctrlState.wheelThrottle) + fx(vessel->ctrlState.wheelThrottleTrim)), -1.0f, 1.0f);
        driveInput = f32store(fx(driveInput) * fx(GetMotorOrientationSign()));
        driveInput = f32store(fx(driveInput) * (Get<bool>("motorInverted") ? -1.0 : 1.0));
        wheel->maxDriveTorque = Get<FloatCurve>("torqueCurve").Evaluate(Mathf::Abs(f32store(fx(wheel->currentState()->angularVelocity) * fx(wheelBase->Get<float>("radius")))));
        wheel->maxDriveTorque = f32store(dotnet::Math::Round(fx(wheel->maxDriveTorque) * 100.0) / 100.0);
        driveInput = Mathf::Clamp(OnDriveUpdate(driveInput), -1.0f, 1.0f);
        if (Get<bool>("autoTorque")) {
            const float tcs = Get<float>("tractionControlScale");
            driveInput = f32store(fx(driveInput) * (1.0 - fx(Mathf::Abs(f32store(fx(wheelBase->slipDisplacement.y) * fx(tcs))))));
            Set<float>("driveLimiter", f32store(fx(Mathf::Lerp(1.0f, f32store(vessel->mainBody()->GeeASL), Mathf::Clamp01(f32store(fx(tcs) / 2.0)))) * 100.0));
        }
        driveInput = f32store(fx(driveInput) * (fx(Get<float>("driveLimiter")) * fx(0.01f)));
        if (maxTorque > 0.0f) {
            driveInput = ApplyTorqueAdjustments(driveInput);
            std::string stateString = Get<std::string>("motorStateString");
            const bool ok = CheckMotorState(Mathf::Abs(driveInput), stateString, resourceFraction, f32store(fx(wheel->maxDriveTorque) / fx(maxTorque)));
            Set<std::string>("motorStateString", stateString);
            if (ok) {
                wheel->driveInput = f32store(fx(driveInput) * fx(f32store(resourceFraction)));
            } else {
                wheel->driveInput = 0.0f;
            }
            Set<float>("driveOutput",
                       f32store(fx(Mathf::Abs(f32store(fx(driveInput) * fx(wheel->maxDriveTorque) / fx(maxTorque)))) * 100.0 * fx(f32store(resourceFraction))));
        } else {
            Set<float>("driveOutput", 0.0f);
            state = MotorState::Disabled;
        }
    } else {
        driveInput = 0.0f;
        Set<float>("driveOutput", 0.0f);
        wheel->driveInput = 0.0f;
    }
}

// ModuleWheelMotor.cs:183
float ModuleWheelMotor::GetMotorOrientationSign() {
    Part* p = PartPtr();
    Vessel* vessel = p->vessel;
    PartTransform& ref = vessel->ReferenceTransform();
    const float num = Vector3f::Dot(p->partTransform.up(), ref.up());
    const float num2 = Vector3f::Dot(p->partTransform.forward(), ref.forward());
    const float num3 = Vector3f::Dot(p->partTransform.forward(), ref.up());
    float num4 = std::numeric_limits<float>::lowest();   // float.MinValue
    float f = num;
    float num5 = Mathf::Abs(num3);
    if (num5 > num4) {
        num4 = num5;
        f = num3;
    }
    num5 = Mathf::Abs(num2);
    if (num5 > num4) {
        num4 = num5;
        f = num2;
    }
    return Mathf::Sign(f);
}

// ModuleWheelMotor.cs:205
bool ModuleWheelMotor::CheckMotorState(float driveInputArg, std::string& stateString, double& rOutput, float torqueScalar) {
    if (GetMotorEnabled(Get<bool>("motorEnabled"), stateString)) {
        const double num = static_cast<double>(driveInputArg) * static_cast<double>(torqueScalar) + Get<double>("idleDrain");
        rOutput = resHandler.UpdateModuleResourceInputs(stateString, num, 0.999, false, false, false);
        if (rOutput > 0.5) {
            if (driveInputArg == 0.0f) {
                state = MotorState::Idle;
                return true;
            }
            state = MotorState::Running;
            return true;
        }
        state = MotorState::NotEnoughResources;
        return rOutput > 0.1;
    }
    rOutput = 0.0;
    return false;
}

// ModuleWheelMotor.cs:240
bool ModuleWheelMotor::GetMotorEnabled(bool baseMotorEnabled, std::string& stateString) {
    if (wheelBase->InopSystems()->HasType(WheelSubsystem::Motor)) {
        stateString = Localizer::Format("#autoLOC_247987");
        state = MotorState::Inoperable;
        return false;
    }
    if (!baseMotorEnabled) {
        stateString = Localizer::Format("#autoLOC_6001071");
        state = MotorState::Disabled;
    }
    return baseMotorEnabled;
}

// ModuleWheelMotor.cs:256
float ModuleWheelMotor::GetMaxSpeed() { return Get<FloatCurve>("torqueCurve").maxTime(); }

// ModuleWheelMotor.cs:267
void ModuleWheelMotor::EvtAutoTorqueToggle() {
    Set<bool>("autoTorque", !Get<bool>("autoTorque"));
    ActionUIUpdate();
    ATsymPartUpdate();
}
// ModuleWheelMotor.cs:275
void ModuleWheelMotor::MotorToggle(KSPActionParam&) { Set<bool>("motorEnabled", !Get<bool>("motorEnabled")); }
// ModuleWheelMotor.cs:281
void ModuleWheelMotor::MotorEnable(KSPActionParam&) { Set<bool>("motorEnabled", true); }
// ModuleWheelMotor.cs:287
void ModuleWheelMotor::MotorDisable(KSPActionParam&) { Set<bool>("motorEnabled", false); }
// ModuleWheelMotor.cs:293
void ModuleWheelMotor::ActAutoTorqueToggle(KSPActionParam& act) {
    Set<bool>("autoTorque", act.type != 0 || (act.type == static_cast<int>(KSPActionType::Toggle) && !Get<bool>("autoTorque")));
    ActionUIUpdate();
}

// ModuleWheelMotor.cs:299
void ModuleWheelMotor::ActionUIUpdate() {
    BaseField* fldDriveLimiter = Fields()["driveLimiter"];
    BaseField* fldTractionControl = Fields()["tractionControlScale"];
    const bool auto_ = Get<bool>("autoTorque");
    fldDriveLimiter->guiActive = !auto_;
    fldDriveLimiter->guiActiveEditor = !auto_;
    fldTractionControl->guiActive = auto_;
    fldTractionControl->guiActiveEditor = auto_;
}

// ModuleWheelMotor.cs:311
void ModuleWheelMotor::ATsymPartUpdate() {
    Part* p = PartPtr();
    for (size_t i = 0, count = p->symmetryCounterparts.size(); i < count; i++) {
        auto* obj = dynamic_cast<ModuleWheelMotor*>(p->symmetryCounterparts[i]->Modules()[static_cast<size_t>(p->Modules().IndexOf(this))]);
        obj->Set<bool>("autoTorque", Get<bool>("autoTorque"));
        obj->ActionUIUpdate();
    }
}

// ModuleWheelMotor.cs:331
void ModuleWheelMotor::RemoveAdjuster(IWheelTorqueAdjuster* a) {
    auto it = std::find(adjusterCache.begin(), adjusterCache.end(), a);
    if (it != adjusterCache.end()) adjusterCache.erase(it);
}

// ModuleWheelMotor.cs:338
float ModuleWheelMotor::ApplyTorqueAdjustments(float torque) {
    for (size_t i = 0; i < adjusterCache.size(); i++) torque = adjusterCache[i]->ApplyTorqueAdjustment(torque);
    return torque;
}

// ---- ModuleWheelMotorSteering --------------------------------------------------------------------------------------------------------------
// ModuleWheelMotorSteering.cs:34
float ModuleWheelMotorSteering::OnDriveUpdate(float motorInput) {
    Part* p = PartPtr();
    Vessel* vessel = p->vessel;
    if (Get<bool>("steeringEnabled")) {
        wFwd = p->partTransform.forward();
        Rcom = (p->partTransform.position - vessel->CurrentCoM()).normalized();
        wXcom = Vector3f::Cross(wFwd, Rcom);
        steerAxisInput = -vessel->ReferenceTransform().forward() * f32store(fx(vessel->ctrlState.wheelSteer) + fx(vessel->ctrlState.wheelSteerTrim));
        wDot = Vector3f::Dot(wXcom, steerAxisInput);
        if (Mathf::Abs(wDot) > 0.1f) {
            wDot = Mathf::Sign(wDot);
            if (Get<bool>("steeringInvert")) wDot = f32store(fx(wDot) * -1.0);
            wheel->maxDriveTorque = Get<float>("steeringTorque");
        } else {
            wDot = 0.0f;
        }
        return f32store(fx(motorInput) + fx(wDot));
    }
    return motorInput;
}

// ModuleWheelMotorSteering.cs:61
bool ModuleWheelMotorSteering::GetMotorEnabled(bool baseMotorEnabled, std::string& stateString) {
    if (wheelBase->InopSystems()->HasType(WheelSubsystem::Steering)) {
        stateString = "Inoperative";
        return false;
    }
    if (!baseMotorEnabled && !Get<bool>("steeringEnabled")) {
        stateString = "Disabled";
        return false;
    }
    return true;
}

// ---- ModuleWheelBrakes ---------------------------------------------------------------------------------------------------------------------
// ModuleWheelBrakes.cs:30
void ModuleWheelBrakes::OnStart(StartState) {
    Part* p = PartPtr();
    Actions().Bind("BrakeAction", [this](KSPActionParam& k) { BrakeAction(k); });
    const int idx = Get<int>("statusLightModuleIndex");
    if (idx != -1) {
        IWheelStatusLight* light = WheelModuleEnvironment::asStatusLight ? WheelModuleEnvironment::asStatusLight(*p->Modules()[static_cast<size_t>(idx)]) : nullptr;
        if (light != nullptr) {
            statusLight = light;
        } else {
            log::Error("[ModuleWheelBrakes]: Module at statusLightModuleIndex " + std::to_string(idx) + " in " + p->gameObjectName + " is not of type ModuleStatusLight!");
        }
    }
    if (Get<float>("brakeInput") < 0.0f) {
        Set<float>("brakeInput", 0.0f);
        if (ResourceFlowEnvironment::LoadedSceneIsFlight() && p->vessel->ActionGroups()[KSPActionGroup::Brakes]) Set<float>("brakeInput", 1.0f);
    }
}

// ModuleWheelBrakes.cs:55
void ModuleWheelBrakes::OnWheelSetup() {
    wheel->maxBrakeTorque = Get<float>("maxBrakeTorque");
    wheel->brakeResponse = Get<float>("brakeResponse");
    if (statusLight != nullptr) statusLight->SetStatus(Get<float>("brakeInput") > 0.0f && Get<float>("brakeTweakable") > 0.0f);
}

// ModuleWheelBrakes.cs:66
void ModuleWheelBrakes::BrakeAction(KSPActionParam& kPar) {
    KSPActionType kSPActionType = kPar.Type();
    if (kSPActionType == KSPActionType::Toggle) kSPActionType = (!(Get<float>("brakeInput") <= 0.0f)) ? KSPActionType::Deactivate : KSPActionType::Activate;
    switch (kSPActionType) {
    case KSPActionType::Deactivate: Set<float>("brakeInput", 0.0f); break;
    case KSPActionType::Activate: Set<float>("brakeInput", 1.0f); break;
    default: break;
    }
    if (statusLight != nullptr) statusLight->SetStatus(kPar.Type() == KSPActionType::Activate && Get<float>("brakeTweakable") > 0.0f);
}

// ModuleWheelBrakes.cs:88
void ModuleWheelBrakes::FixedUpdate() {
    if (ResourceFlowEnvironment::LoadedSceneIsFlight() && baseSetup) {
        Part* p = PartPtr();
        if (!p->fields.packed && !wheelBase->InopSystems()->HasType(WheelSubsystem::Brakes)) {
            const float num = Mathf::Min(1.0f, Mathf::Lerp(1.0f, f32store(p->vessel->mainBody()->GeeASL), Mathf::Abs(wheel->currentState()->angularVelocity)));
            wheel->brakeInput = f32store(fx(ApplyTorqueAdjustments(Get<float>("brakeInput"))) * (fx(Get<float>("brakeTweakable")) * fx(0.01f)) * fx(num));
            wheel->brakeResponse = (wheel->brakeInput != 0.0f) ? Get<float>("brakeResponse") : 100.0f;
        } else {
            wheel->brakeResponse = Get<float>("brakeResponse");
            wheel->brakeInput = 0.0f;
        }
    }
}

// ModuleWheelBrakes.cs:111
void ModuleWheelBrakes::OnSubsystemsModified(WheelSubsystems* s) {
    if (s != wheelBase->InopSystems() || !(statusLight != nullptr)) return;
    if (s->HasType(WheelSubsystem::Brakes)) {
        if (statusLight->IsOn()) statusLight->SetStatus(false);
    } else if (Get<float>("brakeInput") > 0.0f) {
        statusLight->SetStatus(true);
    }
}

void ModuleWheelBrakes::RemoveAdjuster(IWheelTorqueAdjuster* a) {
    auto it = std::find(adjusterCache.begin(), adjusterCache.end(), a);
    if (it != adjusterCache.end()) adjusterCache.erase(it);
}

// ModuleWheelBrakes.cs:146
float ModuleWheelBrakes::ApplyTorqueAdjustments(float torque) {
    for (size_t i = 0; i < adjusterCache.size(); i++) torque = adjusterCache[i]->ApplyTorqueAdjustment(torque);
    return torque;
}

// ---- ModuleWheelLock -----------------------------------------------------------------------------------------------------------------------
// ModuleWheelLock.cs:8
void ModuleWheelLock::OnWheelSetup() {
    wheel->maxBrakeTorque = Get<float>("maxTorque");
    wheel->brakeResponse = 1000.0f;
    wheel->brakeInput = 1.0f;
}

// ---- ModuleWheelSuspension -----------------------------------------------------------------------------------------------------------------
ModuleWheelSuspension::~ModuleWheelSuspension() {
    if (subscribed_) GameEvents::onVesselPartCountChanged.Remove(this, &ModuleWheelSuspension::OnVesselPartCountChanged);
}

// ModuleWheelSuspension.cs:178
void ModuleWheelSuspension::OnAwake() {
    ModuleWheelSubmodule::OnAwake();
    Set<bool>("autoSpringDamper", GameSettings::WHEEL_AUTO_SPRINGDAMPER || Get<float>("maximumLoad") <= 0.0f);
    Events().Bind("EvtAutoSpringDamperToggle", BaseEvent::Handler([this] { EvtAutoSpringDamperToggle(); }));
}

// ModuleWheelSuspension.cs:184
void ModuleWheelSuspension::OnStart(StartState) {
    ActionSpringDamperUIUpdate();
    const std::string name = Get<std::string>("suspensionTransformName");
    Part* p = PartPtr();
    if (!name.empty()) {
        suspensionTransform_ = WheelModuleEnvironment::findModelTransform ? WheelModuleEnvironment::findModelTransform(*p, name) : nullptr;
        if (suspensionTransform_ == nullptr) {
            log::Error("[ModuleWheelSuspension]: No transform called " + name + " found in " + p->gameObjectName + " hierarchy");
        } else {
            if (Get<Vector3f>("suspensionDefaultPos") == -Vector3f::one()) Set<Vector3f>("suspensionDefaultPos", suspensionTransform_->position);
            if (Get<Vector3f>("suspensionPos") != -Vector3f::one()) suspensionTransform_->position = Get<Vector3f>("suspensionPos");
        }
    }
    GameEvents::onVesselPartCountChanged.Add(this, &ModuleWheelSuspension::OnVesselPartCountChanged);
    subscribed_ = true;
}

// ModuleWheelSuspension.cs:222
void ModuleWheelSuspension::OnDestroy() {
    if (subscribed_) GameEvents::onVesselPartCountChanged.Remove(this, &ModuleWheelSuspension::OnVesselPartCountChanged);
    subscribed_ = false;
}

// ModuleWheelSuspension.cs:228 (the transform bookkeeping of the inventory modes is the host's: the position fields are what is persisted)
void ModuleWheelSuspension::OnInventoryModeDisable() {
    if (subscribed_) GameEvents::onVesselPartCountChanged.Remove(this, &ModuleWheelSuspension::OnVesselPartCountChanged);
    subscribed_ = false;
    Set<Vector3f>("suspensionPos", -Vector3f::one());
    if (wheelBase == nullptr) SetWheelBase();
}

// ModuleWheelSuspension.cs:255
void ModuleWheelSuspension::OnInventoryModeEnable() {
    Set<Vector3f>("suspensionPos", -Vector3f::one());
    if (wheelBase == nullptr) SetWheelBase();
}

// ModuleWheelSuspension.cs:281
void ModuleWheelSuspension::OnVesselPartCountChanged(Vessel* changedVessel) {
    if (baseSetup && changedVessel->id == VesselPtr()->id) wheel->wheelCollider->springRate = 0.0f;
}

// ModuleWheelSuspension.cs:289
void ModuleWheelSuspension::OnWheelSetup() {
    Part* p = PartPtr();
    const float rescale = p->fields.rescaleFactor;
    if (suspensionTransform_ != nullptr) wheel->wheelCollider->suspensionTransform = suspensionTransform_;  // ModuleWheelSuspension.cs:291
    wheel->wheelCollider->updateSuspension = !wheelBase->InopSystems()->HasType(WheelSubsystem::Suspension);
    wheel->wheelCollider->suspensionDistance = f32store(fx(Get<float>("suspensionDistance")) * fx(rescale));
    wheel->wheelCollider->suspensionAnchor = Get<float>("targetPosition");
    wheel->wheelCollider->suspensionOffset = f32store(fx(Get<float>("suspensionOffset")) * fx(rescale));
    wheel->wheelCollider->springRate = spring;
    wheel->wheelCollider->damperRate = damper;
    ResetSuspensionCollider(3.0f);
}

// ModuleWheelSuspension.cs:315
void ModuleWheelSuspension::FixedUpdate() {
    Part* p = PartPtr();
    if (ResourceFlowEnvironment::LoadedSceneIsFlight() && baseSetup && !p->fields.packed && wheel != nullptr && wheel->IsGrounded()) {
        vesselMass = static_cast<float>(p->vessel->totalMass);
        SuspensionSpringUpdate(vesselMass);
    }
}

// ModuleWheelSuspension.cs:324 (the transform write-back is the host's; the persisted suspensionPos follows suspensionTransform.localPosition)
void ModuleWheelSuspension::LateUpdate() {
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        if (Get<Vector3f>("suspensionDefaultPos") == -Vector3f::one() && suspensionTransform_ != nullptr)
            Set<Vector3f>("suspensionDefaultPos", suspensionTransform_->position);
        return;
    }
    Part* p = PartPtr();
    if (p->State() == PartStates::CARGO) return;
    if (baseSetup && !p->fields.packed) {
        if (suspensionTransform_ != nullptr) Set<Vector3f>("suspensionPos", suspensionTransform_->position);
    } else if (suspensionTransform_ != nullptr && Get<Vector3f>("suspensionPos") != -Vector3f::one()) {
        suspensionTransform_->position = Get<Vector3f>("suspensionPos");
    }
}

// ModuleWheelSuspension.cs:350
float ModuleWheelSuspension::BoostCurve(float b, float power) {
    return f32store(fx(Mathf::Round(f32store(fx(Mathf::Clamp(f32store(1.0 / fx(Mathf::Abs(f32store(1.0 - 2.0 / fx(Mathf::Pow(Mathf::Clamp(b, 0.0f, 2.0f), Mathf::Clamp(power, 0.01f, autoBoostClamp))))))), 0.01f, 10000000.0f)) * 10.0))) / 10.0);
}

// ModuleWheelSuspension.cs:355
void ModuleWheelSuspension::SuspensionSpringUpdate(float sprungMass) {
    Part* p = PartPtr();
    Vessel* vessel = p->vessel;
    const double gee = vessel->mainBody()->GeeASL;
    suspCompression = wheel->currentState()->suspensionCompression;
    suspMass = sprungMass;
    const float num = f32store(fx(sprungMass) * fx(suspCompression));
    springClampMax = Mathf::Lerp(5000.0f, 10000.0f, Mathf::InverseLerp(50.0f, 200.0f, sprungMass));
    damperClampMax = Mathf::Lerp(5000.0f, 10000.0f, Mathf::InverseLerp(50.0f, 200.0f, sprungMass));
    UpdateAutoBoost(suspCompression);
    boost = f32store(1.0 + fx(Mathf::Clamp(f32store(fx(suspCompression) * fx(BoostCurve(f32store(static_cast<float>(gee) * fx(static_cast<float>(vessel->gravityMultiplier))), 1.0f))), -1.0f, boostClamp)));
    if (!Get<bool>("useDistributedMass")) sprungMassGravity = num;
    const bool autoSpringDamper = Get<bool>("autoSpringDamper");
    const float maximumLoad = Get<float>("maximumLoad");
    if (autoSpringDamper && (!autoSpringDamperSet || (maximumLoad > 0.0f && num > maximumLoad))) {
        if (gee < 1.0) {
            Set<float>("damperTweakable", 1.1f);
        } else {
            const float t = Mathf::InverseLerp(damperLerpBase, static_cast<float>(WheelModuleEnvironment::maximumSurfaceGeeASL ? WheelModuleEnvironment::maximumSurfaceGeeASL() : 0.0), static_cast<float>(gee));
            Set<float>("damperTweakable", static_cast<float>(dotnet::Math::Round(fx(Mathf::Lerp(1.0f, 2.0f, t)), 2, dotnet::MidpointRounding::ToEven)));
        }
        if (spring > 0.0f && (!autoSpringDamperSet || (maximumLoad > 0.0f && num > maximumLoad))) {
            const float value = f32store(fx(Get<float>("suspensionDistance")) * fx(spring) / fx(static_cast<float>(PhysicsGlobals::GravitationalAcceleration)));
            const float t2 = Mathf::InverseLerp(0.0f, f32store(fx(maximumLoad) * 2.0), value);
            Set<float>("springTweakable", static_cast<float>(dotnet::Math::Round(fx(Mathf::Lerp(1.0f, 3.0f, t2)), 2, dotnet::MidpointRounding::ToEven)));
            autoSpringDamperSet = true;
        }
    }
    damperFudge = Mathf::Lerp(1.0f, 15.0f, Mathf::InverseLerp(10.0f, 100.0f, num));
    spring = Mathf::Clamp(f32store(fx(Get<float>("springTweakable")) * fx(Get<float>("springRatio")) * fx(sprungMassGravity) *
                                   fx(BoostCurve(boost, f32store(fx(Get<float>("boostRatio")) + fx(Get<float>("autoBoost")))))),
                          0.01f, springClampMax);
    damper = f32store(fx(Mathf::Sqrt(Mathf::Clamp(f32store(fx(spring) * fx(Get<float>("damperRatio")) * fx(damperFudge)), 0.01f, damperClampMax))) * fx(Get<float>("damperTweakable")));
    if (Get<bool>("adjustForHighGee") && gee > static_cast<double>(Get<float>("highGeeThreshold"))) {
        spring = f32store(fx(spring) * fx(Get<float>("highGeeSpringTweakable")));
        damper = f32store(fx(damper) * fx(Get<float>("highGeeDamperTweakable")));
        highGeeOverride = true;
    } else {
        highGeeOverride = false;
    }
    damper = Mathf::Max(Mathf::Round(damper), 0.1f);
    spring = Mathf::Max(Mathf::Round(spring), 0.1f);
    VPWheelCollider* col = wheel->wheelCollider;
    if (f32store(fx(spring) - fx(col->springRate)) > oscillationDamper) {
        col->springRate = spring;
    } else if (col->springRate > f32store(fx(spring) + fx(oscillationDamper))) {
        col->springRate = spring;
    }
    if (Mathf::Abs(f32store(fx(damper) - fx(col->damperRate))) > 1.05f) col->damperRate = damper;
}

// ModuleWheelSuspension.cs:421
void ModuleWheelSuspension::UpdateAutoBoost(float st) {
    if (!Get<bool>("useAutoBoost")) {
        Set<float>("autoBoost", 0.0f);
    } else if (st > 0.8f) {
        Set<float>("autoBoost", Mathf::Clamp(f32store(fx(Get<float>("autoBoost")) + fx(0.2f) * fx(Time::fixedDeltaTime)), 0.85f, 2.0f));
    } else if (st < 0.4f) {
        Set<float>("autoBoost", Mathf::Max(0.0f, f32store(fx(Get<float>("autoBoost")) - fx(0.2f) * fx(Time::fixedDeltaTime))));
    }
}

// ModuleWheelSuspension.cs:446
void ModuleWheelSuspension::OnSubsystemsModified(WheelSubsystems* s) {
    if (s == wheelBase->InopSystems()) wheel->wheelCollider->updateSuspension = !s->HasType(WheelSubsystem::Suspension);
}

// ModuleWheelSuspension.cs:454 (the suspension collider belongs to the model; the delayed enable is traced)
void ModuleWheelSuspension::ResetSuspensionCollider(float delay) {
    if (!Get<std::string>("suspensionColliderName").empty()) {
        StartCoroutine([this](float d) -> Enumerator {
            co_yield YieldInstruction::WaitForSeconds(d);
            if (WheelModuleEnvironment::suspensionColliderEnable) WheelModuleEnvironment::suspensionColliderEnable(*PartPtr());
        }(delay));
    }
}

// ModuleWheelSuspension.cs:144
void ModuleWheelSuspension::EvtAutoSpringDamperToggle() {
    Set<bool>("autoSpringDamper", !Get<bool>("autoSpringDamper"));
    autoSpringDamperSet = false;
    ActionSpringDamperUIUpdate();
    ATsymPartUpdate();
}

// ModuleWheelSuspension.cs:152
void ModuleWheelSuspension::ActionSpringDamperUIUpdate() {
    BaseEvent* evt = Events()["EvtAutoSpringDamperToggle"];
    BaseField* fldSpring = Fields()["springTweakable"];
    BaseField* fldDamper = Fields()["damperTweakable"];
    evt->guiActive = true;
    evt->guiActiveEditor = true;
    evt->SetGuiName(Localizer::Format("#autoLOC_8002214", std::vector<std::string>{std::to_string(Get<bool>("autoSpringDamper") ? 1 : 0)}));
    const bool show = !Get<bool>("autoSpringDamper") || !GameSettings::WHEEL_AUTO_SPRINGDAMPER;
    fldSpring->guiActive = show;
    fldSpring->guiActiveEditor = show;
    fldDamper->guiActive = show;
    fldDamper->guiActiveEditor = show;
}

// ModuleWheelSuspension.cs:163
void ModuleWheelSuspension::ATsymPartUpdate() {
    Part* p = PartPtr();
    for (size_t i = 0, count = p->symmetryCounterparts.size(); i < count; i++) {
        auto* m = dynamic_cast<ModuleWheelSuspension*>(p->symmetryCounterparts[i]->FindModuleOfClass("ModuleWheelSuspension"));
        if (m != nullptr) {
            m->Set<bool>("autoSpringDamper", Get<bool>("autoSpringDamper"));
            m->autoSpringDamperSet = false;
            m->ActionSpringDamperUIUpdate();
        }
    }
}

// ---- ModuleWheelSteering -------------------------------------------------------------------------------------------------------------------
ModuleWheelSteering::~ModuleWheelSteering() { OnDestroy(); }

// ModuleWheelSteering.cs:146
void ModuleWheelSteering::OnAwake() {
    ModuleWheelSubmodule::OnAwake();
    if (IsNull("steeringCurve")) Set<FloatCurve>("steeringCurve", FloatCurve{});
    if (IsNull("steeringMaxAngleCurve")) Set<FloatCurve>("steeringMaxAngleCurve", FloatCurve{});
    Events().Bind("EvtAutoSteeringAdjustToggle", BaseEvent::Handler([this] { EvtAutoSteeringAdjustToggle(); }));
    Actions().Bind("SteeringToggle", [this](KSPActionParam& p) { SteeringToggle(p); });
}

// ModuleWheelSteering.cs:167
void ModuleWheelSteering::OnStart(StartState) {
    steeringCurveDefined_ = Get<FloatCurve>("steeringCurve").maxTime() != std::numeric_limits<float>::lowest();
    steeringAngleCurveDefined_ = Get<FloatCurve>("steeringMaxAngleCurve").maxTime() != std::numeric_limits<float>::lowest();
    Part* p = PartPtr();
    const std::string caliper = Get<std::string>("caliperTransformName");
    if (!caliper.empty()) {
        VehiclePhysics::NativeTransform* t = WheelModuleEnvironment::findModelTransform ? WheelModuleEnvironment::findModelTransform(*p, caliper) : nullptr;
        caliperTransform_ = t;
        if (t == nullptr) {
            log::Error("[ModuleWheelBase]: No transform called " + caliper + " found in " + p->gameObjectName + " hierarchy");
            return;
        }
    }
    p->partValues.SteeringRadius.Add(this, &ModuleWheelSteering::GetCoMFwdLength);
    radiusRegistered_ = true;
    if (Get<float>("steeringRange") < 0.01f) {
        float mn = 0.0f, mx = 0.0f;
        Get<FloatCurve>("steeringCurve").FindMinMaxValue(mn, mx);
        Set<float>("steeringRange", Mathf::Max(mx, 0.01f));
    }
    if (Get<float>("angleTweakable") < 0.0f) Set<float>("angleTweakable", Get<float>("steeringRange"));
    if (Get<float>("responseTweakable") < 0.0f) Set<float>("responseTweakable", Get<float>("steeringResponse"));
    ActionSteeringAdjustUIUpdate();
    GameEvents::onWheelRepaired.Add(this, &ModuleWheelSteering::OnWheelRepaired);
    subscribed_ = true;
}

// ModuleWheelSteering.cs:211
void ModuleWheelSteering::OnDestroy() {
    Part* p = PartPtr();
    if (radiusRegistered_ && p != nullptr) p->partValues.SteeringRadius.Remove(this, &ModuleWheelSteering::GetCoMFwdLength);
    radiusRegistered_ = false;
    if (subscribed_) GameEvents::onWheelRepaired.Remove(this, &ModuleWheelSteering::OnWheelRepaired);
    subscribed_ = false;
}

// ModuleWheelSteering.cs:220
void ModuleWheelSteering::OnWheelSetup() {
    Part* p = PartPtr();
    if (caliperTransform_ != nullptr) wheel->wheelCollider->caliperTransform = caliperTransform_;  // ModuleWheelSteering.cs:222
    if (Get<float>("angleTweakable") < 0.0f) Set<float>("angleTweakable", Get<float>("steeringResponse"));
    wheel->maxSteerAngle = Get<float>("angleTweakable");
    if (Get<float>("responseTweakable") < 0.0f) Set<float>("responseTweakable", Get<float>("steeringRange"));
    wheel->steeringResponse = Get<float>("steeringResponse");
    SetCaliperUpdate(!wheelBase->InopSystems()->HasType(WheelSubsystem::Steering));
    if (wheelBase->wheelTransform != nullptr) {
        wLeft0_ = p->partTransform.InverseTransformDirection(-wheelBase->wheelTransform->right());
    } else {
        wLeft0_ = Vector3f::left();
    }
}

// ModuleWheelSteering.cs:247
void ModuleWheelSteering::FixedUpdate() {
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight() || !baseSetup) return;
    Part* p = PartPtr();
    Vessel* vessel = p->vessel;
    if (!p->fields.packed && Get<bool>("steeringEnabled") && caliperUpdating_) {
        UpdateCoordFrame();
        steeringInput = Mathf::Clamp(f32store(fx(vessel->ctrlState.wheelSteer) + fx(vessel->ctrlState.wheelSteerTrim)), -1.0f, 1.0f);
        if (Get<bool>("steeringInvert")) steeringInput = f32store(fx(steeringInput) * -1.0);
        wheel->steerInput = UpdateSteering(steeringInput, Get<float>("steeringRange"));
        steeringInputLast = steeringInput;
        wheel->steeringResponse = f32store(fx(Get<float>("steeringResponse")) * fx(GetSteeringResponseScale(f32store(fx(steeringInput) - fx(steeringInputLast)))));
    } else {
        wheel->steeringResponse = Get<float>("steeringResponse");
        wheel->steerInput = 0.0f;
    }
}

// ModuleWheelSteering.cs:272
float ModuleWheelSteering::GetSteeringResponseScale(float steerDelta) {
    VehicleBase::WheelState& cs = *wheel->currentState();
    float num = Mathf::Abs(cs.grounded ? cs.localWheelVelocity.y : prevLocalWheelVelocity_.y);
    if (cs.grounded) prevLocalWheelVelocity_ = cs.localWheelVelocity;
    if (steerDelta < 0.0f) num = 0.0f;
    num = f32store(fx(num) * (1.0 - fx(cs.hit.sidewaysSlip)));
    if (Get<bool>("autoSteeringAdjust")) {
        wheel->maxSteerAngle = f32store(fx(Get<float>("angleTweakable")) *
                                        (steeringAngleCurveDefined_ ? fx(Mathf::Clamp01(Get<FloatCurve>("steeringMaxAngleCurve").Evaluate(num))) : 1.0));
        wheel->maxSteerAngle = f32store(dotnet::Math::Round(fx(wheel->maxSteerAngle) * 100.0) / 100.0);
        if (steeringCurveDefined_) return f32store(fx(Get<FloatCurve>("steeringCurve").Evaluate(num)) / fx(Get<float>("steeringRange")));
        return Get<float>("responseTweakable");
    }
    wheel->maxSteerAngle = Get<float>("angleTweakable");
    return Get<float>("responseTweakable");
}

// ModuleWheelSteering.cs:298
void ModuleWheelSteering::UpdateCoordFrame() {
    Part* p = PartPtr();
    Vessel* vessel = p->vessel;
    PartTransform& ref = vessel->ReferenceTransform();
    const Vector3f vesselUp = vessel->upAxis.ToVector3f();
    const float num = Vector3f::Dot(ref.up(), vesselUp);
    upAxis_ = ref.up();
    if (num < Vector3f::Dot(-ref.forward(), vesselUp)) upAxis_ = -ref.forward();
    Vector3f referenceForward = -ref.forward();
    if (upAxis_ == ref.up()) referenceForward = ref.forward();
    fwdAxis_ = ref.up();
    if (num > Vector3f::Dot(referenceForward, vesselUp)) fwdAxis_ = referenceForward;
    leftAxis_ = Vector3f::Cross(fwdAxis_, upAxis_);
    CoM_ = vessel->CurrentCoM();
    wCenter_ = p->partTransform.TransformPoint(wheelBase->WheelOrgPosR);
    wLeft_ = Vector3f::ProjectOnPlane(p->partTransform.TransformDirection(wLeft0_), upAxis_).normalized();
    wRight_ = -wLeft_;
}

// ModuleWheelSteering.cs:323
float ModuleWheelSteering::UpdateSteering(float input, float steeringRange) {
    Vessel* vessel = VesselPtr();
    const float num = f32store(fx(steeringRange) * fx(input));
    const Vector3f vector = FindTurnCenter(num, vessel->GetVesselValues().SteeringRadius.value(), CoM_);
    if (vector != Vector3f::zero()) {
        sAxis_ = Vector3f::ProjectOnPlane(vector - wCenter_, upAxis_).normalized();
        float a = Vector3f::Dot(wRight_, sAxis_);
        a = Mathf::Max(a, Vector3f::Dot(wLeft_, sAxis_));
        steerAngle = f32store(fx(Mathf::Acos(a)) * fx(57.29578f) * fx(Mathf::Sign(input)));
    } else {
        steerAngle = 0.0f;
    }
    if (CoMfwdLength_ > 0.0f) steerAngle = f32store(0.0 - fx(steerAngle));
    if (steerAngle > 90.0f) {
        steerAngle = f32store(180.0 - fx(steerAngle));
    } else if (steerAngle < -90.0f) {
        steerAngle = f32store(-180.0 - fx(steerAngle));
    }
    return Mathf::Clamp(f32store(fx(steerAngle) / fx(wheel->maxSteerAngle)), -1.0f, 1.0f);
}

// ModuleWheelSteering.cs:355
float ModuleWheelSteering::FindCoMfwdLength(const Vector3f& vesselCoM, const Vector3f& wheelCenter, const Vector3f& fwdAxis) {
    return Vector3f::Dot(wheelCenter - vesselCoM, fwdAxis);
}

// ModuleWheelSteering.cs:360
float ModuleWheelSteering::GetCoMFwdLength() {
    CoMfwdLength_ = FindCoMfwdLength(CoM_, wCenter_, fwdAxis_);
    return Mathf::Abs(CoMfwdLength_);
}

// ModuleWheelSteering.cs:366
Vector3f ModuleWheelSteering::FindTurnCenter(float steerAngleArg, float length, const Vector3f& CoM) {
    if (steerAngleArg != 0.0f) {
        tPivot = fwdAxis_ * length;
        tOrt = (NativeOps::AngleAxis(Mathf::Abs(steerAngleArg), -upAxis_ * Mathf::Sign(steerAngleArg)) * leftAxis_) * Mathf::Sign(steerAngleArg);
        h = f32store(fx(length) * (1.0 / fx(Mathf::Tan(f32store(fx(Mathf::Abs(steerAngleArg)) * fx(Mathf::Deg2Rad))))));
        tCenter = CoM + tPivot + tOrt * h;
    } else {
        tCenter = Vector3f::zero();
    }
    return tCenter;
}

// ModuleWheelSteering.cs:206
void ModuleWheelSteering::SteeringToggle(KSPActionParam& act) {
    Set<bool>("steeringEnabled", act.type == static_cast<int>(KSPActionType::Activate) || (act.type == static_cast<int>(KSPActionType::Toggle) && !Get<bool>("steeringEnabled")));
}

// ModuleWheelSteering.cs:104
void ModuleWheelSteering::EvtAutoSteeringAdjustToggle() {
    Set<bool>("autoSteeringAdjust", !Get<bool>("autoSteeringAdjust"));
    ActionSteeringAdjustUIUpdate();
    ATsymPartUpdate();
}

// ModuleWheelSteering.cs:111
void ModuleWheelSteering::ActionSteeringAdjustUIUpdate() {
    BaseEvent* evt = Events()["EvtAutoSteeringAdjustToggle"];
    BaseField* fldResponse = Fields()["responseTweakable"];
    BaseField* fldAngle = Fields()["angleTweakable"];
    if (evt != nullptr) {
        evt->guiActive = true;
        evt->guiActiveEditor = true;
        evt->SetGuiName(Localizer::Format("#autoLOC_6002674", std::vector<std::string>{std::to_string(Get<bool>("autoSteeringAdjust") ? 1 : 0)}));
    }
    const bool show = !Get<bool>("autoSteeringAdjust") || !GameSettings::WHEEL_AUTO_STEERINGADJUST;
    if (fldResponse != nullptr) {
        fldResponse->guiActive = show;
        fldResponse->guiActiveEditor = show;
    }
    if (fldAngle != nullptr) {
        fldAngle->guiActive = show;
        fldAngle->guiActiveEditor = show;
    }
}

// ModuleWheelSteering.cs:132
void ModuleWheelSteering::ATsymPartUpdate() {
    Part* p = PartPtr();
    for (size_t i = 0, count = p->symmetryCounterparts.size(); i < count; i++) {
        auto* m = dynamic_cast<ModuleWheelSteering*>(p->symmetryCounterparts[i]->FindModuleOfClass("ModuleWheelSteering"));
        if (m != nullptr) {
            m->Set<bool>("autoSteeringAdjust", Get<bool>("autoSteeringAdjust"));
            m->ActionSteeringAdjustUIUpdate();
        }
    }
}

// ModuleWheelSteering.cs:418
void ModuleWheelSteering::OnSubsystemsModified(WheelSubsystems* s) {
    if (s == wheelBase->InopSystems()) SetCaliperUpdate(!s->HasType(WheelSubsystem::Steering));
}

// ModuleWheelSteering.cs:426
void ModuleWheelSteering::SetCaliperUpdate(bool update) {
    caliperUpdating_ = update;
    if (wheel != nullptr && wheel->wheelCollider != nullptr) wheel->wheelCollider->updateCaliper = update;
    ResetCaliper(!caliperUpdating_);
}

// ModuleWheelSteering.cs:436
void ModuleWheelSteering::ResetCaliper(bool resetTransform) {
    steerAngle = 0.0f;
    steeringInput = 0.0f;
    (void)resetTransform;   // caliperTransform.localRotation = caliperRot0: the model transform is the host's
}

// ModuleWheelSteering.cs:446
void ModuleWheelSteering::OnWheelRepaired(Part* p) {
    if (p != nullptr && p == PartPtr()) ResetCaliper(true);
}

// ---- ModuleWheelDamage ---------------------------------------------------------------------------------------------------------------------
ModuleWheelDamage::~ModuleWheelDamage() { OnDestroy(); }

// ModuleWheelDamage.cs:105
bool ModuleWheelDamage::IsImpactDamageEnable() {
    if (Get<float>("impactDamageVelocity") > 0.0f) {
        if (GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED && impactDamageColliderPresent) return true;
        return GameSettings::WHEEL_DAMAGE_WHEELCOLLIDER_ENABLED;
    }
    return false;
}

// ModuleWheelDamage.cs:121
void ModuleWheelDamage::OnStart(StartState) {
    Events().Bind("EventRepairExternal", BaseEvent::Handler([this] { EventRepairExternal(); }));
    Part* p = PartPtr();
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        // onPartCoupleComplete / onPartDeCoupleComplete / onVesselWasModified / onVesselChange: the stress model's immunity reset (host events)
        subscribed_ = true;
    }
    repairKitsNecessary = std::min(std::max(static_cast<int>(fx(p->fields.mass) / fx(GameSettings::PART_REPAIR_MASS_PER_KIT)), 1), GameSettings::PART_REPAIR_MAX_KIT_AMOUNT);
    BaseEvent* eventRepairExternal = Events()["EventRepairExternal"];
    eventRepairExternal->active = Get<bool>("isDamaged") && Get<bool>("isRepairable");
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) Set<std::string>("brokenStatusWarning", Localizer::Format("#autoLOC_6005093"));
    subsystemDamage_ = WheelSubsystem("Wheel Damage", 33, this);
    Set<bool>("isDamaged", !Get<bool>("isDamaged"));
    SetDamaged(!Get<bool>("isDamaged"));
}

// ModuleWheelDamage.cs:208
void ModuleWheelDamage::OnInventoryModeEnable() {
    Set<bool>("isDamaged", !Get<bool>("isDamaged"));
    SetDamaged(!Get<bool>("isDamaged"));
}

// ModuleWheelDamage.cs:219
void ModuleWheelDamage::OnInventoryModeDisable() {
    Set<bool>("isDamaged", !Get<bool>("isDamaged"));
    SetDamaged(!Get<bool>("isDamaged"));
}

// ModuleWheelDamage.cs:230
void ModuleWheelDamage::OnDestroy() { subscribed_ = false; }

// ModuleWheelDamage.cs:263
void ModuleWheelDamage::ResetImmunity() {
    startupTime = 8.0f;
    repairImmunityTimeTotal_ = 30.0f;
}

// ModuleWheelDamage.cs:269
void ModuleWheelDamage::OnWheelSetup() {
    StartCoroutine(DelayedFrames(1, [this] {
        Set<bool>("isDamaged", !Get<bool>("isDamaged"));
        SetDamaged(!Get<bool>("isDamaged"));
    }));
}

// ModuleWheelDamage.cs:278
void ModuleWheelDamage::Initialize() {
    currentDeflection = f32store(fx(Get<float>("deflectionMagnitude")) * fx(wheel->LegacyWheelLoad()));
    currentSlip = f32store(fx(Get<float>("slipMagnitude")) * fx(wheel->LegacyWheelLoad()) * fx(wheelBase->slipDisplacement.magnitude()));
    currentDownForce = wheel->currentState()->downforce;
    lastDeflection = currentDeflection;
    lastSlip = currentSlip;
    lastDownForce = currentDownForce;
    initialized = true;
}

// ModuleWheelDamage.cs:289
void ModuleWheelDamage::FixedUpdate() {
    Part* p = PartPtr();
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight() || !baseSetup || (!(p->vessel == nullptr) && p->vessel->situation == Vessel::Situations::PRELAUNCH)) return;
    if (!p->fields.packed && !Get<bool>("isDamaged")) {
        if (!initialized) Initialize();
        lastDeflection = currentDeflection;
        lastSlip = currentSlip;
        lastDownForce = currentDownForce;
        if (repairImmunityTimeLeft_ > 0.0f) repairImmunityTimeLeft_ = Mathf::Max(0.0f, f32store(fx(repairImmunityTimeLeft_) - fx(Time::fixedDeltaTime)));
        const float num = f32store(1.0 - fx(repairImmunityTimeLeft_) / fx(repairImmunityTimeTotal_));
        currentDeflection = f32store(fx(Get<float>("deflectionMagnitude")) * fx(wheel->LegacyWheelLoad()));
        currentSlip = f32store(fx(Get<float>("slipMagnitude")) * fx(wheel->LegacyWheelLoad()) * fx(wheelBase->slipDisplacement.magnitude()));
        currentDeflection = Mathf::Lerp(lastDeflection, currentDeflection, f32store(fx(Get<float>("deflectionSharpness")) * fx(TimeWarp::fixedDeltaTime())));
        currentSlip = Mathf::Lerp(lastSlip, currentSlip, f32store(fx(Get<float>("slipSharpness")) * fx(TimeWarp::fixedDeltaTime())));
        currentDownForce = wheel->currentState()->downforce;
        if (startupTime > 0.0f) {
            startupTime = f32store(fx(startupTime) - fx(Time::deltaTime));
            return;
        }
        const float num2 = f32store(fx(currentDeflection) * fx(num) * fx(GameSettings::WHEEL_WEIGHT_STRESS_MULTIPLIER));
        const float num3 = f32store(fx(currentSlip) * fx(num) * fx(GameSettings::WHEEL_SLIP_STRESS_MULTIPLIER));
        const float stressTolerance = Get<float>("stressTolerance");
        totalStress = Mathf::Clamp(f32store(fx(num2) + fx(num3)), 0.0f, stressTolerance);
        Set<float>("stressPercent", f32store(fx(totalStress) / fx(stressTolerance) * 100.0));
        const float num4 = f32store(fx(stressTolerance) + fx(stressVariability));
        if (totalStress >= num4) {
            stressTime_ = f32store(fx(stressTime_) + fx(TimeWarp::fixedDeltaTime()));
            const float num5 = f32store(1.0 - fx(Mathf::Clamp01(f32store((fx(totalStress) - fx(num4)) / fx(f32store(fx(num4) / 2.0))))));
            const float num6 = f32store(0.25 + 0.25 * fx(num5));
            if (stressTime_ > num6) SetDamaged(true);
        } else {
            stressTime_ = 0.0f;
        }
        const float num7 = f32store(fx(f32store(fx(currentDownForce) - fx(lastDownForce))) * fx(wheel->gravity.Magnitude()) / fx(static_cast<float>(PhysicsGlobals::GravitationalAcceleration)));
        if (!WheelModuleEnvironment::noCrashDamage) {
            const float impactTolerance = Get<float>("impactTolerance");
            if (num7 >= f32store(fx(impactTolerance) * fx(Get<float>("explodeMultiplier")))) {
                WheelModuleEnvironment::Trace("explode");
            } else if (num7 >= impactTolerance) {
                SetDamaged(true);
            }
        }
    } else {
        currentDeflection = 0.0f;
        currentSlip = 0.0f;
        currentDownForce = 0.0f;
        stressTime_ = 0.0f;
        initialized = false;
        startupTime = 8.0f;
        repairImmunityTimeTotal_ = 30.0f;
        repairImmunityTimeLeft_ = 0.0f;
        totalStress = 0.0f;
        Set<float>("stressPercent", 0.0f);
    }
}

// ModuleWheelDamage.cs:368
void ModuleWheelDamage::SetDamaged(bool damaged) {
    Part* p = PartPtr();
    BaseEvent* eventRepairExternal = Events()["EventRepairExternal"];
    const bool isDamaged = Get<bool>("isDamaged");
    if (damaged && !isDamaged) {
        if (baseSetup) wheelBase->InopSystems()->AddSubsystem(&subsystemDamage_);
        eventRepairExternal->active = damaged && Get<bool>("isRepairable");
        eventRepairExternal->guiActive = damaged && Get<bool>("isRepairable");
        GameEvents::onPartRepaired.Fire(p);
        WheelModuleEnvironment::Trace("onPartRepaired");
    }
    if (!damaged && isDamaged) {
        repairImmunityTimeTotal_ = UnityRandom::Global().Range(30.0f, 90.0f);
        repairImmunityTimeLeft_ = repairImmunityTimeTotal_;
        if (baseSetup) wheelBase->InopSystems()->RemoveSubsystem(&subsystemDamage_);
        eventRepairExternal->active = damaged && Get<bool>("isRepairable");
        eventRepairExternal->guiActive = damaged && Get<bool>("isRepairable");
        GameEvents::onWheelRepaired.Fire(p);
        WheelModuleEnvironment::Trace("onWheelRepaired");
    }
    Set<bool>("isDamaged", damaged);
    stressVariability = f32store(f32store(0.0 - fx(UnityRandom::Global().value())) * fx(Get<float>("stressTolerance")) * fx(0.02f));
}

// ModuleWheelDamage.cs:452 (the EVA skill / repair-kit inventory checks are the host's; the repair itself is SetDamaged(false))
void ModuleWheelDamage::EventRepairExternal() { SetDamaged(false); }

// ModuleWheelDamage.cs:529
void ModuleWheelDamage::HandleCollisionVelocity(const Collider*, const Vector3f& velocity, const Vector3f& normal) {
    if (!IsImpactDamageEnable()) return;
    const Vector3f vector = Vector3f::Project(velocity, normal);
    if (vector.magnitude() < Get<float>("impactDamageVelocity")) return;
    SetDamaged(true);
}

// ---- ModuleWheelDeployment -----------------------------------------------------------------------------------------------------------------
ModuleWheelDeployment::~ModuleWheelDeployment() {}

// ModuleWheelDeployment.cs:139
float ModuleWheelDeployment::Position() const {
    if (Get<float>("deployedPosition") != 0.0f) return position;
    return f32store(1.0 - fx(position));
}

// ModuleWheelDeployment.cs:153
void ModuleWheelDeployment::OnLoad(const ConfigNode& node) {
    if (node.HasValue("position")) position = std::strtof(node.GetValue("position")->c_str(), nullptr);
    if (node.HasValue("slaveModules")) {
        slaveModuleIndices = KSPUtil::ParseArray<int>(*node.GetValue("slaveModules"), [](const std::string& s) { return std::stoi(s); });
        slaveModuleIndicesLoaded = true;
    }
}

// the public (serialized) non-KSPField members OnLoad fills on the prefab: Unity's Instantiate copies them
void ModuleWheelDeployment::CopySerializedFrom(const PartModule& from) {
    PartModule::CopySerializedFrom(from);
    if (const auto* m = dynamic_cast<const ModuleWheelDeployment*>(&from)) {
        position = m->position;
        slaveModuleIndices = m->slaveModuleIndices;
        slaveModuleIndicesLoaded = m->slaveModuleIndicesLoaded;
    }
}

// ModuleWheelDeployment.cs:166
void ModuleWheelDeployment::OnSave(ConfigNode& node) {
    if (animationState_ != nullptr) {
        position = Mathf::Clamp01(animationState_->normalizedTime());
        node.AddValue("position", position);
    }
}

// ModuleWheelDeployment.cs:175
void ModuleWheelDeployment::OnStart(StartState) {
    Part* p = PartPtr();
    Events().Bind("EventToggle", BaseEvent::Handler([this] { EventToggle(); }));
    Actions().Bind("ActionToggle", [this](KSPActionParam& k) { ActionToggle(k); });
    Fields()["stateDisplayString"]->guiActive = ResourceFlowEnvironment::LoadedSceneIsFlight();
    Events()["EventToggle"]->SetGuiName(GetToggleEventName());
    const std::string animTrf = Get<std::string>("animationTrfName"), animState = Get<std::string>("animationStateName");
    if (animTrf.empty()) {
        log::Error("[ModuleWheelDeployment]: Animation transform name is undefined");
        return;
    }
    if (animState.empty()) {
        log::Error("[ModuleWheelDeployment]: Animation state name is undefined");
        return;
    }
    anim_ = WheelModuleEnvironment::findModelAnimator ? WheelModuleEnvironment::findModelAnimator(*p, animTrf, animState) : nullptr;
    if (anim_ == nullptr) {
        log::Error("[ModuleWheelDeployment]: No animation found with clip" + animState + " on transform " + animTrf + " at " + p->gameObjectName + ".");
        return;
    }
    const std::string tgtName = Get<std::string>("deployTargetTransformName");
    if (!tgtName.empty()) {
        deployTgt_ = WheelModuleEnvironment::findModelTransform ? WheelModuleEnvironment::findModelTransform(*p, tgtName) : nullptr;
        if (deployTgt_ == nullptr) {
            log::Error("[ModuleWheelDeployment]: No transform found with id " + tgtName + " on " + p->gameObjectName + ".");
            return;
        }
    }
    const std::string retName = Get<std::string>("retractTransformName");
    if (!retName.empty()) {
        retractTransform_ = WheelModuleEnvironment::findModelTransform ? WheelModuleEnvironment::findModelTransform(*p, retName) : nullptr;
        if (retractTransform_ == nullptr) {
            log::Error("[ModuleWheelDeployment]: No transform found with id " + retName + " on " + p->gameObjectName + ".");
            return;
        }
    }
    if (slaveModuleIndicesLoaded) {
        slaveModules_.clear();
        int count = static_cast<int>(p->Modules().Count());
        while (count-- > 0) {
            IScalarModule* item = p->Modules()[static_cast<size_t>(count)]->AsScalarModule();
            if (item == nullptr) continue;
            int num = static_cast<int>(slaveModuleIndices.size());
            while (num-- > 0) {
                if (slaveModuleIndices[static_cast<size_t>(num)] == count) {
                    slaveModules_.push_back(item);
                    break;
                }
            }
        }
    }
    deploymentSubsystems_ = WheelSubsystem("Wheel Retracted", WheelSubsystem::All, this);
    anim_->Stop();
    animationState_ = (*anim_)[animState];
    animLength = ResourceFlowEnvironment::LoadedSceneIsFlight() ? animationState_->length : 0.0f;
    animationState_->wrapMode = WrapMode::ClampForever;
    animationState_->enabled = true;
    animationState_->weight = 1.0f;
    animationState_->speed = 0.0f;
    position = Mathf::Clamp01(position);
    retractedPosition = f32store(1.0 - fx(Get<float>("deployedPosition")));
    standInCollider_ = false;
    if (Get<std::string>("stateString") != std::string()) {
        animationState_->set_normalizedTime(position);
        FSMSetup(Get<std::string>("stateString"));
    } else {
        animationState_->set_normalizedTime(Get<float>("deployedPosition"));
        FSMSetup("Deployed");
    }
    if (ResourceFlowEnvironment::LoadedSceneIsFlight() && p->vessel->situation == Vessel::Situations::PRELAUNCH && position == Get<float>("deployedPosition"))
        p->vessel->ActionGroups().Set(KSPActionGroup::Gear, true);
}

// ModuleWheelDeployment.cs:278 (the animation state is re-posed from stateString; the protoPartSnapshot write-back is the host's)
void ModuleWheelDeployment::OnInventoryModeDisable() {
    Part* p = PartPtr();
    if (anim_ == nullptr && WheelModuleEnvironment::findModelAnimator)
        anim_ = WheelModuleEnvironment::findModelAnimator(*p, Get<std::string>("animationTrfName"), Get<std::string>("animationStateName"));
    if (anim_ == nullptr) return;
    animationState_ = (*anim_)[Get<std::string>("animationStateName")];
    if (animationState_ == nullptr) return;
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) {
        float normalizedTime = 0.0f;
        const std::string text = Get<std::string>("stateString");
        if (!(text == "Retracted")) {
            if (text == "Deployed") normalizedTime = Get<float>("deployedPosition");
        } else {
            normalizedTime = f32store(1.0 - fx(Get<float>("deployedPosition")));
        }
        animationState_->enabled = true;
        animationState_->weight = 1.0f;
        animationState_->speed = 0.0f;
        animationState_->set_normalizedTime(normalizedTime);
    }
}

// ModuleWheelDeployment.cs:328
void ModuleWheelDeployment::OnStoredInInventory(ModuleInventoryPart&) { Set<std::string>("stateString", "Retracted"); }

// ModuleWheelDeployment.cs:342
void ModuleWheelDeployment::OnWheelSetup() {
    SetInopSubsystems(Position() < Get<float>("TsubSys"));
    if (deployTgt_ != nullptr) {
        wheel->SetWheelTransform(deployTgt_->position, deployTgt_->rotation);
        WheelModuleEnvironment::Trace("setWheelTransform");
    }
}

// ModuleWheelDeployment.cs:355
void ModuleWheelDeployment::SetInopSubsystems(bool disable) {
    if (disable && !subsystemsDisabled) {
        int count = static_cast<int>(slaveModules_.size());
        while (count-- > 0) {
            slaveModules_[static_cast<size_t>(count)]->SetScalar(0.0f);
            slaveModules_[static_cast<size_t>(count)]->SetUIWrite(false);
        }
        WheelModuleEnvironment::Trace("slaves:0");
        wheelBase->InopSystems()->AddSubsystem(&deploymentSubsystems_);
        if (ResourceFlowEnvironment::LoadedSceneIsFlight() && Get<bool>("useStandInCollider") && !standInCollider_) {
            standInCollider_ = true;
            WheelModuleEnvironment::Trace("standIn:create");
        }
    }
    if (!disable && subsystemsDisabled) {
        int count2 = static_cast<int>(slaveModules_.size());
        while (count2-- > 0) slaveModules_[static_cast<size_t>(count2)]->SetUIWrite(true);
        WheelModuleEnvironment::Trace("slaves:ui");
        wheelBase->InopSystems()->RemoveSubsystem(&deploymentSubsystems_);
        if (standInCollider_) {
            standInCollider_ = false;
            WheelModuleEnvironment::Trace("standIn:destroy");
        }
    }
    subsystemsDisabled = disable;
}

void ModuleWheelDeployment::SetWheelTransformFromTarget() {
    if (deployTgt_ != nullptr && wheel != nullptr) {
        wheel->SetWheelTransform(deployTgt_->position, deployTgt_->rotation);
        WheelModuleEnvironment::Trace("setWheelTransform");
    }
}

// ModuleWheelDeployment.cs:405
void ModuleWheelDeployment::FSMSetup(const std::string& startStateName) {
    using SC = KFSMStateChange;
    using CB = KFSMCallback;
    using CC = KFSMEventCondition;
    Part* p = PartPtr();
    auto effect = [this](const char* field, bool hasValue, float value) {
        const std::string name = Get<std::string>(field);
        if (!name.empty()) WheelModuleEnvironment::Trace("fx:" + name + (hasValue ? ":" + std::to_string(value) : std::string()));
    };
    auto cubes = [this, p](float retracted, float deployed) {
        WheelModuleEnvironment::Trace("cubes:Retracted=" + std::to_string(retracted) + ",Deployed=" + std::to_string(deployed));
        p->GetDragCubes().SetCubeWeight("Retracted", retracted);
        p->GetDragCubes().SetCubeWeight("Deployed", deployed);
    };
    auto stateStore = [this](const char* n) {
        stateStore_.push_back(std::make_unique<KFSMState>(n));
        return stateStore_.back().get();
    };
    auto eventStore = [this](const char* n) {
        eventStore_.push_back(std::make_unique<KFSMEvent>(n));
        return eventStore_.back().get();
    };
    st_deployed = stateStore("Deployed");
    st_deployed->OnEnter = SC::Lambda(this, [this, cubes](KFSMState*) {
        position = Get<float>("deployedPosition");
        animationState_->speed = 0.0f;
        animationState_->set_normalizedTime(Get<float>("deployedPosition"));
        SetWheelTransformFromTarget();
        cubes(0.0f, 1.0f);
    });
    fsm.AddState(st_deployed);
    st_retracting = stateStore("Retracting...");
    st_retracting->OnEnter = SC::Lambda(this, [this](KFSMState*) { retractStarted_ = true; });
    st_retracting->OnFixedUpdate = CB::Lambda(this, [this, cubes] {
        if (retractStarted_) {
            retractStarted_ = false;
            if (Get<float>("deployedPosition") == 0.0f) {
                animationState_->speed = Get<float>("retractDurationFactor");
            } else {
                animationState_->speed = 0.0f - Get<float>("retractDurationFactor");
            }
            if (retractTransform_ != nullptr) animationState_->set_normalizedTime(FindTransformAnimationTime(retractTransform_));
        }
        position = Mathf::Clamp01(animationState_->normalizedTime());
        SetWheelTransformFromTarget();
        cubes(f32store(1.0 - fx(Position())), Position());
        if (baseSetup) SetInopSubsystems(Position() < Get<float>("TsubSys"));
    });
    fsm.AddState(st_retracting);
    st_retracted = stateStore("Retracted");
    st_retracted->OnEnter = SC::Lambda(this, [this, cubes](KFSMState*) {
        position = retractedPosition;
        animationState_->speed = 0.0f;
        animationState_->set_normalizedTime(retractedPosition);
        SetWheelTransformFromTarget();
        IWheelStatusLight* lightModule = WheelModuleEnvironment::findLightModule ? WheelModuleEnvironment::findLightModule(*PartPtr()) : nullptr;
        if (lightModule != nullptr && lightModule->IsOn()) WheelModuleEnvironment::Trace("flare:black");
        cubes(1.0f, 0.0f);
    });
    fsm.AddState(st_retracted);
    st_deploying = stateStore("Deploying...");
    st_deploying->OnEnter = SC::Lambda(this, [this](KFSMState*) {
        if (Get<float>("deployedPosition") == 0.0f) {
            animationState_->speed = 0.0f - Get<float>("extendDurationFactor");
        } else {
            animationState_->speed = Get<float>("extendDurationFactor");
        }
        IWheelStatusLight* lightModule = WheelModuleEnvironment::findLightModule ? WheelModuleEnvironment::findLightModule(*PartPtr()) : nullptr;
        if (lightModule != nullptr && lightModule->IsOn()) WheelModuleEnvironment::Trace("flare:white");
    });
    st_deploying->OnUpdate = CB::Lambda(this, [this, cubes] {
        position = Mathf::Clamp01(animationState_->normalizedTime());
        SetWheelTransformFromTarget();
        cubes(f32store(1.0 - fx(Position())), Position());
        if (baseSetup) SetInopSubsystems(Position() < Get<float>("TsubSys"));
    });
    fsm.AddState(st_deploying);
    st_inoperable = stateStore("Inoperable");
    st_inoperable->OnEnter = SC::Lambda(this, [this](KFSMState*) { animationState_->speed = 0.0f; });
    fsm.AddState(st_inoperable);

    on_deploy = eventStore("on_deploy");
    on_deploy->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_deploy->GoToStateOnEvent = st_deploying;
    on_deploy->OnEvent = CB::Lambda(this, [this, effect] {
        effect("fxDeploy", false, 0.0f);
        effect("fxRetract", true, 0.0f);
    });
    fsm.AddEvent(on_deploy, {st_retracted, st_retracting});
    on_deployed = eventStore("on_deployed");
    on_deployed->updateMode = KFSMUpdateMode::LATEUPDATE;
    on_deployed->GoToStateOnEvent = st_deployed;
    on_deployed->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) {
        return ResourceFlowEnvironment::LoadedSceneIsEditor() ||
               Mathf::Abs(f32store(fx(Mathf::Clamp01(animationState_->normalizedTime())) - fx(Get<float>("deployedPosition")))) < 0.01f;
    });
    on_deployed->OnEvent = CB::Lambda(this, [this, effect] {
        if (baseSetup) SetInopSubsystems(false);
        effect("fxDeployed", false, 0.0f);
        effect("fxRetract", true, 0.0f);
        effect("fxDeploy", true, 0.0f);
    });
    fsm.AddEvent(on_deployed, {st_deploying});
    on_retract = eventStore("on_retract");
    on_retract->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_retract->GoToStateOnEvent = st_retracting;
    on_retract->OnEvent = CB::Lambda(this, [this, effect] {
        effect("fxRetract", false, 0.0f);
        effect("fxDeploy", true, 0.0f);
    });
    fsm.AddEvent(on_retract, {st_deployed, st_deploying});
    on_retracted = eventStore("on_retracted");
    on_retracted->updateMode = KFSMUpdateMode::LATEUPDATE;
    on_retracted->GoToStateOnEvent = st_retracted;
    on_retracted->OnCheckCondition = CC::Lambda(this, [this](KFSMState*) {
        return ResourceFlowEnvironment::LoadedSceneIsEditor() || Mathf::Abs(f32store(fx(Mathf::Clamp01(animationState_->normalizedTime())) - fx(retractedPosition))) < 0.01f;
    });
    on_retracted->OnEvent = CB::Lambda(this, [this, effect] {
        if (baseSetup) SetInopSubsystems(true);
        effect("fxRetracted", false, 0.0f);
        effect("fxRetract", true, 0.0f);
        effect("fxDeploy", true, 0.0f);
    });
    fsm.AddEvent(on_retracted, {st_retracting});
    on_inoperable = eventStore("on_inoperable");
    on_inoperable->updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_inoperable->GoToStateOnEvent = st_inoperable;
    on_inoperable->OnCheckCondition = CC::Lambda(this, [](KFSMState*) { return false; });
    on_inoperable->OnEvent = CB::Lambda(this, [this, effect] {
        effect("fxRetract", true, 0.0f);
        effect("fxDeploy", true, 0.0f);
    });
    fsm.AddEventExcluding(on_inoperable, {st_inoperable});
    fsm.OnStateChange = Callback<KFSMState*, KFSMState*, KFSMEvent*>::Bind(this, &ModuleWheelDeployment::OnFSMStateChange);
    fsm.StartFSM(startStateName);
    OnFSMStateChange(st_retracted, st_deployed, on_deploy);
}

// ModuleWheelDeployment.cs:662
void ModuleWheelDeployment::OnFSMStateChange(KFSMState*, KFSMState*, KFSMEvent*) {
    Set<std::string>("stateString", fsm.currentStateName);
    const std::string& s = Get<std::string>("stateString");
    if (s == "Inoperable") Set<std::string>("stateDisplayString", Localizer::Format("#autoLOC_7003226"));
    else if (s == "Retracted") Set<std::string>("stateDisplayString", Localizer::Format("#autoLOC_234861"));
    else if (s == "Deploying...") Set<std::string>("stateDisplayString", Localizer::Format("#autoLOC_6002270"));
    else if (s == "Retracting...") Set<std::string>("stateDisplayString", Localizer::Format("#autoLOC_234856"));
    else if (s == "Deployed") Set<std::string>("stateDisplayString", Localizer::Format("#autoLOC_6002269"));
}

// ModuleWheelDeployment.cs:685
void ModuleWheelDeployment::FixedUpdate() {
    if (fsm.Started()) fsm.FixedUpdateFSM();
}
// ModuleWheelDeployment.cs:693
void ModuleWheelDeployment::Update() {
    if (fsm.Started()) fsm.UpdateFSM();
}
// ModuleWheelDeployment.cs:701
void ModuleWheelDeployment::LateUpdate() {
    if (fsm.Started()) fsm.LateUpdateFSM();
}

// ModuleWheelDeployment.cs:710
void ModuleWheelDeployment::EventToggle() { ToggleDeployment(true); }

// ModuleWheelDeployment.cs:715
void ModuleWheelDeployment::ToggleDeployment(bool affectSymCParts) {
    Part* p = PartPtr();
    if (fsm.CurrentState() != st_deployed && fsm.CurrentState() != st_deploying) {
        if (fsm.CurrentState() == st_retracted || fsm.CurrentState() == st_retracting) {
            const bool shielded = WheelModuleEnvironment::shieldedFromAirstream ? WheelModuleEnvironment::shieldedFromAirstream(*p) : false;
            if (shielded && !Get<bool>("shieldedCanDeploy") && ResourceFlowEnvironment::LoadedSceneIsFlight()) {
                WheelModuleEnvironment::Trace("shieldMsg");
                return;
            }
            fsm.RunEvent(on_deploy);
        }
    } else {
        fsm.RunEvent(on_retract);
    }
    if (affectSymCParts) {
        for (size_t i = 0, count = p->symmetryCounterparts.size(); i < count; i++)
            dynamic_cast<ModuleWheelDeployment*>(p->symmetryCounterparts[i]->Modules()[static_cast<size_t>(p->Modules().IndexOf(this))])->ToggleDeployment(false);
    }
}

// ModuleWheelDeployment.cs:743
std::string ModuleWheelDeployment::GetToggleEventName() {
    if (ResourceFlowEnvironment::LoadedSceneIsEditor()) {
        if (position == Get<float>("deployedPosition")) return Localizer::Format("#autoLOC_247601");
        return Localizer::Format("#autoLOC_6002271");
    }
    if (Get<std::string>("stateString") != "Inoperable") {
        if (position != Get<float>("deployedPosition")) return Localizer::Format("#autoLOC_7003225");
        return Localizer::Format("#autoLOC_7003224");
    }
    return Localizer::Format("#autoLOC_7003226");
}

// ModuleWheelDeployment.cs:765
void ModuleWheelDeployment::ActionToggle(KSPActionParam& kAct) {
    KSPActionType kSPActionType = kAct.Type();
    if (kSPActionType == KSPActionType::Toggle)
        kSPActionType = (fsm.CurrentState() != st_retracted && fsm.CurrentState() != st_retracting) ? KSPActionType::Deactivate : KSPActionType::Activate;
    ActionToggle(kSPActionType);
}

// ModuleWheelDeployment.cs:775
void ModuleWheelDeployment::ActionToggle(KSPActionType action) {
    Part* p = PartPtr();
    switch (action) {
    case KSPActionType::Deactivate:
        if (fsm.CurrentState() == st_deployed || fsm.CurrentState() == st_deploying) fsm.RunEvent(on_retract);
        break;
    case KSPActionType::Activate:
        if (fsm.CurrentState() == st_retracted || fsm.CurrentState() == st_retracting) {
            const bool shielded = WheelModuleEnvironment::shieldedFromAirstream ? WheelModuleEnvironment::shieldedFromAirstream(*p) : false;
            if (shielded && !Get<bool>("shieldedCanDeploy") && ResourceFlowEnvironment::LoadedSceneIsFlight()) {
                WheelModuleEnvironment::Trace("shieldMsg");
            } else {
                fsm.RunEvent(on_deploy);
            }
        }
        break;
    default: break;
    }
}

// ModuleWheelDeployment.cs:841
float ModuleWheelDeployment::FindTransformAnimationTime(NativeTransform* t) {
    const float normalizedTime = animationState_->normalizedTime();
    const Vector3f b = t->position;
    float num = std::numeric_limits<float>::max();
    float result = std::numeric_limits<float>::max();
    auto sample = [this] {
        if (WheelModuleEnvironment::sampleAnimation) WheelModuleEnvironment::sampleAnimation(*anim_);
    };
    if (Get<float>("deployedPosition") == 0.0f) {
        for (float num2 = 0.0f; num2 < 1.0f; num2 = f32store(fx(num2) + fx(0.025f))) {
            animationState_->set_normalizedTime(num2);
            sample();
            const float num3 = Vector3f::Distance(t->position, b);
            if (num3 < num) {
                num = num3;
                result = num2;
            }
        }
    } else {
        for (float num4 = 1.0f; num4 > 0.0f; num4 = f32store(fx(num4) - fx(0.025f))) {
            animationState_->set_normalizedTime(num4);
            sample();
            const float num5 = Vector3f::Distance(t->position, b);
            if (num5 < num) {
                num = num5;
                result = num4;
            }
        }
    }
    animationState_->set_normalizedTime(normalizedTime);
    sample();
    return result;
}

// ---- ModuleWheelBogey ----------------------------------------------------------------------------------------------------------------------
// ModuleWheelBogey.cs:64
void ModuleWheelBogey::OnStart(StartState) {
    Part* p = PartPtr();
    auto find = [&](const std::string& name) -> NativeTransform* {
        return WheelModuleEnvironment::findModelTransform ? WheelModuleEnvironment::findModelTransform(*p, name) : nullptr;
    };
    const std::string bogeyName = Get<std::string>("bogeyTransformName");
    if (!bogeyName.empty()) {
        NativeTransform* t = find(bogeyName);
        if (t != nullptr) bogey = t;
        else log::Error("[ModuleWheelBogey]: No bogey transform found called " + bogeyName + " in " + p->gameObjectName + "!");
    }
    const std::string refName = Get<std::string>("bogeyRefTransformName");
    if (!refName.empty()) {
        NativeTransform* t = find(refName);
        if (t != nullptr) hasBogeyRef = true;
        else log::Error("[ModuleWheelBogey]: No bogey refe transform found called " + refName + " in " + p->gameObjectName + "!");
    } else {
        hasBogeyRef = bogey != nullptr;   // bogeyRef = bogey
    }
    const std::string wheelRefName = Get<std::string>("wheelTransformRefName");
    NativeTransform* wheelTransformRef = nullptr;
    if (!wheelRefName.empty()) {
        wheelTransformRef = find(wheelRefName);
        if (wheelTransformRef == nullptr) log::Error("[ModuleWheelBogey]: No wheel ref transform found called " + wheelRefName + " in " + p->gameObjectName + "!");
    }
    if (!Get<std::string>("wheelTransformBaseName").empty() && wheelTransformRef == nullptr)
        log::Error("[ModuleWheelBogey]: No reference transform defined for " + p->gameObjectName + "! Wheels won't function properly!");
    const int idx = Get<int>("deployModuleIndex");
    if (idx != -1) {
        auto* m = dynamic_cast<ModuleWheelDeployment*>(p->Modules()[static_cast<size_t>(idx)]);
        if (m != nullptr) {
            deployModule = m;
            return;
        }
        log::Error("[ModuleWheelBogey]: No deployment module found at index " + std::to_string(idx) + " in " + p->gameObjectName + "!");
    }
}

// ModuleWheelBogey.cs:134
void ModuleWheelBogey::OnWheelSetup() {
    if (hasBogeyRef) {
        bogeyRot0_ = bogeyRefLocalRotation;
        bogeyUp0_ = bogeyRefLocalRotation * Get<Vector3f>("bogeyUpAxis");
    } else {
        bogeyRot0_ = Quaternionf::identity();
        bogeyUp0_ = Get<Vector3f>("bogeyUpAxis");
    }
    bogeyRotLast_ = bogeyRot0_;
}

// ModuleWheelBogey.cs:149 (the managed math up to bogeyRot; the RotateTowards / Lerp onto bogey.localRotation are native Unity code and belong to
// the engine transform: the host writes bogeyLocalRotation from the target `bogeyRot`)
void ModuleWheelBogey::LateUpdate() {
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight()) return;
    Part* p = PartPtr();
    if (bogey != nullptr && !p->fields.packed && baseSetup && (deployModule == nullptr || deployModule->Position() > 0.8f) &&
        !wheelBase->InopSystems()->HasType(WheelSubsystem::Bogey)) {
        const Vector3f bogeyAxis = Get<Vector3f>("bogeyAxis");
        if (wheel->IsGrounded()) {
            bogeyUp = Vector3f::ProjectOnPlane(wheel->currentState()->hit.normal, bogeyParentRotation * bogeyAxis);
            bogeyAngle = Mathf::Clamp(
                KSPUtil::BearingDegrees(bogeyUp0_, UnityNative::Inverse(bogeyParentRotation) * bogeyUp, bogeyAxis), Get<float>("minPitch"), Get<float>("maxPitch"));
        } else {
            bogeyAngle = Get<float>("restPitch");
        }
        bogeyRot = NativeOps::AngleAxis(bogeyAngle, bogeyAxis) * bogeyRot0_;
    }
}

}  // namespace ModuleWheels

// ---- registration --------------------------------------------------------------------------------------------------------------------------
namespace detail {
void RegisterWheelModules() {
    ModuleFactory::Register("ModuleWheelBase", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleWheelBase>(c)); });
    ModuleFactory::Register("ModuleWheelMotor", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleWheels::ModuleWheelMotor>(c)); });
    ModuleFactory::Register("ModuleWheelMotorSteering",
                            [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleWheels::ModuleWheelMotorSteering>(c)); });
    ModuleFactory::Register("ModuleWheelBrakes", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleWheels::ModuleWheelBrakes>(c)); });
    ModuleFactory::Register("ModuleWheelLock", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleWheels::ModuleWheelLock>(c)); });
    ModuleFactory::Register("ModuleWheelSuspension",
                            [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleWheels::ModuleWheelSuspension>(c)); });
    ModuleFactory::Register("ModuleWheelSteering",
                            [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleWheels::ModuleWheelSteering>(c)); });
    ModuleFactory::Register("ModuleWheelDamage", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleWheels::ModuleWheelDamage>(c)); });
    ModuleFactory::Register("ModuleWheelDeployment",
                            [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleWheels::ModuleWheelDeployment>(c)); });
    ModuleFactory::Register("ModuleWheelBogey", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleWheels::ModuleWheelBogey>(c)); });
}
}  // namespace detail

}  // namespace ksp
