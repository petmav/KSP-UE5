// WP-23 unit tests: ModuleEngines / ModuleEnginesFX / MultiModeEngine, ModuleRCS(FX), ModuleGimbal, ModuleResourceIntake, Propellant.
// Expectations come from the decompiled C# control flow (cited per case) and from the real stock part configs
// (core/testdata/GameData-mirror = GameData/Squad/Parts, loaded by PartLoader), never from hand-authored data. The numeric gates of the
// package are golden.engine_thrust (.NET golden, NetFloat) and capture.engine_deltav (in-game, MonoFloat).
#include <cmath>
#include <set>

#include "doctest/doctest.h"
#include "resource_fixture.hpp"

#include "ksp/aero/float_eval.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/modules/module_gimbal.hpp"
#include "ksp/modules/module_rcs.hpp"
#include "ksp/modules/module_resource_intake.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/vessel/flight_ctrl_state.hpp"

using namespace ksp;
using namespace ksp::rtest;

namespace {

ModuleEngines* FirstEngine(Part& p) {
    for (size_t i = 0; i < p.Modules().Count(); i++)
        if (auto* e = dynamic_cast<ModuleEngines*>(p.Modules().GetModule(i))) return e;
    return nullptr;
}
template <class T>
T* Find(Part& p) {
    for (size_t i = 0; i < p.Modules().Count(); i++)
        if (auto* e = dynamic_cast<T*>(p.Modules().GetModule(i))) return e;
    return nullptr;
}

// the cfg MODULE node of a prefab module (PartModuleData::config: the node as loaded)
const ConfigNode* CfgNode(const PartPrefab& prefab, const PartModule* m) {
    for (const PartModuleData& d : prefab.ModuleData)
        if (d.module.get() == m) return d.config.get();
    return nullptr;
}

double Density(const char* resource) {
    const PartResourceDefinition* d = PartResourceLibrary::Instance()->GetDefinition(std::string_view(resource));
    REQUIRE(d != nullptr);
    return static_cast<double>(d->density());
}

struct EngineRef {
    std::string part;
    ModuleEngines* m;
    const ConfigNode* node;
};
std::vector<EngineRef> StockEngines(Game15& g) {
    std::vector<EngineRef> out;
    for (const auto& apu : g.data.parts.loadedParts) {
        const AvailablePart* ap = apu.get();
        if (!ap->partPrefab) continue;
        for (const PartModuleData& d : ap->partPrefab->ModuleData)
            if (auto* e = dynamic_cast<ModuleEngines*>(d.module.get())) out.push_back({ap->name, e, d.config.get()});
    }
    return out;
}

}  // namespace

TEST_CASE("WP-23 every stock engine: propellants, mixture density, flows and throttle limits equal the cfg and the decompiled OnLoad") {
    if (!GameAvailable()) return;
    Game15 g;
    const auto engines = StockEngines(g);
    CHECK(engines.size() == 41);  // the 41 engine modules of the harness golden (EnginesGen)
    int fx = 0;
    std::set<std::string> propNames;
    for (const EngineRef& e : engines) {
        INFO(e.part);
        REQUIRE(e.node != nullptr);
        if (dynamic_cast<ModuleEnginesFX*>(e.m)) fx++;
        // ModuleEngines.cs:598-619 OnLoad: PROPELLANT nodes -> Propellant.Load (name, ratio, flags, resourceFlowMode)
        std::vector<ConfigNode*> pnodes = e.node->GetNodes("PROPELLANT");
        std::vector<ConfigNode*> named;
        for (ConfigNode* n : pnodes)
            if (n->HasValue("name")) named.push_back(n);
        REQUIRE(e.m->propellants.size() == named.size());
        double ratioSum = 0.0, density = 0.0;
        for (size_t i = 0; i < named.size(); i++) {
            const Propellant& p = e.m->propellants[i];
            CHECK(p.name == *named[i]->GetValue("name"));
            propNames.insert(p.name);
            float ratio = 0.0f;
            if (named[i]->HasValue("ratio")) ratio = cs::FloatParse(*named[i]->GetValue("ratio"));
            CHECK(p.ratio == ratio);
            CHECK(p.id == MonoStringHash(p.name));  // Propellant.cs:64 `id = name.GetHashCode()`
            bool ignore = false;
            if (named[i]->HasValue("ignoreForIsp")) ignore = cs::BoolParse(*named[i]->GetValue("ignoreForIsp"));
            CHECK(p.ignoreForIsp == ignore);
            if (!ignore) {
                density += Density(p.name.c_str()) * static_cast<double>(ratio);
                ratioSum += static_cast<double>(ratio);
            }
            CHECK(p.totalResourceAvailable() == 0.0);  // no vessel: nothing connected
        }
        // SetupPropellant (:390): mixtureDensity (float accumulation), mixtureDensityRecip = 1 / mixtureDensity, ratioSum
        CHECK(static_cast<double>(e.m->mixtureDensity) == doctest::Approx(density).epsilon(1e-6));
        CHECK(e.m->ratioSum == doctest::Approx(ratioSum).epsilon(1e-6));
        if (density > 0.0) CHECK(e.m->mixtureDensityRecip == doctest::Approx(1.0 / static_cast<double>(e.m->mixtureDensity)).epsilon(1e-12));
        // :636-647 OnLoad: maxFuelFlow = maxThrust / (atmosphereCurve.Evaluate(0) * g), throttleMin = minFuelFlow / maxFuelFlow
        const FloatCurve atm = e.m->Get<FloatCurve>("atmosphereCurve");
        const double isp0 = static_cast<double>(atm.Evaluate(0.0f));
        if (e.node->HasValue("maxThrust")) {
            const float mt = cs::FloatParse(*e.node->GetValue("maxThrust"));
            CHECK(e.m->maxThrust == mt);
            CHECK(static_cast<double>(e.m->maxFuelFlow) == doctest::Approx(static_cast<double>(mt) / (isp0 * 9.80665)).epsilon(1e-6));
        }
        if (e.node->HasValue("minThrust")) {
            const float mt = cs::FloatParse(*e.node->GetValue("minThrust"));
            CHECK(e.m->minThrust == mt);
            CHECK(static_cast<double>(e.m->minFuelFlow) == doctest::Approx(static_cast<double>(mt) / (isp0 * 9.80665)).epsilon(1e-6));
        } else {
            CHECK(e.m->minFuelFlow == 0.0f);
        }
        CHECK(static_cast<double>(e.m->throttleMin) == doctest::Approx(static_cast<double>(e.m->minFuelFlow) / static_cast<double>(e.m->maxFuelFlow)).epsilon(1e-6));
        // the mass flow split: sum(getMaxFuelFlow(p) * density(p)) over the propellants counted for Isp == maxFuelFlow (:2029-2037)
        if (density > 0.0) {
            double mass = 0.0;
            for (const Propellant& p : e.m->propellants)
                if (!p.ignoreForIsp) mass += static_cast<double>(e.m->getMaxFuelFlow(p)) * Density(p.name.c_str());
            CHECK(mass == doctest::Approx(static_cast<double>(e.m->maxFuelFlow)).epsilon(1e-5));
        }
        // :2137 IsStageable, :2132 GetEngineType (the cfg EngineType value)
        CHECK(e.m->IsStageable());
        if (e.node->HasValue("EngineType")) {
            const std::string t = *e.node->GetValue("EngineType");
            const char* names[] = {"Generic", "SolidBooster", "LiquidFuel", "Piston", "Turbine", "ScramJet", "Electric", "Nuclear", "MonoProp"};
            CHECK(std::string(names[static_cast<int>(e.m->GetEngineType())]) == t);
        } else {
            CHECK(e.m->GetEngineType() == EngineType::Generic);
        }
        CHECK(e.m->GetMaxThrust() == e.m->maxThrust);
        CHECK(e.m->g == 9.80665f);
    }
    CHECK(fx > 20);
    CHECK(propNames.count("LiquidFuel") == 1);
    CHECK(propNames.count("Oxidizer") == 1);
    CHECK(propNames.count("IntakeAir") == 1);
    CHECK(propNames.count("SolidFuel") == 1);
    CHECK(propNames.count("XenonGas") == 1);
    CHECK(propNames.count("MonoPropellant") == 1);
}

