// WP-mucol unit tests: the .mu collider reader, the part-space collider shapes, their conversion to physics proxies and the scenery
// collider loader.
//
// Gate: tools/mucollider/mu_colliders_reference.py (an independent Python parser written from PartReader.cs; it shares no code with
// the C++ reader) parses the ORIGINAL stock engine .mu files and writes tests/data/mucollider_reference.json: every collider chunk with
// its raw float bit patterns, and the part-space shape computed in float32 per operation. The tests compare bit for bit:
//   * the reduced real models (tests/data/mucollider/<part>.mu = the originals minus everything but the transform tree, TagAndLayer and
//     collider chunks; 19 stock engine parts, 31 colliders, .mu versions 1 to 5) -> chunk values, then part-space shapes;
//   * a synthetic coverage model (every collider chunk layout, triggers, the legacy capsule height, wheel marker, 4-deep rotated and
//     non-uniformly scaled chain, every chunk type the reader must skip) under three PartLoader.CompileModel placements;
//   * when the original files are staged (core/data/mu_staging, local only) the same checks on the full originals.
// Asset origin of the reduced models: GameData/Squad/Parts/Engine/**/*.mu (paths in the reference JSON, with sha256 of the originals).
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

#include "doctest/doctest.h"
#include "ksp/parts/mu_model.hpp"
#include "ksp/parts/part_colliders.hpp"
#include "ksp/physics/mu_part_colliders.hpp"
#include "ksp/physics/physics_world.hpp"
#include "ksp/space_center/scenery_colliders.hpp"
#include "nlohmann/json.hpp"
#include "vessel_physics_fixture.hpp"

using namespace ksp;
using nlohmann::json;

