// WP-23: ModuleEngines.cs, ModuleEnginesFX.cs, MultiModeEngine.cs (numeric and data side).
//
// Ported: every [KSPField] (generated schema, WP-06 FieldStore), OnAwake / OnLoad / OnStart (staging icon default, event
// and field activity flags, propellant setup, thrust transform multipliers), propellant setup (SetupPropellant,
// UpdatePropellantStatus), the thrust / Isp / fuel-flow maths (MaxThrustOutputAtm / Vac, GetEngineThrust, getExhaustVelocity,
// getFuelFlow, GetThrottlingMult, ModifyFlow), the throttle and response model (UpdateThrottle, RequiredPropellantMass,
// CalculateThrust, ThrustUpdate, FixedUpdate), flameout and ignition logic (Flameout, UnFlameout, CheckDeprived, RequestPropellant
// through Part.RequestResource of WP-15, CanStart), Activate / Shutdown / the actions, IStageable behaviour (OnActive, IsStageable),
// the IThrustProvider / IEngineStatus interfaces, CenterOfThrustQuery and MultiModeEngine's mode switching.
//
// Float policy: the multi-operation float32 expressions of the thrust maths are templates over aero::MonoFloat (the game: the
// production instantiation every virtual method calls) and aero::NetFloat (the .NET Framework 4.8 harness: only the
// golden gate `golden.engine_thrust` instantiates it). Single operations (isp * g, thrustPercentage * 0.01f) round once in
// both runtimes.
//
// Not ported (left to WP-20 / Jolt / the model reader / UI): Part.AddForceAtPosition (ThrustUpdate returns the forces as
// EngineForce records; the physics host applies them), Part.AddThermalFlux (ThrustUpdate returns the kilowatts), the world positions of
// the thrust transforms (ModuleEngines.thrustTransforms are PartTransform records a model reader fills through
// ModuleEngines::findModelTransforms; without one the list is empty, as for a part with no model), FXGroup / AudioSource /
// ParticleSystem work (SetupFXGroups, AutoPlaceFXGroup, HijackFX, InitializeFX, FXUpdate effects, the Play* / Deactivate*
// FX methods, EngineExhaustDamage raycasts, ParticlesStop), the stage-icon propellant gauges (UpdatePropellantGauge,
// OnStageIconDestroy), PAW / UI-control callbacks (UI_Control.onFieldChanged of thrustPercentage, throttleModeChanged's
// SetSceneVisibility, UpdateIncludeDVUI, GetDrawModulePanelCallback), GetInfo / GetInfoThrust / GetPrimaryField / GetModuleDisplayName text,
// ScreenMessages, the Missions adjuster plumbing (AdjusterEnginesBase: ApplyThrottleAdjustments is the identity, IsEngineDead is
// false), OnPause / OnUnpause (audio), OnInventoryModeDisable, CheckTransformsUnderwater's world altitude (an input: see
// EngineFlightInput) and CacheLocalStrings. MultiModeEngine: only the audio-free logic is omitted: all of it is ported
// (GameEvents included).
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ksp/aero/float_eval.hpp"
#include "ksp/engines/engine_interfaces.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
struct PartTransform;
class PartResourceDefinition;
class FlightCtrlState;

// A thrust transform of the model (Transform.position / Transform.forward in world space).
struct ThrustTransform {
    Vector3f position{};
    Vector3f forward{0.0f, 0.0f, 1.0f};
};

// What ThrustUpdate (ModuleEngines.cs:1540) asks the physics engine to do instead of calling Part.AddForceAtPosition / AddThermalFlux.
struct EngineForce {
    Vector3f force{};
    Vector3f position{};
};
struct EngineThrustResult {
    std::vector<EngineForce> forces;  // `-transform.forward * finalThrust * thrustTransformMultipliers[i]` at transform.position
    bool hasThermalFlux = false;
    double thermalFluxKilowatts = 0.0;  // Part.AddThermalFlux(kilowatts)
    bool exhaustDamage = false;         // EngineExhaustDamage() was reached (the raycast damage is WP-20 / WP-22's)
};

// Scene inputs of the flight methods that C# reads from the Unity scene.
struct EngineFlightInput {
    bool flightReady = true;                // FlightGlobals.ready
    bool underwater = false;                // CheckTransformsUnderwater() (needs the thrust transforms' altitude)
    bool oceanBody = false;                 // vessel.mainBody.ocean
    float fixedDeltaTime = 0.02f;           // TimeWarp.fixedDeltaTime
};

class ModuleEngines : public PartModule, public IEngineStatus, public IThrustProvider, public IModuleInfo {
public:
    explicit ModuleEngines(const ModuleClassInfo& cls) : PartModule(cls) {}

