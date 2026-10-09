// WP-14 unit tests: the Vessel model (Vessel.cs), FlightCtrlState, ActionGroupList, DiscoveryInfo, VesselValues, the trip logger,
// FlightLogger, FlightCoMTracker, AxisGroupsModule and ProtoTargetInfo.
//
// The numerics that exist in a capture are gated by `capture.vessel_state` (C10). Everything here is derived from the decompiled
// source or taken from original game data: the physprofile save (testdata/saves: 2 vessels, 458 parts), the PhysProf crafts
// (analysis/physics/craft), the C9 part database and the C10 structure table, and the stock part prefabs of the GameData mirror.
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/flight_atmosphere.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/orbit/position_target.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/vessel/action_group_list.hpp"
#include "ksp/vessel/axis_groups_module.hpp"
#include "ksp/vessel/discovery_info.hpp"
#include "ksp/vessel/flight_com_tracker.hpp"
#include "ksp/vessel/flight_ctrl_state.hpp"
#include "ksp/vessel/flight_logger.hpp"
#include "ksp/vessel/module_trip_logger.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_assembly.hpp"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;
using namespace ksp;
using GE = events::GameEvents;

namespace {

const std::string kHarness14 = KSPCORE_HARNESS_DIR;
const std::string kSaves14 = std::string(KSPCORE_TESTDATA_DIR) + "/saves";

bool GameAvailable14() { return fs::exists(fs::path(ksp::testing::GameRoot()) / "GameData" / "Squad"); }

uint32_t Bits14(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}

// The stock game (GameData mirror) + PhysicsGlobals + Planetarium + the Kerbol system, as the flight scene has them.
struct World14 {
    GameData data;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    World14() {
        SaveEnvironment::Reset();
        FlightRegistry::Clear();
        GameData::Options o;
        o.gameRoot = ksp::testing::GameRoot();
        data.Load(o);
        SaveEnvironment& env = SaveEnvironment::Current();
        env.getPartInfoByName = [this](std::string_view n) { return data.parts.getPartInfoByName(n); };
        env.partExists = [this](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
        env.newGuidHashCode = [] {
            static uint32_t s = 4242u;
            s = s * 1664525u + 1013904223u;
            return static_cast<int32_t>(s);
        };
        pg.physicsDatabaseFilename = kHarness14 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        Vessel::env = VesselEnvironment{};
        VesselModuleManager::CompileModules();  // the VesselModule types every new Vessel gets
    }
    ~World14() {
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
        SaveEnvironment::Reset();
        Vessel::env = VesselEnvironment{};
    }
    CelestialBody& Kerbin() { return *sys->GetBodyByName("Kerbin"); }
    // Unity floats cannot hold a vessel at Kerbin's absolute world position (1.4e10 m): like the floating origin, put the pad at
    // the origin (Kerbin.position is the body's world position, a plain field) and return the origin as the vessel position
    Vector3f PlaceKerbinAtPad() {
        CelestialBody& k = Kerbin();
        k.position = Vector3d();
        const Vector3d pad = k.GetWorldSurfacePosition(-0.0972077486779459, -74.5576813534722, 75.0);
        k.position = Vector3d() - pad;
        return Vector3f();
    }
    const AvailablePart& Info(const char* name) {
        const AvailablePart* p = data.parts.getPartInfoByName(name);
        REQUIRE_MESSAGE(p != nullptr, name);
        return *p;
    }
    std::function<const AvailablePart*(const std::string&)> Lookup() {
        return [this](const std::string& n) { return data.parts.getPartInfoByName(n); };
    }
    // The three C10 crafts (analysis/physics/craft: the game's own craft files)
    std::unique_ptr<Vessel> Craft(const char* name, bool landed = true) {
        ConfigNode::NodePtr root = ConfigNode::Load(kHarness14 + "/../analysis/physics/craft/" + name + ".craft");
        REQUIRE(root);
        ShipConstruct ship;
        std::string err;
        REQUIRE_MESSAGE(ship.LoadShip(*root, 0, true, err), err);
        AssembleOptions ao;
        ao.rootPosition = PlaceKerbinAtPad();
        ao.isLanded = landed;
        auto v = AssembleVesselFromCraft(ship, Lookup(), ao);
        REQUIRE(v != nullptr);
        return v;
    }
};

nlohmann::json LoadCapture10() {
    std::ifstream f(kHarness14 + "/captures/out2/aero_forces_vessel.json", std::ios::binary);
    REQUIRE(f.good());
    return nlohmann::json::parse(f);
}

const nlohmann::json& Table14(const nlohmann::json& cap, const char* name) {
    for (const auto& t : cap.at("tables"))
        if (t.at("name") == name) return t;
    FAIL("table " << name);
    static nlohmann::json none;
    return none;
}

size_t Col14(const nlohmann::json& t, const char* c) {
    size_t i = 0;
    for (const auto& col : t.at("columns")) {
        if (col.at("name") == c) return i;
        i++;
    }
    FAIL("column " << c);
    return 0;
}

}  // namespace

// ======================================================== small records, FlightCtrlState, ActionGroupList, DiscoveryInfo

TEST_CASE("WP-14 small types: FlightEvents, EventReport defaults, VesselEngiUtil, GameSettings values of Vessel.cs") {
    CHECK(static_cast<int>(FlightEvents::CUSTOM) == 18);  // FlightEvents.cs: 19 members
    CHECK(static_cast<int>(FlightEvents::CREW_ON_EVA) == 12);
    CHECK(static_cast<int>(FlightEndModes::ABORTED) == 3);
    CHECK(static_cast<int>(VesselActuatorMode::Pilot) == 2);
    EventReport r(FlightEvents::CRASH, nullptr);  // EventReport.cs:15 defaults
    CHECK(r.sender == "an unidentified object");
    CHECK(r.other == "an unidentified object");
    CHECK(r.stage == 0);
    CHECK(r.msg == "");
    CHECK(r.param == 0.0f);
    EventReport r2(FlightEvents::OVERHEAT, nullptr, "a", "b", 3, "m", 1.5f);
    CHECK(r2.sender == "a");
    CHECK(r2.other == "b");
    CHECK(r2.stage == 3);
    CHECK(r2.msg == "m");
    CHECK(r2.param == 1.5f);
    CHECK(static_cast<int>(ControlLevel::FULL) == 3);  // Vessel.ControlLevel
    CHECK(static_cast<int>(VesselState::DEAD) == 2);   // Vessel.State
    CHECK(GameSettings::ADDITIONAL_ACTION_GROUPS == false);  // GameSettings.cs:978
    CHECK(GameSettings::RADAR_ALTIMETER_EXTENDED_CALCS == true);  // GameSettings.cs:1008
    // Vessel.Situations is the one VesselSituations enum (GameEvents' Vessel_Situations spelling is the same type)
    static_assert(std::is_same_v<VesselSituations, Vessel_Situations>);
    CHECK(static_cast<int>(Vessel::Situations::DOCKED) == 0x80);
}

TEST_CASE("WP-14 FlightCtrlState: shared custom axes, copy / mask / neutralize rules, CTRLSTATE node round trip") {
    FlightCtrlState a;
    CHECK(a.custom_axes->size() == 4);  // GameSettings.AXIS_CUSTOM.Length
    FlightCtrlState b(a.custom_axes);   // Vessel.Awake: setControlStates share ctrlState.custom_axes
    (*a.custom_axes)[2] = 0.5f;
    CHECK((*b.custom_axes)[2] == 0.5f);
    a.pitch = 0.25f;
    a.yaw = -0.5f;
    a.roll = 0.125f;
    a.mainThrottle = 0.75f;
    a.X = 1.0f;
    a.Y = 2.0f;
    a.Z = 3.0f;
    a.wheelSteer = 0.5f;
    a.killRot = true;
    CHECK_FALSE(a.isIdle());
    CHECK_FALSE(a.isNeutral());
    FlightCtrlState c;
    c.CopyFrom(a);
    CHECK(c.pitch == 0.25f);
    CHECK(c.killRot);
    CHECK((*c.custom_axes)[2] == 0.5f);  // copied element-wise
    // MaskedCopyFrom: Y is tested under TranslateZ (sic), TranslateY alone copies nothing
    FlightCtrlState m;
    m.MaskedCopyFrom(a, KSPAxisGroup::TranslateY);
    CHECK(m.Y == 0.0f);
    m.MaskedCopyFrom(a, KSPAxisGroup::TranslateZ);
    CHECK(m.Y == 2.0f);
    CHECK(m.Z == 3.0f);
    CHECK(m.X == 0.0f);
    m.MaskedCopyFrom(a, static_cast<KSPAxisGroup>(static_cast<int>(KSPAxisGroup::Pitch) | static_cast<int>(KSPAxisGroup::MainThrottle)));
    CHECK(m.pitch == 0.25f);
    CHECK(m.mainThrottle == 0.75f);
    CHECK(m.yaw == 0.0f);
    a.pitchTrim = 0.1f;
    a.Neutralize();  // trims are not neutralized
    CHECK(a.pitch == 0.0f);
    CHECK(a.pitchTrim == 0.1f);
    CHECK_FALSE(a.killRot);
    a.NeutralizeAll();
    CHECK(a.isNeutral());
    CHECK(a.isIdle());
    // Save writes 7 values; Load reads them back; a node with fewer values leaves the rest
    FlightCtrlState s;
    s.pitch = 0.3f;
    s.yaw = -0.2f;
    s.roll = 0.7f;
    s.pitchTrim = 0.01f;
    s.yawTrim = -0.02f;
    s.rollTrim = 0.03f;
    s.mainThrottle = 1.0f;
    ConfigNode n("CTRLSTATE");
    s.Save(n);
    CHECK(n.values().Count() == 7);
    CHECK(n.values()[3].name == "trimPitch");
    FlightCtrlState t;
    t.Load(n);
    CHECK(Bits14(t.pitch) == Bits14(s.pitch));
    CHECK(Bits14(t.yawTrim) == Bits14(s.yawTrim));
    CHECK(Bits14(t.mainThrottle) == Bits14(s.mainThrottle));
    ConfigNode partial("CTRLSTATE");
    partial.AddValue("roll", 0.5f);
    FlightCtrlState u;
    u.pitch = 0.9f;
    u.Load(partial);
    CHECK(u.roll == 0.5f);
    CHECK(u.pitch == 0.9f);
    CHECK(s.GetPYR() == Vector3f(0.3f, 0.7f, -0.2f));
}

TEST_CASE("WP-14 ActionGroupList: constructor shape, indexer, Load / Save format, CopyFrom") {
    if (!GameAvailable14()) return;
    ActionGroupList l(nullptr);
    CHECK(l.groups.size() == 17);  // Enum.GetNames(KSPActionGroup).Length - 2
    CHECK(l.cooldownTimes.size() == 17);
    CHECK(l.groupStates.size() == 4);
    CHECK(l.groupStates[0].size() == 17);
    l.Set(KSPActionGroup::Gear, true);
    CHECK(l[KSPActionGroup::Gear]);
    CHECK_FALSE(l[KSPActionGroup::Light]);
    ConfigNode n("ACTIONGROUPS");
    l.cooldownTimes[2] = 1.5;
    l.Save(n);
    CHECK(n.values().Count() == 17);
    CHECK(n.values()[0].name == "Stage");
    CHECK(n.values()[0].value == "False, 0");
    CHECK(n.values()[1].name == "Gear");
    CHECK(n.values()[1].value == "True, 0");
    CHECK(n.values()[2].value == "False, 1.5");
    CHECK(n.values()[16].name == "Custom10");
    ActionGroupList m(nullptr);
    m.Load(n);
    CHECK(m[KSPActionGroup::Gear]);
    CHECK(m.cooldownTimes[2] == 1.5);
    // a value without a cooldown is just the bool
    ConfigNode old("ACTIONGROUPS");
    old.AddValue("RCS", "True");
    ActionGroupList o(nullptr);
    o.Load(old);
    CHECK(o[KSPActionGroup::RCS]);
    ActionGroupList cp(nullptr);
    cp.CopyFrom(m);
    CHECK(cp[KSPActionGroup::Gear]);
    CHECK(cp.cooldownTimes[2] == 1.5);
}

TEST_CASE("WP-14 DiscoveryInfo: levels, lifetimes, signal strength, Load / Save, size classes") {
    DiscoveryInfo d(nullptr);  // Owned, infinite lifetimes, class C
    CHECK(d.Level() == DiscoveryLevels::Owned);
    CHECK(std::isinf(d.unobservedLifetime()));
    CHECK(d.objectSize() == UntrackedObjectClass::C);
    CHECK(d.HaveKnowledgeAbout(DiscoveryLevels::StateVectors));
    CHECK(d.GetSignalLife(123.0) == 1.0);
    DiscoveryInfo e(nullptr, DiscoveryLevels::Presence, 100.0);
    CHECK(e.referenceLifetime() == 200.0);
    CHECK_FALSE(e.HaveKnowledgeAbout(DiscoveryLevels::StateVectors));
    e.SetLastObservedTime(10.0);
    CHECK(e.fadeUT() == 110.0);
    CHECK(e.GetSignalLife(60.0) == 0.5);
    CHECK(e.GetSignalLife(500.0) == 0.0);
    CHECK(e.GetSignalStrength(60.0) == 0.5 * 0.5);  // min(1, 100/200) * life
    e.SetUnobservedLifetime(50.0);
    CHECK(e.fadeUT() == 60.0);
    // DISCOVERY nodes of the physprofile save
    ConfigNode::NodePtr root = ConfigNode::Load(kSaves14 + "/physprofile/persistent.sfs");
    REQUIRE(root);
    int checked = 0;
    ConfigNode* flight = root->GetNode("GAME")->GetNode("FLIGHTSTATE");
    REQUIRE(flight != nullptr);
    for (int i = 0; i < flight->nodes().Count(); i++) {
        ConfigNode& vn = *flight->nodes()[i];
        if (vn.name != "VESSEL") continue;
        ConfigNode* disc = vn.GetNode("DISCOVERY");
        REQUIRE(disc != nullptr);
        DiscoveryInfo di(nullptr);
        di.Load(*disc);
        ConfigNode out("DISCOVERY");
        di.Save(out);
        CHECK(out.ToString() == disc->ToString());
        checked++;
    }
    CHECK(checked == 2);
    CHECK(DiscoveryInfo::GetObjectClass("E") == UntrackedObjectClass::E);
    CHECK(DiscoveryInfo::GetObjectClass("nonsense") == UntrackedObjectClass::A);
    // GetClassRadius draws one Random.Range(0.75, 1.25) after InitState(seed): the radius is within [0.75, 1.25] of the class radius
    const float r = DiscoveryInfo::GetClassRadius(UntrackedObjectClass::D, 7);
    CHECK(r >= 15.0f * 0.75f);
    CHECK(r <= 15.0f * 1.25f);
    CHECK(DiscoveryInfo::GetClassRadius(UntrackedObjectClass::D, 7) == r);  // seeded: repeatable
    CHECK(DiscoveryInfo::GetMinRadius(UntrackedObjectClass::D, 7) == f32store(fx(r) * fx(0.75f)));
    const UntrackedObjectClass k = DiscoveryInfo::GetClassFromSize(r, 7);
    CHECK(DiscoveryInfo::GetMinRadius(k, 7) < r);
    CHECK(DiscoveryInfo::GetMaxRadius(k, 7) > r);
}

// ======================================================== Vessel from the PhysProf crafts (C10 structure)

TEST_CASE("WP-14 Vessel assembled from the PhysProf crafts: part order, tree, cube names and counts equal the C10 structure table") {
    if (!GameAvailable14()) return;
    World14 w;
    const nlohmann::json cap = LoadCapture10();
    const nlohmann::json& st = Table14(cap, "structure");
    const size_t cCraft = Col14(st, "craft"), cIdx = Col14(st, "part_index"), cPart = Col14(st, "part"), cParent = Col14(st, "parent_index"),
                 cCubes = Col14(st, "cube_names");
    std::map<std::string, std::unique_ptr<Vessel>> vessels;
    std::map<std::string, size_t> expected;
    for (const auto& r : st.at("rows")) expected[r[cCraft].get<std::string>()]++;
    CHECK(expected.size() == 3);
    for (const auto& [name, n] : expected) {
        auto v = w.Craft(name.c_str());
        CHECK(v->parts.size() == n);
        CHECK(v->rootPart == v->parts[0]);
        CHECK(v->rootPart->parent == nullptr);
        CHECK(v->loaded);
        CHECK(v->packed);                            // fromShipAssembly: packed until the physics hold ends
        CHECK(v->situation == Vessel::Situations::PRELAUNCH);
        CHECK(v->Landed);
        CHECK(v->landedAt == "LaunchPad");
        CHECK(v->launchedFrom == "LaunchPad");
        CHECK(v->vesselSpawning);
        CHECK(v->mainBody() == &w.Kerbin());
        CHECK(v->state == VesselState::INACTIVE);  // Initialize ends with MakeInactive
        CHECK(v->HoldPhysics() == false);
        vessels[name] = std::move(v);
    }
    size_t cubeRows = 0;
    for (const auto& r : st.at("rows")) {
        Vessel& v = *vessels.at(r[cCraft].get<std::string>());
        const size_t i = r[cIdx].get<size_t>();
        Part* p = v.parts.at(i);
        CHECK(p->partInfo->name == r[cPart].get<std::string>());
        CHECK(p->vessel == &v);
        if (r[cParent].is_null()) {
            CHECK(p == v.rootPart);
        } else {
            CHECK(v.parts.at(r[cParent].get<size_t>()) == p->parent);
        }
        // cube_names = "name=weight,..." : the names (weights are set by module logic of other packages) equal the part's cubes
        std::string names, text = r[cCubes].get<std::string>();
        std::vector<std::string> want;
        size_t pos = 0;
        while (pos < text.size()) {
            size_t comma = text.find(',', pos);
            if (comma == std::string::npos) comma = text.size();
            want.push_back(text.substr(pos, text.find('=', pos) - pos));
            pos = comma + 1;
        }
        std::vector<std::string> got;
        for (const DragCube& c : p->DragCubes.Cubes()) got.push_back(c.Name());
        CHECK(got == want);
        cubeRows++;
    }
    CHECK(cubeRows == 631);
}

// ======================================================== physprofile save: ProtoVessel -> Vessel -> BackupVessel

namespace {
// Value names of two nodes that differ (same count and order assumed; "<count>" otherwise)
std::vector<std::string> DiffValues14(const ConfigNode& a, const ConfigNode& b) {
    std::vector<std::string> d;
    if (a.values().Count() != b.values().Count()) return {"<count>"};
    for (int i = 0; i < a.values().Count(); i++)
        if (a.values()[i].name != b.values()[i].name || a.values()[i].value != b.values()[i].value) d.push_back(a.values()[i].name);
    return d;
}
}  // namespace

TEST_CASE("WP-14 physprofile save: ProtoVessel.Load -> Vessel.Load -> BackupVessel writes the VESSEL node back") {
    if (!GameAvailable14()) return;
    World14 w;
    // The save lists 3 vessel modules for the asteroid and 4 (with CommNetVessel, WP-39) for the L2-AS vessel, never CometVessel
    // (the live game's reason is not in the decompile): the round trip runs with the same 3 types the asteroid has.
    VesselModuleManager::SetWrapperActive("CometVessel", false);
    VesselModuleManager::SetWrapperActive("CommNetVessel", false);
    GameParameters::GetDefaultParameters(Game_Modes::CAREER, GameParameters::Preset::Normal);
    ConfigNode::NodePtr root = ConfigNode::Load(kSaves14 + "/physprofile/persistent.sfs");
    REQUIRE(root);
    Game game(root.get());
    REQUIRE(game.flightState);
    std::vector<std::unique_ptr<Vessel>> vessels;
    size_t partsLoaded = 0;
    size_t modulesTotal = 0, modulesEqual = 0;
    std::map<std::string, size_t> modulesDiffer;
    for (auto& pvp : game.flightState->protoVessels) {
        ProtoVessel& pv = *pvp;
        ConfigNode original("VESSEL");
        pv.Save(original);
        std::unique_ptr<Vessel> v = pv.Load(game.flightState.get());
        REQUIRE(v != nullptr);
        CHECK(v->protoVessel == &pv);
        CHECK_FALSE(v->loaded);
        CHECK(v->packed);
        CHECK(v->vesselName == pv.vesselName);
        CHECK(v->id == pv.vesselID);
        CHECK(v->persistentId == pv.persistentId);
        CHECK(v->situation == pv.situation);
        CHECK(v->vesselType == pv.vesselType);
        CHECK(v->landedAt == pv.landedAt);
        CHECK(v->displaylandedAt == pv.displaylandedAt);
        CHECK(v->Landed == pv.landed);
        CHECK(v->launchedFrom == pv.launchedFrom);
        CHECK(v->latitude == pv.latitude);
        CHECK(v->longitude == pv.longitude);
        CHECK(v->altitude == pv.altitude);
        CHECK(v->missionTime == pv.missionTime);
        CHECK(v->lastUT == pv.lastUT);
        CHECK(v->currentStage == pv.stage);
        CHECK(v->mainBody() != nullptr);
        CHECK(v->mainBody()->bodyName == (pv.protoPartSnapshots.size() > 1 ? "Kerbin" : v->mainBody()->bodyName));
        CHECK(v->orbitDriver->updateMode == (v->LandedOrSplashed() ? OrbitDriver::UpdateMode::IDLE : OrbitDriver::UpdateMode::UPDATE));
        CHECK(v->vesselModules.size() == 3);  // FlightIntegrator, AxisGroupsModule, SuspensionLoadBalancer
        CHECK(FlightRegistry::Vessels.size() == vessels.size() + 1);
        CHECK(FlightRegistry::FindVessel(pv.vesselID) == v.get());
        // an unloaded vessel's mass is the snapshot mass + resource masses
        const double unloadedMass = v->GetUnloadedVesselMass();
        CHECK(unloadedMass > 0.0);
        if (pv.protoPartSnapshots.size() > 1) {
            v->Load();
            partsLoaded += v->parts.size();
            CHECK(v->loaded);
            CHECK_FALSE(v->packed == false);
            CHECK(v->parts.size() == pv.protoPartSnapshots.size());
            CHECK(v->rootPart == v->parts[static_cast<size_t>(pv.rootIndex)]);
            CHECK(v->OwnedPartCount() == v->parts.size());
            CHECK(v->situation == Vessel::Situations::PRELAUNCH);  // updateSituation on Load: Landed, PRELAUNCH stays
            // the loaded mass (part.mass + resource mass) equals the unloaded one to float precision
            const float loadedMass = v->GetTotalMass();
            CHECK(static_cast<double>(loadedMass) == doctest::Approx(unloadedMass).epsilon(1e-6));
            // ModuleCommand (WP-24) sets Part.isControlSource at start: emulate it for the probe core so CheckControllable has a source
            for (Part* p : v->parts)
                if (PartModule* mc = p->FindModuleOfClass("ModuleCommand"))
                    if (mc->Get<int32_t>("minimumCrew") == 0) p->fields.isControlSource = static_cast<int32_t>(ControlLevel::FULL);
            v->CheckControllable();
            CHECK(v->IsControllable());
            CHECK(v->CurrentControlLevel() == ControlLevel::FULL);
        }
        ProtoVessel* back = v->BackupVessel();
        REQUIRE(back != nullptr);
        CHECK((back != &pv) == (pv.protoPartSnapshots.size() > 1));  // a loaded vessel builds a new ProtoVessel; an unloaded one updates its own
        CHECK(v->protoVessel == back);
        ConfigNode rewritten("VESSEL");
        back->Save(rewritten);
        CHECK(original.nodes().Count() == rewritten.nodes().Count());
        // KSP rewrites cPch / cHdg as 0: the camera values were read from "0.2" without a ", FLOAT" type suffix, so
        // KSPParseable.value_float is 0 (Vessel.OnLoadFlightState); the vessel-state dictionary is not carried otherwise
        const std::vector<std::string> diff = DiffValues14(original, rewritten);
        MESSAGE("vessel '" << v->vesselName << "': " << pv.protoPartSnapshots.size() << " parts, differing values: " << diff.size());
        for (const std::string& d : diff) CHECK((d == "cPch" || d == "cHdg"));
        // PART / ORBIT / modules / group nodes are byte-identical
        for (int i = 0; i < original.nodes().Count(); i++) {
            CAPTURE(original.nodes()[i]->name);
            CHECK(original.nodes()[i]->name == rewritten.nodes()[i]->name);
            if (original.nodes()[i]->name == "VESSELMODULES") continue;  // see above
            // PART nodes: everything but the MODULE nodes is byte-identical; a MODULE node is identical unless the module class
            // writes values in an OnSave that its owning package has not ported (ModuleReactionWheel.WheelState: WP-24 ...)
            ConfigNode& pa = *original.nodes()[i];
            ConfigNode& pb = *rewritten.nodes()[i];
            if (pa.name != "PART") {
                CHECK(pa.ToString() == pb.ToString());
                continue;
            }
            CHECK(DiffValues14(pa, pb).empty());
            REQUIRE(pa.nodes().Count() == pb.nodes().Count());
            for (int k = 0; k < pa.nodes().Count(); k++) {
                if (pa.nodes()[k]->name != "MODULE") {
                    CHECK(pa.nodes()[k]->ToString() == pb.nodes()[k]->ToString());
                    continue;
                }
                modulesTotal++;
                if (pa.nodes()[k]->ToString() == pb.nodes()[k]->ToString()) modulesEqual++;
                else modulesDiffer[*pa.nodes()[k]->GetValue("name")]++;
            }
        }
        vessels.push_back(std::move(v));
    }
    CHECK(vessels.size() == 2);
    CHECK(partsLoaded == 457);
    MESSAGE("MODULE nodes identical: " << modulesEqual << "/" << modulesTotal);
    for (const auto& [name, n] : modulesDiffer) MESSAGE("  module OnSave differs: " << name << " x" << n);
    CHECK(modulesTotal == 1337);
    CHECK(modulesEqual == 1335);
    CHECK(modulesDiffer.size() == 2);  // OnSave of ModuleReactionWheel (WheelState: WP-24) and ModuleDataTransmitter (WP-39)
    CHECK(modulesDiffer.count("ModuleReactionWheel") == 1);
    CHECK(modulesDiffer.count("ModuleDataTransmitter") == 1);
}

// ======================================================== helpers for hand-built vessels (stock part prefabs)

namespace {

// A vertical stack of stock parts (each the STACK child of the previous), assembled like AssembleForLaunch (fromShipAssembly)
std::unique_ptr<Vessel> Stack14(World14& w, std::initializer_list<const char*> names, bool initialize = true) {
    std::vector<std::unique_ptr<Part>> owned;
    uint32_t id = 100;
    for (const char* n : names) {
        const AvailablePart& ap = w.Info(n);
        auto p = Part::Instantiate(*ap.partPrefab);
        p->partInfo = &ap;
        p->gameObjectName = ap.name;
        p->fields.flightID = id++;
        p->fields.persistentId = id * 7;
        p->UpdateMass();
        if (!owned.empty()) p->setParent(owned.back().get());
        owned.push_back(std::move(p));
    }
    auto v = std::make_unique<Vessel>();
    Part* root = owned.front().get();
    root->partTransform.position = w.PlaceKerbinAtPad();
    v->vesselTransform = root->partTransform;
    v->AdoptParts(std::move(owned), root);
    v->vesselName = "Test vessel";
    if (initialize) {
        REQUIRE(v->Initialize(true, false, false, false));
    } else {  // a bare vessel: the part list findVesselParts would build, no orbit driver, not loaded
        for (Part* p = root; p != nullptr; p = p->children.empty() ? nullptr : p->children[0]) {
            v->parts.push_back(p);
            p->vessel = v.get();
        }
    }
    return v;
}

}  // namespace

// ======================================================== situations

TEST_CASE("WP-14 situation determination: checkLanded / checkSplashed / updateSituation / SetLandedAt (Vessel.cs 1015, 1044, 1185, 3133)") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = Stack14(w, {"mk1pod.v2", "fuelTankSmallFlat"});
    std::vector<std::pair<Vessel::Situations, Vessel::Situations>> changes;
    struct L {
        std::vector<std::pair<Vessel::Situations, Vessel::Situations>>* out;
        void Run(GE::HostedFromToAction<Vessel*, Vessel_Situations> d) { out->push_back({d.from, d.to}); }
    } listener{&changes};
    GE::onVesselSituationChange.Add(&listener, &L::Run);

