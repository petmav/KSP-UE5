// CelestialBody.cs ported operation for operation (C# evaluation order kept: left-to-right products, no reassociation).
// Paths are relative to decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/terrain/pqs_prefab.hpp"

#include <cmath>
#include <limits>

#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"  // NullReferenceException
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace M = dotnet::Math;

namespace {

constexpr double kG = 6.67408E-11;  // CelestialBody.cs:461 (literal)
constexpr double kPI = UtilMath::PI;  // System.Math.PI

ICelestialBodyOrbit& RequireOrbit(const CelestialBody& cb) {
    ICelestialBodyOrbit* o = cb.orbit();
    if (o == nullptr) throw NullReferenceException(cb.bodyName + ": CelestialBody.orbit is null (no run-time Orbit attached)");
    return *o;
}

// C# `x % y` on doubles (IEEE fmod semantics, sign of the dividend)
double CsRem(double x, double y) { return std::fmod(x, y); }

}  // namespace

// CBAttributeMapSO.cs:148 (managed float code: Mono float rule)
float CBAttributeMapSO::RGBColorSqrMag(const Color& colA, const Color& colB) {
    float num = f32store(fx(colA.r) - fx(colB.r));
    const float num2 = f32store(fx(num) * fx(num));
    num = f32store(fx(colA.g) - fx(colB.g));
    const float num3 = f32store(fx(num2) + fx(num) * fx(num));
    num = f32store(fx(colA.b) - fx(colB.b));
    return f32store(fx(num3) + fx(num) * fx(num));
}

// ---------------------------------------------------------------------------------------------------------------------
// CelestialBody.cs:363 SetupConstants. Its last statement,
//   minOrbitalDistance = CelestialUtilities.GetMinimumOrbitalDistance(this, 1f)   (FinePrint/Utilities/CelestialUtilities.cs:187)
// needs the PQS highest peak and TimeWarp's altitude limits (contracts only) and is not evaluated here: minOrbitalDistance
// keeps its serialized value until the terrain/TimeWarp packages provide those inputs (the harness skipped it too).
void CelestialBody::SetupConstants() {
    if (atmosphere) {
        atmosphereTemperatureLapseRate = atmosphereTemperatureSeaLevel / atmosphereDepth;
        atmosphereGasMassLapseRate = GeeASL * PhysicsGlobals::GravitationalAcceleration * atmosphereMolarMass /
                                     (PhysicsGlobals::IdealGasConstant() * atmosphereTemperatureLapseRate);
        radiusAtmoFactor = Radius / atmosphereDepth * (0.0 - M::Log(1E-06));
        atmDensityASL = GetDensity(GetPressure(0.0), GetTemperature(0.0));
        atmPressureASL = GetPressure(0.0) * 0.009869232667160128;
        if (clampInverseRotThreshold && static_cast<double>(inverseRotThresholdAltitude) < atmosphereDepth)
            inverseRotThresholdAltitude = static_cast<float>(atmosphereDepth);
    }
    const double num = Radius * Radius;
    Density = Mass / (4.1887902047863905 * num * Radius);
    SurfaceArea = kPI * 4.0 * num;
    if (orbitDriver) maxAxialDot = M::Sin(UtilMath::DegreesToRadians(orbitDriver->orbit.inclination));
}

// CelestialBody.cs:387
double CelestialBody::GetPressure(double altitude) const {
    if (atmosphere && altitude < atmosphereDepth) {
        if (atmosphereUsePressureCurve) {
            if (atmospherePressureCurveIsNormalized) {
                return Mathf::Lerp(0.0f, static_cast<float>(atmospherePressureSeaLevel),
                                   atmospherePressureCurve.Evaluate(static_cast<float>(altitude / atmosphereDepth)));
            }
            return atmospherePressureCurve.Evaluate(static_cast<float>(altitude));
        }
        return atmospherePressureSeaLevel *
               M::Pow(1.0 - atmosphereTemperatureLapseRate * altitude / atmosphereTemperatureSeaLevel, atmosphereGasMassLapseRate);
    }
    return 0.0;
}

// CelestialBody.cs:404
double CelestialBody::GetPressureAtm(double altitude) const { return GetPressure(altitude) * 0.009869232667160128; }

