// WP-12b: static geometry and queries of the PhysicsWorld, the TerrainColliders manager on synthetic quads (plane), the cook
// budget, and the PQSCity / PQSCity2 collider activation. The reference replays of the collider decisions against
// tools/wp12/colliders_reference.py, the sphere-patch / Kerbin / Mun measurements and the box drop on the KSC pad are in
// test_pqs_quadtree.cpp (they need its scene helpers).
//
// There is no KSP golden or capture for any of this (PhysX is not reproducible); the checks are analytic (line-plane and
// ray-triangle intersections in double) and decision tables derived from the C#.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/physics/physics_world.hpp"
#include "ksp/space_center/city_colliders.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/terrain/terrain_colliders.hpp"

using namespace ksp;
using namespace ksp::physics;

namespace {

// a flat grid of (n x n) cells, 2 triangles per cell, over [-half, half]^2 in x / z, y = a x + b z + c
void MakePlane(int n, double half, double a, double b, double c, std::vector<Vector3f>& v, std::vector<int>& t) {
    const int s = n + 1;
    v.clear();
    t.clear();
    for (int i = 0; i < s; i++)
        for (int j = 0; j < s; j++) {
            const double x = -half + 2.0 * half * j / n, z = -half + 2.0 * half * i / n;
            v.push_back(Vector3f(static_cast<float>(x), static_cast<float>(a * x + b * z + c), static_cast<float>(z)));
        }
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            const int k = i * s + j;
            // Unity winding of the PQS index sets: the numeric normal cross(b - a, c - a) points up (+y)
            t.insert(t.end(), {k, k + 1, k + s});
            t.insert(t.end(), {k + 1, k + s + 1, k + s});
        }
}

double Dot(const Vector3d& a, const Vector3d& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

}  // namespace

TEST_CASE("WP-12b PhysicsWorld static mesh: ray cast distance, point and normal on a tilted plane (analytic)") {
    PhysicsWorld w;
    std::vector<Vector3f> v;
    std::vector<int> t;
    const double a = 0.05, b = -0.02, c = 3.0;
    MakePlane(10, 100.0, a, b, c, v, t);
    const Vector3d centre(6.0e5, -2.0e4, 1.5e5);  // double position far from the origin
    const BodyId id = w.AddStaticMesh(77, centre, v, t);
    REQUIRE(id != kInvalidBody);
    CHECK(w.StaticCount() == 1);
    CHECK(w.StaticBody(77) == id);
    CHECK(w.UserData(id) == 77);
    // plane: y' = a x' + b z' + c (local), world = centre + local. Normal n = normalize(-a, 1, -b).
    const Vector3d n = Vector3d(-a, 1.0, -b).normalized();
    double worst = 0.0, worstN = 0.0;
    for (int k = 0; k < 40; k++) {
        const double x = -90.0 + 4.5 * k, z = 80.0 - 3.7 * k;
        const Vector3d o = centre + Vector3d(x, 40.0, z);
        const auto h = w.RayCast(o, Vector3d(0, -1, 0), 200.0);
        REQUIRE(h.hit);
        const double yPlane = a * x + b * z + c;
        const double expect = 40.0 - yPlane;
        worst = std::max(worst, std::fabs(h.distance - expect));
        worst = std::max(worst, std::fabs(h.point.y - (centre.y + yPlane)));
        worstN = std::max(worstN, (h.normal - n).magnitude());
        CHECK(h.userData == 77);
        CHECK(h.body == id);
    }
    CHECK(worst < 2.0e-4);   // float fraction of a 200 m ray (1.2e-5) + vertices as floats relative to the centre
    CHECK(worstN < 1.0e-6);
    // from below: both sides are hit, the normal faces the ray origin
    const auto up = w.RayCast(centre + Vector3d(10, -50, 10), Vector3d(0, 1, 0), 200.0);
    REQUIRE(up.hit);
    CHECK(Dot(up.normal, Vector3d(0, -1, 0)) > 0.99);
    CHECK(std::fabs(up.distance - (50.0 + a * 10 + b * 10 + c)) < 2.0e-4);
    // a ray that misses (beside the patch) and one that is too short
    CHECK(!w.RayCast(centre + Vector3d(150, 40, 0), Vector3d(0, -1, 0), 200.0).hit);
    CHECK(!w.RayCast(centre + Vector3d(0, 40, 0), Vector3d(0, -1, 0), 20.0).hit);
    // the Moving layer mask does not see a static body
    CHECK(!w.RayCast(centre + Vector3d(0, 40, 0), Vector3d(0, -1, 0), 200.0, PhysicsWorld::kLayerMoving).hit);
    // overlap
    std::vector<BodyId> ov;
    CHECK(w.OverlapSphere(centre + Vector3d(0, 1, 0), 5.0, ov) == 1);
    CHECK(ov[0] == id);
    CHECK(w.OverlapSphere(centre + Vector3d(0, 30, 0), 5.0, ov) == 0);
    // replace: a second mesh under the same key; remove
    MakePlane(4, 50.0, 0.0, 0.0, 10.0, v, t);
    const BodyId id2 = w.AddStaticMesh(77, centre, v, t);
    REQUIRE(id2 != kInvalidBody);
    CHECK(w.StaticCount() == 1);
    CHECK(w.BodyCount() == 1);
    const auto h2 = w.RayCast(centre + Vector3d(0, 40, 0), Vector3d(0, -1, 0), 200.0);
    CHECK(std::fabs(h2.distance - 30.0) < 1.0e-4);
    CHECK(w.RemoveStatic(77));
    CHECK(!w.RemoveStatic(77));
    CHECK(w.StaticCount() == 0);
    CHECK(w.BodyCount() == 0);
    // degenerate input is refused, not cooked
    CHECK(w.AddStaticMesh(5, centre, v, std::vector<int>{}) == kInvalidBody);
    CHECK(w.AddStaticMesh(5, centre, v, std::vector<int>{0, 1, 9999}) == kInvalidBody);
    CHECK(w.StaticCount() == 0);
}

