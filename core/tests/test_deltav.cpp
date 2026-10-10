// WP-17 unit tests: the stock delta-v calculation (VesselDeltaV, DeltaVStageInfo, DeltaVEngineInfo, DeltaVPartInfo, DeltaVEngineStageSet and the
// small files).
//
// The numerics of the 17 crafts of the C5 capture are gated by `capture.deltav_stages` (stages, totals, DeltaVPartInfo staging columns) and
// `capture.engine_deltav` (engines). Everything here is a property that follows from the decompiled control flow, checked on the game's own
// crafts (harness/inputs/Ships, analysis/physics/craft) and part database: the stage list order and contents, the sums VesselDeltaV
// builds, the Tsiolkovsky bounds a burn simulation must respect, vacuum bodies, the situation switch, the engine-less craft. No number is
// invented; every expected value is computed from the decompiled formula over the ship / part values.
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "editor_staging.hpp"
#include "resource_fixture.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/parts/compound_part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/delta_v.hpp"
#include "ksp/vessel/vessel.hpp"

using namespace ksp;
using namespace ksp::rtest;
namespace fs = std::filesystem;

namespace {

const std::string kHarness17 = KSPCORE_HARNESS_DIR;

// The stock game + PhysicsGlobals + Planetarium + the Kerbol system (the bodies of DeltaVAppValues), as the editor scene has them
struct World17 {
    Game15 g;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    World17() {
        SaveEnvironment& env = SaveEnvironment::Current();
        pg.physicsDatabaseFilename = kHarness17 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        env.loadedScene = GameScenes::EDITOR;  // HighLogic.LoadedScene
        DeltaVGlobals::homeBody = &Kerbin();
        DeltaVGlobals::ResetPropellantsToIgnore();
    }
    ~World17() {
        DeltaVGlobals::appValues.reset();
        DeltaVGlobals::homeBody = nullptr;
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
        SaveEnvironment::Current().loadedScene = GameScenes::MAINMENU;
    }
    CelestialBody& Body(const char* n) { return *sys->GetBodyByName(n); }
    CelestialBody& Kerbin() { return Body("Kerbin"); }
};

// One craft in the editor: ShipConstruct.LoadShip with live parts, Part.Start of every part, StageManager's sorting, VesselDeltaV.Create
struct EditorCraft {
    ShipConstruct ship;
    std::unique_ptr<support::EditorSession> editor;
    std::unique_ptr<VesselDeltaV> dv;
    EditorCraft(World17&, const fs::path& file, const CelestialBody& body) {
        ConfigNode::NodePtr root = ConfigNode::Load(file.string());
        REQUIRE_MESSAGE(root, file.string());
        ConfigNode::NodePtr upgraded;
        const std::string* sn = root->GetValuePtr("ship");
        KSPUpgradePipeline::Process(root, sn ? *sn : std::string(), SaveUpgradePipeline::LoadContext::Craft,
                                    [&](const ConfigNode::NodePtr& n) { upgraded = n; },
                                    [&](KSPUpgradePipeline::UpgradeFailOption, const ConfigNode::NodePtr&) {});
        REQUIRE(upgraded);
        std::string err;
        REQUIRE_MESSAGE(ship.LoadShipWithParts(*upgraded, 0, true, err), err);
        editor = std::make_unique<support::EditorSession>(ship.liveParts);
        support::EditorSession::StartParts(ship.liveParts);
        editor->Load();
        // the thrust directions come from the model (not ported): one transform along the part's -Y per engine
        for (Part* p : ship.liveParts)
            for (size_t i = 0; i < p->Modules().Count(); i++)
                if (auto* e = dynamic_cast<ModuleEngines*>(p->Modules().GetModule(i))) {
                    PartTransform t;
                    t.position = p->partTransform.position;
                    t.rotation = Quaternionf::Normalize(p->partTransform.rotation * UnityNative::AngleAxis(90.0f, Vector3f::right()));
                    e->thrustTransforms = {t};
                    e->thrustTransformMultipliers = {1.0f};
                }
        CompoundPart::hooks.editorShipParts = [this]() { return ship.liveParts; };
        for (Part* p : ship.liveParts)
            if (auto* cp = dynamic_cast<CompoundPart*>(p)) cp->OnStartComplete();
        CompoundPart::hooks.editorShipParts = nullptr;
        DeltaVGlobals::appValues = std::make_unique<DeltaVAppValues>(DeltaVGlobals::homeBody);
        DeltaVGlobals::appValues->body = &body;
        dv = VesselDeltaV::Create(&ship);
        dv->Start();
        dv->DriveToCompletion();
    }
    void SetBody(const CelestialBody& b) {
        DeltaVGlobals::appValues->body = &b;
        dv->SetCalcsDirty(false);
        dv->DriveToCompletion();
    }
    ~EditorCraft() {
        dv.reset();
        DeltaVGlobals::appValues.reset();
    }
};

fs::path PhysProfS() { return fs::path(kHarness17) / "inputs" / "Ships" / "VAB" / "PhysProf-S.craft"; }
fs::path BugE() { return fs::path(kHarness17) / "inputs" / "Ships" / "SPH" / "Bug-E Buggy.craft"; }
fs::path KerbalX() { return fs::path(kHarness17) / "inputs" / "Ships" / "VAB" / "Kerbal X.craft"; }

}  // namespace

