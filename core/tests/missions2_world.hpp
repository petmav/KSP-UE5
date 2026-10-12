// WP-54c: the synthetic world of the WP-54b/54c reference tests (moved verbatim out of test_missions2.cpp so test_missions3.cpp shares it).
// Fixture54b (bodies / roster / localizer), World54 + Scene54 (a seeded vessel world and the mission with a Subject node).
#pragma once

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

namespace ksp::mtest54b {

using namespace ksp;
using namespace ksp::Expansions::Missions;
using json = nlohmann::json;
using namespace ksp::mtest;

inline const json& Ref() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp54b_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

inline ConfigNode CfgFromJson(const json& j) {
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

inline std::string SaveDump(const std::function<void(ConfigNode&)>& save) {
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

inline Vessel::Situations SitOf(const std::string& n) {
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
inline std::string BiomeAt(double lat) { return lat < -30.0 ? "Shores" : (lat < 30.0 ? "Grasslands" : "Highlands"); }

// a VESSEL node for the ProtoVessel of an unloaded vessel: its orbit, situation and one PART per part of the world vessel (synthetic input)
inline std::unique_ptr<ProtoVessel> MakeProto54(const json& vj) {
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

inline json PidOrNull54(Vessel* v) { return v == nullptr ? json(nullptr) : json(v->persistentId); }

}  // namespace ksp::mtest54b
