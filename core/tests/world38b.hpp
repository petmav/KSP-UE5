// WP-38b test scaffolding shared by test_fineprint2.cpp and test_fineprint3.cpp (moved verbatim out of test_fineprint2.cpp in WP-38 slice 3):
// the reference loader, World38b (stock Kerbol system, CAREER game, contract system, FinePrint types), VRig / Root / FlightScope / Career38b.
#pragma once
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
#include "ksp/kerbals/crew_generator.hpp"
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
