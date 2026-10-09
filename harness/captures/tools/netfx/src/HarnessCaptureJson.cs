// Profiling branch only. Minimal writer for the harness schema "ksp-golden/1" (harness/README.md, "Compare a C++ result").
// Unity-free on purpose: the same file is compiled into the .NET Framework reference tool (harness/captures/tools/netfx),
// so both runtimes format their output with identical code.
// Every f32/f64 column gets a companion "<name>_hex" string column with the raw IEEE bits (X8 / X16);
// harness/compare/compare.py ignores candidate columns that are not in the golden file.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

public static class HcJson
{
	public enum CT
	{
		F64,
		F32,
		I64,
		Str,
		Bool
	}

	public sealed class Col
	{
		public string Name;

		public CT Type;

		public string Note;
	}

	public sealed class Table
	{
		public string Name;

		public string Description;

		public readonly List<Col> Cols = new List<Col>();

		public readonly List<string> Key = new List<string>();

		public readonly List<object[]> Rows = new List<object[]>();

		public bool Hex = true;

		public Table(string name, string description)
		{
			Name = name;
			Description = description;
		}

		public Table C(string name, CT type, string note = null)
		{
			Cols.Add(new Col
			{
				Name = name,
				Type = type,
				Note = note
			});
			return this;
		}

		public Table Keys(params string[] k)
		{
			Key.AddRange(k);
			return this;
		}

		public void Add(params object[] row)
		{
			if (row.Length != Cols.Count)
			{
				throw new ArgumentException("table " + Name + ": row has " + row.Length + " values, expected " + Cols.Count);
			}
			for (int i = 0; i < row.Length; i++)
			{
				object v = row[i];
				if (v == null)
				{
					continue;
				}
				bool ok;
				switch (Cols[i].Type)
				{
				case CT.F64:
					ok = v is double;
					break;
				case CT.F32:
					ok = v is float;
					break;
				case CT.I64:
					ok = v is int || v is long || v is uint;
					break;
				case CT.Str:
					ok = v is string;
					break;
				default:
					ok = v is bool;
					break;
				}
				if (!ok)
				{
					throw new ArgumentException("table " + Name + ": column " + Cols[i].Name + " expects " + Cols[i].Type + ", got " + v.GetType().Name);
				}
			}
			Rows.Add(row);
		}
	}

	public sealed class GFile
	{
		public string Dataset;

		public string Trust;

		public string Description;

		public readonly List<string> Oracles = new List<string>();

		public readonly List<KeyValuePair<string, object>> Prov = new List<KeyValuePair<string, object>>();

		public readonly List<Table> Tables = new List<Table>();

		public GFile(string dataset, string trust, string description)
		{
			Dataset = dataset;
			Trust = trust;
			Description = description;
		}

		public Table AddTable(Table t)
		{
			Tables.Add(t);
			return t;
		}

		public GFile P(string key, object value)
		{
			for (int i = 0; i < Prov.Count; i++)
			{
				if (Prov[i].Key == key)
				{
					Prov[i] = new KeyValuePair<string, object>(key, value);
					return this;
				}
			}
			Prov.Add(new KeyValuePair<string, object>(key, value));
			return this;
		}

		public int RowCount
		{
			get
			{
				int n = 0;
				foreach (Table t in Tables)
				{
					n += t.Rows.Count;
				}
				return n;
			}
		}

