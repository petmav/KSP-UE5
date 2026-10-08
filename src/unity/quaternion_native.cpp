// Native Unity quaternion functions (see include/ksp/unity/quaternion_native.hpp).
#include "ksp/unity/quaternion_native.hpp"

#include <cmath>

namespace ksp::UnityNative {

// QuaternionScripting::AngleAxis -> AxisAngleToQuaternionSafe(axis, Deg2Rad(angle)) (Runtime/Math/Quaternion.h)
Quaternionf AngleAxis(float angle, const Vector3f& axis) {
    // Unity's Deg2Rad (Runtime/Math/FloatConversion.h) keeps a historical operation order: deg / 360 * 2 * kPI.
    const float rad = angle / 360.0f * 2.0f * 3.14159265358979323846f;
    const float mag = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (mag > 0.000001f) {
        const float halfAngle = rad * 0.5f;
        Quaternionf q;
        q.w = std::cos(halfAngle);
        const float s = std::sin(halfAngle) / mag;
        q.x = s * axis.x;
        q.y = s * axis.y;
        q.z = s * axis.z;
        return q;
    }
    return Quaternionf::identity();
}

}  // namespace ksp::UnityNative
