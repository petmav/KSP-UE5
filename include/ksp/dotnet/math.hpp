// System.Math and CLR conversion semantics used by KSP (Mono runtime icalls, mono/metadata/sysmath.c, which match the
// CLR's COMDouble implementations for the functions below).
#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace ksp::dotnet {

// conv.i4 / conv.i8 on x64 (cvttsd2si): truncation; NaN and out-of-range values give the "integer indefinite" value
// INT_MIN / INT64_MIN. C++ leaves these cases undefined, so they are made explicit here.
inline int32_t ConvToInt32(double d) noexcept {
    if (!(d > -2147483649.0 && d < 2147483648.0)) return std::numeric_limits<int32_t>::min();
    return static_cast<int32_t>(d);
}
inline int64_t ConvToInt64(double d) noexcept {
    if (!(d >= -9223372036854775808.0 && d < 9223372036854775808.0)) return std::numeric_limits<int64_t>::min();
    return static_cast<int64_t>(d);
}

// Which runtime's System.Math semantics to reproduce where runtimes disagree.
//  NetFramework: the golden harness's oracle (.NET Framework 4.8 CLR, x64), verified with PowerShell on this machine:
//       * Sin/Cos/Tan behave like the x87 FSIN/FCOS/FPTAN instructions: argument reduction with the FPU's 66-bit pi
//         (pi87 = pi - 4.0445e-21), so Cos(Math.PI/2) = 6.1230317691118863E-17 (not 6.123233995736766E-17); for
//         |x| >= 2^63 Sin/Cos return x unchanged and Tan returns NaN;
//       * Ceiling(x) for -1 < x < 0 returns +0.0 (C99 ceil gives -0.0).
//  Ucrt: the MSVC UCRT functions, i.e. what Unity's Mono icalls (mono/metadata/sysmath.c) return if they call the CRT.
// Which one the shipped game matches is CAPTURE-SPEC C7. NetFramework is the default because it is what the golden
// data was produced with.
enum class Libm { NetFramework, Ucrt };
Libm GetLibm() noexcept;
void SetLibm(Libm mode) noexcept;  // process-wide; set once at startup

namespace Math {

double Sin(double x) noexcept;
double Cos(double x) noexcept;
double Tan(double x) noexcept;
double Ceiling(double x) noexcept;
inline double Floor(double x) noexcept { return std::floor(x); }

// System.Math.Round(double): banker's rounding, as implemented by the runtime (sysmath.c ves_icall_System_Math_Round /
// CLR COMDouble::Round), including its integral-value shortcut through an int64 cast.
inline double Round(double x) noexcept {
    if (x == static_cast<double>(ConvToInt64(x))) return x;
    const double tmp = x + 0.5;
    double floorTmp = std::floor(tmp);
    if (floorTmp == tmp) {
        if (std::fmod(tmp, 2.0) != 0.0) floorTmp -= 1.0;
    }
    return std::copysign(floorTmp, x);
}

// System.Math.Log(a, newBase) (reference source Math.cs)
inline double Log(double a, double newBase) noexcept {
    if (std::isnan(a)) return a;
    if (std::isnan(newBase)) return newBase;
    if (newBase == 1.0) return std::numeric_limits<double>::quiet_NaN();
    if (a != 1.0 && (newBase == 0.0 || std::isinf(newBase))) return std::numeric_limits<double>::quiet_NaN();
    return std::log(a) / std::log(newBase);
}

// System.Math.Sign(double): throws ArithmeticException on NaN in .NET; here NaN returns 0 (callers never pass NaN).
inline int Sign(double v) noexcept { return v < 0 ? -1 : (v > 0 ? 1 : 0); }

}  // namespace Math
}  // namespace ksp::dotnet
