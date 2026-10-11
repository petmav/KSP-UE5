// WP-33 unit tests: ModuleCargoPart settle / weld sparks / container highlight and the EVA construction attach / detach / backup / restore / gizmo
// paths. Every expectation is derived from the decompiled C# (the derivation is in the comment above each check).
#include "inventory_fixture.hpp"

#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/unity/quaternion_native.hpp"

using namespace ksp;
using namespace ksp::inv33;

namespace {

struct Kerbin33b {
    Game15 g;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    CoroutineScheduler sched;
    Kerbin33b() {
        pg.physicsDatabaseFilename = ksp::testing::GameRoot() + "/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        SaveEnvironment::Current().localBodies = sys->SpawnOrder();
    }
    ~Kerbin33b() {
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
    }
    CelestialBody& Body(const char* n) { return *sys->GetBodyByName(n); }
};

std::unique_ptr<Vessel> VesselAround(CelestialBody& body, VesselType type = VesselType::Debris) {
    auto v = BareVessel(type, Vector3f());
    v->orbitDriver = std::make_unique<OrbitDriver>();
    v->orbitDriver->orbit->referenceBody = &body;
    return v;
}

// advance the coroutine scheduler by `seconds` in whole-second frames
void Run(CoroutineScheduler& s, double seconds) {
    for (double t = 0.0; t < seconds; t += 1.0) {
        s.AdvanceFrame(1.0f);
        s.RunDelayedDynamicFrameRate();
    }
}

}  // namespace

// ModuleCargoPart.cs:80-96 / 206-247: the settle coroutine
TEST_CASE("WP-33 ModuleCargoPart.MakePartSettle: low gravity bodies get a heavy settling mass for kinematicDelay seconds") {
    if (!GameAvailable()) return;
    Reset33 r;
    Kerbin33b w;
    SceneScope scene(GameScenes::FLIGHT);
    InventoryEnvironment::scheduler = &w.sched;
    struct Case {
        const char* body;
        float settling;  // 0 = unchanged
    };
    // GeeASL < 0.8 -> 20 t; < 1.1 -> 4 t; otherwise the mass is left alone
    for (const Case& c : {Case{"Mun", 20.0f}, Case{"Kerbin", 4.0f}, Case{"Eve", 0.0f}}) {
        CHECK_MESSAGE(w.Body(c.body).GeeASL > 0.0, c.body);
        auto kit = w.g.New("evaRepairKit");
        auto* cargo = Mod<ModuleCargoPart>(*kit, "ModuleCargoPart");
        auto v = VesselAround(w.Body(c.body));
        v->parts.push_back(kit.get());
        kit->vessel = v.get();
        const float original = kit->fields.prefabMass;
        cargo->settleCoroutine = w.sched.StartCoroutine(cargo->MakePartSettle(false));
        if (c.settling > 0.0f) {
            CHECK(cargo->beingSettled());
            CHECK(kit->fields.prefabMass == c.settling);
            CHECK(cargo->preSettlingPrefabMass == original);
        } else {
            CHECK_FALSE(cargo->beingSettled());
            CHECK(kit->fields.prefabMass == original);
        }
        Run(w.sched, 2.0);  // kinematicDelay is 3 s: still settling
        if (c.settling > 0.0f) CHECK(kit->fields.prefabMass == c.settling);
        Run(w.sched, 2.0);
        CHECK(kit->fields.prefabMass == original);
        CHECK_FALSE(cargo->beingSettled());
        CHECK(cargo->preSettlingPrefabMass == 0.0f);
        kit->vessel = nullptr;
    }
    // waitForMotionStop: until the surface velocity is below placementMaxRivotVelocity or kinematicDelay * 10 s have passed
    {
        auto kit = w.g.New("evaRepairKit");
        auto* cargo = Mod<ModuleCargoPart>(*kit, "ModuleCargoPart");
        auto v = VesselAround(w.Body("Mun"));
        v->parts.push_back(kit.get());
        kit->vessel = v.get();
        double ut = 0.0;
        InventoryEnvironment::universalTime = [&] { return ut; };
        v->srf_velocity = Vector3d(0.0, 0.0, 5.0);
        cargo->settleCoroutine = w.sched.StartCoroutine(cargo->MakePartSettle(true));
        const float original = cargo->preSettlingPrefabMass;
        Run(w.sched, 4.0);  // past kinematicDelay: waiting for the vessel to stop
        CHECK(kit->fields.prefabMass == 20.0f);
        ut = 31.0;  // more than kinematicDelay * 10 = 30 s later: gives up waiting
        Run(w.sched, 1.0);
        CHECK(kit->fields.prefabMass == original);
        // a stopped vessel releases at once
        cargo->settleCoroutine = w.sched.StartCoroutine(cargo->MakePartSettle(true));
        v->srf_velocity = Vector3d(0.0, 0.0, 0.001);
        Run(w.sched, 4.0);
        CHECK(kit->fields.prefabMass == original);
        kit->vessel = nullptr;
    }
    // OnPartUnpack: a lone landed part settles; a part that was being attached just clears the flag
    {
        auto kit = w.g.New("evaRepairKit");
        auto* cargo = Mod<ModuleCargoPart>(*kit, "ModuleCargoPart");
        auto v = VesselAround(w.Body("Kerbin"));
        v->parts.push_back(kit.get());
        kit->vessel = v.get();
        v->situation = Vessel::Situations::LANDED;
        cargo->Set<bool>("beingAttached", true);
        cargo->OnPartUnpack();
        CHECK_FALSE(cargo->beingAttached());
        CHECK(cargo->settleCoroutine == nullptr);
        cargo->OnPartUnpack();
        CHECK(cargo->settleCoroutine != nullptr);
        CHECK(kit->fields.prefabMass == 4.0f);
        Run(w.sched, 4.0);
        // two parts and not settling: nothing starts
        auto other = w.g.New("evaJetpack");
        v->parts.push_back(other.get());
        auto* cargo2 = Mod<ModuleCargoPart>(*other, "ModuleCargoPart");
        other->vessel = v.get();
        cargo2->OnPartUnpack();
        CHECK(cargo2->settleCoroutine == nullptr);
        cargo2->Set<bool>("beingSettled", true);
        cargo2->OnPartUnpack();
        CHECK(cargo2->settleCoroutine != nullptr);
        Run(w.sched, 4.0);
        other->vessel = nullptr;
        kit->vessel = nullptr;
    }
    InventoryEnvironment::universalTime = nullptr;
}

