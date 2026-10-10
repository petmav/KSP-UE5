// WP-24: ModuleCommand.cs, ModuleSAS.cs, ModuleRemoteController.cs (numeric and state side).
//
// Ported: ModuleCommand's [KSPField] schema (module_schema.inc), OnAwake (consumedResources, Part.isControlSource = FULL on the prefab, the
// hibernation PAW state, the control point dictionary), OnStart / OnLoad (CONTROLPOINT nodes) / OnSave, the control source rules
// (UpdateControlSourceState: resource draw with the hibernation multiplier, minimumCrew, the tourist / inactive crew count, pilots,
// remoteControl / requiresTelemetry / requiresPilot, the CommNet signal, ModuleControlState and VesselControlState results),
// UpdateControlState (Part.isControlSource), IsHibernating, SignalRequired, the control points (ChangeControlPoint, SetControlPoint,
// GetControlPoint, ControlPointsExist), MakeReference, HibernateToggle; ModuleSAS (OnStart, GetValue, FixedUpdate: SASServiceLevel with the
// adjusters, the standalone toggle / state text and resource draw, the PartValues AutopilotSkill / AutopilotSASSkill registrations);
// ModuleRemoteController (Toggle).
//
// Seams (CommandEnvironment, each documented): CommNet (CommNetScenario.CommNetEnabled, the vessel connection) is WP-39's and arrives as
// hooks; FlightDriver.Pause; Part.SetReferenceTransform for a control point; the GameSettings control point visuals (UI only); the Unity
// Quaternion.Euler of a control point child transform; Vessel.RenameVessel; UIPartActionController.ItemListContains (decides only whether
// the crew texts are written to controlSrcStatusText).
//
// Not ported: ICommNetControlSource registration (Start / OnDestroy / OnNetworkInitialised: CommNetVessel.RegisterCommandSource) and
// UpdateNetwork (PAW text of the signal), the control point arrows (UpdateControlPointVisuals / DestroyCPArrows: rendering), GetInfo /
// GetModuleDisplayName (part-info text), CacheLocalStrings.
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/commnet/control_source.hpp"
#include "ksp/modules/control_types.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/util/collections.hpp"

namespace ksp {

class Vessel;
class PartResourceDefinition;
class ModuleCommand;

// What ModuleCommand reads from the vessel CommNetVessel (vessel.connection)
struct CommNetLinkState {
    bool isConnected = false;       // Connection.IsConnected
    double signalStrength = 0.0;    // Connection.SignalStrength
    double firstHopCost = 0.0;      // Connection.ControlPath.First.cost
};

struct CommandEnvironment {
    // CommNetScenario.CommNetEnabled && CommNetScenario.Instance != null. Unset: CommNet is off.
    static inline std::function<bool()> commNetEnabled;
    // vessel.connection: nullopt = null (no CommNetVessel). Unset: null.
    static inline std::function<std::optional<CommNetLinkState>(Vessel&)> connection;
    // FlightDriver.Pause. Unset: false.
    static inline std::function<bool()> flightPaused;
    // UIPartActionController.Instance != null && ItemListContains(part, false). Unset: false.
    static inline std::function<bool(PartModule&)> partInPAW;
    // Part.SetReferenceTransform(controlPoint.transform) in the flight scene. Unset: nothing.
    static inline std::function<void(ModuleCommand&, ControlPoint&)> setPartReferenceTransform;
    // Vessel.RenameVessel (opens the rename dialog). Unset: nothing.
    static inline std::function<void(Vessel&)> renameVessel;
    // GameSettings.CONTROLPOINT_VISUALS_ENABLED (GameSettings.cs:1095) / CONTROLPOINT_ARROWLENGTH (:1096). The defaults of a fresh settings file.
    static inline bool controlPointVisualsEnabled = false;
    static inline float controlPointArrowLength = 2.5f;
    // Quaternion.Euler(orientation) (native Unity). Unset: ModuleCommand::DefaultQuaternionEuler.
    static inline std::function<Quaternionf(const Vector3f&)> quaternionEuler;
    static void Reset();
};

class ModuleCommand : public PartModule, public CommNet::ICommNetControlSource {
public:
    enum class ModuleControlState { NotEnoughCrew, NotEnoughResources, PartialManned, NoControlPoint, TouristCrew, PartialProbe, Nominal };  // :12