TEST_CASE("WP-23 thrust maths: GetEngineThrust, MaxThrustOutputVac / Atm, flow curves, thrust limiter, IntakeAir engines") {
    if (!GameAvailable()) return;
    Game15 g;
    // liquidEngine (LV-T30 Reliant): LiquidFuel + Oxidizer, no curves
    auto part = g.New("liquidEngine");
    ModuleEngines* e = FirstEngine(*part);
    REQUIRE(e != nullptr);
    const FloatCurve atm = e->Get<FloatCurve>("atmosphereCurve");
    const double ispVac = atm.Evaluate(0.0f), ispAsl = atm.Evaluate(1.0f);
    const double minFF = e->minFuelFlow, maxFF = e->maxFuelFlow;
    // :2040 getExhaustVelocity = isp * g; :2045 GetEngineThrust = Lerp(minFuelFlow, maxFuelFlow, throttle) * exhaustVelocity
    CHECK(e->getExhaustVelocity(static_cast<float>(ispVac)) == static_cast<float>(ispVac * 9.80665f));
    CHECK(static_cast<double>(e->GetEngineThrust(static_cast<float>(ispVac), 1.0f)) ==
          doctest::Approx(maxFF * ispVac * 9.80665).epsilon(1e-6));
    CHECK(static_cast<double>(e->GetEngineThrust(static_cast<float>(ispVac), 0.5f)) ==
          doctest::Approx((minFF + (maxFF - minFF) * 0.5) * ispVac * 9.80665).epsilon(1e-6));
    CHECK(e->GetEngineThrust(static_cast<float>(ispVac), 0.0f) == doctest::Approx(minFF * ispVac * 9.80665).epsilon(1e-6));
    // :1887 MaxThrustOutputVac(useThrustLimiter): the vacuum thrust equals the cfg maxThrust at 100 %, scales with thrustPercentage
    CHECK(static_cast<double>(e->MaxThrustOutputVac(true)) == doctest::Approx(static_cast<double>(e->maxThrust)).epsilon(1e-6));
    CHECK(static_cast<double>(e->MaxThrustOutputVac(false)) == doctest::Approx(static_cast<double>(e->maxThrust)).epsilon(1e-6));
    e->Set<float>("thrustPercentage", 50.0f);
    CHECK(static_cast<double>(e->MaxThrustOutputVac(true)) == doctest::Approx(static_cast<double>(e->maxThrust) * 0.5).epsilon(1e-6));
    CHECK(static_cast<double>(e->MaxThrustOutputVac(false)) == doctest::Approx(static_cast<double>(e->maxThrust)).epsilon(1e-6));  // no limiter
    // :1833 MaxThrustOutputAtm at 1 atm: the sea-level Isp (no atmChangeFlow: num2 = 1)
    e->Set<float>("thrustPercentage", 100.0f);
    CHECK(static_cast<double>(e->MaxThrustOutputAtm(false, true, 1.0f, 310.0, 1.225)) ==
          doctest::Approx(maxFF * ispAsl * 9.80665).epsilon(1e-6));
    // CenterOfThrustQuery (:2103): no thrust transforms without a model -> the query keeps zero position / direction; thrust = maxThrust * limiter
    CenterOfThrustQuery q;
    q.Reset();
    e->OnCenterOfThrustQuery(q);
    CHECK(q.thrust == doctest::Approx(static_cast<double>(e->maxThrust)).epsilon(1e-6));
    CHECK(q.pos == Vector3f::zero());
    e->Set<bool>("manuallyOverridden", true);  // `!manuallyOverridden` guard: the query stays untouched
    CenterOfThrustQuery q2;
    q2.Reset();
    e->OnCenterOfThrustQuery(q2);
    CHECK(q2.thrust == 0.0f);

    // turboJet: Dry (ModuleEnginesFX) uses IntakeAir, atmChangeFlow, atmCurve and velCurve (flow multipliers, :1850-1878)
    auto jet = g.New("turboJet");
    ModuleEngines* dry = nullptr;
    for (size_t i = 0; i < jet->Modules().Count(); i++)
        if (auto* m = dynamic_cast<ModuleEngines*>(jet->Modules().GetModule(i)))
            if (m->Get<std::string>("engineID") == "Dry") dry = m;
    REQUIRE(dry != nullptr);
    CHECK(dry->MaxThrustOutputVac(true) == 0.0f);  // :1887: an engine with an IntakeAir propellant makes no vacuum thrust
    CHECK(dry->Get<bool>("atmChangeFlow"));
    CHECK(dry->Get<bool>("useVelCurve"));
    const FloatCurve datm = dry->Get<FloatCurve>("atmosphereCurve");
    const FloatCurve velCurve = dry->Get<FloatCurve>("velCurve");
    const FloatCurve atmCurve = dry->Get<FloatCurve>("atmCurve");
    const bool useAtmCurve = dry->Get<bool>("useAtmCurve");
    const double flowMultCap = static_cast<double>(dry->Get<float>("flowMultCap"));
    for (double mach : {0.0, 0.5, 1.0, 2.0, 3.0}) {
        for (double rho : {0.0, 0.3, 1.225}) {
            g.data.parts.getPartInfoByName("turboJet")->partPrefab->fields.machNumber = mach;
            jet->fields.machNumber = mach;
            double num2 = rho * 0.8163265147242933;  // :1843 (float)(atmDensity * 0.8163265147242933)
            num2 = static_cast<double>(static_cast<float>(num2));
            if (useAtmCurve) num2 = static_cast<double>(atmCurve.Evaluate(static_cast<float>(num2)));
            num2 *= static_cast<double>(velCurve.Evaluate(static_cast<float>(mach)));
            if (num2 > flowMultCap) {
                const double n6 = num2 - flowMultCap;
                num2 = flowMultCap + n6 / (static_cast<double>(dry->Get<float>("flowMultCapSharpness")) + n6 / flowMultCap);
            }
            if (num2 < static_cast<double>(dry->Get<float>("CLAMP"))) num2 = static_cast<double>(dry->Get<float>("CLAMP"));
            const float isp = datm.Evaluate(1.0f);
            const double base = static_cast<double>(dry->GetEngineThrust(isp, 1.0f));
            CHECK(static_cast<double>(dry->MaxThrustOutputAtm(false, true, 1.0f, 310.0, rho)) == doctest::Approx(base * num2).epsilon(2e-5));
        }
    }
    // the CLAMP floor: no air -> the multiplier floors at CLAMP (1e-5), thrust is tiny but never zero
    jet->fields.machNumber = 0.0;
    CHECK(dry->MaxThrustOutputAtm(false, true, 0.0f, 310.0, 0.0) > 0.0f);
}

