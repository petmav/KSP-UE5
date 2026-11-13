// WP-40: EditorLogic.cs (5,490 lines) + EditorLogicBase.cs -- the data / maths side of the part editor: the part-tree operations (attachPart /
// detachPart, symmetry creation and update, delete, duplicate, spawn), the symmetry mode / method state, the attachment search
// (checkAttach / CheckSymPartsAttach), the offset / rotate gizmo callbacks, the undo / redo stack (ShipConstruction.backups) and the ship
// queries (sortedShipList, AreAllPartsConnected, CountAllSceneParts) and the stock pre-flight check list.
//
// WP-40b: the KFSM placement loop (EditorLogic.cs:1236-2366 SetupFSM: the nine states st_podSelect .. st_root_select and the 28 on_* events) is ported in
// src/editor/editor_fsm.cpp onto the KerbalFSM, with the drivers (Update / LateUpdate / FixedUpdate), pickPart / dragOverPlane, the key handlers
// (symmetry, angle snap, undo / redo, delete, offset / rotate reset), StartEditor / NewShip / load and the part of EditorToolsUI that reacts to part
// events and the 1-4 keys. Scene services the C# gets from Unity are EditorEnvironment seams (camera, Physics.Raycast hits, node icon bounds,
// FindPartSurface, UI hover, audio clips, tweak hooks); keys and the mouse come from the WP-50 input device, Mouse, GameSettings and InputLockManager.
// Not ported (the engine shell): every callback that only drives widgets, the part list and categorizer, crew dialogs, Steam export, the
// save / load / exit dialogs, the flag browser, highlighting and layers, the compound part tweak mode, the inventory (cargo) UI.
// The composite steps below (UpdatePlacement, LeavePlace, DropSelectedPart, PickPart) predate the loop and stay for hosts that drive the part tree
// without a FSM.
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

#include "ksp/editor/editor_gizmos.hpp"
#include "ksp/editor/editor_types.hpp"
#include "ksp/fsm/kerbal_fsm.hpp"
#include "ksp/input/control_types.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/compound_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/staging/stage_manager.hpp"

namespace ksp {

class PreFlightCheck;

// EditorLogicBase.cs:7 -- the members that do not depend on the editor scene
class EditorLogicBase {
public:
    virtual ~EditorLogicBase() = default;

    Vector3f selPartGrabOffset{};          // :13
    Vector3f dragPlaneCenter{};            // :15
    Vector3f srfAttachCursorOffset{};      // :17

    // :330 FindPartsInChildren (depth first, the part first)
    static std::vector<Part*> FindPartsInChildren(Part* part);
    static void FindPartsInChildren(std::vector<Part*>& parts, Part* part);   // :292 (recursion)

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
    bool IsCurrentPartFlag() const { return isCurrentPartFlag; }   // :73
    // EditorLogicBase.cs:10 / :33 / :35 (the placement loop's state machine, the lock switch and the screen the editor shows)
    KerbalFSM fsm;
    bool checkInputLocks = true;
    EditorScreen editorScreen = EditorScreen::Parts;
    // :75 partRotationInputUpdate() as the FSM states call it: the keys come from GameSettings.Editor_* through the input device (WP-50),
    // the lock / screen guard, the keyboard rotation, partRotationResetUpdate (virtual) and the PartRotated event (WP-40b)
    void partRotationInputUpdate();
    // :57 partRotationInputUpdate: the keyboard rotation of the selected part. `key` is the pressed command (GameSettings.Editor_yawLeft ...)
    enum class RotationKey { None, YawLeft, YawRight, RollLeft, RollRight, PitchUp, PitchDown };
    // `inRotateTweakState`: fsm.currentStateName == "st_rotate_tweak" (the keys are ignored there, only the reset runs)
    void partRotationInputUpdate(RotationKey key, bool inRotateTweakState);
    virtual void partRotationResetUpdate() {}   // :122

