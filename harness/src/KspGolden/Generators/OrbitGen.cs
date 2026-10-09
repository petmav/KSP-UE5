using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using KspGolden.Infra;

namespace KspGolden.Generators
{
    /// <summary>Dataset 2: orbit math through KSP's Orbit class (and PatchedConics for SOI transitions).</summary>
    public static class OrbitGen
    {
        sealed class Case
        {
            public string Id, Body, Note;
            public double Inc, E, Sma, Lan, ArgPe, MEp, Epoch;
        }

        static List<Case> Cases()
        {
            var cs = new List<Case>();
            void Add(string id, string body, double inc, double e, double sma, double lan, double argPe, double mEp, double epoch, string note)
                => cs.Add(new Case { Id = id, Body = body, Inc = inc, E = e, Sma = sma, Lan = lan, ArgPe = argPe, MEp = mEp, Epoch = epoch, Note = note });
            foreach (var (mEp, ep, tag) in new[] { (0.0, 0.0, "a"), (2.5, 12345.6, "b") })
            {
                Add("kerbin_leo_circ_equ_" + tag, "Kerbin", 0, 0, 680000, 0, 0, mEp, ep, "circular equatorial (e=0: argPe from an; inc=0: an=right)");
                Add("kerbin_circ_inc28_" + tag, "Kerbin", 28.5, 0, 700000, 77.7, 0, mEp, ep, "circular inclined");
                Add("kerbin_polar_" + tag, "Kerbin", 90, 0.001, 750000, 120, 45, mEp, ep, "polar, near-circular");
                Add("kerbin_retro_" + tag, "Kerbin", 135, 0.05, 1.0e6, 300, 200, mEp, ep, "retrograde");
                Add("kerbin_inc180_" + tag, "Kerbin", 180, 0.02, 9.0e5, 0, 30, mEp, ep, "inclination 180");
                Add("kerbin_gto_" + tag, "Kerbin", 5, 0.5, 6.0e6, 10, 270, mEp, ep, "elliptical, Std Kepler solver (e<0.8)");
                Add("kerbin_e079_" + tag, "Kerbin", 3, 0.79, 3.0e6, 45, 90, mEp, ep, "just below the 0.8 solver switch");
                Add("kerbin_e081_" + tag, "Kerbin", 3, 0.81, 3.0e6, 45, 90, mEp, ep, "just above: extreme-eccentricity solver (8 fixed iterations)");
                Add("kerbin_mun_transfer_" + tag, "Kerbin", 0, 0.8897637795275591, 6.35e6, 0, 0, mEp, ep, "Pe 700 km / Ap 12000 km");
                Add("kerbin_e099_" + tag, "Kerbin", 12, 0.99, 4.0e7, 200, 10, mEp, ep, "e=0.99, Ap inside SOI");
                Add("kerbin_hyp12_" + tag, "Kerbin", 10, 1.2, -4.0e6, 30, 60, mEp, ep, "hyperbolic (sma<0)");
                Add("kerbin_hyp3_" + tag, "Kerbin", 100, 3.0, -5.0e5, 250, 300, mEp, ep, "strongly hyperbolic");
                Add("mun_low_circ_" + tag, "Mun", 0, 0, 230000, 0, 0, mEp, ep, "Mun 30 km circular");
                Add("mun_incl_ell_" + tag, "Mun", 45, 0.3, 500000, 15, 75, mEp, ep, "Mun inclined elliptical");
                Add("sun_transfer_" + tag, "Sun", 1.0, 0.2, 1.7e10, 0, 0, mEp, ep, "Kerbol, Kerbin-Duna-like transfer");
                Add("sun_hyp_" + tag, "Sun", 7, 1.05, -1.0e11, 33, 44, mEp, ep, "Kerbol hyperbolic");
            }
            // deterministic pseudo-random coverage (System.Random(20261008)); inputs are recorded so the C++ side reads them
            var rnd = new Random(20261008);
            string[] bodies = { "Kerbin", "Mun", "Sun", "Jool", "Eve", "Minmus" };
            for (int i = 0; i < 60; i++)
            {
                string b = bodies[i % bodies.Length];
                var cb = KspEnv.Body[b];
                double e = new[] { 0.0, rnd.NextDouble() * 0.8, 0.8 + rnd.NextDouble() * 0.19, 1.0 + rnd.NextDouble() * 4 }[i % 4];
                double pe = cb.Radius * (1.05 + rnd.NextDouble() * 20);
                double sma = e < 1 ? pe / (1 - e) : pe / (1 - e); // negative for e>1
                Add($"rand_{i:00}", b, rnd.NextDouble() * 180, e, sma, rnd.NextDouble() * 360, rnd.NextDouble() * 360,
                    (rnd.NextDouble() * 2 - 1) * Math.PI, Math.Round(rnd.NextDouble() * 1e6, 3), "pseudo-random");
            }
            return cs;
        }

