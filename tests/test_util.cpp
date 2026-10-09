// WP-03 utilities: KSPUtil (time/date formatting, SI, coordinates, strings, versions, craft ids), Localizer + Lingoona
// fallback, UtilMath completion, .NET collection semantics (Dictionary order, List.Sort), collection helpers, enum
// formatting, Simplex noise, StringExtensions / StringBuilderCache / MinMaxColor / IntersectInformation.
// Expected values are derived by hand from the C# (KSPUtil.cs etc.); the double-precision and string functions are also
// checked against KSP's own IL on .NET Framework 4.8 in test_wp03_reference.cpp. Float32 paths (HeadingDegrees,
// RoundToPlaces(float), BSPSolver(float), ColorLerp) are "Mono-rule, verified by reasoning" (STATUS.md lists them for a
// future in-game capture).
#include <cmath>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/util/collections.hpp"
#include "ksp/util/enum_extensions.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/util/misc_util.hpp"
#include "ksp/util/simplex.hpp"

using namespace ksp;

namespace {
struct EnglishLocalizer {
    EnglishLocalizer() {
        Localizer::Shutdown();
        Localizer::InitWithBuiltInEnglish();
        GameSettings::KERBIN_TIME = true;
    }
    ~EnglishLocalizer() {
        GameSettings::KERBIN_TIME = true;
        Localizer::Shutdown();
        KSPUtil::CacheLocalStrings();  // back to the uncached (empty) strings
    }
};
// 2 years, 3 days, 4 h, 5 min, 6 s in Kerbin time
constexpr double kT = 9201600.0 * 2 + 21600.0 * 3 + 3600.0 * 4 + 60.0 * 5 + 6.0;
}  // namespace

TEST_CASE("KSPUtil.PrintTime family: Kerbin calendar, valuesOfInterest, signs") {
    EnglishLocalizer loc;
    KSPUtil::DefaultDateTimeFormatter f;
    CHECK(f.Day() == 21600);
    CHECK(f.Year() == 9201600);
    CHECK(f.PrintTime(3723.5, 3, false) == "1h, 2m, 3s");
    CHECK(f.PrintTime(3723.5, 2, false) == "1h, 2m");
    CHECK(f.PrintTime(-3723.5, 2, false) == "- 1h, 2m");
    CHECK(f.PrintTime(3723.5, 2, true) == "+ 1h, 2m");
    CHECK(f.PrintTime(0.0, 3, false) == "0");
    CHECK(f.PrintTime(0.0, 3, true) == "+ ");  // explicitPositive with nothing else (sic)
    CHECK(f.PrintTime(kT, 3, false) == "2y, 3d, 4h");
    CHECK(f.PrintTime(kT, 5, false) == "2y, 3d, 4h, 5m, 6s");
    CHECK(f.PrintTime(kT, 3, false, true) == "2y, 3d, 4h");
    CHECK(f.PrintTime(std::numeric_limits<double>::quiet_NaN(), 3, false) == "NaN");
    CHECK(f.PrintTime(std::numeric_limits<double>::infinity(), 3, false) == "+Inf");
    CHECK(f.PrintTime(-std::numeric_limits<double>::infinity(), 3, false) == "-Inf");
    // a finite UT too large for (int) gives int.MinValue years and Math.Abs throws (sic)
    CHECK_THROWS_AS(f.PrintTime(1e300, 3, false), dotnet::OverflowException);
    CHECK(f.PrintTimeLong(3723.5) == "0Years, 0Days, 1Hour, 2Mins, 3Secs");  // no space before the unit (sic)
    CHECK(f.PrintTimeLong(-61.0) == "-0Years, 0Days, 0Hours, 1Min, 1Sec");
    CHECK(f.PrintTimeStamp(kT, true, true) == "Year 2, Day 3 - 04:05:06");
    CHECK(f.PrintTimeStamp(kT) == "04:05:06");
    CHECK(f.PrintTimeStamp(9201600.0 * 12) == "00:00");  // seconds shown only below 10 years
    CHECK(f.PrintTimeStampCompact(kT, true, true) == "2y, 3d, 04:05:06");
    CHECK(f.PrintTimeCompact(kT, false) == "3:04:05:06");  // years are not shown (sic)
    CHECK(f.PrintTimeCompact(-3723.5, false) == "T-  - 01:02:03");
    CHECK(f.PrintTimeCompact(3723.5, true) == "T+ 01:02:03");
    // Earth calendar
    GameSettings::KERBIN_TIME = false;
    CHECK(f.Day() == 86400);
    CHECK(f.Year() == 31536000);
    CHECK(f.PrintTime(kT, 3, false) == "213d, 22h, 5m");
}

