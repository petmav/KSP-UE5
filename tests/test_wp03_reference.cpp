// WP-03 against KSP's own IL: tests/data/wp03_reference.tsv was produced by tools/wp03_reference (KSP 1.12.5
// Assembly-CSharp run on .NET Framework 4.8, see its README comment). Every row is recomputed with kspcore and must
// match exactly: doubles/floats bit for bit (any NaN matches any NaN), strings byte for byte, exceptions by type.
// The .NET runtime is selected for number formatting (ksp::dotnet::Runtime::NetFramework) because the reference ran
// there; the formatting rows only use values whose text is the same on Mono.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/util/collections.hpp"
#include "ksp/util/enum_extensions.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/util/misc_util.hpp"
#include "ksp/util/simplex.hpp"

using namespace ksp;

namespace {

// ---- row parsing ------------------------------------------------------------------------------------------------------
std::vector<std::string> SplitTabs(const std::string& line) {
    std::vector<std::string> out;
    size_t start = 0;
    while (true) {
        const size_t p = line.find('\t', start);
        out.push_back(line.substr(start, p == std::string::npos ? std::string::npos : p - start));
        if (p == std::string::npos) break;
        start = p + 1;
    }
    return out;
}
std::string Unescape(const std::string& s) {
    if (s == "<null>") return "";  // C# null -> kspcore ""
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            const char c = s[++i];
            out.push_back(c == 't' ? '\t' : c == 'n' ? '\n' : c == 'r' ? '\r' : c);
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}
double Dbl(const std::string& s) {
    REQUIRE(s.size() == 17);
    REQUIRE(s[0] == 'x');
    const uint64_t bits = std::stoull(s.substr(1), nullptr, 16);
    double d;
    std::memcpy(&d, &bits, 8);
    return d;
}
float Flt(const std::string& s) {
    REQUIRE(s.size() == 9);
    REQUIRE(s[0] == 'f');
    const uint32_t bits = static_cast<uint32_t>(std::stoul(s.substr(1), nullptr, 16));
    float f;
    std::memcpy(&f, &bits, 4);
    return f;
}
std::string D(double v) {
    uint64_t bits;
    std::memcpy(&bits, &v, 8);
    char buf[32];
    std::snprintf(buf, sizeof buf, "x%016llX", static_cast<unsigned long long>(bits));
    return buf;
}
std::string F(float v) {
    uint32_t bits;
    std::memcpy(&bits, &v, 4);
    char buf[16];
    std::snprintf(buf, sizeof buf, "f%08X", bits);
    return buf;
}
std::string I(int v) { return std::to_string(v); }
std::string B(bool b) { return b ? "1" : "0"; }
std::string V3(const Vector3f& v) { return F(v.x) + "," + F(v.y) + "," + F(v.z); }

// equality of a recomputed cell with the reference cell: doubles/floats by bits (NaN == NaN), exceptions by type
// (std::out_of_range stands for both IndexOutOfRangeException and ArgumentOutOfRangeException)
bool CellEquals(const std::string& expected, const std::string& actual) {
    if (expected == actual) return true;
    if (expected.size() == 17 && actual.size() == 17 && expected[0] == 'x' && actual[0] == 'x')
        return std::isnan(Dbl(expected)) && std::isnan(Dbl(actual));
    if (expected.size() == 9 && actual.size() == 9 && expected[0] == 'f' && actual[0] == 'f')
        return std::isnan(Flt(expected)) && std::isnan(Flt(actual));
    if ((expected == "EXC:IndexOutOfRangeException" || expected == "EXC:ArgumentOutOfRangeException") &&
        actual == "EXC:OutOfRange")
        return true;
    return false;
}

template <class F>
std::string Try(F&& f) {
    try {
        return f();
    } catch (const dotnet::OverflowException&) {
        return "EXC:OverflowException";
    } catch (const dotnet::FormatException&) {
        return "EXC:FormatException";
    } catch (const NullReferenceException&) {
        return "EXC:NullReferenceException";
    } catch (const ArithmeticException&) {
        return "EXC:ArithmeticException";
    } catch (const InvalidCastException&) {
        return "EXC:InvalidCastException";
    } catch (const DivideByZeroException&) {
        return "EXC:DivideByZeroException";
    } catch (const std::out_of_range&) {
        return "EXC:OutOfRange";
    }
}

struct Stats {
    std::map<std::string, std::pair<int, int>> byKind;  // rows, mismatches
    int total = 0, mismatches = 0;
    void Check(const std::string& kind, const std::string& line, const std::string& expected, const std::string& actual) {
        auto& k = byKind[kind];
        ++k.first;
        ++total;
        if (!CellEquals(expected, actual)) {
            ++k.second;
            if (++mismatches <= 25) MESSAGE("mismatch [" << kind << "] " << line << "\n   expected: " << expected << "\n   actual:   " << actual);
        }
    }
};

// ---- the C# fixtures mirrored ------------------------------------------------------------------------------------------
enum class RefEnum : int { Big = 100, Zero = 0, One = 1, Uno = 1, Two = 2 };
enum class RefFlags : int { None = 0, A = 1, B = 2, C = 4, AB = 3 };
enum class RefByteEnum : uint8_t { X = 1 };

struct Listener {
    std::string id;
    std::vector<std::string>* trace;
    void OnInt(int v) { trace->push_back(id + ":" + std::to_string(v)); }
};
struct Thrower {
    void OnInt(int) { throw std::runtime_error("boom"); }
};
struct Ret {
    double d = 0;
    float f = 0;
    double Dv() { return d; }
    float Fv() { return f; }
};
struct Mod {
    int Twice(int v, std::string) { return v * 2; }
    int AddLen(int v, std::string s) { return v + static_cast<int>(s.size()); }
};

std::string Join(const std::vector<std::string>& v, const char* sep) {
    std::string r;
    for (size_t i = 0; i < v.size(); ++i) r += (i ? sep : "") + v[i];
    return r;
}

}  // namespace

