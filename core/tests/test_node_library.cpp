// WP-55 unit tests: the Making History node library (core/include/ksp/missions/{mission_tests,mission_actions,mission_adjusters,node_library}.hpp).
//
// No golden and no capture of the mission runtime exists and the mission files of the GameData mirror are 0-byte files (see STATUS WP-54), so
// every expectation of the "reference" cases comes from tools/wp55/node_library_reference.py (independent Python written from the decompiled
// C#, tests/data/wp55_reference.json): seeded module configs, vessel / mission scenarios and event sequences for every ported Tests.* /
// Actions.* / Adjusters.* class. The scenarios are synthetic inputs to the algorithms, not game data. The other cases pin down behaviour derived
// from the C# (the derivation is in the comment above the check).
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/career/funding.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/career/scenario_misc.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/missions.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
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

const json& Ref55() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp55_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

std::string Hex32(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    char b[16];
    std::snprintf(b, sizeof b, "0x%08x", u);
    return b;
}
std::string Hex64(double d) {
    uint64_t u;
    std::memcpy(&u, &d, 8);
    char b[24];
    std::snprintf(b, sizeof b, "0x%016llx", static_cast<unsigned long long>(u));
    return b;
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
        }
    }
    return n;
}

std::string G(float v) { return dotnet::FormatSingleG(v); }
std::string GD(double v) { return dotnet::FormatDoubleG(v); }

KSPActionGroup GroupOf(const std::string& name) {
    for (const auto& m : dotnet::EnumTraits<KSPActionGroup>::members)
        if (name == m.name) return m.value;
    return KSPActionGroup::None;
}

// a scoreable test returning a configured accuracy modifier (labelled stub: the Situation tests that return one are deferred)
class AccuracyStub : public TestModule, public IScoreableObjective {
public:
    static constexpr const char* kTypeName = "TestAccuracyStub";
    AccuracyStub() : TestModule(kTypeName) {}
    std::optional<double> number;
    ScoreModifier GetScoreModifier(const std::string& scoreModule) override {
        ScoreModifier r;
        if (scoreModule == "ScoreModule_Accuracy") r.number = number;
        return r;
    }
};

// ---- the world of one case ---------------------------------------------------------------------------------------------------

struct Fixture55 : Fixture54 {
    Game game;
    std::vector<std::unique_ptr<CelestialBody>> bodies;
    std::unique_ptr<PartResourceLibrary> lib;
    Planetarium planetarium;
    Funding funding;
    bool fundingAwake = false;
    uint64_t guidCounter55 = 0;
    Fixture55() {
        planetarium.Awake();
        GameSettings::KERBIN_TIME = true;
        KSPUtil::CacheLocalStrings();
        NodeLibraryEnvironment::Reset();
        FlightRegistry::Clear();
        RegisterNodeLibrary();
        TestModuleRegistry::Register({AccuracyStub::kTypeName, [] { return std::make_unique<AccuracyStub>(); }});
        Adjusters::InitialiseAdjusterTypes();
        for (const char* n : {"Kerbin", "Mun", "Minmus", "Duna"}) {
            auto b = std::make_unique<CelestialBody>();
            b->bodyName = n;
            b->bodyDisplayName = n;
            b->isHomeWorld = std::string(n) == "Kerbin";
            SaveEnvironment::Current().localBodies.push_back(b.get());
            bodies.push_back(std::move(b));
        }
        game.Mode = Game::Modes::CAREER;
        SaveEnvironment::Current().currentGame = &game;
        // PartLoader.getPartInfoByName: any part name resolves to one synthetic AvailablePart (the snapshots only carry resources)
        SaveEnvironment::Current().partExists = [](std::string_view) { return true; };
        SaveEnvironment::Current().getPartInfoByName = [](std::string_view) -> const AvailablePart* {
            static AvailablePart synthetic;
            return &synthetic;
        };
        lib = std::make_unique<PartResourceLibrary>();
        ConfigNode::NodePtr root = ConfigNode::Load(GameData54() + "/Squad/Resources/ResourcesGeneric.cfg");
        REQUIRE(root);
        for (ConfigNode* d : root->GetNodes("RESOURCE_DEFINITION")) lib->resourceDefinitions.Add(*d);
    }
    ~Fixture55() {
        if (fundingAwake) funding.OnDestroy();
        Funding::Instance = nullptr;
        FlightRegistry::Clear();
        SaveEnvironment::Current().currentGame = nullptr;
        SaveEnvironment::Current().localBodies.clear();
        NodeLibraryEnvironment::Reset();
        AwardEnvironment::Reset();
        MissionsEnvironment::Reset();
    }
    CelestialBody* Body(const std::string& name) {
        for (auto& b : bodies)
            if (b->bodyName == name) return b.get();
        return nullptr;
    }
    void SetFunds(const json& funds) {
        if (funds.is_null()) {
            Funding::Instance = nullptr;
            return;
        }
        if (!fundingAwake) {
            funding.OnAwake();
            fundingAwake = true;
        }
        Funding::Instance = &funding;
        funding.SetFunds(funds.get<double>(), TransactionReasons::Mission);
    }
};

struct World {
    std::vector<std::unique_ptr<Part>> parts;
    std::vector<std::unique_ptr<ProtoVessel>> protos;
    std::map<Vessel*, json> resources;
    std::map<uint32_t, Vessel*> byPid;
    // declared last = destroyed first: ~Vessel (Vessel.OnDestroy -> FlightGlobals.RemoveVessel) reads its parts' persistent ids and its proto vessel,
    // so they must outlive it (they were freed first before: a use-after-free that ASan reports, benign on a lucky heap)
    std::vector<std::unique_ptr<Vessel>> vessels;
};

