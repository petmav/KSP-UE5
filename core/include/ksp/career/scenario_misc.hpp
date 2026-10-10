// WP-36: ScenarioAchievements.cs, ScenarioDestructibles.cs, VesselRecovery.cs and the data side of MissionRecoveryDialog
// (KSP.UI.Screens.MissionRecoveryDialog, WP-46 owns the UI object: the fields and the Add*Widget calls the recovery pipeline
// uses are kept here as plain data so the pipeline runs without a GUI).
//
// DestructibleBuilding (DestructibleBuilding.cs, WP-42) is a MonoBehaviour with Save / Load / IsIntact / id /
// FacilityDamageFraction; ScenarioDestructibles only needs those, so it talks to the IDestructibleBuilding interface (WP-42's
// class implements it).
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/career/career_environment.hpp"
#include "ksp/career/currency.hpp"
#include "ksp/career/progress.hpp"
#include "ksp/dotnet/collections.hpp"
#include "ksp/save/game.hpp"

namespace ksp {

// The members of DestructibleBuilding ScenarioDestructibles.ProtoDestructible uses
class IDestructibleBuilding {
public:
    virtual ~IDestructibleBuilding() = default;
    virtual const std::string& BuildingId() const = 0;    // DestructibleBuilding.id
    virtual void Save(ConfigNode& node) = 0;              // DestructibleBuilding.Save
    virtual void Load(const ConfigNode& node) = 0;        // DestructibleBuilding.Load
    virtual bool IsIntact() const = 0;                    // DestructibleBuilding.IsIntact
    virtual float FacilityDamageFraction() const = 0;     // DestructibleBuilding.FacilityDamageFraction
};

// ScenarioDestructibles.cs:11 ([KSPScenario(3198, SPACECENTER, FLIGHT, EDITOR, TRACKSTATION)])
class ScenarioDestructibles : public ScenarioModule {
public:
    // :13
    class ProtoDestructible {
    public:
        ConfigNode::NodePtr configNode;                       // :15
        std::vector<IDestructibleBuilding*> dBuildingRefs;    // :17
        explicit ProtoDestructible(IDestructibleBuilding* dBuilding);   // :23
        explicit ProtoDestructible(ConfigNode::NodePtr node);           // :32
        void AddRef(IDestructibleBuilding* dBuilding);        // :38
        void Load(ConfigNode::NodePtr node);                  // :47
        void Save(ConfigNode& node);                          // :58
        float GetDamage();                                    // :72

    private:
        bool isIntact_ = false;
        float FacilityDamageFraction_ = 0.0f;
    };

    ScenarioDestructibles();
    ~ScenarioDestructibles() override;
    static inline ScenarioDestructibles* Instance = nullptr;    // :97
    static dotnet::Dictionary<std::string, std::shared_ptr<ProtoDestructible>>& protoDestructibles();     // :99
    static dotnet::Dictionary<std::string, std::vector<std::shared_ptr<ProtoDestructible>>>& facilityToDestructibles();  // :101
    void OnAwake() override;                    // :103
    void OnDestroy();                           // :115
    void OnSave(ConfigNode& node) override;     // :123
    void OnLoad(ConfigNode& node) override;     // :182
    static std::string GetFacility(const std::string& dName);   // :133
    static bool AddTo(dotnet::Dictionary<std::string, std::vector<std::shared_ptr<ProtoDestructible>>>& dict, const std::string& key,
                      const std::shared_ptr<ProtoDestructible>& pD);   // :157
    static void RemoveFrom(dotnet::Dictionary<std::string, std::vector<std::shared_ptr<ProtoDestructible>>>& dict, const std::string& key,
                           const std::shared_ptr<ProtoDestructible>& pD);   // :170
    static bool RegisterDestructible(IDestructibleBuilding* dBuilding, const std::string& id);   // :200
    static void UnregisterDestructible(IDestructibleBuilding* dBuilding, const std::string& id, bool saveState);  // :223
    static float GetFacilityDamage(std::string facilityName);   // :235
    static void Register();
    static void ClearStatics();
};

// ScenarioAchievements.cs:16 ([KSPScenario(AddToAllGames, FLIGHT, TRACKSTATION, SPACECENTER, EDITOR)])
class ScenarioAchievements : public ScenarioModule {
public:
    // :18 (the Steam achievement provider)
    class IAchievementProvider {
    public:
        virtual ~IAchievementProvider() = default;
        virtual void SetAchievementComplete(const std::string& id) = 0;   // :20
        virtual bool GetAchievementComplete(const std::string& id) = 0;   // :22
    };
    // :25
    class DebugAchievementProvider : public IAchievementProvider {
    public:
        void SetAchievementComplete(const std::string& id) override;   // :29
        bool GetAchievementComplete(const std::string& id) override;   // :35
    private:
        std::map<std::string, bool> achievements_;
    };

    ScenarioAchievements();
    ~ScenarioAchievements() override;
    void OnLoad(ConfigNode&) override {}     // :49
    void OnSave(ConfigNode&) override {}     // :53
    void OnAwake() override;                 // :57
    void Start();                            // :124 (the EndInTutorials coroutine is a seam: TutorialScenario is another package)
    void OnDestroy();                        // :129
    static void SetProvider(IAchievementProvider* ap);   // :147
    // tutorial scenarios (TutorialScenario.ExclusiveTutorial) are not ported: a host that has one calls EndInTutorials(true)
    void EndInTutorials(bool exclusiveTutorialPresent);  // :165
    static void Register();

