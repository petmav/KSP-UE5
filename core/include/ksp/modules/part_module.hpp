// WP-06 module host, part side: PartModule (PartModule.cs), PartModuleList (PartModuleList.cs), the module interfaces the
// host needs (IModuleInfo.cs, IScalarModule.cs, IPartMassModifier.cs, IPartCostModifier.cs) and the module class registry
// (AssemblyLoader.GetClassByName + gameObject.AddComponent).
//
// The Unity MonoBehaviour lifecycle becomes explicit calls, in the order Part.AddModule / PartLoader.ParsePart use:
//     module = ModuleFactory::Create(className)   // AddComponent: the constructor chain runs the [KSPField] initialisers
//     module->part = &prefab; module->Awake();    // PartModule.Awake (PartModule.cs:743): ModularSetup (BaseFieldList /
//                                                 //   BaseEventList / BaseActionList), resHandler.OnAwake, OnAwake()
//     module->Load(node);                         // PartModule.Load (PartModule.cs:787): isEnabled, persistentId, UPGRADES,
//                                                 //   Fields.Load, EVENTS, ACTIONS, AXISGROUPS, OnLoad(node)
// Save is PartModule.Save (:756). OnStart / OnUpdate / ... are virtual no-ops the owning packages (WP-23 ... WP-30) override.
//
// A module class without a registered C++ implementation is instantiated as the generic PartModule: the schema-driven fields
// (typed values, defaults from the C# initialisers), events and actions (attribute data, no handlers) and no behaviour. A
// module package registers the class (REGISTER_PART_MODULE) with a subclass that overrides the lifecycle hooks and binds
// its [KSPEvent] / [KSPAction] methods in OnAwake with Events().Bind / Actions().Bind.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/modules/base_event.hpp"
#include "ksp/modules/base_field.hpp"
#include "ksp/modules/module_resource.hpp"
#include "ksp/unity/quaternion.hpp"

namespace ksp {

struct PartPrefab;
struct AttachNode;
class Part;
class ProtoPartModuleSnapshot;
class IPartSizeModifier;
class IConstruction;
class ModuleInventoryPart;
class IAnimatedModule;                 // WP-27 ksp/modules/animated_module.hpp
class ITorqueProvider;                 // WP-24 ksp/modules/control_types.hpp
namespace events {
template <class... Ts>
class EventData;                       // ksp/events/event_data.hpp
}
class IAnalyticOverheatModule;         // WP-22 ksp/thermal/thermal_types.hpp
class IAnalyticPreview;
class IAnalyticTemperatureModifier;
class IOverheatDisplay;

// ModifierStagingSituation.cs / ModifierChangeWhen.cs
enum class ModifierStagingSituation { CURRENT, UNSTAGED, STAGED };
enum class ModifierChangeWhen { FIXED, STAGED, CONSTANTLY };

// IPartMassModifier.cs / IPartCostModifier.cs
class IPartMassModifier {
public:
    virtual ~IPartMassModifier() = default;
    virtual float GetModuleMass(float defaultMass, ModifierStagingSituation sit) = 0;
    virtual ModifierChangeWhen GetModuleMassChangeWhen() = 0;
};
class IPartCostModifier {
public:
    virtual ~IPartCostModifier() = default;
    virtual float GetModuleCost(float defaultCost, ModifierStagingSituation sit) = 0;
    virtual ModifierChangeWhen GetModuleCostChangeWhen() = 0;
};

// IModuleInfo.cs (the draw callback is UI)
class IModuleInfo {
public:
    virtual ~IModuleInfo() = default;
    virtual std::string GetModuleTitle() = 0;
    virtual std::string GetInfo() = 0;
    virtual std::string GetPrimaryField() = 0;
};

// IScalarModule.cs (EventData<float,float> OnMoving / EventData<float> OnStop are owned by the implementing module)
class IScalarModule {
public:
    IScalarModule();
    virtual ~IScalarModule();
    // WP-27: the live scalar modules (a subscriber such as ModuleCargoBay must not touch the OnMoving / OnStop events of a deploy module
    // that was destroyed before it: C# keeps the destroyed object alive, C++ does not)
    static bool IsAlive(const IScalarModule* m);
    virtual std::string ScalarModuleID() const = 0;
    virtual float GetScalar() const = 0;
    virtual bool CanMove() const = 0;
    virtual void SetScalar(float t) = 0;
    virtual void SetUIRead(bool state) = 0;
    virtual void SetUIWrite(bool state) = 0;
    virtual bool IsMoving() = 0;
    // IScalarModule.OnMoving / OnStop (EventData<float, float> / EventData<float>), WP-27: the modules that move (ModuleDeployablePart,
    // ModuleAnimateGeneric, ModuleAnimationSetter) fire them; ModuleCargoBay (WP-26) subscribes (ModuleCargoBay.cs:136). nullptr = the
    // implementation has none.
    virtual events::EventData<float, float>* OnMoving() { return nullptr; }
    virtual events::EventData<float>* OnStop() { return nullptr; }
};

// PartModule.StartState (PartModule.cs:71)
enum class StartState : int {
    None = 0, Editor = 1, PreLaunch = 2, Landed = 4, Docked = 8, Flying = 0x10, Splashed = 0x20, SubOrbital = 0x40, Orbital = 0x80
};
// PartModule.PartUpgradeState (PartModule.cs:63)
enum class PartUpgradeState { NONE, LOCKED, AVAILABLE };

class PartModule : public FieldStore {
public:
    explicit PartModule(const ModuleClassInfo& cls);
    ~PartModule() override;
    PartModule(const PartModule&) = delete;
    PartModule& operator=(const PartModule&) = delete;

