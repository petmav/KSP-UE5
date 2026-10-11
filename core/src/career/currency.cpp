// WP-36: Currency modifier queries (CurrencyModifierQuery.cs, ValueModifierQuery.cs), the CareerEnvironment seam and the numeric
// text helper.
#include "ksp/career/currency.hpp"

#include <limits>
#include <stdexcept>
#include "ksp/localization/localizer.hpp"

#include "ksp/career/career_environment.hpp"
#include "ksp/career/format.hpp"
#include "ksp/career/funding.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

using GE = events::GameEvents;

// ---- numeric text -------------------------------------------------------------------------------------------------------

namespace career {

namespace {
std::string Group(const std::string& intDigits) {
    std::string out;
    const int n = static_cast<int>(intDigits.size());
    for (int i = 0; i < n; ++i) {
        out += intDigits[static_cast<size_t>(i)];
        const int rest = n - 1 - i;
        if (rest > 0 && rest % 3 == 0) out += ',';
    }
    return out;
}
}  // namespace

std::string FormatNumber(double value, bool isFloat, std::string_view format) {
    auto fixedText = [&](int decimals) {
        return isFloat ? dotnet::FormatSingleF(static_cast<float>(value), decimals) : dotnet::FormatDoubleF(value, decimals);
    };
    if (format.empty()) return isFloat ? dotnet::FormatSingleG(static_cast<float>(value)) : dotnet::FormatDoubleG(value);
    const char k = format[0];
    auto digits = [&](int dflt) {
        if (format.size() == 1) return dflt;
        int n = 0;
        for (char c : format.substr(1)) {
            if (c < '0' || c > '9') throw std::logic_error("kspcore: unsupported numeric format \"" + std::string(format) + "\"");
            n = n * 10 + (c - '0');
        }
        return n;
    };
    if (k == 'G') {
        const int n = digits(0);
        return isFloat ? dotnet::FormatSingleG(static_cast<float>(value), n) : dotnet::FormatDoubleG(value, n);
    }
    if (k == 'F' || k == 'f') return fixedText(digits(2));
    if (k == 'N' || k == 'n') {
        std::string t = fixedText(digits(2));
        bool neg = false;
        if (!t.empty() && t[0] == '-') {
            neg = true;
            t.erase(0, 1);
        }
        const size_t dot = t.find('.');
        const std::string ip = dot == std::string::npos ? t : t.substr(0, dot);
        const std::string fp = dot == std::string::npos ? "" : t.substr(dot);
        return std::string(neg ? "-" : "") + Group(ip) + fp;
    }
    // custom pattern: [0#]+ ( . [0#]+ )?
    const size_t dot = format.find('.');
    const std::string_view ip = dot == std::string_view::npos ? format : format.substr(0, dot);
    const std::string_view fp = dot == std::string_view::npos ? std::string_view() : format.substr(dot + 1);
    int minInt = 0, minFrac = 0, maxFrac = 0;
    for (char c : ip) {
        if (c == '0') ++minInt;
        else if (c != '#') throw std::logic_error("kspcore: unsupported numeric format \"" + std::string(format) + "\"");
    }
    for (char c : fp) {
        if (c == '0') {
            ++minFrac;
            ++maxFrac;
        } else if (c == '#') {
            ++maxFrac;
        } else {
            throw std::logic_error("kspcore: unsupported numeric format \"" + std::string(format) + "\"");
        }
    }
    std::string t = fixedText(maxFrac);
    bool neg = false;
    if (!t.empty() && t[0] == '-') {
        neg = true;
        t.erase(0, 1);
    }
    const size_t d = t.find('.');
    std::string i = d == std::string::npos ? t : t.substr(0, d);
    std::string f = d == std::string::npos ? "" : t.substr(d + 1);
    while (static_cast<int>(f.size()) > minFrac && !f.empty() && f.back() == '0') f.pop_back();
    while (static_cast<int>(i.size()) < minInt) i.insert(i.begin(), '0');
    if (minInt == 0 && i == "0") i.clear();
    std::string out = i;
    if (!f.empty()) out += "." + f;
    if (out.empty()) out = "0";
    if (neg) {
        bool allZero = true;
        for (char c : out)
            if (c != '0' && c != '.') allZero = false;
        if (!allZero) out.insert(out.begin(), '-');
    }
    return out;
}

// KSPUtil.cs:858
std::string LocalizeNumber(double value, std::string_view format) {
    if (value == std::numeric_limits<double>::infinity()) return KSP::Localization::Localizer::Format("#autoLOC_462439");
    return KSP::Localization::Localizer::Format("<<1>>", {FormatNumber(value, false, format)});
}
// KSPUtil.cs:867
std::string LocalizeNumber(float value, std::string_view format) {
    return KSP::Localization::Localizer::Format("<<1>>", {FormatNumber(static_cast<double>(value), true, format)});
}

}  // namespace career

