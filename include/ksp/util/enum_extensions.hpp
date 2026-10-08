// EnumExtensions.cs: Description / displayDescription / GetDescriptions / ToStringCached over ksp::dotnet enum metadata
// (an enum used here needs a ksp::dotnet::EnumTraits specialisation, see ksp/dotnet/enum.hpp).
#pragma once

#include <string>
#include <type_traits>
#include <vector>

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/localization/localizer.hpp"

namespace ksp::util {

// EnumExtensions.cs:46 Description(): the [Description] of the member named e.ToString(), else e.ToString().
// An undefined value or a [Flags] combination has no member of that name: IndexOutOfRangeException (std::out_of_range).
template <class E>
std::string Description(E e) {
    const std::string s = dotnet::EnumToString(e);
    auto d = dotnet::EnumMemberDescription<E>(s);
    return d ? *d : s;
}

// EnumExtensions.cs:56
template <class E>
std::string displayDescription(E e) {
    return Localizer::Format(Description(e));
}

// EnumExtensions.cs:61: Description of Enum.Parse(name) for every name of Enum.GetNames (sorted-by-value order; for
// duplicate values the description follows the name ToString() picks)
template <class E>
std::vector<std::string> GetDescriptions(E /*value*/) {
    std::vector<std::string> list;
    for (const std::string& n : dotnet::EnumGetNames<E>()) list.push_back(Description(dotnet::EnumParse<E>(n)));
    return list;
}

// EnumExtensions.cs:72 ToStringCached(): (int)(object)value -- InvalidCastException unless the underlying type is int;
// otherwise identical to ToString() (the cache only memoises it).
template <class E>
std::string ToStringCached(E e) {
    if constexpr (!std::is_same_v<std::underlying_type_t<E>, int>) {
        throw InvalidCastException("Specified cast is not valid.");
    } else {
        return dotnet::EnumToString(e);
    }
}

}  // namespace ksp::util
