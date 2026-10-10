// WP-36: Upgradeables/UpgradeableObject.cs, UpgradeableFacility.cs, KSCFacilityLevelText.cs and ScenarioUpgradeableFacilities.cs.
#include "ksp/career/upgradeables.hpp"

#include <algorithm>
#include <cmath>

#include "ksp/career/format.hpp"
#include "ksp/career/game_variables.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/scenario_runner.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/util/enum_extensions.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace Upgradeables {

namespace {
constexpr float kIntMaxAsFloat = 2.1474836E+09f;  // the literal KSCFacilityLevelText compares against
constexpr float kFloatMax = 3.4028235E+38f;
Game* CurrentGame() { return SaveEnvironment::Current().currentGame; }
}  // namespace

// ---- KSCFacilityLevelText -------------------------------------------------------------------------------------------------

// KSCFacilityLevelText.cs:18
std::string KSCFacilityLevelText::GetText(float normLevel) {
    std::string text = textBase.empty() ? "" : textBase;
    while (text.find('[') != std::string::npos) {
        std::string next = ProcessTextTags(text, normLevel);
        if (next == text) break;  // sic: the C# loops forever on a text ProcessTextTags cannot reduce (syntax error); stop instead
        text = std::move(next);
    }
    while (text.find("\n\n") != std::string::npos) text = dotnet::Replace(text, "\n\n", "\n");
    if (text != "" && !linePrefix.empty()) text = linePrefix + dotnet::Replace(dotnet::Trim(text), "\n", "\n" + linePrefix);
    return text;
}

// KSCFacilityLevelText.cs:36
std::string KSCFacilityLevelText::ProcessTextTags(std::string src, float level) {
    if (src.find('[') != std::string::npos) {
        size_t num = src.rfind('[');
        while (num != std::string::npos) {
            const size_t num2 = src.find(']', num);
            if (num2 != std::string::npos) {
                const size_t startIndex = num + 1;
                const size_t length = num2 - num - 1;
                const std::string text = src.substr(startIndex, length);
                if (text.find('[') == std::string::npos && text.find(']') == std::string::npos) {
                    src.erase(num, num2 - num + 1);
                    const std::string value = GetValue(text, level);
                    src.insert(num, value);
                    num = src.rfind('[');
                    continue;
                }
                log::Error("KSCUpgradeableLevelText Syntax Error: Mismatched brackets on replacement block: " + src +
                           ". nesting is not supported, ensure all opening brackets are closed.");
                return src;
            }
            log::Error("KSCUpgradeableLevelText Syntax Error: Missing closing bracket on replacement block: " + src);
            return src;
        }
    }
    return src;
}

// KSCFacilityLevelText.cs:290
std::string KSCFacilityLevelText::GetValueOrFallback(float value, float threshold, const std::string& format, const std::string& fallback,
                                                     const std::function<std::string(float)>& postProcess) {
    if (value < threshold) {
        if (!postProcess) return career::FormatNumber(value, format);
        return postProcess(value);
    }
    return fallback;
}

