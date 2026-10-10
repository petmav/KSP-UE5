// WP-35: science data classes, ScienceUtil, RDTechTree (data side), ExperimentResultDialogPage (data side), RnDCommsStream
// (see ksp/science/science.hpp). ResearchAndDevelopment is in research_and_development.cpp.
#include "ksp/science/science.hpp"

#include <algorithm>

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {

using Loc = KSP::Localization::Localizer;

std::string Fmt(const char* tag, std::vector<std::string> args) { return Loc::Format(tag, args); }

bool Contains(const std::string& s, const std::string& sub) { return s.find(sub) != std::string::npos; }

// string.Split(string[] separator, StringSplitOptions.None) (Mono: left to right, at each position the first separator in array order that
// matches wins, empty entries kept)
std::vector<std::string> SplitByStrings(const std::string& s, const std::vector<std::string>& seps) {
    std::vector<std::string> parts;
    size_t start = 0, i = 0;
    while (i < s.size()) {
        bool hit = false;
        for (const std::string& sep : seps) {
            if (!sep.empty() && s.compare(i, sep.size(), sep) == 0) {
                parts.push_back(s.substr(start, i - start));
                i += sep.size();
                start = i;
                hit = true;
                break;
            }
        }
        if (!hit) i++;
    }
    parts.push_back(s.substr(start));
    return parts;
}

std::vector<std::string> SplitChar(const std::string& s, char c) {
    std::vector<std::string> parts;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); i++) {
        if (i == s.size() || s[i] == c) {
            parts.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    return parts;
}

std::string ReplaceAll(const std::string& s, const std::string& from, const std::string& to) { return dotnet::Replace(s, from, to); }

CelestialBody* FindBodyByName(const std::string& name) {
    if (FlightGlobals::bodies == nullptr) return nullptr;
    for (CelestialBody* b : *FlightGlobals::bodies)
        if (b->bodyName == name) return b;
    return nullptr;
}

GameParameters* CurrentParameters() {
    Game* g = SaveEnvironment::Current().currentGame;
    return g ? g->Parameters.get() : nullptr;
}

}  // namespace

// ----------------------------------------------------------------------------------------------------------------------------------
// ScienceEnvironment

double ScienceEnvironment::UniversalTime() { return universalTime ? universalTime() : SaveUniversalTime(); }

CoroutineScheduler& ScienceEnvironment::Scheduler() {
    if (scheduler != nullptr) return *scheduler;
    static CoroutineScheduler fallback;
    return fallback;
}

