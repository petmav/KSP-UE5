// WP-54 slice 2 unit tests: the mission data classes, the vessel restrictions, the deferred Tests.* and the create Actions
// (core/include/ksp/missions/{mission_data,vessel_restrictions,mission_tests2,mission_actions2}.hpp).
//
// No golden and no capture of the mission runtime exists and the mission files of the GameData mirror are 0-byte files (see STATUS WP-54), so every
// expectation of the "reference" cases comes from tools/wp54b/ref54b.py (independent Python written from the decompiled C#,
// tests/data/wp54b_reference.json). The scenarios are labelled synthetic inputs to the algorithms (a synthetic body list, seeded configs), not game
// data. Floats compare bit-exact; text goes through the real Squad/Localization/dictionary.cfg. The only tolerance is on values that depend on the
// double libm (acos / cos / sin / pow of the great-circle distance and the sphere test): booleans are compared exactly (the scenarios keep a margin
// to the boundary) and the accuracy values within 1e-9 relative.
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/missions.hpp"
#include "ksp/missions/mission_actions2.hpp"
#include "ksp/missions/mission_data.hpp"
#include "ksp/missions/mission_tests2.hpp"
#include "ksp/missions/vessel_restrictions.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/modules/misc_modules.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/science/science.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"
#include "missions_fixture.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using namespace ksp::Expansions::Missions;
using json = nlohmann::json;
using namespace ksp::mtest;

namespace {

const json& Ref() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp54b_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

ConfigNode CfgFromJson(const json& j) {
    ConfigNode n(j["name"].get<std::string>());
    for (const auto& kv : j["values"]) n.AddValue(kv[0].get<std::string>(), kv[1].get<std::string>());
    for (const auto& c : j["nodes"]) {
        ConfigNode child = CfgFromJson(c);
        ConfigNode* added = n.AddNode(child.name);
        for (const auto& v : child.values()) added->AddValue(v.name, v.value);
        for (int i = 0; i < child.nodes().Count(); ++i) {
            ConfigNode* g = added->AddNode(child.nodes()[i]->name);
            for (const auto& v : child.nodes()[i]->values()) g->AddValue(v.name, v.value);
            for (int k = 0; k < child.nodes()[i]->nodes().Count(); ++k) {
                ConfigNode* h = g->AddNode(child.nodes()[i]->nodes()[k]->name);
                for (const auto& v : child.nodes()[i]->nodes()[k]->values()) h->AddValue(v.name, v.value);
            }
        }
    }
    return n;
}

std::string SaveDump(const std::function<void(ConfigNode&)>& save) {
    ConfigNode out("N");
    save(out);
    return Dump(out);
}

// the world of every case: the synthetic bodies of the reference (round stand-ins, labelled synthetic), the localizer, the mission environment
struct Fixture54b : Fixture54 {
    Game game;
    std::vector<std::unique_ptr<CelestialBody>> bodies;
    std::unique_ptr<PartResourceLibrary> lib;
    std::shared_ptr<KerbalRoster> roster;
    Fixture54b() {
        KSPUtil::CacheLocalStrings();
        NodeLibraryEnvironment::Reset();
        MissionDataEnvironment::Reset();
        RestrictionEnvironment::Reset();
        MissionTestsEnvironment::Reset();
        ActionCreateEnvironment::Reset();
        int idx = 0;
        for (const auto& bj : Ref()["bodies"]) {
            auto b = std::make_unique<CelestialBody>();
            b->bodyName = bj["name"].get<std::string>();
            b->bodyDisplayName = bj["display"].get<std::string>();
            b->Radius = bj["radius"].get<double>();
            if (!bj["soi"].is_null()) b->sphereOfInfluence = bj["soi"].get<double>();
            b->gravParameter = bj["mu"].get<double>();
            b->rotationAngle = bj["rot"].get<double>();
            b->directRotAngle = bj["drot"].get<double>();
            b->isHomeWorld = bj["home"].get<bool>();
            b->flightGlobalsIndex = idx++;
            SaveEnvironment::Current().localBodies.push_back(b.get());
            bodies.push_back(std::move(b));
        }
        lib = std::make_unique<PartResourceLibrary>();
        ConfigNode::NodePtr root = ConfigNode::Load(GameData54() + "/Squad/Resources/ResourcesGeneric.cfg");
        REQUIRE(root);
        for (ConfigNode* d : root->GetNodes("RESOURCE_DEFINITION")) lib->resourceDefinitions.Add(*d);
        game.Mode = Game::Modes::MISSION;
        SaveEnvironment::Current().currentGame = &game;
        // PartLoader.getPartInfoByName: any part name resolves to one synthetic AvailablePart (the ProtoVessel snapshots only carry ids)
        SaveEnvironment::Current().partExists = [](std::string_view) { return true; };
        SaveEnvironment::Current().getPartInfoByName = [](std::string_view) -> const AvailablePart* {
            static AvailablePart synthetic;
            return &synthetic;
        };
        // the mission roster of the Kerbal tests: five crew and two tourists (labelled synthetic)
        roster = std::make_shared<KerbalRoster>(Game_Modes::MISSION);
        for (const char* n : {"Jebediah Kerman", "Bill Kerman", "Bob Kerman", "Valentina Kerman", "Gus Kerman"})
            roster->AddCrewMember(std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, n));
        for (const char* n : {"Tourist A", "Tourist B"}) roster->AddCrewMember(std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Tourist, n));
        game.CrewRoster = roster;
        RegisterNodeLibrary();
        RegisterVesselRestrictions();
    }
    ~Fixture54b() {
        game.CrewRoster = nullptr;
        SaveEnvironment::Current().currentGame = nullptr;
        SaveEnvironment::Current().localBodies.clear();
        NodeLibraryEnvironment::Reset();
        MissionDataEnvironment::Reset();
        RestrictionEnvironment::Reset();
        MissionTestsEnvironment::Reset();
        ActionCreateEnvironment::Reset();
    }
    CelestialBody* B(int i) { return bodies[static_cast<size_t>(i)].get(); }
    int IndexOf(CelestialBody* b) {
        for (size_t i = 0; i < bodies.size(); ++i)
            if (bodies[i].get() == b) return static_cast<int>(i);
        return -1;
    }
};

void CheckWaypoint(const FinePrint::Waypoint& w, const json& e) {
    CHECK(w.celestialName == e["celestialName"].get<std::string>());
    CHECK(w.latitude == e["latitude"].get<double>());
    CHECK(w.longitude == e["longitude"].get<double>());
    if (e.contains("altitude")) CHECK(w.altitude == e["altitude"].get<double>());
    if (e.contains("radius")) CHECK(w.radius == e["radius"].get<double>());
    if (e.contains("caption1")) CHECK(w.nodeCaption1 == e["caption1"].get<std::string>());
    if (e.contains("caption2")) CHECK(w.nodeCaption2 == e["caption2"].get<std::string>());
    if (e.contains("caption3")) CHECK(w.nodeCaption3 == e["caption3"].get<std::string>());
}

