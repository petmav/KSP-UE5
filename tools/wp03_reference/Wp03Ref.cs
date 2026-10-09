// WP-03 reference data: runs KSP 1.12.5's own IL (Assembly-CSharp as built by the golden harness, read-only copy) on the
// .NET Framework 4.8 CLR and writes tests/data/wp03_reference.tsv for core/tests/test_wp03_reference.cpp.
// C# 5 (compiled by the .NET Framework csc.exe; see gen_wp03_reference.ps1). Private tool, never distributed.
//
// Row format (tab-separated, '#' lines are comments): KIND <tab> name <tab> inputs... <tab> output
//   doubles are "x" + 16 hex digits of the IEEE bits, floats "f" + 8 hex digits, strings escaped (\\ \t \n \r),
//   a C# null string is <null>, an exception is EXC:<TypeName>.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Serialization;
using System.Text;
using System.Threading;
using KSP.Localization;
using UnityEngine;

[assembly: System.Runtime.Versioning.TargetFramework(".NETFramework,Version=v4.8")]

public enum RefEnum { Big = 100, [Description("Nothing")] Zero = 0, [Description("First")] One = 1, [Description("Primero")] Uno = 1, Two = 2 }

[Flags]
public enum RefFlags { None = 0, A = 1, B = 2, C = 4, AB = 3 }

public enum RefByteEnum : byte { X = 1 }

public sealed class LogSink : ILogHandler
{
    public readonly List<string> Lines = new List<string>();
    public void LogFormat(LogType logType, UnityEngine.Object context, string format, params object[] args)
    {
        string msg = (args == null || args.Length == 0) ? format : string.Format(CultureInfo.InvariantCulture, format, args);
        Lines.Add(logType + ": " + msg);
    }
    public void LogException(Exception exception, UnityEngine.Object context) { Lines.Add("Exception: " + exception.GetType().Name); }
}

public sealed class Listener
{
    public string id;
    public List<string> trace;
    public void OnInt(int v) { trace.Add(id + ":" + v); }
}

// Instance methods as listeners: the C# 5 compiler emits non-capturing lambdas as STATIC methods (Roslyn, which built
// KSP, uses an instance closure), and EventData.Add throws NullReferenceException for a static target.
public sealed class Thrower { public void OnInt(int v) { throw new InvalidOperationException("boom"); } }
public sealed class Ret
{
    public double d; public float f;
    public double D() { return d; }
    public float F() { return f; }
}
public sealed class Mod
{
    public int Twice(int v, string s) { return v * 2; }
    public int AddLen(int v, string s) { return v + s.Length; }
}

public static class Wp03Ref
{
    static TextWriter W;
    static LogSink Log;
    const BindingFlags All = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static;

    static string D(double v) { return "x" + BitConverter.DoubleToInt64Bits(v).ToString("X16"); }
    static string F(float v) { return "f" + BitConverter.ToInt32(BitConverter.GetBytes(v), 0).ToString("X8"); }
    static string S(string s)
    {
        if (s == null) return "<null>";
        return s.Replace("\\", "\\\\").Replace("\t", "\\t").Replace("\n", "\\n").Replace("\r", "\\r");
    }
    static string B(bool b) { return b ? "1" : "0"; }
    static string I(int i) { return i.ToString(CultureInfo.InvariantCulture); }
    static string V3(Vector3 v) { return F(v.x) + "," + F(v.y) + "," + F(v.z); }
    static void Row(params string[] cols) { W.WriteLine(string.Join("\t", cols)); }
    static string Exc(Exception e)
    {
        while (e is TargetInvocationException && e.InnerException != null) e = e.InnerException;
        return "EXC:" + e.GetType().Name;
    }

    public static int Main(string[] args)
    {
        // HighLogic.cs:125 culture "en" with Mono's infinity/NaN symbols (as the harness does, KspEnv.InitCulture)
        CultureInfo c = (CultureInfo)CultureInfo.CreateSpecificCulture("en").Clone();
        c.NumberFormat.PositiveInfinitySymbol = "Infinity";
        c.NumberFormat.NegativeInfinitySymbol = "-Infinity";
        c.NumberFormat.NaNSymbol = "NaN";
        Thread.CurrentThread.CurrentCulture = c;
        Log = new LogSink();
        Debug.unityLogger.logHandler = Log;

        W = new StreamWriter(args[0], false, new UTF8Encoding(false));
        ((StreamWriter)W).NewLine = "\n";
        Row("# WP-03 reference data from KSP 1.12.5 IL on .NET Framework " + Environment.Version + " (tools/wp03_reference)");
        string dictionaryCfg = args[1];

        UtilMathRows();
        SetUpLocalizer(dictionaryCfg);
        DateRows();
        KspUtilRows();
        StringRows();
        SimplexRows();
        EventRows();
        CollectionRows();
        EnumRows();
        W.Flush();
        W.Close();
        return 0;
    }

    // ---- UtilMath (double precision) ------------------------------------------------------------------------------------
    static readonly double[] Xs = { 0.0, -0.0, 0.5, -0.5, 1.0, -1.0, 2.0, -2.0, 0.999, 1.001, 3.14159, -7.25, 10.0, 100.0,
        1e-300, 1e10, -1e10, 710.0, 0.1, 6.5, 361.0, -361.0, 720.5, double.NaN, double.PositiveInfinity, double.NegativeInfinity };

