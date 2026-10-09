using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

namespace KspGolden.Infra
{
    /// <summary>JSON number kept as its source text plus the correctly rounded double.</summary>
    public sealed class JNum
    {
        public readonly string Text;
        public readonly double Value;
        public JNum(string text) { Text = text; Value = ExactNumber.ParseDouble(text); }
        public override string ToString() => Text;
    }

    /// <summary>Order-preserving JSON object.</summary>
    public sealed class JObj
    {
        public readonly List<KeyValuePair<string, object>> Items = new List<KeyValuePair<string, object>>();
        private readonly Dictionary<string, object> map = new Dictionary<string, object>();
        public void Add(string k, object v) { Items.Add(new KeyValuePair<string, object>(k, v)); map[k] = v; }
        public bool Has(string k) => map.ContainsKey(k);
        public object this[string k] => map.TryGetValue(k, out var v) ? v : null;
        public JObj Obj(string k) => this[k] as JObj;
        public List<object> Arr(string k) => this[k] as List<object>;
        public string Str(string k) => this[k] as string;
        public double Num(string k)
        {
            var v = this[k];
            if (v is JNum n) return n.Value;
            if (v is string s) return ExactNumber.ParseDouble(s); // "NaN"/"Infinity" encoded as strings
            throw new KeyNotFoundException("number field '" + k + "' missing");
        }
    }

    public static class JsonReader
    {
        public static object ParseFile(string path) => Parse(File.ReadAllText(path, Encoding.UTF8));

        public static object Parse(string s)
        {
            int i = 0;
            var v = Value(s, ref i);
            Ws(s, ref i);
            if (i != s.Length) throw new FormatException("trailing data at " + i);
            return v;
        }

        private static void Ws(string s, ref int i) { while (i < s.Length && char.IsWhiteSpace(s[i])) i++; }

        private static object Value(string s, ref int i)
        {
            Ws(s, ref i);
            char c = s[i];
            if (c == '{')
            {
                var o = new JObj(); i++; Ws(s, ref i);
                if (s[i] == '}') { i++; return o; }
                while (true)
                {
                    Ws(s, ref i); string k = Str(s, ref i); Ws(s, ref i);
                    if (s[i] != ':') throw new FormatException("expected : at " + i); i++;
                    o.Add(k, Value(s, ref i)); Ws(s, ref i);
                    if (s[i] == ',') { i++; continue; }
                    if (s[i] == '}') { i++; return o; }
                    throw new FormatException("expected , or } at " + i);
                }
            }
            if (c == '[')
            {
                var a = new List<object>(); i++; Ws(s, ref i);
                if (s[i] == ']') { i++; return a; }
                while (true)
                {
                    a.Add(Value(s, ref i)); Ws(s, ref i);
                    if (s[i] == ',') { i++; continue; }
                    if (s[i] == ']') { i++; return a; }
                    throw new FormatException("expected , or ] at " + i);
                }
            }
            if (c == '"') return Str(s, ref i);
            if (At(s, i, "true")) { i += 4; return true; }
            if (At(s, i, "false")) { i += 5; return false; }
            if (At(s, i, "null")) { i += 4; return null; }
            if (At(s, i, "NaN")) { i += 3; return new JNum("NaN"); }
            if (At(s, i, "Infinity")) { i += 8; return new JNum("Infinity"); }
            if (At(s, i, "-Infinity")) { i += 9; return new JNum("-Infinity"); }
            int st = i;
            while (i < s.Length && "+-0123456789.eE".IndexOf(s[i]) >= 0) i++;
            return new JNum(s.Substring(st, i - st));
        }

        private static bool At(string s, int i, string lit) => i + lit.Length <= s.Length && string.CompareOrdinal(s, i, lit, 0, lit.Length) == 0;

