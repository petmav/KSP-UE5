// UTF-16 views of kspcore's UTF-8 strings, for C# code whose behaviour depends on string.Length / char indices
// (char = UTF-16 code unit): GetHashCode_Net35, PrintSpacedStringFromCamelcase, char.IsUpper / char.ToUpper.
#pragma once

#include <string>
#include <string_view>

#include "ksp/dotnet/text.hpp"

namespace ksp::dotnet {

// UTF-8 -> UTF-16 code units (invalid bytes become U+FFFD, like .NET's decoder)
inline std::u16string ToUtf16(std::string_view s) {
    std::u16string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        const char32_t c = NextCodePoint(s, i);
        if (c >= 0x10000) {
            const char32_t v = c - 0x10000;
            out.push_back(static_cast<char16_t>(0xD800 + (v >> 10)));
            out.push_back(static_cast<char16_t>(0xDC00 + (v & 0x3FF)));
        } else {
            out.push_back(static_cast<char16_t>(c));
        }
    }
    return out;
}

// UTF-16 -> UTF-8 (a lone surrogate becomes U+FFFD)
inline std::string FromUtf16(std::u16string_view s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        char32_t c = s[i];
        if (c >= 0xD800 && c <= 0xDBFF && i + 1 < s.size() && s[i + 1] >= 0xDC00 && s[i + 1] <= 0xDFFF) {
            c = 0x10000 + ((c - 0xD800) << 10) + (s[i + 1] - 0xDC00);
            ++i;
        } else if (c >= 0xD800 && c <= 0xDFFF) {
            c = 0xFFFD;
        }
        AppendUtf8(out, c);
    }
    return out;
}

// char.IsUpper / char.ToUpper (invariant mapping) for Basic Latin, Latin-1, Greek and Cyrillic; other characters are
// treated as not upper-case and unchanged (KSP applies these to ASCII identifiers).
inline bool CharIsUpper(char16_t c) {
    return (c >= u'A' && c <= u'Z') || (c >= 0x00C0 && c <= 0x00DE && c != 0x00D7) ||
           (c >= 0x0391 && c <= 0x03A9 && c != 0x03A2) || (c >= 0x0400 && c <= 0x042F);
}
inline char16_t CharToUpper(char16_t c) {
    if (c >= u'a' && c <= u'z') return static_cast<char16_t>(c - 32);
    if (c >= 0x00E0 && c <= 0x00FE && c != 0x00F7) return static_cast<char16_t>(c - 32);
    if (c == 0x00FF) return 0x0178;
    if (c >= 0x03B1 && c <= 0x03C9 && c != 0x03C2) return static_cast<char16_t>(c - 32);
    if (c >= 0x0430 && c <= 0x044F) return static_cast<char16_t>(c - 32);
    if (c >= 0x0450 && c <= 0x045F) return static_cast<char16_t>(c - 80);
    return c;
}

}  // namespace ksp::dotnet
