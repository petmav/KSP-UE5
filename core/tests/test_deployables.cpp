// WP-27 unit tests: parachutes, deployable parts (solar panels, antennas, radiators), animation-driven modules.
// No golden or capture of these modules' runtime behaviour exists, so every expectation is derived: from the decompiled C# (the derivation
// is in the comment above the check), from the game's own data (the part cfg / part database capture: every stock parachute's deploy
// parameters, every stock solar panel's chargeRate and curves, every ModuleAnimateGeneric's fields), or from tools/wp27/deploy_reference.py
// (an independent Python implementation of the parachute drag / heating maths and of the solar panel flux chain; tests/data/wp27_reference.json).
// The animation clips the model would carry (.mu files are not readable here) are supplied by the tests: the clip LENGTHS are test inputs,
// the arithmetic on them is what is verified.
#include <cmath>
#include <cstring>
#include <fstream>

#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/modules/animation_modules.hpp"
#include "ksp/modules/deployables.hpp"
#include "ksp/modules/parachutes.hpp"
#include "ksp/modules/resource_processing.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::support;

namespace {

nlohmann::json LoadRef27() {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp27_reference.json", std::ios::binary);
    REQUIRE(f.good());
    return nlohmann::json::parse(f);
}

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
// ULP distance of two doubles (same sign)
int64_t UlpD(double a, double b) {
    int64_t x, y;
    std::memcpy(&x, &a, 8);
    std::memcpy(&y, &b, 8);
    if (x < 0) x = INT64_MIN - x;
    if (y < 0) y = INT64_MIN - y;
    return x > y ? x - y : y - x;
}
int64_t UlpF(float a, float b) {
    int32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    if (x < 0) x = INT32_MIN - x;
    if (y < 0) y = INT32_MIN - y;
    return x > y ? static_cast<int64_t>(x) - y : static_cast<int64_t>(y) - x;
}

template <class T>
T* As(PartModule* m) {
    T* t = dynamic_cast<T*>(m);
    REQUIRE(t != nullptr);
    return t;
}

// the stock game + PhysicsGlobals (harness/inputs/Physics.cfg = the game's Physics.cfg) + the flight scene
struct World27 {
    Game15 game;
    PhysicsGlobals pg;
    World27() {
        pg.physicsDatabaseFilename = std::string(KSPCORE_HARNESS_DIR) + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        DeployablesEnvironment::Reset();
        ParachuteEnvironment::Reset();
        AnimationModulesEnvironment::Reset();
        ResourceProcessingEnvironment::Reset();
        SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
        Time::deltaTime = 0.02f;
        Time::fixedDeltaTime = 0.02f;
    }
    ~World27() {
        DeployablesEnvironment::Reset();
        ParachuteEnvironment::Reset();
        AnimationModulesEnvironment::Reset();
        ResourceProcessingEnvironment::Reset();
    }
};

// A part with a flight vessel record (the Part.vessel the modules read) and a model: `clips` are the animation clips of the part's one
// Animation component, every transform name resolves.
struct Rig27 {
    Game15& g;
    std::unique_ptr<Part> host;
    TestVessel vessel;
    ProcessingVessel pv;
    std::shared_ptr<Animation> anim = std::make_shared<Animation>();
    double ut = 100000.0;
    explicit Rig27(Game15& game, const char* part) : g(game), host(game.New(part)) {
        host->fields.persistentId = 1;
        vessel.Adopt({host.get()});
        pv.parts = {host.get()};
        pv.situation = VesselSituations::FLYING;
        pv.mainBody.oceanDensity = 1.0;
        ResourceProcessingEnvironment::vesselOf = [this](const Part&) { return &pv; };
        ResourceProcessingEnvironment::universalTime = [this] { return ut; };
        DeployablesEnvironment::hasModelTransform = [](PartModule&, const std::string&) { return true; };
        DeployablesEnvironment::modelAnimations = [this](PartModule&) { return std::vector<std::shared_ptr<Animation>>{anim}; };
    }
    template <class M>
    M* Mod(const char* cls, int index = 0) {
        int n = 0;
        for (size_t i = 0; i < host->Modules().Count(); i++)
            if (host->Modules()[i]->IsA(cls) && n++ == index) return As<M>(host->Modules()[i]);
        FAIL("module not found: " << cls);
        return nullptr;
    }
    float Weight(const char* cube) {
        DragCube* c = host->DragCubes.GetCube(cube);
        REQUIRE_MESSAGE(c != nullptr, cube);
        return c->Weight();
    }
};

std::string Str(PartModule& m, const char* f) { return m.IsNull(f) ? std::string() : m.Get<std::string>(f); }

}  // namespace

// ================================================ parachutes vs the independent reference =========================================

TEST_CASE("WP-27 parachutes: areas, convective stats, safe-deploy states, chute heating, drag lerps equal the Python reference for every stock parachute") {
    if (!GameAvailable()) return;
    World27 w;
    const auto ref = LoadRef27();
    int64_t maxUlp = 0;
    size_t checks = 0;
    for (const auto& c : ref["parachutes"]) {
        const std::string name = c["part"];
        Rig27 rig(w.game, name.c_str());
        auto* m = rig.Mod<ModuleParachute>("ModuleParachute");
        CAPTURE(name);
        // the cfg / capture parameters the reference used are the module's fields
        for (auto it = c["fields"].begin(); it != c["fields"].end(); ++it) {
            const std::string k = it.key();
            if (k == "deployAltitude" || k == "minAirPressureToOpen" || k == "autoCutSpeed" || k == "stowedDrag" || k == "semiDeployedDrag" || k == "fullyDeployedDrag" ||
                k == "deploymentCurve")
                CHECK(Bits(m->Get<float>(k)) == Bits(static_cast<float>(it.value().get<double>())));
            else
                CHECK(m->Get<double>(k) == it.value().get<double>());
        }
        // GetArea (cube sums, Mono float rule) and CalcBaseStats' bisection: exact
        m->CalcBaseStats();
        CHECK(m->areaDeployed == c["areaDeployed"].get<double>());
        CHECK(m->areaSemi == c["areaSemi"].get<double>());
        CHECK(m->invThermalMass == c["invThermalMass"].get<double>());
        CHECK(m->maxSafeSpeedAtRef == c["maxSafeSpeedAtRef"].get<double>());
        checks += 4;
        // SetConvectiveStats / DeploySafe at the reference's flight points (Math.Pow may differ from CPython's libm in the last bit)
        for (const auto& p : c["points"]) {
            m->SetConvectiveStats(p["density"], p["extTemp"], p["mach"], p["conv"]);
            for (auto [got, key] : {std::pair<double, const char*>{m->machHeatMult, "machHeatMult"}, {m->convectivekW, "convectivekW"}, {m->shockTemp, "shockTemp"}}) {
                if (std::string(key) == "machHeatMult" && p["density"].get<double>() <= 0.0) continue;  // not set in the vacuum branch
                const int64_t u = UlpD(got, p[key].get<double>());
                maxUlp = std::max(maxUlp, u);
                CHECK(u <= 4);
                checks++;
            }
            ModuleParachute::deploymentSafeStates st;
            m->timeUnpacked = 50.0;
            rig.ut = 100.0;
            m->DeploySafe(st);
            static const char* const names[] = {"SAFE", "RISKY", "UNSAFE", "NONE"};
            CHECK(std::string(names[static_cast<int>(st)]) == p["safe"].get<std::string>());
            m->timeUnpacked = 99.5;
            m->DeploySafe(st);
            CHECK(std::string(names[static_cast<int>(st)]) == p["unsafeJustUnpacked"].get<std::string>());
            checks += 2;
        }
        // UpdateCut's heating model, 6000 steps of fixedDeltaTime = 0.02 s (the C# adds the two Euler halves and averages)
        for (const auto& h : c["heats"]) {
            rig.pv.atmDensity = 0.5;
            m->Set<double>("chuteTemp", m->Get<double>("startingTemp"));
            m->SetConvectiveStats(h["density"], h["extTemp"], h["mach"], h["conv"]);
            m->deploymentState = ModuleParachute::deploymentStates::DEPLOYED;
            m->hasCanopy = true;
            m->canopyActive = true;
            rig.host->fields.atmDensity = 0.5;
            rig.pv.Landed = false;
            ParachuteEnvironment::velocity = [](Part&) { return Vector3d{0.0, -60.0, 0.0}; };
            int cutStep = -1;
            size_t si = 0;
            for (int step = 1; step <= 6000; step++) {
                // UpdateCut cuts the chute on overheat; the reference keeps integrating, so re-arm after a cut
                m->UpdateCut();
                if (m->deploymentState == ModuleParachute::deploymentStates::CUT) {
                    if (cutStep < 0) cutStep = step;
                    m->deploymentState = ModuleParachute::deploymentStates::DEPLOYED;
                }
                if (step % 1000 == 0) {
                    const double expect = h["series"][si++].get<double>();
                    const int64_t u = UlpD(m->Get<double>("chuteTemp"), expect);
                    maxUlp = std::max(maxUlp, u);
                    CHECK(u == 0);
                    checks++;
                }
            }
            CHECK(cutStep == h["cutStep"].get<int>());
        }
        // FixedUpdate's drag lerps: Pow / Lerp in float32
        m->Set<std::string>("persistentState", "STOWED");
        for (const auto& l : c["lerps"]) {
            const float t = static_cast<float>(l["t"].get<double>());
            const float lt = Mathf::Pow(t, m->Get<float>("deploymentCurve"));
            // Math.Pow: kspcore's dotnet::Math::Pow is the game's libm bit for bit, CPython's pow is the host's (<= 1 float ULP apart)
            CHECK(UlpF(lt, static_cast<float>(l["lerpTime"].get<double>())) <= 1);
            const float base = static_cast<float>(c["baseDrag"].get<double>());
            const float semi = static_cast<float>(fx(base) + fx(Mathf::Lerp(m->Get<float>("stowedDrag"), m->Get<float>("semiDeployedDrag"), lt)));
            const float full = static_cast<float>(fx(base) + fx(Mathf::Lerp(m->Get<float>("semiDeployedDrag"), m->Get<float>("fullyDeployedDrag"), lt)));
            CHECK(UlpF(semi, static_cast<float>(l["semi"].get<double>())) <= 1);
            CHECK(UlpF(full, static_cast<float>(l["full"].get<double>())) <= 1);
            checks += 3;
        }
    }
    MESSAGE("parachute reference: " << ref["parachutes"].size() << " parts, " << checks << " checks, max ULP of the Pow-dependent doubles " << maxUlp);
    CHECK(ref["parachutes"].size() == 5);
}

