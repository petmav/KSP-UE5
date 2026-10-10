// WP-23: ModuleRCS.cs, ModuleRCSFX.cs (numeric and data side).
//
// Ported: the [KSPField] schema, OnAwake / OnLoad (the synthesised PROPELLANT, SetupPropellant, maxFuelFlow), the thruster setup
// through the model reader hook, the control input mapping (Update: deadband, throttle-as-Z, the ReferenceTransform rotation),
// FixedUpdate's thrust maths per thruster (rotation / translation response, full thrust, precision lever), CalculateThrust
// (thrust curve, propellant request through Part.RequestResource, the exhaust velocity), RequestPropellant, GetLeverDistance,
// the propellant helpers (getMaxFuelFlow, UpdatePropellantStatus), GetPotentialTorque (ITorqueProvider's method), IsStageable
// and the staging texts' selection, OnActive, SetResource, ModuleRCSFX (no numeric difference: the FX overrides are FX).
//
// Float policy: MonoFloat (the game).
//
// Not ported: Part.AddForceAtPosition (the forces are returned as RCSForce records for the physics host), FX (SetupFX,
// UpdatePowerFX, DeactivateFX / DeactivatePowerFX apart from zeroing thrustForces), PAW text (GetInfo, UpdateToggles' guiName,
// GetModuleDisplayName), the stage-icon (OnStart's stackIcon.SetIcon, UpdatePropellantGauge, OnStageIconDestroy), the Missions
// adjuster plumbing (AdjusterRCSBase: the Apply*Adjustments are the identity, IsAdjusterBreakingRCS is false), thruster
// `gameObject.activeInHierarchy` (an input), FlightInputHandler.fetch.precisionMode (an input).
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "ksp/aero/float_eval.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
struct PartTransform;
class FlightCtrlState;
class PartResourceDefinition;

// A thruster Transform of the model with the axes FixedUpdate reads (world space).
struct RCSThruster {
    Vector3f position{};
    Vector3f up{0.0f, 1.0f, 0.0f};
    Vector3f forward{0.0f, 0.0f, 1.0f};
    bool activeInHierarchy = true;
};

struct RCSForce {
    Vector3f force{};
    Vector3f position{};
};

// Scene inputs of ModuleRCS.FixedUpdate (the Vessel, TimeWarp, FlightInputHandler)
struct RCSFlightInput {
    bool flightScene = true;          // HighLogic.LoadedSceneIsFlight
    bool highWarp = false;            // TimeWarp.CurrentRate > 1 && WarpMode == HIGH
    bool hasVessel = true;            // base.vessel != null
    bool rcsActionGroup = false;      // vessel.ActionGroups[KSPActionGroup.RCS]
    Vector3f currentCoM{};            // vessel.CurrentCoM
    float staticPressureAtm = 0.0f;   // (float)part.staticPressureAtm
    bool shieldedFromAirstream = false;
    bool precisionMode = true;        // FlightInputHandler.fetch.precisionMode
    float fixedDeltaTime = 0.02f;     // TimeWarp.fixedDeltaTime
};

class ModuleRCS : public PartModule {
public:
    explicit ModuleRCS(const ModuleClassInfo& cls) : PartModule(cls) {}

    // ---- state that is not a [KSPField] (ModuleRCS.cs public / private members) ----
    std::vector<RCSThruster> thrusterTransforms;
    std::vector<float> thrustForces;
    double G = 9.80665;
    Vector3f inputRot{}, inputLin{};
    bool usePrecision = true;
    bool rcs_active = false;
    bool flameout = false;
    double maxFuelFlow = 0.0;
    double flowMult = 1.0;
    double ispMult = 1.0;
    std::vector<Propellant> propellants;
    double mixtureDensity = 0.0;
    double mixtureDensityRecip = 0.0;
    std::vector<const PartResourceDefinition*> consumedResources;
    std::vector<RCSForce> lastForces;  // what the last FixedUpdate asks the physics engine to apply
    bool isOperating = false;

    static inline std::function<std::vector<RCSThruster>(Part&, const std::string&)> findModelTransforms;

    // ---- lifecycle ----
    void OnAwake() override;                       // :377
    void OnLoad(const ConfigNode& node) override;  // :419
    void OnStart(StartState state) override;       // :433
    void OnActive() override;                      // :468
    bool IsStageable() override;                   // :746
    std::string GetStagingEnableText() override;   // :755
    std::string GetStagingDisableText() override;  // :764
    void CopySerializedFrom(const PartModule& from) override;
    void ToggleToggles();                          // :199
    void ToggleAction(KSPActionParam& param);      // :206
    void SetResource(const std::string& resource); // :485

    // ---- maths ----
    // :211 Update: maps the control state to inputRot / inputLin (the vessel's reference rotation is an input)
    void Update(const FlightCtrlState& ctrlState, const Quaternionf& referenceRotation, bool precisionMode);
    float GetLeverDistanceOriginal(const Vector3f& thrusterPosition, const Vector3f& leverPivotPosition) const;  // :264
    float GetLeverDistance(const RCSThruster& trf, const Vector3f& axis, const Vector3f& leverPivotPosition) const;  // :269
    // :277 FixedUpdate: fills thrustForces / lastForces
    void FixedUpdate(const RCSFlightInput& in);
    float CalculateThrust(float totalForce, bool& success, float fixedDeltaTime);  // :566
    void SetupPropellant(const ConfigNode& node);                                   // :613
    void UpdatePropellantStatus();                                                  // :663
    double RequestPropellant(double mass);                                          // :702
    double getMaxFuelFlow(float Isp);                                               // :510
    double getMaxFuelFlow(const Propellant& p, float Isp);                          // :520
    void GetPotentialTorque(Vector3f& pos, Vector3f& neg, const Vector3f& currentCoM, bool precisionMode);  // :773
    void DeactivateFX();                                                            // :874 (thrustForces only)
    bool CanBeDetached() const { return !isOperating; }                             // :953
    bool CanBeOffset() const { return !isOperating; }                               // :958
    bool CanBeRotated() const { return !isOperating; }                              // :963
    const std::vector<const PartResourceDefinition*>& GetConsumedResources() const { return consumedResources; }  // :372

    // Part-side inputs of GetPotentialTorque (vessel.ActionGroups[RCS] mirrored into rcs_active by FixedUpdate)
    bool shieldedFromAirstream = false;  // part.ShieldedFromAirstream, refreshed by the host each tick

protected:
    const FloatCurve& Curve(std::string_view name) const;
    Part* PartPtr() const;
    float currentThrustForce_ = 0.0f;
    float totalThrustForce_ = 0.0f;
    double exhaustVel_ = 0.0;
    float ignitionThreshold_ = 0.1f;  // :180 private float
};

// ModuleRCSFX.cs: only FX overrides (SetupFX, UpdatePowerFX, DeactivatePowerFX, DeactivateFX, OnAwake's part.Effect, display name)
class ModuleRCSFX : public ModuleRCS {
public:
    explicit ModuleRCSFX(const ModuleClassInfo& cls) : ModuleRCS(cls) {}
};

namespace detail {
void RegisterRcsModules();  // module_rcs.cpp
}

}  // namespace ksp
