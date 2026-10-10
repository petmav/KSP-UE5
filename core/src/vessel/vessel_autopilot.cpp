// WP-24: VesselAutopilot.cs (see ksp/vessel/vessel_autopilot.hpp for the scope and the seams).
#include "ksp/vessel/vessel_autopilot.hpp"

#include <cmath>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/kerbals/kerbal_misc.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/modules/control_types.hpp"
#include "ksp/modules/module_gimbal.hpp"
#include "ksp/modules/module_rcs.hpp"
#include "ksp/orbit/maneuver_node.hpp"
#include "ksp/orbit/patched_conic_solver.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/flight_ctrl_state.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using AutopilotMode = VesselAutopilot::AutopilotMode;
using VesselSAS = VesselAutopilot::VesselSAS;

void AutopilotEnvironment::Reset() {
    eulerAngles = nullptr;
    pitchInput = rollInput = yawInput = nullptr;
    speedDisplayMode = nullptr;
    displayVelocitySqrMagnitude = nullptr;
    screenMessages.clear();
    killAngularVelocity = nullptr;
    precisionMode = nullptr;
    torqueProviderHook = nullptr;
}

namespace {
// Quaternion.Internal_MakePositive (Quaternion.cs:240, managed)
Vector3f MakePositive(Vector3f euler) {
    const float num = -0.005729578f;
    const float num2 = f32store(360.0 + fx(num));
    auto fix = [&](float& c) {
        if (c < num) c = f32store(fx(c) + 360.0);
        else if (c > num2) c = f32store(fx(c) - 360.0);
    };
    fix(euler.x);
    fix(euler.y);
    fix(euler.z);
    return euler;
}
// Native Unity QuaternionToEuler (YXZ order, single precision per operation): the formulation of Unity's Math/Quaternion.cpp. The native
// build is not in the captures; the trig calls go through ksp::dotnet (the double result rounded to float).
Vector3f ToEulerRad(const Quaternionf& q) {
    const float x = q.x * 2.0f, y = q.y * 2.0f, z = q.z * 2.0f;
    const float xx = q.x * x, yy = q.y * y, zz = q.z * z;
    const float xy = q.x * y, xz = q.x * z, yz = q.y * z;
    const float wx = q.w * x, wy = q.w * y, wz = q.w * z;
    float m[9];
    m[0] = 1.0f - (yy + zz);
    m[1] = xy + wz;
    m[2] = xz - wy;
    m[3] = xy - wz;
    m[4] = 1.0f - (xx + zz);
    m[5] = yz + wx;
    m[6] = xz + wy;
    m[7] = yz - wx;
    m[8] = 1.0f - (xx + yy);
    auto g = [&](int r, int c) { return m[r + 3 * c]; };
    Vector3f v;
    if (g(1, 2) < 0.999f) {
        if (g(1, 2) > -0.999f) {
            v.x = Mathf::Asin(-g(1, 2));
            v.y = Mathf::Atan2(g(0, 2), g(2, 2));
            v.z = Mathf::Atan2(g(1, 0), g(1, 1));
        } else {
            v.x = Mathf::PI * 0.5f;
            v.y = Mathf::Atan2(g(0, 1), g(0, 0));
            v.z = 0.0f;
        }
    } else {
        v.x = -Mathf::PI * 0.5f;
        v.y = Mathf::Atan2(-g(0, 1), g(0, 0));
        v.z = 0.0f;
    }
    return v;
}
Vector3f EulerAngles(const Quaternionf& q) {
    if (AutopilotEnvironment::eulerAngles) return AutopilotEnvironment::eulerAngles(q);
    return AutopilotEnvironment::DefaultEulerAngles(q);
}
}  // namespace

// see Quaternion.cs:68 eulerAngles = Internal_MakePositive(Internal_ToEulerRad(this) * 57.29578f)
Vector3f AutopilotEnvironment::DefaultEulerAngles(const Quaternionf& q) {
    const Vector3f r = ToEulerRad(q);
    return MakePositive(r * 57.29578f);
}

// ======================================================= VesselAutopilot.VesselSAS

// VesselAutopilot.cs:108
VesselSAS::VesselSAS(Vessel& v) {
    SetLockPitchPID(15000.0f, 0.0f, 2000.0f, 1500.0f);
    SetLockRollPID(9000.0f, 0.0f, 600.0f, 2000.0f);
    SetLockYawPID(15000.0f, 0.0f, 2000.0f, 1500.0f);
    SetVessel(v);
}

// VesselAutopilot.cs:116
void VesselSAS::SetLockYawPID(float Kp, float Ki, float Kd, float clamp) { pidLockedYaw = PIDclamp("Torque Yaw", Kp, Ki, Kd, clamp); }

// VesselAutopilot.cs:121
void VesselSAS::SetLockRollPID(float Kp, float Ki, float Kd, float clamp) { pidLockedRoll = PIDclamp("Torque Roll", Kp, Ki, Kd, clamp); }

// VesselAutopilot.cs:126
void VesselSAS::SetLockPitchPID(float Kp, float Ki, float Kd, float clamp) { pidLockedPitch = PIDclamp("Torque Pitch", Kp, Ki, Kd, clamp); }

// VesselAutopilot.cs:131
void VesselSAS::ResetAllPIDS() {
    pidLockedPitch.Reset();
    pidLockedRoll.Reset();
    pidLockedYaw.Reset();
}