TEST_CASE("WP-12b PhysicsWorld static shapes (city objects) are hit by rays and removed by key") {
    PhysicsWorld w;
    const QuaternionD rot = QuaternionD::AngleAxis(90.0, Vector3d(0, 0, 1));  // x-long box stands up
    w.AddStaticShape(1, ShapeDesc::Box(Vector3f(5, 1, 1)), Vector3d(1e5, 0, 0), rot);
    w.AddStaticShape(2, ShapeDesc::Sphere(2.0f), Vector3d(1e5, 30, 0), QuaternionD::identity());
    // the box is 10 m tall after the rotation: a ray from above hits its top at y = 5, one at the sphere's centre line at y = 32
    const auto top = w.RayCast(Vector3d(1e5, 100, 0), Vector3d(0, -1, 0), 200.0);
    REQUIRE(top.hit);
    CHECK(top.userData == 2);
    CHECK(std::fabs(top.point.y - 32.0) < 1e-3);
    CHECK(w.RayCast(Vector3d(1e5, 20, 0), Vector3d(0, -1, 0), 200.0).userData == 1);
    CHECK(std::fabs(w.RayCast(Vector3d(1e5, 20, 0), Vector3d(0, -1, 0), 200.0).point.y - 5.0) < 1e-3);
    const std::vector<uint64_t> keys = w.StaticKeys();
    CHECK(keys == std::vector<uint64_t>{1, 2});
    CHECK(w.RemoveStatic(2));
    CHECK(w.RayCast(Vector3d(1e5, 100, 0), Vector3d(0, -1, 0), 200.0).userData == 1);
    w.RemoveBody(w.StaticBody(1));   // removed by body id: no stale key
    CHECK(w.StaticCount() == 0);
    CHECK(w.StaticBody(1) == kInvalidBody);
}

namespace {

// A sphere with a prepared quad cache whose quads the tests fabricate (the QuadMeshColliders hook is a std::function)
struct FakeSphere {
    PQSCache cache;
    PQS sphere;
    FakeSphere() {
        cache.meshCache.CreateCache();
        sphere.cache = &cache;
        sphere.radius = 600000.0;
        sphere.maxLevel = 9;
    }
};

// fills quad.vertsD with the template grid scaled to `size` m, heights y = a x + b z + c above `centre`
void FabricateQuad(PQ& q, const PQSMeshCache& mc, int id, const Vector3d& centre, double size, double a, double b, double c) {
    q.id = id;
    q.name = "fake" + std::to_string(id);
    q.subdivision = 9;
    q.positionPlanet = centre;
    q.vertsD.clear();
    for (int i = 0; i < mc.cacheVertCount; i++) {
        const Vector3f& cv = mc.cacheVerts[static_cast<size_t>(i)];
        const double x = static_cast<double>(cv.x) * size, z = static_cast<double>(cv.z) * size;
        q.vertsD.push_back(centre + Vector3d(x, a * x + b * z + c, z));
    }
}

}  // namespace

