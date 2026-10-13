// Wheel spike (core/WHEEL-SPIKE.md): the MANAGED half of a KSP 1.12.5 wheel, ported statement by statement from the C#
// (citations relative to decompiled/KSP-Decompiled/Assembly-CSharp/). This is the code WP-29 ports 1:1 and gates bit-exactly:
//
//   ModuleWheelBase           ModuleWheelBase.cs:376-409 (ApplyGeeBias, UpdateFriction), :421-463 (FixedUpdate)
//   ModuleWheelSuspension     ModuleWheels/ModuleWheelSuspension.cs:315-435 (SuspensionSpringUpdate, BoostCurve, UpdateAutoBoost)
//   SuspensionLoadBalancer    SuspensionLoadBalancer.cs:31-59
//   ModuleWheelMotor          ModuleWheels/ModuleWheelMotor.cs:122-176
//   ModuleWheelBrakes         ModuleWheels/ModuleWheelBrakes.cs:55-104
//   ModuleWheelLock           ModuleWheels/ModuleWheelLock.cs:8-13
//   ModuleWheelDamage         ModuleWheels/ModuleWheelDamage.cs:289-366 (stress / impact, reported only)
//   KSPWheelController        KSPWheelController.cs:210-264 (one VehicleBase per wheel part, DirectDrive, brake torque)
//   VehicleBase.FixedUpdate   VehiclePhysics/VehicleBase.cs:697-849, :934-1033
//   VPWheelCollider           VehiclePhysics/VPWheelCollider.cs:313-473 (GetGroundHit fix-ups, contact depth, UpdateWheelCollider)
//   Wheel / DirectDrive       VehiclePhysics/Wheel.cs, DirectDrive.cs (the tyre / wheel-spin block)
//   Solver                    VehiclePhysics/Solver.cs:181-207, :271-284 (explicit Euler, 8 / 4 substeps)
//   TireFriction              VehiclePhysics/TireFriction.cs (Smooth and Lineal curves)
//
// The NATIVE half (what UnityEngine.WheelCollider = PhysX vehicle SDK does: the suspension ray, the suspension force, the
// WheelHit) is native_suspension.hpp. The two halves meet in NativeHit (what WheelCollider.GetGroundHit returns) and
// NativeSpring (what VPWheelCollider writes into WheelCollider.suspensionSpring).
//
// Spike simplifications (WP-29 must not keep them): plain float32 arithmetic in C++ evaluation order (not kspcore's Mono float
// rule); no steering (wheels straight), no ground materials (KSP has no GroundMaterialManager: grip 1, drag 0), electric
// charge always available (resourceFraction 1), motor orientation sign +1, ModuleWheelDeployment fixed at Deployed.
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Math/Vec3.h>

#include <cmath>
#include <string>

#include "ksp/curves/float_curve.hpp"

namespace ksp {
class Part;
}

namespace wspike {

using JPH::Vec3;

struct V2 {
    float x = 0.0f, y = 0.0f;
    V2() = default;
    V2(float ax, float ay) : x(ax), y(ay) {}
    V2 operator+(V2 o) const { return {x + o.x, y + o.y}; }
    V2 operator-(V2 o) const { return {x - o.x, y - o.y}; }
    V2 operator*(float s) const { return {x * s, y * s}; }
    V2 operator/(float s) const { return {x / s, y / s}; }
    float magnitude() const { return std::sqrt(x * x + y * y); }
};
inline V2 operator*(float s, V2 v) { return {s * v.x, s * v.y}; }

// UnityEngine.Mathf semantics used by the C#
namespace mathf {
inline float Clamp(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline float Clamp01(float v) { return Clamp(v, 0.0f, 1.0f); }
inline float Lerp(float a, float b, float t) { return a + (b - a) * Clamp01(t); }
inline float InverseLerp(float a, float b, float v) { return a != b ? Clamp01((v - a) / (b - a)) : 0.0f; }
inline float Sign(float f) { return f >= 0.0f ? 1.0f : -1.0f; }  // Mathf.Sign(0) == 1
inline float MoveTowards(float cur, float target, float maxDelta) {
    if (std::fabs(target - cur) <= maxDelta) return target;
    return cur + Sign(target - cur) * maxDelta;
}
inline float Round(float f) { return static_cast<float>(std::nearbyint(static_cast<double>(f))); }  // Math.Round: to even
}  // namespace mathf

// ---- TireFriction (TireFriction.cs) ---------------------------------------------------------------------------------------
struct TireFriction {
    enum class Model { Lineal, Smooth };
    // KSP: ModuleWheelBase.cs:361 writes model = Lineal, but m_curve is only rebuilt by SetupFrictionCurves() (constructor and
    // OnAfterDeserialize, TireFriction.cs:401-435), never after that write: the curve object in effect is the constructor's
    // SmoothFriction. Smooth is the default here; Lineal is the alternative reading (bench `friction`).
    Model model = Model::Smooth;
    V2 adherent{0.5f, 0.95f}, peak{1.5f, 1.1f}, limit{4.0f, 0.8f};  // Settings defaults (TireFriction.cs:23-27)
    float frictionMultiplier = 1.0f;                                  // TireFriction.frictionMultiplier (KSP never changes it)

