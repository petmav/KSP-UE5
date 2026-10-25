// WP-38d unit tests: CometDefinition / CometOrbitType / CometManager / DiscoverableObjectsUtil / ScenarioDiscoverableObjects (core/include/ksp/fineprint/space_objects.hpp)
// and the Sentinel seams they answer.
//
// Expectations come from tools/wp38d/space_objects_reference.py (tests/data/wp38d_reference.json): an independent Python model that reads the shipped
// CometDefs.cfg of the GameData mirror, the serialized body table and the decompiled C#, and does not import kspcore. The Unity Random state after each call is part of
// the comparison: it proves the draw order and count. The orbit anomalies are compared with a 1e-9 bound (host libm in the reference, the game's runtime in kspcore).
// Fixtures that are not KSP data: the Kerbal name tables of CrewGenerator and the comet name dictionary states (SYNTHETIC, the text assets are not in the repo), the
// Sentinel orbit fields of the sentinel-orbit table (SYNTHETIC), the fixture vessels of the behaviour cases.
#include "world38b.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/fineprint/sentinel.hpp"
#include "ksp/fineprint/space_objects.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/vessel/vessel.hpp"

namespace SM = ksp::SentinelMission;

namespace {

const json& Ref38d() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp38d_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

// the reference encodes non-finite doubles as "nan" / "inf" / "-inf"
double Dj(const json& j) {
    if (j.is_string()) {
        const std::string s = j.get<std::string>();
        if (s == "nan") return std::numeric_limits<double>::quiet_NaN();
        if (s == "inf") return std::numeric_limits<double>::infinity();
        return -std::numeric_limits<double>::infinity();
    }
    return j.get<double>();
}
bool SameD(double got, const json& want) {
    const double w = Dj(want);
    if (std::isnan(w)) return std::isnan(got);
    return BitsD38b(got) == BitsD38b(w);
}
bool NearD(double got, const json& want, double abs) {
    const double w = Dj(want);
    if (std::isnan(w)) return std::isnan(got);
    if (std::isinf(w)) return got == w;
    return std::fabs(got - w) <= abs;
}
bool SameF(float got, const json& want) { return Bits38b(got) == Bits38b(want.get<float>()); }
#define CHECK_SD(got, want) CHECK_MESSAGE(SameD((got), (want)), #got, " got ", (got), " want ", (want).dump())
#define CHECK_SF(got, want) CHECK_MESSAGE(SameF((got), (want)), #got, " got ", (got), " want ", (want).dump())

bool StateIs(const json& want) {
    const UnityRandom::State s = UnityRandom::Global().state();
    return s.s0 == want[0].get<uint32_t>() && s.s1 == want[1].get<uint32_t>() && s.s2 == want[2].get<uint32_t>() && s.s3 == want[3].get<uint32_t>();
}
#define CHECK_STATE(want) CHECK_MESSAGE(StateIs(want), "Unity Random state after the call differs: want ", (want).dump())

std::string CometDefsPath() { return GameData38b() + "/Squad/Resources/CometDefs.cfg"; }

// A CometManager over the shipped CometDefs.cfg (the GameDatabase seam answers from the loaded file), the Kerbol system, a CAREER game and the part database
struct Space38d {
    World38b w;
    ConfigNode::NodePtr defs;
    std::unique_ptr<CometManager> mgr;
    int savedGeysers, savedDust;
    Space38d(bool manager = true) {
        defs = ConfigNode::Load(CometDefsPath());
        REQUIRE(defs);
        SpaceObjectsEnvironment::Reset();
        SpaceObjectsEnvironment::gameDatabaseConfigNodes = [this](const std::string& t) { return defs->GetNodes(t); };
        savedGeysers = GameSettings::COMET_MAXIMUM_GEYSERS;
        savedDust = GameSettings::COMET_MAXIMUM_NEAR_DUST_EMITTERS;
        GameSettings::COMET_MAXIMUM_GEYSERS = 50;  // GameSettings.SetDefaultValues (GameSettings.cs:1220)
        GameSettings::COMET_MAXIMUM_NEAR_DUST_EMITTERS = 50;
        if (manager) {
            mgr = std::make_unique<CometManager>();
            mgr->Awake();
            mgr->Start();
            REQUIRE(CometManager::Instance == mgr.get());
        }
        UnityRandom::Global().InitState(0);
    }
    ~Space38d() {
        mgr.reset();
        SpaceObjectsEnvironment::Reset();
        CrewGenerator::Reset();
        SM::SentinelEnvironment::Reset();
        FinePrint::Contracts::CometManagerSeam::discoveredComets = nullptr;
        GameSettings::COMET_MAXIMUM_GEYSERS = savedGeysers;
        GameSettings::COMET_MAXIMUM_NEAR_DUST_EMITTERS = savedDust;
        ScenarioDiscoverableObjects::Instance = nullptr;
        SM::SentinelScenario::Instance = nullptr;
    }
    std::shared_ptr<CometOrbitType> Type(const std::string& n) {
        auto t = CometManager::GetCometOrbitType(n);
        REQUIRE(t != nullptr);
        return t;
    }
};

}  // namespace

// ================================================================================================================================ CometManager data

TEST_CASE("WP-38d CometManager.LoadManagerDefinitions / LoadCometTypes read the shipped CometDefs.cfg") {
    Space38d s;
    const json& R = Ref38d();
    const json& m = R["manager"];
    CometManager& c = *s.mgr;
#define MF(f) CHECK_SF(c.f, m[#f])
#define MD(f) CHECK_SD(c.f, m[#f])
#define MI(f) CHECK_MESSAGE(c.f == m[#f].get<int>(), #f)
    MF(maxSpawnDistance); MF(minSpawnDistance);
    MD(maxVFXStartDistance); MD(minVFXStartDistance); MD(maxComaSizeRatio); MD(minComaSizeRatio); MD(maxTailWidthRatio); MD(maxIonTailWidthRatio);
    MD(maxTailLengthRatio); MD(minTailLengthRatio); MD(minGeyserRadiusRatio); MD(maxGeyserRadiusRatio); MD(minNearDustEmitterRadiusRatio);
    MD(maxNearDustEmitterRadiusRatio);
    MF(geyserVisibleDistance); MF(geyserSizeRadiusMultiplier); MF(nearDustVisibleRadiusMultiplier); MF(atmospherePressureForVFXFade);
    MF(gravityAdditionForVFXFadeStart); MF(gravityMultiplierForVFXFadeEnd); MF(gravitySubtractionForVFXFadeEnd); MF(vesselRangeOrbitLoadOverride);
    MF(vesselRangeOrbitUnloadOverride); MF(fragmentdynamicPressurekPa); MF(fragmentInstanceOffset); MF(fragmentAngleVariation); MF(fragmentMinPressureModifier);
    MF(fragmentMaxPressureModifier); MF(fragmentMinSafeTime); MF(fragmentMaxSafeTime);
    MI(minFragments); MI(maxFragments); MI(minFragmentClassStep); MI(maxFragmentClassStep); MI(fragmentIgnoreCollisions);
#undef MF
#undef MD
#undef MI
    // the keys the cfg never carries keep the C# initialisers (the sentinel spawn distances and the pack / unpack overrides are never loaded)
    CHECK_SF(c.minSentinelSpawnDistance, json(1.1f));
    CHECK_SF(c.maxSentinelSpawnDistance, json(2.0f));
    CHECK_SF(c.vesselRangeOrbitPackOverride, json(1100.0f));
    CHECK_SF(c.vesselRangeOrbitUnpackOverride, json(1000.0f));
    CHECK(c.sentinelAngle == 70.0f);
    // curves: the keys and 30+ samples each (float bits)
    std::map<std::string, FloatCurve*> curves{
        {"maxCometsPerUntrackedObjects", &c.maxCometsPerUntrackedObjects}, {"spawnChance", &c.spawnChance}, {"vfxStartDistribution", &c.vfxStartDistribution},
        {"comaSizeRatioDistribution", &c.comaSizeRatioDistribution}, {"tailWidthDistribution", &c.tailWidthDistribution},
        {"tailLengthRatioDistribution", &c.tailLengthRatioDistribution}, {"vfxSizeFromDistance", &c.vfxSizeFromDistance},
        {"nearDustVFXSizeFromDistance", &c.nearDustVFXSizeFromDistance}, {"geyserVFXSizeFromDistance", &c.geyserVFXSizeFromDistance},
        {"geysersRatioDistribution", &c.geysersRatioDistribution},
        {"atmospherePressureFadeDistributionForVFXFade", &c.atmospherePressureFadeDistributionForVFXFade}};
    REQUIRE(R["curves"].size() == curves.size());
    size_t samples = 0;
    for (auto& [name, curve] : curves) {
        const json& t = R["curves"][name];
        REQUIRE(curve->Curve().length() == static_cast<int>(t["keys"].size()));
        for (size_t i = 0; i < t["keys"].size(); ++i) {
            const Keyframe& k = curve->Curve().keys()[i];
            CHECK_SF(k.time, t["keys"][i][0]);
            CHECK_SF(k.value, t["keys"][i][1]);
            CHECK_SF(k.inTangent, t["keys"][i][2]);
            CHECK_SF(k.outTangent, t["keys"][i][3]);
        }
        for (const json& smp : t["samples"]) {
            ++samples;
            const float y = curve->Evaluate(smp[0].get<float>());
            CHECK_MESSAGE(SameF(y, smp[1]), name, " at ", smp[0].dump(), " got ", y, " want ", smp[1].dump());
        }
    }
    CHECK(samples > 400);
    // the four orbit types
    REQUIRE(c.cometDefinitions.size() == R["types"].size());
    for (size_t i = 0; i < c.cometDefinitions.size(); ++i) {
        const CometOrbitType& t = *c.cometDefinitions[i];
        const json& r = R["types"][i];
        CHECK(t.name == r["name"].get<std::string>());
        CHECK(t.displayName == r["displayName"].get<std::string>());
        CHECK(t.description == r["description"].get<std::string>());
        CHECK_SF(t.inclination1Min, r["inclination1Min"]);
        CHECK_SF(t.inclination1Max, r["inclination1Max"]);
        CHECK_SF(t.inclination2Min, r["inclination2Min"]);
        CHECK_SF(t.inclination2Max, r["inclination2Max"]);  // the cfg spells inclination2MAx: never read, stays 0
        CHECK_SF(t.inclination1Chance, r["inclination1Chance"]);
        CHECK_SF(t.peMin, r["peMin"]);
        CHECK_SF(t.peMax, r["peMax"]);
        CHECK_SF(t.apMin, r["apMin"]);
        CHECK_SF(t.apMax, r["apMax"]);
        CHECK(t.useEccentricity == r["useEccentricity"].get<bool>());
        CHECK_SF(t.eccentricityMin, r["eccentricityMin"]);
        CHECK_SF(t.eccentricityMax, r["eccentricityMax"]);
        CHECK(static_cast<int>(t.minCometClass) == r["minCometClass"].get<int>());
        CHECK(static_cast<int>(t.maxCometClass) == r["maxCometClass"].get<int>());
        CHECK(t.chanceWeight == r["chanceWeight"].get<int>());
    }
    // the tail colour lists (the shipped cfg closes each weightedColor with "},")
    REQUIRE(c.ionTailColors.size() == R["ionTailColors"].size());
    for (size_t i = 0; i < c.ionTailColors.size(); ++i) {
        const json& r = R["ionTailColors"][i];
        CHECK_SF(c.ionTailColors[i].emitterColor.r, r["color"][0]);
        CHECK_SF(c.ionTailColors[i].emitterColor.g, r["color"][1]);
        CHECK_SF(c.ionTailColors[i].emitterColor.b, r["color"][2]);
        CHECK_SF(c.ionTailColors[i].emitterColor.a, r["color"][3]);
        CHECK(c.ionTailColors[i].chanceWeight == r["weight"].get<int>());
    }
    REQUIRE(c.dustTailColors.size() == R["dustTailColors"].size());
    CHECK_SF(c.dustTailColors[0].emitterColor.g, R["dustTailColors"][0]["color"][1]);
    CHECK_SF(c.ionTailColorP67.b, R["ionTailColorP67"][2]);
    for (const json& r : R["tailColorPick"]) {
        const int seed = r["seed"].get<int>();
        const Color ion = CometManager::GenerateWeightedIonTailColor(seed);
        const Color dust = CometManager::GenerateWeightedDustTailColor(seed);
        CHECK_SF(ion.r, r["ion"][0]);
        CHECK_SF(ion.g, r["ion"][1]);
        CHECK_SF(ion.b, r["ion"][2]);
        CHECK_SF(ion.a, r["ion"][3]);
        CHECK_SF(dust.r, r["dust"][0]);
        CHECK_SF(dust.g, r["dust"][1]);
        CHECK_SF(dust.b, r["dust"][2]);
        CHECK_SF(dust.a, r["dust"][3]);
    }
    // the C# initialisers when the game database has no COMET_MANAGER node
    {
        CometManager d;
        SpaceObjectsEnvironment::gameDatabaseConfigNodes = [](const std::string&) { return std::vector<ConfigNode*>(); };
        // (a second manager while the first is the Instance destroys itself: Awake is not called, the field initialisers are the defaults)
        d.LoadManagerDefinitions();
        CHECK(d.maxSpawnDistance == 2.0f);
        CHECK(d.minVFXStartDistance == 0.8);
        CHECK(d.maxFragments == 5);
        CHECK(d.cometDefinitions.empty());
        CHECK(d.maxCometsPerUntrackedObjects.Curve().length() == 2);
    }
}

