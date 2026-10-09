// WP-06 field registry: BaseField / BaseFieldList / BaseAxisField (BaseField.cs, BaseFieldList.cs, BaseAxisField.cs,
// BaseAxisFieldList.cs, BasePAWGroup.cs, FieldAttribute.cs, KSPField.cs, KSPAxisField.cs) and the UI_Control attribute
// data model (UI_Control.cs and subclasses; data only, no UI).
//
// C# keeps the [KSPField] members as real fields of the PartModule / ScenarioModule / VesselModule object and reaches them
// by reflection (FieldInfo). kspcore keeps them as typed values in a per-instance FieldStore, indexed like the class's
// BaseFieldList (ModuleClassInfo::Fields(): the class's own [KSPField]s, then each base class's non-private ones). The
// value types are the ModuleValue alternatives (the C# field type decides parse/format: BaseField.ReadPvt). A field that
// is a C# `float` holds a float32 (stores follow the Mono rule), a `double` a double, and so on.
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <typeinfo>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ksp/config/config_node.hpp"
#include "ksp/modules/cs_expr.hpp"
#include "ksp/modules/module_members.hpp"
#include "ksp/parts/part_module_data.hpp"

namespace ksp {

class BaseField;
class BaseFieldList;

// BasePAWGroup.cs
struct BasePAWGroup {
    std::string name, displayName;
    bool startCollapsed = false;
    BasePAWGroup() = default;
    BasePAWGroup(std::string n, std::string d, bool c) : name(std::move(n)), displayName(std::move(d)), startCollapsed(c) {}
};

// UI_Scene (UI_Scene.cs): Editor = 1, Flight = 2, All = 255
enum class UI_Scene : int { None = 0, Editor = 1, Flight = 2, All = 255 };

// UI_Control.cs and subclasses: the attribute's public fields. `typeName` is the class ("UI_FloatRange"); `params` are the
// fields of the subclasses (minValue, maxValue, stepIncrement, enabledText, ...) in UI_Control's first-the-base order with
// the defaults of the UI_*.cs initialisers, overridden by the attribute's named arguments.
class UI_Control {
public:
    std::string typeName = "UI_Label";
    UI_Scene scene = UI_Scene::All;
    bool controlEnabled = true;
    bool requireFullControl = false;
    UI_Scene affectSymCounterparts = UI_Scene::Editor;
    bool suppressEditorShipModified = false;
    std::vector<std::pair<std::string, cs::CsValue>> params;
    BaseField* field = nullptr;  // UI_Control.Setup

    bool Is(std::string_view name) const;
    const cs::CsValue* Param(std::string_view name) const;
    // new UI_Control subclass instance with the attribute's arguments applied (SetupUIControl copies the attribute's fields)
    static std::shared_ptr<UI_Control> FromAttribute(const UiMemberInfo& info);
    static std::shared_ptr<UI_Control> NewLabel();  // SetupUIControlLabel
    // UI_Control.cs Load / Save (the subclasses' own Load/Save are UI-side and not ported)
    void Load(const ConfigNode& node);
    void Save(ConfigNode& node) const;
};

// KSPField.cs / KSPAxisField.cs / FieldAttribute.cs: the attribute instance with the C# constructor defaults. guiName and
// guiUnits go through Localizer.Format (their setters).
struct KSPFieldAttribute {
    bool isPersistant = false, guiActive = false, guiActiveEditor = false, guiActiveUnfocused = false;
    std::string guiName, guiUnits, guiFormat, category, groupName, groupDisplayName;
    bool advancedTweakable = false, groupStartCollapsed = false;
    float unfocusedRange = 2.0f;
    // KSPAxisField
    bool isAxis = false;
    float minValue = 0.0f, maxValue = 1.0f, incrementalSpeed = 1.0f, incrementalSpeedMultiplier = 0.0f;
    bool ignoreClampWhenIncremental = false, ignoreIncrementByZero = true;
    int64_t axisGroup = 0;
    bool axisIncrementalMode = true;  // KSPAxisMode.Incremental
};
// The attribute record of a schema field (arguments evaluated, guiName/guiUnits localised).
KSPFieldAttribute GetKSPFieldAttribute(const ModuleFieldInfo& info);

// Per-instance KSPField storage (C#: the object's fields, initialised by the class's field initialisers in the
// constructor chain). PartModule, ScenarioModule and VesselModule derive from it.
class FieldStore {
public:
    explicit FieldStore(const ModuleClassInfo& cls);
    virtual ~FieldStore() = default;
    const ModuleClassInfo& Class() const { return *cls_; }
    const std::vector<const ModuleFieldInfo*>& FieldInfos() const { return infos_; }
    size_t FieldCount() const { return values_.size(); }
    int IndexOf(std::string_view name) const;  // BaseFieldList order, -1 when absent
    ModuleValue& ValueAt(size_t i) { return values_[i]; }
    const ModuleValue& ValueAt(size_t i) const { return values_[i]; }
    ModuleValue* Value(std::string_view name);
    const ModuleValue* Value(std::string_view name) const;
    // Typed convenience for module code: throws if the field is absent or of another type.
    template <class T>
    T Get(std::string_view name) const {
        const ModuleValue* v = Value(name);
        const T* p = v ? std::get_if<T>(v) : nullptr;
        if (!p) throw std::runtime_error("FieldStore::Get: " + std::string(name) + " is not a " + typeid(T).name());
        return *p;
    }
    template <class T>
    void Set(std::string_view name, T value) {
        ModuleValue* v = Value(name);
        if (!v) throw std::runtime_error("FieldStore::Set: no field " + std::string(name));
        *v = std::move(value);
    }

private:
    const ModuleClassInfo* cls_;
    std::vector<const ModuleFieldInfo*> infos_;
    std::vector<ModuleValue> values_;
};

// BaseField.cs
class BaseField {
public:
    BaseField(const KSPFieldAttribute& attr, const ModuleFieldInfo& info, FieldStore* host, size_t index);
    virtual ~BaseField() = default;

