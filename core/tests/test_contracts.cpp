// WP-37 unit tests: contract system core and stock templates (core/include/ksp/contracts).
//
// No golden or capture of contracts exists (harness/golden/MANIFEST.json and harness/captures/out*/ carry no contract, agent or
// ContractSystem data), so every expectation is derived: from tools/wp37/contract_reference.py (an independent Python / numpy
// implementation that reads Contracts.cfg, Agents.cfg, StoryDefs.cfg, the en-us dictionary and the body table of the repo, and the
// decompiled C#; tests/data/wp37_reference.json), from the decompiled C# (the derivation is in the comment above the check) or from the
// physprofile save of testdata (it has no ContractSystem SCENARIO: asserted below).
//
// Fixtures that are not KSP data (STATUS.md "needs data"): the GameVariables contract / mentality factors (the C# field initialisers; the
// Unity prefab is not in the repo), the two Career multiplier sets, the progress scenario of the generated trees, the generation GUIDs, the
// constant reputation curves (WP-36 fixture).
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <set>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/career/career.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/contracts.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/misc_util.hpp"
#include "ksp/vessel/vessel.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::Contracts;
using GE = events::GameEvents;
using Localizer37 = KSP::Localization::Localizer;
using json = nlohmann::json;

namespace {

const std::string kSaves37 = std::string(KSPCORE_TESTDATA_DIR) + "/saves";
const std::string kHarness37 = KSPCORE_HARNESS_DIR;

const json& Ref37() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp37_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

uint32_t Bits37(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
uint64_t BitsD37(double d) {
    uint64_t u;
    std::memcpy(&u, &d, 8);
    return u;
}
float F37(const json& j) { return j.get<float>(); }
// 0 ULP: the float / double must be the reference's exactly
#define CHECK_F32(got, ref) CHECK_MESSAGE(Bits37(static_cast<float>(got)) == Bits37(static_cast<float>(ref)), "got ", (got), " want ", (ref))
#define CHECK_F64(got, ref) CHECK_MESSAGE(BitsD37(static_cast<double>(got)) == BitsD37(static_cast<double>(ref)), "got ", (got), " want ", (ref))

std::string GameData37() { return ksp::testing::GameRoot() + "/GameData"; }

void LoadLocalizer37() {
    Localizer37::Shutdown();
    Localizer37::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(GameData37() + "/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    Localizer37::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

// A contract with no behaviour of its own: the base class machinery under test
class ProbeContract : public Contract {
public:
    std::string TypeName() const override { return "ProbeContract"; }
    bool MeetRequirements() override { return true; }
    std::vector<CelestialBody*> bodies;
    std::vector<CelestialBody*> WeightBodies() override { return bodies; }
    bool HasWeightBodies() override { return !bodies.empty(); }
    std::string GetTitle() override { return "Probe contract"; }
    std::string GetSynopsys() override { return "probe"; }
    // the Generate of a probe: one funds / reputation / science set-up
    bool Generate() override {
        SetExpiry();
        SetDeadlineDays(3.0f);
        SetFunds(1000.0f, 4000.0f, 2500.0f);
        SetReputation(5.0f, 2.0f);
        SetScience(3.0f);
        return true;
    }
    void Poke(State s, Viewed v) {
        state = s;
        viewed = v;
    }
};

// The stock game database + the Kerbol system + a CAREER game + GameVariables + the agents / story / contract definitions
struct World37 {
    Game15 g;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    Game game;
    std::unique_ptr<GameVariables> gv;
    std::unique_ptr<Agents::AgentList> agents;
    std::unique_ptr<ProgressTracking> pt;
    World37() {
        FlightRegistry::Clear();
        CareerEnvironment::Reset();
        ContractsEnvironment::Reset();
        ContractSystem::ClearTypes();
        ContractSystem::ContractWeights.reset();
        ScenarioUpgradeableFacilities::ClearStatics();
        SaveEnvironment& env = SaveEnvironment::Current();
        pg.physicsDatabaseFilename = kHarness37 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        game.Mode = Game::Modes::CAREER;
        game.flightState = std::make_shared<FlightState>();  // HighLogic.CurrentGame.flightState (no vessels)
        env.currentGame = &game;
        LoadLocalizer37();
        // CelestialBody.Awake (CelestialBody.cs:468): bodyDisplayName = Localizer.Format(bodyDisplayName)
        for (CelestialBody* b : sys->SpawnOrder()) b->bodyDisplayName = Localizer37::Format(b->bodyDisplayName);
        gv = std::make_unique<GameVariables>();
        gv->reputationAddition = AnimationCurve(std::vector<Keyframe>{Keyframe(0.0f, 0.9f)});
        gv->reputationSubtraction = AnimationCurve(std::vector<Keyframe>{Keyframe(0.0f, 1.3f)});
        ContractDefsLite::Load(GameData37() + "/Squad/Contracts/Contracts.cfg");
        // GameDatabase: the story definitions are translated at load (Localizer.TranslateBranch)
        ConfigNode::NodePtr story = ConfigNode::Load(GameData37() + "/Squad/Resources/StoryDefs.cfg");
        REQUIRE(story);
        ConfigNode::NodePtr sd = story->GetNode("STORY_DEF")->CreateCopy();
        Localizer37::TranslateBranch(*sd);
        TextGen::Reset();
        TextGen::Setup(*sd);
        agents = std::make_unique<Agents::AgentList>();
        ConfigNode::NodePtr ag = ConfigNode::Load(GameData37() + "/Squad/Agencies/Agents.cfg");
        REQUIRE(ag);
        agents->LoadAgents(ag->GetNodes("AGENT"));
        ContractSystem::RegisterStockTypes();
        pt = std::make_unique<ProgressTracking>();
        pt->Awake();
        pt->Start();
        Planetarium::fetch->time = 0.0;
        // deterministic Guid.NewGuid (the generated contracts hash it: SystemUtilities.SuperSeed)
        ContractsEnvironment::Current().newGuid = [this] { return nextGuid; };
        ContractsEnvironment::Current().nowTicks = [] { return int64_t(636000000000000000LL); };
    }
    ~World37() {
        agents.reset();
        pt->OnDestroy();  // ProgressTracking.OnDestroy: Stow the achievement nodes (their GameEvents subscriptions) before the tree dies
        pt.reset();
        gv.reset();
        ContractSystem::ClearTypes();
        ContractSystem::ContractWeights.reset();
        ContractSystem::Instance = nullptr;
        ContractsEnvironment::Reset();
        ContractDefsLite::Clear();
        TextGen::Reset();
        CareerEnvironment::Reset();
        ScenarioUpgradeableFacilities::ClearStatics();
        SaveEnvironment::Current().currentGame = nullptr;
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
    }
    Guid nextGuid;
    CelestialBody& Body(const char* n) { return *sys->GetBodyByName(n); }
    GameParameters::CareerParams& Career() { return game.Parameters->Career; }
    void SetMult(const json& m) {
        Career().FundsGainMultiplier = F37(m["fundsGain"]);
        Career().FundsLossMultiplier = F37(m["fundsLoss"]);
        Career().RepGainMultiplier = F37(m["repGain"]);
        Career().RepLossMultiplier = F37(m["repLoss"]);
        Career().ScienceGainMultiplier = F37(m["sciGain"]);
    }
};

}  // namespace

// ---- UnityEngine.Random cross-check (the generator the contract maths draws from) ------------------------------------------------------------
TEST_CASE("WP-37 reference: UnityEngine.Random Range(int) / Range(float) agree with the Python model") {
    int rows = 0;
    for (const json& r : Ref37()["rng"]) {
        UnityRandom u(r["seed"].get<int32_t>());
        const auto& ints = r["ints"];
        for (int i = 0; i < 6; ++i) CHECK(u.Range(0, 1000) == ints[static_cast<size_t>(i)].get<int>());
        for (int i = 0; i < 3; ++i) CHECK(u.Range(1, 7) == ints[static_cast<size_t>(6 + i)].get<int>());
        UnityRandom u2(r["seed"].get<int32_t>());
        for (int i = 0; i < 4; ++i) CHECK_F32(u2.Range(0.0f, 2.0f), F37(r["floats"][static_cast<size_t>(i)]));
        ++rows;
    }
    CHECK(rows == 7);
}

// ---- Contract.SetFunds / SetReputation / SetScience / SetDeadline* (Contract.cs) ---------------------------------------------------------
TEST_CASE("WP-37 Contract reward and deadline maths vs the reference (0 ULP)") {
    World37 w;
    int rows = 0;
    for (const json& r : Ref37()["rewards"]) {
        w.SetMult(Ref37()["mult_sets"][r["mult"].get<size_t>()]);
        ProbeContract c;
        c.prestige = static_cast<ContractPrestige>(r["prestige"].get<int>());
        CelestialBody* body = r["body"].is_null() ? nullptr : &w.Body(r["body"].get<std::string>().c_str());
        const auto& in = r["in"];
        c.SetFunds(F37(in[0]), F37(in[1]), F37(in[2]), body);
        c.SetReputation(F37(in[3]), F37(in[4]), body);
        c.SetScience(F37(in[5]), body);
        INFO("row " << rows << " prestige " << r["prestige"] << " body " << r["body"]);
        CHECK_F64(c.FundsAdvance, r["fa"].get<double>());
        CHECK_F64(c.FundsCompletion, r["fc"].get<double>());
        CHECK_F64(c.FundsFailure, r["ff"].get<double>());
        CHECK_F32(c.ReputationCompletion, F37(r["rc"]));
        CHECK_F32(c.ReputationFailure, F37(r["rf"]));
        CHECK_F32(c.ScienceCompletion, F37(r["sc"]));
        c.SetDeadlineDays(852.0f, body);
        CHECK_F64(c.TimeDeadline, r["daysDeadline"].get<double>());
        c.SetDeadlineYears(2.5f, body);
        CHECK_F64(c.TimeDeadline, r["yearsDeadline"].get<double>());
        ProbeContract c3;
        c3.prestige = c.prestige;
        c3.SetFunds(F37(in[0]), F37(in[1]), body);
        CHECK_F64(c3.FundsAdvance, r["fa2"].get<double>());
        CHECK_F64(c3.FundsCompletion, r["fc2"].get<double>());
        CHECK_F64(c3.FundsFailure, r["ff2"].get<double>());
        ++rows;
    }
    CHECK(rows == 126);
}

TEST_CASE("WP-37 Contract.SetExpiry (int and float draws) vs the reference") {
    World37 w;
    for (const json& r : Ref37()["expiry"]) {
        ProbeContract c;
        UnityRandom::Global().InitState(r["seed"].get<int>());
        c.SetExpiry(1, 7);
        CHECK(c.TimeExpiry >= 0.0);
        CHECK_F64(c.TimeExpiry, r["expiry"].get<double>());
        UnityRandom::Global().InitState(r["seed"].get<int>());
        c.SetExpiry(1.0f, 7.0f);
        CHECK_F64(c.TimeExpiry, r["expiryF"].get<double>());
    }
}

// ---- the agent mentalities ---------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-37 agent mentalities (Factor* helpers, CanProcessContract, ScoreKeyword) vs the reference") {
    World37 w;
    int rows = 0;
    for (const json& r : Ref37()["mentality"]) {
        const std::string name = r["name"].get<std::string>();
        Agents::Agent a("Test Agent", "Test Agent", "", nullptr);
        Agents::AgentMentality* m = Agents::AgentMentality::AddMentality(a, name, F37(r["factor"]));
        REQUIRE(m != nullptr);
        CHECK(m->TypeName() == name);
        ProbeContract c;
        c.prestige = static_cast<ContractPrestige>(r["prestige"].get<int>());
        c.AddKeywords({"Commercial"});
        c.FundsAdvance = 10000.0;
        c.FundsCompletion = 25000.0;
        c.FundsFailure = 12000.0;
        c.ReputationCompletion = 20.0f;
        c.ReputationFailure = 10.0f;
        c.ScienceCompletion = 8.0f;
        c.TimeDeadline = 1e6;
        c.TimeExpiry = 50000.0;
        if (m->CanProcessContract(c)) m->ProcessContract(c);
        const json& o = r["out"];
        INFO(name << " prestige " << r["prestige"] << " factor " << r["factor"]);
        CHECK_F64(c.FundsAdvance, o["fa"].get<double>());
        CHECK_F64(c.FundsCompletion, o["fc"].get<double>());
        CHECK_F64(c.FundsFailure, o["ff"].get<double>());
        CHECK_F32(c.ReputationCompletion, F37(o["rc"]));
        CHECK_F32(c.ReputationFailure, F37(o["rf"]));
        CHECK_F32(c.ScienceCompletion, F37(o["sc"]));
        CHECK_F64(c.TimeDeadline, o["deadline"].get<double>());
        CHECK_F64(c.TimeExpiry, o["expiry"].get<double>());
        ProbeContract c2;
        c2.AddKeywords({"Commercial"});
        c2.AddKeywords({"Record"});
        CHECK(m->CanProcessContract(c2) == r["can"].get<bool>());
        ProbeContract c3;
        c3.AddKeywords({"Record"});
        c3.AddKeywordsRequired({"Record"});
        CHECK(m->CanProcessContract(c3) == r["canRecord"].get<bool>());
        CHECK(static_cast<int>(m->ScoreKeyword("Commercial")) == r["scoreCommercial"].get<int>());
        CHECK(static_cast<int>(m->ScoreKeyword("Record")) == r["scoreRecord"].get<int>());
        CHECK(static_cast<int>(m->ScoreKeyword("Scientific")) == r["scoreScientific"].get<int>());
        ++rows;
    }
    CHECK(rows == 19 * 3 * 4);
}

TEST_CASE("WP-37 Agents.cfg: 27 agents load, the two default mentalities and the standings link") {
    World37 w;
    auto& list = w.agents->Agencies();
    const json& ref = Ref37()["agents"];
    REQUIRE(list.size() == ref.size());
    for (size_t i = 0; i < list.size(); ++i) {
        CHECK(list[i]->Name() == ref[i]["name"].get<std::string>());
        CHECK(list[i]->Title() == ref[i]["title"].get<std::string>());
        const auto& ments = ref[i]["mentality"];
        REQUIRE(list[i]->Mentality().size() == ments.size());
        for (size_t j = 0; j < ments.size(); ++j) {
            CHECK(list[i]->Mentality()[j]->TypeName() == ments[j][0].get<std::string>());
            CHECK_F32(list[i]->Mentality()[j]->Factor(), F37(ments[j][1]));
        }
    }
    CHECK(w.agents->GetAgent("Kerbodyne") != nullptr);
    CHECK(w.agents->GetAgentbyTitle("Kerbodyne") != nullptr);
    CHECK(w.agents->GetAgentbyTitle("no such agency") == nullptr);
}

// ---- ContractSystem.GetContractCounts -----------------------------------------------------------------------------------------------------
TEST_CASE("WP-37 ContractSystem.GetContractCounts vs the reference") {
    int rows = 0;
    for (const json& r : Ref37()["counts"]) {
        int t1 = -1, t2 = -1, t3 = -1;
        ContractSystem::GetContractCounts(F37(r["rep"]), r["avg"].get<int>(), t1, t2, t3);
        INFO("rep " << r["rep"] << " avg " << r["avg"]);
        CHECK(t1 == r["tiers"][0].get<int>());
        CHECK(t2 == r["tiers"][1].get<int>());
        CHECK(t3 == r["tiers"][2].get<int>());
        ++rows;
    }
    CHECK(rows == 3 * 45);
}

// ---- generation weights --------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-37 ContractSystem weights after accept / decline / withdraw / complete vs the reference") {
    World37 w;
    ContractSystem cs;
    cs.Awake();
    ConfigNode node("SCENARIO");
    cs.LoadNow(node);
    const json& wr = Ref37()["weights"];
    CHECK(ContractSystem::WeightDefaultValue() == wr["default"].get<int>());
    CHECK(ContractSystem::WeightMinimumValue() == wr["min"].get<int>());
    CHECK(ContractSystem::WeightMaximumValue() == wr["max"].get<int>());
    CHECK(ContractSystem::TryGetWeightValue("PlantFlag") == wr["default"].get<int>());
    CHECK(ContractSystem::TryGetWeightValue("Mun") == wr["default"].get<int>());
    CHECK(ContractSystem::TryGetWeightValue("noSuchName") == 0);  // Dictionary.TryGetValue leaves default(int)
    CelestialBody* mun = &w.Body("Mun");
    for (const json& s : wr["steps"]) {
        ProbeContract c;
        c.bodies = {mun};
        const std::string op = s["op"].get<std::string>();
        if (op == "accept") c.Poke(Contract::State::Active, Contract::Viewed::Unseen);
        else if (op == "decline") c.Poke(Contract::State::Declined, Contract::Viewed::Unseen);
        else if (op == "complete") c.Poke(Contract::State::Completed, Contract::Viewed::Unseen);
        else if (op == "withdrawRead") c.Poke(Contract::State::Withdrawn, Contract::Viewed::Read);
        else c.Poke(Contract::State::Withdrawn, Contract::Viewed::Seen);
        ContractSystem::AdjustWeight(s["type"].get<std::string>(), &c);
        ContractSystem::AdjustWeight("Mun", &c);
        INFO(s["type"] << " " << op);
        CHECK(ContractSystem::TryGetWeightValue("PlantFlag") == s["PlantFlag"].get<int>());
        CHECK(ContractSystem::TryGetWeightValue("CollectScience") == s["CollectScience"].get<int>());
        CHECK(ContractSystem::TryGetWeightValue("Mun") == s["Mun"].get<int>());
    }
    // an unknown name is not adjusted (WeightAdjustment: TryGetWeight fails)
    ProbeContract c;
    c.Poke(Contract::State::Active, Contract::Viewed::Unseen);
    ContractSystem::AdjustWeight("noSuchName", &c);
    CHECK(ContractSystem::TryGetWeightValue("noSuchName") == 0);
    ContractSystem::WeightAssignment("Mun", 1000);
    CHECK(ContractSystem::TryGetWeightValue("Mun") == wr["max"].get<int>());
    ContractSystem::WeightAssignment("Mun", 1000, true);
    CHECK(ContractSystem::TryGetWeightValue("Mun") == 1000);
    ContractSystem::ResetWeights();
    CHECK(ContractSystem::TryGetWeightValue("Mun") == wr["default"].get<int>());
}

// ---- TextGen --------------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-37 TextGen back stories (StoryDefs.cfg, Unity Random) are byte-exact vs the reference") {
    World37 w;
    int rows = 0;
    for (const json& r : Ref37()["text"]) {
        const auto& allow = r["allow"];
        const std::string got = TextGen::GenerateBackStories(r["type"].get<std::string>(), r["agentTitle"].get<std::string>(), r["topic"].get<std::string>(),
                                                             r["subject"].get<std::string>(), r["seed"].get<int>(), allow[0].get<bool>(), allow[1].get<bool>(),
                                                             allow[2].get<bool>());
        INFO(r["type"] << " seed " << r["seed"]);
        CHECK(got == r["text"].get<std::string>());
        ++rows;
    }
    CHECK(rows == 9 * 7 * 3);
}