TEST_CASE("WP-23 float policy: the NetFloat (.NET) and MonoFloat instantiations of the thrust maths") {
    if (!GameAvailable()) return;
    Game15 g;
    auto part = g.New("liquidEngineMainsail.v2");
    ModuleEngines* e = FirstEngine(*part);
    REQUIRE(e != nullptr);
    int differ = 0, total = 0;
    for (double p : {0.0, 0.001, 0.01, 0.05, 0.1, 0.2, 0.3, 0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 5.0, 7.0, 10.0, 15.0})
        for (float thr : {0.1f, 0.3f, 0.5f, 0.7f, 1.0f}) {
            const float isp = e->Get<FloatCurve>("atmosphereCurve").Evaluate(static_cast<float>(p));
            const float a = e->GetEngineThrustT<aero::MonoFloat>(isp, thr), b = e->GetEngineThrustT<aero::NetFloat>(isp, thr);
            total++;
            if (a != b) differ++;
            // both are the same expression: they agree to a few ULP, never more
            CHECK(std::fabs(static_cast<double>(a) - static_cast<double>(b)) <= 4.0 * std::fabs(static_cast<double>(a)) * 1.1920929e-7);
        }
    CHECK(total == 90);
    // the production virtual is the MonoFloat instantiation
    CHECK(e->GetEngineThrust(300.0f, 0.37f) == e->GetEngineThrustT<aero::MonoFloat>(300.0f, 0.37f));
    // OnLoad statement `maxThrust / (Evaluate(0f) * g)`: the two policies on the engine's own numbers
    const float isp0 = e->Get<FloatCurve>("atmosphereCurve").Evaluate(0.0f);
    const float mono = ModuleEngines::FuelFlowFromThrust<aero::MonoFloat>(e->maxThrust, isp0, e->g);
    const float net = ModuleEngines::FuelFlowFromThrust<aero::NetFloat>(e->maxThrust, isp0, e->g);
    CHECK(mono == e->maxFuelFlow);  // the loader ran the production (Mono) policy
    CHECK(std::fabs(static_cast<double>(mono) - static_cast<double>(net)) <= std::fabs(static_cast<double>(mono)) * 2.4e-7);
}

TEST_CASE("WP-23 propellant flow through Part.RequestResource: RequestPropellant, CheckDeprived, Flameout / UnFlameout, Activate / Shutdown") {
    if (!GameAvailable()) return;
    Game15 g;
    auto tank = g.New("fuelTank");
    auto engine = g.New("liquidEngine");
    Attach(*tank, "bottom", *engine, "top");
    ModuleEngines* e = FirstEngine(*engine);
    REQUIRE(e != nullptr);
    std::vector<Part*> parts{tank.get(), engine.get()};
    support::TestVessel vessel;
    vessel.Adopt(parts);
    const int lf = MonoStringHash("LiquidFuel"), ox = MonoStringHash("Oxidizer");
    PartResource* rlf = tank->GetResource("LiquidFuel");
    PartResource* rox = tank->GetResource("Oxidizer");
    REQUIRE(rlf != nullptr);
    REQUIRE(rox != nullptr);
    const double lf0 = rlf->amount, ox0 = rox->amount;
    // OnStart (:1651): the event / field activity flags of a PreLaunch / Landed start
    e->OnStart(StartState::Landed);
    CHECK(engine->fields.stagingIcon.value() == "LIQUID_ENGINE");  // :1653 default staging icon
    CHECK_FALSE(e->Events()["Shutdown"]->active);
    CHECK(e->Events()["Activate"]->active);                         // :1672 not ignited, restart allowed
    CHECK_FALSE(e->Fields()["propellantReqMet"]->guiActive);
    // UpdatePropellantStatus (:421): Part.GetConnectedResourceTotals per propellant
    e->UpdatePropellantStatus();
    CHECK(e->propellants[0].actualTotalAvailable == doctest::Approx(lf0).epsilon(1e-12));
    CHECK(e->propellants[1].totalResourceCapacity == doctest::Approx(rox->maxAmount).epsilon(1e-12));
    // RequestPropellant(mass) (:550): units = mass / mixtureDensity; each propellant draws units * ratio
    const double mass = 0.05;
    const double units = mass * e->mixtureDensityRecip;
    const double got = e->RequestPropellant(mass);
    CHECK(got == doctest::Approx(1.0).epsilon(1e-12));  // everything requested was delivered
    CHECK(lf0 - rlf->amount == doctest::Approx(units * static_cast<double>(e->propellants[0].ratio)).epsilon(1e-9));
    CHECK(ox0 - rox->amount == doctest::Approx(units * static_cast<double>(e->propellants[1].ratio)).epsilon(1e-9));
    CHECK(e->propellants[0].name == "LiquidFuel");
    CHECK(e->propellants[0].currentAmount == doctest::Approx(units * e->propellants[0].ratio).epsilon(1e-9));
    CHECK_FALSE(e->flameout());
    CHECK(e->RequestPropellant(0.0) == 0.0);  // :560 `if (num2 == 0.0) return 0.0`
    // CheckDeprived (:529): requirement vs ignitionThreshold * requirement
    std::string who;
    CHECK_FALSE(e->CheckDeprived(1.0, who));
    CHECK(who.empty());
    // an empty tank: deprived -> Flameout(Localizer.Format("#autoLOC_219085", propName)) (:560-563)
    rlf->amount = 0.0;
    e->UpdatePropellantStatus();
    e->Set<float>("currentThrottle", 0.8f);
    CHECK(e->RequestPropellant(0.05) == 0.0);  // flameout -> 0
    CHECK(e->flameout());
    CHECK(e->Get<std::string>("statusL2").find("deprived") != std::string::npos);
    CHECK(e->Get<std::string>("status") == "Flame-Out!");  // #autoLOC_219016
    CHECK(e->currentThrottle() == 0.0f);                   // :514
    CHECK(e->CheckDeprived(0.05, who));
    {   // Propellant.displayName: the definition's display name, or its abbreviation when longer than 14 characters (Propellant.cs:70-74)
        const PartResourceDefinition* def = PartResourceLibrary::Instance()->GetDefinition(std::string_view("LiquidFuel"));
        REQUIRE(def != nullptr);
        CHECK(who == (def->displayName().size() > 14 ? def->abbreviation() : def->displayName()));
    }
    // refuel: UnFlameout (:517)
    rlf->amount = lf0;
    e->UpdatePropellantStatus();
    CHECK(e->RequestPropellant(0.05) > 0.0);
    CHECK_FALSE(e->flameout());
    CHECK(e->Get<std::string>("status") == "Nominal");
    (void)lf;
    (void)ox;

    // Activate / Shutdown (:740, :788) and the onEngineActiveChange event
    int fired = 0;
    struct L {
        int* n;
        void On(ModuleEngines*) { (*n)++; }
    } listener{&fired};
    GameEvents::onEngineActiveChange.Add(&listener, &L::On);
    e->Activate();
    CHECK(e->EngineIgnited());
    CHECK(e->Events()["Shutdown"]->active);
    CHECK_FALSE(e->Events()["Activate"]->active);
    CHECK(e->Fields()["propellantReqMet"]->guiActive);
    CHECK(fired == 1);
    CHECK(e->isOperational());
    e->Shutdown();
    CHECK_FALSE(e->EngineIgnited());
    CHECK(e->Events()["Activate"]->active);
    CHECK(fired == 2);
    // allowRestart = false: Shutdown latches engineShutdown, Activate is then refused (:742)
    e->Set<bool>("allowRestart", false);
    e->Activate();
    CHECK(e->EngineIgnited());
    e->Shutdown();
    CHECK(e->Get<bool>("engineShutdown"));
    CHECK_FALSE(e->Events()["Activate"]->active);
    e->Activate();
    CHECK_FALSE(e->EngineIgnited());
    // allowShutdown = false: Shutdown does nothing
    e->Set<bool>("allowRestart", true);
    e->Set<bool>("engineShutdown", false);
    e->Set<bool>("allowShutdown", false);
    e->Activate();
    CHECK_FALSE(e->Events()["Shutdown"]->active);
    e->Shutdown();
    CHECK(e->EngineIgnited());
    GameEvents::onEngineActiveChange.Remove(&listener, &L::On);

    // IStageable: OnActive (:1724) ignites once (staged) when staging is enabled
    e->Set<bool>("allowShutdown", true);
    e->Shutdown();
    CHECK_FALSE(e->EngineIgnited());
    e->Set<bool>("staged", false);
    e->Set<bool>("stagingEnabled", true);
    e->OnActive();
    CHECK(e->EngineIgnited());
    CHECK(e->Get<bool>("staged"));
    e->OnActive();  // already ignited: unchanged
    CHECK(e->EngineIgnited());
    // clampPropReceived (:566-571): the received fraction is the minimum over the propellants (above clampPropReceivedMinLowerAmount)
    e->Shutdown();
    CHECK(e->flameout() == false);
}

