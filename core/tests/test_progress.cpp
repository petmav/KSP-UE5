// WP-36 unit tests: funds, reputation, strategies, facilities, vessel recovery, progress tracking (core/include/ksp/career).
//
// No golden or capture of these systems exists (harness/golden/MANIFEST.json and harness/captures/out*/ carry no funds, reputation,
// strategy, facility, recovery or progress data), so every expectation is derived: from tools/wp36/currency_reference.py (an independent
// numpy float32 / float64 implementation that reads the strategy cfgs of the GameData mirror, the Kerbin radius of the WP-09 catalog source
// and the KSC position of the PQS prefab; tests/data/wp36_reference.json), from the decompiled C# (the derivation is in the comment above the
// check) or from the physprofile save of testdata (the SCENARIO nodes the save really contains).
//
// Fixtures that are not KSP data (STATUS.md "needs data"): the reputationAddition / reputationSubtraction prefab curves (constant curves 0.9 /
// 1.3 here), the facility level costs, the synthetic nodes of the Funding / Reputation / ScenarioUpgradeableFacilities / StrategySystem
// format tests (the physprofile sandbox save has none of those SCENARIOs).
#include <cmath>
#include <cstring>
#include <fstream>
#include <array>
#include <map>
#include <set>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/career/career.hpp"
#include "ksp/career/format.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_types.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using GE = events::GameEvents;
using Localizer36 = KSP::Localization::Localizer;
using json = nlohmann::json;

