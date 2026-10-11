// WP-33: EVAConstructionUtil.cs, EVAConstructionModeController.cs, EVAConstructionModeEditor.cs, EVAConstructionToolsUI.cs,
// PartItemTransfer.cs -- see construction.hpp. The UI-model classes (slot / grid / panel controllers) are in inventory_ui.cpp.
#include "ksp/inventory/construction.hpp"

#include <algorithm>
#include <cmath>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/editor/editor_geometry.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/util/ksp_util.hpp"

namespace ksp {

using GE = GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
AttachRules RulesOf(const Part& p) { return p.fields.attachRules.value_or(AttachRules{}); }
bool IsFlight() { return SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT; }
bool Contains(const std::vector<Vessel*>& v, Vessel* x) { return std::find(v.begin(), v.end(), x) != v.end(); }
Vector3d V3d(const Vector3f& v) { return Vector3d(v); }
// a null Part dereference is a NullReferenceException in C#
Part* Need(Part* p) {
    if (p == nullptr) throw NullReferenceException();
    return p;
}
// FlightGlobals.ActiveVessel.GetWorldPos3D() (the floating-origin world position the shell knows)
Vector3f ActiveWorldPos() {
    if (EVAConstructionEnvironment::activeVesselWorldPos) return EVAConstructionEnvironment::activeVesselWorldPos();
    return FlightRegistry::ActiveVessel->GetWorldPos3D().ToVector3f();
}
// FlightGlobals.getUpAxis(vessel.mainBody, position)
Vector3d UpAxis(Vessel& v, const Vector3d& pos) {
    if (EVAConstructionEnvironment::upAxisAt) return EVAConstructionEnvironment::upAxisAt(pos);
    return FlightMath::getUpAxis(*v.mainBody(), pos);
}
}  // namespace

// ================================================================ EVAConstructionEnvironment

void EVAConstructionEnvironment::Effect(const std::string& name, Part* part) {
    effects.push_back(name);
    if (onEffect) onEffect(name, part);
}

void EVAConstructionEnvironment::Reset() {
    effects.clear();
    onEffect = nullptr;
    hoveredPart = nullptr;
    mouseButtonUp0 = false;
    pointerOverUI = false;
    editorSoftLock = false;
    mouseIsMoving = false;
    groundRaycast = nullptr;
    upAxisAt = nullptr;
    cameraPlanePoint = nullptr;
    surfaceRaycastAll = nullptr;
    nodeIconUnderCursor = nullptr;
    getBoundsPoints = nullptr;
    cargoColliderHitsOtherPart = nullptr;
    cargoColliderPart = nullptr;
    cargoColliderCount = nullptr;
    addVessel = nullptr;
    setPhysicsHoldExpiryOverride = nullptr;
    orbitalPos = nullptr;
    activeVesselWorldPos = nullptr;
    uniqueFlightID = nullptr;
    currentOrbit = nullptr;
    interruptWeld = nullptr;
    weld = nullptr;
    isSeated = nullptr;
    engineerCount = nullptr;
}

// ================================================================ EVAConstructionUtil

// EVAConstructionUtil.cs:3
double EVAConstructionUtil::GetConstructionGee(Vessel* vessel) {
    if (vessel == nullptr) return PhysicsGlobals::GravitationalAcceleration;
    if (vessel->LandedOrSplashed()) return vessel->EVALadderVessel()->gravityForPos.magnitude();
    return vessel->EVALadderVessel()->geeForce * PhysicsGlobals::GravitationalAcceleration;
}

// Part.cs:4060
bool EVAConstructionUtil::IsUnderConstructionWeightLimit(Part& part) {
    if (InventoryEnvironment::IgnoreEVAConstructionMassLimit) return true;
    auto* cpCache = dynamic_cast<ModuleCargoPart*>(part.FindModuleOfClass("ModuleCargoPart"));
    float cpMass;
    if (cpCache != nullptr) cpMass = cpCache->MassForWeightTesting();
    else cpMass = part.fields.mass;
    double num = PhysicsGlobals::GravitationalAcceleration;
    if (part.vessel != nullptr) num = EVAConstructionUtil::GetConstructionGee(part.vessel);
    else if (FlightRegistry::ActiveVessel != nullptr) num = EVAConstructionUtil::GetConstructionGee(FlightRegistry::ActiveVessel);
    part.partWeight = static_cast<double>(f32store((fx(cpMass) + fx(part.GetResourceMass())) * 1000.0)) * num;
    EVAConstructionModeController* c = EVAConstructionModeController::Instance;
    if (IsFlight() && InventoryEnvironment::EVA_CONSTRUCTION_COMBINE_ENABLED && c != nullptr && c->evaEditor != nullptr)
        return part.partWeight <= c->evaEditor->CombinedConstructionWeightLimit();
    return part.partWeight <= PhysicsGlobals::ConstructionWeightLimit();
}

// ================================================================ EVAConstructionModeController

EVAConstructionModeController::EVAConstructionModeController() = default;
EVAConstructionModeController::~EVAConstructionModeController() {
    OnDestroy();
    if (Instance == this) Instance = nullptr;
}

// EVAConstructionModeController.cs:111
bool EVAConstructionModeController::MovementRestricted() {
    if (Instance != nullptr && Instance->evaEditor != nullptr) {
        if (Instance->IsOpen()) return Instance->evaEditor->SelectedPart() != nullptr;
        return false;
    }
    return false;
}

// EVAConstructionModeController.cs:140 (Awake :124 sets Instance)
void EVAConstructionModeController::Start() {
    if (Instance != nullptr && Instance != this) return;  // Awake: `if (Instance) Destroy(this); return;`
    Instance = this;
    loadedModuleInventoryPart.clear();
    displayedInventories.clear();
    GE::onGameSceneLoadRequested.Add(this, &EVAConstructionModeController::OnGameSceneLoadRequested);
    GE::onVesselChange.Add(this, &EVAConstructionModeController::OnVesselChange);
    GE::onEditorPartEvent.Add(this, &EVAConstructionModeController::OnEditorPartEvent);
    GE::onGUIActionGroupFlightShowing.Add(this, &EVAConstructionModeController::OnActionGroupsOpened);
    GE::OnCombinedConstructionWeightLimitChanged.Add(this, &EVAConstructionModeController::UpdateInfoLabels);
    eventsAdded_ = true;
}

// EVAConstructionModeController.cs:159
void EVAConstructionModeController::OnDestroy() {
    if (!eventsAdded_) return;
    GE::onGameSceneLoadRequested.Remove(this, &EVAConstructionModeController::OnGameSceneLoadRequested);
    GE::onVesselChange.Remove(this, &EVAConstructionModeController::OnVesselChange);
    GE::onEditorPartEvent.Remove(this, &EVAConstructionModeController::OnEditorPartEvent);
    GE::onGUIActionGroupFlightShowing.Remove(this, &EVAConstructionModeController::OnActionGroupsOpened);
    GE::OnCombinedConstructionWeightLimitChanged.Remove(this, &EVAConstructionModeController::UpdateInfoLabels);
    eventsAdded_ = false;
}

// EVAConstructionModeController.cs:187 (the EVA_CONSTRUCTION_MODE_TOGGLE key is the shell's: it calls Open/ClosePanel)
void EVAConstructionModeController::Update() {
    if (!isOpen) return;
    SearchForInventoryParts();
    UpdateDisplayedInventories();
    if (panelMode == PanelMode::Construction) {
        constructionGravity = EVAConstructionUtil::GetConstructionGee(FlightRegistry::ActiveVessel);
        if (std::fabs(lastConstructionGravity - constructionGravity) > 0.001) UpdateInfoLabels();
        lastConstructionGravity = constructionGravity;
    }
}

// EVAConstructionModeController.cs:229
void EVAConstructionModeController::OpenConstructionPanel() {
    if (CanOpenConstructionPanel() && !IsOpen()) {
        Vessel* active = FlightRegistry::ActiveVessel;
        activeEVAVessel = (active != nullptr && active->evaController != nullptr) ? active : nullptr;
        if (activeEVAVessel != nullptr && hooks.evaEnterExitConstruction) hooks.evaEnterExitConstruction(true);
        if (evaToolsUI != nullptr) evaToolsUI->ShowModeTools();
        if (hooks.ui) hooks.ui("construction:open");
        panelMode = PanelMode::Construction;
        isOpen = true;
        SearchForInventoryParts();
        UpdateInfoLabels();
        GE::OnEVAConstructionMode.Fire(true);
    }
}

// EVAConstructionModeController.cs:272
void EVAConstructionModeController::OpenCargoPanel() {
    if (CanOpenCargoPanel() && !IsOpen()) {
        if (evaToolsUI != nullptr) evaToolsUI->HideModeTools();
        if (hooks.ui) hooks.ui("cargo:open");
        panelMode = PanelMode::Cargo;
        isOpen = true;
        SearchForInventoryParts();
        GE::OnEVACargoMode.Fire(true);
    }
}

// EVAConstructionModeController.cs:297
void EVAConstructionModeController::ClosePanel() {
    if (!IsOpen()) return;
    evaEditor->ForceDrop();
    if (activeEVAVessel != nullptr && hooks.evaEnterExitConstruction) hooks.evaEnterExitConstruction(false);
    if (hooks.ui) hooks.ui("panel:close");
    isOpen = false;
    loadedModuleInventoryPart.clear();
    displayedInventories.clear();
    uiInventories.clear();
    if (panelMode == PanelMode::Construction) GE::OnEVAConstructionMode.Fire(false);
    if (panelMode == PanelMode::Cargo) GE::OnEVACargoMode.Fire(false);
}

// EVAConstructionModeController.cs:386
bool EVAConstructionModeController::CanOpenConstructionPanel() {
    Vessel* active = FlightRegistry::ActiveVessel;
    const bool map = hooks.mapIsEnabled ? hooks.mapIsEnabled() : false;
    const bool pause = hooks.flightDriverPause ? hooks.flightDriverPause() : false;
    const bool ag = hooks.actionGroupsOpen ? hooks.actionGroupsOpen() : false;
    if (IsFlight() && !map && active != nullptr && active->isEVA() &&
        (!hooks.repairSkillAtLeastZero || hooks.repairSkillAtLeastZero(*active)) && !pause && !ag)
        return true;
    return false;
}

// EVAConstructionModeController.cs:360
bool EVAConstructionModeController::CanOpenCargoPanel() {
    const bool map = hooks.mapIsEnabled ? hooks.mapIsEnabled() : false;
    if (IsFlight() && !map && FlightRegistry::ActiveVessel != nullptr && !CanOpenConstructionPanel()) return true;
    return false;
}

// EVAConstructionModeController.cs:369
void EVAConstructionModeController::OnGameSceneLoadRequested(GameScenes) {
    if (isOpen) ClosePanel();
}

// EVAConstructionModeController.cs:378
void EVAConstructionModeController::OnVesselChange(Vessel* vessel) {
    if (activeEVAVessel != nullptr && hooks.activeEvaInConstructionMode && hooks.activeEvaInConstructionMode()) {
        if (hooks.evaEnterExitConstruction) hooks.evaEnterExitConstruction(false);
    }
    if (isOpen) {
        if (!CanOpenConstructionPanel()) {
            ClosePanel();
            activeEVAVessel = nullptr;
        } else {
            activeEVAVessel = (vessel->evaController != nullptr) ? vessel : nullptr;
            if (activeEVAVessel != nullptr && hooks.evaEnterExitConstruction) hooks.evaEnterExitConstruction(true);
        }
    }
}

