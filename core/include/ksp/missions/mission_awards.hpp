// WP-54: Expansions/Missions/{AwardDefinition,Awards,AwardModule,AwardModule_*,MissionAwards}.cs.
//
// The award definitions (id, awardName, displayName, description, icon sprite) are the serialized list of the `MEAwards.prefab` of the
// Making History asset bundle (Awards.internalAwards): they are not in any cfg and have not been extracted yet (see STATUS.md "Needs data").
// A host (or a test) fills `Awards::internalAwards`. The icon (a Sprite made from a GameDatabase texture) is not modelled.
//
// Vessel-side reads of the modules (FlightGlobals.Vessels / ActiveVessel / PersistentVesselIds, Vessel.GetConnectedResourceTotals,
// PartResourceLibrary, HighLogic.CurrentGame.CrewRoster) go through AwardEnvironment; the events are the real GameEvents.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <optional>

#include "ksp/config/config_node.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/missions/dynamic_module.hpp"
#include "ksp/missions/mission_node.hpp"
#include "ksp/util/collections.hpp"

namespace ksp {
class Vessel;
class EventReport;
class CelestialBody;
}  // namespace ksp

namespace ksp::Expansions::Missions {

class Mission;

// AwardDefinition.cs:7
class AwardDefinition {
public:
    std::string id;           // :9
    std::string name;         // :11
    std::string displayName;  // :13
    std::string description;  // :15
    void Load(const ConfigNode& node);  // :19 (the `icon` texture lookup is GameDatabase / Sprite: not modelled)
    void Save(ConfigNode& node) const;  // :36
};

// Awards.cs:8 (a MonoBehaviour on the prefab; Awake builds the id dictionary)
class Awards {
public:
    Awards() = default;                                          // Awake :18 (call SetupAwards after filling internalAwards)
    std::vector<AwardDefinition> internalAwards;                 // :11 [SerializeField]
    std::vector<AwardDefinition*> AwardDefinitions;              // :16
    void SetupAwards();                                          // :24
    AwardDefinition* GetAwardDefinition(const std::string& id);  // :35

private:
    std::map<std::string, AwardDefinition*> awardsDictionary_;   // :13
};

struct AwardEnvironment {
    // (HighLogic.LoadedSceneIsMissionBuilder ? MissionEditorLogic.Instance.awardDefinitions : MissionSystem.awardDefinitions). Unset: null.
    static inline std::function<Awards*()> awardDefinitions;
    // FlightGlobals.Vessels. Unset: FlightRegistry::Vessels.
    static inline std::function<std::vector<Vessel*>()> vessels;
    // FlightGlobals.ActiveVessel. Unset: FlightRegistry::ActiveVessel.
    static inline std::function<Vessel*()> activeVessel;
    // FlightGlobals.PersistentVesselIds[id] (nullptr when the id is not there). Unset: the vessels list searched by persistentId.
    static inline std::function<Vessel*(uint32_t persistentId)> vesselByPersistentId;
    // FlightGlobals.FindVessel(persistentId, out vessel). Unset: vesselByPersistentId.
    static inline std::function<bool(uint32_t persistentId, Vessel*& vessel)> findVessel;
    // PartResourceLibrary.Instance.GetDefinition(name).id != -1 then vessel.GetConnectedResourceTotals(id, out amount, out max). Unset: false.
    static inline std::function<bool(Vessel*, const std::string& resourceName, double& amount)> connectedResourceTotal;
    // HighLogic.CurrentGame.Mode == MISSION (the PROGRESS nodes are only written in a running mission). Unset: HighLogic.CurrentGame.
    static inline std::function<bool()> currentGameIsMission;
    // HighLogic.CurrentGame.CrewRoster: the rosterStatus of every kerbal (AwardModule_KerbalsDied). Unset: HighLogic.CurrentGame->CrewRoster.
    static inline std::function<std::vector<int>()> crewRosterStatuses;
    // The LaunchPad facility: (pqsName, facility transform parent position) of PSystemSetup.GetSpaceCenterFacility("LaunchPad")
    // (AwardModule_LandedOn). Unset: PSystemSetup::Instance.
    struct LaunchPad {
        std::string pqsName;
        bool hasFacilityPQS = false;
        Vector3d position;
    };
    static inline std::function<std::optional<LaunchPad>()> launchPad;
    // FlightGlobals.GetBodyByName. Unset: SaveEnvironment::Current().localBodies.
    static inline std::function<CelestialBody*(const std::string&)> getBodyByName;

