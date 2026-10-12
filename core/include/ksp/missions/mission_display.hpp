// WP-54: IMENodeDisplay (Assembly-CSharp/Expansions/Missions/IMENodeDisplay.cs) and the seams to the mission-editor UI that the node
// data classes call (MEGUINode, its connectors). The UI itself (MEGUINode, MEGUI_* controls, BaseAPField reflection, the editor
// panels) is not part of this package: it is the WP-46 / editor UI. A host that has a node canvas implements IMEGUINode; with
// `guiNode == nullptr` (play mode, tests) every call below is skipped exactly like the C# `guiNode != null` tests.
#pragma once

#include <any>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "ksp/missions/mission_types.hpp"
#include "ksp/unity/color.hpp"

namespace ksp::Expansions::Missions {

class MissionCelestialBody;
class ParamChoices_CelestialBodySurface;
class MissionKerbal;

namespace Editor {

// ---- WP-54c: the reflection model behind the real BaseAPField (Editor/BaseAPField.cs, BaseField.cs, BaseFieldList.cs, MEGUI_Control.cs, FieldAttribute.cs) -----------------
// C# reflects over the host object (FieldInfo / PropertyInfo / MethodInfo); C++ has no reflection, so a host registers one MemberInfo per [MEGUI_*] member (accessors
// that stand for FieldInfo.GetValue / SetValue) and an IAPFieldHost that resolves the callback names of the attribute (host.GetType().GetMethod(...)).

// CheckpointValidationType.cs:3
enum class CheckpointValidationType { None = 0, Equals = 1, CustomMethod = 2, Controls = 3 };

// MEGUI_Control.cs:8 (the attribute): the fields and constructor defaults. guiName (FieldAttribute.cs) and Tooltip localize on assignment.
struct MEGUI_Control {
    MEGUI_Control();                                           // :88 (defaults below)
    void set_guiName(const std::string& value);                // FieldAttribute.guiName setter: Localizer.Format(value)
    void set_Tooltip(const std::string& value);                // :70 setter: Localizer.Format(value)
    const std::string& guiName() const { return guiName_; }
    const std::string& Tooltip() const { return tooltip_; }
    bool gapDisplay = false;                                   // :26
    std::string onValueChange = "";                            // :28
    std::string onControlCreated = "";                         // :30
    std::optional<std::string> onControlSetupComplete;         // :32 (never initialised: null)
    std::string group = "";                                    // :34
    std::optional<std::string> groupDisplayName;               // :36 (null)
    std::optional<std::string> resetValue;                     // :38 (null)
    bool canBeReset = true;                                    // :40
    bool canBePinned = true;                                   // :42
    bool hideWhenSiblingsExist = false, hideWhenStartNode = false, hideWhenDocked = false, hideWhenInputConnected = false;  // :44-:50
    bool hideWhenOutputConnected = false, hideWhenNoTestModules = false, hideWhenNoActionModules = false, hideOnSetup = false;  // :52-:58
    bool groupStartCollapsed = false;                          // :62
    int order = -1;                                            // :64
    bool tabStop = false;                                      // :66
    CheckpointValidationType checkpointValidation = CheckpointValidationType::Equals;  // :68
    std::optional<std::string> compareValuesForCheckpoint;     // :70 (null)

private:
    std::string guiName_;
    std::string tooltip_;
};

using Boxed = std::any;  // a boxed C# object (an empty any is null)

// MemberInfo / FieldInfo / PropertyInfo of one reflected member
struct MemberInfo {
    enum class Kind { Field, Property };
    Kind kind = Kind::Field;
    std::string name;                                          // MemberInfo.Name
    std::string declaringType;                                 // MemberInfo.DeclaringType (the full type name)
    std::string valueType;                                     // FieldInfo.FieldType / PropertyInfo.PropertyType (Type.Name)
    bool valueTypeHasIConfigNode = false;                      // FieldType.GetInterface("IConfigNode") != null
    bool valueIsEnum = false;                                  // FieldType.BaseType == typeof(Enum)
    bool valueIsBool = false;                                  // FieldType == typeof(bool)
    std::function<Boxed(void* host)> get;                      // FieldInfo.GetValue(host) / PropertyInfo.GetValue(host, null); may throw
    std::function<void(void* host, const Boxed& value)> set;   // FieldInfo.SetValue / PropertyInfo.SetValue; may throw
    std::function<std::string(const Boxed&)> enumDescription;  // Enum.displayDescription() of the boxed value
    std::function<std::string(const Boxed&)> toString;         // object.ToString()
};

// host.GetType().GetMethod(name, flags, null, new Type[]{ paramType }, null): the callbacks a host exposes by name
class IAPFieldHost {
public:
    virtual ~IAPFieldHost() = default;
    // a method `void name(FieldType)` (onValueChange); empty = no such method
    virtual std::function<void(const Boxed&)> FindValueMethod(const std::string& name, const std::string& fieldType) {
        (void)name;
        (void)fieldType;
        return {};
    }
    // onControlCreated / onControlSetupComplete take the control as argument
    virtual std::function<void(void*)> FindControlMethod(const std::string& name) {
        (void)name;
        return {};
    }
    // a method `T name(FieldType)` (compareValuesForCheckpoint); `returnsBool` is ReturnType == typeof(bool)
    struct CompareMethod {
        std::function<bool(const Boxed&)> fn;
        bool returnsBool = true;
    };
    virtual std::optional<CompareMethod> FindCompareMethod(const std::string& name, const std::string& fieldType) {
        (void)name;
        (void)fieldType;
        return std::nullopt;
    }
};

// Editor/BaseAPField.cs: the reflection record of one [MEGUI_*] field. The attribute reflection (BaseAPFieldList, MEGUI controls) is the editor
// UI and is not ported. WP-55 stand-in: only what the node library's GetNodeBodyParameterString overrides read from a field, filled by the
// caller (the editor host, or a test): `name` (FieldInfo name), `guiName` (the localization tag of the attribute), and `value`, the
// `GetValue().ToString()` text when the field is not null (`hasValue` false = GetValue() == null).
class BaseAPField {
public:
    std::string name;
    std::string guiName;
    std::string value;       // MissionsUtils.GetFieldValueForDisplay(field): enum displayDescription, localized bool, else ToString()
    bool hasValue = true;
    // TestModule.GetNodeBodyParameterString :258 looks at the field's type: MissionCelestialBody (pointer to the value) and the slice-2 types
    // ParamChoices_CelestialBodySurface / MissionKerbal (the text their branch returns, precomputed by the caller)
    const MissionCelestialBody* celestialBody = nullptr;
    // WP-54 slice 2: `field.FieldType == typeof(X)` and `field.GetValue() is X`: the type name and the value, for the slice-2 data classes
    const ParamChoices_CelestialBodySurface* paramChoices = nullptr;   // TestModule.cs:259
    const MissionKerbal* missionKerbal = nullptr;                      // TestModule.cs:267
    std::string fieldType;                                                   // FieldType.Name ("SurfaceArea", "MissionOrbit", ...)
    std::optional<std::string> typedText;
    BaseAPField() = default;
    BaseAPField(std::string n, std::string gui, std::string v = "", bool has = true)
        : name(std::move(n)), guiName(std::move(gui)), value(std::move(v)), hasValue(has) {}

