// WP-55: Assembly-CSharp/Expansions/Missions/Adjusters/*.cs (Expansions.Missions.Adjusters): the part-module adjusters and failures, which are
// DynamicModules (registered in DynamicModuleRegistry under their class names; ActionPartFailure / ActionPartRepair keep them in a
// DynamicModuleList). See node_library.hpp for the conventions.
//
// Seams: `adjustedModule as ModuleX` + the calls on it go through NodeLibraryEnvironment::moduleIsA / moduleCall / moduleQuery /
// moduleText (the PartModule side of the adjuster lists is Part.AddPartModuleAdjusterList / PartModule.RemovePartModuleAdjuster of the part
// port; the BaseFieldList / BaseEventList reflection of Setup / CleanUp is the host's `adjusterAttach` hook).
// `typeof(ModuleX)` is the class name string (GetTargetPartModule returns "" for null).
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ksp/missions/dynamic_module.hpp"
#include "ksp/missions/node_library.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {
class PartModule;
}

namespace ksp::Expansions::Missions::Adjusters {

// AdjusterPartModuleBase.cs:10
class AdjusterPartModuleBase : public DynamicModule {
public:
    // :12 / :22 / :32 (nested enums)
    enum class FailureDeploymentState { CurrentState = 0, Deployed = 1, Retracted = 2 };
    enum class FailureActivationState { CurrentState = 0, On = 1, Off = 2 };
    enum class FailureOpenState { CurrentState = 0, Open = 1, Closed = 2 };

    AdjusterPartModuleBase(std::string typeName);                 // :84
    AdjusterPartModuleBase(std::string typeName, MENode* node);   // :90
    bool disableKSPFields = false;                                // :50
    bool disableKSPActions = false;                               // :52
    bool disableKSPEvents = false;                                // :54
    std::string AdjusterState = "#autoLOC_8100276";               // :57 [KSPField]
    PartModule* adjustedModule = nullptr;                         // :59
    Guid adjusterID;                                              // :61
    bool canBeRepaired = false;                                   // :64
    const std::string& ClassName() const { return className; }    // :76
    int ClassID() const { return classID; }                       // :78
    std::string guiName = "#autoLOC_8000002";                     // :48 (protected in C#)

