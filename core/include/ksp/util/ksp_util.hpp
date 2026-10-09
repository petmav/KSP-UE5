// KSPUtil (Assembly-CSharp/KSPUtil.cs), the engine-independent parts, plus IDateTimeFormatter.cs and
// IDateFormatterExtensions.cs. The vector parse/write helpers (ParseVector2/3/4, ParseQuaternion, WriteVector,
// WriteQuaternion) are in ksp/config/kspparseable.hpp (WP-02).
//
// Time and date formatting (DefaultDateTimeFormatter) reproduces KSP exactly, including:
//  * the calendar switch GameSettings.KERBIN_TIME: Kerbin day 21,600 s (6 h) / year 9,201,600 s (426 days), or Earth
//    day 86,400 s / year 31,536,000 s (365 days); components come from (int) truncations of the double UT, so negative
//    times give negative components;
//  * the shared static dateHolder / absDateHolder (every formatter call overwrites them; GetUT*/GetTimeSpan* read the
//    last call's values);
//  * the per-instance caches of PrintTimeStampCompact / PrintDate / PrintDateCompact, keyed on floor(UT) or the date
//    components but not on KERBIN_TIME. Quirk: on a fresh formatter PrintDate(t, false) with year/day/hour/minute all 0,
//    and PrintTimeStampCompact(t) / PrintDateCompact(t, false) with floor(t) == -1, hit the empty cache and return C#
//    null. kspcore returns "" for that null (callers only concatenate it).
//  * unit strings come from the Localizer (KSPUtil.CacheLocalStrings caches 35 of them; PrintTime and PrintDateNew call
//    Localizer.Format on every call). Until CacheLocalStrings has run the cached strings are null, i.e. "".
//
// Not ported here (Unity-bound; see STATUS.md "WP-03 skipped" for destinations): FindInPartModel, recurseModels,
// GetTransformPathToRoot, GetTransformIndexPathToRoot, FindTransformAtIndexPath, HasAncestorTransform,
// HasDescendantTransform, FindComponentsImplementing, FindTagsInChildren, RemoveNonHighlightableRenderers,
// GetPartByCraftID (Part), ClampRectToScreen (Screen), GenerateTexture2DArray, GetActivator/ObjectActivator (reflection),
// ApplicationFileProtocol, GetRelativePath (System.Uri escaping; only the craft-thumbnail UI uses it).
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"  // Guid
#include "ksp/config/kspparseable.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/math/quaterniond.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/unity/color.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

// GameSettings: only the values WP-03 and WP-08 read (the GameSettings port replaces this block). GameSettings.cs:1034
// SetDefaultValues sets KERBIN_TIME = true.
namespace GameSettings {
inline bool KERBIN_TIME = true;
// WP-08 (patched conics): GameSettings.cs:999-1000 SetDefaultValues (settings.cfg may override them in game)
inline int CONIC_PATCH_LIMIT = 3;
inline bool ALWAYS_SHOW_TARGET_APPROACH_MARKERS = false;
// WP-10 (time warp): GameSettings.cs:1003-1011 SetDefaultValues (settings.cfg may override them in game)
inline bool ORBIT_WARP_DOWN_AT_SOI = true;
inline bool SHOW_PWARP_WARNING = true;
inline TimeWarp::MaxRailsRateMode ORBIT_WARP_MAXRATE_MODE = TimeWarp::MaxRailsRateMode::PeAltitude;
inline double ORBIT_WARP_PEMODE_SURFACE_MARGIN = 250.0;
inline float ORBIT_WARP_ALTMODE_LIMIT_MODIFIER = 1.0f;
}

// Versioning (Versioning.cs: values from the shipped asset; KSP 1.12.5 build 3190)
namespace Versioning {
inline int version_major = 1;
inline int version_minor = 12;
inline int Revision = 5;
inline int BuildID = 3190;
}  // namespace Versioning

// VersionCompareResult.cs
enum class VersionCompareResult : int { INCOMPATIBLE_TOO_EARLY = 0, COMPATIBLE = 1, INCOMPATIBLE_TOO_LATE = 2, INVALID = 3 };

