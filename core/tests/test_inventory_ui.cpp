// WP-33 unit tests: the headless inventory UI models (UIPartActionControllerInventory / UIPartActionInventorySlot / UIPartActionInventory /
// InventoryPanelController), the EVA construction controller's inventory registry and PartItemTransfer. Every expectation is derived from the
// decompiled C# (the derivation is in the comment above each check).
#include "inventory_fixture.hpp"

using namespace ksp;
using namespace ksp::inv33;

namespace {

// two inventories with their PAW grids, one controller (editor scene)
struct UiWorld {
    Game15 g;
    SceneScope scene{GameScenes::EDITOR};
    UIPartActionControllerInventory ctrl;
    Host a{g, "cargoContainer"};       // 9 slots, 650 L
    Host b{g, "smallCargoContainer"};  // 3 slots, 180 L
    UIPartActionInventory uiA, uiB;
    UiWorld() {
        uiA.SetupPaw(*a.inv, 9);
        uiB.SetupPaw(*b.inv, 3);
    }
    UIPartActionInventorySlot& SlotA(int i) { return *uiA.slotButton[static_cast<size_t>(i)]; }
    UIPartActionInventorySlot& SlotB(int i) { return *uiB.slotButton[static_cast<size_t>(i)]; }
    Part& P(const char* n) { return Prefab(g, n); }
};

struct Transfer : PartItemTransfer {
    bool IsValidPart(Part& p) override { return p.Resources.Count() > 0; }
    bool IsSemiValidPart(Part& p) override { return p.fields.mass > 1.0f; }
    int hooked = 0, unhooked = 0, found = 0;
    void AfterPartsFound() override { found = static_cast<int>(validParts.size() * 10 + semiValidParts.size()); }
    void HookAdditionalEvents() override { hooked++; }
    void UnhookAdditionalEvents() override { unhooked++; }
};

}  // namespace

// UIPartActionInventory.cs:224-340: the slot icons follow the module events
TEST_CASE("WP-33 UIPartActionInventory: slot icons, stack readout and limit texts follow the module") {
    if (!GameAvailable()) return;
    Reset33 r;
    UiWorld w;
    LoadLocalizer();
    CHECK(w.uiA.slotButton.size() == 9);
    CHECK(w.uiA.fieldValue == 9);
    REQUIRE(w.a.inv->StoreCargoPartAtSlot(&w.P("evaRepairKit"), 0));
    REQUIRE(w.a.inv->UpdateStackAmountAtSlot(0, 3));
    REQUIRE(w.a.inv->StoreCargoPartAtSlot(&w.P("evaJetpack"), 4));
    CHECK(w.uiA.slotPartIcon[0] == "evaRepairKit");
    CHECK(w.uiA.slotPartIcon[4] == "evaJetpack");
    CHECK(w.uiA.slotPartIcon[1] == "");
    CHECK(w.uiA.slotStackUI[0].stackable);
    CHECK(w.uiA.slotStackUI[0].amount == 3);
    CHECK(w.uiA.slotStackUI[0].max == 4);
    CHECK_FALSE(w.uiA.slotStackUI[4].stackable);
    // another module's events do not touch this grid
    REQUIRE(w.b.inv->StoreCargoPartAtSlot(&w.P("evaChute"), 0));
    CHECK(w.uiA.slotPartIcon[1] == "");
    CHECK(w.uiB.slotPartIcon[0] == "evaChute");
    // clearing a slot empties the icon
    w.a.inv->ClearPartAtSlot(4);
    CHECK(w.uiA.slotPartIcon[4] == "");
    // the limits footer (cargoContainer: packedVolumeLimit 650, no mass limit)
    CHECK(w.uiA.limitsFooterActive);
    CHECK(w.uiA.packedVolumeLimitsActive);
    CHECK_FALSE(w.uiA.massLimitsActive);
    CHECK(w.uiA.volumeSliderMax == 650.0f);
    const float used = w.a.inv->volumeOccupied;
    CHECK(w.uiA.volumeSliderValue == used);
    CHECK(w.uiA.packedVolumeText == Localizer::Format("#autoLOC_8003412", {dotnet::FormatSingleF(used, 1), "650.0"}));
    CHECK_FALSE(w.uiA.volumeOverLimit);
    w.uiA.UpdateVolumeLimits(700.0f, 650.0f, true, false);
    CHECK(w.uiA.volumeOverLimit);
    w.uiA.UpdateVolumeLimits(700.0f, 650.0f, false, true);  // constructionOnly: the fixed text
    CHECK(w.uiA.packedVolumeText == Localizer::Format("#autoLOC_6002642"));
    w.uiA.UpdateMassLimits(0.0125f, 0.065f, false);
    CHECK(w.uiA.massText == Localizer::Format("#autoLOC_8003413", {"0.013", "0.065"}));  // ToString("0.000") of 0.0125f
    // the EVA kerbal's own limits include a mass limit
    Host kerbal(w.g, "kerbalEVA");
    UIPartActionInventory uiK;
    uiK.SetupPaw(*kerbal.inv, 2);
    CHECK(uiK.massLimitsActive);
    CHECK(uiK.massSliderMax == kerbal.inv->massLimit());
    // place-part buttons: a slot shows the button when it holds a deployable part and the module can place parts
    w.uiA.UpdatePlacePartIcons(true);
    CHECK(w.uiA.placePartIconActive.size() == 9);
    CHECK_FALSE(w.uiA.placePartIconActive[0]);  // a repair kit is not a ground-deployable part
    w.uiA.ResetPlacePartIcons(true);            // AbleToPlaceParts needs an EVA kerbal landed in flight: false here
    CHECK_FALSE(w.uiA.placePartIconActive[0]);
}

