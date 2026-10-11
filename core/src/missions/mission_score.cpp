// WP-54: ScoreModule*.cs, ScoreRange.cs, ScoreInfo.cs, MissionScore.cs. See mission_score.hpp.
// Float rule (README "Mono float32 rule"): AwardScore / `currentScore += x` are managed float expressions: f32store(fx(a) op fx(b)).
#include "ksp/missions/mission_score.hpp"

#include <algorithm>
#include <cmath>

#include "../save/save_util.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission.hpp"
#include "ksp/missions/mission_awards.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::Expansions::Missions {

using KSP::Localization::Localizer;

namespace {
// float.Equals(float): NaN equals NaN
bool FloatEquals(float a, float b) { return a == b || (a != a && b != b); }
std::string F(float v) { return dotnet::FormatSingleG(v); }
std::string D(double v) { return dotnet::FormatDoubleG(v); }

// double.ToString("00") for non-negative values: rounded half away from zero, at least two digits.
// UNVERIFIED against Mono for negative numbers and for exact halves; the callers pass percentages and resource amounts.
std::string FormatDouble00(double v) {
    double r = std::floor(std::fabs(v) + 0.5);
    std::string s = dotnet::FormatDoubleF(r, 0);
    if (s.size() < 2) s = "0" + s;
    if (v < 0.0 && r != 0.0) s = "-" + s;
    return s;
}

std::vector<std::string>& GlobalScoreModules() {
    static std::vector<std::string> v;
    return v;
}
bool& GlobalScoreModulesSet() {
    static bool b = false;
    return b;
}
void SetDefaultGlobalScoreModules() {
    GlobalScoreModules().clear();
    GlobalScoreModules().push_back("ScoreModule_Completion");
    GlobalScoreModules().push_back("ScoreModule_Modifier");
    GlobalScoreModulesSet() = true;
}

// scoreRanges loader shared by the Time / Accuracy / Resource modules
void LoadRanges(const ConfigNode& node, std::vector<ScoreRange>& ranges) {
    if (node.HasNode("SCORERANGES")) {
        for (const ConfigNode* configNode : node.GetNode("SCORERANGES")->GetNodes("SCORERANGE")) {
            ScoreRange scoreRange;
            scoreRange.Load(*configNode);
            ranges.push_back(scoreRange);
        }
    }
}
void SaveRanges(ConfigNode& node, const std::vector<ScoreRange>& ranges) {
    ConfigNode* configNode = node.AddNode("SCORERANGES");
    for (size_t i = 0; i < ranges.size(); ++i) ranges[i].Save(*configNode->AddNode("SCORERANGE"));
}
bool RangesEqual(const std::vector<ScoreRange>& a, const std::vector<ScoreRange>& b) {
    // MissionCheckpointValidator.CompareObjectLists -> CompareAnyLists (MissionCheckpointValidator.cs:164): count, then list1[i].Equals(list2[i])
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (!a[i].Equals(b[i])) return false;
    return true;
}
}  // namespace

// ---- ScoreRange --------------------------------------------------------------------------------------------------------------

// ScoreRange.cs:23
bool ScoreRange::isValueInRange(double value) const {
    if (value >= minRange) return value <= maxRange;
    return false;
}

// ScoreRange.cs:30
bool ScoreRange::Equals(const ScoreRange& other) const {
    if (UtilMath::Approximately(minRange, other.minRange) && UtilMath::Approximately(maxRange, other.maxRange)) return Mathf::Approximately(score, other.score);
    return false;
}

// ScoreRange.cs:46
void ScoreRange::Load(const ConfigNode& node) {
    node.TryGetValue("minRange", minRange);
    node.TryGetValue("maxRange", maxRange);
    node.TryGetValue("score", score);
}

// ScoreRange.cs:53
void ScoreRange::Save(ConfigNode& node) const {
    node.AddValue("minRange", minRange);
    node.AddValue("maxRange", maxRange);
    node.AddValue("score", score);
}

// ---- ScoreInfo ---------------------------------------------------------------------------------------------------------------

// ScoreInfo.cs:19
ScoreInfo::ScoreInfo() {
    id = MissionsEnvironment::NewGuid();
    score = 0.0f;
    timeTaken = 0.0;
    resultsText = "";
}

