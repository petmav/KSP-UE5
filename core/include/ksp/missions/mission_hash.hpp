// WP-54c: the System.*.GetHashCode helpers the GetHashCode() overrides of the Making History classes use (Mono corlib, the runtime of KSP 1.12.5):
//   int / uint / enum: the value; bool: 1 / 0; float: the IEEE bits, 0 for +-0.0; double: low ^ high word of the bits, 0 for +-0.0 (Single / Double.GetHashCode of the
//   .NET 4.x class library: `if (value == 0) return 0`); string: StringExtensions.GetHashCode_Net35 / Mono's String.GetHashCode (both h = h * 31 + c over the
//   UTF-16 code units with int wrap-around).
// The Unity Object / reference hash codes (UnityEngine.Object.GetHashCode() = the instance id, object.GetHashCode() of a plain class) are not reproducible values:
// MissionDataEnvironment::ObjectHash(ptr) is the seam (default: the address).
#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace ksp::Expansions::Missions::Hash {

inline int32_t Of(double d) {
    if (d == 0) return 0;
    int64_t v;
    std::memcpy(&v, &d, sizeof v);
    return static_cast<int32_t>(static_cast<uint32_t>(v & 0xFFFFFFFFLL)) ^ static_cast<int32_t>(v >> 32);
}
inline int32_t Of(float f) {
    if (f == 0) return 0;
    int32_t v;
    std::memcpy(&v, &f, sizeof v);
    return v;
}
inline int32_t Of(bool b) { return b ? 1 : 0; }
inline int32_t Of(int32_t v) { return v; }
inline int32_t Of(uint32_t v) { return static_cast<int32_t>(v); }
template <class E, class = std::enable_if_t<std::is_enum_v<E>>>
inline int32_t Of(E e) {
    return static_cast<int32_t>(e);
}

}  // namespace ksp::Expansions::Missions::Hash
