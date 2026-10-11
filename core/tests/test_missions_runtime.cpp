// WP-54 unit tests, part 2: behaviour derived from the decompiled C# (no golden / capture / reference exists for it; the graph and score gates are
// in test_missions.cpp). The node library of WP-55 is replaced by the stub modules below, registered in TestModuleRegistry /
// ActionModuleRegistry exactly where WP-55's classes will register themselves.
#include <cmath>
#include <map>

#include "doctest/doctest.h"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/missions/missions.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/vessel/vessel.hpp"
#include "missions_fixture.hpp"

using namespace ksp;
using namespace ksp::Expansions::Missions;
using namespace ksp::mtest;

namespace {

struct Log54 {
    static inline std::vector<std::string> lines;
};

class StubTest : public TestModule {
public:
    static constexpr const char* kTypeName = "StubTest";
    StubTest() : TestModule(kTypeName) {}
    bool result = false;
    int count = 0;
    bool Test() override { return result; }
    void Initialized() override { Log54::lines.push_back("init " + title); }
    void Cleared() override { Log54::lines.push_back("clear " + title); }
    void Load(const ConfigNode& node) override {
        TestModule::Load(node);
        node.TryGetValue("result", result);
        node.TryGetValue("count", count);
    }
    void Save(ConfigNode& node) const override {
        TestModule::Save(node);
        node.AddValue("result", result);
        node.AddValue("count", count);
    }
};

class StubAction : public ActionModule {
public:
    static constexpr const char* kTypeName = "StubAction";
    StubAction() : ActionModule(kTypeName) {}
    int fired = 0;
    Enumerator Fire() override {
        ++fired;
        Log54::lines.push_back("fire " + (node ? node->Title() : std::string("?")));
        co_yield YieldInstruction::Null();
    }
    void OnCloned(ActionModule*& other) override { Log54::lines.push_back("cloned from " + other->name()); }
};

class StubCreateVessel : public ActionModule, public IActionCreateVessel {
public:
    static constexpr const char* kTypeName = "ActionCreateVessel";
    StubCreateVessel() : ActionModule(kTypeName) {}
    bool IsA(std::string_view n) const override { return n == "ActionCreateVessel" || ActionModule::IsA(n); }
    VesselSituation vs;
    VesselSituation* GetVesselSituation() override { return &vs; }
    void Load(const ConfigNode& node) override {
        ActionModule::Load(node);
        node.TryGetValue("persistentId", vs.persistentId);
        node.TryGetValue("playerCreated", vs.playerCreated);
        node.TryGetValue("craftFile", vs.craftFile);
        vs.node = this->node;
    }
    void Save(ConfigNode& node) const override {
        ActionModule::Save(node);
        node.AddValue("persistentId", vs.persistentId);
        node.AddValue("playerCreated", vs.playerCreated);
        node.AddValue("craftFile", vs.craftFile);
    }
};

class StubScoreAction : public ActionModule, public IActionMissionScore {
public:
    static constexpr const char* kTypeName = "ActionMissionScore";
    StubScoreAction() : ActionModule(kTypeName) {}
    bool IsA(std::string_view n) const override { return n == "ActionMissionScore" || ActionModule::IsA(n); }
    std::string GetScoreDescription() override { return "[score " + title + "]"; }
    std::string GetAwardedScoreDescription() override { return "[awarded " + title + "]"; }
};

void RegisterStubs() {
    TestModuleRegistry::Register({"StubTest", [] { return std::make_unique<StubTest>(); }});
    ActionModuleRegistry::Register({"StubAction", [] { return std::make_unique<StubAction>(); }});
    ActionModuleRegistry::Register({"ActionCreateVessel", [] { return std::make_unique<StubCreateVessel>(); }});
    ActionModuleRegistry::Register({"ActionMissionScore", [] { return std::make_unique<StubScoreAction>(); }});
}

class StubGui : public Editor::IMEGUINode {
public:
    std::vector<std::string> calls;
    void SetTitleText(const std::string& t) override { calls.push_back("title " + t); }
    void SetEndNode(bool v) override { calls.push_back(std::string("end ") + (v ? "1" : "0")); }
    void UpdateInputConnectorImage() override { calls.push_back("input"); }
    void SetNodeColor(const Color&) override { calls.push_back("color"); }
    void DisplayNodeBodyParameters() override { calls.push_back("body"); }
    void ClearValidityIndicators() override { calls.push_back("clearValidity"); }
    void SetValidityIndicators(const Editor::MissionValidationTestResult& r) override { calls.push_back("validity " + r.message); }
    std::vector<Editor::IMEGUIConnector*> OutputConnectors() override { return {}; }
};

struct Events54 {
    std::vector<std::string> log;
    void Started(Mission*) { log.push_back("started"); }
    void Finished(Mission*) { log.push_back("finished"); }
    void Completed(Mission*) { log.push_back("completed"); }
    void Failed(Mission*) { log.push_back("failed"); }
    void Changing(Mission*, GameEvents::FromToAction<MENode*, MENode*> a) { log.push_back("changing " + a.from->Title() + ">" + a.to->Title()); }
    void Changed(Mission*, GameEvents::FromToAction<MENode*, MENode*> a) { log.push_back("changed " + a.from->Title() + ">" + a.to->Title()); }
    void Activated(MENode* n) { log.push_back("activated " + n->Title()); }
    void Deactivated(MENode* n) { log.push_back("deactivated " + n->Title()); }
    void GroupInit(TestGroup* g) { log.push_back("groupInit " + g->title); }
    void GroupClear(TestGroup* g) { log.push_back("groupClear " + g->title); }
    Events54() {
        GameEvents::Mission::onStarted.Add(this, &Events54::Started);
        GameEvents::Mission::onFinished.Add(this, &Events54::Finished);
        GameEvents::Mission::onCompleted.Add(this, &Events54::Completed);
        GameEvents::Mission::onFailed.Add(this, &Events54::Failed);
        GameEvents::Mission::onActiveNodeChanging.Add(this, &Events54::Changing);
        GameEvents::Mission::onActiveNodeChanged.Add(this, &Events54::Changed);
        GameEvents::Mission::onNodeActivated.Add(this, &Events54::Activated);
        GameEvents::Mission::onNodeDeactivated.Add(this, &Events54::Deactivated);
        GameEvents::Mission::onTestGroupInitialized.Add(this, &Events54::GroupInit);
        GameEvents::Mission::onTestGroupCleared.Add(this, &Events54::GroupClear);
    }
    ~Events54() {
        GameEvents::Mission::onStarted.Remove(this, &Events54::Started);
        GameEvents::Mission::onFinished.Remove(this, &Events54::Finished);
        GameEvents::Mission::onCompleted.Remove(this, &Events54::Completed);
        GameEvents::Mission::onFailed.Remove(this, &Events54::Failed);
        GameEvents::Mission::onActiveNodeChanging.Remove(this, &Events54::Changing);
        GameEvents::Mission::onActiveNodeChanged.Remove(this, &Events54::Changed);
        GameEvents::Mission::onNodeActivated.Remove(this, &Events54::Activated);
        GameEvents::Mission::onNodeDeactivated.Remove(this, &Events54::Deactivated);
        GameEvents::Mission::onTestGroupInitialized.Remove(this, &Events54::GroupInit);
        GameEvents::Mission::onTestGroupCleared.Remove(this, &Events54::GroupClear);
    }
};

ConfigNode* AddNodeCfg(ConfigNode* nodes, int i, const std::string& title, bool start, bool end, bool objective, std::initializer_list<int> from,
                       std::initializer_list<int> to, bool withTest, bool withAction, const char* endOpt = "Success") {
    ConfigNode* n = nodes->AddNode("NODE");
    n->AddValue("title", title);
    n->AddValue("id", GuidOf(i));
    n->AddValue("isStartNode", start);
    n->AddValue("isEndNode", end);
    n->AddValue("isObjective", objective);
    n->AddValue("missionEndOptions", endOpt);
    if (from.size()) {
        ConfigNode* f = n->AddNode("FROMNODES");
        for (int x : from) f->AddNode("FROMNODE")->AddValue("nodeID", GuidOf(x));
    }
    if (to.size()) {
        ConfigNode* t = n->AddNode("TONODEIDS");
        for (int x : to) t->AddValue("nodeID", GuidOf(x));
    }
    if (withTest) {
        ConfigNode* tg = n->AddNode("TESTGROUPS")->AddNode("TESTGROUP");
        tg->AddValue("title", "g " + title);
        ConfigNode* tm = tg->AddNode("TESTMODULE");
        tm->AddValue("name", "StubTest");
        tm->AddValue("title", "t " + title);
        tm->AddValue("result", false);
        tm->AddValue("count", 7);
    }
    if (withAction) {
        ConfigNode* am = n->AddNode("ACTIONS")->AddNode("ACTIONMODULE");
        am->AddValue("name", "StubAction");
        am->AddValue("title", "a " + title);
        am->AddValue("restartOnSceneLoad", true);
    }
    return n;
}

}  // namespace