        static double[] Times(Orbit o, Case c)
        {
            var ts = new List<double>();
            if (o.eccentricity < 1)
            {
                foreach (var f in new[] { 0.0, 0.1234, 0.25, 0.5, 0.75, 0.999, 1.0, 3.7, 1000.25 }) ts.Add(c.Epoch + f * o.period);
                ts.Add(c.Epoch - 0.3 * o.period);
                ts.Add(1.0e7); ts.Add(3.0e8);
            }
            else
            {
                double n = o.meanMotion;
                foreach (var k in new[] { -5.0, -1.0, -0.1, 0.0, 0.1, 1.0, 5.0, 50.0 }) ts.Add(c.Epoch + k / n);
            }
            return ts.ToArray();
        }

        public static IEnumerable<(string, GoldenFile)> Generate()
        {
            var g = new GoldenFile("orbit_propagation", "direct",
                "Orbit math evaluated by KSP's Orbit class (direct calls on the shipped-IL-equivalent assembly). Frame: KSP 'zup' local " +
                "frame returned by getRelativePositionAtUT / getOrbitalVelocityAtUT (Planetarium.Zup.WorldToLocal applied; Zup = " +
                "PlanetaryFrame(0,90,0) = identity up to ~6e-17 terms). Angles: inclination/LAN/argPe in degrees, anomalies in radians. " +
                "Central bodies use in-game mu (see bodies dataset).");
            g.Oracle("new Orbit(inc,e,sma,lan,argPe,mEp,epoch,body) -> Orbit.SetOrbit/Init").Oracle("Orbit.getObtAtUT").Oracle("Orbit.getMeanAnomalyAtUT")
             .Oracle("Orbit.EccentricAnomalyAtUT").Oracle("Orbit.TrueAnomalyAtUT").Oracle("Orbit.getRelativePositionAtUT").Oracle("Orbit.getOrbitalVelocityAtUT")
             .Oracle("Orbit.GetOrbitalStateVectorsAtUT(UT,out pos,out vel)").Oracle("Orbit.UpdateFromUT").Oracle("Orbit.GetTimeToPeriapsis")
             .Oracle("Orbit.GetNextPeriapsisTime").Oracle("Orbit.GetNextApoapsisTime").Oracle("Orbit.UpdateFromStateVectors").Oracle("Orbit.solveEccentricAnomaly")
             .Oracle("Orbit.GetTrueAnomaly").Oracle("Orbit.GetEccentricAnomaly").Oracle("Orbit.GetMeanAnomaly");

            var cases = Cases();
            var tc = g.AddTable(new Table("cases", "Input orbital elements (Orbit constructor arguments) and Init() outputs."));
            tc.Col("case_id", ColType.Str).Col("body", ColType.Str).Col("mu", ColType.F64, "m^3/s^2").Col("inclination", ColType.F64, "deg").Col("eccentricity", ColType.F64)
              .Col("semiMajorAxis", ColType.F64, "m").Col("LAN", ColType.F64, "deg").Col("argumentOfPeriapsis", ColType.F64, "deg")
              .Col("meanAnomalyAtEpoch", ColType.F64, "rad").Col("epoch", ColType.F64, "s")
              .Col("meanMotion", ColType.F64, "rad/s").Col("period", ColType.F64, "s").Col("ObTAtEpoch", ColType.F64, "s")
              .Col("h_x", ColType.F64, "m^2/s").Col("h_y", ColType.F64).Col("h_z", ColType.F64)
              .Col("eccVec_x", ColType.F64).Col("eccVec_y", ColType.F64).Col("eccVec_z", ColType.F64)
              .Col("an_x", ColType.F64).Col("an_y", ColType.F64).Col("an_z", ColType.F64)
              .Col("note", ColType.Str).Keys("case_id");
            var ts = g.AddTable(new Table("state_at_ut", "Forward propagation at UT."));
            ts.Col("case_id", ColType.Str).Col("UT", ColType.F64, "s").Col("ObT", ColType.F64, "s").Col("meanAnomaly", ColType.F64, "rad", "getMeanAnomalyAtUT (wrapped to [0,2pi) when e<1)")
              .Col("eccentricAnomaly", ColType.F64, "rad").Col("trueAnomaly", ColType.F64, "rad")
              .Col("pos_x", ColType.F64, "m", "getRelativePositionAtUT").Col("pos_y", ColType.F64, "m").Col("pos_z", ColType.F64, "m")
              .Col("vel_x", ColType.F64, "m/s", "getOrbitalVelocityAtUT").Col("vel_y", ColType.F64, "m/s").Col("vel_z", ColType.F64, "m/s")
              .Col("sv_radius", ColType.F64, "m", "GetOrbitalStateVectorsAtUT return").Col("sv_pos_x", ColType.F64, "m").Col("sv_pos_y", ColType.F64, "m").Col("sv_pos_z", ColType.F64, "m")
              .Col("sv_vel_x", ColType.F64, "m/s").Col("sv_vel_y", ColType.F64, "m/s").Col("sv_vel_z", ColType.F64, "m/s")
              .Col("u_radius", ColType.F64, "m", "after UpdateFromUT(UT)").Col("u_orbitalSpeed", ColType.F64, "m/s").Col("u_altitude", ColType.F64, "m")
              .Col("u_orbitalEnergy", ColType.F64, "J/kg").Col("u_timeToPe", ColType.F64, "s").Col("u_timeToAp", ColType.F64, "s").Col("u_orbitPercent", ColType.F64)
              .Col("GetTimeToPeriapsis_final", ColType.F64, "s", "patchEndTransition=FINAL").Col("GetTimeToPeriapsis_default", ColType.F64, "s", "fresh Orbit: patchEndTransition=INITIAL, StartUT=EndUT=0 (KSP quirk)")
              .Col("nextPeTime", ColType.F64, "s").Col("nextApTime", ColType.F64, "s", "NaN for e>=1 (not called)")
              .Keys("case_id", "UT");
            var ti = g.AddTable(new Table("from_state_vectors", "Inverse: new Orbit().UpdateFromStateVectors(getRelativePositionAtUT(UT), getOrbitalVelocityAtUT(UT), body, UT)."));
            ti.Col("case_id", ColType.Str).Col("UT", ColType.F64, "s")
              .Col("inclination", ColType.F64, "deg").Col("eccentricity", ColType.F64).Col("semiMajorAxis", ColType.F64, "m").Col("LAN", ColType.F64, "deg")
              .Col("argumentOfPeriapsis", ColType.F64, "deg").Col("meanAnomalyAtEpoch", ColType.F64, "rad").Col("epoch", ColType.F64, "s")
              .Col("trueAnomaly", ColType.F64, "rad").Col("eccentricAnomaly", ColType.F64, "rad").Col("meanAnomaly", ColType.F64, "rad")
              .Col("period", ColType.F64, "s").Col("ObT", ColType.F64, "s").Col("timeToPe", ColType.F64, "s").Col("timeToAp", ColType.F64, "s").Col("orbitalEnergy", ColType.F64, "J/kg")
              .Keys("case_id", "UT");

            foreach (var c in cases)
            {
                var body = KspEnv.Body[c.Body];
                var o = new Orbit(c.Inc, c.E, c.Sma, c.Lan, c.ArgPe, c.MEp, c.Epoch, body);
                tc.Add(c.Id, c.Body, body.gravParameter, c.Inc, c.E, c.Sma, c.Lan, c.ArgPe, c.MEp, c.Epoch, o.meanMotion, o.period, o.ObTAtEpoch,
                    o.h.x, o.h.y, o.h.z, o.eccVec.x, o.eccVec.y, o.eccVec.z, o.an.x, o.an.y, o.an.z, c.Note);
                foreach (var ut in Times(o, c))
                {
                    var fresh = new Orbit(c.Inc, c.E, c.Sma, c.Lan, c.ArgPe, c.MEp, c.Epoch, body);
                    double obt = fresh.getObtAtUT(ut);
                    double M = fresh.getMeanAnomalyAtUT(ut);
                    double E = fresh.EccentricAnomalyAtUT(ut);
                    double tA = fresh.TrueAnomalyAtUT(ut);
                    var p = fresh.getRelativePositionAtUT(ut);
                    var v = fresh.getOrbitalVelocityAtUT(ut);
                    double r = fresh.GetOrbitalStateVectorsAtUT(ut, out Vector3d sp, out Vector3d sv);
                    double ttpDefault = fresh.GetTimeToPeriapsis();
                    fresh.UpdateFromUT(ut);
                    double ttpDefault2 = fresh.GetTimeToPeriapsis();
                    fresh.patchEndTransition = Orbit.PatchTransitionType.FINAL;
                    double ttpFinal = fresh.GetTimeToPeriapsis();
                    double nextPe = fresh.GetNextPeriapsisTime(ut);
                    double nextAp = fresh.eccentricity < 1 ? fresh.GetNextApoapsisTime(ut) : double.NaN;
                    ts.Add(c.Id, ut, obt, M, E, tA, p.x, p.y, p.z, v.x, v.y, v.z, r, sp.x, sp.y, sp.z, sv.x, sv.y, sv.z,
                        fresh.radius, fresh.orbitalSpeed, fresh.altitude, fresh.orbitalEnergy, fresh.timeToPe, fresh.timeToAp, fresh.orbitPercent,
                        ttpFinal, ttpDefault2, nextPe, nextAp);
                    var inv = new Orbit();
                    inv.UpdateFromStateVectors(p, v, body, ut);
                    ti.Add(c.Id, ut, inv.inclination, inv.eccentricity, inv.semiMajorAxis, inv.LAN, inv.argumentOfPeriapsis, inv.meanAnomalyAtEpoch, inv.epoch,
                        inv.trueAnomaly, inv.eccentricAnomaly, inv.meanAnomaly, inv.period, inv.ObT, inv.timeToPe, inv.timeToAp, inv.orbitalEnergy);
                }
            }

            // hand-made state vectors, including degenerate ones
            var tsv = g.AddTable(new Table("state_vector_cases", "UpdateFromStateVectors on hand-made vectors (zup local frame), incl. degenerate cases."));
            tsv.Col("case_id", ColType.Str).Col("body", ColType.Str).Col("UT", ColType.F64, "s")
               .Col("pos_x", ColType.F64, "m").Col("pos_y", ColType.F64, "m").Col("pos_z", ColType.F64, "m")
               .Col("vel_x", ColType.F64, "m/s").Col("vel_y", ColType.F64, "m/s").Col("vel_z", ColType.F64, "m/s")
               .Col("inclination", ColType.F64, "deg").Col("eccentricity", ColType.F64).Col("semiMajorAxis", ColType.F64, "m").Col("LAN", ColType.F64, "deg")
               .Col("argumentOfPeriapsis", ColType.F64, "deg").Col("meanAnomalyAtEpoch", ColType.F64, "rad").Col("trueAnomaly", ColType.F64, "rad")
               .Col("period", ColType.F64, "s").Col("timeToPe", ColType.F64, "s").Col("timeToAp", ColType.F64, "s")
               .Keys("case_id");
            var kerbin = KspEnv.Body["Kerbin"];
            double vc = Math.Sqrt(kerbin.gravParameter / 700000.0);
            var svCases = new (string id, Vector3d p, Vector3d v)[]
            {
                ("circular_x", new Vector3d(700000, 0, 0), new Vector3d(0, vc, 0)),
                ("circular_y_retro", new Vector3d(0, 700000, 0), new Vector3d(vc, 0, 0)),
                ("circular_polar", new Vector3d(700000, 0, 0), new Vector3d(0, 0, vc)),
                ("elliptic_pe", new Vector3d(700000, 0, 0), new Vector3d(0, vc * 1.2, 0)),
                ("elliptic_inclined", new Vector3d(500000, 400000, 100000), new Vector3d(-1500, 1800, 400)),
                ("escape_exact", new Vector3d(700000, 0, 0), new Vector3d(0, vc * Math.Sqrt(2.0), 0)),
                ("hyperbolic", new Vector3d(700000, 0, 0), new Vector3d(0, vc * 1.6, 100)),
                ("radial_only", new Vector3d(700000, 0, 0), new Vector3d(1000, 0, 0)),
                ("zero_velocity", new Vector3d(700000, 0, 0), new Vector3d(0, 0, 0)),
            };
            foreach (var (id, p, v) in svCases)
            {
                var o = new Orbit();
                o.UpdateFromStateVectors(p, v, kerbin, 1000.0);
                tsv.Add(id, "Kerbin", 1000.0, p.x, p.y, p.z, v.x, v.y, v.z, o.inclination, o.eccentricity, o.semiMajorAxis, o.LAN, o.argumentOfPeriapsis,
                    o.meanAnomalyAtEpoch, o.trueAnomaly, o.period, o.timeToPe, o.timeToAp);
            }

            // Kepler equation solver grid
            var tk = g.AddTable(new Table("kepler", "Orbit.solveEccentricAnomaly(M, e) with Orbit.eccentricity = e (the solvers read the field), and anomaly conversions."));
            tk.Col("e", ColType.F64).Col("M", ColType.F64, "rad").Col("E", ColType.F64, "rad", "solveEccentricAnomaly(M,e) default maxError 1e-7, 8 iterations")
              .Col("trueAnomaly_of_E", ColType.F64, "rad", "GetTrueAnomaly(E)").Col("E_of_trueAnomaly", ColType.F64, "rad", "GetEccentricAnomaly(GetTrueAnomaly(E))")
              .Col("M_of_E", ColType.F64, "rad", "GetMeanAnomaly(E) (wrapped to [0,2pi) for e<1)")
              .Keys("e", "M");
            var es = new[] { 0.0, 1e-6, 0.01, 0.3, 0.7, 0.79999, 0.8, 0.85, 0.9, 0.99, 0.999, 0.9999, 1.0001, 1.01, 1.5, 5.0, 50.0 };
            foreach (var e in es)
            {
                var o = new Orbit(0, e, e < 1 ? 1.0e6 : -1.0e6, 0, 0, 0, 0, kerbin);
                var Ms = e < 1
                    ? new[] { -3.0, -1e-9, 0.0, 1e-12, 1e-6, 0.001, 0.1, 0.5, 1.0, 2.0, Math.PI - 1e-9, Math.PI, Math.PI + 1e-9, 4.0, 2 * Math.PI - 1e-12, 2 * Math.PI, 7.0, 100.0 }
                    : new[] { -100.0, -10.0, -1.0, -1e-6, 0.0, 1e-9, 1e-3, 0.5, 1.0, 3.0, 10.0, 50.0, 1000.0 };
                foreach (var M in Ms)
                {
                    double E = o.solveEccentricAnomaly(M, e);
                    double tA = o.GetTrueAnomaly(E);
                    tk.Add(e, M, E, tA, o.GetEccentricAnomaly(tA), o.GetMeanAnomaly(E));
                }
            }

            // patched conics
            var gp = new GoldenFile("patched_conics", "direct+rehosted",
                "Patched-conic sequences computed by the real PatchedConics.CalculatePatch (CheckEncounter/EncountersBody/Orbit.FindClosestPoints/" +
                "SolveClosestApproach/SolveSOI/UpdateFromOrbitAtUT) with the patch loop of PatchedConicSolver.Update re-hosted (patchLimit 6), " +
                "on the stock body set at the given start UT. Planetarium.Orbits holds the 16 body OrbitDrivers in spawn order.");
            gp.Oracle("PatchedConics.CalculatePatch").Oracle("rehosted: PatchedConicSolver.Update patch loop (PatchedConicSolver.cs:141-179)");
            var tp = gp.AddTable(new Table("patches", "Patch sequences from PatchedConics.CalculatePatch (real), iterated as PatchedConicSolver.Update does (patchLimit 6)."));
            tp.Col("case_id", ColType.Str).Col("patch", ColType.I64).Col("referenceBody", ColType.Str)
              .Col("patchStartTransition", ColType.Str).Col("patchEndTransition", ColType.Str)
              .Col("StartUT", ColType.F64, "s").Col("EndUT", ColType.F64, "s").Col("UTsoi", ColType.F64, "s")
              .Col("inclination", ColType.F64, "deg").Col("eccentricity", ColType.F64).Col("semiMajorAxis", ColType.F64, "m").Col("LAN", ColType.F64, "deg")
              .Col("argumentOfPeriapsis", ColType.F64, "deg").Col("meanAnomalyAtEpoch", ColType.F64, "rad").Col("epoch", ColType.F64, "s")
              .Col("PeR", ColType.F64, "m").Col("ApR", ColType.F64, "m")
              .Col("closestEncounterLevel", ColType.Str).Col("closestEncounterBody", ColType.Str).Col("ClAppr", ColType.F64, "m").Col("UTappr", ColType.F64, "s")
              .Keys("case_id", "patch");
            var consoleErrors = new List<string>();
            foreach (var (id, o0, ut0) in PatchCases())
            {
                var patches = SolvePatches(o0, ut0, 6, consoleErrors, id);
                for (int i = 0; i < patches.Count; i++)
                {
                    var p = patches[i];
                    tp.Add(id, i, p.referenceBody.bodyName, p.patchStartTransition.ToString(), p.patchEndTransition.ToString(), p.StartUT, p.EndUT, p.UTsoi,
                        p.inclination, p.eccentricity, p.semiMajorAxis, p.LAN, p.argumentOfPeriapsis, p.meanAnomalyAtEpoch, p.epoch, p.PeR, p.ApR,
                        p.closestEncounterLevel.ToString(), p.closestEncounterBody != null ? p.closestEncounterBody.bodyName : "", p.ClAppr, p.UTappr);
                }
            }
            gp.Prov("patched_conics_swallowed_exceptions", consoleErrors.Count);
            if (consoleErrors.Count > 0) throw new InvalidOperationException("PatchedConics swallowed exceptions: " + string.Join(" || ", consoleErrors));
            yield return ("orbit_propagation/orbit_propagation.json", g);
            yield return ("patched_conics/patched_conics.json", gp);
        }