    static void UtilMathRows()
    {
        foreach (double x in Xs)
        {
            Row("UM", "ASinh", D(x), D(UtilMath.ASinh(x)));
            Row("UM", "ACosh", D(x), D(UtilMath.ACosh(x)));
            Row("UM", "ATanh", D(x), D(UtilMath.ATanh(x)));
            Row("UM", "ACoth", D(x), D(UtilMath.ACoth(x)));
            Row("UM", "ASech", D(x), D(UtilMath.ASech(x)));
            Row("UM", "ACsch", D(x), D(UtilMath.ACsch(x)));
            Row("UM", "Sech", D(x), D(UtilMath.Sech(x)));
            Row("UM", "Csch", D(x), D(UtilMath.Csch(x)));
            Row("UM", "Coth", D(x), D(UtilMath.Coth(x)));
            Row("UM", "Clamp01", D(x), D(UtilMath.Clamp01(x)));
            Row("UM", "Clamp", D(x), D(UtilMath.Clamp(x, -1.5, 2.5)));
            Row("UM", "ClampRadians", D(x), Math.Abs(x) < 1e9 ? D(UtilMath.ClampRadians(x)) : "skip");
            Row("UM", "ClampRadiansPI", D(x), Math.Abs(x) < 1e9 ? D(UtilMath.ClampRadiansPI(x)) : "skip");
            Row("UM", "ClampRadiansTwoPI", D(x), D(UtilMath.ClampRadiansTwoPI(x)));
            Row("UM", "ClampDegrees360", D(x), D(UtilMath.ClampDegrees360(x)));
            Row("UM", "ClampDegrees180", D(x), D(UtilMath.ClampDegrees180(x)));
            Row("UM", "WrapAroundD", D(x), D(UtilMath.WrapAround(x, -1.0, 3.0)));
            Row("UM", "Lerp", D(x), D(UtilMath.Lerp(2.0, 7.0, x)));
            Row("UM", "LerpUnclamped", D(x), D(UtilMath.LerpUnclamped(2.0, 7.0, x)));
            Row("UM", "InverseLerp", D(x), D(UtilMath.InverseLerp(2.0, 7.0, x)));
            Row("UM", "DegreesToRadians", D(x), D(UtilMath.DegreesToRadians(x)));
            Row("UM", "RadiansToDegrees", D(x), D(UtilMath.RadiansToDegrees(x)));
            Row("UM", "RoundToPlaces", D(x), D(UtilMath.RoundToPlaces(x, 3)));
            Row("UM", "Approximately", D(x), B(UtilMath.Approximately(x, 1.0, 1e-3)));
            string fl;
            try { fl = D(UtilMath.Flatten(x, 2.0, 1.5)); } catch (Exception e) { fl = Exc(e); }
            Row("UM", "Flatten", D(x), fl);
            foreach (double m in new double[] { 0.25, 2.0 })
                Row("UM", "Flatten2", D(x), D(m), (double.IsNaN(x) ? "skip" : D(UtilMath.Flatten(x, m, 0.5))));
        }
        int[] ints = { 0, 1, -1, 7, -7, 12, -13, int.MaxValue, int.MinValue, int.MinValue + 1 };
        foreach (int v in ints)
        {
            string r;
            try { r = I(UtilMath.WrapAround(v, 0, 5)); } catch (Exception e) { r = Exc(e); }
            Row("UM", "WrapAroundI", I(v), I(0), I(5), r);
            try { r = I(UtilMath.WrapAround(v, -3, -1)); } catch (Exception e) { r = Exc(e); }
            Row("UM", "WrapAroundI", I(v), I(-3), I(-1), r);
            Row("UM", "IsPowerOfTwo", I(v), B(UtilMath.IsPowerOfTwo(v)));
            if (v != 0) Row("UM", "IsDivisible", I(v), I(12), B(UtilMath.IsDivisible(v, 12)));
        }
        Row("UM", "Max", D(UtilMath.Max(1.0, 5.0, double.NaN, 3.0)));
        Row("UM", "Min", D(UtilMath.Min(1.0, -5.0, double.NaN, 3.0)));
        Row("UM", "MaxEmpty", D(UtilMath.Max()));
        double[] vals = { 1.0, 7.0, 7.0, -2.0, double.NaN, -2.0 };
        double mx, mn;
        Row("UM", "MaxFrom", I(UtilMath.MaxFrom<int>(i => vals[i], out mx, 0, 1, 2, 3, 4, 5)), D(mx));
        Row("UM", "MinFrom", I(UtilMath.MinFrom<int>(i => vals[i], out mn, 0, 1, 2, 3, 4, 5)), D(mn));
        // BSPSolver(double) on a few functions / starts
        double[] starts = { 2.0, 1.0, 0.25, 3.5 };
        foreach (double s0 in starts)
        {
            double v0 = s0;
            int it = UtilMath.BSPSolver(ref v0, 0.5, x => x * x - 2.0, 0.0, 4.0, 1e-9, 100);
            Row("UM", "BSPSolver", D(s0), I(it), D(v0));
            double v1 = s0;
            int it1 = UtilMath.BSPSolver(ref v1, 0.75, x => Math.Sin(x) - 0.5, 0.0, 4.0, 1e-6, 12);
            Row("UM", "BSPSolverSin", D(s0), I(it1), D(v1));
        }
        // SphereIntersection / AngleBetween on a fixed vector set
        Vector3d[] vs = { new Vector3d(1, 0, 0), new Vector3d(0, 2, 0), new Vector3d(-3, 4, 5), new Vector3d(1e-8, 1, 0),
            new Vector3d(-1, -1e-12, 0), new Vector3d(0, 0, 0), new Vector3d(600000, 12, -7.5) };
        for (int a = 0; a < vs.Length; a++)
            for (int b = 0; b < vs.Length; b++)
            {
                Row("UM", "AngleBetween", I(a), I(b), D(UtilMath.AngleBetween(vs[a], vs[b])));
                double t;
                bool hit = UtilMath.SphereIntersection(2.5, vs[a], vs[b], out t, (a + b) % 2 == 0);
                Row("UM", "SphereIntersection", I(a), I(b), B(hit), D(t));
            }
    }