// UIPartActionInventorySlot.cs:65-340: clicking slots with and without a held part
TEST_CASE("WP-33 UIPartActionInventorySlot: pick up, store in an empty slot, stack, swap, refuse") {
    if (!GameAvailable()) return;
    Reset33 r;
    UiWorld w;
    LoadLocalizer();
    REQUIRE(w.a.inv->StoreCargoPartAtSlot(&w.P("evaRepairKit"), 0));
    REQUIRE(w.a.inv->UpdateStackAmountAtSlot(0, 3));
    REQUIRE(w.a.inv->StoreCargoPartAtSlot(&w.P("evaJetpack"), 1));
    UIPartActionControllerInventory& c = w.ctrl;
    // ---- pick up one of a stack of 3 (no modifier): the stack shrinks, the cursor holds one kit
    w.SlotA(0).SlotClicked();
    REQUIRE(c.CurrentCargoPart() != nullptr);
    CHECK(c.CurrentCargoPart()->partInfo->name == "evaRepairKit");
    CHECK(c.currentInventory == w.a.inv);
    CHECK(c.currentInventorySlotClicked == &w.SlotA(0));
    CHECK(UIPartActionControllerInventory::stackSize == 1);
    CHECK_FALSE(UIPartActionControllerInventory::heldPartIsStack);
    CHECK(w.a.inv->GetStackAmountAtSlot(0) == 2);
    CHECK(w.SlotA(0).selected);
    CHECK(c.controlLocks.count("CargoPartHeld") == 1);
    // ---- drop it into an empty slot of the other container: stored with quantity 1, cursor cleared
    w.SlotB(1).SlotClicked();
    CHECK(c.CurrentCargoPart() == nullptr);
    REQUIRE_FALSE(w.b.inv->IsSlotEmpty(1));
    CHECK(w.b.inv->storedParts[1]->partName == "evaRepairKit");
    CHECK(w.b.inv->GetStackAmountAtSlot(1) == 1);
    CHECK(c.currentInventorySlotClicked == nullptr);
    CHECK_FALSE(UIPartActionControllerInventory::heldPartIsStack);
    CHECK(w.uiB.slotPartIcon[1] == "evaRepairKit");
    CHECK(c.sfx.size() >= 2);
    // ---- pick up the whole stack with the modifier key, drop it onto the stack in B (room for 3): they merge up to the capacity of 4
    InventoryEnvironment::MODIFIER_KEY_DOWN = true;
    w.SlotA(0).SlotClicked();
    InventoryEnvironment::MODIFIER_KEY_DOWN = false;
    CHECK(UIPartActionControllerInventory::heldPartIsStack);
    CHECK(UIPartActionControllerInventory::stackSize == 2);
    CHECK(w.a.inv->IsSlotEmpty(0));  // EmptyStack
    w.SlotB(1).SlotClicked();
    CHECK(w.b.inv->GetStackAmountAtSlot(1) == 3);  // 1 + 2
    CHECK(c.CurrentCargoPart() == nullptr);
    CHECK(UIPartActionControllerInventory::stackSize == 0);
    CHECK_FALSE(UIPartActionControllerInventory::heldPartIsStack);
    // ---- swap: hold the jetpack, click the kit stack in B: B gets the jetpack, the cursor holds the whole kit stack
    w.SlotA(1).SlotClicked();
    REQUIRE(c.CurrentCargoPart() != nullptr);
    CHECK(c.CurrentCargoPart()->partInfo->name == "evaJetpack");
    CHECK(w.a.inv->IsSlotEmpty(1));
    w.SlotB(1).SlotClicked();
    REQUIRE_FALSE(w.b.inv->IsSlotEmpty(1));
    CHECK(w.b.inv->storedParts[1]->partName == "evaJetpack");
    REQUIRE(c.CurrentCargoPart() != nullptr);
    CHECK(c.CurrentCargoPart()->partInfo->name == "evaRepairKit");
    CHECK(UIPartActionControllerInventory::heldPartIsStack);
    CHECK(UIPartActionControllerInventory::stackSize == 3);
    CHECK(c.currentInventorySlotClicked == nullptr);
    // ---- the held stack goes back to an empty slot of A: the whole stack is stored with its quantity
    w.SlotA(5).SlotClicked();
    REQUIRE_FALSE(w.a.inv->IsSlotEmpty(5));
    CHECK(w.a.inv->GetStackAmountAtSlot(5) == 3);
    CHECK(c.CurrentCargoPart() == nullptr);
    CHECK_FALSE(UIPartActionControllerInventory::heldPartIsStack);
    CHECK(UIPartActionControllerInventory::stackSize == 0);
    // ---- an over-limit container refuses the part; a held part with attached children posts the message
    w.SlotA(5).SlotClicked();  // pick one kit up again
    REQUIRE(c.CurrentCargoPart() != nullptr);
    w.b.inv->volumeCapacityReached = true;
    w.SlotB(2).SlotClicked();
    CHECK(w.b.inv->IsSlotEmpty(2));            // nothing stored
    CHECK(c.CurrentCargoPart() != nullptr);    // still held
    w.b.inv->volumeCapacityReached = false;
    Part* held = c.CurrentCargoPart();
    auto child = w.g.New("evaChute");
    child->fields.persistentId = held->fields.persistentId + 1;
    held->children.push_back(child.get());
    InventoryEnvironment::screenMessages.clear();
    w.SlotB(2).SlotClicked();
    CHECK(w.b.inv->IsSlotEmpty(2));
    CHECK(InventoryEnvironment::screenMessages == std::vector<std::string>{Localizer::Format("#autoLOC_6005091")});
    held->children.clear();
    // ---- hovering shows the preview and leaving restores the limits
    w.SlotB(0).OnPointerEnter();
    CHECK(c.currentInventorySlotHovered == &w.SlotB(0));
    CHECK(w.b.inv->showPreview);
    w.b.inv->volumeCapacity = 12345.0f;
    w.SlotB(0).OnPointerExit();
    CHECK(c.currentInventorySlotHovered == nullptr);
    CHECK_FALSE(w.b.inv->showPreview);
    CHECK(w.b.inv->volumeCapacity == w.b.inv->volumeOccupied);  // RestoreLimitsDisplay
    // ---- returning the held part sends it back to the slot it came from
    c.ReturnHeldPart();
    CHECK(c.CurrentCargoPart() == nullptr);
    CHECK(w.a.inv->GetStackAmountAtSlot(5) == 3);
    CHECK(w.SlotA(0).CurrentStackedAmount() == 0);
}

