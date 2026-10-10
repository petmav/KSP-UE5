// WP-40: EditorLogic.cs (5,490 lines) + EditorLogicBase.cs -- the data / maths side of the part editor: the part-tree operations (attachPart /
// detachPart, symmetry creation and update, delete, duplicate, spawn), the symmetry mode / method state, the attachment search
// (checkAttach / CheckSymPartsAttach), the offset / rotate gizmo callbacks, the undo / redo stack (ShipConstruction.backups) and the ship
// queries (sortedShipList, AreAllPartsConnected, CountAllSceneParts) and the stock pre-flight check list.
//
// Not ported (the engine shell / UI): the KFSM state machine of the mouse-driven placement loop (EditorLogic.cs:1236-2366) and every
// callback that only drives widgets, audio, cameras, screen messages, Steam export, crew dialogs and the save / load / exit dialogs. The
// functions the loop calls are ported; the loop's composite steps that are pure data (what `on_partAttached`, `on_partPicked`, the
// st_place update and OnLeave do to the part tree) are exposed as DropSelectedPart / PickPart / UpdatePlacement.
//
// Unity transform model: a Part's transform is `PartTransform` (world position / rotation, parentless); the local position / rotation Unity
// derives from the transform parent (the logical `Part.parent`) are computed by LocalPosition / LocalRotation below (the native matrix
// rounding order is not captured -- see STATUS.md "WP-40").
//
// Seams: EditorEnvironment (cursor / physics / settings), EditorLimits (GameVariables).
#pragma once

#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "ksp/editor/editor_types.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/compound_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/ship/ship_construction.hpp"

namespace ksp {

class PreFlightCheck;

// EditorLogicBase.cs:8 -- the members that do not depend on the editor scene
class EditorLogicBase {
public:
    virtual ~EditorLogicBase() = default;

    Vector3f selPartGrabOffset{};          // :13
    Vector3f dragPlaneCenter{};            // :15
    Vector3f srfAttachCursorOffset{};      // :17

    // :330 FindPartsInChildren (depth first, the part first)
    static std::vector<Part*> FindPartsInChildren(Part* part);
    static void FindPartsInChildren(std::vector<Part*>& parts, Part* part);   // :334 (recursion)

    // :201 clearAttachNodes: drop the two attach node links between `part` and `otherPart`
    static void clearAttachNodes(Part* part, Part* otherPart);
    // :232 PartInHierarchy: `tgtPart` is a (direct or indirect) child of `part`
    bool PartInHierarchy(Part* part, Part* tgtPart) const;

    // :285 FindPartSurface (7-argument form): through EditorEnvironment::findPartSurface
    float FindPartSurface(Part& p, const Vector3f& origin, const Vector3f& direction, const Vector3f& fromParent, const EditorSurfaceHit& hit,
                          Vector3f& srfNormal) const;

protected:
    Part* selectedPart = nullptr;          // :19 protected
    Quaternionf stackRotation = Quaternionf::identity();   // :25
    bool partTweaked = false;              // :27
    bool isCurrentPartFlag = false;        // :35

public:
    bool IsCurrentPartFlag() const { return isCurrentPartFlag; }   // :55
    // :57 partRotationInputUpdate: the keyboard rotation of the selected part. `key` is the pressed command (GameSettings.Editor_yawLeft ...)
    enum class RotationKey { None, YawLeft, YawRight, RollLeft, RollRight, PitchUp, PitchDown };
    // `inRotateTweakState`: fsm.currentStateName == "st_rotate_tweak" (the keys are ignored there, only the reset runs)
    void partRotationInputUpdate(RotationKey key, bool inRotateTweakState);
    virtual void partRotationResetUpdate() {}   // :99

    // :104 displayAttachNodeIcons .. :149 AssignAttachIcon: which attach nodes carry an icon (the icon objects are the shell's; the set of
    // nodes that have one is what checkAttach tests)
    void displayAttachNodeIcons(const std::vector<Part*>& parts, Part* selected, bool stackNodes, bool srfNodes, bool dockNodes);
    bool HasNodeIcon(const AttachNode* n) const { return iconNodes_.count(n) != 0; }
    // the icon pose the shell draws (position = part.TransformPoint(node.position), up = TransformDirection(node.orientation))
    std::function<void(Part&, AttachNode&)> onNodeIconPose;

protected:
    void handleChildNodeIcons(Part* part);                                                  // :115
    void handleAttachNodeIcons(Part* part, bool stackNodes, bool srfNodes, bool dockNodes); // :129
    void AssignAttachIcon(Part* part, AttachNode& node, bool stackNodes, bool srfNodes, bool dockNodes);  // :139
    std::set<const AttachNode*> iconNodes_;
};

class EditorLogic : public EditorLogicBase {
public:
    // EditorLogic.EditorModes
    static inline EditorLogic* fetch = nullptr;   // :111 (the scene's instance)

