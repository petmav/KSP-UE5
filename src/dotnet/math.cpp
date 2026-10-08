// System.Math.Sin/Cos/Tan/Ceiling with selectable runtime semantics (see ksp/dotnet/math.hpp).
//
// X87 emulation: FSIN/FCOS/FPTAN reduce the argument with the FPU's 66-bit pi, evaluate in 64-bit-mantissa extended
// precision and the CLR stores the result to a double. Emulated here as:
//   1. k = nearest(x / P87), r = x - k*P87 as a double-double (P87 = pi87/2 split into four <= 17-bit pieces, so the
//      products are exact for |k| < 2^36 and x - k*PA is exact by Sterbenz);
//   2. sin/cos of the reduced argument in double-double arithmetic (~100 bits);
//   3. round to a 64-bit significand (extended precision), then to double (the CLR's fstp qword).
// This reproduces the harness oracle bit-for-bit except where FSIN's own extended-precision result is not correctly
// rounded and that error crosses a double rounding boundary (rare; within the math_shim tolerance either way).
#include "ksp/dotnet/math.hpp"

#include <atomic>

namespace ksp::dotnet {

namespace {
std::atomic<Libm> g_libm{Libm::NetFramework};

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
            // Outside the exact-reduction range (|x| > 1.0e11 rad): not reachable from KSP; use the CRT.
            return fn == Fn::Sin ? std::sin(x) : (fn == Fn::Cos ? std::cos(x) : std::tan(x));
        }
        r = Reduce87(x, q);
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

Libm GetLibm() noexcept { return g_libm.load(std::memory_order_relaxed); }
void SetLibm(Libm mode) noexcept { g_libm.store(mode, std::memory_order_relaxed); }

namespace Math {
double Sin(double x) noexcept { return GetLibm() == Libm::NetFramework ? X87(Fn::Sin, x) : std::sin(x); }
double Cos(double x) noexcept { return GetLibm() == Libm::NetFramework ? X87(Fn::Cos, x) : std::cos(x); }
double Tan(double x) noexcept { return GetLibm() == Libm::NetFramework ? X87(Fn::Tan, x) : std::tan(x); }
double Ceiling(double x) noexcept {
    const double r = std::ceil(x);
    if (GetLibm() == Libm::NetFramework && r == 0.0 && x < 0.0) return 0.0;  // .NET Framework: Ceiling(-0.5) == +0
    return r;
}
}  // namespace Math

}  // namespace ksp::dotnet