TEST_CASE("WP-12b TerrainColliders: synthetic plane quad, queries, replacement, budget, cleanup") {
    PhysicsWorld world;
    FakeSphere fs;
    TerrainCollidersOptions opt;
    opt.recordEvents = true;
    PQ q1, q2;
    const Vector3d c1(0.0, 600000.0, 0.0), c2(3000.0, 600000.0, 0.0);
    {
        TerrainColliders tc(world, fs.sphere, opt);
        FabricateQuad(q1, fs.cache.meshCache, 11, c1, 2000.0, 0.01, 0.02, 12.0);
        fs.sphere.onQuadColliderEnabled(q1);
        REQUIRE(world.StaticCount() == 1);
        CHECK(tc.CookedCount() == 1);
        CHECK(tc.WorldQuadIds() == std::vector<int>{11});
        REQUIRE(tc.Events().size() == 1);
        CHECK(tc.Events()[0].enabled);
        CHECK(tc.Events()[0].quadId == 11);
        CHECK(tc.Events()[0].geometryHash != 0);
        // analytic: the plane through the fabricated vertices; the radial ray at (x, z) = (300, -200) starts at the point
        const double a = 0.01, b = 0.02, c = 12.0;
        const double x = 300.0, z = -200.0;
        const Vector3d p = c1 + Vector3d(x, 40.0, z);
        const Vector3d dir = p.normalized();      // up at p
        // line p - t dir with the plane y = c1.y + a (X - c1.x) + b (Z - c1.z) + c
        // (p.y - t d.y) - c1.y = a (p.x - t d.x - c1.x) + b (p.z - t d.z - c1.z) + c  ->  t = (p.y - c1.y - a x - b z - c) / (d.y - a d.x - b d.z)
        const double tExpect = (p.y - c1.y - a * x - b * z - c) / (dir.y - a * dir.x - b * dir.z);
        const TerrainHit h = tc.RayDown(p, 500.0);
        REQUIRE(h.hit);
        CHECK(h.quadId == 11);
        CHECK(std::fabs(h.distance - tExpect) < 2.0e-4);
        CHECK(std::fabs(h.height - (h.point.magnitude() - fs.sphere.radius)) < 1e-9);
        const Vector3d n = Vector3d(-a, 1.0, -b).normalized();
        CHECK((h.normal - n).magnitude() < 1e-6);
        double hab = 0.0;
        REQUIRE(tc.HeightAboveTerrain(p, hab, 500.0));
        CHECK(hab == h.distance);
        // below the surface: negative height from the upward hit
        const Vector3d below = c1 + Vector3d(x, -20.0, z);
        REQUIRE(tc.HeightAboveTerrain(below, hab, 500.0));
        CHECK(hab < 0.0);
        // SurfaceAt from far above / below gives the same terrain point (the radial line through the point)
        const TerrainHit s1 = tc.SurfaceAt(p, 300.0, 300.0), s2 = tc.SurfaceAt(below, 300.0, 300.0);
        REQUIRE(s1.hit);
        REQUIRE(s2.hit);
        CHECK((s1.point - s2.point).magnitude() < 0.1);  // the two radial lines differ by 0.03 m horizontally
        Vector3d nn;
        REQUIRE(tc.SurfaceNormal(p, nn));
        CHECK((nn - n).magnitude() < 1e-6);
        // nothing under a point 5 km away
        CHECK(!tc.RayDown(c1 + Vector3d(5000.0, 40.0, 0.0), 500.0).hit);
        std::vector<int> ids;
        CHECK(tc.Overlap(c1 + Vector3d(0, 12.0, 0), 3.0, ids) == 1);
        CHECK(ids == std::vector<int>{11});

        // enabling the same quad again replaces the body (the sharedMesh reassignment); the key is unchanged
        FabricateQuad(q1, fs.cache.meshCache, 11, c1, 2000.0, 0.0, 0.0, 30.0);
        fs.sphere.onQuadColliderEnabled(q1);
        CHECK(world.StaticCount() == 1);
        CHECK(tc.Stats().replaced == 1);
        CHECK(std::fabs(tc.RayDown(c1 + Vector3d(3.3, 100.0, 2.1), 500.0).distance - 70.0) < 1e-3);
        // a second quad; disable twice is harmless; disable removes
        FabricateQuad(q2, fs.cache.meshCache, 12, c2, 2000.0, 0.0, 0.0, 5.0);
        fs.sphere.onQuadColliderEnabled(q2);
        CHECK(tc.WorldQuadIds() == std::vector<int>{11, 12});
        fs.sphere.onQuadColliderDisabled(q1);
        fs.sphere.onQuadColliderDisabled(q1);
        CHECK(tc.WorldQuadIds() == std::vector<int>{12});
        CHECK(tc.Events().size() == 5);
        CHECK(tc.Events()[4].enabled == false);
    }
    // the destructor removes what it created and restores the hooks
    CHECK(world.StaticCount() == 0);
    CHECK(!fs.sphere.onQuadColliderEnabled);
    CHECK(!fs.sphere.onQuadColliderDisabled);
}

