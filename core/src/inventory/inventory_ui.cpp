// WP-33: the headless models of UIPartActionControllerInventory.cs, UIPartActionInventorySlot.cs, UIPartActionInventory.cs and
// InventoryPanelController.cs (the slot-click transfer rules, the held-part state, the panel registries, the limits readout texts), the
// kerbal-inventory glue (ProtoCrewMember.kerbalModule) and the module registration.
#include <algorithm>

#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/inventory/construction.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/module_part_variants.hpp"
#include "ksp/parts/available_part.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/save/crew.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/util/ksp_util.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

using GE = GameEvents;
using Localizer = KSP::Localization::Localizer;

namespace {
bool IsFlight() { return SaveEnvironment::Current().loadedScene == GameScenes::FLIGHT; }
bool IsEditor() { return SaveEnvironment::Current().loadedScene == GameScenes::EDITOR; }
bool Contains(const std::vector<std::string>& v, const std::string& s) { return std::find(v.begin(), v.end(), s) != v.end(); }
std::string VariantNameOf(Part& p) {
    auto* v = dynamic_cast<ModulePartVariants*>(p.FindModuleOfClass("ModulePartVariants"));
    if (v != nullptr && v->SelectedVariant() != nullptr) return v->SelectedVariant()->name;
    return "";
}
}  // namespace

// ================================================================ UIPartActionControllerInventory

// UIPartActionControllerInventory.cs:241 Awake
UIPartActionControllerInventory::UIPartActionControllerInventory() {
    if (Instance != nullptr) {
        log::Error("[UIPartActionControllerInventory]: Instance exists.");
        destroyed_ = true;
    } else {
        Instance = this;
    }
}

UIPartActionControllerInventory::~UIPartActionControllerInventory() {
    OnDestroy();
    if (Instance == this) Instance = nullptr;
}

// UIPartActionControllerInventory.cs:250
void UIPartActionControllerInventory::Start() {
    if (destroyed_) return;
    if (IsEditor()) GE::onEditorPartEvent.Add(this, &UIPartActionControllerInventory::OnCargoPartConstructionEvent);
    availableCargoParts = InventoryEnvironment::GetAvailableCargoPartNames();
    availableInventoryParts = InventoryEnvironment::GetAvailableCargoInventoryPartNames();
    currentHoveredInventoryPart = 0u;
    currentPawAutoOpened = false;
    cargoInventoriesOpen = 0;
    enableCargoLight = false;
    GE::onVesselDestroy.Add(this, &UIPartActionControllerInventory::OnVesselDestroy);
    GE::OnPartPurchased.Add(this, &UIPartActionControllerInventory::OnPartPurchased);
    eventsAdded_ = true;
}

// UIPartActionControllerInventory.cs:274
void UIPartActionControllerInventory::OnDestroy() {
    GE::onEditorPartEvent.Remove(this, &UIPartActionControllerInventory::OnCargoPartConstructionEvent);
    if (eventsAdded_) {
        GE::onVesselDestroy.Remove(this, &UIPartActionControllerInventory::OnVesselDestroy);
        GE::OnPartPurchased.Remove(this, &UIPartActionControllerInventory::OnPartPurchased);
        eventsAdded_ = false;
    }
}

// UIPartActionControllerInventory.cs:139 (setter)
void UIPartActionControllerInventory::SetCurrentCargoPart(Part* p) {
    currentCargoPart_ = p;
    GE::OnInventoryPartOnMouseChanged.Fire(CurrentCargoPart());
}

// UIPartActionControllerInventory.cs:152
ModuleCargoPart* UIPartActionControllerInventory::CurrentModuleCargoPart() const {
    if (currentCargoPart_ != nullptr) return dynamic_cast<ModuleCargoPart*>(currentCargoPart_->FindModuleOfClass("ModuleCargoPart"));
    return nullptr;
}

// UIPartActionControllerInventory.cs:105
ModuleInventoryPart* UIPartActionControllerInventory::CurrentInventorySlotHoveredModule() const {
    if (currentInventorySlotHovered == nullptr) throw NullReferenceException();
    return currentInventorySlotHovered->moduleInventoryPart;
}

// UIPartActionControllerInventory.cs:283
bool UIPartActionControllerInventory::IsKerbalWithinRange(ModuleInventoryPart& inventoryPart) {
    Vessel* active = FlightRegistry::ActiveVessel;
    if (active->isEVA()) currentDistanceFromPart = Vector3f::Distance(active->GetWorldPos3D().ToVector3f(), inventoryPart.TransformPosition());
    return currentDistanceFromPart <= InventoryEnvironment::EVA_INVENTORY_RANGE;
}

// UIPartActionControllerInventory.cs:293
void UIPartActionControllerInventory::OnCargoPartConstructionEvent(ConstructionEventType construction, Part* p) {
    if (!Contains(availableCargoParts, p->partInfo->name)) return;
    switch (construction) {
        case ConstructionEventType::PartOverInventoryGrid:
            if (currentInventorySlotClicked != nullptr) currentInventorySlotClicked->UpdateCurrentSelectedSlot(false);
            break;
        case ConstructionEventType::PartCreated: SetCurrentCargoPart(p); break;
        case ConstructionEventType::PartPicked: SetCurrentCargoPart(p); break;
        case ConstructionEventType::PartDropped:
        case ConstructionEventType::PartAttached:
        case ConstructionEventType::PartDeleted:
            if (currentInventorySlotClicked != nullptr) currentInventorySlotClicked->UpdateCurrentSelectedSlot(false);
            ResetInventoryCacheValues();
            break;
        default: break;
    }
}

// UIPartActionControllerInventory.cs:328
void UIPartActionControllerInventory::ResetInventoryCacheValues(bool destroyCurrentCargoPart) {
    if (currentCargoPart_ != nullptr && destroyCurrentCargoPart) {
        DestroyHeldObject(currentCargoPart_);
        if (hasCurrentInventoryOnlyIcon) {
            hasCurrentInventoryOnlyIcon = false;
        }
    }
    currentInventory = nullptr;
    currentInventorySlotClicked = nullptr;
    SetCurrentCargoPart(nullptr);
    GE::OnInventoryPartOnMouseChanged.Fire(nullptr);
    referenceNextCreatedInventoryPAW = false;
    currentHoveredInventoryPart = 0u;
    currentPawAutoOpened = false;
}

void UIPartActionControllerInventory::DestroyHeldObject(Part* p) {
    if (heldPartOwner.get() == p) heldPartOwner.reset();
    else if (InventoryEnvironment::destroyPartObject) InventoryEnvironment::destroyPartObject(*p);
}

// UIPartActionControllerInventory.cs:344 (PAW windows and SpawnPartActionWindow are the shell's)
void UIPartActionControllerInventory::OnInventoryPartHover(uint32_t partId, Part* p) {
    if (pawWindowOpenFor && pawWindowOpenFor(partId)) return;
    if (currentHoveredInventoryPart != partId) {
        currentHoveredInventoryPart = p->fields.persistentId;
        if (currentCargoPart_ != nullptr && !currentCargoPart_->fields.attachRules.value_or(AttachRules{}).StackOrSurfaceAttachable()) {
            referenceNextCreatedInventoryPAW = true;
            if (spawnPartActionWindow) spawnPartActionWindow(p);
        }
    }
}