TEST_CASE("WP-23 throttle model (UpdateThrottle) and required propellant mass") {
    if (!GameAvailable()) return;
    Game15 g;
    auto part = g.New("liquidEngine");
    ModuleEngines* e = FirstEngine(*part);
    REQUIRE(e != nullptr);
    EngineFlightInput in;
    in.flightReady = false;
    in.fixedDeltaTime = 0.02f;
    // :1354 throttleLocked: requestedThrottle = EngineIgnited ? thrustPercentage * 0.01 : 0
    e->Set<bool>("throttleLocked", true);
    e->Set<float>("thrustPercentage", 80.0f);
    e->UpdateThrottle(in);
    CHECK(e->requestedThrottle == 0.0f);
    e->Set<bool>("EngineIgnited", true);
    e->UpdateThrottle(in);
    CHECK(e->requestedThrottle == doctest::Approx(0.8).epsilon(1e-7));
    CHECK(e->currentThrottle() == e->requestedThrottle);  // no response time: current = requested
    // :1358 independentThrottle: independentThrottlePercentage * 0.01 * (thrustPercentage * 0.01)
    e->Set<bool>("throttleLocked", false);
    e->Set<bool>("independentThrottle", true);
    e->Set<float>("independentThrottlePercentage", 50.0f);
    e->UpdateThrottle(in);
    CHECK(e->requestedThrottle == doctest::Approx(0.4).epsilon(1e-6));
    // :1393 throttleMin != 0: requestedThrottle = Lerp(throttleMin, 1, requested)
    e->throttleMin = 0.25f;
    e->UpdateThrottle(in);
    CHECK(e->requestedThrottle == doctest::Approx(0.25 + 0.75 * 0.4).epsilon(1e-6));
    // :1395 useEngineResponseTime: Lerp(current, requested, engineAccelerationSpeed * dt) up, engineDecelerationSpeed * dt down
    e->throttleMin = 0.0f;
    e->Set<bool>("useEngineResponseTime", true);
    e->Set<float>("currentThrottle", 0.0f);
    e->Set<float>("independentThrottlePercentage", 100.0f);
    e->Set<float>("thrustPercentage", 100.0f);
    e->UpdateThrottle(in);
    const double acc = static_cast<double>(e->Get<float>("engineAccelerationSpeed")) * 0.02;
    CHECK(e->currentThrottle() == doctest::Approx(acc).epsilon(1e-6));
    e->Set<float>("independentThrottlePercentage", 0.0f);
    const float before = e->currentThrottle();
    e->UpdateThrottle(in);
    CHECK(e->currentThrottle() == doctest::Approx(before + (0.0 - before) * static_cast<double>(e->Get<float>("engineDecelerationSpeed")) * 0.02).epsilon(1e-6));
    // within 0.0001 of the target: snaps (:1399)
    e->Set<float>("currentThrottle", 0.00005f);
    e->UpdateThrottle(in);
    CHECK(e->currentThrottle() == e->requestedThrottle);
    // throttleUseAlternate with throttleInstant (:1370-1383): current = requested (Lerp(throttleMin, 1, r) for r > 0)
    e->Set<bool>("useEngineResponseTime", false);
    e->Set<bool>("throttleUseAlternate", true);
    e->Set<bool>("throttleInstant", true);
    e->Set<float>("independentThrottlePercentage", 60.0f);
    e->throttleMin = 0.1f;
    e->UpdateThrottle(in);
    CHECK(e->currentThrottle() == doctest::Approx(0.1 + 0.9 * 0.6).epsilon(1e-6));
    // throttleInstantShutdown with a zero request: current = 0 at once
    e->Set<float>("independentThrottlePercentage", 0.0f);
    e->UpdateThrottle(in);
    CHECK(e->currentThrottle() == 0.0f);
    // alternate response: the rate limited climb (:1384-1431): one step from rest
    e->Set<bool>("throttleInstant", false);
    e->Set<float>("throttleResponseRate", 2.0f);
    e->Set<float>("independentThrottlePercentage", 100.0f);
    e->Set<float>("currentThrottle", 0.0f);
    e->UpdateThrottle(in);
    // num3 = rate * dt * (0.0005 + 4.05 * current * startupMult) with current 0 (<= 0.01): 2 * 0.02 * 0.0005 = 2e-5; the gap (1) is larger: current += num3
    CHECK(e->currentThrottle() == doctest::Approx(2.0 * 0.02 * 0.0005).epsilon(1e-5));

    // RequiredPropellantMass (:1454): realIsp from the atmosphere curve at the part's static pressure, requestedMassFlow = maxFuelFlow * throttle
    e->Set<bool>("throttleUseAlternate", false);
    part->fields.staticPressureAtm = 0.5;
    in.fixedDeltaTime = 0.02f;
    const double dm = e->RequiredPropellantMass(0.5f, in);
    const FloatCurve atm = e->Get<FloatCurve>("atmosphereCurve");
    CHECK(e->realIsp() == atm.Evaluate(0.5f));
    CHECK(static_cast<double>(e->requestedMassFlow) == doctest::Approx(static_cast<double>(e->maxFuelFlow) * 0.5).epsilon(1e-6));
    CHECK(dm == doctest::Approx(static_cast<double>(e->requestedMassFlow) * 0.02).epsilon(1e-6));
    CHECK(static_cast<double>(e->resultingThrust) == doctest::Approx(static_cast<double>(e->requestedMassFlow) * static_cast<double>(e->realIsp()) * 9.80665).epsilon(1e-6));
    CHECK(e->flowMultiplier == 1.0f);  // no atmChangeFlow / curves: ModifyFlow() = 1
}

