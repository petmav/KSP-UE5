// Module behaviours WP-06 ports because the part-database gate observes them: the OnAwake / OnLoad logic of the stock
// modules that changes KSPField values or Part prefab data while the prefab is being compiled (PartLoader.ParsePart ->
// Part.AddModule -> PartModule.Awake -> PartModule.Load), and the IPartMassModifier / IPartCostModifier implementations
// (Part.GetModuleMass / GetModuleCosts feed AvailablePart's module_mass / module_cost).
//
// Scope rule: only what changes a [KSPField] (or Part prefab data) between AddComponent and the end of Load is ported, and
// only the part of OnAwake/OnLoad that does so. Everything else a module does (OnStart ..., events, actions, GetInfo,
// resource handlers, GameEvents subscriptions, FX/animation/transform lookups) belongs to the package that owns the module
// (STATUS.md WP-06 "Module behaviours not ported"); those classes are instantiated as the generic PartModule.
//
// A C# subclass that does not override OnAwake / OnLoad inherits the base class's: ModuleFactory registers the shared
// implementation under each such class name.
#include <algorithm>
#include <limits>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_commnet_parts.hpp"
#include "ksp/modules/module_part_variants.hpp"
#include "ksp/modules/part_structural_modules.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/modules/resource_modules.hpp"
#include "ksp/modules/resource_processing.hpp"
#include "ksp/modules/animation_modules.hpp"
#include "ksp/modules/deployables.hpp"
#include "ksp/modules/parachutes.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/modules/module_strut.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/modules/module_command.hpp"
#include "ksp/modules/module_reaction_wheel.hpp"
#include "ksp/modules/module_gimbal.hpp"
#include "ksp/modules/module_rcs.hpp"
#include "ksp/modules/module_resource_intake.hpp"
#include "ksp/modules/misc_modules.hpp"
#include "ksp/modules/module_structural.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/util/ksp_util.hpp"

#include "ksp/thermal/thermal_modules.hpp"

