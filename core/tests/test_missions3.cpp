// WP-54c unit tests: TestDistance, ActionPartFailure / Repair / ResourceDrain, the Mission craft-file bookkeeping and the rest of the slice-3 pieces
// (core/include/ksp/missions/{mission_tests4,mission_actions3,mission_system,...}.hpp).
//
// Like the WP-54b gate: no golden and no capture of the mission runtime exists and the mission files of the GameData mirror are 0-byte files (see STATUS
// WP-54), so every expectation of the "reference" cases comes from tools/wp54c/ref54c.py (independent Python written from the decompiled C#,
// tests/data/wp54c_reference.json). The scenarios are labelled synthetic inputs to the algorithms, not game data. Text goes through the real
// Squad/Localization/dictionary.cfg. The only tolerance is on values that depend on the double libm (acos / cos / sin of the great-circle distance and of the
// world surface position): booleans are compared exactly, the scenarios keep a relative margin of 1e-6 between the measured distance and the threshold.
#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <variant>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission_actions2.hpp"
#include "ksp/missions/mission_actions3.hpp"
#include "ksp/missions/mission_extras.hpp"
#include "ksp/missions/mission_system.hpp"
#include "ksp/missions/mission_tests4.hpp"
#include "ksp/missions/missions.hpp"
#include "ksp/missions/mission_validator.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/vessel/vessel.hpp"
#include "missions2_world.hpp"
#include "missions_fixture.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using namespace ksp::Expansions::Missions;
using json = nlohmann::json;
using namespace ksp::mtest;
using namespace ksp::mtest54b;

namespace {

const json& Ref3() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp54c_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

// ---- the mission of a case: the "mission" op (vessel situations / create actions on nodes of their own, mapped vessels, create actions, extra nodes) ----------------------------

// one node per create action (GetVesselSituationByVesselID finds the node of the situation, then the FIRST ActionCreateVessel of that node)
MENode* SpawnActionNode(Mission& m, int k) {
    MENode* n = MENode::Spawn(&m);
    n->SetIdForTests(Guid::Parse(GuidOf(200 + k)));
    n->SetTitleNoGui("Create " + std::to_string(k));
    m.nodes.Add(n->id(), n);
    return n;
}

void BuildMissionSpec(Scene54& scene, const json& spec) {
    Mission& m = *scene.mission;
    int k = 0;
    for (const json& vj : spec["vessels"]) {
        auto* a = dynamic_cast<Actions::ActionCreateVessel*>(SpawnActionNode(m, k++)->AddAction("ActionCreateVessel", nullptr));
        REQUIRE(a != nullptr);
        a->vesselSituation->persistentId = vj["pid"].get<uint32_t>();
        a->vesselSituation->vesselName = vj["name"].get<std::string>();
        a->vesselSituation->craftFile = vj["craft"].get<std::string>();
        a->vesselSituation->playerCreated = vj.value("player", true);
    }
    if (spec.contains("failures")) {
        for (const json& fj : spec["failures"]) {
            ConfigNode cfg = CfgFromJson(fj);
            REQUIRE(SpawnActionNode(m, k++)->AddAction("ActionPartFailure", &cfg) != nullptr);
        }
    }
    for (const json& fj : spec["flags"]) {
        auto* a = dynamic_cast<Actions::ActionCreateFlag*>(SpawnActionNode(m, k++)->AddAction("ActionCreateFlag", nullptr));
        REQUIRE(a != nullptr);
        a->persistentID = fj["pid"].get<uint32_t>();
        a->siteName = fj["site"].get<std::string>();
    }
    for (const json& aj : spec["asteroids"]) {
        auto* a = dynamic_cast<Actions::ActionCreateAsteroid*>(SpawnActionNode(m, k++)->AddAction("ActionCreateAsteroid", nullptr));
        REQUIRE(a != nullptr);
        a->asteroid->persistentId = aj["pid"].get<uint32_t>();
        a->asteroid->name = aj["name"].get<std::string>();
    }
    for (const json& cj : spec["comets"]) {
        auto* a = dynamic_cast<Actions::ActionCreateComet*>(SpawnActionNode(m, k++)->AddAction("ActionCreateComet", nullptr));
        REQUIRE(a != nullptr);
        a->comet->persistentId = cj["pid"].get<uint32_t>();
        a->comet->name = cj["name"].get<std::string>();
    }
    m.mappedVessels.clear();
    for (const json& mj : spec["mapped"]) {
        MissionMappedVessel mv(mj["part"].get<uint32_t>(), mj["orig"].get<uint32_t>(), mj["cur"].get<uint32_t>(), mj["pvn"].get<std::string>(), mj["craft"].get<std::string>(),
                               mj["sit"].get<std::string>());
        mv.mappedVesselPersistentId = mj["mapped"].get<uint32_t>();
        m.mappedVessels.push_back(mv);
    }
    if (spec.contains("nodes")) {
        for (const json& nj : spec["nodes"]) {
            MENode* n = MENode::Spawn(&m);
            n->SetIdForTests(Guid::Parse(nj["guid"].get<std::string>()));
            n->SetTitleNoGui(nj["title"].get<std::string>());
            TestGroup* tg = new TestGroup(n);
            n->testGroups.emplace_back(tg);
            const std::string test = nj["test"].get<std::string>();
            if (test == "TestGoTo") {
                const json& a = nj["area"];
                ConfigNode cfg("TESTMODULE");
                cfg.AddValue("bodyName", Ref3()["bodies"][a["body"].get<size_t>()]["name"].get<std::string>());
                cfg.AddValue("lat", a["lat"].get<double>());
                cfg.AddValue("lon", a["lon"].get<double>());
                cfg.AddValue("alt", a["alt"].get<double>());
                cfg.AddValue("radius", a["radius"].get<float>());
                tg->AddTestModule(test, &cfg);
            } else {
                tg->AddTestModule(test, nullptr);
            }
            m.nodes.Add(n->id(), n);
        }
    }
    Runtime::MissionSystem::missions = {&m};
    Mission* mp = &m;
    MissionsEnvironment::editorMission = [mp] { return mp; };
}

}  // namespace

// ---- TestDistance ----------------------------------------------------------------------------------------------------------------------------------------

namespace {

// a MEGUI control that records SetActive(value) as [name, value] (the visibility callbacks of TestDistance)
struct MockControl : Editor::IMEGUIControl {
    std::string name;
    json* log = nullptr;
    void SetActive(bool v) override { log->push_back(json::array({name, v})); }
};
// the node canvas box of the subject node: the UpdateNodeBodyUI calls of the callbacks reach DisplayNodeBodyParameters
struct NullGui : Editor::IMEGUINode {
    json* log = nullptr;
    void SetTitleText(const std::string&) override {}
    void SetEndNode(bool) override {}
    void UpdateInputConnectorImage() override {}
    void SetNodeColor(const Color&) override {}
    void DisplayNodeBodyParameters() override {
        if (log != nullptr) log->push_back(json::array({"body"}));
    }
    void ClearValidityIndicators() override {}
    void SetValidityIndicators(const Editor::MissionValidationTestResult&) override {}
    std::vector<Editor::IMEGUIConnector*> OutputConnectors() override { return {}; }
};
struct MockDropdown : Editor::IMEGUIDropdownList {
    std::optional<int> selected;
    std::string name;
    json* log = nullptr;
    void SetActive(bool v) override { log->push_back(json::array({name, v})); }
    void RebuildDropDown() override {}
    std::optional<int> SelectedValueInt() override { return selected; }
    int GetItemIndex(std::optional<int>) override { return 0; }
};

bool DistanceExtraOp(Fixture54b& world, const std::string& o, const json& op, Scene54& scene, TestModule* mod) {
    if (o == "mission") {
        BuildMissionSpec(scene, op["spec"]);
        NodeLibraryEnvironment::launchSiteDisplayName = [](const std::string& n) { return "[" + n + "]"; };
        return true;
    }
    if (o == "phys") {
        const json& ph = op["phys"];
        for (size_t i = 0; i < world.bodies.size(); ++i) {
            const json& pj = ph[i];
            world.bodies[i]->position = Vector3d(pj["pos"][0].get<double>(), pj["pos"][1].get<double>(), pj["pos"][2].get<double>());
            CelestialFrame::SetFrame(pj["frame"][0].get<double>(), pj["frame"][1].get<double>(), pj["frame"][2].get<double>(), world.bodies[i]->BodyFrame);
        }
        for (auto it = op["vessels"].begin(); it != op["vessels"].end(); ++it) {
            Vessel* v = scene.ByPid(static_cast<uint32_t>(std::stoul(it.key())));
            REQUIRE(v != nullptr);
            v->precalc.firstStatsRunComplete = true;
            v->CoMD = Vector3d(it.value()[0].get<double>(), it.value()[1].get<double>(), it.value()[2].get<double>());
        }
        json sites = op["sites"];
        NodeLibraryEnvironment::getSpaceCenterFacility = [sites](const std::string& name) -> std::function<Vector3d()> {
            auto k = "facility:" + name;
            if (!sites.contains(k)) return {};
            const json p = sites[k];
            return [p] { return Vector3d(p[0].get<double>(), p[1].get<double>(), p[2].get<double>()); };
        };
        NodeLibraryEnvironment::getLaunchSite = [sites](const std::string& name) -> std::function<Vector3d()> {
            auto k = "launchsite:" + name;
            if (!sites.contains(k)) return {};
            const json p = sites[k];
            return [p] { return Vector3d(p[0].get<double>(), p[1].get<double>(), p[2].get<double>()); };
        };
        return true;
    }
    if (o == "uiFrom" || o == "uiTo" || o == "uiSetup") {
        auto* d = dynamic_cast<Tests::TestDistance*>(mod);
        REQUIRE(d != nullptr);
        json log = json::array();
        MockControl fk, fv, tk, tv, tl, tb, ta, tc, tf, ts, tn;
        const std::pair<MockControl*, const char*> names[] = {{&fk, "fromKerbal"}, {&fv, "fromVessel"}, {&tk, "toKerbal"}, {&tv, "toVessel"}, {&tl, "toLocation"}, {&tb, "toBody"},
                                                               {&ta, "toAsteroid"}, {&tc, "toComet"}, {&tf, "toFlag"}, {&ts, "toLaunchSite"}, {&tn, "toNode"}};
        for (auto& [c, n] : names) {
            c->name = n;
            c->log = &log;
        }
        d->distanceFromKerbalParameterReference = &fk;
        d->distanceFromVesselParameterReference = &fv;
        d->distanceToKerbalParameterReference = &tk;
        d->distanceToVesselParameterReference = &tv;
        d->distanceToLocationParameterReference = &tl;
        d->distanceToCelestialBodyParameterReference = &tb;
        d->distanceToAsteroidParameterReference = &ta;
        d->distanceToCometParameterReference = &tc;
        d->distanceToFlagParameterReference = &tf;
        d->distanceToLaunchSiteParameterReference = &ts;
        d->distanceToNodeParameterReference = &tn;
        MockDropdown from, to;
        from.log = to.log = &log;
        NullGui gui;
        gui.log = &log;
        scene.subject->guiNode = &gui;
        if (o == "uiFrom") {
            from.selected = op["sel"].is_null() ? std::nullopt : std::optional<int>(op["sel"].get<int>());
            d->OnDistanceFromValueChange(&from, 0);
        } else if (o == "uiTo") {
            to.selected = op["sel"].is_null() ? std::nullopt : std::optional<int>(op["sel"].get<int>());
            d->OnDistanceToValueChange(&to, 0);
        } else {
            from.selected = op["from"].get<int>();
            to.selected = op["to"].get<int>();
            d->distanceFromTargetParameterReference = &from;
            d->distanceToTargetParameterReference = &to;
            d->ParameterSetupComplete();
            d->distanceFromTargetParameterReference = nullptr;
            d->distanceToTargetParameterReference = nullptr;
        }
        scene.subject->guiNode = nullptr;
        CHECK(log == op["out"]);
        d->distanceFromKerbalParameterReference = d->distanceFromVesselParameterReference = nullptr;
        d->distanceToKerbalParameterReference = d->distanceToVesselParameterReference = d->distanceToLocationParameterReference = nullptr;
        d->distanceToCelestialBodyParameterReference = d->distanceToAsteroidParameterReference = d->distanceToCometParameterReference = nullptr;
        d->distanceToFlagParameterReference = d->distanceToLaunchSiteParameterReference = d->distanceToNodeParameterReference = nullptr;
        return true;
    }
    return false;
}

}  // namespace