// a VESSEL node for a ProtoVessel: pid, one PART per entry of `parts` (resource lists) (synthetic input of the algorithms)
std::unique_ptr<ProtoVessel> MakeProto(const std::string& vesselGuid, uint32_t pid, const json& parts) {
    ConfigNode v("VESSEL");
    v.AddValue("pid", vesselGuid);
    v.AddValue("persistentId", pid);
    v.AddValue("name", "Synthetic");
    v.AddValue("type", "Ship");
    v.AddValue("sit", "ORBITING");
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
    ConfigNode* orbit = v.AddNode("ORBIT");
    orbit->AddValue("SMA", 700000.0);
    orbit->AddValue("ECC", 0.0);
    orbit->AddValue("INC", 0.0);
    orbit->AddValue("LPE", 0.0);
    orbit->AddValue("LAN", 0.0);
    orbit->AddValue("MNA", 0.0);
    orbit->AddValue("EPH", 0.0);
    orbit->AddValue("REF", 1);
    int idx = 0;
    for (const auto& resList : parts) {
        ConfigNode* p = v.AddNode("PART");
        p->AddValue("name", "synthetic" + std::to_string(idx));
        p->AddValue("uid", 4000u + static_cast<uint32_t>(idx));
        p->AddValue("persistentId", 5000u + static_cast<uint32_t>(idx));
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
        for (const auto& r : resList) {
            ConfigNode* rn = p->AddNode("RESOURCE");
            rn->AddValue("name", r["name"].get<std::string>());
            rn->AddValue("amount", r["amount"].get<double>());
            rn->AddValue("maxAmount", r.contains("max") ? r["max"].get<double>() : 1000.0);
            rn->AddValue("flowState", true);
            rn->AddValue("isTweakable", true);
            rn->AddValue("hideFlow", false);
            rn->AddValue("flowMode", "Both");
        }
        ++idx;
    }
    return std::make_unique<ProtoVessel>(v, nullptr);
}

struct Scene {
    Fixture55& fx;
    std::unique_ptr<Mission> mission;
    MENode* subject = nullptr;
    MENode* dockNode = nullptr;
    std::vector<std::unique_ptr<World>> worlds;
    World* world = nullptr;
    double ut = 0.0;
    std::map<Guid, double> activated;

    explicit Scene(Fixture55& f) : fx(f) {}
    ~Scene() {
        FlightRegistry::Clear();
    }

    void BuildMission(const json& env) {
        mission = std::make_unique<Mission>();
        ConfigNode cfg("MISSION");
        cfg.AddValue("title", "node library");
        ConfigNode* nodes = cfg.AddNode("NODES");
        for (const auto& n : env["nodes"]) {
            ConfigNode* c = nodes->AddNode("NODE");
            c->AddValue("title", n["title"].get<std::string>());
            c->AddValue("id", n["guid"].get<std::string>());
            c->AddValue("isStartNode", n["isStart"].get<bool>());
            c->AddValue("activatedUT", n["activatedUT"].get<double>());
            c->AddValue("missionEndOptions", "Success");
        }
        ConfigNode* s = nodes->AddNode("NODE");
        s->AddValue("title", "Subject");
        s->AddValue("id", GuidOf(98));
        s->AddValue("isStartNode", false);
        s->AddValue("missionEndOptions", "Success");
        ConfigNode* d = nodes->AddNode("NODE");
        d->AddValue("title", "Dock parent");
        d->AddValue("id", GuidOf(99));
        d->AddValue("isStartNode", false);
        d->AddValue("missionEndOptions", "Success");
        mission->Load(cfg, false);
        subject = mission->GetNodeById(Guid::Parse(GuidOf(98)));
        dockNode = mission->GetNodeById(Guid::Parse(GuidOf(99)));
        REQUIRE(subject != nullptr);
        REQUIRE(dockNode != nullptr);
    }

    // the vessels / mission state of a scenario
    void SetEnv(const json& env) {
        MissionsEnvironment::universalTime = [this] { return ut; };
        ut = env["ut"].get<double>();
        mission->startedUT = env["startedUT"].get<double>();
        mission->currentScore = env["score"].get<float>();
        for (const auto& n : env["nodes"]) {
            MENode* node = mission->GetNodeById(Guid::Parse(n["guid"].get<std::string>()));
            if (node != nullptr) node->activatedUT = n["activatedUT"].get<double>();
        }
        fx.SetFunds(env["funds"]);
        FlightRegistry::Clear();
        worlds.push_back(std::make_unique<World>());
        world = worlds.back().get();
        int index = 0;
        for (const auto& vj : env["vessels"]) {
            auto v = std::make_unique<Vessel>();
            const uint32_t pid = vj["pid"].get<uint32_t>();
            v->persistentId = pid;
            v->id = Guid::Parse(vj["id"].get<std::string>());
            v->missionTime = vj["missionTime"].get<double>();
            v->altitude = vj["altitude"].get<double>();
            v->radarAltitude = vj["radarAltitude"].get<double>();
            const std::string st = vj["state"].get<std::string>();
            v->state = st == "ACTIVE" ? Vessel::State::ACTIVE : (st == "DEAD" ? Vessel::State::DEAD : Vessel::State::INACTIVE);
            v->currentStage = vj["currentStage"].get<int>();
            v->loaded = vj["loaded"].get<bool>();
            v->vesselType = dotnet::EnumParse<ksp::VesselType>(vj["type"].get<std::string>());
            const std::string disc = vj["discovery"].get<std::string>();
            v->GetDiscoveryInfo().SetLevel(disc == "Owned" ? DiscoveryLevels::Owned : (disc == "Unowned" ? DiscoveryLevels::Unowned : DiscoveryLevels::Presence));
            const auto& masses = vj["partMasses"];
            const auto& ids = vj["partIds"];
            const auto& crew = vj["crew"];
            for (size_t k = 0; k < masses.size(); ++k) {
                auto p = std::make_unique<Part>();
                p->fields.mass = masses[k].get<float>();
                p->fields.persistentId = ids[k].get<uint32_t>();
                if (k < crew.size()) {
                    auto pcm = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew);
                    pcm->trait = crew[k].get<std::string>();
                    p->protoModuleCrew.push_back(pcm);
                }
                v->parts.push_back(p.get());
                world->parts.push_back(std::move(p));
            }
            // crew beyond the part count: put the remaining members on the last part (the list order is what counts)
            for (size_t k = masses.size(); k < crew.size() && !v->parts.empty(); ++k) {
                auto pcm = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew);
                pcm->trait = crew[k].get<std::string>();
                v->parts.back()->protoModuleCrew.push_back(pcm);
            }
            world->resources[v.get()] = vj["resources"];
            if (!v->loaded && vj.contains("proto")) {
                world->protos.push_back(MakeProto(vj["id"].get<std::string>(), pid, vj["proto"]));
                v->protoVessel = world->protos.back().get();
            }
            if (!v->parts.empty()) v->rootPart = v->parts.front();
            world->byPid[pid] = v.get();
            FlightRegistry::Vessels.push_back(v.get());
            FlightRegistry::VesselsLoaded.push_back(v.get());
            world->vessels.push_back(std::move(v));
            ++index;
        }
        const int active = env["active"].get<int>();
        FlightRegistry::ActiveVessel = (active >= 0 && active < static_cast<int>(world->vessels.size())) ? world->vessels[static_cast<size_t>(active)].get() : nullptr;
        World* w = world;
        AwardEnvironment::connectedResourceTotal = [w](Vessel* v, const std::string& name, double& amount) {
            auto it = w->resources.find(v);
            amount = 0.0;
            if (it == w->resources.end() || !it->second.contains(name)) return true;
            amount = it->second[name].get<double>();
            return true;
        };
    }

    Vessel* VesselByPid(uint32_t pid) { return world->byPid.count(pid) ? world->byPid[pid] : nullptr; }
};

