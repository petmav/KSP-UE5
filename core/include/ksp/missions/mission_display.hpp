// WP-54: IMENodeDisplay (Assembly-CSharp/Expansions/Missions/IMENodeDisplay.cs) and the seams to the mission-editor UI that the node
// data classes call (MEGUINode, its connectors). The UI itself (MEGUINode, MEGUI_* controls, BaseAPField reflection, the editor
// panels) is not part of this package: it is the WP-46 / editor UI. A host that has a node canvas implements IMEGUINode; with
// `guiNode == nullptr` (play mode, tests) every call below is skipped exactly like the C# `guiNode != null` tests.
#pragma once

#include <string>
#include <vector>

#include "ksp/missions/mission_types.hpp"
#include "ksp/unity/color.hpp"

namespace ksp::Expansions::Missions {

namespace Editor {
// Editor/BaseAPField.cs: the reflection record of one [MEGUI_*] field (guiName, FieldType, GetValue). Not ported (editor UI); the
// declaration exists so that the GetNodeBodyParameterString signatures keep their C# shape.
class BaseAPField;
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
