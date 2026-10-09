#include "ksp/modules/base_field.hpp"

#include <algorithm>
#include <mutex>
#include <unordered_map>

#include "ksp/core/log.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

namespace {

// Parsed attribute argument lists, cached per schema field (the text never changes).
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

int64_t EnumArg(const cs::CsValue& v, const EnumTable* table, int64_t def) {
    if (v.kind == cs::CsValue::Kind::Int) return v.i;
    if (v.kind == cs::CsValue::Kind::Member && table) {
        for (const auto& [n, val] : table->members)
            if (n == v.s) return val;
    }
    return def;
}

std::string ParseFailureText(std::string_view typeName) {
    if (typeName == "Boolean") return "boolean";
    if (typeName == "Byte") return "byte";
    if (typeName == "UInt16") return "ushort";
    if (typeName == "Char") return "char";
    if (typeName == "UInt64") return "ulong";
    if (typeName == "Int32") return "integer";
    if (typeName == "Int64") return "long";
    if (typeName == "Single") return "float";
    if (typeName == "UInt32") return "uint";
    if (typeName == "Int16") return "short";
    if (typeName == "Double") return "double";
    return "";
}

}  // namespace

// ---- UI_Control ---------------------------------------------------------------------------------------------------------

bool UI_Control::Is(std::string_view name) const {
    for (const UiClassInfo* c = FindUiClass(typeName); c; c = FindUiClass(c->base))
        if (c->name == name) return true;
    return false;
}

const cs::CsValue* UI_Control::Param(std::string_view name) const {
    for (const auto& [n, v] : params)
        if (n == name) return &v;
    return nullptr;
}

// BaseField.cs:517 SetupUIControl: Activator.CreateInstance(attr.GetType()) + copy of every public field of the attribute
std::shared_ptr<UI_Control> UI_Control::FromAttribute(const UiMemberInfo& info) {
    auto c = std::make_shared<UI_Control>();
    c->typeName = info.uiType;
    // class chain base-first: UI_Control's own fields are the typed members above; subclasses' fields are params
    std::vector<const UiClassInfo*> chain;
    for (const UiClassInfo* k = FindUiClass(info.uiType); k; k = FindUiClass(k->base)) chain.push_back(k);
    std::reverse(chain.begin(), chain.end());
    for (const UiClassInfo* k : chain) {
        if (k->name == "UI_Control") continue;
        for (const UiClassField& f : k->fields) {
            cs::CsValue v;
            if (!f.initializer.empty()) cs::EvaluateExpression(f.initializer, v);
            c->params.emplace_back(f.name, v);
        }
    }
    const std::string key = "UI|" + info.uiType + "|" + info.args;
    static std::unordered_map<std::string, std::vector<cs::AttributeArg>> cache;
    auto it = cache.find(key);
    if (it == cache.end()) {
        std::vector<cs::AttributeArg> args;
        cs::ParseAttributeArguments(info.args, args);
        it = cache.emplace(key, std::move(args)).first;
    }
    for (const cs::AttributeArg& a : it->second) {
        if (a.name == "scene") c->scene = static_cast<UI_Scene>(EnumArg(a.value, &UiSceneEnum(), 255));
        else if (a.name == "controlEnabled") c->controlEnabled = cs::AsBool(a.value, true);
        else if (a.name == "requireFullControl") c->requireFullControl = cs::AsBool(a.value, false);
        else if (a.name == "affectSymCounterparts") c->affectSymCounterparts = static_cast<UI_Scene>(EnumArg(a.value, &UiSceneEnum(), 1));
        else if (a.name == "suppressEditorShipModified") c->suppressEditorShipModified = cs::AsBool(a.value, false);
        else {
            bool found = false;
            for (auto& [n, v] : c->params)
                if (n == a.name) { v = a.value; found = true; }
            if (!found) c->params.emplace_back(a.name, a.value);
        }
    }
    return c;
}

// BaseField.cs:536
std::shared_ptr<UI_Control> UI_Control::NewLabel() {
    auto c = std::make_shared<UI_Control>();
    c->typeName = "UI_Label";
    return c;
}

