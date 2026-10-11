// WP-53: the Breaking Ground ground-part modules: ModuleGroundPart.cs, ModuleGroundSciencePart.cs, ModuleGroundCommsPart.cs,
// ModuleGroundExpControl.cs, ModuleGroundExperiment.cs (Assembly-CSharp root). The class chain is
//     PartModule > ModuleCargoPart > ModuleGroundPart > ModuleGroundSciencePart > { ModuleGroundCommsPart > ModuleGroundExpControl,
//                                                                                    ModuleGroundExperiment }
// The schema (KSPField / KSPEvent / KSPAction attribute data and C# initial values) is the generated module_schema.inc /
// module_members.inc, so every [KSPField] stays in the module's typed store (Get<T>/Set<T>); C# properties become accessors.
//
// Seams (SerenityEnvironment, serenity_env.hpp): Rigidbody / FixedJoint / ground contact, the EVA kerbal's inventory, raycast line of
// sight and panel rotation of sun trackers, vessel-marker / map-object refreshes, ScreenMessages. Unity's Start() runs the frame after
// AddComponent: the host calls `Start()` (kspcore does not simulate frames); "Object.Destroy(this)" is `destroyed` + `behaviourEnabled`.
#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ksp/commnet/comm_network.hpp"
#include "ksp/commnet/control_source.hpp"
#include "ksp/core/coroutine.hpp"
#include "ksp/modules/animated_module.hpp"
#include "ksp/modules/module_cargo_part.hpp"
#include "ksp/save/proto_vessel.hpp"
#include "ksp/serenity/serenity_env.hpp"
#include "ksp/util/collections.hpp"

namespace ksp {

class ModuleAnimationGroup;
class ModuleGroundExpControl;
class ModuleGroundExperiment;
class ScienceExperiment;
namespace Expansions::Serenity::DeployedScience::Runtime {
class DeployedScienceCluster;
class DeployedScienceExperiment;
class DeployedSciencePart;
}  // namespace Expansions::Serenity::DeployedScience::Runtime

// ModuleGroundPart.cs:7
class ModuleGroundPart : public ModuleCargoPart, public IAnimatedModule {
public:
    explicit ModuleGroundPart(const ModuleClassInfo& cls) : ModuleCargoPart(cls) {}
    ~ModuleGroundPart() override;
    IAnimatedModule* AsAnimatedModule() override { return this; }

    // [KSPField] accessors
    bool placementAllowXRotation() const { return Get<bool>("placementAllowXRotation"); }  // :14
    bool placementAllowYRotation() const { return Get<bool>("placementAllowYRotation"); }  // :17
    bool placementAllowZRotation() const { return Get<bool>("placementAllowZRotation"); }  // :20
    bool beingDeployed() const { return Get<bool>("beingDeployed"); }                      // :38 protected
    bool deployedOnGround() const { return Get<bool>("deployedOnGround"); }                // :41 protected

    // MonoBehaviour state
    bool behaviourEnabled = true;  // MonoBehaviour.enabled
    bool destroyed = false;        // Object.Destroy(this)

    // protected / private members (:9-:35)
    bool beingRetrieved = false;                                       // :35
    ModuleGroundExpControl* groundExpConModule = nullptr;              // :25
    ModuleAnimationGroup* animationGroup = nullptr;                    // :27
    int firstEmptyInvSlot = 0;                                         // :11
    bool hasJoint = false;                                             // :31 FixedJoint joint != null
    DictionaryValueList<uint32_t, float> childMass;                    // :33
    bool childMassValid = false;                                       // childMass != null
    bool partActionWindowOpen = false;                                 // :29 (UIPartActionWindow != null)

