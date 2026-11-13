// WP-12d item 3: the facility level at run time (ScenarioUpgradeableFacilities -> UpgradeableFacility / UpgradeableSlave -> the KSC
// scenery colliders) and the destruction state (ScenarioDestructibles / DestructibleBuilding -> the collapse / replacement objects), on the
// KSC of data/scenery. Gate: tests/data/wp12d_facility_reference.json from tools/wp12d/facility_reference.py (independent Python: the level
// rules and Unity's activation over the KSC hierarchy): the set of scenery objects in the physics scene, object by object.
#include <cmath>
#include <fstream>
#include <memory>
#include <set>
#include <string>

#include "contact_fixture.hpp"
#include "ksp/career/career_environment.hpp"
#include "ksp/career/scenario_misc.hpp"
#include "ksp/physics/ksc_facilities.hpp"
#include "ksp/space_center/scenery_colliders.hpp"
#include "nlohmann/json.hpp"
#include "scatter_scene.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp20e;
using nlohmann::json;

namespace {

const SceneryColliders& Scenery12d() {
    static const SceneryColliders doc = LoadSceneryColliders(std::string(KSPCORE_DATA_DIR) + "/scenery/scenery_colliders.json");
    return doc;
}

void Frames(int n) {
    CoroutineScheduler& s = CareerEnvironment::Current().Scheduler();
    for (int i = 0; i < n; ++i) {
        s.AdvanceFrame(0.02f);
        s.RunDelayedFixedFrameRate();
        s.RunDelayedDynamicFrameRate();
        s.RunEndOfFrame();
    }
}

// the scenario modules and the scene of one case (statics cleared on both ends)
struct Scenarios {
    ScenarioUpgradeableFacilities upgradeables;
    ScenarioDestructibles destructibles;
    Scenarios() {
        ScenarioUpgradeableFacilities::ClearStatics();
        ScenarioDestructibles::ClearStatics();
        upgradeables.OnAwake();
        destructibles.OnAwake();
        SaveEnvironment::Current().loadedScene = GameScenes::SPACECENTER;
    }
    ~Scenarios() {
        upgradeables.OnDestroy();
        destructibles.OnDestroy();
        ScenarioUpgradeableFacilities::ClearStatics();
        ScenarioDestructibles::ClearStatics();
        DestructibleBuilding::timeSinceLevelLoad = nullptr;
        SaveEnvironment::Current().loadedScene = GameScenes::MAINMENU;
    }
};

std::map<std::string, std::vector<std::string>> Slaves(const json& ref) {
    std::map<std::string, std::vector<std::string>> out;
    for (const auto& [g, n] : ref.at("slaves").items()) out[g] = n.get<std::vector<std::string>>();
    return out;
}

float FromBits(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

}  // namespace

TEST_CASE_FIXTURE(wp12d::StateGuard, "WP-12d KSC facility levels and destroyed buildings -> scenery colliders vs tools/wp12d/facility_reference.py (every object)") {
    if (!wp20b::GameAvailable()) return;
    const json ref = json::parse(std::ifstream(std::string(KSPCORE_TEST_DATA_DIR) + "/wp12d_facility_reference.json"));
    REQUIRE(ref.at("schema") == "kspcore-wp12d-facilities/1");
    const SceneryCity& ksc = *Scenery12d().Find("Kerbin/KSC");
    size_t objects = 0, mismatches = 0;
    for (const json& sc : ref.at("scenarios")) {
        World w(true);
        KerbinScene scene(w);
        Scenarios scen;
        ConfigNode fac("SCENARIO");
        for (const auto& [g, bits] : sc.at("lvl").items()) fac.AddNode(g)->AddValue("lvl", FromBits(bits.get<uint32_t>()));
        scen.upgradeables.OnLoad(fac);
        ConfigNode des("SCENARIO");
        for (const json& id : sc.at("destroyed")) des.AddNode(id.get<std::string>())->AddValue("intact", false);
        scen.destructibles.OnLoad(des);
        SceneryCityHost host(scene.s.bubble, ksc, 0x5343000000000000ull);
        host.SetPose(Vector3d(0.0, -5000.0, 0.0), QuaternionD::identity());
        KSCFacilityHost::Options opt;
        opt.slaveNeighbours = Slaves(ref);
        KSCFacilityHost fh(host, opt);
        fh.Awake();
        fh.Start();
        events::GameEvents::onLevelWasLoaded.Fire(GameScenes::SPACECENTER);   // UpgradeableFacility.LoadOnSceneStart -> RegisterInstance
        Frames(6);                                                              // end of frame; the slaves' DelayedCallback(3)
        for (const auto& [g, n] : sc.at("upgrade").items()) fh.Facility(g)->SetLevel(n.get<int>());
        Frames(4);
        fh.Sync();
        host.ActivateAll();
        // the levels
        for (const auto& [g, lvl] : sc.at("levels").items()) {
            INFO(sc.at("name").get<std::string>() << " " << g);
            REQUIRE(fh.Group(g) != nullptr);
            CHECK(fh.Group(g)->FacilityLevel() == lvl.get<int>());
            CHECK(host.levels().level.at(g) == lvl.get<int>());
        }
        // the objects in the physics scene
        const std::vector<size_t> want = sc.at("placed").get<std::vector<size_t>>();
        const std::set<size_t> wantSet(want.begin(), want.end());
        size_t mis = 0;
        for (size_t i = 0; i < ksc.objects.size(); ++i)
            if (host.set().IsObjectActive(i) != (wantSet.count(i) != 0)) {
                if (mis < 8) MESSAGE("  object " << i << " " << ksc.objects[i].path << ": in the scene " << host.set().IsObjectActive(i) << ", activeSelf "
                                             << host.set().ActiveSelf(i) << ", present " << host.set().Present(i));
                ++mis;
            }
        // destroyed buildings: the ids were live at their level and Loaded as destroyed
        for (const json& id : sc.at("destroyed")) {
            DestructibleBuilding* b = fh.Building(id.get<std::string>());
            REQUIRE(b != nullptr);
            CHECK(b->IsDestroyed());
        }
        MESSAGE(sc.at("name").get<std::string>() << ": " << want.size() << " objects in the scene (" << host.BodyCount() << " static bodies), "
                                                 << fh.LiveBuildings() << " live DestructibleBuildings, " << mis << " mismatches");
        mismatches += mis;
        objects += ksc.objects.size();
    }
    CHECK(objects == 4 * ksc.objects.size());
    CHECK(mismatches == 0);
}

TEST_CASE_FIXTURE(wp12d::StateGuard, "WP-12d KSC run time: an upgraded launch pad swaps its colliders; a collapsing building's colliders sink with the animation and go; repair brings them back") {
    if (!wp20b::GameAvailable()) return;
    const SceneryCity& ksc = *Scenery12d().Find("Kerbin/KSC");
    World w(true);
    KerbinScene scene(w);
    Scenarios scen;
    ConfigNode fac("SCENARIO");
    fac.AddNode("SpaceCenter/LaunchPad")->AddValue("lvl", 0.0f);
    scen.upgradeables.OnLoad(fac);
    SceneryCityHost host(scene.s.bubble, ksc, 0x5343000000000000ull);
    host.SetPose(Vector3d(0.0, -5000.0, 0.0), QuaternionD::identity());
    KSCFacilityHost::Options opt;
    opt.collapseDuration = 2.0f;   // FIXTURE: the DestructibleBuilding prefab values are not extracted
    opt.repairDuration = 1.0f;
    opt.replaceDelay = 0.5f;
    KSCFacilityHost fh(host, opt);
    fh.Awake();
    fh.Start();
    events::GameEvents::onLevelWasLoaded.Fire(GameScenes::SPACECENTER);
    Frames(6);
    fh.Sync();
    host.ActivateAll();
    // the pad spawn transform of the shown level and the pad surface under it
    auto padObjects = [&] {
        std::set<size_t> out;
        for (size_t i = 0; i < ksc.objects.size(); ++i)
            if (host.set().IsObjectActive(i) && ksc.objects[i].path.find("SpaceCenter/LaunchPad/") != std::string::npos) out.insert(i);
        return out;
    };
    CHECK(fh.Facility("SpaceCenter/LaunchPad")->FacilityLevel() == 0);
    const std::set<size_t> level0 = padObjects();
    const size_t bodies0 = host.BodyCount();
    fh.Facility("SpaceCenter/LaunchPad")->SetLevel(2);   // UpgradeableFacility.SetLevel (OnKSCFacilityUpgrading, setLevel)
    Frames(3);
    fh.Sync();
    const std::set<size_t> level2 = padObjects();
    CHECK(fh.Facility("SpaceCenter/LaunchPad")->FacilityLevel() == 2);
    CHECK(host.levels().level.at("SpaceCenter/LaunchPad") == 2);
    size_t shared = 0;
    for (size_t i : level0) shared += level2.count(i);
    MESSAGE("LaunchPad level 0: " << level0.size() << " objects, level 2: " << level2.size() << " (" << shared << " in both); "
                                  << bodies0 << " -> " << host.BodyCount() << " KSC static bodies");
    CHECK(!level2.empty());
    CHECK(shared < level0.size());
    for (size_t i : level2)
        if (!ksc.objects[i].levelGroup.empty()) CHECK(ksc.objects[i].level == 2);

    // a building of a shown level collapses (collapseBehaviour 1: the collapse animation): its colliders move by the offset, then go
    const SceneryDestructible* rec = nullptr;
    for (const SceneryDestructible& d : ksc.destructibles)
        if (!d.id.empty() && fh.Building(d.id) != nullptr && !d.collapsibles.empty() && d.collapsibles[0].collapseBehaviour == 1 &&
            !ksc.objects[d.collapsibles[0].collapse].colliders.empty() && d.collapsibles[0].collapseOffset.y < 0.0f)
            rec = &d;
    REQUIRE(rec != nullptr);
    const size_t obj = rec->collapsibles[0].collapse;
    const uint64_t key = host.KeyOf(obj, 0);
    REQUIRE(scene.s.world.StaticBody(key) != kInvalidBody);
    const Vector3d before = scene.s.world.GetPosition(scene.s.world.StaticBody(key));
    CoroutineScheduler& sched = CareerEnvironment::Current().Scheduler();
    DestructibleBuilding::timeSinceLevelLoad = [&sched] { return static_cast<float>(sched.CurTime()); };
    DestructibleBuilding* b = fh.Building(rec->id);
    b->Demolish();
    Frames(50);   // 1 s of the 2 s collapse
    fh.Sync();
    REQUIRE(scene.s.world.StaticBody(key) != kInvalidBody);
    const Vector3d mid = scene.s.world.GetPosition(scene.s.world.StaticBody(key));
    const double sunk = (before - mid).magnitude();
    // XLerp(0, 1, t = 0.5, 3) of the offset |(0, -5, 0)| x the object's scale chain: between 0 and the whole offset
    MESSAGE(rec->id << " collapsing: collider moved " << sunk << " m after 1 s (offset " << rec->collapsibles[0].collapseOffset.y << " m local)");
    CHECK(sunk > 0.1);
    Frames(60);
    fh.Sync();
    CHECK(b->IsDestroyed());
    CHECK(scene.s.world.StaticBody(key) == kInvalidBody);
    CHECK_FALSE(host.set().IsObjectActive(obj));
    // the LOD / transforms callbacks re-activating the city do not resurrect it (activeSelf stays false)
    host.Deactivate();
    host.ActivateAll();
    CHECK_FALSE(host.set().IsObjectActive(obj));
    if (rec->collapsibles[0].replacement >= 0) CHECK(host.set().IsObjectActive(static_cast<size_t>(rec->collapsibles[0].replacement)));
    // repair: back at the original place
    b->Repair();
    Frames(60);
    fh.Sync();
    CHECK(b->IsIntact());
    REQUIRE(scene.s.world.StaticBody(key) != kInvalidBody);
    CHECK((scene.s.world.GetPosition(scene.s.world.StaticBody(key)) - before).magnitude() < 1e-6);
}
