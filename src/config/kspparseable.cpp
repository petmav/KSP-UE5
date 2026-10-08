// Port of Assembly-CSharp/KSPParseable.cs, the KSPUtil.cs vector helpers it uses, and the *TypeParser.cs classes.
#include "ksp/config/kspparseable.hpp"

#include <cctype>
#include <stdexcept>
#include <vector>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"

namespace ksp {

namespace KSPUtil {

namespace {
std::vector<std::string> SplitOn(std::string_view s, char sep) { return dotnet::Split(s, std::string_view(&sep, 1), false); }
}  // namespace

// KSPUtil.cs:982 / 993
Vector2f ParseVector2(std::string_view s, char separator) {
    const auto a = SplitOn(s, separator);
    if (a.size() < 2) {
        log::Info(std::string("WARNING: Vector2 entry is not formatted properly! proper format for Vector2s is x") + separator + "y");
        return Vector2f::zero();
    }
    return {dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1])};
}

// KSPUtil.cs:1014
Vector3f ParseVector3(std::string_view s, char separator) {
    const auto a = SplitOn(s, separator);
    if (a.size() < 3) {
        log::Info(std::string("WARNING: Vector3 entry is not formatted properly! proper format for Vector3s is x") + separator +
                  "y" + separator + "z");
        return Vector3f::zero();
    }
    return {dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1]), dotnet::ParseSingle(a[2])};
}

// KSPUtil.cs:1057 / 1073
Vector4f ParseVector4(std::string_view s, char separator) {
    const auto a = SplitOn(s, separator);
    if (a.size() < 4) {
        log::Info("WARNING: Vector4 entry is nor formatted properly! proper format for Vector4s is x,y,z,w");
        return Vector4f::zero();
    }
    return {dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1]), dotnet::ParseSingle(a[2]), dotnet::ParseSingle(a[3])};
}

// KSPUtil.cs:1084 / 1095
Quaternionf ParseQuaternion(std::string_view s, char separator) {
    const auto a = SplitOn(s, separator);
    if (a.size() < 4) {
        log::Info("WARNING: Quaternion entry is nor formatted properly! proper format for Quaternion is x,y,z,w");
        return Quaternionf::identity();
    }
    return {dotnet::ParseSingle(a[0]), dotnet::ParseSingle(a[1]), dotnet::ParseSingle(a[2]), dotnet::ParseSingle(a[3])};
}

// KSPUtil.cs:894..
std::string WriteVector(const Vector2f& v, std::string_view sep) {
    return dotnet::FormatSingleG(v.x, 9) + std::string(sep) + dotnet::FormatSingleG(v.y, 9);
}
std::string WriteVector(const Vector3f& v, std::string_view sep) {
    const std::string s(sep);
    return dotnet::FormatSingleG(v.x, 9) + s + dotnet::FormatSingleG(v.y, 9) + s + dotnet::FormatSingleG(v.z, 9);
}
std::string WriteVector(const Vector4f& v, std::string_view sep) {
    const std::string s(sep);
    return dotnet::FormatSingleG(v.x, 9) + s + dotnet::FormatSingleG(v.y, 9) + s + dotnet::FormatSingleG(v.z, 9) + s +
           dotnet::FormatSingleG(v.w, 9);
}
std::string WriteQuaternion(const Quaternionf& q, std::string_view sep) {
    const std::string s(sep);
    return dotnet::FormatSingleG(q.x, 9) + s + dotnet::FormatSingleG(q.y, 9) + s + dotnet::FormatSingleG(q.z, 9) + s +
           dotnet::FormatSingleG(q.w, 9);
}

}  // namespace KSPUtil

namespace {
constexpr const char* kTypeNames[] = {"STRING", "BOOL", "UINT", "INT", "FLOAT", "DOUBLE", "VECTOR2", "VECTOR3", "VECTOR4",
                                      "QUATERNION", "UNKNOWN"};

// Enum.Parse(typeof(Type), s): trims; a numeric string gives that value (even if undefined); otherwise the exact
// (case-sensitive) member name; anything else throws.
KSPParseable::Type EnumParse(std::string_view s) {
    const std::string t = dotnet::Trim(s);
    int32_t n = 0;
    if (!t.empty() && (std::isdigit(static_cast<unsigned char>(t[0])) || t[0] == '-' || t[0] == '+') &&
        dotnet::TryParseInt32(t, n))
        return static_cast<KSPParseable::Type>(n);
    for (int i = 0; i < 11; ++i)
        if (t == kTypeNames[i]) return static_cast<KSPParseable::Type>(i);
    throw std::invalid_argument("Requested value '" + t + "' was not found.");
}
}  // namespace