// UI_Control.cs:Load / ParseEnabled (UI_Control.cs:65)
void UI_Control::Load(const ConfigNode& node) {
    controlEnabled = true;
    if (auto v = node.GetValue("controlEnabled")) {
        bool r = true;
        if (cs::BoolTryParse(*v, r)) controlEnabled = r;
        else {
            controlEnabled = true;
            log::Error("FieldUIControl(" + (field ? field->name() : std::string()) + "): 'enabled' cannot be parsed");
        }
    }
}

// UI_Control.cs:Save
void UI_Control::Save(ConfigNode& node) const { node.AddValue("controlEnabled", dotnet::FormatBool(controlEnabled)); }

// ---- KSPField attribute -------------------------------------------------------------------------------------------------

// FieldAttribute.cs / KSPField.cs:42 / KSPAxisField.cs:22 (the constructors' defaults, then the named arguments)
KSPFieldAttribute GetKSPFieldAttribute(const ModuleFieldInfo& info) {
    KSPFieldAttribute a;
    a.isPersistant = info.isPersistant;
    const FieldMemberInfo* m = FindFieldMember(info);
    if (!m) return a;
    a.isAxis = m->isAxis;
    for (const cs::AttributeArg& arg : CachedArgs(&info, m->args)) {
        const cs::CsValue& v = arg.value;
        const std::string& n = arg.name;
        if (n == "isPersistant") a.isPersistant = cs::AsBool(v, a.isPersistant);
        else if (n == "guiActive") a.guiActive = cs::AsBool(v, false);
        else if (n == "guiActiveEditor") a.guiActiveEditor = cs::AsBool(v, false);
        else if (n == "guiActiveUnfocused") a.guiActiveUnfocused = cs::AsBool(v, false);
        else if (n == "guiName") a.guiName = Localizer::Format(cs::AsString(v, ""));    // FieldAttribute.cs:15
        else if (n == "guiUnits") a.guiUnits = Localizer::Format(cs::AsString(v, ""));  // KSPField.cs:34
        else if (n == "guiFormat") a.guiFormat = cs::AsString(v, "");
        else if (n == "category") a.category = cs::AsString(v, "");
        else if (n == "advancedTweakable") a.advancedTweakable = cs::AsBool(v, false);
        else if (n == "unfocusedRange") a.unfocusedRange = cs::AsFloat(v, 2.0f);
        else if (n == "groupName") a.groupName = cs::AsString(v, "");
        else if (n == "groupDisplayName") a.groupDisplayName = cs::AsString(v, "");
        else if (n == "groupStartCollapsed") a.groupStartCollapsed = cs::AsBool(v, false);
        else if (n == "minValue") a.minValue = cs::AsFloat(v, 0.0f);
        else if (n == "maxValue") a.maxValue = cs::AsFloat(v, 1.0f);
        else if (n == "incrementalSpeed") a.incrementalSpeed = cs::AsFloat(v, 1.0f);
        else if (n == "incrementalSpeedMultiplier") a.incrementalSpeedMultiplier = cs::AsFloat(v, 0.0f);
        else if (n == "ignoreClampWhenIncremental") a.ignoreClampWhenIncremental = cs::AsBool(v, false);
        else if (n == "ignoreIncrementByZero") a.ignoreIncrementByZero = cs::AsBool(v, true);
        else if (n == "axisGroup") a.axisGroup = EnumArg(v, FindModuleEnum("KSPAxisGroup"), 0);
        else if (n == "axisMode") a.axisIncrementalMode = !(v.kind == cs::CsValue::Kind::Member && v.s != "Incremental");
    }
    return a;
}

// ---- FieldStore ---------------------------------------------------------------------------------------------------------

FieldStore::FieldStore(const ModuleClassInfo& cls) : cls_(&cls), infos_(cls.Fields()) {
    values_.reserve(infos_.size());
    for (const ModuleFieldInfo* f : infos_) values_.push_back(cs::FieldInitialValue(*f));
}

int FieldStore::IndexOf(std::string_view name) const {
    for (size_t i = 0; i < infos_.size(); i++)
        if (name == infos_[i]->name) return static_cast<int>(i);
    return -1;
}