json PidOrNull(Vessel* v) { return v == nullptr ? json(nullptr) : json(v->persistentId); }

}  // namespace

// ---- registry and attribute table -----------------------------------------------------------------------------------------------

TEST_CASE("WP-55 registry: the ported classes (accounting from the C# sources) and the [MEScoreModule] table") {
    Fixture55 fx;
    const json& reg = Ref55()["registry"];
    // Tests / Actions: every registered name is in the reference's ported list and vice versa (lookup by name, the order is not observable)
    for (const char* folder : {"Tests", "Actions"}) {
        std::set<std::string> expected;
        for (const auto& n : reg[folder]["ported"]) expected.insert(n.get<std::string>());
        std::set<std::string> deferred;
        for (const auto& n : reg[folder]["deferred"]) deferred.insert(n.get<std::string>());
        for (const std::string& n : expected) {
            if (std::string(folder) == "Tests") CHECK_MESSAGE(TestModuleRegistry::Find(n) != nullptr, n);
            else CHECK_MESSAGE(ActionModuleRegistry::Find(n) != nullptr, n);
        }
        for (const auto& n : reg[folder]["slice2"]) {  // ported by WP-54 slice 2 (gated by test_missions2.cpp)
            if (std::string(folder) == "Tests") CHECK_MESSAGE(TestModuleRegistry::Find(n.get<std::string>()) != nullptr, n.get<std::string>());
            else CHECK_MESSAGE(ActionModuleRegistry::Find(n.get<std::string>()) != nullptr, n.get<std::string>());
        }
        for (const auto& n : reg[folder]["slice3"]) {  // ported by WP-54c (gated by test_missions3.cpp)
            if (std::string(folder) == "Tests") CHECK_MESSAGE(TestModuleRegistry::Find(n.get<std::string>()) != nullptr, n.get<std::string>());
            else CHECK_MESSAGE(ActionModuleRegistry::Find(n.get<std::string>()) != nullptr, n.get<std::string>());
        }
        for (const std::string& n : deferred) {
            // unregistered names are dropped like Type.GetType failing
            if (std::string(folder) == "Tests") CHECK_MESSAGE(TestModuleRegistry::Find(n) == nullptr, n);
            else CHECK_MESSAGE(ActionModuleRegistry::Find(n) == nullptr, n);
        }
    }
    for (const auto& n : reg["Adjusters"]["ported"]) {
        const DynamicModuleType* t = DynamicModuleRegistry::GetClassByName(n.get<std::string>());
        REQUIRE_MESSAGE(t != nullptr, n.get<std::string>());
        CHECK(t->IsSubclassOf("AdjusterPartModuleBase"));
        CHECK(t->IsSubclassOf("DynamicModule"));
        CHECK_FALSE(t->IsSubclassOf("ScoreModule"));
    }
    CHECK(DynamicModuleRegistry::GetClassByName("AdjusterEnginesBase") == nullptr);  // abstract bases are not instantiable
    // [MEScoreModule]: the table equals the attributes parsed from the C# sources
    const json& attr = Ref55()["scoreAttr"];
    for (auto it = attr.begin(); it != attr.end(); ++it) {
        auto found = ScoreModuleAttributes().find(it.key());
        REQUIRE_MESSAGE(found != ScoreModuleAttributes().end(), it.key());
        std::vector<std::string> expected;
        for (const auto& m : it.value()) expected.push_back(m.get<std::string>());
        CHECK_MESSAGE(found->second == expected, it.key());
    }
    std::set<std::string> slice2Tests;
    for (const auto& n : reg["Tests"]["slice2"]) slice2Tests.insert(n.get<std::string>());
    for (const auto& n : reg["Tests"]["slice3"]) slice2Tests.insert(n.get<std::string>());
    for (const auto& kv : ScoreModuleAttributes()) {
        const bool known = attr.contains(kv.first) || slice2Tests.count(kv.first) != 0;
        CHECK_MESSAGE(known, kv.first);
    }
}

// ---- Tests.* ------------------------------------------------------------------------------------------------------------------------

