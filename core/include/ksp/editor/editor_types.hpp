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
struct AvailablePart;
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

// EditorScreen.cs:1 (completes the opaque declaration of events/game_event_types.hpp; moved here from inventory/construction.hpp in WP-40b)
enum class EditorScreen : int { Parts, Actions, Crew, Cargo };

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

// UnityEngine.Ray as the editor reads it (Camera.ScreenPointToRay); the constructor normalises the direction like Ray(origin, direction)
struct EditorRay {
    Vector3f origin{};
    Vector3f direction{};
    EditorRay() = default;
    EditorRay(const Vector3f& o, const Vector3f& d) : origin(o), direction(d.normalized()) {}
};

// The audio clips EditorLogic plays through its AudioSource (EditorLogic.cs:60-84); the clip assets are the shell's
enum class EditorClip : int { PartGrab, PartRelease, Attach, CannotPlace, TweakGrab, TweakRelease, DeletePart, ReRoot };

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
    // the key / setting reads the placement loop makes live (WP-40b): when set they replace the plain fields above (Awake wires them to the
    // WP-50 input device and GameSettings); unset, the fields are the answer (the data-only editor tests)
    std::function<bool()> fineTweakKey, modifierKeyHeld, useAngleSnapSetting;
    bool FineTweak() const { return fineTweakKey ? fineTweakKey() : fineTweak; }
    bool ModifierKey() const { return modifierKeyHeld ? modifierKeyHeld() : modifierKey; }
    bool UseAngleSnap() const { return useAngleSnapSetting ? useAngleSnapSetting() : useAngleSnap; }
    // Part.GetComponent<FlagDecalBackground>() != null (the flag part; components are not modelled): unset = false
    std::function<bool(Part&)> hasFlagDecalBackground;
    // EditorPartList.Instance.RevealPart(partInfo): observer
    std::function<void(const AvailablePart&)> revealPart;
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

    // ---- scene seams of the placement loop (WP-40b). The shell (or a test) implements them; the port never invents an answer for a scene
    // query: where the C# asks Unity, the port asks one of these. Unset seams behave as an empty scene (no hit, pointer not over any UI).
    // editorCamera.transform.position
    std::function<Vector3f()> cameraPosition;
    // editorCamera.ScreenPointToRay(screenPoint)
    std::function<EditorRay(const Vector3f& screenPoint)> screenPointToRay;
    // editorCamera.WorldToScreenPoint(worldPoint)
    std::function<Vector3f(const Vector3f& worldPoint)> worldToScreenPoint;
    // Physics.Raycast(ray, out hit, maxDistance, layerMask) followed by FlightGlobals.GetPartUpwardsCached(hit.collider.gameObject): nullopt = no hit;
    // a hit whose collider belongs to no part has part == nullptr
    std::function<std::optional<EditorSurfaceHit>(const EditorRay& ray, float maxDistance, int layerMask)> raycast;
    // EventSystem.current.IsPointerOverGameObject()
    std::function<bool()> pointerOverUI;
    // EditorLogic.mouseOverModalArea(modalAreas)
    std::function<bool()> mouseOverModalArea;
    // GizmoOffset / GizmoRotate.GetMouseOverGizmo (the gizmo handles are the shell's objects)
    std::function<bool(bool offsetGizmo)> mouseOverGizmo;
    // audioSource.PlayOneShot(clip)
    std::function<void(EditorClip clip)> playOneShot;
    // EditorLogic.HeldPartIsStacked(): UIPartActionControllerInventory.Instance != null && CurrentCargoPart != null && (heldPartIsStack || stackSize > 1); unset: false
    std::function<bool()> heldPartIsStacked;
    // Part.onEditorStartTweak / onEditorEndTweak (virtual; only CompoundPart overrides them: the compound tweak mode is the shell's)
    std::function<void(Part&)> onEditorStartTweak;
    std::function<void(Part&)> onEditorEndTweak;
    // EditorLogic.NewShip: the unsaved-changes dialog (EditorLogic.cs:4031); true = the Discard button (OnNewShipConfirm), false = Cancel; unset: Discard
    std::function<bool()> newShipDialog;
    // EditorLogic.partSearchUpdate / PartCategorizer, ScreenMessages.PostScreenMessage: observers only
    std::function<void(const std::string&)> postScreenMessage;

    static inline EditorEnvironment& Current() {
        static EditorEnvironment v;
        return v;
    }
};

// The scene services of the placement loop as one interface: the shell (the future editor collision world, camera and UI of the Unreal side) implements it
// and `InstallEditorScene` wires it to the `EditorEnvironment` seams above (each virtual is the Unity call it names). A test may keep filling the
// std::function members directly.
class IEditorScene {
public:
    virtual ~IEditorScene() = default;
    virtual Vector3f CameraPosition() = 0;                                                    // editorCamera.transform.position
    virtual EditorRay ScreenPointToRay(const Vector3f& screenPoint) = 0;                      // editorCamera.ScreenPointToRay
    virtual Vector3f WorldToScreenPoint(const Vector3f& worldPoint) = 0;                      // editorCamera.WorldToScreenPoint
    virtual std::optional<EditorSurfaceHit> Raycast(const EditorRay& ray, float maxDistance, int layerMask) = 0;   // Physics.Raycast + GetPartUpwardsCached
    virtual bool NodeIconUnderCursor(Part& part, AttachNode& node, const Vector3f& worldPointOfSelectedNode) = 0;  // attachNode.icon renderer bounds test
    virtual float FindPartSurface(Part& p, const Vector3f& origin, const Vector3f& direction, const Vector3f& fromParent, const EditorSurfaceHit& hit,
                                  Vector3f& srfNormal) = 0;                                   // EditorLogicBase.FindPartSurface (Collider.Raycast on the part colliders)
    virtual bool PointerOverUI() { return false; }                                            // EventSystem.current.IsPointerOverGameObject()
    virtual bool MouseOverModalArea() { return false; }                                       // EditorLogic.mouseOverModalArea
    virtual void PlayOneShot(EditorClip) {}                                                   // audioSource.PlayOneShot
};


// Wires an IEditorScene to the EditorEnvironment seams (the scene must outlive the environment's use).
inline void InstallEditorScene(EditorEnvironment& env, IEditorScene& scene) {
    IEditorScene* sc = &scene;
    env.cameraPosition = [sc] { return sc->CameraPosition(); };
    env.screenPointToRay = [sc](const Vector3f& p) { return sc->ScreenPointToRay(p); };
    env.worldToScreenPoint = [sc](const Vector3f& p) { return sc->WorldToScreenPoint(p); };
    env.raycast = [sc](const EditorRay& r, float d, int m) { return sc->Raycast(r, d, m); };
    env.nodeIconUnderCursor = [sc](Part& p, AttachNode& n, const Vector3f& w) { return sc->NodeIconUnderCursor(p, n, w); };
    env.findPartSurface = [sc](Part& p, const Vector3f& o, const Vector3f& d, const Vector3f& f, const EditorSurfaceHit& h, Vector3f& n) {
        return sc->FindPartSurface(p, o, d, f, h, n);
    };
    env.pointerOverUI = [sc] { return sc->PointerOverUI(); };
    env.mouseOverModalArea = [sc] { return sc->MouseOverModalArea(); };
    env.playOneShot = [sc](EditorClip c) { sc->PlayOneShot(c); };
}

namespace KSP::UI::Screens {
// CraftBrowserDialog.cs:32 LoadType (completes the opaque declaration of events/game_event_types.hpp)
enum class CraftBrowserDialog_LoadType : int { Normal, Merge };
}  // namespace KSP::UI::Screens

}  // namespace ksp
