// WP-32 unit tests, part 6: flag planting, EVA construction / weld hooks, parachute states, collisions, ragdoll bookkeeping, helmet / visor / lamp.
// Expected values are read off the C# (cited per case); SYNTHETIC: the engine answers (rays, colliders, animation clip lengths).
#include "eva_fixture.hpp"

#include "ksp/events/game_events.hpp"
#include "ksp/modules/parachutes.hpp"
#include "ksp/unity/quaternion_native.hpp"

using namespace ksp;
using namespace ksp::eva32;

namespace {

struct FlagMock : IEvaFlag {
    int fail = 0, complete = 0;
    void OnPlacementFail() override { fail++; }
    void OnPlacementComplete() override { complete++; }
};

KFSMEvent* FindEv(KerbalEVA& e, const std::string& name) {
    for (KFSMState* st : e.fsm->GetStates())
        for (KFSMEvent* ev : st->StateEvents())
            if (ev->name == name) return ev;
    return nullptr;
}

// a landed vessel with one pod and the given crew on Kerbin
struct Landed {
    std::unique_ptr<Part> pod;
    std::unique_ptr<Vessel> vessel;
    Landed(EvaWorld& w, const char* podName, VesselType type, std::vector<std::shared_ptr<ProtoCrewMember>> crew) {
        pod = w.g.New(podName);
        for (auto& c : crew) pod->protoModuleCrew.push_back(c);
        vessel = inv33::BareVessel(type, Vector3f(10.0f, 0.0f, 0.0f));
        vessel->orbitDriver = std::make_unique<OrbitDriver>();
        vessel->orbitDriver->orbit->referenceBody = &w.Kerbin();
        vessel->situation = Vessel::Situations::LANDED;
        vessel->Landed = true;
        pod->vessel = vessel.get();
        vessel->parts.push_back(pod.get());
        vessel->rootPart = pod.get();
        FlightRegistry::VesselsLoaded.push_back(vessel.get());
    }
    ~Landed() { pod->vessel = nullptr; }
};

int Count(const ProtoCrewMember& c, const std::string& type, const std::string& target) {
    int n = 0;
    for (const auto& e : c.flightLog.Entries()) n += (e.type == type && e.target == target) ? 1 : 0;
    return n;
}

}  // namespace