    float EvaluateForce(float s, float load, float grip) const;       // CurveBase.EvaluateForce
    float AdherentSlip() const { return adherent.x; }                 // GetAdherentSlip (Lineal / Smooth)
    V2 GetForce(V2 slip, float load, float grip) const;               // :437-450
    bool IsAdherentSlip(V2 slip) const { return slip.magnitude() < AdherentSlip(); }
    float GetAdherentSlipForward(V2 slip) const;                      // :457-466
    float GetAdherentSlipSideways(V2 slip) const;                     // :468-477
    V2 GetAdherentForce(V2 slip, float load, float grip) const;       // :497-501
};

// ---- the per-wheel block chain: VPP Wheel + DirectDrive under Solver.IntegrateEuler ----------------------------------------
struct VppWheelBlock {
    // Wheel (Wheel.cs)
    const TireFriction* tireFriction = nullptr;
    float radius = 0.3f, mass = 20.0f, I = 1.0f, invI = 1.0f;
    float load = 0.0f, grip = 1.0f;
    V2 V, Fext;
    float brakeTorqueAcc = 0.0f;  // m_brakeTorque
    float L = 0.0f;
    V2 P;
    float w = 0.0f, T = 0.0f;
    V2 F;
    float Tr = 0.0f;
    bool isResting = false;
    float forwardSlip = 0.0f;
    V2 tireForce;
    float Ty = 0.0f;
    bool isAdherent = false;
    V2 adherentSlip, adherentForce;
    float inTd = 0.0f, inTb = 0.0f;
    V2 contactSlip;  // m_contact.slip (load / grip are copied per call)
    // Connection wheel <- DirectDrive (Block.Connection)
    float connL = 0.0f, connI = 0.0f, connTr = 0.0f, connOutTd = 0.0f;
    // DirectDrive (DirectDrive.cs)
    float motorInput = 0.0f, maxMotorTorque = 100.0f, maxRpm = 500.0f, damping = 0.95f;