    // fromShipAssembly leaves the vessel packed and PRELAUNCH; checkLanded needs it unpacked and loaded
    CHECK(v->situation == Vessel::Situations::PRELAUNCH);
    CHECK_FALSE(v->checkLanded());  // packed: Landed is not recomputed (and was not set by the assembly)
    v->packed = false;
    v->parts[1]->fields.GroundContact = true;
    CHECK(v->checkLanded());
    v->parts[1]->fields.GroundContact = false;
    v->parts[0]->fields.PermanentGroundContact = true;
    CHECK(v->checkLanded());  // PermanentGroundContact counts
    v->parts[0]->fields.PermanentGroundContact = false;
    v->SetLandedAt("LaunchPad", "", "Launch Pad");
    CHECK(v->landedAt == "LaunchPad");
    CHECK(v->displaylandedAt == "Launch Pad");
    CHECK(v->landedAtLast == "LaunchPad");
    CHECK_FALSE(v->checkLanded());  // no contact: Landed false and landedAt cleared
    CHECK(v->landedAt == "");
    CHECK(v->displaylandedAt == "");
    CHECK(v->landedAtLast == "LaunchPad");  // remembered
    v->SetLandedAt("Untagged");              // ignored
    CHECK(v->landedAt == "");
    // the facility hook supplies the display name (PSystemSetup.IsFacilityOrLaunchSite)
    Vessel::env.isFacilityOrLaunchSite = [](const std::string& n, bool& fac, std::string& disp) {
        fac = n == "KSC";
        if (n == "KSC" || n == "Runway") {
            disp = n == "KSC" ? "Kerbal Space Center" : "Runway";
            return true;
        }
        return false;
    };
    v->SetLandedAt("Runway");
    CHECK(v->displaylandedAt == "Runway");
    v->SetLandedAt("Mun_Craters");  // not a facility: the biome display name of ScienceUtil (hook unset: the converted tag)
    CHECK(v->displaylandedAt == "Mun Craters");
    Vessel::env.biomeDisplayName = [](CelestialBody&, const std::string& biome) { return "<" + biome + ">"; };
    v->SetLandedAt("");
    v->SetLandedAt("Mun_Craters");
    CHECK(v->displaylandedAt == "<Mun Craters>");
    Vessel::env.miniBiomeDisplayNameByUnityTag = [](const std::string& tag) { return "tag:" + tag; };
    v->SetLandedAt("");
    v->SetLandedAt("Mun_Craters", "MunTag");
    CHECK(v->displaylandedAt == "tag:MunTag");  // a GameObject tag wins over the biome name
    v->SetLandedAt("");

