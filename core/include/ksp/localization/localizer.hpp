// KSP.Localization.Localizer (Assembly-CSharp/KSP/Localization/Localizer.cs): the tag table and Format, plus the
// managed fallback of Lingoona.Grammar.useGrammar (Assembly-CSharp-firstpass/Lingoona/Grammar.cs:53/84).
//
// Ported here (WP-03 needs it for KSPUtil's time/date strings): Instance, tag table, Format(template),
// Format(template, params), GetStringByTag, TryGetStringByTag, TranslateBranch, _Format, FormatWithLingoona,
// ReplaceSingleTagIfFound, ReplaceTags, AddTagValuesFromNode, AddTagValuesForLanguage (fed with the Localization nodes
// instead of GameDatabase), unescapeStringToLingoona, unescapeFormattedString, ShowKeysOnScreen, missing-key logging.
// Not ported (WP-51 localization): Init's language-file lookup (buildID64.txt), SwitchToLanguage's GameDatabase scan and
// CacheLocalStrings fan-out to UI classes, mission tags, the native Lingoona grammar engine (lingoona_grammar_kerbal.dll).
//
// Grammar: KSP calls the native Lingoona engine when the DLL is present and falls back to a managed regex otherwise.
// kspcore implements the managed fallback exactly: "<<N>>" -> parameter N with everything from the first '^' removed
// (gender/plural markers); "<<name>>" / "<<X:name>>" with name in [a-z_]+ -> the brackets are removed; a number outside
// 1..count removes the placeholder. For plain "<<N>>" English templates the native engine gives the same text.
#pragma once

#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ksp/config/config_node.hpp"

namespace ksp {

namespace Lingoona {
// Grammar.useGrammar managed fallback (the native library is not used by kspcore)
struct Grammar {
    static std::string useGrammar(std::string templ, const std::vector<std::string>& parameters);  // Grammar.cs:53
    static std::string useGrammar(std::string templ);                                              // Grammar.cs:84
};
}  // namespace Lingoona

namespace KSP::Localization {

class Localizer {
public:
    // Localizer.Instance (null until Init)
    static Localizer* Instance();
    // Localizer.Init (Localizer.cs:111) minus the language-file and GameDatabase parts: creates the instance with an
    // empty tag table (fill it with AddTagValuesForLanguage / AddTagValuesFromNode / SetTagValue).
    static Localizer& Init();
    // kspcore: destroy the instance (tests; KSP never does).
    static void Shutdown();
    // kspcore: Init + the en-us strings of the tags used by WP-03 code (KSPUtil time/date/coordinate units, copied from
    // GameData/Squad/Localization/dictionary.cfg) + KSPUtil.CacheLocalStrings(). For tools and tests that have no
    // GameData; the game loads the full dictionary.
    static Localizer& InitWithBuiltInEnglish();

    // Localizer.cs:154 Format(template): "" when there is no instance
    static std::string Format(std::string_view templ);
    // Localizer.cs:139/144 Format(template, params): NullReferenceException when there is no instance. Callers convert
    // object arguments with their C# ToString() (see ksp::dotnet number formatting).
    static std::string Format(std::string_view templ, const std::vector<std::string>& list);
    // Localizer.cs:134 (KeyNotFoundException for an unknown tag) / Localizer.cs:164
    static std::string GetStringByTag(const std::string& tag);
    static bool TryGetStringByTag(const std::string& tagName, std::string& value);
    // Localizer.cs:169: every value of the tree is replaced by Format(value)
    static void TranslateBranch(ConfigNode& branchRoot);

    static int TagsLength();
    static std::string CurrentLanguage();
    // internal static ShowKeysOnScreen: Format returns the template unchanged (getter false without an instance)
    static bool ShowKeysOnScreen();
    static void SetShowKeysOnScreen(bool v);
    // internal static debugWriteMissingKeysToLog
    static void SetDebugWriteMissingKeysToLog(bool v);

    // Localizer.cs:436 AddTagValuesForLanguage, with GameDatabase.Instance.GetConfigNodes("Localization") passed in:
    // the nodes are processed from the last to the first, and each node's `lang` children from the last to the first.
    void AddTagValuesForLanguage(std::span<ConfigNode* const> localizationNodes, const std::string& lang);
    // Localizer.cs:470 (values processed from the last to the first; a later Add overwrites)
    void AddTagValuesFromNode(const ConfigNode& node);
    // kspcore: set one tag (the value goes through unescapeStringToLingoona like a dictionary entry)
    void SetTagValue(const std::string& tag, const std::string& value);
    void SetCurrentLanguage(std::string lang) { currentLanguage_ = std::move(lang); }

    // Localizer.cs:481 / 489
    static std::string unescapeStringToLingoona(std::string_view originalString);
    static std::string unescapeFormattedString(std::string_view formattedString);

private:
    Localizer() = default;
    std::string _Format(std::string_view templ, const std::vector<std::string>& parameterList);
    std::string FormatWithLingoona(std::string_view templ, const std::vector<std::string>& parameterList);
    std::string ReplaceSingleTagIfFound(std::string_view tag) const;

    std::unordered_map<std::string, std::string> tagValues_;
    std::string currentLanguage_;
    bool showKeysInGame_ = false;
    bool debugWriteMissingKeysToLog_ = false;
    std::vector<std::string> missingKeysList_;  // static in C#
    std::string missingKeyPrefix_ = "Localization.Format Missed Key in:";
};

}  // namespace KSP::Localization

using KSP::Localization::Localizer;

}  // namespace ksp