TEST_CASE("WP-54 node modules: registry, TESTGROUPS / ACTIONS Load and Save, unknown module dropped, parameter bookkeeping") {
    Fixture54 fx;
    RegisterStubs();
    Log54::lines.clear();
    Mission m;
    ConfigNode cfg("MISSION");
    cfg.AddValue("title", "mods");
    ConfigNode* nodes = cfg.AddNode("NODES");
    AddNodeCfg(nodes, 0, "Start", true, false, false, {}, {1}, false, false);
    ConfigNode* n1 = AddNodeCfg(nodes, 1, "Alpha", false, false, true, {0}, {}, true, true);
    // an unknown test module and an unknown action are logged and dropped (TestGroup.cs:119, MENode.cs:727)
    n1->GetNode("TESTGROUPS")->GetNode("TESTGROUP")->AddNode("TESTMODULE")->AddValue("name", "NoSuchTest");
    n1->GetNode("ACTIONS")->AddNode("ACTIONMODULE")->AddValue("name", "NoSuchAction");
    ConfigNode* nb = n1->AddNode("NODEBODY_PARAMETERS");
    ConfigNode* mp = nb->AddNode("MODULE_PARAMETER");
    mp->AddValue("module", "StubTest");
    mp->AddValue("parameter", "count");
    m.Load(cfg, false);
    REQUIRE(m.nodes.Count() == 2);
    MENode* alpha = m.nodes.ValuesList()[1];
    REQUIRE(alpha->testGroups.size() == 1);
    REQUIRE(alpha->testGroups[0]->testModules.size() == 1);  // NoSuchTest dropped
    REQUIRE(alpha->actionModules.size() == 1);               // NoSuchAction dropped
    auto* st = dynamic_cast<StubTest*>(alpha->testGroups[0]->testModules[0].get());
    REQUIRE(st != nullptr);
    CHECK(st->title == "t Alpha");
    CHECK(st->count == 7);
    CHECK(st->testGroup() == alpha->testGroups[0].get());
    CHECK(st->node() == alpha);
    CHECK(st->GetName() == "StubTest");
    CHECK(alpha->HasNodeBodyParameter("StubTest", "count"));
    CHECK(alpha->HasTestModules());
    auto* sa = dynamic_cast<StubAction*>(alpha->actionModules[0].get());
    REQUIRE(sa != nullptr);
    CHECK(sa->node == alpha);
    CHECK(sa->restartOnSceneLoad);
    CHECK_FALSE(alpha->IsVesselNode());
    CHECK_FALSE(alpha->IsScoringNode());
    CHECK_FALSE(alpha->IsLaunchPadNode());

    // Save writes TESTGROUPS / ACTIONS / NODEBODY_PARAMETERS back (MENode.cs:1216-1243)
    ConfigNode out("NODE");
    alpha->Save(out);
    CHECK(out.GetNode("TESTGROUPS")->GetNode("TESTGROUP")->GetNode("TESTMODULE")->GetValue("name") == std::optional<std::string>("StubTest"));
    CHECK(out.GetNode("TESTGROUPS")->GetNode("TESTGROUP")->GetValue("title") == std::optional<std::string>("g Alpha"));
    CHECK(out.GetNode("ACTIONS")->GetNode("ACTIONMODULE")->GetValue("restartOnSceneLoad") == std::optional<std::string>("True"));
    CHECK(out.GetNode("NODEBODY_PARAMETERS")->GetNode("MODULE_PARAMETER")->GetValue("parameter") == std::optional<std::string>("count"));

    // parameter bookkeeping (TestModule.cs:222-262): TestModule.UpdateNodeBodyUI dereferences node.guiNode without a null test
    StubGui gui;
    alpha->guiNode = &gui;
    st->AddParameterToSAP("count");
    st->AddParameterToSAP("count");
    CHECK(st->parametersDisplayedInSAP.size() == 1);
    CHECK(st->HasSAPParameter("count"));
    st->RemoveParameterFromSAP("count");  // the node body parameter goes too, then UpdateNodeBodyUI
    CHECK_FALSE(st->HasSAPParameter("count"));
    CHECK_FALSE(alpha->HasNodeBodyParameter("StubTest", "count"));
    CHECK(gui.calls == std::vector<std::string>{"body"});
    st->AddParameterToNodeBodyAndUpdateUI("count");
    CHECK(st->HasNodeBodyParameter("count"));
    CHECK(gui.calls.size() == 2);
    st->RemoveParameterFromNodeBody("count");
    CHECK_FALSE(st->HasNodeBodyParameter("count"));
    alpha->guiNode = nullptr;

    // MENode.Title setter drives the gui node; the node colour setter on Load; isCatchAll / end node callbacks
    alpha->guiNode = &gui;
    gui.calls.clear();
    alpha->SetTitle("Renamed");
    CHECK(gui.calls == std::vector<std::string>{"title Renamed"});
    CHECK(alpha->gameObjectName == "Node (Renamed)");
    alpha->SetCatchAllNode(true);
    CHECK(gui.calls.back() == "input");
    alpha->guiNode = nullptr;
}

TEST_CASE("WP-54 node clone, MENode.Spawn(MEBasicNode), IsA / GetAllActionModules, vessel nodes") {
    Fixture54 fx;
    RegisterStubs();
    Log54::lines.clear();
    Mission m;
    // MEBasicNode.Load :151: a node library entry (the stock entries live in the Making History cfgs, not in the mirror)
    ConfigNode be("MEBASICNODE");
    be.AddValue("name", "Stub");
    be.AddValue("displayName", "Stub node");
    be.AddValue("description", "does things");
    be.AddValue("category", "Logic");
    be.AddValue("isLogicNode", true);
    be.AddValue("headerColor", "#FF8000");
    be.AddValue("tests", "StubTest");
    be.AddValue("actions", "StubAction");
    be.AddValue("actions", "ActionCreateVessel");
    ConfigNode* sap = be.AddNode("SAP_PARAMETERS");
    ConfigNode* p1 = sap->AddNode("MODULE_PARAMETER");
    p1->AddValue("module", "StubTest");
    p1->AddValue("parameter", "count");
    sap->AddNode("MODULE_PARAMETER")->AddValue("module", "StubTest");
    MEBasicNode basic;
    basic.Load(be);
    CHECK(basic.isObjective == false);  // not given and isLogicNode -> false (MEBasicNode.cs:158)
    CHECK(basic.tests == std::vector<std::string>{"StubTest"});
    CHECK(basic.actions.size() == 2);
    CHECK(basic.defaultSAPParameters.size() == 2);
    CHECK(basic.headerColor.r == doctest::Approx(1.0f));
    CHECK(basic.headerColor.g == doctest::Approx(128.0f / 255.0f));
    MENode* node = MENode::Spawn(&m, basic);
    CHECK(node->Title() == "Stub node");
    CHECK(node->category() == "Logic");
    CHECK(node->basicNodeSource == "Stub");
    CHECK(node->IsLogicNode());
    CHECK(node->isCatchAll());  // a node with test modules becomes a catch-all node (MENode.cs:491)
    CHECK(node->IsOrphanNode());
    REQUIRE(node->testGroups.size() == 1);
    CHECK(node->testGroups[0]->testModules.size() == 1);
    CHECK(node->actionModules.size() == 2);
    CHECK(node->IsVesselNode());  // ActionCreateVessel
    CHECK(node->HasPendingVesselLaunch());  // vesselSituation.launched is false
    CHECK_FALSE(node->IsActiveAndPendingVesselLaunch());  // never activated
    auto* sa = dynamic_cast<StubTest*>(node->testGroups[0]->testModules[0].get());
    REQUIRE(sa);
    CHECK(sa->HasSAPParameter("count"));  // AddParametersDisplayedInSAP matched the module name

    // MENode.Clone :511 copies the groups through Save / Load and calls OnCloned of each action with the original
    node->description = "to be cloned";
    MENode* clone = MENode::Clone(node, Vector2f(3.0f, 4.0f));
    CHECK(clone != node);
    CHECK(clone->Title() == "Stub node");
    CHECK(clone->description == "to be cloned");
    CHECK(clone->editorPosition.x == 3.0f);
    CHECK(clone->isCatchAll());
    CHECK(clone->testGroups.size() == 1);
    CHECK(clone->actionModules.size() == 2);
    CHECK(clone->id() != node->id());
    CHECK(std::count(Log54::lines.begin(), Log54::lines.end(), "cloned from StubAction") == 1);
    // GetAllActionModules<T> matches the exact type (GetType() == typeof(T))
    CHECK(node->GetAllActionModules<StubAction>().size() == 1);
    CHECK(node->GetAllActionModules<StubCreateVessel>().size() == 1);

    // Mission.GetActionCreateVessel / GetAllVesselSituations / GetNodeGuidByVesselID (Mission.cs:2432, 2532, 2320)
    m.nodes.Add(node->id(), node);
    auto* cv = dynamic_cast<StubCreateVessel*>(node->actionModules[1].get());
    REQUIRE(cv);
    cv->vs.persistentId = 4242;
    CHECK(m.GetActionCreateVessel(node) == static_cast<IActionCreateVessel*>(cv));
    CHECK(m.GetAllVesselSituations().size() == 1);
    CHECK(m.GetNodeGuidByVesselID(4242) == node->id());
    CHECK(m.GetNodeGuidByVesselID(1) == Guid::Empty());
    CHECK(m.GetVesselSituationByVesselID(4242) == &cv->vs);
    CHECK(m.GetAllActionModules<StubCreateVessel>().size() == 1);
    CHECK(m.GetNodeById(node->id()) == node);
    CHECK(m.GetNodeById(clone->id()) == nullptr);
    // BlockPlayMission :577: a creator-defined vessel without a craft file blocks the mission
    cv->vs.playerCreated = false;
    CHECK(m.BlockPlayMission());
    REQUIRE(m.BlockedMessages().size() == 1);
    CHECK(m.BlockedMessages()[0].find("Creator Defined") != std::string::npos);
    cv->vs.craftFile = "x.craft";
    CHECK_FALSE(m.BlockPlayMission());
}

