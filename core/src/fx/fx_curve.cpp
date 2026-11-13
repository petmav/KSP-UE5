// WP-UE-16a: FXCurve.cs (see ksp/fx/fx_curve.hpp).
#include "ksp/fx/fx_curve.hpp"

#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/parts/cs_parse.hpp"

namespace ksp {

namespace {
// FXCurve.cs:105 `delimiters` = { ' ', ',', ';', '\t' }; Split(delimiters, StringSplitOptions.RemoveEmptyEntries)
std::vector<std::string> SplitTokens(const std::string& s) { return dotnet::Split(s, " ,;\t", true); }
}  // namespace

// FXCurve.cs:40 (valueName is ignored by the C# constructor)
FXCurve::FXCurve(const std::string& /*valueName*/, float value) {
    fCurve.reset();
    lastValue = value;
    singleValue = value;
    evalSingle = true;
}

// FXCurve.cs:48
void FXCurve::AddKeyframe(float time, float value) { keyFrames.emplace_back(time, value); }

// FXCurve.cs:54
void FXCurve::CompileCurve() const {
    AnimationCurve c;
    c.SetPostWrapMode(WrapMode::ClampForever);
    c.SetPreWrapMode(WrapMode::ClampForever);
    for (const FXKeyFrame& k : keyFrames) c.AddKey(k.keyframe());
    fCurve = std::move(c);
    fCurveCompiled = true;
}

// FXCurve.cs:67
float FXCurve::Value() const {
    if (evalSingle) return singleValue;
    if (!fCurveCompiled) CompileCurve();
    return fCurve->Evaluate(0.0f);
}

// FXCurve.cs:80
float FXCurve::Value(float time) const {
    if (evalSingle) return singleValue;
    if (!fCurveCompiled) CompileCurve();
    return fCurve->Evaluate(time);
}

// FXCurve.cs:93
void FXCurve::Load(const std::string& name, const ConfigNode& node) {
    valueName = name;
    const std::vector<std::string> values = node.GetValues(name);
    if (values.empty()) {
        fCurve.reset();
        evalSingle = true;
        return;
    }
    if (values.size() == 1) {
        const std::vector<std::string> array = SplitTokens(values[0]);
        if (array.empty()) throw std::out_of_range("Index was outside the bounds of the array.");  // array[0] of an empty split
        singleValue = cs::FloatParse(array[0]);
        lastValue = singleValue;
        fCurve.reset();
        evalSingle = true;
        return;
    }
    keyFrames.clear();
    fCurve.reset();
    fCurveCompiled = false;  // (C# leaves fCurveCompiled; CompileCurve below runs unconditionally, so the flag is true afterwards either way)
    for (const std::string& line : values) {
        const std::vector<std::string> array = SplitTokens(line);
        if (array.size() < 2) {
            log::Error("FloatCurve: Invalid line. Requires two values, 'time' and 'value'");
        } else {
            const float t = cs::FloatParse(array[0]);
            const float v = cs::FloatParse(array[1]);
            AddKeyframe(t, v);
        }
    }
    evalSingle = false;
    CompileCurve();
}

// FXCurve.cs:134 (float.ToString() = "G")
void FXCurve::Save(ConfigNode& node) const {
    if (evalSingle) {
        node.AddValue(valueName, dotnet::FormatSingleG(singleValue));
        return;
    }
    for (const FXKeyFrame& k : keyFrames) node.AddValue(valueName, dotnet::FormatSingleG(k.time) + " " + dotnet::FormatSingleG(k.value));
}

}  // namespace ksp
