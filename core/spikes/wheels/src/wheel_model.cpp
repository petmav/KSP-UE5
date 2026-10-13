#include "wheel_model.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "ksp/parts/part.hpp"

namespace wspike {

using namespace mathf;

namespace {

constexpr float kRpmToW = 3.14159265f / 30.0f;  // Block.RpmToW = (float)Math.PI / 30f (Block.cs:53)

// TireFriction.SmoothFriction.CubicLerp (TireFriction.cs:193-199)
float CubicLerp(float x, V2 p0, V2 p1) {
    const float n = (x - p0.x) / (p1.x - p0.x);
    const float n2 = n * n;
    const float n3 = n * n2;
    return p0.y * (2.0f * n3 - 3.0f * n2 + 1.0f) + p1.y * (-2.0f * n3 + 3.0f * n2);
}
// TireFriction.LinealFriction.FastLerp (TireFriction.cs:146-149)
float FastLerp(V2 p0, V2 p1, float x) { return p0.y + (x - p0.x) * (p1.y - p0.y) / (p1.x - p0.x); }

// Wheel.SolveSymmetricConstraint (Wheel.cs:260-272)
float SolveSymmetricConstraint(float tsum, float t, float tmax) {
    const bool flag = tsum >= 0.0f;
    const bool flag2 = tmax >= 0.0f;
    const float num = flag ? tsum : -tsum;
    const float num2 = flag2 ? tmax : -tmax;
    if (tmax == 0.0f || (flag == flag2 && num > num2)) {
        const float k = InverseLerp(0.0f, tsum * 0.5f, t);
        tsum = tsum + (tmax - tsum) * k;  // MathUtility.UnclampedLerp
    }
    return tsum;
}

double RoundDigits(double v, int digits) {  // Math.Round(double, int): to even at the digit
    const double p = std::pow(10.0, digits);
    return std::nearbyint(v * p) / p;
}

Vec3 Project(Vec3 v, Vec3 n) {  // Vector3.Project
    const float d = n.Dot(n);
    if (d < 1e-15f) return Vec3::sZero();
    return n * (v.Dot(n) / d);
}

}  // namespace

// ---- TireFriction ---------------------------------------------------------------------------------------------------------
float TireFriction::EvaluateForce(float s, float load, float grip) const {
    const float num = load * grip;
    if (s <= adherent.x) return num * adherent.y;
    if (model == Model::Smooth) {
        if (s < peak.x) return num * CubicLerp(s, adherent, peak);
        if (s < limit.x) return num * CubicLerp(s, peak, limit);
        return num * limit.y;
    }
    if (s < peak.x) return num * FastLerp(adherent, peak, s);
    if (s < limit.x) return num * FastLerp(peak, limit, s);
    return num * limit.y;
}

V2 TireFriction::GetForce(V2 slip, float load, float grip) const {
    const float mag = slip.magnitude();
    if (mag < 1e-6f) return V2(0.0f, 0.0f);
    slip.x /= mag;
    slip.y /= mag;
    return (-EvaluateForce(mag, load, grip)) * slip * frictionMultiplier;
}

float TireFriction::GetAdherentSlipForward(V2 slip) const {
    const float x = slip.x;
    const float a = AdherentSlip();
    if (!(std::fabs(x) < a)) return 0.0f;
    return std::sqrt(a * a - x * x);
}

float TireFriction::GetAdherentSlipSideways(V2 slip) const {
    const float y = slip.y;
    const float a = AdherentSlip();
    if (!(std::fabs(y) < a)) return 0.0f;
    return std::sqrt(a * a - y * y);
}

V2 TireFriction::GetAdherentForce(V2 slip, float load, float grip) const {
    const float a = AdherentSlip();
    return (-(EvaluateForce(a, load, grip) * frictionMultiplier / a)) * slip;
}

// ---- VPP Wheel + DirectDrive under Solver.IntegrateEuler ------------------------------------------------------------------
void VppWheelBlock::RecalculateConstants() {
    I = mass * radius * radius * 0.5f;
    if (I < 0.01f) I = 0.01f;
    invI = 1.0f / I;
}

void VppWheelBlock::AddBrakeTorque(float t) {
    brakeTorqueAcc += t;
    if (brakeTorqueAcc < 0.0f) brakeTorqueAcc = 0.0f;
}

void VppWheelBlock::ComputeStateUpstream() {
    forwardSlip = static_cast<float>(static_cast<double>(w) * static_cast<double>(radius) - static_cast<double>(V.y));
    const V2 slip(V.x, forwardSlip);
    contactSlip = slip;
    tireForce = tireFriction->GetForce(contactSlip, load, grip);
    Ty = tireForce.y * radius;
    isAdherent = tireFriction->IsAdherentSlip(contactSlip);
    if (isAdherent) {
        adherentSlip.x = slip.x;
        adherentSlip.y = tireFriction->GetAdherentSlipForward(contactSlip);
        contactSlip = adherentSlip;
        contactSlip.y *= Sign(slip.y);
        adherentForce = tireFriction->GetAdherentForce(contactSlip, load, grip);
        const float num = adherentForce.y * radius;
        if (std::fabs(Ty) < std::fabs(num)) Ty = num;
    } else {
        adherentSlip = V2();
        adherentForce = V2();
    }
    connL = L;
    connI = I;
    connTr = Tr;
}

void VppWheelBlock::DirectDriveEvaluate(float dt) {
    const float value = motorInput * maxMotorTorque;
    const float num = connI * std::fabs(maxRpm) * kRpmToW;
    const float mx = (num - connL) / dt - connTr * damping;
    const float mn = (-num - connL) / dt - connTr * damping;
    connOutTd = Clamp(value, mn, mx);
}

void VppWheelBlock::EvaluateTorqueDownstream(float dt) {
    inTd = connOutTd;
    inTb = brakeTorqueAcc;
    const float num = inTb * Sign(0.0f - L);
    float tsum = inTd + Ty + num;
    const float num2 = (0.0f - L) / dt;
    const float num3 = static_cast<float>(static_cast<double>(V.y) / static_cast<double>(radius) - static_cast<double>(w)) * I / dt;
    const float num4 = std::fabs(num2);
    const float num5 = std::fabs(num3);
    if (!(num4 < num5) && std::fabs(num5 - num4) >= 1e-6f) {
        tsum = SolveSymmetricConstraint(tsum, Ty, num3);
        tsum = SolveSymmetricConstraint(tsum, num, num2);
    } else {
        tsum = SolveSymmetricConstraint(tsum, num, num2);
        tsum = SolveSymmetricConstraint(tsum, Ty, num3);
    }
    T = tsum;
    Tr = T - inTd;
    float num6 = Clamp(Fext.y * radius - inTd, -inTb, inTb) + inTd;
    float num9;
    if (num3 <= 0.0f) {
        const float num7 = std::max(Ty, num3);
        if (num6 < 0.0f) {
            const float num8 = Ty * InverseLerp(adherentSlip.y * Sign(forwardSlip), 0.0f, forwardSlip);
            if (num6 < num8) num6 = num8;
            num9 = num6 - num7;
        } else {
            num9 = Clamp(num6, -num7, -Ty);
        }
    } else {
        const float num7 = std::min(Ty, num3);
        if (num6 > 0.0f) {
            const float num10 = Ty * InverseLerp(adherentSlip.y * Sign(forwardSlip), 0.0f, forwardSlip);
            if (num6 > num10) num6 = num10;
            num9 = num6 - num7;
        } else {
            num9 = Clamp(num6, -Ty, -num7);
        }
    }
    F.y = num9 / radius;
    float x;
    if (isAdherent) {
        adherentSlip.y = adherentSlip.y * F.y / adherentForce.y;
        contactSlip = adherentSlip;
        adherentSlip.x = tireFriction->GetAdherentSlipSideways(contactSlip);
        contactSlip = adherentSlip;
        adherentForce = tireFriction->GetAdherentForce(contactSlip, load, grip);
        x = adherentForce.x;
    } else {
        x = tireForce.x;
    }
    x = std::fabs(x);
    F.x = Clamp(Fext.x, -x, x);
    isResting = isResting && x > std::fabs(Fext.x);
    isResting = isResting && inTb > std::fabs(Fext.y * radius - inTd);
}

void VppWheelBlock::Integrate(float dt, int subSteps) {
    // GetState (Wheel.cs:113-119)
    float sL = L;
    V2 sP;
    float sLr = 0.0f;
    isResting = true;
    const float h = dt / static_cast<float>(subSteps);
    for (int j = 0; j < subSteps; j++) {
        // ComputeDerivative: SetSubstepState + ComputeStateUpstream in block order, then EvaluateTorqueDownstream in reverse
        L = sL;
        P = sP;
        w = L * invI;
        ComputeStateUpstream();
        DirectDriveEvaluate(h);
        EvaluateTorqueDownstream(h);
        // EulerStep (Solver.cs:154-159)
        sL = sL + T * h;
        sP = sP + F * h;
        sLr = sLr + Tr * h;
    }
    // SetState (Wheel.cs:251-258), Solver.deltaTime = dt
    L = sL;
    F = sP / dt;
    Tr = sLr / dt;
    w = L * invI;
    brakeTorqueAcc = 0.0f;
}

// ---- configuration --------------------------------------------------------------------------------------------------------
namespace {
template <class T>
bool TryGet(const ksp::PartModule& m, const char* name, T& out) {
    const ksp::ModuleValue* v = m.Value(name);
    if (!v) return false;
    if (const T* p = std::get_if<T>(v)) {
        out = *p;
        return true;
    }
    return false;
}
float F(const ksp::PartModule& m, const char* name, float def) {
    float v = def;
    TryGet(m, name, v);
    return v;
}
bool B(const ksp::PartModule& m, const char* name, bool def) {
    bool v = def;
    TryGet(m, name, v);
    return v;
}
int I32(const ksp::PartModule& m, const char* name, int def) {
    int32_t v = def;
    TryGet(m, name, v);
    return v;
}
}  // namespace

bool ReadWheelConfig(const ksp::Part& p, WheelConfig& c) {
    const ksp::PartModuleList& ml = p.Modules();
    const ksp::PartModule* base = ml.GetModule("ModuleWheelBase");
    if (!base) return false;
    c.partName = p.partInfo ? p.partInfo->name : p.gameObjectName;
    c.rescaleFactor = p.fields.rescaleFactor;
    if (const ksp::ModuleValue* v = base->Value("wheelType"))
        if (const ksp::EnumValue* e = std::get_if<ksp::EnumValue>(v)) c.wheelType = static_cast<int>(e->value);
    c.radius = F(*base, "radius", 0.0f);
    ksp::Vector3f ctr;
    if (TryGet(*base, "center", ctr)) c.center = Vec3(ctr.x, ctr.y, ctr.z);
    c.wcMass = F(*base, "mass", 0.0f);
    c.frictionSharpness = F(*base, "frictionSharpness", 0.0f);
    c.wheelDamping = F(*base, "wheelDamping", 0.05f);
    c.wheelMaxSpeed = F(*base, "wheelMaxSpeed", 1000.0f);
    c.adherentStart = F(*base, "adherentStart", 0.5f);
    c.frictionAdherent = F(*base, "frictionAdherent", 0.25f);
    c.peakStart = F(*base, "peakStart", 4.0f);
    c.frictionPeak = F(*base, "frictionPeak", 1.45f);
    c.limitStart = F(*base, "limitStart", 7.0f);
    c.frictionLimit = F(*base, "frictionLimit", 1.1f);
    c.autoFrictionAvailable = B(*base, "autoFrictionAvailable", true);
    c.autoFriction = B(*base, "autoFriction", true);
    c.useNewFrictionModel = B(*base, "useNewFrictionModel", false);
    c.frictionMultiplier = F(*base, "frictionMultiplier", 1.0f);
    c.geeBias = F(*base, "geeBias", 1.6f);
    c.minimumDownforce = F(*base, "minimumDownforce", 0.5f);
    c.tireForceSharpness = F(*base, "tireForceSharpness", 10.0f);
    c.suspensionForceSharpness = F(*base, "suspensionForceSharpness", 10.0f);
    c.springSlerpRate = F(*base, "springSlerpRate", 0.02f);
    c.groundHeightOffset = F(*base, "groundHeightOffset", 0.0f);
    c.activeSubsteps = I32(*base, "activeSubsteps", -1);
    c.inactiveSubsteps = I32(*base, "inactiveSubsteps", -1);
    if (const ksp::PartModule* s = ml.GetModule("ModuleWheelSuspension")) {
        c.hasSuspension = true;
        c.suspensionDistance = F(*s, "suspensionDistance", 0.0f);
        c.suspensionOffset = F(*s, "suspensionOffset", 0.0f);
        c.targetPosition = F(*s, "targetPosition", 1.0f);
        c.springRatio = F(*s, "springRatio", 50.0f);
        c.springTweakable = F(*s, "springTweakable", 1.0f);
        c.damperRatio = F(*s, "damperRatio", 1.0f);
        c.damperTweakable = F(*s, "damperTweakable", 1.0f);
        c.autoSpringDamper = B(*s, "autoSpringDamper", true);
        c.boostRatio = F(*s, "boostRatio", 0.0f);
        c.maximumLoad = F(*s, "maximumLoad", 0.0f);
        c.useDistributedMass = B(*s, "useDistributedMass", false);
        c.autoBoost = F(*s, "autoBoost", 0.0f);
        c.useAutoBoost = B(*s, "useAutoBoost", true);
        c.adjustForHighGee = B(*s, "adjustForHighGee", true);
        c.highGeeThreshold = F(*s, "highGeeThreshold", 1.5f);
        c.highGeeSpringTweakable = F(*s, "highGeeSpringTweakable", 3.0f);
        c.highGeeDamperTweakable = F(*s, "highGeeDamperTweakable", 1.2f);
    }
    const ksp::PartModule* mot = ml.GetModule("ModuleWheelMotor");
    if (!mot) mot = ml.GetModule("ModuleWheelMotorSteering");
    if (mot) {
        c.hasMotor = true;
        c.driveResponse = F(*mot, "driveResponse", 3.0f);
        c.wheelSpeedMax = F(*mot, "wheelSpeedMax", 120.0f);
        c.driveLimiter = F(*mot, "driveLimiter", 100.0f);
        c.tractionControlScale = F(*mot, "tractionControlScale", 1.0f);
        c.autoTorque = B(*mot, "autoTorque", true);
        c.motorEnabled = B(*mot, "motorEnabled", true);
        c.motorInverted = B(*mot, "motorInverted", false);
        TryGet(*mot, "torqueCurve", c.torqueCurve);
    }
    if (const ksp::PartModule* b = ml.GetModule("ModuleWheelBrakes")) {
        c.hasBrakes = true;
        c.maxBrakeTorque = F(*b, "maxBrakeTorque", 2000.0f);
        c.brakeResponse = F(*b, "brakeResponse", 10.0f);
        c.brakeTweakable = F(*b, "brakeTweakable", 50.0f);
    }
    if (const ksp::PartModule* l = ml.GetModule("ModuleWheelLock")) {
        c.hasLock = true;
        c.lockTorque = F(*l, "maxTorque", 1000.0f);
    }
    if (const ksp::PartModule* d = ml.GetModule("ModuleWheelDamage")) {
        c.hasDamage = true;
        c.stressTolerance = F(*d, "stressTolerance", 0.0f);
        c.impactTolerance = F(*d, "impactTolerance", 0.0f);
        c.deflectionMagnitude = F(*d, "deflectionMagnitude", 0.0f);
        c.deflectionSharpness = F(*d, "deflectionSharpness", 2.0f);
        c.slipMagnitude = F(*d, "slipMagnitude", 0.0f);
        c.slipSharpness = F(*d, "slipSharpness", 2.0f);
    }
    c.hasDeployment = ml.GetModule("ModuleWheelDeployment") != nullptr;
    return true;
}

// ---- KspWheel -------------------------------------------------------------------------------------------------------------
void KspWheel::Setup(const WheelConfig& c, float geeASL, bool activeVessel) {
    cfg = c;
    // ModuleWheelBase.OnStart (:256-274)
    const int activeSubsteps = cfg.activeSubsteps < 0 ? 8 : cfg.activeSubsteps;      // GameSettings.WHEEL_SUBSTEPS_ACTIVE
    const int inactiveSubsteps = cfg.inactiveSubsteps < 0 ? 4 : cfg.inactiveSubsteps;  // WHEEL_SUBSTEPS_INACTIVE
    float minimumDownforce = cfg.minimumDownforce;
    if (cfg.wheelType == 2) minimumDownforce = 10.0f;  // LEG (:271-274)
    // OnStart -> ActionUIUpdate (:1069-1103): no auto friction -> autoFriction false, frictionMultiplier 1
    bool autoFriction = cfg.autoFriction;
    float frictionMultiplier = cfg.frictionMultiplier;
    if (!cfg.autoFrictionAvailable) {
        autoFriction = false;
        frictionMultiplier = 1.0f;
    }
    // wheelSetup (:277-374)
    minimumdownForce = minimumDownforce;
    springSlerpRate_ = cfg.springSlerpRate;
    wcMass = cfg.wcMass;
    integrationSteps = activeVessel ? activeSubsteps : inactiveSubsteps;
    suspensionDistance = 0.02f;
    suspensionAnchor = 0.5f;
    springRate = 10000.0f;
    damperRate = 1.0f;
    tireForceSmoothing = true;
    tireForceSharpness = cfg.tireForceSharpness;
    if (cfg.frictionSharpness > 0.0f) {
        tireSideDeflection = true;
        tireSideDeflectionRate = cfg.frictionSharpness;
    }
    wheelDamping = 1.0f - cfg.wheelDamping;
    maxRpm = SpeedToRpm(cfg.wheelMaxSpeed, cfg.radius);
    wcRadius = cfg.radius * cfg.rescaleFactor;
    wcCenter = cfg.center * cfg.rescaleFactor;
    // OnWheelSetup of the submodules (ModuleWheelBase.cs:709-716: last registered first)
    if (cfg.hasLock) {  // ModuleWheelLock.cs:8-13
        maxBrakeTorque = cfg.lockTorque;
        brakeResponse = 1000.0f;
        brakeInputW = 1.0f;
    }
    if (cfg.hasBrakes) {  // ModuleWheelBrakes.cs:55-63
        maxBrakeTorque = cfg.maxBrakeTorque;
        brakeResponse = cfg.brakeResponse;
    }
    if (cfg.hasMotor) {  // ModuleWheelMotor.cs:122-129
        float mn = 0.0f, mx = 0.0f;
        cfg.torqueCurve.FindMinMaxValue(mn, mx);
        motorMaxTorque = mx;
        maxDriveTorque = motorMaxTorque;
        maxDriveTorque = static_cast<float>(std::nearbyint(static_cast<double>(maxDriveTorque * 100.0f))) / 100.0f;
        maxRpm = SpeedToRpm(cfg.wheelSpeedMax, cfg.radius);
        driveResponse = cfg.driveResponse;
    }
    if (cfg.hasSuspension) {  // ModuleWheelSuspension.cs:289-302 (spring / damper are the module's private fields: 0 at first)
        suspensionDistance = cfg.suspensionDistance * cfg.rescaleFactor;
        suspensionAnchor = cfg.targetPosition;
        springRate = spring;
        damperRate = damper;
    }
    springTweakable = cfg.springTweakable;
    damperTweakable = cfg.damperTweakable;
    autoBoost = cfg.autoBoost;
    driveLimiter = cfg.driveLimiter;
    // ApplyGeeBias(GeeASL) (:355, :376-409) + UpdateFriction
    if (autoFriction) {
        if (cfg.useNewFrictionModel) {
            frictionMultiplier = Clamp(geeASL * cfg.geeBias + 0.4f, 0.4f, 10.0f);
        } else {
            float gb = cfg.geeBias;
            if (gb == 1.6f) gb = 1.0f;
            frictionMultiplier = Clamp(1.0f / geeASL * gb + 1.0f, 0.5f, 10.0f);
        }
    }
    tire.adherent = V2(cfg.adherentStart, cfg.frictionAdherent * frictionMultiplier);
    tire.peak = V2(cfg.peakStart, cfg.frictionPeak * frictionMultiplier);
    tire.limit = V2(cfg.limitStart, cfg.frictionLimit * frictionMultiplier);
    // Wheel.Initialize -> OnInitialize (KSPWheelController.cs:210-228), VehicleBase.OnEnable (:851-920)
    block.tireFriction = &tire;
    block.radius = wcRadius;
    block.mass = wcMass;
    block.RecalculateConstants();
    block.L = 0.0f;
    block.w = 0.0f;
    // VPWheelCollider.OnEnableVehicle (:190-211)
    runtimeSpringRate = springRate;
    runtimeDamperRate = damperRate;
    runtimeSuspensionTravel = suspensionDistance;
    runtimeExtraDownforce = 0.0f;
    lastRuntimeSpringRate = runtimeSpringRate;
    lastRuntimeDamperRate = runtimeDamperRate;
    lastRuntimeSuspensionTravel = runtimeSuspensionTravel;
    nativeSpring = NativeSpring();
    nativeSpring.suspensionDistance = suspensionDistance;
}

float KspWheel::LegacyWheelLoad() const {
    if (!hit.grounded) return 0.0f;
    return std::max(0.0f, contactDepth * nativeSpring.spring * (1.0f / gravityMag));
}

void KspWheel::WheelBaseFixedUpdate(const VesselInputs& in) {
    // ModuleWheelBase.FixedUpdate (:421-463)
    slipDisplacement.x = localWheelVelocity.x;
    slipDisplacement.y = angularVelocity * cfg.radius - localWheelVelocity.y;  // cfg radius: unscaled (rescaleFactor not applied)
    slipDisplacement = slipDisplacement * in.dt;                                // TimeWarp.deltaTime
    isGrounded = hit.grounded;
    // (autoFriction == false -> UpdateFriction every tick: the values do not change in the spike)
    const float gm = in.gravityForPos.Length();  // KSPWheelGravity.Refresh
    gravityMag = gm;
    gravityUp = gm > 0.0f ? -in.gravityForPos / gm : Vec3::sAxisY();
    maxRpm = SpeedToRpm(cfg.wheelMaxSpeed, cfg.radius);
    wheelDamping = 1.0f - cfg.wheelDamping;
}

float KspWheel::BoostCurve(float b, float power) const {
    const float p = std::pow(Clamp(b, 0.0f, 2.0f), Clamp(power, 0.01f, autoBoostClamp_));
    return Round(Clamp(1.0f / std::fabs(1.0f - 2.0f / p), 0.01f, 10000000.0f) * 10.0f) / 10.0f;
}

void KspWheel::UpdateAutoBoost(float st, float fixedDt) {
    if (!cfg.useAutoBoost) autoBoost = 0.0f;
    else if (st > 0.8f) autoBoost = Clamp(autoBoost + 0.2f * fixedDt, 0.85f, 2.0f);
    else if (st < 0.4f) autoBoost = std::max(0.0f, autoBoost - 0.2f * fixedDt);
}

void KspWheel::SuspensionFixedUpdate(const VesselInputs& in) {
    if (cfg.hasSuspension && hit.grounded) SuspensionSpringUpdate(static_cast<float>(in.totalMass), in);
}

void KspWheel::SuspensionSpringUpdate(float sprungMass, const VesselInputs& in) {
    const float suspCompression = suspensionCompression;
    const float num = sprungMass * suspCompression;
    springClampMax = Lerp(5000.0f, 10000.0f, InverseLerp(50.0f, 200.0f, sprungMass));
    damperClampMax = Lerp(5000.0f, 10000.0f, InverseLerp(50.0f, 200.0f, sprungMass));
    UpdateAutoBoost(suspCompression, in.dt);
    boost = 1.0f + Clamp(suspCompression * BoostCurve(static_cast<float>(in.geeASL) * static_cast<float>(in.gravityMultiplier), 1.0f), -1.0f,
                         boostClamp_);
    if (!cfg.useDistributedMass) sprungMassGravity = num;
    if (cfg.autoSpringDamper && (!autoSpringDamperSet || (cfg.maximumLoad > 0.0f && num > cfg.maximumLoad))) {
        if (in.geeASL < 1.0) {
            damperTweakable = 1.1f;
        } else {
            const float t = InverseLerp(damperLerpBase_, static_cast<float>(in.maximumSurfaceGeeASL), static_cast<float>(in.geeASL));
            damperTweakable = static_cast<float>(RoundDigits(Lerp(1.0f, 2.0f, t), 2));
        }
        if (spring > 0.0f && (!autoSpringDamperSet || (cfg.maximumLoad > 0.0f && num > cfg.maximumLoad))) {
            const float value = cfg.suspensionDistance * spring / in.gravitationalAcceleration;
            const float t2 = InverseLerp(0.0f, cfg.maximumLoad * 2.0f, value);
            springTweakable = static_cast<float>(RoundDigits(Lerp(1.0f, 3.0f, t2), 2));
            autoSpringDamperSet = true;
        }
    }
    damperFudge = Lerp(1.0f, 15.0f, InverseLerp(10.0f, 100.0f, num));
    spring = Clamp(springTweakable * cfg.springRatio * sprungMassGravity * BoostCurve(boost, cfg.boostRatio + autoBoost), 0.01f, springClampMax);
    damper = std::sqrt(Clamp(spring * cfg.damperRatio * damperFudge, 0.01f, damperClampMax)) * damperTweakable;
    if (cfg.adjustForHighGee && in.geeASL > static_cast<double>(cfg.highGeeThreshold)) {
        spring *= cfg.highGeeSpringTweakable;
        damper *= cfg.highGeeDamperTweakable;
    }
    damper = std::max(Round(damper), 0.1f);
    spring = std::max(Round(spring), 0.1f);
    if (spring - springRate > oscillationDamper_) springRate = spring;
    else if (springRate > spring + oscillationDamper_) springRate = spring;
    if (std::fabs(damper - damperRate) > 1.05f) damperRate = damper;
}

void SuspensionLoadBalancerFixedUpdate(KspWheel* wheels, int count, const VesselInputs& in) {
    double num = in.totalMass;
    if (in.landed) num = std::nearbyint(num * in.gravityMultiplier * in.geeASL * 10.0) / 10.0;
    int grounded = 0;
    for (int i = 0; i < count; i++)
        if (wheels[i].cfg.hasSuspension && wheels[i].hit.grounded) grounded++;
    for (int j = 0; j < count; j++) {
        if (!wheels[j].cfg.hasSuspension) continue;
        const float num3 = Round(100.0f / (static_cast<float>(grounded) > 0.0f ? static_cast<float>(grounded) : 1.0f)) / 100.0f;
        wheels[j].sprungMassGravity = std::max(static_cast<float>(num * static_cast<double>(num3) * 1.0), 0.5f);
    }
}

void KspWheel::MotorFixedUpdate(const VesselInputs& in) {
    if (!cfg.hasMotor) return;
    if (cfg.motorEnabled) {
        float di = Clamp(in.wheelThrottle + in.wheelThrottleTrim, -1.0f, 1.0f);
        di *= 1.0f;                               // GetMotorOrientationSign(): +1 for the spike's straight wheels
        di *= cfg.motorInverted ? -1.0f : 1.0f;
        maxDriveTorque = cfg.torqueCurve.Evaluate(std::fabs(angularVelocity * cfg.radius));
        maxDriveTorque = static_cast<float>(std::nearbyint(static_cast<double>(maxDriveTorque * 100.0f))) / 100.0f;
        di = Clamp(di, -1.0f, 1.0f);
        if (cfg.autoTorque) {
            di *= 1.0f - std::fabs(slipDisplacement.y * cfg.tractionControlScale);
            driveLimiter = Lerp(1.0f, static_cast<float>(in.geeASL), Clamp01(cfg.tractionControlScale / 2.0f)) * 100.0f;
        }
        di *= driveLimiter * 0.01f;
        motorDriveInput = di;
        if (motorMaxTorque > 0.0f) driveInput = di * 1.0f;  // CheckMotorState: electric charge available, resourceFraction 1
    } else {
        motorDriveInput = 0.0f;
        driveInput = 0.0f;
    }
}

void KspWheel::BrakesFixedUpdate(const VesselInputs& in) {
    if (!cfg.hasBrakes) return;
    brakeInputModule = in.brakeInput;
    const float num = std::min(1.0f, Lerp(1.0f, static_cast<float>(in.geeASL), std::fabs(angularVelocity)));
    brakeInputW = brakeInputModule * (cfg.brakeTweakable * 0.01f) * num;
    brakeResponse = brakeInputW != 0.0f ? cfg.brakeResponse : 100.0f;
}

float KspWheel::GetContactDepth(JPH::RVec3Arg contactPoint, float suspensionTravel) const {
    const float num = Vec3(transformPos - contactPoint).Dot(up);  // -InverseTransformPoint(contactPoint).y (unit-scale transform)
    return suspensionTravel + wcRadius - (num + suspensionDistance * (1.0f - suspensionAnchor));
}

bool KspWheel::VehicleFixedUpdate(const VesselInputs& in, const BodyView& body, Vec3& outForce, JPH::RVec3& outPoint) {
    const float dt = in.dt;
    // CalculateSuspensionTravel (VehicleBase.cs:697-713) with VPWheelCollider.GetGroundHit (:313-349): steer angle 0
    const bool grounded = hit.grounded;
    if (grounded) {
        const Vec3 v = forward;
        const float num = -v.Dot(hit.normal);
        const float num2 = up.Dot(hit.normal);
        if (num2 > 1e-6f) forwardDir = (v + up * (num / num2)).Normalized();
        else forwardDir = num >= 0.0f ? up : -up;
        sidewaysDir = hit.normal.Cross(forwardDir);
    }
    lastContactDepth = contactDepth;
    if (grounded) {
        contactDepth = GetContactDepth(hit.point, lastRuntimeSuspensionTravel);
        suspensionCompression = contactDepth / lastRuntimeSuspensionTravel;
    } else {
        contactDepth = 0.0f;
        suspensionCompression = 0.0f;
        hit.force = 0.0f;
    }
    const float referenceDownforce = body.partRbMass * gravityMag / 1.0f;
    // CalculateSuspensionForces (:715-744), advancedSuspensionDamper false
    if (grounded) {
        contactDepth += runtimeSuspensionTravel - lastRuntimeSuspensionTravel;
        suspensionCompression = contactDepth / runtimeSuspensionTravel;
        contactSpeed = (contactDepth - lastContactDepth) / dt;
        damperForce = hit.force - contactDepth * runtimeSpringRate;
    } else {
        contactSpeed = 0.0f;
        damperForce = 0.0f;
    }
    nativeSpring.forceAppPointDistance = contactDepth - lastRuntimeSuspensionTravel * nativeSpring.targetPosition;  // SetSuspensionForceOffset
    // CalculateDownforce (:746-771)
    if (grounded) {
        downforce = hit.force + runtimeExtraDownforce;
        const float y = up.Dot(sidewaysDir);
        const float x = up.Dot(hit.normal);
        contactAngle = std::atan2(y, x);
        downforce *= std::cos(contactAngle);
        if (downforce < minimumdownForce) downforce = minimumdownForce;
        normalizedLoad = hit.force / referenceDownforce;
    } else {
        contactAngle = 0.0f;
        downforce = 0.0f;
        normalizedLoad = 0.0f;
    }
    // CalculateLocalFrame (:773-825)
    if (!grounded) {
        wheelVelocity = Vec3::sZero();
        localWheelVelocity = V2();
        surfaceForce = Vec3::sZero();
        localSurfaceForce = V2();
        externalTireForce = V2();
    } else {
        Vec3 pv = body.PointVelocity(hit.point);  // static ground: no surface rigidbody
        wheelVelocity = pv - Project(pv, hit.normal);
        localWheelVelocity.y = forwardDir.Dot(wheelVelocity);
        localWheelVelocity.x = sidewaysDir.Dot(wheelVelocity);
        const float num = InverseLerp(1.0f, 0.25f, wheelVelocity.LengthSq());
        if (num > 0.0f) {
            const float num2 = gravityUp.Dot(hit.normal);
            if (num2 > 1e-6f) {
                const Vec3 vec = gravityUp * hit.force / num2;
                surfaceForce = vec - Project(vec, hit.normal);
            } else {
                surfaceForce = gravityUp * 100000.0f;
            }
            localSurfaceForce.y = forwardDir.Dot(surfaceForce);
            localSurfaceForce.x = sidewaysDir.Dot(surfaceForce);
            localSurfaceForce = localSurfaceForce * num;
        } else {
            surfaceForce = Vec3::sZero();
            localSurfaceForce = V2();
        }
        float num3 = (hit.force + runtimeExtraDownforce) * (1.0f / gravityMag);
        if (num3 < 0.0f) num3 = 0.0f;
        const V2 vector2 = (-tireImpulseRatio) * num3 * localWheelVelocity / dt;
        externalTireForce = vector2 + localSurfaceForce;
    }
    // block inputs (:975-986)
    block.tireFriction = &tire;  // (re)bound here: KspWheel objects are copied into containers
    block.load = downforce * 1.0f;
    block.grip = 1.0f;
    block.V = localWheelVelocity;
    block.Fext = externalTireForce * 1.0f;
    const float tmax = std::max(brakeTorque, downforce * 1.0f * block.radius);
    block.Tr = Clamp(reactionTorque, -tmax, tmax);
    // KSPWheelController.DoUpdateBlocks (:240-264); Time.deltaTime in FixedUpdate = fixedDeltaTime
    driveState = MoveTowards(driveState, driveInput, driveResponse * dt);
    brakeState = MoveTowards(brakeState, brakeInputW, brakeResponse * dt);
    block.motorInput = driveState;
    block.maxMotorTorque = maxDriveTorque;
    block.maxRpm = maxRpm;
    block.damping = wheelDamping;
    if (brakeState >= 1.0f) block.AddBrakeTorque(brakeState * maxBrakeTorqueRest);
    else block.AddBrakeTorque(brakeState * maxBrakeTorque);
    // Solver.Integrate (:996): explicit Euler, integrationSteps substeps
    block.Integrate(dt, integrationSteps);
    // results (:1008-1023)
    angularVelocity = block.w;
    tireForce = block.F * 1.0f;
    reactionTorque = block.Tr;
    tireSlip.x = localWheelVelocity.x;
    tireSlip.y = localWheelVelocity.y - angularVelocity * block.radius;
    driveTorque = block.inTd;
    brakeTorque = block.inTb;
    // ApplyTireForce (:827-849): no ground material -> drag 0
    bool applied = false;
    if (!grounded) {
        lastTireForce = V2();
    } else {
        if (tireSideDeflection) tireForce.x = Lerp(lastTireForce.x, tireForce.x, tireSideDeflectionRate * dt);
        if (tireForceSmoothing) {
            tireForce.x = Lerp(lastTireForce.x, tireForce.x, tireForceSharpness * dt);
            tireForce.y = Lerp(lastTireForce.y, tireForce.y, tireForceSharpness * dt);
        }
        lastTireForce = tireForce;
        outForce = forwardDir * tireForce.y + sidewaysDir * tireForce.x;
        outPoint = hit.point;
        applied = true;
    }
    // FixedUpdateVehicleBehaviours -> VPWheelCollider.FixedUpdateVehicle (:228-239)
    UpdateWheelCollider(in);
    lastRuntimeSpringRate = runtimeSpringRate;
    lastRuntimeDamperRate = runtimeDamperRate;
    lastRuntimeSuspensionTravel = runtimeSuspensionTravel;
    runtimeSpringRate = springRate;
    runtimeDamperRate = damperRate;
    runtimeSuspensionTravel = suspensionDistance;
    runtimeExtraDownforce = 0.0f;
    (void)body;
    return applied;
}

void KspWheel::UpdateWheelCollider(const VesselInputs&) {
    if (suspensionDistance < 0.01f) suspensionDistance = 0.01f;
    if (runtimeSuspensionTravel < 0.01f) runtimeSuspensionTravel = 0.01f;
    nativeSpring.suspensionDistance = runtimeSuspensionTravel;
    // WheelCollider.sprungMass: Unity distributes the attached Rigidbody's mass over ITS wheel colliders; KSP has one
    // WheelCollider per wheel-part Rigidbody, so it is that part's rb.mass (sprungMassUnity, set by the host).
    const float num = sprungMassUnity * gravityMag;
    const float num2 = std::max(num / nativeSpring.suspensionDistance, 0.1f);
    NativeSpring s = nativeSpring;
    s.spring = Clamp(runtimeSpringRate * 1.0f, num2, std::numeric_limits<float>::infinity());
    s.damper = std::max(runtimeDamperRate * 1.0f, 0.1f);
    s.targetPosition = Clamp(num / s.spring / nativeSpring.suspensionDistance, 0.1f, 1.0f);
    if (std::isnan(s.targetPosition)) return;
    if (std::fabs(s.targetPosition - nativeSpring.targetPosition) > springSlerpRate_) {
        if (nativeSpring.targetPosition > s.targetPosition) s.targetPosition = nativeSpring.targetPosition - springSlerpRate_;
        else s.targetPosition = nativeSpring.targetPosition + springSlerpRate_;
    }
    nativeSpring.spring = s.spring;
    nativeSpring.damper = s.damper;
    nativeSpring.targetPosition = s.targetPosition;
}

void KspWheel::DamageFixedUpdate(const VesselInputs& in) {
    if (!cfg.hasDamage) return;
    const float lwl = LegacyWheelLoad();
    if (!damageInitialized) {
        currentDeflection = cfg.deflectionMagnitude * lwl;
        currentSlip = cfg.slipMagnitude * lwl * slipDisplacement.magnitude();
        currentDownForce = downforce;
        lastDownForceDmg = currentDownForce;
        damageInitialized = true;
    }
    const float lastDeflection = currentDeflection, lastSlip = currentSlip;
    lastDownForceDmg = currentDownForce;
    currentDeflection = Lerp(lastDeflection, cfg.deflectionMagnitude * lwl, cfg.deflectionSharpness * in.dt);
    currentSlip = Lerp(lastSlip, cfg.slipMagnitude * lwl * slipDisplacement.magnitude(), cfg.slipSharpness * in.dt);
    currentDownForce = downforce;
    totalStress = Clamp(currentDeflection + currentSlip, 0.0f, cfg.stressTolerance);
    if (cfg.stressTolerance > 0.0f) maxStressRatio = std::max(maxStressRatio, (currentDeflection + currentSlip) / cfg.stressTolerance);
    const float impact = (currentDownForce - lastDownForceDmg) * gravityMag / in.gravitationalAcceleration;
    if (cfg.impactTolerance > 0.0f) {
        maxImpact = std::max(maxImpact, impact / cfg.impactTolerance);
        if (impact >= cfg.impactTolerance) impactsOverTolerance++;
    }
}

}  // namespace wspike