// ============================================== parachute state machine on a real stock part ======================================

TEST_CASE("WP-27 ModuleParachute: STOWED -> ACTIVE -> SEMIDEPLOYED -> DEPLOYED -> CUT -> STOWED with the drag cube weights, drag lerp, staging icon") {
    if (!GameAvailable()) return;
    World27 w;
    Rig27 rig(w.game, "parachuteSingle");
    auto* m = rig.Mod<ModuleParachute>("ModuleParachute");
    // the model carries the two chute clips (lengths are test inputs)
    rig.anim->AddClip(Str(*m, "semiDeployedAnimation"), 1.0f);
    rig.anim->AddClip(Str(*m, "fullyDeployedAnimation"), 1.0f);
    rig.host->fields.maximum_drag = 0.02f;
    // OnLoad (ModuleParachute.cs:278): "deploy" fx group and deploymentState from persistentState
    ConfigNode empty("MODULE");
    m->Set<std::string>("persistentState", "DEPLOYED");
    m->OnLoad(empty);
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::DEPLOYED);
    m->Set<std::string>("persistentState", "bogus");
    CHECK_THROWS(m->OnLoad(empty));
    m->Set<std::string>("persistentState", "STOWED");
    m->OnLoad(empty);
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::STOWED);
    // OnStart: stagingIcon default, baseDrag, minAirPressureToOpen editor minimum, events
    rig.host->fields.stagingIcon = std::string("");
    m->OnStart(StartState::Flying);
    CHECK(*rig.host->fields.stagingIcon == "PARACHUTES");
    CHECK(m->baseDrag == 0.02f);
    CHECK(m->Events()["Deploy"]->active);
    CHECK_FALSE(m->Events()["CutParachute"]->active);
    CHECK(m->canopyActive == false);
    CHECK(m->capActive == true);
    CHECK(m->Class().Is("ModuleParachute"));
    CHECK(m->IsStageable());
    // stowed drag: maximum_drag = baseDrag + stowedDrag (f32), cube weights PACKED 1
    CHECK(Bits(rig.host->fields.maximum_drag) == Bits(static_cast<float>(fx(0.02f) + fx(m->Get<float>("stowedDrag")))));
    CHECK(rig.Weight("PACKED") == 1.0f);
    CHECK(rig.Weight("SEMIDEPLOYED") == 0.0f);
    CHECK(rig.Weight("DEPLOYED") == 0.0f);

    // Deploy: STOWED -> ACTIVE (a second Deploy is a no-op); the staging OnActive deploys only when staging is enabled
    m->Deploy();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::ACTIVE);
    CHECK(m->Str("persistentState") == "ACTIVE");
    CHECK(m->Events()["Disarm"]->active);
    CHECK_FALSE(m->Events()["Deploy"]->active);
    m->Deploy();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::ACTIVE);
    // not enough pressure / too slow: stays ACTIVE (pressure * pressureRecip must exceed minAirPressureToOpen = 0.04 -> 4.05 kPa)
    rig.pv.atmDensity = 0.8;
    rig.host->fields.atmDensity = 0.8;  // part.atmDensity == 0 would cut the chute (UpdateCut)
    rig.pv.altitude = 5000.0;  // above deployAltitude (1000): ShouldDeploy false
    ParachuteEnvironment::staticPressure = [](Part&) { return 3.0; };
    ParachuteEnvironment::velocity = [](Part&) { return Vector3d{0.0, -80.0, 0.0}; };
    m->FixedUpdate();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::ACTIVE);
    ParachuteEnvironment::staticPressure = [](Part&) { return 4.2; };  // 4.2 * 0.009869232667160128 = 0.04145 > 0.04
    ParachuteEnvironment::velocity = [](Part&) { return Vector3d{0.0, -0.9, 0.0}; };  // sqrMagnitude 0.81 <= 1: IsMovingFastEnoughToDeploy false
    m->FixedUpdate();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::ACTIVE);
    ParachuteEnvironment::velocity = [](Part&) { return Vector3d{0.0, -80.0, 0.0}; };
    // automateSafeDeploy (0 = the strictest) < deploymentSafeState: the chute only opens in a SAFE state (deploymentSafeState <= automateSafeDeploy)
    m->Set<int32_t>("automateSafeDeploy", 0);
    m->FixedUpdate();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::SEMIDEPLOYED);
    CHECK(m->Str("persistentState") == "SEMIDEPLOYED");
    CHECK(m->canopyActive);
    CHECK_FALSE(m->capActive);
    CHECK(m->deployFxBursts == 1);
    CHECK(m->Events()["CutParachute"]->active);
    CHECK_FALSE(m->Events()["Disarm"]->active);
    auto* semi = (*rig.anim)[Str(*m, "semiDeployedAnimation")];
    CHECK(semi->enabled);
    CHECK(semi->speed == m->Get<float>("semiDeploymentSpeed"));  // 0.5
    CHECK(semi->normalizedTime() == 0.0f);

    // the semi-deploy animation plays: animTime = normalizedTime, lerpTime = Pow(t, deploymentCurve), maximum_drag = baseDrag + Lerp(stowed, semi, lerpTime)
    rig.anim->Tick(0.5f);  // speed 0.5 * 0.5 s = 0.25 of a 1 s clip
    m->FixedUpdate();
    CHECK(m->Get<float>("animTime") == 0.25f);
    const float lt = Mathf::Pow(0.25f, m->Get<float>("deploymentCurve"));
    CHECK(m->lerpTime == lt);
    CHECK(Bits(rig.host->fields.maximum_drag) ==
          Bits(static_cast<float>(fx(0.02f) + fx(Mathf::Lerp(m->Get<float>("stowedDrag"), m->Get<float>("semiDeployedDrag"), lt)))));
    CHECK(Bits(rig.Weight("PACKED")) == Bits(static_cast<float>(1.0 - fx(lt))));
    CHECK(rig.Weight("SEMIDEPLOYED") == lt);
    CHECK(rig.Weight("DEPLOYED") == 0.0f);
    CHECK(rig.host->DragCubes.OcclusionMultiplier() == 0.0f);
    // the animation ends (Once): no longer playing, SEMIDEPLOYED waits for ShouldDeploy (altitude < deployAltitude = 1000)
    rig.anim->Tick(2.0f);
    CHECK_FALSE(rig.anim->isPlaying());
    m->FixedUpdate();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::SEMIDEPLOYED);
    CHECK(m->lerpTime == 1.0f);
    CHECK(m->Get<float>("animTime") == 1.0f);
    CHECK(rig.Weight("SEMIDEPLOYED") == 1.0f);
    rig.pv.altitude = 900.0;
    m->FixedUpdate();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::DEPLOYED);
    CHECK(m->Str("persistentState") == "DEPLOYED");
    CHECK(m->Get<float>("animTime") == 0.0f);
    auto* full = (*rig.anim)[Str(*m, "fullyDeployedAnimation")];
    CHECK(full->enabled);
    CHECK(full->speed == m->Get<float>("deploymentSpeed"));  // 0.12
    CHECK(m->deployFxBursts == 2);
    // full deploy drag: maximum_drag = baseDrag + Lerp(semi, full, Pow(t, curve)); weights PACKED 0, SEMI 1 - d, DEPLOYED d
    rig.anim->Tick(5.0f);  // 0.12 * 5 s = 0.6
    m->FixedUpdate();
    const float t6 = full->normalizedTime();
    CHECK(t6 == m->Get<float>("animTime"));
    const float lt6 = Mathf::Pow(t6, m->Get<float>("deploymentCurve"));
    CHECK(Bits(rig.host->fields.maximum_drag) ==
          Bits(static_cast<float>(fx(0.02f) + fx(Mathf::Lerp(m->Get<float>("semiDeployedDrag"), m->Get<float>("fullyDeployedDrag"), lt6)))));
    CHECK(rig.Weight("DEPLOYED") == lt6);
    CHECK(Bits(rig.Weight("SEMIDEPLOYED")) == Bits(static_cast<float>(1.0 - fx(lt6))));
    CHECK(rig.Weight("PACKED") == 0.0f);

    // UpdateCut: velocity below autoCutSpeed on the ground cuts the chute
    rig.pv.Landed = true;
    rig.host->fields.atmDensity = 0.5;
    ParachuteEnvironment::velocity = [](Part&) { return Vector3d{0.0, -0.1, 0.0}; };
    m->FixedUpdate();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::CUT);
    CHECK(m->Str("persistentState") == "CUT");
    CHECK_FALSE(m->canopyActive);
    CHECK(rig.Weight("PACKED") == 1.0f);
    CHECK(rig.Weight("DEPLOYED") == 0.0f);
    CHECK(Bits(rig.host->fields.maximum_drag) == Bits(static_cast<float>(fx(0.02f) + fx(m->Get<float>("stowedDrag")))));
    CHECK(m->Events()["Repack"]->active);
    // Repack: back to STOWED, cap shown, Deploy available again
    m->Repack();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::STOWED);
    CHECK(m->capActive);
    CHECK(m->Events()["Deploy"]->active);
    CHECK_FALSE(m->Events()["Repack"]->active);
    CHECK(m->deactivateCalls == 1);

    // a vacuum cuts a deployed chute (part.atmDensity == 0); a packed part on the ground cuts a deployed chute
    m->deploymentState = ModuleParachute::deploymentStates::DEPLOYED;
    m->canopyActive = true;
    rig.pv.Landed = false;
    rig.host->fields.atmDensity = 0.0;
    ParachuteEnvironment::velocity = [](Part&) { return Vector3d{0.0, -50.0, 0.0}; };
    m->UpdateCut();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::CUT);
    m->deploymentState = ModuleParachute::deploymentStates::SEMIDEPLOYED;
    m->canopyActive = true;
    rig.host->fields.packed = true;
    rig.pv.Landed = true;
    m->FixedUpdate();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::CUT);
    rig.host->fields.packed = false;
    // shielded part: Deploy refuses unless shieldedCanDeploy
    m->deploymentState = ModuleParachute::deploymentStates::STOWED;
    rig.host->set_ShieldedFromAirstream(true);
    m->Deploy();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::STOWED);
    CHECK_FALSE(ParachuteEnvironment::screenMessages.empty());
}

