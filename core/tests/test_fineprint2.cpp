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
#include <cstdio>
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
#include "ksp/kerbals/experience.hpp"
#include "ksp/science/science.hpp"
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

// ---- parameter texts -------------------------------------------------------------------------------------------------------------------------------
namespace {

std::string ReplaceAgent(std::string s, const std::string& title) {
    const std::string key = "{AGENT}";
    for (size_t p = s.find(key); p != std::string::npos; p = s.find(key, p + title.size())) s.replace(p, key.size(), title);
    return s;
}

Vessel::Situations SituationOf(const std::string& s) {
    if (s == "XXX") return static_cast<Vessel::Situations>(0);
    return dotnet::EnumParseFull<Vessel::Situations>(s);
}

FP::OrbitType OrbitTypeOf38b(const std::string& s) {
    for (int i = 0; i < 7; ++i)
        if (s == OrbitName(static_cast<FP::OrbitType>(i))) return static_cast<FP::OrbitType>(i);
    FAIL("orbit type " << s);
    return FP::OrbitType::RANDOM;
}

std::unique_ptr<ContractParameter> MakeParam(World38b& w, const json& e) {
    const std::string cls = e["cls"].get<std::string>();
    auto body = [&](const char* key) -> CelestialBody* { return e[key].is_null() ? nullptr : &w.Body(e[key].get<std::string>()); };
    if (cls == "Stability") return std::make_unique<P38::StabilityParameter>(F38b(e["hold"]));
    if (cls == "CrewCapacity") return std::make_unique<P38::CrewCapacityParameter>(e["n"].get<int>());
    if (cls == "CrewTrait") return std::make_unique<P38::CrewTraitParameter>(e["trait"].get<std::string>(), e["count"].get<int>(), e["desc"].get<std::string>());
    if (cls == "MobileBase") return std::make_unique<P38::MobileBaseParameter>();
    if (cls == "LocationAndSituation")
        return std::make_unique<P38::LocationAndSituationParameter>(body("body"), SituationOf(e["sit"].get<std::string>()), e["noun"].get<std::string>(), e["final"].get<bool>());
    if (cls == "VesselDestination")
        return std::make_unique<P38::VesselDestinationParameter>(body("body"), dotnet::EnumParseFull<FlightLog_EntryType>(e["entry"].get<std::string>()));
    if (cls == "KerbalDestination")
        return std::make_unique<P38::KerbalDestinationParameter>(body("body"), dotnet::EnumParseFull<FlightLog_EntryType>(e["entry"].get<std::string>()),
                                                                 e["kerbal"].get<std::string>());
    if (cls == "KerbalGeeAdventure") return std::make_unique<P38::KerbalGeeAdventureParameter>(e["kerbal"].get<std::string>());
    if (cls == "KerbalTour")
        return std::make_unique<P38::KerbalTourParameter>(e["kerbal"].get<std::string>(), dotnet::EnumParseFull<ProtoCrewMember_Gender>(e["gender"].get<std::string>()));
    if (cls == "VesselSystems") {
        const std::string m = e["manned"].get<std::string>();
        return std::make_unique<P38::VesselSystemsParameter>(std::vector<std::string>{}, e["descs"].get<std::vector<std::string>>(), e["vdesc"].get<std::string>(),
                                                             dotnet::EnumParseFull<L38::MannedStatus>(m), e["requireNew"].get<bool>());
    }
    if (cls == "PartRequest") {
        ConfigNode n("PART_REQUEST");
        n.AddValue("Article", e["article"].get<std::string>());
        n.AddValue("PartDescription", e["partDescription"].get<std::string>());
        n.AddValue("VesselDescription", e["vesselDescription"].get<std::string>());
        for (const json& p : e["parts"]) n.AddValue("Part", p.get<std::string>());
        for (const json& p : e["modules"]) n.AddValue("Module", p.get<std::string>());
        return std::make_unique<P38::PartRequestParameter>(n);
    }
    if (cls == "ResourcePossession")
        return std::make_unique<P38::ResourcePossessionParameter>(e["name"].get<std::string>(), e["rtitle"].get<std::string>(), e["vessel"].get<std::string>(), e["goal"].get<double>());
    if (cls == "ResourceExtraction")
        return std::make_unique<P38::ResourceExtractionParameter>(e["name"].get<std::string>(), e["rtitle"].get<std::string>(), e["goal"].get<double>(), body("body"),
                                                                  std::vector<std::string>{"ModuleResourceHarvester"});
    if (cls == "Asteroid") return std::make_unique<P38::AsteroidParameter>(e["size"].get<std::string>(), e["forStation"].get<bool>());
    if (cls == "Comet") return std::make_unique<P38::CometParameter>(e["size"].get<std::string>(), e["forStation"].get<bool>());
    if (cls == "CometDetection") return std::make_unique<P38::CometDetectionParameter>(e["count"].get<int>(), "sentinel");
    if (cls == "CometScience") return std::make_unique<P38::CometScienceParameter>(e["id"].get<uint32_t>(), e["name"].get<std::string>(), "subject");
    if (cls == "Construction")
        return std::make_unique<P38::ConstructionParameter>(e["part"].get<std::string>(), e["vessel"].get<uint32_t>(), e["vesselName"].get<std::string>(), body("body"));
    if (cls == "RepairPart")
        return std::make_unique<P38::RepairPartParameter>(e["partId"].get<uint32_t>(), e["part"].get<std::string>(), e["vessel"].get<uint32_t>(), e["vesselName"].get<std::string>(),
                                                          body("body"));
    if (cls == "SpecificOrbit")
        return std::make_unique<P38::SpecificOrbitParameter>(OrbitTypeOf38b(e["orbitType"].get<std::string>()), 10.0, 0.1, 800000.0, 10.0, 20.0, 0.0, 0.0, body("body"), 10.0);
    if (cls == "SpecificOrbitNotes")
        return std::make_unique<P38::SpecificOrbitParameter>(FP::OrbitType::RANDOM, e["inc"].get<double>(), e["ecc"].get<double>(), e["sma"].get<double>(), e["lan"].get<double>(),
                                                             e["arg"].get<double>(), 0.0, 0.0, body("body"), 10.0);
    if (cls == "RoverWayPoint") return std::make_unique<P38::RoverWayPointParameter>(1.0, 2.0, 3.0, 4.0, &w.Body("Kerbin"));
    FAIL("unknown parameter class " << cls);
    return nullptr;
}

}  // namespace

TEST_CASE("WP-38b parameters: titles, notes, messages and hashes (tag tables of the dictionary, the managed Lingoona fallback) vs the reference") {
    World38b w;
    int rows = 0;
    std::map<std::string, int> perClass;
    for (const json& e : Ref38b()["paramTexts"]) {
        auto root = std::make_unique<C38::ARMContract>();
        root->agent = w.agents->GetAgent("Kerbodyne");
        REQUIRE(root->agent != nullptr);
        root->prestige = e.contains("prestige") ? PrestigeOf38b(e["prestige"].get<int>()) : ContractPrestige::Trivial;
        ContractParameter* p = root->AddParameter(MakeParam(w, e));
        INFO(e.dump());
        const std::string cls = e["cls"].get<std::string>();
        if (cls == "SpecificOrbitNotes") {
            CHECK(p->Notes() == e["notes"].get<std::string>());
        } else {
            CHECK(p->Title() == e["title"].get<std::string>());
            if (e.contains("notes") && !e["notes"].is_null()) CHECK(p->Notes() == ReplaceAgent(e["notes"].get<std::string>(), Localizer38b::Format(root->agent->Title())));
            if (e.contains("notes") && e["notes"].is_null()) CHECK(p->Notes().empty());
            if (e.contains("complete")) CHECK(p->MessageComplete() == e["complete"].get<std::string>());
            if (e.contains("hash")) CHECK(p->HashString() == e["hash"].get<std::string>());
        }
        ++perClass[cls];
        ++rows;
    }
    CHECK(rows > 200);
    MESSAGE("parameter text rows: " << rows << " over " << perClass.size() << " classes");
    CHECK(perClass.size() >= 22);
}