    // ---- WP-54c: the real BaseAPField (BaseAPField.cs + BaseField<MEGUI_Control>) -----------------------------------------------------------------------------------------
    // `host` is the object the member belongs to (BaseField._host), `hostType` resolves the callback names (host.GetType().GetMethod).
    BaseAPField(const MEGUI_Control& fieldAttrib, const MemberInfo& memberInfo, IAPFieldHost* hostType, void* host);  // BaseAPField.cs:@BaseAPField
    bool gapDisplay() const { return gapDisplay_; }                                  // :@gapDisplay
    const std::function<void(const Boxed&)>& OnValueChange() const { return onValueChange_; }
    const std::function<void(void*)>& OnControlCreated() const { return onControlCreated_; }
    const std::function<void(void*)>& OnControlSetupComplete() const { return onControlSetupComplete_; }
    const std::function<bool(const Boxed&)>& CompareValuesForCheckpoint() const { return compareValuesForCheckpoint_; }
    bool IsProperty() const { return propertyInfo_; }                                // PropertyInfo != null
    std::string FieldID() const;                                                     // :@FieldID
    std::string FieldTypeName() const;                                               // :@FieldType ("" = null)
    const std::string& Group() const { return group_; }
    const std::optional<std::string>& GroupDisplayName() const { return groupDisplayName_; }
    bool HideWhenSiblingsExist() const { return hideWhenSiblingsExist_; }
    bool HideWhenStartNode() const { return hideWhenStartNode_; }
    bool HideWhenDocked() const { return hideWhenDocked_; }
    bool HideWhenInputConnected() const { return hideWhenInputConnected_; }
    bool HideWhenOutputConnected() const { return hideWhenOutputConnected_; }
    bool HideWhenNoTestModules() const { return hideWhenNoTestModules_; }
    bool HideWhenNoActionModules() const { return hideWhenNoActionModules_; }
    bool HideOnSetup() const { return hideOnSetup_; }
    int Order() const { return order_; }
    bool TabStop() const { return tabStop_; }
    bool GroupStartCollapsed() const { return groupStartCollapsed_; }
    const MEGUI_Control& Attribute() const { return attribute_; }                    // BaseField.Attribute
    bool hasInterface() const { return hasInterface_; }                              // BaseField.hasInterface
    const Boxed& originalValue() const { return originalValue_; }                    // BaseField.originalValue
    bool SetValue(const Boxed& newValue);                                            // BaseAPField.cs:@SetValue
    Boxed GetValue();                                                                // BaseAPField.cs:@GetValue
    void SetOriginalValue();                                                         // BaseAPField.cs:@SetOriginalValue
    // BaseField.OnValueModified (fired by the field path of SetValue, not by the property path)
    std::vector<std::function<void(const Boxed&)>> OnValueModified;
    // MissionsUtils.GetFieldValueForDisplay(field) (MissionsUtils.cs:565)
    std::string GetFieldValueForDisplay();
    const MemberInfo& Member() const { return member_; }