    // ---- state that is not a [KSPField] (public / protected members of ModuleEngines.cs) ----
    std::vector<Propellant> propellants;     // :11 (Unity serializes the list: Instantiate copies it)
    float mixtureDensity = 0.0f;             // :15
    double mixtureDensityRecip = 0.0;        // :17
    double ratioSum = 0.0;                   // :19
    float minThrust = 0.0f;                  // :64
    float maxThrust = 215.0f;                // :66
    float maxFuelFlow = 20.0f;               // :68
    float minFuelFlow = 0.0f;                // :70
    float throttleMin = 0.0f;                // public float throttleMin (no [KSPField]: set by OnLoad)
    std::vector<PartTransform> thrustTransforms;      // :227 (model transforms; see findModelTransforms)
    std::vector<float> thrustTransformMultipliers;    // :229
    float resultingThrust = 0.0f;
    float flowMultiplier = 1.0f;
    float requestedMassFlow = 0.0f;
    float requestedThrottle = 0.0f;
    EngineType engineType = EngineType::Generic;
    float g = 9.80665f;
    std::vector<const PartResourceDefinition*> consumedResources;  // :336 (null entries for unknown propellants, as C#)

    // The model reader's hook: Part.FindModelTransforms(name) for a part (unset: the part has no such transforms).
    static inline std::function<std::vector<PartTransform>(Part&, const std::string&)> findModelTransforms;

    // ---- lifecycle ----
    void OnAwake() override;                          // :1609
    void OnLoad(const ConfigNode& node) override;     // :598
    void OnStart(StartState state) override;          // :1651
    void OnActive() override;                         // :1724
    void OnFixedUpdate() override { FixedUpdate(); }
    void CopySerializedFrom(const PartModule& from) override;  // Unity Instantiate: the public non-KSPField members
    bool IsStageable() override { return true; }     // :2137
    void Flameout(const std::string& message, bool statusOnly = false, bool showFX = true);  // :497 (virtual in C#)
    void UnFlameout(bool showFX = true);              // :517
    void Activate();                                  // :740 [KSPEvent]
    void Shutdown();                                  // :788 [KSPEvent]
    void ToggleIncludeinDV();                         // :822 [KSPEvent]
    void OnAction(KSPActionParam&);                   // :839 [KSPAction]
    void ShutdownAction(KSPActionParam&);             // :852
    void ActivateAction(KSPActionParam&);             // :858
    void ToggleThrottle(KSPActionParam&);             // :864
    void thrustPercentChanged();                      // :870 (the body: the UI callback wiring is UI)
    void SetupPropellant();                           // :390
    void UpdatePropellantStatus(bool doGauge = true); // :421 (the gauge is UI)
    bool CheckDeprived(double requiredPropellant, std::string& propName);  // :529
    double RequestPropellant(double mass);            // :550

    // ---- thrust maths (:1833-2060) ----
    // Template over the float evaluation policy (the file comment). The virtual wrappers below call the MonoFloat instantiation.
    template <class P>
    float GetEngineThrustT(float isp, float throttle) const;                   // :2045
    template <class P>
    float GetThrottlingMultT(float atm, float throttle) const;                 // :2050
    template <class P>
    float MaxThrustOutputAtmT(bool runningActive, bool useThrustLimiter, float atmPressure, double atmTemp, double atmDensity,
                              double machNumber) const;                         // :1833
    template <class P>
    float MaxThrustOutputVacT(bool useThrustLimiter) const;                     // :1887
    // :636-647 OnLoad: `thrust / (atmosphereCurve.Evaluate(0f) * g)`
    template <class P>
    static float FuelFlowFromThrust(float thrust, float ispVac, float g);
    // :390 SetupPropellant's mixtureDensity accumulation (`num += definition.density * propellant.ratio`)
    template <class P>
    void SetupPropellantT();

    virtual float MaxThrustOutputAtm(bool runningActive = false, bool useThrustLimiter = true, float atmPressure = 1.0f,
                                     double atmTemp = 310.0, double atmDensity = 1.225000023841858) const;  // :1833
    virtual float MaxThrustOutputVac(bool useThrustLimiter = true) const;                                  // :1887
    double MassFlow() const { return massFlow; }                                                           // :1913
    virtual float getMaxFuelFlow(const Propellant& p) const;                                               // :2029
    virtual float getFuelFlow(const Propellant& p, float fuelFlow) const;                                  // :2034
    float getExhaustVelocity(float isp) const;                                                             // :2040
    float GetEngineThrust(float isp, float throttle) const;                                                // :2045
    float GetThrottlingMult(float atm, float throttle) const;                                              // :2050
    virtual float ModifyFlow();                                                                            // :2055
    float ApplyThrottleAdjustments(float throttle) const { return throttle; }                              // :2158 (no Missions adjusters)
    bool IsEngineDead() const { return false; }                                                            // :2167
    virtual bool CanBeDetached() const { return !isThrusting; }                                            // :2186
    virtual bool CanBeOffset() const { return !isThrusting; }                                              // :2191
    virtual bool CanBeRotated() const { return !isThrusting; }                                             // :2196
    const std::vector<const PartResourceDefinition*>& GetConsumedResources() const { return consumedResources; }  // :1604

