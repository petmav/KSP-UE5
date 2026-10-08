// body_catalog candidate: mirrors harness/src/KspGolden/Generators/BodiesGen.cs on kspcore's spawned Kerbol system
// (CelestialEnv: KerbolSystemPrefab() at UT 0). Tables:
//   bodies                 every column (tidal-lock rotationPeriod and solarDayLength use the gate's StandInOrbitPeriod,
//                          the Orbit.Init period formula, until WP-07's ksp::Orbit is attached)
//   orbits                 the serialized elements (name, parent, reverse, inclination ... epoch); the propagated columns
//                          (meanMotion ... vel_z) need ksp::Orbit and are excluded in tests/CMakeLists.txt (pending WP-07)
//   atmosphere_profile_1km GetPressure / GetPressureAtm / GetTemperature / GetDensity / GetSpeedOfSound
//   body_curves            the keyframes of every FloatCurve of every body, read back from the spawned bodies
#include <string>
#include <vector>

#include "celestial_env.hpp"
#include "datasets.hpp"
#include "golden_json.hpp"

using namespace ksp;
using golden::ColType;

namespace {

int RunBodyCatalog(const RunnerOptions& opt) {
    CelestialEnv& env = CelestialEnv::Get(opt);
    const std::vector<CelestialBody*>& bodies = env.system->SpawnOrder();
    const PSystemPrefab& prefab = KerbolSystemPrefab();

    golden::File g;
    g.dataset = "body_catalog";
    g.description = "kspcore candidate for body_catalog (KerbolSystemPrefab spawned at UT 0).";

    golden::Table t("bodies", "Physical and rotational parameters per body (in-game state at UT=0).");
    for (const char* c : {"name"}) t.Col(c, ColType::Str);
    t.Col("flightGlobalsIndex", ColType::I64).Col("parent", ColType::Str).Col("spawnOrder", ColType::I64);
    for (const char* c : {"Radius", "GeeASL", "Mass", "gravParameter", "gMagnitudeAtCenter", "Density", "SurfaceArea",
                          "sphereOfInfluence", "hillSphere"})
        t.Col(c, ColType::F64);
    for (const char* c : {"rotates", "tidallyLocked", "solarRotationPeriod"}) t.Col(c, ColType::Bool);
    for (const char* c : {"rotationPeriod_serialized", "rotationPeriod", "rotPeriodRecip", "angularV", "initialRotation",
                          "rotationAngle_UT0", "solarDayLength"})
        t.Col(c, ColType::F64);
    t.Col("inverseRotThresholdAltitude", ColType::F32);
    for (const char* c : {"hasSolidSurface", "isStar", "isHomeWorld", "ocean"}) t.Col(c, ColType::Bool);
    t.Col("oceanDensity", ColType::F64).Col("atmosphere", ColType::Bool).Col("atmosphereContainsOxygen", ColType::Bool);
    for (const char* c : {"atmosphereDepth", "atmosphereTemperatureSeaLevel", "atmospherePressureSeaLevel", "atmosphereMolarMass",
                          "atmosphereAdiabaticIndex", "atmosphereTemperatureLapseRate", "atmosphereGasMassLapseRate"})
        t.Col(c, ColType::F64);
    for (const char* c : {"atmosphereUsePressureCurve", "atmospherePressureCurveIsNormalized", "atmosphereUseTemperatureCurve",
                          "atmosphereTemperatureCurveIsNormalized"})
        t.Col(c, ColType::Bool);
    for (const char* c : {"radiusAtmoFactor", "atmDensityASL", "atmPressureASL", "albedo", "emissivity", "coreTemperatureOffset",
                          "convectionMultiplier", "shockTemperatureMultiplier", "maxAxialDot"})
        t.Col(c, ColType::F64);
    t.Col("timeWarpAltitudeLimits", ColType::Str);
    t.key = {"name"};
    for (size_t i = 0; i < bodies.size(); ++i) {
        const CelestialBody& cb = *bodies[i];
        std::string limits;
        for (size_t k = 0; k < cb.timeWarpAltitudeLimits.size(); ++k)
            limits += (k ? " " : "") + ExactFormatFloat(cb.timeWarpAltitudeLimits[k]);
        const std::string parent = cb.orbitDriver ? cb.orbitDriver->referenceBody->bodyName : "";
        t.Add({cb.bodyName, cb.flightGlobalsIndex, parent, static_cast<int64_t>(i), cb.Radius, cb.GeeASL, cb.Mass,
               cb.gravParameter, cb.gMagnitudeAtCenter, cb.Density, cb.SurfaceArea, cb.sphereOfInfluence, cb.hillSphere,
               cb.rotates, cb.tidallyLocked, cb.solarRotationPeriod, prefab.bodies[i].body.rotationPeriod, cb.rotationPeriod,
               cb.rotPeriodRecip, cb.angularV, cb.initialRotation, cb.rotationAngle, cb.solarDayLength,
               cb.inverseRotThresholdAltitude, cb.hasSolidSurface, cb.isStar, cb.isHomeWorld, cb.ocean, cb.oceanDensity,
               cb.atmosphere, cb.atmosphereContainsOxygen, cb.atmosphereDepth, cb.atmosphereTemperatureSeaLevel,
               cb.atmospherePressureSeaLevel, cb.atmosphereMolarMass, cb.atmosphereAdiabaticIndex,
               cb.atmosphereTemperatureLapseRate, cb.atmosphereGasMassLapseRate, cb.atmosphereUsePressureCurve,
               cb.atmospherePressureCurveIsNormalized, cb.atmosphereUseTemperatureCurve,
               cb.atmosphereTemperatureCurveIsNormalized, cb.radiusAtmoFactor, cb.atmDensityASL, cb.atmPressureASL, cb.albedo,
               cb.emissivity, cb.coreTemperatureOffset, cb.convectionMultiplier, cb.shockTemperatureMultiplier, cb.maxAxialDot,
               limits});
    }
    g.tables.push_back(std::move(t));

    golden::Table o("orbits", "Orbital elements relative to the parent body (serialized; propagated columns pending WP-07).");
    o.Col("name", ColType::Str).Col("parent", ColType::Str).Col("reverse", ColType::Bool);
    for (const char* c : {"inclination", "eccentricity", "semiMajorAxis", "LAN", "argumentOfPeriapsis", "meanAnomalyAtEpoch", "epoch"})
        o.Col(c, ColType::F64);
    o.key = {"name"};
    for (const CelestialBody* cb : bodies) {
        if (!cb->orbitDriver) continue;
        const CelestialOrbitElements& e = cb->orbitDriver->orbit;
        o.Add({cb->bodyName, cb->orbitDriver->referenceBody->bodyName, cb->orbitDriver->reverse, e.inclination, e.eccentricity,
               e.semiMajorAxis, e.LAN, e.argumentOfPeriapsis, e.meanAnomalyAtEpoch, e.epoch});
    }
    g.tables.push_back(std::move(o));

    golden::Table a("atmosphere_profile_1km", "CelestialBody atmosphere functions at 1 km steps, plus depth+1 m.");
    a.Col("name", ColType::Str);
    for (const char* c : {"altitude", "pressure_kPa", "pressure_atm", "temperature", "density", "speedOfSound"}) a.Col(c, ColType::F64);
    a.key = {"name", "altitude"};
    for (const CelestialBody* cb : bodies) {
        if (!cb->atmosphere) continue;
        std::vector<double> alts;
        for (double h = 0; h <= cb->atmosphereDepth; h += 1000.0) alts.push_back(h);
        if (alts.back() != cb->atmosphereDepth) alts.push_back(cb->atmosphereDepth);
        alts.push_back(cb->atmosphereDepth + 1.0);
        for (double h : alts) {
            const double p = cb->GetPressure(h), T = cb->GetTemperature(h), rho = cb->GetDensity(p, T);
            a.Add({cb->bodyName, h, p, cb->GetPressureAtm(h), T, rho, cb->GetSpeedOfSound(p, rho)});
        }
    }
    g.tables.push_back(std::move(a));

    golden::Table c("body_curves", "Keyframes of every FloatCurve on each CelestialBody.");
    c.Col("name", ColType::Str).Col("curve", ColType::Str).Col("index", ColType::I64);
    for (const char* n : {"time", "value", "inTangent", "outTangent"}) c.Col(n, ColType::F32);
    c.Col("weightedMode", ColType::I64).Col("preInfinity", ColType::I64).Col("postInfinity", ColType::I64);
    c.key = {"name", "curve", "index"};
    for (const CelestialBody* cb : bodies) {
        // CelestialBody FloatCurve fields in declaration (MetadataToken) order
        const std::pair<const char*, const FloatCurve*> curves[] = {
            {"atmosphereTemperatureCurve", &cb->atmosphereTemperatureCurve},
            {"latitudeTemperatureBiasCurve", &cb->latitudeTemperatureBiasCurve},
            {"latitudeTemperatureSunMultCurve", &cb->latitudeTemperatureSunMultCurve},
            {"axialTemperatureSunMultCurve", &cb->axialTemperatureSunMultCurve},
            {"axialTemperatureSunBiasCurve", &cb->axialTemperatureSunBiasCurve},
            {"atmosphereTemperatureSunMultCurve", &cb->atmosphereTemperatureSunMultCurve},
            {"eccentricityTemperatureBiasCurve", &cb->eccentricityTemperatureBiasCurve},
            {"atmospherePressureCurve", &cb->atmospherePressureCurve}};
        for (const auto& [cn, fc] : curves) {
            const AnimationCurve& ac = fc->Curve();
            const auto& keys = ac.keys();
            for (size_t i = 0; i < keys.size(); ++i)
                c.Add({cb->bodyName, cn, static_cast<int64_t>(i), keys[i].time, keys[i].value, keys[i].inTangent,
                       keys[i].outTangent, static_cast<int64_t>(keys[i].weightedMode), static_cast<int64_t>(ac.NativePreInfinity()),
                       static_cast<int64_t>(ac.NativePostInfinity())});
        }
    }
    g.tables.push_back(std::move(c));
    return WriteCandidate(opt, g, "body_catalog/body_catalog.json");
}

}  // namespace

KSP_GOLDEN_DATASET("body_catalog", 62, RunBodyCatalog);
