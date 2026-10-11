// WP-38 slice 2 unit tests: the 24 FinePrint parameters, the FinePrint contracts besides the ExplorationContract (ARM, Base, Station, ISRU, Satellite,
// CometSample, Tourism, Survey), the remaining FinePrint.Utilities, ScenarioContractEvents and FinePrint::InstallAll
// (core/include/ksp/fineprint/{utilities,parameters,contracts,scenario_contract_events}.hpp).
//
// No golden or capture of FinePrint exists, so the expectations come from tools/wp38b/fineprint2_reference.py (an independent Python model: it reads the
// mirror cfgs, the golden body table and the decompiled C#; tests/data/wp38b_reference.json) or from the decompiled C# (the derivation is in the
// comment above the check).
//
// Fixtures that are not KSP data (STATUS.md "needs data"): everything of test_fineprint.cpp plus: R&D (the part database is not loaded: the
// FinePrintEnvironment overrides answer ResearchedValidContractObjectives / HaveModuleTech / ProbeCoresUnlocked from the reference fixtures), an empty
// crew roster and no vessels (the contextual branches are off), ContractsEnvironment::newRandomSeed = 4242, and a synthetic SiteGeneratorInfo (the text
// asset is not in the repo; its titles are compared only against the same synthetic tables in the reference).
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/career/career.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/contracts.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/fineprint/contracts.hpp"
#include "ksp/fineprint/parameters.hpp"
#include "ksp/fineprint/scenario_contract_events.hpp"
#include "ksp/fineprint/utilities.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/unity/mathf.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::Contracts;
using Localizer38b = KSP::Localization::Localizer;
using json = nlohmann::json;
namespace L38 = ksp::Contracts::Lite;
namespace FP = ksp::FinePrint;
namespace P38 = ksp::Contracts::Parameters;
namespace C38 = ksp::FinePrint::Contracts;

