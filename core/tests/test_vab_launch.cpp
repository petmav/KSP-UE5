// WP-20g item 6: the VAB launch onto the real KSC launch pad (STATUS.md "WP-20g"). C# paths are relative to
// decompiled/KSP-Decompiled/Assembly-CSharp/. FlightDriver.setStartupNewVessel (FlightDriver.cs:272) -> PSystemSetup's LaunchPad spawn point
// (PSystemSetup.cs:40, the facility table and the KSC hierarchy of data/scenery, WP-12c) -> ShipConstruction.PutShipToGround
// (ShipConstruction.cs:521) -> AssembleForLaunch (:296) -> Part.Start -> Vessel.GoOffRails -> CheckGroundCollision (Vessel.cs:2493) -> the
// physics settle and the RB anchor, compared with KSP's own root altitude on the pad (capture C6, harness/captures/out2/thermal_step.json).
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "contact_fixture.hpp"
#include "doctest/doctest.h"
#include "ksp/core/log.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/physics/floating_origin.hpp"
#include "ksp/physics/mu_part_colliders.hpp"
#include "ksp/physics/scenery_host.hpp"
#include "ksp/save/game.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/space_center/pqs_city.hpp"
#include "ksp/space_center/psystem_setup.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/space_center/scenery_colliders.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using namespace ksp::physics;
using json = nlohmann::json;

namespace {

const SceneryColliders& Scenery20g() {
    static const SceneryColliders doc = LoadSceneryColliders(std::string(KSPCORE_DATA_DIR) + "/scenery/scenery_colliders.json");
    return doc;
}

const PQSCityPrefab& KscPrefab20g() {
    for (const PQSCityPrefab& p : StockPQSCities())
        if (std::string(p.bodyName) == "Kerbin" && std::string(p.name) == "KSC") return p;
    FAIL("no stock Kerbin/KSC PQSCity");
    return StockPQSCities().front();
}

double HexDouble(const std::string& hex) {
    const uint64_t bits = std::stoull(hex, nullptr, 16);
    double d = 0.0;
    std::memcpy(&d, &bits, sizeof d);
    return d;
}

// C6 (harness/captures/out2/thermal_step.json, table "vessel", case "pad"): PhysProf-S parked on the KSC launch pad right after its launch
// in a sandbox game (core/testdata/saves/physprofile/persistent.sfs: Mode = SANDBOX, every facility at its top level). `altitude` is
// Vessel.altitude, the vessel transform's (the root part's) altitude (Vessel.cs:2792), the same in all 500 frames
struct C6Pad {
    double altitude = 0.0;
    size_t frames = 0;
    bool constant = true;
};
C6Pad LoadC6Pad() {
    std::ifstream f(std::string(KSPCORE_HARNESS_DIR) + "/captures/out2/thermal_step.json", std::ios::binary);
    REQUIRE(f.good());
    const json doc = json::parse(f);
    C6Pad out;
    for (const json& t : doc.at("tables")) {
        if (t.at("name").get<std::string>() != "vessel") continue;
        std::map<std::string, size_t> col;
        size_t i = 0;
        for (const json& c : t.at("columns")) col[c.at("name").get<std::string>()] = i++;
        for (const json& r : t.at("rows")) {
            if (r.at(col.at("case")).get<std::string>() != "pad") continue;
            const double a = HexDouble(r.at(col.at("altitude_hex")).get<std::string>());
            if (out.frames == 0) out.altitude = a;
            else if (a != out.altitude) out.constant = false;
            ++out.frames;
        }
    }
    return out;
}

// The reduced .mu files of the stock engines (core/tests/data/mucollider, WP-mucol: the colliders of the originals, sha256 in
// mucollider_reference.json) behind MuPartColliders' loader: a part's model path (the GameData mirror's zero-byte placeholder) -> the reduced
// file of the same origin. Parts without one get the drag-cube box (the labelled FIXTURE of MuPartColliders).
std::shared_ptr<const mu::Model> ReducedEngineModel(const std::string& path) {
    static const json ref = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/mucollider_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    std::string p = path;
    std::replace(p.begin(), p.end(), '\\', '/');
    for (auto it = ref.at("parts").begin(); it != ref.at("parts").end(); ++it) {
        const std::string origin = it.value().at("mu").at("origin").get<std::string>();  // "GameData/Squad/Parts/..."
        if (p.size() >= origin.size() && p.compare(p.size() - origin.size(), origin.size(), origin) == 0)
            return std::make_shared<const mu::Model>(mu::ReadModelFile(std::string(KSPCORE_TEST_DATA_DIR) + "/" + it.value().at("reduced").at("file").get<std::string>()));
    }
    return nullptr;
}

// Collider.bounds of a part's colliders in its current pose (world AABB), what ShipConstruction.PutShipToGround reads from the part's
// GetComponentsInChildren<Collider>(): the model reader's job in a host; here the bubble's proxies of the part
std::vector<Bounds> ProxyBounds(const Part& p, const std::vector<ColliderProxy>& proxies) {
    std::vector<Bounds> out;
    for (const ColliderProxy& c : proxies) {
        std::vector<Vector3f> pts;  // part-space points whose hull bounds the collider
        auto add = [&](const Vector3f& local) { pts.push_back(c.center + c.rotation * local); };
        switch (c.kind) {
            case ShapeKind::Box:
                for (int k = 0; k < 8; ++k)
                    add(Vector3f((k & 1) ? c.halfExtents.x : -c.halfExtents.x, (k & 2) ? c.halfExtents.y : -c.halfExtents.y, (k & 4) ? c.halfExtents.z : -c.halfExtents.z));
                break;
            case ShapeKind::ConvexHull:
                if (c.hull)
                    for (const Vector3f& h : *c.hull) add(h);
                break;
            default:  // sphere / capsule: the box around the swept sphere
                for (int k = 0; k < 8; ++k)
                    add(Vector3f((k & 1) ? c.radius : -c.radius, ((k & 2) ? 1.0f : -1.0f) * (c.halfHeight + c.radius), (k & 4) ? c.radius : -c.radius));
                break;
        }
        if (pts.empty()) continue;
        Vector3f lo(3.4e38f, 3.4e38f, 3.4e38f), hi(-3.4e38f, -3.4e38f, -3.4e38f);
        for (const Vector3f& q : pts) {
            const Vector3f w = p.partTransform.TransformPoint(q);
            lo = Vector3f(std::min(lo.x, w.x), std::min(lo.y, w.y), std::min(lo.z, w.z));
            hi = Vector3f(std::max(hi.x, w.x), std::max(hi.y, w.y), std::max(hi.z, w.z));
        }
        out.push_back(Bounds((lo + hi) * 0.5f, hi - lo));
    }
    return out;
}

// Restores the process-wide launch state (FlightDriver's statics, the scene graph, ShipConstruction's environment)
struct LaunchStateGuard {
    ShipConstructionEnvironment savedEnv = ShipConstruction::env;
    ~LaunchStateGuard() {
        FlightDriver::ResetStatics();
        space_center::SceneGraph::Reset();
        ShipConstruction::env = savedEnv;
        ShipConstruction::backups.clear();
        ShipConstruction::ShipConfig = nullptr;
        ShipConstruction::ShipManifest = nullptr;
        ShipConstruction::ShipType = EditorFacility::None;
    }
};

}  // namespace