    // ---- events / lifecycle ----
    void OnAwake() override;                                           // binds [KSPEvent] RetrievePart
    virtual void RetrievePart();                                       // :48 [KSPEvent]
    bool CanBeStored();                                                // :84
    virtual bool HasChildParts();                                      // :106
    void RetrieveScienceData();                                        // :127
    void OnPartPack() override;                                        // :144
    void OnPartUnpack() override;                                      // :156
    void OnStart(StartState state) override;                           // :177
    void OnDestroy() override;                                         // :210
    void OnUpdate() override;                                          // :227
    virtual Enumerator MakePartKinematic();                            // :236
    void SetDeployedOnGroundFromCoRoutine();                           // :278 (SendMessage target)
    void OnLeavingScene(GameScenes scn);                               // :321
    void OnPartWillDie(Part* p);                                       // :329
    void PlayDeployAnimation() override;                               // :338
    void PlayRetractAnimation();                                       // :350
    void OnRetractCompleted(ModuleAnimationGroup* aGroup);             // :363
    void OnPartActionUIShown(Part* p, bool open);                      // :388/:397 (UIPartActionWindow shown / dismissed)
    void OnVesselChange(Vessel* vessel);                               // :405
    void UpdateGroundModuleUI();                                       // :413 private non-virtual in C#; ModuleGroundSciencePart declares its own virtual UpdateModuleUI
    std::string GetModuleDisplayName() override;                       // :426
    void EnableModule() override;                                      // :431
    void DisableModule() override;                                     // :439 virtual
    bool ModuleIsActive() override;                                    // :444 virtual
    bool IsSituationValid() override;                                  // :465 virtual
    static void CacheLocalStrings() {}                                 // :474 (Localizer.Format is taken at use)

    // the EVA kerbal's inventory (SerenityEnvironment::activeKerbalInventory when the active vessel is an EVA), set by OnVesselChange
    Expansions::Serenity::SerenityEnvironment::KerbalInventory* kerbalInventoryModule = nullptr;  // :9

protected:
    std::shared_ptr<Coroutine> kinematicCoroutine_;
};

// ModuleGroundSciencePart.cs:11
class ModuleGroundSciencePart : public ModuleGroundPart {
public:
    using Cluster = Expansions::Serenity::DeployedScience::Runtime::DeployedScienceCluster;
    explicit ModuleGroundSciencePart(const ModuleClassInfo& cls) : ModuleGroundPart(cls) {}
    ~ModuleGroundSciencePart() override;

    // [KSPField] state behind the properties (:21-:69)
    std::string ConnectionState() const { return Get<std::string>("ConnectionState"); }  // :14
    std::string PowerState() const { return Get<std::string>("PowerState"); }            // :17
    uint32_t ControlUnitId() const { return Get<uint32_t>("ControlUnitId"); }            // :35
    void SetControlUnitId(uint32_t v) { Set<uint32_t>("ControlUnitId", v); }
    float DistanceToController() const { return Get<float>("DistanceToController"); }    // :38
    void SetDistanceToController(float v) { Set<float>("DistanceToController", v); }
    bool TrackSun() const { return Get<bool>("TrackSun"); }                              // :48

    // properties with GameEvents side effects
    int PowerUnitsProduced() const { return Get<int32_t>("powerUnitsProduced"); }               // :111
    void SetPowerUnitsProduced(int v);
    int ActualPowerUnitsProduced() const { return Get<int32_t>("actualPowerUnitsProduced"); }   // :130
    void SetActualPowerUnitsProduced(int v);
    int PowerUnitsRequired() const { return Get<int32_t>("powerUnitsRequired"); }               // :149
    void SetPowerUnitsRequired(int v);
    bool IsSolarPanel() const { return Get<bool>("isSolarPanel"); }                             // :168
    void SetIsSolarPanel(bool v);
    bool Enabled() const { return Get<bool>("moduleEnabled"); }                                 // :187
    void SetEnabled(bool v);
    bool DeployedOnGround() const { return deployedOnGround(); }                                // :207
    Cluster* ScienceClusterData();                                                              // :209

    // private / protected members
    Cluster* scienceClusterData = nullptr;  // :45
    bool trackingLOS = false;               // :75
    std::string blockingObject;             // :77
    bool sunTrackerRunning = false;         // SunTrackerToOrigin is active