    // ---- Localizer with the shipped en-us dictionary ---------------------------------------------------------------------
    static readonly string[] BuiltInTags = { "#autoLOC_7003243", "#autoLOC_7003244", "#autoLOC_7003272", "#autoLOC_7003273",
        "#autoLOC_7003274", "#autoLOC_7003275", "#autoLOC_6002317", "#autoLOC_6002318", "#autoLOC_6002319", "#autoLOC_6002320",
        "#autoLOC_6002321", "#autoLOC_6002322", "#autoLOC_6002323", "#autoLOC_6002324", "#autoLOC_6002325", "#autoLOC_6002326",
        "#autoLOC_6002327", "#autoLOC_6002328", "#autoLOC_6002329", "#autoLOC_6002330", "#autoLOC_6002331", "#autoLOC_6002332",
        "#autoLOC_6002333", "#autoLOC_6002334", "#autoLOC_6002335", "#autoLOC_6002336", "#autoLOC_6002337", "#autoLOC_6002338",
        "#autoLOC_6002339", "#autoLOC_6002340", "#autoLOC_6002341", "#autoLOC_6002342", "#autoLOC_6002343", "#autoLOC_6002344",
        "#autoLOC_6002345", "#autoLOC_7003005", "#autoLOC_462439" };

    static void SetUpLocalizer(string dictionaryCfg)
    {
        Localizer loc = (Localizer)FormatterServices.GetUninitializedObject(typeof(Localizer));
        typeof(Localizer).GetField("tagValues", All).SetValue(loc, new Dictionary<string, string>());
        typeof(Localizer).GetField("_currentLanguage", All).SetValue(loc, "en-us");
        typeof(Localizer).GetField("missingKeyPrefix", All).SetValue(loc, "Localization.Format Missed Key in:");
        typeof(Localizer).GetField("outputForReplaceTags", All).SetValue(loc, new List<string>());
        typeof(Localizer).GetProperty("Instance", All).GetSetMethod(true).Invoke(null, new object[] { loc });
        ConfigNode root = ConfigNode.Load(dictionaryCfg);
        ConfigNode en = root.GetNode("Localization").GetNode("en-us");
        typeof(Localizer).GetMethod("AddTagValuesFromNode", All).Invoke(loc, new object[] { en });
        typeof(KSPUtil).GetMethod("CacheLocalStrings", All).Invoke(null, null);
        foreach (string t in BuiltInTags) Row("TAG", t, S(Localizer.GetStringByTag(t)));
        Row("TAG", "count", I(Localizer.TagsLength));
        // Lingoona managed fallback through Localizer.Format
        string[][] fmt = {
            new[] { "<<1>><<2>><<3>>", "1.23", "k", "m" }, new[] { "Hello <<1>>, <<A:name>> <<3>>.", "Jeb^M" },
            new[] { "a<<b<<1>>", "P" }, new[] { "<<Q:1>> <<x>> <<2>> <<0>>", "P", "Q" }, new[] { "#autoLOC_7003005", "#autoLOC_6002321", "4" },
            new[] { "x\\ny\\tz\\\"", "-" } };
        foreach (string[] f in fmt)
        {
            string[] ps = new string[f.Length - 1];
            Array.Copy(f, 1, ps, 0, ps.Length);
            Row("LOC", "Format", S(f[0]), S(string.Join("|", ps)), S(Localizer.Format(f[0], ps)));
        }
        Row("LOC", "Format0", S("#autoLOC_6002333"), S(Localizer.Format("#autoLOC_6002333")));
        Row("LOC", "Format0", S("plain \\n text"), S(Localizer.Format("plain \\n text")));
    }

    // ---- DefaultDateTimeFormatter --------------------------------------------------------------------------------------
    static readonly double[] Times = { 0.0, 0.5, 1.0, 59.9, 60.0, 3599.0, 3723.5, 21599.0, 21600.0, 86399.0, 86400.0,
        9201599.0, 9201600.0, 18482706.0, 31536000.0, 1e9, 1e12, 1e15, -0.5, -1.0, -61.0, -3723.5, -18482706.0, -1e9,
        2147483647.0, 2147483648.5, 1e300, double.NaN, double.PositiveInfinity, double.NegativeInfinity };

    delegate string DateCall(KSPUtil.DefaultDateTimeFormatter f);

    static void DateRow(string name, double t, string opts, DateCall call)
    {
        string r;
        try { r = S(call(new KSPUtil.DefaultDateTimeFormatter())); } catch (Exception e) { r = Exc(e); }
        Row("DATE", name, GameSettings.KERBIN_TIME ? "K" : "E", D(t), opts, r);
    }