// ---- generated contracts (PlantFlag, CollectScience): the whole Generate path vs the reference ------------------------------------------------
namespace {

void LoadScenario(World37& w, const json& scenario) {
    ConfigNode scen("SCENARIO");
    ConfigNode* prog = scen.AddNode("Progress");
    for (auto it = scenario["progress"].begin(); it != scenario["progress"].end(); ++it) {
        ConfigNode* b = prog->AddNode(it.key());
        for (auto jt = it.value().begin(); jt != it.value().end(); ++jt) {
            ConfigNode* n = b->AddNode(jt.key());
            n->AddValue(jt.value().get<std::string>(), 0.0);
        }
    }
    w.pt->OnLoad(scen);
}

ConfigNode WeightsNode(const json& scenario) {
    ConfigNode node("SCENARIO");
    ConfigNode* wn = node.AddNode("WEIGHTS");
    for (auto it = scenario["weights"].begin(); it != scenario["weights"].end(); ++it) wn->AddValue(it.key(), static_cast<int32_t>(it.value().get<int>()));
    return node;
}

Guid GuidOf(const json& bytes) {
    Guid g;
    for (size_t i = 0; i < 16; ++i) g.bytes[i] = static_cast<uint8_t>(bytes[i].get<int>());
    return g;
}

Contract::ContractPrestige PrestigeOf(const std::string& s) {
    if (s == "Trivial") return ContractPrestige::Trivial;
    if (s == "Significant") return ContractPrestige::Significant;
    return ContractPrestige::Exceptional;
}

}  // namespace