// KSCFacilityLevelText.cs:67
std::string KSCFacilityLevelText::GetValue(const std::string& valueTag, float facilityLevel) {
    std::string lower = valueTag;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (lower.find("#autoloc") != std::string::npos) return Localizer::Format(valueTag);
    if (valueTag == "K" || valueTag == "ja" || valueTag == "ru" || valueTag == "zh" || valueTag == "es" || valueTag == "fr" ||
        valueTag == "it" || valueTag == "de" || valueTag == "pt")
        return "{" + valueTag + "}";
    GameVariables& gv = *GameVariables::Instance;
    std::string text, text2;
    if (valueTag.find(':') != std::string::npos) {
        const std::vector<std::string> array = dotnet::Split(valueTag, ":", false);
        text = array[0];
        text2 = array[1];
        bool flag = false;
        for (const std::string& n : dotnet::EnumGetNames<SpaceCenterFacility>())
            if (n == text2) {
                flag = true;
                break;
            }
        if (flag) {
            facilityLevel = ScenarioUpgradeableFacilities::GetFacilityLevel(text2);
        } else {
            log::Error("[KSCUpgradeableFacilityText]: Parse Error. No KSC Facility exists called " + text2 + "!");
            text2 = dotnet::EnumToString(facility);
            if (facilityLevel == -1.0f) facilityLevel = ScenarioUpgradeableFacilities::GetFacilityLevel(facility);
        }
    } else {
        text = valueTag;
        text2 = dotnet::EnumToString(facility);
        if (facilityLevel == -1.0f) facilityLevel = ScenarioUpgradeableFacilities::GetFacilityLevel(facility);
    }
    auto loc = [](const char* tag) { return Localizer::Format(tag); };
    auto crewLike = [&](float limit) {
        return GetValueOrFallback(limit, kFloatMax, "", loc("#autoLOC_6002245"), [&](float s) {
            return loc("#autoLOC_6002246") + " " + KSPUtil::LocalizeNumber(f32store(fx(s) * 5.0), "0");
        });
    };
    const bool isVAB = facility == SpaceCenterFacility::VehicleAssemblyBuilding;
    const bool isPad = facility == SpaceCenterFacility::LaunchPad;
    const std::string meters = loc("#autoLOC_7001411");
    if (text == "TrackedObjects")
        return GetValueOrFallback(static_cast<float>(gv.GetTrackedObjectLimit(facilityLevel)), kIntMaxAsFloat, "0", loc("#autoLOC_416922"));
    if (text == "ActionGroupsStock") {
        if (!gv.UnlockedActionGroupsStock(facilityLevel, isVAB)) return "";
        return loc("#autoLOC_6002244");
    }
    if (text == "UnownedObjectsMinSize") {
        if (!gv.UnlockedSpaceObjectDiscovery(facilityLevel)) return "";
        const std::string cls(1, static_cast<char>('A' + static_cast<int>(gv.MinTrackedObjectSize(facilityLevel))));
        return loc("#autoLOC_6002242") + " " + cls + " " + loc("#autoLOC_6002243");
    }
    if (text == "FlightPlanning") {
        if (!gv.UnlockedFlightPlanning(facilityLevel)) return "";
        if (gv.GetOrbitDisplayMode(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::TrackingStation)) !=
            GameVariables::OrbitDisplayMode::PatchedConics)
            return "<color=#ffff14>" + loc("#autoLOC_6002238") + "</color>";
        return loc("#autoLOC_6002239");
    }
    if (text == "ScienceDataRatio")
        return KSPUtil::LocalizeNumber(f32store(fx(gv.GetDataToScienceRatio(facilityLevel)) * 100.0), "0.0##") + "%";
    if (text == "ManeuverTool") {
        if (!gv.ManeuverToolAvailable(facilityLevel)) return "";
        if (!gv.UnlockedFlightPlanning(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::MissionControl)))
            return loc("#autoLOC_6002699");
        return loc("#autoLOC_6002675");
    }
    if (text == "facility") return util::displayDescription(facility);
    if (text == "CrewLevel") return crewLike(gv.GetCrewLevelLimit(facilityLevel));
    if (text == "EVAClamber") {
        if (!gv.UnlockedEVAClamber(facilityLevel)) return "";
        return loc("#autoLOC_6002240");
    }
    if (text == "EVAFlags") {
        if (!gv.UnlockedEVAFlags(facilityLevel)) return "";
        return loc("#autoLOC_416979");
    }
    if (text == "DSNRange")
        return Localizer::Format("#autoLOC_416970", {KSPUtil::PrintSI(gv.GetDSNRange(facilityLevel), "")});
    if (text == "PatchesAhead")
        return GetValueOrFallback(static_cast<float>(gv.GetPatchesAheadLimit(facilityLevel)), kIntMaxAsFloat, "0", loc("#autoLOC_416922"));
    if (text == "StrategyLevel") return crewLike(gv.GetStrategyLevelLimit(facilityLevel));
    if (text == "FuelTransfer") {
        if (!gv.UnlockedFuelTransfer(facilityLevel)) return "";
        return loc("#autoLOC_417008");
    }
    if (text == "ContractCount")
        return GetValueOrFallback(static_cast<float>(gv.GetActiveContractsLimit(facilityLevel)), kIntMaxAsFloat, "0", loc("#autoLOC_416922"));
    if (text == "EVASrfSample") {
        CareerEnvironment& env = CareerEnvironment::Current();
        float requiredExperimentLevel = 0.0f;
        if (env.rndSurfaceSampleExperiment && env.rndSurfaceSampleExperiment(requiredExperimentLevel)) {
            if (gv.UnlockedEVA(ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility::AstronautComplex))) {
                if (!(gv.GetExperimentLevel(facilityLevel) >= requiredExperimentLevel)) return "";
                return loc("#autoLOC_6002241");
            }
            if (!(gv.GetExperimentLevel(facilityLevel) >= requiredExperimentLevel)) return "";
            return "<color=#ffff14>" + loc("#autoLOC_416994") + "</color>";
        }
        return "";
    }
    if (text == "ActionGroupsCustom") {
        if (!gv.UnlockedActionGroupsCustom(facilityLevel, isVAB)) return "";
        return loc("#autoLOC_417015");
    }
    if (text == "CrewCount")
        return GetValueOrFallback(static_cast<float>(gv.GetActiveCrewLimit(facilityLevel)), kIntMaxAsFloat, "0", loc("#autoLOC_416922"));
    if (text == "ActionGroups") {
        if (!gv.UnlockedActionGroupsStock(facilityLevel, isVAB)) return "";
        if (!gv.UnlockedActionGroupsCustom(facilityLevel, isVAB)) return loc("#autoLOC_5050026");
        return loc("#autoLOC_417011");
    }
    if (text == "level")
        return career::FormatNumber(f32store(fx(facilityLevel) * fx(static_cast<float>(ScenarioUpgradeableFacilities::GetFacilityLevelCount(text2)))), "0");
    if (text == "StrategyRange")
        return KSPUtil::LocalizeNumber(f32store(fx(gv.GetStrategyCommitRange(facilityLevel)) * 100.0), "0.0#") + "%";
    if (text == "CraftParts")
        return GetValueOrFallback(static_cast<float>(gv.GetPartCountLimit(facilityLevel, isVAB)), kIntMaxAsFloat, "0", loc("#autoLOC_416922"));
    if (text == "UnownedObjects") {
        if (!gv.UnlockedSpaceObjectDiscovery(facilityLevel)) return "";
        return loc("#autoLOC_417002");
    }
    if (text == "CraftSizeYXZ" || text == "CraftSize") {
        const Vector3f craftSize = gv.GetCraftSizeLimit(facilityLevel, isPad);
        return GetValueOrFallback(craftSize.magnitude(), kFloatMax, "", loc("#autoLOC_416922"), [&](float) {
            return KSPUtil::LocalizeNumber(craftSize.y, "0.0") + meters + ", " + KSPUtil::LocalizeNumber(craftSize.x, "0.0") + meters + ", " +
                   KSPUtil::LocalizeNumber(craftSize.z, "0.0") + meters;
        });
    }
    if (text == "CraftMass")
        return GetValueOrFallback(gv.GetCraftMassLimit(facilityLevel, isPad), kFloatMax, "", loc("#autoLOC_416922"),
                                  [&](float s) { return KSPUtil::LocalizeNumber(s, "0.0#") + loc("#autoLOC_7001407"); });
    if (text == "ScienceCost")
        return GetValueOrFallback(gv.GetScienceCostLimit(facilityLevel), kFloatMax, "0", loc("#autoLOC_416922"));
    if (text == "ContractLevel") return crewLike(gv.GetContractLevelLimit(facilityLevel));
    if (text == "CraftSizeHgtDiam") {
        const Vector3f craftSize = gv.GetCraftSizeLimit(facilityLevel, isPad);
        return GetValueOrFallback(craftSize.magnitude(), kFloatMax, "", loc("#autoLOC_416922"), [&](float) {
            return KSPUtil::LocalizeNumber(craftSize.y, "0.0") + meters + ", " +
                   KSPUtil::LocalizeNumber(Mathf::Sqrt(f32store(fx(Mathf::Pow(craftSize.x, 2.0f)) + fx(Mathf::Pow(craftSize.z, 2.0f)))), "0.0") + meters;
        });
    }
    if (text == "EVA") {
        if (!gv.UnlockedEVA(facilityLevel)) return loc("#autoLOC_7003404");
        return loc("#autoLOC_416977");
    }
    if (text == "StrategyCount")
        return GetValueOrFallback(static_cast<float>(gv.GetActiveStrategyLimit(facilityLevel)), kIntMaxAsFloat, "0", loc("#autoLOC_416922"));
    if (text == "MapMode") {
        switch (gv.GetOrbitDisplayMode(facilityLevel)) {
            case GameVariables::OrbitDisplayMode::AllOrbits: return loc("#autoLOC_416965");
            case GameVariables::OrbitDisplayMode::PatchedConics: return loc("#autoLOC_416967");
            default: return "Map View Available";
        }
    }
    return "";
}