    static void DateRows()
    {
        bool[] tf = { false, true };
        foreach (bool kerbin in tf)
        {
            GameSettings.KERBIN_TIME = kerbin;
            foreach (double t in Times)
            {
                double tt = t;
                DateRow("PrintTimeLong", tt, "-", f => f.PrintTimeLong(tt));
                foreach (bool a in tf) foreach (bool b in tf)
                {
                    bool aa = a, bb = b;
                    string o = B(a) + B(b);
                    DateRow("PrintTimeStamp", tt, o, f => f.PrintTimeStamp(tt, aa, bb));
                    DateRow("PrintTimeStampCompact", tt, o, f => f.PrintTimeStampCompact(tt, aa, bb));
                    DateRow("PrintDate", tt, o, f => f.PrintDate(tt, aa, bb));
                    DateRow("PrintDateCompact", tt, o, f => f.PrintDateCompact(tt, aa, bb));
                    foreach (bool u in tf)
                    {
                        bool uu = u;
                        DateRow("PrintDateDelta", tt, o + B(u), f => f.PrintDateDelta(tt, aa, bb, uu));
                        DateRow("PrintDateDeltaCompact", tt, o + B(u), f => f.PrintDateDeltaCompact(tt, aa, bb, uu));
                        DateRow("PrintDateDeltaCompact2", tt, o + B(u), f => f.PrintDateDeltaCompact(tt, aa, bb, uu, 2));
                    }
                }
                foreach (int voi in new[] { 1, 2, 3, 5 })
                    foreach (bool ep in tf) foreach (bool en in tf)
                    {
                        int vv = voi; bool e1 = ep, e2 = en;
                        DateRow("PrintTime", tt, I(voi) + B(ep) + B(en), f => f.PrintTime(tt, vv, e1, e2));
                    }
                foreach (bool ep in tf)
                {
                    bool e1 = ep;
                    DateRow("PrintTimeCompact", tt, B(ep), f => f.PrintTimeCompact(tt, e1));
                    DateRow("PrintDateNew", tt, B(ep), f => f.PrintDateNew(tt, e1));
                }
                // the shared static date holder after a PrintDate call
                KSPUtil.DefaultDateTimeFormatter g = new KSPUtil.DefaultDateTimeFormatter();
                try
                {
                    g.PrintDate(tt, true, true);
                    Row("DATE", "Holder", kerbin ? "K" : "E", D(tt), "-", I(g.GetUTYear()) + "," + I(g.GetUTDay()) + "," +
                        I(g.GetUTHour()) + "," + I(g.GetUTMinute()) + "," + I(g.GetUTSecond()));
                    int y, d, h, m, s;
                    g.GetTimeComponents(tt, out y, out d, out h, out m, out s);
                    Row("DATE", "GetTimeComponents", kerbin ? "K" : "E", D(tt), "-", I(y) + "," + I(d) + "," + I(h) + "," + I(m) + "," + I(s));
                }
                catch (Exception) { }
            }
        }
        GameSettings.KERBIN_TIME = true;
        // a cache sequence on one formatter instance
        KSPUtil.DefaultDateTimeFormatter one = new KSPUtil.DefaultDateTimeFormatter();
        double[] seq = { 30.0, 30.0, 3723.5, 3723.9, 3724.0, -0.5, 18482706.0 };
        foreach (double t in seq)
        {
            Row("DATESEQ", "PrintDate", D(t), S(one.PrintDate(t, false, false)));
            Row("DATESEQ", "PrintDateCompact", D(t), S(one.PrintDateCompact(t, true, false)));
            Row("DATESEQ", "PrintTimeStampCompact", D(t), S(one.PrintTimeStampCompact(t, false, false)));
        }
    }

    // ---- KSPUtil misc ----------------------------------------------------------------------------------------------------
    static void KspUtilRows()
    {
        double[] amounts = { 0.0, 1.0, 1234.5, 0.000123, 999999.0, 12345678.0, 123456.0, 1e30, -1e30, 1e-30, -0.0042, 5e-25,
            0.9995, 999.5, 1e-24, double.PositiveInfinity, double.NaN, -987654.321 };
        foreach (double a in amounts)
            foreach (int sig in new[] { 0, 1, 2, 3, 4 })
                foreach (bool lp in new[] { false, true })
                {
                    string r;
                    try { r = S(KSPUtil.PrintSI(a, "m", sig, lp)); } catch (Exception e) { r = Exc(e); }
                    Row("SI", D(a), I(sig), B(lp), r);
                }
        double[] angles = { 0.0, 12.5, -0.0001, -123.456789, 89.99999, 179.999999, -0.5 / 3600.0, 45.0125, 1e-12, -90.0 };
        foreach (double a in angles)
            foreach (bool m in new[] { false, true }) foreach (bool s in new[] { false, true })
            {
                Row("COORD", "Lat", D(a), B(m) + B(s), S(KSPUtil.PrintLatitude(a, m, s)));
                Row("COORD", "Lon", D(a), B(m) + B(s), S(KSPUtil.PrintLongitude(a, m, s)));
            }
        Row("COORD", "Coords", D(1.0), D(2.0), S(KSPUtil.PrintCoordinates(1.0, 2.0, true)) + "|" + S(KSPUtil.PrintCoordinates(-1.5, -2.25, false)));
        double[] nums = { 3.14159, 2.5, -0.005, 1e20, 0.0, double.PositiveInfinity, double.NegativeInfinity, double.NaN };
        foreach (double n in nums)
            foreach (string f in new[] { "F2", "F0", "G", "G3", "R", "" })
            {
                Row("LN", "D", D(n), S(f), S(KSPUtil.LocalizeNumber(n, f)));
                Row("LN", "F", F((float)n), S(f), S(KSPUtil.LocalizeNumber((float)n, f)));
            }
    }