    void ToggleActive();                    // :222 [KSPEvent]
    void ToggleActiveAction();              // :231 [KSPAction]
    void Start();                           // :237 (Unity message)
    void OnAwake() override;                // :246
    void OnStart(StartState state) override;  // :252
    void OnDestroy() override;              // :303
    void OnUpdate() override;               // :314
    void FixedUpdate();                     // :323 (Unity message)
    bool CalculateTrackingLOS(std::string& blocker);  // :370 (raycasts: SerenityEnvironment::trackingLineOfSight)
    void SetDeployedOnGround();             // :415 internal
    void OnLoad(const ConfigNode& node) override;  // :423
    void RetrievePart() override;           // :439
    Enumerator SunTrackerToOrigin();        // :467
    void OnGroundScienceClusterUpdated(ModuleGroundExpControl* controlUnit, Cluster* cluster);  // :480
    void OnGroundScienceClusterPowerStateChanged(Cluster* scienceCluster);                       // :488
    void OnGroundScienceDeregisterCluster(uint32_t controlUnitId);                               // :496
    virtual void UpdateModuleUI();          // :540 protected virtual (a NEW virtual: ModuleGroundPart's private one is not overridden)
    std::string GetInfo() override;         // :581
    std::string GetModuleDisplayName() override;  // :632
    std::string GetTooltip() override;      // :637
    static void CacheLocalStrings() {}      // :646

protected:
    bool subscribedClusterEvents_ = false;
};

// ModuleGroundCommsPart.cs:6
class ModuleGroundCommsPart : public ModuleGroundSciencePart, public CommNet::ICommAntenna {
public:
    explicit ModuleGroundCommsPart(const ModuleClassInfo& cls) : ModuleGroundSciencePart(cls) {}

    bool antennaCombinable() const { return Get<bool>("antennaCombinable"); }                 // :9
    double antennaCombinableExponent() const { return Get<double>("antennaCombinableExponent"); }  // :12
    double antennaPower() const { return Get<double>("antennaPower"); }                       // :24
    AntennaType antennaType() const;                                                 // :15

    // ICommAntenna (:32-:42, :44, :49, :81, :99)
    bool CommCombinable() const override { return antennaCombinable(); }
    double CommCombinableExponent() const override { return antennaCombinableExponent(); }
    AntennaType CommType() const override { return antennaType(); }
    const DoubleCurve* CommRangeCurve() const override;
    const DoubleCurve* CommScienceCurve() const override;
    double CommPower() override { return CommPowerValue(); }
    bool CanScienceTo(bool combined, double bPower, double sqrDistance) override;
    bool CanComm() override;
    bool CanCommUnloaded(ProtoPartModuleSnapshot* mSnap) override;
    double CommPowerUnloaded(ProtoPartModuleSnapshot* mSnap) override;
    std::string GetInfo() override;                      // :113
    std::string GetModuleDisplayName() override;         // :133

protected:
    virtual double CommPowerValue() { return antennaPower(); }  // `public virtual double CommPower => antennaPower` (ExpControl overrides)

private:
    ModuleGroundExpControl* controllerModule_ = nullptr;  // :26
    Part* loadedPart_ = nullptr;                          // :28
};

// ModuleGroundExpControl.cs:11
class ModuleGroundExpControl : public ModuleGroundCommsPart, public CommNet::ICommNetControlSource {
public:
    explicit ModuleGroundExpControl(const ModuleClassInfo& cls) : ModuleGroundCommsPart(cls) {}
    ~ModuleGroundExpControl() override;

    int controlUnitRange() const { return Get<int32_t>("controlUnitRange"); }       // :14
    int experimentsConnected() const { return Get<int32_t>("experimentsConnected"); }  // :17
    double boostedAntennaPower() const { return Get<double>("boostedAntennaPower"); }  // :29
    bool beingDestroyed = false;                                                    // :31
    double CommPowerValue() override { return antennaPower() + boostedAntennaPower(); }  // :39 override CommPower

