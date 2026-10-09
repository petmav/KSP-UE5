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
#include "ksp/modules/module_part_variants.hpp"
#include "ksp/modules/part_structural_modules.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/modules/resource_modules.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {
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

// ---- ModuleAnimationSetter.cs:40 / ModuleAnimateHeat.cs:22 -------------------------------------------------------------
class ModuleAnimationSetter : public PartModule {
public:
    using PartModule::PartModule;
    void OnAwake() override {
        if (IsNull("animStateCurve")) {
            FloatCurve c;
            c.Add(0.0f, 0.0f);
            c.Add(1.0f, 1.0f);
            Set<FloatCurve>("animStateCurve", std::move(c));
        }
    }
};
class ModuleAnimateHeat : public ModuleAnimationSetter {
public:
    using ModuleAnimationSetter::ModuleAnimationSetter;
    void OnLoad(const ConfigNode& node) override {
        const auto value = node.GetValue("ThermalAnim");
        if (value && !value->empty()) Set<std::string>("animName", *value);
        ModuleAnimationSetter::OnLoad(node);
    }
};

// ---- BaseConverter.cs:460 OnAwake / :342 OnLoad (ModuleResourceConverter, BaseDrill descendants, ModuleScienceConverter) --
// The resource-ratio lists (INPUT/OUTPUT/REQUIRED_RESOURCE) are WP-28's; the prefab has no vessel, so OnLoad returns before
// SetupModule.
class BaseConverter : public PartModule {
public:
    using PartModule::PartModule;
    void OnAwake() override {
        if (IsNull("ThermalEfficiency")) {
            FloatCurve c;
            c.Add(0.0f, 1.0f);
            Set<FloatCurve>("ThermalEfficiency", std::move(c));
        }
        if (IsNull("TemperatureModifier")) {
            FloatCurve c;
            c.Add(0.0f, 100.0f);
            Set<FloatCurve>("TemperatureModifier", std::move(c));
        }
    }
    void OnLoad(const ConfigNode& node) override {
        PartModule::OnLoad(node);
        Set<std::string>("status", Localizer::Format("#autoLOC_257112"));
    }
};

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

// ---- ModuleDeployablePart.cs ------------------------------------------------------------------------------------------
class ModuleDeployablePart : public PartModule {
public:
    using PartModule::PartModule;
    // :542 OnAwake also reads originalRotation / currentRotation from the part's pivot transform (FindModelTransform): the
    // model's transform tree is UE-import data (excluded in tests/data/part_database_exclusions.json)
    // :732
    void OnLoad(const ConfigNode& node) override {
        Set<std::string>("subPartName", Localizer::Format(Get<std::string>("subPartName")));
        Set<std::string>("partType", Localizer::Format(Get<std::string>("partType")));
        if (node.HasValue("stateString")) SetEnumByName("deployState", *node.GetValue("stateString"));
        if (node.HasValue("sunTracking")) {
            if (cs::BoolParse(*node.GetValue("sunTracking"))) {
                Set<bool>("isTracking", true);
                SetEnumByName("trackingMode", "SUN");
            } else {
                Set<bool>("isTracking", false);
            }
        }
        if (node.HasValue("vesselID")) vesselID = *node.GetValue("vesselID");
        destroyOnBreakObjects = node.GetValues("destroyOnBreakObject");
    }
    std::string vesselID;
    std::vector<std::string> destroyOnBreakObjects;
};

class ModuleDeployableAntenna : public ModuleDeployablePart {
public:
    using ModuleDeployablePart::ModuleDeployablePart;
    void OnLoad(const ConfigNode& node) override {  // ModuleDeployableAntenna.cs:23
        ModuleDeployablePart::OnLoad(node);
        Set<std::string>("subPartName", Localizer::Format("#autoLOC_234195"));
        Set<std::string>("partType", Localizer::Format("#autoLOC_234196"));
    }
};

class ModuleDeployableRadiator : public ModuleDeployablePart {
public:
    using ModuleDeployablePart::ModuleDeployablePart;
    void OnLoad(const ConfigNode& node) override {  // ModuleDeployableRadiator.cs:7
        ModuleDeployablePart::OnLoad(node);
        Set<std::string>("subPartName", Localizer::Format("#autoLOC_234369"));
        Set<std::string>("partType", Localizer::Format("#autoLOC_232117"));
    }
};