void ScienceEnvironment::Reset() {
    screenMessages.clear();
    getConfigNodes = nullptr;
    loadConfigFile = nullptr;
    loadedParts = nullptr;
    rndFacilityLevel = nullptr;
    experimentLevel = nullptr;
    rndFacilityDamage = nullptr;
    fundingInstanceExists = nullptr;
    currencyEffectDelta = nullptr;
    formatScienceReward = nullptr;
    commNetEnabled = nullptr;
    commNetBestTransmitter = nullptr;
    commNetScienceCurve = nullptr;
    biomeAtt = nullptr;
    shownPages.clear();
    displayResult = nullptr;
    resultDialogOpen = nullptr;
    partItemTransferActive = nullptr;
    overwriteDialog = nullptr;
    showTransferWarning = false;
    confirmDialog = nullptr;
    deltaTime = 0.02f;
    loadedSceneIsFlight = true;
    loadedSceneIsEditor = false;
    universalTime = nullptr;
    timeWarpDeltaTime = nullptr;
    systemRandomNext = nullptr;
    unityRandomRange = nullptr;
    scheduler = nullptr;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// ScienceExperiment

// ScienceExperiment.cs:38
void ScienceExperiment::Load(ConfigNode& node) {
    std::string s;
    if (node.TryGetValue("id", s)) id = s;
    node.TryGetValue("title", experimentTitle);
    node.TryGetValue("baseValue", baseValue);
    node.TryGetValue("scienceCap", scienceCap);
    node.TryGetValue("situationMask", situationMask);
    node.TryGetValue("biomeMask", biomeMask);
    node.TryGetValue("dataScale", dataScale);
    node.TryGetValue("requireAtmosphere", requireAtmosphere);
    node.TryGetValue("requireNoAtmosphere", requireNoAtmosphere);
    node.TryGetValue("requiredExperimentLevel", requiredExperimentLevel);
    applyScienceScale = true;
    node.TryGetValue("applyScienceScale", applyScienceScale);
    if (!node.HasNode("RESULTS")) return;
    ConfigNode* node2 = node.GetNode("RESULTS");
    int count = node2->values().Count();
    for (int i = 0; i < count; i++) {
        ConfigNode::Value& value = node2->values()[i];
        while (results_.ContainsKey(value.name)) value.name += "*";  // sic: renames the value inside the config node
        results_.Add(value.name, value.value);
    }
}

// ScienceExperiment.cs:69
void ScienceExperiment::Save(ConfigNode& node) const {
    node.AddValue("id", id ? *id : std::string());
    node.AddValue("title", experimentTitle);
    node.AddValue("baseValue", baseValue);
    node.AddValue("situationMask", situationMask);
    node.AddValue("biomeMask", biomeMask);
    node.AddValue("scienceCap", scienceCap);
    node.AddValue("dataScale", dataScale);
    node.AddValue("requireAtmosphere", requireAtmosphere);
    node.AddValue("requireNoAtmosphere", requireNoAtmosphere);
    node.AddValue("requiredExperimentLevel", requiredExperimentLevel);
    node.AddValue("applyScienceScale", applyScienceScale);
}

// ScienceExperiment.cs:84
bool ScienceExperiment::IsAvailableWhile(ExperimentSituations situation, const CelestialBody& body) const {
    if (requireAtmosphere) {
        if (body.atmosphere) return (situationMask & static_cast<uint32_t>(situation)) != 0;
        return false;
    }
    if (requireNoAtmosphere) {
        if (!body.atmosphere) return (situationMask & static_cast<uint32_t>(situation)) != 0;
        return false;
    }
    return (situationMask & static_cast<uint32_t>(situation)) != 0;
}

// ScienceExperiment.cs:105
bool ScienceExperiment::BiomeIsRelevantWhile(ExperimentSituations situation) const {
    return (biomeMask & static_cast<uint32_t>(situation)) != 0;
}

// ScienceExperiment.cs:110
bool ScienceExperiment::IsUnlocked() const {
    const float level = ScienceEnvironment::rndFacilityLevel ? ScienceEnvironment::rndFacilityLevel() : 1.0f;
    const float experimentLevel = ScienceEnvironment::experimentLevel ? ScienceEnvironment::experimentLevel(level) : level;
    return experimentLevel >= requiredExperimentLevel;
}

// ----------------------------------------------------------------------------------------------------------------------------------
// ScienceSubject

// ScienceSubject.cs:23
ScienceSubject::ScienceSubject(std::string id_, std::string title_, float dataScale_, float subjectValue_, float scienceCap_)
    : ScienceSubject(std::move(id_), std::move(title_), dataScale_, subjectValue_, scienceCap_, true) {}

// ScienceSubject.cs:28
ScienceSubject::ScienceSubject(std::string id_, std::string title_, float dataScale_, float subjectValue_, float scienceCap_,
                               bool applyScienceScale_) {
    id = std::move(id_);
    title = std::move(title_);
    dataScale = dataScale_;
    subjectValue = subjectValue_;
    scienceCap = scienceCap_;
    science = 0.0f;
    scientificValue = 1.0f;
    applyScienceScale = applyScienceScale_;
}

// ScienceSubject.cs:40
ScienceSubject::ScienceSubject(const ScienceExperiment& exp, ExperimentSituations sit, const CelestialBody& body, std::string biome,
                               std::string displaybiome) {
    if (Contains(biome, "#autoLOC")) biome = Loc::Format(biome);
    if (Contains(displaybiome, "#autoLOC")) displaybiome = Loc::Format(displaybiome);
    id = (exp.id ? *exp.id : std::string()) + "@" + body.bodyName + dotnet::EnumToString(sit) + ReplaceAll(biome, " ", "");
    title = ScienceUtil::GenerateScienceSubjectTitle(exp, sit, body, biome, displaybiome);
    switch (sit) {
    case ExperimentSituations::FlyingHigh: subjectValue = body.scienceValues.FlyingHighDataValue; break;
    case ExperimentSituations::SrfLanded: subjectValue = body.scienceValues.LandedDataValue; break;
    case ExperimentSituations::SrfSplashed: subjectValue = body.scienceValues.SplashedDataValue; break;
    case ExperimentSituations::FlyingLow: subjectValue = body.scienceValues.FlyingLowDataValue; break;
    default: subjectValue = 1.0f; break;
    case ExperimentSituations::InSpaceHigh: subjectValue = body.scienceValues.InSpaceHighDataValue; break;
    case ExperimentSituations::InSpaceLow: subjectValue = body.scienceValues.InSpaceLowDataValue; break;
    }
    dataScale = exp.dataScale;
    scientificValue = 1.0f;
    scienceCap = f32store(fx(exp.scienceCap) * fx(subjectValue));
    applyScienceScale = exp.applyScienceScale;
    science = 0.0f;
}

// ScienceSubject.cs:91
ScienceSubject::ScienceSubject(const ScienceExperiment& exp, ExperimentSituations sit, const std::string& sourceUid,
                               const std::string& sourceTitle, const CelestialBody& body, std::string biome, std::string displaybiome) {
    if (Contains(biome, "#autoLOC")) biome = Loc::Format(biome);
    if (Contains(displaybiome, "#autoLOC")) displaybiome = Loc::Format(displaybiome);
    else if (!displaybiome.empty()) displaybiome = ScienceUtil::GetBiomedisplayName(&body, displaybiome);
    id = (exp.id ? *exp.id : std::string()) + "@" + body.bodyName + dotnet::EnumToString(sit) +
         ((sourceUid != "") ? (ReplaceAll(biome, " ", "") + "_" + sourceUid) : std::string());
    title = ScienceUtil::GenerateScienceSubjectTitle(exp, sit, sourceUid, sourceTitle, body, biome, displaybiome);
    subjectValue = 1.0f;
    dataScale = exp.dataScale;
    scientificValue = 1.0f;
    scienceCap = f32store(fx(exp.scienceCap) * fx(subjectValue));
    science = 0.0f;
    applyScienceScale = exp.applyScienceScale;
}

// ScienceSubject.cs:123
ScienceSubject::ScienceSubject(ConfigNode& node) { Load(node); }

// ScienceSubject.cs:128
bool ScienceSubject::IsFromBody(const CelestialBody& cb) const { return Contains(id, "@" + cb.bodyName); }

// ScienceSubject.cs:133
bool ScienceSubject::IsFromSituation(ExperimentSituations situation) const { return Contains(id, dotnet::EnumToString(situation)); }

// ScienceSubject.cs:138
bool ScienceSubject::HasPartialIDstring(const std::string& pId) const { return Contains(id, pId); }

// ScienceSubject.cs:143
void ScienceSubject::Load(const ConfigNode& node) {
    if (node.HasValue("id")) id = *node.GetValue("id");
    title = ScienceUtil::GenerateLocalizedTitle(id);
    if (title.empty()) node.TryGetValue("title", title);
    node.TryGetValue("dsc", dataScale);
    node.TryGetValue("scv", scientificValue);
    node.TryGetValue("sbv", subjectValue);
    node.TryGetValue("sci", science);
    applyScienceScale = true;
    node.TryGetValue("asc", applyScienceScale);
    node.TryGetValue("cap", scienceCap);
}

// ScienceSubject.cs:163
void ScienceSubject::Save(ConfigNode& node) const {
    node.AddValue("id", id);
    node.AddValue("title", title);
    node.AddValue("dsc", dataScale);
    node.AddValue("scv", scientificValue);
    node.AddValue("sbv", subjectValue);
    node.AddValue("sci", science);
    node.AddValue("asc", applyScienceScale);
    node.AddValue("cap", scienceCap);
}

// ----------------------------------------------------------------------------------------------------------------------------------
// ScienceData

// ScienceData.cs:23
ScienceData::ScienceData(const ConfigNode& node) { Load(node); }

// ScienceData.cs:28
ScienceData::ScienceData(float amount, float xmitValue, float xmitBonus, const std::string& id, const std::string& dataName, bool triggered_,
                         uint32_t container_)
    : ScienceData(amount, 1.0f, xmitValue, xmitBonus, id, dataName, triggered_, container_) {}

// ScienceData.cs:33
ScienceData::ScienceData(float amount, float scienceValueRatio_, float xmitValue, float xmitBonus, const std::string& id,
                         const std::string& dataName, bool triggered_, uint32_t container_, const std::string& extraResultString_) {
    dataAmount = amount;
    subjectID = id;
    baseTransmitValue = xmitValue;
    transmitBonus = xmitBonus;
    title = dataName;
    triggered = triggered_;
    container = container_;
    scienceValueRatio = scienceValueRatio_;
    extraResultString = extraResultString_;
}

// ScienceData.cs:46
void ScienceData::Load(const ConfigNode& node) {
    node.TryGetValue("data", dataAmount);
    scienceValueRatio = 1.0f;
    node.TryGetValue("scienceValueRatio", scienceValueRatio);
    node.TryGetValue("subjectID", subjectID);
    node.TryGetValue("xmit", baseTransmitValue);
    node.TryGetValue("xmitBonus", transmitBonus);
    const ScienceExperiment* scienceExperiment = nullptr;
    ScienceExperiment blank;
    scienceExperiment = &blank;  // new ScienceExperiment()
    if (!Contains(subjectID, "@")) {
        scienceExperiment = ResearchAndDevelopment::GetExperiment(subjectID);
        if (scienceExperiment == nullptr) throw NullReferenceException("ScienceData.Load: no experiment " + subjectID);
    }
    if (scienceExperiment->id.has_value()) {
        title = ResearchAndDevelopment::GetExperiment(subjectID)->experimentTitle;
    } else {
        title = ScienceUtil::GenerateLocalizedTitle(subjectID, true);
        if (title.empty()) node.TryGetValue("title", title);
    }
    node.TryGetValue("triggered", triggered);
    node.TryGetValue("container", container);
    node.TryGetValue("extraResultString", extraResultString);
}

// ScienceData.cs:76
void ScienceData::Save(ConfigNode& node) const {
    node.AddValue("data", dataAmount);
    node.AddValue("scienceValueRatio", scienceValueRatio);
    node.AddValue("subjectID", subjectID);
    node.AddValue("xmit", baseTransmitValue);
    node.AddValue("xmitBonus", transmitBonus);
    node.AddValue("title", title);
    node.AddValue("triggered", triggered);
    node.AddValue("container", container);
    if (!extraResultString.empty()) node.AddValue("extraResultString", extraResultString);
}

// ScienceData.cs:92
std::shared_ptr<ScienceData> ScienceData::CopyOf(const ScienceData* src) {
    if (src != nullptr)
        return std::make_shared<ScienceData>(src->dataAmount, src->scienceValueRatio, src->baseTransmitValue, src->transmitBonus, src->subjectID,
                                             src->title, src->triggered, src->container, src->extraResultString);
    return nullptr;
}

// TechPerk.cs:14
void TechPerk::SetPerk(bool) { throw std::logic_error("NotImplementedException: TechPerk.SetPerk"); }

// TechPerk.cs:19
void TechPerk::GetPerkState() { throw std::logic_error("NotImplementedException: TechPerk.GetPerkState"); }

// ----------------------------------------------------------------------------------------------------------------------------------
// ScienceUtil

// ScienceUtil.cs:11
ExperimentSituations ScienceUtil::GetExperimentSituation(Vessel& v) {
    CelestialBody* mb = v.mainBody();
    switch (v.situation) {
    case VesselSituations::FLYING:
        if (!(v.altitude < static_cast<double>(mb->scienceValues.flyingAltitudeThreshold))) return ExperimentSituations::FlyingHigh;
        return ExperimentSituations::FlyingLow;
    case VesselSituations::SPLASHED: return ExperimentSituations::SrfSplashed;
    case VesselSituations::LANDED:
    case VesselSituations::PRELAUNCH: return ExperimentSituations::SrfLanded;
    default:
        if (!(v.altitude < static_cast<double>(mb->scienceValues.spaceAltitudeThreshold))) return ExperimentSituations::InSpaceHigh;
        return ExperimentSituations::InSpaceLow;
    }
}

// ScienceUtil.cs:35
std::string ScienceUtil::GetExperimentBiome(const CelestialBody& body, double lat, double lon) {
    if (body.BiomeMap.has_value()) {
        if (!ScienceEnvironment::biomeAtt) return std::string();
        const CBAttributeMapSO::MapAttribute* a = ScienceEnvironment::biomeAtt(body, lat * 0.01745329238474369, lon * 0.01745329238474369);
        return a ? a->name : std::string();
    }
    return std::string();
}

// ScienceUtil.cs:44
std::string ScienceUtil::GetExperimentBiomeLocalized(const CelestialBody& body, double lat, double lon) {
    if (body.BiomeMap.has_value()) {
        if (!ScienceEnvironment::biomeAtt) return std::string();
        const CBAttributeMapSO::MapAttribute* a = ScienceEnvironment::biomeAtt(body, lat * 0.01745329238474369, lon * 0.01745329238474369);
        return a ? a->displayname : std::string();
    }
    return std::string();
}

// ScienceUtil.cs:53
std::string ScienceUtil::GetExperimentBodyName(const std::string& subjectID) {
    std::string result = "";
    try {
        std::vector<std::string> array = SplitChar(subjectID, '@');
        if (array.size() != 2) return result;
        std::vector<std::string> names = dotnet::EnumGetNames<ExperimentSituations>();
        result = SplitByStrings(array[1], names)[0];
    } catch (...) {
    }
    return result;
}

// ScienceUtil.cs:72
void ScienceUtil::GetExperimentFieldsFromScienceID(const std::string& subjectID, std::string& BodyName, ExperimentSituations& Situation,
                                                   std::string& Biome) {
    std::string Situation2;
    Situation = ExperimentSituations::SrfLanded;
    GetExperimentFieldsFromScienceID(subjectID, BodyName, Situation2, Biome);
    ExperimentSituations experimentSituations = ExperimentSituations::SrfLanded;
    try {
        experimentSituations = dotnet::EnumParse<ExperimentSituations>(Situation2);
    } catch (const std::exception& ex) {
        log::Error("[ScienceSubject] - Error in converting Situation " + Situation2 + ex.what());
        return;
    }
    Situation = experimentSituations;
}

// ScienceUtil.cs:90
void ScienceUtil::GetExperimentFieldsFromScienceID(const std::string& subjectID, std::string& BodyName, std::string& Situation,
                                                   std::string& Biome) {
    BodyName = "";
    Situation = "";
    Biome = "";
    std::vector<std::string> array = SplitChar(subjectID, '@');
    if (array.size() != 2 || ResearchAndDevelopment::GetExperiment(array[0]) == nullptr) return;
    if (Contains(array[1], "'_")) {
        size_t num = array[1].find('_');
        array[1] = array[1].substr(0, num + 1);
    }
    std::vector<std::string> names = dotnet::EnumGetNames<ExperimentSituations>();
    std::vector<std::string> array2 = SplitByStrings(array[1], names);
    std::string text;
    text = (array2.size() != 1) ? array[1].substr(array2[0].size(), array[1].size() - array2[0].size() - array2[1].size())
                                : array[1].substr(array2[0].size(), array[1].size() - array2[0].size());
    Situation = text;
    if (FlightGlobals::bodies != nullptr) {
        for (size_t i = 0; i < FlightGlobals::bodies->size(); i++) {
            if (array2[0] == (*FlightGlobals::bodies)[i]->bodyName) {
                BodyName = (*FlightGlobals::bodies)[i]->bodyName;
                break;
            }
        }
    }
    if (array2.size() == 2) Biome = array2[1];
}

// ScienceUtil.cs:124
std::string ScienceUtil::GenerateLocalizedTitle(const std::string& id) { return GenerateLocalizedTitle(id, false); }

// ScienceUtil.cs:129
std::string ScienceUtil::GenerateLocalizedTitle(const std::string& id, bool skipAsteroidsComets) {
    std::string result = "";
    std::vector<std::string> array = SplitChar(id, '@');
    if (array.size() != 2) return result;
    if (array[0] == "recovery") return generateRecoveryLocalizedTitle(id);
    ScienceExperiment* scienceExperiment = ResearchAndDevelopment::GetExperiment(array[0]);
    if (scienceExperiment == nullptr) return result;
    if (Contains(array[1], "'_")) {
        size_t num = array[1].find('_');
        array[1] = array[1].substr(0, num + 1);
    }
    std::vector<std::string> names = dotnet::EnumGetNames<ExperimentSituations>();
    std::vector<std::string> array2 = SplitByStrings(array[1], names);
    if (array2.size() < 2) throw std::out_of_range("ScienceUtil.GenerateLocalizedTitle: array2[1] (IndexOutOfRangeException)");
    if (!array2[1].empty() && array2[1][0] == '_') {
        if (skipAsteroidsComets) return "";
        array2[1] = array2[1].substr(1, array2[1].size() - 1);
    }
    std::string text;
    text = (array2.size() != 1) ? array[1].substr(array2[0].size(), array[1].size() - array2[0].size() - array2[1].size())
                                : array[1].substr(array2[0].size(), array[1].size() - array2[0].size());
    ExperimentSituations experimentSituations = ExperimentSituations::SrfLanded;
    try {
        experimentSituations = dotnet::EnumParse<ExperimentSituations>(text);
    } catch (const std::exception& ex) {
        log::Error("[ScienceSubject] - Error in converting Situation " + text + ex.what());
        return result;
    }
    CelestialBody* celestialBody = FindBodyByName(array2[0]);
    if (celestialBody != nullptr)
        result = GenerateScienceSubjectTitle(*scienceExperiment, experimentSituations, *celestialBody, (array2.size() == 2) ? array2[1] : std::string());
    return result;
}

// ScienceUtil.cs:190
std::string ScienceUtil::generateRecoveryLocalizedTitle(const std::string& id) {
    std::string result = "";
    std::vector<std::string> array = SplitChar(id, '@');
    ScienceExperiment* scienceExperiment = ResearchAndDevelopment::GetExperiment(array[0]);
    if (scienceExperiment == nullptr) return result;
    std::vector<std::string> names = dotnet::EnumGetNames<RecoverySituations>();
    std::vector<std::string> array2 = SplitByStrings(array[1], names);
    std::string text;
    text = (array2.size() != 1) ? array[1].substr(array2[0].size(), array[1].size() - array2[0].size() - array2[1].size())
                                : array[1].substr(array2[0].size(), array[1].size() - array2[0].size());
    RecoverySituations recoverySituations = RecoverySituations::Flew;
    try {
        recoverySituations = dotnet::EnumParse<RecoverySituations>(text);
    } catch (const std::exception& ex) {
        log::Error("[ScienceSubject] - Error in converting Situation " + text + ex.what());
        return result;
    }
    CelestialBody* celestialBody = FindBodyByName(array2[0]);
    if (celestialBody != nullptr) result = GenerateScienceSubjectRecoveryTitle(*scienceExperiment, recoverySituations, *celestialBody);
    return result;
}

// ScienceUtil.cs:230
std::string ScienceUtil::GenerateScienceSubjectTitle(const ScienceExperiment& exp, ExperimentSituations sit, const CelestialBody& body,
                                                     const std::string& biome, const std::string& displaybiome) {
    std::string result = "";
    try {
        const std::string displayName = body.bodyDisplayName;
        const bool flag = BiomeIsUnlisted(&body, biome);
        const std::string empty = (!displaybiome.empty()) ? displaybiome : GetBiomedisplayName(&body, biome);
        const std::string& t = exp.experimentTitle;
        const bool hasBiome = !biome.empty();
        switch (sit) {
        case ExperimentSituations::FlyingHigh:
            result = !hasBiome ? Fmt("#autoLOC_301735", {t, displayName}) : Fmt("#autoLOC_301731", {t, displayName, empty});
            break;
        case ExperimentSituations::SrfLanded:
            result = !hasBiome ? Fmt("#autoLOC_301698", {t, displayName})
                               : (!flag ? Fmt("#autoLOC_301693", {t, displayName, empty}) : Fmt("#autoLOC_301689", {t, empty}));
            break;
        case ExperimentSituations::SrfSplashed:
            result = !hasBiome ? Fmt("#autoLOC_301711", {t, displayName}) : Fmt("#autoLOC_301707", {t, displayName, empty});
            break;
        case ExperimentSituations::FlyingLow:
            result = !hasBiome ? Fmt("#autoLOC_301723", {t, displayName}) : Fmt("#autoLOC_301719", {t, displayName, empty});
            break;
        default:
            result = !hasBiome ? Fmt("#autoLOC_301775", {t, displayName}) : Fmt("#autoLOC_301771", {t, displayName, empty});
            break;
        case ExperimentSituations::InSpaceHigh:
            result = !hasBiome ? Fmt("#autoLOC_301762", {t, displayName}) : Fmt("#autoLOC_301758", {t, displayName, empty});
            break;
        case ExperimentSituations::InSpaceLow:
            result = !hasBiome ? Fmt("#autoLOC_301748", {t, displayName}) : Fmt("#autoLOC_301744", {t, displayName, empty});
            break;
        }
    } catch (const std::exception& ex) {
        log::Error("[Science] - Unable to generate science subject " + (exp.id ? *exp.id : std::string()) + " " + dotnet::EnumToString(sit) + " " + biome +
                   ex.what());
    }
    return result;
}

// ScienceUtil.cs:271
std::string ScienceUtil::GenerateScienceSubjectRecoveryTitle(const ScienceExperiment& exp, RecoverySituations sit, const CelestialBody& body) {
    std::string result = "";
    try {
        const std::string text = Fmt("#autoLOC_7001301", {body.bodyDisplayName});
        // sit.displayDescription() = the [Description] tag of the member
        const std::optional<std::string> desc = dotnet::EnumMemberDescription<RecoverySituations>(dotnet::EnumToString(sit));
        result = Fmt(desc ? desc->c_str() : "", {text});
    } catch (const std::exception& ex) {
        log::Error("[Science] - Unable to generate science subject " + (exp.id ? *exp.id : std::string()) + " " + dotnet::EnumToString(sit) + ex.what());
    }
    return result;
}

// ScienceUtil.cs:286
std::string ScienceUtil::GenerateScienceSubjectTitle(const ScienceExperiment& exp, ExperimentSituations sit, const std::string& sourceUid,
                                                     const std::string& sourceTitle, const CelestialBody& body, const std::string& biome,
                                                     const std::string& displaybiome) {
    (void)sourceUid;
    std::string result = "";
    try {
        const std::string text = Fmt("#autoLOC_7001301", {body.bodyDisplayName});
        const bool flag = BiomeIsUnlisted(&body, biome);
        const std::string empty = (!displaybiome.empty()) ? displaybiome : GetBiomedisplayName(&body, biome);
        const std::string& t = exp.experimentTitle;
        const bool hasBiome = !biome.empty();
        switch (sit) {
        case ExperimentSituations::FlyingHigh:
            result = !hasBiome ? Fmt("#autoLOC_301844", {t, sourceTitle, text}) : Fmt("#autoLOC_301840", {t, sourceTitle, text, empty});
            break;
        case ExperimentSituations::SrfLanded:
            result = !hasBiome ? Fmt("#autoLOC_301811", {t, sourceTitle, text})
                               : (!flag ? Fmt("#autoLOC_301806", {t, sourceTitle, text, empty}) : Fmt("#autoLOC_301802", {t, sourceTitle, empty}));
            break;
        case ExperimentSituations::SrfSplashed:
            result = !hasBiome ? Fmt("#autoLOC_301822", {t, sourceTitle, text}) : Fmt("#autoLOC_301818", {t, sourceTitle, text, empty});
            break;
        case ExperimentSituations::FlyingLow:
            result = !hasBiome ? Fmt("#autoLOC_301833", {t, sourceTitle, text}) : Fmt("#autoLOC_301829", {t, sourceTitle, text, empty});
            break;
        default:
            result = !hasBiome ? Fmt("#autoLOC_301881", {t, sourceTitle, text}) : Fmt("#autoLOC_301877", {t, sourceTitle, text, empty});
            break;
        case ExperimentSituations::InSpaceHigh:
            result = !hasBiome ? Fmt("#autoLOC_301869", {t, sourceTitle, text}) : Fmt("#autoLOC_301865", {t, sourceTitle, text, empty});
            break;
        case ExperimentSituations::InSpaceLow:
            result = !hasBiome ? Fmt("#autoLOC_301856", {t, sourceTitle, text}) : Fmt("#autoLOC_301852", {t, sourceTitle, text, empty});
            break;
        }
    } catch (const std::exception& ex) {
        log::Error("[Science] - Unable to generate science subject " + (exp.id ? *exp.id : std::string()) + " " + dotnet::EnumToString(sit) + " " + biome +
                   ex.what());
    }
    return result;
}

// ScienceUtil.cs:327
float ScienceUtil::GetTransmitterScore(IScienceDataTransmitter& t) {
    // 1f / t.DataRate * (1f + (float)t.DataResourceCost) * (t.IsBusy() ? 100f : 1f) * (t.CanTransmit() ? 1f : 100000f)
    const float dataRate = t.DataRate();
    const float cost = static_cast<float>(t.DataResourceCost());
    const bool busy = t.IsBusy();
    const bool can = t.CanTransmit();
    return f32store(1.0 / fx(dataRate) * (1.0 + fx(cost)) * (busy ? 100.0 : 1.0) * (can ? 1.0 : 100000.0));
}

// ScienceUtil.cs:333
IScienceDataTransmitter* ScienceUtil::GetBestTransmitter(std::vector<IScienceDataTransmitter*>& vesselTransmitters) {
    int index = 0;
    float num = std::numeric_limits<float>::max();
    int count = static_cast<int>(vesselTransmitters.size());
    while (count-- > 0) {
        IScienceDataTransmitter* t = vesselTransmitters[static_cast<size_t>(count)];
        if (t->CanTransmit()) {
            float transmitterScore = GetTransmitterScore(*t);
            if (transmitterScore < num) {
                num = transmitterScore;
                index = count;
            }
        }
    }
    return vesselTransmitters.at(static_cast<size_t>(index));
}

// ScienceUtil.cs:354
IScienceDataTransmitter* ScienceUtil::GetBestTransmitter(Vessel& v) {
    IScienceDataTransmitter* commNet = nullptr;
    if (ScienceEnvironment::commNetBestTransmitter && ScienceEnvironment::commNetBestTransmitter(v, commNet)) return commNet;
    IScienceDataTransmitter* result = nullptr;
    float num = std::numeric_limits<float>::max();
    int count = static_cast<int>(v.Parts().size());
    while (count-- > 0) {
        Part* part = v.Parts()[static_cast<size_t>(count)];
        int count2 = static_cast<int>(part->Modules().Count());
        while (count2-- > 0) {
            auto* t = dynamic_cast<IScienceDataTransmitter*>(part->Modules()[static_cast<size_t>(count2)]);
            if (t != nullptr && t->CanTransmit()) {
                float transmitterScore = GetTransmitterScore(*t);
                if (transmitterScore < num) {
                    num = transmitterScore;
                    result = t;
                }
            }
        }
    }
    return result;
}

// ScienceUtil.cs:383
float ScienceUtil::GetLabScore(ModuleScienceLab&) { return 1.0f; }

namespace {
// ScienceUtil.cs:437
bool CrewListHasType(const std::vector<std::shared_ptr<ProtoCrewMember>>& crew, ProtoCrewMember_KerbalType type) {
    int count = static_cast<int>(crew.size());
    do {
        if (count-- <= 0) return false;
    } while (crew[static_cast<size_t>(count)]->type() != type);
    return true;
}
// ScienceUtil.cs:451
template <class T>
bool CrewListHasEffect(const std::vector<std::shared_ptr<ProtoCrewMember>>& crew) {
    int count = static_cast<int>(crew.size());
    do {
        if (count-- <= 0) return false;
    } while (!crew[static_cast<size_t>(count)]->HasEffect<T>());
    return true;
}
}  // namespace

// ScienceUtil.cs:388
bool ScienceUtil::RequiredUsageInternalAvailable(Vessel& v, Part& p, ExperimentUsageReqs req, const ScienceExperiment& exp, std::string& message) {
    const int r = static_cast<int>(req);
    if (req == ExperimentUsageReqs::Never) {
        message = std::string();
        return false;
    }
    if ((r & static_cast<int>(ExperimentUsageReqs::VesselControl)) != 0 && !v.IsControllable()) {
        message = Fmt("#autoLOC_302066", {exp.experimentTitle});
        return false;
    }
    if ((r & static_cast<int>(ExperimentUsageReqs::CrewInVessel)) != 0 && !CrewListHasType(v.GetVesselCrew(), ProtoCrewMember_KerbalType::Crew)) {
        message = Fmt("#autoLOC_302075", {exp.experimentTitle});
        return false;
    }
    if ((r & static_cast<int>(ExperimentUsageReqs::CrewInPart)) != 0 && !CrewListHasType(p.protoModuleCrew, ProtoCrewMember_KerbalType::Crew)) {
        message = Fmt("#autoLOC_302084", {exp.experimentTitle, p.partInfo->title});
        return false;
    }
    if ((r & static_cast<int>(ExperimentUsageReqs::ScientistCrew)) != 0) {
        if ((r & static_cast<int>(ExperimentUsageReqs::CrewInPart)) != 0) {
            if (!CrewListHasEffect<Experience::Effects::SpecialExperimentSkill>(p.protoModuleCrew)) {
                message = Fmt("#autoLOC_302095", {exp.experimentTitle, p.partInfo->title});
                return false;
            }
        } else {
            if ((r & static_cast<int>(ExperimentUsageReqs::CrewInVessel)) == 0) {
                message = Fmt("#autoLOC_302109", {exp.experimentTitle});
                return false;
            }
            if (!CrewListHasEffect<Experience::Effects::SpecialExperimentSkill>(v.GetVesselCrew())) {
                message = Fmt("#autoLOC_302103", {exp.experimentTitle});
                return false;
            }
        }
    }
    return true;
}

// ScienceUtil.cs:465
bool ScienceUtil::RequiredUsageExternalAvailable(Vessel& v, Vessel& vExt, ExperimentUsageReqs req, const ScienceExperiment& exp, std::string& message) {
    const int r = static_cast<int>(req);
    if (req == ExperimentUsageReqs::Never) {
        message = std::string();
        return false;
    }
    if (!vExt.isEVA()) {
        if ((r & static_cast<int>(ExperimentUsageReqs::ScientistCrew)) != 0) message = Fmt("#autoLOC_302151", {exp.experimentTitle});
        else message = Fmt("#autoLOC_302155", {exp.experimentTitle});
        return false;
    }
    if ((r & static_cast<int>(ExperimentUsageReqs::VesselControl)) != 0 && v.CurrentControlLevel() == ControlLevel::NONE) {
        message = Fmt("#autoLOC_302164", {exp.experimentTitle});
        return false;
    }
    const std::vector<std::shared_ptr<ProtoCrewMember>>& vesselCrew = vExt.GetVesselCrew();
    std::shared_ptr<ProtoCrewMember> protoCrewMember;
    if (!vesselCrew.empty()) protoCrewMember = vesselCrew[0];
    if (protoCrewMember == nullptr) {
        message = Fmt("#autoLOC_302177", {exp.experimentTitle});
        return false;
    }
    if ((r & static_cast<int>(ExperimentUsageReqs::ScientistCrew)) != 0) {
        if (protoCrewMember->type() == ProtoCrewMember_KerbalType::Crew && !protoCrewMember->HasEffect<Experience::Effects::ExternalExperimentSkill>()) {
            message = Fmt("#autoLOC_302187", {exp.experimentTitle});
            return false;
        }
    } else if (protoCrewMember->type() != ProtoCrewMember_KerbalType::Crew) {
        message = Fmt("#autoLOC_302196", {exp.experimentTitle});
        return false;
    }
    return true;
}

// ScienceUtil.cs:516
bool ScienceUtil::BiomeIsUnlisted(const CelestialBody* body, const std::string& biome) {
    if (body != nullptr && body->BiomeMap.has_value()) {
        const auto& attributes = body->BiomeMap->Attributes;
        int num = static_cast<int>(attributes.size());
        // do { if (num-- > 0) { if (name == biome) return false; continue; } return true; } while (!name.Contains(" ") || !(replaced == biome));
        while (true) {
            if (num-- > 0) {
                const std::string& name = attributes[static_cast<size_t>(num)].name;
                if (name == biome) return false;
                if (!Contains(name, " ") || !(ReplaceAll(name, " ", "") == biome)) continue;
                break;
            }
            return true;
        }
        return false;
    }
    return false;
}

// ScienceUtil.cs:540
std::string ScienceUtil::GetBiomedisplayName(const CelestialBody* body, const std::string& biome) { return GetBiomedisplayName(body, biome, true); }

// ScienceUtil.cs:545
std::string ScienceUtil::GetBiomedisplayName(const CelestialBody* body, const std::string& biome, bool formatted) {
    if (body != nullptr && body->BiomeMap.has_value()) {
        const auto& attributes = body->BiomeMap->Attributes;
        int num = static_cast<int>(attributes.size());
        while (num-- > 0) {
            const auto& a = attributes[static_cast<size_t>(num)];
            if (!(a.name == biome)) {
                if (Contains(a.name, " ") && ReplaceAll(a.name, " ", "") == biome) return formatted ? a.displayname : a.localizationTag;
                continue;
            }
            return formatted ? a.displayname : a.localizationTag;
        }
    }
    return ResearchAndDevelopment::GetMiniBiomedisplayNameByScienceID(biome, formatted);
}

}  // namespace ksp
