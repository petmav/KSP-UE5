// WP-12a benchmark: quad build cost per quad and tree update cost per frame (not a test).
//
//   wp12_bench [repeats]
//
// For the stock Kerbin sphere at the KSC pad, the stock Mun and a flat sphere (one constant-height mod: the quad build
// without the height stack), it measures
//   * the initial build (StartSphere: UpdateQuadsInit creates the tree and builds every visible quad),
//   * a rebuild of every built leaf quad (BuildQuad again on the same quads: mean / median per quad),
//   * the height part alone (Mod_OnQuadPreBuild + 225 GetSurfaceHeight on an independent sphere, as BuildQuad
//     evaluates them), so that "quad build - heights" is the WP-12a geometry cost (directions, positions, normals,
//     tangents, channels, mod hooks),
//   * a steady-state frame (FixedUpdate + FrameUpdate with the target at rest: UpdateVisual + UpdateQuads + stitching).
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "ksp/physics/physics_world.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/terrain/pqs_mods_quad.hpp"
#include "ksp/terrain/pqs_parallel.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/terrain/terrain_colliders.hpp"
#include "ksp/unity/matrix_native.hpp"

using namespace ksp;
using Clock = std::chrono::steady_clock;

namespace {

double Ms(Clock::duration d) { return std::chrono::duration<double, std::milli>(d).count(); }

class FlatMod : public PQSMod {
public:
    void OnSetup() override { requirements = PQS::MeshCustomNormals; }
    void OnVertexBuildHeight(PQSVertexBuildData& d) override { d.vertHeight += 100.0; }
    bool AffectsHeight() const override { return true; }
};

void FlightHost(PQS& pqs, const PQSTarget* target) {
    for (auto& m : pqs.childMods) {
        if (auto* cb = dynamic_cast<PQSMod_CelestialBodyTransform*>(m.get())) {
            cb->host.hasBody = [] { return true; };
            cb->host.flightGlobalsExists = [] { return true; };
            cb->host.activeVessel = [target] { return target; };
            cb->host.isCurrentMainBody = [] { return true; };
            cb->host.loadedScene = [] { return static_cast<int>(GameScenes::FLIGHT); };
            cb->host.mapIsEnabled = [] { return false; };
        }
    }
}

void Collect(PQ* q, std::vector<PQ*>& out) {
    out.push_back(q);
    if (q->isSubdivided)
        for (PQ* c : q->subNodes)
            if (c != nullptr) Collect(c, out);
}

double Median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v.empty() ? 0.0 : v[v.size() / 2];
}

