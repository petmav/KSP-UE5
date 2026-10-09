// Runtime member tables of the C# enums used by the WP-05 data model (Part, AvailablePart, PartResource, AttachNode
// fields), generated from the decompiled declarations into part_enums.inc by tools/wp05/gen_part_fields.py.
//
// Enum fields are stored as int32_t plus a table (the part-field reflection table is generic over many enum types);
// EnumTable::Parse / ToString give System.Enum.Parse / Enum.ToString semantics (Mono mscorlib System/Enum.cs):
// - Parse: the string is trimmed; a leading digit, '-' or '+' parses it as a number (any value, defined or not);
//   otherwise it is a ','-separated list of member names (each trimmed, case-sensitive unless ignoreCase), OR-ed
//   together; an unknown name throws ArgumentException("Requested value '<name>' was not found."); empty or
//   whitespace-only input throws ArgumentException("Must specify valid information for parsing in the string.").
// - ToString: the declared name of the value (the first declared one for duplicates is NOT guaranteed by .NET: the
//   runtime binary-searches the value-sorted table; kspcore picks the same member as that search), for [Flags]
//   enums the "A, B" composition, otherwise the decimal number.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ksp {

struct EnumTable {
    const char* csName;
    bool flags;
    const char* file;
    int line;
    std::vector<std::pair<std::string, int64_t>> members;  // declaration order

    int64_t Parse(std::string_view value, bool ignoreCase = false) const;
    bool TryParse(std::string_view value, int64_t& result, bool ignoreCase = false) const;
    std::string ToString(int64_t value) const;
    const char* NameOf(int64_t value) const;  // nullptr when undefined
};

#define KSP_ENUM_BEGIN(cpp, cs, isFlags, file, line) extern const EnumTable kEnum_##cpp;
#define KSP_ENUM_MEMBER(cpp, name, value)
#define KSP_ENUM_END(cpp)
#include "ksp/parts/part_enums.inc"
#undef KSP_ENUM_BEGIN
#undef KSP_ENUM_MEMBER
#undef KSP_ENUM_END

// Typed mirrors of the enums the C++ code branches on (values as declared in C#).
enum class AttachNodeMethod : int32_t { FIXED_JOINT = 0, HINGE_JOINT = 1, LOCKED_JOINT = 2, MERGED_PHYSICS = 3, NO_PHYSICS = 4, NONE = 5 };
enum class AttachNodeType : int32_t { Stack = 0, Surface = 1, Dock = 2 };  // AttachNode.NodeType
enum class PartDragModel : int32_t { DEFAULT = 0, CONIC = 1, CYLINDRICAL = 2, SPHERICAL = 3, CUBE = 4, NONE = 5 };
enum class PartResourceFlowMode : int32_t { None = 0, Out = 1, In = 2, Both = 3 };  // PartResource.FlowMode

}  // namespace ksp