TEST_CASE("WP-12b TerrainColliders: cook budget defers the cook, a disable before it drops the job, geometry is snapshotted") {
    PhysicsWorld world;
    FakeSphere fs;
    TerrainCollidersOptions opt;
    opt.maxCooksPerUpdate = 2;
    opt.recordEvents = true;
    TerrainColliders tc(world, fs.sphere, opt);
    std::vector<PQ> quads(5);
    for (int i = 0; i < 5; i++) {
        FabricateQuad(quads[static_cast<size_t>(i)], fs.cache.meshCache, 100 + i, Vector3d(2500.0 * i, 600000.0, 0.0), 2000.0, 0.0, 0.0, 1.0 + i);
        fs.sphere.onQuadColliderEnabled(quads[static_cast<size_t>(i)]);
    }
    CHECK(tc.PendingCount() == 5);
    CHECK(world.StaticCount() == 0);
    // quad 103 is destroyed before it is cooked
    fs.sphere.onQuadColliderDisabled(quads[3]);
    CHECK(tc.PendingCount() == 4);
    // the quad object is reused and rebuilt with other vertices after the snapshot: the cooked shape must be the snapshot
    FabricateQuad(quads[0], fs.cache.meshCache, 100, Vector3d(0.0, 600000.0, 0.0), 2000.0, 0.0, 0.0, 500.0);
    CHECK(tc.Update() == 2);
    CHECK(tc.WorldQuadIds() == std::vector<int>{100, 101});
    CHECK(std::fabs(tc.RayDown(Vector3d(3.3, 600100.0, 2.1), 1000.0).distance - 99.0) < 1e-3);   // height 1, not 500
    CHECK(tc.Update() == 2);
    CHECK(tc.Update() == 0);
    CHECK(tc.WorldQuadIds() == std::vector<int>{100, 101, 102, 104});
    CHECK(tc.PendingCount() == 0);
    CHECK(tc.Stats().cooked == 4);
    CHECK(tc.Stats().dropped == 1);
    // events were logged at event time, not cook time: 5 enabled + 1 disabled, in order
    REQUIRE(tc.Events().size() == 6);
    CHECK(tc.Events()[5].quadId == 103);
    CHECK(!tc.Events()[5].enabled);
    for (size_t i = 0; i < 6; i++) CHECK(tc.Events()[i].seq == i);
}

TEST_CASE("WP-12b geometry hash is FNV-1a 64 over centre, vertices, indices (value pinned for the Python reference)") {
    const std::vector<Vector3f> v = {Vector3f(1.0f, 2.0f, 3.0f), Vector3f(-4.5f, 0.0f, 0.25f)};
    const std::vector<int> t = {0, 1, 0};
    const uint64_t h = TerrainColliders::ColliderGeometryHash(Vector3d(1.0, -2.0, 3.5), v, t);
    // the same bytes hashed independently (little-endian layout, FNV-1a 64)
    uint64_t r = 14695981039346656037ull;
    auto put = [&r](const void* d, size_t n) {
        const unsigned char* p = static_cast<const unsigned char*>(d);
        for (size_t i = 0; i < n; i++) {
            r ^= p[i];
            r *= 1099511628211ull;
        }
    };
    const double c[3] = {1.0, -2.0, 3.5};
    put(c, sizeof c);
    const uint32_t nv = 2;
    put(&nv, 4);
    const float f[6] = {1.0f, 2.0f, 3.0f, -4.5f, 0.0f, 0.25f};
    put(f, sizeof f);
    const uint32_t ni = 3;
    put(&ni, 4);
    const int32_t ix[3] = {0, 1, 0};
    put(ix, sizeof ix);
    CHECK(h == r);
}

