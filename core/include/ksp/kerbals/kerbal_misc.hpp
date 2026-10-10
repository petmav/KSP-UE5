// WP-31 small classes: APSkillExtensions.cs, KerbalMovementParameters.cs, ModuleExperienceManagement.cs.
#pragma once

#include <functional>
#include <string>

#include "ksp/kerbals/experience.hpp"
#include "ksp/modules/part_module.hpp"

namespace ksp {

class Vessel;

// APSkillExtensions.cs (extension methods on VesselAutopilot.AutopilotMode)
struct APSkillExtensions {
    // :6 (HighLogic.CurrentGame; vessel.VesselValues)
    static bool AvailableAtLevel(VesselAutopilot_AutopilotMode mode, Vessel& vessel);
    // :61 (obsolete overload)
    static bool AvailableAtLevel(VesselAutopilot_AutopilotMode mode, int skillLvl);
    // :69
    static int GetRequiredSkill(VesselAutopilot_AutopilotMode mode);
};

// KerbalMovementParameters.cs ([Serializable] data holder; values come from the EVA prefab, KerbalEVA.movementParameters)
struct KerbalMovementParameters {
    float walkSpeed = 0.0f;
    float runSpeed = 0.0f;
    float jumpForce = 0.0f;
    float strafeSpeed = 0.0f;
    float backwardsSpeed = 0.0f;
    float turnRate = 0.0f;
};

// ModuleExperienceManagement.cs
class ModuleExperienceManagement : public PartModule {
public:
    using PartModule::PartModule;

    // Inputs of FinalizeExperience that belong to other systems (unset: no CommNet, funds are not tracked)
    // `CommNetScenario.CommNetEnabled && (vessel.connection == null || vessel.connection.Signal == SignalStrength.None)`
    static inline std::function<bool(Vessel&)> commNetHasNoSignal;
    static inline std::function<bool(float)> fundingCanAfford;  // Funding.CanAfford (unset: true)
    static inline std::function<void(float)> fundingAddFunds;   // Funding.Instance.AddFunds(-cost, TransactionReasons.Vessels)
    static inline std::function<bool()> fundingPresent;         // Funding.Instance != null (unset: false)
    static inline std::function<void(const std::string&, float)> postScreenMessage;  // ScreenMessages (UI)
    bool enabled = true;                                        // MonoBehaviour.enabled

    void OnStart(StartState state) override;                    // :14
    std::string GetInfo() override;                             // :23
    void FinalizeExperience();                                  // :29 [KSPEvent]
    std::string GetModuleDisplayName() override;                // :71
    float costPerKerbal() { return Get<float>("costPerKerbal"); }  // :12 [KSPField]
};

void RegisterExperienceManagementModule();  // called by RegisterBuiltinModules

}  // namespace ksp
