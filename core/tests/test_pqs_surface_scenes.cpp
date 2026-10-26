// WP-12d scenes on the physics bubble: PQSCity2.OrientateRayCast over the WP-12b terrain colliders at the four stock launch sites, and
// (item 4) the KSC pad: WP-11's terrain height under the pad against the WP-12c pad collider.
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>

#include "contact_fixture.hpp"
#include "scatter_scene.hpp"
#include "ksp/physics/scenery_host.hpp"
#include "ksp/space_center/pqs_city.hpp"
#include "ksp/space_center/scenery_colliders.hpp"
#include "ksp/terrain/pqs_mods.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp20e;

namespace {

const PQSCityPrefab& StockCity2(const std::string& gameObject) {
    for (const PQSCityPrefab& p : StockPQSCities())
        if (gameObject == p.gameObject) return p;
    throw std::runtime_error("no stock city " + gameObject);
}

// Kerbin's stock PQS built around a surface point as the flight scene does (the WP-20e KerbinSphere with another target)
struct SphereAt {
    PQSCache cache;
    std::unique_ptr<PQS> sphere;
    PQSTarget target;
    SphereAt(World& w, double lat, double lon) {
        cache.cachePQInitialCount = 64;
        sphere = terrain::CreateStockPQS("Kerbin", *w.maps);
        for (auto& m : sphere->childMods)
            if (auto* cb = dynamic_cast<PQSMod_CelestialBodyTransform*>(m.get())) {
                cb->host.hasBody = [] { return true; };
                cb->host.flightGlobalsExists = [] { return true; };
                cb->host.activeVessel = [this] { return &target; };
                cb->host.isCurrentMainBody = [] { return true; };
                cb->host.loadedScene = [] { return static_cast<int>(GameScenes::FLIGHT); };
                cb->host.mapIsEnabled = [] { return false; };
            }
        PQSCache::CreateDefaultPresetList();
        sphere->cache = &cache;
        CelestialFrame cf;
        CelestialFrame::SetFrame(0.0, 0.0, 0.0, cf);
        target.position = LatLonGetSurfaceNVector(cf, lat, lon) * (w.Kerbin().Radius + 70.0);
        sphere->SetTarget(&target);
        sphere->Start();
        for (int f = 0; f < 2; ++f) {
            sphere->FixedUpdate();
            sphere->FrameUpdate();
        }
    }
    ~SphereAt() { PQSCache::PresetList.reset(); }
};

}  // namespace

TEST_CASE_FIXTURE(wp12d::StateGuard, "WP-12d PQSCity2.OrientateRayCast over the terrain colliders at the four stock launch sites (the height correction KSP applies, sign as in the C#)") {
    if (!wp20b::GameAvailable()) return;
    for (const char* site : {"Kerbin/Crater_Launch_Site", "Kerbin/MahiMahi_Launch_Site", "Kerbin/Cove_Launch_Site", "Kerbin/GlacierLake_Launch_Site"}) {
        World w(true);
        const PQSCityPrefab& prefab = StockCity2(site);
        SphereAt sa(w, prefab.lat, prefab.lon);
        KerbinScene sc(w);
        CelestialBody& kerbin = w.Kerbin();
        ContactHost& ch = sc.s.bubble.Contacts();
        TerrainColliders* tc = ch.HostTerrain(kerbin, *sa.sphere);
        REQUIRE(tc != nullptr);
        PQSCity2 c2(prefab, &kerbin, sa.sphere.get());
        c2.orientateRayCast = MakeOrientateRayCast(sc.s.bubble, kerbin);
        CelestialFrame cf;
        CelestialFrame::SetFrame(0.0, 0.0, 0.0, cf);
        const Vector3d n = LatLonGetSurfaceNVector(cf, prefab.lat, prefab.lon);
        const double analytic = sa.sphere->GetSurfaceHeight(n, true);
        // the collider surface on the radial line, measured independently (a ray in the physics frame from 1 km above)
        const Vector3d centre = ch.TerrainOrigin(kerbin);
        const Vector3d up = ch.TerrainRotation(kerbin) * n;
        const PhysicsWorld::RayHit h = sc.s.world.RayCast(centre + up * (analytic + 1000.0), up * -1.0, 2000.0, PhysicsWorld::kLayerStatic);
        REQUIRE(h.hit);
        const double collider = (h.point - centre).magnitude();
        c2.Orientate();
        CHECK(c2.raycastCompleted);
        CHECK(c2.positioningCompleted);
        const double placed = Vector3d(c2.localPosition).magnitude();
        // PQSCity2.cs:216-227 + :342-365: num = analytic + s; the city point at num, the ray from 500 m above it: hit.distance - 500 =
        // num - collider; returned num + (num - collider), then + s again
        const double s = prefab.snapHeightOffset;
        const double expected = 2.0 * (analytic + s) - collider + s;
        MESSAGE(std::string(site) << ": analytic " << analytic - kerbin.Radius << " m, collider " << collider - kerbin.Radius << " m (collider - analytic "
                     << collider - analytic << " m), snapHeightOffset " << s << ", placed " << placed - kerbin.Radius << " m = collider + "
                     << placed - collider << " m (without the raycast: analytic + s = collider + " << analytic + s - collider << " m)");
        CHECK(std::fabs(placed - expected) < 0.07);   // float transform positions (1/16 m at 600 km) and the float RaycastHit distance
        CHECK(std::fabs(c2.alt - (expected - sa.sphere->radius)) < 0.07);
        ch.DropTerrain(kerbin);
    }
}

