// WP-20g: the host wiring of a flyable headless core (STATUS.md "WP-20g"). C# paths are relative to
// decompiled/KSP-Decompiled/Assembly-CSharp/.
//   * Part.Pack / Part.Unpack (Part.cs:3747 / :3784) of every part on Vessel.GoOnRails / GoOffRails (Vessel.cs:2356 / :2395) in the bubble,
//     the GameObject.SendMessage receivers in module order, onPartPack / onPartUnpack, Vessel.SendMessage("OnVesselPack" / "OnVesselUnpack");
//   * GameSettings.Awake (GameSettings.cs:1506): KSP's defaults, the shipped keyboard layouts, settings.cfg; every case starts at the defaults;
//   * the module start-up of a vessel entering the flight scene (StartVesselInFlight: Part.cs:1727 / :1945, ModuleJettison.cs:474);
//   * the flight scene's resource-flow host (VesselResourceHost: Vessel.cs:451-464, :3913, :3980, :3994);
//   * the VAB launch onto the real KSC launch pad: tests/test_vab_launch.cpp.
// The servo slot (BaseServo FixedUpdate / Update in PartModuleForces) is gated in tests/test_servo_scenarios.cpp ("WP-52b in the physics bubble").
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "contact_fixture.hpp"
#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/modules/module_structural.hpp"
#include "ksp/modules/parachutes.hpp"
#include "ksp/modules/resource_processing.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/vessel_resource_host.hpp"

using namespace ksp;
using namespace ksp::physics;
namespace fs = std::filesystem;

namespace {

using GE = events::GameEvents;

// onPartPack / onPartUnpack in firing order
struct PackLog {
    std::vector<Part*> packed, unpacked;
    PackLog() {
        GE::onPartPack.Add(this, &PackLog::OnPack);
        GE::onPartUnpack.Add(this, &PackLog::OnUnpack);
    }
    ~PackLog() {
        GE::onPartPack.Remove(this, &PackLog::OnPack);
        GE::onPartUnpack.Remove(this, &PackLog::OnUnpack);
    }
    void OnPack(Part* p) { packed.push_back(p); }
    void OnUnpack(Part* p) { unpacked.push_back(p); }
};

// The Assembly-CSharp receivers of SendMessage("OnPartPack" / "OnPartUnpack") by C# class name, independent of the dispatcher's dynamic_cast
// chain: ModuleCargoPart.cs:80 / :84 (ModuleGroundPart.cs:144 / :156 override them), ModuleDockingNode.cs:681 / :689, BaseServo.cs:1177 /
// :1182 (the four servos), ModuleParachute.cs:811 (OnPartUnpack only). ModuleJointMotor.cs:170 has no kspcore port (no stock part uses it).
size_t ExpectedReceivers(const Part& p, bool unpack) {
    static const std::set<std::string> both = {"ModuleCargoPart",          "ModuleGroundPart",           "ModuleDockingNode",
                                               "ModuleRoboticServoHinge",  "ModuleRoboticRotationServo", "ModuleRoboticServoPiston",
                                               "ModuleRoboticServoRotor"};
    size_t n = 0;
    for (size_t i = 0; i < const_cast<Part&>(p).Modules().Count(); ++i) {
        const std::string& c = const_cast<Part&>(p).Modules().GetModule(i)->ClassName();
        if (both.count(c) != 0 || (unpack && c == "ModuleParachute")) ++n;
    }
    return n;
}

}  // namespace

// ===================================================================================================================== item 1