TEST_CASE("WP-55 reference: Tests.* modules (Awake, Load / Save, Test, GetScoreModifier, events)") {
    Fixture55 fx;
    size_t checked = 0;
    for (const json& c : Ref55()["tests"]) {
        const std::string name = c["name"].get<std::string>();
        Scene scene(fx);
        scene.BuildMission(c["env"]);
        scene.SetEnv(c["env"]);
        ConfigNode stubCfg("N");
        TestGroup* dockGroup = nullptr;
        TestGroup* tg = new TestGroup(scene.subject);
        scene.subject->testGroups.emplace_back(tg);
        ConfigNode cfg("N");
        const bool hasCfg = !c["cfg"].is_null();
        if (hasCfg) cfg = CfgFromJson(c["cfg"]);
        tg->AddTestModule(name, hasCfg ? &cfg : nullptr);
        REQUIRE_MESSAGE(tg->testModules.size() == 1, c["id"].get<std::string>());
        TestModule* mod = dynamic_cast<TestModule*>(tg->testModules[0].get());
        REQUIRE(mod != nullptr);
        for (const json& op : c["ops"]) {
            const std::string o = op["op"].get<std::string>();
            INFO("case ", c["id"].get<std::string>(), " op ", o);
            ++checked;
            if (o == "title") {
                CHECK(mod->title == op["out"].get<std::string>());
            } else if (o == "displayName") {
                CHECK(mod->GetDisplayName() == op["out"].get<std::string>());
            } else if (o == "info") {
                CHECK(mod->GetInfo() == op["out"].get<std::string>());
            } else if (o == "saved") {
                ConfigNode out("N");
                mod->Save(out);
                CHECK(Dump(out) == op["out"].get<std::string>());
            } else if (o == "body") {
                const json& f = op["field"];
                Editor::BaseAPField field(f["name"].get<std::string>(), f["guiName"].get<std::string>(), f["value"].get<std::string>(), f["has"].get<bool>());
                CHECK(mod->GetNodeBodyParameterString(field) == op["out"].get<std::string>());
            } else if (o == "env") {
                scene.SetEnv(op["env"]);
            } else if (o == "test") {
                const json& env = op["env"];
                if (name == "TestAccuracy") {
                    if (env.contains("dockParent")) {
                        if (dockGroup == nullptr) {
                            dockGroup = new TestGroup(scene.dockNode);
                            scene.dockNode->testGroups.emplace_back(dockGroup);
                            dockGroup->AddTestModule("TestAccuracyStub", nullptr);
                        }
                        auto* stub = dynamic_cast<AccuracyStub*>(dockGroup->testModules[0].get());
                        REQUIRE(stub != nullptr);
                        if (env["dockParent"]["accuracyModifier"].is_null()) stub->number.reset();
                        else stub->number = env["dockParent"]["accuracyModifier"].get<double>();
                        scene.subject->dockParentNode = scene.dockNode;
                    } else {
                        scene.subject->dockParentNode = nullptr;
                    }
                }
                CHECK(mod->Test() == op["out"].get<bool>());
            } else if (o == "scoreMod") {
                auto* s = dynamic_cast<IScoreableObjective*>(mod);
                REQUIRE(s != nullptr);
                ScoreModifier r = s->GetScoreModifier(op["module"].get<std::string>());
                const json& out = op["out"];
                CHECK(PidOrNull(r.vessel) == out["vessel"]);
                CHECK(r.number.has_value() == !out["number"].is_null());
            } else if (o == "init") {
                mod->InitializeTest();
            } else if (o == "clear") {
                mod->ClearTest();
            } else if (o == "event") {
                const json& e = op["ev"];
                const std::string en = e["name"].get<std::string>();
                if (en == "stageActivate") {
                    GameEvents::onStageActivate.Fire(e["arg"].get<int>());
                } else if (en == "addVessel" || en == "removeVessel") {
                    Vessel* v = scene.world->vessels[e["vessel"].get<size_t>()].get();
                    if (en == "addVessel") GameEvents::onFlightGlobalsAddVessel.Fire(v);
                    else GameEvents::onFlightGlobalsRemoveVessel.Fire(v);
                } else if (en == "soi") {
                    Vessel* v = scene.VesselByPid(e["vessel"].get<uint32_t>());
                    GameEvents::onVesselSOIChanged.Fire(GameEvents::HostedFromToAction<Vessel*, CelestialBody*>(
                        v, fx.Body(e["from"].get<std::string>()), fx.Body(e["to"].get<std::string>())));
                } else if (en == "recoveryComplete") {
                    auto pv = MakeProto(GuidOf(0), e["pid"].get<uint32_t>(), json::array());
                    KSP::UI::Screens::MissionRecoveryDialog mrd;
                    mrd.fundsEarned = e["fundsEarned"].get<double>();
                    GameEvents::onVesselRecoveryProcessingComplete.Fire(pv.get(), &mrd, 0.0f);
                } else if (en == "recoveryProcessing") {
                    auto pv = MakeProto(e["vesselGuid"].get<std::string>(), e["pid"].get<uint32_t>(), e["parts"]);
                    GameEvents::onVesselRecoveryProcessing.Fire(pv.get(), nullptr, 0.0f);
                } else {
                    FAIL("unknown event " << en);
                }
            } else {
                FAIL("unknown op " << o);
            }
        }
    }
    CHECK(checked > 2000);
}

// the synthetic ProtoVessel the recovery / unloaded-vessel cases are built on
TEST_CASE("WP-55 harness: synthetic ProtoVessel parts and resource snapshots") {
    Fixture55 fx;
    auto pv = MakeProto(GuidOf(3), 7, json::parse(R"([[{"name":"LiquidFuel","amount":5.0}],[{"name":"Oxidizer","amount":2.0,"max":10.0},{"name":"LiquidFuel","amount":1.0}]])"));
    CHECK(pv->persistentId == 7u);
    REQUIRE(pv->protoPartSnapshots.size() == 2);
    auto parts = pv->GetAllProtoPartsIncludingCargo();
    CHECK(parts.size() == 2);
    REQUIRE(pv->protoPartSnapshots[1]->resources.size() == 2);
    CHECK(pv->protoPartSnapshots[1]->resources[0]->amount == 2.0);
    CHECK(pv->protoPartSnapshots[1]->resources[0]->maxAmount == 10.0);
}

// ---- Actions.* ----------------------------------------------------------------------------------------------------------------------

namespace {
struct ResourceEventCounter {
    int count = 0;
    void On(MENode*, Vessel*, Part*, ProtoPartSnapshot*) { ++count; }
    ResourceEventCounter() { GameEvents::Mission::onNodeChangedVesselResources.Add(this, &ResourceEventCounter::On); }
    ~ResourceEventCounter() { GameEvents::Mission::onNodeChangedVesselResources.Remove(this, &ResourceEventCounter::On); }
};

// runs a coroutine to its end, collecting the WaitForSeconds values
std::vector<std::string> RunCoroutine(Enumerator e) {
    std::vector<std::string> waits;
    while (e.MoveNext()) {
        if (e.Current().kind == YieldInstruction::Kind::WaitForSeconds) waits.push_back(G(e.Current().seconds));
    }
    return waits;
}

json WaitsJson(const std::vector<std::string>& w) { return json(w); }
}  // namespace