TEST_CASE("WP-27 ModuleParachute: drag cube names, AssumeDragCubePosition, symmetry count, OnActive, EVA chute rules") {
    if (!GameAvailable()) return;
    World27 w;
    Rig27 rig(w.game, "radialDrogue");
    auto* m = rig.Mod<ModuleParachute>("ModuleParachute");
    {
        auto keep = DeployablesEnvironment::modelAnimations;
        DeployablesEnvironment::modelAnimations = nullptr;
        CHECK(m->GetDragCubeNames().empty());  // no animation component: C# null
        DeployablesEnvironment::modelAnimations = keep;
    }
    rig.anim->AddClip(Str(*m, "semiDeployedAnimation"), 2.0f);
    rig.anim->AddClip(Str(*m, "fullyDeployedAnimation"), 4.0f);
    CHECK(m->GetDragCubeNames() == std::vector<std::string>{"PACKED", "SEMIDEPLOYED", "DEPLOYED"});
    CHECK(m->IsMultipleCubesActive());
    CHECK_FALSE(m->UsesProceduralDragCubes());
    // the stock cubes carry these names (part database capture)
    for (const char* n : {"PACKED", "SEMIDEPLOYED", "DEPLOYED"}) CHECK(rig.host->DragCubes.GetCube(n) != nullptr);
    const std::string sName = Str(*m, "semiDeployedAnimation"), fName = Str(*m, "fullyDeployedAnimation");
    m->AssumeDragCubePosition("DEPLOYED");
    CHECK((*rig.anim)[fName]->normalizedTime() == 1.0f);
    CHECK((*rig.anim)[fName]->enabled);
    CHECK_FALSE((*rig.anim)[sName]->enabled);
    m->AssumeDragCubePosition("SEMIDEPLOYED");
    CHECK((*rig.anim)[sName]->normalizedTime() == 1.0f);
    CHECK((*rig.anim)[sName]->enabled);
    CHECK_FALSE((*rig.anim)[fName]->enabled);
    m->AssumeDragCubePosition("PACKED");
    CHECK((*rig.anim)[sName]->normalizedTime() == 0.0f);
    // OnActive deploys when staging is enabled
    m->Set<bool>("stagingEnabled", false);
    m->OnActive();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::STOWED);
    m->Set<bool>("stagingEnabled", true);
    m->OnActive();
    CHECK(m->deploymentState == ModuleParachute::deploymentStates::ACTIVE);
    // UpdateSymmetry: counterparts + 1 (0 without counterparts)
    m->UpdateSymmetry(rig.host->vessel);
    CHECK(m->symmetryCount == 0);
    auto twin = w.game.New("radialDrogue");
    rig.host->symmetryCounterparts.push_back(twin.get());
    m->UpdateSymmetry(rig.host->vessel);
    CHECK(m->symmetryCount == 2);
    rig.host->symmetryCounterparts.clear();

    // ModuleEvaChute (ModuleEvaChute.cs): never stageable, OnActive empty, activation rules
    auto kerbal = w.game.New("parachuteSingle");
    PartModuleData* md = kerbal->AddModule("ModuleEvaChute", true);
    REQUIRE(md != nullptr);
    auto* eva = As<ModuleEvaChute>(md->module.get());
    CHECK_FALSE(eva->IsStageable());
    eva->Set<bool>("stagingEnabled", true);
    eva->OnActive();
    CHECK(eva->deploymentState == ModuleParachute::deploymentStates::STOWED);
    // CanCrewMemberUseParachute(level): level >= EVAChuteSkill; an empty inventory forbids it (ModuleEvaChute.cs:183)
    eva->evaChuteSkill = 2;
    CHECK(eva->CanCrewMemberUseParachute(2));
    CHECK_FALSE(eva->CanCrewMemberUseParachute(1));
    eva->hasInventory = true;
    eva->inventoryItemCount = 0;
    CHECK_FALSE(eva->CanCrewMemberUseParachute(5));
    eva->inventoryItemCount = 1;
    CHECK(eva->CanCrewMemberUseParachute(5));
    // PassedAdditionalDeploymentChecks matrix (ModuleEvaChute.cs:286)
    ProcessingVessel pv;
    ResourceProcessingEnvironment::vesselOf = [&](const Part&) { return &pv; };
    TestVessel tv;
    tv.Adopt({kerbal.get()});
    auto& k = eva->kerbalEVA;
    k.ableToDeployParachute = true;
    k.JetpackDeployed = false;
    k.seated = false;
    pv.Landed = false;
    pv.Splashed = false;
    CHECK(eva->PassedAdditionalDeploymentChecks());
    pv.Landed = true;
    CHECK_FALSE(eva->PassedAdditionalDeploymentChecks());
    k.seated = true;
    CHECK(eva->PassedAdditionalDeploymentChecks());
    pv.Landed = false;
    pv.Splashed = true;
    kerbal->fields.submergedPortion = 0.5;
    CHECK_FALSE(eva->PassedAdditionalDeploymentChecks());  // seated and submerged
    kerbal->fields.submergedPortion = 0.0;
    CHECK(eva->PassedAdditionalDeploymentChecks());
    k.seated = false;
    CHECK_FALSE(eva->PassedAdditionalDeploymentChecks());  // splashed and not seated
    pv.Splashed = false;
    k.JetpackDeployed = true;
    CHECK_FALSE(eva->PassedAdditionalDeploymentChecks());
    k.JetpackDeployed = false;
    k.ableToDeployParachute = false;
    CHECK_FALSE(eva->PassedAdditionalDeploymentChecks());
    // AllowRepack
    eva->deploymentState = ModuleParachute::deploymentStates::CUT;
    eva->AllowRepack(true);
    CHECK(eva->Events()["Repack"]->active);
    eva->AllowRepack(false);
    CHECK_FALSE(eva->Events()["Repack"]->active);
    // SetEVAChuteActive(false) disables the events and the module
    eva->SetEVAChuteActive(false);
    CHECK_FALSE(eva->Get<bool>("moduleIsEnabled"));
    CHECK_FALSE(eva->Events()["Deploy"]->active);
    // the yaw / pitch / roll rates: derived from ModuleEvaChute.cs:155 with the module's own defaults (yaw 0.5 at 10 m/s, 0.1 at 50 m/s)
    const auto rates = eva->ComputeChuteRates(Vector3f{1.0f, 1.0f, 0.0f}, 30.0);
    // num = (clamp(30,10,50) - 10) / (50 - 10) * (0.1 - 0.5) + 0.5 = 0.3 (float)
    CHECK(UlpF(rates.num, 0.3f) <= 1);
    CHECK(UlpF(rates.num3, 1.0f / 2.0f) == 0);   // turning (x != 0): chuteRollRate / chuteRollRateDivisorWhenPitching = 1 / 2
    CHECK(UlpF(rates.num4, 1.0f / 3.0f) <= 1);   // pitching (y != 0): chutePitchRate / chutePitchRateDivisorWhenTurning = 1 / 3
    CHECK(UlpF(rates.num5, rates.num / 1.5f) <= 1);  // turning: num / chuteYawRateDivisorWhenPitching
    ResourceProcessingEnvironment::vesselOf = nullptr;
}

// ============================================== deployable parts: state machine, drag cubes, shielding, breaking ==================

namespace {
struct SolarRig {
    Rig27 rig;
    ModuleDeployableSolarPanel* m;
    int moves = 0, stops = 0;
    float lastMoveFrom = -1, lastMoveTo = -1, lastStop = -1;
    SolarRig(Game15& g, const char* part) : rig(g, part), m(rig.Mod<ModuleDeployableSolarPanel>("ModuleDeployableSolarPanel")) {
        rig.anim->AddClip(Str(*m, "animationName"), 4.0f);
        m->OnMoving()->Add(this, &SolarRig::OnMove);
        m->OnStop()->Add(this, &SolarRig::OnStopped);
    }
    ~SolarRig() {
        m->OnMoving()->Remove(this, &SolarRig::OnMove);
        m->OnStop()->Remove(this, &SolarRig::OnStopped);
    }
    void OnMove(float a, float b) {
        moves++;
        lastMoveFrom = a;
        lastMoveTo = b;
    }
    void OnStopped(float a) {
        stops++;
        lastStop = a;
    }
    AnimationState& S() { return *(*rig.anim)[Str(*m, "animationName")]; }
};
}  // namespace