TEST_CASE("WP-54 mission run-time state machine: StartMission, UpdateMission, node switching, end node scoring and awards") {
    Fixture54 fx;
    RegisterStubs();
    Log54::lines.clear();
    Events54 ev;
    Game game;
    game.Mode = Game::Modes::MISSION;
    game.flightState = std::make_shared<FlightState>();
    SaveEnvironment::Current().currentGame = &game;
    double ut = 100.0;
    MissionsEnvironment::universalTime = [&ut] { return ut; };
    MissionsEnvironment::gameUniversalTime = [&ut] { return ut; };
    std::vector<std::string> screen;
    MissionsEnvironment::postScreenMessage = [&](const std::string& t, float d) { screen.push_back(t + "@" + std::to_string(static_cast<int>(d))); };

    // Awards: one definition, AwardModule_Score enabled with a threshold of 5
    Awards defs;
    defs.internalAwards.push_back(AwardDefinition());
    defs.internalAwards[0].id = "a1";
    defs.internalAwards[0].name = "AwardModule_Score";
    defs.internalAwards[0].displayName = "Score award";
    defs.SetupAwards();
    AwardEnvironment::awardDefinitions = [&defs] { return &defs; };

    Mission m;
    ConfigNode cfg("MISSION");
    cfg.AddValue("title", "fsm");
    cfg.AddValue("isScoreEnabled", true);
    cfg.AddNode("SITUATION");
    ConfigNode* nodes = cfg.AddNode("NODES");
    AddNodeCfg(nodes, 0, "Start", true, false, false, {}, {1}, false, false);
    ConfigNode* alpha = AddNodeCfg(nodes, 1, "Alpha", false, false, true, {0}, {2}, true, true);
    alpha->AddValue("showScreenMessage", true);
    alpha->AddValue("description", "alpha says hi");
    alpha->AddValue("msgTime", 9);
    AddNodeCfg(nodes, 2, "Omega", false, true, true, {1}, {}, true, false, "Success");
    ConfigNode* sm = cfg.AddNode("SCOREMODULES")->AddNode("SCOREMODULE");
    sm->AddValue("name", "ScoreModule_Completion");
    sm->AddValue("bonusScore", 10.0f);
    ConfigNode* am = cfg.AddNode("AWARDMODULES")->AddNode("AWARDMODULE");
    am->AddValue("name", "AwardModule_Score");
    am->AddValue("awardID", "a1");
    am->AddValue("enabled", true);
    am->AddValue("score", 5.0f);
    m.Load(cfg, false);
    m.InitMission();
    Flow::MEFlowParser::ParseMission(&m);
    REQUIRE(m.nodes.Count() == 3);
    MENode* start = m.startNode();
    MENode* a = m.nodes.ValuesList()[1];
    MENode* o = m.nodes.ValuesList()[2];
    REQUIRE(start != nullptr);
    CHECK(m.activeNode() == start);
    CHECK(m.ActiveNodeName() == "Start");
    CHECK(m.globalScore->activeModules.size() == 1);
    REQUIRE(m.awards->activeModules.size() == 1);
    CHECK(static_cast<AwardModule*>(m.awards->activeModules[0].get())->Definition() == &defs.internalAwards[0]);

    // StartMission :773
    CHECK_FALSE(m.isStarted());
    m.StartMission();
    CHECK(m.isStarted());
    CHECK(m.startedUT == 100.0);
    CHECK(start->activatedUT == 100.0);
    CHECK(ev.log == std::vector<std::string>{"groupInit g Alpha", "started"});
    CHECK(Log54::lines == std::vector<std::string>{"init t Alpha"});
    CHECK(m.IsNextObjective(a));  // SetNextObjectives(start): Alpha is the first objective on the way
    CHECK_FALSE(m.IsNextObjective(o));

    // UpdateMission :835: nothing happens while Alpha's test fails
    ev.log.clear();
    m.UpdateMission();
    CHECK(ev.log.empty());
    CHECK(m.activeNode() == start);

    // a passing test switches the active node: Deactivate(start), Activate(Alpha), the coroutines of Alpha's actions run
    auto* alphaTest = static_cast<StubTest*>(a->testGroups[0]->testModules[0].get());
    alphaTest->result = true;
    ut = 200.0;
    Log54::lines.clear();
    m.UpdateMission();
    CHECK(m.activeNode() == a);
    CHECK(ev.log == std::vector<std::string>{"changing Start>Alpha", "deactivated Start", "groupClear g Alpha", "activated Alpha", "groupInit g Omega",
                                              "changed Start>Alpha"});
    CHECK(a->activatedUT == 200.0);
    CHECK(Log54::lines == std::vector<std::string>{"clear t Alpha", "init t Omega", "fire Alpha"});
    CHECK(m.IsNextObjective(o));
    CHECK(a->HasBeenActivated());
    // Alpha has activateOnceOnly (default true): a second RunTests returns false while it stays activated
    CHECK_FALSE(a->RunTests());

    // Omega is an end node: the passing test ends the mission, scores it and evaluates the awards
    ev.log.clear();
    auto* omegaTest = static_cast<StubTest*>(o->testGroups[0]->testModules[0].get());
    omegaTest->result = true;
    ut = 300.0;
    m.UpdateMission();
    CHECK(m.activeNode() == o);
    CHECK(m.isEnded());
    CHECK(m.isSuccesful());
    CHECK(m.currentScore == 10.0f);
    CHECK(m.globalScore->AwarededScoreDescription() == "Bonus score: awarded 10 points");
    REQUIRE(m.awards->awardedAwards.size() == 1);
    CHECK(m.awards->awardedAwards[0] == "a1");
    CHECK(ev.log == std::vector<std::string>{"changing Alpha>Omega", "deactivated Alpha", "groupClear g Omega", "groupClear g Alpha", "activated Omega",
                                              "changed Alpha>Omega", "completed", "finished"});
    // Save writes the run-time state back (Mission.cs:2174)
    ConfigNode saved("MISSION");
    m.Save(saved);
    CHECK(saved.GetValue("isStarted") == std::optional<std::string>("True"));
    CHECK(saved.GetValue("isEnded") == std::optional<std::string>("True"));
    CHECK(saved.GetValue("isSuccesful") == std::optional<std::string>("True"));
    CHECK(saved.GetValue("activeNodeID") == std::optional<std::string>(GuidOf(2)));
    CHECK(saved.GetValue("currentScore") == std::optional<std::string>("10"));
    CHECK(saved.GetNode("SITUATION") != nullptr);
    CHECK(saved.GetNode("FLOW")->GetNode("BLOCKS") != nullptr);
    CHECK(saved.GetNode("NODES")->CountNodes() == 3);
    SaveEnvironment::Current().currentGame = nullptr;
}

TEST_CASE("WP-54 mission end: fail end node, scoring disabled, activateOnceOnly false, paused node, screen message of the next node") {
    Fixture54 fx;
    RegisterStubs();
    Events54 ev;
    Game game;
    game.Mode = Game::Modes::MISSION;
    game.flightState = std::make_shared<FlightState>();
    SaveEnvironment::Current().currentGame = &game;
    double ut = 5.0;
    MissionsEnvironment::universalTime = [&ut] { return ut; };
    MissionsEnvironment::gameUniversalTime = [&ut] { return ut; };
    std::vector<std::string> screen;
    MissionsEnvironment::postScreenMessage = [&](const std::string& t, float d) { screen.push_back(t + "@" + std::to_string(static_cast<int>(d))); };

    Mission m;
    ConfigNode cfg("MISSION");
    cfg.AddValue("isScoreEnabled", false);
    cfg.AddNode("SITUATION");
    ConfigNode* nodes = cfg.AddNode("NODES");
    AddNodeCfg(nodes, 0, "Start", true, false, false, {}, {1}, false, false);
    ConfigNode* a = AddNodeCfg(nodes, 1, "A", false, false, true, {0}, {2}, true, false);
    a->AddValue("showScreenMessage", true);
    a->AddValue("description", "A speaks");
    a->AddValue("msgTime", 11);
    a->AddValue("activateOnceOnly", false);
    ConfigNode* b = AddNodeCfg(nodes, 2, "B", false, false, true, {1}, {3}, true, false);
    b->AddValue("showScreenMessage", true);
    b->AddValue("description", "B speaks");
    b->AddValue("msgTime", 4);
    AddNodeCfg(nodes, 3, "Boom", false, true, false, {2}, {}, true, false, "Fail");
    m.Load(cfg, false);
    m.InitMission();
    Flow::MEFlowParser::ParseMission(&m);
    m.StartMission();
    MENode* na = m.nodes.ValuesList()[1];
    MENode* nb = m.nodes.ValuesList()[2];
    MENode* nboom = m.nodes.ValuesList()[3];
    static_cast<StubTest*>(na->testGroups[0]->testModules[0].get())->result = true;
    static_cast<StubTest*>(nb->testGroups[0]->testModules[0].get())->result = true;
    static_cast<StubTest*>(nboom->testGroups[0]->testModules[0].get())->result = true;

    // a paused active node is not updated (UpdateMission :843)
    m.activeNode()->isPaused = true;
    ev.log.clear();
    m.UpdateMission();
    CHECK(ev.log.empty());
    m.activeNode()->isPaused = false;

    m.UpdateMission();  // Start -> A: ActivateNode shows B's screen message (toNodes of A)
    CHECK(m.activeNode() == na);
    REQUIRE(screen.size() == 1);
    CHECK(screen[0] == "B speaks@4");
    m.UpdateMission();  // A -> B (A's RunTests would be true again but A is not among Start's toNodes any more)
    CHECK(m.activeNode() == nb);
    ev.log.clear();
    m.UpdateMission();  // B -> Boom: a Fail end node
    CHECK(m.activeNode() == nboom);
    CHECK(m.isEnded());
    CHECK_FALSE(m.isSuccesful());
    CHECK(m.currentScore == 0.0f);  // isScoreEnabled is false
    CHECK(ev.log.back() == "finished");
    CHECK(ev.log[ev.log.size() - 2] == "failed");
    CHECK(m.awards->awardedAwards.empty());
    SaveEnvironment::Current().currentGame = nullptr;
}

