// WP-32 unit tests, part 5: behaviour of KerbalEVA beyond the arithmetic: the OnStart / StartEVA lifecycle, persistence (OnLoad / OnSave), the EVA
// propellant bookkeeping with the real inventory, boarding, flag planting, parachute hooks, construction / weld hooks, ragdoll bookkeeping, collisions.
// The expected values are read off the C# (cited per case); SYNTHETIC: the engine answers (rays, colliders, animation clip lengths).
#include "eva_fixture.hpp"

#include "ksp/events/game_events.hpp"
#include "ksp/modules/parachutes.hpp"

using namespace ksp;
using namespace ksp::eva32;

namespace {

KFSMState* StateByName(KerbalEVA& e, const std::string& n) {
    for (KFSMState* s : e.fsm->GetStates())
        if (s->name == n) return s;
    return nullptr;
}

struct Crew {
    std::vector<std::string> log;
    void Board(GameEvents::FromToAction<Part*, Part*>) { log.push_back("board"); }
    void Xfer(GameEvents::HostedFromToAction<ProtoCrewMember*, Part*>) { log.push_back("transferred"); }
    Crew() {
        GameEvents::onCrewBoardVessel.Add(this, &Crew::Board);
        GameEvents::onCrewTransferred.Add(this, &Crew::Xfer);
    }
    ~Crew() {
        GameEvents::onCrewBoardVessel.Remove(this, &Crew::Board);
        GameEvents::onCrewTransferred.Remove(this, &Crew::Xfer);
    }
};

}  // namespace

// KerbalEVA.cs:1505 OnStart, :1753 StartEVA
TEST_CASE("WP-32 OnStart / StartEVA: the FSM starts in the state the situation decides, the mass fields are scaled, Ready is set after one frame") {
    if (!GameAvailable()) return;
    EvaWorld w;
    for (int situation = 0; situation < 3; situation++) {
        EvaKerbal k(w);
        KerbalEVA& e = *k.eva;
        e.Animations().syncLayers = {0, 1};
        k.vessel->Landed = situation == 0;
        k.vessel->Splashed = situation == 1;
        const float massMult = e.massMultiplier();
        const float push = e.ladderPushoffForce(), jump = e.maxJumpForce(), bound = e.boundForce(), lin = e.linPower();
        e.OnStart(StartState::Landed);
        // the first half of the iterator ran: the animation manager, ragdoll, the FSM exist; Ready only after the yield
        REQUIRE(e.fsm != nullptr);
        CHECK_FALSE(e.Ready);
        CHECK(e.Animations().RandomIdleAnims.size() == 7);
        CHECK(e.Animations().controlPanelAnims.size() == 3);
        CHECK(k.engine.syncedLayers.size() == 2);  // SyncAnimationLayers at the end of Start
        CHECK(Bits(e.ladderPushoffForce()) == Bits(f32store(fx(push) * fx(massMult))));
        CHECK(Bits(e.maxJumpForce()) == Bits(f32store(fx(jump) * fx(massMult))));
        CHECK(Bits(e.boundForce()) == Bits(f32store(fx(bound) * fx(massMult))));
        CHECK(Bits(e.linPower()) == Bits(f32store(fx(lin) * fx(massMult))));
        // part mass = initialMass * massMultiplier (SetupRagdoll)
        CHECK(Bits(k.part->fields.mass) == Bits(f32store(fx(e.initialMass()) * fx(massMult))));
        CHECK_FALSE(e.Events()["PlantFlag"]->active);
        CHECK_FALSE(e.Events()["OnDeboardSeat"]->active);
        Pump(w.sched, 2);
        CHECK(e.Ready);
        REQUIRE(e.fsm->Started());
        const std::string want = situation == 0 ? "Idle (Grounded)" : (situation == 1 ? "Swim (Idle)" : "Idle (Floating)");
        CHECK(e.fsm->currentStateName == want);
        // the kerbal reports its ladder only once the FSM runs
        CHECK_FALSE(e.OnALadder());
        e.OnDestroy();
    }
}

