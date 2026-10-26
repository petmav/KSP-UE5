// WP-12d scenes on the physics bubble: PQSCity2.OrientateRayCast over the WP-12b terrain colliders at the four stock launch sites, and
// (item 4) the KSC pad: WP-11's terrain height under the pad against the WP-12c pad collider.
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>

#include "contact_fixture.hpp"
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

TEST_CASE("WP-12d PQSCity2.OrientateRayCast over the terrain colliders at the four stock launch sites (the height correction KSP applies, sign as in the C#)") {
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
