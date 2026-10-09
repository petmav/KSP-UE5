// Port of Assembly-CSharp/DoubleCurve.cs, Vector3Curve.cs, Vector4Curve.cs
#include "ksp/curves/double_curve.hpp"

#include <algorithm>
#include <stdexcept>

#include "ksp/config/config_node.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace {
constexpr int kFindCurveMinMaxIterations = 100;  // DoubleCurve.cs: findCurveMinMaxInterations
}

// DoubleCurve.cs: DoubleCurve(DoubleKeyframe[]) / DoubleCurve(List<DoubleKeyframe>) (no sort, no tangent recompute)
DoubleCurve::DoubleCurve(const std::vector<DoubleKeyframe>& keyframes) : keys(keyframes) {
    minTime = keys.empty() ? 1.7976931348623157E+308 : keys.front().time;
    maxTime = keys.empty() ? -1.7976931348623157E+308 : keys.back().time;
}

// DoubleCurve.cs: GetInsertionIndex -- after the last key whose time is < time
int DoubleCurve::GetInsertionIndex(double time) const {
    int result = 0;
    int count = static_cast<int>(keys.size());
    while (count-- > 0) {
        if (!(keys[static_cast<size_t>(count)].time >= time)) {
            result = count + 1;
            break;
        }
    }
    return result;
}

// DoubleCurve.cs: Add(time, value)
void DoubleCurve::Add(double time, double value) {
    keys.insert(keys.begin() + GetInsertionIndex(time), DoubleKeyframe(time, value));
    minTime = std::min(minTime, time);
    maxTime = std::max(maxTime, time);
    RecomputeTangents();
}

// DoubleCurve.cs: Add(time, value, inTangent, outTangent)
void DoubleCurve::Add(double time, double value, double inTangent, double outTangent) {
    keys.insert(keys.begin() + GetInsertionIndex(time), DoubleKeyframe(time, value, inTangent, outTangent));
    minTime = std::min(minTime, time);
    maxTime = std::max(maxTime, time);
    RecomputeTangents();
}

// DoubleCurve.cs: Evaluate
double DoubleCurve::Evaluate(double t) const {
    if (keys.empty()) return 0.0;
    int num = static_cast<int>(keys.size()) - 1;
    int num2 = 0;
    if (t < minTime) return keys[static_cast<size_t>(num2)].value;
    if (t > maxTime) return keys[static_cast<size_t>(num)].value;
    while (num - num2 > 1) {
        const int num3 = (num + num2) / 2;
        const double time = keys[static_cast<size_t>(num3)].time;
        if (time == t) return keys[static_cast<size_t>(num3)].value;
        if (time < t) num2 = num3;
        else num = num3;
    }
    const DoubleKeyframe& k1 = keys[static_cast<size_t>(num2)];
    const DoubleKeyframe& k2 = keys[static_cast<size_t>(num)];
    const double num4 = k2.time - k1.time;
    const double num5 = k1.outTangent * num4;
    const double num6 = k2.inTangent * num4;
    t = (t - k1.time) / num4;
    const double num7 = t * t;
    const double num8 = num7 * t;
    const double num9 = 2.0 * num8 - 3.0 * num7 + 1.0;
    const double num10 = num8 - 2.0 * num7 + t;
    const double num11 = num8 - num7;
    const double num12 = -2.0 * num8 + 3.0 * num7;
    return num9 * k1.value + num10 * num5 + num11 * num6 + num12 * k2.value;
}

// DoubleCurve.cs: RecomputeTangents (KSP's bugs kept, see header)
void DoubleCurve::RecomputeTangents() {
    const int count = static_cast<int>(keys.size());
    if (count == 1) {
        keys[0].inTangent = 0.0;
        keys[0].outTangent = 0.0;
        return;
    }
    DoubleKeyframe value = keys[0];
    if (value.autoTangent) {
        value.inTangent = 0.0;
        value.outTangent = (keys[1].value - value.value) / (keys[1].time - value.time);
        keys[0] = value;
    }
    const int num = count - 1;
    value = keys[static_cast<size_t>(num)];
    if (value.autoTangent) {
        value.inTangent = (value.value - keys[static_cast<size_t>(num - 1)].value) /
                          (value.time - keys[static_cast<size_t>(num - 1)].value);  // sic: .value
        value.outTangent = 0.0;
        keys[static_cast<size_t>(num)] = value;
    }
    if (count <= 2) return;
    for (int i = 1; i < num; ++i) {
        value = keys[static_cast<size_t>(i)];
        if (value.autoTangent) {
            const double num2 = (value.value - keys[static_cast<size_t>(i - 1)].value) /
                                (value.time - keys[static_cast<size_t>(i - 1)].value);  // sic: .value
            const double num3 = (keys[static_cast<size_t>(i + 1)].value - value.value) /
                                (keys[static_cast<size_t>(i + 1)].time - value.time);
            value.inTangent = value.outTangent = (num2 + num3) * 0.5;
            // sic: KSP never writes `value` back to keys[i]
        }
    }
}

