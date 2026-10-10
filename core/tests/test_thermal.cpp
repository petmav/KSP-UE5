// WP-22 unit tests: the thermal model pieces that have no in-game capture.
//
// What the captures gate (not repeated here): CelestialBody.GetAtmoThermalStats (capture.atmosphere_thermal_offsets) and the
// FlightIntegrator thermal step (capture.thermal_step: SetSkinProperties, convection, radiation, skin conduction, integration).
// Everything below is derived from the decompiled source (the derivation is in the comment above each check) and from the game's
// own data: the part prefabs of the GameData mirror (HeatShield1, mk1Pod_v2, foldingRadSmall, ISRU, the PhysProf crafts). No
// number is invented: each expectation is the C# formula evaluated with the cfg values read from the prefab.
#include <cmath>
#include <memory>

#include "resource_fixture.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/thermal/flight_integrator_thermal.hpp"
#include "ksp/thermal/thermal_modules.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_assembly.hpp"

using namespace ksp;
using namespace ksp::rtest;
using doctest::Approx;

namespace {

const std::string kHarness22 = KSPCORE_HARNESS_DIR;

// The stock game + PhysicsGlobals (level0 prefab + harness Physics.cfg) as the flight scene has them
struct World22 {
    Game15 game;
    PhysicsGlobals pg;
    World22() {
        pg.physicsDatabaseFilename = kHarness22 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
    }
    // Part.Start() in flight (absorptiveConstant, thermal mass block, 288 K)
    static void StartInFlight(Part& p) {
        Part::StartContext ctx;
        ctx.flight = true;
        ctx.standardSpecificHeatCapacity = PhysicsGlobals::StandardSpecificHeatCapacity();
        p.StartData(ctx);
    }
};

template <class M>
M* ModuleOf(Part& p, const char* cls) {
    M* m = dynamic_cast<M*>(p.FindModuleOfClass(cls));
    REQUIRE_MESSAGE(m != nullptr, cls);
    return m;
}

}  // namespace

// TemperatureModel.cs (internal, unused by the game): every formula below is the C# one with float arithmetic.
TEST_CASE("WP-22 TemperatureModel: linear, quadratic, constant and stepped models") {
    // LinearTemperatureModel(temp1, alt1, temp2, alt2): m = (temp1 - temp2) / (alt1 - alt2), b = temp1 - m * alt1
    //   (300, 0) and (400, 1000): m = (300 - 400) / (0 - 1000) = 0.1, b = 300 -> T(500) = 350
    TemperatureModel::Linear lin(300.0f, 0.0f, 400.0f, 1000.0f);
    CHECK(lin.getTemperatureAtAltitude(500.0f) == Approx(350.0f));
    CHECK(lin.getTemperatureAtAltitude(0.0f) == 300.0f);
    // Quadratic: A h^2 + B h + C
    TemperatureModel::Quadratic quad(0.001f, -1.0f, 300.0f);
    CHECK(quad.getTemperatureAtAltitude(100.0f) == Approx(0.001f * 100.0f * 100.0f - 100.0f + 300.0f));
    TemperatureModel::Constant k(288.0f);
    CHECK(k.getTemperatureAtAltitude(12345.0f) == 288.0f);

    // Stepped: AddInitialConstantStep(min, max, K) then AddLinearStep(limit, temp2) continues from the previous step's end value:
    //   step 0: [0, 100] constant 300; step 1 (limit 200, temp2 250): linear from (T0(100) = 300, 100) to (250, 200)
    TemperatureModel m;
    CHECK(m.AddInitialConstantStep(0.0f, 100.0f, 300.0f));
    CHECK_FALSE(m.AddInitialConstantStep(0.0f, 100.0f, 1.0f));  // only while there is no step
    CHECK(m.AddLinearStep(200.0f, 250.0f));
    CHECK(m.getTemperatureAtAltitude(50.0) == 300.0f);
    CHECK(m.getTemperatureAtAltitude(150.0) == Approx(275.0f));
    CHECK(m.getTemperatureAtAltitude(200.0) == Approx(250.0f));
    // beyond the last step: the search falls off the list -> 0
    CHECK(m.getTemperatureAtAltitude(500.0) == 0.0f);
    // AddConstantStep looks for the first step whose maxAlt is below the limit: a limit not above any step's maxAlt (50 <= 100, 200) is refused
    CHECK_FALSE(m.AddConstantStep(50.0f, 1.0f));
    // the boundary test is `altitude < maxAlt + 0.0001 && !(altitude <= minAlt - 0.0001)`: altitude exactly at the junction (100)
    // belongs to the first step (constant 300), not to the linear step
    CHECK(m.getTemperatureAtAltitude(100.0) == 300.0f);
}

