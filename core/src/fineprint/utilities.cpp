// WP-38 slice 2: FinePrint/Utilities/{VesselUtilities,SystemUtilities,StringUtilities,ProgressUtilities}.cs (the members not ported in WP-37 / WP-38
// slice 1), the IContractObjectiveModule table and FinePrintEnvironment. See include/ksp/fineprint/utilities.hpp.
#include "ksp/fineprint/utilities.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "ksp/career/career.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/contracts/contract_system.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/fineprint/fineprint.hpp"
#include "ksp/fineprint/parameters.hpp"
#include "ksp/kerbals/crew_generator.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/career/progress.hpp"
#include "ksp/save/game.hpp"
#include "ksp/science/science.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/math/util_math.hpp"

namespace ksp::Contracts::Lite {

using KSP::Localization::Localizer;

// ------------------------------------------------------------------------------------------------------------------ FinePrintEnvironment

void FinePrintEnvironment::Post(const std::string& text, float duration, ScreenMessageStyle style) {
    screenMessages.push_back(text);
    if (postScreenMessage) postScreenMessage(text, duration, style);
}

void FinePrintEnvironment::RunDeferred() {
    std::vector<std::function<void()>> run;
    run.swap(deferred);
    for (auto& f : run) f();
}

void FinePrintEnvironment::Reset() {
    postScreenMessage = nullptr;
    screenMessages.clear();
    deferred.clear();
    siteGeneratorInfo = nullptr;
    waypointManagerPresent = nullptr;
    lateralDistanceToVessel = nullptr;
    navWaypoint = nullptr;
    setupOrbitRenderer = nullptr;
    cleanupOrbitRenderer = nullptr;
    orbitRendererDriverOrbit = nullptr;
    deltaTime = nullptr;
    cometTypeName = nullptr;
    researchedValidContractObjectives = nullptr;
    haveModuleTech = nullptr;
    probeCoresUnlocked = nullptr;
}

// ------------------------------------------------------------------------------------------------------------------ StringUtilities

namespace {
// StringUtilities.cs:73-82 (the lists of LoadSiteGenerationInfo)
struct SiteLists {
    std::vector<std::string> greekMap, alphaNumeric, uniqueSites, genericPrefixes, developerPrefixes, abstractPrefixes, abstractSuffixes, planetSuffixes,
        landSuffixes, homeSuffixes;
};
SiteLists& Sites() {
    static SiteLists s;
    return s;
}
// StringUtilities.cs:105 FillListWithElements
std::vector<std::string> FillListWithElements(const ConfigNode* node, const char* nodeName) {
    if (node != nullptr) {
        const ConfigNode* n = node->GetNode(nodeName);
        if (n != nullptr) return n->GetValuesList("Element");
    }
    return {};
}
}  // namespace

namespace StringUtilities {

}  // namespace StringUtilities

namespace LingoonaGrammar {
namespace {
// LingoonaGrammarExtensions.cs:13 LocalizeNoun(gender, isProperNoun) with LocalizeRemoveGender (:34): cut at the last '^'
std::string Noun(std::string input, char g) {
    const size_t pos = input.rfind('^');
    if (pos != std::string::npos) input.erase(pos);
    input += '^';
    input += static_cast<char>(std::toupper(static_cast<unsigned char>(g)));
    return input;
}
}  // namespace
// LingoonaGrammarExtensions.cs:50
std::string LocalizeName(std::string input, ProtoCrewMember_Gender gender) {
    char g = 'n';
    if (gender == ProtoCrewMember_Gender::Male) g = 'm';
    if (gender == ProtoCrewMember_Gender::Female) g = 'f';
    return Noun(std::move(input), g);
}
// LingoonaGrammarExtensions.cs:63
std::string LocalizeName(std::string input, GrammaticalGender gender) {
    char g = 'n';
    if (gender == GrammaticalGender::MASCULINE) g = 'm';
    if (gender == GrammaticalGender::FEMININE) g = 'f';
    return Noun(std::move(input), g);
}
}  // namespace LingoonaGrammar

namespace StringUtilities {

// StringUtilities.cs:272
std::string PackDelimitedString(const std::vector<std::string>& items, char delimiter) {
    std::string out;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i > 0) out += delimiter;
        out += items[i];
    }
    return out;
}
std::string PackDelimitedString(const std::vector<int32_t>& items, char delimiter) {
    std::vector<std::string> s;
    for (int32_t v : items) s.push_back(dotnet::FormatInt64(v));
    return PackDelimitedString(s, delimiter);
}
std::string PackDelimitedString(const std::vector<uint32_t>& items, char delimiter) {
    std::vector<std::string> s;
    for (uint32_t v : items) s.push_back(std::to_string(v));
    return PackDelimitedString(s, delimiter);
}

