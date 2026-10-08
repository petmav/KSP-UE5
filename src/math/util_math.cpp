#include "ksp/math/util_math.hpp"

#include <algorithm>
#include <cfloat>

#include "ksp/dotnet/math.hpp"

namespace ksp {
namespace UtilMath {

// UtilMath.cs:97
double Flatten(double z, double midPoint, double easing) {
    return 1.0 - 1.0 / (std::pow(1.0 / midPoint * std::fabs(z), easing) + 1.0) * static_cast<double>(dotnet::Math::Sign(z));
}

// UtilMath.cs:193
float RoundToPlaces(float value, int decimalPlaces) {
    const float num = f32store(dotnet::Math::Pow(10.0, decimalPlaces));
    // Mono: value * num is evaluated in double (exact product), not rounded to float before Math.Round
    return f32store(dotnet::Math::Round(fx(value) * fx(num)) / fx(num));
}

// UtilMath.cs:196
double RoundToPlaces(double value, int decimalPlaces) {
    const double num = std::pow(10.0, decimalPlaces);
    return dotnet::Math::Round(value * num) / num;
}

// UtilMath.cs:199
int BSPSolver(double& v0, double dv, const std::function<double(double)>& solveFor, double vMin, double vMax, double epsilon,
              int maxIterations) {
    if (v0 < vMin) return 0;
    if (v0 > vMax) return 0;
    int num = 0;
    double num2 = solveFor(v0);
    while (!(dv <= epsilon) && num < maxIterations) {
        double value = solveFor(v0 + dv);
        double value2 = solveFor(v0 - dv);
        if (v0 - dv < vMin) value2 = DBL_MAX;
        if (v0 + dv > vMax) value = DBL_MAX;
        value = std::fabs(value);
        value2 = std::fabs(value2);
        num2 = std::min(num2, std::min(value, value2));
        if (num2 == value2) v0 -= dv;
        else if (num2 == value) v0 += dv;
        dv /= 2.0;
        ++num;
    }
    return num;
}

// UtilMath.cs:223
int BSPSolver(float& v0, float dv, const std::function<float(float)>& solveFor, float vMin, float vMax, float epsilon,
              int maxIterations) {
    if (v0 < vMin) return 0;
    if (v0 > vMax) return 0;
    int num = 0;
    float num2 = solveFor(v0);
    while (!(dv <= epsilon) && num < maxIterations) {  // Mono float rule (dotnet/mono_float.hpp)
        float value = solveFor(f32store(fx(v0) + fx(dv)));
        float value2 = solveFor(f32store(fx(v0) - fx(dv)));
        if (fx(v0) - fx(dv) < fx(vMin)) value2 = FLT_MAX;
        if (fx(v0) + fx(dv) > fx(vMax)) value = FLT_MAX;
        value = std::fabs(value);
        value2 = std::fabs(value2);
        num2 = std::min(num2, std::min(value, value2));
        if (num2 == value2) v0 = f32store(fx(v0) - fx(dv));
        else if (num2 == value) v0 = f32store(fx(v0) + fx(dv));
        dv = f32store(fx(dv) / 2.0);
        ++num;
    }
    return num;
}

// UtilMath.cs:247
bool IsDivisible(int n, int byN) {
    while (byN % 2 == 0) byN /= 2;
    while (byN % 5 == 0) byN /= 5;
    return n % byN == 0;
}

// UtilMath.cs:255
bool Approximately(double a, double b, double epsilon) {
    const double num = std::fabs(a);
    const double num2 = std::fabs(b);
    const double num3 = std::fabs(a - b);
    if (a == b) return true;
    if (a != 0.0 && b != 0.0 && num3 >= 4.9406564584124654E-324) return num3 / (num + num2) < epsilon;
    return num3 < epsilon;
}

// UtilMath.cs:264
bool SphereIntersection(double radius, const Vector3d& position, const Vector3d& velocity, double& time, bool later) {
    const double num = Vector3d::Dot(position, velocity);
    const double num2 = Vector3d::Dot(velocity, velocity);
    const double num3 = Vector3d::Dot(position, position);
    const double num4 = num * num - num2 * (num3 - radius * radius);
    if (num4 >= 0.0 && num2 > 0.0) {
        if (later) time = (0.0 - num + std::sqrt(num4)) / num2;
        else time = (0.0 - num - std::sqrt(num4)) / num2;
        return true;
    }
    time = 0.0;
    return false;
}

// UtilMath.cs:277
bool SphereIntersection(double radius, const Vector3d& position, const Vector3d& velocity, Vector3d& impact, bool later) {
    double time;
    if (SphereIntersection(radius, position, velocity, time, later)) {
        impact = position + velocity * time;
        return true;
    }
    impact = Vector3d::zero();
    return false;
}

// UtilMath.cs:283
double AngleBetween(const Vector3d& v, const Vector3d& w) {
    const Vector3d a = v * w.magnitude();
    const Vector3d b = w * v.magnitude();
    return 2.0 * std::atan2((a - b).magnitude(), (a + b).magnitude());
}

}  // namespace UtilMath

// Planetarium.cs:25 CelestialFrame.SetFrame
void CelestialFrame::SetFrame(double A, double B, double C, CelestialFrame& cf) {
    const double num = dotnet::Math::Cos(A);
    const double num2 = dotnet::Math::Sin(A);
    const double num3 = dotnet::Math::Cos(B);
    const double num4 = dotnet::Math::Sin(B);
    const double num5 = dotnet::Math::Cos(C);
    const double num6 = dotnet::Math::Sin(C);
    cf.X = Vector3d(num * num5 - num2 * num3 * num6, num2 * num5 + num * num3 * num6, num4 * num6);
    cf.Y = Vector3d((0.0 - num) * num6 - num2 * num3 * num5, (0.0 - num2) * num6 + num * num3 * num5, num4 * num5);
    cf.Z = Vector3d(num2 * num4, (0.0 - num) * num4, num3);
}

// Planetarium.cs:37 CelestialFrame.OrbitalFrame (degrees)
void CelestialFrame::OrbitalFrame(double LAN, double Inc, double ArgPe, CelestialFrame& cf) {
    LAN *= UtilMath::PI / 180.0;
    Inc *= UtilMath::PI / 180.0;
    ArgPe *= UtilMath::PI / 180.0;
    SetFrame(LAN, Inc, ArgPe, cf);
}

// Planetarium.cs:45 CelestialFrame.PlanetaryFrame (degrees)
void CelestialFrame::PlanetaryFrame(double ra, double dec, double rot, CelestialFrame& cf) {
    ra = (ra - 90.0) * UtilMath::PI / 180.0;
    dec = (dec - 90.0) * UtilMath::PI / 180.0;
    rot = (rot + 90.0) * UtilMath::PI / 180.0;
    SetFrame(ra, dec, rot, cf);
}

// Planetarium.cs:182 Awake
void PlanetariumAwakeFrames(double inverseRotAngle, CelestialFrame& zup, QuaternionD& rotation) {
    CelestialFrame::PlanetaryFrame(0.0, 90.0, inverseRotAngle, zup);
    rotation = QuaternionD::Inverse(zup.Rotation()).swizzle();
}

}  // namespace ksp
