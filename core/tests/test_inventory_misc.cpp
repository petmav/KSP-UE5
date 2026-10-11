// WP-33 unit tests: editor-side drops onto an inventory (PartDroppedOnInventory, the container highlight), ModuleCargoPart's settle coroutine and
// weld sparks, and the EVA construction attach / detach / backup / restore / gizmo paths. Every expectation is derived from the decompiled C#
// (the derivation is in the comment above each check).
#include "inventory_fixture.hpp"

#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/unity/quaternion_native.hpp"

using namespace ksp;
using namespace ksp::inv33;

// ModuleInventoryPart.cs:254-357: dropping a cargo part on an inventory in the editor
TEST_CASE("WP-33 ModuleInventoryPart.PartDroppedOnInventory: store, stack, limits, the root event, the container highlight") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::EDITOR);
    UIPartActionControllerInventory ctrl;
    Host h(g, "cargoContainer");
    ModuleInventoryPart* inv = h.inv;
    std::vector<Part*> destroyed;
    InventoryEnvironment::destroyPartObject = [&](Part& p) { destroyed.push_back(&p); };
    bool over = true;
    InventoryEnvironment::partMouseOver = [&](ModuleInventoryPart&) { return over; };
    auto kit1 = g.New("evaRepairKit");
    auto kit2 = g.New("evaRepairKit");
    auto jet = g.New("evaJetpack");
    // the mouse is elsewhere: nothing happens
    over = false;
    inv->PartDroppedOnInventory(ConstructionEventType::PartDropped, kit1.get());
    CHECK(inv->InventoryIsEmpty());
    over = true;
    // events other than PartDropped do not store
    inv->PartDroppedOnInventory(ConstructionEventType::PartAttached, kit1.get());
    inv->PartDroppedOnInventory(ConstructionEventType::PartDeleted, kit1.get());
    CHECK(inv->InventoryIsEmpty());
    // a stackable part with no stack to join goes into the first empty slot and the dropped part is destroyed
    inv->PartDroppedOnInventory(ConstructionEventType::PartDropped, kit1.get());
    REQUIRE(inv->storedParts.Count() == 1);
    CHECK(inv->storedParts[0]->partName == "evaRepairKit");
    CHECK(destroyed == std::vector<Part*>{kit1.get()});
    // the second kit joins the stack (stackableQuantity 4 > 1, CanStackInSlot)
    inv->PartDroppedOnInventory(ConstructionEventType::PartDropped, kit2.get());
    CHECK(inv->storedParts.Count() == 1);
    CHECK(inv->GetStackAmountAtSlot(0) == 2);
    CHECK(destroyed.size() == 2);
    // a non-stackable part takes a new slot
    inv->PartDroppedOnInventory(ConstructionEventType::PartDropped, jet.get());
    CHECK(inv->storedParts.Count() == 2);
    CHECK(inv->storedParts[1]->slotIndex == 1);
    // the `root` editor event keeps the part (the editor re-roots with it)
    auto kit3 = g.New("evaRepairKit");
    InventoryEnvironment::editorLastEventName = [] { return std::string("Root Part Selected"); };
    inv->PartDroppedOnInventory(ConstructionEventType::PartDropped, kit3.get());
    CHECK(inv->GetStackAmountAtSlot(0) == 3);
    CHECK(destroyed.size() == 3);  // jet only; kit3 was stored but not destroyed
    CHECK(std::find(destroyed.begin(), destroyed.end(), kit3.get()) == destroyed.end());
    InventoryEnvironment::editorLastEventName = nullptr;
    // a part that is itself an inventory container is not stored (availableCargoInventoryParts)
    auto container = g.New("smallCargoContainer");
    inv->PartDroppedOnInventory(ConstructionEventType::PartDropped, container.get());
    CHECK(inv->storedParts.Count() == 2);
    // the volume limit: 5 L kit with 15 L already in (3 kits) against a 16 L limit -> stored; the next one is refused
    inv->Set<float>("packedVolumeLimit", 16.0f);
    inv->UpdateCapacityValues();  // 3 x 5 + 17 (jetpack) = 32 L already: refused
    auto kit4 = g.New("evaRepairKit");
    const size_t before = destroyed.size();
    inv->PartDroppedOnInventory(ConstructionEventType::PartDropped, kit4.get());
    CHECK(destroyed.size() == before);
    CHECK(inv->GetStackAmountAtSlot(0) == 3);
    // the cheat ignores the limits only for an EVA kerbal's own inventory
    InventoryEnvironment::IgnoreKerbalInventoryLimits = true;
    inv->PartDroppedOnInventory(ConstructionEventType::PartDropped, kit4.get());
    CHECK(destroyed.size() == before);
    InventoryEnvironment::IgnoreKerbalInventoryLimits = false;
    // the mass limit: massOccupied + GetPartMass(p) > massLimit
    Host kerbal(g, "kerbalEVA");
    kerbal.inv->Set<float>("packedVolumeLimit", 0.0f);  // no volume limit, mass limit 0.065 stays
    std::vector<Part*> destroyed2;
    InventoryEnvironment::destroyPartObject = [&](Part& p) { destroyed2.push_back(&p); };
    auto heavy = g.New("evaRepairKit");
    heavy->fields.mass = 0.2f;  // above 0.065
    kerbal.inv->PartDroppedOnInventory(ConstructionEventType::PartDropped, heavy.get());
    CHECK(kerbal.inv->InventoryIsEmpty());
    // VesselEditorPartHighlighter: null clears; a non-cargo name is ignored; a non-attachable cargo part lights the containers
    inv->highlightLog.clear();
    inv->VesselEditorPartHighlighter(nullptr);
    CHECK(inv->highlightLog == std::vector<std::string>{"color:default", "highlight:off"});
    CHECK_FALSE(inv->partHighlightActive);
    inv->highlightLog.clear();
    auto unnamed = std::make_unique<Part>();
    unnamed->gameObjectName = "notACargoPart";
    inv->VesselEditorPartHighlighter(unnamed.get());
    CHECK(inv->highlightLog.empty());
    ctrl.SetCurrentCargoPart(kit1.get());  // evaRepairKit: attachRules 0,0,0,0,0 -> neither srfAttach nor allowStack (the event already ran the handler)
    inv->highlightLog.clear();
    inv->VesselEditorPartHighlighter(kit1.get());
    CHECK(inv->partHighlightActive);
    CHECK(inv->highlightLog == std::vector<std::string>{"color:inventoryContainer", "highlight:on"});
    kit1->fields.attachRules = AttachRules{false, true, false, false, false, false, false, false};  // surface attachable: not a container job
    ctrl.SetCurrentCargoPart(kit1.get());
    inv->highlightLog.clear();
    inv->VesselEditorPartHighlighter(kit1.get());
    CHECK_FALSE(inv->partHighlightActive);
    // OnEditorPartEvent: Detached / Copied set the controller's held part
    ctrl.SetCurrentCargoPart(nullptr);
    inv->OnEditorPartEvent(ConstructionEventType::PartDetached, jet.get());
    CHECK(ctrl.CurrentCargoPart() == jet.get());
    ctrl.SetCurrentCargoPart(nullptr);
    inv->OnEditorPartEvent(ConstructionEventType::PartCopied, jet.get());
    CHECK(ctrl.CurrentCargoPart() == jet.get());
    // a non-cargo part is not held
    ctrl.SetCurrentCargoPart(nullptr);
    inv->OnEditorPartEvent(ConstructionEventType::PartDetached, unnamed.get());
    CHECK(ctrl.CurrentCargoPart() == nullptr);
    // PartPicked / PartDragging are ignored
    inv->OnEditorPartEvent(ConstructionEventType::PartPicked, jet.get());
    CHECK(ctrl.CurrentCargoPart() == nullptr);
}
