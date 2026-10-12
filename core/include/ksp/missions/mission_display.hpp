// WP-54: IMENodeDisplay (Assembly-CSharp/Expansions/Missions/IMENodeDisplay.cs) and the seams to the mission-editor UI that the node
// data classes call (MEGUINode, its connectors). The UI itself (MEGUINode, MEGUI_* controls, BaseAPField reflection, the editor
// panels) is not part of this package: it is the WP-46 / editor UI. A host that has a node canvas implements IMEGUINode; with
// `guiNode == nullptr` (play mode, tests) every call below is skipped exactly like the C# `guiNode != null` tests.
#pragma once

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
