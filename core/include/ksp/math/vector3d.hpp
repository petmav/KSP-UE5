// KSP double-precision vectors: Vector3d (Assembly-CSharp/Vector3d.cs), Vector2d and Vector4d
// (Assembly-CSharp/UnityEngine/Vector2d.cs, Vector4d.cs). Ported operation-for-operation; quirks kept:
//  * Vector2d operator!= is true only when BOTH components differ (Vector2d.cs operator!=).
//  * Vector4d operator!= is true only when ALL components differ (Vector4d.cs operator!=).
//  * Vector3d.Project uses the float epsilon 1.401298464324817E-45 as its zero test.
// Native-only members (Slerp, RotateTowards, OrthoNormalize: InternalCalls that Unity never registers) are omitted.
#pragma once

#include <algorithm>
#include <cmath>

#include "ksp/unity/vector.hpp"

namespace ksp {

struct Vector3d {
    double x = 0.0, y = 0.0, z = 0.0;

    constexpr Vector3d() = default;
    constexpr Vector3d(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    constexpr Vector3d(double x_, double y_) : x(x_), y(y_), z(0.0) {}
    // Vector3d.cs implicit operator Vector3d(Vector3)
    constexpr Vector3d(const Vector3f& v) : x(v.x), y(v.y), z(v.z) {}  // NOLINT(google-explicit-constructor)
    // Vector3d.cs implicit operator Vector3(Vector3d)
    Vector3f ToVector3f() const { return {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)}; }

    static constexpr Vector3d zero() { return {0.0, 0.0, 0.0}; }
    static constexpr Vector3d one() { return {1.0, 1.0, 1.0}; }
    static constexpr Vector3d forward() { return {0.0, 0.0, 1.0}; }
    static constexpr Vector3d back() { return {0.0, 0.0, -1.0}; }
    static constexpr Vector3d up() { return {0.0, 1.0, 0.0}; }
    static constexpr Vector3d down() { return {0.0, -1.0, 0.0}; }
    static constexpr Vector3d left() { return {-1.0, 0.0, 0.0}; }
    static constexpr Vector3d right() { return {1.0, 0.0, 0.0}; }

    Vector3d xzy() const { return {x, z, y}; }
    void Swizzle() { std::swap(y, z); }
    double operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
    double& operator[](int i) { return i == 0 ? x : (i == 1 ? y : z); }

    double magnitude() const { return std::sqrt(x * x + y * y + z * z); }
    double sqrMagnitude() const { return x * x + y * y + z * z; }
    Vector3d normalized() const { return Normalize(*this); }
    void Normalize() { *this = Normalize(*this); }
    void Zero() { x = y = z = 0.0; }
    bool IsZero() const { return x == 0.0 && y == 0.0 && z == 0.0; }
    bool Equals(const Vector3d& o) const { return x == o.x && y == o.y && z == o.z; }

