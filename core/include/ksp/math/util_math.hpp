// UtilMath (Assembly-CSharp/UtilMath.cs, every member; completed in WP-03) and Planetarium.CelestialFrame (Assembly-CSharp/Planetarium.cs:7-58).
#pragma once

#include <cmath>
#include <functional>
#include <initializer_list>
#include <limits>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"

namespace ksp {

namespace UtilMath {

// UtilMath.cs:5-19 (TwoPI/HalfPI/... are mutable statics in C#; (float)Math.PI * 2f is folded by the C# compiler)
inline constexpr double PI = 3.141592653589793;  // System.Math.PI
inline constexpr double Rad2Deg = 180.0 / PI;
inline constexpr double Deg2Rad = PI / 180.0;
inline constexpr double TwoPI = PI * 2.0;
inline constexpr float TwoPIf = 3.14159274f * 2.0f;
inline constexpr double HalfPI = PI / 2.0;
inline constexpr float HalfPIf = 3.14159274f / 2.0f;
inline constexpr float RPM2RadPerSec = TwoPIf / 60.0f;
inline constexpr float RadPerSec2RPM = 60.0f / TwoPIf;

// UtilMath.cs:21
inline void SwapValues(double& a, double& b) { std::swap(a, b); }
// UtilMath.cs:143
inline double Clamp01(double value) {
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}
// UtilMath.cs:28
inline double Lerp(double a, double b, double t) {
    t = Clamp01(t);
    return (1.0 - t) * a + t * b;
}
// UtilMath.cs:34
inline double LerpUnclamped(double a, double b, double t) { return (1.0 - t) * a + t * b; }
// UtilMath.cs:39
inline double InverseLerp(double a, double b, double t) { return (t - a) / (b - a); }
// UtilMath.cs:44
inline double DegreesToRadians(double degrees) { return PI / 180.0 * degrees; }
// UtilMath.cs:49
inline double RadiansToDegrees(double radians) { return 180.0 / PI * radians; }
// UtilMath.cs:54-97 (System.Math: Log/Cosh/Sinh are the VS2013 CRT functions, see ksp/dotnet/math.hpp)
inline double ASinh(double x) { return dotnet::Math::Log(x + dotnet::Math::Sqrt(x * x + 1.0)); }
inline double ACosh(double x) { return dotnet::Math::Log(x + dotnet::Math::Sqrt(x * x - 1.0)); }
inline double ATanh(double x) { return dotnet::Math::Log((1.0 + x) / (1.0 - x)) / 2.0; }
inline double ACoth(double x) { return ATanh(1.0 / x); }
inline double ASech(double x) { return ACosh(1.0 / x); }
inline double ACsch(double x) { return ASinh(1.0 / x); }
inline double Sech(double x) { return 1.0 / dotnet::Math::Cosh(x); }
inline double Csch(double x) { return 1.0 / dotnet::Math::Sinh(x); }
inline double Coth(double x) { return dotnet::Math::Cosh(x) / dotnet::Math::Sinh(x); }
// UtilMath.cs:99 (C# % on floating point == fmod; Mono float rule, see dotnet/mono_float.hpp)
inline float WrapAround(float value, float min, float max) {
    if (value < min) value = f32store(fx(value) + fx(max));
    return f32store(std::fmod(fx(value), fx(max)));
}
// UtilMath.cs:108
inline double WrapAround(double value, double min, double max) {
    if (value < min) value += max;
    return std::fmod(value, max);
}
// UtilMath.cs:117 (unchecked int: `value += max` wraps; `% 0` throws DivideByZeroException; int.MinValue % -1 throws
// OverflowException as on .NET x64)
int WrapAround(int value, int min, int max);
// UtilMath.cs:126
inline double Clamp(double value, double min, double max) {
    if (std::isnan(value)) return min;
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
// UtilMath.cs:156
inline double ClampRadians(double angle) {
    while (angle > PI * 2.0) angle -= PI * 2.0;
    while (angle < 0.0) angle += PI * 2.0;
    return angle;
}
// UtilMath.cs:169
inline double ClampRadiansPI(double angle) {
    while (angle > PI) angle -= PI;
    while (angle < -PI) angle += PI;
    return angle;
}
// UtilMath.cs:182
inline double ClampRadiansTwoPI(double angle) {
    angle = std::fmod(angle, TwoPI);
    if (angle < 0.0) return angle + TwoPI;
    return angle;
}
// UtilMath.cs:192
inline double ClampDegrees360(double angle) {
    angle = std::fmod(angle, 360.0);
    if (angle < 0.0) return angle + 360.0;
    return angle;
}
// UtilMath.cs:202
inline double ClampDegrees180(double angle) {
    angle = ClampDegrees360(angle);
    if (angle > 180.0) angle -= 360.0;
    return angle;
}
// UtilMath.cs:212 (Math.Sign(NaN) throws ArithmeticException)
double Flatten(double z, double midPoint, double easing);
// UtilMath.cs:217 / 231 Max / Min(params double[]): scanned from the last element; start at double.MinValue /
// double.MaxValue and replace only on a strict > / < (NaN elements are skipped; an empty list returns the start value)
double Max(std::span<const double> values);
double Min(std::span<const double> values);
inline double Max(std::initializer_list<double> values) { return Max(std::span<const double>(values.begin(), values.size())); }
inline double Min(std::initializer_list<double> values) { return Min(std::span<const double>(values.begin(), values.size())); }

// UtilMath.cs:245-379 MaxFrom / MinFrom<T>(getValue, [out extreme,] params T[] values): result starts as values[0]
// (IndexOutOfRange -> std::out_of_range for an empty list), the scan runs from the LAST element to the first with a
// strict comparison, so among equal extremes the one with the highest index wins. getValue returns double or float
// (the float overloads start from float.MinValue / float.MaxValue).
namespace detail {
template <class T, class Get, bool IsMax>
T ExtremeFrom(const Get& getValue, std::span<const T> values, decltype(getValue(std::declval<const T&>()))* extreme) {
    using R = decltype(getValue(std::declval<const T&>()));
    static_assert(std::is_same_v<R, double> || std::is_same_v<R, float>, "getValue must return double or float");
    R num = IsMax ? std::numeric_limits<R>::lowest() : std::numeric_limits<R>::max();
    if (values.empty()) throw std::out_of_range("Index was outside the bounds of the array.");
    T result = values[0];
    size_t num2 = values.size();
    while (num2-- > 0) {
        const R num3 = getValue(values[num2]);
        if (IsMax ? (num3 > num) : (num3 < num)) {
            num = num3;
            result = values[num2];
        }
    }
    if (extreme) *extreme = num;
    return result;
}
}  // namespace detail
template <class T, class Get>
T MaxFrom(const Get& getValue, std::span<const T> values) {
    return detail::ExtremeFrom<T, Get, true>(getValue, values, nullptr);
}
template <class T, class Get>
T MinFrom(const Get& getValue, std::span<const T> values) {
    return detail::ExtremeFrom<T, Get, false>(getValue, values, nullptr);
}
template <class T, class Get, class R>
T MaxFrom(const Get& getValue, R& max, std::span<const T> values) {
    return detail::ExtremeFrom<T, Get, true>(getValue, values, &max);
}
template <class T, class Get, class R>
T MinFrom(const Get& getValue, R& min, std::span<const T> values) {
    return detail::ExtremeFrom<T, Get, false>(getValue, values, &min);
}

// UtilMath.cs:381 / 387
float RoundToPlaces(float value, int decimalPlaces);
double RoundToPlaces(double value, int decimalPlaces);
// UtilMath.cs:393 / 434 (Math.Min with .NET NaN / signed-zero semantics)
int BSPSolver(double& v0, double dv, const std::function<double(double)>& solveFor, double vMin, double vMax, double epsilon,
              int maxIterations);
int BSPSolver(float& v0, float dv, const std::function<float(float)>& solveFor, float vMin, float vMax, float epsilon,
              int maxIterations);
// UtilMath.cs:475 (byN == 0 loops forever, as in KSP)
bool IsDivisible(int n, int byN);
// UtilMath.cs:488
inline bool IsPowerOfTwo(int x) { return (x & static_cast<int>(static_cast<unsigned>(x) - 1u)) == 0; }
// UtilMath.cs:493 (default epsilon = double.Epsilon = 4.94e-324)
bool Approximately(double a, double b, double epsilon = 4.9406564584124654E-324);
// UtilMath.cs:509 / 531
bool SphereIntersection(double radius, const Vector3d& position, const Vector3d& velocity, double& time, bool later);
bool SphereIntersection(double radius, const Vector3d& position, const Vector3d& velocity, Vector3d& impact, bool later);
// UtilMath.cs:542
double AngleBetween(const Vector3d& v, const Vector3d& w);

}  // namespace UtilMath

// Planetarium.CelestialFrame (Planetarium.cs:7)
struct CelestialFrame {
    Vector3d X, Y, Z;

    QuaternionD Rotation() const { return QuaternionD(X, Y, Z); }
    Vector3d WorldToLocal(const Vector3d& r) const { return {Vector3d::Dot(r, X), Vector3d::Dot(r, Y), Vector3d::Dot(r, Z)}; }
    Vector3d LocalToWorld(const Vector3d& r) const { return r.x * X + r.y * Y + r.z * Z; }

    static void SetFrame(double A, double B, double C, CelestialFrame& cf);
    static void OrbitalFrame(double LAN, double Inc, double ArgPe, CelestialFrame& cf);
    static void PlanetaryFrame(double ra, double dec, double rot, CelestialFrame& cf);
};

// Planetarium.Awake (Planetarium.cs:182): Zup = PlanetaryFrame(0, 90, inverseRotAngle); rotation = Inverse(Zup.Rotation).swizzle.
void PlanetariumAwakeFrames(double inverseRotAngle, CelestialFrame& zup, QuaternionD& rotation);

}  // namespace ksp
