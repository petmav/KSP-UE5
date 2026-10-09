// GameParameters and the stock parameter nodes; see include/ksp/save/game_parameters.hpp.
#include "ksp/save/game_parameters.hpp"

#include <algorithm>
#include <stdexcept>

#include "ksp/config/config_node.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "save_util.hpp"

namespace ksp {

using save_detail::ParseBool;
using MI = ParameterMemberInfo;
using Localizer = KSP::Localization::Localizer;

namespace {

// Localizer.Format(tag), or the en-us text when no Localizer / no such tag is loaded (KSP always has the dictionary)
std::string LocalizedOr(const char* tag, const char* enUs) {
    if (Localizer::Instance() != nullptr) {
        std::string v;
        if (Localizer::TryGetStringByTag(tag, v)) return Localizer::Format(tag);
    }
    return enUs;
}

// One public instance field / auto-property of a parameter class (pointer to the storage).
template <class C>
struct Binding {
    const char* name;
    MI::Kind kind;
    MI::Type type;
    bool C::*b = nullptr;
    int C::*i = nullptr;
    float C::*f = nullptr;
    std::string C::*s = nullptr;
};
template <class C>
Binding<C> B(const char* n, bool C::*p, MI::Kind k = MI::Kind::Field) {
    return {n, k, MI::Type::Bool, p, nullptr, nullptr, nullptr};
}
template <class C>
Binding<C> I(const char* n, int C::*p) {
    return {n, MI::Kind::Field, MI::Type::Int, nullptr, p, nullptr, nullptr};
}
template <class C>
Binding<C> F(const char* n, float C::*p) {
    return {n, MI::Kind::Field, MI::Type::Float, nullptr, nullptr, p, nullptr};
}
template <class C>
Binding<C> S(const char* n, std::string C::*p) {
    return {n, MI::Kind::Field, MI::Type::String, nullptr, nullptr, nullptr, p};
}

// public instance methods every ParameterNode has (object + ParameterNode); GetMember(name) on them returns a
// MethodInfo, which ParameterNode.Load dereferences as a field (NullReferenceException)
const char* const kNodeMethods[] = {"Load", "Save", "OnLoad", "OnSave", "ToString", "Equals", "GetHashCode", "GetType"};
const char* const kCustomMethods[] = {"get_Title",      "get_DisplaySection", "get_Section", "get_SectionOrder",
                                      "get_GameMode",   "get_HasPresets",     "SetDifficultyPreset",
                                      "Interactible",   "Enabled",            "ValidValues"};
const char* const kCustomReadOnlyProperties[] = {"Title", "DisplaySection", "Section", "SectionOrder", "GameMode",
                                                 "HasPresets"};

template <class C>
std::vector<MI> BuildMembers(const std::vector<Binding<C>>& bindings, bool custom,
                             std::initializer_list<const char*> extraMethods = {}) {
    std::vector<MI> m;
    for (const char* n : kNodeMethods) m.push_back({n, MI::Kind::Method, MI::Type::None, true});
    if (custom)
        for (const char* n : kCustomMethods) m.push_back({n, MI::Kind::Method, MI::Type::None, true});
    for (const char* n : extraMethods) m.push_back({n, MI::Kind::Method, MI::Type::None, true});
    if (custom)
        for (const char* n : kCustomReadOnlyProperties) m.push_back({n, MI::Kind::ReadOnlyProperty, MI::Type::None, true});
    for (const auto& b : bindings)
        if (b.kind == MI::Kind::Property) m.push_back({b.name, b.kind, b.type, true});
    for (const auto& b : bindings)
        if (b.kind == MI::Kind::Field) m.push_back({b.name, b.kind, b.type, true});
    return m;
}

template <class C>
const Binding<C>* FindBinding(const std::vector<Binding<C>>& bindings, std::string_view name) {
    for (const auto& b : bindings)
        if (name == b.name) return &b;
    return nullptr;
}

// obj.ToString() of a boxed value (GameParameters.cs:549)
template <class C>
std::optional<std::string> BindingText(const C& c, const std::vector<Binding<C>>& bindings, std::string_view name) {
    const Binding<C>* b = FindBinding(bindings, name);
    if (!b) return std::nullopt;
    switch (b->type) {
    case MI::Type::Bool: return dotnet::FormatBool(c.*(b->b));
    case MI::Type::Int: return dotnet::FormatInt64(c.*(b->i));
    case MI::Type::Float: return dotnet::FormatSingleG(c.*(b->f));
    case MI::Type::String: return c.*(b->s);
    default: return std::nullopt;
    }
}

// GameParameters.cs:470-512: string as is, int.Parse, float.Parse, bool.Parse
template <class C>
void BindingSet(C& c, const std::vector<Binding<C>>& bindings, std::string_view name, const std::string& text) {
    const Binding<C>* b = FindBinding(bindings, name);
    if (!b) return;
    switch (b->type) {
    case MI::Type::Bool: c.*(b->b) = ParseBool(text); break;
    case MI::Type::Int: c.*(b->i) = dotnet::ParseInt32(text); break;
    case MI::Type::Float: c.*(b->f) = dotnet::ParseSingle(text); break;
    case MI::Type::String: c.*(b->s) = text; break;
    default: break;
    }
}

}  // namespace

// ================================================ ParameterNode =====================================================

// GameParameters.cs:438
void GameParameters::ParameterNode::Load(const ConfigNode* node) {
    if (node == nullptr) {
        log::Warning("GameParameters: Cannot find settings");
        return;
    }
    // GameParameters.cs:445: child nodes whose name is an IConfigNode field (no stock class has one)
    const std::vector<ParameterMemberInfo>& members = Members();
    for (const ConfigNode::Value& value : node->values()) {
        const ParameterMemberInfo* member = nullptr;
        for (const auto& m : members)
            if (value.name == m.name) {
                member = &m;
                break;
            }
        if (member == nullptr) continue;
        if (member->kind == MI::Kind::Method)
            throw NullReferenceException("GameParameters.ParameterNode.Load: member '" + value.name + "' is a method");
        if (member->kind == MI::Kind::ReadOnlyProperty) continue;
        SetMemberValue(value.name, value.value);
    }
    OnLoad(*node);
}

// GameParameters.cs:533
void GameParameters::ParameterNode::Save(ConfigNode& node) const {
    for (const auto& m : Members()) {
        if (m.kind == MI::Kind::Method || m.kind == MI::Kind::ReadOnlyProperty) continue;
        if (!m.autoPersistance) continue;
        const std::optional<std::string> text = GetMemberText(m.name);
        if (text) node.AddValue(m.name, *text);
    }
    OnSave(node);
}

// GameParameters.cs:411
void GameParameters::CustomParameterNode::SetDifficultyPreset(Preset) {
    throw std::logic_error("NotImplementedException: CustomParameterNode.SetDifficultyPreset");
}
bool GameParameters::CustomParameterNode::Interactible(std::string_view, const GameParameters&) const { return true; }
bool GameParameters::CustomParameterNode::Enabled(std::string_view, const GameParameters&) const { return true; }
bool GameParameters::CustomParameterNode::HasValidValues(std::string_view) const { return false; }

// ================================================ stock nodes ======================================================

#define KSP_PARAM_BINDINGS(C, ...)                                                                          \
    namespace {                                                                                             \
    const std::vector<Binding<C>>& Bindings_##C() {                                                         \
        static const std::vector<Binding<C>> b = {__VA_ARGS__};                                             \
        return b;                                                                                           \
    }                                                                                                       \
    }