namespace {

const std::string kHarness36 = KSPCORE_HARNESS_DIR;
const std::string kSaves36 = std::string(KSPCORE_TESTDATA_DIR) + "/saves";

const json& Ref36() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp36_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

uint32_t Bits36(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
// 0 ULP: the float must be the reference's float32 exactly
#define CHECK_F32(got, ref) CHECK_MESSAGE(Bits36(static_cast<float>(got)) == Bits36(static_cast<float>(ref)), "got ", (got), " want ", (ref))

void LoadLocalizer36() {
    Localizer36::Shutdown();
    Localizer36::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(ksp::testing::GameRoot() + "/GameData/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    Localizer36::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

// The stock game database + the Kerbol system + a CAREER game + GameVariables with fixture reputation curves.
struct World36 {
    Game15 g;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    Game game;
    std::unique_ptr<GameVariables> gv;
    World36() {
        FlightRegistry::Clear();
        CareerEnvironment::Reset();
        ScenarioUpgradeableFacilities::ClearStatics();
        ScenarioDestructibles::ClearStatics();
        SaveEnvironment& env = SaveEnvironment::Current();
        pg.physicsDatabaseFilename = kHarness36 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        game.Mode = Game::Modes::CAREER;
        env.currentGame = &game;
        LoadLocalizer36();
        gv = std::make_unique<GameVariables>();
        SetCurves(0.9f, 1.3f);
    }
    ~World36() {
        gv.reset();
        CareerEnvironment::Reset();
        ScenarioUpgradeableFacilities::ClearStatics();
        ScenarioDestructibles::ClearStatics();
        SaveEnvironment::Current().currentGame = nullptr;
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
    }
    // constant curves: one key; Unity evaluates a one-key curve to its value everywhere (fixture, not the prefab curves)
    void SetCurves(float add, float sub) {
        gv->reputationAddition = AnimationCurve(std::vector<Keyframe>{Keyframe(0.0f, add)});
        gv->reputationSubtraction = AnimationCurve(std::vector<Keyframe>{Keyframe(0.0f, sub)});
    }
    CelestialBody& Body(const char* n) { return *sys->GetBodyByName(n); }
    GameParameters::CareerParams& Career() { return game.Parameters->Career; }
};

float F(const json& j) { return j.get<float>(); }

// Reputation.addReputation_granular with constant curves, restated from the C# (Mono float rule): returns the new rep, `applied` the sum
float GranularRef(float rep, float value, float addC, float subC, float* applied = nullptr) {
    const int num = static_cast<int>(std::fabs(value));
    const float delta = static_cast<float>(1.0 * (value >= 0.0f ? 1.0 : -1.0));
    float num2 = 0.0f;
    for (int i = 0; i <= num; ++i) {
        const float d = (i == num) ? static_cast<float>(fx(value) - fx(num2)) : delta;
        const float num3 = static_cast<float>(fx(d) * fx(d < 0.0f ? subC : addC));
        rep = static_cast<float>(fx(rep) + fx(num3));
        num2 = static_cast<float>(fx(num2) + fx(num3));
    }
    if (applied != nullptr) *applied = num2;
    return rep;
}

}  // namespace

// ---- GameVariables ----------------------------------------------------------------------------------------------------------------

// GameVariables.cs:285-:679 over the facility-level grid of the reference (the step functions, the stored-level table, the thresholds
// +-1 ULP): every value is compared exactly
TEST_CASE("WP-36 GameVariables formulas over the facility level grid (0 ULP vs the numpy reference)") {
    REQUIRE(Ref36().contains("gv"));
    World36 w;
    GameVariables& gv = *GameVariables::Instance;
    auto& adv = w.game.Parameters->CustomParams<GameParameters::AdvancedParams>();
    auto& cn = w.game.Parameters->CustomParams<CommNet::CommNetParams>();
    int rows = 0;
    for (const json& r : Ref36()["gv"]["rows"]) {
        const float L = F(r["level"]);
        INFO("level ", L);
        CHECK(gv.GetPartCountLimit(L, true) == r["partCount"].get<int>());
        CHECK(gv.GetPartCountLimit(L, false) == r["partCount"].get<int>());
        CHECK(gv.UnlockedFuelTransfer(L) == r["fuelTransfer"].get<bool>());
        for (int always = 0; always < 2; ++always) {
            adv.ActionGroupsAlways = always != 0;
            CHECK(gv.UnlockedActionGroupsStock(L, true) == r["agStock"][always].get<bool>());
            CHECK(gv.UnlockedActionGroupsCustom(L, false) == r["agCustom"][always].get<bool>());
        }
        adv.ActionGroupsAlways = false;
        CHECK_F32(gv.GetCraftMassLimit(L, true), F(r["craftMass"]));
        const Vector3f sz = gv.GetCraftSizeLimit(L, false);
        CHECK_F32(sz.x, F(r["craftSize"][0]));
        CHECK_F32(sz.y, F(r["craftSize"][1]));
        CHECK_F32(sz.z, F(r["craftSize"][2]));
        CHECK(gv.GetActiveStrategyLimit(L) == r["strategyCount"].get<int>());
        CHECK_F32(gv.GetStrategyCommitRange(L), F(r["strategyRange"]));
        CHECK_F32(gv.GetStrategyLevelLimit(L), F(r["levelLimit"]));
        CHECK(gv.GetActiveContractsLimit(L) == r["contracts"].get<int>());
        CHECK_F32(gv.GetContractLevelLimit(L), F(r["levelLimit"]));
        CHECK(gv.UnlockedFlightPlanning(L) == r["flightPlanning"].get<bool>());
        CHECK(gv.GetActiveCrewLimit(L) == r["crewLimit"].get<int>());
        CHECK_F32(gv.GetCrewLevelLimit(L), F(r["crewLevel"]));
        CHECK(gv.UnlockedEVA(L) == r["eva"].get<bool>());
        CHECK(gv.UnlockedEVAFlags(L) == r["evaFlags"].get<bool>());
        CHECK(gv.UnlockedEVAClamber(L) == r["evaClamber"].get<bool>());
        const float mods[3] = {1.0f, 0.5f, 1.5f};
        for (int i = 0; i < 3; ++i) {
            cn.DSNModifier = mods[i];
            CHECK(gv.GetDSNRange(L) == r["dsnRange"][i].get<double>());
        }
        cn.DSNModifier = 1.0f;
        CHECK(static_cast<int>(gv.GetOrbitDisplayMode(L)) == r["orbitDisplay"].get<int>());
        CHECK(gv.GetTrackedObjectLimit(L) == r["trackedObjects"].get<int>());
        CHECK(gv.GetPatchesAheadLimit(L) == r["patchesAhead"][0].get<int>());
        CHECK(gv.UnlockedSpaceObjectDiscovery(L) == r["spaceObjectDiscovery"].get<bool>());
        CHECK(static_cast<int>(gv.MinTrackedObjectSize(L)) == r["minTracked"].get<int>());
        CHECK(gv.ManeuverToolAvailable(L) == r["maneuverTool"].get<bool>());
        CHECK_F32(gv.GetScienceCostLimit(L), F(r["scienceCost"]));
        CHECK_F32(gv.GetDataToScienceRatio(L), F(r["dataRatio"]));
        CHECK_F32(gv.GetExperimentLevel(L), F(r["experimentLevel"]));
        ++rows;
    }
    CHECK(rows >= 60);
}

// GameVariables.cs:100-:272 and :463: hire cost (the only transcendental), contract / mentality factors, flight envelope score, recovered values
TEST_CASE("WP-36 GameVariables scalar formulas (hire cost, contract and mentality factors, flight envelope) vs the reference") {
    World36 w;
    GameVariables& gv = *GameVariables::Instance;
    const json& sc = Ref36()["gv"]["scalars"];
    const json& mults = sc["hire"]["multipliers"];
    for (const json& row : sc["hire"]["rows"]) {
        for (size_t m = 0; m < mults.size(); ++m) {
            w.Career().FundsLossMultiplier = F(mults[m]);
            CHECK_F32(gv.GetRecruitHireCost(row["active"].get<int>()), F(row["costs"][m]));
        }
    }
    for (const json& r : sc["contractFactors"]) {
        const float m = F(r["mult"]);
        w.Career().FundsGainMultiplier = w.Career().FundsLossMultiplier = w.Career().RepGainMultiplier = w.Career().RepLossMultiplier =
            w.Career().ScienceGainMultiplier = m;
        const auto prestige = static_cast<ContractPrestige>(r["prestige"].get<int>());
        CHECK_F32(gv.GetContractPrestigeFactor(prestige), F(r["factor"]));
        CHECK_F32(gv.GetContractFundsAdvanceFactor(prestige), F(r["gain"]));
        CHECK_F32(gv.GetContractFundsCompletionFactor(prestige), F(r["gain"]));
        CHECK_F32(gv.GetContractReputationCompletionFactor(prestige), F(r["gain"]));
        CHECK_F32(gv.GetContractScienceCompletionFactor(prestige), F(r["gain"]));
        CHECK_F32(gv.GetContractFundsFailureFactor(prestige), F(r["loss"]));
        CHECK_F32(gv.GetContractReputationFailureFactor(prestige), F(r["loss"]));
    }
    for (const json& r : sc["mentality"]) {
        const float f = F(r["f"]);
        CHECK_F32(gv.GetMentalityFundsFactor(f, ContractPrestige::Trivial), F(r["trivial"]));
        CHECK_F32(gv.GetMentalityReputationFactor(f, ContractPrestige::Significant), F(r["significant"]));
        CHECK_F32(gv.GetMentalityScienceFactor(f, ContractPrestige::Exceptional), F(r["exceptional"]));
        CHECK_F32(gv.GetMentalityExpiryFactor(f, ContractPrestige::Trivial), F(r["trivial"]));
        CHECK_F32(gv.GetMentalityDeadlineFactor(f, ContractPrestige::Exceptional), F(r["exceptional"]));
    }
    for (const json& r : sc["flightEnvelope"])
        CHECK_F32(gv.ScoreFlightEnvelope(F(r["alt"]), F(r["altEnv"]), F(r["speed"]), F(r["speedEnv"])), F(r["score"]));
    for (const json& r : sc["recovered"]) {
        CHECK_F32(gv.GetRecoveredPartValue(F(r["v"])), F(r["part"]));
        CHECK_F32(gv.GetRecoveredResourceValue(F(r["v"])), F(r["resource"]));
    }
    // GameVariables.cs:158 ScoreSituation (the literal table): home body / other body
    CelestialBody* home = &w.Body("Kerbin");
    CHECK_F32(gv.ScoreSituation(VesselSituations::ESCAPING, home), 1.1f);
    CHECK_F32(gv.ScoreSituation(VesselSituations::DOCKED, home), 0.1f);
    CHECK_F32(gv.ScoreSituation(VesselSituations::LANDED, &w.Body("Mun")), 1.5f);
    CHECK_F32(gv.ScoreSituation(VesselSituations::ORBITING, &w.Body("Mun")), 0.8f);
    CHECK_F32(gv.ScoreSituation(VesselSituations::PRELAUNCH, &w.Body("Mun")), 0.1f);
}

// GameVariables.cs:537-:559: the three DSN curves
TEST_CASE("WP-36 GameVariables DSN curves") {
    World36 w;
    DoubleCurve range = w.gv->GetDSNRangeCurve();
    DoubleCurve power = w.gv->GetDSNPowerCurve();
    DoubleCurve sci = w.gv->GetDSNScienceCurve();
    CHECK(range.keys.size() == 2);
    CHECK(power.keys.size() == 2);
    CHECK(sci.keys.size() == 3);
    CHECK(power.keys[1].value == 0.10000000149011612);
    CHECK(sci.keys[1].time == 0.5);
}

// ---- strategies ---------------------------------------------------------------------------------------------------------------------

namespace {

// StrategySystem with every strategy of the cfgs, loaded like OnLoad does (STRATEGIES { STRATEGY { name factor } }): `active` names
// (in the order given) are Loaded active, the others stay inactive
struct StrategyWorld {
    World36& w;
    std::unique_ptr<Strategies::StrategySystem> sys;
    explicit StrategyWorld(World36& world) : w(world) {
        sys = std::make_unique<Strategies::StrategySystem>();
        sys->Awake();
    }
    void Activate(const std::vector<std::pair<std::string, float>>& active) {
        ConfigNode game;
        ConfigNode* st = game.AddNode("STRATEGIES");
        for (const auto& a : active) {
            ConfigNode* n = st->AddNode("STRATEGY");
            n->AddValue("name", a.first);
            n->AddValue("date", 0.0);
            n->AddValue("factor", a.second);
        }
        sys->LoadNow(game);
    }
    ~StrategyWorld() { sys.reset(); }
};

}  // namespace

// StrategyConfig.Create / StrategySystemConfig: the stock cfgs (Departments.cfg + Strategies.cfg) give 11 strategies in file order
TEST_CASE("WP-36 StrategySystemConfig loads the stock departments and strategies") {
    World36 w;
    StrategyWorld sw(w);
    auto* cfg = sw.sys->SystemConfig();
    const json& st = Ref36()["strategies"]["strategies"];
    REQUIRE(cfg->Strategies().size() == st.size());
    CHECK(cfg->Departments().size() == 4);
    for (size_t i = 0; i < st.size(); ++i) {
        const Strategies::StrategyConfig& c = *cfg->Strategies()[i];
        CHECK(c.Name() == st[i]["name"].get<std::string>());
        CHECK(c.DepartmentName() == st[i]["department"].get<std::string>());
        CHECK(c.Department() != nullptr);
        CHECK(c.HasFactorSlider() == st[i]["hasSlider"].get<bool>());
        CHECK_F32(c.FactorSliderDefault(), F(st[i]["sliderDefault"]));
        CHECK(c.FactorSliderSteps() == st[i]["sliderSteps"].get<int>());
        std::vector<std::string> tags = st[i]["groupTags"].get<std::vector<std::string>>();
        CHECK(c.GroupTags() == tags);
        CHECK(c.Effects().size() == st[i]["effectTypes"].size());
        // the strategies of a department are registered in file order
        bool found = false;
        for (auto* d : c.Department()->Strategies()) found = found || d == &c;
        CHECK(found);
    }
    // StrategySystem.Instance + the department queries
    CHECK(Strategies::StrategySystem::Instance() == sw.sys.get());
    sw.Activate({{"AppreciationCampaignCfg", 0.5f}});
    CHECK(sw.sys->HasActiveStrategy("Public Relations"));
    CHECK_FALSE(sw.sys->HasActiveStrategy("Finances"));
    CHECK(sw.sys->StrategiesActive().size() == 1);
    CHECK(sw.sys->StrategiesInactive().size() == 10);
    CHECK(sw.sys->GetStrategies("Public Relations").size() >= 1);
}


namespace {

TransactionReasons ReasonOf(const std::string& n) {
    TransactionReasons r{};
    REQUIRE_MESSAGE(dotnet::EnumTryParseFull(n, r), n);
    return r;
}

Strategies::Strategy* Find(Strategies::StrategySystem& sys, const std::string& name) {
    for (auto& x : sys.Strategies())
        if (x->Config()->Name() == name) return x.get();
    return nullptr;
}

void CheckQuery(const json& q, const CurrencyModifierQuery& cq, float factor = -1.0f) {
    CHECK_F32(cq.GetInput(Currency::Funds), F(q["input"][0]));
    for (int c = 0; c < 3; ++c)
        CHECK_MESSAGE(Bits36(cq.GetEffectDelta(static_cast<Currency>(c))) == Bits36(F(q["delta"][c])), q["reason"].get<std::string>(), " currency ", c, " factor ",
                      factor, " input ", q["input"].dump(), " got ", cq.GetEffectDelta(static_cast<Currency>(c)), " want ", q["delta"][c].dump());
}

}  // namespace

// Strategy.InitialCost* / RequiredReputation (FactorLerp = Mathf.Lerp), per stock strategy and factor of the slider grid
TEST_CASE("WP-36 Strategy initial costs and required reputation over the factor grid (0 ULP)") {
    World36 w;
    StrategyWorld sw(w);
    int checked = 0;
    for (const json& s : Ref36()["strategies"]["strategies"]) {
        Strategies::StrategyConfig* cfg = sw.sys->SystemConfig()->GetStrategyConfig(s["name"].get<std::string>());
        REQUIRE(cfg != nullptr);
        for (const json& fr : s["factors"]) {
            std::unique_ptr<Strategies::Strategy> st = Strategies::Strategy::Create("", cfg);
            st->SetFactor(F(fr["factor"]));
            CHECK_F32(st->InitialCostFunds(), F(fr["cost"][0]));
            CHECK_F32(st->InitialCostScience(), F(fr["cost"][1]));
            CHECK_F32(st->InitialCostReputation(), F(fr["cost"][2]));
            CHECK_F32(st->RequiredReputation(), F(fr["required"]));
            ++checked;
        }
        // Strategy.Create: factor = FactorSliderDefault when the slider default is set
        std::unique_ptr<Strategies::Strategy> d = Strategies::Strategy::Create("", cfg);
        CHECK_F32(d->Factor(), F(s["sliderDefault"]));
        // Strategy.LeastDuration / LongestDuration lerp the (never assigned) protected fields: 0 (sic)
        CHECK(d->LeastDuration() == 0.0);
        CHECK(d->LongestDuration() == 0.0);
        CHECK(d->Effects().size() == s["effectTypes"].size());
        for (size_t e = 0; e < d->Effects().size(); ++e)
            CHECK(std::string(d->Effects()[e]->TypeName()) == s["effectTypes"][e].get<std::string>());
    }
    CHECK(checked == 11 * 21);
}

// the effects on a CurrencyModifierQuery and a ValueModifierQuery, one strategy active at a time (CurrencyOperation, CurrencyConverter,
// ValueModifier); the CurrencyExchanger strategies act on registration (below)
TEST_CASE("WP-36 each stock strategy effect on CurrencyModifierQuery / ValueModifierQuery (0 ULP vs the reference)") {
    World36 w;
    StrategyWorld sw(w);
    int queries = 0, values = 0;
    for (const json& s : Ref36()["strategies"]["strategies"]) {
        const std::string name = s["name"].get<std::string>();
        for (const json& fr : s["factors"]) {
            if (fr["queries"].empty()) continue;
            sw.Activate({{name, F(fr["factor"])}});
            REQUIRE(Find(*sw.sys, name) != nullptr);
            CHECK(Find(*sw.sys, name)->IsActive());
            CHECK_F32(Find(*sw.sys, name)->Factor(), F(fr["factor"]));
            for (const json& q : fr["queries"]) {
                CurrencyModifierQuery cq =
                    CurrencyModifierQuery::RunQuery(ReasonOf(q["reason"].get<std::string>()), F(q["input"][0]), F(q["input"][1]), F(q["input"][2]));
                CheckQuery(q, cq);
                ++queries;
            }
            for (const json& v : fr["values"]) {
                ValueModifierQuery vq = ValueModifierQuery::RunQuery(v["valueId"].get<std::string>(), F(v["input"]));
                CHECK_F32(vq.GetEffectDelta(), F(v["delta"]));
                ++values;
            }
        }
    }
    CHECK(queries > 3000);
    CHECK(values > 100);
}

// every non-exchanger strategy active at once: the float delta accumulation order matters (EventData.Fire calls the listeners from the
// last registered to the first)
TEST_CASE("WP-36 all stock strategies active together: CurrencyModifierQuery deltas (0 ULP)") {
    World36 w;
    StrategyWorld sw(w);
    const json& top = Ref36()["strategies"];
    const std::vector<std::string> names = top["activeNames"].get<std::vector<std::string>>();
    for (const json& c : top["combined"]) {
        const float f = F(c["factor"]);
        std::vector<std::pair<std::string, float>> act;
        for (const std::string& n : names) act.emplace_back(n, f);
        sw.Activate(act);
        CHECK(sw.sys->StrategiesActive().size() == names.size());
        for (const json& q : c["queries"]) {
            CurrencyModifierQuery cq = CurrencyModifierQuery::RunQuery(ReasonOf(q["reason"].get<std::string>()), F(q["input"][0]), F(q["input"][1]), F(q["input"][2]));
            CheckQuery(q, cq, f);
        }
    }
}

// Strategy.Save / Load: the node format, active state, date and factor texts (G17 / G9), EFFECT sub nodes
TEST_CASE("WP-36 Strategy.Save / Load and StrategySystem.OnSave format") {
    World36 w;
    StrategyWorld sw(w);
    sw.Activate({{"RecoveryTransponders", 0.3f}, {"AgressiveNegotiations", 0.05f}});
    ConfigNode out;
    sw.sys->OnSave(out);
    ConfigNode* st = out.GetNode("STRATEGIES");
    REQUIRE(st != nullptr);
    REQUIRE(st->GetNodes("STRATEGY").size() == 2);
    ConfigNode* a = st->GetNodes("STRATEGY")[0];
    CHECK(*a->GetValue("name") == "AgressiveNegotiations");  // file order, not activation order
    CHECK(*a->GetValue("factor") == dotnet::FormatSingleG(0.05f, 9));
    CHECK(*a->GetValue("date") == "0");
    CHECK(a->GetNodes("EFFECT").size() == 3);
    CHECK(*a->GetNodes("EFFECT")[0]->GetValue("name") == "CurrencyOperation");
    ConfigNode* b = st->GetNodes("STRATEGY")[1];
    CHECK(*b->GetValue("name") == "RecoveryTransponders");
    CHECK(*b->GetValue("factor") == dotnet::FormatSingleG(0.3f, 9));
    CHECK(b->GetNodes("EFFECT").size() == 2);
    CHECK(*b->GetNodes("EFFECT")[1]->GetValue("name") == "ValueModifier");
    // round trip through a second system
    ConfigNode again;
    {
        StrategyWorld sw2(w);
        ConfigNode copy;
        out.CopyTo(copy);
        sw2.sys->LoadNow(copy);
        sw2.sys->OnSave(again);
    }
    CHECK(again.ToString() == out.ToString());
}

// Strategy.Activate / CanBeActivated: the costs are paid (StrategySetup), the Administration limits and the conflicting group tags apply
TEST_CASE("WP-36 Strategy.Activate pays the initial cost and respects the Administration limits") {
    World36 w;
    StrategyWorld sw(w);
    Funding funds;
    funds.Awake();
    Reputation rep;
    rep.Awake();
    rep.Start();
    CareerEnvironment& env = CareerEnvironment::Current();
    env.administration.present = true;
    env.administration.MaxActiveStrategies = 2;
    env.administration.MaxStrategyCommitLevel = 0.5f;
    sw.Activate({});
    Strategies::Strategy* tp = Find(*sw.sys, "RecoveryTransponders");
    REQUIRE(tp != nullptr);
    std::string reason;
    tp->SetFactor(0.6f);
    CHECK_FALSE(tp->CanBeActivated(reason));  // Factor > MaxStrategyCommitLevel
    tp->SetFactor(0.25f);
    env.administration.ActiveStrategyCount = 2;
    CHECK_FALSE(tp->CanBeActivated(reason));  // ActiveStrategyCount >= MaxActiveStrategies
    env.administration.ActiveStrategyCount = 0;
    funds.SetFunds(100.0, TransactionReasons::Cheating);
    CHECK_FALSE(tp->CanBeActivated(reason));  // not enough funds for initialCostFunds
    funds.SetFunds(1.0e6, TransactionReasons::Cheating);
    CHECK(tp->CanBeActivated(reason));
    const float cost = tp->InitialCostFunds();
    Planetarium::fetch->time = 5000.0;
    CHECK(tp->Activate());
    CHECK(tp->IsActive());
    CHECK(tp->DateActivated() == 5000.0);
    CHECK(funds.Funds() == 1.0e6 - static_cast<double>(cost));
    CHECK(tp->Deactivate());  // LeastDuration is 0 (sic): no minimum
    CHECK_FALSE(tp->IsActive());
}

// CurrencyExchanger.OnRegister: GetTotalInput / GetTotalOutput and the transactions (StrategyInput, StrategyOutput), then the strategy
// deactivates itself one frame later
TEST_CASE("WP-36 CurrencyExchanger strategies (BailoutGrant, researchIPsellout) vs the reference") {
    int rows = 0;
    for (const json& r : Ref36()["exchangers"]) {
        World36 w;
        w.SetCurves(0.9f, 1.3f);
        StrategyWorld sw(w);
        Funding funds;
        funds.Awake();
        Reputation rep;
        rep.Awake();
        rep.Start();
        funds.SetFunds(r["funds"].get<double>(), TransactionReasons::Cheating);
        rep.SetReputation(F(r["reputation"]), TransactionReasons::Cheating);
        CareerEnvironment& env = CareerEnvironment::Current();
        std::vector<float> adds;
        env.rndPresent = [] { return true; };
        env.rndScience = [&] { return F(r["science"]); };
        env.rndAddScience = [&](float v, TransactionReasons) { adds.push_back(v); };
        const std::string name = r["strategy"].get<std::string>();
        sw.Activate({{name, F(r["factor"])}});
        INFO(name, " factor ", F(r["factor"]), " input ", r["input"].get<std::string>());
        CHECK(funds.Funds() == r["fundsAfter"].get<double>());
        CHECK_F32(rep.reputation(), F(r["repAfter"]));
        REQUIRE(adds.size() == r["scienceAdds"].size());
        for (size_t i = 0; i < adds.size(); ++i) CHECK_F32(adds[i], F(r["scienceAdds"][i]));
        Strategies::Strategy* s = Find(*sw.sys, name);
        REQUIRE(s != nullptr);
        CHECK(s->IsActive());
        CareerEnvironment::Current().Scheduler().AdvanceFrame(0.02f);
        CareerEnvironment::Current().Scheduler().RunDelayedDynamicFrameRate();
        CHECK_FALSE(s->IsActive());  // DelayedCallback(1 frame): Parent.Deactivate()
        ++rows;
    }
    CHECK(rows == 18);
}

// ---- Funding / Reputation / CurrencyModifierQuery ------------------------------------------------------------------------------------

// Funding.AddFunds + Reputation.AddReputation with the stock strategies active, the AllowNegativeCurrency clamp, the OnCurrencyModified listeners
// (Funding before Reputation) applying the strategy deltas
TEST_CASE("WP-36 Funding / Reputation transactions with the stock strategies active (0 ULP vs the reference)") {
    for (const json& tx : Ref36()["transactions"]) {
        World36 w;
        w.SetCurves(F(tx["addCurve"]), F(tx["subCurve"]));
        w.game.Parameters->CustomParams<GameParameters::AdvancedParams>().AllowNegativeCurrency = tx["allowNegative"].get<bool>();
        StrategyWorld sw(w);
        const float f = F(tx["factor"]);
        std::vector<std::pair<std::string, float>> act;
        for (const std::string& n : Ref36()["strategies"]["activeNames"].get<std::vector<std::string>>()) act.emplace_back(n, f);
        sw.Activate(act);
        Funding funds;
        funds.Awake();
        Reputation rep;
        rep.Awake();
        rep.Start();
        CHECK(funds.Funds() == 25000.0);
        CHECK(rep.reputation() == 0.0f);
        const json& steps = tx["steps"];
        for (size_t i = 0; i < steps.size(); ++i) {
            const TransactionReasons reason = ReasonOf(steps[i]["reason"].get<std::string>());
            if (steps[i]["kind"].get<std::string>() == "f")
                funds.AddFunds(static_cast<double>(F(steps[i]["amount"])), reason);
            else
                rep.AddReputation(F(steps[i]["amount"]), reason);
            INFO("factor ", f, " allowNegative ", tx["allowNegative"].get<bool>(), " step ", i);
            CHECK(funds.Funds() == tx["after"][i]["funds"].get<double>());
            CHECK_F32(rep.reputation(), F(tx["after"][i]["rep"]));
        }
    }
}

// Funding.AddFunds fires the events in order and Funding.SetFunds / CanAfford / OnLoad / OnSave
TEST_CASE("WP-36 Funding events, CanAfford, SetFunds and the funds save node") {
    World36 w;
    Funding funds;
    funds.Awake();
    std::vector<std::pair<double, TransactionReasons>> seen;
    struct L {
        std::vector<std::pair<double, TransactionReasons>>* v;
        void On(double d, TransactionReasons r) { v->emplace_back(d, r); }
    } l{&seen};
    GE::OnFundsChanged.Add(&l, &L::On);
    funds.AddFunds(1000.0, TransactionReasons::ContractReward);
    funds.AddFunds(-99999.0, TransactionReasons::VesselRollout);
    GE::OnFundsChanged.Remove(&l, &L::On);
    REQUIRE(seen.size() == 2);
    CHECK(seen[0].first == 26000.0);
    CHECK(seen[0].second == TransactionReasons::ContractReward);
    CHECK(seen[1].first == 0.0);  // clamped (AllowNegativeCurrency = false)
    CHECK(Funding::CanAfford(0.0f));
    funds.SetFunds(500.25, TransactionReasons::Cheating);
    CHECK(Funding::CanAfford(500.25f));
    CHECK_FALSE(Funding::CanAfford(500.5f));
    ConfigNode node("SCENARIO");
    funds.OnSave(node);
    CHECK(*node.GetValue("funds") == "500.25");
    ConfigNode in("SCENARIO");
    in.AddValue("funds", "12345.678901234567");
    funds.OnLoad(in);
    CHECK(funds.Funds() == 12345.678901234567);
    ConfigNode out("SCENARIO");
    funds.OnSave(out);
    CHECK(*out.GetValue("funds") == dotnet::FormatDoubleG(12345.678901234567, 17));
}

// CurrencyModifierQuery.GetTotal / CanAfford / the text helpers (CurrencyModifierQuery.cs:86-:273)
TEST_CASE("WP-36 CurrencyModifierQuery totals, CanAfford and text") {
    World36 w;
    Funding funds;
    funds.Awake();
    funds.SetFunds(1000.0, TransactionReasons::Cheating);
    CurrencyModifierQuery q(TransactionReasons::VesselRollout, -800.0f, 0.0f, 0.0f);
    q.AddDelta(Currency::Funds, -150.5f);
    CHECK_F32(q.GetTotal(Currency::Funds), -950.5f);
    CHECK(q.CanAfford(Currency::Funds));
    q.AddDelta(Currency::Funds, -100.0f);
    CHECK_FALSE(q.CanAfford(Currency::Funds));
    Currency missing = Currency::Reputation;
    CHECK_FALSE(q.CanAfford([&](Currency c) { missing = c; }));
    CHECK(missing == Currency::Funds);
    CHECK(q.CanAfford(Currency::Reputation));
    // science needs ResearchAndDevelopment (WP-35): Instance null = affordable
    CurrencyModifierQuery sq(TransactionReasons::RnDTechResearch, 0.0f, -50.0f, 0.0f);
    CHECK(sq.CanAfford(Currency::Science));
    CareerEnvironment::Current().rndPresent = [] { return true; };
    CareerEnvironment::Current().rndScience = [] { return 49.94f; };
    CHECK_FALSE(sq.CanAfford(Currency::Science));  // RoundToPlaces(49.94, 1) = 49.9 < 50.0
    CareerEnvironment::Current().rndScience = [] { return 50.04f; };
    CHECK(sq.CanAfford(Currency::Science));
    // the deltas text: (+x) / (x) with the format, the percentage of the input
    CurrencyModifierQuery tq(TransactionReasons::ContractReward, 1000.0f, 0.0f, 0.0f);
    CHECK(tq.GetEffectDeltaText(Currency::Funds, "N1") == "");
    tq.AddDelta(Currency::Funds, 250.0f);
    CHECK(tq.GetEffectDeltaText(Currency::Funds, "N1") == "(+250.0)");
    CHECK(tq.GetEffectPercentageText(Currency::Funds, "N1") == "(+25.0%)");
    CHECK(tq.GetEffectDeltaText(Currency::Funds, "N1", CurrencyModifierQuery::TextStyling::OnGUI) == "<color=#caff00>(+250.0)</color>");
    tq.AddDelta(Currency::Funds, -1500.0f);
    CHECK(tq.GetEffectDeltaText(Currency::Funds, "N0") == "(-1,250)");
    ValueModifierQuery vq("x", 2.0f);
    vq.AddDelta(0.5f);
    CHECK(vq.GetEffectDeltaText("0.##") == "(+0.5)");
    CHECK(vq.GetEffectPercentageText("0.#") == "(+25%)");
}

// Reputation.cs:45-:112 AddReputation (granular), addReputation_discrete, SetReputation clamp, CurrentRep / UnitRep, OnLoad / OnSave
TEST_CASE("WP-36 Reputation granular and discrete adds, clamp, events and the rep save node") {
    World36 w;
    Reputation rep;
    rep.Awake();
    rep.Start();
    CHECK(Reputation::CurrentRep() == 0.0f);
    std::vector<float> changed;
    struct L {
        std::vector<float>* v;
        void On(float r, TransactionReasons) { v->push_back(r); }
    } l{&changed};
    GE::OnReputationChanged.Add(&l, &L::On);
    // constant curves 0.9 / 1.3: +3.5 = three steps of 0.9, then (3.5 - 2.7) * 0.9 (the remainder uses the APPLIED sum, sic)
    float want = 0.0f, wantApplied = 0.0f;
    want = GranularRef(0.0f, 3.5f, 0.9f, 1.3f, &wantApplied);
    const float applied = rep.AddReputation(3.5f, TransactionReasons::ContractReward);
    CHECK_F32(applied, wantApplied);
    CHECK_F32(rep.reputation(), want);
    CHECK(want > 3.41f);
    CHECK(want < 3.43f);
    CHECK_F32(Reputation::UnitRep(), static_cast<float>(fx(want) / fx(1000.0f)));
    rep.addReputation_discrete(-2.0f, TransactionReasons::VesselLoss);  // one step, subtraction curve
    CHECK_F32(rep.reputation(), static_cast<float>(fx(want) + fx(static_cast<float>(fx(-2.0f) * fx(1.3f)))));
    rep.SetReputation(5000.0f, TransactionReasons::Cheating);
    CHECK(rep.reputation() == 1000.0f);
    rep.SetReputation(-5000.0f, TransactionReasons::Cheating);
    CHECK(rep.reputation() == -1000.0f);
    CHECK(changed.size() >= 4);
    GE::OnReputationChanged.Remove(&l, &L::On);
    ConfigNode in("SCENARIO");
    in.AddValue("rep", "33.3333");
    rep.OnLoad(in);
    CHECK_F32(rep.reputation(), 33.3333f);
    ConfigNode out("SCENARIO");
    rep.OnSave(out);
    CHECK(*out.GetValue("rep") == dotnet::FormatSingleG(33.3333f, 9));
    // Reputation.OnCrewKilled: reputationKerbalDeath * RepLossMultiplier through VesselLoss
    // (the C# field initialiser is +10; the shipped prefab value is a Unity-serialized number this repo does not have: fixture -10)
    w.Career().RepLossMultiplier = 2.0f;
    w.gv->reputationKerbalDeath = -10.0f;
    rep.SetReputation(500.0f, TransactionReasons::Cheating);
    EventReport rpt(FlightEvents::CREW_KILLED, nullptr);
    rep.OnCrewKilled(&rpt);
    CHECK_F32(rep.reputation(), GranularRef(500.0f, -20.0f, 0.9f, 1.3f));
}

// CareerParams starting values: Funding / Reputation OnAwake
TEST_CASE("WP-36 Funding and Reputation start from the Career parameters") {
    World36 w;
    w.Career().StartingFunds = 61234.5f;
    w.Career().StartingReputation = -12.5f;
    Funding funds;
    funds.Awake();
    Reputation rep;
    rep.Awake();
    CHECK(funds.Funds() == 61234.5);
    CHECK(rep.reputation() == -12.5f);
}

// ---- facilities -----------------------------------------------------------------------------------------------------------------------

namespace {

// n+1 levels with the fixture costs of the reference
std::unique_ptr<Upgradeables::UpgradeableFacility> MakeFacility(int maxLevel, const std::string& id) {
    auto f = std::make_unique<Upgradeables::UpgradeableFacility>();
    const json& costs = Ref36()["facility"]["levelCosts"];
    for (int i = 0; i <= maxLevel; ++i) {
        Upgradeables::UpgradeLevel l;
        l.levelCost = F(costs[static_cast<size_t>(i)]);
        f->UpgradeLevels().push_back(l);
    }
    f->name = id;
    f->compiledId = "SpaceCenter/" + id;
    f->Awake();
    return f;
}

}  // namespace

// UpgradeableFacility.setNormLevel / GetNormLevel (UpgradeableFacility.cs:130-:141)
TEST_CASE("WP-36 UpgradeableFacility normalised level maths vs the reference (0 ULP)") {
    World36 w;
    int rows = 0;
    for (const json& r : Ref36()["facility"]["rows"]) {
        auto f = MakeFacility(r["maxLevel"].get<int>(), "VehicleAssemblyBuilding");
        CHECK(f->FacilityLevel() == f->MaxLevel());  // OnAwake: setLevel(MaxLevel)
        f->setNormLevel(F(r["norm"]));
        CHECK(f->FacilityLevel() == r["level"].get<int>());
        CHECK_F32(f->GetNormLevel(), F(r["normBack"]));
        ++rows;
    }
    CHECK(rows >= 69);
}

// UpgradeableFacility.GetUpgradeCost / GetDowngradeCost (:143-:166): scene is game / not game, FundsLossMultiplier
TEST_CASE("WP-36 UpgradeableFacility upgrade / downgrade cost (0 ULP)") {
    World36 w;
    for (const json& r : Ref36()["facility"]["prices"]) {
        auto f = MakeFacility(r["maxLevel"].get<int>(), "Administration");
        f->setLevel(r["level"].get<int>());
        REQUIRE(f->FacilityLevel() == r["level"].get<int>());
        w.Career().FundsLossMultiplier = F(r["mult"]);
        SaveEnvironment::Current().loadedSceneIsGame = true;
        CHECK_F32(f->GetUpgradeCost(), F(r["upGame"]));
        CHECK_F32(f->GetDowngradeCost(), F(r["downGame"]));
        SaveEnvironment::Current().loadedSceneIsGame = false;
        CHECK_F32(f->GetUpgradeCost(), F(r["upFree"]));
        CHECK_F32(f->GetDowngradeCost(), F(r["downFree"]));
    }
}

// ScenarioUpgradeableFacilities: register / load / save / GetFacilityLevel(Count), SlashSanitize, the initial state of a new game
TEST_CASE("WP-36 ScenarioUpgradeableFacilities register, load, save and level queries") {
    World36 w;
    CHECK(ScenarioUpgradeableFacilities::SlashSanitize("Administration") == "SpaceCenter/Administration");
    CHECK(ScenarioUpgradeableFacilities::SlashSanitize("SpaceCenter/Runway") == "SpaceCenter/Runway");
    CHECK(ScenarioUpgradeableFacilities::IsRunway("Island_Airfield"));
    CHECK_FALSE(ScenarioUpgradeableFacilities::IsRunway("LaunchPad"));
    CHECK(ScenarioUpgradeableFacilities::IsLaunchPad("Woomerang_Launch_Site"));
    // no facility registered: level 1, count 1
    CHECK(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::Administration) == 1.0f);
    CHECK(ScenarioUpgradeableFacilities::GetFacilityLevelCount(SpaceCenterFacility::Administration) == 1);
    ScenarioUpgradeableFacilities mod;
    mod.Awake();
    // a new game: no child nodes, the initial state is every upgradeable object at lvl 0 (PSystemSetup.SpaceCenterFacilities seam)
    CareerEnvironment::Current().spaceCenterFacilities = [] {
        std::vector<CareerEnvironment::SpaceCenterFacilityInit> v;
        v.push_back({"Administration", "SpaceCenter/Administration", "SpaceCenter/Administration", 2});
        v.push_back({"Runway", "SpaceCenter/Runway", "SpaceCenter/Runway", 2});
        return v;
    };
    ConfigNode fresh("SCENARIO");
    mod.OnLoad(fresh);
    CHECK(fresh.CountNodes() == 2);
    CHECK(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::Administration) == 0.0f);
    CHECK(ScenarioUpgradeableFacilities::GetFacilityLevelCount(SpaceCenterFacility::Administration) == -1);  // no facility ref yet
    // a saved game: lvl values for facilities without a registered object
    ScenarioUpgradeableFacilities::ClearStatics();
    ConfigNode saved("SCENARIO");
    saved.AddNode("SpaceCenter/Administration")->AddValue("lvl", 0.5f);
    saved.AddNode("SpaceCenter/ResearchAndDevelopment")->AddValue("lvl", 1.0f);
    mod.OnLoad(saved);
    CHECK(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::Administration) == 0.5f);
    CHECK(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::ResearchAndDevelopment) == 1.0f);
    CHECK(ScenarioUpgradeableFacilities::GetFacilityLevel("SpaceCenter/Unknown") == 1.0f);
    ConfigNode out("SCENARIO");
    mod.OnSave(out);
    CHECK(out.ToString() == saved.ToString());
    // a facility object registers and takes the saved level (RegisterUpgradeable -> facilityRef.Load), then the save is written by the object
    auto fac = MakeFacility(2, "Administration");
    fac->id = "SpaceCenter/Administration";
    fac->preCompiledId = true;
    CHECK(fac->RegisterInstance());
    CHECK(fac->FacilityLevel() == 1);  // lvl 0.5 -> floor(0.5 * 3) = 1
    CHECK(ScenarioUpgradeableFacilities::GetFacilityLevelCount(SpaceCenterFacility::Administration) == 2);
    fac->SetLevel(2);
    CHECK(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::Administration) == 1.0f);
    ConfigNode out2("SCENARIO");
    mod.OnSave(out2);
    CHECK(*out2.GetNode("SpaceCenter/Administration")->GetValue("lvl") == "1");
    CHECK(*out2.GetNode("SpaceCenter/ResearchAndDevelopment")->GetValue("lvl") == "1");
    fac->UnregisterInstance();
    CHECK(ScenarioUpgradeableFacilities::GetFacilityLevelCount(SpaceCenterFacility::Administration) == -1);
    // the facility upgrade events
    std::vector<int> events;
    struct L {
        std::vector<int>* v;
        void Up(Upgradeables::UpgradeableFacility*, int lvl) { v->push_back(lvl); }
    } l{&events};
    GE::OnKSCFacilityUpgrading.Add(&l, &L::Up);
    fac->SetLevel(0);
    GE::OnKSCFacilityUpgrading.Remove(&l, &L::Up);
    CHECK(events == std::vector<int>{0});
    mod.CheatFacilities();
}