// UIPartActionInventory.cs:342 / UIPartActionControllerInventory.cs:293-343 / :698-:760
TEST_CASE("WP-33 UIPartActionControllerInventory: held-part cache, construction events, parts created from the inventory") {
    if (!GameAvailable()) return;
    Reset33 r;
    UiWorld w;
    UIPartActionControllerInventory& c = w.ctrl;
    c.Start();
    CHECK(c.availableCargoParts.size() > 100);  // every cargo part (PartModelPurchased defaults to true)
    CHECK(std::find(c.availableCargoParts.begin(), c.availableCargoParts.end(), "evaJetpack") != c.availableCargoParts.end());
    CHECK(std::find(c.availableInventoryParts.begin(), c.availableInventoryParts.end(), "cargoContainer") != c.availableInventoryParts.end());
    // a second instance logs and is destroyed (Awake: `if (Instance != null) Destroy(this)`)
    {
        UIPartActionControllerInventory dup;
        CHECK(UIPartActionControllerInventory::Instance == &c);
    }
    CHECK(UIPartActionControllerInventory::Instance == &c);
    // the setter fires OnInventoryPartOnMouseChanged
    struct L {
        std::vector<Part*> v;
        void M(Part* p) { v.push_back(p); }
    } l;
    GameEvents::OnInventoryPartOnMouseChanged.Add(&l, &L::M);
    auto kit = w.g.New("evaRepairKit");
    c.SetCurrentCargoPart(kit.get());
    CHECK(c.CurrentModuleCargoPart() != nullptr);
    CHECK(l.v == std::vector<Part*>{kit.get()});
    // OnCargoPartConstructionEvent: only for available cargo parts; PartPicked / PartCreated hold, PartDropped resets
    c.OnCargoPartConstructionEvent(ConstructionEventType::PartCreated, kit.get());
    CHECK(c.CurrentCargoPart() == kit.get());
    c.currentInventory = w.a.inv;
    c.OnCargoPartConstructionEvent(ConstructionEventType::PartDropped, kit.get());
    CHECK(c.CurrentCargoPart() == nullptr);  // ResetInventoryCacheValues
    CHECK(c.currentInventory == nullptr);
    CHECK(l.v.back() == nullptr);
    auto notCargo = std::make_unique<Part>();
    notCargo->partInfo = &w.g.Info("fuelLine");
    CHECK(std::find(c.availableCargoParts.begin(), c.availableCargoParts.end(), "fuelLine") != c.availableCargoParts.end());
    GameEvents::OnInventoryPartOnMouseChanged.Remove(&l, &L::M);
    // CreatePartFromInventory: the part is rebuilt from the snapshot, hidden-module state set (flight: CARGO state)
    REQUIRE(w.a.inv->StoreCargoPartAtSlot(&w.P("evaJetpack"), 0));
    {
        SceneScope flight(GameScenes::FLIGHT);
        c.currentInventory = w.a.inv;
        std::unique_ptr<Part> made = c.CreatePartFromInventory(*w.a.inv->storedParts[0]->snapshot);
        REQUIRE(made != nullptr);
        CHECK(made->State() == PartStates::CARGO);
        CHECK(made->fields.ResumeState == static_cast<int32_t>(PartStates::CARGO));
        for (size_t i = 0; i < made->Modules().Count(); i++) CHECK_FALSE(made->Modules()[i]->isEnabled);
    }
    {
        std::unique_ptr<Part> made = c.CreatePartFromInventory(*w.a.inv->storedParts[0]->snapshot);  // editor scene: no CARGO state
        CHECK(made->State() != PartStates::CARGO);
    }
    // ToggleCargoPartsLight counts only outside the editor
    c.ToggleCargoPartsLight(1);
    CHECK(c.cargoInventoriesOpen == 0 + 0);  // the grids' InitializeSlots calls (editor scene) did not count either
    {
        SceneScope flight(GameScenes::FLIGHT);
        c.ToggleCargoPartsLight(1);
        c.ToggleCargoPartsLight(1);
        CHECK(c.cargoInventoriesOpen == 2);
        CHECK(c.enableCargoLight);
        c.ToggleCargoPartsLight(-2);
        CHECK_FALSE(c.enableCargoLight);
    }
}