// EVAConstructionModeController.cs:403 (the slot highlight clears are UIPartActionInventory.SetAllSlotsNotSelected)
void EVAConstructionModeController::OnEditorPartEvent(ConstructionEventType eventType, Part* p) {
    switch (eventType) {
        case ConstructionEventType::PartPicked:
            RemoveInvDisplayItem(p);
            for (auto& d : displayedInventories)
                if (d.uiInventory) d.uiInventory->SetAllSlotsNotSelected();
            break;
        case ConstructionEventType::PartDragging: break;
        case ConstructionEventType::PartAttached:
            for (auto& d : displayedInventories)
                if (d.uiInventory) d.uiInventory->SetAllSlotsNotSelected();
            break;
        case ConstructionEventType::PartDropped:
        case ConstructionEventType::PartDetached:
            RemoveInvDisplayItem(p);
            for (auto& d : displayedInventories)
                if (d.uiInventory) d.uiInventory->SetAllSlotsNotSelected();
            break;
        default: break;
    }
}

ModuleInventoryPart* EVAConstructionModeController::Lookup(uint32_t id) const {
    for (const auto& kv : loadedModuleInventoryPart)
        if (kv.first == id) return kv.second;
    return nullptr;
}

// EVAConstructionModeController.cs:439
void EVAConstructionModeController::RemoveInvDisplayItem(Part* p) {
    if (!(p != nullptr)) return;
    auto it = std::find_if(loadedModuleInventoryPart.begin(), loadedModuleInventoryPart.end(),
                           [&](const auto& kv) { return kv.first == p->fields.persistentId; });
    if (it == loadedModuleInventoryPart.end()) return;
    InventoryDisplayItem* item = nullptr;
    if (TryGetDisplayedInventory(it->second, item)) {
        const int num = static_cast<int>(item - displayedInventories.data());
        if (num >= 0) {
            // Destroy(displayedInventories[num].uiObject)
            UIPartActionInventory* ui = displayedInventories[static_cast<size_t>(num)].uiInventory;
            displayedInventories.erase(displayedInventories.begin() + num);
            uiInventories.erase(std::remove_if(uiInventories.begin(), uiInventories.end(), [&](const auto& u) { return u.get() == ui; }),
                                uiInventories.end());
        }
    }
    // Object.Destroy(loadedModuleInventoryPart[p.persistentId]) destroys the module component (a kerbal module is destroyed too)
    loadedModuleInventoryPart.erase(it);
}

// EVAConstructionModeController.cs:458
void EVAConstructionModeController::OnActionGroupsOpened() { ClosePanel(); }

// EVAConstructionModeController.cs:463
void EVAConstructionModeController::SearchForInventoryParts() {
    const std::vector<Vessel*> vesselsLoaded = FlightRegistry::VesselsLoaded;
    for (Vessel* vessel : vesselsLoaded) {
        const std::vector<Part*>& parts = vessel->parts;
        for (size_t j = 0; j < parts.size(); j++) {
            auto* moduleInventoryPart = dynamic_cast<ModuleInventoryPart*>(parts[j]->FindModuleOfClass("ModuleInventoryPart"));
            const bool flag = parts[j]->isKerbalEVA() && parts.size() > 1;
            if (moduleInventoryPart != nullptr && !flag && Lookup(parts[j]->fields.persistentId) == nullptr)
                loadedModuleInventoryPart.emplace_back(parts[j]->fields.persistentId, moduleInventoryPart);
            if (vessel->isEVA()) continue;
            for (size_t k = 0; k < parts[j]->protoModuleCrew.size(); k++) {
                ModuleInventoryPart* kerbalInventoryModule = KerbalInventory::ModuleFor(*parts[j]->protoModuleCrew[k]);
                if (kerbalInventoryModule != nullptr) {
                    const uint32_t pid = parts[j]->protoModuleCrew[k]->persistentID;
                    kerbalInventoryModule->transformPosition = parts[j]->partTransform.position;
                    if (Lookup(pid) == nullptr) loadedModuleInventoryPart.emplace_back(pid, kerbalInventoryModule);
                }
            }
        }
    }
}

// EVAConstructionModeController.cs:502
void EVAConstructionModeController::UpdateDisplayedInventories() {
    Vector3f position(3.4028235e+38f, 3.4028235e+38f, 3.4028235e+38f);  // maxVector
    if (FlightRegistry::ActiveVessel != nullptr) position = FlightRegistry::ActiveVessel->vesselTransform.position;
    // foreach over the key collection; the C# Remove of a null module throws on the next MoveNext after the null dereference below
    const std::vector<std::pair<uint32_t, ModuleInventoryPart*>> snapshot = loadedModuleInventoryPart;
    for (const auto& kv : snapshot) {
        ModuleInventoryPart* moduleInventoryPart = kv.second;
        if (moduleInventoryPart == nullptr) throw NullReferenceException();
        float num = 0.0f;
        num = (!moduleInventoryPart->kerbalMode()) ? Vector3f::Distance(moduleInventoryPart->PartOf()->partTransform.position, position)
                                                   : Vector3f::Distance(moduleInventoryPart->transformPosition, position);
        InventoryDisplayItem* displayItem = nullptr;
        if (!TryGetDisplayedInventory(moduleInventoryPart, displayItem)) {
            if (!(InventoryEnvironment::EVA_INVENTORY_RANGE > num)) continue;
            InventoryDisplayItem item;
            item.inventoryModule = moduleInventoryPart;
            item.hasUiObject = true;
            auto ui = std::make_unique<UIPartActionInventory>();
            item.uiInventory = ui.get();
            item.uiInventory->SetupConstruction(*moduleInventoryPart);
            uiInventories.push_back(std::move(ui));
            displayedInventories.push_back(item);
        } else if (InventoryEnvironment::EVA_INVENTORY_RANGE < num) {
            UIPartActionInventory* ui = displayItem->uiInventory;
            const int idx = static_cast<int>(displayItem - displayedInventories.data());
            displayedInventories.erase(displayedInventories.begin() + idx);
            moduleInventoryPart->constructorModeInventory = nullptr;
            uiInventories.erase(std::remove_if(uiInventories.begin(), uiInventories.end(), [&](const auto& u) { return u.get() == ui; }),
                                uiInventories.end());
        }
    }
}

// EVAConstructionModeController.cs:550
void EVAConstructionModeController::UpdateInfoLabels() {
    AssistingKerbalsLabelText = Localizer::Format(InventoryEnvironment::EVA_CONSTRUCTION_COMBINE_NONENGINEERS ? "#autoLOC_8014163" : "#autoLOC_8014164");
    AssistingKerbalsNumberText = dotnet::FormatInt64(evaEditor->AssistingKerbals());
    double val = constructionGravity;
    val = std::max(val, 1E-06);
    const double num = evaEditor->CombinedConstructionWeightLimit() / val;
    if (num < 100000.0) {
        const float f = f32store(fx(static_cast<float>(num)) * fx(0.001f));
        MaxMassLimitNumberText = dotnet::FormatSingleF(f, 2) + "t";  // StringBuilderCache.Format("{0:F2}t", (float)num * 0.001f)
    } else {
        MaxMassLimitNumberText = Localizer::Format("#autoLOC_8014166");
    }
}

// EVAConstructionModeController.cs:567
void EVAConstructionModeController::DestroyHeldIcons(UIPartActionInventory* callingInventory) {
    for (auto& d : displayedInventories)
        if (d.uiInventory != callingInventory) d.uiInventory->DestroyHeldPart(true);
}

// EVAConstructionModeController.cs:578
bool EVAConstructionModeController::TryGetDisplayedInventory(ModuleInventoryPart* inventoryModule, InventoryDisplayItem*& displayItem) {
    for (auto& d : displayedInventories) {
        if (d.inventoryModule == inventoryModule) {
            displayItem = &d;
            return true;
        }
    }
    displayItem = nullptr;
    return false;
}

// ================================================================ EVAConstructionToolsUI

// EVAConstructionToolsUI.cs:25 (Editor_modePlace / Editor_modeOffset / Editor_modeRotate GetKeyDown)
void EVAConstructionToolsUI::Update(bool placeKeyDown, bool offsetKeyDown, bool rotateKeyDown) {
    EVAConstructionModeController* c = EVAConstructionModeController::Instance;
    const bool focus = anyTextFieldHasFocus ? anyTextFieldHasFocus() : false;
    if ((!(c != nullptr) || c->IsOpen() || !focus)) {
        if (c == nullptr) throw NullReferenceException();  // `Instance.panelMode` after the guard
        if (c->panelMode == EVAConstructionModeController::PanelMode::Construction) {
            if (placeKeyDown) SetMode(ConstructionMode::Place);
            if (offsetKeyDown) SetMode(ConstructionMode::Move);
            if (rotateKeyDown) SetMode(ConstructionMode::Rotate);
        }
    }
}

// EVAConstructionToolsUI.cs:65
void EVAConstructionToolsUI::onPlaceButtonInput(bool b) {
    if (b && placeButton.interactable) SetMode(ConstructionMode::Place, false);
}

// EVAConstructionToolsUI.cs:73
void EVAConstructionToolsUI::onMoveButtonInput(bool b) {
    if (b && moveButton.interactable) SetMode(ConstructionMode::Move, false);
}

// EVAConstructionToolsUI.cs:81
void EVAConstructionToolsUI::onRotateButtonInput(bool b) {
    if (b && rotateButton.interactable) SetMode(ConstructionMode::Rotate, false);
}

// EVAConstructionToolsUI.cs:89
void EVAConstructionToolsUI::SetMode(ConstructionMode mode, bool updateUI) {
    EVAConstructionModeController* c = EVAConstructionModeController::Instance;
    if (!(c == nullptr) && !(c->evaEditor == nullptr)) {
        if (mode != ConstructionMode::Place && gizmoToolsLocked && gizmoToolsLocked()) {
            OnChangeModeBlocked();
            return;
        }
        if (constructionMode == ConstructionMode::Place && (mode == ConstructionMode::Move || mode == ConstructionMode::Rotate) &&
            c->evaEditor->SelectedPart() != nullptr) {
            c->evaEditor->PlayAudioClip("cannotPlaceClip");
            OnChangeModeBlocked();
            return;
        }
        if (constructionMode != mode) {
            constructionMode = mode;
            GE::OnEVAConstructionModeChanged.Fire(constructionMode);
        }
        if (updateUI) {
            switch (mode) {
                case ConstructionMode::Place: placeButton.isOn = true; break;
                case ConstructionMode::Move: moveButton.isOn = true; break;
                case ConstructionMode::Rotate: rotateButton.isOn = true; break;
                case ConstructionMode::Root: break;
            }
        }
    } else {
        OnChangeModeBlocked();
    }
}

// EVAConstructionToolsUI.cs:133
void EVAConstructionToolsUI::OnChangeModeBlocked() {
    placeButton.isOn = constructionMode == ConstructionMode::Place;
    moveButton.isOn = constructionMode == ConstructionMode::Move;
    rotateButton.isOn = constructionMode == ConstructionMode::Rotate;
}

// EVAConstructionToolsUI.cs:149
void EVAConstructionToolsUI::ShowModeTools() {
    placeButton.active = moveButton.active = rotateButton.active = coordButtonActive = true;
}

// EVAConstructionToolsUI.cs:157
void EVAConstructionToolsUI::HideModeTools() {
    placeButton.active = moveButton.active = rotateButton.active = coordButtonActive = false;
}

// ================================================================ PartItemTransfer

PartItemTransfer::~PartItemTransfer() {
    if (!destroyed) OnDestroy();
}