TEST_CASE("KSPUtil.PrintDate family, caches and the shared date holder") {
    EnglishLocalizer loc;
    KSPUtil::DefaultDateTimeFormatter f;
    // fresh formatter: the (false,false) cache matches y/d/h/m == 0 and returns the never-set (null) string
    CHECK(f.PrintDate(30.0, false).empty());
    CHECK(f.PrintDate(30.0, true) == "Year 1, Day 1 - 0h, 0m");
    CHECK(f.PrintDate(kT, true, true) == "Year 3, Day 4 - 4h, 5m, 6s");
    CHECK(f.PrintDate(kT, false) == "Year 3, Day 4");
    CHECK(f.GetUTYear() == 3);
    CHECK(f.GetUTDay() == 4);
    CHECK(f.GetTimeSpanHour() == 4);
    CHECK(f.PrintDateCompact(kT, true, true) == "Y3, D04, 04:05:06");
    CHECK(f.PrintDateCompact(kT, true, true) == "Y3, D04, 04:05:06");  // cached
    CHECK(f.PrintDateCompact(kT + 0.5, true, false) == "Y3, D04, 04:05");
    KSPUtil::DefaultDateTimeFormatter g;
    CHECK(g.PrintDateCompact(-0.5, false).empty());  // floor(-0.5) == -1 == the initial cache key (sic)
    CHECK(g.PrintTimeStampCompact(-0.25).empty());
    CHECK(f.PrintDateNew(kT, true) == "Year 3, Day 4 - 04:05:06");
    CHECK(f.PrintDateDelta(kT, true, true, false) == "2 years, 3 days, 4 hours, 5 minutes, 6 seconds");
    const double ones = 9201600.0 + 21600.0 + 3600.0 + 60.0 + 1.0;
    CHECK(f.PrintDateDelta(ones, true, true, false) == "1 year, 1 day, 1 hour, 1 minute, 1 second");
    CHECK(f.PrintDateDelta(0.0, false, false, false) == "0 days");
    CHECK(f.PrintDateDelta(0.0, true, false, false) == "0 minutes");
    CHECK(f.PrintDateDelta(0.0, true, true, false) == "0 seconds");
    CHECK(f.PrintDateDelta(-kT, true, true, true) == "2 years, 3 days, 4 hours, 5 minutes, 6 seconds");
    CHECK(f.PrintDateDelta(-kT, true, true, false) == "0 seconds");  // negative components are never printed
    CHECK(f.PrintDateDeltaCompact(kT, true, true, false) == "2y, 3d, 4h, 5m, 6s");
    CHECK(f.PrintDateDeltaCompact(kT, true, true, false, 2) == "2y, 3d");
    CHECK(f.PrintDateDeltaCompact(0.0, true, false, false) == "0m");
    // the static KSPUtil forwarders use the process-wide formatter
    CHECK(KSPUtil::PrintTime(3723.5, 3, false) == "1h, 2m, 3s");
    CHECK(KSPUtil::PrintDateDeltaCompact(kT, true, true, 1) == "2y");
    int y, d, h, m, s;
    IDateFormatterExtensions::GetTimeComponents(f, kT, y, d, h, m, s);
    CHECK((y == 2 && d == 3 && h == 4 && m == 5 && s == 6));
    IDateFormatterExtensions::GetDateComponents(f, kT, y, d, h, m, s);
    CHECK((y == 3 && d == 4));
}

TEST_CASE("KSPUtil without cached strings: units are empty, PrintDateNew needs a Localizer") {
    Localizer::Shutdown();
    KSPUtil::CacheLocalStrings();
    KSPUtil::DefaultDateTimeFormatter f;
    CHECK(f.PrintTime(3723.5, 3, false) == "1, 2, 3");  // Localizer.Format returns "" without an instance
    CHECK(f.PrintDateCompact(kT, false) == "3, 04");
    CHECK_THROWS_AS(f.PrintDateNew(kT, false), NullReferenceException);
}

TEST_CASE("Localizer: tags, parameters, Lingoona fallback, unescaping") {
    EnglishLocalizer loc;
    Localizer& l = *Localizer::Instance();
    l.SetTagValue("#t_a", "Value <<1>> and <<2>>");
    l.SetTagValue("#t_g", "Hello <<1>>, <<A:name>> <<3>>.");
    l.SetTagValue("#t_esc", "line1\\nline2 \\u00E9\\u00C9 \xEF\xBD\xA2x\xEF\xBD\xA3");
    l.SetTagValue("#t_param", "Kerbin");
    CHECK(Localizer::Format("#t_a", {"1", "2"}) == "Value 1 and 2");
    CHECK(Localizer::Format("#t_g", {"Jeb^M"}) == "Hello Jeb, A:name .");  // '^' suffix dropped; named tag unwrapped
    CHECK(Localizer::Format("#t_a", {"#t_param", "x"}) == "Value Kerbin and x");  // tag parameters are translated
    CHECK(Localizer::Format("#t_esc") == "line1\nline2 \xC3\xA9\xC3\x89 {x}");
    CHECK(Localizer::Format("not a tag") == "not a tag");
    CHECK(Localizer::Format("") == "");
    CHECK(Lingoona::Grammar::useGrammar("<<1>> <<x>> <<2>>") == " x ");
    CHECK(Lingoona::Grammar::useGrammar("a<<b<<1>>", {"P"}) == "a<<bP");
    CHECK(Lingoona::Grammar::useGrammar("<<Q:1>>", {"P"}) == "P");
    std::string v;
    CHECK(Localizer::TryGetStringByTag("#autoLOC_6002321", v));
    CHECK(v == "y");
    CHECK_THROWS_AS(Localizer::GetStringByTag("#missing"), KeyNotFoundException);
    Localizer::SetShowKeysOnScreen(true);
    CHECK(Localizer::Format("#t_param") == "#t_param");
    Localizer::SetShowKeysOnScreen(false);
    // TranslateBranch
    auto n = ConfigNode::Create("ROOT");
    n->AddValue("title", "#t_param");
    n->AddNode("CHILD")->AddValue("x", "#t_param");
    Localizer::TranslateBranch(*n);
    CHECK(n->GetValue("title") == "Kerbin");
    CHECK(n->GetNode("CHILD")->GetValue("x") == "Kerbin");
    // AddTagValuesFromNode: values processed from the last, so the FIRST duplicate wins
    auto lang = ConfigNode::Create("en-us");
    lang->AddValue("#dup", "first");
    lang->AddValue("#dup", "second");
    l.AddTagValuesFromNode(*lang);
    CHECK(Localizer::Format("#dup") == "first");
}

