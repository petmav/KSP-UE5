// WP-12c scenarios: the real KSC scenery colliders (data/scenery) hosted in the physics bubble through ContactHost::RegisterStatic, placed by
// WP-42's PQSCity placement of "Kerbin/KSC", with the game's PhysicMaterials (Ground, the default "Metal", PhysX combine).
//   * a craft set down on the launch pad rests on the pad collider (tag LaunchPad) and Vessel.landedAt reads "LaunchPad"
//     (Part.OnCollisionEnter -> LandedCollisionChecks -> Vessel.SetLandedAt(tag));
//   * a rover on the runway: the wheel ray hits the runway collider and ModuleWheelBase.LandingDetectionUpdate names it (landedAt);
//   * the contact records carry the collider's layer / tag / name / CrashObjectName;
//   * PhysX material combine: sliding deceleration = mu_combined * g with Ground (0.9, Maximum), Metal (0.25, Maximum), the legacy rule.
// References: tools/wp12c/scenery_reference.py (vertical rays onto the placed pad / runway triangles in double, the spawn transforms' surface
// heights), analytic sliding decelerations.
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>

#include "contact_fixture.hpp"
#include "doctest/doctest.h"
#include "ksp/aero/lifting_surface.hpp"
#include "ksp/input/flight_input_handler.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/modules/module_resource_intake.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/vessel/vessel_autopilot.hpp"
#include "ksp/physics/scenery_host.hpp"
#include "ksp/physics/wheel_models.hpp"
#include "ksp/space_center/pqs_city.hpp"
#include "ksp/space_center/scenery_colliders.hpp"
#include "ksp/terrain/pqs.hpp"
#include "nlohmann/json.hpp"
#include "vessel_wheels_fixture.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp20e;
using nlohmann::json;

namespace {

const std::string kScenery12c = std::string(KSPCORE_DATA_DIR) + "/scenery/";

const SceneryColliders& Scenery() {
    static const SceneryColliders doc = LoadSceneryColliders(kScenery12c + "scenery_colliders.json");
    return doc;
}

const json& Ref12c() {
    static const json ref = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp12c_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return ref;
}

PhysicsWorldConfig GameWorldConfig() {
    PhysicsWorldConfig cfg;
    Scenery().ApplyTo(cfg);   // Physics.defaultMaterial = Metal, PhysX combine, Physics.bounceThreshold
    return cfg;
}

const PQSCityPrefab& KscPrefab() {
    for (const PQSCityPrefab& p : StockPQSCities())
        if (std::string(p.bodyName) == "Kerbin" && std::string(p.name) == "KSC") return p;
    FAIL("no stock Kerbin/KSC PQSCity");
    return StockPQSCities().front();
}

// the world pose of a scenery node: the city's pose (PQSCity.Orientate) then the node's TRS chain (double; placement, not a gate)
struct CityPose {
    Vector3d localPosition;    // PQS-local
    QuaternionD localRotation;
};
CityPose KscPose(CelestialBody& kerbin) {
    PQS pqs;
    pqs.radius = kerbin.Radius;
    PQSCity ksc(KscPrefab(), &kerbin, &pqs);
    ksc.Orientate();
    return {Vector3d(ksc.localPosition), QuaternionD(ksc.localRotation)};
}

Vector3d NodeInCityFrame(const SceneryCity& city, size_t index) {
    // positions only (the parents' rotations and scales applied), the same composition BuildCityObjects uses, in double
    struct T {
        Vector3d pos;
        QuaternionD rot = QuaternionD::identity();
        Vector3d scl{1.0, 1.0, 1.0};
    };
    std::vector<T> w(index + 1);
    for (size_t i = 1; i <= index; ++i) {
        const SceneryObjectSpec& o = city.objects[i];
        const T& p = w[static_cast<size_t>(o.parent)];
        T t;
        t.pos = p.pos + p.rot * Vector3d(p.scl.x * o.position.x, p.scl.y * o.position.y, p.scl.z * o.position.z);
        t.rot = p.rot * QuaternionD(o.rotation);
        t.scl = Vector3d(p.scl.x * o.scale.x, p.scl.y * o.scale.y, p.scl.z * o.scale.z);
        w[i] = t;
    }
    return w[index].pos;
}

int FindPresent(const SceneryCity& city, const SceneryLevels& lv, const char* leaf, const char* mustContain = nullptr) {
    const std::vector<bool> present = SceneryPresence(city, lv);
    for (size_t i = 0; i < city.objects.size(); ++i) {
        const std::string& p = city.objects[i].path;
        if (!present[i] || p.size() < std::string(leaf).size() || p.compare(p.size() - std::string(leaf).size(), std::string::npos, leaf) != 0) continue;
        if (mustContain != nullptr && p.find(mustContain) == std::string::npos) continue;
        return static_cast<int>(i);
    }
    return -1;
}

}  // namespace