using FP = GameParameters::FlightParams;
using EP = GameParameters::EditorParams;
using TP = GameParameters::TrackingStationParams;
using SP = GameParameters::SpaceCenterParams;
using DP = GameParameters::DifficultyParams;
using CP = GameParameters::CareerParams;
using AP = GameParameters::AdvancedParams;
using CNP = CommNet::CommNetParams;
using MPG = Expansions::Missions::MissionParamsGeneral;
using MPE = Expansions::Missions::MissionParamsExtras;
using MPF = Expansions::Missions::MissionParamsFacilities;

// GameParameters.cs:110
KSP_PARAM_BINDINGS(FP, B("CanQuickSave", &FP::CanQuickSave), B("CanQuickLoad", &FP::CanQuickLoad),
                   B("CanAutoSave", &FP::CanAutoSave), B("CanUseMap", &FP::CanUseMap),
                   B("CanSwitchVesselsNear", &FP::CanSwitchVesselsNear), B("CanSwitchVesselsFar", &FP::CanSwitchVesselsFar),
                   B("CanTimeWarpHigh", &FP::CanTimeWarpHigh), B("CanTimeWarpLow", &FP::CanTimeWarpLow),
                   B("CanEVA", &FP::CanEVA), B("CanIVA", &FP::CanIVA), B("CanBoard", &FP::CanBoard),
                   B("CanRestart", &FP::CanRestart), B("CanLeaveToEditor", &FP::CanLeaveToEditor),
                   B("CanLeaveToTrackingStation", &FP::CanLeaveToTrackingStation),
                   B("CanLeaveToSpaceCenter", &FP::CanLeaveToSpaceCenter),
                   B("CanLeaveToMainMenu", &FP::CanLeaveToMainMenu))
// GameParameters.cs:147
KSP_PARAM_BINDINGS(EP, B("CanSave", &EP::CanSave), B("CanLoad", &EP::CanLoad), B("CanStartNew", &EP::CanStartNew),
                   B("CanLaunch", &EP::CanLaunch), B("CanLeaveToSpaceCenter", &EP::CanLeaveToSpaceCenter),
                   B("CanLeaveToMainMenu", &EP::CanLeaveToMainMenu), I("startUpMode", &EP::startUpMode),
                   S("craftFileToLoad", &EP::craftFileToLoad))