        private static string Str(string s, ref int i)
        {
            if (s[i] != '"') throw new FormatException("expected string at " + i);
            i++;
            var sb = new StringBuilder();
            while (s[i] != '"')
            {
                char c = s[i++];
                if (c == '\\')
                {
                    char e = s[i++];
                    switch (e)
                    {
                        case 'n': sb.Append('\n'); break;
                        case 'r': sb.Append('\r'); break;
                        case 't': sb.Append('\t'); break;
                        case 'b': sb.Append('\b'); break;
                        case 'f': sb.Append('\f'); break;
                        case 'u': sb.Append((char)int.Parse(s.Substring(i, 4), NumberStyles.HexNumber)); i += 4; break;
                        default: sb.Append(e); break;
                    }
                }
                else sb.Append(c);
            }
            i++;
            return sb.ToString();
        }
    }

    /// <summary>Minimal streaming JSON writer with exact float formatting.</summary>
    public sealed class JsonWriter
    {
        private readonly StringBuilder sb = new StringBuilder();
        private readonly Stack<bool> first = new Stack<bool>();
        private readonly Stack<bool> compact = new Stack<bool>();
        private int depth;
        private bool pendingKey;

        public override string ToString() => sb.ToString();

        private void Sep()
        {
            if (pendingKey) { pendingKey = false; return; }
            if (first.Count == 0) return;
            bool f = first.Pop();
            if (!f) sb.Append(',');
            first.Push(false);
            if (!compact.Peek()) { sb.Append('\n'); sb.Append(' ', depth); }
        }

        public JsonWriter BeginObject(bool compactLayout = false) { Sep(); sb.Append('{'); Push(compactLayout); return this; }
        public JsonWriter EndObject() { Pop(); sb.Append('}'); return this; }
        public JsonWriter BeginArray(bool compactLayout = false) { Sep(); sb.Append('['); Push(compactLayout); return this; }
        public JsonWriter EndArray() { Pop(); sb.Append(']'); return this; }

        private void Push(bool c)
        {
            bool parentCompact = compact.Count > 0 && compact.Peek();
            first.Push(true); compact.Push(c || parentCompact); depth++;
        }

        private void Pop()
        {
            bool wasFirst = first.Pop(); bool c = compact.Pop(); depth--;
            if (!c && !wasFirst) { sb.Append('\n'); sb.Append(' ', depth); }
        }

        public JsonWriter Key(string k) { Sep(); WriteString(k); sb.Append(compact.Peek() ? ":" : ": "); pendingKey = true; return this; }
        public JsonWriter Str(string v) { Sep(); if (v == null) sb.Append("null"); else WriteString(v); return this; }
        public JsonWriter Bool(bool v) { Sep(); sb.Append(v ? "true" : "false"); return this; }
        public JsonWriter Null() { Sep(); sb.Append("null"); return this; }
        public JsonWriter Int(long v) { Sep(); sb.Append(v.ToString(CultureInfo.InvariantCulture)); return this; }

        public JsonWriter Dbl(double v)
        {
            Sep();
            string s = ExactNumber.FormatDouble(v);
            if (double.IsNaN(v) || double.IsInfinity(v)) WriteString(s); else sb.Append(s);
            return this;
        }

        public JsonWriter Flt(float v)
        {
            Sep();
            string s = ExactNumber.FormatFloat(v);
            if (float.IsNaN(v) || float.IsInfinity(v)) WriteString(s); else sb.Append(s);
            return this;
        }

        public JsonWriter Raw(object v)
        {
            switch (v)
            {
                case null: return Null();
                case string s: return Str(s);
                case bool b: return Bool(b);
                case float f: return Flt(f);
                case double d: return Dbl(d);
                case int i: return Int(i);
                case long l: return Int(l);
                case uint u: return Int(u);
                case JNum n: return Dbl(n.Value);
                default: return Str(Convert.ToString(v, CultureInfo.InvariantCulture));
            }
        }

        private void WriteString(string s)
        {
            sb.Append('"');
            foreach (char c in s)
            {
                switch (c)
                {
                    case '"': sb.Append("\\\""); break;
                    case '\\': sb.Append("\\\\"); break;
                    case '\n': sb.Append("\\n"); break;
                    case '\r': sb.Append("\\r"); break;
                    case '\t': sb.Append("\\t"); break;
                    default:
                        if (c < 0x20 || c > 0x7e) sb.Append("\\u").Append(((int)c).ToString("x4"));
                        else sb.Append(c);
                        break;
                }
            }
            sb.Append('"');
        }
    }
}
