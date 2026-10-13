// WP-29c: the NATIVE WheelCollider. What UnityEngine.WheelCollider does inside Physics.Simulate for one wheel: Unity builds one PhysX 4.1
// vehicle (PxVehicleNoDrive) per Rigidbody from its WheelCollider components (KSP: one wheel part = one Rigidbody = one WheelCollider), and
// PhysX's vehicle update casts the suspension ray, solves the suspension / tyre law, reports the WheelHit and integrates the wheel spin.
// Everything above it (VPWheelCollider, VehicleBase, KSPWheelController, ModuleWheel*) is managed C# ported 1:1 in WP-29a / WP-29b; this
// file is the native layer they talk to through ksp/wheels/wheel_native.hpp (NativeWheelCollider, WheelHit, GetGroundHit).
//
// Source: PhysX 4.1 (NVIDIAGameWorks/PhysX branch 4.1, BSD-3), physx/source/physxvehicle/src/:
//   PxVehicleUpdate.cpp:1109  computeSuspensionRaycast    ray start = wheel centre at rest - w (radius + maxCompression), w = travel dir
//   PxVehicleUpdate.cpp:7255  PxVehicleWheels4SuspensionRaycasts   ray length = 3 radius + maxCompression + maxDroop
//   PxVehicleUpdate.cpp:2273  intersectRayPlane            jounce dx = (2 radius + maxCompression) - T, T = the ray / hit-plane distance
//   PxVehicleUpdate.cpp:3203-3625  processSuspTireWheels   the hit test, the travel limit, jounce speed, the spring / damper force projected
//                                  on the hit normal, the reaction on a dynamic hit actor, the tyre axes, slips and the tyre force
//   PxVehicleUpdate.cpp:1946 / :1962  computeTireDirs / computeTireSlips;  :4389 integrateNoDriveWheelSpeeds (implicit damping, brake lock)
//   PxVehicleSuspLimitConstraintShader.h:130-142  the travel limit: one unilateral row along the travel direction at the wheel centre
// Arithmetic: float32 per operation in PhysX's statement order (PxVec3 dot / cross / normalize as PhysX writes them); the transcendental
// functions PhysX takes from the CRT's float maths (atanf, sinf, cosf) are the double ones of dotnet::Math rounded to float (portable; one
// ulp of the platform's float functions is not a modelled property).
//
// UNITY GLUE (no source exists; UNVERIFIED, WHEEL-SPIKE.md risk R1, capture C11a settles it):
//   * Unity -> PhysX mapping: WheelCollider.center (local) is the wheel centre at FULL compression; suspensionDistance D extends it down the
//     transform's -up; suspensionSpring.targetPosition t is the rest point as a fraction of the travel from FULL EXTENSION (the reading VPP's
//     own target formula assumes, VPWheelCollider.cs:438-443), so maxDroop = t D, maxCompression = (1 - t) D; spring / damper are the PhysX
//     spring strength / damper rate; sprungMass = Rigidbody.mass / (wheel colliders on it) = the wheel part's rb.mass in KSP.
//   * forceAppPointDistance: measured up the travel from the bottom of the wheel at rest (Unity docs); KSP sets it every tick so that the
//     force acts at the contact point (VehicleBase.cs:740-743 -> VPWheelCollider.SetSuspensionForceOffset).
//   * WheelHit: point / normal = the ray hit, force = the suspension force magnitude (suspensionSpringForces), forwardDir / sidewaysDir /
//     forwardSlip / sidewaysSlip = PhysX's tyre axes and slips; GetGroundHit is true while the wheel is not in the air.
//   * the tyre model: Unity's own shader over WheelFrictionCurve (documented as a two-piece curve with zero tangents at the extremum and the
//     asymptote). Modelled as F = sign(slip) curve(|slip|) stiffness load. KSP gives every WheelCollider stiffness 0 (VPWheelCollider.cs:
//     405-408): the PhysX tyre force is exactly zero in KSP whatever the curve; the tyre is VPP's (WP-29a).
//   * the lateral reference: PhysX uses the chassis right; with one wheel per Rigidbody Unity's chassis axes are taken as the wheel
//     collider transform's (its right is the axle).
//   * rpm = wheel speed in rpm; wheel MOI = 0.5 mass radius^2; wheelDampingRate 0.25 (Unity default).
// NOT MODELLED (PhysX features with no effect on KSP's set-up or no Unity evidence): the eLIMIT_SUSPENSION_EXPANSION_VELOCITY flag (off by
// default), camber, the tyre-load filter, the sticky-tyre friction constraints (PxVehicleSuspLimitConstraintShader.h:145-190: whether
// Unity's WheelCollider activates them is unknown; they would hold a parked wheel at < 0.2 m/s after 1 s), sweeps (Unity casts rays).
// Physics.gravity: KSP sets it to zero (gravity is a per-vessel force, VesselPrecalculate.cs:281-368), so the sprung-mass term
// sprungMass * max(0, g.w) of the law vanishes; `gravity` is still an input (zero in KSP).
#pragma once