TEST_CASE("WP-37 generated PlantFlag / CollectScience contracts: tree, texts, rewards, agent, id vs the reference (0 ULP, byte-exact)") {
    World37 w;
    ContractSystem cs;
    cs.Awake();
    const json& ref = Ref37();
    ConfigNode weights = WeightsNode(ref["scenario"]);
    cs.LoadNow(weights);
    LoadScenario(w, ref["scenario"]);
    int generated = 0, nulls = 0;
    for (const json& e : ref["trees"]) {
        w.SetMult(ref["mult_sets"][e["mult"].get<size_t>()]);
        w.nextGuid = GuidOf(e["guid"]);
        const std::string type = e["type"].get<std::string>();
        std::unique_ptr<Contract> c = cs.GenerateContract(e["seed"].get<int>(), PrestigeOf(e["prestige"].get<std::string>()), type);
        INFO(type << " " << e["prestige"] << " seed " << e["seed"] << " mult " << e["mult"]);
        const json& r = e["result"];
        if (r.is_null()) {
            CHECK(c == nullptr);
            ++nulls;
            continue;
        }
        REQUIRE(c != nullptr);
        ++generated;
        CHECK(c->Agent()->Name() == r["agent"].get<std::string>());
        CHECK(c->Agent()->Title() == r["agentTitle"].get<std::string>());
        CHECK(c->Keywords() == r["keywords"].get<std::vector<std::string>>());
        CHECK(c->Title() == r["title"].get<std::string>());
        CHECK(c->Synopsys() == r["synopsis"].get<std::string>());
        CHECK(c->Description() == r["description"].get<std::string>());
        CHECK(c->ContractID() == r["contractID"].get<long long>());
        CHECK_F64(c->FundsAdvance, r["fa"].get<double>());
        CHECK_F64(c->FundsCompletion, r["fc"].get<double>());
        CHECK_F64(c->FundsFailure, r["ff"].get<double>());
        CHECK_F32(c->ReputationCompletion, F37(r["rc"]));
        CHECK_F32(c->ReputationFailure, F37(r["rf"]));
        CHECK_F32(c->ScienceCompletion, F37(r["sc"]));
        CHECK_F64(c->TimeExpiry, r["expiry"].get<double>());
        CHECK_F64(c->TimeDeadline, r["deadline"].get<double>());
        REQUIRE(c->ParameterCount() == static_cast<int>(r["params"].size()));
        for (int i = 0; i < c->ParameterCount(); ++i) {
            ContractParameter* p = c->GetParameter(i);
            const json& pr = r["params"][static_cast<size_t>(i)];
            CHECK(p->TypeName() == pr["type"].get<std::string>());
            CHECK(p->HashString() == pr["hash"].get<std::string>());
            CHECK(p->Title() == pr["title"].get<std::string>());
            if (auto* pf = dynamic_cast<Parameters::PlantFlag*>(p)) CHECK(pf->TargetBody()->name() == pr["body"].get<std::string>());
            if (auto* cs2 = dynamic_cast<Parameters::CollectScience*>(p)) CHECK(cs2->TargetBody()->name() == pr["body"].get<std::string>());
        }
        if (type == "CollectScience") {
            auto* t = dynamic_cast<Templates::CollectScience*>(c.get());
            REQUIRE(t != nullptr);
            CHECK(dotnet::EnumToString(t->TargetLocation()) == r["location"].get<std::string>());
        }
        CHECK(c->ContractState() == Contract::State::Generated);
        CHECK(c->MissionSeed() == e["seed"].get<int>());
    }
    CHECK(generated + nulls == 480);
    CHECK(generated > 0);
    CHECK(nulls > 0);
}

// ---- the state machine, deadlines, awards and penalties through the WP-36 currency pipeline ---------------------------------------------------
namespace {

// Reputation.addReputation_granular with constant curves, restated from the C# (Mono float rule), as in the WP-36 tests
float GranularRef37(float rep, float value, float addC, float subC) {
    const int num = static_cast<int>(std::fabs(value));
    const float delta = static_cast<float>(1.0 * (value >= 0.0f ? 1.0 : -1.0));
    float num2 = 0.0f;
    for (int i = 0; i <= num; ++i) {
        const float d = (i == num) ? static_cast<float>(fx(value) - fx(num2)) : delta;
        const float num3 = static_cast<float>(fx(d) * fx(d < 0.0f ? subC : addC));
        rep = static_cast<float>(fx(rep) + fx(num3));
        num2 = static_cast<float>(fx(num2) + fx(num3));
    }
    return rep;
}

struct Life37 {
    World37 w;
    Funding funds;
    Reputation rep;
    ContractSystem cs;
    float science = 0.0f;
    int scienceCalls = 0;
    std::vector<std::string> events;
    Life37() {
        funds.Awake();
        rep.Awake();
        rep.Start();
        funds.SetFunds(1000000.0, TransactionReasons::Cheating);
        rep.SetReputation(0.0f, TransactionReasons::Cheating);
        CareerEnvironment& env = CareerEnvironment::Current();
        env.rndPresent = [] { return true; };
        env.rndAddScience = [this](float v, TransactionReasons) {
            science += v;
            ++scienceCalls;
        };
        cs.Awake();
        ConfigNode weights = WeightsNode(Ref37()["scenario"]);
        cs.LoadNow(weights);
        LoadScenario(w, Ref37()["scenario"]);
        w.SetMult(Ref37()["mult_sets"][0]);
        GE::Contract::onOffered.Add(this, &Life37::OnOffered);
        GE::Contract::onAccepted.Add(this, &Life37::OnAccepted);
        GE::Contract::onCompleted.Add(this, &Life37::OnCompleted);
        GE::Contract::onFailed.Add(this, &Life37::OnFailed);
        GE::Contract::onCancelled.Add(this, &Life37::OnCancelled);
        GE::Contract::onDeclined.Add(this, &Life37::OnDeclined);
        GE::Contract::onFinished.Add(this, &Life37::OnFinished);
    }
    ~Life37() {
        GE::Contract::onOffered.Remove(this, &Life37::OnOffered);
        GE::Contract::onAccepted.Remove(this, &Life37::OnAccepted);
        GE::Contract::onCompleted.Remove(this, &Life37::OnCompleted);
        GE::Contract::onFailed.Remove(this, &Life37::OnFailed);
        GE::Contract::onCancelled.Remove(this, &Life37::OnCancelled);
        GE::Contract::onDeclined.Remove(this, &Life37::OnDeclined);
        GE::Contract::onFinished.Remove(this, &Life37::OnFinished);
    }
    void OnOffered(Contract*) { events.push_back("offered"); }
    void OnAccepted(Contract*) { events.push_back("accepted"); }
    void OnCompleted(Contract*) { events.push_back("completed"); }
    void OnFailed(Contract*) { events.push_back("failed"); }
    void OnCancelled(Contract*) { events.push_back("cancelled"); }
    void OnDeclined(Contract*) { events.push_back("declined"); }
    void OnFinished(Contract*) { events.push_back("finished"); }
    // the first contract of the reference trees that generated for (type, prestige) with multiplier set 0
    std::unique_ptr<Contract> Make(const char* type, const char* prestige) {
        for (const json& e : Ref37()["trees"]) {
            if (e["mult"].get<int>() != 0 || e["type"].get<std::string>() != type || e["prestige"].get<std::string>() != prestige || e["result"].is_null()) continue;
            w.nextGuid = GuidOf(e["guid"]);
            auto c = cs.GenerateContract(e["seed"].get<int>(), PrestigeOf(prestige), type);
            REQUIRE(c != nullptr);
            return c;
        }
        FAIL("no reference contract");
        return nullptr;
    }
};

}  // namespace

