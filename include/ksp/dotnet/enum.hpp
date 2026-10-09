// System.Enum name/format semantics for ported C# enums (Mono mscorlib System/Enum.cs, RuntimeType.GetEnumName).
//
// C++ has no reflection: a ported enum that KSP formats, parses or describes declares its metadata once:
//
//   template <> struct ksp::dotnet::EnumTraits<ksp::MyEnum> {
//       static constexpr bool flags = false;                          // [Flags]
//       static constexpr EnumMember<ksp::MyEnum> members[] = {        // C# declaration order
//           {ksp::MyEnum::A, "A"}, {ksp::MyEnum::B, "B", "Shown text"}  // optional [Description("...")]
//       };
//   };
//
// Reproduced: the runtime's value/name table (declaration order, re-sorted by unsigned value with Array.Sort's
// introsort only when not already non-decreasing), GetName's binary search (which picks among duplicate values),
// ToString (name, or the number when undefined; [Flags]: "A, B" built from the highest value down, the number when bits
// remain, the zero member's name or "0" for 0), GetNames (sorted order), Parse of a single defined name.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/number.hpp"

namespace ksp::dotnet {

template <class E>
struct EnumMember {
    E value;
    const char* name;
    const char* description = nullptr;  // [Description("...")], nullptr when absent
};

template <class E>
struct EnumTraits;  // specialise per enum (see the file comment)

namespace detail {

template <class E>
uint64_t EnumToUInt64(E v) {
    using U = std::underlying_type_t<E>;
    if constexpr (std::is_signed_v<U>) return static_cast<uint64_t>(static_cast<int64_t>(static_cast<U>(v)));
    else return static_cast<uint64_t>(static_cast<U>(v));
}

template <class E>
struct EnumTable {
    std::vector<uint64_t> values;
    std::vector<std::string> names;
    std::vector<const char*> descriptions;
};

// Enum.GetCachedValuesAndNames: the runtime returns fields in declaration order and reports whether they are already
// sorted; otherwise Array.Sort(values, names) (introsort on the keys, names moved with them).
template <class E>
const EnumTable<E>& GetTable() {
    static const EnumTable<E> table = [] {
        EnumTable<E> t;
        std::vector<std::pair<uint64_t, size_t>> pairs;
        size_t i = 0;
        bool sorted = true;
        uint64_t previous = 0;
        for (const auto& m : EnumTraits<E>::members) {
            const uint64_t v = EnumToUInt64(m.value);
            if (i > 0 && previous > v) sorted = false;
            previous = v;
            pairs.emplace_back(v, i++);
        }
        if (!sorted)
            ListSort(pairs, [](const std::pair<uint64_t, size_t>& a, const std::pair<uint64_t, size_t>& b) {
                return a.first < b.first ? -1 : (a.first > b.first ? 1 : 0);
            });
        for (const auto& [v, idx] : pairs) {
            t.values.push_back(v);
            t.names.emplace_back(EnumTraits<E>::members[idx].name);
            t.descriptions.push_back(EnumTraits<E>::members[idx].description);
        }
        return t;
    }();
    return table;
}

// Array.BinarySearch(ulong[]) (ArraySortHelper.InternalBinarySearch)
inline int BinarySearch(const std::vector<uint64_t>& a, uint64_t value) {
    int num = 0;
    int num2 = static_cast<int>(a.size()) - 1;
    while (num <= num2) {
        const int num3 = num + ((num2 - num) >> 1);
        if (a[static_cast<size_t>(num3)] == value) return num3;
        if (a[static_cast<size_t>(num3)] < value) num = num3 + 1;
        else num2 = num3 - 1;
    }
    return ~num;
}

template <class E>
std::string NumberString(E v) {
    using U = std::underlying_type_t<E>;
    if constexpr (std::is_signed_v<U>) return FormatInt64(static_cast<int64_t>(static_cast<U>(v)));
    else return FormatUInt64(static_cast<uint64_t>(static_cast<U>(v)));
}

}  // namespace detail

// Enum.GetName / Type.GetEnumName: the name at the binary-search hit, or nullopt
template <class E>
std::optional<std::string> EnumGetName(E value) {
    const auto& t = detail::GetTable<E>();
    const int i = detail::BinarySearch(t.values, detail::EnumToUInt64(value));
    if (i >= 0) return t.names[static_cast<size_t>(i)];
    return std::nullopt;
}

// Enum.ToString() (Enum.cs:137 InternalFormat / 151 InternalFlagsFormat)
template <class E>
std::string EnumToString(E value) {
    if constexpr (!EnumTraits<E>::flags) {
        auto name = EnumGetName(value);
        return name ? *name : detail::NumberString(value);
    } else {
        uint64_t num = detail::EnumToUInt64(value);
        const auto& t = detail::GetTable<E>();
        int num2 = static_cast<int>(t.values.size()) - 1;
        std::string sb;
        bool flag = true;
        const uint64_t num3 = num;
        while (num2 >= 0 && (num2 != 0 || t.values[static_cast<size_t>(num2)] != 0)) {
            const uint64_t v = t.values[static_cast<size_t>(num2)];
            if ((num & v) == v) {
                num -= v;
                if (!flag) sb.insert(0, ", ");
                sb.insert(0, t.names[static_cast<size_t>(num2)]);
                flag = false;
            }
            --num2;
        }
        if (num != 0) return detail::NumberString(value);
        if (num3 == 0) {
            if (!t.values.empty() && t.values[0] == 0) return t.names[0];
            return "0";
        }
        return sb;
    }
}

// Enum.GetNames (sorted-by-value order)
template <class E>
std::vector<std::string> EnumGetNames() {
    return detail::GetTable<E>().names;
}

// Enum.Parse(typeof(E), name) for a single member name (case-sensitive; ArgumentException when unknown)
template <class E>
E EnumParse(std::string_view name) {
    for (const auto& m : EnumTraits<E>::members)
        if (name == m.name) return m.value;
    throw ArgumentException("Requested value '" + std::string(name) + "' was not found.");
}

// Type.GetMember(name) + DescriptionAttribute: the description of the member called `name`; nullopt when the member has
// no [Description]; std::out_of_range when no member has that name (GetMember(...)[0] on an empty array)
template <class E>
std::optional<std::string> EnumMemberDescription(std::string_view name) {
    for (const auto& m : EnumTraits<E>::members)
        if (name == m.name) return m.description ? std::optional<std::string>(m.description) : std::nullopt;
    throw std::out_of_range("Index was outside the bounds of the array.");
}

}  // namespace ksp::dotnet
