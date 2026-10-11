// WP-38: FinePrint/Waypoint.cs (data side) and the static random-position helpers of FinePrint/WaypointManager.cs.
#include "ksp/contracts/waypoints.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/contracts/contract_environment.hpp"
#include "ksp/contracts/fineprint_lite.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp::FinePrint {

namespace M = ksp::dotnet::Math;
namespace CU = ksp::Contracts::Lite::CelestialUtilities;

namespace {
CelestialBody* FindBody(const std::string& celestialName) {
    CelestialBody* celestialBody = nullptr;
    const auto& bodies = FlightRegistry::Bodies();
    int count = static_cast<int>(bodies.size());
    while (count-- > 0) {
        CelestialBody* celestialBody2 = bodies[static_cast<size_t>(count)];
        if (celestialBody2->GetName() == celestialName) celestialBody = celestialBody2;
    }
    return celestialBody;
}
// the `kSPRandom ?? new KSPRandom()` of every call: a null generator draws a fresh tick seeded one (ContractsEnvironment::newRandomSeed)
struct Gen {
    KSPRandom* g;
    std::unique_ptr<KSPRandom> own;
    explicit Gen(KSPRandom* generator, int seedIfNull, bool seedIsUnique) : g(generator) {
        if (g == nullptr) {
            own = std::make_unique<KSPRandom>(seedIsUnique ? seedIfNull : Contracts::ContractsEnvironment::Current().NewRandomSeed());
            g = own.get();
        }
    }
};
}  // namespace

// Waypoint.cs:126
Waypoint::Waypoint() { navigationId = Contracts::ContractsEnvironment::Current().NewGuid(); }

// Waypoint.cs:131 (the map node / position caches are GUI and are not copied)
Waypoint::Waypoint(const Waypoint& that)
    : seed(that.seed), index(that.index), id(that.id), name(that.name), celestialName(that.celestialName), latitude(that.latitude), longitude(that.longitude),
      altitude(that.altitude), height(that.height), isOccluded(that.isOccluded), isExplored(that.isExplored), isClustered(that.isClustered),
      isOnSurface(that.isOnSurface), isNavigatable(that.isNavigatable), visible(that.visible), landLocked(that.landLocked),
      enableMarker(that.enableMarker), enableTooltip(that.enableTooltip), iconSize(that.iconSize), alphaRatio(that.alphaRatio), iconAlpha(that.iconAlpha),
      fadeRange(that.fadeRange), navigationId(that.navigationId) {
    if (navigationId == Guid::Empty()) navigationId = Contracts::ContractsEnvironment::Current().NewGuid();
}

// Waypoint.cs:100
CelestialBody* Waypoint::celestialBody() {
    if (cachedBody_ != nullptr && cachedBody_->GetName() == celestialName) return cachedBody_;
    const auto& bodies = FlightRegistry::Bodies();
    cachedBody_ = !bodies.empty() ? bodies[0] : nullptr;
    int count = static_cast<int>(bodies.size());
    while (count-- > 0) {
        CelestialBody* cb = bodies[static_cast<size_t>(count)];
        if (cb->GetName() == celestialName) cachedBody_ = cb;
    }
    return cachedBody_;
}