// KSCFacilityLevelText.cs:303
KSCFacilityLevelText KSCFacilityLevelText::CreateFromAsset(const std::string& linePrefix_, const std::string& textBase_, SpaceCenterFacility facility_) {
    KSCFacilityLevelText t;
    t.linePrefix = linePrefix_;
    t.textBase = textBase_;
    t.facility = facility_;
    return t;
}

// ---- UpgradeLevel / UpgradeableObject -------------------------------------------------------------------------------------

// UpgradeableObject.cs:31
void UpgradeLevel::Setup(UpgradeableObject* host) { host_ = host; }

// UpgradeableObject.cs:39 (Instantiate / reparent / Destroy of the old facility transform are the host's: onSpawn)
void UpgradeLevel::Spawn() {
    if (hasFacilityPrefab) {
        facilityInstance = true;
        if (onSpawn) onSpawn(*this);
    }
}

// UpgradeableObject.cs:61
void UpgradeLevel::Despawn() {
    if (onDespawn) onDespawn(*this);
    facilityInstance = false;
}

// UpgradeableObject.cs:139
void UpgradeableObject::CompileID() {
    id = compiledId;
    preCompiledId = true;
}

// UpgradeableObject.cs:145
void UpgradeableObject::ClearID() {
    id = "";
    preCompiledId = false;
}

// UpgradeableObject.cs:162
void UpgradeableObject::SetupLevels() {
    int num = static_cast<int>(upgradeLevels_.size());
    while (num-- > 0) upgradeLevels_[static_cast<size_t>(num)].Setup(this);
    setup_ = true;
}

