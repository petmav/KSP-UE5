using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using KspGolden.Infra;
using UnityEngine;

namespace KspGolden.Generators
{
    /// <summary>Dataset 3b: stock aerodynamics - drag cubes, surface drag curves, lifting surfaces, full drag chain.</summary>
    public static class AeroGen
    {
        static readonly string[] PartPicks =
        {
            "aerodynamicNoseCone/noseCone", "mk1Pod_v2/mk1pod_v2", "fuelTankT400/fuelTank", "Rockomax32/Rockomax32_BW", "connector1/wingConnector",
            "wings/delta/deltaWing", "HeatShield1/HeatShield1", "liquidEngineLV-T45_v2/liquidEngine2_v2", "liquidEngineMainsail_v2/liquidEngineMainsail_v2",
            "parachuteMk1/parachuteSingle", "Airbrake/airbrake1", "structuralPanel1x1/structuralPanel1", "bicoupler/mk2_1m_Bicoupler",
            "decouplerRadialTT-38K/radialDecoupler", "LFO_short/mk2FuselageShortLFO", "Size1_Tanks/fuelTankT800/fuelTank_long",
        };

        static Vector3 N(float x, float y, float z) => new Vector3(x, y, z).normalized;

        public static List<(string id, Vector3 dir)> Directions()
        {
            var d = new List<(string, Vector3)>
            {
                ("+X", Vector3.right), ("-X", Vector3.left), ("+Y", Vector3.up), ("-Y", Vector3.down), ("+Z", Vector3.forward), ("-Z", Vector3.back),
            };
            int[] s = { 1, -1 };
            foreach (var a in s) foreach (var b in s)
                {
                    d.Add(($"xy{a:+0;-0}{b:+0;-0}", N(a, b, 0))); d.Add(($"yz{a:+0;-0}{b:+0;-0}", N(0, a, b))); d.Add(($"xz{a:+0;-0}{b:+0;-0}", N(a, 0, b)));
                    foreach (var c in s) d.Add(($"xyz{a:+0;-0}{b:+0;-0}{c:+0;-0}", N(a, b, c)));
                }
            foreach (var aoa in new[] { 2, 5, 10, 15, 20, 30, 45, 60, 75, 89, 120, 150, 175 })
            {
                double r = aoa * Math.PI / 180.0;
                d.Add(($"aoa{aoa}", new Vector3(0f, (float)Math.Cos(r), (float)Math.Sin(r))));
            }
            d.Add(("odd1", N(0.3f, 0.9f, -0.2f))); d.Add(("odd2", N(-0.7f, 0.1f, 0.65f)));
            return d;
        }

        static readonly float[] Machs = { 0f, 0.3f, 0.6f, 0.85f, 0.95f, 1f, 1.1f, 1.5f, 2f, 3f, 5f, 10f, 25f };

        public static Part HostPart()
        {
            var p = U.Alive(U.Uninit<Part>());
            p.attachNodes = new List<AttachNode>();
            p.srfAttachNode = null;
            p.dragModel = Part.DragModel.CUBE;
            return p;
        }