TEST_CASE("WP-37 Contract state machine: Offer / Accept / Complete move funds, reputation and weights exactly") {
    Life37 L;
    auto c = L.Make("PlantFlag", "Significant");
    Contract* raw = c.get();
    const double advance = c->FundsAdvance, completion = c->FundsCompletion;
    const float repC = c->ReputationCompletion;
    Planetarium::fetch->time = 1000.0;
    const int wBefore = ContractSystem::TryGetWeightValue("PlantFlag");
    L.cs.contracts.push_back(std::move(c));
    CHECK(raw->Offer());
    CHECK_FALSE(raw->Offer());  // only Generated -> Offered
    CHECK(raw->ContractState() == Contract::State::Offered);
    CHECK_F64(raw->DateExpire(), 1000.0 + raw->TimeExpiry);
    CHECK(raw->Accept());
    CHECK(raw->ContractState() == Contract::State::Active);
    CHECK_F64(raw->DateAccepted(), 1000.0);
    CHECK_F64(raw->DateDeadline(), 1000.0 + raw->TimeDeadline);
    CHECK(L.funds.Funds() == 1000000.0 + advance);  // AwardAdvance
    CHECK(ContractSystem::TryGetWeightValue("PlantFlag") == std::min(wBefore + 12, 90));
    auto* pf = dynamic_cast<Templates::PlantFlag*>(raw);
    REQUIRE(pf != nullptr);
    const std::string bodyName = pf->TargetBody()->name();
    CHECK(ContractSystem::TryGetWeightValue(bodyName) <= 90);
    Planetarium::fetch->time = 5000.0;
    CHECK(raw->Complete());
    CHECK(raw->ContractState() == Contract::State::Completed);
    CHECK_F64(raw->DateFinished(), 5000.0);
    CHECK(L.funds.Funds() == 1000000.0 + advance + completion);
    CHECK_F32(L.rep.reputation(), GranularRef37(0.0f, repC, 0.9f, 1.3f));
    CHECK(L.scienceCalls == 0);  // ScienceCompletion is 0 for PlantFlag
    CHECK(ContractSystem::TryGetWeightValue("PlantFlag") == std::min(wBefore + 12, 90));  // Completed does not adjust
    CHECK(L.events == std::vector<std::string>{"offered", "accepted", "completed", "finished"});
    CHECK_FALSE(raw->Complete());  // not Active any more
    CHECK_FALSE(raw->Fail());
    CHECK(raw->IsFinished());
}

TEST_CASE("WP-37 Contract failure") {
    Life37 L;
    auto c = L.Make("PlantFlag", "Significant");
    Contract* raw = c.get();
    L.cs.contracts.push_back(std::move(c));
    const double failure = raw->FundsFailure, advance = raw->FundsAdvance;
    const float repF = raw->ReputationFailure;
    raw->Offer();
    raw->Accept();
    const double fundsAfterAccept = L.funds.Funds();
    CHECK(fundsAfterAccept == 1000000.0 + advance);
    CHECK(raw->Fail());
    CHECK(raw->ContractState() == Contract::State::Failed);
    CHECK(L.funds.Funds() == fundsAfterAccept - std::fabs(failure));  // PenalizeFailure
    CHECK_F32(L.rep.reputation(), GranularRef37(0.0f, -std::fabs(repF), 0.9f, 1.3f));
    CHECK(L.events.back() == "finished");
}

TEST_CASE("WP-37 Contract cancellation") {
    Life37 L;
    auto c = L.Make("PlantFlag", "Significant");
    Contract* raw = c.get();
    L.cs.contracts.push_back(std::move(c));
    Planetarium::fetch->time = 100.0;
    raw->Offer();
    raw->Accept();
    const double acc = raw->DateAccepted();
    Planetarium::fetch->time = acc + raw->TimeDeadline * 0.5;
    const double before = L.funds.Funds();
    CHECK(raw->Cancel());
    CHECK(raw->ContractState() == Contract::State::Cancelled);
    // PenalizeCancellation: num = InverseLerp(accepted, deadline, finished); funds = Lerp(advance, failure, num)
    const float num = Mathf::InverseLerp(static_cast<float>(raw->DateAccepted()), static_cast<float>(raw->DateDeadline()), static_cast<float>(raw->DateFinished()));
    const double funds = Mathfx::Lerp(raw->FundsAdvance, raw->FundsFailure, static_cast<double>(num));
    CHECK(L.funds.Funds() == before - std::fabs(funds));
    const float repLoss = Mathfx::Lerp(L.w.Career().RepLossDeclined, raw->ReputationFailure, num);
    CHECK_F32(L.rep.reputation(), GranularRef37(0.0f, -std::fabs(repLoss), 0.9f, 1.3f));
}

TEST_CASE("WP-37 Contract decline") {
    Life37 L;
    auto c = L.Make("PlantFlag", "Significant");
    Contract* raw = c.get();
    L.cs.contracts.push_back(std::move(c));
    const int wBefore = ContractSystem::TryGetWeightValue("PlantFlag");
    raw->Offer();
    CHECK(raw->Decline());
    CHECK(raw->ContractState() == Contract::State::Declined);
    CHECK(ContractSystem::TryGetWeightValue("PlantFlag") == std::max(wBefore - 8, 10));
    // Reputation.AddReputation(-RepLossDeclined = -1) with ContractDecline
    CHECK_F32(L.rep.reputation(), GranularRef37(0.0f, -1.0f, 0.9f, 1.3f));
    CHECK_FALSE(raw->Decline());
}

TEST_CASE("WP-37 Contract withdraw") {
    Life37 L;
    auto c = L.Make("PlantFlag", "Significant");
    Contract* raw = c.get();
    L.cs.contracts.push_back(std::move(c));
    const int wBefore = ContractSystem::TryGetWeightValue("PlantFlag");
    raw->Offer();
    raw->SetViewed(Contract::Viewed::Read);
    raw->SetViewed(Contract::Viewed::Seen);  // never goes back
    CHECK(raw->ContractViewed() == Contract::Viewed::Read);
    raw->Withdraw();
    CHECK(raw->ContractState() == Contract::State::Withdrawn);
    CHECK(ContractSystem::TryGetWeightValue("PlantFlag") == std::max(wBefore - 2, 10));
    raw->Withdraw();  // idempotent
    CHECK(raw->IsFinished());
}

TEST_CASE("WP-37 Contract Update expiry") {
    // Update(): an Offered contract expires, an Active contract passes its deadline (UpdateContracts then archives it)
    Life37 L;
    auto c = L.Make("PlantFlag", "Significant");
    Contract* raw = c.get();
    L.cs.contracts.push_back(std::move(c));
    Planetarium::fetch->time = 0.0;
    raw->Offer();
    Planetarium::fetch->time = raw->DateExpire() - 1.0;
    raw->Update();
    CHECK(raw->ContractState() == Contract::State::Offered);
    Planetarium::fetch->time = raw->DateExpire();
    raw->Update();
    CHECK(raw->ContractState() == Contract::State::OfferExpired);
    CHECK_F64(raw->DateFinished(), raw->DateExpire());
    auto c2 = L.Make("CollectScience", "Trivial");
    Contract* r2 = c2.get();
    L.cs.contracts.push_back(std::move(c2));
    Planetarium::fetch->time = 10.0;
    r2->Offer();
    r2->Accept();
    const double fundsBefore = L.funds.Funds();
    Planetarium::fetch->time = r2->DateDeadline();
    L.cs.UpdateContracts();  // updates the active ones (r2 passes its deadline here), archives the finished ones
    CHECK(r2->ContractState() == Contract::State::DeadlineExpired);
    L.cs.UpdateContracts();  // r2 is finished now: archived
    CHECK(L.funds.Funds() == fundsBefore - std::fabs(r2->FundsFailure));
    CHECK(L.cs.ContractsFinished().size() == 1);  // DeadlineExpired is archived; OfferExpired is not
    CHECK(L.cs.RequireRefresh());
}

TEST_CASE("WP-37 ContractSystem: generation order, duplicate ids, surplus withdrawal and the seed wrap") {
    Life37 L;
    int seed = 12345;
    const int start = seed;
    L.w.nextGuid = GuidOf(Ref37()["trees"][0]["guid"]);
    bool any = L.cs.GenerateContracts(seed, ContractPrestige::Trivial, 3);
    CHECK(seed > start);
    CHECK(any == (L.cs.Contracts().size() > 0));
    CHECK(L.cs.CountContracts(ContractPrestige::Trivial) == static_cast<int>(L.cs.Contracts().size()));
    for (auto& c : L.cs.Contracts()) CHECK(c->ContractState() == Contract::State::Offered);
    std::set<long long> ids;
    for (auto& c : L.cs.Contracts()) CHECK(ids.insert(c->ContractID()).second);
    const size_t n = L.cs.Contracts().size();
    if (n >= 2) {
        CHECK(L.cs.WithdrawSurplusContracts(ContractPrestige::Trivial, 1));
        int offered = 0;
        for (auto& c : L.cs.Contracts()) offered += c->ContractState() == Contract::State::Offered ? 1 : 0;
        CHECK(offered == 1);
        CHECK_FALSE(L.cs.WithdrawSurplusContracts(ContractPrestige::Trivial, 1));
    }
    int s2 = INT32_MAX;
    L.cs.GenerateContract(s2, ContractPrestige::Trivial);
    CHECK(s2 <= INT32_MIN + 60);
    L.cs.UpdateContracts();
    for (auto& c : L.cs.ContractsFinished()) CHECK(c->ContractState() != Contract::State::Withdrawn);
}

