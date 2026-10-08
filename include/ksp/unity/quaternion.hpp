// UnityEngine.Quaternion (float32), managed parts of Unity 2019.4.18f1.
// Spec: decompiled/KSP-Decompiled/Unity/UnityEngine.CoreModule/UnityEngine/Quaternion.cs
//
// Unity implements AngleAxis, LookRotation, Euler, FromToRotation, Slerp, Lerp, Inverse natively (C++ in
// UnityPlayer.dll); those are not ported yet (CAPTURE-SPEC C3 will provide golden data). operator== is Unity's
// dot-product test (Dot > 0.999999f); Equals() is exact. Managed float arithmetic follows Mono's rule
// (ksp/dotnet/mono_float.hpp).
#pragma once

#include "ksp/dotnet/mono_float.hpp"
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
    static float Dot(const Quaternionf& a, const Quaternionf& b) {
        return f32store(fx(a.x) * fx(b.x) + fx(a.y) * fx(b.y) + fx(a.z) * fx(b.z) + fx(a.w) * fx(b.w));
    }
    // Quaternion.cs: IsEqualUsingDot
    static bool IsEqualUsingDot(float dot) { return dot > 0.999999f; }
    // Quaternion.cs: Angle
    static float Angle(const Quaternionf& a, const Quaternionf& b) {
        const float num = Dot(a, b);
        return IsEqualUsingDot(num) ? 0.0f : f32store(fx(Mathf::Acos(Mathf::Min(Mathf::Abs(num), 1.0f))) * 2.0 * fx(57.29578f));
    }
    // Quaternion.cs: Normalize
    static Quaternionf Normalize(const Quaternionf& q) {
        const float num = Mathf::Sqrt(Dot(q, q));
        if (num < Mathf::Epsilon) return identity();
        return {f32store(fx(q.x) / fx(num)), f32store(fx(q.y) / fx(num)), f32store(fx(q.z) / fx(num)), f32store(fx(q.w) / fx(num))};
    }
    Quaternionf normalized() const { return Normalize(*this); }
};

// Quaternion.cs: operator*(Quaternion, Quaternion)
inline Quaternionf operator*(const Quaternionf& lhs, const Quaternionf& rhs) {
    const double lx = lhs.x, ly = lhs.y, lz = lhs.z, lw = lhs.w, rx = rhs.x, ry = rhs.y, rz = rhs.z, rw = rhs.w;
    return {f32store(lw * rx + lx * rw + ly * rz - lz * ry), f32store(lw * ry + ly * rw + lz * rx - lx * rz),
            f32store(lw * rz + lz * rw + lx * ry - ly * rx), f32store(lw * rw - lx * rx - ly * ry - lz * rz)};
}

// Quaternion.cs: operator*(Quaternion, Vector3)
inline Vector3f operator*(const Quaternionf& rotation, const Vector3f& point) {
    const float num = f32store(fx(rotation.x) * 2.0);
    const float num2 = f32store(fx(rotation.y) * 2.0);
    const float num3 = f32store(fx(rotation.z) * 2.0);
    const double n4 = f32store(fx(rotation.x) * fx(num));
    const double n5 = f32store(fx(rotation.y) * fx(num2));
    const double n6 = f32store(fx(rotation.z) * fx(num3));
    const double n7 = f32store(fx(rotation.x) * fx(num2));
    const double n8 = f32store(fx(rotation.x) * fx(num3));
    const double n9 = f32store(fx(rotation.y) * fx(num3));
    const double n10 = f32store(fx(rotation.w) * fx(num));
    const double n11 = f32store(fx(rotation.w) * fx(num2));
    const double n12 = f32store(fx(rotation.w) * fx(num3));
    const double px = point.x, py = point.y, pz = point.z;
    Vector3f result;
    result.x = f32store((1.0 - (n5 + n6)) * px + (n7 - n12) * py + (n8 + n11) * pz);
    result.y = f32store((n7 + n12) * px + (1.0 - (n4 + n6)) * py + (n9 - n10) * pz);
    result.z = f32store((n8 - n11) * px + (n9 + n10) * py + (1.0 - (n4 + n5)) * pz);
    return result;
}

inline bool operator==(const Quaternionf& l, const Quaternionf& r) { return Quaternionf::IsEqualUsingDot(Quaternionf::Dot(l, r)); }
inline bool operator!=(const Quaternionf& l, const Quaternionf& r) { return !(l == r); }

}  // namespace ksp