namespace {

const std::string kData = std::string(KSPCORE_TEST_DATA_DIR) + "/";

const json& Reference() {
    static const json ref = [] {
        std::ifstream f(kData + "mucollider_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return ref;
}

std::vector<uint8_t> ReadAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    REQUIRE_MESSAGE(f.good(), path);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
uint32_t HexBits(const json& j) { return static_cast<uint32_t>(std::stoul(j.get<std::string>(), nullptr, 16)); }
uint64_t FnvOf(const void* data, size_t n) {
    uint64_t h = 0xcbf29ce484222325ull;
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < n; ++i) h = (h ^ p[i]) * 0x100000001b3ull;
    return h;
}
uint64_t HexBits64(const json& j) { return std::stoull(j.get<std::string>(), nullptr, 16); }

size_t g_compared = 0;  // number of float / value comparisons made (reported in the test log)

#define CHECK_BITS(label, value, hexjson)                                  \
    do {                                                                   \
        ++g_compared;                                                      \
        CHECK_MESSAGE(Bits(value) == HexBits(hexjson), label << " got " << (value)); \
    } while (0)

void CheckVec(const char* label, const Vector3f& v, const json& j) {
    CHECK_BITS(label, v.x, j[0]);
    CHECK_BITS(label, v.y, j[1]);
    CHECK_BITS(label, v.z, j[2]);
}

const char* EntryName(mu::EntryType e) {
    switch (e) {
        case mu::EntryType::MeshCollider: return "MeshCollider";
        case mu::EntryType::SphereCollider: return "SphereCollider";
        case mu::EntryType::CapsuleCollider: return "CapsuleCollider";
        case mu::EntryType::BoxCollider: return "BoxCollider";
        case mu::EntryType::MeshCollider2: return "MeshCollider2";
        case mu::EntryType::SphereCollider2: return "SphereCollider2";
        case mu::EntryType::CapsuleCollider2: return "CapsuleCollider2";
        case mu::EntryType::BoxCollider2: return "BoxCollider2";
        case mu::EntryType::WheelCollider: return "WheelCollider";
        default: return "?";
    }
}

void Flatten(const mu::Node& n, std::vector<std::pair<const mu::Node*, const mu::ColliderChunk*>>& out) {
    for (const mu::ColliderChunk& c : n.colliders) out.emplace_back(&n, &c);
    for (const mu::Node& ch : n.children) Flatten(ch, out);
}

// the raw chunk values of every collider (pre-order) against the reference list
void CheckRaw(const mu::Model& model, const json& cols) {
    std::vector<std::pair<const mu::Node*, const mu::ColliderChunk*>> flat;
    Flatten(model.root, flat);
    REQUIRE(flat.size() == cols.size());
    for (size_t i = 0; i < flat.size(); ++i) {
        const mu::Node& n = *flat[i].first;
        const mu::ColliderChunk& c = *flat[i].second;
        const json& r = cols[i];
        INFO("collider " << i << " " << r["path"].get<std::string>());
        CHECK(std::string(EntryName(c.entry)) == r["entry"].get<std::string>());
        CHECK(c.trigger == r["trigger"].get<bool>());
        if (!r["tag"].is_null()) {
            CHECK(n.hasTagAndLayer);
            CHECK(n.tag == r["tag"].get<std::string>());
            CHECK(n.layer == r["layer"].get<int>());
        }
        if (c.kind == mu::ColliderKind::Wheel) continue;
        const json& p = r["params"];
        switch (c.kind) {
            case mu::ColliderKind::Mesh: {
                CHECK(c.convex);
                CHECK(c.convexFlagInFile == p["convexFlag"].get<bool>());
                CHECK(c.mesh.vertices.size() == r["mesh"]["count"].get<size_t>());
                CHECK(FnvOf(c.mesh.vertices.data(), c.mesh.vertices.size() * 12) == HexBits64(r["mesh"]["vertsFnv1a64"]));
                const json& tl = r["mesh"]["triangleLists"];
                REQUIRE(c.mesh.triangleLists.size() == tl.size());
                for (size_t t = 0; t < tl.size(); ++t) {
                    CHECK(c.mesh.triangleLists[t].size() == tl[t]["count"].get<size_t>());
                    CHECK(FnvOf(c.mesh.triangleLists[t].data(), c.mesh.triangleLists[t].size() * 4) == HexBits64(tl[t]["fnv1a64"]));
                }
                ++g_compared;
                break;
            }
            case mu::ColliderKind::Sphere:
                CHECK_BITS("radius", c.radius, p["radius"][0]);
                CheckVec("center", c.center, p["center"]);
                break;
            case mu::ColliderKind::Capsule:
                CHECK_BITS("radius", c.radius, p["radius"][0]);
                CHECK_BITS("height", c.height, p["height"][0]);
                CHECK(c.direction == p["direction"].get<int>());
                CheckVec("center", c.center, p["center"]);
                break;
            case mu::ColliderKind::Box:
                CheckVec("size", c.size, p["size"]);
                CheckVec("center", c.center, p["center"]);
                break;
            default:
                break;
        }
    }
}

// the part-space shapes against the reference (wheel colliders are not shapes)
void CheckShapes(const std::vector<mu::PartColliderShape>& shapes, const json& cols) {
    std::vector<const json*> want;
    for (const json& r : cols)
        if (r["isShape"].get<bool>()) want.push_back(&r);
    REQUIRE(shapes.size() == want.size());
    for (size_t i = 0; i < shapes.size(); ++i) {
        const mu::PartColliderShape& s = shapes[i];
        const json& r = *want[i];
        const json& ps = r["partSpace"];
        INFO("shape " << i << " " << r["path"].get<std::string>());
        CHECK(s.path == r["path"].get<std::string>());
        CHECK(std::string(EntryName(s.entry)) == r["entry"].get<std::string>());
        CHECK(s.trigger == r["trigger"].get<bool>());
        CheckVec("nodePosition", s.nodePosition, ps["nodePosition"]);
        CHECK_BITS("nodeRotation.x", s.nodeRotation.x, ps["nodeRotation"][0]);
        CHECK_BITS("nodeRotation.y", s.nodeRotation.y, ps["nodeRotation"][1]);
        CHECK_BITS("nodeRotation.z", s.nodeRotation.z, ps["nodeRotation"][2]);
        CHECK_BITS("nodeRotation.w", s.nodeRotation.w, ps["nodeRotation"][3]);
        CheckVec("nodeScale", s.nodeScale, ps["nodeScale"]);
        if (ps.contains("center")) CheckVec("center", s.center, ps["center"]);
        switch (s.kind) {
            case mu::ColliderKind::Box:
                CheckVec("halfExtents", s.halfExtents, ps["halfExtents"]);
                break;
            case mu::ColliderKind::Sphere:
                CHECK_BITS("radius", s.radius, ps["radius"]);
                break;
            case mu::ColliderKind::Capsule:
                CHECK_BITS("radius", s.radius, ps["radius"]);
                CHECK_BITS("height", s.height, ps["height"]);
                CHECK_BITS("halfCylinder", s.halfCylinder, ps["halfCylinder"]);
                CHECK(s.axis == ps["axis"].get<int>());
                break;
            case mu::ColliderKind::Mesh:
                CHECK(FnvOf(s.vertices.data(), s.vertices.size() * 12) == HexBits64(ps["vertexFnv1a64"]));
                ++g_compared;
                break;
            default:
                break;
        }
    }
}

Vector3f ParseVec3(const std::string& text) {
    std::string t = text;
    for (char& c : t)
        if (c == ',') c = ' ';
    std::istringstream is(t);
    std::string a, b, c;
    is >> a >> b >> c;
    return Vector3f(std::strtof(a.c_str(), nullptr), std::strtof(b.c_str(), nullptr), std::strtof(c.c_str(), nullptr));
}

mu::ModelInstance InstanceOf(const mu::Model* model, const json& m, bool fromModelNode) {
    mu::ModelInstance mi;
    mi.model = model;
    mi.fromModelNode = fromModelNode;
    if (!fromModelNode) return mi;
    auto vec = [](const json& v) {
        if (v.is_string()) return ParseVec3(v.get<std::string>());
        return Vector3f(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
    };
    if (m.contains("scale")) mi.scale = vec(m["scale"]);
    if (m.contains("position")) mi.position = vec(m["position"]);
    if (m.contains("parent")) mi.parent = m["parent"].get<std::string>();
    if (m.contains("name")) mi.name = m["name"].get<std::string>();
    return mi;
}

float FloatOfHex(const json& j) {
    const uint32_t u = HexBits(j);
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

void CheckPart(const std::string& name, const json& part, const mu::Model& model) {
    INFO("part " << name);
    CHECK(model.version == part["muVersion"].get<int>());
    CheckRaw(model, part["colliders"]);
    const float rescale = FloatOfHex(part["rescaleFactor"]);
    const bool fromNode = part["modelNodes"].get<int>() > 0;
    const json empty = json::object();
    const mu::ModelInstance mi = InstanceOf(&model, part["model"].is_null() ? empty : part["model"], fromNode);
    CheckShapes(mu::BuildPartColliders(rescale, {mi}), part["colliders"]);
}

// ---- tiny .mu writer for the malformed-input cases ----
struct Bytes {
    std::string b;
    void i32(int32_t v) { b.append(reinterpret_cast<const char*>(&v), 4); }
    void f(float v) { b.append(reinterpret_cast<const char*>(&v), 4); }
    void s(const std::string& v) {
        b.push_back(static_cast<char>(v.size()));
        b += v;
    }
};
std::string Header(Bytes& w, int32_t type, int32_t version) {
    w.i32(type);
    w.i32(version);
    w.s("x");
    w.s("root");
    for (int i = 0; i < 3; ++i) w.f(0.0f);
    w.f(0.0f); w.f(0.0f); w.f(0.0f); w.f(1.0f);
    for (int i = 0; i < 3; ++i) w.f(1.0f);
    return w.b;
}

}  // namespace

TEST_CASE("WP-mucol .mu collider chunks and part-space shapes of the 19 stock engine models equal the independent python reference bit for bit") {
    const json& ref = Reference();
    const json& parts = ref["parts"];
    REQUIRE(parts.size() == 19);
    g_compared = 0;
    size_t colliders = 0;
    std::set<std::string> entries;
    std::set<int> versions;
    for (auto it = parts.begin(); it != parts.end(); ++it) {
        const json& part = it.value();
        const std::string file = kData + part["reduced"]["file"].get<std::string>();
        const mu::Model model = mu::ReadModelFile(file);
        CheckPart(it.key(), part, model);
        colliders += part["colliders"].size();
        versions.insert(model.version);
        for (const json& c : part["colliders"]) entries.insert(c["entry"].get<std::string>());
    }
    // sanity of the corpus itself: 31 colliders, .mu versions 1..5, mesh / box / capsule chunks, no trigger among the engines
    CHECK(colliders == 31);
    CHECK(versions == std::set<int>({1, 3, 4, 5}));
    CHECK(entries == std::set<std::string>({"MeshCollider2", "BoxCollider2", "CapsuleCollider2"}));
    MESSAGE("stock engines: " << parts.size() << " parts, " << colliders << " colliders, " << g_compared << " value comparisons, 0 mismatches");
}

TEST_CASE("WP-mucol synthetic coverage model: every collider chunk layout, chain composition and PartLoader.CompileModel placement") {
    const json& synth = Reference()["synthetic"];
    const std::vector<uint8_t> bytes = ReadAll(kData + synth["file"].get<std::string>());
    CHECK(bytes.size() == synth["bytes"].get<size_t>());
    const mu::Model model = mu::ReadModel(bytes.data(), bytes.size());
    CHECK(model.version == 5);
    // 11 colliders: root 3, Arm 3 (one trigger, one wheel marker), Hand 4, Fingertip 2, Side 2 -> 14
    CHECK(model.TotalColliders() == 14);
    const json& cases = synth["cases"];
    REQUIRE(cases.size() == 3);
    // raw chunk values (single-instance case 0 has the file's own pre-order)
    CheckRaw(model, cases[0]["colliders"]);
    g_compared = 0;
    for (const json& cs : cases) {
        INFO("case " << cs["name"].get<std::string>());
        const float rescale = FloatOfHex(cs["rescaleFactor"]);
        const json& models = cs["models"];
        std::vector<mu::ModelInstance> inst;
        if (models.empty()) {
            inst.push_back(InstanceOf(&model, json::object(), false));
        } else {
            for (const json& m : models) inst.push_back(InstanceOf(&model, m, true));
        }
        CheckShapes(mu::BuildPartColliders(rescale, inst), cs["colliders"]);
    }
    MESSAGE("synthetic: 3 placements, " << g_compared << " value comparisons");
}

TEST_CASE("WP-mucol defaults and rules the chunks do not carry (PartReader.cs:233 forced convex, :247 capsule height 2, trigger flag, wheel marker)") {
    const json& synth = Reference()["synthetic"];
    const std::vector<uint8_t> bytes = ReadAll(kData + synth["file"].get<std::string>());
    const mu::Model model = mu::ReadModel(bytes.data(), bytes.size());
    std::vector<std::pair<const mu::Node*, const mu::ColliderChunk*>> flat;
    Flatten(model.root, flat);
    int legacyCapsules = 0, triggers = 0, wheels = 0, forcedConvex = 0;
    for (auto& [n, c] : flat) {
        if (c->entry == mu::EntryType::CapsuleCollider) {
            ++legacyCapsules;
            CHECK(c->height == 2.0f);  // no height field in the legacy chunk: Unity's default
        }
        triggers += c->trigger ? 1 : 0;
        wheels += c->kind == mu::ColliderKind::Wheel ? 1 : 0;
        if (c->kind == mu::ColliderKind::Mesh) {
            CHECK(c->convex);
            forcedConvex += c->convexFlagInFile ? 0 : 1;
        }
    }
    CHECK(legacyCapsules == 1);
    CHECK(triggers == 2);
    CHECK(wheels == 1);
    CHECK(forcedConvex == 2);  // a MeshCollider (legacy) and a MeshCollider2 stored with convex = false are convex anyway
    // triggers and wheels are not rigid-body shapes
    const std::vector<mu::PartColliderShape> shapes = mu::BuildPartColliders(1.0f, {mu::ModelInstance{&model, false, "", std::nullopt, std::nullopt, std::nullopt, std::nullopt}});
    CHECK(shapes.size() == 13);  // 14 chunks minus the wheel marker; triggers stay in the list, flagged
    const std::vector<physics::ColliderProxy> proxies = physics::ProxiesFromShapes(shapes);
    CHECK(proxies.size() == 11);  // minus the two triggers
}

TEST_CASE("WP-mucol malformed files throw (PartReader catches and logs; no shipped file is malformed)") {
    Bytes a;
    Header(a, 12345, 4);
    CHECK_THROWS_AS(mu::ReadModel(reinterpret_cast<const uint8_t*>(a.b.data()), a.b.size()), std::runtime_error);  // PartReader.cs:155
    const std::vector<uint8_t> good = ReadAll(kData + "mucollider/JetEngine.mu");
    for (size_t cut : {size_t(3), good.size() / 3, good.size() / 2, good.size() - 3}) {
        INFO("truncated to " << cut);
        CHECK_THROWS_AS(mu::ReadModel(good.data(), cut), std::runtime_error);
    }
    // a Textures chunk whose count differs from the texture dummies the materials created: PartReader.cs:388 TextureError
    Bytes t;
    Header(t, mu::kFileTypeModelBinary, 5);
    t.i32(static_cast<int32_t>(mu::EntryType::Textures));
    t.i32(1);
    t.s("tex.dds");
    t.i32(0);
    CHECK_THROWS_AS(mu::ReadModel(reinterpret_cast<const uint8_t*>(t.b.data()), t.b.size()), std::runtime_error);
    // a mesh chunk not starting with MeshStart: "Mesh Error"
    Bytes m;
    Header(m, mu::kFileTypeModelBinary, 5);
    m.i32(static_cast<int32_t>(mu::EntryType::MeshFilter));
    m.i32(99);
    CHECK_THROWS_AS(mu::ReadModel(reinterpret_cast<const uint8_t*>(m.b.data()), m.b.size()), std::runtime_error);
    // a file with only the root transform is a valid model without colliders
    Bytes e;
    Header(e, mu::kFileTypeModelBinary, 5);
    const mu::Model empty = mu::ReadModel(reinterpret_cast<const uint8_t*>(e.b.data()), e.b.size());
    CHECK(empty.TotalColliders() == 0);
    CHECK(empty.root.name == "root");
}

TEST_CASE("WP-mucol the original (unreduced) stock engine files, when staged locally, give the same colliders as the reduced ones") {
    const std::string staging = std::string(KSPCORE_DATA_DIR) + "/mu_staging";
    if (!std::filesystem::exists(staging + "/MANIFEST.json")) {
        MESSAGE("core/data/mu_staging is not present (local only: tools/wp17/stage_engine_models.py): skipped, the reduced files cover the same colliders");
        return;
    }
    const json& parts = Reference()["parts"];
    size_t files = 0;
    for (auto it = parts.begin(); it != parts.end(); ++it) {
        const json& part = it.value();
        const std::string path = staging + "/" + it.key() + "/" + part["mu"]["file"].get<std::string>();
        if (!std::filesystem::exists(path)) continue;
        const mu::Model model = mu::ReadModelFile(path);  // the full grammar: renderers, materials, animations, particles ...
        CheckPart(it.key(), part, model);
        ++files;
    }
    CHECK(files == 19);
}

TEST_CASE("WP-mucol mass properties of the collider solids: cube, capsule (numeric), and the real engine hulls vs the python reference") {
    using physics::ColliderProxy;
    // unit cube [-1, 1]^3 as a closed mesh: V = 8, I/m = (a^2 + a^2) / 12 with a = 2
    std::vector<Vector3f> cv = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}};
    const std::vector<int32_t> ct = {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4, 3, 7, 6, 3, 6, 2, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5};
    auto cube = physics::HullMass(cv);
    REQUIRE(cube);
    CHECK(cube->volume == doctest::Approx(8.0).epsilon(1e-6));
    CHECK(std::sqrt(cube->centroid.x * cube->centroid.x + cube->centroid.y * cube->centroid.y + cube->centroid.z * cube->centroid.z) < 1e-6);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) CHECK(cube->inertia[i][j] == doctest::Approx(i == j ? 2.0 / 3.0 : 0.0).epsilon(1e-6).scale(1.0));
    // the hull ignores connectivity and duplicates: interior and repeated points change nothing
    std::vector<Vector3f> messy = cv;
    messy.push_back(Vector3f(0, 0, 0));
    messy.push_back(Vector3f(0.5f, -0.25f, 0.75f));
    messy.insert(messy.end(), cv.begin(), cv.end());
    auto cube2 = physics::HullMass(messy);
    REQUIRE(cube2);
    CHECK(cube2->volume == doctest::Approx(8.0).epsilon(1e-6));
    (void)ct;
    // coplanar points have no hull
    CHECK(!physics::HullMass({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 2, 0}}));

    // capsule: numeric integration on a grid against CapsuleMass (via ProxiesFromShapes)
    mu::PartColliderShape cs;
    cs.kind = mu::ColliderKind::Capsule;
    cs.radius = 0.5f;
    cs.halfCylinder = 0.75f;
    cs.axis = 1;
    const std::vector<ColliderProxy> cap = physics::ProxiesFromShapes({cs});
    REQUIRE(cap.size() == 1);
    REQUIRE(cap[0].mass);
    {
        const double r = 0.5, hc = 0.75;
        const int N = 160;
        const double ext = hc + r, h = 2.0 * ext / N, hr = 2.0 * r / N;
        double V = 0, Ixx = 0, Iyy = 0;
        for (int ix = 0; ix < N; ++ix)
            for (int iy = 0; iy < N; ++iy)
                for (int iz = 0; iz < N; ++iz) {
                    const double x = -r + (ix + 0.5) * hr, y = -ext + (iy + 0.5) * h, z = -r + (iz + 0.5) * hr;
                    const double ay = std::fabs(y);
                    const bool inside = ay <= hc ? (x * x + z * z <= r * r) : (x * x + z * z + (ay - hc) * (ay - hc) <= r * r);
                    if (!inside) continue;
                    const double dv = hr * h * hr;
                    V += dv;
                    Ixx += dv * (y * y + z * z);
                    Iyy += dv * (x * x + z * z);
                }
        CHECK(cap[0].mass->volume == doctest::Approx(V).epsilon(2e-3));
        CHECK(cap[0].mass->inertia[0][0] == doctest::Approx(Ixx / V).epsilon(5e-3));
        CHECK(cap[0].mass->inertia[1][1] == doctest::Approx(Iyy / V).epsilon(5e-3));
        CHECK(cap[0].mass->inertia[2][2] == doctest::Approx(Ixx / V).epsilon(5e-3));
    }
    // a zero-length capsule is a sphere: V = 4/3 pi r^3, I/m = 0.4 r^2
    cs.halfCylinder = 0.0f;
    const auto sph = physics::ProxiesFromShapes({cs});
    CHECK(sph[0].mass->volume == doctest::Approx(4.0 / 3.0 * 3.14159265358979323846 * 0.125).epsilon(1e-12));
    CHECK(sph[0].mass->inertia[0][0] == doctest::Approx(0.4 * 0.25).epsilon(1e-12));
    // Z and X capsules: the proxy rotation maps Jolt's Y axis onto the collider's axis
    for (int axis : {0, 2}) {
        cs.halfCylinder = 0.75f;
        cs.axis = axis;
        const auto p = physics::ProxiesFromShapes({cs})[0];
        const Vector3f y = mu::RotateNative(p.rotation, Vector3f(0, 1, 0));
        INFO("axis " << axis);
        CHECK(std::fabs(y.x) == doctest::Approx(axis == 0 ? 1.0 : 0.0).epsilon(1e-6));
        CHECK(std::fabs(y.z) == doctest::Approx(axis == 2 ? 1.0 : 0.0).epsilon(1e-6));
        CHECK(std::fabs(y.y) < 1e-6f);
    }

    // the real engine hulls: volume, centroid and inertia of Jolt's hull against the python reference (scipy / Qhull hull, float64
    // tetrahedra); the corpus has 28 mesh colliders, some of them not exactly convex (the hull is 2 to 15 % larger than the mesh solid)
    size_t hulls = 0;
    for (auto it = Reference()["parts"].begin(); it != Reference()["parts"].end(); ++it) {
        const json& part = it.value();
        const mu::Model model = mu::ReadModelFile(kData + part["reduced"]["file"].get<std::string>());
        const float rescale = FloatOfHex(part["rescaleFactor"]);
        const json empty = json::object();
        const auto inst = InstanceOf(&model, part["model"].is_null() ? empty : part["model"], part["modelNodes"].get<int>() > 0);
        const auto shapes = mu::BuildPartColliders(rescale, {inst});
        const json& cols = part["colliders"];
        for (size_t i = 0; i < shapes.size(); ++i) {
            if (shapes[i].kind != mu::ColliderKind::Mesh) continue;
            const json& mp = cols[i]["partSpace"]["massProps"];
            INFO(it.key() << " " << shapes[i].path);
            const auto m = physics::HullMass(shapes[i].vertices);
            REQUIRE(m);  // Jolt builds a hull of every real collider mesh
            const double V = std::stod(mp["volume"].get<std::string>());
            CHECK(m->volume == doctest::Approx(V).epsilon(3e-3));  // Jolt's hull merges points within 1e-3 m, float32 integrals
            double lo[3] = {1e30, 1e30, 1e30}, hi[3] = {-1e30, -1e30, -1e30};
            for (const Vector3f& v : shapes[i].vertices) {
                const double c[3] = {v.x, v.y, v.z};
                for (int k = 0; k < 3; ++k) { lo[k] = std::min(lo[k], c[k]); hi[k] = std::max(hi[k], c[k]); }
            }
            const double box = (hi[0] - lo[0]) * (hi[1] - lo[1]) * (hi[2] - lo[2]);
            CHECK(m->volume < box);
            CHECK(m->volume > 0.1 * box);
            const double scale = std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]});
            const double cen[3] = {m->centroid.x, m->centroid.y, m->centroid.z};
            for (int k = 0; k < 3; ++k) {
                CHECK(cen[k] == doctest::Approx(std::stod(mp["centroid"][static_cast<size_t>(k)].get<std::string>())).epsilon(3e-3).scale(scale));
                CHECK(cen[k] > lo[k]);
                CHECK(cen[k] < hi[k]);
                for (int l = 0; l < 3; ++l)
                    CHECK(m->inertia[k][l] == doctest::Approx(std::stod(mp["inertiaPerUnitMass"][static_cast<size_t>(k)][static_cast<size_t>(l)].get<std::string>()))
                                                   .epsilon(3e-3)
                                                   .scale(scale * scale));
            }
            ++hulls;
        }
    }
    CHECK(hulls == 28);  // 31 colliders: 28 meshes, 1 box, 2 capsules
}