TEST_CASE("WP-54 Mission Save / Load round trip: header, situation, tags, mapped vessels, banners, score and award modules, flow") {
    Fixture54 fx;
    RegisterStubs();
    GameParameters::GetDefaultParameters(Game_Modes::MISSION, GameParameters::Preset::Normal);
    MissionsEnvironment::makingHistoryVersion = [] { return std::string("1.12.0"); };
    Game game;
    game.Mode = Game::Modes::MISSION_BUILDER;
    SaveEnvironment::Current().currentGame = &game;

    auto build = [](Mission& m) {
        m.situation->crewRoster = std::make_shared<KerbalRoster>(Game_Modes::MISSION);
        m.situation->gameParameters = GameParameters::GetDefaultParameters(Game_Modes::MISSION, GameParameters::Preset::Normal);
    };
    Mission m;
    build(m);
    ConfigNode cfg("MISSION");
    cfg.AddValue("id", "11111111-2222-3333-4444-555555555555");
    cfg.AddValue("idName", "my-mission");
    cfg.AddValue("expansionVersion", "1.9.0");
    cfg.AddValue("title", "Round trip");
    cfg.AddValue("briefing", "Go\\nto the Mun");
    cfg.AddValue("author", "me");
    cfg.AddValue("modsBriefing", "mods");
    cfg.AddValue("isBriefingSet", true);
    cfg.AddValue("packName", "squad_test");
    cfg.AddValue("order", 12);
    cfg.AddValue("hardIcon", true);
    cfg.AddValue("difficulty", "Extreme");
    cfg.AddValue("seed", 987654);
    cfg.AddValue("historyId", "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee");
    cfg.AddValue("steamPublishedFileId", static_cast<uint64_t>(123456789012ull));
    cfg.AddValue("flag", "Squad/Flags/test");
    cfg.AddValue("bannerMenu", "menu.png");
    cfg.AddValue("bannerSuccess", "ok.png");
    cfg.AddValue("bannerFail", "no.png");
    cfg.AddValue("isScoreEnabled", false);
    cfg.AddValue("maxScore", 100.5f);
    cfg.AddValue("currentScore", 3.25f);
    cfg.AddValue("exportName", "exp");
    cfg.AddValue("missionNameAtLastExport", "oldname");
    cfg.AddValue("isStarted", true);
    cfg.AddValue("cameraLockMode", "IVA");
    cfg.AddValue("cameraLockOptions", "LockAllowMap");
    cfg.AddValue("saveRevertOnSwitchActiveVessel", true);
    cfg.AddValue("briefingNodeActive", false);
    ConfigNode* tags = cfg.AddNode("TAGS");
    tags->AddNode("TAG")->AddValue("name", "tutorial");
    tags->AddNode("TAG")->AddValue("name", "kerbin");
    ConfigNode* mv = cfg.AddNode("MAPPEDVESSELS")->AddNode("MAPPEDVESSEL");
    mv->AddValue("partId", 11u);
    mv->AddValue("mappedId", 22u);
    mv->AddValue("originalId", 33u);
    mv->AddValue("currentId", 44u);
    mv->AddValue("partVslName", "Probe");
    mv->AddValue("craftFileName", "probe.craft");
    mv->AddValue("situationVslName", "Probe S");
    ConfigNode* sit = cfg.AddNode("SITUATION");
    sit->AddValue("startType", "SPH");
    sit->AddValue("startVessel", 77u);
    sit->AddValue("autoGenerateCrew", true);
    sit->AddValue("startUT", 12345.5);
    sit->AddValue("resourceSeed", 4711);
    sit->AddValue("rocMissionSeed", 9);
    sit->AddValue("showBriefing", true);
    sit->AddValue("instructorName", "Instructor_Jeb_MaleVeteran");
    sit->AddValue("vintageSuit", true);
    sit->AddValue("filterType", "availableParts");
    sit->AddValue("availableParts", "mk1pod");  // renamed by MissionsUtils.UpdatePartName
    sit->AddValue("availableParts", "solidBooster.sm");
    ConfigNode* sap = sit->AddNode("SAP_PARAMETERS");
    sap->AddValue("parametersDisplayedInSAP", "startUT");
    ConfigNode* nodes = cfg.AddNode("NODES");
    AddNodeCfg(nodes, 0, "Start", true, false, false, {}, {1}, false, false);
    AddNodeCfg(nodes, 1, "Alpha", false, false, true, {0}, {2}, true, true);
    AddNodeCfg(nodes, 2, "Omega", false, true, true, {1}, {}, true, false);
    ConfigNode* sm = cfg.AddNode("SCOREMODULES");
    ConfigNode* s1 = sm->AddNode("SCOREMODULE");
    s1->AddValue("name", "ScoreModule_Completion");
    s1->AddValue("id", "5");
    s1->AddValue("bonusScore", 25.0f);
    ConfigNode* s2 = sm->AddNode("SCOREMODULE");
    s2->AddValue("name", "ScoreModule_Modifier");
    s2->AddValue("id", "6");
    s2->AddValue("type", "Substract");
    s2->AddValue("modifier", 2.5f);
    ConfigNode* aw = cfg.AddNode("AWARDMODULES");
    ConfigNode* a1 = aw->AddNode("AWARDMODULE");
    a1->AddValue("name", "AwardModule_MissionTime");
    a1->AddValue("id", "9");
    a1->AddValue("awardID", "x1");
    a1->AddValue("enabled", true);
    a1->AddValue("score", 7.5f);
    a1->AddValue("missionTime", 86400.0);
    m.Load(cfg, false);
    m.InitMission();
    Flow::MEFlowParser::ParseMission(&m);

    CHECK(m.title() == "Round trip");
    CHECK(m.gameObjectName == "Mission (my-mission)");
    CHECK(m.idName() == "my-mission");
    CHECK(m.id().ToString() == "11111111-2222-3333-4444-555555555555");
    CHECK(m.difficulty == MissionDifficulty::Extreme);
    CHECK(m.Seed() == 987654);
    CHECK(m.tags == std::vector<std::string>{"tutorial", "kerbin"});
    REQUIRE(m.mappedVessels.size() == 1);
    CHECK(m.mappedVessels[0].mappedVesselPersistentId == 22u);
    CHECK(m.mappedVessels[0].originalVesselPersistentId == 33u);
    CHECK(MappedVessels::ConvertMappedId(m.mappedVessels, 22u) == 44u);  // by mapped id
    CHECK(MappedVessels::ConvertMappedId(m.mappedVessels, 11u) == 44u);  // by part id
    CHECK(MappedVessels::ConvertMappedId(m.mappedVessels, 99u) == 99u);
    CHECK(m.bannerMenu.fileName == "menu.png");
    CHECK(m.bannerMenu.isInMissionFolder);
    CHECK(m.situation->startType == MissionSituation::StartTypeEnum::SPH);
    CHECK(m.situation->startUT == 12345.5);
    CHECK(m.situation->partFilter->filterType == MissionPartFilter::Choices::availableParts);
    CHECK(m.situation->partFilter->availableParts == std::vector<std::string>{"mk1pod.v2", "solidBooster.sm.v2"});
    CHECK(m.situation->missionInstructor.instructorName == "Instructor_Jeb_MaleVeteran");
    CHECK(m.globalScore->activeModules.size() == 2);
    CHECK(m.awards->activeModules.size() == 1);
    CHECK(m.IsTutorialMission() == false);

    ConfigNode out1("MISSION");
    m.Save(out1);
    CHECK(out1.GetValue("expansionVersion") == std::optional<std::string>("1.12.0"));  // MakingHistory installed: the installed version is saved
    CHECK(out1.GetValue("order") == std::optional<std::string>("12"));
    CHECK(out1.GetValue("difficulty") == std::optional<std::string>("Extreme"));
    CHECK(out1.GetValue("steamPublishedFileId") == std::optional<std::string>("123456789012"));
    CHECK(out1.GetValue("maxScore") == std::optional<std::string>("100.5"));
    CHECK(out1.GetValue("cameraLockMode") == std::optional<std::string>("IVA"));
    CHECK(out1.GetValue("saveRevertOnSwitchActiveVessel") == std::optional<std::string>("True"));
    CHECK(out1.GetValue("briefing") == std::optional<std::string>("Go\\nto the Mun"));
    REQUIRE(out1.GetNode("SITUATION"));
    CHECK(out1.GetNode("SITUATION")->GetValue("startType") == std::optional<std::string>("SPH"));
    CHECK(out1.GetNode("SITUATION")->GetValue("availableParts") == std::optional<std::string>("mk1pod.v2"));
    CHECK(out1.GetNode("SITUATION")->GetNode("ROSTER") != nullptr);
    CHECK(out1.GetNode("SITUATION")->GetNode("PARAMETERS") != nullptr);
    REQUIRE(out1.GetNode("SCOREMODULES"));
    CHECK(out1.GetNode("SCOREMODULES")->CountNodes() == 2);
    CHECK(out1.GetNode("MAPPEDVESSELS")->GetNode("MAPPEDVESSEL")->GetValue("partVslName") == std::optional<std::string>("Probe"));
    CHECK(out1.GetNode("NODES")->CountNodes() == 3);
    CHECK(out1.GetNode("FLOW")->GetNode("BLOCKS")->CountNodes() > 0);

    // Load(Save(x)) == x: the second save is identical to the first (module ids included, they are saved and loaded)
    Mission m2;
    build(m2);
    m2.Load(out1, false);
    m2.InitMission();
    ConfigNode out2("MISSION");
    m2.Save(out2);
    CHECK(Dump(out2) == Dump(out1));
    // the FLOW node is read back (Mission.cs:2087): the loaded flow block equals the saved one
    ConfigNode f1("FLOW"), f2("FLOW");
    m.flow->Save(f1);
    m2.flow->Save(f2);
    CHECK(Dump(f1) == Dump(f2));
    SaveEnvironment::Current().currentGame = nullptr;
}

