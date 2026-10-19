// WP-12c unit tests: the KSC / launch-site scenery colliders from the game's assets, their materials, and the PhysX material combine.
//
// Data: core/data/scenery/scenery_colliders.json + scenery_meshes.bin, extracted from KSP_x64_Data by
// tools/mucollider/extract_scenery_colliders.py (UnityPy) and re-extracted by a second route by tools/wp12c/verify_scenery_extraction.py
// (raw type trees, own mesh vertex decode), which wrote tests/data/wp12c_extraction_digest.json (sha256 of both files, what it verified,
// 0 mismatches). CI has no game files: the digest test ties the COMMITTED data to that verification.
// Independent reference: tools/wp12c/scenery_reference.py (numpy float32, no kspcore code) -> tests/data/wp12c_reference.json.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "datasets.hpp"  // Sha256Hex (tools/golden_runner/sha256.cpp)
#include "doctest/doctest.h"
#include "ksp/physics/physics_world.hpp"
#include "ksp/space_center/scenery_colliders.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using nlohmann::json;

namespace {

const std::string kScenery = std::string(KSPCORE_DATA_DIR) + "/scenery/";
const std::string kData = std::string(KSPCORE_TEST_DATA_DIR) + "/";

std::string ReadFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    REQUIRE_MESSAGE(f.good(), path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

const json& Reference() {
    static const json ref = json::parse(ReadFile(kData + "wp12c_reference.json"));
    return ref;
}

const SceneryColliders& Doc() {
    static const SceneryColliders doc = LoadSceneryColliders(kScenery + "scenery_colliders.json");
    return doc;
}

uint64_t Fnv(const void* data, size_t n) {
    uint64_t h = 0xcbf29ce484222325ull;
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 0x100000001b3ull;
    }
    return h;
}
std::string Hex64(uint64_t v) {
    char b[24];
    std::snprintf(b, sizeof b, "%016llx", static_cast<unsigned long long>(v));
    return b;
}
std::string HexF(std::initializer_list<float> vals) {
    std::string s;
    for (float f : vals) {
        uint32_t u;
        std::memcpy(&u, &f, 4);
        char b[12];
        std::snprintf(b, sizeof b, "%08x", u);
        s += b;
    }
    return s;
}
std::string IdxHash(std::vector<size_t> v) {
    std::vector<uint32_t> u(v.begin(), v.end());
    return Hex64(Fnv(u.data(), u.size() * 4));
}
float F(double d) { return static_cast<float>(d); }

std::string ColliderHex(const CityCollider& c) {
    const float px = F(c.localPosition.x), py = F(c.localPosition.y), pz = F(c.localPosition.z);
    const float qx = F(c.localRotation.x), qy = F(c.localRotation.y), qz = F(c.localRotation.z), qw = F(c.localRotation.w);
    switch (c.kind) {
        case CityCollider::Kind::Box:
            return HexF({c.shape.halfExtents.x, c.shape.halfExtents.y, c.shape.halfExtents.z, px, py, pz, qx, qy, qz, qw});
        case CityCollider::Kind::Sphere:
            return HexF({c.shape.radius, px, py, pz, qx, qy, qz, qw});
        case CityCollider::Kind::Capsule:
            return HexF({c.shape.halfHeight, c.shape.radius, px, py, pz, qx, qy, qz, qw});
        default:
            return "";
    }
}

}  // namespace