TEST_CASE("WP-32 StartEVA: a saved state name restores that state; a ladder state falls back to acquire / floating; a trigger grabs the ladder on start") {
    if (!GameAvailable()) return;
    EvaWorld w;
    {
        EvaKerbal k(w);
        KerbalEVA& e = *k.eva;
        e.loadedFromSFS = true;
        e.loadedStateName = "Swim (fwd)";
        e.OnStart(StartState::Landed);
        Pump(w.sched, 2);
        CHECK(e.fsm->currentStateName == "Swim (fwd)");
        CHECK(e.Ready);
    }
    {
        EvaKerbal k(w);
        KerbalEVA& e = *k.eva;
        e.loadedFromSFS = true;
        e.loadedStateName = "Ladder (Climb)";
        e.OnStart(StartState::Landed);
        Pump(w.sched, 2);
        CHECK(e.fsm->currentStateName == "Idle (Floating)");  // no trigger: floating
    }
    {
        EvaKerbal k(w);
        KerbalEVA& e = *k.eva;
        EvaCollider ladder;
        ladder.tag = "Ladder";
        e.OnStart(StartState::Landed);
        e.OnTriggerStay(&ladder);  // before the iterator resumes (the first yield): the autoGrab path
        Pump(w.sched, 2);
        CHECK(e.fsm->currentStateName == "Ladder (Acquire)");
        CHECK(e.currentLadder == &ladder);
        CHECK(e.OnALadder() == e.onLadder);
    }
    {
        EvaKerbal k(w);
        KerbalEVA& e = *k.eva;
        e.loadedFromSFS = true;
        e.loadedStateName = "No Such State";
        e.OnStart(StartState::Landed);
        Pump(w.sched, 2);
        CHECK(e.Ready);  // StartFSM threw; Ready = false then the generic branch started the FSM
        CHECK(e.fsm->Started());
    }
}

// KerbalEVA.cs:1426 OnLoad / :1846 OnSave
TEST_CASE("WP-32 OnLoad / OnSave: state, step, packExt, lightOn, flags, vInfo and the physic material round trip") {
    if (!GameAvailable()) return;
    EvaWorld w;
    EvaKerbal k(w);
    KerbalEVA& e = *k.eva;
    ConfigNode n("MODULE");
    n.AddValue("name", "KerbalEVA");
    n.AddValue("state", "Walk (FPS)");
    n.AddValue("step", "0.375");
    n.AddValue("packExt", "True");
    n.AddValue("lightOn", "True");
    n.AddValue("flags", "3");
    ConfigNode* v = n.AddNode("vInfo");
    v->AddValue("vesselName", "Valentina Kerman");
    v->AddValue("rootUId", "99");
    v->AddValue("vesselType", "EVA");
    n.AddValue("useGlobalPhysicMaterial", "False");
    n.AddValue("kerbalEVADynamicFriction", "0.25");
    n.AddValue("kerbalEVAStaticFriction", "0.5");
    n.AddValue("kerbalEVABounciness", "0.125");
    e.OnLoad(n);
    CHECK(e.loadedFromSFS);
    CHECK(e.loadedStateName == "Walk (FPS)");
    CHECK(e.lastBoundStep() == 0.375f);
    CHECK(e.JetpackDeployed());
    CHECK(e.lampOn());
    CHECK(e.flagItems() == 3);
    REQUIRE(e.kerbalVesselInfo != nullptr);
    CHECK(e.kerbalVesselInfo->name == "Valentina Kerman");
    CHECK(e.kerbalVesselInfo->rootPartUId == 99u);
    CHECK_FALSE(e.useGlobalPhysicMaterial());
    REQUIRE(e.physicMaterial.has_value());
    CHECK(e.physicMaterial->dynamicFriction == 0.25f);
    CHECK(e.physicMaterial->staticFriction == 0.5f);
    CHECK(e.physicMaterial->bounciness == 0.125f);
    // an isCfg node is the prefab config: nothing of the save is read and no rigidbody is added
    ConfigNode cfg("MODULE");
    cfg.AddValue("isCfg", "True");
    EvaKerbal k2(w);
    k2.eva->OnLoad(cfg);
    CHECK_FALSE(k2.eva->loadedFromSFS);
    CHECK(k2.eva->useGlobalPhysicMaterial());
    CHECK(k2.eva->flagItems() == k2.eva->_flags());
    // OnSave writes the state only while the FSM runs; the physic material when it is not the global one
    ConfigNode out("MODULE");
    e.OnSave(out);
    CHECK_FALSE(out.HasValue("state"));
    CHECK(out.GetValue("kerbalEVADynamicFriction").value_or("") == "0.25");
    k.Start();
    ConfigNode out2("MODULE");
    e.OnSave(out2);
    CHECK(out2.GetValue("state").value_or("") == "Idle (Grounded)");
    REQUIRE(out2.HasNode("vInfo"));
    CHECK(out2.GetNode("vInfo")->GetValue("vesselName").value_or("") == "Valentina Kerman");
}