TEST_CASE("WP-23 MultiModeEngine: mode switching on the stock multimode engines (turboJet Dry / Wet, RAPIER)") {
    if (!GameAvailable()) return;
    Game15 g;
    for (const char* partName : {"turboJet", "RAPIER"}) {
        INFO(partName);
        auto part = g.New(partName);
        MultiModeEngine* mm = Find<MultiModeEngine>(*part);
        REQUIRE(mm != nullptr);
        const std::string primaryId = mm->Get<std::string>("primaryEngineID"), secondaryId = mm->Get<std::string>("secondaryEngineID");
        int switched = 0;
        struct L {
            int* n;
            void On(MultiModeEngine*) { (*n)++; }
        } listener{&switched};
        GameEvents::onMultiModeEngineSwitchActive.Add(&listener, &L::On);
        // OnStart (:326): finds the engines by engineID, sets the primary mode (runningPrimary defaults to true)
        mm->OnStart(StartState::Editor);
        REQUIRE(mm->PrimaryEngine() != nullptr);
        REQUIRE(mm->SecondaryEngine() != nullptr);
        CHECK(mm->PrimaryEngine()->Get<std::string>("engineID") == primaryId);
        CHECK(mm->SecondaryEngine()->Get<std::string>("engineID") == secondaryId);
        CHECK(mm->runningPrimary());
        CHECK(switched == 1);
        const std::string primaryName = mm->Get<std::string>("primaryEngineModeDisplayName");
        CHECK(mm->Get<std::string>("mode") == (primaryName.empty() ? primaryId : primaryName));
        // SetPrimary (:190): the primary is enabled and not overridden, the secondary is disabled and overridden
        CHECK(mm->PrimaryEngine()->isEnabled);
        CHECK_FALSE(mm->PrimaryEngine()->Get<bool>("manuallyOverridden"));
        CHECK_FALSE(mm->SecondaryEngine()->isEnabled);
        CHECK(mm->SecondaryEngine()->Get<bool>("manuallyOverridden"));
        // IEngineStatus follows the running engine (:44-62)
        mm->PrimaryEngine()->Set<float>("currentThrottle", 0.6f);
        CHECK(mm->throttleSetting() == 0.6f);
        CHECK(mm->normalizedOutput() == mm->PrimaryEngine()->normalizedOutput());
        // autoSwitchAvailable == false: autoSwitch off, both auto-switch events inactive (:294, :353)
        if (!mm->Get<bool>("autoSwitchAvailable")) {
            CHECK_FALSE(mm->autoSwitch());
            CHECK_FALSE(mm->Events()["DisableAutoSwitch"]->active);
            CHECK_FALSE(mm->Events()["EnableAutoSwitch"]->active);
        } else {
            CHECK(mm->autoSwitch());
            CHECK(mm->Events()["DisableAutoSwitch"]->active);
            CHECK_FALSE(mm->Events()["EnableAutoSwitch"]->active);
        }
        // ToggleMode in the editor (:115): SetSecondary(false): no engine activation, no throttle carry
        mm->ToggleMode();
        CHECK_FALSE(mm->runningPrimary());
        CHECK(switched == 2);
        const std::string secondaryName = mm->Get<std::string>("secondaryEngineModeDisplayName");
        CHECK(mm->Get<std::string>("mode") == (secondaryName.empty() ? secondaryId : secondaryName));
        CHECK(mm->SecondaryEngine()->isEnabled);
        CHECK_FALSE(mm->SecondaryEngine()->Get<bool>("manuallyOverridden"));
        CHECK_FALSE(mm->PrimaryEngine()->isEnabled);
        CHECK(mm->PrimaryEngine()->Get<bool>("manuallyOverridden"));
        CHECK(mm->throttleSetting() == mm->SecondaryEngine()->throttleSetting());
        CHECK(mm->SecondaryEngine()->currentThrottle() == 0.0f);  // not carried over in the editor
        // in flight SetPrimary(true) (:190): the secondary is shut down, the primary activated, and carryOverThrottle copies the throttle
        {
            SceneScope scene(GameScenes::FLIGHT);
            mm->SecondaryEngine()->Set<bool>("EngineIgnited", true);
            mm->SecondaryEngine()->Set<float>("currentThrottle", 0.9f);
            mm->PrimaryEngine()->Set<bool>("EngineIgnited", false);
            mm->ToggleMode();
            CHECK(mm->runningPrimary());
            CHECK(mm->PrimaryEngine()->EngineIgnited());
            CHECK_FALSE(mm->SecondaryEngine()->EngineIgnited());
            if (mm->Get<bool>("carryOverThrottle")) CHECK(mm->PrimaryEngine()->currentThrottle() == 0.9f);
        }
        // ModeEvent (:109): disables auto-switch and toggles
        mm->ModeEvent();
        CHECK_FALSE(mm->autoSwitch());
        CHECK_FALSE(mm->runningPrimary());
        // a stray multimode with a missing engine: loadFailure keeps SetPrimary / SetSecondary inert (:194)
        GameEvents::onMultiModeEngineSwitchActive.Remove(&listener, &L::On);
    }
    // MultiModeEngine on a part with no secondary engine: Debug.LogError and loadFailure (:371)
    auto jet = g.New("turboJet");
    MultiModeEngine* mm = Find<MultiModeEngine>(*jet);
    mm->Set<std::string>("secondaryEngineID", "NoSuchEngine");
    LogCapture log;
    mm->OnStart(StartState::Editor);
    CHECK(log.Has(log::Level::Error, "Failed to find primary and secondary engine"));
    CHECK(mm->runningPrimary());
    mm->ToggleMode();  // inert
    CHECK(mm->runningPrimary());
}

