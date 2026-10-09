// WP-21: C# float32 expression evaluation policies for the aerodynamics port.
//
// KSP's aero maths (DragCubeList, FlightIntegrator.UpdateAerodynamics, ModuleLiftingSurface) is managed float32 code. The
// same IL gives different bits on the two runtimes this project has references for:
//   MonoFloat  Unity's Mono 5.11 (the game): every float expression is evaluated in double and rounded to float32 only where
//              the C# source stores, passes, returns or casts (ksp/dotnet/mono_float.hpp, CAPTURE-SPEC C7). This is the
//              production policy and the one the in-game captures (harness/captures/out*/aero_forces*.json) check.
//   NetFloat   .NET Framework 4.8 RyuJIT x64 (the golden harness): every float operation is an SSE2 single-precision
//              instruction, i.e. rounded to float32 after each operation. Used only to reproduce the harness golden
//              harness/golden/aero_forces (tables that have no in-game capture: drag_chain_kerbin, lifting_surface) with
//              the very same templated port, which proves the operation order of the port against KSP's IL.
// A ported function is a template over the policy P and writes every C# float expression with FE<P>:
//     C#:  retData.exposedArea += num4 / DragCurveMultiplier.Evaluate(machNumber) * num5;
//     C++: retData.exposedArea = S(E(retData.exposedArea) + E(num4) / E(DragCurveMultiplier->Evaluate(machNumber)) * E(num5));
// E(float) widens an operand, S(FE) stores (rounds to float32), D(FE) is the C# widening to double of a float expression
// (Mono: the double value as evaluated, i.e. an exact product stays exact; .NET: the float32 result). Every operator of
// two FE values rounds under NetFloat and does not under MonoFloat. A float literal must be written E(1.0f): mixing FE with a
// bare float is a compile error (it would silently turn into double arithmetic), an FE with a double operand gives double
// (C# promotes float op double to double in both runtimes).
//
// Unity's single-operation helpers (Vector3 operator+ - * /, Mathf.Abs/Min/Max/Sign/Clamp01/Pow/Sqrt) round once per
// component in both runtimes and are taken from ksp/unity unchanged; the multi-operation ones (Vector3.Dot, magnitude,
// normalized, ProjectOnPlane, Mathf.Lerp) differ between the runtimes and are provided here per policy. For MonoFloat they
// equal ksp/unity's functions bit for bit (tests/test_aero.cpp checks that).
#pragma once

#include <cmath>

#include "ksp/dotnet/math.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::aero {

struct MonoFloat {
    static constexpr bool kPerOperationRounding = false;
    static constexpr double R(double v) noexcept { return v; }
};
struct NetFloat {
    static constexpr bool kPerOperationRounding = true;
    static constexpr double R(double v) noexcept { return static_cast<double>(static_cast<float>(v)); }
};

// A C# float32 expression under policy P (its value held exactly as the runtime holds it).
template <class P>
struct FE {
    double v;
    constexpr explicit FE(float f) noexcept : v(static_cast<double>(f)) {}
    FE(double) = delete;  // widen floats explicitly; double operands mix through the operators below
    FE(int) = delete;
    static constexpr FE Raw(double d) noexcept {
        FE e(0.0f);
        e.v = d;
        return e;
    }

    friend constexpr FE operator+(FE a, FE b) noexcept { return Raw(P::R(a.v + b.v)); }
    friend constexpr FE operator-(FE a, FE b) noexcept { return Raw(P::R(a.v - b.v)); }
    friend constexpr FE operator*(FE a, FE b) noexcept { return Raw(P::R(a.v * b.v)); }
    friend constexpr FE operator/(FE a, FE b) noexcept { return Raw(P::R(a.v / b.v)); }
    friend constexpr FE operator-(FE a) noexcept { return Raw(-a.v); }
    // float op double -> double (both runtimes)
    friend constexpr double operator+(FE a, double b) noexcept { return a.v + b; }
    friend constexpr double operator-(FE a, double b) noexcept { return a.v - b; }
    friend constexpr double operator*(FE a, double b) noexcept { return a.v * b; }
    friend constexpr double operator/(FE a, double b) noexcept { return a.v / b; }
    friend constexpr double operator+(double a, FE b) noexcept { return a + b.v; }
    friend constexpr double operator-(double a, FE b) noexcept { return a - b.v; }
    friend constexpr double operator*(double a, FE b) noexcept { return a * b.v; }
    friend constexpr double operator/(double a, FE b) noexcept { return a / b.v; }
    // a bare float operand is a porting error: write E(x)
    friend FE operator+(FE, float) = delete;
    friend FE operator-(FE, float) = delete;
    friend FE operator*(FE, float) = delete;
    friend FE operator/(FE, float) = delete;
    friend FE operator+(float, FE) = delete;
    friend FE operator-(float, FE) = delete;
    friend FE operator*(float, FE) = delete;
    friend FE operator/(float, FE) = delete;
    // comparisons of float expressions happen on the evaluated values
    friend constexpr bool operator<(FE a, FE b) noexcept { return a.v < b.v; }
    friend constexpr bool operator>(FE a, FE b) noexcept { return a.v > b.v; }
    friend constexpr bool operator<=(FE a, FE b) noexcept { return a.v <= b.v; }
    friend constexpr bool operator>=(FE a, FE b) noexcept { return a.v >= b.v; }
    friend constexpr bool operator==(FE a, FE b) noexcept { return a.v == b.v; }
    friend constexpr bool operator!=(FE a, FE b) noexcept { return a.v != b.v; }
};

