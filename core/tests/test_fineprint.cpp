// WP-38 unit tests: FinePrint utilities, random positions / orbit generation and the ExplorationContract (core/include/ksp/fineprint,
// contracts/fineprint_lite.hpp, contracts/waypoints.hpp).
//
// No golden or capture of FinePrint exists (harness/golden/MANIFEST.json and harness/captures/out*/ carry no contract data), so every
// expectation is derived from tools/wp38/fineprint_reference.py (an independent Python / numpy implementation that reads the mirror cfgs,
// the golden body table, Physics.cfg and the decompiled C#; tests/data/wp38_reference.json) or from the decompiled C# (the derivation is in
// the comment above the check).
//
// Fixtures that are not KSP data (STATUS.md "needs data"): the GameVariables contract factors (C# field initialisers, the Unity prefab is not in
// the repo), the two Career multiplier sets, the progress scenarios and generation GUIDs, facilities at the C# default (level 1.0: no
// ScenarioUpgradeableFacilities), no PQS attached (TerrainAltitude 0, GetHighestPeak 0), R&D absent (dockUnlocked false).
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
#include "ksp/fineprint/fineprint.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/unity/mathf.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::Contracts;
using Localizer38 = KSP::Localization::Localizer;
using json = nlohmann::json;
namespace L38 = ksp::Contracts::Lite;
namespace FP = ksp::FinePrint;