// KerbalEVA.cs:6723 PlantFlag, :2773 flagAcquireHeading_OnEnter, :2797 flagPlant_OnEnter, :2812 flagPlant_OnLeave
TEST_CASE("WP-32 flag planting: PlantFlag -> terrain acquire -> planting -> credit for every loaded crew; a failed plant tells the flag") {
    if (!GameAvailable()) return;
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    FlagMock flag;
    int created = 0;
    Vector3f spot;
    Quaternionf rot;
    EvaEnvironment::flagScanSurroundingTerrain = [](const Vector3f& pos, const Vector3f& fwd, float reach) { return pos + fwd * reach; };
    EvaEnvironment::flagCreate = [&](const Vector3f& s, const Quaternionf& r, Part& planter) {
        created++;
        spot = s;
        rot = r;
        CHECK(&planter == k.part.get());
        return &flag;
    };
    k.engine.raycast = [](const Vector3f&, const Vector3f&, float, int, EvaRaycastHit& hit) {
        hit.point = Vector3f(1.0f, 2.0f, 3.0f);
        return true;
    };
    // the other crews that get the credit: a landed ship (2 kerbals), another EVA kerbal; an orbiting ship gets nothing
    auto bob = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Bob Kerman");
    auto bill = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Bill Kerman");
    auto jeb = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Jebediah Kerman");
    auto val2 = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Gene Kerman");
    Landed ship(w, "mk1pod.v2", VesselType::Ship, {bob, bill});
    Landed other(w, "kerbalEVA", VesselType::EVA, {jeb});
    Landed orbiting(w, "mk1pod.v2", VesselType::Ship, {val2});
    orbiting.vessel->situation = Vessel::Situations::ORBITING;
    // CanPlantFlag: ACTIVE vessel, ground contact, flags left, not ragdolled, not constructing
    k.part->fields.GroundContact = true;
    e.set__flags(2);
    e.Events()["PlantFlag"]->active = false;
    e.idle_OnEnter(nullptr);   // UpdateEvent: PlantFlag.active = CanPlantFlag()
    CHECK(e.Events()["PlantFlag"]->active);
    e.PlantFlag();
    CHECK(e.flagItems() == 1);
    CHECK(e.fsm->currentStateName == "Flag-plant Terrain Acquire");
    // flagAcquireHeading_OnEnter: flagSpot = ScanSurroundingTerrain(position, forward, flagReach); SetWaypoint -> tgtRpos = normalized direction to it
    CHECK(e.flagSpot == k.part->partTransform.position + k.part->partTransform.forward() * e.flagReach());
    CHECK(e.tgtRpos == Vector3f(0.0f, 0.0f, 1.0f));
    e.tgtFwd = Vector3f(0.0f, 0.0f, 1.0f);
    e.deltaHdg = 0.0f;
    e.lastDeltaHdg = 0.0f;
    Time::time = f32store(fx(Time::time) + 0.1);
    e.fsm->FixedUpdateFSM();
    CHECK(e.fsm->currentStateName == "Planting Flag");
    CHECK(created == 1);
    CHECK(spot == Vector3f(1.0f, 2.0f, 3.0f));
    CHECK(e.flag == &flag);
    bool locked = false;
    for (const auto& l : EvaEnvironment::controlLockLog) locked |= l == "set:FlagDeployLock_" + k.vessel->id.ToString();
    CHECK(locked);
    // the plant animation ends: the flag is placed and every landed crew gets the entry with the body name
    Time::time = f32store(fx(Time::time) + fx(e.Animations().flagPlant.State()->length) + 0.5);
    e.fsm->UpdateFSM();
    CHECK(e.fsm->currentStateName == "Idle (Grounded)");
    CHECK(flag.complete == 1);
    CHECK(flag.fail == 0);
    CHECK(e.flag == nullptr);
    CHECK(Count(*k.crew, "PlantFlag", "Kerbin") == 1);
    CHECK(Count(*bob, "PlantFlag", "Kerbin") == 1);
    CHECK(Count(*bill, "PlantFlag", "Kerbin") == 1);
    CHECK(Count(*jeb, "PlantFlag", "Kerbin") == 1);
    CHECK(Count(*val2, "PlantFlag", "Kerbin") == 0);
    bool unlocked = false;
    for (const auto& l : EvaEnvironment::controlLockLog) unlocked |= l == "remove:FlagDeployLock_" + k.vessel->id.ToString();
    CHECK(unlocked);
    // second flag, interrupted: OnPlacementFail
    e.PlantFlag();
    CHECK(e.flagItems() == 0);
    e.tgtFwd = Vector3f(0.0f, 0.0f, 1.0f);
    e.fsm->FixedUpdateFSM();
    REQUIRE(e.fsm->currentStateName == "Planting Flag");
    e.fsm->RunEvent(FindEv(e, "Stumble"));
    CHECK(e.fsm->currentStateName == "Ragdoll");
    CHECK(flag.fail == 1);
    CHECK(flag.complete == 1);
    CHECK(Count(*k.crew, "PlantFlag", "Kerbin") == 1);   // no second credit
    // no flags left: the event goes inactive; AddFlag brings it back once the kerbal stands again
    e.isRagdoll = false;
    e.AddFlag(2);
    CHECK(e.flagItems() == 2);
    CHECK(e.Events()["PlantFlag"]->active == e.CanPlantFlag());
    e.isRagdoll = true;
    CHECK_FALSE(e.CanPlantFlag());
    e.isRagdoll = false;
    e.InConstructionMode = true;
    CHECK_FALSE(e.CanPlantFlag());
    e.InConstructionMode = false;
    // the label of the PlantFlag event carries the number of flags (flagItems setter)
    e.set_flagItems(5);
    CHECK(e._flags() == 5);
    e.fsm->StartFSM(e.st_idle_gr);
}