// KerbalEVA.cs:1344 SetupEVAFuel, :1379 ProcessEVAFuel, :1249 UpdatePackModels
TEST_CASE("WP-32 EVA propellant and pack models: the jetpack in the inventory is the fuel tank; models follow the stored parts") {
    if (!GameAvailable()) return;
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    ModuleInventoryPart* inv = inv33::Inv(*k.part);
    inv->OnStart(StartState::Landed);
    e.moduleInventoryPartReference = inv;
    // the cfg values of the stored parts (Squad/Parts/Cargo/Jetpack/Jetpack.cfg: EVA Propellant 5 / 5, resourcePriorityOffset 2)
    REQUIRE(inv->StoreCargoPartAtSlot(&inv33::Prefab(w.g, "evaJetpack"), 0));
    REQUIRE(inv->StoreCargoPartAtSlot(&inv33::Prefab(w.g, "evaChute"), 1));
    e.ProcessEVAFuel();
    REQUIRE(e.propellantResource != nullptr);
    CHECK(e.Fuel() == 5.0);
    CHECK(e.FuelCapacity() == 5.0);
    REQUIRE(e.inventoryPropellantResources.size() == 1);
    CHECK(e.inventoryPropellantResources[0].priority == 2);
    // the resource is off the part's list (C# keeps the reference after Resources.Remove)
    CHECK_FALSE(k.part->Resources.Contains("EVA Propellant"));
    // a second jetpack with another priority and amount: totals add, the list is sorted by priority (the runtime's introspective sort)
    REQUIRE(inv->StoreCargoPartAtSlot(&inv33::Prefab(w.g, "evaJetpack"), 2));
    {
        auto sp = inv->storedParts.ValuesList();
        std::shared_ptr<StoredPart> second;
        for (const auto& p : sp)
            if (p && p->slotIndex == 2) second = p;
        REQUIRE(second != nullptr);
        second->snapshot->resourcePriorityOffset = -1;
        second->snapshot->resources[0]->amount = 1.5;
    }
    e.ProcessEVAFuel();
    CHECK(e.Fuel() == 6.5);
    CHECK(e.FuelCapacity() == 10.0);
    REQUIRE(e.inventoryPropellantResources.size() == 2);
    CHECK(e.inventoryPropellantResources[0].priority == -1);
    CHECK(e.inventoryPropellantResources[1].priority == 2);
    // consumption: UpdatePackFuel drains the lowest priority snapshot first
    e.fuelFlowRate = 20.0f;   // double num = PropellantConsumption * fuelFlowRate (float * float: the exact product)
    const double num = fx(e.PropellantConsumption()) * 20.0;
    e.UpdatePackFuel();
    CHECK(e.inventoryPropellantResources[0].pPResourceSnapshot->amount == 1.5 - num);
    CHECK(e.inventoryPropellantResources[1].pPResourceSnapshot->amount == 5.0);
    CHECK(e.Fuel() == 6.5 - num);
    // UpdatePackModels: jetpack + chute stored (3 parts) -> the jetpack model; HasJetpack follows
    e.UpdatePackModels();
    CHECK(e.HasJetpack);
    CHECK(e.packModelActive["Jetpack"]);
    CHECK(e.HasParachute);
    const bool chuteUsable = e.evaChute != nullptr && e.evaChute->CanCrewMemberUseParachute();
    CHECK(e.packModelActive["Backpack"] == !chuteUsable);          // jetpack present, > 1 item: Backpack unless the chute can be used
    CHECK(e.packModelActive["ChuteJetpack"] == chuteUsable);
    CHECK_FALSE(e.packModelActive["Storage"]);
    CHECK(e.Fields()["thrustPercentage"]->guiActive);
    // remove both jetpacks: the pack goes, the jetpack is stowed (ToggleJetpack(false)) and the thrust slider hides
    e.set_JetpackDeployed(true);
    inv->ClearPartAtSlot(0);
    inv->ClearPartAtSlot(2);
    e.UpdatePackModels();
    CHECK_FALSE(e.HasJetpack);
    CHECK_FALSE(e.packModelActive["Jetpack"]);
    CHECK_FALSE(e.JetpackDeployed());
    CHECK_FALSE(e.Fields()["thrustPercentage"]->guiActive);
    // one item left (the chute): the slim models
    CHECK(inv->InventoryItemCount() == 1);
    CHECK(e.packModelActive["ChuteSt"] == chuteUsable);
    CHECK(e.packModelActive["BackpackSt"] == !chuteUsable);
    // nothing stored: no pack model at all
    inv->ClearPartAtSlot(1);
    e.UpdatePackModels();
    for (const char* n : {"Backpack", "BackpackSt", "Storage", "StorageSlim", "ChuteJetpack", "ChuteSt", "ChuteContainer", "Jetpack"}) CHECK_FALSE(e.packModelActive[n]);
    CHECK_FALSE(e.HasParachute);
}

