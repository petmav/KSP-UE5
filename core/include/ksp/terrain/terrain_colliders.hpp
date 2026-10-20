// WP-12b: terrain colliders. Static collision geometry for the quads of a PQS sphere in a physics::PhysicsWorld, driven by
// exactly the decisions PQSMod_QuadMeshColliders makes, plus the terrain query API of later packages.
//
// What KSP does (Assembly-CSharp/PQSMod_QuadMeshColliders.cs):
//   OnSetup        minLevel = sphere.maxLevel - |maxLevelOffset|                                   (:17)
//   OnQuadBuilt    a quad with subdivision >= minLevel gets a MeshCollider (once; the component stays on the pooled quad),
//                  enabled, sharedMesh = quad.mesh, sharedMaterial = physicsMaterial                (:22-34)
//   OnQuadDestroy  the collider is disabled, mesh and material cleared                              (:36-44)
// That is the whole rule: there is no distance test in the collider mods. Which quads exist at the maximum levels is the PQS
// quadtree's decision (WP-12a: target-driven subdivision, PQ.cs UpdateSubdivision), the collider follows the quad's build and
// its destruction. A quad that stops being visible or gets subdivided keeps its collider until PQS.DestroyQuad (a subdivided
// quad's collider stays active under its children when maxLevelOffset > 0: Stock Kerbin / Mun / ... all have offset 0, so the
// colliders are exactly the built leaves of the maximum level, 1 quad = 1 collider). The sphere's layer is the quads' GameObject
// layer (PQS.cs:1015); the physics matrix between that layer, parts and kerbals is project data (analysis/physics: layer 15
// "Local Scenery" is terrain and scatter, KSP makes no IgnoreLayerCollision call), here: the Static layer of PhysicsWorld.
//
// What this class does with those decisions (the seam PQS::onQuadColliderEnabled / onQuadColliderDisabled of WP-12a):
//   * enabled  -> a static Jolt MeshShape body (PhysicsWorld::AddStaticMesh) for the quad: the triangle set the quad was built
//                 with (cacheIndices[0], as Unity bakes `quad.mesh`), the vertices from the EXACT double planet-relative
//                 vertices PQ::vertsD (not the float mesh: its 1/16 m quantisation at Kerbin's radius would be the collision
//                 surface; the float mesh is what the renderer gets) expressed relative to a double centre, in the physics
//                 frame (`origin` = the planet centre there; the body-fixed frame below the inverse-rotation threshold).
//                 An enabled event for a quad that has a collider replaces it (the sharedMesh re-assignment of a rebuild).
//   * disabled -> the body is removed. A queued (not yet cooked) collider is dropped.
//   * Cooking budget: Unity bakes a MeshCollider synchronously inside sharedMesh = ...; by default so does this class (the
//     body exists when the hook returns). With maxCooksPerUpdate > 0 the geometry is snapshotted at the event and cooked by
//     Update(), oldest event first, at most that many per call (a deterministic count budget, not a time budget).
//
// Changed vs KSP (documented, measured in STATUS.md): the collision mesh uses the double vertices (KSP: the float quad mesh);
// the PhysicMaterial of the quads (PQSMod_QuadMeshColliders.physicsMaterial: "Ground" 0.9 / 0.9 / 0, Maximum / Minimum combine on Kerbin,
// Minmus, Moho, Eve, Laythe, Dres; none = Physics.defaultMaterial elsewhere) is `material` (data/scenery, WP-12c); friction / restitution are
// the body values used when the world does not run PhysX material combining (PhysicsWorldConfig::physxMaterials).
//
// Not here: PQSCity / PQSCity2 object colliders (ksp/space_center/city_colliders.hpp), PQS_Collider / PQS_ColliderAdvanced /
// PQS_PartCollider (never instantiated by any shipped code: only Part.cs destroys a `terrainCollider` that nothing creates),
// PQSMod_CreateSphereCollider (a SphereCollider of radius sphere.radius + radiusOffset on the PQS object: not used by the stock
// prefabs; the body is covered by the quads), SurfaceObject / PQSMod_SurfaceObjectQuads (parenting of vessels to quads for the
// floating origin, no collision).
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/physics/physics_world.hpp"

