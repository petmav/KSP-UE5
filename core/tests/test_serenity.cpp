// WP-53 unit tests: Breaking Ground deployed science and ROCs (src/serenity, src/contracts/serenity_contracts.cpp).
//
// Gate: tools/wp53/serenity_reference.py (numpy, independent of kspcore; tests/data/wp53_reference.json) at 0 ULP for the cluster power state,
// the diminishing-returns table, the experiment science accumulation (Mono float rule over 60 runs x 45 steps), the seismic event maths,
// the ROC seeding of 840 quads and the 40 deterministic ROC placement runs (UnityEngine.Random xorshift128, GetHashCode_Net35, Vector3 maths),
// and the KSPRandom picks of the contracts. The class logic (events, save / load, module chain, contracts) is checked against the C# line by line.
//
// DATA: the GameData mirror holds no SquadExpansion/Serenity (no ROC_DEFINITION, no deployed-science part cfgs, no DEPLOYEDSCIENCE node;
// the part-database capture ran with Serenity=False). The real stock part groundAnchor (ModuleGroundPart + ModuleAnimationGroup) is used for the
// part-level tests; everything Serenity-specific (MODULE values of ModuleGroundExperiment / ExpControl, ROC definitions, quad meshes) is a fixture
// written here from the cfg field names of the decompiled classes. It is NOT game data and no number below claims to be.
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <string>

#include "doctest/doctest.h"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

#include "ksp/career/career_environment.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/serenity_contracts.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/animation_modules.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/science/science.hpp"
#include "ksp/serenity/deployed_science.hpp"
#include "ksp/serenity/ground_modules.hpp"
#include "ksp/serenity/roc.hpp"
#include "ksp/serenity/serenity_misc.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/util/misc_util.hpp"
#include "ksp/vessel/vessel_assembly.hpp"
#include "ksp/modules/vessel_module.hpp"

using namespace ksp;
using namespace ksp::rtest;
using json = nlohmann::json;
using GE = events::GameEvents;
namespace RT = ksp::Expansions::Serenity::DeployedScience::Runtime;
using ksp::Expansions::Serenity::SerenityEnvironment;

