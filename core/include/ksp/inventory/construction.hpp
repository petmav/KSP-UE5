// WP-33: the EVA construction package -- EVAConstructionUtil.cs, EVAConstructionModeController.cs, EVAConstructionModeEditor.cs,
// EVAConstructionToolsUI.cs, PartItemTransfer.cs, and the headless models of the inventory UI: UIPartActionControllerInventory.cs,
// UIPartActionInventorySlot.cs, UIPartActionInventory.cs, InventoryPanelController.cs. ConstructionEventType.cs / ConstructionMode.cs /
// IConstruction.cs were ported by WP-40 / WP-13 (editor/editor_types.hpp, parts/part_types.hpp) and are covered by the tests here.
//
// Ported: the construction weight limit (EVAConstructionUtil.GetConstructionGee, Part.IsUnderConstructionWeightLimit), assisting-kerbal
// combine, CanPartBeEdited, the Place / Move / Rotate / Root mode state machine, attach search (CheckAttach: stack node match, surface
// pose and angle snap), placement validity, attach / detach / drop / pickup bookkeeping, dropped-part vessel nodes, the offset / rotate
// gizmo maths, the inventory panels' part registry (SearchForInventoryParts / UpdateDisplayedInventories / UpdateInventoryLists), the
// info-label numbers, the slot-click transfer rules (store / stack / swap with a held part, stack size, held-stack handling), and the
// PartItemTransfer valid / semi-valid part selection.
//
// Seams (documented stand-ins; the UI is tier C): every Unity widget / audio / highlight / camera / raycast / collider call is routed
// through EVAConstructionEnvironment (named effects + hooks, unset = the object is absent). The effect names are listed at each use.
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "ksp/editor/editor_logic.hpp"
#include "ksp/editor/editor_types.hpp"
#include "ksp/inventory/inventory.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/vector.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

// EditorScreen.cs:1 (completes the opaque declaration of events/game_event_types.hpp)
enum class EditorScreen : int { Parts, Actions, Crew, Cargo };

class Vessel;
class KerbalEVA;
class EVAConstructionModeEditor;
class EVAConstructionToolsUI;
class UIPartActionInventorySlot;

// ================================================================ seams

struct EVAConstructionEnvironment {
    // named engine effects: "audio:<clip>", "highlight", "colliders:trigger", "gizmo:offset", "gizmo:rotate", ... (part may be null)
    static inline std::vector<std::string> effects;
    static inline std::function<void(const std::string& effect, Part* part)> onEffect;
    static void Effect(const std::string& name, Part* part = nullptr);
    // mouse / UI state the placement code polls
    static inline Part* hoveredPart = nullptr;               // Mouse.HoveredPart
    static inline bool mouseButtonUp0 = false;               // Input.GetMouseButtonUp(0)
    static inline bool pointerOverUI = false;                // EventSystem.current.IsPointerOverGameObject()
    static inline bool editorSoftLock = false;               // InputLockManager.IsLocked(ControlTypes.EDITOR_SOFT_LOCK)
    static inline bool mouseIsMoving = false;                // Mouse.IsMoving
    // the ground / cursor geometry of UpdatePartPlacementPosition
    struct GroundHit { Vector3f point, normal; };
    static inline std::function<std::optional<GroundHit>()> groundRaycast;       // Physics.Raycast(ray, 10000, 32768) from the cursor
    static inline std::function<Vector3d(const Vector3d& position)> upAxisAt;     // FlightGlobals.getUpAxis(mainBody, position); unset: FlightMath
    static inline std::function<std::optional<Vector3f>()> cameraPlanePoint;     // plane(camera forward, ReferenceTransform).Raycast(cursor ray)
    // CheckAttach's surface branch: Physics.RaycastAll sorted by distance (nearest first)
    static inline std::function<std::vector<EditorSurfaceHit>()> surfaceRaycastAll;
    // attachNode.icon bounds test (see EditorEnvironment::nodeIconUnderCursor)
    static inline std::function<bool(Part& part, AttachNode& node, const Vector3f& worldPointOfSelectedNode)> nodeIconUnderCursor;
    static inline std::function<float(Part&, const Vector3f& normal, float& centerPointOffset)> getBoundsPoints;  // Part.GetBoundsPoints
    static inline std::function<bool(Part&)> cargoColliderHitsOtherPart;     // unused hook slot (kept for the shell)
    // Part.cargoColliders walk of UpdatePartPlacementPosition: (colliderIndex) -> the Part upwards from the collider (nullptr: none)
    static inline std::function<Part*(Part&, size_t)> cargoColliderPart;
    static inline std::function<size_t(Part&)> cargoColliderCount;
    // ProtoVessel creation of a dropped part
    static inline std::function<ProtoVessel*(const ConfigNode&)> addVessel;   // HighLogic.CurrentGame.AddVessel
    static inline std::function<void(ProtoVessel*)> setPhysicsHoldExpiryOverride;  // unloaded vessel .SetPhysicsHoldExpiryOverride()
    static inline std::function<Quaternionf()> bodyRotation;                  // mainBody.bodyTransform.rotation
    static inline std::function<Vector3d(Vessel&)> orbitalPos;                // vessel.orbit.pos
    static inline std::function<Vector3f()> activeVesselWorldPos;             // FlightGlobals.ActiveVessel.GetWorldPos3D()
    static inline std::function<uint32_t()> uniqueFlightID;                   // ShipConstruction.GetUniqueFlightID(flightState)
    static inline std::function<Orbit(Vessel&)> currentOrbit;                 // vessel.GetCurrentOrbit() (landed: Orbit.CreateRandomOrbitAround)
    static inline std::function<void()> interruptWeld;                        // evaController.InterruptWeld()
    static inline std::function<void(Part&)> weld;                            // evaController.Weld(part2)
    static inline std::function<bool(Vessel&)> isSeated;                      // vessel.evaController.IsSeated()
    static inline std::function<int(Vessel&)> engineerCount;                  // VesselUtilities.VesselCrewWithTraitCount("Engineer", vessel)
    static void Reset();
};