// ScoreInfo.cs:28
void ScoreInfo::Load(const ConfigNode& node) {
    node.TryGetValue("id", id);
    node.TryGetValue("missionId", missionId);
    node.TryGetValue("score", score);
    node.TryGetValue("timeTaken", timeTaken);
    node.TryGetValue("resultsText", resultsText);
    int64_t value = 0;
    completedAt.reset();
    if (node.TryGetValue("completedAt", value)) completedAt = value;
    awardIds = node.GetValuesList("awardId");
}

// ScoreInfo.cs:43
void ScoreInfo::Save(ConfigNode& node) const {
    node.AddValue("id", id.ToString());
    node.AddValue("missionId", missionId.ToString());
    node.AddValue("score", score);
    node.AddValue("timeTaken", timeTaken);
    const std::string value = dotnet::Replace(dotnet::Replace(resultsText, "\n", "\\n"), "\t", "\\t");
    node.AddValue("resultsText", value);
    if (completedAt.has_value()) node.AddValue("completedAt", *completedAt);
    for (size_t i = 0; i < awardIds.size(); ++i) node.AddValue("awardId", awardIds[i]);
}

// ---- ScoreModule -------------------------------------------------------------------------------------------------------------

// ScoreModule.cs:10
IScoreableObjective* ScoreModule::scoreableObjective() {
    if (node->dockParentNode != nullptr) return dynamic_cast<IScoreableObjective*>(node->dockParentNode->testGroups[0]->testModules[0].get());
    return nullptr;
}

// ScoreModule.cs:32
float ScoreModule::AwardScore(float currentScore) { return currentScore; }

// ScoreModule.cs:37
std::string ScoreModule::ScoreDescription() { return ""; }

// ScoreModule.cs:42
std::string ScoreModule::AwarededScoreDescription() { return awardedScoreDescription; }

// ScoreModule.cs:52
void ScoreModule::Load(const ConfigNode& node) {
    DynamicModule::Load(node);
    node.TryGetValue("awardedScoreDescription", awardedScoreDescription);
}

// ScoreModule.cs:58
void ScoreModule::Save(ConfigNode& node) const {
    DynamicModule::Save(node);
    if (!awardedScoreDescription.empty()) node.AddValue("awardedScoreDescription", awardedScoreDescription);
}

// ---- ScoreModule_Completion --------------------------------------------------------------------------------------------------

// ScoreModule_Completion.cs:24
std::string ScoreModule_Completion::GetDisplayName() { return Localizer::Format("#autoLOC_8100148"); }

// ScoreModule_Completion.cs:29
std::string ScoreModule_Completion::GetDisplayToolTip() { return "#autoLOC_8001025"; }

// ScoreModule_Completion.cs:34
float ScoreModule_Completion::AwardScore(float currentScore) {
    awardedScoreDescription = Localizer::Format("#autoLOC_8006057", {F(bonusScore)});
    currentScore = f32store(fx(currentScore) + fx(bonusScore));
    return currentScore;
}

// ScoreModule_Completion.cs:41
std::string ScoreModule_Completion::ScoreDescription() { return Localizer::Format("#autoLOC_8100149", {GetDisplayName(), F(bonusScore)}); }

// ScoreModule_Completion.cs:46
std::vector<std::string> ScoreModule_Completion::GetDefaultPinnedParameters() { return {"bonusScore"}; }

// ScoreModule_Completion.cs:51
bool ScoreModule_Completion::EqualsObject(const DynamicModule& other) const {
    const auto* o = dynamic_cast<const ScoreModule_Completion*>(&other);
    if (o == nullptr) return false;
    if (name() == o->name()) return FloatEquals(bonusScore, o->bonusScore);
    return false;
}

// ScoreModule_Completion.cs:65
std::string ScoreModule_Completion::GetInfo() { return Localizer::Format("#autoLOC_8004160"); }

// ScoreModule_Completion.cs:70
void ScoreModule_Completion::Load(const ConfigNode& node) {
    ScoreModule::Load(node);
    node.TryGetValue("bonusScore", bonusScore);
}

// ScoreModule_Completion.cs:76
void ScoreModule_Completion::Save(ConfigNode& node) const {
    ScoreModule::Save(node);
    node.AddValue("bonusScore", bonusScore);
}