// PartItemTransfer.cs:35
void PartItemTransfer::Setup(Part& src, const std::string& itemType, const std::string& itemName, const std::string& semiValidMsg,
                             std::function<void(DismissAction, Part*)> onDialogDismiss) {
    type = Localizer::Format(itemType);
    onDismiss = std::move(onDialogDismiss);
    srcPart = &src;
    semiValidMessage = semiValidMsg;
    if (Instance != nullptr) {
        DismissAlreadyRunning();
        return;
    }
    Instance = this;
    semiValidParts.clear();
    validParts.clear();
    int count = static_cast<int>(srcPart->vessel->parts.size());
    while (count-- > 0) {
        Part* part = srcPart->vessel->parts[static_cast<size_t>(count)];
        if (part != srcPart) {
            if (IsValidPart(*part)) validParts.push_back(part);
            else if (IsSemiValidPart(*part)) semiValidParts.push_back(part);
        }
    }
    AfterPartsFound();
    partSelectors.clear();
    int count2 = static_cast<int>(semiValidParts.size());
    while (count2-- > 0) partSelectors.emplace_back(semiValidParts[static_cast<size_t>(count2)], 0);
    int count3 = static_cast<int>(validParts.size());
    while (count3-- > 0) partSelectors.emplace_back(validParts[static_cast<size_t>(count3)], 1);
    partSelectors.emplace_back(srcPart, 2);
    controlLockSet = true;  // InputLockManager.SetControlLock(~(UI | CAMERAMODES | CAMERACONTROLS), "itemTransfer")
    screenMessages.push_back(Localizer::Format("#autoLOC_135800", {"#" /*XKCDColors.HexFormat.BrightAqua*/, itemName}));
    GE::onVesselWasModified.Add(this, &PartItemTransfer::onVesselModified);
    GE::onVesselSituationChange.Add(this, &PartItemTransfer::onVesselSituationChange);
    GE::onVesselChange.Add(this, &PartItemTransfer::onVesselChanged);
    eventsAdded_ = true;
    HookAdditionalEvents();
    GE::onItemTransferStarted.Fire(this);
}

// PartItemTransfer.cs:91
void PartItemTransfer::DismissInterrupted() {
    screenMessages.push_back(Localizer::Format("#autoLOC_135812", {type}));
    Dismiss(DismissAction::Interrupted, nullptr);
}

// PartItemTransfer.cs:97
void PartItemTransfer::DismissActive() {
    if (Instance != nullptr) Instance->DismissInterrupted();
}

// PartItemTransfer.cs:105
void PartItemTransfer::DismissAlreadyRunning() {
    screenMessages.push_back(Localizer::Format("#autoLOC_135824", {type}));
    Dismiss(DismissAction::Interrupted, nullptr);
}

// PartItemTransfer.cs:111 (ScreenMessages.RemoveMessage; Object.Destroy(gameObject) -> OnDestroy)
void PartItemTransfer::Dismiss(DismissAction dma, Part* p) {
    if (onDismiss) onDismiss(dma, p);
    OnDestroy();
}

// PartItemTransfer.cs:121
void PartItemTransfer::OnDestroy() {
    if (destroyed) return;
    destroyed = true;
    if (eventsAdded_) {
        GE::onVesselWasModified.Remove(this, &PartItemTransfer::onVesselModified);
        GE::onVesselSituationChange.Remove(this, &PartItemTransfer::onVesselSituationChange);
        GE::onVesselChange.Remove(this, &PartItemTransfer::onVesselChanged);
        UnhookAdditionalEvents();
        eventsAdded_ = false;
    }
    controlLockSet = false;
    partSelectors.clear();
    if (Instance == this) Instance = nullptr;
}

// PartItemTransfer.cs:155
void PartItemTransfer::onVesselModified(Vessel* v) {
    if (v == srcPart->vessel) DismissInterrupted();
}

// PartItemTransfer.cs:163
void PartItemTransfer::onVesselSituationChange(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> vs) {
    if (vs.host == srcPart->vessel) DismissInterrupted();
}

// PartItemTransfer.cs:171
void PartItemTransfer::onVesselChanged(Vessel*) { DismissInterrupted(); }

// PartItemTransfer.cs:176
void PartItemTransfer::OnSrcPartSelect(Part*) { screenMessages.push_back(Localizer::Format("#autoLOC_135891")); }

// PartItemTransfer.cs:181
void PartItemTransfer::OnSemiValidPartSelect(Part*) { screenMessages.push_back(Localizer::Format(semiValidMessage)); }

// PartItemTransfer.cs:186
void PartItemTransfer::OnPartSelect(Part* p) { Dismiss(DismissAction::ItemMoved, p); }

// PartItemTransfer.cs:191
void PartItemTransfer::LateUpdate(bool escapeUp) {
    if (escapeUp) Dismiss(DismissAction::Cancelled, nullptr);
}

// ================================================================ EVAConstructionModeEditor

void EVAConstructionModeEditor::Effect(const std::string& name, Part* p) { EVAConstructionEnvironment::Effect(name, p); }

// EVAConstructionModeEditor.cs:155 / :170
void EVAConstructionModeEditor::Awake() {
    GE::OnEVAConstructionModeChanged.Add(this, &EVAConstructionModeEditor::OnConstructionModeChanged);
    GE::OnInventoryPartOnMouseChanged.Add(this, &EVAConstructionModeEditor::OnCurrentMousePartChanged);
    GE::OnEVAConstructionMode.Add(this, &EVAConstructionModeEditor::OnConstructionMode);
    GE::onVesselUnloaded.Add(this, &EVAConstructionModeEditor::OnVesselUnloaded);
    GE::onGameAboutToQuicksave.Add(this, &EVAConstructionModeEditor::OnGameAboutToQuicksave);
    GE::OnMapEntered.Add(this, &EVAConstructionModeEditor::OnMapViewEntered);
    eventsAdded_ = true;
    attachVessels.clear();
}

// EVAConstructionModeEditor.cs:244
void EVAConstructionModeEditor::OnDestroy() {
    if (eventsAdded_) {
        GE::OnEVAConstructionModeChanged.Remove(this, &EVAConstructionModeEditor::OnConstructionModeChanged);
        GE::OnInventoryPartOnMouseChanged.Remove(this, &EVAConstructionModeEditor::OnCurrentMousePartChanged);
        GE::OnEVAConstructionMode.Remove(this, &EVAConstructionModeEditor::OnConstructionMode);
        GE::onVesselUnloaded.Remove(this, &EVAConstructionModeEditor::OnVesselUnloaded);
        GE::onGameAboutToQuicksave.Remove(this, &EVAConstructionModeEditor::OnGameAboutToQuicksave);
        GE::OnMapEntered.Remove(this, &EVAConstructionModeEditor::OnMapViewEntered);
        eventsAdded_ = false;
    }
    RemoveAttachNodes();
}

EVAConstructionModeEditor::~EVAConstructionModeEditor() {
    try {
        OnDestroy();
    } catch (const std::exception&) {  // a MonoBehaviour exception is logged, never propagated
    }
}

// EVAConstructionModeEditor.cs:180
void EVAConstructionModeEditor::Update(const UpdateInput& in) {
    FlushDestroyed();  // the previous frame's Object.Destroy calls
    EVAConstructionModeController* c = EVAConstructionModeController::Instance;
    if (!c->IsOpen() || c->panelMode != EVAConstructionModeController::PanelMode::Construction) return;
    UpdateAssistingKerbals();
    SnapInputUpdate(in.snapToggleKeyDown);
    if (selectedPart != nullptr) {
        Effect("part:OnConstructionModeUpdate", selectedPart);
        for (size_t i = 0; i < selectedPart->Modules().Count(); i++) selectedPart->Modules()[i]->OnConstructionModeUpdate();
    }
    switch (EVAConstructionMode()) {
        case ConstructionMode::Place:
            InputCheckDetachCompundPart(in.pauseKeyDown);
            UpdatePartPlacementPosition();
            if (!EVAConstructionEnvironment::editorSoftLock) {
                AttachInput();
                DetachInput();
                DropPartInput();
                PickupPartInput();
            }
            if (selectedPart != nullptr) {
                ProcessAttachNodes();
                partRotationInputUpdate(in.rotationKey, false);
            } else {
                RemoveAttachNodes();
            }
            attachSuccesful = false;
            break;
        case ConstructionMode::Move:
            RemoveAttachNodes();
            SelectPartInput();
            CoordToggleInput(in.coordToggleKeyUp);
            break;
        case ConstructionMode::Rotate:
            RemoveAttachNodes();
            SelectPartInput();
            CoordToggleInput(in.coordToggleKeyUp);
            break;
        default: break;
    }
}

// EVAConstructionModeEditor.cs:232
void EVAConstructionModeEditor::FixedUpdate() {
    EVAConstructionModeController* c = EVAConstructionModeController::Instance;
    if (c->IsOpen() && c->panelMode == EVAConstructionModeController::PanelMode::Construction && selectedPart != nullptr) {
        Effect("part:OnConstructionModeFixedUpdate", selectedPart);
        for (size_t i = 0; i < selectedPart->Modules().Count(); i++) selectedPart->Modules()[i]->OnConstructionModeFixedUpdate();
    }
}

// EVAConstructionModeEditor.cs:256
void EVAConstructionModeEditor::OnCurrentMousePartChanged(Part* p) {
    if (!IsFlight()) return;
    if (selectedPart != nullptr) {
        isCurrentPartFlag = selectedPart->FindModuleOfClass("FlagDecalBackground") != nullptr;
        ToggleCollidersTrigger(false);
        handleAttachNodeIcons(selectedPart, false, false, false);
    } else {
        isCurrentPartFlag = false;
    }
    bool currentPartIsAttachable = false;
    if (p != nullptr && p->FindModuleOfClass("ModuleCargoPart") != nullptr) {
        currentPartIsAttachable = true;
        InterruptKerbalWelding();
        selectedPart = p;
        switch (evaConstructionMode) {
            case ConstructionMode::Place:
                FetchColliders();
                ToggleCollidersTrigger(true);
                ToggleOffsetGizmo(false);
                ToggleRotateGizmo(false);
                ToggleCoordSpaceButton(false);
                break;
            case ConstructionMode::Move:
                ToggleRotateGizmo(false);
                ToggleOffsetGizmo(selectedPart->PartCanBeOffset());
                ToggleCoordSpaceButton(true);
                break;
            case ConstructionMode::Rotate:
                ToggleRotateGizmo(selectedPart->PartCanBeRotated());
                ToggleOffsetGizmo(false);
                ToggleCoordSpaceButton(true);
                break;
            default: break;
        }
    } else {
        DestroyHeldPart();
    }
    if (UIPartActionControllerInventory::Instance != nullptr) UIPartActionControllerInventory::Instance->currentPartIsAttachable = currentPartIsAttachable;
}

// EVAConstructionModeEditor.cs:309
void EVAConstructionModeEditor::OnConstructionModeChanged(ConstructionMode mode) {
    if (mode == evaConstructionMode) return;
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    switch (mode) {
        case ConstructionMode::Place:
            if (selectedPart != nullptr && (evaConstructionMode == ConstructionMode::Move || evaConstructionMode == ConstructionMode::Rotate)) {
                if (selectedPart->fields.previousPhysicalSignificance != selectedPart->fields.physicalSignificance) {
                    if (InventoryEnvironment::scheduler)
                        InventoryEnvironment::scheduler->StartCoroutine(ResetToPhysicalPart(selectedPart), 0x7ffffff0);
                }
                ToggleCollidersTrigger(false);
                selectedPart = nullptr;
                rootPart = nullptr;
                if (inv != nullptr) {
                    if (inv->currentInventory != nullptr) inv->currentInventorySlotClicked->UpdateCurrentSelectedSlot(false);
                    inv->ResetInventoryCacheValues();
                }
            }
            ToggleRotateGizmo(false);
            ToggleOffsetGizmo(false);
            ToggleCoordSpaceButton(false);
            break;
        case ConstructionMode::Move:
            if (selectedPart != nullptr && selectedPart->fields.physicalSignificance == static_cast<int32_t>(Part_PhysicalSignificance::FULL))
                selectedPart->DemoteToPhysicslessPart();
            ToggleRotateGizmo(false);
            ToggleOffsetGizmo(selectedPart != nullptr && selectedPart->PartCanBeOffset());
            ToggleCoordSpaceButton(true);
            break;
        case ConstructionMode::Rotate:
            if (selectedPart != nullptr && selectedPart->fields.physicalSignificance == static_cast<int32_t>(Part_PhysicalSignificance::FULL))
                selectedPart->DemoteToPhysicslessPart();
            ToggleRotateGizmo(selectedPart != nullptr && selectedPart->PartCanBeRotated());
            ToggleOffsetGizmo(false);
            ToggleCoordSpaceButton(true);
            break;
        default: break;
    }
    evaConstructionMode = mode;
}