        static IEnumerable<(string, Orbit, double)> PatchCases()
        {
            var kerbin = KspEnv.Body["Kerbin"];
            var mun = KspEnv.Body["Mun"];
            // 1. LEO, no encounter
            yield return ("kerbin_leo", new Orbit(0, 0, 700000, 0, 0, 0, 0, kerbin), 0.0);
            // 2. Hohmann-like Kerbin -> Mun, periapsis direction chosen so apoapsis meets the Mun (+0.05 rad lead)
            {
                double rp = 700000, ra = 12.0e6, a = (rp + ra) / 2, e = (ra - rp) / (ra + rp);
                double n = Math.Sqrt(kerbin.gravParameter / (a * a * a));
                double tHalf = Math.PI / n;
                var munOrbit = mun.orbit;
                double munAngleAtAp = munOrbit.meanAnomalyAtEpoch + munOrbit.meanMotion * tHalf; // Mun: circular, LAN=argPe=0
                double phi = (munAngleAtAp - Math.PI + 0.05) * 180 / Math.PI;
                phi = ((phi % 360) + 360) % 360;
                yield return ("kerbin_to_mun", new Orbit(0, e, a, 0, phi, 0, 0, kerbin), 0.0);
            }
            // 3. Kerbin escape (1.3x escape speed at 700 km)
            {
                double rp = 700000, vesc = Math.Sqrt(2 * kerbin.gravParameter / rp), v = 1.3 * vesc;
                double a = 1.0 / (2.0 / rp - v * v / kerbin.gravParameter); // negative
                double e = 1 - rp / a;
                yield return ("kerbin_escape", new Orbit(2, e, a, 40, 75, 0, 0, kerbin), 0.0);
            }
            // 4. Mun escape (hyperbolic at 30 km)
            {
                double rp = 230000, vesc = Math.Sqrt(2 * mun.gravParameter / rp), v = 1.2 * vesc;
                double a = 1.0 / (2.0 / rp - v * v / mun.gravParameter);
                double e = 1 - rp / a;
                yield return ("mun_escape", new Orbit(10, e, a, 0, 120, 0, 5000, mun), 5000.0);
            }
            // 5. Highly elliptical Kerbin orbit crossing the Mun and Minmus orbits (solver must test both)
            yield return ("kerbin_crossing", new Orbit(4, 0.95, 4.0e7, 70, 10, 0, 0, kerbin), 0.0);
        }

