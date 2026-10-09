// WP-11 terrain unit tests: the stock prefab data equals data/terrain/pqs_prefab.json bit for bit, the extracted maps
// match their manifest (size, sha256, origin file present), the evaluated stack order equals the game's, KSC pad
// height, TerrainAltitude semantics, MapSO bilinear edge/wrap/decode cases, LibNoise values, native
// Quaternion.FromToRotation against the C3 capture, and every stock body evaluates.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <set>
#include <string>

#include "game_root.hpp"
#include "datasets.hpp"  // Sha256Hex (tools/golden_runner/sha256.cpp)
#include "doctest/doctest.h"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/terrain/libnoise.hpp"
#include "ksp/terrain/map_so.hpp"
#include "ksp/terrain/pqs_mods.hpp"
#include "ksp/terrain/pqs_prefab.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/simplex.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using ksp::terrain::MapLibrary;

namespace {

const std::string kData = KSPCORE_DATA_DIR;
const std::string kHarness = KSPCORE_HARNESS_DIR;
const std::string kRoot = KSPCORE_PROJECT_ROOT;

uint64_t Bits(double d) {
    uint64_t u;
    std::memcpy(&u, &d, 8);
    return u;
}
uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
double FromBits(uint64_t u) {
    double d;
    std::memcpy(&d, &u, 8);
    return d;
}
float FromBits32(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

nlohmann::json LoadJson(const std::string& path) {
    const auto text = dotnet::ReadAllText(path);
    REQUIRE_MESSAGE(text.has_value(), path);
    return nlohmann::json::parse(*text);
}

MapLibrary& Maps() {
    static MapLibrary maps(kData + "/terrain");
    return maps;
}

double JsonNumber(const nlohmann::json& j) {
    if (j.is_string()) {
        const std::string s = j.get<std::string>();
        if (s == "Infinity") return std::numeric_limits<double>::infinity();
        if (s == "-Infinity") return -std::numeric_limits<double>::infinity();
        if (s == "NaN") return std::numeric_limits<double>::quiet_NaN();
    }
    if (j.is_boolean()) return j.get<bool>() ? 1.0 : 0.0;
    return j.get<double>();
}

// navigate "a.b[3].c" in a JSON object
const nlohmann::json* JsonAt(const nlohmann::json& root, const std::string& path) {
    const nlohmann::json* cur = &root;
    size_t i = 0;
    while (i < path.size()) {
        if (path[i] == '.') {
            ++i;
            continue;
        }
        if (path[i] == '[') {
            const size_t e = path.find(']', i);
            const size_t idx = std::stoul(path.substr(i + 1, e - i - 1));
            if (!cur->is_array() || idx >= cur->size()) return nullptr;
            cur = &(*cur)[idx];
            i = e + 1;
            continue;
        }
        size_t e = path.find_first_of(".[", i);
        if (e == std::string::npos) e = path.size();
        const std::string key = path.substr(i, e - i);
        if (!cur->is_object() || !cur->contains(key)) return nullptr;
        cur = &(*cur)[key];
        i = e;
    }
    return cur;
}

bool SameNumber(double a, double b) { return Bits(a) == Bits(b) || (std::isnan(a) && std::isnan(b)); }

// compare one visited serialized value with its JSON counterpart; returns an error text or ""
std::string Compare(const SerializedValue& v, const nlohmann::json& j) {
    using K = SerializedValue::Kind;
    switch (v.kind) {
    case K::Double:
    case K::Int:
    case K::Bool:
        return SameNumber(v.number, JsonNumber(j)) ? "" : "number " + std::to_string(v.number) + " vs " + j.dump();
    case K::Float:
        return SameNumber(v.number, static_cast<double>(static_cast<float>(JsonNumber(j)))) && SameNumber(v.number, JsonNumber(j))
                   ? ""
                   : "float " + std::to_string(v.number) + " vs " + j.dump();
    case K::String:
        return v.text == (j.is_null() ? std::string() : j.get<std::string>()) ? "" : "string " + v.text + " vs " + j.dump();
    case K::Map:
        return v.text == (j.is_null() ? std::string() : j.at("map").get<std::string>()) ? "" : "map " + v.text + " vs " + j.dump();
    case K::Vector3:
        return (Bits(v.vec.x) == Bits(static_cast<float>(j.at("x").get<double>())) &&
                Bits(v.vec.y) == Bits(static_cast<float>(j.at("y").get<double>())) &&
                Bits(v.vec.z) == Bits(static_cast<float>(j.at("z").get<double>())))
                   ? ""
                   : "vector3 vs " + j.dump();
    case K::Color:
        return (Bits(v.color.r) == Bits(static_cast<float>(j.at("r").get<double>())) &&
                Bits(v.color.g) == Bits(static_cast<float>(j.at("g").get<double>())) &&
                Bits(v.color.b) == Bits(static_cast<float>(j.at("b").get<double>())) &&
                Bits(v.color.a) == Bits(static_cast<float>(j.at("a").get<double>())))
                   ? ""
                   : "color vs " + j.dump();
    case K::Curve: {
        const auto& keys = j.at("m_Curve");
        const AnimationCurve& c = *v.curve;
        if (static_cast<size_t>(c.length()) != keys.size()) return "curve length";
        if (c.NativePreInfinity() != j.value("m_PreInfinity", 2) || c.NativePostInfinity() != j.value("m_PostInfinity", 2))
            return "curve wrap";
        for (size_t i = 0; i < keys.size(); ++i) {
            const Keyframe& k = c[static_cast<int>(i)];
            const auto& jk = keys[i];
            auto f = [](const nlohmann::json& x) { return static_cast<float>(x.get<double>()); };
            if (Bits(k.time) != Bits(f(jk.at("time"))) || Bits(k.value) != Bits(f(jk.at("value"))) ||
                Bits(k.inTangent) != Bits(f(jk.at("inSlope"))) || Bits(k.outTangent) != Bits(f(jk.at("outSlope"))) ||
                static_cast<int>(k.weightedMode) != jk.value("weightedMode", 0) ||
                Bits(k.inWeight) != Bits(f(jk.at("inWeight"))) || Bits(k.outWeight) != Bits(f(jk.at("outWeight"))))
                return "curve key " + std::to_string(i);
        }
        return "";
    }
    }
    return "kind";
}

// Building a PQS constructs Simplex objects, and every Simplex(seed) reseeds UnityEngine.Random and rewrites the
// static permutation table shared by all Simplex instances (Simplex.cs:155, as in KSP). Restore that process-wide
// state after each terrain test so later test cases (WP-03's Simplex tests) see the state they expect.
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

}  // namespace

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: stock PQS prefab data equals data/terrain/pqs_prefab.json bit for bit") {
    const nlohmann::json root = LoadJson(kData + "/terrain/pqs_prefab.json");
    size_t compared = 0, bodies = 0;
    for (const auto& b : root.at("bodies")) {
        if (!b.contains("pqs")) continue;
        const std::string name = b.at("bodyName").get<std::string>();
        const terrain::StockPQSInfo* info = terrain::FindStockPQS(name);
        REQUIRE_MESSAGE(info != nullptr, name);
        CHECK(info->pqsPathID == b.at("pqs").at("pathID").get<int64_t>());
        CHECK(SameNumber(info->pqsRadius, b.at("pqs").at("radius").get<double>()));
        CHECK(SameNumber(info->bodyRadius, b.at("celestialBody").at("Radius").get<double>()));
        CHECK(info->ocean == (b.at("celestialBody").at("ocean").get<int>() != 0));
        auto pqs = terrain::CreateStockPQS(name, Maps());
        const auto& mods = b.at("mods");
        REQUIRE(pqs->childMods.size() == mods.size());
        CHECK(info->modCount == static_cast<int>(mods.size()));
        for (size_t i = 0; i < mods.size(); ++i) {
            const auto& jm = mods[i];
            const PQSMod& m = *pqs->childMods[i];
            CHECK(m.className == jm.at("class").get<std::string>());
            CHECK(m.gameObjectPath == jm.at("gameObject").get<std::string>());
            CHECK(m.gameObjectActiveSelf == jm.at("activeSelf").get<bool>());
            CHECK(m.AffectsHeight() == jm.at("height_mod").get<bool>());
            m.VisitSerialized([&](const std::string& path, const SerializedValue& v) {
                // PQSROCControl.Awake rewrites modExpansionDisabled (no Serenity DLC): not the serialized value
                if (m.className == "PQSROCControl" && path == "modExpansionDisabled") return;
                const nlohmann::json* j = JsonAt(jm.at("fields"), path);
                INFO(name << " [" << i << "] " << m.className << " " << path);
                REQUIRE(j != nullptr);
                CHECK(Compare(v, *j) == "");
                ++compared;
            });
        }
        ++bodies;
    }
    CHECK(bodies == 15);
    CHECK(compared > 3600);
    MESSAGE("prefab values compared: " << compared);
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: extracted maps match the manifest (size, sha256, origin)") {
    const nlohmann::json manifest = LoadJson(kData + "/terrain/manifest.json");
    REQUIRE(manifest.at("maps").size() == terrain::StockMaps().size());
    for (const auto& e : manifest.at("maps")) {
        const std::string key = e.at("key").get<std::string>();
        const terrain::StockMapInfo* info = nullptr;
        for (const auto& m : terrain::StockMaps())
            if (key == m.key) info = &m;
        REQUIRE_MESSAGE(info != nullptr, key);
        CHECK(std::string(info->sha256) == e.at("sha256").get<std::string>());
        CHECK(info->bytes == static_cast<int64_t>(info->width) * info->height * info->bpp);
        const auto bytes = dotnet::ReadAllBytes(kData + "/terrain/" + e.at("bin").get<std::string>());
        REQUIRE(bytes.has_value());
        CHECK(static_cast<int64_t>(bytes->size()) == info->bytes);
        CHECK(Sha256Hex(*bytes) == info->sha256);
        CHECK(std::filesystem::exists(ksp::testing::GameRoot() + "/" + info->file));  // mirror: zero-byte placeholder
        const auto map = Maps().Get(key);
        CHECK(map->Width() == info->width);
        CHECK(map->GetOrigin().pathID == info->pathID);
    }
    CHECK_THROWS(Maps().Get("no_such_map.1"));
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: evaluated stack order equals the game's (capture mods table)") {
    const nlohmann::json cap = LoadJson(kHarness + "/captures/out2/terrain_height.json");
    const nlohmann::json* mods = nullptr;
    for (const auto& t : cap.at("tables"))
        if (t.at("name") == "mods") mods = &t;
    REQUIRE(mods != nullptr);
    std::map<std::string, std::vector<std::string>> all, height;
    for (const auto& r : mods->at("rows")) {
        // body, idx, class, gameObject, order, modEnabled, requirements, height_mod
        const std::string id = r[2].get<std::string>() + "/" + r[3].get<std::string>() + "/" + std::to_string(r[4].get<int>());
        all[r[0].get<std::string>()].push_back(id);
        if (r[7].get<bool>()) height[r[0].get<std::string>()].push_back(id);
    }
    for (const char* bn : {"Kerbin", "Mun"}) {
        auto pqs = terrain::LoadStockPQS(bn, Maps());
        std::vector<std::string> mine, mineHeight;
        for (PQSMod* m : pqs->mods) {
            const std::string go = m->gameObjectPath.substr(m->gameObjectPath.rfind('/') + 1);
            const std::string id = m->className + "/" + go + "/" + std::to_string(m->order);
            mine.push_back(id);
            if (m->AffectsHeight()) mineHeight.push_back(id);
        }
        std::string mh, gh;
        for (const auto& x : mineHeight) mh += x + " ";
        for (const auto& x : height[bn]) gh += x + " ";
        INFO(std::string(bn) << " kspcore: " << mh << " | game: " << gh);
        if (std::string(bn) == "Mun") {
            CHECK(mh == gh);
        } else {
            // Kerbin: List.Sort ran on a different list in the game (9 runtime PQSCity2 more), so equal-order mods
            // may be permuted; the height mods must agree as multisets per `order` value (checked here) and the
            // permuted ones must commute (checked by the next test case).
            std::map<std::string, std::multiset<std::string>> a, b;
            for (const auto& x : mineHeight) a[x.substr(x.rfind('/') + 1)].insert(x);
            for (const auto& x : height[bn]) b[x.substr(x.rfind('/') + 1)].insert(x);
            CHECK(a == b);
            CHECK(mineHeight.size() == height[bn].size());
        }
        // The Mun's runtime list is the prefab list: the whole evaluated stack (23 mods) matches, incl. List.Sort's
        // permutation of the order-100 mods. Kerbin's runtime list holds 9 more PQSCity2 (spawned set dressing).
        if (std::string(bn) == "Mun") CHECK(mine == all[bn]);
        if (std::string(bn) == "Kerbin") CHECK(mine.size() + 9 == all[bn].size());
    }
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: Kerbin's equal-order height mods commute (KSC, KSC2, Pyramids and their antipodes)") {
    // The game evaluates order 99999 as MapDecal "KSC2", MapDecalTangent "KSC", FlattenArea "Pyramids"; kspcore's
    // List.Sort over the prefab list yields another permutation. Evaluate both orders around each mod's centre and
    // its antipode (MapDecal* have no hemisphere test: outside quad building the decal also lands on the antipode).
    auto pqs = terrain::LoadStockPQS("Kerbin", Maps());
    std::vector<PQSMod*> group;
    std::vector<size_t> slots;
    for (size_t i = 0; i < pqs->mods.size(); ++i)
        if (pqs->mods[i]->order == 99999) {
            group.push_back(pqs->mods[i]);
            slots.push_back(i);
        }
    REQUIRE(group.size() == 3);
    auto find = [&](const char* cls, const char* go) {
        for (PQSMod* m : group)
            if (m->className == cls && m->gameObjectPath == std::string("Kerbin/") + go) return m;
        return static_cast<PQSMod*>(nullptr);
    };
    const std::vector<PQSMod*> gameOrder = {find("PQSMod_MapDecal", "KSC2"), find("PQSMod_MapDecalTangent", "KSC"),
                                            find("PQSMod_FlattenArea", "Pyramids")};
    std::vector<PQSMod*> mine = pqs->mods, game = pqs->mods;
    for (size_t k = 0; k < 3; ++k) game[slots[k]] = gameOrder[k];
    std::vector<Vector3d> centres;
    for (PQSMod* m : gameOrder) {
        REQUIRE(m != nullptr);
        Vector3f p = m->className == "PQSMod_FlattenArea" ? static_cast<PQSMod_FlattenArea*>(m)->position
                                                          : static_cast<PQSMod_MapDecalBase*>(m)->position;
        const Vector3d n = Vector3d(p.normalized());
        centres.push_back(n);
        centres.push_back(Vector3d(-n.x, -n.y, -n.z));
    }
    int evaluated = 0, changed = 0;
    for (const Vector3d& c : centres) {
        // a 41 x 41 patch of +-0.02 rad around the centre (decal radii are <= 7.5 km = 0.0125 rad)
        const Vector3d t1 = Vector3d::Normalize(Vector3d::Cross(c, std::fabs(c.y) < 0.9 ? Vector3d::up() : Vector3d::right()));
        const Vector3d t2 = Vector3d::Cross(c, t1);
        for (int i = -20; i <= 20; ++i)
            for (int j = -20; j <= 20; ++j) {
                const Vector3d d = c + t1 * (i * 0.001) + t2 * (j * 0.001);
                pqs->mods = mine;
                const double hm = pqs->GetSurfaceHeight(d, true);
                pqs->mods = game;
                const double hg = pqs->GetSurfaceHeight(d, true);
                CHECK(Bits(hm) == Bits(hg));
                ++evaluated;
            }
    }
    pqs->mods = mine;
    CHECK(evaluated == 6 * 41 * 41);
    (void)changed;
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: MapDecal applies at the antipode of its centre outside quad building (KSP quirk)") {
    auto with = terrain::LoadStockPQS("Kerbin", Maps());
    auto without = terrain::CreateStockPQS("Kerbin", Maps());
    for (auto& m : without->childMods)
        if (m->className == "PQSMod_MapDecalTangent" && m->gameObjectPath == "Kerbin/KSC") m->modEnabled = false;
    without->SetupForHeight();
    // KSC decal centre (157000, -3000, -570000) and its antipode
    const Vector3d n = Vector3d(Vector3f(157000.0f, -3000.0f, -570000.0f).normalized());
    const Vector3d anti(-n.x, -n.y, -n.z);
    CHECK(with->GetSurfaceHeight(n, true) != without->GetSurfaceHeight(n, true));
    CHECK(with->GetSurfaceHeight(anti, true) != without->GetSurfaceHeight(anti, true));
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: PQS radii and the KSC pad height (capture: 64.7847 m)") {
    auto kerbin = terrain::LoadStockPQS("Kerbin", Maps());
    CHECK(kerbin->radius == 600000.0);
    CHECK(kerbin->radiusMin == 598015.0);  // 600000 + (-1500) + (-485)
    CHECK(kerbin->radiusMax == 608735.0);  // 600000 + 3500 + 750 + 485 + 4000
    CHECK(kerbin->radiusDelta == 10720.0);
    CHECK(kerbin->reqVertexMapCoods);
    // the active vessel on the pad (harness/captures/out2/terrain_height.json provenance)
    const double alt = terrain::TerrainAltitude(kerbin.get(), -0.09720779031430374, -74.55767983607595, true);
    CHECK(Bits(alt) == Bits(64.78470540174749));
    CHECK(alt == doctest::Approx(64.7847).epsilon(1e-6));
    // scene_probe h_ksc (MAINMENU and FLIGHT): 0x4122500191C4E878
    CHECK(Bits(kerbin->GetSurfaceHeight(terrain::GetRelSurfaceNVector(-0.0972, -74.5577))) == 0x4122500191C4E878ull);
    auto mun = terrain::LoadStockPQS("Mun", Maps());
    CHECK(mun->radiusMin == 199600.0);
    CHECK(mun->radiusMax == 208300.0);
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: TerrainAltitude semantics (allowNegative clamp, no PQS, override variant)") {
    auto kerbin = terrain::LoadStockPQS("Kerbin", Maps());
    // (0, 0) is ocean floor: h_override_0_0 = 599153.2792472228 in the capture
    const double neg = terrain::TerrainAltitude(kerbin.get(), 0.0, 0.0, true);
    CHECK(neg < 0.0);
    CHECK(terrain::TerrainAltitude(kerbin.get(), 0.0, 0.0, false) == 0.0);
    CHECK(terrain::TerrainAltitude(kerbin.get(), 0.0, 0.0) == 0.0);
    CHECK(terrain::TerrainAltitude(nullptr, 10.0, 20.0, true) == 0.0);
    CHECK(Bits(terrain::SurfaceHeightOverride(*kerbin, 0.0, 0.0, 600000.0) + 600000.0) == Bits(599153.2792472228));
    CHECK(terrain::FindStockPQS("Kerbin")->ocean);
    CHECK_FALSE(terrain::FindStockPQS("Mun")->ocean);
    CHECK(terrain::FindStockPQS("Jool") == nullptr);
    CHECK(terrain::FindStockPQS("Sun") == nullptr);
    CHECK_THROWS_AS(terrain::CreateStockPQS("Jool", Maps()), std::invalid_argument);
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: the Mun's FlattenArea mods depend on quads unless overrideQuadBuildCheck") {
    auto mun = terrain::LoadStockPQS("Mun", Maps());
    // capture: Mun (-82, 102) h_surface 205384.67548674063 (no quads built), h_override 205312.53309413718
    const Vector3d d = terrain::GetRelSurfaceNVector(-82.0, 102.0);
    CHECK(Bits(mun->GetSurfaceHeight(d)) == Bits(205384.67548674063));
    CHECK(Bits(mun->GetSurfaceHeight(d, true)) == Bits(205312.53309413718));
    mun->Mod_OnQuadBuilt(PQ{});  // after any quad build FlattenArea.quadActive is true
    CHECK(Bits(mun->GetSurfaceHeight(d)) == Bits(205312.53309413718));
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: every stock body evaluates finite heights") {
    for (const auto& info : terrain::StockPQSBodies()) {
        auto pqs = terrain::LoadStockPQS(info.bodyName, Maps());
        INFO(info.bodyName);
        for (double lat = -80.0; lat <= 80.0; lat += 40.0)
            for (double lon = -170.0; lon <= 170.0; lon += 85.0) {
                const double h = pqs->GetSurfaceHeight(terrain::GetRelSurfaceNVector(lat, lon), true);
                CHECK(std::isfinite(h));
                CHECK(h > pqs->radius - 50000.0);
                CHECK(h < pqs->radius + 50000.0);
            }
    }
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: MapSO decode and bilinear wrap/edge cases (hand-computed, Mono float rule)") {
    MapSO g;
    g.CreateMap(MapSO::MapDepth::Greyscale, "grey", 4, 2);
    const uint8_t px[8] = {0, 255, 51, 102, 10, 20, 30, 40};
    for (int i = 0; i < 8; ++i) g.MutableData()[static_cast<size_t>(i)] = px[i];
    CHECK(Bits(MapSO::Byte2Float) == 0x3B808081u);  // 0.003921569f, not 1/255
    CHECK(g.GreyFloat(1, 0) == 1.0f);               // 255 * 0.003921569f rounds to exactly 1.0f
    // u = 1.0 wraps to texel 0 (|x - floor(x)| = 0); exact integer coordinates keep maxX == minX (Ceiling)
    auto c = g.ConstructBilinearCoords(1.0, 0.0);
    CHECK(c.minX == 0);
    CHECK(c.maxX == 0);
    CHECK(c.midX == 0.0f);
    c = g.ConstructBilinearCoords(-0.25, 0.5);  // |-0.25 - (-1)| = 0.75 -> centerX 3.0
    CHECK(c.minX == 3);
    CHECK(c.maxX == 3);
    CHECK(c.minY == 1);
    CHECK(c.maxY == 1);
    c = g.ConstructBilinearCoords(0.99, 0.75);  // centerX 3.96: maxX 4 == width -> 0; centerY 1.5: maxY 2 -> 0
    CHECK(c.minX == 3);
    CHECK(c.maxX == 0);
    CHECK(c.minY == 1);
    CHECK(c.maxY == 0);
    CHECK(Bits(c.midX) == 0x3F75C28Fu);
    // bilinear values (computed by an independent float32 emulation of MapSO.cs:696 / :668)
    CHECK(Bits(g.GetPixelFloat(0.125, 0.25)) == 0x3E8F0F0Fu);
    CHECK(Bits(g.GetPixelFloat(0.99, 0.75)) == 0x3CF57055u);           // double overload
    CHECK(Bits(g.GetPixelFloat(0.99f, 0.75f)) == 0x3CF5704Eu);         // float overload rounds centerX/midX in float
    CHECK(Bits(g.ConstructBilinearCoords(0.99f, 0.75f).midX) == 0x3F75C290u);
    CHECK(g.GetPixelFloat(0.125, 0.25) == g.GetPixelFloat(1.125, -0.75));  // periodic in u and v
    // GetPixelByte(int, int) "wrap": x < 0 -> Width - x (sic): (-1, 0) reads index 5 = row 1, x 1
    CHECK(g.GetPixelByte(-1, 0) == 20);
    CHECK(g.GetPixelByte(4, 1) == 10);  // x >= Width -> x - Width
    CHECK_THROWS_AS(g.GetPixelByte(-1, 1), std::out_of_range);