// Store a float expression (round to float32) / widen it to double as C# does.
template <class P>
constexpr float S(FE<P> e) noexcept { return static_cast<float>(e.v); }
template <class P>
constexpr double D(FE<P> e) noexcept { return e.v; }

// Policy-generic Unity helpers whose float32 result depends on the runtime (multi-operation expressions).
template <class P>
struct U {
    using E = FE<P>;
    // Vector3.cs:325 Dot
    static float Dot(const Vector3f& a, const Vector3f& b) {
        return S(E(a.x) * E(b.x) + E(a.y) * E(b.y) + E(a.z) * E(b.z));
    }
    // Vector3.cs:82 sqrMagnitude
    static float SqrMagnitude(const Vector3f& v) { return S(E(v.x) * E(v.x) + E(v.y) * E(v.y) + E(v.z) * E(v.z)); }
    // Vector3.cs:395 Magnitude: (float)Math.Sqrt(x * x + y * y + z * z)
    static float Magnitude(const Vector3f& v) {
        return static_cast<float>(dotnet::Math::Sqrt(D(E(v.x) * E(v.x) + E(v.y) * E(v.y) + E(v.z) * E(v.z))));
    }
    // Vector3.cs:302 Normalize: num > 1E-05f ? value / num : zero
    static Vector3f Normalize(const Vector3f& v) {
        const float num = Magnitude(v);
        if (num > 1E-05f) return v / num;
        return Vector3f::zero();
    }
    // Vector3.cs:341 ProjectOnPlane
    static Vector3f ProjectOnPlane(const Vector3f& vector, const Vector3f& planeNormal) {
        const float num = Dot(planeNormal, planeNormal);
        if (num < Mathf::Epsilon) return vector;
        const float num2 = Dot(vector, planeNormal);
        return {S(E(vector.x) - E(planeNormal.x) * E(num2) / E(num)), S(E(vector.y) - E(planeNormal.y) * E(num2) / E(num)),
                S(E(vector.z) - E(planeNormal.z) * E(num2) / E(num))};
    }
    // Quaternion.cs operator*(Quaternion lhs, Quaternion rhs)
    static Quaternionf Mul(const Quaternionf& l, const Quaternionf& r) {
        return {S(E(l.w) * E(r.x) + E(l.x) * E(r.w) + E(l.y) * E(r.z) - E(l.z) * E(r.y)),
                S(E(l.w) * E(r.y) + E(l.y) * E(r.w) + E(l.z) * E(r.x) - E(l.x) * E(r.z)),
                S(E(l.w) * E(r.z) + E(l.z) * E(r.w) + E(l.x) * E(r.y) - E(l.y) * E(r.x)),
                S(E(l.w) * E(r.w) - E(l.x) * E(r.x) - E(l.y) * E(r.y) - E(l.z) * E(r.z))};
    }
    // Quaternion.cs operator*(Quaternion rotation, Vector3 point)
    static Vector3f Rotate(const Quaternionf& q, const Vector3f& p) {
        const float num = S(E(q.x) * E(2.0f)), num2 = S(E(q.y) * E(2.0f)), num3 = S(E(q.z) * E(2.0f));
        const float num4 = S(E(q.x) * E(num)), num5 = S(E(q.y) * E(num2)), num6 = S(E(q.z) * E(num3));
        const float num7 = S(E(q.x) * E(num2)), num8 = S(E(q.x) * E(num3)), num9 = S(E(q.y) * E(num3));
        const float num10 = S(E(q.w) * E(num)), num11 = S(E(q.w) * E(num2)), num12 = S(E(q.w) * E(num3));
        const E one(1.0f);
        Vector3f result;
        result.x = S((one - (E(num5) + E(num6))) * E(p.x) + (E(num7) - E(num12)) * E(p.y) + (E(num8) + E(num11)) * E(p.z));
        result.y = S((E(num7) + E(num12)) * E(p.x) + (one - (E(num4) + E(num6))) * E(p.y) + (E(num9) - E(num10)) * E(p.z));
        result.z = S((E(num8) - E(num11)) * E(p.x) + (E(num9) + E(num10)) * E(p.y) + (one - (E(num4) + E(num5))) * E(p.z));
        return result;
    }
    // Quaternion.Inverse is native and policy-independent: WP-01's QuaternionExtensions::Inverse (the conjugate, no division by
    // the squared norm); checked against the 60 captured rows of harness/captures/out/math_shim_native.json in tests/test_aero.cpp
    static Quaternionf Inverse(const Quaternionf& q) { return QuaternionExtensions::Inverse(q); }
    // Vector3.cs:352 Angle (degrees)
    static float Angle(const Vector3f& from, const Vector3f& to) {
        const float num = static_cast<float>(dotnet::Math::Sqrt(D(E(SqrMagnitude(from)) * E(SqrMagnitude(to)))));
        if (num < 1E-15f) return 0.0f;
        const float num2 = Mathf::Clamp(S(E(Dot(from, to)) / E(num)), -1.0f, 1.0f);
        return S(E(static_cast<float>(dotnet::Math::Acos(static_cast<double>(num2)))) * E(57.29578f));
    }
    // Mathf.cs Lerp: a + (b - a) * Clamp01(t)
    static float Lerp(float a, float b, float t) { return S(E(a) + (E(b) - E(a)) * E(Mathf::Clamp01(t))); }
};

}  // namespace ksp::aero