// KSCFacilityLevelText.GetText: the [Tag] replacement against GameVariables
TEST_CASE("WP-36 KSCFacilityLevelText tag replacement") {
    World36 w;
    Upgradeables::KSCFacilityLevelText t;
    t.facility = SpaceCenterFacility::VehicleAssemblyBuilding;
    t.textBase = "Parts: [CraftParts]\n[ActionGroupsStock]\nMass: [CraftMass]";
    // level 0: 30 parts, no action groups (the empty line is removed), 18.0 t
    const std::string t0 = t.GetText(0.0f);
    CHECK(t0 == "*Parts: 30\n*Mass: 18.0" + Localizer36::Format("#autoLOC_7001407"));
    // level 1: unlimited
    const std::string t1 = t.GetText(1.0f);
    CHECK(t1 == "*Parts: " + Localizer36::Format("#autoLOC_416922") + "\n*" + Localizer36::Format("#autoLOC_6002244") + "\n*Mass: " +
                    Localizer36::Format("#autoLOC_416922"));
    t.textBase = "[level] [level:LaunchPad] [bogus]";
    // no facility objects registered: GetFacilityLevelCount = 1 -> level * 1
    CHECK(t.GetText(1.0f) == "*1 1");  // the text is Trimmed before the line prefix is added
}

