// WP-12a: PQS quadtree and quad build (ksp/terrain/pq.hpp, pqs.hpp, pqs_cache.hpp, pqs_mods_quad.hpp).
//
// Gate: tests/data/wp12_reference.json from tools/wp12/pqs_reference.py, an independent Python port of PQ.cs / PQS.cs /
// PQSCache.cs and the quad mods (no kspcore code; its native Unity functions are checked against the in-game C3 capture
// and its x87 Sin / Cos against the in-game libm capture). Every scenario is replayed here frame by frame and compared
// bit for bit: the static mesh cache, the sphere state, every live quad (ids, names, subdivision, quad index, flags,
// edge state, triangle set, neighbours, gcDist / gcd1 / subdivide factor / positionPlanet bits, meshVertMin / Max) and a
// SHA-256 digest of every built quad's mesh data (positions, normals, tangents, uv..uv4, colours, triangle set, double
// vertices, origin, vertex normals, edge normals). Stock Kerbin / Mun: heights come from the WP-11 function through the
// reference's height oracle, so only the sampled quads and their stitching sources carry a digest.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "datasets.hpp"  // Sha256Hex (tools/golden_runner/sha256.cpp)
#include "doctest/doctest.h"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/physics/physics_world.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/terrain/pqs_mods.hpp"
#include "ksp/terrain/pqs_mods_quad.hpp"
#include "ksp/terrain/pqs_parallel.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/terrain/terrain_colliders.hpp"
#include "ksp/unity/matrix_native.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/simplex.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using nlohmann::json;

namespace {

const std::string kData = KSPCORE_DATA_DIR;
const std::string kHarness = KSPCORE_HARNESS_DIR;
const std::string kTestData = KSPCORE_TEST_DATA_DIR;

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
std::string HexD(double d) {
    char b[24];
    std::snprintf(b, sizeof b, "%016llx", static_cast<unsigned long long>(Bits(d)));
    return b;
}
std::string HexF(float f) {
    char b[16];
    std::snprintf(b, sizeof b, "%08x", Bits(f));
    return b;
}
double FromHexD(const json& j) {
    const uint64_t u = std::stoull(j.get<std::string>(), nullptr, 16);
    double d;
    std::memcpy(&d, &u, 8);
    return d;
}

const json& Reference() {
    static const json ref = json::parse(std::ifstream(kTestData + "/wp12_reference.json"));
    return ref;
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
        PQSCache::PresetList.reset();
    }
};

// The reference's synthetic height mod (tools/wp12/pqs_ref_sphere.py SynthHeight): an analytic height in double
// arithmetic, plus colour / uv2 / uv4 data so every PQS channel builder sees values.
class SynthHeightMod : public PQSMod {
public:
    int setupReq = 0;
    double A = 0, B = 0, C = 0, D = 0, E = 0, K1 = 0, K2 = 0, F = 0, hmax = 0;
    void OnSetup() override { requirements = setupReq; }
    void OnVertexBuildHeight(PQSVertexBuildData& d) override {
        const double x = d.directionFromCenter.x, y = d.directionFromCenter.y, z = d.directionFromCenter.z;
        double h = A * (x * y) + B * ((z * z) * x) - C * (y * z) + D * (x - 0.3 * z);
        const double tx = x * K1;
        const double tz = z * K2;
        const double wx = dotnet::Math::Abs((tx - dotnet::Math::Floor(tx)) - 0.5);
        const double wz = dotnet::Math::Abs((tz - dotnet::Math::Floor(tz)) - 0.5);
        h += E * (wx * wz);
        if (F != 0.0) h += F * (d.latitude * d.longitude);
        d.vertHeight += h;
    }
    void OnVertexBuild(PQSVertexBuildData& d) override {
        d.vertColor = Color(static_cast<float>(d.directionFromCenter.x), static_cast<float>(d.directionFromCenter.y),
                            static_cast<float>(d.directionFromCenter.z), d.vertColor.a);
        d.u2 = d.vertHeight * 0.001;
        d.v2 = d.directionFromCenter.y;
        d.u4 = d.directionFromCenter.x - d.directionFromCenter.z;
        d.v4 = d.vertHeight;
    }
    double GetVertexMaxHeight() override { return hmax; }
    double GetVertexMinHeight() override { return -hmax; }
    bool AffectsHeight() const override { return true; }
};

// The flight-scene host of PQSMod_CelestialBodyTransform: FlightGlobals present, the active vessel is `target`, this
// body is the main body, map view off.
void FlightHost(PQSMod_CelestialBodyTransform& cb, const PQSTarget* target) {
    cb.host.hasBody = [] { return true; };
    cb.host.flightGlobalsExists = [] { return true; };
    cb.host.activeVessel = [target] { return target; };
    cb.host.isCurrentMainBody = [] { return true; };
    cb.host.loadedScene = [] { return static_cast<int>(GameScenes::FLIGHT); };
    cb.host.mapIsEnabled = [] { return false; };
}

PQSMod_CelestialBodyTransform::AltitudeFade Fade(const json& a) {
    PQSMod_CelestialBodyTransform::AltitudeFade f;
    f.fadeStart = a[0].get<float>();
    f.fadeEnd = a[1].get<float>();
    f.valueStart = a[2].get<float>();
    f.valueEnd = a[3].get<float>();
    return f;
}

// ---- the mesh digest (tools/wp12/pqs_reference.py mesh_bytes) ----
template <class T>
void Put(std::string& out, const T& v) {
    out.append(reinterpret_cast<const char*>(&v), sizeof v);
}
std::string MeshDigest(const PQ& q) {
    std::string b;
    auto v3 = [&](const std::vector<Vector3f>& a) {
        Put(b, static_cast<uint32_t>(a.size()));
        for (const auto& v : a) {
            Put(b, v.x);
            Put(b, v.y);
            Put(b, v.z);
        }
    };
    auto v2 = [&](const std::vector<Vector2f>& a) {
        Put(b, static_cast<uint32_t>(a.size()));
        for (const auto& v : a) {
            Put(b, v.x);
            Put(b, v.y);
        }
    };
    const PQMesh& m = q.mesh;
    v3(m.vertices);
    v3(m.normals);
    Put(b, static_cast<uint32_t>(m.tangents.size()));
    for (const auto& t : m.tangents) {
        Put(b, t.x);
        Put(b, t.y);
        Put(b, t.z);
        Put(b, t.w);
    }
    v2(m.uv);
    v2(m.uv2);
    v2(m.uv3);
    v2(m.uv4);
    Put(b, static_cast<uint32_t>(m.colors.size()));
    for (const auto& c : m.colors) {
        Put(b, c.r);
        Put(b, c.g);
        Put(b, c.b);
        Put(b, c.a);
    }
    Put(b, static_cast<int32_t>(m.triangleSet));
    Put(b, static_cast<uint32_t>(q.vertsD.size()));
    for (const auto& v : q.vertsD) {
        Put(b, v.x);
        Put(b, v.y);
        Put(b, v.z);
    }
    Put(b, q.meshOrigin.x);
    Put(b, q.meshOrigin.y);
    Put(b, q.meshOrigin.z);
    v3(q.vertNormals);
    for (const auto& e : q.edgeNormals) v3(e);
    return Sha256Hex(b).substr(0, 16);
}

void Walk(const PQ* q, std::vector<const PQ*>& out) {
    out.push_back(q);
    if (q->isSubdivided)
        for (const PQ* c : q->subNodes)
            if (c != nullptr) Walk(c, out);
}
std::vector<const PQ*> WalkSphere(const PQS& s) {
    std::vector<const PQ*> out;
    for (const PQ* r : s.quads) Walk(r, out);
    return out;
}
int Id(const PQ* q) { return q == nullptr ? -1 : q->id; }

json SnapshotQuad(const PQ& q, bool digest) {
    const int flags = (q.isActive ? 1 : 0) | (q.isSubdivided ? 2 : 0) | (q.isVisible ? 4 : 0) | (q.isForcedInvisible ? 8 : 0) |
                      (q.isBuilt ? 16 : 0) | (q.isQueuedForNormalUpdate ? 32 : 0) | (q.isQueuedOnlyForCornerNormalUpdate ? 64 : 0) |
                      (q.isCached ? 128 : 0) | (q.isPendingCollapse ? 256 : 0) | (q.rendererEnabled ? 512 : 0) |
                      (q.meshColliderCreated ? 1024 : 0) | (q.meshColliderEnabled ? 2048 : 0);
    return json::array({q.id, q.name, q.subdivision, q.quadIndex, static_cast<int>(q.plane), flags, static_cast<int>(q.edgeState),
                        q.mesh.triangleSet, Id(q.north), Id(q.south), Id(q.east), Id(q.west), Id(q.parent), HexD(q.gcDist),
                        HexD(q.gcd1), HexD(q.subdivideThresholdFactor), HexD(q.positionPlanet.x), HexD(q.positionPlanet.y),
                        HexD(q.positionPlanet.z), digest ? HexD(q.meshVertMin) : std::string(), digest ? HexD(q.meshVertMax) : std::string(),
                        digest ? MeshDigest(q) : std::string()});
}

struct Scene {
    PQSCache cache;
    std::unique_ptr<PQSHeightWorkers> workers;  // optional (WP-12a optimisation), outlives the sphere
    std::unique_ptr<PQS> sphere;
    PQSTarget target;
    PQSMod_CelestialBodyTransform* cbt = nullptr;
    int clockCalls = 0;
    float clockStep = 0.0f;
};