namespace {
std::vector<std::string> SplitOn(const std::string& items, char delimiter) {
    std::vector<std::string> out;
    size_t start = 0;
    for (;;) {
        const size_t p = items.find(delimiter, start);
        if (p == std::string::npos) {
            out.push_back(items.substr(start));
            break;
        }
        out.push_back(items.substr(start, p - start));
        start = p + 1;
    }
    return out;
}
}  // namespace

// StringUtilities.cs:281 (string TypeConverter: IsValid is true for every input)
std::vector<std::string> UnpackDelimitedStrings(const std::string& items, char delimiter) {
    if (items.empty()) return {};
    return SplitOn(items, delimiter);
}
// StringUtilities.cs:281 with T = int (Int32Converter: IsValid(string) = int.Parse succeeds under the current culture; invariant here)
std::vector<int32_t> UnpackDelimitedInt32(const std::string& items, char delimiter) {
    std::vector<int32_t> list;
    if (items.empty()) return list;
    for (const std::string& s : SplitOn(items, delimiter)) {
        int32_t v = 0;
        if (dotnet::TryParseInt32(s, v)) list.push_back(v);
    }
    return list;
}
std::vector<uint32_t> UnpackDelimitedUInt32(const std::string& items, char delimiter) {
    std::vector<uint32_t> list;
    if (items.empty()) return list;
    for (const std::string& s : SplitOn(items, delimiter)) {
        uint32_t v = 0;
        if (dotnet::TryParseUInt32(s, v)) list.push_back(v);
    }
    return list;
}

// StringUtilities.cs:89
void LoadSiteGenerationInfo() {
    const ConfigNode* configNode = FinePrintEnvironment::siteGeneratorInfo;
    SiteLists& s = Sites();
    s.greekMap = FillListWithElements(configNode, "GreekMap");
    s.alphaNumeric = FillListWithElements(configNode, "Alphanumeric");
    s.uniqueSites = FillListWithElements(configNode, "UniqueSites");
    s.genericPrefixes = FillListWithElements(configNode, "GenericPrefixes");
    s.developerPrefixes = FillListWithElements(configNode, "DeveloperPrefixes");
    s.abstractPrefixes = FillListWithElements(configNode, "AbstractPrefixes");
    s.abstractSuffixes = FillListWithElements(configNode, "AbstractSuffixes");
    s.planetSuffixes = FillListWithElements(configNode, "planetSuffixes");
    s.landSuffixes = FillListWithElements(configNode, "LandSuffixes");
    s.homeSuffixes = FillListWithElements(configNode, "HomeSuffixes");
    Templates::TemplateEnvironment::alphaNumeric = s.alphaNumeric;
}

// StringUtilities.cs:121
std::string GenerateSiteName(int seed, CelestialBody* body, bool landLocked, bool allowNamed) {
    SiteLists& s = Sites();
    KSPRandom kSPRandom(seed);
    if (kSPRandom.Next(1000) == 123) return s.uniqueSites.at(static_cast<size_t>(kSPRandom.Next(static_cast<int>(s.uniqueSites.size()))));
    std::string text3;
    if (allowNamed && kSPRandom.Next(100) < (body == Planetarium::fetch->Home ? 50 : 25)) {
        const std::string text = NamedSitePrefix(&kSPRandom);
        const std::string text2 = NamedSiteSuffix(body, landLocked, &kSPRandom);
        text3 = Localizer::Format("#autoLOC_7001300", {text, text2});
        if (!text3.empty() && text3[0] == '#') text3 = Localizer::Format("<<g:2,1>>", {text, text2});
    } else {
        text3 = s.genericPrefixes.at(static_cast<size_t>(kSPRandom.Next(static_cast<int>(s.genericPrefixes.size())))) + " " + AlphaNumericDesignation(seed);
    }
    return text3;
}

// StringUtilities.cs:146
std::string NamedSitePrefix(KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(ContractsEnvironment::Current().NewRandomSeed());
        generator = own.get();
    }
    SiteLists& s = Sites();
    const ProtoCrewMember_Gender gender = !CoinFlip(generator) ? ProtoCrewMember_Gender::Female : ProtoCrewMember_Gender::Male;
    if (CoinFlip(generator)) {
        const std::string input = ShortKerbalName(CrewGenerator::GetRandomName(gender, generator));
        return LingoonaGrammar::LocalizeName(input, gender);
    }
    if (generator->Next(100) < 10) return s.developerPrefixes.at(static_cast<size_t>(generator->Next(static_cast<int>(s.developerPrefixes.size()))));
    return s.abstractPrefixes.at(static_cast<size_t>(generator->Next(static_cast<int>(s.abstractPrefixes.size()))));
}