TEST_CASE("WP-54 award modules: conditions, PROGRESS nodes only in a running mission, tracking subscriptions, definitions") {
    Fixture54 fx;
    Game game;
    game.Mode = Game::Modes::MISSION;
    SaveEnvironment::Current().currentGame = &game;
    Mission m;
    MENode* node = MENode::Spawn(&m);

    // Awards.SetupAwards :24: duplicate ids throw (Dictionary.Add)
    Awards defs;
    defs.internalAwards.resize(2);
    defs.internalAwards[0].id = "dup";
    defs.internalAwards[1].id = "dup";
    CHECK_THROWS_AS(defs.SetupAwards(), ArgumentException);
    defs.internalAwards[1].id = "other";
    defs.SetupAwards();
    CHECK(defs.GetAwardDefinition("other") == &defs.internalAwards[1]);
    CHECK(defs.GetAwardDefinition("missing") == nullptr);
    defs.internalAwards[0].name = "AwardModule_Score";
    defs.internalAwards[0].displayName = "Scorer";
    defs.internalAwards[1].name = "AwardModule_MissionTime";
    defs.internalAwards[1].displayName = "Speedy";
    AwardEnvironment::awardDefinitions = [&defs] { return &defs; };

    // MissionAwards.FillAwardsList :22 creates one module per definition whose class exists, once
    MissionAwards awards(node);
    awards.FillAwardsList();
    CHECK(awards.activeModules.size() == 2);
    awards.FillAwardsList();
    CHECK(awards.activeModules.size() == 2);
    CHECK(awards.ContainsDefinition(defs.internalAwards[0]));
    auto* scoreAward = static_cast<AwardModule*>(awards.activeModules[0].get());
    CHECK(scoreAward->GetDisplayName() == "Scorer");
    CHECK(scoreAward->TypeName() == "AwardModule_Score");
    CHECK_FALSE(static_cast<AwardModule_Score*>(scoreAward)->canBeDisplayedInEditor());
    CHECK(scoreAward->enabled);  // AwardModule_Score enables itself
    auto* timeAward = static_cast<AwardModule_MissionTime*>(awards.activeModules[1].get());
    CHECK_FALSE(timeAward->enabled);
    timeAward->enabled = true;
    timeAward->missionTime = 1000.0;
    scoreAward->score = 50.0f;
    // GetSupportedTypes: every registered AwardModule subclass
    CHECK(awards.GetSupportedTypes().size() == 10);

    // EvaluateAwards: StopTracking, then the enabled modules whose condition holds contribute their definition id
    m.currentScore = 49.0f;
    MissionsEnvironment::gameUniversalTime = [] { return 900.0; };
    awards.EvaluateAwards(&m);
    CHECK(awards.awardedAwards == std::vector<std::string>{"other"});  // time award only
    m.currentScore = 50.0f;
    MissionsEnvironment::gameUniversalTime = [] { return 1001.0; };
    awards.awardedAwards.clear();
    awards.EvaluateAwards(&m);
    CHECK(awards.awardedAwards == std::vector<std::string>{"dup"});  // score award only
    scoreAward->score = 0.0f;  // AwardModule_Score: score > 0 is required
    awards.awardedAwards.clear();
    awards.EvaluateAwards(&m);
    CHECK(awards.awardedAwards.empty());

    // Save: PROGRESS nodes only when HighLogic.CurrentGame.Mode == MISSION
    AwardModule_KerbalsSurvived survived(node);
    ConfigNode s1("AWARDMODULE");
    survived.Save(s1);
    CHECK(s1.GetNode("PROGRESS") != nullptr);
    CHECK(s1.GetNode("PROGRESS")->GetValue("kerbalsAreAlive") == std::optional<std::string>("True"));
    game.Mode = Game::Modes::SANDBOX;
    ConfigNode s2("AWARDMODULE");
    survived.Save(s2);
    CHECK(s2.GetNode("PROGRESS") == nullptr);
    game.Mode = Game::Modes::MISSION;
    // OnCrewKilled flips kerbalsAreAlive and unsubscribes
    survived.StartTracking();
    GameEvents::onCrewKilled.Fire(nullptr);
    ConfigNode s3("AWARDMODULE");
    survived.Save(s3);
    CHECK(s3.GetNode("PROGRESS")->GetValue("kerbalsAreAlive") == std::optional<std::string>("False"));
    CHECK_FALSE(survived.enabled);
    ConfigNode back("AWARDMODULE");
    survived.Save(back);
    AwardModule_KerbalsSurvived loaded(node);
    loaded.Load(back);
    ConfigNode back2("AWARDMODULE");
    loaded.Save(back2);
    CHECK(Dump(back) == Dump(back2));

    // KerbalsDied: true while no kerbal is Assigned (AwardModule_KerbalsDied.cs:14)
    AwardModule_KerbalsDied died(node);
    AwardEnvironment::crewRosterStatuses = [] { return std::vector<int>{static_cast<int>(ProtoCrewMember_RosterStatus::Available)}; };
    died.EvaluateAward(&m, awards.awardedAwards);  // no definition: nothing is added
    AwardEnvironment::crewRosterStatuses = [] { return std::vector<int>{0, static_cast<int>(ProtoCrewMember_RosterStatus::Assigned)}; };

    // VesselsUsed: persistent ids are collected once (AddUnique); condition is equality with the count
    auto v1 = std::make_unique<Vessel>();
    auto v2 = std::make_unique<Vessel>();
    v1->persistentId = 100;
    v2->persistentId = 200;
    AwardModule_VesselsUsed used(node);
    used.vesselsUsed = 2;
    used.StartTracking();
    GameEvents::onVesselChange.Fire(v1.get());
    GameEvents::onVesselChange.Fire(v1.get());
    ConfigNode u1("AWARDMODULE");
    used.Save(u1);
    CHECK(u1.GetNode("PROGRESS")->GetValuesList("vesselUsedID").size() == 1);
    GameEvents::onVesselChange.Fire(v2.get());
    used.StopTracking();
    GameEvents::onVesselChange.Fire(v1.get());
    ConfigNode u2("AWARDMODULE");
    used.Save(u2);
    CHECK(u2.GetNode("PROGRESS")->GetValuesList("vesselUsedID") == std::vector<std::string>{"100", "200"});

    // Traveled{Less,More}Than: kilometres over all vessels plus destroyed ones; the accumulator is never reset (sic)
    v1->distanceTraveled = 3000.0;
    v2->distanceTraveled = 4000.0;
    AwardEnvironment::vessels = [&] { return std::vector<Vessel*>{v1.get(), v2.get()}; };
    Awards tdefs;
    tdefs.internalAwards.resize(2);
    tdefs.internalAwards[0].id = "more";
    tdefs.internalAwards[1].id = "less";
    tdefs.SetupAwards();
    AwardEnvironment::awardDefinitions = [&tdefs] { return &tdefs; };
    AwardModule_TraveledMoreThan more(node, &tdefs.internalAwards[0]);
    more.distance = 11.5;
    AwardModule_TraveledLessThan less(node, &tdefs.internalAwards[1]);
    less.distance = 7.5;
    std::vector<std::string> got;
    less.EvaluateAward(&m, got);  // 7 km < 7.5
    CHECK(got == std::vector<std::string>{"less"});
    got.clear();
    more.EvaluateAward(&m, got);  // 7 km > 11.5: no
    CHECK(got.empty());
    more.StartTracking();
    GameEvents::onVesselWillDestroy.Fire(v2.get());  // +4000 m
    more.StopTracking();
    GameEvents::onVesselWillDestroy.Fire(v2.get());  // not tracked any more
    more.EvaluateAward(&m, got);  // the first evaluation left 7 km in the accumulator (never reset, sic): 7 + 4 + 7 = 18 km > 11.5
    CHECK(got == std::vector<std::string>{"more"});
    ConfigNode m1("AWARDMODULE");
    more.Save(m1);
    CHECK(m1.GetNode("PROGRESS")->GetValue("distanceTraveled") == std::optional<std::string>("18000"));
    CHECK(m1.GetValue("distance") == std::optional<std::string>("11.5"));
    SaveEnvironment::Current().currentGame = nullptr;
}

