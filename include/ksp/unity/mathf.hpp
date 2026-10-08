// UnityEngine.Mathf (float32), managed implementation of Unity 2019.4.18f1.
// Spec: decompiled/KSP-Decompiled/Unity/UnityEngine.CoreModule/UnityEngine/Mathf.cs
//
// Unity's Mathf transcendental functions are (float)System.Math.F((double)x); System.Math maps to the C runtime, so the
// results here use the MSVC UCRT (the golden harness used the .NET Framework CRT, Unity's Mono uses the UCRT too).
// Arithmetic is IEEE float32 in source order (MSVC x64 /fp:precise: SSE scalar, no contraction).
#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

#include "ksp/dotnet/math.hpp"

namespace ksp::Mathf {

inline constexpr float PI = 3.14159274f;  // (float)Math.PI
inline constexpr float Infinity = std::numeric_limits<float>::infinity();
inline constexpr float NegativeInfinity = -std::numeric_limits<float>::infinity();
inline constexpr float Deg2Rad = PI / 180.0f;  // (float)Math.PI / 180f
inline constexpr float Rad2Deg = 57.29578f;
// Mathf.Epsilon: FloatMinDenormal unless flush-to-zero is enabled (it is not, in KSP or in kspcore).
inline constexpr float Epsilon = 1.401298E-45f;

// Mathf.cs:68-106
inline float Sin(float f) { return static_cast<float>(dotnet::Math::Sin(static_cast<double>(f))); }
inline float Cos(float f) { return static_cast<float>(dotnet::Math::Cos(static_cast<double>(f))); }
inline float Tan(float f) { return static_cast<float>(dotnet::Math::Tan(static_cast<double>(f))); }
inline float Asin(float f) { return static_cast<float>(std::asin(static_cast<double>(f))); }
inline float Acos(float f) { return static_cast<float>(std::acos(static_cast<double>(f))); }
inline float Atan(float f) { return static_cast<float>(std::atan(static_cast<double>(f))); }
inline float Atan2(float y, float x) { return static_cast<float>(std::atan2(static_cast<double>(y), static_cast<double>(x))); }
inline float Sqrt(float f) { return static_cast<float>(std::sqrt(static_cast<double>(f))); }
// Mathf.cs:108 Math.Abs(float)
inline float Abs(float f) { return std::fabs(f); }
inline int Abs(int v) { return v < 0 ? -v : v; }
// Mathf.cs:118, 164
inline float Min(float a, float b) { return (a < b) ? a : b; }
inline int Min(int a, int b) { return (a < b) ? a : b; }
inline float Max(float a, float b) { return (a > b) ? a : b; }
inline int Max(int a, int b) { return (a > b) ? a : b; }
// Mathf.cs:210-233
inline float Pow(float f, float p) { return static_cast<float>(std::pow(static_cast<double>(f), static_cast<double>(p))); }
inline float Exp(float power) { return static_cast<float>(std::exp(static_cast<double>(power))); }
inline float Log(float f, float p) { return static_cast<float>(dotnet::Math::Log(static_cast<double>(f), static_cast<double>(p))); }
inline float Log(float f) { return static_cast<float>(std::log(static_cast<double>(f))); }
inline float Log10(float f) { return static_cast<float>(std::log10(static_cast<double>(f))); }
// Mathf.cs:235-263
inline float Ceil(float f) { return static_cast<float>(dotnet::Math::Ceiling(static_cast<double>(f))); }
inline float Floor(float f) { return static_cast<float>(std::floor(static_cast<double>(f))); }
inline float Round(float f) { return static_cast<float>(dotnet::Math::Round(static_cast<double>(f))); }
inline int CeilToInt(float f) { return dotnet::ConvToInt32(dotnet::Math::Ceiling(static_cast<double>(f))); }
inline int FloorToInt(float f) { return dotnet::ConvToInt32(std::floor(static_cast<double>(f))); }
inline int RoundToInt(float f) { return dotnet::ConvToInt32(dotnet::Math::Round(static_cast<double>(f))); }
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
// Mathf.cs:309-317
inline float Lerp(float a, float b, float t) { return a + (b - a) * Clamp01(t); }
inline float LerpUnclamped(float a, float b, float t) { return a + (b - a) * t; }

// Mathf.cs:435
inline float Repeat(float t, float length) { return Clamp(t - Floor(t / length) * length, 0.0f, length); }
// Mathf.cs:440
inline float PingPong(float t, float length) {
    t = Repeat(t, length * 2.0f);
    return length - Abs(t - length);
}
// Mathf.cs:446
inline float InverseLerp(float a, float b, float value) {
    if (a != b) return Clamp01((value - a) / (b - a));
    return 0.0f;
}
// Mathf.cs:455
inline float DeltaAngle(float current, float target) {
    float num = Repeat(target - current, 360.0f);
    if (num > 180.0f) num -= 360.0f;
    return num;
}
// Mathf.cs:319
inline float LerpAngle(float a, float b, float t) {
    float num = Repeat(b - a, 360.0f);
    if (num > 180.0f) num -= 360.0f;
    return a + num * Clamp01(t);
}
// Mathf.cs:329
inline float MoveTowards(float current, float target, float maxDelta) {
    if (Abs(target - current) <= maxDelta) return target;
    return current + Sign(target - current) * maxDelta;
}
// Mathf.cs:338
inline float MoveTowardsAngle(float current, float target, float maxDelta) {
    const float num = DeltaAngle(current, target);
    if (0.0f - maxDelta < num && num < maxDelta) return target;
    target = current + num;
    return MoveTowards(current, target, maxDelta);
}
// Mathf.cs:349
inline float SmoothStep(float from, float to, float t) {
    t = Clamp01(t);
    t = -2.0f * t * t * t + 3.0f * t * t;
    return to * t + from * (1.0f - t);
}
// Mathf.cs:356
inline float Gamma(float value, float absmax, float gamma) {
    const bool neg = value < 0.0f;
    const float num = Abs(value);
    if (num > absmax) return neg ? (0.0f - num) : num;
    const float num2 = Pow(num / absmax, gamma) * absmax;
    return neg ? (0.0f - num2) : num2;
}
// Mathf.cs:372
inline bool Approximately(float a, float b) { return Abs(b - a) < Max(1E-06f * Max(Abs(a), Abs(b)), Epsilon * 8.0f); }

// Mathf.cs:392 SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime)
inline float SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed,
                        float deltaTime) {
    smoothTime = Max(0.0001f, smoothTime);
    const float num = 2.0f / smoothTime;
    const float num2 = num * deltaTime;
    const float num3 = 1.0f / (1.0f + num2 + 0.48f * num2 * num2 + 0.235f * num2 * num2 * num2);
    float value = current - target;
    const float num4 = target;
    const float num5 = maxSpeed * smoothTime;
    value = Clamp(value, 0.0f - num5, num5);
    target = current - value;
    const float num6 = (currentVelocity + num * value) * deltaTime;
    currentVelocity = (currentVelocity - num * num6) * num3;
    float num7 = target + (value + num6) * num3;
    if ((num4 - current > 0.0f) == (num7 > num4)) {
        num7 = num4;
        currentVelocity = (num7 - num4) / deltaTime;
    }
    return num7;
}
// Mathf.cs:429
inline float SmoothDampAngle(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed,
                             float deltaTime) {
    target = current + DeltaAngle(current, target);
    return SmoothDamp(current, target, currentVelocity, smoothTime, maxSpeed, deltaTime);
}

}  // namespace ksp::Mathf
