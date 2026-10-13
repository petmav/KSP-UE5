// WP-29c: the native WheelCollider (see ksp/wheels/native_wheel_collider.hpp for the sources and the Unity glue). Float32 per operation in
// PhysX 4.1's statement order: the local F3 helpers below are PxVec3's dot / cross / normalize / operators; nothing here goes through the
// Mono-rule operators of ksp/unity/vector.hpp. PhysX citations are physx/source/physxvehicle/src/<file>:<line> of the 4.1 branch; C#
// citations are relative to decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/wheels/native_wheel_collider.hpp"

#include <cfloat>
#include <cmath>

#include "ksp/dotnet/math.hpp"

namespace ksp::VehiclePhysics {

namespace {

struct F3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    F3() = default;
    F3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    explicit F3(const Vector3f& v) : x(v.x), y(v.y), z(v.z) {}
    Vector3f V() const { return Vector3f(x, y, z); }
};
inline F3 Add(const F3& a, const F3& b) { return F3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline F3 Sub(const F3& a, const F3& b) { return F3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline F3 Mul(const F3& a, float s) { return F3(a.x * s, a.y * s, a.z * s); }
inline F3 Neg(const F3& a) { return F3(-a.x, -a.y, -a.z); }
inline float Dot(const F3& a, const F3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }  // PxVec3::dot
inline F3 Cross(const F3& a, const F3& b) {  // PxVec3::cross
    return F3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
inline void Normalize(F3& v) {  // PxVec3::normalize: m = sqrt(|v|^2); if (m > 0) v /= m (operator/= multiplies by 1 / m)
    const float m = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (m > 0.0f) {
        const float f = 1.0f / m;
        v.x *= f;
        v.y *= f;
        v.z *= f;
    }
}
inline float PxMaxF(float a, float b) { return a < b ? b : a; }  // PsMathUtils PxMax
inline float PxMinF(float a, float b) { return a < b ? a : b; }
// PxVehicleUpdate.cpp:1196 computeSign: fsel(f, fsel(-f, 0, 1), -1)
inline float ComputeSign(float f) { return f >= 0.0f ? (-f >= 0.0f ? 0.0f : 1.0f) : -1.0f; }
// the CRT float transcendentals PhysX calls (PxAtan, PxSin, PxCos), here the double ones of dotnet::Math rounded once
inline float AtanF(float x) { return static_cast<float>(dotnet::Math::Atan(static_cast<double>(x))); }
inline float SinF(float x) { return static_cast<float>(dotnet::Math::Sin(static_cast<double>(x))); }
inline float CosF(float x) { return static_cast<float>(dotnet::Math::Cos(static_cast<double>(x))); }

constexpr float kDeg2Rad = 0.0174532924f;                             // UnityEngine.Mathf.Deg2Rad (WheelCollider.steerAngle is in degrees)
constexpr float kRadPerSecToRpm = 60.0f / (2.0f * 3.14159265358979f);  // WheelCollider.rpm
constexpr float kMinLatSpeedForTireModel = 1.0f;                      // PxVehicleUpdate.cpp:179 (1.0 * ToleranceScale.length, length 1)
constexpr float kMinLongSlipDenominator = 4.0f;                       // PxVehicleWheels.cpp:54 / :126 (4.0 * length)
constexpr float kLongSlipOffset = 0.1f * 1.0f;                        // PxVehicleUpdate.cpp:2029 (0.1f * gToleranceScaleLength)
constexpr float kMaxHitActorAcceleration = FLT_MAX;                   // PxVehicleUpdate.cpp:134 gMaxHitActorAccelerationDefault

// PxVehicleUpdate.cpp:1946 computeTireDirs
void ComputeTireDirs(const F3& chassisLatDir, const F3& hitNorm, float wheelSteerAngle, F3& tireLongDir, F3& tireLatDir) {
    F3 tzRaw = Cross(chassisLatDir, hitNorm);
    F3 txRaw = Cross(hitNorm, tzRaw);
    Normalize(tzRaw);
    Normalize(txRaw);
    const float cosWheelSteer = CosF(wheelSteerAngle);
    const float sinWheelSteer = SinF(wheelSteerAngle);
    tireLongDir = Add(Mul(tzRaw, cosWheelSteer), Mul(txRaw, sinWheelSteer));
    tireLatDir = Sub(Mul(txRaw, cosWheelSteer), Mul(tzRaw, sinWheelSteer));
}

// PxVehicleUpdate.cpp:1962 computeTireSlips (isTank false)
void ComputeTireSlips(float longSpeed, float latSpeed, float wheelOmega, float wheelRadius, float maxDenominator, bool isAccelApplied,
                      bool isBrakeApplied, float& longSlip, float& latSlip) {
    const float longSpeedAbs = std::fabs(longSpeed);
    const float wheelSpeed = wheelOmega * wheelRadius;
    const float wheelSpeedAbs = std::fabs(wheelSpeed);
    latSlip = AtanF(latSpeed / (longSpeedAbs + kMinLatSpeedForTireModel));
    if (longSpeed == 0.0f && wheelOmega == 0.0f) {
        longSlip = 0.0f;
        return;
    }
    if (isBrakeApplied || isAccelApplied) {
        longSlip = (wheelSpeed - longSpeed) / (PxMaxF(longSpeedAbs, wheelSpeedAbs) + kLongSlipOffset);
    } else {
        longSlip = (wheelSpeed - longSpeed) / PxMaxF(maxDenominator, PxMaxF(longSpeedAbs, wheelSpeedAbs));
    }
}

}  // namespace

// Unity's WheelFrictionCurve (documented: from (0, 0) to (extremumSlip, extremumValue) with a zero tangent there, then to (asymptoteSlip,
// asymptoteValue) with a zero tangent there, flat beyond). The section shapes are the model's (quadratic, then smoothstep): UNVERIFIED.
float EvaluateFrictionCurve(const WheelFrictionCurve& c, float slip) {
    if (!(slip > 0.0f)) return 0.0f;
    if (slip < c.extremumSlip) {
        const float u = slip / c.extremumSlip;
        return c.extremumValue * (u * (2.0f - u));
    }
    if (slip < c.asymptoteSlip) {
        const float u = (slip - c.extremumSlip) / (c.asymptoteSlip - c.extremumSlip);
        const float h = u * u * (3.0f - 2.0f * u);
        return c.extremumValue + (c.asymptoteValue - c.extremumValue) * h;
    }
    return c.asymptoteValue;
}

NativeWheelGeometry ComputeNativeWheelGeometry(const NativeWheelCollider& wc, const NativeTransform& pose) {
    NativeWheelGeometry g;
    g.top = pose.TransformPoint(wc.center);
    g.up = pose.up();
    g.right = pose.right();
    g.forward = pose.forward();
    const F3 up(g.up);
    const F3 w = Neg(up);
    g.w = w.V();
    const float r = wc.radius;
    const float D = wc.suspensionDistance;
    const float t = wc.suspensionSpring.targetPosition;
    g.radius = r;
    g.maxDroop = D * t;
    g.maxCompression = D * (1.0f - t);
    g.spring = wc.suspensionSpring.spring;
    g.damper = wc.suspensionSpring.damper;
    g.sprungMass = wc.sprungMass;
    g.mass = wc.mass;
    g.moi = 0.5f * wc.mass * r * r;
    g.dampingRate = wc.wheelDampingRate;
    const F3 centreRest = Add(F3(g.top), Mul(w, g.maxCompression));
    g.centreRest = centreRest.V();
    // PxVehicleUpdate.cpp:1109 computeSuspensionRaycast: suspLineStart = transform(wheel centre offset); -= suspLineDir * (radius + maxBounce)
    g.rayOrigin = Sub(centreRest, Mul(w, r + g.maxCompression)).V();
    g.rayDir = g.w;
    // PxVehicleUpdate.cpp:7295-7297: radius + maxBounce + maxDroop + radius, then + radius "for good measure"
    float len = r + g.maxCompression + g.maxDroop + r;
    len += r;
    g.rayLength = len;
    // Unity: forceAppPointDistance measured up from the base of the resting wheel
    const F3 base = Add(centreRest, Mul(w, r));
    g.forceAppPoint = Add(base, Mul(up, wc.forceAppPointDistance)).V();
    return g;
}

NativeWheelStep SimulateNativeWheel(const NativeWheelCollider& wc, const NativeWheelGeometry& g, const NativeGroundQuery& q,
                                    const NativeChassis& chassis, const PhysxWheelDyn& prev, float dt, const Vector3f& gravity) {
    NativeWheelStep st;
    const float recipTimeStep = 1.0f / dt;
    // PxVehicleUpdate.cpp:3227-3240: jounce at max droop, limit off, in the air until we know otherwise
    float jounce = -g.maxDroop;
    st.jounce = jounce;
    const F3 hitNorm(q.normal);
    const F3 w(g.w);
    const float radius = g.radius;
    // PxVehicleUpdate.cpp:6727-6734 (updateNoDrive): the intention to accelerate, :1844 computeNoDriveBrakeTorques, :1735 computeIsAccelApplied
    const float maxAccel = std::fabs(wc.motorTorque);
    const float maxBrake = std::fabs(wc.brakeTorque);
    const bool isIntentionToAccelerate = maxAccel > 0.0f && 0.0f == maxBrake;
    const bool isBrakeApplied = wc.brakeTorque != 0.0f;
    const bool isAccelApplied = isIntentionToAccelerate && wc.motorTorque != 0.0f;
    float tireTorque = 0.0f;
    // PxVehicleUpdate.cpp:3248: a hit, not from inside the shape (distance 0), against the travel direction
    if (q.hit && q.distance != 0.0f && Dot(hitNorm, w) < 0.0f) {
        // storeHit (:2785): the hit plane n.x + d = 0, d = -n.hitPos
        const float hitD = -Dot(hitNorm, F3(q.point));
        // intersectRayPlane (:2273)
        const F3 v(g.rayOrigin);
        bool successIntersection = true;
        float dx = 0.0f;
        F3 wheelBottomPos;
        if (hitNorm.x * v.x + hitNorm.y * v.y + hitNorm.z * v.z + hitD < 0.0f) {
            successIntersection = false;
        } else {
            const F3 pos = v;
            const float T = -(Dot(hitNorm, v) + hitD) / Dot(hitNorm, w);
            const float restT = 2.0f * radius + g.maxCompression;
            dx = restT - T;
            wheelBottomPos = Add(pos, Mul(w, restT - dx));
        }
        st.dx = dx;
        // :3284 the wheel touches the plane above its max droop
        if (dx > -g.maxDroop && successIntersection) {
            st.wheelBottom = wheelBottomPos.V();
            // :3296-3300 the travel limit, the jounce clamped to max compression
            st.limitError = Dot(w, hitNorm) * (-dx + g.maxCompression);
            st.limitActive = dx > g.maxCompression;
            st.limitDir = g.w;
            st.limitPoint = g.centreRest;
            jounce = PxMinF(dx, g.maxCompression);
            st.jounce = jounce;
            // :3313-3324 the velocity of the wheel bottom relative to the hit actor
            const F3 r = Sub(wheelBottomPos, F3(chassis.centerOfMass));
            F3 wheelBottomVel(chassis.velocity);
            wheelBottomVel = Add(wheelBottomVel, Cross(F3(chassis.angularVelocity), r));
            const bool dynamicHitActor = q.rigidbodyActor;
            F3 hitActorVelocity(0.0f, 0.0f, 0.0f);
            if (dynamicHitActor) {
                // PxRigidBodyExt::getVelocityAtPos: v + w x (p - centre of mass)
                hitActorVelocity = Add(F3(q.actorVelocity), Cross(F3(q.actorAngularVelocity), Sub(wheelBottomPos, F3(q.actorCenterOfMass))));
                wheelBottomVel = Sub(wheelBottomVel, hitActorVelocity);
            }
            // :3327-3338 the jounce speed (PX_MAX_F32: no previous step)
            float jounceSpeed;
            if (PhysxWheelDyn::kNoJounce != prev.jounce) {
                jounceSpeed = (jounce - prev.jounce) * recipTimeStep;
            } else {
                jounceSpeed = 0.0f;
            }
            st.jounceSpeed = jounceSpeed;
            // :3340, :3386-3402 the spring / damper force along w, projected on the hit normal
            const F3 grav(gravity);
            const float gravitySuspDir = Dot(grav, w);
            const float alpha = PxMaxF(0.0f, gravitySuspDir);
            const F3 TTimesBeta = (0.0f != alpha) ? Sub(grav, Mul(w, alpha)) : F3(0.0f, 0.0f, 0.0f);
            float suspensionForceW = PxMaxF(0.0f, g.sprungMass * alpha + g.spring * jounce);
            suspensionForceW += jounceSpeed * g.damper;
            float suspensionForceMag = Dot(hitNorm, Sub(Mul(Neg(w), suspensionForceW), Mul(TTimesBeta, g.sprungMass)));
            // :3406-3421 the opposite force on a dynamic hit actor (gMaxHitActorAcceleration: no clamp by default)
            if (dynamicHitActor && !q.kinematicActor) {
                const float dynamicActorInvMass = 1.0f / q.actorMass;
                const float dynamicActorMass = 1.0f / dynamicActorInvMass;
                const float forceSign = ComputeSign(suspensionForceMag);
                const float forceMag = std::fabs(suspensionForceMag);
                const float clampedAccelMag = PxMinF(forceMag * dynamicActorInvMass, kMaxHitActorAcceleration);
                const float clampedForceMag = clampedAccelMag * dynamicActorMass * forceSign;
                suspensionForceMag = clampedForceMag;
                st.hitActorForceValid = true;
                st.hitActorForce = Mul(hitNorm, -clampedForceMag * 1.0f).V();  // timeFraction = 1 (one substep)
                st.hitActorForcePoint = q.point;
            }
            st.suspensionForce = suspensionForceMag;
            st.force = Mul(hitNorm, suspensionForceMag).V();
            st.forcePoint = g.forceAppPoint;
            st.grounded = true;
            // :3461-3492 the tyre axes, the speeds along them and the slips (the wheel speed of the previous step)
            F3 tireLongDir, tireLatDir;
            ComputeTireDirs(F3(g.right), hitNorm, wc.steerAngle * kDeg2Rad, tireLongDir, tireLatDir);
            st.tireLongDir = tireLongDir.V();
            st.tireLatDir = tireLatDir.V();
            st.longSpeed = Dot(wheelBottomVel, tireLongDir);
            st.latSpeed = Dot(wheelBottomVel, tireLatDir);
            ComputeTireSlips(st.longSpeed, st.latSpeed, prev.wheelSpeed, radius, kMinLongSlipDenominator, isAccelApplied, isBrakeApplied, st.longSlip,
                             st.latSlip);
            // :3512-3604 the tyre force (Unity's curves: see the header; zero for KSP's stiffness 0)
            const float tireLoad = suspensionForceMag;
            if (tireLoad > 0.0f) {
                const float longCurve = EvaluateFrictionCurve(wc.forwardFriction, std::fabs(st.longSlip));
                const float latCurve = EvaluateFrictionCurve(wc.sidewaysFriction, std::fabs(st.latSlip));
                st.tireLongForce = ComputeSign(st.longSlip) * longCurve * wc.forwardFriction.stiffness * tireLoad;
                st.tireLatForce = -(ComputeSign(st.latSlip) * latCurve * wc.sidewaysFriction.stiffness * tireLoad);
                tireTorque = -st.tireLongForce * radius;
                st.tireTorque = tireTorque;
                st.tireForce = Add(Mul(tireLongDir, st.tireLongForce), Mul(tireLatDir, st.tireLatForce)).V();
            }
            WheelHit& h = st.hit;
            h.point = q.point;
            h.normal = q.normal;
            h.forwardDir = st.tireLongDir;
            h.sidewaysDir = st.tireLatDir;
            h.force = suspensionForceMag;
            h.forwardSlip = st.longSlip;
            h.sidewaysSlip = st.latSlip;
            h.collider = q.collider;
        }
    }
    // :4389 integrateNoDriveWheelSpeeds with the brake torques of computeNoDriveBrakeTorques (:1844)
    const float oldOmega = prev.wheelSpeed;
    const float brakeTorque = -ComputeSign(oldOmega) * wc.brakeTorque;
    const float subTimeStep = dt * (1.0f / g.moi);
    float result = (oldOmega + subTimeStep * (tireTorque + wc.motorTorque + brakeTorque)) / (1.0f + g.dampingRate * subTimeStep);
    result = (isBrakeApplied && (oldOmega * result <= 0.0f)) ? 0.0f : result;
    st.wheelSpeed = result;
    return st;
}

void CommitNativeWheel(NativeWheelCollider& wc, const NativeWheelStep& s) {
    wc.physx.jounce = s.jounce;
    wc.physx.wheelSpeed = s.wheelSpeed;
    wc.isGrounded = s.grounded;
    wc.rpm = s.wheelSpeed * kRadPerSecToRpm;
    wc.groundHitValid = s.grounded;
    wc.groundHit = s.grounded ? s.hit : WheelHit();
}

void ResetNativeWheel(NativeWheelCollider& wc) {
    wc.physx.SetToRestState();
    wc.isGrounded = false;
    wc.rpm = 0.0f;
    wc.groundHitValid = false;
    wc.groundHit = WheelHit();
}

}  // namespace ksp::VehiclePhysics