// ================================================================ EVAConstructionUtil (EVAConstructionUtil.cs:1)

struct EVAConstructionUtil {
    static double GetConstructionGee(Vessel* vessel);        // :3
    static bool IsUnderConstructionWeightLimit(Part& part);  // Part.cs:4060 (the weight test; the part's cached partWeight is updated)
};

// ================================================================ EVAConstructionModeController (EVAConstructionModeController.cs:12)

class EVAConstructionModeController {
public:
    enum class PanelMode : int { Construction, Cargo };  // :24

    struct InventoryDisplayItem {  // :14
        ModuleInventoryPart* inventoryModule = nullptr;
        bool hasUiObject = false;
        UIPartActionInventory* uiInventory = nullptr;
    };

    static inline EVAConstructionModeController* Instance = nullptr;  // :29
    EVAConstructionModeEditor* evaEditor = nullptr;                    // :31
    EVAConstructionToolsUI* evaToolsUI = nullptr;                      // :33
    PanelMode panelMode = PanelMode::Construction;                      // :88
    // the last text values of the info labels (UpdateInfoLabels)
    std::string AssistingKerbalsLabelText, AssistingKerbalsNumberText, MaxMassLimitNumberText;

    // UI hooks (transitions, buttons, input locks, navball, StageManager, camera highlighter); unset = absent
    struct Hooks {
        std::function<bool()> mapIsEnabled;                 // MapView.MapIsEnabled
        std::function<bool()> flightDriverPause;            // FlightDriver.Pause
        std::function<bool()> actionGroupsOpen;             // ActionGroupsFlightController.Instance.IsOpen
        std::function<bool(Vessel&)> repairSkillAtLeastZero;  // vessel.VesselValues.RepairSkill.value >= 0 (unset: true)
        std::function<void(const std::string&)> ui;         // named UI effects: "transition:In", "lock:MAP_TOGGLE", ...
        std::function<void(bool)> evaEnterExitConstruction; // activeEVA.EnterConstructionMode / ExitConstructionMode (true = enter)
        std::function<bool()> activeEvaInConstructionMode;  // activeEVA.InConstructionMode
        std::function<bool(Vessel&)> hasEvaController;      // vessel.evaController != null
    };
    static inline Hooks hooks;

    EVAConstructionModeController();
    ~EVAConstructionModeController();
    bool IsOpen() const { return isOpen; }           // :115
    bool Hover() const { return hover; }             // :117
    static bool MovementRestricted();                // :119
    void Start();                                    // :147 (event subscriptions)
    void OnDestroy();                                // :168
    void Update();                                   // :189
    void OpenConstructionPanel();                    // :241
    void OpenCargoPanel();                           // :290
    void ClosePanel();                               // :313
    bool CanOpenConstructionPanel();                 // :378
    bool CanOpenCargoPanel();                        // :388
    void OnGameSceneLoadRequested(GameScenes scene); // :398
    void OnVesselChange(Vessel* vessel);             // :408
    void OnEditorPartEvent(ConstructionEventType eventType, Part* p);  // :432
    void RemoveInvDisplayItem(Part* p);              // :482
    void OnActionGroupsOpened();                     // :504
    void SearchForInventoryParts();                  // :509
    void UpdateDisplayedInventories();               // :547
    void UpdateInfoLabels();                         // :604
    void DestroyHeldIcons(UIPartActionInventory* callingInventory);  // :622
    bool TryGetDisplayedInventory(ModuleInventoryPart* inventoryModule, InventoryDisplayItem*& displayItem);  // :632
    void OnPointerEnter() { hover = true; }          // :650
    void OnPointerExit() { hover = false; }          // :655
    double constructionGravity = 0.0;                // :84
    double lastConstructionGravity = 0.0;            // :86
    std::vector<std::pair<uint32_t, ModuleInventoryPart*>> loadedModuleInventoryPart;  // :52 Dictionary<uint, ModuleInventoryPart> (insertion order)
    std::vector<InventoryDisplayItem> displayedInventories;  // :54
    std::vector<std::unique_ptr<UIPartActionInventory>> uiInventories;  // the UI models SetupConstruction created
    Vessel* activeEVAVessel = nullptr;               // activeEVA's vessel (KerbalEVA is WP-32)
    bool navballPreviouslyOpen = false;
    std::function<Vector3f(ModuleInventoryPart&)> inventoryPosition;  // moduleInventoryPart.part.transform.position / kerbal-mode transform

private:
    bool isOpen = false;
    bool hover = false;
    bool eventsAdded_ = false;
    ModuleInventoryPart* Lookup(uint32_t id) const;
};