// UpgradeableObject.cs:172
void UpgradeableObject::setLevel(int lvl) {
    if (!setup_) SetupLevels();
    lvl = Mathf::Clamp(lvl, 0, MaxLevel());
    if (!upgradeLevels_[static_cast<size_t>(lvl)].Spawned()) {
        Despawn(facilityLevel_);
        facilityLevel_ = lvl;
        currentLevel_ = &upgradeLevels_[static_cast<size_t>(facilityLevel_)];
        currentLevel_->Spawn();
        GE::OnUpgradeableObjLevelChange.Fire(this, lvl);
    }
}

// UpgradeableObject.cs:189
void UpgradeableObject::Despawn(int lvl) {
    lvl = Mathf::Clamp(lvl, 0, MaxLevel());
    Despawn(upgradeLevels_[static_cast<size_t>(lvl)]);
}

// UpgradeableObject.cs:195
void UpgradeableObject::Despawn(UpgradeLevel& lvl) {
    if (lvl.Spawned()) lvl.Despawn();
}

// UpgradeableObject.cs:203
void UpgradeableObject::Awake() {
    SetupLevels();
    OnAwake();
}

// UpgradeableObject.cs:209
void UpgradeableObject::Start() {
    OnStart();
    setLevel(facilityLevel_);
    GE::onGameSceneSwitchRequested.Add(this, &UpgradeableObject::OnGameSceneSwitchRequested);
}

// UpgradeableObject.cs:216
void UpgradeableObject::OnGameSceneSwitchRequested(events::GameEvents::FromToAction<GameScenes, GameScenes> action) {
    const GameScenes to = action.to;
    const GameScenes loaded = SaveEnvironment::Current().loadedScene;
    if ((loaded != GameScenes::SPACECENTER || to != GameScenes::FLIGHT) && (loaded != GameScenes::FLIGHT || to != GameScenes::SPACECENTER))
        Despawn(facilityLevel_);
    if ((to == GameScenes::SPACECENTER || to == GameScenes::FLIGHT) && !upgradeLevels_[static_cast<size_t>(facilityLevel_)].Spawned()) {
        upgradeLevels_[static_cast<size_t>(facilityLevel_)].Spawn();
        GE::OnUpgradeableObjLevelChange.Fire(this, facilityLevel_);
    }
}

// UpgradeableObject.cs:230
void UpgradeableObject::OnDestroy() {
    GE::onGameSceneSwitchRequested.Remove(this, &UpgradeableObject::OnGameSceneSwitchRequested);
    OnOnDestroy();
}

// UpgradeableObject.cs:242
void UpgradeableObject::SetLevel(int lvl) { setLevel(lvl); }

// ---- UpgradeableFacility --------------------------------------------------------------------------------------------------

UpgradeableFacility::~UpgradeableFacility() {
    if (eventsAdded_) OnOnDestroy();
}

// UpgradeableFacility.cs:9
void UpgradeableFacility::OnAwake() { setLevel(MaxLevel()); }

// UpgradeableFacility.cs:14
void UpgradeableFacility::OnStart() {
    GE::onLevelWasLoaded.Add(this, &UpgradeableFacility::OnLevelLoaded);
    GE::Mission::onMissionsBuilderPQSLoaded.Add(this, &UpgradeableFacility::OnMissionsBuilderPQSLoaded);
    eventsAdded_ = true;
}

// UpgradeableFacility.cs:20
void UpgradeableFacility::OnOnDestroy() {
    GE::onLevelWasLoaded.Remove(this, &UpgradeableFacility::OnLevelLoaded);
    GE::Mission::onMissionsBuilderPQSLoaded.Remove(this, &UpgradeableFacility::OnMissionsBuilderPQSLoaded);
    eventsAdded_ = false;
}

// UpgradeableFacility.cs:26
bool UpgradeableFacility::RegisterInstance() {
    if (!preCompiledId) id = compiledId;
    bool result = false;
    if (id.empty())
        log::Error("[UpgradeableFacility]: ID for this facility is not defined correctly. Make sure all IDs are compiled and serialized before running the game.");
    else
        result = ScenarioUpgradeableFacilities::RegisterUpgradeable(this, id);
    return result;
}

// UpgradeableFacility.cs:44
void UpgradeableFacility::UnregisterInstance() {
    if (id.empty()) {
        log::Error("[UpgradeableFacility]: ID for this facility is not defined correctly. Cannot unregister.");
        return;
    }
    ScenarioUpgradeableFacilities::UnregisterUpgradeable(this, id);
    log::Info("[" + id + "]: Instance unregistered.");
}

// UpgradeableFacility.cs:55
void UpgradeableFacility::OnMissionsBuilderPQSLoaded() {
    if (SaveEnvironment::Current().loadedScene == GameScenes::MISSIONBUILDER && activeInHierarchy)
        CareerEnvironment::Current().Scheduler().StartCoroutine(LoadOnSceneStart());
}