TEST_CASE("WP-12c pad: the vertical ray through the launch pad spawn point meets the surface the independent reference computes (double, placed float triangles)") {
    const SceneryColliders& doc = Scenery();
    const SceneryCity& ksc = *doc.Find("Kerbin/KSC");
    const json& rays = Ref12c().at("rays");
    // the KSC colliders of the LaunchPad / Runway tags only, level 0, in a bare Jolt world at the city frame
    for (const char* tag : {"LaunchPad", "Runway"}) {
        physics::PhysicsWorld world;
        PQSCityColliders pc(world, 0x100000);
        ApplySceneryLod(pc, ksc);
        pc.set.SetPose(Vector3d(), QuaternionD::identity());
        const std::vector<bool> present = SceneryPresence(ksc);
        std::set<size_t> active;
        for (size_t root : ksc.children)
            for (size_t i : SceneryActiveSubtree(ksc, root, present)) active.insert(i);
        size_t n = 0;
        for (size_t i : active)
            if (ksc.objects[i].tag == tag && !ksc.objects[i].colliders.empty()) {
                pc.set.SetObjectActive(i, true);
                ++n;
            }
        REQUIRE(n > 0);
        world.OptimizeBroadPhase();
        const json& spawn = rays.at(tag).at("spawn");
        const double x = spawn.at("x").get<double>(), z = spawn.at("z").get<double>(), want = spawn.at("surface").get<double>();
        const PhysicsWorld::RayHit h = world.RayCast(Vector3d(x, 500.0, z), Vector3d(0.0, -1.0, 0.0), 1000.0, PhysicsWorld::kLayerStatic);
        REQUIRE(h.hit);
        MESSAGE(tag << ": " << n << " colliders, spawn transform at y " << spawn.at("y").get<double>() << ", reference surface " << want << ", Jolt ray " << h.point.y);
        CHECK(std::fabs(h.point.y - want) < 2e-3);   // Jolt's float ray fraction of a 1000 m ray: ~6e-5 m, plus the float cast of the triangles
        // the sample grid over the pad / runway extent
        size_t compared = 0;
        double worst = 0.0;
        for (const json& s : rays.at(tag).at("samples")) {
            const PhysicsWorld::RayHit hs = world.RayCast(Vector3d(s[0].get<double>(), 500.0, s[1].get<double>()), Vector3d(0.0, -1.0, 0.0), 1000.0,
                                                          PhysicsWorld::kLayerStatic);
            REQUIRE(hs.hit);
            worst = std::max(worst, std::fabs(hs.point.y - s[2].get<double>()));
            ++compared;
        }
        MESSAGE(tag << ": " << compared << " ray samples, worst |Jolt - reference| " << worst << " m");
        CHECK(compared > 20);
        CHECK(worst < 2e-3);
    }
}

TEST_CASE("WP-12c launch sites: the surface under the SpawnPoint of the island airfield and the four stock mobile launch sites (Jolt ray vs the reference)") {
    const SceneryColliders& doc = Scenery();
    const json& sites = Ref12c().at("siteRays");
    REQUIRE(sites.size() == 5);
    for (auto it = sites.begin(); it != sites.end(); ++it) {
        const SceneryCity& city = *doc.Find(it.key());
        const std::string tag = it.value().at("tag").get<std::string>();
        physics::PhysicsWorld world;
        CityColliderSet set(world, 0x200000);
        AddSceneryToSet(set, city);
        set.SetPose(Vector3d(), QuaternionD::identity());
        const std::vector<bool> present = SceneryPresence(city);
        std::set<size_t> active;
        for (size_t i : SceneryInitiallyActive(city)) active.insert(i);
        std::vector<size_t> roots = city.className == "PQSCity2" ? std::vector<size_t>() : city.children;
        for (const SceneryLod& l : city.lod) roots.insert(roots.end(), l.objects.begin(), l.objects.end());
        for (size_t r : roots)
            for (size_t i : SceneryActiveSubtree(city, r, present)) active.insert(i);
        size_t n = 0;
        for (size_t i : active)
            if (city.objects[i].tag == tag && !city.objects[i].colliders.empty()) {
                set.SetObjectActive(i, true);
                ++n;
            }
        REQUIRE(n > 0);
        world.OptimizeBroadPhase();
        const json& spawn = it.value().at("spawn");
        const double x = spawn.at("x").get<double>(), z = spawn.at("z").get<double>(), want = spawn.at("surface").get<double>();
        const PhysicsWorld::RayHit h = world.RayCast(Vector3d(x, 500.0, z), Vector3d(0.0, -1.0, 0.0), 1000.0, PhysicsWorld::kLayerStatic);
        REQUIRE(h.hit);
        MESSAGE(it.key() << " (" << n << " objects tagged " << tag << "): spawn y " << spawn.at("y").get<double>() << ", reference surface " << want << ", Jolt " << h.point.y);
        CHECK(std::fabs(h.point.y - want) < 2e-3);
    }
}