namespace ksp {
void RegisterExperienceManagementModule();  // kerbals/kerbal_misc.cpp (WP-31)
void RegisterTripLoggerModule();  // vessel/module_trip_logger.cpp (WP-14)
void RegisterAeroModules();       // aero/lifting_surface.cpp (WP-21)
namespace {

// Mathf.Clamp(float, min, max) (UnityEngine Mathf.cs)
float MathfClamp(float value, float min, float max) {
    if (value < min) value = min;
    else if (value > max) value = max;
    return value;
}
constexpr float kFloatEpsilon = std::numeric_limits<float>::denorm_min();  // Mathf.Epsilon (float.Epsilon, denormal)

// ModuleToggleCrossfeed (ModuleToggleCrossfeed.cs) moved to WP-15: src/modules/resource_modules.cpp

// ---- ModuleCargoPart.cs:96 ----------------------------------------------------------------------------------------------
class ModuleCargoPart : public PartModule {
public:
    using PartModule::PartModule;
    void OnLoad(const ConfigNode& node) override {
        PartModule::OnLoad(node);
        if (Get<int32_t>("stackableQuantity") > 1 && part && !part->Resources.empty()) {
            log::Info("[ModuleCargoPart]: Setting stackableQuantity to 1 as " + part->gameObjectName + " contains resources");
            Set<int32_t>("stackableQuantity", 1);
        }
    }
};

// ---- ModuleAnimationSetter: WP-27 (modules/animation_modules.hpp); ModuleAnimateHeat.cs:22 -------------------------------------------
class ModuleAnimateHeat : public ModuleAnimationSetter {
public:
    using ModuleAnimationSetter::ModuleAnimationSetter;
    void OnLoad(const ConfigNode& node) override {
        const auto value = node.GetValue("ThermalAnim");
        if (value && !value->empty()) Set<std::string>("animName", *value);
        ModuleAnimationSetter::OnLoad(node);
    }
};

// ---- BaseConverter / drills / converters / harvesters / generators / scanners: WP-28 (modules/resource_processing.hpp) ----

// ---- `if (curve == null) curve = new FloatCurve();` OnAwake bodies --------------------------------------------------------
// ModuleColorChanger.cs:162 (four colour curves), ModuleEngines.cs:1609 / ModuleRCS.cs:378 (atmosphereCurve; the FX
// subclasses call base.OnAwake), ModuleWheelSteering.cs:146, ModuleWheelMotor.cs:89. The curves are filled by the MODULE
// node (BaseFieldList.Load skips null IConfigNode fields, so the empty instance must exist first).
class EmptyCurvesModule : public PartModule {
public:
    EmptyCurvesModule(const ModuleClassInfo& c, std::vector<std::string> curves) : PartModule(c), curves_(std::move(curves)) {}
    void OnAwake() override {
        PartModule::OnAwake();
        for (const std::string& n : curves_)
            if (IsNull(n)) Set<FloatCurve>(n, FloatCurve{});
    }

private:
    std::vector<std::string> curves_;
};

// ---- ModuleDeployablePart / Antenna / Radiator / SolarPanel: WP-27 (modules/deployables.hpp) ----------------------------------------

// ModuleDataTransmitter / ModuleKerbNetAccess / ModuleRelayEnabler / ModuleProbeControlPoint: module_commnet_parts.cpp (WP-39)

// ---- FlagDecalBackground.cs:57 / :563 + IPartMassModifier / IPartCostModifier --------------------------------------------
class FlagDecalBackground : public PartModule, public IPartMassModifier, public IPartCostModifier {
public:
    using PartModule::PartModule;
    void OnAwake() override {
        PartModule::OnAwake();
        Set<std::string>("sizeLocked", Localizer::Format("#autoLOC_8003622"));
    }
    void OnLoad(const ConfigNode& node) override {
        if (!flagMeshes.empty()) return;
        PartModule::OnLoad(node);
        int portrait = 0;
        for (ConfigNode* m : node.GetNodes("MESH")) {
            std::string name, displayName, meshName, orientation = "LANDSCAPE";
            m->TryGetValue("name", name);
            m->TryGetValue("displayName", displayName);
            m->TryGetValue("meshName", meshName);
            m->TryGetValue("orientation", orientation);
            if (!name.empty() && !displayName.empty() && !meshName.empty()) {
                if (orientation == "PORTRAIT") portrait++;
                flagMeshes.push_back(name);
            }
        }
        if (portrait <= 0) Set<bool>("displayingPortrait", false);
    }
    IPartMassModifier* AsMassModifier() override { return this; }
    IPartCostModifier* AsCostModifier() override { return this; }
    float GetModuleMass(float, ModifierStagingSituation) override { return mass; }  // :641
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::FIXED; }
    float GetModuleCost(float, ModifierStagingSituation) override { return cost; }  // :651
    ModifierChangeWhen GetModuleCostChangeWhen() override { return ModifierChangeWhen::FIXED; }
    std::vector<std::string> flagMeshes;
    float mass = 0.0f, cost = 0.0f;  // set when a flag mesh is chosen (flight/editor)
};

// ModulePartVariants.cs: module_part_variants.cpp (WP-13)

// ---- the remaining IPartMassModifier / IPartCostModifier implementations -------------------------------------------------
class ModuleComet : public PartModule, public IPartMassModifier, public ISpaceObjectMass {  // ModuleComet.cs:469
public:
    using PartModule::PartModule;
    IPartMassModifier* AsMassModifier() override { return this; }
    float GetModuleMass(float defaultMass, ModifierStagingSituation) override {
        const float d = f32store(fx(cometMass) - fx(defaultMass));
        return MathfClamp(d, kFloatEpsilon, d < 0.0f ? -d : d);
    }
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }
    void SetSpaceObjectMass(float nMass) override { cometMass = nMass; }  // ModuleComet.cs:479 SetCometMass
    float cometMass = 1.0f;  // private, set by SetCometMass
};

class ModuleAsteroid : public PartModule, public IPartMassModifier, public ISpaceObjectMass {  // ModuleAsteroid.cs:229
public:
    using PartModule::PartModule;
    IPartMassModifier* AsMassModifier() override { return this; }
    float GetModuleMass(float defaultMass, ModifierStagingSituation) override { return f32store(fx(asteroidMass) - fx(defaultMass)); }
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }
    void SetSpaceObjectMass(float nMass) override { asteroidMass = nMass; }  // ModuleAsteroid.cs:239 SetAsteroidMass
    float asteroidMass = 1.0f;  // private, set by SetAsteroidMass
};

class ModuleInventoryPart : public PartModule, public IPartMassModifier, public IPartCostModifier {  // :1928 / :1966
public:
    using PartModule::PartModule;
    IPartMassModifier* AsMassModifier() override { return this; }
    IPartCostModifier* AsCostModifier() override { return this; }
    // storedParts is empty on a prefab (stored snapshots come from saves / the inventory UI: WP-13/18)
    float GetModuleMass(float, ModifierStagingSituation) override { return 0.0f; }
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }
    float GetModuleCost(float, ModifierStagingSituation) override { return 0.0f; }
    ModifierChangeWhen GetModuleCostChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }
};

class PartStatsUpgradeModule : public PartModule, public IPartMassModifier, public IPartCostModifier {  // :93 / :98
public:
    using PartModule::PartModule;
    IPartMassModifier* AsMassModifier() override { return this; }
    IPartCostModifier* AsCostModifier() override { return this; }
    float GetModuleMass(float, ModifierStagingSituation) override { return massOffset; }
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::FIXED; }
    float GetModuleCost(float, ModifierStagingSituation) override { return costOffset; }
    ModifierChangeWhen GetModuleCostChangeWhen() override { return ModifierChangeWhen::FIXED; }
    float massOffset = 0.0f, costOffset = 0.0f;  // set by ApplyUpgradeNode (PartUpgradeManager, WP-32)
};

