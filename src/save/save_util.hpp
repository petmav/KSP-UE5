// Private helpers of the WP-04 save model: the C# parse calls (bool/int/uint/ulong/float/double.Parse with their
// exception types), Enum.ToString / Enum.Parse, Convert.ToString of boxed values.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/config/parse_extensions.hpp"

namespace ksp::save_detail {

// bool.Parse (Boolean.cs: FormatException for anything but True/False, case-insensitive, trimmed)
inline bool ParseBool(std::string_view s) {
    bool b = false;
    if (!dotnet::TryParseBool(s, b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
    return b;
}
// int.Parse
inline int32_t ParseInt(std::string_view s) { return dotnet::ParseInt32(s); }
// uint.Parse
inline uint32_t ParseUInt(std::string_view s) {
    uint32_t v = 0;
    if (dotnet::TryParseUInt32(s, v)) return v;
    int64_t w = 0;
    uint64_t u = 0;
    if (dotnet::TryParseInt64(s, w) || dotnet::TryParseUInt64(s, u))
        throw dotnet::OverflowException("Value was either too large or too small for a UInt32.");
    throw dotnet::FormatException("Input string was not in a correct format.");
}
// ulong.Parse
inline uint64_t ParseULong(std::string_view s) {
    uint64_t v = 0;
    if (dotnet::TryParseUInt64(s, v)) return v;
    int64_t w = 0;
    if (dotnet::TryParseInt64(s, w)) throw dotnet::OverflowException("Value was either too large or too small for a UInt64.");
    throw dotnet::FormatException("Input string was not in a correct format.");
}
// float.Parse / double.Parse
inline float ParseFloat(std::string_view s) { return dotnet::ParseSingle(s); }
inline double ParseDouble(std::string_view s) { return dotnet::ParseDouble(s); }

// Enum.ToString() / Convert.ToString(enum)
template <class E>
std::string EnumName(E e) {
    return dotnet::EnumToString(e);
}
// (E)Enum.Parse(typeof(E), s)
template <class E>
E ParseEnum(std::string_view s) {
    return dotnet::EnumParseFull<E>(s);
}

// ConfigNode.TryGetEnum<T>(name, ref value, default) (ConfigNode.cs:4285: Enum.IsDefined + Enum.Parse of the exact
// string; value = default otherwise)
template <class E>
bool TryGetEnum(const ConfigNode& node, std::string_view name, E& value, E defaultValue) {
    value = defaultValue;
    const std::string* s = node.GetValuePtr(name);
    if (!s) return false;
    for (const auto& m : dotnet::EnumTraits<E>::members)
        if (*s == m.name) {
            value = m.value;
            return true;
        }
    return false;
}

// string == null checks: C# strings that can be null are std::optional<std::string>
inline const std::string& OrEmpty(const std::optional<std::string>& s) {
    static const std::string empty;
    return s ? *s : empty;
}

// ParseExtensions.cs:8 ArrayMinSize
template <class T>
void ArrayMinSize(int minSize, std::vector<T>& array) {
    if (static_cast<int>(array.size()) >= minSize) return;
    array.resize(static_cast<size_t>(minSize));
}

// ParseExtensions.cs:71 TryParseEnumIntArray<T> (Enum.Parse per element; only ArgumentException is caught, the array
// keeps the elements parsed so far and default values after them)
template <class E>
bool TryParseEnumIntArray(std::string_view text, std::vector<E>& flags) {
    const std::vector<std::string> array = ParseExtensions::ParseArray(text);
    flags.assign(array.size(), E{});
    for (size_t i = 0; i < flags.size(); ++i) {
        try {
            flags[i] = dotnet::EnumParseFull<E>(array[i]);
        } catch (const ksp::ArgumentException&) {
            return false;
        }
    }
    return true;
}

// ConfigNode.WriteEnumIntArray<T> ((int) of each element joined with ",")
template <class E>
std::string WriteEnumIntArray(const std::vector<E>& flags) {
    std::string s;
    for (size_t i = 0; i < flags.size(); ++i) {
        if (i > 0) s += ",";
        s += dotnet::FormatInt64(static_cast<int>(flags[i]));
    }
    return s;
}

// ConfigNode.WriteBoolArray / WriteStringArray (null strings append nothing)
inline std::string WriteBoolArray(const std::vector<bool>& flags) {
    std::string s;
    for (size_t i = 0; i < flags.size(); ++i) {
        if (i > 0) s += ",";
        s += dotnet::FormatBool(flags[i]);
    }
    return s;
}
inline std::string WriteStringArray(const std::vector<std::optional<std::string>>& strings) {
    std::string s;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) s += ",";
        if (strings[i]) s += *strings[i];
    }
    return s;
}

// AddValue(string name, string value) with a C# null value: Debug.LogError + ""
void AddNullableValue(ConfigNode& node, std::string_view name, const std::optional<std::string>& value);

// the first child node with this name, as the shared object (C# reference aliasing)
ConfigNode::NodePtr FirstChildShared(const ConfigNode& node, std::string_view name);

}  // namespace ksp::save_detail
