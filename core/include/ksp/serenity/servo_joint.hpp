// WP-52: the joint side of the robotic servos (Breaking Ground, Assembly-CSharp/Expansions/Serenity).
//
// KSP drives its servos with a PhysX ConfigurableJoint (D6) that BaseServo.OnJointInit adds to the moving transform's GameObject: the module writes
// the joint's motion types, limits, limit springs and DRIVES (position spring / damper / maximum force, target rotation / position / velocity) and
// PhysX does the rest. The reduced rigid-body model of WP-20b has no articulated bodies (one Jolt body per rigid cluster, robotic parts welded, see
// vessel_physics.hpp "ARTICULATION POINT"). DECISION (documented in STATUS.md, WP-52):
//
//   * The module logic is ported 1:1 and writes its joint configuration into ConfigurableJointData, a plain-data model of the ConfigurableJoint
//     properties the servos use (this file). That is the engine-agnostic part of the joint drive, and the contract of the seam: an engine
//     (UE5 shell, Jolt) reads ConfigurableJointData every physics step and drives the moving body.
//   * ConfigurableJointExtensions.SetTargetRotationLocal (the one piece of managed joint maths, ConfigurableJointExtensions.cs) is ported.
//   * HeadlessServoPlant is a REFERENCE PLANT: a 1-DOF model of the D6 drive (spring / damper / force clamp, hard limits, velocity drive, brake
//     torque, free spin) that moves the moving node's local transform. It is NOT PhysX: it exists so the servo logic can be exercised closed-loop
//     in tests and headless runs, and as the specification of the drive law a Jolt / UE5 backend must reproduce. Every constant of it is labelled.
//   * The reduced model keeps the servo bodies welded to their cluster until a backend with a real articulated joint exists:
//     `VesselPhysicsOptions::isArticulation` is the hook (ServoArticulation() in base_servo.hpp answers it for robotic parts).
//
// UNVERIFIED (no C# source, no capture): the ConfigurableJoint property defaults (Unity native), the PhysX drive law, the sign convention between
// targetRotation / targetPosition and the physical joint coordinate (derived from the algebra of SetTargetRotationInternal), Quaternion.ToAngleAxis.
#pragma once

#include <cfloat>
#include <functional>
#include <memory>
#include <string>

