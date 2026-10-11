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
#include "ksp/career/career.hpp"
#include "ksp/contracts/contracts.hpp"
#include "ksp/contracts/serenity_contracts.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/animation_modules.hpp"
#include "ksp/modules/science_modules.hpp"
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

// =============================================================================================================================
// The ground-part modules on a real vessel (World with the stock database, the Kerbol system, the WP-00 coroutine scheduler)

namespace {
struct WorldSer {
    GameData data;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    CoroutineScheduler sched;
    double ut = 100.0;
    std::vector<std::unique_ptr<Vessel>> vessels;
    WorldSer() {
        SaveEnvironment::Reset();
        FlightRegistry::Clear();
        Vessel::SpawnedVessels().clear();
        GameData::Options o;
        o.gameRoot = ksp::testing::GameRoot();
        data.Load(o);
        SaveEnvironment& env = SaveEnvironment::Current();
        env.getPartInfoByName = [this](std::string_view n) { return data.parts.getPartInfoByName(n); };
        env.partExists = [this](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
        env.newGuidHashCode = [] {
            static uint32_t s = 4242u;
            s = s * 1664525u + 1013904223u;
            return static_cast<int32_t>(s);
        };
        support::InstallUpgradeHooks(data);
        pg.physicsDatabaseFilename = ksp::testing::GameRoot() + "/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        Vessel::env = VesselEnvironment{};
        VesselModuleManager::CompileModules();
        InstallSerenity();
        ResourceSetup::database = &data.database;
        ResourceSetup::ResetConfig();
        LoadLocalizer53();
        ScienceEnvironment::getConfigNodes = [this](std::string_view t) { return data.database.GetConfigNodes(t); };
        ScienceEnvironment::loadedParts = [this] {
            std::vector<const AvailablePart*> v;
            for (const auto& p : data.parts.loadedParts) v.push_back(p.get());
            return v;
        };
        ScienceEnvironment::universalTime = [this] { return ut; };
        ScienceEnvironment::scheduler = &sched;
        CelestialBody* k = &Kerbin();
        CareerEnvironment::Current().getHomeBody = [k] { return k; };
    }
    ~WorldSer() {
        RT::SerenityBehaviour::DestroyAll();
        vessels.clear();
        Vessel::SpawnedVessels().clear();
        FlightRegistry::Clear();
        FlightRegistry::ActiveVessel = nullptr;
        ScienceEnvironment::scheduler = nullptr;
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
        ResourceSetup::database = nullptr;
        ResourceSetup::ResetConfig();
        SaveEnvironment::Reset();
        Vessel::env = VesselEnvironment{};
    }
    CelestialBody& Kerbin() { return *sys->GetBodyByName("Kerbin"); }
    Vector3f PlaceKerbinAtPad() {
        CelestialBody& k = Kerbin();
        k.position = Vector3d();
        const Vector3d pad = k.GetWorldSurfacePosition(-0.0972077486779459, -74.5576813534722, 75.0);
        k.position = Vector3d() - pad;
        return Vector3f();
    }
    // One landed single-part vessel; `modules` are added to the part after its prefab modules (Part.AddModule(ConfigNode): Awake + Load)
    struct ModDef {
        std::string name;
        std::vector<std::pair<std::string, std::string>> kv;
    };
    Vessel* Mk(const char* partName, uint32_t pid, const std::vector<ModDef>& modules, Vector3f offset = Vector3f()) {
        const AvailablePart* ap = data.parts.getPartInfoByName(partName);
        REQUIRE_MESSAGE(ap != nullptr, partName);
        auto p = Part::Instantiate(*ap->partPrefab);
        p->partInfo = ap;
        p->gameObjectName = ap->name;
        p->fields.flightID = pid + 1;
        p->fields.persistentId = pid;
        p->UpdateMass();
        for (const ModDef& m : modules) {
            ConfigNode node("MODULE");
            node.AddValue("name", m.name);
            for (const auto& kv : m.kv) node.AddValue(kv.first, kv.second);
            REQUIRE_MESSAGE(p->AddModule(node, true) != nullptr, m.name);
        }
        std::vector<std::unique_ptr<Part>> owned;
        Part* root = p.get();
        owned.push_back(std::move(p));
        auto v = std::make_unique<Vessel>();
        root->partTransform.position = PlaceKerbinAtPad() + offset;
        v->vesselTransform = root->partTransform;
        v->AdoptParts(std::move(owned), root);
        v->vesselName = std::string("Vessel ") + std::to_string(pid);
        REQUIRE(v->Initialize(true, false, false, false));
        v->loaded = true;
        FlightRegistry::AddVessel(v.get());
        v->packed = false;
        for (Part* q : v->parts) q->fields.packed = false;
        SaveEnvironment::Current().persistentIds.persistentLoadedPartIds.Add(pid, root);
        vessels.push_back(std::move(v));
        return vessels.back().get();
    }
    template <class T>
    static T* Mod(Part& p, const char* cls) {
        T* t = dynamic_cast<T*>(p.FindModuleOfClass(cls));
        REQUIRE(t != nullptr);
        return t;
    }
    // frames of dt seconds on the coroutine scheduler
    void Frames(int n, float dt = 1.0f) {
        for (int i = 0; i < n; i++) {
            sched.AdvanceFrame(dt);
            sched.RunDelayedFixedFrameRate();
            sched.RunDelayedDynamicFrameRate();
            sched.RunEndOfFrame();
        }
    }
};
}  // namespace

TEST_CASE("WP-53 module chain: the five ground modules are registered, the schema defaults load, ModuleGroundPart on the stock groundAnchor") {
    Clean53 clean;
    WorldSer w;
    const AvailablePart* ap = w.data.parts.getPartInfoByName("groundAnchor");
    REQUIRE(ap != nullptr);
    auto* gp = dynamic_cast<ModuleGroundPart*>(ap->partPrefab->FindModuleOfClass("ModuleGroundPart"));
    REQUIRE(gp != nullptr);
    // the real cfg values (GameData/Squad/Parts/Cargo/GroundAnchor/groundAnchor.cfg)
    CHECK(gp->Get<float>("packedVolume") == 40.0f);
    CHECK(gp->Get<int32_t>("stackableQuantity") == 1);
    CHECK(gp->Get<float>("kinematicDelay") == 0.0f);
    CHECK(gp->Get<std::string>("fxGroupDeploy") == "deploy");
    CHECK(gp->placementAllowXRotation());  // C# initialiser true
    CHECK(gp->AsAnimatedModule() == gp);
    CHECK(gp->GetModuleDisplayName() == KSP::Localization::Localizer::Format("#autoLOC_6012033"));
    CHECK(gp->Events()["RetrievePart"] != nullptr);
    CHECK(!gp->IsDeployedSciencePart());  // no ModuleGroundSciencePart on the anchor
    // groundLight1: ModuleGroundPart with packedVolume 20 / stackableQuantity 2
    auto* gl = dynamic_cast<ModuleGroundPart*>(w.data.parts.getPartInfoByName("groundLight1")->partPrefab->FindModuleOfClass("ModuleGroundPart"));
    REQUIRE(gl != nullptr);
    CHECK(gl->Get<float>("packedVolume") == 20.0f);
    CHECK(gl->Get<int32_t>("stackableQuantity") == 2);

    // the subclass chain, with the fixture MODULE nodes
    Vessel* v = w.Mk("mk1pod.v2", 7001, {{"ModuleGroundExpControl", {}}, {"ModuleGroundExperiment", {{"experimentId", "temperatureScan"}}},
                                           {"ModuleGroundCommsPart", {}}, {"ModuleGroundSciencePart", {}}});
    Part& p = *v->parts[0];
    auto* ctrl = WorldSer::Mod<ModuleGroundExpControl>(p, "ModuleGroundExpControl");
    auto* exp = WorldSer::Mod<ModuleGroundExperiment>(p, "ModuleGroundExperiment");
    auto* comms = WorldSer::Mod<ModuleGroundCommsPart>(p, "ModuleGroundCommsPart");
    auto* sci = WorldSer::Mod<ModuleGroundSciencePart>(p, "ModuleGroundSciencePart");
    CHECK(dynamic_cast<ModuleGroundCommsPart*>(ctrl) != nullptr);
    CHECK(dynamic_cast<ModuleGroundSciencePart*>(exp) != nullptr);
    CHECK(dynamic_cast<ModuleGroundPart*>(comms) != nullptr);
    CHECK(dynamic_cast<ModuleCargoPart*>(sci) != nullptr);
    // schema defaults: ExpControl.OnAwake DistanceToController = 0, Science.OnAwake float.MaxValue; the curves
    CHECK_F32(ctrl->DistanceToController(), 0.0f);
    CHECK_F32(exp->DistanceToController(), 3.4028234663852886e+38f);
    CHECK(ctrl->controlUnitRange() == 20);
    CHECK_F64(ctrl->antennaPower(), 500000.0);
    CHECK_F64(ctrl->CommPower(), 500000.0);  // antennaPower + boostedAntennaPower (0)
    CHECK(ctrl->antennaCombinableExponent() == 0.75);
    const DoubleCurve* r = ctrl->CommRangeCurve();
    REQUIRE(r != nullptr);
    CHECK(r->keys.size() == 2);
    const DoubleCurve* sc = ctrl->CommScienceCurve();
    REQUIRE(sc != nullptr);
    CHECK(sc->keys.size() == 4);
    CHECK_F32(exp->ScienceModifierRate(), 5.0f);
    CHECK(exp->Enabled());  // moduleEnabled initialiser true
    CHECK(!exp->DeployedOnGround());
    CHECK(exp->experimentId() == "temperatureScan");
    CHECK(ctrl->antennaType() == AntennaType::DIRECT);
}

TEST_CASE("WP-53 ModuleGroundPart: deploy on the ground (MakePartKinematic) and retrieve into the EVA kerbal's inventory, stock groundAnchor") {
    Clean53 clean;
    WorldSer w;
    Vessel* v = w.Mk("groundAnchor", 7101, {});
    Part& p = *v->parts[0];
    auto* gp = WorldSer::Mod<ModuleGroundPart>(p, "ModuleGroundPart");
    auto* ag = WorldSer::Mod<ModuleAnimationGroup>(p, "ModuleAnimationGroup");
    gp->OnStart(StartState::Landed);
    CHECK(gp->animationGroup == ag);
    CHECK(gp->groundScienceModule == nullptr);
    // the deploy / retract animations belong to the part model (DeployablesEnvironment::modelAnimations): the group is switched off like the
    // code does for DroppedPart vessels, so PlayDeployAnimation / PlayRetractAnimation skip the engine side
    // (OnPartUnpack re-enables a group of a part being deployed: ModuleGroundPart.cs:162)
    CHECK(gp->ModuleIsActive());  // isEnabled
    // deploy: a part being deployed settles for 3 s (kinematicDelay < 1 is raised to 3), then rivets to the ground
    gp->Set<bool>("beingDeployed", true);
    v->vesselType = VesselType::Unknown;
    SerenityEnvironment::srfVelocityMagnitude = [](Vessel&) { return 0.0; };
    gp->OnPartUnpack();
    CHECK(ag->isEnabled);  // ModuleGroundPart.cs:164
    ag->isEnabled = false;
    CHECK_F32(gp->kinematicDelay(), 3.0f);
    CHECK(!gp->deployedOnGround());
    w.Frames(2);
    CHECK(!gp->deployedOnGround());  // still waiting for the 3 s
    w.Frames(4);
    CHECK(gp->deployedOnGround());
    CHECK(!gp->beingDeployed());
    CHECK(p.fields.PermanentGroundContact);
    CHECK(p.fields.autoStrutMode == static_cast<int32_t>(Part_AutoStrutMode::ForceGrandparent));
    CHECK(v->vesselType == VesselType::DeployedGroundPart);
    CHECK(gp->hasJoint);
    CHECK(std::count(SerenityEnvironment::physicsActions.begin(), SerenityEnvironment::physicsActions.end(), "constraints=FreezeAll") == 1);

    // the part cannot deploy off the ground
    Vessel* v2 = w.Mk("groundAnchor", 7103, {});
    Part& p2 = *v2->parts[0];
    auto* gp2 = WorldSer::Mod<ModuleGroundPart>(p2, "ModuleGroundPart");
    gp2->OnStart(StartState::Landed);
    gp2->Set<bool>("beingDeployed", true);
    SerenityEnvironment::groundContact = [](Part&) { return false; };
    SerenityEnvironment::screenMessages.clear();
    gp2->OnPartUnpack();
    w.Frames(6);
    CHECK(!gp2->deployedOnGround());
    CHECK(gp2->beingDeployed());
    REQUIRE(SerenityEnvironment::screenMessages.size() == 1);
    CHECK(SerenityEnvironment::screenMessages[0] == KSP::Localization::Localizer::Format("#autoLOC_8002326"));
    CHECK(gp2->isEnabled);  // EnableModule()
    SerenityEnvironment::groundContact = nullptr;

    // retrieve: needs an EVA kerbal with a free inventory slot
    Vessel* eva = w.Mk("mk1pod.v2", 7102, {});
    eva->vesselType = VesselType::EVA;
    FlightRegistry::ActiveVessel = eva;
    auto inv = std::make_shared<SerenityEnvironment::KerbalInventory>();
    int storedSlot = -99;
    inv->hasCapacity = [](Part&) { return true; };
    inv->firstEmptySlot = [] { return 2; };
    inv->storeCargoPartAtSlot = [&storedSlot](Part&, int slot) {
        storedSlot = slot;
        return true;
    };
    SerenityEnvironment::activeKerbalInventory = inv;
    gp->OnVesselChange(nullptr);
    CHECK(gp->kerbalInventoryModule == inv.get());
    gp->UpdateGroundModuleUI();
    CHECK(gp->Events()["RetrievePart"]->active);
    inv->firstEmptySlot = [] { return -1; };
    gp->UpdateGroundModuleUI();
    CHECK(!gp->Events()["RetrievePart"]->active);  // a full inventory hides the event
    inv->firstEmptySlot = [] { return 2; };
    inv->hasCapacity = [](Part&) { return false; };
    SerenityEnvironment::screenMessages.clear();
    gp->RetrievePart();  // no capacity: ScreenMessages, nothing else
    REQUIRE(SerenityEnvironment::screenMessages.size() == 1);
    CHECK(!gp->beingRetrieved);
    inv->hasCapacity = [](Part&) { return true; };
    gp->RetrievePart();
    CHECK(gp->beingRetrieved);
    CHECK(storedSlot == -1);
    CHECK(!gp->deployedOnGround());
    CHECK(v->vesselType == VesselType::DeployedSciencePart);
    // no animation to wait for: OnRetractCompleted ran at once and the part left the world
    CHECK(!inv->partBeingRetrieved);
    CHECK(v->state == Vessel::State::DEAD);
    FlightRegistry::ActiveVessel = nullptr;
}

TEST_CASE("WP-53 deployed science end to end: scan, cluster registration, power, science generation, transmission to R&D, save / load, retrieval") {
    Clean53 clean;
    WorldSer w;
    ResearchAndDevelopment rd;
    rd.Awake();
    ConfigNode rdEmpty("ResearchAndDevelopment");
    rd.OnLoad(rdEmpty);
    RT::DeployedScience ds;
    ds.Awake();
    ConfigNode empty("SCENARIO");
    ds.OnLoad(empty);
    REQUIRE(RT::DeployedScience::Instance == &ds);

    Vessel* A = w.Mk("mk1pod.v2", 7201, {{"ModuleGroundExpControl", {{"deployedOnGround", "True"}, {"powerUnitsProduced", "6"}, {"powerUnitsRequired", "1"}}}});
    Vessel* B = w.Mk("mk1pod.v2", 7202,
                     {{"ModuleGroundExperiment", {{"experimentId", "temperatureScan"}, {"deployedOnGround", "True"}, {"powerUnitsRequired", "2"}, {"ScienceModifierRate", "100"}}}},
                     Vector3f(5.0f, 0.0f, 0.0f));
    Vessel* C = w.Mk("mk1pod.v2", 7203,
                     {{"ModuleGroundExperiment", {{"experimentId", "temperatureScan"}, {"deployedOnGround", "True"}, {"powerUnitsRequired", "1"}}}},
                     Vector3f(30.0f, 0.0f, 0.0f));  // out of range (controlUnitRange 20)
    auto* ctrl = WorldSer::Mod<ModuleGroundExpControl>(*A->parts[0], "ModuleGroundExpControl");
    auto* expB = WorldSer::Mod<ModuleGroundExperiment>(*B->parts[0], "ModuleGroundExperiment");
    auto* expC = WorldSer::Mod<ModuleGroundExperiment>(*C->parts[0], "ModuleGroundExperiment");
    struct Counts {
        int registered = 0, updated = 0, powerChanged = 0, transmitted = 0, generated = 0;
        void OnReg(ModuleGroundExpControl*, RT::DeployedScienceCluster*) { ++registered; }
        void OnUpd(ModuleGroundExpControl*, RT::DeployedScienceCluster*) { ++updated; }
        void OnPow(RT::DeployedScienceCluster*) { ++powerChanged; }
        void OnTx(RT::DeployedScienceExperiment*, RT::DeployedSciencePart*, RT::DeployedScienceCluster*, float) { ++transmitted; }
        void OnGen(RT::DeployedScienceExperiment*, RT::DeployedSciencePart*, RT::DeployedScienceCluster*, float) { ++generated; }
    } n;
    GE::onGroundScienceClusterRegistered.Add(&n, &Counts::OnReg);
    GE::onGroundScienceClusterUpdated.Add(&n, &Counts::OnUpd);
    GE::onGroundScienceClusterPowerStateChanged.Add(&n, &Counts::OnPow);
    GE::onGroundScienceTransmitted.Add(&n, &Counts::OnTx);
    GE::onGroundScienceGenerated.Add(&n, &Counts::OnGen);

    ctrl->Start();
    expB->Start();
    expC->Start();
    expB->OnStart(StartState::Landed);  // not a controller: the vessel becomes a DeployedScienceController until a controller claims the part
    expC->OnStart(StartState::Landed);
    CHECK(B->vesselType == VesselType::DeployedScienceController);
    ctrl->OnStart(StartState::Landed);
    CHECK(ctrl->ControlUnitId() == 7201u);
    w.Frames(8);  // WaitForSeconds(2) + one frame per loaded part, then RegisterCluster
    REQUIRE(ds.DeployedScienceClusters().Count() == 1);
    RT::DeployedScienceCluster* cl = ds.DeployedScienceClusters()[7201u];
    REQUIRE(cl != nullptr);
    CHECK(cl->DeployedBody == &w.Kerbin());
    CHECK(n.registered == 1);
    CHECK(n.updated == 1);
    CHECK(expB->ControlUnitId() == 7201u);   // in range: connected
    CHECK(expC->ControlUnitId() == 0u);      // 30 m > controlUnitRange 20
    CHECK(B->vesselType == VesselType::DeployedSciencePart);
    REQUIRE(cl->DeployedScienceParts.size() == 2);
    CHECK(cl->DeployedScienceParts[0]->PartId() == 7201u);
    CHECK(cl->DeployedScienceParts[1]->PartId() == 7202u);
    // power: the controller produces 6 units, needs 1; the experiment needs 2
    CHECK(cl->PowerAvailable() == 6);
    CHECK(cl->PowerRequired() == 3);
    CHECK(cl->IsPowered());
    CHECK(!cl->PartialPower());
    CHECK(ctrl->GetControlSourceState() == VesselControlState::ProbeFull);
    CHECK(ctrl->IsCommCapable());
    CHECK(ctrl->CanComm());
    // OnSave of the controller module records canComm in flight
    ConfigNode ctrlSave("MODULE");
    ctrl->OnSave(ctrlSave);
    CHECK(*ctrlSave.GetValue("canComm") == "True");

    // the experiment component starts (next frame): R&D subject, science value / limit
    RT::SerenityBehaviour::RunPendingStarts();
    RT::DeployedScienceExperiment* exp = cl->DeployedScienceParts[1]->Experiment;
    REQUIRE(exp != nullptr);
    REQUIRE(exp->Experiment != nullptr);
    CHECK(*exp->Experiment->id == "temperatureScan");
    REQUIRE(exp->subject != nullptr);
    CHECK(exp->subject->id == "temperatureScan@KerbinSrfLanded");
    CHECK(exp->ExperimentSituationValid());
    const float expectSv = f32store(fx(exp->Experiment->baseValue) * fx(exp->Experiment->dataScale) * fx(exp->subject->subjectValue) * (fx(100.0f) / 100.0));
    CHECK_F32(exp->ScienceValue(), expectSv);
    CHECK_F32(exp->ScienceLimit(), exp->subject->scienceCap);
    CHECK_F32(expB->ScienceValue(), expectSv);   // pushed into the module (event)
    CHECK_F32(expB->ScienceLimit(), exp->subject->scienceCap);
    CHECK_F32(exp->ScienceDiminishingModifierRate(), 1.0f);

    // toggling the experiment part off drops its power requirement, on restores it (module -> events -> cluster)
    expB->ToggleActive();
    CHECK(!expB->Enabled());
    CHECK(cl->PowerRequired() == 1);
    CHECK(!cl->DeployedScienceParts[1]->Enabled());
    expB->ToggleActive();
    CHECK(expB->Enabled());
    CHECK(cl->PowerRequired() == 3);

    // time passes: DeployedScience.FixedUpdate starts the cluster's UpdateScience after ScienceTimeDelay (60 s)
    // (the toggles above already ran CalcScience once with dt = 0: that is the C# behaviour, an event with num = 0)
    const int gen0 = n.generated;
    const float scienceBefore = rd.Science();
    w.ut = 100.0 + 30.0;
    ds.FixedUpdate();
    w.Frames(2);
    CHECK(exp->TotalScienceGenerated() == 0.0f);  // 30 s < 60 s: no cycle
    CHECK(n.generated == gen0);
    w.ut = 100.0 + 3600.0;
    ds.FixedUpdate();
    w.Frames(3);
    CHECK(n.generated == gen0 + 1);
    const float generated = f32store(fx(expectSv) / 60.0 / 60.0 * fx(f32store(3600.0)) * fx(1.0f) * fx(1.0f));
    CHECK_F32(exp->TotalScienceGenerated(), generated);
    CHECK_F64(cl->LastScienceGeneratedUT(), w.ut);
    CHECK(!cl->UpdatingScience());
    // TimeToSendStoredData: 10 % of the science limit stored sends the data to R&D at the end of the same cycle
    const float storedNow = f32store(fx(generated) / fx(exp->subject->subjectValue));
    const bool sentAtOnce = fx(storedNow) * fx(exp->subject->subjectValue) / fx(exp->ScienceLimit()) * 100.0 >= 10.0;
    if (sentAtOnce) {
        CHECK(n.transmitted == 1);
        CHECK(exp->StoredScienceData() == 0.0f);
        CHECK(rd.Science() > scienceBefore);
    } else {
        CHECK(n.transmitted == 0);
        CHECK_F32(exp->StoredScienceData(), storedNow);
    }
    // later cycles accumulate until the science limit: the experiment part switches itself off
    for (int i = 0; i < 80 && exp->TotalScienceGenerated() < exp->ScienceLimit(); i++) {
        w.ut += 7200.0;
        ds.FixedUpdate();
        w.Frames(3);
    }
    CHECK(n.transmitted >= 1);
    CHECK(rd.Science() > scienceBefore);
    CHECK(exp->TransmittedScienceData() > 0.0f);
    CHECK(exp->TotalScienceGenerated() <= exp->ScienceLimit());
    CHECK(exp->TotalScienceGenerated() >= exp->ScienceLimit());  // capped exactly: the last cycle is clipped to the limit
    w.ut += 7200.0;
    ds.FixedUpdate();  // one more cycle: CalculateScience sees total >= limit and disables the part
    w.Frames(3);
    CHECK(!cl->DeployedScienceParts[1]->Enabled());
    CHECK(!expB->Enabled());
    CHECK(cl->PowerRequired() == 1);
    expB->Set<bool>("moduleEnabled", true);  // (state for the save below)
    cl->DeployedScienceParts[1]->moduleEnabled = true;
    cl->UpdatePowerState();
    CHECK(SerenityEnvironment::screenMessages.size() >= 1);
    REQUIRE(ResearchAndDevelopment::GetSubjectByID("temperatureScan@KerbinSrfLanded") != nullptr);
    CHECK(ResearchAndDevelopment::GetSubjectByID("temperatureScan@KerbinSrfLanded")->science > 0.0f);

    // save / load round trip
    ConfigNode save1("SCENARIO");
    ds.OnSave(save1);
    CHECK(save1.GetNode("SCIENCECLUSTERS")->GetNodes("SCIENCECLUSTER").size() == 1);
    {
        const ConfigNode* sc = save1.GetNode("SCIENCECLUSTERS")->GetNode("SCIENCECLUSTER");
        CHECK(*sc->GetValue("ControlModulePartId") == "7201");
        CHECK(*sc->GetValue("DeployedBody") == "Kerbin");
        CHECK(sc->GetNode("MANNEDSCIENCEPARTS")->GetNodes("MANNEDSCIENCEPART").size() == 2);
        const ConfigNode* second = sc->GetNode("MANNEDSCIENCEPARTS")->GetNodes("MANNEDSCIENCEPART")[1];
        CHECK(*second->GetValue("PartId") == "7202");
        REQUIRE(second->GetNode("EXPERIMENT") != nullptr);
        CHECK(*second->GetNode("EXPERIMENT")->GetValue("ExperimentId") == "temperatureScan");
        CHECK(second->GetNode("EXPERIMENT")->GetNode("distanceCurve") != nullptr);
    }
    {
        RT::DeployedScience ds2;
        RT::DeployedScience::Instance = nullptr;
        ds2.Awake();
        ds2.OnLoad(save1);
        REQUIRE(ds2.DeployedScienceClusters().Count() == 1);
        // the loaded components start next frame: the experiment finds its vessel by the saved VesselId
        RT::SerenityBehaviour::RunPendingStarts();
        ConfigNode save2("SCENARIO");
        ds2.OnSave(save2);
        CHECK(save1.ToString() == save2.ToString());
        RT::DeployedScienceCluster* c2 = ds2.DeployedScienceClusters()[7201u];
        CHECK(c2->DeployedBody == &w.Kerbin());
        CHECK(c2->IsPowered());
        CHECK_F32(c2->DeployedScienceParts[1]->Experiment->TotalScienceGenerated(), exp->TotalScienceGenerated());
        for (auto* c : std::vector<RT::DeployedScienceCluster*>{c2}) c->DestroyGameObject();
        RT::SerenityBehaviour::FlushDestroyed();
        RT::DeployedScience::Instance = &ds;
    }

    // retrieve the experiment into the EVA kerbal's inventory: the science data goes to the container of the active vessel
    Vessel* eva = w.Mk("mk1pod.v2", 7299, {});
    eva->vesselType = VesselType::EVA;
    FlightRegistry::ActiveVessel = eva;
    auto inv = std::make_shared<SerenityEnvironment::KerbalInventory>();
    inv->hasCapacity = [](Part&) { return true; };
    inv->firstEmptySlot = [] { return 0; };
    inv->storeCargoPartAtSlot = [](Part&, int) { return true; };
    SerenityEnvironment::activeKerbalInventory = inv;
    // some data has to be waiting
    exp->storedScienceData = 1.25f;
    auto* container = dynamic_cast<ModuleScienceContainer*>(eva->parts[0]->FindModuleOfClass("ModuleScienceContainer"));
    REQUIRE(container != nullptr);
    const int before = container->GetScienceCount();
    expB->OnVesselChange(nullptr);
    expB->RetrievePart();
    CHECK(expB->beingRetrieved);
    CHECK(expB->ControlUnitId() == 0u);
    CHECK(container->GetScienceCount() == before + 1);
    CHECK(exp->StoredScienceData() == 0.0f);
    // the part left the cluster (GroundSciencePartRemoved): one part, power requirement back to the controller's
    RT::SerenityBehaviour::FlushDestroyed();
    CHECK(cl->DeployedScienceParts.size() == 1);
    CHECK(cl->PowerRequired() == 1);
    FlightRegistry::ActiveVessel = nullptr;

    // the vessel of the controller is destroyed: the cluster is deregistered and destroyed
    GE::onVesselWillDestroy.Fire(A);
    CHECK(ds.DeployedScienceClusters().Count() == 0);
    RT::SerenityBehaviour::FlushDestroyed();
    GE::onGroundScienceClusterRegistered.Remove(&n, &Counts::OnReg);
    GE::onGroundScienceClusterUpdated.Remove(&n, &Counts::OnUpd);
    GE::onGroundScienceClusterPowerStateChanged.Remove(&n, &Counts::OnPow);
    GE::onGroundScienceTransmitted.Remove(&n, &Counts::OnTx);
    GE::onGroundScienceGenerated.Remove(&n, &Counts::OnGen);
}

// =============================================================================================================================
// ROC definitions, ROCManager, PQSROCControl, ROC

namespace {
// ROC_DEFINITION fixture nodes: cfg field names of ROCDefinition.Load / RocCBDefinition.Load; NOT game data (the mirror has no Serenity cfgs)
ConfigNode::NodePtr RocDefNode(const std::string& type, const std::string& prefab, const std::vector<std::pair<std::string, std::vector<std::string>>>& bodies) {
    auto n = std::make_shared<ConfigNode>("ROC_DEFINITION");
    n->AddValue("Type", type);
    n->AddValue("displayName", "Rock " + type);
    n->AddValue("prefabName", prefab);
    n->AddValue("modelName", "model");
    n->AddValue("OrientateUp", true);
    n->AddValue("Depth", 0.4f);
    n->AddValue("CanBeTaken", true);
    n->AddValue("Frequency", 0.5f);
    n->AddValue("CastShadows", false);
    n->AddValue("ReceiveShadows", false);
    n->AddValue("CollisionThreshold", 3.0f);
    n->AddValue("SmallROC", true);
    n->AddValue("RandomDepth", false);
    n->AddValue("RandomOrientation", true);
    n->AddValue("RandomRotation", false);
    n->AddValue("burstEmitterMinWait", 10.0f);
    n->AddValue("burstEmitterMaxWait", 20.0f);
    n->AddValue("sfxVolume", 0.5f);
    n->AddValue("idleClip", "Squad/idle");
    n->AddValue("burstClip", "Squad/burst");
    n->AddValue("vfxBaseForce", 2.5f);
    n->AddValue("applyForces", true);
    n->AddValue("vfxForceRadius", "1.5, 4");
    n->AddValue("forceDirection", "0, 1, 0");
    n->AddValue("vfxRadiusCenter", "0.5, 0.25, 0");
    n->AddValue("Scale", 2.0f);
    n->AddValue("localSpaceScanPoints", "0, 1, 0");
    n->AddValue("localSpaceScanPoints", "3, 0, 0");
    n->AddValue("localSpaceScanPoints", "0, 0, 5");
    ConfigNode* curve = n->AddNode("VFX_CURVEFORCE");
    curve->AddValue("key", "0 1 0 0");
    curve->AddValue("key", "1 0.5 0 0");
    for (const auto& b : bodies) {
        ConfigNode* cb = n->AddNode("CELESTIALBODY");
        cb->AddValue("Name", b.first);
        for (const std::string& biome : b.second) cb->AddValue("Biome", biome);
    }
    return n;
}
}  // namespace

TEST_CASE("WP-53 ROCDefinition / RocCBDefinition load, ContainsCBBiome, and the defaults of an empty node") {
    Clean53 clean;
    ConfigNode::NodePtr n = RocDefNode("ROC_T1", "ROCT1", {{"Kerbin", {"Grasslands", "Shores"}}, {"Mun", {"Highlands"}}});
    ROCDefinition d;
    d.Load(*n);
    CHECK(d.type == "ROC_T1");
    CHECK(d.displayName == "Rock ROC_T1");
    CHECK(d.prefabName == "ROCT1");
    CHECK(d.orientateUp);
    CHECK_F32(d.depth, 0.4f);
    CHECK(d.canBeTaken);
    CHECK_F32(d.frequency, 0.5f);
    CHECK(!d.castShadows);
    CHECK(!d.receiveShadows);
    CHECK(d.smallRoc);
    CHECK(!d.randomDepth);
    CHECK(d.randomOrientation);
    CHECK(d.idleClipPath == "Squad/idle");
    CHECK_F32(d.vfxBaseForce, 2.5f);
    CHECK(d.applyForces);
    CHECK_F32(d.vfxForceRadius.x, 1.5f);
    CHECK_F32(d.vfxForceRadius.y, 4.0f);
    CHECK_F32(d.radiusCenter.y, 0.25f);
    CHECK_F32(d.scale, 2.0f);
    REQUIRE(d.localSpaceScanPoints.size() == 3);
    CHECK_F32(d.localSpaceScanPoints[1].x, 3.0f);
    REQUIRE(d.vfxCurveForce.has_value());
    CHECK_F32(d.vfxCurveForce->Evaluate(0.0f), 1.0f);
    CHECK_F32(d.vfxCurveForce->Evaluate(1.0f), 0.5f);
    // ContainsCBBiome walks the body list
    RocCBDefinition k;
    k.Load(*n->GetNodes("CELESTIALBODY")[0]);
    CHECK(k.name == "Kerbin");
    CHECK(k.biomes == std::vector<std::string>({"Grasslands", "Shores"}));
    d.myCelestialBodies = {k};
    CHECK(d.ContainsCBBiome("Kerbin", "Shores"));
    CHECK(!d.ContainsCBBiome("Kerbin", "Mountains"));
    CHECK(!d.ContainsCBBiome("Mun", "Shores"));
    // defaults of the C# initialisers
    ROCDefinition e;
    ConfigNode none("ROC_DEFINITION");
    e.Load(none);
    CHECK(e.castShadows);
    CHECK(e.receiveShadows);
    CHECK_F32(e.scale, 1.0f);
    CHECK(!e.vfxCurveForce.has_value());
    CHECK(e.localSpaceScanPoints.empty());
    // an empty VFX_CURVEFORCE node (no values) leaves the curve null
    ConfigNode withEmptyCurve("ROC_DEFINITION");
    withEmptyCurve.AddNode("VFX_CURVEFORCE");
    ROCDefinition f;
    f.Load(withEmptyCurve);
    CHECK(!f.vfxCurveForce.has_value());
    // RocCBDefinition.Load with no Name: GetValue returns null (kept as the empty name here)
    RocCBDefinition noname;
    ConfigNode cbEmpty("CELESTIALBODY");
    noname.Load(cbEmpty);
    CHECK(noname.name.empty());
    CHECK(noname.biomes.empty());
}

TEST_CASE("WP-53 ROCManager: definitions, prefab checks, body / biome validation, PQSROCControl population, ROCSeed switch, removed ROCs, stats") {
    Clean53 clean;
    WorldSer w;
    CelestialBody& kerbin = w.Kerbin();
    CelestialBody& mun = *w.sys->GetBodyByName("Mun");
    REQUIRE(kerbin.BiomeMap.has_value());
    REQUIRE(mun.BiomeMap.has_value());
    const std::string kb0 = kerbin.BiomeMap->Attributes[0].name, kb1 = kerbin.BiomeMap->Attributes[1].name;
    const std::string mb0 = mun.BiomeMap->Attributes[0].name;
    // A: an unknown body first (C# indexes the list again after RemoveAt: an invalid body must not be the last entry), Kerbin with two valid
    // biomes + one invalid, Mun with only invalid biomes; B: prefab without a ROC component; C: prefab missing, only a Mun entry with an invalid biome
    std::vector<ConfigNode::NodePtr> defs = {
        RocDefNode("ROC_A", "PrefabA", {{"NoSuchBody", {"X"}}, {"Kerbin", {kb0, "NoSuchBiome", kb1}}, {"Mun", {"NoSuchBiome"}}}),
        RocDefNode("ROC_B", "PrefabB", {{"Mun", {mb0}}}),
        RocDefNode("ROC_C", "PrefabC", {{"Mun", {"NoSuchBiome"}}}),
        RocDefNode("ROC_D", "PrefabD", {{"Mun", {mb0}}}),
    };
    auto prior = ScienceEnvironment::getConfigNodes;
    ScienceEnvironment::getConfigNodes = [&](std::string_view t) {
        if (t == "ROC_DEFINITION") {
            std::vector<ConfigNode*> r;
            for (auto& d : defs) r.push_back(d.get());
            return r;
        }
        return prior(t);
    };
    RocEnvironment::prefabState = [](const std::string& name) { return name == "PrefabB" ? 1 : (name == "PrefabC" ? 0 : 2); };

    ROCManager mgr;
    mgr.Awake();
    REQUIRE(ROCManager::Instance == &mgr);
    CHECK(SaveEnvironment::Current().rocManager == &mgr);  // the save model's removed-ROC list is the manager's
    mgr.Start();
    // B had a prefab without ROC component: removed; C had no prefab: kept (warning only)
    std::vector<ROCDefinition*> list = mgr.rocDefinitions();
    REQUIRE(list.size() == 3);
    CHECK(list[0]->type == "ROC_A");
    CHECK(list[1]->type == "ROC_C");
    CHECK(list[2]->type == "ROC_D");
    CHECK(mgr.rocTypeObjects.ContainsKey("ROC_A"));
    CHECK(!mgr.rocTypeObjects.ContainsKey("ROC_B"));
    CHECK(!mgr.rocTypeObjects.ContainsKey("ROC_C"));
    CHECK(mgr.rocTypeObjects["ROC_A"]->name == "PrefabA(Clone)");
    CHECK(mgr.rocTypeObjects["ROC_A"]->type == "ROC_A");
    CHECK_F32(mgr.rocTypeObjects["ROC_A"]->scale, 2.0f);
    CHECK(mgr.rocTypeObjects["ROC_A"]->localSpaceScanPoints.size() == 3);

    // GetROCControlFromCB: ValidateCBBiomeCombos then a LandClassROC per matching definition / body
    PQSROCControl kerbinControl, munControl;
    kerbinControl.currentCBName = "Kerbin";
    munControl.currentCBName = "Mun";
    RocPqsView view;
    view.maxLevel = 6;
    RocEnvironment::bodyRocControl = [&](CelestialBody* b) -> PQSROCControl* {
        if (b == &kerbin) return &kerbinControl;
        if (b == &mun) return &munControl;
        return nullptr;
    };
    RocEnvironment::pqsOf = [&](PQSROCControl*) { return std::optional<RocPqsView>(view); };
    mgr.GetROCControlFromCB();
    list = mgr.rocDefinitions();
    REQUIRE(list.size() == 2);  // C lost its only (empty-biome) body entry and was removed; A keeps Kerbin with the 2 valid biomes
    CHECK(list[0]->type == "ROC_A");
    CHECK(list[1]->type == "ROC_D");
    REQUIRE(list[0]->myCelestialBodies.size() == 1);
    CHECK(list[0]->myCelestialBodies[0].name == "Kerbin");
    CHECK(list[0]->myCelestialBodies[0].biomes == std::vector<std::string>({kb0, kb1}));
    REQUIRE(list[1]->myCelestialBodies.size() == 1);
    CHECK(list[1]->myCelestialBodies[0].name == "Mun");
    REQUIRE(mgr._pqsRocControls.size() == 2);
    CHECK(kerbinControl.rocs.size() == 1);  // ROC_A on Kerbin
    CHECK(munControl.rocs.size() == 1);     // ROC_D on Mun
    CHECK(kerbinControl.rocs[0]->rocName == "ROC_A");
    CHECK(kerbinControl.rocs[0]->celestialBody == &kerbin);
    CHECK(kerbinControl.rocs[0]->minLevel == 6);
    CHECK(kerbinControl.rocs[0]->maxCache == 120);  // (int)(max(frequency, 1) * maxLevel * 20)
    CHECK(kerbinControl.rocs[0]->rocObjectName == "PrefabA(Clone)");
    CHECK(munControl.rocs[0]->rocObjectName == "PrefabD(Clone)");

    // ---- the ROCSeed of the loaded game decides whether ROCs are active (OnGameStateLoad through the event bus)
    Game game;
    game.Mode = Game_Modes::CAREER;
    SaveEnvironment::Current().currentGame = &game;
    PQS pqsK, pqsM;
    kerbinControl.sphere = &pqsK;
    munControl.sphere = &pqsM;
    kerbinControl.sphereView.isStarted = true;
    munControl.sphereView.isStarted = false;
    game.ROCSeed = 5;
    ConfigNode loadNode("GAME");
    GE::onGameStateLoad.Fire(&loadNode);
    CHECK(mgr.RocsEnabledInCurrentGame());
    CHECK(kerbinControl.modEnabled);
    CHECK(munControl.modEnabled);
    CHECK(RocEnvironment::resetModListCalls == 1);  // only the started sphere is reset when enabling
    game.ROCSeed = -1;
    GE::onGameStateLoad.Fire(&loadNode);
    CHECK(!mgr.RocsEnabledInCurrentGame());
    CHECK(!kerbinControl.modEnabled);
    CHECK(!munControl.modEnabled);
    CHECK(RocEnvironment::resetModListCalls == 3);  // disabling resets every sphere
    GE::onGameNewStart.Fire();
    CHECK(mgr.RocsEnabledInCurrentGame());
    mgr.SetROCsStateForGame(false);
    mgr.onMissionStarted(nullptr);
    CHECK(mgr.RocsEnabledInCurrentGame());

    // ---- removed ROCs: shared with the save model (Game.Save / Game(ConfigNode) use SaveEnvironment::rocManager)
    CHECK(!mgr.ROCRemoved(77));
    mgr.RemoveROC(77);
    mgr.RemoveROC(77);
    mgr.RemoveROC(-3);
    CHECK(mgr.ROCRemoved(77));
    CHECK(mgr.removedROCs == std::vector<int>({77, -3}));
    ConfigNode removed("ROCS");
    mgr.SaveRemoveROCs(removed);
    CHECK(removed.GetValuesList("ROCId") == std::vector<std::string>({"77", "-3"}));
    ConfigNode toLoad("ROCS");
    toLoad.AddValue("ROCId", "5");
    toLoad.AddValue("ROCId", "x");
    toLoad.AddValue("ROCId", "5");
    toLoad.AddValue("ROCId", "12");
    mgr.LoadRemovedROCs(toLoad);
    CHECK(mgr.removedROCs == std::vector<int>({5, 12}));

    // ---- PQSROCControl quad flow: area from the mesh bounds, biome gate, AddScatterMeshController, stats
    kerbinControl.celestialBody = &kerbin;
    PQSROCControl::quadInfo = nullptr;
    kerbinControl.OnSetup();
    CHECK(kerbinControl.rocCount == 1);
    CHECK(kerbinControl.rocMinSubdev == 6);
    PQ pq;
    pq.subdivision = 5;
    kerbinControl.OnQuadPreBuild(pq);
    CHECK(!kerbinControl.rocsActive);  // below the minimum subdivision
    pq.subdivision = 6;
    kerbinControl.OnQuadPreBuild(pq);
    CHECK(kerbinControl.rocsActive);
    PQSVertexBuildData bd;
    bd.allowScatter = true;
    kerbinControl.OnVertexBuild(bd);
    CHECK(kerbinControl.allowROCScatter);
    RocQuad rq;
    rq.name = "Kerbin_quad_1";
    rq.hasMesh = true;
    rq.meshBoundsSize = Vector3f(2000.0f, 5.0f, 3000.0f);
    for (int i = 0; i < 225; i++) {
        rq.verts.push_back(Vector3f(static_cast<float>(i), 600000.0f, static_cast<float>(2 * i)));
        rq.vertNormals.push_back(Vector3f(0.0f, 1.0f, 0.0f));
    }
    PQSROCControl::quadInfo = [&](const PQ&) { return &rq; };
    ScienceEnvironment::biomeAtt = [&](const CelestialBody& b, double, double) -> const CBAttributeMapSO::MapAttribute* { return &b.BiomeMap->Attributes[0]; };
    mgr.debugROCStats = true;
    game.ROCSeed = 5;
    kerbinControl.OnQuadBuilt(pq);
    CHECK_F32(kerbinControl.quadArea, 6.0f);  // 2000 / 1000 * 3000 / 1000
    CHECK(!kerbinControl.rocsActive);         // reset after a build
    CHECK(kerbinControl.rocs[0]->gameSeed == 5);
    CHECK(kerbinControl.rocs[0]->quadSeed == StringExtensions::GetHashCode_Net35(rq.name) + StringExtensions::GetHashCode_Net35("ROC_A") + 5);
    CHECK(kerbinControl.rocs[0]->rocN == 3);  // frequency 0.5 * area 6
    REQUIRE(mgr.rocStats.ContainsKey("ROC_A"));
    CHECK(mgr.rocStats["ROC_A"]->activeQuads == 1);
    CHECK_F32(mgr.rocStats["ROC_A"]->activeQuadArea, 6.0f);
    CHECK_F32(mgr.rocStats["ROC_A"]->rocTypeFrequency, 0.5f);
    CHECK(kerbinControl.quadAppliedROCTypes.ContainsKey(StringExtensions::GetHashCode_Net35(rq.name)));
    // build the ROCs of that quad and count them
    std::vector<RocPlacement> placed = kerbinControl.rocs[0]->CreateScatterMesh(kerbinControl.rocs[0]->lastAssigned);
    CHECK(!placed.empty());
    CHECK(placed.size() <= 3);
    mgr.AddROCStats_ROCs("ROC_A", static_cast<int>(placed.size()), 0.5f);
    CHECK(mgr.rocStats["ROC_A"]->activeRocCount == static_cast<int>(placed.size()));
    CHECK_F32(mgr.rocStats["ROC_A"]->rocCoverage(), f32store(fx(static_cast<float>(placed.size())) / fx(6.0f)));
    int statChanges = 0;
    mgr.OnStatsChanged = [&statChanges] { ++statChanges; };
    // a quad whose biome is not in the definition does nothing
    ScienceEnvironment::biomeAtt = [&](const CelestialBody& b, double, double) -> const CBAttributeMapSO::MapAttribute* { return &b.BiomeMap->Attributes.back(); };
    kerbinControl.OnQuadPreBuild(pq);
    RocQuad rq2;
    rq2.name = "Kerbin_quad_2";
    rq2.hasMesh = false;
    rq2.quadArea = 5.0e10;
    PQSROCControl::quadInfo = [&](const PQ&) { return &rq2; };
    kerbinControl.OnQuadBuilt(pq);
    CHECK(mgr.rocStats["ROC_A"]->activeQuads == 1);  // unchanged
    // no mesh: the area comes from quad.quadArea / 1e10
    ScienceEnvironment::biomeAtt = [&](const CelestialBody& b, double, double) -> const CBAttributeMapSO::MapAttribute* { return &b.BiomeMap->Attributes[1]; };
    kerbinControl.OnQuadPreBuild(pq);
    kerbinControl.OnVertexBuild(bd);
    kerbinControl.OnQuadBuilt(pq);
    CHECK_F32(kerbinControl.quadArea, 5.0f);
    CHECK(mgr.rocStats["ROC_A"]->activeQuads == 2);
    CHECK(statChanges >= 1);
    kerbinControl.OnQuadDestroy(pq);
    CHECK(mgr.rocStats["ROC_A"]->activeQuads == 1);
    CHECK_F32(mgr.rocStats["ROC_A"]->activeQuadArea, 6.0f);  // 6 + 5 - 5 (the subtraction uses the control's current quadArea)
    mgr.SubtractROCStats_ROCs("ROC_A", static_cast<int>(placed.size()));
    CHECK(mgr.rocStats["ROC_A"]->activeRocCount == 0);
    // sphere started / reset build and drop the caches
    kerbinControl.rocs[0]->ClearCache();
    CHECK(kerbinControl.rocs[0]->cacheUnassignedCount == 0);
    CHECK(mgr.rocStats.Count() == 0);  // ClearCache clears the stats
    kerbinControl.OnSphereStarted();
    CHECK(kerbinControl.rocs[0]->cacheUnassignedCount == kerbinControl.rocs[0]->maxCache);
    kerbinControl.OnSphereReset();
    CHECK(kerbinControl.rocs[0]->cacheUnassignedCount == 0);
    mgr.ClearROCsCache();
    kerbinControl.rocs.clear();
    munControl.rocs.clear();
    PQSROCControl::quadInfo = nullptr;
    SaveEnvironment::Current().currentGame = nullptr;
}

TEST_CASE("WP-53 ROC: scan positions, raycast variant, collisions, pick up, and the Perform Experiment hook on a science module") {
    Clean53 clean;
    WorldSer w;
    ResearchAndDevelopment rd;
    rd.Awake();
    ConfigNode rdEmpty("ResearchAndDevelopment");
    rd.OnLoad(rdEmpty);
    ROCManager mgr;
    mgr.Awake();
    ROCDefinition d;
    d.Load(*RocDefNode("ROC_A", "PrefabA", {{"Kerbin", {"Grasslands"}}}));
    ROC roc;
    roc.SetStats(d.type, d.displayName, d.prefabName, d.modelName, d.orientateUp, d.depth, d.canBeTaken, d.frequency, d.myCelestialBodies, d.castShadows,
                 d.receiveShadows, d.collisionThreshold, d.smallRoc, d.randomDepth, d.randomOrientation, d.localSpaceScanPoints, d.burstEmitterMinWait,
                 d.burstEmitterMaxWait, d.randomRotation, d.scale, d.sfxVolume, d.idleClipPath, d.burstClipPath, d.vfxCurveForce, d.vfxBaseForce, d.applyForces,
                 d.vfxForceRadius, d.forceDirection, d.radiusCenter);
    roc.rocID = 4242;
    roc.modelTransform.position = Vector3f(10.0f, 0.0f, 0.0f);
    roc.modelTransform.localScale = Vector3f(2.0f, 2.0f, 2.0f);
    // GetClosestScanPosition: local points (0,1,0), (3,0,0), (0,0,5) -> world (10,2,0), (16,0,0), (10,0,10)
    float dist = 0.0f;
    Vector3f p = roc.GetClosestScanPosition(Vector3f(10.0f, 3.0f, 0.0f), dist);
    CHECK_F32(p.x, 10.0f);
    CHECK_F32(p.y, 2.0f);
    CHECK_F32(dist, 1.0f);
    p = roc.GetClosestScanPosition(Vector3f(15.0f, 0.5f, 0.0f), dist);
    CHECK_F32(p.x, 16.0f);
    CHECK_F32(dist, f32store(std::sqrt(1.0 + 0.25)));
    roc.localSpaceScanPoints.clear();
    p = roc.GetClosestScanPosition(Vector3f(10.0f, 3.0f, 4.0f), dist);  // no scan points: the model origin
    CHECK_F32(p.x, 10.0f);
    CHECK_F32(dist, 5.0f);
    // the ray variant: sorted by distance, the first point whose ray hits a ROC-tagged collider wins; none hits -> the plain result
    roc.localSpaceScanPoints = d.localSpaceScanPoints;
    int rays = 0;
    RocEnvironment::raycastHitsRoc = [&](const Vector3f&, const Vector3f&, float maxDistance, float& hit) {
        ++rays;
        hit = maxDistance * 0.5f;
        return rays == 2;  // the second nearest point is the first visible one
    };
    p = roc.GetClosestScanPositionWithRaycasts(Vector3f(10.0f, 3.0f, 0.0f), dist);
    CHECK(rays == 2);
    CHECK_F32(p.x, 16.0f);  // distance order: (10,2,0) < (16,0,0) < (10,0,10)
    CHECK_F32(dist, f32store(std::sqrt(6.0 * 6.0 + 3.0 * 3.0)) * 0.5f);
    rays = -100;
    p = roc.GetClosestScanPositionWithRaycasts(Vector3f(10.0f, 3.0f, 0.0f), dist);
    CHECK_F32(p.y, 2.0f);
    CHECK_F32(dist, 1.0f);
    // collisions: only fast vessels and small ROCs
    Vessel slow, fast;
    slow.speed = 2.0;
    fast.speed = 3.5;
    roc.CheckCollision(slow);
    CHECK(!roc.destroyed);
    CHECK(!mgr.ROCRemoved(4242));
    roc.CheckCollision(fast);
    CHECK(roc.destroyed);
    CHECK(mgr.ROCRemoved(4242));
    REQUIRE(RocEnvironment::rocExplosions.size() == 1);
    CHECK_F64(RocEnvironment::rocExplosions[0].second, 0.30000001192092896);
    ROC big = roc;
    big.destroyed = false;
    big.smallROC = false;
    big.rocID = 9;
    big.CheckCollision(fast);
    CHECK(!big.destroyed);
    big.PickUpROC();
    CHECK(big.destroyed);
    CHECK(mgr.ROCRemoved(9));
    CHECK_F32(roc.GetVFXForceScale(0.5f), roc.vfxCurveForce->Evaluate(0.5f));

    // Perform Experiment: the experiment "<experimentID>_<ROC type>" must exist
    auto prior = ScienceEnvironment::getConfigNodes;
    std::shared_ptr<ConfigNode> synth;
    ScienceEnvironment::getConfigNodes = [&](std::string_view t) {
        std::vector<ConfigNode*> r = prior(t);
        if (t == "EXPERIMENT_DEFINITION") {
            if (!synth) {
                for (ConfigNode* n : r)
                    if (n->GetValue("id") == std::optional<std::string>("mysteryGoo")) synth = n->CreateCopy();
                REQUIRE(synth != nullptr);
                synth->SetValue("id", "mysteryGoo_ROC_A");
            }
            r.push_back(synth.get());
        }
        return r;
    };
    ResearchAndDevelopment::ResetStatics();
    Vessel* v = w.Mk("GooExperiment", 7301, {});
    FlightRegistry::ActiveVessel = v;
    auto* exp = WorldSer::Mod<ModuleScienceExperiment>(*v->parts[0], "ModuleScienceExperiment");
    exp->fxModuleIndices.reset();
    exp->OnStart(StartState::PreLaunch);
    exp->Set<int>("usageReqMaskInternal", 0);
    CHECK(roc.PerformExperiment(exp));
    CHECK(exp->experiment != nullptr);
    CHECK(*exp->experiment->id == "mysteryGoo_ROC_A");
    CHECK(exp->HasExperimentData());
    CHECK(exp->subject->id.rfind("mysteryGoo_ROC_A@KerbinSrfLanded", 0) == 0);
    ROC other = roc;
    other.type = "ROC_Z";
    ModuleScienceExperiment* none = nullptr;
    CHECK_THROWS_AS(other.PerformExperiment(none), NullReferenceException);
    ResearchAndDevelopment::ResetStatics();
    FlightRegistry::ActiveVessel = nullptr;
}

TEST_CASE("WP-53 ROCManager.ValidateCBBiomeCombos keeps the C# quirk: an unknown body that is the last entry indexes past the list") {
    Clean53 clean;
    WorldSer w;
    ROCManager mgr;
    mgr.Awake();
    mgr._rocDefinitions.push_back(std::make_unique<ROCDefinition>());
    mgr._rocDefinitions[0]->type = "ROC_Q";
    mgr._rocDefinitions[0]->myCelestialBodies.push_back(RocCBDefinition("NoSuchBody", {"X"}));
    // ROCManager.cs:260: RemoveAt(num2) is followed by `myCelestialBodies[num2].biomes.Count` on the same index (ArgumentOutOfRangeException)
    CHECK_THROWS_AS(mgr.ValidateCBBiomeCombos(), std::out_of_range);
}

// =============================================================================================================================
// The Breaking Ground contracts on the WP-37 contract system

namespace KC = ksp::Contracts;
namespace SC = ksp::Expansions::Serenity::Contracts;

namespace {

void LoadProgress53(ProgressTracking& pt, const json& scenario) {
    ConfigNode scen("SCENARIO");
    ConfigNode* prog = scen.AddNode("Progress");
    for (auto it = scenario["progress"].begin(); it != scenario["progress"].end(); ++it) {
        ConfigNode* b = prog->AddNode(it.key());
        for (auto jt = it.value().begin(); jt != it.value().end(); ++jt) {
            ConfigNode* n = b->AddNode(jt.key());
            n->AddValue(jt.value().get<std::string>(), 0.0);
        }
    }
    pt.OnLoad(scen);
}

KC::Contract::ContractPrestige Prestige53(const char* s) {
    if (std::string(s) == "Trivial") return KC::Contract::ContractPrestige::Trivial;
    if (std::string(s) == "Significant") return KC::Contract::ContractPrestige::Significant;
    return KC::Contract::ContractPrestige::Exceptional;
}

// A contract that applies the defaults a Serenity contract's Generate passes to the WP-37 Set* pipeline (the same calls in the same order, no random
// draws of its own): generating it with the seed / prestige of the real contract must give bit-identical expiry, deadline and rewards. The agent's
// mentality factors are applied by ContractSystem.GenerateContract after Generate.
struct ProbeRewards {
    float advance = 0, reward = 0, failure = 0, repReward = 0, repFailure = 0, science = 0;
    int minExpire = 1, maxExpire = 7, deadlineDays = 2130;
    CelestialBody* body = nullptr;
};
class ProbeSerenity : public KC::Contract {
public:
    static inline ProbeRewards r;
    std::string TypeName() const override { return "ProbeSerenity"; }
    bool MeetRequirements() override { return true; }
    bool Generate() override {
        AddKeywords({"Scientific", "Commercial"});
        SetExpiry(r.minExpire, r.maxExpire);
        SetDeadlineDays(static_cast<float>(r.deadlineDays), r.body);
        SetFunds(r.advance, r.reward, r.failure, r.body);
        SetReputation(r.repReward, r.repFailure);
        SetScience(r.science);
        return true;
    }
};
void CheckAgainstProbe(KC::ContractSystem& cs, KC::Contract& real, int seed, const char* prestige, const ProbeRewards& pr) {
    ProbeSerenity::r = pr;
    KC::ContractSystem::RegisterContractType("ProbeSerenity", [] { return std::make_unique<ProbeSerenity>(); });
    auto probe = cs.GenerateContract(seed, Prestige53(prestige), "ProbeSerenity");
    REQUIRE(probe != nullptr);
    CHECK(probe->Agent()->Name() == real.Agent()->Name());
    CHECK_F64(real.FundsAdvance, probe->FundsAdvance);
    CHECK_F64(real.FundsCompletion, probe->FundsCompletion);
    CHECK_F64(real.FundsFailure, probe->FundsFailure);
    CHECK_F32(real.ReputationCompletion, probe->ReputationCompletion);
    CHECK_F32(real.ReputationFailure, probe->ReputationFailure);
    CHECK_F32(real.ScienceCompletion, probe->ScienceCompletion);
    CHECK_F64(real.TimeExpiry, probe->TimeExpiry);
    CHECK_F64(real.TimeDeadline, probe->TimeDeadline);
    CHECK(real.Keywords() == probe->Keywords());
}

// The career, the contract machinery and the Serenity fixtures (ROC definitions, experiment / scanner parts): see the file header for what is fixture
struct ContractRig {
    WorldSer w;
    Game game;
    std::unique_ptr<GameVariables> gv;
    std::unique_ptr<KC::Agents::AgentList> agents;
    std::unique_ptr<ProgressTracking> pt;
    Funding funds;
    Reputation rep;
    KC::ContractSystem cs;
    ResearchAndDevelopment rd;
    ROCManager mgr;
    Guid nextGuid;
    std::vector<std::string> munBiomes, minmusBiomes, dunaBiomes;
    ContractRig() {
        CareerEnvironment::Reset();
        KC::ContractsEnvironment::Reset();
        KC::ContractSystem::ClearTypes();
        KC::ContractSystem::ContractWeights.reset();
        ScenarioUpgradeableFacilities::ClearStatics();
        SaveEnvironment& env = SaveEnvironment::Current();
        CelestialBody* k = &w.Kerbin();
        CareerEnvironment::Current().getHomeBody = [k] { return k; };
        game.Mode = Game::Modes::CAREER;
        game.flightState = std::make_shared<FlightState>();
        env.currentGame = &game;
        gv = std::make_unique<GameVariables>();
        gv->reputationAddition = AnimationCurve(std::vector<Keyframe>{Keyframe(0.0f, 0.9f)});
        gv->reputationSubtraction = AnimationCurve(std::vector<Keyframe>{Keyframe(0.0f, 1.3f)});
        const std::string gd = ksp::testing::GameRoot() + "/GameData";
        KC::ContractDefsLite::Load(gd + "/Squad/Contracts/Contracts.cfg");
        for (CelestialBody* b : w.sys->SpawnOrder()) b->bodyDisplayName = KSP::Localization::Localizer::Format(b->bodyDisplayName);
        ConfigNode::NodePtr story = ConfigNode::Load(gd + "/Squad/Resources/StoryDefs.cfg");
        REQUIRE(story);
        ConfigNode::NodePtr sd = story->GetNode("STORY_DEF")->CreateCopy();
        KSP::Localization::Localizer::TranslateBranch(*sd);
        KC::TextGen::Reset();
        KC::TextGen::Setup(*sd);
        agents = std::make_unique<KC::Agents::AgentList>();
        ConfigNode::NodePtr ag = ConfigNode::Load(gd + "/Squad/Agencies/Agents.cfg");
        REQUIRE(ag);
        agents->LoadAgents(ag->GetNodes("AGENT"));
        KC::ContractSystem::RegisterStockTypes();
        SC::Install();
        pt = std::make_unique<ProgressTracking>();
        pt->Awake();
        pt->Start();
        Planetarium::fetch->time = 0.0;
        KC::ContractsEnvironment::Current().newGuid = [this] { return nextGuid; };
        KC::ContractsEnvironment::Current().nowTicks = [] { return int64_t(636000000000000000LL); };
        funds.Awake();
        rep.Awake();
        rep.Start();
        funds.SetFunds(1000000.0, TransactionReasons::Cheating);
        rep.SetReputation(0.0f, TransactionReasons::Cheating);
        CareerEnvironment::Current().rndPresent = [] { return true; };
        rd.Awake();
        ConfigNode rdEmpty("ResearchAndDevelopment");
        rd.OnLoad(rdEmpty);
        cs.Awake();
        ConfigNode weights("SCENARIO");
        ConfigNode* wn = weights.AddNode("WEIGHTS");
        for (auto it = Ref37Scenario()["weights"].begin(); it != Ref37Scenario()["weights"].end(); ++it)
            wn->AddValue(it.key(), static_cast<int32_t>(it.value().get<int>()));
        cs.LoadNow(weights);
        LoadProgress53(*pt, Ref37Scenario());
        // the career multipliers of the first WP-37 set
        game.Parameters->Career.FundsGainMultiplier = 1.0f;
        game.Parameters->Career.FundsLossMultiplier = 1.0f;
        game.Parameters->Career.RepGainMultiplier = 1.0f;
        game.Parameters->Career.RepLossMultiplier = 1.0f;
        game.Parameters->Career.ScienceGainMultiplier = 1.0f;
        // ROC fixtures: three definitions over the real biome names of the loaded bodies' biome maps
        mgr.Awake();
        const auto names = [&](const char* body, size_t n) {
            std::vector<std::string> r;
            CelestialBody* b = w.sys->GetBodyByName(body);
            REQUIRE(b->BiomeMap.has_value());
            for (size_t i = 0; i < n && i < b->BiomeMap->Attributes.size(); i++) r.push_back(b->BiomeMap->Attributes[i].name);
            return r;
        };
        munBiomes = names("Mun", 6);
        minmusBiomes = names("Minmus", 2);
        dunaBiomes = names("Duna", 1);
        const auto add = [&](const char* type, bool canBeTaken, std::vector<RocCBDefinition> bodies) {
            auto d = std::make_unique<ROCDefinition>();
            d->type = type;
            d->displayName = std::string("Display ") + type;
            d->canBeTaken = canBeTaken;
            d->myCelestialBodies = std::move(bodies);
            mgr._rocDefinitions.push_back(std::move(d));
        };
        add("ROC_A", true, {RocCBDefinition("Mun", munBiomes), RocCBDefinition("Minmus", minmusBiomes)});
        add("ROC_B", false, {RocCBDefinition("Duna", dunaBiomes)});
        add("ROC_C", true, {RocCBDefinition("Mun", {munBiomes[0], munBiomes[1]})});
        mgr.SetROCsStateForGame(true);
        // the parts the contracts look at: experiments on two part prefabs, scanner arms on two others
        const auto addGround = [&](const char* part, const char* expId) {
            AvailablePart* ap = const_cast<AvailablePart*>(w.data.parts.getPartInfoByName(part));
            REQUIRE(ap != nullptr);
            ConfigNode node("MODULE");
            node.AddValue("name", "ModuleGroundExperiment");
            node.AddValue("experimentId", expId);
            REQUIRE(ap->partPrefab->AddModule(node, true) != nullptr);
            return ap;
        };
        deployedParts = {addGround("mk1pod.v2", "temperatureScan"), addGround("GooExperiment", "barometerScan")};
    }
    ~ContractRig() {
        pt->OnDestroy();
        pt.reset();
        agents.reset();
        gv.reset();
        KC::ContractSystem::ClearTypes();
        KC::ContractSystem::ContractWeights.reset();
        KC::ContractSystem::Instance = nullptr;
        KC::ContractsEnvironment::Reset();
        KC::ContractDefsLite::Clear();
        KC::TextGen::Reset();
        CareerEnvironment::Reset();
        ScenarioUpgradeableFacilities::ClearStatics();
        SaveEnvironment::Current().currentGame = nullptr;
        ROCManager::Instance = nullptr;
    }
    std::vector<AvailablePart*> deployedParts;
    static const json& Ref37Scenario() {
        static const json j = [] {
            std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp37_reference.json", std::ios::binary);
            REQUIRE(f.good());
            return json::parse(f)["scenario"];
        }();
        return j;
    }
    std::unique_ptr<KC::Contract> Gen(int seed, const char* prestige, const char* type) {
        nextGuid.bytes[0] = static_cast<uint8_t>(seed);
        nextGuid.bytes[5] = static_cast<uint8_t>(seed >> 8);
        return cs.GenerateContract(seed, Prestige53(prestige), type);
    }
};
}  // namespace

TEST_CASE("WP-53 contract primitives: the KSPRandom picks of the three contracts vs the reference (120 cases)") {
    Clean53 clean;
    for (const json& c : Ref53()["picks"]) {
        KSPRandom g(c["seed"].get<int>());
        const int index = g.Next(0, c["candidates"].get<int>() - 1);
        CHECK(index == c["index"].get<int>());
        const int biomeCount = c["biomeCount"].get<int>();
        int want = 1;
        if (biomeCount >= 5) want = 3;
        else if (biomeCount >= 3) want = 2;
        std::vector<int> picked;
        for (int n = 0; n < 10 && static_cast<int>(picked.size()) < want; n++) {
            const int i2 = g.Next(0, biomeCount - 1);
            if (std::find(picked.begin(), picked.end(), i2) == picked.end()) picked.push_back(i2);
        }
        CHECK(picked == c["biomes"].get<std::vector<int>>());
    }
}

TEST_CASE("WP-53 ContractDefs of the Serenity contracts: the C# initialisers, overridden only when Serenity is installed and the node exists") {
    Clean53 clean;
    SaveEnvironment::Reset();
    SC::SerenityContractDefs::DeployedScienceDefs d = SC::SerenityContractDefs::DeployedScience();
    CHECK(d.MaximumExistent == 5);
    CHECK_F32(d.SciencePercentage, 50.0f);
    CHECK(d.expire.DeadlineDays == 2130);
    CHECK_F32(d.FundsBaseAdvance, 10000.0f);
    CHECK_F32(d.FundsBaseReward, 21000.0f);
    CHECK_F32(d.FundsBaseFailure, 12000.0f);
    CHECK_F32(d.ReputationBaseReward, 6.0f);
    CHECK_F32(d.ReputationBaseFailure, 3.0f);
    CHECK_F32(d.ScienceBaseReward, 0.0f);
    SC::SerenityContractDefs::ROCScienceArmDefs a = SC::SerenityContractDefs::ROCScienceArm();
    CHECK(a.MaximumExistent == 3);
    CHECK_F32(a.SimpleArmPercentage, 40.0f);
    CHECK_F32(a.ComplexArmPercentage, 70.0f);
    CHECK_F32(a.SimpleArmAdvance, 8000.0f);
    CHECK_F32(a.ComplexArmReward, 21000.0f);
    CHECK_F32(a.AdvancedArmFailure, 17000.0f);
    CHECK_F32(a.ReputationAdvancedArmReward, 2.0f);
    SC::SerenityContractDefs::ROCScienceRetrievalDefs r = SC::SerenityContractDefs::ROCScienceRetrieval();
    CHECK(r.MaximumExistent == 2);
    CHECK_F32(r.FundsAdvance, 22000.0f);
    CHECK_F32(r.FundsReward, 35000.0f);
    CHECK_F32(r.FundsFailure, 27000.0f);
    // a Contracts.cfg node overrides the value when Serenity is installed (ContractDefs.cs:2307), and is ignored when it is not
    ConfigNode root("Contracts");
    ConfigNode* ds = root.AddNode("DeployedScience");
    ds->AddValue("MaximumExistent", 9);
    ds->AddValue("SciencePercentage", 75.0f);
    ConfigNode* funds = ds->AddNode("Funds");
    funds->AddValue("BaseReward", 123456.0f);
    KC::ContractDefsLite::Load(root);
    CHECK(SC::SerenityContractDefs::DeployedScience().MaximumExistent == 5);  // Serenity not installed
    InstallSerenity();
    d = SC::SerenityContractDefs::DeployedScience();
    CHECK(d.MaximumExistent == 9);
    CHECK_F32(d.SciencePercentage, 75.0f);
    CHECK_F32(d.FundsBaseReward, 123456.0f);
    CHECK_F32(d.FundsBaseAdvance, 10000.0f);  // keys the node does not carry keep the initialiser
    KC::ContractDefsLite::Clear();
}

TEST_CASE("WP-53 ROCScienceRetrievalContract: generation follows the candidate list and the KSPRandom picks; parameter completes on recovery") {
    Clean53 clean;
    ContractRig R;
    ScienceEnvironment::biomeAtt = nullptr;
    int generated = 0;
    for (int seed = 1; seed <= 24; seed++) {
        auto c = R.Gen(seed, seed % 2 ? "Significant" : "Exceptional", "ROCScienceRetrievalContract");
        // the candidates: reached solid bodies (reverse body order), ROC definitions that can be taken, no finished subject
        const auto reached = KC::Lite::ProgressUtilities::GetBodiesReached(true, [&](CelestialBody* cb) { return cb != Planetarium::fetch->Sun && cb->hasSolidSurface; });
        struct Combo {
            std::string body;
            ROCDefinition* def;
            std::string id;
        };
        std::vector<Combo> list;
        for (CelestialBody* b : reached)
            for (ROCDefinition* d : R.mgr.rocDefinitions())
                for (const RocCBDefinition& cb : d->myCelestialBodies)
                    if (b->name() == cb.name && d->canBeTaken) list.push_back({b->name(), d, "ROCScience_" + d->type + "@" + cb.name + "SrfLanded"});
        REQUIRE(!list.empty());
        REQUIRE(c != nullptr);
        ++generated;
        KSPRandom g(seed);
        const Combo& pick = list[static_cast<size_t>(g.Next(0, static_cast<int>(list.size()) - 1))];
        auto* rc = dynamic_cast<SC::ROCScienceRetrievalContract*>(c.get());
        REQUIRE(rc != nullptr);
        CHECK(rc->SubjectId() == pick.id);
        CHECK(rc->TargetBody()->name() == pick.body);
        CHECK(rc->MissionSeed() == seed);
        // the biomes: 1 / 2 / 3 of the body entry's biomes by their count, KSPRandom continuing after the pick
        const RocCBDefinition* cbd = nullptr;
        for (const RocCBDefinition& cb : pick.def->myCelestialBodies)
            if (cb.name == pick.body) cbd = &cb;
        REQUIRE(cbd != nullptr);
        const int want = cbd->biomes.size() >= 5 ? 3 : (cbd->biomes.size() >= 3 ? 2 : 1);
        std::vector<std::string> biomes;
        for (int n = 0; n < 10 && static_cast<int>(biomes.size()) >= 0 && static_cast<int>(biomes.size()) < want; n++) {
            const std::string b = cbd->biomes[static_cast<size_t>(g.Next(0, static_cast<int>(cbd->biomes.size()) - 1))];
            if (std::find(biomes.begin(), biomes.end(), b) == biomes.end()) biomes.push_back(b);
        }
        for (auto& b : biomes) b = ScienceUtil::GetBiomedisplayName(rc->TargetBody(), b, false);
        ConfigNode saved("CONTRACT");
        c->Save(saved);
        // the Contract.Save wrapper writes the parameters and the contract's own OnSave values: the biomes are repeated `biomes` values
        const ConfigNode* params = &saved;
        CHECK(params->GetValuesList("biomes") == biomes);
        CHECK(*saved.GetValue("rocType") == pick.def->type);
        CHECK(*saved.GetValue("scienceTitle") == pick.def->displayName);
        CHECK(*saved.GetValue("subjectId") == pick.id);
        CHECK(*saved.GetValue("body") == pick.body);
        // parameter, rewards (the defaults through the WP-37 Set* pipeline) and keywords
        REQUIRE(c->ParameterCount() == 1);
        auto* p = dynamic_cast<SC::CollectROCScienceRetrieval*>(c->GetParameter(0));
        REQUIRE(p != nullptr);
        CHECK(p->SubjectId() == pick.id);
        CHECK(p->HashString() == pick.id + " " + pick.body);
        const SC::SerenityContractDefs::ROCScienceRetrievalDefs defs = SC::SerenityContractDefs::ROCScienceRetrieval();
        ProbeRewards pr;
        pr.advance = defs.FundsAdvance;
        pr.reward = defs.FundsReward;
        pr.failure = defs.FundsFailure;
        pr.repReward = defs.ReputationReward;
        pr.repFailure = defs.ReputationFailure;
        pr.science = defs.ScienceReward;
        pr.minExpire = defs.expire.MinimumExpireDays;
        pr.maxExpire = defs.expire.MaximumExpireDays;
        pr.deadlineDays = defs.expire.DeadlineDays;
        pr.body = rc->TargetBody();
        CheckAgainstProbe(R.cs, *c, seed, seed % 2 ? "Significant" : "Exceptional", pr);
        CHECK(c->FundsAdvance > 0.0);
        CHECK(c->FundsCompletion > c->FundsAdvance);
        CHECK(c->ReputationCompletion > 0.0f);
        // title / synopsis / completion message use the display name
        CHECK(c->Title() == KSP::Localization::Localizer::Format("#autoLOC_8004396", {pick.def->displayName}));
        CHECK(c->Synopsys().rfind(KSP::Localization::Localizer::Format("#autoLOC_8004421", {pick.def->displayName}), 0) == 0);
        // load back: the same node
        auto copy = KC::ContractSystem::CreateContract("ROCScienceRetrievalContract");
        REQUIRE(copy != nullptr);
        ConfigNode full("CONTRACT");
        c->Save(full);
        REQUIRE(KC::Contract::Load(*copy, full));
        ConfigNode again("CONTRACT");
        copy->Save(again);
        CHECK(full.ToString() == again.ToString());
    }
    CHECK(generated == 24);
}