namespace {

const std::string kHarness38b = KSPCORE_HARNESS_DIR;

const json& Ref38b() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp38b_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

uint32_t Bits38b(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
uint64_t BitsD38b(double d) {
    uint64_t u;
    std::memcpy(&u, &d, 8);
    return u;
}
float F38b(const json& j) { return j.get<float>(); }
#define CHECK_F32b(got, ref) CHECK_MESSAGE(Bits38b(static_cast<float>(got)) == Bits38b(static_cast<float>(ref)), "got ", (got), " want ", (ref))
#define CHECK_F64b(got, ref) CHECK_MESSAGE(BitsD38b(static_cast<double>(got)) == BitsD38b(static_cast<double>(ref)), "got ", (got), " want ", (ref))

std::string GameData38b() { return ksp::testing::GameRoot() + "/GameData"; }

void LoadLocalizer38b() {
    Localizer38b::Shutdown();
    Localizer38b::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(GameData38b() + "/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    Localizer38b::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

Guid GuidOf38b(const json& bytes) {
    Guid g;
    for (size_t i = 0; i < 16; ++i) g.bytes[i] = static_cast<uint8_t>(bytes[i].get<int>());
    return g;
}

// the synthetic SiteGeneratorInfo (not KSP data): the same lists as tools/wp38b/fineprint2_reference.py SITE_FIX
ConfigNode::NodePtr SiteGeneratorFixture() {
    auto root = std::make_shared<ConfigNode>("SITEGEN");
    const json& site = Ref38b()["siteFixture"];
    for (auto it = site.begin(); it != site.end(); ++it) {
        ConfigNode* n = root->AddNode(it.key());
        for (const json& e : it.value()) n->AddValue("Element", e.get<std::string>());
    }
    return root;
}

// The stock Kerbol system + a CAREER game + GameVariables + the story / agent / contract definitions + the FinePrint types (as World38 of test_fineprint.cpp)
struct World38b {
    Game15 g;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    Game game;
    std::unique_ptr<GameVariables> gv;
    std::unique_ptr<Agents::AgentList> agents;
    std::unique_ptr<ProgressTracking> pt;
    std::unique_ptr<PartResourceLibrary> lib;
    ConfigNode::NodePtr siteNode;
    Guid nextGuid;
    World38b() {
        FlightRegistry::Clear();
        CareerEnvironment::Reset();
        ContractsEnvironment::Reset();
        L38::FinePrintEnvironment::Reset();
        ContractSystem::ClearTypes();
        ContractSystem::ContractWeights.reset();
        ScenarioUpgradeableFacilities::ClearStatics();
        SaveEnvironment& env = SaveEnvironment::Current();
        pg.physicsDatabaseFilename = kHarness38b + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        game.Mode = Game::Modes::CAREER;
        game.flightState = std::make_shared<FlightState>();
        game.CrewRoster = std::make_shared<KerbalRoster>(Game::Modes::CAREER);
        env.currentGame = &game;
        env.loadedScene = GameScenes::SPACECENTER;
        LoadLocalizer38b();
        for (CelestialBody* b : sys->SpawnOrder()) b->bodyDisplayName = Localizer38b::Format(b->bodyDisplayName);
        gv = std::make_unique<GameVariables>();
        gv->reputationAddition = AnimationCurve(std::vector<Keyframe>{Keyframe(0.0f, 0.9f)});
        gv->reputationSubtraction = AnimationCurve(std::vector<Keyframe>{Keyframe(0.0f, 1.3f)});
        ContractDefsLite::Load(GameData38b() + "/Squad/Contracts/Contracts.cfg");
        ConfigNode::NodePtr story = ConfigNode::Load(GameData38b() + "/Squad/Resources/StoryDefs.cfg");
        REQUIRE(story);
        ConfigNode::NodePtr sd = story->GetNode("STORY_DEF")->CreateCopy();
        Localizer38b::TranslateBranch(*sd);
        TextGen::Reset();
        TextGen::Setup(*sd);
        agents = std::make_unique<Agents::AgentList>();
        ConfigNode::NodePtr ag = ConfigNode::Load(GameData38b() + "/Squad/Agencies/Agents.cfg");
        REQUIRE(ag);
        agents->LoadAgents(ag->GetNodes("AGENT"));
        ContractSystem::RegisterStockTypes();
        FP::InstallAll();
        pt = std::make_unique<ProgressTracking>();
        pt->Awake();
        pt->Start();
        Planetarium::fetch->time = 0.0;
        ContractsEnvironment::Current().newGuid = [this] { return nextGuid; };
        ContractsEnvironment::Current().nowTicks = [] { return int64_t(636000000000000000LL); };
        ContractsEnvironment::Current().newRandomSeed = [] { return 4242; };
        for (CelestialBody* b : sys->SpawnOrder()) b->minOrbitalDistance = L38::CelestialUtilities::GetMinimumOrbitalDistance(b, 1.0f);
        // the part resource definitions of the mirror (ResourcesGeneric.cfg, Ore.cfg)
        lib = std::make_unique<PartResourceLibrary>();
        for (const char* fn : {"/Squad/Resources/ResourcesGeneric.cfg", "/Squad/Resources/Ore.cfg"}) {
            ConfigNode::NodePtr rn = ConfigNode::Load(GameData38b() + fn);
            REQUIRE(rn);
            for (ConfigNode* n : rn->GetNodes("RESOURCE_DEFINITION")) lib->resourceDefinitions.Add(*n);
        }
        siteNode = SiteGeneratorFixture();
        L38::FinePrintEnvironment::siteGeneratorInfo = siteNode.get();
        L38::StringUtilities::LoadSiteGenerationInfo();
    }
    ~World38b() {
        agents.reset();
        pt->OnDestroy();
        pt.reset();
        gv.reset();
        lib.reset();
        ContractSystem::ClearTypes();
        ContractSystem::ContractWeights.reset();
        ContractSystem::Instance = nullptr;
        ContractsEnvironment::Reset();
        L38::FinePrintEnvironment::Reset();
        L38::StringUtilities::LoadSiteGenerationInfo();
        ContractDefsLite::Clear();
        TextGen::Reset();
        CareerEnvironment::Reset();
        ScenarioUpgradeableFacilities::ClearStatics();
        SaveEnvironment::Current().currentGame = nullptr;
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
    }
    CelestialBody& Body(const std::string& n) { return *sys->GetBodyByName(n); }
    GameParameters::CareerParams& Career() { return game.Parameters->Career; }
    void SetMult(const json& m) {
        Career().FundsGainMultiplier = F38b(m["fundsGain"]);
        Career().FundsLossMultiplier = F38b(m["fundsLoss"]);
        Career().RepGainMultiplier = F38b(m["repGain"]);
        Career().RepLossMultiplier = F38b(m["repLoss"]);
        Career().ScienceGainMultiplier = F38b(m["sciGain"]);
    }
    // the R&D fixture of the reference: objectives researched, module techs researched, probe cores unlocked
    void SetFixture(const json& f) {
        std::vector<std::string> obj = f["objectives"].get<std::vector<std::string>>();
        std::vector<std::string> mods = f["modules"].get<std::vector<std::string>>();
        const bool probe = f["probe"].get<bool>();
        L38::FinePrintEnvironment::researchedValidContractObjectives = [obj](const std::vector<std::string>& types) {
            for (const std::string& t : types)
                if (std::find(obj.begin(), obj.end(), t) == obj.end()) return false;
            return true;
        };
        L38::FinePrintEnvironment::haveModuleTech = [mods](const std::string& m) { return std::find(mods.begin(), mods.end(), m) != mods.end(); };
        L38::FinePrintEnvironment::probeCoresUnlocked = [probe] { return probe; };
    }
};

void LoadScenario38b(World38b& w, const json& scenario) {
    ConfigNode scen("SCENARIO");
    ConfigNode* prog = scen.AddNode("Progress");
    for (auto it = scenario["top"].begin(); it != scenario["top"].end(); ++it) prog->AddNode(it.key())->AddValue(it.value().get<std::string>(), 0.0);
    for (auto it = scenario["bodies"].begin(); it != scenario["bodies"].end(); ++it) {
        ConfigNode* b = prog->AddNode(it.key());
        for (auto jt = it.value().begin(); jt != it.value().end(); ++jt) b->AddNode(jt.key())->AddValue(jt.value().get<std::string>(), 0.0);
    }
    w.pt->OnLoad(scen);
}

ConfigNode WeightsNode38b(const json& weights) {
    ConfigNode n("SCENARIO");
    ConfigNode* wn = n.AddNode("WEIGHTS");
    for (auto it = weights.begin(); it != weights.end(); ++it) wn->AddValue(it.key(), static_cast<int32_t>(it.value().get<int>()));
    return n;
}

const char* OrbitName(FP::OrbitType t) {
    static const char* names[] = {"SYNCHRONOUS", "STATIONARY", "POLAR", "EQUATORIAL", "KOLNIYA", "TUNDRA", "RANDOM"};
    return names[static_cast<int>(t)];
}

ContractPrestige PrestigeOf38b(int p) { return static_cast<ContractPrestige>(p); }

}  // namespace

// ---- the generated contracts ------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-38b ARM / Base / Station / ISRU / Satellite contracts: generated trees (targets, parameters, texts, rewards, agent, id) vs the reference (0 ULP, byte-exact)") {
    const json& ref = Ref38b();
    std::map<std::string, std::pair<int, int>> counts;
    int paramsChecked = 0;
    int64_t maxUlpOrbit = 0;
    for (size_t si = 0; si < ref["scenarios"].size(); ++si) {
        World38b w;
        ContractSystem cs;
        cs.Awake();
        ConfigNode weights = WeightsNode38b(ref["weights"]);
        cs.LoadNow(weights);
        LoadScenario38b(w, ref["scenarios"][si]);
        int curFix = -1, curMult = -1;
        for (const json& e : ref["contracts"]) {
            if (e["scenario"].get<size_t>() != si) continue;
            if (e["fixture"].get<int>() != curFix) {
                curFix = e["fixture"].get<int>();
                w.SetFixture(ref["fixtures"][static_cast<size_t>(curFix)]);
            }
            if (e["mult"].get<int>() != curMult) {
                curMult = e["mult"].get<int>();
                w.SetMult(ref["mult_sets"][static_cast<size_t>(curMult)]);
            }
            w.nextGuid = GuidOf38b(e["guid"]);
            const std::string type = e["type"].get<std::string>();
            INFO(type << " scenario " << ref["scenarios"][si]["name"] << " fixture " << e["fixture"] << " prestige " << e["prestige"] << " seed " << e["seed"] << " mult " << e["mult"]);
            std::unique_ptr<Contract> c = cs.GenerateContract(e["seed"].get<int>(), PrestigeOf38b(e["prestige"].get<int>()), type);
            const json& r = e["result"];
            if (r.is_null()) {
                INFO("unexpected contract: " << (c ? c->Title() : std::string()));
                CHECK(c == nullptr);
                ++counts[type].second;
                continue;
            }
            REQUIRE(c != nullptr);
            ++counts[type].first;
            CHECK(c->Agent()->Name() == r["agent"].get<std::string>());
            CHECK(c->Agent()->Title() == r["agentTitle"].get<std::string>());
            CHECK(c->Keywords() == r["keywords"].get<std::vector<std::string>>());
            CHECK(c->Title() == r["title"].get<std::string>());
            CHECK(c->Synopsys() == r["synopsis"].get<std::string>());
            CHECK(c->Description() == r["description"].get<std::string>());
            CHECK(c->ContractID() == r["contractID"].get<long long>());
            CHECK(dotnet::EnumToString(c->Prestige()) == r["prestige"].get<std::string>());
            CHECK_F64b(c->FundsAdvance, r["fa"].get<double>());
            CHECK_F64b(c->FundsCompletion, r["fc"].get<double>());
            CHECK_F64b(c->FundsFailure, r["ff"].get<double>());
            CHECK_F32b(c->ReputationCompletion, F38b(r["rc"]));
            CHECK_F32b(c->ReputationFailure, F38b(r["rf"]));
            CHECK_F32b(c->ScienceCompletion, F38b(r["sc"]));
            CHECK_F64b(c->TimeExpiry, r["expiry"].get<double>());
            CHECK_F64b(c->TimeDeadline, r["deadline"].get<double>());
            CHECK(c->CanBeCancelled());
            CHECK(c->CanBeDeclined());
            CHECK(c->MissionSeed() == e["seed"].get<int>());
            REQUIRE(c->ParameterCount() == static_cast<int>(r["params"].size()));
            for (int i = 0; i < c->ParameterCount(); ++i) {
                ContractParameter* p = c->GetParameter(i);
                const json& pr = r["params"][static_cast<size_t>(i)];
                CHECK(p->TypeName() == pr["type"].get<std::string>());
                CHECK(p->HashString() == pr["hash"].get<std::string>());
                if (pr.contains("title") && pr["type"].get<std::string>() != "StationaryPointParameter") CHECK(p->Title() == pr["title"].get<std::string>());
                if (pr["type"] == "SpecificOrbitParameter") {
                    auto* so = dynamic_cast<P38::SpecificOrbitParameter*>(p);
                    REQUIRE(so != nullptr);
                    const json& el = pr["orbit"];
                    CHECK(std::string(OrbitName(so->orbitType)) == pr["orbitType"].get<std::string>());
                    // the draw level elements; the final orbit went through Orbit.UpdateFromStateVectors (see tests/test_fineprint.cpp, GenerateOrbit)
                    CHECK(std::fabs(so->sma - el["sma"].get<double>()) <= 1e-12 * std::fabs(el["sma"].get<double>()));
                    CHECK(std::fabs(std::fabs(so->eccentricity) - std::fabs(el["ecc"].get<double>())) <= 1e-9);
                    CHECK(std::fabs(so->inclination - el["inc"].get<double>()) <= 1e-9);
                    CHECK_F64b(so->deviationWindow, pr["deviation"].get<double>());
                }
                if (pr["type"] == "StationaryPointParameter") {
                    auto* sp = dynamic_cast<P38::StationaryPointParameter*>(p);
                    REQUIRE(sp != nullptr);
                    CHECK_F64b(sp->longitude, pr["longitude"].get<double>());
                    CHECK_F64b(sp->deviation, pr["deviation"].get<double>());
                }
                ++paramsChecked;
            }
            if (type == "ARMContract") {
                auto* a = dynamic_cast<C38::ARMContract*>(c.get());
                REQUIRE(a != nullptr);
                CHECK(a->targetBody->name() == r["targetBody"].get<std::string>());
                CHECK(a->asteroidClass == r["asteroidClass"].get<std::string>());
                CHECK(a->isLanding == r["isLanding"].get<bool>());
                CHECK(a->MessageCompleted().empty() == false);
            } else if (type == "BaseContract") {
                auto* b = dynamic_cast<C38::BaseContract*>(c.get());
                REQUIRE(b != nullptr);
                CHECK(b->targetBody->name() == r["targetBody"].get<std::string>());
                CHECK(b->capacity == r["capacity"].get<int>());
            } else if (type == "StationContract") {
                auto* b = dynamic_cast<C38::StationContract*>(c.get());
                REQUIRE(b != nullptr);
                CHECK(b->targetBody->name() == r["targetBody"].get<std::string>());
                CHECK(b->capacity == r["capacity"].get<int>());
            } else if (type == "ISRUContract") {
                auto* b = dynamic_cast<C38::ISRUContract*>(c.get());
                REQUIRE(b != nullptr);
                CHECK(b->targetBody->name() == r["targetBody"].get<std::string>());
                CHECK(b->isDelivering == r["isDelivering"].get<bool>());
                CHECK_F32b(b->gatherGoal, F38b(r["gatherGoal"]));
                CHECK(b->targetResource == r["targetResource"].get<std::string>());
                CHECK(b->resourceTitle == r["resourceTitle"].get<std::string>());
                if (b->isDelivering) {
                    CHECK(b->deliveryBody->name() == r["deliveryBody"].get<std::string>());
                    CHECK(dotnet::EnumToString(b->deliverySituation) == r["deliverySituation"].get<std::string>());
                }
            } else if (type == "SatelliteContract") {
                auto* b = dynamic_cast<C38::SatelliteContract*>(c.get());
                REQUIRE(b != nullptr);
                CHECK(b->targetBody->name() == r["targetBody"].get<std::string>());
                CHECK(std::string(OrbitName(b->orbitType)) == r["orbitType"].get<std::string>());
                CHECK_F64b(b->deviation, r["deviation"].get<double>());
                CHECK(b->MessageCompleted() == r["completed"].get<std::string>());
            }
        }
    }
    MESSAGE("generated / refused: ARM " << counts["ARMContract"].first << "/" << counts["ARMContract"].second << ", Base " << counts["BaseContract"].first << "/"
                                         << counts["BaseContract"].second << ", Station " << counts["StationContract"].first << "/" << counts["StationContract"].second
                                         << ", ISRU " << counts["ISRUContract"].first << "/" << counts["ISRUContract"].second << ", Satellite "
                                         << counts["SatelliteContract"].first << "/" << counts["SatelliteContract"].second << "; parameters checked " << paramsChecked
                                         << ", max orbit ulp " << maxUlpOrbit);
    for (const char* t : {"ARMContract", "BaseContract", "StationContract", "ISRUContract", "SatelliteContract"}) {
        CHECK(counts[t].first > 0);
        CHECK(counts[t].second > 0);
    }
}