TEST_CASE("WP-54c reference: TestDistance (Load / Save, Test over seeded worlds, validation, body text, score modifier, control visibility)") {
    Fixture54b world;
    ResourceEnvironment::tickCount = [] { return 12345; };  // new VesselGroundLocation(node, Flag): KSPRandom seeded from the tick count (the reference pins it)
    std::set<std::string> classes;
    size_t checked = 0, throwing = 0, cases = 0;
    for (const json& c : Ref3()["cases"]) {
        if (c["kind"].get<std::string>() != "distance") continue;
        ++cases;
        if (std::getenv("KSP_DBG")) std::fprintf(stderr, "DIST case %zu loadThrows=%d\n", cases, (int)c.value("loadThrows", false));
        if (c.value("loadThrows", false)) {
            // TestDistance.Load tests configNode2 where it needs configNode3: a DISTANCETOKERBAL node without DISTANCETOLOCATION throws
            Scene54 scene(world);
            scene.BuildMission();
            TestGroup* tg = new TestGroup(scene.subject);
            scene.subject->testGroups.emplace_back(tg);
            ConfigNode cfg = CfgFromJson(c["cfg"]);
            CHECK_THROWS(tg->AddTestModule("TestDistance", &cfg));
            ++throwing;
            continue;
        }
        json one = json::array({c});
        one[0]["kind"] = "test";
        const ExtraOp extra = [&](const std::string& o, const json& op, Scene54& scene, TestModule* mod) { return DistanceExtraOp(world, o, op, scene, mod); };
        checked += RunTestModuleCases(world, one, extra, classes);
        NodeLibraryEnvironment::Reset();
        MissionsEnvironment::editorMission = nullptr;
        Runtime::MissionSystem::missions.clear();
    }
    ResourceEnvironment::tickCount = nullptr;
    CHECK(cases >= 150);
    CHECK(throwing >= 1);
    CHECK(checked > 3000);
}

// ---- ActionPartFailure / ActionPartRepair / ActionPartResourceDrain ----------------------------------------------------------------------------------------

namespace {

struct MockSelector : Editor::IMEGUIVesselPartSelector {
    Part* part = nullptr;
    Part* GetSelectedPart() override { return part; }
    void RefreshUI() override {}
};
struct MockModuleList : Editor::IMEGUIDynamicModuleListControl {
    void RefreshUI() override {}
    void SetAddButtonText(const std::string&) override {}
};

struct ResEvents {
    json events = json::array();
    void On(MENode*, Vessel* v, Part* p, ProtoPartSnapshot* s) {
        events.push_back(json::array({v != nullptr ? json(v->persistentId) : json(nullptr), p != nullptr ? json(p->fields.persistentId) : json(nullptr),
                                      s != nullptr ? json(s->persistentId) : json(nullptr)}));
    }
    ResEvents() { GameEvents::Mission::onNodeChangedVesselResources.Add(this, &ResEvents::On); }
    ~ResEvents() { GameEvents::Mission::onNodeChangedVesselResources.Remove(this, &ResEvents::On); }
};

// runs a coroutine to its end: counts the `yield return null`, collects the WaitForSeconds
bool RunCo(Enumerator e, int& yields, json& waits, int limit = 100000) {
    yields = 0;
    waits = json::array();
    int guard = 0;
    try {
        while (e.MoveNext()) {
            REQUIRE(++guard < limit);
            if (e.Current().kind == YieldInstruction::Kind::WaitForSeconds) {
                waits.push_back(e.Current().seconds == 0.5f ? "0.5" : "?");
            } else {
                ++yields;
            }
        }
    } catch (const std::exception&) {
        return true;  // the C# NullReferenceException that ends the routine
    }
    return false;
}

json ProtoSnapshot(World54& w) {
    json out = json::array();
    for (auto& v : w.vessels) {
        if (v->loaded || v->protoVessel == nullptr) continue;
        for (size_t i = 0; i < v->protoVessel->protoPartSnapshots.size(); ++i) {
            ProtoPartSnapshot* p = v->protoVessel->protoPartSnapshots[i].get();
            json mods = json::array();
            for (auto& m : p->modules) mods.push_back(Dump(*m->moduleValues));
            json res = json::array();
            for (auto& r : p->resources) res.push_back(json::array({r->resourceName.value_or(""), r->amount}));
            out.push_back(json::array({v->persistentId, p->persistentId, p->state, mods, res}));
        }
    }
    return out;
}

}  // namespace