// EVAConstructionModeEditor.cs:372
Enumerator EVAConstructionModeEditor::ResetToPhysicalPart(Part* p) {
    p->PromoteToPhysicalPart();
    co_yield YieldInstruction::Null();
    Effect("part:rb.isKinematic=packed", p);
    Effect("part:ModulesBeforePartAttachJoint", p);
    if (p->fields.physicalSignificance == static_cast<int32_t>(Part_PhysicalSignificance::FULL)) Effect("part:CreateAttachJoint", p);
    Effect("part:CycleAutoStrut", p);
}

// EVAConstructionModeEditor.cs:385
void EVAConstructionModeEditor::UpdatePartPlacementPosition() {
    isPlacementValid_ = false;
    isPlacementOnGround = false;
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    EVAConstructionModeController* ctl = EVAConstructionModeController::Instance;
    Vessel* active = FlightRegistry::ActiveVessel;
    if ((!(inv != nullptr) || !inv->IsCursorOverAnyPAWOrCargoPane) && !UIPartActionControllerInventory::heldPartIsStack && !ctl->Hover() &&
        !(active == nullptr) && active->isEVA() && !(selectedPart == nullptr)) {
        if (EVAConstructionEnvironment::groundRaycast) {
            if (auto g = EVAConstructionEnvironment::groundRaycast()) {
                Vector3f point = g->point;
                const Vector3d upAxis = UpAxis(*active, V3d(point));
                float centerPointOffset = 0.0f;
                float boundsPoints = EVAConstructionEnvironment::getBoundsPoints
                                         ? EVAConstructionEnvironment::getBoundsPoints(*selectedPart, g->normal, centerPointOffset)
                                         : 0.0f;
                boundsPoints = f32store(fx(boundsPoints) + fx(placementGroundOffset));
                point = (V3d(point) + upAxis * static_cast<double>(boundsPoints)).ToVector3f();
                if (Vector3f::Distance(point, ActiveWorldPos()) < InventoryEnvironment::EVA_CONSTRUCTION_RANGE) {
                    selectedPart->partTransform.position = point;
                    isPlacementValid_ = true;
                    isPlacementOnGround = true;
                }
            }
        }
        if (!isPlacementValid_) {
            if (EVAConstructionEnvironment::cameraPlanePoint) {
                if (auto p2 = EVAConstructionEnvironment::cameraPlanePoint()) {
                    if (Vector3f::Distance(*p2, active->ReferenceTransform().position) < InventoryEnvironment::EVA_CONSTRUCTION_RANGE) {
                        selectedPart->partTransform.position = *p2;
                        isPlacementValid_ = true;
                    } else {
                        isPlacementValid_ = false;
                    }
                }
            }
        }
        vesselRotation = selectedPartOriginalRotation;
        selectedPart->partTransform.rotation = vesselRotation * selectedPart->fields.attRotation;
        attachment = CheckAttach(selectedPart);
        attachmentNull = false;
        attachmentPossible = attachment.possible &&
                             Vector3f::Distance(attachment.position, active->ReferenceTransform().position) < InventoryEnvironment::EVA_CONSTRUCTION_RANGE;
        if (attachmentPossible) {
            selectedPart->fields.attRotation0 = attachment.rotation;
            selectedPart->fields.attPos0 = attachment.position;
            Effect("part:SetParent(null)", selectedPart);
            selectedPart->partTransform.rotation = attachment.rotation * selectedPart->fields.attRotation;
            selectedPart->partTransform.position = attachment.position;
        } else {
            selectedPart->partTransform.rotation = selectedPartOriginalRotation * selectedPart->fields.attRotation;
        }
        const bool flag = EVAConstructionEnvironment::hoveredPart != nullptr;
        isPlacementValid_ = !flag || (flag && (RulesOf(*selectedPart).allowSrfAttach || RulesOf(*selectedPart).allowStack));
        if (EVAConstructionEnvironment::cargoColliderCount && EVAConstructionEnvironment::cargoColliderPart) {
            int count = static_cast<int>(EVAConstructionEnvironment::cargoColliderCount(*selectedPart));
            while (count-- > 0) {
                Part* partUpwardsCached = EVAConstructionEnvironment::cargoColliderPart(*selectedPart, static_cast<size_t>(count));
                if (!(partUpwardsCached != nullptr)) {
                    isPlacementValid_ = false;
                    attachmentPossible = false;
                    continue;
                }
                if (attachment.potentialParent == nullptr) {
                    isPlacementValid_ = false;
                    attachmentPossible = false;
                } else if (partUpwardsCached->fields.persistentId != attachment.potentialParent->fields.persistentId &&
                           partUpwardsCached->fields.persistentId != selectedPart->fields.persistentId) {
                    isPlacementValid_ = false;
                    attachmentPossible = false;
                }
                break;
            }
        }
        HighLightPossibleAttach(attachmentPossible || isPlacementValid_);
        selectedPart->potentialParent = attachment.potentialParent;
        if (EVAConstructionEnvironment::mouseIsMoving) GE::onEditorPartEvent.Fire(ConstructionEventType::PartDragging, selectedPart);
    } else {
        attachmentNull = true;
        attachmentPossible = false;
        if (selectedPart == nullptr) HighLightPossibleAttach(false);
    }
}

// EVAConstructionModeEditor.cs:489
void EVAConstructionModeEditor::HighLightPossibleAttach(bool highlight) {
    if (selectedPart == nullptr) return;
    std::string color = highlight ? "editorAttached" : "editorDetached";
    if (highlight && IsPlacementValid() && !attachmentPossible) color = "constructionPartDropAsNewVessel";
    lastHighlightColor = color;
    Effect("highlight:" + color + (!highlight ? ":OnMouseOver" : ":AlwaysOn"), selectedPart);
}

// EVAConstructionModeEditor.cs:506
void EVAConstructionModeEditor::PickupPartInput() {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    Part* hovered = EVAConstructionEnvironment::hoveredPart;
    if (EVAConstructionEnvironment::mouseButtonUp0 && selectedPart == nullptr && hovered != nullptr &&
        Vector3f::Distance(hovered->partTransform.position, ActiveWorldPos()) <
            InventoryEnvironment::EVA_CONSTRUCTION_RANGE &&
        inv != nullptr && !inv->IsCursorOverAnyPAWOrCargoPane && !EVAConstructionModeController::Instance->Hover() &&
        !EVAConstructionEnvironment::pointerOverUI)
        PickupPart();
}

// EVAConstructionModeEditor.cs:514
void EVAConstructionModeEditor::PickupPart() {
    if (attachSuccesful) return;
    Part* hoveredPart = EVAConstructionEnvironment::hoveredPart;
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (hoveredPart->vessel != nullptr && (hoveredPart->vessel->vesselType == VesselType::DroppedPart || hoveredPart->vessel->vesselType == VesselType::Debris)) {
        if (!CanPartBeEdited(*hoveredPart, false)) return;
        if (hoveredPart->protoPartSnapshot != nullptr) {
            // hoveredPart.protoPartSnapshot = new ProtoPartSnapshot(hoveredPart, hoveredPart.vessel.protoVessel)
            ownedSnapshots.push_back(std::make_unique<ProtoPartSnapshot>(*hoveredPart, hoveredPart->vessel->protoVessel));
            hoveredPart->protoPartSnapshot = ownedSnapshots.back().get();
            if (hoveredPart->vessel != nullptr) {
                BackupPart(hoveredPart, nullptr, true);
                hoveredPart->vessel->Die();
            }
            if (inv != nullptr) {
                std::unique_ptr<Part> created = inv->CreatePartFromInventory(*hoveredPart->protoPartSnapshot);
                Part* raw = AdoptPart(std::move(created));
                inv->SetCurrentCargoPart(raw);
                inv->isSetAsPart = true;
            }
        }
        if (selectedPart != nullptr) {
            selectedPart->fields.attRotation = Quaternionf::identity();
            selectedPart->fields.attRotation0 = EditorLogic::LocalRotation(*hoveredPart);
            selectedPart->fields.attPos0 = EditorLogic::LocalPosition(*hoveredPart);
            selectedPartOriginalRotation = hoveredPart->partTransform.rotation;
            Effect("part:SetLayerRecursive(0)", selectedPart);
            selectedPart->hasRigidbody = true;
            const AvailablePart* partInfoByName =
                PartLoader::Instance() ? PartLoader::Instance()->getPartInfoByName(selectedPart->partInfo->name) : nullptr;
            if (partInfoByName != nullptr) selectedPart->fields.mass = partInfoByName->partPrefab->fields.mass;
            FetchColliders();
            ToggleCollidersTrigger(true);
            PlayAudioClip("partGrabClip");
            Effect("icon:CreateSelectedPartIcon", selectedPart);
            GE::onEditorPartEvent.Fire(ConstructionEventType::PartPicked, hoveredPart);
        }
    }
    if (EVAConstructionEnvironment::interruptWeld) EVAConstructionEnvironment::interruptWeld();
}

// EVAConstructionModeEditor.cs:566
void EVAConstructionModeEditor::RemoveAttachNodes() {
    for (size_t i = 0; i < attachVessels.size(); i++) {
        handleChildNodeIcons(Need(attachVessels[i]->rootPart));
        handleAttachNodeIcons(Need(attachVessels[i]->rootPart), false, false, false);
    }
    if (selectedPart != nullptr) handleAttachNodeIcons(selectedPart, false, false, false);
}

// EVAConstructionModeEditor.cs:579
void EVAConstructionModeEditor::ProcessAttachNodes() {
    const Vector3f b = ActiveWorldPos();
    UpdateVesselsInConstructionRange();
    for (size_t i = 0; i < attachVessels.size(); i++) {
        if (!(attachVessels[i] != nullptr)) continue;
        Vessel* vessel = attachVessels[i];
        for (size_t j = 0, count = vessel->parts.size(); j < count; j++) {
            const float num = Vector3f::Distance(vessel->parts[j]->partTransform.position, b);
            const AttachRules r = RulesOf(*selectedPart);
            if (InventoryEnvironment::EVA_CONSTRUCTION_RANGE > num) handleAttachNodeIcons(vessel->parts[j], r.stack, r.srfAttach, r.allowDock);
            else handleAttachNodeIcons(vessel->parts[j], false, false, false);
        }
    }
    const AttachRules r = RulesOf(*selectedPart);
    handleAttachNodeIcons(selectedPart, r.stack, r.srfAttach, r.allowDock);
    handleChildNodeIcons(selectedPart);
}

// EVAConstructionModeEditor.cs:608
void EVAConstructionModeEditor::UpdateVesselsInConstructionRange() {
    const Vector3f b = ActiveWorldPos();
    for (size_t i = 0; i < FlightRegistry::VesselsLoaded.size(); i++) {
        Vessel* vl = FlightRegistry::VesselsLoaded[i];
        const float num = f32store(fx(Vector3f::Distance(vl->vesselTransform.position, b)) - fx(vl->vesselSize.magnitude()));
        if (!Contains(attachVessels, vl)) {
            if (InventoryEnvironment::EVA_CONSTRUCTION_RANGE > num) attachVessels.push_back(vl);
        } else if (InventoryEnvironment::EVA_CONSTRUCTION_RANGE <= num) {
            handleChildNodeIcons(Need(vl->rootPart));
            handleAttachNodeIcons(Need(vl->rootPart), false, false, false);
            attachVessels.erase(std::find(attachVessels.begin(), attachVessels.end(), vl));
        }
    }
    int count = static_cast<int>(attachVessels.size());
    while (count-- > 0) {
        Vessel* v = attachVessels[static_cast<size_t>(count)];
        if (v != nullptr && !v->loaded) {
            // sic: the C# indexes FlightGlobals.VesselsLoaded[count] here (not attachVessels[count])
            Vessel* bad = static_cast<size_t>(count) < FlightRegistry::VesselsLoaded.size() ? FlightRegistry::VesselsLoaded[static_cast<size_t>(count)] : nullptr;
            if (bad == nullptr) throw std::out_of_range("Index was out of range.");
            handleChildNodeIcons(Need(bad->rootPart));
            handleAttachNodeIcons(Need(bad->rootPart), false, false, false);
            attachVessels.erase(attachVessels.begin() + count);
        } else if (v == nullptr) {
            attachVessels.erase(attachVessels.begin() + count);
        }
    }
}