        public static IEnumerable<(string, GoldenFile)> Generate()
        {
            var g = new GoldenFile("aero_forces", "direct+rehosted",
                "Stock aerodynamics. Drag cubes are the stock PartDatabase.cfg cubes loaded by DragCubeList.LoadCubes and evaluated by the real " +
                "DragCubeList.SetDragWeights / SetPartOcclusion (part without attachments) / SetDrag. 'flow_dir' is the air-relative velocity " +
                "direction in part-local coordinates (Unity left-handed, +Y = part up); SetDrag is called with -flow_dir exactly as " +
                "FlightIntegrator.UpdateAerodynamics passes dragVectorDirLocal. Drag/lift force scalars use the real FlightIntegrator.CalculateDragValue " +
                "and the re-hosted scalar statements of UpdateAerodynamics (FlightIntegrator.cs:2353-2370). Lifting surfaces use the real " +
                "ModuleLiftingSurface.GetLiftVector / GetDragVector with SetupCoefficients' vector arithmetic re-hosted (it reads a Transform).");
            g.Oracle("DragCubeList.LoadCubes(ConfigNode)").Oracle("DragCubeList.SetPart").Oracle("DragCubeList.SetCubeWeight").Oracle("DragCubeList.SetDragWeights")
             .Oracle("DragCubeList.SetPartOcclusion").Oracle("DragCubeList.SetDrag(Vector3,float) -> AddSurfaceDragDirection").Oracle("PhysicsGlobals.DragCurveValue")
             .Oracle("FlightIntegrator.CalculateDragValue(Part)").Oracle("ModuleLiftingSurface.GetLiftVector").Oracle("ModuleLiftingSurface.GetDragVector")
             .Oracle("rehosted: FlightIntegrator.UpdateAerodynamics dragScalar/bodyLiftScalar statements; ModuleLiftingSurface.SetupCoefficients");

            // ---- surface drag curve
            var tdc = g.AddTable(new Table("drag_curve_value", "PhysicsGlobals.DragCurveValue(SurfaceCurves, dotNormalized, mach) - per-face drag multiplier; dotNormalized = (dot(flow, faceNormal)+1)/2."));
            tdc.Col("dotNormalized", ColType.F32).Col("mach", ColType.F32).Col("value", ColType.F32).Keys("dotNormalized", "mach");
            var machGrid = Enumerable.Range(0, 61).Select(i => i * 0.05f).Concat(new[] { 3.5f, 4f, 5f, 7f, 10f, 15f, 20f, 25f, 30f }).ToArray();
            for (int i = 0; i <= 40; i++)
                foreach (var m in machGrid)
                {
                    float dn = i / 40f;
                    tdc.Add(dn, m, PhysicsGlobals.DragCurveValue(PhysicsGlobals.SurfaceCurves, dn, m));
                }

            // ---- drag cubes
            var db = ConfigNode.Load(Path.Combine(Run.InputsDir, "PartDatabase.cfg"));
            var partNodes = db.GetNodes("PART");
            var tcubes = g.AddTable(new Table("cube_inputs", "Drag cube definitions used (raw 'cube' strings from PartDatabase.cfg) and the weights applied."));
            tcubes.Col("case_id", ColType.Str).Col("url", ColType.Str).Col("cubes", ColType.Str).Col("weights", ColType.Str)
                  .Col("postOcclusionArea", ColType.F32, "m^2").Keys("case_id");
            var tw = g.AddTable(new Table("cube_weighted", "After SetDragWeights/SetPartOcclusion: per-face arrays (order XP,XN,YP,YN,ZP,ZN)."));
            tw.Col("case_id", ColType.Str).Col("face", ColType.I64).Col("weightedArea", ColType.F32, "m^2").Col("weightedDrag", ColType.F32)
              .Col("weightedDepth", ColType.F32, "m").Col("areaOccluded", ColType.F32, "m^2").Keys("case_id", "face");
            var td = g.AddTable(new Table("cube_drag", "DragCubeList.SetDrag(-flow_dir, mach) outputs plus FlightIntegrator drag value."));
            td.Col("case_id", ColType.Str).Col("dir_id", ColType.Str).Col("flow_x", ColType.F32).Col("flow_y", ColType.F32).Col("flow_z", ColType.F32).Col("mach", ColType.F32)
              .Col("Area", ColType.F32, "m^2").Col("AreaDrag", ColType.F32, "m^2").Col("DragCoeff", ColType.F32).Col("CrossSectionalArea", ColType.F32, "m^2")
              .Col("ExposedArea", ColType.F32, "m^2").Col("Depth", ColType.F32, "m").Col("TaperDot", ColType.F32)
              .Col("LiftForce_x", ColType.F32).Col("LiftForce_y", ColType.F32).Col("LiftForce_z", ColType.F32)
              .Col("CalculateDragValue", ColType.F64, "m^2", "AreaDrag * PhysicsGlobals.DragCubeMultiplier (float product, widened)")
              .Keys("case_id", "dir_id", "mach");

            var fi = AtmosphereGen.HostFlightIntegrator(KspEnv.Body["Kerbin"], out _);
            U.Set(fi, "cacheDragCubeMultiplier", (float)PhysicsGlobals.DragCubeMultiplier);
            U.Set(fi, "cacheDragMultiplier", (float)PhysicsGlobals.DragMultiplier);
            U.Set(fi, "cacheBodyLiftMultiplier", (float)PhysicsGlobals.BodyLiftMultiplier);
            U.Set(fi, "cacheDragCubesUseSpherical", PhysicsGlobals.DragCubesUseSpherical);
            var cases = new List<(string id, DragCubeList dcl, Part part)>();
            foreach (var pick in PartPicks)
            {
                var node = partNodes.FirstOrDefault(n => (n.GetValue("url") ?? "").EndsWith("/" + pick, StringComparison.Ordinal));
                if (node == null) { KspEnv.Notes.Add("aero: no PartDatabase entry matching " + pick); continue; }
                var cubeNode = node.GetNode("DRAG_CUBE");
                string url = node.GetValue("url");
                string partName = url.Substring(url.LastIndexOf('/') + 1);
                var probe = new DragCubeList();
                probe.LoadCubes(cubeNode);
                var names = probe.Cubes.Select(c => c.Name).ToList();
                var variants = new List<string> { null };
                if (names.Count > 1) variants.AddRange(names);
                foreach (var only in variants)
                {
                    var part = HostPart();
                    var dcl = new DragCubeList();
                    U.Set(part, "dragCubes", dcl);
                    dcl.LoadCubes(cubeNode);
                    dcl.SetPart(part);
                    if (only != null) foreach (var n in names) dcl.SetCubeWeight(n, n == only ? 1f : 0f);
                    dcl.SetDragWeights();
                    dcl.SetPartOcclusion();
                    string id = partName + (only == null ? "" : "@" + only);
                    cases.Add((id, dcl, part));
                    tcubes.Add(id, url, string.Join(" | ", cubeNode.GetValues("cube")), string.Join(" ", dcl.Cubes.Select(c => c.Name + "=" + ExactNumber.FormatFloat(c.Weight))), dcl.PostOcclusionArea);
                    for (int f = 0; f < 6; f++)
                        tw.Add(id, f, dcl.WeightedArea[f], dcl.WeightedDrag[f], dcl.WeightedDepth[f], dcl.AreaOccluded[f]);
                }
            }
            var dirs = Directions();
            foreach (var (id, dcl, part) in cases)
                foreach (var (dirId, dir) in dirs)
                    foreach (var m in Machs)
                    {
                        dcl.SetDrag(-dir, m);
                        double dv = (double)U.Call(fi, "CalculateDragValue", part);
                        var lf = dcl.LiftForce;
                        td.Add(id, dirId, dir.x, dir.y, dir.z, m, dcl.Area, dcl.AreaDrag, dcl.DragCoeff, dcl.CrossSectionalArea, dcl.ExposedArea, dcl.Depth, dcl.TaperDot,
                            lf.x, lf.y, lf.z, dv);
                    }

            // ---- full chain on Kerbin: atmosphere state -> cube drag -> force scalars
            var tchain = g.AddTable(new Table("drag_chain_kerbin", "Kerbin flight condition -> FlightIntegrator atmosphere state -> SetDrag -> UpdateAerodynamics force scalars (flow along part +Y unless noted, part.bodyLiftMultiplier = 1, not submerged)."));
            tchain.Col("case_id", ColType.Str).Col("dir_id", ColType.Str).Col("altitude", ColType.F64, "m").Col("speed", ColType.F64, "m/s")
                  .Col("mach", ColType.F64).Col("dynamicPressurekPa", ColType.F64, "kPa").Col("pseudoReDragMult", ColType.F64)
                  .Col("dragValue", ColType.F64, "m^2", "CalculateDragValue * pseudoReDragMult").Col("dragScalar", ColType.F32, "kN", "(float)(q * dragValue) * cacheDragMultiplier")
                  .Col("bodyLiftScalar", ColType.F32, "kN per unit LiftForce", "(float)q * bodyLiftMultiplier(1) * cacheBodyLiftMultiplier * BodyLift.liftMach(mach)")
                  .Col("bodyLift_x", ColType.F32, "kN").Col("bodyLift_y", ColType.F32, "kN").Col("bodyLift_z", ColType.F32, "kN")
                  .Keys("case_id", "dir_id", "altitude", "speed");
            var chainDirs = dirs.Where(d => d.id == "+Y" || d.id == "aoa10" || d.id == "aoa45" || d.id == "+Z").ToList();
            var bodyLiftMach = PhysicsGlobals.BodyLiftCurve.liftMachCurve;
            foreach (var (id, dcl, part) in cases.Where(c => !c.id.Contains("@")))
                foreach (var (dirId, dir) in chainDirs)
                    foreach (var alt in new[] { 0.0, 5000, 15000, 30000, 50000, 69000 })
                        foreach (var spd in new[] { 50.0, 250, 340, 600, 1200, 2300 })
                        {
                            fi.altitude = alt; fi.spd = spd; fi.sunDot = 1.0; fi.density = 0;
                            U.Set(fi, "atmosphereTemperatureOffset", 0.0);
                            U.Call(fi, "CalculatePressure"); U.Call(fi, "CalculateConstantsAtmosphere");
                            U.Call(fi, "CalculatePressure"); U.Call(fi, "CalculateConstantsAtmosphere");
                            double q = U.Get<double>(fi, "dynamicPressurekPa"), mach = fi.mach, pRe = U.Get<double>(fi, "pseudoReDragMult");
                            dcl.SetDrag(-dir, (float)mach);
                            // FlightIntegrator.cs:2353-2370 (UpdateAerodynamics), not submerged: num2 = 1
                            double num6 = (double)U.Call(fi, "CalculateDragValue", part) * pRe;
                            float dragScalar = (float)(q * num6 * 1.0) * U.Get<float>(fi, "cacheDragMultiplier");
                            float bodyLiftScalar = (float)q * 1f * U.Get<float>(fi, "cacheBodyLiftMultiplier") * bodyLiftMach.Evaluate((float)mach);
                            var bl = bodyLiftScalar * dcl.LiftForce;
                            tchain.Add(id, dirId, alt, spd, mach, q, pRe, num6, dragScalar, bodyLiftScalar, bl.x, bl.y, bl.z);
                        }

            // ---- lifting surfaces
            var tl = g.AddTable(new Table("lifting_surface", "ModuleLiftingSurface.GetLiftVector/GetDragVector (real) for the stock 'Default' curve set. Wing normal (liftVector) = +Z part-local; flow in the Y-Z plane at angle of attack aoa; omnidirectional; Q in Pa."));
            tl.Col("deflectionLiftCoeff", ColType.F32).Col("aoa_deg", ColType.F64).Col("mach", ColType.F32).Col("Q", ColType.F64, "Pa")
              .Col("liftDot", ColType.F32).Col("absDot", ColType.F32)
              .Col("liftScalar", ColType.F32, "kN").Col("lift_x", ColType.F32).Col("lift_y", ColType.F32).Col("lift_z", ColType.F32)
              .Col("dragScalar", ColType.F32, "kN").Col("drag_x", ColType.F32).Col("drag_y", ColType.F32).Col("drag_z", ColType.F32)
              .Keys("deflectionLiftCoeff", "aoa_deg", "mach", "Q");
            ModuleLiftingSurface mls;
            try { mls = U.New<ModuleLiftingSurface>(); } catch (Exception) { mls = U.Alive(U.Uninit<ModuleLiftingSurface>()); }
            var def = PhysicsGlobals.GetLiftingSurfaceCurve("Default");
            // OnStart (ModuleLiftingSurface.cs:155-165): liftingSurfaceCurve != "Custom" -> take the PhysicsGlobals curve set
            mls.liftCurve = def.liftCurve; mls.liftMachCurve = def.liftMachCurve; mls.dragCurve = def.dragCurve; mls.dragMachCurve = def.dragMachCurve;
            mls.omnidirectional = true; mls.nodeEnabled = false; mls.perpendicularOnly = false;
            var liftVector = new Vector3(0f, 0f, 1f);
            foreach (var coeff in new[] { 1.5f, 0.25f, 2.0f })
                foreach (var aoa in Enumerable.Range(-36, 73).Select(i => i * 5.0).Concat(new[] { 1.0, 2.0, 3.0, 7.5, 12.0, 17.0, 22.0 }))
                    foreach (var mach in new[] { 0f, 0.3f, 0.6f, 0.9f, 1.0f, 1.2f, 2f, 5f, 25f })
                    {
                        double q = 10000.0;
                        mls.deflectionLiftCoeff = coeff;
                        double r = aoa * Math.PI / 180.0;
                        var pointVelocity = new Vector3(0f, (float)Math.Cos(r), (float)-Math.Sin(r)) * 100f;
                        // SetupCoefficients (ModuleLiftingSurface.cs:205-230) with liftVector given instead of a Transform axis
                        Vector3 nVel = pointVelocity.normalized;
                        float liftDot = Vector3.Dot(nVel, liftVector);
                        float absDot = Mathf.Abs(liftDot);
                        U.Set(mls, "nVel", nVel);
                        var lift = mls.GetLiftVector(liftVector, liftDot, absDot, q, mach);
                        var drag = mls.GetDragVector(nVel, absDot, q, mach);
                        tl.Add(coeff, aoa, mach, q, liftDot, absDot, mls.liftScalar, lift.x, lift.y, lift.z, mls.dragScalar, drag.x, drag.y, drag.z);
                    }
            yield return ("aero_forces/aero_forces.json", g);
        }
    }
}
