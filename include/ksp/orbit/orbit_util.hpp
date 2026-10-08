// OrbitUtil (Assembly-CSharp/OrbitUtil.cs): phase/ejection angles, Hohmann helpers, body-hierarchy queries.
// Not ported yet: CalculateIntersections (returns IntersectInformation, a WP-03 type; it is a thin wrapper around
// Orbit::FindClosestPoints + GetDTforTrueAnomaly).
#pragma once

#include "ksp/math/vector3d.hpp"
#include "ksp/orbit/celestial_body_ref.hpp"
#include "ksp/orbit/orbit.hpp"

namespace ksp {

struct OrbitUtil {
    // OrbitUtil.cs:13 (degrees in (-180, 180], from the current pos fields)
    static double CurrentPhaseAngle(const Orbit& origin, const Orbit& destination);
    // OrbitUtil.cs:29 (prograde = the reference body's orbital velocity at UT, zero for the Sun)
    static double CurrentEjectionAngle(const Orbit& origin, double UT);
    // OrbitUtil.cs:36 (degrees)
    static double CurrentEjectionAngle(const Orbit& origin, double UT, const Vector3d& prograde);
    // OrbitUtil.cs:85 (uses the current radius fields)
    static double GetTransferTime(const Orbit& o1, const Orbit& o2);
    // OrbitUtil.cs:90
    static double GetTransferPhaseAngle(const Orbit& o1, const Orbit& o2, double Th);
    // OrbitUtil.cs:96: the ancestor of src that orbits the root body (nullptr when src has no reference body)
    static CelestialBodyRef* FindPlanet(CelestialBodyRef* src);
    // OrbitUtil.cs:112: deepest common ancestor of o.referenceBody and cb (hierarchies up to 10 deep, as in KSP;
    // deeper ones throw std::out_of_range like the C# array)
    static CelestialBodyRef* FindCommonAncestor(const Orbit& o, CelestialBodyRef* cb);
    // OrbitUtil.cs:142
    static double GetSmaDistance(const Orbit* o1, const Orbit* o2);
    // OrbitUtil.cs:147 (KSP quirk: the running sum is passed down AND added again, so ancestors are over-counted)
    static double GetSmaDistanceToSun(const Orbit* obt, double d = 0.0);
};

}  // namespace ksp