TEST_CASE("WP-38d CometManager: spawn chance, ShouldSpawnComet, weighted comet type") {
    Space38d s;
    const json& R = Ref38d();
    CometManager& c = *s.mgr;
    ScenarioDiscoverableObjects sdo;
    SM::SentinelScenario sentinel;
    SM::SentinelScenario* const savedSentinel = SM::SentinelScenario::Instance;
    SM::SentinelUtilities::perSentinelObjectLimit = 10;
    auto Arrange = [&](const json& groupMax, const json& sentinelMax, int comets) {
        ScenarioDiscoverableObjects::Instance = groupMax.is_null() ? nullptr : &sdo;
        if (!groupMax.is_null()) sdo.spawnGroupMaxLimit = groupMax.get<int>();
        SM::SentinelScenario::Instance = sentinelMax.is_null() ? nullptr : &sentinel;
        sentinel.deployedSentinels.assign(sentinelMax.is_null() ? 0 : static_cast<size_t>(sentinelMax.get<int>() / 10), nullptr);
        c.Comets().assign(static_cast<size_t>(comets), nullptr);
    };
    size_t n = 0;
    for (const json& r : R["spawnChance"]) {
        Arrange(r["groupMax"], r["sentinelMax"], r["comets"].get<int>());
        const float chance = c.GetSpawnChance();
        CHECK_SF(c.cometCap, r["cap"]);
        CHECK_SF(chance, r["chance"]);
        ++n;
    }
    CHECK(n == 120);
    for (const json& r : R["shouldSpawn"]) {
        Arrange(r["groupMax"], r["sentinelMax"], r["comets"].get<int>());
        UnityRandom::Global().InitState(r["seed"].get<int>());
        const bool result = CometManager::ShouldSpawnComet();
        CHECK(result == r["result"].get<bool>());
        CHECK_SF(c.cometCap, r["cap"]);
        CHECK_STATE(r["state"]);
    }
    ScenarioDiscoverableObjects::Instance = nullptr;
    SM::SentinelScenario::Instance = savedSentinel;
    c.Comets().clear();
    for (const json& r : R["weightedType"]) {
        UnityRandom::Global().InitState(r["seed"].get<int>());
        std::shared_ptr<CometOrbitType> t = CometManager::GenerateWeightedCometType();
        REQUIRE(t != nullptr);
        CHECK(t->name == r["name"].get<std::string>());
        CHECK_STATE(r["state"]);
    }
    // no Instance: no comets, no draws
    UnityRandom::Global().InitState(5);
    const UnityRandom::State before = UnityRandom::Global().state();
    CometManager* const inst = CometManager::Instance;
    CometManager::Instance = nullptr;
    CHECK_FALSE(CometManager::ShouldSpawnComet());
    CHECK(CometManager::GenerateWeightedCometType() == nullptr);
    CHECK(CometManager::GetCometOrbitType("short") == nullptr);
    CometManager::Instance = inst;
    CHECK(UnityRandom::Global().state().s3 == before.s3);
    CHECK(CometManager::GetCometOrbitType("long") != nullptr);
    CHECK(CometManager::GetCometOrbitType("nope") == nullptr);
    sdo.OnDestroy();
}

TEST_CASE("WP-38d CometManager: spawn distances, class radius, GenerateDefinition") {
    Space38d s;
    const json& R = Ref38d();
    CHECK_SD(BodyOrbit(FlightGlobals::GetHomeBody())->semiMajorAxis, R["provenance"]["homeSma"]);
    for (const json& r : R["spawnDistance"]) {
        UnityRandom::Global().InitState(r["seedHome"].get<int>());
        CHECK_SD(CometManager::GenerateHomeSpawnDistance(), r["home"]);
        CHECK_SD(CometManager::GenerateSentinelSpawnDistance(r["sentinelInput"].get<double>()), r["sentinel"]);
        // without an Instance: the defaults (FlightGlobals.ready is false: the 13.6e9 default sma)
        CometManager* const inst = CometManager::Instance;
        CometManager::Instance = nullptr;
        UnityRandom::Global().InitState(r["seedNoInstance"].get<int>());
        CHECK_SD(CometManager::GenerateHomeSpawnDistance(), r["homeNoInstance"]);
        CHECK_SD(CometManager::GenerateSentinelSpawnDistance(r["sentinelInput"].get<double>()), r["sentinelNoInstance"]);
        CHECK_STATE(r["state"]);
        CometManager::Instance = inst;
    }
    for (const json& r : R["classRadius"]) {
        const float radius = DiscoveryInfo::GetClassRadius(static_cast<UntrackedObjectClass>(r["class"].get<int>()), r["seed"].get<int>());
        CHECK_SF(radius, r["radius"]);
        CHECK_STATE(r["state"]);
    }
    size_t n = 0;
    for (const json& r : R["definitions"]) {
        GameSettings::COMET_MAXIMUM_GEYSERS = r["maxGeysers"].get<int>();
        GameSettings::COMET_MAXIMUM_NEAR_DUST_EMITTERS = r["maxDust"].get<int>();
        std::shared_ptr<CometOrbitType> t = s.Type(r["typeName"].get<std::string>());
        CometDefinition d = CometManager::GenerateDefinition(*t, static_cast<UntrackedObjectClass>(r["class"].get<int>()), r["seed"].get<int>());
        CHECK(d.typeName == r["typeName"].get<std::string>());
        CHECK(d.cometType == t.get());
        CHECK(d.seed == r["seed"].get<int>());
        CHECK_SF(d.radius, r["radius"]);
        CHECK_SD(d.vfxStartDistance, r["vfxStartDistance"]);
        CHECK_SD(d.comaWidthRatio, r["comaWidthRatio"]);
        CHECK_SD(d.tailWidthRatio, r["tailWidthRatio"]);
        CHECK_SD(d.tailMaxLengthRatio, r["tailMaxLengthRatio"]);
        CHECK_SD(d.ionTailWidthRatio, r["ionTailWidthRatio"]);
        CHECK(d.numGeysers == r["numGeysers"].get<int>());
        CHECK(d.numNearDustEmitters == r["numNearDustEmitters"].get<int>());
        ++n;
    }
    CHECK(n == 4 * 9 * 5 * 3);
    // CreateVesselNode: the key order and the values the C# writes
    GameSettings::COMET_MAXIMUM_GEYSERS = 50;
    CometDefinition d = CometManager::GenerateDefinition(*s.Type("long"), UntrackedObjectClass::G, 1234);
    ConfigNode::NodePtr node = d.CreateVesselNode(true, 2.5f, true);
    CHECK(node->name == "CometVessel");
    std::vector<std::string> keys;
    for (int i = 0; i < node->values().Count(); ++i) keys.push_back(node->values()[i].name);
    CHECK(keys == std::vector<std::string>{"typeName", "vfxStartDistance", "comaWidthRatio", "tailWidthRatio", "ionTailWidthRatio", "tailMaxLengthRatio", "seed", "radius",
                                            "numGeysers", "numNearDustEmitters", "optimizedCollider", "fragmentDynamicPressureModifier", "hasName"});
    CHECK(node->GetValue("typeName") == "long");
    CHECK(node->GetValue("optimizedCollider") == "True");
    CHECK(node->GetValue("hasName") == "True");
    CHECK(d.CreateVesselNode(false, 0.0f, false)->HasValue("hasName") == false);
    // GenerateDefinition without an Instance: the C# builds the generator first and then dereferences Instance
    std::shared_ptr<CometOrbitType> longType = s.Type("long");
    CometManager* const inst = CometManager::Instance;
    CometManager::Instance = nullptr;
    CHECK_THROWS_AS(CometManager::GenerateDefinition(*longType, UntrackedObjectClass::G, 1), NullReferenceException);
    CometManager::Instance = inst;
}