// CelestialBody.cs:409 (no `atmosphere` check, as in KSP)
double CelestialBody::GetTemperature(double altitude) const {
    if (altitude >= atmosphereDepth) return 0.0;
    if (atmosphereUseTemperatureCurve) {
        if (atmosphereTemperatureCurveIsNormalized) {
            return UtilMath::Lerp(PhysicsGlobals::SpaceTemperature(), atmosphereTemperatureSeaLevel,
                                  atmosphereTemperatureCurve.Evaluate(static_cast<float>(altitude / atmosphereDepth)));
        }
        return atmosphereTemperatureCurve.Evaluate(static_cast<float>(altitude));
    }
    return atmosphereTemperatureSeaLevel - atmosphereTemperatureLapseRate * altitude;
}

// CelestialBody.cs:426
double CelestialBody::GetDensity(double pressure, double temperature) const {
    if (!(pressure <= 0.0) && temperature > 0.0)
        return pressure * 1000.0 * atmosphereMolarMass / (PhysicsGlobals::IdealGasConstant() * temperature);
    return 0.0;
}

// CelestialBody.cs:435
double CelestialBody::GetSpeedOfSound(double pressure, double density) const {
    if (!(pressure <= 0.0) && density > 0.0) return M::Sqrt(atmosphereAdiabaticIndex * (pressure * 1000.0 / density));
    return 0.0;
}

// CelestialBody.cs:444 (Planetarium.fetch.Home must exist when there is a Planetarium, as in KSP)
double CelestialBody::GetSolarPowerFactor(double density) const {
    double num = 1.225;
    if (Planetarium::fetch != nullptr) {
        if (Planetarium::fetch->Home == nullptr) throw NullReferenceException("GetSolarPowerFactor: Planetarium.Home is null");
        if (Planetarium::fetch->Home->atmosphereDepth > 0.0) num = Planetarium::fetch->Home->atmDensityASL;
    }
    const double num2 = (1.0 - PhysicsGlobals::SolarInsolationAtHome()) * num;
    return num2 / (num2 + density * PhysicsGlobals::SolarInsolationAtHome());
}

// CelestialBody.cs:455 Awake. Transform/PQS lookups, Localizer.Format of the display names, DiscoveryInfo and
// BiomeMap.FormatLocalizationTags are engine/UI work. `orbitDriver.referenceBody.orbitingBodies.Add(this)` runs at
// Instantiate time against the *prefab* parent in KSP (SpawnBody re-registers the instance), so it is the spawn's job here.
void CelestialBody::Awake() {
    gMagnitudeAtCenter = GeeASL * PhysicsGlobals::GravitationalAcceleration * M::Pow(Radius, 2.0);
    Mass = Radius * Radius * (GeeASL * PhysicsGlobals::GravitationalAcceleration) / kG;
    gravParameter = Mass * kG;
}

// CelestialBody.cs:831
double CelestialBody::TerrainAltitude(double latitude, double longitude, bool allowNegative) const {
    if (pqsController == nullptr) return 0.0;
    return terrain::TerrainAltitude(pqsController, latitude, longitude, allowNegative);
}

// CelestialBody.cs:477 Start (WP-10 added the TimeWarp limit-count check); AtmosphereFromGround and PQS mod wiring
// are engine-side.
void CelestialBody::Start() {
    if (orbitDriver) {
        const CelestialOrbitElements& orbit = orbitDriver->orbit;
        const CelestialBody& referenceBody = *orbitDriver->referenceBody;
        sphereOfInfluence = orbit.semiMajorAxis * M::Pow(Mass / referenceBody.Mass, 0.4);
        hillSphere = orbit.semiMajorAxis * (1.0 - orbit.eccentricity) * M::Pow(Mass / referenceBody.Mass, 1.0 / 3.0);
        if (solarRotationPeriod) {
            const double num = rotationPeriod;
            const double num2 = kPI * 2.0 * M::Sqrt(M::Pow(M::Abs(orbit.semiMajorAxis), 3.0) / referenceBody.gravParameter);
            const double time = num * num2 / (num2 + num);
            log::Info("[CelestialBody]: " + bodyName + "'s solar day length is " + dotnet::FormatDoubleG(num) +
                      " s long. sidereal day length is " + dotnet::FormatDoubleG(time) + " s long");
            rotationPeriod = time;
        }
    } else {
        sphereOfInfluence = std::numeric_limits<double>::infinity();
        hillSphere = std::numeric_limits<double>::infinity();
    }
    SetupConstants();
    if (TimeWarp::fetch != nullptr && timeWarpAltitudeLimits.size() != TimeWarp::fetch->altitudeLimits.size()) {
        log::Warning("Warp Rate limits for " + bodyName + " didn't match warp rate count. Limits reset to default");
        resetTimeWarpLimits();
    }
    oceanFogPQSDepthRecip = static_cast<float>(1.0 / oceanFogPQSDepth);
}