    void SetVesselNaming();            // :42 [KSPEvent] (Part.SetVesselNaming: PAW / naming dialog, recorded)
    int vesselNamingCalls = 0;
    void Start();                      // :47
    void OnAwake() override;           // :56
    void OnDestroy() override;         // :76
    void OnStart(StartState state) override;  // :87
    void OnSave(ConfigNode& node) override;   // :111
    void OnUpdate() override;          // :119
    std::string GetInfo() override;    // :128
    void OnGroundScienceClusterPowerStateChanged(Cluster* cluster);               // :151
    void OnGroundScienceModuleRemoved(ModuleGroundSciencePart* sciencePart);      // :159
    void OnGroundSciencePartEnabledStateChanged(ModuleGroundSciencePart* sciencePart);  // :174
    void OnGroundScienceClusterUpdated(ModuleGroundExpControl* controlUnit, Cluster* cluster);  // :182
    Enumerator ScanAndAddControlUnit();                                           // :191
    void OnGroundSciencePartDeployed(ModuleGroundSciencePart* sciencePart);       // :222
    void SetDeployedSciencePartVesselType(ModuleGroundSciencePart* sciencePart);  // :247
    void UpdateModuleUI() override;                                               // :281
    void RecalcBoosterPower(ModuleGroundSciencePart* sciencePart);                // :311
    double CommPowerUnloaded(ProtoPartModuleSnapshot* mSnap) override;            // :332
    std::string GetModuleDisplayName() override;                                  // :343
    std::string GetTooltip() override;                                            // :348
    VesselControlState GetControlSourceState() const override;                    // :357
    bool IsCommCapable() const override;                                          // :370
    void UpdateNetwork() override;                                                // :383
    std::string ControlSourceName() const override { return "ModuleGroundExpControl"; }  // ICommNetControlSource.name (MonoBehaviour.name)
    static void CacheLocalStrings() {}                                            // :399

    bool partActionWindow_ = false;
    std::shared_ptr<Coroutine> scanCoroutine_;
};

// ModuleGroundExperiment.cs:6
class ModuleGroundExperiment : public ModuleGroundSciencePart {
public:
    using DSExperiment = Expansions::Serenity::DeployedScience::Runtime::DeployedScienceExperiment;
    using DSPart = Expansions::Serenity::DeployedScience::Runtime::DeployedSciencePart;
    explicit ModuleGroundExperiment(const ModuleClassInfo& cls) : ModuleGroundSciencePart(cls) {}
    ~ModuleGroundExperiment() override;

    std::string experimentId() const { return Get<std::string>("experimentId"); }  // :9
    float ScienceLimit() const { return Get<float>("scienceLimit"); }              // :42
    void SetScienceLimit(float v);
    float ScienceValue() const { return scienceValue; }                            // :58
    void SetScienceValue(float v);
    float ScienceModifierRate() const { return Get<float>("ScienceModifierRate"); }  // :29
    void SetScienceModifierRate(float v) { Set<float>("ScienceModifierRate", v); }
    const FloatCurve* DistanceCurve() const;                                       // :32

    float scienceValue = 0.0f;                  // :16
    DSExperiment* scienceExperiment = nullptr;  // :34
    DSPart* deployedSciencePart = nullptr;      // :36
    ScienceExperiment* experiment = nullptr;    // :38

    void Start();                               // :74
    void OnStart(StartState state) override;    // :84
    void OnDestroy() override;                  // :96
    void OnLoad(const ConfigNode& node) override;  // :102
    void OnGroundScienceGenerated(DSExperiment* scienceExperiment, DSPart* sciencePart, Cluster* cluster, float scienceGenerated);  // :117
    void OnGroundScienceTransmitted(DSExperiment* scienceExperiment, DSPart* sciencePart, Cluster* cluster, float scienceTransmitted);  // :125
    void UpdateModuleUI() override;             // :133
    void RetrievePart() override;               // :168
    std::string GetInfo() override;             // :186
    std::string GetModuleDisplayName() override;  // :203
    static void CacheLocalStrings() {}          // :208
};

namespace detail {
void RegisterSerenityModules();  // serenity/ground_modules.cpp: registers the five module classes (called from RegisterBuiltinModules)
}

}  // namespace ksp