// Waypoint.cs:189
void Waypoint::RandomizeNear(double centerLatitude, double centerLongitude, double minimumDistance, double maximumDistance, bool waterAllowed,
                             KSPRandom* generator) {
    CelestialBody* body = celestialBody();
    if (body == nullptr) return;
    Gen gen(generator, uniqueSeed(), true);
    KSPRandom* kSPRandom = gen.g;
    if (!body->hasSolidSurface) waterAllowed = true;
    if (body->ocean && !waterAllowed) landLocked = true;
    int num = 10000;
    while (num >= 0) {
        latitude = 3.141592653589793 / 2.0 - centerLatitude * (3.141592653589793 / 180.0);
        longitude = centerLongitude * (3.141592653589793 / 180.0);
        const double num2 = minimumDistance + kSPRandom->NextDouble() * (maximumDistance - minimumDistance);
        const double num3 = 3.141592653589793 / 2.0 - num2 / body->Radius;
        const double num4 = 3.141592653589793 * 2.0 * kSPRandom->NextDouble();
        const Vector3d vector3d(M::Cos(num3) * M::Sin(num4), M::Cos(num3) * M::Cos(num4), M::Sin(num3));
        const Vector3d vector3d2(vector3d.x, vector3d.y * M::Cos(latitude) + vector3d.z * M::Sin(latitude),
                                 (0.0 - vector3d.y) * M::Sin(latitude) + vector3d.z * M::Cos(latitude));
        const Vector3d vector3d3(vector3d2.x * M::Cos(longitude) + vector3d2.y * M::Sin(longitude),
                                 (0.0 - vector3d2.x) * M::Sin(longitude) + vector3d2.y * M::Cos(longitude), vector3d2.z);
        latitude = M::Asin(vector3d3.z) * (180.0 / 3.141592653589793);
        longitude = M::Atan2(vector3d3.x, vector3d3.y) * (180.0 / 3.141592653589793);
        if (!(!body->ocean || waterAllowed) && !(CU::TerrainAltitude(body, latitude, longitude, true) >= 0.0)) {
            maximumDistance *= 1.05;
            num--;
            continue;
        }
        break;
    }
}

// Waypoint.cs:227
void Waypoint::RandomizeNear(double centerLatitude, double centerLongitude, double maximumDistance, bool waterAllowed, KSPRandom* generator) {
    Gen gen(generator, uniqueSeed(), true);
    RandomizeNear(centerLatitude, centerLongitude, 0.0, maximumDistance, waterAllowed, gen.g);
}

// Waypoint.cs:233
void Waypoint::RandomizeAwayFrom(double originLatitude, double originLongitude, double minimumDistance, double maximumDistance, int samples,
                                 bool waterAllowed, KSPRandom* generator) {
    CelestialBody* body = celestialBody();
    if (body == nullptr) return;
    Gen gen(generator, uniqueSeed(), true);
    const double centerLatitude = latitude;
    const double centerLongitude = longitude;
    std::vector<std::pair<double, double>> list;
    for (int num = samples; num > 0; num--) {
        RandomizeNear(centerLatitude, centerLongitude, minimumDistance, maximumDistance, waterAllowed, gen.g);
        list.emplace_back(latitude, longitude);
    }
    std::pair<double, double> keyValuePair(originLatitude, originLongitude);
    double num2 = 0.0;
    int count = static_cast<int>(list.size());
    while (count-- > 0) {
        const std::pair<double, double> keyValuePair2 = list[static_cast<size_t>(count)];
        if (CU::GreatCircleDistance(body, keyValuePair2.first, keyValuePair2.second, originLatitude, originLongitude) > num2) {
            keyValuePair = keyValuePair2;
            num2 = CU::GreatCircleDistance(body, keyValuePair.first, keyValuePair.second, originLatitude, originLongitude);
        }
    }
    latitude = keyValuePair.first;
    longitude = keyValuePair.second;
}

// Waypoint.cs:264
void Waypoint::RandomizeAwayFrom(double originLatitude, double originLongitude, double maximumDistance, int samples, bool waterAllowed,
                                 KSPRandom* generator) {
    Gen gen(generator, uniqueSeed(), true);
    RandomizeAwayFrom(originLatitude, originLongitude, 0.0, maximumDistance, samples, waterAllowed, gen.g);
}