ModuleValue* FieldStore::Value(std::string_view name) {
    const int i = IndexOf(name);
    return i < 0 ? nullptr : &values_[static_cast<size_t>(i)];
}
const ModuleValue* FieldStore::Value(std::string_view name) const {
    const int i = IndexOf(name);
    return i < 0 ? nullptr : &values_[static_cast<size_t>(i)];
}

// ---- BaseField ----------------------------------------------------------------------------------------------------------

// BaseField.cs:409 (KSPField) with BaseField<K>.cs:125
BaseField::BaseField(const KSPFieldAttribute& attr, const ModuleFieldInfo& info, FieldStore* host, size_t index)
    : host_(host), info_(&info), attr_(attr), index_(index) {
    name_ = info.name;
    hasInterface_ = IsConfigNodeFieldType(info);
    isPersistant = attr.isPersistant;
    guiInteractable = true;
    guiActive = attr.guiActive;
    guiActiveEditor = attr.guiActiveEditor;
    guiActiveUnfocused = attr.guiActiveUnfocused;
    guiUnfocusedRange = attr.unfocusedRange;
    guiName = attr.guiName;
    guiUnits = attr.guiUnits;
    guiFormat = attr.guiFormat;
    category = attr.category;
    advancedTweakable = attr.advancedTweakable;
    group = BasePAWGroup(attr.groupName, attr.groupDisplayName, attr.groupStartCollapsed);
    SetupUIControls();
}

// BaseField.cs:469
void BaseField::SetupUIControls() {
    const FieldMemberInfo* m = FindFieldMember(*info_);
    if (m) {
        for (const UiMemberInfo& ui : m->ui) {
            std::shared_ptr<UI_Control> c = UI_Control::FromAttribute(ui);
            if ((static_cast<int>(c->scene) & static_cast<int>(UI_Scene::Flight)) != 0) {
                if (!uiControlFlight) {
                    uiControlFlight = c;
                    c->field = this;
                } else {
                    log::Error("BaseField " + name_ + " in " + host_->Class().name + " has too many UICtrls marked for Flight. Max == 1.");
                }
            }
            if ((static_cast<int>(c->scene) & static_cast<int>(UI_Scene::Editor)) != 0) {
                if (!uiControlEditor) {
                    uiControlEditor = UI_Control::FromAttribute(ui);
                    uiControlEditor->field = this;
                    guiActiveEditor = true;
                    continue;
                }
                log::Error("BaseField " + name_ + " in " + host_->Class().name + " has too many UICtrls marked for Editor. Max == 1.");
            }
        }
    }
    if (!uiControlEditor) {
        uiControlEditor = UI_Control::NewLabel();
        uiControlEditor->field = this;
    }
    if (!uiControlFlight) {
        uiControlFlight = UI_Control::NewLabel();
        uiControlFlight->field = this;
    }
}

namespace {
// FieldInfo.SetValue accepts a value of the field's own C# type (or null for a reference type)
bool ValueMatchesField(const ModuleValue& v, const ModuleFieldInfo& info) {
    const std::string_view t = info.typeName;
    if (info.enumTable) return std::holds_alternative<EnumValue>(v);
    if (std::holds_alternative<std::monostate>(v)) return t == "String" || t == "FloatCurve" || t == "DoubleCurve";
    if (t == "Boolean") return std::holds_alternative<bool>(v);
    if (t == "Byte") return std::holds_alternative<uint8_t>(v);
    if (t == "UInt16") return std::holds_alternative<uint16_t>(v);
    if (t == "Char") return std::holds_alternative<char16_t>(v);
    if (t == "UInt64") return std::holds_alternative<uint64_t>(v);
    if (t == "Int32") return std::holds_alternative<int32_t>(v);
    if (t == "Int64") return std::holds_alternative<int64_t>(v);
    if (t == "Single") return std::holds_alternative<float>(v);
    if (t == "UInt32") return std::holds_alternative<uint32_t>(v);
    if (t == "Int16") return std::holds_alternative<int16_t>(v);
    if (t == "Double") return std::holds_alternative<double>(v);
    if (t == "String") return std::holds_alternative<std::string>(v);
    if (t == "Vector2") return std::holds_alternative<Vector2f>(v);
    if (t == "Vector3") return std::holds_alternative<Vector3f>(v);
    if (t == "Vector4") return std::holds_alternative<Vector4f>(v);
    if (t == "Quaternion") return std::holds_alternative<Quaternionf>(v);
    if (t == "FloatCurve") return std::holds_alternative<FloatCurve>(v);
    if (t == "DoubleCurve") return std::holds_alternative<DoubleCurve>(v);
    return false;
}
}  // namespace