TEST_CASE("WP-17 DeltaVSituationOptions / GetSwitchedValue (DeltaVExtensions.cs:3)") {
    CHECK(GetSwitchedValue(DeltaVSituationOptions::SeaLevel, 1, 2, 3) == 1);
    CHECK(GetSwitchedValue(DeltaVSituationOptions::Altitude, 1, 2, 3) == 2);
    CHECK(GetSwitchedValue(DeltaVSituationOptions::Vaccum, 1, 2, 3) == 3);
    CHECK(static_cast<int>(DeltaVSituationOptions::SeaLevel) == 0);
    CHECK(static_cast<int>(DeltaVSituationOptions::Altitude) == 1);
    CHECK(static_cast<int>(DeltaVSituationOptions::Vaccum) == 2);
}

TEST_CASE("WP-17 DeltaVGlobals: PropellantsToIgnore (DeltaVGlobals.cs:60-90), Enable / DisableStockSimluations (:129, :140)") {
    if (!GameAvailable()) return;
    DeltaVGlobals::ResetPropellantsToIgnore();
    const int ia = MonoStringHash("IntakeAir"), ec = MonoStringHash("ElectricCharge");
    const std::vector<int>& d = DeltaVGlobals::PropellantsToIgnore();
    REQUIRE(d.size() == 2);
    CHECK(d[0] == ia);
    CHECK(d[1] == ec);
    CHECK(DeltaVGlobals::PropellantIgnored(ia));
    CHECK_FALSE(DeltaVGlobals::PropellantIgnored(MonoStringHash("LiquidFuel")));
    // the setter keeps the caller's list and appends the two missing entries
    DeltaVGlobals::SetPropellantsToIgnore({MonoStringHash("XenonGas"), ec});
    const std::vector<int>& s = DeltaVGlobals::PropellantsToIgnore();
    REQUIRE(s.size() == 3);
    CHECK(s[0] == MonoStringHash("XenonGas"));
    CHECK(s[1] == ec);
    CHECK(s[2] == ia);
    DeltaVGlobals::ResetPropellantsToIgnore();
    DeltaVGameSettings::DELTAV_CALCULATIONS_ENABLED = false;
    CHECK_FALSE(DeltaVGlobals::EnableStockSimluations());
    DeltaVGameSettings::DELTAV_CALCULATIONS_ENABLED = true;
    CHECK(DeltaVGlobals::EnableStockSimluations());
    CHECK(DeltaVGlobals::DoStockSimulations());
    CHECK(DeltaVGlobals::DisableStockSimluations());
    CHECK_FALSE(DeltaVGlobals::DoStockSimulations());
    DeltaVGlobals::EnableStockSimluations();
}