// ModuleCargoPart.cs:229-246
TEST_CASE("WP-33 ModuleCargoPart: welding sparks follow the last attached part") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    SceneScope scene(GameScenes::FLIGHT);
    EVAConstructionModeController controller;
    EVAConstructionModeEditor editor;
    controller.evaEditor = &editor;
    EVAConstructionModeController::Instance = &controller;
    auto kit = g.New("evaRepairKit");
    auto other = g.New("evaJetpack");
    kit->fields.persistentId = 61;
    other->fields.persistentId = 62;
    auto* cargo = Mod<ModuleCargoPart>(*kit, "ModuleCargoPart");
    auto* cargo2 = Mod<ModuleCargoPart>(*other, "ModuleCargoPart");
    std::vector<std::pair<int, float>> played;
    int stopped = 0;
    InventoryEnvironment::weldSparkClipCount = 3;
    InventoryEnvironment::randomRange = [](int lo, int hi) {
        CHECK(lo == 0);
        CHECK(hi == 3);
        return 2;
    };
    InventoryEnvironment::weldStartDelay = [](KerbalEVA*) { return 0.75f; };
    InventoryEnvironment::playWeldSparks = [&](ModuleCargoPart&, int clip, float delay) { played.emplace_back(clip, delay); };
    InventoryEnvironment::stopWeldSparks = [&](ModuleCargoPart&) { stopped++; };
    editor.lastAttachedPart = kit.get();
    cargo2->OnWeldStart(nullptr);  // not the welded part
    CHECK(played.empty());
    cargo->OnWeldStart(nullptr);
    REQUIRE(played.size() == 1);
    CHECK(played[0].first == 2);
    CHECK(played[0].second == 0.75f);
    cargo->OnWeldStart(nullptr);  // already playing
    CHECK(played.size() == 1);
    cargo2->OnWeldFinish(nullptr);
    CHECK(stopped == 0);
    cargo->OnWeldFinish(nullptr);
    CHECK(stopped == 1);
    cargo->OnWeldFinish(nullptr);
    CHECK(stopped == 1);
    // the events route to the module when it is started in flight
    cargo->OnStart(StartState::Landed);
    struct KE {
        KerbalEVA* k = nullptr;
    };
    GameEvents::OnEVAConstructionWeldStart.Fire(nullptr);
    CHECK(played.size() == 2);
    GameEvents::OnEVAConstructionWeldFinish.Fire(nullptr);
    CHECK(stopped == 2);
    cargo->OnDestroy();
    GameEvents::OnEVAConstructionWeldStart.Fire(nullptr);
    CHECK(played.size() == 2);
}