TEST_CASE("WP-54c reference: ActionPartFailure / ActionPartRepair / ActionPartResourceDrain (Load / Save, Fire against loaded and unloaded vessels, proto adjuster config, drain loop)") {
    Fixture54b world;
    Adjusters::InitialiseAdjusterTypes();  // MissionsUtils.InitialiseAdjusterTypes: the part module <-> adjuster tables (ActionPartFailure.OnPartIDChanged reads them)
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", true);  // ExpansionsLoader.IsExpansionInstalled("MakingHistory") of the Proto* adjuster code
    size_t checked = 0;
    std::map<std::string, int> counts;
    for (const json& c : Ref3()["cases"]) {
        if (c["kind"].get<std::string>() != "pactions") continue;
        const std::string name = c["cls"].get<std::string>();
        counts[name]++;
        for (const char* n : {"Jebediah Kerman", "Bill Kerman", "Bob Kerman", "Valentina Kerman", "Gus Kerman", "Tourist A", "Tourist B"})
            (*world.roster)[std::string(n)]->set_rosterStatus(ProtoCrewMember::RosterStatus::Available);
        Scene54 scene(world);
        scene.BuildMission();
        ConfigNode cfg = CfgFromJson(c["cfg"]);
        IActionModule* ia = scene.subject->AddAction(name, &cfg);
        REQUIRE_MESSAGE(ia != nullptr, name);
        ActionModule* mod = dynamic_cast<ActionModule*>(ia);
        REQUIRE(mod != nullptr);
        std::vector<std::unique_ptr<ResEvents>> events;
        std::shared_ptr<std::deque<double>> reqV = std::make_shared<std::deque<double>>(), reqP = std::make_shared<std::deque<double>>();
        auto requests = std::make_shared<json>(json::array());
        std::map<Part*, const json*> partJson;
        json craft = json::object();
        for (const json& op : c["ops"]) {
            const std::string o = op["op"].get<std::string>();
            INFO("case ", name, " op ", o);
            ++checked;
            if (std::getenv("KSP_DBG")) std::fprintf(stderr, "PA %s op %s\n", name.c_str(), o.c_str());
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
                    CHECK(mod->GetNodeBodyParameterString(field) == op["out"].get<std::string>());
                } else if (o == "mission") {
                    BuildMissionSpec(scene, op["spec"]);
                } else if (o == "env") {
                    scene.SetEnv(op["env"]);
                } else if (o == "pw") {
                    // the PartModule / resource seams of the world just built (labelled synthetic: both sides use the same data)
                    World54* w = scene.world;
                    partJson.clear();
                    const json& vs = w->env["vessels"];
                    for (size_t i = 0; i < w->vessels.size(); ++i)
                        for (size_t k = 0; k < w->vessels[i]->parts.size(); ++k) partJson[w->vessels[i]->parts[k]] = &vs[i]["parts"][k];
                    NodeLibraryEnvironment::findUnloadedPart = [w](uint32_t id) -> ProtoPartSnapshot* {
                        if (id == 0) return nullptr;
                        for (auto& v : w->vessels)
                            if (!v->loaded && v->protoVessel != nullptr)
                                for (auto& p : v->protoVessel->protoPartSnapshots)
                                    if (p->persistentId == id) return p.get();
                        return nullptr;
                    };
                    NodeLibraryEnvironment::partModuleNames = [&partJson](Part* p) {
                        std::vector<std::string> names;
                        for (const json& m : (*partJson.at(p))["modules"]) names.push_back(m["name"].get<std::string>());
                        return names;
                    };
                    NodeLibraryEnvironment::partModuleAdjusterIds = [&partJson](Part* p, int idx) {
                        std::vector<Guid> ids;
                        for (const json& g : (*partJson.at(p))["modules"][static_cast<size_t>(idx)]["ids"]) ids.push_back(Guid::Parse(g.get<std::string>()));
                        return ids;
                    };
                    craft = op["craft"];
                    NodeLibraryEnvironment::craftPartIds = [craft](MENode*, uint32_t vid) -> std::optional<std::vector<uint32_t>> {
                        const std::string k = std::to_string(vid);
                        if (!craft.contains(k)) return std::nullopt;
                        std::vector<uint32_t> ids;
                        for (const json& p : craft[k]) ids.push_back(p.get<uint32_t>());
                        return ids;
                    };
                    reqV->clear();
                    reqP->clear();
                    for (const json& d : op["reqVessel"]) reqV->push_back(d.get<double>());
                    for (const json& d : op["reqPart"]) reqP->push_back(d.get<double>());
                    *requests = json::array();
                    NodeLibraryEnvironment::vesselRequestResource = [reqV, requests](Vessel*, Part*, int, double demand, bool) {
                        requests->push_back(json::array({"vessel", demand}));
                        if (reqV->empty()) return 0.0;
                        const double r = reqV->front();
                        reqV->pop_front();
                        return r;
                    };
                    NodeLibraryEnvironment::partRequestResource = [reqP, requests](Part*, int, double demand) {
                        requests->push_back(json::array({"part", demand}));
                        if (reqP->empty()) return 0.0;
                        const double r = reqP->front();
                        reqP->pop_front();
                        return r;
                    };
                    events.clear();
                    events.push_back(std::make_unique<ResEvents>());
                } else if (o == "validate") {
                    Editor::MissionEditorValidator validator;
                    validator.EnsureResultsForTest();
                    mod->RunValidation(&validator);
                    json got = json::array();
                    for (const auto& r : *validator.Results()) got.push_back(json::array({r.status == Editor::ValidationStatus::Fail ? "fail" : "warn", r.message}));
                    CHECK(got == op["out"]);
                } else if (o == "fire") {
                    NodeLibraryEnvironment::adjusterCalls.clear();
                    if (!events.empty()) events.back()->events = json::array();
                    int yields = 0;
                    json waits;
                    const bool threw = RunCo(mod->Fire(), yields, waits);
                    const json& out = op["out"];
                    CHECK(threw == out.value("throws", false));
                    CHECK(yields == out["yields"].get<int>());
                    CHECK(waits == out["waits"]);
                    if (out.contains("calls")) CHECK(json(NodeLibraryEnvironment::adjusterCalls) == out["calls"]);
                    CHECK(ProtoSnapshot(*scene.world) == op["proto"]);
                    if (name == "ActionPartResourceDrain") {
                        auto* d = dynamic_cast<Actions::ActionPartResourceDrain*>(mod);
                        CHECK(*requests == out["requests"]);
                        CHECK(events.back()->events == out["events"]);
                        CHECK(d->LeftToDrain() == out["left"].get<float>());
                        CHECK(mod->isRunning == out["running"].get<bool>());
                    }
                } else if (o == "supported") {
                    auto* f = dynamic_cast<Actions::ActionPartFailure*>(mod);
                    REQUIRE(f != nullptr);
                    MockSelector sel;
                    MockModuleList list;
                    if (!op["partPid"].is_null()) {
                        for (auto& p : scene.world->parts)
                            if (p->fields.persistentId == op["partPid"].get<uint32_t>()) sel.part = p.get();
                        REQUIRE(sel.part != nullptr);
                    }
                    f->VesselPartSelectorControlCreated(&sel);
                    f->DynamicModuleListControlCreated(&list);
                    std::vector<std::string> got = f->Modules->GetSupportedTypes();
                    std::sort(got.begin(), got.end());
                    CHECK(json(got) == op["out"]);
                    f->partSelectorRef = nullptr;
                    f->dynamicModuleListRef = nullptr;
                } else if (o == "call") {
                    const std::string fn = op["fn"].get<std::string>();
                    const json& a = op["a"];
                    if (fn == "OnVesselPersistentIdChanged") mod->OnVesselPersistentIdChanged(a[0].get<uint32_t>(), a[1].get<uint32_t>());
                    else if (fn == "OnPartPersistentIdChanged") mod->OnPartPersistentIdChanged(a[0].get<uint32_t>(), a[1].get<uint32_t>(), a[2].get<uint32_t>());
                    else FAIL("unknown call " << fn);
                } else if (o == "dropRepair" || o == "dropFailure") {
                    auto* r = dynamic_cast<Actions::ActionPartRepair*>(mod);
                    REQUIRE(r != nullptr);
                    const auto items = o == "dropRepair" ? r->SetRepairTypeDropdown() : r->SetFailureNamesForDropdown();
                    json got = json::array();
                    for (const auto& it : items) got.push_back(json::array({it.key, it.value, it.text}));
                    CHECK(got == op["out"]);
                } else if (o == "setRepair") {
                    auto* r = dynamic_cast<Actions::ActionPartRepair*>(mod);
                    REQUIRE(r != nullptr);
                    r->repairType = dotnet::EnumParse<Actions::ActionPartRepair::RepairChoices>(op["value"].get<std::string>());
                } else if (o == "reload") {
                    ConfigNode cfg2 = CfgFromJson(c["cfg"]);
                    mod = dynamic_cast<ActionModule*>(scene.subject->AddAction(name, &cfg2));
                    REQUIRE(mod != nullptr);
                } else {
                    FAIL("unknown op " << o);
                }
            } catch (const std::exception& e) {
                FAIL_CHECK("exception in " << name << " op " << o << ": " << e.what());
            }
        }
        NodeLibraryEnvironment::Reset();
        MissionsEnvironment::editorMission = nullptr;
        Runtime::MissionSystem::missions.clear();
    }
    CHECK(counts.size() == 3);
    CHECK(checked > 8000);
}

// ---- Mission craft-file bookkeeping (RebuildCraftFileList, lookups, vessel / part event handlers) and ProcessMappedVesselNodeMap ------------------------------------------

namespace {

ConfigNode::NodePtr NodeFromJson(const json& j) {
    auto n = std::make_shared<ConfigNode>(j["name"].get<std::string>());
    for (const auto& kv : j["values"]) n->AddValue(kv[0].get<std::string>(), kv[1].get<std::string>());
    for (const auto& c : j["nodes"]) AddCfgJson(n.get(), c);
    return n;
}

// the in-memory file system of a case: paths are the symbolic "SHIPS/..." names of the reference
struct MemFs {
    std::map<std::string, ConfigNode::NodePtr> files;
    std::set<std::string> dirs;
    std::string ships;  // Mission::ShipsPath() (the prefix the seams translate)
    std::string Sym(const std::string& p) const { return p.rfind(ships, 0) == 0 ? "SHIPS/" + p.substr(ships.size()) : p; }
};

json CraftSnapshot(Mission& m, MemFs& fs, uint32_t uid, int refresh, bool keys) {
    json craft = json::array();
    for (int i = 0; i < m.craftFileList.Count(); ++i) {
        const MissionCraft& c = *m.craftFileList.At(i);
        craft.push_back(json::array({m.craftFileList.KeyAt(i), c.craftFile, fs.Sym(c.craftFolder), c.facility, c.mappedItem, c.mappedVesselName, c.craftPersistentId, c.rootPartPersistentId}));
    }
    auto dumpMapped = [](const MissionMappedVessel& v) {
        return json::array({v.partPersistentId, v.originalVesselPersistentId, v.currentVesselPersistentId, v.partVesselName, v.craftFileName, v.situationVesselName, v.mappedVesselPersistentId});
    };
    json mapped = json::array();
    for (const auto& v : m.mappedVessels) mapped.push_back(dumpMapped(v));
    json files = json::object();
    for (const auto& kv : fs.files) files[kv.first] = Dump(*kv.second);
    json vessels = json::array();
    for (VesselSituation* v : m.GetAllVesselSituations()) vessels.push_back(json::array({v->persistentId, v->craftFile, v->vesselName}));
    json out = {{"craft", craft}, {"mapped", mapped}, {"fs", files}, {"uid", uid}, {"refresh", refresh}, {"vessels", vessels}};
    if (keys) {
        // mappedVesselKeys is private: compared through the ClearMappedVessels result only (the entries that survive); the reference dumps the keys too
        // (see the "keys" check below, which reads them through the friend-free accessor)
    }
    return out;
}

}  // namespace