    // 2 bytes per pixel = (height, alpha); HeightAlpha.Lerp has no clamp
    MapSO ha;
    ha.CreateMap(MapSO::MapDepth::HeightAlpha, "ha", 2, 2);
    const uint8_t hb[8] = {100, 255, 200, 0, 50, 128, 0, 64};
    for (int i = 0; i < 8; ++i) ha.MutableData()[static_cast<size_t>(i)] = hb[i];
    const MapSO::HeightAlpha p11 = ha.GetPixelHeightAlpha(1, 1);
    CHECK(p11.height == 0.0f);
    CHECK(Bits(p11.alpha) == Bits(static_cast<float>(static_cast<double>(MapSO::Byte2Float) * 64.0)));
    const MapSO::HeightAlpha b = ha.GetPixelHeightAlpha(0.25f, 0.25f);
    CHECK(Bits(b.height) == 0x3EAFAFB1u);
    CHECK(Bits(b.alpha) == 0x3EE06061u);
    // GetPixelFloat on a wider map averages the channels: (30 + 60 + 91) / 3 * Byte2Float
    MapSO rgb;
    rgb.CreateMap(MapSO::MapDepth::RGB, "rgb", 1, 1);
    rgb.MutableData() = {30, 60, 91};
    CHECK(Bits(rgb.GetPixelFloat(0, 0)) == 0x3E72479Eu);
    // 3 bytes per pixel in GetPixelHeightAlpha: (channel 0, 1.0)
    CHECK(rgb.GetPixelHeightAlpha(0, 0).alpha == 1.0f);
    CHECK_THROWS_AS(MapSO::FromSerialized("bad", 2, 2, 1, 2, std::vector<uint8_t>(3)), std::invalid_argument);
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: LibNoise values (independent Python transcription of the C#, tools/wp11/libnoise_reference.py)") {
    using namespace ksp::LibNoise;
    // These expectations were computed by tools/wp11/libnoise_reference.py, a separate line-by-line Python reading of
    // Assembly-CSharp/LibNoise/*.cs (IEEE doubles in source order); the first two are also checked by hand:
    // IntValueNoise(0,0,0,0) = (0 * ... + 1376312589) & 0x7FFFFFFF; lattice points give 0 (offsets 0).
    ValueNoiseBasis vn;
    CHECK(vn.IntValueNoise(0, 0, 0, 0) == 1376312589);
    CHECK(vn.IntValueNoise(1, 2, 3, 4) == 1991767149);
    CHECK(vn.IntValueNoise(-7, 100000, -3, 1384047773) == 228499681);
    CHECK(Bits(vn.ValueNoise(5, -6, 7)) == 0x3FEA43BC6B800000ull);
    CHECK(vn.ValueNoise(0, 0, 0) == 1.0 - 1376312589.0 / 1073741824.0);
    GradientNoiseBasis gn;
    CHECK(Bits(gn.GradientCoherentNoise(0.5, 0.25, 0.75, 0, NoiseQuality::Low)) == 0x3FD6ACD3C3EA839Full);
    CHECK(Bits(gn.GradientCoherentNoise(-1.3, 2.7, 0.1, 42, NoiseQuality::Standard)) == 0xBFA30B5A41886B36ull);
    CHECK(Bits(gn.GradientCoherentNoise(10.6, -3.2, 7.9, -5, NoiseQuality::High)) == 0xBFCDFE1E88EA8CA2ull);
    CHECK(gn.GradientCoherentNoise(1.0, 2.0, 3.0, 7, NoiseQuality::Standard) == 0.0);
    CHECK(gn.GradientCoherentNoise(0.0, 0.0, 0.0, 7, NoiseQuality::Standard) == 0.0);  // (int)0 - 1 quirk, weight 1
    CHECK(GradientNoiseBasis::RandomVectors()[0] == -0.763874);
    CHECK(GradientNoiseBasis::RandomVectors()[1023] == 0.0);
    Perlin perlin(2.0, 2.0, 0.5, 6, 123, NoiseQuality::Standard);
    CHECK(Bits(perlin.GetValue(0.3, -0.4, 0.5)) == 0xBFBD64A788B3A17Bull);
    Billow billow(2.0, 2.0, 0.5, 4, 7, NoiseQuality::Low);
    CHECK(Bits(billow.GetValue(0.3, -0.4, 0.5)) == 0xBFD7500443B0C420ull);
    RidgedMultifractal ridged(48.0, 2.0, 6, 2147483646, NoiseQuality::Low);  // seed + i wraps, & 0x7FFFFFFF
    CHECK(Bits(ridged.GetValue(Vector3d(0.6, 0.0, -0.8))) == 0xBFC470FA0368CEACull);
    CHECK(ridged.SpectralWeights()[3] == 0.125);
    Voronoi vor(22.0, 0.0, 824, true);
    CHECK(Bits(vor.GetValue(0.6, 0.0, -0.8)) == 0x3FB88DBF2C03A360ull);
    Voronoi vor2(3.0, 1.0, 5, false);
    CHECK(Bits(vor2.GetValue(0.1, 0.2, 0.3)) == 0x3FEE144B6A800000ull);
    // determinism: same inputs, same bits; octave count range check
    CHECK(Bits(perlin.GetValue(0.3, -0.4, 0.5)) == Bits(Perlin(2.0, 2.0, 0.5, 6, 123, NoiseQuality::Standard).GetValue(0.3, -0.4, 0.5)));
    CHECK_THROWS_AS(Perlin(1.0, 2.0, 0.5, 31, 0, NoiseQuality::Low), ksp::ArgumentException);
    CHECK_THROWS_AS(RidgedMultifractal(1.0, 2.0, 0, 0, NoiseQuality::Low), ksp::ArgumentException);
    // Select(lower, upper, falloff, a, b): the falloff is clamped before the bounds are set (0 - 0) and there is no
    // control module until one is assigned
    auto s = std::make_shared<Modifiers::Select>(0.0, 1.0, 0.25, std::make_shared<Perlin>(), std::make_shared<Billow>());
    CHECK(s->EdgeFalloff() == 0.0);
    CHECK_THROWS_AS(s->GetValue(0.1, 0.2, 0.3), ksp::NullReferenceException);
    Modifiers::ScaleBiasOutput sb(2.0, 0.5, std::make_shared<Perlin>(2.0, 2.0, 0.5, 6, 123, NoiseQuality::Standard));
    CHECK(sb.GetValue(0.3, -0.4, 0.5) == perlin.GetValue(0.3, -0.4, 0.5) * 2.0 + 0.5);
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: native Quaternion.FromToRotation equals the C3 capture (49 rows)") {
    const nlohmann::json cap = LoadJson(kHarness + "/captures/out/math_shim_native.json");
    const auto& t = cap.at("tables")[0];
    std::vector<std::string> cols;
    for (const auto& c : t.at("columns")) cols.push_back(c.at("name").get<std::string>());
    auto col = [&](const std::string& n) { return static_cast<size_t>(std::find(cols.begin(), cols.end(), n) - cols.begin()); };
    auto hx = [](const nlohmann::json& j) { return static_cast<uint32_t>(std::stoul(j.get<std::string>(), nullptr, 16)); };
    int rows = 0;
    for (const auto& r : t.at("rows")) {
        if (r[0] != "Quaternion.FromToRotation") continue;
        const Vector3f from(FromBits32(hx(r[col("a0_hex")])), FromBits32(hx(r[col("a1_hex")])), FromBits32(hx(r[col("a2_hex")])));
        const Vector3f to(FromBits32(hx(r[col("a3_hex")])), FromBits32(hx(r[col("a4_hex")])), FromBits32(hx(r[col("a5_hex")])));
        const Quaternionf q = UnityNative::FromToRotation(from, to);
        INFO("row " << rows);
        CHECK(Bits(q.x) == hx(r[col("r0_hex")]));
        CHECK(Bits(q.y) == hx(r[col("r1_hex")]));
        CHECK(Bits(q.z) == hx(r[col("r2_hex")]));
        CHECK(Bits(q.w) == hx(r[col("r3_hex")]));
        ++rows;
    }
    CHECK(rows == 49);
}

TEST_CASE_FIXTURE(GlobalStateGuard, "terrain: quad-dependent mod state and texture-sampled mods") {
    // FlattenAreaTangential has no override check: inactive until a quad was built (sic)
    PQS pqs;
    pqs.radius = 1000.0;
    auto fat = std::make_unique<PQSMod_FlattenAreaTangential>();
    fat->className = "PQSMod_FlattenAreaTangential";
    fat->outerRadius = 200.0;
    fat->innerRadius = 100.0;
    fat->position = Vector3f(0.0f, 1.0f, 0.0f);
    fat->flattenTo = 50.0;
    auto* fatp = fat.get();
    pqs.childMods.push_back(std::move(fat));
    pqs.SetupForHeight();
    CHECK(pqs.GetSurfaceHeight(Vector3d(0.0, 1.0, 0.0), true) == 1000.0);
    pqs.Mod_OnQuadBuilt(PQ{});
    CHECK(fatp->quadActive);
    CHECK(pqs.GetSurfaceHeight(Vector3d(0.0, 1.0, 0.0)) == 1050.0);  // flattenToRadius / cos(0)
    // VertexHeightMapStep samples a Texture2D natively: without a sampler it throws instead of guessing
    PQS p2;
    p2.radius = 10.0;
    auto step = std::make_unique<PQSMod_VertexHeightMapStep>();
    auto* stepp = step.get();
    p2.childMods.push_back(std::move(step));
    p2.SetupForHeight();
    CHECK_THROWS_AS(p2.GetSurfaceHeight(Vector3d(1.0, 0.0, 0.0)), std::logic_error);
    stepp->heightMapSampler = [](float, float) { return 0.5; };
    stepp->heightMapDeformity = 4.0;
    stepp->OnSetup();
    CHECK(p2.GetSurfaceHeight(Vector3d(1.0, 0.0, 0.0)) == 12.0);
}
