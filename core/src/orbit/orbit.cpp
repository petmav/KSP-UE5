// Orbit (Assembly-CSharp/Orbit.cs), operation-for-operation port. See include/ksp/orbit/orbit.hpp for the frames and
// the list of KSP quirks kept. Every System.Math call is ksp::dotnet::Math; Math.Min/Max keep .NET's NaN rules.
// Parameter names that would hide a member (pos, vel, E, radius, ObT, semiMinorAxis) carry a trailing underscore.
#include "ksp/orbit/orbit.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"  // ksp::NullReferenceException
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/orbit/targeting.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/unity/random.hpp"

namespace ksp {

namespace M = dotnet::Math;

namespace {

constexpr double kPI = 3.141592653589793;                         // Math.PI
constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kFloatDeg2Rad = 0.01745329238474369;             // (double)Mathf.Deg2Rad, as KSP writes it

std::string Str(double v) { return dotnet::FormatDoubleG(v); }   // C# string concatenation of a double

// Math.Sign(double) throws ArithmeticException on NaN (both runtimes); solveEccentricAnomalyExtremeEcc catches it.
struct SignNaN {};
double SignOrThrow(double v) {
    if (std::isnan(v)) throw SignNaN{};
    return static_cast<double>(M::Sign(v));
}

const CelestialFrame& Zup() { return Planetarium::Zup; }

}  // namespace

// CelestialBody.cs:321 `body.orbit` (see orbit.hpp)
Orbit* BodyOrbit(const CelestialBody* body) {
    ICelestialBodyOrbit* link = body->orbit();
    return link != nullptr ? link->GetOrbit() : nullptr;
}

Orbit::FindClosestPointsDelegate Orbit::FindClosestPoints = &Orbit::FindClosestPointsDefault;
Orbit::SolveClosestApproachDelegate Orbit::SolveClosestApproach = &Orbit::SolveClosestApproachDefault;
Orbit::SolveSOI_Delegate Orbit::SolveSOI = &Orbit::SolveSOIDefault;
Orbit::SolveSOI_BSPDelegate Orbit::SolveSOI_BSP = &Orbit::SolveSOI_BSPDefault;
bool Orbit::LegacyOrbitTargeting = false;

// ---- CASolutionState ---------------------------------------------------------------------------------------------

// Orbit.cs:62
Orbit::CASolutionState::CASolutionState(const Orbit* p_, const Orbit* s_, double MaxDT_)
    : p(p_), s(s_), MaxDT(MaxDT_) {}

// Orbit.cs:81
void Orbit::CASolutionState::Update(double t, int& iter, bool dump) {
    iter++;
    rdv = RelativeStateAtUT(*p, *s, t, pstate, sstate, rstate, dump);
    drdv = Vector3d::Dot(rstate.vel, rstate.vel) + Vector3d::Dot(rstate.pos, rstate.acc);
    ddrdv = 3.0 * Vector3d::Dot(rstate.vel, rstate.acc) + Vector3d::Dot(rstate.pos, rstate.jrk);
    maxdt = 2.0 * rstate.pos.magnitude() / rstate.vel.magnitude();
    maxdt = M::Min(maxdt, MaxDT / 2.0);
}

// Orbit.cs:90
double Orbit::CASolutionState::Halley_dt() const {
    return -2.0 * rdv * drdv / (2.0 * drdv * drdv - rdv * ddrdv);
}

// Orbit.cs:95
double Orbit::CASolutionState::Clamp_dt(double dt) const { return M::Min(maxdt, M::Max(0.0 - maxdt, dt)); }

// ---- construction / elements ---------------------------------------------------------------------------------------

// Orbit.cs:316
Orbit::Orbit(double inc, double e, double sma, double lan, double argPe, double mEp, double t, CelestialBody* body) {
    SetOrbit(inc, e, sma, lan, argPe, mEp, t, body);
}

// Orbit.cs:321
Orbit Orbit::CopyOf(const Orbit& orbit) {
    Orbit o;
    o.inclination = orbit.inclination;
    o.eccentricity = orbit.eccentricity;
    o.semiMajorAxis = orbit.semiMajorAxis;
    o.LAN = orbit.LAN;
    o.argumentOfPeriapsis = orbit.argumentOfPeriapsis;
    o.meanAnomalyAtEpoch = orbit.meanAnomalyAtEpoch;
    o.epoch = orbit.epoch;
    o.referenceBody = orbit.referenceBody;
    o.Init();
    return o;
}

// Orbit.cs:282
double Orbit::semiMinorAxis() const {
    if (!(eccentricity < 1.0)) return semiMajorAxis * M::Sqrt(eccentricity * eccentricity - 1.0);
    return semiMajorAxis * M::Sqrt(1.0 - eccentricity * eccentricity);
}

// Orbit.cs:294
double Orbit::semiLatusRectum() const { return h.sqrMagnitude() / referenceBody->gravParameter; }
// Orbit.cs:296
double Orbit::PeR() const { return (1.0 - eccentricity) * semiMajorAxis; }
// Orbit.cs:298
double Orbit::ApR() const { return (1.0 + eccentricity) * semiMajorAxis; }
// Orbit.cs:300
double Orbit::PeA() const { return PeR() - referenceBody->Radius; }
// Orbit.cs:302
double Orbit::ApA() const { return ApR() - referenceBody->Radius; }

// Orbit.cs:334
double Orbit::GetMeanMotion(double sma) const {
    const double num = M::Abs(sma);
    return M::Sqrt(referenceBody->gravParameter / (num * num * num));
}

// Orbit.cs:340
void Orbit::SetOrbit(double inc, double e, double sma, double lan, double argPe, double mEp, double t,
                     CelestialBody* body) {
    inclination = inc;
    eccentricity = e;
    semiMajorAxis = sma;
    LAN = lan;
    argumentOfPeriapsis = argPe;
    meanAnomalyAtEpoch = mEp;
    epoch = t;
    referenceBody = body;
    Init();
}

// Orbit.cs:353
void Orbit::Init() {
    CelestialFrame::OrbitalFrame(LAN, inclination, argumentOfPeriapsis, OrbitFrame);
    an = Vector3d::Cross(Vector3d::forward(), OrbitFrame.Z);
    if (an.sqrMagnitude() == 0.0) an = Vector3d::right();
    eccVec = Zup().WorldToLocal(OrbitFrame.X * eccentricity);
    const double d = referenceBody->gravParameter * semiMajorAxis * (1.0 - eccentricity * eccentricity);
    h = Zup().WorldToLocal(OrbitFrame.Z * M::Sqrt(d));
    meanMotion = GetMeanMotion(semiMajorAxis);
    meanAnomaly = meanAnomalyAtEpoch;
    ObT = meanAnomaly / meanMotion;
    ObTAtEpoch = ObT;
    if (eccentricity < 1.0) {
        period = kPI * 2.0 / meanMotion;
        orbitPercent = meanAnomaly / (kPI * 2.0);
    } else {
        period = kInf;
        orbitPercent = 0.0;
    }
}

// Orbit.cs:380
double Orbit::SafeAcos(double c) {
    if (c > 1.0) c = 1.0;
    else if (c < -1.0) c = -1.0;
    return M::Acos(c);
}

// ---- updates -------------------------------------------------------------------------------------------------------

// Orbit.cs:393
void Orbit::UpdateFromOrbitAtUT(const Orbit& orbit, double UT, CelestialBody* toBody) {
    State state;
    State state2;
    if (orbit.referenceBody->HasChild(toBody)) {
        if (!std::isnan(orbit.GetOrbitalStateVectorsAtUT(UT, state)) &&
            !std::isnan(BodyOrbit(toBody)->GetOrbitalStateVectorsAtUT(UT, state2))) {
            pos = state.pos - state2.pos;
            vel = state.vel - state2.vel;
            UpdateFromFixedVectors(pos, vel, toBody, UT);
        }
    } else if (toBody->HasChild(orbit.referenceBody)) {
        if (!std::isnan(orbit.GetOrbitalStateVectorsAtUT(UT, state)) &&
            !std::isnan(BodyOrbit(orbit.referenceBody)->GetOrbitalStateVectorsAtUT(UT, state2))) {
            pos = state.pos + state2.pos;
            vel = state.vel + state2.vel;
            UpdateFromFixedVectors(pos, vel, toBody, UT);
        }
    } else {
        pos = (orbit.getTruePositionAtUT(UT) - toBody->getTruePositionAtUT(UT)).xzy();
        vel = orbit.getOrbitalVelocityAtUT(UT) + orbit.referenceBody->GetFrameVelAtUT(UT) - toBody->GetFrameVelAtUT(UT);
        UpdateFromStateVectors(pos, vel, toBody, UT);
    }
}

// Orbit.cs:423
void Orbit::UpdateFromStateVectors(Vector3d pos_, Vector3d vel_, CelestialBody* refBody, double UT) {
    pos_ = Zup().LocalToWorld(pos_);
    vel_ = Zup().LocalToWorld(vel_);
    UpdateFromFixedVectors(pos_, vel_, refBody, UT);
    if (std::isnan(argumentOfPeriapsis)) {
        // Quaternion.AngleAxis(0f - (float)LAN, Planetarium.up) * Planetarium.right: native AngleAxis, managed
        // Quaternion * Vector3 (Vector3d arguments narrowed to Vector3, the result widened back).
        const float angle = f32store(0.0 - fx(static_cast<float>(LAN)));
        const Vector3d lhs(UnityNative::AngleAxis(angle, Planetarium::up().ToVector3f()) *
                           Planetarium::right().ToVector3f());
        const Vector3d xzy = eccVec.xzy();
        const double num = Vector3d::Dot(lhs, xzy) / (lhs.magnitude() * xzy.magnitude());
        if (num > 1.0) argumentOfPeriapsis = 0.0;
        else if (num < -1.0) argumentOfPeriapsis = 180.0;
        else argumentOfPeriapsis = M::Acos(num);  // KSP quirk: radians, stored into a degrees field
    }
}

// Orbit.cs:448
void Orbit::UpdateFromFixedVectors(Vector3d pos_, Vector3d vel_, CelestialBody* refBody, double UT) {
    referenceBody = refBody;
    h = Vector3d::Cross(pos_, vel_);
    if (h.sqrMagnitude() == 0.0) {  // h.sqrMagnitude.Equals(0.0)
        inclination = M::Acos(pos_.z / pos_.magnitude()) * (180.0 / kPI);
        an = Vector3d::Cross(pos_, Vector3d::forward());
    } else {
        an = Vector3d::Cross(Vector3d::forward(), h);
        OrbitFrame.Z = h / h.magnitude();
        inclination = UtilMath::AngleBetween(OrbitFrame.Z, Vector3d::forward()) * (180.0 / kPI);
    }
    if (an.sqrMagnitude() == 0.0) an = Vector3d::right();
    LAN = M::Atan2(an.y, an.x) * (180.0 / kPI);
    LAN = std::fmod(LAN + 360.0, 360.0);
    eccVec = (Vector3d::Dot(vel_, vel_) / refBody->gravParameter - 1.0 / pos_.magnitude()) * pos_ -
             Vector3d::Dot(pos_, vel_) * vel_ / refBody->gravParameter;
    eccentricity = eccVec.magnitude();
    orbitalEnergy = vel_.sqrMagnitude() / 2.0 - refBody->gravParameter / pos_.magnitude();
    semiMajorAxis = (eccentricity < 1.0) ? ((0.0 - refBody->gravParameter) / (2.0 * orbitalEnergy))
                                         : ((0.0 - semiLatusRectum()) / (eccVec.sqrMagnitude() - 1.0));
    if (eccentricity == 0.0) {  // eccentricity.Equals(0.0)
        OrbitFrame.X = an.normalized();
        argumentOfPeriapsis = 0.0;
    } else {
        OrbitFrame.X = eccVec.normalized();
        argumentOfPeriapsis = UtilMath::AngleBetween(an, OrbitFrame.X);
        if (Vector3d::Dot(Vector3d::Cross(an, OrbitFrame.X), h) < 0.0) argumentOfPeriapsis = kPI * 2.0 - argumentOfPeriapsis;
    }
    if (h.sqrMagnitude() == 0.0) {
        OrbitFrame.Y = an.normalized();
        OrbitFrame.Z = Vector3d::Cross(OrbitFrame.X, OrbitFrame.Y);
    } else {
        OrbitFrame.Y = Vector3d::Cross(OrbitFrame.Z, OrbitFrame.X);
    }
    argumentOfPeriapsis *= 180.0 / kPI;
    meanMotion = GetMeanMotion(semiMajorAxis);
    const double x = Vector3d::Dot(pos_, OrbitFrame.X);
    const double y = Vector3d::Dot(pos_, OrbitFrame.Y);
    trueAnomaly = M::Atan2(y, x);
    eccentricAnomaly = GetEccentricAnomaly(trueAnomaly);
    meanAnomaly = GetMeanAnomaly(eccentricAnomaly);
    meanAnomalyAtEpoch = meanAnomaly;
    ObT = meanAnomaly / meanMotion;
    ObTAtEpoch = ObT;
    if (eccentricity < 1.0) {
        period = kPI * 2.0 / meanMotion;
        orbitPercent = meanAnomaly / (kPI * 2.0);
        orbitPercent = std::fmod(orbitPercent + 1.0, 1.0);
        timeToPe = std::fmod(period - ObT, period);
        timeToAp = timeToPe - period / 2.0;
        if (timeToAp < 0.0) timeToAp += period;
    } else {
        period = kInf;
        orbitPercent = 0.0;
        timeToPe = 0.0 - ObT;
        timeToAp = kInf;
    }
    radius = pos_.magnitude();
    altitude = radius - refBody->Radius;
    epoch = UT;
    pos = Zup().WorldToLocal(pos_);
    vel = Zup().WorldToLocal(vel_);
    h = Zup().WorldToLocal(h);
    debugPos = pos.ToVector3f();
    debugVel = vel.ToVector3f();
    debugH = h.ToVector3f();
    debugAN = an.ToVector3f();
    debugEccVec = eccVec.ToVector3f();
    OrbitFrameX = OrbitFrame.X.ToVector3f();
    OrbitFrameY = OrbitFrame.Y.ToVector3f();
    OrbitFrameZ = OrbitFrame.Z.ToVector3f();
}

// Orbit.cs:541
void Orbit::UpdateFromUT(double UT) {
    ObT = getObtAtUT(UT);
    meanAnomaly = ObT * meanMotion;
    eccentricAnomaly = solveEccentricAnomaly(meanAnomaly, eccentricity);
    trueAnomaly = GetTrueAnomaly(eccentricAnomaly);
    radius = GetOrbitalStateVectorsAtTrueAnomaly(trueAnomaly, UT, pos, vel);
    orbitalSpeed = vel.magnitude();
    orbitalEnergy = orbitalSpeed * orbitalSpeed / 2.0 - referenceBody->gravParameter / radius;
    altitude = radius - referenceBody->Radius;
    if (eccentricity < 1.0) {
        orbitPercent = meanAnomaly / (kPI * 2.0);
        orbitPercent = std::fmod(orbitPercent + 1.0, 1.0);
        timeToPe = std::fmod(period - ObT, period);
        timeToAp = timeToPe - period / 2.0;
        if (timeToAp < 0.0) timeToAp += period;
    } else {
        orbitPercent = 0.0;
        timeToPe = 0.0 - ObT;
        timeToAp = kInf;
    }
    debugPos = pos.ToVector3f();
    debugVel = vel.ToVector3f();
    debugH = h.ToVector3f();
    debugAN = an.ToVector3f();
    debugEccVec = eccVec.ToVector3f();
}

// ---- time <-> anomaly ----------------------------------------------------------------------------------------------

// Orbit.cs:575
double Orbit::GetDTforTrueAnomalyAtUT(double tA, double UT) const {
    const double obtAtTrueAnomaly = getObtAtTrueAnomaly(tA);
    const double obtAtUT = getObtAtUT(UT);
    if (std::isinf(obtAtTrueAnomaly) && std::isinf(obtAtUT)) return kNaN;
    if (std::isinf(obtAtTrueAnomaly)) return obtAtTrueAnomaly;
    if (std::isinf(obtAtUT)) return 0.0 - obtAtUT;
    double num = obtAtTrueAnomaly - obtAtUT;
    if (eccentricity < 1.0) num -= period * M::Floor(num / period + 0.5);
    return num;
}

// Orbit.cs:599
double Orbit::GetDTforTrueAnomaly(double tA, double wrapAfterSeconds) const {
    const double num = GetEccentricAnomaly(tA);
    const double num2 = GetMeanAnomaly(num);
    const double num3 = num2 / meanMotion;
    double num4;
    if (eccentricity < 1.0) {
        num4 = num3 - ObT;
        if (num4 < 0.0 - M::Abs(wrapAfterSeconds)) num4 += period;
    } else {
        num4 = num3 - ObT;
    }
    if (std::isnan(num4)) {
        log::Info("dT is NaN! tA: " + Str(tA) + ", E: " + Str(num) + ", M: " + Str(num2) + ", T: " + Str(num3) + "\n");
    }
    return num4;
}

// Orbit.cs:624
double Orbit::GetUTforTrueAnomaly(double tA, double wrapAfterSeconds) const {
    return epoch + (ObT - ObTAtEpoch) + GetDTforTrueAnomaly(tA, wrapAfterSeconds);
}

// Orbit.cs:649
double Orbit::getObtAtUT(double UT) const {
    double num;
    if (eccentricity < 1.0) {
        if (std::isinf(UT)) {
            log::Info("getObtAtUT infinite UT on elliptical orbit UT: " + Str(UT) + ", returning NaN\n");
            return kNaN;
        }
        num = std::fmod(UT - epoch + ObTAtEpoch, period);
        if (num > period / 2.0) num -= period;
    } else {
        if (std::isinf(UT)) return UT;
        num = ObTAtEpoch + (UT - epoch);
    }
    return num;
}

// Orbit.cs:676
double Orbit::getObTAtMeanAnomaly(double M_) const { return M_ / meanMotion; }

// Orbit.cs:681
double Orbit::getObtAtTrueAnomaly(double tA) const {
    const double e = GetEccentricAnomaly(tA);
    return GetMeanAnomaly(e) / meanMotion;
}

// Orbit.cs:746 (KSP quirk: a fresh, non-FINAL patch with StartUT = EndUT = 0 returns timeToPe - period)
double Orbit::GetTimeToPeriapsis() const {
    if (eccentricity < 1.0 && patchEndTransition != PatchTransitionType::FINAL && StartUT + timeToPe > EndUT) {
        return timeToPe - period;
    }
    return timeToPe;
}

// Orbit.cs:808
double Orbit::GetEccentricAnomaly(double tA) const {
    const double num = M::Cos(tA / 2.0);
    const double num2 = M::Sin(tA / 2.0);
    if (eccentricity < 1.0) {
        return 2.0 * M::Atan2(M::Sqrt(1.0 - eccentricity) * num2, M::Sqrt(1.0 + eccentricity) * num);
    }
    const double num3 = M::Sqrt((eccentricity - 1.0) / (eccentricity + 1.0)) * num2 / num;
    if (num3 >= 1.0) return kInf;
    if (num3 <= -1.0) return -kInf;
    return M::Log((1.0 + num3) / (1.0 - num3));
}

// Orbit.cs:828
double Orbit::GetMeanAnomaly(double E_) const {
    if (eccentricity < 1.0) return UtilMath::ClampRadiansTwoPI(E_ - eccentricity * M::Sin(E_));
    if (std::isinf(E_)) return E_;
    return eccentricity * M::Sinh(E_) - E_;
}

// Orbit.cs:841
double Orbit::RadiusAtTrueAnomaly(double tA) const {
    return semiLatusRectum() * (1.0 / (1.0 + eccentricity * M::Cos(tA)));
}

// Orbit.cs:846
double Orbit::TrueAnomalyAtRadiusSimple(double R) const {
    return M::Acos(semiLatusRectum() / (R * eccentricity) - 1.0 / eccentricity);
}

// Orbit.cs:851
double Orbit::TrueAnomalyAtRadius(double R) const { return trueAnomalyAtRadiusExtreme(R); }

// Orbit.cs:856
double Orbit::trueAnomalyAtRadiusExtreme(double R) const {
    const double num =
        Vector3d::Cross(getRelativePositionFromEccAnomaly(eccentricAnomaly), getOrbitalVelocityAtObT(ObT)).sqrMagnitude() /
        referenceBody->gravParameter;
    R = (!(eccentricity < 1.0)) ? M::Max(PeR(), R) : M::Min(M::Max(PeR(), R), ApR());
    return M::Acos(num / (eccentricity * R) - 1.0 / eccentricity);
}

// Orbit.cs:863
double Orbit::TrueAnomalyAtUT(double UT) const { return TrueAnomalyAtT(getObtAtUT(UT)); }

// Orbit.cs:868
double Orbit::TrueAnomalyAtT(double T) const {
    const double m = T * meanMotion;
    const double num = solveEccentricAnomaly(m, eccentricity);
    if (std::isnan(num)) return kNaN;
    return GetTrueAnomaly(num);
}

// Orbit.cs:879
double Orbit::EccentricAnomalyAtUT(double UT) const { return EccentricAnomalyAtObT(getObtAtUT(UT)); }

// Orbit.cs:884
double Orbit::EccentricAnomalyAtObT(double T) const {
    const double m = T * meanMotion;
    return solveEccentricAnomaly(m, eccentricity);
}

// Orbit.cs:890. KSP quirk: the branch and the value handed to the solvers are the eccentricity FIELD; `ecc` is unused.
double Orbit::solveEccentricAnomaly(double M_, double /*ecc*/, double maxError, int maxIterations) const {
    if (!(eccentricity < 1.0)) return solveEccentricAnomalyHyp(M_, eccentricity, maxError);
    if (!(eccentricity < 0.8)) return solveEccentricAnomalyExtremeEcc(M_, eccentricity, maxIterations);
    return solveEccentricAnomalyStd(M_, eccentricity, maxError);
}

// Orbit.cs:903
double Orbit::solveEccentricAnomalyStd(double M_, double ecc, double maxError) const {
    double num = 1.0;
    double num2 = M_ + ecc * M::Sin(M_) + 0.5 * ecc * ecc * M::Sin(2.0 * M_);
    while (M::Abs(num) > maxError) {
        const double num3 = num2 - ecc * M::Sin(num2);
        num = (M_ - num3) / (1.0 - ecc * M::Cos(num2));
        num2 += num;
    }
    return num2;
}

// Orbit.cs:916. Math.Sign(NaN) throws ArithmeticException; KSP catches it, prints it and returns NaN.
double Orbit::solveEccentricAnomalyExtremeEcc(double M_, double ecc, int iterations) const {
    try {
        double num = M_ + 0.85 * eccentricity * SignOrThrow(M::Sin(M_));
        for (int i = 0; i < iterations; i++) {
            const double num2 = ecc * M::Sin(num);
            const double num3 = ecc * M::Cos(num);
            const double num4 = num - num2 - M_;
            const double num5 = 1.0 - num3;
            const double num6 = num2;
            num += -5.0 * num4 / (num5 + SignOrThrow(num5) * M::Sqrt(M::Abs(16.0 * num5 * num5 - 20.0 * num4 * num6)));
        }
        return num;
    } catch (const SignNaN&) {
        log::Info("System.ArithmeticException: Function does not accept floating point Not-a-Number values.");
        return kNaN;
    }
}

// Orbit.cs:942
double Orbit::solveEccentricAnomalyHyp(double M_, double ecc, double maxError) const {
    if (std::isinf(M_)) return M_;
    double num = 1.0;
    const double num2 = 2.0 * M_ / ecc;
    double num3 = M::Log(M::Sqrt(num2 * num2 + 1.0) + num2);
    while (M::Abs(num) > maxError) {
        num = (eccentricity * M::Sinh(num3) - num3 - M_) / (eccentricity * M::Cosh(num3) - 1.0);
        num3 -= num;
    }
    return num3;
}

// Orbit.cs:959
double Orbit::GetTrueAnomaly(double E_) const {
    if (eccentricity < 1.0) {
        const double num = M::Cos(E_ / 2.0);
        const double num2 = M::Sin(E_ / 2.0);
        return 2.0 * M::Atan2(M::Sqrt(1.0 + eccentricity) * num2, M::Sqrt(1.0 - eccentricity) * num);
    }
    if (std::isinf(E_) && E_ > 0.0) return M::Acos(-1.0 / eccentricity);
    if (std::isinf(E_) && E_ < 0.0) return 0.0 - M::Acos(-1.0 / eccentricity);
    const double num3 = M::Sinh(E_ / 2.0);
    const double num4 = M::Cosh(E_ / 2.0);
    return 2.0 * M::Atan2(M::Sqrt(eccentricity + 1.0) * num3, M::Sqrt(eccentricity - 1.0) * num4);
}

// Orbit.cs:980
double Orbit::GetTrueAnomalyOfZupVector(const Vector3d& vector) const {
    const Vector3d lhs = Zup().LocalToWorld(vector);
    const double x = Vector3d::Dot(lhs, OrbitFrame.X);
    const double y = Vector3d::Dot(lhs, OrbitFrame.Y);
    return M::Atan2(y, x);
}

// Orbit.cs:1010
double Orbit::TimeOfTrueAnomaly(double tA, double UT) const {
    return getUTAtMeanAnomaly(GetMeanAnomaly(GetEccentricAnomaly(tA)), UT);
}

// Orbit.cs:1015
double Orbit::getUTAtMeanAnomaly(double M_, double UT) const {
    const double meanAnomalyAtUT = getMeanAnomalyAtUT(UT);
    double num = M_ - meanAnomalyAtUT;
    if (eccentricity < 1.0) num = UtilMath::ClampRadiansTwoPI(num);
    return UT + num / GetMeanMotion(semiMajorAxis);
}

// Orbit.cs:1026
double Orbit::getMeanAnomalyAtUT(double UT) const {
    double num = meanAnomalyAtEpoch + GetMeanMotion(semiMajorAxis) * (UT - epoch);
    if (eccentricity < 1.0) num = UtilMath::ClampRadiansTwoPI(num);
    return num;
}

// Orbit.cs:1286
double Orbit::GetNextPeriapsisTime(double UT) const {
    if (eccentricity < 1.0) return TimeOfTrueAnomaly(0.0, UT);
    return UT - getMeanAnomalyAtUT(UT) / GetMeanMotion(semiMajorAxis);
}

// Orbit.cs:1295
double Orbit::GetNextApoapsisTime(double UT) const {
    if (eccentricity < 1.0) return TimeOfTrueAnomaly(kPI, UT);
    return UT;
}

// Orbit.cs:1304
double Orbit::GetNextTimeOfRadius(double UT, double radius_) const {
    if (!(radius_ < PeR()) && (!(eccentricity < 1.0) || radius_ <= ApR())) {
        const double num = TrueAnomalyAtRadius(radius_);
        const double tA = UtilMath::TwoPI - num;
        const double num2 = TimeOfTrueAnomaly(num, UT);
        const double num3 = TimeOfTrueAnomaly(tA, UT);
        if (num3 < num2 && num3 > UT) return num3;
        return num2;
    }
    return UT;
}

// ---- positions and velocities --------------------------------------------------------------------------------------

// Orbit.cs:629
Vector3d Orbit::getPositionAtUT(double UT) const { return getPositionAtT(getObtAtUT(UT)); }

// Orbit.cs:634
Vector3d Orbit::getTruePositionAtUT(double UT) const {
    return getRelativePositionAtUT(UT).xzy() + referenceBody->getTruePositionAtUT(UT);
}

// Orbit.cs:639
Vector3d Orbit::getRelativePositionAtUT(double UT) const { return getRelativePositionAtT(getObtAtUT(UT)); }

// Orbit.cs:644
Vector3d Orbit::getOrbitalVelocityAtUT(double UT) const { return getOrbitalVelocityAtObT(getObtAtUT(UT)); }

// Orbit.cs:687
Vector3d Orbit::GetOrbitNormal() const { return Zup().WorldToLocal(OrbitFrame.Z); }
// Orbit.cs:692
Vector3d Orbit::GetEccVector() const { return Zup().WorldToLocal(OrbitFrame.X); }
// Orbit.cs:697
Vector3d Orbit::GetANVector() const { return Zup().WorldToLocal(an); }

// Orbit.cs:702
Vector3d Orbit::GetVel(const CelestialBody* activeVesselReferenceBody) const {
    const Vector3d vector3d =
        GetFrameVel() - (activeVesselReferenceBody ? activeVesselReferenceBody->GetFrameVel() : Vector3d::zero());
    return Vector3d(vector3d.x, vector3d.z, vector3d.y);
}

// Orbit.cs:708
Vector3d Orbit::GetRelativeVel() const { return vel.xzy(); }

// Orbit.cs:713
Vector3d Orbit::GetRotFrameVel(const CelestialBody& refBody) const {
    if (refBody.rotates && refBody.inverseRotation) return Vector3d::Cross(refBody.zUpAngularVelocity, -pos);
    return Vector3d::zero();
}

// Orbit.cs:722
Vector3d Orbit::GetRotFrameVelAtPos(const CelestialBody& refBody, const Vector3d& refPos) const {
    if (refBody.rotates && refBody.inverseRotation) return Vector3d::Cross(refBody.zUpAngularVelocity, -refPos);
    return Vector3d::zero();
}

// Orbit.cs:731
Vector3d Orbit::GetFrameVel() const { return vel + referenceBody->GetFrameVel(); }

// Orbit.cs:736
Vector3d Orbit::GetFrameVelAtUT(double UT) const {
    return getOrbitalVelocityAtUT(UT) + referenceBody->GetFrameVelAtUT(UT);
}

// Orbit.cs:741
Vector3d Orbit::GetWorldSpaceVel(const CelestialBody* activeVesselReferenceBody) const {
    return GetVel(activeVesselReferenceBody) -
           (referenceBody->inverseRotation ? referenceBody->getRFrmVel(pos + referenceBody->position) : Vector3d::zero());
}

// Orbit.cs:755
double Orbit::GetRadiusAtUT(double time) const {
    Vector3d vector3d;
    Vector3d unused;
    GetOrbitalStateVectorsAtUT(time, vector3d, unused);
    return vector3d.magnitude();
}

// Orbit.cs:761 (float Deg2Rad, sic)
double Orbit::GetRadiusAtPhaseAngle(double phaseAngle) const {
    const double num = (inclination < 90.0) ? 1 : (-1);
    return RadiusAtTrueAnomaly(phaseAngle - kFloatDeg2Rad * (LAN + argumentOfPeriapsis * num));
}

// Orbit.cs:767 (float Deg2Rad, sic)
double Orbit::PhaseAngle(double time, double longitude) const {
    if (!referenceBody->rotates && referenceBody->rotationPeriod != 0.0) {
        return kFloatDeg2Rad * (referenceBody->initialRotation + longitude) +
               kPI * 2.0 / referenceBody->rotationPeriod * time;
    }
    return kFloatDeg2Rad * (referenceBody->initialRotation + longitude);
}

// Orbit.cs:776
double Orbit::PhaseAngle(double time) const {
    const double num = (inclination < 90.0) ? 1 : (-1);
    double num2;
    for (num2 = kFloatDeg2Rad * (LAN + argumentOfPeriapsis * num) + TrueAnomalyAtUT(time) * num; num2 > kPI * 2.0;
         num2 -= kPI * 2.0) {
    }
    for (; num2 < 0.0; num2 += kPI * 2.0) {
    }
    return num2;
}

// Orbit.cs:789
double Orbit::GetRelativeInclination(const Orbit& otherOrbit) const {
    return Vector3d::Angle(GetOrbitNormal(), otherOrbit.GetOrbitNormal());
}

// Orbit.cs:794. Quaternion.AngleAxis is native Unity (UnityNative::AngleAxis); Quaternion * Vector3 is managed.
double Orbit::TrueAnomalyFromVector(const Vector3d& vec) const {
    const Vector3d vector3d = Vector3d::Exclude(-GetOrbitNormal().xzy().normalized(), vec);
    const Vector3d vector3d2(UnityNative::AngleAxis(f32store(0.0 - fx(static_cast<float>(LAN))),
                                                    Planetarium::up().ToVector3f()) *
                             Planetarium::right().ToVector3f());
    Vector3d vector3d3(UnityNative::AngleAxis(static_cast<float>(argumentOfPeriapsis),
                                              (-GetOrbitNormal().xzy().normalized()).ToVector3f()) *
                       vector3d2.ToVector3f());
    vector3d3 = PeR() * vector3d3;
    const double num = Vector3d::Angle(vector3d3, vector3d);
    if (M::Abs(Vector3d::Angle(vector3d, Vector3d::Cross(-GetOrbitNormal().xzy().normalized(), vector3d3))) < 90.0) {
        return num * (kPI / 180.0);
    }
    return (360.0 - num) * (kPI / 180.0);
}

// Orbit.cs:986
Vector3d Orbit::getPositionAtT(double T) const { return referenceBody->position + getRelativePositionAtT(T).xzy(); }

// Orbit.cs:991
Vector3d Orbit::getRelativePositionAtT(double T) const {
    const double m = T * meanMotion;
    const double e = solveEccentricAnomaly(m, eccentricity);
    const double tA = GetTrueAnomaly(e);
    return getRelativePositionFromTrueAnomaly(tA);
}

// Orbit.cs:999
Vector3d Orbit::getPositionFromMeanAnomaly(double M_) const {
    return referenceBody->position + getRelativePositionFromMeanAnomaly(M_).xzy();
}

// Orbit.cs:1004 (maxError 1e-5)
Vector3d Orbit::getRelativePositionFromMeanAnomaly(double M_) const {
    const double e = solveEccentricAnomaly(M_, eccentricity, 1E-05);
    return getRelativePositionFromEccAnomaly(e);
}

// Orbit.cs:1036
Vector3d Orbit::getPositionFromEccAnomaly(double E_) const {
    return referenceBody->position + getRelativePositionFromEccAnomaly(E_).xzy();
}

// Orbit.cs:1041
Vector3d Orbit::getRelativePositionFromEccAnomaly(double E_) const {
    double num;
    double num2;
    if (eccentricity < 1.0) {
        num = semiMajorAxis * (M::Cos(E_) - eccentricity);
        num2 = semiMajorAxis * M::Sqrt(1.0 - eccentricity * eccentricity) * M::Sin(E_);
    } else if (eccentricity > 1.0) {
        num = (0.0 - semiMajorAxis) * (eccentricity - M::Cosh(E_));
        num2 = (0.0 - semiMajorAxis) * M::Sqrt(eccentricity * eccentricity - 1.0) * M::Sinh(E_);
    } else {
        num = 0.0;
        num2 = 0.0;
    }
    const Vector3d r = OrbitFrame.X * num + OrbitFrame.Y * num2;
    return Zup().WorldToLocal(r);
}

// Orbit.cs:1064
Vector3d Orbit::getPositionFromEccAnomalyWithSemiMinorAxis(double E_, double semiMinorAxis_) const {
    return referenceBody->position + getRelativePositionFromEccAnomalyWithSemiMinorAxis(E_, semiMinorAxis_).xzy();
}

// Orbit.cs:1069
Vector3d Orbit::getRelativePositionFromEccAnomalyWithSemiMinorAxis(double E_, double semiMinorAxis_) const {
    double num;
    double num2;
    if (eccentricity < 1.0) {
        num = semiMajorAxis * (M::Cos(E_) - eccentricity);
        num2 = semiMinorAxis_ * M::Sin(E_);
    } else if (eccentricity > 1.0) {
        num = (0.0 - semiMajorAxis) * (eccentricity - M::Cosh(E_));
        num2 = (0.0 - semiMinorAxis_) * M::Sinh(E_);
    } else {
        num = 0.0;
        num2 = 0.0;
    }
    const Vector3d r = OrbitFrame.X * num + OrbitFrame.Y * num2;
    return Zup().WorldToLocal(r);
}

// Orbit.cs:1092
Vector3d Orbit::getPositionFromTrueAnomaly(double tA) const {
    return referenceBody->position + getRelativePositionFromTrueAnomaly(tA).xzy();
}

// Orbit.cs:1097
Vector3d Orbit::getRelativePositionFromTrueAnomaly(double tA) const { return getPositionFromTrueAnomaly(tA, true); }

// Orbit.cs:1102
Vector3d Orbit::getPositionFromTrueAnomaly(double tA, bool worldToLocal) const {
    const double num = M::Cos(tA);
    const double num2 = M::Sin(tA);
    const Vector3d vector3d = semiLatusRectum() / (1.0 + eccentricity * num) * (OrbitFrame.X * num + OrbitFrame.Y * num2);
    if (!worldToLocal) return vector3d;
    return Zup().WorldToLocal(vector3d);
}

// Orbit.cs:1114
double Orbit::getOrbitalSpeedAt(double time) const { return getOrbitalSpeedAtDistance(getRelativePositionAtT(time).magnitude()); }
// Orbit.cs:1119
double Orbit::getOrbitalSpeedAtRelativePos(const Vector3d& relPos) const { return getOrbitalSpeedAtDistance(relPos.magnitude()); }
// Orbit.cs:1124
double Orbit::getOrbitalSpeedAtPos(const Vector3d& pos_) const {
    return getOrbitalSpeedAtDistance((referenceBody->position - pos_).magnitude());
}
// Orbit.cs:1129
double Orbit::getOrbitalSpeedAtDistance(double d) const {
    return M::Sqrt(referenceBody->gravParameter * (2.0 / d - 1.0 / semiMajorAxis));
}

// Orbit.cs:1134
Vector3d Orbit::getOrbitalVelocityAtObT(double ObT_) const { return getOrbitalVelocityAtTrueAnomaly(TrueAnomalyAtT(ObT_)); }

// Orbit.cs:1139
Vector3d Orbit::getOrbitalVelocityAtTrueAnomaly(double tA) const { return getOrbitalVelocityAtTrueAnomaly(tA, true); }

// Orbit.cs:1144
Vector3d Orbit::getOrbitalVelocityAtTrueAnomaly(double tA, bool worldToLocal) const {
    const double num = M::Cos(tA);
    const double num2 = M::Sin(tA);
    const double num3 = M::Sqrt(referenceBody->gravParameter / (semiMajorAxis * (1.0 - eccentricity * eccentricity)));
    const double num4 = (0.0 - num2) * num3;
    const double num5 = (num + eccentricity) * num3;
    const Vector3d vector3d = OrbitFrame.X * num4 + OrbitFrame.Y * num5;
    if (!worldToLocal) return vector3d;
    return Zup().WorldToLocal(vector3d);
}

// Orbit.cs:1159
double Orbit::GetOrbitalStateVectorsAtUT(double UT, Vector3d& pos_, Vector3d& vel_) const {
    return GetOrbitalStateVectorsAtObT(getObtAtUT(UT), UT, pos_, vel_);
}

// Orbit.cs:1164
double Orbit::GetOrbitalStateVectorsAtObT(double ObT_, double UT, Vector3d& pos_, Vector3d& vel_) const {
    return GetOrbitalStateVectorsAtTrueAnomaly(TrueAnomalyAtT(ObT_), UT, pos_, vel_);
}

// Orbit.cs:1169
double Orbit::GetOrbitalStateVectorsAtTrueAnomaly(double tA, double UT, Vector3d& pos_, Vector3d& vel_) const {
    return GetOrbitalStateVectorsAtTrueAnomaly(tA, UT, true, pos_, vel_);
}

// Orbit.cs:1174 (zup frame of the reference body at UT: Planetarium.ZupAtT)
double Orbit::GetOrbitalStateVectorsAtTrueAnomaly(double tA, double UT, bool worldToLocal, Vector3d& pos_,
                                                  Vector3d& vel_) const {
    const double num = M::Cos(tA);
    const double num2 = M::Sin(tA);
    const double num3 = semiMajorAxis * (1.0 - eccentricity * eccentricity);
    const double num4 = M::Sqrt(referenceBody->gravParameter / num3);
    const double num5 = (0.0 - num2) * num4;
    const double num6 = (num + eccentricity) * num4;
    const double num7 = num3 / (1.0 + eccentricity * num);
    const double num8 = num * num7;
    const double num9 = num2 * num7;
    if (worldToLocal) {
        CelestialFrame tempZup;
        Planetarium::ZupAtT(UT, *referenceBody, tempZup);
        pos_ = tempZup.WorldToLocal(OrbitFrame.X * num8 + OrbitFrame.Y * num9);
        vel_ = tempZup.WorldToLocal(OrbitFrame.X * num5 + OrbitFrame.Y * num6);
    } else {
        pos_ = OrbitFrame.X * num8 + OrbitFrame.Y * num9;
        vel_ = OrbitFrame.X * num5 + OrbitFrame.Y * num6;
    }
    return num7;
}

// Orbit.cs:1200
double Orbit::GetOrbitalStateVectorsAtUT(double UT, State& state) const {
    return GetOrbitalStateVectorsAtObT(getObtAtUT(UT), state);
}

// Orbit.cs:1205
double Orbit::GetOrbitalStateVectorsAtObT(double ObT_, State& state) const {
    const double num = TrueAnomalyAtT(ObT_);
    if (std::isnan(num)) {
        state = State{};
        return kNaN;
    }
    return GetOrbitalStateVectorsAtTrueAnomaly(num, state);
}

// Orbit.cs:1216 (world-like frame, with acceleration and jerk)
double Orbit::GetOrbitalStateVectorsAtTrueAnomaly(double tA, State& state) const {
    const double gravParameter = referenceBody->gravParameter;
    const double num = semiMajorAxis * (1.0 - eccentricity * eccentricity);
    const double num2 = eccentricity;
    const double num3 = M::Cos(tA);
    const double num4 = M::Sin(tA);
    const double num5 = 1.0 + num2 * num3;
    const double num6 = num / num5;
    const double num7 = num3 * num6;
    const double num8 = num4 * num6;
    const double num9 = M::Sqrt(gravParameter / num);
    const double num10 = (0.0 - num4) * num9;
    const double num11 = (num3 + num2) * num9;
    const double num12 = (0.0 - gravParameter) / (num6 * num6);
    const double num13 = num3 * num12;
    const double num14 = num4 * num12;
    const double num15 = gravParameter * num9 / (num6 * num6 * num6);
    const double num16 = num15 * ((3.0 * num5 - 2.0) * num4);
    const double num17 = num15 * (2.0 * num2 - (3.0 * num5 - 2.0) * num3);
    state.pos = OrbitFrame.X * num7 + OrbitFrame.Y * num8;
    state.vel = OrbitFrame.X * num10 + OrbitFrame.Y * num11;
    state.acc = OrbitFrame.X * num13 + OrbitFrame.Y * num14;
    state.jrk = OrbitFrame.X * num16 + OrbitFrame.Y * num17;
    state.w = (0.0 - num12) / num9;
    return num6;
}

// Orbit.cs:1244
double Orbit::GetOrbitalCurvatureAtTrueAnomaly(double ta) const {
    const double num = M::Cos(ta);
    const double num2 = eccentricity;
    double num4;
    if (num2 != 1.0) {
        const double num3 = 1.0 + num2 * num;
        num4 = num3 * num3 / (1.0 + 2.0 * num2 * num + num2 * num2);
    } else {
        num4 = (1.0 + num) / 2.0;
    }
    return num4 * M::Sqrt(num4) / semiLatusRectum();
}

// Orbit.cs:1261
Vector3d Orbit::Prograde(double UT) const { return getOrbitalVelocityAtUT(UT).xzy().normalized(); }
// Orbit.cs:1266
Vector3d Orbit::Radial(double UT) const { return Vector3d::Exclude(Prograde(UT), Up(UT)).normalized(); }
// Orbit.cs:1271
Vector3d Orbit::Normal(double /*UT*/) const { return -GetOrbitNormal().xzy().normalized(); }
// Orbit.cs:1276
Vector3d Orbit::Up(double UT) const { return getRelativePositionAtUT(UT).xzy().normalized(); }
// Orbit.cs:1281
Vector3d Orbit::Horizontal(double UT) const { return Vector3d::Exclude(Up(UT), Prograde(UT)).normalized(); }

// ---- static helpers ------------------------------------------------------------------------------------------------

// Orbit.cs:1365
Orbit Orbit::OrbitFromStateVectors(const Vector3d& pos_, const Vector3d& vel_, CelestialBody* body, double UT) {
    Orbit orbit;
    orbit.UpdateFromStateVectors((pos_ - body->position).xzy(), vel_.xzy(), body, UT);
    return orbit;
}

// Orbit.cs:1372
bool Orbit::PeApIntersects(const Orbit& primary, const Orbit& secondary, double threshold) {
    if (primary.eccentricity >= 1.0) return primary.PeR() < secondary.ApR() + threshold;
    if (secondary.eccentricity >= 1.0) return secondary.PeR() < primary.ApR() + threshold;
    return M::Max(primary.PeR(), secondary.PeR()) - M::Min(primary.ApR(), secondary.ApR()) <= threshold;
}

// Orbit.cs:1423 process_intercept
static void process_intercept(const Targeting::Sample& cept, double& Vp, double& Vs, double& D) {
    Vp = cept.v;
    Vs = cept.orbit2->TrueAnomaly(cept.tgt().p);
    D = cept.info().d.magnitude();
}

// Orbit.cs:1385 _FindClosestPoints: up to two local minima of the distance between the conics (Targeting.Intercepts,
// 24 samples), scanned from the end of the list; true anomalies on p (FFp, SFp) and s (FFs, SFs), distances CD, CCD.
int Orbit::FindClosestPointsDefault(const Orbit& p, const Orbit& s, double& CD, double& CCD, double& FFp, double& FFs,
                                    double& SFp, double& SFs, double epsilon, int maxIterations, int& iterationCount) {
    if (LegacyOrbitTargeting) {
        FindClosestPoints_old(p, s, CD, CCD, FFp, FFs, SFp, SFs, epsilon, maxIterations, iterationCount);
        return 2;
    }
    const Targeting::Conic conic = Targeting::Conic::Borrow(p);
    const Targeting::Conic conic2 = Targeting::Conic::Borrow(s);
    Targeting::Workspace ws;
    const std::vector<Targeting::Sample*> list = Targeting::Intercepts(conic, conic2, 24, ws);
    int num = 0;
    int index = -1;
    for (int num2 = static_cast<int>(list.size()) - 1; num2 >= 0; num2--) {
        const Targeting::Sample& smp = *list[static_cast<size_t>(num2)];
        if (!(Vector3d::Dot(smp.tgt().p, smp.src.p) <= 0.0) && smp.minimum) {
            if (num != 0) {
                num++;
                process_intercept(smp, SFp, SFs, CCD);
                break;
            }
            num++;
            process_intercept(smp, FFp, FFs, CD);
            index = num2;
        }
    }
    if (num == 1) process_intercept(*list[static_cast<size_t>(index)], SFp, SFs, CCD);
    return num;
}

// Orbit.cs:1430 (Debug.DrawRay/DrawLine calls omitted)
void Orbit::FindClosestPoints_old(const Orbit& p, const Orbit& s, double& CD, double& CCD, double& FFp, double& FFs,
                                  double& SFp, double& SFs, double /*epsilon*/, int maxIterations, int& iterationCount) {
    const double num = p.inclination * (kPI / 180.0);
    const double num2 = s.inclination * (kPI / 180.0);
    const double num3 = num - num2;
    const double dLAN = p.LAN * (kPI / 180.0) - s.LAN * (kPI / 180.0);
    const double x = 1.0 / M::Sin(num3) * (M::Sin(num) * M::Cos(num2) - M::Sin(num2) * M::Cos(num) * M::Cos(dLAN));
    const double num4 = M::Atan2(1.0 / M::Sin(num3) * (M::Sin(num2) * M::Sin(dLAN)), x);
    const double x2 = 1.0 / M::Sin(num3) * (M::Sin(num) * M::Cos(num2) * M::Cos(dLAN) - M::Sin(num2) * M::Cos(num));
    const double num5 = M::Atan2(1.0 / M::Sin(num3) * (M::Sin(num) * M::Sin(dLAN)), x2);
    FFp = num4 - p.argumentOfPeriapsis * (kPI / 180.0);
    FFs = num5 - s.argumentOfPeriapsis * (kPI / 180.0);
    if (p.eccentricity == 0.0 && s.eccentricity == 0.0) {
        CD = Vector3d::Distance(p.getPositionFromTrueAnomaly(FFp), s.getPositionFromTrueAnomaly(FFs));
        CCD = CD;
    }
    CD = SolveClosestBSP(FFp, FFs, num3, kPI, p, s, 0.0001, maxIterations, iterationCount);
    SFp = FFp + kPI;
    SFs = FFs + kPI;
    CCD = SolveClosestBSP(SFp, SFs, num3, kPI / 2.0, p, s, 0.0001, maxIterations, iterationCount);
    CD = M::Sqrt(CD);
    CCD = M::Sqrt(CCD);
}

// Orbit.cs:1460
double Orbit::SolveClosestBSP(double& Fp, double& Fs, double Ir, double dF, const Orbit& p, const Orbit& s,
                              double /*epsilon*/, int maxIterations, int& iterationCount) {
    double num = std::numeric_limits<double>::max();
    double num2 = std::numeric_limits<double>::max();
    double num3 = 0.0;
    double num4 = dF;
    double num5 = dF;
    if (std::fmod(M::Abs(Ir), kPI) * 2.0 > kPI / 2.0) num5 *= -1.0;
    iterationCount = 0;
    num3 = (p.getRelativePositionFromTrueAnomaly(Fp) - s.getRelativePositionFromTrueAnomaly(Fs)).sqrMagnitude();
    while (!(num4 <= 0.0001) && iterationCount < maxIterations) {
        num = (p.getRelativePositionFromTrueAnomaly(Fp + num4) - s.getRelativePositionFromTrueAnomaly(Fs + num5)).sqrMagnitude();
        num2 = (p.getRelativePositionFromTrueAnomaly(Fp - num4) - s.getRelativePositionFromTrueAnomaly(Fs - num5)).sqrMagnitude();
        num3 = M::Min(num3, M::Min(num, num2));
        if (num3 == num) {
            Fp += num4;
            Fs += num5;
        } else if (num3 == num2) {
            Fp -= num4;
            Fs -= num5;
        }
        num4 *= 0.5;
        num5 *= 0.5;
        iterationCount++;
    }
    return num3;
}

// Orbit.cs:1495
double Orbit::RelativeStateAtUT(const Orbit& p, const Orbit& s, double UT, State& pstate, State& sstate, State& rstate,
                                bool dump) {
    p.GetOrbitalStateVectorsAtUT(UT, pstate);
    s.GetOrbitalStateVectorsAtUT(UT, sstate);
    rstate.pos = pstate.pos - sstate.pos;
    rstate.vel = pstate.vel - sstate.vel;
    rstate.acc = pstate.acc - sstate.acc;
    rstate.jrk = pstate.jrk - sstate.jrk;
    rstate.w = pstate.w - sstate.w;
    if (dump) {
        pstate.DumpData("Ship", UT);
        sstate.DumpData("Mun", UT);
        rstate.DumpData("rel", UT);
    }
    return Vector3d::Dot(rstate.vel, rstate.pos);
}

// Orbit.cs:1513
double Orbit::SynodicPeriod(const Orbit& o1, const Orbit& o2) {
    if (o1.eccentricity < 1.0 && o2.eccentricity < 1.0) return o2.period * o1.period / (o2.period - o1.period);
    if (o1.eccentricity < 1.0) return o1.period;
    if (o2.eccentricity < 1.0) return 0.0 - o2.period;
    return kNaN;
}

// Orbit.cs:1530 _SolveClosestApproach (the GameSettings.VERBOSE_DEBUG_LOG messages are not emitted)
double Orbit::SolveClosestApproachDefault(const Orbit& p, const Orbit& s, double& UT, double dT, double /*threshold*/,
                                          double MinUT, double MaxUT, double epsilon, int maxIterations,
                                          int& iterationCount) {
    if (UT < MinUT) return -1.0;
    if (UT > MaxUT) return -1.0;
    CASolutionState cASolutionState(&p, &s, dT);
    iterationCount = 0;
    if (p.numClosePoints > 1) {
        const double num = UT + p.GetDTforTrueAnomalyAtUT(p.FEVp, UT);
        const double num2 = UT + p.GetDTforTrueAnomalyAtUT(p.SEVp, UT);
        cASolutionState.Update(num, iterationCount, true);
        const double magnitude = cASolutionState.rstate.pos.magnitude();
        const double rdv = cASolutionState.rdv;
        const double num3 = num2;
        cASolutionState.Update(num3, iterationCount);
        const double magnitude2 = cASolutionState.rstate.pos.magnitude();
        const double rdv2 = cASolutionState.rdv;
        if (rdv < 0.0 && rdv2 > 0.0) {
            const double num4 = (0.0 - rdv) / (rdv2 - rdv);
            dT = num4 * (num3 - UT);
            UT += dT;
            if (num4 < 0.5) {
                if (cASolutionState.rdv > rdv2 / 2.0) UT = (num + UT) / 2.0;
            } else if (cASolutionState.rdv < rdv / 2.0) {
                UT = (num2 + UT) / 2.0;
            }
        } else if (magnitude2 < magnitude) {
            UT = num3;
        }
    }
    cASolutionState.Update(UT, iterationCount);
    if (cASolutionState.targetAhead() && cASolutionState.rdv > 0.0 && cASolutionState.drdv > 0.0) {
        dT = (0.0 - cASolutionState.rdv) / cASolutionState.drdv;
        dT = M::Max(dT, 0.0 - cASolutionState.maxdt);
        UT += dT;
        cASolutionState.Update(UT, iterationCount);
    }
    while (!(cASolutionState.drdv > 0.0) && iterationCount < maxIterations) {
        const double rdv3 = cASolutionState.rdv;
        const double drdv = cASolutionState.drdv;
        const double ddrdv = cASolutionState.ddrdv;
        const double num5 = drdv * drdv - 2.0 * ddrdv * rdv3;
        if (num5 >= 0.0) {
            if (cASolutionState.rdv > 0.0) {
                dT = (drdv + M::Sqrt(num5)) / ddrdv;
                dT = cASolutionState.Clamp_dt(dT);
            } else {
                dT = (drdv + M::Sqrt(num5)) / ddrdv;
                dT = cASolutionState.Clamp_dt(dT);
            }
        } else {
            dT = (!(cASolutionState.rdv > 0.0)) ? cASolutionState.maxdt : (0.0 - cASolutionState.maxdt);
        }
        UT += dT;
        cASolutionState.Update(UT, iterationCount);
    }
    if (iterationCount >= maxIterations) return cASolutionState.rstate.pos.magnitude();
    dT = cASolutionState.Halley_dt();
    if (UT + dT < MinUT) {
        if (!(MaxUT - MinUT >= cASolutionState.MaxDT) || (!(p.eccentricity < 1.0) && !(s.eccentricity < 1.0))) {
            UT = MinUT;
            cASolutionState.Update(UT, iterationCount, true);
            return cASolutionState.rstate.pos.magnitude();
        }
        dT += cASolutionState.MaxDT;
    } else if (UT + dT > MaxUT) {
        UT = MaxUT;
        cASolutionState.Update(UT, iterationCount, true);
        return cASolutionState.rstate.pos.magnitude();
    }
    while (iterationCount < maxIterations && !(M::Abs(dT) <= epsilon)) {
        UT += dT;
        UT = M::Min(MaxUT, M::Max(MinUT, UT));
        cASolutionState.Update(UT, iterationCount, true);
        dT = cASolutionState.Halley_dt();
    }
    return cASolutionState.rstate.pos.magnitude();
}

// Orbit.cs:1648 _SolveSOI
bool Orbit::SolveSOIDefault(const Orbit& p, const Orbit& s, double& UT, double dT, double Rsoi, double MinUT,
                            double MaxUT, double epsilon, int maxIterations, int& iterationCount) {
    State pstate;
    State sstate;
    State rstate;
    double num = RelativeStateAtUT(p, s, MaxUT, pstate, sstate, rstate);
    double magnitude = rstate.pos.magnitude();
    if (magnitude > Rsoi) {
        log::Info("[Orbit] _SolveSOI: MaxUT puts p outside of s's SoI, punting to old solver");
        return SolveSOI_BSP(p, s, UT, dT, Rsoi, MinUT, MaxUT, epsilon, maxIterations, iterationCount);
    }
    iterationCount = 1;
    if (magnitude == Rsoi) {
        UT = MaxUT;
        return true;
    }
    UtilMath::SphereIntersection(Rsoi, rstate.pos, rstate.vel, dT, false);
    UT = M::Max(MinUT, MaxUT + dT);
    do {
        num = RelativeStateAtUT(p, s, UT, pstate, sstate, rstate);
        magnitude = rstate.pos.sqrMagnitude();
        dT = (M::Sqrt(magnitude) * Rsoi - magnitude) / num;
        UT += dT;
        UT = M::Min(MaxUT, M::Max(MinUT, UT));
    } while (iterationCount++ < maxIterations && !(M::Abs(dT) <= epsilon));
    return iterationCount < maxIterations;
}

// Orbit.cs:1680 _SolveSOI_BSP (Debug.DrawLine omitted)
bool Orbit::SolveSOI_BSPDefault(const Orbit& p, const Orbit& s, double& UT, double dT, double Rsoi, double MinUT,
                                double MaxUT, double epsilon, int maxIterations, int& iterationCount) {
    if (UT < MinUT) return false;
    if (UT > MaxUT) return false;
    iterationCount = 0;
    bool result = false;
    const double num = Rsoi * Rsoi;
    double num2 = M::Abs((p.getPositionAtUT(UT) - s.getPositionAtUT(UT)).sqrMagnitude() - num);
    while (!(dT <= epsilon) && iterationCount < maxIterations) {
        double num3 = (p.getPositionAtUT(UT + dT) - s.getPositionAtUT(UT + dT)).sqrMagnitude() - num;
        double num4 = (p.getPositionAtUT(UT - dT) - s.getPositionAtUT(UT - dT)).sqrMagnitude() - num;
        if (UT - dT < MinUT) num4 = std::numeric_limits<double>::max();
        if (UT + dT > MaxUT) num3 = std::numeric_limits<double>::max();
        if (num2 < 0.0 || num3 < 0.0 || num4 < 0.0) result = true;
        num3 = M::Abs(num3);
        num4 = M::Abs(num4);
        num2 = M::Min(num2, M::Min(num3, num4));
        if (num2 == num4) UT -= dT;
        else if (num2 == num3) UT += dT;
        dT /= 2.0;
        iterationCount++;
    }
    return result;
}

// Orbit.cs:1731
double Orbit::NextCloseApproachTime(const Orbit& p, const Orbit& s, double UT) {
    double num = UT;
    double num2 = std::numeric_limits<double>::max();
    double num3 = UT;
    double num4 = p.period;
    if (p.eccentricity > 1.0) num4 = 100.0 / p.GetMeanMotion(p.semiMajorAxis);
    double num5 = UT + num4;
    for (int i = 0; i < 8; i++) {
        const double num6 = (num5 - num3) / 20.0;
        for (int j = 0; j < 20; j++) {
            const double num7 = num3 + static_cast<double>(j) * num6;
            const double num8 = Separation(p, s, num7);
            if (num8 < num2) {
                num2 = num8;
                num = num7;
            }
        }
        num3 = UtilMath::Clamp(num - num6, UT, UT + num4);
        num5 = UtilMath::Clamp(num + num6, UT, UT + num4);
    }
    return num;
}

// Orbit.cs:1761
double Orbit::Separation(const Orbit& p, const Orbit& s, double UT) {
    return (p.referenceBody->position + p.getRelativePositionAtUT(UT).xzy() -
            (s.referenceBody->position + s.getRelativePositionAtUT(UT).xzy()))
        .magnitude();
}

// Orbit.cs:1766
double Orbit::AscendingNodeTrueAnomaly(const Orbit& p, const Orbit& s) {
    const Vector3d orbitNormal = p.GetOrbitNormal();
    const Vector3d normalized = Vector3d::Cross(s.GetOrbitNormal(), orbitNormal).normalized();
    return p.GetTrueAnomalyOfZupVector(normalized);
}

// Orbit.cs:1773
double Orbit::DescendingNodeTrueAnomaly(const Orbit& p, const Orbit& s) {
    return std::fmod(AscendingNodeTrueAnomaly(p, s) + kPI, UtilMath::TwoPI);
}

// Orbit.cs:1778
double Orbit::RelativeInclination(const Orbit& p, const Orbit& s) {
    return M::Abs(Vector3d::Angle(-p.GetOrbitNormal().xzy().normalized(), -s.GetOrbitNormal().xzy().normalized()));
}

// Orbit.cs:1783
Orbit Orbit::CreateRandomOrbitAround(CelestialBody* body) {
    UnityRandom& rnd = UnityRandom::Global();
    Orbit orbit;
    orbit.referenceBody = body;
    orbit.eccentricity = rnd.Range(0.0001f, 0.01f);
    orbit.semiMajorAxis = rnd.Range(static_cast<float>(body->atmosphereDepth), static_cast<float>(body->sphereOfInfluence));
    orbit.inclination = rnd.Range(-0.001f, 0.001f);
    orbit.LAN = rnd.Range(0.999f, 1.001f);
    orbit.argumentOfPeriapsis = rnd.Range(0.999f, 1.001f);
    orbit.meanAnomalyAtEpoch = rnd.Range(0.999f, 1.001f);
    orbit.epoch = rnd.Range(0.999f, 1.001f);
    orbit.Init();
    return orbit;
}

// Orbit.cs:1798
Orbit Orbit::CreateRandomOrbitAround(CelestialBody* body, double minAltitude, double maxAltitude) {
    UnityRandom& rnd = UnityRandom::Global();
    Orbit orbit;
    orbit.referenceBody = body;
    orbit.eccentricity = rnd.Range(0.0001f, 0.01f);
    orbit.semiMajorAxis = rnd.Range(static_cast<float>(minAltitude), static_cast<float>(maxAltitude));
    orbit.inclination = rnd.Range(-0.001f, 0.001f);
    orbit.LAN = rnd.Range(0.999f, 1.001f);
    orbit.argumentOfPeriapsis = rnd.Range(0.999f, 1.001f);
    orbit.meanAnomalyAtEpoch = rnd.Range(0.999f, 1.001f);
    orbit.epoch = rnd.Range(0.999f, 1.001f);
    orbit.Init();
    return orbit;
}

// Orbit.cs:1813
Orbit Orbit::CreateRandomOrbitNearby(const Orbit& baseOrbit) {
    UnityRandom& rnd = UnityRandom::Global();
    Orbit orbit;
    orbit.eccentricity = baseOrbit.eccentricity + static_cast<double>(rnd.Range(0.0001f, 0.01f));
    orbit.semiMajorAxis = baseOrbit.semiMajorAxis * static_cast<double>(rnd.Range(0.999f, 1.001f));
    orbit.inclination = baseOrbit.inclination + static_cast<double>(rnd.Range(-0.001f, 0.001f));
    orbit.LAN = baseOrbit.LAN * static_cast<double>(rnd.Range(0.999f, 1.001f));
    orbit.argumentOfPeriapsis = baseOrbit.argumentOfPeriapsis * static_cast<double>(rnd.Range(0.999f, 1.001f));
    orbit.meanAnomalyAtEpoch = baseOrbit.meanAnomalyAtEpoch * static_cast<double>(rnd.Range(0.999f, 1.001f));
    orbit.epoch = baseOrbit.epoch;
    orbit.referenceBody = baseOrbit.referenceBody;
    orbit.Init();
    return orbit;
}

// Orbit.cs:1828
Orbit Orbit::CreateRandomOrbitFlyBy(const CelestialBody& tgtBody, double daysToClosestApproach, double universalTime) {
    UnityRandom& rnd = UnityRandom::Global();
    const double periapsis =
        M::Max(tgtBody.Radius * 3.0, tgtBody.sphereOfInfluence * static_cast<double>(rnd.Range(0.0f, 1.1f)));
    const double deltaVatPeriapsis = rnd.Range(100.0f, 500.0f);
    const Orbit* tgtOrbit = BodyOrbit(&tgtBody);
    if (tgtOrbit == nullptr) throw NullReferenceException("Orbit::CreateRandomOrbitFlyBy: target body has no orbit");
    return CreateRandomOrbitFlyBy(*tgtOrbit, daysToClosestApproach * 24.0 * 60.0 * 60.0, periapsis, deltaVatPeriapsis,
                                  universalTime);
}

// Orbit.cs:1835
Orbit Orbit::CreateRandomOrbitFlyBy(const Orbit& targetOrbit, double timeToPeriapsis, double periapsis,
                                    double deltaVatPeriapsis, double universalTime) {
    UnityRandom& rnd = UnityRandom::Global();
    const Vector3d relativePositionAtUT = targetOrbit.getRelativePositionAtUT(universalTime + timeToPeriapsis);
    const Vector3d orbitalVelocityAtUT = targetOrbit.getOrbitalVelocityAtUT(universalTime + timeToPeriapsis);
    Orbit orbit;
    const Vector3d vector3d = relativePositionAtUT + Vector3d(rnd.onUnitSphere()) * periapsis;
    const Vector3d vector3d2 = orbitalVelocityAtUT + (orbitalVelocityAtUT.normalized() + Vector3d(rnd.onUnitSphere())) * deltaVatPeriapsis;
    orbit.UpdateFromStateVectors(vector3d, vector3d2, targetOrbit.referenceBody, universalTime + timeToPeriapsis);
    return orbit;
}

// Orbit.cs:1847
Vector3d Orbit::Swizzle(Vector3d vec) {
    const double y = vec.y;
    vec.y = vec.z;
    vec.z = y;
    return vec;
}

// ---- enum names ----------------------------------------------------------------------------------------------------

const char* ToString(Orbit::PatchTransitionType t) {
    switch (t) {
        case Orbit::PatchTransitionType::INITIAL: return "INITIAL";
        case Orbit::PatchTransitionType::FINAL: return "FINAL";
        case Orbit::PatchTransitionType::ENCOUNTER: return "ENCOUNTER";
        case Orbit::PatchTransitionType::ESCAPE: return "ESCAPE";
        case Orbit::PatchTransitionType::MANEUVER: return "MANEUVER";
        case Orbit::PatchTransitionType::IMPACT: return "IMPACT";
    }
    return "?";
}

const char* ToString(Orbit::EncounterSolutionLevel l) {
    switch (l) {
        case Orbit::EncounterSolutionLevel::NONE: return "NONE";
        case Orbit::EncounterSolutionLevel::ESCAPE: return "ESCAPE";
        case Orbit::EncounterSolutionLevel::ORBIT_INTERSECT: return "ORBIT_INTERSECT";
        case Orbit::EncounterSolutionLevel::SOI_INTERSECT_2: return "SOI_INTERSECT_2";
        case Orbit::EncounterSolutionLevel::SOI_INTERSECT_1: return "SOI_INTERSECT_1";
    }
    return "?";
}

const char* ToString(Orbit::ObjectType t) {
    switch (t) {
        case Orbit::ObjectType::VESSEL: return "VESSEL";
        case Orbit::ObjectType::SPACE_DEBRIS: return "SPACE_DEBRIS";
        case Orbit::ObjectType::CELESTIAL_BODIES: return "CELESTIAL_BODIES";
        case Orbit::ObjectType::UNKNOWN_MISC: return "UNKNOWN_MISC";
        case Orbit::ObjectType::KERBAL: return "KERBAL";
    }
    return "?";
}

}  // namespace ksp