// BaseField.cs:140
bool BaseField::SetValue(const ModuleValue& newValue) {
    if (!ValueMatchesField(newValue, *info_)) {
        log::Error("Value could not be set to field '" + name_ + "'");
        return false;
    }
    host_->ValueAt(index_) = newValue;
    for (auto& cb : OnValueModified) cb(newValue);
    return true;
}

// BaseField.cs:156
const ModuleValue& BaseField::GetValue() const { return host_->ValueAt(index_); }

// BaseField.cs:174
void BaseField::SetOriginalValue() { originalValue_ = host_->ValueAt(index_); }

// BaseField.cs:461
void BaseField::CopyField(const BaseField& f) {
    if (isPersistant) SetValue(f.GetValue());
}

// BaseField.cs:547 / :712 ReadPvt
bool BaseField::Read(std::string_view value) {
    ModuleValue parsed;
    if (ReadModuleFieldValue(*info_, value, parsed)) return SetValue(parsed);
    const std::string what = ParseFailureText(info_->typeName);
    if (!what.empty())
        log::Error("Invalid " + what + " value! Field " + name_ + ", value " + std::string(value) + " on object of type " +
                   host_->Class().name);
    else if (std::string_view(info_->typeName).rfind("Vector", 0) == 0 || info_->typeName == std::string("Quaternion"))
        log::Error(std::string("Invalid value length for ") + info_->typeName + "! Field " + name_ + ", value " + std::string(value) +
                   " on object of type " + host_->Class().name);
    return false;
}

// BaseField.cs:590
std::optional<std::string> BaseField::GetStringValue(bool gui) const {
    const ModuleValue& v = GetValue();
    if (std::holds_alternative<std::monostate>(v)) return std::nullopt;
    const std::string_view t = info_->typeName;
    auto g9 = [](float f) { return dotnet::FormatSingleG(f, 9); };
    if (const bool* b = std::get_if<bool>(&v)) {
        if (gui) return Localizer::Format(*b ? "#autoLOC_8004438" : "#autoLOC_8004439");
        return dotnet::FormatBool(*b);
    }
    if (const int32_t* i = std::get_if<int32_t>(&v)) {
        if (gui && !guiFormat.empty()) return KSPUtil::LocalizeNumber(static_cast<double>(*i), guiFormat);
        return dotnet::FormatInt64(*i);
    }
    if (const float* f = std::get_if<float>(&v)) {
        if (gui) return guiFormat.empty() ? dotnet::FormatSingleG(*f) : KSPUtil::LocalizeNumber(*f, guiFormat);
        return g9(*f);
    }
    if (const double* d = std::get_if<double>(&v)) {
        if (gui) return guiFormat.empty() ? dotnet::FormatDoubleG(*d) : KSPUtil::LocalizeNumber(*d, guiFormat);
        return dotnet::FormatDoubleG(*d, 17);
    }
    auto vec = [&](std::initializer_list<float> c) {
        std::string s = "(";
        bool first = true;
        for (float x : c) {
            if (!first) s += ", ";
            first = false;
            s += gui ? dotnet::FormatSingleG(x) : g9(x);
        }
        return s + ")";
    };
    if (const Vector2f* x = std::get_if<Vector2f>(&v)) return vec({x->x, x->y});
    if (const Vector3f* x = std::get_if<Vector3f>(&v)) return vec({x->x, x->y, x->z});
    if (const Vector4f* x = std::get_if<Vector4f>(&v)) return vec({x->x, x->y, x->z, x->w});
    if (const Quaternionf* x = std::get_if<Quaternionf>(&v)) return vec({x->x, x->y, x->z, x->w});
    if (const EnumValue* e = std::get_if<EnumValue>(&v)) return e->table->ToString(e->value);
    if (const uint32_t* u = std::get_if<uint32_t>(&v)) return dotnet::FormatUInt64(*u);
    if (const uint8_t* u = std::get_if<uint8_t>(&v)) return dotnet::FormatUInt64(*u);
    if (const uint16_t* u = std::get_if<uint16_t>(&v)) return dotnet::FormatUInt64(*u);
    if (const uint64_t* u = std::get_if<uint64_t>(&v)) return dotnet::FormatUInt64(*u);
    if (const int64_t* i = std::get_if<int64_t>(&v)) return dotnet::FormatInt64(*i);
    if (const int16_t* i = std::get_if<int16_t>(&v)) return dotnet::FormatInt64(*i);
    if (const std::string* s = std::get_if<std::string>(&v)) return *s;
    (void)t;
    return std::nullopt;  // IConfigNode types: (string)value would throw in C#; Save handles them via hasInterface
}