class ModuleDeployableSolarPanel : public ModuleDeployablePart {
public:
    using ModuleDeployablePart::ModuleDeployablePart;
    // ModuleDeployableSolarPanel.cs:97 (resHandler.moduleResourceBasedPrimaryIsInput = false; layer masks: not KSPFields)
    void OnAwake() override {
        ModuleDeployablePart::OnAwake();
        resHandler.moduleResourceBasedPrimaryIsInput = false;  // ModuleDeployableSolarPanel.cs:97 (WP-15)
        if (IsNull("powerCurve")) Set<FloatCurve>("powerCurve", FloatCurve{});
        if (IsNull("temperatureEfficCurve")) {
            FloatCurve c;
            c.Add(4.0f, 1.2f, 0.0f, -0.0006f);
            c.Add(300.0f, 1.0f, -0.0008f, -0.0008f);
            c.Add(1200.0f, 0.134f, -0.00035f, -0.00035f);
            c.Add(1900.0f, 0.02f, -3.72E-05f, -3.72E-05f);
            c.Add(2500.0f, 0.01f, 0.0f, 0.0f);
            Set<FloatCurve>("temperatureEfficCurve", std::move(c));
        }
        if (IsNull("timeEfficCurve")) {
            FloatCurve c;
            c.Add(0.0f, 1.0f);
            Set<FloatCurve>("timeEfficCurve", std::move(c));
        }
        // originalEfficiencyMultiplier = efficiencyMult (not a KSPField)
    }
    // :173 (the output ModuleResource is resHandler state, WP-15)
    void OnLoad(const ConfigNode& node) override {
        const std::string secondary = Get<std::string>("secondaryTransformName");
        if (secondary.empty()) Set<std::string>("secondaryTransformName", Get<std::string>("raycastTransformName"));
        ModuleDeployablePart::OnLoad(node);
        Set<std::string>("subPartName", Localizer::Format("#autoLOC_235328"));
        Set<std::string>("partType", Localizer::Format("#autoLOC_235329"));
        if (node.HasValue("type")) SetEnumByName("panelType", *node.GetValue("type"));
    }
};

// ---- ModuleDataTransmitter.cs:107 / :133 -------------------------------------------------------------------------------
class ModuleDataTransmitter : public PartModule {
public:
    using PartModule::PartModule;
    void OnAwake() override {
        DoubleCurve range;
        range.Add(0.0, 0.0, 0.0, 0.0);
        range.Add(1.0, 1.0, 0.0, 0.0);
        Set<DoubleCurve>("rangeCurve", std::move(range));
        DoubleCurve science;
        science.Add(0.0, 0.0, 0.0, 0.0);
        science.Add(0.44999998807907104, 0.07999999821186066, 0.4951019, 0.4951019);
        science.Add(0.8100000023841858, 0.3499999940395355, 0.8020515, 0.8020515);
        science.Add(1.0, 0.4000000059604645, 0.0, 0.0);
        Set<DoubleCurve>("scienceCurve", std::move(science));
        // consumedResources / transmissionQueue: not KSPFields (resource handler: WP-15)
    }
    void OnLoad(const ConfigNode&) override {
        // DeployFxModules / ProgressFxModules / CommsData (ScienceData queue): science package
        Events()["TransmitIncompleteToggle"]->SetGuiName(Get<bool>("xmitIncomplete") ? "#autoLOC_7001224" : "#autoLOC_7001225");
        UpdatePowerText();
    }
    // :165 (no module adjusters on a prefab: CommPower == antennaPower)
    void UpdatePowerText() {
        Set<std::string>("powerText", KSPUtil::PrintSI(Get<double>("antennaPower"), "") +
                                          (Get<bool>("antennaCombinable") ? " " + Localizer::Format("#autoLOC_236248") : std::string()));
    }
};