    // ---- state (PartModule.cs:86-175) ----
    PartPrefab* part = nullptr;      // PartModule.part (the Part the module is attached to)
    std::string moduleName;          // PartModule.moduleName: ModularSetup sets the class name; AddModule(ConfigNode) callers
                                     // that keep the MODULE `name` overwrite it
    bool isEnabled = true;
    ProtoPartModuleSnapshot* snapshot = nullptr;
    std::vector<ConfigNode::NodePtr> upgrades;  // PartModule.upgrades (null list is the empty vector)
    bool upgradesLoaded = false;                // upgrades != null
    std::vector<std::string> upgradesApplied;
    ModuleResourceHandler resHandler;  // PartModule.cs:175 (WP-15)

    const std::string& ClassName() const { return className_; }
    int ClassID() const { return classID_; }
    const std::string& GUIName() const { return guiName_; }
    BaseFieldList& Fields() { return *fields_; }
    BaseEventList& Events() { return *events_; }
    BaseActionList& Actions() { return *actions_; }
    bool IsA(std::string_view className) const { return Class().Is(className); }
    uint32_t PersistentId() const { return persistentId_; }
    uint32_t GetPersistentId();       // PartModule.cs:1206 (Guid hash; the id is random, only uniqueness is modelled)
    uint32_t NewPersistentId();
    void ClearPersistentId() { persistentId_ = 0; }
    bool HasUpgrades() const { return !upgrades.empty(); }  // PartModule.cs:361

    // ---- lifecycle ----
    void Awake();                       // PartModule.cs:743
    void Save(ConfigNode& node);        // PartModule.cs:756
    void Load(const ConfigNode& node);  // PartModule.cs:787
    void Copy(PartModule& from);        // PartModule.cs:841
    // PartModule.Load's BaseFieldList.Load assignments in node order (what PartModuleData::cfgFields reports)
    const std::vector<ModuleFieldValue>& LastLoadTrace() const { return loadTrace_; }

