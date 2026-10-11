// WP-42 unit tests: scene flow, KSC, facilities, launch sites (core/include/ksp/space_center).
//
// No golden or capture covers launch sites / the KSC / facilities (harness/golden/MANIFEST.json and harness/captures/out*/ were checked), so
// the gate is tools/wp42/ksc_reference.py -> tests/data/wp42_reference.json: an independent Python implementation (no kspcore imports) of
// the PQSCity placement, SpaceCenter.Start, SpaceCenter / LaunchSite great-circle distances, driven by the stock PQSCity / PQSCity2 prefab
// values (core/data/terrain/pqs_prefab.json) and the real KSC DestructibleBuilding ids of the physprofile save; compared at 0 ULP.
//
// FIXTURES (not KSP data, STATUS.md "Needs data"): the PSystemSetup `facilities` / `stocklaunchsites` arrays (the level scene is not
// extracted: only the names that the physprofile save proves exist, plus the launch-site fields the PQSCity2 prefab carries), the Unity
// scene graph lookups (SceneGraph), the collapse durations / repair costs of DestructibleBuilding (prefab values of the KSC scene).
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/career/career.hpp"
#include "ksp/career/career_environment.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/destructible_building.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/space_center/launch_site.hpp"
#include "ksp/space_center/pqs_city.hpp"
#include "ksp/space_center/psystem_setup.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/space_center/space_center.hpp"
#include "ksp/space_center/space_center_building.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/util/misc_util.hpp"
#include "ksp/vessel/vessel.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using GE42 = events::GameEvents;
using json = nlohmann::json;

