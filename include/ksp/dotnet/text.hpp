// .NET string semantics that KSP relies on, on UTF-8 std::string.
//
// KSP is C# running on Unity's Mono: strings are UTF-16 and many operations (Trim, Split, IndexOf, file I/O) carry
// .NET-specific rules. kspcore stores text as UTF-8; every helper here reproduces the .NET result on the UTF-8 encoding
// of the same text. ASCII-only operations (Split on ASCII separators, IndexOf of ASCII characters, Replace of ASCII)
// are byte-exact on UTF-8 because UTF-8 continuation bytes never collide with ASCII.
//
// Behavioural references: decompiled/KSP-Decompiled/Mono/mscorlib/System/String*.cs, Char.cs, IO/File.cs,
// IO/StreamReader.cs (Unity's Mono BCL, i.e. what KSP runs on).
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ksp::dotnet {

// char.IsWhiteSpace for a Unicode scalar value (Mono/CoreFX table: U+0009..U+000D, U+0020, U+0085, U+00A0, U+1680,
// U+2000..U+200A, U+2028, U+2029, U+202F, U+205F, U+3000).
bool IsWhiteSpace(char32_t c) noexcept;

// string.Trim(): removes leading/trailing char.IsWhiteSpace characters (UTF-8 aware).
std::string Trim(std::string_view s);
std::string_view TrimView(std::string_view s) noexcept;

// string.Split(char[] separators, StringSplitOptions). Separators must be ASCII.
std::vector<std::string> Split(std::string_view s, std::string_view separators, bool removeEmptyEntries);

// string.Replace(char, char) / string.Replace(string, string) for ASCII patterns (byte-exact on UTF-8).
std::string Replace(std::string_view s, char from, char to);
std::string Replace(std::string_view s, std::string_view from, std::string_view to);

// string.StartsWith(string): KSP calls the culture-sensitive overload; for the ASCII identifiers used in cfg files this
// is an ordinal prefix test (documented approximation: culture-ignorable code points are not ignored).
bool StartsWith(std::string_view s, std::string_view prefix) noexcept;

// string.Equals(a, b, StringComparison.OrdinalIgnoreCase) for ASCII-insensitive comparison.
bool EqualsOrdinalIgnoreCase(std::string_view a, std::string_view b) noexcept;

// string.CompareTo(string) (culture-sensitive, en-US) approximation used by ValueList.SortByName: compares
// case-insensitively first, then lowercase-before-uppercase, then ordinal. Exact for ASCII letters/digits.
int CompareCulture(std::string_view a, std::string_view b) noexcept;

// ---- UTF-8 / UTF-16 / file I/O -------------------------------------------------------------------------------------

// Decode bytes the way StreamReader(path, detectEncodingFromByteOrderMarks: true, UTF8) does: BOM detection for UTF-8,
// UTF-16 LE/BE and UTF-32 LE/BE (BOM removed), otherwise UTF-8. Invalid sequences become U+FFFD. Returns UTF-8.
std::string DecodeStreamText(std::string_view bytes);

// File.ReadAllLines: decode (see above) and split on "\r\n", "\n" or "\r"; a trailing terminator does not create an
// empty last line. Returns std::nullopt if the file cannot be opened.
std::optional<std::vector<std::string>> ReadAllLines(const std::string& path);
std::optional<std::string> ReadAllText(const std::string& path);
std::optional<std::string> ReadAllBytes(const std::string& path);

// Text -> lines exactly like File.ReadAllLines on already decoded text.
std::vector<std::string> SplitLinesLikeReadLine(std::string_view text);

// Write raw bytes (create/truncate). Returns false on failure.
bool WriteAllBytes(const std::string& path, std::string_view bytes);

// Convert UTF-8 file paths for the Windows API.
std::wstring Utf8ToWide(std::string_view s);

// Iterate code points: decodes one UTF-8 scalar at s[i], advancing i. Invalid -> U+FFFD (one byte consumed).
char32_t NextCodePoint(std::string_view s, size_t& i) noexcept;
void AppendUtf8(std::string& out, char32_t c);

}  // namespace ksp::dotnet
