// WP-42: SpaceCenter.cs.
#include "ksp/space_center/space_center.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/space_center/pqs_city.hpp"

namespace ksp {

using GE = events::GameEvents;

namespace {
// CelestialBody.GetLatitudeAndLongitude(pos) with an identity body frame at the origin (SpaceCenter.Start on a PQSCity placed
// through AttachToCity): the same operations, WorldToLocal being the identity
Vector2d LatitudeAndLongitudeIdentityFrame(const Vector3d& pos) {
    Vector3d rPos = pos.normalized();
    rPos = rPos.xzy();
    double latValue = dotnet::Math::Asin(rPos.z) * 57.295780181884766;
    double lonValue = dotnet::Math::Atan2(rPos.y, rPos.x) * 57.295780181884766;
    if (std::isnan(latValue)) latValue = 0.0;
    if (std::isnan(lonValue)) lonValue = 0.0;
    return Vector2d(latValue, lonValue);
}
}  // namespace

// KSC placement of a PQSCity: the identity body frame path (see pqs_city.hpp)
void SpaceCenter::AttachToCity(const PQSCity& city) {
    const Vector3d pos(city.localPosition);
    transformPosition = [pos] { return pos; };
    identityFrame = true;
}

// SpaceCenter.cs:40
void SpaceCenter::Awake() {
    if (Instance != nullptr) {
        destroyed = true;   // UnityEngine.Object.Destroy(this)
        return;
    }
    Instance = this;
    registered_ = true;
    GE::onLevelWasLoaded.Add(this, &SpaceCenter::OnGameSceneLoaded);
}

// SpaceCenter.cs:51
void SpaceCenter::Start() {
    const Vector3d position = transformPosition ? transformPosition() : Vector3d();
    const Vector2d latitudeAndLongitude = identityFrame ? LatitudeAndLongitudeIdentityFrame(position) : cb->GetLatitudeAndLongitude(position);
    latitude_ = latitudeAndLongitude.x;
    longitude_ = latitudeAndLongitude.y;
    srfNVector_ = cb->GetRelSurfaceNVector(latitude_, longitude_);
}

// SpaceCenter.cs:61
void SpaceCenter::OnDestroy() {
    if (registered_) {
        GE::onLevelWasLoaded.Remove(this, &SpaceCenter::OnGameSceneLoaded);
        registered_ = false;
    }
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

SpaceCenter::~SpaceCenter() { OnDestroy(); }

// SpaceCenter.cs:70
void SpaceCenter::OnGameSceneLoaded(GameScenes scn) {
    if (setAreaTriggerActive) setAreaTriggerActive(scn != GameScenes::SPACECENTER);
}

// SpaceCenter.cs:82
double SpaceCenter::GreatCircleDistance(const Vector3d& fromSrfNrm) const {
    return cb->Radius * dotnet::Math::Acos(Vector3d::Dot(srfNVector_, fromSrfNrm));
}

}  // namespace ksp