// ================================================================ EVAConstructionModeEditor (EVAConstructionModeEditor.cs:16)

class EVAConstructionModeEditor : public EditorLogicBase {
public:
    struct SelectedPartCollider {  // :18
        bool isTrigger = false;
        bool current = false;  // collider.isTrigger as the shell last set it
        explicit SelectedPartCollider(bool trigger) : isTrigger(trigger), current(trigger) {}
    };

    // ---- state (:31-:139) ----
    ConstructionMode evaConstructionMode = ConstructionMode::Place;  // :31
    Part* lastAttachedPart = nullptr;                                 // :33
    Part* rootPart = nullptr;                                         // :35
    Part* backedUpPart = nullptr;                                     // :56
    std::unique_ptr<Part> backedUpPartOwner;
    AttachNode backedUpAttachment;                                    // :58 (clone)
    AttachNode* backedUpParentAttachment = nullptr;                   // :60
    bool hasBackedUpAttachment = false;
    std::vector<Vessel*> attachVessels;                               // :69
    bool gizmoRotateActive = false, gizmoOffsetActive = false;        // gizmoRotate / gizmoOffset != null
    UnityEngine::Space gizmoRotateSpace = UnityEngine::Space::Self, gizmoOffsetSpace = UnityEngine::Space::Self;  // CoordSpace
    Quaternionf gizmoAttRotate = Quaternionf::identity(), gizmoAttRotate0 = Quaternionf::identity();  // :71 / :73
    bool coordSpaceButtonActive = false;                              // coordSpaceBtn.gameObject.activeSelf
    std::string coordSpaceText;                                       // coordSpaceText.text
    bool angleSnapSpriteState = false;
    bool skipPartAttach = false;                                      // :87
    bool allowSrfAttachment = true, allowNodeAttachment = true;       // :100 / :102
    bool attachSuccesful = false;                                     // :104
    bool isPlacementValid_ = false;                                   // :106
    bool isPlacementOnGround = false;                                 // :108
    Quaternionf selectedPartOriginalRotation = Quaternionf::identity();  // :110
    float placementGroundOffset = 0.1f;                               // :112
    Quaternionf vesselRotation = Quaternionf::identity();             // :114
    float dragSharpness = 0.3f;                                       // :116
    float srfAttachAngleSnap = 15.0f, srfAttachAngleSnapFine = 5.0f;  // :118 / :120
    std::vector<SelectedPartCollider> selectedPartColliders;          // :122
    int assistingKerbals = 0;                                         // :124
    double combinedConstructionWeightLimit = 0.0;                     // :126
    bool attachmentPossible = false;                                  // :129
    Attachment attachment;                                            // :96 (null = attachmentNull)
    bool attachmentNull = true;
    Vector3f offsetGap;                                               // :131
    float threshold = 0.0f;                                           // :133
    bool partRootSelected = false;
    Part* selectedCompoundPart = nullptr;                             // EditorLogicBase.selectedCompoundPart
    std::vector<std::string> screenMessages;                          // PostScreenMessage texts in order (modeMsg / interactMsg too)

    ConstructionMode EVAConstructionMode() const { return evaConstructionMode; }  // :141
    Part* SelectedPart() const { return selectedPart; }                           // :143
    void SetSelectedPartForTest(Part* p) { selectedPart = p; }
    Part* LastAttachedPart() const { return lastAttachedPart; }                   // :145
    Part* RootPart() const { return rootPart; }                                   // :147
    bool IsPlacementValid() const { return isPlacementValid_; }                   // :149
    int AssistingKerbals() const { return assistingKerbals; }                     // :151
    double CombinedConstructionWeightLimit() const { return combinedConstructionWeightLimit; }  // :153

