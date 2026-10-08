// UnityEngine.Quaternion (float32), managed parts of Unity 2019.4.18f1.
// Spec: decompiled/KSP-Decompiled/Unity/UnityEngine.CoreModule/UnityEngine/Quaternion.cs
//
// Unity implements AngleAxis, LookRotation, Euler, FromToRotation, Slerp, Lerp, Inverse natively (C++ in
// UnityPlayer.dll); those are not ported yet (CAPTURE-SPEC C3 will provide golden data). operator== is Unity's
// dot-product test (Dot > 0.999999f); Equals() is exact.
#pragma once

#include "ksp/unity/mathf.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

struct Quaternionf {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;

    constexpr Quaternionf() = default;
    constexpr Quaternionf(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    static constexpr Quaternionf identity() { return {0.0f, 0.0f, 0.0f, 1.0f}; }

    bool Equals(const Quaternionf& o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }

    // Quaternion.cs: Dot
    static float Dot(const Quaternionf& a, const Quaternionf& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
    // Quaternion.cs: IsEqualUsingDot
    static bool IsEqualUsingDot(float dot) { return dot > 0.999999f; }
    // Quaternion.cs: Angle
    static float Angle(const Quaternionf& a, const Quaternionf& b) {
        const float num = Dot(a, b);
        return IsEqualUsingDot(num) ? 0.0f : (Mathf::Acos(Mathf::Min(Mathf::Abs(num), 1.0f)) * 2.0f * 57.29578f);
    }
    // Quaternion.cs: Normalize
    static Quaternionf Normalize(const Quaternionf& q) {
        const float num = Mathf::Sqrt(Dot(q, q));
        if (num < Mathf::Epsilon) return identity();
        return {q.x / num, q.y / num, q.z / num, q.w / num};
    }
    Quaternionf normalized() const { return Normalize(*this); }
};

// Quaternion.cs: operator*(Quaternion, Quaternion)
inline Quaternionf operator*(const Quaternionf& lhs, const Quaternionf& rhs) {
    return {lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x,
            lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z};
}

// Quaternion.cs: operator*(Quaternion, Vector3)
inline Vector3f operator*(const Quaternionf& rotation, const Vector3f& point) {
    const float num = rotation.x * 2.0f;
    const float num2 = rotation.y * 2.0f;
    const float num3 = rotation.z * 2.0f;
    const float num4 = rotation.x * num;
    const float num5 = rotation.y * num2;
    const float num6 = rotation.z * num3;
    const float num7 = rotation.x * num2;
    const float num8 = rotation.x * num3;
    const float num9 = rotation.y * num3;
    const float num10 = rotation.w * num;
    const float num11 = rotation.w * num2;
    const float num12 = rotation.w * num3;
    Vector3f result;
    result.x = (1.0f - (num5 + num6)) * point.x + (num7 - num12) * point.y + (num8 + num11) * point.z;
    result.y = (num7 + num12) * point.x + (1.0f - (num4 + num6)) * point.y + (num9 - num10) * point.z;
    result.z = (num8 - num11) * point.x + (num9 + num10) * point.y + (1.0f - (num4 + num5)) * point.z;
    return result;
}

inline bool operator==(const Quaternionf& l, const Quaternionf& r) { return Quaternionf::IsEqualUsingDot(Quaternionf::Dot(l, r)); }
inline bool operator!=(const Quaternionf& l, const Quaternionf& r) { return !(l == r); }

}  // namespace ksp