// UIPartActionControllerInventory.cs:437
void UIPartActionControllerInventory::ReturnHeldPart() {
    if (currentInventorySlotClicked != nullptr && currentCargoPart_ != nullptr) {
        Part* go = currentCargoPart_;
        currentInventorySlotClicked->ReturnHeldPartToThisSlot();
        if (go != nullptr) DestroyHeldObject(go);
        if (hasCurrentInventoryOnlyIcon) hasCurrentInventoryOnlyIcon = false;
    }
}

// UIPartActionControllerInventory.cs:456
void UIPartActionControllerInventory::OnVesselDestroy(Vessel* v) {
    if (currentInventory != nullptr && currentInventory->VesselOf() != nullptr && currentInventory->VesselOf()->persistentId == v->persistentId) ReturnHeldPart();
}

// UIPartActionControllerInventory.cs:608
void UIPartActionControllerInventory::OnPartPurchased(AvailablePart*) {
    availableCargoParts = InventoryEnvironment::GetAvailableCargoPartNames();
    availableInventoryParts = InventoryEnvironment::GetAvailableCargoInventoryPartNames();
}

// UIPartActionControllerInventory.cs:614
void UIPartActionControllerInventory::DestroyHeldPartAsIcon() {
    if (hasCurrentInventoryOnlyIcon) hasCurrentInventoryOnlyIcon = false;
}

// UIPartActionControllerInventory.cs:537
void UIPartActionControllerInventory::ToggleCargoPartsLight(int counter) {
    if (!IsEditor()) {
        cargoInventoriesOpen += counter;
        enableCargoLight = cargoInventoriesOpen > 0;
    }
}

// UIPartActionControllerInventory.cs:660 (SetIconAsPart: the part leaves the icon slot; the flight branch re-parents it to the scene)
void UIPartActionControllerInventory::SetIconAsPart() {
    if (IsCursorOverAnyPAWOrCargoPane || isSetAsPart || !inventoryAndCargoPartExist()) return;
    if (IsEditor()) isSetAsPart = !heldPartIsStack;
    if (!IsFlight() || !(currentCargoPart_ != nullptr)) return;
    EVAConstructionModeController* c = EVAConstructionModeController::Instance;
    if (c != nullptr && c->IsOpen() && c->evaEditor->SelectedPart() != nullptr) {
        currentCargoPart_->partTransform.rotation = Quaternionf::identity();
        currentCargoPart_->partTransform.localScale = Vector3f::one();
        currentCargoPart_->fields.attRotation = Quaternionf::identity();
        currentCargoPart_->fields.attRotation0 = currentCargoPart_->partTransform.rotation;
        currentCargoPart_->fields.attPos0 = currentCargoPart_->partTransform.position;
        currentCargoPart_->hasRigidbody = true;  // AddComponent<Rigidbody> (kinematic, no gravity)
        EVAConstructionEnvironment::Effect("highlight:AlwaysOn", currentCargoPart_);
        isSetAsPart = true;
    } else {
        isSetAsPart = true;
    }
}

// UIPartActionController.InventoryAndCargoPartExist (UIPartActionController.cs): a held cargo part and its inventory
bool UIPartActionControllerInventory::inventoryAndCargoPartExist() const {
    return currentInventory != nullptr && currentCargoPart_ != nullptr;
}

// UIPartActionControllerInventory.cs:732 (state only: whether the held part / icon objects are active)
void UIPartActionControllerInventory::UpdateSelectedPartVisibility() {
    bool flag = false;
    EVAConstructionModeController* c = EVAConstructionModeController::Instance;
    if (hasCurrentInventoryOnlyIcon) {
        flag = IsCursorOverAnyPAWOrCargoPane;
        if (IsFlight()) {
            flag = true;
            if (c != nullptr && c->IsOpen()) {
                switch (c->panelMode) {
                    case EVAConstructionModeController::PanelMode::Cargo: flag = true; break;
                    case EVAConstructionModeController::PanelMode::Construction:
                        if (!IsCursorOverAnyPAWOrCargoPane) flag = false;
                        break;
                }
            }
        }
        if (!flag && hasCurrentInventoryOnlyIcon && stackSize > 1) flag = true;
        iconActive = flag;
    }
    if (!(currentCargoPart_ != nullptr)) return;
    flag = !IsCursorOverAnyPAWOrCargoPane;
    if (IsFlight() && c != nullptr) {
        if (c->IsOpen()) {
            if (c->panelMode == EVAConstructionModeController::PanelMode::Cargo) flag = false;
        } else {
            flag = false;
        }
    }
    if (flag && hasCurrentInventoryOnlyIcon && stackSize > 1) flag = false;
    if (heldPartActive == flag) return;
    const bool num = !heldPartActive && flag;
    heldPartActive = flag;
    if (num) {
        for (size_t i = 0; i < currentCargoPart_->Modules().Count(); i++) currentCargoPart_->Modules()[i]->OnInventoryModeDisable();
    }
}

// UIPartActionControllerInventory.cs:698
std::unique_ptr<Part> UIPartActionControllerInventory::CreatePartFromInventory(ProtoPartSnapshot& protoPartSnapshot) {
    std::unique_ptr<Part> part = protoPartSnapshot.CreatePart();
    ConfigureInventoryPart(*part);
    if (IsFlight()) {
        EVAConstructionEnvironment::Effect("part:OnInventoryModeDisable", part.get());
        for (size_t i = 0; i < part->Modules().Count(); i++) {
            part->Modules()[i]->isEnabled = false;
            if (currentInventory != nullptr) part->Modules()[i]->OnPartCreatedFomInventory(*currentInventory);
        }
    }
    return part;
}

// UIPartActionControllerInventory.cs:719
void UIPartActionControllerInventory::ConfigureInventoryPart(Part& invPart) {
    if (IsFlight()) {
        invPart.set_State(PartStates::CARGO);
        invPart.fields.ResumeState = static_cast<int32_t>(PartStates::CARGO);
    }
    if (IsFlight() || !invPart.fields.attachRules.value_or(AttachRules{}).StackOrSurfaceAttachable())
        EVAConstructionEnvironment::Effect("highlight:AlwaysOn", &invPart);
    for (size_t i = 0; i < invPart.Modules().Count(); i++) {
        invPart.Modules()[i]->OnInventoryModeDisable();
        invPart.Modules()[i]->isEnabled = false;
    }
    if (InventoryEnvironment::checkPartPersistentId) invPart.fields.persistentId = InventoryEnvironment::checkPartPersistentId(invPart);
}

// ================================================================ UIPartActionInventorySlot

UIPartActionInventorySlot::~UIPartActionInventorySlot() { OnDestroy(); }

// UIPartActionInventorySlot.cs:54
void UIPartActionInventorySlot::Setup(UIPartActionInventory* inventoryRef, int index) {
    inventoryPartActionUI = inventoryRef;
    moduleInventoryPart = inventoryPartActionUI->inventoryPartModule;
    slotIndex = index;
    if (!eventsAdded_) {
        GE::onEditorPartDeleted.Add(this, &UIPartActionInventorySlot::OnEditorPartDeleted);
        GE::OnEVACargoMode.Add(this, &UIPartActionInventorySlot::OnCargoModeToggled);
        eventsAdded_ = true;
    }
}