// KerbalEVA.cs:2953 / :2962 / :6127 / :3061-3266: construction mode and welding
TEST_CASE("WP-32 construction mode and welding: enter / exit, Weld needs an engineer, heading, line of sight, interrupt") {
    if (!GameAvailable()) return;
    inv33::Reset33 r33;
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    KerbalEVA::InstallHostHooks();
    // the construction controller exists (Weld does nothing without it)
    auto target = w.g.New("evaRepairKit");
    target->partTransform.position = Vector3f(0.0f, 0.0f, 5.0f);
    e.Weld(target.get());
    CHECK(e.fsm->currentStateName == "Idle (Grounded)");
    EVAConstructionModeController controller;
    EVAConstructionModeEditor editor;
    controller.evaEditor = &editor;
    controller.Start();
    CHECK(EVAConstructionModeController::Instance == &controller);
    // not an engineer: error, no event
    KerbalRoster::SetExperienceTrait(*k.crew, std::string("Pilot"));
    e.Weld(target.get());
    CHECK(e.fsm->currentStateName == "Idle (Grounded)");
    CHECK(e.constructionTarget == nullptr);
    // an engineer
    k.crew->experienceTrait.reset();
    KerbalRoster::SetExperienceTrait(*k.crew, std::string("Engineer"));
    e.Weld(target.get());
    CHECK(e.constructionTarget == target.get());
    CHECK(e.fsm->currentStateName == "Weld Turn to face Part");
    CHECK(e.tgtRpos == Vector3f(0.0f, 0.0f, 1.0f));       // SetWaypoint to the part
    bool weldLock = false;
    for (const auto& l : EvaEnvironment::controlLockLog) weldLock |= l == "set:WeldLock_" + k.vessel->id.ToString();
    CHECK(weldLock);
    // facing the part already (angle 0 < 3 deg): the next fixed update starts the weld; with a clear line of sight the weld animation plays
    Part* tp = target.get();
    PartModelRenderer rend;
    rend.name = "m";
    rend.bounds.set_center(Vector3f(0.0f, 0.0f, 5.0f));
    rend.bounds.set_extents(Vector3f(0.1f, 0.1f, 0.1f));
    tp->modelRenderers.push_back(rend);
    EvaCollider hitCol;
    hitCol.part = tp;
    k.engine.raycast = [&](const Vector3f&, const Vector3f&, float, int, EvaRaycastHit& hit) {
        hit.collider = &hitCol;
        return true;
    };
    e.weldFX = std::make_unique<EvaWeldFx>();
    e.fsm->FixedUpdateFSM();
    CHECK(e.fsm->currentStateName == "Welding part");
    CHECK(e.weldFX->playing);
    CHECK(e.visorState == VisorStates::Lowering);          // LowerVisor(forceHelmet: true)
    CHECK(e.isHelmetEnabled());
    // InterruptWeld: the weld ends, the lock goes, the visor is raised again (it was not lowered before)
    e.InterruptWeld();
    CHECK(e.fsm->currentStateName == "Idle (Grounded)");
    CHECK_FALSE(e.weldFX->playing);
    CHECK(e.visorState == VisorStates::Raising);
    bool weldUnlock = false;
    for (const auto& l : EvaEnvironment::controlLockLog) weldUnlock |= l == "remove:WeldLock_" + k.vessel->id.ToString();
    CHECK(weldUnlock);
    // no line of sight: the weld state ends at once (RunEvent(On_weldComplete) from OnEnter)
    k.engine.raycast = nullptr;
    e.Weld(target.get());
    REQUIRE(e.fsm->currentStateName == "Weld Turn to face Part");
    e.fsm->FixedUpdateFSM();
    CHECK(e.fsm->currentStateName == "Idle (Grounded)");
    CHECK(e.fsm->lastEventName == "Weld Part Complete");
    // construction mode entry/exit from the controller hooks
    CHECK(EVAConstructionModeController::hooks.hasEvaController(*k.vessel));
    EVAConstructionModeController::hooks.evaEnterExitConstruction(true);
    CHECK(e.InConstructionMode);
    CHECK(e.fsm->currentStateName == "Entering construction mode");
    CHECK(EVAConstructionModeController::hooks.activeEvaInConstructionMode());
    EVAConstructionModeController::hooks.evaEnterExitConstruction(false);
    CHECK_FALSE(e.InConstructionMode);
    CHECK(e.fsm->currentStateName == "Exiting construction mode");
    // on a ladder construction mode only flips the flag
    e.fsm->StartFSM(e.st_idle_gr);
    e.onLadder = true;
    e.EnterConstructionMode();
    CHECK(e.InConstructionMode);
    CHECK(e.fsm->currentStateName == "Idle (Grounded)");
    e.onLadder = false;
    e.InConstructionMode = false;
    // the placement-mode hook of the inventory and the jetpack hooks
    FlightRegistry::ActiveVessel = k.vessel.get();
    CHECK(InventoryEnvironment::setPartPlacementMode(true, nullptr));
    CHECK(e.PartPlacementMode());
    CHECK(InventoryEnvironment::setPartPlacementMode(false, nullptr));
    CHECK_FALSE(e.PartPlacementMode());
    e.HasJetpack = true;
    CHECK_FALSE(InventoryEnvironment::jetpackDeployed());
    InventoryEnvironment::toggleJetpack();   // KerbalEVA.ToggleJetpack(): Pack Toggle event
    CHECK(e.JetpackDeployed());
    CHECK(InventoryEnvironment::jetpackDeployed());
    CHECK(EVAConstructionEnvironment::isSeated(*k.vessel) == false);
    CHECK(EVAConstructionEnvironment::engineerCount(*k.vessel) == 1);
    EVAConstructionModeController::Instance = nullptr;
}