// ModuleInventoryPart.cs:1223-1299: containers glow while a cargo part is held and the kerbal is in range
TEST_CASE("WP-33 ModuleInventoryPart.OnUpdate: container highlight decision in flight") {
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
    FlightRegistry::ActiveVessel = k.Add(Vector3f(0.0f, 0.0f, 0.0f));
    auto box = g.New("cargoContainer");
    ModuleInventoryPart* inv = Inv(*box);
    inv->OnStart(StartState::Landed);
    box->partTransform.position = Vector3f(3.0f, 0.0f, 0.0f);
    auto kit = g.New("evaRepairKit");  // not stack / surface attachable
    // nothing held and no highlight: nothing happens
    inv->OnUpdate();
    CHECK_FALSE(inv->containerHighlight);
    CHECK(inv->highlightLog.empty());
    // a cargo part is held and the container is within EVA_INVENTORY_RANGE (5): highlighted (the construction panel is closed)
    ctrl.SetCurrentCargoPart(kit.get());
    inv->OnUpdate();
    CHECK(inv->containerHighlight);
    CHECK(inv->partHighlightActive);
    CHECK(inv->distanceToVessel == 3.0f);
    CHECK(inv->highlightLog == std::vector<std::string>{"color:inventoryContainer", "highlight:on"});
    // out of range: the highlight goes
    box->partTransform.position = Vector3f(6.0f, 0.0f, 0.0f);
    inv->highlightLog.clear();
    inv->OnUpdate();
    CHECK_FALSE(inv->containerHighlight);
    CHECK_FALSE(inv->partHighlightActive);
    CHECK(inv->highlightLog == std::vector<std::string>{"color:default", "highlight:off"});
    // the part is dropped while the container glows: it stops glowing
    box->partTransform.position = Vector3f(3.0f, 0.0f, 0.0f);
    inv->OnUpdate();
    REQUIRE(inv->containerHighlight);
    ctrl.SetCurrentCargoPart(nullptr);
    inv->highlightLog.clear();
    inv->OnUpdate();
    CHECK_FALSE(inv->containerHighlight);
    CHECK(inv->highlightLog == std::vector<std::string>{"color:default", "highlight:off"});
    // the construction panel open in Construction mode: only parts that are neither stack nor surface attachable light the containers
    ctrl.SetCurrentCargoPart(kit.get());
    controller.panelMode = EVAConstructionModeController::PanelMode::Construction;
    controller.hooks.mapIsEnabled = nullptr;
    controller.OpenConstructionPanel();
    REQUIRE(controller.IsOpen());
    inv->OnUpdate();
    CHECK(inv->containerHighlight);  // kit: not attachable
    ctrl.SetCurrentCargoPart(nullptr);
    inv->OnUpdate();
    kit->fields.attachRules = AttachRules{false, false, true, false, false, false, false, false};  // allowStack
    ctrl.SetCurrentCargoPart(kit.get());
    inv->OnUpdate();
    CHECK_FALSE(inv->containerHighlight);  // flag = false: the highlight is left untouched (off)
    controller.ClosePanel();
    // without the controller the flight branch dereferences it (the `Instance == null || Instance != null` quirk)
    EVAConstructionModeController::Instance = nullptr;
    inv->containerHighlight = false;
    CHECK_THROWS_AS(inv->OnUpdate(), NullReferenceException);
    // no active vessel: NullReferenceException (vesselTransform)
    EVAConstructionModeController::Instance = &controller;
    FlightRegistry::ActiveVessel = nullptr;
    CHECK_THROWS_AS(inv->OnUpdate(), NullReferenceException);
}