TEST_CASE("WP-12c pad: a craft set down on the real launch pad rests on it, landed, and Vessel.landedAt is the pad collider's tag, \"LaunchPad\"") {
    if (!wp20b::GameAvailable()) return;
    for (int level : {0, -1}) {
        AnchorSettings settings;
        World w(true);
        KerbinScene sc(w, PhysicsBubbleOptions{}, GameWorldConfig());
        CelestialBody& kerbin = w.Kerbin();
        const SceneryColliders& doc = Scenery();
        const SceneryCity& ksc = *doc.Find("Kerbin/KSC");
        SceneryLevels lv;
        lv.fallback = level;
        const CityPose pose = KscPose(kerbin);
        SceneryCityHost host(sc.s.bubble, ksc, 0x5343000000000000ull, lv);
        host.PlaceOnBody(kerbin, pose.localPosition, pose.localRotation);
        host.ActivateAll();
        sc.s.world.OptimizeBroadPhase();
        ContactHost& ch = sc.s.bubble.Contacts();
        GameSettings::PHYSICS_EASE = false;
        EventLog log;

        const int spawnIdx = FindPresent(ksc, lv, "LaunchPad_spawn");
        REQUIRE(spawnIdx >= 0);
        const Vector3d spawnLocal = pose.localPosition + pose.localRotation * NodeInCityFrame(ksc, static_cast<size_t>(spawnIdx));
        const Vector3d spawnWorld = kerbin.position + kerbin.rotation * spawnLocal;
        const Vector3d up = (spawnWorld - kerbin.position).normalized();
        // the pad surface under the spawn point, from above
        const PhysicsWorld::RayHit h = sc.s.world.RayCast(sc.s.bubble.StaticPhysicsFromWorld(spawnWorld + up * 40.0), up * -1.0, 80.0, PhysicsWorld::kLayerStatic);
        REQUIRE(h.hit);
        const StaticColliderInfo& hitInfo = ch.StaticInfo(h.userData);
        const Vector3d hitWorld = h.point + sc.s.bubble.FrameOriginWorld();
        MESSAGE("level " << level << ": " << host.BodyCount() << " KSC static bodies, spawn " << ksc.objects[static_cast<size_t>(spawnIdx)].path << ", the ray meets \""
                         << hitInfo.name << "\" tag " << hitInfo.tag << " layer " << hitInfo.layer << " " << (hitWorld - spawnWorld).magnitude() << " m from the spawn point");
        CHECK(hitInfo.tag == "LaunchPad");
        CHECK(hitInfo.layer == 15);
        CHECK(!hitInfo.isPQ);
        CHECK(hitInfo.crashObjectName == "LaunchPad");   // CrashObjectName found upwards from the pad collider (Part.GetComponentUpwards)

        Site site = SiteAt(sc, 0.0, 0.0, true);   // axes at the equator are replaced below
        site.up = up;
        const Vector3d polar = (kerbin.rotation * Vector3d(0.0, 1.0, 0.0)).normalized();
        site.north = (polar - up * Vector3d::Dot(polar, up)).normalized();
        site.east = Vector3d::Cross(up, site.north);
        site.surfaceRadius = (hitWorld - kerbin.position).magnitude();
        auto v = w.CraftHead("PhysProf-S", 1);
        PlaceAt(sc, *v, site, 0.25, false);
        sc.s.Start(*v);
        v->MakeActive();
        ResetLevelClock();
        ch.recordDispatch = true;
        REQUIRE(TickUntilUnpacked(sc, *v));
        std::string firstTag;
        int padEnters = 0;
        for (int k = 0; k < 400; ++k) {
            Tick(sc);
            for (const ContactHost::DispatchRecord& d : ch.LastDispatch())
                if (d.kind == PhysicsWorld::ContactEvent::Kind::Enter && !d.collision.isPQ) {
                    if (firstTag.empty()) firstTag = d.collision.tag;
                    padEnters += d.collision.tag == "LaunchPad" ? 1 : 0;
                }
        }
        VesselPhysics* vp = sc.s.bubble.Host().Find(*v);
        REQUIRE(vp != nullptr);
        const Vector3d com = vp->ReadState().CoMD;
        const double speed = vp->ReadState().rb_velocityD.magnitude();
        // the CoM's height above the pad surface (physics frame, along the vertical) vs the craft's own geometry: CoM above its lowest proxy point
        const double comHeight = Vector3d::Dot(wp29c::VesselCoM(*vp) - h.point, up);
        const double expected = static_cast<double>(v->localCoM.y) - LowestProxyY(*v);
        const double descent = comHeight - expected;
        MESSAGE("level " << level << ": first contact tag \"" << firstTag << "\", " << padEnters << " LaunchPad enters; landed " << v->Landed << ", landedAt \"" << v->landedAt
                         << "\", ground-contact parts " << v->GroundContacts.size() << ", speed " << speed << " m/s, CoM height " << comHeight << " m vs geometry " << expected << " m, explosions " << log.explosions);
        CHECK(firstTag == "LaunchPad");
        CHECK(padEnters > 0);
        CHECK(v->Landed);
        CHECK(v->landedAt == "LaunchPad");
        CHECK(v->GroundContacts.size() > 0);
        CHECK(v->situation == Vessel::Situations::LANDED);
        CHECK(log.explosions == 0);
        CHECK(speed < 0.02);
        CHECK(std::fabs(descent) < 0.05);   // resting on the pad collider (contact offset 1 cm, box proxies), not through it
        // the host's static bodies go with it (no leaked registration)
        const size_t before = sc.s.world.StaticCount();
        CHECK(before >= host.BodyCount());
    }
}