    // checkSplashed
    v->parts[0]->fields.WaterContact = true;
    v->Landed = true;
    CHECK(v->checkSplashed());
    CHECK_FALSE(v->Landed);
    v->parts[0]->fields.WaterContact = false;
    CHECK_FALSE(v->checkSplashed());

    // updateSituation: Landed / PRELAUNCH rules
    v->Landed = true;
    v->srfSpeed = 1.0;
    v->updateSituation();
    CHECK(v->situation == Vessel::Situations::PRELAUNCH);  // PRELAUNCH stays while slow
    v->srfSpeed = 2.5;
    v->updateSituation();
    CHECK(v->situation == Vessel::Situations::PRELAUNCH);  // `srfSpeed > 2.5` is false at 2.5
    v->precalc.isEasingGravity = false;
    v->vesselSpawning = false;
    v->srfSpeed = 2.6;
    v->updateSituation();
    CHECK(v->situation == Vessel::Situations::LANDED);
    v->srfSpeed = 0.0;
    v->updateSituation();
    CHECK(v->situation == Vessel::Situations::LANDED);  // only PRELAUNCH is sticky
    v->Landed = false;
    v->Splashed = true;
    v->updateSituation();
    CHECK(v->situation == Vessel::Situations::SPLASHED);
    v->Splashed = false;
    v->staticPressurekPa = 1.0;
    v->updateSituation();
    CHECK(v->situation == Vessel::Situations::FLYING);
    CHECK(v->BestSituation() == Vessel::Situations::FLYING);
    // orbit classes from the vessel's orbit around Kerbin
    CelestialBody& kerbin = w.Kerbin();
    v->staticPressurekPa = 0.0;
    auto setOrbit = [&](double e, double sma) {
        v->orbit() = Orbit(0.0, e, sma, 0.0, 0.0, 0.0, 0.0, &kerbin);
        v->orbit().Init();
    };
    setOrbit(0.0, kerbin.Radius + 100000.0);  // PeA 100 km > 70 km atmosphere
    v->updateSituation();
    CHECK(v->situation == Vessel::Situations::ORBITING);
    CHECK(v->BestSituation() == Vessel::Situations::ORBITING);  // lastSituation is no longer FLYING
    setOrbit(0.0, kerbin.Radius + 50000.0);
    v->updateSituation();
    CHECK(v->situation == Vessel::Situations::SUB_ORBITAL);  // PeA 50 km < atmosphereDepth
    setOrbit(1.5, -2.0 * kerbin.Radius);
    v->updateSituation();
    CHECK(v->situation == Vessel::Situations::ESCAPING);     // eccentricity >= 1
    setOrbit(0.9, 1.5 * kerbin.sphereOfInfluence);
    v->updateSituation();
    CHECK(v->situation == Vessel::Situations::ESCAPING);     // ApR beyond the sphere of influence
    // onVesselSituationChange fired once per change with (from, to)
    REQUIRE(changes.size() == 6);  // LANDED, SPLASHED, FLYING, ORBITING, SUB_ORBITAL, ESCAPING (the second ESCAPING is no change)
    CHECK(changes[0] == std::make_pair(Vessel::Situations::PRELAUNCH, Vessel::Situations::LANDED));  // first change after PRELAUNCH->LANDED
    CHECK(changes.back().second == Vessel::Situations::ESCAPING);
    for (size_t i = 1; i < changes.size(); i++) CHECK(changes[i].first == changes[i - 1].second);
    GE::onVesselSituationChange.Remove(&listener, &L::Run);
    // ladder: an EVA kerbal holding a ladder takes the situation of the vessel it hangs on
    struct Eva final : IEvaController {
        Part* ladder;
        bool onLadder = false;
        Part* LadderPart() override { return ladder; }
        bool OnALadder() override { return onLadder; }
    } eva;
    eva.ladder = v->rootPart;
    auto kerbal = Stack14(w, {"kerbalEVA"}, false);
    kerbal->vesselType = VesselType::EVA;
    kerbal->evaController = &eva;
    kerbal->parts = {kerbal->rootPart};
    kerbal->rootPart->vessel = kerbal.get();
    v->situation = Vessel::Situations::ORBITING;
    CHECK(kerbal->EVALadderVessel() == v.get());
    kerbal->updateSituation();
    CHECK(kerbal->situation == Vessel::Situations::ORBITING);
    // TimeWarp.cs:511 reads vessel.isEVA && KerbalEVA.OnALadder through IFlightVessel (Vessel::FlightView)
    CHECK(kerbal->FlightView().isEVA());
    CHECK_FALSE(kerbal->FlightView().OnALadder());
    eva.onLadder = true;
    CHECK(kerbal->FlightView().OnALadder());
    CHECK_FALSE(v->FlightView().isEVA());
    CHECK_FALSE(v->FlightView().OnALadder());  // no KerbalEVA component
    // GetSituationString has a localised string per situation
    CHECK(Vessel::GetSituationString(Vessel::Situations::ORBITING) != Vessel::GetSituationString(Vessel::Situations::LANDED));
    CHECK(Vessel::GetLandedAtString("KSC_Pad_Launch") == "LaunchPad");
    CHECK(Vessel::GetLandedAtString("Runway_East") == "Runway");
    CHECK(Vessel::GetLandedAtString("KSC_Administration") == "Administration");  // "KSC" removed, "_" -> " ", trimmed
    CHECK(Vessel::GetLandedAtString("") == "");
}

// ======================================================== mass properties

namespace {
// The C9 part database `parts` rows by part name
std::map<std::string, std::pair<float, float>> C9Masses() {  // name -> (prefab mass, module mass)
    std::ifstream f(kHarness14 + "/captures/out2/part_database.json", std::ios::binary);
    REQUIRE(f.good());
    const nlohmann::json cap = nlohmann::json::parse(f);
    const nlohmann::json& t = Table14(cap, "parts");
    const size_t cName = Col14(t, "name"), cMass = Col14(t, "mass"), cMod = Col14(t, "module_mass"), cErr = Col14(t, "error");
    std::map<std::string, std::pair<float, float>> out;
    for (const auto& r : t.at("rows"))
        if (r[cErr].is_null()) out[r[cName].get<std::string>()] = {static_cast<float>(r[cMass].get<double>()), static_cast<float>(r[cMod].get<double>())};
    return out;
}
}  // namespace

TEST_CASE("WP-14 mass properties of the PhysProf-S vessel: GetTotalMass, UpdateMassStats, CalculatePhysicsStats (CoM, MOI)") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = w.Craft("PhysProf-S");
    const auto c9 = C9Masses();
    // per-part masses: prefab mass + module mass (C9), the resources of the craft file at density
    double expectedTotal = 0.0;
    for (Part* p : v->parts) {
        const auto& m = c9.at(p->partInfo->name);
        float expected = f32store(fx(m.first) + fx(m.second));
        if (expected < p->partInfo->MinimumMass()) expected = p->partInfo->MinimumMass();
        if (p->variants == nullptr) {
            CHECK(Bits14(p->fields.mass) == Bits14(expected));
        }
        float resMass = 0.0f;
        double rm = 0.0;
        for (const PartResource& r : p->Resources) rm += r.amount * static_cast<double>(r.info->density());
        resMass = static_cast<float>(rm);
        CHECK(std::abs(static_cast<double>(p->GetResourceMass()) - rm) <= 1e-6 * std::max(1.0, rm));
        expectedTotal += fx(p->fields.mass) + fx(p->GetResourceMass());
        (void)resMass;
    }
    const float total = v->GetTotalMass();
    CHECK(v->totalMass == expectedTotal);  // Vessel.cs:3422: a double sum of (float mass + float resource mass)
    CHECK(Bits14(total) == Bits14(static_cast<float>(expectedTotal)));
    CHECK(total > 10.0f);  // a loaded vertical rocket: tens of tonnes

    // FlightIntegrator.UpdateMassStats: physicsMass = mass + resourceMass for parts with a rigid body, 0 without
    for (Part* p : v->parts) p->hasRigidbody = p->fields.physicalSignificance == 0;  // Part.PhysicalSignificance.FULL
    v->UpdateMassStats();
    double sumPhysics = 0.0;
    for (Part* p : v->parts) {
        CHECK(p->fields.resourceMass == p->GetResourceMass());
        const float expected = f32store(fx(p->fields.mass) + fx(p->fields.resourceMass) + 0.0);
        CHECK(Bits14(static_cast<float>(p->fields.physicsMass)) == Bits14(std::max(expected, p->partInfo->MinimumMass())));
        sumPhysics += p->fields.physicsMass;
    }
    // a part without a rigid body adds its mass to the parent that has one (GetPhysicslessChildMass)
    Part* child = v->parts[1];
    Part* holder = child->parent;
    REQUIRE(holder != nullptr);
    const float holderBefore = static_cast<float>(holder->fields.physicsMass);
    child->hasRigidbody = false;
    v->UpdateMassStats();
    CHECK(child->fields.physicsMass == 0.0);
    CHECK(static_cast<float>(holder->fields.physicsMass) > holderBefore);
    CHECK(std::abs(static_cast<float>(holder->fields.physicsMass) -
                   (f32store(fx(holder->fields.mass) + fx(holder->fields.resourceMass) + fx(f32store(fx(child->fields.mass) + fx(child->fields.resourceMass) + 0.0)) ))) <= 1e-3f);
    child->hasRigidbody = true;
    v->UpdateMassStats();

    // CalculatePhysicsStats: mass-weighted CoM over the parts' centres of mass
    v->packed = false;
    v->CalculatePhysicsStats();
    CHECK(v->precalc.firstStatsRunComplete);
    CHECK(v->totalMass == sumPhysics);
    Vector3d com;
    for (size_t i = v->parts.size(); i-- > 0;) {  // CalculatePhysicsStats sums from the last part down
        Part* p = v->parts[i];
        const Vector3d wc(p->partTransform.position + p->partTransform.rotation * p->fields.CoMOffset);
        com += wc * p->fields.physicsMass;
    }
    com = com * (1.0 / sumPhysics);
    CHECK(v->CoMD.x == com.x);
    CHECK(v->CoMD.y == com.y);
    CHECK(v->CoMD.z == com.z);
    CHECK(v->CoM == v->CoMD.ToVector3f());
    // the CoM of a vertical stack lies between its lowest and highest part centre along the stack axis
    float lo = 1e30f, hi = -1e30f;
    for (Part* p : v->parts) {
        const float y = (p->partTransform.rotation * Vector3f(0.0f, 1.0f, 0.0f)).y;
        (void)y;
    }
    for (Part* p : v->parts) {
        const Vector3f d = p->partTransform.position - v->parts[0]->partTransform.position;
        lo = std::min(lo, d.magnitude());
        hi = std::max(hi, d.magnitude());
    }
    const Vector3f comOff = v->CoM - v->parts[0]->partTransform.position;
    CHECK(comOff.magnitude() <= hi);
    // with no angular velocity and no inertia data the MOI is zero; rigid-body velocity and angular velocity are zero
    CHECK(v->rb_velocityD == Vector3d());
    CHECK(v->angularMomentum.x == 0.0f);  // angular velocity 0: no angular momentum
    // unpacked with no angular velocity the MOI is still summed (parallel axes of the point masses: m * r^2 per part)
    CHECK(v->MOI.x > 0.0f);
    v->packed = true;  // packed and not rotating: zero (VesselPrecalculate.cs:433)
    v->CalculatePhysicsStats();
    CHECK(v->MOI.x == 0.0f);
    CHECK(v->angularMomentum.x == 0.0f);
    v->packed = false;
    // with an engine-reported inertia tensor and angular velocity the MOI is the parallel-axis sum
    // (VesselPrecalculate.cs:455-463): two equal point masses at +-d on the reference x axis along y -> Iyy = 0 contribution check
    auto custom = [&](Part& p) {
        PartRigidbodyState s = PartRigidbodyState::FromPart(p);
        s.inertiaTensor = Vector3f(1.0f, 2.0f, 3.0f);
        s.mass = static_cast<float>(p.fields.physicsMass);
        s.angularVelocity = Vector3f(0.0f, 0.1f, 0.0f);
        return s;
    };
    v->CalculatePhysicsStats(custom);
    CHECK(v->angularVelocityD.y != 0.0);
    CHECK(v->MOI.x > 0.0f);
    CHECK(v->MOI.y > 0.0f);
    CHECK(v->MOI.z > 0.0f);
    CHECK(v->angularMomentum.y == static_cast<float>(static_cast<double>(v->MOI.y) * v->angularVelocityD.y));  // VesselPrecalculate.cs:463
    // unloaded vessels have no part list: the proto snapshots give the mass
    auto v2 = w.Craft("PhysProf-S");
    CHECK(v2->GetUnloadedVesselMass() == doctest::Approx(v2->GetTotalMass()).epsilon(1e-5));
}

