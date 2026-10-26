// WP-42: PQSCity.cs, PQSCity2.cs (placement part) and CelestialBody.GetRandomLatitudeAndLongitude.
#include "ksp/space_center/pqs_city.hpp"

#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
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

PQSCity::PQSCity(const PQSCityPrefab& p, CelestialBody* b, PQS* s) : body(b) {
    name = p.name;
    sphere = s;
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
        double num = sphereSurfaceHeight ? sphereSurfaceHeight(vector3d) : sphere->GetSurfaceHeight(vector3d, true);
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
        spaceCenterFacility.OrientateSetSpawnPoints();   // :343 (facilityTransform != null)
    }
    GE::OnPQSCityOrientated.Fire(body, name);
}

// PQSCity.cs:156 (body = GetComponentInParent<CelestialBody>(): the host assigns `body`)
void PQSCity::Start() {
    GE::OnPQSCityStarting.Fire(this);
    Orientate(false);
}

// PQSCity.cs:192
void PQSCity::OnPostSetup() { HighLogic::Scheduler().StartCoroutine(PostSetup(), owner); }

// PQSCity.cs:197
Enumerator PQSCity::PostSetup() {
    if (randomizeOnSphere) {
        co_yield YieldInstruction::Null();
        Randomize();
    }
    Orientate();
    CheckLocals();
}

// PQSCity.cs:372
void PQSCity::CheckLocals() {
    loadRange = PQS::GameBindings().GetPQSCityLoadRange(this);
    poiRange = PQS::GameBindings().GetPOIRange(this);
}

// PQSCity.cs:234 (the range half: :252-275; `base.gameObject != null` is always true for a live city)
void PQSCity::OnUpdateFinished(float num2) {
    if (!isLoaded && static_cast<double>(num2) < loadRange) {
        isLoaded = true;
        GE::OnPQSCityLoaded.Fire(body, name);
    } else if (isLoaded && static_cast<double>(num2) > loadRange) {
        isLoaded = false;
        GE::OnPQSCityUnloaded.Fire(body, name);
    }
    if (!inPOIRange && static_cast<double>(num2) < POIRange()) {
        inPOIRange = true;
        GE::OnPOIRangeEntered.Fire(body, name);
    } else if (inPOIRange && static_cast<double>(num2) > POIRange()) {
        inPOIRange = false;
        GE::OnPOIRangeExited.Fire(body, name);
    }
}

// PQSCity.cs:287
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

// PQSCity.cs:394
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