// ---- StringUtilities ------------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-38b StringUtilities: TitleCase, ThisThisAndThat, pronouns, ValueAndUnits (decimal #,###.#), CardinalDirectionBetween, IsVowel, CalculateReadDuration, Pack / Unpack") {
    World38b w;
    const json& su = Ref38b()["stringUtils"];
    for (const json& e : su["titleCase"]) CHECK(L38::StringUtilities::TitleCase(e["in"].get<std::string>()) == e["out"].get<std::string>());
    for (const json& e : su["thisThisAndThat"]) CHECK(L38::StringUtilities::ThisThisAndThat(e["in"].get<std::vector<std::string>>()) == e["out"].get<std::string>());
    for (const json& e : su["pronouns"]) {
        const auto casing = dotnet::EnumParseFull<L38::PronounCasing>(e["casing"].get<std::string>());
        const auto gender = dotnet::EnumParseFull<L38::GrammaticalGender>(e["gender"].get<std::string>());
        CHECK(L38::StringUtilities::AppropriatePronoun(casing, gender, e["post"].get<bool>()) == e["out"].get<std::string>());
    }
    for (const json& e : su["valueAndUnits"]) {
        INFO(e.dump());
        const std::string t = e["type"].get<std::string>() == "NULL_" ? std::string("NULL") : e["type"].get<std::string>();
        CHECK(L38::StringUtilities::ValueAndUnits(dotnet::EnumParseFull<L38::RecordTrackType>(t), e["value"].get<double>()) == e["out"].get<std::string>());
    }
    for (const json& e : su["cardinal"]) {
        INFO(e.dump());
        const auto& a = e["args"];
        CHECK(L38::StringUtilities::CardinalDirectionBetween(a[0].get<double>(), a[1].get<double>(), a[2].get<double>(), a[3].get<double>()) == e["out"].get<std::string>());
    }
    for (const json& e : su["isVowel"]) CHECK(L38::StringUtilities::IsVowel(e["c"].get<std::string>()[0]) == e["out"].get<bool>());
    for (const json& e : su["readDuration"]) CHECK_F32b(L38::StringUtilities::CalculateReadDuration(e["in"].get<std::string>()), F38b(e["out"]));
    // PackDelimitedString / UnpackDelimitedString (int, uint, string lists; invalid numbers are dropped)
    CHECK(L38::StringUtilities::PackDelimitedString(std::vector<int32_t>{3, -4, 15}) == "3|-4|15");
    CHECK(L38::StringUtilities::PackDelimitedString(std::vector<std::string>{"a", "b c"}) == "a|b c");
    CHECK(L38::StringUtilities::PackDelimitedString(std::vector<std::string>{}) == "");
    CHECK(L38::StringUtilities::UnpackDelimitedInt32("3|x|-4||15") == std::vector<int32_t>{3, -4, 15});
    CHECK(L38::StringUtilities::UnpackDelimitedStrings("a|b c").size() == 2);
    CHECK(L38::StringUtilities::UnpackDelimitedStrings("").empty());
    CHECK(L38::StringUtilities::UnpackDelimitedUInt32("7|8") == std::vector<uint32_t>{7u, 8u});
    // AlphaNumericDesignation over the synthetic alphabet and the site name of the generic branch (allowNamed false)
    for (const json& e : Ref38b()["alphaNumeric"]) CHECK(L38::StringUtilities::AlphaNumericDesignation(e["seed"].get<int>()) == e["value"].get<std::string>());
    // IntegerToGreek clamps to 0..23
    CHECK(L38::StringUtilities::IntegerToGreek(-5) == Ref38b()["greek"][0].get<std::string>());
    CHECK(L38::StringUtilities::IntegerToGreek(99) == Ref38b()["greek"][23].get<std::string>());
    CHECK(L38::StringUtilities::IntegerToGreek(7) == Ref38b()["greek"][7].get<std::string>());
}

// ---- parameter behaviour: OnSave / OnLoad round trips, OnUpdate counters, event handlers ------------------------------------------------------------
// Derived from the decompiled C# (each check names the member it follows). A loaded vessel is built from the real stock part prefabs of the mirror.
namespace {

struct VRig {
    Vessel v;
    std::vector<std::unique_ptr<Part>> parts;
    VRig(World38b& w, CelestialBody& body, std::initializer_list<const char*> names, Vessel::Situations sit = Vessel::Situations::ORBITING) {
        v.loaded = true;
        v.vesselType = VesselType::Ship;
        v.situation = sit;
        v.vesselName = "Test Vessel";
        v.orbitDriver = std::make_unique<OrbitDriver>();
        v.orbitDriver->orbit->referenceBody = &body;
        for (const char* n : names) AddPart(w, n);
        if (!parts.empty()) v.rootPart = parts[0].get();
    }
    Part* AddPart(World38b& w, const char* name) {
        auto p = w.g.New(name);
        p->vessel = &v;
        v.parts.push_back(p.get());
        parts.push_back(std::move(p));
        return parts.back().get();
    }
};

// an active ARM contract owning `target` and a filler parameter that never completes (so the contract itself does not complete)
struct Root {
    std::unique_ptr<C38::ARMContract> c = std::make_unique<C38::ARMContract>();
    ContractParameter* p = nullptr;
    template <class P>
    P* Own(World38b& w, std::unique_ptr<P> param, bool active = true) {
        c->agent = w.agents->GetAgent("Kerbodyne");
        c->targetBody = &w.Body("Kerbin");  // the contract title reads it
        P* raw = param.get();
        p = c->AddParameter(std::move(param));
        c->AddParameter(std::make_unique<P38::MobileBaseParameter>());  // never completes without a grounded motorized wheel
        if (active) {
            c->state = Contract::State::Active;
            c->Register();
        }
        return raw;
    }
    ~Root() {
        if (c) c->Unregister();
    }
};

struct FlightScope {
    SceneScope scene{GameScenes::FLIGHT};
    explicit FlightScope(Vessel* active) {
        FlightRegistry::ActiveVessel = active;
        FlightRegistry::ready = true;
        L38::FinePrintEnvironment::deltaTime = [] { return 0.5f; };
    }
    ~FlightScope() {
        FlightRegistry::ActiveVessel = nullptr;
        FlightRegistry::ready = false;
    }
};

std::string NodeText(const ConfigNode& n) { return n.ToString(); }

// Funding / Reputation / ContractSystem singletons (as Life37 of test_contracts.cpp)
struct Career38b {
    Funding funds;
    Reputation rep;
    ContractSystem cs;
    explicit Career38b(World38b& w) {
        (void)w;
        funds.Awake();
        rep.Awake();
        rep.Start();
        funds.SetFunds(1000000.0, TransactionReasons::Cheating);
        rep.SetReputation(0.0f, TransactionReasons::Cheating);
        cs.Awake();
        ConfigNode weights = WeightsNode38b(Ref38b()["weights"]);
        cs.LoadNow(weights);
    }
};

// the crew generator's name tables are text assets that are not in the repo: kerbals are created by name
std::shared_ptr<ProtoCrewMember> MakeKerbal(World38b& w, const std::string& name, ProtoCrewMember_KerbalType type) {
    auto k = std::make_shared<ProtoCrewMember>(type, name);
    w.game.CrewRoster->AddCrewMember(k);
    return k;
}

// Save, load into a fresh instance of the registered type, save again: the text of the two nodes agrees
template <class P>
void RoundTrip(P& p, const char* typeName) {
    ConfigNode a("PARAM");
    p.OnSave(a);
    // OnLoad runs on a parameter that already belongs to a contract (Root().ContractState is read): a throwaway holder in the Generated state
    auto holder = std::make_unique<C38::ARMContract>();
    holder->targetBody = Planetarium::fetch->Home;
    ContractParameter* fresh = holder->AddParameter(ContractSystem::CreateParameter(typeName));
    REQUIRE(fresh != nullptr);
    fresh->OnLoad(a);
    ConfigNode b("PARAM");
    fresh->OnSave(b);
    CHECK(NodeText(a) == NodeText(b));
}

}  // namespace