// ---- ModuleGimbal.cs:296 ------------------------------------------------------------------------------------------------
class ModuleGimbal : public PartModule {
public:
    using PartModule::PartModule;
    void OnLoad(const ConfigNode&) override {
        if (Get<float>("gimbalRangeXP") < 0.0f) Set<float>("gimbalRangeXP", Get<float>("gimbalRange"));
        if (Get<float>("gimbalRangeYP") < 0.0f) Set<float>("gimbalRangeYP", Get<float>("gimbalRange"));
        if (Get<float>("gimbalRangeXN") < 0.0f) Set<float>("gimbalRangeXN", Get<float>("gimbalRangeXP"));
        if (Get<float>("gimbalRangeYN") < 0.0f) Set<float>("gimbalRangeYN", Get<float>("gimbalRangeYP"));
    }
};

// ---- ModuleCoreHeat.cs:416 / ModuleResourceIntake.cs:75 / MultiModeEngine.cs:283 ---------------------------------------------
class ModuleCoreHeat : public PartModule {
public:
    using PartModule::PartModule;
    void OnAwake() override {
        if (IsNull("PassiveEnergy")) {
            FloatCurve c;
            c.Add(0.0f, 0.0f);
            Set<FloatCurve>("PassiveEnergy", std::move(c));
        }
    }
    // OnLoad returns at once without a vessel (the prefab has none)
};

class ModuleResourceIntake : public PartModule {
public:
    using PartModule::PartModule;
    void OnAwake() override {
        PartModule::OnAwake();
        if (IsNull("machCurve")) {
            FloatCurve c;
            c.Add(0.0f, 1.0f);
            Set<FloatCurve>("machCurve", std::move(c));
        }
    }
    void OnLoad(const ConfigNode&) override {  // :134 (resourceId = resourceName.GetHashCode(): not a KSPField)
        if (Get<bool>("intakeEnabled")) {
            Events()["Deactivate"]->active = false;
            Events()["Activate"]->active = true;
        }
    }
};

class MultiModeEngine : public PartModule {
public:
    using PartModule::PartModule;
    void OnLoad(const ConfigNode& node) override {
        PartModule::OnLoad(node);
        if (!Get<bool>("autoSwitchAvailable")) {
            Set<bool>("autoSwitch", false);
            BaseEvent* disable = Events()["DisableAutoSwitch"];
            BaseEvent* enable = Events()["EnableAutoSwitch"];
            enable->guiActiveEditor = false;
            enable->guiActive = false;
            enable->active = false;
            disable->guiActiveEditor = false;
            disable->guiActive = false;
            disable->active = false;
        }
    }
};

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
class ModuleComet : public PartModule, public IPartMassModifier {  // ModuleComet.cs:469
public:
    using PartModule::PartModule;
    IPartMassModifier* AsMassModifier() override { return this; }
    float GetModuleMass(float defaultMass, ModifierStagingSituation) override {
        const float d = f32store(fx(cometMass) - fx(defaultMass));
        return MathfClamp(d, kFloatEpsilon, d < 0.0f ? -d : d);
    }
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }
    float cometMass = 1.0f;  // private, set by SetCometMass
};

class ModuleAsteroid : public PartModule, public IPartMassModifier {  // ModuleAsteroid.cs:229
public:
    using PartModule::PartModule;
    IPartMassModifier* AsMassModifier() override { return this; }
    float GetModuleMass(float defaultMass, ModifierStagingSituation) override { return f32store(fx(asteroidMass) - fx(defaultMass)); }
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }
    float asteroidMass = 1.0f;  // private, set by SetAsteroidMass
};

class ModulePartFirework : public PartModule, public IPartMassModifier {  // ModulePartFirework.cs:769
public:
    using PartModule::PartModule;
    IPartMassModifier* AsMassModifier() override { return this; }
    float GetModuleMass(float defaultMass, ModifierStagingSituation) override {
        return f32store(fx(defaultMass) + fx(Get<float>("fireworkShots")) * fx(Get<float>("shellMass")) * fx(0.001f));
    }
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::CONSTANTLY; }
};

class ModuleProceduralFairing : public PartModule, public IPartMassModifier, public IPartCostModifier {  // :1663 / :1682
public:
    using PartModule::PartModule;
    IPartMassModifier* AsMassModifier() override { return this; }
    IPartCostModifier* AsCostModifier() override { return this; }
    float GetModuleMass(float, ModifierStagingSituation sit) override {
        if (sit == ModifierStagingSituation::STAGED) return 0.0f;
        return MathfClamp(panelMass, kFloatEpsilon, panelMass < 0.0f ? -panelMass : panelMass);
    }
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::STAGED; }
    float GetModuleCost(float, ModifierStagingSituation sit) override {
        if (sit == ModifierStagingSituation::STAGED) return 0.0f;
        return f32store(fx(GetFairingArea()) * fx(Get<float>("UnitAreaCost")));
    }
    ModifierChangeWhen GetModuleCostChangeWhen() override { return ModifierChangeWhen::STAGED; }
    float GetFairingArea() const { return 0.0f; }  // sum over the built fairing panels: none on a prefab
    float panelMass = 0.0f;  // private; panelMass = GetFairingArea() * UnitAreaMass when the fairing is built
};