    void Setup(PartModule* newPartModule);                        // :107
    void CleanUp();                                               // :124
    void RemoveAdjuster();                                        // :138
    void UpdateStatusMessage(const std::string& newStatusMessage);  // :154
    std::vector<std::string> GetPartModulesThatCanBeAffected();   // :159
    virtual std::string GetTargetPartModule() { return ""; }      // :178 (typeof(...).Name; "" = null)
    virtual std::vector<std::string> GetPartModuleTargetList() { return {}; }  // :183
    virtual void Activate() {}                                    // :188
    virtual void Deactivate() {}                                  // :192
    virtual void OnPartModuleAdjusterListModified() {}            // :196
    std::string GetDisplayName() override;                        // :200
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;  // :205
    static std::vector<std::unique_ptr<AdjusterPartModuleBase>> CreateModuleAdjusterList(const ConfigNode& node);  // :214
    static std::unique_ptr<AdjusterPartModuleBase> CreateInstanceOfModuleAdjuster(const ConfigNode& node);         // :232
    static std::unique_ptr<AdjusterPartModuleBase> CreateInstanceOfModuleAdjuster(const std::string& className);   // :244
    void Save(ConfigNode& node) const override;                   // :256
    void Load(const ConfigNode& node) override;                   // :264

private:
    void ModularSetup();                                          // :99
    std::string className;                                        // :70
    int classID = 0;                                              // :73
    bool modularSetupDone = false;                                // events / fields != null
};

// ---- abstract bases (AdjusterXBase.cs) ---------------------------------------------------------------------------------------
#define KSP_ADJUSTER_BASE(Name, Target)                                                                                          \
    class Name : public AdjusterPartModuleBase {                                                                                 \
    public:                                                                                                                      \
        explicit Name(std::string typeName) : AdjusterPartModuleBase(std::move(typeName)) {}                                     \
        Name(std::string typeName, MENode* node) : AdjusterPartModuleBase(std::move(typeName), node) {}                          \
        std::string GetTargetPartModule() override { return Target; }

// AdjusterActiveRadiatorBase.cs:7
KSP_ADJUSTER_BASE(AdjusterActiveRadiatorBase, "ModuleActiveRadiator")
    virtual bool IsBlockingCooling() { return false; }                                       // :22
    virtual double ApplyMaxEnergyTransferAdjustment(double maxEnergyTransfer) { return maxEnergyTransfer; }  // :27
};
// AdjusterAlternatorBase.cs:6
KSP_ADJUSTER_BASE(AdjusterAlternatorBase, "ModuleAlternator")
    virtual float ApplyOutputAdjustment(float output) { return output; }                     // :21
};
// AdjusterAnchoredDecouplerBase.cs:6
KSP_ADJUSTER_BASE(AdjusterAnchoredDecouplerBase, "ModuleAnchoredDecoupler")
    virtual bool IsBlockingDecouple() { return false; }                                      // :21
};
// AdjusterCargoBayBase.cs:6
KSP_ADJUSTER_BASE(AdjusterCargoBayBase, "ModuleCargoBay")
    virtual bool IsCargoBayStuck() { return false; }                                         // :21
};
// AdjusterControlSurfaceBase.cs:6
KSP_ADJUSTER_BASE(AdjusterControlSurfaceBase, "ModuleControlSurface")
    virtual float ApplyActuatorSpeedAdjustment(float actuatorSpeed) { return actuatorSpeed; }  // :21
};
// AdjusterDataTransmitterBase.cs:6
KSP_ADJUSTER_BASE(AdjusterDataTransmitterBase, "ModuleDataTransmitter")
    virtual double ApplyPowerAdjustment(double power) { return power; }                      // :21
    virtual bool IsDataTransmitterBroken() { return false; }                                 // :26
};
// AdjusterDecoupleBase.cs:6
KSP_ADJUSTER_BASE(AdjusterDecoupleBase, "ModuleDecouple")
    virtual bool IsBlockingDecouple() { return false; }                                      // :21
};
// AdjusterDeployablePartBase.cs:6
KSP_ADJUSTER_BASE(AdjusterDeployablePartBase, "ModuleDeployablePart")
    virtual bool IsDeployablePartStuck() { return false; }                                   // :21
};
// AdjusterDeployableSolarPanelBase.cs:6
KSP_ADJUSTER_BASE(AdjusterDeployableSolarPanelBase, "ModuleDeployableSolarPanel")
    virtual float ApplyEfficiencyAdjustment(float efficiency) { return efficiency; }         // :21
};
// AdjusterDockingNodeBase.cs:6
KSP_ADJUSTER_BASE(AdjusterDockingNodeBase, "ModuleDockingNode")
    virtual bool IsBlockingUndock() { return false; }                                        // :21
};
// AdjusterEnginesBase.cs:5
KSP_ADJUSTER_BASE(AdjusterEnginesBase, "ModuleEngines")
    virtual float ApplyThrottleAdjustment(float throttle) { return throttle; }               // :20
    virtual bool IsEngineDead() { return false; }                                            // :25
};
// AdjusterFuelLineBase.cs:6
KSP_ADJUSTER_BASE(AdjusterFuelLineBase, "CModuleFuelLine")
};
// AdjusterGeneratorBase.cs:6
KSP_ADJUSTER_BASE(AdjusterGeneratorBase, "ModuleGenerator")
    virtual float ApplyEfficiencyAdjustment(float efficiency) { return efficiency; }         // :21
};
// AdjusterGimbalBase.cs:6
KSP_ADJUSTER_BASE(AdjusterGimbalBase, "ModuleGimbal")
    virtual Vector3f ApplyControlAdjustment(Vector3f control) { return control; }            // :21
};
// AdjusterGPSBase.cs:6
KSP_ADJUSTER_BASE(AdjusterGPSBase, "ModuleGPS")
};
// AdjusterGrappleNodeBase.cs:6
KSP_ADJUSTER_BASE(AdjusterGrappleNodeBase, "ModuleGrappleNode")
    virtual bool IsBlockingGrappleRelease() { return false; }                                // :21
    virtual bool IsBlockingGrappleGrab() { return false; }                                   // :26
};
// AdjusterKerbNetAccessBase.cs:6
KSP_ADJUSTER_BASE(AdjusterKerbNetAccessBase, "ModuleKerbNetAccess")
};
// AdjusterLiftingSurfaceBase.cs:6
KSP_ADJUSTER_BASE(AdjusterLiftingSurfaceBase, "ModuleLiftingSurface")
    virtual Vector3f ApplyLiftForceAdjustment(Vector3f liftForce) { return liftForce; }      // :21
};
// AdjusterLightBase.cs:6
KSP_ADJUSTER_BASE(AdjusterLightBase, "ModuleLight")
    virtual float ApplyIntensityAdjustment(float intensity) { return intensity; }            // :21
};
// AdjusterProbeControlPointBase.cs:6
KSP_ADJUSTER_BASE(AdjusterProbeControlPointBase, "ModuleProbeControlPoint")
    virtual bool IsProbeControlPointBroken() { return false; }                               // :21
};
// AdjusterRCSBase.cs:6
KSP_ADJUSTER_BASE(AdjusterRCSBase, "ModuleRCS")
    virtual Vector3f ApplyInputRotationAdjustment(Vector3f inputRotation) { return inputRotation; }  // :21
    virtual Vector3f ApplyInputLinearAdjustment(Vector3f inputLinear) { return inputLinear; }        // :26
    virtual bool IsRCSBroken() { return false; }                                             // :31
};
// AdjusterReactionWheelBase.cs:6
KSP_ADJUSTER_BASE(AdjusterReactionWheelBase, "ModuleReactionWheel")
    virtual Vector3f ApplyTorqueAdjustment(Vector3f torque) { return torque; }               // :21
    virtual bool IsReactionWheelBroken() { return false; }                                   // :26
};
// AdjusterResourceIntakeBase.cs:6
KSP_ADJUSTER_BASE(AdjusterResourceIntakeBase, "ModuleResourceIntake")
};
// AdjusterSASBase.cs:6
KSP_ADJUSTER_BASE(AdjusterSASBase, "ModuleSAS")
    virtual int ApplySASServiceLevelAdjustment(int sasServiceLevel) { return sasServiceLevel; }  // :21
};
// AdjusterScienceExperimentBase.cs:6
KSP_ADJUSTER_BASE(AdjusterScienceExperimentBase, "ModuleScienceExperiment")
};
// AdjusterToggleCrossfeedBase.cs:6
KSP_ADJUSTER_BASE(AdjusterToggleCrossfeedBase, "ModuleToggleCrossfeed")
};
// AdjusterWheelBrakesBase.cs:6
KSP_ADJUSTER_BASE(AdjusterWheelBrakesBase, "ModuleWheelBrakes")
    virtual float ApplyTorqueAdjustment(float torque) { return torque; }                     // :21
};
// AdjusterWheelDeploymentBase.cs:6
KSP_ADJUSTER_BASE(AdjusterWheelDeploymentBase, "ModuleWheelDeployment")
};
// AdjusterWheelMotorBase.cs:6
KSP_ADJUSTER_BASE(AdjusterWheelMotorBase, "ModuleWheelMotor")
    virtual float ApplyTorqueAdjustment(float torque) { return torque; }                     // :21
};
#undef KSP_ADJUSTER_BASE

