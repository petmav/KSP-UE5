// WP-54 unit tests: Making History mission runtime, slice 1 (core/include/ksp/missions).
//
// No golden and no capture of the mission runtime exists (harness/golden/MANIFEST.json and harness/captures/out*/ carry none), and the mission
// files shipped in the GameData mirror (Squad/Missions/*/persistent.mission, *.loadmeta, *.craft) are all 0 bytes, so no stock mission can be
// loaded. Every expectation of the "reference" cases comes from tools/wp54/mission_reference.py (independent Python written from the decompiled
// C#, tests/data/wp54_reference.json): seeded random mission graphs (flow parser, node Save, validator, objective printing) and score cases
// (float32 Mono rule, the description texts through the real dictionary.cfg). The other cases pin down behaviour derived from the C# (the
// derivation is in the comment above the check). Graphs are synthetic inputs for the algorithms, not game data.
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/missions.hpp"
#include "missions_fixture.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/util/ksp_util.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using namespace ksp::Expansions::Missions;
using json = nlohmann::json;
using Loc54 = KSP::Localization::Localizer;
using namespace ksp::mtest;

namespace {

const json& Ref54() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp54_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

// ---- the graph cases ------------------------------------------------------------------------------------------------------------

// The MISSION node of a spec, with the keys MENode.Save / Mission.Save write.
ConfigNode BuildMission(const json& spec) {
    ConfigNode m("MISSION");
    m.AddValue("id", "11111111-2222-3333-4444-555555555555");
    m.AddValue("title", "Graph mission");
    m.AddValue("isStarted", spec["started"].get<bool>());
    const int active = spec["active"].get<int>();
    if (active >= 0) m.AddValue("activeNodeID", GuidOf(active));
    ConfigNode* nodes = m.AddNode("NODES");
    const auto& ns = spec["nodes"];
    for (size_t i = 0; i < ns.size(); ++i) {
        const json& s = ns[i];
        ConfigNode* n = nodes->AddNode("NODE");
        n->AddValue("title", s["title"].get<std::string>());
        n->AddValue("id", GuidOf(static_cast<int>(i)));
        n->AddValue("description", Esc(s["description"].get<std::string>()));
        if (!s["category"].get<std::string>().empty()) n->AddValue("category", s["category"].get<std::string>());
        n->AddValue("showScreenMessage", s["showMsg"].get<bool>());
        n->AddValue("msgTime", s["msgTime"].get<int>());
        n->AddValue("editorPosition", Vector2f(s["pos"][0].get<float>(), s["pos"][1].get<float>()));
        n->AddValue("isStartNode", s["start"].get<bool>());
        n->AddValue("isEndNode", s["end"].get<bool>());
        n->AddValue("message", Esc(s["message"].get<std::string>()));
        n->AddValue("dockedIndex", s["dockedIndex"].get<int>());
        n->AddValue("isObjective", s["obj"].get<bool>());
        n->AddValue("isCatchAll", s["catch"].get<bool>());
        n->AddValue("isEvent", s["event"].get<bool>());
        n->AddValue("activateOnceOnly", s["once"].get<bool>());
        n->AddValue("activatedUT", s["activatedUT"].get<double>());
        n->AddValue("isLogicNode", s["logic"].get<bool>());
        n->AddValue("basicNodeSource", s["basicSrc"].get<std::string>());
        n->AddValue("basicNodeIconURL", "");
        n->AddValue("nodeColor", "#808080");
        if (s["dockParent"].get<int>() >= 0) n->AddValue("dockParentNodeID", GuidOf(s["dockParent"].get<int>()));
        if (!s["from"].empty()) {
            ConfigNode* f = n->AddNode("FROMNODES");
            for (const auto& x : s["from"]) f->AddNode("FROMNODE")->AddValue("nodeID", GuidOf(x.get<int>()));
        }
        if (!s["to"].empty()) {
            ConfigNode* t = n->AddNode("TONODEIDS");
            for (const auto& x : s["to"]) t->AddValue("nodeID", GuidOf(x.get<int>()));
        }
        if (!s["docked"].empty()) {
            ConfigNode* d = n->AddNode("DOCKEDNODEIDS");
            for (const auto& x : s["docked"]) d->AddValue("nodeID", GuidOf(x.get<int>()));
        }
        n->AddValue("missionEndOptions", s["endOpt"].get<std::string>());
    }
    return m;
}

std::string BlockStr(const Flow::IMEFlowBlock* b, const std::map<const MENode*, int>& idx) {
    switch (b->FlowKind()) {
        case Flow::IMEFlowBlock::Kind::Node: return std::to_string(idx.at(static_cast<const MENode*>(b)));
        case Flow::IMEFlowBlock::Kind::Block:
        case Flow::IMEFlowBlock::Kind::Then:
        case Flow::IMEFlowBlock::Kind::Or: {
            const auto* fb = static_cast<const Flow::MEFlowBlock*>(b);
            std::string s = b->FlowKind() == Flow::IMEFlowBlock::Kind::Or ? "O(" : (b->FlowKind() == Flow::IMEFlowBlock::Kind::Then ? "T(" : "B(");
            for (size_t i = 0; i < fb->Blocks.size(); ++i) {
                if (i) s += " ";
                s += BlockStr(fb->Blocks[i], idx);
            }
            return s + ")";
        }
    }
    return "";
}

json IndexList(const std::vector<MENode*>& l, const std::map<const MENode*, int>& idx) {
    json j = json::array();
    for (MENode* n : l) j.push_back(idx.at(n));
    return j;
}

json PathsJson(const Mission& m, const dotnet::Dictionary<MENode*, std::shared_ptr<Flow::MENodePathInfo>>& d, const std::map<const MENode*, int>& idx) {
    json out = json::array();
    for (const auto& kv : d) {
        json paths = json::array();
        for (const auto& p : kv.second->paths) paths.push_back(IndexList(p->Nodes, idx));
        out.push_back(json::array({idx.at(kv.first), paths}));
    }
    (void)m;
    return out;
}

}  // namespace