// EVAConstructionModeEditor.cs:650-:765 / 781-923 / 1601-1628: attach the held part to a vessel, detach it again, restore the backup
TEST_CASE("WP-33 EVAConstructionModeEditor: AttachInput / AttachPart, DetachInput / DetachPart / BackupPart, OnConstructionMode restore") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    PhysicsWorld phys;
    SceneScope scene(GameScenes::FLIGHT);
    LoadLocalizer();
    UIPartActionControllerInventory ctrl;
    EVAConstructionModeController controller;
    EVAConstructionModeEditor editor;
    controller.evaEditor = &editor;
    EVAConstructionModeController::Instance = &controller;
    Kerbals k;
    FlightRegistry::ActiveVessel = k.Add(Vector3f(0.0f, 0.0f, 0.0f));
    EVAConstructionEnvironment::activeVesselWorldPos = [] { return Vector3f(0.0f, 0.0f, 0.0f); };
    std::vector<std::string> welds;
    EVAConstructionEnvironment::weld = [&](Part& p) { welds.push_back(p.gameObjectName); };
    PartEvents events;
    struct Att {
        std::vector<std::pair<Vessel*, Part*>> attached, detached;
        void A(Vessel* v, Part* p) { attached.emplace_back(v, p); }
        void D(Vessel* v, Part* p) { detached.emplace_back(v, p); }
    } att;
    GameEvents::OnEVAConstructionModePartAttached.Add(&att, &Att::A);
    GameEvents::OnEVAConstructionModePartDetached.Add(&att, &Att::D);
    // the target vessel: a cargo container
    auto parent = g.New("cargoContainer");
    parent->partTransform.position = Vector3f(3.0f, 0.0f, 0.0f);
    parent->fields.persistentId = 201;
    auto target = BareVessel(VesselType::Debris, parent->partTransform.position);
    target->parts.push_back(parent.get());
    target->rootPart = parent.get();
    parent->vessel = target.get();
    FlightRegistry::VesselsLoaded.push_back(target.get());
    // the held part: a second container rebuilt from a snapshot, as the inventory does
    auto source = g.New("cargoContainer");
    source->fields.persistentId = 202;
    ProtoPartSnapshot snap(*source, nullptr);
    std::unique_ptr<Part> made = snap.CreatePart();
    made->fields.persistentId = 203;
    made->partInfo = &g.Info("cargoContainer");
    made->gameObjectName = "cargoContainer";
    made->protoPartSnapshot = &snap;
    Part* held = editor.AdoptPart(std::move(made));
    held->partTransform.position = Vector3f(3.0f, 3.0f, 0.0f);
    editor.SetSelectedPartForTest(held);
    editor.ProcessAttachNodes();  // node icons of the vessels in range
    AttachNode* top = parent->FindAttachNode("top");
    AttachNode* bottom = held->FindAttachNode("bottom");
    EVAConstructionEnvironment::nodeIconUnderCursor = [&](Part& p, AttachNode& n, const Vector3f&) { return &p == parent.get() && &n == top; };
    editor.attachment = editor.CheckAttach(held);
    editor.attachmentNull = false;
    REQUIRE(editor.attachment.possible);
    editor.attachmentPossible = true;
    // no click: nothing; a click over the UI: nothing
    editor.AttachInput();
    CHECK(held == editor.SelectedPart());
    EVAConstructionEnvironment::mouseButtonUp0 = true;
    EVAConstructionEnvironment::pointerOverUI = true;
    editor.AttachInput();
    CHECK(held == editor.SelectedPart());
    EVAConstructionEnvironment::pointerOverUI = false;
    // the click attaches: a new part is built from the snapshot and linked, the held one is destroyed
    ctrl.SetCurrentCargoPart(held);
    editor.AttachInput();
    CHECK(editor.SelectedPart() == nullptr);
    CHECK(editor.attachSuccesful);
    Part* attached = editor.LastAttachedPart();
    REQUIRE(attached != nullptr);
    CHECK(attached->isAttached());
    CHECK(attached->partInfo == &g.Info("cargoContainer"));
    CHECK(top->attachedPart == attached);
    AttachNode* attachedBottom = attached->FindAttachNode("bottom");
    REQUIRE(attachedBottom != nullptr);
    CHECK(attachedBottom->attachedPart == parent.get());
    CHECK(events.Count(ConstructionEventType::PartAttached) == 1);
    CHECK(att.attached.size() == 1);
    CHECK(att.attached[0].first == target.get());
    CHECK(att.attached[0].second == attached);
    CHECK(welds == std::vector<std::string>{"cargoContainer"});
    CHECK(ctrl.CurrentCargoPart() == nullptr);
    CHECK(std::find(EVAConstructionEnvironment::effects.begin(), EVAConstructionEnvironment::effects.end(), "audio:attachClip") !=
          EVAConstructionEnvironment::effects.end());
    // the held part's Object.Destroy is deferred to the end of the frame; then only the attached part is owned
    CHECK(editor.pendingDestroy.size() == 1);
    editor.FlushDestroyed();
    CHECK(editor.ownedParts.size() == 1);
    CHECK(editor.ownedParts[0].get() == attached);
    // the cargo module of the source part recorded beingAttached in its snapshot values
    CHECK(held == held);  // (the held part pointer is dangling now: only compared above)
    (void)bottom;
    // ---- detach: the attached part (not the vessel root) is picked up again: the backup keeps its attachment
    EVAConstructionEnvironment::mouseButtonUp0 = true;
    editor.attachSuccesful = false;
    attached->parent = parent.get();
    attached->fields.persistentId = 204;
    attached->vessel = target.get();
    target->parts.push_back(attached);
    EVAConstructionEnvironment::hoveredPart = attached;
    attached->partTransform.position = Vector3f(3.0f, 0.6f, 0.0f);
    events.events.clear();
    editor.DetachInput();
    CHECK(editor.SelectedPart() == nullptr);  // DetachPart does not select; the held part is the inventory controller's
    CHECK(editor.backedUpPart != nullptr);
    CHECK(editor.hasBackedUpAttachment);
    CHECK(editor.backedUpAttachment.id == "bottom");  // the detached part's node that held the parent
    CHECK(events.Count(ConstructionEventType::PartDetached) == 1);
    CHECK(att.detached.size() == 1);
    CHECK(att.detached[0].first == target.get());
    CHECK(ctrl.CurrentCargoPart() == attached);
    CHECK(ctrl.isSetAsPart);
    CHECK(editor.selectedPartOriginalRotation.w == attached->partTransform.rotation.w);
    CHECK(attached->fields.attRotation.w == 1.0f);
    CHECK_FALSE(editor.backedUpPart == attached);
    // ---- closing the panel (OnConstructionMode(false)) with the detached part still in hand destroys it and puts the backup back
    editor.SetSelectedPartForTest(attached);
    Part* backup = editor.backedUpPart;
    att.attached.clear();
    events.events.clear();
    ctrl.currentInventorySlotClicked = nullptr;
    target->parts.pop_back();  // the destroyed part leaves the vessel's list
    attached->vessel = nullptr;
    editor.OnConstructionMode(false);
    CHECK(editor.SelectedPart() == nullptr);
    CHECK(editor.LastAttachedPart() == backup);
    CHECK(editor.backedUpPart == nullptr);
    CHECK(backup->isAttached());
    CHECK(events.Count(ConstructionEventType::PartAttached) == 1);
    REQUIRE(att.attached.size() == 1);
    CHECK(att.attached[0].second == backup);
    CHECK(parent->FindAttachNode("top")->attachedPart == backup);
    editor.FlushDestroyed();
    // ---- the root part cannot be detached
    editor.backedUpPart = nullptr;
    editor.backedUpPartOwner.reset();
    att.detached.clear();
    EVAConstructionEnvironment::hoveredPart = parent.get();
    editor.DetachInput();
    CHECK(att.detached.empty());
    // ---- closing the panel with a backed-up held part restores it (RestoreLastAttachedPart)
    GameEvents::OnEVAConstructionModePartAttached.Remove(&att, &Att::A);
    GameEvents::OnEVAConstructionModePartDetached.Remove(&att, &Att::D);
    EVAConstructionEnvironment::mouseButtonUp0 = false;
    EVAConstructionEnvironment::hoveredPart = nullptr;
}

