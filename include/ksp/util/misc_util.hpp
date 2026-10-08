// Small WP-03 helpers: StringExtensions.cs (GetHashCode_Net35), StringBuilderCache.cs (Format), MinMaxColor.cs,
// IntersectInformation.cs, DelegateExtensions.cs and CallbackUtil.Clear (no-ops, sic).
//
// StringBuilderCache.Acquire/Release/ToStringAndRelease only pool StringBuilder objects (no observable behaviour);
// C++ code builds std::string directly.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/core/delegate.hpp"
#include "ksp/unity/color.hpp"

namespace ksp {

namespace StringExtensions {
// StringExtensions.cs:3 GetHashCode_Net35: the .NET 3.5 string hash over UTF-16 code units (int arithmetic wraps)
int GetHashCode_Net35(std::string_view str);
}  // namespace StringExtensions

namespace StringBuilderCache {
// StringBuilderCache.cs:39 Format = StringBuilder.AppendFormat(format, args): "{index[,alignment][:format]}", "{{" / "}}"
// escapes. Arguments are already strings (C#: object.ToString(); a format specifier is ignored for strings).
// FormatException("Input string was not in a correct format.") on a malformed format or an index >= args.size().
std::string Format(std::string_view format, const std::vector<std::string>& args);
}  // namespace StringBuilderCache

// UnityEngine.Color.Lerp (managed: t = Clamp01(t); a + (b - a) * t per component, Mono float rule)
Color ColorLerp(const Color& a, const Color& b, float t);

// MinMaxColor.cs
struct MinMaxColor {
    Color min;
    Color max;
    Color GetLerp(float t) const { return ColorLerp(min, max, t); }  // MinMaxColor.cs:12
};

// IntersectInformation.cs (the constructor stores i1Dist in intersect2Distance and i2Dist in intersect1Distance -- sic)
struct IntersectInformation {
    int numberOfIntersections = 0;
    double intersect1UT = 0.0;
    double intersect2Distance = 0.0;
    double intersect2UT = 0.0;
    double intersect1Distance = 0.0;

    IntersectInformation() = default;
    IntersectInformation(int num, double i1Dist, double i1UT, double i2Dist, double i2UT)
        : numberOfIntersections(num), intersect1UT(i1UT), intersect2Distance(i1Dist), intersect2UT(i2UT),
          intersect1Distance(i2Dist) {}
};

// DelegateExtensions.cs / CallbackUtil.cs:136-162 Clear: assigns a new delegate to the by-value parameter, so the
// caller's delegate is unchanged (sic). Ported as no-ops.
namespace DelegateExtensions {
template <class D, class... R>
void Clear(const D& /*f*/, const R&... /*defaultValue*/) {}
}  // namespace DelegateExtensions
namespace CallbackUtil {
template <class... Ts>
void Clear(const Callback<Ts...>& /*cb*/) {}
}  // namespace CallbackUtil

}  // namespace ksp