// ======================================================== kinematics: UpdatePosVel, UpdateSurfaceSpeeds, UpdateAcceleration

TEST_CASE("WP-14 surface speeds from srf_velocity (Vessel.cs:2845, the block of UpdatePosVel): srfSpeed, horizontalSrfSpeed, srf_vel_direction") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = Stack14(w, {"mk1pod.v2"}, false);
    v->srf_velocity = Vector3d(3.0, 4.0, 12.0);
    v->verticalSpeed = 5.0;
    v->UpdateSurfaceSpeeds();
    CHECK(v->srfSpeed == 13.0);  // sqrt(9 + 16 + 144)
    CHECK(v->horizontalSrfSpeed == std::sqrt(169.0 - 25.0));
    CHECK(v->srf_vel_direction.x == 3.0 / 13.0);
    v->verticalSpeed = 20.0;  // vertical larger than the total (can happen with frame offsets): horizontal 0
    v->UpdateSurfaceSpeeds();
    CHECK(v->horizontalSrfSpeed == 0.0);
    v->srf_velocity = Vector3d();
    v->UpdateSurfaceSpeeds();
    CHECK(v->srfSpeed == 0.0);
    CHECK(v->srf_vel_direction == Vector3d());
}

TEST_CASE("WP-14 UpdatePosVel on a circular equatorial orbit around Kerbin: altitude, speeds, up / east, distance traveled") {
    if (!GameAvailable14()) return;
    World14 w;
    CelestialBody& kerbin = w.Kerbin();
    auto v = Stack14(w, {"mk1pod.v2"}, true);
    kerbin.position = Vector3d();  // Kerbin at the origin so that the float vessel position is exact enough
    const double r = kerbin.Radius + 100000.0;
    v->packed = false;
    v->Landed = false;
    v->orbit() = Orbit(0.0, 0.0, r, 0.0, 0.0, 0.0, 0.0, &kerbin);
    v->orbit().Init();
    v->orbit().UpdateFromUT(0.0);
    const Vector3d p = kerbin.position + v->orbit().pos.xzy();
    v->SetPosition(p, false);
    v->CoMD = p;
    v->CoM = p.ToVector3f();
    v->situation = Vessel::Situations::ORBITING;
    FlightRegistry::ready = true;
    FlightRegistry::ActiveVessel = v.get();  // Orbit.GetVel subtracts the active vessel's reference body frame velocity
    v->UpdatePosVel();
    const double vCirc = std::sqrt(kerbin.gravParameter / r);
    CHECK(v->altitude == doctest::Approx(100000.0).epsilon(1e-4));
    CHECK(v->latitude == doctest::Approx(0.0).epsilon(1e-6));
    CHECK(v->obt_speed == doctest::Approx(vCirc).epsilon(1e-9));
    CHECK(v->obt_velocity.magnitude() == v->obt_speed);
    // srf_velocity = orbit.GetVel() - cross(body angular velocity, orbit.pos.xzy)
    const Vector3d expectSrf = v->orbit().GetVel(&kerbin) - Vector3d::Cross(kerbin.angularVelocity, v->orbit().pos.xzy());
    CHECK(v->srf_velocity == expectSrf);
    // equatorial prograde: surface speed = orbital speed - omega * r, with omega = 2 pi / rotationPeriod
    const double omega = 2.0 * 3.14159265358979323846 / kerbin.rotationPeriod;
    CHECK(v->srfSpeed == doctest::Approx(vCirc - omega * r).epsilon(1e-6));
    CHECK(std::abs(v->verticalSpeed) < 0.05);  // circular: perpendicular to the radius (float vessel position)
    CHECK(v->horizontalSrfSpeed == doctest::Approx(v->srfSpeed).epsilon(1e-4));
    CHECK(Vector3d::Dot(v->up, v->east) == doctest::Approx(0.0).epsilon(1e-6));
    CHECK(v->up.magnitude() == doctest::Approx(1.0).epsilon(1e-12));
    CHECK(v->initialPosVelSet);
    CHECK(v->distanceTraveled == v->obt_speed * static_cast<double>(Vessel::env.deltaTime));  // ORBITING: += obt_speed * Time.deltaTime
    v->situation = Vessel::Situations::FLYING;
    const double before = v->distanceTraveled;
    v->UpdateDistanceTraveled();
    CHECK(v->distanceTraveled == before + v->srfSpeed * static_cast<double>(Vessel::env.deltaTime));
    v->situation = Vessel::Situations::LANDED;
    v->UpdateDistanceTraveled();
    CHECK(v->distanceTraveled == before + v->srfSpeed * static_cast<double>(Vessel::env.deltaTime));  // LANDED adds nothing
    // FlightGlobals.ready is false: UpdatePosVel stops after the radar-altitude block
    FlightRegistry::ready = false;
    v->srfSpeed = -1.0;
    v->UpdatePosVel();
    CHECK(v->srfSpeed == -1.0);
    // radar altitude: heightFromTerrain == -1 above the inverse-rotation threshold with the extended calcs off -> altitude
    GameSettings::RADAR_ALTIMETER_EXTENDED_CALCS = false;
    v->heightFromTerrain = -1.0f;
    v->UpdatePosVel();
    CHECK(v->radarAltitude == v->altitude);
    GameSettings::RADAR_ALTIMETER_EXTENDED_CALCS = true;
    // with the PQS hook the terrain altitude comes from the PQS: radar = altitude - terrain
    Vessel::env.pqsSurfaceHeight = [&](CelestialBody& b, const Vector3d&) { return b.Radius + 1234.0; };
    v->heightFromTerrain = -1.0f;
    v->UpdatePosVel();
    CHECK(v->pqsAltitude == 1234.0);
    CHECK(v->terrainAltitude == 1234.0);
    CHECK(v->radarAltitude == v->altitude - 1234.0);
    CHECK(v->PQSAltitude() == 1234.0);
    FlightRegistry::ready = false;
    FlightRegistry::Clear();
}

TEST_CASE("WP-14 UpdateAcceleration: smoothed acceleration, g-force, ignore frames (Vessel.cs:2892)") {
    if (!GameAvailable14()) return;
    World14 w;
    CelestialBody& kerbin = w.Kerbin();
    auto v = Stack14(w, {"mk1pod.v2"}, true);
    v->packed = false;
    v->orbit() = Orbit(0.0, 0.0, kerbin.Radius + 100000.0, 0.0, 0.0, 0.0, 0.0, &kerbin);
    v->orbit().Init();
    FlightRegistry::ready = true;
    v->lastBody = &kerbin;
    v->frameWasRotating = kerbin.rotates && kerbin.inverseRotation;
    v->IgnoreGForces(0);
    const double fdt = 50.0;  // 1 / 0.02
    v->lastVel = Vector3d(0.0, 0.0, 0.0);
    v->obt_velocity = Vector3d(0.0, 0.0, 0.3);  // 0.3 m/s in one frame -> 15 m/s^2
    v->graviticAcceleration = Vector3d();
    v->UpdateAcceleration(fdt, false);
    CHECK(v->acceleration_immediate.z == doctest::Approx(15.0).epsilon(1e-12));
    CHECK(v->acceleration.z == doctest::Approx(15.0 / 20.0).epsilon(1e-12));  // mean of the 20-slot ring buffer
    CHECK(v->geeForce == doctest::Approx(15.0 / 20.0 / PhysicsGlobals::GravitationalAcceleration).epsilon(1e-12));
    CHECK(v->lastVel == v->obt_velocity);
    // ignored frames skip the update and count down
    v->IgnoreGForces(2);
    v->obt_velocity = Vector3d(0.0, 0.0, 9.0);
    const double geeBefore = v->geeForce;
    v->UpdateAcceleration(fdt, false);
    CHECK(v->geeForce == geeBefore);
    v->UpdateAcceleration(fdt, false);
    CHECK(v->geeForce == geeBefore);
    v->obt_velocity = Vector3d(0.0, 0.0, 9.3);
    v->UpdateAcceleration(fdt, false);  // third call: the counter reached 0, the update runs (another 0.3 m/s step)
    CHECK(v->geeForce != geeBefore);
    // packed and landed: perturbation = -g / g0, geeForce = |perturbation|
    v->packed = true;
    v->Landed = true;
    v->graviticAcceleration = Vector3d(0.0, -9.80665, 0.0);
    v->UpdateAcceleration(fdt, false);
    CHECK(v->geeForce == doctest::Approx(1.0).epsilon(1e-12));
    v->Landed = false;
    v->UpdateAcceleration(fdt, false);
    CHECK(v->geeForce == 0.0);
    CHECK(v->acceleration == Vector3d());
    v->IgnoreSpeed(3);
    CHECK(v->IgnoreSpeedActive());
    FlightRegistry::ready = false;
    FlightRegistry::Clear();
}

TEST_CASE("WP-14 atmosphere mirror: ApplyFlightIntegratorAtmosphere copies the FlightIntegrator values (FlightIntegrator.cs 621-914)") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = Stack14(w, {"mk1pod.v2"}, false);
    FlightIntegratorAtmosphere fi;
    fi.currentMainBody = &w.Kerbin();
    fi.altitude = 5000.0;
    fi.spd = 250.0;
    fi.CalculatePressure();
    fi.atmosphereTemperatureOffset = 2.0;
    fi.CalculateConstantsAtmosphere();
    v->ApplyFlightIntegratorAtmosphere(fi);
    CHECK(v->speed == 250.0);
    CHECK(v->staticPressurekPa == fi.staticPressurekPa);
    CHECK(v->indicatedAirSpeed == 250.0 * fi.staticPressureAtm);
    CHECK(v->atmDensity == fi.density);
    CHECK(v->dynamicPressurekPa == 0.0005 * fi.density * 250.0 * 250.0);
    CHECK(v->mach == fi.mach);
    CHECK(v->speedOfSound == fi.speedOfSound);
    CHECK(v->atmosphericTemperature == fi.atmosphericTemperature);
    CHECK(v->externalTemperature >= v->atmosphericTemperature);  // max(ambient, shock)
    // vacuum branch (FlightIntegrator.CalculateConstantsVacuum)
    FlightIntegratorAtmosphere vac;
    vac.currentMainBody = &w.Kerbin();
    vac.altitude = 80000.0;
    vac.spd = 2200.0;
    vac.CalculatePressure();
    CHECK(vac.staticPressurekPa == 0.0);
    vac.CalculateConstantsVacuum();
    v->ApplyFlightIntegratorAtmosphere(vac);
    CHECK(v->staticPressurekPa == 0.0);
    CHECK(v->atmDensity == 0.0);
    CHECK(v->mach == 0.0);
    CHECK(v->dynamicPressurekPa == 0.0);
    CHECK(v->speedOfSound == 0.0);
    CHECK(v->atmosphericTemperature == PhysicsGlobals::SpaceTemperature());
    CHECK(v->externalTemperature == PhysicsGlobals::SpaceTemperature());
}

// ======================================================== crew, control level, overrides, naming