// ---- concrete adjusters / failures -------------------------------------------------------------------------------------------
// Each class: X() (the parameterless constructor), X(MENode*) (the one Activator.CreateInstance(type, node / null) uses), the overrides.
#define KSP_ADJUSTER(Name, Base)                                                                                                 \
    class Name : public DynamicModuleT<Name, Base> {                                                                             \
    public:                                                                                                                      \
        static constexpr const char* kTypeName = #Name;                                                                          \
        Name();                                                                                                                  \
        explicit Name(MENode* node);

KSP_ADJUSTER(AdjusterActiveRadiatorMaxEnergyTransfer, AdjusterActiveRadiatorBase)
    float radiatorMaxEnergyTransferMultiplier = 10.0f;
    void Activate() override;
    double ApplyMaxEnergyTransferAdjustment(double maxEnergyTransfer) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterAlternatorOutput, AdjusterAlternatorBase)
    float alternatorOutputMultiplier = 10.0f;
    void Activate() override;
    float ApplyOutputAdjustment(float output) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterControlSurfaceActuatorSpeed, AdjusterControlSurfaceBase)
    float actuatorSpeedMultiplier = 100.0f;
    void Activate() override;
    float ApplyActuatorSpeedAdjustment(float actuatorSpeed) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterDataTransmitterPower, AdjusterDataTransmitterBase)
    float transmitterPowerMultiplier = 10.0f;
    void Activate() override;
    double ApplyPowerAdjustment(double power) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
    void PowerValueChanged();