    // ---- strings / ids ---------------------------------------------------------------------------------------------------
    static void StringRows()
    {
        string[] strs = { "", "a", "abc", "abcd", "Kerbal Space Program", "mk1pod_4294704370", "\u00e9t\u00e9", "\U0001F680x",
            "longer string with some punctuation: {}[]!?", "ModuleEngineFX" };
        foreach (string s in strs) Row("STR", "Hash35", S(s), I(StringExtensions.GetHashCode_Net35(s)));
        string[] camel = { "", "camelCaseString", "AaBbCcDd", "ABC", "\u00e9tat\u00c9t\u00e9", "EngineFX", "aB", "x", "IntakeAirTest",
            "ModuleRCSFX", "some text", "PartModule2Test" };
        foreach (string s in camel) Row("STR", "Camel", S(s), S(s.PrintSpacedStringFromCamelcase()));
        string[] mods = { "ModuleEngineFX", "ModuleAblator", "LiquidFuel", "ModuleWheelBase", "Module", "SpaceplaneHangar",
            "ModuleModuleX", "moduleLower" };
        foreach (string s in mods)
        {
            Row("STR", "ModuleName", S(s), S(KSPUtil.PrintModuleName(s)));
            Row("STR", "LocalizedModuleName", S(s), S(KSPUtil.PrintLocalizedModuleName(s)));
        }
        string[] san = { "a/b:c", "  ", "CON", "  x.y  ", "my.craft", "Clock$", "com5", "lpt1", "na<me>\"?*|", "" };
        foreach (string s in san)
        {
            Row("STR", "Sanitize_", S(s), S(KSPUtil.SanitizeString(s, '_', true)));
            Row("STR", "SanitizeDash", S(s), S(KSPUtil.SanitizeString(s, '-', false)));
            Row("STR", "SanitizeFilename", S(s), S(KSPUtil.SanitizeFilename(s)));
            Row("STR", "CheckDOSName", S(s), B(KSPUtil.CheckDOSName(s)));
        }
        string[] inst = { "mk1pod (Clone)", "part(Clone)", "a (b)", "plain", "x(clone)(Clone)", "(Clone)" };
        foreach (string s in inst) Row("STR", "SanitizeInstanceName", S(s), S(KSPUtil.SanitizeInstanceName(s)));
        string[] files = { "C:\\x\\y\\craft.v2.craft", "C:\\dir\\name.", "C:\\noext", "C:\\a.b\\c", "C:\\.hidden" };
        foreach (string s in files) Row("STR", "StripFileExtension", S(s), S(KSPUtil.StripFileExtension(new FileInfo(s))));
        string[][] app = { new[] { "", "size0" }, new[] { "size0", "srf" }, new[] { "size0, srf", "srf" }, new[] { "abc", "" } };
        foreach (string[] a in app) Row("STR", "AppendValueToString", S(a[0]), S(a[1]), S(KSPUtil.AppendValueToString(a[0], a[1], ',')));
        string[][] fmts = { new[] { "{0}-{1,4}|{1,-4}|{{x}}", "a", "bc" }, new[] { "{0:F2}", "s" }, new[] { "{1}", "a" },
            new[] { "}", "a" }, new[] { "{0,3}{0}", "\u00e9" }, new[] { "x{ 0 }y", "a" }, new[] { "{0 ,2}", "a" } };
        foreach (string[] f in fmts)
        {
            object[] ps = new object[f.Length - 1];
            Array.Copy(f, 1, ps, 0, ps.Length);
            string r;
            try { r = S(StringBuilderCache.Format(f[0], ps)); } catch (Exception e) { r = Exc(e); }
            Row("STR", "SBFormat", S(f[0]), S(string.Join("|", f.Skip(1).ToArray())), r);
        }
        // versions (Versioning.fetch from the shipped asset: 1.12.5)
        Versioning ver = (Versioning)FormatterServices.GetUninitializedObject(typeof(Versioning));
        ver.versionMajor = 1; ver.versionMinor = 12; ver.revision = 5; ver.buildID = 3190;
        typeof(Versioning).GetField("fetch", All).SetValue(null, ver);
        string[] versions = { "1.12.5", "1.12.6", "1.13.0", "0.90.0", "1.12", "a.b.c", "1.0.0", "1.12.4", "2.0.0", "1.12.5.1", " 1.2.3" };
        foreach (string v in versions)
        {
            string r;
            try { r = KSPUtil.CheckVersion(v, 1, 0, 0).ToString(); } catch (Exception e) { r = Exc(e); }
            Row("STR", "CheckVersion", S(v), r);
            try { r = KSPUtil.CheckVersion(v, 1, 12, 5).ToString(); } catch (Exception e) { r = Exc(e); }
            Row("STR", "CheckVersion1125", S(v), r);
        }
        string[] pids = { "mk1pod_4294704370", "nounderscore", "fuelTank.long_123", "a_b_c", "_x" };
        foreach (string p in pids)
        {
            string r;
            try { r = S(KSPUtil.GetPartName(p)); } catch (Exception e) { r = Exc(e); }
            Row("STR", "GetPartName", S(p), r);
            Row("STR", "GetLinkID", S(p), S(KSPUtil.GetLinkID(p)));
        }
        string[] attn = { "top,fuelTank_4294705000", "bottom,fuelTank_42_0|1.5|0", "srfAttach,part_1,mesh,1|2|3,0|0|0,1|0|0",
            "srfAttach,part_1,mesh,1|2|3", "srfAttach,part_1,mesh", "top", "top,part_7_1|2|3_0|0|0", "top,part_7_1|2|3_4|5|6_7|8|9_1|1|1",
            "x,p_1_1|2" };
        foreach (string a in attn)
        {
            string nodeID = "n", partID = "p", mesh = "m";
            Vector3 pos = new Vector3(9, 9, 9), rot = new Vector3(9, 9, 9), sec = new Vector3(9, 9, 9);
            string r;
            try
            {
                bool ok = KSPUtil.GetAttachNodeInfo(a, ref nodeID, ref partID, ref pos, ref mesh, ref rot, ref sec);
                r = B(ok) + "|" + S(nodeID) + "|" + S(partID) + "|" + S(mesh) + "|" + V3(pos) + "|" + V3(rot) + "|" + V3(sec);
            }
            catch (Exception e) { r = Exc(e); }
            Row("STR", "AttachMesh", S(a), r);
            Vector3 p2 = new Vector3(9, 9, 9), r2 = new Vector3(9, 9, 9), ap = new Vector3(9, 9, 9), ar = new Vector3(9, 9, 9);
            try
            {
                bool ok = KSPUtil.GetAttachNodeInfo(a, ref nodeID, ref partID, ref p2, ref r2, ref ap, ref ar);
                r = B(ok) + "|" + S(nodeID) + "|" + S(partID) + "|" + V3(p2) + "|" + V3(r2) + "|" + V3(ap) + "|" + V3(ar);
            }
            catch (Exception e) { r = Exc(e); }
            Row("STR", "AttachActual", S(a), r);
        }
        string[] guids = { "00000001-0000-0000-0000-000000000000", "ff000000-0000-0000-0000-000000000000",
            "0123abcd-4567-89ef-fedc-ba9876543210", "deadbeef-dead-beef-dead-beefdeadbeef", "00000000-0000-0000-0000-000000000000" };
        foreach (string g in guids)
            foreach (int seed in new[] { 0, 5, -1, 123456789, int.MinValue })
                Row("STR", "SuperSeed", S(g), I(seed), I(KSPUtil.GenerateSuperSeed(new Guid(g), seed)));
        // double-precision vector helpers
        Vector3d[] v3 = { new Vector3d(1, 2, 3), new Vector3d(0.1, -1e-300, 1e300), new Vector3d(-0.0, double.NaN, double.PositiveInfinity) };
        foreach (Vector3d v in v3)
        {
            Row("VEC", "WriteVector3d", D(v.x), D(v.y), D(v.z), S(KSPUtil.WriteVector(v)) + "|" + S(KSPUtil.WriteVector(v, " ; ")));
            QuaternionD q = new QuaternionD(v.x, v.y, v.z, 0.5);
            Row("VEC", "WriteQuaternionD", D(v.x), D(v.y), D(v.z), S(KSPUtil.WriteQuaternion(q)) + "|" + S(KSPUtil.WriteQuaternion(q, "/")));
        }
        string[] vstr = { "1,2,3", "1.5, -2e300 ,3", "1,2", "a,b,c", "1;2;3", "0.1,0.2,0.30000000000000004,4", "1e400,0,0", " 7 , 8 ,9,10" };
        foreach (string s in vstr)
        {
            string r;
            try { Vector3d v = KSPUtil.ParseVector3d(s); r = D(v.x) + "," + D(v.y) + "," + D(v.z); } catch (Exception e) { r = Exc(e); }
            Row("VEC", "ParseVector3d", S(s), r);
            try { Vector3d v = KSPUtil.ParseVector3d(s, ';'); r = D(v.x) + "," + D(v.y) + "," + D(v.z); } catch (Exception e) { r = Exc(e); }
            Row("VEC", "ParseVector3dSemi", S(s), r);
            try { QuaternionD q = KSPUtil.ParseQuaternionD(s); r = D(q.x) + "," + D(q.y) + "," + D(q.z) + "," + D(q.w); } catch (Exception e) { r = Exc(e); }
            Row("VEC", "ParseQuaternionD", S(s), r);
            string[] p = s.Split(',');
            if (p.Length >= 3)
            {
                try { Vector3d v = KSPUtil.ParseVector3d(p[0], p[1], p[2]); r = D(v.x) + "," + D(v.y) + "," + D(v.z); } catch (Exception e) { r = Exc(e); }
                Row("VEC", "ParseVector3dParts", S(s), r);
                try { Vector3 v = KSPUtil.ParseVector3(p[0], p[1], p[2]); r = V3(v); } catch (Exception e) { r = Exc(e); }
                Row("VEC", "ParseVector3Parts", S(s), r);
            }
        }
        Row("STR", "WriteArrayI", S(KSPUtil.WriteArray(new int[] { 1, -2, 3 })));
        Row("STR", "WriteArrayD", S(KSPUtil.WriteArray(new double[] { 0.1, 1e20, -0.0, double.NaN })));
        Row("STR", "WriteArrayF", S(KSPUtil.WriteArray(new float[] { 0.1f, 3e-7f })));
        Row("STR", "WriteArrayB", S(KSPUtil.WriteArray(new bool[] { true, false })));
        Row("STR", "PrintCollection", S(KSPUtil.PrintCollection(new List<string> { "a", "b", "c" }, ", ")));
    }