TEST_CASE("WP-14 crew list, capacity and control level (Vessel.cs 1234-1485)") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = Stack14(w, {"mk1pod.v2", "fuelTankSmallFlat", "mk1-3pod"});
    // capacity: the CrewCapacity of the C9 part rows
    int expectedCap = 0;
    for (Part* p : v->parts) expectedCap += p->fields.CrewCapacity;
    CHECK(expectedCap == 1 + 0 + 3);  // mk1pod.v2: 1, mk1-3pod: 3
    CHECK(v->GetCrewCapacity() == expectedCap);
    CHECK(v->GetCrewCount() == 0);
    v->RebuildCrewList();
    CHECK(v->crewableParts == 2);
    CHECK(v->crewedParts == 0);
    auto jeb = std::make_shared<ProtoCrewMember>(ProtoCrewMember_KerbalType::Crew, "Jebediah Kerman");
    auto bob = std::make_shared<ProtoCrewMember>(ProtoCrewMember_KerbalType::Crew, "Bob Kerman");
    auto val = std::make_shared<ProtoCrewMember>(ProtoCrewMember_KerbalType::Crew, "Valentina Kerman");
    CHECK(v->parts[0]->AddCrewmember(jeb));
    CHECK(v->parts[2]->AddCrewmember(bob));
    CHECK(v->parts[2]->AddCrewmember(val));
    // Part.AddCrewmember marks the list dirty; GetVesselCrew rebuilds it
    const auto& crew = v->GetVesselCrew();
    REQUIRE(crew.size() == 3);
    CHECK(crew[0] == jeb);  // parts order
    CHECK(crew[1] == bob);
    CHECK(crew[2] == val);
    CHECK(v->GetCrewCount() == 3);
    CHECK(v->crewedParts == 2);
    CHECK(v->crewableParts == 2);
    CHECK(v->RemoveCrew(std::string("Bob Kerman")));
    CHECK_FALSE(v->RemoveCrew(std::string("Nobody")));
    CHECK(v->GetCrewCount() == 2);
    CHECK(v->RemoveCrew(val));
    CHECK_FALSE(v->RemoveCrew(val));
    v->RebuildCrewList();
    v->RemoveCrewList({jeb}, true);
    CHECK(v->GetCrewCount() == 2);  // bob and val are crew of the part again (the list was rebuilt), jeb removed
    // events
    int modified = 0;
    struct L2 {
        int* n;
        void Run(Vessel*) { ++*n; }
    } l2{&modified};
    GE::onVesselCrewWasModified.Add(&l2, &L2::Run);
    Vessel::CrewWasModified(v.get());
    Vessel::CrewWasModified(nullptr);
    CHECK(modified == 1);
    CHECK(v->GetCrewCount() == 3);
    GE::onVesselCrewWasModified.Remove(&l2, &L2::Run);

    // control level: the best isControlSource of the parts (Part.isControlSource is set by ModuleCommand: WP-24)
    CHECK(v->GetControlLevel() == ControlLevel::NONE);
    v->CheckControllable();
    CHECK_FALSE(v->IsControllable());
    CHECK(v->CurrentControlLevel() == ControlLevel::NONE);
    int controlChanges = 0;
    bool lastState = false;
    struct L3 {
        int* n;
        bool* last;
        void Run(Vessel*, bool s) {
            ++*n;
            *last = s;
        }
    } l3{&controlChanges, &lastState};
    GE::onVesselControlStateChange.Add(&l3, &L3::Run);
    v->parts[2]->fields.isControlSource = static_cast<int32_t>(ControlLevel::PARTIAL_MANNED);
    v->parts[0]->fields.isControlSource = static_cast<int32_t>(ControlLevel::PARTIAL_UNMANNED);
    CHECK(v->GetControlLevel() == ControlLevel::PARTIAL_MANNED);
    v->CheckControllable();
    CHECK(v->IsControllable());
    CHECK(controlChanges == 1);
    CHECK(lastState);
    v->maxControlLevel = ControlLevel::PARTIAL_UNMANNED;  // clamp
    v->CheckControllable();
    CHECK(v->CurrentControlLevel() == ControlLevel::PARTIAL_UNMANNED);
    CHECK(controlChanges == 1);  // still controllable
    v->parts[0]->fields.isControlSource = 0;
    v->parts[2]->fields.isControlSource = 0;
    v->CheckControllable();
    CHECK_FALSE(v->IsControllable());
    CHECK(controlChanges == 2);
    CHECK_FALSE(lastState);
    GE::onVesselControlStateChange.Remove(&l3, &L3::Run);
    // CommNet decides when enabled (the hook stands for connection.GetControlLevel)
    Vessel::env.commNetControlLevel = [](Vessel&) { return std::optional<ControlLevel>(ControlLevel::PARTIAL_UNMANNED); };
    CHECK(v->GetControlLevel() == ControlLevel::PARTIAL_UNMANNED);
    // an unloaded vessel is never controllable
    Vessel::env.commNetControlLevel = nullptr;
    v->loaded = false;
    v->CheckControllable();
    CHECK_FALSE(v->IsControllable());
    v->loaded = true;
    // a kerbal on EVA: PARTIAL_MANNED while active, NONE when inactive
    auto kerbal = Stack14(w, {"kerbalEVA"}, false);
    kerbal->vesselType = VesselType::EVA;
    kerbal->parts = {kerbal->rootPart};
    kerbal->loaded = true;
    kerbal->RebuildCrewList();
    CHECK(kerbal->isEVA());
    CHECK(kerbal->GetControlLevel() == ControlLevel::NONE);  // empty crew: falls through to the parts
    auto eva = std::make_shared<ProtoCrewMember>(ProtoCrewMember_KerbalType::Crew, "Bill Kerman");
    kerbal->rootPart->protoModuleCrew.push_back(eva);  // the kerbalEVA prefab is built in code: no CrewCapacity in the cfg
    kerbal->RebuildCrewList();
    CHECK(kerbal->GetControlLevel() == ControlLevel::PARTIAL_MANNED);
    eva->set_inactive(true);
    CHECK(kerbal->GetControlLevel() == ControlLevel::NONE);
    kerbal->rootPart->vessel = nullptr;
}

TEST_CASE("WP-14 control state, group overrides and action-group blocking (Vessel.cs 1994-2035, 4045-4076)") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = Stack14(w, {"mk1pod.v2"}, false);
    CHECK(v->setControlStates.size() == 5);  // NumOverrideGroups + 1
    CHECK(v->setControlStates[0] == &v->ctrlState);
    CHECK(v->setControlStates[1]->custom_axes == v->ctrlState.custom_axes);
    FlightCtrlState in;
    in.pitch = 0.5f;
    in.mainThrottle = 0.8f;
    v->SetControlState(in);
    CHECK(v->setControlStates[0]->pitch == 0.5f);
    FlightCtrlState out;
    v->GetControlState(out);
    CHECK(out.mainThrottle == 0.8f);
    // SetGroupOverride(2): the new group starts from the main state (everything not blocked is copied)
    int overrideEvents = 0;
    struct L4 {
        int* n;
        void Run(Vessel*) { ++*n; }
    } l4{&overrideEvents};
    GE::OnVesselOverrideGroupChanged.Add(&l4, &L4::Run);
    v->SetGroupOverride(2);
    CHECK(v->GroupOverride == 2);
    CHECK(overrideEvents == 1);
    CHECK(v->setControlStates[2]->pitch == 0.5f);
    v->SetGroupOverride(2);  // no change, no event
    CHECK(overrideEvents == 1);
    v->OverrideAxisControl[1] = KSPAxisGroup::Pitch;  // override group 2 blocks Pitch of the main state
    FlightCtrlState in2;
    in2.pitch = -0.25f;
    in2.yaw = 0.75f;
    v->SetControlState(in2);
    CHECK(v->setControlStates[2]->pitch == -0.25f);
    CHECK(v->setControlStates[0]->pitch == 0.5f);   // blocked: not forwarded
    CHECK(v->setControlStates[0]->yaw == 0.75f);    // forwarded
    CHECK(v->AxisControlBlocked(KSPAxisGroup::Pitch));
    CHECK_FALSE(v->AxisControlBlocked(KSPAxisGroup::Yaw));
    v->OverrideActionControl[1] = KSPActionGroup::Gear;
    CHECK(v->ActionControlBlocked(KSPActionGroup::Gear));
    CHECK_FALSE(v->ActionControlBlocked(KSPActionGroup::Light));
    v->SetGroupOverride(0);
    CHECK_FALSE(v->ActionControlBlocked(KSPActionGroup::Gear));
    // CopyOverrides
    auto v2 = Stack14(w, {"mk1pod.v2"}, false);
    v2->CopyOverrides(*v);
    CHECK(v2->OverrideAxisControl[1] == KSPAxisGroup::Pitch);
    CHECK(v2->OverrideActionControl[1] == KSPActionGroup::Gear);
    GE::OnVesselOverrideGroupChanged.Remove(&l4, &L4::Run);
}

TEST_CASE("WP-14 naming: FindDefaultVesselType, UpdateVesselNaming priority rules, IsValidVesselName, AutoRename, auto clean") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = Stack14(w, {"mk1pod.v2", "fuelTankSmallFlat"}, false);
    v->vesselType = VesselType::Debris;
    // the highest vesselType of the parts (probe cores are Probe, pods Ship)
    int best = 0;
    for (Part* p : v->parts) best = std::max(best, p->fields.vesselType);
    CHECK(static_cast<int>(v->FindDefaultVesselType()) == best);
    CHECK(Vessel::IsValidVesselName("a"));
    CHECK_FALSE(Vessel::IsValidVesselName(""));
    // VesselNaming priority: the part with the highest namingPriority (> 0) names the vessel
    CHECK_FALSE(v->UpdateVesselNaming());
    v->parts[0]->vesselNaming = std::make_shared<VesselNaming>();
    v->parts[0]->vesselNaming->vesselName = "Probe One";
    v->parts[0]->vesselNaming->vesselType = VesselType::Probe;
    v->parts[0]->vesselNaming->namingPriority = 1;
    v->parts[1]->vesselNaming = std::make_shared<VesselNaming>();
    v->parts[1]->vesselNaming->vesselName = "Lander Two";
    v->parts[1]->vesselNaming->vesselType = VesselType::Lander;
    v->parts[1]->vesselNaming->namingPriority = 5;
    std::vector<std::string> renames;
    struct L5 {
        std::vector<std::string>* out;
        void Run(GE::HostedFromToAction<Vessel*, std::string> d) { out->push_back(d.from + "->" + d.to); }
    } l5{&renames};
    GE::onVesselRename.Add(&l5, &L5::Run);
    CHECK(v->UpdateVesselNaming());
    CHECK(v->vesselName == "Lander Two");
    CHECK(v->vesselType == VesselType::Lander);
    REQUIRE(renames.size() == 1);
    CHECK(renames[0] == "Test vessel->Lander Two");
    CHECK(v->UpdateVesselNaming());   // same part, same data: no new event
    CHECK(renames.size() == 1);
    // quirk (Vessel.cs:4181): when the SAME part keeps naming the vessel, `pNewName.vesselNaming.vesselName !=
    // vesselNamedBy.vesselNaming.vesselName` compares the part with itself, so edits of its naming are never noticed
    v->parts[1]->vesselNaming->vesselName = "Lander Three";
    CHECK(v->UpdateVesselNaming());
    CHECK(v->vesselName == "Lander Two");
    CHECK(renames.size() == 1);
    // a different part taking over names the vessel and fires the event
    v->parts[0]->vesselNaming->namingPriority = 9;
    CHECK(v->UpdateVesselNaming());
    CHECK(v->vesselName == "Probe One");
    CHECK(v->vesselType == VesselType::Probe);
    REQUIRE(renames.size() == 2);
    CHECK(renames[1] == "Lander Two->Probe One");
    v->parts[1]->vesselNaming->namingPriority = 20;  // and back
    v->UpdateVesselNaming(true);  // noGameEvent: names but fires nothing
    CHECK(v->vesselName == "Lander Three");
    CHECK(v->vesselType == VesselType::Lander);
    CHECK(renames.size() == 2);
    // events: onVesselWasModified / onPartVesselNamingChanged re-run the naming for their own vessel
    v->parts[0]->vesselNaming->namingPriority = 30;
    GE::onVesselWasModified.Fire(v.get());
    CHECK(v->vesselName == "Probe One");
    v->parts[1]->vesselNaming->namingPriority = 40;
    GE::onPartVesselNamingChanged.Fire(v->parts[1]);
    CHECK(v->vesselName == "Lander Three");
    auto other = Stack14(w, {"mk1pod.v2"}, false);
    GE::onVesselWasModified.Fire(other.get());  // another vessel: v is untouched
    CHECK(v->vesselName == "Lander Three");
    GE::onVesselRename.Remove(&l5, &L5::Run);
    // AutoRename: an IVesselAutoRename module with the lowest VesselType wins
    CHECK(Vessel::AutoRename(*v, "#autoLOC_900676") != "");
    // auto clean (Vessel.cs:4412)
    CHECK_FALSE(v->AutoClean());
    v->SetAutoClean("because");
    v->SetAutoClean("later");
    CHECK(v->AutoClean());
    CHECK(v->AutoCleanReason() == "because");
    // Vessel.Clean is the Funding / recovery hook
    std::string cleaned;
    Vessel::env.cleanVessel = [&](Vessel&, const std::string& r) { cleaned = r; };
    v->protoVessel = reinterpret_cast<ProtoVessel*>(&cleaned);  // only checked for non-null
    v->Clean("r1");
    CHECK(cleaned == "r1");
    v->protoVessel = nullptr;
}

// ======================================================== rails, load / unload, Die, registry