template <>
struct ksp::dotnet::EnumTraits<RefEnum> {
    static constexpr bool flags = false;
    static constexpr EnumMember<RefEnum> members[] = {{RefEnum::Big, "Big"},
                                                      {RefEnum::Zero, "Zero", "Nothing"},
                                                      {RefEnum::One, "One", "First"},
                                                      {RefEnum::Uno, "Uno", "Primero"},
                                                      {RefEnum::Two, "Two"}};
};
template <>
struct ksp::dotnet::EnumTraits<RefFlags> {
    static constexpr bool flags = true;
    static constexpr EnumMember<RefFlags> members[] = {
        {RefFlags::None, "None"}, {RefFlags::A, "A"}, {RefFlags::B, "B"}, {RefFlags::C, "C"}, {RefFlags::AB, "AB"}};
};
template <>
struct ksp::dotnet::EnumTraits<RefByteEnum> {
    static constexpr bool flags = false;
    static constexpr EnumMember<RefByteEnum> members[] = {{RefByteEnum::X, "X"}};
};

TEST_CASE("WP-03 vs KSP IL on .NET Framework 4.8 (tests/data/wp03_reference.tsv)") {
    std::ifstream in(std::string(KSPCORE_TEST_DATA_DIR) + "/wp03_reference.tsv", std::ios::binary);
    REQUIRE(in.good());
    const dotnet::Runtime savedRuntime = dotnet::GetRuntime();
    dotnet::SetRuntime(dotnet::Runtime::NetFramework);
    log::SetSink(nullptr);

    std::vector<std::string> lines;
    for (std::string l; std::getline(in, l);) lines.push_back(l);

    // the built-in English subset (kspcore's no-GameData fallback) must equal the shipped dictionary (TAG rows); then
    // the full dictionary is loaded like the reference run did (when the KSP dev copy is present)
    Localizer::Shutdown();
    Localizer::InitWithBuiltInEnglish();
    std::map<std::string, std::string> builtIn;
    for (const auto& l : lines)
        if (l.rfind("TAG\t#", 0) == 0) {
            const std::string tag = SplitTabs(l)[1];
            builtIn[tag] = Localizer::GetStringByTag(tag);
        }
    Localizer::Shutdown();
    Localizer::Init();
    const bool haveDictionary = std::filesystem::exists(KSP_DICTIONARY_CFG);
    if (haveDictionary) {
        auto root = ConfigNode::Load(KSP_DICTIONARY_CFG);
        REQUIRE(root);
        ConfigNode* loc = root->GetNode("Localization");
        REQUIRE(loc);
        std::vector<ConfigNode*> nodes{loc};
        Localizer::Instance()->AddTagValuesForLanguage(nodes, "en-us");
        KSPUtil::CacheLocalStrings();
    } else {
        MESSAGE("dictionary.cfg not found at " << KSP_DICTIONARY_CFG << ": using the built-in English subset");
        Localizer::InitWithBuiltInEnglish();
    }

    Stats st;
    const Vector3d vs[] = {Vector3d(1, 0, 0),        Vector3d(0, 2, 0), Vector3d(-3, 4, 5), Vector3d(1e-8, 1, 0),
                           Vector3d(-1, -1e-12, 0), Vector3d(0, 0, 0), Vector3d(600000, 12, -7.5)};
    KSPUtil::DefaultDateTimeFormatter seqFormatter;
    for (const std::string& line : lines) {
        if (line.empty() || line[0] == '#') continue;
        const auto c = SplitTabs(line);
        const std::string& kind = c[0];
        const std::string& name = c.size() > 1 ? c[1] : kind;
        const std::string expected = c.back();
        std::string actual;

        if (kind == "UM") {
            if (expected == "skip") continue;
            if (name == "WrapAroundI") {
                actual = Try([&] { return I(UtilMath::WrapAround(std::stoi(c[2]), std::stoi(c[3]), std::stoi(c[4]))); });
            } else if (name == "IsPowerOfTwo") {
                actual = B(UtilMath::IsPowerOfTwo(std::stoi(c[2])));
            } else if (name == "IsDivisible") {
                actual = B(UtilMath::IsDivisible(std::stoi(c[2]), std::stoi(c[3])));
            } else if (name == "Max") {
                actual = D(UtilMath::Max({1.0, 5.0, std::nan(""), 3.0}));
            } else if (name == "Min") {
                actual = D(UtilMath::Min({1.0, -5.0, std::nan(""), 3.0}));
            } else if (name == "MaxEmpty") {
                actual = D(UtilMath::Max(std::span<const double>()));
            } else if (name == "MaxFrom" || name == "MinFrom") {
                const double vals[] = {1.0, 7.0, 7.0, -2.0, std::nan(""), -2.0};
                const int idx[] = {0, 1, 2, 3, 4, 5};
                double ext = 0;
                auto get = [&](const int& i) { return vals[i]; };
                const int r = name == "MaxFrom" ? UtilMath::MaxFrom(get, ext, std::span<const int>(idx))
                                                : UtilMath::MinFrom(get, ext, std::span<const int>(idx));
                st.Check(kind, line, c[2], I(r));
                actual = D(ext);
            } else if (name == "BSPSolver" || name == "BSPSolverSin") {
                double v0 = Dbl(c[2]);
                int it;
                if (name == "BSPSolver") it = UtilMath::BSPSolver(v0, 0.5, [](double x) { return x * x - 2.0; }, 0.0, 4.0, 1e-9, 100);
                else it = UtilMath::BSPSolver(v0, 0.75, [](double x) { return dotnet::Math::Sin(x) - 0.5; }, 0.0, 4.0, 1e-6, 12);
                st.Check(kind, line, c[3], I(it));
                actual = D(v0);
            } else if (name == "AngleBetween") {
                actual = D(UtilMath::AngleBetween(vs[std::stoi(c[2])], vs[std::stoi(c[3])]));
            } else if (name == "SphereIntersection") {
                const int a = std::stoi(c[2]), b = std::stoi(c[3]);
                double t = 0;
                const bool hit = UtilMath::SphereIntersection(2.5, vs[a], vs[b], t, (a + b) % 2 == 0);
                st.Check(kind, line, c[4], B(hit));
                actual = D(t);
            } else if (name == "Flatten2") {
                actual = D(UtilMath::Flatten(Dbl(c[2]), Dbl(c[3]), 0.5));
            } else {
                const double x = Dbl(c[2]);
                static const std::map<std::string, std::function<std::string(double)>> fns = {
                    {"ASinh", [](double v) { return D(UtilMath::ASinh(v)); }},
                    {"ACosh", [](double v) { return D(UtilMath::ACosh(v)); }},
                    {"ATanh", [](double v) { return D(UtilMath::ATanh(v)); }},
                    {"ACoth", [](double v) { return D(UtilMath::ACoth(v)); }},
                    {"ASech", [](double v) { return D(UtilMath::ASech(v)); }},
                    {"ACsch", [](double v) { return D(UtilMath::ACsch(v)); }},
                    {"Sech", [](double v) { return D(UtilMath::Sech(v)); }},
                    {"Csch", [](double v) { return D(UtilMath::Csch(v)); }},
                    {"Coth", [](double v) { return D(UtilMath::Coth(v)); }},
                    {"Clamp01", [](double v) { return D(UtilMath::Clamp01(v)); }},
                    {"Clamp", [](double v) { return D(UtilMath::Clamp(v, -1.5, 2.5)); }},
                    {"ClampRadians", [](double v) { return D(UtilMath::ClampRadians(v)); }},
                    {"ClampRadiansPI", [](double v) { return D(UtilMath::ClampRadiansPI(v)); }},
                    {"ClampRadiansTwoPI", [](double v) { return D(UtilMath::ClampRadiansTwoPI(v)); }},
                    {"ClampDegrees360", [](double v) { return D(UtilMath::ClampDegrees360(v)); }},
                    {"ClampDegrees180", [](double v) { return D(UtilMath::ClampDegrees180(v)); }},
                    {"WrapAroundD", [](double v) { return D(UtilMath::WrapAround(v, -1.0, 3.0)); }},
                    {"Lerp", [](double v) { return D(UtilMath::Lerp(2.0, 7.0, v)); }},
                    {"LerpUnclamped", [](double v) { return D(UtilMath::LerpUnclamped(2.0, 7.0, v)); }},
                    {"InverseLerp", [](double v) { return D(UtilMath::InverseLerp(2.0, 7.0, v)); }},
                    {"DegreesToRadians", [](double v) { return D(UtilMath::DegreesToRadians(v)); }},
                    {"RadiansToDegrees", [](double v) { return D(UtilMath::RadiansToDegrees(v)); }},
                    {"RoundToPlaces", [](double v) { return D(UtilMath::RoundToPlaces(v, 3)); }},
                    {"Approximately", [](double v) { return B(UtilMath::Approximately(v, 1.0, 1e-3)); }},
                    {"Flatten", [](double v) { return Try([&] { return D(UtilMath::Flatten(v, 2.0, 1.5)); }); }},
                };
                auto it = fns.find(name);
                REQUIRE_MESSAGE(it != fns.end(), "unknown UM row " << name);
                actual = it->second(x);
            }
        } else if (kind == "TAG") {
            if (name == "count") {
                if (!haveDictionary) continue;
                actual = I(Localizer::TagsLength());
            } else {
                st.Check(kind, line, Unescape(c[2]), builtIn[name]);
                if (haveDictionary) st.Check(kind, line, Unescape(c[2]), Localizer::GetStringByTag(name));
                continue;
            }
        } else if (kind == "LOC") {
            if (name == "Format") {
                std::vector<std::string> ps;
                std::stringstream ss(Unescape(c[3]));
                std::string p;
                while (std::getline(ss, p, '|')) ps.push_back(p);
                actual = Localizer::Format(Unescape(c[2]), ps);
            } else {
                actual = Localizer::Format(Unescape(c[2]));
            }
            st.Check(kind, line, Unescape(expected), actual);
            continue;
        } else if (kind == "DATE") {
            GameSettings::KERBIN_TIME = c[2] == "K";
            const double t = Dbl(c[3]);
            const std::string& o = c[4];
            auto bit = [&](size_t i) { return o[i] == '1'; };
            KSPUtil::DefaultDateTimeFormatter f;
            if (name == "Holder") {
                f.PrintDate(t, true, true);
                actual = I(f.GetUTYear()) + "," + I(f.GetUTDay()) + "," + I(f.GetUTHour()) + "," + I(f.GetUTMinute()) + "," +
                         I(f.GetUTSecond());
                st.Check(kind, line, expected, actual);
                continue;
            }
            if (name == "GetTimeComponents") {
                int y, d, h, m, s;
                IDateFormatterExtensions::GetTimeComponents(f, t, y, d, h, m, s);
                st.Check(kind, line, expected, I(y) + "," + I(d) + "," + I(h) + "," + I(m) + "," + I(s));
                continue;
            }
            actual = Try([&]() -> std::string {
                if (name == "PrintTimeLong") return f.PrintTimeLong(t);
                if (name == "PrintTimeStamp") return f.PrintTimeStamp(t, bit(0), bit(1));
                if (name == "PrintTimeStampCompact") return f.PrintTimeStampCompact(t, bit(0), bit(1));
                if (name == "PrintDate") return f.PrintDate(t, bit(0), bit(1));
                if (name == "PrintDateCompact") return f.PrintDateCompact(t, bit(0), bit(1));
                if (name == "PrintDateDelta") return f.PrintDateDelta(t, bit(0), bit(1), bit(2));
                if (name == "PrintDateDeltaCompact") return f.PrintDateDeltaCompact(t, bit(0), bit(1), bit(2));
                if (name == "PrintDateDeltaCompact2") return f.PrintDateDeltaCompact(t, bit(0), bit(1), bit(2), 2);
                if (name == "PrintTime") return f.PrintTime(t, o[0] - '0', bit(1), bit(2));
                if (name == "PrintTimeCompact") return f.PrintTimeCompact(t, bit(0));
                if (name == "PrintDateNew") return f.PrintDateNew(t, bit(0));
                FAIL("unknown DATE row " << name);
                return "";
            });
            if (actual.rfind("EXC:", 0) != 0) {
                st.Check(kind + "." + name, line, Unescape(expected), actual);
                continue;
            }
            st.Check(kind + "." + name, line, expected, actual);
            continue;
        } else if (kind == "DATESEQ") {
            GameSettings::KERBIN_TIME = true;
            const double t = Dbl(c[2]);
            if (name == "PrintDate") actual = seqFormatter.PrintDate(t, false, false);
            else if (name == "PrintDateCompact") actual = seqFormatter.PrintDateCompact(t, true, false);
            else actual = seqFormatter.PrintTimeStampCompact(t, false, false);
            st.Check(kind, line, Unescape(expected), actual);
            continue;
        } else if (kind == "SI") {
            actual = Try([&] { return KSPUtil::PrintSI(Dbl(c[1]), "m", std::stoi(c[2]), c[3] == "1"); });
            st.Check(kind, line, actual.rfind("EXC:", 0) == 0 ? expected : Unescape(expected), actual);
            continue;
        } else if (kind == "COORD") {
            if (name == "Coords") {
                actual = KSPUtil::PrintCoordinates(1.0, 2.0, true) + "|" + KSPUtil::PrintCoordinates(-1.5, -2.25, false);
                // the reference escapes each half separately
                std::string e = expected;
                st.Check(kind, line, Unescape(e), actual);
                continue;
            }
            const double a = Dbl(c[2]);
            const bool m = c[3][0] == '1', s = c[3][1] == '1';
            actual = name == "Lat" ? KSPUtil::PrintLatitude(a, m, s) : KSPUtil::PrintLongitude(a, m, s);
            st.Check(kind, line, Unescape(expected), actual);
            continue;
        } else if (kind == "LN") {
            const std::string fmt = Unescape(c[3]);
            actual = name == "D" ? KSPUtil::LocalizeNumber(Dbl(c[2]), fmt) : KSPUtil::LocalizeNumber(Flt(c[2]), fmt);
            st.Check(kind, line, Unescape(expected), actual);
            continue;
        } else if (kind == "STR") {
            const std::string a = c.size() > 2 ? Unescape(c[2]) : "";
            if (name == "Hash35") actual = I(StringExtensions::GetHashCode_Net35(a));
            else if (name == "Camel") actual = KSPUtil::PrintSpacedStringFromCamelcase(a);
            else if (name == "ModuleName") actual = KSPUtil::PrintModuleName(a);
            else if (name == "LocalizedModuleName") actual = KSPUtil::PrintLocalizedModuleName(a);
            else if (name == "Sanitize_") actual = KSPUtil::SanitizeString(a, '_', true);
            else if (name == "SanitizeDash") actual = KSPUtil::SanitizeString(a, '-', false);
            else if (name == "SanitizeFilename") actual = KSPUtil::SanitizeFilename(a);
            else if (name == "CheckDOSName") actual = B(KSPUtil::CheckDOSName(a));
            else if (name == "SanitizeInstanceName") actual = KSPUtil::SanitizeInstanceName(a);
            else if (name == "StripFileExtension") actual = KSPUtil::StripFileExtension(a);
            else if (name == "AppendValueToString") actual = KSPUtil::AppendValueToString(a, Unescape(c[3]), ',');
            else if (name == "SBFormat") {
                std::vector<std::string> ps;
                std::stringstream ss(Unescape(c[3]));
                std::string p;
                while (std::getline(ss, p, '|')) ps.push_back(p);
                actual = Try([&] { return StringBuilderCache::Format(a, ps); });
            } else if (name == "CheckVersion" || name == "CheckVersion1125") {
                actual = Try([&]() -> std::string {
                    const auto r = name == "CheckVersion" ? KSPUtil::CheckVersion(a, 1, 0, 0) : KSPUtil::CheckVersion(a, 1, 12, 5);
                    static const char* const n[] = {"INCOMPATIBLE_TOO_EARLY", "COMPATIBLE", "INCOMPATIBLE_TOO_LATE", "INVALID"};
                    return n[static_cast<int>(r)];
                });
            } else if (name == "GetPartName") {
                actual = Try([&] { return KSPUtil::GetPartName(a); });
            } else if (name == "GetLinkID") {
                actual = KSPUtil::GetLinkID(a);
            } else if (name == "AttachMesh") {
                std::string nodeID = "n", partID = "p", mesh = "m";
                Vector3f pos(9, 9, 9), rot(9, 9, 9), sec(9, 9, 9);
                actual = Try([&] {
                    const bool ok = KSPUtil::GetAttachNodeInfo(a, nodeID, partID, pos, mesh, rot, sec);
                    return B(ok) + "|" + nodeID + "|" + partID + "|" + mesh + "|" + V3(pos) + "|" + V3(rot) + "|" + V3(sec);
                });
            } else if (name == "AttachActual") {
                std::string nodeID = "n", partID = "p";
                Vector3f p2(9, 9, 9), r2(9, 9, 9), ap(9, 9, 9), ar(9, 9, 9);
                actual = Try([&] {
                    const bool ok = KSPUtil::GetAttachNodeInfo(a, nodeID, partID, p2, r2, ap, ar);
                    return B(ok) + "|" + nodeID + "|" + partID + "|" + V3(p2) + "|" + V3(r2) + "|" + V3(ap) + "|" + V3(ar);
                });
                // the C# row reuses nodeID/partID left over from the AttachMesh call on the same input; both set them first
            } else if (name == "SuperSeed") {
                actual = I(KSPUtil::GenerateSuperSeed(Guid::Parse(a), std::stoi(c[3])));
            } else if (name == "WriteArrayI") {
                const int v[] = {1, -2, 3};
                actual = KSPUtil::WriteArray(std::span<const int>(v));
            } else if (name == "WriteArrayD") {
                const double v[] = {0.1, 1e20, -0.0, std::nan("")};
                actual = KSPUtil::WriteArray(std::span<const double>(v));
            } else if (name == "WriteArrayF") {
                const float v[] = {0.1f, 3e-7f};
                actual = KSPUtil::WriteArray(std::span<const float>(v));
            } else if (name == "WriteArrayB") {
                const bool v[] = {true, false};
                actual = KSPUtil::WriteArray(std::span<const bool>(v));
            } else if (name == "PrintCollection") {
                actual = KSPUtil::PrintCollection(std::vector<std::string>{"a", "b", "c"}, ", ");
            } else {
                FAIL("unknown STR row " << name);
            }
            const bool isExc = expected.rfind("EXC:", 0) == 0;
            st.Check(kind + "." + name, line, isExc ? expected : Unescape(expected), actual);
            continue;
        } else if (kind == "VEC") {
            if (name == "WriteVector3d" || name == "WriteQuaternionD") {
                const Vector3d v(Dbl(c[2]), Dbl(c[3]), Dbl(c[4]));
                if (name == "WriteVector3d") {
                    actual = KSPUtil::WriteVector(v) + "|" + KSPUtil::WriteVector(v, " ; ");
                } else {
                    const QuaternionD q(v.x, v.y, v.z, 0.5);
                    actual = KSPUtil::WriteQuaternion(q) + "|" + KSPUtil::WriteQuaternion(q, "/");
                }
                st.Check(kind, line, Unescape(expected), actual);
                continue;
            }
            const std::string s = Unescape(c[2]);
            auto d3 = [](const Vector3d& v) { return D(v.x) + "," + D(v.y) + "," + D(v.z); };
            actual = Try([&]() -> std::string {
                if (name == "ParseVector3d") return d3(KSPUtil::ParseVector3d(s));
                if (name == "ParseVector3dSemi") return d3(KSPUtil::ParseVector3d(s, ';'));
                if (name == "ParseQuaternionD") {
                    const QuaternionD q = KSPUtil::ParseQuaternionD(s);
                    return D(q.x) + "," + D(q.y) + "," + D(q.z) + "," + D(q.w);
                }
                std::vector<std::string> p;
                std::stringstream ss(s);
                for (std::string part; std::getline(ss, part, ',');) p.push_back(part);
                if (name == "ParseVector3dParts") return d3(KSPUtil::ParseVector3d(p[0], p[1], p[2]));
                return V3(KSPUtil::ParseVector3(p[0], p[1], p[2]));
            });
        } else if (kind == "SIMPLEX") {
            const double x = Dbl(c[2]), y = Dbl(c[3]), z = Dbl(c[4]);
            if (name == "Static") actual = D(SimplexNoise::noise(x, y, z));
            else if (name == "Octaves") actual = D(SimplexNoise::noise(4.0, 0.5, 0.01, x, y, z));
            else {
                Simplex s;
                s.octaves = 3.5;
                s.persistence = 0.6;
                s.frequency = 0.05;
                actual = D(name == "Instance" ? s.noise(x, y, z) : s.noiseNormalized(x, y, z));
            }
        } else if (kind == "EVT") {
            std::vector<std::string> trace;
            Listener a{"a", &trace}, b{"b", &trace}, cc{"c", &trace}, d{"d", &trace};
            using E = events::EventData<int>;
            if (name == "Order") {
                E ev("ref.order");
                ev.Add(&a, &Listener::OnInt);
                ev.Add(&b, &Listener::OnInt);
                ev.Add(&cc, &Listener::OnInt);
                ev.Add(&a, &Listener::OnInt);
                ev.Fire(5);
                ev.Remove(&a, &Listener::OnInt);
                ev.Fire(6);
                actual = Join(trace, " ");
            } else if (name == "R1") {
                E r1("ref.r1");
                r1.Add(&a, &Listener::OnInt);
                r1.Add(&b, &Listener::OnInt);
                r1.Add(E::OnEvent::Lambda(&cc, [&](int v) {
                    trace.push_back("c:" + I(v));
                    r1.Remove(&a, &Listener::OnInt);
                    r1.Add(&b, &Listener::OnInt);
                }));
                r1.Fire(1);
                r1.Fire(2);
                actual = Join(trace, " ");
            } else if (name == "R2" || name == "R3" || name == "R4") {
                E r("ref." + name);
                bool fired = false;
                r.Add(&a, &Listener::OnInt);
                if (name == "R2") {
                    r.Add(E::OnEvent::Lambda(&b, [&](int v) {
                        trace.push_back("b:" + I(v));
                        if (!fired) {
                            fired = true;
                            r.Fire(v + 10);
                        }
                    }));
                    r.Add(&cc, &Listener::OnInt);
                } else {
                    r.Add(&b, &Listener::OnInt);
                    r.Add(E::OnEvent::Lambda(&cc, [&](int v) {
                        trace.push_back("c:" + I(v));
                        if (!fired) {
                            fired = true;
                            r.Remove(&a, &Listener::OnInt);
                            r.Remove(&b, &Listener::OnInt);
                            r.Fire(v + 10);
                        }
                    }));
                    if (name == "R3") r.Add(&d, &Listener::OnInt);
                }
                std::string exc = "none";
                try {
                    r.Fire(1);
                } catch (const std::out_of_range&) {
                    exc = "ArgumentOutOfRangeException";
                }
                if (name == "R4") {
                    r.Fire(2);
                    st.Check(kind, line, c[3], exc);
                    actual = Join(trace, " ");
                    st.Check(kind, line, c[2], actual);
                    continue;
                }
                actual = Join(trace, " ");
            } else if (name == "Throw") {
                std::vector<std::string> logs;
                log::SetSink([&](log::Level l, std::string_view m) {
                    logs.push_back(std::string(l == log::Level::Error ? "Error: " : "Log: ") + std::string(m));
                });
                E ex("ref.throw");
                Thrower th;
                ex.Add(&a, &Listener::OnInt);
                ex.Add(&th, &Thrower::OnInt);
                ex.Add(&cc, &Listener::OnInt);
                ex.Fire(1);
                log::SetSink(nullptr);
                st.Check(kind, line, c[2], Join(trace, " "));
                st.Check(kind, line, c[3], I(static_cast<int>(logs.size())));
                actual = logs.empty() ? "" : logs[0].substr(0, logs[0].find("Thrower") + 7);
                st.Check(kind, line, Unescape(c[4]), actual);
                continue;
            } else if (name == "Modifier") {
                events::EventDataModifier<int, std::string> mod("ref.mod");
                Mod mo;
                mod.Add(&mo, &Mod::Twice);
                mod.Add(&mo, &Mod::AddLen);
                actual = I(mod.Fire(3, "xy"));
            } else if (name == "DoubleMax") {
                events::EventDoubleMax dmax("ref.dmax", -1.0);
                Ret r2{2.0}, r5{5.0}, rn{std::nan("")};
                dmax.Add(&r2, &Ret::Dv);
                dmax.Add(&r5, &Ret::Dv);
                st.Check(kind, line, c[2], D(dmax.Fire()));
                dmax.Add(&rn, &Ret::Dv);
                actual = D(dmax.Fire());
            } else if (name == "FloatMax") {
                events::EventFloatMax fmax("ref.fmax");
                Ret r2, rn;
                r2.f = 2.0f;
                rn.f = std::nanf("");
                fmax.Add(&r2, &Ret::Fv);
                fmax.Add(&rn, &Ret::Fv);
                actual = F(fmax.Fire());
            } else if (name == "DoubleMin") {
                events::EventDoubleMin dmin("ref.dmin", 1.0);
                Ret rm{-0.0}, rp{0.0};
                dmin.Add(&rm, &Ret::Dv);
                dmin.Add(&rp, &Ret::Dv);
                actual = D(dmin.Fire());
            } else if (name == "Comparison") {
                Listener o{"o", &trace};
                events::EventValueComparison<int> cmp(
                    "ref.cmp", -1, events::EventValueComparison<int>::OnCompare::Lambda(&o, [](int x, int y) { return x > y; }));
                cmp.Add(events::EventValueComparison<int>::OnEvent::Lambda(&o, [] { return 3; }));
                cmp.Add(events::EventValueComparison<int>::OnEvent::Lambda(&o, [] { return 1; }));
                cmp.Update();
                st.Check(kind, line, c[2], I(cmp.value()));
                actual = "<null>";  // the hidden eventName property is never assigned (C# null)
                st.Check(kind, line, expected, actual);
                continue;
            } else if (name == "FindDuplicate") {
                using CoupleEvent = events::EventData<events::GameEvents::FromToAction<Part*, Part*>>;
                st.Check(kind, line, c[2],
                         B(events::GameEvents::FindEvent<events::EventData<Part*>>("onPartCouple") == &events::GameEvents::onPartDeCouple));
                actual = B(events::GameEvents::FindEvent<CoupleEvent>("onPartCouple") == nullptr);
            }
        } else if (kind == "GE") {
            // registry coverage: member path, event name and C# type of every GameEvents event
            static const std::map<std::string, const events::GameEventsEntry*> reg = [] {
                std::map<std::string, const events::GameEventsEntry*> m;
                for (const auto& e : events::GameEventsRegistry()) m[e.member] = &e;
                return m;
            }();
            auto it = reg.find(c[1]);
            if (it == reg.end()) {
                st.Check(kind, line, c[1], "<missing in kspcore>");
                continue;
            }
            st.Check(kind, line, Unescape(c[2]), it->second->event->EventName());
            actual = it->second->csType;
        } else if (kind == "SORT") {
            const int n = std::stoi(c[1]);
            uint32_t lcg = static_cast<uint32_t>(n) * 2654435761u;
            std::vector<std::pair<int, int>> list;
            for (int i = 0; i < n; ++i) {
                lcg = lcg * 1664525u + 1013904223u;
                list.emplace_back(static_cast<int>(lcg >> 24) % 7, i);
            }
            dotnet::ListSort(list, [](const auto& x, const auto& y) { return x.first < y.first ? -1 : (x.first > y.first ? 1 : 0); }, n);
            std::vector<std::string> out;
            for (const auto& p : list) out.push_back(I(p.second));
            actual = Join(out, ",");
        } else if (kind == "DICT") {
            dotnet::Dictionary<std::string, int> d;
            for (int i = 0; i < 10; ++i) d.Add("k" + I(i), i);
            d.Remove("k3");
            d.Remove("k7");
            d.Remove("k0");
            d.Add("n1", 11);
            d.Add("n2", 12);
            d.Set("k5", 55);
            d.Add("n3", 13);
            d.Add("n4", 14);
            if (name == "Order") {
                actual = Join(d.Keys(), ",");
            } else if (name == "CopyOrder") {
                dotnet::Dictionary<std::string, int> copy(d);
                copy.Remove("k1");
                copy.Add("z", 0);
                actual = Join(copy.Keys(), ",");
            } else {
                DictionaryValueList<std::string, int> dvl;
                dvl.Add("x", 1);
                dvl.Add("y", 2);
                dvl.Add("x", 3);
                dvl.Set("z", 2);
                dvl.Remove("z");
                std::vector<std::string> vl, kl;
                for (int i = 0; i < dvl.Count(); ++i) {
                    vl.push_back(I(dvl.At(i)));
                    kl.push_back(dvl.KeyAt(i));
                }
                st.Check(kind, line, c[2], Join(vl, ","));
                st.Check(kind, line, c[3], Join(kl, ","));
                actual = Join(dvl.Keys(), ",");
            }
        } else if (kind == "LIST") {
            const std::vector<std::string> names = {"part10", "part9", "Part2", "part9", "a1b22", "a1b3"};
            actual = Join(util::OrderByAlphaNumeric(names, [](const std::string& s) { return s; }), ",");
        } else if (kind == "ENUM") {
            if (name == "RefEnum") {
                const auto e = static_cast<RefEnum>(std::stoi(c[2]));
                st.Check(kind, line, c[3], dotnet::EnumToString(e));
                st.Check(kind, line, c[4] == "EXC:IndexOutOfRangeException" ? c[4] : Unescape(c[4]), Try([&] { return util::Description(e); }));
                actual = util::ToStringCached(e);
            } else if (name == "RefEnumNames") {
                actual = Join(dotnet::EnumGetNames<RefEnum>(), ",");
            } else if (name == "RefEnumDescriptions") {
                actual = Join(util::GetDescriptions(RefEnum::Zero), ",");
            } else if (name == "RefFlags") {
                actual = dotnet::EnumToString(static_cast<RefFlags>(std::stoi(c[2])));
            } else {
                actual = Try([&] { return util::ToStringCached(RefByteEnum::X); });
            }
        } else {
            FAIL("unknown row kind " << kind);
        }
        st.Check(kind, line, expected, actual);
    }

    for (const auto& [k, v] : st.byKind)
        if (v.second) MESSAGE(k << ": " << v.first << " values, " << v.second << " mismatches");
    MESSAGE("wp03_reference: " << st.total << " values compared, " << st.mismatches << " mismatches");
    CHECK(st.mismatches == 0);
    CHECK(st.total > 5000);
    CHECK(events::GameEventsRegistry().size() == 469);

    GameSettings::KERBIN_TIME = true;
    dotnet::SetRuntime(savedRuntime);
    Localizer::Shutdown();
    KSPUtil::CacheLocalStrings();
}