// UIPartActionInventorySlot.cs:65
void UIPartActionInventorySlot::SlotClicked() {
    EVAConstructionModeController* c = EVAConstructionModeController::Instance;
    if ((c != nullptr && c->IsOpen() && c->evaEditor != nullptr && c->evaEditor->EVAConstructionMode() != ConstructionMode::Place) || !pawControllerPresent)
        return;
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (inv->inventoryAndCargoPartExist()) ProcessClickWithHeldPart();
    else ProcessClickNoHeldPart();
    if (IsEditor()) inv->currentInventory = moduleInventoryPart;
}

// UIPartActionInventorySlot.cs:92
void UIPartActionInventorySlot::ProcessClickWithHeldPart() {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    Part* held = inv->CurrentCargoPart();
    // `GetComponentsInChildren<Part>()` walked from the last index down: any part in the held hierarchy with another persistentId
    // ends the do-while and posts the message
    const std::vector<Part*> parts = EditorLogicBase::FindPartsInChildren(held);
    for (Part* p : parts) {
        if (p->fields.persistentId != held->fields.persistentId) {
            InventoryEnvironment::Post(Localizer::Format("#autoLOC_6005091"), 5.0f);
            return;
        }
    }
    auto* heldCargo = dynamic_cast<ModuleCargoPart*>(held->FindModuleOfClass("ModuleCargoPart"));
    if (moduleInventoryPart->volumeCapacityReached || moduleInventoryPart->massCapacityReached || !(heldCargo->packedVolume() >= 0.0f)) return;
    const std::string text = VariantNameOf(*held);
    if (moduleInventoryPart->IsSlotEmpty(slotIndex)) {
        StorePartInEmptySlot(*held, text);
        if (UIPartActionControllerInventory::heldPartIsStack) {
            SetStackedPartsInSlot(slotIndex, UIPartActionControllerInventory::stackSize, text);
            UIPartActionControllerInventory::heldPartIsStack = false;
            UIPartActionControllerInventory::stackSize = 0;
        }
        inv->currentInventorySlotClicked = nullptr;
        inv->currentInventorySlotEmptied = nullptr;
        inventoryPartActionUI->SetAllSlotsNotSelected();
        inv->ResetInventoryCacheValues();
        inventoryPartActionUI->DestroyHeldPart();
        return;
    }
    Part* currentCargoPart = held;
    const int stackSize = UIPartActionControllerInventory::stackSize;
    std::string text2;
    std::shared_ptr<StoredPart> sp;
    if (moduleInventoryPart->storedParts.TryGetValue(slotIndex, sp)) text2 = sp->variantName;
    UIPartActionInventory* uIPartActionInventory = nullptr;
    if (inv->currentInventorySlotClicked != nullptr) uIPartActionInventory = inv->currentInventorySlotClicked->inventoryPartActionUI;
    if (moduleInventoryPart->CanStackInSlot(*currentCargoPart->partInfo, text, slotIndex)) {
        StackPartsInThisSlot(*inv->CurrentCargoPart(), text);
        if (uIPartActionInventory != nullptr) uIPartActionInventory->SetAllSlotsNotSelected();
        inv->currentInventorySlotClicked = nullptr;
        inv->PlayPartDroppedSFX();
        return;
    }
    // swap the held part with the slot's part
    inventoryPartActionUI->DestroyHeldPart();
    std::unique_ptr<Part> part = CreatePartFromThisSlot(slotIndex);
    const bool iconCreated = CreatePartIconFromThisSlot(slotIndex);
    const int currentStackedAmount = CurrentStackedAmount();
    if (CurrentStackedAmount() > 1) {
        EmptyStack();
        inv->currentInventorySlotEmptied = this;
    } else {
        inventoryPartActionUI->inventoryPartModule->ClearPartAtSlot(slotIndex);
        inventoryPartActionUI->slotPartIcon[static_cast<size_t>(slotIndex)] = "";
    }
    inventoryPartActionUI->slotPartIcon[static_cast<size_t>(slotIndex)] = currentCargoPart->partInfo->name;  // slotPartIcon[slotIndex].Create(...)
    inventoryPartActionUI->inventoryPartModule->StoreCargoPartAtSlot(currentCargoPart, slotIndex);
    SetStackedPartsInSlot(slotIndex, stackSize, text);
    if (!text2.empty() && part != nullptr && part->variants != nullptr) part->variants->SetVariant(text2);
    UIPartActionControllerInventory::stackSize = currentStackedAmount;
    UIPartActionControllerInventory::heldPartIsStack = currentStackedAmount > 1;
    Part* newHeld = part.get();
    // the previously held part object goes away with the cursor's icon; the slot's part becomes the held one
    Part* oldHeld = inv->CurrentCargoPart();
    inv->heldPartOwner = std::move(part);
    inv->SetCurrentCargoPart(newHeld);
    (void)oldHeld;
    if (iconCreated) {
        inv->currentInventoryOnlyIconStack = UIPartActionControllerInventory::stackSize;  // editorInventoryOnlyIcon.SetStackAmount
        inv->iconBackgroundVisible = UIPartActionControllerInventory::stackSize > 1;      // ToggleBackgroundVisibility
    }
    if (uIPartActionInventory != nullptr && uIPartActionInventory != inventoryPartActionUI) {
        uIPartActionInventory->SetAllSlotsNotSelected();
    } else if (inv->currentInventorySlotClicked != nullptr) {
        inv->currentInventorySlotClicked->UpdateCurrentSelectedSlot(false);
    }
    inv->currentInventorySlotClicked = nullptr;
    inv->PlayPartDroppedSFX();
    inventoryPartActionUI->SetAllSlotsNotSelected();
}

// UIPartActionInventorySlot.cs:217
void UIPartActionInventorySlot::ProcessClickNoHeldPart() {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (moduleInventoryPart->IsSlotEmpty(slotIndex)) return;
    UpdateCurrentSelectedSlot(true);
    std::unique_ptr<Part> currentCargoPart = CreatePartFromThisSlot(slotIndex);
    bool icon = false;
    if (CurrentStackedAmount() > 1) {
        if (InventoryEnvironment::MODIFIER_KEY_DOWN) {
            UIPartActionControllerInventory::stackSize = CurrentStackedAmount();
            UIPartActionControllerInventory::heldPartIsStack = true;
            inv->currentInventorySlotEmptied = this;
            icon = CreatePartIconFromThisSlot(slotIndex);
            EmptyStack();
        } else {
            UIPartActionControllerInventory::stackSize = 1;
            UIPartActionControllerInventory::heldPartIsStack = false;
            icon = CreatePartIconFromThisSlot(slotIndex);
            RemovePartFromStack(slotIndex);
        }
    } else {
        UIPartActionControllerInventory::stackSize = 1;
        UIPartActionControllerInventory::heldPartIsStack = false;
        icon = CreatePartIconFromThisSlot(slotIndex);
        inventoryPartActionUI->inventoryPartModule->ClearPartAtSlot(slotIndex);
    }
    Part* raw = currentCargoPart.get();
    inv->heldPartOwner = std::move(currentCargoPart);
    inv->SetCurrentCargoPart(raw);
    if (icon) inv->currentInventoryOnlyIconStack = UIPartActionControllerInventory::stackSize;
    inv->controlLocks.insert("CargoPartHeld");  // InputLockManager.SetControlLock(UI_DRAGGING, "CargoPartHeld")
}