// ---- PQSCity / PQSCity2 collider activation ------------------------------------------------------------------------------------

namespace {

CityObject BoxObject(float half, const Vector3d& local) {
    CityObject o;
    CityCollider c;
    c.kind = CityCollider::Kind::Box;
    c.shape = ShapeDesc::Box(Vector3f(half, half, half));
    c.localPosition = local;
    o.colliders.push_back(c);
    return o;
}

}  // namespace

TEST_CASE("WP-12b PQSCity collider switching: transformsActive, LODRange sort and distance rule, target loss, sphere reset") {
    PhysicsWorld world;
    PQSCityColliders city(world, 0x4300000000000000ull);
    const Vector3d pos(600000.0, 100.0, 0.0);
    city.set.SetPose(pos, QuaternionD::identity());
    const size_t pad = city.set.Add(BoxObject(10.0f, Vector3d(0, 0, 0)));      // 0: a direct child (the launch pad)
    const size_t far = city.set.Add(BoxObject(5.0f, Vector3d(100, 0, 0)));     // 1: LOD, visible within 1000 m
    const size_t near = city.set.Add(BoxObject(5.0f, Vector3d(-100, 0, 0)));   // 2: LOD, visible within 300 m
    city.children = {pad};
    PQSCityColliders::LODRange rFar, rNear;
    rFar.visibleRange = 1000.0f;
    rFar.objects = {far};
    rNear.visibleRange = 300.0f;
    rNear.objects = {near};
    city.lod = {rFar, rNear};   // unsorted on purpose: OnSetup sorts by visibleRange (:168)
    city.OnSetup();
    CHECK(city.lod[0].visibleRange == 300.0f);
    CHECK(city.lod[1].visibleRange == 1000.0f);
    // after OnSetup every LOD range is inactive; the children are active only once OnUpdateFinished runs (their SetActive is the
    // engine's: a freshly spawned child is active, so the host places them with transformsActive == true -> Place on first use)
    CHECK(world.StaticCount() == 0);
    // OnSphereStart with a live sphere does nothing (:179)
    CHECK(!city.OnSphereStart(true));
    CHECK(city.transformsActive);
    // a target 2 km away: transforms re-activated (they were never switched off, so nothing changes for children), no LOD in range
    city.set.SetObjectActive(pad, true);   // the child is active in the prefab
    city.OnUpdateFinished(true, pos + Vector3d(2000, 0, 0));
    CHECK(city.set.IsObjectActive(pad));
    CHECK(!city.set.IsObjectActive(far));
    CHECK(!city.set.IsObjectActive(near));
    CHECK(world.StaticCount() == 1);
    // 800 m: the far range only; strictly less-than: exactly 1000 m is out
    city.OnUpdateFinished(true, pos + Vector3d(800, 0, 0));
    CHECK(city.set.IsObjectActive(far));
    CHECK(!city.set.IsObjectActive(near));
    city.OnUpdateFinished(true, pos + Vector3d(1000, 0, 0));
    CHECK(!city.set.IsObjectActive(far));
    // 200 m: both
    city.OnUpdateFinished(true, pos + Vector3d(0, 0, 200));
    CHECK(city.set.IsObjectActive(far));
    CHECK(city.set.IsObjectActive(near));
    CHECK(world.StaticCount() == 3);
    // the colliders are really in the world at the city pose: a ray down on the pad hits its top at y = 100 + 10
    const auto hit = world.RayCast(pos + Vector3d(0, 50, 0), Vector3d(0, -1, 0), 100.0);
    REQUIRE(hit.hit);
    CHECK(std::fabs(hit.point.y - 110.0) < 1e-3);
    // moving the city moves the active colliders
    city.set.SetPose(pos + Vector3d(0, 20, 0), QuaternionD::identity());
    CHECK(std::fabs(world.RayCast(pos + Vector3d(0, 80, 0), Vector3d(0, -1, 0), 100.0).point.y - 130.0) < 1e-3);
    city.set.SetPose(pos, QuaternionD::identity());
    // no target (OnUpdateFinished :234): every LOD range goes off, the children stay
    city.OnUpdateFinished(false, Vector3d());
    CHECK(!city.set.IsObjectActive(far));
    CHECK(!city.set.IsObjectActive(near));
    CHECK(city.set.IsObjectActive(pad));
    // sphere reset (:208): the children go off and transformsActive clears; the next update with a target brings them back
    city.OnSphereReset();
    CHECK(!city.transformsActive);
    CHECK(!city.set.IsObjectActive(pad));
    CHECK(world.StaticCount() == 0);
    city.OnSphereActive(true, pos + Vector3d(50, 0, 0));
    CHECK(city.transformsActive);
    CHECK(city.set.IsObjectActive(pad));
    CHECK(city.set.IsObjectActive(far));
    CHECK(city.set.IsObjectActive(near));
    // OnSphereInactive (:225): every LOD range off
    city.OnSphereInactive();
    CHECK(!city.set.IsObjectActive(far));
    CHECK(!city.set.IsObjectActive(near));
    CHECK(city.set.IsObjectActive(pad));
    // OnSphereStart with a dead sphere deactivates the children once (:179)
    CHECK(!city.OnSphereStart(false));
    CHECK(!city.transformsActive);
    CHECK(!city.set.IsObjectActive(pad));
    // a LOD object that is also a direct child is reactivated by the transforms toggle although its range is not in view (the
    // C# quirk of switching GetChild(i) objects: LODRange.isActive does not see it)
    city.lod[0].objects = {pad};
    city.OnSphereActive(true, pos + Vector3d(5000, 0, 0));
    CHECK(city.set.IsObjectActive(pad));
    CHECK(!city.lod[0].isActive);
}