// BaseField.cs:705
std::string BaseField::GuiString() const {
    std::string text = !guiName.empty() ? guiName : name_;
    return text + ": " + GetStringValue(true).value_or("") + guiUnits;
}

// ---- BaseAxisField ------------------------------------------------------------------------------------------------------

// BaseAxisField.cs:57
BaseAxisField::BaseAxisField(const KSPFieldAttribute& attr, const ModuleFieldInfo& info, FieldStore* host, size_t index)
    : BaseField(attr, info, host, index) {
    minValue = attr.minValue;
    maxValue = attr.maxValue;
    incrementalSpeed = attr.incrementalSpeed;
    incrementalSpeedMultiplier = attr.incrementalSpeedMultiplier;
    defaultAxisGroup = attr.axisGroup;
    axisGroup = attr.axisGroup;
    active = true;
    if (attr.axisIncrementalMode) axisIncremental = (MaxAxisGroup << 1) - 1;
    else axisIncremental = 0;
    axisIncremental &= ~int64_t{64};  // ~KSPAxisGroup.MainThrottle
    axisInverted = 0;
    overrideGroups.assign(NumOverrideGroups, 0);
    overrideIncremental.assign(NumOverrideGroups, axisIncremental);
    overrideInverted.assign(NumOverrideGroups, 0);
    ignoreClampWhenIncremental = attr.ignoreClampWhenIncremental;
    ignoreIncrementByZero = attr.ignoreIncrementByZero;
}

int64_t BaseAxisField::GetAxisGroup(int groupOverride) const {
    if (groupOverride > 0 && groupOverride <= static_cast<int>(overrideGroups.size())) return overrideGroups[static_cast<size_t>(groupOverride - 1)];
    return axisGroup;
}

// BaseAxisField.cs:97 (sic: overrideIncremental is copied from the source's overrideInverted)
void BaseAxisField::CopyField(const BaseField& f) {
    BaseField::CopyField(f);
    const auto* o = dynamic_cast<const BaseAxisField*>(&f);
    if (!o) throw NullReferenceException("Object reference not set to an instance of an object");
    axisGroup = o->axisGroup;
    axisIncremental = o->axisIncremental;
    axisInverted = o->axisInverted;
    for (size_t n = overrideIncremental.size(); n-- > 0;) overrideIncremental[n] = o->overrideInverted[n];
    for (size_t n = overrideInverted.size(); n-- > 0;) overrideInverted[n] = o->overrideInverted[n];
}

bool BaseAxisField::IsInGroup(int64_t group, int overrideGroup, bool overrideDefault, bool include) const {
    bool flag = false;
    if (overrideGroup > 0 && overrideGroup <= static_cast<int>(overrideGroups.size()))
        flag = (overrideGroups[static_cast<size_t>(overrideGroup - 1)] & group) != 0;
    if (overrideGroup == 0 || (!overrideDefault && include)) flag |= (axisGroup & group) != 0;
    return flag;
}

bool BaseAxisField::ContainsNonDefaultAxes() const {
    if (axisGroup != defaultAxisGroup) return true;
    for (int64_t g : overrideGroups)
        if (g != 0) return true;
    return false;
}

