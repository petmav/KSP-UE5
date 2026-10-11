// WP-40: the small declarations of the editor package -- ConstructionEventType.cs, ConstructionMode.cs, Attachment.cs, EditorFacilities.cs,
// SpaceCenterFacility.cs, UnityEngine.Space -- and the two seams the editor maths reads through: EditorEnvironment (the Unity GUI /
// input / camera / physics-raycast / highlighting side of EditorLogic: documented seam object, pattern DeployablesEnvironment) and
// EditorLimits (GameVariables part count / craft size / craft mass limits; WP-36 owns GameVariables, the editor calls through this).
#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "ksp/parts/part_data.hpp"
#include "ksp/parts/part_types.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/unity/quaternion.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class Part;
struct PartTransform;
class ShipConstruct;
struct AttachNode;

namespace UnityEngine {
// UnityEngine.Space (World = 0, Self = 1); completes the opaque declaration of events/game_event_types.hpp
enum class Space : int { World = 0, Self = 1 };
}  // namespace UnityEngine

// ConstructionEventType.cs:1; completes the opaque declaration of events/game_event_types.hpp
enum class ConstructionEventType : int {
    Unknown, PartCreated, PartDropped, PartPicked, PartDragging, PartAttached, PartDetached, PartDeleted, PartCopied, PartRootSelected,
    PartOffsetting, PartOffset, PartRotating, PartRotated, PartTweaked, PartSymmetryDeleted, PartOverInventoryGrid,
    PartPickedInInventoryGrid, PartDroppedInInventoryGrid
};

// ConstructionMode.cs:1
enum class ConstructionMode : int { Place, Move, Rotate, Root };

// EditorFacilities.cs:4 ([Flags])
enum class EditorFacilities : int { NONE = 0, ALL = -1, VAB = 1, SPH = 2 };

// SpaceCenterFacility.cs:3 is declared in save/save_enums.hpp (WP-42: one definition for WP-36 and WP-40)

// Attachment.cs:3 is declared in parts/part_types.hpp (WP-13)

// One surface ray hit of EditorLogic.checkAttach (Physics.Raycast on the editor camera ray): the part under the cursor, the hit point and
// normal, the collider's name and whether it carries the "NoAttach" tag
struct EditorSurfaceHit {
    Part* part = nullptr;               // FlightGlobals.GetPartUpwardsCached(hit.collider.gameObject)
    Vector3f point{};                   // hit.point
    Vector3f normal{};                  // hit.normal
    std::string colliderName;           // hit.collider.name (becomes srfAttachNode.srfAttachMeshName)
    bool noAttachTag = false;           // hit.collider.CompareTag("NoAttach")
    bool fairingPanelCollider = false;  // the collider belongs to a ModuleProceduralFairing panel (the attachment is refused)
};

// GameVariables limits as the editor needs them (GameVariables.GetPartCountLimit / GetCraftSizeLimit / GetCraftMassLimit,
// ScenarioUpgradeableFacilities.GetFacilityLevel; WP-36 implements GameVariables). Unset = no limit (int max / infinity).
struct EditorLimits {
    std::function<float(SpaceCenterFacility)> facilityLevel;                     // ScenarioUpgradeableFacilities.GetFacilityLevel
    std::function<int(float level, bool isVAB)> partCountLimit;                  // GameVariables.Instance.GetPartCountLimit
    std::function<Vector3f(float level, bool isPad)> craftSizeLimit;             // GameVariables.Instance.GetCraftSizeLimit
    std::function<double(float level, bool isPad)> craftMassLimit;               // GameVariables.Instance.GetCraftMassLimit
    static inline EditorLimits& Instance() {
        static EditorLimits v;
        return v;
    }
};

// What EditorLogic / EditorLogicBase / the gizmos read from Unity and the game settings (GUI, input, camera, physics raycasts, highlighting
// and audio are the engine shell's). The defaults are a VAB with all settings at their GameSettings.cs:967-984 defaults and no cursor.
struct EditorEnvironment {
    EditorFacility editorFacility = EditorFacility::VAB;  // EditorDriver.editorFacility
    // CheatOptions
    bool allowPartClipping = false;                // CheatOptions.AllowPartClipping
    bool nonStrictAttachmentOrientation = false;   // CheatOptions.NonStrictAttachmentOrientation
    // GameSettings (defaults: GameSettings.cs:967 VAB_USE_ANGLE_SNAP = false, :969 VAB_FINE_OFFSET_THRESHOLD = 20, :984 EDITOR_UNDO_REDO_LIMIT = 32)
    bool useAngleSnap = false;                     // VAB_USE_ANGLE_SNAP
    float fineOffsetThreshold = 20.0f;             // VAB_FINE_OFFSET_THRESHOLD
    int undoRedoLimit = 32;                        // EDITOR_UNDO_REDO_LIMIT
    bool fineTweak = false;                        // GameSettings.Editor_fineTweak.GetKey()
    bool modifierKey = false;                      // GameSettings.MODIFIER_KEY.GetKey()
    // Part.GetReferenceTransform(): the model transform a part names as its reference (models are the .mu reader's; unset: the part's own
    // transform, which is the reference of every part that does not call SetReferenceTransform)
    std::function<PartTransform*(Part&)> referenceTransform;
    // Part.GetReferenceParent() (CompoundPart overrides it while tweaking its target); unset: part.parent
    std::function<Part*(Part&)> referenceParent;
    // checkAttach, stack branch: attachNode4.icon.GetComponent<Renderer>().bounds.IntersectRay(ray through the screen position of the
    // selected part's node) || bounds.Contains(point); unset: false (no cursor)
    std::function<bool(Part& part, AttachNode& node, const Vector3f& worldPointOfSelectedNode)> nodeIconUnderCursor;
    // checkAttach, surface branch: Physics.Raycast(editorCamera.ScreenPointToRay(mouse - srfAttachCursorOffset), 1000, LayerUtil.DefaultEquivalent)
    std::function<std::optional<EditorSurfaceHit>()> surfaceRaycast;
    // EditorLogicBase.FindPartSurface(p, origin, direction, fromParent, hit, out srfNormal) (Collider.Raycast against the part's colliders)
    std::function<float(Part& p, const Vector3f& origin, const Vector3f& direction, const Vector3f& fromParent, const EditorSurfaceHit& hit,
                        Vector3f& srfNormal)>
        findPartSurface;
    // EditorGeometryUtil.isValidPartCollider (collider tags / layer / WheelCollider): the colliders that count for a part; unset: all
    std::function<bool(Part&, size_t colliderIndex)> isValidPartCollider;
    // Part.editorCollision is set by the physics collision callbacks of the editor scene
    std::map<const Part*, Part*> editorCollision;
    // Part.surfaceAttachGO (the surface attach collider toggled while offset-gizmo testing): a no-op without an engine
    std::function<void(Part&, bool)> setSurfaceAttachActive;
    // GameEvents / audio / screen messages are fired or skipped by the EditorLogic port itself; these only observe
    std::function<void(Part& p, bool attach, bool reset)> onHighlightSelected;   // EditorLogic.highlightSelected (Highlighter)

    static inline EditorEnvironment& Current() {
        static EditorEnvironment v;
        return v;
    }
};

}  // namespace ksp
