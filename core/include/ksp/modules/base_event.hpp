// WP-06 event / action registry: BaseEvent, BaseEventList, BaseEventDetails, BaseAction, BaseActionList
// (BaseEvent.cs, BaseEventList.cs, BaseEventDetails.cs, BaseAction.cs, BaseActionList.cs, KSPEvent.cs, KSPAction.cs).
//
// C# binds each [KSPEvent] / [KSPAction] method of the module to a delegate by reflection (Delegate.CreateDelegate on the
// method name). kspcore keeps the attribute data from the generated registry (module_members.inc) and a handler slot per
// entry: the owning module package binds the C++ method with BindEvent / BindAction (PartModule), exactly as the delegate
// would be. An unbound event reports "Action event delegate is null" when invoked, like a null delegate.
#pragma once

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/modules/base_field.hpp"
#include "ksp/resources/part_resource_library.hpp"

namespace ksp {

class PartModule;

// BaseEventDetails.cs (Hashtable: Add throws on a duplicate key)
class BaseEventDetails {
public:
    enum class Sender { AUTO, STAGING, GUI, USER };
    explicit BaseEventDetails(Sender) {}
    template <class T>
    void Set(const std::string& name, T value) {
        if (!data_.emplace(name, std::any(std::move(value))).second)
            throw std::runtime_error("Item has already been added. Key in dictionary: '" + name + "'");
    }
    template <class T>
    T Get(const std::string& name) const;
    bool Has(const std::string& name) const { return data_.count(name) != 0; }
    int GetInt(const std::string& name) const;
    float GetFloat(const std::string& name) const;
    std::string GetString(const std::string& name) const;
    bool GetBool(const std::string& name) const;
    size_t Count() const { return data_.size(); }

private:
    std::map<std::string, std::any> data_;
};

// KSPEvent.cs: the attribute instance (constructor defaults, then the named arguments)
struct KSPEventAttribute {
    std::string name;
    bool isDefault = false, active = true, guiActive = false, requireFullControl = false, guiActiveEditor = false;
    bool guiActiveUncommand = false, guiActiveUnfocused = false, externalToEVAOnly = true, advancedTweakable = false;
    bool isPersistent = false, wasActiveBeforePartWasAdjusted = false, groupStartCollapsed = false;
    float unfocusedRange = 2.0f;
    std::string guiIcon, guiName, category, groupName, groupDisplayName;
};
KSPEventAttribute MakeKSPEventAttribute(const EventMemberInfo& m);

// KSPAction.cs: constructors (guiName[, actionGroup[, advancedTweakable[, isPersistent]]]) and named arguments
struct KSPActionAttribute {
    std::string guiName;
    bool requireFullControl = false, advancedTweakable = false, isPersistent = true, wasActiveBeforePartWasAdjusted = false;
    int64_t actionGroup = 0;  // KSPActionGroup
    bool activeEditor = true, noLongerAssignable = false;
};
KSPActionAttribute MakeKSPActionAttribute(const ActionMemberInfo& m);

class BaseEventList;

// BaseEvent.cs
class BaseEvent {
public:
    using Handler = std::function<void()>;
    using DetailsHandler = std::function<void(BaseEventDetails*)>;
    BaseEvent(BaseEventList* listParent, std::string name, bool hasDetails, const KSPEventAttribute& attr);  // :73/:96
    const std::string& name() const { return name_; }
    int id() const { return id_; }  // name.GetHashCode()
    bool active, guiActive, requireFullControl, guiActiveEditor, guiActiveUncommand;
    std::string guiIcon, category;
    bool guiActiveUnfocused;
    float unfocusedRange;
    bool externalToEVAOnly, advancedTweakable, isPersistent, wasActiveBeforePartWasAdjusted, assigned = false;
    BaseEventList* listParent;
    BasePAWGroup group;
    const std::string& guiName() const { return guiName_; }
    void SetGuiName(const std::string& v);  // Localizer.Format(value)
    std::string GUIName() const { return guiName_.empty() ? name_ : guiName_; }
    bool EventIsDisabledByVariant() const { return eventIsDisabledByVariant_; }
    void VariantToggleEventDisabled(bool disabled) { eventIsDisabledByVariant_ = disabled; }
    bool hasDetails() const { return hasDetails_; }
    Handler onEvent;
    DetailsHandler onDataEvent;
    void Invoke();                     // :134
    void Invoke(BaseEventDetails* d);  // :151
    std::string ToString() const;      // :168
    void OnLoad(const ConfigNode& node);  // :173
    void OnSave(ConfigNode& node) const;  // :255

private:
    std::string name_, guiName_;
    int id_;
    bool hasDetails_;
    bool eventIsDisabledByVariant_ = false;
};

// BaseEventList.cs
class BaseEventList {
public:
    // :114 BaseEventList(Part, PartModule)
    BaseEventList(PartModule* module, const ModuleClassInfo& cls);
    // :104 BaseEventList(object) (ScenarioModule / VesselModule hosts: no module for the isEnabled check)
    explicit BaseEventList(const ModuleClassInfo& cls);
    PartModule* module = nullptr;
    size_t Count() const { return events_.size(); }
    BaseEvent* operator[](std::string_view name) const;  // by GetHashCode id
    BaseEvent* operator[](int id) const;
    BaseEvent* GetByIndex(size_t i) const { return events_[i].get(); }
    bool Contains(std::string_view name) const;
    bool Contains(int id) const;
    void SetDefault(std::string_view name);
    void ClearDefault() { defaultEvent_ = nullptr; }
    bool Send(std::string_view name, BaseEventDetails* data = nullptr);  // :225
    bool Send(int id, BaseEventDetails* data = nullptr);
    bool SendDefault(BaseEventDetails* data = nullptr);
    void OnLoad(const ConfigNode& node);
    void OnSave(ConfigNode& node) const;
    void AddBaseEventList(const BaseEventList& add);
    void RemoveBaseEventList(const BaseEventList& remove);
    // Bind the C++ implementation of a [KSPEvent] method (the delegate Delegate.CreateDelegate made)
    bool Bind(std::string_view method, BaseEvent::Handler h);
    bool Bind(std::string_view method, BaseEvent::DetailsHandler h);
    auto begin() const { return events_.begin(); }
    auto end() const { return events_.end(); }

private:
    void CreateDelegates(const ModuleClassInfo& cls);  // :135
    BaseEvent* Add(const std::string& name, bool hasDetails, const KSPEventAttribute& attr);  // :150
    std::vector<std::shared_ptr<BaseEvent>> events_;
    BaseEvent* defaultEvent_ = nullptr;
};

// KSPActionType.cs
enum class KSPActionType : int { Activate, Deactivate, Toggle };

// KSPActionParam.cs:3 (WP-16 completed it): the delegate argument of an action. `group` (a KSPActionGroup value) and `type`
// (a KSPActionType value) are plain fields here (the C# has private setters); `Cooldown` only ever grows (Mathf.Max).
struct KSPActionParam {
    int64_t group = 0;
    int type = 0;  // KSPActionType.Activate
    KSPActionParam() = default;
    // KSPActionParam.cs:21
    KSPActionParam(int64_t actionGroup, KSPActionType actionType) : group(actionGroup), type(static_cast<int>(actionType)) {}
    // KSPActionParam.cs:17 get / set (`_cooldown = Mathf.Max(_cooldown, value)`)
    float Cooldown() const { return cooldown_; }
    void SetCooldown(float value) { cooldown_ = value > cooldown_ ? value : cooldown_; }
    KSPActionType Type() const { return static_cast<KSPActionType>(type); }

private:
    float cooldown_ = 0.0f;
};

class BaseActionList;
class Part;

// BaseAction.cs
class BaseAction {
public:
    using Handler = std::function<void(KSPActionParam&)>;
    BaseAction(BaseActionList* listParent, std::string name, const KSPActionAttribute& attr);  // :55
    std::string name;
    int64_t defaultActionGroup, actionGroup;
    std::vector<int64_t> overrideGroups;
    bool active = true, activeEditor, requireFullControl, advancedTweakable, isPersistent, wasActiveBeforePartWasAdjusted,
        noLongerAssignable;
    BaseActionList* listParent;
    const std::string& guiName() const { return guiName_; }
    void SetGuiName(const std::string& v);
    Handler onEvent;
    static int ActionGroupsLength();                 // :37 Enum.GetNames(KSPActionGroup).Length - 2
    // ---- WP-16 (BaseAction.cs statics over a part list; Part.Actions (the Part's own [KSPAction]s) is not modelled by WP-13, so
    // only the modules' actions are listed; GameSettings.ADVANCED_TWEAKABLES is vessel.hpp's GameSettings namespace) ----
    static int GetGroupIndex(int64_t group);         // :394 (0 when `group` is not a single flag below ActionGroupsLength)
    static std::vector<BaseAction*> CreateActionList(const std::vector<Part*>& parts, int64_t group, int overrideGroup, bool overrideDefault, bool include);  // :205
    static std::vector<BaseAction*> CreateActionList(Part& p, int64_t group, int overrideGroup, bool overrideDefault, bool include);  // :236
    static std::vector<bool> CreateGroupList(Part& p, int overrideGroup = 0);      // :267
    static std::vector<bool> CreateGroupList(const std::vector<Part*>& parts, int groupOverride = 0);  // :308
    static bool ContainsNonDefaultActions(Part& p);                                 // :346
    // :379 (InputLockManager.IsLocked(TWEAKABLES_FULLONLY) = `fullOnlyLocked`, HighLogic.LoadedSceneIsEditor = `editorScene`)
    static void FireAction(const std::vector<Part*>& parts, int64_t group, int overrideGroup, KSPActionType type, bool fullOnlyLocked = false, bool editorScene = false);
    int64_t GetActionGroup(int groupOverride) const;  // :73
    void CopyAction(const BaseAction& a);             // :82
    bool ContainsNonDefaultActions() const;           // :93
    void Invoke(KSPActionParam& param);               // :115
    void OnLoad(const ConfigNode& node);              // :150
    void OnSave(ConfigNode& node) const;              // :183

private:
    std::string guiName_;
};

// BaseActionList.cs
class BaseActionList {
public:
    static constexpr int64_t ControlAction = 1 | 8 | 16;  // Stage | RCS | SAS
    BaseActionList(PartModule* module, const ModuleClassInfo& cls);
    explicit BaseActionList(const ModuleClassInfo& cls);
    PartModule* module = nullptr;
    size_t Count() const { return actions_.size(); }
    BaseAction* operator[](std::string_view name) const;  // first with the name
    BaseAction* operator[](size_t i) const { return actions_[i].get(); }
    bool Contains(int64_t group, int groupOverride = 0) const;  // BaseActionList.cs:142
    void OnLoad(const ConfigNode& node);
    void OnSave(ConfigNode& node) const;
    bool Bind(std::string_view method, BaseAction::Handler h);  // every action of that method
    auto begin() const { return actions_.begin(); }
    auto end() const { return actions_.end(); }

private:
    void ReflectActions(const ModuleClassInfo& cls);
    std::vector<std::shared_ptr<BaseAction>> actions_;
};

}  // namespace ksp