// ---- ScoreModule_Modifier ----------------------------------------------------------------------------------------------------

// ScoreModule_Modifier.cs:29
std::string ScoreModule_Modifier::GetDisplayName() { return "#autoLOC_8100125"; }

// ScoreModule_Modifier.cs:34
std::string ScoreModule_Modifier::GetDisplayToolTip() { return "#autoLOC_8001024"; }

// ScoreModule_Modifier.cs:39
float ScoreModule_Modifier::AwardScore(float currentScore) {
    switch (modifierType) {
        default:
            return currentScore;
        case ScoreModifierType::Multiply:
            awardedScoreDescription = Localizer::Format("#autoLOC_8006052", {F(modifierScore)});
            return f32store(fx(currentScore) * fx(modifierScore));
        case ScoreModifierType::Divide:
            awardedScoreDescription = Localizer::Format("#autoLOC_8006053", {F(modifierScore)});
            return f32store(fx(currentScore) / fx(modifierScore));
        case ScoreModifierType::Substract:
            awardedScoreDescription = Localizer::Format("#autoLOC_8006054", {F(modifierScore)});
            return f32store(fx(currentScore) - fx(modifierScore));
        case ScoreModifierType::Set:
            awardedScoreDescription = Localizer::Format("#autoLOC_8006055", {F(modifierScore)});
            return modifierScore;
    }
}

// ScoreModule_Modifier.cs:67
std::string ScoreModule_Modifier::ScoreDescription() {
    if (modifierType != ScoreModifierType::Multiply && modifierType != ScoreModifierType::Divide) {
        if (modifierType == ScoreModifierType::Substract) {
            const std::string text = Localizer::Format("#autoLOC_8005414") + " " + F(modifierScore) + " " + Localizer::Format("#autoLOC_8100147");
            return Localizer::Format("#autoLOC_8004190", {GetDisplayName(), text});
        }
        return Localizer::Format("#autoLOC_8006050", {GetDisplayName(), F(modifierScore)});
    }
    // modifierType.Description(): the [Description] text of the enum member (EnumExtensions: the tag text is formatted by Localizer later)
    return Localizer::Format("#autoLOC_8100154", {GetDisplayName(), EnumDescription(modifierType), F(modifierScore)});
}

// ScoreModule_Modifier.cs:82
std::vector<std::string> ScoreModule_Modifier::GetDefaultPinnedParameters() { return {"modifierScore"}; }

// ScoreModule_Modifier.cs:87
bool ScoreModule_Modifier::EqualsObject(const DynamicModule& other) const {
    const auto* o = dynamic_cast<const ScoreModule_Modifier*>(&other);
    if (o == nullptr) return false;
    if (name() == o->name() && modifierType == o->modifierType) return FloatEquals(modifierScore, o->modifierScore);
    return false;
}

// ScoreModule_Modifier.cs:101
std::string ScoreModule_Modifier::GetInfo() { return Localizer::Format("#autoLOC_8004158"); }

// ScoreModule_Modifier.cs:106
void ScoreModule_Modifier::Load(const ConfigNode& node) {
    ScoreModule::Load(node);
    save_detail::TryGetEnum(node, "type", modifierType, ScoreModifierType::Multiply);
    node.TryGetValue("modifier", modifierScore);
}

// ScoreModule_Modifier.cs:113
void ScoreModule_Modifier::Save(ConfigNode& node) const {
    ScoreModule::Save(node);
    node.AddValue("type", dotnet::EnumToString(modifierType));
    node.AddValue("modifier", modifierScore);
}

// ---- ScoreModule_Time --------------------------------------------------------------------------------------------------------

// ScoreModule_Time.cs:28
std::string ScoreModule_Time::GetDisplayName() { return Localizer::Format("#autoLOC_8100161"); }

// ScoreModule_Time.cs:33
float ScoreModule_Time::AwardScore(float currentScore) {
    int num = 0;
    while (true) {
        if (num < static_cast<int>(scoreRanges.size())) {
            if (scoreRanges[static_cast<size_t>(num)].isValueInRange(MissionsEnvironment::UniversalTime())) break;
            ++num;
            continue;
        }
        return currentScore;
    }
    awardedScoreDescription = Localizer::Format("#autoLOC_8001003", {F(scoreRanges[static_cast<size_t>(num)].score)});
    currentScore = f32store(fx(currentScore) + fx(scoreRanges[static_cast<size_t>(num)].score));
    return currentScore;
}