    explicit ModuleCommand(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleCommand() override;  // drops the OnNetworkInitialized subscription of Start (Unity calls OnDestroy)

    // ---- state that is not a [KSPField] (ModuleCommand.cs) ----
    std::vector<const PartResourceDefinition*> consumedResources;
    float cpArrowLength = 2.0f;
    std::optional<std::vector<std::shared_ptr<ControlPoint>>> controlPointsList;                    // [SerializeField] private, filled by OnLoad
    std::optional<DictionaryValueList<std::string, std::shared_ptr<ControlPoint>>> controlPoints;  // null until OnAwake builds it
    bool showControlPointVisual = false;

    ModuleControlState ModuleState() const { return moduleState_; }          // :105
    bool IsCommCapable() const override { return commCapable_; }             // :417
    VesselControlState GetControlSourceState() const override { return localVesselControlState_; }  // :412
    // ICommNetControlSource (WP-39): `name` (the component name), UpdateNetwork (:345), Start (:319), OnDestroy (:332), OnNetworkInitialised (:341)
    std::string ControlSourceName() const override { return moduleName; }
    void UpdateNetwork() override;
    virtual void Start();
    virtual void OnDestroy();
    virtual void OnNetworkInitialised();
    bool networkInitialised() const { return networkInitialised_; }
    int crewCount() const { return crewCount_; }
    int totalCrewCount() const { return totalCrewCount_; }
    int pilots() const { return pilots_; }
    bool SignalRequired() const;                                             // :115
    bool IsHibernating() const;                                              // :128
    const std::string& ActiveControlPointName();                             // :152 (the KSPField activeControlPointName)
    ControlPoint* ActiveControlPoint() const { return activeControlPoint_.get(); }
    const std::string& ControlPointDisplayName() const { return controlPointDisplayName_; }
    const std::vector<const PartResourceDefinition*>& GetConsumedResources() const { return consumedResources; }  // :287

    // ---- lifecycle ----
    void OnAwake() override;
    void OnStart(StartState state) override;
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) override;
    void CopySerializedFrom(const PartModule& from) override;
    // FixedUpdate (:402): UpdateControlState
    void FixedUpdate();
    void UpdateControlState();                       // :406
    virtual VesselControlState UpdateControlSourceState();  // :420

    // ---- control points ----
    void ToggleControlPointVisual();                 // :157
    void ChangeControlPoint();                       // :230
    void SetControlPoint(std::string newControlPointName = "_default");  // :243
    ControlPoint* GetControlPoint(const std::string& name);              // :672
    bool ControlPointsExist() const;                 // :683
    static Quaternionf DefaultQuaternionEuler(const Vector3f& euler);

    // ---- actions / events ----
    void MakeReferenceToggle(KSPActionParam& act);   // :617
    virtual void MakeReference();                    // :625
    virtual void RenameVessel();                     // :631
    virtual void HibernateToggle(KSPActionParam& param);  // :639

protected:
    ModuleControlState moduleState_ = ModuleControlState::NotEnoughCrew;
    VesselControlState localVesselControlState_ = VesselControlState::None;
    bool commCapable_ = true;
    bool networkInitialised_ = false;
    bool networkSubscribed_ = false;
    int crewCount_ = 0;
    int totalCrewCount_ = 0;
    int pilots_ = 0;
    std::shared_ptr<ControlPoint> activeControlPoint_;
    std::string controlPointDisplayName_;
    void UpdateControlPointEvent();                  // :272
    Vessel* VesselPtr() const;
    std::optional<CommNetLinkState> Connection() const;
};

// ModuleSAS.cs
class ModuleSAS : public PartModule {
public:
    explicit ModuleSAS(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~ModuleSAS() override;

    int targetSASServiceLevel = 0;
    bool standaloneOperational = false;   // [SerializeField] private

    void OnStart(StartState state) override;
    void CopySerializedFrom(const PartModule& from) override;
    void FixedUpdate();
    int GetValue();                       // private int GetValue(): the AutopilotSkill / AutopilotSASSkill provider
    void AddAdjuster(std::function<int(int)> a) { adjusters_.push_back(std::move(a)); }  // AdjusterSASBase.ApplySASServiceLevelAdjustment
    int ApplySASServiceLevelAdjustments(int serviceLevel);
    ModuleCommand* CommandModule() const { return cmdModule_; }

private:
    ModuleCommand* cmdModule_ = nullptr;
    std::vector<std::function<int(int)>> adjusters_;
    Part* registeredOn_ = nullptr;
};

// ModuleRemoteController.cs
class ModuleRemoteController : public PartModule {
public:
    explicit ModuleRemoteController(const ModuleClassInfo& cls) : PartModule(cls) {}
    void OnAwake() override;
    void Toggle();   // :6
};

namespace detail {
void RegisterControlModules();  // module_command.cpp: ModuleCommand, ModuleSAS, ModuleRemoteController (+ ModuleReactionWheel)
}

}  // namespace ksp