TEST_CASE("WP-54 reference: mission graphs (InitMission, flow parser, node Save, validator)") {
    Fixture54 fx;
    const json& graphs = Ref54()["graphs"];
    REQUIRE(graphs.size() == 220);
    size_t nodesChecked = 0, blocksChecked = 0, printChecked = 0;
    for (size_t gi = 0; gi < graphs.size(); ++gi) {
        const json& spec = graphs[gi]["spec"];
        const json& exp = graphs[gi]["expected"];
        INFO("graph " << gi);
        Mission m;
        ConfigNode cfg = BuildMission(spec);
        m.Load(cfg, false);
        m.InitMission();
        std::map<const MENode*, int> idx;
        for (size_t i = 0; i < m.nodes.ValuesList().size(); ++i) idx[m.nodes.ValuesList()[i]] = static_cast<int>(i);
        const auto& ns = m.nodes.ValuesList();
        REQUIRE(ns.size() == spec["nodes"].size());

        // Mission.InitMission :401
        for (size_t i = 0; i < ns.size(); ++i) {
            CHECK_MESSAGE(IndexList(ns[i]->fromNodes, idx) == exp["fromNodes"][i], "fromNodes of node ", i);
            CHECK_MESSAGE(IndexList(ns[i]->toNodes, idx) == exp["toNodes"][i], "toNodes of node ", i);
        }
        // orphans: the nodes that are orphans before the edge pass
        json orphans = json::array();
        for (size_t i = 0; i < ns.size(); ++i) {
            // IsOrphanNode: no fromNodeIDs && !isStartNode && isCatchAll && !IsDocked
            if (exp["isOrphan"][i].get<bool>()) orphans.push_back(static_cast<int>(i));
            CHECK(ns[i]->IsOrphanNode() == exp["isOrphan"][i].get<bool>());
            CHECK(ns[i]->HasBeenActivated() == exp["hasBeenActivated"][i].get<bool>());
            CHECK(ns[i]->IsDockedToStartNode() == exp["isDockedToStart"][i].get<bool>());
        }
        CHECK(orphans == exp["orphans"]);

        // MEFlowParser.ParseMission :172
        std::shared_ptr<Flow::MEFlowThenBlock> block = Flow::MEFlowParser::ParseMission(&m);
        REQUIRE(block != nullptr);
        CHECK(PathsJson(m, m.flow->NodePaths, idx) == exp["paths"]);
        CHECK(PathsJson(m, m.flow->NodeReversePaths, idx) == exp["revPaths"]);
        CHECK(BlockStr(m.flow->missionBlock.get(), idx) == exp["block"].get<std::string>());
        blocksChecked += 1;
        nodesChecked += ns.size();

        // MENode.IsReachable :172 / IsHiddenByEvent :199 (mission.activeNode and isStarted come from the file)
        for (size_t i = 0; i < ns.size(); ++i) {
            CHECK_MESSAGE(ns[i]->IsReachable() == exp["isReachable"][i].get<bool>(), "IsReachable ", i);
            CHECK_MESSAGE(ns[i]->IsHiddenByEvent() == exp["isHidden"][i].get<bool>(), "IsHiddenByEvent ", i);
        }
        if (exp.contains("blockProps")) {
            const json& bp = exp["blockProps"];
            const auto* mb = m.flow->missionBlock.get();
            CHECK(mb->CountObjectives() == bp["countObjectives"].get<int>());
            CHECK(mb->HasObjectives() == bp["hasObjectives"].get<bool>());
            CHECK(mb->HasReachableObjectives() == bp["hasReachable"].get<bool>());
            CHECK(mb->HasVisibleChildren() == bp["hasVisible"].get<bool>());
        }

        // Mission.SetNextObjectives :957 / IsNextObjective :986
        m.SetNextObjectives(m.activeNode());
        json nxt = json::array();
        for (size_t i = 0; i < ns.size(); ++i)
            if (m.IsNextObjective(ns[i])) nxt.push_back(static_cast<int>(i));
        // the reference lists them in discovery order: compare as sets (IsNextObjective has no order)
        json want = exp["nextObjectives"];
        std::sort(want.begin(), want.end());
        CHECK(nxt == want);

        // MissionsUtils.GetNodesOrdered :521
        json ord = json::array(), ordObj = json::array();
        for (MENode* n : MissionsUtils::GetNodesOrdered(&m, false)) ord.push_back(idx.at(n));
        for (MENode* n : MissionsUtils::GetNodesOrdered(&m, true)) ordObj.push_back(idx.at(n));
        CHECK(ord == exp["nodesOrdered"]);
        CHECK(ordObj == exp["nodesOrderedObjectives"]);

        // Mission.PrintObjectives :1037
        if (exp["printable"].get<bool>()) {
            CHECK(m.PrintObjectives(false, false) == exp["printAll"].get<std::string>());
            CHECK(m.PrintObjectives(true, false) == exp["printActivatedOnly"].get<std::string>());
            CHECK(m.PrintObjectives(false, true) == exp["printFromActive"].get<std::string>());
            ++printChecked;
        }

        // MENode.Save :1148 / MissionFlow.Save :40
        for (size_t i = 0; i < ns.size(); ++i) {
            ConfigNode out("NODE");
            ns[i]->Save(out);
            CHECK_MESSAGE(Dump(out) == exp["nodeSave"][i].get<std::string>(), "node ", i);
        }
        ConfigNode flow("FLOW");
        m.flow->Save(flow);
        CHECK(Dump(flow) == exp["flowSave"].get<std::string>());

        // MENode.RunValidationWrapper :943 through MissionEditorValidator.RunValidation :144
        Editor::MissionEditorValidator validator;
        Editor::MissionEditorValidator::RunValidation(&m);
        json fails = json::array();
        for (const auto& r : *validator.Results()) {
            CHECK(r.status == Editor::ValidationStatus::Fail);
            CHECK(r.message == "No out connector and node is not an End node.");
            CHECK(r.type == "Node");
            for (size_t i = 0; i < ns.size(); ++i)
                if (ns[i]->id() == r.nodeId) fails.push_back(static_cast<int>(i));
        }
        CHECK(fails == exp["validatorFails"]);
        CHECK(m.hasBeenValidated);
    }
    CHECK(nodesChecked > 1000);
    CHECK(printChecked > 40);
    MESSAGE("graphs " << graphs.size() << ", nodes " << nodesChecked << ", printed " << printChecked);
}