TEST_CASE("WP-55 reference: Actions.* modules (Awake, Load / Save, Fire effects)") {
    Fixture55 fx;
    size_t checked = 0;
    for (const json& c : Ref55()["actions"]) {
        const std::string name = c["name"].get<std::string>();
        Scene scene(fx);
        scene.BuildMission(c["env"]);
        scene.SetEnv(c["env"]);
        ConfigNode cfg("N");
        const bool hasCfg = !c["cfg"].is_null();
        if (hasCfg) cfg = CfgFromJson(c["cfg"]);
        auto make = [&]() {
            IActionModule* a = scene.subject->AddAction(name, hasCfg ? &cfg : nullptr);
            REQUIRE_MESSAGE(a != nullptr, c["id"].get<std::string>());
            return dynamic_cast<ActionModule*>(a);
        };
        ActionModule* mod = make();
        REQUIRE(mod != nullptr);
        for (const json& op : c["ops"]) {
            const std::string o = op["op"].get<std::string>();
            INFO("case ", c["id"].get<std::string>(), " op ", o);
            ++checked;
            if (o == "title") {
                CHECK(mod->title == op["out"].get<std::string>());
            } else if (o == "displayName") {
                CHECK(mod->GetDisplayName() == op["out"].get<std::string>());
            } else if (o == "info") {
                CHECK(mod->GetInfo() == op["out"].get<std::string>());
            } else if (o == "saved") {
                ConfigNode out("N");
                mod->Save(out);
                CHECK(Dump(out) == op["out"].get<std::string>());
            } else if (o == "scoreDescription") {
                CHECK(dynamic_cast<IActionMissionScore*>(mod)->GetScoreDescription() == op["out"].get<std::string>());
            } else if (o == "appObjectiveInfo") {
                CHECK(mod->GetAppObjectiveInfo() == op["out"].get<std::string>());
            } else if (o == "body") {
                const json& f = op["field"];
                Editor::BaseAPField field(f["name"].get<std::string>(), f["guiName"].get<std::string>(), f["value"].get<std::string>(), f["has"].get<bool>());
                CHECK(mod->GetNodeBodyParameterString(field) == op["out"].get<std::string>());
            } else if (o == "env") {
                scene.SetEnv(op["env"]);
            } else if (o == "fire") {
                const json& env = op["env"];
                ActionModule* a = make();  // ActionVessel caches its vessel: a fresh module per scenario
                NodeLibraryEnvironment::screenMessages.clear();
                NodeLibraryEnvironment::dialogMessages.clear();
                NodeLibraryEnvironment::systemMessages.clear();
                NodeLibraryEnvironment::explodedParts.clear();
                NodeLibraryEnvironment::stagesActivated = 0;
                NodeLibraryEnvironment::moduleCalls.clear();
                scene.mission->isScoreEnabled = env.value("scoreEnabled", true);
                json out = json::object();
                World* w = scene.world;
                auto partIndex = [&](Part* p) -> json {
                    for (size_t vi = 0; vi < w->vessels.size(); ++vi)
                        for (size_t pi = 0; pi < w->vessels[vi]->parts.size(); ++pi)
                            if (w->vessels[vi]->parts[pi] == p) return json::array({vi, pi});
                    return json(nullptr);
                };
                Vessel* active = FlightRegistry::ActiveVessel;
                if (name == "ActionMessage") {
                    RunCoroutine(a->Fire());
                    json arr = json::array();
                    for (const auto& m : NodeLibraryEnvironment::screenMessages)
                        arr.push_back({{"text", m.text}, {"duration", G(m.duration)}, {"style", dotnet::EnumToString(m.style)}, {"persist", m.persist}});
                    out = {{"screen", arr}};
                } else if (name == "ActionDialogMessage") {
                    RunCoroutine(a->Fire());
                    REQUIRE(NodeLibraryEnvironment::dialogMessages.size() == 1);
                    const auto& d = NodeLibraryEnvironment::dialogMessages[0];
                    out = {{"isPaused", scene.subject->isPaused},
                           {"dialog", {{"heading", d.heading}, {"message", d.message}, {"instructor", d.instructorName}, {"button", d.buttonLabel},
                                       {"textAreaSize", d.textAreaSize},
                                       {"area", dotnet::EnumToString(static_cast<Actions::ActionDialogMessage::DialogMessageArea>(d.screenArea))},
                                       {"autoClose", d.autoClose}, {"timeout", d.autoCloseTimeOut}, {"grow", d.autoGrowDialogHeight},
                                       {"instructor2", d.instructorName2}}},
                           {"systemMessage", json::array({NodeLibraryEnvironment::systemMessages.at(0).first, NodeLibraryEnvironment::systemMessages.at(0).second})}};
                } else if (name == "ActionModifierFunds") {
                    RunCoroutine(a->Fire());
                    out = {{"funds", Funding::Instance == nullptr ? json(nullptr) : json(GD(Funding::Instance->Funds()))}};
                } else if (name == "ActionMissionScore" || name == "ActionModifierScore") {
                    RunCoroutine(a->Fire());
                    out = {{"score", G(scene.mission->currentScore)}};
                } else if (name == "ActionVessel") {
                    RunCoroutine(a->Fire());
                } else if (name == "ActionVesselActionGroup") {
                    auto* ag = dynamic_cast<Actions::ActionVesselActionGroup*>(a);
                    const bool before = active != nullptr ? active->ActionGroups()[ag->actionGroup] : false;
                    auto waits = RunCoroutine(a->Fire());
                    const bool after = active != nullptr ? active->ActionGroups()[ag->actionGroup] : false;
                    out = {{"waits", WaitsJson(waits)}, {"toggled", before != after ? json(dotnet::EnumToString(ag->actionGroup)) : json(nullptr)}};
                } else if (name == "ActionVesselExplode") {
                    auto waits = RunCoroutine(a->Fire());
                    json ex = json::array();
                    Vessel* target = dynamic_cast<Actions::ActionVesselExplode*>(a)->vessel();
                    for (Part* p : NodeLibraryEnvironment::explodedParts) {
                        json ix = partIndex(p);
                        ex.push_back(ix[1]);
                    }
                    out = {{"exploded", ex}, {"waits", WaitsJson(waits)}, {"state", target ? json(dotnet::EnumToString(target->state)) : json(nullptr)},
                           {"isRunning", a->isRunning}};
                    if (target != nullptr) out["pid"] = target->persistentId;
                } else if (name == "ActionVesselStage") {
                    auto waits = RunCoroutine(a->Fire());
                    const bool toggled = active != nullptr && active->ActionGroups()[KSPActionGroup::Stage];
                    out = {{"waits", WaitsJson(waits)}, {"stages", NodeLibraryEnvironment::stagesActivated}, {"toggled", toggled ? json("Stage") : json(nullptr)}};
                } else if (name == "ActionSetCamera") {
                    MissionCameraModeOptions m{};
                    MissionCameraLockOptions l{};
                    MissionsEnvironment::setLockedCamera = [&](Mission*, MissionCameraModeOptions a1, MissionCameraLockOptions b1) { m = a1; l = b1; };
                    RunCoroutine(a->Fire());
                    MissionsEnvironment::setLockedCamera = nullptr;
                    out = {{"camera", json::array({dotnet::EnumToString(m), dotnet::EnumToString(l)})}};
                } else if (name == "ActionPartResourceAmount") {
                    ResourceEventCounter ev;
                    json requested = nullptr;
                    const double result = env.value("requestResult", 0.0);
                    NodeLibraryEnvironment::vesselRequestResource = [&](Vessel*, Part*, int, double demand, bool) {
                        requested = GD(demand);
                        return result;
                    };
                    bool threw = false;
                    try {
                        RunCoroutine(a->Fire());
                    } catch (const NullReferenceException&) {
                        threw = true;
                    }
                    NodeLibraryEnvironment::vesselRequestResource = nullptr;
                    Vessel* target = dynamic_cast<Actions::ActionPartResourceAmount*>(a)->vessel();
                    if (threw) {
                        out = {{"throws", "NullReferenceException"}};
                    } else if (target == nullptr) {
                        out = json::object();
                    } else if (target->loaded) {
                        out = {{"requestDemand", requested}, {"event", ev.count}};
                    } else {
                        json amounts = json::array();
                        for (const auto& part : target->protoVessel->protoPartSnapshots) {
                            json row = json::array();
                            for (const auto& r : part->resources) row.push_back(GD(r->amount));
                            amounts.push_back(row);
                        }
                        out = {{"events", ev.count}, {"amounts", amounts}};
                    }
                } else if (name == "ActionPartExplode") {
                    NodeLibraryEnvironment::findLoadedPart = [&](uint32_t id) -> Part* {
                        for (auto& v : w->vessels) {
                            if (!v->loaded) continue;
                            for (Part* p : v->parts)
                                if (p->fields.persistentId == id) return p;
                        }
                        return nullptr;
                    };
                    bool switched = false;
                    NodeLibraryEnvironment::findNearestControllableVessel = [&](Vessel* v) { return v; };
                    NodeLibraryEnvironment::forceSetActiveVessel = [&](Vessel*) { switched = true; };
                    json rr = nullptr;
                    NodeLibraryEnvironment::randomRange = [&](int lo, int hi) {
                        rr = json::array({lo, hi});
                        return std::min(env.value("randomIndex", lo), hi - 1);
                    };
                    try {
                        RunCoroutine(a->Fire());
                        json ex = json::array();
                        for (Part* p : NodeLibraryEnvironment::explodedParts) ex.push_back(partIndex(p));
                        out = {{"exploded", ex}};
                        if (switched) out["switched"] = true;
                        if (!rr.is_null()) out["randomRange"] = rr;
                    } catch (const KeyNotFoundException&) {
                        out = {{"throws", "KeyNotFoundException"}};
                    } catch (const NullReferenceException&) {
                        out = {{"throws", "NullReferenceException"}};
                    }
                    NodeLibraryEnvironment::findLoadedPart = nullptr;
                    NodeLibraryEnvironment::findNearestControllableVessel = nullptr;
                    NodeLibraryEnvironment::forceSetActiveVessel = nullptr;
                    NodeLibraryEnvironment::randomRange = nullptr;
                } else {
                    FAIL("no fire executor for " << name);
                }
                CHECK(out == op["out"]);
            } else {
                FAIL("unknown op " << o);
            }
        }
    }
    CHECK(checked > 600);
}