// ---- save / load -----------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-37 ContractSystem SCENARIO node (WEIGHTS, CONTRACTS, CONTRACT_FINISHED, update, version) round-trips byte-exact") {
    Life37 L;
    auto a = L.Make("PlantFlag", "Significant");
    auto b = L.Make("CollectScience", "Trivial");
    auto d = L.Make("CollectScience", "Significant");
    Contract* ra = a.get();
    Contract* rb = b.get();
    Contract* rd = d.get();
    L.cs.contracts.push_back(std::move(a));
    L.cs.contracts.push_back(std::move(b));
    L.cs.contracts.push_back(std::move(d));
    Planetarium::fetch->time = 2000.0;
    ra->Offer();
    ra->Accept();
    rb->Offer();
    rd->Offer();
    rd->Accept();
    rd->Complete();
    rb->SetViewed(Contract::Viewed::Seen);
    L.cs.UpdateContracts();  // rd -> CONTRACT_FINISHED
    L.cs.SetLastUpdate(1234.5678901234);
    ConfigNode saved("SCENARIO");
    saved.AddValue("name", "ContractSystem");
    L.cs.OnSave(saved);
    ConfigNode* contracts = saved.GetNode("CONTRACTS");
    REQUIRE(contracts != nullptr);
    CHECK(contracts->GetNodes("CONTRACT").size() == 2);
    CHECK(contracts->GetNodes("CONTRACT_FINISHED").size() == 1);
    CHECK(saved.GetNode("WEIGHTS") != nullptr);
    CHECK(saved.GetNode("WEIGHTS")->HasValue("PlantFlag"));
    CHECK(saved.GetNode("WEIGHTS")->HasValue("Mun"));
    ContractSystem cs2;
    // ContractSystem.LoadContract removes the `type` values from the node it loads (cNode.RemoveValues("type")): load from a copy
    ConfigNode::NodePtr loadFrom = saved.CreateCopy();
    cs2.LoadNow(*loadFrom);
    // ContractSystem.OnSave writes `update` to the SCENARIO node but OnLoadRoutine reads it from the CONTRACTS node (sic): it is not restored
    CHECK_F64(cs2.LastUpdate(), -1000.0);
    cs2.SetLastUpdate(1234.5678901234);
    ConfigNode again("SCENARIO");
    again.AddValue("name", "ContractSystem");
    cs2.OnSave(again);
    CHECK(again.ToString() == saved.ToString());
    CHECK(again.SaveToString() == saved.SaveToString());
    REQUIRE(cs2.Contracts().size() == 2);
    CHECK(cs2.Contracts()[0]->ContractState() == Contract::State::Active);
    CHECK(cs2.Contracts()[0]->ContractID() == ra->ContractID());
    CHECK(cs2.Contracts()[0]->ContractGuid() == ra->ContractGuid());
    CHECK(cs2.Contracts()[1]->ContractViewed() == Contract::Viewed::Seen);
    CHECK(ContractSystem::loaded);
}

// The SCENARIO nodes of the (sandbox) physprofile save that belong to this package: ScenarioCustomWaypoints (no WAYPOINT node) round-trips
// byte-exact through the live module; the save has NO ContractSystem node (career scenario): the contract format is exercised above on
// nodes this package writes itself (needs a career save: STATUS.md).
TEST_CASE("WP-37 physprofile save: ScenarioCustomWaypoints round-trips byte-exact, there is no ContractSystem SCENARIO") {
    World37 w;
    RegisterContractScenarioModules();
    ConfigNode::NodePtr save = ConfigNode::Load(kSaves37 + "/physprofile/persistent.sfs");
    REQUIRE(save);
    ConfigNode* game = save->GetNode("GAME");
    REQUIRE(game != nullptr);
    ScenarioRunner runner;
    std::set<std::string> names;
    int roundTripped = 0;
    for (ConfigNode* sn : game->GetNodes("SCENARIO")) {
        const std::string name = *sn->GetValue("name");
        names.insert(name);
        if (name != "ScenarioCustomWaypoints") continue;
        ProtoScenarioModule proto(*sn);
        ScenarioModule* m = proto.Load(runner);
        REQUIRE(m != nullptr);
        CHECK(m->ClassName() == "ScenarioCustomWaypoints");
        ProtoScenarioModule snap(*m);
        CHECK(snap.GetData().ToString() == sn->ToString());
        CHECK(snap.GetData().SaveToString() == sn->SaveToString());
        ++roundTripped;
    }
    CHECK(roundTripped == 1);
    CHECK(names.count("ContractSystem") == 0);
    CHECK(names.count("ProgressTracking") == 1);
}

// Synthetic WAYPOINT nodes (written from the C# OnSave format; the sandbox save has none): load, Waypoint fields, save again byte-exact
TEST_CASE("WP-37 ScenarioCustomWaypoints WAYPOINT nodes and ProtoWaypointInfo (format test)") {
    World37 w;
    ScenarioCustomWaypoints scw;
    scw.Awake();
    ConfigNode node("SCENARIO");
    node.AddValue("name", "ScenarioCustomWaypoints");
    node.AddValue("scene", "7, 8, 21");
    ConfigNode* wp = node.AddNode("WAYPOINT");
    wp->AddValue("name", "Base Alpha");
    wp->AddValue("celestialName", "Mun");
    wp->AddValue("latitude", 12.3456789012345);
    wp->AddValue("longitude", -45.5);
    wp->AddValue("navigationId", "0f8fad5b-d9cb-469f-a165-70867728950e");
    ConfigNode* wp2 = node.AddNode("WAYPOINT");
    wp2->AddValue("navigationId", "00000000-0000-0000-0000-000000000000");
    w.nextGuid.bytes[0] = 7;  // the Guid.NewGuid of the replaced empty id
    scw.Load(node);
    REQUIRE(scw.Waypoints().size() == 2);
    const FinePrint::Waypoint& a = *scw.Waypoints()[0];
    CHECK(a.name == "Base Alpha");
    CHECK(a.celestialName == "Mun");
    CHECK(a.latitude == 12.3456789012345);
    CHECK(a.seed == StringExtensions::GetHashCode_Net35("Base Alpha"));
    CHECK(a.id == "custom");
    CHECK(a.isCustom);
    CHECK(a.isNavigatable);
    CHECK(a.isOnSurface);
    const FinePrint::Waypoint& b = *scw.Waypoints()[1];
    CHECK(b.name == "Custom Waypoint");          // LoadNode default
    CHECK(b.celestialName == "Kerbin");           // FlightGlobals.GetHomeBodyName()
    CHECK_FALSE(b.navigationId == Guid::Empty());  // an empty stored id is replaced
    ConfigNode out("SCENARIO");
    scw.Save(out);
    ConfigNode* wo = out.GetNode("WAYPOINT");
    REQUIRE(wo != nullptr);
    CHECK(*wo->GetValue("navigationId") == "0f8fad5b-d9cb-469f-a165-70867728950e");
    CHECK(*wo->GetValue("latitude") == "12.3456789012345");
    CHECK(out.GetNodes("WAYPOINT").size() == 2);
    // ProtoWaypointInfo (WP-04 data side + the Waypoint members of this package)
    ProtoWaypointInfo info = MakeProtoWaypointInfo(&a);
    ConfigNode inode("WAYPOINT_INFO");
    info.Save(inode);
    CHECK(*inode.GetValue("type") == "Custom");
    CHECK(*inode.GetValue("navigationID") == a.navigationId.ToString());
    ProtoWaypointInfo back(inode);
    CHECK(back.navigationId == a.navigationId);
    ProtoWaypointInfo none = MakeProtoWaypointInfo(nullptr);
    ConfigNode nnode("WAYPOINT_INFO");
    none.Save(nnode);
    CHECK_FALSE(nnode.HasValue("type"));  // Type.Null saves nothing
    FinePrint::WaypointManagerSeam::findWaypoint = [&](const Guid& g) { return g == a.navigationId ? const_cast<FinePrint::Waypoint*>(&a) : nullptr; };
    CHECK(FindWaypoint(info) == &a);
    FinePrint::WaypointManagerSeam::findWaypoint = nullptr;
    CHECK(FindWaypoint(none) == nullptr);
}

// ---- parameters: logic nodes, events, envelopes -------------------------------------------------------------------------------------------------
namespace {

// A contract that holds an explicit parameter tree (no generation)
class TreeContract : public Contract {
public:
    std::string TypeName() const override { return "TreeContract"; }
    bool MeetRequirements() override { return true; }
    std::string GetTitle() override { return "tree"; }
};

}  // namespace

TEST_CASE("WP-37 OR / XOR / EnterOrbit / KerbalDeaths: parameter events complete and fail the contract") {
    Life37 L;
    CelestialBody& mun = L.w.Body("Mun");
    CelestialBody& minmus = L.w.Body("Minmus");
    // OR(EnterOrbit(Mun), EnterOrbit(Minmus)): either completes the OR, which completes the (only) top level parameter
    {
        TreeContract c;
        c.agent = L.w.agents->GetAgent("Kerbodyne");
        ContractParameter* orp = c.AddParameter(std::make_unique<Parameters::OR>());
        orp->AddParameter(std::make_unique<Parameters::EnterOrbit>(&mun));
        orp->AddParameter(std::make_unique<Parameters::EnterOrbit>(&minmus));
        c.state = Contract::State::Offered;
        CHECK(c.Accept());
        CHECK(orp->State() == ParameterState::Incomplete);
        GE::VesselSituation::onOrbit.Fire(nullptr, &minmus);
        CHECK(orp->GetParameter(1)->State() == ParameterState::Complete);
        CHECK(orp->GetParameter(0)->State() == ParameterState::Incomplete);
        CHECK(orp->State() == ParameterState::Complete);
        CHECK(c.ContractState() == Contract::State::Completed);
        CHECK(orp->HashString() == "ORMunMinmus");
    }
    // XOR: exactly one complete child -> Complete; a second one makes it Incomplete again
    {
        TreeContract c;
        c.agent = L.w.agents->GetAgent("Kerbodyne");
        ContractParameter* x = c.AddParameter(std::make_unique<Parameters::XOR>());
        x->AddParameter(std::make_unique<Parameters::EnterSOI>(&mun));
        x->AddParameter(std::make_unique<Parameters::EnterSOI>(&minmus));
        x->SetDisableOnStateChange(false);
        c.state = Contract::State::Offered;
        c.Accept();
        GE::VesselSituation::onFlyBy.Fire(nullptr, &mun);
        CHECK(x->State() == ParameterState::Complete);
        CHECK(c.ContractState() == Contract::State::Completed);
    }
    // KerbalDeaths(2): starts Complete, the second CREW_KILLED fails it and the contract (AllowPartialFailure is false)
    {
        TreeContract c;
        c.agent = L.w.agents->GetAgent("Kerbodyne");
        ContractParameter* kd = c.AddParameter(std::make_unique<Parameters::KerbalDeaths>(2));
        CHECK(kd->State() == ParameterState::Complete);
        CHECK(kd->Title() == Localizer37::Format("#autoLOC_269684", {"2"}));
        c.state = Contract::State::Offered;
        c.Accept();
        EventReport report(FlightEvents::CREW_KILLED, nullptr, "Jeb");
        GE::onCrewKilled.Fire(&report);
        CHECK(kd->State() == ParameterState::Complete);
        GE::onCrewKilled.Fire(&report);
        CHECK(kd->State() == ParameterState::Failed);
        CHECK(c.ContractState() == Contract::State::Failed);
        ConfigNode n("PARAM");
        kd->Save(n);
        CHECK(*n.GetValue("count") == "2");
        CHECK(*n.GetValue("countMax") == "2");
        CHECK(*n.GetValue("state") == "Failed");
    }
    // events of an unregistered parameter do nothing: a finished contract unregisters its parameters
    GE::VesselSituation::onOrbit.Fire(nullptr, &mun);
}