TEST_CASE("WP-54c reference: Mission craft-file bookkeeping (RebuildCraftFileList, MissionCraft ids, lookups, vessel / part event handlers, ProcessMappedVesselNodeMap)") {
    Fixture54b world;
    MissionsEnvironment::craftFileBookkeeping = true;
    size_t checked = 0, craftCases = 0, nodeMaps = 0;
    for (const json& c : Ref3()["cases"]) {
        const std::string kind = c["kind"].get<std::string>();
        if (kind == "craft") {
            ++craftCases;
            const json& spec = c["spec"];
            MemFs fs;
            auto uid = std::make_shared<uint32_t>(spec["uid"].get<uint32_t>());
            auto refresh = std::make_shared<int>(0);
            std::vector<std::unique_ptr<Vessel>> vessels;  // declared last among the case's fixtures that own Vessels: destroyed first
            std::vector<std::unique_ptr<Part>> parts;
            auto mission = std::make_unique<Mission>();
            fs.ships = mission->ShipsPath();
            for (const auto& kv : spec["files"].items()) fs.files[kv.key()] = NodeFromJson(kv.value());
            for (const json& d : spec["dirs"]) fs.dirs.insert(d.get<std::string>());
            MemFs* fsp = &fs;
            MissionDataEnvironment::directoryExists = [fsp](const std::string& p) { return fsp->dirs.count(fsp->Sym(p)) != 0; };
            MissionDataEnvironment::getFiles = [fsp](const std::string& p, const std::string&) {
                std::vector<std::string> out;
                const std::string sym = fsp->Sym(p);
                for (const auto& kv : fsp->files) {
                    if (kv.first.rfind(sym, 0) == 0 && kv.first.find('/', sym.size()) == std::string::npos && kv.first.size() > 6 && kv.first.substr(kv.first.size() - 6) == ".craft")
                        out.push_back(p + kv.first.substr(sym.size()));
                }
                return out;
            };
            MissionDataEnvironment::fileExists = [fsp](const std::string& p) { return fsp->files.count(fsp->Sym(p)) != 0; };
            MissionDataEnvironment::loadConfigNode = [fsp](const std::string& p) -> ConfigNode::NodePtr {
                auto it = fsp->files.find(fsp->Sym(p));
                return it == fsp->files.end() ? nullptr : it->second->CreateCopy();
            };
            MissionDataEnvironment::saveConfigNode = [fsp](const ConfigNode& n, const std::string& p) { fsp->files[fsp->Sym(p)] = n.CreateCopy(); };
            MissionMappedVessel::getUniquePersistentId = [uid] { return (*uid)++; };
            MissionsEnvironment::refreshMissionRequested = [refresh](Mission&, bool) { ++*refresh; };
            int k = 0;
            for (const json& vj : spec["vessels"]) {
                auto* a = dynamic_cast<Actions::ActionCreateVessel*>(SpawnActionNode(*mission, k++)->AddAction("ActionCreateVessel", nullptr));
                REQUIRE(a != nullptr);
                a->vesselSituation->persistentId = vj["pid"].get<uint32_t>();
                a->vesselSituation->vesselName = vj["name"].get<std::string>();
                a->vesselSituation->craftFile = vj["craft"].get<std::string>();
            }
            for (const json& mj : spec["mapped"]) {
                MissionMappedVessel mv(mj["part"].get<uint32_t>(), mj["orig"].get<uint32_t>(), mj["cur"].get<uint32_t>(), mj["pvn"].get<std::string>(), mj["craft"].get<std::string>(),
                                       mj["sit"].get<std::string>());
                mv.mappedVesselPersistentId = mj["mapped"].get<uint32_t>();
                mission->mappedVessels.push_back(mv);
            }
            *uid = spec["uid"].get<uint32_t>();  // the VesselSituation / MissionMappedVessel constructors above consumed ids: the reference starts counting here
            auto vesselOf = [&](uint32_t pid) -> Vessel* {
                for (auto& v : vessels)
                    if (v->persistentId == pid) return v.get();
                vessels.push_back(std::make_unique<Vessel>());
                vessels.back()->persistentId = pid;
                return vessels.back().get();
            };
            for (const json& op : c["ops"]) {
                const std::string o = op["op"].get<std::string>();
                INFO("craft case op ", o);
                ++checked;
                try {
                    if (o == "rebuild") {
                        if (op["out"].is_string()) {
                            CHECK_THROWS(mission->RebuildCraftFileList());
                            break;
                        }
                        mission->RebuildCraftFileList();
                        const json got = CraftSnapshot(*mission, fs, *uid, *refresh, false);
                        json want = op["out"];
                        want.erase("keys");
                        CHECK(got == want);
                    } else if (o == "craftByVessel") {
                        MissionCraft* mc = mission->GetCraftBySituationsVesselID(op["pid"].get<uint32_t>());
                        CHECK((mc == nullptr ? json(nullptr) : json(mc->craftFile)) == op["out"]);
                    } else if (o == "craftByFile") {
                        MissionCraft* mc = mission->GetMissionCraftByFileName(op["name"].get<std::string>());
                        CHECK((mc == nullptr ? json(nullptr) : json::array({mc->craftFile, fs.Sym(mc->craftFolder), mc->mappedVesselName, mc->mappedItem})) == op["out"]);
                    } else if (o == "sitByCraft") {
                        VesselSituation* s = mission->GetMissionSituationByCraftFileName(op["name"].get<std::string>());
                        CHECK((s == nullptr ? json(nullptr) : json(s->persistentId)) == op["out"]);
                    } else if (o == "craftByName") {
                        MissionCraft* mc = mission->GetMissionCraftByName(op["name"].get<std::string>());
                        CHECK((mc == nullptr ? json(nullptr) : json(mc->craftFile)) == op["out"]);
                    } else if (o == "sitIndexByVessel") {
                        CHECK(mission->GetSituationsIndexByVessel(op["pid"].get<uint32_t>()) == op["out"].get<int>());
                    } else if (o == "sitIndexByPart") {
                        if (op["out"].is_string()) CHECK_THROWS(mission->GetSituationsIndexByPart(op["pid"].get<uint32_t>()));
                        else CHECK(mission->GetSituationsIndexByPart(op["pid"].get<uint32_t>()) == op["out"].get<int>());
                    } else if (o == "craftIndexByPart") {
                        CHECK(mission->GetCraftFileIndexByPart(op["pid"].get<uint32_t>()) == op["out"].get<int>());
                    } else if (o == "addFile") {
                        fs.files[op["path"].get<std::string>()] = NodeFromJson(op["cfg"]);
                        fs.dirs.insert("SHIPS/VAB/");
                    } else if (o == "evNaming") {
                        auto part = std::make_unique<Part>();
                        part->fields.persistentId = op["part"].get<uint32_t>();
                        part->vesselNaming = std::make_shared<VesselNaming>();
                        part->vesselNaming->vesselName = op["name"].get<std::string>();
                        part->vessel = op["vessel"].is_null() ? nullptr : vesselOf(op["vessel"].get<uint32_t>());
                        GameEvents::onPartVesselNamingChanged.Fire(part.get());
                        part->vessel = nullptr;
                        parts.push_back(std::move(part));
                        CHECK(CraftSnapshot(*mission, fs, *uid, *refresh, false) == [&] { json w = op["out"]; w.erase("keys"); return w; }());
                    } else if (o == "evRename") {
                        Vessel* v = vesselOf(op["vessel"].get<uint32_t>());
                        GameEvents::onVesselRename.Fire(GameEvents::HostedFromToAction<Vessel*, std::string>(v, "old", op["to"].get<std::string>()));
                        CHECK(CraftSnapshot(*mission, fs, *uid, *refresh, false) == op["out"]);
                    } else if (o == "evDecouple" || o == "evUndocking") {
                        Vessel* oldV = vesselOf(op["old"].get<uint32_t>());
                        Vessel* newV = vesselOf(op["new"].get<uint32_t>());
                        newV->parts.clear();
                        std::vector<std::unique_ptr<Part>> mine;
                        for (const json& pp : op["parts"]) {
                            mine.push_back(std::make_unique<Part>());
                            mine.back()->fields.persistentId = pp.get<uint32_t>();
                            newV->parts.push_back(mine.back().get());
                        }
                        if (o == "evDecouple") GameEvents::onPartDeCoupleNewVesselComplete.Fire(oldV, newV);
                        else GameEvents::onVesselsUndocking.Fire(oldV, newV);
                        newV->parts.clear();
                        CHECK(CraftSnapshot(*mission, fs, *uid, *refresh, false) == op["out"]);
                    } else if (o == "evVesselId") {
                        GameEvents::onVesselPersistentIdChanged.Fire(op["old"].get<uint32_t>(), op["new"].get<uint32_t>());
                        CHECK(CraftSnapshot(*mission, fs, *uid, *refresh, false) == op["out"]);
                    } else if (o == "evDocking") {
                        GameEvents::onVesselDocking.Fire(op["old"].get<uint32_t>(), op["new"].get<uint32_t>());
                        CHECK(CraftSnapshot(*mission, fs, *uid, *refresh, false) == op["out"]);
                    } else if (o == "evPartId") {
                        GameEvents::onPartPersistentIdChanged.Fire(op["vessel"].get<uint32_t>(), op["old"].get<uint32_t>(), op["new"].get<uint32_t>());
                        CHECK(CraftSnapshot(*mission, fs, *uid, *refresh, false) == op["out"]);
                    } else {
                        FAIL("unknown op " << o);
                    }
                } catch (const std::exception& e) {
                    FAIL_CHECK("exception in craft op " << o << ": " << e.what());
                }
            }
            MissionDataEnvironment::Reset();
            MissionMappedVessel::getUniquePersistentId = nullptr;
            MissionsEnvironment::refreshMissionRequested = nullptr;
            mission.reset();
            vessels.clear();
        } else if (kind == "nodemap") {
            ++nodeMaps;
            auto mission = std::make_unique<Mission>();
            for (const json& mj : c["mapped"]) {
                MissionMappedVessel mv(mj["part"].get<uint32_t>(), mj["orig"].get<uint32_t>(), mj["cur"].get<uint32_t>(), mj["pvn"].get<std::string>(), mj["craft"].get<std::string>(),
                                       mj["sit"].get<std::string>());
                mv.mappedVesselPersistentId = mj["mapped"].get<uint32_t>();
                mission->mappedVessels.push_back(mv);
            }
            std::vector<std::function<uint32_t&()>> fields;  // one id field per module entry, in the order of the reference
            int k = 0;
            for (const json& mj : c["mods"]) {
                MENode* n = SpawnActionNode(*mission, k++);
                const std::string kindStr = mj["kind"].get<std::string>();
                const uint32_t value = mj["value"].get<uint32_t>();
                const bool isTest = kindStr.rfind("test:", 0) == 0;
                std::string rest = kindStr.substr(isTest ? 5 : 7);
                std::string sub;
                if (const size_t colon = rest.find(':'); colon != std::string::npos) {
                    sub = rest.substr(colon + 1);
                    rest = rest.substr(0, colon);
                }
                if (isTest) {
                    TestGroup* tg = new TestGroup(n);
                    n->testGroups.emplace_back(tg);
                    tg->AddTestModule(rest, nullptr);
                    ITestModule* tm = tg->testModules.back().get();
                    if (auto* d = dynamic_cast<Tests::TestDistance*>(tm)) {
                        d->distanceToTarget = Tests::TestDistance::DistanceToChoices::Vessel;
                        d->distanceFromTarget = Tests::TestDistance::DistanceFromChoices::Vessel;
                        uint32_t& f = sub == "to" ? d->distanceToVesselID : d->distanceFromVesselID;
                        f = value;
                        fields.push_back([&f]() -> uint32_t& { return f; });
                    } else if (auto* pd = dynamic_cast<Tests::TestPartDocking*>(tm)) {
                        uint32_t& f = sub == "one" ? pd->partOnevesselPartIDs->VesselID : pd->partTwovesselPartIDs->VesselID;
                        f = value;
                        fields.push_back([&f]() -> uint32_t& { return f; });
                    } else if (auto* ca = dynamic_cast<Tests::TestCrewAssignment*>(tm)) {
                        uint32_t& f = ca->vesselPartIDs->VesselID;
                        f = value;
                        fields.push_back([&f]() -> uint32_t& { return f; });
                    } else if (auto* tv = dynamic_cast<Tests::TestVessel*>(tm)) {
                        uint32_t& f = tv->vesselID;
                        f = value;
                        fields.push_back([&f]() -> uint32_t& { return f; });
                    } else {
                        FAIL("unexpected test " << rest);
                    }
                } else {
                    IActionModule* am = n->AddAction(rest, nullptr);
                    REQUIRE(am != nullptr);
                    uint32_t* f = nullptr;
                    if (auto* a1 = dynamic_cast<Actions::ActionPartExplode*>(am)) f = &a1->vesselPartIDs.VesselID;
                    else if (auto* a2 = dynamic_cast<Actions::ActionPartFailure*>(am)) f = &a2->vesselPartIDs.VesselID;
                    else if (auto* a3 = dynamic_cast<Actions::ActionPartRepair*>(am)) f = &a3->vesselPartIDs.VesselID;
                    else if (auto* a4 = dynamic_cast<Actions::ActionPartResourceDrain*>(am)) f = &a4->vesselPartIDs.VesselID;
                    REQUIRE(f != nullptr);
                    *f = value;
                    fields.push_back([f]() -> uint32_t& { return *f; });
                }
            }
            auto read = [&] {
                json v = json::array();
                for (auto& f : fields) v.push_back(f());
                return v;
            };
            for (const json& op : c["ops"]) {
                const std::string o = op["op"].get<std::string>();
                ++checked;
                if (o == "process") {
                    mission->ProcessMappedVesselNodeMap(op["mapped"].get<uint32_t>(), op["new"].get<uint32_t>());
                    CHECK(read() == op["out"]);
                } else {
                    mission->RemapVesselId(op["cur"].get<uint32_t>());
                    CHECK(read() == op["out"]);
                    json mapped = json::array();
                    for (const auto& v : mission->mappedVessels)
                        mapped.push_back(json::array({v.partPersistentId, v.originalVesselPersistentId, v.currentVesselPersistentId, v.partVesselName, v.craftFileName, v.situationVesselName,
                                                      v.mappedVesselPersistentId}));
                    CHECK(mapped == op["mapped"]);
                }
            }
        }
    }
    MissionsEnvironment::craftFileBookkeeping = false;
    CHECK(craftCases >= 100);
    CHECK(nodeMaps >= 50);
    CHECK(checked > 2000);
}