TEST_CASE("WP-12c data: the committed extraction is the one verified against the game (digests, counts, per-mesh sha256)") {
    const json digest = json::parse(ReadFile(kData + "wp12c_extraction_digest.json"));
    const std::string doc = ReadFile(kScenery + "scenery_colliders.json");
    const std::string blob = ReadFile(kScenery + "scenery_meshes.bin");
    CHECK(Sha256Hex(doc) == digest.at("scenery_colliders.json").get<std::string>());
    CHECK(Sha256Hex(blob) == digest.at("scenery_meshes.bin").get<std::string>());
    CHECK(digest.at("mismatches").get<int>() == 0);

    // counts of the verified extraction = the parsed document
    const json raw = json::parse(doc);
    size_t objects = 0, colliders = 0, meshColliders = 0;
    for (const json& c : raw.at("cities")) {
        objects += c.at("objects").size();
        for (const json& o : c.at("objects"))
            for (const json& k : o.at("colliders")) {
                ++colliders;
                meshColliders += k.at("type") == "Mesh" ? 1 : 0;
            }
    }
    const json& v = digest.at("verified");
    CHECK(raw.at("cities").size() == v.at("cities").get<size_t>());
    CHECK(objects == v.at("objects").get<size_t>());
    CHECK(colliders == v.at("colliders").get<size_t>());
    CHECK(meshColliders == v.at("meshColliders").get<size_t>());
    CHECK(raw.at("meshes").size() == digest.at("meshesVerified").get<size_t>());

    // every mesh record: sha256 of its bytes in the blob (the value the verifier compared against an independent decode of the asset)
    size_t total = 0;
    for (const json& m : raw.at("meshes")) {
        const size_t nv = m.at("vertexCount"), ni = m.at("indexCount"), off = m.at("offset");
        REQUIRE(off + nv * 12 + ni * 4 <= blob.size());
        CHECK(Sha256Hex(blob.substr(off, nv * 12 + ni * 4)) == m.at("sha256").get<std::string>());
        total += nv * 12 + ni * 4;
    }
    CHECK(total == blob.size());
    MESSAGE(raw.at("cities").size() << " cities, " << objects << " objects, " << colliders << " colliders (" << meshColliders << " mesh), "
                                    << raw.at("meshes").size() << " meshes, " << blob.size() << " bytes");
}