// UIPartActionInventorySlot.cs:263
void UIPartActionInventorySlot::EmptyStack() {
    if (UIPartActionControllerInventory::Instance != nullptr) UIPartActionControllerInventory::Instance->currentInventorySlotEmptied = this;
    inventoryPartActionUI->inventoryPartModule->ClearPartAtSlot(slotIndex);
    ClearEmptiedSlot();
}

// UIPartActionInventorySlot.cs:273
void UIPartActionInventorySlot::ClearEmptiedSlot(int) {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (inv->currentInventorySlotEmptied != nullptr && inv->currentInventorySlotEmptied == this) {
        inv->currentInventorySlotEmptied = nullptr;
        inventoryPartActionUI->slotPartIcon[static_cast<size_t>(slotIndex)] = "";  // SetEmptySlot
        SetStackedPartsInSlot(slotIndex, 1);
    }
}

// UIPartActionInventorySlot.cs:284
void UIPartActionInventorySlot::StorePartInEmptySlot(Part& partToStore, const std::string& variantName) {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (inv->currentInventorySlotClicked == nullptr) inv->currentInventorySlotClicked = this;
    else inv->currentInventorySlotClicked->UpdateCurrentSelectedSlot(false);
    auto* cargoPartRef = dynamic_cast<ModuleCargoPart*>(partToStore.FindModuleOfClass("ModuleCargoPart"));
    moduleInventoryPart->StoreCargoPartAtSlot(&partToStore, slotIndex);
    if (inv->currentInventorySlotEmptied != nullptr) {
        if (UIPartActionControllerInventory::heldPartIsStack) {
            SetStackedPartsInSlot(slotIndex, UIPartActionControllerInventory::stackSize, variantName);
            UIPartActionControllerInventory::stackSize = 0;
            UIPartActionControllerInventory::heldPartIsStack = false;
        } else {
            SetStackedPartsInSlot(slotIndex, inv->currentInventorySlotEmptied->CurrentStackedAmount(), variantName);
        }
    }
    const int howMany = (!InventoryEnvironment::MODIFIER_KEY_DOWN || !IsEditor()) ? CurrentStackedAmount() : cargoPartRef->stackableQuantity();
    SetStackedPartsInSlot(slotIndex, howMany, variantName);
    if (inv->currentInventorySlotClicked != nullptr) inv->currentInventorySlotClicked->inventoryPartActionUI->DestroyHeldPart();
    inventoryPartActionUI->DestroyHeldPart();
    inv->PlayPartDroppedSFX();
    inv->ResetInventoryCacheValues();
}

// UIPartActionInventorySlot.cs:316
bool UIPartActionInventorySlot::StackPartsInThisSlot(Part& heldPart, const std::string& partVariantName) {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (inv->currentInventorySlotClicked == nullptr) inv->currentInventorySlotClicked = this;
    else inv->currentInventorySlotClicked->UpdateCurrentSelectedSlot(false);
    if ((UIPartActionControllerInventory::heldPartIsStack && UIPartActionControllerInventory::stackSize > 1) || InventoryEnvironment::MODIFIER_KEY_DOWN) {
        int num = (!InventoryEnvironment::MODIFIER_KEY_DOWN || !IsEditor())
                      ? UIPartActionControllerInventory::stackSize
                      : inv->CurrentModuleCargoPart()->stackableQuantity() - CurrentStackedAmount();
        while (num > 0 && HaveStackingSpace()) {
            StackPartsInSlot(slotIndex, partVariantName, false);
            num--;
        }
        UIPartActionControllerInventory::stackSize = num;
        UIPartActionControllerInventory::heldPartIsStack = num > 1;
        if (num == 0) {
            inventoryPartActionUI->DestroyHeldPart();
            inv->ResetInventoryCacheValues();
        } else if (inv->hasCurrentInventoryOnlyIcon) {
            inv->currentInventoryOnlyIconStack = num;
        }
        return true;
    }
    if (HaveStackingSpace() && heldPart.Resources.Count() == 0) {
        StackPartsInSlot(slotIndex, partVariantName);
        inventoryPartActionUI->DestroyHeldPart();
        inv->ResetInventoryCacheValues();
        return true;
    }
    return false;
}

// UIPartActionInventorySlot.cs:353
void UIPartActionInventorySlot::StackPartsInSlot(int slot, const std::string& variantName, bool destroyHeldPart) {
    if (InventoryEnvironment::MODIFIER_KEY_DOWN && IsEditor()) moduleInventoryPart->UpdateStackAmountAtSlot(slot, MaxStackedAmount(), variantName);
    else moduleInventoryPart->UpdateStackAmountAtSlot(slot, CurrentStackedAmount() + 1, variantName);
    if (destroyHeldPart && UIPartActionControllerInventory::stackSize == 0) inventoryPartActionUI->DestroyHeldPart();
}

// UIPartActionInventorySlot.cs:367
void UIPartActionInventorySlot::SetStackedPartsInSlot(int slot, int howMany, const std::string& variantName) {
    moduleInventoryPart->UpdateStackAmountAtSlot(slot, howMany, variantName);
}

// UIPartActionInventorySlot.cs:372
void UIPartActionInventorySlot::RemovePartFromStack(int slot, const std::string& variantName) {
    moduleInventoryPart->UpdateStackAmountAtSlot(slot, CurrentStackedAmount() - 1, variantName);
}

// UIPartActionInventorySlot.cs:383
std::unique_ptr<Part> UIPartActionInventorySlot::CreatePartFromThisSlot(int slotIdx) {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    std::unique_ptr<Part> part;
    inv->PlayPartSelectedSFX();
    inv->currentInventorySlotClicked = this;
    inv->currentInventory = inventoryPartActionUI->inventoryPartModule;
    std::shared_ptr<StoredPart> sp;
    if (slotIdx > -1 && moduleInventoryPart->storedParts.TryGetValue(slotIdx, sp)) {
        part = inv->CreatePartFromInventory(*sp->snapshot);
    }
    if (part == nullptr) return nullptr;
    if (IsFlight()) {
        Quaternionf selectedPartOriginalRotation = Quaternionf::identity();
        Vessel* active = FlightRegistry::ActiveVessel;
        if (active != nullptr && InventoryEnvironment::constructionOriginalRotation) {
            switch (active->situation) {
                case Vessel::Situations::LANDED:
                case Vessel::Situations::PRELAUNCH:
                case Vessel::Situations::FLYING:
                case Vessel::Situations::DOCKED: selectedPartOriginalRotation = InventoryEnvironment::constructionOriginalRotation(*active); break;
                default: break;
            }
        }
        EVAConstructionModeController::Instance->evaEditor->selectedPartOriginalRotation = selectedPartOriginalRotation;
    }
    if (!sp->variantName.empty()) {
        auto* v = dynamic_cast<ModulePartVariants*>(part->FindModuleOfClass("ModulePartVariants"));
        if (v != nullptr) v->SetVariant(sp->variantName);
    }
    // SetPartAsIcon: parented under the grid's selected-item icon, rotated Euler(-15,0,0) then (0,-30,0) (the shell draws it)
    inv->isSetAsPart = false;
    if (!EVAConstructionUtil::IsUnderConstructionWeightLimit(*part) && EVAConstructionModeController::Instance != nullptr &&
        !EVAConstructionModeController::Instance->Hover() && !inv->IsCursorOverAnyPAWOrCargoPane)
        EVAConstructionEnvironment::Effect("highlight:editorDetached", part.get());
    part->set_isAttached(false);
    return part;
}