    struct UpdateInput {  // the polled input of Update()
        bool snapToggleKeyDown = false, coordToggleKeyUp = false, pauseKeyDown = false;
        RotationKey rotationKey = RotationKey::None;
    };
    EVAConstructionModeEditor() = default;
    ~EVAConstructionModeEditor() override;
    void Awake();                                                // :155
    void OnDestroy();                                            // :244
    void Update(const UpdateInput& in);                          // :180
    void Update() { Update(UpdateInput{}); }
    void FixedUpdate();                                          // :232
    void OnCurrentMousePartChanged(Part* p);                     // :256
    void OnConstructionModeChanged(ConstructionMode mode);       // :309
    void OnGameAboutToQuicksave() { PutPartBackInInventory(); }  // :362
    void OnMapViewEntered() { PutPartBackInInventory(); }        // :367
    Enumerator ResetToPhysicalPart(Part* p);                     // :372
    void UpdatePartPlacementPosition();                          // :385
    void HighLightPossibleAttach(bool highlight);                // :489
    void PickupPartInput();                                      // :506
    void PickupPart();                                           // :514
    void RemoveAttachNodes();                                    // :566
    void ProcessAttachNodes();                                   // :579
    void UpdateVesselsInConstructionRange();                     // :608
    void OnVesselUnloaded(Vessel* vessel);                       // :644
    void AttachInput();                                          // :650
    void partRotationResetUpdate() override;                     // :675
    Part* AttachPart(Part* part, Attachment& attach);            // :686
    void SetModuleCargoPartValue(Part* part, const std::string& name, const std::string& value);  // :756
    void DetachInput();                                          // :765
    void DetachPart(Part* part);                                 // :781
    void CopyActionGroups(Part* sPart, Part* cPart);             // :803
    void OnPartDropped();                                        // :827
    void BackupPart(Part* part, Part* partParent, bool droppedPart = false);  // :835
    void RestoreLastAttachedPart();                              // :881
    void RestoreLastDroppedPart();                               // :923
    Attachment CheckAttach(Part* selPart);                       // :933
    void ForceDrop();                                            // :1115
    void DropPartInput();                                        // :1130
    void DropAttachablePart(const std::string& partName, const Vector3f& partPosition, const Quaternionf& rotation);  // :1138
    ConfigNode::NodePtr GetProtoVesselNode(const std::string& partName, const Vector3f& partPosition, const Quaternionf& rotation,
                                           Vessel& vessel, Part& part);   // :1169
    ConfigNode::NodePtr CreatePartNode(uint32_t id, Part& part, const AvailablePart* availablePart);  // :1245
    void UpdateAssistingKerbals();                               // :1265
    void ToggleRotateGizmo(bool activate);                       // :1311
    void OnRotateGizmoUpdate(const Quaternionf& dRot, const Quaternionf& gizmoRotation, const Quaternionf& hostRot0);  // :1342
    void OnRotateGizmoUpdated(const Quaternionf& dRot);          // :1359
    Vector3f GetPivotOffset(const Vector3f& pivot0, const Vector3f& pivot) { return pivot0 - pivot; }  // :1371
    void ToggleOffsetGizmo(bool activate);                       // :1376
    void OnOffsetGizmoUpdate(const Vector3f& dPos, Vector3f& gizmoPosition);  // :1405
    void OnOffsetGizmoUpdated(const Vector3f& dPos);             // :1444
    void CoordToggleInput(bool keyUp);                           // :1456
    void ToggleCoordSpaceButton(bool toggle);                    // :1464
    void ChangeCoordSpace();                                     // :1476
    void SnapInputUpdate(bool keyDown);                          // :1512
    void SnapButton();                                           // :1520
    void PutPartBackInInventory();                               // :1528
    void SelectPartInput();                                      // :1536
    void GetRootPart();                                          // :1564
    void FetchColliders(const std::vector<bool>& isTriggers);    // :1572
    void ToggleCollidersTrigger(bool toggle);                    // :1586
    void OnConstructionMode(bool opened);                        // :1601
    void DestroyHeldPart();                                      // :1629
    void BreakSymmetry(Part* part);                              // :1646
    void DestroyBackupPart();                                    // :1658
    bool CanPartBeEdited(Part& part, bool weightOnlyCheck);      // :1666
    void CannotInteract();                                       // :1696
    void InterruptKerbalWelding();                               // :1701
    void InputCheckDetachCompundPart(bool pauseDown);            // :1709
    void PlayAudioClip(const std::string& clip);                 // :1717
    void FetchColliders();                                       // :1572 with the part's own collider states from the shell (all non-trigger)

    // ownership of parts the editor created (Object.Instantiate / Destroy): Part objects live here until DestroyHeldPart
    std::vector<std::unique_ptr<Part>> ownedParts;
    Part* AdoptPart(std::unique_ptr<Part> p) {
        Part* raw = p.get();
        ownedParts.push_back(std::move(p));
        return raw;
    }
    std::vector<std::unique_ptr<Part>> pendingDestroy;  // Object.Destroy'ed parts, freed at the end of the frame
    void ReleaseOwned(Part* p);
    void FlushDestroyed();                                // end of frame: frees pendingDestroy
    static inline std::function<void(Part&, const AttachNode&)> onAttachFlight;  // unused slot (reserved)
    std::function<std::unique_ptr<Part>(ProtoPartSnapshot&)> createPartFromSnapshot;  // protoPartSnapshot.CreatePart() (unset: ProtoPartSnapshot::CreatePart)
    std::string cacheAutoLOC_6006095 = "#autoLOC_6006095";
    std::vector<std::unique_ptr<ProtoPartSnapshot>> ownedSnapshots;   // snapshots the editor created (`new ProtoPartSnapshot(part, vessel)`)
    std::string lastHighlightColor;                                   // HighLightPossibleAttach's colour name
    bool resetRotationKeyDown = false, selectedPartIsReferenceTransform = true;  // Editor_resetRotation.GetKeyDown(); transform == GetReferenceTransform()
    std::string FairingHitColliderName;                               // FairingHitCollider.name (:93)
    Vector3f diff;                                                    // :139
    std::function<std::vector<bool>(Part*)> colliderTriggers;         // the part's colliders' isTrigger flags (FetchColliders)
    void PostMessage(const std::string& text);                        // ScreenMessages.PostScreenMessage(text, 5s)
    static void Effect(const std::string& name, Part* p);
    bool eventsAdded_ = false;             // EditorLogicBase.cacheAutoLOC_6006095
};