// UpgradeableFacility.cs:63
void UpgradeableFacility::OnLevelLoaded(GameScenes scn) {
    if ((static_cast<unsigned>(static_cast<int>(scn) - 5) <= 2u || scn == GameScenes::MISSIONBUILDER) && activeInHierarchy)
        CareerEnvironment::Current().Scheduler().StartCoroutine(LoadOnSceneStart());
}

// UpgradeableFacility.cs:71
Enumerator UpgradeableFacility::LoadOnSceneStart() {
    co_yield YieldInstruction::WaitForEndOfFrame();
    if (!RegisterInstance()) Reset();
}

// UpgradeableFacility.cs:80
void UpgradeableFacility::Reset() { setLevel(MaxLevel()); }

// UpgradeableFacility.cs:85
void UpgradeableFacility::SetLevel(int lvl) {
    GE::OnKSCFacilityUpgrading.Fire(this, lvl);
    setLevel(lvl);
    if (activeInHierarchy) CareerEnvironment::Current().Scheduler().StartCoroutine(waitForLevelSpawn(lvl));
}

// UpgradeableFacility.cs:95
Enumerator UpgradeableFacility::waitForLevelSpawn(int lvl) {
    while (CurrentLevel() == nullptr || !CurrentLevel()->Spawned()) co_yield YieldInstruction::Null();
    co_yield YieldInstruction::Null();
    GE::OnKSCFacilityUpgraded.Fire(this, lvl);
}

// UpgradeableFacility.cs:105
void UpgradeableFacility::Save(ConfigNode& node) { node.AddValue("lvl", GetNormLevel()); }

// UpgradeableFacility.cs:110
void UpgradeableFacility::Load(const ConfigNode* node) {
    if (node == nullptr) {
        log::Info("[" + id + "]: Loading... Node is null");
        setLevel(0);
        return;
    }
    if (node->HasValue("lvl")) {
        const float normLevel = dotnet::ParseSingle(*node->GetValue("lvl"));
        setNormLevel(normLevel);
    }
    if (node->HasValue("level")) {
        facilityLevel_ = dotnet::ParseInt32(*node->GetValue("level"));
        setLevel(facilityLevel_);
    }
}

// UpgradeableFacility.cs:130
void UpgradeableFacility::setNormLevel(float lvl) {
    lvl = Mathf::Clamp01(lvl);
    facilityLevel_ = Mathf::FloorToInt(f32store(fx(lvl) * fx(static_cast<float>(MaxLevel() + 1))));
    facilityLevel_ = Mathf::Clamp(facilityLevel_, 0, MaxLevel());
    setLevel(facilityLevel_);
}

// UpgradeableFacility.cs:138
float UpgradeableFacility::GetNormLevel() { return f32store(fx(static_cast<float>(facilityLevel_)) / fx(static_cast<float>(MaxLevel()))); }

// UpgradeableFacility.cs:143
float UpgradeableFacility::GetUpgradeCost() {
    if (facilityLevel_ != MaxLevel()) {
        const float cost = upgradeLevels_[static_cast<size_t>(facilityLevel_ + 1)].levelCost;
        if (SaveEnvironment::Current().loadedSceneIsGame) return f32store(fx(cost) * fx(CurrentGame()->Parameters->Career.FundsLossMultiplier));
        return cost;
    }
    return 0.0f;
}

// UpgradeableFacility.cs:156
float UpgradeableFacility::GetDowngradeCost() {
    if (facilityLevel_ != 0) {
        const float cost = upgradeLevels_[static_cast<size_t>(facilityLevel_ - 1)].levelCost;
        if (SaveEnvironment::Current().loadedSceneIsGame)
            return f32store(fx(cost) * fx(0.667f) * fx(CurrentGame()->Parameters->Career.FundsLossMultiplier));
        return f32store(fx(cost) * fx(0.667f));
    }
    return 0.0f;
}

// UpgradeableFacility.cs:169
std::string UpgradeableFacility::GetLevelText() { return GetLevelText(FacilityLevel()); }

// UpgradeableFacility.cs:174
std::string UpgradeableFacility::GetNextLevelText() { return GetLevelText(FacilityLevel() + 1); }

// UpgradeableFacility.cs:179
std::string UpgradeableFacility::GetLevelText(int lvl) {
    if (lvl <= MaxLevel() && lvl >= 0) {
        UpgradeLevel& l = upgradeLevels_[static_cast<size_t>(lvl)];
        if (l.levelStats) return l.levelStats->GetText(f32store(fx(static_cast<float>(lvl)) / fx(static_cast<float>(MaxLevel()))));
        if (l.levelCost != 0.0f) log::Error("[" + name + "]: No Level Text for level " + std::to_string(lvl) + "!");
        return "";
    }
    return "";
}

}  // namespace Upgradeables