private:
    void UpdatePowerValueDisplay();
};
KSP_ADJUSTER(AdjusterDeployableSolarPanelEfficiency, AdjusterDeployableSolarPanelBase)
    float solarPanelEfficiencyMultiplier = 10.0f;
    void Activate() override;
    float ApplyEfficiencyAdjustment(float efficiency) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterEnginesThrottle, AdjusterEnginesBase)
    float engineThrottleMultiplier = 10.0f;
    void Activate() override;
    float ApplyThrottleAdjustment(float throttle) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterGeneratorEfficiency, AdjusterGeneratorBase)
    float generatorOutputMultiplier = 10.0f;
    void Activate() override;
    float ApplyEfficiencyAdjustment(float efficiency) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterGimbalLimitControl, AdjusterGimbalBase)
    float pitchControlMultiplier = 10.0f;
    float rollControlMultiplier = 10.0f;
    float yawControlMultiplier = 10.0f;
    void Activate() override;
    Vector3f ApplyControlAdjustment(Vector3f control) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterKerbNetAccessDisabled, AdjusterKerbNetAccessBase)
    void Activate() override;
};
KSP_ADJUSTER(AdjusterLiftingSurfaceLiftForce, AdjusterLiftingSurfaceBase)
    float liftForceMultiplier = 10.0f;
    void Activate() override;
    Vector3f ApplyLiftForceAdjustment(Vector3f liftForce) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterLightIntensity, AdjusterLightBase)
    float lightIntensityMultiplier = 10.0f;
    void Activate() override;
    float ApplyIntensityAdjustment(float intensity) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterReactionWheelTorque, AdjusterReactionWheelBase)
    float pitchTorqueMultiplier = 10.0f;
    float rollTorqueMultiplier = 10.0f;
    float yawTorqueMultiplier = 10.0f;
    void Activate() override;
    Vector3f ApplyTorqueAdjustment(Vector3f torque) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterSASServiceLevel, AdjusterSASBase)
    int sasServiceLevel = 0;
    void Activate() override;
    int ApplySASServiceLevelAdjustment(int oldSASServiceLevel) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterWheelBrakesTorque, AdjusterWheelBrakesBase)
    float brakeTorqueMultiplier = 10.0f;
    void Activate() override;
    float ApplyTorqueAdjustment(float torque) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(AdjusterWheelMotorTorque, AdjusterWheelMotorBase)
    float motorTorqueMultiplier = 10.0f;
    void Activate() override;
    float ApplyTorqueAdjustment(float torque) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};

// Failure*.cs
KSP_ADJUSTER(FailureActiveRadiatorStuck, AdjusterActiveRadiatorBase)
    FailureActivationState stuckState = FailureActivationState::CurrentState;
    void Activate() override;
    bool IsBlockingCooling() override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(FailureAnchoredDecouplerCantDecouple, AdjusterAnchoredDecouplerBase)
    void Activate() override;
    bool IsBlockingDecouple() override;
};
KSP_ADJUSTER(FailureCargoBayStuck, AdjusterCargoBayBase)
    FailureOpenState stuckState = FailureOpenState::CurrentState;
    void Activate() override;
    void Deactivate() override;
    bool IsCargoBayStuck() override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(FailureControlSurfaceDisableControl, AdjusterControlSurfaceBase)
    void Activate() override;
    float ApplyActuatorSpeedAdjustment(float actuatorSpeed) override;
};
KSP_ADJUSTER(FailureCrossfeedStuck, AdjusterToggleCrossfeedBase)
    FailureActivationState stuckState = FailureActivationState::CurrentState;
    void Activate() override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(FailureDataTransmitterBroken, AdjusterDataTransmitterBase)
    void Activate() override;
    bool IsDataTransmitterBroken() override;
};
KSP_ADJUSTER(FailureDecoupleCantDecouple, AdjusterDecoupleBase)
    void Activate() override;
    bool IsBlockingDecouple() override;
};
KSP_ADJUSTER(FailureDeployablePartStuck, AdjusterDeployablePartBase)
    FailureDeploymentState stuckState = FailureDeploymentState::CurrentState;
    void Activate() override;
    bool IsDeployablePartStuck() override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(FailureDockingNodeCantUndock, AdjusterDockingNodeBase)
    void Activate() override;
    bool IsBlockingUndock() override;
};
KSP_ADJUSTER(FailureEnginesDeadEngine, AdjusterEnginesBase)
    void Activate() override;
    bool IsEngineDead() override;
};
KSP_ADJUSTER(FailureFuelLineBlocked, AdjusterFuelLineBase)
    void Activate() override;
    void Deactivate() override;
};
KSP_ADJUSTER(FailureGPSNoDisplay, AdjusterGPSBase)
    void Activate() override;
};
KSP_ADJUSTER(FailureGrappleNodeCantGrapple, AdjusterGrappleNodeBase)
    void Activate() override;
    bool IsBlockingGrappleGrab() override;
};
KSP_ADJUSTER(FailureGrappleNodeCantRelease, AdjusterGrappleNodeBase)
    void Activate() override;
    void Deactivate() override;
    bool IsBlockingGrappleRelease() override;
};
KSP_ADJUSTER(FailureLightFlickering, AdjusterLightBase)
    float maximumFlickerTime = 200.0f;
    void Activate() override;
    float ApplyIntensityAdjustment(float intensity) override;
    std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