void Bench(const char* label, std::unique_ptr<PQS> sphere, std::unique_ptr<PQS> heightSphere, double lat, double lon,
           double alt, int repeats) {
    terrain::MapLibrary* unused = nullptr;
    (void)unused;
    PQSCache cache;
    cache.cachePQInitialCount = 64;
    PQSTarget target;
    target.position = terrain::GetRelSurfaceNVector(lat, lon) * (sphere->radius + alt);
    FlightHost(*sphere, &target);
    sphere->cache = &cache;
    sphere->SetTarget(&target);
    PQSCache::CreateDefaultPresetList();

    const auto t0 = Clock::now();
    sphere->Start();
    const double start = Ms(Clock::now() - t0);
    sphere->FixedUpdate();
    sphere->FrameUpdate();
    std::vector<PQ*> all;
    for (PQ* r : sphere->quads) Collect(r, all);
    std::vector<PQ*> leaves;
    for (PQ* q : all)
        if (q->isBuilt && !q->isSubdivided) leaves.push_back(q);
    const int initialBuilt = static_cast<int>(std::count_if(all.begin(), all.end(), [](PQ* q) { return q->isBuilt; }));

    // rebuild every built leaf (BuildQuad: same quad, same result)
    std::vector<double> perQuad;
    for (int r = 0; r < repeats; r++) {
        for (PQ* q : leaves) {
            q->isBuilt = false;
            const auto a = Clock::now();
            q->isBuilt = sphere->BuildQuad(*q);
            perQuad.push_back(Ms(Clock::now() - a) * 1000.0);
        }
    }
    // the height part alone, on an independent sphere in the build state
    std::vector<double> perQuadHeights;
    const PQSMeshCache& mc = cache.meshCache;
    std::vector<Vector3d> gv(static_cast<size_t>(mc.cacheVertCount));
    for (int r = 0; r < repeats; r++) {
        for (PQ* q : leaves) {
            for (int i = 0; i < mc.cacheVertCount; i++)
                gv[static_cast<size_t>(i)] = Vector3d(MultiplyPoint3x4(q->quadMatrix, mc.cacheVerts[static_cast<size_t>(i)]));
            PQ probe;
            probe.positionPlanetRelative = q->positionPlanetRelative;
            probe.subdivision = q->subdivision;
            const auto a = Clock::now();
            heightSphere->Mod_OnQuadPreBuild(probe);
            double sink = 0.0;
            for (const Vector3d& g : gv) sink += heightSphere->GetSurfaceHeight(g);
            heightSphere->Mod_OnQuadBuilt(probe);
            perQuadHeights.push_back(Ms(Clock::now() - a) * 1000.0);
            if (sink == 1.2345) std::printf("!");
        }
    }
    // WP-12b: the colliders of the built maximum-level quads: Jolt MeshShape cook + insert per quad (TerrainColliders), the
    // replacement of an enabled collider (a rebuilt quad), and radial ray casts against them
    {
        physics::PhysicsWorld world;
        TerrainCollidersOptions copt;
        TerrainColliders tc(world, *sphere, copt);
        const size_t adopted = tc.AdoptExisting();
        std::vector<double> cook;
        const auto c0 = Clock::now();
        for (int r = 0; r < repeats; r++)
            for (PQ* q : leaves) {
                if (!q->meshColliderEnabled) continue;
                const auto a = Clock::now();
                sphere->onQuadColliderEnabled(*q);
                cook.push_back(Ms(Clock::now() - a) * 1000.0);
            }
        const double cookTotal = Ms(Clock::now() - c0);
        std::vector<double> rays;
        const Vector3d pos = target.position;
        double sink = 0.0;
        const int nRays = 2000;
        const auto r0 = Clock::now();
        for (int i = 0; i < nRays; i++) {
            const Vector3d p = pos + Vector3d(0.37 * i - 300.0, 0.0, 0.21 * i - 200.0);
            sink += tc.SurfaceAt(p, 200.0, 200.0).distance;
        }
        const double rayUs = Ms(Clock::now() - r0) * 1000.0 / nRays;
        if (sink == 1.2345) std::printf("!");
        std::printf("%-26s colliders: %zu quads adopted, cook+insert per quad: mean %8.1f us, median %8.1f us, max %8.1f us (%zu cooks, %.1f ms);"
                    " first cook of all: %.1f us per quad; SurfaceAt ray cast %.2f us\n",
                    label, adopted, cook.empty() ? 0.0 : std::accumulate(cook.begin(), cook.end(), 0.0) / static_cast<double>(cook.size()),
                    Median(cook), cook.empty() ? 0.0 : *std::max_element(cook.begin(), cook.end()), cook.size(), cookTotal,
                    adopted > 0 ? tc.Stats().cookSeconds * 1.0e6 / static_cast<double>(tc.Stats().cooked) : 0.0, rayUs);
    }
    // steady-state frames (target at rest)
    std::vector<double> frame;
    for (int r = 0; r < 20 * repeats; r++) {
        const auto a = Clock::now();
        sphere->FixedUpdate();
        sphere->FrameUpdate();
        frame.push_back(Ms(Clock::now() - a));
    }
    double sum = 0.0, sumH = 0.0;
    for (double v : perQuad) sum += v;
    for (double v : perQuadHeights) sumH += v;
    const double mean = sum / static_cast<double>(perQuad.size());
    const double meanH = perQuadHeights.empty() ? 0.0 : sumH / static_cast<double>(perQuadHeights.size());
    std::printf("%-26s quads %4zu, built at start %4d; StartSphere %8.2f ms (%.1f us per built quad)\n", label, all.size(),
                initialBuilt, start, start * 1000.0 / initialBuilt);
    std::printf("%-26s BuildQuad per quad: mean %8.1f us, median %8.1f us (%zu builds); heights alone: mean %8.1f us, median %8.1f us;"
                " geometry (build - heights) %6.1f us\n",
                label, mean, Median(perQuad), perQuad.size(), meanH, Median(perQuadHeights), mean - meanH);
    std::printf("%-26s steady frame (FixedUpdate + FrameUpdate, target at rest): median %.3f ms\n", label, Median(frame));
    PQSCache::PresetList.reset();
    sphere.reset();  // the spheres return their quads to the pool: the PQSCache must outlive them
}

}  // namespace

