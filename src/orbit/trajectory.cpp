// Trajectory (Trajectory.cs); see include/ksp/orbit/trajectory.hpp.
#include "ksp/orbit/trajectory.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/orbit/orbit.hpp"

namespace ksp {

namespace {
namespace M = dotnet::Math;
using PT = Orbit::PatchTransitionType;
constexpr double kPI = UtilMath::PI;

template <class T>
void EnsureArray(std::optional<std::vector<T>>& a, int n) {  // if (a == null || a.Length != n) a = new T[n]
    if (n < 0) throw dotnet::OverflowException("Arithmetic operation resulted in an overflow.");
    if (!a || static_cast<int>(a->size()) != n) a = std::vector<T>(static_cast<size_t>(n));
}

const CelestialBody& Req(const CelestialBody* b) {
    if (b == nullptr) throw NullReferenceException("Trajectory: body");
    return *b;
}

const Orbit& Req(const Orbit* o) {
    if (o == nullptr) throw NullReferenceException("Trajectory: patch");
    return *o;
}

// rPoints[i] = ... over i < tPoints.Length (C# arrays: null -> NullReferenceException, short -> IndexOutOfRange)
template <class F>
void FillPoints(const std::optional<std::vector<Vector3d>>& tPoints, std::vector<Vector3d>* rPoints, F convert) {
    if (!tPoints) throw NullReferenceException("Trajectory: tPoints");
    const size_t num = tPoints->size();
    for (size_t i = 0; i < num; ++i) {
        if (rPoints == nullptr) throw NullReferenceException("Trajectory: rPoints");
        rPoints->at(i) = convert(i);
    }
}
}  // namespace

// Trajectory.cs:38
void Trajectory::UpdateFromOrbit(Orbit& orbit, int sampleCount) {
    EnsureArray(tPoints, sampleCount);
    EnsureArray(Vs, sampleCount);
    EnsureArray(tTimes, sampleCount);
    std::vector<Vector3d>& tp = *tPoints;
    std::vector<double>& vs = *Vs;
    std::vector<double>& tt = *tTimes;
    const double maxValue = std::numeric_limits<double>::max();
    patch = &orbit;
    if (orbit.eccentricity < 1.0) {
        if (orbit.patchEndTransition != PT::FINAL && !orbit.debug_returnFullEllipseTrajectory) {
            orbit.fromE = orbit.EccentricAnomalyAtUT(orbit.StartUT);
            orbit.toE = orbit.EccentricAnomalyAtUT(orbit.EndUT);
            orbit.fromV = orbit.GetTrueAnomaly(orbit.fromE);
            orbit.toV = orbit.GetTrueAnomaly(orbit.toE);
            if (orbit.fromV > orbit.toV) {
                orbit.fromE = 0.0 - (kPI * 2.0 - orbit.fromE);
                orbit.fromV = 0.0 - (kPI * 2.0 - orbit.fromV);
            }
            orbit.sampleInterval = (orbit.toE - orbit.fromE) / static_cast<double>(sampleCount - 5);
            orbit.fromE -= orbit.sampleInterval * 2.0;
            for (int i = 0; i < sampleCount; ++i) {
                orbit.E = orbit.fromE + orbit.sampleInterval * static_cast<double>(i);
                orbit.V = orbit.GetTrueAnomaly(orbit.E);
                vs[static_cast<size_t>(i)] = orbit.V;
                tt[static_cast<size_t>(i)] = orbit.StartUT + orbit.GetDTforTrueAnomaly(orbit.V, maxValue);
                if (orbit.fromE < 0.0 && orbit.E > 0.0 && !std::isinf(orbit.period)) tt[static_cast<size_t>(i)] += orbit.period;
                tp[static_cast<size_t>(i)] = orbit.getRelativePositionFromEccAnomaly(orbit.E).xzy();
            }
        } else {
            orbit.sampleInterval = kPI * 2.0 / static_cast<double>(sampleCount);
            for (int j = 0; j < sampleCount; ++j) {
                orbit.E = static_cast<double>(j) * orbit.sampleInterval;
                orbit.V = orbit.GetTrueAnomaly(orbit.E);
                vs[static_cast<size_t>(j)] = orbit.V;
                tt[static_cast<size_t>(j)] = orbit.StartUT + orbit.GetDTforTrueAnomaly(orbit.V, maxValue);
                tp[static_cast<size_t>(j)] = orbit.getRelativePositionFromEccAnomaly(orbit.E).xzy();
            }
        }
    } else {
        orbit.fromE = orbit.EccentricAnomalyAtUT(orbit.StartUT);
        orbit.toE = orbit.EccentricAnomalyAtUT(orbit.EndUT);
        orbit.fromV = orbit.GetTrueAnomaly(orbit.fromE);
        orbit.toV = orbit.GetTrueAnomaly(orbit.toE);
        if (std::isinf(orbit.fromE)) {
            if (orbit.fromE > 0.0)
                orbit.fromV *= 0.95;
            else
                orbit.fromV = 0.95 * (orbit.fromV - kPI * 2.0);
            orbit.fromE = orbit.GetEccentricAnomaly(orbit.fromV);
        }
        if (std::isinf(orbit.toE)) {
            if (orbit.toE > 0.0)
                orbit.toV *= 0.95;
            else
                orbit.toV = 0.95 * (orbit.toV - kPI * 2.0);
            orbit.toE = orbit.GetEccentricAnomaly(orbit.toV);
        }
        orbit.sampleInterval = (orbit.toE - orbit.fromE) / static_cast<double>(sampleCount - 1);
        for (int k = 0; k < sampleCount; ++k) {  // Vs is not written for hyperbolas (sic)
            orbit.E = orbit.fromE + orbit.sampleInterval * static_cast<double>(k);
            orbit.V = orbit.GetTrueAnomaly(orbit.E);
            tt[static_cast<size_t>(k)] = orbit.StartUT + orbit.GetDTforTrueAnomaly(orbit.V, maxValue);
            if (orbit.fromE < 0.0 && orbit.E > 0.0 && !std::isinf(orbit.period)) tt[static_cast<size_t>(k)] += orbit.period;
            tp[static_cast<size_t>(k)] = orbit.getRelativePositionFromEccAnomaly(orbit.E).xzy();
        }
    }
    if (orbit.eccentricity < 1.0) {
        periapsis = orbit.getRelativePositionAtT(0.0).xzy();
        apoapsis = orbit.getRelativePositionAtT(orbit.period * 0.5).xzy();
    } else {
        periapsis = orbit.GetEccVector().xzy().normalized() * ((0.0 - orbit.semiMajorAxis) * (orbit.eccentricity - 1.0));
        apoapsis = Vector3d::zero();
    }
    patchStartPoint = orbit.getRelativePositionAtUT(orbit.StartUT).xzy();
    patchEndPoint = orbit.getRelativePositionAtUT(orbit.EndUT).xzy();
    refBodyPos = Vector3d::zero();
    referenceBody = orbit.referenceBody;
}

// Trajectory.cs:153-207: the new Trajectory's tPoints are null when GetPoints* fills them (KSP quirk: throws)
Trajectory Trajectory::ReframeToLocal() const {
    Trajectory t;
    t.patch = patch;
    t.referenceBody = referenceBody;
    GetPointsLocal(t.tPoints ? &*t.tPoints : nullptr);
    t.periapsis = ConvertPointToLocal(periapsis);
    t.apoapsis = ConvertPointToLocal(apoapsis);
    t.patchStartPoint = ConvertPointToLocal(patchStartPoint);
    t.patchEndPoint = ConvertPointToLocal(patchEndPoint);
    t.refBodyPos = ConvertPointToLocal(refBodyPos);
    return t;
}

Trajectory Trajectory::ReframeToRelative(CelestialBody* relativeTo) const {
    Trajectory t;
    t.patch = patch;
    t.referenceBody = referenceBody;
    GetPointsRelative(t.tPoints ? &*t.tPoints : nullptr, relativeTo);
    const Orbit& p = Req(patch);
    t.periapsis = ConvertPointToRelative(periapsis, p.StartUT + p.timeToPe, relativeTo);
    t.apoapsis = ConvertPointToRelative(apoapsis, p.StartUT + p.timeToAp, relativeTo);
    t.patchStartPoint = ConvertPointToRelative(patchStartPoint, p.StartUT, relativeTo);
    t.patchEndPoint = ConvertPointToRelative(patchEndPoint, p.EndUT, relativeTo);
    t.refBodyPos = ConvertPointToRelative(refBodyPos, p.StartUT + p.timeToPe, relativeTo);
    return t;
}

Trajectory Trajectory::ReframeToLocalAtUT(CelestialBody* relativeTo, double atUT) const {
    Trajectory t;
    t.patch = patch;
    t.referenceBody = referenceBody;
    GetPointsLocalAtUT(t.tPoints ? &*t.tPoints : nullptr, relativeTo, atUT);
    t.periapsis = ConvertPointToLocalAtUT(periapsis, atUT, relativeTo);
    t.apoapsis = ConvertPointToLocalAtUT(apoapsis, atUT, relativeTo);
    t.patchStartPoint = ConvertPointToLocalAtUT(patchStartPoint, atUT, relativeTo);
    t.patchEndPoint = ConvertPointToLocalAtUT(patchEndPoint, atUT, relativeTo);
    t.refBodyPos = ConvertPointToLocalAtUT(refBodyPos, atUT, relativeTo);
    return t;
}

Trajectory Trajectory::ReframeToLerped(CelestialBody* relativeFrom, CelestialBody* relativeTo, double minUT, double escapeUT,
                                       double linearity) const {
    Trajectory t;
    t.patch = patch;
    t.referenceBody = referenceBody;
    GetPointsLerped(t.tPoints ? &*t.tPoints : nullptr, relativeFrom, relativeTo, minUT, escapeUT, linearity);
    const Orbit& p = Req(patch);
    t.periapsis = ConvertPointToLerped(periapsis, p.StartUT + p.timeToPe, relativeFrom, relativeTo, minUT, escapeUT, linearity);
    t.apoapsis = ConvertPointToLerped(apoapsis, p.StartUT + p.timeToAp, relativeFrom, relativeTo, minUT, escapeUT, linearity);
    t.patchStartPoint = ConvertPointToLerped(patchStartPoint, p.StartUT, relativeFrom, relativeTo, minUT, escapeUT, linearity);
    t.patchEndPoint = ConvertPointToLerped(patchEndPoint, p.EndUT, relativeFrom, relativeTo, minUT, escapeUT, linearity);
    t.refBodyPos = ConvertPointToLerped(refBodyPos, p.StartUT + p.timeToPe, relativeFrom, relativeTo, minUT, escapeUT, linearity);
    return t;
}

// Trajectory.cs:209
Vector3d Trajectory::ConvertPointToLocal(const Vector3d& point) const { return point + Req(referenceBody).position; }

// Trajectory.cs:214
Vector3d Trajectory::ConvertPointToRelative(const Vector3d& point, double time, const CelestialBody* relativeTo) const {
    return point + Req(referenceBody).getTruePositionAtUT(time) - Req(relativeTo).getTruePositionAtUT(time) + relativeTo->position;
}

// Trajectory.cs:219
Vector3d Trajectory::ConvertPointToLocalAtUT(const Vector3d& point, double atUT, const CelestialBody* relativeTo) const {
    return point + Req(referenceBody).getTruePositionAtUT(atUT) - Req(relativeTo).getTruePositionAtUT(atUT) + relativeTo->position;
}

// Trajectory.cs:224
Vector3d Trajectory::ConvertPointToLerped(const Vector3d& point, double time, const CelestialBody* relativeFrom,
                                          const CelestialBody* relativeTo, double minUT, double escapeUT, double linearity) const {
    const Vector3d from = point + Req(relativeFrom).position;
    const Vector3d to = point + relativeFrom->getTruePositionAtUT(time) - Req(relativeTo).getTruePositionAtUT(time) + relativeTo->position;
    return Vector3d::Lerp(from, to,
                          M::Pow(UtilMath::InverseLerp(M::Max(minUT, Planetarium::GetUniversalTime()), escapeUT, time), linearity));
}

// Trajectory.cs:231-277
void Trajectory::GetPointsLocal(std::vector<Vector3d>* rPoints) const {
    if (referenceBody != nullptr) FillPoints(tPoints, rPoints, [&](size_t i) { return ConvertPointToLocal((*tPoints)[i]); });
}

void Trajectory::GetPointsRelative(std::vector<Vector3d>* rPoints, const CelestialBody* relativeTo) const {
    if (referenceBody != nullptr)
        FillPoints(tPoints, rPoints, [&](size_t i) { return ConvertPointToRelative((*tPoints)[i], tTimes->at(i), relativeTo); });
}

void Trajectory::GetPointsLocalAtUT(std::vector<Vector3d>* rPoints, const CelestialBody* relativeTo, double atUT) const {
    if (referenceBody != nullptr)
        FillPoints(tPoints, rPoints, [&](size_t i) { return ConvertPointToLocalAtUT((*tPoints)[i], atUT, relativeTo); });
}

void Trajectory::GetPointsLerped(std::vector<Vector3d>* rPoints, const CelestialBody* relativeFrom,
                                 const CelestialBody* relativeTo, double minUT, double escapeUT, double linearity) const {
    if (referenceBody != nullptr)
        FillPoints(tPoints, rPoints, [&](size_t i) {
            return ConvertPointToLerped((*tPoints)[i], tTimes->at(i), relativeFrom, relativeTo, minUT, escapeUT, linearity);
        });
}

void Trajectory::RewindTimeToPe() {
    Req(patch);
    Orbit& p = *patch;
    if (p.eccentricity < 1.0 && p.patchEndTransition != PT::FINAL && p.StartUT + p.timeToPe > p.EndUT) p.timeToPe -= p.period;
}

// Trajectory.cs:279-409
Vector3d Trajectory::GetPeriapsisLocal() const { return ConvertPointToLocal(periapsis); }
Vector3d Trajectory::GetPeriapsisRelative(const CelestialBody* relativeTo) {
    RewindTimeToPe();
    return ConvertPointToRelative(periapsis, patch->StartUT + patch->timeToPe, relativeTo);
}
Vector3d Trajectory::GetPeriapsisLocalAtUT(const CelestialBody* relativeTo, double atUT) const {
    return ConvertPointToLocalAtUT(periapsis, atUT, relativeTo);
}
Vector3d Trajectory::GetPeriapsisLerped(const CelestialBody* relativeFrom, const CelestialBody* relativeTo, double minUT,
                                        double escapeUT, double linearity) {
    RewindTimeToPe();
    return ConvertPointToLerped(periapsis, patch->StartUT + patch->timeToPe, relativeFrom, relativeTo, minUT, escapeUT, linearity);
}
Vector3d Trajectory::GetApoapsisLocal() const {
    if (Req(patch).eccentricity >= 1.0) return Vector3d::zero();
    return ConvertPointToLocal(apoapsis);
}
Vector3d Trajectory::GetApoapsisRelative(const CelestialBody* relativeTo) const {
    if (Req(patch).eccentricity >= 1.0) return Vector3d::zero();
    return ConvertPointToRelative(apoapsis, patch->StartUT + patch->timeToAp, relativeTo);
}
Vector3d Trajectory::GetApoapsisLocalAtUT(const CelestialBody* relativeTo, double atUT) const {
    if (Req(patch).eccentricity >= 1.0) return Vector3d::zero();
    return ConvertPointToLocalAtUT(apoapsis, atUT, relativeTo);
}
Vector3d Trajectory::GetApoapsisLerped(const CelestialBody* relativeFrom, const CelestialBody* relativeTo, double minUT,
                                       double escapeUT, double linearity) const {
    if (Req(patch).eccentricity >= 1.0) return Vector3d::zero();
    return ConvertPointToLerped(apoapsis, patch->StartUT + patch->timeToAp, relativeFrom, relativeTo, minUT, escapeUT, linearity);
}
Vector3d Trajectory::GetPatchStartLocal() const { return ConvertPointToLocal(patchStartPoint); }
Vector3d Trajectory::GetPatchStartRelative(const CelestialBody* relativeTo) const {
    return ConvertPointToRelative(patchStartPoint, Req(patch).StartUT, relativeTo);
}
Vector3d Trajectory::GetPatchStartLocalAtUT(const CelestialBody* relativeTo, double atUT) const {
    return ConvertPointToLocalAtUT(patchStartPoint, atUT, relativeTo);
}
Vector3d Trajectory::GetPatchStartLerped(const CelestialBody* relativeFrom, const CelestialBody* relativeTo, double minUT,
                                         double escapeUT, double linearity) const {
    return ConvertPointToLerped(patchStartPoint, Req(patch).StartUT, relativeFrom, relativeTo, minUT, escapeUT, linearity);
}
Vector3d Trajectory::GetPatchEndLocal() const { return ConvertPointToLocal(patchEndPoint); }
Vector3d Trajectory::GetPatchEndRelative(const CelestialBody* relativeTo) const {
    return ConvertPointToRelative(patchEndPoint, Req(patch).EndUT, relativeTo);
}
Vector3d Trajectory::GetPatchEndLocalAtUT(const CelestialBody* relativeTo, double atUT) const {
    return ConvertPointToLocalAtUT(patchEndPoint, atUT, relativeTo);
}
Vector3d Trajectory::GetPatchEndLerped(const CelestialBody* relativeFrom, const CelestialBody* relativeTo, double minUT,
                                       double escapeUT, double linearity) const {
    return ConvertPointToLerped(patchEndPoint, Req(patch).EndUT, relativeFrom, relativeTo, minUT, escapeUT, linearity);
}
Vector3d Trajectory::GetRefBodyPosLocal() const { return ConvertPointToLocal(refBodyPos); }
Vector3d Trajectory::GetRefBodyPosRelative(const CelestialBody* relativeTo) {
    RewindTimeToPe();
    return ConvertPointToRelative(refBodyPos, patch->StartUT + patch->timeToPe, relativeTo);
}
Vector3d Trajectory::GetRefBodyPosLocalAtUT(const CelestialBody* relativeTo, double atUT) const {
    return ConvertPointToLocalAtUT(refBodyPos, atUT, relativeTo);
}
Vector3d Trajectory::GetRefBodyPosLerped(const CelestialBody* relativeFrom, const CelestialBody* relativeTo, double minUT,
                                         double escapeUT, double linearity) {
    RewindTimeToPe();
    return ConvertPointToLerped(refBodyPos, patch->StartUT + patch->timeToPe, relativeFrom, relativeTo, minUT, escapeUT, linearity);
}

// Trajectory.cs:421
void Trajectory::GetColors(const Color& baseColor, std::vector<Color>& colors) const {
    if (!tPoints) throw NullReferenceException("Trajectory.GetColors: tPoints");
    const int len = static_cast<int>(tPoints->size());
    int num = len;
    while (num-- > 0) {
        if (Req(patch).eccentricity < 1.0 && patch->patchEndTransition != PT::FINAL && (num < 2 || num >= len - 2))
            colors.at(static_cast<size_t>(num)) = Color::clear();
        else
            colors.at(static_cast<size_t>(num)) = baseColor;
    }
}

}  // namespace ksp