// UIPartActionInventorySlot.cs:441 (the icon object is the shell's: state only)
bool UIPartActionInventorySlot::CreatePartIconFromThisSlot(int slotIdx) {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    inv->PlayPartSelectedSFX();
    inv->currentInventorySlotClicked = this;
    inv->currentInventory = inventoryPartActionUI->inventoryPartModule;
    if (slotIdx > -1 && moduleInventoryPart->storedParts.ContainsKey(slotIdx)) {
        inv->hasCurrentInventoryOnlyIcon = true;
        inv->currentInventoryOnlyIconStack = UIPartActionControllerInventory::stackSize;
        EVAConstructionModeController* c = EVAConstructionModeController::Instance;
        if (IsEditor() || (c != nullptr && c->IsOpen())) inv->iconBackgroundVisible = UIPartActionControllerInventory::stackSize > 1;
        return true;
    }
    return false;
}

// UIPartActionInventorySlot.cs:488
void UIPartActionInventorySlot::ReturnHeldPartToThisSlot() {
    UIPartActionControllerInventory::Instance->PlayPartDroppedSFX();
    SlotClicked();
}

// UIPartActionInventorySlot.cs:494
void UIPartActionInventorySlot::UpdateCurrentSelectedSlot(bool isCurrent) {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (pawControllerPresent) {
        if (inv->inventoryAndCargoPartExist() && inv->currentInventorySlotClicked != nullptr && inv->currentInventorySlotClicked != this)
            inv->currentInventorySlotClicked->UpdateCurrentSelectedSlot(false);
        selected = isCurrent;
    }
    inventoryPartActionUI->slotStackUI[static_cast<size_t>(slotIndex)] = {IsStackable(), CurrentStackedAmount(), MaxStackedAmount()};  // UpdateStackUI
}

// UIPartActionInventorySlot.cs:515
void UIPartActionInventorySlot::OnPointerEnter() {
    if (!(inventoryPartActionUI == nullptr) && slotIndex <= moduleInventoryPart->InventorySlots() - 1 && UIPartActionControllerInventory::Instance != nullptr) {
        UIPartActionControllerInventory::Instance->currentInventorySlotHovered = this;
        moduleInventoryPart->showPreview = true;
    }
}

// UIPartActionInventorySlot.cs:525
void UIPartActionInventorySlot::OnPointerExit() {
    if (UIPartActionControllerInventory::Instance->CurrentCargoPart() != nullptr) moduleInventoryPart->RestoreLimitsDisplay();
    UIPartActionControllerInventory::Instance->currentInventorySlotHovered = nullptr;
    moduleInventoryPart->showPreview = false;
}

// UIPartActionInventorySlot.cs:534
void UIPartActionInventorySlot::OnEditorPartDeleted(Part*) {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (inv->currentInventorySlotEmptied != nullptr) inv->currentInventorySlotEmptied->ClearEmptiedSlot();
    if (UIPartActionControllerInventory::heldPartIsStack) {
        UIPartActionControllerInventory::heldPartIsStack = false;
        UIPartActionControllerInventory::stackSize = 0;
    }
}

// UIPartActionInventorySlot.cs:544
void UIPartActionInventorySlot::OnCargoModeToggled(bool opened) {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (!opened && IsFlight() && inv != nullptr && inv->currentInventorySlotClicked == this) inv->ReturnHeldPart();
}

// UIPartActionInventorySlot.cs:552
void UIPartActionInventorySlot::OnDestroy() {
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (IsFlight() && inv != nullptr && inv->currentInventorySlotClicked == this) {
        if (inv->currentPawAutoOpening) {
            inv->currentPawAutoOpening = false;
            return;
        }
        inv->ReturnHeldPart();
        inv->DestroyTooltip();
    }
    if (eventsAdded_) {
        GE::onEditorPartDeleted.Remove(this, &UIPartActionInventorySlot::OnEditorPartDeleted);
        GE::OnEVACargoMode.Remove(this, &UIPartActionInventorySlot::OnCargoModeToggled);
        eventsAdded_ = false;
    }
}

// ================================================================ UIPartActionInventory

UIPartActionInventory::UIPartActionInventory() = default;
UIPartActionInventory::~UIPartActionInventory() { OnDestroy(); }

// UIPartActionInventory.cs:85
bool UIPartActionInventory::AbleToPlaceParts() const { return inventoryPartModule != nullptr ? inventoryPartModule->AbleToPlaceParts() : false; }

// UIPartActionInventory.cs:90
void UIPartActionInventory::OnDestroy() {
    if (!eventsAdded_) return;
    GE::onVesselSituationChange.Remove(this, &UIPartActionInventory::OnVesselSituationChange);
    GE::onVesselChange.Remove(this, &UIPartActionInventory::OnVesselChange);
    GE::onModuleInventoryChanged.Remove(this, &UIPartActionInventory::OnModuleInventoryChanged);
    GE::onModuleInventorySlotChanged.Remove(this, &UIPartActionInventory::OnModuleInventorySlotChanged);
    eventsAdded_ = false;
    if (UIPartActionControllerInventory::Instance != nullptr) UIPartActionControllerInventory::Instance->ToggleCargoPartsLight(-1);
    UIPartActionControllerInventory::Instance ? (void)UIPartActionControllerInventory::Instance->controlLocks.erase("CargoPartHeld") : void();
}

// UIPartActionInventory.cs:106 (flight / editor PAW)
void UIPartActionInventory::SetupPaw(ModuleInventoryPart& inventoryPart, int slots) {
    inventoryPartModule = &inventoryPart;
    inventoryPart.pawInventory = this;
    inventoryPart.pawInventoryActive = true;
    if (inventoryPartModule->IsKerbalOnEVA()) {
        GE::onVesselSituationChange.Add(this, &UIPartActionInventory::OnVesselSituationChange);
        GE::onVesselChange.Add(this, &UIPartActionInventory::OnVesselChange);
    }
    InitializeMassVolumeDisplay();
    GE::onModuleInventoryChanged.Add(this, &UIPartActionInventory::OnModuleInventoryChanged);
    GE::onModuleInventorySlotChanged.Add(this, &UIPartActionInventory::OnModuleInventorySlotChanged);
    eventsAdded_ = true;
    fieldValue = slots;
    InitializeSlots();
}

// UIPartActionInventory.cs:148
void UIPartActionInventory::SetupConstruction(ModuleInventoryPart& inventoryPart) {
    inventoryPartModule = &inventoryPart;
    inventoryPartModule->constructorModeInventory = this;
    inCargoPane = true;
    InitializeMassVolumeDisplay();
    if (inventoryPartModule->IsKerbalOnEVA()) {
        GE::onVesselSituationChange.Add(this, &UIPartActionInventory::OnVesselSituationChange);
        GE::onVesselChange.Add(this, &UIPartActionInventory::OnVesselChange);
    }
    GE::onModuleInventoryChanged.Add(this, &UIPartActionInventory::OnModuleInventoryChanged);
    GE::onModuleInventorySlotChanged.Add(this, &UIPartActionInventory::OnModuleInventorySlotChanged);
    eventsAdded_ = true;
    fieldValue = inventoryPartModule->InventorySlots();
    if (inventoryPartModule->kerbalMode() && inventoryPartModule->kerbalReference != nullptr)
        inventoryNameText = inventoryPartModule->kerbalReference->displayName();
    else
        inventoryNameText = inventoryPartModule->PartOf()->partInfo->title;
    InitializeSlots();
}