TEST_CASE("WP-37 ReachSituation / ReachSpeedEnvelope / ReachAltitudeEnvelope / ReachFlightEnvelope / ReachDestination checks") {
    Life37 L;
    Vessel v;
    CelestialBody& kerbin = L.w.Body("Kerbin");
    Parameters::ReachSituation landed(Vessel::Situations::LANDED, "Be ");
    v.situation = Vessel::Situations::PRELAUNCH;
    CHECK(landed.checkVesselSituation(&v));  // LANDED accepts PRELAUNCH
    v.situation = Vessel::Situations::LANDED;
    CHECK(landed.checkVesselSituation(&v));
    v.situation = Vessel::Situations::FLYING;
    CHECK_FALSE(landed.checkVesselSituation(&v));
    Parameters::ReachSituation pre(Vessel::Situations::PRELAUNCH, "");
    v.situation = Vessel::Situations::LANDED;
    v.landedAt = "Runway";
    CHECK(pre.checkVesselSituation(&v));
    v.landedAt = "LaunchPad";
    CHECK(pre.checkVesselSituation(&v));
    v.landedAt = "Somewhere";
    CHECK_FALSE(pre.checkVesselSituation(&v));
    CHECK(landed.Title() == "Be " + Parameters::ReachSituation::GetTitleStringShort(Vessel::Situations::LANDED));
    CHECK(Parameters::ReachSituation::GetTitleStringShort(static_cast<Vessel::Situations>(128)) == Localizer37::Format("#autoLOC_268861"));
    // ReachSpeedEnvelope (the orbital branch tests srfSpeed >= minSpeed, sic)
    Parameters::ReachSpeedEnvelope spd(100.0f, 500.0f);
    v.situation = Vessel::Situations::FLYING;
    v.srfSpeed = 99.9;
    CHECK_FALSE(spd.checkVesselWithinEnvelope(&v));
    v.srfSpeed = 100.0;
    CHECK(spd.checkVesselWithinEnvelope(&v));
    v.srfSpeed = 500.0;
    CHECK_FALSE(spd.checkVesselWithinEnvelope(&v));
    v.situation = Vessel::Situations::ORBITING;
    v.obt_speed = 499.0;
    v.srfSpeed = 120.0;
    CHECK(spd.checkVesselWithinEnvelope(&v));
    v.srfSpeed = 50.0;
    CHECK_FALSE(spd.checkVesselWithinEnvelope(&v));
    v.situation = Vessel::Situations::DOCKED;
    CHECK_FALSE(spd.checkVesselWithinEnvelope(&v));
    CHECK(spd.minSpeed == 100.0);
    CHECK(Parameters::ReachSpeedEnvelope().maxSpeed == std::numeric_limits<double>::max());
    CHECK(Parameters::ReachSpeedEnvelope(500.0f, 100.0f).minSpeed == 100.0);  // Math.Min / Math.Max of the arguments
    // load / save (SystemUtilities.LoadNode defaults)
    ConfigNode sn("PARAM");
    spd.Save(sn);
    CHECK(*sn.GetValue("minSpd") == "100");
    Parameters::ReachSpeedEnvelope spd2;
    spd2.Load(sn);
    CHECK(spd2.minSpeed == 100.0);
    CHECK(spd2.maxSpeed == 500.0);
    Parameters::ReachSpeedEnvelope spd3;
    ConfigNode empty("PARAM");
    spd3.Load(empty);
    CHECK(spd3.minSpeed == 0.0);
    CHECK(spd3.maxSpeed == std::numeric_limits<double>::max());
    // ReachFlightEnvelope
    Parameters::ReachFlightEnvelope fe(&kerbin, Vessel::Situations::FLYING, nullptr, 20000.0f, 5000.0f, 600.0f, 200.0f, "");
    CHECK(fe.useAltLimits);
    CHECK(fe.useSpdLimits);
    v.situation = Vessel::Situations::FLYING;
    v.srfSpeed = 300.0;
    v.altitude = 10000.0;
    CHECK(fe.checkVesselWithinFlightEnvelope(&v));
    v.altitude = 4999.0;
    CHECK_FALSE(fe.checkVesselWithinFlightEnvelope(&v));
    v.altitude = 20000.0;
    CHECK_FALSE(fe.checkVesselWithinFlightEnvelope(&v));
    v.situation = Vessel::Situations::LANDED;
    CHECK_FALSE(fe.checkVesselWithinFlightEnvelope(&v));  // not the envelope situation
    // GetTitle mutates the stored title (sic): a second call appends again
    const std::string t1 = fe.Title();
    const std::string t2 = fe.Title();
    CHECK(t2.size() > t1.size());
    // ReachDestination
    Parameters::ReachDestination rd(&kerbin, "Reach ");
    CHECK(rd.Title() == "Reach " + Localizer37::Format("#autoLOC_7001301", {kerbin.displayName()}));
    // parameter ids and the tree helpers
    TreeContract c;
    c.agent = L.w.agents->GetAgent("Kerbodyne");
    c.AddParameter(std::make_unique<Parameters::ReachDestination>(&kerbin, ""), "dest");
    CHECK(c.GetParameter("dest") != nullptr);
    CHECK(c.GetParameterT<Parameters::ReachDestination>() != nullptr);
    CHECK(c.GetParameterT<Parameters::ReachDestination>(std::string("dest")) != nullptr);
    CHECK(c.GetParameterT<Parameters::ReachDestination>(std::string("other")) == nullptr);
    CHECK(c.GetParameterOfType("ReachDestination") != nullptr);
    c.RemoveParameter("dest");
    CHECK(c.ParameterCount() == 0);
}

TEST_CASE("WP-37 Acquire / Recover parameters: kerbals, parts, save format") {
    Life37 L;
    TreeContract c;
    c.agent = L.w.agents->GetAgent("Kerbodyne");
    auto* ac = static_cast<Parameters::AcquireCrew*>(c.AddParameter(std::make_unique<Parameters::AcquireCrew>("Get Jeb", Parameters::AcquireCrew::CompleteCondition::All)));
    ac->AddKerbal("Jebediah Kerman");
    ac->AddKerbal("Jebediah Kerman");  // no duplicate
    ac->AddKerbal("Bill Kerman");
    auto* ap = static_cast<Parameters::AcquirePart*>(c.AddParameter(std::make_unique<Parameters::AcquirePart>("Get parts")));
    ap->AddPart(11u);
    ap->AddPart(11u);
    ap->AddPart(22u);
    auto* rk = static_cast<Parameters::RecoverKerbal*>(c.AddParameter(std::make_unique<Parameters::RecoverKerbal>("Recover Jeb")));
    rk->AddKerbal("Jebediah Kerman");
    auto* rp = static_cast<Parameters::RecoverPart*>(c.AddParameter(std::make_unique<Parameters::RecoverPart>(
        "Recover part", Parameters::RecoverPart::CompleteCondition::AllCandidates, Parameters::RecoverPart::CompleteCondition::AnyCandidate)));
    rp->AddPartToRecover(11u);
    rp->AddPartToRecover(11u);
    CHECK(ac->kerbalsToRecover.size() == 2);
    CHECK(ap->partsToRecover.size() == 2);
    CHECK(rp->partsToRecover.size() == 1);
    ConfigNode n1("PARAM"), n2("PARAM"), n3("PARAM"), n4("PARAM");
    ac->Save(n1);
    ap->Save(n2);
    rk->Save(n3);
    rp->Save(n4);
    CHECK(*n1.GetValue("winOn") == "0");  // (int)winCondition
    CHECK(n1.GetValues("kerbal") == std::vector<std::string>{"Jebediah Kerman", "Bill Kerman"});
    CHECK(n2.GetValues("part") == std::vector<std::string>{"11", "22"});
    CHECK(*n3.GetValue("failOn") == "All");
    CHECK(*n3.GetValue("winOn") == "All");
    CHECK(*n4.GetValue("winOn") == "AllCandidates");
    CHECK(*n4.GetValue("failOn") == "AnyCandidate");
    Parameters::AcquireCrew ac2;
    ac2.Load(n1);
    CHECK(ac2.kerbalsToRecover == ac->kerbalsToRecover);
    CHECK(ac2.winCondition == Parameters::AcquireCrew::CompleteCondition::All);
    Parameters::RecoverPart rp2;
    rp2.Load(n4);
    CHECK(rp2.partsToRecover == rp->partsToRecover);
    CHECK(rp2.winCondition == Parameters::RecoverPart::CompleteCondition::AllCandidates);
    // the AcquireCrew / AcquirePart events (events are live once the contract is Active)
    // Contract.SetState registers the parameters BEFORE it stores the new state, so AcquireCrew.OnRegister (`Root.ContractState == Active`) only
    // subscribes when the contract is registered again while Active (ContractSystem.RegisterContracts after a load): do that here
    c.state = Contract::State::Active;
    c.Register();
    EventReport killed(FlightEvents::CREW_KILLED, nullptr, "Bill Kerman");
    GE::onCrewKilled.Fire(&killed);  // All: any death fails the AcquireCrew
    CHECK(ac->State() == ParameterState::Failed);
    c.Unregister();  // the parameters must not outlive their GameEvents subscriptions (the later tests fire the same static events)
}