// ---- ScenarioUpgradeableFacilities ------------------------------------------------------------------------------------------

namespace {
std::unordered_map<std::string, std::string>& SlashSanitized() {
    static std::unordered_map<std::string, std::string> m;
    return m;
}
std::map<SpaceCenterFacility, std::string>& FacilityStrings() {
    static std::map<SpaceCenterFacility, std::string> m;
    return m;
}
Game* CurrentGameU() { return SaveEnvironment::Current().currentGame; }
}  // namespace

dotnet::Dictionary<std::string, std::shared_ptr<ScenarioUpgradeableFacilities::ProtoUpgradeable>>& ScenarioUpgradeableFacilities::protoUpgradeables() {
    static dotnet::Dictionary<std::string, std::shared_ptr<ProtoUpgradeable>> d;
    return d;
}

void ScenarioUpgradeableFacilities::ClearStatics() {
    protoUpgradeables().Clear();
    SlashSanitized().clear();
    FacilityStrings().clear();
}

// ScenarioUpgradeableFacilities.cs:24
ScenarioUpgradeableFacilities::ProtoUpgradeable::ProtoUpgradeable(Upgradeables::UpgradeableFacility* facility) {
    facilityRefs.push_back(facility);
    level_ = facility->GetNormLevel();
}

// ScenarioUpgradeableFacilities.cs:31
ScenarioUpgradeableFacilities::ProtoUpgradeable::ProtoUpgradeable(ConfigNode::NodePtr node) : configNode(std::move(node)) {}

// ScenarioUpgradeableFacilities.cs:37
void ScenarioUpgradeableFacilities::ProtoUpgradeable::AddRef(Upgradeables::UpgradeableFacility* facility) {
    if (facilityRefs.empty()) level_ = facility->GetNormLevel();
    facilityRefs.push_back(facility);
}

// ScenarioUpgradeableFacilities.cs:46
void ScenarioUpgradeableFacilities::ProtoUpgradeable::Load(ConfigNode::NodePtr node) {
    configNode = std::move(node);
    int count = static_cast<int>(facilityRefs.size());
    while (count-- > 0) facilityRefs[static_cast<size_t>(count)]->Load(configNode.get());
    GetLevel();
}

// ScenarioUpgradeableFacilities.cs:57 (configNode keeps a copy of the saved node: the C# keeps the reference to the caller's node)
void ScenarioUpgradeableFacilities::ProtoUpgradeable::Save(ConfigNode& node) {
    if (!facilityRefs.empty()) {
        facilityRefs[0]->Save(node);
        configNode = node.CreateCopy();
        level_ = static_cast<float>(facilityRefs[0]->FacilityLevel());
    } else {
        configNode->CopyTo(node);
    }
}

// ScenarioUpgradeableFacilities.cs:71
float ScenarioUpgradeableFacilities::ProtoUpgradeable::GetLevel() {
    if (!facilityRefs.empty())
        level_ = facilityRefs[0]->GetNormLevel();
    else if (configNode != nullptr && configNode->HasValue("lvl"))
        level_ = dotnet::ParseSingle(*configNode->GetValue("lvl"));
    else
        level_ = 1.0f;
    return level_;
}

// ScenarioUpgradeableFacilities.cs:88
int ScenarioUpgradeableFacilities::ProtoUpgradeable::GetLevelCount() {
    if (!facilityRefs.empty()) return facilityRefs[0]->MaxLevel();
    return -1;
}

ScenarioUpgradeableFacilities::ScenarioUpgradeableFacilities() : ScenarioModule("ScenarioUpgradeableFacilities") {}
ScenarioUpgradeableFacilities::~ScenarioUpgradeableFacilities() {
    // (the C# object outlives its Unity destroy; here the listener OnDestroy added must not outlive the object)
    if (eventsAdded_) GE::onLevelWasLoaded.Remove(this, &ScenarioUpgradeableFacilities::CleanUpUnmanagedData);
    if (Instance != nullptr && Instance == this) Instance = nullptr;
}

// ScenarioUpgradeableFacilities.cs:106
void ScenarioUpgradeableFacilities::OnAwake() {
    if (Instance != nullptr) return;  // Object.Destroy(this)
    Instance = this;
    protoUpgradeables().Clear();
}

// ScenarioUpgradeableFacilities.cs:117
void ScenarioUpgradeableFacilities::OnDestroy() {
    if (Instance != nullptr && Instance == this) {
        Instance = nullptr;
        // Application.isPlaying: the level-loaded cleanup listener
        GE::onLevelWasLoaded.Add(this, &ScenarioUpgradeableFacilities::CleanUpUnmanagedData);
        eventsAdded_ = true;
    }
}