namespace {

const std::string kHarness38 = KSPCORE_HARNESS_DIR;

const json& Ref38() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp38_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

uint32_t Bits38(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
uint64_t BitsD38(double d) {
    uint64_t u;
    std::memcpy(&u, &d, 8);
    return u;
}
float F38(const json& j) { return j.get<float>(); }
// 0 ULP
#define CHECK_F32(got, ref) CHECK_MESSAGE(Bits38(static_cast<float>(got)) == Bits38(static_cast<float>(ref)), "got ", (got), " want ", (ref))
#define CHECK_F64(got, ref) CHECK_MESSAGE(BitsD38(static_cast<double>(got)) == BitsD38(static_cast<double>(ref)), "got ", (got), " want ", (ref))

// ULP distance of two finite doubles (for the measured-ULP statistics)
int64_t UlpD(double a, double b) {
    int64_t x, y;
    std::memcpy(&x, &a, 8);
    std::memcpy(&y, &b, 8);
    if (x < 0) x = INT64_MIN - x;
    if (y < 0) y = INT64_MIN - y;
    return x > y ? x - y : y - x;
}

std::string GameData38() { return ksp::testing::GameRoot() + "/GameData"; }

void LoadLocalizer38() {
    Localizer38::Shutdown();
    Localizer38::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(GameData38() + "/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    Localizer38::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

// The stock Kerbol system + a CAREER game + GameVariables + the story / agent / contract definitions (as World37 of test_contracts.cpp)
struct World38 {
    Game15 g;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    Game game;
    std::unique_ptr<GameVariables> gv;
    std::unique_ptr<Agents::AgentList> agents;
    std::unique_ptr<ProgressTracking> pt;
    Guid nextGuid;
    World38() {
        FlightRegistry::Clear();
        CareerEnvironment::Reset();
        ContractsEnvironment::Reset();
        ContractSystem::ClearTypes();
        ContractSystem::ContractWeights.reset();
        ScenarioUpgradeableFacilities::ClearStatics();
        SaveEnvironment& env = SaveEnvironment::Current();
        pg.physicsDatabaseFilename = kHarness38 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        game.Mode = Game::Modes::CAREER;
        game.flightState = std::make_shared<FlightState>();
        env.currentGame = &game;
        LoadLocalizer38();
        for (CelestialBody* b : sys->SpawnOrder()) b->bodyDisplayName = Localizer38::Format(b->bodyDisplayName);
        gv = std::make_unique<GameVariables>();
        gv->reputationAddition = AnimationCurve(std::vector<Keyframe>{Keyframe(0.0f, 0.9f)});
        gv->reputationSubtraction = AnimationCurve(std::vector<Keyframe>{Keyframe(0.0f, 1.3f)});
        ContractDefsLite::Load(GameData38() + "/Squad/Contracts/Contracts.cfg");
        ConfigNode::NodePtr story = ConfigNode::Load(GameData38() + "/Squad/Resources/StoryDefs.cfg");
        REQUIRE(story);
        ConfigNode::NodePtr sd = story->GetNode("STORY_DEF")->CreateCopy();
        Localizer38::TranslateBranch(*sd);
        TextGen::Reset();
        TextGen::Setup(*sd);
        agents = std::make_unique<Agents::AgentList>();
        ConfigNode::NodePtr ag = ConfigNode::Load(GameData38() + "/Squad/Agencies/Agents.cfg");
        REQUIRE(ag);
        agents->LoadAgents(ag->GetNodes("AGENT"));
        ContractSystem::RegisterStockTypes();
        FP::Install();
        pt = std::make_unique<ProgressTracking>();
        pt->Awake();
        pt->Start();
        Planetarium::fetch->time = 0.0;
        ContractsEnvironment::Current().newGuid = [this] { return nextGuid; };
        ContractsEnvironment::Current().nowTicks = [] { return int64_t(636000000000000000LL); };
        // CelestialBody.SetupConstants (CelestialBody.cs:384) ends with minOrbitalDistance = GetMinimumOrbitalDistance(this, 1f); kspcore keeps the serialized
        // value (WP-10), so the fixture applies it (Sun last: it reads the home body only)
        for (CelestialBody* b : sys->SpawnOrder()) b->minOrbitalDistance = L38::CelestialUtilities::GetMinimumOrbitalDistance(b, 1.0f);
    }
    ~World38() {
        agents.reset();
        pt->OnDestroy();
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
    CelestialBody& Body(const std::string& n) { return *sys->GetBodyByName(n); }
    GameParameters::CareerParams& Career() { return game.Parameters->Career; }
    void SetMult(const json& m) {
        Career().FundsGainMultiplier = F38(m["fundsGain"]);
        Career().FundsLossMultiplier = F38(m["fundsLoss"]);
        Career().RepGainMultiplier = F38(m["repGain"]);
        Career().RepLossMultiplier = F38(m["repLoss"]);
        Career().ScienceGainMultiplier = F38(m["sciGain"]);
    }
};

FP::Utilities::ProgressType TypeOf(const std::string& s) { return dotnet::EnumParseFull<FP::Utilities::ProgressType>(s == "NULL" ? std::string("NULL") : s); }

Guid GuidOf38(const json& bytes) {
    Guid g;
    for (size_t i = 0; i < 16; ++i) g.bytes[i] = static_cast<uint8_t>(bytes[i].get<int>());
    return g;
}

FP::OrbitType OrbitTypeOf(const std::string& s) {
    static const char* names[] = {"SYNCHRONOUS", "STATIONARY", "POLAR", "EQUATORIAL", "KOLNIYA", "TUNDRA", "RANDOM"};
    for (int i = 0; i < 7; ++i)
        if (s == names[i]) return static_cast<FP::OrbitType>(i);
    FAIL("orbit type " << s);
    return FP::OrbitType::RANDOM;
}

}  // namespace

// ---- KSPRandom ---------------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-38 KSPRandom draw sequences (Next, Next(max), Next(min,max), NextDouble) agree with the Python model") {
    int rows = 0;
    for (const json& r : Ref38()["krandom"]) {
        const int32_t seed = r["seed"].get<int32_t>();
        INFO("seed " << seed);
        KSPRandom k(seed);
        for (const json& v : r["next"]) CHECK(k.Next() == v.get<int32_t>());
        for (const json& v : r["nextMax100"]) CHECK(k.Next(100) == v.get<int32_t>());
        for (const json& v : r["nextRange"]) CHECK(k.Next(-5, 50) == v.get<int32_t>());
        for (const json& v : r["nextRangeTiny"]) CHECK(k.Next(3, 4) == v.get<int32_t>());
        for (const json& v : r["double"]) CHECK_F64(k.NextDouble(), v.get<double>());
        ++rows;
    }
    CHECK(rows == 8);
}

// ---- the fixture itself: body constants (Awake mass / gravParameter, Start sphere of influence, SetupConstants minOrbitalDistance) -------------------
TEST_CASE("WP-38 fixture bodies: Mass, gravParameter, sphereOfInfluence and GetMinimumOrbitalDistance vs the Python model") {
    World38 w;
    int rows = 0;
    for (const json& b : Ref38()["bodies"]) {
        CelestialBody& cb = w.Body(b["name"].get<std::string>());
        INFO(cb.name());
        CHECK_F64(cb.Mass, b["mass"].get<double>());
        CHECK_F64(cb.gravParameter, b["mu"].get<double>());
        if (b["soi"].is_null())
            CHECK(std::isinf(cb.sphereOfInfluence));
        else
            CHECK_F64(cb.sphereOfInfluence, b["soi"].get<double>());
        CHECK_F64(cb.minOrbitalDistance, b["minOrb"].get<double>());
        ++rows;
    }
    CHECK(rows == 17);
}

// ---- ProgressUtilities ---------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-38 ProgressUtilities: ScoreProgressType, OutlierWorldFirstContract, ProgressTypePrestige vs the reference (0 ULP)") {
    World38 w;
    int rows = 0;
    for (const json& r : Ref38()["scores"]) {
        const auto t = TypeOf(r["type"].get<std::string>());
        CelestialBody* b = r["body"].is_null() ? nullptr : &w.Body(r["body"].get<std::string>());
        INFO(r["type"] << " " << r["body"]);
        CHECK_F32(L38::ProgressUtilities::ScoreProgressType(t, b), F38(r["score"]));
        CHECK(L38::ProgressUtilities::OutlierWorldFirstContract(t, b) == r["outlier"].get<bool>());
        CHECK(static_cast<int>(L38::ProgressUtilities::ProgressTypePrestige(t, b)) == r["prestige"].get<int>());
        ++rows;
    }
    CHECK(rows == 29 * 7);
}

TEST_CASE("WP-38 ProgressUtilities.WorldFirstStandardReward (CONTRACT / PROGRESS x currency x type x body x 2 multiplier sets) vs the reference (0 ULP)") {
    World38 w;
    const json& ref = Ref38();
    int rows = 0;
    int cur = -1;
    for (const json& r : ref["worldFirst"]) {
        if (r["mult"].get<int>() != cur) {
            cur = r["mult"].get<int>();
            w.SetMult(ref["mult_sets"][static_cast<size_t>(cur)]);
        }
        const auto t = TypeOf(r["type"].get<std::string>());
        CelestialBody* b = r["body"].is_null() ? nullptr : &w.Body(r["body"].get<std::string>());
        const std::string cs = r["currency"].get<std::string>();
        const Currency c = cs == "Funds" ? Currency::Funds : (cs == "Science" ? Currency::Science : Currency::Reputation);
        const auto reward = r["reward"].get<std::string>() == "CONTRACT" ? FP::Utilities::ProgressRewardType::CONTRACT : FP::Utilities::ProgressRewardType::PROGRESS;
        INFO(r["type"] << " " << r["body"] << " " << r["reward"] << " " << cs << " mult " << cur);
        CHECK_F32(L38::ProgressUtilities::WorldFirstStandardReward(reward, c, t, b), F38(r["value"]));
        // the WP-36 seam installed by FinePrint::Install computes the same value
        CHECK_F32(CareerEnvironment::Current().worldFirstStandardReward(reward, c, t, b), F38(r["value"]));
        ++rows;
    }
    CHECK(rows == 2 * 28 * 7 * 2 * 3);
}

TEST_CASE("WP-38 ProgressUtilities.WorldFirstIntervalReward and FindNextRecord vs the reference (0 ULP)") {
    World38 w;
    const json& ref = Ref38();
    int cur = -1;
    int rows = 0;
    for (const json& r : ref["worldFirstInterval"]) {
        if (r["mult"].get<int>() != cur) {
            cur = r["mult"].get<int>();
            w.SetMult(ref["mult_sets"][static_cast<size_t>(cur)]);
        }
        const std::string cs = r["currency"].get<std::string>();
        const Currency c = cs == "Funds" ? Currency::Funds : (cs == "Science" ? Currency::Science : Currency::Reputation);
        INFO(r["type"] << " " << r["body"] << " " << cs << " " << r["cur"] << "/" << r["total"]);
        CHECK_F32(L38::ProgressUtilities::WorldFirstIntervalReward(FP::Utilities::ProgressRewardType::PROGRESS, c, TypeOf(r["type"].get<std::string>()),
                                                                   &w.Body(r["body"].get<std::string>()), r["cur"].get<int>(), r["total"].get<int>()),
                  F38(r["value"]));
        ++rows;
    }
    CHECK(rows == 2 * 5 * 2 * 3 * 7);
    for (const json& r : ref["records"]) {
        int interval = 0;
        INFO("record " << r["current"] << " " << r["max"] << " " << r["round"]);
        const double v = L38::ProgressUtilities::FindNextRecord(r["current"].get<double>(), r["max"].get<double>(), r["round"].get<double>(), interval);
        CHECK_F64(v, r["value"].get<double>());
        CHECK(interval == r["interval"].get<int>());
        int seamInterval = 0;
        CHECK_F64(CareerEnvironment::Current().findNextRecord(r["current"].get<double>(), r["max"].get<double>(), r["round"].get<double>(), seamInterval),
                  r["value"].get<double>());
    }
    CHECK(CareerEnvironment::Current().progressionRecordSplit() == 5);
    CHECK_F64(CareerEnvironment::Current().progressionMaxDepthRecord(), 750.0);
}

// ---- CelestialUtilities / SystemUtilities ------------------------------------------------------------------------------------------------------
TEST_CASE("WP-38 CelestialUtilities: synchronous / Kolniya SMA, CanBody*, ranking, time warp, LLAtoECEF, great circle vs the reference") {
    World38 w;
    const json& ref = Ref38();
    int64_t maxUlpGc = 0;
    int rows = 0;
    for (const json& r : ref["celestial"]) {
        CelestialBody* b = &w.Body(r["name"].get<std::string>());
        INFO(b->name());
        CHECK_F64(L38::CelestialUtilities::SynchronousSMA(b), r["syncSMA"].get<double>());
        CHECK_F64(L38::CelestialUtilities::KolniyaSMA(b), r["kolniyaSMA"].get<double>());
        CHECK(L38::CelestialUtilities::CanBodyBeSynchronous(b, 0.0) == r["canSync0"].get<bool>());
        CHECK(L38::CelestialUtilities::CanBodyBeSynchronous(b, 0.5) == r["canSync5"].get<bool>());
        CHECK(L38::CelestialUtilities::CanBodyBeKolniya(b) == r["canKol"].get<bool>());
        CHECK(L38::CelestialUtilities::CanBodyBeTundra(b) == r["canTundra"].get<bool>());
        CHECK_F32(L38::CelestialUtilities::PlanetScienceRanking(b), F38(r["ranking"]));
        CHECK_F64(L38::CelestialUtilities::GetMinimumOrbitalDistance(b, 1.0f), r["minOrb"].get<double>());
        const double alts[] = {-1.0, 0.0, 10000.0, 100000.0, 1e12};
        for (size_t i = 0; i < 5; ++i) CHECK_F64(L38::CelestialUtilities::GetNextTimeWarp(b, alts[i]), r["nextWarp"][i].get<double>());
        const Vector3f lla = L38::CelestialUtilities::LLAtoECEF(12.5, -77.25, 1500.0, b->Radius);
        CHECK_F32(lla.x, F38(r["lla"][0]));
        CHECK_F32(lla.y, F38(r["lla"][1]));
        CHECK_F32(lla.z, F38(r["lla"][2]));
        const double gc = L38::CelestialUtilities::GreatCircleDistance(b, 10.0, 20.0, -30.0, 100.0);
        maxUlpGc = std::max(maxUlpGc, UlpD(gc, r["gc"].get<double>()));
        ++rows;
    }
    CHECK(rows == 17);
    // GetRelSurfaceNVector is the WP-10 quaternion chain; the reference restates it from the C# in plain doubles (acos amplifies the last bits)
    MESSAGE("great circle max ULP vs the Python model: " << maxUlpGc);
    CHECK(maxUlpGc == 0);
    CHECK_F64(L38::CelestialUtilities::GetSolarExtents(), ref["solarExtents"].get<double>());
    for (const json& r : ref["deviation"]) {
        CHECK(L38::SystemUtilities::WithinDeviation(r["v1"].get<double>(), r["v2"].get<double>(), r["dev"].get<double>()) == r["within"].get<bool>());
        CHECK_F64(L38::SystemUtilities::MeasureDeviation(r["v1"].get<double>(), r["v2"].get<double>()), r["measure"].get<double>());
    }
    // structure helpers (derived from the C#): ChildrenOf / GetHostPlanet / GetPlanetarySystem / GetNeighbors
    CHECK(L38::CelestialUtilities::GetHostPlanet(&w.Body("Mun")) == &w.Body("Kerbin"));
    CHECK(L38::CelestialUtilities::GetHostPlanet(&w.Body("Kerbin")) == &w.Body("Kerbin"));
    CHECK(L38::CelestialUtilities::GetHostPlanet(nullptr) == w.planetarium.Sun);
    CHECK(L38::CelestialUtilities::GetPlanetarySystem(&w.Body("Kerbin")).size() == 2);
    CHECK(L38::CelestialUtilities::GetNeighbors(&w.Body("Kerbin")).size() == 3);  // Sun + Mun + Minmus
    CHECK(L38::CelestialUtilities::IsGasGiant(&w.Body("Jool")));
    CHECK(L38::CelestialUtilities::IsFlyablePlanet(&w.Body("Laythe")));
    CHECK_FALSE(L38::CelestialUtilities::IsFlyablePlanet(&w.Body("Mun")));
    CHECK(L38::StringUtilities::PossessiveString("Kerbals") == "Kerbals'");
    CHECK(L38::StringUtilities::PossessiveString("Jeb") == "Jeb's");
    CHECK(L38::StringUtilities::PossessiveString("Mech") == "Mech'");
}

// ---- OrbitUtilities.GenerateOrbit ----------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-38 OrbitUtilities.GenerateOrbit: the generated elements follow the KSPRandom draw sequence (Python model)") {
    World38 w;
    int rows = 0;
    double maxRelSma = 0.0, maxRelEcc = 0.0, maxAbsInc = 0.0;
    std::string worst;
    for (const json& r : Ref38()["orbits"]) {
        CelestialBody& b = w.Body(r["body"].get<std::string>());
        const FP::OrbitType ot = OrbitTypeOf(r["type"].get<std::string>());
        INFO(b.name() << " " << r["type"] << " seed " << r["seed"] << " alt " << r["alt"]);
        const Orbit o = FP::Utilities::OrbitUtilities::GenerateOrbit(r["seed"].get<int>(), &b, ot, r["alt"].get<double>(), r["inc"].get<double>(), r["ecc"].get<double>());
        const json& el = r["el"];
        CHECK(o.referenceBody == &b);
        // GenerateFinalization round-trips the elements through state vectors (Orbit.UpdateFromStateVectors), which changes the last bits: the elements
        // are compared with a relative tolerance (they are the draw-level values of the reference, not a golden of the final Orbit)
        const double sma = el["sma"].get<double>();
        const double ecc = el["ecc"].get<double>();
        maxRelSma = std::max(maxRelSma, std::fabs(o.semiMajorAxis - sma) / std::fabs(sma));
        const double relEcc = std::fabs(std::fabs(o.eccentricity) - std::fabs(ecc)) / std::max(1e-3, std::fabs(ecc));  // |ecc|: a negative raw ecc (Sun TUNDRA: min distance > 2 x sma) is mirrored by the state vector round trip
        if (relEcc > maxRelEcc) {
            maxRelEcc = relEcc;
            worst = b.name() + " " + r["type"].get<std::string>() + " seed " + std::to_string(r["seed"].get<int>()) + " alt " + std::to_string(r["alt"].get<double>()) +
                    " got ecc " + std::to_string(o.eccentricity) + " want " + std::to_string(ecc);
        }
        maxAbsInc = std::max(maxAbsInc, std::fabs(o.inclination - el["inc"].get<double>()));
        ++rows;
    }
    CHECK(rows == 7 * 7 * 4 * 3);
    MESSAGE("GenerateOrbit vs draw-level elements: max rel sma " << maxRelSma << ", max rel ecc " << maxRelEcc << ", max abs inclination " << maxAbsInc << " deg; worst ecc: " << worst);
    CHECK(maxRelSma < 1e-13);
    CHECK(maxRelEcc < 1e-11);
    CHECK(maxAbsInc < 1e-12);
}

TEST_CASE("WP-38 OrbitUtilities.IdentifyOrbit classifies the generated deterministic types; ValidateOrbit repairs NaN elements") {
    World38 w;
    CelestialBody& kerbin = w.Body("Kerbin");
    const double sig = ContractDefsLite::F("Satellite", "", "SignificantDeviation");
    CHECK(sig == 5.0);
    for (int seed : {3, 4, 5}) {
        Orbit o = FP::Utilities::OrbitUtilities::GenerateOrbit(seed, &kerbin, FP::OrbitType::STATIONARY, 0.4, 0.4);
        CHECK(FP::Utilities::OrbitUtilities::IdentifyOrbit(o) == FP::OrbitType::STATIONARY);
        Orbit s = FP::Utilities::OrbitUtilities::GenerateOrbit(seed, &kerbin, FP::OrbitType::SYNCHRONOUS, 0.4, 0.4);
        CHECK(FP::Utilities::OrbitUtilities::IdentifyOrbit(s) == FP::OrbitType::SYNCHRONOUS);
    }
    Orbit good = FP::Utilities::OrbitUtilities::GenerateOrbit(11, &kerbin, FP::OrbitType::RANDOM, 0.4, 0.4);
    Orbit copy = good;
    CHECK(FP::Utilities::OrbitUtilities::ValidateOrbit(11, copy, FP::OrbitType::RANDOM, 0.4, 0.4, "test"));
    Orbit bad = good;
    bad.eccentricity = std::nan("");
    bad.LAN = std::numeric_limits<double>::infinity();
    CHECK_FALSE(FP::Utilities::OrbitUtilities::ValidateOrbit(11, bad, FP::OrbitType::RANDOM, 0.4, 0.4, "test"));
    CHECK(std::isfinite(bad.eccentricity));
    CHECK(std::isfinite(bad.LAN));
}

// ---- WaypointManager.ChooseRandomPosition* and Waypoint.Randomize* ---------------------------------------------------------------------------------
TEST_CASE("WP-38 WaypointManager.ChooseRandomPosition / ChooseRandomPositionNear vs the reference (0 ULP)") {
    World38 w;
    int rows = 0;
    for (const json& r : Ref38()["positions"]) {
        KSPRandom g(r["seed"].get<int>());
        double lat = 0.0, lon = 0.0;
        INFO(r["kind"] << " " << r["body"] << " seed " << r["seed"] << " water " << r["water"]);
        if (r["kind"].get<std::string>() == "random") {
            FP::WaypointManager::ChooseRandomPosition(lat, lon, r["body"].get<std::string>(), r["water"].get<bool>(), r["equatorial"].get<bool>(), &g);
        } else {
            FP::WaypointManager::ChooseRandomPositionNear(lat, lon, r["clat"].get<double>(), r["clon"].get<double>(), r["body"].get<std::string>(), r["radius"].get<double>(),
                                                          r["water"].get<bool>(), &g);
        }
        CHECK_F64(lat, r["lat"].get<double>());
        CHECK_F64(lon, r["lon"].get<double>());
        ++rows;
    }
    CHECK(rows == 5 * 3 * 2 * 3);
    // an unknown body leaves the position at 0
    KSPRandom g(1);
    double lat = 5.0, lon = 5.0;
    FP::WaypointManager::ChooseRandomPosition(lat, lon, "NoSuchBody", true, false, &g);
    CHECK(lat == 0.0);
    CHECK(lon == 0.0);
}

TEST_CASE("WP-38 Waypoint.RandomizeNear / RandomizeAwayFrom vs the reference") {
    World38 w;
    int rows = 0;
    int64_t maxUlp = 0;
    for (const json& r : Ref38()["waypoints"]) {
        FP::Waypoint wp;
        wp.celestialName = r["body"].get<std::string>();
        wp.latitude = -3.0;
        wp.longitude = 40.0;
        KSPRandom g(r["seed"].get<int>());
        INFO(r["kind"] << " " << r["body"] << " seed " << r["seed"] << " water " << r["water"]);
        if (r["kind"].get<std::string>() == "near") {
            wp.RandomizeNear(-3.0, 40.0, 100.0, 5000.0, r["water"].get<bool>(), &g);
            CHECK(wp.landLocked == r["landLocked"].get<bool>());
        } else {
            wp.RandomizeAwayFrom(10.0, 20.0, 50.0, 8000.0, 3, r["water"].get<bool>(), &g);
        }
        maxUlp = std::max({maxUlp, UlpD(wp.latitude, r["lat"].get<double>()), UlpD(wp.longitude, r["lon"].get<double>())});
        if (r["kind"].get<std::string>() == "near") {
            CHECK_F64(wp.latitude, r["lat"].get<double>());
            CHECK_F64(wp.longitude, r["lon"].get<double>());
        }
        ++rows;
    }
    CHECK(rows == 4 * 3 * 2 * 2);
    MESSAGE("Waypoint.Randomize* max ULP vs the Python model (away-from includes the quaternion great circle): " << maxUlp);
    CHECK(maxUlp == 0);
    // uniqueSeed: (index + seed) * (index + seed + 1) / 2 + seed
    FP::Waypoint a;
    a.seed = 7;
    a.index = 3;
    CHECK(a.uniqueSeed() == (3 + 7) * (3 + 7 + 1) / 2 + 7);
    FP::Waypoint copy(a);
    CHECK(copy.seed == 7);
    CHECK(copy.name == "Site");
    CHECK(copy.id == "default");
    CHECK(copy.navigationId == a.navigationId);
}

// ---- ProgressTrackingParameter -------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-38 ProgressTrackingParameter: titles, notes and messages for every ProgressType (tag tables parsed from the C# by the reference)") {
    World38 w;
    int rows = 0;
    for (const json& r : Ref38()["ptpTexts"]) {
        Parameters::ProgressTrackingParameter p(L38::ProgressMilestone(&w.Body(r["body"].get<std::string>()), TypeOf(r["type"].get<std::string>())));
        INFO(r["type"] << " " << r["body"]);
        CHECK(p.IsTutorial() == r["tutorial"].get<bool>());
        CHECK(p.GetTitle() == r["title"].get<std::string>());
        CHECK(p.InstructionalNote() == r["instr"].get<std::string>());
        CHECK(p.FlavorNote() == r["flavor"].get<std::string>());
        if (!r["msg"].is_null()) CHECK(p.GetMessageComplete() == Localizer38::Format(r["msg"].get<std::string>()));
        ++rows;
    }
    CHECK(rows == 28 * 4);
}

