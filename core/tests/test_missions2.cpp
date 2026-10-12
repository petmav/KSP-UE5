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
        RegisterNodeLibrary();
        RegisterVesselRestrictions();
    }
    ~Fixture54b() {
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
