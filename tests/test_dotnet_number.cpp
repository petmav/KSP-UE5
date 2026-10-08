// .NET numeric parsing/formatting semantics (Mono/CLR) used by KSP's cfg handling.
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "doctest/doctest.h"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"

using namespace ksp::dotnet;

static uint64_t Bits(double d) {
    uint64_t b;
    std::memcpy(&b, &d, 8);
    return b;
}

TEST_CASE("double.TryParse accepts .NET Float|AllowThousands syntax") {
    double d = -1;
    CHECK(TryParseDouble("1,000", d));
    CHECK(d == 1000.0);
    CHECK(TryParseDouble(" -1.5e3 ", d));
    CHECK(d == -1500.0);
    CHECK(TryParseDouble(".5", d));
    CHECK(d == 0.5);
    CHECK(TryParseDouble("5.", d));
    CHECK(d == 5.0);
    CHECK(TryParseDouble("+7", d));
    CHECK(d == 7.0);
    CHECK_FALSE(TryParseDouble("1e", d));
    CHECK_FALSE(TryParseDouble("abc", d));
    CHECK_FALSE(TryParseDouble("", d));
    CHECK_FALSE(TryParseDouble("- 5", d));
    CHECK_FALSE(TryParseDouble("1.2.3", d));
    CHECK_FALSE(TryParseDouble("0x10", d));
    CHECK_FALSE(TryParseDouble("1e400", d));  // overflow -> false (no infinity result)
    CHECK(TryParseDouble("Infinity", d));
    CHECK(std::isinf(d));
    CHECK(TryParseDouble("  -Infinity ", d));
    CHECK((std::isinf(d) && d < 0));
    CHECK(TryParseDouble("NaN", d));
    CHECK(std::isnan(d));
    CHECK_FALSE(TryParseDouble("inf", d));
    CHECK_FALSE(TryParseDouble("\xE2\x88\x9E", d));  // .NET Core's infinity symbol is not Mono's
}

TEST_CASE("negative zero parses to +0 (runtime NumberBufferToDouble)") {
    double d = 1;
    CHECK(TryParseDouble("-0", d));
    CHECK(Bits(d) == 0);
    CHECK(TryParseDouble("-0.0", d));
    CHECK(Bits(d) == 0);
}

TEST_CASE("float.TryParse rejects float overflow, Parse throws") {
    float f = 0;
    CHECK_FALSE(TryParseSingle("1e39", f));
    CHECK(TryParseSingle("3.4e38", f));
    CHECK_THROWS_AS(ParseSingle("1e39"), OverflowException);
    CHECK_THROWS_AS(ParseSingle("x"), FormatException);
    CHECK(ParseSingle("Infinity") == std::numeric_limits<float>::infinity());
    CHECK(ParseSingle("0.85") == 0.85f);
}

TEST_CASE("NumberToDouble reproduces common decimal values") {
    for (const char* s : {"0.1", "0.85", "1.19", "2.83", "0.6960422", "101.325", "13599840256", "3531600000000",
                          "6.67408E-11", "9.81"}) {
        double d = 0;
        REQUIRE(TryParseDouble(s, d));
        CHECK(d == std::strtod(s, nullptr));
    }
}

TEST_CASE("integer parsing (NumberStyles.Integer)") {
    int32_t i = 5;
    CHECK(TryParseInt32(" 42 ", i));
    CHECK(i == 42);
    CHECK(TryParseInt32("-2147483648", i));
    CHECK(i == INT32_MIN);
    CHECK_FALSE(TryParseInt32("2147483648", i));
    CHECK_FALSE(TryParseInt32("4.0", i));
    CHECK_FALSE(TryParseInt32("1,000", i));
    CHECK_FALSE(TryParseInt32("1e3", i));
    CHECK(TryParseInt32("007", i));
    CHECK(i == 7);
    uint32_t u = 0;
    CHECK_FALSE(TryParseUInt32("-1", u));
    CHECK(TryParseUInt32("4294967295", u));
    CHECK(u == 4294967295u);
    uint64_t ul = 0;
    CHECK(TryParseUInt64("18446744073709551615", ul));
    CHECK_FALSE(TryParseUInt64("18446744073709551616", ul));
    uint8_t b = 0;
    CHECK(TryParseByte("255", b));
    CHECK_FALSE(TryParseByte("256", b));
}

TEST_CASE("bool.TryParse") {
    bool b = false;
    CHECK(TryParseBool("True", b));
    CHECK(b);
    CHECK(TryParseBool("false", b));
    CHECK_FALSE(b);
    CHECK(TryParseBool("  TRUE \t", b));
    CHECK(b);
    CHECK_FALSE(TryParseBool("1", b));
    CHECK_FALSE(TryParseBool("yes", b));
}

TEST_CASE("G9 / G17 / G formatting (.NET Framework rules)") {
    CHECK(FormatSingleG(0.1f, 9) == "0.100000001");
    CHECK(FormatSingleG(0.1f) == "0.1");
    CHECK(FormatSingleG(1234567.0f) == "1234567");
    CHECK(FormatSingleG(12345678.0f) == "1.234568E+07");
    CHECK(FormatSingleG(1e-5f) == "1E-05");
    CHECK(FormatSingleG(0.0001f) == "0.0001");
    CHECK(FormatSingleG(-0.0f, 9) == "0");
    CHECK(FormatSingleG(std::numeric_limits<float>::infinity(), 9) == "Infinity");
    CHECK(FormatDoubleG(1e20, 17) == "1E+20");
    CHECK(FormatDoubleG(1.0 / 3.0, 17) == "0.33333333333333331");
    CHECK(FormatDoubleG(0.1, 17) == "0.10000000000000001");
    CHECK(FormatDoubleG(13599840256.0, 17) == "13599840256");
    CHECK(FormatDoubleG(0.1) == "0.1");
    CHECK(FormatDoubleG(123456789012345680.0) == "1.23456789012346E+17");
    CHECK(FormatDoubleG(std::nan(""), 17) == "NaN");
    CHECK(FormatDoubleG(-1.5, 17) == "-1.5");
}

TEST_CASE("Mono formatting rounds 17 digits half-up (CAPTURE-SPEC C7)") {
    REQUIRE(GetRuntime() == Runtime::Mono);
    CHECK(FormatDoubleG(5.997450259893145) == "5.99745025989315");
    CHECK(FormatDoubleG(5.997450259893145, 0, Runtime::NetFramework) == "5.99745025989314");
    CHECK(FormatDoubleG(0.1, 17) == "0.10000000000000001");
    CHECK(FormatDoubleF(2.5, 0) == "3");
    CHECK(FormatDoubleF(-1.25, 6) == "-1.250000");
    CHECK(FormatDoubleR(0.1) == "0.1");
    CHECK(FormatSingleR(0.1f) == "0.1");
}
