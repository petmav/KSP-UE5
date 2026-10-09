#include "ksp/modules/base_event.hpp"

#include <typeinfo>
#include <unordered_map>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/resources/part_resource_library.hpp"

namespace ksp {

namespace {

bool MakingHistory() { return SaveEnvironment::Current().IsExpansionInstalled("MakingHistory"); }

const std::vector<cs::AttributeArg>& CachedArgs(const void* key, const std::string& text) {
    static std::unordered_map<const void*, std::vector<cs::AttributeArg>> cache;
    auto it = cache.find(key);
    if (it == cache.end()) {
        std::vector<cs::AttributeArg> args;
        cs::ParseAttributeArguments(text, args);
        it = cache.emplace(key, std::move(args)).first;
    }
    return it->second;
}

int64_t GroupArg(const cs::CsValue& v, int64_t def) {
    if (v.kind == cs::CsValue::Kind::Int) return v.i;
    if (v.kind == cs::CsValue::Kind::Member) {
        if (const EnumTable* t = FindModuleEnum("KSPActionGroup"))
            for (const auto& [n, val] : t->members)
                if (n == v.s) return val;
    }
    return def;
}

// BaseAction.ParseGroup (BaseAction.cs:139): Enum.Parse, ArgumentException keeps the default
int64_t ParseActionGroup(const std::optional<std::string>& name, int64_t def) {
    if (name && !name->empty()) {
        int64_t v = 0;
        const EnumTable* t = FindModuleEnum("KSPActionGroup");
        if (t && t->TryParse(*name, v)) return v;
    }
    return def;
}

std::string ActionGroupString(int64_t v) {
    const EnumTable* t = FindModuleEnum("KSPActionGroup");
    return t ? t->ToString(v) : std::to_string(v);
}

}  // namespace

// ---- BaseEventDetails ---------------------------------------------------------------------------------------------------

template <class T>
T BaseEventDetails::Get(const std::string& name) const {
    auto it = data_.find(name);
    if (it != data_.end()) return std::any_cast<T>(it->second);
    log::Error("Invalid key: " + name);
    return T{};
}
template int BaseEventDetails::Get<int>(const std::string&) const;
template float BaseEventDetails::Get<float>(const std::string&) const;
template bool BaseEventDetails::Get<bool>(const std::string&) const;
template std::string BaseEventDetails::Get<std::string>(const std::string&) const;

// BaseEventDetails.cs:84-129
int BaseEventDetails::GetInt(const std::string& n) const { return Get<int>(n); }
float BaseEventDetails::GetFloat(const std::string& n) const { return Get<float>(n); }
std::string BaseEventDetails::GetString(const std::string& n) const {
    auto it = data_.find(n);
    if (it != data_.end()) return std::any_cast<std::string>(it->second);
    log::Error("Invalid key: " + n);
    return "";
}
bool BaseEventDetails::GetBool(const std::string& n) const { return Get<bool>(n); }

// ---- attributes ---------------------------------------------------------------------------------------------------------

// KSPEvent.cs:55 constructor defaults + named arguments
KSPEventAttribute MakeKSPEventAttribute(const EventMemberInfo& m) {
    KSPEventAttribute a;
    for (const cs::AttributeArg& arg : CachedArgs(&m, m.args)) {
        const cs::CsValue& v = arg.value;
        const std::string& n = arg.name;
        if (n == "name") a.name = cs::AsString(v, "");
        else if (n == "isDefault") a.isDefault = cs::AsBool(v, false);
        else if (n == "active") a.active = cs::AsBool(v, true);
        else if (n == "guiActive") a.guiActive = cs::AsBool(v, false);
        else if (n == "requireFullControl") a.requireFullControl = cs::AsBool(v, false);
        else if (n == "guiActiveEditor") a.guiActiveEditor = cs::AsBool(v, false);
        else if (n == "guiActiveUncommand") a.guiActiveUncommand = cs::AsBool(v, false);
        else if (n == "guiActiveUnfocused") a.guiActiveUnfocused = cs::AsBool(v, false);
        else if (n == "unfocusedRange") a.unfocusedRange = cs::AsFloat(v, 2.0f);
        else if (n == "externalToEVAOnly") a.externalToEVAOnly = cs::AsBool(v, true);
        else if (n == "guiIcon") a.guiIcon = cs::AsString(v, "");
        else if (n == "guiName") a.guiName = Localizer::Format(cs::AsString(v, ""));  // KSPEvent.cs:46
        else if (n == "category") a.category = cs::AsString(v, "");
        else if (n == "advancedTweakable") a.advancedTweakable = cs::AsBool(v, false);
        else if (n == "isPersistent") a.isPersistent = cs::AsBool(v, false);
        else if (n == "wasActiveBeforePartWasAdjusted") a.wasActiveBeforePartWasAdjusted = cs::AsBool(v, false);
        else if (n == "groupName") a.groupName = cs::AsString(v, "");
        else if (n == "groupDisplayName") a.groupDisplayName = cs::AsString(v, "");
        else if (n == "groupStartCollapsed") a.groupStartCollapsed = cs::AsBool(v, false);
    }
    return a;
}

// KSPAction.cs:21-60: positional (guiName, actionGroup, advancedTweakable, isPersistent) then named arguments
KSPActionAttribute MakeKSPActionAttribute(const ActionMemberInfo& m) {
    KSPActionAttribute a;
    size_t pos = 0;
    for (const cs::AttributeArg& arg : CachedArgs(&m, m.args)) {
        const cs::CsValue& v = arg.value;
        const std::string& n = arg.name;
        if (n.empty()) {
            if (pos == 0) a.guiName = cs::AsString(v, "");
            else if (pos == 1) a.actionGroup = GroupArg(v, 0);
            else if (pos == 2) a.advancedTweakable = cs::AsBool(v, false);
            else if (pos == 3) a.isPersistent = cs::AsBool(v, true);
            pos++;
        } else if (n == "guiName") a.guiName = cs::AsString(v, "");
        else if (n == "requireFullControl") a.requireFullControl = cs::AsBool(v, false);
        else if (n == "advancedTweakable") a.advancedTweakable = cs::AsBool(v, false);
        else if (n == "isPersistent") a.isPersistent = cs::AsBool(v, true);
        else if (n == "wasActiveBeforePartWasAdjusted") a.wasActiveBeforePartWasAdjusted = cs::AsBool(v, false);
        else if (n == "actionGroup") a.actionGroup = GroupArg(v, 0);
        else if (n == "activeEditor") a.activeEditor = cs::AsBool(v, true);
        else if (n == "noLongerAssignable") a.noLongerAssignable = cs::AsBool(v, false);
    }
    return a;
}

// ---- BaseEvent ----------------------------------------------------------------------------------------------------------

BaseEvent::BaseEvent(BaseEventList* parent, std::string name, bool details, const KSPEventAttribute& a)
    : name_(std::move(name)), hasDetails_(details) {
    id_ = MonoStringHash(name_);
    active = a.active;
    guiActive = a.guiActive;
    requireFullControl = a.requireFullControl;
    guiActiveEditor = a.guiActiveEditor;
    guiActiveUncommand = a.guiActiveUncommand;
    guiIcon = a.guiName;
    SetGuiName(a.guiName);
    category = a.category;
    advancedTweakable = a.advancedTweakable;
    guiActiveUnfocused = a.guiActiveUnfocused;
    unfocusedRange = a.unfocusedRange;
    externalToEVAOnly = a.externalToEVAOnly;
    isPersistent = a.isPersistent;
    wasActiveBeforePartWasAdjusted = a.wasActiveBeforePartWasAdjusted;
    listParent = parent;
    group = BasePAWGroup(a.groupName, a.groupDisplayName, a.groupStartCollapsed);
}

void BaseEvent::SetGuiName(const std::string& v) { guiName_ = Localizer::Format(v); }

void BaseEvent::Invoke() {
    if (onEvent) onEvent();
    else if (onDataEvent) onDataEvent(nullptr);
    else log::Error("Action event delegate is null");
}

void BaseEvent::Invoke(BaseEventDetails* data) {
    if (onEvent) onEvent();
    else if (onDataEvent) onDataEvent(data);
    else log::Error("Action event delegate is null");
}

std::string BaseEvent::ToString() const {
    return name_ + " (Active:" + dotnet::FormatBool(active) + ", GUIActive:" + dotnet::FormatBool(guiActive) + ")";
}

// BaseEvent.OnLoad (BaseEvent.cs:168)
void BaseEvent::OnLoad(const ConfigNode& node) {
    auto b = [&](const char* key, bool& dst) {
        if (auto t = node.GetValue(key)) dst = cs::BoolParse(*t);
    };
    b("active", active);
    b("guiActive", guiActive);
    b("guiActiveEditor", guiActiveEditor);
    b("requireFullControl", requireFullControl);
    b("guiActiveUncommand", guiActiveUncommand);
    b("advancedTweakable", advancedTweakable);
    if (auto t = node.GetValue("guiIcon")) guiIcon = *t;
    if (auto t = node.GetValue("guiName")) SetGuiName(*t);
    if (auto t = node.GetValue("category")) category = *t;
    b("guiActiveUnfocused", guiActiveUnfocused);
    if (auto t = node.GetValue("unfocusedRange")) unfocusedRange = cs::FloatParse(*t);
    b("externalToEVAOnly", externalToEVAOnly);
    if (MakingHistory()) b("wasActiveBeforePartWasAdjusted", wasActiveBeforePartWasAdjusted);
}

// BaseEvent.cs:255 (sic: "category" is written from guiIcon)
void BaseEvent::OnSave(ConfigNode& node) const {
    node.AddValue("active", dotnet::FormatBool(active));
    node.AddValue("guiActive", dotnet::FormatBool(guiActive));
    if (requireFullControl) node.AddValue("requireFullControl", dotnet::FormatBool(requireFullControl));
    if (guiActiveEditor) node.AddValue("guiActiveEditor", dotnet::FormatBool(guiActiveEditor));
    node.AddValue("guiActiveUncommand", dotnet::FormatBool(guiActiveUncommand));
    node.AddValue("guiIcon", guiIcon);
    node.AddValue("guiName", guiName_);
    node.AddValue("category", guiIcon);
    node.AddValue("advancedTweakable", advancedTweakable);
    node.AddValue("guiActiveUnfocused", dotnet::FormatBool(guiActiveUnfocused));
    node.AddValue("unfocusedRange", dotnet::FormatSingleG(unfocusedRange));
    node.AddValue("externalToEVAOnly", dotnet::FormatBool(externalToEVAOnly));
    if (MakingHistory()) node.AddValue("wasActiveBeforePartWasAdjusted", dotnet::FormatBool(wasActiveBeforePartWasAdjusted));
}

// ---- BaseEventList ------------------------------------------------------------------------------------------------------

BaseEventList::BaseEventList(PartModule* m, const ModuleClassInfo& cls) : module(m) { CreateDelegates(cls); }
BaseEventList::BaseEventList(const ModuleClassInfo& cls) { CreateDelegates(cls); }

// BaseEventList.cs:135 (ReflectedData :11)
void BaseEventList::CreateDelegates(const ModuleClassInfo& cls) {
    const auto reflected = ReflectedEvents(cls);
    // events without BaseEventDetails first, then the ones with (two lists in ReflectedData)
    for (int pass = 0; pass < 2; pass++)
        for (const EventMemberInfo* e : reflected)
            if (e->hasDetails == (pass == 1)) Add(e->method, e->hasDetails, MakeKSPEventAttribute(*e));
}

// BaseEventList.cs:150
BaseEvent* BaseEventList::Add(const std::string& name, bool hasDetails, const KSPEventAttribute& attr) {
    if (Contains(name)) {
        log::Error("Action '" + name + "' already defined.");
        return nullptr;
    }
    events_.push_back(std::make_shared<BaseEvent>(this, name, hasDetails, attr));
    return events_.back().get();
}

BaseEvent* BaseEventList::operator[](int id) const {
    for (const auto& e : events_)
        if (e->id() == id) return e.get();
    return nullptr;
}
BaseEvent* BaseEventList::operator[](std::string_view name) const { return (*this)[MonoStringHash(name)]; }
bool BaseEventList::Contains(int id) const { return (*this)[id] != nullptr; }
bool BaseEventList::Contains(std::string_view name) const { return Contains(MonoStringHash(name)); }

void BaseEventList::SetDefault(std::string_view name) {
    if (BaseEvent* e = (*this)[name]) defaultEvent_ = e;
}

void BaseEventList::AddBaseEventList(const BaseEventList& add) {
    for (const auto& e : add.events_) events_.push_back(e);
}
void BaseEventList::RemoveBaseEventList(const BaseEventList& remove) {
    for (const auto& e : remove.events_)
        for (size_t i = 0; i < events_.size(); i++)
            if (events_[i] == e) { events_.erase(events_.begin() + static_cast<std::ptrdiff_t>(i)); break; }
}

// BaseEventList.cs:225
bool BaseEventList::Send(std::string_view name, BaseEventDetails* data) {
    if (module && !module->isEnabled) return false;
    for (const auto& e : events_)
        if (e->active && e->name() == name) {
            e->Invoke(data);
            return true;
        }
    return false;
}
bool BaseEventList::Send(int id, BaseEventDetails* data) {
    if (module && !module->isEnabled) return false;
    for (const auto& e : events_)
        if (e->active && e->id() == id) {
            e->Invoke(data);
            return true;
        }
    return false;
}
bool BaseEventList::SendDefault(BaseEventDetails* data) {
    if (module && !module->isEnabled) return false;
    if (defaultEvent_ && defaultEvent_->active) {
        defaultEvent_->Invoke(data);
        return true;
    }
    return false;
}

void BaseEventList::OnLoad(const ConfigNode& node) {
    for (const auto& e : events_)
        if (ConfigNode* n = node.GetNode(e->name())) e->OnLoad(*n);
}
void BaseEventList::OnSave(ConfigNode& node) const {
    for (const auto& e : events_)
        if (e->isPersistent) e->OnSave(*node.AddNode(e->name()));
}

bool BaseEventList::Bind(std::string_view method, BaseEvent::Handler h) {
    BaseEvent* e = (*this)[method];
    if (!e || e->hasDetails()) return false;
    e->onEvent = std::move(h);
    return true;
}
bool BaseEventList::Bind(std::string_view method, BaseEvent::DetailsHandler h) {
    BaseEvent* e = (*this)[method];
    if (!e || !e->hasDetails()) return false;
    e->onDataEvent = std::move(h);
    return true;
}

// ---- BaseAction ---------------------------------------------------------------------------------------------------------

BaseAction::BaseAction(BaseActionList* parent, std::string n, const KSPActionAttribute& a) : name(std::move(n)) {
    listParent = parent;
    defaultActionGroup = a.actionGroup;
    actionGroup = a.actionGroup;
    SetGuiName(a.guiName);
    requireFullControl = a.requireFullControl;
    overrideGroups.assign(BaseAxisField::NumOverrideGroups, 0);
    active = true;
    advancedTweakable = a.advancedTweakable;
    isPersistent = a.isPersistent;
    wasActiveBeforePartWasAdjusted = a.wasActiveBeforePartWasAdjusted;
    activeEditor = a.activeEditor;
    noLongerAssignable = a.noLongerAssignable;
}

void BaseAction::SetGuiName(const std::string& v) { guiName_ = Localizer::Format(v); }

// BaseAction.cs:37
int BaseAction::ActionGroupsLength() {
    const EnumTable* t = FindModuleEnum("KSPActionGroup");
    return t ? static_cast<int>(t->members.size()) - 2 : 0;
}

int64_t BaseAction::GetActionGroup(int groupOverride) const {
    if (groupOverride > 0 && groupOverride <= static_cast<int>(overrideGroups.size())) return overrideGroups[static_cast<size_t>(groupOverride - 1)];
    return actionGroup;
}

void BaseAction::CopyAction(const BaseAction& a) {
    actionGroup = a.actionGroup;
    for (size_t n = overrideGroups.size(); n-- > 0;) overrideGroups[n] = a.overrideGroups[n];
}

bool BaseAction::ContainsNonDefaultActions() const {
    if (actionGroup != defaultActionGroup) return true;
    for (int64_t g : overrideGroups)
        if (g != 0) return true;
    return false;
}

void BaseAction::Invoke(KSPActionParam& param) {
    if (onEvent) onEvent(param);
    else log::Error("Action event delegate is null");
}

// BaseAction.cs:150
void BaseAction::OnLoad(const ConfigNode& node) {
    actionGroup = ParseActionGroup(node.GetValue("actionGroup"), defaultActionGroup);
    for (size_t i = 0; i < overrideGroups.size(); i++)
        overrideGroups[i] = ParseActionGroup(node.GetValue("overrideGroup" + std::to_string(i)), 0);
    if (auto t = node.GetValue("active")) {
        bool r = true;
        if (cs::BoolTryParse(*t, r)) active = r;
    }
    if (MakingHistory())
        if (auto t = node.GetValue("wasActiveBeforePartWasAdjusted")) wasActiveBeforePartWasAdjusted = cs::BoolParse(*t);
}

// BaseAction.cs:183
void BaseAction::OnSave(ConfigNode& node) const {
    node.AddValue("actionGroup", ActionGroupString(actionGroup));
    if (isPersistent && !active) node.AddValue("active", dotnet::FormatBool(false));
    for (size_t i = 0; i < overrideGroups.size(); i++)
        if (overrideGroups[i] != 0) node.AddValue("overrideGroup" + std::to_string(i), ActionGroupString(overrideGroups[i]));
    if (MakingHistory()) node.AddValue("wasActiveBeforePartWasAdjusted", dotnet::FormatBool(wasActiveBeforePartWasAdjusted));
}

// ---- BaseActionList -----------------------------------------------------------------------------------------------------

BaseActionList::BaseActionList(PartModule* m, const ModuleClassInfo& cls) : module(m) { ReflectActions(cls); }
BaseActionList::BaseActionList(const ModuleClassInfo& cls) { ReflectActions(cls); }

// BaseActionList.cs:79 ReflectActions (ReflectedData :11)
void BaseActionList::ReflectActions(const ModuleClassInfo& cls) {
    for (const ActionMemberInfo* a : ReflectedActions(cls))
        actions_.push_back(std::make_shared<BaseAction>(this, a->method, MakeKSPActionAttribute(*a)));
}

BaseAction* BaseActionList::operator[](std::string_view name) const {
    for (const auto& a : actions_)
        if (a->name == name) return a.get();
    return nullptr;
}

bool BaseActionList::Contains(int64_t group, int groupOverride) const {
    for (const auto& a : actions_)
        if ((a->GetActionGroup(groupOverride) & group) != 0) return true;
    return false;
}

void BaseActionList::OnLoad(const ConfigNode& node) {
    for (const auto& a : actions_)
        if (ConfigNode* n = node.GetNode(a->name)) a->OnLoad(*n);
}
void BaseActionList::OnSave(ConfigNode& node) const {
    for (const auto& a : actions_) a->OnSave(*node.AddNode(a->name));
}

bool BaseActionList::Bind(std::string_view method, BaseAction::Handler h) {
    bool any = false;
    for (const auto& a : actions_)
        if (a->name == method) {
            a->onEvent = h;
            any = true;
        }
    return any;
}

}  // namespace ksp