TEST_CASE("WP-38d CometOrbitType: CalculateHomeOrbit and CalculateSentinelOrbit against the draw-level reference") {
    Space38d s;
    const json& R = Ref38d();
    auto CheckOrbit = [&](const Orbit& o, const json& r, bool home) {
        CHECK_SD(o.inclination, r["inc"]);
        CHECK_SD(o.eccentricity, r["ecc"]);
        CHECK_SD(o.semiMajorAxis, r["sma"]);
        CHECK_STATE(r["state"]);
        if (r.contains("lan_final")) {
            if (home)
                CHECK_SD(o.LAN, r["lan_final"]);
            else
                CHECK_MESSAGE(NearD(o.LAN, r["lan_final"], 1e-9), "LAN got ", o.LAN, " want ", r["lan_final"].dump());
            CHECK_MESSAGE(NearD(o.meanAnomalyAtEpoch, r["mna"], 1e-9), "mna got ", o.meanAnomalyAtEpoch, " want ", r["mna"].dump());
        } else {
            CHECK_SD(o.LAN, r["lan"]);
        }
    };
    size_t nHome = 0, nSentinel = 0, nInvalid = 0;
    for (const json& r : R["orbits"]) {
        std::shared_ptr<CometOrbitType> t = s.Type(r["type"].get<std::string>());
        UnityRandom::Global().InitState(r["seed"].get<int>());
        const Orbit o = t->CalculateHomeOrbit();
        CHECK(o.referenceBody == &s.w.Body("Sun"));
        CheckOrbit(o, r, true);
        ++nHome;
    }
    for (const json& r : R["sentinelOrbits"]) {
        std::shared_ptr<CometOrbitType> t = s.Type(r["type"].get<std::string>());
        UnityRandom::Global().InitState(r["seed"].get<int>());
        Orbit sentinel;
        sentinel.LAN = r["sentinel"]["LAN"].get<double>();
        sentinel.argumentOfPeriapsis = r["sentinel"]["argPe"].get<double>();
        sentinel.trueAnomaly = r["sentinel"]["trueAnomaly"].get<double>();
        sentinel.radius = r["sentinel"]["radius"].get<double>();
        bool valid = false;
        const Orbit o = t->CalculateSentinelOrbit(sentinel, r["minDiscovery"].get<double>(), valid);
        CHECK(valid == r["valid"].get<bool>());
        CheckOrbit(o, r, false);
        ++nSentinel;
        if (!valid) ++nInvalid;
    }
    CHECK(nHome == 160);
    CHECK(nSentinel == 160);
    CHECK(nInvalid > 0);  // the table has invalid and valid orbits (the spawn distance past the apoapsis of a bound orbit)
    // GetRandomObjClass: Random.Range((int)minCometClass, (int)maxCometClass + 1)
    for (int seed = 1; seed < 30; ++seed) {
        for (const auto& t : s.mgr->cometDefinitions) {
            UnityRandom::Global().InitState(seed);
            const int expect = UnityRandom::Global().Range(static_cast<int>(t->minCometClass), static_cast<int>(t->maxCometClass) + 1);
            UnityRandom::Global().InitState(seed);
            const UntrackedObjectClass cls = t->GetRandomObjClass();
            CHECK(static_cast<int>(cls) == expect);
            CHECK(static_cast<int>(cls) >= static_cast<int>(t->minCometClass));
            CHECK(static_cast<int>(cls) <= static_cast<int>(t->maxCometClass));
        }
    }
    // the public constructor and Load
    CometOrbitType made("Made", -1.0f, 2.0f, 3.0f, 4.0f, 0.5f, 0.1f, 0.2f, 0.3f, 0.4f, true, 1.1f, 1.2f, UntrackedObjectClass::B, UntrackedObjectClass::D);
    CHECK(made.name == "Made");
    CHECK(made.useEccentricity);
    CHECK(made.maxCometClass == UntrackedObjectClass::D);
    CHECK(made.chanceWeight == 5);
    ConfigNode cfg("COMET_ORBIT_TYPE");
    cfg.AddValue("name", "Loaded");
    cfg.AddValue("minCometClass", "C");
    cfg.AddValue("maxCometClass", "NotAClass");
    cfg.AddValue("chanceWeight", static_cast<int32_t>(9));
    made.Load(cfg);
    CHECK(made.name == "Loaded");
    CHECK(made.minCometClass == UntrackedObjectClass::C);
    CHECK(made.maxCometClass == UntrackedObjectClass::I);  // TryGetEnum default
    CHECK(made.chanceWeight == 9);
    CHECK(made.inclination1Min == -1.0f);                  // keys that are absent keep their value
}

// ================================================================================================================================ names

TEST_CASE("WP-38d DiscoverableObjectsUtil: asteroid names, comet names") {
    Space38d s;
    const json& R = Ref38d();
    for (const json& r : R["asteroidNames"]) {
        UnityRandom::Global().InitState(r["seed"].get<int>());
        CHECK(DiscoverableObjectsUtil::GenerateAsteroidName() == r["name"].get<std::string>());
        CHECK_STATE(r["state"]);
    }
    const json& tables = R["cometNameTables"];
    auto V = [](const json& a) { return a.get<std::vector<std::string>>(); };
    // SYNTHETIC name tables (the CrewGenerator text assets are not in the repo)
    CrewGenerator::SetNameTables(V(tables["names"]["male"]), V(tables["names"]["female"]), V(tables["special"]["male"]), V(tables["special"]["female"]), "Kerman");
    Game& game = s.w.game;
    for (const json& r : R["cometNames"]) {
        game.cometNames.Clear();
        for (const json& kv : r["dict"]) game.cometNames.Add(kv[0].get<std::string>(), kv[1].get<int>());
        KSPRandom k(r["seed"].get<int>());
        CHECK(DiscoverableObjectsUtil::GenerateCometName(&k) == r["name"].get<std::string>());
        REQUIRE(game.cometNames.Count() == static_cast<int>(r["after"].size()));
        for (size_t i = 0; i < r["after"].size(); ++i) {
            CHECK(game.cometNames.KeysList()[i] == r["after"][i][0].get<std::string>());
            CHECK(game.cometNames[r["after"][i][0].get<std::string>()] == r["after"][i][1].get<int>());
        }
    }
    // no HighLogic.CurrentGame: the plain generated name with " 1"
    SaveEnvironment::Current().currentGame = nullptr;
    for (const json& r : R["cometNamesNoGame"]) {
        KSPRandom k(r["seed"].get<int>());
        CHECK(DiscoverableObjectsUtil::GenerateCometName(&k) == r["name"].get<std::string>());
    }
    SaveEnvironment::Current().currentGame = &game;
    // the tables are required (the C# indexes null arrays)
    CrewGenerator::Reset();
    KSPRandom k(1);
    CHECK_THROWS_AS(DiscoverableObjectsUtil::GenerateCometName(&k), NullReferenceException);
}

// ================================================================================================================================ ScenarioDiscoverableObjects

namespace {

// the proto-vessel nodes of one spawn, read back from SpaceObjectsEnvironment::addedNodes
const ConfigNode& LastNode() {
    REQUIRE(!SpaceObjectsEnvironment::addedNodes.empty());
    return *SpaceObjectsEnvironment::addedNodes.back();
}
double NodeD(const ConfigNode& n, const char* key) {
    double v = std::numeric_limits<double>::quiet_NaN();
    REQUIRE_MESSAGE(n.TryGetValue(key, v), key);
    return v;
}
std::vector<std::string> ChildNames(const ConfigNode& n) {
    std::vector<std::string> order;
    for (ConfigNode* c : n.GetNodes()) order.push_back(c->name);
    return order;
}

// HighLogic.CurrentGame.AddVessel replaced by a recorder: the proto vessel ids count up, the vessels are empty fixtures
struct Recorder {
    std::vector<std::unique_ptr<Vessel>> vessels;
    uint32_t nextId = 5000;
    Recorder() {
        SpaceObjectsEnvironment::addVessel = [this](const ConfigNode& n) {
            AddedVessel a;
            a.persistentId = nextId++;
            auto v = std::make_unique<Vessel>();
            v->persistentId = a.persistentId;
            v->vesselType = VesselType::SpaceObject;
            v->loaded = false;
            std::string name;
            n.TryGetValue("name", name);
            v->vesselName = name;
            a.vessel = v.get();
            vessels.push_back(std::move(v));
            return a;
        };
    }
};

}  // namespace

TEST_CASE("WP-38d ScenarioDiscoverableObjects: the default size curve and GetSizeCurveBasedClass") {
    Space38d s;
    const json& R = Ref38d();
    ScenarioDiscoverableObjects sdo;
    sdo.Awake();
    CHECK(ScenarioDiscoverableObjects::Instance == &sdo);
    REQUIRE(sdo.hasSizeCurve());
    const json& t = R["sizeCurve"];
    REQUIRE(sdo.sizeCurve().Curve().length() == static_cast<int>(t["keys"].size()));
    for (size_t i = 0; i < t["keys"].size(); ++i) {
        const Keyframe& k = sdo.sizeCurve().Curve().keys()[i];
        CHECK_SF(k.time, t["keys"][i][0]);
        CHECK_SF(k.value, t["keys"][i][1]);
        CHECK_SF(k.inTangent, t["keys"][i][2]);
        CHECK_SF(k.outTangent, t["keys"][i][3]);
    }
    for (const json& smp : t["samples"]) CHECK_SF(sdo.sizeCurve().Evaluate(smp[0].get<float>()), smp[1]);
    size_t n = 0;
    for (const json& r : R["sizeClass"]) {
        sdo.minAsteroidClass = static_cast<UntrackedObjectClass>(r["min"].get<int>());
        sdo.maxAsteroidClass = static_cast<UntrackedObjectClass>(r["max"].get<int>());
        UnityRandom::Global().InitState(r["seed"].get<int>());
        const UntrackedObjectClass c = sdo.GetSizeCurveBasedClass(r["lo"].get<float>(), r["hi"].get<float>());
        CHECK(static_cast<int>(c) == r["class"].get<int>());
        CHECK_STATE(r["state"]);
        ++n;
    }
    CHECK(n == 180);
    // a second instance destroys itself (OnAwake logs the error and leaves Instance alone)
    ScenarioDiscoverableObjects second;
    second.Awake();
    CHECK(second.destroyed_);
    CHECK(ScenarioDiscoverableObjects::Instance == &sdo);
    second.destroyed_ = false;
}

