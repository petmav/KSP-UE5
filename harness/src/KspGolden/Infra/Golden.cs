using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace KspGolden.Infra
{
    public enum ColType { F64, F32, I64, Str, Bool }

    public sealed class Column
    {
        public string Name, Unit, Note;
        public ColType Type;
        public Column(string name, ColType type, string unit = "", string note = null) { Name = name; Type = type; Unit = unit; Note = note; }
    }

    /// <summary>One table of a golden dataset: typed columns, key columns, rows.</summary>
    public sealed class Table
    {
        public readonly string Name, Description;
        public readonly List<Column> Columns = new List<Column>();
        public readonly List<string> Key = new List<string>();
        public readonly List<object[]> Rows = new List<object[]>();

        public Table(string name, string description) { Name = name; Description = description; }

        public Table Col(string name, ColType type, string unit = "", string note = null) { Columns.Add(new Column(name, type, unit, note)); return this; }
        public Table Keys(params string[] k) { Key.AddRange(k); return this; }

        public void Add(params object[] row)
        {
            if (row.Length != Columns.Count) throw new ArgumentException($"table {Name}: row has {row.Length} values, expected {Columns.Count}");
            for (int i = 0; i < row.Length; i++)
            {
                var c = Columns[i];
                object v = row[i];
                if (v == null) continue;
                bool ok = c.Type switch
                {
                    ColType.F64 => v is double,
                    ColType.F32 => v is float,
                    ColType.I64 => v is int || v is long || v is uint,
                    ColType.Str => v is string,
                    ColType.Bool => v is bool,
                    _ => false
                };
                if (!ok) throw new ArgumentException($"table {Name}: column {c.Name} expects {c.Type}, got {v.GetType().Name}");
            }
            Rows.Add(row);
        }
    }

    /// <summary>A golden dataset file (schema "ksp-golden/1").</summary>
    public sealed class GoldenFile
    {
        public readonly string Dataset, Trust, Description;
        public readonly List<KeyValuePair<string, object>> Provenance = new List<KeyValuePair<string, object>>();
        public readonly List<string> Oracles = new List<string>();
        public readonly List<Table> Tables = new List<Table>();

        public GoldenFile(string dataset, string trust, string description) { Dataset = dataset; Trust = trust; Description = description; }

        public Table AddTable(Table t) { Tables.Add(t); return t; }
        public GoldenFile Oracle(string o) { Oracles.Add(o); return this; }
        public GoldenFile Prov(string k, object v) { Provenance.Add(new KeyValuePair<string, object>(k, v)); return this; }

        public int RowCount => Tables.Sum(t => t.Rows.Count);

        public void Write(string path)
        {
            var w = new JsonWriter();
            w.BeginObject();
            w.Key("schema").Str("ksp-golden/1");
            w.Key("dataset").Str(Dataset);
            w.Key("trust").Str(Trust);
            w.Key("description").Str(Description);
            w.Key("oracles").BeginArray(); foreach (var o in Oracles) w.Str(o); w.EndArray();
            w.Key("provenance").BeginObject();
            foreach (var kv in Run.CommonProvenance.Concat(Provenance)) { w.Key(kv.Key).Raw(kv.Value); }
            w.EndObject();
            w.Key("tables").BeginArray();
            foreach (var t in Tables)
            {
                w.BeginObject();
                w.Key("name").Str(t.Name);
                w.Key("description").Str(t.Description);
                w.Key("key").BeginArray(true); foreach (var k in t.Key) w.Str(k); w.EndArray();
                w.Key("columns").BeginArray();
                foreach (var c in t.Columns)
                {
                    w.BeginObject(true);
                    w.Key("name").Str(c.Name);
                    w.Key("type").Str(c.Type.ToString().ToLowerInvariant());
                    w.Key("unit").Str(c.Unit ?? "");
                    if (c.Note != null) w.Key("note").Str(c.Note);
                    w.EndObject();
                }
                w.EndArray();
                w.Key("row_count").Int(t.Rows.Count);
                w.Key("rows").BeginArray();
                foreach (var r in t.Rows)
                {
                    w.BeginArray(true);
                    foreach (var v in r) w.Raw(v);
                    w.EndArray();
                }
                w.EndArray();
                w.EndObject();
            }
            w.EndArray();
            w.EndObject();
            Directory.CreateDirectory(Path.GetDirectoryName(path));
            File.WriteAllText(path, w.ToString() + "\n", new UTF8Encoding(false));
        }

        /// <summary>Markdown description generated from the same metadata as the JSON (inputs, units, methods).</summary>
        public void WriteReadme(string path, string jsonFileName, IEnumerable<string> extraNotes)
        {
            var sb = new StringBuilder();
            sb.AppendLine("# " + Dataset).AppendLine();
            sb.AppendLine("Generated by `harness/run.ps1` (KspGolden). Do not edit; regenerate.").AppendLine();
            sb.AppendLine("- File: `" + jsonFileName + "` (schema `ksp-golden/1`, " + RowCount + " rows)");
            sb.AppendLine("- Trust: **" + Trust + "** (see harness/README.md, Trust levels)").AppendLine();
            sb.AppendLine("## What it is").AppendLine().AppendLine(Description).AppendLine();
            foreach (var n in extraNotes) sb.AppendLine(n).AppendLine();
            sb.AppendLine("## KSP methods used as oracles").AppendLine();
            foreach (var o in Oracles) sb.AppendLine("- `" + o + "`");
            sb.AppendLine();
            foreach (var t in Tables)
            {
                sb.AppendLine("## Table `" + t.Name + "` (" + t.Rows.Count + " rows)").AppendLine().AppendLine(t.Description).AppendLine();
                sb.AppendLine("Key: " + string.Join(", ", t.Key.Select(k => "`" + k + "`"))).AppendLine();
                sb.AppendLine("| column | type | unit | note |").AppendLine("|---|---|---|---|");
                foreach (var c in t.Columns)
                    sb.AppendLine("| " + c.Name + " | " + c.Type.ToString().ToLowerInvariant() + " | " + (c.Unit ?? "") + " | " + (c.Note ?? "").Replace("|", "\\|") + " |");
                sb.AppendLine();
            }
            File.WriteAllText(path, sb.ToString(), new UTF8Encoding(false));
        }
    }
}