bool Near(double a, double b, double rel = 1e-9) { return std::fabs(a - b) <= rel * std::max(1.0, std::fabs(b)); }

std::vector<double> OrbSnapshot(Fixture54b& fx, MissionOrbit& o) {
    Orbit* ob = o.orbit();
    return {o.semiMajorAxis, o.eccentricity, o.inclination, o.LAN, o.argOfPeriapsis, o.MeanAnomalyAtEpoch(), o.epoch, static_cast<double>(o.ReferenceBodyIndex),
            ob->semiMajorAxis, ob->eccentricity, ob->inclination, ob->LAN, ob->argumentOfPeriapsis, ob->meanAnomalyAtEpoch, ob->epoch,
            static_cast<double>(fx.IndexOf(ob->referenceBody))};
}

}  // namespace

// ---- data classes ----------------------------------------------------------------------------------------------------------------

TEST_CASE("WP-54b reference: data classes (VesselGroundLocation, SurfaceArea, SurfaceVolume, Asteroid, Comet, ParamChoices, LaunchSiteSituation, MissionOrbit, VesselLocation, Crew)") {
    Fixture54b world;
    std::map<std::string, int> counts;
    for (const json& c : Ref()["cases"]) {
        const std::string kind = c["kind"].get<std::string>();
        const ConfigNode cfg = c.contains("cfg") ? CfgFromJson(c["cfg"]) : ConfigNode("N");
        if (kind == "ground") {
            VesselGroundLocation g;
            g.Load(cfg);
            CHECK(SaveDump([&](ConfigNode& n) { g.Save(n); }) == c["save"].get<std::string>());
            CHECK(g.GetNodeBodyParameterString() == c["text"].get<std::string>());
            CHECK(g.GetExtraText() == c["extra"].get<std::string>());
            auto w = g.GetWaypoint();
            CheckWaypoint(*w, c["waypoint"]);
            VesselGroundLocation g2;
            g2.Load(cfg);
            CHECK_FALSE(g.Equals(g2));  // MissionQuaternion.Equals is reference equality
            CHECK(g.HasWaypoint());
            CHECK(g.HasNodeLabel());
        } else if (kind == "area") {
            SurfaceArea a;
            a.Load(cfg);
            CHECK(SaveDump([&](ConfigNode& n) { a.Save(n); }) == c["save"].get<std::string>());
            CHECK(a.GetNodeBodyParameterString() == c["text"].get<std::string>());
            CHECK(a.GetExtraText() == c["extra"].get<std::string>());
            CheckWaypoint(*a.GetWaypoint(), c["waypoint"]);
            for (const auto& p : c["points"]) {
                const double lat = p["lat"].get<double>(), lon = p["lon"].get<double>();
                CHECK(a.IsPointInCircle(lat, lon) == p["inside"].get<bool>());
                CHECK(Near(a.PointInCircleAccuracy(lat, lon), p["accuracy"].get<double>()));
            }
            SurfaceArea b1;
            b1.Load(cfg);
            CHECK(a.Equals(b1) == c["equalsClone"].get<bool>());
            SurfaceArea b2;
            b2.Load(cfg);
            b2.radius = f32store(fx(b2.radius) * 1.5 + 1.0);
            CHECK(a.Equals(b2) == c["equalsOther"].get<bool>());
        } else if (kind == "volume") {
            SurfaceVolume dflt;
            CHECK(dflt.radius == c["default"]["radius"].get<float>());
            CHECK(dflt.heightConeMax == c["default"]["hmax"].get<float>());
            SurfaceVolume v;
            v.Load(cfg);
            CHECK(SaveDump([&](ConfigNode& n) { v.Save(n); }) == c["save"].get<std::string>());
            CHECK(v.GetNodeBodyParameterString() == c["text"].get<std::string>());
            CHECK(v.GetExtraText() == c["extra"].get<std::string>());
            CheckWaypoint(*v.GetWaypoint(), c["waypoint"]);
            for (const auto& p : c["points"])
                CHECK(v.IsInsideVolume(p["lat"].get<float>(), p["lon"].get<float>(), p["alt"].get<float>()) == p["inside"].get<bool>());
            v.showNodeLabel = false;
            CHECK_FALSE(v.HasNodeLabel());
            v.showNodeLabel = true;
            CHECK(v.HasNodeLabel());
            SurfaceVolume v2;
            v2.Load(cfg);
            CHECK(v.Equals(v2) == c["equalsClone"].get<bool>());
            v2.heightConeMax = f32store(fx(v2.heightConeMax) + 100.0);
            CHECK(v.Equals(v2) == c["equalsOther"].get<bool>());
        } else if (kind == "asteroid") {
            Asteroid a;
            a.Load(cfg);
            CHECK(SaveDump([&](ConfigNode& n) { a.Save(n); }) == c["save"].get<std::string>());
        } else if (kind == "comet") {
            if (c["defs"].is_null()) {
                MissionDataEnvironment::cometDefinitions = nullptr;
            } else {
                std::vector<MissionDataEnvironment::CometDefinition> defs;
                for (const auto& d : c["defs"]) defs.push_back({d[0].get<std::string>(), d[1].get<std::string>()});
                MissionDataEnvironment::cometDefinitions = [defs] { return std::optional<std::vector<MissionDataEnvironment::CometDefinition>>(defs); };
            }
            Comet comet;
            CHECK(comet.cometType == c["initial"]["type"].get<std::string>());
            CHECK(comet.cometDisplayType == c["initial"]["display"].get<std::string>());
            comet.Load(cfg);
            CHECK(SaveDump([&](ConfigNode& n) { comet.Save(n); }) == c["save"].get<std::string>());
            CHECK(comet.cometDisplayType == c["display"].get<std::string>());
            MissionDataEnvironment::cometDefinitions = nullptr;
        } else if (kind == "surface") {
            ParamChoices_CelestialBodySurface p;
            p.bodyData = std::make_unique<MissionCelestialBody>(c["bodyAny"].get<bool>() ? nullptr : world.B(0));
            p.biomeData = std::make_unique<Editor::MissionBiome>(world.B(0), "");
            p.areaData = std::make_unique<SurfaceArea>(world.B(0), 0.0, 0.0, 150000.0f);
            p.launchSiteName = "";
            p.Load(cfg);
            CHECK(SaveDump([&](ConfigNode& n) { p.Save(n); }) == c["save"].get<std::string>());
            CHECK(p.NodeBodyParameterString() == c["text"].get<std::string>());
            CHECK(p.HasWaypoint() == c["hasWaypoint"].get<bool>());
            CHECK(p.GetExtraText() == c["extra"].get<std::string>());
            CHECK((p.GetWaypoint() != nullptr) == c["hasWaypoint"].get<bool>());
        } else if (kind == "launchsite") {
            LaunchSiteSituation ls(nullptr);
            ls.Load(cfg);
            CHECK(SaveDump([&](ConfigNode& n) { ls.Save(n); }) == "N{" + c["save"].get<std::string>() + "}");
            CHECK(ls.LaunchSiteObjectName() == c["objectName"].get<std::string>());
            CHECK(ls.launchSiteName == c["name"].get<std::string>());
        } else if (kind == "orbit") {
            MissionOrbit o(cfg);
            auto expectSnap = c["snapshots"].begin();
            auto compare = [&](MissionOrbit& orb, const json& snap) {
                const std::vector<double> got = OrbSnapshot(world, orb);
                REQUIRE(got.size() == snap.size());
                for (size_t i = 0; i < got.size(); ++i) CHECK_MESSAGE(got[i] == snap[i].get<double>(), "field " << i);
            };
            compare(o, *expectSnap++);
            for (const auto& op : c["ops"]) {
                const std::string name = op["op"].get<std::string>();
                if (name == "Reset") {
                    o.Reset();
                } else if (name == "Body") {
                    o.set_Body(world.B(op["value"].get<int>()));
                } else {
                    const double v = std::stod(op["value"].get<std::string>());
                    if (name == "Apoapsis") o.set_Apoapsis(v);
                    else if (name == "Periapsis") o.set_Periapsis(v);
                    else if (name == "SemiMajorAxis") o.set_SemiMajorAxis(v);
                    else if (name == "Eccentricity") o.set_Eccentricity(v);
                    else if (name == "Inclination") o.set_Inclination(v);
                    else if (name == "Lan") o.set_Lan(v);
                    else if (name == "ArgumentOfPeriapsis") o.set_ArgumentOfPeriapsis(v);
                    else if (name == "Epoch") o.set_Epoch(v);
                    else if (name == "MeanAnomalyAtEpoch") o.set_MeanAnomalyAtEpoch(v);
                    else FAIL("unknown op " << name);
                }
                compare(o, *expectSnap++);
            }
            CHECK(SaveDump([&](ConfigNode& n) { o.Save(n); }) == c["save"].get<std::string>());
            CHECK(o.GetNodeBodyParameterString() == c["text"].get<std::string>());
            MissionOrbit clone(cfg);
            CHECK(MissionOrbit(cfg).Equals(clone) == c["equalsClone"].get<bool>());
            CHECK(o.Equals(clone) == c["equalsMoved"].get<bool>());
            // MissionOrbit.Load: the setters, from the second configuration
            ConfigNode base("ORBIT");
            for (const auto& kv : std::vector<std::pair<const char*, const char*>>{{"SMA", "900000"}, {"ECC", "0"}, {"INC", "0"}, {"LPE", "0"}, {"LAN", "0"}, {"MNA", "0"}, {"EPH", "0"}, {"REF", "0"}})
                base.AddValue(kv.first, kv.second);
            MissionOrbit o2(base);
            o2.Load(CfgFromJson(c["loadCfg"]));
            CHECK(SaveDump([&](ConfigNode& n) { o2.Save(n); }) == c["loadSave"].get<std::string>());
            const std::vector<double> got = OrbSnapshot(world, o2);
            for (size_t i = 0; i < got.size(); ++i) CHECK_MESSAGE(got[i] == c["loadSnapshot"][i].get<double>(), "load field " << i);
            for (size_t i = 0; i < world.bodies.size(); ++i) CHECK(MissionOrbit::GetEditorMaxOrbitRadius(world.B(static_cast<int>(i))) == c["maxRadius"][i].get<double>());
        } else if (kind == "location") {
            VesselLocation loc;
            loc.Load(cfg);
            CHECK(SaveDump([&](ConfigNode& n) { loc.Save(n); }) == c["save"].get<std::string>());
        } else if (kind == "crew") {
            Crew crew;
            crew.Load(cfg);
            CHECK(SaveDump([&](ConfigNode& n) { crew.Save(n); }) == c["save"].get<std::string>());
            for (size_t i = 0; i < crew.crewNames.size(); ++i) CHECK(crew.LocalizeCrewMember(static_cast<int>(i)) == c["localized"][i].get<std::string>());
        } else {
            continue;
        }
        counts[kind]++;
    }
    CHECK(counts.size() >= 10);
}