    EditorLogic();
    ~EditorLogic();
    EditorLogic(const EditorLogic&) = delete;
    EditorLogic& operator=(const EditorLogic&) = delete;

    // ---- state (EditorLogic.cs:66-340) ---------------------------------------------------------------------------------------------
    Quaternionf vesselRotation = Quaternionf::identity();   // :66
    float srfAttachAngleSnap = 15.0f;                       // :70
    float srfAttachAngleSnapFine = 5.0f;                    // :72
    bool allowSrfAttachment = true;                         // :166
    bool allowNodeAttachment = true;                        // :168
    SymmetryMethod symmetryMethod = SymmetryMethod::Radial;  // :178
    int symmetryMode = 0;                                   // :182
    int symmetryModeTmp = 0;                                // :184
    std::string shipNameField, shipDescriptionField;        // shipNameField.text / shipDescriptionField.text
    std::string FlagURL;                                    // EditorLogic.FlagURL
    // the ship being edited (ShipConstruct: the live parts are `ship->liveParts`); owned by the editor (RestoreState replaces it)
    ShipConstruct* ship = nullptr;
    // the hooks of a scene without FlightGlobals (HighLogic.LoadedSceneIsEditor with FlightGlobals.fetch == null)

    Part* SelectedPart() const { return selectedPart; }     // :437 (static in C#: fetch.selectedPart)
    Part* RootPart() const { return rootPart; }             // :450
    void SetSelectedPart(Part* p) { selectedPart = p; }     // OnSubassemblyDialogDismiss :2829
    void SetRootPart(Part* p) { rootPart = p; }
    ConstructionMode EditorConstructionMode() const { return constructionMode; }   // :482
    void SetConstructionMode(ConstructionMode m) { constructionMode = m; }
    int UndoLevel() const { return undoLevel; }
    Attachment& CurrentAttachment() { return attachment; }
    std::vector<Attachment>& PartAttachments() { return cPartAttachments; }
    UnityEngine::Space symmetryCoordSpace = UnityEngine::Space::Self;   // :258
    bool tmpSymMethodInUse() const { return tmpSymMethodInUse_; }
    int SymmetryModeBeforeNodeAttachment() const { return symmetryModeBeforeNodeAttachment; }
    // gizmo scratch state of the placement loop (:300-335)
    int symUpdateMode = 0;                                  // :316
    Part* symUpdateParent = nullptr;                        // :318
    AttachNode* symUpdateAttachNode = nullptr;              // :320
    Vector3f gizmoPivot{};                                  // :322
    Quaternionf gizmoAttRotate = Quaternionf::identity();   // :254 (private)
    Quaternionf gizmoAttRotate0 = Quaternionf::identity();  // :256
    // the transform of the rotate / offset gizmo object (GizmoRotate / GizmoOffset are the shell's objects; their maths is in
    // editor_gizmos.hpp). coordSpace: gizmoRotate.CoordSpace
    struct GizmoState {
        UnityEngine::Space coordSpace = UnityEngine::Space::Self;
        PartTransform transform{};      // gizmo.transform
        Quaternionf hostRot0 = Quaternionf::identity();   // GizmoRotate.HostRot0
    };
    GizmoState gizmoRotate, gizmoOffset;
    bool inRotateTweakState = false;                        // fsm.CurrentState == st_rotate_tweak
    bool inOffsetTweakState = false;                        // fsm.CurrentState == st_offset_tweak