    virtual void OnAwake() {}
    virtual void OnIconCreate() {}
    // PartModule.OnStart is empty in C#; kspcore's also applies the `stagingIcon` default that the OnStart of the stageable modules
    // other packages own (ModuleEngines, ModuleParachute, LaunchClamp, ...) would set (module_staging.cpp). A subclass that overrides
    // OnStart and calls ModuleX::OnStart is unaffected: the table only names classes without a ported implementation.
    virtual void OnStart(StartState);
    virtual void OnStartFinished(StartState) {}
    virtual void OnStartBeforePartAttachJoint(StartState) {}      // PartModule.cs:873 (WP-13)
    virtual void OnInitialize() {}
    virtual void OnActive() {}
    virtual void OnInactive() {}
    virtual void OnUpdate() {}
    virtual void OnFixedUpdate() {}
    virtual void OnSave(ConfigNode&) {}
    virtual void OnLoad(const ConfigNode&) {}
    virtual void OnCopy(PartModule&) {}
    // WP-13 (Part.OnWillBeCopied / OnWasCopied / OnWillBeMirrored / PromoteToPhysicalPart / DemoteToPhysicslessPart call these)
    virtual void OnWillBeCopied(bool /*asSymCounterpart*/) {}                      // PartModule.cs:919
    virtual void OnWasCopied(PartModule* /*copyPartModule*/, bool /*asSymCounterpart*/) {}  // :923
    virtual bool OnWillBeMirrored(Quaternionf& /*rotation*/, AttachNode* /*selPartNode*/, Part* /*partParent*/) { return false; }  // :927
    virtual void PromoteToPhysicalPart() {}                                        // :948
    virtual void DemoteToPhysicslessPart() {}                                      // :952
    // WP-33 (PartModule.cs:932-962): the inventory / EVA-construction hooks; every one is an empty virtual in C#
    virtual void ResetWheelGroundCheck() {}                                        // PartModule.cs:1253 (internal; ModuleWheelBase)
    virtual void OnInventoryModeDisable() {}                                       // :932
    virtual void OnInventoryModeEnable() {}                                        // :936
    virtual void OnConstructionModeUpdate() {}                                     // :940
    virtual void OnConstructionModeFixedUpdate() {}                                // :944
    virtual void OnPartCreatedFomInventory(ModuleInventoryPart&) {}                // :956
    virtual void OnStoredInInventory(ModuleInventoryPart&) {}                      // :960
    // Unity's Instantiate: the serialized state of `from` (non-private KSPField values, moduleName, isEnabled,
    // upgradesApplied) is copied into this fresh component before its Awake. Modules with serialized state of their own
    // (ModulePartVariants.variantList) extend it.
    virtual void CopySerializedFrom(const PartModule& from);
    virtual std::string GetInfo() { return ""; }                   // PartModule.cs:905
    virtual std::string GetModuleDisplayName() { return ""; }      // PartModule.cs:910
    virtual void LoadUpgrades(const ConfigNode& node);             // PartModule.cs:422 (no PartUpgradeManager: see FindUpgrades)
    // PartModule.cs:272-345. WP-16 (src/modules/module_staging.cpp): the base implementations also answer for the per-class
    // overrides of the stageable modules whose own package has not ported the class yet (ModuleEngines, ModuleRCS, ModuleGimbal,
    // ModuleJettison, ModuleDockingNode, ModuleScienceExperiment, ModuleTestSubject, ModuleEvaChute); a ported subclass overrides.
    virtual bool IsStageable();                                    // :272
    // `ModuleAttributes.isStageable = value` (PartModule.cs:25): ModuleAttributes is the per-TYPE ReflectedAttributes (PartModule.cs:746
    // GetReflectedAttributes(GetType())), so this writes the flag shared by every instance of this module class.
    void SetModuleAttributesIsStageable(bool value);
    // C++ only (tests): forget every per-type ReflectedAttributes.isStageable (the next query re-evaluates the OnActive rule), like a fresh process.
    static void ResetReflectedAttributesForTests();
    virtual bool StagingEnabled();                                 // :277
    virtual void UpdateStagingToggle();                            // :282
    virtual void SetStaging(bool newValue);                        // :296
    void ToggleStaging();                                          // :303 [KSPEvent] (WP-16; bound in Awake)
    void SetStagingState(bool newState);                           // :326
    virtual bool StagingToggleEnabledEditor();                     // :333
    virtual bool StagingToggleEnabledFlight();                     // :338
    virtual std::string GetStagingEnableText();                    // :343
    virtual std::string GetStagingDisableText();                   // :352

    // Typed helpers for module code (Enum.Parse over the field's enum table; null checks of reference-typed fields)
    bool IsNull(std::string_view field) const;
    void SetEnumByName(std::string_view field, std::string_view member);  // Enum.Parse(typeof(T), member)
    std::string EnumName(std::string_view field) const;                   // enum value.ToString()