// IDateTimeFormatter.cs
class IDateTimeFormatter {
public:
    virtual ~IDateTimeFormatter() = default;
    virtual int Minute() const = 0;
    virtual int Hour() const = 0;
    virtual int Day() const = 0;
    virtual int Year() const = 0;
    virtual std::string PrintTimeLong(double time) = 0;
    virtual std::string PrintTimeStamp(double time, bool days = false, bool years = false) = 0;
    virtual std::string PrintTimeStampCompact(double time, bool days = false, bool years = false) = 0;
    virtual std::string PrintTime(double time, int valuesOfInterest, bool explicitPositive) = 0;
    virtual std::string PrintTime(double time, int valuesOfInterest, bool explicitPositive, bool logEnglish) = 0;
    virtual std::string PrintTimeCompact(double time, bool explicitPositive) = 0;
    virtual std::string PrintDateDelta(double time, bool includeTime, bool includeSeconds, bool useAbs) = 0;
    virtual std::string PrintDateDeltaCompact(double time, bool includeTime, bool includeSeconds, bool useAbs) = 0;
    virtual std::string PrintDateDeltaCompact(double time, bool includeTime, bool includeSeconds, bool useAbs,
                                              int interestedPlaces) = 0;
    virtual std::string PrintDate(double time, bool includeTime, bool includeSeconds = false) = 0;
    virtual std::string PrintDateNew(double time, bool includeTime) = 0;
    virtual std::string PrintDateCompact(double time, bool includeTime, bool includeSeconds = false) = 0;
};

// IDateFormatterExtensions.cs
namespace IDateFormatterExtensions {
// IDateFormatterExtensions.cs:3 (casts are C# (int) truncations: NaN / out of range -> int.MinValue)
void GetTimeComponents(const IDateTimeFormatter& formatter, double time, int& years, int& days, int& hours, int& minutes,
                       int& seconds);
// IDateFormatterExtensions.cs:16 (years and days 1-based)
void GetDateComponents(const IDateTimeFormatter& formatter, double time, int& years, int& days, int& hours, int& minutes,
                       int& seconds);
}  // namespace IDateFormatterExtensions

namespace KSPUtil {

// KSPUtil.cs:33 DefaultDateTimeFormatter
class DefaultDateTimeFormatter : public IDateTimeFormatter {
public:
    int Minute() const override { return 60; }
    int Hour() const override { return 3600; }
    int Day() const override;
    int Year() const override;
    std::string PrintTimeLong(double time) override;
    std::string PrintTimeStamp(double time, bool days = false, bool years = false) override;
    std::string PrintTimeStampCompact(double time, bool days = false, bool years = false) override;
    std::string PrintTime(double time, int valuesOfInterest, bool explicitPositive) override;
    std::string PrintTime(double time, int valuesOfInterest, bool explicitPositive, bool logEnglish) override;
    std::string PrintTimeCompact(double time, bool explicitPositive) override;
    std::string PrintDateDelta(double time, bool includeTime, bool includeSeconds, bool useAbs) override;
    std::string PrintDateDeltaCompact(double time, bool includeTime, bool includeSeconds, bool useAbs) override;
    std::string PrintDateDeltaCompact(double time, bool includeTime, bool includeSeconds, bool useAbs,
                                      int interestedPlaces) override;
    std::string PrintDate(double time, bool includeTime, bool includeSeconds = false) override;
    std::string PrintDateNew(double time, bool includeTime) override;
    std::string PrintDateCompact(double time, bool includeTime, bool includeSeconds = false) override;