// ---- ExplorationContract.Generate ----------------------------------------------------------------------------------------------------------------
namespace {

void LoadScenario38(World38& w, const json& scenario) {
    ConfigNode scen("SCENARIO");
    ConfigNode* prog = scen.AddNode("Progress");
    for (auto it = scenario["top"].begin(); it != scenario["top"].end(); ++it) prog->AddNode(it.key())->AddValue(it.value().get<std::string>(), 0.0);
    for (auto it = scenario["bodies"].begin(); it != scenario["bodies"].end(); ++it) {
        ConfigNode* b = prog->AddNode(it.key());
        for (auto jt = it.value().begin(); jt != it.value().end(); ++jt) b->AddNode(jt.key())->AddValue(jt.value().get<std::string>(), 0.0);
    }
    w.pt->OnLoad(scen);
}

ContractPrestige PrestigeOf38(const std::string& s) {
    if (s == "Trivial") return ContractPrestige::Trivial;
    if (s == "Significant") return ContractPrestige::Significant;
    return ContractPrestige::Exceptional;
}

}  // namespace

TEST_CASE("WP-38 ExplorationContract: generated contracts (milestones, tree, texts, rewards, agent, id) vs the reference (0 ULP, byte-exact)") {
    const json& ref = Ref38();
    int generated = 0, nulls = 0, tutorials = 0, multi = 0;
    for (size_t si = 0; si < ref["scenarios"].size(); ++si) {
        World38 w;
        ContractSystem cs;
        cs.Awake();
        ConfigNode weights("SCENARIO");
        weights.AddNode("WEIGHTS");
        cs.LoadNow(weights);
        LoadScenario38(w, ref["scenarios"][si]);
        for (const json& e : ref["exploration"]) {
            if (e["scenario"].get<size_t>() != si) continue;
            w.SetMult(ref["mult_sets"][e["mult"].get<size_t>()]);
            w.nextGuid = GuidOf38(e["guid"]);
            INFO("scenario " << ref["scenarios"][si]["name"] << " seed " << e["seed"] << " mult " << e["mult"]);
            std::unique_ptr<Contract> c = cs.GenerateContract(e["seed"].get<int>(), ContractPrestige::Trivial, "ExplorationContract");
            const json& r = e["result"];
            if (r.is_null()) {
                std::string d;
                if (c != nullptr) {
                    d = c->Title();
                    for (int i = 0; i < c->ParameterCount(); ++i)
                        if (auto* pp = dynamic_cast<Parameters::ProgressTrackingParameter*>(c->GetParameter(i)))
                            d += " | " + pp->milestone.body->name() + " " + dotnet::EnumToString(pp->milestone.type);
                }
                INFO("unexpected contract: " << d);
                CHECK(c == nullptr);
                ++nulls;
                continue;
            }
            REQUIRE(c != nullptr);
            ++generated;
            auto* ex = dynamic_cast<FP::Contracts::ExplorationContract*>(c.get());
            REQUIRE(ex != nullptr);
            CHECK(c->Agent()->Name() == r["agent"].get<std::string>());
            CHECK(c->Agent()->Title() == r["agentTitle"].get<std::string>());
            CHECK(c->KeywordsRequired() == r["keywordsRequired"].get<std::vector<std::string>>());
            CHECK(c->Title() == r["title"].get<std::string>());
            CHECK(c->Synopsys() == r["synopsis"].get<std::string>());
            CHECK(c->Description() == r["description"].get<std::string>());
            CHECK(c->ContractID() == r["contractID"].get<long long>());
            CHECK(dotnet::EnumToString(c->Prestige()) == r["prestige"].get<std::string>());
            CHECK(ex->TargetBody()->name() == r["targetBody"].get<std::string>());
            CHECK_F32(ex->BodyRatio(), F38(r["bodyRatio"]));
            CHECK(ex->IsTutorial() == r["tutorial"].get<bool>());
            CHECK_F64(c->FundsAdvance, r["fa"].get<double>());
            CHECK_F64(c->FundsCompletion, r["fc"].get<double>());
            CHECK_F64(c->FundsFailure, r["ff"].get<double>());
            CHECK_F32(c->ReputationCompletion, F38(r["rc"]));
            CHECK_F32(c->ReputationFailure, F38(r["rf"]));
            CHECK_F32(c->ScienceCompletion, F38(r["sc"]));
            CHECK_F64(c->TimeExpiry, r["expiry"].get<double>());
            CHECK_F64(c->TimeDeadline, r["deadline"].get<double>());
            CHECK(c->ContractState() == Contract::State::Generated);
            CHECK_FALSE(c->CanBeCancelled());
            CHECK_FALSE(c->CanBeDeclined());
            CHECK_FALSE(c->CanBeFailed());
            REQUIRE(c->ParameterCount() == static_cast<int>(r["params"].size()));
            if (r["tutorial"].get<bool>()) ++tutorials;
            if (c->ParameterCount() > 1) ++multi;
            for (int i = 0; i < c->ParameterCount(); ++i) {
                auto* p = dynamic_cast<Parameters::ProgressTrackingParameter*>(c->GetParameter(i));
                REQUIRE(p != nullptr);
                const json& pr = r["params"][static_cast<size_t>(i)];
                CHECK(p->milestone.body->name() == pr["body"].get<std::string>());
                CHECK(dotnet::EnumToString(p->milestone.type) == pr["type"].get<std::string>());
                CHECK(p->HashString() == pr["hash"].get<std::string>());
                CHECK(p->Title() == pr["title"].get<std::string>());
                CHECK(p->InstructionalNote() == pr["instr"].get<std::string>());
                CHECK(p->FlavorNote() == pr["flavor"].get<std::string>());
                CHECK_F64(p->FundsCompletion, pr["fc"].get<double>());
                CHECK_F32(p->ScienceCompletion, F38(pr["sc"]));
                CHECK_F32(p->ReputationCompletion, F38(pr["rc"]));
            }
            CHECK(c->MissionSeed() == e["seed"].get<int>());
        }
    }
    CHECK(generated + nulls == 540);
    CHECK(generated == 480);
    CHECK(tutorials > 0);
    CHECK(multi > 0);
}

