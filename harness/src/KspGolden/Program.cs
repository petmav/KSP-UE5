using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using KspGolden.Generators;
using KspGolden.Infra;

namespace KspGolden
{
    public static class Program
    {
        static readonly (string name, Func<IEnumerable<(string file, GoldenFile g)>> gen)[] Generators =
        {
            ("curves_eval", FloatCurveGen.Generate),
            ("body_catalog", BodiesGen.Generate),
            ("orbit_propagation", OrbitGen.Generate),
            ("atmosphere_curves", AtmosphereGen.Generate),
            ("aero_forces", AeroGen.Generate),
            ("physics_constants", ThermalGen.Generate),
            ("confignode_roundtrip", ConfigNodeGen.Generate),
            ("engine_thrust", EnginesGen.Generate),
            ("math_shim", MathGen.Generate),
        };

        public static int Main(string[] args)
        {
            string harness = null, outDir = null, label = "rebuilt-from-decompile", pristineSha = "unknown";
            var only = new HashSet<string>();
            bool echo = false;
            for (int i = 0; i < args.Length; i++)
            {
                switch (args[i])
                {
                    case "--harness": harness = args[++i]; break;
                    case "--out": outDir = args[++i]; break;
                    case "--datasets": foreach (var d in args[++i].Split(',')) only.Add(d.Trim()); break;
                    case "--ksp-il": label = args[++i]; break;
                    case "--input-sha": pristineSha = args[++i]; break;
                    case "--echo-log": echo = true; break;
                    default: Console.Error.WriteLine("unknown arg " + args[i]); return 2;
                }
            }
            harness = Path.GetFullPath(harness ?? Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "..", ".."));
            Run.HarnessRoot = harness;
            Run.InputsDir = Path.Combine(harness, "inputs");
            Run.GoldenDir = Path.GetFullPath(outDir ?? Path.Combine(harness, "golden"));
            Run.SerializedDir = Path.Combine(harness, "golden", "serialized");

            var sw = Stopwatch.StartNew();
            KspEnv.InitCulture();
            KspEnv.InitLogging(echo);
            Run.InitProvenance(label, pristineSha);
            KspEnv.InitPhysicsGlobals();
            KspEnv.InitPlanetarium();
            KspEnv.InitBodies(0.0);
            AtmosphereGen.EnsureGame();
            Console.WriteLine($"world ready in {sw.ElapsedMilliseconds} ms: {KspEnv.BodiesBySpawn.Count} bodies");

            var manifest = new List<(string ds, string file, GoldenFile g)>();
            int failures = 0;
            foreach (var (name, gen) in Generators)
            {
                if (only.Count > 0 && !only.Contains(name)) continue;
                var t = Stopwatch.StartNew();
                try
                {
                    foreach (var (file, g) in gen())
                    {
                        string path = Path.Combine(Run.GoldenDir, file);
                        g.Write(path);
                        g.WriteReadme(Path.Combine(Path.GetDirectoryName(path), "README.md"), Path.GetFileName(path), DatasetNotes.For(g.Dataset));
                        manifest.Add((name, file, g));
                        Console.WriteLine($"  {name,-11} {file,-48} {g.RowCount,8} rows  trust={g.Trust}");
                    }
                }
                catch (Exception e)
                {
                    failures++;
                    Console.Error.WriteLine($"GENERATOR FAILED: {name}: {e}");
                }
                Console.WriteLine($"  {name} done in {t.ElapsedMilliseconds} ms");
            }

            // run log (not golden; may differ between runs)
            var log = new List<string>();
            log.Add("ksp_il=" + label);
            log.AddRange(KspEnv.Notes.Select(n => "note: " + n));
            if (Rehosted.CelestialBodyLifecycle.SetupConstantsNote != null) log.Add("note: " + Rehosted.CelestialBodyLifecycle.SetupConstantsNote);
            log.AddRange(Run.Log.Lines.Select(l => "unity-log: " + l));
            Directory.CreateDirectory(Run.GoldenDir);
            File.WriteAllLines(Path.Combine(Run.GoldenDir, "_run.log"), log);
            WriteManifest(manifest, only.Count > 0);
            Console.WriteLine($"total {sw.ElapsedMilliseconds} ms, failures={failures}");
            return failures == 0 ? 0 : 1;
        }

        static void WriteManifest(List<(string ds, string file, GoldenFile g)> m, bool partial)
        {
            var w = new JsonWriter();
            w.BeginObject();
            w.Key("schema").Str("ksp-golden-manifest/1");
            w.Key("partial_run").Bool(partial);
            w.Key("files").BeginArray();
            foreach (var (ds, file, g) in m)
            {
                w.BeginObject(true);
                w.Key("group").Str(ds); w.Key("file").Str(file.Replace('\\', '/')); w.Key("dataset").Str(g.Dataset);
                w.Key("trust").Str(g.Trust); w.Key("rows").Int(g.RowCount);
                w.Key("tables").BeginArray(); foreach (var t in g.Tables) w.Str(t.Name + ":" + t.Rows.Count); w.EndArray();
                w.EndObject();
            }
            w.EndArray();
            w.EndObject();
            File.WriteAllText(Path.Combine(Run.GoldenDir, partial ? "MANIFEST.partial.json" : "MANIFEST.json"), w.ToString() + "\n");
        }
    }
}
