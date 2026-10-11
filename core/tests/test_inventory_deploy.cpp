// WP-33 unit tests: placing parts from the inventory (ModuleInventoryPart.DeployInventoryItem / CreatePartObject / SetPositionFromTerrain /
// UpdatePlacement / GetProtoVesselNode / CreatePartNode), dropping and attaching parts in EVA construction mode (EVAConstructionModeEditor
// UpdatePartPlacementPosition / AttachPart / DetachPart / DropAttachablePart / GetProtoVesselNode), the gizmo callbacks. The world is Kerbin
// (CelestialSystem) with an EVA kerbal vessel on it; the engine side (raycast, bounds, scene objects) is supplied through the environments.
#include "inventory_fixture.hpp"

#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/unity/quaternion_native.hpp"

using namespace ksp;
using namespace ksp::inv33;

namespace {

struct Kerbin33 {
    Game15 g;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    CoroutineScheduler sched;
    Kerbin33() {
        pg.physicsDatabaseFilename = ksp::testing::GameRoot() + "/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        SaveEnvironment::Current().localBodies = sys->SpawnOrder();
    }
    ~Kerbin33() {
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
    }
    CelestialBody& Kerbin() { return *sys->GetBodyByName("Kerbin"); }
};

// an EVA kerbal standing on Kerbin: vessel, kerbalEVA part with its inventory module, crew member
struct EvaKerbal {
    std::unique_ptr<Vessel> vessel;
    std::unique_ptr<Part> part;
    std::shared_ptr<ProtoCrewMember> crew;
    FakeEva ctl;
    ModuleInventoryPart* inv = nullptr;
    EvaKerbal(Kerbin33& w, StartState startState = StartState::Landed) {
        KerbalInventory::Install();
        vessel = BareVessel(VesselType::EVA, Vector3f());
        vessel->orbitDriver = std::make_unique<OrbitDriver>();
        vessel->orbitDriver->orbit->referenceBody = &w.Kerbin();
        vessel->Landed = true;
        vessel->situation = Vessel::Situations::LANDED;
        vessel->evaController = &ctl;
        part = w.g.New("kerbalEVA");
        crew = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Valentina Kerman");
        part->protoModuleCrew.push_back(crew);
        part->vessel = vessel.get();
        vessel->parts.push_back(part.get());
        vessel->rootPart = part.get();
        FlightRegistry::VesselsLoaded.push_back(vessel.get());
        FlightRegistry::ActiveVessel = vessel.get();
        inv = Inv(*part);
        inv->OnStart(startState);
    }
    ~EvaKerbal() {
        if (inv != nullptr) inv->OnDestroy();
        part->vessel = nullptr;
        KerbalInventory::ClearAll();
        KerbalInventory::Uninstall();
    }
};

// Time.deltaTime-free coroutine pump: the frames the placement coroutine needs (two `yield return null`)
void Pump(CoroutineScheduler& s, int frames = 3) {
    for (int i = 0; i < frames; i++) {
        s.AdvanceFrame(0.02f);
        s.RunDelayedDynamicFrameRate();
    }
}

Bounds MakeBounds(const Vector3f& extents) {
    Bounds b;
    b.set_extents(extents);
    return b;
}

float F(double x) { return f32store(x); }

}  // namespace