// DoubleCurve.cs: Load (double.Parse; a 1-value key logs an error, then throws IndexOutOfRange)
void DoubleCurve::Load(const ConfigNode& node) {
    keys.clear();
    minTime = 1.7976931348623157E+308;
    maxTime = -1.7976931348623157E+308;
    for (const std::string& line : node.GetValues("key")) {
        const std::vector<std::string> a = dotnet::Split(line, " ,;\t", true);
        if (a.size() < 2)
            log::Error("DoubleCurve: Invalid line. Requires two values, 'time' and 'value', or four values, 'time', 'value', "
                       "'inTangent', and 'outTangent'.");
        if (a.size() == 4) {
            Add(dotnet::ParseDouble(a[0]), dotnet::ParseDouble(a[1]), dotnet::ParseDouble(a[2]), dotnet::ParseDouble(a[3]));
        } else {
            if (a.empty()) throw std::out_of_range("DoubleCurve.Load: Index was outside the bounds of the array.");
            const double t = dotnet::ParseDouble(a[0]);
            if (a.size() < 2) throw std::out_of_range("DoubleCurve.Load: Index was outside the bounds of the array.");
            Add(t, dotnet::ParseDouble(a[1]));
        }
    }
}

// DoubleCurve.cs: Save (double + " " uses double.ToString())
void DoubleCurve::Save(ConfigNode& node) const {
    using dotnet::FormatDoubleG;
    for (const DoubleKeyframe& k : keys) {
        if (k.autoTangent) node.AddValue("key", FormatDoubleG(k.time) + " " + FormatDoubleG(k.value));
        else
            node.AddValue("key", FormatDoubleG(k.time) + " " + FormatDoubleG(k.value) + " " + FormatDoubleG(k.inTangent) +
                                     " " + FormatDoubleG(k.outTangent));
    }
}

// DoubleCurve.cs: FindMinMaxValue
void DoubleCurve::FindMinMaxValue(double& min, double& max) const {
    double tMin, tMax;
    FindMinMaxValue(min, max, tMin, tMax);
}

void DoubleCurve::FindMinMaxValue(double& min, double& max, double& tMin, double& tMax) const {
    tMin = 0.0;
    tMax = 0.0;
    min = 1.7976931348623157E+308;
    max = -1.7976931348623157E+308;
    double num = 1.7976931348623157E+308;
    double num2 = -1.7976931348623157E+308;
    for (const DoubleKeyframe& k : keys) {
        if (k.time < num) num = k.time;
        if (k.time > num2) num2 = k.time;
    }
    const double num3 = (num2 - num) / static_cast<double>(kFindCurveMinMaxIterations);
    for (int j = 0; j < kFindCurveMinMaxIterations; ++j) {
        const double num4 = num + static_cast<double>(j) * num3;
        const double num5 = Evaluate(num4);
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

// Vector3Curve.cs / Vector4Curve.cs
void Vector3Curve::Add(float time, const Vector3f& value) {
    x_.AddKey(time, value.x);
    y_.AddKey(time, value.y);
    z_.AddKey(time, value.z);
    minTime_ = Mathf::Min(minTime_, time);
    maxTime_ = Mathf::Max(maxTime_, time);
}

void Vector3Curve::Add(float time, const Color& value) {
    x_.AddKey(time, value.r);
    y_.AddKey(time, value.g);
    z_.AddKey(time, value.b);
    minTime_ = Mathf::Min(minTime_, time);
    maxTime_ = Mathf::Max(maxTime_, time);
}

void Vector4Curve::Add(float time, const Vector4f& value) {
    x_.AddKey(time, value.x);
    y_.AddKey(time, value.y);
    z_.AddKey(time, value.z);
    w_.AddKey(time, value.w);
    minTime_ = Mathf::Min(minTime_, time);
    maxTime_ = Mathf::Max(maxTime_, time);
}

void Vector4Curve::Add(float time, const Color& value) {
    x_.AddKey(time, value.r);
    y_.AddKey(time, value.g);
    z_.AddKey(time, value.b);
    w_.AddKey(time, value.a);
    minTime_ = Mathf::Min(minTime_, time);
    maxTime_ = Mathf::Max(maxTime_, time);
}

}  // namespace ksp
