// A small evaluator for the C# constant expressions that appear in the module metadata the decompile gives us:
// KSPField initialisers ("1f / 17f", "new Vector2(0f, 177f)", "Vector3.up", "Localizer.Format(\"#autoLOC_6001415\")",
// "TransformDir.Z", "double.NaN", ...) and attribute argument lists ("guiActive = true, guiName = \"#autoLOC_6001382\",
// affectSymCounterparts = UI_Scene.Editor", KSPAction's positional "\"#autoLOC_6001380\", KSPActionGroup.Stage").
//
// The C# compiler folded constant expressions at compile time (one IEEE operation per node, in the operand type), so
// float operands are combined with Mono's float rule (evaluate wide, round at the store: f32store(fx(a) op fx(b))).
// The text is ILSpy's rendering of the IL constants (shortest round-trip literals), so literals are parsed with the
// correctly rounded std::from_chars (the C# compiler's own conversion) rather than the runtime's float.Parse.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/parts/part_module_data.hpp"

namespace ksp::cs {

struct CsValue {
    enum class Kind { Null, Bool, Int, Float, Double, String, Vector, Member, New };
    Kind kind = Kind::Null;
    bool b = false;
    int64_t i = 0;          // Int (int/uint/long literals; unsigned 32-bit values fit)
    float f = 0.0f;         // Float
    double d = 0.0;         // Double
    std::string s;          // String; Member: the member name ("Stage" of "KSPActionGroup.Stage"); New: unused
    std::string typeName;   // Member: the type path ("KSPActionGroup", "ModuleDeployablePart.DeployState"); New: type
    std::vector<float> vec;  // Vector: components of new Vector2/3/4 / Quaternion, Vector3.up ...
    std::string vecType;     // "Vector2" | "Vector3" | "Vector4" | "Quaternion"
    bool operator==(const CsValue& o) const;
};

struct AttributeArg {
    std::string name;  // empty for positional arguments
    CsValue value;
};

// Evaluate one constant expression. Returns false when the text is outside the supported grammar (the caller keeps the
// C# default). `Localizer.Format(...)` calls go through ksp::Localizer.
bool EvaluateExpression(std::string_view text, CsValue& out);
// "a = 1, b = \"x\", UI_Scene.All" -> arguments (split on top-level commas).
bool ParseAttributeArguments(std::string_view text, std::vector<AttributeArg>& out);

// KSPField initialiser -> typed value for the field (the C# field type decides the conversion). `evaluated` is false
// when the expression is not understood (the value is then the C# default).
ModuleValue FieldInitialValue(const ModuleFieldInfo& info, bool* evaluated = nullptr);
// default(T) for the field type as the Unity-created component holds it: numbers 0, bool false, string null,
// vectors/quaternion zero (Quaternion is (0,0,0,0), not identity), enums 0, string / FloatCurve / DoubleCurve null (the
// capture shows module OnAwake bodies guarded by `if (curve == null)` creating their defaults, so AddComponent leaves them
// null).
ModuleValue DefaultFieldValue(const ModuleFieldInfo& info);
// Convert a constant to the field type (C# implicit conversions of constants); false when incompatible.
bool ConvertConstant(const CsValue& v, const ModuleFieldInfo& info, ModuleValue& out);

// Typed accessors used by attribute readers.
bool AsBool(const CsValue& v, bool def);
float AsFloat(const CsValue& v, float def);
std::string AsString(const CsValue& v, const std::string& def);

}  // namespace ksp::cs