    // :104 displayAttachNodeIcons .. :149 AssignAttachIcon: which attach nodes carry an icon (the icon objects are the shell's; the set of
    // nodes that have one is what checkAttach tests)
    void displayAttachNodeIcons(const std::vector<Part*>& parts, Part* selected, bool stackNodes, bool srfNodes, bool dockNodes);
    bool HasNodeIcon(const AttachNode* n) const { return iconNodes_.count(n) != 0; }
    // the icon pose the shell draws (position = part.TransformPoint(node.position), up = TransformDirection(node.orientation))
    std::function<void(Part&, AttachNode&)> onNodeIconPose;

protected:
    void handleChildNodeIcons(Part* part);                                                  // :137
    void handleAttachNodeIcons(Part* part, bool stackNodes, bool srfNodes, bool dockNodes); // :129
    void AssignAttachIcon(Part* part, AttachNode& node, bool stackNodes, bool srfNodes, bool dockNodes);  // :157
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
    Quaternionf vesselRotation = Quaternionf::identity();   // :45
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

    Part* SelectedPart() const { return selectedPart; }     // :414 (static in C#: fetch.selectedPart)
    Part* RootPart() const { return rootPart; }             // :426
    void SetSelectedPart(Part* p) { selectedPart = p; }     // OnSubassemblyDialogDismiss :2829
    void SetRootPart(Part* p) { rootPart = p; }
    ConstructionMode EditorConstructionMode() const { return constructionMode; }   // :450
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
    Quaternionf gizmoAttRotate = Quaternionf::identity();   // :45 (private)
    Quaternionf gizmoAttRotate0 = Quaternionf::identity();  // :45
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
    // on_partPicked.OnEvent (EditorLogic.cs:1776) on the already selected part (the FSM event body)
    void OnPartPickedEvent();

    // ---- gizmo callbacks (:2445-2600) --------------------------------------------------------------------------------------------
    void onRotateGizmoUpdate(const Quaternionf& dRot);      // :2445
    void onRotateGizmoUpdated(const Quaternionf& dRot);     // :2471
    void partRotationResetUpdate() override;                // :2480 (the resetRotation key was pressed with the part's own transform as reference)
    static Vector3f getPivotOffset(const Vector3f& pivot0, const Vector3f& pivot) { return pivot0 - pivot; }   // :2514
    void onOffsetGizmoUpdate(const Vector3f& dPos);         // :2519
    void onOffsetGizmoUpdated(const Vector3f& dPos);        // :2563
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
    bool AreAllPartsConnected();                            // :5407
    int CountAllSceneParts(bool includeSelected);           // :5421
    bool ShipIsValid(const ShipConstruct* s) const;         // :5429
    Vector3f initialPodPosition() const;                    // :376
    Bounds editorBounds() const;                            // :402 (EditorBounds.Instance)

    // ======================================================================================================================================
    // WP-40b: the KFSM placement loop (EditorLogic.cs:1236-2366) and what drives it. The scene services are EditorEnvironment seams; the
    // input comes from the WP-50 input device (InputEnvironment::Input(), Mouse, GameSettings.Editor_* bindings, InputLockManager).
    // ======================================================================================================================================

    // EditorDriver.StartupBehaviours: what StartEditor loads
    enum class StartupBehaviour : int { START_CLEAN, LOAD_FROM_CACHE, LOAD_FROM_FILE };
    StartupBehaviour startupBehaviour = StartupBehaviour::START_CLEAN;   // EditorDriver.StartupBehaviour
    std::string filePathToLoad;                                           // EditorDriver.filePathToLoad
    bool restartingEditor = false;                                        // EditorDriver.fetch.restartingEditor

