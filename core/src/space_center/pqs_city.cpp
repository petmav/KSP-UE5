// WP-42: PQSCity.cs, PQSCity2.cs (placement part) and CelestialBody.GetRandomLatitudeAndLongitude.
#include "ksp/space_center/pqs_city.hpp"

#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/launch_site.hpp"
#include "ksp/space_center/space_center.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/util/misc_util.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {

constexpr double kPI = 3.141592653589793;

// the lat / lon / alt of a body-relative position in an identity body frame (CelestialBody.GetLatLonAlt with WorldToLocal = identity;
// the xzy swizzle of the world position is part of GetLatLonAlt)
void LatLonAltIdentity(const Vector3d& localPos, double radius, double& lat, double& lon, double& alt) {
    Vector3d rPos = localPos.xzy();
    const double magnitude = rPos.magnitude();
    alt = magnitude - radius;
    rPos = rPos / magnitude;
    lat = dotnet::Math::Asin(rPos.z) * (180.0 / kPI);
    lon = dotnet::Math::Atan2(rPos.y, rPos.x) * (180.0 / kPI);
    if (std::isnan(lat)) lat = 0.0;
    if (std::isnan(lon)) lon = 0.0;
}

Vector3f ToVector3f(const std::array<float, 3>& a) { return {a[0], a[1], a[2]}; }

Vector3f NarrowToFloat(const Vector3d& v) {
    return {static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)};
}

}  // namespace

// LatLon.cs:13
Vector3d LatLonGetSurfaceNVector(const CelestialFrame& cf, double lat, double lon) {
    lat *= kPI / 180.0;
    lon *= kPI / 180.0;
    const Vector3d r = Planetarium::SphericalVector(lat, lon);
    return cf.LocalToWorld(r).xzy();
}

// ---- PQSCity ----------------------------------------------------------------------------------------------------------------

PQSCity::PQSCity(const PQSCityPrefab& p, CelestialBody* b, PQS* s) : name(p.name), body(b), sphere(s) {
    randomizeOnSphere = p.randomizeOnSphere;
    repositionToSphere = p.repositionToSphere;
    repositionToSphereSurface = p.repositionToSphereSurface;
    repositionToSphereSurfaceAddHeight = p.repositionToSphereSurfaceAddHeight;
    repositionRadial = ToVector3f(p.repositionRadial);
    repositionRadiusOffset = p.repositionRadiusOffset;
    reorientToSphere = p.reorientToSphere;
    reorientInitialUp = ToVector3f(p.reorientInitialUp);
    reorientFinalAngle = p.reorientFinalAngle;
    useIndividualPOIRange = p.useIndividualPOIRange;
    individualPOIRange = p.individualPOIRange;
}

void PQSCity::LatLonAlt(double& latitude, double& longitude, double& altitude) const {
    if (worldPositionOf)
        body->GetLatLonAlt(worldPositionOf(*this), latitude, longitude, altitude);
    else
        LatLonAltIdentity(Vector3d(localPosition), sphere != nullptr ? sphere->radius : body->Radius, latitude, longitude, altitude);
}

// PQSCity.cs:306
void PQSCity::Orientate(bool allowReposition) {
    if (body == nullptr) ResetCelestialBody();
    const Vector3d vector3d(repositionRadial.normalized());
    if (repositionToSphere) {
        planetRelativePosition = vector3d * (sphere->radius + repositionRadiusOffset);
        localPosition = NarrowToFloat(planetRelativePosition);
    }
    if (repositionToSphereSurface && allowReposition) {
        double num = sphereSurfaceHeight ? sphereSurfaceHeight(vector3d) : sphere->GetSurfaceHeight(vector3d);
        if (repositionToSphereSurfaceAddHeight) num += repositionRadiusOffset;
        planetRelativePosition = vector3d * num;
        localPosition = NarrowToFloat(planetRelativePosition);
    }
    if (reorientToSphere) {
        localRotation = UnityNative::FromToRotation(reorientInitialUp, localPosition.normalized()) *
                        UnityNative::AngleAxis(reorientFinalAngle, Vector3f::up());
    }
    if (body == nullptr) {
        log::Warning("[PQSCity]: " + name + " is not parented to a valid CelestialBody.");
    } else {
        LatLonAlt(lat, lon, alt);
        if (launchSite != nullptr && launchSite->HasLaunchSiteTransform()) launchSite->SetSpawnPointsLatLonAlt();
    }
    GE::OnPQSCityOrientated.Fire(body, name);
}