TEST_CASE("WP-38b StabilityParameter: hold timer (10 s at 0.5 s per update), the three instability conditions, node format") {
    World38b w;
    Career38b career(w);
    VRig r(w, w.Body("Kerbin"), {"mk1pod.v2"}, Vessel::Situations::LANDED);
    r.v.Landed = true;
    FlightScope fs(&r.v);
    Root root;
    auto* p = root.Own(w, std::make_unique<P38::StabilityParameter>(10.0f));
    // StabilityParameter.cs:57 OnUpdate: landed, srf_velocity.sqrMagnitude <= 0.09, geeForce irrelevant when landed
    for (int i = 0; i < 19; ++i) p->Update();
    CHECK(p->State() == ParameterState::Incomplete);
    CHECK_F32b(p->holdTimer, 0.5f);
    p->Update();
    CHECK(p->State() == ParameterState::Complete);
    // moving surface speed 0.4 m/s (0.16 > 0.09): unstable, the complete parameter reverts
    r.v.srf_velocity = Vector3d(0.4, 0.0, 0.0);
    p->Update();
    CHECK(p->State() == ParameterState::Incomplete);
    // and the timer is reset to holdSeconds while unstable
    p->Update();
    CHECK_F32b(p->holdTimer, 10.0f);
    // flying with g > 0.1 is unstable too; sub-orbital below 500 m height is unstable
    r.v.srf_velocity = Vector3d(0.0, 0.0, 0.0);
    r.v.Landed = false;
    r.v.geeForce = 0.2;
    p->Update();
    CHECK_F32b(p->holdTimer, 10.0f);
    r.v.geeForce = 0.0;
    r.v.situation = Vessel::Situations::SUB_ORBITAL;
    r.v.heightFromTerrain = 300.0f;
    p->Update();
    CHECK_F32b(p->holdTimer, 10.0f);
    r.v.heightFromTerrain = -1.0f;
    p->Update();
    CHECK_F32b(p->holdTimer, 9.5f);
    // node: holdSeconds (a float); an unreadable value loads the default 10
    ConfigNode n("PARAM");
    p->OnSave(n);
    CHECK(*n.GetValue("holdSeconds") == "10");
    ConfigNode bad("PARAM");
    bad.AddValue("holdSeconds", "abc");
    P38::StabilityParameter q(3.0f);
    q.OnLoad(bad);
    CHECK_F32b(q.holdSeconds, 10.0f);
    RoundTrip(*p, "StabilityParameter");
    // the title rounds half up
    CHECK(P38::StabilityParameter(2.5f).GetTitle() == Localizer38b::Format("#autoLOC_285413", {"3"}));
}

TEST_CASE("WP-38b CrewCapacityParameter / CrewTraitParameter / MobileBaseParameter / LocationAndSituationParameter: five consecutive successes, dirty-vessel events") {
    World38b w;
    Career38b career(w);
    CelestialBody& mun = w.Body("Mun");
    VRig r(w, mun, {"mk1-3pod", "mk1pod.v2"}, Vessel::Situations::ORBITING);  // capacity 3 + 1
    FlightScope fs(&r.v);
    CHECK(r.v.GetCrewCapacity() == 4);
    {
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::CrewCapacityParameter>(4));
        for (int i = 0; i < 4; ++i) p->Update();
        CHECK(p->State() == ParameterState::Incomplete);
        CHECK(p->successCounter == 4);
        p->Update();
        CHECK(p->State() == ParameterState::Complete);
        // VesselWasModified of the active vessel marks it dirty; a smaller vessel makes the next update revert
        Part* last = r.parts.back().get();
        r.v.parts.pop_back();
        events::GameEvents::onVesselWasModified.Fire(&r.v);
        CHECK(p->dirtyVessel);
        p->Update();
        CHECK(p->State() == ParameterState::Incomplete);
        r.v.parts.push_back(last);
        // other vessels do not mark it dirty
        VRig other(w, mun, {"mk1pod.v2"});
        p->dirtyVessel = false;
        events::GameEvents::onVesselWasModified.Fire(&other.v);
        CHECK_FALSE(p->dirtyVessel);
        // onPartCouple / onPartDie of a part of the active vessel
        events::GameEvents::onPartDie.Fire(r.parts[0].get());
        CHECK(p->dirtyVessel);
        p->dirtyVessel = false;
        events::GameEvents::onPartCouple.Fire(events::GameEvents::FromToAction<Part*, Part*>(r.parts[0].get(), other.parts[0].get()));
        CHECK(p->dirtyVessel);
        ConfigNode n("PARAM");
        p->OnSave(n);
        CHECK(*n.GetValue("targetCapacity") == "4");
        RoundTrip(*p, "CrewCapacityParameter");
        // the notes follow the SpecificVessel sibling's vessel (none here): null
        CHECK(p->Notes().empty());
    }
    {
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::LocationAndSituationParameter>(&mun, Vessel::Situations::ORBITING, "probe"));
        for (int i = 0; i < 5; ++i) p->Update();
        CHECK(p->State() == ParameterState::Complete);
        r.v.situation = Vessel::Situations::LANDED;
        events::GameEvents::onVesselSituationChange.Fire(events::GameEvents::HostedFromToAction<Vessel*, Vessel::Situations>(&r.v, Vessel::Situations::ORBITING, Vessel::Situations::LANDED));
        p->Update();
        CHECK(p->State() == ParameterState::Incomplete);
        CHECK(p->successCounter == 5);  // the revert branch does not touch the counter
        p->Update();
        CHECK(p->successCounter == 0);  // an incomplete parameter that is not valid resets it
        r.v.situation = Vessel::Situations::ORBITING;
        r.v.vesselType = VesselType::EVA;  // an EVA kerbal does not count
        events::GameEvents::onVesselSOIChanged.Fire(events::GameEvents::HostedFromToAction<Vessel*, CelestialBody*>(&r.v, &mun, &mun));
        for (int i = 0; i < 5; ++i) p->Update();
        CHECK(p->State() == ParameterState::Incomplete);
        r.v.vesselType = VesselType::Ship;
        events::GameEvents::onVesselSOIChanged.Fire(events::GameEvents::HostedFromToAction<Vessel*, CelestialBody*>(&r.v, &mun, &mun));
        for (int i = 0; i < 5; ++i) p->Update();
        CHECK(p->State() == ParameterState::Complete);
        ConfigNode n("PARAM");
        p->OnSave(n);
        CHECK(*n.GetValue("targetBody") == std::to_string(mun.flightGlobalsIndex));
        CHECK(*n.GetValue("targetSituation") == "ORBITING");
        CHECK(*n.GetValue("noun") == "probe");
        CHECK(*n.GetValue("finalObjective") == "False");
        RoundTrip(*p, "LocationAndSituationParameter");
    }
    {
        // CrewTraitParameter counts crew of the trait; a vessel without crew has none
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::CrewTraitParameter>("Pilot", 1, "vessel"));
        for (int i = 0; i < 6; ++i) p->Update();
        CHECK(p->State() == ParameterState::Incomplete);
        ConfigNode n("PARAM");
        p->OnSave(n);
        CHECK(*n.GetValue("targetTrait") == "Pilot");
        CHECK(*n.GetValue("targetCount") == "1");
        CHECK(*n.GetValue("vesselDescription") == "vessel");
        RoundTrip(*p, "CrewTraitParameter");
        auto crew = MakeKerbal(w, "Jeb Kerman", ProtoCrewMember_KerbalType::Crew);
        crew->trait = "Pilot";
        KerbalRoster::SetExperienceTrait(*crew, std::string("Pilot"));
        r.parts[0]->protoModuleCrew.push_back(crew);
        r.v.CrewListSetDirty();
        events::GameEvents::onVesselCrewWasModified.Fire(&r.v);
        for (int i = 0; i < 5; ++i) p->Update();
        // ExperienceTrait.TypeName is the config title (a localised word in the game; the untranslated mirror leaves the tag): compare with it
        const std::string typeName = crew->experienceTrait->TypeName();
        CHECK(r.v.GetVesselCrew().size() == 1);
        CHECK(L38::VesselCrewWithTraitCount(typeName, &r.v) == 1);
        CHECK(L38::VesselCrewWithTrait(typeName, &r.v).size() == 1);
        CHECK(L38::VesselCrewWithTraitCount("Scientist", &r.v) == 0);
        p->targetTrait = typeName;
        p->dirtyVessel = true;
        for (int i = 0; i < 5; ++i) p->Update();
        CHECK(p->State() == ParameterState::Complete);
        r.parts[0]->protoModuleCrew.clear();
    }
}