// KerbalEVA.cs:4662 IsKerbalInStateAbleToDeployParachute, :4690-4775 the parachute states
TEST_CASE("WP-32 parachute states: able-to-deploy table, semi / fully deployed, cut, the chute host callbacks") {
    if (!GameAvailable()) return;
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    // the four states that can deploy (idle floating, ragdoll, semi deployed, seated)
    for (KFSMState* st : e.fsm->GetStates()) {
        // states whose OnEnter needs a ladder, a clamber path, a seat or a pose (physics bubble / WP-19b): their membership in the set is covered
        // by the table test (the able-to-deploy predicate is a pure comparison of CurrentState)
        if (st->name.rfind("Ladder", 0) == 0 || st->name.rfind("Clamber", 0) == 0 || st->name.rfind("Seated", 0) == 0 || st->name.rfind("Weld", 0) == 0 ||
            st->name.find("Parachute") != std::string::npos)
            continue;   // (the chute states are exercised below with a deployed chute: leaving them cuts it)
        try { e.fsm->StartFSM(st); } catch (const std::exception& ex) { FAIL_CHECK(st->name << ": " << ex.what()); continue; }
        const bool want = st->name == "Idle (Floating)" || st->name == "Ragdoll" || st->name == "Semi Deployed Parachute" || st->name == "Seated (Command)";
        INFO(st->name);
        CHECK(e.IsKerbalInStateAbleToDeployParachute() == want);
        CHECK(e.IsChuteState() == (st->name == "Semi Deployed Parachute" || st->name == "Fully Deployed Parachute"));
    }
    if (e.evaChute == nullptr) {   // the chute module is the stowed evaChute part's; give the kerbal one so the host callbacks run (SYNTHETIC wiring)
        k.part->AddModule("ModuleEvaChute");
        e.evaChute = dynamic_cast<ModuleEvaChute*>(k.part->FindModuleOfClass("ModuleEvaChute"));
    }
    REQUIRE(e.evaChute != nullptr);
    e.evaChute->hasCanopy = true;   // SYNTHETIC: the canopy object of the chute prefab (engine side)
    e.fsm->StartFSM(e.st_idle_fl);
    e.SyncEvaChuteHost();
    CHECK(e.evaChute->kerbalEVA.ableToDeployParachute);
    CHECK_FALSE(e.evaChute->kerbalEVA.IsChuteState);
    // OnParachuteSemiDeployed: the chute tells the kerbal; Semi Deployed Parachute (OnEnter deploys the chute and un-ragdolls)
    e.evaChute->OnParachuteSemiDeployed();
    CHECK(e.evaChute->kerbalEVA.onParachuteSemiDeployedCalls == 1);
    CHECK(e.fsm->currentStateName == "Semi Deployed Parachute");
    CHECK(e.IsKerbalInStateAbleToDeployParachute());
    CHECK(e.evaChute->deploymentState == ModuleParachute::deploymentStates::ACTIVE);
    e.OnParachuteFullyDeployed();
    CHECK(e.fsm->currentStateName == "Fully Deployed Parachute");
    CHECK(e.IsChuteState());
    // cutting from the fully deployed state: back to floating
    e.evaChute->deploymentState = ModuleParachute::deploymentStates::DEPLOYED;
    e.OnParachuteCut();
    CHECK(e.fsm->currentStateName == "Idle (Floating)");
    CHECK(e.fsm->lastEventName == "Parachute Cut");
    // OnParachuteCut with the state name out of step with the state (mid-transition) does nothing
    e.fsm->currentStateName = "something else";
    e.OnParachuteCut();
    CHECK(e.fsm->currentStateName == "something else");
    e.fsm->currentStateName = e.fsm->CurrentState()->name;
}