TEST_CASE("WP-12b PQSCity2 collider switching: Setup deactivates, squared range rule, KeepActive, target loss, dead sphere") {
    PhysicsWorld world;
    PQSCity2Colliders city(world, 0x4400000000000000ull);
    const Vector3d pos(0.0, 600000.0, 0.0);
    city.set.SetPose(pos, QuaternionD::identity());
    city.set.SetObjectActive(city.set.Add(BoxObject(3.0f, Vector3d(0, 0, 0))), true);   // 0: starts active; Setup switches it off
    city.set.Add(BoxObject(3.0f, Vector3d(50, 0, 0)));                                  // 1
    city.set.Add(BoxObject(3.0f, Vector3d(-50, 0, 0)));                                 // 2
    PQSCity2Colliders::LodObject a, b, k;
    a.visibleRange = 500.0f;
    a.objects = {0};
    b.visibleRange = 100.0f;
    b.objects = {1};
    k.visibleRange = 10.0f;
    k.objects = {2};
    k.KeepActive = true;
    city.objects = {a, b, k};
    CHECK(world.StaticCount() == 1);
    city.OnSetup();
    CHECK(city.objects[0].visibleRangeSqr == 250000.0f);
    // OnSetup: Setup(all) then SetInactive: KeepActive forces the third object active again (SetActive :40)
    CHECK(!city.set.IsObjectActive(0));
    CHECK(!city.set.IsObjectActive(1));
    CHECK(city.set.IsObjectActive(2));
    CHECK(city.objects[2].isActive);
    // target 300 m away: only the 500 m object
    city.OnUpdateFinished(true, pos + Vector3d(300, 0, 0));
    CHECK(city.set.IsObjectActive(0));
    CHECK(!city.set.IsObjectActive(1));
    CHECK(city.inVisibleRange);
    // 99 m: both; exactly 100 m is out (strict <)
    city.OnUpdateFinished(true, pos + Vector3d(0, 0, 99));
    CHECK(city.set.IsObjectActive(1));
    city.OnUpdateFinished(true, pos + Vector3d(0, 0, 100));
    CHECK(!city.set.IsObjectActive(1));
    CHECK(city.set.IsObjectActive(0));
    // no target: SetInactive (KeepActive survives)
    city.OnUpdateFinished(false, Vector3d());
    CHECK(!city.set.IsObjectActive(0));
    CHECK(city.set.IsObjectActive(2));
    // OnSphereStart: a live sphere changes nothing, a dead one switches off
    city.OnSphereActive(true, pos + Vector3d(10, 0, 0));
    CHECK(city.set.IsObjectActive(0));
    CHECK(!city.OnSphereStart(true));
    CHECK(city.set.IsObjectActive(0));
    CHECK(!city.OnSphereStart(false));
    CHECK(!city.set.IsObjectActive(0));
    city.OnSphereActive(true, pos);
    city.OnSphereReset();
    CHECK(!city.set.IsObjectActive(0));
    city.OnSphereActive(true, pos);
    city.OnSphereInactive();
    CHECK(!city.set.IsObjectActive(0));
    CHECK(city.set.IsObjectActive(2));
    // set destruction removes the bodies
}
