// WP-06 module host, vessel side: VesselModule (VesselModule.cs) and VesselModuleManager (VesselModuleManager.cs).
//
// A VesselModule is a MonoBehaviour added to the Vessel's GameObject once per registered type (ordered by GetOrder). The
// Vessel itself is WP-14: `vessel` is an opaque handle here, and the activation test (ShouldBeActive) takes the two facts it
// reads from HighLogic / Vessel as arguments. Lifecycle as explicit calls:
//     m = VesselModuleFactory::Create(cls); m->vessel = v; m->Awake();   // AddComponent (VesselModule.Awake: BaseFieldList)
//     m->Start();  m->Load(node) / m->Save(node);
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/modules/base_field.hpp"

namespace ksp {

class VesselModule : public FieldStore {
public:
    // VesselModule.Activation (VesselModule.cs:7)
    enum Activation : int {
        Never = 0, FlightScene = 1, NonFlightScenes = 2, LoadedVessels = 4, UnloadedVessels = 8, AllScenes = 3,
        LoadedOrUnloaded = 0xC, Always = 0xFF
    };

    explicit VesselModule(const ModuleClassInfo& cls);
    ~VesselModule() override;

    void* vessel = nullptr;           // VesselModule.vessel (WP-14's Vessel)
    bool enabled = true;              // Behaviour.enabled
    BaseFieldList& Fields() { return *fields_; }
    std::string ClassName() const { return Class().name; }

    virtual int GetOrder() { return 999; }                        // VesselModule.cs:44
    virtual Activation GetActivation() { return Always; }         // VesselModule.cs:49
    void Awake();                                                 // :54
    void Start() { OnStart(); }                                   // :63
    void Load(const ConfigNode& node);                            // :68
    void Save(ConfigNode& node);                                  // :74
    virtual void OnLoadVessel() {}
    virtual void OnUnloadVessel() {}
    virtual void OnGoOnRails() {}
    virtual void OnGoOffRails() {}
    // :99. `loadedSceneIsFlight` = HighLogic.LoadedSceneIsFlight, `vesselLoaded` = vessel.loaded
    virtual bool ShouldBeActive(bool loadedSceneIsFlight, bool vesselLoaded);

protected:
    virtual void OnAwake() {}
    virtual void OnStart() {}
    virtual void OnLoad(const ConfigNode&) {}
    virtual void OnSave(ConfigNode&) {}

private:
    std::unique_ptr<BaseFieldList> fields_;
};

// AssemblyLoader.GetClassByName(typeof(VesselModule), name) + AddComponent
class VesselModuleFactory {
public:
    using Creator = std::function<std::shared_ptr<VesselModule>(const ModuleClassInfo&)>;
    static void Register(const std::string& className, Creator creator);
    static std::shared_ptr<VesselModule> Create(const ModuleClassInfo& cls);
};

// VesselModuleManager.cs
class VesselModuleManager {
public:
    struct VesselModuleWrapper {
        const ModuleClassInfo* type;
        bool active = true;
        int order = 0;
        explicit VesselModuleWrapper(const ModuleClassInfo* t) : type(t) {}
    };
    // :34 CompileModules: every VesselModule subclass of the schema (file order), its order read from a temporary instance
    static void CompileModules();
    static std::vector<VesselModuleWrapper>& Modules();
    static std::vector<VesselModuleWrapper> GetModules(bool activeOnly, bool order);  // :64 (stable sort by order)
    static VesselModuleWrapper* GetWrapper(const ModuleClassInfo* moduleType);
    static VesselModuleWrapper* GetWrapper(std::string_view moduleTypeName);
    static void SetWrapperActive(const ModuleClassInfo* moduleType, bool isActive);
    static void SetWrapperActive(std::string_view moduleTypeName, bool isActive);
    // :178 AddModulesToVessel: one module per active type in order; a type the vessel already has is skipped (except
    // CometVessel). `existing` stands for gameObject.GetComponent(type).
    static void AddModulesToVessel(void* vessel, std::vector<std::shared_ptr<VesselModule>>& modules,
                                   const std::function<bool(const ModuleClassInfo&)>& vesselHasComponent = nullptr,
                                   bool loadedSceneIsFlight = false, bool vesselLoaded = false);
    static bool RemoveModuleOfType(const ModuleClassInfo* vesselModuleType);  // :229
};

}  // namespace ksp
