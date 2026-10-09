using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using KspGolden.Infra;
using UnityEngine;

namespace KspGolden.Generators
{
    /// <summary>Dataset 6: engine Isp/thrust curves of every stock engine module and KSP-form rocket-equation values.</summary>
    public static class EnginesGen
    {
        sealed class Eng { public string Id, File, Part; public ModuleEngines M; public Part P; public ConfigNode Node; }

        static readonly double[] PressuresAtm = { 0, 0.001, 0.01, 0.05, 0.1, 0.2, 0.3, 0.5, 0.75, 1.0, 1.25, 1.5, 2, 3, 5, 7, 10, 15 };

        static List<Eng> LoadEngines(List<string> notes)
        {
            string root = Path.Combine(Run.InputsDir, "GameData", "Squad", "Parts");
            var list = new List<Eng>();
            foreach (var file in Directory.GetFiles(root, "*.cfg", SearchOption.AllDirectories).OrderBy(f => f, StringComparer.Ordinal))
            {
                var cn = ConfigNode.Load(file);
                if (cn == null) continue;
                foreach (var partNode in cn.GetNodes("PART"))
                {
                    string partName = partNode.GetValue("name");
                    int idx = 0;
                    foreach (var mn in partNode.GetNodes("MODULE"))
                    {
                        string mname = mn.GetValue("name");
                        if (mname != "ModuleEngines" && mname != "ModuleEnginesFX") continue;
                        idx++;
                        var part = AeroGen.HostPart();
                        ModuleEngines m = mname == "ModuleEnginesFX" ? (ModuleEngines)U.New<ModuleEnginesFX>() : U.New<ModuleEngines>();
                        U.Set(m, "_part", part);
                        // PartModule.ModularSetup: fields = new BaseFieldList(this); PartModule.Load -> Fields.Load(node) (KSP's own [KSPField] parser)
                        var fields = new BaseFieldList(m);
                        fields.Load(mn);
                        // IConfigNode fields (FloatCurves) are loaded from sub-nodes of the same name by PartModule.Load
                        foreach (var fname in new[] { "atmosphereCurve", "atmCurve", "velCurve", "atmCurveIsp", "velCurveIsp", "thrustCurve", "throttleIspCurve", "throttleIspCurveAtmStrength" })
                        {
                            var sub = mn.GetNode(fname);
                            if (sub == null) continue;
                            var fc = new FloatCurve();
                            fc.Load(sub);
                            U.Set(m, fname, fc);
                        }
                        // OnAwake (ModuleEngines.cs:1615): propellants = new List<Propellant>(); OnLoad (:600-620): PROPELLANT nodes -> Propellant.Load (real)
                        m.propellants = new List<Propellant>();
                        foreach (var pn in mn.GetNodes("PROPELLANT"))
                        {
                            if (!pn.HasValue("name")) continue;
                            var prop = new Propellant();
                            prop.Load(pn);
                            m.propellants.Add(prop);
                        }
                        // ModuleEngines.OnLoad (ModuleEngines.cs:636-647), re-hosted (OnLoad also needs PartResourceLibrary and model transforms):
                        if (mn.HasValue("maxThrust")) { m.maxThrust = float.Parse(mn.GetValue("maxThrust")); m.maxFuelFlow = m.maxThrust / (m.atmosphereCurve.Evaluate(0f) * m.g); }
                        if (mn.HasValue("minThrust")) { m.minThrust = float.Parse(mn.GetValue("minThrust")); m.minFuelFlow = m.minThrust / (m.atmosphereCurve.Evaluate(0f) * m.g); }
                        m.throttleMin = m.minFuelFlow / m.maxFuelFlow;
                        try { m.SetupPropellant(); } catch (Exception ex) { notes.Add(partName + ": SetupPropellant " + ex.GetType().Name); }
                        string rel = file.Substring(Run.InputsDir.Length + 1).Replace('\\', '/');
                        string eid = mn.GetValue("engineID");
                        list.Add(new Eng { Id = partName + "/" + mname + (string.IsNullOrEmpty(eid) ? "" : "[" + eid + "]") + (idx > 1 && string.IsNullOrEmpty(eid) ? "#" + idx : ""), File = rel, Part = partName, M = m, P = part, Node = mn });
                    }
                }
            }
            return list;
        }

