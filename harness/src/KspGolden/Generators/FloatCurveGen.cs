using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using KspGolden.Infra;
using UnityEngine;

namespace KspGolden.Generators
{
    /// <summary>
    /// Dataset 7: FloatCurve. Keys as produced by KSP's FloatCurve.Load (including Unity's auto-tangents for
    /// 2-value keys) and dense evaluations of FloatCurve.Evaluate for every curve found in PhysicsGlobals,
    /// the celestial bodies, every stock part cfg, plus synthetic edge cases.
    /// </summary>
    public static class FloatCurveGen
    {
        public sealed class CurveSource
        {
            public string Id, Origin, Path;
            public FloatCurve Curve;
            public ConfigNode Node; // null for non-cfg sources
        }

        public static IEnumerable<(string, GoldenFile)> Generate()
        {
            var sources = new List<CurveSource>();
            sources.AddRange(PhysicsGlobalsCurves());
            sources.AddRange(BodyCurves());
            var cfgErrors = new List<string>();
            sources.AddRange(PartCfgCurves(cfgErrors));
            var synth = Synthetic();
            sources.AddRange(synth);

            var g = new GoldenFile("curves_eval", "direct+rehosted-native",
                "FloatCurve semantics. FloatCurve.Load/Add/Evaluate are KSP's own code (direct); the UnityEngine.AnimationCurve " +
                "underneath is the harness's managed port of Unity 2019.4.18f1's native AnimationCurveTpl<float> (KspUnityShim), " +
                "transcribed op-for-op from UnityPlayer.dll disassembly with Unity's public PDB symbols (re-hosted native; not executed natively). " +
                "Values are float32. Sample times include every key time, the float32 neighbours of every key time, segment midpoints and a " +
                "uniform grid extending 10% beyond the key range on both sides.");
            g.Oracle("FloatCurve.Load(ConfigNode)").Oracle("FloatCurve.Add(float,float)").Oracle("FloatCurve.Add(float,float,float,float)").Oracle("FloatCurve.Evaluate(float)")
             .Oracle("rehosted-native: AnimationCurveTpl<float>::Evaluate / CalculateCacheData / FindIndexForSampling / AddKey / AddKeySmoothTangents / RecalculateSplineSlopeT (UnityPlayer.dll 2019.4.18f1)");
            g.Prov("part_cfg_parse_errors", cfgErrors.Count);

            var tc = g.AddTable(new Table("curves", "One row per curve."));
            tc.Col("curve_id", ColType.Str).Col("origin", ColType.Str).Col("path", ColType.Str).Col("key_count", ColType.I64)
              .Col("minTime", ColType.F32).Col("maxTime", ColType.F32).Col("cfg_keys", ColType.Str, "", "raw 'key' values joined by ' | ' (cfg sources)")
              .Keys("curve_id");
            var tk = g.AddTable(new Table("keys", "Keyframes after loading (FloatCurve.Curve.keys)."));
            tk.Col("curve_id", ColType.Str).Col("index", ColType.I64).Col("time", ColType.F32).Col("value", ColType.F32)
              .Col("inTangent", ColType.F32).Col("outTangent", ColType.F32).Keys("curve_id", "index");
            var ts = g.AddTable(new Table("samples", "FloatCurve.Evaluate(t)."));
            ts.Col("curve_id", ColType.Str).Col("i", ColType.I64).Col("t", ColType.F32).Col("value", ColType.F32).Keys("curve_id", "i");

            int cacheMismatch = 0;
            foreach (var s in sources)
            {
                var keys = s.Curve.Curve.keys;
                string raw = s.Node != null ? string.Join(" | ", s.Node.GetValues("key")) : "";
                tc.Add(s.Id, s.Origin, s.Path, keys.Length, s.Curve.minTime, s.Curve.maxTime, raw);
                for (int i = 0; i < keys.Length; i++)
                    tk.Add(s.Id, i, keys[i].time, keys[i].value, keys[i].inTangent, keys[i].outTangent);
                var ts_ = SampleTimes(keys);
                var vals = ts_.Select(t => s.Curve.Evaluate(t)).ToArray();
                // self-check: evaluation must not depend on the curve's internal cache (reverse order, fresh evaluation)
                for (int i = ts_.Count - 1; i >= 0; i--)
                {
                    float v = s.Curve.Evaluate(ts_[i]);
                    if (BitConverter.ToInt32(BitConverter.GetBytes(v), 0) != BitConverter.ToInt32(BitConverter.GetBytes(vals[i]), 0) && !(float.IsNaN(v) && float.IsNaN(vals[i]))) cacheMismatch++;
                }
                for (int i = 0; i < ts_.Count; i++) ts.Add(s.Id, i, ts_[i], vals[i]);
            }
            g.Prov("cache_order_mismatches", cacheMismatch);
            if (cacheMismatch != 0) throw new InvalidOperationException("AnimationCurve evaluation depended on evaluation order in " + cacheMismatch + " samples");
            yield return ("curves_eval/curves_eval.json", g);
        }