// ---- MissionScoreInfo, MissionLocalizer, MissionImporting, MissionEditorHistory --------------------------------------------------------------------------------

namespace {

json ScoreView(const MissionScoreInfo::ScoreList& l) {
    json out = json::array();
    for (const auto& s : l)
        out.push_back(json::array({s->id.ToString(), s->missionId.ToString(), s->score, s->timeTaken, s->awardIds, s->completedAt.has_value() ? json(*s->completedAt) : json(nullptr)}));
    return out;
}
std::string DumpScores(const MissionScoreInfo& m) {
    ConfigNode n("N");
    m.Save(n);
    return Dump(n);
}
std::string RStrip(std::string s) {
    while (!s.empty() && s.back() == '/') s.pop_back();
    return s;
}

struct ImportedCounter {
    int n = 0;
    void On() { ++n; }
};

struct MockTarget : Editor::IMEHistoryTarget {
    int id = 0;
    int calls = 0;
    ConfigNode::NodePtr GetState() override {
        auto n = std::make_shared<ConfigNode>("S");
        n->AddValue("v", ++calls);
        return n;
    }
};

}  // namespace

TEST_CASE("WP-54c reference: MissionScoreInfo (AddScore, sort, delete, queries, Load / Save, score file)") {
    Fixture54b world;
    size_t checked = 0;
    for (const json& c : Ref3()["cases"]) {
        if (c["kind"].get<std::string>() != "scoreinfo") continue;
        uint64_t gcounter = c["spec"]["guid0"].get<uint64_t>();
        MissionsEnvironment::newGuid = [&gcounter] { return Guid::Parse(GuidOf(static_cast<int>(gcounter++))); };
        double ut = 0.0;
        MissionsEnvironment::gameUniversalTime = [&ut] { return ut; };
        int64_t now = 0;
        MissionExtrasEnvironment::dateTimeNowFileTimeUtc = [&now] { return now; };
        std::map<std::string, std::unique_ptr<Mission>> missions;
        auto missionOf = [&](const std::string& g) {
            auto it = missions.find(g);
            if (it == missions.end()) {
                auto m = std::make_unique<Mission>();
                m->SetIdForTests(Guid::Parse(g));
                it = missions.emplace(g, std::move(m)).first;
            }
            return it->second.get();
        };
        for (int k = 10; k <= 12; ++k) missionOf(GuidOf(k));  // the Missions consume Guid.NewGuid() in their constructors: create them first, then pin the counter
        const uint64_t g0 = c["spec"]["guid0"].get<uint64_t>();
        gcounter = g0;
        auto info = std::make_unique<MissionScoreInfo>();
        std::string savedRoot;
        std::string savedPath;
        MissionExtrasEnvironment::saveConfigNode = [&](const ConfigNode& n, const std::string& p) {
            savedRoot = Dump(n);
            savedPath = p;
        };
        ConfigNode::NodePtr fileNode;
        MissionExtrasEnvironment::loadConfigNode = [&](const std::string&) -> ConfigNode::NodePtr { return fileNode; };
        for (const json& op : c["ops"]) {
            const std::string o = op["op"].get<std::string>();
            INFO("scoreinfo op ", o);
            ++checked;
            if (o == "load") {
                ConfigNode cfg = CfgFromJson(op["cfg"]);
                info->Load(cfg);
            } else if (o == "save") {
                CHECK(DumpScores(*info) == op["out"].get<std::string>());
                CHECK(gcounter == op["guid"].get<uint64_t>());
            } else if (o == "add") {
                Mission* m = missionOf(op["mission"].get<std::string>());
                m->currentScore = op["score"].get<float>();
                m->isScoreEnabled = op["enabled"].get<bool>();
                m->awards->awardedAwards = op["awards"].get<std::vector<std::string>>();
                ut = op["ut"].get<double>();
                now = op["now"].get<int64_t>();
                const uint64_t before = gcounter;
                gcounter = op["guid"].get<uint64_t>() - (op["enabled"].get<bool>() ? 1 : 0);  // the reference counter before this add
                (void)before;
                info->AddScore(*m);
                CHECK(gcounter == op["guid"].get<uint64_t>());
                CHECK(ScoreView(*info->GetMissionScores(m)) == op["scores"]);
            } else if (o == "query") {
                Mission* m = missionOf(op["mission"].get<std::string>());
                CHECK(ScoreView(*info->GetMissionScores(m)) == op["scores"]);
                auto s0 = info->GetMissionScore(m, 0);
                auto s1 = info->GetMissionScore(m, 1);
                auto s5 = info->GetMissionScore(m, 5);
                CHECK((s0 ? json(s0->id.ToString()) : json(nullptr)) == op["score0"]);
                CHECK((s1 ? json(s1->id.ToString()) : json(nullptr)) == op["score1"]);
                CHECK((s5 ? json(s5->id.ToString()) : json(nullptr)) == op["score5"]);
                CHECK(json(info->GetMissionAwards(m)) == op["awards"]);
            } else if (o == "delete") {
                info->DeleteScore(Guid::Parse(op["id"].get<std::string>()));
                CHECK(DumpScores(*info) == op["dump"].get<std::string>());
            } else if (o == "deleteMission") {
                info->DeleteMissionScores(missionOf(op["mission"].get<std::string>()));
                CHECK(DumpScores(*info) == op["dump"].get<std::string>());
            } else if (o == "saveFile") {
                info->SaveScores();
                CHECK(savedRoot == op["out"].get<std::string>());
                CHECK(savedPath == op["path"].get<std::string>());
                ConfigNode::NodePtr root = std::make_shared<ConfigNode>("");
                // the file the C# would read back: the saved node
                ConfigNode* ms = root->AddNode("MISSIONSCORES");
                info->Save(*ms);
                fileNode = root;
            } else if (o == "loadFile") {
                gcounter = op["guid"].get<uint64_t>() - fileNode->GetNode("MISSIONSCORES")->GetNodes("SCOREINFO").size();
                std::unique_ptr<MissionScoreInfo> m2 = MissionScoreInfo::LoadScores();
                CHECK(gcounter == op["guid"].get<uint64_t>());
                CHECK(DumpScores(*m2) == op["dump"].get<std::string>());
                info = std::move(m2);
            } else if (o == "nullMission") {
                CHECK(info->GetMissionScores(nullptr)->empty());
                CHECK(info->GetMissionScore(nullptr, 0) == nullptr);
                CHECK(info->GetMissionAwards(nullptr).empty());
                info->DeleteMissionScores(nullptr);
            } else {
                FAIL("unknown op " << o);
            }
        }
        MissionsEnvironment::newGuid = nullptr;
        MissionsEnvironment::gameUniversalTime = nullptr;
        MissionExtrasEnvironment::Reset();
    }
    CHECK(checked > 500);
}

TEST_CASE("WP-54c reference: MissionLocalizer.LoadMissionLocalizationFiles") {
    Fixture54b world;
    size_t cases = 0;
    for (const json& c : Ref3()["cases"]) {
        if (c["kind"].get<std::string>() != "localizer") continue;
        ++cases;
        const json& spec = c["spec"];
        MissionsUtils::Paths::applicationRootPath = spec["root"].get<std::string>();
        std::set<std::string> dirs;
        std::map<std::string, ConfigNode::NodePtr> files;
        for (const json& d : spec["dirs"]) dirs.insert(d.get<std::string>());
        if (!spec["missionsExists"].get<bool>()) dirs.erase("ROOT/Missions");
        for (const auto& kv : spec["files"].items()) files[kv.key()] = NodeFromJson(kv.value());
        MissionExtrasEnvironment::directoryExists = [&](const std::string& p) { return dirs.count(RStrip(p)) != 0; };
        MissionExtrasEnvironment::getDirectories = [&](const std::string& dir) {
            std::vector<std::string> out;
            for (const auto& d : dirs)
                if (d.rfind(dir, 0) == 0 && d.find('/', dir.size()) == std::string::npos) out.push_back(d);
            return out;
        };
        MissionExtrasEnvironment::getFiles = [&](const std::string& dir, const std::string& ext) {
            std::vector<std::string> out;
            for (const auto& kv : files)
                if (kv.first.rfind(dir, 0) == 0 && kv.first.find('/', dir.size()) == std::string::npos && kv.first.size() > ext.size() &&
                    kv.first.compare(kv.first.size() - ext.size(), ext.size(), ext) == 0)
                    out.push_back(kv.first);
            return out;
        };
        MissionExtrasEnvironment::loadConfigNode = [&](const std::string& p) -> ConfigNode::NodePtr {
            auto it = files.find(p);
            return it == files.end() ? nullptr : it->second->CreateCopy();
        };
        const std::vector<ConfigNode::NodePtr> got = MissionLocalizer::LoadMissionLocalizationFiles();
        json dumps = json::array();
        for (const auto& n : got) dumps.push_back(Dump(*n));
        CHECK(dumps == c["out"]);
        MissionExtrasEnvironment::Reset();
        MissionsUtils::Paths::applicationRootPath = "";
    }
    CHECK(cases >= 40);
}