namespace ksp {

class PQ;
class PQS;

struct TerrainCollidersOptions {
    // static body key of a quad collider = keyBase + (uint32)quad.id (several spheres in one world use different bases)
    uint64_t keyBase = 0x5451000000000000ull;
    // the planet centre in the physics frame (body-fixed frame: zero)
    Vector3d origin;
    // WP-20e: the orientation of the sphere's local frame in the physics frame (physics = origin + rotation * planet-relative); identity in
    // the body-fixed frame. A host whose physics axes are not the body's (the physics bubble: the world axes) passes the body's rotation.
    QuaternionD rotation = QuaternionD::identity();
    // 0 = cook inside the hook (KSP: synchronous bake); N > 0 = at most N per Update(), oldest first
    int maxCooksPerUpdate = 0;
    // PhysicMaterial "Ground" is not extracted yet (FIXTURE: the spike's value)
    float friction = 0.6f;
    float restitution = 0.0f;
    std::shared_ptr<const physics::PhysicMaterialDesc> material;   // WP-12c: null = the world's default material
    // keep the ordered event log (the activation gate)
    bool recordEvents = false;
};

// one enable / disable decision of PQSMod_QuadMeshColliders, as seen by this class
struct ColliderEvent {
    uint32_t seq = 0;
    bool enabled = false;     // false: disabled
    int quadId = 0;
    std::string name;
    int subdivision = 0;
    uint64_t geometryHash = 0;   // enabled events: FNV-1a 64 over the collider geometry (see ColliderGeometryHash), else 0
};

struct TerrainHit {
    bool hit = false;
    double distance = 0.0;       // along the ray
    Vector3d point;              // physics frame
    Vector3d normal;             // unit, on the side of the ray origin
    int quadId = -1;             // the quad whose collider was hit
    double radius = 0.0;         // |point - origin|
    double height = 0.0;         // radius - sphere.radius (altitude above the sphere's nominal radius, i.e. KSP's terrain altitude)
};

struct TerrainColliderStats {
    uint64_t enabledEvents = 0, disabledEvents = 0, cooked = 0, replaced = 0, removed = 0, dropped = 0, failed = 0;
    double cookSeconds = 0.0, maxCookSeconds = 0.0;  // wall time inside PhysicsWorld::AddStaticMesh
};

class TerrainColliders {
public:
    TerrainColliders(physics::PhysicsWorld& world, PQS& sphere, const TerrainCollidersOptions& options = {});
    // Removes every collider it created and restores the sphere's previous hooks. Destroy it BEFORE the sphere.
    ~TerrainColliders();
    TerrainColliders(const TerrainColliders&) = delete;
    TerrainColliders& operator=(const TerrainColliders&) = delete;

    // Picks up the quads whose MeshCollider is already enabled (a manager attached to a sphere that has built quads); each gets
    // the enabled event it would have had. Returns how many.
    size_t AdoptExisting();

    // Cooks queued colliders (see maxCooksPerUpdate); returns how many were cooked. A no-op in synchronous mode.
    size_t Update();

    // colliders cooked and present in the world / queued for cooking / both
    size_t CookedCount() const;
    size_t PendingCount() const;
    // the quad ids of the cooked colliders as the PHYSICS WORLD holds them (its static keys in this manager's range), ascending
    std::vector<int> WorldQuadIds() const;
    // the quad ids this manager tracks (cooked or pending), ascending
    std::vector<int> TrackedQuadIds() const;

    const std::vector<ColliderEvent>& Events() const { return events_; }
    void ClearEvents() { events_.clear(); }
    const TerrainColliderStats& Stats() const { return stats_; }
    const TerrainCollidersOptions& Options() const { return opt_; }

    // The geometry digest of a quad's collider as built from `quad` now (the hash the enabled event records): FNV-1a 64 over
    // centre (3 doubles), vertex count (u32), the vertices relative to the centre (3 floats each), index count (u32), indices
    // (i32). tools/wp12/colliders_reference.py computes the same bytes independently.
    static uint64_t ColliderGeometryHash(const Vector3d& centre, const std::vector<Vector3f>& vertices, const std::vector<int>& indices);

    // ---- queries (terrain only: the Static layer) ----------------------------------------------------------------------------
    // Closest terrain along the ray (physics frame); direction need not be normalised. A ray that misses is retried from four
    // points 2 mm to the side: neighbouring quads are separate meshes (float vertices rounded relative to their own centres), so
    // a ray through a vertex on a shared border can pass the <= 1e-4 m crack between them.
    TerrainHit RayCast(const Vector3d& origin, const Vector3d& direction, double maxDistance) const;
    // From `point` straight down the local vertical (towards the planet centre).
    TerrainHit RayDown(const Vector3d& point, double maxDistance) const;
    // The terrain on the radial line through `point`: the ray starts `lift` above the point and looks `lift + depth` towards
    // the centre. hit.height is then the terrain altitude, comparable to PQS.GetSurfaceHeight(direction) - sphere.radius.
    TerrainHit SurfaceAt(const Vector3d& point, double lift = 3000.0, double depth = 3000.0) const;
    // Distance from `point` down to the terrain along the local vertical (Vessel.heightFromTerrain for a point; negative
    // when the point is below the surface and the terrain is found above it within maxDistance). False when nothing is hit.
    bool HeightAboveTerrain(const Vector3d& point, double& height, double maxDistance = 3000.0) const;
    // Surface normal of the terrain on the radial line through `point`; false when there is no collider there.
    bool SurfaceNormal(const Vector3d& point, Vector3d& normal, double lift = 3000.0, double depth = 3000.0) const;
    // Quad ids of the terrain colliders overlapping a sphere (ascending).
    size_t Overlap(const Vector3d& centre, double radius, std::vector<int>& quadIds) const;

private:
    struct Entry;
    void OnEnabled(PQ& quad);
    void OnDisabled(PQ& quad);
    void Cook(Entry& e);
    TerrainHit Convert(const physics::PhysicsWorld::RayHit& h) const;

    physics::PhysicsWorld& world_;
    PQS& sphere_;
    TerrainCollidersOptions opt_;
    std::function<void(PQ&)> prevEnabled_, prevDisabled_;
    std::map<PQ*, std::unique_ptr<Entry>> entries_;      // tracked colliders by quad object (the pooled PQ is the Unity component's owner)
    std::vector<Entry*> queue_;           // pending, oldest first
    std::vector<ColliderEvent> events_;
    uint32_t seq_ = 0;
    TerrainColliderStats stats_;
};

}  // namespace ksp