TEST_CASE("WP-12c data: PhysicMaterials of the game (Ground, the default Metal), the stock tags, launch sites, facilities") {
    const SceneryColliders& d = Doc();
    // PhysicsManager.m_DefaultMaterial = "Metal" (globalgamemanagers): the material of every collider without one (all stock scenery colliders)
    REQUIRE(d.defaultMaterial.has_value());
    CHECK(d.defaultMaterial->name == "Metal");
    CHECK(d.defaultMaterial->dynamicFriction == 0.25f);
    CHECK(d.defaultMaterial->staticFriction == 0.25f);
    CHECK(d.defaultMaterial->bounciness == 0.0f);
    CHECK(d.defaultMaterial->frictionCombine == 3);   // Maximum
    CHECK(d.defaultMaterial->bounceCombine == 2);     // Minimum
    CHECK(d.bounceThreshold == 0.2f);
    // PQSMod_QuadMeshColliders.physicsMaterial = "Ground" on the bodies that have one
    REQUIRE(d.materials.size() == 1);
    CHECK(d.materials[0].name == "Ground");
    CHECK(d.materials[0].dynamicFriction == doctest::Approx(0.9f));
    CHECK(d.materials[0].staticFriction == doctest::Approx(0.9f));
    CHECK(d.materials[0].bounciness == 0.0f);
    CHECK(d.materials[0].frictionCombine == 3);
    CHECK(d.materials[0].bounceCombine == 2);
    for (const char* b : {"Kerbin", "Minmus", "Moho", "Eve", "Laythe", "Dres"}) {
        REQUIRE(d.terrainMaterial.count(b));
        CHECK(d.terrainMaterial.at(b) == 0);
        CHECK(d.TerrainMaterial(b)->name == "Ground");
    }
    for (const char* b : {"Mun", "Duna", "Ike", "Vall", "Bop", "Tylo", "Gilly", "Pol", "Eeloo"}) {
        CHECK(d.terrainMaterial.at(b) == -1);
        CHECK(d.TerrainMaterial(b)->name == "Metal");   // no material on the collider = Physics.defaultMaterial
    }
    // no stock scenery collider carries its own material
    for (const SceneryCity& c : d.cities)
        for (const SceneryObjectSpec& o : c.objects)
            for (const SceneryColliderSpec& k : o.colliders) CHECK(k.material == -1);

    // PSystemSetup (level9): the stock launch sites and facilities
    std::set<std::string> sites;
    for (const SceneryLaunchSite& s : d.launchSites) sites.insert(s.name);
    for (const char* n : {"Island_Airfield", "Crater_Launch_Site", "MahiMahi_Launch_Site", "Cove_Launch_Site", "GlacierLake_Launch_Site"}) CHECK(sites.count(n));
    for (const char* n : {"Woomerang_Launch_Site", "Desert_Airfield", "Desert_Launch_Site"}) {
        CHECK(sites.count(n));   // Making History: in the table, no scenery in the base game
        for (const SceneryLaunchSite& s : d.launchSites)
            if (s.name == n) CHECK(s.bundleName == "makinghistory_assets");
    }
    std::set<std::string> fac;
    for (const SceneryFacility& f : d.facilities) fac.insert(f.name);
    for (const char* n : {"LaunchPad", "Runway", "VAB", "SPH", "TrackingStation", "AstronautComplex", "MissionControl", "RnD", "Administration", "FlagPole"}) CHECK(fac.count(n));
    const std::set<std::string> tags(d.tags.begin(), d.tags.end());
    for (const char* t : {"LaunchPad", "Runway", "KSC_Runway_09", "KSC_Runway_27", "KSC_VAB_Main_Building", "KSC_LaunchPad_Platform", "Island_Airfield"}) CHECK(tags.count(t));

    // the cities
    for (const char* n : {"Kerbin/KSC", "Kerbin/KSC2", "Kerbin/IslandAirfield", "Kerbin/Crater_Launch_Site", "Kerbin/MahiMahi_Launch_Site",
                          "Kerbin/Cove_Launch_Site", "Kerbin/GlacierLake_Launch_Site", "Mun/ArmstrongMemorial", "Kerbin/Pyramids"})
        CHECK(d.Find(n) != nullptr);
    const SceneryCity& ksc = *d.Find("Kerbin/KSC");
    CHECK(ksc.className == "PQSCity");
    CHECK(ksc.objects.size() == 2795);
    CHECK(ksc.children.size() == 4);   // SpaceCenterMarker, DestructionCollider, SpaceCenter (the spawned KSC prefab), CommNetNode
    CHECK(ksc.destructibles.size() == 75);
    // the spawned prefab keeps the placeholder's name, as UpgradeLevel.Spawn / NestedPrefabSpawner.Spawn name it
    CHECK(ksc.FindObject("KSC/SpaceCenter") >= 0);
    CHECK(ksc.FindObject("KSC/SpaceCenter/LaunchPad/Facility/LaunchPad_spawn") >= 0);
    CHECK(ksc.FindObject("KSC/SpaceCenter/Runway/Facility/End09/SpawnPoint") >= 0);
    // the facility levels: every level of the nine upgradeable facilities and the slaves is in the data
    std::map<std::string, std::set<int>> levels;
    for (const SceneryObjectSpec& o : ksc.objects)
        if (!o.levelGroup.empty()) levels[o.levelGroup].insert(o.level);
    CHECK(levels.size() == 19);
    CHECK(levels.at("SpaceCenter/LaunchPad") == std::set<int>{0, 1, 2});
    CHECK(levels.at("SpaceCenter/Runway") == std::set<int>{0, 1, 2});
    CHECK(levels.at("SpaceCenter/VehicleAssemblyBuilding") == std::set<int>{0, 1, 2});
}