// VesselAutopilot.cs:138
void VesselSAS::ResetTuningScalars() {
    pidLockedPitch.tuningScalar = sasTuningScalar_;
    pidLockedRoll.tuningScalar = sasTuningScalar_;
    pidLockedYaw.tuningScalar = sasTuningScalar_;
    AutoTuneReset();
}

// VesselAutopilot.cs:146
void VesselSAS::TuneScalars(float scalar) {
    pidLockedPitch.tuningScalar = scalar;
    pidLockedRoll.tuningScalar = scalar;
    pidLockedYaw.tuningScalar = scalar;
}

// VesselAutopilot.cs:153
void VesselSAS::SetTuningScalar(float scalar) {
    sasTuningScalar_ = scalar;
    ResetTuningScalars();
}

// VesselAutopilot.cs:159
void VesselSAS::SetVessel(Vessel& v) {
    vessel_ = &v;
    storedTransform_ = &v.ReferenceTransform();
}

// VesselAutopilot.cs:165
bool VesselSAS::CanEngageSAS() { return APSkillExtensions::AvailableAtLevel(AutopilotMode::StabilityAssist, *vessel_); }

// VesselAutopilot.cs:170
void VesselSAS::LockRotation(const Quaternionf& newRotation) { lockedRotation = newRotation; }

// VesselAutopilot.cs:175 (the GameEvents.onVesselWasModified subscription is the host's: it calls VesselModified)
void VesselSAS::Start() {
    if (vessel_ != nullptr) {
        LockRotation(vessel_->ReferenceTransform().rotation);
        ResetAllPIDS();
    }
    sasEquippedVessel_ = CanEngageSAS();
    isStarted_ = true;
}

// VesselAutopilot.cs:187
void VesselSAS::Update() {
    if (!isStarted_) Start();
    sasEquippedVessel_ = CanEngageSAS();
    if (ResourceFlowEnvironment::LoadedSceneIsFlight()) SASUpdate();
}

// VesselAutopilot.cs:200
void VesselSAS::SASUpdate() {
    if (vessel_ == nullptr) return;
    if (!sasEquippedVessel_) {
        if (FBWconnected_) {
            DisconnectFlyByWire();
            ResetAllPIDS();
        }
    } else {
        CheckStoredTransform();
    }
}

// VesselAutopilot.cs:220
void VesselSAS::CheckStoredTransform() {
    if (storedTransform_ != &vessel_->ReferenceTransform() || storedTransform_ == nullptr) {
        storedTransform_ = &vessel_->ReferenceTransform();
        LockRotation(storedTransform_->rotation);
        ResetAllPIDS();
    }
}

// VesselAutopilot.cs:230 (FlightGlobals.ActiveVessel test here; the GameSettings axes / SpaceNavigator / keys are AutopilotEnvironment::yawInput)
bool VesselSAS::CheckYawInput() {
    if (FlightRegistry::ActiveVessel != vessel_) return false;
    return AutopilotEnvironment::yawInput ? AutopilotEnvironment::yawInput(controlDetectionThreshold) : false;
}

// VesselAutopilot.cs:259
bool VesselSAS::CheckRollInput() {
    if (FlightRegistry::ActiveVessel != vessel_) return false;
    return AutopilotEnvironment::rollInput ? AutopilotEnvironment::rollInput(controlDetectionThreshold) : false;
}

// VesselAutopilot.cs:288
bool VesselSAS::CheckPitchInput() {
    if (FlightRegistry::ActiveVessel != vessel_) return false;
    return AutopilotEnvironment::pitchInput ? AutopilotEnvironment::pitchInput(controlDetectionThreshold) : false;
}

// VesselAutopilot.cs:317
bool VesselSAS::UpdatePitchInput() {
    if (CheckPitchInput() && vessel_->ctrlState.pitch != 0.0f) {
        if (vessel_->ctrlState.pitch < 0.0f) {
            if (!(sasResponse_.x < 0.0)) {
                sasResponse_.x = vessel_->ctrlState.pitch;
                return true;
            }
            if (fx(vessel_->ctrlState.pitch) < sasResponse_.x) {
                sasResponse_.x = vessel_->ctrlState.pitch;
                return true;
            }
        } else {
            if (!(sasResponse_.x > 0.0)) {
                sasResponse_.x = vessel_->ctrlState.pitch;
                return true;
            }
            if (fx(vessel_->ctrlState.pitch) > sasResponse_.x) {
                sasResponse_.x = vessel_->ctrlState.pitch;
                return true;
            }
        }
    }
    return false;
}

// VesselAutopilot.cs:351
bool VesselSAS::UpdateRollInput() {
    if (CheckRollInput() && vessel_->ctrlState.roll != 0.0f) {
        if (vessel_->ctrlState.roll < 0.0f) {
            if (!(sasResponse_.y < 0.0)) {
                sasResponse_.y = vessel_->ctrlState.roll;
                return true;
            }
            if (fx(vessel_->ctrlState.roll) < sasResponse_.y) {
                sasResponse_.y = vessel_->ctrlState.roll;
                return true;
            }
        } else {
            if (!(sasResponse_.y > 0.0)) {
                sasResponse_.y = vessel_->ctrlState.roll;
                return true;
            }
            if (fx(vessel_->ctrlState.roll) > sasResponse_.y) {
                sasResponse_.y = vessel_->ctrlState.roll;
                return true;
            }
        }
    }
    return false;
}

