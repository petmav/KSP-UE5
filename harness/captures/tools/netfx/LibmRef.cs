// .NET Framework 4.8 x64 reference for CAPTURE-SPEC C7. Compiles HarnessCaptureC7.cs + HarnessCaptureJson.cs
// (copied verbatim from decompiled/KSP-Decompiled branch `profiling`, Assembly-CSharp/PhysProf/) with this Main.
//   LibmRef.exe gen <inDir>            -> <inDir>/libm_inputs.tsv (Specials + libm_base_inputs.tsv + System.Random(7)), f32_inputs.tsv (System.Random(11))
//   LibmRef.exe run <inDir> <out.json> -> libm_dotnet.json (same tables as the in-game libm_mono.json)
//   LibmRef.exe fmt <in.txt> <out.txt> -> .NET ToString("G6".."G9"/"G15".."G17") candidates (used by normalize.py)
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;

public static class LibmRef
{
    public static int Main(string[] args)
    {
        if (args.Length >= 2 && args[0] == "gen") return Gen(args[1]);
        if (args.Length >= 3 && args[0] == "run") return RunAll(args[1], args[2]);
        if (args.Length >= 3 && args[0] == "fmt") return Fmt(args[1], args[2]);
        Console.Error.WriteLine("usage: LibmRef gen <inDir> | run <inDir> <out.json>");
        return 2;
    }

    static int Gen(string inDir)
    {
        var vals = new List<double>();
        var srcs = new List<string>();
        var seen = new HashSet<long>();
        Action<double, string> add = (x, s) =>
        {
            long b = BitConverter.DoubleToInt64Bits(x);
            if (!seen.Add(b)) return;
            vals.Add(x); srcs.Add(s);
        };
        foreach (double x in HcC7.Specials()) add(x, "special");
        foreach (string[] f in HcJson.ReadTsv(Path.Combine(inDir, "libm_base_inputs.tsv"))) add(HcJson.FromHex64(f[0]), f[1]);
        List<double> r7 = HcC7.GenLibmRandom();
        for (int k = 0; k < r7.Count; k++) add(r7[k], "System.Random(7)#" + k.ToString(CultureInfo.InvariantCulture));
        var sb = new StringBuilder();
        sb.Append("# libm inputs: HcC7.Specials, libm_base_inputs.tsv (orbit_propagation), 1000 x System.Random(7).NextDouble()*200-100 (.NET Framework); unique by bits\n");
        for (int i = 0; i < vals.Count; i++) sb.Append(HcJson.Hex64(vals[i])).Append('\t').Append(srcs[i].Replace("\\", "\\\\").Replace("\t", "\t")).Append('\n');
        File.WriteAllText(Path.Combine(inDir, "libm_inputs.tsv"), sb.ToString(), new UTF8Encoding(false));
        float[][] F = HcC7.GenF32Inputs();
        sb.Clear();
        sb.Append("# a b c d e x (float32 hex) from System.Random(11) (.NET Framework), see HcC7.GenF32Inputs\n");
        foreach (float[] r in F) sb.Append(string.Join("\t", Array.ConvertAll(r, HcJson.Hex32))).Append('\n');
        File.WriteAllText(Path.Combine(inDir, "f32_inputs.tsv"), sb.ToString(), new UTF8Encoding(false));
        Console.WriteLine("libm_inputs.tsv: " + vals.Count + " values; f32_inputs.tsv: " + F.Length + " rows");
        return 0;
    }

    // fmt: for each input line "f <hex32>" / "d <hex64>" write the .NET Framework ToString("G6".."G9") / ("G15".."G17")
    // candidates (tab-separated); normalize.py applies the harness ExactNumber round-trip choice to them.
    static int Fmt(string inFile, string outFile)
    {
        var sb = new StringBuilder();
        foreach (string line in File.ReadAllLines(inFile))
        {
            if (line.Length == 0) { sb.Append('\n'); continue; }
            string[] f = line.Split(' ');
            if (f[0] == "f")
            {
                float v = HcJson.FromHex32(f[1]);
                for (int p = 6; p <= 9; p++) sb.Append(p == 6 ? "" : "\t").Append(v.ToString("G" + p, CultureInfo.InvariantCulture));
            }
            else
            {
                double v = HcJson.FromHex64(f[1]);
                for (int p = 15; p <= 17; p++) sb.Append(p == 15 ? "" : "\t").Append(v.ToString("G" + p, CultureInfo.InvariantCulture));
            }
            sb.Append('\n');
        }
        File.WriteAllText(outFile, sb.ToString(), new UTF8Encoding(false));
        return 0;
    }

    static int RunAll(string inDir, string outFile)
    {
        var sw = System.Diagnostics.Stopwatch.StartNew();
        HcJson.GFile g = HcC7.Build("libm_dotnet", inDir, Console.WriteLine);
        string exe = Assembly.GetExecutingAssembly().Location;
        string sha;
        using (var h = SHA256.Create()) sha = BitConverter.ToString(h.ComputeHash(File.ReadAllBytes(exe))).Replace("-", "").ToLowerInvariant();
        object[] da = Assembly.GetExecutingAssembly().GetCustomAttributes(typeof(System.Diagnostics.DebuggableAttribute), false);
        g.P("runtime", ".NET Framework CLR " + Environment.Version + (Environment.Is64BitProcess ? " x64" : " x86") + " (RyuJIT)");
        g.P("pointer_size", IntPtr.Size);
        g.P("os", Environment.OSVersion.ToString());
        g.P("culture", CultureInfo.CurrentCulture.Name);
        g.P("tool", "harness/captures/tools/netfx/LibmRef.exe");
        g.P("tool_sha256", sha);
        g.P("tool_jit", da.Length == 0 ? "no DebuggableAttribute (optimized)" : "IsJITOptimizerDisabled=" + ((System.Diagnostics.DebuggableAttribute)da[0]).IsJITOptimizerDisabled);
        g.P("utc", DateTime.UtcNow.ToString("o", CultureInfo.InvariantCulture));
        g.P("capture_seconds", sw.Elapsed.TotalSeconds);
        g.Write(outFile);
        Console.WriteLine("wrote " + outFile + " rows=" + g.RowCount);
        return 0;
    }
}