    void RecalculateConstants();           // Wheel.cs:76-84
    void AddBrakeTorque(float t);          // Wheel.cs:86-94
    // Solver.IntegrateEuler (Solver.cs:181-207) over [wheel, directDrive]
    void Integrate(float dt, int subSteps);

private:
    void ComputeStateUpstream();           // Wheel.cs:128-163
    void DirectDriveEvaluate(float dt);    // DirectDrive.cs:34-41
    void EvaluateTorqueDownstream(float dt);  // Wheel.cs:165-242
};

// ---- the native interface (filled / consumed by native_suspension) ---------------------------------------------------------
// What WheelCollider.GetGroundHit returns (UnityEngine.WheelHit), as VPWheelCollider.GetGroundHit post-processes it.
struct NativeHit {
    bool grounded = false;
    JPH::RVec3 point;      // contact point (physics frame), already + rb.velocity * fixedDeltaTime (VPWheelCollider.cs:324-327)
    Vec3 normal = Vec3::sAxisY();
    float force = 0.0f;    // WheelHit.force: PhysX suspSpringForce (suspension spring + damper, projected on the normal)
    float sidewaysSlip = 0.0f, forwardSlip = 0.0f;  // PhysX slips (read by ModuleWheelSteering only)
    float compression = 0.0f;  // diagnostic: suspension compression from full extension (m)
    bool bottomed = false;     // diagnostic: PhysX suspension limit active
};
// What VPWheelCollider.UpdateWheelCollider writes into WheelCollider.suspensionSpring (JointSpring) + forceAppPointDistance.
struct NativeSpring {
    float spring = 35000.0f, damper = 4500.0f, targetPosition = 0.5f;  // Unity's WheelCollider defaults until the first write
    float forceAppPointDistance = 0.0f;
    float suspensionDistance = 0.1f;
};

// ---- configuration (KSPField values of the part's modules: cfg defaults, then the craft's persistent values) ---------------
struct WheelConfig {
    std::string partName;
    int wheelType = 1;  // WheelType: FREE 0, MOTORIZED 1, LEG 2
    float rescaleFactor = 1.25f;
    // ModuleWheelBase
    float radius = 0.0f;          // cfg (unscaled: used by the motor, maxRpm, slip displacement)
    Vec3 center = Vec3::sZero();
    float wcMass = 0.0f;          // ModuleWheelBase.mass (the WheelCollider / VPP wheel mass)
    float frictionSharpness = 0.0f, wheelDamping = 0.05f, wheelMaxSpeed = 1000.0f;
    float adherentStart = 0.5f, frictionAdherent = 0.25f, peakStart = 4.0f, frictionPeak = 1.45f, limitStart = 7.0f, frictionLimit = 1.1f;
    bool autoFrictionAvailable = true, autoFriction = true, useNewFrictionModel = false;
    float frictionMultiplier = 1.0f, geeBias = 1.6f, minimumDownforce = 0.5f;
    float tireForceSharpness = 10.0f, suspensionForceSharpness = 10.0f, springSlerpRate = 0.02f, groundHeightOffset = 0.0f;
    int activeSubsteps = -1, inactiveSubsteps = -1;
    // ModuleWheelSuspension
    bool hasSuspension = false;
    float suspensionDistance = 0.0f, suspensionOffset = 0.0f, targetPosition = 1.0f, springRatio = 50.0f, springTweakable = 1.0f;
    float damperRatio = 1.0f, damperTweakable = 1.0f, boostRatio = 0.0f, maximumLoad = 0.0f, autoBoost = 0.0f;
    bool autoSpringDamper = true, useDistributedMass = false, useAutoBoost = true, adjustForHighGee = true;
    float highGeeThreshold = 1.5f, highGeeSpringTweakable = 3.0f, highGeeDamperTweakable = 1.2f;
    // ModuleWheelMotor
    bool hasMotor = false;
    float driveResponse = 3.0f, wheelSpeedMax = 120.0f, driveLimiter = 100.0f, tractionControlScale = 1.0f;
    bool autoTorque = true, motorEnabled = true, motorInverted = false;
    ksp::FloatCurve torqueCurve;
    // ModuleWheelBrakes
    bool hasBrakes = false;
    float maxBrakeTorque = 2000.0f, brakeResponse = 10.0f, brakeTweakable = 50.0f;
    // ModuleWheelLock
    bool hasLock = false;
    float lockTorque = 1000.0f;
    // ModuleWheelDamage
    bool hasDamage = false;
    float stressTolerance = 0.0f, impactTolerance = 0.0f, deflectionMagnitude = 0.0f, slipMagnitude = 0.0f;
    float deflectionSharpness = 2.0f, slipSharpness = 2.0f;
    // ModuleWheelDeployment
    bool hasDeployment = false;
};
// Reads the module fields of a live kspcore Part (generic PartModules: schema defaults, cfg values, craft persistent values).
bool ReadWheelConfig(const ksp::Part& p, WheelConfig& out);

// Vessel-level inputs of one tick
struct VesselInputs {
    float dt = 0.02f;                 // Time.fixedDeltaTime (= TimeWarp.fixedDeltaTime)
    Vec3 gravityForPos = Vec3(0, -9.81f, 0);  // vessel.gravityForPos (physics frame)
    double geeASL = 1.0;              // mainBody.GeeASL
    double gravityMultiplier = 1.0;   // vessel.gravityMultiplier
    double totalMass = 1.0;           // vessel.totalMass / GetTotalMass()
    bool landed = false;              // vessel.Landed
    float wheelThrottle = 0.0f, wheelThrottleTrim = 0.0f;  // ctrlState
    float brakeInput = 0.0f;          // ModuleWheelBrakes.brakeInput (brake action group: 0 / 1)
    bool activeVessel = true;
    double maximumSurfaceGeeASL = 1.7; // PSystemManager.MaximumSurfaceGeeASL (stock: Tylo 0.8 / Eve 1.7 / Moho... ; Eve's 1.7 is the max)
    float gravitationalAcceleration = 9.80665f;  // PhysicsGlobals.GravitationalAcceleration
};

// Rigid-body queries the managed code makes (Rigidbody.GetPointVelocity / velocity / mass of the wheel part's body)
struct BodyView {
    JPH::RVec3 com;
    Vec3 v = Vec3::sZero(), w = Vec3::sZero();
    float partRbMass = 1.0f;  // m_rigidbody.mass: the WHEEL PART's rb.mass (VehicleBase.cs:967, Unity's sprungMass)
    Vec3 PointVelocity(JPH::RVec3Arg p) const { return v + w.Cross(Vec3(p - com)); }
};

// One wheel part: ModuleWheelBase + submodules + KSPWheelController(VehicleBase) + the VPWheelCollider wrapper.
class KspWheel {
public:
    WheelConfig cfg;
    // geometry of the WheelCollider transform (physics frame, refreshed every tick from the body pose)
    Vec3 up = Vec3::sAxisY(), forward = Vec3::sAxisZ();
    JPH::RVec3 transformPos;  // wheelCollider transform origin