// VesselAutopilot.cs:385
bool VesselSAS::UpdateYawInput() {
    if (CheckYawInput() && vessel_->ctrlState.yaw != 0.0f) {
        if (vessel_->ctrlState.yaw < 0.0f) {
            if (!(sasResponse_.z < 0.0)) {
                sasResponse_.z = vessel_->ctrlState.yaw;
                return true;
            }
            if (fx(vessel_->ctrlState.yaw) < sasResponse_.z) {
                sasResponse_.z = vessel_->ctrlState.yaw;
                return true;
            }
        } else {
            if (!(sasResponse_.z > 0.0)) {
                sasResponse_.z = vessel_->ctrlState.yaw;
                return true;
            }
            if (fx(vessel_->ctrlState.yaw) > sasResponse_.z) {
                sasResponse_.z = vessel_->ctrlState.yaw;
                return true;
            }
        }
    }
    return false;
}

// VesselAutopilot.cs:419
float VesselSAS::EvaluateScalar(float ratio) {
    if (ratio <= 0.0f) return 10.0f;
    if (fx(ratio) <= 0.1) return f32store(fx(-90.0f) * fx(ratio) + fx(10.0f));
    if (ratio <= 10.0f) return 1.0f;
    if (ratio <= 50.0f) return f32store(fx(-0.0167f) * fx(ratio) + fx(1.1667f));
    return 0.5f;
}

// VesselAutopilot.cs:440
void VesselSAS::AutoTuneScalar() {
    if (autoTune) {
        float value = Mathf::Min(autoScalarMax.x, EvaluateScalar(angularAccelerationMax_.x));
        autoScalar_.x = Mathf::Clamp(value, autoScalarMin.x, autoScalarMax.x);
        value = Mathf::Min(autoScalarMax.y, EvaluateScalar(angularAccelerationMax_.y));
        autoScalar_.y = Mathf::Clamp(value, autoScalarMin.y, autoScalarMax.y);
        value = Mathf::Min(autoScalarMax.z, EvaluateScalar(angularAccelerationMax_.z));
        autoScalar_.z = Mathf::Clamp(value, autoScalarMin.z, autoScalarMax.z);
        const float rate = TimeWarp::CurrentRate();
        value = autoScalar_.x;
        pidLockedPitch.tuningScalar = f32store(fx(sasTuningScalar_) * fx(value) / fx(rate));
        pidLockedPitch.clampScalar = f32store(fx(rate) / fx(decayScalar_.x));
        value = autoScalar_.y;
        pidLockedRoll.tuningScalar = f32store(fx(sasTuningScalar_) * fx(value) / fx(rate));
        pidLockedRoll.clampScalar = f32store(fx(rate) / fx(decayScalar_.y));
        value = autoScalar_.z;
        pidLockedYaw.tuningScalar = f32store(fx(sasTuningScalar_) * fx(value) / fx(rate));
        pidLockedYaw.clampScalar = f32store(fx(rate) / fx(decayScalar_.z));
    } else {
        TuneScalars(f32store(fx(sasTuningScalar_) / fx(TimeWarp::CurrentRate())));
    }
}

// VesselAutopilot.cs:466
void VesselSAS::AutoTuneReset() {
    autoScalar_ = Vector3f::one();
    pidLockedPitch.kdScalar = 1.0f;
    pidLockedRoll.kdScalar = 1.0f;
    pidLockedYaw.kdScalar = 1.0f;
}

// VesselAutopilot.cs:474 (the ITorqueProvider modules: PartModule::AsTorqueProvider; ModuleRCS and ModuleGimbal take the scene inputs the
// C# reads inside GetPotentialTorque; AutopilotEnvironment::torqueProviderHook serves the rest)
Vector3f VesselSAS::GetTotalVesselTorque(Vessel& v) {
    (void)v;
    Vector3f zero = Vector3f::zero();
    posTorque = Vector3f::zero();
    negTorque = Vector3f::zero();
    int count = static_cast<int>(vessel_->parts.size());
    while (count-- > 0) {
        Part* part = vessel_->parts[static_cast<size_t>(count)];
        int count2 = static_cast<int>(part->Modules().Count());
        while (count2-- > 0) {
            PartModule* module = part->Modules()[static_cast<size_t>(count2)];
            Vector3f pos = Vector3f::zero(), neg = Vector3f::zero();
            bool isProvider = false;
            if (AutopilotEnvironment::torqueProviderHook && AutopilotEnvironment::torqueProviderHook(*vessel_, *module, pos, neg)) {
                isProvider = true;
            } else if (ITorqueProvider* tp = module->AsTorqueProvider()) {
                tp->GetPotentialTorque(pos, neg);
                isProvider = true;
            } else if (auto* rcs = dynamic_cast<ModuleRCS*>(module)) {
                rcs->GetPotentialTorque(pos, neg, vessel_->CurrentCoM(), AutopilotEnvironment::precisionMode ? AutopilotEnvironment::precisionMode() : true);
                isProvider = true;
            } else if (auto* gimbal = dynamic_cast<ModuleGimbal*>(module)) {
                gimbal->GetPotentialTorque(pos, neg, vessel_->ReferenceTransform(), vessel_->CurrentCoM());
                isProvider = true;
            }
            if (isProvider) {
                posTorque += pos;
                negTorque += neg;
            }
        }
    }
    zero.x = Mathf::Max(posTorque.x, negTorque.x);
    zero.y = Mathf::Max(posTorque.y, negTorque.y);
    zero.z = Mathf::Max(posTorque.z, negTorque.z);
    return zero;
}

