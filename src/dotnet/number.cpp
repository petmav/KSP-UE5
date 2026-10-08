#include "ksp/dotnet/number.hpp"

#include <charconv>
#include <cmath>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <string>

#include "ksp/dotnet/text.hpp"

namespace ksp::dotnet {

namespace {

// double.NaN / float.NaN in .NET are 0.0/0.0 on x86/x64: the "negative" quiet NaN (0xFFF8000000000000 / 0xFFC00000).
double DotNetNaN() {
    const uint64_t b = 0xFFF8000000000000ull;
    double d;
    std::memcpy(&d, &b, sizeof d);
    return d;
}
float DotNetNaNf() {
    const uint32_t b = 0xFFC00000u;
    float f;
    std::memcpy(&f, &b, sizeof f);
    return f;
}

// ---------------------------------------------------------------------------------------------------------------------
// Power-of-ten tables of the runtime's NumberToDouble. Read byte-for-byte from the shipped Mono runtime
// decompiled/KSP-dev/MonoBleedingEdge/EmbedRuntime/mono-2.0-bdwgc.dll (file offsets 4259328: rgval64Power10 + exponents,
// 4259584: rgval64Power10By16 + exponents). They are identical to the CLR's number.cpp tables.
// ---------------------------------------------------------------------------------------------------------------------
constexpr uint64_t kPow10[30] = {
    // 10^1 .. 10^15
    0xa000000000000000ull, 0xc800000000000000ull, 0xfa00000000000000ull, 0x9c40000000000000ull, 0xc350000000000000ull,
    0xf424000000000000ull, 0x9896800000000000ull, 0xbebc200000000000ull, 0xee6b280000000000ull, 0x9502f90000000000ull,
    0xba43b74000000000ull, 0xe8d4a51000000000ull, 0x9184e72a00000000ull, 0xb5e620f480000000ull, 0xe35fa931a0000000ull,
    // 10^-1 .. 10^-15
    0xcccccccccccccccdull, 0xa3d70a3d70a3d70bull, 0x83126e978d4fdf3cull, 0xd1b71758e219652eull, 0xa7c5ac471b478425ull,
    0x8637bd05af6c69b7ull, 0xd6bf94d5e57a42beull, 0xabcc77118461ceffull, 0x89705f4136b4a599ull, 0xdbe6fecebdedd5c2ull,
    0xafebff0bcb24ab02ull, 0x8cbccc096f5088cfull, 0xe12e13424bb40e18ull, 0xb424dc35095cd813ull, 0x901d7cf73ab0acdcull,
};
constexpr int8_t kPow10Exp[15] = {4, 7, 10, 14, 17, 20, 24, 27, 30, 34, 37, 40, 44, 47, 50};
constexpr uint64_t kPow10By16[42] = {
    // 10^16 .. 10^336
    0x8e1bc9bf04000000ull, 0x9dc5ada82b70b59eull, 0xaf298d050e4395d6ull, 0xc2781f49ffcfa6d4ull, 0xd7e77a8f87daf7faull,
    0xefb3ab16c59b14a0ull, 0x850fadc09923329cull, 0x93ba47c980e98cdeull, 0xa402b9c5a8d3a6e6ull, 0xb616a12b7fe617a8ull,
    0xca28a291859bbf90ull, 0xe070f78d39275566ull, 0xf92e0c3537826140ull, 0x8a5296ffe33cc92cull, 0x9991a6f3d6bf1762ull,
    0xaa7eebfb9df9de8aull, 0xbd49d14aa79dbc7eull, 0xd226fc195c6a2f88ull, 0xe950df20247c83f8ull, 0x81842f29f2cce373ull,
    0x8fcac257558ee4e2ull,
    // 10^-16 .. 10^-336
    0xe69594bec44de160ull, 0xcfb11ead453994c3ull, 0xbb127c53b17ec165ull, 0xa87fea27a539e9b3ull, 0x97c560ba6b0919b5ull,
    0x88b402f7fd7553abull, 0xf64335bcf065d3a0ull, 0xddd0467c64bce4c4ull, 0xc7caba6e7c5382edull, 0xb3f4e093db73a0b7ull,
    0xa21727db38cb0053ull, 0x91ff83775423cc29ull, 0x8380dea93da4bc82ull, 0xece53cec4a314f00ull, 0xd5605fcdcf32e217ull,
    0xc0314325637a1978ull, 0xad1c8eab5ee43ba2ull, 0x9becce62836ac5b0ull, 0x8c71dcd9ba0b495cull, 0xfd00b89747823938ull,
    0xe3e27a444d8d991aull,
};
constexpr int16_t kPow10By16Exp[21] = {54,  107, 160, 213, 266, 319, 373, 426, 479,  532, 585,
                                       638, 691, 745, 798, 851, 904, 957, 1010, 1064, 1117};

inline uint64_t Mul32x32To64(uint64_t a, uint64_t b) {
    return static_cast<uint64_t>(static_cast<uint32_t>(a)) * static_cast<uint64_t>(static_cast<uint32_t>(b));
}

inline uint64_t Mul64Lossy(uint64_t a, uint64_t b, int* pexp) {
    uint64_t val = Mul32x32To64(a >> 32, b >> 32) + (Mul32x32To64(a >> 32, b) >> 32) + (Mul32x32To64(a, b >> 32) >> 32);
    if ((val & 0x8000000000000000ull) == 0) {
        val <<= 1;
        *pexp -= 1;
    }
    return val;
}

inline uint32_t DigitsToInt(const char* p, int count) {
    uint32_t res = static_cast<uint32_t>(*p - '0');
    for (int i = 1; i < count; ++i) res = 10 * res + static_cast<uint32_t>(p[i] - '0');
    return res;
}

// CLR number.cpp NumberToDouble (== Mono number-ms.c mono_double_from_number).
uint64_t NumberToDoubleBits(const char* digits, int numberScale, bool sign) {
    uint64_t bits = 0;
    const char* src = digits;
    const int total = static_cast<int>(std::strlen(src));
    int remaining = total;
    while (*src == '0') {
        --remaining;
        ++src;
    }
    if (remaining == 0) {
        bits = 0;
    } else {
        int count = remaining < 9 ? remaining : 9;
        remaining -= count;
        uint64_t val = DigitsToInt(src, count);
        if (remaining > 0) {
            count = remaining < 9 ? remaining : 9;
            remaining -= count;
            const auto mult = static_cast<uint32_t>(kPow10[count - 1] >> (64 - kPow10Exp[count - 1]));
            val = Mul32x32To64(val, mult) + DigitsToInt(src + 9, count);
        }
        const int scale = numberScale - (total - remaining);
        const int absscale = scale < 0 ? -scale : scale;
        if (absscale >= 22 * 16) {
            bits = scale > 0 ? 0x7FF0000000000000ull : 0;
        } else {
            int exp = 64;
            if ((val & 0xFFFFFFFF00000000ull) == 0) { val <<= 32; exp -= 32; }
            if ((val & 0xFFFF000000000000ull) == 0) { val <<= 16; exp -= 16; }
            if ((val & 0xFF00000000000000ull) == 0) { val <<= 8; exp -= 8; }
            if ((val & 0xF000000000000000ull) == 0) { val <<= 4; exp -= 4; }
            if ((val & 0xC000000000000000ull) == 0) { val <<= 2; exp -= 2; }
            if ((val & 0x8000000000000000ull) == 0) { val <<= 1; exp -= 1; }

            int index = absscale & 15;
            if (index) {
                const int multexp = kPow10Exp[index - 1];
                exp += (scale < 0) ? (-multexp + 1) : multexp;
                const uint64_t multval = kPow10[index + ((scale < 0) ? 15 : 0) - 1];
                val = Mul64Lossy(val, multval, &exp);
            }
            index = absscale >> 4;
            if (index) {
                const int multexp = kPow10By16Exp[index - 1];
                exp += (scale < 0) ? (-multexp + 1) : multexp;
                const uint64_t multval = kPow10By16[index + ((scale < 0) ? 21 : 0) - 1];
                val = Mul64Lossy(val, multval, &exp);
            }
            // round & scale down
            if (static_cast<uint32_t>(val) & (1u << 10)) {
                uint64_t tmp = val + ((1u << 10) - 1) + ((static_cast<uint32_t>(val) >> 11) & 1);
                if (tmp < val) {
                    tmp = (tmp >> 1) | 0x8000000000000000ull;
                    exp += 1;
                }
                val = tmp;
            }
            exp += 0x3FE;
            if (exp <= 0) {
                if (exp == -52 && val >= 0x8000000000000058ull) val = 1;
                else if (exp <= -52) val = 0;
                else val >>= (-exp + 12);
            } else if (exp >= 0x7FF) {
                val = 0x7FF0000000000000ull;
            } else {
                val = (static_cast<uint64_t>(exp) << 52) + ((val >> 11) & 0x000FFFFFFFFFFFFFull);
            }
            bits = val;
        }
    }
    if (sign) bits |= 0x8000000000000000ull;
    return bits;
}

// ---------------------------------------------------------------------------------------------------------------------
// ParseNumber (Mono mscorlib System/Number.cs ParseNumber, i.e. the .NET reference-source state machine).
// en-US / invariant NumberFormatInfo: PositiveSign "+", NegativeSign "-", decimal ".", group ",", currency "$",
// NumberNegativePattern 1.
// ---------------------------------------------------------------------------------------------------------------------
constexpr int kNumberMaxDigits = 50;
constexpr int StateSign = 0x1, StateParens = 0x2, StateDigits = 0x4, StateNonZero = 0x8, StateDecimal = 0x10,
              StateCurrency = 0x20;

struct NumberBuffer {
    int precision = 0;
    int scale = 0;
    bool sign = false;
    char digits[kNumberMaxDigits + 1] = {};
};

struct Cursor {
    std::string_view s;
    size_t p = 0;
    char at(size_t i) const { return i < s.size() ? s[i] : '\0'; }
    char ch() const { return at(p); }
};

inline bool IsWhite(char ch) { return ch == ' ' || (ch >= '\t' && ch <= '\r'); }
inline bool IsDigit(char ch) { return ch >= '0' && ch <= '9'; }

// MatchChars: returns position after `str` if it matches at p, else npos. (U+00A0 in the pattern also matches ' ';
// never the case for en-US.)
inline size_t MatchChars(const Cursor& c, size_t p, std::string_view str) {
    if (str.empty()) return std::string_view::npos;
    for (size_t k = 0; k < str.size(); ++k)
        if (c.at(p + k) != str[k]) return std::string_view::npos;
    return p + str.size();
}

bool ParseNumber(Cursor& c, unsigned options, NumberBuffer& number, bool parseDecimal) {
    constexpr size_t npos = std::string_view::npos;
    number.scale = 0;
    number.sign = false;
    std::string_view currSymbol;  // empty == null
    std::string_view decSep = ".", groupSep = ",", numDecSep, numGroupSep;
    bool parsingCurrency = false;
    if (options & AllowCurrencySymbol) {
        currSymbol = "$";
        numDecSep = ".";
        numGroupSep = ",";
        parsingCurrency = true;
    }
    int state = 0;
    bool signflag = false;
    size_t p = c.p;
    char ch = c.at(p);
    size_t next;
    for (;;) {
        if (!IsWhite(ch) || (options & AllowLeadingWhite) == 0 ||
            ((state & StateSign) != 0 && (state & StateCurrency) == 0 /* && NumberNegativePattern(1) != 2 */)) {
            if ((signflag = ((options & AllowLeadingSign) != 0 && (state & StateSign) == 0)) &&
                (next = MatchChars(c, p, "+")) != npos) {
                state |= StateSign;
                p = next - 1;
            } else if (signflag && (next = MatchChars(c, p, "-")) != npos) {
                state |= StateSign;
                number.sign = true;
                p = next - 1;
            } else if (ch == '(' && (options & AllowParentheses) != 0 && (state & StateSign) == 0) {
                state |= StateSign | StateParens;
                number.sign = true;
            } else if (!currSymbol.empty() && (next = MatchChars(c, p, currSymbol)) != npos) {
                state |= StateCurrency;
                currSymbol = {};
                p = next - 1;
            } else {
                break;
            }
        }
        ch = c.at(++p);
    }
    int digCount = 0;
    int digEnd = 0;
    for (;;) {
        if (IsDigit(ch)) {
            state |= StateDigits;
            if (ch != '0' || (state & StateNonZero) != 0) {
                if (digCount < kNumberMaxDigits) {
                    number.digits[digCount++] = ch;
                    if (ch != '0' || parseDecimal) digEnd = digCount;
                }
                if ((state & StateDecimal) == 0) number.scale++;
                state |= StateNonZero;
            } else if ((state & StateDecimal) != 0) {
                number.scale--;
            }
        } else if ((options & AllowDecimalPoint) != 0 && (state & StateDecimal) == 0 &&
                   ((next = MatchChars(c, p, decSep)) != npos ||
                    (parsingCurrency && (state & StateCurrency) == 0 && (next = MatchChars(c, p, numDecSep)) != npos))) {
            state |= StateDecimal;
            p = next - 1;
        } else if ((options & AllowThousands) != 0 && (state & StateDigits) != 0 && (state & StateDecimal) == 0 &&
                   ((next = MatchChars(c, p, groupSep)) != npos ||
                    (parsingCurrency && (state & StateCurrency) == 0 && (next = MatchChars(c, p, numGroupSep)) != npos))) {
            p = next - 1;
        } else {
            break;
        }
        ch = c.at(++p);
    }
    bool negExp = false;
    number.precision = digEnd;
    number.digits[digEnd] = '\0';
    if ((state & StateDigits) != 0) {
        if ((ch == 'E' || ch == 'e') && (options & AllowExponent) != 0) {
            const size_t temp = p;
            ch = c.at(++p);
            if ((next = MatchChars(c, p, "+")) != npos) {
                ch = c.at(p = next);
            } else if ((next = MatchChars(c, p, "-")) != npos) {
                ch = c.at(p = next);
                negExp = true;
            }
            if (IsDigit(ch)) {
                int exp = 0;
                do {
                    exp = exp * 10 + (ch - '0');
                    ch = c.at(++p);
                    if (exp > 1000) {
                        exp = 9999;
                        while (IsDigit(ch)) ch = c.at(++p);
                    }
                } while (IsDigit(ch));
                if (negExp) exp = -exp;
                number.scale += exp;
            } else {
                p = temp;
                ch = c.at(p);
            }
        }
        for (;;) {
            if (!IsWhite(ch) || (options & AllowTrailingWhite) == 0) {
                if ((signflag = ((options & AllowTrailingSign) != 0 && (state & StateSign) == 0)) &&
                    (next = MatchChars(c, p, "+")) != npos) {
                    state |= StateSign;
                    p = next - 1;
                } else if (signflag && (next = MatchChars(c, p, "-")) != npos) {
                    state |= StateSign;
                    number.sign = true;
                    p = next - 1;
                } else if (ch == ')' && (state & StateParens) != 0) {
                    state &= ~StateParens;
                } else if (!currSymbol.empty() && (next = MatchChars(c, p, currSymbol)) != npos) {
                    currSymbol = {};
                    p = next - 1;
                } else {
                    break;
                }
            }
            ch = c.at(++p);
        }
        if ((state & StateParens) == 0) {
            if ((state & StateNonZero) == 0) {
                if (!parseDecimal) number.scale = 0;
                if ((state & StateDecimal) == 0) number.sign = false;
            }
            c.p = p;
            return true;
        }
    }
    c.p = p;
    return false;
}

// TryStringToNumber: parse, then everything after the number must be NUL characters.
bool TryStringToNumber(std::string_view s, unsigned options, NumberBuffer& number, bool parseDecimal) {
    Cursor c{s, 0};
    if (!ParseNumber(c, options, number, parseDecimal)) return false;
    for (size_t i = c.p; i < s.size(); ++i)
        if (s[i] != '\0') return false;
    return true;
}

// Number.NumberBufferToDouble icall: NumberToDouble then reject infinity; a zero result loses its sign.
bool BufferToDouble(const NumberBuffer& number, double& value) {
    const uint64_t bits = NumberToDoubleBits(number.digits, number.scale, number.sign);
    const unsigned e = static_cast<unsigned>((bits >> 52) & 0x7FF);
    if (e == 0x7FF) return false;
    double d;
    std::memcpy(&d, &bits, sizeof d);
    if (e == 0 && (bits & 0x000FFFFFFFFFFFFFull) == 0) d = 0.0;
    value = d;
    return true;
}

bool NumberToInt32(const NumberBuffer& number, int32_t& value) {
    int i = number.scale;
    if (i > 10 || i < number.precision) return false;
    const char* p = number.digits;
    uint32_t n = 0;
    while (--i >= 0) {
        if (n > 214748364u) return false;
        n *= 10;
        if (*p != '\0') n += static_cast<uint32_t>(*p++ - '0');
    }
    int32_t sn = static_cast<int32_t>(n);
    if (number.sign) {
        sn = static_cast<int32_t>(0u - n);
        if (sn > 0) return false;
    } else if (sn < 0) {
        return false;
    }
    value = sn;
    return true;
}

bool NumberToInt64(const NumberBuffer& number, int64_t& value) {
    int i = number.scale;
    if (i > 19 || i < number.precision) return false;
    const char* p = number.digits;
    uint64_t n = 0;
    while (--i >= 0) {
        if (n > 922337203685477580ull) return false;
        n *= 10;
        if (*p != '\0') n += static_cast<uint64_t>(*p++ - '0');
    }
    int64_t sn = static_cast<int64_t>(n);
    if (number.sign) {
        sn = static_cast<int64_t>(0ull - n);
        if (sn > 0) return false;
    } else if (sn < 0) {
        return false;
    }
    value = sn;
    return true;
}

bool NumberToUInt32(const NumberBuffer& number, uint32_t& value) {
    int i = number.scale;
    if (i > 10 || i < number.precision || number.sign) return false;
    const char* p = number.digits;
    uint32_t n = 0;
    while (--i >= 0) {
        if (n > 429496729u) return false;
        n *= 10;
        if (*p != '\0') {
            const uint32_t n2 = n + static_cast<uint32_t>(*p++ - '0');
            if (n2 < n) return false;
            n = n2;
        }
    }
    value = n;
    return true;
}

bool NumberToUInt64(const NumberBuffer& number, uint64_t& value) {
    int i = number.scale;
    if (i > 20 || i < number.precision || number.sign) return false;
    const char* p = number.digits;
    uint64_t n = 0;
    while (--i >= 0) {
        if (n > 1844674407370955161ull) return false;
        n *= 10;
        if (*p != '\0') {
            const uint64_t n2 = n + static_cast<uint64_t>(*p++ - '0');
            if (n2 < n) return false;
            n = n2;
        }
    }
    value = n;
    return true;
}

}  // namespace

bool NumberBufferToDouble(std::string_view digits, int scale, bool negative, double& value) {
    NumberBuffer nb;
    const size_t n = digits.size() < kNumberMaxDigits ? digits.size() : kNumberMaxDigits;
    std::memcpy(nb.digits, digits.data(), n);
    nb.digits[n] = '\0';
    nb.scale = scale;
    nb.sign = negative;
    return BufferToDouble(nb, value);
}

// Mono mscorlib System/Double.cs TryParse(string, NumberStyles, NumberFormatInfo, out double)
bool TryParseDouble(std::string_view s, double& result, unsigned styles) {
    NumberBuffer nb;
    result = 0.0;
    if (TryStringToNumber(s, styles, nb, false) && BufferToDouble(nb, result)) return true;
    result = 0.0;
    const std::string_view t = TrimView(s);
    if (t == "Infinity") { result = std::numeric_limits<double>::infinity(); return true; }
    if (t == "-Infinity") { result = -std::numeric_limits<double>::infinity(); return true; }
    if (t == "NaN") { result = DotNetNaN(); return true; }
    return false;
}

// Mono mscorlib System/Single.cs TryParse + Number.TryParseSingle (double conversion, reject float overflow)
bool TryParseSingle(std::string_view s, float& result, unsigned styles) {
    NumberBuffer nb;
    result = 0.0f;
    double d = 0.0;
    if (TryStringToNumber(s, styles, nb, false) && BufferToDouble(nb, d)) {
        const float f = static_cast<float>(d);
        if (f != std::numeric_limits<float>::infinity() && f != -std::numeric_limits<float>::infinity()) {
            result = f;
            return true;
        }
    }
    result = 0.0f;
    const std::string_view t = TrimView(s);
    if (t == "Infinity") { result = std::numeric_limits<float>::infinity(); return true; }
    if (t == "-Infinity") { result = -std::numeric_limits<float>::infinity(); return true; }
    if (t == "NaN") { result = DotNetNaNf(); return true; }
    return false;
}

// Mono mscorlib System/Number.cs ParseDouble
double ParseDouble(std::string_view s, unsigned styles) {
    NumberBuffer nb;
    if (!TryStringToNumber(s, styles, nb, false)) {
        const std::string_view t = TrimView(s);
        if (t == "Infinity") return std::numeric_limits<double>::infinity();
        if (t == "-Infinity") return -std::numeric_limits<double>::infinity();
        if (t == "NaN") return DotNetNaN();
        throw FormatException("Input string was not in a correct format.");
    }
    double d = 0.0;
    if (!BufferToDouble(nb, d)) throw OverflowException("Value was either too large or too small for a Double.");
    return d;
}

// Mono mscorlib System/Number.cs ParseSingle
float ParseSingle(std::string_view s, unsigned styles) {
    NumberBuffer nb;
    if (!TryStringToNumber(s, styles, nb, false)) {
        const std::string_view t = TrimView(s);
        if (t == "Infinity") return std::numeric_limits<float>::infinity();
        if (t == "-Infinity") return -std::numeric_limits<float>::infinity();
        if (t == "NaN") return DotNetNaNf();
        throw FormatException("Input string was not in a correct format.");
    }
    double d = 0.0;
    if (!BufferToDouble(nb, d)) throw OverflowException("Value was either too large or too small for a Single.");
    const float f = static_cast<float>(d);
    if (f == std::numeric_limits<float>::infinity() || f == -std::numeric_limits<float>::infinity())
        throw OverflowException("Value was either too large or too small for a Single.");
    return f;
}

bool TryParseInt32(std::string_view s, int32_t& result, unsigned styles) {
    NumberBuffer nb;
    result = 0;
    int32_t v = 0;
    if (!TryStringToNumber(s, styles, nb, false) || !NumberToInt32(nb, v)) return false;
    result = v;
    return true;
}

bool TryParseUInt32(std::string_view s, uint32_t& result, unsigned styles) {
    NumberBuffer nb;
    result = 0;
    uint32_t v = 0;
    if (!TryStringToNumber(s, styles, nb, false) || !NumberToUInt32(nb, v)) return false;
    result = v;
    return true;
}

bool TryParseInt64(std::string_view s, int64_t& result, unsigned styles) {
    NumberBuffer nb;
    result = 0;
    int64_t v = 0;
    if (!TryStringToNumber(s, styles, nb, false) || !NumberToInt64(nb, v)) return false;
    result = v;
    return true;
}

bool TryParseUInt64(std::string_view s, uint64_t& result, unsigned styles) {
    NumberBuffer nb;
    result = 0;
    uint64_t v = 0;
    if (!TryStringToNumber(s, styles, nb, false) || !NumberToUInt64(nb, v)) return false;
    result = v;
    return true;
}

// Byte.TryParse: Int32 parse then range check [0, 255].
bool TryParseByte(std::string_view s, uint8_t& result, unsigned styles) {
    result = 0;
    int32_t v = 0;
    if (!TryParseInt32(s, v, styles) || v < 0 || v > 255) return false;
    result = static_cast<uint8_t>(v);
    return true;
}

bool TryParseInt16(std::string_view s, int16_t& result, unsigned styles) {
    result = 0;
    int32_t v = 0;
    if (!TryParseInt32(s, v, styles) || v < -32768 || v > 32767) return false;
    result = static_cast<int16_t>(v);
    return true;
}

bool TryParseUInt16(std::string_view s, uint16_t& result, unsigned styles) {
    result = 0;
    uint32_t v = 0;
    if (!TryParseUInt32(s, v, styles) || v > 65535) return false;
    result = static_cast<uint16_t>(v);
    return true;
}

int32_t ParseInt32(std::string_view s) {
    NumberBuffer nb;
    if (!TryStringToNumber(s, Integer, nb, false)) throw FormatException("Input string was not in a correct format.");
    int32_t v = 0;
    if (!NumberToInt32(nb, v)) throw OverflowException("Value was either too large or too small for an Int32.");
    return v;
}

uint8_t ParseByte(std::string_view s) {
    const int32_t v = ParseInt32(s);
    if (v < 0 || v > 255) throw OverflowException("Value was either too large or too small for an unsigned byte.");
    return static_cast<uint8_t>(v);
}

// Mono mscorlib System/Boolean.cs TryParse
bool TryParseBool(std::string_view s, bool& result) {
    result = false;
    if (EqualsOrdinalIgnoreCase(s, "True")) { result = true; return true; }
    if (EqualsOrdinalIgnoreCase(s, "False")) { result = false; return true; }
    // TrimWhiteSpaceAndNull
    std::string_view t = s;
    for (;;) {
        const std::string_view before = t;
        while (!t.empty() && t.front() == '\0') t.remove_prefix(1);
        while (!t.empty() && t.back() == '\0') t.remove_suffix(1);
        t = TrimView(t);
        if (t.size() == before.size()) break;
    }
    if (EqualsOrdinalIgnoreCase(t, "True")) { result = true; return true; }
    if (EqualsOrdinalIgnoreCase(t, "False")) { result = false; return true; }
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------
// Formatting. .NET Framework semantics (COMNumber::FormatDouble/FormatSingle -> DoubleToNumber (_ecvt: correctly
// rounded) -> RoundNumber -> FormatGeneral). NOTE: Unity's Mono uses the managed System/NumberFormatter.cs, whose
// double path builds 17 digits from a 64-bit approximation (MantissaBitsTable); its last digit can differ from the
// correctly rounded one. Which one KSP actually writes is CAPTURE-SPEC C7 territory; the harness goldens were produced
// on .NET Framework, so that is what is implemented here.
// ---------------------------------------------------------------------------------------------------------------------
namespace {

struct DecDigits {
    char dig[40] = {};  // NUL-terminated significant digits
    int scale = 0;      // value = 0.d1d2... * 10^scale
    bool sign = false;
};

// DoubleToNumber(value, precision): `precision` significant digits of the exact binary value, rounded half AWAY from
// zero on exact decimal ties (the CRT _ecvt used by the CLR does this; e.g. 150592385628536.5 -> "150592385628537" at
// 15 digits). std::to_chars rounds ties to even, so a tie is detected (next digit '5' in a (precision+1)-digit
// rendering) and resolved from the exact decimal expansion.
DecDigits DoubleToNumber(double value, int precision) {
    DecDigits n;
    n.sign = std::signbit(value);
    if (value == 0.0) {
        n.scale = 0;
        return n;
    }
    const double a = value < 0 ? -value : value;
    char buf[1200];
    // (precision + 1) significant digits, to see the first discarded digit
    auto r = std::to_chars(buf, buf + sizeof buf, a, std::chars_format::scientific, precision);
    *r.ptr = '\0';
    auto digitsOf = [](const char* s, std::string& digs, int& e10) {
        digs.clear();
        const char* q = s;
        for (; *q && *q != 'e'; ++q)
            if (*q != '.') digs.push_back(*q);
        e10 = std::atoi(q + 1);
    };
    std::string digs;
    int e10 = 0;
    digitsOf(buf, digs, e10);
    if (digs.back() == '5') {
        // possible exact tie: use the exact expansion (a double has at most 767 significant digits)
        r = std::to_chars(buf, buf + sizeof buf, a, std::chars_format::scientific, 1100);
        *r.ptr = '\0';
        digitsOf(buf, digs, e10);
    }
    // round the digit string to `precision` digits, half away from zero (ties included)
    std::string kept = digs.substr(0, static_cast<size_t>(precision));
    if (digs.size() > static_cast<size_t>(precision) && digs[static_cast<size_t>(precision)] >= '5') {
        int i = precision - 1;
        while (i >= 0 && kept[static_cast<size_t>(i)] == '9') kept[static_cast<size_t>(i--)] = '0';
        if (i >= 0) kept[static_cast<size_t>(i)]++;
        else {
            kept.insert(kept.begin(), '1');
            kept.pop_back();
            ++e10;
        }
    }
    int k = static_cast<int>(kept.size());
    while (k > 0 && kept[static_cast<size_t>(k - 1)] == '0') --k;
    std::memcpy(n.dig, kept.data(), static_cast<size_t>(k));
    n.dig[k] = '\0';
    n.scale = e10 + 1;
    return n;
}

// Number.RoundNumber
void RoundNumber(DecDigits& number, int pos) {
    char* dig = number.dig;
    int i = 0;
    while (i < pos && dig[i] != '\0') ++i;
    if (i == pos && dig[i] >= '5') {
        while (i > 0 && dig[i - 1] == '9') --i;
        if (i > 0) {
            dig[i - 1]++;
        } else {
            number.scale++;
            dig[0] = '1';
            i = 1;
        }
    } else {
        while (i > 0 && dig[i - 1] == '0') --i;
    }
    if (i == 0) {
        number.scale = 0;
        number.sign = false;
    }
    dig[i] = '\0';
}

// Number.FormatGeneral + FormatExponent (expChar 'E', minDigits 2, positive sign shown)
std::string FormatGeneral(const DecDigits& number, int nMaxDigits) {
    std::string sb;
    if (number.sign) sb.push_back('-');
    int digPos = number.scale;
    bool scientific = false;
    if (digPos > nMaxDigits || digPos < -3) {
        digPos = 1;
        scientific = true;
    }
    const char* dig = number.dig;
    if (digPos > 0) {
        do {
            sb.push_back((*dig != '\0') ? *dig++ : '0');
        } while (--digPos > 0);
    } else {
        sb.push_back('0');
    }
    if (*dig != '\0' || digPos < 0) {
        sb.push_back('.');
        while (digPos < 0) {
            sb.push_back('0');
            ++digPos;
        }
        while (*dig != '\0') sb.push_back(*dig++);
    }
    if (scientific) {
        int e = number.scale - 1;
        sb.push_back('E');
        if (e < 0) {
            sb.push_back('-');
            e = -e;
        } else {
            sb.push_back('+');
        }
        char eb[16];
        const auto r = std::to_chars(eb, eb + sizeof eb, e);
        const int len = static_cast<int>(r.ptr - eb);
        for (int z = len; z < 2; ++z) sb.push_back('0');
        sb.append(eb, r.ptr);
    }
    return sb;
}

}  // namespace

std::string FormatDoubleG(double value, int precision) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value > 0 ? "Infinity" : "-Infinity";
    int digits = precision;
    int prec = 15;
    if (digits > 15) prec = 17;
    DecDigits n = DoubleToNumber(value, prec);
    int nMax = digits;
    if (nMax < 1) nMax = prec;  // default 'G' pads to the number's precision
    RoundNumber(n, nMax);
    return FormatGeneral(n, nMax);
}

std::string FormatSingleG(float value, int precision) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value > 0 ? "Infinity" : "-Infinity";
    int digits = precision;
    int prec = 7;
    if (digits > 7) prec = 9;
    DecDigits n = DoubleToNumber(static_cast<double>(value), prec);
    int nMax = digits;
    if (nMax < 1) nMax = prec;
    RoundNumber(n, nMax);
    return FormatGeneral(n, nMax);
}

std::string FormatInt64(int64_t v) {
    char b[32];
    const auto r = std::to_chars(b, b + sizeof b, v);
    return std::string(b, r.ptr);
}

std::string FormatUInt64(uint64_t v) {
    char b[32];
    const auto r = std::to_chars(b, b + sizeof b, v);
    return std::string(b, r.ptr);
}

}  // namespace ksp::dotnet
