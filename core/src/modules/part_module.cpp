#include "ksp/modules/part_module.hpp"
#include "ksp/modules/axis_groups_manager.hpp"

#include <algorithm>
#include <map>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/resources/part_resource_library.hpp"

namespace ksp {

// ---- PartModule ---------------------------------------------------------------------------------------------------------

// The C# constructor chain (field initialisers) is FieldStore's constructor.
PartModule::PartModule(const ModuleClassInfo& cls) : FieldStore(cls) {
    className_ = cls.name;  // ReflectedAttributes: moduleType.Name
    classID_ = MonoStringHash(className_);
}

PartModule::~PartModule() = default;

// PartModule.cs:233 ModularSetup (+ AxisGroupsManager.BuildBaseAxisFields inside BaseFieldList's constructor)
void PartModule::ModularSetup() {
    events_ = std::make_unique<BaseEventList>(this, Class());
    actions_ = std::make_unique<BaseActionList>(this, Class());
    fields_ = std::make_unique<BaseFieldList>(*this);
    AxisGroupsManager::BuildBaseAxisFields(*this);
    // ModuleAttributes.kspModules is empty for every class (KSPModule is never applied): guiName = PrintModuleName
    guiName_ = KSPUtil::PrintModuleName(className_);
    moduleName = className_;  // both branches of the partInfo test assign className
}

// PartModule.cs:743
void PartModule::Awake() {
    ModularSetup();
    resHandler.SetPartModule(this);  // PartModule.cs:748
    resHandler.OnAwake();
    OnAwake();
}

// PartModule.cs:756
void PartModule::Save(ConfigNode& node) {
    node.ClearValues();
    node.ClearNodes();
    node.AddValue("name", className_);
    node.AddValue("isEnabled", isEnabled);
    if (persistentId_ != 0) node.AddValue("persistentId", persistentId_);
    // HasAdjusters: module adjusters (AdjusterPartModuleBase, WP-13/Strategies) are not modelled: no ADJUSTERS node
    Fields().Save(node);
    Events().OnSave(*node.AddNode("EVENTS"));
    Actions().OnSave(*node.AddNode("ACTIONS"));
    AxisGroupsManager::SaveAxisFieldNodes(*this, node);  // AxisGroupsManager.cs:36
    OnSave(node);
    // SaveUpgradesApplied(node.AddNode("UPGRADESAPPLIED")) (PartModule.cs:400)
    ConfigNode* ua = node.AddNode("UPGRADESAPPLIED");
    for (const std::string& u : upgradesApplied) ua->AddValue("name", u);
}

// PartModule.cs:422 LoadUpgrades (the LoadUpgradesDel hook is PartUpgradeManager's, not installed here)
void PartModule::LoadUpgrades(const ConfigNode& node) {
    ConfigNode* up = node.GetNode("UPGRADES");
    if (up) {
        upgrades.clear();
        upgradesLoaded = true;
        for (int i = 0; i < up->nodes().Count(); i++) upgrades.push_back(up->nodes().Shared(i));
    } else {
        if (upgradesLoaded && !upgrades.empty()) return;
        // part.partInfo.partPrefab.Modules[index].upgrades: the prefab module of the same slot (none while compiling it)
        upgradesLoaded = true;
    }
}

// PartModule.cs:787
void PartModule::Load(const ConfigNode& node) {
    if (node.HasValue("isEnabled")) isEnabled = cs::BoolParse(*node.GetValue("isEnabled"));
    else isEnabled = true;
    persistentId_ = 0;
    node.TryGetValue("persistentId", persistentId_);
    // ADJUSTERS / ADJUSTERSTOREMOVE: AdjusterPartModuleBase.CreateModuleAdjusterList (Strategies/adjusters): not modelled
    LoadUpgrades(node);
    // LoadUpgradesApplied(upgradesApplied, node.GetNode("UPGRADESAPPLIED")) (PartModule.cs:409)
    if (ConfigNode* ua = node.GetNode("UPGRADESAPPLIED")) {
        upgradesApplied.clear();
        for (const std::string& v : ua->GetValues("name")) upgradesApplied.push_back(v);
    }
    // if (!HighLogic.LoadedSceneIsEditor) ApplyUpgradeNode(upgradesApplied, node, doLoad: false): PartUpgradeManager (WP-32)
    loadTrace_.clear();
    Fields().Load(node, &loadTrace_);
    if (node.HasNode("EVENTS")) Events().OnLoad(*node.GetNode("EVENTS"));
    if (node.HasNode("ACTIONS")) Actions().OnLoad(*node.GetNode("ACTIONS"));
    AxisGroupsManager::LoadAxisFieldNodes(*this, node);  // AxisGroupsManager.cs:19
    resHandler.OnLoad(node);  // PartModule.cs:829
    try {
        OnLoad(node);
    } catch (const std::exception& e) {
        log::Error("Module " + moduleName + " threw during OnLoad: " + e.what());
    }
    UpdateStagingToggle();
}

// PartModule.cs:841
void PartModule::Copy(PartModule& from) {
    for (size_t i = Actions().Count(); i-- > 0;) Actions()[i]->CopyAction(*from.Actions()[i]);
    for (size_t i = Fields().Count(); i-- > 0;) Fields()[i]->CopyField(*from.Fields()[i]);
    Fields().SetOriginalValue();
    OnCopy(from);
}

// Object.Instantiate of a module component (WP-13): moduleName, isEnabled, upgradesApplied and the serialized KSPFields
void PartModule::CopySerializedFrom(const PartModule& from) {
    CopyValuesFrom(from);
    moduleName = from.moduleName;
    isEnabled = from.isEnabled;
    upgradesApplied = from.upgradesApplied;
}

// PartModule.cs:282
void PartModule::UpdateStagingToggle() {
    BaseEvent* e = Events()["ToggleStaging"];
    if (!e) throw NullReferenceException("Object reference not set to an instance of an object");
    if (Get<bool>("stagingEnabled")) e->SetGuiName(GetStagingDisableText());
    else e->SetGuiName(GetStagingEnableText());
    e->guiActiveEditor = StagingToggleEnabledEditor();
    e->guiActive = StagingToggleEnabledFlight();
}

// PartModule.cs:296
void PartModule::SetStaging(bool newValue) {
    Set<bool>("stagingEnabled", newValue);
    UpdateStagingToggle();
}

// PartModule.cs:343 / :352
std::string PartModule::GetStagingEnableText() {
    const ModuleValue* v = Value("stagingEnableText");
    if (const auto* s = std::get_if<std::string>(v); s && !s->empty()) return *s;
    return Localizer::Format("#autoLOC_232338");
}
std::string PartModule::GetStagingDisableText() {
    const ModuleValue* v = Value("stagingDisableText");
    if (const auto* s = std::get_if<std::string>(v); s && !s->empty()) return *s;
    return Localizer::Format("#autoLOC_232342");
}

// PartModule.cs:1206 (the C# id is a random Guid hash; a counter keeps kspcore deterministic and still unique)
uint32_t PartModule::GetPersistentId() {
    static uint32_t counter = 0;
    while (persistentId_ == 0) persistentId_ = 0x9E3779B1u * (++counter);
    return persistentId_;
}
uint32_t PartModule::NewPersistentId() {
    ClearPersistentId();
    return GetPersistentId();
}

bool PartModule::IsNull(std::string_view field) const {
    const ModuleValue* v = Value(field);
    return !v || std::holds_alternative<std::monostate>(*v);
}

void PartModule::SetEnumByName(std::string_view field, std::string_view member) {
    ModuleValue* v = Value(field);
    const int i = IndexOf(field);
    if (!v || i < 0 || !FieldInfos()[static_cast<size_t>(i)]->enumTable) throw std::runtime_error("PartModule::SetEnumByName: " + std::string(field) + " is not an enum field");
    const EnumTable* t = FieldInfos()[static_cast<size_t>(i)]->enumTable;
    *v = EnumValue{t->Parse(member), t};
}

std::string PartModule::EnumName(std::string_view field) const {
    const ModuleValue* v = Value(field);
    const auto* e = v ? std::get_if<EnumValue>(v) : nullptr;
    if (!e) throw std::runtime_error("PartModule::EnumName: " + std::string(field) + " is not an enum field");
    return e->table->ToString(e->value);
}

// ---- PartModuleList -----------------------------------------------------------------------------------------------------

// PartModuleList.cs:19
PartModule* PartModuleList::operator[](std::string_view className) const {
    const int hash = MonoStringHash(className);
    for (const auto& m : modules_)
        if (m->ClassID() == hash) return m.get();
    log::Error("Cannot find module '" + std::string(className) + "' (" + std::to_string(hash) + ")");
    return nullptr;
}

PartModule* PartModuleList::ByPersistentId(uint32_t id) const {
    for (const auto& m : modules_)
        if (m->PersistentId() == id) return m.get();
    return nullptr;
}

std::vector<PartModule*> PartModuleList::GetModules(const std::function<bool(PartModule&)>& is) const {
    std::vector<PartModule*> out;
    for (const auto& m : modules_)
        if (is(*m)) out.push_back(m.get());
    return out;
}

IScalarModule* PartModuleList::GetScalarModule(std::string_view name) const {
    for (const auto& m : modules_) {
        IScalarModule* s = m->AsScalarModule();
        if (s && s->ScalarModuleID() == name) return s;
    }
    return nullptr;
}

std::vector<IScalarModule*> PartModuleList::GetScalarModules(std::string_view name) const {
    std::vector<IScalarModule*> out;
    for (const auto& m : modules_) {
        IScalarModule* s = m->AsScalarModule();
        if (s && s->ScalarModuleID() == name) out.push_back(s);
    }
    return out;
}

// PartModuleList.cs:240 (module.gameObject == part.gameObject: the module belongs to this list's part)
void PartModuleList::Add(std::shared_ptr<PartModule> module) {
    if (std::find(modules_.begin(), modules_.end(), module) == modules_.end()) modules_.push_back(std::move(module));
}

void PartModuleList::Remove(const PartModule* module) {
    auto it = std::find_if(modules_.begin(), modules_.end(), [&](const auto& m) { return m.get() == module; });
    if (it != modules_.end()) modules_.erase(it);
}

int PartModuleList::IndexOf(const PartModule* module) const {
    for (size_t i = 0; i < modules_.size(); i++)
        if (modules_[i].get() == module) return static_cast<int>(i);
    return -1;
}

PartModule* PartModuleList::GetModule(std::string_view moduleName) const {
    for (const auto& m : modules_)
        if (m->moduleName == moduleName) return m.get();
    return nullptr;
}

PartModule* PartModuleList::GetModule(const std::function<bool(PartModule&)>& is, int index) const {
    int n = 0;
    for (const auto& m : modules_)
        if (is(*m)) {
            if (n == index) return m.get();
            n++;
        }
    return nullptr;
}

bool PartModuleList::Contains(int classID) const {
    for (const auto& m : modules_)
        if (m->ClassID() == classID) return true;
    return false;
}
bool PartModuleList::Contains(std::string_view className) const { return Contains(MonoStringHash(className)); }
bool PartModuleList::ContainsPersistentId(uint32_t id) const { return ByPersistentId(id) != nullptr; }

bool PartModuleList::Send(int actionID, BaseEventDetails* data) {
    bool result = false;
    for (const auto& m : modules_)
        if (m->Events().Send(actionID, data)) result = true;
    return result;
}
bool PartModuleList::Send(std::string_view actionName, BaseEventDetails* data) { return Send(MonoStringHash(actionName), data); }
bool PartModuleList::SendDefault(BaseEventDetails* data) {
    bool result = false;
    for (const auto& m : modules_)
        if (m->Events().SendDefault(data)) result = true;
    return result;
}

// ---- ScalarModuleSetHandler ---------------------------------------------------------------------------------------------

void ScalarModuleSetHandler::Load(const ConfigNode& node) {
    std::vector<std::string> values = node.GetValues(nodeName);
    if (!values.empty()) fxModuleNames = std::move(values);
}

void ScalarModuleSetHandler::Save(ConfigNode& node) const {
    if (!fxModuleNames) throw NullReferenceException("Object reference not set to an instance of an object");
    for (const std::string& n : *fxModuleNames) node.AddValue("moduleID", n);
}

void ScalarModuleSetHandler::Initialize(const PartModuleList& modules) {
    fxModules.clear();
    if (!fxModuleNames) throw NullReferenceException("Object reference not set to an instance of an object");
    for (const std::string& n : *fxModuleNames) {
        const auto found = modules.GetScalarModules(dotnet::Trim(n));
        fxModules.insert(fxModules.end(), found.begin(), found.end());
    }
}

void ScalarModuleSetHandler::SetFXModules(float scalar) {
    for (IScalarModule* m : fxModules) m->SetScalar(scalar);
}

// ---- ModuleFactory ------------------------------------------------------------------------------------------------------

namespace detail {
void RegisterBuiltinModules();  // module_behaviours.cpp
}

namespace {
std::map<std::string, ModuleFactory::Creator>& Creators() {
    static std::map<std::string, ModuleFactory::Creator> m;
    return m;
}
void EnsureBuiltins() {
    static bool done = false;
    if (done) return;
    done = true;
    detail::RegisterBuiltinModules();
}
}  // namespace

void ModuleFactory::Register(const std::string& className, Creator creator) { Creators()[className] = std::move(creator); }

bool ModuleFactory::IsRegistered(const std::string& className) {
    EnsureBuiltins();
    return Creators().count(className) != 0;
}

std::shared_ptr<PartModule> ModuleFactory::Create(const ModuleClassInfo& cls) {
    EnsureBuiltins();
    auto it = Creators().find(cls.name);
    if (it != Creators().end()) return it->second(cls);
    return std::make_shared<PartModule>(cls);
}

// ---- Part.GetModuleMass / GetModuleCosts --------------------------------------------------------------------------------

// Part.cs:7532
float PartGetModuleMass(const PartModuleList& modules, float defaultMass, ModifierStagingSituation sit) {
    float num = 0.0f;
    for (const auto& m : modules)
        if (IPartMassModifier* mm = m->AsMassModifier()) num = f32store(fx(num) + fx(mm->GetModuleMass(defaultMass, sit)));
    return num;
}

// Part.cs:7518
float PartGetModuleCosts(const PartModuleList& modules, float defaultCost, ModifierStagingSituation sit) {
    float num = 0.0f;
    for (const auto& m : modules)
        if (IPartCostModifier* cm = m->AsCostModifier()) num = f32store(fx(num) + fx(cm->GetModuleCost(defaultCost, sit)));
    return num;
}

}  // namespace ksp