// ---- vessel restrictions ---------------------------------------------------------------------------------------------------------

namespace {
std::unique_ptr<VesselRestriction> MakeRestriction(const std::string& type) {
    const DynamicModuleType* t = DynamicModuleRegistry::GetClassByName(type);
    REQUIRE_MESSAGE(t != nullptr, type);
    std::unique_ptr<DynamicModule> m = t->create(nullptr);
    auto* raw = dynamic_cast<VesselRestriction*>(m.get());
    REQUIRE(raw != nullptr);
    m.release();
    return std::unique_ptr<VesselRestriction>(raw);
}

void InstallEnv(const json& e) {
    RestrictionEnvironment::shipCost = [v = e["cost"].get<float>()] { return v; };
    RestrictionEnvironment::shipTotalMass = [v = e["mass"].get<float>()] { return v; };
    RestrictionEnvironment::shipPartCount = [v = e["parts"].get<int>()] { return v; };
    RestrictionEnvironment::shipSize = [s = e["size"]] { return Vector3f(s[0].get<float>(), s[1].get<float>(), s[2].get<float>()); };
    RestrictionEnvironment::stageCount = [v = e["stages"].get<int>()] { return v; };
    std::vector<std::pair<std::string, double>> res;
    for (const auto& r : e["resources"]) res.emplace_back(r[0].get<std::string>(), r[1].get<double>());
    RestrictionEnvironment::shipResources = [res] { return res; };
    const json cc = e["crewCount"];
    RestrictionEnvironment::crewCount = [cc]() -> std::optional<int> {
        if (cc.is_null()) return std::nullopt;
        return cc.get<int>();
    };
    const json ct = e["crewTraits"];
    RestrictionEnvironment::crewTraits = [ct]() -> std::optional<std::vector<std::string>> {
        if (ct.is_null()) return std::nullopt;
        std::vector<std::string> v;
        for (const auto& t : ct) v.push_back(t.get<std::string>());
        return v;
    };
    RestrictionEnvironment::time = [v = e["time"].get<float>()] { return v; };
}
}  // namespace

