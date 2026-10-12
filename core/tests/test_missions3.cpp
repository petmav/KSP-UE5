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
#include "ksp/missions/mission_system.hpp"
#include "ksp/missions/mission_tests4.hpp"
#include "ksp/missions/missions.hpp"
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