// EVAConstructionModeController.cs:509-600 / 313 / 378 / 408 / 432: the inventory registry and panel state
TEST_CASE("WP-33 EVAConstructionModeController: inventory registry, range, panel open / close and events") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    PhysicsWorld phys;
    SceneScope scene(GameScenes::FLIGHT);
    LoadLocalizer();
    KerbalInventory::Install();
    UIPartActionControllerInventory ctrl;
    EVAConstructionModeController controller;
    EVAConstructionModeEditor editor;
    controller.evaEditor = &editor;
    controller.Start();
    CHECK(EVAConstructionModeController::Instance == &controller);
    Kerbals k;
    Vessel* active = k.Add(Vector3f(0.0f, 0.0f, 0.0f));
    FlightRegistry::ActiveVessel = active;
    // a base: container 2 m from the kerbal, another 30 m away, a command pod with a kerbal aboard
    auto near = g.New("cargoContainer");
    near->fields.persistentId = 101;
    near->partTransform.position = Vector3f(2.0f, 0.0f, 0.0f);
    auto far = g.New("smallCargoContainer");
    far->fields.persistentId = 102;
    far->partTransform.position = Vector3f(30.0f, 0.0f, 0.0f);
    auto pod = g.New("mk1pod.v2");
    pod->fields.persistentId = 103;
    pod->partTransform.position = Vector3f(4.0f, 0.0f, 0.0f);
    auto jeb = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Jebediah Kerman");
    jeb->persistentID = 777;
    pod->protoModuleCrew.push_back(jeb);
    auto base = BareVessel(VesselType::Base, Vector3f());
    for (Part* p : {near.get(), far.get(), pod.get()}) {
        base->parts.push_back(p);
        p->vessel = base.get();
    }
    base->rootPart = near.get();
    FlightRegistry::VesselsLoaded.push_back(base.get());
    controller.SearchForInventoryParts();
    // the container modules by part persistentId, and the crew member's inventory by the kerbal's persistentID
    auto& reg = controller.loadedModuleInventoryPart;
    CHECK(reg.size() >= 4);  // near, far, pod (mk1pod.v2 carries a ModuleInventoryPart) and the kerbal
    bool haveKerbal = false;
    for (const auto& kv : reg) haveKerbal |= kv.first == 777u;
    CHECK(haveKerbal);
    ModuleInventoryPart* jebInv = KerbalInventory::ModuleFor(*jeb);
    CHECK(jebInv->transformPosition.x == 4.0f);  // placed on the pod while the kerbal is aboard
    controller.SearchForInventoryParts();       // idempotent
    CHECK(controller.loadedModuleInventoryPart.size() == reg.size());
    // displayed inventories: inside EVA_INVENTORY_RANGE (5) of the active kerbal only
    controller.UpdateDisplayedInventories();
    CHECK(controller.displayedInventories.size() == 3);  // near (2 m), pod (4 m), the kerbal's (4 m)
    CHECK(near->FindModuleOfClass("ModuleInventoryPart") != nullptr);
    EVAConstructionModeController::InventoryDisplayItem* item = nullptr;
    CHECK(controller.TryGetDisplayedInventory(Inv(*near), item));
    CHECK(Inv(*near)->constructorModeInventory == item->uiInventory);
    CHECK_FALSE(controller.TryGetDisplayedInventory(Inv(*far), item));
    // moving away removes the display; coming closer adds it
    near->partTransform.position = Vector3f(9.0f, 0.0f, 0.0f);
    controller.UpdateDisplayedInventories();
    CHECK_FALSE(controller.TryGetDisplayedInventory(Inv(*near), item));
    far->partTransform.position = Vector3f(1.0f, 0.0f, 0.0f);
    controller.UpdateDisplayedInventories();
    CHECK(controller.TryGetDisplayedInventory(Inv(*far), item));
    // PartPicked: the picked part's display goes (RemoveInvDisplayItem) and all the grids lose their selection
    controller.OnEditorPartEvent(ConstructionEventType::PartPicked, far.get());
    CHECK_FALSE(controller.TryGetDisplayedInventory(Inv(*far), item));
    // panel state: opening the construction panel needs an EVA kerbal and no map / pause / action-group window
    int modeEvents = 0;
    struct M {
        int* n;
        void C(bool open) { *n += open ? 1 : -1; }
    } m{&modeEvents};
    GameEvents::OnEVAConstructionMode.Add(&m, &M::C);
    CHECK(controller.CanOpenConstructionPanel());
    CHECK_FALSE(controller.CanOpenCargoPanel());
    controller.hooks.mapIsEnabled = [] { return true; };
    CHECK_FALSE(controller.CanOpenConstructionPanel());
    CHECK_FALSE(controller.CanOpenCargoPanel());  // the map blocks both
    controller.hooks.mapIsEnabled = nullptr;
    controller.hooks.flightDriverPause = [] { return true; };
    CHECK_FALSE(controller.CanOpenConstructionPanel());
    CHECK(controller.CanOpenCargoPanel());  // flight, an active vessel, but not eligible for construction
    controller.hooks.flightDriverPause = nullptr;
    int enter = 0, exit = 0;
    controller.hooks.evaEnterExitConstruction = [&](bool e) { (e ? enter : exit)++; };
    controller.OpenConstructionPanel();
    CHECK(controller.IsOpen());
    CHECK(controller.panelMode == EVAConstructionModeController::PanelMode::Construction);
    CHECK(enter == 1);
    CHECK(modeEvents == 1);
    CHECK(controller.AssistingKerbalsNumberText == "0");
    controller.OpenConstructionPanel();  // already open
    CHECK(enter == 1);
    CHECK(EVAConstructionModeController::MovementRestricted() == false);
    auto kit = g.New("evaRepairKit");
    editor.SetSelectedPartForTest(kit.get());
    CHECK(EVAConstructionModeController::MovementRestricted());  // open and a part in hand
    editor.SetSelectedPartForTest(nullptr);
    controller.ClosePanel();
    CHECK_FALSE(controller.IsOpen());
    CHECK(exit == 1);
    CHECK(modeEvents == 0);
    CHECK(controller.loadedModuleInventoryPart.empty());
    CHECK(controller.displayedInventories.empty());
    // cargo panel: the same but fires OnEVACargoMode and keeps panelMode = Cargo
    controller.hooks.flightDriverPause = [] { return true; };
    controller.OpenCargoPanel();
    CHECK(controller.IsOpen());
    CHECK(controller.panelMode == EVAConstructionModeController::PanelMode::Cargo);
    controller.OnGameSceneLoadRequested(GameScenes::SPACECENTER);  // closes the open panel
    CHECK_FALSE(controller.IsOpen());
    controller.hooks.flightDriverPause = nullptr;
    // OnVesselChange: leaving construction mode on the previous kerbal; an open panel with an ineligible new vessel closes
    controller.OpenConstructionPanel();
    REQUIRE(controller.IsOpen());
    controller.hooks.activeEvaInConstructionMode = [] { return true; };
    FlightRegistry::ActiveVessel = base.get();  // not an EVA kerbal: CanOpenConstructionPanel false
    controller.OnVesselChange(base.get());
    CHECK_FALSE(controller.IsOpen());
    CHECK(controller.activeEVAVessel == nullptr);
    GameEvents::OnEVAConstructionMode.Remove(&m, &M::C);
    jeb.reset();
    KerbalInventory::ClearAll();
}