TEST_CASE("WP-54c reference: MissionImporting (zip import, overwrite popups, move / rename of the imported archive)") {
    Fixture54b world;
    size_t cases = 0, popups = 0;
    for (const json& c : Ref3()["cases"]) {
        if (c["kind"].get<std::string>() != "importing") continue;
        ++cases;
        MissionsUtils::Paths::applicationRootPath = "ROOT/";
        const json& w = c["world"];
        std::set<std::string> dirs(w["dirs"].begin(), w["dirs"].end());
        std::set<std::string> files(w["files"].begin(), w["files"].end());
        std::set<std::string> moveFails(w["moveFails"].begin(), w["moveFails"].end());
        const json& zips = w["zips"];
        int fires = 0;
        std::vector<std::string> parses;
        MissionExtrasEnvironment::directoryExists = [&](const std::string& p) { return dirs.count(RStrip(p)) != 0; };
        MissionExtrasEnvironment::createDirectory = [&](const std::string& p) { dirs.insert(RStrip(p)); };
        MissionExtrasEnvironment::deleteDirectory = [&](const std::string& p0) {
            const std::string p = RStrip(p0);
            for (auto it = dirs.begin(); it != dirs.end();) it = (*it == p || it->rfind(p + "/", 0) == 0) ? dirs.erase(it) : std::next(it);
            for (auto it = files.begin(); it != files.end();) it = it->rfind(p + "/", 0) == 0 ? files.erase(it) : std::next(it);
        };
        MissionExtrasEnvironment::getFiles = [&](const std::string& dir, const std::string& ext) {
            std::vector<std::string> out;
            for (const auto& f : files)
                if (f.rfind(dir, 0) == 0 && f.find('/', dir.size()) == std::string::npos && f.size() > ext.size() && f.compare(f.size() - ext.size(), ext.size(), ext) == 0) out.push_back(f);
            return out;
        };
        MissionExtrasEnvironment::fileExists = [&](const std::string& p) { return files.count(p) != 0; };
        MissionExtrasEnvironment::moveFile = [&](const std::string& src, const std::string& dst, bool overwrite) {
            if (moveFails.count(src) || !files.count(src)) return false;
            if (overwrite && files.count(dst)) files.erase(dst);
            files.erase(src);
            files.insert(dst);
            return true;
        };
        MissionExtrasEnvironment::dateTimeStamp = [] { return std::string("2026_10_12_09_30_00"); };
        MissionExtrasEnvironment::zipFilesAtRoot = [&](const std::string& z) {
            if (zips[z].value("throwRoot", false)) throw std::runtime_error("zip");
            return zips[z]["rootFiles"].get<bool>();
        };
        MissionExtrasEnvironment::zipTopLevelDirectories = [&](const std::string& z) {
            if (zips[z].value("throwDirs", false)) throw std::runtime_error("zip");
            return zips[z]["dirs"].get<std::vector<std::string>>();
        };
        MissionExtrasEnvironment::zipDecompress = [&](const std::string& z, const std::string& dest) {
            if (zips[z].value("throwDecompress", false)) throw std::runtime_error("zip");
            for (const json& d : zips[z]["creates"]) dirs.insert(dest + d.get<std::string>());
        };
        MissionExtrasEnvironment::parseMissionPacksCFG = [&](const std::string& p) { parses.push_back(p); };
        ImportedCounter imported;
        GameEvents::Mission::onMissionImported.Add(&imported, &ImportedCounter::On);
        MissionImporting::ImportNewZips();
        // the host answers the popups in order; a click may spawn another popup
        json popupLog = json::array();
        const json& answers = c["answers"];
        size_t ai = 0;
        while (!MissionExtrasEnvironment::popups.empty()) {
            MissionExtrasEnvironment::PopupDialog p = std::move(MissionExtrasEnvironment::popups.front());
            MissionExtrasEnvironment::popups.pop_front();
            json labels = json::array();
            for (const auto& b : p.buttons) labels.push_back(b.label);
            popupLog.push_back(json::array({p.name, p.message, p.title, labels}));
            const json& a = answers[ai % answers.size()];
            ++ai;
            if (!a.is_null()) p.buttons[a.get<size_t>()].callback();
        }
        ++popups;
        const json& out = c["out"];
        CHECK(json(std::vector<std::string>(files.begin(), files.end())) == out["files"]);
        CHECK(json(std::vector<std::string>(dirs.begin(), dirs.end())) == out["dirs"]);
        CHECK(imported.n == out["fires"].get<int>());
        (void)fires;
        CHECK(json(parses) == out["parses"]);
        CHECK(popupLog == out["popups"]);
        json msgs = json::array();
        for (const auto& m : NodeLibraryEnvironment::screenMessages) msgs.push_back(json::array({m.text, m.duration, dotnet::EnumToString(m.style)}));
        CHECK(msgs == out["msgs"]);
        NodeLibraryEnvironment::Reset();
        MissionExtrasEnvironment::Reset();
        GameEvents::Mission::onMissionImported.Remove(&imported, &ImportedCounter::On);
        MissionsUtils::Paths::applicationRootPath = "";
    }
    CHECK(cases >= 100);
}

TEST_CASE("WP-54c reference: MissionEditorHistory (undo / redo stacks, frame grouping, state ids, destroy-on-clear, exception flush)") {
    Fixture54b world;
    size_t checked = 0, cases = 0;
    for (const json& c : Ref3()["cases"]) {
        if (c["kind"].get<std::string>() != "history") continue;
        ++cases;
        uint64_t gcounter = c["ops"][0]["guid0"].get<uint64_t>();
        auto mission = std::make_unique<Mission>();
        MissionsEnvironment::newGuid = [&gcounter] { return Guid::Parse(GuidOf(static_cast<int>(gcounter++))); };
        MissionsEnvironment::editorMission = [m = mission.get()]() -> Mission* { return m; };
        int frame = 0;
        Editor::MissionEditorHistory::Environment::frameCount = [&frame] { return frame; };
        std::map<int, MockTarget> targets;
        auto history = std::make_unique<Editor::MissionEditorHistory>();
        REQUIRE(Editor::MissionEditorHistory::Instance() == history.get());
        json callbacks = json::array();
        int throwOn = 0;
        bool editorPresent = true;
        auto target = [&](int id) {
            MockTarget& t = targets[id];
            t.id = id;
            return &t;
        };
        auto view = [&] {
            auto grp = [&](const std::vector<std::vector<Editor::MissionEditorHistory::ActionView>>& g) {
                json out = json::array();
                for (const auto& group : g) {
                    json gj = json::array();
                    for (const auto& a : group)
                        gj.push_back(json::array({static_cast<MockTarget*>(a.target)->id, Dump(*a.data), a.stateId.ToString(), a.destroyOnClear, a.destroyTargets}));
                    out.push_back(gj);
                }
                return out;
            };
            json selected = json::array();
            for (auto* t : Editor::MissionEditorHistory::Environment::selected) selected.push_back(static_cast<MockTarget*>(t)->id);
            json dt = json::array();
            for (auto* t : Editor::MissionEditorHistory::Environment::destroyedTargets) dt.push_back(static_cast<MockTarget*>(t)->id);
            return json{{"undo", grp(history->UndoView())},
                        {"redo", grp(history->RedoView())},
                        {"inProgress", history->ActionInProgress},
                        {"hasHistory", Editor::MissionEditorHistory::HasHistory()},
                        {"state", mission->historyId.ToString()},
                        {"selected", selected},
                        {"destroyed", json(Editor::MissionEditorHistory::Environment::destroyed)},
                        {"destroyedTargets", dt},
                        {"callbacks", callbacks}};
        };
        for (const json& op : c["ops"]) {
            const std::string o = op["op"].get<std::string>();
            INFO("history op ", o);
            ++checked;
            if (o == "init") {
                // Instance, the default state id (Guid.Empty)
                CHECK(mission->historyId == Guid());
            } else if (o == "push") {
                const int id = op["target"].get<int>();
                const auto cb = [id, &callbacks, &throwOn](ConfigNode& data, Editor::HistoryType t) {
                    callbacks.push_back(json::array({id, Dump(data), t == Editor::HistoryType::Undo ? "undo" : "redo"}));
                    if (throwOn == id) throw std::runtime_error("callback");
                };
                if (op["out"].is_string()) {
                    CHECK_THROWS(Editor::MissionEditorHistory::PushUndoAction(target(id), cb, op["destroyOnClear"].get<bool>(), op["destroyTargets"].get<std::vector<int>>()));
                } else {
                    Editor::MissionEditorHistory::PushUndoAction(target(id), cb, op["destroyOnClear"].get<bool>(), op["destroyTargets"].get<std::vector<int>>());
                    CHECK(view() == op["out"]);
                }
            } else if (o == "undo") {
                Editor::MissionEditorHistory::Undo();
                CHECK(view() == op["out"]);
            } else if (o == "redo") {
                Editor::MissionEditorHistory::Redo();
                CHECK(view() == op["out"]);
            } else if (o == "frame") {
                frame = op["value"].get<int>();
            } else if (o == "clear") {
                Editor::MissionEditorHistory::Clear();
                CHECK(view() == op["out"]);
            } else if (o == "mission") {
                const std::string v = op["value"].get<std::string>();
                editorPresent = v != "none";
                mission->packName = v == "tutorial" ? "squad_MakingHistory_Tutorial" : "";
                Mission* mp = mission.get();
                MissionsEnvironment::editorMission = [mp, &editorPresent]() -> Mission* { return editorPresent ? mp : nullptr; };
            } else if (o == "throwOn") {
                throwOn = op["target"].is_null() ? 0 : op["target"].get<int>();
            } else {
                FAIL("unknown op " << o);
            }
        }
        // a second instance is destroyed on Awake
        Editor::MissionEditorHistory second;
        CHECK(second.destroyedAsDuplicate());
        CHECK(Editor::MissionEditorHistory::Instance() == history.get());
        history.reset();
        CHECK(Editor::MissionEditorHistory::Instance() == nullptr);
        MissionsEnvironment::newGuid = nullptr;
        MissionsEnvironment::editorMission = nullptr;
        Editor::MissionEditorHistory::Environment::Reset();
    }
    CHECK(cases >= 70);
    CHECK(checked > 1000);
}

// ---- GetHashCode overrides -------------------------------------------------------------------------------------------------------------------------------------