TEST_CASE("WP-20g Part.Pack / Part.Unpack of every part on Vessel.GoOffRails / GoOnRails in the bubble (Part.cs:3747 / :3784, Vessel.cs:2356 / :2395)") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w(true);
    wp20d::PadScene pad(w);
    PackLog log;
    auto v = w.Craft("PhysProf-S");
    pad.Place(*v);
    const size_t n = v->parts.size();
    REQUIRE(n == 21);
    for (Part* p : v->parts) CHECK(p->fields.packed);  // the craft loader leaves every part packed (ShipConstruct.cs:670-678)
    pad.s.Start(*v);
    REQUIRE(wp20e::TickUntilUnpacked(pad, *v));
    // GoOffRails: Part.Unpack of every part in vessel order, each firing onPartUnpack once (Vessel.cs:2449-2451, Part.cs:3808)
    REQUIRE(log.unpacked.size() == n);
    for (size_t i = 0; i < n; ++i) CHECK(log.unpacked[i] == v->parts[i]);
    for (Part* p : v->parts) CHECK_FALSE(p->fields.packed);
    CHECK(log.packed.empty());
    CHECK(pad.s.bubble.Host().Find(*v) != nullptr);  // the host built the vessel's bodies (Part.Unpack's rb.isKinematic = false)
    for (int k = 0; k < 5; ++k) pad.s.Tick();
    // GoOnRails: Part.Pack of every part (Vessel.cs:2373-2377): onPartPack per part, Part.packed, the bodies dropped by the host
    v->GoOnRails();
    CHECK(v->packed);
    REQUIRE(log.packed.size() == n);
    for (size_t i = 0; i < n; ++i) CHECK(log.packed[i] == v->parts[i]);
    for (Part* p : v->parts) CHECK(p->fields.packed);
    CHECK(pad.s.bubble.Host().Find(*v) == nullptr);
    // a packed vessel does not pack again (Vessel.cs:2358), nor a packed part (Part.cs:3749)
    v->GoOnRails();
    for (Part* p : v->parts) p->Pack();
    CHECK(log.packed.size() == n);
    // back off the rails through the scene (OrbitPhysicsManager.LateUpdate, OrbitPhysicsManager.cs:107): every part unpacked again
    REQUIRE(wp20e::TickUntilUnpacked(pad, *v, 50));
    CHECK(log.unpacked.size() == 2 * n);
    for (Part* p : v->parts) CHECK_FALSE(p->fields.packed);
    CHECK(pad.s.bubble.Host().Find(*v) != nullptr);
    MESSAGE("PhysProf-S: " << log.unpacked.size() << " Part.Unpack, " << log.packed.size() << " Part.Pack over GoOffRails / GoOnRails / GoOffRails");
}

TEST_CASE("WP-20g SendMessage receivers of Part.Pack / Unpack in module order, Vessel.SendMessage on the root part only, Pack of a DEAD part") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w;
    // stock crafts with docking ports, parachutes, cargo parts and launch clamps (harness/inputs/Ships: the game's own Ships folder)
    const LaunchClamp::Hooks savedClamp = LaunchClamp::hooks;
    int clampSet = 0, clampUnset = 0;
    LaunchClamp::hooks.setJoint = [&](Part&) { ++clampSet; };
    LaunchClamp::hooks.unsetJoint = [&](Part&) { ++clampUnset; };
    size_t receiversPack = 0, receiversUnpack = 0, parachutes = 0, clamps = 0;
    size_t loaded = 0, refused = 0;
    std::vector<fs::path> crafts;
    for (const char* facility : {"VAB", "SPH"})
        for (const auto& e : fs::directory_iterator(fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / facility))
            if (e.path().extension() == ".craft") crafts.push_back(e.path());
    std::sort(crafts.begin(), crafts.end());
    for (const fs::path& craft : crafts) {
        CAPTURE(craft.filename().string());
        std::unique_ptr<Vessel> v;
        try {
            v = w.Craft(craft.string());
        } catch (const std::exception&) {
            ++refused;  // a craft with parts the game renamed since (ShipConstruct.LoadShip refuses it, as KSP does)
            continue;
        }
        REQUIRE(v != nullptr);
        ++loaded;
        PackLog log;
        // Part.Unpack / Part.Pack of every part (no rigid-body host here: Vessel::env.unpackPart / packPart unset)
        for (Part* p : v->parts) {
            const size_t want = ExpectedReceivers(*p, true);
            p->Unpack();
            CHECK_FALSE(p->fields.packed);
            CHECK(p->SendPartMessage("OnPartUnpack") == want);  // the dispatcher reaches exactly the C# receivers
            CHECK(p->SendPartMessage("OnPartPack") == ExpectedReceivers(*p, false));
            CHECK(p->SendPartMessage("OnSomethingElse") == 0);
            receiversUnpack += want;
            receiversPack += ExpectedReceivers(*p, false);
            for (size_t i = 0; i < p->Modules().Count(); ++i)
                if (auto* chute = dynamic_cast<ModuleParachute*>(p->Modules().GetModule(i))) {
                    // ModuleParachute.cs:811: timeUnpacked = Planetarium.GetUniversalTime()
                    CHECK(chute->timeUnpacked == ResourceProcessingEnvironment::UniversalTime());
                    ++parachutes;
                }
        }
        REQUIRE(log.unpacked.size() == v->parts.size());
        for (size_t i = 0; i < v->parts.size(); ++i) CHECK(log.unpacked[i] == v->parts[i]);
        for (Part* p : v->parts) p->Pack();
        REQUIRE(log.packed.size() == v->parts.size());
        for (Part* p : v->parts) CHECK(p->fields.packed);
        // Vessel.SendMessage reaches the components of the Vessel's own GameObject, the root part's: no clamp is the root here
        CHECK(v->rootPart->SendPartMessage("OnVesselPack") == 0);
        CHECK(v->rootPart->SendPartMessage("OnVesselUnpack") == 0);
        // a clamp's own GameObject (the clamp as the root of its vessel after release): LaunchClamp.cs:324 / :332
        for (Part* p : v->parts)
            if (p->FindModuleOfClass("LaunchClamp") != nullptr) {
                ++clamps;
                const int s0 = clampSet, u0 = clampUnset;
                CHECK(p->SendPartMessage("OnVesselUnpack") == 1);
                CHECK(clampSet == s0 + 1);
                CHECK(p->SendPartMessage("OnVesselUnpack") == 1);  // the joint is set: no second SetJoint (:334)
                CHECK(clampSet == s0 + 1);
                CHECK(p->SendPartMessage("OnVesselPack") == 1);
                CHECK(clampUnset == u0 + 1);
            }
        // a DEAD part does not pack (Part.cs:3749)
        Part* last = v->parts.back();
        last->Unpack();
        const size_t before = log.packed.size();
        last->set_State(PartStates::DEAD);
        last->Pack();
        CHECK(log.packed.size() == before);
        CHECK_FALSE(last->fields.packed);
        last->set_State(PartStates::IDLE);
    }
    LaunchClamp::hooks = savedClamp;
    MESSAGE(loaded << " stock crafts (" << refused << " refused by LoadShip): receivers reached: OnPartUnpack " << receiversUnpack << ", OnPartPack " << receiversPack << " (" << parachutes
                   << " parachutes, " << clamps << " launch clamps)");
    CHECK(loaded >= 4);
    CHECK(parachutes >= 2);
    CHECK(clamps >= 3);
    CHECK(receiversUnpack > receiversPack);
}