// ---- CareerEnvironment --------------------------------------------------------------------------------------------------

namespace {
CareerEnvironment& EnvRef() {
    static CareerEnvironment env;
    return env;
}
}  // namespace

CareerEnvironment& CareerEnvironment::Current() { return EnvRef(); }

void CareerEnvironment::Reset() {
    CareerEnvironment& e = EnvRef();
    e = CareerEnvironment();
}

CoroutineScheduler& CareerEnvironment::Scheduler() { return scheduler != nullptr ? *scheduler : own_; }

// FlightGlobals.GetHomeBody (FlightGlobals.cs): the body with isHomeWorld
CelestialBody* CareerEnvironment::HomeBody() {
    if (getHomeBody) return getHomeBody();
    for (CelestialBody* b : FlightRegistry::Bodies())
        if (b != nullptr && b->isHomeWorld) return b;
    return nullptr;
}

// ---- ValueModifierQuery ---------------------------------------------------------------------------------------------------

namespace {
using TS = ValueModifierQuery::TextStyling;
// the shared tail of GetEffectDeltaText: `plus` / `minus` open tags and the close tag per styling
std::string Styled(TS style, float v, const std::string& text, const char* pctSuffix, bool percentageLessIsGoodQuirk) {
    const bool pos = v > 0.0f;
    switch (style) {
        case TS::OnGUI:
            return std::string(pos ? "<color=#caff00>(+" : "<color=#feb200>(") + text + pctSuffix + ")</color>";
        case TS::EzGUIRichText:
            return std::string(pos ? "<#caff00>(+" : "<#feb200>(") + text + pctSuffix + ")</>";
        case TS::OnGUI_LessIsGood:
            (void)percentageLessIsGoodQuirk;
            return std::string(pos ? "<color=#feb200>(+" : "<color=#caff00>(") + text + pctSuffix + ")</color>";
        case TS::EzGUIRichText_LessIsGood:
            return std::string(pos ? "<#feb200>(+" : "<#caff00>(") + text + pctSuffix + ")</>";
        default:
            return std::string(pos ? "(+" : "(") + text + pctSuffix + ")";
    }
}
}  // namespace

// ValueModifierQuery.cs:20
ValueModifierQuery::ValueModifierQuery(std::string valueId, float v0) : valueId_(std::move(valueId)), input_(v0), delta_(0.0f) {}

// ValueModifierQuery.cs:30
void ValueModifierQuery::AddDelta(float val) { delta_ = f32store(fx(delta_) + fx(val)); }

// ValueModifierQuery.cs:40
std::string ValueModifierQuery::GetEffectDeltaText(const std::string& format, TextStyling textStyle) const {
    if (delta_ == 0.0f) return "";
    const std::string text = career::FormatNumber(delta_, format);
    return Styled(textStyle, delta_, text, "", false);
}

// ValueModifierQuery.cs:64 (sic: the EzGUIRichText_LessIsGood branch puts the ')' before the '%')
std::string ValueModifierQuery::GetEffectPercentageText(const std::string& format, TextStyling textStyle) const {
    if (delta_ == 0.0f) return "";
    const float pct = f32store(fx(delta_) / fx(input_) * 100.0);
    const std::string text = career::FormatNumber(pct, format);
    const bool pos = delta_ > 0.0f;
    switch (textStyle) {
        case TS::OnGUI: return std::string(pos ? "<color=#caff00>(+" : "<color=#feb200>(") + text + "%)</color>";
        case TS::EzGUIRichText: return std::string(pos ? "<#caff00>(+" : "<#feb200>(") + text + "%)</>";
        case TS::OnGUI_LessIsGood: return std::string(pos ? "<color=#feb200>(+" : "<color=#caff00>(") + text + "%)</color>";
        case TS::EzGUIRichText_LessIsGood: return std::string(pos ? "<#feb200>(+" : "<#caff00>(") + text + ")%</>";
        default: return std::string(pos ? "(+" : "(") + text + "%)";
    }
}