// EVAConstructionModeEditor.cs:644
void EVAConstructionModeEditor::OnVesselUnloaded(Vessel* vessel) {
    handleChildNodeIcons(Need(vessel->rootPart));
    handleAttachNodeIcons(Need(vessel->rootPart), false, false, false);
}

// EVAConstructionModeEditor.cs:650
void EVAConstructionModeEditor::AttachInput() {
    if (!EVAConstructionEnvironment::mouseButtonUp0 || !(selectedPart != nullptr) || attachmentNull || !attachmentPossible ||
        EVAConstructionEnvironment::pointerOverUI || !CanPartBeEdited(*selectedPart, true))
        return;
    Part* data = AttachPart(selectedPart, attachment);
    ToggleCollidersTrigger(false);
    attachSuccesful = true;
    GE::onEditorPartEvent.Fire(ConstructionEventType::PartAttached, selectedPart);
    GE::OnEVAConstructionModePartAttached.Fire(attachment.potentialParent->vessel, data);
    lastAttachedPart = data;
    selectedPart = nullptr;
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (inv != nullptr) {
        if (inv->currentInventory != nullptr && inv->currentInventorySlotClicked != nullptr) inv->currentInventorySlotClicked->UpdateCurrentSelectedSlot(false);
        inv->SetCurrentCargoPart(nullptr);
        inv->ResetInventoryCacheValues(false);
    }
    DestroyBackupPart();
}

// EVAConstructionModeEditor.cs:675
void EVAConstructionModeEditor::partRotationResetUpdate() {
    // GameSettings.Editor_resetRotation.GetKeyDown() && selectedPart.transform == selectedPart.GetReferenceTransform()
    if (resetRotationKeyDown && selectedPartIsReferenceTransform) {
        srfAttachCursorOffset = Vector3f::zero();
        selectedPart->fields.attRotation = Quaternionf::identity();
        EditorLogic::SetLocalRotation(*selectedPart, selectedPart->fields.attRotation0);  // transform.localRotation = attRotation0
        partTweaked = true;
    }
}

// EVAConstructionModeEditor.cs:686
Part* EVAConstructionModeEditor::AttachPart(Part* part, Attachment& attach) {
    if (part == nullptr || attach.potentialParent == nullptr) return nullptr;
    auto* cargo = dynamic_cast<ModuleCargoPart*>(part->FindModuleOfClass("ModuleCargoPart"));
    cargo->Set<bool>("beingAttached", true);
    SetModuleCargoPartValue(part, "beingAttached", "True");
    std::unique_ptr<Part> created = part->protoPartSnapshot->CreatePart();
    Part* part2 = AdoptPart(std::move(created));
    part2->set_isAttached(true);
    part2->partInfo = part->partInfo;
    int32_t value = 0;
    if (part->protoPartSnapshot->partInfo->partConfig && part->protoPartSnapshot->partInfo->partConfig->TryGetValue("PhysicsSignificance", value))
        part2->fields.physicalSignificance = value;
    part2->partTransform.rotation = part->partTransform.rotation;
    part2->partTransform.position = part->partTransform.position;
    ReleaseOwned(part);  // Object.Destroy(part.gameObject)
    if (UIPartActionControllerInventory::Instance != nullptr) UIPartActionControllerInventory::Instance->DestroyHeldPartAsIcon();
    if (attach.callerPartNode != nullptr) {
        if (attach.callerPartNode->owner->fields.persistentId == part->fields.persistentId) {
            AttachNode* attachNode = part2->FindAttachNode(attach.callerPartNode->id);
            if (attachNode != nullptr) {
                attachNode->attachedPart = attach.potentialParent;
            } else if (attach.callerPartNode->id == part2->srfAttachNode->id) {
                part2->srfAttachNode->attachedPart = attach.potentialParent;
                part2->srfAttachNode->srfAttachMeshName = attach.callerPartNode->srfAttachMeshName;
            }
        } else {
            attach.callerPartNode->attachedPart = attach.potentialParent;
        }
    }
    if (attach.otherPartNode != nullptr) attach.otherPartNode->attachedPart = part2;
    part2->fields.attPos0 = EditorLogic::LocalPosition(*part);      // part.transform.localPosition
    part2->fields.attRotation0 = EditorLogic::LocalRotation(*part);  // part.transform.localRotation
    Effect("part:OnAttachFlight", part2);
    if (attach.mode == AttachModes::SRF_ATTACH) {
        part2->fields.attachMode = static_cast<int32_t>(AttachModes::SRF_ATTACH);
        part2->srfAttachNode->attachedPart = attach.potentialParent;
    }
    Effect("highlight:default:AlwaysOn", part2);
    PlayAudioClip("attachClip");
    if (EVAConstructionEnvironment::weld) EVAConstructionEnvironment::weld(*part2);
    if (part2->isCompund()) selectedCompoundPart = part2;
    return part2;
}

// EVAConstructionModeEditor.cs:756
void EVAConstructionModeEditor::SetModuleCargoPartValue(Part* part, const std::string& name, const std::string& value) {
    PartModule* moduleCargoPart = part->FindModuleOfClass("ModuleCargoPart");
    if (moduleCargoPart != nullptr) {
        ProtoPartModuleSnapshot* m = part->protoPartSnapshot->FindModule(moduleCargoPart->moduleName);
        if (m != nullptr) m->moduleValues->SetValue(name, value, true);
    }
}

// EVAConstructionModeEditor.cs:765
void EVAConstructionModeEditor::DetachInput() {
    if (selectedPart != nullptr) return;
    Part* hoveredPart = EVAConstructionEnvironment::hoveredPart;
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (!attachSuccesful && EVAConstructionEnvironment::mouseButtonUp0 && hoveredPart != nullptr &&
        hoveredPart->FindModuleOfClass("ModuleCargoPart") != nullptr &&
        Vector3f::Distance(hoveredPart->partTransform.position, ActiveWorldPos()) <
            InventoryEnvironment::EVA_CONSTRUCTION_RANGE &&
        hoveredPart->vessel != nullptr && hoveredPart->vessel->rootPart->fields.persistentId != hoveredPart->fields.persistentId && inv != nullptr &&
        !inv->IsCursorOverAnyPAWOrCargoPane && !EVAConstructionModeController::Instance->Hover() && !EVAConstructionEnvironment::pointerOverUI &&
        CanPartBeEdited(*hoveredPart, false)) {
        Vessel* vessel = hoveredPart->vessel;
        DetachPart(hoveredPart);
        PlayAudioClip("partGrabClip");
        GE::onEditorPartEvent.Fire(ConstructionEventType::PartDetached, hoveredPart);
        GE::OnEVAConstructionModePartDetached.Fire(vessel, hoveredPart);
    }
}

// EVAConstructionModeEditor.cs:781
void EVAConstructionModeEditor::DetachPart(Part* part) {
    Part* parent = part->parent;
    const Vector3f localPosition = EditorLogic::LocalPosition(*part);
    selectedPartOriginalRotation = part->partTransform.rotation;
    BackupPart(part, parent);
    Effect("part:OnDetachFlight", part);
    part->fields.attRotation = Quaternionf::identity();
    part->fields.attRotation0 = EditorLogic::LocalRotation(*part);
    part->fields.attPos0 = localPosition;
    Effect("part:RefreshHighlighter", part);
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (inv != nullptr) {
        inv->SetCurrentCargoPart(part);
        inv->isSetAsPart = true;
    }
    isCurrentPartFlag = selectedPart != nullptr && selectedPart->FindModuleOfClass("FlagDecalBackground") != nullptr;
    Effect("icon:CreateSelectedPartIcon", part);
    FetchColliders();
    ToggleCollidersTrigger(true);
}

// EVAConstructionModeEditor.cs:803
void EVAConstructionModeEditor::CopyActionGroups(Part* sPart, Part* cPart) {
    if (sPart == nullptr || cPart == nullptr || sPart == cPart) return;
    int count = static_cast<int>(cPart->Modules().Count());
    while (count-- > 0) {
        PartModule* sm = sPart->Modules()[static_cast<size_t>(count)];
        PartModule* cm = cPart->Modules()[static_cast<size_t>(count)];
        if (sm->Actions().Count() == cm->Actions().Count()) {
            for (size_t i = 0; i < cm->Actions().Count(); i++) cm->Actions()[i]->actionGroup = sm->Actions()[i]->actionGroup;
        }
    }
    int count2 = static_cast<int>(sPart->children.size());
    while (count2-- > 0) CopyActionGroups(sPart->children[static_cast<size_t>(count2)], cPart->children[static_cast<size_t>(count2)]);
}

// EVAConstructionModeEditor.cs:827
void EVAConstructionModeEditor::OnPartDropped() {
    if (selectedPart != nullptr && !selectedPart->isAttached() && UIPartActionControllerInventory::Instance != nullptr)
        UIPartActionControllerInventory::Instance->ReturnHeldPart();
}

// EVAConstructionModeEditor.cs:835
void EVAConstructionModeEditor::BackupPart(Part* part, Part* partParent, bool droppedPart) {
    if (part == nullptr || (partParent == nullptr && !droppedPart)) return;
    if (backedUpPart != nullptr) DestroyBackupPart();
    backedUpPartOwner = Part::Instantiate(*part);
    backedUpPart = backedUpPartOwner.get();
    if (partParent != nullptr) backedUpPart->parent = partParent;
    if (!droppedPart) {
        AttachNode* n = part->FindAttachNodeByPart(partParent);
        backedUpAttachment = *n;  // AttachNode.Clone
        hasBackedUpAttachment = true;
        backedUpParentAttachment = partParent->FindAttachNodeByPart(part);
    }
    backedUpPart->fields.attachMode = part->fields.attachMode;
    {
        std::string& nm = backedUpPart->gameObjectName;
        const size_t at = nm.find("(Clone)");
        if (at != std::string::npos) nm.replace(at, 7, "(Backup)");
    }
    if (part->protoPartSnapshot == nullptr) {
        ownedSnapshots.push_back(std::make_unique<ProtoPartSnapshot>(*part, part->vessel->protoVessel));
        backedUpPart->protoPartSnapshot = ownedSnapshots.back().get();
    } else {
        backedUpPart->protoPartSnapshot = part->protoPartSnapshot;
    }
    ownedSnapshots.push_back(std::make_unique<ProtoPartSnapshot>(*part, part->vessel->protoVessel));
    backedUpPart->protoPartSnapshot = ownedSnapshots.back().get();
    backedUpPart->partTransform.position = part->partTransform.position;
    backedUpPart->partTransform.rotation = part->partTransform.rotation;
}