PQSCity2::PQSCity2(const PQSCityPrefab& p, CelestialBody* b, PQS* s) : objectName(p.objectName), displayobjectName(p.displayobjectName), body(b) {
    name = p.name;
    sphere = s;
    OrientateOnStart = p.OrientateOnStart;
    OrientateOnPostSetup = p.OrientateOnPostSetup;
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

// PQSCity2.cs:377
void PQSCity2::Reset() {
    up = Vector3f::up();
    positioningCompleted = false;
    raycastCompleted = false;
    positionedWhenVisible = false;
    inVisibleRange = false;
}

// PQSCity2.cs:386 (body = GetComponentInParent<CelestialBody>(): the host assigns `body`)
void PQSCity2::Start() {
    if (!started_) {
        GE::OnScenerySettingChanged.Add(this, &PQSCity2::OnScenerySettingChanged);
        started_ = true;
    }
    if (OrientateOnStart) Orientate();
}

// PQSCity2.cs:396 OnDestroy
PQSCity2::~PQSCity2() {
    if (started_) GE::OnScenerySettingChanged.Remove(this, &PQSCity2::OnScenerySettingChanged);
}

// PQSCity2.cs:401
void PQSCity2::OnScenerySettingChanged() {
    Reset();
    positionedWhenVisible = false;
}

// PQSCity2.cs:434
void PQSCity2::OnPostSetup() {
    if (OrientateOnPostSetup) Orientate();
    poiRange = PQS::GameBindings().GetPOIRange(this);
}

// PQSCity2.cs:129
std::string PQSCity2::DisplaySurfaceObjectName() const { return KSP::Localization::Localizer::Format(displayobjectName); }

float PQSCity2::DistanceFromOrigin() const {
    if (distanceFromOrigin) return distanceFromOrigin();
    if (!worldPositionOf) return 0.0f;
    return Vector3f::Distance(NarrowToFloat(worldPositionOf(*this)), Vector3f::zero());
}

// PQSCity2.cs:186. Engine-side and not modelled: FloatingOrigin.SetOffset around the snap in the non-flight scenes (no floating origin in
// the core) and the world rotation of baseRotFacesNorthPole (no stock city sets it; a QuaternionD chain on transform.rotation). sic: the
// raycast branch adds snapHeightOffset a second time.
void PQSCity2::Orientate() {
    if (positioningCompleted && (!positioningCompleted || positionedWhenVisible)) return;
    if (setVisibility) setVisibility();
    if (positioningCompleted && !inVisibleRange) return;
    CelestialFrame cf;
    CelestialFrame::SetFrame(0.0, 0.0, 0.0, cf);
    const Vector3d surfaceNVector = LatLonGetSurfaceNVector(cf, lat, lon);
    Vector3d vector3d = surfaceNVector * (sphere->radius + alt);
    planetRelativePosition = vector3d;
    localPosition = NarrowToFloat(planetRelativePosition);
    if (setOnWaterSurface && OrientateOnWater(surfaceNVector)) return;
    double num = 0.0;
    if (snapToSurface) {
        const float num2 = DistanceFromOrigin();
        num = sphereSurfaceHeight ? sphereSurfaceHeight(surfaceNVector) : sphere->GetSurfaceHeight(surfaceNVector, true);
        num += snapHeightOffset;
        if (raycastSurface && !raycastCompleted && fx(num2) < 7000.0) {
            if (orientateRayCast) num = orientateRayCast(surfaceNVector, num, raycastCompleted);
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
        spaceCenterFacility.OrientateSetSpawnPoints();   // :253 (facilityTransform != null)
    }
    positioningCompleted = true;
    if (setVisibility) setVisibility();
    if (inVisibleRange) positionedWhenVisible = true;
    if (positionMobileLaunchPad && (!snapToSurface || !raycastSurface || (raycastSurface && raycastCompleted)) && positionedWhenVisible &&
        positionMobileLaunchPad->CompleteOrientation)
        positionMobileLaunchPad->CompleteOrientation(body, objectName, true);
    if (onOrientated) onOrientated();
}

// PQSCity2.cs:272 in the PQS-local frame of this port (transform.position = the local position of an identity body frame:
// celestialBody.GetWorldSurfacePosition(lat, lon, 0) is the surface direction times the body radius; the up vector is the direction from
// the sphere centre). FloatingOrigin.SetOffset and the north-pole world rotation are engine-side as in Orientate.
bool PQSCity2::OrientateOnWater(const Vector3d& surfPosNorm) {
    const double h = sphereSurfaceHeight ? sphereSurfaceHeight(surfPosNorm) : sphere->GetSurfaceHeight(surfPosNorm, true);
    if (h - sphere->radius < 0.0) {
        float num2 = 0.0f;
        if (positionMobileLaunchPad) num2 = positionMobileLaunchPad->waterModelClearance;
        CelestialFrame cf;
        CelestialFrame::SetFrame(0.0, 0.0, 0.0, cf);
        const Vector3f vector = NarrowToFloat(LatLonGetSurfaceNVector(cf, lat, lon) * (body != nullptr ? body->Radius : sphere->radius));
        const Vector3d normalized = Vector3d(vector).normalized();
        localPosition = vector;
        if (num2 != 0.0f) localPosition = NarrowToFloat(Vector3d(localPosition) + normalized * static_cast<double>(num2));
        if (baseRotFacesNorthPole) {
            // engine-side, as in Orientate
        } else {
            localRotation = UnityNative::FromToRotation(up, NarrowToFloat(surfPosNorm)) * UnityNative::AngleAxis(static_cast<float>(rotation), Vector3f::up());
        }
        positioningCompleted = true;
        raycastSurface = false;
        if (positionMobileLaunchPad && positionMobileLaunchPad->CompleteOrientation) positionMobileLaunchPad->CompleteOrientation(body, objectName, true);
        if (body != nullptr) LatLonAlt(lat, lon, alt);
        if (launchSite != nullptr) launchSite->SetSpawnPointsLatLonAlt();
        spaceCenterFacility.OrientateSetSpawnPoints();
        if (setVisibility) setVisibility();
        if (inVisibleRange) positionedWhenVisible = true;
        GE::OnPQSCityOrientated.Fire(body, objectName);
        if (onOrientated) onOrientated();
        return true;
    }
    setOnWaterSurface = false;
    return false;
}

// PQSCity2.cs:458 (after setVisibility; `base.gameObject != null` is always true for a live city). The target distance is taken after
// a re-orientation (the C# measures it there).
void PQSCity2::OnUpdateFinished(const std::function<float()>& targetDistance) {
    if (inVisibleRange && (!positioningCompleted || !positionedWhenVisible || (raycastSurface && !raycastCompleted))) {
        Reset();
        if (positionMobileLaunchPad && positionMobileLaunchPad->OnScenerySettingChanged) positionMobileLaunchPad->OnScenerySettingChanged();
        Orientate();
    }
    const float num = targetDistance();
    if (!inPOIRange && static_cast<double>(num) < POIRange()) {
        inPOIRange = true;
        GE::OnPOIRangeEntered.Fire(body, name);
    } else if (inPOIRange && static_cast<double>(num) > POIRange()) {
        inPOIRange = false;
        GE::OnPOIRangeExited.Fire(body, name);
    }
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