namespace {

const std::string kHarness53 = KSPCORE_HARNESS_DIR;

const json& Ref53() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp53_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

uint32_t Bits53(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
uint64_t BitsD53(double d) {
    uint64_t u;
    std::memcpy(&u, &d, 8);
    return u;
}
// 0 ULP: the float / double must be the reference's exactly
#define CHECK_F32(got, ref) CHECK_MESSAGE(Bits53(static_cast<float>(got)) == Bits53(static_cast<float>(ref)), "got ", (got), " want ", (ref))
#define CHECK_F64(got, ref) CHECK_MESSAGE(BitsD53(static_cast<double>(got)) == BitsD53(static_cast<double>(ref)), "got ", (got), " want ", (ref))

// The ROC / deployed science state is process-wide: every test starts and ends clean
struct Clean53 {
    Clean53() { Reset(); }
    ~Clean53() { Reset(); }
    static void Reset() {
        RT::SerenityBehaviour::DestroyAll();
        SerenityEnvironment::Reset();
        RocEnvironment::Reset();
        Expansions::Serenity::SerenityMiscEnvironment::Reset();
        Expansions::Serenity::Contracts::SerenityContractEnvironment::Reset();
        RT::DeployedScience::Instance = nullptr;
        RT::DeployedScience::IsActive = false;
        ROCManager::Instance = nullptr;
        ScienceEnvironment::Reset();
        CareerEnvironment::Reset();
    }
};

// A scenery of one Kerbol system for the tests that need bodies
struct Bodies53 {
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    Bodies53() {
        pg.physicsDatabaseFilename = kHarness53 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        SaveEnvironment::Current().localBodies = sys->SpawnOrder();
        FlightRegistry::Clear();
    }
    ~Bodies53() {
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
    }
    CelestialBody& Body(const char* n) { return *sys->GetBodyByName(n); }
};

void LoadLocalizer53() {
    using Loc = KSP::Localization::Localizer;
    Loc::Shutdown();
    Loc::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(ksp::testing::GameRoot() + "/GameData/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    Loc::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

void InstallSerenity() {
    SaveEnvironment& env = SaveEnvironment::Current();
    env.SetStockExpansionInstalled("Serenity", true);
    env.loadedScene = GameScenes::FLIGHT;
    env.loadedSceneIsGame = true;
}

}  // namespace

// =============================================================================================================================
// DeployedScienceCluster.UpdatePowerState vs the reference

TEST_CASE("WP-53 DeployedScienceCluster.UpdatePowerState vs the reference (600 cases, power units and the multiplier)") {
    Clean53 clean;
    SaveEnvironment::Reset();
    InstallSerenity();
    const json& ref = Ref53()["power"];
    double ut53 = 0.0;
    ScienceEnvironment::universalTime = [&ut53] { return ut53; };
    struct Counter {
        int fired = 0;
        void On(RT::DeployedScienceCluster*) { ++fired; }
    } counter;
    GE::onGroundScienceClusterPowerStateChanged.Add(&counter, &Counter::On);
    Part dummy;
    int powered = 0, partial = 0, half = 0;
    for (const json& c : ref) {
        ConfigNode node("SCIENCECLUSTER");
        node.AddValue("ControlModulePartId", static_cast<uint32_t>(c["controllerId"].get<uint32_t>()));
        node.AddValue("ControlModuleEnabled", c["controllerEnabled"].get<bool>());
        ConfigNode* parts = node.AddNode("MANNEDSCIENCEPARTS");
        for (const json& p : c["parts"]) {
            ConfigNode* pn = parts->AddNode("MANNEDSCIENCEPART");
            pn->AddValue("PartId", p["id"].get<uint32_t>());
            pn->AddValue("Enabled", p["enabled"].get<bool>());
            pn->AddValue("DeployedOnGround", p["onGround"].get<bool>());
            pn->AddValue("PowerUnitsProduced", p["produced"].get<int>());
            pn->AddValue("ActualPowerUnitsProduced", p["actual"].get<int>());
            pn->AddValue("PowerUnitsRequired", p["required"].get<int>());
            pn->AddValue("IsSolarPanel", p["solar"].get<bool>());
            pn->AddValue("IsAntenna", false);
        }
        PersistentIdRegistry& reg = SaveEnvironment::Current().persistentIds;
        for (const json& p : c["parts"])
            if (p["loaded"].get<bool>()) reg.persistentLoadedPartIds.Add(p["id"].get<uint32_t>(), &dummy);
        RT::DeployedScienceCluster* cl = RT::DeployedScienceCluster::SpawnandLoad(node);
        REQUIRE(cl != nullptr);
        cl->isPowered = c["isPowered"].get<bool>();
        cl->controllerPartEnabled = c["controllerEnabled"].get<bool>();
        counter.fired = 0;
        cl->UpdatePowerState();
        const json& e = c["expect"];
        CHECK(cl->PowerRequired() == e["powerRequired"].get<int>());
        CHECK(cl->PowerAvailable() == e["powerAvailable"].get<int>());
        CHECK(cl->PartialPower() == e["partialPower"].get<bool>());
        CHECK_F32(cl->PartialPowerMultiplier(), e["partialMult"].get<float>());
        CHECK(cl->IsPowered() == e["isPowered"].get<bool>());
        CHECK(cl->ControllerPartEnabled() == e["controllerEnabled"].get<bool>());
        CHECK(cl->SolarPanelUnitsProduced() == e["solarUnits"].get<int>());
        CHECK(counter.fired == (e["powerChanged"].get<bool>() ? 1 : 0));
        if (e["isPowered"].get<bool>()) ++powered;
        if (e["partialPower"].get<bool>()) ++partial;
        if (e["partialMult"].get<double>() == 0.5) ++half;
        for (const json& p : c["parts"]) reg.persistentLoadedPartIds.Remove(p["id"].get<uint32_t>());
        cl->DestroyGameObject();
        RT::SerenityBehaviour::FlushDestroyed();
    }
    GE::onGroundScienceClusterPowerStateChanged.Remove(&counter, &Counter::On);
    CHECK(powered > 50);
    CHECK(partial > 50);
    CHECK(half > 5);  // the branch without loaded parts that halves the multiplier
}

// =============================================================================================================================
// DeployedScience: diminishing returns, defaults, OnLoad / OnSave

TEST_CASE("WP-53 DeployedScience.GetDiminishingReturnsMultiplier vs the reference (insertion order matters)") {
    Clean53 clean;
    SaveEnvironment::Reset();
    InstallSerenity();
    for (const json& t : Ref53()["dimret"]) {
        RT::DeployedScience ds;
        ds.Awake();
        // the first table is SetupDiminishingReturns(null): no DIMINISHINGRETURNS node and no DEPLOYEDSCIENCE database node
        const bool isDefault = t["keys"].size() == 5 && t["keys"][0] == 1 && t["values"][1].get<float>() > 0.19f && t["values"][1].get<float>() < 0.21f;
        ConfigNode loadNode("SCENARIO");
        if (!isDefault) {
            ConfigNode* dim = loadNode.AddNode("DIMINISHINGRETURNS");
            for (size_t i = 0; i < t["keys"].size(); i++) {
                ConfigNode* en = dim->AddNode("ENTRY");
                en->AddValue("ExpCount", t["keys"][i].get<int>());
                en->AddValue("Rate", t["values"][i].get<float>());
            }
        }
        ds.OnLoad(loadNode);
        for (int n = 0; n < 9; n++) {
            CHECK_F32(ds.GetDiminishingReturnsMultiplier(n), t["results"][static_cast<size_t>(n)].get<float>());
        }
        CHECK(ds.DiminishingReturns().Count() == static_cast<int>(t["keys"].size()));
    }
}

TEST_CASE("WP-53 DeployedScience defaults: seismic energies, delays, save node") {
    Clean53 clean;
    SaveEnvironment::Reset();
    InstallSerenity();
    RT::DeployedScience ds;
    ds.Awake();
    ConfigNode empty("SCENARIO");
    ds.OnLoad(empty);
    CHECK(RT::DeployedScience::Instance == &ds);
    CHECK(ds.ScienceTimeDelay == 60.0);
    CHECK(ds.DataSendFailedTimeDelay == 600.0);
    CHECK(ds.MinimumSeismicEnergyRequired == 3000.0f);
    CHECK(ds.SeismicScienceProcessingDelay == 3);
    CHECK(ds.SeismicEnergyRates().Count() == 15);
    float e = 0;
    CHECK(ds.SeismicEnergyRates().TryGetValue("Kerbin", e));
    CHECK_F32(e, 4.184E+10f);
    CHECK(ds.SeismicEnergyRates().TryGetValue("Gilly", e));
    CHECK_F32(e, 1000000.0f);
    CHECK(ds.SeismicEnergyRates().KeysList().front() == "Moho");
    CHECK(ds.SeismicEnergyRates().KeysList().back() == "Eeloo");
    CHECK(ds.DiminishingReturns().KeysList() == std::vector<int32_t>({1, 2, 3, 4, 5}));
    // OnSave -> OnLoad -> OnSave is stable
    ConfigNode s1("SCENARIO");
    ds.OnSave(s1);
    RT::DeployedScience ds2;
    RT::DeployedScience::Instance = nullptr;
    ds2.Awake();
    ds2.OnLoad(s1);
    ConfigNode s2("SCENARIO");
    ds2.OnSave(s2);
    CHECK(s1.ToString() == s2.ToString());
    CHECK(s1.GetNode("SEISMICENERGY")->GetNodes("ENTRY").size() == 15);
    CHECK(*s1.GetNode("SEISMICENERGY")->GetValue("MinimumEnergyRequired") == "3000");
}

TEST_CASE("WP-53 DeployedScience without the Serenity expansion destroys itself") {
    Clean53 clean;
    SaveEnvironment::Reset();
    SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", false);
    RT::DeployedScience ds;
    ds.Awake();
    CHECK(ds.destroyed);
    CHECK(RT::DeployedScience::Instance == nullptr);
}

// =============================================================================================================================
// DeployedScienceExperiment.CalculateScience / TimeToSendStoredData vs the reference

namespace {
// A cluster with a controller part, a powered state and one experiment part: the objects CalculateScience reads
struct ExpRig {
    RT::DeployedScienceCluster* cluster = nullptr;
    RT::DeployedSciencePart* part = nullptr;
    RT::DeployedScienceExperiment* exp = nullptr;
    ScienceExperiment sciExp;
    std::shared_ptr<ScienceSubject> subject;
    Vessel vessel;
    ExpRig(float scienceValue, float limit, float subjectValue, float dim, const std::string& expId) {
        ConfigNode cn("SCIENCECLUSTER");
        cn.AddValue("ControlModulePartId", 100u);
        cn.AddValue("ControlModuleEnabled", true);
        cluster = RT::DeployedScienceCluster::SpawnandLoad(cn);
        cluster->isPowered = true;
        part = RT::DeployedSciencePart::Spawn();
        part->partId = 101;
        part->moduleEnabled = true;
        part->deployedOnGround = true;
        part->Cluster = cluster;
        cluster->DeployedScienceParts.push_back(part);
        exp = RT::DeployedScienceExperiment::Spawn();
        exp->partId = 101;
        exp->experimentId = expId;
        exp->experimentName = "ModuleGroundExperiment";
        exp->Cluster = cluster;
        exp->sciencePart = part;
        part->Experiment = exp;
        exp->experimentSituationValid = true;
        exp->scienceValue = scienceValue;
        exp->scienceLimit = limit;
        exp->scienceDiminishingModifierRate = dim;
        exp->Experiment = &sciExp;
        exp->ExperimentVessel = &vessel;
        subject = std::make_shared<ScienceSubject>("id@KerbinSrfLanded", "title", 1.0f, subjectValue, limit);
        exp->subject = subject;
    }
};
}  // namespace

TEST_CASE("WP-53 DeployedScienceExperiment.CalculateScience / TimeToSendStoredData vs the reference (60 runs x 45 steps, Mono float rule)") {
    Clean53 clean;
    SaveEnvironment::Reset();
    InstallSerenity();
    double ut = 0.0;
    ScienceEnvironment::universalTime = [&ut] { return ut; };
    int generatedSteps = 0, disabled = 0, sends = 0;
    for (const json& run : Ref53()["science"]) {
        ExpRig rig(run["scienceValue"].get<float>(), run["limit"].get<float>(), run["subjectValue"].get<float>(), run["dim"].get<float>(), "someExperiment");
        ut = run["startUT"].get<double>();
        rig.exp->LastScienceGeneratedUT = ut;
        float lastGenerated = -1.0f;
        struct L {
            float* out;
            void On(RT::DeployedScienceExperiment*, RT::DeployedSciencePart*, RT::DeployedScienceCluster*, float n) { *out = n; }
        } l{&lastGenerated};
        GE::onGroundScienceGenerated.Add(&l, &L::On);
        for (const json& s : run["steps"]) {
            ut = s["ut"].get<double>();
            lastGenerated = -1.0f;
            rig.exp->CalculateScience(s["solar"].get<float>());
            if (s["generated"].is_null()) {
                CHECK(lastGenerated == -1.0f);
            } else {
                CHECK_F32(lastGenerated, s["generated"].get<float>());
                ++generatedSteps;
            }
            CHECK_F32(rig.exp->TotalScienceGenerated(), s["total"].get<float>());
            CHECK_F32(rig.exp->StoredScienceData(), s["stored"].get<float>());
            CHECK(rig.part->Enabled() == s["enabled"].get<bool>());
            CHECK(rig.exp->TimeToSendStoredData() == s["send"].get<bool>());
            if (!s["enabled"].get<bool>()) ++disabled;
            if (s["send"].get<bool>()) ++sends;
            if (s["transmit"].get<bool>()) rig.exp->storedScienceData = 0.0f;
            CHECK_F64(rig.exp->LastScienceGeneratedUT, s["ut"].get<double>());
        }
        GE::onGroundScienceGenerated.Remove(&l, &L::On);
        rig.cluster->DestroyGameObject();
        rig.exp->DestroyGameObject();
        rig.part->DestroyGameObject();
        RT::SerenityBehaviour::FlushDestroyed();
    }
    CHECK(generatedSteps > 500);
    CHECK(disabled > 100);  // runs that hit the science cap
    CHECK(sends > 100);
}

// =============================================================================================================================
// DeployedScienceExperiment.ProcessSeismicEvent vs the reference

TEST_CASE("WP-53 DeployedScienceExperiment.ProcessSeismicEvent maths vs the reference (160 events)") {
    Clean53 clean;
    SaveEnvironment::Reset();
    InstallSerenity();
    Bodies53 bodies;
    CelestialBody* kerbin = &bodies.Body("Kerbin");
    CareerEnvironment::Current().getHomeBody = [kerbin] { return kerbin; };
    LoadLocalizer53();
    double ut = 0.0;
    ScienceEnvironment::universalTime = [&ut] { return ut; };
    int ignored = 0, processed = 0, capped = 0;
    for (const json& c : Ref53()["seismic"]) {
        RT::DeployedScience ds;
        RT::DeployedScience::Instance = nullptr;
        ds.Awake();
        ConfigNode load("SCENARIO");
        ConfigNode* se = load.AddNode("SEISMICENERGY");
        se->AddValue("MinimumEnergyRequired", c["minEnergy"].get<float>());
        se->AddValue("SeismicScienceProcessingDelay", 3);
        ConfigNode* en = se->AddNode("ENTRY");
        en->AddValue("BodyName", "Kerbin");
        en->AddValue("Energy", c["rate"].get<float>());
        ds.OnLoad(load);
        ExpRig rig(0.0f, c["limit"].get<float>(), c["subjectValue"].get<float>(), c["dim"].get<float>(), RT::DeployedScience::SeismicExperimentId);
        rig.cluster->DeployedBody = kerbin;
        rig.subject->dataScale = c["dataScale"].get<float>();
        rig.exp->scienceModifierRate = c["rateMod"].get<float>();
        rig.exp->solarMultiplier = c["solar"].get<float>();
        rig.exp->totalScienceGenerated = c["total0"].get<float>();
        rig.exp->storedScienceData = c["stored0"].get<float>();
        for (const json& p : c["parts"]) {
            auto d = std::make_shared<RT::DeployedScienceExperiment::SeismicPartData>();
            d->mass = p["mass"].get<float>();
            d->velocity = p["velocity"].get<float>();
            d->impactDistance = p["impactDistance"].get<double>();
            d->impactDistanceMultiplier = p["mult"].get<float>();
            rig.exp->seismicPartList.push_back(d);
        }
        rig.exp->seismicEventDataSendPending = true;
        float gotNum3 = -1.0f, gotStored = -1.0f, gotTotal = -1.0f;
        struct L {
            float *n, *s, *t;
            void On(RT::DeployedScienceExperiment* e, RT::DeployedSciencePart*, RT::DeployedScienceCluster*, float num) {
                *n = num;
                *s = e->StoredScienceData();
                *t = e->TotalScienceGenerated();
            }
        } l{&gotNum3, &gotStored, &gotTotal};
        GE::onGroundScienceGenerated.Add(&l, &L::On);
        rig.exp->ProcessSeismicEvent();
        GE::onGroundScienceGenerated.Remove(&l, &L::On);
        if (c["ignored"].get<bool>()) {
            ++ignored;
            CHECK(gotNum3 == -1.0f);
            CHECK(rig.exp->seismicPartList.size() == c["parts"].size());  // nothing removed
        } else {
            ++processed;
            CHECK_F32(gotNum3, c["num3"].get<float>());
            CHECK_F32(gotTotal, c["total"].get<float>());
            CHECK_F32(gotStored, c["stored"].get<float>());
            CHECK_F32(rig.exp->TotalScienceGenerated(), c["total"].get<float>());
            CHECK_F32(rig.exp->StoredScienceData(), 0.0f);  // SendDataToComms transmitted it
            CHECK(rig.exp->seismicPartList.empty());
            CHECK(!rig.exp->SeismicEventDataSendPending());
            if (gotTotal == c["limit"].get<float>()) ++capped;
        }
        rig.cluster->DestroyGameObject();
        rig.exp->DestroyGameObject();
        rig.part->DestroyGameObject();
        RT::SerenityBehaviour::FlushDestroyed();
        RT::DeployedScience::Instance = nullptr;
    }
    CHECK(ignored > 10);
    CHECK(processed > 50);
    CHECK(capped > 3);
}

// =============================================================================================================================
// ROC seeding and placement vs the reference

namespace {
struct RocRig {
    ROCManager mgr;
    ROCDefinition def;
    PQSROCControl ctl;
    std::unique_ptr<LandClassROC> lc;
    Game game;
    RocRig(const std::string& rocName, const std::string& objName, float freq, float area, int gameSeed, bool surfaceRelative, bool randomDepth, float depth) {
        InstallSerenity();
        ROCManager::Instance = &mgr;
        def.type = rocName;
        def.frequency = freq;
        def.depth = depth;
        def.randomDepth = randomDepth;
        def.orientateUp = true;
        def.scale = 1.0f;
        auto roc = std::make_shared<ROC>();
        roc->name = objName;
        mgr.rocTypeObjects.Add(rocName, roc);
        ctl.currentCBName = "Kerbin";
        ctl.quadArea = area;
        game.Mode = Game_Modes::MISSION;
        SaveEnvironment::Current().currentGame = &game;
        RocEnvironment::resourceScenarioPresent = true;
        RocEnvironment::rocMissionSeed = [gameSeed] { return gameSeed; };
        lc = std::make_unique<LandClassROC>(&def, &ctl);
        RocPqsView view;
        view.maxLevel = 1;
        view.surfaceRelativeQuads = surfaceRelative;
        lc->Setup(view);
    }
    ~RocRig() {
        lc.reset();
        SaveEnvironment::Current().currentGame = nullptr;
        ROCManager::Instance = nullptr;
    }
};
}  // namespace

TEST_CASE("WP-53 LandClassROC.AddScatterMeshController seeding vs the reference (840 quads)") {
    Clean53 clean;
    SaveEnvironment::Reset();
    int n = 0, ones = 0, multi = 0, none = 0;
    for (const json& c : Ref53()["rocSeeds"]) {
        RocRig rig(c["roc"].get<std::string>(), c["roc"].get<std::string>() + "(Clone)", c["freq"].get<float>(), c["area"].get<float>(), c["gameSeed"].get<int>(), false,
                   false, 0.5f);
        RocQuad quad;
        quad.name = c["quad"].get<std::string>();
        rig.lc->AddScatterMeshController(&quad, 1);
        const json& e = c["expect"];
        CHECK(rig.lc->gameSeed == c["gameSeed"].get<int>());
        CHECK(rig.lc->quadSeed == e["quadSeed"].get<int>());
        CHECK(rig.lc->rocN == e["rocN"].get<int>());
        CHECK(rig.lc->maxROCs == e["maxROCs"].get<int>());
        if (rig.lc->rocN >= 1) {
            REQUIRE(rig.lc->lastAssigned != nullptr);
            CHECK(rig.lc->lastAssigned->count == e["rocN"].get<int>());
            CHECK(rig.lc->lastAssigned->quadSeed == e["quadSeed"].get<int>());
            CHECK(rig.lc->cacheAssignedCount == 1);
        } else {
            CHECK(rig.lc->cacheAssignedCount == 0);
        }
        ++n;
        if (e["rocN"].get<int>() == 1) ++ones;
        if (e["rocN"].get<int>() > 1) ++multi;
        if (e["rocN"].get<int>() < 1) ++none;
    }
    CHECK(n == 840);
    CHECK(ones > 100);
    CHECK(multi > 100);
    CHECK(none > 50);
}

TEST_CASE("WP-53 LandClassROC.CreateScatterMesh placements vs the reference (40 quads: positions, ids, duplicate suppression)") {
    Clean53 clean;
    SaveEnvironment::Reset();
    int total = 0, dups = 0;
    for (const json& c : Ref53()["rocPlace"]) {
        RocRig rig(c["roc"].get<std::string>(), c["obj"].get<std::string>(), c["freq"].get<float>(), c["area"].get<float>(), c["gameSeed"].get<int>(),
                   c["surfaceRelative"].get<bool>(), c["randomDepth"].get<bool>(), c["depth"].get<float>());
        const float sizeY = c["sizeY"].get<float>(), extentsY = c["extentsY"].get<float>();
        RocEnvironment::rendererBoundsY = [=](const std::string&) { return std::make_pair(sizeY, extentsY); };
        RocQuad quad;
        quad.name = c["quad"].get<std::string>();
        for (const json& v : c["verts"]) {
            quad.verts.push_back(Vector3f(v[0].get<float>(), v[1].get<float>(), v[2].get<float>()));
            quad.vertNormals.push_back(Vector3f(0.0f, 1.0f, 0.0f));
        }
        const json& pp = c["positionPlanet"];
        quad.positionPlanet = Vector3f(pp[0].get<float>(), pp[1].get<float>(), pp[2].get<float>());
        // pre-existing dedupe entries
        if (!c["usedIn"].empty()) {
            auto list = std::make_shared<std::vector<std::shared_ptr<RocPositionInfo>>>();
            for (const json& u : c["usedIn"]) {
                auto info = std::make_shared<RocPositionInfo>();
                info->rocType = "other";
                info->position = Vector3f(u[0].get<float>(), u[1].get<float>(), u[2].get<float>());
                list->push_back(info);
            }
            rig.ctl.rocPositionsUsed.Add(StringExtensions::GetHashCode_Net35(quad.name), list);
        }
        rig.lc->AddScatterMeshController(&quad, 1);
        const json& e = c["expect"];
        REQUIRE(rig.lc->rocN == e["seed"]["rocN"].get<int>());
        std::vector<RocPlacement> placements;
        if (rig.lc->rocN >= 1) {
            REQUIRE(rig.lc->lastAssigned != nullptr);
            placements = rig.lc->CreateScatterMesh(rig.lc->lastAssigned);
            CHECK(rig.lc->lastAssigned->isBuilt);
            CHECK(rig.lc->lastAssigned->active);
        }
        size_t k = 0;
        for (const json& r : e["recs"]) {
            ++total;
            if (r["duplicate"].get<bool>()) {
                ++dups;
                continue;
            }
            REQUIRE(k < placements.size());
            const RocPlacement& pl = placements[k++];
            CHECK(pl.rocID == r["rocID"].get<int>());
            CHECK_F32(pl.localPosition.x, r["localPosition"][0].get<float>());
            CHECK_F32(pl.localPosition.y, r["localPosition"][1].get<float>());
            CHECK_F32(pl.localPosition.z, r["localPosition"][2].get<float>());
            CHECK_F32(pl.scale, 1.0f);
            // orientateUp, no random rotation / orientation: AngleAxis(0, up) * quadRotation == quadRotation (identity here)
            CHECK_F32(pl.localRotation.x, 0.0f);
            CHECK_F32(pl.localRotation.w, 1.0f);
        }
        CHECK(k == placements.size());
        // the dedupe list holds the reference's positions in order
        const RocPositionList used = rig.ctl.rocPositionsUsed[StringExtensions::GetHashCode_Net35(quad.name)];
        REQUIRE(used->size() == e["used"].size());
        for (size_t i = 0; i < used->size(); i++) {
            CHECK_F32((*used)[i]->position.x, e["used"][i][0].get<float>());
            CHECK_F32((*used)[i]->position.y, e["used"][i][1].get<float>());
            CHECK_F32((*used)[i]->position.z, e["used"][i][2].get<float>());
        }
    }
    CHECK(total > 100);
    CHECK(dups >= 3);
}
