#include "ksp/dotnet/text.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ksp::dotnet {

// Mono mscorlib System/Char.cs IsWhiteSpace (Latin-1 fast path + Unicode categories Zs/Zl/Zp).
bool IsWhiteSpace(char32_t c) noexcept {
    if (c <= 0xFF) return c == 0x20 || (c >= 0x09 && c <= 0x0D) || c == 0x85 || c == 0xA0;
    return c == 0x1680 || (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 || c == 0x202F || c == 0x205F ||
           c == 0x3000;
}

char32_t NextCodePoint(std::string_view s, size_t& i) noexcept {
    const auto b0 = static_cast<unsigned char>(s[i]);
    if (b0 < 0x80) { ++i; return b0; }
    int len = 0;
    char32_t cp = 0;
    char32_t minv = 0;
    if ((b0 & 0xE0) == 0xC0) { len = 2; cp = b0 & 0x1F; minv = 0x80; }
    else if ((b0 & 0xF0) == 0xE0) { len = 3; cp = b0 & 0x0F; minv = 0x800; }
    else if ((b0 & 0xF8) == 0xF0) { len = 4; cp = b0 & 0x07; minv = 0x10000; }
    else { ++i; return 0xFFFD; }
    if (i + len > s.size()) { ++i; return 0xFFFD; }
    for (int k = 1; k < len; ++k) {
        const auto b = static_cast<unsigned char>(s[i + k]);
        if ((b & 0xC0) != 0x80) { ++i; return 0xFFFD; }
        cp = (cp << 6) | (b & 0x3F);
    }
    if (cp < minv || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) { ++i; return 0xFFFD; }
    i += len;
    return cp;
}

void AppendUtf8(std::string& out, char32_t c) {
    if (c < 0x80) out.push_back(static_cast<char>(c));
    else if (c < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (c >> 6)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else if (c < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (c >> 12)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (c >> 18)));
        out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }
}

std::string_view TrimView(std::string_view s) noexcept {
    // leading
    size_t start = 0;
    while (start < s.size()) {
        const auto b = static_cast<unsigned char>(s[start]);
        if (b < 0x80) {
            if (!IsWhiteSpace(b)) break;
            ++start;
            continue;
        }
        size_t j = start;
        const char32_t cp = NextCodePoint(s, j);
        if (!IsWhiteSpace(cp)) break;
        start = j;
    }
    // trailing: walk back to the start of the last code point
    size_t end = s.size();
    while (end > start) {
        const auto b = static_cast<unsigned char>(s[end - 1]);
        if (b < 0x80) {
            if (!IsWhiteSpace(b)) break;
            --end;
            continue;
        }
        size_t lead = end - 1;
        while (lead > start && (static_cast<unsigned char>(s[lead]) & 0xC0) == 0x80) --lead;
        size_t j = lead;
        const char32_t cp = NextCodePoint(s, j);
        if (j != end || !IsWhiteSpace(cp)) break;
        end = lead;
    }
    return s.substr(start, end - start);
}

std::string Trim(std::string_view s) { return std::string(TrimView(s)); }

std::vector<std::string> Split(std::string_view s, std::string_view separators, bool removeEmptyEntries) {
    std::vector<std::string> out;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || separators.find(s[i]) != std::string_view::npos) {
            if (!removeEmptyEntries || i > start) out.emplace_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    return out;
}

std::string Replace(std::string_view s, char from, char to) {
    std::string r(s);
    std::replace(r.begin(), r.end(), from, to);
    return r;
}

std::string Replace(std::string_view s, std::string_view from, std::string_view to) {
    if (from.empty()) return std::string(s);
    std::string r;
    r.reserve(s.size());
    size_t pos = 0;
    for (;;) {
        const size_t f = s.find(from, pos);
        if (f == std::string_view::npos) break;
        r.append(s.substr(pos, f - pos));
        r.append(to);
        pos = f + from.size();
    }
    r.append(s.substr(pos));
    return r;
}

bool StartsWith(std::string_view s, std::string_view prefix) noexcept { return s.substr(0, prefix.size()) == prefix; }

static char AsciiLower(char c) noexcept { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c; }

bool EqualsOrdinalIgnoreCase(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (AsciiLower(a[i]) != AsciiLower(b[i])) return false;
    return true;
}

int CompareCulture(std::string_view a, std::string_view b) noexcept {
    const size_t n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i) {
        const char la = AsciiLower(a[i]), lb = AsciiLower(b[i]);
        if (la != lb) return static_cast<unsigned char>(la) < static_cast<unsigned char>(lb) ? -1 : 1;
    }
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            const bool aLower = a[i] >= 'a' && a[i] <= 'z';
            return aLower ? -1 : 1;  // lowercase sorts first in en-US
        }
    }
    return 0;
}

// ---- decoding ------------------------------------------------------------------------------------------------------