TEST_CASE("WP-54b reference: VesselRestriction* (Load / Save, IsComplete, GetStateMessage, UI entry, SameComparator) and VesselSituation Load / Save") {
    Fixture54b world;
    int restrictions = 0, situations = 0;
    for (const json& c : Ref()["cases"]) {
        const std::string kind = c["kind"].get<std::string>();
        if (kind == "restriction") {
            ++restrictions;
            std::unique_ptr<VesselRestriction> m = MakeRestriction(c["type"].get<std::string>());
            const ConfigNode cfg = CfgFromJson(c["cfg"]);
            m->Load(cfg);
            CHECK(SaveDump([&](ConfigNode& n) { m->Save(n); }) == c["save"].get<std::string>());
            CHECK(m->GetDisplayName() == c["display"].get<std::string>());
            for (const json& sc : c["scenarios"]) {
                InstallEnv(sc["env"]);
                CHECK(m->IsComplete() == sc["complete"].get<bool>());
                CHECK(m->GetStateMessage() == sc["message"].get<std::string>());
                // the app list entry: text, state image and the readyToLaunch reset of the situation
                VesselSituation vs;
                vs.readyToLaunch = true;
                m->AddAppUIReference(true, &vs);
                CHECK(m->UIEntry.text == sc["ui"]["text"].get<std::string>());
                CHECK(m->UIEntry.state == sc["ui"]["state"].get<int>());
                CHECK(vs.readyToLaunch == !sc["ui"]["readyReset"].get<bool>());
                m->ClearAppUIREference();
                CHECK_FALSE(m->UIEntry.present);
            }
            std::unique_ptr<VesselRestriction> other = MakeRestriction(c["type"].get<std::string>());
            other->Load(CfgFromJson(c["otherCfg"]));
            if (c["same"].is_string()) {
                CHECK_THROWS(m->SameComparatorWrapper(*other));
            } else {
                CHECK(m->SameComparatorWrapper(*other) == c["same"].get<bool>());
            }
            // a restriction of another type is never "the same"
            std::unique_ptr<VesselRestriction> stages = MakeRestriction(c["type"].get<std::string>() == "VesselRestriction_Stages" ? "VesselRestriction_Cost" : "VesselRestriction_Stages");
            CHECK_FALSE(m->SameComparatorWrapper(*stages));
            RestrictionEnvironment::Reset();
        } else if (kind == "situation") {
            ++situations;
            ResourceEnvironment::tickCount = [t = c["tick"].get<int32_t>()] { return t; };
            MissionMappedVessel::getUniquePersistentId = [v = c["defaultPid"].get<uint32_t>()] { return v; };
            std::unique_ptr<Mission> mission = std::make_unique<Mission>();
            MENode* node = MENode::Spawn(mission.get());
            VesselSituation vs(mission.get(), node);
            vs.Load(CfgFromJson(c["cfg"]));
            const std::string got = SaveDump([&](ConfigNode& n) { vs.Save(n); });
            CHECK(got == c["save"].get<std::string>());
            ResourceEnvironment::tickCount = nullptr;
            MissionMappedVessel::getUniquePersistentId = nullptr;
        }
    }
    CHECK(restrictions > 100);
    CHECK(situations > 40);
}

// ---- Tests.* (the slice-2 set) -----------------------------------------------------------------------------------------------------

namespace {

Vessel::Situations SitOf(const std::string& n) {
    if (n == "LANDED") return Vessel::Situations::LANDED;
    if (n == "SPLASHED") return Vessel::Situations::SPLASHED;
    if (n == "PRELAUNCH") return Vessel::Situations::PRELAUNCH;
    if (n == "FLYING") return Vessel::Situations::FLYING;
    if (n == "SUB_ORBITAL") return Vessel::Situations::SUB_ORBITAL;
    if (n == "ORBITING") return Vessel::Situations::ORBITING;
    if (n == "ESCAPING") return Vessel::Situations::ESCAPING;
    return Vessel::Situations::DOCKED;
}

// the synthetic ScienceUtil.GetExperimentBiome stand-in (the real one needs a biome map): the same lat bands as the reference
std::string BiomeAt(double lat) { return lat < -30.0 ? "Shores" : (lat < 30.0 ? "Grasslands" : "Highlands"); }

// a VESSEL node for the ProtoVessel of an unloaded vessel: its orbit, situation and one PART per part of the world vessel (synthetic input)
std::unique_ptr<ProtoVessel> MakeProto54(const json& vj) {
    ConfigNode v("VESSEL");
    v.AddValue("pid", vj["id"].get<std::string>());
    v.AddValue("persistentId", vj["pid"].get<uint32_t>());
    v.AddValue("name", "Synthetic");
    v.AddValue("type", "Ship");
    v.AddValue("sit", vj["protoSit"].get<std::string>());
    v.AddValue("landed", false);
    v.AddValue("landedAt", "");
    v.AddValue("splashed", false);
    v.AddValue("met", 0.0);
    v.AddValue("lct", 0.0);
    v.AddValue("root", 0);
    v.AddValue("lat", 0.0);
    v.AddValue("lon", 0.0);
    v.AddValue("alt", 0.0);
    v.AddValue("hgt", 0.0);
    v.AddValue("nrm", Vector3f(0, 0, 1));
    v.AddValue("rot", Quaternionf(0, 0, 0, 1));
    v.AddValue("CoM", Vector3f(0, 0, 0));
    v.AddValue("stg", 0);
    v.AddValue("prst", false);
    v.AddValue("ref", 0);
    v.AddValue("ctrl", false);
    const json& o = vj["orbit"];
    ConfigNode* orbit = v.AddNode("ORBIT");
    orbit->AddValue("SMA", o["sma"].get<double>());
    orbit->AddValue("ECC", o["ecc"].get<double>());
    orbit->AddValue("INC", o["inc"].get<double>());
    orbit->AddValue("LPE", o["argpe"].get<double>());
    orbit->AddValue("LAN", o["lan"].get<double>());
    orbit->AddValue("MNA", o["mna"].get<double>());
    orbit->AddValue("EPH", o["epoch"].get<double>());
    orbit->AddValue("REF", o["ref"].get<int>());
    int idx = 0;
    for (const json& pj : vj["parts"]) {
        ConfigNode* p = v.AddNode("PART");
        p->AddValue("name", "synthetic" + std::to_string(idx));
        p->AddValue("uid", 4000u + static_cast<uint32_t>(idx));
        p->AddValue("persistentId", pj["pid"].get<uint32_t>());
        p->AddValue("parent", 0);
        p->AddValue("position", Vector3f(0, 0, 0));
        p->AddValue("rotation", Quaternionf(0, 0, 0, 1));
        p->AddValue("mirror", Vector3f(1, 1, 1));
        p->AddValue("istg", 0);
        p->AddValue("dstg", 0);
        p->AddValue("sqor", 0);
        p->AddValue("sidx", -1);
        p->AddValue("attm", 0);
        p->AddValue("mass", 1.0);
        p->AddValue("temp", 300.0);
        p->AddValue("expt", 0.5);
        p->AddValue("state", 0);
        p->AddValue("attached", true);
        p->AddValue("crew", "");
        ++idx;
    }
    return std::make_unique<ProtoVessel>(v, nullptr);
}

struct World54 {
    std::vector<std::unique_ptr<Part>> parts;
    std::vector<std::shared_ptr<ProtoCrewMember>> crew;
    std::map<uint32_t, Vessel*> byPid;
    std::map<Vessel*, std::string> crash;
    std::map<Part*, bool> asteroid;
    std::map<Part*, std::string> grapple;
    std::vector<std::unique_ptr<ProtoVessel>> protos;
    json env;
    // declared last = destroyed first: ~Vessel (FlightGlobals.RemoveVessel) reads its parts and proto vessel, which must outlive it
    std::vector<std::unique_ptr<Vessel>> vessels;
};

struct Scene54 {
    Fixture54b& fx;
    std::unique_ptr<Mission> mission;
    MENode* subject = nullptr;
    std::vector<std::unique_ptr<World54>> worlds;
    World54* world = nullptr;
    double ut = 0.0;
    explicit Scene54(Fixture54b& f) : fx(f) { fx.game.Mode = Game::Modes::CAREER; }  // Mission.Load in MISSION mode needs a SITUATION node
    ~Scene54() {
        FlightRegistry::Clear();
        SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
        fx.game.Mode = Game::Modes::MISSION;
    }