    // the handlers (private in C#)
    void OnLaunch(Vessel* vessel);                                        // :188
    void OnFlyby(Vessel* vessel, CelestialBody* body);                    // :204
    void OnOrbit(Vessel* vessel, CelestialBody* body);                    // :219
    void OnLanding(Vessel* vessel, CelestialBody* body);                  // :232
    void OnReturnFromOrbit(Vessel* vessel, CelestialBody* body);          // :256
    void OnTargetedLanding(Vessel* vessel, std::string to, KSPAchievements::ReturnFrom from);  // :264
    void OnContractCompleted(Contracts::Contract* contract);              // :272
    void OnVesselRecovered(ProtoVessel* protoVessel, KSP::UI::Screens::MissionRecoveryDialog* dialog, float amount);  // :277
    void OnCrewEVA(events::GameEvents::FromToAction<Part*, Part*> action);   // :297
    void OnFlagPlant(Vessel* vessel);                                     // :311
    void OnPartsCoupled(events::GameEvents::FromToAction<Part*, Part*> action);  // :319
    static const std::map<std::string, bool>& achievements() { return achievements_; }
    bool DestroyRequested() const { return destroyRequested_; }  // Object.Destroy(this) was called (the host destroys the module)

private:
    void CheckAchievement(const std::string& id);   // :152
    static inline IAchievementProvider* provider_ = nullptr;
    static inline std::map<std::string, bool> achievements_;
    bool eventsAdded_ = false;
    bool destroyRequested_ = false;
};

}  // namespace ksp

namespace ksp::KSP::UI::Screens {

// KSP.UI.Screens.MissionRecoveryDialog (data side; the widgets are recorded, not drawn)
class MissionRecoveryDialog {
public:
    struct PartWidget { std::string partName; float dryCost; float fuelCost; };
    struct ResourceWidget { std::string resourceName; float amount; float unitCost; };
    struct CrewWidget { std::string name; };
    double fundsEarned = 0.0;
    float reputationEarned = 0.0f;
    float scienceEarned = 0.0f;
    double beforeMissionFunds = 0.0;
    float beforeMissionReputation = 0.0f;
    float beforeMissionScience = 0.0f;
    double totalFunds = 0.0;
    float totalReputation = 0.0f;
    float totalScience = 0.0f;
    std::string recoveryLocation, recoveryFactor, FundsModifier, RepModifier, ScienceModifier;
    std::vector<PartWidget> partWidgets;
    std::vector<ResourceWidget> resourceWidgets;
    std::vector<CrewWidget> crewWidgets;
    void AddPartWidget(PartWidget w) { partWidgets.push_back(std::move(w)); }
    void AddResourceWidget(ResourceWidget w) { resourceWidgets.push_back(std::move(w)); }
    void AddCrewWidget(CrewWidget w) { crewWidgets.push_back(std::move(w)); }
    // MissionRecoveryDialog.CreateFullDialog(pv)
    static std::unique_ptr<MissionRecoveryDialog> CreateFullDialog(ProtoVessel*) { return std::make_unique<MissionRecoveryDialog>(); }
};

}  // namespace ksp::KSP::UI::Screens

namespace ksp {

// VesselRecovery.cs:15 ([KSPScenario(3198, SPACECENTER, FLIGHT, TRACKSTATION, EDITOR)])
class VesselRecovery : public ScenarioModule {
public:
    VesselRecovery();
    ~VesselRecovery() override;
    void OnAwake() override;       // :17
    void OnDestroy();              // :22
    // :31 the recovery value maths. `dialog` receives the data the C# puts into the MissionRecoveryDialog (nullptr for quick).
    void OnVesselRecovered(ProtoVessel* pv, bool quick);
    // The recovery factor of OnVesselRecovered (num3 of the C#: the funds / reputation multiplier passed to
    // onVesselRecoveryProcessing) for a vessel `distance` metres from the space centre: pure function of (distance, radius, landedAt)
    struct RecoveryFactor {
        float num3 = 0.0f;  // the factor passed to onVesselRecoveryProcessing
        float num4 = 0.0f;  // the unmodified factor (vanilla), used for the "(+x%)" text
    };
    static RecoveryFactor ComputeRecoveryFactor(double num, double num2, float minDelta, float maxDelta, const std::string& landedAt);  // :37-:70
    static void Register();
    // the last dialog created by a non-quick recovery (CreateFullDialog; the UI owns it in KSP)
    KSP::UI::Screens::MissionRecoveryDialog* lastDialog() { return lastDialog_.get(); }

private:
    bool checkLaunchSites(ProtoVessel* pv, float& recoveryFactor, float& vanillaFactor);   // :140
    void recoverVesselCrew(ProtoVessel* pv, KSP::UI::Screens::MissionRecoveryDialog* mrDialog);  // :160
    std::unique_ptr<KSP::UI::Screens::MissionRecoveryDialog> lastDialog_;
    bool eventsAdded_ = false;
};

}  // namespace ksp
