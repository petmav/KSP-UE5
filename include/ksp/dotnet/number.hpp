// .NET numeric parsing and formatting as executed by KSP (Unity 2019.4 Mono BCL, en-US culture with Mono's
// "Infinity"/"-Infinity"/"NaN" symbols; the invariant culture uses the same symbols).
//
// Parsing is a port of the managed ParseNumber state machine (decompiled/KSP-Decompiled/Mono/mscorlib/System/Number.cs)
// plus the runtime's native NumberBufferToDouble (CLR number.cpp NumberToDouble; Mono's number-ms.c is the same code)
// using the power-of-ten tables read from the shipped mono-2.0-bdwgc.dll. This is deliberately NOT correctly rounded:
// it reproduces the runtime's 64-bit-mantissa algorithm so that double values parsed from cfg/sfs files match KSP.
//
// Formatting ("G9"/"G17"/default "G") follows .NET Framework's number formatting (correctly rounded digits, 'G' layout:
// scientific when the decimal exponent is >= precision or < -4, "E+XX"/"E-XX" with at least two exponent digits,
// trailing zeros removed, negative zero printed as "0"). See the note in number.cpp about Mono's formatter.
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ksp::dotnet {

// System.Globalization.NumberStyles subset.
enum NumberStyles : unsigned {
    None = 0,
    AllowLeadingWhite = 0x1,
    AllowTrailingWhite = 0x2,
    AllowLeadingSign = 0x4,
    AllowTrailingSign = 0x8,
    AllowParentheses = 0x10,
    AllowDecimalPoint = 0x20,
    AllowThousands = 0x40,
    AllowExponent = 0x80,
    AllowCurrencySymbol = 0x100,
    Integer = AllowLeadingWhite | AllowTrailingWhite | AllowLeadingSign,
    Float = AllowLeadingWhite | AllowTrailingWhite | AllowLeadingSign | AllowDecimalPoint | AllowExponent,
    FloatAndThousands = Float | AllowThousands,  // default style of float.Parse / double.Parse / TryParse
};

class FormatException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
class OverflowException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// double.TryParse(s) / float.TryParse(s) (style Float|AllowThousands, en-US/invariant). On failure `result` is set to 0
// exactly like .NET's out parameter.
bool TryParseDouble(std::string_view s, double& result, unsigned styles = FloatAndThousands);
bool TryParseSingle(std::string_view s, float& result, unsigned styles = FloatAndThousands);
// double.Parse / float.Parse: throw FormatException / OverflowException like .NET.
double ParseDouble(std::string_view s, unsigned styles = FloatAndThousands);
float ParseSingle(std::string_view s, unsigned styles = FloatAndThousands);

// int/uint/long/ulong/byte/short TryParse (style Integer).
bool TryParseInt32(std::string_view s, int32_t& result, unsigned styles = Integer);
bool TryParseUInt32(std::string_view s, uint32_t& result, unsigned styles = Integer);
bool TryParseInt64(std::string_view s, int64_t& result, unsigned styles = Integer);
bool TryParseUInt64(std::string_view s, uint64_t& result, unsigned styles = Integer);
bool TryParseByte(std::string_view s, uint8_t& result, unsigned styles = Integer);
bool TryParseInt16(std::string_view s, int16_t& result, unsigned styles = Integer);
bool TryParseUInt16(std::string_view s, uint16_t& result, unsigned styles = Integer);
int32_t ParseInt32(std::string_view s);
uint8_t ParseByte(std::string_view s);

// bool.TryParse: "True"/"False" ignoring ASCII case, optionally surrounded by whitespace and NULs.
bool TryParseBool(std::string_view s, bool& result);

// The runtime's NumberBufferToDouble, exposed for tests: digits (no leading zeros), decimal exponent `scale` such that
// value = 0.d1d2d3... * 10^scale, and sign. Returns false on overflow to infinity.
bool NumberBufferToDouble(std::string_view digits, int scale, bool negative, double& value);

// ---- formatting ----
// value.ToString("G<precision>", CultureInfo.InvariantCulture). precision 0 = default ("G": 15 for double).
std::string FormatDoubleG(double value, int precision = 0);
// float.ToString("G<precision>"). precision 0 = default ("G": 7 for float).
std::string FormatSingleG(float value, int precision = 0);
// int.ToString(), etc.
std::string FormatInt64(int64_t v);
std::string FormatUInt64(uint64_t v);
inline std::string FormatBool(bool b) { return b ? "True" : "False"; }

}  // namespace ksp::dotnet