// The synthetic sphere of a scenario (sc != nullptr: the scene's main sphere, with the flight host)
std::unique_ptr<PQS> MakeSynthSphere(const json& s, Scene* sc) {
    const json& sp = s.at("sphere");
    auto sphere = std::make_unique<PQS>();
    {
        PQS& p = *sphere;
        p.name = sp.at("name").get<std::string>();
        p.radius = sp.at("radius").get<double>();
        p.surfaceRelativeQuads = sp.at("surfaceRelativeQuads").get<bool>();
        p.buildTangents = sp.at("buildTangents").get<bool>();
        for (const json& m : s.at("stack")) {
            const std::string t = m.at("type").get<std::string>();
            std::unique_ptr<PQSMod> mod;
            if (t == "SynthHeight") {
                auto x = std::make_unique<SynthHeightMod>();
                x->setupReq = m.at("req").get<int>();
                x->A = m.at("A").get<double>();
                x->B = m.at("B").get<double>();
                x->C = m.at("C").get<double>();
                x->D = m.at("D").get<double>();
                x->E = m.at("E").get<double>();
                x->K1 = m.at("K1").get<double>();
                x->K2 = m.at("K2").get<double>();
                x->F = m.at("F").get<double>();
                x->hmax = m.at("hmax").get<double>();
                mod = std::move(x);
            } else if (t == "FlattenArea") {
                auto x = std::make_unique<PQSMod_FlattenArea>();
                x->outerRadius = m.at("outerRadius").get<double>();
                x->innerRadius = m.at("innerRadius").get<double>();
                const json& pos = m.at("position");
                x->position = Vector3f(static_cast<float>(pos[0].get<double>()), static_cast<float>(pos[1].get<double>()),
                                       static_cast<float>(pos[2].get<double>()));
                x->flattenTo = m.at("flattenTo").get<double>();
                x->smoothStart = m.at("smoothStart").get<double>();
                x->smoothEnd = m.at("smoothEnd").get<double>();
                mod = std::move(x);
            } else if (t == "QuadEnhanceCoast") {
                auto x = std::make_unique<PQSMod_QuadEnhanceCoast>();
                x->coastLessThan = m.at("coastLessThan").get<double>();
                x->oceanFactor = m.at("oceanFactor").get<double>();
                x->coastFactor = m.at("coastFactor").get<double>();
                mod = std::move(x);
            } else if (t == "UVPlanetRelativePosition") {
                mod = std::make_unique<PQSMod_UVPlanetRelativePosition>();
            } else if (t == "AltitudeUV") {
                auto x = std::make_unique<PQSMod_AltitudeUV>();
                x->atmosphereHeight = m.at("atmosphereHeight").get<double>();
                x->oceanDepth = m.at("oceanDepth").get<double>();
                x->invert = m.at("invert").get<bool>();
                mod = std::move(x);
            } else if (t == "AltitudeAlpha") {
                auto x = std::make_unique<PQSMod_AltitudeAlpha>();
                x->atmosphereDepth = m.at("atmosphereDepth").get<double>();
                x->invert = m.at("invert").get<bool>();
                mod = std::move(x);
            } else if (t == "QuadMeshColliders") {
                auto x = std::make_unique<PQSMod_QuadMeshColliders>();
                x->maxLevelOffset = m.at("maxLevelOffset").get<int>();
                mod = std::move(x);
            } else if (t == "CelestialBodyTransform") {
                auto x = std::make_unique<PQSMod_CelestialBodyTransform>();
                x->deactivateAltitude = m.at("deactivateAltitude").get<double>();
                x->highQualityShaderDeactivateAltitude = m.at("hqDeactivate").get<double>();
                x->planetFade = Fade(m.at("planetFade"));
                for (const json& f : m.at("secondaryFades")) x->secondaryFades.push_back(Fade(f));
                if (sc != nullptr) {
                    FlightHost(*x, &sc->target);
                    sc->cbt = x.get();
                }
                mod = std::move(x);
            } else {
                FAIL("unknown mod " << t);
            }
            mod->className = "PQSMod_" + t;
            mod->order = m.at("order").get<int>();
            p.childMods.push_back(std::move(mod));
        }
    }
    return sphere;
}

void BuildScene(Scene& sc, const json& s, GlobalStateGuard&, int workerThreads = 0) {
    sc.cache.cachePQInitialCount = 64;
    const json& sp = s.at("sphere");
    if (s.at("kind") == "synth") {
        sc.sphere = MakeSynthSphere(s, &sc);
        PQS& p = *sc.sphere;
        const json& pr = sp.at("preset");
        PQSCache::PresetList = std::make_unique<PQSCache::PQSGlobalPresetList>();
        PQSCache::PQSPreset preset;
        preset.name = "Test";
        preset.spherePresets.emplace_back(p.name, pr[0].get<double>(), pr[1].get<int>(), pr[2].get<int>());
        PQSCache::PresetList->presets.push_back(preset);
        PQSCache::PresetList->preset = "Test";
    } else {
        sc.sphere = terrain::CreateStockPQS(s.at("body").get<std::string>(), Maps());
        for (auto& m : sc.sphere->childMods) {
            if (auto* cb = dynamic_cast<PQSMod_CelestialBodyTransform*>(m.get())) {
                FlightHost(*cb, &sc.target);
                sc.cbt = cb;
            }
        }
        PQSCache::CreateDefaultPresetList();  // "Default": the scenario's `preset`
    }
    if (workerThreads > 0) {
        // independent spheres with the same height stack, set up for GetSurfaceHeight
        PQSHeightWorkers::Factory factory;
        if (s.at("kind") == "synth") {
            factory = [&s] {
                auto w = MakeSynthSphere(s, nullptr);
                w->SetupForHeight();
                return w;
            };
        } else {
            const std::string body = s.at("body").get<std::string>();
            factory = [body] { return terrain::LoadStockPQS(body, Maps()); };
        }
        sc.workers = std::make_unique<PQSHeightWorkers>(factory, workerThreads);
        sc.sphere->heightWorkers = sc.workers.get();
    }
    PQS& p = *sc.sphere;
    p.cache = &sc.cache;
    sc.clockStep = sp.at("clockStep").get<float>();
    if (sc.clockStep != 0.0f) {
        p.realtimeSinceStartup = [&sc] {
            ++sc.clockCalls;
            return static_cast<float>(sc.clockCalls) * sc.clockStep;
        };
    }
    p.SetTarget(&sc.target);
}

// tools/wp12/pqs_reference.py rows_text: fields joined by '|', rows by a line feed
std::string RowsText(const json& rows) {
    std::string t;
    for (size_t k = 0; k < rows.size(); k++) {
        if (k > 0) t += static_cast<char>(10);
        for (size_t i = 0; i < rows[k].size(); i++) {
            if (i > 0) t += '|';
            const json& v = rows[k][i];
            t += v.is_string() ? v.get<std::string>() : std::to_string(v.get<int64_t>());
        }
    }
    return t;
}

json SnapshotScene(const Scene& sc, bool real, const json& refSnapshot) {
    const PQS& p = *sc.sphere;
    std::set<std::string> withDigest;
    if (real)
        for (const json& n : refSnapshot.at("exact")) withDigest.insert(n.get<std::string>());
    json quads = json::array();
    for (const PQ* q : WalkSphere(p)) quads.push_back(SnapshotQuad(*q, q->isBuilt && (!real || withDigest.count(q->name) > 0)));
    int updates = p.updateSphereRoutineCount();
    json sphere = json::array({p.isActive ? 1 : 0, p.isAlive ? 1 : 0, p.isStarted ? 1 : 0, p.isThinking ? 1 : 0,
                               HexD(p.visibleRadius), HexD(p.horizonDistance), HexD(p.targetHeight), HexD(p.targetAltitude),
                               p.maxLevelAtCurrentTgtSpeed, p.quadCount, static_cast<int>(p.normalUpdateList.size()),
                               sc.cache.cachePQAssignedCount, sc.cache.cachePQUnassignedCount, sc.cache.cachePQTotalCount,
                               sc.cache.quadsMaterialised(), real ? std::string() : HexD(p.meshVertMin), real ? std::string() : HexD(p.meshVertMax),
                               updates, sc.cbt != nullptr ? HexF(sc.cbt->planetFade.a) : std::string()});
    return json{{"sphere", sphere}, {"quads", quads}};
}

const char* kQuadFields[] = {"id", "name", "subdivision", "quadIndex", "plane", "flags", "edgeState", "triangleSet", "north",
                             "south", "east", "west", "parent", "gcDist", "gcd1", "subdivideThresholdFactor", "positionPlanet.x",
                             "positionPlanet.y", "positionPlanet.z", "meshVertMin", "meshVertMax", "meshDigest"};
const char* kSphereFields[] = {"isActive", "isAlive", "isStarted", "isThinking", "visibleRadius", "horizonDistance",
                               "targetHeight", "targetAltitude", "maxLevelAtCurrentTgtSpeed", "quadCount",
                               "normalUpdateList", "poolAssigned", "poolUnassigned", "poolTotal", "poolMaterialised",
                               "meshVertMin", "meshVertMax", "updateSphereRoutines", "planetFade.a"};

struct Tally {
    int64_t quadRows = 0, values = 0, digests = 0, mismatches = 0;
    int64_t allRows = 0, allDigests = 0;  // every frame (through the rows digest)
};