// ===================================================================================================================== item 2

TEST_CASE("WP-20g GameSettings.Awake: KSP's defaults, the shipped keyboard layouts, settings.cfg when present (GameSettings.cs:1506); every case starts at the defaults") {
    // the hygiene listener ran SetDefaultValues before this case (what GameSettings.Awake does once per game start): the C# defaults
    // (GameSettings.cs:957-1405), not the field initialisers (0 / false)
    CHECK(GameSettings::VESSEL_ANCHOR_TIME_THRESHOLD == 1.5f);       // :1122
    CHECK(GameSettings::VESSEL_ANCHOR_VELOCITY_THRESHOLD == 0.05f);  // :1121
    CHECK(GameSettings::VESSEL_ANCHOR_BREAK_TORQUE == 100.0f);       // :1126
    CHECK(GameSettings::PHYSICS_FRAME_DT_LIMIT == 0.04f);            // :995
    CHECK(GameSettings::CONIC_PATCH_LIMIT == 3);                     // :999
    CHECK(GameSettings::PHYSICS_EASE);                               // :1005
    CHECK(GameSettings::WHEEL_DAMAGE_IMPACTCOLLIDER_ENABLED);        // :1066
    CHECK_FALSE(GameSettings::INPUT_DEVICES);                        // made by Awake only (:1534)

    struct Scratch {
        fs::path path;
        Scratch() {
            std::random_device rd;
            path = fs::temp_directory_path() / ("kspcore_wp20g_" + std::to_string(rd()));
            fs::create_directories(path / "GameData" / "Squad" / "Controls");
        }
        ~Scratch() {
            std::error_code ec;
            fs::remove_all(path, ec);
        }
    } root;
    const fs::path controls = fs::path(ksp::testing::GameRoot()) / "GameData" / "Squad" / "Controls";
    size_t layouts = 0;
    for (const auto& e : fs::directory_iterator(controls))
        if (e.path().extension() == ".cfg") {
            fs::copy_file(e.path(), root.path / "GameData" / "Squad" / "Controls" / e.path().filename());
            ++layouts;
        }
    REQUIRE(layouts == 9);
    const size_t listeners = GE::onGameStateSaved.ListenerCount();

    // no settings.cfg: the detected keyboard layout (Qwerty.cfg without a system layout) and the defaults, written when asked (:1550-1555)
    GameSettings::VESSEL_ANCHOR_TIME_THRESHOLD = 9.0f;  // a stale value: Awake starts from SetDefaultValues (:1529)
    GameSettings::AwakeOptions o;
    o.gameRoot = root.path.string();
    o.writeSettingsCfg = true;
    GameSettings::Awake(o);
    CHECK(GameSettings::VESSEL_ANCHOR_TIME_THRESHOLD == 1.5f);
    CHECK(GameSettings::INPUT_DEVICES);
    CHECK(GameSettings::layoutConfigNodes.Count() == static_cast<int>(layouts));  // ParseLayoutsCfg over Controls/*.cfg (:1541-1544)
    CHECK(GameSettings::CURRENT_LAYOUT_SETTINGS == "Qwerty.cfg");
    CHECK(fs::path(GameSettings::configFilePath) == root.path / "settings.cfg");
    CHECK_FALSE(GameSettings::saveOnGameSave);                                  // :1564
    CHECK(GE::onGameStateSaved.ListenerCount() == listeners + 1);              // :1565
    REQUIRE(fs::exists(root.path / "settings.cfg"));                           // WriteCfg (:1554)

    // settings.cfg present and not overridden: ParseCfg reads it (:1545-1549); a value changed in the file is the value afterwards
    {
        ConfigNode::NodePtr cfg = ConfigNode::Load((root.path / "settings.cfg").string());
        REQUIRE(cfg);
        REQUIRE(cfg->HasValue("VESSEL_ANCHOR_TIME_THRESHOLD"));
        cfg->SetValue("VESSEL_ANCHOR_TIME_THRESHOLD", "2.25");
        cfg->SetValue("CONIC_PATCH_LIMIT", "5");
        REQUIRE(cfg->Save((root.path / "settings.cfg").string()));
    }
    o.writeSettingsCfg = false;
    // the singleton (:1508-1513): a second Awake while the instance lives does nothing
    CHECK(GameSettings::Ready());
    GameSettings::Awake(o);
    CHECK(GameSettings::VESSEL_ANCHOR_TIME_THRESHOLD == 1.5f);
    CHECK(GE::onGameStateSaved.ListenerCount() == listeners + 1);
    // OnDestroy (:1573) releases it; the next game start reads the file
    GameSettings::OnDestroy();
    CHECK_FALSE(GameSettings::Ready());
    CHECK(GE::onGameStateSaved.ListenerCount() == listeners);
    GameSettings::Awake(o);
    CHECK(GameSettings::VESSEL_ANCHOR_TIME_THRESHOLD == 2.25f);
    CHECK(GameSettings::CONIC_PATCH_LIMIT == 5);
    CHECK(GameSettings::layoutConfigNodes.Count() == static_cast<int>(layouts));  // the new instance's layouts (no duplicates)
    CHECK(GE::onGameStateSaved.ListenerCount() == listeners + 1);
    // overrideSettings: the file is ignored, the defaults and the detected layout again
    GameSettings::OnDestroy();
    GameSettings::overrideSettings = true;
    GameSettings::Awake(o);
    CHECK(GameSettings::VESSEL_ANCHOR_TIME_THRESHOLD == 1.5f);
    CHECK(GameSettings::CONIC_PATCH_LIMIT == 3);
    GameSettings::overrideSettings = false;
    GameSettings::OnDestroy();
    CHECK(GE::onGameStateSaved.ListenerCount() == listeners);
    // (the hygiene listener restores the defaults and drops INPUT_DEVICES / the layouts after this case)
}