// ================================================================ EVAConstructionToolsUI (EVAConstructionToolsUI.cs)

class EVAConstructionToolsUI {
public:
    // The tool toggles of the construction panel (the widgets are the shell's: `isOn`, `interactable` and the active flags)
    struct Toggle {
        bool isOn = false;
        bool interactable = true;
        bool active = true;
    };
    Toggle placeButton, moveButton, rotateButton;  // :9 / :12 / :15
    bool coordButtonActive = true;                  // :18 coordButton.gameObject.activeSelf
    ConstructionMode constructionMode = ConstructionMode::Place;  // :20
    // InputLockManager.IsLocked(ControlTypes.EDITOR_GIZMO_TOOLS), DeltaVApp.AnyTextFieldHasFocus(); unset: false
    static inline std::function<bool()> gizmoToolsLocked, anyTextFieldHasFocus;

    void Update(bool placeKeyDown, bool offsetKeyDown, bool rotateKeyDown);  // :27
    void onPlaceButtonInput(bool b);                          // :69
    void onMoveButtonInput(bool b);                           // :77
    void onRotateButtonInput(bool b);                         // :85
    void SetMode(ConstructionMode mode, bool updateUI = true);  // :93
    void OnChangeModeBlocked();                               // :139
    void ShowModeTools();                                     // :155 (internal)
    void HideModeTools();                                     // :163 (internal)
};

// ================================================================ PartItemTransfer (PartItemTransfer.cs:6)

class PartItemTransfer {
public:
    enum class DismissAction : int { Cancelled, Interrupted, ItemMoved };  // :8
    static inline PartItemTransfer* Instance = nullptr;                      // :15

    Part* srcPart = nullptr;                // :17
    std::vector<Part*> semiValidParts;      // :19
    std::vector<Part*> validParts;          // :21
    std::string type = "Item";              // :23
    std::string semiValidMessage = "#autoLOC_135736";  // :25
    std::function<void(DismissAction, Part*)> onDismiss;  // :27
    // PartSelector.Create per part: (part, kind) in creation order; kind 0 = semi-valid, 1 = valid, 2 = source
    std::vector<std::pair<Part*, int>> partSelectors;      // :35
    std::vector<std::string> screenMessages;               // ScreenMessages.PostScreenMessage texts
    bool controlLockSet = false;                           // InputLockManager "itemTransfer"
    bool destroyed = false;

    virtual ~PartItemTransfer();
    virtual void Setup(Part& src, const std::string& itemType, const std::string& itemName, const std::string& semiValidMsg,
                       std::function<void(DismissAction, Part*)> onDialogDismiss);  // :38
    virtual void DismissInterrupted();                  // :86
    static void DismissActive();                        // :92
    virtual void DismissAlreadyRunning();               // :100
    virtual void Dismiss(DismissAction dma, Part* p);   // :106
    virtual void OnDestroy();                           // :116
    virtual void onVesselModified(Vessel* v);           // :168
    virtual void onVesselSituationChange(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs);  // :177
    virtual void onVesselChanged(Vessel*);              // :185
    virtual void OnSrcPartSelect(Part*);                // :190
    virtual void OnSemiValidPartSelect(Part*);          // :195
    virtual void OnPartSelect(Part* p);                 // :200
    void LateUpdate(bool escapeUp);                     // :205

protected:
    virtual bool IsValidPart(Part& p) = 0;              // :134
    virtual bool IsSemiValidPart(Part& p) = 0;          // :136
    virtual void AfterPartsFound() {}                   // :138
    virtual void HookAdditionalEvents() {}              // :143
    virtual void UnhookAdditionalEvents() {}            // :148
    bool eventsAdded_ = false;
};

// ================================================================ inventory UI models

// UIPartActionControllerInventory.cs:9 -- the cargo-drag state shared by the slots (statics) and the cursor-held part
class UIPartActionControllerInventory {
public:
    static inline UIPartActionControllerInventory* Instance = nullptr;  // :11
    static inline bool heldPartIsStack = false;                          // :13
    static inline int stackSize = 0;                                     // :15
    static inline int amountToStore = 0;                                 // :17

