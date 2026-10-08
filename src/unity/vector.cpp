// UnityEngine.Vector3 managed functions. Spec: decompiled/KSP-Decompiled/Unity/UnityEngine.CoreModule/UnityEngine/Vector3.cs
// Mono float32 rule (ksp/dotnet/mono_float.hpp): double expressions, f32store() at every C# store/argument/return.
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
    return {f32store(fx(a.x) + (fx(b.x) - fx(a.x)) * fx(t)), f32store(fx(a.y) + (fx(b.y) - fx(a.y)) * fx(t)),
            f32store(fx(a.z) + (fx(b.z) - fx(a.z)) * fx(t))};
}

// Vector3.cs: LerpUnclamped
Vector3f Vector3f::LerpUnclamped(const Vector3f& a, const Vector3f& b, float t) {
    return {f32store(fx(a.x) + (fx(b.x) - fx(a.x)) * fx(t)), f32store(fx(a.y) + (fx(b.y) - fx(a.y)) * fx(t)),
            f32store(fx(a.z) + (fx(b.z) - fx(a.z)) * fx(t))};
}

// Vector3.cs: MoveTowards
Vector3f Vector3f::MoveTowards(const Vector3f& current, const Vector3f& target, float maxDistanceDelta) {
    const float num = f32store(fx(target.x) - fx(current.x));
    const float num2 = f32store(fx(target.y) - fx(current.y));
    const float num3 = f32store(fx(target.z) - fx(current.z));
    const float num4 = f32store(fx(num) * fx(num) + fx(num2) * fx(num2) + fx(num3) * fx(num3));
    if (num4 == 0.0f || (maxDistanceDelta >= 0.0f && fx(num4) <= fx(maxDistanceDelta) * fx(maxDistanceDelta))) return target;
    const float num5 = f32store(std::sqrt(fx(num4)));
    return {f32store(fx(current.x) + fx(num) / fx(num5) * fx(maxDistanceDelta)),
            f32store(fx(current.y) + fx(num2) / fx(num5) * fx(maxDistanceDelta)),
            f32store(fx(current.z) + fx(num3) / fx(num5) * fx(maxDistanceDelta))};
}

// Vector3.cs: Reflect
Vector3f Vector3f::Reflect(const Vector3f& inDirection, const Vector3f& inNormal) {
    const float num = f32store(-2.0 * fx(Dot(inNormal, inDirection)));
    return {f32store(fx(num) * fx(inNormal.x) + fx(inDirection.x)), f32store(fx(num) * fx(inNormal.y) + fx(inDirection.y)),
            f32store(fx(num) * fx(inNormal.z) + fx(inDirection.z))};
}

// Vector3.cs: Project
Vector3f Vector3f::Project(const Vector3f& vector, const Vector3f& onNormal) {
    const float num = Dot(onNormal, onNormal);
    if (num < Mathf::Epsilon) return zero();
    const float num2 = Dot(vector, onNormal);
    return {f32store(fx(onNormal.x) * fx(num2) / fx(num)), f32store(fx(onNormal.y) * fx(num2) / fx(num)),
            f32store(fx(onNormal.z) * fx(num2) / fx(num))};
}

// Vector3.cs: ProjectOnPlane
Vector3f Vector3f::ProjectOnPlane(const Vector3f& vector, const Vector3f& planeNormal) {
    const float num = Dot(planeNormal, planeNormal);
    if (num < Mathf::Epsilon) return vector;
    const float num2 = Dot(vector, planeNormal);
    return {f32store(fx(vector.x) - fx(planeNormal.x) * fx(num2) / fx(num)),
            f32store(fx(vector.y) - fx(planeNormal.y) * fx(num2) / fx(num)),
            f32store(fx(vector.z) - fx(planeNormal.z) * fx(num2) / fx(num))};
}

// Vector3.cs: Angle
float Vector3f::Angle(const Vector3f& from, const Vector3f& to) {
    const float num = f32store(std::sqrt(fx(from.sqrMagnitude()) * fx(to.sqrMagnitude())));
    if (num < 1E-15f) return 0.0f;
    const float num2 = Mathf::Clamp(f32store(fx(Dot(from, to)) / fx(num)), -1.0f, 1.0f);
    return f32store(fx(f32store(dotnet::Math::Acos(fx(num2)))) * fx(57.29578f));
}

// Vector3.cs: SignedAngle
float Vector3f::SignedAngle(const Vector3f& from, const Vector3f& to, const Vector3f& axis) {
    const float num = Angle(from, to);
    const float num2 = f32store(fx(from.y) * fx(to.z) - fx(from.z) * fx(to.y));
    const float num3 = f32store(fx(from.z) * fx(to.x) - fx(from.x) * fx(to.z));
    const float num4 = f32store(fx(from.x) * fx(to.y) - fx(from.y) * fx(to.x));
    const float num5 = Mathf::Sign(f32store(fx(axis.x) * fx(num2) + fx(axis.y) * fx(num3) + fx(axis.z) * fx(num4)));
    return f32store(fx(num) * fx(num5));
}