    void BuildMission() {
        mission = std::make_unique<Mission>();
        ConfigNode cfg("MISSION");
        cfg.AddValue("title", "slice 2");
        ConfigNode* nodes = cfg.AddNode("NODES");
        ConfigNode* s = nodes->AddNode("NODE");
        s->AddValue("title", "Subject");
        s->AddValue("id", GuidOf(98));
        s->AddValue("isStartNode", true);
        s->AddValue("missionEndOptions", "Success");
        mission->Load(cfg, false);
        subject = mission->GetNodeById(Guid::Parse(GuidOf(98)));
        REQUIRE(subject != nullptr);
        MissionsEnvironment::universalTime = [this] { return ut; };
        // the mission roster of the Kerbal tests: crew and tourists (labelled synthetic)
        if (mission->situation != nullptr) mission->situation->crewRoster = fx.roster;
    }

    void SetEnv(const json& env) {
        FlightRegistry::Clear();
        SaveEnvironment::Current().persistentIds.ClearpersistentIdDictionaries();
        worlds.push_back(std::make_unique<World54>());
        world = worlds.back().get();
        world->env = env;
        ut = env["ut"].get<double>();
        for (const json& vj : env["vessels"]) {
            auto v = std::make_unique<Vessel>();
            v->persistentId = vj["pid"].get<uint32_t>();
            v->id = Guid::Parse(vj["id"].get<std::string>());
            v->vesselType = dotnet::EnumParse<ksp::VesselType>(vj["type"].get<std::string>());
            v->situation = SitOf(vj["sit"].get<std::string>());
            v->latitude = vj["lat"].get<double>();
            v->longitude = vj["lon"].get<double>();
            v->altitude = vj["alt"].get<double>();
            v->srfSpeed = vj["srf"].get<double>();
            v->obt_speed = vj["obt"].get<double>();
            v->geeForce = vj["gee"].get<double>();
            v->loaded = vj["loaded"].get<bool>();
            v->landedAt = vj["landedAt"].get<std::string>();
            const auto& ov = vj["obtVel"];
            v->obt_velocity = Vector3d(ov[0].get<double>(), ov[1].get<double>(), ov[2].get<double>());
            const json& o = vj["orbit"];
            v->orbitDriver = std::make_unique<OrbitDriver>();
            v->orbitDriver->orbit = std::make_shared<Orbit>(o["inc"].get<double>(), o["ecc"].get<double>(), o["sma"].get<double>(), o["lan"].get<double>(), o["argpe"].get<double>(),
                                                           o["mna"].get<double>(), o["epoch"].get<double>(), fx.B(o["ref"].get<int>()));
            {   // DiscoveryInfo.Load (SetLevel would fire onKnowledgeChanged for a half-built vessel)
                ConfigNode dn("DISCOVERY");
                dn.AddValue("state", static_cast<int32_t>(vj["disc"].get<std::string>() == "Owned" ? DiscoveryLevels::Owned : DiscoveryLevels::Unowned));
                v->GetDiscoveryInfo().Load(dn);
            }
            for (const json& pj : vj["parts"]) {
                auto p = std::make_unique<Part>();
                p->fields.persistentId = pj["pid"].get<uint32_t>();
                p->fields.CrewCapacity = pj["cap"].get<int>();
                p->vessel = v.get();
                for (const json& nj : pj["crew"]) {
                    auto pcm = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, nj.get<std::string>());
                    p->protoModuleCrew.push_back(pcm);
                    world->crew.push_back(pcm);
                }
                world->asteroid[p.get()] = pj.value("ast", false);
                if (pj.contains("grapple") && !pj["grapple"].is_null()) world->grapple[p.get()] = pj["grapple"].get<std::string>();
                v->parts.push_back(p.get());
                world->parts.push_back(std::move(p));
            }
            if (!v->parts.empty()) v->rootPart = v->parts.front();
            if (!v->loaded) {
                world->protos.push_back(MakeProto54(vj));
                v->protoVessel = world->protos.back().get();
                // ProtoVessel.protoVessel crew: the crew the Python scenario put in the vessel's parts
                for (Part* p : v->parts)
                    for (const auto& pcm : p->protoModuleCrew) v->protoVessel->AddCrew(pcm);
            }
            if (!vj["crash"].is_null()) world->crash[v.get()] = vj["crash"].get<std::string>();
            world->byPid[v->persistentId] = v.get();
            FlightRegistry::Vessels.push_back(v.get());
            FlightRegistry::VesselsLoaded.push_back(v.get());
            world->vessels.push_back(std::move(v));
        }
        const int act = env["active"].get<int>();
        FlightRegistry::ActiveVessel = (act >= 0 && act < static_cast<int>(world->vessels.size())) ? world->vessels[static_cast<size_t>(act)].get() : nullptr;
        World54* w = world;
        MissionTestsEnvironment::vesselCrashObjectName = [w](Vessel* v) -> std::optional<std::string> {
            auto it = w->crash.find(v);
            if (it == w->crash.end()) return std::nullopt;
            return it->second;
        };
        MissionTestsEnvironment::experimentBiome = [](CelestialBody*, double lat, double) { return BiomeAt(lat); };
        const json mb = env["mainBody"];
        Fixture54b* f = &fx;
        MissionTestsEnvironment::currentMainBody = [mb, f]() -> CelestialBody* { return mb.is_null() ? nullptr : f->B(mb.get<int>()); };
        const json sv = env["shipObtVel"];
        MissionTestsEnvironment::shipObtVelocity = [sv] { return Vector3d(sv[0].get<double>(), sv[1].get<double>(), sv[2].get<double>()); };
        MissionTestsEnvironment::vesselAtOrbit = [](Orbit* o, double dev, Vessel* v) {
            Orbit& vo = v->orbit();
            if (vo.referenceBody != o->referenceBody) return false;
            return std::fabs(vo.semiMajorAxis - o->semiMajorAxis) <= dev / 100.0 * std::fabs(o->semiMajorAxis);
        };
        MissionTestsEnvironment::vesselAtOrbitAccuracy = [](Orbit* o, double dev, Vessel* v) {
            Orbit& vo = v->orbit();
            if (vo.referenceBody != o->referenceBody) return 0.0;
            if (!(std::fabs(vo.semiMajorAxis - o->semiMajorAxis) <= dev / 100.0 * std::fabs(o->semiMajorAxis))) return 0.0;
            return 1.0 - std::fabs(vo.semiMajorAxis - o->semiMajorAxis) / std::fabs(o->semiMajorAxis);
        };
        MissionTestsEnvironment::persistentLoadedPart = [w](uint32_t id) -> Part* {
            for (auto& v : w->vessels)
                if (v->loaded)
                    for (Part* p : v->parts)
                        if (p->fields.persistentId == id) return p;
            return nullptr;
        };
        MissionTestsEnvironment::persistentUnloadedPart = [w](uint32_t id) -> std::optional<MissionTestsEnvironment::UnloadedPart> {
            for (auto& v : w->vessels)
                if (!v->loaded)
                    for (Part* p : v->parts)
                        if (p->fields.persistentId == id) return MissionTestsEnvironment::UnloadedPart{p->protoModuleCrew, v->persistentId};
            return std::nullopt;
        };
        NodeLibraryEnvironment::findLoadedPart = [w](uint32_t id) -> Part* {
            if (id == 0) return nullptr;
            for (auto& v : w->vessels)
                if (v->loaded)
                    for (Part* p : v->parts)
                        if (p->fields.persistentId == id) return p;
            return nullptr;
        };
        MissionTestsEnvironment::findUnloadedPartVessel = [w](uint32_t id) -> std::optional<uint32_t> {
            if (id == 0) return std::nullopt;
            for (auto& v : w->vessels)
                if (!v->loaded)
                    for (Part* p : v->parts)
                        if (p->fields.persistentId == id) return v->persistentId;
            return std::nullopt;
        };
        MissionTestsEnvironment::partIsAsteroidOrComet = [w](Part* p) { return w->asteroid.count(p) != 0 && w->asteroid[p]; };
        // TestGrapple caches its ModuleGrappleNode for good, so the state lookup spans every world this scene has built
        MissionTestsEnvironment::grappleNodeState = [this](Part* p) -> std::optional<std::string> {
            for (auto& wd : worlds) {
                auto it = wd->grapple.find(p);
                if (it != wd->grapple.end()) return it->second;
            }
            return std::nullopt;
        };
    }
    Vessel* ByPid(uint32_t pid) { return world->byPid.count(pid) ? world->byPid[pid] : nullptr; }
};