TEST_CASE("WP-12c loader vs the independent reference: every collider of every city in the city frame (float32, bit for bit), the GameObject data of the contact records") {
    const SceneryColliders& d = Doc();
    const json& ref = Reference().at("cities");
    REQUIRE(ref.size() == d.cities.size());
    size_t compared = 0, objectsChecked = 0;
    for (const SceneryCity& city : d.cities) {
        const std::string key = city.name + "#" + std::to_string(city.gameObjectPathId);
        REQUIRE_MESSAGE(ref.contains(key), key);
        const json& rc = ref.at(key).at("objects");
        const std::vector<CityObject> built = BuildCityObjects(city);
        REQUIRE(built.size() == rc.size());
        for (size_t i = 0; i < built.size(); ++i) {
            const json& ro = rc[i];
            const CityObject& o = built[i];
            ++objectsChecked;
            CHECK_MESSAGE(o.layer == ro.at("layer").get<int>(), key << " #" << i);
            CHECK(o.tag == ro.at("tag").get<std::string>());
            CHECK(o.name == ro.at("name").get<std::string>());
            CHECK(o.crashObjectName == ro.at("crash").get<std::string>());
            CHECK(o.mobileLaunchPad == ro.at("mobile").get<bool>());
            const json& rcol = ro.at("colliders");
            REQUIRE_MESSAGE(o.colliders.size() == rcol.size(), key << " #" << i);
            for (size_t k = 0; k < o.colliders.size(); ++k) {
                const CityCollider& c = o.colliders[k];
                const json& rk = rcol[k];
                ++compared;
                if (c.kind == CityCollider::Kind::Mesh) {
                    CHECK(rk.at("k") == "M");
                    CHECK(c.vertices.size() == rk.at("n").get<size_t>());
                    CHECK(c.triangles.size() == rk.at("t").get<size_t>());
                    std::vector<float> flat;
                    flat.reserve(c.vertices.size() * 3);
                    for (const Vector3f& v : c.vertices) {
                        flat.push_back(v.x);
                        flat.push_back(v.y);
                        flat.push_back(v.z);
                    }
                    std::vector<uint32_t> idx(c.triangles.begin(), c.triangles.end());
                    CHECK_MESSAGE(Hex64(Fnv(flat.data(), flat.size() * 4)) == rk.at("vh").get<std::string>(), key << " #" << i << " mesh vertices");
                    CHECK(Hex64(Fnv(idx.data(), idx.size() * 4)) == rk.at("ih").get<std::string>());
                } else {
                    const char kind = c.kind == CityCollider::Kind::Box ? 'B' : (c.kind == CityCollider::Kind::Sphere ? 'S' : 'C');
                    CHECK(rk.at("k").get<std::string>() == std::string(1, kind));
                    CHECK_MESSAGE(ColliderHex(c) == rk.at("v").get<std::string>(), key << " #" << i << " collider " << k);
                }
            }
        }
    }
    MESSAGE(compared << " colliders of " << objectsChecked << " objects in " << d.cities.size() << " cities: 0 mismatches expected");
    CHECK(compared > 2000);
}