// EVAConstructionModeEditor.cs:1342-1370 / 1405-1455 (gizmo callbacks)
TEST_CASE("WP-33 EVAConstructionModeEditor: rotate and offset gizmo callbacks") {
    if (!GameAvailable()) return;
    Reset33 r;
    Game15 g;
    PhysicsWorld phys;
    SceneScope scene(GameScenes::FLIGHT);
    EVAConstructionModeEditor editor;
    PartEvents events;
    auto parent = g.New("cargoContainer");
    parent->partTransform.position = Vector3f(0.0f, 0.0f, 0.0f);
    auto child = g.New("cargoContainer");
    child->parent = parent.get();
    child->partTransform.position = Vector3f(0.0f, 0.6f, 0.0f);
    editor.SetSelectedPartForTest(child.get());
    // ---- rotate: Self -> rotation = gizmo.rotation * initRotation, World -> dRot * hostRot0; attRotation0 = localRotation,
    //      attRotation = gizmoAttRotate * Inverse(gizmoAttRotate0) * localRotation
    const Quaternionf gizmoRot = UnityNative::AngleAxis(40.0f, Vector3f::up());
    const Quaternionf dRot = UnityNative::AngleAxis(15.0f, Vector3f::right());
    const Quaternionf host0 = UnityNative::AngleAxis(25.0f, Vector3f::forward());
    editor.gizmoAttRotate = UnityNative::AngleAxis(5.0f, Vector3f::up());
    editor.gizmoAttRotate0 = UnityNative::AngleAxis(3.0f, Vector3f::up());
    editor.gizmoRotateSpace = UnityEngine::Space::Self;
    editor.OnRotateGizmoUpdate(dRot, gizmoRot, host0);
    const Quaternionf selfRot = gizmoRot * child->fields.initRotation;
    CHECK(Bits(child->partTransform.rotation.y) == Bits(selfRot.y));
    CHECK(Bits(child->partTransform.rotation.w) == Bits(selfRot.w));
    const Quaternionf local = EditorLogic::LocalRotation(*child);
    CHECK(Bits(child->fields.attRotation0.w) == Bits(local.w));
    const Quaternionf attRot = editor.gizmoAttRotate * UnityNative::Inverse(editor.gizmoAttRotate0) * local;
    CHECK(Bits(child->fields.attRotation.y) == Bits(attRot.y));
    CHECK(events.Count(ConstructionEventType::PartRotating) == 1);
    editor.gizmoRotateSpace = UnityEngine::Space::World;
    editor.OnRotateGizmoUpdate(dRot, gizmoRot, host0);
    const Quaternionf worldRot = dRot * host0;
    CHECK(Bits(child->partTransform.rotation.x) == Bits(worldRot.x));
    CHECK(Bits(child->partTransform.rotation.z) == Bits(worldRot.z));
    editor.OnRotateGizmoUpdated(dRot);
    CHECK(events.Count(ConstructionEventType::PartRotated) == 1);
    // ---- offset: the part follows the gizmo; attPos = localPosition - attPos0
    child->fields.attPos0 = Vector3f(0.0f, 0.5f, 0.0f);
    editor.rootPart = nullptr;  // selected == root: no separation test
    Vector3f gizmoPos(0.0f, 0.75f, 0.1f);
    editor.OnOffsetGizmoUpdate(Vector3f::zero(), gizmoPos);
    CHECK(child->partTransform.position.y == 0.75f);
    const Vector3f localPos = EditorLogic::LocalPosition(*child);
    const Vector3f attPos = localPos - child->fields.attPos0;
    CHECK(Bits(child->fields.attPos.y) == Bits(attPos.y));
    CHECK(Bits(child->fields.attPos.z) == Bits(attPos.z));
    CHECK(events.Count(ConstructionEventType::PartOffsetting) == 1);
    editor.OnOffsetGizmoUpdated(Vector3f::zero());
    CHECK(events.Count(ConstructionEventType::PartOffset) == 1);
    // the GetPivotOffset helper
    CHECK(editor.GetPivotOffset(Vector3f(1.0f, 2.0f, 3.0f), Vector3f(0.5f, 0.5f, 0.5f)).y == 1.5f);
    // stack separation: with the root elsewhere the bounds test limits the move (TestPartBoundsSeparate): parts without collider bounds never separate
    editor.rootPart = parent.get();
    Vector3f g2(0.0f, 5.0f, 0.0f);
    editor.OnOffsetGizmoUpdate(Vector3f::zero(), g2);
    CHECK(child->partTransform.position.y == 5.0f);  // no bounds -> TestPartBoundsSeparate false -> the gizmo position stands
    CHECK(editor.threshold == 0.1f);
    EditorEnvironment::Current().fineTweak = true;
    editor.OnOffsetGizmoUpdate(Vector3f::zero(), g2);
    EditorEnvironment::Current().fineTweak = false;
    CHECK(editor.threshold == InventoryEnvironment::VAB_FINE_OFFSET_THRESHOLD);
    // reset-rotation key: attRotation back to identity, localRotation = attRotation0
    child->fields.attRotation = UnityNative::AngleAxis(20.0f, Vector3f::up());
    child->fields.attRotation0 = UnityNative::AngleAxis(10.0f, Vector3f::up());
    editor.resetRotationKeyDown = true;
    editor.partRotationResetUpdate();
    CHECK(child->fields.attRotation.w == 1.0f);
    CHECK(Bits(EditorLogic::LocalRotation(*child).y) == Bits(child->fields.attRotation0.y));
    editor.resetRotationKeyDown = false;
}