// VesselAutopilot.cs:500
void VesselSAS::UpdateMaximumAcceleration() {
    angularAccelerationMax_.x = Mathf::Max(f32store(fx(torqueVector_.x) / fx(vessel_->MOI.x)), 0.0001f);
    angularAccelerationMax_.y = Mathf::Max(f32store(fx(torqueVector_.y) / fx(vessel_->MOI.y)), 0.0001f);
    angularAccelerationMax_.z = Mathf::Max(f32store(fx(torqueVector_.z) / fx(vessel_->MOI.z)), 0.0001f);
}

// VesselAutopilot.cs:507
void VesselSAS::UpdateVesselTorque(FlightCtrlState& s) {
    (void)s;
    torqueVector_ = GetTotalVesselTorque(*vessel_);
}

// VesselAutopilot.cs:517
void VesselSAS::ConnectFlyByWire(bool reset) {
    if (!FBWconnected_) {
        vessel_->OnAutopilotUpdate += Delegate<void(FlightCtrlState&)>::Bind(this, &VesselSAS::ControlUpdate);
        vessel_->OnAutopilotUpdate += Delegate<void(FlightCtrlState&)>::Bind(this, &VesselSAS::UpdateVesselTorque);
        FBWconnected_ = true;
        if (!(storedTransform_ != &vessel_->ReferenceTransform()) && !(storedTransform_ == nullptr)) {
            LockRotation(storedTransform_->rotation);
        } else {
            storedTransform_ = &vessel_->ReferenceTransform();
            LockRotation(storedTransform_->rotation);
        }
    }
    if (reset) ResetAllPIDS();
}

// VesselAutopilot.cs:542
void VesselSAS::DisconnectFlyByWire() {
    targetOrientation = Vector3f::zero();
    lockedRotation = Quaternionf::identity();
    ResetAllPIDS();
    if (FBWconnected_) {
        vessel_->OnAutopilotUpdate -= Delegate<void(FlightCtrlState&)>::Bind(this, &VesselSAS::ControlUpdate);
        vessel_->OnAutopilotUpdate -= Delegate<void(FlightCtrlState&)>::Bind(this, &VesselSAS::UpdateVesselTorque);
        FBWconnected_ = false;
    }
}

// VesselAutopilot.cs:557
void VesselSAS::SetTargetOrientation(const Vector3f& tgtOrientation, bool reset) {
    ConnectFlyByWire(reset);
    targetOrientation = Vector3f::Normalize(tgtOrientation);
    CheckStoredTransform();
}

// VesselAutopilot.cs:578
Vector3f VesselSAS::PitchYawAngle(const PartTransform& t, const Vector3f& v2) {
    const Vector3f vector = t.InverseTransformDirection(v2);
    Vector3f result{};
    result.x = f32store(90.0 - fx(Mathf::Atan2(vector.y, vector.z)) * fx(57.29578f));
    if (result.x > 180.0f) result.x = f32store(fx(result.x) - 360.0);
    result.y = 0.0f;
    result.z = f32store(fx(Mathf::Atan2(vector.x, vector.y)) * fx(57.29578f));
    return result;
}

// VesselAutopilot.cs:578
void VesselSAS::PitchYawAngle(const PartTransform& t, const Vector3f& v2, float& pitch, float& yaw) {
    const Vector3f vector = t.InverseTransformDirection(v2);
    pitch = f32store(90.0 - fx(Mathf::Atan2(vector.y, vector.z)) * fx(57.29578f));
    if (pitch > 180.0f) pitch = f32store(fx(pitch) - 360.0);
    yaw = f32store(fx(Mathf::Atan2(vector.x, vector.y)) * fx(57.29578f));
}

// VesselAutopilot.cs:589
void VesselSAS::CheckCoasting() {
    if (angularAccelerationMax_.x > 0.0f) {
        const float num = Mathf::Abs(f32store(fx(Time::deltaTime) + fx(vessel_->angularVelocity.x) / fx(angularAccelerationMax_.x)));
        const float num2 = Mathf::Abs(f32store(fx(rotationDelta_.x) * fx(Mathf::Deg2Rad) / fx(vessel_->angularVelocity.x)));
        if (fx(num) * fx(stopScalar) > fx(num2) && dotnet::Math::Sign(rotationDelta_.x) != dotnet::Math::Sign(vessel_->angularVelocity.x)) {
            pidLockedPitch.Reset();
            rotationDelta_.x = f32store(0.0 - fx(rotationDelta_.x));
        } else if (fx(num) * fx(coastScalar) > fx(num2) &&
                   dotnet::Math::Sign(rotationDelta_.x) != dotnet::Math::Sign(vessel_->angularVelocity.x)) {
            pidLockedPitch.Reset();
            rotationDelta_.x = 0.0f;
        }
    }
    if (angularAccelerationMax_.z > 0.0f) {
        const float num3 = Mathf::Abs(f32store(fx(Time::deltaTime) + fx(vessel_->angularVelocity.z) / fx(angularAccelerationMax_.z)));
        const float num4 = Mathf::Abs(f32store(fx(rotationDelta_.z) * fx(Mathf::Deg2Rad) / fx(vessel_->angularVelocity.z)));
        if (fx(num3) * fx(stopScalar) > fx(num4) && dotnet::Math::Sign(rotationDelta_.z) != dotnet::Math::Sign(vessel_->angularVelocity.z)) {
            pidLockedYaw.Reset();
            rotationDelta_.z = f32store(0.0 - fx(rotationDelta_.z));
        } else if (fx(num3) * fx(coastScalar) > fx(num4) &&
                   dotnet::Math::Sign(rotationDelta_.z) != dotnet::Math::Sign(vessel_->angularVelocity.z)) {
            pidLockedYaw.Reset();
            rotationDelta_.z = 0.0f;
        }
    }
}