TEST_CASE("WP-12c runway: a rover on the real runway collider, the wheel ray meets it, ModuleWheelBase names the surface (Vessel.landedAt)") {
    if (!wp20b::GameAvailable()) return;
    wp29c::WheelSettingsScope settings;
    World w;
    wp29c::LoadedRover r = wp29c::LoadRover(w, "Prospector Rover");
    // RoverScene's pad scene (its terrain patch lies 1.6 km from the runway); the pad point is moved to the runway before the rover is set down
    wp29c::RoverScene scene(w, wp29c::RoverOptions(r));
    CelestialBody& kerbin = w.Kerbin();
    const SceneryColliders& doc = Scenery();
    const SceneryCity& ksc = *doc.Find("Kerbin/KSC");
    SceneryLevels lv;
    const CityPose pose = KscPose(kerbin);
    SceneryCityHost host(scene.pad.s.bubble, ksc, 0x5343000000000000ull, lv);
    host.PlaceOnBody(kerbin, pose.localPosition, pose.localRotation);
    host.ActivateAll();
    scene.pad.s.world.OptimizeBroadPhase();
    const int spawnIdx = FindPresent(ksc, lv, "SpawnPoint", "End09");
    REQUIRE(spawnIdx >= 0);
    const Vector3d spawnLocal = pose.localPosition + pose.localRotation * NodeInCityFrame(ksc, static_cast<size_t>(spawnIdx));
    const Vector3d spawnWorld = kerbin.position + kerbin.rotation * spawnLocal;
    const Vector3d up = (spawnWorld - kerbin.position).normalized();
    ContactHost& ch = scene.pad.s.bubble.Contacts();
    const PhysicsWorld::RayHit h = scene.pad.s.world.RayCast(scene.pad.s.bubble.StaticPhysicsFromWorld(spawnWorld + up * 40.0), up * -1.0, 80.0, PhysicsWorld::kLayerStatic);
    REQUIRE(h.hit);
    const StaticColliderInfo& hitInfo = ch.StaticInfo(h.userData);
    const Vector3d hitWorld = h.point + scene.pad.s.bubble.FrameOriginWorld();
    MESSAGE("runway spawn " << ksc.objects[static_cast<size_t>(spawnIdx)].path << ": the ray meets \"" << hitInfo.name << "\" tag " << hitInfo.tag);
    CHECK(hitInfo.layer == 15);
    CHECK(hitInfo.tag != "Untagged");
    // the runway point replaces the pad point of the scene (the rover is placed on it), facing east along runway 09
    scene.pad.padWorld = hitWorld;
    scene.pad.padUp = up;
    const Vector3d polar = (kerbin.rotation * Vector3d(0.0, 1.0, 0.0)).normalized();
    scene.pad.padNorth = (polar - up * Vector3d::Dot(polar, up)).normalized();
    scene.pad.padEast = Vector3d::Cross(up, scene.pad.padNorth);
    scene.Place(r, 90.0);
    scene.pad.s.Start(*r.vessel);
    for (int k = 0; k < 400; ++k) scene.Tick();
    Vessel& v = *r.vessel;
    VesselPhysics* vp = scene.pad.s.bubble.Host().Find(v);
    REQUIRE(vp != nullptr);
    size_t grounded = 0, wheels = 0;
    VesselWheels* vw = scene.wheels.Find(v);
    REQUIRE(vw != nullptr);
    for (auto& wh : vw->Wheels()) {
        ++wheels;
        grounded += wh->last.grounded ? 1 : 0;
    }
    MESSAGE("rover: " << grounded << " of " << wheels << " wheels grounded, Landed " << v.Landed << ", landedAt \"" << v.landedAt << "\", speed "
                      << vp->ReadState().rb_velocityD.magnitude());
    CHECK(wheels == 4);
    CHECK(grounded == wheels);
    CHECK(v.Landed);
    CHECK(v.landedAt == hitInfo.tag);
    CHECK(v.landedAt.find("Runway") != std::string::npos);
}