TEST_CASE("WP-mucol proxies of the real engines in the compound: cluster inertia and the Jolt hull shape") {
    if (!wp20b::GameAvailable()) return;
    using namespace physics;
    // the hook feeds VesselPhysics real .mu proxies (Mainsail colliders on every
    // part of the head of PhysProf-S: LABELLED, the parts of this fixture are a probe core and tanks, the shapes are the Mainsail's)
    const json& part = Reference()["parts"]["liquidEngineMainsail.v2"];
    const mu::Model model = mu::ReadModelFile(kData + part["reduced"]["file"].get<std::string>());
    const mu::ModelInstance mi = InstanceOf(&model, json::object(), false);
    const auto shapes = mu::BuildPartColliders(FloatOfHex(part["rescaleFactor"]), {mi});
    const std::vector<ColliderProxy> proxies = ProxiesFromShapes(shapes);
    REQUIRE(proxies.size() == 2);
    for (const ColliderProxy& p : proxies) {
        CHECK(p.kind == ShapeKind::ConvexHull);
        CHECK(p.hull);
        CHECK(p.mass);
    }
    wp20b::World w;
    auto v = w.CraftHead("PhysProf-S", 3);
    REQUIRE(v->parts.size() == 3);
    VesselPhysicsOptions opt;
    opt.colliders = [&](const Part&) { return proxies; };
    PhysicsWorld world;
    VesselPhysics vp(world, *v, VesselPose{}, opt);
    REQUIRE(vp.ClusterCount() == 1);
    // the per-part tensor is the proxies' volume-weighted tensor (Part.cs:1890 stand-in) times Max(1, rb.mass): the cluster's own-term sum
    const Vector3f iu = ProxyInertiaPerUnitMass(proxies);
    CHECK(iu.x > 0.0f);
    CHECK(iu.y > 0.0f);
    CHECK(iu.z > 0.0f);
    double Ixx = 0, Iyy = 0, Izz = 0;
    const Vector3d c = vp.ClusterCenterOfMass(0);
    for (Part* p : v->parts) {
        const double m = vp.PartMass(*p);
        const Vector3f pc = p->fields.orgPos + p->fields.orgRot * p->fields.CoMOffset;
        const double dx = pc.x - c.x, dy = pc.y - c.y, dz = pc.z - c.z;
        // parts in PhysProf-S head are stacked along Y with identity orgRot, so the per-unit tensor needs no rotation
        const double k = std::max(1.0, m);
        Ixx += iu.x * k + m * (dy * dy + dz * dz);
        Iyy += iu.y * k + m * (dx * dx + dz * dz);
        Izz += iu.z * k + m * (dx * dx + dy * dy);
    }
    double I[3][3];
    vp.ClusterInertia(0, I);
    CHECK(I[0][0] == doctest::Approx(Ixx).epsilon(1e-6));
    CHECK(I[1][1] == doctest::Approx(Iyy).epsilon(1e-6));
    CHECK(I[2][2] == doctest::Approx(Izz).epsilon(1e-6));
    CHECK(world.GetMass(vp.ClusterBody(0)) == doctest::Approx(vp.ClusterMass(0)).epsilon(1e-6));
}

