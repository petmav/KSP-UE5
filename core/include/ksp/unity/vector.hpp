// UnityEngine.Vector2 / Vector3 / Vector4 (float32), managed parts of Unity 2019.4.18f1.
// Spec: decompiled/KSP-Decompiled/Unity/UnityEngine.CoreModule/UnityEngine/Vector{2,3,4}.cs
//
// Semantics notes:
//  * operator== is Unity's approximate comparison (squared distance < 9.9999994E-11f for Vector3); use Equals() for the
//    exact component comparison (Vector3.Equals).
//  * Native-only functions (Slerp, RotateTowards, OrthoNormalize) are not provided; they need CAPTURE-SPEC C3.
//  * Managed float arithmetic follows Mono's rule (ksp/dotnet/mono_float.hpp): double expressions, rounded where the
//    C# source stores, passes or returns a float (constructor arguments included).
#pragma once

#include <cmath>

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

struct Vector3f {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    static constexpr float kEpsilon = 1E-05f;
    static constexpr float kEpsilonNormalSqrt = 1E-15f;

    constexpr Vector3f() = default;
    constexpr Vector3f(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    constexpr Vector3f(float x_, float y_) : x(x_), y(y_), z(0.0f) {}

    static constexpr Vector3f zero() { return {0.0f, 0.0f, 0.0f}; }
    static constexpr Vector3f one() { return {1.0f, 1.0f, 1.0f}; }
    static constexpr Vector3f up() { return {0.0f, 1.0f, 0.0f}; }
    static constexpr Vector3f down() { return {0.0f, -1.0f, 0.0f}; }
    static constexpr Vector3f left() { return {-1.0f, 0.0f, 0.0f}; }
    static constexpr Vector3f right() { return {1.0f, 0.0f, 0.0f}; }
    static constexpr Vector3f forward() { return {0.0f, 0.0f, 1.0f}; }
    static constexpr Vector3f back() { return {0.0f, 0.0f, -1.0f}; }

    float operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
    float& operator[](int i) { return i == 0 ? x : (i == 1 ? y : z); }

    // Vector3.cs: magnitude = (float)Math.Sqrt(x*x + y*y + z*z)
    // Vector3.cs: (float)Math.Sqrt(x * x + y * y + z * z) -- the sum is a double expression, never rounded to float
    float magnitude() const { return f32store(std::sqrt(fx(x) * fx(x) + fx(y) * fx(y) + fx(z) * fx(z))); }
    float sqrMagnitude() const { return f32store(fx(x) * fx(x) + fx(y) * fx(y) + fx(z) * fx(z)); }
    Vector3f normalized() const;

    bool Equals(const Vector3f& o) const { return x == o.x && y == o.y && z == o.z; }

    static float Magnitude(const Vector3f& v) { return v.magnitude(); }
    static float SqrMagnitude(const Vector3f& v) { return v.sqrMagnitude(); }
    static Vector3f Normalize(const Vector3f& v);
    static float Dot(const Vector3f& a, const Vector3f& b) {
        return f32store(fx(a.x) * fx(b.x) + fx(a.y) * fx(b.y) + fx(a.z) * fx(b.z));
    }
    static Vector3f Cross(const Vector3f& l, const Vector3f& r) {
        return {f32store(fx(l.y) * fx(r.z) - fx(l.z) * fx(r.y)), f32store(fx(l.z) * fx(r.x) - fx(l.x) * fx(r.z)),
                f32store(fx(l.x) * fx(r.y) - fx(l.y) * fx(r.x))};
    }
    static Vector3f Scale(const Vector3f& a, const Vector3f& b) {
        return {f32store(fx(a.x) * fx(b.x)), f32store(fx(a.y) * fx(b.y)), f32store(fx(a.z) * fx(b.z))};
    }
    static Vector3f Lerp(const Vector3f& a, const Vector3f& b, float t);
    static Vector3f LerpUnclamped(const Vector3f& a, const Vector3f& b, float t);
    static Vector3f MoveTowards(const Vector3f& current, const Vector3f& target, float maxDistanceDelta);
    static Vector3f Reflect(const Vector3f& inDirection, const Vector3f& inNormal);
    static Vector3f Project(const Vector3f& vector, const Vector3f& onNormal);
    static Vector3f ProjectOnPlane(const Vector3f& vector, const Vector3f& planeNormal);
    static Vector3f Exclude(const Vector3f& excludeThis, const Vector3f& fromThat) { return ProjectOnPlane(fromThat, excludeThis); }
    static float Angle(const Vector3f& from, const Vector3f& to);
    static float SignedAngle(const Vector3f& from, const Vector3f& to, const Vector3f& axis);
    static float AngleBetween(const Vector3f& from, const Vector3f& to);
    static float Distance(const Vector3f& a, const Vector3f& b);
    static Vector3f ClampMagnitude(const Vector3f& vector, float maxLength);
    static Vector3f Min(const Vector3f& a, const Vector3f& b) { return {Mathf::Min(a.x, b.x), Mathf::Min(a.y, b.y), Mathf::Min(a.z, b.z)}; }
    static Vector3f Max(const Vector3f& a, const Vector3f& b) { return {Mathf::Max(a.x, b.x), Mathf::Max(a.y, b.y), Mathf::Max(a.z, b.z)}; }
    static Vector3f SmoothDamp(const Vector3f& current, Vector3f target, Vector3f& currentVelocity, float smoothTime,
                               float maxSpeed, float deltaTime);
};

// Vector3.cs operators: each component is a float constructor argument (one rounding of the double result)
inline Vector3f operator+(const Vector3f& a, const Vector3f& b) {
    return {f32store(fx(a.x) + fx(b.x)), f32store(fx(a.y) + fx(b.y)), f32store(fx(a.z) + fx(b.z))};
}
inline Vector3f operator-(const Vector3f& a, const Vector3f& b) {
    return {f32store(fx(a.x) - fx(b.x)), f32store(fx(a.y) - fx(b.y)), f32store(fx(a.z) - fx(b.z))};
}
// Vector3.cs:425 shows `new Vector3(0f - a.x, ...)`, which is how ILSpy prints the IL `neg` instruction: the operator is an IEEE
// negation (-(+0) = -0). Evidence (WP-21): harness/golden/aero_forces lifting_surface writes lift_x = -0.0 for
// `-liftVector * liftScalar` with liftVector.x = +0 (ModuleLiftingSurface.cs:248).
inline Vector3f operator-(const Vector3f& a) { return {-a.x, -a.y, -a.z}; }
inline Vector3f operator*(const Vector3f& a, float d) {
    return {f32store(fx(a.x) * fx(d)), f32store(fx(a.y) * fx(d)), f32store(fx(a.z) * fx(d))};
}
inline Vector3f operator*(float d, const Vector3f& a) { return a * d; }
inline Vector3f operator/(const Vector3f& a, float d) {
    return {f32store(fx(a.x) / fx(d)), f32store(fx(a.y) / fx(d)), f32store(fx(a.z) / fx(d))};
}
inline Vector3f& operator+=(Vector3f& a, const Vector3f& b) { return a = a + b; }
inline Vector3f& operator-=(Vector3f& a, const Vector3f& b) { return a = a - b; }
inline Vector3f& operator*=(Vector3f& a, float d) { return a = a * d; }
inline Vector3f& operator/=(Vector3f& a, float d) { return a = a / d; }
// Vector3.cs operator== (approximate)
inline bool operator==(const Vector3f& l, const Vector3f& r) {
    const float n = f32store(fx(l.x) - fx(r.x)), n2 = f32store(fx(l.y) - fx(r.y)), n3 = f32store(fx(l.z) - fx(r.z));
    const float n4 = f32store(fx(n) * fx(n) + fx(n2) * fx(n2) + fx(n3) * fx(n3));
    return n4 < 9.9999994E-11f;
}
inline bool operator!=(const Vector3f& l, const Vector3f& r) { return !(l == r); }

struct Vector2f {
    float x = 0.0f, y = 0.0f;
    constexpr Vector2f() = default;
    constexpr Vector2f(float x_, float y_) : x(x_), y(y_) {}
    static constexpr Vector2f zero() { return {0.0f, 0.0f}; }
    float magnitude() const { return f32store(std::sqrt(fx(x) * fx(x) + fx(y) * fx(y))); }
    float sqrMagnitude() const { return f32store(fx(x) * fx(x) + fx(y) * fx(y)); }
    bool Equals(const Vector2f& o) const { return x == o.x && y == o.y; }
};
inline Vector2f operator+(const Vector2f& a, const Vector2f& b) { return {f32store(fx(a.x) + fx(b.x)), f32store(fx(a.y) + fx(b.y))}; }
inline Vector2f operator-(const Vector2f& a, const Vector2f& b) { return {f32store(fx(a.x) - fx(b.x)), f32store(fx(a.y) - fx(b.y))}; }
inline Vector2f operator*(const Vector2f& a, float d) { return {f32store(fx(a.x) * fx(d)), f32store(fx(a.y) * fx(d))}; }
inline Vector2f operator/(const Vector2f& a, float d) { return {f32store(fx(a.x) / fx(d)), f32store(fx(a.y) / fx(d))}; }
// Vector2.cs operator== (approximate): return num * num + num2 * num2 < 9.9999994E-11f (compare in double)
inline bool operator==(const Vector2f& l, const Vector2f& r) {
    const float n = f32store(fx(l.x) - fx(r.x)), n2 = f32store(fx(l.y) - fx(r.y));
    return fx(n) * fx(n) + fx(n2) * fx(n2) < fx(9.9999994E-11f);
}
inline bool operator!=(const Vector2f& l, const Vector2f& r) { return !(l == r); }

struct Vector4f {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    constexpr Vector4f() = default;
    constexpr Vector4f(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    static constexpr Vector4f zero() { return {0.0f, 0.0f, 0.0f, 0.0f}; }
    // Vector4.cs: Dot(a, a) (float return) / (float)Math.Sqrt(Dot(a, a))
    float sqrMagnitude() const { return f32store(fx(x) * fx(x) + fx(y) * fx(y) + fx(z) * fx(z) + fx(w) * fx(w)); }
    float magnitude() const { return f32store(std::sqrt(fx(sqrMagnitude()))); }
    bool Equals(const Vector4f& o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
};
inline Vector4f operator+(const Vector4f& a, const Vector4f& b) {
    return {f32store(fx(a.x) + fx(b.x)), f32store(fx(a.y) + fx(b.y)), f32store(fx(a.z) + fx(b.z)), f32store(fx(a.w) + fx(b.w))};
}
inline Vector4f operator-(const Vector4f& a, const Vector4f& b) {
    return {f32store(fx(a.x) - fx(b.x)), f32store(fx(a.y) - fx(b.y)), f32store(fx(a.z) - fx(b.z)), f32store(fx(a.w) - fx(b.w))};
}
inline Vector4f operator*(const Vector4f& a, float d) {
    return {f32store(fx(a.x) * fx(d)), f32store(fx(a.y) * fx(d)), f32store(fx(a.z) * fx(d)), f32store(fx(a.w) * fx(d))};
}
inline Vector4f operator/(const Vector4f& a, float d) {
    return {f32store(fx(a.x) / fx(d)), f32store(fx(a.y) / fx(d)), f32store(fx(a.z) / fx(d)), f32store(fx(a.w) / fx(d))};
}
// Vector4.cs operator== (approximate)
inline bool operator==(const Vector4f& l, const Vector4f& r) {
    const float n = f32store(fx(l.x) - fx(r.x)), n2 = f32store(fx(l.y) - fx(r.y)), n3 = f32store(fx(l.z) - fx(r.z)),
                n4 = f32store(fx(l.w) - fx(r.w));
    const float n5 = f32store(fx(n) * fx(n) + fx(n2) * fx(n2) + fx(n3) * fx(n3) + fx(n4) * fx(n4));
    return n5 < 9.9999994E-11f;
}
inline bool operator!=(const Vector4f& l, const Vector4f& r) { return !(l == r); }

}  // namespace ksp
