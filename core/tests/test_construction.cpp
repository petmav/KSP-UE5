// WP-33 unit tests: EVA construction (EVAConstructionModeEditor / ModeController / Util), PartItemTransfer and the inventory UI models
// (UIPartActionControllerInventory / InventorySlot / UIPartActionInventory / InventoryPanelController). Every expectation is derived from the
// decompiled C# (the derivation is in the comment above each check); the weight-limit numbers against the Python reference are in
// test_inventory.cpp.
#include "inventory_fixture.hpp"
#include "ksp/unity/quaternion_native.hpp"

using namespace ksp;
using namespace ksp::inv33;

// ConstructionEventType.cs:1 / ConstructionMode.cs:1 / IConstruction.cs:1 (ported by WP-40 / WP-13; the values are the C# declaration order)
TEST_CASE("WP-33 ConstructionEventType / ConstructionMode / IConstruction: declaration order and the Part permissions") {
    CHECK(static_cast<int>(ConstructionEventType::Unknown) == 0);
    CHECK(static_cast<int>(ConstructionEventType::PartCreated) == 1);
    CHECK(static_cast<int>(ConstructionEventType::PartDropped) == 2);
    CHECK(static_cast<int>(ConstructionEventType::PartPicked) == 3);
    CHECK(static_cast<int>(ConstructionEventType::PartDragging) == 4);
    CHECK(static_cast<int>(ConstructionEventType::PartAttached) == 5);
    CHECK(static_cast<int>(ConstructionEventType::PartDetached) == 6);
    CHECK(static_cast<int>(ConstructionEventType::PartDeleted) == 7);
    CHECK(static_cast<int>(ConstructionEventType::PartCopied) == 8);
    CHECK(static_cast<int>(ConstructionEventType::PartRootSelected) == 9);
    CHECK(static_cast<int>(ConstructionEventType::PartOffsetting) == 10);
    CHECK(static_cast<int>(ConstructionEventType::PartOffset) == 11);
    CHECK(static_cast<int>(ConstructionEventType::PartRotating) == 12);
    CHECK(static_cast<int>(ConstructionEventType::PartRotated) == 13);
    CHECK(static_cast<int>(ConstructionEventType::PartTweaked) == 14);
    CHECK(static_cast<int>(ConstructionEventType::PartSymmetryDeleted) == 15);
    CHECK(static_cast<int>(ConstructionEventType::PartOverInventoryGrid) == 16);
    CHECK(static_cast<int>(ConstructionEventType::PartPickedInInventoryGrid) == 17);
    CHECK(static_cast<int>(ConstructionEventType::PartDroppedInInventoryGrid) == 18);
    CHECK(static_cast<int>(ConstructionMode::Place) == 0);
    CHECK(static_cast<int>(ConstructionMode::Move) == 1);
    CHECK(static_cast<int>(ConstructionMode::Rotate) == 2);
    CHECK(static_cast<int>(ConstructionMode::Root) == 3);
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    // a part with no IConstruction module can always be detached / offset / rotated, unless a kerbal hangs on its ladder
    auto kit = g.New("evaRepairKit");
    CHECK(kit->PartCanBeDetached());
    CHECK(kit->PartCanBeOffset());
    CHECK(kit->PartCanBeRotated());
    kit->fields.hasKerbalOnLadder = true;
    CHECK_FALSE(kit->PartCanBeDetached());
    CHECK_FALSE(kit->PartCanBeOffset());
    CHECK_FALSE(kit->PartCanBeRotated());
}

