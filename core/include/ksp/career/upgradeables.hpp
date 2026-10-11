// WP-36: the upgradeable KSC facilities. Upgradeables/UpgradeableObject.cs, UpgradeableFacility.cs, KSCFacilityLevelText.cs,
// ScenarioUpgradeableFacilities.cs, SpaceCenterFacility.cs.
//
// Data side only. The visual part of a level (facilityPrefab: Instantiate / Destroy of the facility GameObject, the transform
// bookkeeping p0/r0/s0) is a seam: UpgradeLevel::Spawn / Despawn flip `spawned` and call the optional hooks the host installs.
// The level costs (UpgradeLevel.levelCost) and the number of levels are Unity-serialized on the KSC scene's UpgradeableFacility
// components: a host fills `UpgradeLevels` (STATUS.md "needs data").
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ksp/career/career_environment.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/dotnet/enum.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/save/game.hpp"

namespace ksp {

// SpaceCenterFacility.cs:2 is declared in save/save_enums.hpp (WP-42: one definition for WP-36 and WP-40; the [Description]s are below)

}  // namespace ksp

template <>
struct ksp::dotnet::EnumTraits<ksp::SpaceCenterFacility> {
    static constexpr bool flags = false;
    static constexpr EnumMember<ksp::SpaceCenterFacility> members[] = {
        {ksp::SpaceCenterFacility::Administration, "Administration", "#autoLOC_6001644"},
        {ksp::SpaceCenterFacility::AstronautComplex, "AstronautComplex", "#autoLOC_6001643"},
        {ksp::SpaceCenterFacility::LaunchPad, "LaunchPad", "#autoLOC_6001663"},
        {ksp::SpaceCenterFacility::MissionControl, "MissionControl", "#autoLOC_6001645"},
        {ksp::SpaceCenterFacility::ResearchAndDevelopment, "ResearchAndDevelopment", "#autoLOC_6001646"},
        {ksp::SpaceCenterFacility::Runway, "Runway", "#autoLOC_6001662"},
        {ksp::SpaceCenterFacility::TrackingStation, "TrackingStation", "#autoLOC_6001648"},
        {ksp::SpaceCenterFacility::SpaceplaneHangar, "SpaceplaneHangar", "#autoLOC_6001661"},
        {ksp::SpaceCenterFacility::VehicleAssemblyBuilding, "VehicleAssemblyBuilding", "#autoLOC_6001660"}};
};

namespace ksp::Upgradeables {

// KSCFacilityLevelText.cs:8 ([Serializable]; the text of a level, with [Tag] replacement blocks evaluated against GameVariables)
class KSCFacilityLevelText {
public:
    std::string textBase;                                    // :10
    SpaceCenterFacility facility = SpaceCenterFacility::Administration;  // :12
    std::string linePrefix = "*";                            // :14
    std::string GetText(float normLevel);                    // :18
    // :303 (KSCUpgradeableLevelText is a ScriptableObject asset: its three serialized fields)
    static KSCFacilityLevelText CreateFromAsset(const std::string& linePrefix, const std::string& textBase, SpaceCenterFacility facility);

private:
    std::string ProcessTextTags(std::string src, float level);  // :36
    std::string GetValue(const std::string& valueTag, float facilityLevel);  // :67
    static std::string GetValueOrFallback(float value, float threshold, const std::string& format, const std::string& fallback,
                                          const std::function<std::string(float)>& postProcess = nullptr);  // :290
};

class UpgradeableObject;

// UpgradeableObject.cs:9 UpgradeLevel
class UpgradeLevel {
public:
    float levelCost = 0.0f;                              // :11
    std::string levelText;                               // :13 (KSCUpgradeableLevelText asset name, informational)
    std::shared_ptr<KSCFacilityLevelText> levelStats;    // :15
    bool hasFacilityPrefab = false;                      // :17 (facilityPrefab != null)
    bool facilityInstance = false;                       // :19 (facilityInstance != null)
    bool Spawned() const { return facilityInstance; }    // :29
    void Setup(UpgradeableObject* host);                 // :31
    void Spawn();                                        // :39 (the Instantiate / reparenting is the host's: onSpawn)
    void Despawn();                                      // :61
    std::function<void(UpgradeLevel&)> onSpawn, onDespawn;  // seams for the GameObject work

private:
    UpgradeableObject* host_ = nullptr;
};

// UpgradeableObject.cs:6 (abstract MonoBehaviour)
class UpgradeableObject {
public:
    UpgradeableObject() = default;
    virtual ~UpgradeableObject() = default;
    UpgradeableObject(const UpgradeableObject&) = delete;
    UpgradeableObject& operator=(const UpgradeableObject&) = delete;

