// UnityEngine.QuaternionD (KSP's double quaternion, Assembly-CSharp/UnityEngine/QuaternionD.cs).
// Only the managed members exist: Euler, FromToRotation, Slerp, Lerp, UnclampedSlerp, AxisAngle and the euler/axis-angle
// conversions are `extern` InternalCalls that Unity never registers (dead in game) and are therefore not provided.
// Quirk kept: Inverse is a plain conjugate (assumes unit quaternions).
#pragma once

#include "ksp/math/vector3d.hpp"
#include "ksp/unity/quaternion.hpp"

namespace ksp {

struct QuaternionD {
    double x = 0.0, y = 0.0, z = 0.0, w = 0.0;

    constexpr QuaternionD() = default;
    constexpr QuaternionD(double x_, double y_, double z_, double w_) : x(x_), y(y_), z(z_), w(w_) {}
    // QuaternionD.cs: QuaternionD(Vector3d X, Vector3d Y, Vector3d Z) -- rotation from basis vectors
    QuaternionD(const Vector3d& X, const Vector3d& Y, const Vector3d& Z);
    // implicit operator QuaternionD(Quaternion)
    constexpr QuaternionD(const Quaternionf& q) : x(q.x), y(q.y), z(q.z), w(q.w) {}  // NOLINT(google-explicit-constructor)
    // implicit operator Quaternion(QuaternionD)
    Quaternionf ToQuaternionf() const {
        return {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), static_cast<float>(w)};
    }

    static constexpr QuaternionD identity() { return {0.0, 0.0, 0.0, 1.0}; }
    QuaternionD swizzle() const { return {0.0 - x, 0.0 - z, 0.0 - y, w}; }
    bool Equals(const QuaternionD& o) const { return x == o.x && y == o.y && z == o.z && w == o.w; }

    void FrameVectors(Vector3d& frameX, Vector3d& frameY, Vector3d& frameZ) const;

    static double Dot(const QuaternionD& a, const QuaternionD& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
    static QuaternionD AngleAxis(double angle, const Vector3d& axis);
    static QuaternionD LookRotation(Vector3d forward, Vector3d up);
    static QuaternionD LookRotation(const Vector3d& forward) { return LookRotation(forward, Vector3d::up()); }
    static QuaternionD Inverse(const QuaternionD& q) { return {0.0 - q.x, 0.0 - q.y, 0.0 - q.z, q.w}; }
    static double Angle(const QuaternionD& a, const QuaternionD& b);
};

QuaternionD operator*(const QuaternionD& lhs, const QuaternionD& rhs);
Vector3d operator*(const QuaternionD& rotation, const Vector3d& point);
inline bool operator==(const QuaternionD& l, const QuaternionD& r) { return l.x == r.x && l.y == r.y && l.z == r.z && l.w == r.w; }
inline bool operator!=(const QuaternionD& l, const QuaternionD& r) {
    if (l.x == r.x && l.y == r.y && l.z == r.z) return l.w != r.w;
    return true;
}

}  // namespace ksp
