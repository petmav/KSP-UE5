// WP-29a: the NATIVE half of a KSP wheel, as a data seam. kspcore has no engine, so the Unity objects the VehiclePhysics (VPP) wheel code
// talks to are plain state here:
//
//   NativeRigidbody       UnityEngine.Rigidbody (position of the centre of mass, velocity, angular velocity, mass, the force calls)
//   NativeTransform       UnityEngine.Transform (position + rotation; scale is 1: KSP rescales the wheel collider host by 1 / rescaleFactor,
//                         ModuleWheelBase.cs:293-297, so the lossy scale of the host is 1 for every stock part)
//   NativeWheelCollider   UnityEngine.WheelCollider (PhysX): what VPWheelCollider writes (spring, damper, target position, centre, radius,
//                         steer angle, motor torque, force application point) and what it reads (GetGroundHit, sprungMass). The suspension
//                         ray / force law itself is WP-29c: a host (or a test) fills `groundHit` before every FixedUpdate.
//   WheelHit / Collider   UnityEngine.WheelHit / Collider (only attachedRigidbody and sharedMaterial are read by VPP)
//
// Numerics: the managed parts of Unity (Vector3 / Quaternion operators) follow the Mono float rule through ksp/unity/vector.hpp. The native
// functions below (Transform.up / forward / right, Transform.InverseTransformPoint / Direction, Rigidbody.GetPointVelocity,
// Quaternion.AngleAxis) are per-operation float32. They have no C# source; they are written from Unity's documented formulation
// (Runtime/Math/Quaternion.h RotateVectorByQuat, PhysX getVelocityAtPos) and are UNVERIFIED against the game: a capture of
// Transform.up / InverseTransformPoint and Rigidbody.GetPointVelocity would settle them (STATUS.md WP-29a).
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::VehiclePhysics {

// ---- per-operation float32 helpers for the native functions (no Mono widening) -----------------------------------------------------------
Vector3f NativeRotate(const Quaternionf& q, const Vector3f& v);  // RotateVectorByQuat
Vector3f NativeCross(const Vector3f& a, const Vector3f& b);

// Natives with a UCRT-libm dependency. The production binding is the capture-verified ksp::UnityNative::AngleAxis (sinf / cosf of the
// Windows UCRT); an independent implementation cannot reproduce the UCRT single-precision sinf / cosf bit for bit, so the wheel gate
// (tests/test_wheels.cpp) rebinds this hook to the reference's formulation (cosf / sinf of the half angle rounded from the correctly
// rounded double) and a separate case bounds the difference between the two bindings.
struct NativeOps {
    static inline std::function<Quaternionf(float angleDegrees, const Vector3f& axis)> AngleAxis = [](float angle, const Vector3f& axis) {
        return UnityNative::AngleAxis(angle, axis);
    };
};

struct NativeTransform {
    Vector3f position = Vector3f::zero();
    Quaternionf rotation = Quaternionf::identity();

    Vector3f up() const;       // Transform.up
    Vector3f forward() const;  // Transform.forward
    Vector3f right() const;    // Transform.right
    Vector3f TransformPoint(const Vector3f& p) const;
    Vector3f InverseTransformPoint(const Vector3f& p) const;
    Vector3f InverseTransformDirection(const Vector3f& d) const;
};

struct NativeRigidbody {
    float mass = 1.0f;
    Vector3f velocity = Vector3f::zero();
    Vector3f angularVelocity = Vector3f::zero();
    Vector3f worldCenterOfMass = Vector3f::zero();
    Vector3f centerOfMass = Vector3f::zero();  // local
    bool isKinematic = false;
    float maxDepenetrationVelocity = 0.0f;
    float maxAngularVelocity = 7.0f;  // Unity default

    struct ForceCall {
        Vector3f force, position;
    };
    std::vector<ForceCall> forces;  // AddForceAtPosition calls since the host cleared it

    Vector3f GetPointVelocity(const Vector3f& worldPoint) const;  // native: v + w x (p - com), float32 per operation
    void AddForceAtPosition(const Vector3f& force, const Vector3f& position) { forces.push_back({force, position}); }
};

struct PhysicMaterial {
    int id = 0;
};

struct Collider {
    NativeRigidbody* attachedRigidbody = nullptr;
    std::shared_ptr<PhysicMaterial> sharedMaterial;
};

struct WheelHit {
    Vector3f point = Vector3f::zero();
    Vector3f normal = Vector3f::zero();
    Vector3f forwardDir = Vector3f::zero();
    Vector3f sidewaysDir = Vector3f::zero();
    float force = 0.0f;
    float forwardSlip = 0.0f;
    float sidewaysSlip = 0.0f;
    Collider* collider = nullptr;
};

struct JointSpring {
    float spring = 0.0f;
    float damper = 0.0f;
    float targetPosition = 0.0f;
};

// UnityEngine.WheelCollider. Defaults are Unity's documented ones (mass 20, radius 0.5, suspensionDistance 0.3, spring 35000, damper 4500,
// targetPosition 0.5); VPWheelCollider overwrites the ones it cares about before the first tick.
class NativeWheelCollider {
public:
    bool enabled = true;
    Vector3f center = Vector3f::zero();
    float radius = 0.5f;
    float mass = 20.0f;
    float suspensionDistance = 0.3f;
    JointSpring suspensionSpring{35000.0f, 4500.0f, 0.5f};
    float forceAppPointDistance = 0.0f;
    float sprungMass = 0.0f;
    float motorTorque = 0.0f;
    float steerAngle = 0.0f;
    NativeRigidbody* attachedRigidbody = nullptr;
    // ConfigureVehicleSubsteps(speedThreshold, belowThreshold, aboveThreshold) was last called with these
    float substepSpeedThreshold = 0.0f;
    int substepsBelow = 0, substepsAbove = 0;
    // WP-29c fills these before each FixedUpdate: the WheelHit of the last Physics.Simulate
    bool groundHitValid = false;
    WheelHit groundHit;

    bool GetGroundHit(WheelHit& hit) const {  // WheelCollider.GetGroundHit
        hit = groundHit;
        return groundHitValid;
    }
    void ConfigureVehicleSubsteps(float speedThreshold, int stepsBelowThreshold, int stepsAboveThreshold) {
        substepSpeedThreshold = speedThreshold;
        substepsBelow = stepsBelowThreshold;
        substepsAbove = stepsAboveThreshold;
    }
};

}  // namespace ksp::VehiclePhysics