TEST_CASE_FIXTURE(wp12d::StateGuard, "WP-12d KSC pad: WP-11's terrain under the launch-pad spawn point (the MapDecalTangent ground) vs the pad collider (WP-12c) and the terrain collider (WP-12b)") {
    if (!wp20b::GameAvailable()) return;
    const SceneryColliders doc = LoadSceneryColliders(std::string(KSPCORE_DATA_DIR) + "/scenery/scenery_colliders.json");
    const SceneryCity& ksc = *doc.Find("Kerbin/KSC");
    for (int level : {0, -1}) {
        World w(true);
        CelestialBody& kerbin = w.Kerbin();
        PQS flat;   // KscPose: repositionToSphere needs only the radius (PQSCity.cs:315)
        flat.radius = kerbin.Radius;
        PQSCity city(StockCity2("Kerbin/KSC"), &kerbin, &flat);
        city.Orientate();
        SceneryLevels lv;
        lv.fallback = level;
        const std::vector<bool> present = SceneryPresence(ksc, lv);
        const int spawn = SceneryFindObject(ksc, "KSC/SpaceCenter/LaunchPad/Facility/LaunchPad_spawn", present);
        REQUIRE(spawn >= 0);
        const std::vector<SceneryObjectPose> poses = SceneryObjectPoses(ksc);
        const Vector3d spawnLocal = Vector3d(city.localPosition) + QuaternionD(city.localRotation) * Vector3d(poses[static_cast<size_t>(spawn)].position);
        const Vector3d n = spawnLocal.normalized();
        double lat = 0.0, lon = 0.0;
        {
            const Vector3d r = n.xzy();
            lat = dotnet::Math::Asin(r.z) * (180.0 / 3.141592653589793);
            lon = dotnet::Math::Atan2(r.y, r.x) * (180.0 / 3.141592653589793);
        }
        const double hPublic = w.pqs->GetSurfaceHeight(n) - kerbin.Radius;
        const double hOverride = w.pqs->GetSurfaceHeight(n, true) - kerbin.Radius;
        // the terrain collider (PQ meshes around the pad, WP-12b) and then the KSC colliders, each on the radial line through the spawn point
        SphereAt sa(w, lat, lon);
        KerbinScene sc(w);
        ContactHost& ch = sc.s.bubble.Contacts();
        REQUIRE(ch.HostTerrain(kerbin, *sa.sphere) != nullptr);
        const Vector3d centre = ch.TerrainOrigin(kerbin);
        const Vector3d up = ch.TerrainRotation(kerbin) * n;
        const double top = spawnLocal.magnitude() + 100.0;
        const PhysicsWorld::RayHit pq = sc.s.world.RayCast(centre + up * top, up * -1.0, 400.0, PhysicsWorld::kLayerStatic);
        REQUIRE(pq.hit);
        const double terrainCollider = (pq.point - centre).magnitude() - kerbin.Radius;
        SceneryCityHost host(sc.s.bubble, ksc, 0x5343000000000000ull, lv);
        host.PlaceOnBody(kerbin, Vector3d(city.localPosition), QuaternionD(city.localRotation));
        host.ActivateAll();
        const PhysicsWorld::RayHit pad = sc.s.world.RayCast(centre + up * top, up * -1.0, 400.0, PhysicsWorld::kLayerStatic);
        REQUIRE(pad.hit);
        const StaticColliderInfo& info = ch.StaticInfo(pad.userData);
        const double padSurface = (pad.point - centre).magnitude() - kerbin.Radius;
        MESSAGE("KSC level " << (level < 0 ? std::string("highest") : std::to_string(level)) << ": spawn point lat " << lat << " lon " << lon << ", "
                             << spawnLocal.magnitude() - kerbin.Radius << " m; KSC origin " << Vector3d(city.localPosition).magnitude() - kerbin.Radius
                             << " m (radius + repositionRadiusOffset 42.7); WP-11 terrain " << hPublic << " m (override " << hOverride
                             << "); terrain collider " << terrainCollider << " m; pad collider \"" << info.name << "\" tag " << info.tag << " at "
                             << padSurface << " m: pad - terrain " << padSurface - hOverride << " m, pad - terrain collider " << padSurface - terrainCollider << " m");
        // the in-game capture's terrain under the active vessel on the pad (harness/captures/out2/terrain_height.json, set active_vessel:
        // lat -0.09720779031430374, lon -74.55767983607595, h_override 0x4122500191C4E80B = 600064.7847054017): WP-11 bit for bit
        {
            const double capLat = -0.09720779031430374, capLon = -74.55767983607595;
            const double h = w.pqs->GetSurfaceHeight(terrain::GetRelSurfaceNVector(capLat, capLon), true);
            CHECK(h == 600064.7847054017);
            MESSAGE("the capture's active vessel stood " << (lat - capLat) * 600000.0 * 3.141592653589793 / 180.0 << " m north, "
                    << (lon - capLon) * 600000.0 * 3.141592653589793 / 180.0 << " m east of the spawn point's radial line");
            CHECK(std::fabs(lat - capLat) < 1e-4);   // the same place (~10 m), not a gate
            CHECK(std::fabs(lon - capLon) < 1e-4);
        }
        CHECK(info.tag == "LaunchPad");
        CHECK(!info.isPQ);
        CHECK(padSurface > hOverride);   // the pad stands on the flattened ground
        CHECK(std::fabs(terrainCollider - hOverride) < 1.6);   // WP-12b's collider-vs-WP-11 bound within 200 m of the pad
        ch.DropTerrain(kerbin);
    }
}
