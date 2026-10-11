// WP-50: FlightInputHandler.cs and AxisGroupsInputHandler.cs.
#include "ksp/input/flight_input_handler.hpp"

#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/input/space_navigator.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/axis_groups_module.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {

bool Unlocked(ControlTypes t) { return InputLockManager::IsUnlocked(t); }

// the `SpaceNavigator.Instance != null && !(SpaceNavigator.Instance is SpaceNavigatorNoDevice)` test
bool SpaceNavHasDevice() {
    SpaceNavigator* inst = SpaceNavigator::Instance();
    return inst != nullptr && dynamic_cast<SpaceNavigatorNoDevice*>(inst) == nullptr;
}

// Mathf.Clamp(v - 0.1f * Time.deltaTime, -1f, 1f) style trim step
float TrimStep(float trim, float sign) {
    return Mathf::Clamp(f32store(fx(trim) + sign * fx(0.1f) * fx(Time::deltaTime)), -1.0f, 1.0f);
}

// precisionX = minus.GetKey() ? Mathf.Max(-1f, precisionX - sens * dt) : (plus.GetKey() ? Mathf.Min(1f, precisionX + sens * dt) : v)
float PrecisionStep(KeyBinding& minusKey, KeyBinding& plusKey, float precisionValue, float fallback) {
    const double step = fx(GameSettings::INPUT_KEYBOARD_SENSIVITITY) * fx(Time::deltaTime);
    if (minusKey.GetKey()) return Mathf::Max(-1.0f, f32store(fx(precisionValue) - step));
    if (plusKey.GetKey()) return Mathf::Min(1.0f, f32store(fx(precisionValue) + step));
    return fallback;
}

// non-precision keys: minus ? -1f : (plus ? 1f : value)
float KeyOrValue(KeyBinding& minusKey, KeyBinding& plusKey, float value) {
    return minusKey.GetKey() ? -1.0f : (plusKey.GetKey() ? 1.0f : value);
}

}  // namespace

// FlightInputHandler.cs:83
void FlightInputHandler::Awake() {
    fetch = this;
    state = FlightCtrlState();
}

// FlightInputHandler.cs:89
void FlightInputHandler::Start() {
    rcslock = true;
    hasSpaceNavDevice = SpaceNavHasDevice();
    GE::OnVesselOverrideGroupChanged.Add(this, &FlightInputHandler::OnVesselOverrideGroupChanged);
}

// FlightInputHandler.cs:96
void FlightInputHandler::OnVesselOverrideGroupChanged(Vessel* v) {
    if (v == FlightRegistry::ActiveVessel) ResumeVesselCtrlState(v);
}

// FlightInputHandler.cs:104
void FlightInputHandler::SetNeutralControls() {
    state = FlightCtrlState();
    FlightRegistry::ActiveVessel->ctrlState.NeutralizeAll();
    if (fetch != nullptr) {
        fetch->throttle = -1.0f;
        fetch->rcslock = true;
    }
}

// FlightInputHandler.cs:115
void FlightInputHandler::ResumeVesselCtrlState(Vessel* v) {
    v->GetControlState(state);
    FlightRegistry::ActiveVessel->ActionGroups().CopyFrom(v->ActionGroups());
    if (fetch != nullptr) {
        fetch->throttle = f32store(fx(state.mainThrottle) * 2.0 - 1.0);
        fetch->refocusThrottle = true;
    }
}

// FlightInputHandler.cs:126
void FlightInputHandler::SetLaunchCtrlState() {
    SetNeutralControls();
    state.mainThrottle = GameSettings::PRELAUNCH_DEFAULT_THROTTLE;
    if (fetch != nullptr) fetch->refocusThrottle = true;
}

// FlightInputHandler.cs:136
void FlightInputHandler::OnApplicationFocus(bool focus) {
    hasFocus = focus;
    if (focus) refocusThrottle = true;
}

