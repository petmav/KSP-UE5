using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using KspGolden.Infra;

namespace KspGolden.Generators
{
    /// <summary>Dataset 5: ConfigNode parse/serialise corpus (real ConfigNode.Load / Parse / Save / ToString).</summary>
    public static class ConfigNodeGen
    {
        static readonly string[] Craft =
        {
            "Ships/VAB/Kerbal X.craft", "Ships/VAB/Jumping Flea.craft", "Ships/VAB/Kerbal 1.craft", "Ships/VAB/Kerbal 2.craft",
            "Ships/VAB/Learstar A1.craft", "Ships/VAB/Ion-Powered Space Probe.craft", "Ships/VAB/Dynawing.craft", "Ships/VAB/ComSat Lx.craft",
            "Ships/SPH/Aeris 3A.craft", "Ships/SPH/Aeris 4A.craft", "Ships/SPH/Gull.craft", "Ships/SPH/Stearwing A300.craft",
            "Ships/SPH/Bug-E Buggy.craft", "Ships/SPH/Mallard.craft",
        };

        static readonly string[] Cfgs =
        {
            "Physics.cfg", "PartDatabase.cfg",
            "GameData/Squad/Resources/ResourcesGeneric.cfg", "GameData/Squad/Resources/TechTree.cfg",
            "GameData/Squad/Parts/Command/mk1pod_v2/mk1Pod_v2.cfg", "GameData/Squad/Parts/FuelTank/Size1_Tanks/fuelTankT400.cfg",
            "GameData/Squad/Parts/Engine/liquidEngineLV-T45_v2/liquidEngineLV-T45_v2.cfg", "GameData/Squad/Parts/Engine/liquidEngineMainsail_v2/liquidEngineMainsail_v2.cfg",
            "GameData/Squad/Parts/Engine/rapierEngine/rapierEngine.cfg", "GameData/Squad/Parts/Aero/wings/connector1.cfg",
            "GameData/Squad/Parts/Engine/jetEngines/jetEngineBasic.cfg", "GameData/Squad/Parts/Utility/parachuteMk1/parachuteMk1.cfg",
        };

        /// <summary>Synthetic edge cases, written to disk with the exact bytes given, then loaded with ConfigNode.Load (File.ReadAllLines).</summary>
        static readonly (string id, string text)[] Synthetic =
        {
            ("basic_crlf", "a = 1\r\nNODE\r\n{\r\n\tb = 2\r\n}\r\n"),
            ("basic_lf", "a = 1\nNODE\n{\n\tb = 2\n}\n"),
            ("cr_only", "a = 1\rNODE\r{\rb = 2\r}\r"),
            ("utf8_bom", "﻿a = 1\r\nb = 2\r\n"),
            ("comments", "// header\r\na = 1 // trailing comment\r\n  // indented comment\r\nb = http://example.com/x\r\n"),
            ("braces_same_line", "NODE { a = 1 }\r\nN2{\r\nb=2}\r\nN3\r\n{c = 3\r\n}\r\n"),
            ("brace_after_value", "NODE\r\n{\r\na = 1}\r\nb = 2\r\n"),
            ("multiple_equals", "a = b = c\r\nk==v\r\n"),
            ("empty_name_and_value", "= value\r\nname =\r\n=\r\n"),
            ("whitespace_and_tabs", "  \t key with spaces \t=\t value  with\ttabs \t\r\n"),
            ("bare_token_dropped", "orphan\r\na = 1\r\nNODE\r\n{\r\nalsoOrphan\r\n}\r\n"),
            ("node_id", "PART (myId)\r\n{\r\nname = x\r\n}\r\nPART(noSpace)\r\n{\r\n}\r\n"),
            ("duplicate_keys", "key = 1\r\nkey = 2\r\nNODE\r\n{\r\n}\r\nNODE\r\n{\r\nkey = 3\r\n}\r\n"),
            ("unbalanced_close", "a = 1\r\n}\r\nb = 2\r\n"),
            ("unterminated_node", "NODE\r\n{\r\na = 1\r\n"),
            ("anonymous_node", "{\r\na = 1\r\n}\r\n"),
            ("unicode", "name = Kérbal ☃ 中文\r\n"),
            ("numbers", "f = 1E-05\r\ng = -0.0\r\nh = 1.0000001\r\ninf = Infinity\r\nnan = NaN\r\n"),
            ("nested_deep", "A\r\n{\r\nB\r\n{\r\nC\r\n{\r\nd = 4\r\n}\r\n}\r\n}\r\n"),
            ("only_comment", "// nothing\r\n"),
            ("empty_file", ""),
        };

        static string Sha(byte[] b) { using (var h = SHA256.Create()) return BitConverter.ToString(h.ComputeHash(b)).Replace("-", "").ToLowerInvariant(); }