// ---- recovery ----------------------------------------------------------------------------------------------------------------------

namespace {

struct RecoveryWorld {
    World36& w;
    std::array<double, 3> ksc;
    explicit RecoveryWorld(World36& world) : w(world) {
        const json& rec = Ref36()["recovery"];
        ksc = {rec["ksc"][0].get<double>(), rec["ksc"][1].get<double>(), rec["ksc"][2].get<double>()};
        CareerEnvironment& env = CareerEnvironment::Current();
        env.spaceCenterBody = [this] { return &w.Body("Kerbin"); };
        env.spaceCenterGreatCircleDistance = [this](double lat, double lon) {
            const Vector3d v = w.Body("Kerbin").GetRelSurfaceNVector(lat, lon);
            return w.Body("Kerbin").Radius * dotnet::Math::Acos(ksc[0] * v.x + ksc[1] * v.y + ksc[2] * v.z);  // SpaceCenter.GreatCircleDistance
        };
    }
};

}  // namespace

// VesselRecovery.OnVesselRecovered: the distance factor (Mathf.Lerp between the min and max recovery fractions, the strategy ValueModifier
// deltas, the KSC / Runway / LaunchPad overrides) over a lat / lon grid around Kerbin with the real radius and the KSC position
TEST_CASE("WP-36 VesselRecovery factor over a distance grid with the real Kerbin radius and KSC position (0 ULP)") {
    World36 w;
    RecoveryWorld rw(w);
    const json& rec = Ref36()["recovery"];
    CHECK(w.Body("Kerbin").Radius == rec["radius"].get<double>());
    int rows = 0;
    double worstRel = 0.0;
    for (const json& r : rec["rows"]) {
        const double dist = CareerEnvironment::Current().spaceCenterGreatCircleDistance(r["lat"].get<double>(), r["lon"].get<double>());
        const double want = r["distance"].get<double>();
        worstRel = std::max(worstRel, std::fabs(dist - want) / std::max(1.0, std::fabs(want)));
        const VesselRecovery::RecoveryFactor rf = VesselRecovery::ComputeRecoveryFactor(dist, w.Body("Kerbin").Radius * 3.141592653589793, F(r["minDelta"]),
                                                                                        F(r["maxDelta"]), r["landedAt"].get<std::string>());
        CHECK_F32(rf.num3, F(r["num3"]));
        CHECK_F32(rf.num4, F(r["num4"]));
        ++rows;
    }
    CHECK(rows > 1000);
    CHECK(worstRel < 1e-12);  // the double distance (CRT sin/cos/acos vs numpy libm): last-digit differences only
    INFO("worst relative distance difference ", worstRel);
}