TEST_CASE("WP-27 ModuleDeployablePart: RETRACTED -> EXTENDING -> EXTENDED -> RETRACTING -> RETRACTED, animation arithmetic, drag cubes, scalar events") {
    if (!GameAvailable()) return;
    World27 w;
    SolarRig s(w.game, "solarPanels4");
    ModuleDeployableSolarPanel* m = s.m;
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::RETRACTED);
    m->OnStart(StartState::Flying);
    CHECK(m->useAnimation());
    CHECK(m->Events()["Extend"]->active);
    CHECK_FALSE(m->Events()["Retract"]->active);
    CHECK(s.S().wrapMode == WrapMode::ClampForever);  // startFSM
    CHECK_FALSE(s.rig.anim->isPlaying());             // RETRACTED: Stop
    // the scalar module view (cargo bay hook): GetScalar 0, CanMove true, not moving
    IScalarModule* sc = m->AsScalarModule();
    REQUIRE(sc != nullptr);
    CHECK(sc->GetScalar() == 0.0f);
    CHECK(sc->CanMove());
    CHECK_FALSE(sc->IsMoving());
    CHECK(sc->ScalarModuleID() == "deployablePart");

    // Extend: speed 1, normalizedTime 0, EXTENDING, OnMoving(0, 1)
    m->Extend();
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::EXTENDING);
    CHECK(s.S().speed == 1.0f);
    CHECK(s.S().normalizedTime() == 0.0f);
    CHECK(s.S().enabled);
    CHECK(s.moves == 1);
    CHECK(s.lastMoveFrom == 0.0f);
    CHECK(s.lastMoveTo == 1.0f);
    CHECK_FALSE(m->Events()["Extend"]->active);
    CHECK(sc->IsMoving());
    CHECK(sc->GetScalar() == 0.0f);
    // 1 s of a 4 s clip: normalizedTime 0.25; the RETRACTED cube weight is 1 - t, EXTENDED_A (non tracking) 1 - retracted... SetDragCubes(1 - t, 0)
    s.rig.anim->Tick(1.0f);
    m->FixedUpdate();
    CHECK(s.S().normalizedTime() == 0.25f);
    CHECK(m->Get<std::string>("status") == Localizer::Format("#autoLOC_234841"));
    CHECK(sc->GetScalar() == 0.25f);
    const bool tracking = m->Get<bool>("isTracking");
    CHECK(s.rig.Weight("RETRACTED") == 0.75f);
    if (!tracking) CHECK(s.rig.Weight("EXTENDED_A") == 0.25f);
    else CHECK(s.rig.Weight("EXTENDED_A") == 0.0f);  // angle 0
    CHECK(Bits(m->Get<float>("storedAnimationTime")) == Bits(0.25f));
    CHECK(m->Get<float>("storedAnimationSpeed") == 1.0f);
    // the clip ends (ClampForever keeps it at 1): EXTENDED, OnStop(1), Retract available
    s.rig.anim->Tick(10.0f);
    m->FixedUpdate();
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::EXTENDED);
    CHECK(s.stops == 1);
    CHECK(s.lastStop == 1.0f);
    CHECK(m->Events()["Retract"]->active == m->Get<bool>("retractable"));
    CHECK(sc->GetScalar() == 1.0f);
    CHECK_FALSE(s.rig.anim->isPlaying());  // Stop rewound it (updateFSM EXTENDING): sic, GetScalar answers 1 for EXTENDED anyway
    // EXTENDED without a tracking transform: SetDragCubes(0, 0)
    m->hasTrackingTransformLocal = false;
    m->FixedUpdate();
    CHECK(m->Get<std::string>("status") == Localizer::Format("#autoLOC_234828"));
    CHECK(s.rig.Weight("RETRACTED") == 0.0f);
    CHECK(DeployablesEnvironment::scheduleSetCollisionIgnoresCalls >= 2);

    // Retract: speed -1, normalizedTime 1, RETRACTING, OnMoving(1, 0); the RETRACTED weight rises as the clip runs backwards
    m->Retract();
    if (m->Get<bool>("retractable")) {
        CHECK(m->deployState() == ModuleDeployablePart::DeployState::RETRACTING);
        CHECK(s.S().speed == -1.0f);
        CHECK(s.S().normalizedTime() == 1.0f);
        CHECK(s.lastMoveFrom == 1.0f);
        CHECK(s.lastMoveTo == 0.0f);
        s.rig.anim->Tick(2.0f);
        m->FixedUpdate();
        CHECK(s.S().normalizedTime() == 0.5f);
        CHECK(s.rig.Weight("RETRACTED") == 0.5f);
        s.rig.anim->Tick(5.0f);  // ClampForever stops at 0
        m->FixedUpdate();
        CHECK(m->deployState() == ModuleDeployablePart::DeployState::RETRACTED);
        CHECK(s.lastStop == 0.0f);
        CHECK(m->Events()["Extend"]->active);
    }
    // the toggle action: Activate extends, Deactivate retracts, Toggle follows the state (ModuleDeployablePart.cs:287)
    KSPActionParam toggle(0, KSPActionType::Toggle);
    m->set_deployState(ModuleDeployablePart::DeployState::RETRACTED);  // (solarPanels4 is not retractable: Retract above is a no-op)
    m->ExtendPanelsAction(toggle);
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::EXTENDING);
    // SetScalar > 0 extends, <= 0 retracts (only while not broken)
    m->set_deployState(ModuleDeployablePart::DeployState::EXTENDED);
    m->SetScalar(0.0f);
    CHECK((m->deployState() == ModuleDeployablePart::DeployState::RETRACTING) == m->Get<bool>("retractable"));
}

TEST_CASE("WP-27 ModuleDeployablePart: shielding, breaking (collision, pressure, g), repair, antenna rules, construction states") {
    if (!GameAvailable()) return;
    World27 w;
    SolarRig s(w.game, "solarPanels4");
    ModuleDeployableSolarPanel* m = s.m;
    m->OnStart(StartState::Flying);
    // shielded from the airstream with applyShielding: GetScalar 0, CanMove false in flight (applyShielding -> false), Extend refused with a message
    s.rig.host->set_ShieldedFromAirstream(true);
    m->Set<bool>("applyShielding", true);
    m->Set<bool>("applyShieldingExtend", true);
    CHECK(m->GetScalar() == 0.0f);
    CHECK_FALSE(m->CanMove());
    m->Extend();
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::RETRACTED);
    CHECK_FALSE(DeployablesEnvironment::screenMessages.empty());
    // not applyShielding but applyShieldingExtend: CanMove = !applyShieldingExtend
    m->Set<bool>("applyShielding", false);
    CHECK_FALSE(m->CanMove());
    m->Set<bool>("applyShieldingExtend", false);
    CHECK(m->CanMove());
    s.rig.host->set_ShieldedFromAirstream(false);
    // construction states: BROKEN / EXTENDING / RETRACTING forbid detaching, offsetting, rotating
    m->set_deployState(ModuleDeployablePart::DeployState::EXTENDING);
    CHECK_FALSE(m->CanBeDetached());
    m->set_deployState(ModuleDeployablePart::DeployState::RETRACTED);
    CHECK(m->CanBeRotated());
    // module adjusters: a stuck panel cannot move
    m->stuckAdjusters.push_back([] { return true; });
    CHECK_FALSE(m->CanMove());
    m->stuckAdjusters.clear();

    // OnCollisionEnter: impactResistance (2) while extended; retracted tracking panels use impactResistanceRetracted (5)
    m->set_deployState(ModuleDeployablePart::DeployState::EXTENDED);
    m->hasPivot = true;
    m->OnCollisionEnter(m->Get<float>("impactResistance") - 0.5f);
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::EXTENDED);
    m->OnCollisionEnter(m->Get<float>("impactResistance") + 0.5f);
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::BROKEN);
    CHECK_FALSE(m->Events()["Extend"]->active);
    CHECK_FALSE(m->Events()["Retract"]->active);
    // repair: BROKEN -> RETRACTED, startFSM, onPartRepaired
    m->CheatRepair();
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::RETRACTED);
    CHECK(m->Events()["Extend"]->active);
    // NoCrashDamage / packed parts do not break
    DeployablesEnvironment::noCrashDamage = true;
    m->set_deployState(ModuleDeployablePart::DeployState::EXTENDED);
    m->breakPanels();
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::EXTENDED);
    DeployablesEnvironment::noCrashDamage = false;
    s.rig.host->fields.packed = true;
    m->breakPanels();
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::EXTENDED);
    s.rig.host->fields.packed = false;

    // ShouldBreakFromPressure: q (kPa) >= windResistance (3) and q * max(|dot(vel, forward)|, minAoA) > windResistance
    DeployablesEnvironment::trackingFrame = [](ModuleDeployablePart&) {
        DeployablesEnvironment::TrackingFrame f;
        f.pivotForward = Vector3f{0.0f, 0.0f, 2.0f};  // normalised inside
        return std::optional(f);
    };
    s.rig.pv.velocityD = Vector3d{0.0, 0.0, 100.0};
    s.rig.host->fields.dynamicPressurekPa = 2.9;
    CHECK_FALSE(m->ShouldBreakFromPressure());
    s.rig.host->fields.dynamicPressurekPa = 3.5;
    CHECK(m->ShouldBreakFromPressure());   // |dot| = 1: 3.5 > 3
    s.rig.pv.velocityD = Vector3d{100.0, 0.0, 0.0};
    CHECK_FALSE(m->ShouldBreakFromPressure());  // |dot| = 0 -> minAoAForQCheck 0.1875: 3.5 * 0.1875 = 0.66 < 3
    s.rig.host->fields.dynamicPressurekPa = 17.0;
    CHECK(m->ShouldBreakFromPressure());   // 17 * 0.1875 = 3.19 > 3
    s.rig.host->fields.dynamicPressurekPa = 3.5;
    // ShouldBreakFromG: vessel.geeForce > gResistance (infinity by default)
    CHECK_FALSE(m->ShouldBreakFromG());
    m->Set<double>("gResistance", 5.0);
    s.rig.pv.geeForce = 6.0;
    CHECK(m->ShouldBreakFromG());
    // updateFSM breaks an extended panel on pressure (flight) with the notification
    m->set_deployState(ModuleDeployablePart::DeployState::EXTENDED);
    s.rig.pv.velocityD = Vector3d{0.0, 0.0, 100.0};
    s.rig.pv.geeForce = 0.0;
    m->hasTrackingTransformLocal = false;
    m->updateFSM();
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::BROKEN);
    CHECK(DeployablesEnvironment::breakNotificationCalls == 1);
    CHECK(DeployablesEnvironment::flightLog.size() == 1);
    // BROKEN: status text, RETRACTED weight 1
    m->updateFSM();
    CHECK(m->Get<std::string>("status") == Localizer::Format("#autoLOC_234868"));
    CHECK(s.rig.Weight("RETRACTED") == 1.0f);
    DeployablesEnvironment::trackingFrame = nullptr;

    // ModuleDeployableAntenna: CanMove false while the pressure would break it; Extend refused in flight under pressure; MinAoA 0.5
    Rig27 arig(w.game, "HighGainAntenna");
    auto* a = arig.Mod<ModuleDeployableAntenna>("ModuleDeployableAntenna");
    arig.anim->AddClip(Str(*a, "animationName"), 2.0f);
    a->OnStart(StartState::Flying);
    CHECK(a->MinAoAForQCheck() == 0.5f);
    DeployablesEnvironment::trackingFrame = [](ModuleDeployablePart&) {
        DeployablesEnvironment::TrackingFrame f;
        f.pivotForward = Vector3f{0.0f, 0.0f, 1.0f};
        return std::optional(f);
    };
    arig.pv.velocityD = Vector3d{0.0, 0.0, 1.0};
    arig.host->fields.dynamicPressurekPa = a->Get<float>("windResistance") * 2.0f;
    CHECK(a->CanMove() == false);
    a->Extend();
    CHECK(a->deployState() == ModuleDeployablePart::DeployState::RETRACTED);
    arig.host->fields.dynamicPressurekPa = 0.0;
    CHECK(a->CanMove());
    a->Extend();
    CHECK(a->deployState() == ModuleDeployablePart::DeployState::EXTENDING);
    DeployablesEnvironment::trackingFrame = nullptr;
}