TEST_CASE("WP-54c reference: GetHashCode overrides (ScoreRange, VesselPartIDPair, MissionInstructor, MissionBiome, MissionOrbit, VesselGroundLocation, SurfaceArea, SurfaceVolume, MissionKerbal, DynamicModule(List))") {
    Fixture54b world;
    size_t n = 0;
    std::set<std::string> classes;
    for (const json& c : Ref3()["cases"]) {
        if (c["kind"].get<std::string>() != "hash") continue;
        const std::string cls = c["cls"].get<std::string>();
        const json& a = c["args"];
        classes.insert(cls);
        ++n;
        INFO("hash case ", cls);
        const void* bodies[4] = {};
        for (size_t i = 0; i < world.bodies.size() && i < 4; ++i) bodies[i] = world.bodies[i].get();
        const void* rotationPtr = nullptr;
        const void* listPtr = nullptr;
        MissionDataEnvironment::objectHashCode = [&](const void* p) -> int32_t {
            for (int i = 0; i < 4; ++i)
                if (p == bodies[i]) return 1000 + i;
            if (p == rotationPtr) return 7777;
            if (p == listPtr) return 4242;
            return -1;
        };
        const int32_t want = c["out"].is_number() ? c["out"].get<int32_t>() : 0;
        if (cls == "ScoreRange") {
            ScoreRange s;
            s.minRange = a["min"].get<double>();
            s.maxRange = a["max"].get<double>();
            s.score = a["score"].get<float>();
            CHECK(s.GetHashCode() == want);
        } else if (cls == "VesselPartIDPair") {
            VesselPartIDPair p;
            p.VesselID = a["v"].get<uint32_t>();
            p.partID = a["p"].get<uint32_t>();
            CHECK(p.GetHashCode() == want);
        } else if (cls == "MissionInstructor") {
            MissionInstructor m;
            m.instructorName = a["name"].get<std::string>();
            m.vintageSuit = a["vintage"].get<bool>();
            CHECK(m.GetHashCode() == want);
        } else if (cls == "MissionBiome") {
            Editor::MissionBiome b(world.B(a["body"].get<int>()), a["name"].get<std::string>());
            CHECK(b.GetHashCode() == want);
        } else if (cls == "MissionOrbit") {
            ConfigNode cfg("ORBIT");
            for (const char* k : {"SMA", "ECC", "INC", "LPE", "LAN", "MNA", "EPH"}) cfg.AddValue(k, a[k].get<double>());
            cfg.AddValue("REF", a["REF"].get<int>());
            MissionOrbit o(cfg);
            CHECK(o.GetHashCode() == want);
        } else if (cls == "VesselGroundLocation") {
            VesselGroundLocation g;
            g.targetBody = world.B(a["body"].get<int>());
            g.altitude = a["alt"].get<double>();
            g.latitude = a["lat"].get<double>();
            g.longitude = a["lon"].get<double>();
            rotationPtr = g.rotation.get();
            CHECK(g.GetHashCode() == want);
        } else if (cls == "SurfaceArea") {
            SurfaceArea s(world.B(a["body"].get<int>()), 0.0, 0.0, a["radius"].get<float>());
            s.latitude = a["lat"].get<double>();
            s.longitude = a["lon"].get<double>();
            CHECK(s.GetHashCode() == want);
        } else if (cls == "SurfaceVolume") {
            SurfaceVolume v;
            v.body = world.B(a["body"].get<int>());
            v.latitude = a["lat"].get<double>();
            v.longitude = a["lon"].get<double>();
            v.radius = a["radius"].get<float>();
            v.shape = a["shape"].get<int>() == 0 ? SurfaceVolume::VolumeShape::Cone : SurfaceVolume::VolumeShape::Sphere;
            v.heightSphere = a["hs"].get<float>();
            v.heightConeMin = a["hmin"].get<float>();
            v.heightConeMax = a["hmax"].get<float>();
            CHECK(v.GetHashCode() == want);
        } else if (cls == "MissionKerbal") {
            if (a.is_null()) {
                MissionKerbal mk;
                CHECK_THROWS(mk.GetHashCode());
            } else {
                auto pcm = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, a["name"].get<std::string>());
                pcm->stupidity = a["stupidity"].get<float>();
                pcm->courage = a["courage"].get<float>();
                pcm->isBadass = a["bad"].get<bool>();
                pcm->veteran = a["vet"].get<bool>();
                pcm->hasToured = a["toured"].get<bool>();
                pcm->UTaR = a["utar"].get<double>();
                const ProtoCrewMember::KerbalType types[] = {ProtoCrewMember::KerbalType::Crew, ProtoCrewMember::KerbalType::Applicant, ProtoCrewMember::KerbalType::Unowned,
                                                             ProtoCrewMember::KerbalType::Tourist};
                MissionKerbal mk(pcm, nullptr, nullptr, types[a["type"].get<int>()]);
                CHECK(mk.GetHashCode() == want);
            }
        } else if (cls == "DynamicModule") {
            const DynamicModuleType* t = DynamicModuleRegistry::GetClassByName(a["name"].get<std::string>());
            REQUIRE(t != nullptr);
            std::unique_ptr<DynamicModule> m = t->create(nullptr);
            CHECK(m->GetHashCode() == want);
        } else if (cls == "DynamicModuleList") {
            DynamicModuleList l(nullptr);
            listPtr = &l.activeModules;
            CHECK(l.GetHashCode() == want);
        } else {
            FAIL("unknown class " << cls);
        }
        MissionDataEnvironment::objectHashCode = nullptr;
    }
    CHECK(classes.size() == 11);
    CHECK(n > 300);
}

// ---- BaseAPField / BaseAPFieldList (the real reflection record) ----------------------------------------------------------------------------------------------------

#include "ksp/missions/mission_apfield.hpp"

namespace {

using Editor::Boxed;

Boxed ToBoxed(const json& tv) {
    const std::string t = tv["t"].get<std::string>();
    if (t == "int") return Boxed(tv["v"].get<int>());
    if (t == "double") return Boxed(tv["v"].get<double>());
    if (t == "bool") return Boxed(tv["v"].get<bool>());
    return Boxed(tv["v"].get<std::string>());
}
json FromBoxed(const std::string& t, const Boxed& b) {
    if (!b.has_value()) return json(nullptr);
    if (t == "int") return json{{"t", t}, {"v", std::any_cast<int>(b)}};
    if (t == "double") return json{{"t", t}, {"v", std::any_cast<double>(b)}};
    if (t == "bool") return json{{"t", t}, {"v", std::any_cast<bool>(b)}};
    return json{{"t", t}, {"v", std::any_cast<std::string>(b)}};
}
std::string FmtBoxed(const Boxed& b) {
    if (b.type() == typeid(bool)) return std::any_cast<bool>(b) ? "True" : "False";
    if (b.type() == typeid(int)) return std::to_string(std::any_cast<int>(b));
    if (b.type() == typeid(double)) return dotnet::FormatDoubleG(std::any_cast<double>(b), 15);
    return std::any_cast<std::string>(b);
}
bool TypeOk(const std::string& t, const Boxed& b) {
    if (t == "int") return b.type() == typeid(int);
    if (t == "double") return b.type() == typeid(double);
    if (t == "bool") return b.type() == typeid(bool);
    return b.type() == typeid(std::string);
}

struct ScriptHost : Editor::IAPFieldHost {
    const json* methods = nullptr;
    std::vector<std::string>* log = nullptr;
    std::function<void(const Boxed&)> FindValueMethod(const std::string& name, const std::string& fieldType) override {
        if (!methods->contains(name)) return {};
        const json& m = (*methods)[name];
        if (m["kind"].get<std::string>() != "value" || m["param"].get<std::string>() != fieldType) return {};
        const bool throws = m["throws"].get<bool>();
        auto* l = log;
        return [name, throws, l](const Boxed& v) {
            l->push_back("vm:" + name + "(" + FmtBoxed(v) + ")");
            if (throws) throw std::runtime_error("onValueChange");
        };
    }
    std::function<void(void*)> FindControlMethod(const std::string& name) override {
        if (!methods->contains(name) || (*methods)[name]["kind"].get<std::string>() != "control") return {};
        auto* l = log;
        return [name, l](void*) { l->push_back("ctl:" + name); };
    }
    std::optional<Editor::IAPFieldHost::CompareMethod> FindCompareMethod(const std::string& name, const std::string& fieldType) override {
        if (!methods->contains(name)) return std::nullopt;
        const json& m = (*methods)[name];
        if (m["kind"].get<std::string>() != "compare" || m["param"].get<std::string>() != fieldType) return std::nullopt;
        const bool result = m["result"].get<bool>();
        return Editor::IAPFieldHost::CompareMethod{[result](const Boxed&) { return result; }, m["returnsBool"].get<bool>()};
    }
};

Editor::MEGUI_Control AttrOf(const json& a) {
    Editor::MEGUI_Control c;
    c.set_guiName(a["guiName"].get<std::string>());
    c.set_Tooltip(a["tooltip"].get<std::string>());
    c.gapDisplay = a["gapDisplay"].get<bool>();
    c.onValueChange = a["onValueChange"].get<std::string>();
    c.onControlCreated = a["onControlCreated"].get<std::string>();
    if (!a["onControlSetupComplete"].is_null()) c.onControlSetupComplete = a["onControlSetupComplete"].get<std::string>();
    c.group = a["group"].get<std::string>();
    if (!a["groupDisplayName"].is_null()) c.groupDisplayName = a["groupDisplayName"].get<std::string>();
    c.order = a["order"].get<int>();
    c.tabStop = a["tabStop"].get<bool>();
    c.groupStartCollapsed = a["groupStartCollapsed"].get<bool>();
    if (!a["compare"].is_null()) c.compareValuesForCheckpoint = a["compare"].get<std::string>();
    c.canBeReset = a["canBeReset"].get<bool>();
    c.canBePinned = a["canBePinned"].get<bool>();
    if (!a["resetValue"].is_null()) c.resetValue = a["resetValue"].get<std::string>();
    const std::string cp = a["checkpoint"].get<std::string>();
    c.checkpointValidation = cp == "None" ? Editor::CheckpointValidationType::None
                             : cp == "Equals" ? Editor::CheckpointValidationType::Equals
                             : cp == "CustomMethod" ? Editor::CheckpointValidationType::CustomMethod
                                                    : Editor::CheckpointValidationType::Controls;
    c.hideWhenSiblingsExist = a["hideWhenSiblingsExist"].get<bool>();
    c.hideWhenStartNode = a["hideWhenStartNode"].get<bool>();
    c.hideWhenDocked = a["hideWhenDocked"].get<bool>();
    c.hideWhenInputConnected = a["hideWhenInputConnected"].get<bool>();
    c.hideWhenOutputConnected = a["hideWhenOutputConnected"].get<bool>();
    c.hideWhenNoTestModules = a["hideWhenNoTestModules"].get<bool>();
    c.hideWhenNoActionModules = a["hideWhenNoActionModules"].get<bool>();
    c.hideOnSetup = a["hideOnSetup"].get<bool>();
    return c;
}

json PropsOf(Editor::BaseAPField& f) {
    const Editor::MEGUI_Control& a = f.Attribute();
    json hide = json::array({f.HideWhenSiblingsExist(), f.HideWhenStartNode(), f.HideWhenDocked(), f.HideWhenInputConnected(), f.HideWhenOutputConnected(), f.HideWhenNoTestModules(),
                             f.HideWhenNoActionModules(), f.HideOnSetup()});
    const char* cps[] = {"None", "Equals", "CustomMethod", "Controls"};
    return json{{"name", f.name}, {"guiName", f.guiName}, {"gap", f.gapDisplay()}, {"group", f.Group()},
                {"groupDisplayName", f.GroupDisplayName().has_value() ? json(*f.GroupDisplayName()) : json(nullptr)}, {"order", f.Order()}, {"tabStop", f.TabStop()}, {"hide", hide},
                {"groupStartCollapsed", f.GroupStartCollapsed()}, {"hasOvc", static_cast<bool>(f.OnValueChange())}, {"hasOcc", static_cast<bool>(f.OnControlCreated())},
                {"hasOcsc", static_cast<bool>(f.OnControlSetupComplete())}, {"hasCmp", static_cast<bool>(f.CompareValuesForCheckpoint())}, {"fieldID", f.FieldID()},
                {"fieldType", f.FieldTypeName()}, {"hasInterface", f.hasInterface()}, {"isProperty", f.IsProperty()}, {"original", FromBoxed(f.FieldTypeName(), f.originalValue())},
                {"canBeReset", a.canBeReset}, {"canBePinned", a.canBePinned}, {"tooltip", a.Tooltip()}, {"resetValue", a.resetValue.has_value() ? json(*a.resetValue) : json(nullptr)},
                {"checkpoint", cps[static_cast<int>(a.checkpointValidation)]}};
}

}  // namespace