TEST_CASE("WP-17 GameSettings defaults of the delta-v simulation (GameSettings.cs:1080-1088)") {
    CHECK(DeltaVGameSettings::DELTAV_CALCULATIONS_TIMESTEP == 5.0f);
    CHECK(DeltaVGameSettings::DELTAV_CALCULATIONS_BIGTIMESTEP == 100.0f);
    CHECK(DeltaVGameSettings::DELTAV_ACTIVE_VESSEL_TIMESTEP == 5.0f);
    CHECK(DeltaVGameSettings::DELTAV_ACTIVE_STAGE_UPDATE_SECS == 0.2f);
    CHECK(DeltaVGameSettings::DELTAV_ALL_STAGES_UPDATE_SECS == 4.0f);
}

TEST_CASE("WP-17 DeltaVAppValues: atmPressure / atmDensity (DeltaVAppValues.cs:81-83)") {
    if (!GameAvailable()) return;
    World17 w;
    DeltaVAppValues v(&w.Kerbin());
    CHECK(v.situation == DeltaVSituationOptions::Vaccum);  // :88
    CHECK(v.altitude == 0.0);
    CHECK(v.atmPressure() == static_cast<float>(w.Kerbin().GetPressure(0.0) * 0.009869232667160128));
    CHECK(v.atmDensity() == w.Kerbin().GetDensity(w.Kerbin().GetPressure(0.0), w.Kerbin().GetTemperature(0.0)));
    CHECK(v.atmPressure() > 0.99f);
    v.body = &w.Body("Mun");
    CHECK(v.atmPressure() == 0.0f);  // no atmosphere
    CHECK(v.atmDensity() == 0.0);
}

TEST_CASE("WP-17 PhysProf-S in the editor: stage list, part staging, sums (VesselDeltaV.cs:853 RunCalculations)") {
    if (!GameAvailable()) return;
    World17 w;
    EditorCraft c(w, PhysProfS(), w.Kerbin());
    VesselDeltaV& dv = *c.dv;
    CHECK(dv.IsReady());
    CHECK(dv.RunCount() == 2);          // the first run sets syncListInstances = false and marks the calculations dirty again (:927-931)
    CHECK(dv.CompletedEvents() == 1);   // only the second run fires onDeltaVCalcsCompleted
    CHECK_FALSE(dv.SyncListInstances());
    CHECK_FALSE(dv.CalcsDirty());
    const std::vector<DeltaVStageInfoPtr> stages = dv.OperatingStageInfo();
    REQUIRE(stages.size() == 2);
    // DeltaVEngineStageSet.cs:104: sorted by stage, highest first
    CHECK(stages[0]->stage == StageManager::LastStage());
    CHECK(stages[1]->stage == 0);
    CHECK(stages[0]->stage > stages[1]->stage);
    // the totals are the sums of the stages (VesselDeltaV.cs:895-915), in the stage order
    double vac = 0.0, asl = 0.0, act = 0.0, burn = 0.0;
    for (const auto& s : stages) {
        vac += s->deltaVinVac;
        asl += s->deltaVatASL;
        act += s->deltaVActual;  // stageDVActual: the editor with DeltaVAppValues
        burn += s->stageBurnTime;
    }
    CHECK(dv.TotalDeltaVVac() == vac);
    CHECK(dv.TotalDeltaVASL() == asl);
    CHECK(dv.TotalDeltaVActual() == act);
    CHECK(dv.TotalBurnTime() == burn);
    CHECK(dv.GetSituationTotalDeltaV(DeltaVSituationOptions::Vaccum) == vac);
    CHECK(dv.GetSituationTotalDeltaV(DeltaVSituationOptions::SeaLevel) == asl);
    // part staging: every ship part has a DeltaVPartInfo except launch clamps; engines are flagged, decouplers have their module
    int engines = 0, decouplers = 0;
    for (Part* p : c.ship.liveParts) {
        DeltaVPartInfo* info = deltav_ext::Get(dv.PartInfo(), p);
        if (p->isLaunchClamp()) {
            CHECK(info == nullptr);
            continue;
        }
        REQUIRE(info != nullptr);
        CHECK(info->part == p);
        CHECK(info->isEngine == p->HasModuleImplementing([](PartModule& m) { return dynamic_cast<ModuleEngines*>(&m) != nullptr; }));
        CHECK(info->isDecoupler == (p->FindModuleOfClass("ModuleDecouple") != nullptr || p->FindModuleOfClass("ModuleAnchoredDecoupler") != nullptr));
        CHECK(info->activationStage >= info->decoupleStage);
        if (info->isEngine) engines++;
        if (info->isDecoupler) decouplers++;
    }
    CHECK(engines == 3);
    CHECK(decouplers >= 1);
    // the first (highest) stage burns the three engines, stage 0 holds no active engine
    CHECK(stages[0]->enginesInStage.size() == 3);
    CHECK(stages[0]->enginesActiveInStage.size() == 3);
    CHECK(stages[1]->enginesActiveInStage.empty());
    CHECK(stages[1]->deltaVinVac == 0.0f);
    CHECK(stages[1]->endMass == stages[1]->startMass);  // DeltaVStageInfo.cs:855: no engines, endMass = startMass
}