TEST_CASE("WP-38b MobileBaseParameter: needs a grounded motorized wheel while landed or pre-launch") {
    World38b w;
    Career38b career(w);
    VRig r(w, w.Body("Mun"), {"roverBody.v2", "roverWheel1"}, Vessel::Situations::LANDED);
    r.v.Landed = true;
    FlightScope fs(&r.v);
    PartModule* wheel = nullptr;
    for (Part* part : r.v.parts)
        for (size_t i = 0; i < part->Modules().Count(); ++i)
            if (part->Modules()[i]->IsA("ModuleWheelBase")) wheel = part->Modules()[i];
    REQUIRE(wheel != nullptr);
    CHECK(L38::IsValidContractObjective(*wheel, "Wheel") == (wheel->EnumName("wheelType") == "MOTORIZED"));
    CHECK_FALSE(L38::VesselHasWheelsOnGround(&r.v, {L38::WheelType::MOTORIZED}));  // isGrounded false
    wheel->Set<bool>("isGrounded", true);
    CHECK(L38::VesselHasWheelsOnGround(&r.v, {L38::WheelType::FREE, L38::WheelType::MOTORIZED}) == (wheel->EnumName("wheelType") != "LEG"));
    // in orbit the check never applies
    r.v.situation = Vessel::Situations::ORBITING;
    CHECK_FALSE(L38::VesselHasWheelsOnGround(&r.v, {L38::WheelType::MOTORIZED, L38::WheelType::FREE, L38::WheelType::LEG}));
    r.v.situation = Vessel::Situations::LANDED;
    Root root;
    auto* p = root.Own(w, std::make_unique<P38::MobileBaseParameter>());
    // the Root's filler is a MobileBaseParameter too; drive the owned one
    for (int i = 0; i < 5; ++i) p->Update();
    CHECK(p->State() == (wheel->EnumName("wheelType") == "MOTORIZED" ? ParameterState::Complete : ParameterState::Incomplete));
}

TEST_CASE("WP-38b ResourcePossession / ResourceExtraction / PartRequest / VesselSystems parameters") {
    World38b w;
    Career38b career(w);
    CelestialBody& mun = w.Body("Mun");
    VRig r(w, mun, {"mk1pod.v2", "fuelTankSmallFlat", "HighGainAntenna", "solarPanels1"}, Vessel::Situations::ORBITING);
    FlightScope fs(&r.v);
    const double fuel = L38::VesselResourceAmount("LiquidFuel", &r.v);
    CHECK(fuel > 0.0);
    CHECK(L38::VesselResourceAmount("NoSuchResource", &r.v) == 0.0);
    {
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::ResourcePossessionParameter>("LiquidFuel", "fuel", "ship", std::floor(fuel)));
        for (int i = 0; i < 5; ++i) p->Update();
        CHECK(p->State() == ParameterState::Complete);  // Math.Ceiling(current) >= goal
        CHECK_F64b(p->currentResource, fuel);
        p->goalResource = fuel + 100.0;
        p->Update();
        CHECK(p->State() == ParameterState::Incomplete);
        ConfigNode n("PARAM");
        p->OnSave(n);
        CHECK(*n.GetValue("resourceName") == "LiquidFuel");
        CHECK(*n.GetValue("vesselName") == "ship");
        RoundTrip(*p, "ResourcePossessionParameter");
        // the sibling SpecificVesselParameter supplies the notes
        auto* sv = root.c->AddParameter(std::make_unique<P38::SpecificVesselParameter>(&r.v));
        CHECK(root.c->GetParameterT<P38::SpecificVesselParameter>() == dynamic_cast<P38::SpecificVesselParameter*>(sv));
        CHECK(p->Notes() != "");
    }
    {
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::ResourceExtractionParameter>("Ore", "ore", 1000.0, &mun, std::vector<std::string>{"ModuleCommand"}));
        PartModule* cmd = nullptr;
        for (size_t i = 0; i < r.parts[0]->Modules().Count(); ++i)
            if (r.parts[0]->Modules()[i]->moduleName == "ModuleCommand") cmd = r.parts[0]->Modules()[i];
        REQUIRE(cmd != nullptr);
        L38::FinePrintEnvironment::screenMessages.clear();
        events::GameEvents::OnResourceConverterOutput.Fire(cmd, std::string("Ore"), 300.0);  // 30 % >= 25 %: first notification, level 0.25 -> 0.5
        CHECK_F64b(p->totalHarvested, 300.0);
        CHECK_F32b(p->notifyLevel, 0.5f);
        CHECK(L38::FinePrintEnvironment::screenMessages.size() == 1);
        events::GameEvents::OnResourceConverterOutput.Fire(cmd, std::string("Other"), 5000.0);   // wrong resource
        events::GameEvents::OnResourceConverterOutput.Fire(cmd, std::string("Ore"), -1.0);       // not positive
        CHECK_F64b(p->totalHarvested, 300.0);
        events::GameEvents::OnResourceConverterOutput.Fire(cmd, std::string("Ore"), 700.0);
        CHECK_F64b(p->totalHarvested, 1000.0);
        for (int i = 0; i < 5; ++i) p->Update();
        CHECK(p->State() == ParameterState::Complete);
        ConfigNode n("PARAM");
        p->OnSave(n);
        CHECK(*n.GetValue("totalHarvested") == "1000");
        CHECK(*n.GetValue("modules") == "ModuleCommand");
        RoundTrip(*p, "ResourceExtractionParameter");
    }
    {
        // PartRequestParameter: existing parts of the vessel at creation are ignored
        ConfigNode req("PART_REQUEST");
        req.AddValue("Part", "HighGainAntenna");
        req.AddValue("Module", "ModuleSolarPanelNoSuchModule");
        P38::PartRequestParameter fresh(req, &r.v);
        CHECK(fresh.existingParts.size() == 1);
        CHECK(fresh.GetTitle() == Localizer38b::Format("#autoLOC_7001087", {"part", "vessel"}));
        CHECK_FALSE(fresh.VesselHasPartRequest(&r.v));
        Part* second = r.AddPart(w, "HighGainAntenna");
        second->fields.flightID = 987654u;
        CHECK(fresh.VesselHasPartRequest(&r.v));
        ConfigNode n("PARAM");
        fresh.OnSave(n);
        CHECK(n.GetValue("partNames") == std::optional<std::string>("HighGainAntenna"));
        CHECK(*n.GetValue("moduleNames") == "ModuleSolarPanelNoSuchModule");
        r.v.parts.pop_back();
    }
    {
        // VesselSystemsParameter: objective modules of the vessel, the launch id, the manned status
        CHECK(L38::HasValidContractObjectives(r.v, {"Antenna", "Generator"}));
        CHECK_FALSE(L38::HasValidContractObjectives(r.v, {"Antenna", "Dock"}));
        CHECK(L38::VesselLaunchedAfterID(0u, &r.v, {}));
        r.parts[0]->fields.launchID = 5u;
        r.parts[1]->fields.launchID = 9u;
        r.parts[2]->fields.launchID = 9u;
        r.parts[3]->fields.launchID = 9u;
        CHECK_FALSE(L38::VesselLaunchedAfterID(6u, &r.v, {}));
        CHECK(L38::VesselLaunchedAfterID(6u, &r.v, {"mk1pod_v2"}));  // the ignored part name has '_' -> '.'
        CHECK(L38::VesselLaunchedAfterID(5u, &r.v, {}));
        r.v.CheckControllable();
        P38::VesselSystemsParameter vsp({"Antenna", "Generator"}, {"a", "b"}, "probe", L38::MannedStatus::UNMANNED, true);
        vsp.launchID = 5u;
        CHECK(vsp.isVesselValid(&r.v) == r.v.IsControllable());
        vsp.launchID = 6u;
        CHECK_FALSE(vsp.isVesselValid(&r.v));
        CHECK_FALSE(vsp.isVesselValid(nullptr));
        ConfigNode n("PARAM");
        vsp.OnSave(n);
        CHECK(*n.GetValue("typeString") == "probe");
        CHECK(*n.GetValue("mannedStatus") == "1");
        CHECK(*n.GetValue("requireNew") == "True");
        CHECK(*n.GetValue("checkModuleTypes") == "Antenna|Generator");
        CHECK(*n.GetValue("launchID") == "6");
        P38::VesselSystemsParameter back;
        back.OnLoad(n);
        CHECK(back.launchID == 6u);
        CHECK(back.mannedStatus == L38::MannedStatus::UNMANNED);
        CHECK(back.checkModuleTypes == std::vector<std::string>{"Antenna", "Generator"});
    }
}

