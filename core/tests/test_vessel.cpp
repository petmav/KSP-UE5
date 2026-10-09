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
        ao.rootPosition = Kerbin().GetWorldSurfacePosition(-0.0972077486779459, -74.5576813534722, 75.0).ToVector3f();
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
