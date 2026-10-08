// UnityEngine.Vector2 / Vector3 / Vector4 (float32), managed parts of Unity 2019.4.18f1.
// Spec: decompiled/KSP-Decompiled/Unity/UnityEngine.CoreModule/UnityEngine/Vector{2,3,4}.cs
//
// Semantics notes:
//  * operator== is Unity's approximate comparison (squared distance < 9.9999994E-11f for Vector3); use Equals() for the
//    exact component comparison (Vector3.Equals).
//  * Native-only functions (Slerp, RotateTowards, OrthoNormalize) are not provided; they need CAPTURE-SPEC C3.
#pragma once

#include <cmath>

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
    float magnitude() const { return static_cast<float>(std::sqrt(static_cast<double>(x * x + y * y + z * z))); }
    float sqrMagnitude() const { return x * x + y * y + z * z; }
    Vector3f normalized() const;

    bool Equals(const Vector3f& o) const { return x == o.x && y == o.y && z == o.z; }

    static float Magnitude(const Vector3f& v) { return v.magnitude(); }
    static float SqrMagnitude(const Vector3f& v) { return v.sqrMagnitude(); }
    static Vector3f Normalize(const Vector3f& v);
    static float Dot(const Vector3f& a, const Vector3f& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    static Vector3f Cross(const Vector3f& l, const Vector3f& r) {
        return {l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x};
    }
    static Vector3f Scale(const Vector3f& a, const Vector3f& b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
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

inline Vector3f operator+(const Vector3f& a, const Vector3f& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vector3f operator-(const Vector3f& a, const Vector3f& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vector3f operator-(const Vector3f& a) { return {0.0f - a.x, 0.0f - a.y, 0.0f - a.z}; }
inline Vector3f operator*(const Vector3f& a, float d) { return {a.x * d, a.y * d, a.z * d}; }
inline Vector3f operator*(float d, const Vector3f& a) { return {a.x * d, a.y * d, a.z * d}; }
inline Vector3f operator/(const Vector3f& a, float d) { return {a.x / d, a.y / d, a.z / d}; }
inline Vector3f& operator+=(Vector3f& a, const Vector3f& b) { return a = a + b; }
inline Vector3f& operator-=(Vector3f& a, const Vector3f& b) { return a = a - b; }
inline Vector3f& operator*=(Vector3f& a, float d) { return a = a * d; }
inline Vector3f& operator/=(Vector3f& a, float d) { return a = a / d; }
// Vector3.cs operator== (approximate)
inline bool operator==(const Vector3f& l, const Vector3f& r) {
    const float n = l.x - r.x, n2 = l.y - r.y, n3 = l.z - r.z;
    return n * n + n2 * n2 + n3 * n3 < 9.9999994E-11f;
}
inline bool operator!=(const Vector3f& l, const Vector3f& r) { return !(l == r); }

struct Vector2f {
    float x = 0.0f, y = 0.0f;
    constexpr Vector2f() = default;
    constexpr Vector2f(float x_, float y_) : x(x_), y(y_) {}
    static constexpr Vector2f zero() { return {0.0f, 0.0f}; }
    float magnitude() const { return static_cast<float>(std::sqrt(static_cast<double>(x * x + y * y))); }
    float sqrMagnitude() const { return x * x + y * y; }
    bool Equals(const Vector2f& o) const { return x == o.x && y == o.y; }
};
inline Vector2f operator+(const Vector2f& a, const Vector2f& b) { return {a.x + b.x, a.y + b.y}; }
inline Vector2f operator-(const Vector2f& a, const Vector2f& b) { return {a.x - b.x, a.y - b.y}; }
inline Vector2f operator*(const Vector2f& a, float d) { return {a.x * d, a.y * d}; }
inline Vector2f operator/(const Vector2f& a, float d) { return {a.x / d, a.y / d}; }
// Vector2.cs operator== (approximate)
inline bool operator==(const Vector2f& l, const Vector2f& r) {
    const float n = l.x - r.x, n2 = l.y - r.y;
    return n * n + n2 * n2 < 9.9999994E-11f;
}
inline bool operator!=(const Vector2f& l, const Vector2f& r) { return !(l == r); }

struct Vector4f {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
    constexpr Vector4f() = default;
    constexpr Vector4f(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    static constexpr Vector4f zero() { return {0.0f, 0.0f, 0.0f, 0.0f}; }
    float sqrMagnitude() const { return x * x + y * y + z * z + w * w; }
    float magnitude() const { return static_cast<float>(std::sqrt(static_cast<double>(x * x + y * y + z * z + w * w))); }
    bool Equals(const Vector4f& o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
};
inline Vector4f operator+(const Vector4f& a, const Vector4f& b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
inline Vector4f operator-(const Vector4f& a, const Vector4f& b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
inline Vector4f operator*(const Vector4f& a, float d) { return {a.x * d, a.y * d, a.z * d, a.w * d}; }
inline Vector4f operator/(const Vector4f& a, float d) { return {a.x / d, a.y / d, a.z / d, a.w / d}; }
// Vector4.cs operator== (approximate)
inline bool operator==(const Vector4f& l, const Vector4f& r) {
    const float n = l.x - r.x, n2 = l.y - r.y, n3 = l.z - r.z, n4 = l.w - r.w;
    return n * n + n2 * n2 + n3 * n3 + n4 * n4 < 9.9999994E-11f;
}
inline bool operator!=(const Vector4f& l, const Vector4f& r) { return !(l == r); }

}  // namespace ksp