        public static IEnumerable<(string, GoldenFile)> Generate()
        {
            var notes = new List<string>();
            KspEnv.InitResources();
            var engines = LoadEngines(notes);
            var g = new GoldenFile("engine_thrust", "direct+rehosted",
                "Every ModuleEngines/ModuleEnginesFX in the stock part cfgs. Modules are constructed with their real constructors, " +
                "[KSPField]s are parsed by KSP's BaseFieldList.Load, FloatCurves by FloatCurve.Load; maxFuelFlow/minFuelFlow/throttleMin are the " +
                "re-hosted OnLoad statements (ModuleEngines.cs:636-647). Thrust/Isp come from the real atmosphereCurve.Evaluate, " +
                "MaxThrustOutputAtm, MaxThrustOutputVac, GetEngineThrust, getExhaustVelocity. Pressures are in atm (KSP evaluates atmosphereCurve in atm). " +
                "The rocket_equation table re-hosts DeltaVStageInfo.StoreInterimDeltaV's dV expression for a single +Y engine (the stock " +
                "VesselDeltaV simulation itself needs a full ShipConstruct and is capture-spec only).");
            g.Oracle("BaseFieldList.Load(ConfigNode)").Oracle("FloatCurve.Load").Oracle("FloatCurve.Evaluate").Oracle("ModuleEngines.MaxThrustOutputAtm")
             .Oracle("ModuleEngines.MaxThrustOutputVac").Oracle("ModuleEngines.GetEngineThrust").Oracle("ModuleEngines.getExhaustVelocity")
             .Oracle("rehosted: ModuleEngines.OnLoad maxFuelFlow/minFuelFlow/throttleMin; DeltaVStageInfo.StoreInterimDeltaV dV expression");
            var te = g.AddTable(new Table("engines", "Per engine module: loaded parameters."));
            te.Col("engine_id", ColType.Str).Col("file", ColType.Str).Col("maxThrust", ColType.F32, "kN").Col("minThrust", ColType.F32, "kN")
              .Col("g", ColType.F32, "m/s^2").Col("maxFuelFlow", ColType.F32, "t/s").Col("minFuelFlow", ColType.F32, "t/s").Col("throttleMin", ColType.F32)
              .Col("isp_vac", ColType.F32, "s").Col("isp_asl", ColType.F32, "s").Col("exhaustVelocity_vac", ColType.F32, "m/s")
              .Col("MaxThrustOutputVac", ColType.F32, "kN").Col("atmChangeFlow", ColType.Bool).Col("useAtmCurve", ColType.Bool).Col("useVelCurve", ColType.Bool)
              .Col("useThrustCurve", ColType.Bool).Col("flowMultCap", ColType.F32).Col("multIsp", ColType.F32).Col("multFlow", ColType.F32)
              .Col("mixtureDensity", ColType.F32, "t/unit", "SetupPropellant (real)").Col("ratioSum", ColType.F64)
              .Col("propellant_maxFuelFlow", ColType.Str, "units/s", "name:ratio:getMaxFuelFlow(p) per propellant, float32 G9")
              .Col("atmosphereCurve_keys", ColType.Str).Keys("engine_id");
            var tp = g.AddTable(new Table("isp_thrust_vs_pressure", "atmosphereCurve.Evaluate(p) and MaxThrustOutputAtm(runningActive=false, useThrustLimiter=true, atmPressure=p, atmTemp=310, atmDensity=1.225*p) with part.machNumber=0."));
            tp.Col("engine_id", ColType.Str).Col("pressure_atm", ColType.F32, "atm").Col("isp", ColType.F32, "s").Col("MaxThrustOutputAtm", ColType.F32, "kN")
              .Col("GetEngineThrust_full", ColType.F32, "kN", "GetEngineThrust(isp(p), 1)").Col("GetEngineThrust_half", ColType.F32, "kN", "GetEngineThrust(isp(p), 0.5)")
              .Keys("engine_id", "pressure_atm");
            var tk = g.AddTable(new Table("kerbin_flight", "Engines in Kerbin atmosphere: altitude x mach (pressure/temperature/density from Kerbin's GetPressure/GetTemperature/GetDensity), MaxThrustOutputAtm with part.machNumber = mach (jets use atmCurve/velCurve)."));
            tk.Col("engine_id", ColType.Str).Col("altitude", ColType.F64, "m").Col("mach", ColType.F64).Col("pressure_atm", ColType.F32, "atm")
              .Col("density", ColType.F64, "kg/m^3").Col("isp", ColType.F32, "s").Col("MaxThrustOutputAtm", ColType.F32, "kN").Keys("engine_id", "altitude", "mach");
            var tr = g.AddTable(new Table("rocket_equation", "dV per DeltaVStageInfo.StoreInterimDeltaV (single engine, thrust vector +Y): |T_vec * (float)(ln(m0/m1) * isp * g0 / T)| with float masses."));
            tr.Col("engine_id", ColType.Str).Col("situation", ColType.Str).Col("m0", ColType.F32, "t").Col("m1", ColType.F32, "t")
              .Col("thrust", ColType.F32, "kN").Col("isp", ColType.F64, "s").Col("dV", ColType.F32, "m/s").Col("burnTime_fullThrottle", ColType.F64, "s", "(m0-m1)/maxFuelFlow")
              .Keys("engine_id", "situation", "m0", "m1");
            var kerbin = KspEnv.Body["Kerbin"];
            foreach (var e in engines)
            {
                var m = e.M;
                float ispVac = m.atmosphereCurve.Evaluate(0f), ispAsl = m.atmosphereCurve.Evaluate(1f);
                float vac = m.MaxThrustOutputVac(true);
                te.Add(e.Id, e.File, m.maxThrust, m.minThrust, m.g, m.maxFuelFlow, m.minFuelFlow, m.throttleMin, ispVac, ispAsl, m.getExhaustVelocity(ispVac), vac,
                    m.atmChangeFlow, m.useAtmCurve, m.useVelCurve, m.useThrustCurve, m.flowMultCap, m.multIsp, m.multFlow,
                    m.mixtureDensity, m.ratioSum,
                    string.Join(" ", m.propellants.Select(pp => pp.name + ":" + ExactNumber.FormatFloat(pp.ratio) + ":" + ExactNumber.FormatFloat(m.getMaxFuelFlow(pp)))),
                    string.Join(" | ", e.Node.GetNode("atmosphereCurve")?.GetValues("key") ?? new string[0]));
                e.P.machNumber = 0;
                foreach (var p in PressuresAtm)
                {
                    float pf = (float)p;
                    float isp = m.atmosphereCurve.Evaluate(pf);
                    tp.Add(e.Id, pf, isp, m.MaxThrustOutputAtm(false, true, pf, 310.0, 1.225 * p), m.GetEngineThrust(isp, 1f), m.GetEngineThrust(isp, 0.5f));
                }
                foreach (var alt in new[] { 0.0, 2000, 5000, 10000, 15000, 20000, 25000, 30000, 40000, 50000, 69000 })
                    foreach (var mach in new[] { 0.0, 0.3, 0.8, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0 })
                    {
                        double pk = kerbin.GetPressure(alt), T = kerbin.GetTemperature(alt), rho = kerbin.GetDensity(pk, T);
                        float patm = (float)(pk * 0.009869232667160128);
                        e.P.machNumber = mach;
                        tk.Add(e.Id, alt, mach, patm, rho, m.atmosphereCurve.Evaluate(patm), m.MaxThrustOutputAtm(false, true, patm, T, rho));
                    }
                e.P.machNumber = 0;
                foreach (var (sit, p) in new[] { ("vacuum", 0f), ("kerbin_asl", 1f) })
                {
                    float thrust = p == 0f ? vac : m.MaxThrustOutputAtm(false, true, p, 310.0, 1.225);
                    double isp = m.atmosphereCurve.Evaluate(p);
                    foreach (var (m0, m1) in new[] { (10f, 5f), (20f, 8f), (3.5f, 1.2f), (100f, 40f), (1.5f, 1.4999f) })
                    {
                        // DeltaVStageInfo.cs:342: Vector3 v = enginesThrust * (float)(Math.Log(startingMass / currentMass) * isp * g0 / (double)thrust)
                        var thrustVec = new Vector3(0f, thrust, 0f);
                        Vector3 v = thrustVec * (float)(Math.Log(m0 / m1) * isp * PhysicsGlobals.GravitationalAcceleration / (double)thrust);
                        tr.Add(e.Id, sit, m0, m1, thrust, isp, v.magnitude, (double)(m0 - m1) / m.maxFuelFlow);
                    }
                }
            }
            g.Prov("engine_modules", engines.Count);
            yield return ("engine_thrust/engine_thrust.json", g);
        }
    }
}