    // ---- symmetry mode / method (:1002-1217) ------------------------------------------------------------------------------------
    // symButton(evtData): `button` = nullopt (the left click: next mode), 0 left (next mode), 1 right (previous mode), 2 middle (toggle the method)
    enum class SymButton { Left, Right, Middle };
    void symButton(SymButton button = SymButton::Left);     // :1002
    void nextSymMode();                                     // :1040
    void prevSymMode();                                     // :1070
    void SetSymState();                                     // :1100 (RestoreSymmetryState; the sprites are the shell's)
    void ToggleSymMethod();                                 // :1157 (symInputUpdate: Editor_toggleSymMethod key)
    void ToggleSymmetryCoordSpace();                        // :1177 (symInputUpdate: Editor_coordSystem key, Radial method only)
    void onSymMethodChange(SymmetryMethod method);          // :1197

    // ---- symmetry maths (:3140-3520) ---------------------------------------------------------------------------------------------
    void RestoreSymmetryState();                            // :3140
    void RestoreSymmetryModeBeforeNodeAttachment();         // :3157
    void createSymmetry(int mode, Attachment& attach);      // :3164
    void UpdatePartAndChildren(Part* SourcePart, Part* UpdatePart, SymmetryMethod SymMethod);   // :3276
    void UpdateSymmetry(Part* selPart, int symMode, Part* partParent, AttachNode* selPartNode); // :3315
    void UpdateChildMirrorSymmetry(Part* part, Part* mirrorRoot, int symIdx);                   // :3391
    void cleanSymmetry();                                   // :3422
    void CheckCopiedSymmetryPersistentId(Part* selected);   // :3443
    void wipeSymmetry(Part* selPart);                       // :3464
    void deleteSymmetryParts();                             // :3487
    void deletePartAndSymmetryParts(Part* part, bool symmetry = false);   // :3509
    static Part* FirstNonSymmetricalParentFrom(Part* p);    // :3519

    // ---- the attachment search and the tree operations (:3528-3966) ---------------------------------------------------------------
    Attachment checkAttach(Part* selPart);                  // :3528
    bool checkEditorCollision(Attachment& attach);          // :3686
    std::vector<Attachment> CheckSymPartsAttach(const Attachment& oAttach);   // :3718
    void attachPart(Part* part, Attachment& attach);        // :3835
    void detachPart(Part* part);                            // :3874
    void CopyActionGroups(Part* sPart, Part* cPart);        // :3889
    bool attachSymParts(std::vector<Attachment>& cAttaches);   // :3913
    void addToShip(Part* part);                             // :3925
    void removeFromShip(Part* part);                        // :3949
    void highlightSelected(bool attach, Part* part, bool reset = false);   // :3968

    // ---- create / duplicate / delete (:2866-3130) ---------------------------------------------------------------------------------
    // OnPodSpawn :2866 / SpawnPart :2906: Instantiate(partInfo.partPrefab), persistentId, selectedPart (root part for the first part)
    Part* OnPodSpawn(const AvailablePart& pod);
    Part* SpawnPart(const AvailablePart& partInfo);
    void SetIconAsPart(Part* part);                         // :2946
    void SpawnConstruct(ShipConstruct* construct);          // :2996 (ArgumentNullException for null)
    Part* DuplicatePart(Part* p);                           // :3028
    static void DeletePart(Part* part);                     // :3049
    void deletePart(Part* part, bool symmetry = false, bool printConfirmation = false);   // :3064
    void DestroySelectedPart();                             // :3101
    // the live-object check Unity's `== null` does on a destroyed UnityEngine.Object
    bool IsDestroyed(const Part* p) const { return p == nullptr || destroyed_.count(p) != 0; }

    // ---- composite steps of the placement loop (data only) -------------------------------------------------------------------------
    // st_place.OnUpdate (:1380-1440) after the cursor moved the part: attachment = checkAttach, position, potentialParent bookkeeping,
    // createSymmetry / deleteSymmetryParts, cPartAttachments = CheckSymPartsAttach, node icons
    void UpdatePlacement();
    // st_place.OnLeave (:1900-1925): cleanSymmetry, CheckCopiedSymmetryPersistentId, PartPlaced
    void LeavePlace(bool toIdle);
    // on_partAttached.OnEvent (:1940-1980): attach the selected part and its symmetry counterparts when every attachment is possible;
    // false = the drop was refused (the shell plays the cannot-place clip)
    bool DropSelectedPart();
    // on_partPicked.OnEvent (:1770-1800) for a part of the ship: detachPart, deleteSymmetryParts, backup
    void PickPart(Part* part);