    // ---- Simplex (double; default permutation tables, no seeding: UnityEngine.Random is native) ------------------------
    static void SimplexRows()
    {
        double[] cs = { 0.0, 0.1, -0.1, 1.0, -1.0, 1.7, -3.3, 12.25, 100.5, -250.75, 1e5 + 0.3 };
        foreach (double x in cs)
            foreach (double y in new[] { 0.0, 0.5, -2.25 })
            {
                double z = x * 0.37 - y;
                Row("SIMPLEX", "Static", D(x), D(y), D(z), D(SimplexNoise.noise(x, y, z)));
                Row("SIMPLEX", "Octaves", D(x), D(y), D(z), D(SimplexNoise.noise(4.0, 0.5, 0.01, x, y, z)));
            }
        Simplex s = new Simplex();
        s.octaves = 3.5; s.persistence = 0.6; s.frequency = 0.05;
        foreach (double x in cs)
        {
            Row("SIMPLEX", "Instance", D(x), D(x * 2), D(-x), D(s.noise(x, x * 2, -x)));
            Row("SIMPLEX", "InstanceNorm", D(x), D(x * 2), D(-x), D(s.noiseNormalized(x, x * 2, -x)));
        }
    }

    // ---- event bus scenarios on KSP's EventData ------------------------------------------------------------------------
    static string Trace(List<string> t) { return string.Join(" ", t.ToArray()); }