TEST_CASE("WP-38d ScenarioDiscoverableObjects: SpawnAsteroid (home and Dres) against the reference") {
    Space38d s;
    const json& R = Ref38d();
    Recorder rec;
    ScenarioDiscoverableObjects sdo;
    sdo.Awake();
    REQUIRE(std::fabs(s.w.Body("Dres").sphereOfInfluence - R["dres"]["soi"].get<double>()) <= 1e-15 * R["dres"]["soi"].get<double>());
    size_t home = 0, dres = 0;
    uint32_t expectId = 5000;
    for (const json& r : R["spawnAsteroid"]) {
        const bool reached = r["reached"].get<bool>();
        SpaceObjectsEnvironment::bodyReached = [reached](const std::string& n) { return reached && n == "Dres"; };
        UnityRandom::Global().InitState(r["seed0"].get<int>());
        sdo.SpawnAsteroid();
        CHECK(sdo.lastSeed() == r["seed"].get<int>());
        const ConfigNode& n = LastNode();
        CHECK(n.GetValue("name") == r["name"].get<std::string>());
        CHECK(n.GetValue("type") == "SpaceObject");
        CHECK(n.GetValue("sit") == "ORBITING");
        const ConfigNode* orbit = n.GetNode("ORBIT");
        const ConfigNode* part = n.GetNode("PART");
        const ConfigNode* discovery = n.GetNode("DISCOVERY");
        REQUIRE(orbit != nullptr);
        REQUIRE(part != nullptr);
        REQUIRE(discovery != nullptr);
        // node order of CreateVesselNode: ORBIT, the part nodes, then the additional nodes in the order of the C# arguments (ACTIONGROUPS, DISCOVERY)
        CHECK(ChildNames(n) == std::vector<std::string>{"ORBIT", "PART", "ACTIONGROUPS", "DISCOVERY"});
        CHECK(part->GetValue("name") == "PotatoRoid");
        CHECK(part->GetValue("uid") == std::to_string(static_cast<uint32_t>(r["seed"].get<int>())));
        CHECK(discovery->GetValue("size") == std::to_string(r["class"].get<int>()));
        CHECK(discovery->GetValue("state") == "1");  // DiscoveryLevels.Presence
        CHECK_SD(NodeD(*discovery, "lifetime"), r["lifeTime"]);
        CHECK_SD(NodeD(*discovery, "refTime"), r["lifeTimeMax"]);
        CHECK_STATE(r["state"]);
        CHECK(sdo.discoveredObjectIDs.back() == expectId);
        CHECK(sdo.untrackedObjectIDs.back() == expectId);
        ++expectId;
        if (r["dres"].get<bool>()) {
            ++dres;
            const json& o = r["orbit"];
            CHECK(orbit->GetValue("REF") == std::to_string(s.w.Body("Dres").flightGlobalsIndex));
            CHECK_SD(NodeD(*orbit, "ECC"), o["ecc"]);
            CHECK_SD(NodeD(*orbit, "SMA"), o["sma"]);
            CHECK_SD(NodeD(*orbit, "INC"), o["inc"]);
            CHECK_SD(NodeD(*orbit, "LAN"), o["lan"]);
            CHECK_SD(NodeD(*orbit, "LPE"), o["argpe"]);
            CHECK_SD(NodeD(*orbit, "MNA"), o["mna"]);
            CHECK_SD(NodeD(*orbit, "EPH"), o["epoch"]);
        } else {
            ++home;
            CHECK(orbit->GetValue("REF") == std::to_string(s.w.Body("Sun").flightGlobalsIndex));  // the fly-by orbit is heliocentric
        }
    }
    CHECK(home > 40);
    CHECK(dres > 10);
    // SpawnLastAsteroid replays lastSeed: the same seed, the same node text apart from the Guid / ids
    SpaceObjectsEnvironment::bodyReached = nullptr;
    sdo.setLastSeed(777);
    SpaceObjectsEnvironment::addedNodes.clear();
    sdo.SpawnLastAsteroid();
    const std::string first = LastNode().GetNode("DISCOVERY")->ToString() + LastNode().GetValue("name").value_or("");
    sdo.SpawnLastAsteroid();
    CHECK(LastNode().GetNode("DISCOVERY")->ToString() + LastNode().GetValue("name").value_or("") == first);
}

TEST_CASE("WP-38d ScenarioDiscoverableObjects: SpawnComet against the reference, the CometVessel node") {
    Space38d s;
    const json& R = Ref38d();
    Recorder rec;
    ScenarioDiscoverableObjects sdo;
    sdo.Awake();
    for (const json& r : R["spawnComet"]) {
        UnityRandom::Global().InitState(r["seed0"].get<int>());
        sdo.SpawnComet(r["type"].get<std::string>());
        CHECK(sdo.lastSeed() == r["seed"].get<int>());
        const ConfigNode& n = LastNode();
        CHECK(n.GetValue("name") == "UnknownComet");
        const ConfigNode* discovery = n.GetNode("DISCOVERY");
        const ConfigNode* vm = n.GetNode("VESSELMODULES");
        const ConfigNode* orbit = n.GetNode("ORBIT");
        REQUIRE(discovery != nullptr);
        REQUIRE(vm != nullptr);
        REQUIRE(orbit != nullptr);
        CHECK(ChildNames(n) == std::vector<std::string>{"ORBIT", "PART", "ACTIONGROUPS", "DISCOVERY", "VESSELMODULES"});
        CHECK(discovery->GetValue("size") == std::to_string(r["class"].get<int>()));
        CHECK_SD(NodeD(*discovery, "lifetime"), r["lifeTime"]);
        CHECK_SD(NodeD(*discovery, "refTime"), r["lifeTimeMax"]);
        const ConfigNode* cv = vm->GetNode("CometVessel");
        REQUIRE(cv != nullptr);
        const json& d = r["definition"];
        CHECK(cv->GetValue("typeName") == r["type"].get<std::string>());
        CHECK_SD(NodeD(*cv, "vfxStartDistance"), d["vfxStartDistance"]);
        CHECK_SD(NodeD(*cv, "comaWidthRatio"), d["comaWidthRatio"]);
        CHECK_SD(NodeD(*cv, "tailWidthRatio"), d["tailWidthRatio"]);
        CHECK_SD(NodeD(*cv, "tailMaxLengthRatio"), d["tailMaxLengthRatio"]);
        CHECK_SD(NodeD(*cv, "ionTailWidthRatio"), d["ionTailWidthRatio"]);
        CHECK(cv->GetValue("seed") == std::to_string(r["seed"].get<int>()));
        CHECK(std::stof(cv->GetValue("radius").value_or("0")) == d["radius"].get<float>());
        CHECK(cv->GetValue("numGeysers") == std::to_string(d["numGeysers"].get<int>()));
        CHECK(cv->GetValue("numNearDustEmitters") == std::to_string(d["numNearDustEmitters"].get<int>()));
        CHECK(cv->GetValue("optimizedCollider") == "False");
        CHECK(cv->GetValue("fragmentDynamicPressureModifier") == "0");
        CHECK_FALSE(cv->HasValue("hasName"));
        const ConfigNode* part = n.GetNode("PART");
        REQUIRE(part != nullptr);
        CHECK(part->GetValue("name") == "PotatoComet");
        CHECK(cv->GetValue("cometPartId") == part->GetValue("persistentId"));  // the CometVessel remembers the part's persistentId
        const json& o = r["orbit"];
        CHECK_SD(NodeD(*orbit, "INC"), o["inc"]);
        CHECK_SD(NodeD(*orbit, "ECC"), o["ecc"]);
        CHECK_SD(NodeD(*orbit, "SMA"), o["sma"]);
        CHECK_SD(NodeD(*orbit, "LAN"), o["lan_final"]);
        CHECK_MESSAGE(NearD(NodeD(*orbit, "MNA"), o["mna"], 1e-9), "MNA");
        CHECK_STATE(r["state"]);
    }
    // an unknown type name logs a warning and spawns nothing; no draw happens
    const size_t before = SpaceObjectsEnvironment::addedNodes.size();
    UnityRandom::Global().InitState(9);
    const UnityRandom::State st = UnityRandom::Global().state();
    sdo.SpawnComet(std::string("no such type"));
    CHECK(SpaceObjectsEnvironment::addedNodes.size() == before);
    CHECK(UnityRandom::Global().state().s3 == st.s3);
    // SpawnComet() draws the weighted type first
    UnityRandom::Global().InitState(12345);
    sdo.SpawnComet();
    CHECK(SpaceObjectsEnvironment::addedNodes.size() == before + 1);
    // without a CometManager the weighted type is null: the C# dereferences it
    CometManager* const inst = CometManager::Instance;
    CometManager::Instance = nullptr;
    CHECK_THROWS_AS(sdo.SpawnComet(), NullReferenceException);
    CometManager::Instance = inst;
}