    // EditorLogic.cs:97-160 (the states and events; public so a host or test can read the state and run the MANUAL_TRIGGER events)
    KFSMState st_podSelect{"st_podSelect"}, st_idle{"st_idle"}, st_place{"st_place"}, st_offset_select{"st_offset_select"},
        st_offset_tweak{"st_offset_tweak"}, st_rotate_select{"st_rotate_select"}, st_rotate_tweak{"st_rotate_tweak"},
        st_root_unselected{"st_root_unselected"}, st_root_select{"st_root_select"};
    KFSMEvent on_podSelect{"on_podSelect"}, on_podDeleted{"on_podDeleted"}, on_partCreated{"on_partCreated"}, on_partPicked{"on_partPicked"},
        on_partCopied{"on_partCopied"}, on_partReveal{"on_partReveal"}, on_partDropped{"on_partDropped"}, on_partAttached{"on_partAttached"},
        on_partDeleted{"on_partDeleted"}, on_partLost{"on_partLost"}, on_partOverInventoryPAW{"on_partOverInventoryPAW"},
        on_goToModeOffset{"on_goToModeOffset"}, on_offsetSelect{"on_offsetSelect"}, on_offsetDeselect{"on_offsetDeselect"},
        on_offsetReset{"on_offsetReset"}, on_goToModeRotate{"on_goToModeRotate"}, on_rotateSelect{"on_rotateSelect"},
        on_rotateDeselect{"on_rotateDeselect"}, on_rotateReset{"on_rotateReset"}, on_goToModeRoot{"on_goToModeRoot"},
        on_rootPickSet{"on_rootPickSet"}, on_rootDeselect{"on_rootDeselect"}, on_rootSelect{"on_rootSelect"},
        on_rootSelectFail{"on_rootSelectFail"}, on_goToModePlace{"on_goToModePlace"}, on_undoRedo{"on_undoRedo"}, on_newShip{"on_newShip"},
        on_shipLoaded{"on_shipLoaded"};
    float dragSharpness = 0.3f;                             // :47
    bool skipPartAttach = false;                            // :234
    std::vector<Part*> rootCandidates;                      // :129 (the PartSelector targets; the selector widgets belong to the shell)
    int undoIndexAtLastSave = 0;                            // :97
    std::string vesselNameAtLastSave;                       // :99
    ConstructionMode toolsConstructionMode = ConstructionMode::Place;   // EditorToolsUI.cs:25 (constructionMode of the tools panel)

    // lifetime (EditorLogic.cs:466 Awake, :519 OnDestroy, :540 Start)
    void Awake();                                           // :466 (the GameEvents listeners that drive the loop)
    void OnDestroy();                                       // :519
    void Start();                                           // :545: SetupFSM, StartEditor(false), fsm.StartFSM(rootPart ? st_idle : st_podSelect)
    void SetupFSM();                                        // :1236
    void Update();                                          // :2375 (fsm.UpdateFSM; the host calls Mouse::Update() first, like the Mouse behaviour)
    void LateUpdate();                                      // :2407
    void FixedUpdate();                                     // :2367
    // the coroutines EditorLogic starts (CallbackUtil.DelayedCallback(1, ...) in RestoreState): the host calls this once per frame after
    // Update, like Mouse::ResumeCoroutines (a coroutine started inside Update first resumes in the next frame pass)
    void RunCoroutines();
    void StartEditor(bool isRestart);                       // :606
    void NewShip();                                         // :4023 (the unsaved-changes dialog is EditorEnvironment::newShipDialog)
    void OnNewShipConfirm();                                // :4046
    void ShipToLoadSelected(const std::string& path, KSP::UI::Screens::CraftBrowserDialog_LoadType loadType);   // :4768
    void ShipToLoadSelected(const ConfigNode::NodePtr& node, KSP::UI::Screens::CraftBrowserDialog_LoadType loadType);   // :4804
    static void LoadShipFromFile(const std::string& path); // :4760

    // the part list (EditorLogic.cs:2834-2864): OnPartListIconTap spawns, or drops the held part when one is held
    void OnPartListIconTap(const AvailablePart& p);         // :2834
    void OnPartListBackgroundTap();                         // :2858
    void ReleasePartToIcon();                               // :2977

    // EditorToolsUI.cs (the construction-mode panel: key shortcuts, SetMode, the part-event reactions); the buttons belong to the shell
    void toolsSetMode(ConstructionMode mode, bool updateUI = true);   // EditorToolsUI.cs:126
    void toolsUpdate();                                     // EditorToolsUI.cs:103 (Editor_mode* keys)
    void toolsOnEditorPartEvent(ConstructionEventType evt, Part* part);   // EditorToolsUI.cs:217
    void onConstructionModeChanged(ConstructionMode mode);  // :2420