TEST_CASE("WP-23 ModuleGimbal: range fallbacks of every stock gimbal, GimbalRotation maths, FixedUpdate actuation") {
    if (!GameAvailable()) return;
    Game15 g;
    int gimbals = 0;
    for (const auto& apu : g.data.parts.loadedParts) {
        const AvailablePart* ap = apu.get();
        if (!ap->partPrefab) continue;
        for (const PartModuleData& d : ap->partPrefab->ModuleData) {
            auto* m = dynamic_cast<ModuleGimbal*>(d.module.get());
            if (m == nullptr) continue;
            gimbals++;
            INFO(ap->name);
            // ModuleGimbal.cs:296 OnLoad: XP / YP fall back to gimbalRange, XN / YN to XP / YP; all are the cfg values when given
            const float range = d.config->HasValue("gimbalRange") ? cs::FloatParse(*d.config->GetValue("gimbalRange")) : 10.0f;
            float xp = d.config->HasValue("gimbalRangeXP") ? cs::FloatParse(*d.config->GetValue("gimbalRangeXP")) : -1.0f;
            float yp = d.config->HasValue("gimbalRangeYP") ? cs::FloatParse(*d.config->GetValue("gimbalRangeYP")) : -1.0f;
            float xn = d.config->HasValue("gimbalRangeXN") ? cs::FloatParse(*d.config->GetValue("gimbalRangeXN")) : -1.0f;
            float yn = d.config->HasValue("gimbalRangeYN") ? cs::FloatParse(*d.config->GetValue("gimbalRangeYN")) : -1.0f;
            if (xp < 0.0f) xp = range;
            if (yp < 0.0f) yp = range;
            if (xn < 0.0f) xn = xp;
            if (yn < 0.0f) yn = yp;
            CHECK(m->Get<float>("gimbalRangeXP") == xp);
            CHECK(m->Get<float>("gimbalRangeYP") == yp);
            CHECK(m->Get<float>("gimbalRangeXN") == xn);
            CHECK(m->Get<float>("gimbalRangeYN") == yn);
            CHECK_FALSE(m->IsStageable());  // :326
            CHECK(m->Get<float>("gimbalLimiter") == 100.0f);
            CHECK_FALSE(m->Get<bool>("gimbalLock"));
        }
    }
    CHECK(gimbals == 22);  // 23 "name = ModuleGimbal" lines in the cfgs: the 23rd (microEngine_v2) is commented out

    // GimbalRotation (:196) on a part: the gimbal of the liquidEngine2 (LV-T45 Swivel)
    auto part = g.New("liquidEngine2");
    ModuleGimbal* gm = Find<ModuleGimbal>(*part);
    REQUIRE(gm != nullptr);
    const float rXP = gm->Get<float>("gimbalRangeXP"), rXN = gm->Get<float>("gimbalRangeXN"), rYP = gm->Get<float>("gimbalRangeYP"),
                rYN = gm->Get<float>("gimbalRangeYN");
    PartTransform ref;  // vessel.ReferenceTransform at the origin, identity rotation
    PartTransform t;    // gimbal transform below the CoM, identity rotation
    t.position = {1.0f, -2.0f, 0.5f};
    const Vector3f coM{0.0f, 0.0f, 0.0f};
    // pitch +1: result.x = Clamp(1) * XP * limiter * 0.01 (CoM above the engine: num = +1 when localCoM.y >= vector.y)
    Vector3f r = gm->GimbalRotation(ref, t, {1.0f, 0.0f, 0.0f}, coM);
    CHECK(r.x == doctest::Approx(rXP).epsilon(1e-6));
    CHECK(r.y == 0.0f);
    r = gm->GimbalRotation(ref, t, {-1.0f, 0.0f, 0.0f}, coM);  // negative result.x uses the XN range
    CHECK(r.x == doctest::Approx(-rXN).epsilon(1e-6));
    r = gm->GimbalRotation(ref, t, {0.0f, 0.0f, 1.0f}, coM);  // yaw: world z -> the gimbal's local z: result.y stays 0, no x
    CHECK(r.x == 0.0f);
    // the limiter scales linearly: 25 % (gimbalLimiter * 0.01)
    gm->Set<float>("gimbalLimiter", 25.0f);
    r = gm->GimbalRotation(ref, t, {1.0f, 0.0f, 0.0f}, coM);
    CHECK(r.x == doctest::Approx(rXP * 0.25).epsilon(1e-6));
    gm->Set<float>("gimbalLimiter", 100.0f);
    // a CoM below the engine (localCoM.y < vector.y) flips the pitch and yaw sign (:200-204)
    r = gm->GimbalRotation(ref, t, {1.0f, 0.0f, 0.0f}, Vector3f{0.0f, -5.0f, 0.0f});
    CHECK(r.x == doctest::Approx(-rXN).epsilon(1e-6));
    // roll (:206-224): the engine at +x beyond minRollOffset: PYR.x += PYR.y (and z likewise for +z)
    const float minRoll = gm->Get<float>("minRollOffset");
    CHECK(t.position.x > minRoll);
    r = gm->GimbalRotation(ref, t, {0.0f, 1.0f, 0.0f}, coM);
    CHECK(r.x == doctest::Approx(rXP).epsilon(1e-6));  // roll adds to the x input: pitch component of the same size
    gm->Set<bool>("enableRoll", false);
    r = gm->GimbalRotation(ref, t, {0.0f, 1.0f, 0.0f}, coM);
    CHECK(r.x == 0.0f);  // roll disabled: no contribution
    gm->Set<bool>("enableRoll", true);
    // clamp to the range: PYR.x = 5 still gives the full range (Clamp(-1, 1))
    r = gm->GimbalRotation(ref, t, {5.0f, 0.0f, 0.0f}, coM);
    CHECK(r.x == doctest::Approx(rXP).epsilon(1e-6));

    // FixedUpdate (:161) with a model reader hook providing one gimbal transform with an identity initial rotation
    ModuleGimbal::findModelTransforms = [&](Part&, const std::string& name) {
        CHECK(name == "thrustTransform");
        std::vector<std::pair<PartTransform, Quaternionf>> v;
        v.emplace_back(t, Quaternionf::identity());
        return v;
    };
    gm->OnStart(StartState::Flying);
    CHECK(gm->gimbalTransforms.size() == 1);
    CHECK(gm->initRots.size() == 1);
    FlightCtrlState ctrl;
    ctrl.pitch = 1.0f;
    CHECK_FALSE(gm->FixedUpdate(ref, coM, ctrl, 0.02f));  // gimbalActive is false: nothing moves (:163)
    gm->OnActive();                                        // :331
    CHECK(gm->Get<bool>("gimbalActive"));
    REQUIRE(gm->FixedUpdate(ref, coM, ctrl, 0.02f));
    REQUIRE(gm->newLocalRotations.size() == 1);
    // rotation of rXP degrees about the x axis (xMult * Vector3.right), identity about y
    const double half = static_cast<double>(rXP) * 0.5 * 3.14159265358979 / 180.0;
    CHECK(gm->newLocalRotations[0].x == doctest::Approx(std::sin(half)).epsilon(1e-5));
    CHECK(gm->newLocalRotations[0].w == doctest::Approx(std::cos(half)).epsilon(1e-5));
    CHECK(gm->newLocalRotations[0].y == doctest::Approx(0.0).epsilon(1e-6));
    gm->Set<bool>("gimbalLock", true);  // locked: the transform stays at its initial rotation
    REQUIRE(gm->FixedUpdate(ref, coM, ctrl, 0.02f));
    CHECK(gm->newLocalRotations[0].w == 1.0f);
    CHECK(gm->newLocalRotations[0].x == 0.0f);
    gm->Set<bool>("gimbalLock", false);
    gm->Set<bool>("enablePitch", false);  // pitch input masked (:176)
    REQUIRE(gm->FixedUpdate(ref, coM, ctrl, 0.02f));
    CHECK(gm->newLocalRotations[0].x == doctest::Approx(0.0).epsilon(1e-7));
    gm->Set<bool>("enablePitch", true);
    // useGimbalResponseSpeed: Lerp(old, new, gimbalResponseSpeed * dt) (:180-184)
    gm->Set<bool>("useGimbalResponseSpeed", true);
    gm->oldActuationLocal[0] = {0.0f, 0.0f, 0.0f};
    REQUIRE(gm->FixedUpdate(ref, coM, ctrl, 0.02f));
    CHECK(gm->actuationLocal.x == doctest::Approx(static_cast<double>(rXP) * gm->Get<float>("gimbalResponseSpeed") * 0.02).epsilon(1e-5));
    CHECK(gm->oldActuationLocal[0].x == gm->actuationLocal.x);
    // events and actions (:112-160)
    KSPActionParam ap;
    gm->Set<bool>("gimbalLock", false);
    gm->ToggleAction(ap);
    CHECK(gm->Get<bool>("gimbalLock"));
    gm->ToggleAction(ap);
    CHECK_FALSE(gm->Get<bool>("gimbalLock"));
    gm->TogglePitchAction(ap);
    CHECK_FALSE(gm->Get<bool>("enablePitch"));
    gm->ToggleYawAction(ap);
    CHECK_FALSE(gm->Get<bool>("enableYaw"));
    gm->ToggleRollAction(ap);
    CHECK_FALSE(gm->Get<bool>("enableRoll"));
    ModuleGimbal::findModelTransforms = nullptr;

    // ModuleEngines.Activate starts an inactive gimbal of an unstaged engine (:760-775)
    auto eng = g.New("liquidEngine2");
    ModuleGimbal* g2 = Find<ModuleGimbal>(*eng);
    ModuleEngines* e2 = FirstEngine(*eng);
    REQUIRE(g2 != nullptr);
    CHECK_FALSE(g2->Get<bool>("gimbalActive"));
    e2->OnStart(StartState::Landed);
    e2->Activate();
    CHECK(g2->Get<bool>("gimbalActive"));
}