TEST_CASE("KSPUtil.PrintSI, LocalizeNumber, coordinates") {
    EnglishLocalizer loc;
    CHECK(KSPUtil::PrintSI(1234.5, "m") == "1.23km");
    CHECK(KSPUtil::PrintSI(0.000123, "g") == "123\xCE\xBCg");
    CHECK(KSPUtil::PrintSI(999999.0, "m") == "1000km");  // rounding after the prefix choice (sic)
    CHECK(KSPUtil::PrintSI(0.0, "m") == "0m");
    CHECK(KSPUtil::PrintSI(12345678.0, "W", 2) == "12MW");
    CHECK(KSPUtil::PrintSI(123456.0, "W", 1) == "100kW");
    CHECK(KSPUtil::PrintSI(1e30, "g") == "1000000.00Yg");  // beyond yotta the mantissa grows
    CHECK(KSPUtil::PrintSI(1234.5, "m", 3, true) == "1.23kilom");
    CHECK(KSPUtil::PrintSI(std::numeric_limits<double>::infinity(), "m") == "Infinitym");
    CHECK_THROWS_AS(KSPUtil::PrintSI(123456.0, "W", 0), std::out_of_range);  // digitsScale[3]
    CHECK(KSPUtil::LocalizeNumber(3.14159, "F2") == "3.14");
    CHECK(KSPUtil::LocalizeNumber(std::numeric_limits<double>::infinity(), "F2") == "Infinity");
    CHECK(KSPUtil::LocalizeNumber(2.5f, "G") == "2.5");
    CHECK_THROWS_AS(KSPUtil::LocalizeNumber(1.0, "N0"), std::logic_error);
    CHECK(KSPUtil::PrintLatitude(12.5) == "12\xC2\xB0 30' 0\" N");
    CHECK(KSPUtil::PrintLatitude(-0.0001) == "0\xC2\xB0 0' 0\" S");
    CHECK(KSPUtil::PrintLongitude(-123.456789) == "123\xC2\xB0 27' 24\" W");
    CHECK(KSPUtil::PrintLongitude(10.0, false, false) == "10\xC2\xB0 E");
    CHECK(KSPUtil::PrintCoordinates(1.0, 2.0, true) == "1\xC2\xB0 0' 0\" N, 2\xC2\xB0 0' 0\" E");
}