TEST_CASE("WP-17 PhysProf-S: burn simulation respects the rocket equation (DeltaVStageInfo.cs:331, :531)") {
    if (!GameAvailable()) return;
    World17 w;
    EditorCraft c(w, PhysProfS(), w.Kerbin());
    const std::vector<DeltaVStageInfoPtr> stages = c.dv->OperatingStageInfo();
    REQUIRE(stages.size() == 2);
    const DeltaVStageInfo& s = *stages[0];
    // the burn removes propellant only: the stage loses mass (DeltaVStageInfo.cs:1017 GetCurrentStageMass)
    CHECK(s.endMass < s.startMass);
    CHECK(s.endMass > 0.0f);
    CHECK(s.stageBurnTime > 0.0);
    // the mass the stage starts with is the craft (no crew in the unmanned PhysProf craft): GetShipMass
    float dry = 0.0f, fuel = 0.0f;
    c.ship.GetShipMass(dry, fuel);
    CHECK(s.startMass == f32store(fx(dry) + fx(fuel)));
    // Tsiolkovsky: every engine of the stage has an Isp between the slowest and the fastest, so the summed interim burns are bounded
    double ispMin = 1e30, ispMax = 0.0;
    for (const auto& e : s.enginesActiveInStage) {
        ispMin = std::min(ispMin, e->ispVac);
        ispMax = std::max(ispMax, e->ispVac);
    }
    const double g = PhysicsGlobals::GravitationalAcceleration;
    const double lnRatio = std::log(static_cast<double>(s.startMass) / static_cast<double>(s.endMass));
    CHECK(s.ispVac >= ispMin);
    CHECK(s.ispVac <= ispMax);
    CHECK(static_cast<double>(s.deltaVinVac) <= ispMax * g * lnRatio * 1.0001);
    CHECK(static_cast<double>(s.deltaVinVac) >= ispMin * g * lnRatio * 0.9999);
    // the atmosphere only loses: sea level thrust and Isp are below vacuum, so are delta-v and TWR
    CHECK(s.thrustASL < s.thrustVac);
    CHECK(s.ispASL < s.ispVac);
    CHECK(s.deltaVatASL < s.deltaVinVac);
    CHECK(s.TWRASL < s.TWRVac);
    // TWR (DeltaVStageInfo.cs:1152): thrust / (startMass * ((float)g * GeeASL)), the Mono float rule
    const float gF = f32store(g);
    const float gee = f32store(w.Kerbin().GeeASL);
    CHECK(s.TWRVac == f32store(fx(s.thrustVac) / (fx(s.startMass) * (fx(gF) * fx(gee)))));
    CHECK(s.TWRASL == f32store(fx(s.thrustASL) / (fx(s.startMass) * (fx(gF) * fx(gee)))));
    // the situation getters (DeltaVStageInfo.cs:142-172)
    CHECK(s.GetSituationDeltaV(DeltaVSituationOptions::SeaLevel) == s.deltaVatASL);
    CHECK(s.GetSituationDeltaV(DeltaVSituationOptions::Altitude) == s.deltaVActual);
    CHECK(s.GetSituationDeltaV(DeltaVSituationOptions::Vaccum) == s.deltaVinVac);
    CHECK(s.GetSituationISP(DeltaVSituationOptions::Vaccum) == s.ispVac);
    CHECK(s.GetSituationThrust(DeltaVSituationOptions::SeaLevel) == s.thrustASL);
    CHECK(s.GetSituationTWR(DeltaVSituationOptions::Altitude) == s.TWRActual);
}

