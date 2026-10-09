// Port of Assembly-CSharp/KSP/Localization/Localizer.cs (tag table + Format) and the managed fallback of
// Assembly-CSharp-firstpass/Lingoona/Grammar.cs useGrammar.
#include "ksp/localization/localizer.hpp"

#include <algorithm>
#include <memory>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace Lingoona {

namespace {

bool IsAsciiDigit(char c) { return c >= '0' && c <= '9'; }
bool IsLowerOrUnderscore(char c) { return (c >= 'a' && c <= 'z') || c == '_'; }

struct GrammarMatch {
    size_t index = 0;   // Groups[0].Index
    size_t length = 0;  // Groups[0].Length
    std::string group1;
};

// Regex("<<(?:[^>]+:)?([a-z_]+|\\d+)>>").Match(s): leftmost match with .NET backtracking order (the optional group is
// greedy: the longest "[^>]+:" prefix is tried first, then shorter ones, then none). \d is ASCII here (KSP's
// int.Parse would reject other Unicode digits anyway).
bool MatchPlaceholder(const std::string& s, GrammarMatch& m) {
    for (size_t p = s.find("<<"); p != std::string::npos; p = s.find("<<", p + 1)) {
        const size_t q = p + 2;
        size_t r = s.find('>', q);
        if (r == std::string::npos) r = s.size();
        // candidate starts of the capture: after each ':' in s[q+1..r) (longest first), then q itself
        std::vector<size_t> starts;
        for (size_t c = r; c-- > q + 1;)
            if (s[c] == ':') starts.push_back(c + 1);
        starts.push_back(q);
        for (size_t t : starts) {
            size_t e = t;
            while (e < s.size() && IsLowerOrUnderscore(s[e])) ++e;
            if (e == t) {
                while (e < s.size() && IsAsciiDigit(s[e])) ++e;
            }
            if (e > t && s.compare(e, 2, ">>") == 0) {
                m.index = p;
                m.length = e + 2 - p;
                m.group1 = s.substr(t, e - t);
                return true;
            }
        }
    }
    return false;
}

}  // namespace

// Grammar.cs:53 (managed branch)
std::string Grammar::useGrammar(std::string templ, const std::vector<std::string>& parameters) {
    GrammarMatch match;
    while (MatchPlaceholder(templ, match)) {
        const char c = match.group1[0];
        if (c >= '0' && c <= '9') {
            const int num = dotnet::ParseInt32(match.group1);
            templ.erase(match.index, match.length);
            if (num > 0 && static_cast<size_t>(num) <= parameters.size()) {
                std::string text = parameters[static_cast<size_t>(num - 1)];
                const size_t num2 = text.find('^');
                if (num2 != std::string::npos) text.erase(num2);
                templ.insert(match.index, text);
            }
        } else {
            templ.erase(match.index, 2);
            templ.erase(match.index + match.length - 4, 2);
        }
    }
    return templ;
}

// Grammar.cs:84 (managed branch)
std::string Grammar::useGrammar(std::string templ) {
    GrammarMatch match;
    while (MatchPlaceholder(templ, match)) {
        const char c = match.group1[0];
        if (c < '0' || c > '9') {
            templ.erase(match.index, 2);
            templ.erase(match.index + match.length - 4, 2);
        } else {
            templ.erase(match.index, match.length);
        }
    }
    return templ;
}

}  // namespace Lingoona