// UIPartActionInventory.cs:190
void UIPartActionInventory::OnVesselSituationChange(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> data) {
    if (data.host->id == inventoryPartModule->VesselOf()->id) UpdatePlacePartIcons(data.to == Vessel::Situations::LANDED);
}

// UIPartActionInventory.cs:198
void UIPartActionInventory::OnVesselChange(Vessel* vsl) { UpdatePlacePartIcons(vsl->id == inventoryPartModule->VesselOf()->id); }

// UIPartActionInventory.cs:203
void UIPartActionInventory::OnModuleInventoryChanged(ModuleInventoryPart* changed) {
    if (!(changed != inventoryPartModule)) {
        for (size_t i = 0; i < slotPartIcon.size(); i++) UpdateSlot(static_cast<int>(i), false);
    }
}

// UIPartActionInventory.cs:215
void UIPartActionInventory::OnModuleInventorySlotChanged(ModuleInventoryPart* inventory, int slotChanged) {
    if (!(inventoryPartModule != inventory)) UpdateSlot(slotChanged, true);
}

// UIPartActionInventory.cs:224
void UIPartActionInventory::UpdateSlot(int slotChanged, bool updateStackUI) {
    const std::string text = slotPartIcon.at(static_cast<size_t>(slotChanged));
    std::shared_ptr<StoredPart> val;
    if (inventoryPartModule->storedParts.TryGetValue(slotChanged, val)) {
        if (text != val->partName) {
            DestroyItem(slotChanged);
            SpawnLoadedPart(*val);
        }
    } else if (!text.empty()) {
        DestroyItem(slotChanged);
    }
    if (updateStackUI) {
        UIPartActionInventorySlot& s = *slotButton.at(static_cast<size_t>(slotChanged));
        slotStackUI.at(static_cast<size_t>(slotChanged)) = {s.IsStackable(), s.CurrentStackedAmount(), s.MaxStackedAmount()};
    }
}

// UIPartActionInventory.cs:255
void UIPartActionInventory::InitializeSlots() {
    if (UIPartActionControllerInventory::Instance != nullptr) UIPartActionControllerInventory::Instance->ToggleCargoPartsLight(1);
    slotButton.clear();
    slotPartIcon.clear();
    slotStackUI.clear();
    for (int i = 0; i < fieldValue; i++) {
        slotPartIcon.push_back("");
        slotStackUI.push_back({false, 0, 0});
        slotButton.push_back(std::make_unique<UIPartActionInventorySlot>());
        if (!inventoryPartModule->storedPartsNull && inventoryPartModule->storedParts.ContainsKey(i)) SpawnLoadedPart(*inventoryPartModule->storedParts[i]);
        else SpawnEmptySlot(i);
    }
}

// UIPartActionInventory.cs:301
void UIPartActionInventory::SpawnEmptySlot(int slotIndex) {
    if (slotIndex > -1 && slotIndex <= static_cast<int>(slotButton.size()) - 1) slotButton[static_cast<size_t>(slotIndex)]->Setup(this, slotIndex);
    DestroyItem(slotIndex);
}

// UIPartActionInventory.cs:310
void UIPartActionInventory::SpawnLoadedPart(StoredPart& sp) {
    if (sp.slotIndex > -1 && sp.slotIndex <= static_cast<int>(slotButton.size()) - 1) {
        slotButton[static_cast<size_t>(sp.slotIndex)]->Setup(this, sp.slotIndex);
        if (!sp.partName.empty()) {
            sp.partName = KSPUtil::SanitizeInstanceName(sp.partName);
            slotPartIcon[static_cast<size_t>(sp.slotIndex)] = sp.partName;  // slotPartIcon[...].Create(null, ap, sp, ...)
            if (sp.quantity > 1) slotButton[static_cast<size_t>(sp.slotIndex)]->SetStackedPartsInSlot(sp.slotIndex, sp.quantity);
        } else {
            slotPartIcon[static_cast<size_t>(sp.slotIndex)] = "";
        }
    } else {
        log::Warning("[UIPartActionInventory]: Invalid slotIndex passed Index: " + std::to_string(sp.slotIndex));
    }
}

// ModuleInventoryPart.cs:349 (grid.pawInventory.slotPartIcon[num].Create(...))
void UIPartActionInventory::SpawnLoadedPartAtSlot(int slot) {
    std::shared_ptr<StoredPart> sp;
    if (inventoryPartModule->storedParts.TryGetValue(slot, sp)) slotPartIcon[static_cast<size_t>(slot)] = sp->partName;
}

// UIPartActionInventory.cs:342
void UIPartActionInventory::DestroyHeldPart(bool fromConstructionController) {
    if (inCargoPane && !fromConstructionController) {
        if (IsFlight()) EVAConstructionModeController::Instance->DestroyHeldIcons(this);
        if (IsEditor() && InventoryPanelController::Instance != nullptr) InventoryPanelController::Instance->DestroyHeldIcons(this);
    }
    UIPartActionControllerInventory* inv = UIPartActionControllerInventory::Instance;
    if (inv->hasCurrentInventoryOnlyIcon) inv->hasCurrentInventoryOnlyIcon = false;
    inv->controlLocks.erase("CargoPartHeld");
}

// UIPartActionInventory.cs:370
void UIPartActionInventory::DestroyItem(int index) {
    if (static_cast<int>(slotPartIcon.size()) >= index && index >= 0 && index < static_cast<int>(slotPartIcon.size())) slotPartIcon[static_cast<size_t>(index)] = "";
}

// UIPartActionInventory.cs:438
void UIPartActionInventory::SetAllSlotsNotSelected() {
    for (auto& s : slotButton) s->UpdateCurrentSelectedSlot(false);
}

// UIPartActionInventory.cs:446
void UIPartActionInventory::StartPartPlacement(int slotIconIndex) {
    if (slotIconIndex > -1 && inventoryPartModule != nullptr) {
        UpdatePlacePartIcons(false);
        inventoryPartModule->DeployInventoryItem(slotIconIndex);
    }
}

// UIPartActionInventory.cs:466
void UIPartActionInventory::ResetPlacePartIcons(bool active) {
    if (active) {
        if (inventoryPartModule != nullptr && inventoryPartModule->VesselOf() != nullptr && inventoryPartModule->AbleToPlaceParts()) UpdatePlacePartIcons(true);
        else UpdatePlacePartIcons(false);
    } else {
        UpdatePlacePartIcons(false);
    }
}

// UIPartActionInventory.cs:484
void UIPartActionInventory::UpdatePlacePartIcons(bool active) {
    placePartIconActive.assign(slotPartIcon.size(), false);
    for (size_t i = 0; i < slotPartIcon.size(); i++) {
        const bool hasIcon = !slotPartIcon[i].empty();
        EVAConstructionModeController* c = EVAConstructionModeController::Instance;
        placePartIconActive[i] = active && hasIcon && SlotIsDeployable(static_cast<int>(i)) && (c == nullptr || !c->IsOpen());
    }
}

bool UIPartActionInventory::SlotIsDeployable(int slot) const {
    if (slotPartIcon[static_cast<size_t>(slot)].empty()) return false;
    const AvailablePart* ap = PartLoader::Instance() ? PartLoader::Instance()->getPartInfoByName(slotPartIcon[static_cast<size_t>(slot)]) : nullptr;
    return ap != nullptr && ap->partPrefab->isGroundDeployable();  // EditorPartIcon.IsDeployablePart
}