// ModuleInventoryPart.cs:475-670 / 1300-1403: placing a ground part from the inventory of the EVA kerbal
TEST_CASE("WP-33 ModuleInventoryPart: DeployInventoryItem -> CreatePartObject -> SetPositionFromTerrain -> UpdatePlacement") {
    if (!GameAvailable()) return;
    Reset33 r;
    Kerbin33 w;
    SceneScope scene(GameScenes::FLIGHT);
    LoadLocalizer();
    EvaKerbal k(w);
    ModuleInventoryPart* inv = k.inv;
    InventoryEnvironment::scheduler = &w.sched;
    UIPartActionControllerInventory ctrl;
    UIPartActionInventory pawGrid;
    // the kerbal's default inventory (evaChute, evaJetpack) plus a ground anchor in a free slot; the grid is built for 4 slots
    inv->SetInventorySlots(4);
    pawGrid.SetupPaw(*inv, inv->InventorySlots());
    REQUIRE(inv->StoreCargoPartAtSlot(&Prefab(w.g, "groundAnchor"), 3));
    CHECK(inv->InventoryItemCanBeDeployed(3));
    CHECK_FALSE(inv->InventoryItemCanBeDeployed(0));  // the chute is not ground-deployable
    CHECK_FALSE(inv->InventoryItemCanBeDeployed(2));  // empty slot
    // AbleToPlaceParts: EVA, landed, active vessel, nothing selected
    CHECK(inv->AbleToPlaceParts());
    k.vessel->Landed = false;
    CHECK_FALSE(inv->AbleToPlaceParts());
    k.vessel->Landed = true;
    // the engine side: terrain 50.3 m below the ray origin (50 m above the spawn point), flat, the part has 0.25 m of bounds in the up direction
    int ray = 0;
    InventoryEnvironment::raycast = [&](const Vector3f&, const Vector3f&, float, PlacementRayHit& hit) {
        ray++;
        hit.point = Vector3f();
        hit.normal = Vector3f(0.0f, 1.0f, 0.0f);
        hit.distance = 50.3f;
        hit.onTerrainLayer = true;
        return true;
    };
    InventoryEnvironment::getBoundsPoints = [](Part&, const Vector3f&, float& centerPointOffset) {
        centerPointOffset = 0.0f;
        return 0.25f;
    };
    InventoryEnvironment::getPartRendererBound = [](Part&) { return MakeBounds(Vector3f(0.5f, 0.25f, 0.3f)); };
    EVAConstructionEnvironment::upAxisAt = [](const Vector3d&) { return Vector3d(0.0, 1.0, 0.0); };
    InventoryEnvironment::bodyPosition = [] { return Vector3d(0.0, -600000.0, 0.0); };
    InventoryEnvironment::checkPartPersistentId = [](Part& p) { return p.fields.persistentId + 5000u; };
    int placementMode = 0;
    InventoryEnvironment::setPartPlacementMode = [&](bool m, ModuleInventoryPart*) {
        placementMode += m ? 1 : -1;
        return true;
    };
    bool jetpack = true;
    int toggles = 0;
    InventoryEnvironment::jetpackDeployed = [&] { return jetpack; };
    InventoryEnvironment::toggleJetpack = [&] {
        toggles++;
        jetpack = !jetpack;
    };
    int created = 0;
    InventoryEnvironment::placementPartCreated = [&](Part&) { created++; };
    k.vessel->vesselTransform.rotation = Quaternionf::identity();
    inv->DeployInventoryItem(3);
    // StartCoroutine ran to the first yield: the part exists, in PLACEMENT state, positioned in front of the kerbal
    REQUIRE(inv->selectedPart != nullptr);
    CHECK(placementMode == 1);
    CHECK(toggles == 1);  // the jetpack is folded away
    CHECK(inv->selectedPart->State() == PartStates::PLACEMENT);
    CHECK(inv->selectedPart->fields.ResumeState == static_cast<int32_t>(PartStates::PLACEMENT));
    CHECK(inv->selectedPart->gameObjectName == "groundAnchor");
    CHECK(inv->selectedPart->fields.persistentId == inv->selectedPart->fields.persistentId);
    CHECK(inv->selectedPart->fields.physicalSignificance == static_cast<int32_t>(Part_PhysicalSignificance::NONE));
    CHECK_FALSE(inv->partFullyCreated);
    CHECK(inv->placementPendingIndex == 3);
    CHECK(inv->placementAllowXRotation == true);  // ModuleGroundPart defaults: all axes allowed
    // spawnDistance = max(extents) + 0.2f; position = start + up * (boundsPoints - num2), boundsPoints = 0.25 + 0.1, num2 = 50.3 - 50
    CHECK(Bits(inv->spawnDistance) == Bits(F(0.5 + F(0.2))));
    const float bp = F(F(0.25) + F(0.1));
    const float num2 = F(F(50.3) - 50.0);
    const float lift = F(bp - num2);  // num2 (0.3) < boundsPoints (0.35)
    const Vector3f start = k.vessel->vesselTransform.position + k.vessel->vesselTransform.forward() * inv->spawnDistance;
    CHECK(inv->placementStartingPosition.x == start.x);
    CHECK(inv->placementStartingPosition.z == start.z);
    CHECK(Bits(inv->selectedPart->partTransform.position.y) == Bits(F(start.y + double(lift))));
    CHECK(inv->placementonTerrain);
    CHECK(inv->placementInsideCap);
    CHECK(ray >= 1);
    Pump(w.sched);
    CHECK(inv->partFullyCreated);
    CHECK(inv->placementPositionOk);
    CHECK(created == 1);
    CHECK_FALSE(inv->AbleToPlaceParts());  // a part is selected
    // a second request while one is pending is refused (the grid's place icons are reset)
    inv->DeployInventoryItem(3);
    CHECK(placementMode == 1);
    // ---- rotating: TRANSLATE_UP held for 0.1 s at 80 deg/s -> Rotate(right * 80 * 0.1) about the local x axis
    ModuleInventoryPart::PlacementInput in;
    in.deltaTime = 0.1f;
    in.up = true;
    const Quaternionf before = inv->selectedPart->partTransform.rotation;
    inv->UpdatePlacement(in);
    const Quaternionf expectRot = before * UnityNative::Euler(Vector3f::right() * inv->placementRotateSpeed() * 0.1f);
    CHECK(Bits(inv->selectedPart->partTransform.rotation.x) == Bits(expectRot.x));
    CHECK(Bits(inv->selectedPart->partTransform.rotation.w) == Bits(expectRot.w));
    // the reset key restores the kerbal's orientation first
    in.up = false;
    in.rcsToggleDown = true;
    inv->UpdatePlacement(in);
    CHECK(Bits(inv->selectedPart->partTransform.rotation.w) == Bits(k.part->partTransform.rotation.w));
    in.rcsToggleDown = false;
    // a blocked spot flips the highlight state once
    InventoryEnvironment::currentCollisionCount = [](Part&) { return 2; };
    inv->highlightLog.clear();
    inv->UpdatePlacement(in);
    inv->UpdatePlacement(in);
    CHECK(Has(inv->highlightLog, "placement:blocked"));
    CHECK(std::count(inv->highlightLog.begin(), inv->highlightLog.end(), std::string("placement:blocked")) == 1);
    CHECK_FALSE(inv->placementPositionOk);
    // jump while blocked: the "not allowed" sound, the part stays
    in.jumpDown = true;
    inv->UpdatePlacement(in);
    CHECK(inv->placementSoundPlays == 1);
    CHECK(inv->selectedPart != nullptr);
    CHECK_FALSE(inv->IsSlotEmpty(3));
    // ---- jump on a free spot deploys: the vessel node is handed to AddVessel, the slot clears, the cooldown starts
    InventoryEnvironment::currentCollisionCount = [](Part&) { return 0; };
    ConfigNode::NodePtr deployed;
    InventoryEnvironment::addVessel = [&](const ConfigNode& n) -> ProtoVessel* {
        deployed = n.CreateCopy();
        return nullptr;
    };
    uint32_t pid = 9000;
    InventoryEnvironment::uniquePersistentId = [&] { return pid++; };
    InventoryEnvironment::uniqueFlightID = [] { return 321u; };
    InventoryEnvironment::newGuidN = [] { return std::string("0123456789abcdef0123456789abcdef"); };
    InventoryEnvironment::randomRotation = [] { return Quaternionf::identity(); };
    InventoryEnvironment::universalTime = [] { return 1234.5; };
    struct Deployed {
        std::vector<std::string> v;
        void D(std::string n) { v.push_back(n); }
    } dep;
    GameEvents::onDeployGroundPart.Add(&dep, &Deployed::D);
    const Vector3f posAtDeploy = inv->selectedPart->partTransform.position;
    in.jumpDown = true;
    inv->UpdatePlacement(in);
    GameEvents::onDeployGroundPart.Remove(&dep, &Deployed::D);
    CHECK(inv->selectedPart == nullptr);
    CHECK(inv->IsSlotEmpty(3));
    CHECK(dep.v == std::vector<std::string>{"groundAnchor"});
    CHECK(inv->placementCoolDown);
    CHECK(inv->placementPendingIndex == -1);
    CHECK_FALSE(inv->partFullyCreated);
    REQUIRE(deployed != nullptr);
    // the vessel node (ProtoVessel.CreateVesselNode + the overrides of GetProtoVesselNode)
    std::string v;
    CHECK(deployed->TryGetValue("pid", v));
    CHECK(v == "0123456789abcdef0123456789abcdef");
    deployed->TryGetValue("name", v);
    CHECK(v == w.g.Info("groundAnchor").title);  // the vessel is named after the part title
    deployed->TryGetValue("type", v);
    CHECK(v == "Unknown");
    deployed->TryGetValue("sit", v);
    CHECK(v == "LANDED");
    deployed->TryGetValue("landedAt", v);
    CHECK(v == "Kerbin");
    bool b = false;
    CHECK(deployed->TryGetValue("landed", b));
    CHECK(b);
    CHECK(deployed->TryGetValue("splashed", b));
    CHECK_FALSE(b);
    CHECK(deployed->TryGetValue("skipGroundPositioning", b));
    CHECK_FALSE(b);
    CHECK(deployed->TryGetValue("vesselSpawning", b));
    CHECK(b);
    CHECK(deployed->TryGetValue("prst", b));
    CHECK(b);
    double lat = 0.0, lon = 0.0, alt = 0.0, nlat = 0.0, nlon = 0.0, nalt = 0.0;
    w.Kerbin().GetLatLonAlt(Vector3d(posAtDeploy), lat, lon, alt);
    deployed->TryGetValue("lat", nlat);
    deployed->TryGetValue("lon", nlon);
    deployed->TryGetValue("alt", nalt);
    CHECK(nlat == lat);
    CHECK(nlon == lon);
    CHECK(nalt == alt);
    int pqs = -1;
    CHECK(deployed->TryGetValue("PQSMin", pqs));
    CHECK(pqs == 0);
    CHECK(deployed->HasNode("ORBIT"));
    CHECK(deployed->HasNode("ACTIONGROUPS"));
    ConfigNode* partNode = deployed->GetNode("PART");
    REQUIRE(partNode != nullptr);
    uint32_t uid = 0;
    CHECK(partNode->TryGetValue("uid", uid));
    CHECK(uid == 321u);
    CHECK(partNode->TryGetValue("mid", uid));
    CHECK(uid == 321u);
    partNode->TryGetValue("name", v);
    CHECK(v == "groundAnchor");
    int state = -1;
    CHECK(partNode->TryGetValue("state", state));
    CHECK(state == 0);
    CHECK(partNode->HasValue("flag"));
    // the cooldown (1 s of UT) ends: placement mode is left and the grid's place buttons are reset
    CHECK(placementMode == 1);
    InventoryEnvironment::universalTime = [] { return 1234.5 + 0.5; };
    inv->UpdatePlacement(in);
    CHECK(inv->placementCoolDown);
    CHECK(placementMode == 1);
    InventoryEnvironment::universalTime = [] { return 1234.5 + 1.5; };
    inv->UpdatePlacement(in);
    CHECK_FALSE(inv->placementCoolDown);
    CHECK(placementMode == 0);
    // ---- cancelling (Escape) removes the preview and starts the cooldown
    REQUIRE(inv->StoreCargoPartAtSlot(&Prefab(w.g, "groundAnchor"), 3));
    inv->DeployInventoryItem(3);
    Pump(w.sched);
    REQUIRE(inv->selectedPart != nullptr);
    in.jumpDown = false;
    in.escapeDown = true;
    inv->UpdatePlacement(in);
    CHECK(inv->selectedPart == nullptr);
    CHECK(inv->placementCoolDown);
    CHECK_FALSE(inv->IsSlotEmpty(3));  // nothing deployed
    in.escapeDown = false;
    // a deployed jetpack still on: the kerbal's jetpack blocks placement (UpdatePlacement cancels)
    inv->placementCoolDown = false;
    inv->DeployInventoryItem(3);
    Pump(w.sched);
    in.jetpackDeployed = true;
    inv->UpdatePlacement(in);
    CHECK(inv->selectedPart == nullptr);
}

