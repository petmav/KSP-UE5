// WP-29a: VehiclePhysics/VehicleBase.cs (the VPP vehicle component: per-tick wheel pipeline, Update, behaviours, impacts).
// Every function cites File.cs:line of decompiled/KSP-Decompiled/Assembly-CSharp (tools/wp28/check_cites.py verifies the citations).
// Float32 follows the Mono rule (ksp/dotnet/mono_float.hpp); the Unity natives it calls are the seam in ksp/wheels/wheel_native.hpp.
#include <cmath>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/wheels/vehicle_physics.hpp"

namespace ksp::VehiclePhysics {

// VehiclePhysics/VehicleBase.cs:347
Vector3f VehicleBase::GetWheelLocalPosition(VPWheelCollider& wheelCol) {
    return transform->InverseTransformPoint(wheelCol.cachedTransform->TransformPoint(wheelCol.center));
}

// VehiclePhysics/VehicleBase.cs:358
void VehicleBase::SingleStep() {
    if (paused() && !m_singleFixedStep && !m_singleUpdateStep) {
        m_singleFixedStep = true;
        m_singleUpdateStep = true;
    }
}

// VehiclePhysics/VehicleBase.cs:367
void VehicleBase::Reposition(const Vector3f& position, const Quaternionf& rotation) {
    transform->position = position;
    transform->rotation = rotation;
    NotifyReposition();
}

// VehiclePhysics/VehicleBase.cs:374
void VehicleBase::SetWheelRadius(int wheelIndex, float radius) {
    if (wheelIndex >= 0 && wheelIndex < static_cast<int>(m_wheels.size())) {
        radius = Mathf::Max(radius, 0.01f);
        m_wheels[static_cast<size_t>(wheelIndex)]->radius = radius;
        m_wheelState[static_cast<size_t>(wheelIndex)]->wheelCol->radius = radius;
    }
}

// VehiclePhysics/VehicleBase.cs:384
float VehicleBase::GetWheelRadius(int wheelIndex) {
    if (wheelIndex >= 0 && wheelIndex < static_cast<int>(m_wheels.size())) return m_wheels[static_cast<size_t>(wheelIndex)]->radius;
    return 0.0f;
}

// VehiclePhysics/VehicleBase.cs:393
void VehicleBase::SetWheelTireFriction(int wheelIndex, std::shared_ptr<TireFriction> friction) {
    if (wheelIndex >= 0 && wheelIndex < static_cast<int>(m_wheels.size()) && friction != nullptr) m_wheels[static_cast<size_t>(wheelIndex)]->tireFriction = std::move(friction);
}

// VehiclePhysics/VehicleBase.cs:401
void VehicleBase::SetWheelTireFrictionMultiplier(int wheelIndex, float frictionMultiplier) {
    if (wheelIndex >= 0 && wheelIndex < static_cast<int>(m_wheels.size()))
        m_wheels[static_cast<size_t>(wheelIndex)]->tireFriction->frictionMultiplier = Mathf::Max(0.0f, frictionMultiplier);
}

// VehiclePhysics/VehicleBase.cs:409
std::shared_ptr<TireFriction> VehicleBase::GetWheelTireFriction(int wheelIndex) {
    if (wheelIndex >= 0 && wheelIndex < static_cast<int>(m_wheels.size())) return m_wheels[static_cast<size_t>(wheelIndex)]->tireFriction;
    return nullptr;
}

// VehiclePhysics/VehicleBase.cs:418
float VehicleBase::GetWheelAngularVelocityForSlip(int wheelIndex, float slip) {
    if (wheelIndex >= 0 && wheelIndex < static_cast<int>(m_wheels.size())) {
        WheelState& ws = *m_wheelState[static_cast<size_t>(wheelIndex)];
        Wheel& wheel = *m_wheels[static_cast<size_t>(wheelIndex)];
        if (!ws.grounded) return 0.0f;
        return f32store((fx(ws.localWheelVelocity.y) + fx(slip)) / fx(wheel.radius));
    }
    return 0.0f;
}

// VehiclePhysics/VehicleBase.cs:433
Vector2f VehicleBase::GetWheelPeakSlip(int wheelIndex) {
    if (wheelIndex >= 0 && wheelIndex < static_cast<int>(m_wheels.size())) {
        Wheel& wheel = *m_wheels[static_cast<size_t>(wheelIndex)];
        return wheel.tireFriction->GetPeakSlipBounds(wheel.contactPatch());
    }
    return Vector2f::zero();
}

// VehiclePhysics/VehicleBase.cs:443
Vector2f VehicleBase::GetWheelAdherentSlip(int wheelIndex) {
    if (wheelIndex >= 0 && wheelIndex < static_cast<int>(m_wheels.size())) {
        Wheel& wheel = *m_wheels[static_cast<size_t>(wheelIndex)];
        return wheel.tireFriction->GetAdherentSlipBounds(wheel.contactPatch());
    }
    return Vector2f::zero();
}

// VehiclePhysics/VehicleBase.cs:453
void VehicleBase::AddWheelBrakeTorque(int wheelIndex, float torque) {
    if (wheelIndex >= 0 && wheelIndex < static_cast<int>(m_wheels.size())) m_wheels[static_cast<size_t>(wheelIndex)]->AddBrakeTorque(torque);
}

// VehiclePhysics/VehicleBase.cs:461
VehicleBase::VehicleStateVars VehicleBase::GetVehicleStateVars() {
    VehicleStateVars result;
    result.time = m_time;
    result.lastVelocity = m_lastVelocity;
    result.lastImpactTime = m_lastImpactTime;
    return result;
}

// VehiclePhysics/VehicleBase.cs:470
void VehicleBase::SetVehicleStateVars(const VehicleStateVars& stateVars) {
    m_time = stateVars.time;
    m_lastVelocity = stateVars.lastVelocity;
    m_lastImpactTime = stateVars.lastImpactTime;
}

// VehiclePhysics/VehicleBase.cs:477
std::vector<VehicleBase::WheelStateVars> VehicleBase::GetWheelStateVars() {
    std::vector<WheelStateVars> array(m_wheels.size());
    for (size_t i = 0; i < m_wheels.size(); i++) {
        array[i].L = m_wheels[i]->L;
        array[i].Tr = m_wheels[i]->Tr;
        array[i].contactDepth = m_wheelState[i]->contactDepth;
        array[i].lastTireForce = m_wheelState[i]->lastTireForce;
    }
    return array;
}

// VehiclePhysics/VehicleBase.cs:493
void VehicleBase::SetWheelStateVars(const std::vector<WheelStateVars>& stateVars) {
    if (stateVars.size() == m_wheels.size()) {
        for (size_t i = 0; i < m_wheels.size(); i++) {
            m_wheels[i]->L = stateVars[i].L;
            m_wheels[i]->Tr = stateVars[i].Tr;
            m_wheelState[i]->reactionTorque = stateVars[i].Tr;
            m_wheelState[i]->contactDepth = stateVars[i].contactDepth;
            m_wheelState[i]->lastTireForce = stateVars[i].lastTireForce;
        }
    }
}

// VehiclePhysics/VehicleBase.cs:584
void VehicleBase::ComputeVehicleSpeed() {
    Vector3f velocity = rigidbody->velocity;
    Vector3f forward = transform->forward();
    m_speed = Vector3f::Dot(velocity, forward);
    velocity.y = 0.0f;
    forward.y = 0.0f;
    m_speedAngle = f32store(fx(Vector3f::Angle(velocity, forward)) * fx(Mathf::Sign(Vector3f::Dot(velocity, transform->right()))));
}

// VehiclePhysics/VehicleBase.cs:594
void VehicleBase::ComputeLocalAcceleration() {
    const Vector3f velocity = rigidbody->velocity;
    const Vector3f direction = (velocity - m_lastVelocity) / Time::deltaTime;
    m_localAcceleration = transform->InverseTransformDirection(direction);
    m_lastVelocity = velocity;
}

// VehiclePhysics/VehicleBase.cs:602
void VehicleBase::ConfigureCenterOfMass() {
    if (centerOfMass != nullptr) {
        const Vector3f vector = transform->InverseTransformPoint(centerOfMass->position);
        if ((rigidbody->centerOfMass - vector).sqrMagnitude() > 1E-07f) rigidbody->centerOfMass = vector;
    }
}

// VehiclePhysics/VehicleBase.cs:614
float VehicleBase::ComputeCombinedSlip(const Vector2f& localVelocity, const Vector2f& tireSlip) {
    const float magnitude = localVelocity.magnitude();
    if (magnitude > 0.01f) {
        const float num = f32store(fx(tireSlip.x) * fx(localVelocity.x) / fx(magnitude));
        const float y = tireSlip.y;
        return Mathf::Sqrt(f32store(fx(num) * fx(num) + fx(y) * fx(y)));
    }
    return tireSlip.magnitude();
}

// VehiclePhysics/VehicleBase.cs:626
void VehicleBase::SetNumberOfWheels(int numberOfWheels) {
    m_wheels.assign(static_cast<size_t>(numberOfWheels), nullptr);
    m_wheelState.assign(static_cast<size_t>(numberOfWheels), nullptr);
    for (size_t i = 0; i < static_cast<size_t>(numberOfWheels); i++) {
        m_wheels[i] = std::make_shared<Wheel>();
        m_wheelState[i] = std::make_shared<WheelState>();
        m_wheelState[i]->lastGroundHit.physicMaterial = std::make_shared<PhysicMaterial>();
    }
}

// VehiclePhysics/VehicleBase.cs:638
void VehicleBase::OnInitialize() { SetNumberOfWheels(4); }
// VehiclePhysics/VehicleBase.cs:643
void VehicleBase::OnFinalize() {}
// VehiclePhysics/VehicleBase.cs:647
void VehicleBase::DoUpdateBlocks() {}
// VehiclePhysics/VehicleBase.cs:651
void VehicleBase::DoUpdateData() {}
// VehiclePhysics/VehicleBase.cs:655
void VehicleBase::OnUpdate() {}

// VehiclePhysics/VehicleBase.cs:664
int VehicleBase::GetWheelIndex(int axle, WheelPos position) {
    int num = static_cast<int>(position);
    if (num < 0) {
        num = 0;
    } else if (num >= 1) {
        num = 0;
    }
    const int num2 = axle + num;
    if (num2 >= 0 && num2 < static_cast<int>(m_wheels.size())) return num2;
    return -1;
}

// VehiclePhysics/VehicleBase.cs:683
int VehicleBase::GetAxleCount() { return static_cast<int>(m_wheels.size()) / 1; }

// VehiclePhysics/VehicleBase.cs:688
int VehicleBase::GetWheelsInAxle(int axle) {
    if (axle >= 0 && axle < GetAxleCount()) return 1;
    return 0;
}

// VehiclePhysics/VehicleBase.cs:697
void VehicleBase::CalculateSuspensionTravel(WheelState& wheel) {
    VPWheelCollider* wheelCol = wheel.wheelCol;
    wheel.grounded = wheelCol->GetGroundHit(wheel.hit);
    wheel.lastContactDepth = wheel.contactDepth;
    if (wheel.grounded) {
        wheel.contactDepth = wheelCol->GetContactDepth(wheel.hit.point, wheelCol->lastRuntimeSuspensionTravel());
        wheel.suspensionCompression = f32store(fx(wheel.contactDepth) / fx(wheelCol->lastRuntimeSuspensionTravel()));
    } else {
        wheel.contactDepth = 0.0f;
        wheel.suspensionCompression = 0.0f;
        wheel.hit.force = 0.0f;
    }
}

// VehiclePhysics/VehicleBase.cs:715
void VehicleBase::CalculateSuspensionForces(WheelState& wheel) {
    VPWheelCollider* wheelCol = wheel.wheelCol;
    if (wheel.grounded) {
        wheel.contactDepth = f32store(fx(wheel.contactDepth) + (fx(wheelCol->runtimeSuspensionTravel) - fx(wheelCol->lastRuntimeSuspensionTravel())));
        wheel.suspensionCompression = f32store(fx(wheel.contactDepth) / fx(wheelCol->runtimeSuspensionTravel));
        wheel.contactSpeed = f32store((fx(wheel.contactDepth) - fx(wheel.lastContactDepth)) / fx(Time::deltaTime));
        if (advancedSuspensionDamper) {
            wheel.damperForce = f32store(fx(wheel.contactSpeed) * fx(wheelCol->runtimeDamperRate) * fx(scaleFactor));
            // `vehicle` is the STATIC VehicleBase.vehicle (set only inside the onImpact / onRawCollision callbacks): null here, so the C#
            // dereferences null and throws (the advanced damper is dead code in KSP, which never sets advancedSuspensionDamper).
            if (vehicle == nullptr) throw NullReferenceException();
            const float max = f32store(fx(suspensionDamperLimitFactor) * fx(rigidbody->mass) * fx(vehicle->gravity.Magnitude()));
            wheel.damperForce = Mathf::Clamp(wheel.damperForce, f32store(0.0 - fx(wheel.hit.force)), max);
            wheel.hit.force = f32store(fx(wheel.hit.force) + fx(wheel.damperForce));
        } else {
            wheel.damperForce = f32store(fx(wheel.hit.force) - fx(wheel.contactDepth) * fx(wheelCol->runtimeSpringRate) * fx(scaleFactor));
        }
    } else {
        wheel.contactSpeed = 0.0f;
        wheel.damperForce = 0.0f;
    }
    if (accurateSuspensionForces) wheelCol->SetSuspensionForceOffset(wheel.contactDepth);
}

// VehiclePhysics/VehicleBase.cs:746
void VehicleBase::CalculateDownforce(WheelState& wheel, float referenceDownforce) {
    if (wheel.grounded) {
        wheel.downforce = f32store(fx(wheel.hit.force) + fx(wheel.wheelCol->runtimeExtraDownforce));
        if (contactAngleAffectsTireForce) {
            const Vector3f up = wheel.wheelCol->cachedTransform->up();
            const float y = Vector3f::Dot(up, wheel.hit.sidewaysDir);
            const float x = Vector3f::Dot(up, wheel.hit.normal);
            wheel.contactAngle = Mathf::Atan2(y, x);
            wheel.downforce = f32store(fx(wheel.downforce) * fx(Mathf::Cos(wheel.contactAngle)));
        }
        if (wheel.downforce < minimumdownForce) wheel.downforce = minimumdownForce;
        wheel.normalizedLoad = f32store(fx(wheel.hit.force) / fx(referenceDownforce));
    } else {
        wheel.contactAngle = 0.0f;
        wheel.downforce = 0.0f;
        wheel.normalizedLoad = 0.0f;
    }
}

// VehiclePhysics/VehicleBase.cs:773
void VehicleBase::CalculateLocalFrame(WheelState& wheel) {
    if (!wheel.grounded) {
        wheel.wheelVelocity = Vector3f::zero();
        wheel.localWheelVelocity = Vector2f::zero();
        wheel.surfaceForce = Vector3f::zero();
        wheel.localSurfaceForce = Vector2f::zero();
        wheel.externalTireForce = Vector2f::zero();
        return;
    }
    wheel.wheelVelocity = wheel.wheelCol->GetTangentVelocity(wheel.hit.point, wheel.hit.normal, wheel.hit.collider->attachedRigidbody);
    wheel.localWheelVelocity.y = Vector3f::Dot(wheel.hit.forwardDir, wheel.wheelVelocity);
    wheel.localWheelVelocity.x = Vector3f::Dot(wheel.hit.sidewaysDir, wheel.wheelVelocity);
    const float num = Mathf::InverseLerp(1.0f, 0.25f, wheel.wheelVelocity.sqrMagnitude());
    if (num > 0.0f) {
        const float num2 = Vector3f::Dot(gravity.Up(), wheel.hit.normal);
        if (num2 > 1E-06f) {
            const Vector3f vector = gravity.Up() * wheel.hit.force / num2;
            wheel.surfaceForce = vector - Vector3f::Project(vector, wheel.hit.normal);
        } else {
            wheel.surfaceForce = gravity.Up() * 100000.0f;
        }
        wheel.localSurfaceForce.y = Vector3f::Dot(wheel.hit.forwardDir, wheel.surfaceForce);
        wheel.localSurfaceForce.x = Vector3f::Dot(wheel.hit.sidewaysDir, wheel.surfaceForce);
        wheel.localSurfaceForce = wheel.localSurfaceForce * num;
    } else {
        wheel.surfaceForce = Vector3f::zero();
        wheel.localSurfaceForce = Vector2f::zero();
    }
    float num3 = f32store((fx(wheel.hit.force) + fx(wheel.wheelCol->runtimeExtraDownforce)) * (1.0 / fx(gravity.Magnitude())));
    if (num3 < 0.0f) num3 = 0.0f;
    const float t = f32store((0.0 - fx(tireImpulseRatio)) * fx(num3));
    const Vector2f vector2 = (t * wheel.localWheelVelocity) / Time::deltaTime;
    wheel.externalTireForce = vector2 + wheel.localSurfaceForce;
    if (groundMaterialManager != nullptr) {
        GroundMaterialHit groundHit;
        groundHit.physicMaterial = wheel.hit.collider->sharedMaterial;
        groundHit.collider = wheel.hit.collider;
        groundHit.hitPoint = wheel.hit.point;
        groundMaterialManager->GetGroundMaterialCached(*this, groundHit, wheel.lastGroundHit, wheel.groundMaterial);
    }
}

// VehiclePhysics/VehicleBase.cs:827
void VehicleBase::ApplyTireForce(WheelState& wheel) {
    if (!wheel.grounded) {
        wheel.lastTireForce = Vector2f::zero();
        return;
    }
    const float num = wheel.groundMaterial != nullptr ? wheel.groundMaterial->drag : 0.0f;
    const Vector2f vector = f32store(0.0 - fx(wheel.hit.force) * fx(wheel.localWheelVelocity.magnitude()) * fx(num) * fx(0.001f)) * wheel.localWheelVelocity;
    if (tireSideDeflection) {
        wheel.tireForce.x = Mathf::Lerp(wheel.lastTireForce.x, wheel.tireForce.x, f32store(fx(tireSideDeflectionRate) * fx(Time::deltaTime)));
    }
    if (tireForceSmoothing) {
        wheel.tireForce.x = Mathf::Lerp(wheel.lastTireForce.x, wheel.tireForce.x, f32store(fx(tireForceSharpness) * fx(Time::deltaTime)));
        wheel.tireForce.y = Mathf::Lerp(wheel.lastTireForce.y, wheel.tireForce.y, f32store(fx(tireForceSharpness) * fx(Time::deltaTime)));
    }
    wheel.lastTireForce = wheel.tireForce;
    const Vector3f vector2 = wheel.hit.forwardDir * f32store(fx(wheel.tireForce.y) + fx(vector.y)) + wheel.hit.sidewaysDir * f32store(fx(wheel.tireForce.x) + fx(vector.x));
    const Vector3f vector3 = advancedSuspensionDamper ? (wheel.wheelCol->cachedTransform->up() * wheel.damperForce) : Vector3f::zero();
    wheel.wheelCol->ApplyForce(vector2 + vector3, wheel.hit.point, wheel.hit.collider->attachedRigidbody);
}

// Unity's `enabled` setter: OnEnable / OnDisable run on the transition (OnDisable only after an OnEnable has run)
void VehicleBase::enabled(bool value) {
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
void VehicleBase::AddComponentActivate() {
    if (m_enabled) {
        m_enableRan = true;
        OnEnable();
    }
}

// VehiclePhysics/VehicleBase.cs:851
void VehicleBase::OnEnable() {
    if (rigidbody == nullptr) {
        DebugLogError("Rigidbody not found in this GameObject nor any of its parents. Vehicle controller disabled.");
        enabled(false);
        return;
    }
    const Vector3f vector = rigidbody->centerOfMass;
    m_time = 0.0f;
    VPWheelCollider::scaleFactor = scaleFactor;
    OnInitialize();
    int num = 0;
    const int num2 = static_cast<int>(m_wheels.size());
    while (true) {
        if (num < num2) {
            WheelState& obj = *m_wheelState[static_cast<size_t>(num)];
            Wheel& wheel = *m_wheels[static_cast<size_t>(num)];
            if (!(obj.wheelCol == nullptr)) {
                if (wheel.tireFriction != nullptr) {
                    if (wheel.radius < 0.01f || !(wheel.mass >= 0.01f)) break;
                    wheel.RecalculateConstants();
                    wheel.RecalculateVars();
                    num++;
                    continue;
                }
                DebugLogError("Missing tire friction data for wheel[" + std::to_string(num) + "]");
                enabled(false);
                return;
            }
            DebugLogError("Missing WheelCollider for wheelsState[" + std::to_string(num) + "]");
            enabled(false);
            return;
        }
        if (centerOfMass != nullptr) {
            rigidbody->centerOfMass = transform->InverseTransformPoint(centerOfMass->position);
        } else {
            rigidbody->centerOfMass = vector;
        }
        rigidbody->maxAngularVelocity = 14.0f;
        m_solver = std::make_unique<Solver>();
        {
            std::vector<Wheel*> raw;
            for (auto& w : m_wheels) raw.push_back(w.get());
            if (!m_solver->Initialize(raw, true)) DebugLogWarning("Block chain initialization failed. Ensure all blocks are properly connected.\n" + m_solver->resultMessage());
        }
        m_solver->Integrate(m_time, Time::fixedDeltaTime, 1, false);
        EnableVehicleBehaviours();
        m_initialized = true;
        return;
    }
    DebugLogError("Invalid wheel properties (radius and/or mass) for wheel[" + std::to_string(num) + "]");
    enabled(false);
}

// VehiclePhysics/VehicleBase.cs:922
void VehicleBase::OnDisable() {
    if (!(rigidbody == nullptr)) {
        ClearCollisionData();
        m_initialized = false;
        DisableVehicleBehaviours();
        OnFinalize();
        m_solver.reset();
    }
}

// VehiclePhysics/VehicleBase.cs:934
void VehicleBase::FixedUpdate() {
    const float magnitude = rigidbody->velocity.magnitude();
    if (magnitude > 1.0f) {
        rigidbody->maxDepenetrationVelocity = 1.0f;
    } else {
        rigidbody->maxDepenetrationVelocity = Mathf::Lerp(8.0f, 1.0f, magnitude);
    }
    if (paused()) {
        if (!m_singleFixedStep) return;
        m_singleFixedStep = false;
    }
    if (onPreDynamicsStep) onPreDynamicsStep();
    VPWheelCollider::scaleFactor = scaleFactor;
    const float num = f32store(1.0 / fx(scaleFactor));
    ComputeVehicleSpeed();
    ComputeLocalAcceleration();
    for (auto& wheel : m_wheelState) CalculateSuspensionTravel(*wheel);
    UpdateSuspensionBehaviours();
    const float referenceDownforce = f32store(fx(rigidbody->mass) * fx(gravity.Magnitude()) / static_cast<double>(static_cast<float>(m_wheelState.size())));
    for (auto& wheel2 : m_wheelState) {
        CalculateSuspensionForces(*wheel2);
        CalculateDownforce(*wheel2, referenceDownforce);
        CalculateLocalFrame(*wheel2);
    }
    for (size_t j = 0; j < m_wheelState.size(); j++) {
        Wheel& wheel3 = *m_wheels[j];
        WheelState& ws = *m_wheelState[j];
        wheel3.load = f32store(fx(ws.downforce) * fx(num));
        wheel3.grip = ws.groundMaterial != nullptr ? ws.groundMaterial->grip : 1.0f;
        wheel3.V = ws.localWheelVelocity;
        wheel3.Fext = ws.externalTireForce * num;
        const float num3 = Mathf::Max(ws.brakeTorque, f32store(fx(ws.downforce) * fx(num) * fx(wheel3.radius)));
        wheel3.Tr = Mathf::Clamp(ws.reactionTorque, f32store(0.0 - fx(num3)), num3);
    }
    if (onBeforeUpdateBlocks) onBeforeUpdateBlocks();
    DoUpdateBlocks();
    if (onBeforeIntegrationStep) onBeforeIntegrationStep();
    m_solver->Integrate(m_time, Time::deltaTime, integrationSteps, integrationUseRK4);
    int num4 = 0;
    for (size_t k = 0; k < m_wheelState.size(); k++) {
        if (m_wheels[k]->isResting) num4++;
    }
    const int length = static_cast<int>(m_wheelState.size());
    const bool flag = (vehicleSleepCriteria == VehicleSleepCriteria::Strict) ? (num4 >= length / 2) : (num4 > length / 2);
    for (size_t l = 0; l < m_wheelState.size(); l++) {
        Wheel& wheel4 = *m_wheels[l];
        WheelState& ws2 = *m_wheelState[l];
        ws2.angularVelocity = wheel4.angularVelocity();
        ws2.tireForce = wheel4.F * scaleFactor;
        ws2.reactionTorque = wheel4.Tr;
        ws2.tireSlip.x = ws2.localWheelVelocity.x;
        ws2.tireSlip.y = f32store(fx(ws2.localWheelVelocity.y) - fx(ws2.angularVelocity) * fx(wheel4.radius));
        ws2.combinedTireSlip = ComputeCombinedSlip(ws2.localWheelVelocity, ws2.tireSlip);
        ws2.driveTorque = wheel4.driveTorque();
        ws2.brakeTorque = wheel4.brakeTorque();
        ApplyTireForce(ws2);
        const bool flag2 = ws2.localWheelVelocity.magnitude() < wheelSleepVelocity;
        ws2.wheelCol->canSleep = !inhibitWheelSleep && (flag ? flag2 : (flag2 && wheel4.isResting));
    }
    inhibitWheelSleep = false;
    DoUpdateData();
    if (!disableContactProcessing) ProcessImpacts();
    ConfigureCenterOfMass();
    FixedUpdateVehicleBehaviours();
    m_time = f32store(fx(m_time) + fx(Time::deltaTime));
}

// VehiclePhysics/VehicleBase.cs:1035
void VehicleBase::Update() {
    if (paused()) {
        if (m_singleFixedStep || !m_singleUpdateStep) return;
        m_singleUpdateStep = false;
    }
    if (onPreVisualUpdate) onPreVisualUpdate();
    for (int i = 0; i < wheelCount(); i++) {
        WheelState& ws = *m_wheelState[static_cast<size_t>(i)];
        ws.wheelCol->steerAngle(ws.steerAngle);
        ws.wheelCol->angularVelocity = invertVisualWheelSpinDirection ? f32store(0.0 - fx(ws.angularVelocity)) : ws.angularVelocity;
        ws.wheelCol->UpdateVisualWheel(paused() ? Time::fixedDeltaTime : Time::deltaTime);
    }
    invertVisualWheelSpinDirection = false;
    if (!disableContactProcessing) UpdateDragState(Vector3f::zero(), Vector3f::zero(), m_localDragHardness);
    OnUpdate();
    UpdateVehicleBehaviours();
}

// VehiclePhysics/VehicleBase.cs:1075
void VehicleBase::EnableVehicleBehaviours() {
    for (VehicleBehaviour* vehicleBehaviour : childBehaviours) {
        if (vehicleBehaviour->enabled() && vehicleBehaviour->activeInHierarchy) {
            AddVehicleBehaviour(vehicleBehaviour);
        } else {
            RemoveVehicleBehaviour(vehicleBehaviour);
        }
    }
}

// VehiclePhysics/VehicleBase.cs:1091
void VehicleBase::DisableVehicleBehaviours() {
    while (!m_activeBehaviours.empty()) RemoveVehicleBehaviour(m_activeBehaviours[0]);
}

// VehiclePhysics/VehicleBase.cs:1099
void VehicleBase::FixedUpdateVehicleBehaviours() {
    for (size_t i = 0; i < m_activeBehaviours.size(); i++) m_activeBehaviours[i]->FixedUpdateVehicle();
}

// VehiclePhysics/VehicleBase.cs:1107
void VehicleBase::UpdateVehicleBehaviours() {
    for (size_t i = 0; i < m_activeBehaviours.size(); i++) m_activeBehaviours[i]->UpdateVehicle();
}

// VehiclePhysics/VehicleBase.cs:1115
void VehicleBase::UpdateSuspensionBehaviours() {
    for (size_t i = 0; i < m_activeBehaviours.size(); i++) m_activeBehaviours[i]->UpdateVehicleSuspension();
}

// VehiclePhysics/VehicleBase.cs:1123
void VehicleBase::AddVehicleBehaviour(VehicleBehaviour* vb) {
    bool contains = false;
    for (VehicleBehaviour* b : m_activeBehaviours)
        if (b == vb) contains = true;
    if (contains || vb->vehicle() == nullptr) return;
    vb->OnEnableVehicle();
    if (vb->enabled() && vb->activeInHierarchy) {
        size_t i = 0;
        while (i < m_activeBehaviours.size() && vb->GetUpdateOrder() >= m_activeBehaviours[i]->GetUpdateOrder()) i++;
        m_activeBehaviours.insert(m_activeBehaviours.begin() + static_cast<std::ptrdiff_t>(i), vb);
        if (paused()) vb->OnEnterPause();
    }
}

// VehiclePhysics/VehicleBase.cs:1144
void VehicleBase::RemoveVehicleBehaviour(VehicleBehaviour* vb) {
    for (size_t i = 0; i < m_activeBehaviours.size(); i++) {
        if (m_activeBehaviours[i] == vb) {
            if (paused()) vb->OnLeavePause();
            vb->OnDisableVehicle();
            m_activeBehaviours.erase(m_activeBehaviours.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }
}

// VehiclePhysics/VehicleBase.cs:1157
void VehicleBase::RegisterVehicleBehaviour(VehicleBehaviour* vb) {
    if (initialized()) AddVehicleBehaviour(vb);
}

// VehiclePhysics/VehicleBase.cs:1165
void VehicleBase::UnregisterVehicleBehaviour(VehicleBehaviour* vb) {
    if (initialized()) RemoveVehicleBehaviour(vb);
}

// VehiclePhysics/VehicleBase.cs:1173
void VehicleBase::NotifyReposition() {
    if (initialized()) {
        for (size_t i = 0; i < m_activeBehaviours.size(); i++) m_activeBehaviours[i]->OnReposition();
    }
}

// VehiclePhysics/VehicleBase.cs:1184
void VehicleBase::NotifyEnterPause() {
    if (initialized()) {
        for (size_t i = 0; i < m_activeBehaviours.size(); i++) m_activeBehaviours[i]->OnEnterPause();
    }
}

// VehiclePhysics/VehicleBase.cs:1195
void VehicleBase::NotifyLeavePause() {
    if (initialized()) {
        for (size_t i = 0; i < m_activeBehaviours.size(); i++) m_activeBehaviours[i]->OnLeavePause();
    }
}

// VehiclePhysics/VehicleBase.cs:306
void VehicleBase::paused(bool value) {
    if (m_paused != value) {
        m_singleFixedStep = false;
        m_singleUpdateStep = false;
        m_paused = value;
        if (m_paused) {
            NotifyEnterPause();
        } else {
            NotifyLeavePause();
        }
    }
}

// VehiclePhysics/VehicleBase.cs:1206
void VehicleBase::DebugLogWarning(const std::string& message) { log.push_back("W " + message); }
// VehiclePhysics/VehicleBase.cs:1211
void VehicleBase::DebugLogError(const std::string& message) { log.push_back("E " + message); }

// VehiclePhysics/VehicleBase.cs:1216
void VehicleBase::ClearCollisionData() {
    m_sumImpactCount = 0;
    m_sumImpactPosition = Vector3f::zero();
    m_sumImpactVelocity = Vector3f::zero();
    m_sumImpactHardness = 0;
    m_lastImpactTime = 0.0f;
    m_localDragPosition = Vector3f::zero();
    m_localDragVelocity = Vector3f::zero();
    m_localDragHardness = 0;
    m_lastImpactedMaterial.physicMaterial = nullptr;
    m_impactedGroundMaterial = nullptr;
}

// VehiclePhysics/VehicleBase.cs:1230
void VehicleBase::ProcessImpacts() {
    if (m_time - m_lastImpactTime >= impactInterval && m_sumImpactCount > 0) {
        const float num = f32store(1.0 / static_cast<double>(static_cast<float>(m_sumImpactCount)));
        m_sumImpactPosition *= num;
        m_sumImpactVelocity *= num;
        if (onImpact) {
            vehicle = this;
            onImpact();
            vehicle = nullptr;
        }
        m_sumImpactCount = 0;
        m_sumImpactPosition = Vector3f::zero();
        m_sumImpactVelocity = Vector3f::zero();
        m_sumImpactHardness = 0;
        m_lastImpactTime = f32store(fx(m_time) + fx(impactInterval) * fx(UnityRandom::Global().Range(f32store(0.0 - fx(impactIntervalRandom)), impactIntervalRandom)));
    }
}

// VehiclePhysics/VehicleBase.cs:1255
void VehicleBase::OnCollisionEnter(const Collision& collision) { OnCollision(collision, true); }

// VehiclePhysics/VehicleBase.cs:1260
void VehicleBase::OnCollisionStay(const Collision& collision) { OnCollision(collision, false); }

// VehiclePhysics/VehicleBase.cs:1265
void VehicleBase::OnCollision(const Collision& collision, bool isCollisionEnter_) {
    if (!initialized() || disableContactProcessing) return;
    int num = 0;
    Vector3f zero = Vector3f::zero();
    Vector3f zero2 = Vector3f::zero();
    int num2 = 0;
    int num3 = 0;
    Vector3f zero3 = Vector3f::zero();
    Vector3f zero4 = Vector3f::zero();
    int num4 = 0;
    const float num5 = f32store(fx(impactMinSpeed) * fx(impactMinSpeed));
    for (const ContactPoint& contactPoint : collision.contacts) {
        m_lastContactedCollider = contactPoint.otherCollider;
        int num6 = 0;
        if (groundMaterialManager != nullptr) {
            GroundMaterialHit groundHit;
            groundHit.physicMaterial = contactPoint.otherCollider->sharedMaterial;
            groundHit.collider = contactPoint.otherCollider;
            groundHit.hitPoint = contactPoint.point;
            groundMaterialManager->GetGroundMaterialCached(*this, groundHit, m_lastImpactedMaterial, m_impactedGroundMaterial);
            if (m_impactedGroundMaterial != nullptr) num6 = (m_impactedGroundMaterial->surfaceType == GroundMaterial::SurfaceType::Hard) ? 1 : (-1);
        }
        Vector3f pointVelocity = contactPoint.thisRigidbody->GetPointVelocity(contactPoint.point);
        if (contactPoint.otherCollider->attachedRigidbody != nullptr) pointVelocity -= contactPoint.otherCollider->attachedRigidbody->GetPointVelocity(contactPoint.point);
        const float num7 = Vector3f::Dot(pointVelocity, contactPoint.normal);
        if (!(num7 < 0.0 - fx(impactThreeshold)) && (!isCollisionEnter_ || collision.relativeVelocity.sqrMagnitude() <= num5)) {
            if (num7 < impactThreeshold) {
                num3++;
                zero3 += contactPoint.point;
                zero4 += pointVelocity;
                num4 += num6;
            }
        } else {
            num++;
            zero += contactPoint.point;
            zero2 += collision.relativeVelocity;
            num2 += num6;
        }
    }
    if (num > 0) {
        const float num8 = f32store(1.0 / static_cast<double>(static_cast<float>(num)));
        zero *= num8;
        zero2 *= num8;
        m_sumImpactCount++;
        m_sumImpactPosition += transform->InverseTransformPoint(zero);
        m_sumImpactVelocity += transform->InverseTransformDirection(zero2);
        m_sumImpactHardness += num2;
    }
    if (num3 > 0) {
        const float num9 = f32store(1.0 / static_cast<double>(static_cast<float>(num3)));
        zero3 *= num9;
        zero4 *= num9;
        UpdateDragState(transform->InverseTransformPoint(zero3), transform->InverseTransformDirection(zero4), num4);
    }
    if (onRawCollision) {
        vehicle = this;
        currentCollision = &collision;
        isCollisionEnter = isCollisionEnter_;
        onRawCollision();
        currentCollision = nullptr;
        vehicle = nullptr;
    }
}

// VehiclePhysics/VehicleBase.cs:1363
void VehicleBase::UpdateDragState(const Vector3f& dragPosition, const Vector3f& dragVelocity, int dragHardness) {
    if (dragVelocity.sqrMagnitude() > 0.0001f) {
        m_localDragPosition = Vector3f::Lerp(m_localDragPosition, dragPosition, f32store(10.0 * fx(Time::deltaTime)));
        m_localDragVelocity = Vector3f::Lerp(m_localDragVelocity, dragVelocity, f32store(20.0 * fx(Time::deltaTime)));
        m_localDragHardness = dragHardness;
    } else {
        m_localDragVelocity = Vector3f::Lerp(m_localDragVelocity, Vector3f::zero(), f32store(10.0 * fx(Time::deltaTime)));
    }
}

}  // namespace ksp::VehiclePhysics