TEST_CASE("WP-27 retractable deployables (antenna, radiator): the full extend / retract cycle with the animation speeds of ModuleDeployablePart.cs:350, :389") {
    if (!GameAvailable()) return;
    World27 w;
    for (const char* part : {"HighGainAntenna", "foldingRadMed"}) {
        Rig27 rig(w.game, part);
        auto* m = rig.Mod<ModuleDeployablePart>(std::string(part) == "HighGainAntenna" ? "ModuleDeployableAntenna" : "ModuleDeployableRadiator");
        CAPTURE(part);
        rig.anim->AddClip(Str(*m, "animationName"), 2.0f);
        AnimationState& st = *(*rig.anim)[Str(*m, "animationName")];
        m->OnStart(StartState::Flying);
        m->hasTrackingTransformLocal = false;
        CHECK(m->Get<bool>("retractable"));
        m->Extend();
        CHECK(m->deployState() == ModuleDeployablePart::DeployState::EXTENDING);
        rig.anim->Tick(3.0f);
        m->FixedUpdate();
        CHECK(m->deployState() == ModuleDeployablePart::DeployState::EXTENDED);
        CHECK(m->Events()["Retract"]->active);
        // the transition frame still sets 1 - normalizedTime AFTER Stop rewound the clip to 0 (KSP quirk: RETRACTED weight 1 for one frame)
        CHECK(rig.Weight("RETRACTED") == 1.0f);
        m->FixedUpdate();
        CHECK(rig.Weight("RETRACTED") == 0.0f);
        m->Retract();
        CHECK(m->deployState() == ModuleDeployablePart::DeployState::RETRACTING);
        CHECK(st.speed == -1.0f);
        CHECK(st.normalizedTime() == 1.0f);
        CHECK_FALSE(m->Events()["Retract"]->active);
        rig.anim->Tick(1.0f);  // half of the 2 s clip back
        m->FixedUpdate();
        CHECK(st.normalizedTime() == 0.5f);
        CHECK(rig.Weight("RETRACTED") == 0.5f);
        CHECK(m->Get<float>("storedAnimationSpeed") == -1.0f);
        rig.anim->Tick(2.0f);
        m->FixedUpdate();
        CHECK(m->deployState() == ModuleDeployablePart::DeployState::RETRACTED);
        CHECK(m->Events()["Extend"]->active);
        m->FixedUpdate();
        CHECK(rig.Weight("RETRACTED") == 1.0f);
        // editor: speed +-editorAnimationSpeedMult, Retract allowed even when not retractable
        SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
        m->Extend();
        CHECK(st.speed == m->Get<float>("editorAnimationSpeedMult"));
        SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    }
}

TEST_CASE("WP-27 deployable parts: Load / Save, drag cube names, tracking modes, radiator display names") {
    if (!GameAvailable()) return;
    World27 w;
    SolarRig s(w.game, "solarPanels4");
    ModuleDeployableSolarPanel* m = s.m;
    // OnLoad (ModuleDeployablePart.cs:732): stateString -> deployState, sunTracking -> isTracking + SUN, vesselID, destroyOnBreakObject list
    auto node = ConfigNode::Parse("MODULE\n{\n name = ModuleDeployableSolarPanel\n stateString = EXTENDED\n sunTracking = False\n vesselID = abc\n destroyOnBreakObject = a\n destroyOnBreakObject = b\n}\n");
    REQUIRE(node);
    const ConfigNode* mod = node->nodes()[0];
    m->OnLoad(*mod);
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::EXTENDED);
    CHECK_FALSE(m->Get<bool>("isTracking"));
    CHECK(m->vesselID == "abc");
    CHECK(m->destroyOnBreakObjects == std::vector<std::string>{"a", "b"});
    CHECK(m->Get<std::string>("partType") == Localizer::Format("#autoLOC_235329"));
    ConfigNode save("MODULE");
    m->OnSave(save);
    CHECK(save.GetValues("destroyOnBreakObject") == std::vector<std::string>{"a", "b"});
    auto node2 = ConfigNode::Parse("MODULE\n{\n sunTracking = True\n}\n");
    m->OnLoad(*node2->nodes()[0]);
    CHECK(m->Get<bool>("isTracking"));
    CHECK(m->trackingMode() == ModuleDeployablePart::TrackingMode::SUN);
    // GetDragCubeNames: the animation component exists -> RETRACTED, EXTENDED_A (+ EXTENDED_B when tracking)
    CHECK(m->GetDragCubeNames() == std::vector<std::string>{"RETRACTED", "EXTENDED_A", "EXTENDED_B"});
    m->Set<bool>("isTracking", false);
    CHECK(m->GetDragCubeNames() == std::vector<std::string>{"RETRACTED", "EXTENDED_A"});
    m->Set<bool>("useAnimation", false);
    CHECK_FALSE(m->IsMultipleCubesActive());
    m->Set<bool>("isTracking", true);
    CHECK(m->IsMultipleCubesActive());
    // the stock panels' drag cubes exist under these names (part database capture): RETRACTED, EXTENDED_A (, EXTENDED_B)
    CHECK(s.rig.host->DragCubes.GetCube("RETRACTED") != nullptr);
    CHECK(s.rig.host->DragCubes.GetCube("EXTENDED_A") != nullptr);
    // AssumeDragCubePosition
    m->Set<bool>("useAnimation", true);
    m->AssumeDragCubePosition("EXTENDED_B");
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::EXTENDED);
    CHECK(s.S().normalizedTime() == 1.0f);
    CHECK(s.S().speed == 0.0f);
    m->AssumeDragCubePosition("RETRACTED");
    CHECK(m->deployState() == ModuleDeployablePart::DeployState::RETRACTED);
    CHECK(s.S().normalizedTime() == 0.0f);
    // display names
    CHECK(m->GetModuleDisplayName() == Localizer::Format("#autoLoc_6003038"));
    Rig27 r(w.game, "foldingRadMed");
    auto* rad = r.Mod<ModuleDeployableRadiator>("ModuleDeployableRadiator");
    CHECK(rad->GetModuleDisplayName() == Localizer::Format("#autoLOC_7000030"));
    CHECK(rad->Get<std::string>("partType") == Localizer::Format("#autoLOC_232117"));
    Rig27 mg(w.game, "Magnetometer");
    auto* base = mg.Mod<ModuleDeployablePart>("ModuleDeployablePart");
    CHECK(base->GetModuleDisplayName() == Localizer::Format("#autoLoc_6003036"));
    // every stock deployable part: DeployState parsed from the cfg is RETRACTED unless stateString says otherwise, trackingMode SUN for
    // the tracking panels (sunTracking = True sets it)
    int panels = 0;
    for (const char* n : {"solarPanelOX10C", "solarPanelSP10C", "solarPanelOX10L", "solarPanelSP10L", "solarPanels2", "solarPanels4", "solarPanels1", "solarPanels3",
                          "largeSolarPanel", "solarPanels5"}) {
        auto p = w.game.New(n);
        auto* sp = dynamic_cast<ModuleDeployableSolarPanel*>(p->FindModuleOfClass("ModuleDeployableSolarPanel"));
        REQUIRE(sp != nullptr);
        CHECK(sp->deployState() == ModuleDeployablePart::DeployState::RETRACTED);
        CHECK(sp->Get<float>("chargeRate") > 0.0f);
        CHECK_FALSE(sp->resHandler.outputResources.empty());  // created by OnLoad from chargeRate (ModuleDeployableSolarPanel.cs:184)
        CHECK(sp->resHandler.outputResources[0]->name == "ElectricCharge");
        CHECK(sp->resHandler.outputResources[0]->rate == static_cast<double>(sp->Get<float>("chargeRate")));
        CHECK(sp->Get<FloatCurve>("temperatureEfficCurve").Evaluate(300.0f) == 1.0f);  // the OnAwake default curve key
        panels++;
    }
    CHECK(panels == 10);
}

// ============================================== solar panel flux maths vs the independent reference ===============================

TEST_CASE("WP-27 ModuleDeployableSolarPanel: sunAOA, temperature / time efficiency, distance multiplier and flow rate equal the Python reference for every stock panel") {
    if (!GameAvailable()) return;
    World27 w;
    const auto ref = LoadRef27();
    int64_t maxUlp = 0;
    size_t checks = 0;
    for (const auto& c : ref["solar"]) {
        const std::string name = c["part"];
        SolarRig s(w.game, name.c_str());
        ModuleDeployableSolarPanel* m = s.m;
        CAPTURE(name);
        // chargeRate / flowMult are the cfg values the reference read
        CHECK(Bits(m->Get<float>("chargeRate")) == Bits(static_cast<float>(c["chargeRate"].get<double>())));
        CHECK(Bits(m->Get<float>("flowMult")) == Bits(static_cast<float>(c["flowMult"].get<double>())));
        m->OnStart(StartState::Flying);
        m->set_deployState(ModuleDeployablePart::DeployState::EXTENDED);
        m->Set<bool>("useCurve", false);
        m->hasTrackingDotTransform = true;
        m->Set<double>("launchUT", 1000.0);
        for (const auto& smp : c["samples"]) {
            const std::string typ = smp["type"];
            EnumValue pt = m->Get<EnumValue>("panelType");
            pt.value = typ == "FLAT" ? 0 : typ == "CYLINDRICAL" ? 1 : 2;
            m->Set<EnumValue>("panelType", pt);
            EnumValue at = m->Get<EnumValue>("alignType");
            at.value = 0;  // PIVOT: the cylinder uses trackingDotTransform.forward
            m->Set<EnumValue>("alignType", at);
            const float dot = static_cast<float>(smp["dot"].get<double>());
            m->lastFrame.reset();
            DeployablesEnvironment::TrackingFrame f;
            f.trackingDotForward = Vector3f{1.0f, 0.0f, 0.0f};
            f.trackingDirection = Vector3f{dot, 0.0f, 0.0f};  // Dot = dot
            f.trackingDistance = 1.0e9f;
            m->lastFrame = f;
            s.rig.pv.solarFlux = smp["solarFlux"];
            s.rig.host->fields.skinTemperature = smp["skinTemperature"];
            s.rig.host->fields.submergedPortion = 0.0;
            s.rig.ut = 1000.0 + smp["ut_minus_launch_seconds"].get<double>();
            m->Set<float>("efficiencyMult", static_cast<float>(smp["efficiencyMult"].get<double>()));
            m->PostCalculateTracking(true, f.trackingDirection);
            // sunAOA: float32 (Clamp / (1 - |dot|) * 1/pi)
            CHECK(Bits(m->Get<float>("sunAOA")) == Bits(static_cast<float>(smp["sunAOA"].get<double>())));
            // the temperature curve is Hermite arithmetic: the reference writes it from the documented form; allow a few float ULP
            const int64_t u1 = UlpD(m->_efficMult, smp["efficMult"].get<double>());
            const int64_t u2 = UlpD(m->_distMult, smp["distMult"].get<double>());
            const int64_t u3 = UlpD(m->_flowRate, smp["flowRate"].get<double>());
            maxUlp = std::max({maxUlp, u1 / (1LL << 29), u3 / (1LL << 29)});  // float-sized ULPs of the double
            CHECK(u2 == 0);
            CHECK(std::fabs(m->_efficMult - smp["efficMult"].get<double>()) <= 4e-7 * std::max(1.0, std::fabs(m->_efficMult)));
            CHECK(std::fabs(m->_flowRate - smp["flowRate"].get<double>()) <= 4e-7 * std::max(1.0, std::fabs(m->_flowRate)));
            checks += 5;
            // no line of sight: sunAOA 0, flow 0
            m->lastFrame = f;
            m->PostCalculateTracking(false, f.trackingDirection);
            CHECK(m->Get<float>("sunAOA") == 0.0f);
            CHECK(m->_flowRate == 0.0);
        }
        // PostFSMUpdate zeroes sunAOA / flowRate unless EXTENDED
        m->set_deployState(ModuleDeployablePart::DeployState::RETRACTED);
        m->Set<float>("sunAOA", 0.7f);
        m->Set<float>("flowRate", 3.0f);
        m->PostFSMUpdate();
        CHECK(m->Get<float>("sunAOA") == 0.0f);
        CHECK(m->Get<float>("flowRate") == 0.0f);
    }
    MESSAGE("solar reference: " << ref["solar"].size() << " panels, " << checks << " checks, max curve-derived deviation " << maxUlp << " float-ULP");
    CHECK(ref["solar"].size() == 10);
}