    // PartResourceLibrary.Instance.GetDefinition(name).id (-1 when the library or the name is unknown)
    static int ResourceDefinitionId(const std::string& resourceName);
    static std::vector<Vessel*> Vessels();
    static Vessel* ActiveVessel();
    static Vessel* VesselByPersistentId(uint32_t persistentId);
    static bool ConnectedResourceTotal(Vessel* vessel, const std::string& resourceName, double& amount);
    static bool CurrentGameIsMission();
    static void Reset();
};

// AwardModule.cs:9
class AwardModule : public DynamicModule {
public:
    explicit AwardModule(std::string typeName, MENode* node);                      // :37
    AwardModule(std::string typeName, MENode* node, AwardDefinition* definition);   // :43
    bool enabled = false;                                                           // :11
    float score = 0.0f;                                                             // :13
    AwardDefinition* Definition();                                                  // :19
    std::string GetDisplayName() override;                                          // :48
    void EvaluateAward(Mission* mission, std::vector<std::string>& awards);          // :63 (`ref List<string>`; the list is created when null in C#)
    virtual void StartTracking() {}                                                 // :77
    virtual void StopTracking() {}                                                  // :81
    void Load(const ConfigNode& node) override;                                     // :85
    void Save(ConfigNode& node) const override;                                     // :93

protected:
    virtual bool EvaluateCondition(Mission* mission);                               // :57
    std::string awardID;                                                            // :17

private:
    AwardDefinition* definition_ = nullptr;                                         // :15
};

// AwardModule_Fuel.cs:7
class AwardModule_Fuel : public DynamicModuleT<AwardModule_Fuel, AwardModule> {
public:
    static constexpr const char* kTypeName = "AwardModule_Fuel";
    explicit AwardModule_Fuel(MENode* node);                                        // :19
    AwardModule_Fuel(MENode* node, AwardDefinition* definition);                    // :24
    ~AwardModule_Fuel() override;
    std::string resourceName = "LiquidFuel";                                        // :10
    double resourceAmount = 0.0;                                                    // :13
    void StartTracking() override;                                                  // :37
    void StopTracking() override;                                                   // :47
    void Load(const ConfigNode& node) override;                                     // :82
    void Save(ConfigNode& node) const override;                                     // :107

protected:
    bool EvaluateCondition(Mission* mission) override;                              // :29

private:
    void OnVesselWillDestroy(Vessel* data);                                         // :66
    void OnVesselCreated(Vessel* data);                                             // :75
    double GetCurrentResourceFromVessel(Vessel* v);                                 // :55
    DictionaryValueList<Vessel*, double> vesselResource;                      // :15 DictionaryValueList<Vessel, double>
    double totalSpentResource = 0.0;                                                // :17
    int resourceDefinitionId = -1;                                                  // :19 resourceDefinition.id
    bool subscribed_ = false;
};

// AwardModule_KerbalsDied.cs:3
class AwardModule_KerbalsDied : public DynamicModuleT<AwardModule_KerbalsDied, AwardModule> {
public:
    static constexpr const char* kTypeName = "AwardModule_KerbalsDied";
    explicit AwardModule_KerbalsDied(MENode* node);                                 // :5
    AwardModule_KerbalsDied(MENode* node, AwardDefinition* definition);             // :9

protected:
    bool EvaluateCondition(Mission* mission) override;                              // :14
};

// AwardModule_KerbalsSurvived.cs:3
class AwardModule_KerbalsSurvived : public DynamicModuleT<AwardModule_KerbalsSurvived, AwardModule> {
public:
    static constexpr const char* kTypeName = "AwardModule_KerbalsSurvived";
    explicit AwardModule_KerbalsSurvived(MENode* node);                             // :7
    AwardModule_KerbalsSurvived(MENode* node, AwardDefinition* definition);         // :11
    ~AwardModule_KerbalsSurvived() override;
    void StartTracking() override;                                                  // :24
    void StopTracking() override;                                                   // :29
    void Load(const ConfigNode& node) override;                                     // :40
    void Save(ConfigNode& node) const override;                                     // :49

protected:
    bool EvaluateCondition(Mission* mission) override;                              // :19
    bool kerbalsAreAlive = true;                                                    // :5

private:
    void OnCrewKilled(EventReport* report);                                         // :34
    bool subscribed_ = false;
};

// AwardModule_LandedOn.cs:8
class AwardModule_LandedOn : public DynamicModuleT<AwardModule_LandedOn, AwardModule> {
public:
    static constexpr const char* kTypeName = "AwardModule_LandedOn";
    explicit AwardModule_LandedOn(MENode* node);                                    // :16
    AwardModule_LandedOn(MENode* node, AwardDefinition* definition);                // :20
    uint32_t vesselID = 0;                                                          // :11
    double distance = 0.0;                                                          // :14
    void Load(const ConfigNode& node) override;                                     // :56
    void Save(ConfigNode& node) const override;                                     // :63

protected:
    bool EvaluateCondition(Mission* mission) override;                              // :25

private:
    Vessel* GetVessel();                                                            // :40
};

// AwardModule_MissionTime.cs:5
class AwardModule_MissionTime : public DynamicModuleT<AwardModule_MissionTime, AwardModule> {
public:
    static constexpr const char* kTypeName = "AwardModule_MissionTime";
    explicit AwardModule_MissionTime(MENode* node);                                 // :10
    AwardModule_MissionTime(MENode* node, AwardDefinition* definition);             // :14
    double missionTime = 0.0;                                                       // :8
    void Load(const ConfigNode& node) override;                                     // :24
    void Save(ConfigNode& node) const override;                                     // :30

protected:
    bool EvaluateCondition(Mission* mission) override;                              // :19
};

// AwardModule_PlanetsLanded.cs:6
class AwardModule_PlanetsLanded : public DynamicModuleT<AwardModule_PlanetsLanded, AwardModule> {
public:
    static constexpr const char* kTypeName = "AwardModule_PlanetsLanded";
    explicit AwardModule_PlanetsLanded(MENode* node);                               // :14
    AwardModule_PlanetsLanded(MENode* node, AwardDefinition* definition);           // :19
    ~AwardModule_PlanetsLanded() override;
    int planetsLanded = 0;                                                          // :9
    void StartTracking() override;                                                  // :29
    void StopTracking() override;                                                   // :34
    void Load(const ConfigNode& node) override;                                     // :47
    void Save(ConfigNode& node) const override;                                     // :58

protected:
    bool EvaluateCondition(Mission* mission) override;                              // :24
    std::vector<std::string> landedOn;                                              // :11

private:
    void OnVesselSitutationChange(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> data);  // :39
    bool subscribed_ = false;
};

// AwardModule_Score.cs:3
class AwardModule_Score : public DynamicModuleT<AwardModule_Score, AwardModule> {
public:
    static constexpr const char* kTypeName = "AwardModule_Score";
    explicit AwardModule_Score(MENode* node);                                       // :7
    AwardModule_Score(MENode* node, AwardDefinition* definition);                   // :13
    bool canBeDisplayedInEditor() const override { return false; }                  // :5
    void Load(const ConfigNode& node) override;                                     // :28
    void Save(ConfigNode& node) const override;                                     // :33

protected:
    bool EvaluateCondition(Mission* mission) override;                              // :19
};

// AwardModule_TraveledLessThan.cs:5 / AwardModule_TraveledMoreThan.cs:5 (identical but for the comparison)
class AwardModule_TraveledLessThan : public DynamicModuleT<AwardModule_TraveledLessThan, AwardModule> {
public:
    static constexpr const char* kTypeName = "AwardModule_TraveledLessThan";
    explicit AwardModule_TraveledLessThan(MENode* node);                            // :13
    AwardModule_TraveledLessThan(MENode* node, AwardDefinition* definition);        // :17
    ~AwardModule_TraveledLessThan() override;
    double distance = 0.0;                                                          // :8
    void StartTracking() override;                                                  // :34
    void StopTracking() override;                                                   // :39
    void Load(const ConfigNode& node) override;                                     // :49
    void Save(ConfigNode& node) const override;                                     // :59

protected:
    bool EvaluateCondition(Mission* mission) override;                              // :22
    double distanceTraveled = 0.0;                                                  // :10

private:
    void OnVesselWillDestroy(Vessel* data);                                         // :44
    bool subscribed_ = false;
};
class AwardModule_TraveledMoreThan : public DynamicModuleT<AwardModule_TraveledMoreThan, AwardModule> {
public:
    static constexpr const char* kTypeName = "AwardModule_TraveledMoreThan";
    explicit AwardModule_TraveledMoreThan(MENode* node);                            // :13
    AwardModule_TraveledMoreThan(MENode* node, AwardDefinition* definition);        // :17
    ~AwardModule_TraveledMoreThan() override;
    double distance = 0.0;                                                          // :8
    void StartTracking() override;                                                  // :34
    void StopTracking() override;                                                   // :39
    void Load(const ConfigNode& node) override;                                     // :49
    void Save(ConfigNode& node) const override;                                     // :59

protected:
    bool EvaluateCondition(Mission* mission) override;                              // :22
    double distanceTraveled = 0.0;                                                  // :10

private:
    void OnVesselWillDestroy(Vessel* data);                                         // :44
    bool subscribed_ = false;
};

// AwardModule_VesselsUsed.cs:6
class AwardModule_VesselsUsed : public DynamicModuleT<AwardModule_VesselsUsed, AwardModule> {
public:
    static constexpr const char* kTypeName = "AwardModule_VesselsUsed";
    explicit AwardModule_VesselsUsed(MENode* node);                                 // :14
    AwardModule_VesselsUsed(MENode* node, AwardDefinition* definition);             // :19
    ~AwardModule_VesselsUsed() override;
    int vesselsUsed = 0;                                                            // :9
    void StartTracking() override;                                                  // :29
    void StopTracking() override;                                                   // :34
    void Load(const ConfigNode& node) override;                                     // :44
    void Save(ConfigNode& node) const override;                                     // :60

protected:
    bool EvaluateCondition(Mission* mission) override;                              // :24
    std::vector<uint32_t> vesselsUsedList;                                          // :11

private:
    void OnVesselChange(Vessel* data);                                              // :39
    bool subscribed_ = false;
};

// MissionAwards.cs:10
class MissionAwards : public DynamicModuleList {
public:
    explicit MissionAwards(MENode* node);                                           // :15
    std::vector<std::string> awardedAwards;                                         // :12
    void FillAwardsList();                                                          // :22
    bool ContainsDefinition(const AwardDefinition& definition);                     // :41
    std::vector<std::string> GetSupportedTypes() override;                          // :54
    void EvaluateAwards(Mission* mission);                                          // :75
    void StartTracking();                                                           // :87
    void StopTracking();                                                            // :101
    std::unique_ptr<DynamicModuleList> Clone() const override;
};

// Registers the AwardModule family with DynamicModuleRegistry (idempotent).
void RegisterAwardModules();

}  // namespace ksp::Expansions::Missions
