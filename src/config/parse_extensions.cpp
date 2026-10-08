// Port of Assembly-CSharp/ParseExtensions.cs (+ ColorUtility.TryParseHtmlString, Unity native, documented behaviour).
#include "ksp/config/parse_extensions.hpp"

#include <cctype>

#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp::ParseExtensions {

// ParseExtensions.cs ParseArray(string text, StringSplitOptions options, params char[] separators)
std::vector<std::string> ParseArray(std::string_view text, std::string_view separators, bool removeEmptyEntries) {
    if (text.empty()) return {};
    if (separators.empty()) separators = ",";
    std::vector<std::string> array = dotnet::Split(text, separators, removeEmptyEntries);
    for (std::string& s : array) s = dotnet::Trim(s);
    return array;
}

// ParseExtensions.cs TryParseBoolArray
bool TryParseBoolArray(std::string_view text, std::vector<bool>& flags) {
    const std::vector<std::string> array = ParseArray(text);
    flags.assign(array.size(), false);
    for (size_t i = 0; i < array.size(); ++i) {
        bool b = false;
        if (!dotnet::TryParseBool(array[i], b)) return false;
        flags[i] = b;
    }
    return true;
}

namespace {
template <size_t N>
bool ParseFloats(std::string_view text, float (&out)[N]) {
    const std::vector<std::string> array = ParseArray(text);
    if (array.size() != N) return false;
    for (size_t i = 0; i < N; ++i)
        if (!dotnet::TryParseSingle(array[i], out[i])) return false;
    return true;
}
template <size_t N>
bool ParseDoubles(std::string_view text, double (&out)[N]) {
    const std::vector<std::string> array = ParseArray(text);
    if (array.size() != N) return false;
    for (size_t i = 0; i < N; ++i)
        if (!dotnet::TryParseDouble(array[i], out[i])) return false;
    return true;
}
}  // namespace

// ParseExtensions.cs TryParseVector3
bool TryParseVector3(std::string_view text, Vector3f& result) {
    float v[3];
    if (!ParseFloats(text, v)) { result = Vector3f::zero(); return false; }
    result = {v[0], v[1], v[2]};
    return true;
}

// ParseExtensions.cs TryParseVector3d
bool TryParseVector3d(std::string_view text, Vector3d& result) {
    double v[3];
    if (!ParseDoubles(text, v)) { result = Vector3d::zero(); return false; }
    result = {v[0], v[1], v[2]};
    return true;
}

// ParseExtensions.cs TryParseVector2
bool TryParseVector2(std::string_view text, Vector2f& result) {
    float v[2];
    if (!ParseFloats(text, v)) { result = Vector2f::zero(); return false; }
    result = {v[0], v[1]};
    return true;
}

// ParseExtensions.cs TryParseVector2d
bool TryParseVector2d(std::string_view text, Vector2d& result) {
    double v[2];
    if (!ParseDoubles(text, v)) { result = Vector2d::zero(); return false; }
    result = {v[0], v[1]};
    return true;
}

// ParseExtensions.cs TryParseVector4
bool TryParseVector4(std::string_view text, Vector4f& result) {
    float v[4];
    if (!ParseFloats(text, v)) { result = Vector4f::zero(); return false; }
    result = {v[0], v[1], v[2], v[3]};
    return true;
}

// ParseExtensions.cs TryParseVector4d
bool TryParseVector4d(std::string_view text, Vector4d& result) {
    double v[4];
    if (!ParseDoubles(text, v)) { result = Vector4d::zero(); return false; }
    result = {v[0], v[1], v[2], v[3]};
    return true;
}

// ParseExtensions.cs TryParseQuaternion
bool TryParseQuaternion(std::string_view text, Quaternionf& result) {
    float v[4];
    if (!ParseFloats(text, v)) { result = Quaternionf::identity(); return false; }
    result = {v[0], v[1], v[2], v[3]};
    return true;
}

// ParseExtensions.cs TryParseQuaternionD
bool TryParseQuaternionD(std::string_view text, QuaternionD& result) {
    double v[4];
    if (!ParseDoubles(text, v)) { result = QuaternionD::identity(); return false; }
    result = {v[0], v[1], v[2], v[3]};
    return true;
}