// RadiatorUtilities.cs: GetThermalData / TransferEnergy on real part values (foldingRadSmall: maxTemp 2500, radiatorHeadroom 0.75)
TEST_CASE("WP-22 RadiatorUtilities.GetThermalData and TransferEnergy") {
    World22 w;
    auto rad = w.game.New("foldingRadSmall");
    auto hot = w.game.New("mk1pod.v2");
    World22::StartInFlight(*rad);
    World22::StartInFlight(*hot);
    CHECK(rad->fields.maxTemp == 2500.0);
    CHECK(rad->fields.radiatorHeadroom == 0.75);
    rad->fields.temperature = 400.0;
    hot->fields.temperature = 1200.0;
    const Radiators::RadiatorData r = RadiatorUtilities::GetThermalData(*rad);
    const Radiators::RadiatorData h = RadiatorUtilities::GetThermalData(*hot);
    // Energy = thermalMass * temperature; MaxEnergy = thermalMass * maxTemp * radiatorMax; EnergyCap = thermalMass * maxTemp * radiatorHeadroom
    CHECK(r.Energy == rad->fields.thermalMass * 400.0);
    CHECK(r.MaxEnergy == rad->fields.thermalMass * rad->fields.maxTemp * rad->fields.radiatorMax);
    CHECK(r.EnergyCap == rad->fields.thermalMass * 2500.0 * 0.75);
    CHECK(r.part == rad.get());
    // TransferEnergy: min(tgt.EnergyCap - tgt.Energy, src.Energy - src.MaxEnergy) leaves the source and enters the target
    const double xfer = std::min(r.EnergyCap - r.Energy, h.Energy - h.MaxEnergy);
    rad->fields.thermalInternalFlux = 0.0;
    hot->fields.thermalInternalFlux = 0.0;
    RadiatorUtilities::TransferEnergy(h, r);
    CHECK(hot->fields.thermalInternalFlux == -xfer);
    CHECK(rad->fields.thermalInternalFlux == xfer);
}

