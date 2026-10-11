// WP-50 internal helpers shared by the input .cpp files: the C# parse calls the binding Load methods make.
#pragma once

#include <string>

#include "ksp/config/config_node.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"

namespace ksp::input_detail {

// int.Parse(string)
inline int ParseInt(const std::string& s) { return dotnet::ParseInt32(s); }

// ulong.Parse(string): NumberStyles.Integer
inline uint64_t ParseULong(const std::string& s) {
    uint64_t v = 0;
    if (dotnet::TryParseUInt64(s, v)) return v;
    bool digits = false;
    for (char c : dotnet::Trim(s)) {
        if (c >= '0' && c <= '9') digits = true;
        else if (c != '+' && c != '-') { digits = false; break; }
    }
    if (digits) throw dotnet::OverflowException("Value was either too large or too small for a UInt64.");
    throw dotnet::FormatException("Input string was not in a correct format.");
}

// float.Parse(string, CultureInfo.InvariantCulture)
inline float ParseFloatInvariant(const std::string& s) { return dotnet::ParseSingle(s); }

// bool.Parse(string)
inline bool ParseBool(const std::string& s) {
    bool b = false;
    if (!dotnet::TryParseBool(s, b)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
    return b;
}

// node.GetValue(key) where C# null and "" behave alike for the callers
inline std::string GetValueOrEmpty(const ConfigNode& node, const char* key) {
    auto v = node.GetValue(key);
    return v ? *v : std::string();
}

}  // namespace ksp::input_detail