    ModuleInventoryPart* currentInventory = nullptr;                      // :25
    UIPartActionInventorySlot* currentInventorySlotClicked = nullptr;     // :27
    UIPartActionInventorySlot* currentInventorySlotHovered = nullptr;     // :29
    UIPartActionInventorySlot* currentInventorySlotEmptied = nullptr;     // :31
    bool currentPartIsAttachable = false;                                 // :40
    Part* currentCargoPart_ = nullptr;                                    // :44
    bool IsCursorOverAnyPAWOrCargoPane = false;                           // :60
    bool isHeldPartOutOfActionWindows = false;                            // :73
    bool isSetAsPart = false;                                             // :215
    bool editorPartPickedBlockSfx = false, editorPartDroppedBlockSfx = false;
    std::vector<std::string> availableCargoParts;                         // :94
    std::vector<std::string> availableInventoryParts;                     // :97
    bool hasCurrentInventoryOnlyIcon = false;                             // CurrentInventoryOnlyIcon != null
    int currentInventoryOnlyIconStack = 0;
    std::vector<std::string> sfx;                                         // PlayPartSelectedSFX / PlayPartDroppedSFX
    std::unique_ptr<Part> heldPartOwner;                                  // the Part CreatePartFromInventory made
    bool currentPawAutoOpening = false;
    bool currentPawAutoOpened = false;
    std::set<std::string> controlLocks;                                   // InputLockManager locks set here ("CargoPartHeld")
    bool iconBackgroundVisible = false;                                   // CurrentInventoryOnlyIcon.ToggleBackgroundVisibility
    bool iconActive = false;                                              // CurrentInventoryOnlyIcon.gameObject.activeInHierarchy
    bool heldPartActive = true;                                           // CurrentCargoPart.gameObject.activeInHierarchy
    float currentDistanceFromPart = 0.0f;                                 // :83
    bool inventoryAndCargoPartExist() const;                              // UIPartActionController.InventoryAndCargoPartExist
    void DestroyHeldObject(Part* p);

    UIPartActionControllerInventory();
    ~UIPartActionControllerInventory();
    void Start();                                                         // :250
    void OnDestroy();                                                     // :274
    Part* CurrentCargoPart() const { return currentCargoPart_; }          // :139
    void SetCurrentCargoPart(Part* p);                                    // :139 setter (fires OnInventoryPartOnMouseChanged)
    ModuleCargoPart* CurrentModuleCargoPart() const;                      // :152
    ModuleInventoryPart* CurrentInventorySlotHoveredModule() const;       // :105
    bool IsKerbalWithinRange(ModuleInventoryPart& inventoryPart);         // :283
    void OnCargoPartConstructionEvent(ConstructionEventType construction, Part* p);  // :293
    void ResetInventoryCacheValues() { ResetInventoryCacheValues(true); } // :323
    void ResetInventoryCacheValues(bool destroyCurrentCargoPart);         // :328
    void ReturnHeldPart();                                                // :437
    void OnVesselDestroy(Vessel* v);                                      // :456
    void OnPartPurchased(AvailablePart* part);                            // :608
    void DestroyHeldPartAsIcon();                                         // :614
    void PlayPartSelectedSFX() { sfx.push_back("pickup"); }               // :545
    void PlayPartDroppedSFX() { sfx.push_back("dropped"); }               // :550
    void SetIconAsPart();                                                 // :660
    void UpdateSelectedPartVisibility();                                  // :732 (state only)
    // :689 / :701: Instantiate + ConfigureInventoryPart
    std::unique_ptr<Part> CreatePartFromInventory(ProtoPartSnapshot& protoPartSnapshot);  // :698
    void ConfigureInventoryPart(Part& invPart);                           // :719
    // ---- the held part's presentation (UI state of UIPartActionControllerInventory)
    float partScale = 1.0f, partScaleMultiplier = 1.0f;                   // :64 / :62 (serialized UI values)
    float cargoPartDepthOffset = 0.0f;                                    // :60
    float partIconDepthDistance = 39.0f, iconHeight = 58.0f;              // :124 / :126
    Vector3f heldPartScale = Vector3f::one();                             // CurrentCargoPart.transform.localScale
    Vector3f heldPartPosition;                                            // CurrentCargoPart.transform.localPosition
    Quaternionf heldPartRotation = Quaternionf::identity();
    bool currentCargoExists = false;                                      // :90
    void UpdateCursorOverPAWs(bool anyPawWindowHovered);                  // :384
    void UpdateCurrentCargoPart(const Vector2f& mouseLocalPoint);         // :398
    void MoveSelectedPart(const Vector2f& mouseLocalPoint);               // :415
    Vector3f dragOverPlane(const Vector2f& mouseLocalPoint) const;        // :423
    void UpdatePartIconScale();                                           // :640
    void SetPartAsIcon(UIPartActionInventory& icon, Part& part);          // :623
    void CreateFlightTooltip(const AvailablePart& ap);                    // :468
    void DestroyTooltip();                                                // :485
    std::string tooltipTitle, tooltipText;                                // the flight tooltip texts ("" = no tooltip)
    bool tooltipActive = false;
    std::function<void()> releasePartToIcon;                              // EditorLogic.fetch.ReleasePartToIcon()
    void ToggleCargoPartsLight(int counter);                              // :537
    int cargoInventoriesOpen = 0;
    bool enableCargoLight = false;
    uint32_t currentHoveredInventoryPart = 0;
    bool destroyed_ = false, eventsAdded_ = false;
    void OnInventoryPartHover(uint32_t partId, Part* p);                  // :344 (reference to the PAW spawn is a hook)
    std::function<bool(uint32_t partId)> pawWindowOpenFor;               // pawController.windows[i].part.persistentId == partId
    std::function<void(Part*)> spawnPartActionWindow;                    // UIPartActionController.Instance.SpawnPartActionWindow
    bool referenceNextCreatedInventoryPAW = false;
};