    // VPWheelCollider + derived WheelCollider values (VPWheelCollider.cs:12-26 defaults, overwritten in Setup)
    float wcRadius = 0.3f, suspensionDistance = 0.25f, suspensionAnchor = 0.5f, springRate = 30000.0f, damperRate = 1500.0f;
    float wcMass = 20.0f;
    float sprungMassUnity = 1.0f;  // WheelCollider.sprungMass: the wheel part's rb.mass (one WheelCollider per Rigidbody)
    Vec3 wcCenter = Vec3::sZero();
    float runtimeSpringRate = 0, runtimeDamperRate = 0, runtimeSuspensionTravel = 0, runtimeExtraDownforce = 0;
    float lastRuntimeSpringRate = 0, lastRuntimeDamperRate = 0, lastRuntimeSuspensionTravel = 0;
    NativeSpring nativeSpring;

    // VehicleBase.WheelState (VehicleBase.cs:18-75)
    NativeHit hit;
    float contactDepth = 0, suspensionCompression = 0, downforce = 0, normalizedLoad = 0, contactAngle = 0, contactSpeed = 0;
    float damperForce = 0, lastContactDepth = 0;
    Vec3 forwardDir = Vec3::sAxisZ(), sidewaysDir = Vec3::sAxisX();
    Vec3 wheelVelocity = Vec3::sZero(), surfaceForce = Vec3::sZero();
    V2 localWheelVelocity, localSurfaceForce, externalTireForce, tireForce, tireSlip, lastTireForce;
    float angularVelocity = 0, reactionTorque = 0, combinedTireSlip = 0, driveTorque = 0, brakeTorque = 0;
    VppWheelBlock block;
    TireFriction tire;

    // KSPWheelController (KSPWheelController.cs:10-56)
    float maxDriveTorque = 0, maxBrakeTorque = 0, maxBrakeTorqueRest = 1000.0f, maxRpm = 0, wheelDamping = 0.95f;
    float driveInput = 0, brakeInputW = 0, driveResponse = 0, brakeResponse = 0, driveState = 0, brakeState = 0;
    int integrationSteps = 4;
    float minimumdownForce = 0;
    float tireForceSharpness = 5.0f, tireImpulseRatio = 0.5f;
    bool tireForceSmoothing = true, tireSideDeflection = false;
    float tireSideDeflectionRate = 10.0f;
    float gravityMag = 9.81f;
    Vec3 gravityUp = Vec3::sAxisY();

