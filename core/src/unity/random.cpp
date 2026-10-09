// UnityEngine.Random: see include/ksp/unity/random.hpp. Formulas: harness/captures/README.md "C2" (fitted 100% against
// the in-game capture), operation order from harness/captures/tools/random_vec_fit.py.
#include "ksp/unity/random.hpp"

#include <cmath>

#include "ksp/dotnet/ucrt_libm.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace {
constexpr float kTwoPi = 6.28318548f;  // (float)(2 * pi)
}

UnityRandom& UnityRandom::Global() {
    static UnityRandom g;
    return g;
}

void UnityRandom::InitState(int32_t seed) {
    s_.s0 = static_cast<uint32_t>(seed);
    s_.s1 = s_.s0 * 1812433253u + 1u;
    s_.s2 = s_.s1 * 1812433253u + 1u;
    s_.s3 = s_.s2 * 1812433253u + 1u;
}

uint32_t UnityRandom::NextUInt() {
    uint32_t t = s_.s0 ^ (s_.s0 << 11);
    t ^= t >> 8;
    s_.s0 = s_.s1;
    s_.s1 = s_.s2;
    s_.s2 = s_.s3;
    s_.s3 = s_.s3 ^ (s_.s3 >> 19) ^ t;
    return s_.s3;
}

float UnityRandom::value() { return static_cast<float>(NextUInt() & 0x7FFFFFu) / 8388607.0f; }

float UnityRandom::Range(float min, float max) {
    const float v = value();
    return v * min + (1.0f - v) * max;
}

int32_t UnityRandom::Range(int32_t min, int32_t max) {
    if (max > min) {
        const uint32_t span = static_cast<uint32_t>(max) - static_cast<uint32_t>(min);
        return static_cast<int32_t>(static_cast<uint32_t>(min) + (NextUInt() % span));
    }
    if (max < min) {
        const uint32_t span = static_cast<uint32_t>(min) - static_cast<uint32_t>(max);
        return static_cast<int32_t>(static_cast<uint32_t>(min) - (NextUInt() % span));
    }
    return min;
}

// z = Range(-1,1); a = Range(0, 2pi); r = sqrtf(1 - z*z); (r cos a, r sin a, z)
Vector3f UnityRandom::onUnitSphere() {
    const float z = Range(-1.0f, 1.0f);
    const float a = Range(0.0f, kTwoPi);
    const float r = std::sqrt(1.0f - z * z);
    return {r * dotnet::NativeCosf(a), r * dotnet::NativeSinf(a), z};
}

// onUnitSphere * powf(value, 1/3)
Vector3f UnityRandom::insideUnitSphere() {
    const Vector3f u = onUnitSphere();
    const float s = dotnet::NativePowf(value(), 0.333333343f);
    return {u.x * s, u.y * s, u.z * s};
}

// theta = Range(0, 2pi); r = sqrtf(Range(0,1)); (r cos theta, r sin theta)
Vector2f UnityRandom::insideUnitCircle() {
    const float t = Range(0.0f, kTwoPi);
    const float r = std::sqrt(Range(0.0f, 1.0f));
    return {r * dotnet::NativeCosf(t), r * dotnet::NativeSinf(t)};
}

// Normalize(Range(-1,1) x4) as (x, y, z, w), divide by the norm, then negate all components if w < 0
Quaternionf UnityRandom::rotation() {
    const float x = Range(-1.0f, 1.0f);
    const float y = Range(-1.0f, 1.0f);
    const float z = Range(-1.0f, 1.0f);
    const float w = Range(-1.0f, 1.0f);
    const float n = std::sqrt(((x * x + y * y) + z * z) + w * w);
    Quaternionf q(x / n, y / n, z / n, w / n);
    if (q.w < 0.0f) q = Quaternionf(-q.x, -q.y, -q.z, -q.w);
    return q;
}

// u1 = Range(0,1); u2, u3 = Range(0, 2pi); a = sqrtf(1-u1); b = sqrtf(u1); (a sin u2, a cos u2, b sin u3, b cos u3), w >= 0
Quaternionf UnityRandom::rotationUniform() {
    const float u1 = Range(0.0f, 1.0f);
    const float u2 = Range(0.0f, kTwoPi);
    const float u3 = Range(0.0f, kTwoPi);
    const float a = std::sqrt(1.0f - u1);
    const float b = std::sqrt(u1);
    Quaternionf q(a * dotnet::NativeSinf(u2), a * dotnet::NativeCosf(u2), b * dotnet::NativeSinf(u3), b * dotnet::NativeCosf(u3));
    if (q.w < 0.0f) q = Quaternionf(-q.x, -q.y, -q.z, -q.w);
    return q;
}

// UnityEngine/Random.cs ColorHSV (managed)
Color UnityRandom::ColorHSV(float hueMin, float hueMax, float saturationMin, float saturationMax, float valueMin,
                            float valueMax, float alphaMin, float alphaMax) {
    const float h = Mathf::Lerp(hueMin, hueMax, value());
    const float s = Mathf::Lerp(saturationMin, saturationMax, value());
    const float v = Mathf::Lerp(valueMin, valueMax, value());
    Color result = HSVToRGB(h, s, v, true);
    result.a = Mathf::Lerp(alphaMin, alphaMax, value());
    return result;
}

// UnityEngine/Color.cs:259 HSVToRGB (managed, Mono float rule)
Color HSVToRGB(float H, float S, float V, bool hdr) {
    Color result = Color::white();
    if (S == 0.0f) {
        result.r = V;
        result.g = V;
        result.b = V;
    } else if (V == 0.0f) {
        result.r = 0.0f;
        result.g = 0.0f;
        result.b = 0.0f;
    } else {
        result.r = 0.0f;
        result.g = 0.0f;
        result.b = 0.0f;
        const float num = f32store(fx(H) * 6.0);
        const int num2 = dotnet::ConvToInt32(fx(Mathf::Floor(num)));
        const float num3 = f32store(fx(num) - fx(static_cast<float>(num2)));
        const float num4 = f32store(fx(V) * (1.0 - fx(S)));
        const float num5 = f32store(fx(V) * (1.0 - fx(S) * fx(num3)));
        const float num6 = f32store(fx(V) * (1.0 - fx(S) * (1.0 - fx(num3))));
        switch (num2) {
            case 0: result.r = V; result.g = num6; result.b = num4; break;
            case 1: result.r = num5; result.g = V; result.b = num4; break;
            case 2: result.r = num4; result.g = V; result.b = num6; break;
            case 3: result.r = num4; result.g = num5; result.b = V; break;
            case 4: result.r = num6; result.g = num4; result.b = V; break;
            case 5: result.r = V; result.g = num4; result.b = num5; break;
            case 6: result.r = V; result.g = num6; result.b = num4; break;
            case -1: result.r = V; result.g = num4; result.b = num5; break;
            default: break;
        }
        if (!hdr) {
            result.r = Mathf::Clamp(result.r, 0.0f, 1.0f);
            result.g = Mathf::Clamp(result.g, 0.0f, 1.0f);
            result.b = Mathf::Clamp(result.b, 0.0f, 1.0f);
        }
    }
    return result;
}

}  // namespace ksp