// FlightInputHandler.cs:145
void FlightInputHandler::Update() {
    if (!FlightRegistry::ready || FlightDriver::Pause()) return;
    Vessel* activeVessel = FlightRegistry::ActiveVessel;
    using namespace GameSettings;
    const IInputDevice& input = InputEnvironment::Input();
    if (!activeVessel->isEVA()) {
        if (Unlocked(ControlTypes::MISC)) {
            if (MODIFIER_KEY->GetKey() && input.GetKeyDown(KeyCode::L)) {
                stageLock = !stageLock;
                if (stageLock) InputLockManager::SetControlLock(ControlTypes::STAGING, "manualStageLock");
                else InputLockManager::RemoveControlLock("manualStageLock");
                // MonoBehaviour.print("staging lock/unlock")
            }
            if (PRECISION_CTRL->GetKeyDown()) {
                precisionMode = !precisionMode;
                GE::Input::OnPrecisionModeToggle.Fire(precisionMode);
            }
        }
        if (Unlocked(ControlTypes::RCS) && RCS_TOGGLE->GetKeyDown()) {
            activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::RCS);
            if (!activeVessel->ActionControlBlocked(KSPActionGroup::RCS)) rcslock = !rcslock;  // + print("RCS lock/unlock")
        }
        if (Unlocked(ControlTypes::GROUP_GEARS)) {
            if (LANDING_GEAR->GetKeyDown()) {
                activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Gear);
                state.gearDown = true;
                state.gearUp = true;
            } else {
                state.gearDown = false;
                state.gearUp = false;
            }
        }
        if (Unlocked(ControlTypes::GROUP_LIGHTS)) {
            if (HEADLIGHT_TOGGLE->GetKeyDown()) {
                activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Light);
                state.headlight = true;
            } else {
                state.headlight = false;
            }
        }
        if (Unlocked(ControlTypes::THROTTLE_CUT_MAX) && THROTTLE_CUTOFF->GetKeyDown() && !MODIFIER_KEY->GetKey()) state.mainThrottle = 0.0f;
        if (Unlocked(ControlTypes::THROTTLE_CUT_MAX) && THROTTLE_FULL->GetKeyDown() && !MODIFIER_KEY->GetKey()) state.mainThrottle = 1.0f;
        if (Unlocked(ControlTypes::STAGING) && LAUNCH_STAGES->GetKeyDown()) {
            if (!activeVessel->ActionControlBlocked(KSPActionGroup::Stage)) {
                if (host.activateNextStage) host.activateNextStage();
                else StageManager::ActivateNextStage();
            }
            activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Stage);
        }
        if (Docking_toggleRotLin->GetDoubleTapDown()) {
            linRotSwitchHold = !linRotSwitchHold;
            if (host.postScreenMessage) {
                host.postScreenMessage(Localizer::Format("#autoLOC_7003261", {linRotSwitchHold ? Localizer::Format("#autoLOC_7003262")
                                                                                              : Localizer::Format("#autoLOC_7003263")}),
                                       2.0f);
            }
        } else if (linRotSwitchHold) {
            if (Docking_toggleRotLin->GetKeyDown()) InputBinding::linRotState = true;
            if (Docking_toggleRotLin->GetKeyUp()) InputBinding::linRotState = false;
        } else if (Docking_toggleRotLin->GetKeyDown()) {
            InputBinding::linRotState = !InputBinding::linRotState;
        }
        if (Unlocked(ControlTypes::SAS)) {
            if (SAS_HOLD->GetKeyDown()) {
                if (!activeVessel->ActionControlBlocked(KSPActionGroup::SAS)) state.killRot = !state.killRot;
                activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::SAS);
            }
            if (SAS_HOLD->GetKeyUp()) {
                if (!activeVessel->ActionControlBlocked(KSPActionGroup::SAS)) state.killRot = !state.killRot;
                activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::SAS);
            }
            if (SAS_TOGGLE->GetKeyDown()) {
                if (!activeVessel->ActionControlBlocked(KSPActionGroup::SAS)) state.killRot = !state.killRot;
                activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::SAS);
            }
        }
        if (Unlocked(ControlTypes::CUSTOM_ACTION_GROUPS)) {
            if (host.actionGroupsAppPresent) {
                if (AGROUP_SELECT_NEXT->GetKeyDown() && host.actionGroupsSelectNext) host.actionGroupsSelectNext();
                if (AGROUP_SELECT_PREV->GetKeyDown() && host.actionGroupsSelectPrev) host.actionGroupsSelectPrev();
            }
            if (CustomActionGroup1->GetKeyDown()) activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Custom01);
            if (CustomActionGroup2->GetKeyDown()) activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Custom02);
            if (CustomActionGroup3->GetKeyDown()) activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Custom03);
            if (CustomActionGroup4->GetKeyDown()) activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Custom04);
            if (CustomActionGroup5->GetKeyDown()) activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Custom05);
            if (CustomActionGroup6->GetKeyDown()) activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Custom06);
            if (CustomActionGroup7->GetKeyDown()) activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Custom07);
            if (CustomActionGroup8->GetKeyDown()) activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Custom08);
            if (CustomActionGroup9->GetKeyDown()) activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Custom09);
            if (CustomActionGroup10->GetKeyDown()) activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Custom10);
        }
        if (Unlocked(ControlTypes::GROUP_ABORT) && AbortActionGroup->GetKeyDown()) activeVessel->ActionGroups().ToggleGroup(KSPActionGroup::Abort);
        if (Unlocked(ControlTypes::GROUP_BRAKES)) {
            if (BRAKES->GetKeyDown()) activeVessel->ActionGroups().SetGroup(KSPActionGroup::Brakes, true);
            if (BRAKES->GetKeyUp()) activeVessel->ActionGroups().SetGroup(KSPActionGroup::Brakes, false);
        }
    } else if (Unlocked(ControlTypes::UI_DIALOGS) && SAS_TOGGLE->GetKeyDown()) {
        EVA_ROTATE_ON_MOVE = !EVA_ROTATE_ON_MOVE;
    }
    if (hasSpaceNavDevice && TOGGLE_SPACENAV_FLIGHT_CONTROL->GetKeyUp()) {
        if (SPACENAV_USE_AS_FLIGHT_CONTROL) {
            SPACENAV_USE_AS_FLIGHT_CONTROL = false;
            if (host.postScreenMessage) host.postScreenMessage(Localizer::Format("#autoLOC_175414"), 3.0f);
        } else {
            SPACENAV_USE_AS_FLIGHT_CONTROL = true;
            if (host.postScreenMessage) host.postScreenMessage(Localizer::Format("#autoLOC_175419"), 3.0f);
        }
    }
}