// GameParameters.cs:166
KSP_PARAM_BINDINGS(TP, B("CanFlyVessel", &TP::CanFlyVessel), B("CanAbortVessel", &TP::CanAbortVessel),
                   B("CanLeaveToSpaceCenter", &TP::CanLeaveToSpaceCenter),
                   B("CanLeaveToMainMenu", &TP::CanLeaveToMainMenu))
// GameParameters.cs:177
KSP_PARAM_BINDINGS(SP, B("CanGoInVAB", &SP::CanGoInVAB), B("CanGoInSPH", &SP::CanGoInSPH),
                   B("CanGoInTrackingStation", &SP::CanGoInTrackingStation), B("CanLaunchAtPad", &SP::CanLaunchAtPad),
                   B("CanLaunchAtRunway", &SP::CanLaunchAtRunway), B("CanGoToAdmin", &SP::CanGoToAdmin),
                   B("CanGoToAstronautC", &SP::CanGoToAstronautC), B("CanGoToMissionControl", &SP::CanGoToMissionControl),
                   B("CanGoToRnD", &SP::CanGoToRnD), B("CanSelectFlag", &SP::CanSelectFlag),
                   B("CanLeaveToMainMenu", &SP::CanLeaveToMainMenu))
// GameParameters.cs:202
KSP_PARAM_BINDINGS(DP, B("AutoHireCrews", &DP::AutoHireCrews), B("MissingCrewsRespawn", &DP::MissingCrewsRespawn),
                   F("RespawnTimer", &DP::RespawnTimer),
                   B("BypassEntryPurchaseAfterResearch", &DP::BypassEntryPurchaseAfterResearch),
                   B("AllowStockVessels", &DP::AllowStockVessels),
                   B("IndestructibleFacilities", &DP::IndestructibleFacilities),
                   F("ResourceAbundance", &DP::ResourceAbundance), F("ReentryHeatScale", &DP::ReentryHeatScale),
                   B("EnableCommNet", &DP::EnableCommNet), B("AllowOtherLaunchSites", &DP::AllowOtherLaunchSites),
                   B("persistKerbalInventories", &DP::persistKerbalInventories))
// GameParameters.cs:227
KSP_PARAM_BINDINGS(CP, S("TechTreeUrl", &CP::TechTreeUrl), F("StartingFunds", &CP::StartingFunds),
                   F("StartingScience", &CP::StartingScience), F("StartingReputation", &CP::StartingReputation),
                   F("FundsGainMultiplier", &CP::FundsGainMultiplier), F("RepGainMultiplier", &CP::RepGainMultiplier),
                   F("ScienceGainMultiplier", &CP::ScienceGainMultiplier),
                   F("FundsLossMultiplier", &CP::FundsLossMultiplier), F("RepLossMultiplier", &CP::RepLossMultiplier),
                   F("RepLossDeclined", &CP::RepLossDeclined))
// GameParameters.cs:250 (EnableKerbalExperience is handled by the class itself: a custom property)
KSP_PARAM_BINDINGS(AP, B("ImmediateLevelUp", &AP::ImmediateLevelUp, MI::Kind::Property),
                   B("AllowNegativeCurrency", &AP::AllowNegativeCurrency),
                   B("PressurePartLimits", &AP::PressurePartLimits), B("GPartLimits", &AP::GPartLimits),
                   B("GKerbalLimits", &AP::GKerbalLimits), F("KerbalGToleranceMult", &AP::KerbalGToleranceMult),
                   B("ResourceTransferObeyCrossfeed", &AP::ResourceTransferObeyCrossfeed),
                   B("ActionGroupsAlways", &AP::ActionGroupsAlways),
                   F("BuildingImpactDamageMult", &AP::BuildingImpactDamageMult),
                   B("PartUpgradesInSandbox", &AP::PartUpgradesInSandbox),
                   B("PartUpgradesInCareer", &AP::PartUpgradesInCareer),
                   B("PartUpgradesInMission", &AP::PartUpgradesInMission),
                   B("EnableFullSASInSandbox", &AP::EnableFullSASInSandbox),
                   B("EnableFullSASInMissions", &AP::EnableFullSASInMissions))
// CommNet/CommNetParams.cs:8
KSP_PARAM_BINDINGS(CNP, B("requireSignalForControl", &CNP::requireSignalForControl),
                   B("plasmaBlackout", &CNP::plasmaBlackout), F("rangeModifier", &CNP::rangeModifier),
                   F("DSNModifier", &CNP::DSNModifier), F("occlusionMultiplierVac", &CNP::occlusionMultiplierVac),
                   F("occlusionMultiplierAtm", &CNP::occlusionMultiplierAtm),
                   B("enableGroundStations", &CNP::enableGroundStations))
