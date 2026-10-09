#include "doctest/doctest.h"
#include "ksp/dotnet/text.hpp"

using namespace ksp::dotnet;

TEST_CASE("Trim uses .NET char.IsWhiteSpace") {
    CHECK(Trim("  a b \t") == "a b");
    CHECK(Trim("\xC2\xA0x\xE3\x80\x80") == "x");      // NBSP and ideographic space
    CHECK(Trim("\xE2\x80\x8Bx") == "\xE2\x80\x8Bx");  // zero-width space is not whitespace
    CHECK(Trim("\xEF\xBB\xBFx") == "\xEF\xBB\xBFx");  // U+FEFF is not whitespace in .NET 4
    CHECK(Trim("") == "");
    CHECK(Trim(" \r\n") == "");
}

TEST_CASE("ReadLine splitting and BOM handling") {
    using V = std::vector<std::string>;
    CHECK(SplitLinesLikeReadLine("a\r\nb\nc\rd") == V{"a", "b", "c", "d"});
    CHECK(SplitLinesLikeReadLine("a\r\n") == V{"a"});
    CHECK(SplitLinesLikeReadLine("a\n\nb") == V{"a", "", "b"});
    CHECK(SplitLinesLikeReadLine("") == V{});
    CHECK(SplitLinesLikeReadLine("\r") == V{""});
    CHECK(DecodeStreamText("\xEF\xBB\xBFx") == "x");
    CHECK(DecodeStreamText(std::string("\xFF\xFEh\0i\0", 6)) == "hi");
    CHECK(DecodeStreamText("a\xFF"
                           "b") == "a\xEF\xBF\xBD"
                                   "b");
}

TEST_CASE("Split keeps or removes empty entries") {
    using V = std::vector<std::string>;
    CHECK(Split("a,,b", ",", false) == V{"a", "", "b"});
    CHECK(Split("a,,b", ",", true) == V{"a", "b"});
    CHECK(Split("", ",", false) == V{""});
    CHECK(Split("", ",", true) == V{});
}
