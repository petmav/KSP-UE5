#include "ksp/core/delegate.hpp"

#include <cstdlib>
#include <string_view>

#if !defined(_MSC_VER)
#include <cxxabi.h>
#endif

namespace ksp::detail {

std::string CsTypeName(const std::type_info& ti) {
#if defined(_MSC_VER)
    std::string_view n = ti.name();
    for (std::string_view prefix : {"class ", "struct ", "enum ", "union "})
        if (n.substr(0, prefix.size()) == prefix) {
            n.remove_prefix(prefix.size());
            break;
        }
    std::string full(n);
#else
    int status = 0;
    char* dem = abi::__cxa_demangle(ti.name(), nullptr, nullptr, &status);
    std::string full = (status == 0 && dem) ? dem : ti.name();
    std::free(dem);
#endif
    // drop namespace / enclosing-class qualifiers outside template brackets (C# Type.Name has neither)
    int depth = 0;
    size_t start = 0;
    for (size_t i = 0; i < full.size(); ++i) {
        const char c = full[i];
        if (c == '<') ++depth;
        else if (c == '>') --depth;
        else if (depth == 0 && c == ':' && i + 1 < full.size() && full[i + 1] == ':') start = i + 2;
    }
    return full.substr(start);
}

}  // namespace ksp::detail