// EVAConstructionModeEditor.cs:1265
TEST_CASE("WP-33 EVAConstructionModeEditor.UpdateAssistingKerbals: range, seated and engineer filters, the changed event") {
    Reset33 r;
    PhysicsWorld phys;
    SceneScope scene(GameScenes::FLIGHT);
    Kerbals k;
    Vessel* active = k.Add(Vector3f(0.0f, 0.0f, 0.0f));
    k.Add(Vector3f(4.0f, 0.0f, 0.0f));                  // in range, not an engineer
    k.Add(Vector3f(0.0f, 5.0f, 0.0f), true);            // seated: not counted
    k.Add(Vector3f(20.0f, 0.0f, 0.0f));                 // out of range
    k.Add(Vector3f(7.0f, 0.0f, 0.0f), false, 1);        // exactly at the combine range (<=): an engineer
    FlightRegistry::ActiveVessel = active;
    EVAConstructionModeController controller;
    EVAConstructionModeEditor editor;
    controller.evaEditor = &editor;
    EVAConstructionModeController::Instance = &controller;
    Weight w;
    const double base = PhysicsGlobals::ConstructionWeightLimit();
    const double per = PhysicsGlobals::ConstructionWeightLimitPerKerbalCombine();
    // any trait counts (EVA_CONSTRUCTION_COMBINE_NONENGINEERS default true)
    editor.UpdateAssistingKerbals();
    CHECK(editor.AssistingKerbals() == 2);
    CHECK(editor.CombinedConstructionWeightLimit() == base + 2.0 * per);
    CHECK(w.fired == 1);
    editor.UpdateAssistingKerbals();  // unchanged: no event
    CHECK(w.fired == 1);
    // engineers only
    InventoryEnvironment::EVA_CONSTRUCTION_COMBINE_NONENGINEERS = false;
    editor.UpdateAssistingKerbals();
    CHECK(editor.AssistingKerbals() == 1);
    CHECK(editor.CombinedConstructionWeightLimit() == base + per);
    CHECK(w.fired == 2);
    // combine disabled: 0 assistants, limit 0.0 (the weight test then uses the plain limit), event on change
    InventoryEnvironment::EVA_CONSTRUCTION_COMBINE_ENABLED = false;
    editor.UpdateAssistingKerbals();
    CHECK(editor.AssistingKerbals() == 0);
    CHECK(editor.CombinedConstructionWeightLimit() == 0.0);
    CHECK(w.fired == 3);
    editor.UpdateAssistingKerbals();
    CHECK(w.fired == 3);
    // no active vessel behaves like disabled
    InventoryEnvironment::EVA_CONSTRUCTION_COMBINE_ENABLED = true;
    FlightRegistry::ActiveVessel = nullptr;
    editor.UpdateAssistingKerbals();
    CHECK(editor.AssistingKerbals() == 0);
    CHECK(w.fired == 3);
}

// EVAConstructionModeEditor.cs:1666 / 1696
TEST_CASE("WP-33 EVAConstructionModeEditor.CanPartBeEdited: weight, detach, crew and children messages") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    PhysicsWorld phys;
    SceneScope scene(GameScenes::FLIGHT);
    LoadLocalizer();
    EVAConstructionModeEditor editor;
    auto kit = g.New("evaRepairKit");
    auto vessel = BareVessel(VesselType::Debris, Vector3f());
    vessel->Landed = true;
    vessel->gravityForPos = Vector3d(0.0, 0.0, 9.80665);
    kit->vessel = vessel.get();
    CHECK(editor.CanPartBeEdited(*kit, true));
    CHECK(editor.CanPartBeEdited(*kit, false));
    CHECK(editor.screenMessages.empty());
    // too heavy: the same message for both modes
    InventoryEnvironment::IgnoreEVAConstructionMassLimit = false;
    kit->fields.mass = 1.0f;  // 1 t -> 9806 N > 588.399 N
    CHECK_FALSE(editor.CanPartBeEdited(*kit, true));
    REQUIRE(editor.screenMessages.size() == 1);
    CHECK(editor.screenMessages[0] == Msg("#autoLOC_8012062", kit->partInfo->title));
    // ... unless the cheat is on
    InventoryEnvironment::IgnoreEVAConstructionMassLimit = true;
    CHECK(editor.CanPartBeEdited(*kit, true));
    InventoryEnvironment::IgnoreEVAConstructionMassLimit = false;
    kit->fields.mass = 0.005f;
    // PartCanBeDetached false: "cannot interact" (EditorLogicBase.cacheAutoLOC_6006095)
    editor.screenMessages.clear();
    kit->fields.hasKerbalOnLadder = true;
    CHECK_FALSE(editor.CanPartBeEdited(*kit, false));
    CHECK(editor.screenMessages == std::vector<std::string>{editor.cacheAutoLOC_6006095});
    CHECK(editor.CanPartBeEdited(*kit, true));  // weight-only check ignores it
    kit->fields.hasKerbalOnLadder = false;
    // crew aboard
    editor.screenMessages.clear();
    kit->protoModuleCrew.push_back(std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Bob Kerman"));
    CHECK_FALSE(editor.CanPartBeEdited(*kit, false));
    CHECK(editor.screenMessages == std::vector<std::string>{Msg("#autoLOC_6011175", kit->partInfo->title)});
    kit->protoModuleCrew.clear();
    // children / linked compound parts: the "has children" message
    editor.screenMessages.clear();
    auto child = g.New("evaJetpack");
    kit->children.push_back(child.get());
    CHECK_FALSE(editor.CanPartBeEdited(*kit, false));
    CHECK(editor.screenMessages == std::vector<std::string>{Msg("#autoLOC_6006096", kit->partInfo->title)});
    kit->children.clear();
    kit->vessel = nullptr;
}

