using System;
using System.Collections.Generic;
using System.Linq;
using KspGolden.Infra;

namespace KspGolden.Generators
{
    /// <summary>Dataset 3a: atmosphere profiles and the FlightIntegrator atmospheric/thermal state (Kerbin + every atmospheric body).</summary>
    public static class AtmosphereGen
    {
        public static FlightIntegrator HostFlightIntegrator(CelestialBody body, out Vessel vessel)
        {
            FlightIntegrator fi;
            try { fi = U.New<FlightIntegrator>(); }
            catch (Exception) { fi = U.Alive(U.Uninit<FlightIntegrator>()); }
            vessel = U.Alive(U.Uninit<Vessel>());
            vessel.situation = Vessel.Situations.FLYING;
            U.Set(fi, "vessel", vessel);
            U.Set(fi, "currentMainBody", body);
            return fi;
        }

        /// <summary>HighLogic.CurrentGame with default (Normal) GameParameters: ReentryHeatScale = 1 (GameParameters.cs:218).</summary>
        public static void EnsureGame()
        {
            if (HighLogic.CurrentGame != null) return;
            var hl = U.New<HighLogic>();
            U.SetStatic(typeof(HighLogic), "fetch", hl);
            var game = U.Uninit<Game>();
            // GameParameters' static ctor enumerates AssemblyLoader.loadedAssemblies (KSP's plugin list); with no
            // plugins loaded that list is empty, as for a stock install's custom-parameter scan.
            if (AssemblyLoader.loadedAssemblies == null) AssemblyLoader.loadedAssemblies = new AssemblyLoader.LoadedAssembyList();
            game.Parameters = new GameParameters();
            U.Set(hl, "currentGame", game);
        }