    // BaseField<K>.SetValue(newValue, host) / GetValue(host): what BaseFieldList.SetValue / GetValue call (no onValueChange, no validation; a property member has no FieldInfo)
    bool SetValue(const Boxed& newValue, void* host);                                // BaseField.cs:@SetValue
    Boxed GetValue(void* host);                                                      // BaseField.cs:@GetValue

private:
    MEGUI_Control attribute_;
    MemberInfo member_;
    void* host_ = nullptr;
    Boxed originalValue_;
    bool hasInterface_ = false;
    bool propertyInfo_ = false;
    bool gapDisplay_ = false;
    std::string group_;
    std::optional<std::string> groupDisplayName_;
    std::function<void(const Boxed&)> onValueChange_;
    std::function<void(void*)> onControlCreated_, onControlSetupComplete_;
    std::function<bool(const Boxed&)> compareValuesForCheckpoint_;
    bool hideWhenSiblingsExist_ = false, hideWhenStartNode_ = false, hideWhenDocked_ = false, hideWhenInputConnected_ = false, hideWhenOutputConnected_ = false;
    bool hideWhenNoTestModules_ = false, hideWhenNoActionModules_ = false, hideOnSetup_ = false;
    int order_ = -1;
    bool tabStop_ = false, groupStartCollapsed_ = false;
};
}  // namespace Editor

// Forward declarations for the editor control seams below
}  // namespace ksp::Expansions::Missions
namespace ksp {
class Part;
}
namespace ksp::Expansions::Missions {

namespace Editor {

// Editor/MEGUIDropDownItem.cs:3 (a drop-down entry: key, boxed value, display text). The boxed value is kept as its text (enum member name / guid).
struct MEGUIDropDownItem {
    std::string key;
    std::string value;
    std::string text;
};

// The editor controls the actions / tests keep a reference to (OnXControlCreated callbacks). Seams: a host that has the mission editor implements
// them; with no control (play mode, tests without a UI) every `!= null` test of the C# is false.
class IMEGUIVesselPartSelector {  // MEGUIParameterVesselPartSelector
public:
    virtual ~IMEGUIVesselPartSelector() = default;
    virtual Part* GetSelectedPart() = 0;
    virtual void RefreshUI() = 0;
};
class IMEGUIDynamicModuleListControl {  // MEGUIParameterDynamicModuleList
public:
    virtual ~IMEGUIDynamicModuleListControl() = default;
    virtual void RefreshUI() = 0;
    virtual void SetAddButtonText(const std::string& text) = 0;  // addButtonText.text = ...
};
class IMEGUIControl {  // any MEGUIParameter* control: the gameObject.SetActive(value) the visibility callbacks call
public:
    virtual ~IMEGUIControl() = default;
    virtual void SetActive(bool value) = 0;
};
class IMEGUIDropdownList : public IMEGUIControl {  // MEGUIParameterDropdownList
public:
    virtual void RebuildDropDown() = 0;
    virtual std::optional<int> SelectedValueInt() = 0;            // (int)SelectedValue for an enum drop-down, nullopt = SelectedValue == null
    virtual int GetItemIndex(std::optional<int> value) = 0;       // GetItemIndex(SelectedValue)
};

}  // namespace Editor

// IMENodeDisplay.cs:6
class IMENodeDisplay {
public:
    virtual ~IMENodeDisplay() = default;
    virtual std::string GetName() = 0;                                   // :8
    virtual std::string GetDisplayName() = 0;                            // :10
    virtual void AddParameterToSAP(const std::string& parameter) = 0;    // :12
    virtual void RemoveParameterFromSAP(const std::string& parameter) = 0;
    virtual void AddParameterToNodeBody(const std::string& parameter) = 0;
    virtual void AddParameterToNodeBodyAndUpdateUI(const std::string& parameter) = 0;
    virtual void RemoveParameterFromNodeBody(const std::string& parameter) = 0;
    virtual void RemoveParameterFromNodeBodyAndUpdateUI(const std::string& parameter) = 0;
    virtual bool HasNodeBodyParameter(const std::string& parameter) = 0;
    virtual bool HasSAPParameter(const std::string& parameter) = 0;
    virtual std::string GetNodeBodyParameterString(const Editor::BaseAPField& field) = 0;  // :26 (editor reflection: base returns "")
    virtual void UpdateNodeBodyUI() = 0;
    virtual std::vector<IMENodeDisplay*> GetInternalParametersToDisplay() = 0;
    virtual MENode* GetNode() = 0;
    virtual std::string GetInfo() = 0;
    virtual void ParameterSetupComplete() = 0;
};

namespace Editor {

// Editor/MEGUINodeConnector (the output connector of a node canvas box): the two members MENode.Save reads (MENode.cs:1197-1199).
class IMEGUIConnector {
public:
    virtual ~IMEGUIConnector() = default;
    virtual bool IsConnectedToNode(const class IMEGUINode* other) const = 0;
    virtual Color LineColour() const = 0;
};

// Editor/MEGUINode (the node canvas box). Seam of the data classes to the editor UI.
class IMEGUINode {
public:
    virtual ~IMEGUINode() = default;
    virtual void SetTitleText(const std::string& text) = 0;           // MENode.Title setter (MENode.cs:151)
    virtual void SetEndNode(bool value) = 0;                          // MENode.SetEndNode (:577)
    virtual void UpdateInputConnectorImage() = 0;                     // MENode.SetCatchAllNode (:597)
    virtual void SetNodeColor(const Color& color) = 0;                // MENode.SetNodeColor (:825)
    virtual void DisplayNodeBodyParameters() = 0;                     // TestModule / ActionModule / DynamicModule.UpdateNodeBodyUI
    virtual void ClearValidityIndicators() = 0;                       // MissionEditorValidator.RunValidation (:152)
    virtual void SetValidityIndicators(const MissionValidationTestResult& result) = 0;  // MissionEditorValidator.AddNodeResult (:230)
    virtual std::vector<IMEGUIConnector*> OutputConnectors() = 0;     // MENode.Save (:1195)
};

}  // namespace Editor

}  // namespace ksp::Expansions::Missions