// ScoreModule_Time.cs:55
std::string ScoreModule_Time::ScoreDescription() {
    KSPUtil::DefaultDateTimeFormatter dateFormatter;
    std::string text = Localizer::Format("#autoLOC_8100157", {GetDisplayName()});
    for (size_t i = 0; i < scoreRanges.size(); ++i) {
        text += Localizer::Format("#autoLOC_8100158", {dateFormatter.PrintTimeStampCompact(scoreRanges[i].minRange, true, true),
                                                       dateFormatter.PrintTimeStampCompact(scoreRanges[i].maxRange, true, true), F(scoreRanges[i].score)});
    }
    return text;
}

// ScoreModule_Time.cs:67
std::vector<std::string> ScoreModule_Time::GetDefaultPinnedParameters() { return {"scoreRanges"}; }

// ScoreModule_Time.cs:72
bool ScoreModule_Time::EqualsObject(const DynamicModule& other) const {
    const auto* o = dynamic_cast<const ScoreModule_Time*>(&other);
    if (o == nullptr) return false;
    if (name() == o->name()) return RangesEqual(scoreRanges, o->scoreRanges);
    return false;
}

// ScoreModule_Time.cs:86
std::string ScoreModule_Time::GetInfo() { return Localizer::Format("#autoLOC_8004156"); }

// ScoreModule_Time.cs:91
void ScoreModule_Time::Load(const ConfigNode& node) {
    ScoreModule::Load(node);
    LoadRanges(node, scoreRanges);
}

// ScoreModule_Time.cs:107
void ScoreModule_Time::Save(ConfigNode& node) const {
    ScoreModule::Save(node);
    SaveRanges(node, scoreRanges);
}

// ---- ScoreModule_Accuracy ----------------------------------------------------------------------------------------------------

// ScoreModule_Accuracy.cs:24
std::string ScoreModule_Accuracy::GetDisplayName() { return Localizer::Format("#autoLOC_8001002"); }

// ScoreModule_Accuracy.cs:29
float ScoreModule_Accuracy::AwardScore(float currentScore) {
    // (double)base.scoreableObjective.GetScoreModifier(GetType()) * 100.0: NullReferenceException without an objective, InvalidCastException
    // when the boxed value is not a double
    IScoreableObjective* objective = scoreableObjective();
    if (objective == nullptr) throw NullReferenceException();
    const ScoreModifier modifier = objective->GetScoreModifier(TypeName());
    if (!modifier.number.has_value()) throw InvalidCastException("Specified cast is not valid.");
    const double value = *modifier.number * 100.0;
    int num = 0;
    while (true) {
        if (num < static_cast<int>(scoreRanges.size())) {
            if (scoreRanges[static_cast<size_t>(num)].isValueInRange(value)) break;
            ++num;
            continue;
        }
        return currentScore;
    }
    awardedScoreDescription = Localizer::Format("#autoLOC_8006056", {FormatDouble00(value), F(scoreRanges[static_cast<size_t>(num)].score)});
    currentScore = f32store(fx(currentScore) + fx(scoreRanges[static_cast<size_t>(num)].score));
    return currentScore;
}

// ScoreModule_Accuracy.cs:54
std::string ScoreModule_Accuracy::ScoreDescription() {
    std::string text = Localizer::Format("#autoLOC_8100157", {GetDisplayName()});
    for (size_t i = 0; i < scoreRanges.size(); ++i) {
        text += Localizer::Format("#autoLOC_8100156", {D(scoreRanges[i].minRange), D(scoreRanges[i].maxRange), F(scoreRanges[i].score)});
    }
    return text;
}

// ScoreModule_Accuracy.cs:66
std::vector<std::string> ScoreModule_Accuracy::GetDefaultPinnedParameters() { return {"scoreRanges"}; }

// ScoreModule_Accuracy.cs:71
bool ScoreModule_Accuracy::EqualsObject(const DynamicModule& other) const {
    const auto* o = dynamic_cast<const ScoreModule_Accuracy*>(&other);
    if (o == nullptr) return false;
    if (name() == o->name()) return RangesEqual(scoreRanges, o->scoreRanges);
    return false;
}