    const std::string& name() const { return name_; }
    std::string guiName;
    FieldStore* host() const { return host_; }
    const ModuleFieldInfo& FieldInfo() const { return *info_; }
    const KSPFieldAttribute& Attribute() const { return attr_; }
    bool hasInterface() const { return hasInterface_; }
    bool uiControlOnly() const { return uiControlOnly_; }
    const ModuleValue& originalValue() const { return originalValue_; }

    bool isPersistant = false, guiActive = false, guiInteractable = true, guiActiveEditor = false, guiActiveUnfocused = false;
    float guiUnfocusedRange = 2.0f;
    bool guiRemovedIfPinned = false;
    std::string guiUnits, guiFormat, category;
    bool advancedTweakable = false;
    BasePAWGroup group;
    std::shared_ptr<UI_Control> uiControlFlight, uiControlEditor;
    bool WasActiveBeforePartWasAdjusted = false;

    // BaseField.cs:140/156 SetValue / GetValue over the typed store. SetValue rejects a value of the wrong C# type (C#:
    // FieldInfo.SetValue throws, the error is logged, false is returned).
    bool SetValue(const ModuleValue& newValue);
    const ModuleValue& GetValue() const;
    virtual void SetOriginalValue();
    virtual void CopyField(const BaseField& f);  // BaseField.cs:461
    // BaseField.cs:547 Read(value, host) -> ReadPvt (BaseField.cs:712): TryParse per C# type, errors are logged and the field
    // keeps its value; returns whether the value was assigned.
    bool Read(std::string_view value);
    // BaseField.cs:590 (null string -> nullopt). `gui` formats with the localised/gui forms where the C# code does.
    std::optional<std::string> GetStringValue(bool gui) const;
    std::string GuiString() const;  // BaseField.cs:705
    std::vector<std::function<void(const ModuleValue&)>> OnValueModified;  // event Callback<object>

private:
    friend class BaseFieldList;
    std::string name_;
    FieldStore* host_;
    const ModuleFieldInfo* info_;
    KSPFieldAttribute attr_;
    size_t index_;
    bool hasInterface_ = false;
    bool uiControlOnly_ = false;
    ModuleValue originalValue_;
    void SetupUIControls();  // BaseField.cs:469
};

// BaseAxisField.cs (data and the group/serialisation members; CreateAxisList/AddAxis are Part-level, WP-13/16)
class BaseAxisField : public BaseField {
public:
    BaseAxisField(const KSPFieldAttribute& attr, const ModuleFieldInfo& info, FieldStore* host, size_t index);
    static constexpr int64_t MaxAxisGroup = 4096;  // KSPAxisGroup.Custom04
    static constexpr int NumOverrideGroups = 4;    // Vessel.NumOverrideGroups (Vessel.cs:468)
    float minValue, maxValue, incrementalSpeed, incrementalSpeedMultiplier;
    bool ignoreClampWhenIncremental, ignoreIncrementByZero, active = true;
    int64_t defaultAxisGroup, axisGroup, axisIncremental, axisInverted;
    std::vector<int64_t> overrideGroups, overrideIncremental, overrideInverted;
    int64_t GetAxisGroup(int groupOverride) const;                                         // BaseAxisField.cs:88
    void CopyField(const BaseField& f) override;                                           // BaseAxisField.cs:97
    bool IsInGroup(int64_t group, int overrideGroup, bool overrideDefault, bool include) const;  // :116
    bool ContainsNonDefaultAxes() const;                                                   // :130
    void OnLoad(const ConfigNode& node);                                                   // :169
    void OnSave(ConfigNode& node) const;                                                   // :187
    void SetAxis(float axisValue);                                                         // :213
};

// BaseFieldList.cs (BaseFieldList<BaseField, KSPField> and the BaseFieldList subclass)
class BaseFieldList {
public:
    explicit BaseFieldList(FieldStore& host);  // BaseFieldList.cs:128/CreateList :154 (+ AxisGroupsManager.BuildBaseAxisFields)
    size_t Count() const { return fields_.size(); }
    BaseField* operator[](std::string_view fieldName) const;  // first field with the name, nullptr when absent
    BaseField* operator[](size_t index) const { return fields_[index].get(); }
    ModuleValue GetValue(std::string_view fieldName) const;  // monostate when absent
    bool SetValue(std::string_view fieldName, const ModuleValue& value);
    void SetOriginalValue();
    bool ReadValue(std::string_view valueName, std::string_view value);
    // BaseFieldList.cs:293. `trace` (optional) receives one record per assignment BaseField.Read / the IConfigNode Load made,
    // in node order (values, then nodes): what PartModuleData::cfgFields reports.
    void Load(const ConfigNode& node, std::vector<ModuleFieldValue>* trace = nullptr);
    void Save(ConfigNode& node) const;  // BaseFieldList.cs:366
    void Add(std::shared_ptr<BaseField> f) { fields_.push_back(std::move(f)); }
    void AddBaseFieldList(const BaseFieldList& add);
    void RemoveBaseFieldList(const BaseFieldList& remove);
    auto begin() const { return fields_.begin(); }
    auto end() const { return fields_.end(); }

private:
    FieldStore* host_;
    std::vector<std::shared_ptr<BaseField>> fields_;
};

}  // namespace ksp