// ModuleConductionMultiplier.cs:24 on the real mk1Pod_v2 part: modifiedConductionFactor 0.003, convectionFluxThreshold 3000
TEST_CASE("WP-22 ModuleConductionMultiplier: heat conductivity drops with the convective flux per exposed area") {
    World22 w;
    auto pod = w.game.New("mk1pod.v2");
    ModuleConductionMultiplier* m = ModuleOf<ModuleConductionMultiplier>(*pod, "ModuleConductionMultiplier");
    CHECK(m->Get<double>("modifiedConductionFactor") == 0.003);
    CHECK(m->Get<double>("convectionFluxThreshold") == 3000.0);
    const double orig = pod->fields.heatConductivity;
    CHECK(orig == Approx(0.1));  // mk1Pod_v2.cfg: heatConductivity = 0.1 (loaded through a float cast, as the capture shows for other parts)
    m->OnAwake();
    m->Start();
    CHECK(m->originalConduction == orig);
    CHECK(m->threshRecip == 1.0 / 3000.0);
    // t = (flux / exposedArea - threshold) * threshRecip, Lerp clamps t to [0, 1]
    pod->fields.exposedArea = 2.0;
    pod->fields.thermalConvectionFlux = 6000.0;  // 3000 / area -> t = 0
    m->FixedUpdate();
    CHECK(pod->fields.heatConductivity == orig);
    pod->fields.thermalConvectionFlux = 12000.0;  // 6000 / area -> t = 1: orig * factor
    m->FixedUpdate();
    CHECK(pod->fields.heatConductivity == Approx(orig * 0.003));
    pod->fields.thermalConvectionFlux = 9000.0;  // 4500 -> t = 0.5
    m->FixedUpdate();
    CHECK(pod->fields.heatConductivity == Approx(orig * (1.0 + 0.003) / 2.0));
    pod->fields.thermalConvectionFlux = 1.0e9;  // far above: clamped to t = 1
    m->FixedUpdate();
    CHECK(pod->fields.heatConductivity == Approx(orig * 0.003));
    // no exposed area: the original value is restored
    pod->fields.exposedArea = 0.0;
    m->FixedUpdate();
    CHECK(pod->fields.heatConductivity == orig);
    // outside the flight scene the module does nothing
    ThermalModuleEnvironment::loadedSceneIsFlight = false;
    pod->fields.heatConductivity = 7.0;
    m->FixedUpdate();
    CHECK(pod->fields.heatConductivity == 7.0);
    ThermalModuleEnvironment::loadedSceneIsFlight = true;
}

// ModuleAblator.cs:166 on the real HeatShield1 (Ablator, lossExp -7500, lossConst 0.1, pyrolysisLossFactor 6000, threshold 500, reentryConductivity 0.01)
TEST_CASE("WP-22 ModuleAblator: conductivity switch and ablation loss / flux") {
    World22 w;
    auto shield = w.game.New("HeatShield1");
    World22::StartInFlight(*shield);
    ModuleAblator* m = ModuleOf<ModuleAblator>(*shield, "ModuleAblator");
    CHECK(m->Get<double>("lossExp") == -7500.0);
    CHECK(m->Get<double>("lossConst") == 0.1);
    CHECK(m->Get<double>("ablationTempThresh") == 500.0);
    m->charModuleFound = true;
    m->Start();
    const double origConductivity = shield->fields.heatConductivity;
    CHECK(m->origConductivity == origConductivity);
    REQUIRE(m->useAblator);
    const PartResourceDefinition* ablator = PartResourceLibrary::Instance()->GetDefinition("Ablator");
    REQUIRE(ablator != nullptr);
    // pyrolysisLoss = pyrolysisLossFactor * specificHeatCapacity; density = definition.density
    CHECK(m->pyrolysisLoss == 6000.0 * static_cast<double>(ablator->specificHeatCapacity()));
    CHECK(m->density == static_cast<double>(ablator->density()));

    // no skin heating: below the threshold nothing ablates; the conductivity follows the occlusion of the bottom face
    shield->fields.skinTemperature = 300.0;
    m->FixedUpdate();
    CHECK(m->Get<double>("loss") == 0.0);
    CHECK(m->Get<double>("flux") == 0.0);
    // not occluded (AreaOccluded[3] = 0 <= 0.1 * WeightedArea[3]): the original conductivity
    CHECK(shield->fields.heatConductivity == origConductivity);
    // occluded bottom (AreaOccluded > 0.1 * WeightedArea): reentryConductivity
    shield->DragCubes.WeightedArea()[3] = 2.0f;
    shield->DragCubes.AreaOccluded()[3] = 1.0f;
    m->FixedUpdate();
    CHECK(shield->fields.heatConductivity == 0.01);
}