template <class T>
void Reg(std::initializer_list<const char*> names) {
    for (const char* n : names)
        ModuleFactory::Register(n, [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<T>(c)); });
}

void RegCurves(std::initializer_list<const char*> names, std::vector<std::string> curves) {
    for (const char* n : names)
        ModuleFactory::Register(n, [curves](const ModuleClassInfo& c) {
            return std::static_pointer_cast<PartModule>(std::make_shared<EmptyCurvesModule>(c, curves));
        });
}

}  // namespace

namespace detail {
void RegisterBuiltinModules() {
    detail::RegisterDecouplerModules();  // ModuleDecouplerBase / ModuleDecouple / ModuleAnchoredDecoupler (WP-16)
    detail::RegisterDockingModules();    // WP-25: ModuleDockingNode
    detail::RegisterGrappleModules();    // WP-25: ModuleGrappleNode
    detail::RegisterStrutModules();      // WP-25: CModuleStrut
    detail::RegisterResourceModules();  // ModuleToggleCrossfeed, ModuleResourceAutoShiftState, CModuleFuelLine, EVAResourceContainer (WP-15)
    Reg<ModuleCargoPart>({"ModuleCargoPart"});
    detail::RegisterAnimationModules();  // WP-27: ModuleAnimationSetter, ModuleAnimateGeneric, ModuleAnimationGroup, ModuleAnimatorLandingGear
    detail::RegisterDeployableModules();  // WP-27: ModuleDeployablePart / Antenna / Radiator / SolarPanel
    detail::RegisterParachuteModules();   // WP-27: ModuleParachute, ModuleEvaChute
    Reg<ModuleAnimateHeat>({"ModuleAnimateHeat"});
    detail::RegisterResourceProcessingModules();  // WP-28: BaseConverter family, generators, scanners
    detail::RegisterCommNetModules();  // WP-39: ModuleDataTransmitter, ModuleKerbNetAccess, ModuleRelayEnabler, ModuleProbeControlPoint, CommNetVessel, CommNetScenario
    detail::RegisterThermalModules();  // ModuleCoreHeat, ModuleActiveRadiator, ModuleAblator, ModuleConductionMultiplier (WP-22)
    detail::RegisterEngineModules();  // WP-23: ModuleEngines, ModuleEnginesFX, MultiModeEngine (module_engines.cpp)
    detail::RegisterRcsModules();     // WP-23: ModuleRCS, ModuleRCSFX
    detail::RegisterControlModules(); // WP-24: ModuleCommand, ModuleSAS, ModuleRemoteController, ModuleReactionWheel
    detail::RegisterGimbalModule();   // WP-23: ModuleGimbal
    detail::RegisterIntakeModule();   // WP-23: ModuleResourceIntake
    Reg<FlagDecalBackground>({"FlagDecalBackground"});
    RegisterModulePartVariants();  // WP-13: modules/module_part_variants.cpp
    RegisterStructuralModules();   // WP-13: ModuleMirrorPlacement, ModuleDisplaceTweak
    RegisterExperienceManagementModule();  // WP-31: ModuleExperienceManagement
    detail::RegisterWp26Modules();  // WP-26: ModuleProceduralFairing, ModuleJettison, ModuleCargoBay, ModuleServiceModule, LaunchClamp, ModuleStructuralNode(+Toggle), ModuleDynamicNodes (module_structural.cpp)
    RegisterTripLoggerModule();    // WP-14: ModuleTripLogger (vessel/module_trip_logger.cpp)
    RegisterAeroModules();         // WP-21: ModuleLiftingSurface, ModuleControlSurface, ModuleAeroSurface
    Reg<ModuleComet>({"ModuleComet"});
    Reg<ModuleAsteroid>({"ModuleAsteroid"});
    detail::RegisterMiscModules();  // WP-30: ModuleGPS, FlagSite, ModulePartFirework (misc_modules.cpp)
    Reg<ModuleInventoryPart>({"ModuleInventoryPart"});
    Reg<PartStatsUpgradeModule>({"PartStatsUpgradeModule"});
    RegCurves({"ModuleColorChanger"}, {"redCurve", "greenCurve", "blueCurve", "alphaCurve"});
    RegCurves({"ModuleWheelSteering"}, {"steeringCurve", "steeringMaxAngleCurve"});
    RegCurves({"ModuleWheelMotor", "ModuleWheelMotorSteering"}, {"torqueCurve"});
}
}  // namespace detail

}  // namespace ksp