// InventoryPanelController.cs:108-300: the editor's cargo panel lists the ship's inventories and its crew's
TEST_CASE("WP-33 InventoryPanelController: ship and crew inventories, display order, open / close") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::EDITOR);
    KerbalInventory::Install();
    UIPartActionControllerInventory ctrl;
    InventoryPanelController panel;
    panel.Start();
    CHECK(InventoryPanelController::Instance == &panel);
    auto box = g.New("cargoContainer");
    auto tank = g.New("fuelTankSmall");
    ShipConstruct ship;
    ship.liveParts = {tank.get(), box.get()};
    auto jeb = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Jebediah Kerman");
    InventoryPanelController::shipManifestPresent = true;
    InventoryPanelController::shipManifestCrew = [&] { return std::vector<std::shared_ptr<ProtoCrewMember>>{jeb}; };
    // closed: nothing is displayed, Update does nothing
    panel.OnEditorShipModified(&ship);
    CHECK(panel.moduleInventories.size() == 3 - 1 + 0);  // the container and the kerbal (fuelTankSmall has no inventory module)
    panel.Update();
    CHECK(panel.displayedInventories.empty());
    // open the Cargo screen: Update builds the grids, kerbals first
    panel.OnEditorScreenChange(EditorScreen::Cargo);
    CHECK(panel.IsOpen());
    panel.Update();
    REQUIRE(panel.displayedInventories.size() == 2);
    REQUIRE(panel.siblingOrder.size() == 2);
    CHECK(panel.displayedInventories[static_cast<size_t>(panel.siblingOrder[0])].inventoryModule->kerbalMode());
    CHECK_FALSE(panel.displayedInventories[static_cast<size_t>(panel.siblingOrder[1])].inventoryModule->kerbalMode());
    // the kerbal leaves the ship: its grid goes at the next Update
    InventoryPanelController::shipManifestCrew = [] { return std::vector<std::shared_ptr<ProtoCrewMember>>{}; };
    panel.OnEditorShipModified(&ship);
    CHECK(panel.moduleInventories.size() == 1);
    panel.Update();
    CHECK(panel.displayedInventories.size() == 1);
    CHECK(panel.displayedInventories[0].inventoryModule == Inv(*box));
    // closing clears everything
    panel.OnEditorScreenChange(EditorScreen::Parts);
    CHECK_FALSE(panel.IsOpen());
    CHECK(panel.moduleInventories.empty());
    CHECK(panel.displayedInventories.empty());
    panel.OnEditorScreenChange(EditorScreen::Parts);  // unchanged
    jeb.reset();
    KerbalInventory::ClearAll();
}