    // ---- gizmo callbacks (:2445-2600) --------------------------------------------------------------------------------------------
    void onRotateGizmoUpdate(const Quaternionf& dRot);      // :2445
    void onRotateGizmoUpdated(const Quaternionf& dRot);     // :2471
    void partRotationResetUpdate() override;                // :2480 (the resetRotation key was pressed with the part's own transform as reference)
    static Vector3f getPivotOffset(const Vector3f& pivot0, const Vector3f& pivot) { return pivot0 - pivot; }   // :2514
    void onOffsetGizmoUpdate(const Vector3f& dPos);         // :2519
    void onOffsetGizmoUpdated(const Vector3f& dPos);        // :2563
    void partOffsetInputUpdate(bool resetKeyPressed);       // :2576
    void OnNewRootSelect(Part* newRoot);                    // :2602

    // ---- undo / redo (:2675-2830) -----------------------------------------------------------------------------------------------------
    void SetBackup();                                       // :2675
    void ResetBackup();                                     // :2701
    // :2771 RestoreState(offset): offset -1 undo, +1 redo. `inPodSelect`: fsm.CurrentState == st_podSelect
    void RestoreState(int offset, bool inPodSelect = false);
    // the editor's ship after RestoreState / NewShip (the editor owns it)
    ShipConstruct* AdoptShip(std::unique_ptr<ShipConstruct> s);

    // ---- ship queries (:5380-5430) -------------------------------------------------------------------------------------------------------
    std::vector<Part*> getSortedShipList();                 // :5380
    static std::vector<Part*> SortedShipList();             // :464
    bool AreAllPartsConnected();                            // :5399
    int CountAllSceneParts(bool includeSelected);           // :5421
    bool ShipIsValid(const ShipConstruct* s) const;         // :5429
    Vector3f initialPodPosition() const;                    // :414
    Bounds editorBounds() const;                            // :434 (EditorBounds.Instance)

    // ---- launch checks (:4993) ----------------------------------------------------------------------------------------------------------
    // GetStockPreFlightCheck(siteName): the stock list of tests the Launch button runs
    std::unique_ptr<PreFlightCheck> GetStockPreFlightCheck(const std::string& siteName);
    std::function<void()> proceedWithVesselLaunch, abortLaunch;   // :5126 / :5168 (the shell's launch continuation)
    std::string launchSiteDisplayName(const std::string& siteName) const { return siteName; }

    // ---- Unity transform helpers (Part.transform.parent / localPosition / localRotation) --------------------------------------------
    static Vector3f LocalPosition(const Part& p);           // transform.localPosition: parent.InverseTransformPoint(world)
    static Quaternionf LocalRotation(const Part& p);        // transform.localRotation: Inverse(parent.rotation) * rotation (native)
    static void SetLocalPosition(Part& p, const Vector3f& v);
    static void SetLocalRotation(Part& p, const Quaternionf& q);
    static void SetRotation(Part& p, const Quaternionf& q);   // transform.rotation = q (Unity normalises on set)
    static PartTransform& ReferenceTransform(Part& p);      // Part.GetReferenceTransform()

private:
    // st_place state (:300-340)
    Part* rootPart = nullptr;
    ConstructionMode constructionMode = ConstructionMode::Place;
    Attachment attachment;
    std::vector<Attachment> cPartAttachments;
    SymmetryMethod symmetryMethodTmp = SymmetryMethod::Radial;   // :180
    int symmetryModeBeforeNodeAttachment = -1;              // :186
    bool tmpSymMethodInUse_ = false;                        // :188
    int undoLevel = 0;                                      // :88
    std::unique_ptr<ShipConstruct> shipOwned_;
    std::vector<std::unique_ptr<Part>> editorParts_;        // the Parts the editor created (Instantiate)
    std::vector<std::unique_ptr<ShipConstruct>> oldShips_;  // ships replaced by RestoreState (DestroyImmediate; kept alive)
    CompoundPart* selectedCompoundPart = nullptr;           // EditorLogicBase.cs:21
    std::unordered_set<const Part*> destroyed_;

    Part* InstantiateCopy(Part* p);                         // recursive Object.Instantiate of the part and its child hierarchy
    void Destroy(Part* p);                                  // Object.Destroy(part.gameObject)
    void RegisterEditorPart(std::unique_ptr<Part> p);
};

}  // namespace ksp