// UIPartActionInventorySlot.cs:10 -- one inventory slot: the click / hover rules
class UIPartActionInventorySlot {
public:
    int slotIndex = 0;                                   // :19
    UIPartActionInventory* inventoryPartActionUI = nullptr;  // :21
    ModuleInventoryPart* moduleInventoryPart = nullptr;  // :33
    bool selected = false;                                // slotImage.sprite == slot_partItem_current

    bool HaveStackingSpace() const { return moduleInventoryPart->HasStackingSpace(slotIndex); }        // :41
    int CurrentStackedAmount() const { return moduleInventoryPart->GetStackAmountAtSlot(slotIndex); }  // :43
    int MaxStackedAmount() const { return moduleInventoryPart->GetStackCapacityAtSlot(slotIndex); }    // :45
    bool IsStackable() const { return moduleInventoryPart->IsStackable(slotIndex); }                   // :47
    bool CanStackInSlot(const AvailablePart& part, const std::string& partVariantName) const {         // :49
        return moduleInventoryPart->CanStackInSlot(part, partVariantName, slotIndex);
    }
    void Setup(UIPartActionInventory* inventoryRef, int index);       // :54
    void SlotClicked();                                                // :65
    void ProcessClickWithHeldPart();                                   // :92
    void ProcessClickNoHeldPart();                                     // :217
    void EmptyStack();                                                 // :263
    void ClearEmptiedSlot(int prevStackAmount = 1);                    // :273
    void StorePartInEmptySlot(Part& partToStore, const std::string& variantName);  // :284
    bool StackPartsInThisSlot(Part& heldPart, const std::string& partVariantName); // :316
    void StackPartsInSlot(int slot, const std::string& variantName = "", bool destroyHeldPart = true);  // :353
    void SetStackedPartsInSlot(int slot, int howMany, const std::string& variantName = "");  // :367
    void RemovePartFromStack(int slot, const std::string& variantName = "");  // :372
    std::unique_ptr<Part> CreatePartFromThisSlot(int slotIndex);       // :383
    bool CreatePartIconFromThisSlot(int slotIndex);                    // :441 (state of the inventory-only icon; true = an icon exists)
    bool pawControllerPresent = true;                                  // UIPartActionController.Instance != null
    void ReturnHeldPartToThisSlot();                                   // :488
    void UpdateCurrentSelectedSlot(bool isCurrent);                    // :494
    void OnPointerEnter();                                             // :515
    void OnPointerExit();                                              // :525
    void OnEditorPartDeleted(Part* part);                              // :534 (the C# handler is registered per slot)
    void OnCargoModeToggled(bool opened);                              // :544
    void OnDestroy();                                                  // :552
    std::vector<std::string> log;
    bool eventsAdded_ = false;
    ~UIPartActionInventorySlot();
};

// UIPartActionInventory.cs:12 -- the grid model of one inventory (slot icons are part names; the widgets are the shell's)
class UIPartActionInventory {
public:
    int fieldValue = 0;                                   // :31
    std::vector<std::unique_ptr<UIPartActionInventorySlot>> slotButton;  // :37
    std::vector<std::string> slotPartIcon;                // :39 (the part name each slot icon shows; "" = empty slot)
    bool inCargoPane = false;                             // :55
    int selectedIconChildren = 0;                         // currentSelectedItemPartIcon.childCount (the held part parented under the grid icon)
    bool parentWindowHovered = false;                     // :80
    // UIPartActionInventory.Update: the PAW window that hosts the grid (absent in the construction / cargo panes)
    struct UpdateInput {
        bool windowPresent = false, windowHover = false, windowDisplayDirty = false;
    };
    void Update(const UpdateInput& in);                   // :384
    bool pawControllerPresent = true;                     // UIPartActionController.Instance != null
    ModuleInventoryPart* inventoryPartModule = nullptr;   // :73
    // limits display (UpdateVolumeLimits / UpdateMassLimits): the last texts and fill state
    float volumeSliderMax = 0.0f, volumeSliderValue = 0.0f;
    std::string packedVolumeText, massText;
    bool volumeOverLimit = false, massOverLimit = false;
    float massSliderMax = 0.0f, massSliderValue = 0.0f;
    bool limitsFooterActive = false, packedVolumeLimitsActive = false, massLimitsActive = false;
    std::string inventoryNameText;
    std::vector<bool> placePartIconActive;                // btnPlacePart.gameObject.activeSelf per slot
    struct StackUI { bool stackable = false; int amount = 0, max = 0; };
    std::vector<StackUI> slotStackUI;                     // EditorPartIcon.UpdateStackUI(IsStackable, CurrentStackedAmount, MaxStackedAmount)
    void SpawnLoadedPartAtSlot(int slot);                 // ModuleInventoryPart.cs:349
    bool eventsAdded_ = false;