// FlightInputHandler.cs:362
void FlightInputHandler::ProcessAxis(AxisBinding& axisBinding, KeyBinding& plusKeyBinding, KeyBinding& minusKeyBinding, float& axisValue,
                                     float& precisionAxis) {
    axisValue = axisBinding.GetAxis();
    if (precisionMode) {
        precisionAxis = PrecisionStep(minusKeyBinding, plusKeyBinding, precisionAxis, axisValue);
        axisValue = precisionAxis;
    } else {
        axisValue = KeyOrValue(minusKeyBinding, plusKeyBinding, axisValue);
        precisionAxis = axisValue;
    }
}

// FlightInputHandler.cs:377
void FlightInputHandler::FixedUpdate() {
    if (!HighLogic::LoadedSceneIsFlight || !FlightRegistry::ready) return;
    using namespace GameSettings;
    const IInputDevice& input = InputEnvironment::Input();
    if (!FlightDriver::Pause() && !FlightRegistry::ActiveVessel->isEVA()) {
        throttle = f32store(fx(state.mainThrottle) * 2.0 - 1.0);
        if (Unlocked(ControlTypes::THROTTLE)) {
            throttle = THROTTLE_UP->GetKey() ? Mathf::Min(1.0f, f32store(fx(throttle) + 1.0 * fx(Time::deltaTime)))
                                             : (THROTTLE_DOWN->GetKey() ? Mathf::Max(-1.0f, f32store(fx(throttle) - 1.0 * fx(Time::deltaTime)))
                                                                        : throttle);
            throttle = Mathf::Clamp(f32store(fx(throttle) + fx(AXIS_THROTTLE_INC->GetAxis()) * fx(Time::deltaTime)), -1.0f, 1.0f);
            axisThrottle = AXIS_THROTTLE->GetAxis();
            if (refocusThrottle) {
                lastAxisThrottle = axisThrottle;
                refocusThrottle = false;
                throttleFocus = false;
            }
            if (throttleFocus ? (axisThrottle != lastAxisThrottle)
                              : (Mathf::Abs(f32store(fx(axisThrottle) - fx(lastAxisThrottle))) > 0.25f)) {
                throttle = axisThrottle;
                lastAxisThrottle = throttle;
                throttleFocus = true;
            }
        }
        if (Unlocked(ControlTypes::ROLL)) {
            state.roll = AXIS_ROLL->GetAxis();
            if (MODIFIER_KEY->GetKey()) {
                if (ROLL_LEFT->GetKey()) state.rollTrim = TrimStep(state.rollTrim, -1.0);
                if (ROLL_RIGHT->GetKey()) state.rollTrim = TrimStep(state.rollTrim, 1.0);
            } else {
                if (SPACENAV_USE_AS_FLIGHT_CONTROL && SpaceNavHasDevice()) {
                    state.roll = f32store(fx(state.roll) - fx(QuaternionRoll(SpaceNavigator::Rotation())) * fx(SPACENAV_FLIGHT_SENS_ROT));
                }
                if (precisionMode) {
                    precisionRoll = PrecisionStep(*ROLL_LEFT, *ROLL_RIGHT, precisionRoll, state.roll);
                    state.roll = precisionRoll;
                } else {
                    state.roll = KeyOrValue(*ROLL_LEFT, *ROLL_RIGHT, state.roll);
                    precisionRoll = state.roll;
                }
            }
        } else {
            state.roll = 0.0f;
        }
        if (Unlocked(ControlTypes::PITCH)) {
            state.pitch = AXIS_PITCH->GetAxis();
            if (MODIFIER_KEY->GetKey()) {
                if (PITCH_DOWN->GetKey()) state.pitchTrim = TrimStep(state.pitchTrim, -1.0);
                if (PITCH_UP->GetKey()) state.pitchTrim = TrimStep(state.pitchTrim, 1.0);
            } else {
                if (SPACENAV_USE_AS_FLIGHT_CONTROL && SpaceNavHasDevice()) {
                    state.pitch = f32store(fx(state.pitch) - fx(QuaternionPitch(SpaceNavigator::Rotation())) * fx(SPACENAV_FLIGHT_SENS_ROT));
                }
                if (precisionMode) {
                    precisionPitch = PrecisionStep(*PITCH_DOWN, *PITCH_UP, precisionPitch, state.pitch);
                    state.pitch = precisionPitch;
                } else {
                    state.pitch = KeyOrValue(*PITCH_DOWN, *PITCH_UP, state.pitch);
                    precisionPitch = state.pitch;
                }
            }
        } else {
            state.pitch = 0.0f;
        }
        if (Unlocked(ControlTypes::YAW)) {
            state.yaw = AXIS_YAW->GetAxis();
            if (MODIFIER_KEY->GetKey()) {
                if (YAW_LEFT->GetKey()) state.yawTrim = TrimStep(state.yawTrim, -1.0);
                if (YAW_RIGHT->GetKey()) state.yawTrim = TrimStep(state.yawTrim, 1.0);
            } else {
                if (SPACENAV_USE_AS_FLIGHT_CONTROL && SpaceNavHasDevice()) {
                    state.yaw = f32store(fx(state.yaw) + fx(QuaternionYaw(SpaceNavigator::Rotation())) * fx(SPACENAV_FLIGHT_SENS_ROT));
                }
                if (precisionMode) {
                    precisionYaw = PrecisionStep(*YAW_LEFT, *YAW_RIGHT, precisionYaw, state.yaw);
                    state.yaw = precisionYaw;
                } else {
                    state.yaw = KeyOrValue(*YAW_LEFT, *YAW_RIGHT, state.yaw);
                    precisionYaw = state.yaw;
                }
            }
        } else {
            state.yaw = 0.0f;
        }
        if (MODIFIER_KEY->GetKey() && input.GetKeyDown(KeyCode::X)) {
            if (Unlocked(ControlTypes::PITCH)) state.pitchTrim = 0.0f;
            if (Unlocked(ControlTypes::ROLL)) state.rollTrim = 0.0f;
            if (Unlocked(ControlTypes::YAW)) state.yawTrim = 0.0f;
            if (Unlocked(ControlTypes::WHEEL_STEER)) state.wheelSteerTrim = 0.0f;
            if (Unlocked(ControlTypes::WHEEL_THROTTLE)) state.wheelThrottleTrim = 0.0f;
        }
        if (Unlocked(ControlTypes::LINEAR)) {
            state.X = f32store(0.0 - fx(state.X));
            state.Y = f32store(0.0 - fx(state.Y));
            state.Z = f32store(0.0 - fx(state.Z));
            ProcessAxis(*AXIS_TRANSLATE_X, *TRANSLATE_RIGHT, *TRANSLATE_LEFT, state.X, precisionX);
            ProcessAxis(*AXIS_TRANSLATE_Y, *TRANSLATE_DOWN, *TRANSLATE_UP, state.Y, precisionY);
            state.Y = f32store(0.0 - fx(state.Y));
            ProcessAxis(*AXIS_TRANSLATE_Z, *TRANSLATE_FWD, *TRANSLATE_BACK, state.Z, precisionZ);
            if (SPACENAV_USE_AS_FLIGHT_CONTROL) {
                state.X = f32store(fx(state.X) + fx(SpaceNavigator::Translation().x) * fx(SPACENAV_FLIGHT_SENS_LIN));
                state.Y = f32store(fx(state.Y) + fx(SpaceNavigator::Translation().y) * fx(SPACENAV_FLIGHT_SENS_LIN));
                state.Z = f32store(fx(state.Z) + fx(SpaceNavigator::Translation().z) * fx(SPACENAV_FLIGHT_SENS_LIN));
            }
        } else {
            state.X = 0.0f;
            state.Y = 0.0f;
            state.Z = 0.0f;
        }
        state.mainThrottle = throttle;
        if (Unlocked(ControlTypes::WHEEL_STEER)) {
            state.wheelSteer = AXIS_WHEEL_STEER->GetAxis();
            if (MODIFIER_KEY->GetKey()) {
                if (WHEEL_STEER_RIGHT->GetKey()) state.wheelSteerTrim = TrimStep(state.wheelSteerTrim, -1.0);
                if (WHEEL_STEER_LEFT->GetKey()) state.wheelSteerTrim = TrimStep(state.wheelSteerTrim, 1.0);
            } else {
                if (SPACENAV_USE_AS_FLIGHT_CONTROL && SpaceNavHasDevice()) {
                    state.wheelSteer = f32store(fx(state.wheelSteer) - fx(QuaternionYaw(SpaceNavigator::Rotation())) * fx(SPACENAV_FLIGHT_SENS_ROT));
                }
                if (precisionMode) {
                    precisionWheelSteer = PrecisionStep(*WHEEL_STEER_RIGHT, *WHEEL_STEER_LEFT, precisionWheelSteer, state.wheelSteer);
                    state.wheelSteer = precisionWheelSteer;
                } else {
                    state.wheelSteer = KeyOrValue(*WHEEL_STEER_RIGHT, *WHEEL_STEER_LEFT, state.wheelSteer);
                    precisionWheelSteer = state.wheelSteer;
                }
            }
        } else {
            state.wheelSteer = 0.0f;
        }
        if (Unlocked(ControlTypes::WHEEL_THROTTLE)) {
            state.wheelThrottle = AXIS_WHEEL_THROTTLE->GetAxis();
            if (MODIFIER_KEY->GetKey()) {
                if (WHEEL_THROTTLE_DOWN->GetKey()) state.wheelThrottleTrim = TrimStep(state.wheelThrottleTrim, -1.0);
                if (WHEEL_THROTTLE_UP->GetKey()) state.wheelThrottleTrim = TrimStep(state.wheelThrottleTrim, 1.0);
            } else {
                if (SPACENAV_USE_AS_FLIGHT_CONTROL) {
                    state.wheelThrottle = f32store(fx(state.wheelThrottle) + fx(SpaceNavigator::Translation().z) * fx(SPACENAV_FLIGHT_SENS_LIN));
                }
                if (precisionMode) {
                    precisionWheelThrottle = PrecisionStep(*WHEEL_THROTTLE_DOWN, *WHEEL_THROTTLE_UP, precisionWheelThrottle, state.wheelThrottle);
                    state.wheelThrottle = precisionWheelThrottle;
                } else {
                    state.wheelThrottle = KeyOrValue(*WHEEL_THROTTLE_DOWN, *WHEEL_THROTTLE_UP, state.wheelThrottle);
                    precisionWheelThrottle = state.wheelThrottle;
                }
            }
        } else {
            state.wheelThrottle = 0.0f;
        }
        if (Unlocked(ControlTypes::CUSTOM_ACTION_GROUPS)) {
            int num = AXIS_CUSTOM->Length();
            if (num > static_cast<int>(state.custom_axes->size())) num = static_cast<int>(state.custom_axes->size());
            if (precision_custom_axis.empty() || static_cast<int>(precision_custom_axis.size()) < num) {
                precision_custom_axis.assign(static_cast<size_t>(num), 0.0f);
            }
            for (int i = 0; i < num; i++) {
                AxisKeyBinding& b = (*AXIS_CUSTOM)[i];
                ProcessAxis(*b.axisBinding, *b.plusKeyBinding, *b.minusKeyBinding, (*state.custom_axes)[static_cast<size_t>(i)],
                            precision_custom_axis[static_cast<size_t>(i)]);
            }
        }
        OnRawAxisInput(state);
        state.mainThrottle = f32store((fx(state.mainThrottle) + 1.0) * fx(0.5f));
        state.roll = f32store(fx(state.roll) + fx(state.rollTrim));
        state.pitch = f32store(fx(state.pitch) + fx(state.pitchTrim));
        state.yaw = f32store(fx(state.yaw) + fx(state.yawTrim));
        state.wheelSteer = f32store(fx(state.wheelSteer) + fx(state.wheelSteerTrim));
        state.wheelThrottle = f32store(fx(state.wheelThrottle) + fx(state.wheelThrottleTrim));
        state.X = f32store(0.0 - fx(state.X));
        state.Z = f32store(0.0 - fx(state.Z));
    } else {
        state.pitch = 0.0f;
        state.yaw = 0.0f;
        state.roll = 0.0f;
        state.X = 0.0f;
        state.Y = 0.0f;
        state.Z = 0.0f;
    }
    if (FlightRegistry::ActiveVessel == nullptr) return;
    for (size_t j = 0; j < FlightRegistry::VesselsLoaded.size(); j++) {
        Vessel* vessel = FlightRegistry::VesselsLoaded[j];
        if (!vessel->packed) {
            if (vessel == FlightRegistry::ActiveVessel) {
                vessel->SetControlState(state);
            } else {
                vessel->ctrlState.pitch = f32store(0.0 + fx(vessel->ctrlState.pitchTrim));
                vessel->ctrlState.yaw = f32store(0.0 + fx(vessel->ctrlState.yawTrim));
                vessel->ctrlState.roll = f32store(0.0 + fx(vessel->ctrlState.rollTrim));
                vessel->ctrlState.X = 0.0f;
                vessel->ctrlState.Y = 0.0f;
                vessel->ctrlState.Z = 0.0f;
            }
            if (host.feedInputFeed) host.feedInputFeed(vessel);
        }
    }
}