class ModuleJettison : public PartModule, public IPartMassModifier {  // ModuleJettison.cs:602
public:
    using PartModule::PartModule;
    IPartMassModifier* AsMassModifier() override { return this; }
    float GetModuleMass(float, ModifierStagingSituation) override {
        if (!Get<bool>("modifyJettisonedMass")) return 0.0f;
        if (Get<bool>("isJettisoned")) return Get<bool>("useCalculatedMass") ? -partDensity : -Get<float>("jettisonedObjectMass");
        return 0.0f;
    }
    ModifierChangeWhen GetModuleMassChangeWhen() override { return ModifierChangeWhen::STAGED; }
    float partDensity = 0.0f;  // private
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
    detail::RegisterResourceModules();  // ModuleToggleCrossfeed, ModuleResourceAutoShiftState, CModuleFuelLine, EVAResourceContainer (WP-15)
    Reg<ModuleCargoPart>({"ModuleCargoPart"});
    Reg<ModuleAnimationSetter>({"ModuleAnimationSetter"});
    Reg<ModuleAnimateHeat>({"ModuleAnimateHeat"});
    Reg<BaseConverter>({"BaseConverter", "BaseDrill", "ModuleAsteroidDrill", "ModuleCometDrill", "ModuleResourceConverter",
                        "ModuleResourceHarvester", "ModuleScienceConverter"});
    Reg<ModuleDeployablePart>({"ModuleDeployablePart"});
    Reg<ModuleDeployableAntenna>({"ModuleDeployableAntenna"});
    Reg<ModuleDeployableRadiator>({"ModuleDeployableRadiator"});
    Reg<ModuleDeployableSolarPanel>({"ModuleDeployableSolarPanel"});
    Reg<ModuleDataTransmitter>({"ModuleDataTransmitter"});
    Reg<ModuleGimbal>({"ModuleGimbal"});
    Reg<ModuleCoreHeat>({"ModuleCoreHeat"});
    Reg<ModuleResourceIntake>({"ModuleResourceIntake"});
    Reg<MultiModeEngine>({"MultiModeEngine"});
    Reg<FlagDecalBackground>({"FlagDecalBackground"});
    RegisterModulePartVariants();  // WP-13: modules/module_part_variants.cpp
    RegisterStructuralModules();   // WP-13: ModuleMirrorPlacement, ModuleDisplaceTweak
    RegisterTripLoggerModule();    // WP-14: ModuleTripLogger (vessel/module_trip_logger.cpp)
    RegisterAeroModules();         // WP-21: ModuleLiftingSurface, ModuleControlSurface, ModuleAeroSurface
    Reg<ModuleComet>({"ModuleComet"});
    Reg<ModuleAsteroid>({"ModuleAsteroid"});
    Reg<ModulePartFirework>({"ModulePartFirework"});
    Reg<ModuleProceduralFairing>({"ModuleProceduralFairing"});
    Reg<ModuleJettison>({"ModuleJettison"});
    Reg<ModuleInventoryPart>({"ModuleInventoryPart"});
    Reg<PartStatsUpgradeModule>({"PartStatsUpgradeModule"});
    RegCurves({"ModuleColorChanger"}, {"redCurve", "greenCurve", "blueCurve", "alphaCurve"});
    RegCurves({"ModuleEngines", "ModuleEnginesFX", "ModuleRCS", "ModuleRCSFX"}, {"atmosphereCurve"});
    RegCurves({"ModuleWheelSteering"}, {"steeringCurve", "steeringMaxAngleCurve"});
    RegCurves({"ModuleWheelMotor", "ModuleWheelMotorSteering"}, {"torqueCurve"});
}
}  // namespace detail

}  // namespace ksp