// EVAConstructionModeEditor.cs:881
void EVAConstructionModeEditor::RestoreLastAttachedPart() {
    if (!(backedUpPart != nullptr)) return;
    if (backedUpPart->parent == nullptr) {
        std::string& nm = backedUpPart->gameObjectName;
        const size_t at = nm.find("(Backup)");
        if (at != std::string::npos) nm.erase(at, 8);
        if (selectedPart == nullptr) selectedPart = backedUpPart;
        RestoreLastDroppedPart();
        return;
    }
    {
        std::string& nm = backedUpPart->gameObjectName;
        const size_t at = nm.find("(Backup)");
        if (at != std::string::npos) nm.erase(at, 8);
    }
    backedUpPart->set_isAttached(true);
    int32_t value = 0;
    if (backedUpPart->protoPartSnapshot->partInfo->partConfig && backedUpPart->protoPartSnapshot->partInfo->partConfig->TryGetValue("PhysicsSignificance", value))
        backedUpPart->fields.physicalSignificance = value;
    backedUpPart->FindAttachNode(backedUpAttachment.id);
    if (backedUpParentAttachment != nullptr) backedUpParentAttachment->attachedPart = backedUpPart;
    Effect("part:OnAttachFlight", backedUpPart);
    Effect("part:CreateAttachJoint", backedUpPart);
    GE::onEditorPartEvent.Fire(ConstructionEventType::PartAttached, backedUpPart);
    GE::OnEVAConstructionModePartAttached.Fire(backedUpPart->vessel, backedUpPart);
    lastAttachedPart = backedUpPart;
    Effect("highlight:default:OnMouseOver:off", backedUpPart);
    PlayAudioClip("attachClip");
    backedUpPart = nullptr;
}

// EVAConstructionModeEditor.cs:923
void EVAConstructionModeEditor::RestoreLastDroppedPart() {
    if (backedUpPart != nullptr) {
        const Quaternionf bodyRot = InventoryEnvironment::bodyRotation ? InventoryEnvironment::bodyRotation() : Quaternionf::identity();
        const Quaternionf rotation = UnityNative::Inverse(bodyRot) * backedUpPart->partTransform.rotation;
        DropAttachablePart(backedUpPart->gameObjectName, backedUpPart->partTransform.position, rotation);
        DestroyBackupPart();
    }
}

// EVAConstructionModeEditor.cs:933
Attachment EVAConstructionModeEditor::CheckAttach(Part* selPart) {
    EditorEnvironment& env = EditorEnvironment::Current();
    Attachment attachment;
    attachment.caller = selPart;
    attachment.position = selPart->partTransform.position;
    attachment.rotation = vesselRotation;
    attachment.potentialParent = nullptr;
    bool flag = false;
    bool flag2 = false;
    const AttachRules selRules = RulesOf(*selPart);
    if (selRules.stack && allowNodeAttachment) {
        for (size_t i = 0; i < attachVessels.size(); i++) {
            Vessel* vessel = attachVessels[i];
            for (size_t j = 0, count = vessel->parts.size(); j < count; j++) {
                Part* part = vessel->parts[j];
                const AttachRules rules = RulesOf(*part);
                if (part == selPart || PartInHierarchy(selPart, part) || (!rules.allowStack && !rules.allowDock)) continue;
                AttachNode* attachNode = nullptr;
                AttachNode* attachNode2 = nullptr;
                for (size_t k = 0, count2 = selPart->attachNodes.size(); k < count2; k++) {
                    AttachNode* attachNode3 = &selPart->attachNodes[k];
                    if (attachNode3->attachedPart != nullptr || !HasNodeIcon(attachNode3)) continue;
                    const Vector3f normalized = selPart->partTransform.TransformDirection(attachNode3->orientation).normalized();
                    for (size_t l = 0, count3 = part->attachNodes.size(); l < count3; l++) {
                        AttachNode* attachNode4 = &part->attachNodes[l];
                        if (((!HasNodeIcon(attachNode4) || attachNode4->attachedPart != nullptr) && !env.allowPartClipping) ||
                            attachNode3->nodeType != attachNode4->nodeType ||
                            !(EVAConstructionEnvironment::nodeIconUnderCursor &&
                              EVAConstructionEnvironment::nodeIconUnderCursor(*part, *attachNode4, selPart->partTransform.TransformPoint(attachNode3->position))))
                            continue;
                        const Vector3f normalized2 = part->partTransform.TransformDirection(attachNode4->orientation).normalized();
                        const float num = Vector3f::Dot(normalized, normalized2);
                        if (!env.nonStrictAttachmentOrientation) {
                            if (num < -0.7f) {
                                attachNode2 = attachNode4;
                                attachNode = attachNode3;
                                attachment.potentialParent = part;
                                stackRotation = UnityNative::FromToRotation(normalized, -normalized2);
                                attachment.rotation = stackRotation * vesselRotation;
                                attachment.position = part->partTransform.TransformPoint(attachNode2->position + attachNode2->offset) -
                                                      attachment.rotation * selPart->fields.attRotation * attachNode->position;
                                break;
                            }
                        } else if (!(Mathf::Abs(num) <= 0.7f)) {
                            attachNode2 = attachNode4;
                            attachNode = attachNode3;
                            attachment.potentialParent = part;
                            stackRotation = UnityNative::FromToRotation(normalized * Mathf::Sign(num), normalized2);
                            attachment.rotation = stackRotation * vesselRotation;
                            attachment.position = part->partTransform.TransformPoint(attachNode2->position + attachNode2->offset) -
                                                  attachment.rotation * selPart->fields.attRotation * attachNode->position;
                            break;
                        }
                    }
                }
                if (attachNode != nullptr && attachNode2 != nullptr) {
                    flag = true;
                    attachment.callerPartNode = attachNode;
                    attachment.otherPartNode = attachNode2;
                    selPart->fields.attachMode = static_cast<int32_t>(AttachModes::STACK);
                }
            }
        }
    }
    if ((selRules.srfAttach && allowSrfAttachment && !InventoryEnvironment::MODIFIER_KEY_DOWN) || (InventoryEnvironment::MODIFIER_KEY_DOWN && isCurrentPartFlag)) {
        bool flag3 = false;
        Part* part2 = nullptr;
        EditorSurfaceHit hit;
        if (EVAConstructionEnvironment::surfaceRaycastAll) {
            // Physics.RaycastAll sorted by distance: the first hit whose part exists and is not CARGO
            for (const EditorSurfaceHit& h : EVAConstructionEnvironment::surfaceRaycastAll()) {
                part2 = h.part;
                if (part2 != nullptr && part2->State() != PartStates::CARGO) {
                    flag3 = true;
                    hit = h;
                    break;
                }
            }
        }
        if (flag3 && part2 != nullptr) {
            if (isCurrentPartFlag) {
                FairingHitColliderName = hit.colliderName;
            } else {
                FairingHitColliderName.clear();
                if (hit.fairingPanelCollider) {
                    attachment.possible = false;
                    return attachment;
                }
            }
            if (part2 != nullptr) {
                if (hit.noAttachTag) part2 = nullptr;
                if (part2 != nullptr && RulesOf(*part2).allowSrfAttach) {
                    attachment.callerPartNode = selPart->srfAttachNode.get();
                    attachment.otherPartNode = nullptr;
                    selPart->fields.attachMode = static_cast<int32_t>(AttachModes::SRF_ATTACH);
                    selPart->srfAttachNode->srfAttachMeshName = hit.colliderName;
                    attachment.potentialParent = part2;
                    flag2 = true;
                    attachment.rotation = UnityNative::LookRotation(hit.normal, part2->partTransform.rotation * Vector3f::up()) *
                                          UnityNative::LookRotation(selPart->srfAttachNode->orientation, Vector3f::up());
                    attachment.position = hit.point - attachment.rotation * selPart->fields.attRotation * selPart->srfAttachNode->position;
                    if (InventoryEnvironment::VAB_USE_ANGLE_SNAP) {
                        Vector3f vector = hit.point - part2->partTransform.position;
                        const Vector3f rayOrigin = part2->partTransform.position;
                        const Vector3f rayDir = Vector3f::Normalize(part2->partTransform.up());
                        const Vector3f vector2 = Vector3f::ProjectOnPlane(vector, rayDir);
                        const Vector3f forward = part2->partTransform.forward();
                        const float num2 = Vector3f::Dot(vector, rayDir);
                        const float num3 = KSPUtil::BearingDegrees(forward, vector2.normalized(), rayDir);
                        const float num4 = env.fineTweak ? srfAttachAngleSnapFine : srfAttachAngleSnap;
                        const float q = f32store(static_cast<double>(num3) / static_cast<double>(num4));
                        float angle;
                        if (!(static_cast<double>(q) - static_cast<double>(Mathf::Floor(q)) > 0.5))
                            angle = f32store(static_cast<double>(Mathf::FloorToInt(q)) * static_cast<double>(num4));
                        else
                            angle = f32store(static_cast<double>(Mathf::CeilToInt(q)) * static_cast<double>(num4));
                        const Vector3f vector3 = UnityNative::AngleAxis(angle, rayDir) * forward;
                        Vector3f vector4 = rayOrigin + rayDir * num2;
                        vector = Vector3f::Normalize(vector);
                        Vector3f srfNormal{};
                        vector4 += vector3 * FindPartSurface(*part2, rayOrigin, rayDir, vector, hit, srfNormal);
                        if (Mathf::Abs(Vector3f::Dot(srfNormal, rayDir)) < 1.0f)
                            srfNormal = UnityNative::FromToRotation(Vector3f::ProjectOnPlane(srfNormal, rayDir).normalized(), vector3) * srfNormal;
                        attachment.rotation = UnityNative::LookRotation(srfNormal, part2->partTransform.rotation * Vector3f::up()) *
                                              UnityNative::LookRotation(selPart->srfAttachNode->orientation, Vector3f::up());
                        attachment.position = vector4 - attachment.rotation * selPart->fields.attRotation * selPart->srfAttachNode->position;
                    }
                }
            }
        }
    }
    attachment.mode = static_cast<AttachModes>(selPart->fields.attachMode);
    attachment.possible = (flag2 || flag) && !attachment.collision;
    return attachment;
}

// EVAConstructionModeEditor.cs:1115
void EVAConstructionModeEditor::ForceDrop() {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (inv != nullptr && inv->CurrentCargoPart() != nullptr && !inv->isSetAsPart) {
        if (backedUpPart != nullptr) RestoreLastDroppedPart();
    } else if (evaConstructionMode == ConstructionMode::Place && selectedPart != nullptr && CanPartBeEdited(*selectedPart, true)) {
        const Quaternionf bodyRot = InventoryEnvironment::bodyRotation ? InventoryEnvironment::bodyRotation() : Quaternionf::identity();
        DropAttachablePart(selectedPart->gameObjectName, selectedPart->partTransform.position, UnityNative::Inverse(bodyRot) * selectedPart->partTransform.rotation);
    }
}

// EVAConstructionModeEditor.cs:1130
void EVAConstructionModeEditor::DropPartInput() {
    if (EVAConstructionEnvironment::mouseButtonUp0 && isPlacementValid_ && selectedPart != nullptr && FlightRegistry::ActiveVessel != nullptr &&
        !EVAConstructionEnvironment::pointerOverUI && CanPartBeEdited(*selectedPart, true)) {
        const Quaternionf bodyRot = InventoryEnvironment::bodyRotation ? InventoryEnvironment::bodyRotation() : Quaternionf::identity();
        DropAttachablePart(selectedPart->gameObjectName, selectedPart->partTransform.position, UnityNative::Inverse(bodyRot) * selectedPart->partTransform.rotation);
    }
}