TEST_CASE("WP-17 PhysProf-S: DeltaVEngineInfo isp / thrust / propellants (DeltaVEngineInfo.cs:610, :687)") {
    if (!GameAvailable()) return;
    World17 w;
    EditorCraft c(w, PhysProfS(), w.Kerbin());
    const std::vector<DeltaVEngineInfoPtr>& engines = c.dv->OperatingEngineInfo();
    REQUIRE(engines.size() == 3);
    const float ambient = DeltaVGlobals::DeltaVAppValuesPtr()->atmPressure();
    for (const auto& e : engines) {
        // the engine info reads exactly the module's curve and thrust functions
        const FloatCurve curve = e->engine->Get<FloatCurve>("atmosphereCurve");
        CHECK(e->ispVac == static_cast<double>(curve.Evaluate(0.0f)));
        CHECK(e->ispActual == static_cast<double>(curve.Evaluate(ambient)));
        CHECK(e->ispASL == static_cast<double>(curve.Evaluate(f32store(w.Kerbin().atmPressureASL))));
        CHECK(e->thrustVac == e->engine->MaxThrustOutputVac());
        CHECK(e->startBurnStage == static_cast<Part*>(e->engine->part)->fields.inverseStage);
        CHECK_FALSE(e->requiresAir);
        CHECK(e->RequiresAir() == false);
        REQUIRE(e->propellantInfo.size() == 2);  // LiquidFuel, Oxidizer
        CHECK(e->propellantInfo[0].propellant->name == "LiquidFuel");
        CHECK(e->propellantInfo[1].propellant->name == "Oxidizer");
        // the thrust vector of the stand-in thrust transform has the engine's thrust as its length (unit direction)
        CHECK(std::fabs(static_cast<double>(e->thrustVectorVac.magnitude()) - static_cast<double>(e->thrustVac)) < 1e-3 * static_cast<double>(e->thrustVac));
        // the lookups of DeltaVEngineExtensions.cs
        CHECK(deltav_ext::Contains(engines, static_cast<Part*>(e->engine->part)));
        CHECK(deltav_ext::Get(engines, static_cast<Part*>(e->engine->part)) == e.get());
    }
    CHECK_FALSE(deltav_ext::Contains(engines, c.ship.liveParts[0]));
    CHECK(deltav_ext::Get(engines, c.ship.liveParts[0]) == nullptr);
}