// ---- PartTestConstraint / ModuleTestSubject vs the part cfgs ------------------------------------------------------------------------------------------
TEST_CASE("WP-37 PartTestConstraint.Load over every ModuleTestSubject of the stock parts vs the reference (1426 CONSTRAINT nodes)") {
    int constraints = 0, parts = 0;
    const std::string base = GameData37();
    for (const json& p : Ref37()["part_constraints"]) {
        ConfigNode::NodePtr root = ConfigNode::Load(base + "/" + p["file"].get<std::string>());
        REQUIRE(root);
        ConfigNode* part = nullptr;
        for (ConfigNode* n : root->GetNodes("PART"))
            if (n->GetValue("name") == std::optional<std::string>(p["part"].get<std::string>())) part = n;
        REQUIRE(part != nullptr);
        ConfigNode* mod = nullptr;
        for (ConfigNode* m : part->GetNodes("MODULE"))
            if (m->GetValue("name") == std::optional<std::string>("ModuleTestSubject")) mod = m;
        REQUIRE(mod != nullptr);
        std::vector<ConfigNode*> nodes = mod->GetNodes("CONSTRAINT");
        CHECK(nodes.size() == p["total"].get<size_t>());
        size_t valid = 0;
        for (ConfigNode* cn : nodes) {
            PartTestConstraint c(cn);
            if (!c.valid) continue;
            REQUIRE(valid < p["valid"].size());
            const json& r = p["valid"][valid++];
            INFO(p["part"] << " " << r["type"]);
            CHECK(dotnet::EnumToString(c.type) == r["type"].get<std::string>());
            CHECK(dotnet::EnumToString(c.test) == r["test"].get<std::string>());
            CHECK(c.body == r["body"].get<std::string>());
            CHECK(c.prestige == r["prestige"].get<std::string>());
            CHECK(c.value == r["value"].get<std::string>());
            CHECK(c.valueB == r["valueB"].get<bool>());
            CHECK(c.valueI == r["valueI"].get<uint32_t>());
            CHECK(dotnet::EnumToString(c.valueR) == r["valueR"].get<std::string>());
            CHECK(c.situationMask == r["situationMask"].get<uint32_t>());
            if (r["valueD"].is_null())
                CHECK(std::isnan(c.valueD));
            else
                CHECK_F64(c.valueD, r["valueD"].get<double>());
            ++constraints;
        }
        CHECK(valid == p["valid"].size());
        ++parts;
    }
    CHECK(parts == 75);
    CHECK(constraints == 1426);
}

TEST_CASE("WP-37 PartTestConstraint tests, body / prestige / situation availability") {
    World37 w;
    ConfigNode alt("CONSTRAINT");
    alt.AddValue("type", "ALTITUDE");
    alt.AddValue("test", "GT");
    alt.AddValue("value", "1000");
    PartTestConstraint gt(&alt);
    CHECK(gt.valid);
    CHECK(gt.Test(1000.5));
    CHECK_FALSE(gt.Test(1000.0));
    CHECK(gt.Test(1001));  // the int overload compares with valueI (uint)
    CHECK_FALSE(gt.Test(999));
    ConfigNode bad("CONSTRAINT");
    bad.AddValue("type", "ALTITUDE");
    bad.AddValue("value", "high");  // not a number: invalid
    CHECK_FALSE(PartTestConstraint(&bad).valid);
    ConfigNode badType("CONSTRAINT");
    badType.AddValue("type", "WEIGHT");
    CHECK_FALSE(PartTestConstraint(&badType).valid);
    CHECK_FALSE(PartTestConstraint().valid);
    CHECK_FALSE(PartTestConstraint(&bad).Test(1.0));  // an invalid constraint tests false
    ConfigNode atm("CONSTRAINT");
    atm.AddValue("type", "ATMOSPHERE");
    atm.AddValue("value", "True");
    PartTestConstraint a(&atm);
    CHECK(a.valid);
    CHECK(a.Test(true));
    CHECK_FALSE(a.Test(false));
    ConfigNode sit("CONSTRAINT");
    sit.AddValue("type", "SITUATION");
    sit.AddValue("test", "EQ");
    sit.AddValue("value", "24");  // FLYING (8) | SUB_ORBITAL (16)
    PartTestConstraint s(&sit);
    CHECK(s.valid);
    CHECK(s.Test(8u));
    CHECK_FALSE(s.Test(32u));
    std::vector<PartTestConstraint> list{s};
    CHECK(PartTestConstraint::SituationAvailable(list, Vessel::Situations::FLYING));
    CHECK_FALSE(PartTestConstraint::SituationAvailable(list, Vessel::Situations::ORBITING));
    ConfigNode body("CONSTRAINT");
    body.AddValue("type", "ATMOSPHERE");
    body.AddValue("value", "False");
    body.AddValue("body", "_NotHome");
    body.AddValue("prestige", "Significant");
    PartTestConstraint b(&body);
    CHECK(b.BodyAvailable(&w.Body("Mun")));
    CHECK_FALSE(b.BodyAvailable(&w.Body("Kerbin")));
    CHECK(b.PrestigeAvailable(ContractPrestige::Significant));
    CHECK_FALSE(b.PrestigeAvailable(ContractPrestige::Trivial));
    CHECK(PartTestConstraint().PrestigeAvailable(ContractPrestige::Exceptional));
    CHECK_FALSE(PartTestConstraint().SituationAvailable(Vessel::Situations::LANDED));  // invalid -> false
}

TEST_CASE("WP-37 ModuleTestSubject is a real PartModule on the stock parts (HeatShield0)") {
    Game15 g;
    const AvailablePart& info = g.Info("HeatShield0");
    REQUIRE(info.partPrefab != nullptr);
    PartModule* m = info.partPrefab->FindModuleOfClass("ModuleTestSubject");
    REQUIRE(m != nullptr);
    auto* mts = dynamic_cast<ModuleTestSubject*>(m);
    REQUIRE(mts != nullptr);
    const json* ref = nullptr;
    for (const json& p : Ref37()["part_constraints"])
        if (p["part"].get<std::string>() == "HeatShield0") ref = &p;
    REQUIRE(ref != nullptr);
    CHECK(mts->constraints.size() == (*ref)["valid"].size());
    CHECK(mts->situationMask() == static_cast<uint32_t>(std::stoul((*ref)["situationMask"].get<std::string>())));
    CHECK(mts->useStaging() == ((*ref)["useStaging"].get<std::string>() == "True"));
    CHECK(mts->SituationAvailable(Vessel::Situations::FLYING));
    CHECK_FALSE(mts->IsStageable());
    // a second instance (Instantiate) keeps the constraints
    auto p2 = Part::Instantiate(*info.partPrefab);
    auto* mts2 = dynamic_cast<ModuleTestSubject*>(p2->FindModuleOfClass("ModuleTestSubject"));
    REQUIRE(mts2 != nullptr);
    CHECK(mts2->constraints.size() == mts->constraints.size());
}

// ---- predicates -------------------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-37 predicates: AgentParts / AgentFlag load and save, LaunchID date capture, IsActiveVessel") {
    Life37 L;
    TreeContract c;
    c.agent = L.w.agents->GetAgent("Kerbodyne");
    Agents::Agent* kerbodyne = L.w.agents->GetAgent("Kerbodyne");
    REQUIRE(kerbodyne != nullptr);
    Predicates::AgentParts ap(&c, kerbodyne, 2);
    CHECK(ap.Description() == Localizer37::Format("Vessel must contain at least <<1>> parts made by <<2>>", {"2", kerbodyne->Title()}));
    Predicates::AgentParts ap1(&c, kerbodyne, 1);
    CHECK(ap1.Description() == Localizer37::Format("#autoLOC_271049", {kerbodyne->Title()}));
    ConfigNode n("PREDICATE");
    ap.Save(n);
    CHECK(*n.GetValue("agent") == "Kerbodyne");
    CHECK(*n.GetValue("count") == "2");
    Predicates::AgentParts ap2(&c);
    ap2.Load(n);
    CHECK(ap2.PartCount() == 2);
    CHECK(ap2.GetAgent() == kerbodyne);
    Predicates::AgentFlag af(&c, kerbodyne);
    ConfigNode n2("PREDICATE");
    af.Save(n2);
    Predicates::AgentFlag af2(&c);
    af2.Load(n2);
    CHECK(af2.GetAgent() == kerbodyne);
    // LaunchID captures the launch id and the date when the contract becomes Active
    c.state = Contract::State::Offered;
    {
        Predicates::LaunchID lid(&c);
        L.w.game.launchID = 42;
        Planetarium::fetch->time = 777.0;
        L.w.game.flightState->universalTime = 777.0;
        c.Accept();
        CHECK(lid.Date() == 777.0);
        CHECK_THROWS(lid.ID());  // `public uint ID => ID;` recurses forever in the C#
        ConfigNode ln("PREDICATE");
        lid.Save(ln);
        CHECK(*ln.GetValue("launchID") == "42");
        CHECK(*ln.GetValue("date") == "777");
        Vessel v;
        v.launchTime = 800.0;
        CHECK(lid.Test(&v));
        v.launchTime = 700.0;
        CHECK_FALSE(lid.Test(&v));
    }
    CHECK(c.OnStateChange.EventName() == "OnStateChange");
    Predicates::IsActiveVessel iav(&c);
    Vessel v;
    CHECK_FALSE(iav.Test(&v));
    FlightRegistry::ActiveVessel = &v;
    CHECK(iav.Test(&v));
    FlightRegistry::ActiveVessel = nullptr;
}