TEST_CASE("WP-27 ModuleDeployableSolarPanel: useCurve distance multiplier, submersion, launchUT, efficiency adjusters, tracking LOS") {
    if (!GameAvailable()) return;
    World27 w;
    SolarRig s(w.game, "solarPanels4");
    ModuleDeployableSolarPanel* m = s.m;
    m->OnStart(StartState::Flying);
    m->set_deployState(ModuleDeployablePart::DeployState::EXTENDED);
    m->hasTrackingDotTransform = true;
    m->Set<double>("launchUT", 0.0);
    s.rig.ut = 0.0;
    s.rig.pv.solarFlux = 1360.0;
    s.rig.host->fields.skinTemperature = 300.0;
    DeployablesEnvironment::TrackingFrame f;
    f.trackingDotForward = Vector3f{0.0f, 0.0f, 1.0f};
    f.trackingDirection = Vector3f{0.0f, 0.0f, 1.0f};
    f.trackingDistance = 13599840256.0f;  // 1 AU
    m->lastFrame = f;
    // useCurve: _distMult = powerCurve.Evaluate(distance) (the curve of the cfg when it has one, else the empty FloatCurve: 0)
    m->Set<bool>("useCurve", true);
    FloatCurve pc;
    pc.Add(0.0f, 10.0f);
    pc.Add(13599840256.0f, 1.0f);
    m->Set<FloatCurve>("powerCurve", pc);
    m->PostCalculateTracking(true, f.trackingDirection);
    CHECK(m->_distMult == static_cast<double>(pc.Evaluate(13599840256.0f)));
    CHECK(m->_distMult == 1.0);
    m->Set<bool>("useCurve", false);
    // submersion: portion in (0, 1): _flowRate *= LerpUnclamped(1, 1 / (1 + depth * oceanDensity), portion), status gets cacheAutoLOC_235468 appended
    m->PostCalculateTracking(true, f.trackingDirection);
    const double dry = m->_flowRate;
    CHECK(dry == 1.0);
    s.rig.host->fields.submergedPortion = 0.5;
    s.rig.host->fields.maxDepth = 3.0;
    DeployablesEnvironment::secondaryTransformAltitude = [](ModuleDeployablePart&) { return -2.0; };
    m->PostCalculateTracking(true, f.trackingDirection);
    // num = 0 - (-2) = 2; num = (2 * 3 + 3) * 0.25 = 2.25; num2 = 1 / (1 + 2.25 * 1.0); flowRate *= (1 - t) * 1 + t * num2 at t = 0.5
    const double num2 = 1.0 / (1.0 + 2.25 * 1.0);
    CHECK(m->_flowRate == dry * ((1.0 - 0.5) * 1.0 + 0.5 * num2));
    CHECK(m->Get<std::string>("status").size() > Localizer::Format("#autoLOC_235418").size());
    s.rig.host->fields.submergedPortion = 1.0;
    m->PostCalculateTracking(true, f.trackingDirection);
    CHECK(m->_flowRate == dry * num2);
    s.rig.host->fields.submergedPortion = 0.0;
    // efficiency adjusters (module adjusters) chain over originalEfficiencyMultiplier in FixedUpdate; launchUT is set while PRELAUNCH or < 0
    m->efficiencyAdjusters.push_back([](float e) { return e * 0.5f; });
    m->efficiencyAdjusters.push_back([](float e) { return e + 0.1f; });
    m->Set<double>("launchUT", -1.0);
    m->hasTrackingTransformLocal = false;  // updateFSM's EXTENDED branch would need a tracking frame
    s.rig.ut = 777.0;
    s.rig.pv.situation = VesselSituations::FLYING;
    m->FixedUpdate();
    CHECK(m->Get<double>("launchUT") == 777.0);
    CHECK(Bits(m->Get<float>("efficiencyMult")) == Bits(static_cast<float>(fx(fx(m->originalEfficiencyMultiplier) * 0.5) + 0.1)) );
    s.rig.ut = 900.0;
    m->FixedUpdate();
    CHECK(m->Get<double>("launchUT") == 777.0);  // flying: unchanged
    s.rig.pv.situation = VesselSituations::PRELAUNCH;
    m->FixedUpdate();
    CHECK(m->Get<double>("launchUT") == 900.0);
    // tracking LOS: aero shielding blocks, the raycast hook decides otherwise
    std::string blocker;
    s.rig.host->set_ShieldedFromAirstream(true);
    CHECK_FALSE(m->CalculateTrackingLOS(f.trackingDirection, blocker));
    CHECK(blocker == "aero shielding");
    s.rig.host->set_ShieldedFromAirstream(false);
    CHECK(m->CalculateTrackingLOS(f.trackingDirection, blocker));
    DeployablesEnvironment::trackingRaycast = [](ModuleDeployablePart&, const Vector3f&, std::string& b) {
        b = "Mun";
        return false;
    };
    CHECK_FALSE(m->CalculateTrackingLOS(f.trackingDirection, blocker));
    CHECK(blocker == "Mun");
    // CalculateTracking: the pivot alignment hook, the sin of the pivot's euler Y drives the cube weights for a tracking panel
    DeployablesEnvironment::trackingRaycast = nullptr;
    m->Set<bool>("isTracking", true);
    m->Set<bool>("useAnimation", true);
    m->hasPanelRotationTransform = true;
    m->hasPivot = true;
    float alignOffset = -1.0f, alignT = -1.0f;
    DeployablesEnvironment::alignPanel = [&](ModuleDeployablePart&, float o, float t) {
        alignOffset = o;
        alignT = t;
    };
    f.panelEulerY = 30.0f;
    DeployablesEnvironment::trackingFrame = [&](ModuleDeployablePart&) { return std::optional(f); };
    s.rig.pv.solarFlux = 1360.0;
    m->CalculateTracking();
    CHECK(alignOffset == m->Get<float>("TrackingAlignmentOffset"));
    CHECK(alignT == static_cast<float>(fx(TimeWarp::deltaTime()) * fx(m->Get<float>("trackingSpeed"))));
    const float angle = Mathf::Abs(Mathf::Sin(static_cast<float>(fx(0.017453292f) * fx(30.0f))));
    // SetDragCubes(0, angle): RETRACTED 0, EXTENDED_A (1 - 0) * angle, EXTENDED_B (1 - 0) * (1 - angle)
    CHECK(s.rig.Weight("RETRACTED") == 0.0f);
    CHECK(Bits(s.rig.Weight("EXTENDED_A")) == Bits(angle));
    CHECK(Bits(s.rig.Weight("EXTENDED_B")) == Bits(static_cast<float>(1.0 - fx(angle))));
    DeployablesEnvironment::alignPanel = nullptr;
    DeployablesEnvironment::trackingFrame = nullptr;
    // SetDragCubes weights equal the reference for (retracted, angle) pairs (float32)
    const auto ref = LoadRef27();
    for (const auto& d : ref["dragWeights"]) {
        const float r = static_cast<float>(d["retracted"].get<double>()), a = static_cast<float>(d["angle"].get<double>());
        m->Set<bool>("isTracking", true);
        m->SetDragCubes(r, a);
        CHECK(Bits(s.rig.Weight("RETRACTED")) == Bits(r));
        CHECK(Bits(s.rig.Weight("EXTENDED_A")) == Bits(static_cast<float>(d["A"].get<double>())));
        CHECK(Bits(s.rig.Weight("EXTENDED_B")) == Bits(static_cast<float>(d["B"].get<double>())));
        m->Set<bool>("isTracking", false);
        m->SetDragCubes(r, a);
        CHECK(Bits(s.rig.Weight("EXTENDED_A")) == Bits(static_cast<float>(d["Aonly"].get<double>())));
    }
}

// ============================================== ModuleAnimateGeneric / AnimationGroup / AnimationSetter / LandingGear =============