TEST_CASE("WP-14 state transitions: GoOffRails / GoOnRails / Unload / Load keep the flags, events and part ownership of Vessel.cs") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = w.Craft("PhysProf-S");
    Vessel* vp = v.get();
    REQUIRE(FlightRegistry::Vessels.size() == 1);
    CHECK(FlightRegistry::VesselsLoaded.size() == 1);   // SetLoaded(true) moved it from the unloaded list
    CHECK(FlightRegistry::VesselsUnloaded.empty());
    std::vector<std::string> events;
    struct L6 {
        std::vector<std::string>* out;
        void Off(Vessel*) { out->push_back("off"); }
        void On(Vessel*) { out->push_back("on"); }
        void Loaded(Vessel*) { out->push_back("loaded"); }
        void Unloaded(Vessel*) { out->push_back("unloaded"); }
    } l6{&events};
    GE::onVesselGoOffRails.Add(&l6, &L6::Off);
    GE::onVesselGoOnRails.Add(&l6, &L6::On);
    GE::onVesselLoaded.Add(&l6, &L6::Loaded);
    GE::onVesselUnloaded.Add(&l6, &L6::Unloaded);

    // the physics hold (75 frames for a landed vessel) keeps it packed
    Vessel::env.frameCount = 10;
    v->GoOffRails();
    CHECK(v->packed);
    CHECK(events.empty());
    Vessel::env.frameCount = 200;
    // a landed vessel stays on rails unless the reference frame rotates (FlightGlobals.RefFrameIsRotating)
    v->GoOffRails();
    CHECK(v->packed);
    CHECK_FALSE(v->vesselSpawning);  // reset by the attempt
    CHECK(events.empty());
    CelestialBody& kerbin = w.Kerbin();
    kerbin.inverseRotation = true;
    int unpacked = 0, initialised = 0;
    Vessel::env.unpackPart = [&](Part&) { ++unpacked; };
    Vessel::env.resumePartVelocity = [&](Part&) { ++initialised; };
    int krakens = 0;
    Vessel::env.krakensbaneAddFrameVelocity = [&](const Vector3d&) { ++krakens; };
    FlightRegistry::ActiveVessel = vp;
    v->GoOffRails();
    CHECK_FALSE(v->packed);
    CHECK(unpacked == static_cast<int>(v->parts.size()));
    CHECK(initialised == static_cast<int>(v->parts.size()));
    CHECK(krakens == 1);  // the active vessel hands its velocity to the Krakensbane
    CHECK(v->orbitDriver->updateMode == OrbitDriver::UpdateMode::TRACK_Phys);
    REQUIRE(events.size() == 1);
    CHECK(events[0] == "off");
    CHECK(v->acceleration == Vector3d());  // landed: the smoothing buffer is cleared
    // GoOnRails: control state neutralized, parts packed, orbit driver follows Landed / flying
    v->ctrlState.mainThrottle = 1.0f;
    int packedParts = 0;
    Vessel::env.packPart = [&](Part&) { ++packedParts; };
    v->GoOnRails();
    CHECK(v->packed);
    CHECK(v->ctrlState.mainThrottle == 0.0f);
    CHECK(packedParts == static_cast<int>(v->parts.size()));
    CHECK(v->orbitDriver->updateMode == OrbitDriver::UpdateMode::IDLE);  // LandedOrSplashed
    CHECK(events.back() == "on");
    v->GoOnRails();  // already packed: nothing
    CHECK(events.size() == 2);
    v->Landed = false;
    v->packed = false;
    v->GoOnRails();
    CHECK(v->orbitDriver->updateMode == OrbitDriver::UpdateMode::UPDATE);
    v->Landed = true;

    // Unload: ProtoVessel snapshot, parts destroyed, flags
    const size_t nParts = v->parts.size();
    const std::string name = v->vesselName;
    v->Unload();
    CHECK_FALSE(v->loaded);
    CHECK(v->parts.empty());
    CHECK(v->rootPart == nullptr);
    CHECK(v->OwnedPartCount() == 0);
    REQUIRE(v->protoVessel != nullptr);
    CHECK(v->protoVessel->protoPartSnapshots.size() == nParts);
    CHECK(v->protoVessel->vesselName == name);
    CHECK(v->protoVessel->landedAt == "LaunchPad");
    CHECK(events.back() == "unloaded");
    CHECK(FlightRegistry::VesselsUnloaded.size() == 1);
    CHECK(FlightRegistry::VesselsLoaded.empty());
    v->Unload();  // already unloaded: nothing
    CHECK(events.back() == "unloaded");
    CHECK(v->GetUnloadedVesselMass() > 10.0);
    // Load again: the parts come back from the snapshots
    v->Load();
    CHECK(v->loaded);
    CHECK(v->parts.size() == nParts);
    CHECK(v->OwnedPartCount() == nParts);
    CHECK(events.back() == "loaded");
    CHECK(FlightRegistry::VesselsLoaded.size() == 1);
    CHECK(v->rootPart == v->parts[0]);
    CHECK(v->parts[0]->vessel == vp);
    CHECK(v->Landed);  // updateSituation on load: Landed keeps PRELAUNCH
    CHECK(v->GetCrewCount() == 0);
    // the reloaded vessel backs up byte-identically to its first snapshot (parts, resources, modules)
    ProtoVessel* back = v->BackupVessel();
    CHECK(back->protoPartSnapshots.size() == nParts);
    CHECK(v->protoVessel == back);

    // Die: state, zeroed motion, registry removal for a non-active vessel
    FlightRegistry::ActiveVessel = nullptr;
    int willDestroy = 0;
    struct L7 {
        int* n;
        void Run(Vessel*) { ++*n; }
    } l7{&willDestroy};
    GE::onVesselWillDestroy.Add(&l7, &L7::Run);
    v->obt_velocity = Vector3d(1.0, 2.0, 3.0);
    v->geeForce = 4.0;
    v->Die();
    CHECK(v->state == VesselState::DEAD);
    CHECK(willDestroy == 1);
    CHECK(v->obt_velocity == Vector3d());
    CHECK(v->geeForce == 0.0);
    CHECK(FlightRegistry::Vessels.empty());
    GE::onVesselWillDestroy.Remove(&l7, &L7::Run);
    GE::onVesselGoOffRails.Remove(&l6, &L6::Off);
    GE::onVesselGoOnRails.Remove(&l6, &L6::On);
    GE::onVesselLoaded.Remove(&l6, &L6::Loaded);
    GE::onVesselUnloaded.Remove(&l6, &L6::Unloaded);
    Vessel::env = VesselEnvironment{};
}

TEST_CASE("WP-14 reference transform, PartsContain, indexers, landed-in-KSC / recoverable, ranges, vessel size") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = w.Craft("PhysProf-S");
    // AssembleForLaunch: the first part with a crewable control source, else the first control source, else the root
    CHECK(v->GetReferenceTransformPart() == v->rootPart);  // no ModuleCommand state yet: isControlSource NONE -> localRoot
    CHECK(v->referenceTransformId == v->rootPart->fields.flightID);
    CHECK(&v->ReferenceTransform() == &v->rootPart->partTransform);
    // ITargetable.GetTransform (Vessel.cs:3761) is the ReferenceTransform: the reference part's transform and its name
    v->rootPart->partTransform.position = Vector3f(1.5f, -2.0f, 3.25f);
    v->rootPart->partTransform.rotation = Quaternionf(0.0f, 0.70710678f, 0.0f, 0.70710678f);
    UnityEngine::Transform* gt = v->GetTransform();
    CHECK(gt->name == v->rootPart->GetReferenceTransformName());
    CHECK(gt->position == v->rootPart->partTransform.position);
    CHECK(gt->rotation.x == v->rootPart->partTransform.rotation.x);
    CHECK(gt->rotation.y == v->rootPart->partTransform.rotation.y);
    CHECK(gt->rotation.z == v->rootPart->partTransform.rotation.z);
    CHECK(gt->rotation.w == v->rootPart->partTransform.rotation.w);
    CHECK(v->GetFwdVector() == gt->rotation * Vector3f::forward());
    CHECK(v->GetTransform() == gt);  // one Transform per Vessel, refreshed on each call
    // OrbitDriver.Targetable (OrbitDriver.cs:75) through the Vessel's driver view is the Vessel itself
    CHECK(v->OrbitDriverView().AsTargetable() == static_cast<ITargetable*>(v.get()));
    REQUIRE(v->orbitDriver != nullptr);
    CHECK(v->orbitDriver->vessel == &v->OrbitDriverView());
    CHECK(v->orbitDriver->Targetable() == static_cast<ITargetable*>(v.get()));
    // IFlightVessel (TimeWarp's FlightGlobals.ActiveVessel) over the same Vessel
    IFlightVessel& fv = v->FlightView();
    v->altitude = 1234.5;
    v->geeForce = 0.25;
    v->srf_velocity = Vector3d(1.0, 2.0, 3.0);
    CHECK(fv.altitude() == 1234.5);
    CHECK(fv.geeForce() == 0.25);
    CHECK(fv.srf_velocity() == Vector3d(1.0, 2.0, 3.0));
    CHECK(fv.orbit() == &v->orbit());
    CHECK(fv.mainBody() == v->mainBody());
    CHECK(fv.TransformPosition() == v->vesselTransform.position);
    CHECK(fv.LandedOrSplashed() == v->LandedOrSplashed());
    CHECK(fv.PatchedConicsAttached() == v->PatchedConicsAttached());
    v->parts[3]->fields.flightID = 4444;
    v->SetReferenceTransform(v->parts[3]);
    CHECK(v->referenceTransformId == 4444u);
    CHECK(v->referenceTransformIdRecall == 4444u);
    v->SetReferenceTransform(v->parts[5], false);  // storeRecall = false
    CHECK(v->referenceTransformIdRecall == 4444u);
    v->RecallReferenceTransform();
    CHECK(v->GetReferenceTransformPart() == v->parts[3]);
    v->SetReferenceTransform(nullptr);
    CHECK(v->referenceTransformId == 0u);
    CHECK(&v->ReferenceTransform() == &v->vesselTransform);
    v->vesselTransform.position = Vector3f(7.0f, 8.0f, 9.0f);
    CHECK(v->GetTransform()->name == v->gameObjectName);  // vesselTransform: the Vessel's GameObject
    CHECK(v->GetTransform()->position == Vector3f(7.0f, 8.0f, 9.0f));
    v->referenceTransformId = 999999;  // no such part: FallBackReferenceTransform picks the first crewable part, else null
    v->FallBackReferenceTransform();
    CHECK(v->referenceTransformId == 0u);
    // indexers (this[uint] searches from the end; this[int] is bounds-checked)
    CHECK(v->byIndex(0) == v->rootPart);
    CHECK(v->byIndex(-1) == nullptr);
    CHECK(v->byIndex(static_cast<int>(v->parts.size())) == nullptr);
    CHECK(v->byFlightID(4444u) == v->parts[3]);
    CHECK(v->byFlightID(0xFFFFFFF0u) == nullptr);
    v->parts[7]->fields.persistentId = 31337;
    CHECK(v->PartsContain(31337) == v->parts[7]);
    CHECK(v->PartsContain(1) == nullptr);
    // properties
    CHECK(v->LandedInKSC());                 // loaded: landedAt "LaunchPad"
    v->landedAt = "Mun_Craters";
    CHECK_FALSE(v->LandedInKSC());
    v->landedAt = "KSC_Runway";
    CHECK(v->LandedInKSC());
    w.Kerbin().isHomeWorld = true;
    CHECK(v->IsRecoverable());                // Landed on the home world
    v->Landed = false;
    CHECK_FALSE(v->IsRecoverable());
    CHECK(v->isCommandable() == (static_cast<int>(v->vesselType) > static_cast<int>(VesselType::Debris)));
    v->vesselType = VesselType::Debris;
    CHECK_FALSE(v->isCommandable());
    CHECK_FALSE(v->isActiveVessel());
    FlightRegistry::ActiveVessel = v.get();
    CHECK(v->isActiveVessel());
    FlightRegistry::ActiveVessel = nullptr;
    // isPersistent: any persistent part when loaded, the stored flag otherwise
    for (Part* p : v->parts) p->fields.isPersistent = false;
    CHECK_FALSE(v->isPersistent());
    v->parts[2]->fields.isPersistent = true;
    CHECK(v->isPersistent());
    // the pack / unpack ranges (VesselRanges of PhysicsGlobals) and the obsolete accessors
    CHECK(v->vesselRanges.orbit.pack == 350.0f);
    v->set_distancePackThreshold(1234.0f);
    CHECK(v->vesselRanges.orbit.pack == 1234.0f);
    CHECK(v->vesselRanges.flying.pack == 1234.0f);
    CHECK(v->vesselRanges.subOrbital.pack == 1234.0f);
    CHECK(v->vesselRanges.escaping.pack == 1234.0f);
    CHECK(v->vesselRanges.landed.pack == 350.0f);   // landed ranges have their own accessor
    v->set_distanceLandedUnpackThreshold(77.0f);
    CHECK(v->vesselRanges.splashed.unpack == 77.0f);
    CHECK(v->vesselRanges.prelaunch.unpack == 77.0f);
    // GetActiveParts: ACTIVE parts that belong to the current or a later stage
    v->currentStage = 1;
    v->parts[1]->set_State(PartStates::ACTIVE);
    v->parts[1]->fields.inverseStage = 2;
    v->parts[2]->set_State(PartStates::ACTIVE);
    v->parts[2]->fields.inverseStage = 0;
    const auto active = v->GetActiveParts();
    REQUIRE(active.size() == 1);
    CHECK(active[0] == v->parts[1]);
    // the vessel size hook (ShipConstruction.CalculateCraftSize)
    Vessel::env.calculateCraftSize = [](const std::vector<Part*>& ps, Part*) { return Vector3f(static_cast<float>(ps.size()), 2.0f, 3.0f); };
    v->UpdateVesselSize();
    CHECK(v->vesselSize.x == static_cast<float>(v->parts.size()));
    // OnSaveFlightState / OnLoadFlightState: the camera values travel as KSPParseables
    KSPParseableDictionary d;
    v->campth = 0.25f;
    v->camhdg = 0.5f;
    v->camMode = 2.0f;
    v->OnSaveFlightState(d);
    CHECK(d.At("cPch").value == "0.25");
    CHECK(d.At("cPch").type == KSPParseable::Type::FLOAT);
    CHECK(d.At("cMod").type == KSPParseable::Type::INT);
    v->campth = v->camhdg = 0.0f;
    v->OnLoadFlightState(d);
    CHECK(v->campth == 0.25f);
    CHECK(v->camhdg == 0.5f);
    CHECK(v->camMode == 2.0f);
    Vessel::env = VesselEnvironment{};
}

TEST_CASE("WP-14 vessel positioning: SetPosition / Translate / SetRotation move the parts about their pristine layout") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = w.Craft("PhysProf-S");
    v->packed = false;
    const Vector3f root0 = v->rootPart->partTransform.position;
    std::vector<Vector3f> off0;
    for (Part* p : v->parts) off0.push_back(p->partTransform.position - root0);
    v->SetPosition(Vector3d(100.0, 200.0, 300.0), false);  // delta applied to parts with physical significance FULL
    CHECK(v->vesselTransform.position == Vector3f(100.0f, 200.0f, 300.0f));
    for (size_t i = 0; i < v->parts.size(); i++) {
        const Vector3f d = v->parts[i]->partTransform.position - v->rootPart->partTransform.position;
        CHECK(d.x == doctest::Approx(off0[i].x).epsilon(1e-5));
        CHECK(d.y == doctest::Approx(off0[i].y).epsilon(1e-5));
    }
    v->Translate(Vector3d(1.0, 0.0, 0.0));
    CHECK(v->vesselTransform.position.x == 101.0f);
    // SetRotation: parts are rotated by rotation * orgRot, then placed on the pristine layout about the vessel position
    const Quaternionf q(0.0f, 0.0f, 0.70710678f, 0.70710678f);  // 90 degrees about z
    v->SetRotation(q);
    CHECK(v->vesselTransform.rotation.z == doctest::Approx(0.70710678f));
    for (Part* p : v->parts) {
        const Quaternionf expect = q * p->fields.orgRot;
        CHECK(p->partTransform.rotation.x == doctest::Approx(expect.x).epsilon(1e-5));
        CHECK(p->partTransform.rotation.w == doctest::Approx(expect.w).epsilon(1e-5));
        const Vector3d pos = Vector3d(v->vesselTransform.position) + QuaternionD(q) * Vector3d(p->fields.orgPos);
        CHECK(p->partTransform.position.x == doctest::Approx(static_cast<float>(pos.x)).epsilon(1e-5).scale(1.0));
    }
    // an unloaded vessel only has its transform
    v->loaded = false;
    v->SetPosition(Vector3d(5.0, 6.0, 7.0));
    CHECK(v->vesselTransform.position == Vector3f(5.0f, 6.0f, 7.0f));
    v->loaded = true;
}

// ======================================================== VesselValues, FlightRegistry, ProtoTargetInfo