        /// <summary>PatchedConicSolver.Update loop (PatchedConicSolver.cs:141-179) around the real PatchedConics.CalculatePatch.</summary>
        static List<Orbit> SolvePatches(Orbit first, double ut, int patchLimit, List<string> errors, string id)
        {
            var pars = new PatchedConics.SolverParameters();
            var patches = new List<Orbit> { first };
            for (int i = 0; i < patchLimit + 1; i++) patches.Add(new Orbit());
            first.patchStartTransition = Orbit.PatchTransitionType.INITIAL;
            first.StartUT = ut;
            first.EndUT = first.eccentricity < 1.0 ? first.StartUT + first.period : first.period;
            int num = 0;
            bool more;
            var oldOut = Console.Out;
            var sw = new StringWriter();
            Console.SetOut(sw);
            try
            {
                do
                {
                    more = PatchedConics.CalculatePatch(patches[num], patches[num + 1], num != 0 ? patches[num].epoch : ut, pars, null);
                    num++;
                } while (more && num < patchLimit);
            }
            finally { Console.SetOut(oldOut); }
            if (sw.ToString().Length > 0) errors.Add(id + ": " + sw.ToString().Trim());
            return patches.Take(num + (more ? 1 : 0)).Where(p => p.referenceBody != null).ToList();
        }
    }
}