// StringUtilities.cs:165
std::string NamedSiteSuffix(CelestialBody* body, bool landLocked, KSPRandom* generator) {
    std::unique_ptr<KSPRandom> own;
    if (generator == nullptr) {
        own = std::make_unique<KSPRandom>(ContractsEnvironment::Current().NewRandomSeed());
        generator = own.get();
    }
    SiteLists& s = Sites();
    std::vector<std::vector<std::string>*> list{&s.abstractSuffixes};
    if (!body->hasSolidSurface) return SystemUtilities::RandomSplitChoice(list, generator);
    list.push_back(&s.planetSuffixes);
    if (body->ocean && !landLocked) return SystemUtilities::RandomSplitChoice(list, generator);
    list.push_back(&s.landSuffixes);
    if (body == Planetarium::fetch->Home) list.push_back(&s.homeSuffixes);
    return SystemUtilities::RandomSplitChoice(list, generator);
}

// StringUtilities.cs:210
std::string IntegerToGreek(int x) { return Sites().greekMap.at(static_cast<size_t>(std::min(std::max(x, 0), 23))); }

// StringUtilities.cs:255
std::string TitleCase(const std::string& str) {
    // TextInfo.ToTitleCase: a letter that follows a non-letter starts a word (upper case), the other letters are lower case; all-upper-case
    // words (acronyms) are kept
    std::string out = str;
    size_t i = 0;
    while (i < out.size()) {
        while (i < out.size() && !std::isalpha(static_cast<unsigned char>(out[i]))) ++i;
        size_t j = i;
        bool allUpper = true;
        while (j < out.size() && std::isalpha(static_cast<unsigned char>(out[j]))) {
            if (!std::isupper(static_cast<unsigned char>(out[j]))) allUpper = false;
            ++j;
        }
        if (j > i) {
            if (!(allUpper && j - i > 1)) {
                out[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[i])));
                for (size_t k = i + 1; k < j; ++k) out[k] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[k])));
            }
        }
        i = j;
    }
    return out;
}

namespace {
// Convert.ToDecimal(x).ToString("#,###.#"): Convert.ToDecimal(double) rounds to 15 significant digits; the format rounds half away from zero to one
// decimal; '#' prints no leading / trailing zero (an integral value has no decimal point, zero prints as the empty string)
std::string DecimalHashCommaHashDot(double x) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.14e", x < 0 ? -x : x);
    std::string m(buf);
    const size_t e = m.find('e');
    const int exp10 = std::atoi(m.c_str() + e + 1);
    std::string digits = m.substr(0, 1) + m.substr(2, e - 2);  // 15 significant digits d.dddd -> dddd
    // value = 0.d1d2... * 10^(exp10 + 1)
    std::string intPart, fracPart;
    const int pointPos = exp10 + 1;
    for (int i = 0; i < static_cast<int>(digits.size()) || i < pointPos; ++i) {
        const char c = i < static_cast<int>(digits.size()) ? digits[static_cast<size_t>(i)] : '0';
        if (i < pointPos)
            intPart += c;
        else
            fracPart += c;
    }
    if (pointPos <= 0) {
        intPart.clear();
        fracPart = std::string(static_cast<size_t>(-pointPos), '0') + digits;
    }
    // round half away from zero at one decimal
    std::string all = intPart + (fracPart.empty() ? "0" : fracPart.substr(0, 1));
    const bool up = fracPart.size() > 1 && fracPart[1] >= '5';
    if (up) {
        int i = static_cast<int>(all.size()) - 1;
        while (i >= 0) {
            if (all[static_cast<size_t>(i)] == '9') {
                all[static_cast<size_t>(i)] = '0';
                --i;
            } else {
                ++all[static_cast<size_t>(i)];
                break;
            }
        }
        if (i < 0) all.insert(all.begin(), '1');
    }
    std::string ip = all.substr(0, all.size() - 1);
    const char frac = all.back();
    while (!ip.empty() && ip[0] == '0') ip.erase(ip.begin());
    std::string grouped;
    for (size_t i = 0; i < ip.size(); ++i) {
        grouped += ip[i];
        const size_t rem = ip.size() - 1 - i;
        if (rem % 3 == 0 && rem != 0) grouped += ',';
    }
    std::string out = grouped;
    if (frac != '0') out += std::string(".") + frac;
    if (out.empty()) return "";
    const bool isZero = grouped.empty() && frac == '0';
    if (isZero) return "";
    return (x < 0 ? "-" : "") + out;
}
}  // namespace