// ---- the score cases -------------------------------------------------------------------------------------------------------------

namespace {
// A test module that stands for the objective of the dock parent (IScoreableObjective of WP-55's TestVessel / TestDistance ...)
class StubScoreable : public TestModule, public IScoreableObjective {
public:
    StubScoreable() : TestModule("StubScoreable") {}
    ScoreModifier modifier;
    ScoreModifier GetScoreModifier(const std::string&) override { return modifier; }
};

// two nodes: node `parent` holds the scoreable test module, `scoreNode` is docked to it
struct ScoreWorld {
    Mission mission;
    MENode* parent;
    MENode* scoreNode;
    StubScoreable* stub;
    ScoreWorld() {
        parent = MENode::Spawn(&mission);
        scoreNode = MENode::Spawn(&mission);
        auto group = std::make_unique<TestGroup>(parent);
        auto s = std::make_unique<StubScoreable>();
        stub = s.get();
        s->Initialize(group.get());
        group->testModules.push_back(std::move(s));
        parent->testGroups.push_back(std::move(group));
        scoreNode->dockParentNode = parent;
    }
};

void AddRanges(ConfigNode& cfg, const json& ranges) {
    ConfigNode* rs = cfg.AddNode("SCORERANGES");
    for (const auto& r : ranges) {
        ConfigNode* n = rs->AddNode("SCORERANGE");
        n->AddValue("minRange", r["min"].get<double>());
        n->AddValue("maxRange", r["max"].get<double>());
        n->AddValue("score", r["score"].get<float>());
    }
}
}  // namespace

