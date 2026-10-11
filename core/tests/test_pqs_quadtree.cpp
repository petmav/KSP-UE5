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
#include "ksp/save/save_enums.hpp"
#include "ksp/terrain/pq.hpp"
#include "ksp/terrain/pqs_cache.hpp"
#include "ksp/terrain/pqs_mods.hpp"
#include "ksp/terrain/pqs_mods_quad.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
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
    std::unique_ptr<PQS> sphere;
    PQSTarget target;
    PQSMod_CelestialBodyTransform* cbt = nullptr;
    int clockCalls = 0;
    float clockStep = 0.0f;
};

void BuildScene(Scene& sc, const json& s, GlobalStateGuard&) {
    sc.cache.cachePQInitialCount = 64;
    const json& sp = s.at("sphere");
    if (s.at("kind") == "synth") {
        sc.sphere = std::make_unique<PQS>();
        PQS& p = *sc.sphere;
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
                FlightHost(*x, &sc.target);
                sc.cbt = x.get();
                mod = std::move(x);
            } else {
                FAIL("unknown mod " << t);
            }
            mod->className = "PQSMod_" + t;
            mod->order = m.at("order").get<int>();
            p.childMods.push_back(std::move(mod));
        }
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

void ReplayScenario(const json& s) {
    GlobalStateGuard guard;
    Scene sc;
    BuildScene(sc, s, guard);
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
