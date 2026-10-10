// WP-23: ModuleResourceIntake.cs (numeric and data side).
//
// Ported: the [KSPField] schema, OnAwake (default machCurve), OnStart (activity flags, resource definition, density, the occlusion
// node, the intake's resource), OnLoad, the events and actions (Activate, Deactivate, ToggleAction x2), FixedUpdate's intake
// area / speed maths (airSpeed, area * unitScalar * machCurve, atmosphere density, resource units, transfer into the part's
// resource through Part.TransferResource / RequestResource of WP-15) and its status logic.
//
// Float policy: the intake maths is double precision (the C# uses double for speed, area and density); the only float
// expressions are single operations, so both runtimes agree.
//
// Not ported: GetInfo / GetModuleDisplayName text (PAW), CacheLocalStrings. The vessel's atmosphere values and the intake
// Transform are inputs (IntakeFlightInput, `hasIntakeTransform` set by the model reader hook).
#pragma once

#include <functional>
#include <string>

#include "ksp/modules/part_module.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
struct AttachNode;
struct PartResource;
class PartResourceDefinition;

// What FixedUpdate reads from the Vessel / CelestialBody / intake Transform.
struct IntakeFlightInput {
    bool hasVessel = true;                     // base.vessel != null
    bool atmosphereContainsOxygen = true;      // vessel.mainBody.atmosphereContainsOxygen
    bool oceanBody = false;                    // vessel.mainBody.ocean
    double oceanDensity = 0.0;                 // vessel.mainBody.oceanDensity
    double staticPressurekPa = 0.0;            // vessel.staticPressurekPa
    Vector3f srf_vel_direction{};              // vessel.srf_vel_direction
    double srfSpeed = 0.0;                     // vessel.srfSpeed
    double mach = 0.0;                         // vessel.mach
    double atmDensity = 0.0;                   // vessel.atmDensity
    double intakeAltitude = 1.0;               // FlightGlobals.getAltitudeAtPos(intakeTransform.position, mainBody)
    Vector3f intakeForward{0.0f, 0.0f, 1.0f};  // intakeTransform.forward
    bool hasIntakeTransform = true;            // intakeTransform != null
    float fixedDeltaTime = 0.02f;              // TimeWarp.fixedDeltaTime
};

class ModuleResourceIntake : public PartModule {
public:
    explicit ModuleResourceIntake(const ModuleClassInfo& cls) : PartModule(cls) {}

    // ---- state that is not a [KSPField] (ModuleResourceIntake.cs public members) ----
    int32_t resourceId = 0;
    double resourceUnits = 0.0;
    const PartResourceDefinition* resourceDef = nullptr;
    double densityRecip = 0.0;
    PartResource* res = nullptr;
    AttachNode* node = nullptr;
    bool checkNode = false;
    bool hasIntakeTransform = false;  // `intakeTransform != null` after OnStart's FindModelTransform

    static inline std::function<bool(Part&, const std::string&)> findModelTransform;  // the model reader's hook

    void OnAwake() override;                       // :75
    void OnStart(StartState state) override;       // :85
    void OnLoad(const ConfigNode& node) override;  // :134
    void ToggleAction(KSPActionParam& param);      // :161 [KSPAction]
    void ToggleAction(KSPActionType action);       // :173
    void Deactivate();                             // :186 [KSPEvent]
    void Activate();                               // :194 [KSPEvent]
    // :201 FixedUpdate
    void FixedUpdate(const IntakeFlightInput& in);

private:
    Part* PartPtr() const;
};

namespace detail {
void RegisterIntakeModule();  // module_resource_intake.cpp
}

}  // namespace ksp
