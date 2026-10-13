// WP-29a: KSPWheelController.cs (the KSP-side VehicleBase: one wheel, one DirectDrive, input ramps, brake torque).
// Every function cites File.cs:line of decompiled/KSP-Decompiled/Assembly-CSharp (tools/wp28/check_cites.py verifies the citations).
// Float32 follows the Mono rule (ksp/dotnet/mono_float.hpp).
#include <limits>

#include "ksp/core/unity_time.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/wheels/vehicle_physics.hpp"

namespace ksp::VehiclePhysics {

// KSPWheelController.cs:174
std::shared_ptr<KSPWheelController> KSPWheelController::Create(NativeRigidbody* rb, NativeTransform* hostTransform, NativeTransform* wheelColliderHost) {
    // host.GetComponent<KSPWheelController>() ?? host.AddComponent<KSPWheelController>(): the host is new, so AddComponent
    auto obj = std::make_shared<KSPWheelController>();
    obj->rigidbody = rb;
    obj->transform = hostTransform;
    obj->Awake();
    obj->AddComponentActivate();
    // wheelColliderHost.GetComponent<VPWheelCollider>() ?? wheelColliderHost.gameObject.AddComponent<VPWheelCollider>()
    obj->m_ownedWheelCollider = std::make_unique<VPWheelCollider>();
    VPWheelCollider* vpw = obj->m_ownedWheelCollider.get();
    vpw->cachedTransform = wheelColliderHost;
    vpw->rigidbodyTransform = hostTransform;
    vpw->wheelColliderData.attachedRigidbody = rb;
    vpw->parentVehicle = obj.get();
    obj->childBehaviours.push_back(vpw);
    vpw->Awake();
    vpw->AddComponentActivate();
    obj->wheelCollider = vpw;
    obj->wcTransform = wheelColliderHost;
    obj->maxRpm = std::numeric_limits<float>::max();  // float.MaxValue
    obj->maxBrakeTorque = 0.0f;
    obj->maxDriveTorque = 0.0f;
    obj->maxSteerAngle = 0.0f;
    obj->steeringResponse = 0.0f;
    obj->driveResponse = 0.0f;
    obj->brakeResponse = 0.0f;
    return obj;
}

// KSPWheelController.cs:68
bool KSPWheelController::IsGrounded() const {
    if (!init) return false;
    return wheelState()[0]->grounded;
}

// KSPWheelController.cs:68
void KSPWheelController::IsGrounded(bool value) {
    if (init) wheelState()[0]->grounded = value;
}

// KSPWheelController.cs:87
Vector3f KSPWheelController::SuspensionAxis() const {
    if (!init) return Vector3f::up();
    return transform->up();
}

// KSPWheelController.cs:101
VehicleBase::WheelState* KSPWheelController::currentState() const {
    if (!init) return nullptr;
    return wheelState()[0].get();
}

// KSPWheelController.cs:113
float KSPWheelController::LegacyWheelLoad() {
    if (!currentState()->grounded) return 0.0f;
    return Mathf::Max(0.0f, f32store(fx(currentState()->contactDepth) * fx(currentState()->wheelCol->effectiveSpringRate()) * (1.0 / fx(gravity.Magnitude()))));
}

// KSPWheelController.cs:125
Vector3f KSPWheelController::WheelCenter() const {
    if (wheelCollider != nullptr && wheelCollider->cachedTransform != nullptr) {
        const Vector3f position = wheelCollider->center + Vector3f::up() * wheelCollider->suspensionDistance * f32store(1.0 - fx(wheelCollider->suspensionAnchor));
        return wheelCollider->cachedTransform->TransformPoint(position) + WheelDown() * currentState()->contactDepth;
    }
    return Vector3f::zero();
}

// KSPWheelController.cs:138
float KSPWheelController::WheelRadius() const {
    if (wheelCollider == nullptr) return 0.0f;
    return wheelCollider->radius;
}

// KSPWheelController.cs:150
Vector3f KSPWheelController::WheelDown() const {
    if (wheelCollider != nullptr && wheelCollider->cachedTransform != nullptr) return -wheelCollider->cachedTransform->up();
    return Vector3f::zero();
}

// KSPWheelController.cs:162
Vector3f KSPWheelController::WheelBase() const {
    if (wheelCollider != nullptr && wheelCollider->cachedTransform != nullptr) return WheelCenter() + WheelDown() * WheelRadius();
    return Vector3f::zero();
}

// KSPWheelController.cs:189
void KSPWheelController::SetRigidbodyTarget(NativeRigidbody* tgt) { tgtRb = tgt != nullptr ? tgt : rb(); }

// KSPWheelController.cs:194
void KSPWheelController::Awake() {
    if (HighLogic::LoadedSceneIsGame()) enabled(false);
}

// KSPWheelController.cs:202
void KSPWheelController::Initialize() {
    if (!init) OnInitialize();
}

// KSPWheelController.cs:210
void KSPWheelController::OnInitialize() {
    tgtRb = rb();
    SetNumberOfWheels(1);
    if (wheelCollider == nullptr) {
        DebugLogError("Missing WheelCollider");
        return;
    }
    wheelState()[0]->wheelCol = wheelCollider;
    wheels()[0]->tireFriction = tireFriction;
    wheels()[0]->radius = wheelCollider->radius;
    wheels()[0]->mass = wheelCollider->mass;
    vehicleSleepCriteria = VehicleSleepCriteria::Relaxed;
    wheelSleepVelocity = 0.0f;
    m_directDrive = std::make_shared<DirectDrive>();
    Block::Connect(*wheels()[0], 0, *m_directDrive, 0);
    init = true;
}

// KSPWheelController.cs:230
void KSPWheelController::OnUpdate() {
    if (keyboardSteering) {
        steerInput = (getKey && getKey(KeyA)) ? (-1.0f) : ((getKey && getKey(KeyD)) ? 1.0f : 0.0f);
    }
    steerState = Mathf::MoveTowards(steerState, steerInput, f32store(fx(steeringResponse) * fx(Time::deltaTime)));
    wheelState()[0]->steerAngle = f32store(fx(steerState) * fx(maxSteerAngle));
}

// KSPWheelController.cs:240
void KSPWheelController::DoUpdateBlocks() {
    if (keyboardDrive) {
        driveInput = (getKey && getKey(KeyS)) ? (-1.0f) : ((getKey && getKey(KeyW)) ? 1.0f : 0.0f);
    }
    if (keyboardBrakes) {
        brakeInput = (getKey && getKey(KeySpace)) ? 1.0f : 0.0f;
    }
    driveState = Mathf::MoveTowards(driveState, driveInput, f32store(fx(driveResponse) * fx(Time::deltaTime)));
    brakeState = Mathf::MoveTowards(brakeState, brakeInput, f32store(fx(brakeResponse) * fx(Time::deltaTime)));
    m_directDrive->motorInput = driveState;
    m_directDrive->maxMotorTorque = maxDriveTorque;
    m_directDrive->maxRpm = maxRpm;
    m_directDrive->damping = wheelDamping;
    if (brakeState >= 1.0f) {
        wheels()[0]->AddBrakeTorque(f32store(fx(brakeState) * fx(maxBrakeTorqueRest)));
    } else {
        wheels()[0]->AddBrakeTorque(f32store(fx(brakeState) * fx(maxBrakeTorque)));
    }
}

// KSPWheelController.cs:266
void KSPWheelController::SetWheelTransform(const Vector3f& position, const Quaternionf& rotation) {
    if (wcTransform != nullptr) {
        wcTransform->position = position;
        wcTransform->rotation = rotation;
    }
}

}  // namespace ksp::VehiclePhysics