// ===================================================================================================================== items 3 and 4

TEST_CASE("WP-20g module start-up of a vessel entering the flight scene: once per part, before its first flight tick (Part.cs:1727 / :1945, ModuleJettison.cs:474)") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w(true);
    wp20d::PadScene pad(w);
    auto v = w.Craft("PhysProf-S");
    pad.Place(*v);  // StartVesselInFlight
    std::vector<std::array<float, 6>> weights;
    for (Part* p : v->parts) {
        CHECK(p->fields.started);
        std::array<float, 6> a{};
        std::memcpy(a.data(), p->DragCubes.WeightedArea().data(), sizeof(float) * 6);
        weights.push_back(a);
    }
    // Unity starts a MonoBehaviour once: a second call (the bubble's onVesselLoaded of a vessel the host started) changes nothing
    StartVesselInFlight(*v);
    for (size_t i = 0; i < v->parts.size(); ++i)
        CHECK(std::memcmp(v->parts[i]->DragCubes.WeightedArea().data(), weights[i].data(), sizeof(float) * 6) == 0);
    // the drag-cube weights KSP flies with are compared against C10 in tests/test_flight_input_feed.cpp ("WP-20f module lifecycle on the pad")
}

TEST_CASE("WP-20g the flight scene's resource-flow host: one VesselResourceSets per Vessel behind the WP-15 seams (Vessel.cs:3913 / :3980 / :3994), dropped with the vessel") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w(true);
    // a host's own seams are kept: the bubble saves them on Install and restores them on Uninstall
    int sentinelCalls = 0;
    ResourceFlowEnvironment::vesselHost = [&](Vessel*) -> IResourceVessel* {
        ++sentinelCalls;
        return nullptr;
    };
    {
        wp20d::PadScene pad(w);
        VesselResourceHost& rh = pad.s.bubble.Resources();
        REQUIRE(rh.Installed());
        auto v = w.Craft("PhysProf-S");
        pad.Place(*v);  // Vessel.UpdateResourceSets through Vessel::env.updateResourceSets
        CHECK(rh.Count() == 1);
        VesselResourceSets* sets = rh.Find(*v);
        REQUIRE(sets != nullptr);
        REQUIRE(sets->resourcePartSet);
        CHECK(rh.Rebuilds() >= 1);
        IResourceVessel* iv = ResourceFlowEnvironment::Host(v.get());
        REQUIRE(iv != nullptr);
        CHECK(iv == ResourceFlowEnvironment::Host(v.get()));
        CHECK(sentinelCalls == 0);
        // the vessel-wide totals (Vessel.cs:3994: resourcePartSet over every part) equal the parts' own amounts
        for (const char* res : {"LiquidFuel", "Oxidizer", "ElectricCharge"}) {
            const int id = MonoStringHash(res);
            double amount = 0.0, maxAmount = 0.0, sum = 0.0, sumMax = 0.0;
            iv->GetConnectedResourceTotals(id, false, amount, maxAmount, true);
            for (Part* p : v->parts)
                if (PartResource* r = p->GetResource(res)) {
                    sum += r->amount;
                    sumMax += r->maxAmount;
                }
            CAPTURE(res);
            CHECK(amount == doctest::Approx(sum).epsilon(1e-12));
            CHECK(maxAmount == doctest::Approx(sumMax).epsilon(1e-12));
            CHECK(sum > 0.0);
        }
        // the engines draw through the host in the tick (ModuleEngines -> Part.RequestResource -> the vessel's sets)
        pad.s.Start(*v);
        REQUIRE(wp20e::TickUntilUnpacked(pad, *v));
        const int lf = MonoStringHash("LiquidFuel");
        double before = 0.0, maxA = 0.0, after = 0.0;
        iv->GetConnectedResourceTotals(lf, false, before, maxA, true);
        wp20d::IgniteFirstStage(*v);
        for (int k = 0; k < 50; ++k) pad.s.Tick();
        iv = ResourceFlowEnvironment::Host(v.get());
        iv->GetConnectedResourceTotals(lf, false, after, maxA, true);
        MESSAGE("PhysProf-S through the bubble's resource host: LiquidFuel " << before << " -> " << after << " in 50 ticks; " << rh.Rebuilds()
                                                                             << " UpdateResourceSets served");
        CHECK(after < before);
        // the registry removal drops the entry and the parts' crossfeed sets (they point at the vessel's parts)
        FlightGlobals::RemoveVessel(v.get());
        CHECK(rh.Count() == 0);
        CHECK(rh.Find(*v) == nullptr);
        for (Part* p : v->parts) CHECK_FALSE(p->crossfeedPartSet);
    }
    REQUIRE(ResourceFlowEnvironment::vesselHost);
    CHECK(ResourceFlowEnvironment::vesselHost(nullptr) == nullptr);
    CHECK(sentinelCalls == 1);  // the seam that was there before the bubble is back
    ResourceFlowEnvironment::vesselHost = nullptr;
}
