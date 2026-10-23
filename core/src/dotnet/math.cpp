// System.Math and numeric conversions with selectable runtime semantics (see ksp/dotnet/math.hpp).
//
// X87 emulation: FSIN/FCOS/FPTAN reduce the argument with the FPU's 66-bit pi, evaluate in 64-bit-mantissa extended
// precision and the CLR stores the result to a double. Emulated here as:
//   1. k = nearest(x / P87), r = x - k*P87 as a double-double (P87 = pi87/2 split into four <= 17-bit pieces, so the
//      products are exact for |k| < 2^36 and x - k*PA is exact by Sterbenz);
//   2. sin/cos of the reduced argument in double-double arithmetic (~100 bits);
//   3. round to a 64-bit significand (extended precision), then to double (the CLR's fstp qword).
// Both runtimes JIT Math.Sin/Math.Cos to these x87 semantics (CAPTURE-SPEC C7: neither the VS2013 nor the current CRT
// matches them near multiples of pi/2 or for huge arguments); the emulation reproduces all 3,093 captured inputs.
//
// The other transcendental functions are the C runtime's. KSP's Mono (and .NET Framework 4.8) are linked against the
// Visual Studio 2013 CRT (msvcr120): its cosh/sinh/tanh/exp/pow/asin/atan/atan2 differ in the last bit from the current
// Windows UCRT on ~1% of inputs (C7 capture; checked function by function with ctypes). On Windows the VS2013 CRT is
// loaded at runtime from msvcr120_clr0400.dll (installed with .NET Framework 4.x, i.e. every Windows 10/11) or
// msvcr120.dll. Everywhere else (Linux, or Windows when neither DLL is present or KSP_LIBM_PORTABLE is set) the same
// functions run as portable software: ksp::dotnet::vs2013 (src/dotnet/vs2013_libm_gen.cpp), a mechanical lift of the
// CRT's own x64 machine code, bit-exact with the DLL (LibmBackend() reports which).
#include "ksp/dotnet/math.hpp"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string_view>

#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/ucrt_libm.hpp"
#include "ksp/dotnet/vs2013_libm.hpp"

#ifdef _WIN32
#include <intrin.h>
#include <windows.h>
#endif