// PQSCity.cs:272
void PQSCity::Randomize() {
    repositionToSphereSurface = true;
    if (body == nullptr) {
        log::Warning("[PQSCity]: " + name + " is not parented to a valid CelestialBody.");
        return;
    }
    KSPRandom kSPRandom(GetPQSCitySeed(*this));
    double latitude = 0.0;
    double longitude = 0.0;
    GetRandomLatitudeAndLongitude(*body, latitude, longitude, false, false, &kSPRandom);
    CelestialFrame cf;
    CelestialFrame::SetFrame(0.0, 0.0, 0.0, cf);
    const Vector3d surfaceNVector = LatLonGetSurfaceNVector(cf, latitude, longitude);
    const Vector3d radial = surfaceNVector * sphere->radius;
    repositionRadial = NarrowToFloat(radial);
    reorientFinalAngle = static_cast<float>(kSPRandom.NextDouble() * 360.0);
}

// PQSCity.cs:378
int PQSCity::GetPQSCitySeed(const PQSCity& city) {
    if (city.body != nullptr) return GetPQSCitySeed(city.name, city.body->bodyName);
    return 0;
}

// PQSCity.cs:387
int PQSCity::GetPQSCitySeed(const std::string& name, const std::string& body) {
    const int32_t hashCode_Net = StringExtensions::GetHashCode_Net35(name + body);
    Game* game = SaveEnvironment::Current().currentGame;
    const int32_t num = game != nullptr ? game->Seed : 0;
    // int arithmetic wraps (unchecked)
    const uint32_t a = static_cast<uint32_t>(num) + static_cast<uint32_t>(hashCode_Net);
    const uint32_t b = a + 1u;
    const int32_t prod = static_cast<int32_t>(a * b);
    // C# int division truncates toward zero; INT_MIN / 2 is exact
    return static_cast<int32_t>(static_cast<uint32_t>(prod / 2) + static_cast<uint32_t>(hashCode_Net));
}

// PQSCity.cs:395
int PQSCity::GetStableHashCode(const std::string& str, bool Bit32) {
    uint32_t num = 5381u;
    if (Bit32) num = 352654597u;
    uint32_t num2 = num;
    const int n = static_cast<int>(str.size());
    for (int i = 0; i < n && str[static_cast<size_t>(i)] != 0; i += 2) {
        num = ((num << 5) + num) ^ static_cast<uint32_t>(static_cast<unsigned char>(str[static_cast<size_t>(i)]));
        if (i == n - 1 || str[static_cast<size_t>(i + 1)] == '\0') break;
        num2 = ((num2 << 5) + num2) ^ static_cast<uint32_t>(static_cast<unsigned char>(str[static_cast<size_t>(i + 1)]));
    }
    return static_cast<int32_t>(num + num2 * 1566083941u);
}

// ---- PQSCity2 ---------------------------------------------------------------------------------------------------------------

PQSCity2::PQSCity2(const PQSCityPrefab& p, CelestialBody* b, PQS* s) : name(p.name), objectName(p.objectName), displayobjectName(p.displayobjectName), body(b), sphere(s) {
    lat = p.lat;
    lon = p.lon;
    alt = p.alt;
    up = ToVector3f(p.up);
    rotation = p.rotation;
    snapToSurface = p.snapToSurface;
    raycastSurface = p.raycastSurface;
    setOnWaterSurface = p.setOnWaterSurface;
    snapHeightOffset = p.snapHeightOffset;
    baseRotFacesNorthPole = p.baseRotFacesNorthPole;
    useIndividualPOIRange = p.useIndividualPOIRange;
    individualPOIRange = p.individualPOIRange;
}