TEST_CASE("WP-17 the same craft on Mun (no atmosphere): sea level == vacuum, one run per situation") {
    if (!GameAvailable()) return;
    World17 w;
    EditorCraft c(w, PhysProfS(), w.Kerbin());
    const double kerbinVac = c.dv->TotalDeltaVVac();
    const double kerbinAsl = c.dv->TotalDeltaVASL();
    CHECK(kerbinAsl < kerbinVac);
    const int runs = c.dv->RunCount();
    c.SetBody(w.Body("Mun"));
    CHECK(c.dv->RunCount() == runs + 1);  // SetCalcsDirty(false): syncListInstances stays false, one more run
    CHECK(c.dv->TotalDeltaVVac() == kerbinVac);  // the vacuum numbers do not depend on the body
    CHECK(c.dv->TotalDeltaVASL() == c.dv->TotalDeltaVVac());
    CHECK(c.dv->TotalDeltaVActual() == c.dv->TotalDeltaVVac());
    for (const auto& s : c.dv->OperatingStageInfo()) {
        CHECK(s->deltaVatASL == s->deltaVinVac);
        CHECK(s->deltaVActual == s->deltaVinVac);
        CHECK(s->ispASL == s->ispVac);
        CHECK(s->thrustASL == s->thrustVac);
    }
    // the TWR uses the situation body's surface gravity (DeltaVStageInfo.cs:1152)
    const DeltaVStageInfo& s = *c.dv->OperatingStageInfo()[0];
    const float gF = f32store(PhysicsGlobals::GravitationalAcceleration);
    CHECK(s.TWRVac == f32store(fx(s.thrustVac) / (fx(s.startMass) * (fx(gF) * fx(f32store(w.Body("Mun").GeeASL))))));
}

TEST_CASE("WP-17 a craft without engines: IsReady stays false, a mass-only stage (VesselDeltaV.cs:871-890)") {
    if (!GameAvailable()) return;
    World17 w;
    EditorCraft c(w, BugE(), w.Kerbin());
    CHECK_FALSE(c.dv->IsReady());  // _isReady is set only by a full run
    CHECK(c.dv->OperatingEngineInfo().empty());
    CHECK(c.dv->TotalDeltaVVac() == 0.0);
    CHECK(c.dv->TotalBurnTime() == 0.0);
    CHECK(c.dv->RunCount() == 2);
    CHECK(c.dv->CompletedEvents() == 2);  // the engine-less path fires the event on both runs
    const std::vector<DeltaVStageInfoPtr> stages = c.dv->OperatingStageInfo();
    REQUIRE(stages.size() == 1);
    CHECK(stages[0]->stage == 0);
}

TEST_CASE("WP-17 Kerbal X: asparagus stages through fuel lines, separation indexes, nested stage parts") {
    if (!GameAvailable()) return;
    World17 w;
    EditorCraft c(w, KerbalX(), w.Kerbin());
    const std::vector<DeltaVStageInfoPtr> stages = c.dv->OperatingStageInfo();
    REQUIRE(stages.size() == 8);
    for (size_t i = 0; i < stages.size(); i++) CHECK(stages[i]->stage == static_cast<int>(stages.size() - 1 - i));  // 7, 6, ... 0
    // the first stage lights all seven engines; each asparagus stage drops a pair (engines active: 7, 5, 3, 1)
    CHECK(stages[0]->enginesActiveInStage.size() == 7);
    CHECK(stages[1]->enginesActiveInStage.size() == 5);
    CHECK(stages[2]->enginesActiveInStage.size() == 3);
    CHECK(stages[3]->enginesActiveInStage.size() == 1);
    for (const auto& s : stages) {
        CHECK(s->startMass >= s->endMass);
        CHECK(s->parts.size() >= 3);
        CHECK(s->separationIndex <= s->stage);
    }
    // a stage's parts are those with decoupleStage <= stage: stage 0's parts are a subset of every other stage's
    for (const auto& p : stages.back()->parts)
        for (const auto& s : stages) CHECK(deltav_ext::ContainsPart(s->parts, p->part));
    // GetHighestSeparationStage (VesselDeltaV.cs:1235): an index of the list returns itself, otherwise the largest smaller index (or 0)
    const std::vector<int>& sep = c.dv->SeparationStageIndexes();
    REQUIRE_FALSE(sep.empty());
    for (int idx : sep) CHECK(c.dv->GetHighestSeparationStage(idx) == idx);
    CHECK(c.dv->GetHighestSeparationStage(-5) == 0);
    // the delta-v of a stage that burns is positive and finite, a stage without active engines has none
    for (const auto& s : stages) {
        if (s->enginesActiveInStage.empty()) CHECK(s->deltaVinVac == 0.0f);
        else CHECK(s->deltaVinVac > 0.0f);
        CHECK(std::isfinite(s->deltaVinVac));
    }
}