json PidOrNull54(Vessel* v) { return v == nullptr ? json(nullptr) : json(v->persistentId); }

}  // namespace

TEST_CASE("WP-54b reference: the deferred Tests.* modules (Awake, Load / Save, Test, GetScoreModifier, events)") {
    Fixture54b world;
    size_t checked = 0;
    std::set<std::string> classes;
    for (const json& c : Ref()["cases"]) {
        if (c["kind"].get<std::string>() != "test") continue;
        const std::string name = c["cls"].get<std::string>();
        classes.insert(name);
        if (std::getenv("KSP_DBG")) std::fprintf(stderr, "CASE %s\n", name.c_str());
        for (const char* n : {"Jebediah Kerman", "Bill Kerman", "Bob Kerman", "Valentina Kerman", "Gus Kerman", "Tourist A", "Tourist B"})
            (*world.roster)[std::string(n)]->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
        Scene54 scene(world);
        scene.BuildMission();
        TestGroup* tg = new TestGroup(scene.subject);
        scene.subject->testGroups.emplace_back(tg);
        ConfigNode cfg = CfgFromJson(c["cfg"]);
        try {
            tg->AddTestModule(name, &cfg);
        } catch (const std::exception& e) {
            FAIL("AddTestModule(" << name << ") threw " << e.what());
        }
        REQUIRE_MESSAGE(tg->testModules.size() == 1, name);
        TestModule* mod = dynamic_cast<TestModule*>(tg->testModules[0].get());
        REQUIRE(mod != nullptr);
        for (const json& op : c["ops"]) {
            const std::string o = op["op"].get<std::string>();
            INFO("case ", name, " op ", o);
            if (std::getenv("KSP_DBG")) std::fprintf(stderr, "  op %s\n", o.c_str());
            ++checked;
            try {
                if (o == "title") {
                    CHECK(mod->title == op["out"].get<std::string>());
                } else if (o == "info") {
                    CHECK(mod->GetInfo() == op["out"].get<std::string>());
                } else if (o == "saved") {
                    ConfigNode out("N");
                    mod->Save(out);
                    CHECK(Dump(out) == op["out"].get<std::string>());
                } else if (o == "body") {
                    const json& f = op["field"];
                    Editor::BaseAPField field(f["name"].get<std::string>(), f["guiName"].get<std::string>(), f["value"].get<std::string>(), f["has"].get<bool>());
                    field.fieldType = f.value("fieldType", "");
                    CHECK(mod->GetNodeBodyParameterString(field) == op["out"].get<std::string>());
                } else if (o == "roster") {
                    const std::string st = op["status"].get<std::string>();
                    (*world.roster)[op["name"].get<std::string>()]->set_rosterStatus(st == "Available"  ? ProtoCrewMember::RosterStatus::Available
                                                                                      : st == "Assigned" ? ProtoCrewMember::RosterStatus::Assigned
                                                                                      : st == "Dead"     ? ProtoCrewMember::RosterStatus::Dead
                                                                                                         : ProtoCrewMember::RosterStatus::Missing);
                } else if (o == "env") {
                    scene.SetEnv(op["env"]);
                } else if (o == "setUT") {
                    scene.ut = op["ut"].get<double>();
                } else if (o == "test") {
                    if (op["out"].is_string()) {
                        CHECK_THROWS(mod->Test());
                    } else {
                        const bool gotT = mod->Test();
                        if (gotT != op["out"].get<bool>() && std::getenv("KSP_DBG")) std::fprintf(stderr, "MISMATCH %s ops=%zu cfg=%s\n", name.c_str(), checked, c["cfg"].dump().c_str());
                        CHECK(gotT == op["out"].get<bool>());
                    }
                } else if (o == "scoreMod") {
                    auto* s = dynamic_cast<IScoreableObjective*>(mod);
                    REQUIRE(s != nullptr);
                    if (op.value("throws", false)) {
                        CHECK_THROWS(s->GetScoreModifier(op["module"].get<std::string>()));
                    } else {
                        ScoreModifier r = s->GetScoreModifier(op["module"].get<std::string>());
                        const json& out = op["out"];
                        CHECK(PidOrNull54(r.vessel) == out["vessel"]);
                        CHECK(r.number.has_value() == !out["number"].is_null());
                        if (r.number.has_value() && !out["number"].is_null()) {
                            if (op.value("approx", false)) CHECK(Near(*r.number, out["number"].get<double>()));
                            else CHECK(*r.number == out["number"].get<double>());
                        }
                    }
                } else if (o == "init") {
                    mod->InitializeTest();
                } else if (o == "clear") {
                    mod->ClearTest();
                } else if (o == "event") {
                    const std::string en = op["name"].get<std::string>();
                    if (std::getenv("KSP_DBG")) std::fprintf(stderr, "  event %s\n", en.c_str());
                    if (en == "flagPlanted") {
                        static int dummy[16];
                        FlagSite* fs = reinterpret_cast<FlagSite*>(&dummy[op["vessel"].get<uint32_t>() % 16]);
                        Vessel* fv = scene.ByPid(op["vessel"].get<uint32_t>());
                        MissionTestsEnvironment::flagSiteVessel = [fv](FlagSite*) { return fv; };
                        GameEvents::afterFlagPlanted.Fire(fs);
                    } else if (en == "crewKilled") {
                        const std::string sender = op["sender"].get<std::string>();
                        MissionTestsEnvironment::eventReportSender = [sender](EventReport*) { return sender; };
                        GameEvents::onCrewKilled.Fire(nullptr);
                    } else if (en == "vesselRecovered") {
                        json vj = scene.world->env["vessels"].empty() ? json(nullptr) : scene.world->env["vessels"][0];
                        auto pv = MakeProto54(vj.is_null() ? json::parse(R"({"id":"00000000-0000-0000-0000-000000000001","pid":9001,"protoSit":"ORBITING","parts":[],"orbit":{"inc":0.0,"ecc":0.0,"sma":900000.0,"lan":0.0,"argpe":0.0,"mna":0.0,"epoch":0.0,"ref":0}})") : vj);
                        for (const json& n : op["crew"]) pv->AddCrew((*world.roster)[n.get<std::string>()]);
                        GameEvents::onVesselRecovered.Fire(pv.get(), false);
                    } else if (en == "knowledgeChanged") {
                        Vessel* kv = scene.ByPid(op["vessel"].get<uint32_t>());
                        GameEvents::onKnowledgeChanged.Fire(GameEvents::HostedFromToAction<IDiscoverable*, DiscoveryLevels>(
                            kv, kv->GetDiscoveryInfo().Level(), op["to"].get<std::string>() == "Owned" ? DiscoveryLevels::Owned : DiscoveryLevels::Unowned));
                    } else if (en == "crewTransferred") {
                        std::shared_ptr<ProtoCrewMember> pcm = (*world.roster)[op["sender"].get<std::string>()];
                        GameEvents::onCrewTransferred.Fire(GameEvents::HostedFromToAction<ProtoCrewMember*, Part*>(pcm.get(), nullptr, nullptr));
                    } else if (en == "vesselWillDestroy") {
                        GameEvents::onVesselWillDestroy.Fire(scene.ByPid(op["vessel"].get<uint32_t>()));
                    } else if (en == "partCouple") {
                        // real parts: VesselPrecalculate.onDockingComplete (every vessel subscribes) reads from.vessel / to.vessel
                        REQUIRE(!scene.world->parts.empty());
                        GameEvents::onDockingComplete.Fire(GameEvents::FromToAction<Part*, Part*>(scene.world->parts.front().get(), scene.world->parts.back().get()));
                    } else if (en == "partUndock") {
                        GameEvents::onPartUndockComplete.Fire(scene.world->parts.front().get());
                    } else if (en == "scienceReceived") {
                        ScienceSubject subject(op["id"].get<std::string>(), "title", 1.0f, 1.0f, 100.0f);
                        GameEvents::OnScienceRecieved.Fire(0.0f, &subject, nullptr, false);
                    } else if (en == "experimentStored") {
                        ScienceData data(1.0f, 1.0f, 1.0f, op["id"].get<std::string>(), "data");
                        GameEvents::OnExperimentStored.Fire(&data);
                    } else {
                        FAIL("unknown event " << en);
                    }
                } else if (o == "call") {
                    const std::string fn = op["fn"].get<std::string>();
                    const json& a = op["a"];
                    if (fn == "OnVesselPersistentIdChanged") {
                        mod->OnVesselPersistentIdChanged(a[0].get<uint32_t>(), a[1].get<uint32_t>());
                    } else if (fn == "OnPartPersistentIdChanged") {
                        mod->OnPartPersistentIdChanged(a[0].get<uint32_t>(), a[1].get<uint32_t>(), a[2].get<uint32_t>());
                    } else if (fn == "OnVesselDocking") {
                        mod->OnVesselDocking(a[0].get<uint32_t>(), a[1].get<uint32_t>());
                    } else if (fn == "OnVesselsUndocking") {
                        mod->OnVesselsUndocking(scene.ByPid(a[0].get<uint32_t>()), scene.ByPid(a[1].get<uint32_t>()));
                    } else {
                        FAIL("unknown call " << fn);
                    }
                } else if (o == "validate") {
                    Editor::MissionEditorValidator validator;
                    validator.EnsureResultsForTest();
                    mod->RunValidation(&validator);
                    json got = json::array();
                    for (const auto& r : *validator.Results()) got.push_back(json::array({r.status == Editor::ValidationStatus::Fail ? "fail" : "warn", r.message}));
                    CHECK(got == op["out"]);
                } else {
                    FAIL("unknown op " << o);
                }
            } catch (const std::exception& e) {
                FAIL_CHECK("exception in " << name << " op " << o << ": " << e.what());
            }
        }
        // scene teardown: a test initialised twice holds two event registrations (as in C#); drop them before the module dies
        for (int k = 0; k < 24; ++k) {
            try { mod->ClearTest(); } catch (const std::exception&) {}
        }
    }
    CHECK(checked > 3000);
    CHECK(classes.size() >= 11);
}