// EVAConstructionModeEditor.cs:1138
void EVAConstructionModeEditor::DropAttachablePart(const std::string& partName, const Vector3f& partPosition, const Quaternionf& rotation) {
    ConfigNode::NodePtr protoVesselNode = GetProtoVesselNode(partName, partPosition, rotation, *FlightRegistry::ActiveVessel, *selectedPart);
    ProtoVessel* protoVessel = EVAConstructionEnvironment::addVessel ? EVAConstructionEnvironment::addVessel(*protoVesselNode) : nullptr;
    for (size_t i = 0; i < FlightRegistry::VesselsUnloaded.size(); i++) {
        if (protoVessel != nullptr && protoVessel->persistentId == FlightRegistry::VesselsUnloaded[i]->persistentId) {
            if (EVAConstructionEnvironment::setPhysicsHoldExpiryOverride) EVAConstructionEnvironment::setPhysicsHoldExpiryOverride(protoVessel);
            break;
        }
    }
    const bool serenity = ModuleCargoPart::isExpansionInstalled ? ModuleCargoPart::isExpansionInstalled("Serenity") : true;
    if (serenity && selectedPart->FindModuleOfClass("ModuleGroundSciencePart") != nullptr)
        PostMessage(Localizer::Format("#autoLOC_6002644", {selectedPart->partInfo->title}));
    GE::onEditorPartEvent.Fire(ConstructionEventType::PartDropped, selectedPart);
    DestroyHeldPart();
    selectedPartColliders.clear();
    PlayAudioClip("attachClip");
    DestroyBackupPart();
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (inv != nullptr) {
        if (inv->currentInventorySlotClicked != nullptr) inv->currentInventorySlotClicked->UpdateCurrentSelectedSlot(false);
        inv->ResetInventoryCacheValues();
    }
}

// EVAConstructionModeEditor.cs:1169
ConfigNode::NodePtr EVAConstructionModeEditor::GetProtoVesselNode(const std::string& partName, const Vector3f& partPosition, const Quaternionf& rotation,
                                                                  Vessel& vessel, Part& part) {
    const uint32_t uniqueFlightID = EVAConstructionEnvironment::uniqueFlightID ? EVAConstructionEnvironment::uniqueFlightID() : 0u;
    Orbit orbit;
    Vector3d vector3d = Vector3d(Vector3f::zero());
    const Vessel::Situations situation = vessel.situation;
    switch (situation) {
        case Vessel::Situations::LANDED:
        case Vessel::Situations::PRELAUNCH: orbit = Orbit::CreateRandomOrbitAround(vessel.mainBody()); break;
        case Vessel::Situations::FLYING:
        case Vessel::Situations::SUB_ORBITAL:
        case Vessel::Situations::ORBITING:
        case Vessel::Situations::ESCAPING: {
            orbit = EVAConstructionEnvironment::currentOrbit ? EVAConstructionEnvironment::currentOrbit(vessel) : Orbit();
            const Vector3d pos = EVAConstructionEnvironment::orbitalPos ? EVAConstructionEnvironment::orbitalPos(vessel) : Vector3d(Vector3f::zero());
            vector3d = pos + Vector3d(partPosition.x, partPosition.z, partPosition.y);
            orbit.UpdateFromStateVectors(vector3d, orbit.vel, orbit.referenceBody, InventoryEnvironment::UT());
            orbit.Init();
            orbit.UpdateFromUT(InventoryEnvironment::UT());
            break;
        }
        default: break;
    }
    const AvailablePart* partInfo = part.partInfo;
    std::string vesselName = partName;
    if (partInfo != nullptr) vesselName = partInfo->partPrefab->partInfo->title;
    ConfigNode::NodePtr configNode =
        ProtoVesselNodes::CreateVesselNode(vesselName, VesselType::DroppedPart, orbit, 0, {CreatePartNode(uniqueFlightID, part, partInfo)}, FlightRegistry::Bodies());
    double alt = vessel.altitude;
    double lat = vessel.latitude;
    double lon = vessel.longitude;
    configNode->SetValue("skipGroundPositioning", false);
    switch (situation) {
        case Vessel::Situations::LANDED:
        case Vessel::Situations::PRELAUNCH:
            vessel.mainBody()->GetLatLonAlt(Vector3d(partPosition), lat, lon, alt);
            if ((situation == Vessel::Situations::LANDED || situation == Vessel::Situations::PRELAUNCH) && isPlacementValid_ && !isPlacementOnGround) {
                configNode->SetValue("sit", std::string("SUB_ORBITAL"));
                configNode->SetValue("landed", true);
                configNode->SetValue("skipGroundPositioning", true);
                configNode->SetValue("skipGroundPositioningForDroppedPart", true, true);
            } else {
                configNode->SetValue("sit", std::string("LANDED"));
                configNode->SetValue("landed", true);
                configNode->SetValue("skipGroundPositioning", false);
            }
            break;
        case Vessel::Situations::FLYING:
        case Vessel::Situations::SUB_ORBITAL:
        case Vessel::Situations::ORBITING:
        case Vessel::Situations::ESCAPING:
            vessel.mainBody()->GetLatLonAltOrbital(vector3d, lat, lon, alt);
            configNode->SetValue("sit", std::string("ORBITING"));
            configNode->SetValue("landed", false);
            configNode->SetValue("skipGroundPositioning", true);
            break;
        default: break;
    }
    configNode->SetValue("lat", lat);
    configNode->SetValue("lon", lon);
    configNode->SetValue("alt", alt);
    configNode->SetValue("splashed", false);
    configNode->SetValue("vesselSpawning", true);
    configNode->AddValue("prst", true);
    configNode->AddValue("rot", rotation);
    configNode->SetValue("PQSMin", 0, true);
    configNode->SetValue("PQSMax", 0, true);
    return configNode;
}

// EVAConstructionModeEditor.cs:1245
ConfigNode::NodePtr EVAConstructionModeEditor::CreatePartNode(uint32_t id, Part& part, const AvailablePart*) {
    ConfigNode::NodePtr configNode = ProtoVesselNodes::CreatePartNode(&part, part.partInfo->name, id, {});
    configNode->SetValue("flag", part.fields.flagURL.value_or(""), true);
    for (size_t i = 0; i < part.Modules().Count(); i++) {
        auto* moduleCargoPart = dynamic_cast<ModuleCargoPart*>(part.Modules()[i]);
        if (moduleCargoPart != nullptr) {
            Vessel* active = FlightRegistry::ActiveVessel;
            moduleCargoPart->Set<bool>("beingSettled", active != nullptr && (active->situation == Vessel::Situations::LANDED || active->situation == Vessel::Situations::PRELAUNCH));
            moduleCargoPart->Save(*configNode->AddNode("MODULE"));
        } else {
            part.Modules()[i]->Save(*configNode->AddNode("MODULE"));
        }
    }
    return configNode;
}

// EVAConstructionModeEditor.cs:1265
void EVAConstructionModeEditor::UpdateAssistingKerbals() {
    const double num = combinedConstructionWeightLimit;
    Vessel* active = FlightRegistry::ActiveVessel;
    if (InventoryEnvironment::EVA_CONSTRUCTION_COMBINE_ENABLED && !(active == nullptr)) {
        int num2 = 0;
        const Vector3f position = active->vesselTransform.position;
        const std::vector<Vessel*>& vesselsLoaded = FlightRegistry::VesselsLoaded;
        for (size_t i = 0; i < vesselsLoaded.size(); i++) {
            Vessel* vessel = vesselsLoaded[i];
            if (vessel->isEVA() && vessel != active && vessel->evaController != nullptr &&
                Vector3f::Distance(vessel->vesselTransform.position, position) <= InventoryEnvironment::EVA_CONSTRUCTION_COMBINE_RANGE &&
                !(EVAConstructionEnvironment::isSeated ? EVAConstructionEnvironment::isSeated(*vessel) : false)) {
                if (InventoryEnvironment::EVA_CONSTRUCTION_COMBINE_NONENGINEERS) {
                    num2++;
                } else if ((EVAConstructionEnvironment::engineerCount ? EVAConstructionEnvironment::engineerCount(*vessel) : 0) > 0) {
                    num2++;
                }
            }
        }
        assistingKerbals = num2;
        combinedConstructionWeightLimit = PhysicsGlobals::ConstructionWeightLimit() + static_cast<double>(num2) * PhysicsGlobals::ConstructionWeightLimitPerKerbalCombine();
        if (num != combinedConstructionWeightLimit) {
            GE::OnCombinedConstructionWeightLimitChanged.Fire();
            if (num > combinedConstructionWeightLimit && SelectedPart() != nullptr && !EVAConstructionUtil::IsUnderConstructionWeightLimit(*SelectedPart())) {
                PostMessage(Localizer::Format("#autoLOC_8012062", {SelectedPart()->partInfo->title}));
                const Quaternionf bodyRot = InventoryEnvironment::bodyRotation ? InventoryEnvironment::bodyRotation() : Quaternionf::identity();
                DropAttachablePart(selectedPart->gameObjectName, selectedPart->partTransform.position,
                                   UnityNative::Inverse(bodyRot) * selectedPart->partTransform.rotation);
            }
        }
    } else {
        assistingKerbals = 0;
        combinedConstructionWeightLimit = 0.0;
        if (num != combinedConstructionWeightLimit) GE::OnCombinedConstructionWeightLimitChanged.Fire();
    }
}

// EVAConstructionModeEditor.cs:1311
void EVAConstructionModeEditor::ToggleRotateGizmo(bool activate) {
    if (activate) {
        if (!(selectedPart == nullptr) && selectedPart->PartCanBeRotated()) {
            gizmoRotateActive = true;  // GizmoRotate.Attach (a previous gizmo is Detach'ed first)
            gizmoAttRotate = selectedPart->fields.attRotation;
            gizmoAttRotate0 = selectedPart->fields.attRotation0;
            PlayAudioClip("tweakGrabClip");
            FetchColliders();
            ToggleCollidersTrigger(true);
        } else {
            CannotInteract();
        }
    } else if (gizmoRotateActive) {
        gizmoRotateActive = false;  // gizmoRotate.Detach()
    }
}

// EVAConstructionModeEditor.cs:1342 (gizmoRotation = gizmoRotate.transform.rotation, hostRot0 = gizmoRotate.HostRot0)
void EVAConstructionModeEditor::OnRotateGizmoUpdate(const Quaternionf& dRot, const Quaternionf& gizmoRotation, const Quaternionf& hostRot0) {
    switch (gizmoRotateSpace) {
        case UnityEngine::Space::Self: selectedPart->partTransform.rotation = gizmoRotation * selectedPart->fields.initRotation; break;
        case UnityEngine::Space::World: selectedPart->partTransform.rotation = dRot * hostRot0; break;
    }
    selectedPart->fields.attRotation0 = EditorLogic::LocalRotation(*selectedPart);
    selectedPart->fields.attRotation = gizmoAttRotate * UnityNative::Inverse(gizmoAttRotate0) * EditorLogic::LocalRotation(*selectedPart);
    GE::onEditorPartEvent.Fire(ConstructionEventType::PartRotating, selectedPart);
}

// EVAConstructionModeEditor.cs:1359
void EVAConstructionModeEditor::OnRotateGizmoUpdated(const Quaternionf&) {
    if (selectedPart != nullptr) {
        if (selectedPart->vessel != nullptr) selectedPart->UpdateOrgPosAndRot(selectedPart->vessel->rootPart);
        GE::onEditorPartEvent.Fire(ConstructionEventType::PartRotated, selectedPart);
    }
}

// EVAConstructionModeEditor.cs:1376
void EVAConstructionModeEditor::ToggleOffsetGizmo(bool activate) {
    if (activate) {
        if (!(selectedPart == nullptr) && selectedPart->PartCanBeOffset()) {
            gizmoOffsetActive = true;
            PlayAudioClip("tweakGrabClip");
            FetchColliders();
            ToggleCollidersTrigger(true);
        } else {
            CannotInteract();
        }
    } else if (gizmoOffsetActive) {
        gizmoOffsetActive = false;
    }
}

