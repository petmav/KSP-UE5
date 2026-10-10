// WP-36: the numeric ToString formats the career GUI text uses and ksp::dotnet does not have: "N<d>" (group separators), "F<d>",
// "G<d>" and the custom patterns "0", "0.0", "0.0#", "0.##" (digit placeholders). Text only: nothing in the funds / progress maths
// reads these strings. Invariant culture, Mono rounding of the shortest fixed-point text (unverified for ties).
#pragma once

#include <string>
#include <string_view>

namespace ksp::career {

std::string FormatNumber(double value, bool isFloat, std::string_view format);
inline std::string FormatNumber(float value, std::string_view format) { return FormatNumber(static_cast<double>(value), true, format); }
inline std::string FormatNumber(double value, std::string_view format) { return FormatNumber(value, false, format); }

}  // namespace ksp::career
