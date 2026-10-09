// DoubleCurve / DoubleKeyframe (Assembly-CSharp/DoubleCurve.cs, DoubleKeyframe.cs): KSP's double-precision Hermite
// curve, and Vector3Curve / Vector4Curve (Vector3Curve.cs, Vector4Curve.cs): per-component Unity AnimationCurves.
//
// DoubleCurve quirks kept (all in RecomputeTangents, DoubleCurve.cs): the last key's and the middle keys' left slope
// divide by (time - previous.VALUE) instead of (time - previous.time); the middle keys' recomputed tangents are written
// to a struct copy and never stored, so only the first and last auto-tangent keys change; with a single key both of its
// tangents are zeroed even if they were given explicitly. Evaluate is a cubic Hermite
// on the bracketing keys (binary search); outside [minTime, maxTime] it clamps to the end values.
#pragma once

#include <vector>

#include "ksp/unity/animation_curve.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class ConfigNode;

struct DoubleKeyframe {
    double inTangent = 0.0;
    double outTangent = 0.0;
    double time = 0.0;
    double value = 0.0;
    bool autoTangent = false;

    DoubleKeyframe() = default;
    // DoubleKeyframe.cs: (time, value) -> tangents 0, autoTangent true
    DoubleKeyframe(double t, double v) : time(t), value(v), autoTangent(true) {}
    DoubleKeyframe(double t, double v, double in, double out)
        : inTangent(in), outTangent(out), time(t), value(v), autoTangent(false) {}
};

class DoubleCurve {
public:
    std::vector<DoubleKeyframe> keys;
    double minTime = 1.7976931348623157E+308;
    double maxTime = -1.7976931348623157E+308;

    DoubleCurve() = default;
    explicit DoubleCurve(const std::vector<DoubleKeyframe>& keyframes);

    void Add(double time, double value);
    void Add(double time, double value, double inTangent, double outTangent);
    double Evaluate(double t) const;
    void Load(const ConfigNode& node);
    void Save(ConfigNode& node) const;
    void FindMinMaxValue(double& min, double& max) const;
    void FindMinMaxValue(double& min, double& max, double& tMin, double& tMax) const;

private:
    int GetInsertionIndex(double time) const;
    void RecomputeTangents();
};

// Vector3Curve.cs: three AnimationCurves fed with AddKey(t, v) (Unity smooth tangents)
class Vector3Curve {
public:
    float minTime() const { return minTime_; }
    float maxTime() const { return maxTime_; }
    void Add(float time, const Vector3f& value);
    void Add(float time, const Color& value);
    Vector3f EvaluateVector(float time) const { return {x_.Evaluate(time), y_.Evaluate(time), z_.Evaluate(time)}; }
    Color EvaluateColor(float time) const { return Color(x_.Evaluate(time), y_.Evaluate(time), z_.Evaluate(time)); }

private:
    AnimationCurve x_, y_, z_;
    float minTime_ = 3.40282347E+38f;
    float maxTime_ = -3.40282347E+38f;
};

// Vector4Curve.cs
class Vector4Curve {
public:
    float minTime() const { return minTime_; }
    float maxTime() const { return maxTime_; }
    void Add(float time, const Vector4f& value);
    void Add(float time, const Color& value);
    Vector4f EvaluateVector(float time) const {
        return {x_.Evaluate(time), y_.Evaluate(time), z_.Evaluate(time), w_.Evaluate(time)};
    }
    Color EvaluateColor(float time) const {
        return Color(x_.Evaluate(time), y_.Evaluate(time), z_.Evaluate(time), w_.Evaluate(time));
    }

private:
    AnimationCurve x_, y_, z_, w_;
    float minTime_ = 3.40282347E+38f;
    float maxTime_ = -3.40282347E+38f;
};

}  // namespace ksp