TEST_CASE("WP-38b kerbal parameters: KerbalDestination flight logs, KerbalGeeAdventure / KerbalTour deaths, tourist flags, recovery") {
    World38b w;
    Career38b career(w);
    CelestialBody& mun = w.Body("Mun");
    auto k = MakeKerbal(w, "Bob Kerman", ProtoCrewMember_KerbalType::Tourist);
    const std::string name = k->name().value_or("");
    REQUIRE(!name.empty());
    {
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::KerbalDestinationParameter>(&mun, FlightLog_EntryType::Orbit, name), false);
        root.c->state = Contract::State::Active;
        CHECK_FALSE(p->CheckFlightLogs());  // empty logs
        k->flightLog.AddEntry(FlightLog_EntryType::Orbit, std::string("Mun"));
        CHECK(p->CheckFlightLogs());
        CHECK(p->State() == ParameterState::Complete);
        ConfigNode n("PARAM");
        p->OnSave(n);
        CHECK(*n.GetValue("targetType") == "Orbit");
        CHECK(*n.GetValue("kerbalName") == name);
        RoundTrip(*p, "KerbalDestinationParameter");
        k->flightLog.Clear();
    }
    {
        // KerbalTourParameter.OnCrewKilled: partial failure is switched off, then the parameter fails
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::KerbalTourParameter>(name, ProtoCrewMember_Gender::Female));
        CHECK(p->allowPartialFailure);
        VRig dead(w, mun, {"mk1pod.v2"});  // the progress tracker reads report.origin.vessel
        EventReport evt(FlightEvents::CREW_KILLED, dead.parts[0].get(), name);
        events::GameEvents::onCrewKilled.Fire(&evt);
        CHECK(p->State() == ParameterState::Failed);
        CHECK_FALSE(p->allowPartialFailure);
        CHECK(root.c->ContractState() == Contract::State::Failed);  // allowPartialFailure was cleared before SetFailed: the contract fails
    }
    {
        // KerbalInactiveChange fails with a dedicated message while FailOnInactive is set
        Root root;
        auto* tq = root.Own(w, std::make_unique<P38::KerbalTourParameter>(name, ProtoCrewMember_Gender::Male));
        tq->OnKerbalInactiveChange(k.get(), false, true);
        CHECK(tq->kerbalInactive);
        CHECK(k->hasToured);
        CHECK(tq->State() == ParameterState::Failed);
        CHECK(tq->MessageFailed() == Localizer38b::Format("#autoLOC_283554", {tq->Title(), name}));
        ConfigNode n("PARAM");
        tq->OnSave(n);
        CHECK(*n.GetValue("kerbalGender") == "Male");
        RoundTrip(*tq, "KerbalTourParameter");
        k->hasToured = false;
    }
    {
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::KerbalGeeAdventureParameter>(name));
        VRig dead(w, mun, {"mk1pod.v2"});
        EventReport other(FlightEvents::CREW_KILLED, dead.parts[0].get(), "Somebody Else");
        events::GameEvents::onCrewKilled.Fire(&other);
        CHECK(p->State() == ParameterState::Incomplete);
        // OnKerbalPassedOut completes only when the children are complete (none: AllChildParametersComplete is true)
        events::GameEvents::onKerbalPassedOutFromGeeForce.Fire(k.get());
        CHECK(p->State() == ParameterState::Complete);
        CHECK(k->hasToured);
        RoundTrip(*p, "KerbalGeeAdventureParameter");
    }
}

TEST_CASE("WP-38b asteroid / comet parameters: seeds from docked space objects, tow check, node format; CometDetection; CometScience") {
    World38b w;
    Career38b career(w);
    CelestialBody& sun = w.Body("Sun");
    VRig r(w, sun, {"mk1pod.v2"}, Vessel::Situations::ORBITING);
    Part* rock = r.AddPart(w, "mk1pod.v2");
    rock->AddModule("ModuleAsteroid");
    PartModule* m = rock->Modules()[rock->Modules().Count() - 1];
    REQUIRE(m->IsA("ModuleAsteroid"));
    m->Set<int32_t>("seed", 4711);
    FlightScope fs(&r.v);
    Root root;
    auto* p = root.Own(w, std::make_unique<P38::AsteroidParameter>("c", false));
    CHECK(p->asteroidClass == "C");
    // not tracked yet: no seeds
    CHECK_FALSE(p->VesselTowingAsteroid(&r.v));
    p->asteroidSeeds.push_back(4711);
    CHECK(p->VesselTowingAsteroid(&r.v));
    p->dirtyVessel = true;
    for (int i = 0; i < 5; ++i) p->Update();
    CHECK(p->State() == ParameterState::Complete);
    ConfigNode n("PARAM");
    p->OnSave(n);
    CHECK(*n.GetValue("asteroidClass") == "C");
    CHECK(*n.GetValue("forStation") == "False");
    CHECK(*n.GetValue("seedList") == "4711");
    RoundTrip(*p, "AsteroidParameter");
    // CometDetectionParameter: GameEvents.onSentinelCometDetected counts; the last one completes the parameter and the contract
    Root root2;
    auto* cd = root2.Own(w, std::make_unique<P38::CometDetectionParameter>(2, "sentinel"));
    events::GameEvents::onSentinelCometDetected.Fire(&r.v);
    CHECK(cd->cometCountDetected == 1);
    CHECK(cd->RemainingDiscoveries == 1);
    CHECK(cd->State() == ParameterState::Incomplete);
    CHECK(L38::FinePrintEnvironment::screenMessages.back() == Localizer38b::Format("#autoLOC_8003397", {"1", "2", Localizer38b::Format(root2.c->Agent()->Title())}));
    ConfigNode cn("PARAM");
    cd->OnSave(cn);
    CHECK(*cn.GetValue("cometCount") == "1");
    CHECK(*cn.GetValue("cometCountRequired") == "2");
    RoundTrip(*cd, "CometDetectionParameter");
    // CometScienceParameter: the subject id changes on rename until the data is collected; OnScience completes the contract
    Root root3;
    auto* cs = root3.Own(w, std::make_unique<P38::CometScienceParameter>(77u, "Borrelly", "cometSample_Short@SunInSpaceHigh_Borrelly"));
    L38::FinePrintEnvironment::cometTypeName = [](Vessel&) { return std::string("Long"); };
    r.v.persistentId = 77u;
    events::GameEvents::onVesselRename.Fire(events::GameEvents::HostedFromToAction<Vessel*, std::string>(&r.v, "Borrelly", "Halley"));
    CHECK(cs->cometName == "Halley");
    CHECK(cs->scienceSubjectId == "cometSample_Long@SunInSpaceHigh_Halley");
    events::GameEvents::onVesselPersistentIdChanged.Fire(77u, 78u);
    CHECK(cs->cometPersistentId == 78u);
    ScienceData data(5.0f, 1.0f, 0.0f, "cometSample_Long@SunInSpaceHigh_Halley", "Halley");
    events::GameEvents::OnExperimentDeployed.Fire(&data);
    CHECK(cs->scienceDataCollected);
    events::GameEvents::OnTriggeredDataTransmission.Fire(&data, &r.v, true);  // aborted: ignored
    CHECK(cs->State() == ParameterState::Incomplete);
    events::GameEvents::OnTriggeredDataTransmission.Fire(&data, &r.v, false);
    CHECK(cs->State() == ParameterState::Complete);
    ConfigNode sn("PARAM");
    cs->OnSave(sn);
    CHECK(*sn.GetValue("cometName") == "Halley");
    CHECK(*sn.GetValue("cometPersistentId") == "78");
    CHECK(*sn.GetValue("scienceDataCollected") == "True");
    RoundTrip(*cs, "CometScienceParameter");
    L38::FinePrintEnvironment::cometTypeName = nullptr;
}