TEST_CASE("WP-54 reference: score modules (float32, descriptions through the real dictionary)") {
    Fixture54 fx;
    const json& scores = Ref54()["scores"];
    std::map<std::string, int> counts;
    for (size_t ci = 0; ci < scores.size(); ++ci) {
        const json& c = scores[ci];
        INFO("score case " << ci << " " << c["type"].get<std::string>());
        const std::string type = c["type"].get<std::string>();
        const float current = c["current"].get<float>();
        const uint32_t want = static_cast<uint32_t>(std::stoul(c["result"].get<std::string>(), nullptr, 16));
        ScoreWorld w;
        ConfigNode cfg("SCOREMODULE");
        if (type == "Completion") {
            ScoreModule_Completion mod(w.scoreNode);
            cfg.AddValue("bonusScore", c["bonus"].get<float>());
            mod.Load(cfg);
            CHECK(Bits54(mod.AwardScore(current)) == want);
            CHECK(mod.AwarededScoreDescription() == c["awarded"].get<std::string>());
            CHECK(mod.ScoreDescription() == c["desc"].get<std::string>());
        } else if (type == "Modifier") {
            ScoreModule_Modifier mod(w.scoreNode);
            cfg.AddValue("type", c["modType"].get<std::string>());
            cfg.AddValue("modifier", c["mod"].get<float>());
            mod.Load(cfg);
            CHECK(Bits54(mod.AwardScore(current)) == want);
            CHECK(mod.AwarededScoreDescription() == c["awarded"].get<std::string>());
            CHECK(mod.ScoreDescription() == c["desc"].get<std::string>());
        } else if (type == "Time") {
            ScoreModule_Time mod(w.scoreNode);
            AddRanges(cfg, c["ranges"]);
            mod.Load(cfg);
            MissionsEnvironment::universalTime = [ut = c["ut"].get<double>()] { return ut; };
            CHECK(Bits54(mod.AwardScore(current)) == want);
            CHECK(mod.AwarededScoreDescription() == c["awarded"].get<std::string>());
        } else if (type == "Accuracy") {
            ScoreModule_Accuracy mod(w.scoreNode);
            AddRanges(cfg, c["ranges"]);
            mod.Load(cfg);
            w.stub->modifier.number = c["accuracy"].get<double>();
            CHECK(Bits54(mod.AwardScore(current)) == want);
            CHECK(mod.AwarededScoreDescription() == c["awarded"].get<std::string>());
            CHECK(mod.ScoreDescription() == c["desc"].get<std::string>());
        } else if (type == "Resource") {
            ScoreModule_Resource mod(w.scoreNode);
            AddRanges(cfg, c["ranges"]);
            mod.Load(cfg);
            Vessel* fake = reinterpret_cast<Vessel*>(static_cast<uintptr_t>(0x1000));
            AwardEnvironment::activeVessel = [fake] { return fake; };
            AwardEnvironment::connectedResourceTotal = [amt = c["amount"].get<double>()](Vessel*, const std::string& name, double& amount) {
                CHECK(name == "LiquidFuel");
                amount = amt;
                return true;
            };
            CHECK(Bits54(mod.AwardScore(current)) == want);
            CHECK(mod.AwarededScoreDescription() == c["awarded"].get<std::string>());
            CHECK(mod.ScoreDescription() == c["desc"].get<std::string>());
        }
        ++counts[type];
    }
    CHECK(counts["Completion"] == 60);
    CHECK(counts["Modifier"] == 120);
    CHECK(counts["Time"] == 60);
    CHECK(counts["Accuracy"] == 80);
    CHECK(counts["Resource"] == 80);
}