void PQSCity2::LatLonAlt(double& latitude, double& longitude, double& altitude) const {
    if (worldPositionOf)
        body->GetLatLonAlt(worldPositionOf(*this), latitude, longitude, altitude);
    else
        LatLonAltIdentity(Vector3d(localPosition), sphere != nullptr ? sphere->radius : body->Radius, latitude, longitude, altitude);
}

// PQSCity2.cs:390
void PQSCity2::Reset() {
    up = Vector3f::up();
    positioningCompleted = false;
    raycastCompleted = false;
    positionedWhenVisible = false;
    inVisibleRange = false;
}

// PQSCity2.cs:186 (the setOnWaterSurface branch needs engine world transforms: deferred; baseRotFacesNorthPole uses Quaternion.LookRotation /
// Euler, UNVERIFIED natives of UnityNative)
void PQSCity2::Orientate() {
    if (positioningCompleted && (!positioningCompleted || positionedWhenVisible)) return;
    if (positioningCompleted && !inVisibleRange) return;
    CelestialFrame cf;
    CelestialFrame::SetFrame(0.0, 0.0, 0.0, cf);
    const Vector3d surfaceNVector = LatLonGetSurfaceNVector(cf, lat, lon);
    Vector3d vector3d = surfaceNVector * (sphere->radius + alt);
    planetRelativePosition = vector3d;
    localPosition = NarrowToFloat(planetRelativePosition);
    double num = 0.0;
    if (snapToSurface) {
        num = sphereSurfaceHeight ? sphereSurfaceHeight(surfaceNVector) : sphere->GetSurfaceHeight(surfaceNVector);
        num += snapHeightOffset;
        if (raycastSurface && !raycastCompleted && orientateRayCast) {
            num = orientateRayCast(surfaceNVector, num, raycastCompleted);
            num += snapHeightOffset;
        }
        alt = num - sphere->radius;
        planetRelativePosition = surfaceNVector * num;
        localPosition = NarrowToFloat(planetRelativePosition);
    }
    if (baseRotFacesNorthPole) {
        // QuaternionD chain on the transform: engine-side (needs transform.rotation); the local-frame result is not modelled
    } else {
        localRotation = UnityNative::FromToRotation(up, NarrowToFloat(surfaceNVector)) * UnityNative::AngleAxis(static_cast<float>(rotation), Vector3f::up());
    }
    if (body == nullptr) {
        log::Warning("[PQSCity]: " + name + " is not parented to a valid CelestialBody.");
    } else {
        LatLonAlt(lat, lon, alt);
        if (launchSite != nullptr) launchSite->SetSpawnPointsLatLonAlt();
    }
    positioningCompleted = true;
    inVisibleRange = false;
    if (onPositioned) onPositioned();
}

// ---- CelestialBody.GetRandomLatitudeAndLongitude -------------------------------------------------------------------------------

// CelestialBody.cs:799
void GetRandomLatitudeAndLongitude(CelestialBody& body, double& latitude, double& longitude, bool waterAllowed, bool spaceCenterAllowed, KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(SaveEnvironment::Current().TickCount() ^ SaveEnvironment::Current().NewGuidHashCode());
        generator = own.get();
    }
    if (!body.hasSolidSurface) waterAllowed = true;
    const bool flag = SpaceCenter::Instance != nullptr && SpaceCenter::Instance->cb == &body;
    for (int num = 1000; num >= 0; num--) {
        double num2 = generator->NextDouble();
        num2 = 1.0 - num2 * 2.0;
        latitude = dotnet::Math::Asin(num2) * 57.295780181884766;
        longitude = generator->NextDouble() * 360.0 - 180.0;
        if (!body.ocean || waterAllowed || !(body.TerrainAltitude(latitude, longitude, true) < 0.0)) {
            bool flag2 = false;
            if (flag && !spaceCenterAllowed && SpaceCenter::Instance->GreatCircleDistance(body.GetRelSurfaceNVector(latitude, longitude)) <= 4000.0) flag2 = true;
            if (!flag2) break;
        }
    }
}

}  // namespace ksp