namespace {
int64_t ParseAxisGroup(const std::optional<std::string>& name, int64_t def) {
    int64_t r = def;
    if (name && !name->empty()) {
        const EnumTable* t = FindModuleEnum("KSPAxisGroup");
        int64_t v = 0;
        if (t && t->TryParse(*name, v)) r = v;  // ArgumentException keeps the default
    }
    return r;
}
std::string AxisGroupString(int64_t v) {
    const EnumTable* t = FindModuleEnum("KSPAxisGroup");
    return t ? t->ToString(v) : std::to_string(v);
}
}  // namespace

// BaseAxisField.cs:169
void BaseAxisField::OnLoad(const ConfigNode& node) {
    axisGroup = ParseAxisGroup(node.GetValue("axisGroup"), defaultAxisGroup);
    axisIncremental = ParseAxisGroup(node.GetValue("axisIncremental"), 0);
    node.TryGetValue("axisSpeedMultiplier", incrementalSpeedMultiplier);
    axisInverted = ParseAxisGroup(node.GetValue("axisInverted"), 0);
    for (size_t i = 0; i < overrideGroups.size(); i++) {
        overrideGroups[i] = ParseAxisGroup(node.GetValue("overrideGroup" + std::to_string(i)), 0);
        overrideIncremental[i] = ParseAxisGroup(node.GetValue("overrideIncremental" + std::to_string(i)), 0);
        overrideInverted[i] = ParseAxisGroup(node.GetValue("overrideInverted" + std::to_string(i)), 0);
    }
}

// BaseAxisField.cs:187
void BaseAxisField::OnSave(ConfigNode& node) const {
    node.AddValue("axisGroup", AxisGroupString(axisGroup));
    node.AddValue("axisIncremental", AxisGroupString(axisIncremental));
    node.AddValue("axisSpeedMultiplier", incrementalSpeedMultiplier);
    node.AddValue("axisInverted", AxisGroupString(axisInverted));
    for (size_t i = 0; i < overrideGroups.size(); i++) {
        if (overrideGroups[i] != 0) node.AddValue("overrideGroup" + std::to_string(i), AxisGroupString(overrideGroups[i]));
        if (overrideIncremental[i] != 0) node.AddValue("overrideIncremental" + std::to_string(i), AxisGroupString(overrideIncremental[i]));
        if (overrideInverted[i] != 0) node.AddValue("overrideInverted" + std::to_string(i), AxisGroupString(overrideInverted[i]));
    }
}

// BaseAxisField.cs:213 (Mono float rule: wide evaluation, one store)
void BaseAxisField::SetAxis(float axisValue) {
    const float num = f32store((fx(axisValue) + 1.0) / 2.0);
    const float num2 = f32store(fx(minValue) + (fx(maxValue) - fx(minValue)) * fx(num));
    SetValue(num2);
}

// ---- BaseFieldList ------------------------------------------------------------------------------------------------------

// BaseFieldList.cs:154 CreateList (+ BaseFieldList.cs:230 override, AxisGroupsManager.BuildBaseAxisFields)
BaseFieldList::BaseFieldList(FieldStore& host) : host_(&host) {
    const auto& infos = host.FieldInfos();
    fields_.reserve(infos.size());
    for (size_t i = 0; i < infos.size(); i++) {
        const KSPFieldAttribute attr = GetKSPFieldAttribute(*infos[i]);
        std::shared_ptr<BaseField> f = attr.isAxis ? std::make_shared<BaseAxisField>(attr, *infos[i], &host, i)
                                                   : std::make_shared<BaseField>(attr, *infos[i], &host, i);
        f->SetOriginalValue();
        fields_.push_back(std::move(f));
    }
}

BaseField* BaseFieldList::operator[](std::string_view fieldName) const {
    for (const auto& f : fields_)
        if (f->name() == fieldName) return f.get();
    return nullptr;
}

ModuleValue BaseFieldList::GetValue(std::string_view fieldName) const {
    BaseField* f = (*this)[fieldName];
    return f ? f->GetValue() : ModuleValue{};
}

bool BaseFieldList::SetValue(std::string_view fieldName, const ModuleValue& value) {
    BaseField* f = (*this)[fieldName];
    return f && f->SetValue(value);
}