TEST_CASE("WP-38d ScenarioDiscoverableObjects: persistence, game database limits, events, expiry") {
    Space38d s;
    ScenarioDiscoverableObjects sdo;
    sdo.Awake();
    // OnLoad reads SPACEOBJECTS_LIMITS from the (shipped) database and the two id lists
    ConfigNode node("SCENARIO");
    node.AddValue("untrackedIDs", "5, 7,x,,9999999999,12");
    node.AddValue("discoveredIDs", "5,7,8");
    sdo.OnLoad(node);
    CHECK(sdo.spawnGroupMinLimit == 3);
    CHECK(sdo.spawnGroupMaxLimit == 10);
    CHECK(sdo.untrackedObjectIDs == std::vector<uint32_t>{5, 7, 12});  // uint.TryParse: whitespace allowed, garbage and overflow skipped
    CHECK(sdo.discoveredObjectIDs == std::vector<uint32_t>{5, 7, 8});
    ConfigNode out("SCENARIO");
    sdo.OnSave(out);
    CHECK(out.GetValue("untrackedIDs") == "5,7,12");
    CHECK(out.GetValue("discoveredIDs") == "5,7,8");
    ConfigNode empty("SCENARIO");
    sdo.OnLoad(empty);  // no values: string.Split of "" gives one empty item, nothing is added
    CHECK(sdo.untrackedObjectIDs.empty());
    CHECK(sdo.discoveredObjectIDs.empty());
    // LoadGameDBSettings: the first node wins; keys that are absent keep the field; no node: warning only
    ConfigNode lim1("SPACEOBJECTS_LIMITS");
    lim1.AddValue("homeBodySpawnGroupMinLimit", static_cast<int32_t>(2));
    ConfigNode lim2("SPACEOBJECTS_LIMITS");
    lim2.AddValue("homeBodySpawnGroupMaxLimit", static_cast<int32_t>(99));
    sdo.LoadGameDBSettings({&lim1, &lim2});
    CHECK(sdo.spawnGroupMinLimit == 2);
    CHECK(sdo.spawnGroupMaxLimit == 10);
    sdo.LoadGameDBSettings({});
    CHECK(sdo.spawnGroupMinLimit == 2);
    // the persistent fields go through the generic field store (lastSeed, sizeCurve)
    sdo.setLastSeed(424242);
    ConfigNode saved("SCENARIO");
    sdo.Save(saved);
    CHECK(saved.GetValue("name") == "ScenarioDiscoverableObjects");
    CHECK(saved.GetValue("lastSeed") == "424242");
    CHECK(saved.HasNode("sizeCurve"));

    // OnKnowledgeChanged (through the real DiscoveryInfo.SetLevel -> GameEvents.onKnowledgeChanged) and OnVesselDestroy
    Vessel v;
    v.persistentId = 77;
    v.loaded = true;
    sdo.discoveredObjectIDs = {77};
    sdo.untrackedObjectIDs.clear();
    v.GetDiscoveryInfo().SetLevel(DiscoveryLevels::Presence);  // no state vectors: untracked
    CHECK(sdo.untrackedObjectIDs == std::vector<uint32_t>{77});
    v.GetDiscoveryInfo().SetLevel(DiscoveryLevels::Presence);  // AddUnique
    CHECK(sdo.untrackedObjectIDs == std::vector<uint32_t>{77});
    v.GetDiscoveryInfo().SetLevel(DiscoveryLevels::Owned);  // tracked
    CHECK(sdo.untrackedObjectIDs.empty());
    sdo.discoveredObjectIDs.clear();
    v.GetDiscoveryInfo().SetLevel(DiscoveryLevels::Presence);  // not a discovered object: ignored
    CHECK(sdo.untrackedObjectIDs.empty());
    // a host that is not a vessel is ignored
    sdo.OnKnowledgeChanged(events::GameEvents::HostedFromToAction<IDiscoverable*, DiscoveryLevels>(nullptr, DiscoveryLevels::None, DiscoveryLevels::Presence));
    sdo.discoveredObjectIDs = {77, 78};
    sdo.untrackedObjectIDs = {77, 78};
    events::GameEvents::onVesselDestroy.Fire(&v);
    CHECK(sdo.discoveredObjectIDs == std::vector<uint32_t>{78});
    CHECK(sdo.untrackedObjectIDs == std::vector<uint32_t>{78});

    // ExpireDiscoveredObjects: the vessels are walked from the end; the first lost one dies (event + Die), later ones only count
    Vessel a, b, c;
    for (Vessel* x : {&a, &b, &c}) {
        x->loaded = true;
        x->vesselType = VesselType::SpaceObject;
        x->GetDiscoveryInfo().SetLevel(DiscoveryLevels::Presence);
        x->GetDiscoveryInfo().SetLastObservedTime(0.0);
    }
    a.persistentId = 1;
    b.persistentId = 2;
    c.persistentId = 3;
    a.GetDiscoveryInfo().SetUnobservedLifetime(1000.0);  // alive at UT 500
    b.GetDiscoveryInfo().SetUnobservedLifetime(100.0);   // lost
    c.GetDiscoveryInfo().SetUnobservedLifetime(100.0);   // lost
    FlightRegistry::Vessels = {&a, &b, &c};
    sdo.discoveredObjectIDs = {1, 2, 3};
    int expired = 0;
    Vessel* expiredVessel = nullptr;
    struct Listener {
        int* n;
        Vessel** v;
        void On(Vessel* x) {
            ++*n;
            *v = x;
        }
    } listener{&expired, &expiredVessel};
    events::GameEvents::OnDiscoverableObjectExpired.Add(&listener, &Listener::On);
    CHECK(sdo.ExpireDiscoveredObjects(500.0));
    events::GameEvents::OnDiscoverableObjectExpired.Remove(&listener, &Listener::On);
    CHECK(expired == 1);
    CHECK(expiredVessel == &c);  // the walk starts at the end
    CHECK(c.state == Vessel::State::DEAD);
    CHECK(b.state != Vessel::State::DEAD);  // the second lost vessel survives this sweep
    FlightRegistry::Vessels.clear();
    CHECK_FALSE(sdo.ExpireDiscoveredObjects(500.0));  // nothing left to expire
}

TEST_CASE("WP-38d ScenarioDiscoverableObjects: Start, the spawn daemon and UpdateSpaceObjects") {
    Space38d s(false);  // no CometManager: ShouldSpawnComet is false and draws nothing
    Recorder rec;
    ScenarioDiscoverableObjects sdo;
    sdo.Awake();
    sdo.spawnOddsAgainst = 1;  // Random.Range(0, 1) == 0: every pass with a free slot spawns
    sdo.untrackedObjectIDs = {42, 43};
    sdo.setLastSeed(31337);
    // Start: Random.InitState(lastSeed); the untracked ids that are no longer vessels are dropped
    SpaceObjectsEnvironment::persistentVesselIdKnown = [](uint32_t id) { return id == 43; };
    SpaceObjectsEnvironment::discoveryUnlocked = [] { return false; };
    CoroutineScheduler& sched = CareerEnvironment::Current().Scheduler();
    sdo.Start();
    CHECK(sdo.untrackedObjectIDs == std::vector<uint32_t>{43});
    {
        UnityRandom expect(31337);
        CHECK(UnityRandom::Global().state().s0 == expect.state().s0);
        CHECK(UnityRandom::Global().state().s3 == expect.state().s3);  // nothing was drawn: discovery is not unlocked
    }
    sched.AdvanceFrame(0.02f);
    sched.RunDelayedDynamicFrameRate();
    CHECK(SpaceObjectsEnvironment::addedNodes.empty());
    sdo.OnDestroy();
    CHECK(sdo.discoveredObjectIDs.empty());
    // unlocked: one pass spawns, then the daemon waits spawnInterval (15 s at TimeWarp rate 1) before the next one
    ScenarioDiscoverableObjects sdo2;
    SpaceObjectsEnvironment::discoveryUnlocked = [] { return true; };
    SpaceObjectsEnvironment::bodyReached = [](const std::string&) { return false; };
    sdo2.Awake();
    sdo2.spawnOddsAgainst = 1;
    sdo2.setLastSeed(5);
    sdo2.Start();
    sched.AdvanceFrame(0.02f);
    sched.RunDelayedDynamicFrameRate();
    CHECK(SpaceObjectsEnvironment::addedNodes.size() == 1);
    for (int i = 0; i < 10; ++i) {
        sched.AdvanceFrame(1.0f);
        sched.RunDelayedDynamicFrameRate();
    }
    CHECK(SpaceObjectsEnvironment::addedNodes.size() == 1);  // 10 s: still waiting
    for (int i = 0; i < 8; ++i) {
        sched.AdvanceFrame(1.0f);
        sched.RunDelayedDynamicFrameRate();
    }
    CHECK(SpaceObjectsEnvironment::addedNodes.size() == 2);  // past 15 s: the second pass
    CHECK(sdo2.discoveredObjectIDs.size() == 2);
    sdo2.OnDestroy();
}

