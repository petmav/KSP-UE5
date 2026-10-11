// WP-12a: PQS quadtree and quad build (ksp/terrain/pq.hpp, pqs.hpp, pqs_cache.hpp, pqs_mods_quad.hpp).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/terrain/pqs_mods.hpp"
#include "ksp/terrain/pqs_mods_quad.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/matrix_native.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/simplex.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;

namespace {

const std::string kData = KSPCORE_DATA_DIR;
const std::string kHarness = KSPCORE_HARNESS_DIR;

terrain::MapLibrary& Maps() {
    static terrain::MapLibrary maps(kData + "/terrain");
    return maps;
}

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
uint64_t Bits(double d) {
    uint64_t u;
    std::memcpy(&u, &d, 8);
    return u;
}

// Simplex / UnityEngine.Random global state is rewritten by building a PQS (see test_terrain.cpp)
struct GlobalStateGuard {
    std::array<int, 256> simplexP = Simplex::StaticPermutation();
    std::array<int, 256> simplexNoiseP = SimplexNoise::StaticPermutation();
    UnityRandom::State random = UnityRandom::Global().state();
    ~GlobalStateGuard() {
        Simplex::StaticPermutation() = simplexP;
        SimplexNoise::StaticPermutation() = simplexNoiseP;
        UnityRandom::Global().SetState(random);
    }
};

// The flight-scene host of a stock sphere: FlightGlobals present, the active vessel is `target`, this body is the
// main body (PQSMod_CelestialBodyTransform activates the sphere below deactivateAltitude).
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

int CountQuads(const PQ* q, std::map<int, int>& perLevel, int& built, int& visible) {
    if (q == nullptr) return 0;
    perLevel[q->subdivision]++;
    if (q->isBuilt) built++;
    if (q->isVisible) visible++;
    int n = 1;
    if (q->isSubdivided)
        for (const PQ* c : q->subNodes) n += CountQuads(c, perLevel, built, visible);
    return n;
}

}  // namespace

TEST_CASE("WP-12a native Matrix4x4.TRS and Vector3.OrthoNormalize vs the C3 capture") {
    const nlohmann::json cap = nlohmann::json::parse(std::ifstream(kHarness + "/captures/out/math_shim_native.json"));
    const auto& tab = cap.at("tables").at(0);
    std::vector<std::string> cols;
    for (const auto& c : tab.at("columns")) cols.push_back(c.at("name").get<std::string>());
    auto col = [&](const std::string& n) {
        return static_cast<size_t>(std::find(cols.begin(), cols.end(), n) - cols.begin());
    };
    auto hexf = [](const nlohmann::json& j) {
        const uint32_t u = static_cast<uint32_t>(std::stoul(j.get<std::string>(), nullptr, 16));
        float f;
        std::memcpy(&f, &u, 4);
        return f;
    };
    int trs = 0, on = 0;
    for (const auto& r : tab.at("rows")) {
        const std::string fn = r[col("fn")].get<std::string>();
        auto a = [&](int i) { return hexf(r[col("a" + std::to_string(i) + "_hex")]); };
        auto res = [&](int i) { return hexf(r[col("r" + std::to_string(i) + "_hex")]); };
        if (fn == "Matrix4x4.TRS") {
            const Matrix4x4f m = UnityNative::TRS(Vector3f(a(0), a(1), a(2)), Quaternionf(a(3), a(4), a(5), a(6)), Vector3f(a(7), a(8), a(9)));
            const float mine[16] = {m.m00, m.m10, m.m20, m.m30, m.m01, m.m11, m.m21, m.m31,
                                    m.m02, m.m12, m.m22, m.m32, m.m03, m.m13, m.m23, m.m33};
            for (int i = 0; i < 16; i++) CHECK(Bits(mine[i]) == Bits(res(i)));
            trs++;
        } else if (fn == "Vector3.OrthoNormalize") {
            Vector3f u(a(0), a(1), a(2)), v(a(3), a(4), a(5));
            UnityNative::OrthoNormalize(u, v);
            const float mine[6] = {u.x, u.y, u.z, v.x, v.y, v.z};
            for (int i = 0; i < 6; i++) CHECK(Bits(mine[i]) == Bits(res(i)));
            on++;
        }
    }
    CHECK(trs == 45);
    CHECK(on == 49);
}

TEST_CASE_FIXTURE(GlobalStateGuard, "WP-12a smoke: Kerbin quadtree around the KSC pad") {
    PQSCache cache;
    cache.cachePQInitialCount = 64;
    PQSCache::CreateDefaultPresetList();
    auto kerbin = terrain::CreateStockPQS("Kerbin", Maps());
    kerbin->cache = &cache;
    PQSTarget vessel;
    vessel.position = terrain::GetRelSurfaceNVector(-0.0972, -74.5577) * (600000.0 + 70.0);
    FlightHost(*kerbin, &vessel);
    kerbin->Start();
    CHECK(kerbin->isStarted);
    CHECK(kerbin->isAlive);
    for (int f = 0; f < 4; f++) {
        kerbin->FixedUpdate();
        kerbin->FrameUpdate();
    }
    CHECK(kerbin->isActive);
    std::map<int, int> perLevel;
    int built = 0, visible = 0, total = 0;
    for (const PQ* q : kerbin->quads) total += CountQuads(q, perLevel, built, visible);
    std::string lv;
    for (auto& [l, n] : perLevel) lv += std::to_string(l) + ":" + std::to_string(n) + " ";
    MESSAGE("quads " << total << " built " << built << " visible " << visible << " levels " << lv
                     << " maxLevel " << kerbin->maxLevel << " pool " << cache.quadsMaterialised());
    CHECK(total > 6);
    CHECK(perLevel.rbegin()->first == kerbin->maxLevel);
    PQSCache::PresetList.reset();
}