TEST_CASE("KSPUtil string helpers") {
    EnglishLocalizer loc;
    CHECK(KSPUtil::PrintModuleName("ModuleEngineFX") == "Engine FX");
    CHECK(KSPUtil::PrintSpacedStringFromCamelcase("camelCaseString") == "Camel Case String");
    CHECK(KSPUtil::PrintSpacedStringFromCamelcase("AaBbCcDd") == "Aa Bb CcDd");  // loop bound is the original length
    CHECK(KSPUtil::PrintSpacedStringFromCamelcase("ABC") == "ABC");
    CHECK(KSPUtil::PrintSpacedStringFromCamelcase("") == "");
    CHECK(KSPUtil::PrintSpacedStringFromCamelcase("\xC3\xA9tat\xC3\x89t\xC3\xA9") == "\xC3\x89tat \xC3\x89t\xC3\xA9");
    CHECK(KSPUtil::PrintLocalizedModuleName("ModuleAblator") == "#autoLOC_501008");
    CHECK(KSPUtil::PrintLocalizedModuleName("LiquidFuel") == "#autoLOC_502032");
    CHECK(KSPUtil::PrintLocalizedModuleName("ModuleWheelBase") == "Wheel Base");
    CHECK(KSPUtil::SanitizeString("a/b:c", '_', true) == "a_b_c");
    CHECK(KSPUtil::SanitizeString("  ", '_', true) == "Unnamed");
    CHECK(KSPUtil::SanitizeString("CON", '_', false) == "CON_");
    CHECK(KSPUtil::SanitizeString("  x.y  ", '-', false) == "x-y");
    CHECK(KSPUtil::SanitizeFilename("my.craft") == "my_craft");
    CHECK(KSPUtil::CheckDOSName("Clock$"));
    CHECK_FALSE(KSPUtil::CheckDOSName("com5"));
    CHECK(KSPUtil::SanitizeInstanceName("mk1pod (Clone)") == "mk1pod ");  // the "(clone" match overrides (sic)
    CHECK(KSPUtil::SanitizeInstanceName("part(Clone)") == "part");
    CHECK(KSPUtil::SanitizeInstanceName("a (b)") == "a");
    CHECK(KSPUtil::StripFileExtension("C:\\x\\y\\craft.v2.craft") == "craft.v2");
    CHECK(KSPUtil::StripFileExtension("dir/name.") == "name.");
    CHECK(KSPUtil::StripFileExtension("noext") == "noext");
    CHECK(KSPUtil::AppendValueToString("", "size0", ',') == "size0");
    CHECK(KSPUtil::AppendValueToString("size0", "srf", ',') == "size0, srf");
    CHECK(KSPUtil::AppendValueToString("size0, srf", "srf", ',') == "size0, srf");
    const std::vector<KSPUtil::StringReplacement> rep = {{"a", "bb"}, {"b", "c"}};
    CHECK(KSPUtil::ReplaceString("aab", rep) == "ccccc");
    CHECK(KSPUtil::PrintCollection(std::vector<std::string>{"a", "b", "c"}) == "a, b, c");
    CHECK(KSPUtil::PrintCollection(std::vector<int>{1, 2}, "|", [](int v) { return std::to_string(v * 10); }) == "10|20");
    const std::vector<int> ia = {1, -2};
    CHECK(KSPUtil::WriteArray(std::span<const int>(ia)) == "1; -2");
    const std::vector<double> da = {0.1, 1e20};
    CHECK(KSPUtil::WriteArray(std::span<const double>(da)) == "0.1; 1E+20");
    const auto parsed = KSPUtil::ParseArray<int>(" 1; 2 ;3", [](const std::string& s) { return dotnet::ParseInt32(s); });
    CHECK(parsed == std::vector<int>{1, 2, 3});
}

TEST_CASE("KSPUtil.CheckVersion, craft id parsing, GenerateSuperSeed") {
    CHECK(KSPUtil::CheckVersion("1.12.5", 1, 0, 0) == VersionCompareResult::COMPATIBLE);
    CHECK(KSPUtil::CheckVersion("1.12.6", 1, 0, 0) == VersionCompareResult::INCOMPATIBLE_TOO_LATE);
    CHECK(KSPUtil::CheckVersion("1.13.0", 1, 0, 0) == VersionCompareResult::INCOMPATIBLE_TOO_LATE);
    CHECK(KSPUtil::CheckVersion("0.90.0", 1, 0, 0) == VersionCompareResult::INCOMPATIBLE_TOO_EARLY);
    CHECK(KSPUtil::CheckVersion("1.12", 1, 0, 0) == VersionCompareResult::INVALID);
    CHECK_THROWS_AS(KSPUtil::CheckVersion("a.b.c", 1, 0, 0), dotnet::FormatException);
    CHECK(KSPUtil::GetPartName("mk1pod_4294704370") == "mk1pod");
    CHECK_THROWS_AS(KSPUtil::GetPartName("nounderscore"), std::out_of_range);
    std::string pn, cid;
    KSPUtil::GetPartInfo("fuelTank.long_123", pn, cid);
    CHECK((pn == "fuelTank.long" && cid == "123"));
    CHECK(KSPUtil::GetLinkID("strut_123") == "123");
    CHECK(KSPUtil::GetLinkID("abc") == "abc");
    std::string nodeID, partID, mesh;
    Vector3f pos(9, 9, 9), rot(9, 9, 9), sec(9, 9, 9), apos, arot;
    CHECK_FALSE(KSPUtil::GetAttachNodeInfo("top,fuelTank_4294705000", nodeID, partID, pos));
    CHECK((nodeID == "top" && partID == "4294705000" && pos.Equals(Vector3f(9, 9, 9))));
    CHECK(KSPUtil::GetAttachNodeInfo("bottom,fuelTank_42_0|1.5|0", nodeID, partID, pos));
    CHECK(pos.Equals(Vector3f(0, 1.5f, 0)));
    CHECK(KSPUtil::GetAttachNodeInfo("srfAttach,part_1,mesh,1|2|3,0|0|0,1|0|0", nodeID, partID, pos, mesh, rot, sec));
    CHECK((mesh == "mesh" && pos.Equals(Vector3f(1, 2, 3)) && rot.Equals(Vector3f(0, 0, 0)) && sec.Equals(Vector3f(1, 0, 0))));
    CHECK_THROWS_AS(KSPUtil::GetAttachNodeInfo("srfAttach,part_1,mesh,1|2|3", nodeID, partID, pos, mesh, rot, sec),
                    std::out_of_range);
    CHECK(KSPUtil::GetAttachNodeInfo("top,part_7_1|2|3_0|0|0", nodeID, partID, pos, rot, apos, arot));
    CHECK((pos.Equals(Vector3f(1, 2, 3)) && rot.Equals(Vector3f(0, 0, 0)) && std::isinf(apos.x) && std::isinf(arot.z)));
    CHECK_THROWS_AS(KSPUtil::GetAttachNodeInfo("top", nodeID, partID, pos), std::out_of_range);
    // GenerateSuperSeed: bytes 0..2 folded with 4..6, 8..10, 12..14; byte 3 untouched (sic)
    Guid g = Guid::Parse("00000001-0000-0000-0000-000000000000");  // bytes[0] = 1
    CHECK(KSPUtil::GenerateSuperSeed(g, 0) == 1);       // (1+0)*(2)/2 + 0
    CHECK(KSPUtil::GenerateSuperSeed(g, 5) == 26);      // (6*7)/2 + 5
    Guid h = Guid::Parse("ff000000-0000-0000-0000-000000000000");  // bytes[3] = 0xff -> num negative
    // num = -16777216: (num * (num + 1)) wraps to -16777216, / 2 = -8388608
    CHECK(KSPUtil::GenerateSuperSeed(h, 0) == -8388608);
}