// ScoreModule_Accuracy.cs:85
std::string ScoreModule_Accuracy::GetInfo() { return Localizer::Format("#autoLOC_8004159"); }

// ScoreModule_Accuracy.cs:90
void ScoreModule_Accuracy::Load(const ConfigNode& node) {
    ScoreModule::Load(node);
    LoadRanges(node, scoreRanges);
}

// ScoreModule_Accuracy.cs:106
void ScoreModule_Accuracy::Save(ConfigNode& node) const {
    ScoreModule::Save(node);
    SaveRanges(node, scoreRanges);
}

// ---- ScoreModule_Resource ----------------------------------------------------------------------------------------------------

// ScoreModule_Resource.cs:27
std::string ScoreModule_Resource::GetDisplayName() { return Localizer::Format("#autoLOC_8100133"); }

// ScoreModule_Resource.cs:32
float ScoreModule_Resource::AwardScore(float currentScore) {
    IScoreableObjective* objective = scoreableObjective();
    if (objective == nullptr) throw NullReferenceException();
    Vessel* vessel = objective->GetScoreModifier(TypeName()).vessel;
    if (vessel == nullptr) vessel = AwardEnvironment::ActiveVessel();
    if (vessel != nullptr) {
        double amount = 0.0;
        if (AwardEnvironment::ConnectedResourceTotal(vessel, resourceName, amount)) {
            for (size_t i = 0; i < scoreRanges.size(); ++i) {
                if (scoreRanges[i].isValueInRange(amount)) {
                    awardedScoreDescription = Localizer::Format("#autoLOC_8006058", {FormatDouble00(amount), F(scoreRanges[i].score)});
                    currentScore = f32store(fx(currentScore) + fx(scoreRanges[i].score));
                    return currentScore;
                }
            }
        }
    }
    return currentScore;
}

// ScoreModule_Resource.cs:64
std::string ScoreModule_Resource::ScoreDescription() {
    std::string text = Localizer::Format("#autoLOC_8100157", {GetDisplayName()});
    for (size_t i = 0; i < scoreRanges.size(); ++i) {
        text += Localizer::Format("#autoLOC_8100160", {D(scoreRanges[i].minRange), D(scoreRanges[i].maxRange), F(scoreRanges[i].score)});
    }
    return text;
}

// ScoreModule_Resource.cs:86
std::vector<std::string> ScoreModule_Resource::GetDefaultPinnedParameters() { return {"resourceName", "scoreRanges"}; }

// ScoreModule_Resource.cs:91
bool ScoreModule_Resource::EqualsObject(const DynamicModule& other) const {
    const auto* o = dynamic_cast<const ScoreModule_Resource*>(&other);
    if (o == nullptr) return false;
    if (name() == o->name() && resourceName == o->resourceName) return RangesEqual(scoreRanges, o->scoreRanges);
    return false;
}

// ScoreModule_Resource.cs:105
std::string ScoreModule_Resource::GetInfo() { return Localizer::Format("#autoLOC_8004157"); }

// ScoreModule_Resource.cs:110
void ScoreModule_Resource::Load(const ConfigNode& node) {
    ScoreModule::Load(node);
    node.TryGetValue("resourceName", resourceName);
    LoadRanges(node, scoreRanges);
}

// ScoreModule_Resource.cs:126
void ScoreModule_Resource::Save(ConfigNode& node) const {
    ScoreModule::Save(node);
    node.AddValue("resourceName", resourceName);
    SaveRanges(node, scoreRanges);
}

// ---- MissionScore ------------------------------------------------------------------------------------------------------------

// MissionScore.cs:10
const std::vector<std::string>& MissionScore::globalScoreModules() {
    // a null list (C#) is reported as the empty list; SetDefaultGlobalScoreModules runs in the first constructor
    return GlobalScoreModules();
}

// MissionScore.cs:27
MissionScore::MissionScore(MENode* n) : DynamicModuleList(n) {
    if (!GlobalScoreModulesSet()) SetDefaultGlobalScoreModules();
    rootNodeName = "SCOREMODULES";
    moduleNodeName = "SCOREMODULE";
}