// the whole OnVesselRecovered pipeline on a ProtoVessel: ValueModifierQuery with the active RecoveryTransponders, the events, the dialog data
TEST_CASE("WP-36 VesselRecovery.OnVesselRecovered pipeline: events, strategy value deltas and dialog") {
    World36 w;
    RecoveryWorld rw(w);
    CareerEnvironment::Current().launchSites = [] { return std::vector<CareerEnvironment::LaunchSiteInfo>{{"Island_Airfield", false, nullptr, "", ""}}; };
    StrategyWorld sw(w);
    Funding funds;
    funds.Awake();
    Reputation rep;
    rep.Awake();
    rep.Start();
    VesselRecovery rec;
    rec.Awake();
    sw.Activate({{"RecoveryTransponders", 0.5f}});
    std::vector<float> scores, completes;
    struct L {
        std::vector<float>* a;
        std::vector<float>* b;
        void Proc(ProtoVessel*, KSP::UI::Screens::MissionRecoveryDialog*, float s) { a->push_back(s); }
        void Done(ProtoVessel*, KSP::UI::Screens::MissionRecoveryDialog*, float s) { b->push_back(s); }
    } l{&scores, &completes};
    GE::onVesselRecoveryProcessing.Add(&l, &L::Proc);
    GE::onVesselRecoveryProcessingComplete.Add(&l, &L::Done);
    ConfigNode vn("VESSEL");
    ProtoVessel pv(vn, &w.game);
    pv.latitude = 30.0;
    pv.longitude = 100.0;
    pv.landedAt = "";
    rec.OnVesselRecovered(&pv, false);
    REQUIRE(scores.size() == 1);
    REQUIRE(completes.size() == 1);
    CHECK(scores[0] == completes[0]);
    // the score equals ComputeRecoveryFactor with the live ValueModifier deltas (0.05 .. 0.15 and -0.06 .. -0.10 at factor 0.5)
    const double dist = CareerEnvironment::Current().spaceCenterGreatCircleDistance(30.0, 100.0);
    ValueModifierQuery qmin = ValueModifierQuery::RunQuery("RecoveryMinimumDelta", 1.0f);
    ValueModifierQuery qmax = ValueModifierQuery::RunQuery("RecoveryMaximumDelta", 1.0f);
    CHECK(qmin.GetEffectDelta() > 0.0f);
    CHECK(qmax.GetEffectDelta() < 0.0f);
    const VesselRecovery::RecoveryFactor rf = VesselRecovery::ComputeRecoveryFactor(dist, w.Body("Kerbin").Radius * 3.141592653589793, qmin.GetEffectDelta(),
                                                                                    qmax.GetEffectDelta(), "");
    CHECK_F32(scores[0], rf.num3);
    REQUIRE(rec.lastDialog() != nullptr);
    CHECK(rec.lastDialog()->recoveryLocation == Localizer36::Format("#autoLOC_303276", {career::FormatNumber(dist * 0.0010000000474974513, false, "0.0")}));
    CHECK(rec.lastDialog()->beforeMissionFunds == 25000.0);
    // a launch site match gives 1.0 (checkLaunchSites): Island_Airfield
    pv.landedAt = "Island_Airfield";
    scores.clear();
    rec.OnVesselRecovered(&pv, true);
    REQUIRE(scores.size() == 1);
    CHECK(scores[0] == 1.0f);
    pv.landedAt = "KSC";
    scores.clear();
    rec.OnVesselRecovered(&pv, true);
    CHECK_F32(scores[0], 0.98f);
    GE::onVesselRecoveryProcessing.Remove(&l, &L::Proc);
    GE::onVesselRecoveryProcessingComplete.Remove(&l, &L::Done);
}