// KerbalEVA.cs:5148 OnCollisionEnter, :5185 CompareContactsByNormalToSurface, :5209 isSelfCollision, :5243 SurfaceContact, :5261 GetL19Contact
TEST_CASE("WP-32 collisions: contact ordering, last collision memory, stumble threshold, self collisions, layer-19 ground contact") {
    if (!GameAvailable()) return;
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    EvaEnvironment::universalTime = [] { return 500.0; };
    k.vessel->upAxis = Vector3d(0.0, 1.0, 0.0);
    k.part->partTransform.position = Vector3f(0.0f, 1.0f, 0.0f);
    // contact points: the one whose normal is the most vertical wins (CompareContactsByNormalToSurface)
    KerbalEVA::EvaCollision c;
    c.layer = 0;
    c.relativeVelocitySqrMagnitude = 1.0f;
    c.contacts = {{Vector3f(1.0f, 0.0f, 0.0f), Vector3f(1.0f, 0.0f, 0.0f)}, {Vector3f(0.0f, 0.0f, 2.0f), Vector3f(0.0f, 1.0f, 0.0f)},
                  {Vector3f(0.0f, 0.0f, 3.0f), Vector3f(0.6f, 0.8f, 0.0f)}};
    e.Ready = false;
    e.OnCollisionEnter(c);       // not ready: ignored
    CHECK(e.lastCollisionTime == 0.0);
    e.Ready = true;
    k.vessel->packed = true;
    e.OnCollisionEnter(c);       // packed: ignored
    CHECK(e.lastCollisionTime == 0.0);
    k.vessel->packed = false;
    e.OnCollisionEnter(c);
    CHECK(e.lastCollisionNormal == Vector3f(0.0f, 1.0f, 0.0f));
    CHECK(e.lastCollisionDirection == (Vector3f(0.0f, 0.0f, 2.0f) - Vector3f(0.0f, 1.0f, 0.0f)).normalized());
    CHECK(e.lastCollisionTime == 500.0);
    CHECK(e.fsm->currentStateName == "Idle (Grounded)");      // 1 m/s: no stumble
    // a collision with the kerbal's own body does not refresh the memory
    EvaCollider own;
    e.characterColliders.push_back(&own);
    EvaEnvironment::universalTime = [] { return 600.0; };
    c.collider = &own;
    e.OnCollisionEnter(c);
    CHECK(e.lastCollisionTime == 500.0);
    CHECK(e.isSelfCollision(c));
    c.collider = nullptr;
    CHECK_FALSE(e.isSelfCollision(c));
    // above the stumble threshold: Stumble -> Ragdoll (sqr relative velocity 3.5^2 = 12.25 is not above)
    const float st = e.stumbleThreshold();
    c.relativeVelocitySqrMagnitude = f32store(fx(st) * fx(st));
    e.OnCollisionEnter(c);
    CHECK(e.fsm->currentStateName == "Idle (Grounded)");
    c.relativeVelocitySqrMagnitude = std::nextafter(c.relativeVelocitySqrMagnitude, 100.0f);
    e.OnCollisionEnter(c);
    CHECK(e.fsm->currentStateName == "Ragdoll");
    CHECK(e.isRagdoll);
    e.fsm->StartFSM(e.st_idle_gr);
    e.isRagdoll = false;
    // a dead part never stumbles
    k.part->set_State(PartStates::DEAD);
    c.relativeVelocitySqrMagnitude = 100.0f;
    e.OnCollisionEnter(c);
    CHECK(e.fsm->currentStateName == "Idle (Grounded)");
    k.part->set_State(PartStates::IDLE);
    // the ROC tag shows the pick-up event
    KerbalEVA::EvaCollision roc;
    roc.layer = 5;
    roc.isRoc = true;
    roc.relativeVelocitySqrMagnitude = 0.0f;
    e.availableROCSmall = true;
    e.OnCollisionEnter(roc);
    CHECK(e.Events()["PickUpROC"]->guiActive);
    e.OnCollisionExit(roc);
    CHECK_FALSE(e.Events()["PickUpROC"]->guiActive);
    // SurfaceContact: Landed, or a layer-19 contact when not splashed, low and slow
    k.vessel->Landed = false;
    k.vessel->Splashed = false;
    k.vessel->heightFromTerrain = 1.0f;
    k.vessel->srfSpeed = 1.0;
    CHECK_FALSE(e.SurfaceContact());
    e.currentCollisionLayers = {0, 19};
    CHECK(e.SurfaceContact());
    CHECK(e.SurfaceOrSplashed());
    k.vessel->srfSpeed = 10.0;
    CHECK_FALSE(e.SurfaceContact());
    k.vessel->srfSpeed = 1.0;
    k.vessel->heightFromTerrain = 1000.0f;
    CHECK_FALSE(e.SurfaceContact());
    k.vessel->heightFromTerrain = 1.0f;
    k.vessel->Splashed = true;
    CHECK_FALSE(e.SurfaceContact());
    CHECK(e.SurfaceOrSplashed());
    e.currentCollisionLayers.clear();
    EvaEnvironment::universalTime = nullptr;
}