void BaseFieldList::SetOriginalValue() {
    for (auto& f : fields_) f->SetOriginalValue();
}

// BaseFieldList.cs:259
bool BaseFieldList::ReadValue(std::string_view valueName, std::string_view value) {
    BaseField* f = (*this)[valueName];
    if (!f) {
        log::Error("Field '" + std::string(valueName) + "' not found.");
        return false;
    }
    f->Read(value);
    return true;
}

// BaseFieldList.cs:293
void BaseFieldList::Load(const ConfigNode& node, std::vector<ModuleFieldValue>* trace) {
    for (const auto& value : node.values()) {
        BaseField* bf = (*this)[value.name];
        if (!bf || bf->hasInterface() || bf->uiControlOnly()) continue;
        const bool parsed = bf->Read(value.value);
        if (trace) {
            ModuleFieldValue fv;
            fv.field = &bf->FieldInfo();
            fv.raw = value.value;
            fv.parsed = parsed;
            if (parsed) fv.value = bf->GetValue();
            trace->push_back(std::move(fv));
        }
        if (bf->uiControlFlight->typeName != "UI_Label") {
            if (ConfigNode* n2 = node.GetNode(value.name + "_UIFlight")) bf->uiControlFlight->Load(*n2);
        } else if (bf->uiControlEditor->typeName != "UI_Label") {
            if (ConfigNode* n3 = node.GetNode(value.name + "_UIEditor")) bf->uiControlEditor->Load(*n3);
        }
    }
    for (int j = 0; j < node.nodes().Count(); j++) {
        const ConfigNode::NodePtr child = node.nodes().Shared(j);
        BaseField* bf = (*this)[child->name];
        if (!bf || !bf->hasInterface() || bf->uiControlOnly()) continue;
        ModuleValue& cur = host_->ValueAt(static_cast<size_t>(host_->IndexOf(bf->name())));
        if (std::holds_alternative<std::monostate>(cur)) continue;
        if (auto* fc = std::get_if<FloatCurve>(&cur)) fc->Load(*child);
        else if (auto* dc = std::get_if<DoubleCurve>(&cur)) dc->Load(*child);
        if (trace) {
            ModuleFieldValue fv;
            fv.field = &bf->FieldInfo();
            fv.fromNode = true;
            fv.node = child;
            fv.value = cur;
            fv.parsed = true;
            trace->push_back(std::move(fv));
        }
        if (bf->uiControlFlight->typeName != "UI_Label") {
            if (ConfigNode* n4 = node.GetNode(child->name + "_UIFlight")) bf->uiControlFlight->Load(*n4);
        } else if (bf->uiControlEditor->typeName != "UI_Label") {
            if (ConfigNode* n5 = node.GetNode(child->name + "_UIEditor")) bf->uiControlEditor->Load(*n5);
        }
    }
    SetOriginalValue();
}

// BaseFieldList.cs:366
void BaseFieldList::Save(ConfigNode& node) const {
    for (const auto& f : fields_) {
        if (!f->isPersistant || f->uiControlOnly()) continue;
        if (!f->hasInterface()) {
            const auto s = f->GetStringValue(false);
            if (!s) {
                log::Error("Input is null for field '" + f->name() + "' in config node '" + node.name + "'");
                node.AddValue(f->name(), "");
            } else {
                node.AddValue(f->name(), *s);
            }
            continue;
        }
        const ModuleValue& v = f->GetValue();
        if (const auto* fc = std::get_if<FloatCurve>(&v)) fc->Save(*node.AddNode(f->name()));
        else if (const auto* dc = std::get_if<DoubleCurve>(&v)) dc->Save(*node.AddNode(f->name()));
    }
}

void BaseFieldList::AddBaseFieldList(const BaseFieldList& add) {
    for (const auto& f : add.fields_) fields_.push_back(f);
}

void BaseFieldList::RemoveBaseFieldList(const BaseFieldList& remove) {
    for (const auto& f : remove.fields_) {
        auto it = std::find(fields_.begin(), fields_.end(), f);
        if (it != fields_.end()) fields_.erase(it);
    }
}

}  // namespace ksp