// ScenarioUpgradeableFacilities.cs:129
void ScenarioUpgradeableFacilities::CleanUpUnmanagedData(GameScenes scn) {
    GE::onLevelWasLoaded.Remove(this, &ScenarioUpgradeableFacilities::CleanUpUnmanagedData);
    eventsAdded_ = false;
    if (scn == GameScenes::MAINMENU) {
        log::Info("[ScenarioUpgradeables]: Back to Main Menu. Clearing persistent data.");
        protoUpgradeables().Clear();
        return;
    }
    bool flag = true;
    const auto& scenarios = CurrentGameU()->scenarios;
    int count = static_cast<int>(scenarios.size());
    while (count-- > 0) {
        if (scenarios[static_cast<size_t>(count)]->moduleName == "ScenarioUpgradeableFacilities") {
            flag = false;
            break;
        }
    }
    if (flag) {
        log::Info("[ScenarioUpgradeables]: Loaded Game has no Upgradeables Scenario Module. Clearing persistent data.");
        protoUpgradeables().Clear();
    }
}

// ScenarioUpgradeableFacilities.cs:155
bool ScenarioUpgradeableFacilities::RegisterUpgradeable(Upgradeables::UpgradeableFacility* facilityRef, const std::string& id) {
    auto& d = protoUpgradeables();
    if (!d.ContainsKey(id)) {
        d.Add(id, std::make_shared<ProtoUpgradeable>(facilityRef));
    } else {
        ProtoUpgradeable& p = *d.At(id);
        if (std::find(p.facilityRefs.begin(), p.facilityRefs.end(), facilityRef) == p.facilityRefs.end()) p.AddRef(facilityRef);
        if (Instance != nullptr) {
            facilityRef->Load(p.configNode.get());
            return true;
        }
    }
    return false;
}

// ScenarioUpgradeableFacilities.cs:176
void ScenarioUpgradeableFacilities::UnregisterUpgradeable(Upgradeables::UpgradeableFacility* facilityRef, const std::string& id) {
    auto& d = protoUpgradeables();
    if (d.ContainsKey(id)) {
        auto& refs = d.At(id)->facilityRefs;
        auto it = std::find(refs.begin(), refs.end(), facilityRef);
        if (it != refs.end()) refs.erase(it);
    }
}

// ScenarioUpgradeableFacilities.cs:184
std::string ScenarioUpgradeableFacilities::GetFacilityName(SpaceCenterFacility facility) {
    switch (facility) {
        case SpaceCenterFacility::Administration: return Localizer::Format("#autoLOC_6001644");
        case SpaceCenterFacility::AstronautComplex: return Localizer::Format("#autoLOC_6001643");
        case SpaceCenterFacility::LaunchPad: return Localizer::Format("#autoLOC_6001663");
        case SpaceCenterFacility::MissionControl: return Localizer::Format("#autoLOC_6001645");
        case SpaceCenterFacility::ResearchAndDevelopment: return Localizer::Format("#autoLOC_6001646");
        case SpaceCenterFacility::Runway: return Localizer::Format("#autoLOC_6001662");
        case SpaceCenterFacility::TrackingStation: return Localizer::Format("#autoLOC_6001648");
        case SpaceCenterFacility::SpaceplaneHangar: return Localizer::Format("#autoLOC_6001661");
        case SpaceCenterFacility::VehicleAssemblyBuilding: return Localizer::Format("#autoLOC_6001660");
        default: return "Unlocalised facility name!";
    }
}

// ScenarioUpgradeableFacilities.cs:211
float ScenarioUpgradeableFacilities::GetFacilityLevel(SpaceCenterFacility facility) {
    auto& fs = FacilityStrings();
    if (fs.find(facility) == fs.end()) fs[facility] = dotnet::EnumToString(facility);
    return GetFacilityLevel(fs[facility]);
}

// ScenarioUpgradeableFacilities.cs:220
float ScenarioUpgradeableFacilities::GetFacilityLevel(const std::string& facilityId_) {
    const std::string facilityId = SlashSanitize(facilityId_);
    float result = 1.0f;
    auto& d = protoUpgradeables();
    if (d.Count() > 0) {
        auto* p = d.Find(facilityId);
        if (p != nullptr) result = (*p)->GetLevel();
    }
    return result;
}

// ScenarioUpgradeableFacilities.cs:231
std::string ScenarioUpgradeableFacilities::SlashSanitize(const std::string& instr) {
    auto& m = SlashSanitized();
    auto it = m.find(instr);
    if (it != m.end()) return it->second;
    const bool flag = instr.find('/') != std::string::npos;
    m[instr] = flag ? instr : "SpaceCenter/" + instr;
    return m[instr];
}

// ScenarioUpgradeableFacilities.cs:258
int ScenarioUpgradeableFacilities::GetFacilityLevelCount(SpaceCenterFacility facility) {
    auto& fs = FacilityStrings();
    if (fs.find(facility) == fs.end()) fs[facility] = dotnet::EnumToString(facility);
    return GetFacilityLevelCount(fs[facility]);
}

