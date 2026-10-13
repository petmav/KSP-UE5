// WP-12b: terrain colliders (see ksp/terrain/terrain_colliders.hpp). Glue between the PQSMod_QuadMeshColliders decisions of
// WP-12a and PhysicsWorld; the KSP rule itself lives in pqs_mods_quad.cpp (PQSMod_QuadMeshColliders.cs:17-44). No ported
// maths here: the geometry is a subtraction in double and a narrowing to float, the queries are Jolt's.
#include "ksp/terrain/terrain_colliders.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/terrain/pqs_cache.hpp"

namespace ksp {

struct TerrainColliders::Entry {
    PQ* quad = nullptr;
    uint64_t key = 0;
    int quadId = 0;
    // geometry snapshot taken at the event (the pooled quad is reused and rebuilt later)
    Vector3d centre;
    std::vector<Vector3f> vertices;
    std::vector<int> indices;
    bool cooked = false;
};

namespace {

uint64_t Fnv(uint64_t h, const void* data, size_t n) {
    const unsigned char* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 1099511628211ull;
    }
    return h;
}

}  // namespace

uint64_t TerrainColliders::ColliderGeometryHash(const Vector3d& centre, const std::vector<Vector3f>& vertices,
                                                const std::vector<int>& indices) {
    uint64_t h = 14695981039346656037ull;
    const double c[3] = {centre.x, centre.y, centre.z};
    h = Fnv(h, c, sizeof c);
    const uint32_t nv = static_cast<uint32_t>(vertices.size());
    h = Fnv(h, &nv, sizeof nv);
    for (const Vector3f& v : vertices) {
        const float f[3] = {v.x, v.y, v.z};
        h = Fnv(h, f, sizeof f);
    }
    const uint32_t ni = static_cast<uint32_t>(indices.size());
    h = Fnv(h, &ni, sizeof ni);
    for (int i : indices) {
        const int32_t x = static_cast<int32_t>(i);
        h = Fnv(h, &x, sizeof x);
    }
    return h;
}

TerrainColliders::TerrainColliders(physics::PhysicsWorld& world, PQS& sphere, const TerrainCollidersOptions& options)
    : world_(world), sphere_(sphere), opt_(options) {
    prevEnabled_ = sphere_.onQuadColliderEnabled;
    prevDisabled_ = sphere_.onQuadColliderDisabled;
    sphere_.onQuadColliderEnabled = [this](PQ& q) {
        if (prevEnabled_) prevEnabled_(q);
        OnEnabled(q);
    };
    sphere_.onQuadColliderDisabled = [this](PQ& q) {
        if (prevDisabled_) prevDisabled_(q);
        OnDisabled(q);
    };
}

TerrainColliders::~TerrainColliders() {
    sphere_.onQuadColliderEnabled = prevEnabled_;
    sphere_.onQuadColliderDisabled = prevDisabled_;
    for (auto& kv : entries_)
        if (kv.second->cooked) world_.RemoveStatic(kv.second->key);
}

size_t TerrainColliders::AdoptExisting() {
    size_t n = 0;
    std::vector<PQ*> stack;
    for (auto it = sphere_.quads.rbegin(); it != sphere_.quads.rend(); ++it)
        if (*it != nullptr) stack.push_back(*it);
    while (!stack.empty()) {
        PQ* q = stack.back();
        stack.pop_back();
        if (q->meshColliderEnabled && entries_.find(q) == entries_.end()) {
            OnEnabled(*q);
            ++n;
        }
        if (q->isSubdivided)
            for (int i = 3; i >= 0; --i)
                if (q->subNodes[static_cast<size_t>(i)] != nullptr) stack.push_back(q->subNodes[static_cast<size_t>(i)]);
    }
    return n;
}