namespace ksp::dotnet {

namespace {
std::atomic<Runtime> g_runtime{Runtime::Mono};

// ---- double-double arithmetic ----------------------------------------------------------------------------------------
struct DD {
    double hi, lo;
};
inline DD TwoSum(double a, double b) {
    const double s = a + b;
    const double bb = s - a;
    const double err = (a - (s - bb)) + (b - bb);
    return {s, err};
}
inline DD QuickTwoSum(double a, double b) {
    const double s = a + b;
    return {s, b - (s - a)};
}
inline DD TwoProd(double a, double b) {
    const double p = a * b;
    return {p, std::fma(a, b, -p)};
}
inline DD Add(DD a, DD b) {
    DD s = TwoSum(a.hi, b.hi);
    const DD t = TwoSum(a.lo, b.lo);
    s.lo += t.hi;
    s = QuickTwoSum(s.hi, s.lo);
    s.lo += t.lo;
    return QuickTwoSum(s.hi, s.lo);
}
inline DD Mul(DD a, DD b) {
    DD p = TwoProd(a.hi, b.hi);
    p.lo += a.hi * b.lo + a.lo * b.hi;
    return QuickTwoSum(p.hi, p.lo);
}
inline DD Div(DD a, DD b) {
    const double q1 = a.hi / b.hi;
    DD r = Add(a, Mul(b, DD{-q1, 0.0}));
    const double q2 = r.hi / b.hi;
    r = Add(r, Mul(b, DD{-q2, 0.0}));
    const double q3 = r.hi / b.hi;
    return Add(QuickTwoSum(q1, q2), DD{q3, 0.0});
}

// 1/n! as double-double, n = 0..29
struct InvFact {
    DD v[30];
    InvFact() {
        v[0] = {1.0, 0.0};
        for (int n = 1; n < 30; ++n) v[n] = Div(v[n - 1], DD{static_cast<double>(n), 0.0});
    }
};
const InvFact& Fact() {
    static const InvFact f;
    return f;
}

// sin(r), |r| <= ~pi/4, r given as double-double. Taylor series to r^29 (error < 1e-32 relative).
DD SinDD(DD r) {
    const InvFact& f = Fact();
    const DD r2 = Mul(r, r);
    DD s{0.0, 0.0};
    for (int n = 29; n >= 3; n -= 2) {
        const DD c = ((n / 2) % 2) ? DD{-f.v[n].hi, -f.v[n].lo} : f.v[n];
        s = Add(Mul(s, r2), c);
    }
    // sin = r + r*r2*s
    return Add(r, Mul(Mul(r, r2), s));
}

// cos(r) = 1 - r^2/2! + ...
DD CosDD(DD r) {
    const InvFact& f = Fact();
    const DD r2 = Mul(r, r);
    DD s{0.0, 0.0};
    for (int n = 28; n >= 2; n -= 2) {
        const DD c = ((n / 2) % 2) ? DD{-f.v[n].hi, -f.v[n].lo} : f.v[n];
        s = Add(Mul(s, r2), c);
    }
    return Add(DD{1.0, 0.0}, Mul(r2, s));
}

// Round a double-double to a 64-bit significand (x87 extended), then to double (round-to-nearest-even both times).
double RoundExtendedThenDouble(DD v) {
    if (v.lo == 0.0 || !std::isfinite(v.hi) || v.hi == 0.0) return v.lo == 0.0 ? v.hi : v.hi + v.lo;
    int e;
    std::frexp(v.hi, &e);  // v.hi = m * 2^e, 0.5 <= |m| < 1  -> unit in the 64th bit = 2^(e-64)
    // If hi is a power of two and lo pulls the value below it, the binade is one lower.
    if (std::fabs(v.hi) == std::ldexp(0.5, e) && (v.lo < 0) == (v.hi > 0)) --e;
    const double q = std::ldexp(1.0, e - 64);
    const double lo64 = std::nearbyint(v.lo / q) * q;  // hi is a multiple of 2^11 * q: parity decided by lo alone
    return v.hi + lo64;
}

// x87 internal pi/2 = 0x3243F6A8885A308D3 * 2^-65 (66 significant bits), split into four pieces of <= 17 bits so that
// k*piece is exact for |k| < 2^36 (Cody-Waite); x - k*kPA is exact by Sterbenz.
constexpr double kPA = 0x1.921fp+0;
constexpr double kPB = 0x1.6a88p-17;
constexpr double kPC = 0x1.0b46p-34;
constexpr double kPD = 0x1.1a6p-54;
constexpr double kP87 = kPA + kPB + kPC + kPD;  // rounded; only used to pick k
constexpr double kTwo63 = 9223372036854775808.0;
constexpr double kPiOver4 = 0.78539816339744828;
constexpr double kMaxK = 68719476736.0;  // 2^36

// Reduce by the x87 pi/2. Returns r = x - k*P87 as double-double, quadrant = k mod 4.
DD Reduce87(double x, int& quadrant) {
    const double k = std::nearbyint(x / kP87);
    DD r{x - k * kPA, 0.0};
    r = Add(r, DD{-k * kPB, 0.0});
    r = Add(r, DD{-k * kPC, 0.0});
    r = Add(r, DD{-k * kPD, 0.0});
    quadrant = static_cast<int>(static_cast<long long>(k) & 3);
    return r;
}

// ---- exact reduction for |k| >= 2^36 (|x| up to 2^63), in units of 2^-65 with 192-bit integers -------------------
struct U192 {
    uint64_t w[3];  // little-endian limbs, two's complement when used as signed
};
inline uint64_t MulHi(uint64_t a, uint64_t b, uint64_t& lo) {
#ifdef _MSC_VER
    uint64_t hi;
    lo = _umul128(a, b, &hi);
    return hi;
#else
    const unsigned __int128 p = static_cast<unsigned __int128>(a) * b;
    lo = static_cast<uint64_t>(p);
    return static_cast<uint64_t>(p >> 64);
#endif
}
inline U192 Sub(const U192& a, const U192& b) {
    U192 r;
    uint64_t borrow = 0;
    for (int i = 0; i < 3; ++i) {
        const uint64_t d = a.w[i] - b.w[i];
        const uint64_t b1 = a.w[i] < b.w[i] ? 1u : 0u;
        r.w[i] = d - borrow;
        const uint64_t b2 = d < borrow ? 1u : 0u;
        borrow = b1 | b2;
    }
    return r;
}
inline bool Negative(const U192& a) { return (a.w[2] >> 63) != 0; }
inline U192 Neg(const U192& a) { return Sub(U192{{0, 0, 0}}, a); }
// k (unsigned, < 2^63) * M (M = 0x3243F6A8885A308D3 = 3 * 2^64 + 0x243F6A8885A308D3)
constexpr uint64_t kMlo = 0x243F6A8885A308D3ull;
constexpr uint64_t kMhi = 0x3ull;
inline U192 MulM(uint64_t k) {
    uint64_t lo;
    const uint64_t hi = MulHi(k, kMlo, lo);
    uint64_t lo2;
    const uint64_t hi2 = MulHi(k, kMhi, lo2);  // k * 3: hi2 <= 1
    U192 r{{lo, hi + lo2, hi2}};
    if (r.w[1] < hi) ++r.w[2];  // carry
    return r;
}
// a (< 2^126) as double (only used for estimates)
inline double ToDouble(const U192& a) {
    return static_cast<double>(a.w[2]) * 3.4028236692093846e38 + static_cast<double>(a.w[1]) * 18446744073709551616.0 +
           static_cast<double>(a.w[0]);
}

// Reduce |x| (>= 2^36 * P87, < 2^63) exactly: returns r = |x| - k*P87 (|r| <= P87/2) as double-double, k mod 4.
DD Reduce87Big(double ax, int& quadrant) {
    int e;
    const double m = std::frexp(ax, &e);  // ax = m * 2^e, 0.5 <= m < 1
    const uint64_t mx = static_cast<uint64_t>(std::ldexp(m, 53));
    const int shift = e - 53 + 65;  // X = mx * 2^shift in units of 2^-65; 49 <= shift <= 75
    U192 X{{0, 0, 0}};
    if (shift < 64) {
        X.w[0] = mx << shift;
        X.w[1] = shift ? (mx >> (64 - shift)) : 0;
    } else {
        X.w[1] = mx << (shift - 64);
        X.w[2] = (shift - 64) ? (mx >> (128 - shift)) : 0;
    }
    uint64_t k = static_cast<uint64_t>(ax / kP87);
    U192 R = Sub(X, MulM(k));
    const double Md = 57952155664616982739.0;  // M as double (estimate only)
    for (int iter = 0; iter < 8; ++iter) {
        const bool neg = Negative(R);
        const U192 mag = neg ? Neg(R) : R;
        const double q = std::floor(ToDouble(mag) / Md);
        if (q >= 1.0) {
            const uint64_t qi = static_cast<uint64_t>(q);
            if (neg) {
                k -= qi;
                R = Sub(R, Neg(MulM(qi)));
            } else {
                k += qi;
                R = Sub(R, MulM(qi));
            }
            continue;
        }
        if (neg) {  // -M < R < 0 -> 0 <= R < M
            k -= 1;
            R = Sub(R, Neg(MulM(1)));
        }
        break;
    }
    // now 0 <= R < M; pick the nearest multiple
    U192 twoR{{R.w[0] << 1, (R.w[1] << 1) | (R.w[0] >> 63), (R.w[2] << 1) | (R.w[1] >> 63)}};
    const U192 M1 = MulM(1);
    const bool above = !Negative(Sub(M1, twoR)) ? false : true;  // 2R > M
    if (above) {
        k += 1;
        R = Sub(R, M1);
    }
    quadrant = static_cast<int>(k & 3);
    const bool neg = Negative(R);
    const U192 mag = neg ? Neg(R) : R;  // < 2^65
    const uint64_t top = (mag.w[1] << 31) | (mag.w[0] >> 33);  // bits 33..65
    const uint64_t bot = mag.w[0] & ((1ull << 33) - 1);
    DD r = TwoSum(std::ldexp(static_cast<double>(top), 33 - 65), std::ldexp(static_cast<double>(bot), -65));
    if (neg) r = DD{-r.hi, -r.lo};
    return r;
}

enum class Fn { Sin, Cos, Tan };

double X87(Fn fn, double x) {
    if (std::isnan(x)) return x;
    if (std::isinf(x)) return std::numeric_limits<double>::quiet_NaN();
    if (!(std::fabs(x) < kTwo63)) return fn == Fn::Tan ? std::numeric_limits<double>::quiet_NaN() : x;
    if (x == 0.0) return fn == Fn::Cos ? 1.0 : x;  // sin(+-0) = +-0, tan(+-0) = +-0
    DD r{x, 0.0};
    int q = 0;
    if (std::fabs(x) > kPiOver4) {
        if (std::fabs(x) >= kMaxK * kPA) {
            // huge argument: exact 192-bit reduction of |x|, then odd/even symmetry
            r = Reduce87Big(std::fabs(x), q);
            if (x < 0) {
                r = DD{-r.hi, -r.lo};
                q = (4 - q) & 3;
            }
        } else {
            r = Reduce87(x, q);
        }
    }
    DD v{0.0, 0.0};
    switch (fn) {
        case Fn::Sin: {
            const DD s = (q & 1) ? CosDD(r) : SinDD(r);
            v = (q & 2) ? DD{-s.hi, -s.lo} : s;
            break;
        }
        case Fn::Cos: {
            const DD c = (q & 1) ? SinDD(r) : CosDD(r);
            const bool neg = (q == 1 || q == 2);
            v = neg ? DD{-c.hi, -c.lo} : c;
            break;
        }
        case Fn::Tan: {
            const DD s = SinDD(r), c = CosDD(r);
            v = (q & 1) ? Div(DD{-c.hi, -c.lo}, s) : Div(s, c);
            break;
        }
    }
    return RoundExtendedThenDouble(v);
}
}  // namespace

Runtime GetRuntime() noexcept { return g_runtime.load(std::memory_order_relaxed); }
void SetRuntime(Runtime r) noexcept { g_runtime.store(r, std::memory_order_relaxed); }

// ---- C runtime backend ------------------------------------------------------------------------------------------------
namespace {
using F1 = double (*)(double);
using F2 = double (*)(double, double);
struct Crt {
    F1 tan = nullptr, asin = nullptr, acos = nullptr, atan = nullptr, sinh = nullptr, cosh = nullptr, tanh = nullptr,
       exp = nullptr, log = nullptr, log10 = nullptr;
    F2 atan2 = nullptr, pow = nullptr;
    const char* name = "";
};

[[maybe_unused]] bool UsePortable() {
#ifdef KSP_LIBM_PORTABLE
    return true;
#else
    // read once per process (WP-20f: NativeSinf / NativeCosf / NativePowf ask on every call; getenv cost ~1 us per call on Windows,
    // half of a ModuleGimbal.FixedUpdate). The switch is a process-start setting (ctest ENVIRONMENT, CMakeLists.txt:80).
    static const bool portable = [] {
        const char* e = std::getenv("KSPCORE_LIBM_PORTABLE");
        return e && *e && *e != '0';
    }();
    return portable;
#endif
}

const Crt& GetCrt() {
    static Crt crt;
    static std::once_flag once;
    std::call_once(once, [] {
        crt = Crt{vs2013::tan, vs2013::asin, vs2013::acos, vs2013::atan, vs2013::sinh, vs2013::cosh, vs2013::tanh,
                  vs2013::exp, vs2013::log, vs2013::log10, vs2013::atan2, vs2013::pow,
                  "VS2013 CRT (portable software lift of msvcr120_clr0400.dll)"};
#ifdef _WIN32
        if (UsePortable()) return;
        for (const wchar_t* dll : {L"msvcr120_clr0400.dll", L"msvcr120.dll"}) {
            HMODULE h = LoadLibraryW(dll);
            if (!h) continue;
            auto f1 = [h](const char* n) { return reinterpret_cast<F1>(GetProcAddress(h, n)); };
            auto f2 = [h](const char* n) { return reinterpret_cast<F2>(GetProcAddress(h, n)); };
            Crt c{f1("tan"), f1("asin"), f1("acos"), f1("atan"), f1("sinh"), f1("cosh"), f1("tanh"), f1("exp"),
                  f1("log"), f1("log10"), f2("atan2"), f2("pow"), nullptr};
            if (c.tan && c.asin && c.acos && c.atan && c.sinh && c.cosh && c.tanh && c.exp && c.log && c.log10 &&
                c.atan2 && c.pow) {
                c.name = dll == std::wstring_view(L"msvcr120_clr0400.dll") ? "VS2013 CRT (msvcr120_clr0400.dll)"
                                                                           : "VS2013 CRT (msvcr120.dll)";
                crt = c;
                return;
            }
        }
#endif
    });
    return crt;
}
}  // namespace

const char* LibmBackend() noexcept { return GetCrt().name; }

// ---- Unity native single-precision functions (UCRT sinf/cosf/powf) ----
#ifdef _WIN32
float NativeSinf(float x) { return UsePortable() ? ucrt::sinf(x) : std::sin(x); }
float NativeCosf(float x) { return UsePortable() ? ucrt::cosf(x) : std::cos(x); }
float NativePowf(float x, float y) { return UsePortable() ? ucrt::powf(x, y) : std::pow(x, y); }
#else
float NativeSinf(float x) { return ucrt::sinf(x); }
float NativeCosf(float x) { return ucrt::cosf(x); }
float NativePowf(float x, float y) { return ucrt::powf(x, y); }
#endif

namespace {
constexpr double kTwo64 = 18446744073709551616.0;
constexpr double kTwo63d = 9223372036854775808.0;
}  // namespace

// (ulong)d -- see header
uint64_t ConvToUInt64(double d) noexcept {
    if (std::isnan(d) || d >= kTwo64 || d < -kTwo63d)
        return GetRuntime() == Runtime::Mono ? 0x8000000000000000ull : 0ull;
    if (d >= kTwo63d) return static_cast<uint64_t>(static_cast<int64_t>(d - kTwo63d)) ^ 0x8000000000000000ull;
    return static_cast<uint64_t>(static_cast<int64_t>(d));
}

// (double)ulong: Mono rounds correctly; .NET Framework's JIT_ULng2Dbl converts as signed and adds 2^64 (two roundings).
double ConvUInt64ToDouble(uint64_t u) noexcept {
    if (GetRuntime() == Runtime::Mono) return static_cast<double>(u);
    double conv = static_cast<double>(static_cast<int64_t>(u));
    if (conv < 0) conv += kTwo64;
    return conv;
}

// Convert.ToInt32(double) (mscorlib Convert.cs)
int32_t ConvertToInt32(double value) {
    if (value >= 0) {
        if (value < 2147483647.5) {
            int32_t result = static_cast<int32_t>(value);
            const double dif = value - result;
            if (dif > 0.5 || (dif == 0.5 && (result & 1) != 0)) ++result;
            return result;
        }
    } else if (value >= -2147483648.5) {
        int32_t result = static_cast<int32_t>(value);
        const double dif = value - result;
        if (dif < -0.5 || (dif == -0.5 && (result & 1) != 0)) --result;
        return result;
    }
    throw OverflowException("Value was either too large or too small for an Int32.");
}

namespace Math {
double Sin(double x) noexcept { return X87(Fn::Sin, x); }
double Cos(double x) noexcept { return X87(Fn::Cos, x); }
double Tan(double x) noexcept { return GetRuntime() == Runtime::NetFramework ? X87(Fn::Tan, x) : GetCrt().tan(x); }
double Asin(double x) noexcept { return GetCrt().asin(x); }
double Acos(double x) noexcept { return GetCrt().acos(x); }
double Atan(double x) noexcept { return GetCrt().atan(x); }
double Sinh(double x) noexcept { return GetCrt().sinh(x); }
double Cosh(double x) noexcept { return GetCrt().cosh(x); }
double Tanh(double x) noexcept { return GetCrt().tanh(x); }
double Exp(double x) noexcept { return GetCrt().exp(x); }
double Log(double x) noexcept { return GetCrt().log(x); }
double Log10(double x) noexcept { return GetCrt().log10(x); }

// Mono sysmath.c ves_icall_System_Math_Atan2: a zero result is returned as +0. Both runtimes: Atan2(+-Inf, +-Inf) = NaN.
double Atan2(double y, double x) noexcept {
    if (std::isinf(x) && std::isinf(y)) return std::numeric_limits<double>::quiet_NaN();
    const double r = GetCrt().atan2(y, x);
    if (GetRuntime() == Runtime::Mono && r == 0.0) return 0.0;
    return r;
}

// Math.Pow. Mono (capture: Mathf.Pow(-0f, -3f) = +Infinity) returns +Infinity for a zero base with a negative exponent;
// the CRT (and .NET Framework) give -Infinity for -0 raised to a negative odd integer.
double Pow(double x, double y) noexcept {
    if (GetRuntime() == Runtime::Mono && x == 0.0 && y < 0.0) return std::numeric_limits<double>::infinity();
    return GetCrt().pow(x, y);
}

double Ceiling(double x) noexcept {
    const double r = std::ceil(x);
    if (GetRuntime() == Runtime::NetFramework && r == 0.0 && x < 0.0) return 0.0;  // .NET Framework: Ceiling(-0.5) == +0
    return r;
}

// mscorlib Math.cs InternalRound
double Round(double value, int digits, MidpointRounding mode) noexcept {
    static constexpr double kPow10[16] = {1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0, 10000000.0, 100000000.0,
                                          1000000000.0, 10000000000.0, 100000000000.0, 1000000000000.0, 10000000000000.0,
                                          100000000000000.0, 1000000000000000.0};
    if (std::fabs(value) < 10000000000000000.0) {
        const double num = kPow10[digits];
        value *= num;
        if (mode == MidpointRounding::AwayFromZero) {
            double ip;
            const double frac = std::modf(value, &ip);  // SplitFractionDouble
            value = ip;
            if (std::fabs(frac) >= 0.5) value += static_cast<double>(Sign(frac));
        } else {
            value = Round(value);
        }
        value /= num;
    }
    return value;
}

// mscorlib Math.cs IEEERemainder
double IEEERemainder(double x, double y) noexcept {
    if (std::isnan(x)) return x;
    if (std::isnan(y)) return y;
    const double num = std::fmod(x, y);
    if (std::isnan(num)) return std::numeric_limits<double>::quiet_NaN();
    if (num == 0.0 && std::signbit(x)) return -0.0;
    const double num2 = num - std::fabs(y) * static_cast<double>(Sign(x));
    if (std::fabs(num2) == std::fabs(num)) {
        const double num3 = x / y;
        if (std::fabs(Round(num3)) > std::fabs(num3)) return num2;
        return num;
    }
    if (std::fabs(num2) < std::fabs(num)) return num2;
    return num;
}
}  // namespace Math

}  // namespace ksp::dotnet
