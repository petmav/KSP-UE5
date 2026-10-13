#include "ksp/modules/vessel_module.hpp"

#include <algorithm>
#include <map>

#include "ksp/core/log.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/vessel/axis_groups_module.hpp"

namespace ksp {

// ---- VesselModule -------------------------------------------------------------------------------------------------------

VesselModule::VesselModule(const ModuleClassInfo& cls) : FieldStore(cls) {}
VesselModule::~VesselModule() = default;

// VesselModule.cs:49 (vessel = GetComponent<Vessel>() is the caller's `vessel` assignment)
void VesselModule::Awake() {
    fields_ = std::make_unique<BaseFieldList>(*this);
    OnAwake();
}

// VesselModule.cs:61
void VesselModule::Load(const ConfigNode& node) {
    Fields().Load(node);
    OnLoad(node);
}

// VesselModule.cs:67
void VesselModule::Save(ConfigNode& node) {
    Fields().Save(node);
    OnSave(node);
}

// VesselModule.cs:105
bool VesselModule::ShouldBeActive(bool loadedSceneIsFlight, bool vesselLoaded) {
    const Activation activation = GetActivation();
    if (activation == Always) return true;
    if (activation == Never) return false;
    if ((activation & FlightScene) == 0 && loadedSceneIsFlight) return false;
    if ((activation & NonFlightScenes) == 0 && !loadedSceneIsFlight) return false;
    if ((activation & LoadedVessels) == 0 && vesselLoaded) return false;
    if ((activation & UnloadedVessels) == 0 && !vesselLoaded) return false;
    return true;
}

// ---- factory ------------------------------------------------------------------------------------------------------------

namespace {

// FlightIntegrator.cs:225 GetOrder = 0 (the class itself is WP-21; its order is what the manager needs)
class OrderedVesselModule : public VesselModule {
public:
    OrderedVesselModule(const ModuleClassInfo& c, int order, Activation act) : VesselModule(c), order_(order), act_(act) {}
    int GetOrder() override { return order_; }
    Activation GetActivation() override { return act_; }

private:
    int order_;
    Activation act_;
};

std::map<std::string, VesselModuleFactory::Creator>& Creators() {
    static std::map<std::string, VesselModuleFactory::Creator> m = [] {
        std::map<std::string, VesselModuleFactory::Creator> r;
        // FlightIntegrator.cs:225
        r["FlightIntegrator"] = [](const ModuleClassInfo& c) {
            return std::make_shared<OrderedVesselModule>(c, 0, VesselModule::Always);
        };
        // AxisGroupsModule.cs (WP-14: the real module)
        r["AxisGroupsModule"] = CreateAxisGroupsModule;
        return r;
    }();
    return m;
}

}  // namespace

void VesselModuleFactory::Register(const std::string& className, Creator creator) { Creators()[className] = std::move(creator); }

std::shared_ptr<VesselModule> VesselModuleFactory::Create(const ModuleClassInfo& cls) {
    // the built-in behaviours (VesselModules such as CommNetVessel included) register on the first factory use
    // (RegisterBuiltinModules): without this a VesselModule made before any PartModule came out as the plain base class
    (void)ModuleFactory::IsRegistered(cls.name);
    auto it = Creators().find(cls.name);
    if (it != Creators().end()) return it->second(cls);
    return std::make_shared<VesselModule>(cls);
}

// ---- VesselModuleManager ------------------------------------------------------------------------------------------------

namespace {
std::vector<VesselModuleManager::VesselModuleWrapper>& ModulesStorage() {
    static std::vector<VesselModuleManager::VesselModuleWrapper> m;
    return m;
}
}  // namespace

std::vector<VesselModuleManager::VesselModuleWrapper>& VesselModuleManager::Modules() { return ModulesStorage(); }

void VesselModuleManager::CompileModules() {
    auto& modules = ModulesStorage();
    modules.clear();
    for (const ModuleClassInfo& c : HostClasses()) {
        if (!c.Is("VesselModule") || std::string_view(c.name) == "VesselModule") continue;
        VesselModuleWrapper w(&c);
        try {
            std::shared_ptr<VesselModule> m = VesselModuleFactory::Create(c);
            w.order = m->GetOrder();
            log::Info(std::string("VesselModules: Found VesselModule of type ") + c.name + " with order " + std::to_string(w.order));
            modules.push_back(w);
        } catch (const std::exception& e) {
            log::Error(std::string("VesselModules: Error getting order of VesselModule of type ") + c.name +
                       " so it was not added. Exception: " + e.what());
        }
    }
    log::Info("VesselModules: Found " + std::to_string(modules.size()) + " VesselModule types");
}

std::vector<VesselModuleManager::VesselModuleWrapper> VesselModuleManager::GetModules(bool activeOnly, bool order) {
    auto& modules = ModulesStorage();
    if (!activeOnly && !order) return modules;
    std::vector<VesselModuleWrapper> list;
    for (const VesselModuleWrapper& w : modules)
        if (!activeOnly || w.active) list.push_back(w);
    // List.Sort is unstable (introsort); order values are distinct per type except the 999 default, so the relative order of
    // equal keys is the only difference and callers must not depend on it. kspcore keeps registration order (stable).
    if (order) std::stable_sort(list.begin(), list.end(), [](const auto& a, const auto& b) { return a.order < b.order; });
    return list;
}

VesselModuleManager::VesselModuleWrapper* VesselModuleManager::GetWrapper(const ModuleClassInfo* t) {
    for (auto& w : ModulesStorage())
        if (w.type == t) return &w;
    return nullptr;
}
VesselModuleManager::VesselModuleWrapper* VesselModuleManager::GetWrapper(std::string_view name) {
    for (auto& w : ModulesStorage())
        if (name == w.type->name) return &w;
    return nullptr;
}
void VesselModuleManager::SetWrapperActive(const ModuleClassInfo* t, bool isActive) {
    for (auto& w : ModulesStorage())
        if (w.type == t) w.active = isActive;
}
void VesselModuleManager::SetWrapperActive(std::string_view name, bool isActive) {
    for (auto& w : ModulesStorage())
        if (name == w.type->name) w.active = isActive;
}

// VesselModuleManager.cs:143
void VesselModuleManager::AddModulesToVessel(void* vessel, std::vector<std::shared_ptr<VesselModule>>& modules,
                                             const std::function<bool(const ModuleClassInfo&)>& vesselHasComponent,
                                             bool loadedSceneIsFlight, bool vesselLoaded) {
    const auto list = GetModules(true, true);
    modules.clear();
    for (const VesselModuleWrapper& w : list) {
        const bool has = vesselHasComponent && vesselHasComponent(*w.type);
        if (!has || std::string_view(w.type->name) == "CometVessel") {
            std::shared_ptr<VesselModule> m = VesselModuleFactory::Create(*w.type);
            m->vessel = vessel;
            m->Awake();
            m->enabled = m->ShouldBeActive(loadedSceneIsFlight, vesselLoaded);
            modules.push_back(std::move(m));
        }
    }
}

bool VesselModuleManager::RemoveModuleOfType(const ModuleClassInfo* t) {
    auto& modules = ModulesStorage();
    for (size_t i = 0; i < modules.size(); i++)
        if (modules[i].type == t) {
            modules.erase(modules.begin() + static_cast<std::ptrdiff_t>(i));
            return true;
        }
    return false;
}

}  // namespace ksp