// ModuleInventoryPart.cs:1816-1860: the deployed part's skill-scaled module values
TEST_CASE("WP-33 ModuleInventoryPart.CreatePartNode: deployed-science skills scale powerUnitsProduced and ScienceModifierRate, beingDeployed is set") {
    if (!GameAvailable()) return;
    Reset33 r;
    Kerbin33 w;
    SceneScope scene(GameScenes::FLIGHT);
    LoadLocalizer();
    EvaKerbal k(w);
    ConfigNode::NodePtr cfgRoot = ConfigNode::Parse(
        "EXPERIENCE_TRAIT\n{\nname = Synth\ntitle = #autoLOC_500101\ndesc = #autoLOC_500102\n"
        "EFFECT\n{\nname = DeployedScienceExpSkill\nmodifiers = 0.3, 0.35, 0.4, 0.45, 0.5\n}\n"
        "EFFECT\n{\nname = DeployedSciencePowerSkill\nmodifiers = 0.1, 0.2, 0.3, 0.4, 0.5\n}\n}\n");
    std::vector<const ConfigNode*> nodes{cfgRoot->GetNode("EXPERIENCE_TRAIT")};
    Experience::ExperienceSystemConfig cfg(nodes);
    SaveEnvironment::Current().experienceConfigs = std::make_shared<Experience::ExperienceSystemConfig>(nodes);
    SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", true);
    k.crew->experienceLevel = 3;
    k.crew->experienceTrait = Experience::ExperienceTrait::Create("Synth", cfg.GetExperienceTraitConfig("Synth"), k.crew.get());
    k.crew->RegisterExperienceTraits(*k.part);
    auto* power = k.crew->GetEffect<Experience::Effects::DeployedSciencePowerSkill>();
    auto* expSkill = k.crew->GetEffect<Experience::Effects::DeployedScienceExpSkill>();
    REQUIRE(power != nullptr);
    REQUIRE(expSkill != nullptr);
    // a stored part carrying a ground-experiment MODULE with powerUnitsProduced and beingDeployed
    ProtoPartSnapshot kit(Prefab(w.g, "evaRepairKit"), nullptr);
    ConfigNode partNode("PART");
    kit.Save(partNode);
    ConfigNode* m1 = partNode.AddNode("MODULE");
    m1->AddValue("name", "ModuleGroundExperiment");
    m1->AddValue("powerUnitsProduced", 3);
    m1->AddValue("beingDeployed", false);
    ConfigNode* m2 = partNode.AddNode("MODULE");
    m2->AddValue("name", "ModuleOther");
    m2->AddValue("powerUnitsProduced", 0);  // not > 0: untouched
    StoredPart sp("evaRepairKit", 0);
    sp.snapshot = std::make_shared<ProtoPartSnapshot>(partNode, nullptr, nullptr);
    ConfigNode::NodePtr out = k.inv->CreatePartNode(77u, sp);
    std::vector<ConfigNode*> mods = out->GetNodes("MODULE");
    ConfigNode* exp1 = nullptr;
    ConfigNode* other = nullptr;
    for (ConfigNode* m : mods) {
        std::string n;
        m->TryGetValue("name", n);
        if (n == "ModuleGroundExperiment") exp1 = m;
        if (n == "ModuleOther") other = m;
    }
    REQUIRE(exp1 != nullptr);
    REQUIRE(other != nullptr);
    float units = 0.0f;
    CHECK(exp1->TryGetValue("powerUnitsProduced", units));
    CHECK(Bits(units) == Bits(f32store(fx(3.0f) + fx(power->GetValue()))));
    float rate = 0.0f;
    CHECK(exp1->TryGetValue("ScienceModifierRate", rate));
    CHECK(Bits(rate) == Bits(f32store(fx(expSkill->GetValue()) * 100.0)));
    bool deploying = false;
    CHECK(exp1->TryGetValue("beingDeployed", deploying));
    CHECK(deploying);
    int zero = -1;
    CHECK(other->TryGetValue("powerUnitsProduced", zero));
    CHECK(zero == 0);
    uint32_t uid = 0;
    CHECK(out->TryGetValue("uid", uid));
    CHECK(uid == 77u);
    // a kerbal without the effect (trait absent) cannot be asked: the C# dereferences a null trait -> NullReferenceException
    k.crew->UnregisterExperienceTraits(*k.part);
    k.crew->experienceTrait.reset();
    CHECK_THROWS_AS(k.inv->CreatePartNode(78u, sp), NullReferenceException);
    SaveEnvironment::Current().experienceConfigs.reset();
}