        public static IEnumerable<(string, GoldenFile)> Generate()
        {
            var g = new GoldenFile("confignode_roundtrip", "direct",
                "KSP's ConfigNode on real files and edge cases: ConfigNode.Load(path) (File.ReadAllLines -> PreFormatConfig -> RecurseFormat), " +
                "ConfigNode.Save(path) (StreamWriter, UTF-8 without BOM, Environment.NewLine = CRLF on Windows), ConfigNode.ToString() " +
                "(\\n line ends, includes node ids) and ConfigNode.Parse(string). Load returns a node named 'root'; Save writes root's values and " +
                "child nodes without the root wrapper. Exact saved bytes are in confignode_roundtrip/saved/<id>.cfg and the parsed trees in " +
                "confignode_roundtrip/trees/<id>.json.");
            g.Oracle("ConfigNode.Load(string)").Oracle("ConfigNode.Save(string)").Oracle("ConfigNode.ToString()").Oracle("ConfigNode.Parse(string)");
            var tc = g.AddTable(new Table("corpus", "One row per input."));
            tc.Col("id", ColType.Str).Col("source", ColType.Str).Col("input_sha256", ColType.Str).Col("input_bytes", ColType.I64)
              .Col("loaded", ColType.Bool, "", "ConfigNode.Load returned non-null")
              .Col("root_values", ColType.I64).Col("root_nodes", ColType.I64).Col("total_values", ColType.I64).Col("total_nodes", ColType.I64)
              .Col("save_sha256", ColType.Str).Col("save_bytes", ColType.I64)
              .Col("tostring_sha256", ColType.Str).Col("parse_tree_equals_load_tree", ColType.Bool, "", "ConfigNode.Parse(File.ReadAllText) tree == Load tree")
              .Col("resave_identical", ColType.Bool, "", "Save(Load(Save(Load(x)))) bytes == Save(Load(x))")
              .Keys("id");
            var ts = g.AddTable(new Table("save_text", "Exact ConfigNode.Save output for the synthetic cases (full text; real files are in saved/)."));
            ts.Col("id", ColType.Str).Col("save_text", ColType.Str).Col("tostring_text", ColType.Str).Keys("id");

            string outDir = Path.Combine(Run.GoldenDir, "confignode_roundtrip");
            string savedDir = Path.Combine(outDir, "saved"), treeDir = Path.Combine(outDir, "trees"), synthDir = Path.Combine(outDir, "synthetic_inputs");
            foreach (var d in new[] { savedDir, treeDir, synthDir }) Directory.CreateDirectory(d);
            string tmp = Path.Combine(Path.GetTempPath(), "kspgolden_cfg_" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(tmp);
            try
            {
                var inputs = new List<(string id, string path, string source, bool synthetic)>();
                foreach (var c in Craft) inputs.Add(("craft_" + San(Path.GetFileNameWithoutExtension(c)), Path.Combine(Run.InputsDir, c), c, false));
                foreach (var c in Cfgs) inputs.Add(("cfg_" + San(Path.GetFileNameWithoutExtension(c)), Path.Combine(Run.InputsDir, c), c, false));
                foreach (var (id, text) in Synthetic)
                {
                    string p = Path.Combine(synthDir, id + ".cfg");
                    File.WriteAllBytes(p, new UTF8Encoding(false).GetBytes(text));
                    inputs.Add(("syn_" + id, p, "synthetic_inputs/" + id + ".cfg", true));
                }
                foreach (var (id, path, source, synthetic) in inputs)
                {
                    if (!File.Exists(path)) { KspEnv.Notes.Add("confignode: missing input " + source); continue; }
                    var bytes = File.ReadAllBytes(path);
                    var node = ConfigNode.Load(path);
                    string savePath = Path.Combine(savedDir, id + ".cfg");
                    byte[] saved = null; string tostr = null; bool parseEq = false, resaveSame = false;
                    int rv = 0, rn = 0, tv = 0, tn = 0;
                    if (node != null)
                    {
                        node.Save(savePath);
                        saved = File.ReadAllBytes(savePath);
                        tostr = node.ToString();
                        rv = node.values.Count; rn = node.nodes.Count; Count(node, ref tv, ref tn);
                        var parsed = ConfigNode.Parse(File.ReadAllText(path));
                        parseEq = TreeJson(parsed) == TreeJson(node);
                        var again = ConfigNode.Load(savePath);
                        string p2 = Path.Combine(tmp, id + ".cfg");
                        if (again != null) { again.Save(p2); resaveSame = File.ReadAllBytes(p2).SequenceEqual(saved); }
                        File.WriteAllText(Path.Combine(treeDir, id + ".json"), TreeJson(node) + "\n", new UTF8Encoding(false));
                    }
                    tc.Add(id, source, Sha(bytes), bytes.LongLength, node != null, rv, rn, tv, tn,
                        saved != null ? Sha(saved) : "", saved != null ? saved.LongLength : 0L,
                        tostr != null ? Sha(Encoding.UTF8.GetBytes(tostr)) : "", parseEq, resaveSame);
                    if (synthetic) ts.Add(id, saved != null ? Encoding.UTF8.GetString(saved) : null, tostr);
                }
            }
            finally { try { Directory.Delete(tmp, true); } catch { } }
            yield return ("confignode_roundtrip/confignode_roundtrip.json", g);
        }

        static string San(string s) => new string(s.Select(c => char.IsLetterOrDigit(c) || c == '-' || c == '_' ? c : '_').ToArray());

        static void Count(ConfigNode n, ref int v, ref int nn)
        {
            v += n.values.Count; nn += n.nodes.Count;
            foreach (ConfigNode c in n.nodes) Count(c, ref v, ref nn);
        }

        /// <summary>Tree dump: {"name","id","comment","values":[[name,value,comment]...],"nodes":[...]} preserving order.</summary>
        public static string TreeJson(ConfigNode n)
        {
            var w = new JsonWriter();
            Write(w, n);
            return w.ToString();
        }

        static void Write(JsonWriter w, ConfigNode n)
        {
            w.BeginObject();
            w.Key("name").Str(n.name);
            w.Key("id").Str(n.id);
            w.Key("comment").Str(n.comment);
            w.Key("values").BeginArray();
            foreach (ConfigNode.Value v in n.values) { w.BeginArray(true); w.Str(v.name); w.Str(v.value); w.Str(v.comment); w.EndArray(); }
            w.EndArray();
            w.Key("nodes").BeginArray();
            foreach (ConfigNode c in n.nodes) Write(w, c);
            w.EndArray();
            w.EndObject();
        }
    }
}