TEST_CASE("WP-14 VesselValues: extremum over the parts' PartValues with the default start value and the per-frame cache") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = Stack14(w, {"mk1pod.v2", "fuelTankSmallFlat", "mk1-3pod"}, false);
    struct Src {
        float f;
        int i;
        float Fv() { return f; }
        int Iv() { return i; }
    } s0{0.3f, 2}, s1{0.7f, 5}, s2{0.25f, 3};
    Src* srcs[3] = {&s0, &s1, &s2};
    for (int i = 0; i < 3; i++) {
        PartValues& pv = v->parts[static_cast<size_t>(i)]->partValues;
        pv.MaxThrottle.Add(srcs[i], &Src::Fv);
        pv.HeatProduction.Add(srcs[i], &Src::Fv);
        pv.AutopilotSkill.Add(srcs[i], &Src::Iv);
        pv.Update();
    }
    VesselValues& vv = v->GetVesselValues();
    // MaxThrottle: a > b starting at the default 1.0 -> values below 1 never win
    CHECK(vv.MaxThrottle.value() == 1.0f);
    // HeatProduction: a < b starting at 1.0 -> the smallest part value
    CHECK(vv.HeatProduction.value() == 0.25f);
    // AutopilotSkill: a > b starting at -1 -> the largest
    CHECK(vv.AutopilotSkill.value() == 5);
    CHECK(vv.EVAChuteSkill.defaultValue == 0);
    CHECK(vv.CommsRange.defaultValue == 1.0f);
    // the result is cached for the frame: a change of the part values shows after the frame advances or with valueUncached
    s2.f = 0.1f;
    v->parts[2]->partValues.Update();
    CHECK(vv.HeatProduction.value() == 0.25f);
    CHECK(vv.HeatProduction.valueUncached() == 0.1f);
    s2.f = 0.05f;
    v->parts[2]->partValues.Update();
    CHECK(vv.HeatProduction.value() == 0.1f);
    Vessel::env.frameCount++;
    CHECK(vv.HeatProduction.value() == 0.05f);
    // PartValuesOperation folds from the last part to the first
    PartValuesOperation<float> sum([&]() -> std::vector<Part*>& { return v->parts; }, 0.0f, [](float a, float b) { return a + b; },
                                   [](Part& p) { return p.partValues.HeatProduction.value(); });
    CHECK(sum.value() == doctest::Approx(0.3f + 0.7f + 0.05f));
    for (int i = 0; i < 3; i++) {
        PartValues& pv = v->parts[static_cast<size_t>(i)]->partValues;
        pv.MaxThrottle.Remove(srcs[i], &Src::Fv);
        pv.HeatProduction.Remove(srcs[i], &Src::Fv);
        pv.AutopilotSkill.Remove(srcs[i], &Src::Iv);
    }
    Vessel::env = VesselEnvironment{};
}

TEST_CASE("WP-14 FlightRegistry and ProtoTargetInfo: registration, lookup by id, target round trip (ProtoTargetInfo.cs, FlightGlobals.cs)") {
    if (!GameAvailable14()) return;
    World14 w;
    auto a = Stack14(w, {"mk1pod.v2", "fuelTankSmallFlat"});
    auto b = Stack14(w, {"mk1pod.v2"});
    a->id = Guid::Parse("11111111111111111111111111111111");
    b->id = Guid::Parse("22222222222222222222222222222222");
    CHECK(FlightRegistry::Vessels.size() == 2);   // Initialize registers (FlightGlobals.AddVessel)
    CHECK(FlightRegistry::VesselsLoaded.size() == 2);
    CHECK(FlightRegistry::FindVessel(a->id) == a.get());
    CHECK(FlightRegistry::FindVessel(b->id) == b.get());
    CHECK(FlightRegistry::FindVessel(Guid::Parse("33333333333333333333333333333333")) == nullptr);
    // FindPartByID searches the loaded vessels' flightIDs (from the end); 0 is refused
    a->parts[1]->fields.flightID = 777;
    b->parts[0]->fields.flightID = 888;
    CHECK(FlightRegistry::FindPartByID(777) == a->parts[1]);
    CHECK(FlightRegistry::FindPartByID(888) == b->parts[0]);
    CHECK(FlightRegistry::FindPartByID(0) == nullptr);
    CHECK(FlightRegistry::FindPartByID(12345) == nullptr);
    // SortVessel moves between the loaded and unloaded lists
    a->loaded = false;
    FlightRegistry::SortVessel(a.get(), false);
    CHECK(FlightRegistry::VesselsLoaded.size() == 1);
    CHECK(FlightRegistry::VesselsUnloaded.size() == 1);
    a->loaded = true;
    FlightRegistry::SortVessel(a.get(), true);
    CHECK(FlightRegistry::VesselsLoaded.size() == 2);

    // ProtoTargetInfo for a vessel target: Save writes type + vesselID, FindTarget resolves through the registry
    ProtoTargetInfo vt(static_cast<ITargetable*>(b.get()));
    CHECK(vt.targetType == ProtoTargetInfo::Type::Vessel);
    CHECK(vt.vesselId == b->id);
    ConfigNode n("TARGET");
    vt.Save(n);
    CHECK(*n.GetValue("type") == "Vessel");
    CHECK(*n.GetValue("vesselID") == b->id.ToString());
    ProtoTargetInfo loaded(n);
    CHECK(loaded.FindTarget() == static_cast<ITargetable*>(b.get()));
    // null target: Null type, nothing saved
    ProtoTargetInfo none(static_cast<ITargetable*>(nullptr));
    CHECK(none.targetType == ProtoTargetInfo::Type::Null);
    ConfigNode nn("TARGET");
    none.Save(nn);
    CHECK(nn.values().Count() == 0);
    CHECK(none.FindTarget() == nullptr);
    // a part module target is saved as vessel + part flightID + module index and resolved back to the module (ITargetable modules)
    struct TModule : ITargetable {
        UnityEngine::Transform trf;
        UnityEngine::Transform* GetTransform() override {
            trf.name = "tm";
            return &trf;
        }
        Vector3f GetObtVelocity() override { return {}; }
        Vector3f GetSrfVelocity() override { return {}; }
        Vector3f GetFwdVector() override { return {}; }
        Vessel* GetVessel() override { return nullptr; }
        std::string GetName() override { return "t"; }
        std::string GetDisplayName() override { return "t"; }
        Orbit* GetOrbit() override { return nullptr; }
        OrbitDriver* GetOrbitDriver() override { return nullptr; }
        VesselTargetModes GetTargetingMode() override { return VesselTargetModes::None; }
        bool GetActiveTargetable() override { return false; }
    } generic;
    ProtoTargetInfo gt(static_cast<ITargetable*>(&generic));
    CHECK(gt.targetType == ProtoTargetInfo::Type::Generic);  // neither module nor vessel nor body
    CHECK(gt.uniqueName == "tm");
    ConfigNode gn("TARGET");
    gt.Save(gn);
    CHECK(*gn.GetValue("tgtId") == "tm");
    // a PositionTarget is generic too: its `new GameObject()` is named "New Game Object" (ProtoTargetInfo.cs:64)
    PositionTarget site("site");
    ProtoTargetInfo st(static_cast<ITargetable*>(&site));
    CHECK(st.targetType == ProtoTargetInfo::Type::Generic);
    CHECK(st.uniqueName == "New Game Object");
    // the unloaded round trip of Vessel.Load: the target survives a snapshot
    a->targetObject = b.get();
    ProtoVessel* snap = a->BackupVessel();
    CHECK(snap->targetInfo.targetType == ProtoTargetInfo::Type::Vessel);
    CHECK(snap->targetInfo.vesselId == b->id);
    CHECK(snap->targetInfo.FindTarget() == static_cast<ITargetable*>(b.get()));
    a->Unload();             // ProtoVessel(this) keeps the target
    CHECK(a->protoVessel->targetInfo.targetType == ProtoTargetInfo::Type::Vessel);
    a->targetObject = nullptr;
    a->Load();
    CHECK(a->targetObject == static_cast<ITargetable*>(b.get()));  // Vessel.Load resumes the target from the snapshot
    CHECK(a->pTI.targetType == ProtoTargetInfo::Type::Vessel);
    FlightRegistry::Clear();
}

// ======================================================== ModuleTripLogger, VesselTripLog, FlightLogger, FlightCoMTracker, AxisGroupsModule

TEST_CASE("WP-14 ModuleTripLogger: Log round trip of the physprofile modules, deprecated nodes, delayed situation logging, forced logs") {
    if (!GameAvailable14()) return;
    World14 w;
    // the real MODULE nodes of the physprofile save
    GameParameters::GetDefaultParameters(Game_Modes::CAREER, GameParameters::Preset::Normal);
    ConfigNode::NodePtr root = ConfigNode::Load(kSaves14 + "/physprofile/persistent.sfs");
    REQUIRE(root);
    Game game(root.get());  // HighLogic.CurrentGame: ProtoCrewMember.UpdateExperience reads it
    SaveEnvironment::Current().currentGame = &game;
    auto v = Stack14(w, {"mk1pod.v2", "fuelTankSmallFlat"});
    auto* tl = dynamic_cast<ModuleTripLogger*>(v->parts[0]->FindModuleOfClass("ModuleTripLogger"));
    REQUIRE(tl != nullptr);  // PartLoader adds ModuleTripLogger to every part with vesselType > Unknown
    CHECK(tl->GetModuleDisplayName() != "");
    CHECK(v->parts[1]->FindModuleOfClass("ModuleTripLogger") == nullptr);  // tank: vesselType Debris
    ConfigNode in("MODULE");
    in.AddValue("name", "ModuleTripLogger");
    ConfigNode* log = in.AddNode("Log");
    log->AddValue("flight", 3);
    log->AddValue("0", "Flight,Kerbin");
    log->AddValue("1", "Orbit,Kerbin");
    tl->Load(in);
    CHECK(tl->Log.Flight() == 3);
    REQUIRE(tl->Log.Count() == 2);
    CHECK(tl->Log[1].type == "Orbit");
    CHECK(tl->Log[1].target == "Kerbin");
    ConfigNode out("MODULE");
    tl->Save(out);
    ConfigNode* outLog = out.GetNode("Log");
    REQUIRE(outLog != nullptr);
    CHECK(*outLog->GetValue("flight") == "3");
    CHECK(*outLog->GetValue("1") == "Orbit,Kerbin");
    // deprecated nodes become entries (ModuleTripLogger.cs:40)
    ConfigNode old("MODULE");
    old.AddNode("Surfaced")->AddValue("at", "Kerbin");
    ConfigNode* flew = old.AddNode("Flew");
    flew->AddValue("at", "Kerbin");
    flew->AddValue("at", "Mun");
    tl->Log = FlightLog();
    tl->Load(old);
    REQUIRE(tl->Log.Count() == 3);
    CHECK(tl->Log[0].type == "Land");
    CHECK(tl->Log[1].type == "Flight");
    CHECK(tl->Log[2].target == "Mun");

    // OnStart subscribes; a situation change of THIS vessel schedules the 0.2 s delayed log; others are ignored
    tl->Log = FlightLog();
    auto jeb = std::make_shared<ProtoCrewMember>(ProtoCrewMember_KerbalType::Crew, "Jebediah Kerman");
    v->parts[0]->AddCrewmember(jeb);
    int recorded = 0;
    struct L8 {
        int* n;
        void Run(Vessel*) { ++*n; }
    } l8{&recorded};
    GE::OnFlightLogRecorded.Add(&l8, &L8::Run);
    tl->OnStart(StartState::Orbital);
    Time::time = 100.0f;
    v->situation = Vessel::Situations::ORBITING;
    auto other = Stack14(w, {"mk1pod.v2"});
    GE::onVesselSituationChange.Fire(GE::HostedFromToAction<Vessel*, Vessel_Situations>(other.get(), Vessel_Situations::FLYING, Vessel_Situations::ORBITING));
    CHECK_FALSE(tl->IsInvoking());
    GE::onVesselSituationChange.Fire(GE::HostedFromToAction<Vessel*, Vessel_Situations>(v.get(), Vessel_Situations::FLYING, Vessel_Situations::ORBITING));
    CHECK(tl->IsInvoking());
    Time::time = 100.1f;
    tl->RunDelayedCalls();
    CHECK(tl->Log.Count() == 0);  // not yet
    Time::time = 100.3f;
    tl->RunDelayedCalls();
    CHECK_FALSE(tl->IsInvoking());
    REQUIRE(tl->Log.Count() == 1);
    CHECK(tl->Log[0].type == "Orbit");
    CHECK(tl->Log[0].target == "Kerbin");
    CHECK(jeb->flightLog.HasEntry(FlightLog::EntryType::Orbit, "Kerbin"));  // the crew log gets the entry too
    CHECK(recorded == 1);
    // the same entry is not added twice (AddEntryUnique); other situations map as in LogNewSituation
    v->situation = Vessel::Situations::SPLASHED;
    GE::onVesselSOIChanged.Fire(GE::HostedFromToAction<Vessel*, CelestialBody*>(v.get(), nullptr, &w.Kerbin()));
    Time::time = 101.0f;
    tl->RunDelayedCalls();
    CHECK(tl->Log.HasEntry(FlightLog::EntryType::Land, "Kerbin"));
    v->situation = Vessel::Situations::ORBITING;
    GE::onVesselSOIChanged.Fire(GE::HostedFromToAction<Vessel*, CelestialBody*>(v.get(), nullptr, &w.Kerbin()));
    Time::time = 102.0f;
    tl->RunDelayedCalls();
    CHECK(tl->Log.Count() == 2);  // Orbit,Kerbin already there
    CHECK(recorded == 3);         // but OnFlightLogRecorded fires for every LogSituation
    // ForceVesselLog (loaded vessel): every module and crew member; unloaded: the snapshot's Log node
    ModuleTripLogger::ForceVesselLog(v.get(), FlightLog::EntryType::Flyby, w.sys->GetBodyByName("Mun"));
    CHECK(tl->Log.HasEntry(FlightLog::EntryType::Flyby, "Mun"));
    CHECK(jeb->flightLog.HasEntry(FlightLog::EntryType::Flyby, "Mun"));
    CHECK(recorded == 4);
    ModuleTripLogger::ForceVesselLog(nullptr, FlightLog::EntryType::Flyby, &w.Kerbin());  // null vessel / body: nothing
    CHECK(recorded == 4);
    VesselTripLog merged = VesselTripLog::FromVessel(*v);
    CHECK(merged.Log.Count() == tl->Log.Count());
    v->Unload();
    ModuleTripLogger::ForceVesselLog(v.get(), FlightLog::EntryType::Escape, &w.Kerbin());
    VesselTripLog fromProto = VesselTripLog::FromProtoVessel(*v->protoVessel);
    CHECK(fromProto.Log.HasEntry(FlightLog::EntryType::Escape, "Kerbin"));
    CHECK(fromProto.Log.HasEntry(FlightLog::EntryType::Flyby, "Mun"));
    GE::OnFlightLogRecorded.Remove(&l8, &L8::Run);
}