// Expansions/Missions/MissionParamsGeneral.cs:10
KSP_PARAM_BINDINGS(MPG, B("enableFunding", &MPG::enableFunding), F("startingFunds", &MPG::startingFunds),
                   S("spacer", &MPG::spacer), B("enableKerbalLevels", &MPG::enableKerbalLevels),
                   I("kerbalLevelPilot", &MPG::kerbalLevelPilot), I("kerbalLevelScientist", &MPG::kerbalLevelScientist),
                   I("kerbalLevelEngineer", &MPG::kerbalLevelEngineer), S("spacer2", &MPG::spacer2),
                   B("preventVesselRecovery", &MPG::preventVesselRecovery))
// Expansions/Missions/MissionParamsExtras.cs:10
KSP_PARAM_BINDINGS(MPE, B("facilityOpenAC", &MPE::facilityOpenAC), B("astronautHiresAreFree", &MPE::astronautHiresAreFree),
                   S("spacer", &MPE::spacer), B("facilityOpenEditor", &MPE::facilityOpenEditor),
                   B("launchSitesOpen", &MPE::launchSitesOpen), S("spacer2", &MPE::spacer2),
                   B("cheatsEnabled", &MPE::cheatsEnabled))
// Expansions/Missions/MissionParamsFacilities.cs:9
KSP_PARAM_BINDINGS(MPF, I("facilityLevelAdmin", &MPF::facilityLevelAdmin), I("facilityLevelAC", &MPF::facilityLevelAC),
                   I("facilityLevelLaunchpad", &MPF::facilityLevelLaunchpad), I("facilityLevelMC", &MPF::facilityLevelMC),
                   I("facilityLevelRD", &MPF::facilityLevelRD), I("facilityLevelRunway", &MPF::facilityLevelRunway),
                   I("facilityLevelSPH", &MPF::facilityLevelSPH), I("facilityLevelTS", &MPF::facilityLevelTS),
                   I("facilityLevelVAB", &MPF::facilityLevelVAB))

#undef KSP_PARAM_BINDINGS

