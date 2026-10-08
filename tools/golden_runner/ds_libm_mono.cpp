// libm_mono candidate: mirrors the C7 capture kernel (harness/captures/tools/netfx/src/HarnessCaptureC7.cs, a verbatim
// copy of the profiling-branch Assembly-CSharp/PhysProf/HarnessCaptureC7.cs) with kspcore's runtime layer in its
// default Mono mode. Inputs: harness/captures/in/{libm_inputs,f32_inputs,parse_tokens}.tsv.
// Golden: harness/captures/out/libm_mono.json (in-game Mono 5.11 x64).
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/dotnet/text.hpp"

using namespace ksp;
using namespace ksp::dotnet;
using golden::Cell;
using golden::ColType;

namespace {

// HcJson.ReadTsv: skip empty lines and '#' comments, split on tab, unescape \t \n \r and \x -> x
std::vector<std::vector<std::string>> ReadTsv(const std::string& path) {
    auto text = ReadAllText(path);
    if (!text) throw std::runtime_error("cannot read " + path);
    std::vector<std::vector<std::string>> rows;
    for (const std::string& line : SplitLinesLikeReadLine(*text)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f = Split(line, "\t", false);
        for (std::string& s : f) {
            if (s.find('\\') == std::string::npos) continue;
            std::string o;
            for (size_t i = 0; i < s.size(); ++i) {
                if (s[i] == '\\' && i + 1 < s.size()) {
                    const char n = s[++i];
                    o.push_back(n == 't' ? '\t' : (n == 'n' ? '\n' : (n == 'r' ? '\r' : n)));
                } else {
                    o.push_back(s[i]);
                }
            }
            s = o;
        }
        rows.push_back(std::move(f));
    }
    return rows;
}

double FromHex64(const std::string& h) {
    const uint64_t b = std::stoull(h, nullptr, 16);
    double d;
    std::memcpy(&d, &b, 8);
    return d;
}
float FromHex32(const std::string& h) {
    const uint32_t b = static_cast<uint32_t>(std::stoul(h, nullptr, 16));
    float f;
    std::memcpy(&f, &b, 4);
    return f;
}
uint64_t Bits(double d) {
    uint64_t b;
    std::memcpy(&b, &d, 8);
    return b;
}
uint32_t Bits(float f) {
    uint32_t b;
    std::memcpy(&b, &f, 4);
    return b;
}

// HcC7.RtD / RtF: Parse(s, NumberStyles.Float, invariant) gives the same bits (or both NaN)
bool RtD(const std::string& s, double x) {
    double y = 0;
    if (!TryParseDouble(s, y, Float)) return false;
    return Bits(y) == Bits(x) || (std::isnan(x) && std::isnan(y));
}
bool RtF(const std::string& s, float f) {
    float y = 0;
    if (!TryParseSingle(s, y, Float)) return false;
    return Bits(y) == Bits(f) || (std::isnan(f) && std::isnan(y));
}

}  // namespace