    static void EventRows()
    {
        List<string> trace = new List<string>();
        Listener a = new Listener { id = "a", trace = trace }, b = new Listener { id = "b", trace = trace },
            c = new Listener { id = "c", trace = trace }, d = new Listener { id = "d", trace = trace };
        // order + Remove takes the last match
        EventData<int> ev = new EventData<int>("ref.order");
        ev.Add(a.OnInt); ev.Add(b.OnInt); ev.Add(c.OnInt); ev.Add(a.OnInt);
        ev.Fire(5);
        ev.Remove(a.OnInt);
        ev.Fire(6);
        Row("EVT", "Order", S(Trace(trace)));
        // R1: removal / add during Fire
        trace.Clear();
        EventData<int> r1 = new EventData<int>("ref.r1");
        r1.Add(a.OnInt); r1.Add(b.OnInt);
        r1.Add(v => { trace.Add("c:" + v); r1.Remove(a.OnInt); r1.Add(b.OnInt); });
        r1.Fire(1); r1.Fire(2);
        Row("EVT", "R1", S(Trace(trace)));
        // R2: nested fire of the same event
        trace.Clear();
        EventData<int> r2 = new EventData<int>("ref.r2");
        bool fired2 = false;
        r2.Add(a.OnInt);
        r2.Add(v => { trace.Add("b:" + v); if (!fired2) { fired2 = true; r2.Fire(v + 10); } });
        r2.Add(c.OnInt);
        r2.Fire(1);
        Row("EVT", "R2", S(Trace(trace)));
        // R3: removal of earlier listeners + nested fire
        trace.Clear();
        EventData<int> r3 = new EventData<int>("ref.r3");
        bool fired3 = false;
        r3.Add(a.OnInt); r3.Add(b.OnInt);
        r3.Add(v => { trace.Add("c:" + v); if (!fired3) { fired3 = true; r3.Remove(a.OnInt); r3.Remove(b.OnInt); r3.Fire(v + 10); } });
        r3.Add(d.OnInt);
        r3.Fire(1);
        Row("EVT", "R3", S(Trace(trace)));
        // R4: the refilled list is shorter than the outer index
        trace.Clear();
        EventData<int> r4 = new EventData<int>("ref.r4");
        bool fired4 = false;
        r4.Add(a.OnInt); r4.Add(b.OnInt);
        r4.Add(v => { trace.Add("c:" + v); if (!fired4) { fired4 = true; r4.Remove(a.OnInt); r4.Remove(b.OnInt); r4.Fire(v + 10); } });
        string exc = "none";
        try { r4.Fire(1); } catch (Exception e) { exc = e.GetType().Name; }
        r4.Fire(2);
        Row("EVT", "R4", S(Trace(trace)), exc);
        // exception isolation
        trace.Clear();
        int before = Log.Lines.Count;
        EventData<int> ex = new EventData<int>("ref.throw");
        ex.Add(a.OnInt);
        ex.Add(new Thrower().OnInt);
        ex.Add(c.OnInt);
        ex.Fire(1);
        Row("EVT", "Throw", S(Trace(trace)), I(Log.Lines.Count - before), S(Log.Lines[before].Substring(0, Log.Lines[before].IndexOf("Thrower") + 7)));
        // modifiers and value events
        EventDataModifier<int, string> mod = new EventDataModifier<int, string>("ref.mod");
        Mod mo = new Mod();
        mod.Add(mo.Twice);
        mod.Add(mo.AddLen);
        Row("EVT", "Modifier", I(mod.Fire(3, "xy")));
        EventDoubleMax dmax = new EventDoubleMax("ref.dmax", -1.0);
        dmax.Add(new Ret { d = 2.0 }.D); dmax.Add(new Ret { d = 5.0 }.D);
        string m1 = D(dmax.Fire());
        dmax.Add(new Ret { d = double.NaN }.D);
        Row("EVT", "DoubleMax", m1, D(dmax.Fire()));
        EventFloatMax fmax = new EventFloatMax("ref.fmax");
        fmax.Add(new Ret { f = 2.0f }.F); fmax.Add(new Ret { f = float.NaN }.F);
        Row("EVT", "FloatMax", F(fmax.Fire()));
        EventDoubleMin dmin = new EventDoubleMin("ref.dmin", 1.0);
        dmin.Add(new Ret { d = -0.0 }.D); dmin.Add(new Ret { d = 0.0 }.D);
        Row("EVT", "DoubleMin", D(dmin.Fire()));
        EventValueComparison<int> cmp = new EventValueComparison<int>("ref.cmp", -1, (x, y) => x > y);
        int three = 3, one = 1;
        cmp.Add(() => three); cmp.Add(() => one);
        cmp.Update();
        Row("EVT", "Comparison", I(cmp.value), S(cmp.eventName));
        // GameEvents registry: every event field (static, nested static classes, GameEvents.Modifiers)
        List<string> rows = new List<string>();
        foreach (FieldInfo fi in typeof(GameEvents).GetFields(All))
            if (typeof(BaseGameEvent).IsAssignableFrom(fi.FieldType))
                rows.Add(fi.Name + "\t" + S(((BaseGameEvent)fi.GetValue(null)).EventName) + "\t" + TypeText(fi.FieldType));
        foreach (Type nt in typeof(GameEvents).GetNestedTypes(All))
            foreach (FieldInfo fi in nt.GetFields(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static))
                if (typeof(BaseGameEvent).IsAssignableFrom(fi.FieldType))
                    rows.Add(nt.Name + "." + fi.Name + "\t" + S(((BaseGameEvent)fi.GetValue(null)).EventName) + "\t" + TypeText(fi.FieldType));
        foreach (FieldInfo fi in typeof(GameEvents.GameModifiers).GetFields(BindingFlags.Public | BindingFlags.Instance))
            if (typeof(BaseGameEvent).IsAssignableFrom(fi.FieldType))
                rows.Add("Modifiers." + fi.Name + "\t" + S(((BaseGameEvent)fi.GetValue(GameEvents.Modifiers)).EventName) + "\t" + TypeText(fi.FieldType));
        rows.Sort(StringComparer.Ordinal);
        foreach (string r in rows) Row("GE", r);
        Row("EVT", "FindDuplicate", B(GameEvents.FindEvent<EventData<Part>>("onPartCouple") == GameEvents.onPartDeCouple),
            B(GameEvents.FindEvent<EventData<GameEvents.FromToAction<Part, Part>>>("onPartCouple") == null));
    }