    // ---- flight (:1352-1600) ----
    virtual void UpdateThrottle(const EngineFlightInput& in);  // :1352
    double RequiredPropellantMass(float throttleAmount, const EngineFlightInput& in);  // :1454
    float CalculateThrust(const EngineFlightInput& in);        // :1478
    EngineThrustResult ThrustUpdate(const EngineFlightInput& in);  // :1540
    // :1587 FixedUpdate (the scene inputs come from `flightInput`, set by the physics host before each tick)
    void FixedUpdate();
    EngineFlightInput flightInput;
    EngineThrustResult lastThrust;  // what the last FixedUpdate asked of the physics engine
    bool warping = false;           // TimeWarping(): (rate > 1 && HIGH warp) || part.packed (:1530)

    // ---- IThrustProvider / IEngineStatus ----
    void OnCenterOfThrustQuery(CenterOfThrustQuery& qry) override;  // :2103
    float GetMaxThrust() override { return maxThrust; }              // :2122
    float GetCurrentThrust() override;                               // :2127
    EngineType GetEngineType() override { return engineType; }       // :2132
    bool isOperational() const override;                             // :90-100
    float normalizedOutput() const override;                         // :104
    float throttleSetting() const override;                          // :106
    std::string engineName() const override;                         // :108
    float normalizedThrustOutput() const;                            // :84
    bool getFlameoutState() const;                                   // :86
    bool getIgnitionState() const;                                   // :88

    // IModuleInfo (the texts are UI: GetModuleTitle is the localised class title)
    std::string GetModuleTitle() override { return "Engine"; }
    std::string GetInfo() override { return ""; }
    std::string GetPrimaryField() override { return ""; }
    IModuleInfo* AsModuleInfo() override { return this; }

    // Typed readers of the persistent flight fields
    bool EngineIgnited() const { return Get<bool>("EngineIgnited"); }
    bool flameout() const { return Get<bool>("flameout"); }
    float currentThrottle() const { return Get<float>("currentThrottle"); }
    float finalThrust() const { return Get<float>("finalThrust"); }
    float thrustPercentage() const { return Get<float>("thrustPercentage"); }
    float realIsp() const { return Get<float>("realIsp"); }

protected:
    double massFlow = 0.0;      // :314
    bool isThrusting = false;   // :23
    bool listenersSet = false;  // :316

    const FloatCurve& Curve(std::string_view name) const;  // a null KSPField curve acts as an empty curve
    Part* PartPtr() const;
    friend class MultiModeEngine;
};

// ModuleEnginesFX.cs
class ModuleEnginesFX : public ModuleEngines {
public:
    explicit ModuleEnginesFX(const ModuleClassInfo& cls) : ModuleEngines(cls) {}
    float engineSpool = 0.0f;                  // :34
    void OnAwake() override;                   // :162
    bool CanStart();                           // :132
    // ModuleEnginesFX.cs:59 FXUpdate: the engine spool value (the effects themselves are FX)
    void FXUpdate(float fixedDeltaTime);       // :59
};

// MultiModeEngine.cs
class MultiModeEngine : public PartModule, public IEngineStatus {
public:
    explicit MultiModeEngine(const ModuleClassInfo& cls) : PartModule(cls) {}
    ~MultiModeEngine() override;

    ModuleEnginesFX* PrimaryEngine() const { return primaryEngine_; }      // MultiModeEngine.cs:47
    ModuleEnginesFX* SecondaryEngine() const { return secondaryEngine_; }  // MultiModeEngine.cs:49

    void OnAwake() override;                       // :273
    void OnLoad(const ConfigNode& node) override;  // :283
    void OnStart(StartState state) override;       // :309
    void OnUpdate() override;                      // :380
    void OnAutoSwitch(MultiModeEngine* other);     // :89
    void onVesselModified(Vessel* v);              // :104
    void ModeAction(KSPActionParam&);              // :113 [KSPAction]
    void DisableAutoSwitch();                      // :119 [KSPEvent]
    void EnableAutoSwitch();                       // :127
    void ModeEvent();                              // :135
    void ToggleMode();                             // :141
    void ShutdownAction(KSPActionParam&);          // :154
    void ActivateAction(KSPActionParam&);          // :167
    void OnAction(KSPActionParam&);                // :180
    void SetPrimary(bool fireEvents);              // :203
    void SetSecondary(bool fireEvents);            // :238

    bool isOperational() const override;
    float normalizedOutput() const override;
    float throttleSetting() const override;
    std::string engineName() const override { return Get<std::string>("engineID"); }

    bool runningPrimary() const { return Get<bool>("runningPrimary"); }
    bool autoSwitch() const { return Get<bool>("autoSwitch"); }

private:
    ModuleEnginesFX* primaryEngine_ = nullptr;
    ModuleEnginesFX* secondaryEngine_ = nullptr;
    bool loadFailure_ = false;
    MultiModeEngine* leadModule_ = nullptr;
    bool registered_ = false;
    void ApplyAutoSwitchEvents();
};

namespace detail {
void RegisterEngineModules();  // registers ModuleEngines, ModuleEnginesFX, MultiModeEngine (module_engines.cpp)
}

}  // namespace ksp