        public static List<float> SampleTimes(Keyframe[] keys)
        {
            var set = new SortedSet<float>();
            if (keys.Length == 0) { foreach (var t in new[] { -1f, 0f, 1f }) set.Add(t); return set.ToList(); }
            float tmin = keys[0].time, tmax = keys[keys.Length - 1].time;
            float span = tmax - tmin;
            if (span <= 0) span = Math.Max(1f, Math.Abs(tmin));
            foreach (var k in keys)
            {
                set.Add(k.time);
                set.Add(NextDown(k.time));
                set.Add(NextUp(k.time));
            }
            for (int i = 0; i + 1 < keys.Length; i++)
            {
                float a = keys[i].time, b = keys[i + 1].time;
                set.Add(a + (b - a) * 0.5f);
                set.Add(a + (b - a) * 0.25f);
                set.Add(a + (b - a) * 0.75f);
            }
            const int N = 64;
            float lo = tmin - 0.1f * span, hi = tmax + 0.1f * span;
            for (int i = 0; i <= N; i++) set.Add(lo + (hi - lo) * i / N);
            return set.Where(f => !float.IsNaN(f) && !float.IsInfinity(f)).ToList();
        }

        public static float NextUp(float f)
        {
            if (float.IsNaN(f) || float.IsPositiveInfinity(f)) return f;
            if (f == 0) return float.Epsilon;
            int b = BitConverter.ToInt32(BitConverter.GetBytes(f), 0);
            b += f > 0 ? 1 : -1;
            return BitConverter.ToSingle(BitConverter.GetBytes(b), 0);
        }

        public static float NextDown(float f) => -NextUp(-f);

        static IEnumerable<CurveSource> PhysicsGlobalsCurves()
        {
            var pg = KspEnv.Physics;
            foreach (var name in new[] { "dragCurveTip", "dragCurveSurface", "dragCurveTail", "dragCurveMultiplier", "dragCurveCd", "dragCurveCdPower", "dragCurvePseudoReynolds" })
                yield return new CurveSource { Id = "PhysicsGlobals." + name, Origin = "PhysicsGlobals (prefab, then Physics.cfg via LoadDatabase)", Path = name, Curve = U.Get<FloatCurve>(pg, name) };
            foreach (var kv in PhysicsGlobals.LiftingSurfaceCurves.OrderBy(k => k.Key, StringComparer.Ordinal))
            {
                var c = kv.Value;
                yield return new CurveSource { Id = $"LiftingSurface.{kv.Key}.lift", Origin = "PhysicsGlobals.LiftingSurfaceCurves", Path = kv.Key + "/lift", Curve = c.liftCurve };
                yield return new CurveSource { Id = $"LiftingSurface.{kv.Key}.liftMach", Origin = "PhysicsGlobals.LiftingSurfaceCurves", Path = kv.Key + "/liftMach", Curve = c.liftMachCurve };
                yield return new CurveSource { Id = $"LiftingSurface.{kv.Key}.drag", Origin = "PhysicsGlobals.LiftingSurfaceCurves", Path = kv.Key + "/drag", Curve = c.dragCurve };
                yield return new CurveSource { Id = $"LiftingSurface.{kv.Key}.dragMach", Origin = "PhysicsGlobals.LiftingSurfaceCurves", Path = kv.Key + "/dragMach", Curve = c.dragMachCurve };
            }
        }

        static IEnumerable<CurveSource> BodyCurves()
        {
            foreach (var cb in KspEnv.BodiesBySpawn)
                foreach (var f in typeof(CelestialBody).GetFields().Where(f => f.FieldType == typeof(FloatCurve)).OrderBy(f => f.MetadataToken))
                {
                    var fc = (FloatCurve)f.GetValue(cb);
                    if (fc.Curve.length == 0) continue;
                    yield return new CurveSource { Id = $"Body.{cb.bodyName}.{f.Name}", Origin = "CelestialBody (sharedassets9 prefab, deserialized)", Path = f.Name, Curve = fc };
                }
        }