// ValueModifierQuery.cs:88
ValueModifierQuery ValueModifierQuery::RunQuery(const std::string& valueId, float input) {
    ValueModifierQuery q(valueId, input);
    GE::Modifiers.onValueModifierQuery.Fire(&q);
    return q;
}

// ---- CurrencyModifierQuery ------------------------------------------------------------------------------------------------

// CurrencyModifierQuery.cs:29
CurrencyModifierQuery::CurrencyModifierQuery(TransactionReasons reason_, float f0, float s0, float r0)
    : reason(reason_), inputFunds_(f0), inputScience_(s0), inputRep_(r0), deltaFunds_(0.0f), deltaScience_(0.0f), deltaRep_(0.0f) {}

// CurrencyModifierQuery.cs:40
float CurrencyModifierQuery::GetInput(Currency c) const {
    switch (c) {
        case Currency::Funds: return inputFunds_;
        case Currency::Science: return inputScience_;
        case Currency::Reputation: return inputRep_;
        default: return 0.0f;
    }
}

// CurrencyModifierQuery.cs:55
void CurrencyModifierQuery::AddDelta(Currency c, float val) {
    switch (c) {
        case Currency::Funds: deltaFunds_ = f32store(fx(deltaFunds_) + fx(val)); break;
        case Currency::Science: deltaScience_ = f32store(fx(deltaScience_) + fx(val)); break;
        case Currency::Reputation: deltaRep_ = f32store(fx(deltaRep_) + fx(val)); break;
    }
}

// CurrencyModifierQuery.cs:71
float CurrencyModifierQuery::GetEffectDelta(Currency c) const {
    switch (c) {
        case Currency::Funds: return deltaFunds_;
        case Currency::Science: return deltaScience_;
        case Currency::Reputation: return deltaRep_;
        default: return 0.0f;
    }
}

// CurrencyModifierQuery.cs:86
float CurrencyModifierQuery::GetTotal(Currency c) const { return f32store(fx(GetInput(c)) + fx(GetEffectDelta(c))); }

// CurrencyModifierQuery.cs:91
std::string CurrencyModifierQuery::GetCostLine(bool displayInverted, bool useCurrencyColors, bool useInsufficientCurrencyColors,
                                               bool includePercentage, const std::string& seperator) const {
    float num = GetTotal(Currency::Funds);
    float num2 = GetTotal(Currency::Science);
    float num3 = GetTotal(Currency::Reputation);
    if (displayInverted) {
        num = -num;
        num2 = -num2;
        num3 = -num3;
    }
    const bool flag = CanAfford(Currency::Funds);
    const bool flag2 = CanAfford(Currency::Science);
    const bool flag3 = CanAfford(Currency::Reputation);
    const std::string text = !Mathf::Approximately(num, 0.0f)
                                 ? "<sprite=\"CurrencySpriteAsset\" name=\"Funds\" tint=1> " + career::LocalizeNumber(num, "N2")
                                 : "";
    const std::string text2 = !Mathf::Approximately(num2, 0.0f)
                                  ? "<sprite=\"CurrencySpriteAsset\" name=\"Science\" tint=1> " + career::LocalizeNumber(num2, "N0")
                                  : "";
    const std::string text3 = !Mathf::Approximately(num3, 0.0f)
                                  ? "<sprite=\"CurrencySpriteAsset\" name=\"Reputation\" tint=1> " + career::LocalizeNumber(num3, "F2")
                                  : "";
    const char* brightOrange = "#ff5b00";  // XKCDColors.HexFormat.BrightOrange
    auto colored = [&](const std::string& body, bool afford, const char* ok) {
        const bool wrap = (useInsufficientCurrencyColors && !afford) || useCurrencyColors;
        if (!wrap) return body;
        return std::string("<color=") + ((!useInsufficientCurrencyColors || afford) ? ok : brightOrange) + ">" + body + "</color>";
    };
    std::string text4;
    if (!text.empty()) {
        text4 = colored(text, flag, "#B4D455");
        if (includePercentage && deltaFunds_ != 0.0f)
            text4 += " " + GetEffectPercentageText(Currency::Funds, "N1", TextStyling::OnGUI_LessIsGood);
    }
    if (!text2.empty()) {
        if (!text4.empty()) text4 += seperator;
        text4 += colored(text2, flag2, "#6DCFF6");
        if (includePercentage && deltaScience_ != 0.0f)
            text4 += " " + GetEffectPercentageText(Currency::Science, "N1", TextStyling::OnGUI_LessIsGood);
    }
    if (!text3.empty()) {
        if (!text4.empty()) text4 += seperator;
        text4 += colored(text3, flag3, "#E0D503");
        if (includePercentage && deltaRep_ != 0.0f)
            text4 += " " + GetEffectPercentageText(Currency::Reputation, "N1", TextStyling::OnGUI_LessIsGood);
    }
    return text4;
}