// Funding.onVesselRecoveryProcessing / Reputation.onvesselRecoveryProcessing on the real vessels of the physprofile save: the funds equal the
// part costs times the score, the widgets list every part
TEST_CASE("WP-36 recovery funds and reputation for the vessels of the physprofile save") {
    World36 w;
    ConfigNode::NodePtr save = ConfigNode::Load(kSaves36 + "/physprofile/persistent.sfs");
    REQUIRE(save);
    ConfigNode* game = save->GetNode("GAME");
    REQUIRE(game != nullptr);
    ConfigNode* flight = game->GetNode("FLIGHTSTATE");
    REQUIRE(flight != nullptr);
    auto vessels = flight->GetNodes("VESSEL");
    REQUIRE(!vessels.empty());
    Funding funds;
    funds.Awake();
    Reputation rep;
    rep.Awake();
    rep.Start();
    int checked = 0;
    for (ConfigNode* vn : vessels) {
        ProtoVessel pv(*vn, &w.game);
        const std::vector<ProtoPartSnapshot*> parts = pv.GetAllProtoPartsIncludingCargo();
        if (parts.empty()) continue;
        double expected = 0.0;
        const float score = 0.75f;
        KSP::UI::Screens::MissionRecoveryDialog dlg;
        for (ProtoPartSnapshot* p : parts) {
            const AvailablePart* ap = p->partInfo != nullptr ? p->partInfo : w.g.data.parts.getPartInfoByName(p->partName.value_or(""));
            if (ap == nullptr) continue;
            float dry = 0.0f, fuel = 0.0f;
            ShipConstruction::GetPartCosts(*p, false, ap, dry, fuel);
            dry = static_cast<float>(fx(dry) * fx(score));
            fuel = static_cast<float>(fx(fuel) * fx(score));
            expected += fx(dry) + fx(fuel);
        }
        funds.SetFunds(1000.0, TransactionReasons::Cheating);
        funds.onVesselRecoveryProcessing(&pv, &dlg, score);
        CHECK(dlg.fundsEarned == expected);
        CHECK(funds.Funds() == 1000.0 + expected);
        CHECK(!dlg.partWidgets.empty());
        // Reputation: one AddReputation(reputationKerbalRecovery * RepGainMultiplier) per crew member
        rep.SetReputation(0.0f, TransactionReasons::Cheating);
        const int crew = static_cast<int>(pv.GetVesselCrew().size());
        rep.onvesselRecoveryProcessing(&pv, &dlg, score);
        float want = 0.0f;
        for (int i = 0; i < crew; ++i) {
            // granular add of 25 with the constant 0.9 curve: 25 steps of 0.9 + the remainder 0
            float r = want;
            float n2 = 0.0f;
            for (int k = 0; k <= 25; ++k) {
                float n3 = (k == 25) ? static_cast<float>(fx(25.0f) - fx(n2)) * 0.9f : 0.9f;
                r = static_cast<float>(fx(r) + fx(n3));
                n2 = static_cast<float>(fx(n2) + fx(n3));
            }
            want = r;
        }
        CHECK_F32(rep.reputation(), want);
        ++checked;
    }
    CHECK(checked >= 1);
}

// ---- ScenarioDestructibles / ScenarioAchievements ---------------------------------------------------------------------------------------

namespace {

struct FakeBuilding : IDestructibleBuilding {
    std::string id;
    bool intact = true;
    float damage = 0.25f;
    int loads = 0;
    const std::string& BuildingId() const override { return id; }
    void Save(ConfigNode& node) override { node.AddValue("intact", intact); }
    void Load(const ConfigNode& node) override {
        ++loads;
        if (node.HasValue("intact")) intact = *node.GetValue("intact") == "True";
    }
    bool IsIntact() const override { return intact; }
    float FacilityDamageFraction() const override { return damage; }
};

}  // namespace

// ScenarioDestructibles.cs:133-:253 on the nodes of the physprofile save
TEST_CASE("WP-36 ScenarioDestructibles: facility names, register / unregister, damage") {
    World36 w;
    CHECK(ScenarioDestructibles::GetFacility("SpaceCenter/LaunchPad/Facility/LaunchPadMedium/ksp_pad_cylTank") == "SpaceCenter/LaunchPad");
    CHECK(ScenarioDestructibles::GetFacility("SpaceCenter/Administration") == "SpaceCenter/Administration");
    CHECK(ScenarioDestructibles::GetFacility("x/y") == "x/y");
    ScenarioDestructibles mod;
    mod.Awake();
    ConfigNode::NodePtr save = ConfigNode::Load(kSaves36 + "/physprofile/persistent.sfs");
    REQUIRE(save);
    ConfigNode* scn = nullptr;
    for (ConfigNode* n : save->GetNode("GAME")->GetNodes("SCENARIO"))
        if (n->GetValue("name") && *n->GetValue("name") == "ScenarioDestructibles") scn = n;
    REQUIRE(scn != nullptr);
    mod.OnLoad(*scn);
    CHECK(ScenarioDestructibles::protoDestructibles().Count() == scn->CountNodes());
    CHECK(ScenarioDestructibles::GetFacilityDamage("LaunchPad") == 0.0f);  // everything intact in the save
    // a building registers, takes the saved state and reports damage when it is destroyed
    FakeBuilding b;
    b.id = scn->nodes()[0]->name;
    b.intact = false;
    CHECK(ScenarioDestructibles::RegisterDestructible(&b, b.id));
    CHECK(b.loads == 1);
    CHECK(b.intact);  // restored from the saved `intact = True`
    b.intact = false;
    const std::string fac = ScenarioDestructibles::GetFacility(b.id);
    CHECK_F32(ScenarioDestructibles::GetFacilityDamage(fac), 0.25f);
    ConfigNode out("SCENARIO");
    mod.OnSave(out);
    CHECK(*out.GetNode(b.id)->GetValue("intact") == "False");
    ScenarioDestructibles::UnregisterDestructible(&b, b.id, true);
    // a building the save does not know is added with its own state
    FakeBuilding nb;
    nb.id = "SpaceCenter/Newplace/Facility/x";
    nb.intact = false;
    CHECK_FALSE(ScenarioDestructibles::RegisterDestructible(&nb, nb.id));
    CHECK_F32(ScenarioDestructibles::GetFacilityDamage("SpaceCenter/Newplace"), 0.25f);
    ScenarioDestructibles::UnregisterDestructible(&nb, nb.id, false);
}

namespace {

struct RecordingProvider : ScenarioAchievements::IAchievementProvider {
    std::vector<std::string> set;
    std::map<std::string, bool> done;
    void SetAchievementComplete(const std::string& id) override {
        set.push_back(id);
        done[id] = true;
    }
    bool GetAchievementComplete(const std::string& id) override { return done[id]; }
};

}  // namespace