TEST_CASE("WP-38d CometManager: vessel events, discovered comets, fragments; the Sentinel seams") {
    Space38d s;
    Recorder rec;
    // PotatoComet space objects register on onFlightGlobalsAddVessel and leave on onFlightGlobalsRemoveVessel
    VRig comet(s.w, s.w.Body("Sun"), {"PotatoComet"});
    comet.v.vesselType = VesselType::SpaceObject;
    comet.v.persistentId = 901;
    VRig rock(s.w, s.w.Body("Sun"), {"PotatoRoid"});
    rock.v.vesselType = VesselType::SpaceObject;
    rock.v.persistentId = 902;
    VRig ship(s.w, s.w.Body("Sun"), {"PotatoComet"});
    ship.v.vesselType = VesselType::Ship;
    ship.v.persistentId = 903;
    for (Vessel* v : {&comet.v, &rock.v, &ship.v, &comet.v}) events::GameEvents::onFlightGlobalsAddVessel.Fire(v);
    CHECK(s.mgr->Comets() == std::vector<Vessel*>{&comet.v});  // roids and ships do not count; AddUnique
    comet.v.GetDiscoveryInfo().SetLevel(DiscoveryLevels::Presence);
    CHECK(s.mgr->DiscoveredComets().empty());
    comet.v.GetDiscoveryInfo().SetLevel(DiscoveryLevels::StateVectors);
    CHECK(s.mgr->DiscoveredComets() == std::vector<Vessel*>{&comet.v});
    events::GameEvents::onFlightGlobalsRemoveVessel.Fire(&rock.v);
    CHECK(s.mgr->Comets().size() == 1);
    events::GameEvents::onFlightGlobalsRemoveVessel.Fire(&comet.v);
    CHECK(s.mgr->Comets().empty());
    // a second CometManager destroys itself and clears Instance (the first one stays alive, without an Instance)
    {
        CometManager second;
        second.Awake();
        CHECK(second.destroyed());
        CHECK(CometManager::Instance == nullptr);
        CometManager::Instance = s.mgr.get();
    }
    CHECK(CometManager::VFXUpdateFrequency() == 30.0);
    CHECK(CometManager::VFXUpdateFrequencyInAtmosphere() == 0.029999999329447746);
    CHECK(CometManager::SentinelDiscoveryArc() == 70.0f);

    // SpawnCometFragments: 3..5 fragments named "<name>-A".., classes stepped down
    comet.v.vesselName = "Test Comet";
    comet.v.loaded = true;
    comet.v.GetDiscoveryInfo().SetLevel(DiscoveryLevels::Owned);
    comet.v.GetDiscoveryInfo().SetUntrackedObjectSize(UntrackedObjectClass::G);
    comet.v.GetDiscoveryInfo().SetLastObservedTime(123.0);
    comet.v.GetDiscoveryInfo().SetReferenceLifetime(456.0);
    comet.v.externalTemperature = 11.0;
    comet.v.atmosphericTemperature = 22.0;
    comet.v.srf_vel_direction = Vector3d(0.0, 1.0, 0.0);
    comet.v.orbitDriver->orbit = std::make_shared<Orbit>(5.0, 0.7, 4.0e10, 40.0, 10.0, 0.5, 0.0, &s.w.Body("Sun"));
    comet.v.orbitDriver->orbit->UpdateFromUT(1000.0);
    Planetarium::fetch->time = 1000.0;
    CometVesselSeam seam;
    seam.Vessel_ = &comet.v;
    seam.CometType = s.Type("long").get();
    seam.radius = 30.0f;
    int explosions = 0;
    float explosionScale = 0.0f;
    Vessel* followed = nullptr;
    CometManager::explodeWithDebris = [&](Vessel&, float scale, Vessel* f) {
        ++explosions;
        explosionScale = scale;
        followed = f;
    };
    CometManager::cometFragmentDynamicPressureModifier = [](Vessel&) { return std::optional<float>(2.0f); };
    SpaceObjectsEnvironment::addedNodes.clear();
    UnityRandom::Global().InitState(2024);
    s.mgr->SpawnCometFragments(seam);
    const size_t fragments = SpaceObjectsEnvironment::addedNodes.size();
    CHECK(fragments >= 3);
    CHECK(fragments <= 5);
    CHECK(explosions == 1);
    CHECK(explosionScale == doctest::Approx(6.0f * 0.2f));  // (float)UntrackedObjectClass.G * explosionScaleMultiplier
    REQUIRE(rec.vessels.size() == fragments);
    CHECK(followed == rec.vessels[0].get());
    for (size_t i = 0; i < fragments; ++i) {
        const ConfigNode& n = *SpaceObjectsEnvironment::addedNodes[i];
        CHECK(n.GetValue("name") == "Test Comet-" + std::string(1, static_cast<char>('A' + i)));
        const ConfigNode* cv = n.GetNode("VESSELMODULES")->GetNode("CometVessel");
        REQUIRE(cv != nullptr);
        CHECK(cv->GetValue("optimizedCollider") == "True");
        const float pressure = std::stof(cv->GetValue("fragmentDynamicPressureModifier").value_or("0"));
        CHECK(pressure >= 2.0f + 1.0f);  // component + Random.Range(fragmentMaxPressureModifier = 1, fragmentMinPressureModifier = 6)
        CHECK(pressure <= 2.0f + 6.0f);
        const int cls = std::stoi(n.GetNode("DISCOVERY")->GetValue("size").value_or("0"));
        CHECK(cls >= 6 - 2);  // G stepped down by Range(1, 2 + 1)
        CHECK(cls <= 6 - 1);
        Vessel& f = *rec.vessels[i];
        CHECK(f.vesselName == n.GetValue("name"));
        CHECK(f.IgnoreCollisionsFrames() == 20);
        CHECK(f.externalTemperature == 11.0);
        CHECK(f.atmosphericTemperature == 22.0);
        CHECK(f.GetDiscoveryInfo().Level() == DiscoveryLevels::Owned);
        // sic: SpawnCometFragment passes the original's lastObservedTime as the fragment's lifeTime and its referenceLifetime as lifeTimeMax
        CHECK(n.GetNode("DISCOVERY")->GetValue("lifetime") == "123");
        CHECK(n.GetNode("DISCOVERY")->GetValue("refTime") == "456");
    }
    // determinism: the same seed replays the same fragments
    auto Sig = [](const ConfigNode& n) {
        const ConfigNode* cv = n.GetNode("VESSELMODULES")->GetNode("CometVessel");
        return cv->GetValue("seed").value_or("") + "/" + cv->GetValue("radius").value_or("") + "/" + cv->GetValue("fragmentDynamicPressureModifier").value_or("") + "/" +
               n.GetNode("ORBIT")->GetValue("SMA").value_or("") + "/" + n.GetNode("DISCOVERY")->GetValue("size").value_or("");
    };
    const std::string firstRun = Sig(*SpaceObjectsEnvironment::addedNodes[0]);
    SpaceObjectsEnvironment::addedNodes.clear();
    UnityRandom::Global().InitState(2024);
    s.mgr->SpawnCometFragments(seam);
    CHECK(Sig(*SpaceObjectsEnvironment::addedNodes[0]) == firstRun);
    // a class A comet cannot step down: no fragments, the explosion still happens
    comet.v.GetDiscoveryInfo().SetUntrackedObjectSize(UntrackedObjectClass::A);
    SpaceObjectsEnvironment::addedNodes.clear();
    explosions = 0;
    s.mgr->SpawnCometFragments(seam);
    CHECK(SpaceObjectsEnvironment::addedNodes.empty());
    CHECK(explosions == 1);
    CometManager::explodeWithDebris = nullptr;
    CometManager::cometFragmentDynamicPressureModifier = nullptr;
    CometVesselSeam none;
    s.mgr->SpawnCometFragments(none);  // no vessel: nothing happens

    // the Sentinel seams
    ScenarioDiscoverableObjects sdo;
    sdo.Awake();
    sdo.minAsteroidClass = UntrackedObjectClass::B;
    sdo.maxAsteroidClass = UntrackedObjectClass::H;
    sdo.minUntrackedLifetime = 2.0f;
    sdo.maxUntrackedLifetime = 30.0f;
    sdo.untrackedObjectIDs = {1, 2, 3};
    SpaceObjects::InstallSentinelSeams();
    const auto dobj = SM::SentinelEnvironment::GetDiscoverableObjects();
    REQUIRE(dobj.has_value());
    CHECK(dobj->minAsteroidClass == UntrackedObjectClass::B);
    CHECK(dobj->maxAsteroidClass == UntrackedObjectClass::H);
    CHECK(dobj->minUntrackedLifetime == 2.0f);
    CHECK(dobj->maxUntrackedLifetime == 30.0f);
    CHECK(dobj->untrackedObjectIDsCount == 3);
    FlightRegistry::Vessels = {&comet.v, &rock.v};
    CHECK(SM::SentinelEnvironment::countSpaceObjects() == 2);
    FlightRegistry::Vessels.clear();
    UnityRandom::Global().InitState(4);
    const std::string seamName = SM::SentinelEnvironment::generateAsteroidName();
    UnityRandom::Global().InitState(4);
    CHECK(seamName == DiscoverableObjectsUtil::GenerateAsteroidName());
    auto type = SM::SentinelEnvironment::getCometOrbitType("short");
    REQUIRE(type != nullptr);
    CHECK(SM::SentinelEnvironment::getCometOrbitType("nope") == nullptr);
    UnityRandom::Global().InitState(31);
    const bool should = SM::SentinelEnvironment::shouldSpawnComet();
    UnityRandom::Global().InitState(31);
    CHECK(should == CometManager::ShouldSpawnComet());
    SM::SentinelEnvironment::CometDefinitionSeam def = SM::SentinelEnvironment::generateCometDefinition(*type, UntrackedObjectClass::F, 99);
    CHECK(BitsD38b(def.vfxStartDistance) == BitsD38b(CometManager::GenerateDefinition(*s.Type("short"), UntrackedObjectClass::F, 99).vfxStartDistance));
    Orbit o(3.0, 0.5, 3.0e10, 10.0, 20.0, 0.1, 0.0, &s.w.Body("Sun"));
    SpaceObjectsEnvironment::addedNodes.clear();
    SM::SentinelEnvironment::spawnComet("Sent Comet", o, def, 4242u, UntrackedObjectClass::F, 1000.0, 2000.0, false, 0.0f);
    SM::SentinelEnvironment::spawnAsteroid("Ast. XYZ-001", o, 77u, UntrackedObjectClass::C, 10.0, 20.0);
    REQUIRE(SpaceObjectsEnvironment::addedNodes.size() == 2);
    CHECK(SpaceObjectsEnvironment::addedNodes[0]->GetValue("name") == "Sent Comet");
    CHECK(SpaceObjectsEnvironment::addedNodes[0]->GetNode("PART")->GetValue("name") == "PotatoComet");
    CHECK(SpaceObjectsEnvironment::addedNodes[1]->GetNode("PART")->GetValue("name") == "PotatoRoid");
    s.mgr->Comets().push_back(&comet.v);
    comet.v.GetDiscoveryInfo().SetLevel(DiscoveryLevels::StateVectors);
    CHECK(FinePrint::Contracts::CometManagerSeam::discoveredComets() == std::vector<Vessel*>{&comet.v});
    s.mgr->Comets().clear();
    SpaceObjects::UninstallSentinelSeams();
    CHECK_FALSE(static_cast<bool>(SM::SentinelEnvironment::discoverableObjectsProvider));
    CHECK_FALSE(static_cast<bool>(SM::SentinelEnvironment::shouldSpawnComet));
}

TEST_CASE("WP-38d DiscoverableObjectsUtil.SpawnAsteroid / SpawnComet: the default AddVessel builds a real vessel from PotatoRoid / PotatoComet") {
    Space38d s;
    Planetarium::fetch->time = 500.0;
    UnityRandom::Global().InitState(11);
    const Orbit orbit = Orbit::CreateRandomOrbitFlyBy(s.w.Body("Kerbin"), 30.0, Planetarium::GetUniversalTime());
    const size_t before = FlightRegistry::Vessels.size();
    AddedVessel a = DiscoverableObjectsUtil::SpawnAsteroid("Ast. QQQ-123", orbit, 31337u, UntrackedObjectClass::D, 3.0 * 86400.0, 20.0 * 86400.0);
    REQUIRE(a.protoVessel != nullptr);
    REQUIRE(a.vessel != nullptr);
    CHECK(a.persistentId != 0);
    CHECK(FlightRegistry::Vessels.size() == before + 1);
    CHECK(a.vessel->vesselName == "Ast. QQQ-123");
    CHECK(a.vessel->vesselType == VesselType::SpaceObject);
    CHECK(a.vessel->GetDiscoveryInfo().Level() == DiscoveryLevels::Presence);
    CHECK(a.vessel->GetDiscoveryInfo().objectSize() == UntrackedObjectClass::D);
    CHECK(a.vessel->GetDiscoveryInfo().referenceLifetime() == 20.0 * 86400.0);
    CHECK(a.protoVessel->protoPartSnapshots.size() == 1);
    CHECK(a.protoVessel->protoPartSnapshots[0]->partName.value_or("") == "PotatoRoid");
    CHECK(a.vessel->orbit().referenceBody == &s.w.Body("Sun"));
    // a comet: the definition travels in VESSELMODULES
    CometDefinition d = CometManager::GenerateDefinition(*s.Type("short"), UntrackedObjectClass::F, 8888);
    UnityRandom::Global().InitState(12);
    const Orbit home = s.Type("short")->CalculateHomeOrbit();
    AddedVessel c = DiscoverableObjectsUtil::SpawnComet("Comet Test 1", home, d, 8888u, UntrackedObjectClass::F, 1.0e6, 2.0e6, false, 0.0f);
    REQUIRE(c.vessel != nullptr);
    CHECK(c.protoVessel->protoPartSnapshots[0]->partName.value_or("") == "PotatoComet");
    CHECK(c.vessel->GetDiscoveryInfo().objectSize() == UntrackedObjectClass::F);
    // the scenario's bookkeeping takes the proto vessel's persistentId
    ScenarioDiscoverableObjects sdo;
    sdo.Awake();
    UnityRandom::Global().InitState(99);
    sdo.SpawnAsteroid();
    CHECK(sdo.discoveredObjectIDs.size() == 1);
    CHECK(sdo.untrackedObjectIDs == sdo.discoveredObjectIDs);
    CHECK(SpaceObjectsEnvironment::spawned.size() == 3);
}

