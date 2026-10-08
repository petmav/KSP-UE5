// UtilMath (Assembly-CSharp/UtilMath.cs) and Planetarium.CelestialFrame (Assembly-CSharp/Planetarium.cs:7-58).
#pragma once

#include <cmath>
#include <functional>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"

namespace ksp {

namespace UtilMath {

inline constexpr double PI = 3.141592653589793;  // System.Math.PI
inline constexpr double Rad2Deg = 180.0 / PI;
inline constexpr double Deg2Rad = PI / 180.0;
inline constexpr double TwoPI = PI * 2.0;
inline constexpr float TwoPIf = 3.14159274f * 2.0f;
inline constexpr double HalfPI = PI / 2.0;
inline constexpr float HalfPIf = 3.14159274f / 2.0f;
inline constexpr float RPM2RadPerSec = TwoPIf / 60.0f;
inline constexpr float RadPerSec2RPM = 60.0f / TwoPIf;

// UtilMath.cs:11
inline void SwapValues(double& a, double& b) { std::swap(a, b); }
// UtilMath.cs:64
inline double Clamp01(double value) {
    if (value < 0.0) return 0.0;
    if (value > 1.0) return 1.0;
    return value;
}
// UtilMath.cs:15-21
inline double Lerp(double a, double b, double t) {
    t = Clamp01(t);
    return (1.0 - t) * a + t * b;
}
inline double LerpUnclamped(double a, double b, double t) { return (1.0 - t) * a + t * b; }
inline double InverseLerp(double a, double b, double t) { return (t - a) / (b - a); }
inline double DegreesToRadians(double degrees) { return PI / 180.0 * degrees; }
inline double RadiansToDegrees(double radians) { return 180.0 / PI * radians; }
// UtilMath.cs:26-43
inline double ASinh(double x) { return std::log(x + std::sqrt(x * x + 1.0)); }
inline double ACosh(double x) { return std::log(x + std::sqrt(x * x - 1.0)); }
inline double ATanh(double x) { return std::log((1.0 + x) / (1.0 - x)) / 2.0; }
inline double ACoth(double x) { return ATanh(1.0 / x); }
inline double ASech(double x) { return ACosh(1.0 / x); }
inline double ACsch(double x) { return ASinh(1.0 / x); }
inline double Sech(double x) { return 1.0 / std::cosh(x); }
inline double Csch(double x) { return 1.0 / std::sinh(x); }
inline double Coth(double x) { return std::cosh(x) / std::sinh(x); }
// UtilMath.cs:44-55 (C# % on floating point == fmod)
inline float WrapAround(float value, float min, float max) {
    if (value < min) value += max;
    return std::fmod(value, max);
}
inline double WrapAround(double value, double min, double max) {
    if (value < min) value += max;
    return std::fmod(value, max);
}
inline int WrapAround(int value, int min, int max) {
    if (value < min) value += max;
    return value % max;
}
// UtilMath.cs:56
inline double Clamp(double value, double min, double max) {
    if (std::isnan(value)) return min;
    if (value < min) return min;
    if (value > max) return max;
    return value;
}
// UtilMath.cs:70
inline double ClampRadians(double angle) {
    while (angle > PI * 2.0) angle -= PI * 2.0;
    while (angle < 0.0) angle += PI * 2.0;
    return angle;
}
// UtilMath.cs:76
inline double ClampRadiansPI(double angle) {
    while (angle > PI) angle -= PI;
    while (angle < -PI) angle += PI;
    return angle;
}
// UtilMath.cs:82
inline double ClampRadiansTwoPI(double angle) {
    angle = std::fmod(angle, TwoPI);
    if (angle < 0.0) return angle + TwoPI;
    return angle;
}
// UtilMath.cs:87
inline double ClampDegrees360(double angle) {
    angle = std::fmod(angle, 360.0);
    if (angle < 0.0) return angle + 360.0;
    return angle;
}
// UtilMath.cs:92
inline double ClampDegrees180(double angle) {
    angle = ClampDegrees360(angle);
    if (angle > 180.0) angle -= 360.0;
    return angle;
}
// UtilMath.cs:97 (Math.Sign(z) cast to double)
double Flatten(double z, double midPoint, double easing);
// UtilMath.cs:193
float RoundToPlaces(float value, int decimalPlaces);
double RoundToPlaces(double value, int decimalPlaces);
// UtilMath.cs:199 / 223
int BSPSolver(double& v0, double dv, const std::function<double(double)>& solveFor, double vMin, double vMax, double epsilon,
              int maxIterations);
int BSPSolver(float& v0, float dv, const std::function<float(float)>& solveFor, float vMin, float vMax, float epsilon,
              int maxIterations);
// UtilMath.cs:247
bool IsDivisible(int n, int byN);
inline bool IsPowerOfTwo(int x) { return (x & (x - 1)) == 0; }
// UtilMath.cs:255 (default epsilon = double.Epsilon = 4.94e-324)
bool Approximately(double a, double b, double epsilon = 4.9406564584124654E-324);
// UtilMath.cs:264-282
bool SphereIntersection(double radius, const Vector3d& position, const Vector3d& velocity, double& time, bool later);
bool SphereIntersection(double radius, const Vector3d& position, const Vector3d& velocity, Vector3d& impact, bool later);
// UtilMath.cs:283
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