// CelestialBody.cs:1013 (debugTimeWarpLimits only prints)
void CelestialBody::resetTimeWarpLimits() {
    const TimeWarp* fetch = TimeWarp::fetch;
    if (fetch == nullptr) throw NullReferenceException("CelestialBody.resetTimeWarpLimits: TimeWarp.fetch");
    timeWarpAltitudeLimits.assign(fetch->altitudeLimits.size(), 0.0f);
    for (size_t i = 0; i < fetch->altitudeLimits.size(); ++i)
        timeWarpAltitudeLimits[i] = f32store(fx(fetch->altitudeLimits[i]) * fx(static_cast<float>(Radius)) *
                                             fx(GameSettings::ORBIT_WARP_ALTMODE_LIMIT_MODIFIER));
}

// CelestialBody.cs:535 CBUpdate. transformRight/Up and bodyTransform.rotation are the shell's (it reads `rotation`).
void CelestialBody::CBUpdate() {
    gMagnitudeAtCenter = GeeASL * PhysicsGlobals::GravitationalAcceleration * Radius * Radius;
    Mass = Radius * Radius * (GeeASL * PhysicsGlobals::GravitationalAcceleration) / kG;
    gravParameter = Mass * kG;
    ICelestialBodyOrbit* o = orbit();
    if (rotates && rotationPeriod != 0.0 && (!tidallyLocked || (o != nullptr && o->period() != 0.0))) {
        if (tidallyLocked) rotationPeriod = o->period();
        rotPeriodRecip = 1.0 / rotationPeriod;
        angularVelocity = Vector3d::down() * (kPI * 2.0 * rotPeriodRecip);
        zUpAngularVelocity = Vector3d::back() * (kPI * 2.0 * rotPeriodRecip);
        rotationAngle = CsRem(initialRotation + 360.0 * rotPeriodRecip * Planetarium::GetUniversalTime(), 360.0);
        angularV = angularVelocity.magnitude();
        if (!inverseRotation) {
            directRotAngle = CsRem(rotationAngle - Planetarium::InverseRotAngle(), 360.0);
            CelestialFrame::PlanetaryFrame(0.0, 90.0, directRotAngle, BodyFrame);
            rotation = BodyFrame.Rotation().swizzle();
        } else {
            Planetarium::SetInverseRotAngle(CsRem(rotationAngle - directRotAngle, 360.0));
            CelestialFrame::PlanetaryFrame(0.0, 90.0, Planetarium::InverseRotAngle(), Planetarium::Zup);
            Planetarium::SetRotation(QuaternionD::Inverse(Planetarium::Zup.Rotation()).swizzle());
        }
    }
    if (orbitDriver && orbitDriver->link != nullptr) orbitDriver->link->UpdateOrbit();
    if (Planetarium::fetch == nullptr) throw NullReferenceException("CBUpdate: no Planetarium (FlightGlobals.Bodies[0] fallback not ported)");
    const CelestialBody* celestialBody = this;
    const CelestialBody* celestialBody2 = Planetarium::fetch->Sun;
    while (celestialBody->referenceBody() != celestialBody2 && celestialBody->referenceBody() != nullptr)
        celestialBody = celestialBody->referenceBody();
    if (ICelestialBodyOrbit* ro = celestialBody->orbit()) {
        const double num = ro->period() - rotationPeriod;
        solarDayLength = (num == 0.0) ? std::numeric_limits<double>::max() : ro->period() * rotationPeriod / num;
    } else {
        solarDayLength = 1.0;
    }
}