// ================================================================================================================================ ConstructShip / CraftProfileInfo

#include "ksp/fineprint/prebuilt_spawn.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/ship/ship_template.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/wheels/wheel_modules.hpp"

namespace {

namespace LT = ksp::Contracts::Lite;

// Real stock crafts stand in for the pre-built contract crafts (the Squad/Contracts/PrebuiltCraft files of the mirror are empty): harness/inputs/Ships
std::string ShipPath(const char* folder, const char* name) { return std::string(KSPCORE_HARNESS_DIR) + "/inputs/Ships/" + folder + "/" + name + ".craft"; }

std::shared_ptr<LT::PreBuiltCraftDefinition> CraftDef(const std::string& path) {
    ConfigNode::NodePtr root = ConfigNode::Load(path);
    REQUIRE(root);
    auto def = std::make_shared<LT::PreBuiltCraftDefinition>(path, root);
    return def;
}

struct SpawnWorld {
    Space38d s{false};
    int puts = 0;
    Vector3f lastPos{};
    Quaternionf lastRot = Quaternionf::identity();
    SpawnWorld() {
        // SYNTHETIC placement: the mirror has no part models, so the collider bounds PutShipToGround reads are empty; the rigid placement of the ship at
        // the spawn point stands in for it and records what ConstructShip passes
        LT::PreBuiltSpawnEnvironment::putShipToGround = [this](ShipConstruct& ship, const LT::PreBuiltVesselSituation&, const Vector3f& pos, const Quaternionf& rot) {
            ++puts;
            lastPos = pos;
            lastRot = rot;
            for (Part* p : ship.liveParts) {
                p->partTransform.position = pos + rot * p->fields.orgPos;
                p->partTransform.rotation = rot * p->fields.orgRot;
            }
        };
        // SYNTHETIC terrain: a flat 70 m surface (the PQS of the bodies is not built in the fixture)
        LT::PreBuiltSpawnEnvironment::surfaceHeight = [](CelestialBody* b, const Vector3d&) { return b->Radius + 70.0; };
        s.w.game.flagURL = "Squad/Flags/default";
        Planetarium::fetch->UpdateCBsRecursive(Planetarium::fetch->Sun);  // the scene updates the body positions every frame (FlightGlobals.getMainBody reads them)
    }
    ~SpawnWorld() {
        LT::PreBuiltSpawnEnvironment::Reset();
        LT::PreBuiltShipSpawner::Reset();
        FlightRegistry::Clear();
    }
};

LT::PreBuiltVesselSituation LandedSituation(CelestialBody* body, const std::string& craftFile) {
    LT::PreBuiltVesselSituation sit;
    sit.targetBody = body;
    sit.latitude = -0.0972;
    sit.longitude = -74.5576;
    sit.situation = LT::PreBuiltVesselSituation::StartSituation::LANDED;
    sit.craftFile = craftFile;
    sit.vesselName = "Contract Rover Alpha";
    return sit;
}

}  // namespace

namespace {
Vector3f ShipTemplate_SizeOf(const ConfigNode::NodePtr& root) {
    ShipTemplate t;
    t.LoadShip(root->CreateCopy());
    return t.GetShipSize();
}
}  // namespace

TEST_CASE("WP-38d CraftProfileInfo.LoadDetailsFromCraftFile: the template fields, Save / Load") {
    Space38d s(false);
    const std::string path = ShipPath("VAB", "PhysProf-S");
    ConfigNode::NodePtr root = ConfigNode::Load(path);
    REQUIRE(root);
    LT::CraftProfileInfo info;
    info.LoadDetailsFromCraftFile(*root, path, false, true);
    CHECK(info.shipName == root->GetValue("ship").value_or("?"));
    CHECK(info.shipFacility == EditorFacility::VAB);
    CHECK(info.partCount == root->CountNodes());
    CHECK(info.partCount > 5);
    CHECK(info.stageCount >= 1);
    CHECK(info.totalMass > 0.0f);
    CHECK(info.totalCost > 0.0f);
    CHECK_FALSE(info.duplicatedParts);
    CHECK(info.shipPartsUnlocked());
    CHECK(info.shipPartModulesAvailable());
    CHECK_FALSE(info.CraftHasErrors());
    CHECK(info.description == root->GetValue("description").value_or(""));
    CHECK(info.compatibility == VersionCompareResult::COMPATIBLE);
    CHECK(info.shipSize == ShipTemplate_SizeOf(root));
    CHECK(info.steamPublishedFileId == 0);
    // the stage count is the highest istg + 1 (ShipTemplate.LoadShip)
    int istg = -1;
    for (ConfigNode* p : root->GetNodes("PART"))
        for (const std::string& v : p->GetValuesList("istg")) istg = std::max(istg, std::stoi(v));
    CHECK(info.stageCount == istg + 1);
    // Save / Load round trip
    ConfigNode saved("CRAFT");
    info.Save(saved);
    CHECK(saved.GetValue("shipName") == info.shipName);
    CHECK(saved.GetValue("type") == "VAB");
    CHECK(saved.GetValuesList("partNames").size() == info.partNames.size());
    LT::CraftProfileInfo loaded;
    loaded.Load(saved);
    CHECK(loaded.shipName == info.shipName);
    CHECK(loaded.description == info.description);
    CHECK(loaded.partCount == info.partCount);
    CHECK(loaded.stageCount == info.stageCount);
    CHECK(Bits38b(loaded.totalMass) == Bits38b(info.totalMass));
    CHECK(Bits38b(loaded.totalCost) == Bits38b(info.totalCost));
    CHECK(loaded.shipFacility == EditorFacility::VAB);
    CHECK(loaded.partNames == info.partNames);
    CHECK(loaded.partModules == info.partModules);
    CHECK(loaded.compatibility == VersionCompareResult::COMPATIBLE);
    CHECK(loaded.shipPartsUnlocked());
    // the description's line breaks travel as U+00A8
    info.description = "line one\nline two";
    ConfigNode saved2("CRAFT");
    info.Save(saved2);
    CHECK(saved2.GetValue("description").value_or("").find('\n') == std::string::npos);
    LT::CraftProfileInfo loaded2;
    loaded2.Load(saved2);
    CHECK(loaded2.description == "line one\nline two");
    // an unknown part: unavailable, and the load of the saved info flags it again
    ConfigNode::NodePtr bad = root->CreateCopy();
    bad->GetNodes("PART")[0]->SetValue("part", "noSuchPart_4294000000");
    LT::CraftProfileInfo badInfo;
    badInfo.LoadDetailsFromCraftFile(*bad, path, false, true);
    CHECK(badInfo.CraftHasErrors());
    CHECK_FALSE(badInfo.shipPartsUnlocked());
    CHECK(std::find(badInfo.UnavailableShipParts.begin(), badInfo.UnavailableShipParts.end(), "noSuchPart") != badInfo.UnavailableShipParts.end());
    // a node without `description` cannot be loaded (the C# dereferences null)
    ConfigNode noDesc("CRAFT");
    CHECK_THROWS_AS(loaded.Load(noDesc), NullReferenceException);
}

TEST_CASE("WP-38d ContractSystem.ConstructShip: a landed rover craft becomes a vessel in the flight state") {
    SpawnWorld w;
    const std::string path = ShipPath("SPH", "Ore Scanner");
    auto def = CraftDef(path);
    CelestialBody& kerbin = w.s.w.Body("Kerbin");
    LT::PreBuiltVesselSituation sit = LandedSituation(&kerbin, path);
    int spawnedEvents = 0;
    Vessel* eventVessel = nullptr;
    struct L {
        int* n;
        Vessel** v;
        void On(Vessel* x) {
            ++*n;
            *v = x;
        }
    } listener{&spawnedEvents, &eventVessel};
    events::GameEvents::Contract::onContractPreBuiltVesselSpawned.Add(&listener, &L::On);
    int created = 0;
    struct C {
        int* n;
        void On(Vessel*) { ++*n; }
    } cl{&created};
    events::GameEvents::onNewVesselCreated.Add(&cl, &C::On);
    const size_t protoBefore = w.s.w.game.flightState->protoVessels.size();
    const int activeBefore = w.s.w.game.flightState->activeVesselIdx;
    const LT::PreBuiltSpawnResult r = LT::PreBuiltShipSpawner::ConstructShip(*def, sit);
    events::GameEvents::Contract::onContractPreBuiltVesselSpawned.Remove(&listener, &L::On);
    events::GameEvents::onNewVesselCreated.Remove(&cl, &C::On);
    REQUIRE_MESSAGE(r.spawned, r.error);
    REQUIRE(r.vessel != nullptr);
    CHECK(spawnedEvents == 1);
    CHECK(eventVessel == r.vessel);
    CHECK(created == 1);  // onNewVesselCreated (non-flight scene)
    CHECK(r.vessel->vesselName == "Contract Rover Alpha");  // shipOut.shipName = vesselSituation.vesselName (not player created)
    CHECK(r.vessel->Landed);
    CHECK(r.vessel->situation == Vessel::Situations::LANDED);
    CHECK(r.vessel->latitude == static_cast<double>(static_cast<float>(sit.latitude)));
    CHECK(r.vessel->longitude == static_cast<double>(static_cast<float>(sit.longitude)));
    REQUIRE(r.vesselConfigNode->GetNode("ORBIT") != nullptr);
    CHECK_MESSAGE(r.vesselConfigNode->GetNode("ORBIT")->GetValue("REF").value_or("none") == std::to_string(kerbin.flightGlobalsIndex), "REF ", r.vesselConfigNode->GetNode("ORBIT")->GetValue("REF").value_or("none"), " want ", kerbin.flightGlobalsIndex);  // the snapshot's orbit is Kerbin's (orbit.referenceBody = targetBody)
    CHECK(r.vessel->PQSminLevel == 0);
    CHECK(r.vessel->PQSmaxLevel == 0);
    CHECK(r.vessel->mainBody() == &kerbin);
    // the placement: one PutShipToGround at the world surface point of the (float) latitude / longitude at the PQS height, rotated LookRotation(rel) * q * Euler
    CHECK(w.puts == 1);
    const Vector3d expectPos = kerbin.GetWorldSurfacePosition(static_cast<double>(static_cast<float>(sit.latitude)), static_cast<double>(static_cast<float>(sit.longitude)), 70.0);
    CHECK(Bits38b(w.lastPos.x) == Bits38b(expectPos.ToVector3f().x));
    CHECK(Bits38b(w.lastPos.y) == Bits38b(expectPos.ToVector3f().y));
    CHECK(Bits38b(w.lastPos.z) == Bits38b(expectPos.ToVector3f().z));
    const Vector3d rel = kerbin.GetRelSurfacePosition(static_cast<double>(static_cast<float>(sit.latitude)),
                                                     static_cast<double>(static_cast<float>(sit.longitude)) + kerbin.directRotAngle, 70.0);
    const Quaternionf expectRot = (UnityNative::LookRotation(rel.ToVector3f(), Vector3f::up()) * Quaternionf::identity()) *
                                  UnityNative::Euler(Vector3f(0.0f, -90.0f, -90.0f));  // an SPH craft
    CHECK(Bits38b(w.lastRot.x) == Bits38b(expectRot.x));
    CHECK(Bits38b(w.lastRot.w) == Bits38b(expectRot.w));
    CHECK(std::find(FlightRegistry::Vessels.begin(), FlightRegistry::Vessels.end(), r.vessel) != FlightRegistry::Vessels.end());
    ConfigNode::NodePtr craftRoot = def->craftNode;
    CHECK(static_cast<int>(r.vessel->parts.size()) == craftRoot->CountNodes());
    // the proto vessel the reload produced and the one the flight state keeps
    REQUIRE(r.vesselConfigNode != nullptr);
    CHECK(r.vesselConfigNode->GetValue("name") == "Contract Rover Alpha");
    CHECK(r.vesselConfigNode->GetValue("sit") == "LANDED");
    CHECK(r.vesselConfigNode->GetValue("landed") == "True");
    CHECK(r.vesselConfigNode->GetValue("splashed") == "False");
    CHECK(r.vesselConfigNode->GetValue("PQSMin") == "0");
    CHECK(w.s.w.game.flightState->protoVessels.size() == protoBefore + 1);
    CHECK(w.s.w.game.flightState->activeVesselIdx == activeBefore);  // focusOnSpawn is false: the active vessel index is left alone
    // the brakes: brakesOn is the VesselLocation default (true): the Brakes group is set and every brake module is at 1
    CHECK(r.vessel->ActionGroups()[KSPActionGroup::Brakes]);
    bool anyBrake = false;
    for (PartModule* m : r.vessel->FindPartModulesImplementing([](PartModule& pm) { return dynamic_cast<ModuleWheels::ModuleWheelBrakes*>(&pm) != nullptr; })) {
        anyBrake = true;
        CHECK(m->Get<float>("brakeInput") == 1.0f);
    }
    CHECK(anyBrake);
    CHECK(LT::PreBuiltShipSpawner::spawned.size() == 1);
}