// ModuleActiveRadiator.cs:276 InternalCooling and :353 IsSibling, on the real foldingRadSmall (maxEnergyTransfer 2500, overcoolFactor 0.25)
TEST_CASE("WP-22 ModuleActiveRadiator: IsSibling and InternalCooling energy transfer") {
    World22 w;
    auto root = w.game.New("mk1pod.v2");
    auto rad = w.game.New("foldingRadSmall");
    auto other = w.game.New("mk1pod.v2");
    for (Part* p : {root.get(), rad.get(), other.get()}) World22::StartInFlight(*p);
    // rad is a child of root; other is a second child of root (a sibling of rad)
    rad->setParent(root.get());
    other->setParent(root.get());
    ModuleActiveRadiator* m = ModuleOf<ModuleActiveRadiator>(*rad, "ModuleActiveRadiator");
    CHECK(m->Get<double>("maxEnergyTransfer") == 2500.0);
    CHECK(m->Get<double>("overcoolFactor") == 0.25);
    // IsSibling: the radiator's parent, the grandparent, or a part with the same parent
    CHECK(m->IsSibling(*root));
    CHECK(m->IsSibling(*other));
    auto stray = w.game.New("mk1pod.v2");
    CHECK_FALSE(m->IsSibling(*stray));

    // InternalCooling needs a vessel for externalTemperature / IsFirstFrame: a real assembled one stands in (the PhysProf-S craft)
    SaveEnvironment::Current().newGuidHashCode = [] { return 1; };
    Vessel host;  // only externalTemperature is read
    host.externalTemperature = 4.0;
    root->vessel = rad->vessel = other->vessel = &host;
    const double dt = static_cast<double>(TimeWarp::fixedDeltaTime());
    // a hot part (temperature above maxTemp * radiatorMax and above the radiator's own temperature * overcoolFactor)
    other->fields.temperature = other->fields.maxTemp * other->fields.radiatorMax + 300.0;
    rad->fields.temperature = 300.0;
    rad->fields.thermalInternalFlux = 0.0;
    other->fields.thermalInternalFlux = 0.0;
    m->nonRadiatorParts = {other.get()};
    Radiators::RadiatorData radData = RadiatorUtilities::GetThermalData(*rad);
    const Radiators::RadiatorData hotData = RadiatorUtilities::GetThermalData(*other);
    m->InternalCooling(radData, 1);
    // coolParts = {other}: excess = (Energy - MaxEnergy) / dt / (radCount + coolParts) ; headroom = min(EnergyCap - Energy, maxEnergyTransfer) / dt;
    // transfer = min(headroom, excess) * min(1, energyTransferScale)
    const double excess = (hotData.Energy - hotData.MaxEnergy) / dt / 2.0;
    const double headroom = std::min(radData.EnergyCap - radData.Energy, 2500.0) / dt;
    const double expected = std::min(headroom, excess) * std::min(1.0, m->Get<double>("energyTransferScale"));
    CHECK(m->coolParts.size() == 1);
    CHECK(expected > 0.0);
    // AddThermalFlux(kW) = kW * fixedDeltaTime: the hot part loses, the radiator gains
    CHECK(other->fields.thermalInternalFlux == Approx(-expected * dt));
    CHECK(rad->fields.thermalInternalFlux == Approx(expected * dt));
    // a part cooler than radiator.temperature * overcoolFactor is not cooled
    other->fields.temperature = 1.0;
    other->fields.thermalInternalFlux = 0.0;
    m->InternalCooling(radData, 1);
    CHECK(m->coolParts.empty());
    root->vessel = rad->vessel = other->vessel = nullptr;
}

