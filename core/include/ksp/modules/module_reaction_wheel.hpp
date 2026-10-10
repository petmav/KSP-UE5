// WP-24: ModuleReactionWheel.cs (numeric and state side).
//
// Ported: the [KSPField] schema (module_schema.inc), OnAwake (consumedResources), OnStart (PAW field / event / action state), OnLoad / OnSave
// (WheelState), the State property (stateString), FixedUpdate / ActiveUpdate (control input mapping per VesselActuatorMode, the
// authority limiter, the torque adjusters, the Lerp'ed inputVector, inputSum, the resource draw through resHandler.UpdateModuleResourceInputs,
// the torque handed to Part.AddTorque), the actions / events (CycleAction, Activate, Deactivate, Toggle, OnToggle), GetPotentialTorque,
// GetAppliedTorque.
//
// Float policy: MonoFloat (the game).
//
// Seams: Part.AddTorque (physics) is ReactionWheelEnvironment::addTorque and the recorded lastTorque; FlightInputHandler.state.GetPYR() of the
// active vessel is ReactionWheelEnvironment::activeVesselPYR; the Missions adjusters (AdjusterReactionWheelBase) are IReactionWheelAdjuster
// objects the host adds. Not ported: GetInfo / GetModuleDisplayName (PAW / part-info text: PartModuleUtil.PrintResourceRequirements and
// the "0.0###" float format are not modelled, as in ModuleResourceHandler), CacheLocalStrings (the strings are looked up where used).
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "ksp/modules/control_types.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/vessel/vessel_types.hpp"  // VesselActuatorMode

namespace ksp {

class Part;
class PartResourceDefinition;
class FlightCtrlState;
class Vessel;

class ModuleReactionWheel;

// Expansions.Missions.Adjusters.AdjusterReactionWheelBase (the two members ModuleReactionWheel calls)
class IReactionWheelAdjuster {
public:
    virtual ~IReactionWheelAdjuster() = default;
    virtual bool IsReactionWheelBroken() = 0;
    virtual Vector3f ApplyTorqueAdjustment(const Vector3f& torque) = 0;
};

struct ReactionWheelEnvironment {
    // FlightInputHandler.state.GetPYR() (pitch, roll, yaw of the pilot input, the active vessel only). Unset: zero.
    static inline std::function<Vector3f()> activeVesselPYR;
    // Part.AddTorque(world torque). Unset: only ModuleReactionWheel::lastTorque records it.
    static inline std::function<void(ModuleReactionWheel&, const Vector3f&)> addTorque;
    static void Reset() {
        activeVesselPYR = nullptr;
        addTorque = nullptr;
    }
};

class ModuleReactionWheel : public PartModule, public ITorqueProvider {
public:
    enum class WheelState { Active, Disabled, Broken };  // ModuleReactionWheel.cs:10

    explicit ModuleReactionWheel(const ModuleClassInfo& cls) : PartModule(cls) {}

    // ---- state that is not a [KSPField] (ModuleReactionWheel.cs:30-60) ----
    WheelState wheelState = WheelState::Active;
    bool operational = true;
    Vector3f inputVector{0.0f, 0.0f, 0.0f};
    float inputSum = 0.0f;
    std::vector<const PartResourceDefinition*> consumedResources;
    // what the last ActiveUpdate asked Part.AddTorque for (valid when torqueApplied)
    Vector3f lastTorque{};
    bool torqueApplied = false;

    WheelState State() const { return wheelState; }          // ModuleReactionWheel.cs:73
    void SetState(WheelState value);                          // ModuleReactionWheel.cs:73 (setter: stateString = displayDescription)
    static const char* WheelStateName(WheelState s);          // Enum.ToString
    const std::vector<const PartResourceDefinition*>& GetConsumedResources() const { return consumedResources; }

    // ---- lifecycle ----
    void OnAwake() override;
    void OnStart(StartState state) override;
    void OnLoad(const ConfigNode& node) override;
    void OnSave(ConfigNode& node) override;
    void CopySerializedFrom(const PartModule& from) override;
    ITorqueProvider* AsTorqueProvider() override { return this; }

    // ---- maths ----
    void FixedUpdate();                                       // :158 (HighLogic.LoadedSceneIsFlight && part.started && moduleIsEnabled ...)
    Vector3f GetAppliedTorque() const { return inputVectorTemp_; }
    void CycleAction(KSPActionParam& param);
    void Activate(KSPActionParam& param);
    void Deactivate(KSPActionParam& param);
    void Toggle(KSPActionParam& param);
    void OnToggle();
    void GetPotentialTorque(Vector3f& pos, Vector3f& neg) override;

    void AddAdjuster(IReactionWheelAdjuster* a) { adjusterCache_.push_back(a); }   // OnModuleAdjusterAdded
    void RemoveAdjuster(IReactionWheelAdjuster* a);                                  // OnModuleAdjusterRemoved
    bool IsAdjusterBreakingReactionWheel();
    Vector3f ApplyTorqueAdjustments(Vector3f torque);

private:
    Vector3f cmdInput_{};
    Vector3f inputVectorTemp_{};
    std::vector<IReactionWheelAdjuster*> adjusterCache_;
    void ActiveUpdate(VesselActuatorMode mode);
    Vessel* VesselPtr() const;
};

namespace detail {
void RegisterControlModules();  // module_command.cpp / module_reaction_wheel.cpp: ModuleCommand, ModuleSAS, ModuleRemoteController, ModuleReactionWheel
}

}  // namespace ksp
