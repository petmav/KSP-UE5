using System;
using System.Collections.Generic;
using System.Linq;
using KspGolden.Infra;

namespace KspGolden.Generators
{
    /// <summary>Dataset 1: celestial body catalog as KSP holds it after load (UT = 0).</summary>
    public static class BodiesGen
    {
        public static IEnumerable<(string, GoldenFile)> Generate()
        {
            var g = new GoldenFile("body_catalog", "direct+rehosted",
                "Every body of the stock Kerbol system as KSP holds it at UT=0 after PSystemManager spawn, CelestialBody Awake/Start, " +
                "OrbitDriver.Start and the first Planetarium.FixedUpdate/CBUpdate. Serialized inputs come from the PSystem prefab in " +
                "sharedassets9.assets (see golden/serialized/celestial_bodies.serialized.json). Derived quantities: Mass/gravParameter/" +
                "gMagnitudeAtCenter/SOI/hillSphere/solar-period conversion/rotation values are RE-HOSTED expressions of Awake/Start/CBUpdate " +
                "(Rehosted/CelestialBodyLifecycle.cs); atmosphere constants come from the real CelestialBody.SetupConstants(); orbit " +
                "quantities from the real Orbit.Init()/UpdateFromUT(0); atmosphere profiles from the real GetPressure/GetTemperature/" +
                "GetDensity/GetSpeedOfSound.");
            g.Oracle("CelestialBody.SetupConstants()").Oracle("CelestialBody.GetPressure(double)").Oracle("CelestialBody.GetPressureAtm(double)")
             .Oracle("CelestialBody.GetTemperature(double)").Oracle("CelestialBody.GetDensity(double,double)").Oracle("CelestialBody.GetSpeedOfSound(double,double)")
             .Oracle("Orbit.Init()").Oracle("Orbit.UpdateFromUT(double)")
             .Oracle("rehosted: CelestialBody.Awake/Start/CBUpdate numeric statements (CelestialBody.cs:456-590)");

            var t = g.AddTable(new Table("bodies", "Physical and rotational parameters per body (in-game state at UT=0)."));
            t.Col("name", ColType.Str).Col("flightGlobalsIndex", ColType.I64).Col("parent", ColType.Str).Col("spawnOrder", ColType.I64)
             .Col("Radius", ColType.F64, "m").Col("GeeASL", ColType.F64, "g0").Col("Mass", ColType.F64, "kg").Col("gravParameter", ColType.F64, "m^3/s^2")
             .Col("gMagnitudeAtCenter", ColType.F64, "m^3/s^2", "CBUpdate form GeeASL*g0*R*R")
             .Col("Density", ColType.F64, "kg/m^3").Col("SurfaceArea", ColType.F64, "m^2")
             .Col("sphereOfInfluence", ColType.F64, "m").Col("hillSphere", ColType.F64, "m")
             .Col("rotates", ColType.Bool).Col("tidallyLocked", ColType.Bool).Col("solarRotationPeriod", ColType.Bool)
             .Col("rotationPeriod_serialized", ColType.F64, "s").Col("rotationPeriod", ColType.F64, "s", "after Start (solar->sidereal) and CBUpdate (tidal lock)")
             .Col("rotPeriodRecip", ColType.F64, "1/s").Col("angularV", ColType.F64, "rad/s").Col("initialRotation", ColType.F64, "deg")
             .Col("rotationAngle_UT0", ColType.F64, "deg").Col("solarDayLength", ColType.F64, "s")
             .Col("inverseRotThresholdAltitude", ColType.F32, "m").Col("hasSolidSurface", ColType.Bool).Col("isStar", ColType.Bool).Col("isHomeWorld", ColType.Bool)
             .Col("ocean", ColType.Bool).Col("oceanDensity", ColType.F64, "t/m^3")
             .Col("atmosphere", ColType.Bool).Col("atmosphereContainsOxygen", ColType.Bool).Col("atmosphereDepth", ColType.F64, "m")
             .Col("atmosphereTemperatureSeaLevel", ColType.F64, "K").Col("atmospherePressureSeaLevel", ColType.F64, "kPa")
             .Col("atmosphereMolarMass", ColType.F64, "kg/mol").Col("atmosphereAdiabaticIndex", ColType.F64)
             .Col("atmosphereTemperatureLapseRate", ColType.F64, "K/m").Col("atmosphereGasMassLapseRate", ColType.F64)
             .Col("atmosphereUsePressureCurve", ColType.Bool).Col("atmospherePressureCurveIsNormalized", ColType.Bool)
             .Col("atmosphereUseTemperatureCurve", ColType.Bool).Col("atmosphereTemperatureCurveIsNormalized", ColType.Bool)
             .Col("radiusAtmoFactor", ColType.F64).Col("atmDensityASL", ColType.F64, "kg/m^3").Col("atmPressureASL", ColType.F64, "atm")
             .Col("albedo", ColType.F64).Col("emissivity", ColType.F64).Col("coreTemperatureOffset", ColType.F64, "K")
             .Col("convectionMultiplier", ColType.F64).Col("shockTemperatureMultiplier", ColType.F64).Col("maxAxialDot", ColType.F64)
             .Col("timeWarpAltitudeLimits", ColType.Str, "m", "space-separated float32 list as serialized")
             .Keys("name");

            foreach (var cb in KspEnv.BodiesBySpawn)
            {
                var ser = KspEnv.SerializedBody[cb];
                string parent = cb.orbitDriver != null ? cb.orbit.referenceBody.bodyName : "";
                t.Add(cb.bodyName, cb.flightGlobalsIndex, parent, KspEnv.BodiesBySpawn.IndexOf(cb),
                    cb.Radius, cb.GeeASL, cb.Mass, cb.gravParameter, cb.gMagnitudeAtCenter, cb.Density, cb.SurfaceArea,
                    cb.sphereOfInfluence, cb.hillSphere, cb.rotates, cb.tidallyLocked, cb.solarRotationPeriod,
                    ser.Num("rotationPeriod"), cb.rotationPeriod, cb.rotPeriodRecip, cb.angularV, cb.initialRotation, cb.rotationAngle, cb.solarDayLength,
                    cb.inverseRotThresholdAltitude, cb.hasSolidSurface, cb.isStar, cb.isHomeWorld, cb.ocean, cb.oceanDensity,
                    cb.atmosphere, cb.atmosphereContainsOxygen, cb.atmosphereDepth, cb.atmosphereTemperatureSeaLevel, cb.atmospherePressureSeaLevel,
                    cb.atmosphereMolarMass, cb.atmosphereAdiabaticIndex, cb.atmosphereTemperatureLapseRate, cb.atmosphereGasMassLapseRate,
                    cb.atmosphereUsePressureCurve, cb.atmospherePressureCurveIsNormalized, cb.atmosphereUseTemperatureCurve, cb.atmosphereTemperatureCurveIsNormalized,
                    cb.radiusAtmoFactor, cb.atmDensityASL, cb.atmPressureASL, cb.albedo, cb.emissivity, cb.coreTemperatureOffset,
                    cb.convectionMultiplier, cb.shockTemperatureMultiplier, cb.maxAxialDot,
                    string.Join(" ", (cb.timeWarpAltitudeLimits ?? new float[0]).Select(ExactNumber.FormatFloat)));
            }

            var o = g.AddTable(new Table("orbits", "Orbital elements relative to the parent body (KSP inertial frame, Planetarium.Zup applied) and the state at UT=0 from Orbit.UpdateFromUT(0)."));
            o.Col("name", ColType.Str).Col("parent", ColType.Str).Col("reverse", ColType.Bool)
             .Col("inclination", ColType.F64, "deg").Col("eccentricity", ColType.F64).Col("semiMajorAxis", ColType.F64, "m")
             .Col("LAN", ColType.F64, "deg").Col("argumentOfPeriapsis", ColType.F64, "deg").Col("meanAnomalyAtEpoch", ColType.F64, "rad").Col("epoch", ColType.F64, "s")
             .Col("meanMotion", ColType.F64, "rad/s").Col("period", ColType.F64, "s").Col("PeR", ColType.F64, "m").Col("ApR", ColType.F64, "m")
             .Col("ObT_UT0", ColType.F64, "s").Col("meanAnomaly_UT0", ColType.F64, "rad").Col("eccentricAnomaly_UT0", ColType.F64, "rad").Col("trueAnomaly_UT0", ColType.F64, "rad")
             .Col("radius_UT0", ColType.F64, "m").Col("orbitalSpeed_UT0", ColType.F64, "m/s")
             .Col("pos_x", ColType.F64, "m", "Orbit.pos (KSP z-up 'zup' frame)").Col("pos_y", ColType.F64, "m").Col("pos_z", ColType.F64, "m")
             .Col("vel_x", ColType.F64, "m/s").Col("vel_y", ColType.F64, "m/s").Col("vel_z", ColType.F64, "m/s")
             .Keys("name");
            foreach (var cb in KspEnv.BodiesBySpawn.Where(b => b.orbitDriver != null))
            {
                var ob = cb.orbit;
                o.Add(cb.bodyName, ob.referenceBody.bodyName, cb.orbitDriver.reverse, ob.inclination, ob.eccentricity, ob.semiMajorAxis, ob.LAN,
                    ob.argumentOfPeriapsis, ob.meanAnomalyAtEpoch, ob.epoch, ob.meanMotion, ob.period, ob.PeR, ob.ApR,
                    ob.ObT, ob.meanAnomaly, ob.eccentricAnomaly, ob.trueAnomaly, ob.radius, ob.orbitalSpeed,
                    ob.pos.x, ob.pos.y, ob.pos.z, ob.vel.x, ob.vel.y, ob.vel.z);
            }

            var a = g.AddTable(new Table("atmosphere_profile_1km", "Real CelestialBody atmosphere functions at 1 km steps from 0 to atmosphereDepth (inclusive), plus depth+1 m."));
            a.Col("name", ColType.Str).Col("altitude", ColType.F64, "m")
             .Col("pressure_kPa", ColType.F64, "kPa", "GetPressure(alt)").Col("pressure_atm", ColType.F64, "atm", "GetPressureAtm(alt)")
             .Col("temperature", ColType.F64, "K", "GetTemperature(alt) (no latitude/sun offset)")
             .Col("density", ColType.F64, "kg/m^3", "GetDensity(GetPressure, GetTemperature)")
             .Col("speedOfSound", ColType.F64, "m/s", "GetSpeedOfSound(GetPressure, density)")
             .Keys("name", "altitude");
            foreach (var cb in KspEnv.BodiesBySpawn.Where(b => b.atmosphere))
            {
                var alts = new List<double>();
                for (double h = 0; h <= cb.atmosphereDepth; h += 1000.0) alts.Add(h);
                if (alts[alts.Count - 1] != cb.atmosphereDepth) alts.Add(cb.atmosphereDepth);
                alts.Add(cb.atmosphereDepth + 1.0);
                foreach (var h in alts)
                {
                    double p = cb.GetPressure(h), T = cb.GetTemperature(h), rho = cb.GetDensity(p, T);
                    a.Add(cb.bodyName, h, p, cb.GetPressureAtm(h), T, rho, cb.GetSpeedOfSound(p, rho));
                }
            }

            var c = g.AddTable(new Table("body_curves", "Keyframes of every FloatCurve on each CelestialBody exactly as serialized (float32). Wrap modes are Unity-native values (2 = clamp)."));
            c.Col("name", ColType.Str).Col("curve", ColType.Str).Col("index", ColType.I64)
             .Col("time", ColType.F32).Col("value", ColType.F32).Col("inTangent", ColType.F32).Col("outTangent", ColType.F32)
             .Col("weightedMode", ColType.I64).Col("preInfinity", ColType.I64).Col("postInfinity", ColType.I64)
             .Keys("name", "curve", "index");
            foreach (var cb in KspEnv.BodiesBySpawn)
            {
                foreach (var f in typeof(CelestialBody).GetFields().Where(f => f.FieldType == typeof(FloatCurve)).OrderBy(f => f.MetadataToken))
                {
                    var fc = (FloatCurve)f.GetValue(cb);
                    var ac = fc.Curve;
                    var keys = ac.keys;
                    for (int i = 0; i < keys.Length; i++)
                        c.Add(cb.bodyName, f.Name, i, keys[i].time, keys[i].value, keys[i].inTangent, keys[i].outTangent, (int)keys[i].weightedMode, ac.NativePreInfinity, ac.NativePostInfinity);
                }
            }
            yield return ("body_catalog/body_catalog.json", g);
        }
    }
}
