// Port of Assembly-CSharp/FloatCurve.cs
#include "ksp/curves/float_curve.hpp"

#include <stdexcept>

#include "ksp/config/config_node.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace {
constexpr int kFindCurveMinMaxIterations = 100;  // FloatCurve.cs:18
}

// FloatCurve.cs:36
FloatCurve::FloatCurve() {
    fCurve_ = AnimationCurve();
    minTime_ = FLT_MAX;
    maxTime_ = -FLT_MAX;
    fCurve_.SetPostWrapMode(WrapMode::ClampForever);
    fCurve_.SetPreWrapMode(WrapMode::ClampForever);
}

// FloatCurve.cs:45
FloatCurve::FloatCurve(const std::vector<Keyframe>& keyframes) : FloatCurve() {
    for (const Keyframe& k : keyframes) Add(k.time, k.value, k.inTangent, k.outTangent);
}

// FloatCurve.cs:54
void FloatCurve::Add(float time, float value) {
    fCurve_.AddKey(time, value);
    minTime_ = Mathf::Min(minTime_, time);
    maxTime_ = Mathf::Max(maxTime_, time);
}

// FloatCurve.cs:61 (Keyframe built with default fields, so weights are 0 and weightedMode None)
void FloatCurve::Add(float time, float value, float inTangent, float outTangent) {
    Keyframe key;
    key.inTangent = inTangent;
    key.outTangent = outTangent;
    key.time = time;
    key.value = value;
    fCurve_.AddKey(key);
    minTime_ = Mathf::Min(minTime_, time);
    maxTime_ = Mathf::Max(maxTime_, time);
}

// FloatCurve.cs:78
void FloatCurve::Load(const ConfigNode& node) {
    fCurve_ = AnimationCurve();
    minTime_ = FLT_MAX;
    maxTime_ = -FLT_MAX;
    const std::vector<std::string> values = node.GetValues("key");
    for (const std::string& line : values) {
        const std::vector<std::string> array = dotnet::Split(line, " ,;\t", /*removeEmptyEntries=*/true);
        if (array.size() < 2) log::Error("FloatCurve: Invalid line. Requires two values, 'time' and 'value'");
        if (array.size() == 4) {
            Add(dotnet::ParseSingle(array[0]), dotnet::ParseSingle(array[1]), dotnet::ParseSingle(array[2]),
                dotnet::ParseSingle(array[3]));
        } else {
            // C# evaluates array[0] then array[1]; a short array throws IndexOutOfRangeException.
            if (array.empty()) throw std::out_of_range("FloatCurve.Load: Index was outside the bounds of the array.");
            const float t = dotnet::ParseSingle(array[0]);
            if (array.size() < 2) throw std::out_of_range("FloatCurve.Load: Index was outside the bounds of the array.");
            Add(t, dotnet::ParseSingle(array[1]));
        }
    }
}

// FloatCurve.cs:103 (float + " " concatenation uses float.ToString(): "G", 7 digits)
void FloatCurve::Save(ConfigNode& node) const {
    for (const Keyframe& k : fCurve_.keys()) {
        node.AddValue("key", dotnet::FormatSingleG(k.time) + " " + dotnet::FormatSingleG(k.value) + " " +
                                 dotnet::FormatSingleG(k.inTangent) + " " + dotnet::FormatSingleG(k.outTangent));
    }
}

// FloatCurve.cs:111
void FloatCurve::FindMinMaxValue(float& min, float& max) const {
    min = FLT_MAX;
    max = -FLT_MAX;
    float num = FLT_MAX;
    float num2 = -FLT_MAX;
    for (const Keyframe& k : fCurve_.keys()) {
        if (k.time < num) num = k.time;
        if (k.time > num2) num2 = k.time;
    }
    const float num3 = f32store((fx(num2) - fx(num)) / static_cast<double>(kFindCurveMinMaxIterations));
    for (int j = 0; j < kFindCurveMinMaxIterations; ++j) {
        const float num4 = fCurve_.Evaluate(f32store(fx(num) + static_cast<double>(j) * fx(num3)));
        if (num4 < min) min = num4;
        if (num4 > max) max = num4;
    }
}

// FloatCurve.cs:149
void FloatCurve::FindMinMaxValue(float& min, float& max, float& tMin, float& tMax) const {
    min = FLT_MAX;
    max = -FLT_MAX;
    tMin = 0.0f;
    tMax = 0.0f;
    float num = FLT_MAX;
    float num2 = -FLT_MAX;
    for (const Keyframe& k : fCurve_.keys()) {
        if (k.time < num) num = k.time;
        if (k.time > num2) num2 = k.time;
    }
    const float num3 = f32store((fx(num2) - fx(num)) / static_cast<double>(kFindCurveMinMaxIterations));
    for (int j = 0; j < kFindCurveMinMaxIterations; ++j) {
        const float num4 = f32store(fx(num) + static_cast<double>(j) * fx(num3));
        const float num5 = fCurve_.Evaluate(num4);
        if (num5 < min) {
            min = num5;
            tMin = num4;
        }
        if (num5 > max) {
            max = num5;
            tMax = num4;
        }
    }
}

}  // namespace ksp