    static double Magnitude(const Vector3d& a) { return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z); }
    static double SqrMagnitude(const Vector3d& a) { return a.x * a.x + a.y * a.y + a.z * a.z; }
    static double Dot(const Vector3d& l, const Vector3d& r) { return l.x * r.x + l.y * r.y + l.z * r.z; }
    static Vector3d Cross(const Vector3d& l, const Vector3d& r) {
        return {l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x};
    }
    static Vector3d Scale(const Vector3d& a, const Vector3d& b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
    static Vector3d Normalize(const Vector3d& value);
    static Vector3d Lerp(const Vector3d& from, const Vector3d& to, double t);
    static Vector3d Reflect(const Vector3d& inDirection, const Vector3d& inNormal);
    static Vector3d Project(const Vector3d& vector, const Vector3d& onNormal);
    static Vector3d Exclude(const Vector3d& excludeThis, const Vector3d& fromThat);
    static double Angle(const Vector3d& from, const Vector3d& to);
    static double AngleBetween(const Vector3d& from, const Vector3d& to);
    static double Distance(const Vector3d& a, const Vector3d& b);
    static Vector3d Min(const Vector3d& l, const Vector3d& r) { return {std::min(l.x, r.x), std::min(l.y, r.y), std::min(l.z, r.z)}; }
    static Vector3d Max(const Vector3d& l, const Vector3d& r) { return {std::max(l.x, r.x), std::max(l.y, r.y), std::max(l.z, r.z)}; }
    Vector3d Basis(const Vector3d& X, const Vector3d& Y, const Vector3d& Z) const;
};

inline Vector3d operator+(const Vector3d& a, const Vector3d& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vector3d operator-(const Vector3d& a, const Vector3d& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vector3d operator-(const Vector3d& a) { return {0.0 - a.x, 0.0 - a.y, 0.0 - a.z}; }
inline Vector3d operator*(const Vector3d& a, double d) { return {a.x * d, a.y * d, a.z * d}; }
inline Vector3d operator*(double d, const Vector3d& a) { return {a.x * d, a.y * d, a.z * d}; }
inline Vector3d operator/(const Vector3d& a, double d) { return {a.x / d, a.y / d, a.z / d}; }
// Vector3d.cs mixed float/double operators: the float operand is widened first.
inline Vector3d operator+(const Vector3f& a, const Vector3d& b) { return {double(a.x) + b.x, double(a.y) + b.y, double(a.z) + b.z}; }
inline Vector3d operator+(const Vector3d& a, const Vector3f& b) { return {a.x + double(b.x), a.y + double(b.y), a.z + double(b.z)}; }
inline Vector3d operator-(const Vector3f& a, const Vector3d& b) { return {double(a.x) - b.x, double(a.y) - b.y, double(a.z) - b.z}; }
inline Vector3d operator-(const Vector3d& a, const Vector3f& b) { return {a.x - double(b.x), a.y - double(b.y), a.z - double(b.z)}; }
inline Vector3d& operator+=(Vector3d& a, const Vector3d& b) { return a = a + b; }
inline Vector3d& operator-=(Vector3d& a, const Vector3d& b) { return a = a - b; }
inline Vector3d& operator*=(Vector3d& a, double d) { return a = a * d; }
inline Vector3d& operator/=(Vector3d& a, double d) { return a = a / d; }
inline bool operator==(const Vector3d& l, const Vector3d& r) { return l.x == r.x && l.y == r.y && l.z == r.z; }
inline bool operator!=(const Vector3d& l, const Vector3d& r) {
    if (l.x == r.x && l.y == r.y) return l.z != r.z;
    return true;
}

struct Vector2d {
    double x = 0.0, y = 0.0;
    constexpr Vector2d() = default;
    constexpr Vector2d(double x_, double y_) : x(x_), y(y_) {}
    static constexpr Vector2d zero() { return {0.0, 0.0}; }
    static constexpr Vector2d one() { return {1.0, 1.0}; }
    static constexpr Vector2d up() { return {0.0, 1.0}; }
    static constexpr Vector2d right() { return {1.0, 0.0}; }
    double magnitude() const { return std::sqrt(x * x + y * y); }
    double sqrMagnitude() const { return x * x + y * y; }
    Vector2d normalized() const {
        Vector2d r(x, y);
        r.Normalize();
        return r;
    }
    void Normalize();
    static double Dot(const Vector2d& l, const Vector2d& r) { return l.x * r.x + l.y * r.y; }
    static Vector2d Lerp(const Vector2d& from, const Vector2d& to, double t);
    static Vector2d MoveTowards(const Vector2d& current, const Vector2d& target, double maxDistanceDelta);
    static double Angle(const Vector2d& from, const Vector2d& to);
    static Vector2d ClampMagnitude(const Vector2d& v, double maxLength);
    bool Equals(const Vector2d& o) const { return x == o.x && y == o.y; }
};
inline Vector2d operator+(const Vector2d& a, const Vector2d& b) { return {a.x + b.x, a.y + b.y}; }
inline Vector2d operator-(const Vector2d& a, const Vector2d& b) { return {a.x - b.x, a.y - b.y}; }
inline Vector2d operator-(const Vector2d& a) { return {0.0 - a.x, 0.0 - a.y}; }
inline Vector2d operator*(const Vector2d& a, double d) { return {a.x * d, a.y * d}; }
inline Vector2d operator*(double d, const Vector2d& a) { return {a.x * d, a.y * d}; }
inline Vector2d operator/(const Vector2d& a, double d) { return {a.x / d, a.y / d}; }
inline bool operator==(const Vector2d& l, const Vector2d& r) { return l.x == r.x && l.y == r.y; }
inline bool operator!=(const Vector2d& l, const Vector2d& r) {
    if (l.x != r.x) return l.y != r.y;  // sic (Vector2d.cs)
    return false;
}

struct Vector4d {
    double x = 0.0, y = 0.0, z = 0.0, w = 0.0;
    constexpr Vector4d() = default;
    constexpr Vector4d(double x_, double y_, double z_, double w_) : x(x_), y(y_), z(z_), w(w_) {}
    constexpr Vector4d(double x_, double y_, double z_) : x(x_), y(y_), z(z_), w(0.0) {}
    static constexpr Vector4d zero() { return {0.0, 0.0, 0.0, 0.0}; }
    static double Dot(const Vector4d& a, const Vector4d& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
    static double Magnitude(const Vector4d& a) { return std::sqrt(Dot(a, a)); }
    double magnitude() const { return Magnitude(*this); }
    static Vector4d Normalize(const Vector4d& a);
    bool Equals(const Vector4d& o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }
};
inline Vector4d operator+(const Vector4d& a, const Vector4d& b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
inline Vector4d operator-(const Vector4d& a, const Vector4d& b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
inline Vector4d operator*(const Vector4d& a, double d) { return {a.x * d, a.y * d, a.z * d, a.w * d}; }
inline Vector4d operator/(const Vector4d& a, double d) { return {a.x / d, a.y / d, a.z / d, a.w / d}; }
inline bool operator==(const Vector4d& l, const Vector4d& r) { return l.x == r.x && l.y == r.y && l.z == r.z && l.w == r.w; }
inline bool operator!=(const Vector4d& l, const Vector4d& r) {
    if (l.x != r.x && l.y != r.y && l.z != r.z) return l.w != r.w;  // sic (Vector4d.cs)
    return false;
}

}  // namespace ksp