// CelestialBody.cs:971
void CelestialBody::GetSolarAtmosphericEffects(double sunDot, double density, double& solarAirMass,
                                               double& solarFluxMultiplier) const {
    const double num = radiusAtmoFactor * sunDot;
    if (num < 0.0)
        solarAirMass = M::Sqrt(2.0 * radiusAtmoFactor + 1.0);
    else
        solarAirMass = M::Sqrt(num * num + 2.0 * radiusAtmoFactor + 1.0) - num;
    solarFluxMultiplier = GetSolarPowerFactor(density * solarAirMass);
}

// CelestialBody.cs:985
double CelestialBody::GetFullTemperature(double altitude, double atmoSunBasedTempOffset) const {
    return GetTemperature(altitude) +
           static_cast<double>(atmosphereTemperatureSunMultCurve.Evaluate(static_cast<float>(altitude))) * atmoSunBasedTempOffset;
}

// ---------------------------------------------------------------------------------------------------------------------
// frames (CelestialBody.cs:610-695)
Vector3d CelestialBody::GetFrameVel() const {
    if (orbitDriver && orbitDriver->enabled) {
        const ICelestialBodyOrbit& o = RequireOrbit(*this);
        return o.vel() + (orbitDriver->referenceBody != nullptr ? orbitDriver->referenceBody->GetFrameVel() : Vector3d::zero());
    }
    return Vector3d::zero();
}

Vector3d CelestialBody::GetFrameVelAtUT(double UT) const {
    if (orbitDriver && orbitDriver->enabled) {
        const ICelestialBodyOrbit& o = RequireOrbit(*this);
        return o.getOrbitalVelocityAtUT(UT) +
               (orbitDriver->referenceBody != nullptr ? orbitDriver->referenceBody->GetFrameVelAtUT(UT) : Vector3d::zero());
    }
    return Vector3d::zero();
}

Vector3d CelestialBody::getRFrmVel(const Vector3d& worldPos) const { return Vector3d::Cross(angularVelocity, worldPos - position); }

// CelestialBody.cs:633
Vector3d CelestialBody::getRFrmVelOrbit(const Orbit& o) const { return Vector3d::Cross(angularVelocity, o.pos.xzy()); }

Vector3d CelestialBody::getTruePositionAtUT(double UT) const {
    if (orbitDriver && orbitDriver->enabled)
        return RequireOrbit(*this).getRelativePositionAtUT(UT).xzy() + orbitDriver->referenceBody->getTruePositionAtUT(UT);
    return position;
}

Vector3d CelestialBody::getPositionAtUT(double UT) const {
    if (orbitDriver && orbitDriver->enabled)
        return RequireOrbit(*this).getRelativePositionAtUT(UT).xzy() + orbitDriver->referenceBody->position;
    return position;
}

Vector3d CelestialBody::GetRelSurfaceNVector(double lat, double lon) const {
    lat *= kPI / 180.0;
    lon *= kPI / 180.0;
    return Planetarium::SphericalVector(lat, lon).xzy();
}

Vector3d CelestialBody::GetSurfaceNVector(double lat, double lon) const {
    lat *= kPI / 180.0;
    lon *= kPI / 180.0;
    const Vector3d r = Planetarium::SphericalVector(lat, lon);
    return BodyFrame.LocalToWorld(r).xzy();
}

Vector3d CelestialBody::GetRelSurfacePosition(double lat, double lon, double alt) const {
    return GetRelSurfaceNVector(lat, lon) * (Radius + alt);
}

Vector3d CelestialBody::GetRelSurfacePosition(const Vector3d& worldPosition) const {
    return BodyFrame.WorldToLocal((worldPosition - position).xzy()).xzy();
}

Vector3d CelestialBody::GetRelSurfaceDirection(const Vector3d& worldDirection) const {
    return BodyFrame.WorldToLocal(worldDirection.xzy()).xzy();
}

Vector3d CelestialBody::GetWorldSurfacePosition(double lat, double lon, double alt) const {
    return BodyFrame.LocalToWorld(GetRelSurfacePosition(lat, lon, alt).xzy()).xzy() + position;
}

// CelestialBody.cs:697-800
double CelestialBody::GetLatitude(const Vector3d& pos, bool isRadial) const {
    Vector3d rPos = isRadial ? pos.normalized() : (pos - position).normalized();
    rPos = BodyFrame.WorldToLocal(rPos.xzy());
    double latValue = M::Asin(rPos.z) * 57.295780181884766;
    if (std::isnan(latValue)) latValue = 0.0;
    return latValue;
}