// EVAConstructionModeEditor.cs:309 / 256
TEST_CASE("WP-33 EVAConstructionModeEditor: Place / Move / Rotate transitions, gizmos, held part") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    PhysicsWorld phys;
    SceneScope scene(GameScenes::FLIGHT);
    UIPartActionControllerInventory ctrl;
    EVAConstructionModeController controller;
    EVAConstructionModeEditor editor;
    controller.evaEditor = &editor;
    EVAConstructionModeController::Instance = &controller;
    auto kit = g.New("evaRepairKit");
    auto holder = g.New("cargoContainer");  // DemoteToPhysicslessPart needs a parent
    kit->parent = holder.get();
    editor.SetSelectedPartForTest(kit.get());
    // same mode: nothing
    editor.OnConstructionModeChanged(ConstructionMode::Place);
    CHECK(editor.EVAConstructionMode() == ConstructionMode::Place);
    CHECK(editor.EVAConstructionMode() == ConstructionMode::Place);
    // Place -> Move: the part is demoted to a physicsless part, the offset gizmo is attached when PartCanBeOffset, the coord button shows
    CHECK(kit->fields.physicalSignificance == static_cast<int32_t>(Part_PhysicalSignificance::FULL));
    editor.OnConstructionModeChanged(ConstructionMode::Move);
    CHECK(editor.EVAConstructionMode() == ConstructionMode::Move);
    CHECK(kit->fields.physicalSignificance == static_cast<int32_t>(Part_PhysicalSignificance::NONE));
    CHECK(editor.gizmoOffsetActive);
    CHECK_FALSE(editor.gizmoRotateActive);
    CHECK(editor.coordSpaceButtonActive);
    // Move -> Rotate: the rotate gizmo replaces the offset gizmo (PartCanBeRotated asks CanBeOffset, sic)
    editor.OnConstructionModeChanged(ConstructionMode::Rotate);
    CHECK(editor.gizmoRotateActive);
    CHECK_FALSE(editor.gizmoOffsetActive);
    CHECK(editor.gizmoAttRotate.w == kit->fields.attRotation.w);
    // Rotate -> Place: previousPhysicalSignificance != physicalSignificance restarts the physics part; selection and root are cleared
    kit->fields.previousPhysicalSignificance = static_cast<int32_t>(Part_PhysicalSignificance::FULL);
    editor.OnConstructionModeChanged(ConstructionMode::Place);
    CHECK(editor.SelectedPart() == nullptr);
    CHECK(editor.RootPart() == nullptr);
    CHECK_FALSE(editor.gizmoRotateActive);
    CHECK_FALSE(editor.gizmoOffsetActive);
    CHECK_FALSE(editor.coordSpaceButtonActive);
    CHECK(editor.EVAConstructionMode() == ConstructionMode::Place);
    // a part that cannot be offset: the gizmo is refused with the "cannot interact" message
    kit->fields.hasKerbalOnLadder = true;
    editor.SetSelectedPartForTest(kit.get());
    editor.OnConstructionModeChanged(ConstructionMode::Move);
    CHECK_FALSE(editor.gizmoOffsetActive);
    CHECK(editor.screenMessages.empty());  // ToggleOffsetGizmo(false) when PartCanBeOffset is false: no message (CannotInteract only on activate)
    kit->fields.hasKerbalOnLadder = false;
    // ChangeCoordSpace: offset gizmo first, then rotate; each flips Self <-> World with its text and message
    editor.OnConstructionModeChanged(ConstructionMode::Rotate);
    editor.screenMessages.clear();
    editor.ChangeCoordSpace();
    CHECK(editor.gizmoRotateSpace == UnityEngine::Space::World);
    CHECK(editor.coordSpaceText == "#autoLOC_6001217");
    CHECK(editor.screenMessages == std::vector<std::string>{"#autoLOC_6001223"});
    editor.ChangeCoordSpace();
    CHECK(editor.gizmoRotateSpace == UnityEngine::Space::Self);
    CHECK(editor.screenMessages.back() == "#autoLOC_6001224");
    // SnapButton toggles VAB_USE_ANGLE_SNAP and fires onEditorSnapModeChange
    struct Snap {
        std::vector<bool> v;
        void S(bool b) { v.push_back(b); }
    } snap;
    GameEvents::onEditorSnapModeChange.Add(&snap, &Snap::S);
    editor.SnapButton();
    editor.SnapInputUpdate(true);
    editor.SnapInputUpdate(false);
    GameEvents::onEditorSnapModeChange.Remove(&snap, &Snap::S);
    CHECK(snap.v == std::vector<bool>{true, false});
    CHECK_FALSE(InventoryEnvironment::VAB_USE_ANGLE_SNAP);
}

