// WP-12a optimisation: evaluate the vertex heights of quads that are about to be built on worker threads.
//
// The height stack dominates the quad build (stock Kerbin: ~0.7 ms of ~0.73 ms per quad on a Ryzen 9 7950X3D, the
// geometry part ~35-40 us; tools/wp12/bench.cpp). KSP builds quads one at a time inside the tree walk; the core keeps
// that order and every side effect, but where a batch of quads is about to be built it evaluates their height pass
// (BuildQuad's Mod_OnQuadPreBuild + 225 x Mod_OnVertexBuildHeight + Mod_OnQuadBuilt, PQS.cs:2249-2262) in parallel on
// independent spheres with the same stack, and BuildQuad then takes the per-vertex results instead of running the
// height mods inline:
//   * UpdateQuadsInit, before its building pass: the leaves predicted visible (gcd1 < visibleRadius, PQ.cs:288);
//   * PQ.Subdivide, before its SetupQuad / UpdateVisibility loop: the new children predicted visible.
// A result is used only for the quad geometry it was computed for (quad matrix, centre, subdivision); a missed or wrong
// prediction only costs time (the quad is built inline). What the height pass leaves in VertexBuildData for the rest of
// the build is reproduced per vertex: vertHeight, the vertColor components the height mods wrote (detected with a NaN
// sentinel; unwritten components keep the value carried from the previous vertex, as inline), allowScatter; the map
// coordinates (PQS.BuildVertexMapCoords) are still computed by the building sphere.
//
// Exactness: bit-identical to the inline build when the stack's heights do not depend on mod scratch state carried from
// earlier evaluations (true for every stock mod; WP-11 notes the scratch fields). Gated by tests/test_pqs_quadtree.cpp:
// the four reference scenarios replayed with workers give the same quad rows and mesh digests as without (and as the
// Python reference). The mod objects of the building sphere do not run OnVertexBuildHeight for prefetched quads, so
// their scratch fields differ afterwards; nothing reads them before writing (same gate, plus the terrain_height capture
// replayed on the live sphere).
#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/color.hpp"

namespace ksp {

// What BuildQuad's height pass leaves in VertexBuildData, per vertex
struct PQSBuildHeights {
    std::vector<double> vertHeight;
    std::vector<Color> vertColor;
    std::vector<uint8_t> colorWritten;  // bit k: component k (r, g, b, a) was written by the height mods
    std::vector<uint8_t> allowScatter;
    // WP-12d: per vertex, the land-class list PQSLandControl's OnVertexBuild reads (index, normalised delta); empty without a land control
    std::vector<std::vector<std::pair<int, double>>> landControlDeltas;
    // the geometry the heights belong to
    Matrix4x4f quadMatrix;
    Vector3d positionPlanetRelative;
    int subdivision = 0;
    bool Matches(const PQ& q, const Matrix4x4f& m) const;
};

class PQSHeightWorkers {
public:
    using Factory = std::function<std::unique_ptr<PQS>()>;
    // `factory` returns an independent sphere with the same height stack, ready for GetSurfaceHeight (e.g.
    // terrain::LoadStockPQS(name, maps)). The spheres are built here, on the calling thread; building a stock stack
    // reseeds UnityEngine.Random and the Simplex tables (WP-11), so their global state is saved and restored.
    PQSHeightWorkers(const Factory& factory, int threads);
    ~PQSHeightWorkers();
    PQSHeightWorkers(const PQSHeightWorkers&) = delete;
    PQSHeightWorkers& operator=(const PQSHeightWorkers&) = delete;

    // Evaluate the height pass of `quads` (each with the quad matrix its build will use) in parallel; blocks until done.
    void Prefetch(const std::vector<std::pair<PQ*, Matrix4x4f>>& quads, const PQSMeshCache& cache);
    // The result for `q` if one was prefetched for this geometry (consumed); nullptr otherwise
    const PQSBuildHeights* Find(const PQ& q) const;
    void Clear() { results_.clear(); }

    int threads() const { return static_cast<int>(spheres_.size()); }
    int64_t prefetched = 0, used = 0;

private:
    void WorkerLoop(int index);
    std::vector<std::unique_ptr<PQS>> spheres_;
    std::vector<std::thread> threads_;
    std::mutex mutex_;
    std::condition_variable cv_, done_;
    const std::vector<std::pair<PQ*, Matrix4x4f>>* jobs_ = nullptr;
    const PQSMeshCache* cache_ = nullptr;
    std::vector<PQSBuildHeights> out_;
    size_t next_ = 0;
    int busy_ = 0;
    uint64_t generation_ = 0;
    bool stop_ = false;
    std::unordered_map<const PQ*, PQSBuildHeights> results_;
};

}  // namespace ksp