// VesselAutopilot.cs:623
void VesselSAS::StabilityDecay() {
    if (useDecay) {
        if (std::fabs(sasResponse_.x) < fx(decayResponseThreshold.x) && angularDeltaRad_.x < fx(decayDeltaThreshold.x)) {
            decayScalar_.x = Mathf::Max(f32store(fx(decayScalar_.x) - fx(decayRate.x)), decayMin.x);
        } else {
            decayScalar_.x = Mathf::Min(f32store(fx(decayScalar_.x) + fx(0.1f)), 1.0f);
        }
        if (std::fabs(sasResponse_.y) < fx(decayResponseThreshold.y) && angularDeltaRad_.y < fx(decayDeltaThreshold.y)) {
            decayScalar_.y = Mathf::Max(f32store(fx(decayScalar_.y) - fx(decayRate.y)), decayMin.y);
        } else {
            decayScalar_.y = Mathf::Min(f32store(fx(decayScalar_.y) + fx(0.1f)), 1.0f);
        }
        if (std::fabs(sasResponse_.z) < fx(decayResponseThreshold.z) && angularDeltaRad_.z < fx(decayDeltaThreshold.z)) {
            decayScalar_.z = Mathf::Max(f32store(fx(decayScalar_.z) - fx(decayRate.z)), decayMin.z);
        } else {
            decayScalar_.z = Mathf::Min(f32store(fx(decayScalar_.z) + fx(0.1f)), 1.0f);
        }
        if (decayScalar_ == decayMin) {
            if (!decayLocked_) {
                decayLocked_ = true;
                LockRotation(currentRotation_);
                KillAngularVelocity();
                ResetAllPIDS();
            }
        } else {
            decayLocked_ = false;
        }
    } else {
        decayScalar_ = Vector3f::one();
    }
}

// VesselAutopilot.cs:672
void VesselSAS::KillAngularVelocity() {
    if (AutopilotEnvironment::killAngularVelocity) AutopilotEnvironment::killAngularVelocity(*vessel_);
}

// VesselAutopilot.cs:688
void VesselSAS::ResetDecay() { decayScalar_ = Vector3f::one() * 1.0f; }

// VesselAutopilot.cs:693
void VesselSAS::CheckDamping() {
    dampingMode = false;
    pitchInput_ = UpdatePitchInput();
    rollInput_ = UpdateRollInput();
    yawInput_ = UpdateYawInput();
    if (pitchInput_) {
        LockRotation(currentRotation_);
        pidLockedPitch.Reset();
        dampingMode = true;
    }
    if (rollInput_) {
        LockRotation(currentRotation_);
        pidLockedRoll.Reset();
        dampingMode = true;
    }
    if (yawInput_) {
        LockRotation(currentRotation_);
        pidLockedYaw.Reset();
        dampingMode = true;
    }
    if (!pitchInput_ && !rollInput_ && !yawInput_) {
        if (--dampingCooldownTimer_ < 0) dampingCooldownTimer_ = 0;
    } else {
        dampingCooldownTimer_ = dampingCooldown;
    }
    if (Mathf::Abs(vessel_->angularVelocity.x) > overrideMinimumMagnitude) {
        dampingMode = true;
        LockRotation(lastRotation_);
        if (lockedMode) {
            if (dampingCooldownTimer_ > 0) pidLockedPitch.Reset();
            else ResetAllPIDS();
        }
    }
    if (Mathf::Abs(vessel_->angularVelocity.y) > overrideMinimumMagnitude) {
        dampingMode = true;
        LockRotation(lastRotation_);
        if (dampingCooldownTimer_ > 0) pidLockedRoll.Reset();
        else ResetAllPIDS();
    }
    if (!(Mathf::Abs(vessel_->angularVelocity.z) > overrideMinimumMagnitude)) return;
    dampingMode = true;
    LockRotation(lastRotation_);
    if (lockedMode) {
        if (dampingCooldownTimer_ > 0) pidLockedYaw.Reset();
        else ResetAllPIDS();
    }
}

// VesselAutopilot.cs:776
Quaternionf VesselSAS::GetRotationDelta() { return QuaternionExtensions::Inverse(currentRotation_) * lockedRotation; }

// VesselAutopilot.cs:781
float VesselSAS::AngularTrim(float angle) {
    if (!(angle > 180.0f)) return angle;
    return f32store(fx(angle) - 360.0);
}