TEST_CASE("WP-38b construction / repair parameters, SpecificOrbit validation and save, Rover and Stationary waypoint parameters") {
    World38b w;
    Career38b career(w);
    CelestialBody& duna = w.Body("Duna");
    {
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::ConstructionParameter>("cupola", 1234u, "Duna Base", &duna));
        auto* q = root.c->AddParameter(std::make_unique<P38::RepairPartParameter>(55u, "cupola", 1234u, "Duna Base", &duna));
        q->Register();
        auto* rp = dynamic_cast<P38::RepairPartParameter*>(q);
        // vessel / part persistent id changes follow the C# handlers
        events::GameEvents::onVesselPersistentIdChanged.Fire(1234u, 1235u);
        CHECK(p->vesselPersistentId == 1235u);
        CHECK(rp->vesselPersistentId == 1235u);
        events::GameEvents::onPartPersistentIdChanged.Fire(1235u, 55u, 56u);
        CHECK(rp->partPersistentId == 56u);
        events::GameEvents::onPartPersistentIdChanged.Fire(9999u, 56u, 57u);
        CHECK(rp->partPersistentId == 56u);
        ConfigNode n("PARAM");
        rp->OnSave(n);
        CHECK(*n.GetValue("bodyName") == "Duna");
        CHECK(*n.GetValue("partName") == "cupola");
        CHECK(*n.GetValue("partPersistentId") == "56");
        RoundTrip(*rp, "RepairPartParameter");
        ConfigNode cn("PARAM");
        p->OnSave(cn);
        CHECK(*cn.GetValue("vesselPersistentId") == "1235");
        RoundTrip(*p, "ConstructionParameter");
        // wrong vessel / part: nothing happens
        VRig other(w, duna, {"cupola"}, Vessel::Situations::LANDED);
        other.v.persistentId = 77u;
        events::GameEvents::OnEVAConstructionModePartAttached.Fire(&other.v, other.parts[0].get());
        CHECK(p->State() == ParameterState::Incomplete);
        events::GameEvents::onPartRepaired.Fire(other.parts[0].get());
        CHECK(rp->State() == ParameterState::Incomplete);
    }
    {
        // OnConstructionPartAttached (the matching vessel and part name) completes the parameter and then the contract
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::ConstructionParameter>("cupola", 1234u, "Duna Base", &duna));
        VRig r(w, duna, {"cupola"}, Vessel::Situations::LANDED);
        r.v.persistentId = 1234u;
        events::GameEvents::OnEVAConstructionModePartAttached.Fire(&r.v, r.parts[0].get());
        CHECK(p->State() == ParameterState::Complete);
    }
    {
        // OnPartRepaired: the part's persistent id (0 = any part) on the vessel with the persistent id
        Root root;
        root.c->prestige = ContractPrestige::Trivial;
        auto* q = root.Own(w, std::make_unique<P38::RepairPartParameter>(55u, "cupola", 1234u, "Duna Base", &duna));
        VRig r(w, duna, {"cupola"}, Vessel::Situations::LANDED);
        r.v.persistentId = 1234u;
        r.parts[0]->fields.persistentId = 55u;
        events::GameEvents::onPartRepaired.Fire(r.parts[0].get());
        CHECK(q->State() == ParameterState::Complete);
    }
    {
        // SpecificOrbitParameter: node format, ValidateOrbit leaves a valid orbit alone
        Root root;
        root.c->prestige = ContractPrestige::Significant;
        auto* p = root.Own(w, std::make_unique<P38::SpecificOrbitParameter>(FP::OrbitType::POLAR, 90.0, 0.01, 800000.0, 12.0, 34.0, 0.5, 100.0, &w.Body("Kerbin"), 5.0));
        ConfigNode n("PARAM");
        p->OnSave(n);
        CHECK(*n.GetValue("TargetBody") == std::to_string(w.Body("Kerbin").flightGlobalsIndex));
        CHECK(*n.GetValue("deviationWindow") == "5");
        CHECK(*n.GetValue("orbitType") == "2");
        CHECK(*n.GetValue("sma") == "800000");
        CHECK(*n.GetValue("lan") == "12");
        RoundTrip(*p, "SpecificOrbitParameter");
        // TriggerRange of the survey parameters follows the ContractDefs trigger ranges
        const double lo = ContractDefsLite::D("Survey", "", "MinimumTriggerRange");
        const double hi = ContractDefsLite::D("Survey", "", "MaximumTriggerRange");
        CHECK_F64b(P38::SurveyWaypointParameter::TriggerRange(L38::FlightBand::GROUND), std::min(lo, hi));
        CHECK_F64b(P38::SurveyWaypointParameter::TriggerRange(L38::FlightBand::LOW), (lo + hi) / 2.0);
        CHECK_F64b(P38::SurveyWaypointParameter::TriggerRange(L38::FlightBand::HIGH), hi);
        CHECK_F64b(P38::SurveyWaypointParameter::TriggerRange(L38::FlightBand::NONE), 0.0);
    }
    {
        // StationaryPointParameter: longitude within deviation / 100 * 360 degrees of the vessel (wrap around 180)
        VRig r(w, w.Body("Kerbin"), {"mk1pod.v2"}, Vessel::Situations::ORBITING);
        FlightScope fs(&r.v);
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::StationaryPointParameter>(&w.Body("Kerbin"), 179.0, 5.0));
        p->submittedWaypoint = true;  // OnUpdate acts only once the waypoint was submitted (flight / track station load)
        r.v.longitude = -179.0;      // 2 degrees away across the date line: within 18 degrees
        for (int i = 0; i < 5; ++i) p->Update();
        CHECK(p->State() == ParameterState::Complete);
        r.v.longitude = 100.0;
        p->Update();
        CHECK(p->State() == ParameterState::Incomplete);
        ConfigNode n("PARAM");
        p->OnSave(n);
        CHECK(*n.GetValue("longitude") == "179");
        CHECK(*n.GetValue("deviation") == "5");
        RoundTrip(*p, "StationaryPointParameter");
    }
    {
        // RoverWayPointParameter: save format and the persistent id tracking
        Root root;
        auto* p = root.Own(w, std::make_unique<P38::RoverWayPointParameter>(1.5, 2.5, 3.5, 4.5, &w.Body("Mun")));
        p->roverVslId = 11u;
        events::GameEvents::onVesselPersistentIdChanged.Fire(11u, 12u);
        CHECK(p->roverVslId == 12u);
        ConfigNode n("PARAM");
        p->OnSave(n);
        CHECK(*n.GetValue("bodyName") == "Mun");
        CHECK(*n.GetValue("craftEndLat") == "3.5");
        CHECK(*n.GetValue("roverVslId") == "12");
        CHECK(n.HasValue("endWPName"));
        RoundTrip(*p, "RoverWayPointParameter");
    }
}