TEST_CASE("WP-54 validator: result dedupe, Status / IsValid, modes, a second validator is discarded, node highlighting") {
    Fixture54 fx;
    Mission m;
    MENode* a = MENode::Spawn(&m, "A");
    MENode* b = MENode::Spawn(&m, "B");
    a->SetIdForTests(Guid::Parse(GuidOf(0)));
    b->SetIdForTests(Guid::Parse(GuidOf(1)));
    CHECK(Editor::MissionEditorValidator::Instance() == nullptr);
    CHECK(Editor::MissionEditorValidator::Mode() == Editor::ValidatorMode::Manual);  // without an instance
    {
        Editor::MissionEditorValidator v;
        CHECK(Editor::MissionEditorValidator::Instance() == &v);
        CHECK(v.mode == Editor::ValidatorMode::AutoAfterRun);  // GameSettings.MISSION_VALIDATOR_MODE
        Editor::MissionEditorValidator second;                  // Awake: DestroyImmediate(this)
        CHECK(Editor::MissionEditorValidator::Instance() == &v);
        CHECK(v.Status() == Editor::ValidationStatus::None);  // Results == null
        CHECK_THROWS_AS(v.IsValid(), NullReferenceException);
        StubGui gui;
        a->guiNode = &gui;
        m.nodes.Add(a->id(), a);
        m.nodes.Add(b->id(), b);
        a->isStartNode = true;
        Editor::MissionEditorValidator::RunValidation(&m);  // no results: B has no out connector
        REQUIRE(v.Results() != nullptr);
        CHECK(v.Status() == Editor::ValidationStatus::Fail);
        CHECK_FALSE(v.IsValid());
        CHECK(m.ValidationResults() == v.Results());
        CHECK(gui.calls == std::vector<std::string>{"clearValidity"});  // node A: start node -> situation validator (nothing)
        v.AddNodePass(a, "fine");
        v.AddNodePass(a, "fine");  // identical (node, status, message): not added twice
        v.AddNodeWarn(a, "careful");
        v.AddNodeWarn(a, "careful 2");
        CHECK(v.Results()->size() == 4);  // the B failure + pass + two warnings
        v.AddNodeFail(a, "bad");
        REQUIRE(gui.calls.size() >= 3);
        CHECK(gui.calls[1] == "validity careful");  // Pass results are not highlighted, Warn and Fail are
        CHECK(gui.calls.back() == "validity bad");
        // Status: Pass -> Warn -> Fail (Fail breaks)
        Editor::MissionEditorValidator::ResetValidator();
        CHECK(v.Status() == Editor::ValidationStatus::None);
        v.highlightGUINodes = false;
        Editor::MissionEditorValidator::RunValidation(&m);
        CHECK(v.Results()->size() == 1);
        // modes cycle Manual > Save > AutoAfterRun > FullAuto > Manual
        v.mode = Editor::ValidatorMode::FullAuto;
        CHECK(Editor::MissionEditorValidator::NextMode() == Editor::ValidatorMode::Manual);
        CHECK(Editor::MissionEditorValidator::NextMode() == Editor::ValidatorMode::Save);
        // RunValidationOnParamChange: only in AutoAfterRun (after a run) or FullAuto
        m.hasBeenValidated = false;
        v.mode = Editor::ValidatorMode::AutoAfterRun;
        Editor::MissionEditorValidator::ResetValidator();
        Editor::MissionEditorValidator::RunValidationOnParamChange();  // mission_ == null: nothing
        CHECK(v.Results() == nullptr);
        Editor::MissionEditorValidator::RunValidation(&m);
        auto first = v.Results();
        v.mode = Editor::ValidatorMode::Manual;
        Editor::MissionEditorValidator::RunValidationOnParamChange();
        CHECK(v.Results() == first);
        v.mode = Editor::ValidatorMode::FullAuto;
        Editor::MissionEditorValidator::RunValidationOnParamChange();
        CHECK(v.Results() != first);
        a->guiNode = nullptr;
    }
    CHECK(Editor::MissionEditorValidator::Instance() == nullptr);
}

TEST_CASE("WP-54 flow blocks and paths: Save / Load round trip, convergence, extensions, UpdateFlowUIItems") {
    Fixture54 fx;
    Mission m;
    std::vector<MENode*> n;
    for (int i = 0; i < 5; ++i) {
        n.push_back(MENode::Spawn(&m, "N" + std::to_string(i)));
        n[i]->SetIdForTests(Guid::Parse(GuidOf(i)));
        m.nodes.Add(n[i]->id(), n[i]);
    }
    // MEPath.GetConvergence :55: first index >= minHops whose node is in the target
    Flow::MEPath p1(n[0]);
    p1.AddNodes({n[1], n[2], n[3]});
    Flow::MEPath p2(n[4]);
    p2.AddNodes({n[2], n[3]});
    std::unique_ptr<Flow::MEFlowConvergence> c;
    REQUIRE(p1.GetConvergence(p2, c));
    CHECK(c->Node() == n[2]);
    CHECK(c->Hops() == 2);
    REQUIRE(p1.GetConvergence(p2, c, 3));
    CHECK(c->Node() == n[3]);
    CHECK(c->Hops() == 3);
    CHECK_FALSE(p1.GetConvergence(p2, c, 4));
    CHECK(c == nullptr);
    CHECK(p1.First() == n[0]);
    CHECK(Flow::MEPath(&m).First() == nullptr);
    auto clone = p1.Clone();
    clone->Nodes.pop_back();
    CHECK(p1.Nodes.size() == 4);

    // MEFlowExtensions :7-:44
    Flow::PathList paths{std::make_shared<Flow::MEPath>(p1), std::make_shared<Flow::MEPath>(p2), std::make_shared<Flow::MEPath>(&m),
                         std::make_shared<Flow::MEPath>(*clone)};
    paths[2]->Nodes.clear();
    CHECK(Flow::CountStartNodes(paths) == 2);  // n0 (twice) and n4; the empty path is skipped
    CHECK(Flow::DistinctStartNodes(paths) == std::vector<MENode*>{n[0], n[4]});
    CHECK(Flow::GetPathsStarting(paths, n[0]).size() == 2);

    // MEPath / MENodePathInfo Save + Load
    ConfigNode pc("PATH");
    p1.Save(pc);
    CHECK(pc.GetValuesList("nodeId").size() == 4);
    Flow::MEPath loaded(&m);
    loaded.Load(pc);
    CHECK(loaded.Nodes == p1.Nodes);
    pc.AddValue("nodeId", GuidOf(99));  // an unknown id is logged and skipped
    loaded.Load(pc);
    CHECK(loaded.Nodes == p1.Nodes);
    Flow::MENodePathInfo info(n[0]);
    info.AddPath(std::make_shared<Flow::MEPath>(p1));
    info.AddPath(std::make_shared<Flow::MEPath>(p2));
    ConfigNode ic("INFO");
    info.Save(ic);
    CHECK(ic.GetValue("startNodeId") == std::optional<std::string>(GuidOf(0)));
    CHECK(ic.GetNodes("PATH").size() == 2);
    Flow::MENodePathInfo info2(&m);
    info2.Load(ic);
    CHECK(info2.node == n[0]);
    CHECK(info2.paths.size() == 2);
    CHECK(info2.paths[1]->Nodes == p2.Nodes);

    // blocks: Save writes NODE / THEN / OR, Load rebuilds them; unknown NODE ids are skipped
    Flow::MEFlowThenBlock root(&m);
    root.AddBlock(n[0]);
    auto orb = std::make_shared<Flow::MEFlowOrBlock>(&m);
    auto t1 = std::make_shared<Flow::MEFlowThenBlock>(&m);
    t1->AddBlock(n[1]);
    t1->AddBlock(n[2]);
    auto t2 = std::make_shared<Flow::MEFlowThenBlock>(&m);
    t2->AddBlock(n[3]);
    orb->AddOwnedBlock(t1);
    orb->AddOwnedBlock(t2);
    root.AddOwnedBlock(orb);
    root.AddBlock(n[4]);
    ConfigNode bc("BLOCKS");
    root.Save(bc);
    CHECK(Dump(bc) == "BLOCKS{NODE{id=" + GuidOf(0) + ";}OR{THEN{NODE{id=" + GuidOf(1) + ";}NODE{id=" + GuidOf(2) + ";}}THEN{NODE{id=" + GuidOf(3) +
                           ";}}}NODE{id=" + GuidOf(4) + ";}}");
    bc.AddNode("NODE")->AddValue("id", GuidOf(77));
    Flow::MEFlowThenBlock again(&m);
    again.Load(bc);
    ConfigNode bc2("BLOCKS");
    again.Save(bc2);
    bc.RemoveNodes("NODE");  // drop the unknown-id node (and the other NODE entries) to compare the structure
    CHECK(again.Blocks.size() == 3);
    CHECK(Dump(bc2).find(GuidOf(77)) == std::string::npos);
    // properties of an MEFlowBlock tree: objectives counted per node / non-empty sub block
    n[1]->isObjective = true;
    n[3]->isObjective = true;
    CHECK(root.CountObjectives() == 1);  // the Or block contains objectives; n0 / n4 are not
    CHECK(root.HasObjectives());
    CHECK(orb->CountObjectives() == 2);
    // UpdateFlowUIItems visits Or / Then blocks (UpdateMissionFlowUIDirect) and nodes (OnUpdateFlowUI)
    std::vector<std::string> visited;
    Flow::MEFlowParser parser;
    root.OnUpdateFlowUI = [&](Flow::MEFlowParser*) { visited.push_back("root"); };
    orb->OnUpdateFlowUI = [&](Flow::MEFlowParser*) { visited.push_back("or"); };
    t1->OnUpdateFlowUI = [&](Flow::MEFlowParser*) { visited.push_back("t1"); };
    n[2]->OnUpdateFlowUI = [&](Flow::MEFlowParser*) { visited.push_back("n2"); };
    parser.UpdateFlowUIItems(&root);
    CHECK(visited == std::vector<std::string>{"root", "or", "t1", "n2"});
    // the plain MEFlowBlock base logs an error for UpdateMissionFlowUI (it should be hidden by the child types)
    Flow::MEFlowBlock plain(&m);
    plain.UpdateMissionFlowUI(&parser);
}