TEST_CASE("WP-mucol MuPartColliders on a PartLoader part: the Mainsail's .mu colliders through the loader seam, fixture fallback without data") {
    if (!wp20b::GameAvailable()) return;
    using namespace physics;
    wp20b::World w;
    auto v = w.CraftHead("PhysProf-S", 3);
    REQUIRE(v->parts.size() == 3);
    Part* mainsail = nullptr;
    for (Part* p : v->parts)
        if (p->partInfo && p->partInfo->name == "liquidEngineMainsail.v2") mainsail = p;
    REQUIRE(mainsail);
    REQUIRE(!mainsail->models.empty());
    // the mirror holds 0-byte placeholders for every .mu: without data the hook answers with the labelled fixture
    {
        MuPartColliders lib;
        std::vector<ColliderProxy> out;
        CHECK(!lib.Proxies(*mainsail, out));
        const auto fixture = lib.Hook()(*mainsail);
        const auto drag = DragCubeColliderProxy(*mainsail);
        REQUIRE(fixture.size() == drag.size());
        CHECK(fixture[0].kind == ShapeKind::Box);
        CHECK(lib.stats().modelErrors >= 1);
        CHECK(lib.stats().realParts == 0);
    }
    // the same part with the real model data (the reduced original, handed through the loader seam): the python reference's shapes
    const json& part = Reference()["parts"]["liquidEngineMainsail.v2"];
    const std::string reducedPath = kData + part["reduced"]["file"].get<std::string>();
    MuPartColliders lib([&](const std::string&) { return std::make_shared<const mu::Model>(mu::ReadModelFile(reducedPath)); });
    CHECK(mainsail->fields.rescaleFactor == FloatOfHex(part["rescaleFactor"]));
    CheckShapes(lib.Shapes(*mainsail), part["colliders"]);
    std::vector<ColliderProxy> out;
    REQUIRE(lib.Proxies(*mainsail, out));
    CHECK(out.size() == 2);
    CHECK(lib.stats().realParts == 1);
    CHECK(lib.stats().modelsRead == 1);
    const auto viaHook = lib.Hook()(*mainsail);
    CHECK(viaHook.size() == 2);
    CHECK(viaHook[0].kind == ShapeKind::ConvexHull);
}