protected:
    void UpdateFlickeringData();
    float currentIncrement = -0.1f;
    float gradient = -0.1f;
};
KSP_ADJUSTER(FailureLightStuck, AdjusterLightBase)
    FailureActivationState stuckState = FailureActivationState::CurrentState;
    void Activate() override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(FailureProbeControlPointBroken, AdjusterProbeControlPointBase)
    bool IsProbeControlPointBroken() override;
};
KSP_ADJUSTER(FailureRCSFireRandomly, AdjusterRCSBase)
    void Activate() override;
    Vector3f ApplyInputRotationAdjustment(Vector3f inputRotation) override;
    Vector3f ApplyInputLinearAdjustment(Vector3f inputLinear) override;
};
KSP_ADJUSTER(FailureRCSStopWorking, AdjusterRCSBase)
    void Activate() override;
    bool IsRCSBroken() override;
};
KSP_ADJUSTER(FailureReactionWheelBroken, AdjusterReactionWheelBase)
    void Activate() override;
    bool IsReactionWheelBroken() override;
};
KSP_ADJUSTER(FailureReactionWheelStuck, AdjusterReactionWheelBase)
    void Activate() override;
    Vector3f ApplyTorqueAdjustment(Vector3f torque) override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
protected:
    Vector3f stuckTorque = Vector3f::zero();
    bool loadedFromConfigNode = false;
};
KSP_ADJUSTER(FailureResourceIntakeStuck, AdjusterResourceIntakeBase)
    FailureActivationState stuckState = FailureActivationState::CurrentState;
    void Activate() override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
KSP_ADJUSTER(FailureScienceExperimentDisable, AdjusterScienceExperimentBase)
    void Activate() override;
};
KSP_ADJUSTER(FailureWheelDeploymentStuck, AdjusterWheelDeploymentBase)
    FailureDeploymentState stuckState = FailureDeploymentState::CurrentState;
    void Activate() override;
    void Save(ConfigNode& node) const override;
    void Load(const ConfigNode& node) override;
};
#undef KSP_ADJUSTER

// MissionsUtils.cs:449 InitialiseAdjusterTypes: the two tables (insertion ordered) built from the adjusters registered below
struct AdjusterTypeTables {
    std::vector<std::pair<std::string, std::vector<std::string>>> adjusterTypesSupportedByPartModule;  // part module name -> adjuster names
    std::vector<std::pair<std::string, std::vector<std::string>>> partModuleTypesSupportedByAdjuster;  // adjuster name -> part module names
};
AdjusterTypeTables& GetAdjusterTypeTables();
void InitialiseAdjusterTypes();  // MissionsUtils.cs:449
void RegisterAdjusters();        // registers the 39 concrete types (and the abstract bases' names for IsSubclassOf)

}  // namespace ksp::Expansions::Missions::Adjusters

// ---- enum metadata ---------------------------------------------------------------------------------------------------------
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Adjusters::AdjusterPartModuleBase::FailureDeploymentState> {
    using E = ksp::Expansions::Missions::Adjusters::AdjusterPartModuleBase::FailureDeploymentState;
    static constexpr bool flags = false;
    // AdjusterPartModuleBase.cs:12
    static constexpr EnumMember<E> members[] = {{E::CurrentState, "CurrentState", "#autoLOC_8005002"},
                                                {E::Deployed, "Deployed", "#autoLOC_6002269"},
                                                {E::Retracted, "Retracted", "#autoLOC_6001081"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Adjusters::AdjusterPartModuleBase::FailureActivationState> {
    using E = ksp::Expansions::Missions::Adjusters::AdjusterPartModuleBase::FailureActivationState;
    static constexpr bool flags = false;
    // AdjusterPartModuleBase.cs:22
    static constexpr EnumMember<E> members[] = {
        {E::CurrentState, "CurrentState", "#autoLOC_8005002"}, {E::On, "On", "#autoLOC_8005003"}, {E::Off, "Off", "#autoLOC_8005004"}};
};
template <>
struct ksp::dotnet::EnumTraits<ksp::Expansions::Missions::Adjusters::AdjusterPartModuleBase::FailureOpenState> {
    using E = ksp::Expansions::Missions::Adjusters::AdjusterPartModuleBase::FailureOpenState;
    static constexpr bool flags = false;
    // AdjusterPartModuleBase.cs:32
    static constexpr EnumMember<E> members[] = {
        {E::CurrentState, "CurrentState", "#autoLOC_8005002"}, {E::Open, "Open", "#autoLOC_8005009"}, {E::Closed, "Closed", "#autoLOC_8005416"}};
};