// ScenarioAchievements.cs:152 CheckAchievement + the body-name driven handlers (no Vessel needed for Flyby / ReturnFromOrbit)
TEST_CASE("WP-36 ScenarioAchievements provider and body-name achievements") {
    World36 w;
    RecordingProvider prov;
    ScenarioAchievements::SetProvider(&prov);
    ScenarioAchievements mod;
    mod.Awake();
    CHECK(ScenarioAchievements::achievements().size() == 44);
    CHECK_FALSE(mod.DestroyRequested());
    GE::VesselSituation::onFlyBy.Fire(nullptr, &w.Body("Eeloo"));
    GE::VesselSituation::onFlyBy.Fire(nullptr, &w.Body("Mun"));
    GE::VesselSituation::onFlyBy.Fire(nullptr, &w.Body("Mun"));  // already complete: the provider is asked once
    GE::VesselSituation::onFlyBy.Fire(nullptr, w.planetarium.Sun);  // Sun: ignored
    GE::VesselSituation::onReturnFromOrbit.Fire(nullptr, &w.Body("Duna"));
    GE::VesselSituation::onReturnFromOrbit.Fire(nullptr, &w.Body("Kerbin"));  // home world: ignored
    GE::VesselSituation::onTargetedLanding.Fire(nullptr, std::string("KSC/LaunchPad"), KSPAchievements::ReturnFrom::Orbit);
    GE::VesselSituation::onTargetedLanding.Fire(nullptr, std::string("Runway"), KSPAchievements::ReturnFrom::Orbit);  // not the pad
    GE::VesselSituation::onTargetedLanding.Fire(nullptr, std::string("LaunchPad"), KSPAchievements::ReturnFrom::Flight);  // wrong level
    CHECK(prov.set == std::vector<std::string>{"FirstEelooFlyby", "FlybyMun", "ReturnFromDuna", "TargetedLaunchPadLanding"});
    // a SCENARIO game mode destroys the module (Object.Destroy(this)) but OnAwake continues
    w.game.Mode = Game::Modes::SCENARIO;
    ScenarioAchievements mod2;
    mod2.Awake();
    CHECK(mod2.DestroyRequested());
    ScenarioAchievements::SetProvider(nullptr);
}

// ---- progress tracking ----------------------------------------------------------------------------------------------------------------------

namespace {

struct TestNode : ProgressNode {
    TestNode() : ProgressNode("TestNode", false) {}
    using ProgressNode::AwardProgress;
    using ProgressNode::AwardProgressInterval;
    using ProgressNode::AwardProgressStandard;
    using ProgressNode::CrewSensitiveComplete;
};

}  // namespace

// ProgressNode Reach / Complete / Load / Save (ProgressNode.cs:47-:143)
TEST_CASE("WP-36 ProgressNode reach, complete, load and save formats") {
    World36 w;
    Planetarium::fetch->time = 1234.5;
    TestNode n;
    CHECK_FALSE(n.IsReached());
    ConfigNode out0;
    n.Save(out0);
    CHECK(out0.CountValues() == 0);  // not reached: nothing
    n.Reach();
    CHECK(n.IsReached());
    CHECK(n.AchieveDate == 1234.5);
    ConfigNode out1;
    n.Save(out1);
    CHECK(*out1.GetValue("reached") == "1234.5");
    Planetarium::fetch->time = 2000.0;
    n.Complete();
    CHECK(n.IsComplete());
    ConfigNode out2;
    n.Save(out2);
    CHECK(*out2.GetValue("completed") == "2000");  // neither manned nor unmanned flag: "completed"
    // Load: completedManned / completedUnmanned / completed / reached
    struct Case {
        const char* key;
        bool manned, unmanned, complete;
    } cases[] = {{"reached", false, false, false}, {"completed", true, true, true}, {"completedManned", true, false, true}, {"completedUnmanned", false, true, true}};
    for (const Case& c : cases) {
        TestNode m;
        ConfigNode node;
        node.AddValue(c.key, 77.25);
        m.Load(node);
        CHECK(m.IsReached());
        CHECK(m.IsComplete() == c.complete);
        CHECK(m.IsCompleteManned() == c.manned);
        CHECK(m.IsCompleteUnmanned() == c.unmanned);
        CHECK(m.AchieveDate == 77.25);
        ConfigNode back;
        m.Save(back);
        CHECK(back.ToString() == node.ToString());
    }
    // the summary line
    TestNode s;
    s.Reach();
    CHECK(s.GetNodeSummary("") == "TestNode=2000.0");
    // CheatComplete
    TestNode c;
    c.CheatComplete();
    CHECK(c.IsComplete());
    CHECK(c.IsCompleteManned());
    CHECK(c.IsCompleteUnmanned());
}

// ProgressNode.AwardProgress*: career mode only, Funding / Reputation / R&D, the WP-38 reward seam
TEST_CASE("WP-36 ProgressNode.AwardProgress pays through Funding, Reputation and the R&D seam") {
    World36 w;
    Funding funds;
    funds.Awake();
    Reputation rep;
    rep.Awake();
    rep.Start();
    CareerEnvironment& env = CareerEnvironment::Current();
    std::vector<float> sci;
    env.rndPresent = [] { return true; };
    env.rndAddScience = [&](float v, TransactionReasons r) {
        CHECK(r == TransactionReasons::Progression);
        sci.push_back(v);
    };
    env.worldFirstStandardReward = [](FinePrint::Utilities::ProgressRewardType, Currency c, FinePrint::Utilities::ProgressType p, CelestialBody*) {
        CHECK(p == FinePrint::Utilities::ProgressType::FIRSTLAUNCH);
        return c == Currency::Funds ? 1000.0f : (c == Currency::Science ? 5.0f : 2.0f);
    };
    env.worldFirstIntervalReward = [](FinePrint::Utilities::ProgressRewardType, Currency c, FinePrint::Utilities::ProgressType, CelestialBody*, int cur, int total) {
        return static_cast<float>(cur) * 10.0f + static_cast<float>(total) + (c == Currency::Funds ? 0.0f : 0.5f);
    };
    std::string msg;
    env.worldFirstMessage = [&](const std::string& d, float f, float s, float r) { msg = d + "|" + std::to_string(f) + "|" + std::to_string(s) + "|" + std::to_string(r); };
    TestNode n;
    n.AwardProgressStandard("First", FinePrint::Utilities::ProgressType::FIRSTLAUNCH);
    CHECK(funds.Funds() == 26000.0);
    CHECK_F32(rep.reputation(), GranularRef(0.0f, 2.0f, 0.9f, 1.3f));
    REQUIRE(sci.size() == 1);
    CHECK(sci[0] == 5.0f);
    CHECK(msg.rfind("First|", 0) == 0);
    n.AwardProgressInterval("Rec", 2, 10, FinePrint::Utilities::ProgressType::ALTITUDERECORD);
    CHECK(funds.Funds() == 26000.0 + 30.0);
    // a sandbox game pays nothing
    w.game.Mode = Game::Modes::SANDBOX;
    n.AwardProgress("x", 500.0f, 5.0f, 5.0f);
    CHECK(funds.Funds() == 26030.0);
    CHECK(sci.size() == 2);  // the Interval call paid science; the sandbox call did not
}

// ProgressTracking: the tree, the Progress node format, find / cheat, the body subtrees linked from the home world
TEST_CASE("WP-36 ProgressTracking tree, load / save of every node kind and the body subtree logic") {
    World36 w;
    ProgressTracking pt;
    pt.Awake();
    REQUIRE(pt.achievementTree != nullptr);
    // 11 top level nodes + 50 points of interest + 17 body subtrees
    CHECK(pt.achievementTree->Count() == 11 + 50 + static_cast<int>(w.sys->SpawnOrder().size()));
    CHECK(pt.celestialBodyHome != nullptr);
    CHECK(pt.celestialBodyHome->Body == &w.Body("Kerbin"));
    // the per-body subtree node ids (CelestialBodySubtree.cs:56-:100)
    KSPAchievements::CelestialBodySubtree* kerbin = pt.GetBodyTree("Kerbin");
    KSPAchievements::CelestialBodySubtree* mun = pt.GetBodyTree(&w.Body("Mun"));
    REQUIRE(kerbin != nullptr);
    REQUIRE(mun != nullptr);
    CHECK(kerbin->Subtree().Count() == 14);  // home world: no Flight / Suborbit / Flyby / ReturnFromFlyby / ReturnFromSurface
    CHECK(mun->Subtree().Count() == 19);
    CHECK(mun->Subtree()["ReturnFromFlyBy"] != nullptr);
    CHECK(mun->Subtree()["ReturnFromSurface"] != nullptr);
    CHECK(kerbin->Subtree()["ReturnFromOrbit"] != nullptr);
    CHECK(pt.FindNode({"Mun", "Orbit"}) == mun->orbit);
    CHECK(pt.FindNode({"Mun", "Nothing"}) == nullptr);
    CHECK(pt.FindNode({"FirstLaunch", "x"}) == nullptr);
    CHECK_FALSE(pt.NodeReached({}));
    CHECK(w.Body("Mun").progressTree == mun);
    // linking: Kerbin's children are its moons + its parent body (the Sun), sorted by RecoveryValue
    CHECK(kerbin->childTrees.size() >= 2);
    CHECK(mun->parentTree == kerbin);
    // events: the subtree reaches when one of its nodes is reached, and completes when all are
    pt.Start();
    Planetarium::fetch->time = 500.0;
    CHECK_FALSE(mun->IsReached());
    mun->orbit->Reach();
    CHECK(mun->IsReached());
    CHECK(pt.NodeReached({"Mun"}));
    CHECK_FALSE(pt.NodeComplete({"Mun"}));
    int count = mun->Subtree().Count();
    while (count-- > 0) mun->Subtree()[count]->Complete();
    // the subtree is checked on OnProgressReached only, and Complete() reaches before it marks the node complete (sic): the subtree
    // completes on the next Reach of any child
    CHECK_FALSE(mun->IsComplete());
    mun->orbit->Reach();
    CHECK(mun->IsComplete());
    // Save: only reached nodes, `Progress { Mun { <child nodes> ... } }`
    ConfigNode out("SCENARIO");
    pt.OnSave(out);
    ConfigNode* prog = out.GetNode("Progress");
    REQUIRE(prog != nullptr);
    REQUIRE(prog->GetNode("Mun") != nullptr);
    CHECK(prog->GetNode("Mun")->GetNode("Orbit") != nullptr);
    CHECK(prog->GetNode("Mun")->GetNode("Orbit")->GetNode("vessel") != nullptr);  // CelestialBodyOrbit.OnSave always writes `vessel`
    CHECK(*prog->GetNode("Mun")->GetValue("completed") == "500");
    CHECK(prog->GetNode("Kerbin") == nullptr);
    // a second tracker reloads it byte-exact
    pt.OnDestroy();
    ProgressTracking p2;
    p2.Awake();
    ConfigNode copy("SCENARIO");
    out.CopyTo(copy);
    p2.OnLoad(copy);
    ConfigNode out2("SCENARIO");
    p2.OnSave(out2);
    CHECK(out2.ToString() == out.ToString());
    CHECK(p2.NodeComplete({"Mun"}));
    CHECK(p2.GenerateSummary().find("MunOrbit=500.0") != std::string::npos);  // the subtree summary lists its reached children with the body name as base
    // cheating completes every node except the non cheatable launch sites
    p2.CheatProgression();
    CHECK(p2.NodeComplete({"FirstLaunch"}));
    CHECK(p2.NodeCompleteManned({"Duna", "Landing"}));
    CHECK_FALSE(p2.achievementTree->operator[]("POIKerbinCrater_Launch_Site")->IsComplete());
    p2.OnDestroy();
}