// EVAConstructionModeEditor.cs:1405 (gizmoPosition = gizmoOffset.transform.position, updated in place like the gizmo object)
void EVAConstructionModeEditor::OnOffsetGizmoUpdate(const Vector3f&, Vector3f& gizmoPosition) {
    PartTransform& transform = selectedPart->partTransform;
    transform.position = gizmoPosition;
    EditorEnvironment& env = EditorEnvironment::Current();
    if (rootPart != nullptr && selectedPart != rootPart) {
        threshold = env.fineTweak ? InventoryEnvironment::VAB_FINE_OFFSET_THRESHOLD : 0.1f;
        if (env.setSurfaceAttachActive) env.setSurfaceAttachActive(*selectedPart, true);
        Part* referenceParent = env.referenceParent ? env.referenceParent(*selectedPart) : selectedPart->parent;
        if (referenceParent != nullptr && EditorGeometryUtil::TestPartBoundsSeparate(*selectedPart, *referenceParent, threshold, offsetGap)) {
            if (selectedPart->fields.attachMode == static_cast<int32_t>(AttachModes::STACK)) {
                AttachNode* childToParent = selectedPart->FindAttachNodeByPart(selectedPart->parent);
                AttachNode* parentToChild = selectedPart->parent->FindAttachNodeByPart(selectedPart);
                const Vector3f local = EditorLogic::LocalPosition(*selectedPart);
                diff = parentToChild->position - (local + selectedPart->fields.attRotation * childToParent->position);
                if (fx(diff.sqrMagnitude()) > fx(threshold) * fx(static_cast<float>(parentToChild->size + 1))) {
                    gizmoPosition -= offsetGap;
                    transform.position = gizmoPosition;
                }
            } else {
                transform.position = gizmoPosition;
            }
        }
        if (env.setSurfaceAttachActive) env.setSurfaceAttachActive(*selectedPart, false);
    }
    selectedPart->fields.attPos = transform.position - selectedPart->fields.attPos0;
    GE::onEditorPartEvent.Fire(ConstructionEventType::PartOffsetting, selectedPart);
}

// EVAConstructionModeEditor.cs:1444
void EVAConstructionModeEditor::OnOffsetGizmoUpdated(const Vector3f&) {
    if (selectedPart != nullptr) {
        if (selectedPart->vessel != nullptr) selectedPart->UpdateOrgPosAndRot(selectedPart->vessel->rootPart);
        GE::onEditorPartEvent.Fire(ConstructionEventType::PartOffset, selectedPart);
    }
}

// EVAConstructionModeEditor.cs:1456
void EVAConstructionModeEditor::CoordToggleInput(bool keyUp) {
    if (keyUp) ChangeCoordSpace();
}

// EVAConstructionModeEditor.cs:1464
void EVAConstructionModeEditor::ToggleCoordSpaceButton(bool toggle) {
    if (toggle) toggle = selectedPart != nullptr;
    coordSpaceButtonActive = toggle;
}

// EVAConstructionModeEditor.cs:1476
void EVAConstructionModeEditor::ChangeCoordSpace() {
    if (gizmoOffsetActive) {
        switch (gizmoOffsetSpace) {
            case UnityEngine::Space::Self:
                gizmoOffsetSpace = UnityEngine::Space::World;
                coordSpaceText = "#autoLOC_6001217";
                PostMessage("#autoLOC_6001221");
                break;
            case UnityEngine::Space::World:
                gizmoOffsetSpace = UnityEngine::Space::Self;
                coordSpaceText = "#autoLOC_6001218";
                PostMessage("#autoLOC_6001222");
                break;
        }
    } else if (gizmoRotateActive) {
        switch (gizmoRotateSpace) {
            case UnityEngine::Space::Self:
                gizmoRotateSpace = UnityEngine::Space::World;
                coordSpaceText = "#autoLOC_6001217";
                PostMessage("#autoLOC_6001223");
                break;
            case UnityEngine::Space::World:
                gizmoRotateSpace = UnityEngine::Space::Self;
                coordSpaceText = "#autoLOC_6001218";
                PostMessage("#autoLOC_6001224");
                break;
        }
    }
}

// EVAConstructionModeEditor.cs:1512
void EVAConstructionModeEditor::SnapInputUpdate(bool keyDown) {
    if (keyDown) SnapButton();
}

// EVAConstructionModeEditor.cs:1520
void EVAConstructionModeEditor::SnapButton() {
    InventoryEnvironment::VAB_USE_ANGLE_SNAP = !InventoryEnvironment::VAB_USE_ANGLE_SNAP;
    angleSnapSpriteState = InventoryEnvironment::VAB_USE_ANGLE_SNAP;
    GE::onEditorSnapModeChange.Fire(InventoryEnvironment::VAB_USE_ANGLE_SNAP);
}

// EVAConstructionModeEditor.cs:1528
void EVAConstructionModeEditor::PutPartBackInInventory() {
    if (selectedPart != nullptr && UIPartActionControllerInventory::Instance != nullptr) UIPartActionControllerInventory::Instance->ReturnHeldPart();
}

// EVAConstructionModeEditor.cs:1536
void EVAConstructionModeEditor::SelectPartInput() {
    Part* hovered = EVAConstructionEnvironment::hoveredPart;
    if (!EVAConstructionEnvironment::mouseButtonUp0 || !(hovered != nullptr) ||
        (!(selectedPart == nullptr) && selectedPart->fields.persistentId == hovered->fields.persistentId) ||
        hovered->FindModuleOfClass("ModuleCargoPart") == nullptr || !CanPartBeEdited(*hovered, false))
        return;
    if (selectedPart != nullptr) {
        if (selectedPart->fields.previousPhysicalSignificance != selectedPart->fields.physicalSignificance && InventoryEnvironment::scheduler)
            InventoryEnvironment::scheduler->StartCoroutine(ResetToPhysicalPart(selectedPart), 0x7ffffff0);
        if (selectedPart->fields.persistentId != hovered->fields.persistentId) ToggleCollidersTrigger(false);
    }
    selectedPart = hovered;
    GetRootPart();
    OnCurrentMousePartChanged(selectedPart);
    BreakSymmetry(selectedPart);
    if (evaConstructionMode == ConstructionMode::Move || evaConstructionMode == ConstructionMode::Rotate) selectedPart->DemoteToPhysicslessPart();
}

// EVAConstructionModeEditor.cs:1564
void EVAConstructionModeEditor::GetRootPart() {
    if (!(selectedPart == nullptr) && !(selectedPart->vessel == nullptr) && !(selectedPart->vessel->rootPart == nullptr)) rootPart = selectedPart->vessel->rootPart;
}

// EVAConstructionModeEditor.cs:1572 (one SelectedPartCollider per collider of the part: the shell reports each collider's isTrigger)
void EVAConstructionModeEditor::FetchColliders(const std::vector<bool>& isTriggers) {
    selectedPartColliders.clear();
    if (SelectedPart() != nullptr)
        for (bool t : isTriggers) selectedPartColliders.emplace_back(t);
}

void EVAConstructionModeEditor::FetchColliders() {
    std::vector<bool> triggers;
    if (colliderTriggers) triggers = colliderTriggers(selectedPart);
    FetchColliders(triggers);
}

// EVAConstructionModeEditor.cs:1586
void EVAConstructionModeEditor::ToggleCollidersTrigger(bool toggle) {
    if (selectedPartColliders.empty()) return;
    for (auto& c : selectedPartColliders) c.current = toggle | c.isTrigger;
}

// EVAConstructionModeEditor.cs:1601
void EVAConstructionModeEditor::OnConstructionMode(bool opened) {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (!opened) {
        if (selectedPart != nullptr && backedUpPart != nullptr && inv != nullptr && inv->currentInventorySlotClicked == nullptr) {
            DestroyHeldPart();
            inv->ResetInventoryCacheValues();
            RestoreLastAttachedPart();
        }
        if (selectedPart != nullptr && selectedPart->fields.previousPhysicalSignificance != selectedPart->fields.physicalSignificance && InventoryEnvironment::scheduler)
            InventoryEnvironment::scheduler->StartCoroutine(ResetToPhysicalPart(selectedPart), 0x7ffffff0);
        ToggleOffsetGizmo(false);
        ToggleRotateGizmo(false);
        ToggleCollidersTrigger(false);
        selectedPartColliders.clear();
        OnPartDropped();
        RemoveAttachNodes();
    } else {
        PutPartBackInInventory();
        ToggleCoordSpaceButton(false);
    }
}

// EVAConstructionModeEditor.cs:1629
void EVAConstructionModeEditor::DestroyHeldPart() {
    if (selectedPart != nullptr) {
        ReleaseOwned(selectedPart);  // Object.Destroy(selectedPart.gameObject)
        selectedPart = nullptr;
    }
    if (EVAConstructionModeController::Instance != nullptr && EVAConstructionModeController::Instance->IsOpen()) GE::OnCollisionIgnoreUpdate.Fire();
}

// EVAConstructionModeEditor.cs:1646
void EVAConstructionModeEditor::BreakSymmetry(Part* part) {
    if (part != nullptr) {
        part->RemoveFromSymmetry();
        if (part->vessel != nullptr) GE::onVesselWasModified.Fire(part->vessel);
    }
}

// EVAConstructionModeEditor.cs:1658
void EVAConstructionModeEditor::DestroyBackupPart() {
    if (backedUpPart != nullptr) {
        backedUpPartOwner.reset();  // Object.Destroy(backedUpPart.gameObject); the field keeps the dead reference in C#
    }
}

// EVAConstructionModeEditor.cs:1666
bool EVAConstructionModeEditor::CanPartBeEdited(Part& part, bool weightOnlyCheck) {
    if (!EVAConstructionUtil::IsUnderConstructionWeightLimit(part)) {
        PostMessage(Localizer::Format("#autoLOC_8012062", {part.partInfo->title}));
        return false;
    }
    if (weightOnlyCheck) return true;
    auto* component = dynamic_cast<ModuleCargoPart*>(part.FindModuleOfClass("ModuleCargoPart"));
    if (part.children.size() <= 0 && !component->HasLinkedCompoundParts()) {
        if (!part.PartCanBeDetached()) {
            CannotInteract();
            return false;
        }
        if (part.protoModuleCrew.size() > 0) {
            PostMessage(Localizer::Format("#autoLOC_6011175", {part.partInfo->title}));
            return false;
        }
        return true;
    }
    PostMessage(Localizer::Format("#autoLOC_6006096", {part.partInfo->title}));
    return false;
}

// EVAConstructionModeEditor.cs:1696
void EVAConstructionModeEditor::CannotInteract() { PostMessage(cacheAutoLOC_6006095); }

// EVAConstructionModeEditor.cs:1701
void EVAConstructionModeEditor::InterruptKerbalWelding() {
    Vessel* active = FlightRegistry::ActiveVessel;
    if (active != nullptr && active->isEVA()) {
        if (EVAConstructionEnvironment::interruptWeld) EVAConstructionEnvironment::interruptWeld();
    }
}

// EVAConstructionModeEditor.cs:1709
void EVAConstructionModeEditor::InputCheckDetachCompundPart(bool pauseDown) {
    if (pauseDown && selectedCompoundPart != nullptr) {
        auto* cp = dynamic_cast<CompoundPart*>(selectedCompoundPart);
        if (cp != nullptr && cp->attachState() == CompoundPart::AttachState::Attaching) DetachPart(selectedCompoundPart);
    }
}

// EVAConstructionModeEditor.cs:1717
void EVAConstructionModeEditor::PlayAudioClip(const std::string& clip) {
    if (clip.empty()) return;
    Effect("audio:" + clip, nullptr);
}

void EVAConstructionModeEditor::PostMessage(const std::string& text) {
    screenMessages.push_back(text);
    InventoryEnvironment::Post(text, 5.0f);
}

// UnityEngine.Object.Destroy(gameObject) is deferred to the end of the frame: the part stays alive (its nodes are still read by the
// caller, e.g. AttachPart's Attachment) until FlushDestroyed()
void EVAConstructionModeEditor::ReleaseOwned(Part* p) {
    auto it = std::find_if(ownedParts.begin(), ownedParts.end(), [&](const std::unique_ptr<Part>& u) { return u.get() == p; });
    if (it != ownedParts.end()) {
        pendingDestroy.push_back(std::move(*it));
        ownedParts.erase(it);
    }
}

void EVAConstructionModeEditor::FlushDestroyed() { pendingDestroy.clear(); }

}  // namespace ksp
