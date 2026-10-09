// The C# primitive parsers the loaders call, with the runtime's semantics (Mono, en-US culture):
// float.Parse / double.Parse (NumberStyles.Float | AllowThousands), int.Parse, bool.Parse, and their TryParse forms.
// Throwing forms raise dotnet::FormatException / dotnet::OverflowException like C#.
#pragma once

#include <cstdint>
#include <string_view>

#include "ksp/dotnet/number.hpp"

namespace ksp::cs {

inline float FloatParse(std::string_view s) { return dotnet::ParseSingle(s); }
inline double DoubleParse(std::string_view s) { return dotnet::ParseDouble(s); }
inline int32_t IntParse(std::string_view s) { return dotnet::ParseInt32(s); }
inline bool FloatTryParse(std::string_view s, float& r) { return dotnet::TryParseSingle(s, r); }
inline bool DoubleTryParse(std::string_view s, double& r) { return dotnet::TryParseDouble(s, r); }
inline bool IntTryParse(std::string_view s, int32_t& r) { return dotnet::TryParseInt32(s, r); }
inline bool BoolTryParse(std::string_view s, bool& r) { return dotnet::TryParseBool(s, r); }
// bool.Parse: FormatException("String was not recognized as a valid Boolean.")
inline bool BoolParse(std::string_view s) {
    bool r = false;
    if (!dotnet::TryParseBool(s, r)) throw dotnet::FormatException("String was not recognized as a valid Boolean.");
    return r;
}

}  // namespace ksp::cs