        public static IEnumerable<(string, GoldenFile)> Generate()
        {
            var g = new GoldenFile("atmosphere_curves", "direct",
                "Atmosphere and flight-state values computed by KSP's own CelestialBody and FlightIntegrator methods. FlightIntegrator is " +
                "hosted without Unity (fields set: currentMainBody, altitude, spd, sunDot, atmosphereTemperatureOffset, situation) and its real " +
                "protected methods CalculatePressure() and CalculateConstantsAtmosphere() are invoked; CalculateConstantsAtmosphere is called twice " +
                "per row because GetSolarAtmosphericEffects reads the density left by the previous physics frame (the second call is the steady state). " +
                "HighLogic.CurrentGame.Parameters = new GameParameters() (Normal difficulty, ReentryHeatScale = 1).");
            g.Oracle("CelestialBody.GetPressure").Oracle("CelestialBody.GetTemperature").Oracle("CelestialBody.GetFullTemperature(double,double)")
             .Oracle("CelestialBody.GetDensity").Oracle("CelestialBody.GetSpeedOfSound").Oracle("CelestialBody.GetSolarAtmosphericEffects").Oracle("CelestialBody.GetSolarPowerFactor")
             .Oracle("FlightIntegrator.CalculatePressure").Oracle("FlightIntegrator.CalculateConstantsAtmosphere").Oracle("FlightIntegrator.CalculateShockTemperature")
             .Oracle("FlightIntegrator.CalculateConvectiveCoefficient(Vessel.Situations)").Oracle("FlightIntegrator.CalculateConvectiveCoefficientNewtonian")
             .Oracle("FlightIntegrator.CalculateConvectiveCoefficientMach").Oracle("FlightIntegrator.CalculateDensityThermalLerp")
             .Oracle("FlightIntegrator.CalculateBackgroundRadiationTemperature").Oracle("PhysicsGlobals.DragCurvePseudoReynolds.Evaluate");

            var tp = g.AddTable(new Table("profile_100m", "Kerbin at 100 m steps 0..70000 m; other atmospheric bodies at 500 m steps; plus atmosphereDepth +/- 1 m."));
            tp.Col("body", ColType.Str).Col("altitude", ColType.F64, "m").Col("pressure_kPa", ColType.F64, "kPa").Col("pressure_atm", ColType.F64, "atm")
              .Col("temperature", ColType.F64, "K").Col("density", ColType.F64, "kg/m^3").Col("speedOfSound", ColType.F64, "m/s")
              .Col("tempSunMult", ColType.F32, "", "atmosphereTemperatureSunMultCurve.Evaluate((float)alt)")
              .Col("fullTemperature_off20", ColType.F64, "K", "GetFullTemperature(alt, 20)")
              .Keys("body", "altitude");
            foreach (var cb in KspEnv.BodiesBySpawn.Where(b => b.atmosphere))
            {
                double step = cb.bodyName == "Kerbin" ? 100.0 : 500.0;
                var alts = new SortedSet<double>();
                for (double h = 0; h <= cb.atmosphereDepth; h += step) alts.Add(h);
                alts.Add(cb.atmosphereDepth); alts.Add(cb.atmosphereDepth - 1); alts.Add(cb.atmosphereDepth + 1); alts.Add(-100.0);
                foreach (var h in alts)
                {
                    double p = cb.GetPressure(h), T = cb.GetTemperature(h), rho = cb.GetDensity(p, T);
                    tp.Add(cb.bodyName, h, p, cb.GetPressureAtm(h), T, rho, cb.GetSpeedOfSound(p, rho),
                        cb.atmosphereTemperatureSunMultCurve.Evaluate((float)h), cb.GetFullTemperature(h, 20.0));
                }
            }

            var tf = g.AddTable(new Table("flight_state", "FlightIntegrator.CalculatePressure + CalculateConstantsAtmosphere (steady state) for altitude x speed x sun/temperature-offset grids."));
            tf.Col("body", ColType.Str).Col("situation", ColType.Str).Col("altitude", ColType.F64, "m").Col("speed", ColType.F64, "m/s")
              .Col("sunDot", ColType.F64).Col("atmTempOffset", ColType.F64, "K", "input atmosphereTemperatureOffset (normally from GetAtmoThermalStats)")
              .Col("staticPressurekPa", ColType.F64, "kPa").Col("staticPressureAtm", ColType.F64, "atm")
              .Col("atmosphericTemperature", ColType.F64, "K", "GetFullTemperature(alt, offset)").Col("density", ColType.F64, "kg/m^3")
              .Col("dynamicPressurekPa", ColType.F64, "kPa", "0.0005*rho*v^2").Col("speedOfSound", ColType.F64, "m/s").Col("mach", ColType.F64)
              .Col("solarAirMass", ColType.F64).Col("solarFluxMultiplier", ColType.F64)
              .Col("convectiveMachLerp", ColType.F64).Col("shockTemperature", ColType.F64, "K", "CalculateShockTemperature()")
              .Col("externalTemperature", ColType.F64, "K", "max(atmTemp, shock)")
              .Col("convectiveCoefficient", ColType.F64, "kW/(m^2 K)").Col("convNewtonian", ColType.F64).Col("convMach", ColType.F64)
              .Col("pseudoReynolds", ColType.F64, "kg/(m^2 s)").Col("pseudoReDragMult", ColType.F32)
              .Col("densityThermalLerp", ColType.F64).Col("backgroundRadiationTemp", ColType.F64, "K")
              .Keys("body", "situation", "altitude", "speed", "sunDot", "atmTempOffset");
            var ctorErr = new List<string>();
            foreach (var cb in KspEnv.BodiesBySpawn.Where(b => b.atmosphere && b.bodyName != "Sun"))
            {
                var fi = HostFlightIntegrator(cb, out var vessel);
                double[] alts = cb.bodyName == "Kerbin"
                    ? new[] { 0.0, 500, 1000, 2500, 5000, 7500, 10000, 15000, 20000, 25000, 30000, 35000, 40000, 45000, 50000, 55000, 60000, 65000, 69000, 69999, 70000 }
                    : Enumerable.Range(0, 11).Select(i => cb.atmosphereDepth * i / 10.0).Concat(new[] { cb.atmosphereDepth - 1 }).ToArray();
                double[] speeds = { 0, 10, 50, 100, 200, 300, 340, 400, 600, 1000, 1500, 2000, 2500, 3000, 4000, 6000 };
                var sunGrid = cb.bodyName == "Kerbin" ? new[] { (1.0, 0.0), (0.0, 0.0), (-1.0, 0.0), (0.5, 15.0) } : new[] { (1.0, 0.0) };
                var sits = cb.bodyName == "Kerbin" ? new[] { Vessel.Situations.FLYING, Vessel.Situations.SPLASHED } : new[] { Vessel.Situations.FLYING };
                foreach (var sit in sits)
                    foreach (var h in alts)
                    {
                        if (sit == Vessel.Situations.SPLASHED && h > 0) continue;
                        foreach (var v in speeds)
                            foreach (var (sunDot, off) in sunGrid)
                            {
                                vessel.situation = sit;
                                fi.altitude = h; fi.spd = v; fi.sunDot = sunDot;
                                U.Set(fi, "atmosphereTemperatureOffset", off);
                                U.Set(fi, "solarFlux", 1360.0);
                                fi.density = 0;
                                U.Call(fi, "CalculatePressure");
                                U.Call(fi, "CalculateConstantsAtmosphere");
                                U.Call(fi, "CalculatePressure");
                                U.Call(fi, "CalculateConstantsAtmosphere");
                                double shock = fi.CalculateShockTemperature();
                                double cn = (double)U.Call(fi, "CalculateConvectiveCoefficientNewtonian");
                                double cmach = (double)U.Call(fi, "CalculateConvectiveCoefficientMach");
                                double dtl = fi.CalculateDensityThermalLerp();
                                U.Set(fi, "densityThermalLerp", dtl);
                                double bgT = (double)U.CallSig(fi, new[] { typeof(double) }, "CalculateBackgroundRadiationTemperature", fi.atmosphericTemperature);
                                tf.Add(cb.bodyName, sit.ToString(), h, v, sunDot, off,
                                    U.Get<double>(fi, "staticPressurekPa"), U.Get<double>(fi, "staticPressureAtm"), fi.atmosphericTemperature, fi.density,
                                    U.Get<double>(fi, "dynamicPressurekPa"), vessel.speedOfSound, fi.mach,
                                    U.Get<double>(fi, "solarAirMass"), U.Get<double>(fi, "solarFluxMultiplier"),
                                    fi.convectiveMachLerp, shock, fi.externalTemperature, fi.convectiveCoefficient, cn, cmach,
                                    U.Get<double>(fi, "pseudoReynolds"), (float)U.Get<double>(fi, "pseudoReDragMult"), dtl, bgT);
                            }
                    }
            }

            var ts = g.AddTable(new Table("solar_effects", "CelestialBody.GetSolarAtmosphericEffects(sunDot, density) and GetSolarPowerFactor(density) on Kerbin."));
            ts.Col("body", ColType.Str).Col("sunDot", ColType.F64).Col("density", ColType.F64, "kg/m^3")
              .Col("solarAirMass", ColType.F64).Col("solarFluxMultiplier", ColType.F64).Col("solarPowerFactor", ColType.F64).Keys("body", "sunDot", "density");
            foreach (var name in new[] { "Kerbin", "Eve", "Duna", "Laythe", "Jool" })
            {
                var cb = KspEnv.Body[name];
                foreach (var sd in new[] { -1.0, -0.2, 0.0, 0.05, 0.2, 0.5, 0.866, 1.0 })
                    foreach (var rho in new[] { 0.0, 1e-6, 1e-3, 0.01, 0.1, 0.5, 1.225, 5.0 })
                    {
                        cb.GetSolarAtmosphericEffects(sd, rho, out double am, out double fm);
                        ts.Add(name, sd, rho, am, fm, cb.GetSolarPowerFactor(rho));
                    }
            }
            yield return ("atmosphere_curves/atmosphere_curves.json", g);
        }
    }
}
