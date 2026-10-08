// FloatCurve (Assembly-CSharp/FloatCurve.cs): KSP's cfg-loadable wrapper around UnityEngine.AnimationCurve.
//
// Load(node): every `key` value is split on ' ', ',', ';', '\t'; exactly 4 numbers -> Add(t, v, in, out) (explicit
// tangents, AddKey without smoothing); any other count -> Add(t, v) (Unity smooth tangents on the key and both
// neighbours). Fewer than 2 numbers: KSP logs an error and then throws IndexOutOfRangeException (std::out_of_range
// here); numbers are parsed with float.Parse (FormatException / OverflowException).
#pragma once

#include <cfloat>

#include "ksp/unity/animation_curve.hpp"

namespace ksp {

class ConfigNode;

class FloatCurve {
public:
    // FloatCurve.cs:36
    FloatCurve();
    // FloatCurve.cs:45 FloatCurve(Keyframe[]): Add(time, value, inTangent, outTangent) per key
    explicit FloatCurve(const std::vector<Keyframe>& keyframes);

    AnimationCurve& Curve() { return fCurve_; }
    const AnimationCurve& Curve() const { return fCurve_; }
    void SetCurve(const AnimationCurve& c) { fCurve_ = c; }

    float minTime() const { return minTime_; }
    float maxTime() const { return maxTime_; }
    // Serialized FloatCurves (Unity asset data) carry their own _minTime/_maxTime.
    void SetSerializedRange(float minTime, float maxTime) {
        minTime_ = minTime;
        maxTime_ = maxTime;
    }

    void Add(float time, float value);
    void Add(float time, float value, float inTangent, float outTangent);
    float Evaluate(float time) const { return fCurve_.Evaluate(time); }

    void Load(const ConfigNode& node);
    void Save(ConfigNode& node) const;

    void FindMinMaxValue(float& min, float& max) const;
    void FindMinMaxValue(float& min, float& max, float& tMin, float& tMax) const;

private:
    AnimationCurve fCurve_;
    float minTime_ = FLT_MAX;
    float maxTime_ = -FLT_MAX;
};

}  // namespace ksp