    std::string id;                  // :81
    bool preCompiledId = false;      // :84
    std::string name;                // GameObject.name (UpgradeableFacility.GetLevelText logs it)
    std::string compiledId;          // HierarchyUtil.CompileID(transform, "SpaceCenter") of this object (the host fills it)
    bool activeInHierarchy = true;   // gameObject.activeInHierarchy
    std::vector<UpgradeLevel>& UpgradeLevels() { return upgradeLevels_; }   // :98
    int FacilityLevel() const { return facilityLevel_; }                   // :122
    int MaxLevel() const { return static_cast<int>(upgradeLevels_.size()) - 1; }  // :124
    UpgradeLevel* CurrentLevel() { return currentLevel_; }                  // :126
    void CompileID();                // :139 (id = compiledId, preCompiledId = true)
    void ClearID();                  // :145
    void SetupLevels();              // :162
    void setLevel(int lvl);          // :172
    void Despawn(int lvl);           // :189
    void Despawn(UpgradeLevel& lvl); // :195
    void Awake();                    // :203
    void Start();                    // :209
    void OnGameSceneSwitchRequested(events::GameEvents::FromToAction<GameScenes, GameScenes> action);  // :216
    void OnDestroy();                // :230
    virtual void SetLevel(int lvl);  // :242

protected:
    virtual void OnAwake() = 0;      // :236
    virtual void OnStart() = 0;      // :238
    virtual void OnOnDestroy() = 0;  // :240
    std::vector<UpgradeLevel> upgradeLevels_;  // :87
    int facilityLevel_ = 0;          // :92
    UpgradeLevel* currentLevel_ = nullptr;     // :94
    bool setup_ = false;             // :96
    friend class UpgradeLevel;
};

// UpgradeableFacility.cs:7
class UpgradeableFacility : public UpgradeableObject {
public:
    bool RegisterInstance();                          // :26
    void UnregisterInstance();                        // :44
    void SetLevel(int lvl) override;                  // :85
    void Save(ConfigNode& node);                      // :105
    void Load(const ConfigNode* node);                // :110 (nullptr = the "Node is null" path)
    void setNormLevel(float lvl);                     // :130
    float GetNormLevel();                             // :138
    float GetUpgradeCost();                           // :143
    float GetDowngradeCost();                         // :156
    std::string GetLevelText();                       // :169
    std::string GetNextLevelText();                   // :174
    std::string GetLevelText(int lvl);                // :179
    // the scene events of OnStart (:14); public so a host can drive them
    void OnLevelLoaded(GameScenes scn);               // :63
    void OnMissionsBuilderPQSLoaded();                // :55
    void Reset();                                     // :80
    ~UpgradeableFacility() override;

protected:
    void OnAwake() override;       // :9
    void OnStart() override;       // :14
    void OnOnDestroy() override;   // :20

private:
    Enumerator LoadOnSceneStart();                    // :71
    Enumerator waitForLevelSpawn(int lvl);            // :95
    bool eventsAdded_ = false;
};

}  // namespace ksp::Upgradeables

namespace ksp {

// ScenarioUpgradeableFacilities.cs:14 ([KSPScenario(1056, SPACECENTER, EDITOR, FLIGHT, TRACKSTATION)])
class ScenarioUpgradeableFacilities : public ScenarioModule {
public:
    // :16
    class ProtoUpgradeable {
    public:
        ConfigNode::NodePtr configNode;                                   // :18 (the node object, shared like the C# reference)
        std::vector<Upgradeables::UpgradeableFacility*> facilityRefs;     // :20
        explicit ProtoUpgradeable(Upgradeables::UpgradeableFacility* facility);   // :24
        explicit ProtoUpgradeable(ConfigNode::NodePtr node);                      // :31
        void AddRef(Upgradeables::UpgradeableFacility* facility);          // :37
        void Load(ConfigNode::NodePtr node);                               // :46
        void Save(ConfigNode& node);                                       // :57
        float GetLevel();                                                  // :71
        int GetLevelCount();                                               // :88

    private:
        float level_ = 0.0f;
    };

    ScenarioUpgradeableFacilities();
    ~ScenarioUpgradeableFacilities() override;
    static inline ScenarioUpgradeableFacilities* Instance = nullptr;       // :98
    // :100
    static dotnet::Dictionary<std::string, std::shared_ptr<ProtoUpgradeable>>& protoUpgradeables();

    void OnAwake() override;                           // :106
    void OnDestroy();                                  // :117
    void CleanUpUnmanagedData(GameScenes scn);         // :129
    static bool RegisterUpgradeable(Upgradeables::UpgradeableFacility* facilityRef, const std::string& id);   // :155
    static void UnregisterUpgradeable(Upgradeables::UpgradeableFacility* facilityRef, const std::string& id);  // :176
    static std::string GetFacilityName(SpaceCenterFacility facility);        // :184
    static float GetFacilityLevel(SpaceCenterFacility facility);              // :211
    static float GetFacilityLevel(const std::string& facilityId);             // :220
    static std::string SlashSanitize(const std::string& instr);               // :231
    static int GetFacilityLevelCount(SpaceCenterFacility facility);           // :258
    static int GetFacilityLevelCount(const std::string& facilityId);          // :267
    static bool IsRunway(const std::string& launchLocation);                  // :285
    static bool IsLaunchPad(const std::string& launchLocation);               // :294
    void OnSave(ConfigNode& node) override;                                   // :303
    void OnLoad(ConfigNode& node) override;                                   // :313
    void CheatFacilities();                                                   // :388
    static void Register();
    static void ClearStatics();   // tests: protoUpgradeables / the sanitize caches

private:
    ConfigNode getInitialState(const ConfigNode& node);          // :343
    ConfigNode getInitialMissionState(const ConfigNode& node);   // :362
    bool eventsAdded_ = false;
};

}  // namespace ksp