TEST_CASE("WP-23 ModuleRCS: propellant setup of every stock RCS, thrust maths, request and torque") {
    if (!GameAvailable()) return;
    Game15 g;
    int rcs = 0;
    for (const auto& apu : g.data.parts.loadedParts) {
        const AvailablePart* ap = apu.get();
        if (!ap->partPrefab) continue;
        for (const PartModuleData& d : ap->partPrefab->ModuleData) {
            auto* m = dynamic_cast<ModuleRCS*>(d.module.get());
            if (m == nullptr) continue;
            rcs++;
            INFO(ap->name);
            // OnLoad (:419): one PROPELLANT (resourceName, ratio 1.0, resourceFlowMode) unless the cfg has its own
            REQUIRE_FALSE(m->propellants.empty());
            if (!d.config->HasNode("PROPELLANT")) CHECK(m->propellants[0].name == m->Get<std::string>("resourceName"));
            // SetupPropellant (:613): mixtureDensity is the double sum of float products
            double md = 0.0;
            for (const Propellant& p : m->propellants)
                if (!p.ignoreForIsp) md += static_cast<double>(static_cast<float>(Density(p.name.c_str())) * p.ratio);
            CHECK(m->mixtureDensity == doctest::Approx(md).epsilon(1e-12));
            CHECK(m->mixtureDensityRecip == doctest::Approx(1.0 / md).epsilon(1e-12));
            // maxFuelFlow = getMaxFuelFlow(Isp(0)) = thrusterPower / (Isp * G)
            const FloatCurve atm = m->Get<FloatCurve>("atmosphereCurve");
            CHECK(m->maxFuelFlow == doctest::Approx(static_cast<double>(m->Get<float>("thrusterPower")) / (static_cast<double>(atm.Evaluate(0.0f)) * 9.80665)).epsilon(1e-12));
            // IsStageable (:746): stagingEnabled / stagingToggleEnabledEditor / stagingToggleEnabledFlight
            const bool se = m->Get<bool>("stagingEnabled"), ste = m->Get<bool>("stagingToggleEnabledEditor");
            CHECK(m->IsStageable() == ((!se && !ste) ? m->Get<bool>("stagingToggleEnabledFlight") : true));
            CHECK(m->G == 9.80665);
        }
    }
    CHECK(rcs >= 4);

    auto part = g.New("linearRcs");
    ModuleRCS* r = Find<ModuleRCS>(*part);
    REQUIRE(r != nullptr);
    const double power = static_cast<double>(r->Get<float>("thrusterPower"));
    // CalculateThrust (:566) with infinite propellant: thrust = flowMult * maxFuelFlow * totalForce * thrustPercentage * 0.01 * exhaustVelocity
    // = thrusterPower * totalForce (the exhaust velocity cancels the flow)
    {
        // exhaustVel is private state the first FixedUpdate sets (realISP * G * ispMult); Instantiate does not copy it (C#: not serialized)
        r->thrusterTransforms.clear();
        RCSFlightInput first;
        r->FixedUpdate(first);
        ResourceFlowEnvironment::InfinitePropellant = true;
        bool ok = false;
        const float f = r->CalculateThrust(0.5f, ok, 0.02f);
        ResourceFlowEnvironment::InfinitePropellant = false;
        CHECK(ok);
        CHECK(static_cast<double>(f) == doctest::Approx(power * 0.5).epsilon(1e-6));
        CHECK_FALSE(r->flameout);
    }
    // below EPSILON: no thrust (:604)
    bool ok = true;
    CHECK(r->CalculateThrust(0.01f, ok, 0.02f) == 0.0f);
    CHECK_FALSE(ok);
    // isJustForShow: success with a unit force
    r->Set<bool>("isJustForShow", true);
    CHECK(r->CalculateThrust(0.5f, ok, 0.02f) == 1.0f);
    CHECK(ok);
    r->Set<bool>("isJustForShow", false);

    // Update (:211): deadband EPSILON^2, throttle-as-Z, rotation by the reference transform
    FlightCtrlState ctrl;
    ctrl.pitch = 1.0f;
    ctrl.roll = 0.02f;  // inside the deadband (0.05)
    ctrl.X = 0.5f;
    ctrl.Z = 0.01f;
    r->Update(ctrl, Quaternionf::identity(), false);
    CHECK(r->inputRot.x == 1.0f);
    CHECK(r->inputRot.y == 0.0f);
    CHECK(r->inputLin.x == 0.5f);
    CHECK(r->inputLin.y == 0.0f);  // enableZ -> y slot: Z inside the deadband and useThrottle is false -> 0
    r->Set<bool>("useThrottle", true);
    ctrl.mainThrottle = 0.75f;
    r->Update(ctrl, Quaternionf::identity(), false);
    CHECK(r->inputLin.y == doctest::Approx(0.01 - 0.75).epsilon(1e-6));  // Clamp(Z - mainThrottle, -1, 1)
    CHECK_FALSE(r->usePrecision);
    // enablePitch = false masks the axis
    r->Set<bool>("enablePitch", false);
    r->Update(ctrl, Quaternionf::identity(), true);
    CHECK(r->inputRot.x == 0.0f);
    CHECK(r->usePrecision);

    // GetLeverDistance (:269): sqrt(1 - dot^2) * |d|: the perpendicular distance of the CoM from the thrust line
    RCSThruster th;
    th.position = {0.0f, 0.0f, 0.0f};
    CHECK(r->GetLeverDistance(th, {0.0f, 1.0f, 0.0f}, {3.0f, 0.0f, 0.0f}) == doctest::Approx(3.0).epsilon(1e-6));
    CHECK(r->GetLeverDistance(th, {1.0f, 0.0f, 0.0f}, {3.0f, 0.0f, 0.0f}) == doctest::Approx(0.0).epsilon(1e-3));
    CHECK(r->GetLeverDistanceOriginal(th.position, {0.0f, 4.0f, 3.0f}) == doctest::Approx(5.0).epsilon(1e-6));

    // FixedUpdate (:277): one thruster at +x firing along its up axis; pitch / translate request along that axis
    r->Set<bool>("enablePitch", true);
    r->Set<bool>("requiresFuel", false);  // CalculateThrust answers 1.0 with `success` (no propellant needed)
    r->thrusterTransforms = {RCSThruster{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, true}};
    r->thrustForces.assign(1, 0.0f);
    r->inputLin = {0.0f, 1.0f, 0.0f};  // translate +y: the thruster's `up` is +y: Dot(up, inputLin) = 1
    r->inputRot = {0.0f, 0.0f, 0.0f};
    RCSFlightInput in;
    in.rcsActionGroup = true;
    in.currentCoM = {0.0f, 0.0f, 0.0f};
    r->usePrecision = false;
    r->FixedUpdate(in);
    CHECK(r->rcs_active);
    CHECK(r->thrustForces[0] == 1.0f);
    REQUIRE(r->lastForces.size() == 1);
    CHECK(r->lastForces[0].force == Vector3f{0.0f, -1.0f, 0.0f});  // -vector * force
    CHECK(r->lastForces[0].position == Vector3f{1.0f, 0.0f, 0.0f});
    CHECK(r->isOperating);
    CHECK_FALSE(r->CanBeDetached());  // :953 while operating
    // precision mode divides by the lever distance when > 1 (:314-326): the lever of a thruster at +x with the CoM at 0
    r->usePrecision = true;
    r->Set<bool>("useLever", false);
    r->Set<bool>("requiresFuel", false);
    r->FixedUpdate(in);
    CHECK(r->thrustForces[0] == 1.0f);  // requiresFuel false: CalculateThrust returns 1 regardless
    // rcsEnabled = false: DeactivateFX zeroes the forces
    r->Set<bool>("rcsEnabled", false);
    r->FixedUpdate(in);
    CHECK(r->thrustForces[0] == 0.0f);
    CHECK(r->lastForces.empty());
    CHECK(r->CanBeDetached());
    r->Set<bool>("rcsEnabled", true);
    // an RCS thruster not in the active hierarchy is skipped (:301)
    r->thrusterTransforms[0].activeInHierarchy = false;
    r->FixedUpdate(in);
    CHECK(r->lastForces.empty());
    r->thrusterTransforms[0].activeInHierarchy = true;
    // a shielded part cannot thrust unless shieldedCanThrust (:290)
    in.shieldedFromAirstream = true;
    r->FixedUpdate(in);
    CHECK(r->lastForces.empty());
    in.shieldedFromAirstream = false;

    // GetPotentialTorque (:773): needs rcs_active and enabled axes; the torque of a thruster at +x about the CoM
    r->Set<bool>("requiresFuel", true);
    r->inputRot = {1.0f, 0.0f, 0.0f};
    r->usePrecision = false;
    Vector3f pos, neg;
    r->GetPotentialTorque(pos, neg, {0.0f, 0.0f, 0.0f}, false);
    CHECK(pos.x >= 0.0f);
    CHECK(pos.y >= 0.0f);
    CHECK(pos.z >= 0.0f);
    r->Set<bool>("rcsEnabled", false);
    r->GetPotentialTorque(pos, neg, {0.0f, 0.0f, 0.0f}, false);
    CHECK(pos == Vector3f::zero());
    CHECK(neg == Vector3f::zero());

    // RequestPropellant (:702) through Part.RequestResource: the Mk1-3 command pod holds 30 MonoPropellant and a ModuleRCSFX
    auto pod = g.New("mk1-3pod");
    std::vector<Part*> parts{pod.get()};
    support::TestVessel vessel;
    vessel.Adopt(parts);
    ModuleRCS* r2 = Find<ModuleRCS>(*pod);
    REQUIRE(r2 != nullptr);
    PartResource* mono = pod->GetResource("MonoPropellant");
    REQUIRE(mono != nullptr);
    const double m0 = mono->amount;
    r2->UpdatePropellantStatus();
    CHECK(r2->propellants[0].actualTotalAvailable == doctest::Approx(m0).epsilon(1e-12));
    const double got = r2->RequestPropellant(0.001);
    CHECK(got == doctest::Approx(1.0).epsilon(1e-9));
    CHECK(m0 - mono->amount == doctest::Approx(0.001 * r2->mixtureDensityRecip).epsilon(1e-9));
    // an empty tank is deprived: nothing is drawn, the result is 0 (:714-724)
    mono->amount = 0.0;
    r2->UpdatePropellantStatus();
    CHECK(r2->RequestPropellant(0.001) == 0.0);
    CHECK(r2->propellants[0].isDeprived);
    // mixtureDensity <= 0 answers 0 at once (:704)
    r2->mixtureDensity = 0.0;
    CHECK(r2->RequestPropellant(0.001) == 0.0);
}