TEST_CASE("WP-38 ExplorationContract: node round trip, MeetRequirements and the ContractSystem registration") {
    World38 w;
    ContractSystem cs;
    cs.Awake();
    ConfigNode weights("SCENARIO");
    weights.AddNode("WEIGHTS");
    cs.LoadNow(weights);
    CHECK(ContractSystem::HasContractType("ExplorationContract"));
    LoadScenario38(w, Ref38()["scenarios"][6]);
    w.nextGuid = GuidOf38(Ref38()["exploration"][0]["guid"]);
    std::unique_ptr<Contract> c = cs.GenerateContract(6018 * 2 + 11, ContractPrestige::Trivial, "ExplorationContract");
    REQUIRE(c != nullptr);
    CHECK(c->MeetRequirements());
    ConfigNode node("CONTRACT");
    c->Save(node);
    CHECK(node.GetValue("type") == std::optional<std::string>("ExplorationContract"));
    ConfigNode::NodePtr copyPtr = node.CreateCopy();
    ConfigNode& copy = *copyPtr;
    std::unique_ptr<Contract> loaded = ContractSystem::CreateContract("ExplorationContract");
    REQUIRE(loaded != nullptr);
    REQUIRE(Contract::Load(*loaded, copy));
    auto* a = dynamic_cast<FP::Contracts::ExplorationContract*>(c.get());
    auto* b = dynamic_cast<FP::Contracts::ExplorationContract*>(loaded.get());
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    CHECK(b->TargetBody() == a->TargetBody());
    CHECK_F32(b->BodyRatio(), a->BodyRatio());
    CHECK(b->ParameterCount() == a->ParameterCount());
    CHECK(b->Title() == a->Title());
    for (int i = 0; i < a->ParameterCount(); ++i) {
        auto* pa = dynamic_cast<Parameters::ProgressTrackingParameter*>(a->GetParameter(i));
        auto* pb = dynamic_cast<Parameters::ProgressTrackingParameter*>(b->GetParameter(i));
        REQUIRE(pa != nullptr);
        REQUIRE(pb != nullptr);
        CHECK(pb->milestone.body == pa->milestone.body);
        CHECK(pb->milestone.type == pa->milestone.type);
    }
}