// every node kind's own OnLoad / OnSave (vessel / crew / base / station / record) and the records' state
TEST_CASE("WP-36 achievement node formats: records, vessel and crew references") {
    World36 w;
    ProgressTracking pt;
    pt.Awake();
    ConfigNode in("Progress");
    ConfigNode* fl = in.AddNode("FirstLaunch");
    fl->AddValue("completed", 12.0);
    ConfigNode* tb = in.AddNode("TowerBuzz");
    tb->AddValue("reached", 1.0);
    ConfigNode* v = tb->AddNode("vessel");
    v->AddValue("name", "Buzz One");
    v->AddValue("flag", "Squad/Flags/default");
    ConfigNode* ra = in.AddNode("RecordsAltitude");
    ra->AddValue("reached", 99.5);
    ra->AddValue("record", 12345.678);
    ConfigNode* rs = in.AddNode("RecordsSpeed");
    rs->AddValue("completedManned", 5.0);
    rs->AddValue("record", 2500.0);
    ConfigNode* poi = in.AddNode("POIKerbinUFO");
    poi->AddValue("completed", 3.0);
    poi->AddNode("vessel")->AddValue("name", "Seeker");
    poi->GetNode("vessel")->AddValue("flag", "");
    ConfigNode* kerb = in.AddNode("Kerbin");
    kerb->AddValue("reached", 7.0);
    ConfigNode* orb = kerb->AddNode("Orbit");
    orb->AddValue("completedUnmanned", 8.0);
    orb->AddNode("vessel")->AddValue("name", "Sat");
    orb->GetNode("vessel")->AddValue("flag", "f");
    ConfigNode* rec = kerb->AddNode("ReturnFromOrbit");
    rec->AddValue("reached", 9.0);
    rec->AddNode("vessel")->AddValue("name", "Cap");
    rec->GetNode("vessel")->AddValue("flag", "g");
    ConfigNode scenario("SCENARIO");
    scenario.AddNode("Progress");
    for (int i = 0; i < in.CountNodes(); ++i) scenario.GetNode("Progress")->AddNode(in.nodes().Shared(i));
    ConfigNode copy("SCENARIO");
    scenario.CopyTo(copy);
    pt.OnLoad(copy);
    CHECK(pt.NodeComplete({"FirstLaunch"}));
    CHECK(pt.NodeCompleteManned({"FirstLaunch"}));
    CHECK(pt.NodeCompleteManned({"RecordsSpeed"}));
    CHECK_FALSE(pt.NodeCompleteUnmanned({"RecordsSpeed"}));
    CHECK(pt.altitudeRecords->record == 12345.678);
    CHECK_FALSE(pt.altitudeRecords->IsComplete());
    CHECK(pt.NodeCompleteUnmanned({"Kerbin", "Orbit"}));
    ConfigNode out("SCENARIO");
    pt.OnSave(out);
    // the file order of the tree: FirstLaunch, ..., RecordsAltitude, ..., RecordsSpeed, TowerBuzz, POIKerbinUFO, Kerbin
    CHECK(out.GetNode("Progress")->ToString() == scenario.GetNode("Progress")->ToString());
}

// ---- the physprofile save --------------------------------------------------------------------------------------------------------------------

// The SCENARIO nodes of the (sandbox) physprofile save that belong to this package round-trip byte-exact through the live module
// (ScenarioRunner.AddModule -> Load, then ProtoScenarioModule(module) = Save). The save has ProgressTracking (empty Progress),
// ScenarioAchievements, ScenarioDestructibles (23 intact flags) and VesselRecovery; it has no Funding, Reputation,
// ScenarioUpgradeableFacilities or StrategySystem node (sandbox).
TEST_CASE("WP-36 SCENARIO nodes of the physprofile save round-trip byte-exact") {
    World36 w;
    RegisterCareerScenarioModules();
    ConfigNode::NodePtr save = ConfigNode::Load(kSaves36 + "/physprofile/persistent.sfs");
    REQUIRE(save);
    ConfigNode* game = save->GetNode("GAME");
    REQUIRE(game != nullptr);
    const std::set<std::string> ours = {"Funding", "Reputation", "ProgressTracking", "ScenarioUpgradeableFacilities", "StrategySystem",
                                        "ScenarioAchievements", "ScenarioDestructibles", "VesselRecovery"};
    ScenarioRunner runner;
    std::set<std::string> present;
    for (ConfigNode* sn : game->GetNodes("SCENARIO")) {
        const std::string name = *sn->GetValue("name");
        if (!ours.count(name)) continue;
        present.insert(name);
        ProtoScenarioModule proto(*sn);
        ScenarioModule* m = proto.Load(runner);
        REQUIRE_MESSAGE(m != nullptr, name);
        ProtoScenarioModule snap(*m);
        INFO(name);
        CHECK(snap.GetData().ToString() == sn->ToString());
        CHECK(snap.GetData().SaveToString() == sn->SaveToString());
    }
    const std::set<std::string> expected = {"ProgressTracking", "ScenarioAchievements", "ScenarioDestructibles", "VesselRecovery"};
    CHECK(present == expected);
}

// The SCENARIO formats the sandbox save lacks (Funding, Reputation, StrategySystem, ScenarioUpgradeableFacilities) through the same path:
// synthetic nodes written from the C# OnSave formats (not original data), loaded and saved again byte-exact
TEST_CASE("WP-36 Funding / Reputation / StrategySystem / ScenarioUpgradeableFacilities nodes round-trip (synthetic, format test)") {
    World36 w;
    RegisterCareerScenarioModules();
    ScenarioRunner runner;
    auto roundTrip = [&](ConfigNode& node) {
        ProtoScenarioModule proto(node);
        ScenarioModule* m = proto.Load(runner);
        REQUIRE(m != nullptr);
        ProtoScenarioModule snap(*m);
        CHECK(snap.GetData().ToString() == node.ToString());
    };
    ConfigNode funds("SCENARIO");
    funds.AddValue("name", "Funding");
    funds.AddValue("scene", "7, 8, 5, 6");
    funds.AddValue("funds", 61234.5678901234);
    roundTrip(funds);
    ConfigNode rep("SCENARIO");
    rep.AddValue("name", "Reputation");
    rep.AddValue("scene", "7, 8, 5, 6");
    rep.AddValue("rep", 123.456f);
    roundTrip(rep);
    ConfigNode strat("SCENARIO");
    strat.AddValue("name", "StrategySystem");
    strat.AddValue("scene", "7, 8, 5, 6");
    ConfigNode* sts = strat.AddNode("STRATEGIES");
    ConfigNode* s1 = sts->AddNode("STRATEGY");
    s1->AddValue("name", "AgressiveNegotiations");
    s1->AddValue("date", 1000.5);
    s1->AddValue("factor", 0.25f);
    for (const char* e : {"CurrencyOperation", "CurrencyOperation", "CurrencyOperation"}) s1->AddNode("EFFECT")->AddValue("name", e);
    // OnLoad of the StrategySystem runs a frame later (the coroutine): drive the scheduler once
    {
        ProtoScenarioModule proto(strat);
        ScenarioModule* m = proto.Load(runner);
        REQUIRE(m != nullptr);
        CareerEnvironment::Current().Scheduler().AdvanceFrame(0.02f);
        CareerEnvironment::Current().Scheduler().RunDelayedDynamicFrameRate();
        ProtoScenarioModule snap(*m);
        CHECK(snap.GetData().ToString() == strat.ToString());
    }
    ConfigNode fac("SCENARIO");
    fac.AddValue("name", "ScenarioUpgradeableFacilities");
    fac.AddValue("scene", "5, 6, 7, 8");
    fac.AddNode("SpaceCenter/Administration")->AddValue("lvl", 0.5f);
    fac.AddNode("SpaceCenter/LaunchPad")->AddValue("lvl", 1.0f);
    fac.AddNode("SpaceCenter/Runway")->AddValue("lvl", 0.0f);
    roundTrip(fac);
}

// ---- recovery of a roster member, quick path with no currencies -----------------------------------------------------------------------------

// VesselRecovery with no Funding / Reputation / R&D and an uncrewed vessel completes at once
TEST_CASE("WP-36 VesselRecovery without career modules completes the processing immediately") {
    World36 w;
    RecoveryWorld rw(w);
    VesselRecovery rec;
    rec.Awake();
    std::vector<KSP::UI::Screens::MissionRecoveryDialog*> dialogs;
    struct L {
        std::vector<KSP::UI::Screens::MissionRecoveryDialog*>* d;
        void Done(ProtoVessel*, KSP::UI::Screens::MissionRecoveryDialog* dlg, float) { d->push_back(dlg); }
    } l{&dialogs};
    GE::onVesselRecoveryProcessingComplete.Add(&l, &L::Done);
    ConfigNode vn("VESSEL");
    ProtoVessel pv(vn, &w.game);
    rec.OnVesselRecovered(&pv, false);
    GE::onVesselRecoveryProcessingComplete.Remove(&l, &L::Done);
    REQUIRE(dialogs.size() == 1);
    CHECK(dialogs[0] != nullptr);  // vesselRef == null: the shortcut needs a live vessel with no crew, so the full path ran
}