// ---- the create actions (ActionCreateAsteroid / Comet / Kerbal / Flag): Awake defaults, Load / Save, GetInfo, body text; GetProtoVesselNode of ActionCreateFlag ---------

TEST_CASE("WP-54b reference: Actions.ActionCreate{Asteroid,Comet,Kerbal,Flag} (Awake, Load / Save, GetInfo, body text, flag proto node)") {
    Fixture54b world;
    size_t checked = 0;
    std::set<std::string> names;
    using Env = ActionCreateEnvironment;
    // the rot component goes through libm sin / cos (the lat / lon of the case): compared per component with 1e-12 + 1e-5 relative; the rest exactly
    auto CmpApproxRot = [](const std::string& got, const std::string& want) {
        auto split = [](const std::string& t, std::string& before, std::vector<double>& rot, std::string& after) {
            const size_t a = t.find("rot=");
            const size_t e = t.find(';', a);
            before = t.substr(0, a);
            after = t.substr(e);
            std::stringstream ss(t.substr(a + 4, e - a - 4));
            std::string tok;
            rot.clear();
            while (std::getline(ss, tok, ',')) rot.push_back(std::stod(tok));
        };
        std::string b1, a1, b2, a2;
        std::vector<double> r1, r2;
        split(got, b1, r1, a1);
        split(want, b2, r2, a2);
        CHECK(b1 == b2);
        CHECK(a1 == a2);
        REQUIRE(r1.size() == 4);
        REQUIRE(r2.size() == 4);
        for (size_t k = 0; k < 4; ++k) CHECK(std::fabs(r1[k] - r2[k]) <= 1e-12 + 1e-5 * std::fabs(r2[k]));
    };
    auto Cmp = [](const std::string& got, const std::string& want) {
        if (got != want && std::getenv("KSP_DBG")) std::fprintf(stderr, "GOT  %s\nWANT %s\n", got.c_str(), want.c_str());
        CHECK(got == want);
    };
    for (const json& c : Ref()["cases"]) {
        if (c["kind"].get<std::string>() != "action") continue;
        const std::string name = c["name"].get<std::string>();
        names.insert(name);
        for (const char* n : {"Jebediah Kerman", "Bill Kerman", "Bob Kerman", "Valentina Kerman", "Gus Kerman", "Tourist A", "Tourist B"})
            (*world.roster)[std::string(n)]->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
        Scene54 scene(world);
        scene.BuildMission();
        MissionMappedVessel::getUniquePersistentId = [] { return 424242u; };
        ResourceEnvironment::tickCount = [] { return 12345; };  // new KSPRandom(): the position of a new VesselGroundLocation
        // labelled-synthetic stand-ins for the ProtoVessel node builders and the Unity natives (both sides of the gate use the same ones)
        Env::createRandomOrbitAround = [](CelestialBody* b) { return Orbit(1.0, 0.5, b->Radius * 3.0, 2.0, 3.0, 4.0, 5.0, b); };
        Env::createVesselNode = [](const std::string& vname, VesselType type, const Orbit& orbit, int rootIdx, const std::vector<ConfigNode::NodePtr>& parts,
                                   const std::vector<ConfigNode::NodePtr>& extra) {
            auto v = std::make_shared<ConfigNode>("VESSEL");
            v->AddValue("name", vname);
            v->AddValue("type", dotnet::EnumToString(type));
            v->AddValue("orbitRef", orbit.referenceBody->name());
            v->AddValue("orbitSma", orbit.semiMajorAxis);
            v->AddValue("rootIdx", rootIdx);
            v->AddValue("persistentId", 0);
            v->AddValue("landedAt", "");
            v->AddValue("lat", 0);
            v->AddValue("lon", 0);
            v->AddValue("sit", "");
            v->AddValue("alt", 0);
            v->AddValue("skipGroundPositioning", true);
            v->AddValue("landed", false);
            v->AddValue("splashed", false);
            v->AddValue("vesselSpawning", false);
            v->AddValue("PQSMin", -1);
            v->AddValue("PQSMax", -1);
            for (const auto& p : parts) v->AddNode(p);
            for (const auto& p : extra) v->AddNode(p);
            return v;
        };
        Env::createPartNode = [](const std::string& partName, uint32_t id, const std::vector<std::shared_ptr<ProtoCrewMember>>&) {
            auto p = std::make_shared<ConfigNode>("PART");
            p->AddValue("name", partName);
            p->AddValue("id", id);
            return p;
        };
        Env::uniqueFlightID = [] { return 7777u; };
        Env::fromToRotation = [](const Vector3f& a, const Vector3f& b) {
            return Quaternionf(f32store(fx(a.x) + fx(b.x)), f32store(fx(a.y) + fx(b.y)), f32store(fx(a.z) + fx(b.z)), 1.0f);
        };
        Env::angleAxis = [](float angle, const Vector3f& axis) {
            return Quaternionf(f32store(fx(axis.x) * fx(angle)), f32store(fx(axis.y) * fx(angle)), f32store(fx(axis.z) * fx(angle)), angle);
        };
        if (c.contains("flagUrl")) scene.mission->flagURL = c["flagUrl"].get<std::string>();
        ConfigNode cfg = CfgFromJson(c["cfg"]);
        IActionModule* a = scene.subject->AddAction(name, &cfg);
        REQUIRE_MESSAGE(a != nullptr, name);
        ActionModule* mod = dynamic_cast<ActionModule*>(a);
        REQUIRE(mod != nullptr);
        for (const json& op : c["ops"]) {
            const std::string o = op["op"].get<std::string>();
            INFO("case ", name, " op ", o);
            ++checked;
            try {
                if (o == "title") {
                    CHECK(mod->title == op["out"].get<std::string>());
                } else if (o == "info") {
                    CHECK(mod->GetInfo() == op["out"].get<std::string>());
                } else if (o == "saved") {
                    ConfigNode out("N");
                    mod->Save(out);
                    Cmp(Dump(out), op["out"].get<std::string>());
                } else if (o == "body") {
                    const json& f = op["field"];
                    Editor::BaseAPField field(f["name"].get<std::string>(), f["guiName"].get<std::string>(), f["value"].get<std::string>(), f["has"].get<bool>());
                    field.fieldType = f.value("fieldType", "");
                    Cmp(mod->GetNodeBodyParameterString(field), op["out"].get<std::string>());
                } else if (o == "proto") {
                    Env::hasPQS = [pqs = op["pqs"].get<bool>()](CelestialBody*) { return pqs; };
                    auto* flag = dynamic_cast<Actions::ActionCreateFlag*>(mod);
                    REQUIRE(flag != nullptr);
                    ConfigNode::NodePtr node = flag->GetProtoVesselNode();
                    if (op["approxRot"].get<bool>()) CmpApproxRot(Dump(*node), op["out"].get<std::string>());
                    else Cmp(Dump(*node), op["out"].get<std::string>());
                } else {
                    FAIL("unknown op " << o);
                }
            } catch (const std::exception& e) {
                FAIL_CHECK("exception in " << name << " op " << o << ": " << e.what());
            }
        }
        MissionMappedVessel::getUniquePersistentId = nullptr;
        ResourceEnvironment::tickCount = nullptr;
        Env::Reset();
    }
    CHECK(checked > 800);
    CHECK(names.size() == 4);
}