// UIPartActionInventory.cs:517
void UIPartActionInventory::InitializeMassVolumeDisplay() {
    limitsFooterActive = inventoryPartModule->HasMassLimit() || inventoryPartModule->HasPackedVolumeLimit();
    packedVolumeLimitsActive = inventoryPartModule->HasPackedVolumeLimit();
    massLimitsActive = inventoryPartModule->HasMassLimit();
    if (inventoryPartModule != nullptr) {
        UpdateVolumeLimits(inventoryPartModule->volumeCapacity, inventoryPartModule->packedVolumeLimit(),
                           inventoryPartModule->volumeCapacity > inventoryPartModule->packedVolumeLimit(), false);
        UpdateMassLimits(inventoryPartModule->massCapacity, inventoryPartModule->massLimit(), inventoryPartModule->massCapacity > inventoryPartModule->massLimit());
    }
}

// UIPartActionInventory.cs:540
void UIPartActionInventory::UpdateVolumeLimits(float currentPackedVolume, float maxPackedVolume, bool overLimit, bool constructionOnly) {
    volumeSliderMax = maxPackedVolume;
    volumeSliderValue = currentPackedVolume;
    if (constructionOnly) packedVolumeText = Localizer::Format("#autoLOC_6002642");
    else packedVolumeText = Localizer::Format("#autoLOC_8003412", {dotnet::FormatSingleF(currentPackedVolume, 1), dotnet::FormatSingleF(maxPackedVolume, 1)});
    volumeOverLimit = overLimit;
}

// UIPartActionInventory.cs:566
void UIPartActionInventory::UpdateMassLimits(float currentMass, float maxMass, bool overLimit) {
    massSliderMax = maxMass;
    massSliderValue = currentMass;
    massText = Localizer::Format("#autoLOC_8003413", {dotnet::FormatSingleF(currentMass, 3), dotnet::FormatSingleF(maxMass, 3)});
    massOverLimit = overLimit;
}

// ================================================================ InventoryPanelController

InventoryPanelController::InventoryPanelController() {
    if (Instance != nullptr) return;  // Awake: `if (Instance) Destroy(this)`
    Instance = this;
}

InventoryPanelController::~InventoryPanelController() {
    OnDestroy();
    if (Instance == this) Instance = nullptr;
}

// InventoryPanelController.cs:80
void InventoryPanelController::Start() {
    GE::onEditorShipModified.Add(this, &InventoryPanelController::OnEditorShipModified);
    GE::onEditorShipCrewModified.Add(this, &InventoryPanelController::OnEditorCrewModified);
    GE::onEditorScreenChange.Add(this, &InventoryPanelController::OnEditorScreenChange);
    eventsAdded_ = true;
}

// InventoryPanelController.cs:88
void InventoryPanelController::OnDestroy() {
    if (!eventsAdded_) return;
    GE::onEditorShipModified.Remove(this, &InventoryPanelController::OnEditorShipModified);
    GE::onEditorShipCrewModified.Remove(this, &InventoryPanelController::OnEditorCrewModified);
    GE::onEditorScreenChange.Remove(this, &InventoryPanelController::OnEditorScreenChange);
    eventsAdded_ = false;
}

// InventoryPanelController.cs:95
void InventoryPanelController::Update() {
    if (isOpen) UpdateDisplayedInventories();
}

// InventoryPanelController.cs:108
void InventoryPanelController::OnEditorScreenChange(EditorScreen screen) {
    isOpen = screen == EditorScreen::Cargo;
    if (isOpen == prevIsOpen) return;
    prevIsOpen = IsOpen();
    if (isOpen) {
        // EditorPartList.Instance.ExcludeFilters.AddFilter(cargoPartsFilter)
    } else {
        moduleInventories.clear();
        displayedInventories.clear();
        uiInventories.clear();
    }
}

// InventoryPanelController.cs:146
void InventoryPanelController::OnEditorCrewModified(VesselCrewManifest* manifest) {
    if (manifest != nullptr) crew = manifest->GetAllCrew(false);
    if (EditorLogic::fetch != nullptr && EditorLogic::fetch->ship != nullptr) UpdateInventoryLists(EditorLogic::fetch->ship->liveParts, crew);
}

// InventoryPanelController.cs:158
void InventoryPanelController::OnEditorShipModified(ShipConstruct* ship) {
    if (shipManifestPresent && shipManifestCrew) crew = shipManifestCrew();
    if (ship != nullptr) UpdateInventoryLists(ship->liveParts, crew);
}

// InventoryPanelController.cs:170
void InventoryPanelController::UpdateInventoryLists(const std::vector<Part*>& parts, std::vector<std::shared_ptr<ProtoCrewMember>> crewList) {
    for (size_t i = 0; i < parts.size(); i++) {
        auto* m = dynamic_cast<ModuleInventoryPart*>(parts[i]->FindModuleOfClass("ModuleInventoryPart"));
        if (m != nullptr && std::find(moduleInventories.begin(), moduleInventories.end(), m) == moduleInventories.end()) moduleInventories.push_back(m);
    }
    if (shipManifestPresent) {
        crewList = shipManifestCrew ? shipManifestCrew() : std::vector<std::shared_ptr<ProtoCrewMember>>{};
        for (size_t j = 0; j < crewList.size(); j++) {
            ModuleInventoryPart* k = KerbalInventory::ModuleFor(*crewList[j]);
            if (k != nullptr && std::find(moduleInventories.begin(), moduleInventories.end(), k) == moduleInventories.end()) moduleInventories.push_back(k);
        }
    }
    int count = static_cast<int>(moduleInventories.size());
    while (count-- > 0) {
        bool flag = false;
        for (size_t k = 0; k < parts.size(); k++) {
            auto* m2 = dynamic_cast<ModuleInventoryPart*>(parts[k]->FindModuleOfClass("ModuleInventoryPart"));
            if (m2 != nullptr && moduleInventories[static_cast<size_t>(count)] == m2) {
                flag = true;
                break;
            }
        }
        for (size_t l = 0; l < crewList.size(); l++) {
            ModuleInventoryPart* k2 = KerbalInventory::ModuleFor(*crewList[l]);
            if (k2 != nullptr && moduleInventories[static_cast<size_t>(count)] == k2) {
                flag = true;
                break;
            }
        }
        if (!flag) moduleInventories.erase(moduleInventories.begin() + count);
    }
    for (auto& d : displayedInventories)
        if (d.uiInventory != nullptr) d.uiInventory->SetAllSlotsNotSelected();
}