#include "ksp/unity/color.hpp"  // Matrix4x4f
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::Expansions::Serenity {

// UnityEngine.ConfigurableJointMotion
enum class ConfigurableJointMotion : int { Locked = 0, Limited = 1, Free = 2 };
// UnityEngine.RotationDriveMode
enum class RotationDriveMode : int { XYAndZ = 0, Slerp = 1 };
// UnityEngine.SoftJointLimit / SoftJointLimitSpring / JointDrive (value types; the Unity defaults are native: UNVERIFIED)
struct SoftJointLimit {
    float limit = 0.0f, bounciness = 0.0f, contactDistance = 0.0f;
};
struct SoftJointLimitSpring {
    float spring = 0.0f, damper = 0.0f;
};
struct JointDrive {
    float positionSpring = 0.0f, positionDamper = 0.0f;
    float maximumForce = FLT_MAX;  // PhysX forceLimit default (PX_MAX_F32): UNVERIFIED for Unity's wrapper
};

// Matrix4x4 maths the servos use (servoParentTransform = part.worldToLocalMatrix * jointParent.localToWorldMatrix, its inverse, and
// `matrix * vector3`, which is Matrix4x4 * Vector4 with w = 0 and then Vector4 -> Vector3). operator*(Matrix4x4, Matrix4x4) and
// operator*(Matrix4x4, Vector4) are managed in Unity (Matrix4x4.cs:328 / :350: Mono float rule); Matrix4x4.inverse is native (Internal Inverse):
// the maths below is a double Gauss-Jordan rounded to float, UNVERIFIED against the native rounding.
Matrix4x4f MatrixMultiply(const Matrix4x4f& lhs, const Matrix4x4f& rhs);
Vector3f MatrixMultiplyDirection(const Matrix4x4f& lhs, const Vector3f& v);  // Matrix4x4 * Vector4(v, w = 0) -> xyz
Matrix4x4f MatrixInverse(const Matrix4x4f& m);

// The ConfigurableJoint properties BaseServo and its subclasses read and write, with the Unity defaults.
struct ConfigurableJointData {
    Vector3f anchor{0, 0, 0};
    Vector3f axis{1, 0, 0};
    Vector3f secondaryAxis{0, 1, 0};
    Vector3f connectedAnchor{0, 0, 0};
    bool autoConfigureConnectedAnchor = true;
    bool configuredInWorldSpace = false;
    bool hasConnectedBody = false;             // connectedBody != null (the part's Rigidbody: PostStartInitJoint)
    float connectedMassScale = 1.0f;
    ConfigurableJointMotion xMotion = ConfigurableJointMotion::Locked, yMotion = ConfigurableJointMotion::Locked, zMotion = ConfigurableJointMotion::Locked;
    ConfigurableJointMotion angularXMotion = ConfigurableJointMotion::Locked, angularYMotion = ConfigurableJointMotion::Locked,
                            angularZMotion = ConfigurableJointMotion::Locked;
    SoftJointLimit linearLimit, lowAngularXLimit, highAngularXLimit;
    SoftJointLimitSpring linearLimitSpring, angularXLimitSpring;
    JointDrive xDrive, angularXDrive, slerpDrive;  // xDrive is the LINEAR drive, angularXDrive the angular one (the hinge reads one, writes the other)
    RotationDriveMode rotationDriveMode = RotationDriveMode::XYAndZ;
    Vector3f targetPosition{0, 0, 0}, targetVelocity{0, 0, 0}, targetAngularVelocity{0, 0, 0};
    Quaternionf targetRotation = Quaternionf::identity();
};

// ConfigurableJointExtensions.cs:5 SetTargetRotationLocal / :14 SetTargetRotation (space = Self / World) via SetTargetRotationInternal (:23).
// Quaternion.LookRotation and Quaternion.Inverse are the native shims (quaternion_native.hpp, UNVERIFIED LookRotation).
// `logError` receives the Debug.LogError text when the joint is in the wrong space (the C# logs and goes on).
void SetTargetRotationLocal(ConfigurableJointData& joint, const Quaternionf& targetLocalRotation, const Quaternionf& startLocalRotation);
void SetTargetRotationWorld(ConfigurableJointData& joint, const Quaternionf& targetWorldRotation, const Quaternionf& startWorldRotation);

// Quaternion.ToAngleAxis(out angle, out axis) (native, Runtime/Math/Quaternion.h QuaternionToAxisAngle; degrees): UNVERIFIED formulation.
void ToAngleAxis(const Quaternionf& q, float& angle, Vector3f& axis);

// The physical coordinate the D6 drive pulls the joint to, read back from the joint data (derivation in servo_joint.cpp): for a revolute hinge
// the twist of targetRotation about the joint's X axis with the sign of SetTargetRotationInternal's inversion removed (degrees).
double PhysicalTargetAngleDegrees(const ConfigurableJointData& joint);

// ---- the reference plant ----------------------------------------------------------------------------------------------------------
// One moving body (the servo Rigidbody) on a 1-DOF joint to a fixed base. SYNTHETIC parameters: the inertia (kg m^2, or mass for the
// prismatic joint) is not game data.
struct ServoPlantState {
    double position = 0.0, velocity = 0.0;  // degrees, deg/s (revolute) or m, m/s (prismatic): the PHYSICAL coordinate
    double lastForce = 0.0;                 // the drive force / torque of the last step (after the clamp)
};
struct ServoPlantParams {
    double inertia = 1.0;     // SYNTHETIC
    double brakeFreeDamping = 0.0;
};
// PhysX D6 drive law (force mode), implicit integration: F = k (target - x) + c (targetVel - v), |F| <= maximumForce. Documented model.
// `limitLo/Hi` are the physical limits (hard clamp, velocity zeroed) used when `limited`.
void StepDrive1D(ServoPlantState& s, const ServoPlantParams& p, const JointDrive& drive, double target, double targetVelocity, bool limited,
                 double limitLo, double limitHi, double dt);

}  // namespace ksp::Expansions::Serenity