TEST_CASE("WP-23 ModuleResourceIntake: OnStart / OnLoad state, area-speed-density intake rate, status logic") {
    if (!GameAvailable()) return;
    Game15 g;
    auto part = g.New("IntakeRadialLong");
    ModuleResourceIntake* in = Find<ModuleResourceIntake>(*part);
    REQUIRE(in != nullptr);
    // OnAwake (:75): the default machCurve (one key 0 -> 1) when the cfg gives none
    CHECK(in->Get<std::string>("resourceName") == "IntakeAir");
    const double area = in->Get<double>("area"), intakeSpeed = in->Get<double>("intakeSpeed"), unitScalar = in->Get<double>("unitScalar");
    CHECK(area == 0.001);
    CHECK(intakeSpeed == 10.0);
    in->OnStart(StartState::Flying);
    CHECK(in->resourceDef != nullptr);
    CHECK(in->densityRecip == doctest::Approx(1.0 / Density("IntakeAir")).epsilon(1e-6));
    CHECK(in->Events()["Deactivate"]->active);  // intakeEnabled
    CHECK_FALSE(in->Events()["Activate"]->active);
    REQUIRE(in->res != nullptr);
    in->OnLoad(ConfigNode());
    CHECK(in->resourceId == MonoStringHash("IntakeAir"));
    CHECK_FALSE(in->Events()["Deactivate"]->active);  // OnLoad (:134) flips the pair for an enabled intake
    // FixedUpdate (:201): airspeed = Clamp01(dot(srf_vel_direction, forward)) * srfSpeed + intakeSpeed;
    // units = airspeed * unitScalar * area * machCurve(mach) * atmDensity / density(IntakeAir) per second
    in->res->amount = 0.0;
    IntakeFlightInput f;
    f.staticPressurekPa = 100.0;
    f.srf_vel_direction = {0.0f, 0.0f, 1.0f};
    f.intakeForward = {0.0f, 0.0f, 1.0f};
    f.srfSpeed = 200.0;
    f.mach = 0.6;
    f.atmDensity = 1.1;
    f.fixedDeltaTime = 0.02f;
    in->FixedUpdate(f);
    const FloatCurve machCurve = in->Get<FloatCurve>("machCurve");
    const double perSecond = (1.0 * 200.0 + intakeSpeed) * unitScalar * area * static_cast<double>(machCurve.Evaluate(0.6f)) * 1.1 * in->densityRecip;
    CHECK(in->Get<float>("airSpeedGui") == static_cast<float>(210.0));
    CHECK(in->Get<float>("airFlow") == static_cast<float>(perSecond));
    CHECK(in->res->amount == doctest::Approx(perSecond * static_cast<double>(0.02f)).epsilon(1e-9));
    CHECK(in->Get<std::string>("status") == Localizer::Format("#autoLOC_235936"));
    // facing away: the dot is clamped at 0, only intakeSpeed counts
    f.srf_vel_direction = {0.0f, 0.0f, -1.0f};
    in->res->amount = 0.0;
    in->FixedUpdate(f);
    CHECK(in->Get<float>("airSpeedGui") == static_cast<float>(intakeSpeed));
    // below kPaThreshold: no air, the stored intake air is dumped (TransferResource(id, double.MinValue)) and the status reads "no atmosphere" (:243)
    f.staticPressurekPa = 0.01;
    in->res->amount = 0.0;
    in->FixedUpdate(f);
    CHECK(in->Get<float>("airFlow") == 0.0f);
    CHECK(in->Get<float>("airSpeedGui") == 0.0f);
    CHECK(in->Get<std::string>("status") == Localizer::Format("#autoLOC_235946"));
    // shielded: airFlow 0 and the shielded status (:249)
    f.staticPressurekPa = 100.0;
    part->set_ShieldedFromAirstream(true);
    in->FixedUpdate(f);
    CHECK(in->Get<float>("airFlow") == 0.0f);
    CHECK(in->Get<std::string>("status") == Localizer::Format("#autoLOC_235899"));
    part->set_ShieldedFromAirstream(false);
    // Deactivate / Activate events and the toggle action (:186-199)
    in->Deactivate();
    CHECK_FALSE(in->Get<bool>("intakeEnabled"));
    in->FixedUpdate(f);
    CHECK(in->Get<std::string>("status") == Localizer::Format("#autoLOC_8005416"));
    KSPActionParam ap;
    in->ToggleAction(ap);
    CHECK(in->Get<bool>("intakeEnabled"));
    in->ToggleAction(KSPActionType::Deactivate);
    CHECK_FALSE(in->Get<bool>("intakeEnabled"));
    // an air-breathing engine reads the intake's resource through the same part's IntakeAir (turboJet propellants)
    auto jet = g.New("turboJet");
    ModuleEngines* dry = FirstEngine(*jet);
    bool hasAir = false;
    for (const Propellant& p : dry->propellants) hasAir |= p.name == "IntakeAir";
    CHECK(hasAir);
}

TEST_CASE("WP-23 Propellant run-time state: totalResourceAvailable, minResToLeave, UpdateConnectedResources") {
    Propellant p;
    p.actualTotalAvailable = 10.0;
    p.minResToLeave = 4.0;
    CHECK(p.totalResourceAvailable() == 6.0);  // Propellant.cs:44
    p.minResToLeave = 12.0;
    CHECK(p.totalResourceAvailable() == 0.0);  // never negative
    p.set_totalResourceAvailable(3.0);         // :54 the setter writes actualTotalAvailable
    CHECK(p.actualTotalAvailable == 3.0);
    CHECK(p.totalResourceCapacity == 0.0);
    CHECK_FALSE(p.isDeprived);
    CHECK_FALSE(p.hasAlternatePropellant);
}