// EVAConstructionModeEditor.cs:256 (OnCurrentMousePartChanged), 1629 (DestroyHeldPart), 1572-1600 (colliders)
TEST_CASE("WP-33 EVAConstructionModeEditor.OnCurrentMousePartChanged: cargo parts are selected, others destroy the held part") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    PhysicsWorld phys;
    SceneScope scene(GameScenes::FLIGHT);
    UIPartActionControllerInventory ctrl;
    EVAConstructionModeController controller;
    EVAConstructionModeEditor editor;
    controller.evaEditor = &editor;
    EVAConstructionModeController::Instance = &controller;
    editor.colliderTriggers = [](Part*) { return std::vector<bool>{false, true, false}; };
    auto kit = g.New("evaRepairKit");
    auto tank = std::make_unique<Part>();  // no ModuleCargoPart
    GameEvents::OnInventoryPartOnMouseChanged.Add(&editor, &EVAConstructionModeEditor::OnCurrentMousePartChanged);
    ctrl.SetCurrentCargoPart(kit.get());  // fires OnInventoryPartOnMouseChanged
    CHECK(editor.SelectedPart() == kit.get());
    CHECK(ctrl.currentPartIsAttachable);
    // Place mode: colliders fetched and forced to triggers (isTrigger | previous), gizmos off
    REQUIRE(editor.selectedPartColliders.size() == 3);
    CHECK(editor.selectedPartColliders[0].current);
    CHECK(editor.selectedPartColliders[1].current);
    editor.ToggleCollidersTrigger(false);
    CHECK_FALSE(editor.selectedPartColliders[0].current);
    CHECK(editor.selectedPartColliders[1].current);  // keeps its own trigger flag
    // a non cargo part clears the selection (DestroyHeldPart) and the attachable flag
    ctrl.SetCurrentCargoPart(tank.get());
    CHECK(editor.SelectedPart() == nullptr);
    CHECK_FALSE(ctrl.currentPartIsAttachable);
    // null (nothing under the cursor) behaves the same
    ctrl.SetCurrentCargoPart(kit.get());
    CHECK(editor.SelectedPart() == kit.get());
    ctrl.SetCurrentCargoPart(nullptr);
    CHECK(editor.SelectedPart() == nullptr);
    // not in flight: ignored
    {
        SceneScope editorScene(GameScenes::EDITOR);
        ctrl.SetCurrentCargoPart(kit.get());
        CHECK(editor.SelectedPart() == nullptr);
    }
    GameEvents::OnInventoryPartOnMouseChanged.Remove(&editor, &EVAConstructionModeEditor::OnCurrentMousePartChanged);
}

