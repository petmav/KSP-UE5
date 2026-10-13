// WP-29a: the per-operation float32 natives of the wheel seam (see ksp/wheels/wheel_native.hpp). No C# source exists for these:
// UNVERIFIED against the game (STATUS.md WP-29a).
#include "ksp/wheels/wheel_native.hpp"

namespace ksp::VehiclePhysics {

// Unity Runtime/Math/Quaternion.h RotateVectorByQuat: float32 per operation.
Vector3f NativeRotate(const Quaternionf& q, const Vector3f& v) {
    const float x = q.x * 2.0f;
    const float y = q.y * 2.0f;
    const float z = q.z * 2.0f;
    const float xx = q.x * x;
    const float yy = q.y * y;
    const float zz = q.z * z;
    const float xy = q.x * y;
    const float xz = q.x * z;
    const float yz = q.y * z;
    const float wx = q.w * x;
    const float wy = q.w * y;
    const float wz = q.w * z;
    Vector3f r;
    r.x = (1.0f - (yy + zz)) * v.x + (xy - wz) * v.y + (xz + wy) * v.z;
    r.y = (xy + wz) * v.x + (1.0f - (xx + zz)) * v.y + (yz - wx) * v.z;
    r.z = (xz - wy) * v.x + (yz + wx) * v.y + (1.0f - (xx + yy)) * v.z;
    return r;
}

Vector3f NativeCross(const Vector3f& a, const Vector3f& b) {
    Vector3f r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

Vector3f NativeTransform::up() const { return NativeRotate(rotation, Vector3f(0.0f, 1.0f, 0.0f)); }
Vector3f NativeTransform::forward() const { return NativeRotate(rotation, Vector3f(0.0f, 0.0f, 1.0f)); }
Vector3f NativeTransform::right() const { return NativeRotate(rotation, Vector3f(1.0f, 0.0f, 0.0f)); }

Vector3f NativeTransform::TransformPoint(const Vector3f& p) const {
    const Vector3f r = NativeRotate(rotation, p);
    return Vector3f(r.x + position.x, r.y + position.y, r.z + position.z);
}

Vector3f NativeTransform::InverseTransformPoint(const Vector3f& p) const {
    const Vector3f d(p.x - position.x, p.y - position.y, p.z - position.z);
    return NativeRotate(Quaternionf(-rotation.x, -rotation.y, -rotation.z, rotation.w), d);
}

Vector3f NativeTransform::InverseTransformDirection(const Vector3f& d) const {
    return NativeRotate(Quaternionf(-rotation.x, -rotation.y, -rotation.z, rotation.w), d);
}

Vector3f NativeRigidbody::GetPointVelocity(const Vector3f& p) const {
    const Vector3f d(p.x - worldCenterOfMass.x, p.y - worldCenterOfMass.y, p.z - worldCenterOfMass.z);
    const Vector3f c = NativeCross(angularVelocity, d);
    return Vector3f(velocity.x + c.x, velocity.y + c.y, velocity.z + c.z);
}

}  // namespace ksp::VehiclePhysics
