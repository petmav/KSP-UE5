// UnityEngine.Random (Unity 2019.4.18f1): native xorshift128 generator and its float/int/vector mappings.
//
// Spec: CAPTURE-SPEC C2, harness/captures/README.md ("C2: UnityEngine.Random") and reports/C2_*.md, fitted bit-exactly
// against in-game captures (harness/captures/out/unity_random.json). All native arithmetic is float32 per operation
// with uint32 wrap-around; sinf/cosf/sqrtf/powf are the C runtime's float functions. ColorHSV is managed Unity code
// (UnityEngine/Random.cs + Color.HSVToRGB) and therefore follows Mono's float rule.
//
// Unity keeps one process-wide generator (Random.state); Global() is that instance. Not thread-safe (neither is Unity's).
#pragma once

#include <cstdint>

#include "ksp/unity/color.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class UnityRandom {
public:
    struct State {
        uint32_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
    };

    UnityRandom() { InitState(0); }
    explicit UnityRandom(int32_t seed) { InitState(seed); }

    static UnityRandom& Global();

    // Random.InitState(seed): s0 = seed; s(n+1) = s(n) * 1812433253 + 1
    void InitState(int32_t seed);
    State state() const { return s_; }
    void SetState(const State& s) { s_ = s; }

    // xorshift128 step; returns the new s3
    uint32_t NextUInt();

    // Random.value = (float)(r & 0x7FFFFF) / 8388607f, in [0, 1] inclusive
    float value();
    // Random.Range(float min, float max) = v*min + (1-v)*max (reversed lerp; one draw)
    float Range(float min, float max);
    // Random.Range(int min, int max): [min, max) for max > min, (max, min] for max < min, min (no draw) for max == min
    int32_t Range(int32_t min, int32_t max);

    Vector3f onUnitSphere();      // 2 draws
    Vector3f insideUnitSphere();  // 3 draws
    Vector2f insideUnitCircle();  // 2 draws
    Quaternionf rotation();       // 4 draws
    Quaternionf rotationUniform();  // 3 draws
    // Random.ColorHSV(hueMin, hueMax, satMin, satMax, valMin, valMax, alphaMin, alphaMax) (managed; 4 draws)
    Color ColorHSV(float hueMin = 0.0f, float hueMax = 1.0f, float saturationMin = 0.0f, float saturationMax = 1.0f,
                   float valueMin = 0.0f, float valueMax = 1.0f, float alphaMin = 1.0f, float alphaMax = 1.0f);

private:
    State s_;
};

// UnityEngine.Color.HSVToRGB(H, S, V, hdr) (managed, Mono float rule)
Color HSVToRGB(float H, float S, float V, bool hdr);

}  // namespace ksp