TEST_CASE("WP-54c reference: BaseAPField / BaseAPFieldList (attribute copy, callback resolution, SetValue field and property paths, validation, original value)") {
    Fixture54b world;
    size_t checked = 0, cases = 0;
    for (const json& c : Ref3()["cases"]) {
        if (c["kind"].get<std::string>() != "apfield") continue;
        ++cases;
        // the host: values by member name
        std::map<std::string, Boxed> values;
        std::vector<Editor::BaseAPFieldList::ReflectedMember> fields, props;
        for (const json& m : c["members"]) {
            Editor::BaseAPFieldList::ReflectedMember rm;
            rm.attribute = AttrOf(m["attr"]);
            const std::string name = m["name"].get<std::string>();
            const std::string type = m["type"].get<std::string>();
            const bool isProp = m["kind"].get<std::string>() == "property";
            values[name] = ToBoxed({{"t", type}, {"v", m["init"]}});
            Editor::MemberInfo mi;
            mi.kind = isProp ? Editor::MemberInfo::Kind::Property : Editor::MemberInfo::Kind::Field;
            mi.name = name;
            mi.declaringType = m["declaring"].get<std::string>();
            mi.valueType = type;
            mi.valueTypeHasIConfigNode = m["iconfig"].get<bool>();
            mi.valueIsBool = type == "bool";
            const bool readonly = m.value("readonly", false);
            const json throwOn = m.value("throwOn", json::array());
            mi.get = [&values, name](void*) { return values[name]; };
            mi.set = [&values, name, type, isProp, readonly, throwOn](void*, const Boxed& v) {
                if (!TypeOk(type, v)) throw std::invalid_argument("type");
                if (isProp) {
                    if (readonly) throw std::runtime_error("readonly");
                    for (const json& t : throwOn)
                        if (FromBoxed(type, v)["v"] == t) throw std::runtime_error("setter");
                }
                values[name] = v;
            };
            mi.toString = [](const Boxed& b) { return FmtBoxed(b); };
            rm.member = mi;
            (isProp ? props : fields).push_back(rm);
        }
        // reflection order: fields (declaration order), then properties
        Editor::BaseAPFieldList::ResetReflectedTypes();
        Editor::BaseAPFieldList::RegisterReflectedType("HostX", fields, props);
        std::vector<std::string> log;
        ScriptHost host;
        host.methods = &c["methods"];
        host.log = &log;
        int validations = 0;
        MissionsEnvironment::updateValidationLed = [&validations] { ++validations; };
        std::unique_ptr<Editor::MissionEditorValidator> validator;
        Mission mission;
        const json& vs = c["validator"];
        if (!vs.is_null()) {
            validator = std::make_unique<Editor::MissionEditorValidator>();
            validator->mode = vs["mode"].get<std::string>() == "FullAuto"        ? Editor::ValidatorMode::FullAuto
                              : vs["mode"].get<std::string>() == "AutoAfterRun" ? Editor::ValidatorMode::AutoAfterRun
                                                                                : Editor::ValidatorMode::Manual;
            Editor::MissionEditorValidator::RunValidation(&mission);
            mission.hasBeenValidated = vs["validated"].get<bool>();
            validations = 0;
        }
        void* hostPtr = c["hostNull"].get<bool>() ? nullptr : static_cast<void*>(&values);
        Editor::BaseAPFieldList list("HostX", &host, hostPtr);
        std::vector<int> modified(static_cast<size_t>(c["hostNull"].get<bool>() ? 0 : list.Count()), 0);
        if (!c["hostNull"].get<bool>()) {
            for (int i = 0; i < list.Count(); ++i) list[i]->OnValueModified.push_back([&modified, i](const Boxed&) { ++modified[static_cast<size_t>(i)]; });
        }
        // the reference lists fields in member order; the list is fields first, then properties: map names to the list
        for (const json& op : c["ops"]) {
            const std::string o = op["op"].get<std::string>();
            INFO("apfield op ", o);
            ++checked;
            try {
                if (o == "countThrows") {
                    CHECK_THROWS(list.Count());
                    continue;
                }
                if (o == "props") {
                    // compare as a set keyed by member name (the reference lists them in member order)
                    std::map<std::string, json> got;
                    for (int i = 0; i < list.Count(); ++i) got[list[i]->name] = PropsOf(*list[i]);
                    for (const json& want : op["out"]) CHECK(got[want["name"].get<std::string>()] == want);
                    CHECK(got.size() == op["out"].size());
                } else if (o == "count") {
                    CHECK(list.Count() == op["out"].get<int>());
                } else if (o == "set") {
                    log.clear();
                    const int before = validations;
                    Editor::BaseAPField* f = list[op["name"].get<std::string>()];
                    REQUIRE(f != nullptr);
                    std::variant<bool, std::string> res;
                    try {
                        res = f->SetValue(ToBoxed(op["value"]));
                    } catch (const std::exception&) {
                        res = std::string("throws");
                    }
                    if (std::holds_alternative<bool>(res)) CHECK(json(std::get<bool>(res)) == op["out"]);
                    else CHECK(json(std::get<std::string>(res)) == op["out"]);
                    CHECK(json(log) == op["log"]);
                    CHECK(validations - before == op["validations"].get<int>());
                    CHECK(FromBoxed(f->FieldTypeName(), f->GetValue()) == op["after"]);
                    int idx = 0;
                    for (int i = 0; i < list.Count(); ++i)
                        if (list[i] == f) idx = i;
                    CHECK(modified[static_cast<size_t>(idx)] == op["modified"].get<int>());
                } else if (o == "listSet") {
                    Editor::BaseAPField* f = list[op["name"].get<std::string>()];
                    CHECK(list.SetValue(op["name"].get<std::string>(), ToBoxed(op["value"])) == op["out"].get<bool>());
                    CHECK(FromBoxed(f->FieldTypeName(), f->GetValue()) == op["after"]);
                    int idx = 0;
                    for (int i = 0; i < list.Count(); ++i)
                        if (list[i] == f) idx = i;
                    CHECK(modified[static_cast<size_t>(idx)] == op["modified"].get<int>());
                } else if (o == "get") {
                    Editor::BaseAPField* f = list[op["name"].get<std::string>()];
                    CHECK(FromBoxed(f->FieldTypeName(), f->GetValue()) == op["out"]);
                } else if (o == "listGet") {
                    Editor::BaseAPField* f = list[op["name"].get<std::string>()];
                    CHECK(FromBoxed(f->FieldTypeName(), list.GetValue(op["name"].get<std::string>())) == op["out"]);
                } else if (o == "orig") {
                    list.SetOriginalValue();
                    std::map<std::string, json> got;
                    for (int i = 0; i < list.Count(); ++i) got[list[i]->name] = FromBoxed(list[i]->FieldTypeName(), list[i]->originalValue());
                    json want = json::object();
                    // the reference lists them in member order: compare per name
                    size_t k = 0;
                    for (const json& m : c["members"]) want[m["name"].get<std::string>()] = op["out"][k++];
                    for (const auto& kv : got) CHECK(kv.second == want[kv.first]);
                } else if (o == "index") {
                    CHECK(list[op["name"].get<std::string>()] == nullptr);
                } else if (o == "ctl") {
                    log.clear();
                    Editor::BaseAPField* f = list[op["name"].get<std::string>()];
                    if (f->OnControlCreated()) f->OnControlCreated()(nullptr);
                    if (f->OnControlSetupComplete()) f->OnControlSetupComplete()(nullptr);
                    CHECK(json(log) == op["out"]);
                } else if (o == "cmp") {
                    Editor::BaseAPField* f = list[op["name"].get<std::string>()];
                    if (f->CompareValuesForCheckpoint()) CHECK(json(f->CompareValuesForCheckpoint()(ToBoxed(op["value"]))) == op["out"]);
                    else CHECK(op["out"].is_null());
                } else if (o == "display") {
                    Editor::BaseAPField* f = list[op["name"].get<std::string>()];
                    if (op["out"].get<std::string>() == "throws") CHECK_THROWS(f->GetFieldValueForDisplay());
                    else CHECK(f->GetFieldValueForDisplay() == op["out"].get<std::string>());
                } else {
                    FAIL("unknown op " << o);
                }
            } catch (const std::exception& e) {
                FAIL_CHECK("exception in apfield op " << o << ": " << e.what());
            }
        }
        MissionsEnvironment::updateValidationLed = nullptr;
        validator.reset();
        Editor::BaseAPFieldList::ResetReflectedTypes();
    }
    CHECK(cases >= 200);
    CHECK(checked > 1500);
}