#define KSP_PARAM_PLAIN_IMPL(C, CUSTOM)                                                                         \
    const std::vector<ParameterMemberInfo>& C::Members() const {                                                \
        static const std::vector<ParameterMemberInfo> m = BuildMembers(Bindings_##C(), CUSTOM);                 \
        return m;                                                                                               \
    }                                                                                                           \
    std::optional<std::string> C::GetMemberText(std::string_view name) const {                                  \
        return BindingText(*this, Bindings_##C(), name);                                                        \
    }                                                                                                           \
    void C::SetMemberValue(std::string_view name, const std::string& text) {                                    \
        BindingSet(*this, Bindings_##C(), name, text);                                                          \
    }

KSP_PARAM_PLAIN_IMPL(FP, false)
KSP_PARAM_PLAIN_IMPL(EP, false)
KSP_PARAM_PLAIN_IMPL(TP, false)
KSP_PARAM_PLAIN_IMPL(SP, false)
KSP_PARAM_PLAIN_IMPL(DP, false)
KSP_PARAM_PLAIN_IMPL(CP, false)
KSP_PARAM_PLAIN_IMPL(CNP, true)
KSP_PARAM_PLAIN_IMPL(MPG, true)
KSP_PARAM_PLAIN_IMPL(MPE, true)
KSP_PARAM_PLAIN_IMPL(MPF, true)

#undef KSP_PARAM_PLAIN_IMPL

// GameParameters.cs:206: RespawnTimer = (float)GameSettings.DEFAULT_KERBAL_RESPAWN_TIMER
GameParameters::DifficultyParams::DifficultyParams()
    : RespawnTimer(static_cast<float>(SaveEnvironment::Current().DEFAULT_KERBAL_RESPAWN_TIMER)) {}

// ---- AdvancedParams (GameParameters.cs:250) ----

const std::vector<ParameterMemberInfo>& AP::Members() const {
    static const std::vector<ParameterMemberInfo> m = [] {
        std::vector<ParameterMemberInfo> v =
            BuildMembers(Bindings_AP(), true,
                         {"get_EnableKerbalExperience", "set_EnableKerbalExperience", "get_ImmediateLevelUp",
                          "set_ImmediateLevelUp", "KerbalExperienceEnabled"});
        // properties in declaration order: EnableKerbalExperience (custom accessors) before ImmediateLevelUp
        auto it = std::find_if(v.begin(), v.end(), [](const ParameterMemberInfo& x) {
            return std::string_view(x.name) == "ImmediateLevelUp";
        });
        v.insert(it, {"EnableKerbalExperience", MI::Kind::Property, MI::Type::Bool, true});
        return v;
    }();
    return m;
}

std::optional<std::string> AP::GetMemberText(std::string_view name) const {
    if (name == "EnableKerbalExperience") return dotnet::FormatBool(GetEnableKerbalExperience());
    return BindingText(*this, Bindings_AP(), name);
}

void AP::SetMemberValue(std::string_view name, const std::string& text) {
    if (name == "EnableKerbalExperience") {
        SetEnableKerbalExperience(ParseBool(text));
        return;
    }
    BindingSet(*this, Bindings_AP(), name, text);
}

// GameParameters.cs:317
bool AP::GetEnableKerbalExperience() const {
    if (!enableKerbalExperience.has_value()) {
        const Game* g = SaveEnvironment::Current().currentGame;
        if (g == nullptr) return true;
        if (g->Mode != Game_Modes::CAREER) return g->Mode == Game_Modes::MISSION;
        return true;
    }
    return *enableKerbalExperience;
}

// GameParameters.cs:342
bool AP::KerbalExperienceEnabled(Game_Modes gameMode) const {
    if (!enableKerbalExperience.has_value()) {
        if (gameMode != Game_Modes::CAREER) return gameMode == Game_Modes::MISSION;
        return true;
    }
    return *enableKerbalExperience;
}

std::string AP::Title() const { return LocalizedOr("#autoLOC_140938", "Advanced Options"); }

// GameParameters.cs:356
void AP::SetDifficultyPreset(Preset preset) {
    switch (preset) {
    case Preset::Easy:
        BuildingImpactDamageMult = 0.03f;
        ImmediateLevelUp = true;
        break;
    case Preset::Moderate:
        BuildingImpactDamageMult = 0.1f;
        AllowNegativeCurrency = true;
        ResourceTransferObeyCrossfeed = true;
        break;
    case Preset::Hard:
        BuildingImpactDamageMult = 0.2f;
        AllowNegativeCurrency = true;
        ResourceTransferObeyCrossfeed = true;
        break;
    default: break;
    }
}

// GameParameters.cs:378
bool AP::Interactible(std::string_view member, const GameParameters& parameters) const {
    if (member == "ImmediateLevelUp") return parameters.CustomParams<AdvancedParams>().GetEnableKerbalExperience();
    if (member == "BuildingImpactDamageMult") return !parameters.Difficulty.IndestructibleFacilities;
    if (member == "KerbalGToleranceMult") return parameters.CustomParams<AdvancedParams>().GKerbalLimits;
    return CustomParameterNode::Interactible(member, parameters);
}

// GameParameters.cs:394
void AP::OnLoad(const ConfigNode& node) {
    const Game* g = SaveEnvironment::Current().currentGame;
    if (!node.HasValue("EnableKerbalExperience") && g != nullptr) SetEnableKerbalExperience(g->Mode == Game_Modes::CAREER);
}

// ---- CommNetParams (CommNet/CommNetParams.cs) ----

std::string CNP::Title() const { return LocalizedOr("#autoLOC_117012", "CommNet Options"); }
std::string CNP::DisplaySection() const { return LocalizedOr("#autoLoc_6002170", "#autoLoc_6002170"); }

// CommNetParams.cs:43
bool CNP::Interactible(std::string_view, const GameParameters& parameters) const {
    return parameters.Difficulty.EnableCommNet;
}

// CommNetParams.cs:48
void CNP::SetDifficultyPreset(GameParameters::Preset preset) {
    switch (preset) {
    case GameParameters::Preset::Easy:
        requireSignalForControl = false;
        rangeModifier = 1.5f;
        occlusionMultiplierVac = 0.0f;
        occlusionMultiplierAtm = 0.0f;
        plasmaBlackout = false;
        break;
    case GameParameters::Preset::Moderate:
        rangeModifier = 0.8f;
        occlusionMultiplierVac = 1.0f;
        occlusionMultiplierAtm = 0.85f;
        plasmaBlackout = false;
        break;
    case GameParameters::Preset::Hard:
        rangeModifier = 0.65f;
        occlusionMultiplierVac = 1.0f;
        occlusionMultiplierAtm = 1.0f;
        plasmaBlackout = false;
        break;
    default: break;
    }
}

// ---- MissionParamsGeneral (Expansions/Missions/MissionParamsGeneral.cs) ----

// MissionParamsGeneral.cs:47
bool MPG::Interactible(std::string_view member, const GameParameters& parameters) const {
    if (member != "kerbalLevelPilot" && member != "kerbalLevelScientist" && member != "kerbalLevelEngineer") {
        if (member == "startingFunds") return enableFunding;
        return CustomParameterNode::Interactible(member, parameters);
    }
    return enableKerbalLevels;
}

// MissionParamsGeneral.cs:59
void MPG::SetDifficultyPreset(GameParameters::Preset preset) {
    using P = GameParameters::Preset;
    switch (preset) {
    case P::Easy:
        enableFunding = false; startingFunds = 250000.0f; enableKerbalLevels = false;
        kerbalLevelPilot = 5; kerbalLevelScientist = 5; kerbalLevelEngineer = 5; preventVesselRecovery = false;
        break;
    case P::Normal:
        enableFunding = false; startingFunds = 100000.0f; enableKerbalLevels = false;
        kerbalLevelPilot = 4; kerbalLevelScientist = 4; kerbalLevelEngineer = 4; preventVesselRecovery = false;
        break;
    case P::Moderate:
        enableFunding = true; startingFunds = 25000.0f; enableKerbalLevels = true;
        kerbalLevelPilot = 2; kerbalLevelScientist = 2; kerbalLevelEngineer = 2; preventVesselRecovery = false;
        break;
    case P::Hard:
        enableFunding = true; startingFunds = 10000.0f; enableKerbalLevels = true;
        kerbalLevelPilot = 1; kerbalLevelScientist = 1; kerbalLevelEngineer = 1; preventVesselRecovery = true;
        break;
    default: break;
    }
}

// ---- MissionParamsExtras (Expansions/Missions/MissionParamsExtras.cs) ----

// MissionParamsExtras.cs:37
bool MPE::Interactible(std::string_view member, const GameParameters& parameters) const {
    if (member == "astronautHiresAreFree") return facilityOpenAC;
    if (member == "launchSitesOpen") return facilityOpenEditor;
    return CustomParameterNode::Interactible(member, parameters);
}

// MissionParamsExtras.cs:49
void MPE::SetDifficultyPreset(GameParameters::Preset preset) {
    using P = GameParameters::Preset;
    switch (preset) {
    case P::Easy:
        facilityOpenAC = false; astronautHiresAreFree = true; facilityOpenEditor = false; launchSitesOpen = true;
        cheatsEnabled = true;
        break;
    case P::Normal:
        facilityOpenAC = false; astronautHiresAreFree = true; facilityOpenEditor = false; launchSitesOpen = true;
        cheatsEnabled = false;
        break;
    case P::Moderate:
        facilityOpenAC = true; astronautHiresAreFree = true; facilityOpenEditor = false; launchSitesOpen = true;
        cheatsEnabled = false;
        break;
    case P::Hard:
        facilityOpenAC = true; astronautHiresAreFree = false; facilityOpenEditor = false; launchSitesOpen = true;
        cheatsEnabled = false;
        break;
    default: break;
    }
}

// ---- MissionParamsFacilities (Expansions/Missions/MissionParamsFacilities.cs:41) ----
void MPF::SetDifficultyPreset(GameParameters::Preset preset) {
    using P = GameParameters::Preset;
    int level = -1;
    switch (preset) {
    case P::Easy: level = 3; break;
    case P::Normal: level = 3; break;
    case P::Moderate: level = 2; break;
    case P::Hard: level = 1; break;
    default: return;
    }
    facilityLevelAdmin = level; facilityLevelAC = level; facilityLevelLaunchpad = level; facilityLevelMC = level;
    facilityLevelRD = level; facilityLevelRunway = level; facilityLevelSPH = level; facilityLevelTS = level;
    facilityLevelVAB = level;
}

// ================================================ GameParameters ===================================================

namespace {
// AssemblyLoader.loadedAssemblies.TypeOperation discovery order (Mono Assembly.GetTypes = TypeDef row order of the
// shipped Assembly-CSharp.dll; see the header comment)
const std::vector<GameParameters::ParameterType>& StockParameterTypes() {
    static const std::vector<GameParameters::ParameterType> t = {
        {MPE::kTypeName, [] { return std::unique_ptr<GameParameters::CustomParameterNode>(new MPE()); }},
        {MPF::kTypeName, [] { return std::unique_ptr<GameParameters::CustomParameterNode>(new MPF()); }},
        {MPG::kTypeName, [] { return std::unique_ptr<GameParameters::CustomParameterNode>(new MPG()); }},
        {CNP::kTypeName, [] { return std::unique_ptr<GameParameters::CustomParameterNode>(new CNP()); }},
        {AP::kTypeName, [] { return std::unique_ptr<GameParameters::CustomParameterNode>(new AP()); }},
    };
    return t;
}
}  // namespace

// GameParameters.cs:616 static ParameterTypes (GenerateParameterTypes at static construction)
std::vector<const GameParameters::ParameterType*>& GameParameters::ParameterTypes() {
    static std::vector<const ParameterType*> list = [] {
        std::vector<const ParameterType*> v;
        for (const auto& t : StockParameterTypes()) {
            v.push_back(&t);
            log::Info("[GameParameters]: Loaded custom parameter class " + t.name + ".");
        }
        return v;
    }();
    return list;
}

void GameParameters::ResetParameterTypes() {
    auto& list = ParameterTypes();
    list.clear();
    for (const auto& t : StockParameterTypes()) list.push_back(&t);
}

// GameParameters.cs:625
GameParameters::GameParameters() {
    preset = Preset::Normal;
    auto& types = ParameterTypes();
    int count = static_cast<int>(types.size());
    while (count-- > 0) {
        const ParameterType* type = types[static_cast<size_t>(count)];
        customParams_.emplace_back(type->name, type->create());
    }
    auto titleOf = [this](const ParameterType* t) {
        for (const auto& [name, node] : customParams_)
            if (name == t->name) return node->Title();
        throw KeyNotFoundException("The given key was not present in the dictionary.");
    };
    dotnet::ListSort(types, [&](const ParameterType* a, const ParameterType* b) {
        return dotnet::CompareCulture(titleOf(a), titleOf(b));
    });
}

// GameParameters.cs:641
GameParameters::GameParameters(const ConfigNode& node) : GameParameters() { Load(node); }

void GameParameters::throw_missing(const char* name) {
    throw ArgumentException(std::string("Couldn't find custom parameter ") + name + "!");
}

// GameParameters.cs:723
GameParameters::CustomParameterNode* GameParameters::CustomParams(std::string_view typeName) const {
    for (const auto& [name, node] : customParams_)
        if (name == typeName) return node.get();
    log::Warning("Couldn't find custom parameter " + std::string(typeName.empty() ? "null" : typeName) + "!");
    return nullptr;
}

std::vector<std::pair<std::string, GameParameters::CustomParameterNode*>> GameParameters::CustomParamsInOrder() const {
    std::vector<std::pair<std::string, CustomParameterNode*>> v;
    for (const auto& [name, node] : customParams_) v.emplace_back(name, node.get());
    return v;
}

// GameParameters.cs:661
void GameParameters::Load(const ConfigNode& node) {
    if (node.HasValue("preset")) preset = save_detail::ParseEnum<Preset>(*node.GetValuePtr("preset"));
    for (const auto& child : node.nodes()) {
        const std::string& n = child->name;
        if (n == "TRACKINGSTATION") TrackingStation.Load(child.get());
        else if (n == "SPACECENTER") SpaceCenter.Load(child.get());
        else if (n == "EDITOR") Editor.Load(child.get());
        else if (n == "FLIGHT") Flight.Load(child.get());
        else if (n == "CAREER") Career.Load(child.get());
        else if (n == "DIFFICULTY") Difficulty.Load(child.get());
        else if (n == "RESEARCHANDDEVELOPMENT") {
        } else LoadCustomNode(*child);
    }
}

// GameParameters.cs:692
void GameParameters::Save(ConfigNode& node) const {
    node.AddValue("preset", save_detail::EnumName(preset));
    Flight.Save(*node.AddNode("FLIGHT"));
    Editor.Save(*node.AddNode("EDITOR"));
    TrackingStation.Save(*node.AddNode("TRACKINGSTATION"));
    SpaceCenter.Save(*node.AddNode("SPACECENTER"));
    Difficulty.Save(*node.AddNode("DIFFICULTY"));
    Career.Save(*node.AddNode("CAREER"));
    for (const auto& [name, param] : customParams_) {
        auto node2 = std::make_shared<ConfigNode>(name);
        node.AddNode(node2);
        param->Save(*node2);
    }
}

// GameParameters.cs:735
void GameParameters::LoadCustomNode(const ConfigNode& node) {
    const ParameterType* type = nullptr;
    auto& types = ParameterTypes();
    int count = static_cast<int>(types.size());
    while (count-- > 0)
        if (types[static_cast<size_t>(count)]->name == node.name) {
            type = types[static_cast<size_t>(count)];
            break;
        }
    CustomParameterNode* value = nullptr;
    if (type != nullptr)
        for (const auto& [name, p] : customParams_)
            if (name == type->name) value = p.get();
    if (value != nullptr) value->Load(&node);
    else log::Warning("[GameParameters]: Couldn't find type for custom parameter " + node.name + ".");
}

std::map<GameParameters::Preset, std::shared_ptr<GameParameters>>& GameParameters::DifficultyPresets() {
    static std::map<Preset, std::shared_ptr<GameParameters>> presets;
    return presets;
}

// GameParameters.cs:759
void GameParameters::SetDifficultyPresets() {
    auto& presets = DifficultyPresets();
    if (presets.size() == 5) return;
    presets.clear();
    auto g = std::make_shared<GameParameters>();
    g->Difficulty.AllowStockVessels = true;
    g->Difficulty.AllowOtherLaunchSites = true;
    g->Difficulty.IndestructibleFacilities = true;
    g->Difficulty.ResourceAbundance = 1.2f;
    g->Difficulty.ReentryHeatScale = 0.5f;
    g->Difficulty.EnableCommNet = false;
    g->Difficulty.persistKerbalInventories = false;
    g->Career.StartingFunds = 250000.0f;
    g->Career.FundsGainMultiplier = 2.0f;
    g->Career.RepGainMultiplier = 2.0f;
    g->Career.ScienceGainMultiplier = 2.0f;
    g->Career.FundsLossMultiplier = 0.5f;
    g->Career.RepLossMultiplier = 0.5f;
    g->Career.RepLossDeclined = 0.0f;
    presets[Preset::Easy] = g;
    g = std::make_shared<GameParameters>();
    g->Difficulty.AllowOtherLaunchSites = true;
    presets[Preset::Normal] = g;
    g->Difficulty.EnableCommNet = true;  // after Add (same object)
    g = std::make_shared<GameParameters>();
    g->Difficulty.MissingCrewsRespawn = false;
    g->Difficulty.BypassEntryPurchaseAfterResearch = false;
    g->Difficulty.ResourceAbundance = 0.8f;
    g->Difficulty.ReentryHeatScale = 1.0f;
    g->Difficulty.AllowOtherLaunchSites = true;
    g->Difficulty.persistKerbalInventories = false;
    g->Career.StartingFunds = 15000.0f;
    g->Career.FundsGainMultiplier = 0.9f;
    g->Career.RepGainMultiplier = 0.9f;
    g->Career.ScienceGainMultiplier = 0.9f;
    g->Career.FundsLossMultiplier = 1.5f;
    g->Career.RepLossMultiplier = 1.5f;
    g->Career.RepLossDeclined = 2.0f;
    g->Difficulty.EnableCommNet = true;
    presets[Preset::Moderate] = g;
    g = std::make_shared<GameParameters>();
    g->Difficulty.AllowStockVessels = false;
    g->Difficulty.AllowOtherLaunchSites = true;
    g->Difficulty.MissingCrewsRespawn = false;
    g->Difficulty.BypassEntryPurchaseAfterResearch = false;
    g->Difficulty.ResourceAbundance = 0.5f;
    g->Difficulty.ReentryHeatScale = 1.0f;
    g->Difficulty.EnableCommNet = true;
    g->Difficulty.persistKerbalInventories = false;
    g->Flight.CanRestart = false;
    g->Flight.CanLeaveToEditor = false;
    g->Flight.CanQuickLoad = false;
    g->Career.StartingFunds = 10000.0f;
    g->Career.FundsGainMultiplier = 0.6f;
    g->Career.RepGainMultiplier = 0.6f;
    g->Career.ScienceGainMultiplier = 0.6f;
    g->Career.FundsLossMultiplier = 2.0f;
    g->Career.RepLossMultiplier = 2.0f;
    g->Career.RepLossDeclined = 3.0f;
    presets[Preset::Hard] = g;
    presets[Preset::Custom] = std::make_shared<GameParameters>();
}

// GameParameters.cs:840
std::shared_ptr<GameParameters> GameParameters::GetDefaultParameters(Game_Modes mode, Preset p) {
    SetDifficultyPresets();
    std::shared_ptr<GameParameters> value;
    auto& presets = DifficultyPresets();
    auto it = presets.find(p);
    if (it == presets.end()) {
        value = std::make_shared<GameParameters>();
    } else {
        ConfigNode node("PARAMS");
        it->second->Save(node);
        value = std::make_shared<GameParameters>(node);
    }
    switch (mode) {
    case Game_Modes::SANDBOX:
        value->Difficulty.BypassEntryPurchaseAfterResearch = true;
        value->Difficulty.AllowStockVessels = p != Preset::Hard;
        value->CustomParams<AdvancedParams>().SetEnableKerbalExperience(false);
        break;
    default:
        value->CustomParams<AdvancedParams>().SetEnableKerbalExperience(true);
        value->Difficulty.AllowOtherLaunchSites = false;
        value->Difficulty.persistKerbalInventories = false;
        break;
    case Game_Modes::SCIENCE_SANDBOX:
        value->Difficulty.BypassEntryPurchaseAfterResearch = true;
        value->CustomParams<AdvancedParams>().SetEnableKerbalExperience(false);
        break;
    case Game_Modes::MISSION:
        value->SpaceCenter.CanGoToRnD = false;
        value->SpaceCenter.CanGoToAdmin = false;
        value->SpaceCenter.CanGoToMissionControl = false;
        value->Difficulty.AllowOtherLaunchSites = false;
        value->Difficulty.BypassEntryPurchaseAfterResearch = true;
        value->CustomParams<AdvancedParams>().SetEnableKerbalExperience(true);
        value->Difficulty.persistKerbalInventories = false;
        break;
    }
    for (auto& [name, node] : value->customParams_) {
        if (!node->HasPresets()) continue;
        try {
            node->SetDifficultyPreset(p);
        } catch (const std::exception& e) {
            log::Error("Error calling custom SetDifficultyPreset method in type " + name + ":");
            log::Error(e.what());
        }
    }
    value->preset = p;
    return value;
}

}  // namespace ksp