TEST_CASE("WP-20g VAB launch onto the real KSC launch pad: FlightDriver.setStartupNewVessel, the LaunchPad spawn point, PutShipToGround, AssembleForLaunch, GoOffRails; the root altitude vs C6") {
    if (!wp20b::GameAvailable()) return;
    const C6Pad c6 = LoadC6Pad();
    REQUIRE(c6.frames == 500);
    REQUIRE(c6.constant);
    LaunchStateGuard guard;
    wp20b::World w(true);
    // the bubble with the game's PhysicMaterials (WP-12c) and the real colliders of the stock engines (WP-mucol), drag-cube boxes elsewhere
    MuPartColliders colliders(&ReducedEngineModel);
    PhysicsBubbleOptions bo;
    bo.vessel.colliders = colliders.Hook();
    PhysicsWorldConfig cfg;
    Scenery20g().ApplyTo(cfg);
    wp20e::KerbinScene sc(w, bo, cfg);
    CelestialBody& kerbin = w.Kerbin();

    // the KSC as the flight scene has it: PQSCity.Orientate (WP-42), every facility at its top level (sandbox), the colliders in the bubble
    const SceneryCity& ksc = *Scenery20g().Find("Kerbin/KSC");
    SceneryLevels lv;
    lv.fallback = -1;
    PQS cityPqs;
    cityPqs.radius = kerbin.Radius;
    PQSCity city(KscPrefab20g(), &kerbin, &cityPqs);
    city.Orientate();
    SceneryCityHost kscHost(sc.s.bubble, ksc, 0x5343000000000000ull, lv);
    kscHost.PlaceOnBody(kerbin, Vector3d(city.localPosition), QuaternionD(city.localRotation));
    kscHost.ActivateAll();
    sc.s.world.OptimizeBroadPhase();

    // Unity's Transform.Find over the KSC hierarchy (SceneGraph): the scenery objects' poses through the city transform and Kerbin's
    const std::vector<bool> present = SceneryPresence(ksc, lv);
    const std::vector<SceneryObjectPose> poses = SceneryObjectPoses(ksc);
    auto worldPose = [&](int i, Vector3d& pos, QuaternionD& rot) {
        const QuaternionD cityRot(city.localRotation);
        pos = kerbin.position + kerbin.rotation * (Vector3d(city.localPosition) + cityRot * Vector3d(poses[static_cast<size_t>(i)].position));
        rot = kerbin.rotation * cityRot * QuaternionD(poses[static_cast<size_t>(i)].rotation);
    };
    auto find = [&](const std::string& pqsName, const std::string& path) -> std::optional<Vector3d> {
        if (pqsName != "Kerbin") return std::nullopt;
        const int i = SceneryFindObject(ksc, path, present);
        if (i < 0) return std::nullopt;
        Vector3d pos;
        QuaternionD rot;
        worldPose(i, pos, rot);
        return pos;
    };
    space_center::SceneGraph::FindInPQS = find;
    space_center::SceneGraph::FindChild = [&](const std::string& pqsName, const std::string& parent, const std::string& child) {
        return find(pqsName, parent + "/" + child);
    };
    // PSystemSetup's facility table (the PSystemSetup prefab's `facilities`, data/scenery: WP-12c's extraction)
    PSystemSetup setup;
    setup.Awake();
    REQUIRE(PSystemSetup::Instance == &setup);
    setup.pqsArray = {{"Kerbin", w.pqs.get()}};
    setup.bodyOfPQS = [&](const space_center::PQSInfo&) { return &kerbin; };
    for (const SceneryFacility& f : Scenery20g().facilities) {
        PSystemSetup::SpaceCenterFacility fac;
        fac.name = f.name;
        fac.pqsName = f.pqsName;
        fac.facilityTransformName = f.facilityTransformName;
        fac.facilityDisplayName = f.facilityName;
        fac.editorFacility = static_cast<EditorFacility>(f.editorFacility);
        for (const auto& [name, url] : f.spawnPoints) {
            PSystemSetup::SpaceCenterFacility::SpawnPoint sp;
            sp.name = name;
            sp.spawnTransformURL = url;
            fac.spawnPoints.push_back(sp);
        }
        setup.facilities.push_back(std::move(fac));
    }
    setup.SetupFacilities();
    REQUIRE(setup.GetSpaceCenterFacility("LaunchPad") != nullptr);
    REQUIRE(setup.GetSpaceCenterFacility("LaunchPad")->facilityTransformFound);
    const int spawnIndex = SceneryFindObject(ksc, "KSC/SpaceCenter/LaunchPad/Facility/LaunchPad_spawn", present);
    REQUIRE(spawnIndex >= 0);
    CHECK(ksc.objects[static_cast<size_t>(spawnIndex)].tag == "LaunchPad");  // displaylandedAt is the spawn transform's tag's mini biome

    // FlightDriver.StartWithNewLaunch's statics (FlightDriver.cs:461-470; its SaveGame and the scene load are the host's scene switch)
    Game game;
    ShipConstruction::env.CurrentGame = &game;
    // PhysProf-S has no crew: the manifest's crew assignment (VesselCrewManifest.cs:523, HighLogic.CurrentGame.CrewRoster) has nothing to do
    struct NoCrew : VesselCrewManifest {
        int assigned = 0;
        void AssignCrewToVessel(ShipConstruct&) override { ++assigned; }
    } manifest;
    FlightDriver::StartupBehaviour = FlightDriver::StartupBehaviours::NEW_FROM_FILE;
    FlightDriver::newShipToLoadPath = wp20b::CraftDir() + "/PhysProf-S.craft";
    FlightDriver::newShipFlagURL = "Squad/Flags/default";
    FlightDriver::newShipManifest = &manifest;
    FlightDriver::LaunchSiteName = "LaunchPad";
    std::unique_ptr<Vessel> v;
    Vector3d spawnSeen;
    int placed = 0;
    float lowestBefore = 0.0f;
    // FlightDriver.cs:295 FloatingOrigin.SetOffset(spawn position): the spawn transform at the origin of Unity's world
    FlightDriver::host.placeAtSpawn = [&](const Vector3d& spawn) {
        spawnSeen = spawn;
        ++placed;
        FloatingOrigin::SetOffset(spawn);
    };
    FlightDriver::host.beginNewVessel = [&](ShipConstruct& ship) {
        // the colliders' bounds in the ship's editor pose (the model reader's Part.collider data)
        float lowest = 3.4e38f;
        for (Part* p : ship.liveParts) {
            p->colliderBounds = ProxyBounds(*p, bo.vessel.colliders(*p));
            for (const Bounds& b : p->colliderBounds) lowest = std::min(lowest, b.min().y);
        }
        lowestBefore = lowest;
        // FlightDriver.cs:366-369: PutShipToGround on the spawn transform (its Unity world pose after the offset), AssembleForLaunch
        Vector3d spawnPos;
        QuaternionD spawnRot;
        worldPose(spawnIndex, spawnPos, spawnRot);
        SpawnPoint sp;
        sp.position = spawnPos.ToVector3f();
        sp.rotation = spawnRot.ToQuaternionf();
        ShipConstruction::PutShipToGround(ship, sp);
        Part* root = ship.liveParts[0]->localRoot();
        LaunchPlacement lp;
        lp.rootPosition = root->partTransform.position;
        lp.rootRotation = root->partTransform.rotation;
        v = ShipConstruction::AssembleForLaunch(ship, FlightDriver::LaunchSiteName, "Launch Pad", FlightDriver::newShipFlagURL, game, &manifest, lp);
    };
    // the log lines KSP wrote for this launch in the C6 / C10 capture run (core/testdata/logs/ksp-capture-capture2-final2-KSP.log:4271, :4333)
    struct Log {
        std::vector<std::string> lines;
        Log() { log::SetSink([this](log::Level, std::string_view m) { lines.emplace_back(m); }); }
        ~Log() { log::SetSink(nullptr); }
        std::string Find(const std::string& needle) const {
            for (const std::string& l : lines)
                if (l.find(needle) != std::string::npos) return l;
            return "";
        }
    } logged;
    FlightDriver fd;
    fd.setStartupNewVessel();
    REQUIRE(placed == 1);
    REQUIRE(v != nullptr);
    CHECK(v->parts.size() == 21);
    CHECK(v->landedAt == "LaunchPad");
    CHECK(v->situation == Vessel::Situations::PRELAUNCH);
    CHECK(v->vesselSpawning);
    CHECK(FlightDriver::CanRevertToPrelaunch);
    CHECK(manifest.assigned == 1);
    CHECK(fd.newVessel != nullptr);
    CHECK(fd.newVessel->shipFacility == EditorFacility::VAB);  // the LaunchPad facility's editorFacility (PSystemSetup data)

    // the vessel enters the flight scene (what Unity runs before its first FixedUpdate: Part.Start with the module lifecycle, the Rigidbodies,
    // FlightIntegrator.UpdateMassStats, OrbitDriver.Start; the same statements as wp20e::PlaceAt without its placement)
    StartPartRigidbodies(*v);
    v->UpdateMassStats();
    v->loaded = true;
    v->packed = true;
    kerbin.GetLatLonAlt(Vector3d(v->vesselTransform.position), v->latitude, v->longitude, v->altitude);
    v->srfRelRotation = QuaternionExtensions::Inverse(kerbin.rotation.ToQuaternionf()) * v->vesselTransform.rotation;
    v->localCoM = wp19b::VesselFrameCom(*v).ToVector3f();
    v->lastBody = &kerbin;
    {
        const double ut = Planetarium::GetUniversalTime();
        const Vector3d rel = Vector3d(v->vesselTransform.position) - kerbin.position;
        auto o = std::make_shared<Orbit>();
        o->UpdateFromStateVectors(rel.xzy(), kerbin.getRFrmVel(Vector3d(v->vesselTransform.position)).xzy(), &kerbin, ut);
        v->orbitDriver->orbit = o;
        v->orbitDriver->updateMode = OrbitDriver::UpdateMode::IDLE;
    }
    if (std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), v.get()) == FlightGlobals::Vessels.end()) FlightGlobals::AddVessel(v.get());
    FlightGlobals::SortVessel(v.get(), true);
    StartVesselInFlight(*v);
    v->UpdateResourceSets();
    v->CoMD = Vector3d(v->vesselTransform.position) + QuaternionD(v->vesselTransform.rotation) * Vector3d(v->localCoM);
    v->orbitDriver->Start();
    const double rootAtSpawn = (Vector3d(v->vesselTransform.position) - kerbin.position).magnitude() - kerbin.Radius;
    Vector3d spawnNow;
    QuaternionD spawnRotNow;
    worldPose(spawnIndex, spawnNow, spawnRotNow);
    const double spawnAltitude = (spawnNow - kerbin.position).magnitude() - kerbin.Radius;

    sc.s.Start(*v);
    v->MakeActive();
    wp20e::ResetLevelClock();
    ContactHost& ch = sc.s.bubble.Contacts();
    ch.recordDispatch = true;
    wp20e::EventLog events;
    REQUIRE(wp20e::TickUntilUnpacked(sc, *v));
    const int ignoreFrames = v->IgnoreCollisionsFrames();
    const double rootAfterGoOffRails = (Vector3d(v->vesselTransform.position) - kerbin.position).magnitude() - kerbin.Radius;
    std::string firstTag;
    double minAlt = 1e30, maxAlt = -1e30;
    const int ticks = 600;  // 12 s: the vessel anchors after 1.5 s at rest (Vessel.cs:1105)
    for (int k = 0; k < ticks; ++k) {
        wp20e::Tick(sc);
        for (const ContactHost::DispatchRecord& d : ch.LastDispatch())
            if (d.kind == PhysicsWorld::ContactEvent::Kind::Enter && firstTag.empty()) firstTag = d.collision.tag;
        if (k >= ticks - 100) {
            const double a = (Vector3d(v->vesselTransform.position) - kerbin.position).magnitude() - kerbin.Radius;
            minAlt = std::min(minAlt, a);
            maxAlt = std::max(maxAlt, a);
        }
    }
    VesselPhysics* vp = sc.s.bubble.Host().Find(*v);
    REQUIRE(vp != nullptr);
    const double speed = vp->ReadState().rb_velocityD.magnitude();
    const double rootAltitude = (Vector3d(v->vesselTransform.position) - kerbin.position).magnitude() - kerbin.Radius;
    const double diff = rootAltitude - c6.altitude;
    MESSAGE("VAB launch of PhysProf-S onto the LaunchPad (top level): spawn transform at " << spawnAltitude << " m (" << spawnNow.magnitude()
            << " m from Unity's origin after FloatingOrigin.SetOffset), the ship's lowest collider bound "
            << lowestBefore << " m in the editor; root after PutShipToGround " << rootAtSpawn << " m, after GoOffRails / CheckGroundCollision " << rootAfterGoOffRails
            << " m (ignore-collision frames " << ignoreFrames << "); real colliders on " << colliders.stats().realParts << " part calls");
    MESSAGE("after " << ticks << " ticks: first contact tag \"" << firstTag << "\", landedAt \"" << v->landedAt << "\", landed " << v->Landed << ", situation "
            << static_cast<int>(v->situation) << ", speed " << speed << " m/s, explosions " << events.explosions << "; root altitude " << rootAltitude
            << " m (last 100 ticks " << minAlt << " - " << maxAlt << ") vs C6 " << c6.altitude << " m: " << diff << " m");
    // ShipConstruction.PutShipToGround's lowest collider point (ShipConstruction.cs:540, float.ToString()): the real Mainsail collider's bound in
    // the editor pose gives KSP's own number, digit for digit
    const std::string put = logged.Find("putting ship to ground: ");
    const std::string moved = logged.Find("ground contact! - error. Moving Vessel");
    MESSAGE("ours: \"" << put << "\" / \"" << moved << "\"; KSP.log: \"putting ship to ground: -6.869718\" / \"[PhysProf-S]: ground contact! - error. Moving Vessel  down -0.101m\"");
    CHECK(put == "putting ship to ground: -6.869718");
    CHECK(moved.rfind("[PhysProf-S]: ground contact! - error. Moving Vessel  down -0.", 0) == 0);  // moved down by a decimetre as KSP (the cm: see STATUS)
    CHECK(firstTag == "LaunchPad");
    CHECK(v->Landed);
    CHECK(v->landedAt == "LaunchPad");
    CHECK(v->situation == Vessel::Situations::PRELAUNCH);
    CHECK(events.explosions == 0);
    CHECK(v->parts.size() == 21);
    CHECK(speed < 0.01);
    CHECK(maxAlt - minAlt < 1e-3);
    // KSP composes the KSC's world pose in Unity's float32 Transform hierarchy with Kerbin's transform ~600 km from the floating origin
    // (one float ulp there is 0.0625 m, rounding up to 0.031 m per component); ours is composed in double. With Jolt's resting penetration
    // (<= 0.02 m, STATUS WP-20d) that bounds the expected difference at 0.05 m. The capture value is KSP's, independent of every input here.
    CHECK(std::fabs(diff) < 0.05);
}