static std::string Utf16ToUtf8(std::string_view bytes, bool bigEndian) {
    std::string out;
    out.reserve(bytes.size());
    const size_t n = bytes.size() / 2;
    auto unit = [&](size_t k) -> char16_t {
        const auto b0 = static_cast<unsigned char>(bytes[2 * k]), b1 = static_cast<unsigned char>(bytes[2 * k + 1]);
        return bigEndian ? static_cast<char16_t>((b0 << 8) | b1) : static_cast<char16_t>((b1 << 8) | b0);
    };
    for (size_t k = 0; k < n; ++k) {
        const char16_t u = unit(k);
        if (u >= 0xD800 && u <= 0xDBFF && k + 1 < n) {
            const char16_t u2 = unit(k + 1);
            if (u2 >= 0xDC00 && u2 <= 0xDFFF) {
                AppendUtf8(out, 0x10000 + ((static_cast<char32_t>(u) - 0xD800) << 10) + (u2 - 0xDC00));
                ++k;
                continue;
            }
        }
        if (u >= 0xD800 && u <= 0xDFFF) AppendUtf8(out, 0xFFFD);
        else AppendUtf8(out, u);
    }
    if (bytes.size() % 2) AppendUtf8(out, 0xFFFD);
    return out;
}

static std::string Utf32ToUtf8(std::string_view bytes, bool bigEndian) {
    std::string out;
    const size_t n = bytes.size() / 4;
    for (size_t k = 0; k < n; ++k) {
        const auto* p = reinterpret_cast<const unsigned char*>(bytes.data() + 4 * k);
        const char32_t c = bigEndian ? (char32_t(p[0]) << 24 | char32_t(p[1]) << 16 | char32_t(p[2]) << 8 | p[3])
                                     : (char32_t(p[3]) << 24 | char32_t(p[2]) << 16 | char32_t(p[1]) << 8 | p[0]);
        AppendUtf8(out, (c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) ? 0xFFFD : c);
    }
    if (bytes.size() % 4) AppendUtf8(out, 0xFFFD);
    return out;
}

static std::string SanitizeUtf8(std::string_view bytes) {
    // Fast path: valid UTF-8 is returned unchanged.
    size_t i = 0;
    bool valid = true;
    while (i < bytes.size()) {
        if (static_cast<unsigned char>(bytes[i]) < 0x80) { ++i; continue; }
        const size_t before = i;
        const char32_t cp = NextCodePoint(bytes, i);
        if (cp == 0xFFFD && !(i - before == 3 && bytes.substr(before, 3) == "\xEF\xBF\xBD")) { valid = false; break; }
    }
    if (valid) return std::string(bytes);
    std::string out;
    out.reserve(bytes.size() + 16);
    i = 0;
    while (i < bytes.size()) AppendUtf8(out, NextCodePoint(bytes, i));
    return out;
}

std::string DecodeStreamText(std::string_view b) {
    auto at = [&](size_t k) { return static_cast<unsigned char>(b[k]); };
    if (b.size() >= 2) {
        if (at(0) == 0xFE && at(1) == 0xFF) return Utf16ToUtf8(b.substr(2), true);
        if (at(0) == 0xFF && at(1) == 0xFE) {
            if (b.size() >= 4 && at(2) == 0 && at(3) == 0) return Utf32ToUtf8(b.substr(4), false);
            return Utf16ToUtf8(b.substr(2), false);
        }
        if (b.size() >= 3 && at(0) == 0xEF && at(1) == 0xBB && at(2) == 0xBF) return SanitizeUtf8(b.substr(3));
        if (b.size() >= 4 && at(0) == 0 && at(1) == 0 && at(2) == 0xFE && at(3) == 0xFF) return Utf32ToUtf8(b.substr(4), true);
    }
    return SanitizeUtf8(b);
}

std::vector<std::string> SplitLinesLikeReadLine(std::string_view text) {
    std::vector<std::string> lines;
    size_t start = 0, i = 0;
    while (i < text.size()) {
        const char c = text[i];
        if (c == '\r' || c == '\n') {
            lines.emplace_back(text.substr(start, i - start));
            if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') ++i;
            ++i;
            start = i;
        } else {
            ++i;
        }
    }
    if (start < text.size()) lines.emplace_back(text.substr(start));
    return lines;
}

std::wstring Utf8ToWide(std::string_view s) {
#ifdef _WIN32
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
#else
    return std::wstring(s.begin(), s.end());
#endif
}

static FILE* OpenFile(const std::string& path, const char* mode) {
#ifdef _WIN32
    const std::wstring wmode(mode, mode + std::strlen(mode));
    return _wfopen(Utf8ToWide(path).c_str(), wmode.c_str());
#else
    return std::fopen(path.c_str(), mode);
#endif
}

std::optional<std::string> ReadAllBytes(const std::string& path) {
    FILE* f = OpenFile(path, "rb");
    if (!f) return std::nullopt;
    std::string data;
    char buf[1 << 16];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) data.append(buf, n);
    std::fclose(f);
    return data;
}

std::optional<std::string> ReadAllText(const std::string& path) {
    auto bytes = ReadAllBytes(path);
    if (!bytes) return std::nullopt;
    return DecodeStreamText(*bytes);
}

std::optional<std::vector<std::string>> ReadAllLines(const std::string& path) {
    auto text = ReadAllText(path);
    if (!text) return std::nullopt;
    return SplitLinesLikeReadLine(*text);
}

bool WriteAllBytes(const std::string& path, std::string_view bytes) {
    FILE* f = OpenFile(path, "wb");
    if (!f) return false;
    const size_t w = std::fwrite(bytes.data(), 1, bytes.size(), f);
    const bool ok = std::fclose(f) == 0 && w == bytes.size();
    return ok;
}

}  // namespace ksp::dotnet
