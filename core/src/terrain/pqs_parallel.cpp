// WP-12a: parallel height pass for quads about to be built (see include/ksp/terrain/pqs_parallel.hpp).
#include "ksp/terrain/pqs_parallel.hpp"

#include <array>
#include <cstring>

#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/unity/matrix_native.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/simplex.hpp"

namespace ksp {

namespace {
constexpr uint32_t kSentinel = 0x7FBADBADu;  // a NaN no height mod writes
float Sentinel() {
    float f;
    std::memcpy(&f, &kSentinel, 4);
    return f;
}
bool IsSentinel(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u == kSentinel;
}
bool SameBits(const Matrix4x4f& a, const Matrix4x4f& b) { return std::memcmp(&a, &b, sizeof(Matrix4x4f)) == 0; }
}  // namespace

bool PQSBuildHeights::Matches(const PQ& q, const Matrix4x4f& m) const {
    return subdivision == q.subdivision && SameBits(quadMatrix, m) &&
           std::memcmp(&positionPlanetRelative, &q.positionPlanetRelative, sizeof(Vector3d)) == 0;
}

// The height half of BuildQuad (PQS.cs:2249-2262, 2277-2287) on this sphere, for the geometry of `quad` with the quad
// matrix `m`; see PQSHeightWorkers.
void PQS::EvaluateBuildHeights(const PQ& quad, const Matrix4x4f& m, const PQSMeshCache& cache, PQSBuildHeights& out) {
    PQ probe;  // the quad as the height mods see it
    probe.positionPlanetRelative = quad.positionPlanetRelative;
    probe.subdivision = quad.subdivision;
    probe.plane = quad.plane;
    probe.quadMatrix = m;
    out.quadMatrix = m;
    out.positionPlanetRelative = quad.positionPlanetRelative;
    out.subdivision = quad.subdivision;
    const size_t n = static_cast<size_t>(cache.cacheVertCount);
    out.vertHeight.resize(n);
    out.vertColor.resize(n);
    out.colorWritten.resize(n);
    out.allowScatter.resize(n);
    Mod_OnQuadPreBuild(probe);
    vbData_.buildQuad = &probe;
    vbData_.allowScatter = true;
    vbData_.gnomonicPlane = static_cast<int>(probe.plane);
    const float s = Sentinel();
    for (int i = 0; i < cache.cacheVertCount; i++) {
        const size_t k = static_cast<size_t>(i);
        vertexIndex = i;
        vbData_.globalV = Vector3d(MultiplyPoint3x4(m, cache.cacheVerts[k]));
        vbData_.directionFromCenter = vbData_.globalV.normalized();
        vbData_.vertHeight = radius;
        vbData_.vertIndex = i;
        vbData_.vertColor = Color(s, s, s, s);
        Mod_OnVertexBuildHeight(vbData_, false);
        out.vertHeight[k] = vbData_.vertHeight;
        out.vertColor[k] = vbData_.vertColor;
        out.colorWritten[k] = static_cast<uint8_t>((IsSentinel(vbData_.vertColor.r) ? 0 : 1) | (IsSentinel(vbData_.vertColor.g) ? 0 : 2) |
                                                   (IsSentinel(vbData_.vertColor.b) ? 0 : 4) | (IsSentinel(vbData_.vertColor.a) ? 0 : 8));
        out.allowScatter[k] = vbData_.allowScatter ? 1 : 0;
    }
    vertexIndex = cache.cacheVertCount;
    Mod_OnQuadBuilt(probe);
    vbData_.buildQuad = nullptr;
}

PQSHeightWorkers::PQSHeightWorkers(const Factory& factory, int threads) {
    // building a stock stack reseeds UnityEngine.Random and rewrites the Simplex tables (WP-11): keep the caller's state
    const std::array<int, 256> simplexP = Simplex::StaticPermutation();
    const std::array<int, 256> simplexNoiseP = SimplexNoise::StaticPermutation();
    const UnityRandom::State random = UnityRandom::Global().state();
    for (int i = 0; i < threads; i++) spheres_.push_back(factory());
    Simplex::StaticPermutation() = simplexP;
    SimplexNoise::StaticPermutation() = simplexNoiseP;
    UnityRandom::Global().SetState(random);
    for (int i = 0; i < threads; i++) threads_.emplace_back([this, i] { WorkerLoop(i); });
}

PQSHeightWorkers::~PQSHeightWorkers() {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto& t : threads_) t.join();
}

void PQSHeightWorkers::WorkerLoop(int index) {
    uint64_t seen = 0;
    for (;;) {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            cv_.wait(lk, [&] { return stop_ || generation_ != seen; });
            if (stop_) return;
            seen = generation_;
        }
        for (;;) {
            size_t k;
            {
                std::lock_guard<std::mutex> lk(mutex_);
                if (next_ >= jobs_->size()) break;
                k = next_++;
            }
            const auto& job = (*jobs_)[k];
            spheres_[static_cast<size_t>(index)]->EvaluateBuildHeights(*job.first, job.second, *cache_, out_[k]);
        }
        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (--busy_ == 0) done_.notify_one();
        }
    }
}

void PQSHeightWorkers::Prefetch(const std::vector<std::pair<PQ*, Matrix4x4f>>& quads, const PQSMeshCache& cache) {
    if (quads.empty() || spheres_.empty()) return;
    out_.assign(quads.size(), PQSBuildHeights());
    {
        std::lock_guard<std::mutex> lk(mutex_);
        jobs_ = &quads;
        cache_ = &cache;
        next_ = 0;
        busy_ = static_cast<int>(threads_.size());
        ++generation_;
    }
    cv_.notify_all();
    {
        std::unique_lock<std::mutex> lk(mutex_);
        done_.wait(lk, [&] { return busy_ == 0; });
        jobs_ = nullptr;
    }
    for (size_t i = 0; i < quads.size(); i++) results_[quads[i].first] = std::move(out_[i]);
    prefetched += static_cast<int64_t>(quads.size());
}

const PQSBuildHeights* PQSHeightWorkers::Find(const PQ& q) const {
    auto it = results_.find(&q);
    if (it == results_.end() || !it->second.Matches(q, q.quadMatrix)) return nullptr;
    return &it->second;
}

}  // namespace ksp