// ---- templates that need the part database / R&D: PartTest and RecoverAsset state, text and node formats -------------------------------------------
TEST_CASE("WP-37 PartTest template: situation strings, OnLoad / OnSave and the parameter hash by repeatability") {
    Life37 L;
    Templates::PartTest t;
    t.agent = L.w.agents->GetAgent("Kerbodyne");
    CelestialBody& mun = L.w.Body("Mun");
    CelestialBody& kerbin = L.w.Body("Kerbin");
    // GetSituationString(sit, body, biome) for the haul and non-haul variants
    CHECK(t.GetSituationString(Vessel::Situations::FLYING, &kerbin, "") == Localizer37::Format("#autoLOC_6100009", {kerbin.displayName()}));
    CHECK(t.GetSituationString(Vessel::Situations::LANDED, &mun, "") == Localizer37::Format("#autoLOC_6100012", {mun.displayName()}));
    CHECK(t.GetSituationString(Vessel::Situations::PRELAUNCH, &kerbin, "") == Localizer37::Format("#autoLOC_6100007"));
    CHECK(t.GetSituationString(Vessel::Situations::DOCKED, &kerbin, "") == "No situation report available");
    CHECK(t.GetSituationString(Vessel::Situations::ORBITING, &mun, "Highlands") == Localizer37::Format("#autoLOC_6100029", {mun.displayName(), "Highlands"}));
    // node round trip
    ConfigNode node("CONTRACT");
    node.AddValue("part", "HeatShield0");
    node.AddValue("haul", "True");
    node.AddValue("dest", static_cast<int32_t>(mun.flightGlobalsIndex));
    node.AddValue("sit", "ORBITING");
    node.AddValue("exp", "True");
    t.OnLoad(node);
    CHECK(t.Hauled());
    CHECK(t.Destination() == &mun);
    CHECK(t.TargetSituation() == Vessel::Situations::ORBITING);
    CHECK(t.PartIsExperimental);
    CHECK(t.TargetPart() != nullptr);
    CHECK(t.PartName == t.TargetPart()->title);
    ConfigNode out("CONTRACT");
    t.OnSave(out);
    CHECK(*out.GetValue("part") == "HeatShield0");
    CHECK(*out.GetValue("haul") == "True");
    CHECK(*out.GetValue("dest") == dotnet::FormatInt64(mun.flightGlobalsIndex));
    CHECK(*out.GetValue("sit") == "ORBITING");
    // Contracts.Parameters.PartTest hash: ALWAYS adds the unique id, BODYANDSITUATION the body and situation, ONCEPERPART only the part
    Parameters::PartTest p(t.TargetPart(), std::nullopt, 0, &mun, Vessel::Situations::ORBITING, "uid", false);
    CHECK(p.HashString() == "HeatShield0uid");
    p.repeatability = 1;
    CHECK(p.HashString() == "HeatShield0MunORBITING");
    p.repeatability = 2;
    CHECK(p.HashString() == "HeatShield0");
    ConfigNode pn("PARAM");
    p.Save(pn);
    CHECK(*pn.GetValue("repeatability") == "ONCEPERPART");
    CHECK_FALSE(pn.HasValue("notes"));  // null notes are not written
}

TEST_CASE("WP-37 RecoverAsset template: multiplier, hash, node formats and the parameter wiring with a spawn seam") {
    Life37 L;
    Templates::TemplateEnvironment::Reset();
    Templates::RecoverAsset r;
    r.agent = L.w.agents->GetAgent("Kerbodyne");
    ConfigNode node("CONTRACT");
    node.AddValue("targetBody", static_cast<int32_t>(L.w.Body("Mun").flightGlobalsIndex));
    node.AddValue("kerbalName", "Jebediah Kerman");
    node.AddValue("partName", "mk1pod.v2");
    node.AddValue("partID", static_cast<uint32_t>(4242));
    node.AddValue("recoveryLocation", static_cast<int32_t>(1));
    node.AddValue("recoveryType", static_cast<int32_t>(1));
    L.w.game.CrewRoster = std::make_shared<KerbalRoster>(Game_Modes::CAREER);
    L.w.game.CrewRoster->AddCrewMember(std::make_shared<ProtoCrewMember>(ProtoCrewMember_KerbalType::Crew, "Jebediah Kerman"));
    r.OnLoad(node);
    CHECK(r.TargetBody() == &L.w.Body("Mun"));
    CHECK(r.Location() == Templates::RecoverAsset::RecoveryLocation::ORBITLOW);
    CHECK(r.Type() == Templates::RecoverAsset::RecoveryType::KERBAL);
    CHECK(r.PartID() == 4242u);
    CHECK(r.PartNameValue() == "mk1pod.v2");
    CHECK(r.RecoveryKerbal() != nullptr);
    ConfigNode out("CONTRACT");
    r.OnSave(out);
    CHECK(*out.GetValue("recoveryLocation") == "1");
    CHECK(*out.GetValue("recoveryType") == "1");
    CHECK(*out.GetValue("partID") == "4242");
    // OnAccepted calls the spawn seam (the vessel creation is WP-38 / WP-19), then wires the part into the parameters
    bool spawned = false, land = true;
    Templates::TemplateEnvironment::spawnRecoveryVessels = [&](Templates::RecoverAsset& a, bool l) {
        spawned = true;
        land = l;
        a.SetPartID(77u);
    };
    r.AddParameter(std::make_unique<Parameters::AcquireCrew>("crew"));
    r.AddParameter(std::make_unique<Parameters::RecoverKerbal>("recover"));
    r.OnAccepted();
    CHECK(spawned);
    CHECK_FALSE(land);
    CHECK(r.GetParameterT<Parameters::AcquireCrew>()->kerbalsToRecover.size() == 1);
    CHECK(r.GetParameterT<Parameters::RecoverKerbal>()->kerbalsToRecover.size() == 1);
    Templates::TemplateEnvironment::Reset();
}

// ---- GrandTour: structure, rewards from the visited bodies, the node format (the WP-38 parameter classes are test stand-ins) ------------------------------
TEST_CASE("WP-37 GrandTour template: tour selection, parameter wiring, reward sums and node round trip (WP-38 parameters are stand-ins)") {
    Life37 L;
    Templates::TemplateEnvironment::Reset();
    Templates::TemplateEnvironment::makeVesselSystemsParameter = [](const std::string& title) { return std::make_unique<Parameters::AcquirePart>(title); };
    Templates::TemplateEnvironment::makeVesselDestinationParameter = [](CelestialBody* b, int) { return std::make_unique<Parameters::EnterSOI>(b); };
    Templates::TemplateEnvironment::makeLocationAndSituationParameter = [](CelestialBody* b, Vessel::Situations, const std::string&, bool) {
        return std::make_unique<Parameters::ReachDestination>(b, "");
    };
    // many bodies flown by (the Flyby node of the body subtrees): Mun, Minmus, Duna, Eve, Moho, Ike, Eeloo, Dres
    ConfigNode scen("SCENARIO");
    ConfigNode* prog = scen.AddNode("Progress");
    for (const char* b : {"Kerbin", "Mun", "Minmus", "Duna", "Eve", "Moho", "Ike", "Eeloo", "Dres"}) {
        ConfigNode* bn = prog->AddNode(b);
        bn->AddNode("Orbit")->AddValue("completed", 0.0);
        if (std::string(b) != "Kerbin") bn->AddNode("Flyby")->AddValue("completed", 0.0);
    }
    L.w.pt->OnLoad(scen);
    int generated = 0;
    for (int seed = 1; seed < 4000 && generated < 6; ++seed) {
        Templates::GrandTour probe;
        // MeetRequirements: (float)(HashNumber((uint)MissionSeed) % 100) <= Rarity (5)
        const std::unique_ptr<Contract> c = L.cs.GenerateContract(seed, seed % 2 ? ContractPrestige::Significant : ContractPrestige::Exceptional, "GrandTour");
        const bool meets = static_cast<float>(Lite::HashNumber(static_cast<uint32_t>(seed)) % 100u) <= 5.0f;
        if (!meets) {
            CHECK(c == nullptr);
            continue;
        }
        if (c == nullptr) continue;  // e.g. not enough bodies for the prestige
        ++generated;
        auto* gt = dynamic_cast<Templates::GrandTour*>(c.get());
        REQUIRE(gt != nullptr);
        INFO("seed " << seed);
        const auto& bodies = gt->TargetBodies();
        REQUIRE(bodies.size() >= 1);
        // one VesselSystems stand-in, then one destination parameter per target body, then the final one when there is a final destination
        CHECK(c->ParameterCount() >= static_cast<int>(bodies.size()) + 1);
        CHECK(c->GetParameter(0)->TypeName() == "AcquirePart");
        for (size_t i = 0; i < bodies.size(); ++i) CHECK(static_cast<Parameters::EnterSOI*>(c->GetParameter(static_cast<int>(i) + 1))->TargetBody() == bodies[i]);
        // Funds.BaseAdvance summed over the bodies: ScoreProgressType * RecoveryValue (before the prestige / agent factors): the reward grows with the tour
        CHECK(c->FundsAdvance > 0.0);
        CHECK_FALSE(c->Title().empty());
        CHECK_FALSE(c->Synopsys().empty());
        CHECK_FALSE(c->Description().empty());
        CHECK(c->Synopsys().find(bodies[0]->displayName().substr(0, bodies[0]->displayName().find('^'))) != std::string::npos);  // Lingoona drops the ^N gender tag
        // node format
        ConfigNode saved("CONTRACT");
        c->Save(saved);
        CHECK(saved.GetValues("body").size() == bodies.size());
        CHECK(*saved.GetValue("type") == "GrandTour");
        L.cs.contracts.clear();
        L.cs.contracts.push_back(std::make_unique<Templates::GrandTour>());
        auto& loaded = *L.cs.contracts.back();
        REQUIRE(Contract::Load(loaded, *saved.CreateCopy()));
        ConfigNode saved2("CONTRACT");
        loaded.Save(saved2);
        CHECK(saved2.ToString() == saved.ToString());
    }
    CHECK(generated >= 1);
    Templates::TemplateEnvironment::Reset();
}