TEST_CASE("KSPUtil maths helpers: heading/bearing (Mono-rule, verified by reasoning), matrices") {
    const Vector3f up(0, 1, 0), north(0, 0, 1), east(1, 0, 0);
    CHECK(KSPUtil::BearingDegrees(north, north, up) == 0.0f);
    // Cross(north, east) = +up: Acos(0) * 1 * 57.29578f rounded once from double; Cross(east, north) = -up
    const float acos0 = Mathf::Acos(0.0f);
    CHECK(KSPUtil::BearingDegrees(north, east, up) == f32store(fx(acos0) * fx(57.29578f)));
    CHECK(KSPUtil::HeadingRadians(north, east, up) == acos0);
    CHECK(KSPUtil::BearingRadians(east, north, up) == -acos0);
    CHECK(KSPUtil::HeadingRadians(east, north, up) == UtilMath::WrapAround(-acos0, 0.0f, 6.28318548f));
    CHECK(KSPUtil::HeadingDegrees(east, north, up) ==
          UtilMath::WrapAround(f32store(-fx(acos0) * fx(57.29578f)), 0.0f, 360.0f));
    const Matrix4x4f m = KSPUtil::OuterProduct(Vector3f(1, 2, 3), Vector3f(4, 5, 6));
    CHECK((m.m00 == 4 && m.m12 == 12 && m.m21 == 15 && m.m33 == 1 && m.m03 == 0));
    Matrix4x4f a = KSPUtil::ToDiagonalMatrix(2.0f);
    KSPUtil::Add(a, m);
    CHECK((a.m00 == 6 && a.m33 == 3 && a.m01 == 5));
    CHECK(KSPUtil::Diag(a).x == 6.0);
    Matrix4x4f d = KSPUtil::ToDiagonalMatrix(Vector3f(1, 2, 3));
    CHECK((d.m11 == 2 && d.m33 == 1));
    const std::set<int> s1 = {1, 2, 3}, s2 = {3, 2, 1}, s3 = {1, 2, 4};
    CHECK(KSPUtil::DeepCompare(s1, s2));
    CHECK_FALSE(KSPUtil::DeepCompare(s1, s3));
}

