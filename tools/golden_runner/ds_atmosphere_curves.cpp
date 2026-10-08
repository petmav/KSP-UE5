// atmosphere_curves candidate: mirrors harness/src/KspGolden/Generators/AtmosphereGen.cs with kspcore's CelestialBody
// and FlightIntegratorAtmosphere (FlightIntegrator.CalculatePressure / CalculateConstantsAtmosphere, called twice per row
// for the steady state, one integrator per body reused across rows like the generator's hosted FlightIntegrator).
#include <set>
#include <string>
#include <vector>

#include "celestial_env.hpp"
#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/celestial/flight_atmosphere.hpp"

using namespace ksp;
using golden::ColType;

namespace {

int RunAtmosphereCurves(const RunnerOptions& opt) {
    CelestialEnv& env = CelestialEnv::Get(opt);
    const std::vector<CelestialBody*>& bodies = env.system->SpawnOrder();

    golden::File g;
    g.dataset = "atmosphere_curves";
    g.description = "kspcore candidate for atmosphere_curves.";

    golden::Table tp("profile_100m", "Kerbin at 100 m steps; other atmospheric bodies at 500 m steps; depth +/- 1 m.");
    tp.Col("body", ColType::Str);
    for (const char* c : {"altitude", "pressure_kPa", "pressure_atm", "temperature", "density", "speedOfSound"}) tp.Col(c, ColType::F64);
    tp.Col("tempSunMult", ColType::F32).Col("fullTemperature_off20", ColType::F64);
    tp.key = {"body", "altitude"};
    for (const CelestialBody* cb : bodies) {
        if (!cb->atmosphere) continue;
        const double step = cb->bodyName == "Kerbin" ? 100.0 : 500.0;
        std::set<double> alts;  // SortedSet<double>
        for (double h = 0; h <= cb->atmosphereDepth; h += step) alts.insert(h);
        alts.insert(cb->atmosphereDepth);
        alts.insert(cb->atmosphereDepth - 1);
        alts.insert(cb->atmosphereDepth + 1);
        alts.insert(-100.0);
        for (double h : alts) {
            const double p = cb->GetPressure(h), T = cb->GetTemperature(h), rho = cb->GetDensity(p, T);
            tp.Add({cb->bodyName, h, p, cb->GetPressureAtm(h), T, rho, cb->GetSpeedOfSound(p, rho),
                    cb->atmosphereTemperatureSunMultCurve.Evaluate(static_cast<float>(h)), cb->GetFullTemperature(h, 20.0)});
        }
    }
    g.tables.push_back(std::move(tp));

    golden::Table tf("flight_state", "FlightIntegrator.CalculatePressure + CalculateConstantsAtmosphere (steady state).");
    tf.Col("body", ColType::Str).Col("situation", ColType::Str);
    for (const char* c : {"altitude", "speed", "sunDot", "atmTempOffset", "staticPressurekPa", "staticPressureAtm",
                          "atmosphericTemperature", "density", "dynamicPressurekPa", "speedOfSound", "mach", "solarAirMass",
                          "solarFluxMultiplier", "convectiveMachLerp", "shockTemperature", "externalTemperature",
                          "convectiveCoefficient", "convNewtonian", "convMach", "pseudoReynolds"})
        tf.Col(c, ColType::F64);
    tf.Col("pseudoReDragMult", ColType::F32).Col("densityThermalLerp", ColType::F64).Col("backgroundRadiationTemp", ColType::F64);
    tf.key = {"body", "situation", "altitude", "speed", "sunDot", "atmTempOffset"};
    const double speeds[] = {0, 10, 50, 100, 200, 300, 340, 400, 600, 1000, 1500, 2000, 2500, 3000, 4000, 6000};
    for (const CelestialBody* cb : bodies) {
        if (!cb->atmosphere || cb->bodyName == "Sun") continue;
        FlightIntegratorAtmosphere fi;  // AtmosphereGen.HostFlightIntegrator
        fi.currentMainBody = cb;
        fi.situation = VesselSituations::FLYING;
        const bool kerbin = cb->bodyName == "Kerbin";
        std::vector<double> alts;
        if (kerbin) {
            alts = {0.0, 500, 1000, 2500, 5000, 7500, 10000, 15000, 20000, 25000, 30000, 35000, 40000, 45000, 50000, 55000,
                    60000, 65000, 69000, 69999, 70000};
        } else {
            for (int i = 0; i < 11; ++i) alts.push_back(cb->atmosphereDepth * i / 10.0);
            alts.push_back(cb->atmosphereDepth - 1);
        }
        std::vector<std::pair<double, double>> sunGrid = {{1.0, 0.0}};
        if (kerbin) sunGrid = {{1.0, 0.0}, {0.0, 0.0}, {-1.0, 0.0}, {0.5, 15.0}};
        std::vector<VesselSituations> sits = {VesselSituations::FLYING};
        if (kerbin) sits.push_back(VesselSituations::SPLASHED);
        for (VesselSituations sit : sits)
            for (double h : alts) {
                if (sit == VesselSituations::SPLASHED && h > 0) continue;
                for (double v : speeds)
                    for (const auto& [sunDot, off] : sunGrid) {
                        fi.situation = sit;
                        fi.altitude = h;
                        fi.spd = v;
                        fi.sunDot = sunDot;
                        fi.atmosphereTemperatureOffset = off;
                        fi.solarFlux = 1360.0;
                        fi.density = 0;
                        fi.CalculatePressure();
                        fi.CalculateConstantsAtmosphere();
                        fi.CalculatePressure();
                        fi.CalculateConstantsAtmosphere();
                        const double shock = fi.CalculateShockTemperature();
                        const double cn = fi.CalculateConvectiveCoefficientNewtonian();
                        const double cmach = fi.CalculateConvectiveCoefficientMach();
                        const double dtl = fi.CalculateDensityThermalLerp();
                        fi.densityThermalLerp = dtl;
                        const double bgT = fi.CalculateBackgroundRadiationTemperature(fi.atmosphericTemperature);
                        tf.Add({cb->bodyName, ToString(sit), h, v, sunDot, off, fi.staticPressurekPa, fi.staticPressureAtm,
                                fi.atmosphericTemperature, fi.density, fi.dynamicPressurekPa, fi.speedOfSound, fi.mach,
                                fi.solarAirMass, fi.solarFluxMultiplier, fi.convectiveMachLerp, shock, fi.externalTemperature,
                                fi.convectiveCoefficient, cn, cmach, fi.pseudoReynolds, static_cast<float>(fi.pseudoReDragMult),
                                dtl, bgT});
                    }
            }
    }
    g.tables.push_back(std::move(tf));

    golden::Table ts("solar_effects", "CelestialBody.GetSolarAtmosphericEffects(sunDot, density) and GetSolarPowerFactor(density).");
    ts.Col("body", ColType::Str);
    for (const char* c : {"sunDot", "density", "solarAirMass", "solarFluxMultiplier", "solarPowerFactor"}) ts.Col(c, ColType::F64);
    ts.key = {"body", "sunDot", "density"};
    for (const char* name : {"Kerbin", "Eve", "Duna", "Laythe", "Jool"}) {
        const CelestialBody* cb = env.system->GetBodyByName(name);
        for (double sd : {-1.0, -0.2, 0.0, 0.05, 0.2, 0.5, 0.866, 1.0})
            for (double rho : {0.0, 1e-6, 1e-3, 0.01, 0.1, 0.5, 1.225, 5.0}) {
                double am = 0, fm = 0;
                cb->GetSolarAtmosphericEffects(sd, rho, am, fm);
                ts.Add({name, sd, rho, am, fm, cb->GetSolarPowerFactor(rho)});
            }
    }
    g.tables.push_back(std::move(ts));
    return WriteCandidate(opt, g, "atmosphere_curves/atmosphere_curves.json");
}

}  // namespace

KSP_GOLDEN_DATASET("atmosphere_curves", 63, RunAtmosphereCurves);