#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/wheels/wheel_native.hpp"

namespace ksp::VehiclePhysics {

// Unity's WheelFrictionCurve evaluation (documented shape, UNVERIFIED; inert in KSP: stiffness 0). slip >= 0; returns the curve value
// (multiplied by stiffness and the tyre load by the caller).
float EvaluateFrictionCurve(const WheelFrictionCurve& c, float slip);

// What Unity hands PhysX for one wheel this step, in the frame of the transforms (Unity's world frame; kspcore's transform frame):
struct NativeWheelGeometry {
    Vector3f top;               // wheel centre at full compression: transform.TransformPoint(center)
    Vector3f up, right, forward;  // the wheel collider transform's axes
    Vector3f w;                 // suspension travel direction (-up)
    float radius = 0.0f;
    float maxCompression = 0.0f, maxDroop = 0.0f;  // (1 - t) D, t D
    float spring = 0.0f, damper = 0.0f, sprungMass = 0.0f;
    float mass = 0.0f, moi = 0.0f, dampingRate = 0.0f;
    Vector3f centreRest;        // wheel centre at zero jounce: top + w maxCompression
    Vector3f forceAppPoint;     // bottom of the resting wheel + up forceAppPointDistance
    Vector3f rayOrigin;         // computeSuspensionRaycast
    Vector3f rayDir;            // = w
    float rayLength = 0.0f;     // radius + maxCompression + maxDroop + radius + radius
};
NativeWheelGeometry ComputeNativeWheelGeometry(const NativeWheelCollider& wc, const NativeTransform& pose);

// The scene query of the suspension ray (the host casts it against the world without the wheel's own vessel), transform frame
struct NativeGroundQuery {
    bool hit = false;
    float distance = 0.0f;      // along the ray from rayOrigin (PhysX rejects 0: the ray started inside the shape)
    Vector3f point, normal;     // hit position, unit normal
    Collider* collider = nullptr;
    // the hit actor when it is a rigid body (another vessel's part): PxRigidBodyExt::getVelocityAtPos and the reaction force
    bool rigidbodyActor = false;
    bool kinematicActor = false;
    float actorMass = 0.0f;
    Vector3f actorVelocity, actorAngularVelocity, actorCenterOfMass;
};

// The Rigidbody the wheel collider is attached to (PhysX's chassis: position = the centre of mass)
struct NativeChassis {
    Vector3f centerOfMass, velocity, angularVelocity;
};

// One step of the wheel (processSuspTireWheels with Unity's one substep, then integrateNoDriveWheelSpeeds)
struct NativeWheelStep {
    bool grounded = false;              // !isInAir
    float dx = 0.0f;                    // the raw jounce of intersectRayPlane (0 without a valid hit)
    float jounce = 0.0f;                // stored as the next step's previous jounce (-maxDroop in the air)
    float jounceSpeed = 0.0f;
    float suspensionForce = 0.0f;       // suspensionSpringForces[i]: the force magnitude along the hit normal (WheelHit.force)
    Vector3f force;                     // hitNorm * suspensionForce, on the chassis at forcePoint
    Vector3f forcePoint;
    bool limitActive = false;           // the travel-limit row (dx > maxCompression)
    float limitError = 0.0f;            // (w . n)(maxCompression - dx): positive while over-compressed
    Vector3f limitDir, limitPoint;      // row direction (w) and anchor (the wheel centre at rest)
    Vector3f wheelBottom;               // the point of the suspension line on the hit plane
    bool hitActorForceValid = false;    // the reaction on a dynamic, non-kinematic hit actor
    Vector3f hitActorForce, hitActorForcePoint;
    Vector3f tireLongDir, tireLatDir;
    float longSpeed = 0.0f, latSpeed = 0.0f, longSlip = 0.0f, latSlip = 0.0f;
    float tireLongForce = 0.0f, tireLatForce = 0.0f, tireTorque = 0.0f;
    Vector3f tireForce;                 // on the chassis at forcePoint (0 in KSP)
    float wheelSpeed = 0.0f;            // after the step (rad/s)
    WheelHit hit;                       // GetGroundHit's answer (valid when grounded)
};

// The law. `prev` is the wheel's PhysX state before the step; dt the physics step; gravity Physics.gravity (zero in KSP).
NativeWheelStep SimulateNativeWheel(const NativeWheelCollider& wc, const NativeWheelGeometry& g, const NativeGroundQuery& q,
                                    const NativeChassis& chassis, const PhysxWheelDyn& prev, float dt, const Vector3f& gravity = Vector3f::zero());

// Writes the step into the component (what the managed side reads afterwards): physx state, isGrounded, rpm, the WheelHit of GetGroundHit
// (groundHit / groundHitValid), sprungMass. A disabled collider: no hit, rest state (Unity drops the PhysX wheel while the component is off).
void CommitNativeWheel(NativeWheelCollider& wc, const NativeWheelStep& s);
void ResetNativeWheel(NativeWheelCollider& wc);

}  // namespace ksp::VehiclePhysics
