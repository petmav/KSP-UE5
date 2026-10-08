// ParseExtensions (Assembly-CSharp/ParseExtensions.cs) and UnityEngine.ColorUtility.TryParseHtmlString.
// Value strings are split on ',' (empty entries removed) and each part trimmed; numbers use float/double.TryParse with
// NumberStyles.Float|AllowThousands (so "1,000" alone parses as 1000, but inside a vector the comma splits).
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp::ParseExtensions {

// ParseExtensions.cs ParseArray(text, separators...) (default separator ','; RemoveEmptyEntries; parts trimmed)
std::vector<std::string> ParseArray(std::string_view text, std::string_view separators = ",", bool removeEmptyEntries = true);

bool TryParseBoolArray(std::string_view text, std::vector<bool>& flags);
bool TryParseVector3(std::string_view text, Vector3f& result);
bool TryParseVector3d(std::string_view text, Vector3d& result);
bool TryParseVector2(std::string_view text, Vector2f& result);
bool TryParseVector2d(std::string_view text, Vector2d& result);
bool TryParseVector4(std::string_view text, Vector4f& result);
bool TryParseVector4d(std::string_view text, Vector4d& result);
bool TryParseQuaternion(std::string_view text, Quaternionf& result);
bool TryParseQuaternionD(std::string_view text, QuaternionD& result);
bool TryParseRect(std::string_view text, Rect& result);
// 3 or 4 components, each Mathf.Clamp01'd; alpha defaults to 1.
bool TryParseColor(std::string_view text, Color& result);
// exactly 4 byte components
bool TryParseColor32(std::string_view text, Color32& result);

}  // namespace ksp::ParseExtensions

namespace ksp::ColorUtility {
// UnityEngine.ColorUtility.TryParseHtmlString (native): "#RGB", "#RGBA", "#RRGGBB", "#RRGGBBAA" or a named colour
// (red, cyan, blue, darkblue, lightblue, purple, yellow, lime, fuchsia, white, silver, grey, black, orange, brown,
// maroon, green, olive, navy, teal, aqua, magenta). Reimplemented from Unity's documentation; no golden data yet.
bool TryParseHtmlString(std::string_view htmlString, Color& color);
}  // namespace ksp::ColorUtility