    UIPartActionInventory();
    ~UIPartActionInventory();
    bool AbleToPlaceParts() const;                        // :85
    void SetupConstruction(ModuleInventoryPart& inventoryPart);  // :148
    void SetupPaw(ModuleInventoryPart& inventoryPart, int slots);  // Setup (:106): the flight / editor PAW grid
    void OnDestroy();                                     // :90
    void OnVesselSituationChange(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> data);  // :190
    void OnVesselChange(Vessel* vsl);                     // :198
    void OnModuleInventoryChanged(ModuleInventoryPart* changed);  // :203
    void OnModuleInventorySlotChanged(ModuleInventoryPart* inventory, int slotChanged);  // :215
    void UpdateSlot(int slotChanged, bool updateStackUI);  // :224
    void InitializeSlots();                               // :255
    void SpawnEmptySlot(int slotIndex);                   // :301
    void SpawnLoadedPart(StoredPart& sp);                 // :310
    void DestroyHeldPart() { DestroyHeldPart(false); }    // :337
    void DestroyHeldPart(bool fromConstructionController);  // :342
    void DestroyItem(int index);                          // :370
    void SetAllSlotsNotSelected();                        // :438
    void StartPartPlacement(int slotIconIndex);           // :446 (EditorPartIcon -> its slot index)
    void ResetPlacePartIcons(bool active);                // :466
    void UpdatePlacePartIcons(bool active);               // :484
    void InitializeMassVolumeDisplay();                   // :517 (internal)
    void UpdateVolumeLimits(float currentPackedVolume, float maxPackedVolume, bool overLimit, bool constructionOnly);  // :540 (internal)
    void UpdateMassLimits(float currentMass, float maxMass, bool overLimit);  // :566 (internal)
    // per slot: the part the icon shows is deployable (EditorPartIcon.IsDeployablePart) -- derived from the stored part's prefab
    bool SlotIsDeployable(int slot) const;
    static inline std::function<void(const std::string&)> uiEffect;
};

// InventoryPanelController.cs:7 -- the editor's cargo panel registry
class InventoryPanelController {
public:
    struct InventoryDisplayItem {  // :9
        ModuleInventoryPart* inventoryModule = nullptr;
        UIPartActionInventory* uiInventory = nullptr;
        bool hasUiObject = false;
    };
    static inline InventoryPanelController* Instance = nullptr;  // :16
    std::vector<ModuleInventoryPart*> moduleInventories;          // :38
    std::vector<InventoryDisplayItem> displayedInventories;       // :40
    std::vector<std::unique_ptr<UIPartActionInventory>> uiInventories;
    std::vector<std::shared_ptr<ProtoCrewMember>> crew;           // :46
    std::vector<int> siblingOrder;                                 // SetSiblingIndex order (indices into displayedInventories)
    bool IsOpen() const { return isOpen; }                         // :55
    bool Hover() const { return hover; }                           // :57
    InventoryPanelController();
    ~InventoryPanelController();
    void Start();                                                  // :80
    void OnDestroy();                                              // :88
    void Update();                                                 // :95
    void Reset(EditorScreen screen) { OnEditorScreenChange(screen); }  // :103
    void OnEditorScreenChange(EditorScreen screen);                // :108 (EditorScreen.Cargo opens the panel)
    void OnEditorCrewModified(VesselCrewManifest* manifest);       // :146
    void OnEditorShipModified(ShipConstruct* ship);                // :158
    void UpdateInventoryLists(const std::vector<Part*>& parts, std::vector<std::shared_ptr<ProtoCrewMember>> crewList);  // :170
    void UpdateDisplayedInventories();                             // :229
    bool TryGetDisplayedInventory(ModuleInventoryPart* inventoryModule, InventoryDisplayItem*& displayItem);  // :273
    void DestroyHeldIcons(UIPartActionInventory* callingInventory);  // :293
    void OnPointerEnter() { hover = true; }
    void OnPointerExit() { hover = false; }
    // ShipConstruction.ShipManifest.GetAllCrew(includeNulls: false); unset: no manifest
    static inline std::function<std::vector<std::shared_ptr<ProtoCrewMember>>()> shipManifestCrew;
    static inline bool shipManifestPresent = false;
    bool prevIsOpen = false;

private:
    bool isOpen = false;
    bool hover = false;
    bool eventsAdded_ = false;
};

}  // namespace ksp