TEST_CASE("WP-mucol Jolt convex hull of the real engine colliders: ray casts against the hull equal the mesh surface") {
    using namespace physics;
    // the collider mesh of a KSP convex MeshCollider is closed and convex, so Jolt's hull of its vertices has the mesh's surface:
    // vertical rays from above hit the topmost mesh triangle (Moller-Trumbore, double) at the same height
    size_t rays = 0;
    double worst = 0.0;
    for (const char* name : {"liquidEngineMainsail.v2", "turboFanSize2", "Size3AdvancedEngine", "solidBooster.v2"}) {
        const json& part = Reference()["parts"][name];
        const mu::Model model = mu::ReadModelFile(kData + part["reduced"]["file"].get<std::string>());
        const json empty = json::object();
        const auto inst = InstanceOf(&model, part["model"].is_null() ? empty : part["model"], part["modelNodes"].get<int>() > 0);
        const auto shapes = mu::BuildPartColliders(FloatOfHex(part["rescaleFactor"]), {inst});
        const auto proxies = ProxiesFromShapes(shapes);
        REQUIRE(!proxies.empty());
        PhysicsWorld world;
        uint64_t key = 1;
        for (const ColliderProxy& p : proxies) {
            if (p.kind != ShapeKind::ConvexHull) continue;
            REQUIRE(world.AddStaticShape(key++, ShapeDesc::ConvexHull(p.hull), Vector3d(0, 0, 0), QuaternionD::identity()) != kInvalidBody);
        }
        // the union of the hulls: the topmost surface over the mesh triangles of every mesh collider
        for (double x = -0.4; x <= 0.4; x += 0.2)
            for (double z = -0.4; z <= 0.4; z += 0.2) {
                double best = -1e30;
                for (const mu::PartColliderShape& s : shapes) {
                    if (s.kind != mu::ColliderKind::Mesh || s.trigger) continue;
                    for (size_t t = 0; t + 2 < s.triangles.size(); t += 3) {
                        const Vector3d a(s.vertices[static_cast<size_t>(s.triangles[t])]), b(s.vertices[static_cast<size_t>(s.triangles[t + 1])]),
                            c(s.vertices[static_cast<size_t>(s.triangles[t + 2])]);
                        // intersect the vertical line (x, *, z) with the triangle: barycentric in the xz projection
                        const double d = (b.z - c.z) * (a.x - c.x) + (c.x - b.x) * (a.z - c.z);
                        if (std::fabs(d) < 1e-12) continue;
                        const double l1 = ((b.z - c.z) * (x - c.x) + (c.x - b.x) * (z - c.z)) / d;
                        const double l2 = ((c.z - a.z) * (x - c.x) + (a.x - c.x) * (z - c.z)) / d;
                        const double l3 = 1.0 - l1 - l2;
                        if (l1 < -1e-9 || l2 < -1e-9 || l3 < -1e-9) continue;
                        best = std::max(best, l1 * a.y + l2 * b.y + l3 * c.y);
                    }
                }
                if (best < -1e29) continue;  // the ray misses every collider
                const auto hit = world.RayCast(Vector3d(x, 100.0, z), Vector3d(0, -1, 0), 200.0);
                REQUIRE(hit.userData != 0);
                const double y = 100.0 - hit.distance;
                worst = std::max(worst, std::fabs(y - best));
                ++rays;
            }
    }
    CHECK(rays > 20);
    CHECK(worst < 1e-3);
    MESSAGE("hull ray casts: " << rays << " rays, worst |hit - mesh| = " << worst << " m");
}