double CelestialBody::GetLongitude(const Vector3d& pos, bool isRadial) const {
    Vector3d rPos = isRadial ? pos.normalized() : (pos - position).normalized();
    rPos = BodyFrame.WorldToLocal(rPos.xzy());
    double lonValue = M::Atan2(rPos.y, rPos.x) * 57.295780181884766;
    if (std::isnan(lonValue)) lonValue = 0.0;
    return lonValue;
}

Vector2d CelestialBody::GetLatitudeAndLongitude(const Vector3d& pos, bool isRadial) const {
    Vector3d rPos = isRadial ? pos.normalized() : (pos - position).normalized();
    rPos = BodyFrame.WorldToLocal(rPos.xzy());
    double latValue = M::Asin(rPos.z) * 57.295780181884766;
    double lonValue = M::Atan2(rPos.y, rPos.x) * 57.295780181884766;
    if (std::isnan(latValue)) latValue = 0.0;
    if (std::isnan(lonValue)) lonValue = 0.0;
    return Vector2d(latValue, lonValue);
}

bool CelestialBody::GetImpactLatitudeAndLongitude(const Vector3d& pos, const Vector3d& velocity, double& latitude,
                                                  double& longitude) const {
    const double num = Vector3d::Dot(pos, velocity);
    const double num2 = Vector3d::Dot(velocity, velocity);
    const double num3 = Vector3d::Dot(pos, pos);
    const double num4 = num / num2;
    const double num5 = num4 * num4 - (num3 - Radius * Radius) / num2;
    if (num5 >= 0.0) {
        const double num6 = 0.0 - num4 - M::Sqrt(num5);
        const Vector3d vector3d = pos + velocity * num6;
        latitude = M::Asin(vector3d.y / vector3d.magnitude()) * (180.0 / kPI);
        longitude = M::Atan2(vector3d.z, vector3d.x) * (180.0 / kPI);
        return true;
    }
    latitude = 0.0;
    longitude = 0.0;
    return false;
}

double CelestialBody::GetAltitude(const Vector3d& worldPos) const { return (worldPos - position).magnitude() - Radius; }

void CelestialBody::GetLatLonAlt(const Vector3d& worldPos, double& lat, double& lon, double& alt) const {
    Vector3d rPos = BodyFrame.WorldToLocal((worldPos - position).xzy());
    const double magnitude = rPos.magnitude();
    alt = magnitude - Radius;
    rPos = rPos / magnitude;
    lat = M::Asin(rPos.z) * (180.0 / kPI);
    lon = M::Atan2(rPos.y, rPos.x) * (180.0 / kPI);
    if (std::isnan(lat)) lat = 0.0;
    if (std::isnan(lon)) lon = 0.0;
}

void CelestialBody::GetLatLonAltOrbital(const Vector3d& worldPos, double& lat, double& lon, double& alt) const {
    Vector3d rPos = BodyFrame.WorldToLocal(worldPos);
    const double magnitude = rPos.magnitude();
    alt = magnitude - Radius;
    rPos = rPos / magnitude;
    lat = M::Asin(rPos.z) * (180.0 / kPI);
    lon = M::Atan2(rPos.y, rPos.x) * (180.0 / kPI);
    if (std::isnan(lat)) lat = 0.0;
    if (std::isnan(lon)) lon = 0.0;
}

// ---------------------------------------------------------------------------------------------------------------------
// CelestialBody.cs:846
const CelestialBody* CelestialBody::GetBodyReferencing(const CelestialBody* body, const CelestialBody* sun) {
    while (body != nullptr && body->orbit() != nullptr && body->orbitDriver->referenceBody != nullptr &&
           body->orbitDriver->referenceBody != sun)
        body = body->orbitDriver->referenceBody;
    return body;
}

// CelestialBody.cs:1024
bool CelestialBody::HasParent(const CelestialBody* body) const {
    if (referenceBody() == body) return true;
    if (referenceBody() == this) return false;
    return referenceBody()->HasParent(body);
}

// CelestialBody.cs:1037
bool CelestialBody::HasChild(const CelestialBody* body) const {
    for (const CelestialBody* child : orbitingBodies) {
        if (child == body) return true;
        if (child->HasChild(body)) return true;
    }
    return false;
}

}  // namespace ksp