// FlightInputHandler.cs:695
void FlightInputHandler::OnDestroy() {
    GE::OnVesselOverrideGroupChanged.Remove(this, &FlightInputHandler::OnVesselOverrideGroupChanged);
    state.mainThrottle = 0.0f;
    state.pitch = 0.0f;
    state.yaw = 0.0f;
    state.roll = 0.0f;
    state.X = 0.0f;
    state.Y = 0.0f;
    state.Z = 0.0f;
    InputLockManager::RemoveControlLock("manualStageLock");
    if (fetch != nullptr && fetch == this) fetch = nullptr;
}

// AxisGroupsInputHandler.cs:8
void AxisGroupsInputHandler::Awake() {
    FlightInputHandler::OnRawAxisInput =
        FlightInputHandler::OnRawAxisInput + FlightInputCallback::Bind(this, &AxisGroupsInputHandler::AxisInputHandler);
    GE::onVesselChange.Add(this, &AxisGroupsInputHandler::onVesselChange);
}

// AxisGroupsInputHandler.cs:14
void AxisGroupsInputHandler::OnDestroy() {
    FlightInputHandler::OnRawAxisInput =
        FlightInputHandler::OnRawAxisInput - FlightInputCallback::Bind(this, &AxisGroupsInputHandler::AxisInputHandler);
    GE::onVesselChange.Remove(this, &AxisGroupsInputHandler::onVesselChange);
}