    // input helpers of the states (EditorLogic.cs:1133 symInputUpdate, :1189 snapInputUpdate, :2708 UndoRedoInputUpdate, :3131 deleteInputUpdate,
    // :5365 partSearchUpdate)
    void snapButton();                                      // :1125
    void symInputUpdate();
    void snapInputUpdate();
    void UndoRedoInputUpdate();
    void deleteInputUpdate();
    void partSearchUpdate() {}                              // PartCategorizer search field: the shell
    void partOffsetInputUpdate();                           // :2576 (reads Editor_resetRotation itself)
    // the drag plane and the picking ray (:2621-2673)
    Part* pickPart(int layerMask, bool pickRoot, bool pickRootIfFrozen);
    void CenterDragPlane(const Vector3f& pos);
    bool dragOverPlane(Vector3f& position);
    bool HeldPartIsStacked();                               // :5439
    // Part.frozen / freeze() / unfreeze() (Part.cs:199, :2407, :2422; the Part class has no such field: the editor owns the flag)
    bool IsFrozen(const Part* p) const { return frozenParts_.count(p) != 0; }
    void freeze(Part* p);
    void unfreeze(Part* p);
    // the live gizmos (GizmoOffset.Attach / GizmoRotate.Attach): null while no tweak state is active; the shell drives the handles through them
    EditorGizmos::GizmoOffset* OffsetGizmo() { return gizmoOffsetObj_.get(); }
    EditorGizmos::GizmoRotate* RotateGizmo() { return gizmoRotateObj_.get(); }
    int FramesInCurrentState() const { return fsm.FramesInCurrentState(); }
    // every Part the editor created or loaded (Instantiate / ShipConstruct), in creation order, destroyed ones included
    std::vector<Part*> SceneParts() const;
    // layerMask = 67109122 + LayerUtil.DefaultEquivalent (:498)
    int layerMask = 67109122 + 8519681;

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
    CompoundPart* selectedCompoundPart = nullptr;
    std::unordered_set<const Part*> frozenParts_;
    std::unique_ptr<EditorGizmos::GizmoOffset> gizmoOffsetObj_;
    std::unique_ptr<EditorGizmos::GizmoRotate> gizmoRotateObj_;
    struct PendingCallback {
        int framesLeft;
        std::function<void()> fn;
    };
    std::vector<PendingCallback> coroutines_;
    bool awakened_ = false;
    bool attachmentSet_ = false;                            // EditorLogic.attachment != null (the C# field is a class reference)
    std::vector<std::unique_ptr<ShipConstruct>> mergedConstructs_;   // constructs read by the Merge load (SpawnConstruct)
    void SyncReRootContext();
    bool attachmentPossibleOrThrow();
    void AttachOffsetGizmo(Part& part);
    void AttachRotateGizmo(Part& part);
    void GizmoStateFromObject(bool rotate);
    void GizmoStateToObject(bool rotate);           // EditorLogicBase.cs:27
    std::unordered_set<const Part*> destroyed_;

    Part* InstantiateCopy(Part* p);                         // recursive Object.Instantiate of the part and its child hierarchy
    void Destroy(Part* p);                                  // Object.Destroy(part.gameObject)
    void RegisterEditorPart(std::unique_ptr<Part> p);
};

// The IStagingHost of an editor scene driven by an EditorLogic (StageManager reads EditorLogic.RootPart / SelectedPart through it)
class EditorLogicStagingHost : public IStagingHost {
public:
    explicit EditorLogicStagingHost(EditorLogic& e) : editor_(&e) {}
    bool LoadedSceneIsFlight() const override { return false; }
    bool LoadedSceneIsEditor() const override { return true; }
    Part* RootPart() override { return editor_->RootPart(); }
    Part* SelectedPart() override { return editor_->SelectedPart(); }

private:
    EditorLogic* editor_;
};

}  // namespace ksp