TEST_CASE("WP-mucol scenery collider loader: the documented JSON format (SYNTHETIC format fixture, not game data) feeds CityColliderSet") {
    const char* doc = R"({
      "schema": "kspcore-scenery-colliders/1",
      "source": {"note": "synthetic format fixture"},
      "cities": [ {
        "name": "Test/City",
        "objects": [
          {"path": "City", "parent": -1, "position": [5,5,5], "rotation": [0,0,0,1], "scale": [1,1,1], "colliders": []},
          {"path": "City/Pad", "parent": 0, "position": [10,0,0], "rotation": [0,0.70710678,0,0.70710678], "scale": [2,1,2],
           "colliders": [ {"type": "Box", "center": [0,1,0], "size": [4,2,6]},
                          {"type": "Box", "trigger": true, "center": [0,1,0], "size": [40,2,60]} ]},
          {"path": "City/Pad/Tower", "parent": 1, "position": [0,2,0], "rotation": [0,0,0,1], "scale": [1,3,1],
           "colliders": [ {"type": "Capsule", "center": [0,0,0], "radius": 0.5, "height": 2, "direction": 0},
                          {"type": "Sphere", "center": [0,1,0], "radius": 0.5},
                          {"type": "Mesh", "convex": false, "vertices": [0,0,0, 1,0,0, 0,0,1, 1,0,1], "triangles": [0,2,1, 1,2,3]} ]}
        ],
        "children": [1],
        "lod": [ {"visibleRange": 2500, "objects": [2]} ]
      } ] })";
    const SceneryColliders sc = ParseSceneryColliders(doc);
    REQUIRE(sc.cities.size() == 1);
    const SceneryCity* city = sc.Find("Test/City");
    REQUIRE(city);
    CHECK(city->objects.size() == 3);
    CHECK(city->children == std::vector<size_t>({1}));
    const std::vector<CityObject> objs = BuildCityObjects(*city);
    REQUIRE(objs.size() == 3);
    CHECK(objs[0].colliders.empty());
    REQUIRE(objs[1].colliders.size() == 1);  // the trigger is dropped
    // Pad: scale (2,1,2), rotated 90 deg about Y at (10,0,0): box centre (0,1,0) -> (10,1,0); half extents |size * scale| / 2 = (4, 1, 6)
    const CityCollider& pad = objs[1].colliders[0];
    CHECK(pad.kind == CityCollider::Kind::Box);
    CHECK(pad.localPosition.x == doctest::Approx(10.0).epsilon(1e-6));
    CHECK(pad.localPosition.y == doctest::Approx(1.0).epsilon(1e-6));
    CHECK(pad.shape.halfExtents.x == doctest::Approx(4.0));
    CHECK(pad.shape.halfExtents.y == doctest::Approx(1.0));
    CHECK(pad.shape.halfExtents.z == doctest::Approx(6.0));
    // Tower under the Pad: position (0,2,0) scaled (2,1,2) rotated by Ry(90) -> (10, 2, 0); scale (2,3,2); capsule along X: radius
    // 0.5 * max(3, 2) = 1.5, height 2 * 2 = 4 -> half cylinder 0.5
    REQUIRE(objs[2].colliders.size() == 3);
    const CityCollider& cap = objs[2].colliders[0];
    CHECK(cap.kind == CityCollider::Kind::Capsule);
    CHECK(cap.localPosition.x == doctest::Approx(10.0).epsilon(1e-6));
    CHECK(cap.localPosition.y == doctest::Approx(2.0).epsilon(1e-6));
    CHECK(cap.shape.radius == doctest::Approx(1.5));
    CHECK(cap.shape.halfHeight == doctest::Approx(0.5));
    CHECK(objs[2].colliders[1].shape.radius == doctest::Approx(0.5 * 3.0));
    CHECK(objs[2].colliders[2].kind == CityCollider::Kind::Mesh);
    CHECK(objs[2].colliders[2].triangles.size() == 6);
    // the activation structure over the loaded objects
    physics::PhysicsWorld world;
    PQSCityColliders city1(world, 1000);
    ApplySceneryLod(city1, *city);
    CHECK(city1.set.ObjectCount() == 3);
    CHECK(city1.children == std::vector<size_t>({1}));
    REQUIRE(city1.lod.size() == 1);
    CHECK(city1.lod[0].objects == std::vector<size_t>({2}));
    city1.set.SetPose(Vector3d(100, 0, 0), QuaternionD::identity());
    city1.set.SetObjectActive(1, true);
    city1.set.SetObjectActive(2, true);
    CHECK(city1.set.BodyCount() == 4);  // pad box + capsule + sphere + mesh
    const auto hit = world.RayCast(Vector3d(110, 50, 0), Vector3d(0, -1, 0), 100.0);
    CHECK(hit.userData != 0);
    // malformed documents are refused
    CHECK_THROWS_AS(ParseSceneryColliders(R"({"schema":"other","cities":[]})"), std::runtime_error);
    CHECK_THROWS_AS(ParseSceneryColliders(R"({"schema":"kspcore-scenery-colliders/1","cities":[{"name":"x","objects":[
        {"parent":1,"position":[0,0,0],"rotation":[0,0,0,1],"scale":[1,1,1]}]}]})"), std::runtime_error);
    CHECK_THROWS_AS(ParseSceneryColliders(R"({"schema":"kspcore-scenery-colliders/1","cities":[{"name":"x","objects":[
        {"parent":-1,"position":[0,0,0],"rotation":[0,0,0,1],"scale":[1,1,1],"colliders":[
          {"type":"Mesh","vertices":[0,0,0,1,0,0,0,1,0],"triangles":[0,1,5]}]}]}]})"), std::runtime_error);
    // the extracted file, when present (tools/mucollider/extract_scenery_colliders.py): loads and builds
    const std::string extracted = std::string(KSPCORE_DATA_DIR) + "/scenery/scenery_colliders.json";
    if (std::filesystem::exists(extracted)) {
        const SceneryColliders real = LoadSceneryColliders(extracted);
        CHECK(!real.cities.empty());
        for (const SceneryCity& c : real.cities) CHECK(!BuildCityObjects(c).empty());
    } else {
        MESSAGE("core/data/scenery/scenery_colliders.json not present: the KSC extraction is the documented next step (STATUS.md WP-mucol)");
    }
}
