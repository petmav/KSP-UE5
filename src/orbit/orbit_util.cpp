// OrbitUtil (Assembly-CSharp/OrbitUtil.cs); see include/ksp/orbit/orbit_util.hpp.
#include "ksp/orbit/orbit_util.hpp"

#include <array>

#include "ksp/dotnet/math.hpp"

namespace ksp {

namespace M = dotnet::Math;

namespace {
constexpr double kPI = 3.141592653589793;

// OrbitUtil.cs:132 (static CelestialBody[10] arrays in KSP; local here, same indexing)
void GetBodyHierarchy(CelestialBodyRef* cb, std::array<CelestialBodyRef*, 10>& hierarchy, int& i) {
    hierarchy.at(static_cast<size_t>(i)) = cb;
    CelestialBodyRef* ref = cb->referenceBody();
    if (ref != nullptr && ref != cb) {
        i++;
        GetBodyHierarchy(ref, hierarchy, i);
    }
}
}  // namespace

// OrbitUtil.cs:13
double OrbitUtil::CurrentPhaseAngle(const Orbit& origin, const Orbit& destination) {
    const Vector3d normalized = origin.GetOrbitNormal().normalized();
    const Vector3d vector3d = Vector3d::Exclude(normalized, destination.pos);
    double num = Vector3d::Angle(origin.pos, vector3d);
    if (Vector3d::Dot(Vector3d::Cross(origin.pos, vector3d), normalized) < 0.0) num = 360.0 - num;
    if (num > 180.0) num -= 360.0;
    return num;
}

// OrbitUtil.cs:29
double OrbitUtil::CurrentEjectionAngle(const Orbit& origin, double UT) {
    const CelestialBodyRef* referenceBody = origin.referenceBody;
    const Orbit* bodyOrbit = BodyOrbit(referenceBody);
    const Vector3d prograde = (bodyOrbit != nullptr) ? bodyOrbit->getOrbitalVelocityAtUT(UT) : Vector3d::zero();
    return CurrentEjectionAngle(origin, UT, prograde);
}

// OrbitUtil.cs:36
double OrbitUtil::CurrentEjectionAngle(const Orbit& origin, double UT, const Vector3d& prograde) {
    double num = 0.0;
    const Vector3d relativePositionAtUT = origin.getRelativePositionAtUT(UT);
    num = std::fmod((M::Atan2(prograde.y, prograde.x) - M::Atan2(relativePositionAtUT.y, relativePositionAtUT.x)) * (180.0 / kPI),
                    360.0);
    if (num > 180.0) num -= 360.0;
    if (num < -180.0) num += 360.0;
    return num;
}

// OrbitUtil.cs:85
double OrbitUtil::GetTransferTime(const Orbit& o1, const Orbit& o2) {
    return kPI * M::Sqrt(M::Pow(o1.radius + o2.radius, 3.0) / (8.0 * o1.referenceBody->gravParameter));
}

// OrbitUtil.cs:90
double OrbitUtil::GetTransferPhaseAngle(const Orbit& /*o1*/, const Orbit& o2, double Th) {
    const double num = M::Sqrt(o2.referenceBody->gravParameter / M::Pow(o2.radius, 3.0));
    return kPI - num * Th;
}

// OrbitUtil.cs:96
CelestialBodyRef* OrbitUtil::FindPlanet(CelestialBodyRef* src) {
    if (src->referenceBody() == nullptr) return nullptr;
    CelestialBodyRef* celestialBody = src;
    do {
        celestialBody = src;
        src = src->referenceBody();
    } while (src->referenceBody() != src && src->referenceBody() != nullptr);
    return celestialBody;
}

// OrbitUtil.cs:112
CelestialBodyRef* OrbitUtil::FindCommonAncestor(const Orbit& o, CelestialBodyRef* cb) {
    if (o.referenceBody == cb) return cb;
    std::array<CelestialBodyRef*, 10> p1Hierarchy{};
    std::array<CelestialBodyRef*, 10> p2Hierarchy{};
    int p1i = 0;
    GetBodyHierarchy(o.referenceBody, p1Hierarchy, p1i);
    int p2i = 0;
    GetBodyHierarchy(cb, p2Hierarchy, p2i);
    CelestialBodyRef* result = nullptr;
    while (p1i >= 0 && p2i >= 0 && p1Hierarchy[static_cast<size_t>(p1i)] == p2Hierarchy[static_cast<size_t>(p2i)]) {
        result = p1Hierarchy[static_cast<size_t>(p1i)];
        p1i--;
        p2i--;
    }
    return result;
}

// OrbitUtil.cs:142
double OrbitUtil::GetSmaDistance(const Orbit* o1, const Orbit* o2) { return GetSmaDistanceToSun(o1) - GetSmaDistanceToSun(o2); }

// OrbitUtil.cs:147
double OrbitUtil::GetSmaDistanceToSun(const Orbit* obt, double d) {
    if (obt != nullptr) {
        d += obt->semiMajorAxis;
        if (obt->referenceBody != nullptr) d += GetSmaDistanceToSun(BodyOrbit(obt->referenceBody), d);
    }
    return d;
}

}  // namespace ksp