TEST_CASE("WP-27 ModuleAnimateGeneric on a real part: Toggle, animation arithmetic, states, one-shot, events (the cargo bay hook), save") {
    if (!GameAvailable()) return;
    World27 w;
    Rig27 rig(w.game, "GooExperiment");
    auto* m = rig.Mod<ModuleAnimateGeneric>("ModuleAnimateGeneric");
    CHECK(Str(*m, "animationName") == "Deploy");
    rig.anim->AddClip("Deploy", 2.0f);
    int moves = 0, stops = 0;
    float mf = -1, mt = -1, ms = -1;
    struct L {
        int* moves;
        int* stops;
        float *mf, *mt, *ms;
        void M(float a, float b) {
            (*moves)++;
            *mf = a;
            *mt = b;
        }
        void S(float a) {
            (*stops)++;
            *ms = a;
        }
    } l{&moves, &stops, &mf, &mt, &ms};
    // the cargo bay subscribes through IScalarModule (ModuleCargoBay.cs:136)
    IScalarModule* sc = m->AsScalarModule();
    REQUIRE(sc != nullptr);
    sc->OnMoving()->Add(&l, &L::M);
    sc->OnStop()->Add(&l, &L::S);
    m->OnStart(StartState::Flying);
    auto* st = m->GetState();
    REQUIRE(st != nullptr);
    CHECK(st->speed == 0.0f);
    CHECK(st->enabled);
    CHECK(st->weight == 1.0f);
    CHECK(st->layer == m->Get<int32_t>("layer"));
    CHECK(m->aniState() == ModuleAnimateGeneric::animationStates::LOCKED);
    CHECK(m->scalarLerpMaxDelta == 0.5f);  // 1 / length
    // OnStart drag state: animTime 0 -> A 0, B 1
    CHECK(rig.Weight("A") == 0.0f);
    CHECK(rig.Weight("B") == 1.0f);
    // Toggle: animSwitch true -> false, speed +1 (the animation goes to the end), MOVING, OnMoving(0, 1)
    CHECK(m->Get<bool>("animSwitch"));
    m->Toggle();
    CHECK_FALSE(m->Get<bool>("animSwitch"));
    CHECK(m->aniState() == ModuleAnimateGeneric::animationStates::MOVING);
    CHECK(st->speed == 1.0f);
    CHECK(moves == 1);
    CHECK(mf == 0.0f);
    CHECK(mt == 1.0f);
    CHECK(m->Get<std::string>("status") == Localizer::Format("#autoLOC_215506"));
    rig.anim->Tick(1.0f);  // half of the 2 s clip
    m->FixedUpdate();
    CHECK(m->Get<float>("animTime") == 0.5f);
    CHECK(m->Get<float>("animSpeed") == 1.0f);
    CHECK(rig.Weight("A") == 0.5f);
    CHECK(rig.Weight("B") == 0.5f);
    CHECK(sc->GetScalar() == 0.5f);
    CHECK(sc->IsMoving());
    rig.anim->Tick(1.5f);  // the Once clip ends and rewinds
    CHECK_FALSE(rig.anim->isPlaying());
    m->FixedUpdate();
    CHECK(m->aniState() == ModuleAnimateGeneric::animationStates::LOCKED);
    CHECK(m->Get<float>("animTime") == 1.0f);  // animSwitch false: end position
    CHECK(st->normalizedTime() == 1.0f);
    CHECK(stops == 1);
    CHECK(ms == 1.0f);
    CHECK(rig.Weight("A") == 1.0f);
    CHECK(rig.Weight("B") == 0.0f);
    CHECK(m->Get<std::string>("status") == Localizer::Format("#autoLOC_215719"));
    // toggle back: speed -1 from the end
    st->set_normalizedTime(1.0f);
    m->Toggle();
    CHECK(m->Get<bool>("animSwitch"));
    CHECK(st->speed == -1.0f);
    CHECK(mf == 1.0f);
    CHECK(mt == 0.0f);
    // in the editor with instantAnimInEditor the speed is +-10 * length
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    m->Toggle();
    CHECK(st->speed == 20.0f);
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    // allowManualControl false: ToggleAction and Toggle do nothing; the cargo bay's failure hooks flip it and restore it
    m->Set<bool>("allowManualControl", false);
    const bool before = m->Get<bool>("animSwitch");
    m->Toggle();
    CHECK(m->Get<bool>("animSwitch") == before);
    m->Set<bool>("allowManualControl", true);
    m->Set<float>("deployPercent", 80.0f);
    m->Set<float>("animSpeed", 0.5f);
    m->FailAnimationForCargoBay(40.0f, true);
    CHECK_FALSE(m->Get<bool>("allowManualControl"));
    CHECK(m->revertDeployPercentOnSave == 40.0f);
    CHECK(m->revertAnimSpeedOnSave == 0.5f);
    // OnSave rewrites deployPercent / animSpeed / aniState when frozen mid-movement
    ConfigNode node("MODULE");
    node.AddValue("deployPercent", "80");
    node.AddValue("animSpeed", "0.5");
    node.AddValue("aniState", "LOCKED");
    m->OnSave(node);
    CHECK(*node.GetValue("deployPercent") == "40");
    CHECK(*node.GetValue("animSpeed") == "0.5");
    CHECK(*node.GetValue("aniState") == "MOVING");
    m->RestartFailedAnimationForCargoBay();
    CHECK(m->Get<bool>("allowManualControl"));
    CHECK(m->Get<float>("deployPercent") == 40.0f);
    CHECK(m->revertDeployPercentOnSave == -1.0f);
    CHECK(m->aniState() == ModuleAnimateGeneric::animationStates::MOVING);
    // SetScalar (scalar module drive): MoveTowards(animTime, t, scalarLerpMaxDelta * Time.deltaTime), snap states near 0 / 1
    m->Set<bool>("isOneShot", false);
    m->Set<float>("animTime", 0.5f);
    sc->SetScalar(1.0f);
    CHECK(m->Get<float>("animTime") == Mathf::MoveTowards(0.5f, 1.0f, static_cast<float>(fx(0.5f) * fx(Time::deltaTime))));
    CHECK(m->aniState() == ModuleAnimateGeneric::animationStates::MOVING);
    m->Set<float>("animTime", 0.999f);
    sc->SetScalar(1.0f);
    CHECK(m->aniState() == ModuleAnimateGeneric::animationStates::LOCKED);
    CHECK_FALSE(m->Get<bool>("animSwitch"));
    // isOneShot -> FIXED at the end
    m->Set<bool>("isOneShot", true);
    m->Set<float>("animTime", 0.999f);
    sc->SetScalar(1.0f);
    CHECK(m->aniState() == ModuleAnimateGeneric::animationStates::FIXED);
    // the module's drag cube names
    CHECK(m->GetDragCubeNames() == std::vector<std::string>{"A", "B"});
    sc->OnMoving()->Remove(&l, &L::M);
    sc->OnStop()->Remove(&l, &L::S);
}

TEST_CASE("WP-27 ModuleAnimateGeneric: every stock instance has the fields of its cfg; deploy-percent clamp; crew capacity") {
    if (!GameAvailable()) return;
    World27 w;
    // every ModuleAnimateGeneric of every stock part: animationName non-empty, toggle event / action bound, action group resolved
    int n = 0;
    for (const char* part : {"InflatableHeatShield", "mk2LanderCabin.v2", "JetEngine", "turboFanSize2", "GooExperiment", "science.module", "dockingPortLateral", "dockingPort1",
                             "GrapplingDevice", "mk2CargoBayL", "mk2CargoBayS", "mk2DockingPort", "mk3CargoBayL", "mk3CargoBayM", "mk3CargoRamp", "mk3CargoBayS",
                             "ServiceBay.125.v2", "ServiceBay.250.v2", "smallClaw"}) {
        auto p = w.game.New(part);
        for (size_t i = 0; i < p->Modules().Count(); i++) {
            auto* m = dynamic_cast<ModuleAnimateGeneric*>(p->Modules()[i]);
            if (m == nullptr) continue;
            CAPTURE(part);
            CHECK_FALSE(Str(*m, "animationName").empty());
            REQUIRE(m->toggleEvt != nullptr);
            REQUIRE(m->toggleAct != nullptr);
            // OnAwake: REPLACEWITHDEFAULT -> defaultActionGroup
            const int64_t dg = m->Get<EnumValue>("defaultActionGroup").value;
            CHECK(m->toggleAct->actionGroup == dg);
            CHECK(m->toggleAct->defaultActionGroup == dg);
            n++;
        }
    }
    CHECK(n == 19);
    // deployPercent clamp (ModuleAnimateGeneric.cs:402): a changed deployPercent clamps the animation at num = deployPercent * 0.01 (> 0.995 -> 1)
    Rig27 rig(w.game, "mk3CargoBayL");
    auto* m = rig.Mod<ModuleAnimateGeneric>("ModuleAnimateGeneric");
    rig.anim->AddClip(Str(*m, "animationName"), 3.0f);
    m->OnStart(StartState::Flying);
    m->Set<float>("deployPercent", 50.0f);
    m->FixedUpdate();
    CHECK(m->aniState() == ModuleAnimateGeneric::animationStates::CLAMPED);
    CHECK(m->Get<float>("animTime") == 0.5f);
    CHECK(m->Get<float>("animSpeed") == 0.0f);
    CHECK(m->Get<std::string>("status") == Localizer::Format("#autoLOC_215639"));
    CHECK(rig.Weight("A") == 0.5f);
    // revClampPercent (the cargo bays have it) mirrors the clamp: num = 1 - num
    const bool revPct = m->Get<bool>("revClampPercent"), revDir = m->Get<bool>("revClampDirection");
    m->Set<float>("deployPercent", 99.8f);
    m->FixedUpdate();
    CHECK(m->Get<float>("animTime") == (revPct ? 0.0f : 1.0f));  // 99.8 % > 0.995 -> 1
    m->Set<float>("deployPercent", 0.0f);
    m->FixedUpdate();
    CHECK(m->Get<float>("animTime") == (revPct ? 1.0f : 0.0f));
    // the toggle state follows deployPercent: revClampDirection: > 0 -> false; else == 0 -> false
    CHECK(m->Get<bool>("animSwitch") == (revDir ? true : false));
}