// KerbalEVA.cs:6615 BoardPart .. :6720 proceedAndBoard
TEST_CASE("WP-32 boarding: the checks in order (boarding allowed, capacity, inventory, science), the crew move and the events") {
    if (!GameAvailable()) return;
    EvaWorld w;
    inv33::LoadLocalizer();
    Crew crew;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    // the target: a command pod in a landed vessel
    std::unique_ptr<Part> pod = w.g.New("mk1pod.v2");
    pod->fields.CrewCapacity = 1;
    std::unique_ptr<Vessel> podVessel = inv33::BareVessel(VesselType::Ship, Vector3f(5.0f, 0.0f, 0.0f));
    podVessel->orbitDriver = std::make_unique<OrbitDriver>();
    podVessel->orbitDriver->orbit->referenceBody = &w.Kerbin();
    pod->vessel = podVessel.get();
    podVessel->parts.push_back(pod.get());
    podVessel->rootPart = pod.get();
    FlightRegistry::VesselsLoaded.push_back(podVessel.get());
    auto jeb = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Jebediah Kerman");
    // 1. flight parameter CanBoard off
    EvaEnvironment::canBoard = [] { return false; };
    e.BoardPart(pod.get());
    REQUIRE(EvaEnvironment::postedMessages.size() == 1);
    CHECK(EvaEnvironment::postedMessages[0] == KSP::Localization::Localizer::Format("#autoLOC_115948"));
    EvaEnvironment::canBoard = nullptr;
    EvaEnvironment::postedMessages.clear();
    // 2. no free seat
    pod->protoModuleCrew.push_back(jeb);
    e.BoardPart(pod.get());
    REQUIRE(EvaEnvironment::postedMessages.size() == 1);
    CHECK(EvaEnvironment::postedMessages[0] == KSP::Localization::Localizer::Format("#autoLOC_115954"));
    CHECK(crew.log.empty());
    pod->protoModuleCrew.clear();
    EvaEnvironment::postedMessages.clear();
    // 3. science aboard the EVA vessel and no container in the pod: the dialog is shown, nothing moves; "proceed" boards
    e.scienceDataCount = [](Part&) { return 2; };
    std::function<void()> proceed;
    e.popupStoreExperimentsIssue = [&](const std::string&, std::function<void()> p) { proceed = std::move(p); };
    e.BoardPart(pod.get());
    CHECK(e.boardingPopupShown);
    CHECK(crew.log.empty());
    CHECK(pod->protoModuleCrew.empty());
    // the container stores the data: boarding goes on without the dialog
    e.boardingPopupShown = false;
    e.storeScienceInContainer = [](Part&, Part&) { return true; };
    e.BoardPart(pod.get());
    CHECK_FALSE(e.boardingPopupShown);
    REQUIRE(pod->protoModuleCrew.size() == 1);
    CHECK(pod->protoModuleCrew[0] == k.crew);
    CHECK(k.part->protoModuleCrew.empty());
    CHECK(crew.log == std::vector<std::string>{"board", "transferred"});
    CHECK(k.vessel->state == Vessel::State::DEAD);   // base.vessel.Die()
    CHECK(FlightRegistry::ActiveVessel == podVessel.get());
    // the flight log got the BoardVessel entry with the body name
    bool logged = false;
    for (const auto& en : k.crew->flightLog.Entries()) logged |= en.type == "BoardVessel" && en.target == "Kerbin";
    CHECK(logged);
    CHECK(proceed != nullptr);   // the dialog's callback is only ever invoked by the UI
    pod->vessel = nullptr;
}