// ModuleCoreHeat.cs on the real ISRU part (CoreTempGoal 1000, CoreEnergyMultiplier 0.1, CoreToPartRatio 0.1, HeatRadiantMultiplier 0.05, ...)
TEST_CASE("WP-22 ModuleCoreHeat: core temperature, starting temperature, delta time, converter heat") {
    World22 w;
    auto isru = w.game.New("ISRU");
    World22::StartInFlight(*isru);
    ModuleCoreHeat* m = ModuleOf<ModuleCoreHeat>(*isru, "ModuleCoreHeat");
    m->OnAwake();
    CHECK(m->Get<double>("CoreTempGoal") == 1000.0);
    CHECK(m->Get<double>("CoreShutdownTemp") == 4000.0);
    CHECK(m->Get<double>("MaxCoolant") == 200.0);
    // CheckStartingTemperature: NaN core energy, or a core below 5 K, restarts at thermalMass * temperature
    isru->fields.temperature = 288.0;
    m->Set<double>("CoreThermalEnergy", std::numeric_limits<double>::quiet_NaN());
    m->CheckStartingTemperature();
    CHECK(m->Get<double>("CoreThermalEnergy") == isru->fields.thermalMass * 288.0);
    // CoreTemperature = CoreThermalEnergy / thermalMass
    CHECK(m->CoreTemperature() == 288.0);
    m->Set<double>("CoreThermalEnergy", isru->fields.thermalMass * 700.0);
    CHECK(m->CoreTemperature() == Approx(700.0));
    // AddEnergyToCore accumulates in energyTemp; ResolveNetEnergy folds it into CoreThermalEnergy
    m->AddEnergyToCore(10.0);
    m->AddEnergyToCore(5.0);
    const double before = m->Get<double>("CoreThermalEnergy");
    CHECK(m->energyTemp == 15.0);
    m->ResolveNetEnergy();
    CHECK(m->Get<double>("CoreThermalEnergy") == before + 15.0);
    CHECK(m->energyTemp == 0.0);
    // GetTransferAmount = energy * multiplier
    CHECK(m->GetTransferAmount(100.0, m->Get<double>("HeatRadiantMultiplier")) == 100.0 * 0.05);

    // GetDeltaTime (Mono: Math.Abs / Math.Min on doubles): -1 until the level has loaded 1 s and FlightGlobals is ready
    ThermalModuleEnvironment::timeSinceLevelLoad = 0.5f;
    CHECK(m->GetDeltaTime() == -1.0);
    ThermalModuleEnvironment::timeSinceLevelLoad = 5.0f;
    ThermalModuleEnvironment::universalTime = 1000.0;
    m->lastUpdateTime = 0.0;
    CHECK(m->GetDeltaTime() == -1.0);  // first call only records the time
    CHECK(m->lastUpdateTime == 1000.0);
    m->lastCoreTemp = m->CoreTemperature();
    CHECK(m->GetDeltaTime() == static_cast<double>(TimeWarp::fixedDeltaTime()));  // core temperature unchanged
    m->lastCoreTemp = 0.0;
    ThermalModuleEnvironment::universalTime = 1010.0;
    // min(UT - lastUpdateTime, MaxCalculationWarp / 50) = min(10, 20) = 10
    CHECK(m->GetDeltaTime() == 10.0);
    CHECK(m->lastUpdateTime == 1010.0);
    m->lastCoreTemp = 0.0;
    ThermalModuleEnvironment::universalTime = 1100.0;
    CHECK(m->GetDeltaTime() == 20.0);  // clamped to MaxCalculationWarp / 50

    // converters: an active converter that generates heat adds TemperatureModifier(CoreTemperature) * dt (scaled down above
    // lastTimeFactor 1, reduced by the crew bonus above the goal temperature); the seam is ConverterState
    FloatCurve curve;
    curve.Add(0.0f, 10.0f);
    ModuleCoreHeat::ConverterState conv;
    conv.isActivated = true;
    conv.generatesHeat = true;
    conv.lastTimeFactor = 2.0;
    conv.temperatureModifier = &curve;
    conv.crewHeatBonus = 0.25;
    m->converters = {&conv};
    CHECK(m->HasActiveConverters());
    CHECK(m->HasEnabledConverters());
    m->Set<double>("CoreThermalEnergy", isru->fields.thermalMass * 700.0);  // below the 1000 K goal: no crew bonus
    m->lastFlux = 3.5;
    m->energyTemp = 0.0;
    m->ResolveConverterEnergy(0.02);
    // num = 10 * 0.02 = 0.2; lastTimeFactor > 1 -> /2 = 0.1
    CHECK(m->energyTemp == Approx(0.1));
    CHECK(conv.lastHeatFlux == 3.5);
    m->Set<double>("CoreThermalEnergy", isru->fields.thermalMass * 1500.0);  // above the goal: num -= num * crewHeatBonus
    m->energyTemp = 0.0;
    m->ResolveConverterEnergy(0.02);
    CHECK(m->energyTemp == Approx(0.1 - 0.1 * 0.25));
    conv.isActivated = false;
    CHECK_FALSE(m->HasActiveConverters());
    CHECK_FALSE(m->HasEnabledConverters());
    m->converters.clear();
}