		public void Write(string path)
		{
			StringBuilder sb = new StringBuilder(1 << 20);
			sb.Append("{\n");
			sb.Append(" \"schema\": \"ksp-golden/1\",\n");
			sb.Append(" \"dataset\": ").Append(Q(Dataset)).Append(",\n");
			sb.Append(" \"trust\": ").Append(Q(Trust)).Append(",\n");
			sb.Append(" \"description\": ").Append(Q(Description)).Append(",\n");
			sb.Append(" \"oracles\": [");
			for (int i = 0; i < Oracles.Count; i++)
			{
				sb.Append(i == 0 ? "\n  " : ",\n  ").Append(Q(Oracles[i]));
			}
			sb.Append(Oracles.Count > 0 ? "\n ],\n" : "],\n");
			sb.Append(" \"provenance\": {");
			for (int i = 0; i < Prov.Count; i++)
			{
				sb.Append(i == 0 ? "\n  " : ",\n  ").Append(Q(Prov[i].Key)).Append(": ").Append(Val(Prov[i].Value));
			}
			sb.Append(Prov.Count > 0 ? "\n },\n" : "},\n");
			sb.Append(" \"tables\": [");
			for (int ti = 0; ti < Tables.Count; ti++)
			{
				Table t = Tables[ti];
				List<Col> cols = new List<Col>();
				List<int> src = new List<int>();
				List<bool> hex = new List<bool>();
				for (int c = 0; c < t.Cols.Count; c++)
				{
					cols.Add(t.Cols[c]);
					src.Add(c);
					hex.Add(item: false);
					if (t.Hex && (t.Cols[c].Type == CT.F32 || t.Cols[c].Type == CT.F64))
					{
						cols.Add(new Col
						{
							Name = t.Cols[c].Name + "_hex",
							Type = CT.Str,
							Note = "IEEE bits of " + t.Cols[c].Name
						});
						src.Add(c);
						hex.Add(item: true);
					}
				}
				sb.Append(ti == 0 ? "\n  {\n" : ",\n  {\n");
				sb.Append("   \"name\": ").Append(Q(t.Name)).Append(",\n");
				sb.Append("   \"description\": ").Append(Q(t.Description)).Append(",\n");
				sb.Append("   \"key\": [");
				for (int k = 0; k < t.Key.Count; k++)
				{
					sb.Append(k == 0 ? "" : ", ").Append(Q(t.Key[k]));
				}
				sb.Append("],\n   \"columns\": [");
				for (int c = 0; c < cols.Count; c++)
				{
					sb.Append(c == 0 ? "\n    " : ",\n    ");
					sb.Append("{\"name\": ").Append(Q(cols[c].Name)).Append(", \"type\": ").Append(Q(cols[c].Type.ToString().ToLowerInvariant())).Append(", \"unit\": \"\"");
					if (cols[c].Note != null)
					{
						sb.Append(", \"note\": ").Append(Q(cols[c].Note));
					}
					sb.Append("}");
				}
				sb.Append("\n   ],\n");
				sb.Append("   \"row_count\": ").Append(t.Rows.Count.ToString(CultureInfo.InvariantCulture)).Append(",\n");
				sb.Append("   \"rows\": [");
				for (int r = 0; r < t.Rows.Count; r++)
				{
					object[] row = t.Rows[r];
					sb.Append(r == 0 ? "\n    [" : ",\n    [");
					for (int c = 0; c < cols.Count; c++)
					{
						if (c > 0)
						{
							sb.Append(", ");
						}
						object v = row[src[c]];
						if (!hex[c])
						{
							sb.Append(Val(v));
						}
						else if (v == null)
						{
							sb.Append("null");
						}
						else if (v is float)
						{
							sb.Append('"').Append(Hex32((float)v)).Append('"');
						}
						else
						{
							sb.Append('"').Append(Hex64((double)v)).Append('"');
						}
					}
					sb.Append("]");
				}
				sb.Append(t.Rows.Count > 0 ? "\n   ]\n  }" : "]\n  }");
			}
			sb.Append("\n ]\n}\n");
			string dir = Path.GetDirectoryName(path);
			if (!string.IsNullOrEmpty(dir))
			{
				Directory.CreateDirectory(dir);
			}
			File.WriteAllText(path, sb.ToString(), new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));
		}
	}

	public static string Hex32(float f)
	{
		return BitConverter.ToInt32(BitConverter.GetBytes(f), 0).ToString("X8", CultureInfo.InvariantCulture);
	}

	public static string Hex64(double d)
	{
		return BitConverter.DoubleToInt64Bits(d).ToString("X16", CultureInfo.InvariantCulture);
	}

	public static float FromHex32(string s)
	{
		return BitConverter.ToSingle(BitConverter.GetBytes(int.Parse(s, NumberStyles.HexNumber, CultureInfo.InvariantCulture)), 0);
	}

	public static double FromHex64(string s)
	{
		return BitConverter.Int64BitsToDouble(long.Parse(s, NumberStyles.HexNumber, CultureInfo.InvariantCulture));
	}

	// G9 / G17 always round-trip; the post-processing script re-checks every decimal against its _hex column.
	public static string F32(float f)
	{
		if (float.IsNaN(f))
		{
			return "\"NaN\"";
		}
		if (float.IsPositiveInfinity(f))
		{
			return "\"Infinity\"";
		}
		if (float.IsNegativeInfinity(f))
		{
			return "\"-Infinity\"";
		}
		if (f == 0f)
		{
			return (BitConverter.ToInt32(BitConverter.GetBytes(f), 0) < 0) ? "-0.0" : "0.0";
		}
		return Norm(f.ToString("G9", CultureInfo.InvariantCulture));
	}

	public static string F64(double d)
	{
		if (double.IsNaN(d))
		{
			return "\"NaN\"";
		}
		if (double.IsPositiveInfinity(d))
		{
			return "\"Infinity\"";
		}
		if (double.IsNegativeInfinity(d))
		{
			return "\"-Infinity\"";
		}
		if (d == 0.0)
		{
			return (BitConverter.DoubleToInt64Bits(d) < 0) ? "-0.0" : "0.0";
		}
		return Norm(d.ToString("G17", CultureInfo.InvariantCulture));
	}

	static string Norm(string s)
	{
		s = s.Replace("E", "e");
		if (s.IndexOf('.') < 0 && s.IndexOf('e') < 0)
		{
			s += ".0";
		}
		return s;
	}

	public static string Val(object v)
	{
		if (v == null)
		{
			return "null";
		}
		if (v is float)
		{
			return F32((float)v);
		}
		if (v is double)
		{
			return F64((double)v);
		}
		if (v is bool)
		{
			return ((bool)v) ? "true" : "false";
		}
		if (v is int)
		{
			return ((int)v).ToString(CultureInfo.InvariantCulture);
		}
		if (v is long)
		{
			return ((long)v).ToString(CultureInfo.InvariantCulture);
		}
		if (v is uint)
		{
			return ((uint)v).ToString(CultureInfo.InvariantCulture);
		}
		if (v is string[])
		{
			string[] a = (string[])v;
			StringBuilder sb = new StringBuilder("[");
			for (int i = 0; i < a.Length; i++)
			{
				sb.Append(i == 0 ? "" : ", ").Append(Q(a[i]));
			}
			return sb.Append("]").ToString();
		}
		return Q(Convert.ToString(v, CultureInfo.InvariantCulture));
	}

	public static string Q(string s)
	{
		if (s == null)
		{
			return "null";
		}
		StringBuilder sb = new StringBuilder(s.Length + 2);
		sb.Append('"');
		foreach (char c in s)
		{
			switch (c)
			{
			case '"':
				sb.Append("\\\"");
				break;
			case '\\':
				sb.Append("\\\\");
				break;
			case '\n':
				sb.Append("\\n");
				break;
			case '\r':
				sb.Append("\\r");
				break;
			case '\t':
				sb.Append("\\t");
				break;
			default:
				if (c < ' ' || c > '~')
				{
					sb.Append("\\u").Append(((int)c).ToString("x4", CultureInfo.InvariantCulture));
				}
				else
				{
					sb.Append(c);
				}
				break;
			}
		}
		sb.Append('"');
		return sb.ToString();
	}

	// Input files written by harness/captures/tools/prep_inputs.py: tab-separated, backslash escapes \\ \t \n \r.
	public static string Unesc(string s)
	{
		if (s.IndexOf('\\') < 0)
		{
			return s;
		}
		StringBuilder sb = new StringBuilder(s.Length);
		for (int i = 0; i < s.Length; i++)
		{
			char c = s[i];
			if (c == '\\' && i + 1 < s.Length)
			{
				char n = s[++i];
				sb.Append(n == 't' ? '\t' : (n == 'n' ? '\n' : (n == 'r' ? '\r' : n)));
			}
			else
			{
				sb.Append(c);
			}
		}
		return sb.ToString();
	}

	public static List<string[]> ReadTsv(string path)
	{
		List<string[]> list = new List<string[]>();
		foreach (string line in File.ReadAllLines(path, Encoding.UTF8))
		{
			if (line.Length == 0 || line[0] == '#')
			{
				continue;
			}
			string[] f = line.Split('\t');
			for (int i = 0; i < f.Length; i++)
			{
				f[i] = Unesc(f[i]);
			}
			list.Add(f);
		}
		return list;
	}
}