TEST_CASE("WP-12c levels and activation vs the independent reference: presence for three level selections, the switched subtrees, the initially active objects") {
    const SceneryColliders& d = Doc();
    const json& ref = Reference().at("cities");
    for (const SceneryCity& city : d.cities) {
        const json& rc = ref.at(city.name + "#" + std::to_string(city.gameObjectPathId));
        std::set<std::string> groups;
        for (const SceneryObjectSpec& o : city.objects)
            if (!o.levelGroup.empty()) groups.insert(o.levelGroup);
        SceneryLevels first, highest, mixed;
        first.fallback = 0;
        highest.fallback = -1;
        mixed.fallback = 1;
        int gi = 0;
        for (const std::string& g : groups) mixed.level[g] = gi++ % 3;
        const std::pair<const char*, const SceneryLevels*> sels[] = {{"first", &first}, {"highest", &highest}, {"mixed", &mixed}};
        for (const auto& [name, lv] : sels) {
            const std::vector<bool> pres = SceneryPresence(city, *lv);
            std::vector<size_t> presentIdx;
            for (size_t i = 0; i < pres.size(); ++i)
                if (pres[i]) presentIdx.push_back(i);
            CHECK_MESSAGE(IdxHash(presentIdx) == rc.at("presence").at(name).get<std::string>(), city.name << " presence " << name);
            std::vector<size_t> roots = city.children;
            for (const SceneryLod& l : city.lod) roots.insert(roots.end(), l.objects.begin(), l.objects.end());
            std::set<size_t> u;
            for (size_t r : roots)
                for (size_t i : SceneryActiveSubtree(city, r, pres)) u.insert(i);
            CHECK_MESSAGE(IdxHash(std::vector<size_t>(u.begin(), u.end())) == rc.at("subtrees").at(name).get<std::string>(), city.name << " subtrees " << name);
            CHECK_MESSAGE(IdxHash(SceneryInitiallyActive(city, *lv)) == rc.at("initial").at(name).get<std::string>(), city.name << " initial " << name);
        }
    }
    // the level selection is real: KSC at the first level has the level-0 pad, at the highest level the big one
    const SceneryCity& ksc = *d.Find("Kerbin/KSC");
    SceneryLevels hi;
    hi.fallback = -1;
    const std::vector<bool> p0 = SceneryPresence(ksc), p2 = SceneryPresence(ksc, hi);
    size_t n0 = 0, n2 = 0;
    for (size_t i = 0; i < ksc.objects.size(); ++i) {
        n0 += p0[i] ? 1 : 0;
        n2 += p2[i] ? 1 : 0;
    }
    CHECK(n0 < ksc.objects.size());
    CHECK(n2 < ksc.objects.size());
    CHECK(n0 != n2);
}

// ---- PhysX material combine (PhysicsWorldConfig::physxMaterials) ---------------------------------------------------------------------------
namespace {

std::shared_ptr<const physics::PhysicMaterialDesc> Mat(float f, int frictionCombine, float bounce = 0.0f, int bounceCombine = 0) {
    auto m = std::make_shared<physics::PhysicMaterialDesc>();
    m->dynamicFriction = m->staticFriction = f;
    m->bounciness = bounce;
    m->frictionCombine = frictionCombine;
    m->bounceCombine = bounceCombine;
    return m;
}

// A 100 kg cube slides at 8 m/s on a static floor under a downward force m * g each step; the speed lost in 0.5 s is mu * g * 0.5
double SlideLoss(bool physx, std::shared_ptr<const physics::PhysicMaterialDesc> boxMat, std::shared_ptr<const physics::PhysicMaterialDesc> floorMat,
                 float boxBodyFriction, float floorBodyFriction, double* heightAfter = nullptr) {
    physics::PhysicsWorldConfig cfg;
    cfg.physxMaterials = physx;
    cfg.defaultMaterial = *Mat(0.25f, 3, 0.0f, 2);
    physics::PhysicsWorld world(cfg);
    physics::ShapeDesc floorShape = physics::ShapeDesc::Box(Vector3f(50.0f, 50.0f, 50.0f));
    floorShape.material = floorMat;
    world.AddStaticShape(1, floorShape, Vector3d(0.0, -50.0, 0.0), QuaternionD::identity(), floorBodyFriction, 0.0f);
    physics::BodyDesc d;
    d.shape = physics::ShapeDesc::Box(Vector3f(0.5f, 0.5f, 0.5f));
    d.shape.material = boxMat;
    d.position = Vector3d(0.0, 0.5, 0.0);
    d.mass = 100.0f;
    d.friction = boxBodyFriction;
    d.restitution = 0.0f;
    d.linearVelocity = Vector3d(8.0, 0.0, 0.0);
    const physics::BodyId id = world.CreateBody(d);
    const double g = 9.80665;
    const float dt = 1.0f / 60.0f;
    for (int k = 0; k < 30; ++k) {
        world.AddForce(id, Vector3d(0.0, -100.0 * g, 0.0));
        world.Step(dt);
    }
    if (heightAfter != nullptr) *heightAfter = world.GetPosition(id).y;
    return 8.0 - world.GetLinearVelocity(id).x;
}

}  // namespace