TEST_CASE("UtilMath completion: Max/Min(params), MaxFrom/MinFrom, .NET Math.Min in BSPSolver, WrapAround(int)") {
    CHECK(UtilMath::Max({1.0, 5.0, 3.0}) == 5.0);
    CHECK(UtilMath::Min({1.0, 5.0, -3.0}) == -3.0);
    CHECK(UtilMath::Max({}) == std::numeric_limits<double>::lowest());
    CHECK(UtilMath::Max({std::nan(""), 2.0}) == 2.0);  // NaN never compares greater
    struct Item {
        int id;
        double v;
    };
    const std::vector<Item> items = {{0, 1.0}, {1, 7.0}, {2, 7.0}, {3, -2.0}};
    auto get = [](const Item& i) { return i.v; };
    CHECK(UtilMath::MaxFrom(get, std::span<const Item>(items)).id == 2);  // scanned from the end: last equal wins
    CHECK(UtilMath::MinFrom(get, std::span<const Item>(items)).id == 3);
    double mx = 0;
    CHECK(UtilMath::MaxFrom(get, mx, std::span<const Item>(items)).id == 2);
    CHECK(mx == 7.0);
    float fmin = 0;
    auto getf = [](const Item& i) { return static_cast<float>(i.v); };
    CHECK(UtilMath::MinFrom(getf, fmin, std::span<const Item>(items)).id == 3);
    CHECK(fmin == -2.0f);
    CHECK_THROWS_AS(UtilMath::MaxFrom(get, std::span<const Item>()), std::out_of_range);
    CHECK(UtilMath::WrapAround(-1, 0, 5) == 4);
    CHECK(UtilMath::WrapAround(7, 0, 5) == 2);
    CHECK_THROWS_AS(UtilMath::WrapAround(1, 0, 0), DivideByZeroException);
    CHECK(UtilMath::WrapAround(std::numeric_limits<int>::max(), std::numeric_limits<int>::max() + 0, 1) == 0);
    CHECK_THROWS_AS(UtilMath::Flatten(std::nan(""), 1.0, 2.0), ArithmeticException);
    CHECK(UtilMath::IsPowerOfTwo(std::numeric_limits<int>::min()));
    CHECK(dotnet::Math::Min(0.0, -0.0) == 0.0);
    CHECK(std::signbit(dotnet::Math::Min(0.0, -0.0)));
    CHECK(std::isnan(dotnet::Math::Min(1.0, std::nan(""))));
    CHECK(std::isnan(dotnet::Math::Max(std::nan(""), 1.0)));
    // BSPSolver(double): sqrt(2) by bisection on |x^2 - 2| (29 halvings of dv = 0.5 reach 1e-9)
    double v0 = 2.0;
    const int it = UtilMath::BSPSolver(v0, 0.5, [](double x) { return x * x - 2.0; }, 0.0, 4.0, 1e-9, 100);
    CHECK(it == 29);
    CHECK(std::fabs(v0 - std::sqrt(2.0)) < 1e-8);
    // the initial value is NOT taken absolute (sic): a negative start never compares equal to |f| and v0 never moves
    double v1 = 1.0;
    CHECK(UtilMath::BSPSolver(v1, 0.5, [](double x) { return x * x - 2.0; }, 0.0, 4.0, 1e-9, 100) == 29);
    CHECK(v1 == 1.0);
}