namespace KSP::Localization {

namespace {
std::unique_ptr<Localizer>& InstancePtr() {
    static std::unique_ptr<Localizer> instance;
    return instance;
}

// .NET string.Replace(string, string) (ordinal)
std::string ReplaceAll(std::string s, std::string_view from, std::string_view to) {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

bool IsUpperHex(char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'); }
}  // namespace

Localizer* Localizer::Instance() { return InstancePtr().get(); }

// Localizer.cs:111 (LanguageIds / buildID64.txt / SwitchToLanguage / GameSettings flags: WP-51)
Localizer& Localizer::Init() {
    auto& p = InstancePtr();
    if (!p) p.reset(new Localizer());
    return *p;
}

void Localizer::Shutdown() { InstancePtr().reset(); }

Localizer& Localizer::InitWithBuiltInEnglish() {
    Localizer& l = Init();
    // GameData/Squad/Localization/dictionary.cfg, en-us (KSP 1.12.5): the tags WP-03 code formats
    static const std::pair<const char*, const char*> kEnglish[] = {
        {"#autoLOC_7003243", "y"},       {"#autoLOC_7003244", "d"},       {"#autoLOC_7003272", "N"},
        {"#autoLOC_7003273", "S"},       {"#autoLOC_7003274", "E"},       {"#autoLOC_7003275", "W"},
        {"#autoLOC_6002317", "s"},       {"#autoLOC_6002318", "m"},       {"#autoLOC_6002319", "h"},
        {"#autoLOC_6002320", "d"},       {"#autoLOC_6002321", "y"},       {"#autoLOC_6002322", "Year"},
        {"#autoLOC_6002323", "Years"},   {"#autoLOC_6002324", "Day"},     {"#autoLOC_6002325", "Days"},
        {"#autoLOC_6002326", "Hour"},    {"#autoLOC_6002327", "Hours"},   {"#autoLOC_6002328", "Min"},
        {"#autoLOC_6002329", "Mins"},    {"#autoLOC_6002330", "Sec"},     {"#autoLOC_6002331", "Secs"},
        {"#autoLOC_6002332", "T-"},      {"#autoLOC_6002333", "T+"},      {"#autoLOC_6002334", "year"},
        {"#autoLOC_6002335", "years"},   {"#autoLOC_6002336", "days"},    {"#autoLOC_6002337", "day"},
        {"#autoLOC_6002338", "hour"},    {"#autoLOC_6002339", "hours"},   {"#autoLOC_6002340", "minutes"},
        {"#autoLOC_6002341", "minute"},  {"#autoLOC_6002342", "seconds"}, {"#autoLOC_6002343", "second"},
        {"#autoLOC_6002344", "Y"},       {"#autoLOC_6002345", "D"},       {"#autoLOC_7003005", "Year <<1>>, Day <<2>>"},
        {"#autoLOC_462439", "Infinity"},
    };
    for (const auto& [tag, value] : kEnglish) l.SetTagValue(tag, value);
    if (l.currentLanguage_.empty()) l.currentLanguage_ = "en-us";
    KSPUtil::CacheLocalStrings();
    return l;
}

// Localizer.cs:154
std::string Localizer::Format(std::string_view templ) {
    Localizer* inst = Instance();
    if (inst == nullptr) return "";
    return inst->_Format(templ, {});
}

// Localizer.cs:139 / 144
std::string Localizer::Format(std::string_view templ, const std::vector<std::string>& list) {
    Localizer* inst = Instance();
    if (inst == nullptr) throw NullReferenceException();
    return inst->_Format(templ, list);
}

// Localizer.cs:134
std::string Localizer::GetStringByTag(const std::string& tag) {
    Localizer* inst = Instance();
    if (inst == nullptr) throw NullReferenceException();
    auto it = inst->tagValues_.find(tag);
    if (it == inst->tagValues_.end()) throw KeyNotFoundException("The given key was not present in the dictionary.");
    return it->second;
}

// Localizer.cs:164
bool Localizer::TryGetStringByTag(const std::string& tagName, std::string& value) {
    Localizer* inst = Instance();
    if (inst == nullptr) throw NullReferenceException();
    auto it = inst->tagValues_.find(tagName);
    if (it == inst->tagValues_.end()) {
        value.clear();
        return false;
    }
    value = it->second;
    return true;
}

// Localizer.cs:169
void Localizer::TranslateBranch(ConfigNode& branchRoot) {
    for (ConfigNode* n : branchRoot.GetNodes()) TranslateBranch(*n);
    auto& values = branchRoot.values();
    for (int j = 0; j < values.Count(); ++j) values[j].value = Format(values[j].value);
}

int Localizer::TagsLength() {
    Localizer* inst = Instance();
    if (inst == nullptr) throw NullReferenceException();
    return static_cast<int>(inst->tagValues_.size());
}

std::string Localizer::CurrentLanguage() {
    Localizer* inst = Instance();
    if (inst == nullptr) throw NullReferenceException();
    return inst->currentLanguage_;
}

bool Localizer::ShowKeysOnScreen() {
    Localizer* inst = Instance();
    return inst != nullptr && inst->showKeysInGame_;
}

void Localizer::SetShowKeysOnScreen(bool v) {
    Localizer* inst = Instance();
    if (inst == nullptr) throw NullReferenceException();
    inst->showKeysInGame_ = v;
}

void Localizer::SetDebugWriteMissingKeysToLog(bool v) {
    Localizer* inst = Instance();
    if (inst == nullptr) throw NullReferenceException();
    inst->debugWriteMissingKeysToLog_ = v;
}

// Localizer.cs:183
std::string Localizer::_Format(std::string_view templ, const std::vector<std::string>& parameterList) {
    if (ShowKeysOnScreen()) return std::string(templ);
    std::string result;
    if (templ.empty()) return result;
    result = !parameterList.empty() ? FormatWithLingoona(templ, parameterList) : ReplaceSingleTagIfFound(templ);
    result = unescapeFormattedString(result);
    if (debugWriteMissingKeysToLog_ && result.find(missingKeyPrefix_) == std::string::npos) {
        std::string lower = result;
        for (char& ch : lower)
            if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
        if (lower.find("#autoloc") != std::string::npos &&
            std::find(missingKeysList_.begin(), missingKeysList_.end(), result) == missingKeysList_.end()) {
            missingKeysList_.push_back(result);
            log::Warning(missingKeyPrefix_ + " " + result);
        }
    }
    return result;
}

// Localizer.cs:204 (parameters that are tags are translated first: ReplaceTags)
std::string Localizer::FormatWithLingoona(std::string_view templ, const std::vector<std::string>& parameterList) {
    std::string t = ReplaceSingleTagIfFound(templ);
    std::vector<std::string> parameters;
    parameters.reserve(parameterList.size());
    for (const auto& p : parameterList) parameters.push_back(ReplaceSingleTagIfFound(p));
    return Lingoona::Grammar::useGrammar(std::move(t), parameters);
}

// Localizer.cs:211
std::string Localizer::ReplaceSingleTagIfFound(std::string_view tag) const {
    if (tag.empty()) return "";
    auto it = tagValues_.find(std::string(tag));
    if (it != tagValues_.end()) return it->second;
    return std::string(tag);
}

// Localizer.cs:436
void Localizer::AddTagValuesForLanguage(std::span<ConfigNode* const> localizationNodes, const std::string& lang) {
    size_t num = localizationNodes.size();
    while (num-- > 0) {
        const auto nodes = localizationNodes[num]->GetNodes(lang);
        size_t num2 = nodes.size();
        while (num2-- > 0) AddTagValuesFromNode(*nodes[num2]);
    }
}

// Localizer.cs:470
void Localizer::AddTagValuesFromNode(const ConfigNode& node) {
    const auto& values = node.values();
    int count = values.Count();
    while (count-- > 0) {
        const ConfigNode::Value& value = values[count];
        tagValues_[value.name] = unescapeStringToLingoona(value.value);
    }
}

void Localizer::SetTagValue(const std::string& tag, const std::string& value) {
    tagValues_[tag] = unescapeStringToLingoona(value);
}

// Localizer.cs:481: "｢" -> "{", "｣" -> "}", then \uXXXX / \UXXXX (uppercase hex) -> that UTF-16 code unit
// (a lone surrogate cannot be represented in UTF-8 and becomes U+FFFD; a high+low escape pair is combined).
std::string Localizer::unescapeStringToLingoona(std::string_view originalString) {
    std::string text = ReplaceAll(std::string(originalString), "\xEF\xBD\xA2", "{");
    text = ReplaceAll(std::move(text), "\xEF\xBD\xA3", "}");
    std::string out;
    out.reserve(text.size());
    auto hex4 = [&](size_t i, unsigned& v) {
        if (i + 6 > text.size() || text[i] != '\\' || (text[i + 1] != 'u' && text[i + 1] != 'U')) return false;
        v = 0;
        for (size_t k = i + 2; k < i + 6; ++k) {
            const char c = text[k];
            if (!IsUpperHex(c)) return false;
            v = v * 16 + static_cast<unsigned>(c <= '9' ? c - '0' : c - 'A' + 10);
        }
        return true;
    };
    for (size_t i = 0; i < text.size();) {
        unsigned v;
        if (hex4(i, v)) {
            unsigned lo;
            if (v >= 0xD800 && v <= 0xDBFF && hex4(i + 6, lo) && lo >= 0xDC00 && lo <= 0xDFFF) {
                dotnet::AppendUtf8(out, static_cast<char32_t>(0x10000 + ((v - 0xD800) << 10) + (lo - 0xDC00)));
                i += 12;
                continue;
            }
            dotnet::AppendUtf8(out, (v >= 0xD800 && v <= 0xDFFF) ? U'�' : static_cast<char32_t>(v));
            i += 6;
            continue;
        }
        out.push_back(text[i]);
        ++i;
    }
    return out;
}

// Localizer.cs:489
std::string Localizer::unescapeFormattedString(std::string_view formattedString) {
    std::string s = ReplaceAll(std::string(formattedString), "\\n", "\n");
    s = ReplaceAll(std::move(s), "\\\"", "\"");
    return ReplaceAll(std::move(s), "\\t", "\t");
}

}  // namespace KSP::Localization
}  // namespace ksp