// MissionScore.cs:69
void MissionScore::AddGlobalScoreModule(const std::string& scoreModule) {
    if (!GlobalScoreModulesSet()) SetDefaultGlobalScoreModules();
    const DynamicModuleType* t = DynamicModuleRegistry::GetClassByName(scoreModule);
    if (t == nullptr || !t->IsSubclassOf("ScoreModule")) {
        log::Warning("[MissionScore]: The supplied type " + scoreModule + " is not a sub class of the ScoreModule class.");
    } else if (std::find(GlobalScoreModules().begin(), GlobalScoreModules().end(), scoreModule) != GlobalScoreModules().end()) {
        log::Warning("[MissionScore]: The supplied score module " + scoreModule + " has been already defined as a global score module.");
    } else {
        GlobalScoreModules().push_back(scoreModule);
    }
}

void MissionScore::ResetGlobalScoreModules() {
    GlobalScoreModules().clear();
    GlobalScoreModulesSet() = false;
}

// MissionScore.cs:12
IScoreableObjective* MissionScore::scoreableObjective() {
    if (node() != nullptr && node()->dockParentNode != nullptr)
        return dynamic_cast<IScoreableObjective*>(node()->dockParentNode->testGroups[0]->testModules[0].get());
    return nullptr;
}

// MissionScore.cs:43
void MissionScore::AwardScore(Mission* mission) {
    for (size_t i = 0; i < activeModules.size(); ++i) {
        if (auto* scoreModule = dynamic_cast<ScoreModule*>(activeModules[i].get())) mission->currentScore = scoreModule->AwardScore(mission->currentScore);
    }
}

// MissionScore.cs:53
std::string MissionScore::ScoreDescription() {
    std::string text;
    for (size_t i = 0; i < activeModules.size(); ++i) {
        if (auto* scoreModule = dynamic_cast<ScoreModule*>(activeModules[i].get())) text = text + Localizer::Format("#autoLOC_8005404") + " " + scoreModule->ScoreDescription();
    }
    return text;
}

// MissionScore.cs:64
std::string MissionScore::AwarededScoreDescription() {
    std::string text;
    for (size_t i = 0; i < activeModules.size(); ++i) {
        if (auto* scoreModule = dynamic_cast<ScoreModule*>(activeModules[i].get())) {
            const std::string text2 = scoreModule->AwarededScoreDescription();
            if (!text2.empty()) text = text + "\n" + text2;
        }
    }
    if (!text.empty()) return text.substr(1);
    return text;
}

// MissionScore.cs:98
std::vector<std::string> MissionScore::GetSupportedTypes() {
    IScoreableObjective* objective = scoreableObjective();
    if (objective != nullptr) {
        supportedTypes.clear();
        supportedTypes.insert(supportedTypes.end(), GlobalScoreModules().begin(), GlobalScoreModules().end());
        if (allowedSystemsOf) {
            const std::vector<std::string> allowed = allowedSystemsOf(objective);
            supportedTypes.insert(supportedTypes.end(), allowed.begin(), allowed.end());
        }
        return supportedTypes;
    }
    return GlobalScoreModules();
}

// DynamicModuleList.cs:140 Clone (MissionScore has the (MENode) constructor)
std::unique_ptr<DynamicModuleList> MissionScore::Clone() const {
    auto list = std::make_unique<MissionScore>(node());
    list->activeModules.clear();
    for (size_t i = 0; i < activeModules.size(); ++i) list->activeModules.push_back(activeModules[i]->Clone());
    list->supportedTypes = supportedTypes;
    return list;
}

// ---- registration ------------------------------------------------------------------------------------------------------------

namespace {
template <class T>
void RegisterScore() {
    DynamicModuleType t;
    t.name = T::kTypeName;
    t.bases = {"ScoreModule", "DynamicModule"};
    t.create = [](MENode* node) -> std::unique_ptr<DynamicModule> { return std::make_unique<T>(node); };
    DynamicModuleRegistry::Register(std::move(t));
}
}  // namespace

void RegisterScoreModules() {
    RegisterScore<ScoreModule_Completion>();
    RegisterScore<ScoreModule_Modifier>();
    RegisterScore<ScoreModule_Time>();
    RegisterScore<ScoreModule_Accuracy>();
    RegisterScore<ScoreModule_Resource>();
}

}  // namespace ksp::Expansions::Missions