// KerbalEVA.cs:5374 SetRagdoll, :5459 updateRagdollVelocities, KerbalRagdollNode.cs:18
TEST_CASE("WP-32 ragdoll bookkeeping: SetRagdoll toggles nodes / colliders / animation; node velocities follow the kinematic update") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    e.ragdollNodes.resize(3);
    for (auto& n : e.ragdollNodes) n.velocity = Vector3f(1.0f, 2.0f, 3.0f);
    k.engine.velocity = Vector3f(7.0f, 8.0f, 9.0f);
    k.engine.animEnabled = true;
    e.sciencePanelAnimPlaying = true;
    e.partPlacementMode = false;
    e.SetRagdoll(true);
    CHECK_FALSE(k.engine.animEnabled);
    for (size_t i = 0; i < 3; i++) {
        CHECK(e.ragdollNodes[i].goActive);
        CHECK(k.engine.nodes[i].active);
        CHECK(k.engine.nodes[i].velocity == Vector3f(1.0f, 2.0f, 3.0f));
    }
    CHECK_FALSE(k.engine.characterCollidersEnabled);
    CHECK(k.engine.otherRagdollCollidersEnabled);
    CHECK_FALSE(e.sciencePanelAnimPlaying);     // the interaction animations are cut short
    // back: nodes off, their velocity is the rigidbody's, the colliders swap, the animation returns (unless the pose is preserved)
    e.SetRagdoll(false);
    CHECK(k.engine.animEnabled);
    for (size_t i = 0; i < 3; i++) {
        CHECK_FALSE(e.ragdollNodes[i].goActive);
        CHECK(e.ragdollNodes[i].velocity == Vector3f(7.0f, 8.0f, 9.0f));
    }
    CHECK(k.engine.characterCollidersEnabled);
    CHECK_FALSE(k.engine.otherRagdollCollidersEnabled);
    e.SetRagdoll(true);
    e.SetRagdoll(false, true);
    CHECK_FALSE(k.engine.animEnabled);            // preservePose: the animation stays off
    k.engine.animEnabled = true;
    // updateVelocity: (vector - lastPos) * fdtRecip + rootVel in double, narrowed
    int n = 0;
    for (const auto& c : ref["ragdoll"]) {
        KerbalRagdollNode node;
        node.goPosition = V3(c["go"]);
        node.lastPos = V3(c["last"]);
        node.updateVelocity(V3(c["root"]), Vector3d(c["rv"][0].get<double>(), c["rv"][1].get<double>(), c["rv"][2].get<double>()), F(c["fdt"]));
        const std::string what = "ragdoll case " + std::to_string(n++);
        CheckV3(node.velocity, c["vel"], what + " velocity");
        CheckV3(node.lastPos, c["lastOut"], what + " lastPos");
    }
    CHECK(n == 60);
    // updateRagdollVelocities drives every node from the rigidbody velocity (and does nothing while packed)
    for (auto& nd : e.ragdollNodes) nd.goPosition = Vector3f(0.5f, 0.5f, 0.5f);
    k.vessel->packed = true;
    for (auto& nd : e.ragdollNodes) nd.velocity = Vector3f();
    e.updateRagdollVelocities();
    CHECK(e.ragdollNodes[0].velocity == Vector3f());
    k.vessel->packed = false;
    Time::fixedDeltaTime = 0.02f;
    e.updateRagdollVelocities();
    CHECK(e.ragdollNodes[0].lastPos == Vector3f(0.5f, 0.5f, 0.5f) - k.part->partTransform.position);
    // the ragdoll integration: a reference-frame change holds the physics for two frames (updateRagdollVelocities instead of the forces)
    e.referenceFrameChanged_rdPhysHold = 2;
    e.IntegrateRagdollRigidbodyForces();
    CHECK(e.referenceFrameChanged_rdPhysHold == 1);
    CHECK(k.engine.nodes[0].forces.empty());
}