// EVAConstructionModeEditor.cs:933 CheckAttach: the stack branch (node match + pose), the surface branch (pose, refusals)
TEST_CASE("WP-33 EVAConstructionModeEditor.CheckAttach: stack node matching and pose, surface attach, refusals") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    PhysicsWorld phys;
    SceneScope scene(GameScenes::FLIGHT);
    UIPartActionControllerInventory ctrl;
    EVAConstructionModeController controller;
    EVAConstructionModeEditor editor;
    controller.evaEditor = &editor;
    EVAConstructionModeController::Instance = &controller;
    Kerbals k;
    Vessel* active = k.Add(Vector3f(0.0f, 0.0f, 0.0f));
    FlightRegistry::ActiveVessel = active;
    EVAConstructionEnvironment::activeVesselWorldPos = [] { return Vector3f(0.0f, 0.0f, 0.0f); };
    // the vessel to attach to: one cargo container 3 m from the kerbal
    auto parent = g.New("cargoContainer");
    parent->partTransform.position = Vector3f(3.0f, 0.0f, 0.0f);
    auto target = BareVessel(VesselType::Debris, parent->partTransform.position);
    target->parts.push_back(parent.get());
    target->rootPart = parent.get();
    parent->vessel = target.get();
    FlightRegistry::VesselsLoaded.push_back(target.get());
    // the part in hand
    auto held = g.New("cargoContainer");
    held->partTransform.position = Vector3f(3.0f, 3.0f, 0.0f);
    editor.SetSelectedPartForTest(held.get());
    AttachNode* top = parent->FindAttachNode("top");
    AttachNode* bottom = held->FindAttachNode("bottom");
    REQUIRE(top != nullptr);
    REQUIRE(bottom != nullptr);
    // node icons exist for the nodes of the parts in range (ProcessAttachNodes)
    editor.ProcessAttachNodes();
    CHECK(editor.attachVessels.size() == 2);  // the kerbal's own vessel (in range) and the target
    int probes = 0;
    EVAConstructionEnvironment::nodeIconUnderCursor = [&](Part& p, AttachNode& n, const Vector3f& w) {
        probes++;
        return &p == parent.get() && &n == top && w.x == held->partTransform.position.x && w.y == held->partTransform.position.y + bottom->position.y;
    };
    Attachment a = editor.CheckAttach(held.get());
    CHECK(a.possible);
    CHECK(a.mode == AttachModes::STACK);
    CHECK(a.potentialParent == parent.get());
    CHECK(a.caller == held.get());
    CHECK(a.callerPartNode == bottom);
    CHECK(a.otherPartNode == top);
    CHECK(held->fields.attachMode == static_cast<int32_t>(AttachModes::STACK));
    // axis aligned: FromToRotation(-y, -(+y)) = identity; position = parentPos + (top.position + top.offset) - bottom.position
    CHECK(a.rotation.x == 0.0f);
    CHECK(a.rotation.y == 0.0f);
    CHECK(a.rotation.z == 0.0f);
    CHECK(a.rotation.w == 1.0f);
    const Vector3f expect = parent->partTransform.position + (top->position + top->offset) - bottom->position;
    CHECK(Bits(a.position.x) == Bits(expect.x));
    CHECK(Bits(a.position.y) == Bits(expect.y));
    CHECK(Bits(a.position.z) == Bits(expect.z));
    CHECK(probes >= 1);
    // a node that already has a part attached is skipped
    // (the held part's top node would otherwise mate the parent's bottom node)
    parent->FindAttachNode("top")->attachedPart = held.get();
    parent->FindAttachNode("bottom")->attachedPart = held.get();
    Attachment none = editor.CheckAttach(held.get());
    CHECK_FALSE(none.possible);
    CHECK(none.potentialParent == nullptr);
    parent->FindAttachNode("top")->attachedPart = nullptr;
    parent->FindAttachNode("bottom")->attachedPart = nullptr;
    // the cursor is not over the node icon
    EVAConstructionEnvironment::nodeIconUnderCursor = [](Part&, AttachNode&, const Vector3f&) { return false; };
    CHECK_FALSE(editor.CheckAttach(held.get()).possible);
    // non-strict orientation (cheat): |dot| > 0.7 matches parallel nodes too (the sign flips the target)
    EVAConstructionEnvironment::nodeIconUnderCursor = [](Part&, AttachNode&, const Vector3f&) { return true; };
    // only held.bottom and parent.top are free; turned over, the held bottom faces +y like the parent's top: dot = +1
    held->FindAttachNode("top")->attachedPart = parent.get();
    parent->FindAttachNode("bottom")->attachedPart = held.get();
    held->partTransform.rotation = UnityNative::AngleAxis(180.0f, Vector3f::right());
    CHECK_FALSE(editor.CheckAttach(held.get()).possible);
    EditorEnvironment::Current().nonStrictAttachmentOrientation = true;
    Attachment ns = editor.CheckAttach(held.get());
    EditorEnvironment::Current().nonStrictAttachmentOrientation = false;
    CHECK(ns.possible);
    CHECK(ns.callerPartNode == bottom);
    CHECK(ns.otherPartNode == top);
    held->partTransform.rotation = Quaternionf::identity();
    held->FindAttachNode("top")->attachedPart = nullptr;
    parent->FindAttachNode("bottom")->attachedPart = nullptr;
    // surface attach: the nearest non-CARGO part under the ray; pose = LookRotation(normal, up) * LookRotation(srfNode.orientation, up)
    held->fields.attachRules = AttachRules{false, true, false, false, false, false, false, false};  // srfAttach only
    held->srfAttachNode->orientation = Vector3f(0.0f, 1.0f, 0.0f);
    held->srfAttachNode->position = Vector3f(0.0f, -0.1f, 0.0f);
    parent->fields.attachRules = AttachRules{true, false, true, true, false, false, false, false};
    EditorSurfaceHit hit;
    hit.part = parent.get();
    hit.point = Vector3f(3.5f, 0.2f, 0.0f);
    hit.normal = Vector3f(1.0f, 0.0f, 0.0f);
    hit.colliderName = "body";
    auto cargoPart = g.New("evaJetpack");
    cargoPart->set_State(PartStates::CARGO);  // PartStates.CARGO parts are not valid targets
    EditorSurfaceHit cargoHit;
    cargoHit.part = cargoPart.get();
    cargoHit.point = Vector3f(0.0f, 0.0f, 0.0f);
    EVAConstructionEnvironment::surfaceRaycastAll = [&] { return std::vector<EditorSurfaceHit>{cargoHit, hit}; };
    Attachment s = editor.CheckAttach(held.get());
    CHECK(s.possible);
    CHECK(s.mode == AttachModes::SRF_ATTACH);
    CHECK(s.potentialParent == parent.get());
    CHECK(s.callerPartNode == held->srfAttachNode.get());
    CHECK(s.otherPartNode == nullptr);
    CHECK(held->srfAttachNode->srfAttachMeshName == "body");
    const Quaternionf rot = UnityNative::LookRotation(hit.normal, parent->partTransform.rotation * Vector3f::up()) *
                            UnityNative::LookRotation(held->srfAttachNode->orientation, Vector3f::up());
    CHECK(Bits(s.rotation.x) == Bits(rot.x));
    CHECK(Bits(s.rotation.w) == Bits(rot.w));
    const Vector3f pos = hit.point - rot * held->fields.attRotation * held->srfAttachNode->position;
    CHECK(Bits(s.position.x) == Bits(pos.x));
    CHECK(Bits(s.position.y) == Bits(pos.y));
    // the NoAttach tag and a fairing panel refuse the attachment; a part that forbids surface attach too
    hit.noAttachTag = true;
    CHECK_FALSE(editor.CheckAttach(held.get()).possible);
    hit.noAttachTag = false;
    hit.fairingPanelCollider = true;
    CHECK_FALSE(editor.CheckAttach(held.get()).possible);
    hit.fairingPanelCollider = false;
    parent->fields.attachRules = AttachRules{true, false, true, false, false, false, false, false};
    CHECK_FALSE(editor.CheckAttach(held.get()).possible);
    // the modifier key without a flag part suppresses the surface search
    parent->fields.attachRules = AttachRules{true, false, true, true, false, false, false, false};
    InventoryEnvironment::MODIFIER_KEY_DOWN = true;
    CHECK_FALSE(editor.CheckAttach(held.get()).possible);
    InventoryEnvironment::MODIFIER_KEY_DOWN = false;
    CHECK(editor.CheckAttach(held.get()).possible);
    // the angle snap: rotation about the parent's axis rounded to srfAttachAngleSnap (15 deg)
    InventoryEnvironment::VAB_USE_ANGLE_SNAP = true;
    EditorEnvironment::Current().findPartSurface = [](Part&, const Vector3f&, const Vector3f&, const Vector3f&, const EditorSurfaceHit& h, Vector3f& srfNormal) {
        srfNormal = h.normal;
        return 0.5f;
    };
    Attachment snap = editor.CheckAttach(held.get());
    InventoryEnvironment::VAB_USE_ANGLE_SNAP = false;
    CHECK(snap.possible);
    CHECK(snap.potentialParent == parent.get());
    EditorEnvironment::Current().findPartSurface = nullptr;
    EVAConstructionEnvironment::surfaceRaycastAll = nullptr;
}