void TerrainColliders::OnEnabled(PQ& quad) {
    stats_.enabledEvents++;
    auto it = entries_.find(&quad);
    if (it != entries_.end()) {
        // the sharedMesh of an enabled collider is reassigned (the quad was rebuilt): replace
        Entry& old = *it->second;
        if (old.cooked) {
            world_.RemoveStatic(old.key);
            stats_.replaced++;
        } else {
            queue_.erase(std::remove(queue_.begin(), queue_.end(), &old), queue_.end());
            stats_.dropped++;
        }
        entries_.erase(it);
    }
    auto e = std::make_unique<Entry>();
    e->quad = &quad;
    e->quadId = quad.id;
    e->key = opt_.keyBase + static_cast<uint32_t>(quad.id);
    // geometry: the quad mesh as of now (Unity bakes sharedMesh at the assignment): triangles = cacheIndices[0]
    e->centre = quad.positionPlanet + opt_.origin;
    e->indices = sphere_.meshCache().cacheIndices[0];
    const size_t n = quad.vertsD.size();
    e->vertices.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        const Vector3d d = quad.vertsD[i] - quad.positionPlanet;
        e->vertices.push_back(Vector3f(static_cast<float>(d.x), static_cast<float>(d.y), static_cast<float>(d.z)));
    }
    if (opt_.recordEvents) {
        ColliderEvent ev;
        ev.seq = seq_;
        ev.enabled = true;
        ev.quadId = quad.id;
        ev.name = quad.name;
        ev.subdivision = quad.subdivision;
        ev.geometryHash = ColliderGeometryHash(e->centre, e->vertices, e->indices);
        events_.push_back(std::move(ev));
    }
    ++seq_;
    Entry* raw = e.get();
    entries_[&quad] = std::move(e);
    if (opt_.maxCooksPerUpdate > 0) {
        queue_.push_back(raw);
    } else {
        Cook(*raw);
    }
}

void TerrainColliders::OnDisabled(PQ& quad) {
    stats_.disabledEvents++;
    if (opt_.recordEvents) {
        ColliderEvent ev;
        ev.seq = seq_;
        ev.enabled = false;
        ev.quadId = quad.id;
        ev.name = quad.name;
        ev.subdivision = quad.subdivision;
        events_.push_back(std::move(ev));
    }
    ++seq_;
    auto it = entries_.find(&quad);
    if (it == entries_.end()) return;  // disabled twice (OnQuadDestroy fires for every destroy of a quad that ever had one)
    Entry& e = *it->second;
    if (e.cooked) {
        world_.RemoveStatic(e.key);
        stats_.removed++;
    } else {
        queue_.erase(std::remove(queue_.begin(), queue_.end(), &e), queue_.end());
        stats_.dropped++;
    }
    entries_.erase(it);
}

void TerrainColliders::Cook(Entry& e) {
    const auto t0 = std::chrono::steady_clock::now();
    const physics::BodyId id = world_.AddStaticMesh(e.key, e.centre, e.vertices, e.indices, opt_.friction, opt_.restitution);
    const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    stats_.cookSeconds += s;
    stats_.maxCookSeconds = std::max(stats_.maxCookSeconds, s);
    if (id == physics::kInvalidBody) {
        stats_.failed++;
        return;
    }
    e.cooked = true;
    stats_.cooked++;
    // the snapshot is no longer needed
    e.vertices.clear();
    e.vertices.shrink_to_fit();
    e.indices.clear();
    e.indices.shrink_to_fit();
}

size_t TerrainColliders::Update() {
    if (opt_.maxCooksPerUpdate <= 0 || queue_.empty()) return 0;
    const size_t n = std::min(queue_.size(), static_cast<size_t>(opt_.maxCooksPerUpdate));
    for (size_t i = 0; i < n; ++i) Cook(*queue_[i]);
    queue_.erase(queue_.begin(), queue_.begin() + static_cast<std::ptrdiff_t>(n));
    world_.OptimizeBroadPhase();
    return n;
}

size_t TerrainColliders::CookedCount() const {
    size_t n = 0;
    for (const auto& kv : entries_) n += kv.second->cooked ? 1 : 0;
    return n;
}

size_t TerrainColliders::PendingCount() const { return queue_.size(); }