// ---- VesselUtilities, SurveyDefinitions, ScenarioContractEvents, InstallAll seams ------------------------------------------------------------------
TEST_CASE("WP-38b VesselUtilities: part / module / resource queries, crew, ownership, flight band, classification, orbit match") {
    World38b w;
    Career38b career(w);
    CelestialBody& kerbin = w.Body("Kerbin");
    CelestialBody& mun = w.Body("Mun");
    VRig r(w, kerbin, {"mk1pod.v2", "fuelTankSmallFlat", "HighGainAntenna", "solarPanels1"}, Vessel::Situations::LANDED);
    r.v.Landed = true;
    r.v.latitude = 10.0;
    r.v.longitude = -30.0;
    r.v.altitude = 500.0;
    r.v.id = Guid::Parse("11111111-2222-3333-4444-555555555555");
    FlightRegistry::Vessels = {&r.v};
    // ActiveVesselFallback: no active vessel outside flight, the active vessel inside
    Vessel* v = nullptr;
    CHECK_FALSE(L38::ActiveVesselFallback(v, false));
    {
        FlightScope fs(&r.v);
        v = nullptr;
        CHECK(L38::ActiveVesselFallback(v, false));
        CHECK(v == &r.v);
        CHECK(L38::VesselHasPartName("HighGainAntenna"));
        CHECK(L38::VesselHasPartName("mk1pod_v2"));  // "_" stands for "."
        CHECK_FALSE(L38::VesselHasPartName("noSuchPart"));
    }
    CHECK(L38::VesselHasModuleName("ModuleCommand", &r.v));
    CHECK_FALSE(L38::VesselHasModuleName("ModuleNothing", &r.v));
    std::vector<std::string> parts{"noSuch", "solarPanels1"};
    std::vector<std::string> modules{"ModuleNothing"};
    CHECK(L38::VesselHasAnyParts(&parts, &r.v));
    CHECK_FALSE(L38::VesselHasAnyModules(&modules, &r.v));
    CHECK(L38::VesselHasAnyPartsOrModules(&parts, &modules, &r.v));
    CHECK_FALSE(L38::VesselHasAnyPartsOrModules(nullptr, nullptr, &r.v));
    CHECK(L38::VesselHasAnyPartsOrModules(nullptr, &modules, &r.v) == L38::VesselHasAnyModules(&modules, &r.v));
    CHECK(L38::VesselPartAndModuleCount({"mk1pod_v2", "HighGainAntenna"}, {"ModuleCommand"}, &r.v) >= 3);
    // part ids
    for (size_t i = 0; i < r.parts.size(); ++i) r.parts[i]->fields.flightID = 100u + static_cast<uint32_t>(i);
    std::vector<uint32_t> ids = L38::GetPartIDList(&r.v);
    CHECK(ids == std::vector<uint32_t>{103u, 102u, 101u, 100u});  // walked from the end
    CHECK(L38::FindVesselWithPartIDs(&ids) == &r.v);
    std::vector<uint32_t> none{1u, 2u};
    CHECK(L38::FindVesselWithPartIDs(&none) == nullptr);
    CHECK(L38::FindVesselWithPartIDs(nullptr) == nullptr);
    CHECK(L38::VesselID(&r.v) == 100u);
    // resources, crew capacity
    CHECK(L38::ActualCrewCapacity(&r.v) == r.v.GetCrewCapacity());
    // ownership / discovery
    CHECK(L38::VesselIsOwned(&r.v));  // DiscoveryInfo defaults to Owned
    r.v.GetDiscoveryInfo().SetLevel(DiscoveryLevels::Presence);
    CHECK_FALSE(L38::VesselIsOwned(&r.v));
    L38::DiscoverVessel(&r.v);
    CHECK(L38::VesselIsOwned(&r.v));
    // flight band
    CHECK(L38::GetFlightBand(1000.0, &r.v) == L38::FlightBand::GROUND);
    r.v.Landed = false;
    r.v.situation = Vessel::Situations::FLYING;
    r.v.altitude = 5000.0;
    CHECK(L38::GetFlightBand(1000.0, &r.v) == L38::FlightBand::HIGH);
    CHECK(L38::GetFlightBand(9000.0, &r.v) == L38::FlightBand::LOW);
    // classification: a command vessel with antenna and generator and no crew is a probe; landed with a big crew capacity and a docking port: a base
    r.v.CheckControllable();
    CHECK(L38::ClassifyVesselType(&r.v, false) != VesselType::Station);
    CHECK(L38::ClassifyVesselType(&r.v, true) == VesselType::Ship);  // the vessel type is returned when nothing else matches
    // SpecificVesselClassAt walks FlightGlobals.Vessels from the end; here: one vessel at Kerbin, none at Mun
    CHECK(L38::SpecificVesselClassAt(&mun, VesselType::Base).empty());
    CHECK(L38::SpecificVesselClassAt(&kerbin, VesselType::Ship, true, false, true).size() == 1);
    CHECK(L38::SpecificVesselClassAt(&kerbin, VesselType::Ship, true, true, true).size() == 1);  // excludeActive only matters in flight
    CHECK(L38::SpecificVesselClassAt(nullptr, VesselType::Ship).empty());
    auto classes = L38::ClassifyAllVesselsAt(&kerbin, true);
    REQUIRE(classes.size() == 1);
    CHECK(classes[0].first == &r.v);
    // VesselAtOrbit: the vessel must orbit the body of the orbit with PeA / ApA / inclination / LAN / argument inside the window
    r.v.situation = Vessel::Situations::ORBITING;
    Orbit* vo = r.v.orbitDriver->orbit.get();
    vo->SetOrbit(10.0, 0.0, 700000.0, 40.0, 20.0, 0.0, 0.0, &kerbin);
    Orbit same(10.0, 0.0, 700000.0, 40.0, 20.0, 0.0, 0.0, &kerbin);
    CHECK(L38::VesselAtOrbit(&same, 5.0, &r.v));
    Orbit higher(10.0, 0.0, 900000.0, 40.0, 20.0, 0.0, 0.0, &kerbin);
    CHECK_FALSE(L38::VesselAtOrbit(&higher, 5.0, &r.v));
    Orbit tilted(60.0, 0.0, 700000.0, 40.0, 20.0, 0.0, 0.0, &kerbin);
    CHECK_FALSE(L38::VesselAtOrbit(&tilted, 5.0, &r.v));
    Orbit moon(10.0, 0.0, 700000.0, 40.0, 20.0, 0.0, 0.0, &mun);
    CHECK_FALSE(L38::VesselAtOrbit(&moon, 5.0, &r.v));
    CHECK_FALSE(L38::VesselAtOrbit(nullptr, 5.0, &r.v));
    CHECK(L38::VesselAtOrbitAccuracy(&same, 5.0, &r.v) > 0.0);
    CHECK_F64b(L38::VesselAtOrbitAccuracy(&higher, 5.0, &r.v), 0.0);
    // GenerateAdjustedVesselOrbit draws from the given generator: same seed, same orbit; the result differs from the vessel's orbit
    KSPRandom g1(55), g2(55);
    Orbit a1 = L38::GenerateAdjustedVesselOrbit(7.0, 5.0, 2, &g1, &r.v);
    Orbit a2 = L38::GenerateAdjustedVesselOrbit(7.0, 5.0, 2, &g2, &r.v);
    CHECK_F64b(a1.semiMajorAxis, a2.semiMajorAxis);
    CHECK_F64b(a1.inclination, a2.inclination);
    CHECK((a1.semiMajorAxis != vo->semiMajorAxis || a1.inclination != vo->inclination || a1.LAN != vo->LAN));
    // an escape orbit comes back unchanged
    vo->SetOrbit(10.0, 1.5, -700000.0, 40.0, 20.0, 0.0, 0.0, &kerbin);
    Orbit a3 = L38::GenerateAdjustedVesselOrbit(7.0, 5.0, 2, &g1, &r.v);
    CHECK_F64b(a3.eccentricity, 1.5);
    CHECK(L38::FindFirstPartOrModuleName(nullptr, nullptr) == nullptr);
    // HighestPodCapacity: the real prefabs of the mirror (mk1-3pod seats 3, the cockpit / cabin seat up to the Mk3 pod)
    CHECK(L38::HighestPodCapacity() >= 3);
}

TEST_CASE("WP-38b SurveyDefinition / SurveyDefinitionParameter: the Contracts.cfg survey definitions, copies keep the order, parameters are copied reversed") {
    World38b w;
    L38::ResetSurveyDefinitions();
    ConfigNode* survey = ContractDefsLite::Node("Survey");
    REQUIRE(survey != nullptr);
    std::vector<ConfigNode*> nodes = survey->GetNodes("SURVEY_DEFINITION");
    REQUIRE(nodes.size() > 1);
    auto defs = L38::SurveyDefinitions();
    REQUIRE(defs.size() == nodes.size());
    for (size_t i = 0; i < defs.size(); ++i) {
        // the getter builds the list from the nodes in reverse and returns a reverse copy: the node order
        CHECK(defs[i]->DataName == nodes[i]->GetValue("DataName").value_or("generic"));
        CHECK(defs[i]->Node == nodes[i]);
        std::vector<ConfigNode*> pnodes = nodes[i]->GetNodes("PARAM");
        REQUIRE(defs[i]->Parameters.size() == pnodes.size());
        // SurveyDefinition(node) adds the PARAM nodes from the last; the copy constructor reverses again, the getter copies once more: node order again
        for (size_t j = 0; j < pnodes.size(); ++j) {
            CHECK(defs[i]->Parameters[j]->Experiment == pnodes[j]->GetValue("Experiment").value_or("experiment"));
            CHECK(defs[i]->Parameters[j]->Definition != nullptr);
        }
    }
    // a second call hands out an independent copy
    defs[0]->Parameters.clear();
    auto defs2 = L38::SurveyDefinitions();
    CHECK(defs2[0]->Parameters.size() == nodes[0]->GetNodes("PARAM").size());
    // defaults of the C# initialisers
    L38::SurveyDefinitionParameter dp;
    CHECK(dp.Experiment == "experiment");
    CHECK(dp.AllowGround);
    CHECK_FALSE(dp.CrewRequired);
    ConfigNode pn("PARAM");
    pn.AddValue("EVARequired", "True");
    pn.AddValue("Tech", "a");
    pn.AddValue("Tech", "b");
    L38::SurveyDefinition sd;
    L38::SurveyDefinitionParameter ep(&sd, &pn);
    CHECK(ep.EVARequired);
    CHECK(ep.CrewRequired);  // EVARequired implies CrewRequired
    CHECK(ep.Tech == std::vector<std::string>{"a", "b"});
    L38::SurveyDefinitionParameter cp(ep);
    CHECK(cp.Tech == std::vector<std::string>{"b", "a"});  // the copy constructor walks the list from the end
    CHECK_F32b(sd.FundsReward, ContractDefsLite::F("Survey", "Funds", "DefaultReward"));
    L38::ResetSurveyDefinitions();
}

