// UnityEngine.Mathf (float32), managed implementation of Unity 2019.4.18f1.
// Spec: decompiled/KSP-Decompiled/Unity/UnityEngine.CoreModule/UnityEngine/Mathf.cs
//
// Evaluated with Mono's float32 rule (ksp/dotnet/mono_float.hpp): expressions in double, rounded at stores, arguments
// and returns. Transcendentals are (float)System.Math.F((double)x) via ksp::dotnet::Math (runtime-specific semantics,
// e.g. Pow(-0, -3) = +Infinity on Mono).
#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"

namespace ksp::Mathf {

inline constexpr float PI = 3.14159274f;  // (float)Math.PI
inline constexpr float Infinity = std::numeric_limits<float>::infinity();
inline constexpr float NegativeInfinity = -std::numeric_limits<float>::infinity();
inline constexpr float Deg2Rad = 0.0174532924f;  // (float)Math.PI / 180f (compile-time constant)
inline constexpr float Rad2Deg = 57.29578f;
// Mathf.Epsilon: FloatMinDenormal unless flush-to-zero is enabled (it is not, in KSP or in kspcore).
inline constexpr float Epsilon = 1.401298E-45f;

// Mathf.cs:68-106
inline float Sin(float f) { return f32store(dotnet::Math::Sin(fx(f))); }
inline float Cos(float f) { return f32store(dotnet::Math::Cos(fx(f))); }
inline float Tan(float f) { return f32store(dotnet::Math::Tan(fx(f))); }
inline float Asin(float f) { return f32store(dotnet::Math::Asin(fx(f))); }
inline float Acos(float f) { return f32store(dotnet::Math::Acos(fx(f))); }
inline float Atan(float f) { return f32store(dotnet::Math::Atan(fx(f))); }
inline float Atan2(float y, float x) { return f32store(dotnet::Math::Atan2(fx(y), fx(x))); }
inline float Sqrt(float f) { return f32store(dotnet::Math::Sqrt(fx(f))); }
// Mathf.cs:108 Math.Abs(float)
inline float Abs(float f) { return std::fabs(f); }
inline int Abs(int v) { return v < 0 ? -v : v; }
// Mathf.cs:118, 164
inline float Min(float a, float b) { return (a < b) ? a : b; }
inline int Min(int a, int b) { return (a < b) ? a : b; }
inline float Max(float a, float b) { return (a > b) ? a : b; }
inline int Max(int a, int b) { return (a > b) ? a : b; }
// Mathf.cs:210-233
inline float Pow(float f, float p) { return f32store(dotnet::Math::Pow(fx(f), fx(p))); }
inline float Exp(float power) { return f32store(dotnet::Math::Exp(fx(power))); }
inline float Log(float f, float p) { return f32store(dotnet::Math::Log(fx(f), fx(p))); }
inline float Log(float f) { return f32store(dotnet::Math::Log(fx(f))); }
inline float Log10(float f) { return f32store(dotnet::Math::Log10(fx(f))); }
// Mathf.cs:235-263
inline float Ceil(float f) { return f32store(dotnet::Math::Ceiling(fx(f))); }
inline float Floor(float f) { return f32store(dotnet::Math::Floor(fx(f))); }
inline float Round(float f) { return f32store(dotnet::Math::Round(fx(f))); }
inline int CeilToInt(float f) { return dotnet::ConvToInt32(dotnet::Math::Ceiling(fx(f))); }
inline int FloorToInt(float f) { return dotnet::ConvToInt32(dotnet::Math::Floor(fx(f))); }
inline int RoundToInt(float f) { return dotnet::ConvToInt32(dotnet::Math::Round(fx(f))); }
// Mathf.cs:265 (Sign(0) == 1, Sign(-0) == 1, Sign(NaN) == -1)
inline float Sign(float f) { return (f >= 0.0f) ? 1.0f : -1.0f; }

// Mathf.cs:270
inline float Clamp(float value, float min, float max) {
    if (value < min) value = min;
    else if (value > max) value = max;
    return value;
}
inline int Clamp(int value, int min, int max) {
    if (value < min) value = min;
    else if (value > max) value = max;
    return value;
}
// Mathf.cs:296
inline float Clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}
// Mathf.cs:309 return a + (b - a) * Clamp01(t);
inline float Lerp(float a, float b, float t) { return f32store(fx(a) + (fx(b) - fx(a)) * fx(Clamp01(t))); }
// Mathf.cs:314 return a + (b - a) * t;
inline float LerpUnclamped(float a, float b, float t) { return f32store(fx(a) + (fx(b) - fx(a)) * fx(t)); }