// ScenarioUpgradeableFacilities.cs:267
int ScenarioUpgradeableFacilities::GetFacilityLevelCount(const std::string& facilityId_) {
    const std::string facilityId = SlashSanitize(facilityId_);
    int result = 1;
    auto& d = protoUpgradeables();
    if (d.Count() > 0) {
        auto* p = d.Find(facilityId);
        if (p != nullptr)
            result = (*p)->GetLevelCount();
        else
            log::Error("[ScenarioUpgradeableFacilities]: No facility exists with id " + facilityId);
    }
    return result;
}

// ScenarioUpgradeableFacilities.cs:285
bool ScenarioUpgradeableFacilities::IsRunway(const std::string& launchLocation) {
    return launchLocation == "Runway" || launchLocation == "Island_Airfield" || launchLocation == "Desert_Airfield";
}

// ScenarioUpgradeableFacilities.cs:294
bool ScenarioUpgradeableFacilities::IsLaunchPad(const std::string& launchLocation) {
    return launchLocation == "LaunchPad" || launchLocation == "Woomerang_Launch_Site" || launchLocation == "Desert_Launch_Site";
}

// ScenarioUpgradeableFacilities.cs:303
void ScenarioUpgradeableFacilities::OnSave(ConfigNode& node) {
    for (const std::string& current : protoUpgradeables().Keys()) {
        ConfigNode* n = node.AddNode(current);
        protoUpgradeables().At(current)->Save(*n);
    }
}

// ScenarioUpgradeableFacilities.cs:313
void ScenarioUpgradeableFacilities::OnLoad(ConfigNode& node) {
    if (node.CountNodes() == 0) {
        log::Info("[ScenarioUpgradeableFacilities]: Loading in initial state... ");
        if (CurrentGameU()->Mode != Game::Modes::MISSION)
            getInitialState(node).CopyTo(node);
        else
            getInitialMissionState(node).CopyTo(node);
    }
    log::Info("[ScenarioUpgradeableFacilities]: Loading... " + std::to_string(protoUpgradeables().Count()) + " objects registered");
    const int count = node.CountNodes();
    for (int i = 0; i < count; ++i) {
        ConfigNode::NodePtr configNode = node.nodes().Shared(i);
        auto* existing = protoUpgradeables().Find(configNode->name);
        if (existing != nullptr)
            (*existing)->Load(configNode);
        else
            protoUpgradeables().Add(configNode->name, std::make_shared<ProtoUpgradeable>(configNode));
    }
}

// ScenarioUpgradeableFacilities.cs:343
ConfigNode ScenarioUpgradeableFacilities::getInitialState(const ConfigNode& node) {
    ConfigNode configNode;
    node.CopyTo(configNode);
    CareerEnvironment& env = CareerEnvironment::Current();
    if (env.spaceCenterFacilities) {
        for (const auto& f : env.spaceCenterFacilities()) {
            if (!f.upgradeableObjectId.empty()) {
                auto n = ConfigNode::Create(f.upgradeableObjectId);
                n->AddValue("lvl", 0.0f);
                configNode.AddNode(n);
            }
        }
    }
    return configNode;
}

// ScenarioUpgradeableFacilities.cs:362
ConfigNode ScenarioUpgradeableFacilities::getInitialMissionState(const ConfigNode& node) {
    // (the "mission mode by mistake" branch of the C# is unreachable from OnLoad)
    ConfigNode configNode;
    node.CopyTo(configNode);
    CareerEnvironment& env = CareerEnvironment::Current();
    if (env.spaceCenterFacilities) {
        for (const auto& f : env.spaceCenterFacilities()) {
            if (!f.upgradeableFacilityId.empty()) {
                auto n = ConfigNode::Create(f.upgradeableFacilityId);
                const float num2 = env.missionsFacilityLimit ? env.missionsFacilityLimit(f.facilityName, f.maxLevel) : static_cast<float>(f.maxLevel);
                n->AddValue("lvl", f32store((fx(num2) - fx(1.0f)) / fx(static_cast<float>(f.maxLevel))));
                configNode.AddNode(n);
            }
        }
    }
    return configNode;
}

// ScenarioUpgradeableFacilities.cs:388
void ScenarioUpgradeableFacilities::CheatFacilities() {
    for (const auto& kv : protoUpgradeables()) {
        ProtoUpgradeable& current = *kv.second;
        int count = static_cast<int>(current.facilityRefs.size());
        while (count-- > 0) {
            Upgradeables::UpgradeableFacility* f = current.facilityRefs[static_cast<size_t>(count)];
            f->SetLevel(f->MaxLevel());
        }
    }
}

void ScenarioUpgradeableFacilities::Register() {
    ScenarioModuleFactory::Register("ScenarioUpgradeableFacilities",
                                    [](const std::string&) { return std::unique_ptr<ScenarioModule>(new ScenarioUpgradeableFacilities()); });
}

}  // namespace ksp
