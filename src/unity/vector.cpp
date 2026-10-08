// UnityEngine.Vector3 managed functions. Spec: decompiled/KSP-Decompiled/Unity/UnityEngine.CoreModule/UnityEngine/Vector3.cs
#include "ksp/unity/vector.hpp"

namespace ksp {

// Vector3.cs: Normalize(Vector3) -- num = Magnitude(value); num > 1E-05f ? value / num : zero
Vector3f Vector3f::Normalize(const Vector3f& value) {
    const float num = Magnitude(value);
    if (num > 1E-05f) return value / num;
    return zero();
}

Vector3f Vector3f::normalized() const { return Normalize(*this); }

// Vector3.cs: Lerp
Vector3f Vector3f::Lerp(const Vector3f& a, const Vector3f& b, float t) {
    t = Mathf::Clamp01(t);
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

// Vector3.cs: LerpUnclamped
Vector3f Vector3f::LerpUnclamped(const Vector3f& a, const Vector3f& b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t};
}

// Vector3.cs: MoveTowards
Vector3f Vector3f::MoveTowards(const Vector3f& current, const Vector3f& target, float maxDistanceDelta) {
    const float num = target.x - current.x;
    const float num2 = target.y - current.y;
    const float num3 = target.z - current.z;
    const float num4 = num * num + num2 * num2 + num3 * num3;
    if (num4 == 0.0f || (maxDistanceDelta >= 0.0f && num4 <= maxDistanceDelta * maxDistanceDelta)) return target;
    const float num5 = static_cast<float>(std::sqrt(static_cast<double>(num4)));
    return {current.x + num / num5 * maxDistanceDelta, current.y + num2 / num5 * maxDistanceDelta,
            current.z + num3 / num5 * maxDistanceDelta};
}

// Vector3.cs: Reflect
Vector3f Vector3f::Reflect(const Vector3f& inDirection, const Vector3f& inNormal) {
    const float num = -2.0f * Dot(inNormal, inDirection);
    return {num * inNormal.x + inDirection.x, num * inNormal.y + inDirection.y, num * inNormal.z + inDirection.z};
}

// Vector3.cs: Project
Vector3f Vector3f::Project(const Vector3f& vector, const Vector3f& onNormal) {
    const float num = Dot(onNormal, onNormal);
    if (num < Mathf::Epsilon) return zero();
    const float num2 = Dot(vector, onNormal);
    return {onNormal.x * num2 / num, onNormal.y * num2 / num, onNormal.z * num2 / num};
}

// Vector3.cs: ProjectOnPlane
Vector3f Vector3f::ProjectOnPlane(const Vector3f& vector, const Vector3f& planeNormal) {
    const float num = Dot(planeNormal, planeNormal);
    if (num < Mathf::Epsilon) return vector;
    const float num2 = Dot(vector, planeNormal);
    return {vector.x - planeNormal.x * num2 / num, vector.y - planeNormal.y * num2 / num,
            vector.z - planeNormal.z * num2 / num};
}

// Vector3.cs: Angle
float Vector3f::Angle(const Vector3f& from, const Vector3f& to) {
    const float num = static_cast<float>(std::sqrt(static_cast<double>(from.sqrMagnitude() * to.sqrMagnitude())));
    if (num < 1E-15f) return 0.0f;
    const float num2 = Mathf::Clamp(Dot(from, to) / num, -1.0f, 1.0f);
    return static_cast<float>(std::acos(static_cast<double>(num2))) * 57.29578f;
}

// Vector3.cs: SignedAngle
float Vector3f::SignedAngle(const Vector3f& from, const Vector3f& to, const Vector3f& axis) {
    const float num = Angle(from, to);
    const float num2 = from.y * to.z - from.z * to.y;
    const float num3 = from.z * to.x - from.x * to.z;
    const float num4 = from.x * to.y - from.y * to.x;
    const float num5 = Mathf::Sign(axis.x * num2 + axis.y * num3 + axis.z * num4);
    return num * num5;
}

// Vector3.cs: AngleBetween (obsolete, radians)
float Vector3f::AngleBetween(const Vector3f& from, const Vector3f& to) {
    return static_cast<float>(
        std::acos(static_cast<double>(Mathf::Clamp(Dot(from.normalized(), to.normalized()), -1.0f, 1.0f))));
}

// Vector3.cs: Distance
float Vector3f::Distance(const Vector3f& a, const Vector3f& b) {
    const float num = a.x - b.x, num2 = a.y - b.y, num3 = a.z - b.z;
    return static_cast<float>(std::sqrt(static_cast<double>(num * num + num2 * num2 + num3 * num3)));
}

// Vector3.cs: ClampMagnitude
Vector3f Vector3f::ClampMagnitude(const Vector3f& vector, float maxLength) {
    const float num = vector.sqrMagnitude();
    if (num > maxLength * maxLength) {
        const float num2 = static_cast<float>(std::sqrt(static_cast<double>(num)));
        const float num3 = vector.x / num2;
        const float num4 = vector.y / num2;
        const float num5 = vector.z / num2;
        return {num3 * maxLength, num4 * maxLength, num5 * maxLength};
    }
    return vector;
}

// Vector3.cs: SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime)
Vector3f Vector3f::SmoothDamp(const Vector3f& current, Vector3f target, Vector3f& currentVelocity, float smoothTime,
                              float maxSpeed, float deltaTime) {
    float num = 0.0f, num2 = 0.0f, num3 = 0.0f;
    smoothTime = Mathf::Max(0.0001f, smoothTime);
    const float num4 = 2.0f / smoothTime;
    const float num5 = num4 * deltaTime;
    const float num6 = 1.0f / (1.0f + num5 + 0.48f * num5 * num5 + 0.235f * num5 * num5 * num5);
    float num7 = current.x - target.x;
    float num8 = current.y - target.y;
    float num9 = current.z - target.z;
    const Vector3f vector = target;
    const float num10 = maxSpeed * smoothTime;
    const float num11 = num10 * num10;
    const float num12 = num7 * num7 + num8 * num8 + num9 * num9;
    if (num12 > num11) {
        const float num13 = static_cast<float>(std::sqrt(static_cast<double>(num12)));
        num7 = num7 / num13 * num10;
        num8 = num8 / num13 * num10;
        num9 = num9 / num13 * num10;
    }
    target.x = current.x - num7;
    target.y = current.y - num8;
    target.z = current.z - num9;
    const float num14 = (currentVelocity.x + num4 * num7) * deltaTime;
    const float num15 = (currentVelocity.y + num4 * num8) * deltaTime;
    const float num16 = (currentVelocity.z + num4 * num9) * deltaTime;
    currentVelocity.x = (currentVelocity.x - num4 * num14) * num6;
    currentVelocity.y = (currentVelocity.y - num4 * num15) * num6;
    currentVelocity.z = (currentVelocity.z - num4 * num16) * num6;
    num = target.x + (num7 + num14) * num6;
    num2 = target.y + (num8 + num15) * num6;
    num3 = target.z + (num9 + num16) * num6;
    const float num17 = vector.x - current.x;
    const float num18 = vector.y - current.y;
    const float num19 = vector.z - current.z;
    const float num20 = num - vector.x;
    const float num21 = num2 - vector.y;
    const float num22 = num3 - vector.z;
    if (num17 * num20 + num18 * num21 + num19 * num22 > 0.0f) {
        num = vector.x;
        num2 = vector.y;
        num3 = vector.z;
        currentVelocity.x = (num - vector.x) / deltaTime;
        currentVelocity.y = (num2 - vector.y) / deltaTime;
        currentVelocity.z = (num3 - vector.z) / deltaTime;
    }
    return {num, num2, num3};
}

}  // namespace ksp