// ---- Adjusters.* ------------------------------------------------------------------------------------------------------------------

namespace {
struct DummyModule {
    int x = 0;
};

Vector3f V3(const json& a) { return Vector3f(a[0].get<float>(), a[1].get<float>(), a[2].get<float>()); }
json V3Hex(Vector3f v) { return json::array({Hex32(v.x), Hex32(v.y), Hex32(v.z)}); }

std::unique_ptr<Adjusters::AdjusterPartModuleBase> MakeAdjuster(const std::string& name, int ctor) {
    if (ctor == 1) return Adjusters::AdjusterPartModuleBase::CreateInstanceOfModuleAdjuster(name);
    // the parameterless constructors
#define KSP_CTOR0(X) \
    if (name == #X) return std::make_unique<Adjusters::X>();
    KSP_CTOR0(AdjusterActiveRadiatorMaxEnergyTransfer) KSP_CTOR0(AdjusterAlternatorOutput) KSP_CTOR0(AdjusterControlSurfaceActuatorSpeed)
    KSP_CTOR0(AdjusterDataTransmitterPower) KSP_CTOR0(AdjusterDeployableSolarPanelEfficiency) KSP_CTOR0(AdjusterEnginesThrottle)
    KSP_CTOR0(AdjusterGeneratorEfficiency) KSP_CTOR0(AdjusterGimbalLimitControl) KSP_CTOR0(AdjusterKerbNetAccessDisabled)
    KSP_CTOR0(AdjusterLiftingSurfaceLiftForce) KSP_CTOR0(AdjusterLightIntensity) KSP_CTOR0(AdjusterReactionWheelTorque)
    KSP_CTOR0(AdjusterSASServiceLevel) KSP_CTOR0(AdjusterWheelBrakesTorque) KSP_CTOR0(AdjusterWheelMotorTorque)
    KSP_CTOR0(FailureActiveRadiatorStuck) KSP_CTOR0(FailureAnchoredDecouplerCantDecouple) KSP_CTOR0(FailureCargoBayStuck)
    KSP_CTOR0(FailureControlSurfaceDisableControl) KSP_CTOR0(FailureCrossfeedStuck) KSP_CTOR0(FailureDataTransmitterBroken)
    KSP_CTOR0(FailureDecoupleCantDecouple) KSP_CTOR0(FailureDeployablePartStuck) KSP_CTOR0(FailureDockingNodeCantUndock)
    KSP_CTOR0(FailureEnginesDeadEngine) KSP_CTOR0(FailureFuelLineBlocked) KSP_CTOR0(FailureGPSNoDisplay) KSP_CTOR0(FailureGrappleNodeCantGrapple)
    KSP_CTOR0(FailureGrappleNodeCantRelease) KSP_CTOR0(FailureLightFlickering) KSP_CTOR0(FailureLightStuck) KSP_CTOR0(FailureProbeControlPointBroken)
    KSP_CTOR0(FailureRCSFireRandomly) KSP_CTOR0(FailureRCSStopWorking) KSP_CTOR0(FailureReactionWheelBroken) KSP_CTOR0(FailureReactionWheelStuck)
    KSP_CTOR0(FailureResourceIntakeStuck) KSP_CTOR0(FailureScienceExperimentDisable) KSP_CTOR0(FailureWheelDeploymentStuck)
#undef KSP_CTOR0
    return nullptr;
}

template <class T>
json StuckState(Adjusters::AdjusterPartModuleBase* a) {
    auto* t = dynamic_cast<T*>(a);
    return t ? json(dotnet::EnumToString(t->stuckState)) : json(nullptr);
}
json StuckStateOf(Adjusters::AdjusterPartModuleBase* a) {
    using namespace Adjusters;
    for (json j : {StuckState<FailureActiveRadiatorStuck>(a), StuckState<FailureCargoBayStuck>(a), StuckState<FailureCrossfeedStuck>(a),
                   StuckState<FailureDeployablePartStuck>(a), StuckState<FailureLightStuck>(a), StuckState<FailureResourceIntakeStuck>(a),
                   StuckState<FailureWheelDeploymentStuck>(a)})
        if (!j.is_null()) return j;
    return json(nullptr);
}

// the Apply* maths by class name
json ApplyOf(Adjusters::AdjusterPartModuleBase* a, const std::string& name, const json& args) {
    using namespace Adjusters;
    const auto f = [&] { return args[0].get<float>(); };
#define KSP_F(X, FN) \
    if (name == #X) return Hex32(dynamic_cast<X*>(a)->FN(f()));
    KSP_F(AdjusterAlternatorOutput, ApplyOutputAdjustment) KSP_F(AdjusterControlSurfaceActuatorSpeed, ApplyActuatorSpeedAdjustment)
    KSP_F(AdjusterDeployableSolarPanelEfficiency, ApplyEfficiencyAdjustment) KSP_F(AdjusterEnginesThrottle, ApplyThrottleAdjustment)
    KSP_F(AdjusterGeneratorEfficiency, ApplyEfficiencyAdjustment) KSP_F(AdjusterLightIntensity, ApplyIntensityAdjustment)
    KSP_F(AdjusterWheelBrakesTorque, ApplyTorqueAdjustment) KSP_F(AdjusterWheelMotorTorque, ApplyTorqueAdjustment)
    KSP_F(FailureControlSurfaceDisableControl, ApplyActuatorSpeedAdjustment)
#undef KSP_F
    if (name == "AdjusterActiveRadiatorMaxEnergyTransfer")
        return Hex64(dynamic_cast<AdjusterActiveRadiatorMaxEnergyTransfer*>(a)->ApplyMaxEnergyTransferAdjustment(args[0].get<double>()));
    if (name == "AdjusterDataTransmitterPower") return Hex64(dynamic_cast<AdjusterDataTransmitterPower*>(a)->ApplyPowerAdjustment(args[0].get<double>()));
    if (name == "AdjusterSASServiceLevel")
        return std::to_string(dynamic_cast<AdjusterSASServiceLevel*>(a)->ApplySASServiceLevelAdjustment(args[0].get<int>()));
    if (name == "AdjusterLiftingSurfaceLiftForce") return V3Hex(dynamic_cast<AdjusterLiftingSurfaceLiftForce*>(a)->ApplyLiftForceAdjustment(V3(args)));
    if (name == "AdjusterGimbalLimitControl") return V3Hex(dynamic_cast<AdjusterGimbalLimitControl*>(a)->ApplyControlAdjustment(V3(args)));
    if (name == "AdjusterReactionWheelTorque") return V3Hex(dynamic_cast<AdjusterReactionWheelTorque*>(a)->ApplyTorqueAdjustment(V3(args)));
    if (name == "FailureReactionWheelStuck") return V3Hex(dynamic_cast<FailureReactionWheelStuck*>(a)->ApplyTorqueAdjustment(Vector3f()));
    if (name == "FailureRCSFireRandomly") {
        size_t i = 0;
        NodeLibraryEnvironment::randomValue = [&] { return args[i++].get<float>(); };
        Vector3f r = dynamic_cast<FailureRCSFireRandomly*>(a)->ApplyInputLinearAdjustment(Vector3f());
        NodeLibraryEnvironment::randomValue = nullptr;
        return V3Hex(r);
    }
    FAIL("no apply executor for " << name);
    return nullptr;
}

json FlagOf(Adjusters::AdjusterPartModuleBase* a, const std::string& fn) {
    using namespace Adjusters;
#define KSP_FLAG(BASE, FNAME) \
    if (fn == #FNAME) { auto* b = dynamic_cast<BASE*>(a); REQUIRE(b != nullptr); return b->FNAME(); }
    KSP_FLAG(AdjusterActiveRadiatorBase, IsBlockingCooling)
    KSP_FLAG(AdjusterCargoBayBase, IsCargoBayStuck) KSP_FLAG(AdjusterDataTransmitterBase, IsDataTransmitterBroken)
    KSP_FLAG(AdjusterDeployablePartBase, IsDeployablePartStuck) KSP_FLAG(AdjusterDockingNodeBase, IsBlockingUndock)
    KSP_FLAG(AdjusterEnginesBase, IsEngineDead) KSP_FLAG(AdjusterGrappleNodeBase, IsBlockingGrappleRelease)
    KSP_FLAG(AdjusterGrappleNodeBase, IsBlockingGrappleGrab) KSP_FLAG(AdjusterProbeControlPointBase, IsProbeControlPointBroken)
    KSP_FLAG(AdjusterRCSBase, IsRCSBroken) KSP_FLAG(AdjusterReactionWheelBase, IsReactionWheelBroken)
#undef KSP_FLAG
    if (fn == "IsBlockingDecouple") {  // AdjusterDecoupleBase and AdjusterAnchoredDecouplerBase both declare it
        if (auto* d = dynamic_cast<AdjusterDecoupleBase*>(a)) return d->IsBlockingDecouple();
        if (auto* d = dynamic_cast<AdjusterAnchoredDecouplerBase*>(a)) return d->IsBlockingDecouple();
    }
    FAIL("no flag executor for " << fn);
    return nullptr;
}

void SetScenarioHooks(const json& scen) {
    DummyModule* dummy = new DummyModule;  // never dereferenced: only the hooks look at it
    (void)dummy;
}
}  // namespace

TEST_CASE("WP-55 reference: Adjusters.* dynamic modules (Load / Save, maths, Activate, flags)") {
    Fixture55 fx;
    size_t checked = 0;
    static DummyModule dummy;
    PartModule* fakeModule = reinterpret_cast<PartModule*>(&dummy);
    for (const json& c : Ref55()["adjusters"]) {
        const std::string name = c["name"].get<std::string>();
        MissionsEnvironment::newGuid = [&fx] {
            ++fx.guidCounter55;
            char b[48];
            std::snprintf(b, sizeof b, "55555555-0000-0000-0000-%012llu", static_cast<unsigned long long>(fx.guidCounter55));
            return Guid::Parse(b);
        };
        fx.guidCounter55 = 0;
        DynamicModule::ResetObjectIds();
        auto mod = MakeAdjuster(name, c["ctor"].get<int>());
        REQUIRE_MESSAGE(mod != nullptr, c["id"].get<std::string>());
        if (!c["cfg"].is_null()) {
            ConfigNode cfg = CfgFromJson(c["cfg"]);
            mod->Load(cfg);
        }
        for (const json& op : c["ops"]) {
            const std::string o = op["op"].get<std::string>();
            INFO("case ", c["id"].get<std::string>(), " op ", o);
            ++checked;
            if (o == "guiName") {
                auto tmp = MakeAdjuster(name, op["ctor"].get<int>());
                REQUIRE(tmp != nullptr);
                CHECK(tmp->guiName == op["out"].get<std::string>());
            } else if (o == "displayName") {
                CHECK(mod->GetDisplayName() == op["out"].get<std::string>());
            } else if (o == "saved") {
                ConfigNode out("N");
                mod->Save(out);
                CHECK(Dump(out) == op["out"].get<std::string>());
            } else if (o == "flags") {
                CHECK(json::array({mod->canBeRepaired, mod->disableKSPFields, mod->disableKSPActions, mod->disableKSPEvents}) == op["out"]);
            } else if (o == "body") {
                const json& f = op["field"];
                Editor::BaseAPField field(f["name"].get<std::string>(), f["guiName"].get<std::string>(), f["value"].get<std::string>(), f["has"].get<bool>());
                CHECK(mod->GetNodeBodyParameterString(field) == op["out"].get<std::string>());
            } else if (o == "apply") {
                if (name == "FailureLightFlickering") {
                    const json& a = op["args"];
                    NodeLibraryEnvironment::randomRangeFloat = [&](float, float) { return a[1].get<float>(); };
                    NodeLibraryEnvironment::deltaTime = [&] { return a[2].get<float>(); };
                    auto* fl = dynamic_cast<Adjusters::FailureLightFlickering*>(mod.get());
                    const float r = fl->ApplyIntensityAdjustment(a[0].get<float>());
                    NodeLibraryEnvironment::randomRangeFloat = nullptr;
                    NodeLibraryEnvironment::deltaTime = nullptr;
                    CHECK(Hex32(r) == op["out"]["out"].get<std::string>());
                } else {
                    CHECK(ApplyOf(mod.get(), name, op["args"]) == op["out"]["out"]);
                }
            } else if (o == "activate" || o == "deactivate") {
                const json& scen = op["scen"];
                std::unique_ptr<Adjusters::AdjusterPartModuleBase> target;
                Adjusters::AdjusterPartModuleBase* t = mod.get();
                if (op.value("fresh", false)) {
                    target = MakeAdjuster(name, c["ctor"].get<int>());
                    if (!c["cfg"].is_null()) {
                        ConfigNode cfg = CfgFromJson(c["cfg"]);
                        target->Load(cfg);
                    }
                    t = target.get();
                }
                NodeLibraryEnvironment::moduleCalls.clear();
                const std::string module = scen.contains("module") && !scen["module"].is_null() ? scen["module"].get<std::string>() : "";
                NodeLibraryEnvironment::moduleIsA = [module](PartModule*, const std::string& cls) { return cls == module; };
                NodeLibraryEnvironment::moduleQuery = [scen](PartModule*, const std::string&, const std::string& q) {
                    return scen.contains("queries") && scen["queries"].contains(q) && scen["queries"][q].get<bool>();
                };
                NodeLibraryEnvironment::moduleText = [scen](PartModule*, const std::string&, const std::string& m) {
                    return scen.contains("text") && scen["text"].contains(m) ? scen["text"][m].get<std::string>() : std::string();
                };
                NodeLibraryEnvironment::reactionWheelAppliedTorque = [scen](PartModule*) {
                    return scen.contains("torque") ? V3(scen["torque"]) : Vector3f();
                };
                if (scen.contains("rnd")) NodeLibraryEnvironment::randomRangeFloat = [scen](float, float) { return scen["rnd"].get<float>(); };
                if (scen.contains("dt")) NodeLibraryEnvironment::deltaTime = [scen] { return scen["dt"].get<float>(); };
                t->adjustedModule = fakeModule;
                t->AdjusterState = "#autoLOC_8100276";
                if (o == "activate") t->Activate();
                else t->Deactivate();
                json calls = json::array();
                for (const auto& s : NodeLibraryEnvironment::moduleCalls) calls.push_back(s);
                NodeLibraryEnvironment::moduleIsA = nullptr;
                NodeLibraryEnvironment::moduleQuery = nullptr;
                NodeLibraryEnvironment::moduleText = nullptr;
                NodeLibraryEnvironment::reactionWheelAppliedTorque = nullptr;
                NodeLibraryEnvironment::randomRangeFloat = nullptr;
                NodeLibraryEnvironment::deltaTime = nullptr;
                if (o == "deactivate") {
                    CHECK(calls == op["out"]["calls"]);
                } else {
                    const json& exp = op["out"];
                    const json status = t->AdjusterState == "#autoLOC_8100276" ? json(nullptr) : json(t->AdjusterState);
                    CHECK(status == exp["status"]);
                    CHECK(calls == exp["calls"]);
                    if (exp.contains("stuckState")) CHECK(StuckStateOf(t) == exp["stuckState"]);
                }
            } else if (o == "flag") {
                CHECK(FlagOf(mod.get(), op["fn"].get<std::string>()) == op["out"]);
            } else {
                FAIL("unknown op " << o);
            }
        }
    }
    CHECK(checked > 2000);
}
