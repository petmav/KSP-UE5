// Port of Assembly-CSharp/StringExtensions.cs, StringBuilderCache.cs (Format), MinMaxColor.cs (Color.Lerp).
#include "ksp/util/misc_util.hpp"

#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/utf16.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace StringExtensions {
// StringExtensions.cs:3
int GetHashCode_Net35(std::string_view s) {
    const std::u16string str = dotnet::ToUtf16(s);
    size_t i = 0;
    uint32_t num = 0;
    for (; i + 1 < str.size(); i += 2) {
        const uint32_t num2 = (num << 5) - num + str[i];
        num = (num2 << 5) - num2 + str[i + 1];
    }
    if (i < str.size()) num = (num << 5) - num + str[i];
    return static_cast<int>(num);
}
}  // namespace StringExtensions

namespace StringBuilderCache {
namespace {
[[noreturn]] void FormatError() { throw dotnet::FormatException("Input string was not in a correct format."); }
}  // namespace

// StringBuilderCache.cs:39 -> StringBuilder.AppendFormat (reference source AppendFormatHelper)
std::string Format(std::string_view format, const std::vector<std::string>& args) {
    std::string out;
    size_t pos = 0;
    const size_t len = format.size();
    char ch = '\0';
    while (true) {
        while (pos < len) {
            ch = format[pos];
            ++pos;
            if (ch == '}') {
                if (pos < len && format[pos] == '}') ++pos;
                else FormatError();
            }
            if (ch == '{') {
                if (pos < len && format[pos] == '{') {
                    ++pos;
                } else {
                    --pos;
                    break;
                }
            }
            out.push_back(ch);
        }
        if (pos == len) break;
        ++pos;
        if (pos == len || (ch = format[pos]) < '0' || ch > '9') FormatError();
        int index = 0;
        do {
            index = index * 10 + ch - '0';
            ++pos;
            if (pos == len) FormatError();
            ch = format[pos];
        } while (ch >= '0' && ch <= '9' && index < 1000000);
        if (index >= static_cast<int>(args.size()))
            throw dotnet::FormatException(
                "Index (zero based) must be greater than or equal to zero and less than the size of the argument list.");
        while (pos < len && (ch = format[pos]) == ' ') ++pos;
        bool leftJustify = false;
        int width = 0;
        if (ch == ',') {
            ++pos;
            while (pos < len && format[pos] == ' ') ++pos;
            if (pos == len) FormatError();
            ch = format[pos];
            if (ch == '-') {
                leftJustify = true;
                ++pos;
                if (pos == len) FormatError();
                ch = format[pos];
            }
            if (ch < '0' || ch > '9') FormatError();
            do {
                width = width * 10 + ch - '0';
                ++pos;
                if (pos == len) FormatError();
                ch = format[pos];
            } while (ch >= '0' && ch <= '9' && width < 1000000);
        }
        while (pos < len && (ch = format[pos]) == ' ') ++pos;
        if (ch == ':') {
            ++pos;
            while (true) {
                if (pos == len) FormatError();
                ch = format[pos];
                ++pos;
                if (ch == '{') {
                    if (pos < len && format[pos] == '{') ++pos;
                    else FormatError();
                } else if (ch == '}') {
                    if (pos < len && format[pos] == '}') {
                        ++pos;
                    } else {
                        --pos;
                        break;
                    }
                }
                // the format specifier is collected and ignored: string arguments are not IFormattable
            }
        }
        if (ch != '}') FormatError();
        ++pos;
        const std::string& s = args[static_cast<size_t>(index)];
        const int pad = width - static_cast<int>(dotnet::ToUtf16(s).size());
        if (!leftJustify && pad > 0) out.append(static_cast<size_t>(pad), ' ');
        out += s;
        if (leftJustify && pad > 0) out.append(static_cast<size_t>(pad), ' ');
    }
    return out;
}
}  // namespace StringBuilderCache

// UnityEngine/Color.cs Lerp (managed)
Color ColorLerp(const Color& a, const Color& b, float t) {
    t = Mathf::Clamp01(t);
    return Color(f32store(fx(a.r) + (fx(b.r) - fx(a.r)) * fx(t)), f32store(fx(a.g) + (fx(b.g) - fx(a.g)) * fx(t)),
                 f32store(fx(a.b) + (fx(b.b) - fx(a.b)) * fx(t)), f32store(fx(a.a) + (fx(b.a) - fx(a.a)) * fx(t)));
}

}  // namespace ksp