// CurrencyModifierQuery.cs:144
bool CurrencyModifierQuery::CanAfford(const std::function<void(Currency)>& onInsufficientCurrency) const {
    const bool flag = CanAfford(Currency::Funds);
    const bool flag2 = flag && CanAfford(Currency::Science);
    const bool flag3 = flag2 && CanAfford(Currency::Reputation);
    if (onInsufficientCurrency && !flag3) {
        if (!flag) onInsufficientCurrency(Currency::Funds);
        else if (!flag2) onInsufficientCurrency(Currency::Science);
        else if (!flag3) onInsufficientCurrency(Currency::Reputation);
    }
    return flag3;
}

// CurrencyModifierQuery.cs:167
bool CurrencyModifierQuery::CanAfford(Currency c) const {
    const float num = -(f32store(fx(GetInput(c)) + fx(GetEffectDelta(c))));
    if (Mathf::Approximately(num, 0.0f)) return true;
    CareerEnvironment& env = CareerEnvironment::Current();
    switch (c) {
        case Currency::Funds:
            if (Funding::Instance != nullptr) return static_cast<double>(num) <= Funding::Instance->Funds();
            return true;
        case Currency::Science:
            if (env.rndPresent && env.rndPresent()) {
                const float science = env.rndScience ? env.rndScience() : 0.0f;
                return UtilMath::RoundToPlaces(science, 1) >= UtilMath::RoundToPlaces(num, 1);
            }
            return true;
        case Currency::Reputation: return true;
        default: return true;
    }
}

// CurrencyModifierQuery.cs:195
std::string CurrencyModifierQuery::GetEffectDeltaText(Currency c, const std::string& format, TextStyling textStyle) const {
    float num = 0.0f;
    switch (c) {
        case Currency::Funds: num = deltaFunds_; break;
        case Currency::Science: num = deltaScience_; break;
        case Currency::Reputation: num = deltaRep_; break;
    }
    if (num == 0.0f) return "";
    return Styled(textStyle, num, career::FormatNumber(num, format), "", false);
}

// CurrencyModifierQuery.cs:231 (sic: OnGUI_LessIsGood uses "<+" / "<" and "%></color>")
std::string CurrencyModifierQuery::GetEffectPercentageText(Currency c, const std::string& format, TextStyling textStyle) const {
    float num = 0.0f, num2 = 0.0f, num3 = 0.0f;
    switch (c) {
        case Currency::Funds:
            num = deltaFunds_;
            num2 = inputFunds_;
            num3 = f32store(fx(deltaFunds_) / fx(inputFunds_) * 100.0);
            break;
        case Currency::Science:
            num = deltaScience_;
            num2 = inputScience_;
            num3 = f32store(fx(deltaScience_) / fx(inputScience_) * 100.0);
            break;
        case Currency::Reputation:
            num = deltaRep_;
            num2 = inputRep_;
            num3 = f32store(fx(deltaRep_) / fx(inputRep_) * 100.0);
            break;
    }
    if (num != 0.0f && num2 != 0.0f) {
        const std::string text = career::FormatNumber(num3, format);
        const bool pos = num > 0.0f;
        switch (textStyle) {
            case TextStyling::OnGUI: return std::string(pos ? "<color=#caff00>(+" : "<color=#feb200>(") + text + "%)</color>";
            case TextStyling::EzGUIRichText: return std::string(pos ? "<#caff00>(+" : "<#feb200>(") + text + "%)</>";
            case TextStyling::OnGUI_LessIsGood: return std::string(pos ? "<color=#feb200><+" : "<color=#caff00><") + text + "%></color>";
            case TextStyling::EzGUIRichText_LessIsGood: return std::string(pos ? "<#feb200>(+" : "<#caff00>(") + text + ")%</>";
            default: return std::string(pos ? "(+" : "(") + text + "%)";
        }
    }
    return "";
}

// CurrencyModifierQuery.cs:275
CurrencyModifierQuery CurrencyModifierQuery::RunQuery(TransactionReasons reason, float f0, float s0, float r0) {
    CurrencyModifierQuery q(reason, f0, s0, r0);
    GE::Modifiers.OnCurrencyModifierQuery.Fire(&q);
    return q;
}

}  // namespace ksp
