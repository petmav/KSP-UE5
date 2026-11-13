// WP-UE-16a: FXCurve.cs (the cfg curve every EFFECTS class evaluates against the effect power).
//
// Forms (FXCurve.Load, FXCurve.cs:92): no line -> evalSingle with the previous singleValue (-1 by default); ONE line ->
// singleValue = float.Parse(first token) (a one-line "time value" curve therefore yields the TIME, a C# quirk kept); two or more
// lines -> keyframes (lines with fewer than two tokens are skipped with an error) compiled to a Unity AnimationCurve with
// ClampForever on both ends. Keyframe(time, value) has zero tangents, so a multi-line curve is a flat-tangent cubic Hermite
// (AnimationCurve::AddKey(Keyframe) keeps tangents as given; a key whose time already exists is rejected).
//
// Quirk kept: the C# constructor FXCurve(string valueName, float value) ignores valueName (it stays "" until Load).
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/unity/animation_curve.hpp"

namespace ksp {

// FXCurve.FXKeyFrame (FXCurve.cs:9)
struct FXKeyFrame {
    float time = 0.0f;
    float value = 0.0f;
    FXKeyFrame() = default;
    FXKeyFrame(float t, float v) : time(t), value(v) {}
    Keyframe keyframe() const { return Keyframe(time, value); }  // :19
};

// FXCurve.cs:6 ([Serializable]: Instantiate copies every public field, including the compiled curve)
class FXCurve {
public:
    float singleValue = -1.0f;      // :20
    std::string valueName;          // :22
    float lastValue = 1.0f;         // :24
    bool evalSingle = true;         // :26
    std::vector<FXKeyFrame> keyFrames;  // :28
    mutable std::optional<AnimationCurve> fCurve;  // :30 (null = nullopt)
    mutable bool fCurveCompiled = false;           // :32

    FXCurve() = default;
    FXCurve(const std::string& valueName, float value);  // :36
    void AddKeyframe(float time, float value);            // :44
    float Value() const;                                  // :61 (Evaluate(0))
    float Value(float time) const;                        // :75
    void Load(const std::string& valueName, const ConfigNode& node);  // :89
    void Save(ConfigNode& node) const;                    // :132
    float implicitFloat() const { return lastValue; }     // :146 `implicit operator float`

private:
    void CompileCurve() const;  // :47
};

}  // namespace ksp