// PartItemTransfer.cs:38-209
TEST_CASE("WP-33 PartItemTransfer: valid / semi-valid selection, selectors, dismissal paths") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::FLIGHT);
    LoadLocalizer();
    auto src = g.New("evaJetpack");
    auto tankA = g.New("RCSFuelTank");  // resource-carrying -> valid
    auto heavy = g.New("evaRepairKit");
    heavy->fields.mass = 2.0f;  // semi-valid (mass > 1)
    auto plain = g.New("evaChute");
    plain->Resources.Clear();
    auto tankB = g.New("RCSFuelTank");
    auto v = BareVessel(VesselType::Ship, Vector3f());
    for (Part* p : {src.get(), tankA.get(), heavy.get(), plain.get(), tankB.get()}) {
        v->parts.push_back(p);
        p->vessel = v.get();
    }
    std::vector<std::pair<PartItemTransfer::DismissAction, Part*>> dismissed;
    auto cb = [&](PartItemTransfer::DismissAction a, Part* p) { dismissed.emplace_back(a, p); };
    const size_t vesselChangeListeners = GameEvents::onVesselChange.Count();
    {
        Transfer t;
        t.Setup(*src, "#autoLOC_135736", "Fuel", "#autoLOC_135736", cb);
        CHECK(PartItemTransfer::Instance == &t);
        CHECK(t.type == Localizer::Format("#autoLOC_135736"));
        // the parts are visited from the last to the first: tankB, tankA (valid); heavy (semi-valid); the source and the plain part are neither
        REQUIRE(t.validParts.size() == 2);
        CHECK(t.validParts[0] == tankB.get());
        CHECK(t.validParts[1] == tankA.get());
        REQUIRE(t.semiValidParts.size() == 1);
        CHECK(t.semiValidParts[0] == heavy.get());
        CHECK(t.found == 21);
        // selectors: semi-valid (reverse), valid (reverse), then the source
        REQUIRE(t.partSelectors.size() == 4);
        CHECK(t.partSelectors[0] == std::make_pair(heavy.get(), 0));
        CHECK(t.partSelectors[1] == std::make_pair(tankA.get(), 1));
        CHECK(t.partSelectors[2] == std::make_pair(tankB.get(), 1));
        CHECK(t.partSelectors[3] == std::make_pair(src.get(), 2));
        CHECK(t.controlLockSet);
        CHECK(t.hooked == 1);
        CHECK(t.screenMessages.size() == 1);
        CHECK(GameEvents::onVesselChange.Count() == vesselChangeListeners + 1);
        // selecting: the source and semi-valid parts only warn; a valid part dismisses with ItemMoved
        t.OnSrcPartSelect(src.get());
        t.OnSemiValidPartSelect(heavy.get());
        CHECK(t.screenMessages.size() == 3);
        CHECK(t.screenMessages[1] == Localizer::Format("#autoLOC_135891"));
        CHECK(t.screenMessages[2] == Localizer::Format("#autoLOC_135736"));
        CHECK(dismissed.empty());
        // a second transfer while one runs is interrupted at once, the first stays
        {
            Transfer t2;
            std::vector<PartItemTransfer::DismissAction> d2;
            t2.Setup(*src, "#autoLOC_135736", "Fuel", "#autoLOC_135736", [&](PartItemTransfer::DismissAction a, Part*) { d2.push_back(a); });
            CHECK(d2 == std::vector<PartItemTransfer::DismissAction>{PartItemTransfer::DismissAction::Interrupted});
            CHECK(PartItemTransfer::Instance == &t);
            CHECK(t2.destroyed);
        }
        CHECK(PartItemTransfer::Instance == &t);
        t.OnPartSelect(tankA.get());
        REQUIRE(dismissed.size() == 1);
        CHECK(dismissed[0].first == PartItemTransfer::DismissAction::ItemMoved);
        CHECK(dismissed[0].second == tankA.get());
        CHECK(t.destroyed);
        CHECK(t.unhooked == 1);
        CHECK(PartItemTransfer::Instance == nullptr);
        CHECK(GameEvents::onVesselChange.Count() == vesselChangeListeners);
    }
    // interruptions: the source vessel modified / a situation change of it / any vessel change; Escape cancels
    auto run = [&](auto&& act) {
        dismissed.clear();
        Transfer t;
        t.Setup(*src, "#autoLOC_135736", "Fuel", "#autoLOC_135736", cb);
        act(t);
        return dismissed.size() == 1 ? dismissed[0].first : PartItemTransfer::DismissAction::ItemMoved;
    };
    CHECK(run([&](Transfer&) { GameEvents::onVesselWasModified.Fire(v.get()); }) == PartItemTransfer::DismissAction::Interrupted);
    CHECK(run([&](Transfer&) { GameEvents::onVesselWasModified.Fire(nullptr); }) == PartItemTransfer::DismissAction::ItemMoved);  // another vessel: no dismissal
    CHECK(run([&](Transfer&) {
              GameEvents::onVesselSituationChange.Fire(GameEvents::HostedFromToAction<Vessel*, Vessel_Situations>(v.get(), Vessel::Situations::LANDED,
                                                                                                                     Vessel::Situations::FLYING));
          }) == PartItemTransfer::DismissAction::Interrupted);
    CHECK(run([&](Transfer&) { GameEvents::onVesselChange.Fire(v.get()); }) == PartItemTransfer::DismissAction::Interrupted);
    CHECK(run([&](Transfer& t) { t.LateUpdate(true); }) == PartItemTransfer::DismissAction::Cancelled);
    CHECK(run([&](Transfer& t) { t.LateUpdate(false); }) == PartItemTransfer::DismissAction::ItemMoved);  // no dismissal
    // DismissActive interrupts the running one (and does nothing when none runs)
    PartItemTransfer::DismissActive();
    CHECK(run([&](Transfer&) { PartItemTransfer::DismissActive(); }) == PartItemTransfer::DismissAction::Interrupted);
    for (Part* p : {src.get(), tankA.get(), heavy.get(), plain.get(), tankB.get()}) p->vessel = nullptr;
}