    // C# interface queries (`module is IPartMassModifier`): subclasses implementing the interface expose it here
    virtual IPartMassModifier* AsMassModifier() { return nullptr; }
    virtual IPartCostModifier* AsCostModifier() { return nullptr; }
    virtual IModuleInfo* AsModuleInfo() { return nullptr; }
    virtual IScalarModule* AsScalarModule() { return nullptr; }
    virtual IAnimatedModule* AsAnimatedModule() { return nullptr; }   // `module is IAnimatedModule` (WP-27)
    virtual IPartSizeModifier* AsSizeModifier() { return nullptr; }   // `module is IPartSizeModifier` (WP-13)
    virtual IConstruction* AsConstruction() { return nullptr; }       // `module is IConstruction` (WP-13)
    virtual IAnalyticOverheatModule* AsAnalyticOverheatModule() { return nullptr; }  // `module is IAnalyticOverheatModule` (WP-22)
    virtual IAnalyticPreview* AsAnalyticPreview() { return nullptr; }                // `module is IAnalyticPreview` (WP-22)
    virtual IAnalyticTemperatureModifier* AsAnalyticTemperatureModifier() { return nullptr; }  // (WP-22)
    virtual IOverheatDisplay* AsOverheatDisplay() { return nullptr; }                // `module is IOverheatDisplay` (WP-22)
    virtual ITorqueProvider* AsTorqueProvider() { return nullptr; }                  // `module is ITorqueProvider` (WP-24)

private:
    std::string className_, guiName_;
    int classID_ = 0;
    uint32_t persistentId_ = 0;
    std::unique_ptr<BaseFieldList> fields_;
    std::unique_ptr<BaseEventList> events_;
    std::unique_ptr<BaseActionList> actions_;
    std::vector<ModuleFieldValue> loadTrace_;
    void ModularSetup();  // PartModule.cs:233
};

// PartModuleList.cs
class PartModuleList {
public:
    PartModuleList() = default;
    size_t Count() const { return modules_.size(); }
    PartModule* operator[](std::string_view className) const;  // by ClassID; logs when absent
    PartModule* operator[](size_t index) const { return modules_[index].get(); }
    PartModule* ByPersistentId(uint32_t id) const;
    std::vector<PartModule*> GetModules(const std::function<bool(PartModule&)>& is) const;  // GetModules<T>()
    IScalarModule* GetScalarModule(std::string_view name) const;
    std::vector<IScalarModule*> GetScalarModules(std::string_view name) const;
    void Add(std::shared_ptr<PartModule> module);
    void Remove(const PartModule* module);
    int IndexOf(const PartModule* module) const;
    PartModule* GetModule(size_t index) const { return modules_[index].get(); }
    PartModule* GetModule(std::string_view moduleName) const;  // by moduleName equality
    PartModule* GetModule(const std::function<bool(PartModule&)>& is, int index = 0) const;  // GetModule<T>(index)
    bool Contains(std::string_view className) const;
    bool Contains(int classID) const;
    bool ContainsPersistentId(uint32_t id) const;
    void Clear() { modules_.clear(); }
    bool Send(std::string_view actionName, BaseEventDetails* data = nullptr);  // PartModuleList.cs:401
    bool Send(int actionID, BaseEventDetails* data = nullptr);
    bool SendDefault(BaseEventDetails* data = nullptr);
    auto begin() const { return modules_.begin(); }
    auto end() const { return modules_.end(); }

private:
    std::vector<std::shared_ptr<PartModule>> modules_;
};

// ScalarModuleSetHandler.cs: the `scalarModules = id` values of a MODULE node resolved to the part's IScalarModules
class ScalarModuleSetHandler {
public:
    std::string nodeName = "scalarModules";
    std::optional<std::vector<std::string>> fxModuleNames;  // null until Load finds values
    std::vector<IScalarModule*> fxModules;
    ScalarModuleSetHandler() = default;                                              // :9
    explicit ScalarModuleSetHandler(std::string configValueName) : nodeName(std::move(configValueName)) {}  // :13
    virtual ~ScalarModuleSetHandler() = default;
    virtual void Load(const ConfigNode& node);                      // :18
    virtual void Save(ConfigNode& node) const;                      // :27 (NullReferenceException while fxModuleNames is null)
    virtual void Initialize(const PartModuleList& modules);         // :36 (Part.Modules.GetScalarModules(name.Trim()))
    virtual void SetFXModules(float scalar);                        // :46
};

// AssemblyLoader.GetClassByName(typeof(PartModule), name) + AddComponent: creates the module object of a class.
class ModuleFactory {
public:
    using Creator = std::function<std::shared_ptr<PartModule>(const ModuleClassInfo&)>;
    // Register the implementation of one module class (exact Type.Name). The last registration of a name wins.
    static void Register(const std::string& className, Creator creator);
    // Creates the object for a resolved class: the registered implementation, else the generic PartModule.
    static std::shared_ptr<PartModule> Create(const ModuleClassInfo& cls);
    static bool IsRegistered(const std::string& className);
};

template <class T>
struct ModuleRegistrar {
    explicit ModuleRegistrar(const char* className) {
        ModuleFactory::Register(className, [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<T>(c)); });
    }
};

// Part-level modifier sums over a module list (Part.GetModuleMass / GetModuleCosts, Part.cs:7518/7532).
float PartGetModuleMass(const PartModuleList& modules, float defaultMass, ModifierStagingSituation sit = ModifierStagingSituation::CURRENT);
float PartGetModuleCosts(const PartModuleList& modules, float defaultCost, ModifierStagingSituation sit = ModifierStagingSituation::CURRENT);

}  // namespace ksp