// KerbalEVA.cs:7047 ToggleHelmetAndNeckRing, :6141 visor events, :4990 ToggleLamp
TEST_CASE("WP-32 helmet, neck ring, visor events and the lamp") {
    if (!GameAvailable()) return;
    EvaWorld w;
    inv33::LoadLocalizer();
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    int changed = 0;
    struct H {
        int* n;
        void C(KerbalEVA*, bool, bool) { (*n)++; }
    } h{&changed};
    GameEvents::OnHelmetChanged.Add(&h, &H::C);
    // safe air: Kerbin-like atmosphere with oxygen, 1 atm, 300 K
    CelestialBody& b = w.Kerbin();
    b.atmosphere = true;
    b.atmosphereDepth = 70000.0;
    b.atmosphereContainsOxygen = true;
    k.vessel->altitude = 100.0;
    k.part->fields.staticPressureAtm = 1.0;
    k.part->fields.skinTemperature = 300.0;
    k.part->fields.temperature = 300.0;
    e.set_isHelmetEnabled(true);
    e.set_isNeckRingEnabled(true);
    e.Ready = true;
    e.ToggleHelmet(false);
    CHECK_FALSE(e.isHelmetEnabled());
    CHECK_FALSE(e.helmetActiveSelf);
    CHECK_FALSE(k.crew->hasHelmetOn);
    CHECK(e.helmetColliderRadius == e.helmetColliderSetup.helmetOffRadius);
    CHECK(changed == 1);
    // helmet off, neck ring stays visible only if requested
    e.ToggleNeckRing(false);
    CHECK_FALSE(e.neckRingActiveSelf);
    CHECK_FALSE(k.crew->hasNeckRingOn);
    CHECK(changed == 2);
    e.ChangeNeckRing();                  // ToggleNeckRing(!isNeckRingEnabled || isHelmetEnabled) -> on
    CHECK(e.isNeckRingEnabled());
    e.ChangeHelmet();                    // helmet back on (also forces the neck ring)
    CHECK(e.isHelmetEnabled());
    CHECK(e.helmetActiveSelf);
    CHECK(e.helmetColliderRadius == e.helmetColliderSetup.helmetOnRadius);
    // unsafe air: the helmet stays on and a message with the reason is posted
    k.part->fields.staticPressureAtm = 0.05;
    EvaEnvironment::postedMessages.clear();
    const int before = changed;
    e.ToggleHelmet(false);
    CHECK(e.isHelmetEnabled());
    CHECK(changed == before);
    REQUIRE(EvaEnvironment::postedMessages.size() == 1);
    CHECK(e.HelmetUnsafeReason() == e.cacheAutoLOC_6010009);
    // not the active vessel and Ready: ignored
    k.part->fields.staticPressureAtm = 1.0;
    FlightRegistry::ActiveVessel = nullptr;
    e.ToggleHelmet(false);
    CHECK(e.isHelmetEnabled());
    FlightRegistry::ActiveVessel = k.vessel.get();
    // the helmet transform does not exist: error, nothing changes
    e.helmetTransformExists = false;
    e.ToggleHelmet(false);
    CHECK(e.isHelmetEnabled());
    e.helmetTransformExists = true;
    // visor: Lower with the helmet on -> Lowering, the event labels flip; Raise with restoreHelmet false keeps the helmet
    e.LowerVisor();
    CHECK(e.VisorState() == VisorStates::Lowering);
    CHECK_FALSE(e.Events()["LowerVisor"]->guiActive);
    CHECK(e.Events()["RaiseVisor"]->guiActive);
    e.RaiseVisor();
    CHECK(e.VisorState() == VisorStates::Raising);
    CHECK(e.Events()["LowerVisor"]->guiActive);
    // forceHelmet puts the helmet back first
    e.ToggleHelmet(false);
    REQUIRE_FALSE(e.isHelmetEnabled());
    e.LowerVisor(false);          // helmet off, not forced: nothing
    CHECK(e.VisorState() == VisorStates::Raising);
    e.LowerVisor(true);
    CHECK(e.isHelmetEnabled());
    CHECK(e.VisorState() == VisorStates::Lowering);
    // lamp: only the active vessel's kerbal
    e.set_lampOn(false);
    e.ToggleLamp();
    CHECK(e.lampOn());
    e.ToggleLamp();
    CHECK_FALSE(e.lampOn());
    FlightRegistry::ActiveVessel = nullptr;
    e.ToggleLamp();
    CHECK_FALSE(e.lampOn());
    FlightRegistry::ActiveVessel = k.vessel.get();
    GameEvents::OnHelmetChanged.Remove(&h, &H::C);
}