// StartSphere (UpdateQuadsInit's tree + building pass) inline vs with N height workers (PQSHeightWorkers)
void BenchWorkers(const char* body, double lat, double lon, double alt, terrain::MapLibrary& maps, int threads, int repeats) {
    std::vector<double> times;
    int64_t used = 0;
    for (int r = 0; r < repeats; r++) {
        PQSCache cache;
        cache.cachePQInitialCount = 64;
        std::unique_ptr<PQSHeightWorkers> workers;
        if (threads > 0)
            workers = std::make_unique<PQSHeightWorkers>([&] { return terrain::LoadStockPQS(body, maps); }, threads);
        auto sphere = terrain::CreateStockPQS(body, maps);
        PQSTarget target;
        target.position = terrain::GetRelSurfaceNVector(lat, lon) * (sphere->radius + alt);
        FlightHost(*sphere, &target);
        sphere->cache = &cache;
        sphere->heightWorkers = workers.get();
        sphere->SetTarget(&target);
        PQSCache::CreateDefaultPresetList();
        const auto t0 = Clock::now();
        sphere->Start();
        times.push_back(Ms(Clock::now() - t0));
        if (workers) used = workers->used;
        sphere.reset();
        PQSCache::PresetList.reset();
    }
    std::printf("%-8s StartSphere with %2d height workers: median %8.2f ms (%lld builds took prefetched heights)\n", body, threads,
                Median(times), static_cast<long long>(used));
}

int main(int argc, char** argv) {
    const int repeats = argc > 1 ? std::atoi(argv[1]) : 3;
    terrain::MapLibrary maps(KSPCORE_DATA_DIR "/terrain");
    for (int threads : {0, 2, 4, 8, 15}) BenchWorkers("Kerbin", -0.0972, -74.5577, 70.0, maps, threads, repeats);
    for (int threads : {0, 4, 15}) BenchWorkers("Mun", 3.0, 25.0, 6000.0, maps, threads, repeats);
    Bench("Kerbin (KSC pad)", terrain::CreateStockPQS("Kerbin", maps), terrain::LoadStockPQS("Kerbin", maps), -0.0972,
          -74.5577, 70.0, repeats);
    Bench("Mun", terrain::CreateStockPQS("Mun", maps), terrain::LoadStockPQS("Mun", maps), 3.0, 25.0, 6000.0, repeats);
    auto flat = std::make_unique<PQS>();
    flat->name = "Kerbin";  // the Kerbin preset (min 2, max 9)
    flat->radius = 600000.0;
    flat->buildTangents = true;
    flat->surfaceRelativeQuads = true;
    flat->childMods.push_back(std::make_unique<FlatMod>());
    auto flatH = std::make_unique<PQS>();
    flatH->radius = 600000.0;
    flatH->childMods.push_back(std::make_unique<FlatMod>());
    flatH->SetupForHeight();
    Bench("flat (no height stack)", std::move(flat), std::move(flatH), -0.0972, -74.5577, 70.0, repeats);
    return 0;
}