    int GetUTYear() const;
    int GetUTDay() const;
    int GetUTHour() const;
    int GetUTMinute() const;
    int GetUTSecond() const;
    int GetTimeSpanYear() const;
    int GetTimeSpanDay() const;
    int GetTimeSpanHour() const;
    int GetTimeSpanMinute() const;
    int GetTimeSpanSecond() const;

private:
    double cachedSecondsUTforTimeStampCompact = -1.0;
    bool cachedDaysforTimeStampCompact = false;
    bool cachedYearsforTimeStampCompact = false;
    std::string cachedTimeStampCompact;  // C# null initially: ""
    int cachedYearsforPrintDate = 0;
    int cachedDaysforPrintDate = 0;
    int cachedHoursforPrintDate = 0;
    int cachedMinsforPrintDate = 0;
    int cachedSecondsforPrintDate = 0;
    bool cachedIncludeTimeforPrintDate = false;
    bool cachedIncludeSecondsforPrintDate = false;
    std::string cachedPrintDate;
    double cachedSecondsUTforPrintDateCompact = -1.0;
    bool cachedIncludeTimeforPrintDateCompact = false;
    bool cachedIncludeSecondsforPrintDateCompact = false;
    std::string cachedPrintDateCompact;
};

// KSPUtil.cs:779 dateTimeFormatter: get creates a DefaultDateTimeFormatter when none is set; set replaces it (a mod's
// formatter). Process-wide.
IDateTimeFormatter& dateTimeFormatter();
void SetDateTimeFormatter(std::shared_ptr<IDateTimeFormatter> formatter);

// KSPUtil.cs:819 CacheLocalStrings (Localizer.Format of 35 tags; called by Localizer on a language switch)
void CacheLocalStrings();

// KSPUtil.cs:757-777 (public static, mutable in C#)
inline std::array<std::string, 17> shortSIprefixes = {"y", "z", "a", "f", "p", "n", "\xCE\xBC", "m", "", "k",
                                                      "M", "G", "T", "P", "E", "Z", "Y"};
inline std::array<std::string, 17> longSIprefixes = {"yocto", "zepto", "atto", "femto", "pico", "nano",
                                                     "micro", "milli", "",     "kilo",  "mega", "giga",
                                                     "tera",  "peta",  "exa",  "zetta", "yotta"};
inline std::array<double, 17> prefixMults = {1E-24, 1E-21, 1E-18, 1E-15, 1E-12, 1E-09, 1E-06, 0.001, 1.0,
                                             1000.0, 1000000.0, 1000000000.0, 1000000000000.0, 1000000000000000.0,
                                             1E+18, 1E+21, 1E+24};
inline std::array<double, 3> digitsScale = {1.0, 10.0, 100.0};
inline int unitIndex = 8;

// KSPUtil.cs:795 ApplicationRootPath = Application.dataPath + "/../" (Windows/Linux player). The host sets the data
// path (KSP_x64_Data); GetOrCreatePath builds on it.
inline std::string ApplicationDataPath;
std::string ApplicationRootPath();

// KSPUtil.cs:858 / 867. value.ToString(format): kspcore supports the standard formats "G[n]", "F[n]", "R" (and "");
// any other format (N, E, P, custom patterns such as "0.0") throws std::logic_error until ksp::dotnet ports Mono's
// NumberFormatter.
std::string LocalizeNumber(double value, std::string_view format);
std::string LocalizeNumber(float value, std::string_view format);
// KSPUtil.cs:872
std::string PrintSI(double amount, std::string_view unitName, int sigFigs = 3, bool longPrefix = false);

// KSPUtil.cs:920-1137, the double-precision and per-component overloads (the float string forms are in
// ksp/config/kspparseable.hpp): "G17" components; double.Parse / float.Parse (FormatException / OverflowException);
// fewer components: Debug.Log warning and zero / identity
std::string WriteVector(const Vector3d& vector, std::string_view separator = ",");
std::string WriteQuaternion(const QuaternionD& quaternion, std::string_view separator = ",");
Vector3d ParseVector3d(std::string_view vectorString, char separator = ',');
Vector3d ParseVector3d(std::string_view x, std::string_view y, std::string_view z);
QuaternionD ParseQuaternionD(std::string_view quaternionString, char separator = ',');
QuaternionD ParseQuaternionD(std::string_view x, std::string_view y, std::string_view z, std::string_view w);
Vector2f ParseVector2(std::string_view x, std::string_view y);
Vector3f ParseVector3(std::string_view x, std::string_view y, std::string_view z);
Vector4f ParseVector4(std::string_view x, std::string_view y, std::string_view z, std::string_view w);
Quaternionf ParseQuaternion(std::string_view x, std::string_view y, std::string_view z, std::string_view w);

// KSPUtil.cs:1138 WriteArray<T>: elements joined with "; " (T.ToString())
std::string WriteArray(std::span<const std::string> array);
std::string WriteArray(std::span<const int> array);
std::string WriteArray(std::span<const double> array);
std::string WriteArray(std::span<const float> array);
std::string WriteArray(std::span<const bool> array);
// KSPUtil.cs:1153 ParseArray<T>: Split(';') (empty entries kept), Trim, parser
template <class T>
std::vector<T> ParseArray(std::string_view arrayString, const std::function<T(const std::string&)>& parser);

// KSPUtil.cs:1195 / 1200 / 1219
std::string PrintCoordinates(double latitude, double longitude, bool singleLine, bool includeMinutes = true,
                             bool includeSeconds = true);
std::string PrintLatitude(double latitude, bool includeMinutes = true, bool includeSeconds = true);
std::string PrintLongitude(double longitude, bool includeMinutes = true, bool includeSeconds = true);

// KSPUtil.cs:1238-1296: forwarders to dateTimeFormatter
std::string PrintTimeLong(double time);
std::string PrintTimeStamp(double time, bool days = false, bool years = false);
std::string PrintTimeStampCompact(double time, bool days = false, bool years = false);
std::string PrintTime(double time, int valuesOfInterest, bool explicitPositive);
std::string PrintTime(double time, int valuesOfInterest, bool explicitPositive, bool logEnglish);
std::string PrintTimeCompact(double time, bool explicitPositive);
std::string PrintDateDelta(double time, bool includeTime, bool includeSeconds = false, bool useAbs = false);
std::string PrintDateDeltaCompact(double time, bool includeTime, bool includeSeconds, bool useAbs = false);
std::string PrintDateDeltaCompact(double time, bool includeTime, bool includeSeconds, int interestedPlaces,
                                  bool useAbs = false);
std::string PrintDate(double time, bool includeTime, bool includeSeconds = false);
std::string PrintDateNew(double time, bool includeTime);
std::string PrintDateCompact(double time, bool includeTime, bool includeSeconds = false);

// KSPUtil.cs:1298 / 1308 / 1355
std::string PrintModuleName(std::string_view moduleName);
std::string PrintLocalizedModuleName(std::string_view moduleName);
std::string PrintSpacedStringFromCamelcase(std::string_view s);

// KSPUtil.cs:1397 / 1406 / 1411
std::string GetOrCreatePath(std::string_view relPath);
std::string GenerateFilePathWithDate(std::string_view filePath);  // " (yyyy_MM_dd_HH_mm_ss)" before the last '.'
bool MoveFile(const std::string& filePath, const std::string& destinationPath, bool overwrite = false);

// KSPUtil.cs:1500 StripFileExtension(FileInfo): file name without directory and without Path.GetExtension(name)
std::string StripFileExtension(std::string_view filePath);
// KSPUtil.cs:1505 / 1531 / 1536 / 1555
std::string SanitizeString(std::string_view originalString, char replacementChar, bool replaceEmpty);
std::string SanitizeFilename(std::string_view originalFilename);
bool CheckDOSName(std::string_view originalString);
std::string SanitizeInstanceName(std::string_view originalString);
// KSPUtil.cs:1575 (float sum of file sizes in Directory.GetFiles order; totalFiles is overwritten by each recursive
// call, so it ends up counting only the last subdirectory's files when there are subdirectories -- sic)
float CalculateFolderSize(const std::string& folderPath, int& totalFiles);
// KSPUtil.cs:1608 / 1621 (int.Parse of each of exactly 3 '.'-separated parts: FormatException / OverflowException)
VersionCompareResult CheckVersion(std::string_view versionString, int lastMajor, int lastMinor, int lastRev);
VersionCompareResult CheckVersion(int version_major, int version_minor, int version_revision, int lastMajor,
                                  int lastMinor, int lastRev);

// KSPUtil.cs:674 / 1675
struct StringReplacement {
    std::string badString;
    std::string replacement;
};
std::string ReplaceString(std::string src, std::span<const StringReplacement> replacements);

// KSPUtil.cs:1702-1736 (managed float maths, Mono float rule; v1 == v2 is Unity's approximate equality)
float HeadingRadians(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis);
float HeadingDegrees(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis);
float BearingRadians(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis);
float BearingDegrees(const Vector3f& v1, const Vector3f& v2, const Vector3f& upAxis);

// KSPUtil.cs:1738 / 1743 (separator between elements; stringAccessor = c.ToString() by default)
template <class T, class Accessor>
std::string PrintCollection(const std::vector<T>& collection, std::string_view separator, Accessor&& stringAccessor) {
    std::string text;
    const size_t num = collection.size();
    for (size_t i = 0; i < num; ++i) {
        text += stringAccessor(collection[i]);
        if (i != num - 1) text += separator;
    }
    return text;
}
std::string PrintCollection(const std::vector<std::string>& collection, std::string_view separator = ", ");

// KSPUtil.cs:1760 (string.Contains: ordinal substring)
std::string AppendValueToString(std::string_view s0, std::string_view val, char separator);

// KSPUtil.cs:1784-1952 (Matrix4x4 helpers; float stores per element)
Matrix4x4f Add(const Matrix4x4f& left, const Matrix4x4f& right);
void Add(Matrix4x4f& left, const Matrix4x4f& right);  // both `ref` overloads
Vector3d Diag(const Matrix4x4f& m);
Matrix4x4f ToDiagonalMatrix(float v);
void ToDiagonalMatrix(float v, Matrix4x4f& m);
void ToDiagonalMatrix2(float v, Matrix4x4f& m);
Matrix4x4f ToDiagonalMatrix(const Vector3f& v);
Matrix4x4f ToDiagonalMatrix2(const Vector3f& v, Matrix4x4f& m);
Matrix4x4f OuterProduct(const Vector3f& left, const Vector3f& right);
void OuterProduct(const Vector3f& left, const Vector3f& right, Matrix4x4f& m);
void OuterProduct2(const Vector3f& left, const Vector3f& right, Matrix4x4f& m);

// KSPUtil.cs:1954 DeepCompare(HashSet a, HashSet b): same count and every element of a contained in b
template <class Set>
bool DeepCompare(const Set& a, const Set& b) {
    if (a.size() != b.size()) return false;
    for (const auto& current : a)
        if (b.find(current) == b.end()) return false;
    return true;
}

// KSPUtil.cs:1975-2074: part / attach-node ids of craft files ("name_craftID", "nodeID,partName_craftID[_x|y|z...]")
// (ArgumentOutOfRangeException / IndexOutOfRange -> std::out_of_range where KSP's Substring / array index throw)
std::string GetPartName(std::string_view configPartID);
void GetPartInfo(std::string_view configPartID, std::string& partName, std::string& craftID);
bool GetAttachNodeInfo(std::string_view configAttnID, std::string& nodeID, std::string& attnPartID, Vector3f& attnPos);
bool GetAttachNodeInfo(std::string_view configAttnID, std::string& nodeID, std::string& attnPartID, Vector3f& attnPos,
                       std::string& attachMeshName);
bool GetAttachNodeInfo(std::string_view configAttnID, std::string& nodeID, std::string& attnPartID, Vector3f& attnPos,
                       std::string& attachMeshName, Vector3f& attnRot, Vector3f& secAxis);
bool GetAttachNodeInfo(std::string_view configAttnID, std::string& nodeID, std::string& attnPartID, Vector3f& attnPos,
                       Vector3f& attnRot, Vector3f& attnActualPos, Vector3f& attnActualRot);
std::string GetLinkID(std::string_view configAttnID);

// KSPUtil.cs:2117 (Guid.ToByteArray order; only bytes 0..2 are folded -- sic; int arithmetic wraps)
int GenerateSuperSeed(const Guid& guid, int seed);

}  // namespace KSPUtil

// ---- template definitions ----
template <class T>
std::vector<T> KSPUtil::ParseArray(std::string_view arrayString, const std::function<T(const std::string&)>& parser) {
    std::vector<T> out;
    size_t start = 0;
    while (true) {
        const size_t sep = arrayString.find(';', start);
        const std::string_view part =
            arrayString.substr(start, sep == std::string_view::npos ? std::string_view::npos : sep - start);
        out.push_back(parser(dotnet::Trim(part)));
        if (sep == std::string_view::npos) break;
        start = sep + 1;
    }
    return out;
}

}  // namespace ksp