// WP-29d: the runway roll on the real KSC runway collider (WP-12c): the Prospector on the real wheel models rolls east from the 09 threshold at
// full throttle; the line and the heading hold, the wheels report the runway tag (Vessel.landedAt).
TEST_CASE("WP-29d runway roll on the real KSC runway collider (line held, heading held, landedAt Runway)") {
    if (!wp20b::GameAvailable()) return;
    wp29c::WheelSettingsScope settings;
    World w;
    wp29c::LoadedRover r = wp29c::LoadRover(w, "Prospector Rover");
    wp29c::RoverScene scene(w, wp29c::RealGeometry());
    CelestialBody& kerbin = w.Kerbin();
    const SceneryCity& ksc = *Scenery().Find("Kerbin/KSC");
    SceneryLevels lv;
    const CityPose pose = KscPose(kerbin);
    SceneryCityHost host(scene.pad.s.bubble, ksc, 0x5343000000000000ull, lv);
    host.PlaceOnBody(kerbin, pose.localPosition, pose.localRotation);
    host.ActivateAll();
    scene.pad.s.world.OptimizeBroadPhase();
    const int spawnIdx = FindPresent(ksc, lv, "SpawnPoint", "End09");
    REQUIRE(spawnIdx >= 0);
    const Vector3d spawnLocal = pose.localPosition + pose.localRotation * NodeInCityFrame(ksc, static_cast<size_t>(spawnIdx));
    const Vector3d spawnWorld = kerbin.position + kerbin.rotation * spawnLocal;
    const Vector3d up = (spawnWorld - kerbin.position).normalized();
    const PhysicsWorld::RayHit h = scene.pad.s.world.RayCast(scene.pad.s.bubble.StaticPhysicsFromWorld(spawnWorld + up * 40.0), up * -1.0, 80.0, PhysicsWorld::kLayerStatic);
    REQUIRE(h.hit);
    scene.pad.padWorld = h.point + scene.pad.s.bubble.FrameOriginWorld();
    scene.pad.padUp = up;
    const Vector3d polar = (kerbin.rotation * Vector3d(0.0, 1.0, 0.0)).normalized();
    scene.pad.padNorth = (polar - up * Vector3d::Dot(polar, up)).normalized();
    scene.pad.padEast = Vector3d::Cross(up, scene.pad.padNorth);
    scene.Place(r, 90.0);
    scene.pad.s.Start(*r.vessel);
    Vessel& v = *r.vessel;
    for (int k = 0; k < 150; ++k) scene.Tick();
    VesselPhysics* vp = scene.pad.s.bubble.Host().Find(v);
    REQUIRE(vp != nullptr);
    const auto forward = [&] { return Vector3d(v.vesselTransform.rotation * r.craftFwd).normalized(); };
    const Vector3d f0 = forward(), x0 = wp29c::VesselCoM(*vp);
    v.ctrlState.wheelThrottle = 1.0f;
    double maxLateral = 0.0, maxHeading = 0.0, along = 0.0;
    int ticks = 0;
    for (; ticks < 1600 && along < 800.0; ++ticks) {
        wp29c::FillElectricCharge(v);
        scene.Tick();
        vp = scene.pad.s.bubble.Host().Find(v);
        REQUIRE(vp != nullptr);
        const Vector3d d = wp29c::VesselCoM(*vp) - x0;
        along = Vector3d::Dot(d, f0);
        maxLateral = std::max(maxLateral, std::fabs(Vector3d::Dot(Vector3d::Cross(f0, d), up)));
        const Vector3d f = forward();
        maxHeading = std::max(maxHeading, std::fabs(std::atan2(Vector3d::Dot(Vector3d::Cross(f0, f), up), Vector3d::Dot(f0, f))) * 180.0 / 3.14159265358979323846);
    }
    MESSAGE("runway roll (real collider): " << along << " m in " << ticks * 0.02 << " s, speed " << wp29c::VesselVelocity(*vp).magnitude() << " m/s, lateral deviation " << maxLateral
                                            << " m, heading drift " << maxHeading << " deg, landedAt \"" << v.landedAt << "\"");
    CHECK(along > 500.0);
    CHECK(maxLateral < 0.02 * along);
    CHECK(maxHeading < 1.5);
    CHECK(v.landedAt.find("Runway") != std::string::npos);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
// WP-21b: a stock aircraft on the real KSC runway (WP-12c collider) with WP-29d's gear geometry: the Aeris 3A (3 x LY-01 SmallGearBay, one
// turbojet, an intake fuselage, delta wings with elevons) set down at the 09 threshold facing east.
namespace {

const physics::WheelModelLibrary& AeroLibrary() {
    static const physics::WheelModelLibrary lib(std::string(KSPCORE_DATA_DIR) + "/aero_models");
    return lib;
}

// The flight scene's input side (tests/test_flight_input_feed.cpp InputScene): GameSettings at their defaults, HighLogic.LoadedSceneIsFlight, the
// scene's FlightInputHandler (FixedUpdate by the bubble), a sandbox Game (VesselAutopilot's skill rule reads HighLogic.CurrentGame)
struct AircraftInput {
    PhysicsBubble* bubble;
    FlightInputHandler fih;
    Game game;
    explicit AircraftInput(PhysicsBubble& b) : bubble(&b) {
        GameSettings::SetDefaultValues();
        HighLogic::LoadedSceneIsFlight = true;
        FlightDriver::pause = false;
        FlightInputHandler::host = FlightInputHost{};
        Time::deltaTime = Time::fixedDeltaTime;  // Time.deltaTime inside FixedUpdate
        fih.Awake();
        fih.Start();
        game.Mode = Game_Modes::SANDBOX;
        game.CrewRoster = std::make_shared<KerbalRoster>(Game_Modes::SANDBOX);
        SaveEnvironment::Current().currentGame = &game;
        b.SetFlightInputHandler(&fih);
    }
    ~AircraftInput() {
        bubble->SetFlightInputHandler(nullptr);
        fih.OnDestroy();
        FlightInputHandler::fetch = nullptr;
        FlightInputHandler::state = FlightCtrlState();
        FlightInputHandler::host = FlightInputHost{};
        HighLogic::LoadedSceneIsFlight = false;
        SaveEnvironment::Current().currentGame = nullptr;
    }
};

// The stick: an OnFlyByWire callback (Vessel.FeedInputFeed's last callback, the way the C10 capture sets its throttle, HarnessCapture2Flight.cs:332)
// writing the pilot's pitch / yaw / roll after FlightInputHandler's own axis reading (which has no input device here)
struct Pilot {
    float pitch = 0.0f, yaw = 0.0f, roll = 0.0f;
    void Fbw(FlightCtrlState& s) {
        s.pitch = pitch;
        s.yaw = yaw;
        s.roll = roll;
    }
};

struct AircraftRun {
    World w;
    wp29c::LoadedRover r;
    std::unique_ptr<wp29c::RoverScene> scene;
    SceneryLevels lv;
    std::unique_ptr<SceneryCityHost> host;
    std::unique_ptr<AircraftInput> input;
    Pilot pilot;
    Vector3d up, east, north, hit, hitPhys;  // hitPhys: the runway point in the physics frame (the world frame shifts with the floating origin)
    double runwayRadius = 0.0;  // |runway point - Kerbin centre|: the world frame shifts with the floating origin, a radius does not
    explicit AircraftRun(const std::string& craft, double clearance = 0.0, bool airborne = false, double headingDeg = 90.0) {
        r = wp29c::LoadRover(w, craft);
        // the nose direction in the vessel frame: from the engine to the root cockpit, on the ground plane
        {
            Vessel& v = *r.vessel;
            Vector3d ePos, rPos(v.parts.front()->fields.orgPos);
            for (Part* p : v.parts)
                if (p->FindModuleOfClass("ModuleEngines") != nullptr) {
                    ePos = Vector3d(p->fields.orgPos);
                    break;
                }
            const Vector3d u = Vector3d(r.craftUp).normalized();
            Vector3d f = rPos - ePos;
            f = f - u * Vector3d::Dot(f, u);
            r.craftFwd = f.normalized().ToVector3f();
        }
        const auto lookup = AeroLibrary().ModelTransformLookup();
        ModuleResourceIntake::findModelTransform = [lookup](Part& p, const std::string& n) {
            PartTransform t;
            return lookup(p, n, t);
        };
        scene = std::make_unique<wp29c::RoverScene>(w, wp29c::RealGeometry(), AeroLibrary().ColliderHook());
        input = std::make_unique<AircraftInput>(scene->pad.s.bubble);
        PhysicsGlobals::SetApplyDrag(true);  // RoverScene turns the aero off for the rover references; an aircraft flies in the air
        scene->pad.s.bubble.ModuleForces().hooks.modelTransform = lookup;
        CelestialBody& kerbin = w.Kerbin();
        const SceneryCity& ksc = *Scenery().Find("Kerbin/KSC");
        const CityPose pose = KscPose(kerbin);
        host = std::make_unique<SceneryCityHost>(scene->pad.s.bubble, ksc, 0x5343000000000000ull, lv);
        host->PlaceOnBody(kerbin, pose.localPosition, pose.localRotation);
        host->ActivateAll();
        scene->pad.s.world.OptimizeBroadPhase();
        const int spawnIdx = FindPresent(ksc, lv, "SpawnPoint", "End09");
        REQUIRE(spawnIdx >= 0);
        const Vector3d spawnLocal = pose.localPosition + pose.localRotation * NodeInCityFrame(ksc, static_cast<size_t>(spawnIdx));
        const Vector3d spawnWorld = kerbin.position + kerbin.rotation * spawnLocal;
        up = (spawnWorld - kerbin.position).normalized();
        const PhysicsWorld::RayHit h = scene->pad.s.world.RayCast(scene->pad.s.bubble.StaticPhysicsFromWorld(spawnWorld + up * 40.0), up * -1.0, 80.0, PhysicsWorld::kLayerStatic);
        REQUIRE(h.hit);
        hitPhys = h.point;
        hit = h.point + scene->pad.s.bubble.FrameOriginWorld();
        runwayRadius = (hit - kerbin.position).magnitude();
        scene->pad.padWorld = hit;
        scene->pad.padUp = up;
        const Vector3d polar = (kerbin.rotation * Vector3d(0.0, 1.0, 0.0)).normalized();
        north = (polar - up * Vector3d::Dot(polar, up)).normalized();
        east = Vector3d::Cross(up, north);
        scene->pad.padNorth = north;
        scene->pad.padEast = east;
        scene->Place(r, headingDeg, clearance, airborne);
        // the module lifecycle of the non-wheel modules (the wheel host starts its own): Part.ModulesOnStart / ModulesOnStartFinished
        for (Part* p : r.vessel->parts)
            for (size_t i = 0; i < p->Modules().Count(); ++i) {
                PartModule* m = p->Modules().GetModule(i);
                if (dynamic_cast<ModuleWheelBase*>(m) != nullptr) continue;
                m->OnStart(StartState::Landed);
            }
        for (Part* p : r.vessel->parts)
            for (size_t i = 0; i < p->Modules().Count(); ++i) {
                PartModule* m = p->Modules().GetModule(i);
                if (dynamic_cast<ModuleWheelBase*>(m) != nullptr) continue;
                m->OnStartFinished(StartState::Landed);
            }
        r.vessel->OnFlyByWire += Delegate<void(FlightCtrlState&)>::Bind(&pilot, &Pilot::Fbw);
        scene->pad.s.Start(*r.vessel);
    }
    ~AircraftRun() {
        r.vessel.reset();
        ModuleResourceIntake::findModelTransform = nullptr;
        input.reset();
        host.reset();
        scene.reset();
    }
    Vessel& V() { return *r.vessel; }
    VesselPhysics* Vp() { return scene->pad.s.bubble.Host().Find(V()); }
    void Tick() { scene->Tick(); }
    // the distance from the CoM down to the static collider below it (the runway), along the local vertical; -1 when there is none within 100 m
    double GroundBelow(VesselPhysics& vp) {
        const Vector3d com = wp29c::VesselCoM(vp);
        const PhysicsWorld::RayHit h = scene->pad.s.world.RayCast(com + up * 20.0, up * -1.0, 120.0, PhysicsWorld::kLayerStatic);
        return h.hit ? Vector3d::Dot(com - h.point, up) : -1.0;
    }
    Vector3d Fwd() { return Vector3d(V().vesselTransform.rotation * r.craftFwd).normalized(); }
    VesselWheels& Wheels() { return *scene->wheels.Find(V()); }
    // the height of a world point above the runway (the sphere through the runway point: the runway is flat to 0.1 mm over a craft's span)
    double Height(const Vector3d& worldPoint) { return (worldPoint - w.Kerbin().position).magnitude() - runwayRadius; }
    // the lowest tyre bottom (wheel at full extension) above the runway point, along the runway normal
    double LowestTyreBottom() {
        double lo = 1e9;
        for (auto& wh : Wheels().Wheels()) {
            if (wh->Controller() == nullptr || wh->Controller()->wheelCollider == nullptr) continue;
            const auto& g = wh->lastGeometry;
            const auto& wc = wh->Controller()->wheelCollider->wheelColliderData;
            const Vector3d bottom = Vector3d(g.top) - Vector3d(g.up) * (static_cast<double>(wc.suspensionDistance) + static_cast<double>(wc.radius));
            lo = std::min(lo, Height(bottom));
        }
        return lo;
    }
    // the height of the lowest tyre bottom above the runway when the craft is placed at clearance 0 (a measuring scene: Place puts the lowest
    // collider point at the clearance, the tyres hang below it)
    static double MeasureTyreBottom(const std::string& craft) {
        AircraftRun m(craft, 0.0, true);
        for (int k = 0; k < 60; ++k) {
            m.Tick();
            if (k >= 4 && m.Vp() != nullptr && m.Wheels().SetUpCount() == static_cast<int>(m.Wheels().Wheels().size())) break;
        }
        m.Tick();
        const double bottom = m.LowestTyreBottom();
        MESSAGE("measure: vp " << (m.Vp() != nullptr) << " packed " << m.V().packed << " setup " << (m.Vp() ? m.Wheels().SetUpCount() : -1) << " bottom " << bottom);
        REQUIRE(std::fabs(bottom) < 100.0);
        return bottom;
    }
    Vector3d CraftUp() { return Vector3d(V().vesselTransform.rotation * r.craftUp).normalized(); }
};

}  // namespace

TEST_CASE("WP-21b probe: the Aeris 3A roll and rotation") {
    if (!wp20b::GameAvailable()) return;
    wp29c::WheelSettingsScope settings;
    const double bottom0 = AircraftRun::MeasureTyreBottom("Aeris 3A");
    AircraftRun a("Aeris 3A", -bottom0);
    Vessel& v = a.V();
    wp29c::FillElectricCharge(v);
    wp29c::SetBrakes(v, true);
    for (int k = 0; k < 150; ++k) a.Tick();
    wp29c::SetBrakes(v, false);
    wp20d::IgniteFirstStage(v, -1);
    FlightInputHandler::state.mainThrottle = 1.0f;
    ModuleEngines* eng = nullptr;
    for (Part* p : v.parts)
        for (size_t i = 0; i < p->Modules().Count(); ++i)
            if (auto* e = dynamic_cast<ModuleEngines*>(p->Modules().GetModule(i))) eng = e;
    const double thetaT = 10.0 * 3.14159265358979323846 / 180.0;
    Vector3d com0 = wp29c::VesselCoM(*a.Vp());
    const Vector3d f0 = a.Fwd();
    double liftoffT = -1.0, liftoffV = 0.0, thetaPrev = 0.03;
    for (int k = 0; k < 1600; ++k) {
        const double theta = std::asin(std::clamp(Vector3d::Dot(a.Fwd(), a.up), -1.0, 1.0));
        const double q = (theta - thetaPrev) / 0.02;
        thetaPrev = theta;
        a.pilot.pitch = v.srfSpeed > 70.0 ? static_cast<float>(std::clamp(2.0 * (thetaT - theta) - 0.6 * q, -0.5, 0.5)) : 0.0f;
        a.Tick();
        VesselPhysics* vp = a.Vp();
        if (vp == nullptr) continue;
        if (liftoffT < 0.0 && a.Wheels().GroundedCount() == 0) {
            liftoffT = k * 0.02;
            liftoffV = v.srfSpeed;
        }
        if (k % 50 == 0) {
            const Vector3d com = wp29c::VesselCoM(*vp);
            MESSAGE("t " << k * 0.02 << ": along " << Vector3d::Dot(com - com0, f0) << " ground " << a.GroundBelow(*vp) << " srfSpeed " << v.srfSpeed << " vspeed " << v.verticalSpeed << " pitch deg " << theta * 57.29578 << " in " << a.pilot.pitch
                         << " thrust " << eng->finalThrust() << " grounded " << a.Wheels().GroundedCount() << " landed " << v.Landed);
        }
        if (liftoffT > 0.0 && k * 0.02 > liftoffT + 12.0) break;
    }
    MESSAGE("lift-off at " << liftoffT << " s, " << liftoffV << " m/s");
}