// InventoryPanelController.cs:229
void InventoryPanelController::UpdateDisplayedInventories() {
    bool flag = false;
    for (size_t i = 0; i < moduleInventories.size(); i++) {
        ModuleInventoryPart* inventoryModule = moduleInventories[i];
        InventoryDisplayItem* displayItem = nullptr;
        if (TryGetDisplayedInventory(inventoryModule, displayItem)) continue;
        InventoryDisplayItem item;
        item.inventoryModule = inventoryModule;
        item.hasUiObject = true;
        auto ui = std::make_unique<UIPartActionInventory>();
        item.uiInventory = ui.get();
        item.uiInventory->SetupConstruction(*moduleInventories[i]);
        uiInventories.push_back(std::move(ui));
        displayedInventories.push_back(item);
        flag = true;
    }
    int count = static_cast<int>(displayedInventories.size());
    while (count-- > 0) {
        if (std::find(moduleInventories.begin(), moduleInventories.end(), displayedInventories[static_cast<size_t>(count)].inventoryModule) == moduleInventories.end()) {
            UIPartActionInventory* ui = displayedInventories[static_cast<size_t>(count)].uiInventory;
            displayedInventories.erase(displayedInventories.begin() + count);
            uiInventories.erase(std::remove_if(uiInventories.begin(), uiInventories.end(), [&](const auto& u) { return u.get() == ui; }), uiInventories.end());
            flag = true;
        }
    }
    if (!flag) return;
    siblingOrder.clear();
    for (size_t j = 0; j < displayedInventories.size(); j++)
        if (displayedInventories[j].inventoryModule->kerbalMode()) siblingOrder.push_back(static_cast<int>(j));
    for (size_t k = 0; k < displayedInventories.size(); k++)
        if (!displayedInventories[k].inventoryModule->kerbalMode()) siblingOrder.push_back(static_cast<int>(k));
}

// InventoryPanelController.cs:273
bool InventoryPanelController::TryGetDisplayedInventory(ModuleInventoryPart* inventoryModule, InventoryDisplayItem*& displayItem) {
    for (auto& d : displayedInventories) {
        if (d.inventoryModule == inventoryModule) {
            displayItem = &d;
            return true;
        }
    }
    displayItem = nullptr;
    return false;
}

// InventoryPanelController.cs:293
void InventoryPanelController::DestroyHeldIcons(UIPartActionInventory* callingInventory) {
    for (auto& d : displayedInventories)
        if (d.uiInventory != callingInventory) d.uiInventory->DestroyHeldPart(true);
}

// ================================================================ kerbal glue (ProtoCrewMember.cs:348 kerbalModule)

namespace {
struct KerbalModuleRecord {
    std::unique_ptr<Part> part;
    ModuleInventoryPart* module = nullptr;
};
std::map<ProtoCrewMember*, KerbalModuleRecord>& Records() {
    static std::map<ProtoCrewMember*, KerbalModuleRecord> r;
    return r;
}
const AvailablePart* EvaPartOf(ProtoCrewMember& crew) {
    const std::string name = crew.GetKerbalEVAPartName();
    if (PartLoader::Instance() != nullptr) return PartLoader::Instance()->getPartInfoByName(name);
    const SaveEnvironment& env = SaveEnvironment::Current();
    return env.getPartInfoByName ? env.getPartInfoByName(name) : nullptr;
}
}  // namespace

// ProtoCrewMember.cs:348
ModuleInventoryPart* KerbalInventory::ModuleFor(ProtoCrewMember& crew) {
    auto& recs = Records();
    auto it = recs.find(&crew);
    if (it != recs.end()) return it->second.module;
    const AvailablePart* ap = EvaPartOf(crew);
    if (ap == nullptr) throw NullReferenceException();
    auto* cachedPrefabModule = dynamic_cast<ModuleInventoryPart*>(ap->partPrefab->FindModuleOfClass("ModuleInventoryPart"));
    if (cachedPrefabModule == nullptr) throw NullReferenceException();
    KerbalModuleRecord rec;
    rec.part = Part::Instantiate(*ap->partPrefab);
    rec.part->partInfo = ap;
    rec.part->gameObjectName = ap->name;
    rec.module = dynamic_cast<ModuleInventoryPart*>(rec.part->FindModuleOfClass("ModuleInventoryPart"));
    rec.module->kerbalReference = crew.shared_from_this();
    Game* game = SaveEnvironment::Current().currentGame;
    if (KerbalInventoryScenario::Instance != nullptr && game != nullptr && (game->Mode == Game_Modes::SCENARIO || game->Mode == Game_Modes::SCENARIO_NON_RESUMABLE))
        KerbalInventoryScenario::Instance->AddKerbalInventoryInstance(crew.name().value_or(""), rec.module);
    rec.module->Load(*crew.InventoryNode());
    rec.module->OnStart(StartState::Editor);
    ModuleInventoryPart* m = rec.module;
    recs.emplace(&crew, std::move(rec));
    return m;
}

void KerbalInventory::Forget(ProtoCrewMember* crew) { Records().erase(crew); }
void KerbalInventory::ClearAll() { Records().clear(); }

// ProtoCrewMember.cs:831 SaveInventory
void ProtoCrewMember::SaveInventory(ModuleInventoryPart* inventory) {
    inventoryNode = std::make_shared<ConfigNode>("INVENTORY");
    if (type() != KerbalType::Tourist && inventory != nullptr) inventory->Save(*inventoryNode);
}

void KerbalInventory::Install() {
    ProtoCrewMember::kerbalModuleCost = [](ProtoCrewMember& c) { return KerbalInventory::ModuleFor(c)->GetModuleCost(0.0f, ModifierStagingSituation::CURRENT); };
    ProtoCrewMember::kerbalModuleMass = [](ProtoCrewMember& c) { return KerbalInventory::ModuleFor(c)->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT); };
    // ProtoCrewMember.cs:789 ResourceMass: the EVA prefab part's resources
    ProtoCrewMember::kerbalPrefabResourceMass = [](ProtoCrewMember& c) {
        float num = 0.0f;
        const AvailablePart* ap = EvaPartOf(c);
        if (ap == nullptr) throw NullReferenceException();
        Part& cachedPart = *ap->partPrefab;
        for (int i = 0; i < cachedPart.Resources.Count(); i++)
            num = f32store(fx(num) + fx(f32store(cachedPart.Resources[static_cast<size_t>(i)].amount)) * fx(cachedPart.Resources[static_cast<size_t>(i)].info->density()));
        return num;
    };
    SaveEnvironment::Current().saveDefaultKerbalInventory = [](std::string_view evaPartName, ConfigNode& inventory) {
        const AvailablePart* ap = PartLoader::Instance() ? PartLoader::Instance()->getPartInfoByName(std::string(evaPartName)) : nullptr;
        if (ap == nullptr) return;
        auto* m = dynamic_cast<ModuleInventoryPart*>(ap->partPrefab->FindModuleOfClass("ModuleInventoryPart"));
        if (m != nullptr) m->SaveDefault(inventory);
    };
}

void KerbalInventory::Uninstall() {
    ProtoCrewMember::kerbalModuleCost = nullptr;
    ProtoCrewMember::kerbalModuleMass = nullptr;
    ProtoCrewMember::kerbalPrefabResourceMass = nullptr;
    SaveEnvironment::Current().saveDefaultKerbalInventory = nullptr;
}

// ================================================================ registration

namespace detail {
void RegisterInventoryModules() {
    ModuleFactory::Register("ModuleCargoPart", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleCargoPart>(c)); });
    for (const char* n : {"ModuleGroundPart", "ModuleGroundSciencePart", "ModuleGroundCommsPart", "ModuleGroundExpControl", "ModuleGroundExperiment"})
        ModuleFactory::Register(n, [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<GroundPartSeam>(c)); });
    ModuleFactory::Register("ModuleInventoryPart", [](const ModuleClassInfo& c) { return std::static_pointer_cast<PartModule>(std::make_shared<ModuleInventoryPart>(c)); });
}
}  // namespace detail

}  // namespace ksp