void CompareSnapshot(const std::string& where, const json& mine, const json& ref, Tally& t) {
    const json& ms = mine.at("sphere");
    const json& rs = ref.at("sphere");
    for (size_t i = 0; i < rs.size(); i++) {
        if (rs[i].is_string() && rs[i].get<std::string>().empty()) continue;
        t.values++;
        if (ms[i] != rs[i]) {
            if (++t.mismatches <= 20) FAIL_CHECK(where << " sphere." << kSphereFields[i] << ": " << ms[i] << " != reference " << rs[i]);
        }
    }
    const json& mq = mine.at("quads");
    t.allRows += static_cast<int64_t>(mq.size());
    for (const json& r : mq) t.allDigests += r[21].get<std::string>().empty() ? 0 : 1;
    t.values++;
    if (Sha256Hex(RowsText(mq)).substr(0, 16) != ref.at("rows_digest").get<std::string>()) {
        if (++t.mismatches <= 20)
            FAIL_CHECK(where << " quad rows digest differs" << (ref.at("quads").is_null() ? " (rows not kept: rerun pqs_reference.py --full)" : ""));
    }
    if (ref.at("quads").is_null()) return;
    const json& rq = ref.at("quads");
    if (mq.size() != rq.size()) {
        if (++t.mismatches <= 20) FAIL_CHECK(where << " quad count " << mq.size() << " != reference " << rq.size());
    }
    const size_t n = std::min(mq.size(), rq.size());
    for (size_t k = 0; k < n; k++) {
        t.quadRows++;
        for (size_t i = 0; i < rq[k].size(); i++) {
            if (rq[k][i].is_string() && rq[k][i].get<std::string>().empty()) continue;
            t.values++;
            if (i == 21) t.digests++;
            if (mq[k][i] != rq[k][i]) {
                if (++t.mismatches <= 20)
                    FAIL_CHECK(where << " quad #" << k << " " << rq[k][1] << " ." << kQuadFields[i] << ": " << mq[k][i]
                                         << " != reference " << rq[k][i]);
            }
        }
    }
}

void ReplayScenario(const json& s, int workerThreads = 0) {
    GlobalStateGuard guard;
    Scene sc;
    BuildScene(sc, s, guard, workerThreads);
    const bool real = s.at("kind") == "real";
    Tally t;
    const json& frames = s.at("frames");
    const json& snaps = s.at("snapshots");
    REQUIRE(frames.size() == snaps.size());
    for (size_t f = 0; f < frames.size(); f++) {
        const json& fr = frames[f];
        const json& tg = fr.at("target");
        sc.target.position = Vector3d(tg[0].get<double>(), tg[1].get<double>(), tg[2].get<double>());
        for (int k = 0; k < fr.at("fixed").get<int>(); k++) sc.sphere->FixedUpdate();
        for (const json& a : fr.at("actions")) {
            const std::string act = a.get<std::string>();
            if (act == "start") {
                sc.sphere->Start();
            } else if (act == "activate") {
                sc.sphere->ActivateSphere();
            } else if (act == "rebuild") {
                sc.sphere->RebuildSphere();
            } else if (act == "frame") {
                sc.sphere->FrameUpdate();
            } else {
                FAIL("unknown action " << act);
            }
        }
        const json mine = SnapshotScene(sc, real, snaps[f]);
        CompareSnapshot(s.at("name").get<std::string>() + " frame " + std::to_string(f), mine, snaps[f], t);
    }
    // full dumps of the first sampled quads (redundant with the digests; better messages)
    std::map<std::string, const PQ*> byName;
    for (const PQ* q : WalkSphere(*sc.sphere)) byName[q->name] = q;
    int dumped = 0;
    for (const json& d : s.at("dumps")) {
        const PQ* q = byName.at(d.at("name").get<std::string>());
        const json& v = d.at("vertices");
        REQUIRE(v.size() == q->mesh.vertices.size());
        for (size_t i = 0; i < v.size(); i++) {
            CHECK(HexF(q->mesh.vertices[i].x) == v[i][0].get<std::string>());
            CHECK(HexF(q->mesh.vertices[i].y) == v[i][1].get<std::string>());
            CHECK(HexF(q->mesh.vertices[i].z) == v[i][2].get<std::string>());
            CHECK(HexF(q->mesh.normals[i].x) == d.at("normals")[i][0].get<std::string>());
            CHECK(HexD(q->vertsD[i].y) == d.at("vertsD")[i][1].get<std::string>());
        }
        dumped++;
    }
    CHECK(t.mismatches == 0);
    CHECK(t.digests > 0);
    if (sc.workers) {
        CHECK(sc.workers->used > 0);
        MESSAGE(s.at("name").get<std::string>() << " with " << sc.workers->threads() << " height workers: " << sc.workers->prefetched
                                                << " quads prefetched, " << sc.workers->used << " builds took them");
    }
    MESSAGE(s.at("name").get<std::string>() << ": " << frames.size() << " frames, " << t.allRows << " quad rows and "
                                            << t.allDigests << " mesh digests compared (" << t.quadRows
                                            << " rows field by field, the rest through each frame's rows digest), "
                                            << dumped << " full dumps, " << t.mismatches << " mismatches");
}

const json& ScenarioNamed(const std::string& name) {
    for (const json& s : Reference().at("scenarios"))
        if (s.at("name") == name) return s;
    FAIL("scenario " << name << " missing from wp12_reference.json");
    static json none;
    return none;
}

}  // namespace