// EVAConstructionModeEditor.cs:385-487: where the held part goes (ground / camera plane) and whether the spot is valid
TEST_CASE("WP-33 EVAConstructionModeEditor.UpdatePartPlacementPosition: ground and camera-plane placement, range, validity") {
    if (!GameAvailable()) return;
    Reset33 r;
    Kerbin33 w;
    SceneScope scene(GameScenes::FLIGHT);
    EvaKerbal k(w);
    UIPartActionControllerInventory ctrl;
    EVAConstructionModeController controller;
    EVAConstructionModeEditor editor;
    controller.evaEditor = &editor;
    EVAConstructionModeController::Instance = &controller;
    EVAConstructionEnvironment::activeVesselWorldPos = [] { return Vector3f(0.0f, 0.0f, 0.0f); };
    EVAConstructionEnvironment::upAxisAt = [](const Vector3d&) { return Vector3d(0.0, 1.0, 0.0); };
    EVAConstructionEnvironment::getBoundsPoints = [](Part&, const Vector3f&, float& c) {
        c = 0.0f;
        return 0.4f;
    };
    auto held = w.g.New("evaRepairKit");
    held->fields.attachRules = AttachRules{false, true, false, false, false, false, false, false};
    held->fields.persistentId = 55;
    editor.SetSelectedPartForTest(held.get());
    editor.placementGroundOffset = 0.1f;
    editor.selectedPartOriginalRotation = UnityNative::AngleAxis(30.0f, Vector3f::up());
    held->fields.attRotation = UnityNative::AngleAxis(10.0f, Vector3f::right());
    PartEvents events;
    // ---- ground hit within range: position = hit + up * (bounds + offset), valid and on the ground
    EVAConstructionEnvironment::groundRaycast = [] { return std::optional<EVAConstructionEnvironment::GroundHit>({Vector3f(3.0f, 0.0f, 4.0f), Vector3f(0.0f, 1.0f, 0.0f)}); };
    editor.UpdatePartPlacementPosition();
    CHECK(editor.IsPlacementValid());
    CHECK(editor.isPlacementOnGround);
    CHECK(held->partTransform.position.x == 3.0f);
    CHECK(Bits(held->partTransform.position.y) == Bits(F(F(0.4) + F(0.1))));
    CHECK(held->partTransform.position.z == 4.0f);
    // the rotation is the stored original times attRotation (no attachment possible: no stack / surface hit)
    const Quaternionf rot = editor.selectedPartOriginalRotation * held->fields.attRotation;
    CHECK(Bits(held->partTransform.rotation.y) == Bits(rot.y));
    CHECK_FALSE(editor.attachmentPossible);
    CHECK(held->potentialParent == nullptr);
    CHECK(events.Count(ConstructionEventType::PartDragging) == 0);
    EVAConstructionEnvironment::mouseIsMoving = true;
    editor.UpdatePartPlacementPosition();
    CHECK(events.Count(ConstructionEventType::PartDragging) == 1);
    EVAConstructionEnvironment::mouseIsMoving = false;
    // ---- beyond EVA_CONSTRUCTION_RANGE (7 m): the ground hit is refused, the camera plane takes over (also out of range: invalid)
    EVAConstructionEnvironment::groundRaycast = [] { return std::optional<EVAConstructionEnvironment::GroundHit>({Vector3f(30.0f, 0.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f)}); };
    EVAConstructionEnvironment::cameraPlanePoint = [] { return std::optional<Vector3f>(Vector3f(0.0f, 2.0f, 3.0f)); };
    editor.UpdatePartPlacementPosition();
    CHECK(editor.IsPlacementValid());
    CHECK_FALSE(editor.isPlacementOnGround);
    CHECK(held->partTransform.position.y == 2.0f);
    EVAConstructionEnvironment::cameraPlanePoint = [] { return std::optional<Vector3f>(Vector3f(0.0f, 20.0f, 3.0f)); };
    editor.UpdatePartPlacementPosition();
    CHECK(editor.IsPlacementValid());  // the final assignment overrides: isPlacementValid = !hovered || rules (no hover)
    // ---- hovering a part that allows neither surface attach nor stacking makes the spot invalid
    auto other = w.g.New("evaChute");
    other->fields.attachRules = AttachRules{};
    EVAConstructionEnvironment::hoveredPart = other.get();
    held->fields.attachRules = AttachRules{};  // the held part itself allows neither
    editor.UpdatePartPlacementPosition();
    CHECK_FALSE(editor.IsPlacementValid());
    held->fields.attachRules = AttachRules{false, true, false, true, false, false, false, false};  // allowSrfAttach
    editor.UpdatePartPlacementPosition();
    CHECK(editor.IsPlacementValid());
    EVAConstructionEnvironment::hoveredPart = nullptr;
    // ---- cargo colliders: a collider that belongs to no part (or to a part other than the parent / itself) invalidates the placement
    auto stranger = w.g.New("evaJetpack");
    stranger->fields.persistentId = 99;
    EVAConstructionEnvironment::cargoColliderCount = [](Part&) { return size_t(2); };
    EVAConstructionEnvironment::cargoColliderPart = [&](Part&, size_t i) -> Part* { return i == 1 ? stranger.get() : nullptr; };
    editor.UpdatePartPlacementPosition();
    CHECK_FALSE(editor.IsPlacementValid());  // index 1 first (count-- loop): a stranger, no potential parent -> invalid, loop breaks
    EVAConstructionEnvironment::cargoColliderPart = [&](Part& p, size_t) -> Part* { return &p; };
    editor.UpdatePartPlacementPosition();
    CHECK_FALSE(editor.IsPlacementValid());  // with a collider present and no potential parent the spot is invalid whatever the collider is (cs:459)
    EVAConstructionEnvironment::cargoColliderCount = nullptr;
    EVAConstructionEnvironment::cargoColliderPart = nullptr;
    // ---- not an EVA kerbal / cursor over a PAW / a held stack: nothing is placed
    ctrl.IsCursorOverAnyPAWOrCargoPane = true;
    held->partTransform.position = Vector3f(-1.0f, -1.0f, -1.0f);
    editor.UpdatePartPlacementPosition();
    CHECK_FALSE(editor.IsPlacementValid());
    CHECK(held->partTransform.position.x == -1.0f);
    ctrl.IsCursorOverAnyPAWOrCargoPane = false;
    UIPartActionControllerInventory::heldPartIsStack = true;
    editor.UpdatePartPlacementPosition();
    CHECK_FALSE(editor.IsPlacementValid());
    UIPartActionControllerInventory::heldPartIsStack = false;
    controller.OnPointerEnter();  // hovering the construction panel
    editor.UpdatePartPlacementPosition();
    CHECK_FALSE(editor.IsPlacementValid());
    controller.OnPointerExit();
}