TEST_CASE("WP-38d ContractSystem.ConstructShip: orbiting, splashed, focus, failures") {
    SpawnWorld w;
    CelestialBody& kerbin = w.s.w.Body("Kerbin");
    // ---- orbiting: the orbit snapshot is rotated to the spawn UT (MissionOrbit.RelativeOrbit)
    {
        const std::string path = ShipPath("VAB", "PhysProf-S");
        auto def = CraftDef(path);
        LT::PreBuiltVesselSituation sit;
        sit.targetBody = &kerbin;
        sit.situation = LT::PreBuiltVesselSituation::StartSituation::ORBITING;
        sit.hasOrbit = true;
        sit.orbit = Orbit(10.0, 0.01, 900000.0, 25.0, 40.0, 1.5, 0.0, &kerbin);
        sit.vesselName = "Contract Sat Beta";
        sit.focusOnSpawn = true;
        Planetarium::fetch->time = 5000.0;
        const LT::PreBuiltSpawnResult r = LT::PreBuiltShipSpawner::ConstructShip(*def, sit);
        REQUIRE_MESSAGE(r.spawned, r.error);
        CHECK(r.vessel->situation == Vessel::Situations::ORBITING);
        CHECK_FALSE(r.vessel->Landed);
        const Orbit& o = r.vessel->orbit();
        CHECK(o.referenceBody == &kerbin);
        CHECK(o.semiMajorAxis == 900000.0);
        CHECK(o.eccentricity == 0.01);
        CHECK(o.inclination == 10.0);
        // meanAnomalyAtEpoch -= UT * 360 / period * (pi / 180)
        Orbit probe(10.0, 0.01, 900000.0, 25.0, 40.0, 1.5, 0.0, &kerbin);
        const double expect = 1.5 - 5000.0 * 360.0 / probe.period * (3.1415926535897931 / 180.0);
        CHECK(std::fabs(o.meanAnomalyAtEpoch - expect) < 1e-9);
        CHECK(r.vesselConfigNode->GetValue("sit") == "ORBITING");
        CHECK(r.vesselConfigNode->GetValue("landed") == "False");
        // focusOnSpawn sets the flight state's active vessel index
        CHECK(w.s.w.game.flightState->activeVesselIdx == static_cast<int>(w.s.w.game.flightState->protoVessels.size()) - 1);
        // the launch clamp check refuses an orbiting craft with clamps
        LT::PreBuiltSpawnEnvironment::checkLaunchClamps = [](ShipConstruct&, const LT::PreBuiltVesselSituation&, bool) { return false; };
        const size_t before = LT::PreBuiltShipSpawner::spawned.size();
        const LT::PreBuiltSpawnResult bad = LT::PreBuiltShipSpawner::ConstructShip(*def, sit);
        CHECK_FALSE(bad.spawned);
        CHECK(bad.error.find("launch clamps") != std::string::npos);
        CHECK(LT::PreBuiltShipSpawner::spawned.size() == before);
        LT::PreBuiltSpawnEnvironment::checkLaunchClamps = nullptr;
        sit.playerCreated = true;  // a player-created situation skips the clamp check and keeps the craft's own name
        LT::PreBuiltSpawnEnvironment::checkLaunchClamps = [](ShipConstruct&, const LT::PreBuiltVesselSituation&, bool) { return false; };
        const LT::PreBuiltSpawnResult kept = LT::PreBuiltShipSpawner::ConstructShip(*def, sit);
        CHECK(kept.spawned);
        CHECK(kept.vessel->vesselName != "Contract Sat Beta");
        LT::PreBuiltSpawnEnvironment::checkLaunchClamps = nullptr;
    }
    // ---- splashed: a surface below sea level
    {
        const std::string path = ShipPath("SPH", "Ore Scanner");
        auto def = CraftDef(path);
        LT::PreBuiltVesselSituation sit = LandedSituation(&kerbin, path);
        sit.splashed = true;
        bool clampCheckOverride = false;
        LT::PreBuiltSpawnEnvironment::checkLaunchClamps = [&](ShipConstruct&, const LT::PreBuiltVesselSituation&, bool o) {
            clampCheckOverride = o;
            return true;
        };
        LT::PreBuiltSpawnEnvironment::surfaceHeight = [](CelestialBody* b, const Vector3d&) { return b->Radius - 25.0; };
        const LT::PreBuiltSpawnResult r = LT::PreBuiltShipSpawner::ConstructShip(*def, sit);
        REQUIRE_MESSAGE(r.spawned, r.error);
        CHECK(clampCheckOverride);  // overrideSituationCheck: true
        CHECK(r.vessel->Splashed);
        CHECK(r.vesselConfigNode->GetValue("sit") == "SPLASHED");
        CHECK(r.vesselConfigNode->GetValue("splashed") == "True");
        CHECK(r.vesselConfigNode->GetValue("landed") == "False");
        // a surface above sea level cancels the splashed flag
        LT::PreBuiltSpawnEnvironment::surfaceHeight = [](CelestialBody* b, const Vector3d&) { return b->Radius + 25.0; };
        const LT::PreBuiltSpawnResult dry = LT::PreBuiltShipSpawner::ConstructShip(*def, sit);
        REQUIRE_MESSAGE(dry.spawned, dry.error);
        CHECK(dry.vessel->Landed);
        CHECK_FALSE(dry.vessel->Splashed);
        LT::PreBuiltSpawnEnvironment::checkLaunchClamps = nullptr;
    }
    // ---- failures: an unknown part, no PQS
    {
        const std::string path = ShipPath("SPH", "Ore Scanner");
        auto def = CraftDef(path);
        def->craftNode->GetNodes("PART")[0]->SetValue("part", "noSuchPart_4294000000");
        const size_t before = FlightRegistry::Vessels.size();
        const LT::PreBuiltSpawnResult bad = LT::PreBuiltShipSpawner::ConstructShip(*def, LandedSituation(&kerbin, path));
        CHECK_FALSE(bad.spawned);
        CHECK_FALSE(bad.error.empty());
        CHECK(FlightRegistry::Vessels.size() == before);
        auto good = CraftDef(path);
        LT::PreBuiltSpawnEnvironment::surfaceHeight = nullptr;  // the fixture bodies have no PQS: the C# dereferences pqsController
        CHECK_THROWS_AS(LT::PreBuiltShipSpawner::ConstructShip(*good, LandedSituation(&kerbin, path)), NullReferenceException);
    }
    // ---- the host seams: SpawnPreBuiltShip stores the result; Install hooks the contract templates' spawn request
    {
        const std::string path = ShipPath("SPH", "Ore Scanner");
        auto def = CraftDef(path);
        LT::PreBuiltSpawnEnvironment::surfaceHeight = [](CelestialBody* b, const Vector3d&) { return b->Radius + 70.0; };
        int toggles = 0;
        LT::PreBuiltSpawnEnvironment::toggleVesselComponents = [&](Vessel&, bool) { ++toggles; };
        LT::PreBuiltShipSpawner::Install();
        REQUIRE(static_cast<bool>(LT::PreBuiltEnvironment::spawnPreBuiltShip));
        LT::PreBuiltEnvironment::spawnPreBuiltShip(*def, LandedSituation(&kerbin, path));
        CHECK(LT::PreBuiltShipSpawner::last.spawned);
        CHECK(toggles >= 1);  // ToggleVesselComponents(vessel2, false)
        CareerEnvironment::Current().Scheduler().AdvanceFrame(0.02f);
        CareerEnvironment::Current().Scheduler().RunEndOfFrame();
        CHECK(toggles >= 2);  // ReactivateComponents: WaitForEndOfFrame, then ToggleVesselComponents(vessel, true)
        LT::PreBuiltShipSpawner::Uninstall();
        CHECK_FALSE(static_cast<bool>(LT::PreBuiltEnvironment::spawnPreBuiltShip));
    }
}


