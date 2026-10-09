// Trajectory (Trajectory.cs, 436 lines): the sampled points of one conic patch for the map view (PatchRendering, WP-44)
// and the frame conversions between body-relative, "relative to another body" and lerped frames. Pure maths over Orbit
// and CelestialBody positions; the drawing is the shell's.
//   UpdateFromOrbit(orbit, n)  samples n points (Unity axes, relative to the patch's reference body) in eccentric
//                              anomaly: closed non-FINAL patches between StartUT and EndUT with two extra samples on each
//                              side, closed FINAL patches over a full ellipse, hyperbolas between StartUT and EndUT (an
//                              infinite end is pulled in to 95 % of its true anomaly). Writes the orbit's sampling fields
//                              (fromE/toE/fromV/toV/sampleInterval/E/V) like KSP.
//   ConvertPointTo*            local (referenceBody.position), relative (another body's motion over time), local at a
//                              UT, lerped (blend local -> relative between minUT and escapeUT)
// C# arrays are nullable: tPoints / Vs / tTimes are std::optional (null until UpdateFromOrbit), the GetPoints* outputs
// are pointers. KSP quirk kept: the Reframe* methods hand the new Trajectory's tPoints (still null) to GetPoints*, which
// throws NullReferenceException as soon as a point is written (they cannot succeed for a sampled trajectory).
#pragma once

#include <optional>
#include <vector>

#include "ksp/math/vector3d.hpp"
#include "ksp/unity/color.hpp"

namespace ksp {

class CelestialBody;
class Orbit;

class Trajectory {
public:
    std::optional<std::vector<double>> Vs;  // public double[] Vs
    CelestialBody* referenceBody = nullptr;
    Orbit* patch = nullptr;

    const std::optional<std::vector<Vector3d>>& TPoints() const { return tPoints; }  // Trajectory.cs:26
    void SetTPoints(std::optional<std::vector<Vector3d>> v) { tPoints = std::move(v); }

    void UpdateFromOrbit(Orbit& orbit, int sampleCount);  // Trajectory.cs:38

    Trajectory ReframeToLocal() const;                                  // :153
    Trajectory ReframeToRelative(CelestialBody* relativeTo) const;      // :167
    Trajectory ReframeToLocalAtUT(CelestialBody* relativeTo, double atUT) const;  // :181
    Trajectory ReframeToLerped(CelestialBody* relativeFrom, CelestialBody* relativeTo, double minUT, double escapeUT,
                               double linearity) const;                 // :195

    Vector3d ConvertPointToLocal(const Vector3d& point) const;                                          // :209
    Vector3d ConvertPointToRelative(const Vector3d& point, double time, const CelestialBody* relativeTo) const;  // :214
    Vector3d ConvertPointToLocalAtUT(const Vector3d& point, double atUT, const CelestialBody* relativeTo) const;  // :219
    Vector3d ConvertPointToLerped(const Vector3d& point, double time, const CelestialBody* relativeFrom,
                                  const CelestialBody* relativeTo, double minUT, double escapeUT, double linearity) const;  // :224

    // GetPoints*: rPoints = nullptr is a C# null array (NullReferenceException once a point is written)
    void GetPointsLocal(std::vector<Vector3d>* rPoints) const;                                           // :231
    void GetPointsRelative(std::vector<Vector3d>* rPoints, const CelestialBody* relativeTo) const;      // :243
    void GetPointsLocalAtUT(std::vector<Vector3d>* rPoints, const CelestialBody* relativeTo, double atUT) const;  // :255
    void GetPointsLerped(std::vector<Vector3d>* rPoints, const CelestialBody* relativeFrom, const CelestialBody* relativeTo,
                         double minUT, double escapeUT, double linearity) const;                        // :267

    Vector3d GetPeriapsisLocal() const;                                       // :279
    Vector3d GetPeriapsisRelative(const CelestialBody* relativeTo);           // :284 (may rewind patch.timeToPe)
    Vector3d GetPeriapsisLocalAtUT(const CelestialBody* relativeTo, double atUT) const;
    Vector3d GetPeriapsisLerped(const CelestialBody* relativeFrom, const CelestialBody* relativeTo, double minUT,
                                double escapeUT, double linearity);
    Vector3d GetApoapsisLocal() const;                                        // :307
    Vector3d GetApoapsisRelative(const CelestialBody* relativeTo) const;
    Vector3d GetApoapsisLocalAtUT(const CelestialBody* relativeTo, double atUT) const;
    Vector3d GetApoapsisLerped(const CelestialBody* relativeFrom, const CelestialBody* relativeTo, double minUT,
                               double escapeUT, double linearity) const;
    Vector3d GetPatchStartLocal() const;                                      // :343
    Vector3d GetPatchStartRelative(const CelestialBody* relativeTo) const;
    Vector3d GetPatchStartLocalAtUT(const CelestialBody* relativeTo, double atUT) const;
    Vector3d GetPatchStartLerped(const CelestialBody* relativeFrom, const CelestialBody* relativeTo, double minUT,
                                 double escapeUT, double linearity) const;
    Vector3d GetPatchEndLocal() const;                                        // :363
    Vector3d GetPatchEndRelative(const CelestialBody* relativeTo) const;
    Vector3d GetPatchEndLocalAtUT(const CelestialBody* relativeTo, double atUT) const;
    Vector3d GetPatchEndLerped(const CelestialBody* relativeFrom, const CelestialBody* relativeTo, double minUT,
                               double escapeUT, double linearity) const;
    Vector3d GetRefBodyPosLocal() const;                                      // :383
    Vector3d GetRefBodyPosRelative(const CelestialBody* relativeTo);          // (may rewind patch.timeToPe)
    Vector3d GetRefBodyPosLocalAtUT(const CelestialBody* relativeTo, double atUT) const;
    Vector3d GetRefBodyPosLerped(const CelestialBody* relativeFrom, const CelestialBody* relativeTo, double minUT,
                                 double escapeUT, double linearity);

    const std::vector<Vector3d>* GetPoints() const { return tPoints ? &*tPoints : nullptr; }  // :411
    const std::vector<double>* GetTimes() const { return tTimes ? &*tTimes : nullptr; }       // :416
    void GetColors(const Color& baseColor, std::vector<Color>& colors) const;                 // :421

private:
    void RewindTimeToPe();  // the shared `patch.timeToPe -= patch.period` guard of GetPeriapsis/RefBodyPos *Relative/*Lerped

    std::optional<std::vector<Vector3d>> tPoints;
    Vector3d periapsis;
    Vector3d apoapsis;
    Vector3d patchStartPoint;
    Vector3d patchEndPoint;
    Vector3d refBodyPos;
    std::optional<std::vector<double>> tTimes;
};

}  // namespace ksp
