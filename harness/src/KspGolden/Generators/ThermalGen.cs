using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using KspGolden.Infra;

namespace KspGolden.Generators
{
    /// <summary>Dataset 4: PhysicsGlobals constants (thermal, aero, buoyancy, kerbal G, ...) as loaded in game.</summary>
    public static class ThermalGen
    {
        public static IEnumerable<(string, GoldenFile)> Generate()
        {
            var g = new GoldenFile("physics_constants", "direct",
                "Every PhysicsGlobals field after the in-game load sequence: constructor -> level0 prefab values (Unity deserialization) -> " +
                "PhysicsGlobals.LoadDatabase(Physics.cfg) (real method) -> Start's bodyLiftCurve -> CalculateValues' solarLuminosity (re-hosted, " +
                "needs FlightGlobals). 'source' says which layer the final value came from. FloatCurve fields are sampled in curves_eval " +
                "(ids PhysicsGlobals.* and LiftingSurface.*).");
            g.Oracle("PhysicsGlobals.LoadDatabase()").Oracle("PhysicsGlobals static properties");
            var t = g.AddTable(new Table("physicsglobals", "One row per field (instance and static)."));
            t.Col("field", ColType.Str).Col("static", ColType.Bool).Col("type", ColType.Str).Col("value", ColType.Str, "", "exact text (float G9 / double shortest round-trip)")
             .Col("value_f64", ColType.F64, "", "numeric value widened to double (null if not numeric)").Col("source", ColType.Str).Keys("field");

            var cfg = ConfigNode.Load(KspEnv.PhysicsCfgPath);
            var cfgKeys = new HashSet<string>(cfg.values.Cast<ConfigNode.Value>().Select(v => v.name), StringComparer.Ordinal);
            var ser = (JObj)JsonReader.ParseFile(Path.Combine(Run.SerializedDir, "physicsglobals.serialized.json"));
            var serKeys = new HashSet<string>(ser.Obj("PhysicsGlobals").Items.Select(i => i.Key), StringComparer.Ordinal);
            var pg = KspEnv.Physics;
            var fields = new List<FieldInfo>();
            for (var x = typeof(PhysicsGlobals); x != typeof(UnityEngine.MonoBehaviour); x = x.BaseType)
                fields.AddRange(x.GetFields(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly));
            foreach (var f in fields.OrderBy(f => f.Name, StringComparer.Ordinal))
            {
                if (f.IsLiteral && !f.FieldType.IsPrimitive && f.FieldType != typeof(string)) continue;
                object v = f.IsLiteral ? f.GetRawConstantValue() : f.GetValue(f.IsStatic ? null : pg);
                var ft = f.FieldType;
                if (typeof(UnityEngine.Object).IsAssignableFrom(ft)) continue;
                string source = f.IsLiteral ? "const" : f.IsStatic ? "static initializer" : cfgKeys.Contains(f.Name) ? "Physics.cfg" : serKeys.Contains(f.Name) ? "prefab (level0)" : "constructor";
                if (f.Name == "solarLuminosity") source = "CalculateValues (re-hosted)";
                if (ft == typeof(FloatCurve)) { t.Add(f.Name, f.IsStatic, "FloatCurve", "see curves_eval PhysicsGlobals." + f.Name, null, cfgKeys.Contains(f.Name) ? "Physics.cfg" : source); continue; }
                if (v is IDictionary dict && f.Name == "liftingSurfaceCurves")
                {
                    t.Add(f.Name, f.IsStatic, "Dictionary<string,LiftingSurfaceCurve>", string.Join(" ", dict.Keys.Cast<string>().OrderBy(k => k, StringComparer.Ordinal)), null, "LoadDefaultLiftingSurfaceCurves + Physics.cfg LIFTING_SURFACE_CURVES");
                    continue;
                }
                if (ft == typeof(VesselRanges))
                {
                    foreach (var sf in typeof(VesselRanges).GetFields(BindingFlags.Instance | BindingFlags.Public))
                    {
                        var sit = sf.GetValue(v);
                        if (sit == null) continue;
                        foreach (var rf in sit.GetType().GetFields(BindingFlags.Instance | BindingFlags.Public))
                        {
                            var rv = rf.GetValue(sit);
                            if (rv is float fl) t.Add(f.Name + "." + sf.Name + "." + rf.Name, false, "float", ExactNumber.FormatFloat(fl), (double)fl, "Physics.cfg VesselRanges");
                        }
                    }
                    continue;
                }
                switch (v)
                {
                    case double d: t.Add(f.Name, f.IsStatic, "double", ExactNumber.FormatDouble(d), d, source); break;
                    case float fl: t.Add(f.Name, f.IsStatic, "float", ExactNumber.FormatFloat(fl), (double)fl, source); break;
                    case int i: t.Add(f.Name, f.IsStatic, "int", i.ToString(CultureInfo.InvariantCulture), (double)i, source); break;
                    case bool b: t.Add(f.Name, f.IsStatic, "bool", b ? "True" : "False", b ? 1.0 : 0.0, source); break;
                    case string s: t.Add(f.Name, f.IsStatic, "string", s, null, source); break;
                    case Enum en: t.Add(f.Name, f.IsStatic, ft.Name, en.ToString(), Convert.ToDouble(en, CultureInfo.InvariantCulture), source); break;
                    case UnityEngine.Vector3 v3: t.Add(f.Name, f.IsStatic, "Vector3", string.Join(" ", ExactNumber.FormatFloat(v3.x), ExactNumber.FormatFloat(v3.y), ExactNumber.FormatFloat(v3.z)), null, source); break;
                    default: break; // collections, delegates, Unity handles: not physics constants
                }
            }
            // public static properties that compute values
            t.Add("SolarLuminosity(property)", true, "double", ExactNumber.FormatDouble(PhysicsGlobals.SolarLuminosity), PhysicsGlobals.SolarLuminosity, "CalculateValues (re-hosted)");
            t.Add("IdealGasConstant(property)", true, "double", ExactNumber.FormatDouble(PhysicsGlobals.IdealGasConstant), PhysicsGlobals.IdealGasConstant, "static initializer");
            t.Add("StefanBoltzmanConstant(property)", true, "double", ExactNumber.FormatDouble(PhysicsGlobals.StefanBoltzmanConstant), PhysicsGlobals.StefanBoltzmanConstant, "static initializer");
            t.Add("BoltzmannConstant(property)", true, "double", ExactNumber.FormatDouble(PhysicsGlobals.BoltzmannConstant), PhysicsGlobals.BoltzmannConstant, "static initializer");
            t.Add("AvogadroConstant(property)", true, "double", ExactNumber.FormatDouble(PhysicsGlobals.AvogadroConstant), PhysicsGlobals.AvogadroConstant, "static initializer");
            yield return ("physics_constants/physics_constants.json", g);
        }
    }
}