// Vector3.cs: AngleBetween (obsolete, radians)
float Vector3f::AngleBetween(const Vector3f& from, const Vector3f& to) {
    return f32store(dotnet::Math::Acos(fx(Mathf::Clamp(Dot(from.normalized(), to.normalized()), -1.0f, 1.0f))));
}

// Vector3.cs: Distance
float Vector3f::Distance(const Vector3f& a, const Vector3f& b) {
    const float num = f32store(fx(a.x) - fx(b.x)), num2 = f32store(fx(a.y) - fx(b.y)), num3 = f32store(fx(a.z) - fx(b.z));
    return f32store(std::sqrt(fx(num) * fx(num) + fx(num2) * fx(num2) + fx(num3) * fx(num3)));
}

// Vector3.cs: ClampMagnitude
Vector3f Vector3f::ClampMagnitude(const Vector3f& vector, float maxLength) {
    const float num = vector.sqrMagnitude();
    if (fx(num) > fx(maxLength) * fx(maxLength)) {
        const float num2 = f32store(std::sqrt(fx(num)));
        const float num3 = f32store(fx(vector.x) / fx(num2));
        const float num4 = f32store(fx(vector.y) / fx(num2));
        const float num5 = f32store(fx(vector.z) / fx(num2));
        return {f32store(fx(num3) * fx(maxLength)), f32store(fx(num4) * fx(maxLength)), f32store(fx(num5) * fx(maxLength))};
    }
    return vector;
}

// Vector3.cs: SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime)
Vector3f Vector3f::SmoothDamp(const Vector3f& current, Vector3f target, Vector3f& currentVelocity, float smoothTime,
                              float maxSpeed, float deltaTime) {
    float num = 0.0f, num2 = 0.0f, num3 = 0.0f;
    smoothTime = Mathf::Max(0.0001f, smoothTime);
    const float num4 = f32store(2.0 / fx(smoothTime));
    const float num5 = f32store(fx(num4) * fx(deltaTime));
    const float num6 = f32store(1.0 / (1.0 + fx(num5) + fx(0.48f) * fx(num5) * fx(num5) +
                                       fx(0.235f) * fx(num5) * fx(num5) * fx(num5)));
    float num7 = f32store(fx(current.x) - fx(target.x));
    float num8 = f32store(fx(current.y) - fx(target.y));
    float num9 = f32store(fx(current.z) - fx(target.z));
    const Vector3f vector = target;
    const float num10 = f32store(fx(maxSpeed) * fx(smoothTime));
    const float num11 = f32store(fx(num10) * fx(num10));
    const float num12 = f32store(fx(num7) * fx(num7) + fx(num8) * fx(num8) + fx(num9) * fx(num9));
    if (num12 > num11) {
        const float num13 = f32store(std::sqrt(fx(num12)));
        num7 = f32store(fx(num7) / fx(num13) * fx(num10));
        num8 = f32store(fx(num8) / fx(num13) * fx(num10));
        num9 = f32store(fx(num9) / fx(num13) * fx(num10));
    }
    target.x = f32store(fx(current.x) - fx(num7));
    target.y = f32store(fx(current.y) - fx(num8));
    target.z = f32store(fx(current.z) - fx(num9));
    const float num14 = f32store((fx(currentVelocity.x) + fx(num4) * fx(num7)) * fx(deltaTime));
    const float num15 = f32store((fx(currentVelocity.y) + fx(num4) * fx(num8)) * fx(deltaTime));
    const float num16 = f32store((fx(currentVelocity.z) + fx(num4) * fx(num9)) * fx(deltaTime));
    currentVelocity.x = f32store((fx(currentVelocity.x) - fx(num4) * fx(num14)) * fx(num6));
    currentVelocity.y = f32store((fx(currentVelocity.y) - fx(num4) * fx(num15)) * fx(num6));
    currentVelocity.z = f32store((fx(currentVelocity.z) - fx(num4) * fx(num16)) * fx(num6));
    num = f32store(fx(target.x) + (fx(num7) + fx(num14)) * fx(num6));
    num2 = f32store(fx(target.y) + (fx(num8) + fx(num15)) * fx(num6));
    num3 = f32store(fx(target.z) + (fx(num9) + fx(num16)) * fx(num6));
    const float num17 = f32store(fx(vector.x) - fx(current.x));
    const float num18 = f32store(fx(vector.y) - fx(current.y));
    const float num19 = f32store(fx(vector.z) - fx(current.z));
    const float num20 = f32store(fx(num) - fx(vector.x));
    const float num21 = f32store(fx(num2) - fx(vector.y));
    const float num22 = f32store(fx(num3) - fx(vector.z));
    if (fx(num17) * fx(num20) + fx(num18) * fx(num21) + fx(num19) * fx(num22) > 0.0) {
        num = vector.x;
        num2 = vector.y;
        num3 = vector.z;
        currentVelocity.x = f32store((fx(num) - fx(vector.x)) / fx(deltaTime));
        currentVelocity.y = f32store((fx(num2) - fx(vector.y)) / fx(deltaTime));
        currentVelocity.z = f32store((fx(num3) - fx(vector.z)) / fx(deltaTime));
    }
    return {num, num2, num3};
}

}  // namespace ksp
