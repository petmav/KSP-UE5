// Enum.Parse(Type, string) with the full .NET semantics (referencesource System/Enum.cs TryParseEnum, which Mono 5.11
// uses): the value is trimmed; a leading digit, '-' or '+' parses it as a number of the underlying type (any number is
// accepted, defined or not; OverflowException propagates); otherwise the comma-separated names are trimmed, matched
// case-sensitively and OR-ed together. Unknown names, empty strings: ArgumentException. The member table is the
// declaration-order EnumTraits<E> of ksp/dotnet/enum.hpp (EnumParse there accepts a single exact name only).
#pragma once

#include <string>
#include <string_view>
#include <type_traits>

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"

namespace ksp::dotnet {

namespace detail {
// Convert.ChangeType(value, underlyingType): Int32/Int64/UInt32... Parse with NumberStyles.Integer
template <class U>
bool ParseUnderlying(std::string_view s, U& out, bool& overflow) {
    overflow = false;
    if constexpr (std::is_same_v<U, int32_t>) {
        int64_t v = 0;
        if (!TryParseInt64(s, v)) return false;
        if (v < INT32_MIN || v > INT32_MAX) { overflow = true; return false; }
        out = static_cast<int32_t>(v);
        return true;
    } else if constexpr (std::is_same_v<U, uint32_t>) {
        int64_t v = 0;
        if (!TryParseInt64(s, v)) return false;
        if (v < 0 || v > static_cast<int64_t>(UINT32_MAX)) { overflow = true; return false; }
        out = static_cast<uint32_t>(v);
        return true;
    } else if constexpr (std::is_same_v<U, int64_t>) {
        return TryParseInt64(s, out);
    } else {
        uint64_t v = 0;
        if (!TryParseUInt64(s, v)) return false;
        out = static_cast<U>(v);
        return true;
    }
}
}  // namespace detail

// Enum.TryParse semantics without throwing (false where Enum.Parse would throw ArgumentException)
template <class E>
bool EnumTryParseFull(std::string_view text, E& result) {
    using U = std::underlying_type_t<E>;
    const std::string value = Trim(text);
    if (value.empty()) return false;
    const char c0 = value[0];
    if ((c0 >= '0' && c0 <= '9') || c0 == '-' || c0 == '+') {
        U u{};
        bool overflow = false;
        if (detail::ParseUnderlying<U>(value, u, overflow)) {
            result = static_cast<E>(u);
            return true;
        }
        if (overflow) throw OverflowException("Value was either too large or too small for the enum's underlying type.");
        // FormatException: fall through and parse as names ("1, 2" etc.)
    }
    const auto& table = detail::GetTable<E>();
    uint64_t acc = 0;
    for (const std::string& part : Split(value, ",", false)) {
        const std::string name = Trim(part);
        bool found = false;
        for (size_t j = 0; j < table.names.size(); ++j)
            if (table.names[j] == name) {
                acc |= table.values[j];
                found = true;
                break;
            }
        if (!found) return false;
    }
    result = static_cast<E>(static_cast<U>(acc));
    return true;
}

// Enum.Parse(typeof(E), text): ArgumentException when not a member / number (message as in .NET)
template <class E>
E EnumParseFull(std::string_view text) {
    E result{};
    if (!EnumTryParseFull<E>(text, result)) {
        if (Trim(text).empty())
            throw ksp::ArgumentException("Must specify valid information for parsing in the string.");
        throw ksp::ArgumentException("Requested value '" + std::string(text) + "' was not found.");
    }
    return result;
}

}  // namespace ksp::dotnet
