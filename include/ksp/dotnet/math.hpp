// System.Math, numeric conversions and the runtime they are evaluated by.
//
// KSP runs on Unity 2019.4's Mono 5.11 (x64). The golden harness ran KSP's IL on .NET Framework 4.8 (x64). The two
// runtimes agree on almost everything that matters (CAPTURE-SPEC C7, harness/captures/README.md and
// reports/C7_analysis.md: 24 of 25 libm functions bit-identical on 3,093 inputs) and differ in a few places, all of
// which are reproduced here behind ksp::dotnet::Runtime:
//
//   function / conversion      Mono (in game, default)                     .NET Framework 4.8 (harness oracle)
//   Math.Sin, Math.Cos         x87 FSIN/FCOS semantics on both runtimes: reduction with the FPU's 66-bit pi
//                              (Sin(Math.PI) = 1.2246063538223773E-16), |x| >= 2^63 returns x
//   Math.Tan                   CRT tan (within 1 ULP of exact)              x87 FPTAN semantics (Tan(PI/2) = 1.633177e16,
//                                                                            NaN for |x| >= 2^63)
//   Math.Ceiling(-0.5)         -0.0 (IEEE)                                  +0.0
//   Math.Atan2 -> zero result  +0.0 (sysmath.c: result == -0 ? 0 : result)  IEEE (-0.0 for Atan2(-0, x>0))
//   Math.Pow(-0, odd negative) +Infinity                                    -Infinity (IEEE)
//   (ulong)double out of range 0x8000000000000000                           0
//   (double)ulong              correctly rounded                            1 ULP low for some values >= 2^63
//
// Asin, Acos, Atan, Atan2, Sinh, Cosh, Tanh, Exp, Log, Log10, Pow (and Tan on Mono) are the Visual Studio 2013 C
// runtime's functions on both runtimes; kspcore calls the same DLL on Windows (msvcr120_clr0400.dll / msvcr120.dll,
// see LibmBackend()); the current UCRT differs from it in the last bit on ~1% of inputs. Sqrt, Floor, Ceiling, Round,
// IEEERemainder and % are exact operations. Verified bit-for-bit against harness/captures/out/libm_mono.json
// (capture.libm_mono) and against .NET Framework results (tests/data/netfx_reference.tsv).
#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace ksp::dotnet {

enum class Runtime { Mono, NetFramework };
Runtime GetRuntime() noexcept;
void SetRuntime(Runtime r) noexcept;  // process-wide; set once at startup (tests switch it temporarily)

// Which C runtime implements the CRT-backed functions: "VS2013 CRT (msvcr120_clr0400.dll)" / "VS2013 CRT (msvcr120.dll)"
// (bit-exact with KSP) or "C++ <cmath> ..." (portable fallback, last-bit differences possible).
const char* LibmBackend() noexcept;

// ---- unchecked C# numeric conversions as JIT-compiled on x64 (cvttsd2si; both runtimes unless noted) ----------------
// (int)d: truncation; NaN / out of range -> int.MinValue
inline int32_t ConvToInt32(double d) noexcept {
    if (!(d > -2147483649.0 && d < 2147483648.0)) return std::numeric_limits<int32_t>::min();
    return static_cast<int32_t>(d);
}
// (long)d: truncation; NaN / out of range -> long.MinValue
inline int64_t ConvToInt64(double d) noexcept {
    if (!(d >= -9223372036854775808.0 && d < 9223372036854775808.0)) return std::numeric_limits<int64_t>::min();
    return static_cast<int64_t>(d);
}
// (uint)d = low 32 bits of (long)d
inline uint32_t ConvToUInt32(double d) noexcept { return static_cast<uint32_t>(static_cast<uint64_t>(ConvToInt64(d))); }
// (short)d / (byte)d = low bits of (int)d
inline int16_t ConvToInt16(double d) noexcept {
    return static_cast<int16_t>(static_cast<uint16_t>(static_cast<uint32_t>(ConvToInt32(d))));
}
inline uint8_t ConvToByte(double d) noexcept { return static_cast<uint8_t>(static_cast<uint32_t>(ConvToInt32(d))); }
// (ulong)d: Mono: values below 2^63 via (long) (negatives wrap), [2^63, 2^64) exact, otherwise 0x8000000000000000;
// .NET Framework: 0 for NaN / infinities / >= 2^64.
uint64_t ConvToUInt64(double d) noexcept;
// (double)ulong
double ConvUInt64ToDouble(uint64_t u) noexcept;
// Convert.ToInt32(double): round half to even; throws dotnet::OverflowException outside [-2^31-0.5, 2^31-0.5) and NaN.
int32_t ConvertToInt32(double d);

enum class MidpointRounding { ToEven = 0, AwayFromZero = 1 };

namespace Math {

double Sin(double x) noexcept;
double Cos(double x) noexcept;
double Tan(double x) noexcept;
double Asin(double x) noexcept;
double Acos(double x) noexcept;
double Atan(double x) noexcept;
double Atan2(double y, double x) noexcept;
double Sinh(double x) noexcept;
double Cosh(double x) noexcept;
double Tanh(double x) noexcept;
double Exp(double x) noexcept;
double Log(double x) noexcept;
double Log10(double x) noexcept;
inline double Sqrt(double x) noexcept { return std::sqrt(x); }
double Pow(double x, double y) noexcept;
inline double Floor(double x) noexcept { return std::floor(x); }
double Ceiling(double x) noexcept;
inline double Abs(double x) noexcept { return std::fabs(x); }
inline float Abs(float x) noexcept { return std::fabs(x); }

// System.Math.Round(double): banker's rounding as implemented by the runtime (sysmath.c ves_icall_System_Math_Round /
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
// Math.Round(value, digits, mode) (mscorlib Math.cs InternalRound): |value| >= 1e16 is returned unchanged.
double Round(double value, int digits, MidpointRounding mode) noexcept;
inline double Round(double value, MidpointRounding mode) noexcept { return Round(value, 0, mode); }

// Math.Log(a, newBase) (mscorlib Math.cs)
inline double Log(double a, double newBase) noexcept {
    if (std::isnan(a)) return a;
    if (std::isnan(newBase)) return newBase;
    if (newBase == 1.0) return std::numeric_limits<double>::quiet_NaN();
    if (a != 1.0 && (newBase == 0.0 || std::isinf(newBase))) return std::numeric_limits<double>::quiet_NaN();
    return std::log(a) / std::log(newBase);
}

// Math.IEEERemainder (mscorlib Math.cs)
double IEEERemainder(double x, double y) noexcept;

// Math.Max(double, double) / Math.Min(double, double) (mscorlib Math.cs; Mono's corlib agrees): NaN in either argument
// gives NaN (std::max/std::min do not); ties return val2, so Max(-0.0, +0.0) = +0.0 and Max(+0.0, -0.0) = -0.0.
inline double Max(double val1, double val2) noexcept {
    if (val1 > val2) return val1;
    if (std::isnan(val1)) return val1;
    return val2;
}
inline double Min(double val1, double val2) noexcept {
    if (val1 < val2) return val1;
    if (std::isnan(val1)) return val1;
    return val2;
}

// Math.Sign(double): throws ArithmeticException on NaN in .NET; here NaN returns 0 (callers never pass NaN).
inline int Sign(double v) noexcept { return v < 0 ? -1 : (v > 0 ? 1 : 0); }
inline int Sign(float v) noexcept { return v < 0 ? -1 : (v > 0 ? 1 : 0); }

}  // namespace Math
}  // namespace ksp::dotnet