std::string KSPParseable::TypeName(Type t) {
    const int i = static_cast<int>(t);
    if (i >= 0 && i < 11) return kTypeNames[i];
    return dotnet::FormatInt64(i);  // Enum.ToString of an undefined value
}

// KSPParseable.cs: KSPParseable(string Value)
KSPParseable::KSPParseable(std::string_view text) {
    value = "";
    type = Type::UNKNOWN;
    try {
        const auto a = dotnet::Split(text, ",", false);
        value = dotnet::Trim(a[0]);
        if (a.size() > 1) type = EnumParse(dotnet::Trim(a.back()));
    } catch (...) {
    }
}

KSPParseable KSPParseable::FromBool(bool v) { return Make(dotnet::FormatBool(v), Type::BOOL); }
KSPParseable KSPParseable::FromUInt(uint32_t v) { return Make(dotnet::FormatUInt64(v), Type::UINT); }
KSPParseable KSPParseable::FromInt(int32_t v) { return Make(dotnet::FormatInt64(v), Type::INT); }
KSPParseable KSPParseable::FromFloat(float v) { return Make(dotnet::FormatSingleG(v), Type::FLOAT); }
KSPParseable KSPParseable::FromDouble(double v) { return Make(dotnet::FormatDoubleG(v), Type::DOUBLE); }

namespace {
bool ParseBoolOrThrow(const std::string& s) {
    bool b = false;
    if (!dotnet::TryParseBool(s, b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
    return b;
}
uint32_t ParseUIntOrThrow(const std::string& s) {
    uint32_t u = 0;
    if (!dotnet::TryParseUInt32(s, u)) throw dotnet::FormatException("Input string was not in a correct format.");
    return u;
}
}  // namespace

int32_t KSPParseable::value_int() const { return type != Type::INT ? 0 : dotnet::ParseInt32(value); }
uint32_t KSPParseable::value_uint() const { return type != Type::UINT ? 0u : ParseUIntOrThrow(value); }
float KSPParseable::value_float() const { return type != Type::FLOAT ? 0.0f : dotnet::ParseSingle(value); }
double KSPParseable::value_double() const { return type != Type::DOUBLE ? 0.0 : dotnet::ParseDouble(value); }
bool KSPParseable::value_bool() const { return type != Type::BOOL ? false : ParseBoolOrThrow(value); }
Vector2f KSPParseable::value_v2() const { return type != Type::VECTOR2 ? Vector2f::zero() : KSPUtil::ParseVector2(value); }
Vector3f KSPParseable::value_v3() const { return type != Type::VECTOR3 ? Vector3f::zero() : KSPUtil::ParseVector3(value); }
Vector4f KSPParseable::value_v4() const { return type != Type::VECTOR4 ? Vector4f::zero() : KSPUtil::ParseVector4(value); }
Quaternionf KSPParseable::value_quat() const {
    return type != Type::QUATERNION ? Quaternionf::identity() : KSPUtil::ParseQuaternion(value);
}

std::string KSPParseable::Save() const { return value + ", " + TypeName(type); }

namespace TypeParsers {

// IntTypeParser.cs
ParsedObject ParseInt(std::string_view s) {
    int32_t i = 0;
    if (dotnet::TryParseInt32(s, i)) return i;
    int64_t l = 0;
    if (dotnet::TryParseInt64(s, l)) {
        if (l > 2147483647LL) return INT32_MAX;
        if (l < -2147483648LL) return INT32_MIN;
    }
    return int32_t(0);
}

// FloatTypeParser.cs (failure: boxed int 0)
ParsedObject ParseFloat(std::string_view s) {
    float f = 0.0f;
    if (dotnet::TryParseSingle(s, f)) return f;
    return int32_t(0);
}

// DoubleTypeParser.cs (failure: boxed int 0)
ParsedObject ParseDouble(std::string_view s) {
    double d = 0.0;
    if (dotnet::TryParseDouble(s, d)) return d;
    return int32_t(0);
}

ParsedObject ParseString(std::string_view s) { return std::string(s); }

// *TypeParser.Convert: o.ToString()
std::string Convert(const ParsedObject& o) {
    struct V {
        std::string operator()(const std::string& s) const { return s; }
        std::string operator()(int32_t i) const { return dotnet::FormatInt64(i); }
        std::string operator()(float f) const { return dotnet::FormatSingleG(f); }
        std::string operator()(double d) const { return dotnet::FormatDoubleG(d); }
    };
    return std::visit(V{}, o);
}

}  // namespace TypeParsers

}  // namespace ksp
