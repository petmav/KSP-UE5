#include "ksp/parts/ksp_enums.hpp"

#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"

namespace ksp {

#define KSP_ENUM_BEGIN(cpp, cs, isFlags, file, line) const EnumTable kEnum_##cpp{cs, isFlags, file, line, {
#define KSP_ENUM_MEMBER(cpp, name, value) {name, value},
#define KSP_ENUM_END(cpp) }};
#include "ksp/parts/part_enums.inc"
#undef KSP_ENUM_BEGIN
#undef KSP_ENUM_MEMBER
#undef KSP_ENUM_END

namespace {

bool EqualsName(std::string_view a, std::string_view b, bool ignoreCase) {
    return ignoreCase ? dotnet::EqualsOrdinalIgnoreCase(a, b) : a == b;
}

}  // namespace

// Mono mscorlib System/Enum.cs Parse(Type, string, bool) -> TryParseEnum
bool EnumTable::TryParse(std::string_view value, int64_t& result, bool ignoreCase) const {
    result = 0;
    const std::string v = dotnet::Trim(value);
    if (v.empty()) return false;
    const char c0 = v[0];
    if ((c0 >= '0' && c0 <= '9') || c0 == '-' || c0 == '+') {
        int64_t n = 0;
        if (!dotnet::TryParseInt64(v, n)) return false;
        result = n;
        return true;
    }
    int64_t acc = 0;
    for (const std::string& part : dotnet::Split(v, ",", false)) {
        const std::string name = dotnet::Trim(part);
        bool found = false;
        for (const auto& [n, val] : members)
            if (EqualsName(n, name, ignoreCase)) {
                acc |= val;
                found = true;
                break;
            }
        if (!found) return false;
    }
    result = acc;
    return true;
}

int64_t EnumTable::Parse(std::string_view value, bool ignoreCase) const {
    int64_t r = 0;
    if (dotnet::Trim(value).empty())
        throw ArgumentException("Must specify valid information for parsing in the string.");
    if (!TryParse(value, r, ignoreCase)) {
        // the first undefined name is reported
        std::string bad = dotnet::Trim(value);
        for (const std::string& part : dotnet::Split(bad, ",", false)) {
            const std::string name = dotnet::Trim(part);
            bool found = false;
            for (const auto& m : members)
                if (EqualsName(m.first, name, ignoreCase)) found = true;
            if (!found) {
                bad = name;
                break;
            }
        }
        throw ArgumentException("Requested value '" + bad + "' was not found.");
    }
    return r;
}

const char* EnumTable::NameOf(int64_t value) const {
    // The runtime binary-searches the value-sorted name table; with distinct values that is the declared member.
    std::vector<std::pair<int64_t, size_t>> sorted;
    for (size_t i = 0; i < members.size(); i++) sorted.push_back({members[i].second, i});
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const auto& a, const auto& b) { return static_cast<uint64_t>(a.first) < static_cast<uint64_t>(b.first); });
    size_t lo = 0, hi = sorted.size();
    while (lo < hi) {
        const size_t mid = lo + (hi - lo) / 2;
        const uint64_t mv = static_cast<uint64_t>(sorted[mid].first), v = static_cast<uint64_t>(value);
        if (mv == v) return members[sorted[mid].second].first.c_str();
        if (mv < v) lo = mid + 1; else hi = mid;
    }
    return nullptr;
}

std::string EnumTable::ToString(int64_t value) const {
    if (const char* n = NameOf(value)) return n;
    if (flags) {
        // Enum.InternalFlagsFormat: from the highest value down
        std::vector<std::pair<std::string, int64_t>> m = members;
        std::stable_sort(m.begin(), m.end(), [](const auto& a, const auto& b) {
            return static_cast<uint64_t>(a.second) < static_cast<uint64_t>(b.second);
        });
        uint64_t rest = static_cast<uint64_t>(value);
        std::string out;
        for (size_t i = m.size(); i-- > 0;) {
            const uint64_t mv = static_cast<uint64_t>(m[i].second);
            if (i == 0 && mv == 0) break;
            if ((rest & mv) == mv && mv != 0) {
                rest -= mv;
                out = out.empty() ? m[i].first : m[i].first + ", " + out;
            }
        }
        if (rest == 0 && !out.empty()) return out;
    }
    return dotnet::FormatInt64(value);
}

}  // namespace ksp