TEST_CASE("WP-12a native Matrix4x4.TRS and Vector3.OrthoNormalize vs the C3 capture") {
    const json cap = json::parse(std::ifstream(kHarness + "/captures/out/math_shim_native.json"));
    const auto& tab = cap.at("tables").at(0);
    std::vector<std::string> cols;
    for (const auto& c : tab.at("columns")) cols.push_back(c.at("name").get<std::string>());
    auto col = [&](const std::string& n) { return static_cast<size_t>(std::find(cols.begin(), cols.end(), n) - cols.begin()); };
    auto hexf = [](const json& j) {
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

TEST_CASE("WP-12a PQS static mesh cache == reference (cacheVerts, UVs, UVQuad, 17 index sets, tangents)") {
    const json& c = Reference().at("cache");
    PQSMeshCache m;
    m.CreateCache();
    REQUIRE(m.cacheSideVertCount == c.at("side").get<int>());
    int n = 0;
    for (int i = 0; i < m.cacheVertCount; i++) {
        const size_t k = static_cast<size_t>(i);
        CHECK(HexF(m.cacheVerts[k].x) == c.at("cacheVerts")[k][0].get<std::string>());
        CHECK(HexF(m.cacheVerts[k].y) == c.at("cacheVerts")[k][1].get<std::string>());
        CHECK(HexF(m.cacheVerts[k].z) == c.at("cacheVerts")[k][2].get<std::string>());
        CHECK(HexF(m.cacheUVs[k].x) == c.at("cacheUVs")[k][0].get<std::string>());
        CHECK(HexF(m.cacheUVs[k].y) == c.at("cacheUVs")[k][1].get<std::string>());
        CHECK(HexD(m.cacheUVQuad[k].x) == c.at("cacheUVQuad")[k][0].get<std::string>());
        CHECK(HexD(m.cacheUVQuad[k].y) == c.at("cacheUVQuad")[k][1].get<std::string>());
        for (int j = 0; j < 3; j++) {
            CHECK(HexF(m.tan1[k][j]) == c.at("tan1")[k][static_cast<size_t>(j)].get<std::string>());
            CHECK(HexF(m.tan2[k][j]) == c.at("tan2")[k][static_cast<size_t>(j)].get<std::string>());
        }
        const Vector4f& t = m.cacheTangents[k];
        CHECK(HexF(t.x) == c.at("cacheTangents")[k][0].get<std::string>());
        CHECK(HexF(t.y) == c.at("cacheTangents")[k][1].get<std::string>());
        CHECK(HexF(t.z) == c.at("cacheTangents")[k][2].get<std::string>());
        CHECK(HexF(t.w) == c.at("cacheTangents")[k][3].get<std::string>());
        n++;
    }
    for (size_t s = 0; s < 17; s++) CHECK(m.cacheIndices[s] == c.at("cacheIndices")[s].get<std::vector<int>>());
    CHECK(n == 225);
}

TEST_CASE("WP-12a reference replay: synthetic Kerbin-sized sphere (custom normals, stitching, tangents, FlattenArea, coast, rebuild, clock)") {
    ReplayScenario(ScenarioNamed("synth_a"));
}

TEST_CASE("WP-12a reference replay: synthetic Mun-sized sphere (sphere normals, planet-centred vertices, quad UVs, map coords)") {
    ReplayScenario(ScenarioNamed("synth_b"));
}

TEST_CASE("WP-12a reference replay: stock Kerbin at the KSC pad (WP-11 heights through the oracle)") {
    ReplayScenario(ScenarioNamed("kerbin_ksc"));
}

TEST_CASE("WP-12a reference replay: stock Mun (WP-11 heights through the oracle)") {
    ReplayScenario(ScenarioNamed("mun"));
}

TEST_CASE("WP-12a PQSHeightWorkers: every reference scenario replayed with 4 height workers is bit-identical") {
    // the optimisation's gate: the same quad rows and mesh digests as the inline build and the Python reference
    for (const char* name : {"synth_a", "synth_b", "kerbin_ksc", "mun"}) ReplayScenario(ScenarioNamed(name), 4);
}

TEST_CASE("WP-12a PQSHeightWorkers: every built quad of stock Kerbin / Mun is bit-identical to the inline build") {
    // the reference digests cover the sampled stock quads only; here every quad: inline scene vs worker scene
    for (const char* name : {"kerbin_ksc", "mun"}) {
        const json& s = ScenarioNamed(name);
        GlobalStateGuard guard;
        std::map<std::string, std::string> inlineDigest;
        {
            Scene sc;
            BuildScene(sc, s, guard);
            const json& fr = s.at("frames");
            for (size_t f = 0; f < fr.size(); f++) {
                const json& tg = fr[f].at("target");
                sc.target.position = Vector3d(tg[0].get<double>(), tg[1].get<double>(), tg[2].get<double>());
                for (int k = 0; k < fr[f].at("fixed").get<int>(); k++) sc.sphere->FixedUpdate();
                for (const json& a : fr[f].at("actions")) {
                    if (a == "start") sc.sphere->Start();
                    if (a == "frame") sc.sphere->FrameUpdate();
                }
            }
            for (const PQ* q : WalkSphere(*sc.sphere))
                if (q->isBuilt) inlineDigest[q->name] = MeshDigest(*q);
        }
        Scene sc;
        BuildScene(sc, s, guard, 4);
        const json& fr = s.at("frames");
        for (size_t f = 0; f < fr.size(); f++) {
            const json& tg = fr[f].at("target");
            sc.target.position = Vector3d(tg[0].get<double>(), tg[1].get<double>(), tg[2].get<double>());
            for (int k = 0; k < fr[f].at("fixed").get<int>(); k++) sc.sphere->FixedUpdate();
            for (const json& a : fr[f].at("actions")) {
                if (a == "start") sc.sphere->Start();
                if (a == "frame") sc.sphere->FrameUpdate();
            }
        }
        int64_t built = 0, same = 0;
        for (const PQ* q : WalkSphere(*sc.sphere)) {
            if (!q->isBuilt) continue;
            built++;
            auto it = inlineDigest.find(q->name);
            if (it != inlineDigest.end() && it->second == MeshDigest(*q)) same++;
        }
        CHECK(built == static_cast<int64_t>(inlineDigest.size()));
        CHECK(same == built);
        MESSAGE(std::string(name) << ": " << built << " built quads, " << same << " identical to the inline build ("
                                  << sc.workers->used << " took prefetched heights)");
    }
}

namespace {

// Run the first `frames` frames of a reference scenario and keep the scene alive.
void RunFrames(Scene& sc, const json& s, int frames, GlobalStateGuard& g) {
    BuildScene(sc, s, g);
    const json& fr = s.at("frames");
    for (int f = 0; f < frames && f < static_cast<int>(fr.size()); f++) {
        const json& tg = fr[static_cast<size_t>(f)].at("target");
        sc.target.position = Vector3d(tg[0].get<double>(), tg[1].get<double>(), tg[2].get<double>());
        for (int k = 0; k < fr[static_cast<size_t>(f)].at("fixed").get<int>(); k++) sc.sphere->FixedUpdate();
        for (const json& a : fr[static_cast<size_t>(f)].at("actions")) {
            const std::string act = a.get<std::string>();
            if (act == "start") sc.sphere->Start();
            if (act == "activate") sc.sphere->ActivateSphere();
            if (act == "rebuild") sc.sphere->RebuildSphere();
            if (act == "frame") sc.sphere->FrameUpdate();
        }
    }
}

std::vector<std::array<double, 4>> CaptureSamples(const std::string& body) {
    static const json cap = json::parse(std::ifstream(kHarness + "/captures/out2/terrain_height.json"));
    std::vector<std::array<double, 4>> out;  // lat, lon, h_surface, h_override
    for (const json& t : cap.at("tables")) {
        if (t.at("name") != "samples") continue;
        for (const json& r : t.at("rows")) {
            if (r[0].get<std::string>() != body) continue;
            out.push_back({FromHexD(r[3]), FromHexD(r[5]), FromHexD(r[7]), FromHexD(r[9])});
        }
    }
    return out;
}

}  // namespace

TEST_CASE("WP-12a every built quad's vertices are the WP-11 heights at the vertex directions (stock Kerbin, Mun)") {
    for (const char* name : {"kerbin_ksc", "mun"}) {
        GlobalStateGuard guard;
        Scene sc;
        const json& s = ScenarioNamed(name);
        RunFrames(sc, s, 3, guard);
        // an independent stock sphere evaluates the build-state heights (PQS.cs:2249-2262) for every built quad
        auto builder = terrain::LoadStockPQS(s.at("body").get<std::string>(), Maps());
        const PQSMeshCache& mc = sc.cache.meshCache;
        int64_t quads = 0, verts = 0, bad = 0;
        for (const PQ* q : WalkSphere(*sc.sphere)) {
            if (!q->isBuilt) continue;
            PQ probe;  // the quad as the height mods see it in OnQuadPreBuild / OnQuadBuilt
            probe.positionPlanetRelative = q->positionPlanetRelative;
            probe.subdivision = q->subdivision;
            builder->Mod_OnQuadPreBuild(probe);
            for (int i = 0; i < mc.cacheVertCount; i++) {
                const Vector3d gv(MultiplyPoint3x4(q->quadMatrix, mc.cacheVerts[static_cast<size_t>(i)]));
                const double h = builder->GetSurfaceHeight(gv);
                const Vector3d expect = gv.normalized() * h;
                const Vector3d& got = q->vertsD[static_cast<size_t>(i)];
                if (Bits(got.x) != Bits(expect.x) || Bits(got.y) != Bits(expect.y) || Bits(got.z) != Bits(expect.z)) bad++;
                verts++;
            }
            builder->Mod_OnQuadBuilt(probe);
            quads++;
        }
        CHECK(quads > 100);
        CHECK(bad == 0);
        MESSAGE(std::string(name) << ": " << quads << " built quads, " << verts << " vertices == WP-11 GetSurfaceHeight in the build state, "
                     << bad << " differ");
    }
}

TEST_CASE("WP-12a the quad-building sphere still returns the in-game surface heights (terrain_height capture, 140,522 points)") {
    // The capture (FLIGHT, Kerbin's quads built, the Mun's never) is WP-11's gate. Here the SAME sphere that just built
    // hundreds of quads (mod scratch state, static build scratch, quad-dependent mod state after OnQuadBuilt) answers
    // GetSurfaceHeight for every capture point: Kerbin bit for bit; the Mun has now built quads, so its three
    // FlattenArea points take the built-state value, which the capture records as h_override (WP-11 quirk 4).
    for (const char* name : {"kerbin_ksc", "mun"}) {
        GlobalStateGuard guard;
        Scene sc;
        const json& s = ScenarioNamed(name);
        RunFrames(sc, s, 3, guard);
        const std::string body = s.at("body").get<std::string>();
        const auto samples = CaptureSamples(body);
        int64_t n = 0, bad = 0, builtState = 0;
        for (const auto& r : samples) {
            const Vector3d dir = terrain::GetRelSurfaceNVector(r[0], r[1]);
            const double h = sc.sphere->GetSurfaceHeight(dir);
            const double ho = sc.sphere->GetSurfaceHeight(dir, true);
            double want = r[2];
            if (Bits(r[2]) != Bits(r[3]) && body == "Mun") {
                want = r[3];
                builtState++;
            }
            if (Bits(h) != Bits(want) || Bits(ho) != Bits(r[3])) bad++;
            n++;
        }
        CHECK(bad == 0);
        CHECK(n > 60000);
        MESSAGE(body << ": " << n << " capture points bit-exact after " << sc.cache.quadsMaterialised() << " quads ("
                     << builtState << " points in the built-state value), " << bad << " differ");
    }
}

TEST_CASE("WP-12a PQSCache pool: Stack semantics, counters, fresh quads materialised on demand") {
    PQSCache c;
    c.cachePQInitialCount = 2;
    c.cachePQIncreasePerFrame = 3;
    c.CreateCache();
    CHECK(c.cacheReady);
    CHECK(c.cachePQTotalCount == 2);
    CHECK(c.quadsMaterialised() == 0);
    PQ* a = c.GetQuad();
    PQ* b = c.GetQuad();
    CHECK(c.cachePQUnassignedCount == 0);
    PQ* d = c.GetQuad();  // the stack is empty: IncreasePQCache(3)
    CHECK(c.cachePQTotalCount == 5);
    CHECK(c.cachePQAssignedCount == 3);
    CHECK(c.cachePQUnassignedCount == 2);
    a->isBuilt = true;
    a->edgeState = PQS::EdgeState::NorthLerp;
    a->vertNormals[7] = Vector3f(1.0f, 2.0f, 3.0f);
    c.DestroyQuad(*a);
    CHECK(!a->isBuilt);
    CHECK(a->subdivideThresholdFactor == 1.0);
    c.DestroyQuad(*b);
    CHECK(c.GetQuad() == b);  // LIFO
    PQ* again = c.GetQuad();
    CHECK(again == a);
    CHECK(again->edgeState == PQS::EdgeState::NorthLerp);  // not reset by DestroyQuad (PQSCache.cs:448)
    CHECK(again->vertNormals[7].Equals(Vector3f(1.0f, 2.0f, 3.0f)));
    CHECK(c.GetQuad() != d);
    CHECK(c.quadsMaterialised() == 4);
    CHECK(a->mesh.vertices.size() == 225);
    CHECK(a->mesh.triangleSet == 0);
    CHECK(c.meshCache.cacheIndices[0].size() == 1176);
}

TEST_CASE("WP-12a terrain presets: default list, ConfigNode round trip, GetPreset, SetPreset(string) quirk, cache settings") {
    PQSCache::CreateDefaultPresetList();
    REQUIRE(PQSCache::PresetList);
    CHECK(PQSCache::PresetList->preset == "Default");
    CHECK(PQSCache::PresetList->presetIndex == 1);
    const auto* mun = PQSCache::PresetList->GetPreset("Mun");
    REQUIRE(mun != nullptr);
    CHECK(mun->minDistance == 6.0);
    CHECK(mun->maxSubdivision == 8);
    CHECK(PQSCache::PresetList->GetPreset("Jool") == nullptr);
    ConfigNode settings("root");
    PQSCache::SavePresetList(settings);
    ConfigNode* terrain = settings.GetNode("TERRAIN");
    REQUIRE(terrain != nullptr);
    ConfigNode* pqs = terrain->AddNode("PQS");
    pqs->AddValue("vertCount", "17");
    pqs->AddValue("cacheMeshSize", "1");
    pqs->AddValue("cacheQuadSize", "3");
    PQSMeshCache mc;
    PQSCache::PresetList.reset();
    PQSCache::LoadPresetList(settings, &mc);
    REQUIRE(PQSCache::PresetList);
    CHECK(PQSCache::PresetList->presets.size() == 3);
    CHECK(PQSCache::PresetList->presets[2].spherePresets[0].maxSubdivision == 10);  // High / Kerbin
    CHECK(PQSCache::PresetList->GetPreset("Kerbin")->maxSubdivision == 9);
    CHECK(mc.cacheSideVertCount == 17);
    CHECK(mc.cacheQuadSizeDiv2 == 1.5f);
    // PQSCache.cs:64 compares each preset's name with the CURRENT preset and renames inside the loop: from Default,
    // "Low" (listed before it) keeps Default's index, "High" (after it) happens to match again and gets its own
    PQSCache::PresetList->SetPreset(std::string("Low"));
    CHECK(PQSCache::PresetList->preset == "Low");
    CHECK(PQSCache::PresetList->presetIndex == 1);
    PQSCache::PresetList->SetPreset(1);
    PQSCache::PresetList->SetPreset(std::string("High"));
    CHECK(PQSCache::PresetList->preset == "High");
    CHECK(PQSCache::PresetList->presetIndex == 2);
    PQSCache::PresetList->SetPreset(2);
    CHECK(PQSCache::PresetList->presetIndex == 2);
    // an incompatible version reverts to the defaults
    ConfigNode old("root");
    ConfigNode* t2 = old.AddNode("TERRAIN");
    t2->AddValue("preset", "Low");
    t2->AddValue("version", "1.0.0");
    t2->AddNode("PRESET")->AddValue("name", "Low");
    PQSCache::LoadPresetList(old, nullptr);
    CHECK(PQSCache::PresetList->preset == "Default");
    CHECK(PQSCache::PresetList->presets.size() == 3);
    PQSCache::PresetList.reset();
    // a 17-vertex cache builds
    mc.CreateCache();
    CHECK(mc.cacheVertCount == 289);
    CHECK(mc.cacheIndices[16].size() == static_cast<size_t>(16 * 16 * 6));
}

TEST_CASE("WP-12a QuadMeshColliders seam, CelestialBodyTransform fades, render handoff of the built quads") {
    GlobalStateGuard guard;
    Scene sc;
    const json& s = ScenarioNamed("synth_a");
    int enabled = 0, disabled = 0;
    BuildScene(sc, s, guard);
    sc.sphere->onQuadColliderEnabled = [&](PQ& q) {
        enabled++;
        CHECK(q.subdivision >= sc.sphere->maxLevel - 1);
    };
    sc.sphere->onQuadColliderDisabled = [&](PQ&) { disabled++; };
    const json& fr = s.at("frames");
    for (size_t f = 0; f < fr.size(); f++) {
        const json& tg = fr[f].at("target");
        sc.target.position = Vector3d(tg[0].get<double>(), tg[1].get<double>(), tg[2].get<double>());
        for (int k = 0; k < fr[f].at("fixed").get<int>(); k++) sc.sphere->FixedUpdate();
        for (const json& a : fr[f].at("actions")) {
            const std::string act = a.get<std::string>();
            if (act == "start") sc.sphere->Start();
            if (act == "rebuild") sc.sphere->RebuildSphere();
            if (act == "frame") sc.sphere->FrameUpdate();
        }
    }
    CHECK(enabled > 0);
    CHECK(disabled > 0);
    int drawn = 0, bad = 0;
    double worst = 0.0;
    for (const PQ* q : WalkSphere(*sc.sphere)) {
        if (q->meshColliderEnabled && !q->isBuilt) bad++;
        if (!(q->isBuilt && q->rendererEnabled)) continue;
        drawn++;
        // positions: meshOrigin (double) + float vertices = the double vertex to float precision
        for (size_t i = 0; i < q->vertsD.size(); i++) {
            const Vector3d p = q->meshOrigin + Vector3d(q->mesh.vertices[i]);
            worst = std::max({worst, std::fabs(p.x - q->vertsD[i].x), std::fabs(p.y - q->vertsD[i].y), std::fabs(p.z - q->vertsD[i].z)});
        }
        for (int idx : sc.cache.meshCache.cacheIndices[static_cast<size_t>(q->mesh.triangleSet)]) bad += idx < 225 ? 0 : 1;
        bad += q->mesh.normals.size() == 225 && q->mesh.tangents.size() == 225 && q->mesh.uv2.size() == 225 &&
                       q->mesh.uv4.size() == 225 && q->mesh.colors.size() == 225
                   ? 0
                   : 1;
    }
    CHECK(drawn > 50);
    CHECK(bad == 0);
    CHECK(worst <= 0.0625);  // half the float spacing of the 600 km planet-relative coordinates (KSP's own quantisation)
    MESSAGE("synth_a: " << drawn << " drawn quads, max |origin + vertex - double vertex| = " << worst << " m");
    // AltitudeFade.DoFade (PQSMod_CelestialBodyTransform.cs:35)
    PQSMod_CelestialBodyTransform::AltitudeFade f;
    f.fadeStart = 20000.0f;
    f.fadeEnd = 120000.0f;
    f.valueStart = 0.0f;
    f.valueEnd = 1.0f;
    int resets = 0;
    f.resetTerrainShaderOffset = [&] { resets++; };
    f.DoFade(10000.0);
    CHECK(f.a == 0.0f);
    f.DoFade(70000.0);
    CHECK(f.a == 0.5f);
    f.DoFade(130000.0);
    CHECK(f.a == 1.0f);
    CHECK(resets == 1);
    f.DoFade(true);
    CHECK(f.a == 0.0f);
}

TEST_CASE("WP-12a PQS frame helpers: LineSphereIntersection, edge rotation") {
    Vector3d hit;
    CHECK(PQS::LineSphereIntersection(Vector3d(0, 0, -10), Vector3d(0, 0, 1), 2.0, hit));
    CHECK(hit.z == -2.0);
    CHECK(!PQS::LineSphereIntersection(Vector3d(0, 5, -10), Vector3d(0, 0, 1), 2.0, hit));
    CHECK(PQS::LineSphereIntersection(Vector3d(0, 1, 0), Vector3d(1, 0, 0), 2.0, hit));  // inside: the start point
    CHECK(hit.y == 1.0);
    CHECK(PQS::GetEdgeRotatedCounterclockwise(PQS::QuadEdge::North) == PQS::QuadEdge::West);
    CHECK(PQS::GetEdgeRotatedClockwise(PQS::QuadEdge::North) == PQS::QuadEdge::East);
    CHECK(PQS::GetEdgeRotatedClockwise(PQS::QuadEdge::Null) == PQS::QuadEdge::Null);
}

// =====================================================================================================================
// WP-12b: terrain colliders. Gate: tests/data/wp12b_reference.json from tools/wp12/colliders_reference.py (independent Python:
// the activation decisions of PQSMod_QuadMeshColliders as the target moves, the collider geometry hash, the collider set per
// frame), analytic ray-triangle checks on a flat-height sphere (every quad of the patch), the measured agreement with WP-11
// GetSurfaceHeight on stock Kerbin and Mun, and the box dropped on the KSC pad.
// =====================================================================================================================

namespace {

const json& Reference12b() {
    static const json ref = json::parse(std::ifstream(kTestData + "/wp12b_reference.json"));
    return ref;
}

const json& Scenario12b(const std::string& name) {
    for (const json& s : Reference12b().at("scenarios"))
        if (s.at("name") == name) return s;
    FAIL("scenario " << name << " missing from wp12b_reference.json");
    static json none;
    return none;
}

std::string Hex64(uint64_t v) {
    char b[24];
    std::snprintf(b, sizeof b, "%016llx", static_cast<unsigned long long>(v));
    return b;
}

// One frame of a scenario (the same actions as ReplayScenario)
void RunFrame(Scene& sc, const json& fr) {
    const json& tg = fr.at("target");
    sc.target.position = Vector3d(tg[0].get<double>(), tg[1].get<double>(), tg[2].get<double>());
    for (int k = 0; k < fr.at("fixed").get<int>(); k++) sc.sphere->FixedUpdate();
    for (const json& a : fr.at("actions")) {
        const std::string act = a.get<std::string>();
        if (act == "start") {
            sc.sphere->Start();
        } else if (act == "activate") {
            sc.sphere->ActivateSphere();
        } else if (act == "rebuild") {
            sc.sphere->RebuildSphere();
        } else if (act == "frame") {
            sc.sphere->FrameUpdate();
        } else {
            FAIL("unknown action " << act);
        }
    }
}

std::vector<int> CollidersByFlags(const PQS& p) {
    std::vector<int> ids;
    for (const PQ* q : WalkSphere(p))
        if (q->meshColliderEnabled) ids.push_back(q->id);
    std::sort(ids.begin(), ids.end());
    return ids;
}

// Replays a collider scenario against the reference: per frame the ordered enable / disable events (seq, kind, quad id, name,
// subdivision, geometry hash where the reference has one) and the set of quads holding a collider in the PhysicsWorld.
void ReplayColliders(const json& s, int workerThreads, int cookBudget) {
    GlobalStateGuard guard;
    Scene sc;
    BuildScene(sc, s, guard, workerThreads);
    physics::PhysicsWorld world;
    TerrainCollidersOptions opt;
    opt.recordEvents = true;
    opt.maxCooksPerUpdate = cookBudget;
    TerrainColliders tc(world, *sc.sphere, opt);
    const json& frames = s.at("frames");
    const json& refs = s.at("colliders");
    REQUIRE(frames.size() == refs.size());
    int64_t events = 0, hashes = 0, mismatches = 0, setChecks = 0, flagChecks = 0;
    uint32_t nextSeq = 0;
    const std::string name = s.at("name").get<std::string>() + (workerThreads > 0 ? " (workers)" : "") + (cookBudget > 0 ? " (budget)" : "");
    for (size_t f = 0; f < frames.size(); f++) {
        RunFrame(sc, frames[f]);
        const json& rf = refs[f];
        const json& rev = rf.at("events");
        const std::vector<ColliderEvent>& mine = tc.Events();
        if (mine.size() != rev.size()) {
            if (++mismatches <= 20) FAIL_CHECK(name << " frame " << f << ": " << mine.size() << " events, reference " << rev.size());
        } else {
            for (size_t i = 0; i < mine.size(); i++) {
                const json& r = rev[i];
                const ColliderEvent& e = mine[i];
                events++;
                bool ok = r[0].get<uint32_t>() == e.seq && r[0].get<uint32_t>() == nextSeq && (r[1].get<std::string>() == "E") == e.enabled &&
                          r[2].get<int>() == e.quadId && r[3].get<std::string>() == e.name && r[4].get<int>() == e.subdivision;
                nextSeq++;
                if (!r[5].get<std::string>().empty()) {
                    hashes++;
                    ok = ok && r[5].get<std::string>() == Hex64(e.geometryHash);
                }
                if (!ok && ++mismatches <= 20) FAIL_CHECK(name << " frame " << f << " event #" << i << " " << r.dump() << " != quad " << e.quadId << " " << e.name);
            }
        }
        tc.ClearEvents();
        while (tc.PendingCount() > 0) tc.Update();
        setChecks++;
        if (tc.WorldQuadIds() != rf.at("ids").get<std::vector<int>>()) {
            if (++mismatches <= 20) FAIL_CHECK(name << " frame " << f << ": the world's collider set differs from the reference");
        }
        flagChecks++;
        if (tc.WorldQuadIds() != CollidersByFlags(*sc.sphere)) {
            if (++mismatches <= 20) FAIL_CHECK(name << " frame " << f << ": the world's collider set differs from the quads' meshColliderEnabled");
        }
    }
    CHECK(mismatches == 0);
    CHECK(events == s.at("event_count").get<int64_t>());
    MESSAGE(name << ": " << frames.size() << " frames, " << events << " enable / disable events (" << hashes << " geometry hashes), " << setChecks
                 << " collider sets vs the reference and " << flagChecks << " vs the quad flags, " << mismatches << " mismatches; " << world.StaticCount()
                 << " colliders at the end, " << tc.Stats().cooked << " cooked, " << tc.Stats().dropped << " dropped");
}

}  // namespace

TEST_CASE("WP-12b collider activation vs the independent reference: synth_a (maxLevelOffset 1, CBT deactivation, rebuild)") {
    ReplayColliders(Scenario12b("synth_a"), 0, 0);
}
TEST_CASE("WP-12b collider activation vs the independent reference: synth_b") { ReplayColliders(Scenario12b("synth_b"), 0, 0); }
TEST_CASE("WP-12b collider activation vs the independent reference: synth_c (ground walk, climb above the deactivation altitude, rebuild)") {
    ReplayColliders(Scenario12b("synth_c"), 0, 0);
}
TEST_CASE("WP-12b collider activation vs the independent reference: stock Kerbin at the KSC pad") {
    ReplayColliders(Scenario12b("kerbin_ksc"), 0, 0);
}
TEST_CASE("WP-12b collider activation vs the independent reference: stock Mun") { ReplayColliders(Scenario12b("mun"), 0, 0); }

TEST_CASE("WP-12b the cook budget and the height workers do not change the activation decisions") {
    // events are recorded at event time; with a budget of 7 cooks per Update the set is compared after draining the queue
    for (const char* n : {"synth_a", "synth_c"}) ReplayColliders(Scenario12b(n), 0, 7);
    for (const char* n : {"synth_a", "kerbin_ksc"}) ReplayColliders(Scenario12b(n), 4, 0);
}

namespace {

// ray - triangle (Moller-Trumbore) in double: distance t > 0 along d from the origin, or -1
double RayTriangle(const Vector3d& o, const Vector3d& d, const Vector3d& a, const Vector3d& b, const Vector3d& c, Vector3d& normal) {
    const Vector3d e1 = b - a, e2 = c - a;
    const Vector3d p = Vector3d::Cross(d, e2);
    const double det = Vector3d::Dot(e1, p);
    if (std::fabs(det) < 1e-300) return -1.0;
    const Vector3d tv = o - a;
    const double u = Vector3d::Dot(tv, p) / det;
    if (u < -1e-12 || u > 1.0 + 1e-12) return -1.0;
    const Vector3d q = Vector3d::Cross(tv, e1);
    const double v = Vector3d::Dot(d, q) / det;
    if (v < -1e-12 || u + v > 1.0 + 1e-12) return -1.0;
    const double t = Vector3d::Dot(e2, q) / det;
    normal = Vector3d::Cross(e1, e2).normalized();
    return t;
}

// the cell sample points of a quad: bilinear positions of the 4 corners at (u, v) fractions
const double kSamples[5][2] = {{0.45, 0.4}, {0.2, 0.3}, {0.8, 0.6}, {0.5, 0.1}, {0.1, 0.5}};

Vector3d CellPoint(const PQ& q, int i, int j, const double* uv) {
    const int s = 15;
    const Vector3d& a = q.vertsD[static_cast<size_t>(i * s + j)];
    const Vector3d& b = q.vertsD[static_cast<size_t>(i * s + j + 1)];
    const Vector3d& c = q.vertsD[static_cast<size_t>((i + 1) * s + j)];
    const Vector3d& d = q.vertsD[static_cast<size_t>((i + 1) * s + j + 1)];
    return (a * ((1 - uv[0]) * (1 - uv[1]))) + (b * (uv[0] * (1 - uv[1]))) + (c * ((1 - uv[0]) * uv[1])) + (d * (uv[0] * uv[1]));
}

}  // namespace

TEST_CASE("WP-12b sphere patch: radial ray cast and normals against ray-triangle intersection of the collider triangles (analytic)") {
    // A sphere whose height stack is a constant zero: every vertex lies exactly on the sphere of radius R, the collider
    // facets are chords. For every quad with a collider and 5 sample directions per cell the answer of the world's ray cast is
    // compared with the intersection of the same radial line with the quad's own triangles, in double, and with the sphere:
    // R - r is the chord deficit (0 <= R - r <= R (1 - cos e), e = the longest angular edge of the quad).
    const json spec = json::parse(R"({
      "name":"flat","kind":"synth",
      "sphere":{"name":"Flat","radius":600000.0,"surfaceRelativeQuads":true,"buildTangents":false,"preset":[6.0,1,9],"clockStep":0.0},
      "stack":[{"type":"SynthHeight","order":10,"req":0,"A":0.0,"B":0.0,"C":0.0,"D":0.0,"E":0.0,"K1":1.0,"K2":1.0,"F":0.0,"hmax":100.0},
               {"type":"QuadMeshColliders","order":70,"maxLevelOffset":0}],
      "frames":[{"target":[0,0,0],"fixed":0,"actions":["start","activate"]},{"target":[0,0,0],"fixed":1,"actions":["frame"]},{"target":[0,0,0],"fixed":1,"actions":["frame"]}]})");
    json s = spec;
    const double R = 600000.0;
    const Vector3d tgt = Vector3d(0.17, 0.98, 0.1).normalized() * (R + 300.0);
    for (json& f : s["frames"]) f["target"] = json::array({tgt.x, tgt.y, tgt.z});
    GlobalStateGuard guard;
    Scene sc;
    BuildScene(sc, s, guard);
    physics::PhysicsWorld world;
    TerrainCollidersOptions opt;
    TerrainColliders tc(world, *sc.sphere, opt);
    for (const json& f : s["frames"]) RunFrame(sc, f);
    REQUIRE(tc.CookedCount() >= 9);
    const std::vector<int>& idx = sc.cache.meshCache.cacheIndices[0];
    int64_t samples = 0, misses = 0, normalBad = 0, boundBad = 0;
    double worstDist = 0.0, worstAng = 0.0, worstDeficit = 0.0, worstRatio = 0.0;
    for (const PQ* q : WalkSphere(*sc.sphere)) {
        if (!q->meshColliderEnabled) continue;
        double eMax = 0.0;
        for (size_t t = 0; t + 2 < idx.size(); t += 3)
            for (int e = 0; e < 3; e++) {
                const Vector3d a = q->vertsD[static_cast<size_t>(idx[t + static_cast<size_t>(e)])].normalized();
                const Vector3d b = q->vertsD[static_cast<size_t>(idx[t + static_cast<size_t>((e + 1) % 3)])].normalized();
                eMax = std::max(eMax, std::acos(std::min(1.0, Vector3d::Dot(a, b))));
            }
        const double deficitBound = R * (1.0 - std::cos(eMax));
        for (int i = 0; i < 14; i++)
            for (int j = 0; j < 14; j++)
                for (const auto& uv : kSamples) {
                    const Vector3d dir = CellPoint(*q, i, j, uv).normalized();
                    // analytic: the nearest triangle of this quad along the radial line
                    double tBest = 1e300;
                    Vector3d nBest;
                    for (size_t t = 0; t + 2 < idx.size(); t += 3) {
                        Vector3d n;
                        const double tt = RayTriangle(Vector3d(), dir, q->vertsD[static_cast<size_t>(idx[t])], q->vertsD[static_cast<size_t>(idx[t + 1])],
                                                      q->vertsD[static_cast<size_t>(idx[t + 2])], n);
                        if (tt > 0.0 && tt < tBest) {
                            tBest = tt;
                            nBest = n;
                        }
                    }
                    if (tBest > 1e299) continue;
                    samples++;
                    const TerrainHit h = tc.SurfaceAt(dir * R, 300.0, 300.0);
                    if (!h.hit) {
                        misses++;
                        continue;
                    }
                    worstDist = std::max(worstDist, std::fabs(h.radius - tBest));
                    // the facet normal (the numeric cross of the Unity winding points outward)
                    const double ang = (h.normal.normalized() - nBest).magnitude();   // chord of the angle (acos of a dot near 1 amplifies float rounding)
                    worstAng = std::max(worstAng, ang);
                    if (ang > 1.0e-5) normalBad++;
                    const double deficit = R - h.radius;
                    worstDeficit = std::max(worstDeficit, deficit);
                    worstRatio = std::max(worstRatio, deficit / deficitBound);
                    if (deficit < -1.0e-3 || deficit > deficitBound + 1.0e-3) boundBad++;
                }
    }
    CHECK(samples > 9 * 14 * 14 * 4);
    CHECK(misses == 0);
    CHECK(worstDist < 1.0e-3);
    CHECK(normalBad == 0);
    CHECK(boundBad == 0);
    MESSAGE("flat sphere patch: " << tc.CookedCount() << " collider quads, " << samples << " radial rays, ray vs analytic triangle intersection max |dr| "
                                  << worstDist << " m, max normal angle error " << worstAng << " rad, max chord deficit R - r " << worstDeficit
                                  << " m (" << worstRatio * 100.0 << " % of the sagitta bound), " << misses << " misses");
}

namespace {

struct HeightStats {
    int64_t quads = 0, samples = 0, misses = 0, vertexMisses = 0, vertexSamples = 0;
    double maxAbs = 0.0, sumAbs = 0.0, p99 = 0.0, maxVertex = 0.0, maxSlopeAngle = 0.0, spacingMax = 0.0, spacingMin = 1e30;
    std::vector<double> errs;
    struct Worst {
        double err, relief, dist;
        int level;
        std::string quad;
    };
    std::vector<Worst> worst;
    double maxOverRelief = 0.0;
    // by distance of the sample from the target: < 200 m, < 500 m, < 1 km, < 3 km, everything
    static constexpr double kRing[5] = {200.0, 500.0, 1000.0, 3000.0, 1e30};
    int64_t ringN[5] = {0, 0, 0, 0, 0};
    double ringMax[5] = {0, 0, 0, 0, 0}, ringSum[5] = {0, 0, 0, 0, 0};
};

// Collider-vs-WP-11 agreement of every collider quad of a scenario's last frame: the radial ray through sample points of every
// cell against GetSurfaceHeight in the build state on an independent sphere, and through the vertices themselves.
HeightStats MeasureHeights(const char* scenario, int frames, double* terrainSlope = nullptr) {
    HeightStats st;
    GlobalStateGuard guard;
    Scene sc;
    const json& s = Scenario12b(scenario);
    BuildScene(sc, s, guard, 4);
    physics::PhysicsWorld world;
    TerrainCollidersOptions opt;
    TerrainColliders tc(world, *sc.sphere, opt);
    const json& fr = s.at("frames");
    for (int f = 0; f < frames && f < static_cast<int>(fr.size()); f++) RunFrame(sc, fr[static_cast<size_t>(f)]);
    auto builder = terrain::LoadStockPQS(s.at("body").get<std::string>(), Maps());
    const json& t0 = fr[0].at("target");
    const Vector3d pad(t0[0].get<double>(), t0[1].get<double>(), t0[2].get<double>());
    for (const PQ* q : WalkSphere(*sc.sphere)) {
        if (!q->meshColliderEnabled) continue;
        st.quads++;
        PQ probe;
        probe.positionPlanetRelative = q->positionPlanetRelative;
        probe.subdivision = q->subdivision;
        builder->Mod_OnQuadPreBuild(probe);
        {
            const Vector3d a = q->vertsD[0].normalized(), b = q->vertsD[1].normalized();
            const double sp = std::acos(std::min(1.0, Vector3d::Dot(a, b))) * sc.sphere->radius;
            st.spacingMax = std::max(st.spacingMax, sp);
            st.spacingMin = std::min(st.spacingMin, sp);
        }
        auto test = [&](const Vector3d& dir, bool vertex, double relief) {
            const double h = builder->GetSurfaceHeight(dir);
            const TerrainHit hit = tc.SurfaceAt(dir * h, 400.0, 400.0);
            if (!hit.hit) {
                (vertex ? st.vertexMisses : st.misses)++;
                return;
            }
            const double err = hit.radius - h;
            if (vertex) {
                st.vertexSamples++;
                st.maxVertex = std::max(st.maxVertex, std::fabs(err));
            } else {
                st.samples++;
                const double dist = (dir * h - pad).magnitude();
                st.worst.push_back({std::fabs(err), relief, dist, q->subdivision, q->name});
                if (relief > 1.0) st.maxOverRelief = std::max(st.maxOverRelief, std::fabs(err) / relief);
                for (int rg = 0; rg < 5; rg++)
                    if (dist < HeightStats::kRing[rg]) {
                        st.ringN[rg]++;
                        st.ringMax[rg] = std::max(st.ringMax[rg], std::fabs(err));
                        st.ringSum[rg] += std::fabs(err);
                    }
                st.maxAbs = std::max(st.maxAbs, std::fabs(err));
                st.sumAbs += std::fabs(err);
                st.errs.push_back(std::fabs(err));
            }
        };
        for (const Vector3d& v : q->vertsD) test(v.normalized(), true, 0.0);
        for (int i = 0; i < 14; i++)
            for (int j = 0; j < 14; j++) {
                double lo = 1e300, hi = -1e300;
                for (int di = 0; di < 2; di++)
                    for (int dj = 0; dj < 2; dj++) {
                        const double r = q->vertsD[static_cast<size_t>((i + di) * 15 + j + dj)].magnitude();
                        lo = std::min(lo, r);
                        hi = std::max(hi, r);
                    }
                for (const auto& uv : kSamples) test(CellPoint(*q, i, j, uv).normalized(), false, hi - lo);
            }
        builder->Mod_OnQuadBuilt(probe);
    }
    std::sort(st.errs.begin(), st.errs.end());
    if (!st.errs.empty()) st.p99 = st.errs[static_cast<size_t>(static_cast<double>(st.errs.size() - 1) * 0.99)];
    (void)terrainSlope;
    return st;
}

}  // namespace

TEST_CASE("WP-12b collider ray heights vs WP-11 GetSurfaceHeight: stock Kerbin (KSC) and Mun (measured bound)") {
    // The collider mesh interpolates the terrain linearly between the 15 x 15 vertices of a maximum-level quad (the vertices are
    // WP-11 heights bit for bit, WP-12a): at a vertex the ray must return the vertex height to float precision, between the
    // vertices the difference is the terrain's own curvature over one cell plus the chord of the sphere. Bound = measured.
    struct Case {
        const char* name;
        int frames;
        double bound;       // asserted maximum |ray height - GetSurfaceHeight| over the cell samples
        double vertexBound;
    };
    for (const Case c : {Case{"kerbin_ksc", 7, 0.0, 0.0}, Case{"mun", 4, 0.0, 0.0}}) {
        HeightStats st = MeasureHeights(c.name, c.frames);
        REQUIRE(st.quads > 0);
        MESSAGE(c.name << ": " << st.quads << " collider quads, vertex spacing " << st.spacingMin << " - " << st.spacingMax << " m, " << st.vertexSamples
                       << " vertex rays: max |dr| " << st.maxVertex << " m; " << st.samples << " cell samples: max |dr| " << st.maxAbs << " m, mean "
                       << (st.samples > 0 ? st.sumAbs / static_cast<double>(st.samples) : 0.0) << " m, p99 " << st.p99 << " m");
        std::sort(st.worst.begin(), st.worst.end(), [](const HeightStats::Worst& a, const HeightStats::Worst& b) { return a.err > b.err; });
        for (size_t k = 0; k < 6 && k < st.worst.size(); k++)
            MESSAGE(c.name << " worst #" << k << ": |dr| " << st.worst[k].err << " m, cell relief " << st.worst[k].relief << " m, " << st.worst[k].dist
                           << " m from the pad, level " << st.worst[k].level << " quad " << st.worst[k].quad);
        MESSAGE(c.name << " max |dr| / cell relief (cells with relief > 1 m): " << st.maxOverRelief);
        for (int rg = 0; rg < 5; rg++)
            MESSAGE(c.name << " samples within " << HeightStats::kRing[rg] << " m of the target: " << st.ringN[rg] << ", max |dr| " << st.ringMax[rg]
                           << " m, mean " << (st.ringN[rg] > 0 ? st.ringSum[rg] / static_cast<double>(st.ringN[rg]) : 0.0) << " m");
        CHECK(st.misses == 0);   // no cell sample falls through
        CHECK(st.vertexMisses * 200 < st.vertexSamples);   // rays through the outermost border vertices of the collider region
        MESSAGE(c.name << ": " << st.vertexMisses << " of " << st.vertexSamples + st.vertexMisses << " vertex rays and " << st.misses << " cell rays found no collider");
        CHECK(st.maxVertex < 2.0e-3);   // float vertices relative to a double centre + the float ray fraction
    }
}

// ---- the KSC pad: a box dropped from 10 m ---------------------------------------------------------------------------------------

namespace {

struct DropResult {
    double restCentreRadius = 0.0;   // |box centre| at the end
    double wp11Radius = 0.0;         // GetSurfaceHeight (build state) at the box's direction
    double terrainRayRadius = 0.0;   // the collider mesh under the box's direction
    double minCentreHeight = 1e30;   // over the run: |centre| - mesh radius below it (must stay >= half the box minus the slop)
    double endSpeed = 0.0;
    double impactSpeed = 0.0;
    uint64_t hash = 0, hashMid = 0;
    size_t statics = 0;
    int steps = 0;
};

// The box drops onto the quads of `sc` (Kerbin at the KSC pad, frames 0-2 of kerbin_ksc run). PhysicsWorld with `threads`.
DropResult DropBox(Scene& sc, int threads, bool updatePQS, physics::PhysicsWorld::RayHit* lastHit = nullptr) {
    using namespace physics;
    PhysicsWorldConfig cfg;
    cfg.threads = threads;
    PhysicsWorld world(cfg);
    TerrainCollidersOptions opt;
    TerrainColliders tc(world, *sc.sphere, opt);
    tc.AdoptExisting();
    DropResult r;
    r.statics = world.StaticCount();
    auto builder = terrain::LoadStockPQS("Kerbin", Maps());
    // the pad: frame 0 of kerbin_ksc is lat -0.0972, lon -74.5577 at 70 m
    const json& fr0 = Scenario12b("kerbin_ksc").at("frames")[0];
    const Vector3d padPos(fr0.at("target")[0].get<double>(), fr0.at("target")[1].get<double>(), fr0.at("target")[2].get<double>());
    const Vector3d up0 = padPos.normalized();
    // WP-11 height in the build state of the quad under the pad
    const TerrainHit under = tc.SurfaceAt(padPos, 300.0, 300.0);
    REQUIRE(under.hit);
    const PQ* quadUnder = nullptr;
    for (const PQ* q : WalkSphere(*sc.sphere))
        if (q->id == under.quadId) quadUnder = q;
    REQUIRE(quadUnder != nullptr);
    PQ probe;
    probe.positionPlanetRelative = quadUnder->positionPlanetRelative;
    probe.subdivision = quadUnder->subdivision;
    builder->Mod_OnQuadPreBuild(probe);
    const double h0 = builder->GetSurfaceHeight(up0);
    const Vector3d start = up0 * (h0 + 10.0 + 0.5);
    BodyDesc d;
    d.shape = ShapeDesc::Box(Vector3f(0.5f, 0.5f, 0.5f));
    d.position = start;
    d.mass = 100.0f;
    // align the box's y axis with the local vertical
    const Vector3d ax = Vector3d::Cross(Vector3d(0, 1, 0), up0);
    const double ang = std::acos(Vector3d::Dot(Vector3d(0, 1, 0), up0));
    d.rotation = QuaternionD::AngleAxis(ang * 57.29577951308232, ax.normalized());
    const BodyId box = world.CreateBody(d);
    REQUIRE(box != kInvalidBody);
    const double mu = 3.5316e12;
    const int steps = 150;   // 3 s at 50 Hz: the fall takes 1.43 s
    for (int i = 0; i < steps; i++) {
        const Vector3d p = world.GetPosition(box);
        const double rr = p.magnitude();
        world.AddForce(box, p * (-mu * 100.0 / (rr * rr * rr)));   // gravity as a force toward the planet centre (Physics.gravity = 0)
        world.Step(0.02f);
        if (updatePQS) {
            sc.target.position = world.GetPosition(box);
            sc.sphere->FixedUpdate();
            sc.sphere->FrameUpdate();
            tc.Update();
        }
        const Vector3d np = world.GetPosition(box);
        const TerrainHit th = tc.SurfaceAt(np, 50.0, 50.0);
        if (th.hit) r.minCentreHeight = std::min(r.minCentreHeight, np.magnitude() - th.radius);
        r.impactSpeed = std::max(r.impactSpeed, world.GetLinearVelocity(box).magnitude());
        if (i == 74) r.hashMid = world.StateHash();
    }
    r.steps = steps;
    const Vector3d p = world.GetPosition(box);
    r.restCentreRadius = p.magnitude();
    r.endSpeed = world.GetLinearVelocity(box).magnitude();
    r.hash = world.StateHash();
    const Vector3d dirEnd = p.normalized();
    r.wp11Radius = builder->GetSurfaceHeight(dirEnd);
    r.terrainRayRadius = tc.SurfaceAt(p, 50.0, 50.0).radius;
    if (lastHit != nullptr) *lastHit = world.RayCast(p, p * -1.0, 5.0, PhysicsWorld::kLayerStatic);
    return r;
}

}  // namespace

TEST_CASE("WP-12b KSC pad: a box dropped from 10 m rests at the WP-11 surface height, no tunnelling, deterministic at 1/2/4/8 threads") {
    GlobalStateGuard guard;
    Scene sc;
    const json& s = Scenario12b("kerbin_ksc");
    BuildScene(sc, s, guard, 4);
    for (size_t f = 0; f < 3; f++) RunFrame(sc, s.at("frames")[f]);   // start + two frames at the pad
    // the terrain under the pad is flattened (FlattenArea), the same sphere serves every world
    DropResult ref;
    for (int threads : {1, 2, 4, 8}) {
        const DropResult r = DropBox(sc, threads, false);
        if (threads == 1) {
            ref = r;
            // 0.5 = half the box. WP-11's surface height is the mesh-free truth under the box.
            const double centreAbove = r.restCentreRadius - r.wp11Radius;
            MESSAGE("pad drop, " << r.statics << " terrain colliders: rest centre " << centreAbove << " m above the WP-11 surface (box half 0.5), mesh under the box "
                                 << r.terrainRayRadius - r.wp11Radius << " m from WP-11, centre " << r.restCentreRadius - r.terrainRayRadius << " m above the mesh, end speed " << r.endSpeed << " m/s, max speed " << r.impactSpeed
                                 << " m/s, lowest centre height over the run " << r.minCentreHeight << " m");
            CHECK(r.statics >= 9);
            // the collider mesh interpolates the terrain linearly between vertices 154 m apart: under the box it is 0.32 m below the
            // WP-11 surface (the flattened pad still has sub-cell relief); within 200 m of the pad the measured bound is 1.51 m
            CHECK(std::fabs(r.terrainRayRadius - r.wp11Radius) < 1.51);
            CHECK(std::fabs(centreAbove - 0.5 + (r.wp11Radius - r.terrainRayRadius)) < 0.05);   // rests ON THE MESH (Jolt penetration slop 0.02 m)
            CHECK(std::fabs(centreAbove - 0.5) < 1.51 + 0.05);                                 // hence on the WP-11 surface within the stated bound
            CHECK(r.endSpeed < 0.05);
            CHECK(r.minCentreHeight > 0.4);                                   // never below the surface: no tunnelling
            CHECK(r.impactSpeed > 13.0);                                      // it really fell 10 m (sqrt(2 g 10) = 14.0 m/s)
            CHECK(r.impactSpeed < 15.0);
        } else {
            CHECK(r.hash == ref.hash);
            CHECK(r.hashMid == ref.hashMid);
            CHECK(r.restCentreRadius == ref.restCentreRadius);
        }
    }
    // run-to-run
    const DropResult again = DropBox(sc, 1, false);
    CHECK(again.hash == ref.hash);
}

TEST_CASE("WP-12b KSC pad: the same drop while the PQS keeps updating (target = the box) rests at the same height") {
    GlobalStateGuard guard;
    Scene sc;
    const json& s = Scenario12b("kerbin_ksc");
    BuildScene(sc, s, guard, 4);
    for (size_t f = 0; f < 3; f++) RunFrame(sc, s.at("frames")[f]);
    const DropResult still = DropBox(sc, 2, false);
    const DropResult live = DropBox(sc, 2, true);
    CHECK(std::fabs(live.restCentreRadius - still.restCentreRadius) < 0.02);
    CHECK(live.endSpeed < 0.05);
    CHECK(live.minCentreHeight > 0.4);
}