// AxisGroupsInputHandler.cs:20
void AxisGroupsInputHandler::onVesselChange(Vessel* vessel) {
    VesselAxisGroups = nullptr;
    int count = static_cast<int>(vessel->vesselModules.size());
    while (count-- > 0) {
        AxisGroupsModule* m = dynamic_cast<AxisGroupsModule*>(vessel->vesselModules[static_cast<size_t>(count)].get());
        if (m != nullptr) {
            VesselAxisGroups = m;
            return;
        }
    }
}

// AxisGroupsInputHandler.cs:38
void AxisGroupsInputHandler::AxisInputHandler(FlightCtrlState& ctrlState) {
    if (VesselAxisGroups == nullptr) return;
    VesselAxisGroups->UpdateAxisGroup(KSPAxisGroup::Pitch, ctrlState.pitch);
    VesselAxisGroups->UpdateAxisGroup(KSPAxisGroup::Yaw, ctrlState.yaw);
    VesselAxisGroups->UpdateAxisGroup(KSPAxisGroup::Roll, ctrlState.roll);
    VesselAxisGroups->UpdateAxisGroup(KSPAxisGroup::TranslateX, ctrlState.X);
    VesselAxisGroups->UpdateAxisGroup(KSPAxisGroup::TranslateY, ctrlState.Y);
    VesselAxisGroups->UpdateAxisGroup(KSPAxisGroup::TranslateZ, ctrlState.Z);
    VesselAxisGroups->UpdateAxisGroup(KSPAxisGroup::MainThrottle, ctrlState.mainThrottle);
    VesselAxisGroups->UpdateAxisGroup(KSPAxisGroup::WheelSteer, ctrlState.wheelSteer);
    VesselAxisGroups->UpdateAxisGroup(KSPAxisGroup::WheelThrottle, ctrlState.wheelThrottle);
    for (int i = 0; i < static_cast<int>(ctrlState.custom_axes->size()); i++) {
        int axisGroup = 512 << i;
        VesselAxisGroups->UpdateAxisGroup(static_cast<KSPAxisGroup>(axisGroup), (*ctrlState.custom_axes)[static_cast<size_t>(i)]);
    }
}

}  // namespace ksp