static int RunLibmMono(const RunnerOptions& opt) {
    const std::string in = opt.harness + "/captures/in";
    std::vector<double> xs;
    std::vector<std::string> xsrc;
    for (const auto& f : ReadTsv(in + "/libm_inputs.tsv")) {
        xs.push_back(FromHex64(f[0]));
        xsrc.push_back(f.size() > 1 ? f[1] : "");
    }
    std::vector<std::array<float, 6>> F;
    for (const auto& f : ReadTsv(in + "/f32_inputs.tsv")) {
        std::array<float, 6> row{};
        for (int k = 0; k < 6; ++k) row[static_cast<size_t>(k)] = FromHex32(f[static_cast<size_t>(k)]);
        F.push_back(row);
    }

    golden::File g;
    g.dataset = "libm_mono";
    g.description = std::string("kspcore candidate for the C7 capture (System.Math, float32 evaluation, conversions, formatting); libm backend: ") + LibmBackend() + ".";

    // ---- f64
    golden::Table t64("f64", "System.Math on double inputs.");
    t64.Col("fn", ColType::Str).Col("i", ColType::I64).Col("x", ColType::F64).Col("y", ColType::F64).Col("r", ColType::F64)
        .Col("src", ColType::Str);
    t64.key = {"fn", "i"};
    const double third = 1.0 / 3.0;
    const double twoPi = 3.141592653589793 * 2.0;
    for (size_t i = 0; i < xs.size(); ++i) {
        const double x = xs[i];
        const std::string& s = xsrc[i];
        const int64_t ii = static_cast<int64_t>(i);
        auto A = [&](const char* fn, double xv, Cell y, double r) { t64.Add({fn, ii, xv, y, r, s}); };
        A("Math.Sin", x, nullptr, Math::Sin(x));
        A("Math.Cos", x, nullptr, Math::Cos(x));
        A("Math.Tan", x, nullptr, Math::Tan(x));
        A("Math.Asin", x, nullptr, Math::Asin(x));
        A("Math.Acos", x, nullptr, Math::Acos(x));
        A("Math.Atan", x, nullptr, Math::Atan(x));
        A("Math.Atan2(x,0.7)", x, 0.7, Math::Atan2(x, 0.7));
        A("Math.Atan2(0.7,x)", x, 0.7, Math::Atan2(0.7, x));
        A("Math.Sinh", x, nullptr, Math::Sinh(x));
        A("Math.Cosh", x, nullptr, Math::Cosh(x));
        A("Math.Tanh", x, nullptr, Math::Tanh(x));
        A("Math.Exp", x, nullptr, Math::Exp(x));
        A("Math.Log", x, nullptr, Math::Log(x));
        A("Math.Log10", x, nullptr, Math::Log10(x));
        A("Math.Sqrt", x, nullptr, Math::Sqrt(x));
        A("Math.Pow(x,0.4)", x, 0.4, Math::Pow(x, 0.4));
        A("Math.Pow(|x|,1/3)", std::fabs(x), third, Math::Pow(std::fabs(x), third));
        A("Math.Pow(x,2.0)", x, 2.0, Math::Pow(x, 2.0));
        A("Math.Pow(1.0000001,x)", 1.0000001, x, Math::Pow(1.0000001, x));
        A("Math.Floor", x, nullptr, Math::Floor(x));
        A("Math.Ceiling", x, nullptr, Math::Ceiling(x));
        A("Math.Round", x, nullptr, Math::Round(x));
        A("Math.Round(AwayFromZero)", x, nullptr, Math::Round(x, MidpointRounding::AwayFromZero));
        A("Math.IEEERemainder(x,2pi)", x, twoPi, Math::IEEERemainder(x, twoPi));
        A("x%2pi", x, twoPi, std::fmod(x, twoPi));
    }

    // ---- f32 wrappers: (float)Math.F((double)f) with f = (float)x
    golden::Table t32("f32", "float32 wrappers of System.Math on f = (float)x, plus float %.");
    t32.Col("fn", ColType::Str).Col("i", ColType::I64).Col("x", ColType::F32).Col("r", ColType::F32);
    t32.key = {"fn", "i"};
    for (size_t i = 0; i < xs.size(); ++i) {
        const float f = f32store(xs[i]);
        const int64_t ii = static_cast<int64_t>(i);
        auto A = [&](const char* fn, float r) { t32.Add({fn, ii, f, r}); };
        A("(float)Math.Sin(f)", f32store(Math::Sin(fx(f))));
        A("(float)Math.Cos(f)", f32store(Math::Cos(fx(f))));
        A("(float)Math.Tan(f)", f32store(Math::Tan(fx(f))));
        A("(float)Math.Asin(f)", f32store(Math::Asin(fx(f))));
        A("(float)Math.Acos(f)", f32store(Math::Acos(fx(f))));
        A("(float)Math.Atan(f)", f32store(Math::Atan(fx(f))));
        A("(float)Math.Atan2(f,0.7f)", f32store(Math::Atan2(fx(f), fx(0.7f))));
        A("(float)Math.Exp(f)", f32store(Math::Exp(fx(f))));
        A("(float)Math.Log(f)", f32store(Math::Log(fx(f))));
        A("(float)Math.Log10(f)", f32store(Math::Log10(fx(f))));
        A("(float)Math.Sqrt(f)", f32store(Math::Sqrt(fx(f))));
        A("(float)Math.Pow(f,0.4f)", f32store(Math::Pow(fx(f), fx(0.4f))));
        A("(float)Math.Floor(f)", f32store(Math::Floor(fx(f))));
        A("(float)Math.Round(f)", f32store(Math::Round(fx(f))));
        A("f%360f", f32store(std::fmod(fx(f), 360.0)));
    }

    // ---- float32 expression probes (HarnessCaptureC7.cs Probe*), Mono rule: double expressions, rounding at stores
    golden::Table tp("f32ops", "float32 expression probes.");
    tp.Col("probe", ColType::Str).Col("i", ColType::I64).Col("a", ColType::F32).Col("b", ColType::F32).Col("c", ColType::F32)
        .Col("d", ColType::F32).Col("e", ColType::F32).Col("x", ColType::F32).Col("r", ColType::F32);
    tp.key = {"probe", "i"};
    golden::Table tw("f32ops_wide", "float32 expressions assigned to double.");
    tw.Col("probe", ColType::Str).Col("i", ColType::I64).Col("a", ColType::F32).Col("b", ColType::F32).Col("c", ColType::F32)
        .Col("r", ColType::F64);
    tw.key = {"probe", "i"};
    for (size_t i = 0; i < F.size(); ++i) {
        const auto& v = F[i];
        const float a = v[0], b = v[1], c = v[2], d = v[3], e = v[4], x = v[5];
        const int64_t ii = static_cast<int64_t>(i);
        const float poly = f32store(((fx(x) * fx(1.1f) + fx(0.3f)) * fx(x) - fx(0.7f)) / fx(3.3f));
        const float dot2 = f32store(fx(a) * fx(b) + fx(c) * fx(d) - fx(e));
        const float cast = f32store(fx(f32store(fx(f32store(fx(a) * fx(b))) + fx(f32store(fx(c) * fx(d))))) - fx(e));
        float acc = 0.0f;
        for (int k = 0; k < 16; ++k) acc = f32store(fx(acc) * fx(0.999f) + fx(a) * fx(b));
        const float t1 = f32store(fx(a) * fx(b)), t2 = f32store(fx(c) * fx(d));
        const float locals = f32store(fx(t1) + fx(t2) - fx(e));
        const float sq = f32store(std::sqrt(fx(a) * fx(a) + fx(b) * fx(b)));
        const float sqrtDiv = f32store(fx(sq) / fx(c));
        const float p = f32store(fx(a) * fx(b));
        const float cmp = (fx(a) * fx(b) + fx(c) == fx(p) + fx(c)) ? 1.0f : 0.0f;
        const float arg = f32store(fx(p) + fx(c));
        auto P = [&](const char* probe, float r) { tp.Add({probe, ii, a, b, c, d, e, x, r}); };
        P("poly", poly);
        P("dot2", dot2);
        P("cast_each", cast);
        P("acc_loop", acc);
        P("dot2_call", dot2);
        P("locals", locals);
        P("sqrt_div", sqrtDiv);
        P("compare", cmp);
        P("arg_noinline", arg);
        P("arg_inline", arg);
        auto W = [&](const char* probe, double r) { tw.Add({probe, ii, a, b, c, r}); };
        const double stored = fx(f32store(fx(a) * fx(b)));
        W("ret_noinline", stored);
        W("ret_inline", stored);
        W("field", stored);
        W("struct_field", stored);
        W("static_field", stored);
        W("local", stored);
        W("mul_wide", fx(a) * fx(b));
        W("muladd_wide", fx(a) * fx(b) + fx(c));
    }

    // ---- conversions
    golden::Table tc("conv", "Unchecked C# conversions of special doubles/floats to integers.");
    tc.Col("fn", ColType::Str).Col("i", ColType::I64).Col("x", ColType::F64).Col("r", ColType::I64).Col("err", ColType::Str);
    tc.key = {"fn", "i"};
    const double inf = std::numeric_limits<double>::infinity();
    const double cx[] = {std::numeric_limits<double>::quiet_NaN(), inf, -inf, 0.0, -0.0, 0.5, -0.5, 0.9999999, -0.9999999, 1.5,
                         2.5, -2.5, 3.5, 255.5, 256.0, -129.0, 32767.5, 32768.0, 65536.0, 2147483647.0, 2147483647.5,
                         2147483648.0, -2147483648.0, -2147483648.5, -2147483649.0, 4294967295.0, 4294967296.0, -4294967296.0,
                         9007199254740993.0, 9.2233720368547758E+18, -9.2233720368547758E+18, 1e19, 1.8446744073709552E+19,
                         -1e19, 1e20, -1e20, 1e30, 3.4028234663852886E+38, 1e300};
    for (size_t i = 0; i < std::size(cx); ++i) {
        const double x = cx[i];
        const float f = f32store(x);
        const int64_t ii = static_cast<int64_t>(i);
        auto C = [&](const char* fn, int64_t r) { tc.Add({fn, ii, x, r, nullptr}); };
        C("(int)d", ConvToInt32(x));
        C("(long)d", ConvToInt64(x));
        C("(uint)d", static_cast<int64_t>(ConvToUInt32(x)));
        C("(ulong)d", static_cast<int64_t>(ConvToUInt64(x)));
        C("(short)d", ConvToInt16(x));
        C("(byte)d", ConvToByte(x));
        C("(int)(float)d", ConvToInt32(fx(f)));
        C("(long)(float)d", ConvToInt64(fx(f)));
        C("(uint)(float)d", static_cast<int64_t>(ConvToUInt32(fx(f))));
        try {
            tc.Add({"Convert.ToInt32(d)", ii, x, int64_t(ConvertToInt32(x)), nullptr});
        } catch (const OverflowException&) {
            tc.Add({"Convert.ToInt32(d)", ii, x, nullptr, "OverflowException"});
        }
    }

    golden::Table ti("iconv", "Integer to floating-point conversions.");
    ti.Col("fn", ColType::Str).Col("i", ColType::I64).Col("v", ColType::Str).Col("r", ColType::F64);
    ti.key = {"fn", "i"};
    const uint64_t uv[] = {0ull, 1ull, 16777217ull, 9007199254740993ull, 9223372036854775807ull, 9223372036854775808ull,
                           9223372036854776833ull, 9223372036854777856ull, 18446744073709550592ull, 18446744073709551615ull,
                           18014398509481985ull, 2147483649ull};
    for (size_t i = 0; i < std::size(uv); ++i) {
        const uint64_t u = uv[i];
        const int64_t l = static_cast<int64_t>(u);
        const int64_t ii = static_cast<int64_t>(i);
        ti.Add({"(double)ulong", ii, FormatUInt64(u), ConvUInt64ToDouble(u)});
        ti.Add({"(double)(float)ulong", ii, FormatUInt64(u), fx(f32store(ConvUInt64ToDouble(u)))});
        ti.Add({"(double)long", ii, FormatInt64(l), static_cast<double>(l)});
        ti.Add({"(double)(float)long", ii, FormatInt64(l), fx(static_cast<float>(l))});
        const uint32_t ui = static_cast<uint32_t>(u);
        ti.Add({"(double)(float)uint", ii, FormatUInt64(ui), fx(static_cast<float>(ui))});
        const int32_t i32 = static_cast<int32_t>(ui);
        ti.Add({"(double)(float)int", ii, FormatInt64(i32), fx(static_cast<float>(i32))});
    }

    // ---- formatting / parsing
    golden::Table ts("tostring", "double/float formatting with InvariantCulture.");
    ts.Col("fn", ColType::Str).Col("i", ColType::I64).Col("x", ColType::F64).Col("s", ColType::Str).Col("rt", ColType::Bool);
    ts.key = {"fn", "i"};
    const size_t nts = std::min<size_t>(xs.size(), 600);
    for (size_t i = 0; i < nts; ++i) {
        const double x = xs[i];
        const float f = f32store(x);
        const int64_t ii = static_cast<int64_t>(i);
        const std::string s1 = FormatDoubleG(x), s2 = FormatDoubleR(x), s3 = FormatDoubleG(x, 17), s4 = FormatSingleG(f),
                          s5 = FormatSingleR(f), s6 = FormatDoubleF(x, 6), s7 = FormatSingleG(f, 9);
        ts.Add({"d.ToString()", ii, x, s1, RtD(s1, x)});
        ts.Add({"d.ToString(R)", ii, x, s2, RtD(s2, x)});
        ts.Add({"d.ToString(G17)", ii, x, s3, RtD(s3, x)});
        ts.Add({"d.ToString(F6)", ii, x, s6, nullptr});
        ts.Add({"f.ToString()", ii, x, s4, RtF(s4, f)});
        ts.Add({"f.ToString(R)", ii, x, s5, RtF(s5, f)});
        ts.Add({"f.ToString(G9)", ii, x, s7, RtF(s7, f)});
    }

    golden::Table tq("parse", "float.Parse / double.Parse of numeric tokens.");
    tq.Col("i", ColType::I64).Col("s", ColType::Str).Col("f", ColType::F32).Col("d", ColType::F64).Col("err", ColType::Str);
    tq.key = {"i"};
    const auto toks = ReadTsv(in + "/parse_tokens.tsv");
    for (size_t i = 0; i < toks.size(); ++i) {
        const std::string& s = toks[i][0];
        Cell fv, dv;
        std::string err;
        bool hasErr = false;
        try {
            fv = ParseSingle(s);
        } catch (const FormatException&) {
            err = "float:FormatException";
            hasErr = true;
        } catch (const OverflowException&) {
            err = "float:OverflowException";
            hasErr = true;
        }
        try {
            dv = ParseDouble(s);
        } catch (const std::exception& ex) {
            const bool fmt = dynamic_cast<const FormatException*>(&ex) != nullptr;
            err = (hasErr ? err + " " : std::string()) + "double:" + (fmt ? "FormatException" : "OverflowException");
            hasErr = true;
        }
        tq.Add({static_cast<int64_t>(i), s, fv, dv, hasErr ? Cell(err) : Cell(nullptr)});
    }

    golden::Table tr("sysrand", "System.Random(seed): independent generators per column.");
    tr.Col("seed", ColType::I64).Col("k", ColType::I64).Col("next", ColType::I64).Col("nextDouble", ColType::F64)
        .Col("next0_100", ColType::I64).Col("nextm1000_1000", ColType::I64);
    tr.key = {"seed", "k"};
    for (const int32_t seed : {0, 1, 7, 11, 42, 12345, 20261008, INT32_MAX, -5}) {
        SystemRandom r1(seed), r2(seed), r3(seed), r4(seed);
        for (int k = 0; k < 32; ++k)
            tr.Add({int64_t(seed), int64_t(k), int64_t(r1.Next()), r2.NextDouble(), int64_t(r3.Next(0, 100)),
                    int64_t(r4.Next(-1000, 1000))});
    }

    g.tables = {std::move(t64), std::move(t32), std::move(tp), std::move(tw), std::move(tc),
                std::move(ti),  std::move(ts),  std::move(tq), std::move(tr)};
    return WriteCandidate(opt, g, "libm_mono/libm_mono.json");
}

KSP_GOLDEN_DATASET("libm_mono", 60, RunLibmMono);