// VesselAutopilot.cs:790
void VesselSAS::ControlUpdate(FlightCtrlState& s) {
    if (storedTransform_ == nullptr) return;
    currentRotation_ = storedTransform_->rotation;
    UpdateMaximumAcceleration();
    rotationDelta_ = EulerAngles(QuaternionExtensions::Inverse(GetRotationDelta()));
    if (!lockedMode) {
        PitchYawAngle(vessel_->ReferenceTransform(), targetOrientation, neededPitch_, neededYaw_);
        rotationDelta_.x = f32store(0.0 - fx(neededPitch_));
        rotationDelta_.z = neededYaw_;
        CheckCoasting();
    } else if (!dampingMode) {
        CheckCoasting();
    }
    angularDelta_.x = AngularTrim(rotationDelta_.x);
    angularDelta_.y = AngularTrim(rotationDelta_.y);
    angularDelta_.z = AngularTrim(rotationDelta_.z);
    angularDeltaRad_ = angularDelta_ * 0.01745329238474369;
    StabilityDecay();
    AutoTuneScalar();
    const double dt = fx(TimeWarp::deltaTime());
    sasResponse_.x = pidLockedPitch.Update(angularDeltaRad_.x, dt) / pidLockedPitch.clamp;
    sasResponse_.y = pidLockedRoll.Update(angularDeltaRad_.y, dt) / pidLockedRoll.clamp;
    sasResponse_.z = pidLockedYaw.Update(angularDeltaRad_.z, dt) / pidLockedYaw.clamp;
    CheckDamping();
    sasResponse_.x = UtilMath::Clamp(sasResponse_.x, -1.0, 1.0);
    sasResponse_.y = UtilMath::Clamp(sasResponse_.y, -1.0, 1.0);
    sasResponse_.z = UtilMath::Clamp(sasResponse_.z, -1.0, 1.0);
    s.pitch = static_cast<float>(sasResponse_.x);
    s.roll = static_cast<float>(sasResponse_.y);
    s.yaw = static_cast<float>(sasResponse_.z);
    lastRotation_ = currentRotation_;
}

// VesselAutopilot.cs:906
void VesselSAS::Destroy() {
    DisconnectFlyByWire();
}

// VesselAutopilot.cs:834
void VesselSAS::VesselModified(Vessel& v) {
    sasEquippedVessel_ = CanEngageSAS();
    if (vessel_ == &v && !lockedMode) DisconnectFlyByWire();
}

// VesselAutopilot.cs:843
void VesselSAS::ModuleSetup() { sasEquippedVessel_ = CanEngageSAS(); }

// ======================================================= VesselAutopilot

// VesselAutopilot.cs:896
VesselAutopilot::VesselAutopilot(Vessel& vessel) : vessel_(&vessel) { sas_ = std::make_unique<VesselSAS>(vessel); }

VesselAutopilot::~VesselAutopilot() = default;

// the Enabled setter, VesselAutopilot.cs:938 (the Enabled setter)
void VesselAutopilot::SetEnabled(bool value) {
    if (!enabled_ && value) Enable();
    else if (enabled_ && !value) Disable();
}

// VesselAutopilot.cs:906
void VesselAutopilot::Destroy() { sas_->Destroy(); }

// VesselAutopilot.cs:911
void VesselAutopilot::SetupModules() { sas_->ModuleSetup(); }

// VesselAutopilot.cs:916
void VesselAutopilot::Update() {
    if (vessel_->ActionGroups()[KSPActionGroup::SAS]) {
        if (!enabled_) {
            if (!Enable()) {
                vessel_->ActionGroups().SetGroup(KSPActionGroup::SAS, false);
                if (FlightRegistry::ActiveVessel == vessel_)
                    AutopilotEnvironment::screenMessages.push_back("<color=orange><b>" +
                                                                   Localizer::Format("#autoLOC_6003109", {vessel_->GetDisplayName()}) + "</b></color>");
            }
        } else if (!CanSetMode(mode_)) {
            if (mode_ != AutopilotMode::StabilityAssist && CanSetMode(AutopilotMode::StabilityAssist)) {
                SetMode(AutopilotMode::StabilityAssist);
            } else {
                vessel_->ActionGroups().SetGroup(KSPActionGroup::SAS, false);
                if (FlightRegistry::ActiveVessel == vessel_)
                    AutopilotEnvironment::screenMessages.push_back(Localizer::Format("#autoLOC_137027"));
                else
                    AutopilotEnvironment::screenMessages.push_back("<color=orange><b>" +
                                                                   Localizer::Format("#autoLOC_6003110", {vessel_->GetDisplayName()}) + "</b></color>");
            }
        }
    } else if (enabled_) {
        Disable();
    }
    if (enabled_) SetAutopilot(false);
}

// VesselAutopilot.cs:966
bool VesselAutopilot::Enable() { return Enable(AutopilotMode::StabilityAssist); }

// VesselAutopilot.cs:966
bool VesselAutopilot::Enable(AutopilotMode newMode) {
    if (enabled_) Disable();
    mode_ = newMode;
    sas_->lockedMode = newMode == AutopilotMode::StabilityAssist;
    if (!SetAutopilot(true)) return false;
    enabled_ = true;
    return true;
}

// VesselAutopilot.cs:982
bool VesselAutopilot::Disable() {
    sas_->DisconnectFlyByWire();
    enabled_ = false;
    vessel_->ctrlState.NeutralizeStick();
    return true;
}