// FlightIntegratorThermal: the conduction pass on a real two-part graph (HeatShield1 below mk1pod_v2): energy flows from the hot part to the cold one
TEST_CASE("WP-22 FlightIntegratorThermal.UpdateConduction: heat flows hot -> cold through the thermal link") {
    World22 w;
    auto pod = w.game.New("mk1pod.v2");
    auto shield = w.game.New("HeatShield1");
    Attach(*pod, "bottom", *shield, "top");
    for (Part* p : {pod.get(), shield.get()}) World22::StartInFlight(*p);
    pod->fields.temperature = 600.0;
    shield->fields.temperature = 300.0;
    pod->fields.skinTemperature = shield->fields.skinTemperature = 300.0;
    pod->fields.skinUnexposedTemperature = shield->fields.skinUnexposedTemperature = 300.0;
    Vessel host;
    host.parts = {pod.get(), shield.get()};
    pod->vessel = shield->vessel = &host;

    FlightIntegratorThermal fi;
    fi.vessel = &host;
    fi.CacheGlobals();
    fi.partCount = 2;
    fi.UpdateThermalGraph();
    REQUIRE(fi.partThermalDataCount == 2);
    // ThermalLink: parent <-> child, contact area from the node (max(contactArea, 0.01)), sqrt cached
    REQUIRE(pod->ptd != nullptr);
    REQUIRE(pod->ptd->thermalLinks.size() == 1);
    const ThermalLink& link = *pod->ptd->thermalLinks[0];
    CHECK(link.remotePart == shield.get());
    CHECK(link.contactArea >= 0.01);
    CHECK(link.contactAreaSqrt == std::sqrt(link.contactArea));
    CHECK(shield->ptd->thermalLinks.size() == 1);
    // each part's conduction multiplier (PrecalcConduction) is what the pass reads
    fi.PrecalcConduction(*pod->ptd);
    fi.PrecalcConduction(*shield->ptd);
    CHECK(pod->ptd->conductionMult == fi.cacheConductionFactor * 10.0 * pod->fields.heatConductivity);
    fi.UpdateConduction();
    // the hot part loses what the cold one gains (before the convergence factor)
    const double hotFlux = pod->ptd->intConductionFlux;
    const double coldFlux = shield->ptd->intConductionFlux;
    CHECK(hotFlux + coldFlux == Approx(0.0).epsilon(1e-9));
    CHECK(coldFlux > 0.0);
    // conducted energy per link = contactArea * conductionMult(hot) * heatConductivity(cold) * deltaT (ThermalLink.temperatureDelta
    // is set in the pass); no clamp applies for this small delta
    const double expected = link.contactArea * (pod->ptd->conductionMult * shield->fields.heatConductivity) *
                            (pod->fields.temperature - shield->fields.temperature);
    // the pass iterates from the end of the temperature-sorted list: the hot part is the source
    CHECK(std::abs(coldFlux) <= std::abs(expected) * (1.0 + 1e-12));
    pod->ptd = shield->ptd = nullptr;
    pod->vessel = shield->vessel = nullptr;
}