// EVAConstructionModeEditor.cs:1115-1263 / 686-:765: dropping a held part as a new vessel, with the node of each situation
TEST_CASE("WP-33 EVAConstructionModeEditor: DropAttachablePart / GetProtoVesselNode / CreatePartNode, ForceDrop, Pickup, Detach and Restore") {
    if (!GameAvailable()) return;
    Reset33 r;
    Kerbin33 w;
    SceneScope scene(GameScenes::FLIGHT);
    LoadLocalizer();
    EvaKerbal k(w);
    UIPartActionControllerInventory ctrl;
    EVAConstructionModeController controller;
    EVAConstructionModeEditor editor;
    controller.evaEditor = &editor;
    EVAConstructionModeController::Instance = &controller;
    PartEvents events;
    InventoryEnvironment::uniquePersistentId = [] { return 5555u; };
    InventoryEnvironment::newGuidN = [] { return std::string("ffffffffffffffffffffffffffffffff"); };
    InventoryEnvironment::randomRotation = [] { return Quaternionf::identity(); };
    InventoryEnvironment::universalTime = [] { return 100.0; };
    EVAConstructionEnvironment::uniqueFlightID = [] { return 4242u; };
    std::vector<ConfigNode::NodePtr> added;
    EVAConstructionEnvironment::addVessel = [&](const ConfigNode& n) -> ProtoVessel* {
        added.push_back(n.CreateCopy());
        return nullptr;
    };
    InventoryEnvironment::bodyRotation = [] { return Quaternionf::identity(); };
    auto held = w.g.New("evaRepairKit");
    held->fields.persistentId = 31;
    held->partInfo = &w.g.Info("evaRepairKit");
    held->vessel = nullptr;
    // Landed vessel, a valid spot on the ground -> LANDED, skipGroundPositioning false
    editor.SetSelectedPartForTest(held.get());
    editor.isPlacementValid_ = true;
    editor.isPlacementOnGround = true;
    const Vector3f pos(1.5f, 0.25f, -2.0f);
    ConfigNode::NodePtr n1 = editor.GetProtoVesselNode("evaRepairKit", pos, Quaternionf::identity(), *k.vessel, *held);
    std::string sv;
    n1->TryGetValue("sit", sv);
    CHECK(sv == "LANDED");
    n1->TryGetValue("type", sv);
    CHECK(sv == "DroppedPart");
    n1->TryGetValue("name", sv);
    CHECK(sv == held->partInfo->partPrefab->partInfo->title);
    bool b = false;
    CHECK(n1->TryGetValue("landed", b));
    CHECK(b);
    CHECK(n1->TryGetValue("skipGroundPositioning", b));
    CHECK_FALSE(b);
    CHECK_FALSE(n1->HasValue("skipGroundPositioningForDroppedPart"));
    double lat = 0.0, lon = 0.0, alt = 0.0, nlat = 0.0, nalt = 0.0;
    w.Kerbin().GetLatLonAlt(Vector3d(pos), lat, lon, alt);
    n1->TryGetValue("lat", nlat);
    n1->TryGetValue("alt", nalt);
    CHECK(nlat == lat);
    CHECK(nalt == alt);
    CHECK(n1->TryGetValue("prst", b));
    CHECK(b);
    // the PART node: ProtoVessel.CreatePartNode + flag + each module saved (ModuleCargoPart.beingSettled = active vessel landed)
    ConfigNode* pn = n1->GetNode("PART");
    REQUIRE(pn != nullptr);
    uint32_t uid = 0;
    CHECK(pn->TryGetValue("uid", uid));
    CHECK(uid == 4242u);
    pn->TryGetValue("name", sv);
    CHECK(sv == "evaRepairKit");
    float mass = 0.0f;
    pn->TryGetValue("mass", mass);
    CHECK(Bits(mass) == Bits(held->fields.mass));
    CHECK(pn->HasValue("flag"));
    bool settled = false;
    ConfigNode* cargoNode = nullptr;
    for (ConfigNode* m : pn->GetNodes("MODULE")) {
        std::string n;
        m->TryGetValue("name", n);
        if (n == "ModuleCargoPart") cargoNode = m;
    }
    REQUIRE(cargoNode != nullptr);
    CHECK(cargoNode->TryGetValue("beingSettled", settled));
    CHECK(settled);
    // valid spot but not on the ground (the camera plane): the dropped part starts SUB_ORBITAL with ground positioning skipped
    editor.isPlacementOnGround = false;
    ConfigNode::NodePtr n2 = editor.GetProtoVesselNode("evaRepairKit", pos, Quaternionf::identity(), *k.vessel, *held);
    n2->TryGetValue("sit", sv);
    CHECK(sv == "SUB_ORBITAL");
    CHECK(n2->TryGetValue("landed", b));
    CHECK(b);
    CHECK(n2->TryGetValue("skipGroundPositioning", b));
    CHECK(b);
    CHECK(n2->TryGetValue("skipGroundPositioningForDroppedPart", b));
    CHECK(b);
    // PRELAUNCH behaves like LANDED
    k.vessel->situation = Vessel::Situations::PRELAUNCH;
    editor.isPlacementOnGround = true;
    ConfigNode::NodePtr n3 = editor.GetProtoVesselNode("evaRepairKit", pos, Quaternionf::identity(), *k.vessel, *held);
    n3->TryGetValue("sit", sv);
    CHECK(sv == "LANDED");
    k.vessel->situation = Vessel::Situations::LANDED;
    // ---- DropAttachablePart: AddVessel, PartDropped, the held part is destroyed, backups dropped, cache reset
    struct Dropped {
        std::vector<Part*> v;
        void D(ConstructionEventType t, Part* p) {
            if (t == ConstructionEventType::PartDropped) v.push_back(p);
        }
    } dropped;
    GameEvents::onEditorPartEvent.Add(&dropped, &Dropped::D);
    auto owned = w.g.New("evaRepairKit");
    owned->fields.persistentId = 32;
    owned->partInfo = &w.g.Info("evaRepairKit");
    Part* ownedRaw = editor.AdoptPart(std::move(owned));
    editor.SetSelectedPartForTest(ownedRaw);
    editor.DropAttachablePart("evaRepairKit", pos, Quaternionf::identity());
    GameEvents::onEditorPartEvent.Remove(&dropped, &Dropped::D);
    CHECK(added.size() == 1);
    CHECK(dropped.v.size() == 1);
    CHECK(dropped.v[0] == ownedRaw);
    CHECK(editor.SelectedPart() == nullptr);
    CHECK(editor.ownedParts.empty());
    CHECK(editor.selectedPartColliders.empty());
    CHECK(std::find(EVAConstructionEnvironment::effects.begin(), EVAConstructionEnvironment::effects.end(), "audio:attachClip") !=
          EVAConstructionEnvironment::effects.end());
    // ---- ForceDrop: a held part of the construction panel is dropped where it is when allowed
    added.clear();
    auto again = w.g.New("evaRepairKit");
    again->fields.persistentId = 33;
    again->partInfo = &w.g.Info("evaRepairKit");
    PhysicsGlobals* pg = PhysicsGlobals::Instance();
    REQUIRE(pg != nullptr);
    Part* againRaw = editor.AdoptPart(std::move(again));
    editor.SetSelectedPartForTest(againRaw);
    editor.ForceDrop();
    CHECK(added.size() == 1);
    // too heavy: ForceDrop refuses with the message
    auto heavy = w.g.New("evaRepairKit");
    heavy->fields.persistentId = 34;
    heavy->partInfo = &w.g.Info("evaRepairKit");
    heavy->fields.mass = 5.0f;
    heavy->vessel = k.vessel.get();
    k.vessel->gravityForPos = Vector3d(0.0, 0.0, 9.80665);
    Part* heavyRaw = editor.AdoptPart(std::move(heavy));
    editor.SetSelectedPartForTest(heavyRaw);
    editor.screenMessages.clear();
    editor.ForceDrop();
    CHECK(added.size() == 1);
    CHECK(editor.screenMessages == std::vector<std::string>{Msg("#autoLOC_8012062", heavyRaw->partInfo->title)});
    heavyRaw->vessel = nullptr;
    editor.SetSelectedPartForTest(nullptr);
}