    // ModuleWheelBase / submodules
    V2 slipDisplacement;
    bool isGrounded = false;
    float spring = 0, damper = 0, boost = 0, damperFudge = 0, sprungMassGravity = 0, springClampMax = 5000, damperClampMax = 5000;
    float autoBoost = 0, springTweakable = 1, damperTweakable = 1;
    bool autoSpringDamperSet = false;
    float motorDriveInput = 0, motorMaxTorque = 0, driveLimiter = 100;
    float brakeInputModule = 0;  // ModuleWheelBrakes.brakeInput (persistent; 0 / 1 from the action group)

    // ModuleWheelDamage (diagnostic: the C# would damage / explode the part)
    float currentDeflection = 0, currentSlip = 0, currentDownForce = 0, lastDownForceDmg = 0, totalStress = 0;
    float maxImpact = 0, maxStressRatio = 0;
    bool damageInitialized = false;
    int impactsOverTolerance = 0;

    // ---- setup: ModuleWheelBase.wheelSetup (:277-374) + submodule OnWheelSetup + ApplyGeeBias(GeeASL)
    void Setup(const WheelConfig& c, float geeASL, bool activeVessel);

    // ---- per tick, in the assumed script order (WHEEL-SPIKE.md 1.4) ----
    void WheelBaseFixedUpdate(const VesselInputs& in);                  // ModuleWheelBase.cs:421-463
    void SuspensionFixedUpdate(const VesselInputs& in);                 // ModuleWheelSuspension.cs:315-322
    void MotorFixedUpdate(const VesselInputs& in);                      // ModuleWheelMotor.cs:131-176
    void BrakesFixedUpdate(const VesselInputs& in);                     // ModuleWheelBrakes.cs:88-104
    // VehicleBase.FixedUpdate for this wheel's controller (one wheel per controller): reads `hit` (the last native query),
    // integrates the wheel spin, returns the tyre force and its application point (ApplyTireForce: AddForceAtPosition).
    bool VehicleFixedUpdate(const VesselInputs& in, const BodyView& body, Vec3& outForce, JPH::RVec3& outPoint);
    void DamageFixedUpdate(const VesselInputs& in);                     // ModuleWheelDamage.cs:289-366 (diagnostic)
    float LegacyWheelLoad() const;                                      // KSPWheelController.cs:113-123

    // Unity center (top of the suspension travel) in the transform's frame: center + up * D * (1 - anchor) (VPWheelCollider.cs:421)
    Vec3 UnityCenterLocal() const { return wcCenter + Vec3::sAxisY() * (suspensionDistance * (1.0f - suspensionAnchor)); }

private:
    float BoostCurve(float b, float power) const;                       // ModuleWheelSuspension.cs:350-353
    void SuspensionSpringUpdate(float sprungMass, const VesselInputs& in);  // :355-419
    void UpdateAutoBoost(float st, float fixedDt);                      // :421-435
    void UpdateWheelCollider(const VesselInputs& in);                   // VPWheelCollider.cs:414-473
    float GetContactDepth(JPH::RVec3Arg contactPoint, float suspensionTravel) const;  // VPWheelCollider.cs:351-359
    float autoBoostClamp_ = 1.0f, boostClamp_ = 0.5f, oscillationDamper_ = 3.0f, damperLerpBase_ = 1.0f;
    float springSlerpRate_ = 0.02f;  // VPWheelCollider.springSlerpRate (ModuleWheelBase.springSlerpRate, wheelSetup :290-293)
};

// SuspensionLoadBalancer.FixedUpdate (SuspensionLoadBalancer.cs:31-59): distributed sprung mass over the grounded wheels.
void SuspensionLoadBalancerFixedUpdate(KspWheel* wheels, int count, const VesselInputs& in);

// WheelUtil.SpeedToRpm (ModuleWheels/WheelUtil.cs:17-20)
inline float SpeedToRpm(float speed, float wheelRadius) { return speed * 30.0f / (wheelRadius * 3.14159265f); }

}  // namespace wspike
