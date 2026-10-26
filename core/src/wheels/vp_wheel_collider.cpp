// WP-29a: VehiclePhysics/VPWheelCollider.cs, VehicleBehaviour.cs, GroundMaterialManagerBase.cs, DataBus.cs and KSPWheelGravity.cs.
// Every function cites File.cs:line of decompiled/KSP-Decompiled/Assembly-CSharp (tools/wp28/check_cites.py verifies the citations).
// The renderer-only parts of VPWheelCollider (mesh / caliper / suspension transform placement, AdjustToWheelMesh, OnValidate) are not ported
// (see the header comment of ksp/wheels/vehicle_physics.hpp).
#include <cmath>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/wheels/vehicle_physics.hpp"

namespace ksp::VehiclePhysics {

// ---- DataBus ---------------------------------------------------------------------------------------------------------------------------
// VehiclePhysics/DataBus.cs:9
DataBus::DataBus() {
    m_data.resize(3);
    m_data[0].assign(10, 0);
    m_data[1].assign(22, 0);
    m_data[2].assign(10, 0);
}

// ---- GroundMaterialManagerBase ---------------------------------------------------------------------------------------------------------
// VehiclePhysics/GroundMaterialManagerBase.cs:9
void GroundMaterialManagerBase::GetGroundMaterialCached(VehicleBase& vehicle, const GroundMaterialHit& groundHit, GroundMaterialHit& cachedGroundHit,
                                                        GroundMaterial*& groundMaterial) {
    if (groundHit.physicMaterial != cachedGroundHit.physicMaterial) {
        cachedGroundHit = groundHit;
        groundMaterial = GetGroundMaterial(vehicle, groundHit);
    }
}

// ---- KSPWheelGravity -------------------------------------------------------------------------------------------------------------------
// KSPWheelGravity.cs:13
void KSPWheelGravity::Value(const Vector3f& value) {
    if (gravityValue != value) {
        gravityValue = value;
        Refresh();
    }
}

// KSPWheelGravity.cs:29
float KSPWheelGravity::Magnitude() {
    if (!gravityCached) Refresh();
    return gravityMagnitude;
}

// KSPWheelGravity.cs:41
Vector3f KSPWheelGravity::Up() {
    if (!gravityCached) Refresh();
    return gravityUp;
}

// KSPWheelGravity.cs:53
void KSPWheelGravity::Refresh() {
    gravityMagnitude = gravityValue.magnitude();
    gravityUp = (-gravityValue).normalized();
    gravityCached = true;
}

// ---- VehicleBehaviour ------------------------------------------------------------------------------------------------------------------
// VehiclePhysics/VehicleBehaviour.cs:9
void VehicleBehaviour::OnEnableVehicle() {}
// VehiclePhysics/VehicleBehaviour.cs:13
void VehicleBehaviour::OnDisableVehicle() {}
// VehiclePhysics/VehicleBehaviour.cs:17
void VehicleBehaviour::UpdateVehicle() {}
// VehiclePhysics/VehicleBehaviour.cs:21
void VehicleBehaviour::FixedUpdateVehicle() {}
// VehiclePhysics/VehicleBehaviour.cs:25
int VehicleBehaviour::GetUpdateOrder() { return 0; }
// VehiclePhysics/VehicleBehaviour.cs:30
void VehicleBehaviour::UpdateVehicleSuspension() {}
// VehiclePhysics/VehicleBehaviour.cs:34
void VehicleBehaviour::OnEnableComponent() {}
// VehiclePhysics/VehicleBehaviour.cs:38
void VehicleBehaviour::OnDisableComponent() {}
// VehiclePhysics/VehicleBehaviour.cs:42
void VehicleBehaviour::OnReposition() {}
// VehiclePhysics/VehicleBehaviour.cs:46
void VehicleBehaviour::OnEnterPause() {}
// VehiclePhysics/VehicleBehaviour.cs:50
void VehicleBehaviour::OnLeavePause() {}

// VehiclePhysics/VehicleBehaviour.cs:54
void VehicleBehaviour::DebugLogWarning(const std::string& message) { log.push_back("W " + message); }
// VehiclePhysics/VehicleBehaviour.cs:59
void VehicleBehaviour::DebugLogError(const std::string& message) { log.push_back("E " + message); }

// Unity's `enabled` setter (OnEnable / OnDisable on the transition)
void VehicleBehaviour::SetEnabled(bool value) {
    if (m_enabled == value) return;
    m_enabled = value;
    if (value) {
        m_enableRan = true;
        OnEnable();
    } else if (m_enableRan) {
        m_enableRan = false;
        OnDisable();
    }
}

// AddComponent<T>(): Awake has run, now OnEnable if the component is still enabled
void VehicleBehaviour::AddComponentActivate() {
    if (m_enabled) {
        m_enableRan = true;
        OnEnable();
    }
}

// VehiclePhysics/VehicleBehaviour.cs:64
void VehicleBehaviour::OnEnable() {
    m_vehicle = GetVehicle();
    if (m_vehicle == nullptr) {
        DebugLogWarning("This component requires a VehicleBase-derived component. Component disabled.");
        SetEnabled(false);
        return;
    }
    OnEnableComponent();
    if (m_vehicle != nullptr) m_vehicle->RegisterVehicleBehaviour(this);
}

// VehiclePhysics/VehicleBehaviour.cs:80
VehicleBase* VehicleBehaviour::GetVehicle() { return parentVehicle; }

// VehiclePhysics/VehicleBehaviour.cs:85
void VehicleBehaviour::OnDisable() {
    if (m_vehicle != nullptr) {
        m_vehicle->UnregisterVehicleBehaviour(this);
        OnDisableComponent();
        m_vehicle = nullptr;
    }
}

// ---- VPWheelCollider -------------------------------------------------------------------------------------------------------------------
VPWheelCollider::VPWheelCollider() : m_visualSteerAngle(0.0f) {}

// VehiclePhysics/VPWheelCollider.cs:112
void VPWheelCollider::steerAngle(float value) {
    if (isActiveAndEnabled()) {
        m_wheelCollider->steerAngle = disableSteerAngleFix ? value : FixSteerAngle(value);
        m_steerAngle = value;
    }
}

// VehiclePhysics/VPWheelCollider.cs:185
void VPWheelCollider::Awake() { m_wheelCollider = SetupWheelCollider(); }

// VehiclePhysics/VPWheelCollider.cs:190
void VPWheelCollider::OnEnableVehicle() {
    m_wheelCollider->enabled = true;
    m_rigidbody = m_wheelCollider->attachedRigidbody;
    m_wheelCollider->ConfigureVehicleSubsteps(1000.0f, 1, 1);
    // WP-29e (VPWheelCollider.cs:198-204): the transform fields are whatever the wheel set-up had assigned when the component was enabled
    m_isCaliperChildOfSuspension = caliperTransform != nullptr && suspensionTransform != nullptr && visual.isChildOf && visual.isChildOf(caliperTransform, suspensionTransform);
    m_isWheelChildOfSuspension = wheelTransform != nullptr && suspensionTransform != nullptr && visual.isChildOf && visual.isChildOf(wheelTransform, suspensionTransform);
    m_isWheelChildOfCaliper = wheelTransform != nullptr && caliperTransform != nullptr && visual.isChildOf && visual.isChildOf(wheelTransform, caliperTransform);
    m_suspensionPosition = cachedTransform->position;
    if (wheelTransform != nullptr && visual.localEulerAnglesX) m_angularPosition = visual.localEulerAnglesX(wheelTransform);
    m_visualSteerAngle.Reset(m_steerAngle);
    runtimeSpringRate = springRate;
    runtimeDamperRate = damperRate;
    runtimeSuspensionTravel = suspensionDistance;
    runtimeExtraDownforce = 0.0f;
}

// VehiclePhysics/VPWheelCollider.cs:213
void VPWheelCollider::OnDisableVehicle() { m_wheelCollider->enabled = false; }

// VehiclePhysics/VPWheelCollider.cs:218
void VPWheelCollider::OnDisableComponent() { m_wheelCollider->enabled = false; }

// VehiclePhysics/VPWheelCollider.cs:223
int VPWheelCollider::GetUpdateOrder() { return 100; }

// VehiclePhysics/VPWheelCollider.cs:228
void VPWheelCollider::FixedUpdateVehicle() {
    UpdateWheelCollider(*m_wheelCollider);
    m_lastRuntimeSpringRate = runtimeSpringRate;
    m_lastRuntimeDamperRate = runtimeDamperRate;
    m_lastRuntimeSuspensionTravel = runtimeSuspensionTravel;
    runtimeSpringRate = springRate;
    runtimeDamperRate = damperRate;
    runtimeSuspensionTravel = suspensionDistance;
    runtimeExtraDownforce = 0.0f;
    m_visualSteerAngle.Set(m_steerAngle);
}

// VehiclePhysics/VPWheelCollider.cs:241
void VPWheelCollider::UpdateVisualWheel(float deltaTime) {
    if (!isActiveAndEnabled()) {
        if (hideWheelOnDisable && wheelTransform != nullptr && visual.activeInHierarchy && visual.setActive && visual.activeInHierarchy(wheelTransform))
            visual.setActive(wheelTransform, false);
        return;
    }
    if (hideWheelOnDisable && wheelTransform != nullptr && visual.activeInHierarchy && visual.setActive && !visual.activeInHierarchy(wheelTransform))
        visual.setActive(wheelTransform, true);
    const Vector3f origin = cachedTransform->TransformPoint(m_wheelCollider->center) + cachedTransform->up() * radius;
    const Vector3f vector = -cachedTransform->up();
    const float hitDistance = visualRaycast ? visualRaycast(origin, vector, f32store(fx(m_lastRuntimeSuspensionTravel) + fx(radius) * 2.0)) : -1.0f;
    m_visualGrounded = hitDistance >= 0.0f;
    if (m_visualGrounded) {
        m_contactDistance = Mathf::Clamp(f32store(fx(hitDistance) - fx(radius) * 2.0), 0.0f, m_lastRuntimeSuspensionTravel);
    } else {
        m_contactDistance = m_lastRuntimeSuspensionTravel;
    }
    m_angularPosition = f32store(std::fmod(fx(m_angularPosition) + fx(angularVelocity) * fx(deltaTime), fx(m_2PI)));
    if (!disableSuspensionMovement) {
        m_suspensionPosition = cachedTransform->position +
                               vector * f32store(fx(m_contactDistance) + fx(groundPenetration) - fx(suspensionDistance) * (1.0 - fx(suspensionAnchor)) + fx(suspensionOffset));
    } else {
        m_suspensionPosition = cachedTransform->position;
    }
    // WP-29e: the Transform writes (VPWheelCollider.cs:275-300), through the host
    VisualWriteState st;
    st.suspensionTransform = suspensionTransform;
    st.caliperTransform = caliperTransform;
    st.wheelTransform = wheelTransform;
    st.updateSuspension = updateSuspension;
    st.updateCaliper = updateCaliper;
    st.updateWheel = updateWheel;
    st.caliperChildOfSuspension = m_isCaliperChildOfSuspension;
    st.wheelChildOfSuspension = m_isWheelChildOfSuspension;
    st.wheelChildOfCaliper = m_isWheelChildOfCaliper;
    st.suspensionPosition = m_suspensionPosition;
    st.visualSteerAngle = m_visualSteerAngle.GetInterpolated(visual.frameRatio);
    st.angularPosition = m_angularPosition;
    WriteVisualTransforms(st, visual);
}

// VehiclePhysics/VPWheelCollider.cs:275. `host.setPosition` / `setLocalRotation` unset: nothing is written (a host without a model)
void VPWheelCollider::WriteVisualTransforms(const VisualWriteState& st, const VisualTransformHost& host) {
    if (!host.setPosition || !host.setLocalRotation) return;
    if (st.suspensionTransform != nullptr && st.updateSuspension) {
        host.setPosition(st.suspensionTransform, st.suspensionPosition);
    }
    if (st.caliperTransform != nullptr && st.updateCaliper) {
        if (st.suspensionTransform == nullptr || !st.caliperChildOfSuspension) {
            host.setPosition(st.caliperTransform, st.suspensionPosition);
        }
        host.setLocalRotation(st.caliperTransform, UnityNative::Euler(Vector3f(0.0f, st.visualSteerAngle, 0.0f)));
    }
    if (st.wheelTransform != nullptr && st.updateWheel) {
        if ((st.suspensionTransform == nullptr && st.caliperTransform == nullptr) || (!st.wheelChildOfSuspension && !st.wheelChildOfCaliper)) {
            host.setPosition(st.wheelTransform, st.suspensionPosition);
        }
        const float spin = f32store(fx(st.angularPosition) * fx(57.29578f));
        if (st.caliperTransform != nullptr && st.wheelChildOfCaliper) {
            host.setLocalRotation(st.wheelTransform, UnityNative::Euler(Vector3f(spin, 0.0f, 0.0f)));
        } else {
            host.setLocalRotation(st.wheelTransform, UnityNative::Euler(Vector3f(spin, st.visualSteerAngle, 0.0f)));
        }
    }
}

// VehiclePhysics/VPWheelCollider.cs:304
void VPWheelCollider::SetSuspensionForceOffset(float offset) {
    if (!(m_wheelCollider == nullptr)) {
        const float forceAppPointDistance = f32store(fx(offset) - fx(m_lastRuntimeSuspensionTravel) * fx(m_wheelCollider->suspensionSpring.targetPosition));
        m_wheelCollider->forceAppPointDistance = forceAppPointDistance;
    }
}

// VehiclePhysics/VPWheelCollider.cs:313
bool VPWheelCollider::GetGroundHit(WheelHit& hit) {
    if (!(m_wheelCollider == nullptr) && !(m_rigidbody == nullptr)) {
        bool flag = m_wheelCollider->GetGroundHit(hit);
        if (!isActiveAndEnabled()) flag = false;
        if (flag) {
            if (m_rigidbody != nullptr) hit.point += m_rigidbody->velocity * Time::fixedDeltaTime;
            if (!disableWheelReferenceFrameFix) {
                const Vector3f vector = NativeOps::AngleAxis(m_steerAngle, cachedTransform->up()) * cachedTransform->forward();
                const float num = f32store(0.0 - fx(Vector3f::Dot(vector, hit.normal)));
                const float num2 = Vector3f::Dot(cachedTransform->up(), hit.normal);
                if (num2 > 1E-06f) {
                    const Vector3f vector2 = cachedTransform->up() * f32store(fx(num) / fx(num2));
                    hit.forwardDir = (vector + vector2).normalized();
                } else {
                    hit.forwardDir = (num >= 0.0f) ? cachedTransform->up() : (-cachedTransform->up());
                }
                hit.sidewaysDir = Vector3f::Cross(hit.normal, hit.forwardDir);
            }
        }
        return flag;
    }
    hit = WheelHit();
    return false;
}

// VehiclePhysics/VPWheelCollider.cs:351
float VPWheelCollider::GetContactDepth(const Vector3f& contactPoint, float suspensionTravel) {
    if (suspensionDistance < minSuspensionDistance) suspensionDistance = minSuspensionDistance;
    const float num = f32store(0.0 - fx(cachedTransform->InverseTransformPoint(contactPoint).y));
    return f32store(fx(suspensionTravel) + fx(radius) - (fx(num) + fx(suspensionDistance) * (1.0 - fx(suspensionAnchor))));
}

// VehiclePhysics/VPWheelCollider.cs:361
Vector3f VPWheelCollider::GetTangentVelocity(const Vector3f& contactPoint, const Vector3f& surfaceNormal, NativeRigidbody* surfaceRigidbody) {
    Vector3f pointVelocity = m_rigidbody->GetPointVelocity(contactPoint);
    if (surfaceRigidbody != nullptr) pointVelocity -= surfaceRigidbody->GetPointVelocity(contactPoint);
    return pointVelocity - Vector3f::Project(pointVelocity, surfaceNormal);
}

// VehiclePhysics/VPWheelCollider.cs:371
void VPWheelCollider::ApplyForce(const Vector3f& force, const Vector3f& position, NativeRigidbody* otherRb) {
    m_rigidbody->AddForceAtPosition(force, position);
    if (otherRb != nullptr && !otherRb->isKinematic) {
        const float num = Mathf::Clamp01(f32store(fx(otherRb->mass) / fx(m_rigidbody->mass)));
        otherRb->AddForceAtPosition((-force) * num, position);
    }
}

// VehiclePhysics/VPWheelCollider.cs:391
NativeWheelCollider* VPWheelCollider::ResetWheelCollider() {
    m_wheelCollider = SetupWheelCollider();
    return m_wheelCollider;
}

// VehiclePhysics/VPWheelCollider.cs:397
NativeWheelCollider* VPWheelCollider::SetupWheelCollider() {
    // GetComponent<WheelCollider>() or AddComponent: wheelColliderData is that component
    WheelFrictionCurve wheelFrictionCurve;  // default(WheelFrictionCurve): every field 0, then stiffness = 0
    wheelFrictionCurve.stiffness = 0.0f;
    wheelColliderData.sidewaysFriction = wheelFrictionCurve;
    wheelColliderData.forwardFriction = wheelFrictionCurve;
    wheelColliderData.mass = 1.0f;
    wheelColliderData.enabled = enabled();
    return &wheelColliderData;
}

// VehiclePhysics/VPWheelCollider.cs:414
void VPWheelCollider::UpdateWheelCollider(NativeWheelCollider& wheelCol) {
    wheelCol.motorTorque = canSleep ? 0.0f : 1E-05f;
    if (suspensionDistance < minSuspensionDistance) suspensionDistance = minSuspensionDistance;
    const Vector3f b = center + Vector3f::up() * suspensionDistance * f32store(1.0 - fx(suspensionAnchor));
    if (!MathUtility::Vector3Equals(wheelCol.center, b)) wheelCol.center = b;
    if (runtimeSuspensionTravel < minSuspensionDistance) runtimeSuspensionTravel = minSuspensionDistance;
    if (wheelCol.suspensionDistance != runtimeSuspensionTravel) wheelCol.suspensionDistance = runtimeSuspensionTravel;
    if (wheelCol.radius != radius) wheelCol.radius = radius;
    const float num = f32store(fx(wheelCol.sprungMass) * fx(vehicle() == nullptr ? 9.80665f : vehicle()->gravity.Magnitude()));
    const float num2 = Mathf::Max(f32store(fx(num) / fx(wheelCol.suspensionDistance)), 0.1f);
    JointSpring suspensionSpring;
    suspensionSpring.spring = Mathf::Clamp(f32store(fx(runtimeSpringRate) * fx(scaleFactor)), num2, Mathf::Infinity);
    suspensionSpring.damper = vehicle()->advancedSuspensionDamper ? 0.0f : Mathf::Max(f32store(fx(runtimeDamperRate) * fx(scaleFactor)), 0.1f);
    suspensionSpring.targetPosition = Mathf::Clamp(f32store(fx(num) / fx(suspensionSpring.spring) / fx(wheelCol.suspensionDistance)), 0.1f, 1.0f);
    if (std::isnan(suspensionSpring.targetPosition)) return;
    const JointSpring suspensionSpring2 = wheelCol.suspensionSpring;
    if (Mathf::Abs(f32store(fx(suspensionSpring.targetPosition) - fx(suspensionSpring2.targetPosition))) > springSlerpRate) {
        if (suspensionSpring2.targetPosition > suspensionSpring.targetPosition) {
            suspensionSpring.targetPosition = f32store(fx(suspensionSpring2.targetPosition) - fx(springSlerpRate));
        } else {
            suspensionSpring.targetPosition = f32store(fx(suspensionSpring2.targetPosition) + fx(springSlerpRate));
        }
    }
    if (suspensionSpring2.spring != suspensionSpring.spring || suspensionSpring2.damper != suspensionSpring.damper ||
        suspensionSpring2.targetPosition != suspensionSpring.targetPosition) {
        wheelCol.suspensionSpring = suspensionSpring;
    }
}

// VehiclePhysics/VPWheelCollider.cs:475
float VPWheelCollider::FixSteerAngle(float inputSteerAngle) {
    if (!isActiveAndEnabled()) return inputSteerAngle;
    const Vector3f vector = NativeOps::AngleAxis(inputSteerAngle, cachedTransform->up()) * cachedTransform->forward();
    const Vector3f vector2 = vector - Vector3f::Project(vector, rigidbodyTransform->up());
    return f32store(fx(Vector3f::Angle(rigidbodyTransform->forward(), vector2)) * fx(Mathf::Sign(Vector3f::Dot(rigidbodyTransform->right(), vector2))));
}

}  // namespace ksp::VehiclePhysics