// Mathf.cs:435 return Clamp(t - Floor(t / length) * length, 0f, length);
inline float Repeat(float t, float length) {
    return Clamp(f32store(fx(t) - fx(Floor(f32store(fx(t) / fx(length)))) * fx(length)), 0.0f, length);
}
// Mathf.cs:440
inline float PingPong(float t, float length) {
    t = Repeat(t, f32store(fx(length) * 2.0));
    return f32store(fx(length) - fx(Abs(f32store(fx(t) - fx(length)))));
}
// Mathf.cs:446
inline float InverseLerp(float a, float b, float value) {
    if (a != b) return Clamp01(f32store((fx(value) - fx(a)) / (fx(b) - fx(a))));
    return 0.0f;
}
// Mathf.cs:455
inline float DeltaAngle(float current, float target) {
    float num = Repeat(f32store(fx(target) - fx(current)), 360.0f);
    if (num > 180.0f) num = f32store(fx(num) - 360.0);
    return num;
}
// Mathf.cs:319
inline float LerpAngle(float a, float b, float t) {
    float num = Repeat(f32store(fx(b) - fx(a)), 360.0f);
    if (num > 180.0f) num = f32store(fx(num) - 360.0);
    return f32store(fx(a) + fx(num) * fx(Clamp01(t)));
}
// Mathf.cs:329
inline float MoveTowards(float current, float target, float maxDelta) {
    if (Abs(f32store(fx(target) - fx(current))) <= maxDelta) return target;
    return f32store(fx(current) + fx(Sign(f32store(fx(target) - fx(current)))) * fx(maxDelta));
}
// Mathf.cs:338
inline float MoveTowardsAngle(float current, float target, float maxDelta) {
    const float num = DeltaAngle(current, target);
    if (0.0 - fx(maxDelta) < fx(num) && num < maxDelta) return target;
    target = f32store(fx(current) + fx(num));
    return MoveTowards(current, target, maxDelta);
}
// Mathf.cs:349
inline float SmoothStep(float from, float to, float t) {
    t = Clamp01(t);
    t = f32store(-2.0 * fx(t) * fx(t) * fx(t) + 3.0 * fx(t) * fx(t));
    return f32store(fx(to) * fx(t) + fx(from) * (1.0 - fx(t)));
}
// Mathf.cs:356
inline float Gamma(float value, float absmax, float gamma) {
    const bool neg = value < 0.0f;
    const float num = Abs(value);
    if (num > absmax) return neg ? (0.0f - num) : num;
    const float num2 = f32store(fx(Pow(f32store(fx(num) / fx(absmax)), gamma)) * fx(absmax));
    return neg ? (0.0f - num2) : num2;
}
// Mathf.cs:372 return Abs(b - a) < Max(1E-06f * Max(Abs(a), Abs(b)), Epsilon * 8f);
inline bool Approximately(float a, float b) {
    return Abs(f32store(fx(b) - fx(a))) <
           Max(f32store(fx(1E-06f) * fx(Max(Abs(a), Abs(b)))), f32store(fx(Epsilon) * 8.0));
}

// Mathf.cs:392 SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime)
inline float SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed,
                        float deltaTime) {
    smoothTime = Max(0.0001f, smoothTime);
    const float num = f32store(2.0 / fx(smoothTime));
    const float num2 = f32store(fx(num) * fx(deltaTime));
    const float num3 = f32store(1.0 / (1.0 + fx(num2) + fx(0.48f) * fx(num2) * fx(num2) +
                                       fx(0.235f) * fx(num2) * fx(num2) * fx(num2)));
    float value = f32store(fx(current) - fx(target));
    const float num4 = target;
    const float num5 = f32store(fx(maxSpeed) * fx(smoothTime));
    value = Clamp(value, f32store(0.0 - fx(num5)), num5);
    target = f32store(fx(current) - fx(value));
    const float num6 = f32store((fx(currentVelocity) + fx(num) * fx(value)) * fx(deltaTime));
    currentVelocity = f32store((fx(currentVelocity) - fx(num) * fx(num6)) * fx(num3));
    float num7 = f32store(fx(target) + (fx(value) + fx(num6)) * fx(num3));
    if ((fx(num4) - fx(current) > 0.0) == (num7 > num4)) {
        num7 = num4;
        currentVelocity = f32store((fx(num7) - fx(num4)) / fx(deltaTime));
    }
    return num7;
}
// Mathf.cs:429
inline float SmoothDampAngle(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed,
                             float deltaTime) {
    target = f32store(fx(current) + fx(DeltaAngle(current, target)));
    return SmoothDamp(current, target, currentVelocity, smoothTime, maxSpeed, deltaTime);
}

}  // namespace ksp::Mathf