TEST_CASE("WP-12c PhysX material combine: the sliding deceleration is mu_combined * g for the combine-mode priority rules (Maximum > Multiply > Minimum > Average)") {
    const double g = 9.80665, t = 0.5;
    struct Case {
        const char* what;
        std::shared_ptr<const physics::PhysicMaterialDesc> box, floor;
        double mu;
    };
    const SceneryColliders& doc = Doc();
    const auto ground = std::make_shared<const physics::PhysicMaterialDesc>(doc.materials[0].Desc());
    const auto metal = std::make_shared<const physics::PhysicMaterialDesc>(doc.defaultMaterial->Desc());
    const Case cases[] = {
        {"part (Metal) on Kerbin's Ground: Maximum / Maximum -> 0.9", metal, ground, 0.9},
        {"part (Metal) on a KSC collider (default Metal): Maximum -> 0.25", metal, metal, 0.25},
        {"Average 0.8 vs Average 0.4 -> 0.6", Mat(0.8f, 0), Mat(0.4f, 0), 0.6},
        {"Minimum 0.8 vs Average 0.4: Minimum has priority -> 0.4", Mat(0.8f, 2), Mat(0.4f, 0), 0.4},
        {"Multiply 0.8 vs Minimum 0.5: Multiply has priority -> 0.4", Mat(0.8f, 1), Mat(0.5f, 2), 0.4},
        {"Maximum 0.2 vs Multiply 0.9: Maximum has priority -> 0.9", Mat(0.2f, 3), Mat(0.9f, 1), 0.9},
        {"Minimum 0.7 vs Minimum 0.3 -> 0.3", Mat(0.7f, 2), Mat(0.3f, 2), 0.3},
    };
    for (const Case& c : cases) {
        const double loss = SlideLoss(true, c.box, c.floor, 0.5f, 0.5f);
        const double want = c.mu * g * t;
        MESSAGE(c.what << ": speed lost " << loss << " m/s, analytic " << want);
        CHECK(std::fabs(loss - want) < 0.002 * want + 0.002);   // Jolt's solver reproduces mu * g * t to 4 digits
    }
    // the legacy rule (physxMaterials off): Jolt's sqrt(f1 * f2) of the bodies' friction
    const double legacy = SlideLoss(false, nullptr, nullptr, 0.25f, 0.9f);
    CHECK(std::fabs(legacy - std::sqrt(0.25 * 0.9) * g * t) < 0.002 * std::sqrt(0.25 * 0.9) * g * t + 0.002);
    // and the two rules differ for the stock pair by the factor 0.9 / 0.474
    const double px = SlideLoss(true, metal, ground, 0.25f, 0.9f);
    CHECK(px > 1.8 * legacy);
}

TEST_CASE("WP-12c materials: the scenery host gives static bodies the game's materials; a world without the config keeps the old rule") {
    const SceneryColliders& doc = Doc();
    physics::PhysicsWorldConfig cfg;
    doc.ApplyTo(cfg);
    CHECK(cfg.physxMaterials);
    CHECK(cfg.defaultMaterial.dynamicFriction == 0.25f);
    CHECK(cfg.defaultMaterial.frictionCombine == 3);
    CHECK(cfg.defaultMaterial.bounceCombine == 2);
    CHECK(cfg.minVelocityForRestitution == 0.2f);
    const auto kerbin = doc.TerrainMaterialDesc("Kerbin");
    REQUIRE(kerbin != nullptr);
    CHECK(kerbin->dynamicFriction == doctest::Approx(0.9f));
    const auto mun = doc.TerrainMaterialDesc("Mun");   // no material on the collider: Physics.defaultMaterial
    REQUIRE(mun != nullptr);
    CHECK(mun->dynamicFriction == 0.25f);
    physics::PhysicsWorldConfig plain;
    CHECK(!plain.physxMaterials);
}