// VesselAutopilot.cs:990
bool VesselAutopilot::SetAutopilot(bool initialize) {
    const AutopilotSpeedMode m = AutopilotEnvironment::speedDisplayMode ? AutopilotEnvironment::speedDisplayMode() : AutopilotSpeedMode::Orbit;
    switch (m) {
    default: return SetAutopilotOrbit(initialize);
    case AutopilotSpeedMode::Surface: return SetAutopilotSurface(initialize);
    case AutopilotSpeedMode::Target: return SetAutopilotTarget(initialize);
    }
}

// VesselAutopilot.cs:1003
bool VesselAutopilot::SetMode(AutopilotMode newMode) {
    if (!CanSetMode(newMode)) return false;
    if (enabled_) return Enable(newMode);
    mode_ = newMode;
    sas_->lockedMode = newMode == AutopilotMode::StabilityAssist;
    return true;
}

// VesselAutopilot.cs:1018
bool VesselAutopilot::CanSetMode(AutopilotMode m) {
    if (!APSkillExtensions::AvailableAtLevel(m, *vessel_)) return false;
    const AutopilotSpeedMode sm = AutopilotEnvironment::speedDisplayMode ? AutopilotEnvironment::speedDisplayMode() : AutopilotSpeedMode::Orbit;
    switch (m) {
    case AutopilotMode::Prograde:
    case AutopilotMode::Retrograde:
        if (VectorLockInvalid(*vessel_, 1.0f)) return false;
        break;
    case AutopilotMode::Normal:
    case AutopilotMode::Antinormal:
    case AutopilotMode::RadialIn:
    case AutopilotMode::RadialOut:
        if (sm == AutopilotSpeedMode::Orbit && VectorLockInvalid(*vessel_, 1.0f)) return false;
        break;
    case AutopilotMode::Target:
    case AutopilotMode::AntiTarget:
        if (TargetLockInvalid(*vessel_)) return false;
        break;
    case AutopilotMode::Maneuver:
        if (ManeuverLockInvalid(vessel_->patchedConicSolver)) return false;
        break;
    default: break;
    }
    return true;
}

namespace {
Vector3f ToVec3(const Vector3d& v) { return v.ToVector3f(); }
UnityEngine::Transform& TargetTransform(Vessel& v) {
    if (v.targetObject == nullptr) throw NullReferenceException();
    return *v.targetObject->GetTransform();
}
bool HasManeuver(Vessel& v) { return v.patchedConicSolver != nullptr && !v.patchedConicSolver->maneuverNodes.empty(); }
}  // namespace

// VesselAutopilot.cs:1059
bool VesselAutopilot::SetAutopilotOrbit(bool initialize) {
    VesselSAS& sas = *sas_;
    Vessel& vessel = *vessel_;
    switch (mode_) {
    case AutopilotMode::StabilityAssist:
        if (initialize) {
            if (!SAS().CanEngageSAS()) return false;
            sas.ConnectFlyByWire(true);
        } else {
            sas.Update();
        }
        return true;
    case AutopilotMode::Prograde: sas.SetTargetOrientation(ToVec3(vessel.obt_velocity), initialize); return true;
    case AutopilotMode::Retrograde: sas.SetTargetOrientation(ToVec3(-vessel.obt_velocity), initialize); return true;
    case AutopilotMode::Normal: sas.SetTargetOrientation(ToVec3(vessel.orbit().h.xzy()), initialize); return true;
    case AutopilotMode::Antinormal: sas.SetTargetOrientation(ToVec3(-vessel.orbit().h.xzy()), initialize); return true;
    case AutopilotMode::RadialIn:
        sas.SetTargetOrientation(-Vector3f::Cross(ToVec3(vessel.obt_velocity), ToVec3(vessel.orbit().h.xzy())), initialize);
        return true;
    case AutopilotMode::RadialOut:
        sas.SetTargetOrientation(Vector3f::Cross(ToVec3(vessel.obt_velocity), ToVec3(vessel.orbit().h.xzy())), initialize);
        return true;
    case AutopilotMode::Target:
        if (vessel.targetObject != nullptr) {
            sas.SetTargetOrientation(Vector3f::Normalize(TargetTransform(vessel).position - vessel.ReferenceTransform().position), initialize);
            return true;
        }
        return false;
    case AutopilotMode::AntiTarget:
        if (vessel.targetObject != nullptr) {
            sas.SetTargetOrientation(Vector3f::Normalize(vessel.ReferenceTransform().position - TargetTransform(vessel).position), initialize);
            return true;
        }
        return false;
    case AutopilotMode::Maneuver:
        if (!HasManeuver(vessel)) return false;
        sas.SetTargetOrientation(ToVec3(vessel.patchedConicSolver->maneuverNodes[0]->GetBurnVector(&vessel.orbit())), initialize);
        return true;
    }
    return true;
}

