// UnityEngine.Keyframe and UnityEngine.AnimationCurve (Unity 2019.4.18f1 native AnimationCurveTpl<float>).
//
// Port of the harness's managed transcription harness/src/KspUnityShim/AnimationCurve.cs, which was itself transcribed
// instruction-by-instruction from UnityPlayer.dll (RVAs in that file). Every float operation is float32, in the native
// operation order, so results are bit-identical to the golden data (curves_eval: 0 ULP).
//
// Evaluation: per segment Unity caches the cubic ((a*u+b)*u+c)*u+d in u = t - key.time (segment length clamped to
// >= 1e-4; +Infinity out/in tangent = constant segment; -Infinity is not special); outside the key range the curve
// clamps (t >= last key returns the last value; Evaluate(+Inf) is NaN through 0*Inf).
// Thread safety: Evaluate() is const and stateless for curves whose key times are strictly increasing (everything
// AddKey produces); the result is identical to Unity's cached evaluation. Curves built from raw keyframes containing
// duplicate times fall back to a faithful emulation of Unity's per-curve cache (not thread-safe, like Unity).
//
// Not supported (throw std::logic_error, as the harness shim does): weighted (Bezier) segments, Loop/PingPong wrap
// modes evaluated outside the key range, MoveKey.
#pragma once

#include <cstdint>
#include <vector>

namespace ksp {

enum class WeightedMode : int32_t { None = 0, In = 1, Out = 2, Both = 3 };

// UnityEngine.WrapMode (managed enum values)
enum class WrapMode : int32_t { Once = 1, Loop = 2, PingPong = 4, Default = 0, ClampForever = 8, Clamp = 1 };

struct Keyframe {
    float time = 0.0f;
    float value = 0.0f;
    float inTangent = 0.0f;
    float outTangent = 0.0f;
    int32_t tangentMode = 0;  // legacy m_TangentMode (unused by the evaluator)
    WeightedMode weightedMode = WeightedMode::None;
    float inWeight = 0.0f;
    float outWeight = 0.0f;

    constexpr Keyframe() = default;
    // Keyframe(float time, float value): tangents 0, weights 0 (managed ctor)
    constexpr Keyframe(float t, float v) : time(t), value(v) {}
    // Keyframe(float time, float value, float inTangent, float outTangent)
    constexpr Keyframe(float t, float v, float in, float out) : time(t), value(v), inTangent(in), outTangent(out) {}
    constexpr Keyframe(float t, float v, float in, float out, float inW, float outW)
        : time(t), value(v), inTangent(in), outTangent(out), weightedMode(WeightedMode::None), inWeight(inW), outWeight(outW) {}
};

class AnimationCurve {
public:
    AnimationCurve() = default;
    // AnimationCurve(params Keyframe[]) / keys setter: drop keys with non-finite time or value, stable sort by time.
    explicit AnimationCurve(const std::vector<Keyframe>& keys) { SetKeys(keys); }

    // Unity deserialization (StreamedBinaryRead): keys verbatim (no sort/filter), raw native wrap values
    // (2 = clamp, 1 = repeat, 0 = ping-pong, 3 = default).
    static AnimationCurve CreateFromSerialized(const std::vector<Keyframe>& keys, int preInfinity, int postInfinity);

    const std::vector<Keyframe>& keys() const { return keys_; }
    void SetKeys(const std::vector<Keyframe>& keys);
    int length() const { return static_cast<int>(keys_.size()); }
    const Keyframe& operator[](int i) const { return keys_[static_cast<size_t>(i)]; }

    // AnimationCurveTpl<float>::AddKey(Keyframe): lower_bound by time; a key whose time already exists is rejected (-1).
    int AddKey(const Keyframe& key);
    // AddKeySmoothTangents: insert (t, v, 0, 0, weights 1/3) and recompute the tangents of the key and both neighbours.
    int AddKey(float time, float value);
    void RemoveKey(int index);
    void SmoothTangents(int index, float weight);

    WrapMode preWrapMode() const { return preWrapManaged_; }
    WrapMode postWrapMode() const { return postWrapManaged_; }
    void SetPreWrapMode(WrapMode m);
    void SetPostWrapMode(WrapMode m);
    int NativePreInfinity() const { return preInfinity_; }
    int NativePostInfinity() const { return postInfinity_; }

    float Evaluate(float time) const;

    static AnimationCurve Constant(float timeStart, float timeEnd, float value);
    static AnimationCurve Linear(float timeStart, float valueStart, float timeEnd, float valueEnd);
    static AnimationCurve EaseInOut(float timeStart, float valueStart, float timeEnd, float valueEnd);

private:
    struct Cache {
        int index = 0;
        float time = 0.0f;  // set to +Inf by Invalidate()
        float timeEnd = 0.0f;
        float a = 0.0f, b = 0.0f, c = 0.0f, d = 0.0f;
    };

    void InvalidateCache() const;
    void UpdateDuplicateFlag();
    void RecalculateSplineSlope(int key, float b);
    bool IsValid() const;
    void FindIndexForSampling(const Cache& cache, float t, int& lhs, int& rhs) const;
    void CalculateCacheData(Cache& cache, int lhsIndex, int rhsIndex, float timeOffset) const;
    float EvaluateWith(Cache& cache, float time) const;

    std::vector<Keyframe> keys_;
    int preInfinity_ = 2;
    int postInfinity_ = 2;
    WrapMode preWrapManaged_ = WrapMode::ClampForever;
    WrapMode postWrapManaged_ = WrapMode::ClampForever;
    bool hasDuplicateTimes_ = false;
    mutable Cache cache_ = InitialCache();

    static Cache InitialCache();
};

}  // namespace ksp
