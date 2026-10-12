// WP-54c: Assembly-CSharp/Expansions/Missions/Editor/{BaseAPField,BaseAPFieldList}.cs, BaseField.cs (BaseField<K>), BaseFieldList.cs (BaseFieldList<R, K>),
// MEGUI_Control.cs and FieldAttribute.cs. See mission_display.hpp / mission_apfield.hpp. Cite placeholders `File.cs:@Name[#k]` are resolved by
// tools/wp55/check_cites.py --resolve.
#include "ksp/missions/mission_apfield.hpp"

#include <stdexcept>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/missions/mission_validator.hpp"

namespace ksp::Expansions::Missions::Editor {

using KSP::Localization::Localizer;

namespace {
// BaseFieldList.reflectedAttributeCache: type name -> ReflectedData
std::map<std::string, std::pair<std::vector<BaseAPFieldList::ReflectedMember>, std::vector<BaseAPFieldList::ReflectedMember>>>& Registry() {
    static std::map<std::string, std::pair<std::vector<BaseAPFieldList::ReflectedMember>, std::vector<BaseAPFieldList::ReflectedMember>>> r;
    return r;
}
}  // namespace

// ---- MEGUI_Control.cs / FieldAttribute.cs ---------------------------------------------------------------------------------------------------------------------

// MEGUI_Control.cs:84 (FieldAttribute() first: guiName = "")
MEGUI_Control::MEGUI_Control() {
    set_guiName("");
    set_Tooltip("");
}
// FieldAttribute.cs, guiName setter (line 11)
void MEGUI_Control::set_guiName(const std::string& value) { guiName_ = Localizer::Format(value); }
// MEGUI_Control.cs, Tooltip setter (line 72)
void MEGUI_Control::set_Tooltip(const std::string& value) { tooltip_ = Localizer::Format(value); }

// ---- BaseAPField ----------------------------------------------------------------------------------------------------------------------------------------------

// BaseAPField.cs:258 (BaseField<K>.BaseField(fieldAttrib, memberInfo, host) first)
BaseAPField::BaseAPField(const MEGUI_Control& fieldAttrib, const MemberInfo& memberInfo, IAPFieldHost* hostType, void* host)
    : attribute_(fieldAttrib), member_(memberInfo), host_(host) {
    // BaseField.cs:125
    name = member_.name;
    propertyInfo_ = member_.kind == MemberInfo::Kind::Property;
    hasInterface_ = !propertyInfo_ && member_.valueTypeHasIConfigNode;
    guiName = Localizer::Format(fieldAttrib.guiName().empty() ? member_.name : fieldAttrib.guiName());
    fieldType = member_.valueType;
    // BaseAPField.cs
    gapDisplay_ = fieldAttrib.gapDisplay;
    group_ = fieldAttrib.group;
    groupDisplayName_ = fieldAttrib.groupDisplayName;
    if (hostType != nullptr) {
        if (!fieldAttrib.onValueChange.empty()) onValueChange_ = hostType->FindValueMethod(fieldAttrib.onValueChange, member_.valueType);
        if (!fieldAttrib.onControlCreated.empty()) onControlCreated_ = hostType->FindControlMethod(fieldAttrib.onControlCreated);
        if (fieldAttrib.onControlSetupComplete.has_value() && !fieldAttrib.onControlSetupComplete->empty())
            onControlSetupComplete_ = hostType->FindControlMethod(*fieldAttrib.onControlSetupComplete);
        if (fieldAttrib.compareValuesForCheckpoint.has_value() && !fieldAttrib.compareValuesForCheckpoint->empty()) {
            std::optional<IAPFieldHost::CompareMethod> m = hostType->FindCompareMethod(*fieldAttrib.compareValuesForCheckpoint, member_.valueType);
            if (m.has_value()) {
                if (!m->returnsBool) {
                    // Debug.Log("[Checkpoint validation]: The method ... has an invalid return type.")
                } else {
                    compareValuesForCheckpoint_ = m->fn;
                }
            }
        }
    }
    hideWhenSiblingsExist_ = fieldAttrib.hideWhenSiblingsExist;
    hideWhenStartNode_ = fieldAttrib.hideWhenStartNode;
    hideWhenDocked_ = fieldAttrib.hideWhenDocked;
    hideWhenInputConnected_ = fieldAttrib.hideWhenInputConnected;
    hideWhenOutputConnected_ = fieldAttrib.hideWhenOutputConnected;
    hideWhenNoTestModules_ = fieldAttrib.hideWhenNoTestModules;
    hideWhenNoActionModules_ = fieldAttrib.hideWhenNoActionModules;
    hideOnSetup_ = fieldAttrib.hideOnSetup;
    order_ = fieldAttrib.order;
    tabStop_ = fieldAttrib.tabStop;
    groupStartCollapsed_ = fieldAttrib.groupStartCollapsed;
}

// BaseAPField.cs:84  (string.Concat(MemberInfo.DeclaringType, ".", name))
std::string BaseAPField::FieldID() const { return member_.declaringType + "." + name; }

// BaseAPField.cs, FieldType getter (line 86)
std::string BaseAPField::FieldTypeName() const {
    if (propertyInfo_ || member_.kind == MemberInfo::Kind::Field) return member_.valueType;
    return "";
}

// BaseField.cs:140
bool BaseAPField::SetValue(const Boxed& newValue, void* host) {
    try {
        if (propertyInfo_ || !member_.set) throw NullReferenceException();  // _fieldInfo is null for a property member
        member_.set(host, newValue);
        for (const auto& cb : OnValueModified) cb(newValue);
        return true;
    } catch (const std::exception&) {
        // PDebug.Error("Value ... could not be set to field ...")
        return false;
    }
}

// BaseField.cs:156
Boxed BaseAPField::GetValue(void* host) {
    try {
        if (propertyInfo_ || !member_.get) throw NullReferenceException();
        return member_.get(host);
    } catch (const std::exception&) {
        // PDebug.Error("Value could not be retrieved from field ...")
        return Boxed();
    }
}

// BaseAPField.cs:299
bool BaseAPField::SetValue(const Boxed& newValue) {
    if (!propertyInfo_) {
        if (SetValue(newValue, host_)) {
            if (onValueChange_) onValueChange_(newValue);
            MissionEditorValidator::RunValidationOnParamChange();
            return true;
        }
        return false;
    }
    try {
        member_.set(host_, newValue);
        if (onValueChange_) onValueChange_(newValue);
        MissionEditorValidator::RunValidationOnParamChange();
        return true;
    } catch (const std::exception&) {
        // PDebug.Error("Value ... could not be set to field ...")
        return false;
    }
}

// BaseAPField.cs:332
Boxed BaseAPField::GetValue() {
    if (!propertyInfo_) return GetValue(host_);
    try {
        return member_.get(host_);
    } catch (const std::exception&) {
        // PDebug.Error("Value could not be retrieved from field ...")
        return Boxed();
    }
}

// BaseAPField.cs:354
void BaseAPField::SetOriginalValue() {
    if (!propertyInfo_) {
        // BaseField.cs:174: _originalValue = _fieldInfo.GetValue(_host)
        originalValue_ = member_.get(host_);
    } else {
        originalValue_ = member_.get(host_);
    }
}

// MissionsUtils.cs:565
std::string BaseAPField::GetFieldValueForDisplay() {
    if (member_.valueIsEnum) return member_.enumDescription(GetValue());
    if (member_.valueIsBool) {
        Boxed v = GetValue();
        if (!v.has_value()) throw NullReferenceException();  // (bool)null
        return std::any_cast<bool>(v) ? Localizer::Format("#autoLOC_439839") : Localizer::Format("#autoLOC_439840");
    }
    Boxed v = GetValue();
    if (!v.has_value()) throw NullReferenceException();  // null.ToString()
    return member_.toString(v);
}

// ---- BaseAPFieldList / BaseFieldList<R, K> -----------------------------------------------------------------------------------------------------------------

void BaseAPFieldList::RegisterReflectedType(const std::string& typeName, std::vector<ReflectedMember> fields, std::vector<ReflectedMember> properties) {
    Registry()[typeName] = {std::move(fields), std::move(properties)};
}
void BaseAPFieldList::ResetReflectedTypes() { Registry().clear(); }

// BaseFieldList.cs:125
BaseAPFieldList::BaseAPFieldList() : fields_(std::vector<std::unique_ptr<BaseAPField>>()) {}

// BaseFieldList.cs:135
BaseAPFieldList::BaseAPFieldList(const std::string& hostTypeName, IAPFieldHost* hostType, void* host) {
    if (host == nullptr) {
        // PDebug.Error("Field: Host is null")
        return;
    }
    host_ = host;
    typeName_ = hostTypeName;
    fields_ = std::vector<std::unique_ptr<BaseAPField>>();
    CreateList(hostTypeName, hostType, host);
}

// BaseFieldList.cs:159
void BaseAPFieldList::CreateList(const std::string& typeName, IAPFieldHost* hostType, void* instance) {
    auto it = Registry().find(typeName);
    if (it == Registry().end()) return;
    auto create = [&](const ReflectedMember& m) {
        auto val = std::make_unique<BaseAPField>(m.attribute, m.member, hostType, instance);
        val->SetOriginalValue();
        fields_->push_back(std::move(val));
    };
    for (const ReflectedMember& m : it->second.first) create(m);
    for (const ReflectedMember& m : it->second.second) create(m);
}

// BaseFieldList.cs:86
int BaseAPFieldList::Count() const {
    if (!fields_.has_value()) throw NullReferenceException();
    return static_cast<int>(fields_->size());
}

// BaseFieldList.cs:131[]
BaseAPField* BaseAPFieldList::operator[](const std::string& fieldName) {
    if (!fields_.has_value()) throw NullReferenceException();
    for (auto& f : *fields_)
        if (f->name == fieldName) return f.get();
    return nullptr;
}

// BaseFieldList.cs:131[]#2
BaseAPField* BaseAPFieldList::operator[](int index) {
    if (!fields_.has_value()) throw NullReferenceException();
    if (index < 0 || index >= static_cast<int>(fields_->size())) throw std::out_of_range("index");
    return (*fields_)[static_cast<size_t>(index)].get();
}

void BaseAPFieldList::SetAt(int index, std::unique_ptr<BaseAPField> value) {
    if (!fields_.has_value()) throw NullReferenceException();
    if (index < 0 || index >= static_cast<int>(fields_->size())) throw std::out_of_range("index");
    (*fields_)[static_cast<size_t>(index)] = std::move(value);
}

// BaseFieldList.cs:186
Boxed BaseAPFieldList::GetValue(const std::string& fieldName) {
    BaseAPField* f = (*this)[fieldName];
    if (f == nullptr) return Boxed();
    // BaseField<K>.GetValue(host): the field path (a property member has no FieldInfo: logged, null)
    return f->GetValue(host_);
}

// BaseFieldList.cs:201
bool BaseAPFieldList::SetValue(const std::string& fieldName, const Boxed& value) {
    BaseAPField* f = (*this)[fieldName];
    if (f == nullptr) return false;
    return f->SetValue(value, host_);
}

// BaseFieldList.cs:211
void BaseAPFieldList::SetOriginalValue() {
    if (!fields_.has_value()) throw NullReferenceException();
    for (auto& f : *fields_) f->SetOriginalValue();
}

// BaseFieldList.cs:220
void BaseAPFieldList::Add(std::unique_ptr<BaseAPField> element) {
    if (!fields_.has_value()) throw NullReferenceException();
    fields_->push_back(std::move(element));
}

}  // namespace ksp::Expansions::Missions::Editor
