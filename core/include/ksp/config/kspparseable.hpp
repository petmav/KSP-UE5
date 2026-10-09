// KSPParseable (Assembly-CSharp/KSPParseable.cs), the KSPUtil vector parse/write helpers it relies on (KSPUtil.cs
// ParseVector2/3/4, ParseQuaternion, WriteVector, WriteQuaternion -- the rest of KSPUtil is WP-03), and the modding
// type parsers (IntTypeParser.cs, FloatTypeParser.cs, DoubleTypeParser.cs, StringTypeParser.cs).
//
// Quirks kept: the type parsers box a plain int 0 on failure (so a failed double parse yields an int, as in KSP);
// IntTypeParser saturates values that only parse as long; KSPUtil.Parse* split on ',' without removing empty entries
// and only check a minimum count (extra components are ignored); KSPParseable(string) keeps the value even when the
// type name does not parse (type UNKNOWN).
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>

#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

namespace KSPUtil {
// KSPUtil.cs:982.. (float.Parse on each component; fewer components: Debug.Log warning, zero / identity)
Vector2f ParseVector2(std::string_view s, char separator = ',');
Vector3f ParseVector3(std::string_view s, char separator = ',');
Vector4f ParseVector4(std::string_view s, char separator = ',');
Quaternionf ParseQuaternion(std::string_view s, char separator = ',');
// KSPUtil.cs:894.. ("G9" components)
std::string WriteVector(const Vector2f& v, std::string_view separator = ",");
std::string WriteVector(const Vector3f& v, std::string_view separator = ",");
std::string WriteVector(const Vector4f& v, std::string_view separator = ",");
std::string WriteQuaternion(const Quaternionf& q, std::string_view separator = ",");
}  // namespace KSPUtil

struct KSPParseable {
    enum class Type : int32_t { STRING, BOOL, UINT, INT, FLOAT, DOUBLE, VECTOR2, VECTOR3, VECTOR4, QUATERNION, UNKNOWN };

    std::string value;
    Type type = Type::UNKNOWN;

    KSPParseable() = default;
    // KSPParseable(string Value): "value, TYPE" (last comma-separated field is the type name)
    explicit KSPParseable(std::string_view text);
    // KSPParseable(object Value, Type Type)
    static KSPParseable FromString(std::string_view v) { return Make(std::string(v), Type::STRING); }
    static KSPParseable FromBool(bool v);
    static KSPParseable FromUInt(uint32_t v);
    static KSPParseable FromInt(int32_t v);
    static KSPParseable FromFloat(float v);
    static KSPParseable FromDouble(double v);
    static KSPParseable FromVector2(const Vector2f& v) { return Make(KSPUtil::WriteVector(v), Type::VECTOR2); }
    static KSPParseable FromVector3(const Vector3f& v) { return Make(KSPUtil::WriteVector(v), Type::VECTOR3); }
    static KSPParseable FromVector4(const Vector4f& v) { return Make(KSPUtil::WriteVector(v), Type::VECTOR4); }
    static KSPParseable FromQuaternion(const Quaternionf& q) { return Make(KSPUtil::WriteQuaternion(q), Type::QUATERNION); }

    // typed accessors: default value when the type differs; Parse exceptions propagate like in KSP
    int32_t value_int() const;
    uint32_t value_uint() const;
    float value_float() const;
    double value_double() const;
    bool value_bool() const;
    Vector2f value_v2() const;
    Vector3f value_v3() const;
    Vector4f value_v4() const;
    Quaternionf value_quat() const;

    // value + ", " + type (enum name)
    std::string Save() const;

    static std::string TypeName(Type t);

private:
    static KSPParseable Make(std::string v, Type t) {
        KSPParseable p;
        p.value = std::move(v);
        p.type = t;
        return p;
    }
};

// ITypeParser implementations (boxed results modelled as a variant)
using ParsedObject = std::variant<std::string, int32_t, float, double>;
namespace TypeParsers {
ParsedObject ParseInt(std::string_view s);     // int, else long saturated to int, else 0
ParsedObject ParseFloat(std::string_view s);   // float, else int 0
ParsedObject ParseDouble(std::string_view s);  // double, else int 0
ParsedObject ParseString(std::string_view s);  // the string itself
std::string Convert(const ParsedObject& o);    // o.ToString()
}  // namespace TypeParsers

}  // namespace ksp