TEST_CASE(".NET Dictionary order and List.Sort (introsort) semantics") {
    dotnet::Dictionary<std::string, int> d;
    d.Add("a", 1);
    d.Add("b", 2);
    d.Add("c", 3);
    d.Remove("b");
    d.Add("d", 4);  // reuses b's slot
    CHECK(d.Keys() == std::vector<std::string>{"a", "d", "c"});
    d.Remove("a");
    d.Remove("c");
    d.Add("e", 5);  // last freed slot first (c's)
    d.Add("f", 6);  // then a's
    CHECK(d.Keys() == std::vector<std::string>{"f", "d", "e"});
    CHECK_THROWS_AS(d.Add("f", 0), ArgumentException);
    CHECK_THROWS_AS(d.At("zz"), KeyNotFoundException);
    dotnet::Dictionary<std::string, int> copy(d);  // compacting copy in enumeration order
    copy.Add("g", 7);
    CHECK(copy.Keys() == std::vector<std::string>{"f", "d", "e", "g"});
    d.Set("d", 40);
    CHECK(d.Values() == std::vector<int>{6, 40, 5});

    // sort correctness on a larger input (introsort with heapsort fallback)
    std::vector<int> v;
    for (int i = 0; i < 1000; ++i) v.push_back((i * 7919) % 1009);
    dotnet::ListSort(v, [](int a, int b) { return a < b ? -1 : (a > b ? 1 : 0); });
    CHECK(std::is_sorted(v.begin(), v.end()));
    // up to 16 elements the algorithm is an insertion sort: stable for equal keys
    std::vector<std::pair<int, int>> small = {{2, 0}, {1, 1}, {2, 2}, {1, 3}, {0, 4}};
    dotnet::ListSort(small, [](const auto& a, const auto& b) { return a.first - b.first; });
    CHECK(small == std::vector<std::pair<int, int>>{{0, 4}, {1, 1}, {1, 3}, {2, 0}, {2, 2}});
    std::vector<int> bad = {3, 1, 2, 5, 4, 9, 8, 7, 6, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    CHECK_THROWS_AS(dotnet::ListSort(bad, [](int, int) -> int { throw std::runtime_error("x"); }),
                    dotnet::InvalidOperationException);
}

TEST_CASE("Collection helpers: ListExtension, ArrayExtensions, BiDictionary, DictionaryValueList, ListDictionary") {
    std::vector<int> l = {1, 2};
    CHECK(util::AddUnique(l, 2) == 2);
    util::AddUnique(l, 3);
    CHECK(l == std::vector<int>{1, 2, 3});
    util::AddUniqueRange(l, std::vector<int>{3, 4, 4, 5});
    CHECK(l == std::vector<int>{1, 2, 3, 4, 5});
    CHECK_THROWS_AS(util::MaxAt(l, [](int v) { return v; }), std::out_of_range);  // always (sic)
    CHECK_THROWS_AS(util::MinAt(std::vector<int>{}, [](int v) { return v; }), ArgumentNullException);
    std::vector<int> target = {0, 0, 0};
    util::SetRange(target, std::vector<int>{7, 8, 9, 10}, 1);
    CHECK(target == std::vector<int>{0, 7, 8});
    const std::vector<int> arr = {5, 6, 7};
    CHECK(util::IndexOf(std::span<const int>(arr), 7) == 2);
    CHECK(util::IndexOf(std::span<const int>(arr), 8) == -1);
    // Shuffle uses UnityEngine.Random.Range(0, num + 1) from the end
    UnityRandom r(42);
    UnityRandom r2(42);
    std::vector<int> sh = {0, 1, 2, 3, 4};
    util::Shuffle(sh, r);
    std::vector<int> expect = {0, 1, 2, 3, 4};
    for (int num = 4; num >= 1; --num) std::swap(expect[static_cast<size_t>(r2.Range(0, num + 1))], expect[static_cast<size_t>(num)]);
    CHECK(sh == expect);
    // OrderByAlphaNumeric: digit runs padded, stable
    const std::vector<std::string> names = {"part10", "part9", "Part2", "part9"};
    const auto ordered = util::OrderByAlphaNumeric(names, [](const std::string& s) { return s; });
    CHECK(ordered == std::vector<std::string>{"Part2", "part9", "part9", "part10"});
    struct P {
        uint32_t persistentId;
    };
    P p1{1}, p2{2};
    std::vector<P*> parts = {&p1};
    CHECK(util::ContainsId(parts, &p1));
    CHECK_FALSE(util::ContainsId(parts, &p2));
    CHECK(util::ContainsId<P>(parts, 1u));

    BiDictionaryOneToOne<std::string, int> bi;
    bi.Add("a", 1);
    CHECK_THROWS_AS(bi.Add("b", 1), ArgumentException);
    CHECK(bi.GetBySecond(1) == "a");
    CHECK_FALSE(bi.TryAdd("a", 2));
    bi.RemoveBySecond(1);
    CHECK(bi.Count() == 0);
    CHECK_THROWS_AS(bi.GetByFirst("a"), ArgumentException);

    DictionaryValueList<std::string, int> dvl;
    dvl.Add("x", 1);
    dvl.Add("y", 2);
    CHECK(dvl.Add("x", 3));  // re-added at the end
    CHECK(dvl.KeysList() == std::vector<std::string>{"y", "x"});
    CHECK(dvl.ValuesList() == std::vector<int>{2, 3});
    dvl.Set("z", 2);  // equal value to y's
    dvl.Remove("z");  // removes the FIRST 2 in the value list (y's) -- lists desynchronise (sic)
    CHECK(dvl.ValuesList() == std::vector<int>{3, 2});
    CHECK(dvl.KeysList() == std::vector<std::string>{"y", "x"});
    dvl.SortList([](int a, int b) { return a - b; });
    CHECK(dvl.ValuesList() == std::vector<int>{2, 3});

    ListDictionary<std::string, int> ld;
    CHECK_FALSE(ld.Add("k", 1));
    CHECK(ld.Add("k", 1));  // already present
    ld.Add("k", 2);
    ld.Add("k", 1);
    CHECK(ld["k"]->size() == 2);
    ListDictionary<std::string, int> shallow(ld, true), deep(ld, false);
    ld["k"]->push_back(9);
    CHECK(shallow["k"]->size() == 3);  // shares the list object
    CHECK(deep["k"]->size() == 2);
    CHECK(ld["missing"]->empty());
    CHECK(ld.Remove("k", 9));
    CHECK_FALSE(ld.Remove("k", 42));
}

namespace {
enum class TestEnum : int { Zero = 0, One = 1, Uno = 1, Two = 2, Big = 100 };
enum class TestFlags : int { None = 0, A = 1, B = 2, C = 4, AB = 3 };
enum class ByteEnum : uint8_t { X = 1 };
}  // namespace
template <>
struct ksp::dotnet::EnumTraits<TestEnum> {
    static constexpr bool flags = false;
    static constexpr EnumMember<TestEnum> members[] = {{TestEnum::Big, "Big"},
                                                       {TestEnum::Zero, "Zero", "Nothing"},
                                                       {TestEnum::One, "One", "First"},
                                                       {TestEnum::Uno, "Uno", "Primero"},
                                                       {TestEnum::Two, "Two"}};
};
template <>
struct ksp::dotnet::EnumTraits<TestFlags> {
    static constexpr bool flags = true;
    static constexpr EnumMember<TestFlags> members[] = {
        {TestFlags::None, "None"}, {TestFlags::A, "A"}, {TestFlags::B, "B"}, {TestFlags::C, "C"}, {TestFlags::AB, "AB"}};
};
template <>
struct ksp::dotnet::EnumTraits<ByteEnum> {
    static constexpr bool flags = false;
    static constexpr EnumMember<ByteEnum> members[] = {{ByteEnum::X, "X"}};
};

TEST_CASE("Enum formatting (Enum.ToString semantics) and EnumExtensions") {
    CHECK(dotnet::EnumToString(TestEnum::Two) == "Two");
    CHECK(dotnet::EnumToString(static_cast<TestEnum>(7)) == "7");
    // values sorted (unsorted declaration -> introsort): [0 Zero, 1 One, 1 Uno, 2 Two, 100 Big]; binary search for 1
    // probes index 2 first -> "Uno"
    CHECK(dotnet::EnumToString(TestEnum::One) == "Uno");
    CHECK(dotnet::EnumGetNames<TestEnum>() == std::vector<std::string>{"Zero", "One", "Uno", "Two", "Big"});
    CHECK(dotnet::EnumToString(static_cast<TestFlags>(7)) == "AB, C");
    CHECK(dotnet::EnumToString(static_cast<TestFlags>(5)) == "A, C");
    CHECK(dotnet::EnumToString(TestFlags::None) == "None");
    CHECK(dotnet::EnumToString(static_cast<TestFlags>(9)) == "9");
    CHECK(util::Description(TestEnum::Zero) == "Nothing");
    CHECK(util::Description(TestEnum::Two) == "Two");
    CHECK(util::Description(TestEnum::One) == "Primero");  // ToString picks "Uno"
    CHECK_THROWS_AS(util::Description(static_cast<TestEnum>(7)), std::out_of_range);
    CHECK(util::GetDescriptions(TestEnum::Zero) == std::vector<std::string>{"Nothing", "Primero", "Primero", "Two", "Big"});
    CHECK(util::ToStringCached(TestEnum::Big) == "Big");
    CHECK_THROWS_AS(util::ToStringCached(ByteEnum::X), InvalidCastException);
    Localizer::Shutdown();
    CHECK(util::displayDescription(TestEnum::Zero).empty());  // Localizer.Format without an instance
}

TEST_CASE("Simplex / SimplexNoise: kernel quirks and the shared static table") {
    Simplex s;
    s.octaves = 1;
    s.persistence = 0.5;
    s.frequency = 1;
    // the default instance and the static SimplexNoise use the same initial table
    for (double x : {0.1, 1.7, -3.3, 12.25}) CHECK(s.noise(x, 0.5 * x, -x) == SimplexNoise::noise(x, 0.5 * x, -x));
    CHECK(s.noiseNormalized(1, 2, 3) == (SimplexNoise::noise(1, 2, 3) + 1.0) * 0.5);
    CHECK(SimplexNoise::noise(2, 0.5, 1, 1, 2, 3) == (SimplexNoise::noise(1, 2, 3) + 0.5 * SimplexNoise::noise(2, 4, 6)) / 1.5);
    Simplex zero;  // octaves 0 -> 0/0
    CHECK(std::isnan(zero.noise(1, 2, 3)));
    const auto saved = Simplex::StaticPermutation();
    const auto savedRandom = UnityRandom::Global().state();
    // seeding rewrites the shared table and reseeds UnityEngine.Random; p[255] is never redrawn
    Simplex seeded(1234, 1, 0.5, 1);
    UnityRandom check(1234);
    for (int i = 0; i < 255; ++i) CHECK(Simplex::StaticPermutation()[static_cast<size_t>(i)] == check.Range(0, 255));
    CHECK(Simplex::StaticPermutation()[255] == 180);
    CHECK(UnityRandom::Global().state().s0 == check.state().s0);
    Simplex later;  // constructed after the seed: uses the modified shared table
    later.octaves = 1;
    later.frequency = 1;
    CHECK(later.noise(0.3, 0.4, 0.5) == seeded.noise(0.3, 0.4, 0.5));
    Simplex::StaticPermutation() = saved;
    UnityRandom::Global().SetState(savedRandom);
}

TEST_CASE("StringExtensions, StringBuilderCache.Format, MinMaxColor, IntersectInformation, Clear no-ops") {
    CHECK(StringExtensions::GetHashCode_Net35("abc") == 96354);
    CHECK(StringExtensions::GetHashCode_Net35("") == 0);
    CHECK(StringBuilderCache::Format("{0}-{1,4}|{1,-4}|{{x}}", {"a", "bc"}) == "a-  bc|bc  |{x}");
    CHECK(StringBuilderCache::Format("{0:F2}", {"s"}) == "s");
    CHECK_THROWS_AS(StringBuilderCache::Format("{1}", {"a"}), dotnet::FormatException);
    CHECK_THROWS_AS(StringBuilderCache::Format("}", {}), dotnet::FormatException);
    MinMaxColor mm{Color(0, 0, 0, 0), Color(1, 0.5f, 0.25f, 1)};
    const Color c = mm.GetLerp(0.5f);
    CHECK((c.r == 0.5f && c.g == 0.25f && c.b == 0.125f && c.a == 0.5f));
    CHECK(mm.GetLerp(2.0f).r == 1.0f);  // t clamped
    const IntersectInformation ii(2, 10.0, 1.0, 20.0, 2.0);
    CHECK((ii.intersect2Distance == 10.0 && ii.intersect1Distance == 20.0));  // swapped by the constructor (sic)
    Callback<> cb = Callback<>::Lambda(&mm, [] {});
    CallbackUtil::Clear(cb);
    DelegateExtensions::Clear(cb, 0);
    CHECK_FALSE(cb == nullptr);  // unchanged
}