        /// <summary>All nodes in stock part cfgs that carry 'key' values, loaded with KSP's ConfigNode.Load and FloatCurve.Load.</summary>
        public static IEnumerable<CurveSource> PartCfgCurves(List<string> errors)
        {
            string root = Path.Combine(Run.InputsDir, "GameData", "Squad", "Parts");
            var files = Directory.GetFiles(root, "*.cfg", SearchOption.AllDirectories).OrderBy(f => f, StringComparer.Ordinal).ToList();
            var seen = new HashSet<string>();
            foreach (var file in files)
            {
                string rel = file.Substring(Run.InputsDir.Length + 1).Replace('\\', '/');
                var cn = ConfigNode.Load(file);
                if (cn == null) { errors.Add(rel + ": ConfigNode.Load returned null"); continue; }
                foreach (var (node, path) in Walk(cn, ""))
                {
                    if (!node.HasValue("key")) continue;
                    var fc = new FloatCurve();
                    try { fc.Load(node); }
                    catch (Exception e) { errors.Add(rel + ":" + path + ": " + e.GetType().Name); continue; }
                    string id = "cfg:" + rel + ":" + path;
                    if (!seen.Add(id)) { int n = 2; while (!seen.Add(id + "#" + n)) n++; id += "#" + n; }
                    yield return new CurveSource { Id = id, Origin = "part cfg", Path = rel + ":" + path, Curve = fc, Node = node };
                }
            }
        }

        static IEnumerable<(ConfigNode, string)> Walk(ConfigNode n, string path)
        {
            var counts = new Dictionary<string, int>();
            foreach (var c in n.nodes.Cast<ConfigNode>())
            {
                string label = c.name;
                string nameVal = c.GetValue("name");
                if (!string.IsNullOrEmpty(nameVal)) label += "[" + nameVal + "]";
                counts.TryGetValue(label, out int k); counts[label] = k + 1;
                if (k > 0) label += "#" + (k + 1);
                string p = path.Length == 0 ? label : path + "/" + label;
                yield return (c, p);
                foreach (var x in Walk(c, p)) yield return x;
            }
        }

        static List<CurveSource> Synthetic()
        {
            var defs = new (string id, string[] keys, string why)[]
            {
                ("auto_tangents_sorted", new[] { "0 0", "1 1", "2 4", "3 9" }, "2-value keys: Unity AddKeySmoothTangents recomputes neighbour slopes"),
                ("auto_tangents_unsorted", new[] { "3 9", "0 0", "2 4", "1 1" }, "insertion order changes which neighbours get recalculated"),
                ("mixed_explicit_and_auto", new[] { "0 1 0 0", "10 2", "20 0 0 0" }, "a 2-value key overwrites the explicit tangents of its neighbours"),
                ("duplicate_time", new[] { "0 0 0 0", "1 1 0 0", "1 5 0 0", "2 2 0 0" }, "AddKey rejects a key whose time already exists"),
                ("single_key", new[] { "5 3" }, "1 key: Evaluate returns its value everywhere"),
                ("empty", new string[0], "no keys: Evaluate returns 0"),
                ("stepped_infinite_tangent", new[] { "0 0 0 Infinity", "1 1 Infinity 0", "2 3 0 0" }, "+Infinity out/in tangent -> constant segment"),
                ("negative_infinite_tangent", new[] { "0 0 0 -Infinity", "1 1 0 0" }, "-Infinity is NOT treated as stepped"),
                ("tiny_dx", new[] { "0 0 1 1", "0.00001 1 1 1", "1 2 0 0" }, "segment shorter than 1e-4: length clamped to 1e-4 in CalculateCacheData"),
                ("comma_semicolon_tab", new[] { "0,1,0,0", "1;2;0;0", "2\t3\t0\t0" }, "FloatCurve.Load delimiters ' ' ',' ';' '\\t'"),
                ("three_and_five_values", new[] { "0 1 2", "1 2 3 4 5", "2 0 0 0" }, "only exactly 4 values are read as tangents; 3 or 5 values -> Add(t,v)"),
                ("large_magnitude", new[] { "0 101325 0 -12", "35000 0.5 -0.0001 -0.0001", "70000 0 0 0" }, "large values: float rounding in the cubic"),
                ("isp_like", new[] { "0 320", "1 270", "9 0.001" }, "typical atmosphereCurve with auto tangents"),
            };
            var list = new List<CurveSource>();
            foreach (var d in defs)
            {
                var node = new ConfigNode("CURVE");
                foreach (var k in d.keys) node.AddValue("key", k);
                var fc = new FloatCurve();
                fc.Load(node);
                list.Add(new CurveSource { Id = "synthetic." + d.id, Origin = "synthetic: " + d.why, Path = d.id, Curve = fc, Node = node });
            }
            // FloatCurve(Keyframe[]) path (used by KSP code-side defaults, e.g. PhysicsGlobals field initializers)
            var kc = new FloatCurve(new[] { new Keyframe(0f, 1f, 0f, 0f), new Keyframe(0.85f, 1.19f, 0.6960422f, 0.6960422f), new Keyframe(1.1f, 2.83f, 0.730473f, 0.730473f), new Keyframe(5f, 4f, 0f, 0f) });
            list.Add(new CurveSource { Id = "synthetic.keyframe_ctor_dragTip", Origin = "synthetic: new FloatCurve(Keyframe[]) -> Add(t,v,in,out) per key", Path = "keyframe_ctor", Curve = kc });
            return list;
        }
    }
}