// ParseExtensions.cs TryParseRect
bool TryParseRect(std::string_view text, Rect& result) {
    float v[4];
    if (!ParseFloats(text, v)) { result = Rect(); return false; }
    result = {v[0], v[1], v[2], v[3]};
    return true;
}

// ParseExtensions.cs TryParseColor
bool TryParseColor(std::string_view text, Color& result) {
    const std::vector<std::string> array = ParseArray(text);
    if (array.size() == 3) {
        float r, g, b;
        if (!dotnet::TryParseSingle(array[0], r) || !dotnet::TryParseSingle(array[1], g) || !dotnet::TryParseSingle(array[2], b)) {
            result = Color::white();
            return false;
        }
        result = Color(Mathf::Clamp01(r), Mathf::Clamp01(g), Mathf::Clamp01(b));
        return true;
    }
    if (array.size() == 4) {
        float r, g, b, a;
        if (!dotnet::TryParseSingle(array[0], r) || !dotnet::TryParseSingle(array[1], g) ||
            !dotnet::TryParseSingle(array[2], b) || !dotnet::TryParseSingle(array[3], a)) {
            result = Color::white();
            return false;
        }
        result = Color(Mathf::Clamp01(r), Mathf::Clamp01(g), Mathf::Clamp01(b), Mathf::Clamp01(a));
        return true;
    }
    result = Color::white();
    return false;
}

// ParseExtensions.cs TryParseColor32
bool TryParseColor32(std::string_view text, Color32& result) {
    const std::vector<std::string> array = ParseArray(text);
    uint8_t v[4];
    if (array.size() != 4) { result = Color32(); return false; }
    for (size_t i = 0; i < 4; ++i)
        if (!dotnet::TryParseByte(array[i], v[i])) { result = Color32(); return false; }
    result = Color32(v[0], v[1], v[2], v[3]);
    return true;
}

}  // namespace ksp::ParseExtensions

namespace ksp::ColorUtility {

namespace {
int HexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
}  // namespace

bool TryParseHtmlString(std::string_view s, Color& color) {
    if (s.empty()) return false;
    if (s[0] == '#') {
        const std::string_view h = s.substr(1);
        for (char c : h)
            if (HexVal(c) < 0) return false;
        uint8_t ch[4] = {0, 0, 0, 255};
        if (h.size() == 3 || h.size() == 4) {
            for (size_t i = 0; i < h.size(); ++i) ch[i] = static_cast<uint8_t>(HexVal(h[i]) * 17);
        } else if (h.size() == 6 || h.size() == 8) {
            for (size_t i = 0; i < h.size() / 2; ++i) ch[i] = static_cast<uint8_t>(HexVal(h[2 * i]) * 16 + HexVal(h[2 * i + 1]));
        } else {
            return false;
        }
        color = Color32(ch[0], ch[1], ch[2], ch[3]).ToColor();
        return true;
    }
    struct Named {
        const char* name;
        uint8_t r, g, b;
    };
    static const Named kNamed[] = {
        {"red", 255, 0, 0},       {"cyan", 0, 255, 255},     {"blue", 0, 0, 255},       {"darkblue", 0, 0, 160},
        {"lightblue", 173, 216, 230}, {"purple", 128, 0, 128}, {"yellow", 255, 255, 0},  {"lime", 0, 255, 0},
        {"fuchsia", 255, 0, 255}, {"white", 255, 255, 255},  {"silver", 192, 192, 192}, {"grey", 128, 128, 128},
        {"black", 0, 0, 0},       {"orange", 255, 165, 0},   {"brown", 165, 42, 42},    {"maroon", 128, 0, 0},
        {"green", 0, 128, 0},     {"olive", 128, 128, 0},    {"navy", 0, 0, 128},       {"teal", 0, 128, 128},
        {"aqua", 0, 255, 255},    {"magenta", 255, 0, 255},
    };
    for (const Named& n : kNamed) {
        if (dotnet::EqualsOrdinalIgnoreCase(s, n.name)) {
            color = Color32(n.r, n.g, n.b, 255).ToColor();
            return true;
        }
    }
    return false;
}

}  // namespace ksp::ColorUtility