// StringUtilities.cs:320
std::string ValueAndUnits(RecordTrackType trackType, double trackValue) {
    double num = trackValue;
    std::string text = Localizer::Format("#autoLOC_289916");
    switch (trackType) {
        case RecordTrackType::SPEED: text = Localizer::Format("#autoLOC_289933"); break;
        case RecordTrackType::ALTITUDE:
        case RecordTrackType::DEPTH:
        case RecordTrackType::DISTANCE:
            if (num >= 10000.0) {
                num /= 1000.0;
                text = Localizer::Format("#autoLOC_289926");
            } else {
                text = Localizer::Format("#autoLOC_289929");
            }
            break;
        default: break;
    }
    return DecimalHashCommaHashDot(num) + text;
}

// StringUtilities.cs:346
GrammaticalGender KerbalGrammaticalGender(ProtoCrewMember& pcm) {
    switch (pcm.gender()) {
        default: return GrammaticalGender::NEUTRAL;
        case ProtoCrewMember_Gender::Female: return GrammaticalGender::FEMININE;
        case ProtoCrewMember_Gender::Male: return GrammaticalGender::MASCULINE;
    }
}

// StringUtilities.cs:359
std::string AppropriatePronoun(PronounCasing casing, GrammaticalGender gender, bool postFix) {
    std::string text = Localizer::Format("#autoLOC_289969");
    switch (gender) {
        default:
            switch (casing) {
                case PronounCasing::SUBJECTIVE: text = "they"; break;
                case PronounCasing::OBJECTIVE: text = "them"; break;
                case PronounCasing::POSSESSIVE: text = "their"; break;
            }
            break;
        case GrammaticalGender::FEMININE:
            switch (casing) {
                case PronounCasing::SUBJECTIVE: text = "she"; break;
                case PronounCasing::OBJECTIVE: text = "her"; break;
                case PronounCasing::POSSESSIVE: text = "her"; break;
            }
            break;
        case GrammaticalGender::MASCULINE:
            switch (casing) {
                case PronounCasing::SUBJECTIVE: text = "he"; break;
                case PronounCasing::OBJECTIVE: text = "him"; break;
                case PronounCasing::POSSESSIVE: text = "his"; break;
            }
            break;
    }
    if (casing == PronounCasing::POSSESSIVE && postFix && text.back() != 's') text += "s";
    return text;
}

// StringUtilities.cs:414
std::string CardinalDirectionBetween(double originLatitude, double originLongitude, double destinationLatitude, double destinationLongitude) {
    constexpr double pi = 3.14159265358979323846;
    originLatitude *= pi / 180.0;
    originLongitude *= pi / 180.0;
    destinationLatitude *= pi / 180.0;
    destinationLongitude *= pi / 180.0;
    const double num = destinationLongitude - originLongitude;
    const double y = dotnet::Math::Sin(num) * dotnet::Math::Cos(destinationLatitude);
    const double x = dotnet::Math::Cos(originLatitude) * dotnet::Math::Sin(destinationLatitude) -
                     dotnet::Math::Sin(originLatitude) * dotnet::Math::Cos(destinationLatitude) * dotnet::Math::Cos(num);
    switch (static_cast<int>(dotnet::Math::Round(std::fmod(dotnet::Math::Atan2(y, x) * (180.0 / pi) + 360.0, 360.0) / 45.0))) {
        default: return Localizer::Format("#autoLOC_290062");
        case 1: return Localizer::Format("#autoLOC_290048");
        case 2: return Localizer::Format("#autoLOC_290050");
        case 3: return Localizer::Format("#autoLOC_290052");
        case 4: return Localizer::Format("#autoLOC_290054");
        case 5: return Localizer::Format("#autoLOC_290056");
        case 6: return Localizer::Format("#autoLOC_290058");
        case 7: return Localizer::Format("#autoLOC_290060");
    }
}

// StringUtilities.cs:444
bool IsVowel(char c) {
    const char array[6] = {'a', 'e', 'i', 'o', 'u', 'y'};
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    int num = 6;
    do {
        if (num-- <= 0) return false;
    } while (array[num] != c);
    return true;
}

// StringUtilities.cs:460
float CalculateReadDuration(const std::string& s) {
    const float num = 200.0f;
    const float num2 = 5.0f;
    return static_cast<float>(f32store(f32store(f32store(static_cast<float>(s.size()) / num2) / num) * 60.0f));
}

// StringUtilities.cs:559
std::string SpecificVesselName(Contract& contract) {
    auto* parameter = contract.GetParameterT<ksp::Contracts::Parameters::SpecificVesselParameter>();
    if (parameter != nullptr) return parameter->targetVesselName;
    return Localizer::Format("#autoLOC_6001220");
}

}  // namespace StringUtilities
}  // namespace ksp::Contracts::Lite