TEST_CASE("WP-54 small data classes: MissionPack (real MissionPacks.cfg), MissionInstructor, part filter, UpdatePartName, ScoreInfo, ScoreRange, MissionMappedVessel") {
    Fixture54 fx;
    // MissionPack.cs:50: the stock pack cfg shipped in GameData/Squad/Missions
    ConfigNode::NodePtr packs = ConfigNode::Load(GameData54() + "/Squad/Missions/MissionPacks.cfg");
    REQUIRE(packs);
    ConfigNode* pn = packs->GetNode("MISSIONPACK");
    REQUIRE(pn);
    MissionPack pack;
    pack.Load(*pn);
    CHECK(pack.name == "squad_ESA");
    CHECK(pack.displayName == "#autoLOC_8003396");  // not translated at load (the tag text is "ESA Collaboration")
    CHECK(pack.order == 400);
    CHECK(pack.color == "#63AE55FF");
    CHECK(pack.description == "Unknown");  // the key is absent: Localizer.Format("#autoLOC_168872")
    MissionPack def;
    CHECK(def.order == INT32_MAX);
    CHECK(def.color == "#5576AEFF");
    ConfigNode ps("PACK");
    pack.Save(ps);
    CHECK(ps.GetValue("order") == std::optional<std::string>("400"));
    MissionPack other;
    other.displayName = "Z";
    other.order = 400;
    pack.displayName = "A";
    CHECK(MissionPack::CompareOrder(pack, other) < 0);  // equal order -> display name
    other.order = 5;
    CHECK(MissionPack::CompareOrder(pack, other) > 0);

    // MissionInstructor
    MissionInstructor mi;
    CHECK(mi.instructorName == "Instructor_Gene");
    CHECK(mi.GetNodeBodyParameterString(nullptr) == "Instructor Avatar: Gene Kerman");
    mi.instructorName = "Instructor_Val_FemaleVeteran";
    mi.vintageSuit = true;
    CHECK(mi.GetNodeBodyParameterString(nullptr).find("\nVintage Suit: True") != std::string::npos);
    ConfigNode ic("S");
    mi.Save(ic);
    MissionInstructor mi2;
    mi2.Load(ic);
    CHECK(mi2.Equals(mi));
    ConfigNode empty("S");
    mi2.Load(empty);  // instructorName = string.Empty
    CHECK(mi2.instructorName == "");
    ConfigNode ic2("S");
    mi2.Save(ic2);
    CHECK_FALSE(ic2.HasValue("instructorName"));

    // MissionPartFilter
    MissionPartFilter f(nullptr, nullptr);
    CHECK(f.CheckPartExcludelist("any"));
    f.unavailableParts = {"a", "b"};
    CHECK_FALSE(f.CheckPartExcludelist("a"));
    CHECK(f.CheckPartExcludelist("c"));
    f.filterType = MissionPartFilter::Choices::availableParts;
    f.availableParts = {"x"};
    CHECK(f.CheckPartExcludelist("x"));
    CHECK_FALSE(f.CheckPartExcludelist("y"));
    MissionsEnvironment::loadedPartNames = [] { return std::vector<std::string>{"x", "y", "z", "y"}; };
    CHECK(f.GetExcludedParts() == std::vector<std::string>{"y", "z"});
    f.filterType = MissionPartFilter::Choices::unavailableParts;
    CHECK(f.GetExcludedParts() == std::vector<std::string>{"a", "b"});
    ConfigNode fc("S");
    f.Save(fc);
    CHECK(Dump(fc) == "S{filterType=unavailableParts;unavailableParts=a;unavailableParts=b;}");
    // MissionsUtils.UpdatePartName: the retired Making History part names
    CHECK(MissionsUtils::UpdatePartName("fuelTank1-2") == "Rockomax32.BW");
    CHECK(MissionsUtils::UpdatePartName("Mark1-2Pod") == "mk1-3pod");
    CHECK(MissionsUtils::UpdatePartName("toroidalFuelTank") == "externalTankToroid");
    CHECK(MissionsUtils::UpdatePartName("somethingElse") == "somethingElse");

    // ScoreRange / ScoreInfo
    ScoreRange r1, r2;
    r1.minRange = r2.minRange = 1.0;
    r1.maxRange = 2.0;
    r2.maxRange = 2.0;
    r1.score = r2.score = 3.5f;
    CHECK(r1.Equals(r2));
    r2.score = 3.6f;
    CHECK_FALSE(r1.Equals(r2));
    CHECK(r1.isValueInRange(1.0));
    CHECK(r1.isValueInRange(2.0));
    CHECK_FALSE(r1.isValueInRange(2.0000001));
    ScoreInfo si;
    si.missionId = Guid::Parse(GuidOf(3));
    si.score = 99.5f;
    si.timeTaken = 1234.5;
    si.resultsText = "line1\nline2\tx";
    si.awardIds = {"a", "b"};
    si.completedAt = 133000000000000000LL;
    ConfigNode sc("SCORE");
    si.Save(sc);
    CHECK(sc.GetValue("resultsText") == std::optional<std::string>("line1\\nline2\\tx"));
    CHECK(sc.GetValue("completedAt") == std::optional<std::string>("133000000000000000"));
    ScoreInfo si2;
    si2.Load(sc);
    CHECK(si2.id == si.id);
    CHECK(si2.score == 99.5f);
    CHECK(si2.awardIds == si.awardIds);
    CHECK(si2.completedAt == si.completedAt);

    // MissionMappedVessel extensions (Mission.cs / MissionMappedVesselsExtension.cs)
    std::vector<MissionMappedVessel> list;
    list.emplace_back(1u, 10u, 20u, "Probe", "p.craft", "S");
    list.emplace_back(2u, 11u, 21u, "#autoLOC_8100086", "q.craft", "T");
    list[0].mappedVesselPersistentId = 100;
    list[1].mappedVesselPersistentId = 200;
    CHECK(MappedVessels::IndexPartId(list, 2u) == 1);
    CHECK(MappedVessels::IndexMappedVesselId(list, 100u) == 0);
    CHECK(MappedVessels::IndexCurrentVesselId(list, 21u) == 1);
    CHECK(MappedVessels::MappedVesselName(list, Loc54::Format("#autoLOC_8100086")) == 1);
    CHECK(MappedVessels::Exists(list, 10u, 1u, "Probe", "p.craft"));  // by the original vessel id
    CHECK(MappedVessels::Exists(list, 20u, 1u, "Probe", "p.craft"));  // or the current one
    CHECK_FALSE(MappedVessels::Exists(list, 20u, 2u, "Probe", "p.craft"));
    CHECK(MappedVessels::ConvertMappedId(list, 2u) == 21u);  // part id first
    CHECK(MappedVessels::ConvertMappedId(list, 100u) == 20u);  // then the mapped id
    CHECK(MappedVessels::ConvertMappedId(list, 5u) == 5u);
    ConfigNode mn("MISSION");
    MissionMappedVessel::Save(mn, list);
    auto loaded = MissionMappedVessel::Load(*mn.GetNode("MAPPEDVESSELS"));
    REQUIRE(loaded.size() == 2);
    CHECK(loaded[1].mappedVesselPersistentId == 200u);
    CHECK(loaded[1].originalVesselPersistentId == 11u);
    ConfigNode none("MISSION");
    MissionMappedVessel::Save(none, {});
    CHECK(none.GetNode("MAPPEDVESSELS") == nullptr);  // an empty list writes nothing
}