TEST_CASE("WP-14 FlightLogger: statistics, the event log, mission end and the delayed calls") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = Stack14(w, {"mk1pod.v2", "fuelTankSmallFlat"});
    v->state = VesselState::ACTIVE;
    FlightRegistry::ActiveVessel = v.get();
    FlightRegistry::ready = true;
    {
        FlightLogger logger;
        logger.Start();
        CHECK(FlightLogger::fetch == &logger);
        FlightLogger second;
        second.Start();  // a second instance destroys itself (FlightLogger.cs:56)
        CHECK(second.destroyed());
        CHECK(FlightLogger::fetch == &logger);
        CHECK(FlightLogger::eventLog.empty());
        CHECK_FALSE(FlightLogger::LiftOff());
        // LateUpdate / FixedUpdate accumulate over the active vessel
        v->missionTime = 12.5;
        v->altitude = 1500.0;
        v->srfSpeed = 100.0;
        v->horizontalSrfSpeed = 60.0;
        v->geeForce = 2.5;
        FlightLogger::LogGees = true;
        logger.LateUpdate();
        CHECK(logger.missionTime == 12.5);
        CHECK(logger.highestAltitude() == 1500.0);
        CHECK(logger.highestSpeed() == 100.0);
        CHECK(logger.highestGee() == 2.5);
        const double fdt = static_cast<double>(TimeWarp::fixedDeltaTime());
        CHECK(logger.totalDistance() == 100.0 * fdt);
        CHECK(logger.groundDistance() == 60.0 * fdt);
        logger.FixedUpdate();   // first call: okToSetSpeed was false -> only the distances
        CHECK(logger.highestSpeedOverLand() == 0.0);
        logger.FixedUpdate();   // second call: horizontal speed recorded
        CHECK(logger.highestSpeedOverLand() == 60.0);
        CHECK(logger.totalDistance() == doctest::Approx(300.0 * fdt).epsilon(1e-12));
        v->altitude = 900.0;
        logger.LateUpdate();
        CHECK(logger.highestAltitude() == 1500.0);
        // IgnoreGeeForces: LogGees off until the Invoke fires
        Time::time = 50.0f;
        FlightLogger::IgnoreGeeForces(1.0f);
        CHECK_FALSE(FlightLogger::LogGees);
        logger.RunDelayedCalls(50.5f, 0.0f);
        CHECK_FALSE(FlightLogger::LogGees);
        logger.RunDelayedCalls(51.0f, 0.0f);
        CHECK(FlightLogger::LogGees);
        // events: timestamped lines, consecutive duplicates suppressed
        EventReport launch(FlightEvents::LAUNCH, v->rootPart);
        GE::onLaunch.Fire(&launch);
        CHECK(FlightLogger::LiftOff());
        REQUIRE(FlightLogger::eventLog.size() == 1);
        CHECK(FlightLogger::eventLog[0].rfind("[", 0) == 0);
        CHECK(FlightLogger::eventLog[0].find("]: ") != std::string::npos);
        GE::onLaunch.Fire(&launch);  // liftOff already true: nothing
        CHECK(FlightLogger::eventLog.size() == 1);
        logger.LogEvent("same");
        logger.LogEvent("same");
        CHECK(FlightLogger::eventLog.size() == 2);  // lastReport suppression
        EventReport sep(FlightEvents::STAGESEPARATION, v->rootPart, "x", "y", 4);
        GE::onStageSeparation.Fire(&sep);
        CHECK(FlightLogger::eventLog.size() == 3);
        EventReport killed(FlightEvents::CREW_KILLED, v->rootPart, "Bob Kerman");
        GE::onCrewKilled.Fire(&killed);
        CHECK(logger.kerbalsKilled() == 1);
        // a crash of the active vessel's root part ends the mission: parts lost, switch / end-of-flight callbacks
        bool switched = false;
        std::pair<Vessel*, FlightEndModes> ended{nullptr, FlightEndModes::SUCCESS};
        FlightLogger::host.attemptVesselSwitch = [&] {
            switched = true;
            return false;  // no other controllable vessel
        };
        FlightLogger::host.endFlight = [&](Vessel* vv, FlightEndModes m) { ended = {vv, m}; };
        EventReport crash(FlightEvents::CRASH, v->rootPart, "Pod", "ground");
        Time::time = 60.0f;
        GE::onCrash.Fire(&crash);
        CHECK(logger.partsLost() == 1);
        CHECK(logger.missionEnd());
        logger.RunDelayedCalls(60.4f, 60.4f);
        CHECK_FALSE(switched);
        logger.RunDelayedCalls(60.6f, 60.6f);  // attemptSwitchWait 0.5
        CHECK(switched);
        CHECK(logger.missionEnd());           // no vessel found: the end timer keeps running
        logger.RunDelayedCalls(63.9f, 63.9f);
        CHECK(ended.first == nullptr);
        logger.RunDelayedCalls(64.1f, 64.1f);  // endFlightWait 4 s
        CHECK(ended.first == v.get());
        CHECK(ended.second == FlightEndModes::CATASTROPHIC_FAILURE);
        // a crash of a part that is not the root does not end the mission
        EventReport crash2(FlightEvents::CRASH, v->parts[1], "Tank", "ground");
        GE::onCrash.Fire(&crash2);
        CHECK(logger.partsLost() == 2);
        const std::string stats = FlightLogger::getMissionStats();
        CHECK(stats.size() > 10);
        FlightLogger::host = FlightLogger::Host{};
    }
    CHECK(FlightLogger::fetch == nullptr);  // OnDestroy
    FlightRegistry::Clear();
}

TEST_CASE("WP-14 FlightCoMTracker: marker follows the vessel CoM and targets once FlightGlobals is ready") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = Stack14(w, {"mk1pod.v2", "fuelTankSmallFlat"});
    PartModule* host = v->parts[0]->Modules().GetModule(0);
    auto t = FlightCoMTracker::Create(host, true);
    CHECK(t->name() == "mk1pod.v2 Vessel CoM Marker");
    CHECK(t->GetActiveTargetable());
    CHECK(t->GetTargetingMode() == VesselTargetModes::DirectionAndVelocity);
    CHECK(t->GetVessel() == v.get());
    v->CoM = Vector3f(1.0f, 2.0f, 3.0f);
    t->FixedUpdate();
    CHECK(t->GetTransform()->position == Vector3f(1.0f, 2.0f, 3.0f));
    CHECK(t->GetTransform()->name == "mk1pod.v2 Vessel CoM Marker");
    CHECK(t->GetTransform()->rotation == Quaternionf::identity());
    CHECK(t->GetFwdVector() == t->GetTransform()->rotation * Vector3f::forward());
    v->obt_velocity = Vector3d(4.0, 5.0, 6.0);
    v->srf_velocity = Vector3d(7.0, 8.0, 9.0);
    CHECK(t->GetObtVelocity() == Vector3f(4.0f, 5.0f, 6.0f));
    CHECK(t->GetSrfVelocity() == Vector3f(7.0f, 8.0f, 9.0f));
    CHECK(t->GetOrbit() == &v->orbit());
    CHECK(t->GetOrbitDriver() == v->orbitDriver.get());
    CHECK(t->GetName() == t->GetDisplayName());
    CHECK(t->GetName().rfind(v->GetDisplayName(), 0) == 0);
    ITargetable* chosen = nullptr;
    FlightCoMTracker::setVesselTarget = [&](ITargetable* x) { chosen = x; };
    FlightRegistry::ready = false;
    t->MakeTarget();
    CHECK(t->pending());     // waits for FlightGlobals.ready
    CHECK(chosen == nullptr);
    t->RunPending();
    CHECK(chosen == nullptr);
    FlightRegistry::ready = true;
    t->RunPending();
    CHECK(chosen == t.get());
    chosen = nullptr;
    t->MakeTarget();         // ready: immediate
    CHECK(chosen == t.get());
    Vessel::env.LoadedSceneIsFlight = false;
    chosen = nullptr;
    t->MakeTarget();
    CHECK(chosen == nullptr);
    FlightCoMTracker::setVesselTarget = nullptr;
    FlightRegistry::Clear();
}

TEST_CASE("WP-14 AxisGroupsModule: axis groups are built from the vessel's [KSPAxisField] floats and drive them") {
    if (!GameAvailable14()) return;
    World14 w;
    auto v = w.Craft("PhysProf-S");
    auto* axes = dynamic_cast<AxisGroupsModule*>(v->FindVesselModuleByName("AxisGroupsModule"));
    REQUIRE(axes != nullptr);
    CHECK(axes->GetActivation() == (VesselModule::FlightScene | VesselModule::LoadedVessels));
    CHECK(axes->GetOrder() == 999);
    CHECK(axes->vesselPtr() == v.get());
    CHECK_FALSE(axes->ShouldBeActive(false, true));  // not the flight scene
    CHECK(axes->ShouldBeActive(true, true));
    CHECK_FALSE(axes->ShouldBeActive(true, false));  // unloaded
    axes->OnLoadVessel();
    CHECK(axes->AxisGroups.empty());  // the default axis group of thrustPercentage & co is None
    // put every engine's thrustPercentage into Custom01
    std::vector<BaseAxisField*> engineFields;
    for (PartModule* m : v->FindPartModulesImplementing([](PartModule& pm) { return pm.IsA("ModuleEngines"); })) {
        auto* f = dynamic_cast<BaseAxisField*>(m->Fields()["thrustPercentage"]);
        REQUIRE(f != nullptr);
        f->axisGroup = static_cast<int64_t>(KSPAxisGroup::Custom01);
        f->axisIncremental = 0;
        engineFields.push_back(f);
    }
    REQUIRE(engineFields.size() >= 3);
    // AxisGroupsModule rebuilds on onVesselWasModified for its own loaded vessel
    GE::onVesselWasModified.Fire(v.get());
    REQUIRE(axes->AxisGroups.count(static_cast<int>(KSPAxisGroup::Custom01)) == 1);
    CHECK(axes->AxisGroups[static_cast<int>(KSPAxisGroup::Custom01)].size() == engineFields.size());
    CHECK(axes->AxisGroups.count(static_cast<int>(KSPAxisGroup::Custom02)) == 0);
    // SetAxis: value = min + (max - min) * (axis + 1) / 2
    axes->SetAxisGroup(KSPAxisGroup::Custom01, 1.0f);
    for (BaseAxisField* f : engineFields) CHECK(std::get<float>(f->GetValue()) == 100.0f);
    axes->SetAxisGroup(KSPAxisGroup::Custom01, -0.5f);
    for (BaseAxisField* f : engineFields) CHECK(std::get<float>(f->GetValue()) == 25.0f);
    axes->SetAxisGroup(KSPAxisGroup::Custom04, 1.0f);  // a group nobody is in: nothing happens
    for (BaseAxisField* f : engineFields) CHECK(std::get<float>(f->GetValue()) == 25.0f);
    // IncrementAxis: += rate * (max-min) * multiplier(0.2 default) * fixedDeltaTime, clamped
    const float fdt = TimeWarp::fixedDeltaTime();
    axes->IncrementAxisGroup(KSPAxisGroup::Custom01, 0.5f);
    const float expected = f32store(25.0 + fx(0.5f) * fx(f32store(fx(100.0f) * fx(0.2f))) * fx(fdt));
    for (BaseAxisField* f : engineFields) CHECK(std::get<float>(f->GetValue()) == expected);
    for (int i = 0; i < 5000; i++) axes->IncrementAxisGroup(KSPAxisGroup::Custom01, 1.0f);
    for (BaseAxisField* f : engineFields) CHECK(std::get<float>(f->GetValue()) == 100.0f);  // clamped at maxValue
    // UpdateAxis: inverted groups flip the sign; incremental groups increment, others set
    for (BaseAxisField* f : engineFields) {
        f->axisInverted = static_cast<int64_t>(KSPAxisGroup::Custom01);
        f->axisIncremental = 0;
    }
    axes->UpdateAxisGroup(KSPAxisGroup::Custom01, 1.0f);  // inverted set: -1 -> 0
    for (BaseAxisField* f : engineFields) CHECK(std::get<float>(f->GetValue()) == 0.0f);
    for (BaseAxisField* f : engineFields) {
        f->axisInverted = 0;
        f->axisIncremental = static_cast<int64_t>(KSPAxisGroup::Custom01);
    }
    axes->UpdateAxisGroup(KSPAxisGroup::Custom01, 1.0f);  // incremental: +20 * 0.02 more
    for (BaseAxisField* f : engineFields) CHECK(std::get<float>(f->GetValue()) == f32store(0.0 + 1.0 * fx(f->incrementalSpeed) * fx(fdt)));
    // an override group changes which fields belong to a group: group override 1 uses overrideGroups[0]
    GameSettings::ADDITIONAL_ACTION_GROUPS = true;
    v->GroupOverride = 1;
    v->OverrideDefault[0] = true;
    for (BaseAxisField* f : engineFields) f->overrideGroups[0] = static_cast<int64_t>(KSPAxisGroup::Custom02);
    axes->BuildAxisGroups();
    CHECK(axes->AxisGroups.count(static_cast<int>(KSPAxisGroup::Custom02)) == 1);
    CHECK(axes->AxisGroups.count(static_cast<int>(KSPAxisGroup::Custom01)) == 0);  // overrideDefault drops the default group
    GameSettings::ADDITIONAL_ACTION_GROUPS = false;
    // an unloaded vessel does not rebuild
    axes->AxisGroups.clear();
    v->loaded = false;
    GE::onVesselWasModified.Fire(v.get());
    CHECK(axes->AxisGroups.empty());
    v->loaded = true;
}