// VesselAutopilot.cs:1122
bool VesselAutopilot::SetAutopilotSurface(bool initialize) {
    VesselSAS& sas = *sas_;
    Vessel& vessel = *vessel_;
    switch (mode_) {
    case AutopilotMode::StabilityAssist:
        if (initialize) {
            if (!sas.CanEngageSAS()) return false;
            sas.ConnectFlyByWire(true);
        } else {
            sas.Update();
        }
        return true;
    case AutopilotMode::Prograde: sas.SetTargetOrientation(ToVec3(vessel.srf_velocity), initialize); return true;
    case AutopilotMode::Retrograde: sas.SetTargetOrientation(ToVec3(-vessel.srf_velocity), initialize); return true;
    case AutopilotMode::Normal: sas.SetTargetOrientation(ToVec3(CelestialBody::RotationAxis()), initialize); return true;
    case AutopilotMode::Antinormal: sas.SetTargetOrientation(ToVec3(-CelestialBody::RotationAxis()), initialize); return true;
    case AutopilotMode::RadialIn: sas.SetTargetOrientation(ToVec3(vessel.upAxis), initialize); return true;
    case AutopilotMode::RadialOut: sas.SetTargetOrientation(ToVec3(-vessel.upAxis), initialize); return true;
    case AutopilotMode::Target:
        if (vessel.targetObject != nullptr) {
            sas.SetTargetOrientation(Vector3f::Normalize(TargetTransform(vessel).position - vessel.ReferenceTransform().position), initialize);
            return true;
        }
        return false;
    case AutopilotMode::AntiTarget:
        if (vessel.targetObject != nullptr) {
            sas.SetTargetOrientation(Vector3f::Normalize(vessel.ReferenceTransform().position - TargetTransform(vessel).position), initialize);
            return true;
        }
        return false;
    case AutopilotMode::Maneuver:
        if (!HasManeuver(vessel)) return false;
        sas.SetTargetOrientation(ToVec3(vessel.patchedConicSolver->maneuverNodes[0]->GetBurnVector(&vessel.orbit())), initialize);
        return true;
    }
    return true;
}

// VesselAutopilot.cs:1185
bool VesselAutopilot::SetAutopilotTarget(bool initialize) {
    VesselSAS& sas = *sas_;
    Vessel& vessel = *vessel_;
    if (sas_ == nullptr) return false;
    switch (mode_) {
    case AutopilotMode::StabilityAssist:
        if (initialize) {
            if (!sas.CanEngageSAS()) return false;
            sas.ConnectFlyByWire(true);
        } else {
            sas.Update();
        }
        return true;
    case AutopilotMode::Prograde:
    case AutopilotMode::Retrograde: {
        if (vessel.targetObject == nullptr) throw NullReferenceException();
        const Vector3d rel = vessel.obt_velocity - Vector3d(vessel.targetObject->GetObtVelocity());
        sas.SetTargetOrientation(mode_ == AutopilotMode::Prograde ? ToVec3(rel) : ToVec3(-rel), initialize);
        return true;
    }
    case AutopilotMode::Normal: sas.SetTargetOrientation(ToVec3(CelestialBody::RotationAxis()), initialize); return true;
    case AutopilotMode::Antinormal: sas.SetTargetOrientation(ToVec3(-CelestialBody::RotationAxis()), initialize); return true;
    case AutopilotMode::RadialIn: sas.SetTargetOrientation(ToVec3(vessel.upAxis), initialize); return true;
    case AutopilotMode::RadialOut: sas.SetTargetOrientation(ToVec3(-vessel.upAxis), initialize); return true;
    case AutopilotMode::Target:
        if (vessel.targetObject != nullptr) {
            sas.SetTargetOrientation(Vector3f::Normalize(TargetTransform(vessel).position - vessel.ReferenceTransform().position), initialize);
            return true;
        }
        return false;
    case AutopilotMode::AntiTarget:
        if (vessel.targetObject != nullptr) {
            sas.SetTargetOrientation(Vector3f::Normalize(vessel.ReferenceTransform().position - TargetTransform(vessel).position), initialize);
            return true;
        }
        return false;
    case AutopilotMode::Maneuver:
        if (!HasManeuver(vessel)) return false;
        sas.SetTargetOrientation(ToVec3(vessel.patchedConicSolver->maneuverNodes[0]->GetBurnVector(&vessel.orbit())), initialize);
        return true;
    }
    return true;
}

// VesselAutopilot.cs:1252
bool VesselAutopilot::VectorLockInvalid(Vessel& v, float threshold) {
    if (AutopilotEnvironment::displayVelocitySqrMagnitude) return AutopilotEnvironment::displayVelocitySqrMagnitude(v) < fx(threshold);
    const AutopilotSpeedMode sm = AutopilotEnvironment::speedDisplayMode ? AutopilotEnvironment::speedDisplayMode() : AutopilotSpeedMode::Orbit;
    // FlightGlobals.GetDisplayVelocity (FlightGlobals.cs:1991) of the active vessel; the Target speed mode needs ship_tgtVelocity (hook)
    if (FlightRegistry::ActiveVessel == nullptr) throw NullReferenceException();
    const Vector3d d = sm == AutopilotSpeedMode::Surface ? FlightRegistry::ActiveVessel->srf_velocity : FlightRegistry::ActiveVessel->obt_velocity;
    return d.sqrMagnitude() < fx(threshold);
}

// VesselAutopilot.cs:1257
bool VesselAutopilot::TargetLockInvalid(Vessel& v) { return v.targetObject == nullptr; }

// VesselAutopilot.cs:1262
bool VesselAutopilot::ManeuverLockInvalid(PatchedConicSolver* pcs) {
    if (!(pcs == nullptr) && !pcs->maneuverNodes.empty()) {
        if (pcs->maneuverNodes[0]->DeltaV.sqrMagnitude() > 1.0) return pcs->maneuverNodes[0]->GetBurnVector(&vessel_->orbit()).sqrMagnitude() < 1.0;
        return false;
    }
    return true;
}

}  // namespace ksp