// WaypointManager.cs:227 (kSPRandom is never null here: the C# `kSPRandom?.NextDouble() ?? generator.NextDouble() * 360.0 - 180.0` therefore always
// takes the left side, so the longitude is a plain NextDouble() in [0, 1))
void WaypointManager::ChooseRandomPosition(double& latitude, double& longitude, const std::string& celestialName, bool waterAllowed, bool equatorial,
                                           KSPRandom* generator) {
    Gen gen(generator, 0, false);
    KSPRandom* kSPRandom = gen.g;
    latitude = 0.0;
    longitude = 0.0;
    CelestialBody* celestialBody = FindBody(celestialName);
    if (celestialBody == nullptr) return;
    if (!celestialBody->hasSolidSurface) waterAllowed = true;
    if (celestialBody->ocean && !waterAllowed) {
        int num = 10000;
        while (num >= 0) {
            if (!equatorial) {
                double num2 = kSPRandom->NextDouble();
                num2 = 1.0 - num2 * 2.0;
                latitude = M::Asin(num2) * 57.295780181884766;
            } else {
                latitude = 0.0;
            }
            longitude = kSPRandom->NextDouble();
            if (!(CU::TerrainAltitude(celestialBody, latitude, longitude, true) > 0.0)) {
                num--;
                continue;
            }
            break;
        }
    } else {
        if (!equatorial) {
            double num3 = kSPRandom->NextDouble();
            num3 = 1.0 - num3 * 2.0;
            latitude = M::Asin(num3) * 57.295780181884766;
        } else {
            latitude = 0.0;
        }
        longitude = kSPRandom->NextDouble();
    }
}

// WaypointManager.cs:290
void WaypointManager::ChooseRandomPositionNear(double& latitude, double& longitude, double centerLatitude, double centerLongitude,
                                               const std::string& celestialName, double searchRadius, bool waterAllowed, KSPRandom* generator) {
    latitude = 0.0;
    longitude = 0.0;
    Gen gen(generator, 0, false);
    KSPRandom* kSPRandom = gen.g;
    CelestialBody* celestialBody = FindBody(celestialName);
    if (celestialBody == nullptr) return;
    if (!celestialBody->hasSolidSurface) waterAllowed = true;
    const auto cosTerm = [&](double num) {
        // (double)Mathf.Abs(Mathf.Cos((float)Math.PI / 180f * (float)centerLatitude) * (float)num)
        constexpr float kDeg2Rad = 3.14159274f / 180.0f;  // the C# constant (float)Math.PI / 180f folded in float
        const float c = Mathf::Cos(f32store(fx(kDeg2Rad) * fx(static_cast<float>(centerLatitude))));
        return static_cast<double>(Mathf::Abs(f32store(fx(c) * fx(static_cast<float>(num)))));
    };
    if (celestialBody->ocean && !waterAllowed) {
        int num = 10000;
        while (num >= 0) {
            const double num2 = celestialBody->Radius * 2.0 * 3.1415927410125732 / 360.0;
            const double num3 = centerLongitude - searchRadius / cosTerm(num2);
            const double num4 = centerLongitude + searchRadius / cosTerm(num2);
            const double num5 = centerLatitude - searchRadius / num2;
            const double num6 = centerLatitude + searchRadius / num2;
            latitude = num5 + kSPRandom->NextDouble() * (num6 - num5);
            longitude = num3 + kSPRandom->NextDouble() * (num4 - num3);
            if (!(CU::TerrainAltitude(celestialBody, latitude, longitude, true) > 0.0)) {
                searchRadius *= 1.05;
                num--;
                continue;
            }
            break;
        }
    } else {
        const double num7 = celestialBody->Radius * 2.0 * 3.1415927410125732 / 360.0;
        const double num8 = centerLongitude - searchRadius / cosTerm(num7);
        const double num9 = centerLongitude + searchRadius / cosTerm(num7);
        const double num10 = centerLatitude - searchRadius / num7;
        const double num11 = centerLatitude + searchRadius / num7;
        latitude = num10 + kSPRandom->NextDouble() * (num11 - num10);
        longitude = num8 + kSPRandom->NextDouble() * (num9 - num8);
    }
}

// WaypointManager.cs:166
float WaypointManager::Distance(double latitude1, double longitude1, double altitude1, double latitude2, double longitude2, double altitude2,
                                CelestialBody* body) {
    const Vector3d worldSurfacePosition = body->GetWorldSurfacePosition(latitude1, longitude1, altitude1);
    const Vector3d worldSurfacePosition2 = body->GetWorldSurfacePosition(latitude2, longitude2, altitude2);
    return static_cast<float>(Vector3d::Distance(worldSurfacePosition, worldSurfacePosition2));
}

}  // namespace ksp::FinePrint