TEST_CASE("WP-38b ScenarioContractEvents: game seed, Invoke relay to the SpecificVessel parameters, tourist departures, registration") {
    World38b w;
    Career38b career(w);
    FP::ScenarioContractEvents::gameSeed = -1;
    FP::ScenarioContractEvents sce;
    ConfigNode save("SCENARIO");
    ConfigNode load("SCENARIO");
    sce.OnLoad(load);  // no id: a seed is drawn and never -1
    CHECK(FP::ScenarioContractEvents::GameSeed() != -1);
    const int seed = FP::ScenarioContractEvents::GameSeed();
    sce.OnSave(save);
    CHECK(*save.GetValue("id") == std::to_string(seed));
    ConfigNode again("SCENARIO");
    again.AddValue("id", "12345");
    FP::ScenarioContractEvents other;
    other.OnLoad(again);
    CHECK(FP::ScenarioContractEvents::GameSeed() == 12345);
    // events: the cache collects the vessel ids, the timer fires after 0.2 s and clears the cache
    sce.OnAwake();
    VRig r(w, w.Body("Kerbin"), {"mk1pod.v2"}, Vessel::Situations::ORBITING);
    r.v.id = Guid::Parse("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee");
    events::GameEvents::onVesselWasModified.Fire(&r.v);
    CHECK(sce.specificVesselCache.size() == 1);
    CHECK(sce.IsInvoking());
    sce.AdvanceTime(0.1f);
    CHECK(sce.IsInvoking());
    sce.AdvanceTime(0.2f);
    CHECK_FALSE(sce.IsInvoking());
    CHECK(sce.specificVesselCache.empty());
    // the rename relay reaches SpecificVesselRename of the contract's parameters when the contract implements ICheckSpecificVessels
    w.game.Mode = Game::Modes::CAREER;
    auto base = std::make_unique<C38::BaseContract>();
    base->targetBody = &w.Body("Kerbin");
    base->agent = w.agents->GetAgent("Kerbodyne");
    auto* sv = dynamic_cast<P38::SpecificVesselParameter*>(base->AddParameter(std::make_unique<P38::SpecificVesselParameter>(&r.v)));
    sv->tracked = true;
    Contract* raw = base.get();
    career.cs.Contracts().push_back(std::move(base));
    int refreshed = 0;
    P38::SpecificVesselParameter::refreshContractRequested = [&](Contract& c, bool changePositions) {
        CHECK(&c == raw);
        CHECK_FALSE(changePositions);
        ++refreshed;
    };
    events::GameEvents::onVesselRename.Fire(events::GameEvents::HostedFromToAction<Vessel*, std::string>(&r.v, "Test Vessel", "Renamed"));
    CHECK(sv->targetVesselName == "Renamed");
    CHECK(refreshed == 1);
    P38::SpecificVesselParameter::refreshContractRequested = nullptr;
    // SpecificVesselParameter tracks the vessel by its part ids
    FlightRegistry::Vessels = {&r.v};
    sv->targetVessel = nullptr;
    sv->TargetVesselSearch(false);
    CHECK(sv->targetVessel == &r.v);
    CHECK(sv->targetVesselPartCount == 1);
    CHECK(sv->TargetBody == &w.Body("Kerbin"));
    // a vanished vessel: the contract is killed on the next frame (the coroutine's second segment)
    FlightRegistry::Vessels.clear();
    sv->tracked = true;
    sv->partIDs = {424242u};
    sv->TargetVesselSearch(false);
    CHECK(sv->targetVessel == nullptr);
    CHECK(L38::FinePrintEnvironment::deferred.size() == 1);
    L38::FinePrintEnvironment::RunDeferred();  // KillContract: tracked = false, SetIncomplete, then the frame after
    CHECK_FALSE(sv->tracked);
    CHECK(L38::FinePrintEnvironment::deferred.size() == 1);
    // remove the contract from the system before the deferred second segment may delete it
    L38::FinePrintEnvironment::deferred.clear();
    sce.OnDestroy();
    // registration
    CHECK_NOTHROW(FP::ScenarioContractEvents::Register());
}

TEST_CASE("WP-38b InstallAll: contract and parameter types are registered, the FinePrint seams answer, ContractDefs values are read from Contracts.cfg") {
    World38b w;
    for (const char* t : {"ARMContract", "BaseContract", "CometSampleContract", "ISRUContract", "SatelliteContract", "StationContract", "SurveyContract", "TourismContract",
                          "ExplorationContract"})
        CHECK(ContractSystem::HasContractType(t));
    for (const char* t : {"AsteroidParameter", "CometDetectionParameter", "CometParameter", "CometScienceParameter", "ConstructionParameter", "CrewCapacityParameter",
                          "CrewTraitParameter", "KerbalDestinationParameter", "KerbalGeeAdventureParameter", "KerbalTourParameter", "LocationAndSituationParameter",
                          "MobileBaseParameter", "PartRequestParameter", "RepairPartParameter", "ResourceExtractionParameter", "ResourcePossessionParameter",
                          "RoverWayPointParameter", "SpecificOrbitParameter", "SpecificVesselParameter", "StabilityParameter", "StationaryPointParameter",
                          "SurveyWaypointParameter", "VesselDestinationParameter", "VesselSystemsParameter", "ProgressTrackingParameter"}) {
        INFO(t);
        CHECK(ContractSystem::HasParameterType(t));
        CHECK(ContractSystem::CreateParameter(t) != nullptr);
    }
    // CareerEnvironment::valueAndUnits and the GrandTour factories
    CHECK(CareerEnvironment::Current().valueAndUnits(static_cast<int>(L38::RecordTrackType::SPEED), 12.0) == L38::StringUtilities::ValueAndUnits(L38::RecordTrackType::SPEED, 12.0));
    auto vs = Templates::TemplateEnvironment::makeVesselSystemsParameter("probe");
    REQUIRE(vs != nullptr);
    CHECK(vs->TypeName() == "VesselSystemsParameter");
    auto vd = Templates::TemplateEnvironment::makeVesselDestinationParameter(&w.Body("Mun"), static_cast<int>(FlightLog_EntryType::Land));
    CHECK(vd->Title() == Localizer38b::Format("#autoLOC_286262", {w.Body("Mun").displayName()}));
    auto ls = Templates::TemplateEnvironment::makeLocationAndSituationParameter(&w.Body("Mun"), Vessel::Situations::LANDED, "probe", true);
    CHECK(ls->Title() == Localizer38b::Format("#autoLOC_7000013", {"0", "probe", w.Body("Mun").displayName()}));
    // the real ResearchedValidContractObjectives walks the loaded part prefabs of the mirror (no R&D: every part tech is available)
    L38::FinePrintEnvironment::researchedValidContractObjectives = nullptr;
    CHECK(L38::ResearchedValidContractObjectives({"Antenna"}));
    CHECK(L38::ResearchedValidContractObjectives({"Antenna", "Generator", "Dock", "Wheel"}));
    CHECK_FALSE(L38::ResearchedValidContractObjectives({"NoSuchObjective"}));
    CHECK_FALSE(L38::ResearchedValidContractObjectives({}));
    CHECK(Templates::TemplateEnvironment::researchedValidContractObjectives("Dock"));
    L38::FinePrintEnvironment::haveModuleTech = nullptr;
    CHECK(L38::ProgressUtilities::HaveModuleTech("ModuleCommand"));
    { std::string mc = "ModuleCommand"; CHECK_FALSE(L38::ProgressUtilities::HaveModuleTech("ModuleCommand", &mc)); }
    CHECK_FALSE(L38::ProgressUtilities::HaveModuleTech("ModuleNoSuchModule"));
    CHECK(L38::ProgressUtilities::HavePartTech("mk1pod_v2", false));
    CHECK_FALSE(L38::ProgressUtilities::HavePartTech("noSuchPart", false));
    L38::FinePrintEnvironment::probeCoresUnlocked = nullptr;
    CHECK(C38::SatelliteContract::ProbeCoresUnlocked());
    // the experiment geometry of ExperimentPossibleAt (altitude bands against the body's science thresholds)
    std::string exp = "crewReport";
    CelestialBody& kerbin = w.Body("Kerbin");
    {
        // an experiment known to the (cfg-less) test world answers through IsAvailableWhile, an unknown one is never possible
        ScienceExperiment* known = ResearchAndDevelopment::GetExperiment(exp);
        const bool possible = L38::ProgressUtilities::ExperimentPossibleAt(&exp, &kerbin, 0.0, 0.0, 10.0, 0.0);
        if (known != nullptr) CHECK(possible == known->IsAvailableWhile(ExperimentSituations::FlyingLow, kerbin));
        else CHECK_FALSE(possible);
    }
    CHECK_FALSE(L38::ProgressUtilities::ExperimentPossibleAt(nullptr, &kerbin, 0.0, 0.0, 10.0, 0.0));
    std::string nope = "noSuchExperiment";
    CHECK_FALSE(L38::ProgressUtilities::ExperimentPossibleAt(&nope, &kerbin, 0.0, 0.0, 10.0, 0.0));
}
