// WP-40b: the KFSM placement loop of EditorLogic (EditorLogic.cs:1236-2366 SetupFSM and the state / event bodies), the Update / LateUpdate /
// FixedUpdate drivers, the input helpers the states call, pickPart / dragOverPlane, StartEditor (new / load ship) and the part of EditorToolsUI that
// reacts to part events and key presses. See include/ksp/editor/editor_logic.hpp and STATUS.md "WP-40b".
//
// Scene services (camera, raycast, UI hover, audio, tweak hooks) are EditorEnvironment seams; keys and the mouse come from the WP-50 input device,
// Mouse, GameSettings.Editor_* and InputLockManager. Unity `x != null` on a UnityEngine.Object is false for a destroyed object: IsDestroyed().
#include "ksp/editor/editor_logic.hpp"

#include <algorithm>
#include <filesystem>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/editor/editor_geometry.hpp"
#include "ksp/editor/editor_reroot.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/inventory/inventory.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_device.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/input/mouse.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/core/log.hpp"

namespace ksp {

using QuaternionExtensions::Inverse;
using events::GameEvents;
using KSP::UI::Screens::CraftBrowserDialog_LoadType;

namespace {

const IInputDevice& In() { return InputEnvironment::Input(); }
EditorEnvironment& Env() { return EditorEnvironment::Current(); }

bool PointerOverUI() {
    const EditorEnvironment& e = Env();
    return e.pointerOverUI && e.pointerOverUI();
}

void PlayClip(EditorClip c) {
    const EditorEnvironment& e = Env();
    if (e.playOneShot) e.playOneShot(c);
}

bool ModifierKeyHeld() { return GameSettings::MODIFIER_KEY && GameSettings::MODIFIER_KEY->GetKey(); }

bool KeyDownOf(const std::shared_ptr<KeyBinding>& b) { return b && b->GetKeyDown(); }
bool KeyUpOf(const std::shared_ptr<KeyBinding>& b) { return b && b->GetKeyUp(); }

constexpr int kMaskWithLayers = 0x200000;

}  // namespace

// ==================================================================================================================================
// lifetime
// ==================================================================================================================================

// EditorLogic.cs:466 (the GameEvents listeners that drive the loop; the UI listeners belong to the shell)
void EditorLogic::Awake() {
    if (awakened_) return;
    awakened_ = true;
    EditorEnvironment& env = Env();
    env.fineTweakKey = [] { return GameSettings::Editor_fineTweak && GameSettings::Editor_fineTweak->GetKey(); };
    env.modifierKeyHeld = [] { return ModifierKeyHeld(); };
    env.useAngleSnapSetting = [] { return GameSettings::VAB_USE_ANGLE_SNAP; };
    GameEvents::onEditorConstructionModeChange.Add(this, &EditorLogic::onConstructionModeChanged);
    GameEvents::onEditorSymmetryMethodChange.Add(this, &EditorLogic::onSymMethodChange);
    // EditorToolsUI.Start registers this one; it is wired here so a host can add its own observers after Awake (listeners fire last to first)
    GameEvents::onEditorPartEvent.Add(this, &EditorLogic::toolsOnEditorPartEvent);
}

// EditorLogic.cs:519
void EditorLogic::OnDestroy() {
    if (!awakened_) return;
    awakened_ = false;
    GameEvents::onEditorConstructionModeChange.Remove(this, &EditorLogic::onConstructionModeChanged);
    GameEvents::onEditorSymmetryMethodChange.Remove(this, &EditorLogic::onSymMethodChange);
    GameEvents::onEditorPartEvent.Remove(this, &EditorLogic::toolsOnEditorPartEvent);
}

// EditorLogic.cs:545-592 (Start, the state part)
void EditorLogic::Start() {
    Awake();
    onSymMethodChange(symmetryMethod);
    SetupFSM();
    StartEditor(false);
    if (!IsDestroyed(rootPart)) {
        // rootPart.gameObject.SetLayerRecursive(0, ...): layers are the shell's
        fsm.StartFSM(&st_idle);
    } else {
        fsm.StartFSM(&st_podSelect);
    }
}

// EditorLogic.cs:2375
void EditorLogic::Update() {
    if (fsm.Started()) {
        fsm.UpdateFSM();
        if (GameSettings::PAUSE && GameSettings::PAUSE->GetKeyDown(true) && selectedCompoundPart != nullptr &&
            selectedCompoundPart->attachState() == CompoundPart::AttachState::Attaching) {
            selectedPart = selectedCompoundPart;
            fsm.RunEvent(&on_partPicked);
            // CreateSelectedPartIcon(): the shell
            InputLockManager::RemoveControlLock("CompoundPart_Placement");
        }
    }
    // EditorToolsUI.Update (a separate behaviour; the order of the two Update calls is not defined by the C#: EditorLogic first here)
    toolsUpdate();
}

// EditorLogic.cs:2407
void EditorLogic::LateUpdate() {
    if (fsm.Started()) fsm.LateUpdateFSM();
}

// EditorLogic.cs:2367
void EditorLogic::FixedUpdate() {
    if (fsm.Started()) fsm.FixedUpdateFSM();
}

void EditorLogic::RunCoroutines() {
    std::vector<PendingCallback> pending;
    pending.swap(coroutines_);
    for (PendingCallback& c : pending) {
        if (c.framesLeft > 0) c.framesLeft--;
        if (c.framesLeft <= 0) c.fn();
        else coroutines_.push_back(std::move(c));
    }
}

std::vector<Part*> EditorLogic::SceneParts() const {
    std::vector<Part*> out;
    for (const auto& p : editorParts_) out.push_back(p.get());
    auto addShip = [&out](const ShipConstruct* s) {
        if (s == nullptr) return;
        for (const auto& p : s->ownedParts) out.push_back(p.get());
    };
    for (const auto& s : oldShips_) addShip(s.get());
    addShip(shipOwned_.get());
    std::sort(out.begin(), out.end(), [](const Part* a, const Part* b) { return a->instanceID < b->instanceID; });
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

void EditorLogic::freeze(Part* p) {
    // Part.cs:2407: stackIcon.Freeze (the stage icon: WP-16 / UI), frozen = true, SetHighlight (the shell), then the children
    frozenParts_.insert(p);
    for (size_t i = 0, count = p->children.size(); i < count; i++) freeze(p->children[i]);
}

void EditorLogic::unfreeze(Part* p) {
    // Part.cs:2422
    frozenParts_.erase(p);
    for (size_t i = 0, count = p->children.size(); i < count; i++) unfreeze(p->children[i]);
}

// ==================================================================================================================================
// StartEditor / NewShip / load
// ==================================================================================================================================

// EditorLogic.cs:606
void EditorLogic::StartEditor(bool isRestart) {
    if (isRestart) GameEvents::onEditorRestart.Fire();
    if (!restartingEditor) {
        InputLockManager::RemoveControlLock("Editor_outOfPartMode");
        InputLockManager::RemoveControlLock("EditorLogic_rootPartMode");
    }
    onSymMethodChange(symmetryMethod);
    editorScreen = EditorScreen::Parts;   // SelectPanelParts(isReset: true)
    undoLevel = 0;
    EditorEnvironment& env = Env();
    env.undoRedoLimit = GameSettings::EDITOR_UNDO_REDO_LIMIT;   // undoLimit = GameSettings.EDITOR_UNDO_REDO_LIMIT
    ShipConstruction::ClearBackups();
    AdoptShip(std::make_unique<ShipConstruct>(env.editorFacility));
    // UpdateUI(): the shell
    bool loaded = false;
    switch (startupBehaviour) {
    case StartupBehaviour::LOAD_FROM_CACHE:
        if (ShipConstruction::ShipConfig != nullptr) {
            std::unique_ptr<ShipConstruct> ld = ShipConstruction::LoadShip();
            if (ld != nullptr) AdoptShip(std::move(ld));
            if (ship != nullptr && !ship->liveParts.empty()) {
                rootPart = ship->liveParts[0]->localRoot();
                shipNameField = Localizer::Format(ship->shipName);
                shipDescriptionField = Localizer::Format(ship->shipDescription);
                FlagURL = ship->missionFlag;
                editorScreen = EditorScreen::Parts;
                SetBackup();
                vesselNameAtLastSave = ship->shipName;
                // the cached crew manifest checks (ResetCrewAssignment / CrewAssignmentDialog): the shell's
                loaded = true;
                break;
            }
            AdoptShip(std::make_unique<ShipConstruct>(env.editorFacility));
        }
        break;
    case StartupBehaviour::LOAD_FROM_FILE:
        FlagURL = "";
        if (!std::filesystem::exists(filePathToLoad)) {
            log::Error("[EditorDriver]: No file found at " + filePathToLoad);
        } else {
            std::unique_ptr<ShipConstruct> ld = ShipConstruction::LoadShip(filePathToLoad);
            if (ld != nullptr) {
                AdoptShip(std::move(ld));
                rootPart = ship->liveParts[0]->localRoot();
                shipNameField = Localizer::Format(ship->shipName);
                shipDescriptionField = Localizer::Format(ship->shipDescription);
                FlagURL = ship->missionFlag;
                editorScreen = EditorScreen::Parts;
                SetBackup();
                vesselNameAtLastSave = ship->shipName;
                loaded = true;
                break;
            }
            AdoptShip(std::make_unique<ShipConstruct>(env.editorFacility));
        }
        break;
    default: break;
    }
    if (!loaded) {
        ship->shipName = Localizer::Format("#autoLOC_900530");
        FlagURL = "";
        rootPart = nullptr;
        shipNameField = Localizer::Format(ship->shipName);
        shipDescriptionField = Localizer::Format(ship->shipDescription);
        vesselNameAtLastSave = "";
    }
    ship->shipFacility = env.editorFacility;
    // EditorPanels.ShowPartsList, Input.ResetInputAxes, FlagSetup (the flag browser needs HighLogic.CurrentGame: the shell)
    undoIndexAtLastSave = undoLevel;
    if (isRestart) {
        if (!IsDestroyed(rootPart)) fsm.RunEvent(&on_shipLoaded);
        else fsm.RunEvent(&on_newShip);
    }
    GameEvents::onEditorStarted.Fire();
}

// EditorLogic.cs:4023
void EditorLogic::NewShip() {
    toolsSetMode(ConstructionMode::Place);
    if (undoIndexAtLastSave != undoLevel && ship->liveParts.size() > 0) {
        // the unsaved-changes dialog: Cancel (OnNewShipDialogDismiss) or Discard (OnNewShipConfirm); the Save button saves first (the shell)
        const EditorEnvironment& e = Env();
        const bool discard = !e.newShipDialog || e.newShipDialog();
        if (discard) OnNewShipConfirm();
        else {
            InputLockManager::RemoveControlLock("SaveConfirmationDialog");
            GameEvents::onEditorNewShipDialogDismiss.Fire();
        }
    } else {
        OnNewShipConfirm();
    }
}

// EditorLogic.cs:4046
void EditorLogic::OnNewShipConfirm() {
    InputLockManager::RemoveControlLock("SaveConfirmationDialog");   // OnNewShipDialogDismiss
    GameEvents::onEditorNewShipDialogDismiss.Fire();
    startupBehaviour = StartupBehaviour::START_CLEAN;
    ShipConstruction::ShipConfig = nullptr;
    StartEditor(true);
}

// EditorLogic.cs:4760
void EditorLogic::LoadShipFromFile(const std::string& path) {
    if (fetch != nullptr) fetch->ShipToLoadSelected(path, CraftBrowserDialog_LoadType::Normal);
}

// EditorLogic.cs:4768
void EditorLogic::ShipToLoadSelected(const std::string& path, CraftBrowserDialog_LoadType loadType) {
    InputLockManager::RemoveControlLock("EditorLogic_dialog_softLock");
    if (path.empty()) return;
    switch (loadType) {
    case CraftBrowserDialog_LoadType::Normal:
        startupBehaviour = StartupBehaviour::LOAD_FROM_FILE;
        filePathToLoad = path;
        StartEditor(true);
        GameEvents::onEditorLoad.Fire(ship, loadType);
        break;
    case CraftBrowserDialog_LoadType::Merge: {
        auto construct = std::make_unique<ShipConstruct>();
        ConfigNode::NodePtr root = ConfigNode::Load(path);
        construct->LoadShipWithParts(*root, 0);
        ShipConstruct* raw = construct.get();
        mergedConstructs_.push_back(std::move(construct));
        SpawnConstruct(raw);
        GameEvents::onEditorLoad.Fire(raw, loadType);
        break;
    }
    }
}

// EditorLogic.cs:4804
void EditorLogic::ShipToLoadSelected(const ConfigNode::NodePtr& node, CraftBrowserDialog_LoadType loadType) {
    InputLockManager::RemoveControlLock("EditorLogic_dialog_softLock");
    if (node == nullptr) return;
    switch (loadType) {
    case CraftBrowserDialog_LoadType::Normal:
        startupBehaviour = StartupBehaviour::LOAD_FROM_CACHE;
        ShipConstruction::ShipConfig = node;
        StartEditor(true);
        GameEvents::onEditorLoad.Fire(ship, loadType);
        break;
    case CraftBrowserDialog_LoadType::Merge: {
        auto construct = std::make_unique<ShipConstruct>();
        construct->LoadShipWithParts(*node, 0);
        ShipConstruct* raw = construct.get();
        mergedConstructs_.push_back(std::move(construct));
        SpawnConstruct(raw);
        GameEvents::onEditorLoad.Fire(raw, loadType);
        break;
    }
    }
    shipNameField = Localizer::Format(ship->shipName);
}

// EditorLogic.cs:2834
void EditorLogic::OnPartListIconTap(const AvailablePart& p) {
    if (fsm.CurrentState() == &st_place) OnPartListBackgroundTap();
    else SpawnPart(p);
}

// EditorLogic.cs:2858
void EditorLogic::OnPartListBackgroundTap() {
    if (fsm.CurrentState() == &st_place) DestroySelectedPart();
}

// EditorLogic.cs:2977
void EditorLogic::ReleasePartToIcon() {
    if (fsm.Started()) fsm.RunEvent(&on_partOverInventoryPAW);
}

// ==================================================================================================================================
// EditorToolsUI (the construction-mode panel)
// ==================================================================================================================================

// EditorToolsUI.cs:126
void EditorLogic::toolsSetMode(ConstructionMode mode, bool updateUI) {
    (void)updateUI;   // the toggle buttons are the shell's
    switch (mode) {
    case ConstructionMode::Root:
        if (InputLockManager::IsLocked(ControlTypes::EDITOR_ROOT_REFLOW)) return;
        break;
    default:
        if (InputLockManager::IsLocked(ControlTypes::EDITOR_GIZMO_TOOLS)) return;
        break;
    case ConstructionMode::Place: break;
    }
    if ((mode == ConstructionMode::Move || mode == ConstructionMode::Rotate) && !IsDestroyed(selectedPart) && !ship->Contains(selectedPart)) {
        PlayClip(EditorClip::CannotPlace);
        return;
    }
    if (toolsConstructionMode != mode) {
        toolsConstructionMode = mode;
        GameEvents::onEditorConstructionModeChange.Fire(toolsConstructionMode);
    }
}

// EditorToolsUI.cs:103
void EditorLogic::toolsUpdate() {
    // !NameOrDescriptionFocused() && !DeltaVApp.AnyTextFieldHasFocus() && !RoboticControllerManager.AnyWindowTextFieldHasFocus(): text fields are the shell's
    if (KeyDownOf(GameSettings::Editor_modePlace)) toolsSetMode(ConstructionMode::Place);
    if (KeyDownOf(GameSettings::Editor_modeOffset)) toolsSetMode(ConstructionMode::Move);
    if (KeyDownOf(GameSettings::Editor_modeRotate)) toolsSetMode(ConstructionMode::Rotate);
    if (KeyDownOf(GameSettings::Editor_modeRoot)) toolsSetMode(ConstructionMode::Root);
}

// EditorToolsUI.cs:217
void EditorLogic::toolsOnEditorPartEvent(ConstructionEventType evt, Part* part) {
    (void)part;
    if (editorScreen == EditorScreen::Cargo) return;
    switch (evt) {
    case ConstructionEventType::PartCreated: toolsSetMode(ConstructionMode::Place); break;
    case ConstructionEventType::PartPicked: toolsSetMode(ConstructionMode::Place); break;
    case ConstructionEventType::PartDeleted:
        if (IsDestroyed(rootPart)) {
            // the tools panel is hidden (gameObject.SetActive(false)): UI
        } else {
            toolsSetMode(ConstructionMode::Place);
        }
        break;
    case ConstructionEventType::PartCopied: toolsSetMode(ConstructionMode::Place); break;
    case ConstructionEventType::PartRootSelected: toolsSetMode(ConstructionMode::Place); break;
    default: break;
    }
}

// EditorLogic.cs:2420
void EditorLogic::onConstructionModeChanged(ConstructionMode mode) {
    if (mode != constructionMode) {
        switch (mode) {
        default: fsm.RunEvent(&on_goToModePlace); break;
        case ConstructionMode::Move: fsm.RunEvent(&on_goToModeOffset); break;
        case ConstructionMode::Rotate: fsm.RunEvent(&on_goToModeRotate); break;
        case ConstructionMode::Root: fsm.RunEvent(&on_goToModeRoot); break;
        }
        constructionMode = mode;
    }
}

// ==================================================================================================================================
// input helpers of the states
// ==================================================================================================================================

// EditorLogic.cs:1125
void EditorLogic::snapButton() {
    GameSettings::VAB_USE_ANGLE_SNAP = !GameSettings::VAB_USE_ANGLE_SNAP;
    Env().useAngleSnap = GameSettings::VAB_USE_ANGLE_SNAP;
    GameEvents::onEditorSnapModeChange.Fire(GameSettings::VAB_USE_ANGLE_SNAP);
    // GizmoOffset / GizmoRotate listen to onEditorSnapModeChange (Attach adds component.onAngleSnapChanged)
    if (gizmoOffsetObj_) gizmoOffsetObj_->onEditorSnapChanged(GameSettings::VAB_USE_ANGLE_SNAP);
    if (gizmoRotateObj_) gizmoRotateObj_->onAngleSnapChanged(GameSettings::VAB_USE_ANGLE_SNAP);
}

// EditorLogic.cs:1133
void EditorLogic::symInputUpdate() {
    // AnyTextFieldHasFocus(), DeltaVApp / RoboticControllerManager text fields: the shell's
    if (!InputLockManager::IsUnlocked(ControlTypes::EDITOR_SYM_SNAP_UI)) return;
    if (KeyDownOf(GameSettings::Editor_toggleSymMode)) {
        RestoreSymmetryState();
        if (!In().GetKey(KeyCode::LeftShift) && !In().GetKey(KeyCode::RightShift)) nextSymMode();
        else prevSymMode();
    }
    if (KeyUpOf(GameSettings::Editor_toggleSymMethod)) {
        RestoreSymmetryState();
        switch (symmetryMethod) {
        case SymmetryMethod::Mirror: symmetryMethod = SymmetryMethod::Radial; break;
        case SymmetryMethod::Radial: symmetryMethod = SymmetryMethod::Mirror; break;
        }
        GameEvents::onEditorSymmetryMethodChange.Fire(symmetryMethod);
    }
    if (!KeyDownOf(GameSettings::Editor_coordSystem)) return;
    RestoreSymmetryState();
    if (symmetryMethod == SymmetryMethod::Radial) {
        switch (symmetryCoordSpace) {
        case UnityEngine::Space::Self: symmetryCoordSpace = UnityEngine::Space::World; break;
        case UnityEngine::Space::World: symmetryCoordSpace = UnityEngine::Space::Self; break;
        }
        GameEvents::onEditorSymmetryCoordsChange.Fire(symmetryCoordSpace);
    }
}

// EditorLogic.cs:1189
void EditorLogic::snapInputUpdate() {
    if (InputLockManager::IsUnlocked(ControlTypes::EDITOR_SYM_SNAP_UI) && KeyDownOf(GameSettings::Editor_toggleAngleSnap)) snapButton();
}

// EditorLogic.cs:2708 (the player build on Windows / Linux: Application.isEditor is false, the platform is not OSXPlayer)
void EditorLogic::UndoRedoInputUpdate() {
    if (!InputLockManager::IsUnlocked(ControlTypes::EDITOR_UNDO_REDO)) return;
    if (In().GetKey(KeyCode::LeftControl)) {
        if (In().GetKeyDown(KeyCode::Z)) RestoreState(-1);
    }
    if (!In().GetKey(KeyCode::LeftControl)) return;
    if (In().GetKeyDown(KeyCode::Y)) RestoreState(1);
}

// EditorLogic.cs:3131
void EditorLogic::deleteInputUpdate() {
    const bool flag = constructionMode == ConstructionMode::Place ||
                      (selectedPart != rootPart && (constructionMode == ConstructionMode::Move || constructionMode == ConstructionMode::Rotate));
    if (InputLockManager::IsUnlocked(ControlTypes::EDITOR_PAD_PICK_PLACE) && !IsDestroyed(selectedPart) && flag && In().GetKeyDown(KeyCode::Delete))
        DestroySelectedPart();
}

// EditorLogicBase.cs:75
void EditorLogicBase::partRotationInputUpdate() {
    partTweaked = false;
    if (!checkInputLocks || InputLockManager::IsUnlocked(ControlTypes::EDITOR_GIZMO_TOOLS) || editorScreen == EditorScreen::Cargo) {
        PartFields& f = selectedPart->fields;
        const bool fine = GameSettings::Editor_fineTweak && GameSettings::Editor_fineTweak->GetKey();
        auto axisOf = [&](const Vector3f& dir) { return Inverse(f.initRotation) * dir; };
        if (fsm.currentStateName != "st_rotate_tweak") {
            if (KeyDownOf(GameSettings::Editor_yawLeft)) {
                f.attRotation = UnityNative::AngleAxis(fine ? 5.0f : 90.0f, axisOf(Vector3f::forward())) * f.attRotation;
                partTweaked = true;
            } else if (KeyDownOf(GameSettings::Editor_yawRight)) {
                f.attRotation = UnityNative::AngleAxis(fine ? -5.0f : -90.0f, axisOf(Vector3f::forward())) * f.attRotation;
                partTweaked = true;
            }
            if (KeyDownOf(GameSettings::Editor_rollLeft)) {
                f.attRotation = UnityNative::AngleAxis(fine ? 5.0f : 90.0f, axisOf(Vector3f::up())) * f.attRotation;
                partTweaked = true;
            } else if (KeyDownOf(GameSettings::Editor_rollRight)) {
                f.attRotation = UnityNative::AngleAxis(fine ? -5.0f : -90.0f, axisOf(Vector3f::up())) * f.attRotation;
                partTweaked = true;
            } else if (KeyDownOf(GameSettings::Editor_pitchUp)) {
                f.attRotation = UnityNative::AngleAxis(fine ? -5.0f : -90.0f, axisOf(Vector3f::right())) * f.attRotation;
                partTweaked = true;
            } else if (KeyDownOf(GameSettings::Editor_pitchDown)) {
                f.attRotation = UnityNative::AngleAxis(fine ? 5.0f : 90.0f, axisOf(Vector3f::right())) * f.attRotation;
                partTweaked = true;
            }
        }
        partRotationResetUpdate();
    }
    if (partTweaked) {
        GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartRotated, selectedPart);
        partTweaked = false;
    }
}

// ==================================================================================================================================
// pickPart / dragOverPlane
// ==================================================================================================================================

// EditorLogic.cs:2621
Part* EditorLogic::pickPart(int layerMask_, bool pickRoot, bool pickRootIfFrozen) {
    EditorEnvironment& env = Env();
    if (PointerOverUI()) return nullptr;
    if (!env.screenPointToRay || !env.raycast) return nullptr;
    const EditorRay ray = env.screenPointToRay(In().mousePosition());
    const std::optional<EditorSurfaceHit> hit = env.raycast(ray, 1000.0f, layerMask_);
    if (!hit) return nullptr;
    Part* part = hit->part;
    if (part == nullptr) return nullptr;
    if (IsFrozen(part) ? pickRootIfFrozen : pickRoot) part = part->localRoot();
    const Vector3f camPos = env.cameraPosition ? env.cameraPosition() : Vector3f::zero();
    Vector3f normalized = (camPos - part->partTransform.position).normalized();
    normalized = Vector3f(normalized.x, 0.0f, normalized.z);
    selPartGrabOffset = hit->point - part->partTransform.position;
    const Quaternionf look = UnityNative::LookRotation(normalized - (normalized - ray.direction), Vector3f::up());
    selPartGrabOffset = Inverse(look) * selPartGrabOffset;
    selPartGrabOffset = Vector3f(selPartGrabOffset.x, selPartGrabOffset.y, 0.0f);
    selPartGrabOffset = UnityNative::LookRotation(normalized - (normalized - ray.direction), Vector3f::up()) * selPartGrabOffset;
    CenterDragPlane(part->partTransform.position + selPartGrabOffset);
    return part;
}

// EditorLogic.cs:2651
void EditorLogic::CenterDragPlane(const Vector3f& pos) { dragPlaneCenter = Vector3f(pos.x, 0.0f, pos.z); }

// EditorLogic.cs:2656
bool EditorLogic::dragOverPlane(Vector3f& position) {
    EditorEnvironment& env = Env();
    position = Vector3f::zero();
    if (!env.screenPointToRay || !env.cameraPosition) return false;
    const EditorRay ray = env.screenPointToRay(In().mousePosition());
    const Vector3f cam = env.cameraPosition();
    const float num = Vector3f::Distance(cam, dragPlaneCenter);
    const Vector3f normalized = (dragPlaneCenter - cam).normalized();
    const float y = f32store(fx(Mathf::Atan2(dragPlaneCenter.x - cam.x, dragPlaneCenter.z - cam.z)) * 57.29578);
    const Quaternionf yRot = UnityNative::Euler(Vector3f(0.0f, y, 0.0f));
    const float halfPi = Mathf::PI / 2.0f;
    const float f = f32store(fx(halfPi) - fx(Mathf::Acos(Vector3f::Dot(normalized, yRot * Vector3f::forward()))));
    const float num2 = f32store(fx(num) * fx(Mathf::Sin(f)));
    const float f2 = Mathf::Acos(Vector3f::Dot(ray.direction, yRot * Vector3f::forward()));
    const float num3 = f32store(fx(num2) * fx(Mathf::Tan(f2)));
    const float num4 = Mathf::Sqrt(f32store(fx(num2) * fx(num2) + fx(num3) * fx(num3)));   // one managed expression: evaluated in double, rounded when passed
    position = cam + ray.direction * num4 - selPartGrabOffset;
    // editorBounds.Contains(position): Bounds.Contains is native (inclusive on every axis; UNVERIFIED against the game)
    const Bounds b = editorBounds();
    const Vector3f mn = b.min(), mx = b.max();
    return !(position.x < mn.x || position.y < mn.y || position.z < mn.z || position.x > mx.x || position.y > mx.y || position.z > mx.z);
}

// EditorLogic.cs:5439
bool EditorLogic::HeldPartIsStacked() {
    const EditorEnvironment& e = Env();
    return e.heldPartIsStacked && e.heldPartIsStacked();
}

// ==================================================================================================================================
// gizmo objects
// ==================================================================================================================================

namespace {
EditorGizmos::GizmoHost HostOf(Part& p) {
    PartTransform& ref = EditorLogic::ReferenceTransform(p);
    EditorGizmos::GizmoHost h;
    h.transform = &ref;
    h.parent = (&ref == &p.partTransform) ? (p.parent != nullptr ? &p.parent->partTransform : nullptr) : &p.partTransform;
    return h;
}
}  // namespace

void EditorLogic::AttachOffsetGizmo(Part& part) {
    gizmoOffsetObj_ = std::make_unique<EditorGizmos::GizmoOffset>(EditorGizmos::GizmoOffset::Attach(
        HostOf(part), part.fields.initRotation, [this](const Vector3f& d) { onOffsetGizmoUpdate(d); },
        [this](const Vector3f& d) { onOffsetGizmoUpdated(d); }));
    GizmoStateFromObject(false);
}

void EditorLogic::AttachRotateGizmo(Part& part) {
    gizmoRotateObj_ = std::make_unique<EditorGizmos::GizmoRotate>(EditorGizmos::GizmoRotate::Attach(
        HostOf(part), gizmoPivot, part.fields.initRotation, [this](const Quaternionf& d) { onRotateGizmoUpdate(d); },
        [this](const Quaternionf& d) { onRotateGizmoUpdated(d); }));
    GizmoStateFromObject(true);
}

void EditorLogic::GizmoStateFromObject(bool rotate) {
    if (rotate) {
        if (!gizmoRotateObj_) return;
        gizmoRotate.coordSpace = gizmoRotateObj_->CoordSpace();
        gizmoRotate.transform = gizmoRotateObj_->transform;
        gizmoRotate.hostRot0 = gizmoRotateObj_->HostRot0();
    } else {
        if (!gizmoOffsetObj_) return;
        gizmoOffset.coordSpace = gizmoOffsetObj_->CoordSpace();
        gizmoOffset.transform = gizmoOffsetObj_->transform;
    }
}

void EditorLogic::GizmoStateToObject(bool rotate) {
    if (rotate) {
        if (gizmoRotateObj_) gizmoRotateObj_->transform = gizmoRotate.transform;
    } else {
        if (gizmoOffsetObj_) gizmoOffsetObj_->transform = gizmoOffset.transform;
    }
}

// ==================================================================================================================================
// SetupFSM
// ==================================================================================================================================

// EditorLogic.cs:1236
void EditorLogic::SetupFSM() {
    using KFSMStateChange = Delegate<void(KFSMState*)>;
    // ---- st_podSelect ----
    st_podSelect.OnEnter = KFSMStateChange::Lambda(this, [this](KFSMState* from) {
        if (from == &st_podSelect) {
            if (!restartingEditor) log::Warning("[EditorLogic Warning]: Trying to set rootPartMode, but mode is already set.");
        } else {
            // EditorPartList filters / Refresh and editorCamera.SendMessage("ResetCamera"): the shell
            editorScreen = EditorScreen::Parts;
            InputLockManager::SetControlLock(
                ~(ControlTypes::UI | ControlTypes::EDITOR_ICON_HOVER | ControlTypes::EDITOR_ICON_PICK | ControlTypes::EDITOR_TAB_SWITCH |
                  ControlTypes::EDITOR_LOAD | ControlTypes::EDITOR_EXIT | ControlTypes::EDITOR_UNDO_REDO | ControlTypes::GUI),
                "EditorLogic_rootPartMode");
        }
    });
    st_podSelect.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::partSearchUpdate);
    st_podSelect.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::UndoRedoInputUpdate);
    st_podSelect.OnLeave = KFSMStateChange::Lambda(this, [this](KFSMState* to) {
        if (to == &st_idle) {
            if (fsm.LastEvent() == &on_podSelect && !IsDestroyed(rootPart) && !ship->Contains(rootPart)) {
                rootPart->partTransform.position = initialPodPosition();
                rootPart->fields.attPos0 = initialPodPosition();
                SetRotation(*rootPart, vesselRotation);
                rootPart->fields.attRotation0 = vesselRotation;
                RestoreSymmetryState();
                rootPart->fields.symMethod = static_cast<int32_t>(symmetryMethod);
                addToShip(rootPart);
                const bool num8 = undoLevel == 0;
                SetBackup();
                // ship.vesselDeltaV = VesselDeltaV.Create(ship): the delta-v tool is the shell's
                GameEvents::onEditorPodPicked.Fire(rootPart);
                (void)num8;   // ResetCrewAssignment (first pod) / the manifest merge (later): the crew dialog is the shell's
                PlayClip(EditorClip::Attach);
            }
            InputLockManager::RemoveControlLock("EditorLogic_rootPartMode");
            // button interactable flags, EditorPanels.ShowPartsList, EditorPartList filters: the shell
        }
    });
    fsm.AddState(&st_podSelect);

    // ---- st_idle ----
    st_idle.OnEnter = KFSMStateChange::Lambda(this, [](KFSMState*) {});
    st_idle.OnUpdate = KFSMCallback::Lambda(this, [] {});
    st_idle.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::symInputUpdate);
    st_idle.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::snapInputUpdate);
    st_idle.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::UndoRedoInputUpdate);
    st_idle.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::partSearchUpdate);
    st_idle.OnLeave = KFSMStateChange::Lambda(this, [](KFSMState*) {});
    fsm.AddState(&st_idle);

    // ---- st_place ----
    st_place.OnEnter = KFSMStateChange::Lambda(this, [this](KFSMState*) {
        // selectedPart.gameObject.SetLayerRecursive(1, ...): layers are the shell's
        deleteSymmetryParts();
        RestoreSymmetryState();
        // UIPartActionControllerInventory.editorPartPickedBlockSfx: the inventory UI is the shell's (null Instance branch)
        PlayClip(EditorClip::PartGrab);
        if (selectedPart == RootPart()) GameEvents::onEditorPodSelected.Fire(RootPart());
        // radialSymmetryBtn / radialSymmetryText: the shell
    });
    st_place.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::symInputUpdate);
    st_place.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::snapInputUpdate);
    st_place.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::UndoRedoInputUpdate);
    st_place.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::partSearchUpdate);
    st_place.OnUpdate += KFSMCallback::Lambda(this, [this] {
        if (Mouse::Right.GetButtonUp() && Mouse::Right.WasDragging()) srfAttachCursorOffset = Vector3f::zero();
        if (!IsDestroyed(selectedPart)) {
            Vector3f position = Vector3f::zero();
            if (dragOverPlane(position))
                selectedPart->partTransform.position = Vector3f::Lerp(selectedPart->partTransform.position, position, dragSharpness * Time::deltaTime);
            SetRotation(*selectedPart, vesselRotation * selectedPart->fields.initRotation * selectedPart->fields.attRotation);
            if (!skipPartAttach) {
                attachment = checkAttach(selectedPart);
                attachmentSet_ = true;
            } else if (attachmentSet_) {
                attachment.possible = false;
            }
            if (HeldPartIsStacked()) {
                if (!attachmentSet_) throw NullReferenceException();
                attachment.possible = false;
            }
            if (!attachmentSet_) throw NullReferenceException();
            selectedPart->fields.attRotation0 = attachment.rotation;
            selectedPart->fields.attPos0 = attachment.position;
            if (attachment.possible) SetRotation(*selectedPart, attachment.rotation * selectedPart->fields.attRotation);
            selectedPart->partTransform.position = attachment.position;
            const AttachRules rules = selectedPart->fields.attachRules.value_or(AttachRules{});
            if (selectedPart->isCargoPart() && !rules.StackOrSurfaceAttachable()) {
                bool flag3 = false;
                if (Mouse::HoveredPart != nullptr) {
                    auto* inv = dynamic_cast<ModuleInventoryPart*>(Mouse::HoveredPart->FindModuleOfClass("ModuleInventoryPart"));
                    if (inv != nullptr && inv->TotalEmptySlots() > 0) {
                        highlightSelected(true, selectedPart);
                        flag3 = true;
                    }
                }
                if (!flag3) highlightSelected(false, selectedPart);
            } else {
                highlightSelected(attachment.possible, selectedPart);
            }
            if (selectedPart->potentialParent != nullptr && attachment.potentialParent != selectedPart->potentialParent) {
                deleteSymmetryParts();
                wipeSymmetry(selectedPart);
            }
            selectedPart->potentialParent = attachment.potentialParent;
            if (attachment.potentialParent != nullptr) {
                createSymmetry(symmetryMode, attachment);
            } else {
                deleteSymmetryParts();
                if (symmetryModeBeforeNodeAttachment >= 0) RestoreSymmetryModeBeforeNodeAttachment();
            }
            cPartAttachments = CheckSymPartsAttach(attachment);
            displayAttachNodeIcons(ship->liveParts, selectedPart, rules.stack, rules.srfAttach, rules.allowDock);
            GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartDragging, selectedPart);
        }
    });
    st_place.OnUpdate += KFSMCallback::Lambda(this, [this] { partRotationInputUpdate(); });   // base.partRotationInputUpdate
    st_place.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::deleteInputUpdate);
    st_place.OnLeave = KFSMStateChange::Lambda(this, [this](KFSMState* to) {
        if (!IsDestroyed(selectedPart)) {
            displayAttachNodeIcons(ship->liveParts, selectedPart, false, false, false);
            if (!attachmentSet_) throw NullReferenceException();   // `if (attachment.possible)` (the layer reset: the shell)
        }
        cleanSymmetry();
        CheckCopiedSymmetryPersistentId(selectedPart);
        GameEvents::onEditorPartPlaced.Fire(selectedPart);
        if (to == &st_idle && !IsDestroyed(selectedPart)) {
            selectedPart = nullptr;
            // UIPartActionControllerInventory.Instance.CurrentCargoPart = null: the inventory UI is the shell's
        }
        if (to != &st_rotate_tweak) srfAttachCursorOffset = Vector3f::zero();
    });
    fsm.AddState(&st_place);

    // ---- st_offset_select ----
    st_offset_select.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::UndoRedoInputUpdate);
    st_offset_select.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::snapInputUpdate);
    st_offset_select.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::partSearchUpdate);
    fsm.AddState(&st_offset_select);

    // ---- st_offset_tweak ----
    st_offset_tweak.OnEnter = KFSMStateChange::Lambda(this, [this](KFSMState*) {
        const EditorEnvironment& env = Env();
        if (env.onEditorStartTweak) env.onEditorStartTweak(*selectedPart);   // selectedPart.onEditorStartTweak()
        symUpdateMode = static_cast<int>(selectedPart->symmetryCounterparts.size());
        if (ship->Contains(selectedPart)) {
            symUpdateParent = selectedPart->parent;
            symUpdateAttachNode = selectedPart->FindAttachNodeByPart(symUpdateParent);
        } else {
            symUpdateParent = attachment.potentialParent;
            symUpdateAttachNode = attachment.callerPartNode;
        }
        AttachOffsetGizmo(*selectedPart);
        PlayClip(EditorClip::TweakGrab);
    });
    st_offset_tweak.OnUpdate += KFSMCallback::Lambda(this, [this] { partOffsetInputUpdate(); });
    st_offset_tweak.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::UndoRedoInputUpdate);
    st_offset_tweak.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::snapInputUpdate);
    st_offset_tweak.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::deleteInputUpdate);
    st_offset_tweak.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::partSearchUpdate);
    st_offset_tweak.OnUpdate += KFSMCallback::Lambda(this, [this] {
        if (KeyUpOf(GameSettings::Editor_coordSystem) && !gizmoOffsetObj_->IsDragging()) {
            switch (gizmoOffsetObj_->CoordSpace()) {
            case UnityEngine::Space::Self: gizmoOffsetObj_->SetCoordSystem(UnityEngine::Space::World); break;
            case UnityEngine::Space::World: gizmoOffsetObj_->SetCoordSystem(UnityEngine::Space::Self); break;
            }
            GizmoStateFromObject(false);
        }
    });
    st_offset_tweak.OnLeave = KFSMStateChange::Lambda(this, [this](KFSMState* to) {
        gizmoOffsetObj_.reset();   // gizmoOffset.Detach()
        symUpdateMode = 0;
        symUpdateParent = nullptr;
        symUpdateAttachNode = nullptr;
        if (to != &st_offset_tweak && to != &st_rotate_tweak && !IsDestroyed(selectedPart)) {
            const EditorEnvironment& env = Env();
            if (env.onEditorEndTweak) env.onEditorEndTweak(*selectedPart);
            if (to == &st_idle) selectedPart = nullptr;
        }
        PlayClip(EditorClip::TweakRelease);
    });
    fsm.AddState(&st_offset_tweak);

    // ---- st_rotate_select ----
    st_rotate_select.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::UndoRedoInputUpdate);
    st_rotate_select.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::snapInputUpdate);
    st_rotate_select.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::partSearchUpdate);
    fsm.AddState(&st_rotate_select);

    // ---- st_rotate_tweak ----
    st_rotate_tweak.OnEnter = KFSMStateChange::Lambda(this, [this](KFSMState*) {
        const EditorEnvironment& env = Env();
        if (env.onEditorStartTweak) env.onEditorStartTweak(*selectedPart);
        PartTransform& referenceTransform = ReferenceTransform(*selectedPart);
        symUpdateMode = static_cast<int>(selectedPart->symmetryCounterparts.size());
        if (ship->Contains(selectedPart)) {
            symUpdateParent = selectedPart->parent;
            symUpdateAttachNode = selectedPart->FindAttachNodeByPart(symUpdateParent);
        } else {
            symUpdateParent = attachment.potentialParent;
            symUpdateAttachNode = attachment.callerPartNode;
        }
        if (symUpdateAttachNode != nullptr) gizmoPivot = referenceTransform.TransformPoint(symUpdateAttachNode->position);
        else gizmoPivot = referenceTransform.position;
        AttachRotateGizmo(*selectedPart);
        gizmoAttRotate = selectedPart->fields.attRotation;
        gizmoAttRotate0 = selectedPart->fields.attRotation0;
        PlayClip(EditorClip::TweakGrab);
    });
    st_rotate_tweak.OnUpdate += KFSMCallback::Lambda(this, [this] { partRotationInputUpdate(); });
    st_rotate_tweak.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::UndoRedoInputUpdate);
    st_rotate_tweak.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::snapInputUpdate);
    st_rotate_tweak.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::deleteInputUpdate);
    st_rotate_tweak.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::partSearchUpdate);
    st_rotate_tweak.OnUpdate += KFSMCallback::Lambda(this, [this] {
        if (KeyUpOf(GameSettings::Editor_coordSystem) && !gizmoRotateObj_->IsDragging()) {
            switch (gizmoRotateObj_->CoordSpace()) {
            case UnityEngine::Space::Self: gizmoRotateObj_->SetCoordSystem(UnityEngine::Space::World); break;
            case UnityEngine::Space::World: gizmoRotateObj_->SetCoordSystem(UnityEngine::Space::Self); break;
            }
            GizmoStateFromObject(true);
        }
    });
    st_rotate_tweak.OnLeave = KFSMStateChange::Lambda(this, [this](KFSMState* to) {
        EditorEnvironment& env = Env();
        if (to == &st_place && !ship->Contains(selectedPart) && attachmentSet_ && attachment.possible && attachment.mode == AttachModes::SRF_ATTACH) {
            const Vector3f ws = env.worldToScreenPoint ? env.worldToScreenPoint(ReferenceTransform(*selectedPart).TransformPoint(attachment.callerPartNode->position))
                                                       : Vector3f::zero();
            srfAttachCursorOffset = In().mousePosition() - ws;
        } else {
            srfAttachCursorOffset = Vector3f::zero();
        }
        gizmoRotateObj_.reset();   // gizmoRotate.Detach()
        symUpdateMode = 0;
        symUpdateParent = nullptr;
        symUpdateAttachNode = nullptr;
        if (to != &st_offset_tweak && to != &st_rotate_tweak && !IsDestroyed(selectedPart)) {
            if (env.onEditorEndTweak) env.onEditorEndTweak(*selectedPart);
            if (to == &st_idle) selectedPart = nullptr;
        }
        PlayClip(EditorClip::TweakRelease);
    });
    fsm.AddState(&st_rotate_tweak);

    // ---- st_root_unselected ----
    st_root_unselected.OnEnter = KFSMStateChange::Lambda(this, [](KFSMState*) {
        // ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_125058"), modeMsg): the shell
    });
    st_root_unselected.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::UndoRedoInputUpdate);
    st_root_unselected.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::snapInputUpdate);
    st_root_unselected.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::partSearchUpdate);
    st_root_unselected.OnUpdate = KFSMCallback::Lambda(this, [] {});   // sic: the C# then assigns an empty delegate, dropping the three above
    st_root_unselected.OnLeave = KFSMStateChange::Lambda(this, [](KFSMState*) {});
    fsm.AddState(&st_root_unselected);

    // ---- st_root_select ----
    st_root_select.OnEnter = KFSMStateChange::Lambda(this, [this](KFSMState*) {
        rootCandidates.clear();   // rootCandidates = new List<PartSelector>()
        // part.SetHighlight(false) / SetHighlightType(Disabled) over FindPartsInChildren(selectedPart): the shell
        SyncReRootContext();
        std::vector<Part*> list3 = EditorReRootUtil::GetRootCandidates(EditorLogicBase::FindPartsInChildren(selectedPart));
        for (size_t i = 0; i < list3.size(); i++) rootCandidates.push_back(list3[i]);   // PartSelector.Create(part4, OnNewRootSelect, ...)
    });
    st_root_select.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::UndoRedoInputUpdate);
    st_root_select.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::snapInputUpdate);
    st_root_select.OnUpdate += KFSMCallback::Bind(this, &EditorLogic::partSearchUpdate);
    st_root_select.OnLeave = KFSMStateChange::Lambda(this, [this](KFSMState* to) {
        // SetHighlightDefault over FindPartsInChildren(selectedPart.localRoot), rootCandidates[n].Dismiss(): the shell
        rootCandidates.clear();
        if (to == &st_place) toolsSetMode(ConstructionMode::Place);
        else if (to == &st_offset_tweak) toolsSetMode(ConstructionMode::Move);
        else if (to == &st_rotate_tweak) toolsSetMode(ConstructionMode::Rotate);
    });
    fsm.AddState(&st_root_select);

    // ---- events ----
    on_podSelect.GoToStateOnEvent = &st_idle;
    on_podSelect.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_podSelect.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) { return !IsDestroyed(rootPart); });
    fsm.AddEvent(&on_podSelect, {&st_podSelect});

    on_podDeleted.GoToStateOnEvent = &st_podSelect;
    on_podDeleted.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_podDeleted.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) { return IsDestroyed(rootPart); });
    on_podDeleted.OnEvent = KFSMCallback::Lambda(this, [this] {
        SetBackup();
        // ship.vesselDeltaV destroy: the shell
        if (editorScreen == EditorScreen::Cargo) editorScreen = EditorScreen::Parts;   // SelectPanelParts()
        GameEvents::onEditorPodDeleted.Fire();
    });
    fsm.AddEventExcluding(&on_podDeleted, std::vector<KFSMState*>{});

    on_partCreated.GoToStateOnEvent = &st_place;
    on_partCreated.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_partCreated.OnEvent = KFSMCallback::Lambda(this, [this] {
        PlayClip(EditorClip::PartGrab);
        GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartCreated, selectedPart);
        isCurrentPartFlag = !IsDestroyed(selectedPart) && Env().hasFlagDecalBackground && Env().hasFlagDecalBackground(*selectedPart);
        // CreateSelectedPartIcon(): the shell
    });
    fsm.AddEventExcluding(&on_partCreated, {&st_place});

    on_partPicked.updateMode = KFSMUpdateMode::UPDATE;
    on_partPicked.GoToStateOnEvent = &st_place;
    on_partPicked.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) {
        if (InputLockManager::IsUnlocked(ControlTypes::EDITOR_PAD_PICK_PLACE) && In().GetMouseButtonUp(0) && !Mouse::Left.WasDragging(25.0f) &&
            fsm.FramesInCurrentState() > 1 && !PointerOverUI() && !HeldPartIsStacked()) {
            selectedPart = pickPart(layerMask | kMaskWithLayers, In().GetKey(KeyCode::LeftShift), !In().GetKey(KeyCode::LeftShift));
            isCurrentPartFlag = !IsDestroyed(selectedPart) && Env().hasFlagDecalBackground && Env().hasFlagDecalBackground(*selectedPart);
            // CreateSelectedPartIcon(): the shell
            if (!IsDestroyed(selectedPart) && !ModifierKeyHeld()) return !In().GetKey(KeyCode::LeftControl);
            return false;
        }
        return false;
    });
    on_partPicked.OnEvent = KFSMCallback::Lambda(this, [this] { OnPartPickedEvent(); });
    fsm.AddEvent(&on_partPicked, {&st_idle});

    on_partCopied.GoToStateOnEvent = &st_place;
    on_partCopied.updateMode = KFSMUpdateMode::UPDATE;
    on_partCopied.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) {
        if (InputLockManager::IsUnlocked(ControlTypes::EDITOR_PAD_PICK_PLACE) && In().GetMouseButtonUp(0) && !Mouse::Left.WasDragging(25.0f) &&
            fsm.FramesInCurrentState() > 1 && !PointerOverUI()) {
            selectedPart = pickPart(layerMask | kMaskWithLayers, In().GetKey(KeyCode::LeftShift), false);
            if (!IsDestroyed(selectedPart) && selectedPart != rootPart && ModifierKeyHeld()) return !In().GetKey(KeyCode::LeftControl);
            return false;
        }
        return false;
    });
    on_partCopied.OnEvent = KFSMCallback::Lambda(this, [this] {
        selectedPart->OnWillBeCopied(false);
        Part* part3 = DuplicatePart(selectedPart);
        selectedPart->OnWasCopied(part3, false);
        part3->OnCopy(selectedPart, false);
        selectedPart = part3;
        EditorLogicBase::clearAttachNodes(selectedPart, selectedPart->parent);
        selectedPart->clearParent();
        wipeSymmetry(selectedPart);
        if (IsFrozen(selectedPart)) unfreeze(selectedPart);
        CheckCopiedSymmetryPersistentId(selectedPart);
        selPartGrabOffset = Vector3f::zero();
        GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartCopied, selectedPart);
    });
    fsm.AddEvent(&on_partCopied, {&st_idle});

    on_partReveal.GoToStateOnEvent = &st_idle;
    on_partReveal.updateMode = KFSMUpdateMode::UPDATE;
    on_partReveal.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) {
        if (InputLockManager::IsUnlocked(ControlTypes::EDITOR_PAD_PICK_PLACE) && In().GetMouseButtonUp(0) && !Mouse::Left.WasDragging(25.0f) &&
            fsm.FramesInCurrentState() > 1 && !PointerOverUI()) {
            selectedPart = pickPart(layerMask | kMaskWithLayers, In().GetKey(KeyCode::LeftShift), false);
            if (!IsDestroyed(selectedPart)) return In().GetKey(KeyCode::LeftControl);
            return false;
        }
        return false;
    });
    on_partReveal.OnEvent = KFSMCallback::Lambda(this, [this] {
        const EditorEnvironment& env = Env();
        if (env.revealPart && selectedPart->partInfo != nullptr) env.revealPart(*selectedPart->partInfo);   // EditorPartList.Instance.RevealPart
        selectedPart = nullptr;
    });
    fsm.AddEvent(&on_partReveal, {&st_idle});

    on_partDropped.updateMode = KFSMUpdateMode::LATEUPDATE;
    on_partDropped.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) {
        if (!IsDestroyed(selectedPart) && fsm.FramesInCurrentState() > 0 && In().GetMouseButtonUp(0)) {
            if (!attachmentSet_) throw NullReferenceException();
            if (!attachment.possible) {
                const EditorEnvironment& env = Env();
                if (!(env.mouseOverModalArea && env.mouseOverModalArea()) && !PointerOverUI() && !HeldPartIsStacked()) {
                    bool flag2 = true;
                    int num4 = static_cast<int>(cPartAttachments.size());
                    while (num4-- > 0) {
                        if (cPartAttachments[static_cast<size_t>(num4)].collision) {
                            flag2 = false;
                            break;
                        }
                    }
                    if (!attachment.collision && flag2) {
                        if (selectedPart == rootPart) SetBackup();
                        return true;
                    }
                    PlayClip(EditorClip::CannotPlace);
                    return false;
                }
            }
        }
        return false;
    });
    on_partDropped.OnEvent = KFSMCallback::Lambda(this, [this] {
        PlayClip(EditorClip::PartRelease);
        isCurrentPartFlag = false;
        // layers and the highlighter: the shell
        deleteSymmetryParts();
        if (selectedPart != rootPart) freeze(selectedPart);
        // UIPartActionControllerInventory.Instance...: the inventory UI is the shell's
        GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartDropped, selectedPart);
        on_partDropped.GoToStateOnEvent = &st_idle;
    });
    fsm.AddEvent(&on_partDropped, {&st_place});

    on_partAttached.updateMode = KFSMUpdateMode::UPDATE;
    on_partAttached.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) {
        return !IsDestroyed(selectedPart) && In().GetMouseButtonUp(0) && fsm.FramesInCurrentState() > 0 && attachmentPossibleOrThrow() && !PointerOverUI();
    });
    on_partAttached.OnEvent = KFSMCallback::Lambda(this, [this] {
        isCurrentPartFlag = false;
        if (selectedPart->symmetryCounterparts.size() > 0) {
            RestoreSymmetryState();
            bool flag = true;
            int num3 = static_cast<int>(cPartAttachments.size());
            while (num3-- > 0) {
                if (!cPartAttachments[static_cast<size_t>(num3)].possible) {
                    flag = false;
                    break;
                }
            }
            if (!flag) {
                PlayClip(EditorClip::CannotPlace);
                on_partAttached.GoToStateOnEvent = &st_place;
                return;
            }
            attachPart(selectedPart, attachment);
            attachSymParts(cPartAttachments);
        } else {
            attachPart(selectedPart, attachment);
            if (symmetryModeBeforeNodeAttachment >= 0) RestoreSymmetryModeBeforeNodeAttachment();
        }
        SetBackup();
        // RefreshCrewAssignment, ModuleCargoPart / UIPartActionControllerInventory: the crew dialog and the inventory UI are the shell's
        PlayClip(EditorClip::Attach);
        on_partAttached.GoToStateOnEvent = &st_idle;
        CenterDragPlane(selectedPart->partTransform.position + selPartGrabOffset);
        GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartAttached, selectedPart);
    });
    fsm.AddEvent(&on_partAttached, {&st_place});

    on_partDeleted.GoToStateOnEvent = &st_idle;
    on_partDeleted.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_partDeleted.OnEvent = KFSMCallback::Lambda(this, [this] {
        if (!IsDestroyed(selectedPart)) DestroySelectedPart();
        isCurrentPartFlag = false;
        GameEvents::onEditorPartDeleted.Fire(selectedPart);
    });
    fsm.AddEvent(&on_partDeleted, {&st_place, &st_offset_tweak, &st_rotate_tweak});

    on_partLost.GoToStateOnEvent = &st_idle;
    on_partLost.updateMode = KFSMUpdateMode::UPDATE;
    on_partLost.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) { return IsDestroyed(selectedPart); });
    on_partLost.OnEvent = KFSMCallback::Lambda(this, [this] {
        log::Warning("[EditorLogic]: Selected Part was lost for unknown reasons! This is possibly unwanted behaviour.");
        isCurrentPartFlag = false;
        GameEvents::onEditorPartEvent.Fire(ConstructionEventType::Unknown, selectedPart);
    });
    fsm.AddEvent(&on_partLost, {&st_place});

    on_partOverInventoryPAW.GoToStateOnEvent = &st_idle;
    on_partOverInventoryPAW.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_partOverInventoryPAW.OnEvent = KFSMCallback::Lambda(this, [this] {
        if (IsDestroyed(selectedPart)) return;
        isCurrentPartFlag = false;
        // layers, highlighter, UIPartActionControllerInventory: the shell
        deleteSymmetryParts();
        if (selectedPart != rootPart) freeze(selectedPart);
        GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartOverInventoryGrid, selectedPart);
    });
    fsm.AddEventExcluding(&on_partOverInventoryPAW, std::vector<KFSMState*>{});

    on_goToModeOffset.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_goToModeOffset.OnEvent = KFSMCallback::Lambda(this, [this] {
        if (IsDestroyed(selectedPart)) {
            on_goToModeOffset.GoToStateOnEvent = &st_offset_select;
        } else if (!ship->Contains(selectedPart)) {
            on_goToModeOffset.GoToStateOnEvent = &st_place;
            on_partPicked.OnEvent();
        } else {
            on_goToModeOffset.GoToStateOnEvent = &st_offset_tweak;
        }
    });
    fsm.AddEvent(&on_goToModeOffset, {&st_idle, &st_rotate_select, &st_rotate_tweak, &st_root_unselected, &st_root_select});

    on_offsetSelect.updateMode = KFSMUpdateMode::UPDATE;
    on_offsetSelect.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) {
        if (In().GetMouseButtonUp(0) && !PointerOverUI()) {
            selectedPart = pickPart(layerMask | 4 | kMaskWithLayers, In().GetKey(KeyCode::LeftShift), false);
            if (!IsDestroyed(selectedPart)) {
                if (!ship->Contains(selectedPart)) {
                    on_offsetSelect.GoToStateOnEvent = &st_place;
                    on_partPicked.OnEvent();
                    return false;
                }
                on_offsetSelect.GoToStateOnEvent = &st_offset_tweak;
                return true;
            }
        }
        return false;
    });
    fsm.AddEvent(&on_offsetSelect, {&st_offset_select});

    on_offsetDeselect.GoToStateOnEvent = &st_offset_select;
    on_offsetDeselect.updateMode = KFSMUpdateMode::UPDATE;
    on_offsetDeselect.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) {
        const EditorEnvironment& env = Env();
        const bool overGizmo = env.mouseOverGizmo && env.mouseOverGizmo(true);
        if (Mouse::Left.GetButtonDown() && !Mouse::Left.WasDragging() && !overGizmo && !PointerOverUI()) {
            Part* part2 = pickPart(layerMask | 4 | kMaskWithLayers, In().GetKey(KeyCode::LeftShift), false);
            if (IsDestroyed(part2)) {
                if (env.onEditorEndTweak) env.onEditorEndTweak(*selectedPart);
                selectedPart = nullptr;
                return true;
            }
            if (EditorGeometryUtil::GetPixelDistance(env.worldToScreenPoint ? env.worldToScreenPoint(gizmoOffsetObj_->transform.position) : Vector3f::zero(),
                                                     In().mousePosition()) > 75.0f) {
                if (env.onEditorEndTweak) env.onEditorEndTweak(*selectedPart);
                selectedPart = part2;
                return true;
            }
        }
        return false;
    });
    fsm.AddEvent(&on_offsetDeselect, {&st_offset_tweak});

    on_offsetReset.GoToStateOnEvent = &st_offset_tweak;
    on_offsetReset.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    fsm.AddEvent(&on_offsetReset, {&st_offset_tweak});

    on_goToModeRotate.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_goToModeRotate.OnEvent = KFSMCallback::Lambda(this, [this] {
        if (IsDestroyed(selectedPart)) {
            on_goToModeRotate.GoToStateOnEvent = &st_rotate_select;
        } else if (!ship->Contains(selectedPart)) {
            on_goToModeRotate.GoToStateOnEvent = &st_place;
            on_partPicked.OnEvent();
        } else {
            on_goToModeRotate.GoToStateOnEvent = &st_rotate_tweak;
        }
    });
    fsm.AddEvent(&on_goToModeRotate, {&st_idle, &st_offset_select, &st_offset_tweak, &st_root_unselected, &st_root_select});

    on_rotateSelect.GoToStateOnEvent = &st_rotate_tweak;
    on_rotateSelect.updateMode = KFSMUpdateMode::UPDATE;
    on_rotateSelect.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) {
        if (In().GetMouseButtonUp(0) && !PointerOverUI()) {
            selectedPart = pickPart(layerMask | 4 | kMaskWithLayers, In().GetKey(KeyCode::LeftShift), false);
            if (!IsDestroyed(selectedPart)) {
                if (!ship->Contains(selectedPart)) {
                    on_rotateSelect.GoToStateOnEvent = &st_place;
                    on_partPicked.OnEvent();
                    return false;
                }
                on_rotateSelect.GoToStateOnEvent = &st_rotate_tweak;
                return true;
            }
        }
        return false;
    });
    fsm.AddEvent(&on_rotateSelect, {&st_rotate_select});

    on_rotateDeselect.GoToStateOnEvent = &st_rotate_select;
    on_rotateDeselect.updateMode = KFSMUpdateMode::UPDATE;
    on_rotateDeselect.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) {
        const EditorEnvironment& env = Env();
        const bool overGizmo = env.mouseOverGizmo && env.mouseOverGizmo(false);
        if (Mouse::Left.GetButtonDown() && !Mouse::Left.WasDragging() && !overGizmo && !PointerOverUI()) {
            Part* part = pickPart(layerMask | 4 | kMaskWithLayers, In().GetKey(KeyCode::LeftShift), false);
            if (IsDestroyed(part)) {
                if (env.onEditorEndTweak) env.onEditorEndTweak(*selectedPart);
                selectedPart = nullptr;
                return true;
            }
            if (EditorGeometryUtil::GetPixelDistance(env.worldToScreenPoint ? env.worldToScreenPoint(gizmoRotateObj_->transform.position) : Vector3f::zero(),
                                                     In().mousePosition()) > 75.0f) {
                if (env.onEditorEndTweak) env.onEditorEndTweak(*selectedPart);
                selectedPart = part;
                return true;
            }
        }
        return false;
    });
    fsm.AddEvent(&on_rotateDeselect, {&st_rotate_tweak});

    on_rotateReset.GoToStateOnEvent = &st_rotate_tweak;
    on_rotateReset.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    fsm.AddEvent(&on_rotateReset, {&st_rotate_tweak});

    on_goToModeRoot.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_goToModeRoot.OnEvent = KFSMCallback::Lambda(this, [this] {
        if (!IsDestroyed(selectedPart) &&
            (fsm.CurrentState() == &st_place || fsm.CurrentState() == &st_offset_tweak || fsm.CurrentState() == &st_rotate_tweak)) {
            on_goToModeRoot.GoToStateOnEvent = &st_root_select;
        } else {
            on_goToModeRoot.GoToStateOnEvent = &st_root_unselected;
        }
    });
    fsm.AddEventExcluding(&on_goToModeRoot, {&st_root_select, &st_root_unselected, &st_podSelect});

    on_rootPickSet.updateMode = KFSMUpdateMode::UPDATE;
    on_rootPickSet.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) {
        if (In().GetMouseButtonUp(0) && !Mouse::Left.WasDragging(25.0f) && !PointerOverUI()) {
            selectedPart = pickPart(layerMask | 4 | kMaskWithLayers, true, true);
            return !IsDestroyed(selectedPart);
        }
        return false;
    });
    on_rootPickSet.OnEvent = KFSMCallback::Lambda(this, [this] {
        if (selectedPart->children.size() > 0) {
            on_rootPickSet.GoToStateOnEvent = &st_root_select;
        } else {
            on_rootPickSet.GoToStateOnEvent = &st_root_unselected;
            PlayClip(EditorClip::CannotPlace);
            selectedPart = nullptr;
        }
    });
    fsm.AddEvent(&on_rootPickSet, {&st_root_unselected});

    on_rootDeselect.GoToStateOnEvent = &st_root_unselected;
    on_rootDeselect.updateMode = KFSMUpdateMode::LATEUPDATE;
    on_rootDeselect.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) {
        if (Mouse::Left.GetButtonUp() && !Mouse::Left.WasDragging() && IsDestroyed(pickPart(layerMask | 4 | kMaskWithLayers, false, false))) {
            // layers and SetHighlightDefault over FindPartsInChildren(selectedPart.localRoot): the shell
            selectedPart = nullptr;
            return true;
        }
        return false;
    });
    fsm.AddEvent(&on_rootDeselect, {&st_root_select});

    on_rootSelect.GoToStateOnEvent = &st_place;
    on_rootSelect.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_rootSelect.OnEvent = KFSMCallback::Lambda(this, [this] {
        PlayClip(EditorClip::ReRoot);
        GameEvents::onEditorPartEvent.Fire(ConstructionEventType::PartRootSelected, selectedPart);
    });
    fsm.AddEvent(&on_rootSelect, {&st_root_select});

    on_rootSelectFail.updateMode = KFSMUpdateMode::UPDATE;
    on_rootSelectFail.GoToStateOnEvent = &st_root_unselected;
    on_rootSelectFail.OnCheckCondition = KFSMEventCondition::Lambda(this, [this](KFSMState*) { return fsm.FramesInCurrentState() > 0 && rootCandidates.empty(); });
    on_rootSelectFail.OnEvent = KFSMCallback::Lambda(this, [this] {
        PlayClip(EditorClip::CannotPlace);
        selectedPart = nullptr;
    });
    fsm.AddEvent(&on_rootSelectFail, {&st_root_select});

    on_goToModePlace.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_goToModePlace.OnEvent = KFSMCallback::Lambda(this, [this] {
        if (!IsDestroyed(selectedPart) && !ship->Contains(selectedPart)) {
            unfreeze(selectedPart);
            on_goToModePlace.GoToStateOnEvent = &st_place;
        } else {
            on_goToModePlace.GoToStateOnEvent = &st_idle;
        }
    });
    fsm.AddEvent(&on_goToModePlace, {&st_offset_select, &st_offset_tweak, &st_rotate_select, &st_rotate_tweak, &st_root_unselected, &st_root_select});

    on_undoRedo.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_undoRedo.OnEvent = KFSMCallback::Lambda(this, [this] {
        if (fsm.CurrentState() == &st_rotate_tweak) on_undoRedo.GoToStateOnEvent = &st_rotate_select;
        else if (fsm.CurrentState() == &st_offset_tweak) on_undoRedo.GoToStateOnEvent = &st_offset_select;
    });
    fsm.AddEvent(&on_undoRedo, {&st_offset_tweak, &st_rotate_tweak});

    on_newShip.GoToStateOnEvent = &st_podSelect;
    on_newShip.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_newShip.OnEvent = KFSMCallback::Lambda(this, [this] {
        if (ship != nullptr) {
            for (size_t j = 0, count = ship->liveParts.size(); j < count; j++)
                if (ship->liveParts[j] != nullptr) Destroy(ship->liveParts[j]);   // DestroyImmediate(ship.parts[j].gameObject)
        }
        const std::vector<Part*> array2 = SceneParts();   // Part.allParts.ToArray()
        for (Part* p : array2) Destroy(p);
        selectedPart = nullptr;
    });
    fsm.AddEventExcluding(&on_newShip, std::vector<KFSMState*>{});

    on_shipLoaded.GoToStateOnEvent = &st_idle;
    on_shipLoaded.updateMode = KFSMUpdateMode::MANUAL_TRIGGER;
    on_shipLoaded.OnEvent = KFSMCallback::Lambda(this, [this] {
        const std::vector<Part*> array = SceneParts();
        for (Part* p : array)
            if (!ship->Contains(p)) Destroy(p);
        selectedPart = nullptr;
        // rootPart.gameObject.SetLayerRecursive, ResetCrewAssignment: the shell
    });
    fsm.AddEventExcluding(&on_shipLoaded, std::vector<KFSMState*>{});
}

void EditorLogic::SyncReRootContext() {
    EditorReRootUtil::context.selectedPart = selectedPart;
    EditorReRootUtil::context.shipContains = [this](const Part* p) { return ship != nullptr && ship->Contains(p); };
}

bool EditorLogic::attachmentPossibleOrThrow() {
    if (!attachmentSet_) throw NullReferenceException();
    return attachment.possible;
}

}  // namespace ksp