namespace {

const std::string kHarness42 = KSPCORE_HARNESS_DIR;
const std::string kSaves42 = std::string(KSPCORE_TESTDATA_DIR) + "/saves";

const json& Ref42() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp42_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

uint64_t Bits42(double d) {
    uint64_t u;
    std::memcpy(&u, &d, 8);
    return u;
}
uint32_t Bits42f(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
// the reference writes NaN as null
double D42(const json& j) { return j.is_null() ? std::nan("") : j.get<double>(); }
bool SameDouble(double a, double b) { return (std::isnan(a) && std::isnan(b)) || Bits42(a) == Bits42(b); }
#define CHECK_D42(got, ref) CHECK_MESSAGE(SameDouble((got), (ref)), "got ", (got), " want ", (ref))
#define CHECK_F42(got, ref) CHECK_MESSAGE(Bits42f(static_cast<float>(got)) == Bits42f(static_cast<float>(ref)), "got ", (got), " want ", (ref))

void LoadLocalizer42() {
    KSP::Localization::Localizer::Shutdown();
    KSP::Localization::Localizer::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(ksp::testing::GameRoot() + "/GameData/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    KSP::Localization::Localizer::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

// one frame of the Unity player loop for the coroutines
void Frame42(CoroutineScheduler& s, float dt = 0.02f) {
    s.AdvanceFrame(dt);
    s.RunDelayedFixedFrameRate();
    s.RunDelayedDynamicFrameRate();
    s.RunEndOfFrame();
}

struct World42 {
    Game15 g;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    Game game;
    World42() {
        FlightRegistry::Clear();
        CareerEnvironment::Reset();
        ScenarioDestructibles::ClearStatics();
        space_center::SceneGraph::Reset();
        SpaceCenterUI::Reset();
        HighLogic::ResetStatics();
        FlightDriver::ResetStatics();
        EditorDriver::ResetStatics();
        SaveEnvironment& env = SaveEnvironment::Current();
        pg.physicsDatabaseFilename = kHarness42 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        game.Mode = Game::Modes::CAREER;
        env.currentGame = &game;
        LoadLocalizer42();
    }
    ~World42() {
        PSystemSetup::UninstallSeams();
        CareerEnvironment::Reset();
        ScenarioDestructibles::ClearStatics();
        space_center::SceneGraph::Reset();
        SpaceCenterUI::Reset();
        HighLogic::ResetStatics();
        FlightDriver::ResetStatics();
        EditorDriver::ResetStatics();
        SaveEnvironment::Current().currentGame = nullptr;
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
    }
    CelestialBody& Body(const std::string& n) { return *sys->GetBodyByName(n); }
};

const PQSCityPrefab& PrefabAt(const std::string& body, int modIndex) {
    for (const PQSCityPrefab& p : StockPQSCities())
        if (body == p.bodyName && p.modIndex == modIndex) return p;
    FAIL("no stock PQSCity " << body << " #" << modIndex);
    return StockPQSCities().front();
}

const PQSCityPrefab& Prefab(const std::string& body, const std::string& name) {
    for (const PQSCityPrefab& p : StockPQSCities())
        if (body == p.bodyName && name == p.name) return p;
    FAIL("no stock PQSCity " << body << "/" << name);
    return StockPQSCities().front();
}

}  // namespace

// ---- PQSCity / PQSCity2 / SpaceCenter / LaunchSite against the independent Python reference ----------------------------------------

// PQSCity.Orientate / PQSCity2.Orientate on every stock city that needs no terrain: local position (float32), lat / lon / alt (double) 0 ULP
TEST_CASE("WP-42 PQSCity / PQSCity2 placement against ksc_reference.py (0 ULP)") {
    World42 w;
    int rows = 0;
    for (const json& r : Ref42()["cities"]) {
        const std::string bodyName = r["body"].get<std::string>(), name = r["name"].get<std::string>();
        const PQSCityPrefab& prefab = PrefabAt(bodyName, r["modIndex"].get<int>());
        PQS pqs;
        pqs.radius = r["radius"].get<double>();
        const auto& local = r["local"];
        if (r["class"].get<std::string>() == "PQSCity") {
            PQSCity city(prefab, &w.Body(bodyName), &pqs);
            city.Orientate();
            CHECK_F42(city.localPosition.x, local[0].get<float>());
            CHECK_F42(city.localPosition.y, local[1].get<float>());
            CHECK_F42(city.localPosition.z, local[2].get<float>());
            CHECK_D42(city.lat, r["lat"].get<double>());
            CHECK_D42(city.lon, r["lon"].get<double>());
            CHECK_D42(city.alt, r["alt"].get<double>());
            // the float32 native FromToRotation * AngleAxis against the double analytic quaternion: a consistency check (the native is
            // verified against the game by WP-11 / WP-17b), compared up to float rounding, either sign
            if (r.contains("rotation_double")) {
                const auto& q = r["rotation_double"];
                const double got[4] = {city.localRotation.x, city.localRotation.y, city.localRotation.z, city.localRotation.w};
                double dpos = 0.0, dneg = 0.0;
                for (int i = 0; i < 4; ++i) {
                    dpos = std::max(dpos, std::fabs(got[i] - q[i].get<double>()));
                    dneg = std::max(dneg, std::fabs(got[i] + q[i].get<double>()));
                }
                CHECK_MESSAGE(std::min(dpos, dneg) < 2e-6, bodyName, "/", name, " rotation differs by ", std::min(dpos, dneg));
            }
        } else {
            PQSCityPrefab p2 = prefab;
            // the pre-snap placement is the reference for the cities that snap to the terrain
            if (r["snap"].get<bool>()) {
                p2.snapToSurface = false;
                p2.setOnWaterSurface = false;
                p2.baseRotFacesNorthPole = false;
            }
            PQSCity2 city(p2, &w.Body(bodyName), &pqs);
            city.Orientate();
            // Moho/Mohole sits at latitude 90: x = cos(pi/2) * radius is ~1.5e-11 m. KSP's Mono runtime evaluates Math.Cos with the x87 fcos
            // (ksp::dotnet::Math, verified against the game's libm capture), whose 66-bit pi reduction gives 6.1230e-17 where the C library
            // cos of the Python reference gives 6.1232e-17: the x component is compared to 1e-9 m, not bit-exact (STATUS.md exclusion)
            const bool polar = std::fabs(std::fabs(r["prefab_lat"].get<double>()) - 90.0) < 1e-9;
            if (polar) {
                // the game's own Math.Cos(pi/2) (harness/captures/out/libm_mono.json, Math.Cos row i=9: 6.123031769111886e-17, bits
                // 3C91A60000000000) is what the port's x87 cos must return; the Python reference's C-library cos differs, hence the tolerance below
                CHECK(Bits42(ksp::dotnet::Math::Cos(1.5707963267948966)) == 0x3C91A60000000000ULL);
                CHECK(std::fabs(city.localPosition.x - local[0].get<float>()) < 1e-9f);
            } else {
                CHECK_F42(city.localPosition.x, local[0].get<float>());
            }
            CHECK_F42(city.localPosition.y, local[1].get<float>());
            CHECK_F42(city.localPosition.z, local[2].get<float>());
            if (!r["snap"].get<bool>()) {
                CHECK_D42(city.lat, r["lat"].get<double>());
                CHECK_D42(city.lon, r["lon"].get<double>());
                CHECK_D42(city.alt, r["alt"].get<double>());
            }
        }
        ++rows;
    }
    CHECK(rows == static_cast<int>(Ref42()["cities"].size()));
    CHECK(rows >= 14);
}

// SpaceCenter.Start on the Kerbin/KSC PQSCity and SpaceCenter.GreatCircleDistance over the lat / lon grid (247 rows, 0 ULP); the same
// distances through the CareerEnvironment seam WP-36's VesselRecovery / RecordsDistance read (PSystemSetup::InstallSeams)
TEST_CASE("WP-42 SpaceCenter KSC position and great-circle distance grid against ksc_reference.py (0 ULP)") {
    World42 w;
    CelestialBody& kerbin = w.Body("Kerbin");
    PQS pqs;
    pqs.radius = Ref42()["spaceCenter"]["radius"].get<double>();
    CHECK(kerbin.Radius == pqs.radius);
    PQSCity ksc(Prefab("Kerbin", "KSC"), &kerbin, &pqs);
    ksc.Orientate();
    SpaceCenter sc;
    sc.cb = &kerbin;
    sc.AttachToCity(ksc);
    sc.Awake();
    CHECK(SpaceCenter::Instance == &sc);
    sc.Start();
    const json& ref = Ref42()["spaceCenter"];
    CHECK_D42(sc.Latitude(), ref["lat"].get<double>());
    CHECK_D42(sc.Longitude(), ref["lon"].get<double>());
    for (int i = 0; i < 3; ++i) CHECK_D42(sc.SrfNVector()[i], ref["srfNVector"][static_cast<size_t>(i)].get<double>());
    PSystemSetup setup;
    setup.Awake();
    PSystemSetup::InstallSeams();
    CareerEnvironment& env = CareerEnvironment::Current();
    CHECK(env.spaceCenterBody() == &kerbin);
    int rows = 0;
    for (const json& r : ref["grid"]) {
        const double lat = r["lat"].get<double>(), lon = r["lon"].get<double>();
        const double want = D42(r["distance"]);
        CHECK_D42(sc.GreatCircleDistance(kerbin.GetRelSurfaceNVector(lat, lon)), want);
        CHECK_D42(env.spaceCenterGreatCircleDistance(lat, lon), want);
        ++rows;
    }
    CHECK(rows == 247);
    // a second SpaceCenter destroys itself (SpaceCenter.cs:38)
    SpaceCenter second;
    second.Awake();
    CHECK(second.destroyed);
    CHECK(SpaceCenter::Instance == &sc);
    // SpaceCenter.OnGameSceneLoaded toggles the area trigger (only the SPACECENTER scene disables it)
    std::vector<bool> trig;
    sc.setAreaTriggerActive = [&](bool a) { trig.push_back(a); };
    GE42::onLevelWasLoaded.Fire(GameScenes::SPACECENTER);
    GE42::onLevelWasLoaded.Fire(GameScenes::FLIGHT);
    CHECK(trig == std::vector<bool>{false, true});
    setup.OnDestroy();
}

// LaunchSite.GreatCircleDistance between the four Making History launch-site PQSCity2 objects of Kerbin (16 ordered pairs, 0 ULP)
TEST_CASE("WP-42 LaunchSite great-circle matrix against ksc_reference.py (0 ULP)") {
    World42 w;
    CelestialBody& kerbin = w.Body("Kerbin");
    PQS pqs;
    pqs.radius = kerbin.Radius;
    std::map<std::string, std::unique_ptr<PQSCity2>> cities;
    std::map<std::string, std::unique_ptr<LaunchSite>> sites;
    for (const json& r : Ref42()["siteMatrix"]) {
        for (const char* k : {"a", "b"}) {
            const std::string n = r[k].get<std::string>();
            if (cities.count(n)) continue;
            cities[n] = std::make_unique<PQSCity2>(Prefab("Kerbin", n), &kerbin, &pqs);
            sites[n] = std::make_unique<LaunchSite>(n, "Kerbin", "#autoLOC_x", std::vector<LaunchSite::SpawnPoint>{}, n, EditorFacility::VAB);
            sites[n]->pqsCity2 = cities[n].get();
        }
    }
    int pairs = 0;
    for (const json& r : Ref42()["siteMatrix"]) {
        const std::string a = r["a"].get<std::string>(), b = r["b"].get<std::string>();
        const PQSCity2& cb = *cities[b];
        const double got = sites[a]->GreatCircleDistance(kerbin.GetRelSurfaceNVector(cb.lat, cb.lon));
        CHECK_D42(got, D42(r["distance"]));
        ++pairs;
    }
    CHECK(pairs == 16);
    // a LaunchSite without a city has no body: double.MinValue (LaunchSite.cs:270)
    LaunchSite orphan("o", "Kerbin", "o", {}, "o", EditorFacility::SPH);
    CHECK(orphan.GreatCircleDistance(Vector3d(1, 0, 0)) == -std::numeric_limits<double>::max());
}

// ---- save round trip: the real KSC DestructibleBuilding ids of the physprofile save -----------------------------------------------------

TEST_CASE("WP-42 DestructibleBuilding Load / Save round trip on the 39 real KSC building nodes (byte-exact)") {
    World42 w;
    ConfigNode::NodePtr save = ConfigNode::Load(kSaves42 + "/physprofile/persistent.sfs");
    REQUIRE(save);
    ConfigNode* scn = nullptr;
    for (ConfigNode* n : save->GetNode("GAME")->GetNodes("SCENARIO"))
        if (n->GetValue("name") && *n->GetValue("name") == "ScenarioDestructibles") scn = n;
    REQUIRE(scn != nullptr);
    const json& ids = Ref42()["destructibles"];
    REQUIRE(ids.size() == 39);
    CHECK(scn->CountNodes() == 39);
    ScenarioDestructibles mod;
    mod.Awake();
    mod.OnLoad(*scn);
    int n = 0;
    std::vector<std::unique_ptr<DestructibleBuilding>> buildings;
    for (const json& r : ids) {
        auto d = std::make_unique<DestructibleBuilding>();
        d->compiledId = r["id"].get<std::string>();
        d->CompileID();
        d->CollapsibleObjects.resize(1);
        d->CollapsibleObjects[0].collapseDuration = 2.0f;
        d->CollapsibleObjects[0].Init();
        d->IsIntact();
        // ScenarioDestructibles.RegisterDestructible finds the saved node and calls DestructibleBuilding.Load
        CHECK(d->RegisterInstance());
        CHECK(d->IsIntact() == (r["intact"].get<std::string>() == "True"));
        ConfigNode out("X");
        d->Save(out);
        CHECK(*out.GetValue("intact") == r["intact"].get<std::string>());
        buildings.push_back(std::move(d));
        ++n;
    }
    CHECK(n == 39);
    CHECK(ScenarioDestructibles::GetFacilityDamage("LaunchPad") == 0.0f);
    // OnSave writes the same nodes back (the state of the live buildings)
    ConfigNode saved("SCENARIO");
    mod.OnSave(saved);
    CHECK(saved.CountNodes() == 39);
    for (const json& r : ids) CHECK(*saved.GetNode(r["id"].get<std::string>())->GetValue("intact") == "True");
    for (auto& b : buildings) b->UnregisterInstance(false);
}

// ---- DestructibleBuilding state machine (DestructibleBuilding.cs:401-:700) ------------------------------------------------------------

TEST_CASE("WP-42 DestructibleBuilding damage, Demolish, collapse and Repair") {
    World42 w;
    CoroutineScheduler& sched = CareerEnvironment::Current().Scheduler();
    SaveEnvironment::Current().loadedSceneIsGame = true;
    REQUIRE(w.game.Parameters != nullptr);
    w.game.Parameters->Difficulty.IndestructibleFacilities = false;   // the default sandbox preset makes facilities indestructible
    DestructibleBuilding d;
    d.compiledId = "SpaceCenter/Test/Facility/building";
    d.CompileID();
    d.FacilityDamageFractionValue = 25.0f;
    d.CollapsibleObjects.resize(2);
    d.CollapsibleObjects[0].collapseBehaviour = DestructibleBuilding::CollapsibleObject::Behaviour::Collapse;
    d.CollapsibleObjects[0].collapseDuration = 1.0f;
    d.CollapsibleObjects[0].repairDuration = 3.0f;
    d.CollapsibleObjects[0].collapseOffset = Vector3f(0.0f, -5.0f, 0.0f);
    d.CollapsibleObjects[0].hasReplacementObject = true;
    d.CollapsibleObjects[0].replaceDelay = 0.5f;   // (replacementObject activates only inside the collapse loop, replaceDelay < collapseDuration)
    d.CollapsibleObjects[1].collapseBehaviour = DestructibleBuilding::CollapsibleObject::Behaviour::Disable;
    d.CollapsibleObjects[1].collapseDuration = 0.5f;
    d.CollapsibleObjects[1].repairDuration = 2.0f;
    for (auto& c : d.CollapsibleObjects) c.Init();
    float now = 0.0f;
    DestructibleBuilding::timeSinceLevelLoad = [&] { return now; };
    std::vector<std::string> events;
    struct Rec {
        std::vector<std::string>* e;
        void a(DestructibleBuilding*) { e->push_back("collapsing"); }
        void b(DestructibleBuilding*) { e->push_back("collapsed"); }
        void c(DestructibleBuilding*) { e->push_back("repairing"); }
        void e2(DestructibleBuilding*) { e->push_back("repaired"); }
    } rec{&events};
    GE42::OnKSCStructureCollapsing.Add(&rec, &Rec::a);
    GE42::OnKSCStructureCollapsed.Add(&rec, &Rec::b);
    GE42::OnKSCStructureRepairing.Add(&rec, &Rec::c);
    GE42::OnKSCStructureRepaired.Add(&rec, &Rec::e2);
    // PhysicsGlobals.BuildingImpactDamageMinDamageFraction (100, PhysicsGlobals.cs:395) gates tiny hits (d > 100); the demolish threshold is
    // impactMomentumThreshold * BuildingToughnessFactor = 375
    d.AddDamage(100.0f);
    CHECK(d.Damage() == 0.0f);
    d.AddDamage(120.0f);
    CHECK(d.IsIntact());
    CHECK(d.Damage() == 120.0f);
    // damageUpdate: damage -= 500 * 0.75 / 5 * fixedDeltaTime per step (Mono float rule)
    float expect = 120.0f;
    for (int i = 0; i < 3; ++i) {
        d.FixedUpdate(0.02f);
        expect = static_cast<float>(static_cast<double>(expect) - 500.0 * static_cast<double>(0.75f) / 5.0 * static_cast<double>(TimeWarp::fixedDeltaTime()));
    }
    CHECK_F42(d.Damage(), expect);
    d.AddDamage(400.0f);
    CHECK_FALSE(d.IsIntact());   // Demolish ran
    CHECK(events == std::vector<std::string>{"collapsing"});
    d.AddDamage(1000.0f);        // already collapsing: no second Demolish
    CHECK(events.size() == 1);
    // the collapse coroutines: object 1 (Disable) is destroyed after 0.5 s, object 0 (Collapse) after its 1 s interpolation; the building
    // reports collapsed once the longest collapseDuration (1 s) elapsed
    for (int i = 0; i < 40 && !d.IsDestroyed(); ++i) {
        now += 0.05f;
        Frame42(sched, 0.05f);
    }
    CHECK(d.IsDestroyed());
    CHECK(events.back() == "collapsed");
    CHECK_FALSE(d.CollapsibleObjects[0].collapseObjectActive);
    CHECK_FALSE(d.CollapsibleObjects[1].collapseObjectActive);
    CHECK(d.CollapsibleObjects[0].replacementObjectActive);
    d.Repair();
    CHECK(events.back() == "repairing");
    for (int i = 0; i < 100 && !d.IsIntact(); ++i) {
        now += 0.05f;
        Frame42(sched, 0.05f);
    }
    CHECK(d.IsIntact());
    CHECK(events.back() == "repaired");
    CHECK(d.CollapsibleObjects[0].collapseObjectActive);
    CHECK_FALSE(d.CollapsibleObjects[0].replacementObjectActive);
    CHECK(d.CollapsibleObjects[0].localPosition.y == 0.0f);   // SetDestroyed(false) restores the start pose
    CHECK(d.GetRepairSequenceDuration() == 3.0f);
    // Load restores intact / destroyed
    ConfigNode node("X");
    node.AddValue("intact", false);
    d.Load(node);
    CHECK_FALSE(d.IsIntact());
    CHECK(d.IsDestroyed());
    d.Reset();
    CHECK(d.IsIntact());
    GE42::OnKSCStructureCollapsing.Remove(&rec, &Rec::a);
    GE42::OnKSCStructureCollapsed.Remove(&rec, &Rec::b);
    GE42::OnKSCStructureRepairing.Remove(&rec, &Rec::c);
    GE42::OnKSCStructureRepaired.Remove(&rec, &Rec::e2);
    DestructibleBuilding::ClearStatics();
}

// ---- HighLogic / scene flow --------------------------------------------------------------------------------------------------------

TEST_CASE("WP-42 HighLogic.LoadScene: events, flags, transition matrix, loaded events") {
    World42 w;
    CoroutineScheduler& sched = CareerEnvironment::Current().Scheduler();
    HighLogic hl;
    hl.Awake();
    CHECK(HighLogic::fetch == &hl);
    HighLogic::SaveFolder("physprofile");
    CHECK(HighLogic::SaveFolder() == "physprofile");
    // flags per scene (HighLogic.cs:300)
    struct Row {
        GameScenes s;
        bool editor, planetarium, flight, game, missionBuilder;
    };
    const Row rows[] = {{GameScenes::MAINMENU, false, false, false, false, false}, {GameScenes::SPACECENTER, false, true, false, true, false},
                        {GameScenes::EDITOR, true, false, false, true, false},    {GameScenes::FLIGHT, false, true, true, true, false},
                        {GameScenes::TRACKSTATION, false, true, false, true, false}, {GameScenes::MISSIONBUILDER, false, false, false, false, true}};
    std::vector<std::pair<int, bool>> loads;
    HighLogic::sceneLoader = [&](int idx, bool buffered) { loads.push_back({idx, buffered}); };
    std::vector<std::string> order;
    struct L {
        std::vector<std::string>* o;
        void req(GameScenes s) { o->push_back("request:" + std::to_string(static_cast<int>(s))); }
        void sw(events::GameEvents::FromToAction<GameScenes, GameScenes> a) { o->push_back("switch:" + std::to_string(static_cast<int>(a.from)) + ">" + std::to_string(static_cast<int>(a.to))); }
        void lvl(GameScenes s) { o->push_back("loaded:" + std::to_string(static_cast<int>(s))); }
        void gui(GameScenes s) { o->push_back("gui:" + std::to_string(static_cast<int>(s))); }
    } l{&order};
    GE42::onGameSceneLoadRequested.Add(&l, &L::req);
    GE42::onGameSceneSwitchRequested.Add(&l, &L::sw);
    GE42::onLevelWasLoaded.Add(&l, &L::lvl);
    GE42::onLevelWasLoadedGUIReady.Add(&l, &L::gui);
    for (const Row& r : rows) {
        HighLogic::LoadScene(r.s);
        CHECK(HighLogic::LoadedScene() == r.s);
        CHECK(HighLogic::LoadedSceneIsEditor == r.editor);
        CHECK(HighLogic::LoadedSceneHasPlanetarium() == r.planetarium);
        CHECK(HighLogic::LoadedSceneIsFlight == r.flight);
        CHECK(HighLogic::LoadedSceneIsGame() == r.game);
        CHECK(HighLogic::LoadedSceneIsMissionBuilder == r.missionBuilder);
    }
    CHECK(loads.size() == 6);
    CHECK(loads[1] == std::make_pair(5, false));
    CHECK(order[0] == "request:2");
    CHECK(order[1] == "switch:2>2");   // MAINMENU (the SaveEnvironment default) to MAINMENU
    CHECK(order[2] == "request:5");
    CHECK(order[3] == "switch:2>5");
    // a transition marked in the matrix goes through the loading buffer (bufferedLoad)
    hl.sceneBufferTransitionMatrix.From[static_cast<size_t>(GameScenes::MISSIONBUILDER)].To[static_cast<size_t>(GameScenes::MAINMENU)] = true;
    loads.clear();
    HighLogic::LoadScene(GameScenes::MAINMENU);
    REQUIRE(loads.size() == 1);
    CHECK(loads[0] == std::make_pair(2, true));
    // Unity raises sceneLoaded; FireLoadedEvent waits one frame, FireLoadedEventGUIReady one frame
    order.clear();
    hl.OnSceneLoaded(5, false);
    CHECK(order.empty());
    Frame42(sched);
    CHECK(order == std::vector<std::string>{"loaded:5", "gui:5"});
    hl.OnSceneLoaded(1, false);   // the loading buffer never raises the loaded events
    Frame42(sched);
    CHECK(order.size() == 2);
    hl.OnSceneLoaded(7, true);    // additive loads are ignored
    Frame42(sched);
    CHECK(order.size() == 2);
    CHECK(HighLogic::GetLoadedGameSceneFromBuildIndex(-1) == HighLogic::LoadedScene());
    CHECK(HighLogic::GetLoadedGameSceneFromBuildIndex(21) == GameScenes::MISSIONBUILDER);
    GE42::onGameSceneLoadRequested.Remove(&l, &L::req);
    GE42::onGameSceneSwitchRequested.Remove(&l, &L::sw);
    GE42::onLevelWasLoaded.Remove(&l, &L::lvl);
    GE42::onLevelWasLoadedGUIReady.Remove(&l, &L::gui);
    hl.OnDestroy();
    CHECK(HighLogic::fetch == nullptr);
}

TEST_CASE("WP-42 FlightDriver / EditorDriver startup state and launch-site validity") {
    World42 w;
    HighLogic hl;
    hl.Awake();
    HighLogic::sceneLoader = [](int, bool) {};
    FlightDriver::StartAndFocusVessel("persistent", 3);
    CHECK(FlightDriver::StartupBehaviour == FlightDriver::StartupBehaviours::RESUME_SAVED_FILE);
    CHECK(FlightDriver::FocusVesselAfterLoad == 3);
    CHECK(HighLogic::LoadedScene() == GameScenes::FLIGHT);
    CHECK_FALSE(FlightDriver::CanRevert());
    FlightDriver::CanRevertToPostInit = true;
    CHECK(FlightDriver::CanRevert());
    auto cache = std::make_shared<Game>();
    FlightDriver::StartAndFocusVessel(cache, 1);
    CHECK(FlightDriver::StartupBehaviour == FlightDriver::StartupBehaviours::RESUME_SAVED_CACHE);
    CHECK(FlightDriver::FlightStateCache == cache);
    CHECK_FALSE(FlightDriver::resumeCacheUsed);
    // FlightDriver.OnDestroy drops a used cache
    FlightDriver fd;
    fd.Awake();
    FlightDriver::resumeCacheUsed = true;
    fd.OnDestroy();
    CHECK(FlightDriver::FlightStateCache == nullptr);
    // SetPause raises the events and records the state
    int paused = 0, unpaused = 0;
    struct P {
        int *a, *b;
        void p() { ++*a; }
        void u() { ++*b; }
    } pp{&paused, &unpaused};
    GE42::onGamePause.Add(&pp, &P::p);
    GE42::onGameUnpause.Add(&pp, &P::u);
    FlightDriver::SetPause(true);
    CHECK(FlightDriver::Pause());
    FlightDriver::SetPause(false);
    CHECK_FALSE(FlightDriver::Pause());
    CHECK((paused == 1 && unpaused == 1));
    GE42::onGamePause.Remove(&pp, &P::p);
    GE42::onGameUnpause.Remove(&pp, &P::u);
    // EditorDriver.StartEditor
    EditorDriver::StartEditor(EditorFacility::SPH);
    CHECK(EditorDriver::editorFacility == EditorFacility::SPH);
    CHECK(HighLogic::LoadedScene() == GameScenes::EDITOR);
    // setupValidLaunchSites: FIXTURE facilities named like the physprofile save's KSC ids ("LaunchPad" VAB, "Runway" SPH) plus the Making
    // History launch sites of the PQSCity2 prefab (requiresPOIVisit, poiName = the real stock values)
    PSystemSetup setup;
    setup.Awake();
    setup.facilities.resize(2);
    setup.facilities[0].name = "LaunchPad";
    setup.facilities[0].editorFacility = EditorFacility::VAB;
    setup.facilities[1].name = "Runway";
    setup.facilities[1].editorFacility = EditorFacility::SPH;
    std::vector<std::unique_ptr<LaunchSite>> mh;
    for (const char* n : {"Crater_Launch_Site", "MahiMahi_Launch_Site"}) {
        const PQSCityPrefab& p = Prefab("Kerbin", n);
        auto ls = std::make_unique<LaunchSite>(n, "Kerbin", n, std::vector<LaunchSite::SpawnPoint>{}, n, EditorFacility::VAB);
        ls->BundleName = "makinghistory_assets";
        ls->requiresPOIVisit = p.requiresPOIVisit;
        ls->poiName = p.poiName;
        setup.launchsites.push_back(ls.get());
        mh.push_back(std::move(ls));
    }
    auto stockSite = std::make_unique<LaunchSite>("Island_Airfield", "Kerbin", "#x", std::vector<LaunchSite::SpawnPoint>{}, "IslandAirfield", EditorFacility::SPH);
    stockSite->BundleName = "stock";
    setup.launchsites.push_back(stockSite.get());
    EditorDriver::editorFacility = EditorFacility::VAB;
    EditorDriver::setupValidLaunchSites();
    CHECK(EditorDriver::ValidLaunchSites() == std::vector<std::string>{"LaunchPad"});   // MakingHistory not installed: no MH sites
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", true);
    REQUIRE(w.game.Parameters != nullptr);
    w.game.Parameters->Difficulty.AllowOtherLaunchSites = true;
    CHECK(EditorDriver::OtherLaunchSitesValid());
    EditorDriver::setupValidLaunchSites();
    CHECK(EditorDriver::ValidLaunchSites() == std::vector<std::string>{"LaunchPad", "Crater_Launch_Site", "MahiMahi_Launch_Site"});
    w.game.Parameters->Difficulty.AllowOtherLaunchSites = false;
    CHECK_FALSE(EditorDriver::OtherLaunchSitesValid());
    EditorDriver::editorFacility = EditorFacility::SPH;
    EditorDriver::setupValidLaunchSites();
    // sandbox / career with a stock-bundle site "": BundleName "stock" passes the second branch
    CHECK(EditorDriver::ValidLaunchSites() == std::vector<std::string>{"Runway", "Island_Airfield"});
    CHECK(EditorDriver::setLaunchSite("Island_Airfield"));
    CHECK(EditorDriver::SelectedLaunchSiteName() == "Island_Airfield");
    CHECK_FALSE(EditorDriver::setLaunchSite("Nowhere"));   // falls back to the default of the facility
    CHECK(EditorDriver::SelectedLaunchSiteName() == "Runway");
    EditorDriver::saveselectedLaunchSite();
    CHECK(w.game.defaultSPHLaunchSite == "Runway");
    // ControlTypes mask of SetInputLockFromGameParameters
    w.game.Parameters->Editor.CanSave = false;
    w.game.Parameters->Editor.CanLaunch = false;
    CHECK(EditorDriver::SetInputLockFromGameParameters() == (EditorDriver::EDITOR_SAVE | EditorDriver::EDITOR_LAUNCH));
    setup.OnDestroy();
    hl.OnDestroy();
}

// ---- PSystemSetup / LaunchSite.Setup / seams ------------------------------------------------------------------------------------------

TEST_CASE("WP-42 PSystemSetup facility and launch-site registry, SetupLaunchSites, CareerEnvironment seams") {
    World42 w;
    CoroutineScheduler& sched = CareerEnvironment::Current().Scheduler();
    CelestialBody& kerbin = w.Body("Kerbin");
    PQS pqs;
    pqs.radius = kerbin.Radius;
    pqs.name = "Kerbin";
    PSystemSetup setup;
    setup.Awake();
    CHECK(PSystemSetup::Instance == &setup);
    PSystemSetup second;
    second.Awake();
    CHECK(PSystemSetup::Instance == &setup);
    setup.pqsArray = {{"Kerbin", &pqs}};
    setup.bodyOfPQS = [&](const space_center::PQSInfo&) { return &kerbin; };
    // FIXTURE scene graph: every transform sits at the origin
    space_center::SceneGraph::FindInPQS = [](const std::string& pqsName, const std::string& path) -> std::optional<Vector3d> {
        if (pqsName == "Kerbin" && (path == "Crater_Launch_Site" || path == "KSC/Runway" || path == "KSC/LaunchPad")) return Vector3d(0, 0, 0);
        return std::nullopt;
    };
    space_center::SceneGraph::FindChild = [](const std::string&, const std::string&, const std::string& child) -> std::optional<Vector3d> {
        if (child == "spawn") return Vector3d(600000.0, 0.0, 0.0);
        return std::nullopt;
    };
    // facilities (names of the real save ids)
    setup.facilities.resize(2);
    setup.facilities[0].name = "LaunchPad";
    setup.facilities[0].pqsName = "Kerbin";
    setup.facilities[0].facilityTransformName = "KSC/LaunchPad";
    setup.facilities[0].facilityDisplayName = "#autoLOC_6001663";
    setup.facilities[0].editorFacility = EditorFacility::VAB;
    setup.facilities[0].spawnPoints.resize(1);
    setup.facilities[0].spawnPoints[0].name = "LaunchPad";
    setup.facilities[0].spawnPoints[0].spawnTransformURL = "spawn";
    setup.facilities[1].name = "Runway";
    setup.facilities[1].pqsName = "Kerbin";
    setup.facilities[1].facilityTransformName = "KSC/Runway";
    setup.facilities[1].facilityDisplayName = "#autoLOC_6001662";
    setup.facilities[1].editorFacility = EditorFacility::SPH;
    setup.SetupFacilities();
    CHECK(setup.facilities[0].facilityName == "LaunchPad");   // the last path component
    CHECK(setup.facilities[0].facilityPQS == &pqs);
    CHECK(setup.facilities[0].hostBody == &kerbin);
    CHECK(setup.facilities[0].IsLaunchFacility());
    CHECK_FALSE(setup.facilities[1].IsLaunchFacility());
    CHECK(setup.spaceCenterFacilityLaunchSites.size() == 1);
    CHECK(setup.GetSpaceCenterFacility("Runway") == &setup.facilities[1]);
    CHECK(setup.GetSpaceCenterFacility("Nope") == nullptr);
    std::string disp;
    bool isFacility = false;
    CHECK(setup.IsFacilityOrLaunchSite("Runway", isFacility, disp));
    CHECK(isFacility);
    CHECK(disp == "#autoLOC_6001662");
    CHECK_FALSE(setup.IsFacilityOrLaunchSite("Nope", isFacility, disp));
    CHECK(setup.GetLaunchSiteBodyName("Runway") == "Kerbin");
    CHECK(setup.GetLaunchSiteBody("Runway") == &kerbin);
    CHECK(setup.GetLaunchSiteDisplayName("Nope") == "Nope");
    // spawn point lat / lon / alt (SetSpawnPointLatLonAlt: GetLatLonAlt of the found transform)
    auto* spawn = setup.facilities[0].GetSpawnPoint("LaunchPad");
    REQUIRE(spawn != nullptr);
    CHECK(setup.facilities[0].GetSpawnPoint("zzz") == nullptr);
    spawn->SetSpawnPointLatLonAlt();
    CHECK(spawn->latlonaltSet);
    double lat = 0, lon = 0, alt = 0;
    kerbin.GetLatLonAlt(Vector3d(600000.0, 0.0, 0.0), lat, lon, alt);
    CHECK_D42(spawn->latitude, lat);
    CHECK_D42(spawn->longitude, lon);
    CHECK_D42(spawn->altitude, alt);

    // a stock launch site bound to a PQSCity2 component (Crater_Launch_Site: the prefab carries requiresPOIVisit / poiName)
    const PQSCityPrefab& cp = Prefab("Kerbin", "Crater_Launch_Site");
    PQSCity2 city2(cp, &kerbin, &pqs);
    city2.sphereSurfaceHeight = [&](const Vector3d&) { return pqs.radius; };   // FIXTURE: a flat terrain
    setup.findPQSCity2 = [&](const std::string&, const std::string& path) -> PQSCity2* { return path == "Crater_Launch_Site" ? &city2 : nullptr; };
    std::vector<std::unique_ptr<LaunchSite>> stock;
    stock.push_back(std::make_unique<LaunchSite>("Crater_Launch_Site", "Kerbin", "#autoLOC_6006117", std::vector<LaunchSite::SpawnPoint>{{"Crater_Launch_Site", "spawn"}},
                                                  "Crater_Launch_Site", EditorFacility::VAB));
    stock.back()->BundleName = "makinghistory_assets";
    stock.back()->requiresPOIVisit = cp.requiresPOIVisit;
    stock.back()->poiName = cp.poiName;
    stock.push_back(std::make_unique<LaunchSite>("Missing_Site", "Kerbin", "#autoLOC_x", std::vector<LaunchSite::SpawnPoint>{}, "Missing", EditorFacility::VAB));
    for (auto& s : stock) setup.stocklaunchsites.push_back(std::move(s));
    std::vector<std::string> hooks;
    setup.sceneHook = [&](const std::string& what, const Vector3d*) { hooks.push_back(what); };
    // MakingHistory not installed: the MH site is skipped, the other one cannot be set up
    {
        auto e = setup.SetupLaunchSites();
        sched.StartCoroutine(std::move(e));
        for (int i = 0; i < 4; ++i) Frame42(sched);
        CHECK(setup.launchsites.empty());
    }
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", true);
    setup.launchsites.clear();
    hooks.clear();
    int orientated = 0;
    struct O {
        int* n;
        void f(CelestialBody*, std::string) { ++*n; }
    } oo{&orientated};
    GE42::OnPQSCityOrientated.Add(&oo, &O::f);
    sched.StartCoroutine(setup.SetupLaunchSites());
    for (int i = 0; i < 6; ++i) Frame42(sched);
    GE42::OnPQSCityOrientated.Remove(&oo, &O::f);
    REQUIRE(setup.launchsites.size() == 1);
    LaunchSite* crater = setup.launchsites[0];
    CHECK(crater->name == "Crater_Launch_Site");
    CHECK(crater->IsSetup());
    CHECK(crater->pqsCity2 == &city2);
    CHECK(city2.launchSite == crater);
    CHECK(city2.positioningCompleted);   // PQSCity2.Orientate ran after the temp FloatingOrigin offset
    CHECK(orientated == 0);              // PQSCity2.Orientate fires no OnPQSCityOrientated (PQSCity.Orientate does)
    CHECK(std::find(hooks.begin(), hooks.end(), "FloatingOrigin.SetOffset") != hooks.end());
    CHECK(crater->spawnPoints[0].latlonaltSet);
    CHECK(setup.GetLaunchSite("Crater_Launch_Site") == crater);
    CHECK(setup.GetLaunchSite("#autoLOC_6006117") == crater);   // by launchSiteName too
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", false);
    CHECK(setup.GetLaunchSite("Crater_Launch_Site") == nullptr);   // a MakingHistory site needs the expansion
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", true);
    CHECK(setup.IsStockLaunchSite("Missing_Site"));
    CHECK_FALSE(setup.IsStockLaunchSite("Nope"));
    CHECK(setup.NonStockLaunchSites().empty());
    CHECK(setup.GetLaunchSiteBody("Crater_Launch_Site") == &kerbin);
    // AddLaunchSite / RemoveLaunchSite
    LaunchSite extra("Extra", "Kerbin", "#autoLOC_extra", {}, "KSC/Runway", EditorFacility::SPH);
    CHECK_FALSE(setup.AddLaunchSite(&extra));   // not set up
    CHECK(setup.AddLaunchSite(&extra, true));
    CHECK_FALSE(setup.AddLaunchSite(&extra, true));   // already exists
    CHECK(setup.NonStockLaunchSites().size() == 1);
    CHECK(setup.RemoveLaunchSite("Extra"));
    CHECK_FALSE(setup.RemoveLaunchSite("Extra"));
    // LaunchSite.Setup failure paths
    {
        LaunchSite ls("L", "Nope", "x", {}, "KSC/Runway", EditorFacility::SPH);
        PQSCity2 c(cp, &kerbin, &pqs);
        CHECK_FALSE(ls.Setup(&c, setup.pqsArray));              // PQS name not found
        LaunchSite ls2("L2", "Kerbin", "x", {}, "NoTransform", EditorFacility::SPH);
        CHECK_FALSE(ls2.Setup(&c, setup.pqsArray));             // transform not found
        CHECK_FALSE(ls2.IsSetup());
        CHECK(ls2.pqsCity2 == &c);
        PQSCity* none = nullptr;
        CHECK_FALSE(ls2.Setup(none, setup.pqsArray));           // "pqsCity cannot be null"
    }
    // CareerEnvironment seams
    SpaceCenter sc;
    sc.cb = &kerbin;
    sc.Awake();
    PSystemSetup::InstallSeams();
    CareerEnvironment& env = CareerEnvironment::Current();
    const auto infos = env.launchSites();
    REQUIRE(infos.size() == 1);
    CHECK(infos[0].name == "Crater_Launch_Site");
    CHECK(infos[0].requiresPOIVisit);
    CHECK(infos[0].body == &kerbin);
    CHECK(infos[0].launchSiteName == "#autoLOC_6006117");
    CHECK(env.isStockLaunchSite("Crater_Launch_Site"));
    CelestialBody* siteBody = nullptr;
    double dist = 0.0;
    CHECK(env.launchSiteDistance("Crater_Launch_Site", siteBody, 10.0, 20.0, dist));
    CHECK(siteBody == &kerbin);
    CHECK_D42(dist, crater->GreatCircleDistance(kerbin.GetRelSurfaceNVector(10.0, 20.0)));
    CHECK_FALSE(env.launchSiteDistance("Nope", siteBody, 10.0, 20.0, dist));
    LaunchSite* found = nullptr;
    struct F {
        LaunchSite** f;
        void h(LaunchSite* s) { *f = s; }
    } ff{&found};
    GE42::LaunchSiteFound.Add(&ff, &F::h);
    env.launchSiteFound("Crater_Launch_Site");
    GE42::LaunchSiteFound.Remove(&ff, &F::h);
    CHECK(found == crater);
    // facility upgradeable ids (scene data fixture) through the getInitialState seam
    setup.facilities[0].upgradeableObjectInParentId = "SpaceCenter/LaunchPad";
    setup.facilities[0].upgradeableFacilityId = "SpaceCenter/LaunchPad";
    setup.facilities[0].upgradeableFacilityMaxLevel = 2;
    const auto fi = env.spaceCenterFacilities();
    REQUIRE(fi.size() == 2);
    CHECK(fi[0].facilityName == "LaunchPad");
    CHECK(fi[0].upgradeableObjectId == "SpaceCenter/LaunchPad");
    CHECK(fi[0].maxLevel == 2);
    CHECK(fi[1].upgradeableObjectId.empty());
    // the scene-mode switches: SetSpaceCentre reverses the home body (PSystemSetup.cs:925)
    setup.kscBody = &kerbin;
    setup.OnLevelLoaded(GameScenes::SPACECENTER);
    CHECK(kerbin.inverseRotation);
    CHECK(kerbin.orbitDriver->reverse);
    CHECK(FlightGlobals::currentMainBody == &kerbin);
    CHECK_FALSE(Planetarium::fetch->pause);
    setup.OnLevelLoaded(GameScenes::EDITOR);
    CHECK(Planetarium::fetch->pause);
    setup.OnSceneChange(GameScenes::FLIGHT);
    CHECK_FALSE(kerbin.inverseRotation);
    CHECK(FlightGlobals::currentMainBody == nullptr);
    sc.OnDestroy();
    setup.OnDestroy();
}

// ---- SpaceCenterBuilding / facility entry ----------------------------------------------------------------------------------------------

TEST_CASE("WP-42 SpaceCenterBuilding damage, repair and facility entry rules") {
    World42 w;
    CoroutineScheduler& sched = CareerEnvironment::Current().Scheduler();
    REQUIRE(w.game.Parameters != nullptr);
    HighLogic hl;
    hl.Awake();
    SaveEnvironment::Current().loadedSceneIsGame = true;
    std::vector<std::string> popups, locks;
    SpaceCenterUI::popup = [&](const std::string& id, const std::string&, const std::string&) { popups.push_back(id); };
    SpaceCenterUI::controlLock = [&](const std::string& id, bool set) { locks.push_back((set ? "+" : "-") + id); };
    int saves = 0;
    SpaceCenterUI::saveGame = [&] { ++saves; };
    DestructibleBuilding a, b;
    a.id = "SpaceCenter/Test/a";
    b.id = "SpaceCenter/Test/b";
    a.FacilityDamageFractionValue = 30.0f;
    b.FacilityDamageFractionValue = 80.0f;
    a.RepairCost = 1000.0f;
    b.RepairCost = 2500.0f;
    a.CollapseReputationHit = 5.0f;
    b.CollapseReputationHit = 10.0f;
    a.CollapsibleObjects.resize(1);
    b.CollapsibleObjects.resize(1);
    a.CollapsibleObjects[0].repairDuration = 1.0f;
    b.CollapsibleObjects[0].repairDuration = 4.0f;
    for (auto* d : {&a, &b}) d->CollapsibleObjects[0].Init();
    VehicleAssemblyBuilding vab;
    vab.facilityName = "VehicleAssemblyBuilding";
    vab.buildingInfoName = "VAB";
    vab.destructibles = {&a, &b};
    CHECK(vab.GetStructureDamage() == 0.0f);
    CHECK(vab.IsFacilityOperational(vab.GetStructureDamage()));
    a.Demolish();
    CHECK_F42(vab.GetStructureDamage(), 30.0f);
    b.Demolish();
    CHECK_F42(vab.GetStructureDamage(), 110.0f);   // not operational at >= 100 (SpaceCenterBuilding.cs:629)
    CHECK_FALSE(vab.IsFacilityOperational(110.0f));
    CHECK(SpaceCenterBuilding::GetStructureDamageLevel(0.0f) == KSP::Localization::Localizer::Format("#autoLOC_7003217"));
    CHECK(SpaceCenterBuilding::GetStructureDamageLevel(20.0f) == KSP::Localization::Localizer::Format("#autoLOC_7003218"));
    CHECK(SpaceCenterBuilding::GetStructureDamageLevel(80.1f) == KSP::Localization::Localizer::Format("#autoLOC_7003221"));
    // nothing is destroyed yet (collapse coroutines pending): RepairCost counts IsDestroyed only
    CHECK(vab.GetRepairsCost() == 0.0f);
    w.game.Parameters->Career.RepLossMultiplier = 2.0f;
    w.game.Parameters->Career.FundsLossMultiplier = 0.5f;
    CHECK_F42(vab.GetCollapseReputationHit(), 30.0f);   // (5 + 10) * 2
    for (int i = 0; i < 30; ++i) Frame42(sched, 0.1f);
    CHECK(a.IsDestroyed());
    CHECK(b.IsDestroyed());
    CHECK_F42(vab.GetRepairsCost(), 1750.0f);   // (1000 + 2500) * 0.5
    // repair: both repair, the longest sequence is 4 s, the save is delayed by num + 0.1 s
    vab.RepairFacility(false);
    CHECK(saves == 0);
    for (int i = 0; i < 20; ++i) Frame42(sched, 0.1f);
    CHECK(saves == 0);
    for (int i = 0; i < 30; ++i) Frame42(sched, 0.1f);
    CHECK(saves == 1);
    CHECK(a.IsIntact());
    CHECK(b.IsIntact());
    CHECK(vab.GetStructureDamage() == 0.0f);
    // entering: the VAB needs CanGoInVAB (otherwise the locked dialog); NON_RESUMABLE scenarios have the building closed
    CHECK(vab.IsOpen());
    EditorDriver::ResetStatics();
    HighLogic::sceneLoader = [](int, bool) {};
    vab.EnterBuilding();
    CHECK(HighLogic::LoadedScene() == GameScenes::EDITOR);
    CHECK(EditorDriver::editorFacility == EditorFacility::VAB);
    CHECK(EditorDriver::StartupBehaviour == EditorDriver::StartupBehaviours::START_CLEAN);
    CHECK(saves == 2);
    w.game.Parameters->SpaceCenter.CanGoInVAB = false;
    popups.clear();
    SaveEnvironment::Current().loadedSceneIsGame = true;
    vab.EnterBuilding();
    CHECK(popups == std::vector<std::string>{"FacilityLocked"});
    CHECK(locks.back() == "+VABFacility");
    w.game.Mode = Game_Modes::SCENARIO_NON_RESUMABLE;
    CHECK_FALSE(vab.IsOpen());
    TrackingStationBuilding ts;
    CHECK_FALSE(ts.IsOpen());
    w.game.Mode = Game_Modes::CAREER;
    CHECK(ts.IsOpen());
    RnDBuilding rnd;
    CHECK_FALSE(rnd.IsOpen());   // ResearchAndDevelopment.Instance == null
    RnDBuilding::rndPresent = [] { return true; };
    CHECK(rnd.IsOpen());
    int rndSpawn = 0;
    struct R {
        int* n;
        void f() { ++*n; }
    } rr{&rndSpawn};
    GE42::onGUIRnDComplexSpawn.Add(&rr, &R::f);
    rnd.EnterBuilding();
    GE42::onGUIRnDComplexSpawn.Remove(&rr, &R::f);
    CHECK(rndSpawn == 1);
    RnDBuilding::rndPresent = nullptr;
    hl.OnDestroy();
}

// ---- SpaceCenter PQSCity.Randomize / GetPQSCitySeed / CelestialBody.GetRandomLatitudeAndLongitude ---------------------------------------

TEST_CASE("WP-42 PQSCity seed and Randomize wiring (KSPRandom draw order)") {
    World42 w;
    CelestialBody& kerbin = w.Body("Kerbin");
    PQS pqs;
    pqs.radius = kerbin.Radius;
    // GetPQSCitySeed (PQSCity.cs:387): (g + h) * (g + h + 1) / 2 + h with int wrap-around and h = GetHashCode_Net35(name + body)
    const int32_t h = StringExtensions::GetHashCode_Net35("RandolithKerbin");
    for (int seed : {0, 1, 12345, -7, 2000000000}) {
        w.game.Seed = seed;
        const int32_t a = static_cast<int32_t>(static_cast<uint32_t>(seed) + static_cast<uint32_t>(h));
        const int32_t prod = static_cast<int32_t>(static_cast<uint32_t>(a) * static_cast<uint32_t>(a + 1));
        CHECK(PQSCity::GetPQSCitySeed("Randolith", "Kerbin") == prod / 2 + h);
    }
    w.game.Seed = 12345;
    PQSCity c(Prefab("Kerbin", "Randolith"), &kerbin, &pqs);
    CHECK(c.randomizeOnSphere);
    c.Randomize();
    CHECK(c.repositionToSphereSurface);
    // restated: KSPRandom(seed) -> lat = asin(1 - 2 u) * 57.29578..., lon = u * 360 - 180 (the first draw is accepted: no ocean test without a PQS
    // controller, no SpaceCenter), then the surface vector times the sphere radius and the final angle = u * 360
    KSPRandom rnd(PQSCity::GetPQSCitySeed("Randolith", "Kerbin"));
    double lat = 0, lon = 0;
    GetRandomLatitudeAndLongitude(kerbin, lat, lon, false, false, &rnd);
    KSPRandom rnd2(PQSCity::GetPQSCitySeed("Randolith", "Kerbin"));
    double lat2 = 0, lon2 = 0;
    GetRandomLatitudeAndLongitude(kerbin, lat2, lon2, false, false, &rnd2);
    CHECK_D42(lat, lat2);
    CHECK_D42(lon, lon2);
    CHECK(lat >= -90.0);
    CHECK(lat <= 90.0);
    CHECK(lon >= -180.0);
    CHECK(lon <= 180.0);
    CelestialFrame cf;
    CelestialFrame::SetFrame(0.0, 0.0, 0.0, cf);
    const Vector3d nv = LatLonGetSurfaceNVector(cf, lat, lon) * pqs.radius;
    CHECK_F42(c.repositionRadial.x, static_cast<float>(nv.x));
    CHECK_F42(c.repositionRadial.y, static_cast<float>(nv.y));
    CHECK_F42(c.repositionRadial.z, static_cast<float>(nv.z));
    CHECK_F42(c.reorientFinalAngle, static_cast<float>(rnd2.NextDouble() * 360.0));
    // with a SpaceCenter on the body, draws within 4 km of it are rejected
    PQSCity ksc(Prefab("Kerbin", "KSC"), &kerbin, &pqs);
    ksc.Orientate();
    SpaceCenter sc;
    sc.cb = &kerbin;
    sc.AttachToCity(ksc);
    sc.Awake();
    sc.Start();
    KSPRandom r3(1);
    for (int i = 0; i < 50; ++i) {
        double la = 0, lo = 0;
        GetRandomLatitudeAndLongitude(kerbin, la, lo, false, false, &r3);
        CHECK(sc.GreatCircleDistance(kerbin.GetRelSurfaceNVector(la, lo)) > 4000.0);
    }
    sc.OnDestroy();
}

// Gate findings F2 / F3 / F5 (PROGRESS-wp42-gate.md): Orientate re-sets the SpaceCenterFacility spawn points (PQSCity.cs:343, PQSCity2.cs:253),
// PQSCity2.Orientate leaves inVisibleRange alone (only setVisibility / Reset write it), and a bundle prefab forces the PQS position only
// when it carries a PQSMod_FlattenArea (PSystemSetup.cs:511-518)
TEST_CASE("WP-42 gate fixes: Orientate calls SpaceCenterFacility.SetSpawnPointsLatLonAlt, inVisibleRange untouched, forcePQSPosition needs FlattenArea") {
    World42 w;
    CelestialBody& kerbin = w.Body("Kerbin");
    PQS pqs;
    pqs.radius = kerbin.Radius;
    pqs.name = "Kerbin";
    {   // PQSCity.Orientate (PQSCity.cs:341-344)
        PQSCity ksc(Prefab("Kerbin", "KSC"), &kerbin, &pqs);
        int calls = 0;
        bool hasTransform = true;
        ksc.spaceCenterFacility.hasFacilityTransform = [&] { return hasTransform; };
        ksc.spaceCenterFacility.setSpawnPointsLatLonAlt = [&] { ++calls; };
        ksc.Orientate();
        CHECK(calls == 1);
        hasTransform = false;   // spaceCenterFacility.facilityTransform == null: not called
        ksc.Orientate();
        CHECK(calls == 1);
        PQSCity bare(Prefab("Kerbin", "KSC"), &kerbin, &pqs);   // spaceCenterFacility == null: no call, no crash
        bare.Orientate();
    }
    {   // PQSCity2.Orientate (PQSCity2.cs:251-254)
        PQSCity2 c2(Prefab("Kerbin", "Crater_Launch_Site"), &kerbin, &pqs);
        c2.sphereSurfaceHeight = [&](const Vector3d&) { return pqs.radius; };
        int calls = 0;
        c2.spaceCenterFacility.hasFacilityTransform = [] { return true; };
        c2.spaceCenterFacility.setSpawnPointsLatLonAlt = [&] { ++calls; };
        c2.inVisibleRange = true;
        c2.Orientate();
        CHECK(calls == 1);
        CHECK(c2.positioningCompleted);
        CHECK(c2.inVisibleRange);   // F3: no C# counterpart for clearing it at the end of Orientate
        c2.Reset();
        CHECK_FALSE(c2.inVisibleRange);   // PQSCity2.Reset (PQSCity2.cs:383) does
    }
    // F5: SetupLaunchSites with a MakingHistory bundle prefab
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", true);
    CoroutineScheduler& sched = CareerEnvironment::Current().Scheduler();
    for (const bool flatten : {false, true}) {
        PSystemSetup setup;
        setup.Awake();
        setup.pqsArray = {{"Kerbin", &pqs}};
        setup.bodyOfPQS = [&](const space_center::PQSInfo&) { return &kerbin; };
        space_center::SceneGraph::FindInPQS = [](const std::string&, const std::string&) -> std::optional<Vector3d> { return Vector3d(0, 0, 0); };
        space_center::SceneGraph::FindChild = [](const std::string&, const std::string&, const std::string&) -> std::optional<Vector3d> {
            return Vector3d(600000.0, 0.0, 0.0);
        };
        PQSCity2 city2(Prefab("Kerbin", "Crater_Launch_Site"), &kerbin, &pqs);
        city2.sphereSurfaceHeight = [&](const Vector3d&) { return pqs.radius; };
        setup.instantiateBundlePrefab = [&](LaunchSite& site, const std::string&) {
            site.pqsCity2 = &city2;
            return true;
        };
        setup.prefabHasFlattenArea = [&](LaunchSite&) { return flatten; };
        auto site = std::make_unique<LaunchSite>("Crater_Launch_Site", "Kerbin", "#autoLOC_6006117",
                                                  std::vector<LaunchSite::SpawnPoint>{{"Crater_Launch_Site", "spawn"}}, "Crater_Launch_Site", EditorFacility::VAB);
        site->BundleName = "makinghistory_assets";
        site->prefabPath = "Crater_Launch_Site.prefab";
        setup.stocklaunchsites.push_back(std::move(site));
        std::vector<std::string> hooks;
        setup.sceneHook = [&](const std::string& what, const Vector3d*) { hooks.push_back(what); };
        sched.StartCoroutine(setup.SetupLaunchSites());
        for (int i = 0; i < 10; ++i) Frame42(sched, 1.0f);
        REQUIRE(setup.launchsites.size() == 1);
        const auto has = [&](const char* h) { return std::find(hooks.begin(), hooks.end(), h) != hooks.end(); };
        CHECK(has("FloatingOrigin.SetOffset"));
        CHECK(has("ForceInitPQS") == flatten);
        CHECK(has("PQS.SetTarget(null)") == flatten);
        CHECK(city2.positioningCompleted);
    }
}