TEST_CASE("WP-54 MissionSituation: vessel list, start type, Save in the builder / mission modes") {
    Fixture54 fx;
    RegisterStubs();
    Game game;
    game.Mode = Game::Modes::MISSION;
    SaveEnvironment::Current().currentGame = &game;
    Mission m;
    m.situation->crewRoster = std::make_shared<KerbalRoster>(Game_Modes::MISSION);
    m.situation->gameParameters = GameParameters::GetDefaultParameters(Game_Modes::MISSION, GameParameters::Preset::Normal);
    ConfigNode cfg("MISSION");
    cfg.AddNode("SITUATION");
    ConfigNode* nodes = cfg.AddNode("NODES");
    AddNodeCfg(nodes, 0, "Start", true, false, false, {}, {}, false, false);
    ConfigNode* v1 = AddNodeCfg(nodes, 1, "V1", false, false, false, {}, {}, false, false);
    ConfigNode* v2 = AddNodeCfg(nodes, 2, "V2", false, false, false, {}, {}, false, false);
    for (ConfigNode* v : {v1, v2}) {
        ConfigNode* am = v->AddNode("ACTIONS")->AddNode("ACTIONMODULE");
        am->AddValue("name", "ActionCreateVessel");
        am->AddValue("persistentId", v == v1 ? 1001u : 1002u);
        am->AddValue("playerCreated", v == v1);
    }
    // the start node docks both vessel nodes (their dockParentNodeID points at the start node)
    nodes->GetNodes("NODE")[1]->AddValue("dockParentNodeID", GuidOf(0));
    nodes->GetNodes("NODE")[2]->AddValue("dockParentNodeID", GuidOf(0));
    ConfigNode* dn = nodes->GetNodes("NODE")[0]->AddNode("DOCKEDNODEIDS");
    dn->AddValue("nodeID", GuidOf(1));
    dn->AddValue("nodeID", GuidOf(2));
    m.Load(cfg, false);
    m.InitMission();
    MENode* n1 = m.nodes.ValuesList()[1];
    MENode* n2 = m.nodes.ValuesList()[2];
    REQUIRE(m.startNode()->dockedNodes.size() == 2);
    CHECK(n1->IsDockedToStartNode());
    CHECK(n1->IsVesselNode());
    auto* cv1 = dynamic_cast<StubCreateVessel*>(n1->actionModules[0].get());
    auto* cv2 = dynamic_cast<StubCreateVessel*>(n2->actionModules[0].get());
    REQUIRE(cv1);
    REQUIRE(cv2);
    cv1->vs.node = n1;
    cv2->vs.node = n2;

    // FillVesselSituationList :543 (the start node's docked nodes): player created vessels are not ready, others are
    MissionSituation& s = *m.situation;
    s.FillVesselSituationList(m.startNode());
    CHECK(s.VesselSituationList().Count() == 2);
    CHECK_FALSE(cv1->vs.readyToLaunch);
    CHECK(cv2->vs.readyToLaunch);
    CHECK(s.VesselsArePending());
    CHECK(s.NumberofVesselsPending() == 1);
    CHECK(s.CurrentVesselToBuildId() == 1001u);  // UpdateVesselBuildValues picked the first pending player vessel
    CHECK(s.CurrentVesselToBuild() == &cv1->vs);
    CHECK(s.VesselSituationListContains(1002u));
    CHECK(s.GetVesselSituationByVesselID(1001u) == &cv1->vs);
    // ready / revert / launched flags by vessel id through the mission lookups
    CHECK(s.VesselSituationReadyToLaunch(&cv1->vs));
    CHECK(cv1->vs.readyToLaunch);
    CHECK(s.VesselSituationRevertLaunch(&cv1->vs));
    CHECK_FALSE(cv1->vs.readyToLaunch);
    CHECK(s.VesselSituationLaunched(&cv2->vs));
    s.RemoveLaunchedVessels();
    CHECK(s.VesselSituationList().Count() == 1);
    // MENode.HasPendingVesselLaunch: a vessel that has not launched is pending
    CHECK(n1->HasPendingVesselLaunch());
    CHECK(m.PendingVesselLaunch(n1));
    CHECK_FALSE(n2->HasPendingVesselLaunch());

    // Save in mission mode writes VESSELSTOBUILDLIST / STARTINGACTIONSLIST (MissionSituation.cs:468)
    ConfigNode sc("SITUATION");
    s.Save(sc);
    REQUIRE(sc.GetNode("VESSELSTOBUILDLIST"));
    CHECK(sc.GetNode("VESSELSTOBUILDLIST")->GetNode("VESSELTOBUILD")->GetValue("NodeGuid") == std::optional<std::string>(GuidOf(1)));
    CHECK(sc.GetNode("STARTINGACTIONSLIST") == nullptr);  // no non-vessel actions
    CHECK(sc.GetValue("startType") == std::optional<std::string>("VesselList"));
    // in the builder scene Save recomputes the start type from the docked nodes (setStartType :352)
    SaveEnvironment::Current().loadedScene = GameScenes::MISSIONBUILDER;
    ConfigNode sb("SITUATION");
    s.Save(sb);
    SaveEnvironment::Current().loadedScene = GameScenes::SPACECENTER;
    CHECK(sb.GetValue("startType") == std::optional<std::string>("VAB"));  // a player-created vessel, VAB
    CHECK(sb.GetValue("startVessel") == std::optional<std::string>("1001"));
    SaveEnvironment::Current().currentGame = nullptr;
}

TEST_CASE("WP-54 enums: member names and [Description] tags match the decompiled declarations") {
    CHECK(dotnet::EnumToString(MissionEndOptions::Fail) == "Fail");
    CHECK(dotnet::EnumToString(MissionDifficulty::Extreme) == "Extreme");
    CHECK(EnumDescription(MissionDifficulty::Advanced) == "#autoLOC_8003169");
    CHECK(EnumDescription(ScoreModifierType::Substract) == "#autoLOC_8100129");
    CHECK(EnumDescription(ScoreModifierType::Set) == "#autoLOC_8100130");
    CHECK(EnumDescription(MissionCameraLockOptions::LockDisableMap) == "#autoLOC_8004189");
    CHECK(EnumDescription(TestComparisonOperator::GreaterThanorEqual) == "#autoLOC_8000057");
    CHECK(EnumDescription(MissionTypes::Base) == "Base");  // no [Description]: the name
    CHECK(EnumDescription(Editor::ValidationStatus::Fail) == "#autoLOC_8003072");
    CHECK(EnumDescription(Editor::ValidatorMode::FullAuto) == "#autoLOC_8200121");
    CHECK(static_cast<int>(MissionSituation::StartTypeEnum::SaveFile) == 4);
    CHECK(dotnet::EnumToString(ActionNumberOperator::Divide) == "Divide");
    CHECK(Guid::Parse(GuidOf(5)).ToString() == GuidOf(5));
    // the Guid hash used by DictionaryValueList<Guid, ...>
    CHECK(std::hash<Guid>()(Guid::Parse(GuidOf(1))) != std::hash<Guid>()(Guid::Parse(GuidOf(2))));
}

TEST_CASE("WP-54 DynamicModule: ids, Clone, Equals, registry, MissionScore global modules") {
    Fixture54 fx;
    Mission m;
    MENode* node = MENode::Spawn(&m);
    ScoreModule_Completion a(node);
    ScoreModule_Completion b(node);
    CHECK(a.id() == "1");  // ObjectIDGenerator ids are 1, 2, 3 ...
    CHECK(b.id() == "2");
    CHECK(a.name() == "ScoreModule_Completion");
    CHECK(a.GetName() == "ScoreModule_Completion_1");
    a.bonusScore = 4.0f;
    b.bonusScore = 4.0f;
    CHECK(a.EqualsObject(b));  // name and field
    b.bonusScore = 5.0f;
    CHECK_FALSE(a.EqualsObject(b));
    ScoreModule_Modifier mod(node);
    CHECK_FALSE(a.EqualsObject(mod));
    CHECK(a.Equals(b));  // DynamicModule.Equals: the name only
    // Clone: a new instance consumes an id, then every field including id is copied
    DynamicModule::ResetObjectIds();
    ScoreModule_Completion c(node);  // id 1
    c.bonusScore = 9.0f;
    c.AddParameterToSAP("bonusScore");
    c.AddParameterToSAP("bonusScore");
    auto clone = c.Clone();
    CHECK(clone->id() == "1");
    CHECK(DynamicModule::NextObjectId() == 3);  // the clone's construction took id 2
    auto* cc = static_cast<ScoreModule_Completion*>(clone.get());
    CHECK(cc->bonusScore == 9.0f);
    CHECK(cc->parametersDisplayedInSAP.size() == 1);

    // MissionScore: the default global modules, AddGlobalScoreModule, supported types with a scoreable objective, DynamicModuleList Load / Save / Clone
    MissionScore ms(node);
    CHECK(MissionScore::globalScoreModules() == std::vector<std::string>{"ScoreModule_Completion", "ScoreModule_Modifier"});
    MissionScore::AddGlobalScoreModule("ScoreModule_Time");
    MissionScore::AddGlobalScoreModule("ScoreModule_Time");  // already there: warning
    MissionScore::AddGlobalScoreModule("AwardModule_Score");  // not a ScoreModule: warning
    CHECK(MissionScore::globalScoreModules().size() == 3);
    CHECK(ms.GetSupportedTypes().size() == 3);
    ConfigNode cfg("MISSION");
    ConfigNode* l = cfg.AddNode("SCOREMODULES");
    ConfigNode* m1 = l->AddNode("SCOREMODULE");
    m1->AddValue("name", "ScoreModule_Accuracy");
    m1->AddValue("id", "17");
    ConfigNode* r = m1->AddNode("SCORERANGES")->AddNode("SCORERANGE");
    r->AddValue("minRange", 0.0);
    r->AddValue("maxRange", 50.0);
    r->AddValue("score", 10.0f);
    l->AddNode("SCOREMODULE")->AddValue("name", "NoSuchScoreModule");
    ms.Load(cfg);
    REQUIRE(ms.activeModules.size() == 1);  // the unknown module is logged and dropped
    CHECK(ms.activeModules[0]->id() == "17");
    CHECK(ms.ContainsModule("ScoreModule_Accuracy"));
    CHECK_FALSE(ms.ContainsModule("ScoreModule_Time"));
    ConfigNode saved("MISSION");
    ms.Save(saved);
    CHECK(saved.GetNode("SCOREMODULES")->GetNode("SCOREMODULE")->GetValue("name") == std::optional<std::string>("ScoreModule_Accuracy"));
    auto listClone = ms.Clone();
    CHECK(listClone->activeModules.size() == 1);
    CHECK(listClone->Equals(ms));
    CHECK(ms.ScoreDescription().find("Scoring for") != std::string::npos);
    // the mission's total: AwardScore chains the modules over mission.currentScore
    m.currentScore = 1.0f;
    ScoreModule_Completion bonus(node);
    bonus.bonusScore = 2.0f;
    ms.activeModules.clear();  // (ScoreModule_Accuracy would need the scoreable objective of a dock parent)
    ms.activeModules.push_back(bonus.Clone());
    ms.AwardScore(&m);
    CHECK(m.currentScore == 3.0f);
}