TEST_CASE("WP-27 ModuleAnimationGroup: deploy / retract states, the IAnimatedModule fan-out to converters, drag cubes, events") {
    if (!GameAvailable()) return;
    World27 w;
    Rig27 rig(w.game, "MiniDrill");
    auto* g = rig.Mod<ModuleAnimationGroup>("ModuleAnimationGroup");
    CHECK(Str(*g, "deployAnimationName") == "Deploy");
    CHECK(Str(*g, "activeAnimationName") == "Drill");
    rig.anim->AddClip("Deploy", 2.0f);
    rig.anim->AddClip("Drill", 1.0f);
    // two animation components would be one each in the model; the rig serves one Animation with both clips (FindModelAnimators filters by clip)
    CHECK(g->GetDragCubeNames() == std::vector<std::string>{"DEPLOY_A", "DEPLOY_B", "ACTIVE_A", "ACTIVE_B"});
    int stateEvents = 0;
    bool lastState = false;
    struct L {
        int* n;
        bool* s;
        void E(ModuleAnimationGroup*, bool st) {
            (*n)++;
            *s = st;
        }
    } l{&stateEvents, &lastState};
    GameEvents::OnAnimationGroupStateChanged.Add(&l, &L::E);
    g->Start();
    // Start: layers, modules found (IAnimatedModule: the harvester is a BaseConverter, the drills too), Setup localises the gui names
    CHECK(g->hasModules);
    CHECK(g->_Modules.size() >= 3);
    CHECK((*rig.anim)["Deploy"]->layer == 3);
    CHECK((*rig.anim)["Drill"]->layer == 4);
    CHECK_FALSE(g->IsDeployed());
    // retracted at start: the converters are disabled (LateSetup: DisableModules)
    g->LateUpdate();
    for (IAnimatedModule* am : g->_Modules)
        if (auto* conv = dynamic_cast<BaseConverter*>(dynamic_cast<PartModule*>(am))) CHECK_FALSE(conv->isEnabled);
    // DeployModule: isDeployed, DeployModule event off, the deploy animation plays at speed 1; DeployComplete after the clip (flight: animation lock)
    stateEvents = 0;
    g->DeployModule();
    CHECK(g->IsDeployed());
    CHECK_FALSE(g->Events()["DeployModule"]->active);
    CHECK((*rig.anim)["Deploy"]->speed == 1.0f);
    CHECK(rig.anim->IsPlaying("Deploy"));
    CHECK(g->_animationLock);
    CHECK(rig.Weight("DEPLOY_B") == 1.0f);
    CHECK(rig.Weight("DEPLOY_A") == 0.0f);
    CHECK(AnimationModulesEnvironment::partEffects.back().first == "Deploy");
    // Update while the clip plays: nothing; once it stops the post-animation method runs (DeployComplete: RetractModule on, modules enabled)
    rig.pv.parts = {rig.host.get()};
    g->Update();
    CHECK(g->_animationLock);
    rig.anim->Tick(5.0f);
    g->Update();
    CHECK_FALSE(g->_animationLock);
    CHECK(g->deployCompleteCalls == 1);
    CHECK(g->Events()["RetractModule"]->active);
    CHECK(stateEvents >= 1);
    CHECK(lastState);
    for (IAnimatedModule* am : g->_Modules)
        if (am->IsSituationValid()) CHECK(dynamic_cast<PartModule*>(am)->isEnabled);  // EnableModules ran for every module valid in this situation
    // activity: a deployed group plays the Drill clip while some module is active (or alwaysActive), else the (absent) deactivate clip
    g->Set<bool>("alwaysActive", true);
    g->Update();
    CHECK(rig.anim->IsPlaying("Drill"));
    CHECK((*rig.anim)["Drill"]->speed == 1.0f);
    CHECK(rig.Weight("ACTIVE_A") == 1.0f);
    CHECK(g->savedAnimationName == "Drill");
    // RetractModule: speed -1, deploy clip set to its end first (resetAnimationTime), active clip stopped
    g->RetractModule();
    CHECK_FALSE(g->IsDeployed());
    CHECK((*rig.anim)["Deploy"]->speed == -1.0f);
    CHECK((*rig.anim)["Deploy"]->time == (*rig.anim)["Deploy"]->length);
    CHECK_FALSE(rig.anim->IsPlaying("Drill"));
    CHECK(rig.Weight("DEPLOY_A") == 1.0f);
    CHECK(lastState == false);
    rig.anim->Tick(10.0f);
    g->Update();
    CHECK(g->retractCompleteCalls >= 1);
    CHECK(g->Events()["DeployModule"]->active);
    // the actions
    KSPActionParam p;
    g->DeployModuleAction(p);
    CHECK(g->IsDeployed());
    g->DeployModuleAction(p);  // already deployed: no second DeployModule
    g->ToggleModuleAction(p);
    CHECK_FALSE(g->IsDeployed());
    g->RetractModuleAction(p);  // already retracted
    CHECK_FALSE(g->IsDeployed());
    // BaseConverter's animation gate (WP-28) now reads the real group
    GameEvents::OnAnimationGroupStateChanged.Remove(&l, &L::E);
    // AssumeDragCubePosition: the named clip goes to its end / start with speed 0
    g->AssumeDragCubePosition("ACTIVE_B");
    CHECK((*rig.anim)["Drill"]->normalizedTime() == 1.0f);
    g->AssumeDragCubePosition("DEPLOY_A");
    CHECK((*rig.anim)["Deploy"]->normalizedTime() == 0.0f);
}

TEST_CASE("WP-27 ModuleAnimationGroup + BaseConverter: a converter of a part with an animation group starts only when the group is deployed") {
    if (!GameAvailable()) return;
    World27 w;
    Rig27 rig(w.game, "ISRU");
    auto* g = rig.Mod<ModuleAnimationGroup>("ModuleAnimationGroup");
    auto* conv = rig.Mod<ModuleResourceConverter>("ModuleResourceConverter");
    CHECK(Str(*g, "deployAnimationName").empty());  // ISRU: only the running clip
    g->Set<bool>("isDeployed", false);
    conv->hasAnimationGroup = true;
    conv->partAnimationModule = g;
    ResourceProcessingEnvironment::screenMessages.clear();
    conv->StartResourceConverter();
    CHECK_FALSE(conv->Get<bool>("IsActivated"));  // IsAnimationGroupDeployed: the real group, not deployed
    g->Set<bool>("isDeployed", true);
    conv->StartResourceConverter();
    CHECK(conv->Get<bool>("IsActivated"));
}

TEST_CASE("WP-27 ModuleAnimationSetter / ModuleAnimatorLandingGear") {
    if (!GameAvailable()) return;
    World27 w;
    // ModuleAnimationSetter: animStateCurve default (0,0)(1,1), SetScalar ignores NaN, UpdateAnim writes normalizedTime of every state
    auto host = w.game.New("parachuteSingle");
    PartModuleData* md = host->AddModule("ModuleAnimationSetter", true);
    REQUIRE(md != nullptr);
    auto* s = As<ModuleAnimationSetter>(md->module.get());
    CHECK(s->IsNull("animStateCurve") == false);
    CHECK(s->Get<FloatCurve>("animStateCurve").Evaluate(0.25f) == 0.25f);
    CHECK(s->Get<std::string>("animName") == "HeatAnimationEmissive");
    auto a1 = std::make_shared<Animation>(), a2 = std::make_shared<Animation>();
    a1->AddClip("HeatAnimationEmissive", 3.0f);
    a2->AddClip("HeatAnimationEmissive", 5.0f);
    DeployablesEnvironment::modelAnimations = [&](PartModule&) { return std::vector<std::shared_ptr<Animation>>{a1, a2}; };
    s->OnStart(StartState::Flying);
    CHECK(s->stateCount == 2);
    CHECK((*a1)["HeatAnimationEmissive"]->enabled);
    CHECK((*a1)["HeatAnimationEmissive"]->speed == 0.0f);
    CHECK((*a1)["HeatAnimationEmissive"]->layer == s->Get<int32_t>("layer"));
    s->SetScalar(std::nanf(""));
    CHECK(s->GetScalar() == 0.0f);
    s->SetScalar(0.5f);
    CHECK(s->GetScalar() == 0.5f);
    CHECK(s->animState == 0.5f);
    s->Update();
    CHECK((*a1)["HeatAnimationEmissive"]->normalizedTime() == 0.5f);
    CHECK((*a2)["HeatAnimationEmissive"]->normalizedTime() == 0.5f);
    CHECK(s->CanMove());
    CHECK_FALSE(s->IsMoving());
    CHECK(s->AsScalarModule() == static_cast<IScalarModule*>(s));
    // ModuleAnimatorLandingGear: gearExtension toggles 0 <-> 1 and the event gui name follows
    PartModuleData* md2 = host->AddModule("ModuleAnimatorLandingGear", true);
    REQUIRE(md2 != nullptr);
    auto* lg = As<ModuleAnimatorLandingGear>(md2->module.get());
    lg->OnStart(StartState::Flying);
    CHECK(lg->Events()["InputGearToggle"]->guiName() == Localizer::Format("#autoLOC_6001870"));
    lg->InputGearToggle();
    CHECK(lg->Get<float>("gearExtension") == 1.0f);
    CHECK(lg->Events()["InputGearToggle"]->guiName() == Localizer::Format("#autoLOC_6001869"));
    lg->InputGearToggle();
    CHECK(lg->Get<float>("gearExtension") == 0.0f);
    DeployablesEnvironment::modelAnimations = nullptr;
}

// ============================================== the Unity Animation model ======================================================

TEST_CASE("WP-27 UnityEngine.Animation model: Play / Stop / wrap modes / normalizedTime") {
    Animation a;
    AnimationState& s = a.AddClip("c", 4.0f);
    CHECK(a.GetClip("c"));
    CHECK_FALSE(a.GetClip("d"));
    CHECK(a["d"] == nullptr);
    CHECK_FALSE(a.isPlaying());
    s.speed = 1.0f;
    s.set_normalizedTime(0.5f);
    CHECK(s.time == 2.0f);
    CHECK(a.Play("c"));
    CHECK_FALSE(a.Play("d"));
    CHECK(a.IsPlaying("c"));
    a.Tick(1.0f);
    CHECK(s.normalizedTime() == 0.75f);
    // Once: stops at the end and rewinds
    a.Tick(1.0f);
    CHECK_FALSE(a.isPlaying());
    CHECK(s.time == 0.0f);
    // ClampForever: keeps playing at the end; backwards clamps at 0
    s.wrapMode = WrapMode::ClampForever;
    s.enabled = true;
    a.Tick(10.0f);
    CHECK(a.isPlaying());
    CHECK(s.normalizedTime() == 1.0f);
    s.speed = -1.0f;
    a.Tick(10.0f);
    CHECK(s.normalizedTime() == 0.0f);
    CHECK(a.isPlaying());
    // Stop rewinds; Stop(name) of an unknown clip is a no-op
    s.time = 3.0f;
    a.Stop("c");
    CHECK(s.time == 0.0f);
    CHECK_FALSE(s.enabled);
    a.Stop("zz");
    // Play stops the other states of the same layer
    AnimationState& o = a.AddClip("o", 2.0f);
    a.Play("c");
    o.layer = 1;
    a.Play("o");
    CHECK(a.IsPlaying("c"));
    o.layer = 0;
    a.Play("o");
    CHECK_FALSE(a.IsPlaying("c"));
    // Loop wraps, PingPong reflects
    o.wrapMode = WrapMode::Loop;
    o.speed = 1.0f;
    o.time = 1.5f;
    a.Tick(1.0f);
    CHECK(o.time == 0.5f);
    o.wrapMode = WrapMode::PingPong;
    o.time = 1.5f;
    a.Tick(1.0f);
    CHECK(o.time == 1.5f);
    CHECK(o.speed == -1.0f);
    // a zero-length clip has normalizedTime 0
    AnimationState& z = a.AddClip("z", 0.0f);
    z.time = 3.0f;
    CHECK(z.normalizedTime() == 0.0f);
}