    // C# type text normalised like gen_game_events.py: generic args by simple name, nested types by their own name
    static string TypeText(Type t)
    {
        if (!t.IsGenericType) return t.Name;
        string n = t.Name.Substring(0, t.Name.IndexOf('`'));
        return n + "<" + string.Join(", ", t.GetGenericArguments().Select(x => TypeText(x)).ToArray()) + ">";
    }

    // ---- .NET collections -------------------------------------------------------------------------------------------------
    static void CollectionRows()
    {
        foreach (int n in new[] { 17, 50, 200, 1000 })
        {
            uint lcg = (uint)n * 2654435761u;
            List<KeyValuePair<int, int>> list = new List<KeyValuePair<int, int>>(n);
            for (int i = 0; i < n; i++)
            {
                lcg = lcg * 1664525u + 1013904223u;
                list.Add(new KeyValuePair<int, int>((int)(lcg >> 24) % 7, i));
            }
            list.Sort((x, y) => x.Key.CompareTo(y.Key));
            Row("SORT", I(n), string.Join(",", list.Select(p => I(p.Value)).ToArray()));
        }
        Dictionary<string, int> d = new Dictionary<string, int>();
        for (int i = 0; i < 10; i++) d.Add("k" + i, i);
        d.Remove("k3"); d.Remove("k7"); d.Remove("k0");
        d.Add("n1", 11); d.Add("n2", 12);
        d["k5"] = 55;
        d.Add("n3", 13); d.Add("n4", 14);
        Row("DICT", "Order", string.Join(",", d.Keys.ToArray()));
        Dictionary<string, int> copy = new Dictionary<string, int>(d);
        copy.Remove("k1");
        copy.Add("z", 0);
        Row("DICT", "CopyOrder", string.Join(",", copy.Keys.ToArray()));
        // DictionaryValueList / ListDictionary / BiDictionary
        DictionaryValueList<string, int> dvl = new DictionaryValueList<string, int>();
        dvl.Add("x", 1); dvl.Add("y", 2); dvl.Add("x", 3); dvl["z"] = 2; dvl.Remove("z");
        List<string> vl = new List<string>(), kl = new List<string>();
        for (int i = 0; i < dvl.Count; i++) { vl.Add(I(dvl.At(i))); kl.Add(dvl.KeyAt(i)); }
        Row("DICT", "DVL", string.Join(",", vl.ToArray()), string.Join(",", kl.ToArray()), string.Join(",", dvl.Keys.ToArray()));
        List<string> names = new List<string> { "part10", "part9", "Part2", "part9", "a1b22", "a1b3" };
        Row("LIST", "OrderByAlphaNumeric", string.Join(",", names.OrderByAlphaNumeric(s => s).ToArray()));
    }

    // ---- enums ------------------------------------------------------------------------------------------------------------
    static void EnumRows()
    {
        foreach (int v in new[] { 0, 1, 2, 7, 100 })
        {
            RefEnum e = (RefEnum)v;
            string desc;
            try { desc = S(e.Description()); } catch (Exception ex) { desc = Exc(ex); }
            Row("ENUM", "RefEnum", I(v), S(e.ToString()), desc, S(e.ToStringCached()));
        }
        Row("ENUM", "RefEnumNames", string.Join(",", Enum.GetNames(typeof(RefEnum))));
        Row("ENUM", "RefEnumDescriptions", S(string.Join(",", EnumExtensions.GetDescriptions<RefEnum>(RefEnum.Zero).ToArray())));
        foreach (int v in new[] { 0, 1, 3, 5, 7, 8, 9 })
            Row("ENUM", "RefFlags", I(v), S(((RefFlags)v).ToString()));
        string r;
        try { r = RefByteEnum.X.ToStringCached(); } catch (Exception ex) { r = Exc(ex); }
        Row("ENUM", "ByteToStringCached", r);
    }
}
