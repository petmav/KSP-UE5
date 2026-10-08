// Port of the engine-independent parts of Assembly-CSharp/KSPUtil.cs and IDateFormatterExtensions.cs.
#include "ksp/util/ksp_util.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <system_error>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/utf16.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/unity/mathf.hpp"

namespace ksp {

namespace {

// C# unchecked int arithmetic
int WrapAdd(int a, int b) { return static_cast<int>(static_cast<uint32_t>(a) + static_cast<uint32_t>(b)); }
int WrapSub(int a, int b) { return static_cast<int>(static_cast<uint32_t>(a) - static_cast<uint32_t>(b)); }
int WrapMul(int a, int b) { return static_cast<int>(static_cast<uint32_t>(a) * static_cast<uint32_t>(b)); }

// Math.Abs(int) / Mathf.Abs(int): OverflowException for int.MinValue
int AbsInt(int v) {
    if (v == std::numeric_limits<int>::min())
        throw dotnet::OverflowException("Negating the minimum value of a twos complement number is invalid.");
    return v < 0 ? -v : v;
}

std::string IntStr(int v) { return dotnet::FormatInt64(v); }

// string.Substring(start, length) with .NET's ArgumentOutOfRangeException -> std::out_of_range
std::string Substring(std::string_view s, long long start, long long length) {
    if (start < 0 || length < 0 || start + length > static_cast<long long>(s.size()))
        throw std::out_of_range("Index and length must refer to a location within the string.");
    return std::string(s.substr(static_cast<size_t>(start), static_cast<size_t>(length)));
}

// string.Split(char) (empty entries kept)
std::vector<std::string> SplitChar(std::string_view s, char sep) {
    std::vector<std::string> out;
    size_t start = 0;
    while (true) {
        const size_t p = s.find(sep, start);
        if (p == std::string_view::npos) {
            out.emplace_back(s.substr(start));
            break;
        }
        out.emplace_back(s.substr(start, p - start));
        start = p + 1;
    }
    return out;
}

const std::string& At(const std::vector<std::string>& v, size_t i) {
    if (i >= v.size()) throw std::out_of_range("Index was outside the bounds of the array.");
    return v[i];
}

std::string ReplaceChar(std::string s, char from, char to) {
    for (char& c : s)
        if (c == from) c = to;
    return s;
}

std::string AsciiLower(std::string_view s) {
    std::string r(s);
    for (char& c : r)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return r;
}

// KSPUtil.cs:683-751 cacheAutoLOC_* (null until CacheLocalStrings: "")
struct LocalStrings {
    std::string s7003243, s7003244, s7003272, s7003273, s7003274, s7003275;
    std::string s6002317, s6002318, s6002319, s6002320, s6002321, s6002322, s6002323, s6002324, s6002325, s6002326,
        s6002327, s6002328, s6002329, s6002330, s6002331, s6002332, s6002333, s6002334, s6002335, s6002336, s6002337,
        s6002338, s6002339, s6002340, s6002341, s6002342, s6002343, s6002344, s6002345;
};
LocalStrings& L() {
    static LocalStrings l;
    return l;
}

// DefaultDateTimeFormatter statics (KSPUtil.cs:43-47): shared by every formatter instance
int dateHolder[5];
int absDateHolder[5];
double secondsUTHolder = 0.0;

constexpr int KerbinDay = 21600;
constexpr int KerbinYear = 9201600;
constexpr int EarthDay = 86400;
constexpr int EarthYear = 31536000;

// KSPUtil.cs:109
std::string& AddUnits(std::string& sb, int val, const std::string& singular, const std::string& plural) {
    sb += IntStr(val);
    sb += (val == 1) ? singular : plural;
    return sb;
}

// KSPUtil.cs:116
const char* IsBadNum(double time) {
    if (std::isnan(time)) return "NaN";
    if (time == std::numeric_limits<double>::infinity()) return "+Inf";
    if (time == -std::numeric_limits<double>::infinity()) return "-Inf";
    return nullptr;
}

// KSPUtil.cs:295
void get_date_from_UT(double time, int year_len, int day_len) {
    secondsUTHolder = std::floor(time);
    const int num = dotnet::ConvToInt32(time / static_cast<double>(year_len));
    time -= static_cast<double>(num) * static_cast<double>(year_len);
    const int num2 = dotnet::ConvToInt32(time);
    const int num3 = num2 / 60 % 60;
    const int num4 = num2 / 3600 % (day_len / 3600);
    const int num5 = num2 / day_len;
    absDateHolder[0] = AbsInt(dateHolder[0] = num2 % 60);
    absDateHolder[1] = AbsInt(dateHolder[1] = num3);
    absDateHolder[2] = AbsInt(dateHolder[2] = num4);
    absDateHolder[3] = AbsInt(dateHolder[3] = num5);
    absDateHolder[4] = AbsInt(dateHolder[4] = num);
}

// KSPUtil.cs:283
void GetDateFromUT(double time) {
    if (GameSettings::KERBIN_TIME) get_date_from_UT(time, KerbinYear, KerbinDay);
    else get_date_from_UT(time, EarthYear, EarthDay);
}

// KSPUtil.cs:639
void AppendTimeToStringBuilder(std::string& sb, bool displaySeconds) {
    if (dateHolder[2] < 0 || dateHolder[1] < 0 || dateHolder[0] < 0) sb += " - ";
    int num = 5;
    while (num-- > 0) absDateHolder[num] = AbsInt(dateHolder[num]);
    if (absDateHolder[2] < 10) sb += "0";
    sb += IntStr(absDateHolder[2]);
    sb += ":";
    if (absDateHolder[1] < 10) sb += "0";
    sb += IntStr(absDateHolder[1]);
    if (displaySeconds) {
        sb += ":";
        if (absDateHolder[0] < 10) sb += "0";
        sb += IntStr(absDateHolder[0]);
    }
}

std::shared_ptr<IDateTimeFormatter>& FormatterSlot() {
    static std::shared_ptr<IDateTimeFormatter> f;
    return f;
}

// value.ToString(format) for the standard formats ksp::dotnet implements
std::string FormatWith(double value, std::string_view format, bool isFloat) {
    auto digits = [&](std::string_view f, int dflt) {
        if (f.size() == 1) return dflt;
        int n = 0;
        for (char c : f.substr(1)) {
            if (c < '0' || c > '9') throw std::logic_error("kspcore: unsupported numeric format \"" + std::string(format) + "\"");
            n = n * 10 + (c - '0');
        }
        return n;
    };
    if (format.empty()) return isFloat ? dotnet::FormatSingleG(static_cast<float>(value)) : dotnet::FormatDoubleG(value);
    const char k = format[0];
    if (k == 'G') {
        const int n = digits(format, 0);
        return isFloat ? dotnet::FormatSingleG(static_cast<float>(value), n) : dotnet::FormatDoubleG(value, n);
    }
    if (k == 'F' || k == 'f') {
        const int n = digits(format, 2);  // NumberFormatInfo.NumberDecimalDigits
        return isFloat ? dotnet::FormatSingleF(static_cast<float>(value), n) : dotnet::FormatDoubleF(value, n);
    }
    if (k == 'R' && format.size() == 1)
        return isFloat ? dotnet::FormatSingleR(static_cast<float>(value)) : dotnet::FormatDoubleR(value);
    throw std::logic_error("kspcore: unsupported numeric format \"" + std::string(format) +
                           "\" (Mono NumberFormatter custom/N/E/P formats are not ported yet)");
}

}  // namespace

// ---- IDateFormatterExtensions.cs ---------------------------------------------------------------------------------

namespace IDateFormatterExtensions {
// IDateFormatterExtensions.cs:3
void GetTimeComponents(const IDateTimeFormatter& formatter, double time, int& years, int& days, int& hours, int& minutes,
                       int& seconds) {
    const int year = formatter.Year();
    const int day = formatter.Day();
    years = dotnet::ConvToInt32(time / static_cast<double>(year));
    time -= static_cast<double>(years) * static_cast<double>(year);
    seconds = dotnet::ConvToInt32(time);
    minutes = seconds / 60 % 60;
    hours = seconds / 3600 % (day / 3600);
    days = seconds / day;
    seconds %= 60;
}
// IDateFormatterExtensions.cs:16
void GetDateComponents(const IDateTimeFormatter& formatter, double time, int& years, int& days, int& hours, int& minutes,
                       int& seconds) {
    GetTimeComponents(formatter, time, years, days, hours, minutes, seconds);
    years = WrapAdd(years, 1);
    days = WrapAdd(days, 1);
}
}  // namespace IDateFormatterExtensions

namespace KSPUtil {

// ---- DefaultDateTimeFormatter (KSPUtil.cs:33-672) ----------------------------------------------------------------

// KSPUtil.cs:77 / 89
int DefaultDateTimeFormatter::Day() const { return GameSettings::KERBIN_TIME ? KerbinDay : EarthDay; }
int DefaultDateTimeFormatter::Year() const { return GameSettings::KERBIN_TIME ? KerbinYear : EarthYear; }

// KSPUtil.cs:133
std::string DefaultDateTimeFormatter::PrintTimeLong(double time) {
    if (const char* text = IsBadNum(time)) return text;
    GetDateFromUT(time);
    std::string sb;
    if (dateHolder[4] < 0 || dateHolder[3] < 0 || dateHolder[2] < 0 || dateHolder[1] < 0 || dateHolder[0] < 0) sb += "-";
    const LocalStrings& l = L();
    AddUnits(sb, absDateHolder[4], l.s6002322, l.s6002323) += ", ";
    AddUnits(sb, absDateHolder[3], l.s6002324, l.s6002325) += ", ";
    AddUnits(sb, absDateHolder[2], l.s6002326, l.s6002327) += ", ";
    AddUnits(sb, absDateHolder[1], l.s6002328, l.s6002329) += ", ";
    AddUnits(sb, absDateHolder[0], l.s6002330, l.s6002331);
    return sb;
}

// KSPUtil.cs:154
std::string DefaultDateTimeFormatter::PrintTimeStamp(double time, bool days, bool years) {
    if (const char* text = IsBadNum(time)) return text;
    GetDateFromUT(time);
    std::string sb;
    const LocalStrings& l = L();
    if (years) sb += l.s6002322 + " " + IntStr(dateHolder[4]) + ", ";
    if (days) sb += l.s6002324 + " " + IntStr(dateHolder[3]) + " - ";
    AppendTimeToStringBuilder(sb, absDateHolder[4] < 10);
    return sb;
}

// KSPUtil.cs:175
std::string DefaultDateTimeFormatter::PrintTimeStampCompact(double time, bool days, bool years) {
    if (const char* text = IsBadNum(time)) return text;
    GetDateFromUT(time);
    if (cachedSecondsUTforTimeStampCompact == secondsUTHolder && cachedDaysforTimeStampCompact == days &&
        cachedYearsforTimeStampCompact == years)
        return cachedTimeStampCompact;
    cachedSecondsUTforTimeStampCompact = secondsUTHolder;
    cachedDaysforTimeStampCompact = days;
    cachedYearsforTimeStampCompact = years;
    std::string sb;
    const LocalStrings& l = L();
    if (years) sb += IntStr(dateHolder[4]) + l.s7003243 + ", ";
    if (days) sb += IntStr(dateHolder[3]) + l.s7003244 + ", ";
    AppendTimeToStringBuilder(sb, absDateHolder[4] < 10);
    cachedTimeStampCompact = sb;
    return cachedTimeStampCompact;
}

// KSPUtil.cs:204
std::string DefaultDateTimeFormatter::PrintTime(double time, int valuesOfInterest, bool explicitPositive) {
    return PrintTime(time, valuesOfInterest, explicitPositive, false);
}

// KSPUtil.cs:209
std::string DefaultDateTimeFormatter::PrintTime(double time, int valuesOfInterest, bool explicitPositive, bool logEnglish) {
    if (const char* text = IsBadNum(time)) return text;
    const bool flag = time < 0.0;
    GetDateFromUT(time);
    static const char* const kTags[5] = {"#autoLOC_6002317", "#autoLOC_6002318", "#autoLOC_6002319", "#autoLOC_6002320",
                                         "#autoLOC_6002321"};
    static const char* const kEnglish[5] = {"s", "m", "h", "d", "y"};
    const char* const* array = logEnglish ? kEnglish : kTags;
    std::string sb;
    if (flag) sb += "- ";
    else if (explicitPositive) sb += "+ ";
    int num = 5;
    while (num-- > 0) {
        if (dateHolder[num] == 0) continue;
        const int stop = Mathf::Max(WrapSub(num, valuesOfInterest), -1);
        for (int num2 = num; num2 > stop; --num2) {
            sb += IntStr(AbsInt(dateHolder[num2]));
            sb += Localizer::Format(array[num2]);
            if (num2 - 1 > Mathf::Max(WrapSub(num, valuesOfInterest), -1)) sb += ", ";
        }
        break;
    }
    if (sb.empty()) sb = "0";
    return sb;
}

// KSPUtil.cs:257
std::string DefaultDateTimeFormatter::PrintTimeCompact(double time, bool explicitPositive) {
    if (const char* text = IsBadNum(time)) return text;
    const bool neg = time < 0.0;
    GetDateFromUT(time);
    std::string sb;
    const LocalStrings& l = L();
    if (neg) sb += l.s6002332 + " ";
    else if (explicitPositive) sb += l.s6002333 + " ";
    if (dateHolder[3] > 0) sb += IntStr(AbsInt(dateHolder[3])) + ":";
    AppendTimeToStringBuilder(sb, true);
    return sb;
}

// KSPUtil.cs:321
std::string DefaultDateTimeFormatter::PrintDateDelta(double time, bool includeTime, bool includeSeconds, bool useAbs) {
    if (const char* text = IsBadNum(time)) return text;
    if (useAbs && time < 0.0) time = 0.0 - time;
    std::string sb;
    GetDateFromUT(time);
    const LocalStrings& l = L();
    auto item = [&](int v, const std::string& unit) {
        if (!sb.empty()) sb += ", ";
        sb += IntStr(v) + " " + unit;
    };
    if (dateHolder[4] > 1) sb += IntStr(dateHolder[4]) + " " + l.s6002335;
    else if (dateHolder[4] == 1) sb += IntStr(dateHolder[4]) + " " + l.s6002334;
    if (dateHolder[3] > 1) item(dateHolder[3], l.s6002336);
    else if (dateHolder[3] == 1) item(dateHolder[3], l.s6002337);
    if (includeTime) {
        if (dateHolder[2] > 1) item(dateHolder[2], l.s6002339);
        else if (dateHolder[2] == 1) item(dateHolder[2], l.s6002338);
        if (dateHolder[1] > 1) item(dateHolder[1], l.s6002340);
        else if (dateHolder[1] == 1) item(dateHolder[1], l.s6002341);
        if (includeSeconds) {
            if (dateHolder[0] > 1) item(dateHolder[0], l.s6002342);
            else if (dateHolder[0] == 1) item(dateHolder[0], l.s6002343);
        }
    }
    if (sb.empty())
        sb += !includeTime ? ("0 " + l.s6002336) : (includeSeconds ? ("0 " + l.s6002342) : ("0 " + l.s6002340));
    return sb;
}

// KSPUtil.cs:419
std::string DefaultDateTimeFormatter::PrintDateDeltaCompact(double time, bool includeTime, bool includeSeconds, bool useAbs) {
    return PrintDateDeltaCompact(time, includeTime, includeSeconds, useAbs, 5);
}

// KSPUtil.cs:424
std::string DefaultDateTimeFormatter::PrintDateDeltaCompact(double time, bool includeTime, bool includeSeconds, bool useAbs,
                                                            int interestedPlaces) {
    int num = 0;
    if (const char* text = IsBadNum(time)) return text;
    if (useAbs && time < 0.0) time = 0.0 - time;
    std::string sb;
    GetDateFromUT(time);
    const LocalStrings& l = L();
    if (dateHolder[4] > 0 && num < interestedPlaces) {
        sb += IntStr(dateHolder[4]) + l.s6002321;
        ++num;
    }
    if (dateHolder[3] > 0 && num < interestedPlaces) {
        if (!sb.empty()) sb += ", ";
        sb += IntStr(dateHolder[3]) + l.s6002320;
        ++num;
    }
    if (includeTime) {
        if (dateHolder[2] > 0 && num < interestedPlaces) {
            if (!sb.empty()) sb += ", ";
            sb += IntStr(dateHolder[2]) + l.s6002319;
            ++num;
        }
        if (dateHolder[1] > 0 && num < interestedPlaces) {
            if (!sb.empty()) sb += ", ";
            sb += IntStr(dateHolder[1]) + l.s6002318;
            ++num;
        }
        if (includeSeconds && dateHolder[0] > 0 && num < interestedPlaces) {
            if (!sb.empty()) sb += ", ";
            sb += IntStr(dateHolder[0]) + l.s6002317;
        }
    }
    if (sb.empty())
        sb += !includeTime ? ("0" + l.s6002320) : (includeSeconds ? ("0" + l.s6002317) : ("0" + l.s6002318));
    return sb;
}

// KSPUtil.cs:488-536 (read the shared dateHolder of the last formatter call)
int DefaultDateTimeFormatter::GetUTYear() const { return WrapAdd(dateHolder[4], 1); }
int DefaultDateTimeFormatter::GetUTDay() const { return WrapAdd(dateHolder[3], 1); }
int DefaultDateTimeFormatter::GetUTHour() const { return dateHolder[2]; }
int DefaultDateTimeFormatter::GetUTMinute() const { return dateHolder[1]; }
int DefaultDateTimeFormatter::GetUTSecond() const { return dateHolder[0]; }
int DefaultDateTimeFormatter::GetTimeSpanYear() const { return dateHolder[4]; }
int DefaultDateTimeFormatter::GetTimeSpanDay() const { return dateHolder[3]; }
int DefaultDateTimeFormatter::GetTimeSpanHour() const { return dateHolder[2]; }
int DefaultDateTimeFormatter::GetTimeSpanMinute() const { return dateHolder[1]; }
int DefaultDateTimeFormatter::GetTimeSpanSecond() const { return dateHolder[0]; }

// KSPUtil.cs:538
std::string DefaultDateTimeFormatter::PrintDate(double time, bool includeTime, bool includeSeconds) {
    if (const char* text = IsBadNum(time)) return text;
    GetDateFromUT(time);
    if (cachedIncludeTimeforPrintDate == includeTime && cachedIncludeSecondsforPrintDate == includeSeconds) {
        if (!includeSeconds && cachedYearsforPrintDate == dateHolder[4] && cachedDaysforPrintDate == dateHolder[3] &&
            cachedHoursforPrintDate == dateHolder[2] && cachedMinsforPrintDate == dateHolder[1])
            return cachedPrintDate;
        if (includeSeconds && cachedYearsforPrintDate == dateHolder[4] && cachedDaysforPrintDate == dateHolder[3] &&
            cachedHoursforPrintDate == dateHolder[2] && cachedMinsforPrintDate == dateHolder[1] &&
            cachedSecondsforPrintDate == dateHolder[0])
            return cachedPrintDate;
    }
    cachedYearsforPrintDate = dateHolder[4];
    cachedDaysforPrintDate = dateHolder[3];
    cachedHoursforPrintDate = dateHolder[2];
    cachedMinsforPrintDate = dateHolder[1];
    cachedSecondsforPrintDate = dateHolder[0];
    cachedIncludeTimeforPrintDate = includeTime;
    cachedIncludeSecondsforPrintDate = includeSeconds;
    const LocalStrings& l = L();
    std::string sb = l.s6002322 + " ";
    sb += IntStr(WrapAdd(dateHolder[4], 1));
    sb += ", " + l.s6002324 + " ";
    sb += IntStr(WrapAdd(dateHolder[3], 1));
    if (includeTime) {
        sb += " - ";
        sb += IntStr(dateHolder[2]);
        sb += l.s6002319 + ", ";
        sb += IntStr(dateHolder[1]);
        sb += l.s6002318;
        if (includeSeconds) {
            sb += ", ";
            sb += IntStr(dateHolder[0]);
            sb += l.s6002317;
        }
    }
    cachedPrintDate = sb;
    return cachedPrintDate;
}

// KSPUtil.cs:587
std::string DefaultDateTimeFormatter::PrintDateNew(double time, bool includeTime) {
    if (const char* text = IsBadNum(time)) return text;
    std::string sb;
    GetDateFromUT(time);
    sb += Localizer::Format("#autoLOC_7003005", {IntStr(WrapAdd(dateHolder[4], 1)), IntStr(WrapAdd(dateHolder[3], 1))});
    if (includeTime) {
        sb += " - ";
        AppendTimeToStringBuilder(sb, true);
    }
    return sb;
}

// KSPUtil.cs:605
std::string DefaultDateTimeFormatter::PrintDateCompact(double time, bool includeTime, bool includeSeconds) {
    if (const char* text = IsBadNum(time)) return text;
    GetDateFromUT(time);
    if (cachedSecondsUTforPrintDateCompact == secondsUTHolder && cachedIncludeTimeforPrintDateCompact == includeTime &&
        cachedIncludeSecondsforPrintDateCompact == includeSeconds)
        return cachedPrintDateCompact;
    cachedSecondsUTforPrintDateCompact = secondsUTHolder;
    cachedIncludeTimeforPrintDateCompact = includeTime;
    cachedIncludeSecondsforPrintDateCompact = includeSeconds;
    const LocalStrings& l = L();
    std::string sb = l.s6002344;
    sb += IntStr(WrapAdd(dateHolder[4], 1));
    sb += ", ";
    sb += l.s6002345;
    if (WrapAdd(dateHolder[3], 1) < 10) sb += "0";
    sb += IntStr(WrapAdd(dateHolder[3], 1));
    if (includeTime) {
        sb += ", ";
        AppendTimeToStringBuilder(sb, includeSeconds);
    }
    cachedPrintDateCompact = sb;
    return cachedPrintDateCompact;
}

// ---- KSPUtil statics ------------------------------------------------------------------------------------------------

// KSPUtil.cs:779
IDateTimeFormatter& dateTimeFormatter() {
    auto& f = FormatterSlot();
    if (!f) f = std::make_shared<DefaultDateTimeFormatter>();
    return *f;
}
void SetDateTimeFormatter(std::shared_ptr<IDateTimeFormatter> formatter) { FormatterSlot() = std::move(formatter); }

// KSPUtil.cs:795 (non-OSX players)
std::string ApplicationRootPath() { return ApplicationDataPath + "/../"; }

// KSPUtil.cs:819
void CacheLocalStrings() {
    LocalStrings& l = L();
    auto f = [](const char* tag) { return Localizer::Format(tag); };
    l.s7003243 = f("#autoLOC_7003243");
    l.s7003244 = f("#autoLOC_7003244");
    l.s7003272 = f("#autoLOC_7003272");
    l.s7003273 = f("#autoLOC_7003273");
    l.s7003274 = f("#autoLOC_7003274");
    l.s7003275 = f("#autoLOC_7003275");
    l.s6002317 = f("#autoLOC_6002317");
    l.s6002318 = f("#autoLOC_6002318");
    l.s6002319 = f("#autoLOC_6002319");
    l.s6002320 = f("#autoLOC_6002320");
    l.s6002321 = f("#autoLOC_6002321");
    l.s6002322 = f("#autoLOC_6002322");
    l.s6002323 = f("#autoLOC_6002323");
    l.s6002324 = f("#autoLOC_6002324");
    l.s6002325 = f("#autoLOC_6002325");
    l.s6002326 = f("#autoLOC_6002326");
    l.s6002327 = f("#autoLOC_6002327");
    l.s6002328 = f("#autoLOC_6002328");
    l.s6002329 = f("#autoLOC_6002329");
    l.s6002330 = f("#autoLOC_6002330");
    l.s6002331 = f("#autoLOC_6002331");
    l.s6002332 = f("#autoLOC_6002332");
    l.s6002333 = f("#autoLOC_6002333");
    l.s6002334 = f("#autoLOC_6002334");
    l.s6002335 = f("#autoLOC_6002335");
    l.s6002336 = f("#autoLOC_6002336");
    l.s6002337 = f("#autoLOC_6002337");
    l.s6002338 = f("#autoLOC_6002338");
    l.s6002339 = f("#autoLOC_6002339");
    l.s6002340 = f("#autoLOC_6002340");
    l.s6002341 = f("#autoLOC_6002341");
    l.s6002342 = f("#autoLOC_6002342");
    l.s6002343 = f("#autoLOC_6002343");
    l.s6002344 = f("#autoLOC_6002344");
    l.s6002345 = f("#autoLOC_6002345");
}

// KSPUtil.cs:858
std::string LocalizeNumber(double value, std::string_view format) {
    if (value == std::numeric_limits<double>::infinity()) return Localizer::Format("#autoLOC_462439");
    return Localizer::Format("<<1>>", {FormatWith(value, format, false)});
}
// KSPUtil.cs:867
std::string LocalizeNumber(float value, std::string_view format) {
    return Localizer::Format("<<1>>", {FormatWith(value, format, true)});
}

// KSPUtil.cs:872
std::string PrintSI(double amount, std::string_view unitName, int sigFigs, bool longPrefix) {
    if (amount != 0.0 && !std::isinf(amount) && !std::isnan(amount)) {
        const int num = dotnet::ConvToInt32(std::floor(dotnet::Math::Log10(std::fabs(amount))));
        const int num2 = (num >= 0) ? (num / 3) : (WrapSub(num, 2) / 3);
        const int num3 = WrapAdd(WrapSub(num, WrapMul(num2, 3)), 1);
        int num4 = WrapSub(sigFigs, num3);
        const int num5 = std::max(0, std::min(WrapAdd(num2, unitIndex), static_cast<int>(prefixMults.size()) - 1));
        const std::string text = longPrefix ? longSIprefixes[static_cast<size_t>(num5)] : shortSIprefixes[static_cast<size_t>(num5)];
        amount /= prefixMults[static_cast<size_t>(num5)];
        if (num4 < 0) {
            const long long idx = -static_cast<long long>(num4);
            if (idx >= static_cast<long long>(digitsScale.size()))
                throw std::out_of_range("Index was outside the bounds of the array.");
            const double num6 = digitsScale[static_cast<size_t>(idx)];
            amount = dotnet::Math::Round(amount / num6) * num6;
            num4 = 0;
        }
        return Localizer::Format("<<1>><<2>><<3>>", {dotnet::FormatDoubleF(amount, num4), text, std::string(unitName)});
    }
    return Localizer::Format("<<1>><<2>>", {dotnet::FormatDoubleG(amount), std::string(unitName)});
}

// KSPUtil.cs:1138 WriteArray<T>
namespace {
template <class T, class F>
std::string WriteArrayImpl(std::span<const T> array, F&& toString) {
    std::string text;
    for (size_t i = 0; i < array.size(); ++i) {
        text += toString(array[i]);
        if (i + 1 < array.size()) text += "; ";
    }
    return text;
}
}  // namespace
std::string WriteArray(std::span<const std::string> array) {
    return WriteArrayImpl(array, [](const std::string& s) { return s; });
}
std::string WriteArray(std::span<const int> array) {
    return WriteArrayImpl(array, [](int v) { return IntStr(v); });
}
std::string WriteArray(std::span<const double> array) {
    return WriteArrayImpl(array, [](double v) { return dotnet::FormatDoubleG(v); });
}
std::string WriteArray(std::span<const float> array) {
    return WriteArrayImpl(array, [](float v) { return dotnet::FormatSingleG(v); });
}
std::string WriteArray(std::span<const bool> array) {
    return WriteArrayImpl(array, [](bool v) { return dotnet::FormatBool(v); });
}

// KSPUtil.cs:1195
std::string PrintCoordinates(double latitude, double longitude, bool singleLine, bool includeMinutes, bool includeSeconds) {
    return PrintLatitude(latitude, includeMinutes, includeSeconds) + (singleLine ? ", " : "\n") +
           PrintLongitude(longitude, includeMinutes, includeSeconds);
}

namespace {
// KSPUtil.cs:1200 / 1219 (identical bodies apart from the hemisphere letters)
std::string PrintAngleDMS(double angle, bool includeMinutes, bool includeSeconds, const std::string& pos, const std::string& neg) {
    int num = dotnet::ConvToInt32(dotnet::Math::Round(angle * 3600.0));
    const int value = num / 3600;
    num = AbsInt(num % 3600);
    const int num2 = num / 60;
    num %= 60;
    std::string text = IntStr(AbsInt(value)) + "\xC2\xB0 ";
    if (includeMinutes) text = text + IntStr(num2) + "' ";
    if (includeSeconds) text = text + IntStr(num) + "\" ";
    return text + ((angle >= 0.0) ? pos : neg);
}
}  // namespace

std::string PrintLatitude(double latitude, bool includeMinutes, bool includeSeconds) {
    return PrintAngleDMS(latitude, includeMinutes, includeSeconds, L().s7003272, L().s7003273);
}
std::string PrintLongitude(double longitude, bool includeMinutes, bool includeSeconds) {
    return PrintAngleDMS(longitude, includeMinutes, includeSeconds, L().s7003274, L().s7003275);
}

// KSPUtil.cs:1238-1296
std::string PrintTimeLong(double time) { return dateTimeFormatter().PrintTimeLong(time); }
std::string PrintTimeStamp(double time, bool days, bool years) { return dateTimeFormatter().PrintTimeStamp(time, days, years); }
std::string PrintTimeStampCompact(double time, bool days, bool years) {
    return dateTimeFormatter().PrintTimeStampCompact(time, days, years);
}
std::string PrintTime(double time, int valuesOfInterest, bool explicitPositive) {
    return dateTimeFormatter().PrintTime(time, valuesOfInterest, explicitPositive);
}
std::string PrintTime(double time, int valuesOfInterest, bool explicitPositive, bool logEnglish) {
    return dateTimeFormatter().PrintTime(time, valuesOfInterest, explicitPositive, logEnglish);
}
std::string PrintTimeCompact(double time, bool explicitPositive) {
    return dateTimeFormatter().PrintTimeCompact(time, explicitPositive);
}
std::string PrintDateDelta(double time, bool includeTime, bool includeSeconds, bool useAbs) {
    return dateTimeFormatter().PrintDateDelta(time, includeTime, includeSeconds, useAbs);
}
std::string PrintDateDeltaCompact(double time, bool includeTime, bool includeSeconds, bool useAbs) {
    return dateTimeFormatter().PrintDateDeltaCompact(time, includeTime, includeSeconds, useAbs);
}
std::string PrintDateDeltaCompact(double time, bool includeTime, bool includeSeconds, int interestedPlaces, bool useAbs) {
    return dateTimeFormatter().PrintDateDeltaCompact(time, includeTime, includeSeconds, useAbs, interestedPlaces);
}
std::string PrintDate(double time, bool includeTime, bool includeSeconds) {
    return dateTimeFormatter().PrintDate(time, includeTime, includeSeconds);
}
std::string PrintDateNew(double time, bool includeTime) { return dateTimeFormatter().PrintDateNew(time, includeTime); }
std::string PrintDateCompact(double time, bool includeTime, bool includeSeconds) {
    return dateTimeFormatter().PrintDateCompact(time, includeTime, includeSeconds);
}

// KSPUtil.cs:1298
std::string PrintModuleName(std::string_view moduleName) {
    std::string_view text = moduleName;
    if (dotnet::StartsWith(text, "Module")) text = moduleName.substr(6);
    return PrintSpacedStringFromCamelcase(text);
}

// KSPUtil.cs:1308
std::string PrintLocalizedModuleName(std::string_view moduleName) {
    std::string_view text = moduleName;
    if (dotnet::StartsWith(text, "Module")) text = moduleName.substr(6);
    static const std::pair<std::string_view, const char*> kMap[] = {
        {"Ablator", "#autoLOC_501008"},        {"SpacePlaneHangar", "#autoLOC_6001661"},
        {"IntakeAir", "#autoLOC_6002101"},     {"Oxidizer", "#autoLOC_502035"},
        {"Runway", "#autoLOC_300899"},         {"LaunchPad", "#autoLOC_300898"},
        {"ElectricCharge", "#autoLOC_313132"}, {"SolidFuel", "#autoLOC_501001"},
        {"Vertical", "#autoLOC_300898"},       {"LiquidFuel", "#autoLOC_502032"},
        {"EVAPropellant", "#autoLOC_501006"},  {"VehicleAssemblyBuilding", "#autoLOC_6001660"},
        {"MonoPropellant", "#autoLOC_501002"}, {"XenonGas", "#autoLOC_501003"},
        {"Ore", "#autoLOC_501007"},            {"SpaceplaneHangar", "#autoLOC_6001661"},
    };
    for (const auto& [k, v] : kMap)
        if (text == k) return v;
    return PrintSpacedStringFromCamelcase(text);
}

// KSPUtil.cs:1355 (UTF-16 indices; the loop bound is the ORIGINAL length, so text beyond it is not scanned -- sic)
std::string PrintSpacedStringFromCamelcase(std::string_view sv) {
    if (sv.empty()) return std::string();
    const std::u16string s = dotnet::ToUtf16(sv);
    std::u16string text = s;
    const size_t length = text.size();
    for (size_t i = 0; i < length; ++i) {
        if (i == 0 && !dotnet::CharIsUpper(text[i])) {
            text = std::u16string(1, dotnet::CharToUpper(s[0])) + s.substr(1);
        } else if (i != 0 && dotnet::CharIsUpper(text[i]) && !dotnet::CharIsUpper(text[i - 1])) {
            text.insert(i, 1, u' ');
            ++i;
        }
    }
    return dotnet::FromUtf16(text);
}

// KSPUtil.cs:1397
std::string GetOrCreatePath(std::string_view relPath) {
    const std::string path = ApplicationRootPath() + std::string(relPath);
    std::error_code ec;
    const std::filesystem::path p(dotnet::Utf8ToWide(path));
    if (!std::filesystem::is_directory(p, ec)) std::filesystem::create_directories(p, ec);
    return path;
}

// KSPUtil.cs:1406
std::string GenerateFilePathWithDate(std::string_view filePath) {
    const size_t dot = filePath.rfind('.');
    if (dot == std::string_view::npos) throw std::out_of_range("startIndex");  // Insert(-1, ...)
    const std::time_t now = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y_%m_%d_%H_%M_%S", &tm);
    std::string r(filePath);
    r.insert(dot, std::string(" (") + buf + ")");
    return r;
}

// KSPUtil.cs:1411
bool MoveFile(const std::string& filePath, const std::string& destinationPath, bool overwrite) {
    namespace fs = std::filesystem;
    const fs::path src(dotnet::Utf8ToWide(filePath)), dst(dotnet::Utf8ToWide(destinationPath));
    std::error_code ec;
    std::string message;
    if (overwrite && fs::is_regular_file(dst, ec)) fs::remove(dst, ec);
    if (!fs::exists(src, ec)) message = "Could not find file '" + filePath + "'.";
    else if (fs::exists(dst, ec)) message = "Cannot create a file when that file already exists.";  // File.Move
    else {
        fs::rename(src, dst, ec);
        if (ec) message = ec.message();
    }
    if (!message.empty()) {
        log::Error("Unable to move file from " + filePath + " to " + destinationPath + "\n" + message);
        return false;
    }
    return true;
}

// KSPUtil.cs:1500 (FileInfo.Name minus Path.GetExtension: last '.' after the last separator, not the final character)
std::string StripFileExtension(std::string_view filePath) {
    const size_t sep = filePath.find_last_of("\\/:");
    const std::string_view name = sep == std::string_view::npos ? filePath : filePath.substr(sep + 1);
    const size_t dot = name.rfind('.');
    size_t extLen = 0;
    if (dot != std::string_view::npos && dot != name.size() - 1) extLen = name.size() - dot;
    return std::string(name.substr(0, name.size() - extLen));
}

// KSPUtil.cs:1505
std::string SanitizeString(std::string_view originalStringIn, char replacementChar, bool replaceEmpty) {
    std::string originalString(originalStringIn);
    for (char bad : {'\\', '/', '.', ':', '|', '*', '?', '{', '}', '<', '>', '"'})
        originalString = ReplaceChar(std::move(originalString), bad, replacementChar);
    originalString = dotnet::Trim(originalString);
    if (replaceEmpty && originalString.empty()) originalString = "Unnamed";
    else if (CheckDOSName(originalString)) originalString += replacementChar;
    return originalString;
}

// KSPUtil.cs:1531
std::string SanitizeFilename(std::string_view originalFilename) {
    return SanitizeString(Localizer::Format(originalFilename), '_', true);
}

// KSPUtil.cs:1536
bool CheckDOSName(std::string_view originalString) {
    static const char* const kNames[17] = {"con",  "prn",  "aux",  "nul",  "clock$", "lst",  "lpt",  "lpt0", "lpt1",
                                           "lpt2", "lpt3", "lpt4", "com0", "com1",   "com2", "com3", "com4"};
    const std::string lower = AsciiLower(originalString);
    for (const char* n : kNames)
        if (lower == n) return true;
    return false;
}

// KSPUtil.cs:1555
std::string SanitizeInstanceName(std::string_view originalString) {
    std::string result(originalString);
    size_t num = originalString.find(" (");
    if (num == std::string_view::npos) num = originalString.find('(');
    if (num != std::string_view::npos) result = std::string(originalString.substr(0, num));
    num = AsciiLower(originalString).find("(clone");
    if (num != std::string::npos) result = std::string(originalString.substr(0, num));
    return result;
}

namespace {
// KSPUtil.cs:1575 (Directory.GetFiles then Directory.GetDirectories, both in file-system enumeration order)
float CalculateFolderSizeImpl(const std::filesystem::path& root, int& totalFiles) {
    namespace fs = std::filesystem;
    float num = 0.0f;
    totalFiles = 0;
    try {
        if (!fs::is_directory(root)) return num;
        std::vector<fs::path> dirs;
        for (const auto& e : fs::directory_iterator(root)) {
            if (e.is_directory()) {
                dirs.push_back(e.path());
            } else if (e.is_regular_file()) {
                num = f32store(fx(num) + fx(static_cast<float>(static_cast<int64_t>(e.file_size()))));
                ++totalFiles;
            }
        }
        for (const auto& d : dirs) num = f32store(fx(num) + fx(CalculateFolderSizeImpl(d, totalFiles)));
    } catch (const std::exception& ex) {
        log::Error(std::string("Unable to calculate folder size: ") + ex.what());
    }
    return num;
}
}  // namespace

float CalculateFolderSize(const std::string& folderPath, int& totalFiles) {
    return CalculateFolderSizeImpl(std::filesystem::path(dotnet::Utf8ToWide(folderPath)), totalFiles);
}

// KSPUtil.cs:1608
VersionCompareResult CheckVersion(std::string_view versionString, int lastMajor, int lastMinor, int lastRev) {
    const std::vector<std::string> array = SplitChar(versionString, '.');
    if (array.size() != 3) return VersionCompareResult::INVALID;
    const int version_major = dotnet::ParseInt32(array[0]);
    const int version_minor = dotnet::ParseInt32(array[1]);
    const int version_revision = dotnet::ParseInt32(array[2]);
    return CheckVersion(version_major, version_minor, version_revision, lastMajor, lastMinor, lastRev);
}

// KSPUtil.cs:1621
VersionCompareResult CheckVersion(int version_major, int version_minor, int version_revision, int lastMajor, int lastMinor,
                                  int lastRev) {
    bool flag = false;
    if (version_major > lastMajor) flag = true;
    else if (version_major == lastMajor) {
        if (version_minor > lastMinor) flag = true;
        else if (version_minor == lastMinor && version_revision >= lastRev) flag = true;
    }
    if (version_major <= Versioning::version_major &&
        (version_major != Versioning::version_major || version_minor <= Versioning::version_minor) &&
        (version_major != Versioning::version_major || version_minor != Versioning::version_minor ||
         version_revision <= Versioning::Revision)) {
        if (flag) return VersionCompareResult::COMPATIBLE;
        return VersionCompareResult::INCOMPATIBLE_TOO_EARLY;
    }
    return VersionCompareResult::INCOMPATIBLE_TOO_LATE;
}

// KSPUtil.cs:1675 (string.Replace(string, string): ArgumentException for an empty oldValue)
std::string ReplaceString(std::string src, std::span<const StringReplacement> replacements) {
    for (const StringReplacement& r : replacements) {
        if (r.badString.empty()) throw ArgumentException("String cannot be of zero length.");
        size_t pos = 0;
        while ((pos = src.find(r.badString, pos)) != std::string::npos) {
            src.replace(pos, r.badString.size(), r.replacement);
            pos += r.replacement.size();
        }
    }
    return src;
}

// KSPUtil.cs:1702: WrapAround(Acos(Dot) * Sign(Dot(Cross, up)), 0, (float)Math.PI * 2f) -- the argument is a float store
float HeadingRadians(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis) {
    if (v1 == v2) return 0.0f;
    const float arg = f32store(fx(Mathf::Acos(Vector3f::Dot(v1, v2))) *
                               fx(Mathf::Sign(Vector3f::Dot(Vector3f::Cross(v1, v2), upAxis))));
    return UtilMath::WrapAround(arg, 0.0f, 6.28318548f);  // (float)Math.PI * 2f, folded by the C# compiler
}
// KSPUtil.cs:1711
float HeadingDegrees(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis) {
    if (v1 == v2) return 0.0f;
    const float arg = f32store(fx(Mathf::Acos(Vector3f::Dot(v1, v2))) *
                               fx(Mathf::Sign(Vector3f::Dot(Vector3f::Cross(v1, v2), upAxis))) * fx(57.29578f));
    return UtilMath::WrapAround(arg, 0.0f, 360.0f);
}
// KSPUtil.cs:1720
float BearingRadians(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis) {
    if (v1 == v2) return 0.0f;
    return f32store(fx(Mathf::Acos(Vector3f::Dot(v1, v2))) * fx(Mathf::Sign(Vector3f::Dot(Vector3f::Cross(v1, v2), upAxis))));
}
// KSPUtil.cs:1729
float BearingDegrees(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis) {
    if (v1 == v2) return 0.0f;
    return f32store(fx(Mathf::Acos(Vector3f::Dot(v1, v2))) *
                    fx(Mathf::Sign(Vector3f::Dot(Vector3f::Cross(v1, v2), upAxis))) * fx(57.29578f));
}

// KSPUtil.cs:1738
std::string PrintCollection(const std::vector<std::string>& collection, std::string_view separator) {
    return PrintCollection(collection, separator, [](const std::string& s) { return s; });
}

// KSPUtil.cs:1760
std::string AppendValueToString(std::string_view s0, std::string_view val, char separator) {
    std::string text(s0);
    if (s0.empty()) text = std::string(val);
    else if (text.find(val) == std::string::npos) text = text + separator + " " + std::string(val);
    return text;
}

// KSPUtil.cs:1784 (column-wise Vector4 sums)
Matrix4x4f Add(const Matrix4x4f& left, const Matrix4x4f& right) {
    Matrix4x4f m = left;
    Add(m, right);
    return m;
}
// KSPUtil.cs:1794 / 1814
void Add(Matrix4x4f& left, const Matrix4x4f& right) {
    left.m00 += right.m00; left.m01 += right.m01; left.m02 += right.m02; left.m03 += right.m03;
    left.m10 += right.m10; left.m11 += right.m11; left.m12 += right.m12; left.m13 += right.m13;
    left.m20 += right.m20; left.m21 += right.m21; left.m22 += right.m22; left.m23 += right.m23;
    left.m30 += right.m30; left.m31 += right.m31; left.m32 += right.m32; left.m33 += right.m33;
}
// KSPUtil.cs:1834
Vector3d Diag(const Matrix4x4f& m) { return Vector3d(m.m00, m.m11, m.m22); }
// KSPUtil.cs:1839
Matrix4x4f ToDiagonalMatrix(float v) {
    Matrix4x4f m = Matrix4x4f::identity();
    ToDiagonalMatrix(v, m);
    return m;
}
// KSPUtil.cs:1861
void ToDiagonalMatrix(float v, Matrix4x4f& m) {
    m = Matrix4x4f();
    m.m00 = m.m11 = m.m22 = m.m33 = v;
}
// KSPUtil.cs:1881
void ToDiagonalMatrix2(float v, Matrix4x4f& m) { m.m00 = m.m11 = m.m22 = m.m33 = v; }
// KSPUtil.cs:1889
Matrix4x4f ToDiagonalMatrix(const Vector3f& v) {
    Matrix4x4f m = Matrix4x4f::identity();
    m.m00 = v.x;
    m.m11 = v.y;
    m.m22 = v.z;
    return m;
}
// KSPUtil.cs:1898
Matrix4x4f ToDiagonalMatrix2(const Vector3f& v, Matrix4x4f& m) {
    m.m00 = v.x;
    m.m11 = v.y;
    m.m22 = v.z;
    return m;
}
// KSPUtil.cs:1906
Matrix4x4f OuterProduct(const Vector3f& left, const Vector3f& right) {
    Matrix4x4f m = Matrix4x4f::identity();
    OuterProduct2(left, right, m);
    return m;
}
// KSPUtil.cs:1921
void OuterProduct(const Vector3f& left, const Vector3f& right, Matrix4x4f& m) {
    OuterProduct2(left, right, m);
    m.m03 = m.m13 = m.m23 = m.m30 = m.m31 = m.m32 = 0.0f;
    m.m33 = 1.0f;
}
// KSPUtil.cs:1941
void OuterProduct2(const Vector3f& left, const Vector3f& right, Matrix4x4f& m) {
    m.m00 = left.x * right.x; m.m01 = left.x * right.y; m.m02 = left.x * right.z;
    m.m10 = left.y * right.x; m.m11 = left.y * right.y; m.m12 = left.y * right.z;
    m.m20 = left.z * right.x; m.m21 = left.z * right.y; m.m22 = left.z * right.z;
}

// KSPUtil.cs:1975
std::string GetPartName(std::string_view configPartID) {
    const size_t i = configPartID.find('_');
    const long long length = i == std::string_view::npos ? -1 : static_cast<long long>(i);
    return Substring(configPartID, 0, length);
}
// KSPUtil.cs:1981
void GetPartInfo(std::string_view configPartID, std::string& partName, std::string& craftID) {
    const size_t i = configPartID.find('_');
    const long long num = i == std::string_view::npos ? -1 : static_cast<long long>(i);
    partName = Substring(configPartID, 0, num);
    craftID = Substring(configPartID, num + 1, static_cast<long long>(configPartID.size()) - num - 1);
}
// KSPUtil.cs:1988
bool GetAttachNodeInfo(std::string_view configAttnID, std::string& nodeID, std::string& attnPartID, Vector3f& attnPos) {
    std::string attachMeshName;
    return GetAttachNodeInfo(configAttnID, nodeID, attnPartID, attnPos, attachMeshName);
}
// KSPUtil.cs:1994
bool GetAttachNodeInfo(std::string_view configAttnID, std::string& nodeID, std::string& attnPartID, Vector3f& attnPos,
                       std::string& attachMeshName) {
    Vector3f attnRot = Vector3f::zero();
    Vector3f secAxis = Vector3f::zero();
    return GetAttachNodeInfo(configAttnID, nodeID, attnPartID, attnPos, attachMeshName, attnRot, secAxis);
}
// KSPUtil.cs:2001
bool GetAttachNodeInfo(std::string_view configAttnID, std::string& nodeID, std::string& attnPartID, Vector3f& attnPos,
                       std::string& attachMeshName, Vector3f& attnRot, Vector3f& secAxis) {
    bool result = false;
    const std::vector<std::string> array = SplitChar(configAttnID, ',');
    nodeID = array[0];
    const std::vector<std::string> array2 = SplitChar(At(array, 1), '_');
    attnPartID = At(array2, 1);
    if (array2.size() > 2) {
        attnPos = ParseVector3(ReplaceChar(array2[2], '|', ','));
        result = true;
    }
    if (nodeID == "srfAttach" && array.size() > 2) {
        attachMeshName = array[2];
        if (array.size() > 3) {
            const std::string vectorString2 = ReplaceChar(array[3], '|', ',');
            const std::string vectorString3 = ReplaceChar(At(array, 4), '|', ',');
            const std::string vectorString4 = ReplaceChar(At(array, 5), '|', ',');
            attnPos = ParseVector3(vectorString2);
            attnRot = ParseVector3(vectorString3);
            secAxis = ParseVector3(vectorString4);
            result = true;
        }
    }
    return result;
}
// KSPUtil.cs:2031
bool GetAttachNodeInfo(std::string_view configAttnID, std::string& nodeID, std::string& attnPartID, Vector3f& attnPos,
                       Vector3f& attnRot, Vector3f& attnActualPos, Vector3f& attnActualRot) {
    constexpr float inf = std::numeric_limits<float>::infinity();
    const Vector3f positiveInfinity(inf, inf, inf);
    bool result = false;
    const std::vector<std::string> array = SplitChar(configAttnID, ',');
    nodeID = array[0];
    const std::vector<std::string> array2 = SplitChar(At(array, 1), '_');
    attnPartID = At(array2, 1);
    if (array2.size() >= 3) {
        result = true;
        attnPos = ParseVector3(array2[2], '|');
        attnRot = array2.size() >= 4 ? ParseVector3(array2[3], '|') : positiveInfinity;
        attnActualPos = array2.size() >= 5 ? ParseVector3(array2[4], '|') : positiveInfinity;
        attnActualRot = array2.size() >= 6 ? ParseVector3(array2[5], '|') : positiveInfinity;
    }
    return result;
}
// KSPUtil.cs:2070
std::string GetLinkID(std::string_view configAttnID) {
    const size_t i = configAttnID.find('_');
    const long long num = i == std::string_view::npos ? -1 : static_cast<long long>(i);
    return Substring(configAttnID, num + 1, static_cast<long long>(configAttnID.size()) - num - 1);
}

// KSPUtil.cs:2117
int GenerateSuperSeed(const Guid& guid, int seed) {
    uint8_t array[16];
    for (int i = 0; i < 16; ++i) array[i] = guid.bytes[i];
    for (int i = 0; i < 3; ++i) {
        array[i] ^= array[i + 4];
        array[i] ^= array[i + 8];
        array[i] ^= array[i + 12];
    }
    const int num = static_cast<int>(static_cast<uint32_t>(array[0]) | (static_cast<uint32_t>(array[1]) << 8) |
                                     (static_cast<uint32_t>(array[2]) << 16) | (static_cast<uint32_t>(array[3]) << 24));
    const int a = WrapAdd(num, seed);
    const int b = WrapAdd(a, 1);
    return WrapAdd(WrapMul(a, b) / 2, seed);
}

}  // namespace KSPUtil
}  // namespace ksp