std::vector<int> TerrainColliders::WorldQuadIds() const {
    std::vector<int> out;
    for (uint64_t k : world_.StaticKeys())
        if (k >= opt_.keyBase && k - opt_.keyBase <= 0xFFFFFFFFull) out.push_back(static_cast<int>(static_cast<uint32_t>(k - opt_.keyBase)));
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<int> TerrainColliders::TrackedQuadIds() const {
    std::vector<int> out;
    for (const auto& kv : entries_) out.push_back(kv.second->quadId);
    std::sort(out.begin(), out.end());
    return out;
}

TerrainHit TerrainColliders::Convert(const physics::PhysicsWorld::RayHit& h) const {
    TerrainHit t;
    if (!h.hit) return t;
    t.hit = true;
    t.distance = h.distance;
    t.point = h.point;
    t.normal = h.normal;
    t.quadId = static_cast<int>(static_cast<uint32_t>(h.userData - opt_.keyBase));
    t.radius = (h.point - opt_.origin).magnitude();
    t.height = t.radius - sphere_.radius;
    return t;
}

TerrainHit TerrainColliders::RayCast(const Vector3d& origin, const Vector3d& direction, double maxDistance) const {
    physics::PhysicsWorld::RayHit h = world_.RayCast(origin, direction, maxDistance, physics::PhysicsWorld::kLayerStatic);
    if (!h.hit) {
        // Neighbouring quads are separate meshes whose float vertices are rounded relative to their own centres: a ray through a
        // vertex shared by two of them can pass the crack between them (<= 1e-4 m wide) and Jolt's triangle test is not watertight
        // across bodies. Retry from four points 2 mm to the side.
        const Vector3d d = direction.normalized();
        const Vector3d axis = std::fabs(d.x) < 0.57 ? Vector3d(1, 0, 0) : (std::fabs(d.y) < 0.57 ? Vector3d(0, 1, 0) : Vector3d(0, 0, 1));
        const Vector3d u = Vector3d::Cross(d, axis).normalized() * 0.002;
        const Vector3d v = Vector3d::Cross(d, u).normalized() * 0.002;
        const Vector3d nudge[4] = {u, u * -1.0, v, v * -1.0};
        for (const Vector3d& n : nudge) {
            h = world_.RayCast(origin + n, direction, maxDistance, physics::PhysicsWorld::kLayerStatic);
            if (h.hit) break;
        }
    }
    return Convert(h);
}

TerrainHit TerrainColliders::RayDown(const Vector3d& point, double maxDistance) const {
    const Vector3d up = (point - opt_.origin).normalized();
    return RayCast(point, up * -1.0, maxDistance);
}

TerrainHit TerrainColliders::SurfaceAt(const Vector3d& point, double lift, double depth) const {
    const Vector3d up = (point - opt_.origin).normalized();
    return RayCast(point + up * lift, up * -1.0, lift + depth);
}

bool TerrainColliders::HeightAboveTerrain(const Vector3d& point, double& height, double maxDistance) const {
    const Vector3d up = (point - opt_.origin).normalized();
    TerrainHit h = RayCast(point, up * -1.0, maxDistance);
    if (h.hit) {
        height = h.distance;
        return true;
    }
    h = RayCast(point, up, maxDistance);
    if (h.hit) {
        height = -h.distance;
        return true;
    }
    return false;
}

bool TerrainColliders::SurfaceNormal(const Vector3d& point, Vector3d& normal, double lift, double depth) const {
    const TerrainHit h = SurfaceAt(point, lift, depth);
    if (h.hit) normal = h.normal;
    return h.hit;
}

size_t TerrainColliders::Overlap(const Vector3d& centre, double radius, std::vector<int>& quadIds) const {
    std::vector<physics::BodyId> bodies;
    world_.OverlapSphere(centre, radius, bodies, physics::PhysicsWorld::kLayerStatic);
    quadIds.clear();
    for (physics::BodyId b : bodies) {
        const uint64_t k = world_.UserData(b);
        if (k >= opt_.keyBase && k - opt_.keyBase <= 0xFFFFFFFFull) quadIds.push_back(static_cast<int>(static_cast<uint32_t>(k - opt_.keyBase)));
    }
    std::sort(quadIds.begin(), quadIds.end());
    return quadIds.size();
}

}  // namespace ksp
