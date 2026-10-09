// WP-15 unit tests, part 1: PartResource / PartResourceList, Part.TransferResource, the PartSet solver, Part.RequestResource per
// ResourceFlowMode, the flow graph (SCC) on the stock crafts.
//
// There is no golden for the flow numerics (no resource_flow capture exists: STATUS.md WP-15 "Needs"). Expectations are derived from
// the decompiled source (the derivation is in the comment above each check) or are properties the C# control flow guarantees
// (conservation, clamping, priority order, proportional split, SCC partition = BFS closure). Parts, resources, crafts and saves are
// the game's own (GameData mirror, harness/inputs/Ships, testdata/saves).
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

#include "resource_fixture.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::support;
using ksp::RUI::Algorithms::SCCFlowGraph;
using ksp::RUI::Algorithms::StackFlowGraph;
namespace fs = std::filesystem;

// ================================================== PartResource ============================================================

TEST_CASE("WP-15 PartResource: CanProvide / Flowing truth tables and the flowState / flowMode property events") {
    // PartResource.cs:183 CanProvide(demand): demand > 0 needs amount > 0, demand < 0 needs room, 0 never
    PartResource r;
    r.amount = 5.0;
    r.maxAmount = 10.0;
    CHECK(r.CanProvide(1.0));
    CHECK(r.CanProvide(-1.0));
    CHECK_FALSE(r.CanProvide(0.0));
    r.amount = 0.0;
    CHECK_FALSE(r.CanProvide(1.0));
    CHECK(r.CanProvide(-1.0));
    r.amount = 10.0;
    CHECK(r.CanProvide(1.0));
    CHECK_FALSE(r.CanProvide(-1.0));

    // :196 CanProvide(pulling) = amount condition && _flowState && (_flowMode & Out|In) > 0; :213 Flowing(pulling) drops the amount
    // condition. FlowMode: None 0, Out 1, In 2, Both 3.
    for (int mode = 0; mode <= 3; mode++)
        for (int state = 0; state <= 1; state++)
            for (double amount : {0.0, 5.0, 10.0}) {
                PartResource p;
                p.maxAmount = 10.0;
                p.amount = amount;
                p.flowState = state != 0;
                p.flowMode = static_cast<PartResourceFlowMode>(mode);
                const bool out = (mode & 1) != 0, in = (mode & 2) != 0;
                CHECK(p.Flowing(true) == (state && out));
                CHECK(p.Flowing(false) == (state && in));
                CHECK(p.CanProvide(true) == (amount > 0.0 && state && out));
                CHECK(p.CanProvide(false) == (amount < 10.0 && state && in));
            }

    if (!GameAvailable()) return;
    // flowState (:36) and flowMode (:53) are properties: the setter fires the event with (host, from, to) only when the value changed
    EventLog log;
    PartResource q;
    q.flowState = true;
    q.flowMode = PartResourceFlowMode::Both;
    q.SetFlowState(true);
    CHECK(log.events.empty());
    q.SetFlowState(false);
    q.SetFlowMode(PartResourceFlowMode::Both);
    q.SetFlowMode(PartResourceFlowMode::Out);
    CHECK(log.events == std::vector<std::string>{"FlowState:1>0", "FlowMode:3>1"});
    // Copy (:94) writes the backing fields: no event
    log.Clear();
    PartResource c;
    c.Copy(q);
    CHECK(log.events.empty());
    CHECK(c.flowMode == PartResourceFlowMode::Out);
    // the copy constructors set `part` from the source and, with a flag, simulationResource
    PartResource s(q, true);
    CHECK(s.simulationResource);
    CHECK(s.part == q.part);
}

TEST_CASE("WP-15 PartResourceList: dictionary order, duplicate refusal, flowing lookups, simulation twins, list-change events") {
    if (!GameAvailable()) return;
    Game15 g;
    auto tank = g.New("fuelTank");
    PartResourceList& R = tank->Resources;
    // PartResource cfg of fuelTankT400.cfg: LiquidFuel 180/180 then Oxidizer 220/220 (dict.At(index) is insertion order)
    REQUIRE(R.IsValid());
    REQUIRE(R.Count() == 2);
    CHECK(R.At(0).resourceName == "LiquidFuel");
    CHECK(R.At(1).resourceName == "Oxidizer");
    CHECK(R.At(0).amount == 180.0);
    CHECK(R.At(1).maxAmount == 220.0);
    CHECK(R.Contains("LiquidFuel"));
    CHECK(R.Contains(Id("Oxidizer")));
    CHECK_FALSE(R.Contains("Ore"));
    CHECK(R.Get("Ore") == nullptr);
    CHECK(R.HasFlowable());  // LiquidFuel flowMode STACK_PRIORITY_SEARCH (3) > NO_FLOW (0)
    CHECK(R.HasFlowableUnhidden());
    for (const PartResource& pr : R) CHECK(pr.part == tank.get());

    // GetFlowing / GetFlowingTotals / GetAll(Flowing)
    double amount = -1, maxAmount = -1;
    R.GetFlowingTotals(Id("LiquidFuel"), amount, maxAmount, true);
    CHECK(amount == 180.0);
    CHECK(maxAmount == 180.0);
    std::vector<PartResource*> all, flowing;
    CHECK(R.GetAll(all, Id("LiquidFuel")));
    CHECK(R.GetAllFlowing(flowing, Id("LiquidFuel")));
    CHECK(all.size() == 1);
    CHECK(flowing.size() == 1);
    R.Get("LiquidFuel")->flowState = false;
    CHECK(R.GetFlowing(Id("LiquidFuel"), true) == nullptr);
    R.GetFlowingTotals(Id("LiquidFuel"), amount, maxAmount, true);
    CHECK(amount == 0.0);
    CHECK(maxAmount == 0.0);
    flowing.clear();
    CHECK_FALSE(R.GetAllFlowing(flowing, Id("LiquidFuel")));
    R.Get("LiquidFuel")->flowState = true;
    R.Get("LiquidFuel")->flowMode = PartResourceFlowMode::In;  // push only
    CHECK(R.GetFlowing(Id("LiquidFuel"), true) == nullptr);
    CHECK(R.GetFlowing(Id("LiquidFuel"), false) != nullptr);
    R.Get("LiquidFuel")->flowMode = PartResourceFlowMode::Both;

    // Add(ConfigNode) refuses a duplicate and an unknown definition with a warning; Remove fires onPartResourceListChange
    EventLog events;
    LogCapture log;
    auto node = ConfigNode::Parse("RESOURCE\n{\n name = LiquidFuel\n amount = 1\n maxAmount = 1\n}\n");
    CHECK(R.Add(*node->nodes()[0]) == nullptr);
    // PartResourceList.cs:195: "Part already contains " + (simulationSet ? " simulation resource '" : " resource '") (sic: two spaces)
    CHECK(log.Has(log::Level::Warning, "Part already contains  resource 'LiquidFuel'"));
    auto bad = ConfigNode::Parse("RESOURCE\n{\n name = Unobtainium\n amount = 1\n maxAmount = 1\n}\n");
    CHECK(R.Add(*bad->nodes()[0]) == nullptr);
    CHECK(log.Has(log::Level::Warning, "Could not create PartResource of type 'Unobtainium"));
    CHECK(R.Add("Ore", 5.0, 10.0, true, true, false, true, PartResourceFlowMode::Both) != nullptr);
    CHECK(R.Count() == 3);
    CHECK(R.Remove("Ore"));
    CHECK_FALSE(R.Remove("Ore"));
    // Add(name, ...) assigns the properties flowState / flowMode (events from false / None), then fires the list change; Remove fires it
    CHECK(events.events == std::vector<std::string>{"FlowState:0>1", "FlowMode:0>3", "ListChange", "ListChange"});

    // simulation twins: SetupSimulationResources(refList, simulationSet: true) copies every resource with simulationResource = true;
    // RefreshSimulationListAmounts updates amount / _flowMode / _flowState of the existing ones (no events) and adds missing ones
    tank->SetupSimulationResources();
    PartResourceList sim(tank.get(), &R, true);
    REQUIRE(sim.Count() == 2);
    CHECK(sim.At(0).simulationResource);
    CHECK(sim.At(0).part == tank.get());
    CHECK(&sim.At(0) != &R.At(0));
    R.At(0).amount = 33.0;
    R.At(0).flowMode = PartResourceFlowMode::Out;
    events.Clear();
    sim.RefreshSimulationListAmounts(&R);
    CHECK(sim.At(0).amount == 33.0);
    CHECK(sim.At(0).flowMode == PartResourceFlowMode::Out);
    CHECK(events.events.empty());
    R.Add("Ore", 5.0, 10.0, true, true, false, true, PartResourceFlowMode::Both);
    sim.RefreshSimulationListAmounts(&R);
    CHECK(sim.Count() == 3);
    CHECK(sim.Get("Ore") != nullptr);
}

// ================================================== Part.TransferResource ===================================================

TEST_CASE("WP-15 Part.TransferResource: clamping, return values, flow-mode refusal, events, thermal mixing") {
    if (!GameAvailable()) return;
    Game15 g;
    auto tank = g.New("fuelTank");
    PartResource& lf = *tank->GetResource("LiquidFuel");  // 180 / 180, density 0.005, hsp 2010
    EventLog events;
    Part* self = tank.get();

    // Part.cs:6605. Negative `amount` takes from the resource and returns the (positive) amount taken, positive adds and returns the
    // negated amount added; both clamp at 0 / maxAmount. An event fires on every transition between empty / partial / full.
    CHECK(tank->TransferResource(lf, -30.0, self) == 30.0);  // pull 30: 180 -> 150  (amount2 == max: FullNonempty)
    CHECK(lf.amount == 150.0);
    CHECK(tank->TransferResource(lf, -1000.0, self) == 150.0);  // pull more than there is: returns amount2, amount 0 (NonemptyEmpty)
    CHECK(lf.amount == 0.0);
    CHECK(tank->TransferResource(lf, 10.0, self) == -10.0);  // push 10 into empty (EmptyNonempty)
    CHECK(lf.amount == 10.0);
    CHECK(tank->TransferResource(lf, 1000.0, self) == -170.0);  // push more than fits: -(max - amount) (NonemptyFull)
    CHECK(lf.amount == 180.0);
    CHECK(tank->TransferResource(lf, -1000.0, self) == 180.0);  // pull everything from a full tank (FullEmpty)
    CHECK(tank->TransferResource(lf, 1000.0, self) == -180.0);  // fill an empty tank (EmptyFull)
    CHECK(events.events == std::vector<std::string>{"FullNonempty", "NonemptyEmpty", "EmptyNonempty", "NonemptyFull", "FullEmpty", "EmptyFull"});

    // simulate = true: same arithmetic, no events
    events.Clear();
    CHECK(tank->TransferResource(lf, -10.0, self, true) == 10.0);
    CHECK(lf.amount == 170.0);
    CHECK(events.events.empty());

    // flowMode None refuses everything; Out refuses pushes, In refuses pulls; amount 0 is a no-op
    lf.flowMode = PartResourceFlowMode::None;
    CHECK(tank->TransferResource(lf, -10.0, self) == 0.0);
    CHECK(tank->TransferResource(lf, 10.0, self) == 0.0);
    lf.flowMode = PartResourceFlowMode::Out;
    CHECK(tank->TransferResource(lf, 10.0, self) == 0.0);
    CHECK(tank->TransferResource(lf, -10.0, self) == 10.0);
    lf.flowMode = PartResourceFlowMode::In;
    CHECK(tank->TransferResource(lf, -10.0, self) == 0.0);
    CHECK(tank->TransferResource(lf, 10.0, self) == -10.0);
    lf.flowMode = PartResourceFlowMode::Both;
    CHECK(tank->TransferResource(lf, 0.0, self) == 0.0);

    // TransferResource(int id, amount) on the part's own list; an unknown id logs an error and returns 0
    LogCapture log;
    CHECK(tank->TransferResource(Id("LiquidFuel"), -5.0) == 5.0);
    CHECK(tank->TransferResource(Id("Ore"), -5.0) == 0.0);
    CHECK(log.Has(log::Level::Error, "Resource System Error: Resource id"));

    // Thermal mixing (flight scene, other != this, not simulated): Part.cs:6620-6630 / 6646-6650. The receiving part's temperature is
    // the heat-capacity weighted mean: T = (T * thermalMass + c * T_other) / (thermalMass + c), c = amount * density * hsp (double,
    // density and hsp are floats widened).
    SceneScope flight(GameScenes::FLIGHT);
    auto other = g.New("fuelTank");
    const double density = static_cast<double>(lf.info->density());
    const double hsp = static_cast<double>(lf.info->specificHeatCapacity());
    REQUIRE(hsp > 0.0);
    tank->fields.temperature = 300.0;
    tank->fields.thermalMass = 100.0;
    other->fields.temperature = 500.0;
    other->fields.thermalMass = 80.0;
    lf.amount = 0.0;
    CHECK(tank->TransferResource(lf, 40.0, other.get()) == -40.0);  // push 40 (partial fill): num4 = 40 * density * hsp
    const double c1 = 40.0 * density * hsp;
    CHECK(tank->fields.temperature == (300.0 * 100.0 + c1 * 500.0) / (100.0 + c1));
    CHECK(other->fields.temperature == 500.0);  // the sender is not changed when pushing
    // pulling 25 from `tank` into `other` (partial): the OTHER part is mixed: T_o = (T_o * tm_o + c * T) / (tm_o + c)
    const double tankT = tank->fields.temperature;
    CHECK(tank->TransferResource(lf, -25.0, other.get()) == 25.0);
    const double c2 = 25.0 * density * hsp;
    CHECK(other->fields.temperature == (500.0 * 80.0 + c2 * tankT) / (80.0 + c2));
    CHECK(tank->fields.temperature == tankT);
    // a part transferring to itself, or a simulated transfer, mixes nothing
    const double t3 = other->fields.temperature;
    tank->TransferResource(lf, -5.0, tank.get());
    tank->TransferResource(lf, -5.0, other.get(), true);
    CHECK(other->fields.temperature == t3);
}

// ================================================== PartSet =================================================================

TEST_CASE("WP-15 PartSet: crossfeed set of a tank stack, totals, priority lists") {
    if (!GameAvailable()) return;
    Game15 g;
    Stack3 s(g);
    // A - B - C are attached by stack nodes and every FL-T400 has fuelCrossFeed = true: one strongly connected component, so all three
    // parts share one crossfeed PartSet holding the three parts (PartSet.BuildPartSets)
    REQUIRE(s.A->crossfeedPartSet);
    CHECK(s.A->crossfeedPartSet == s.B->crossfeedPartSet);
    CHECK(s.B->crossfeedPartSet == s.C->crossfeedPartSet);
    PartHashSetPtr parts = s.A->crossfeedPartSet->GetParts();
    CHECK(parts->Count() == 3);
    CHECK(parts->Contains(s.A.get()));
    CHECK(parts->Contains(s.C.get()));
    CHECK(s.A->crossfeedPartSet->ContainsPart(s.B.get()));
    CHECK(s.vessel.sets.crossfeedSets.size() == 1);
    CHECK_FALSE(s.A->crossfeedPartSet->IsSimulationSet());
    REQUIRE(s.A->simulationCrossfeedPartSet);
    CHECK(s.A->simulationCrossfeedPartSet->IsSimulationSet());
    CHECK(s.A->simulationCrossfeedPartSet != s.A->crossfeedPartSet);  // Both: the simulation set is a copy (new PartSet(value))

    // GetConnectedResourceTotals (PartSet.cs:123): pulling amount = sum of amounts, pushing amount = sum of (max - amount)
    double amount, maxAmount;
    s.A->GetConnectedResourceTotals(s.lf, ResourceFlowMode::STACK_PRIORITY_SEARCH, amount, maxAmount, true);
    CHECK(amount == 600.0);
    CHECK(maxAmount == 1200.0);
    s.A->GetConnectedResourceTotals(s.lf, ResourceFlowMode::STACK_PRIORITY_SEARCH, amount, maxAmount, false);
    CHECK(amount == 600.0);  // 300 + 200 + 100 free
    CHECK(maxAmount == 1200.0);
    // the vessel-wide modes read the vessel's resourcePartSet
    s.A->GetConnectedResourceTotals(s.lf, ResourceFlowMode::ALL_VESSEL, amount, maxAmount, true);
    CHECK(amount == 600.0);
    // threshold overload (PartSet.cs:159): pulling uses num3 = maxAmount * (1 - maxAmount * threshold) (sic: the C# multiplies twice), so
    // any threshold with max * threshold >= 1 yields nothing. threshold 0: num3 = 400, amount = 100 + 200 + 300.
    s.A->crossfeedPartSet->GetConnectedResourceTotals(s.lf, amount, maxAmount, 0.0, true);
    CHECK(amount == 600.0);
    CHECK(maxAmount == 1200.0);
    s.A->crossfeedPartSet->GetConnectedResourceTotals(s.lf, amount, maxAmount, 0.25, true);  // num2 = 100, num3 = 400 * (1 - 100) < 0
    CHECK(amount == 0.0);
    CHECK(maxAmount == 0.0);
    // pushing: num5 = max * threshold = 100; num6 = num5 - amount = 0, -100, -200 -> none above 1e-9; maxAmount += num5 for each
    s.A->crossfeedPartSet->GetConnectedResourceTotals(s.lf, amount, maxAmount, 0.25, false);
    CHECK(amount == 0.0);
    CHECK(maxAmount == 300.0);

    // the pull list of LiquidFuel: three lists in ascending priority (0, 10, 20), each with one resource (PartSet.BuildResList)
    PartSet::ResourcePrioritySet* list = s.A->crossfeedPartSet->GetResourceList(s.lf, true, false);
    REQUIRE(list->lists.size() == 3);
    CHECK(list->lists[0][0] == s.A->GetResource("LiquidFuel"));
    CHECK(list->lists[1][0] == s.B->GetResource("LiquidFuel"));
    CHECK(list->lists[2][0] == s.C->GetResource("LiquidFuel"));
    CHECK(list->set.size() == 3);
    // Oxidizer too (same flow mode), but a resource nobody has gives an empty set
    CHECK(s.A->crossfeedPartSet->GetResourceList(Id("Oxidizer"), true, false)->lists.size() == 3);
    CHECK(s.A->crossfeedPartSet->GetResourceList(Id("Ore"), true, false)->lists.empty());
}

TEST_CASE("WP-15 PartSet.RequestResource: highest priority first, proportional inside a list, sequential across lists") {
    if (!GameAvailable()) return;
    Game15 g;
    Stack3 s(g);  // A = 100 (priority 0), B = 200 (10), C = 300 (20); STACK_PRIORITY_SEARCH = LiquidFuel's own flow mode
    const double eps = 1e-12;
    // PartSet.cs:265 ProcessRequest(usePri = true), pulling. Lists are visited from the LAST (highest priority). For one list:
    //   available = sum(amount); if available > demand: every resource gives demand * amount / available (proportional) and the
    //   request ends; otherwise TransferResource(r, -demand) takes everything the resource has and the next list gets the rest.
    // 1) demand 50: C (300) covers it: only C changes, by 50 (-50/300 * 300 up to rounding)
    double got = s.A->RequestResource(s.lf, 50.0);
    CHECK(got == doctest::Approx(50.0).epsilon(eps));
    CHECK(s.Amount(*s.C) == doctest::Approx(250.0).epsilon(eps));
    CHECK(s.Amount(*s.B) == 200.0);
    CHECK(s.Amount(*s.A) == 100.0);
    // 2) demand 400: C has 250 < 400: sequential, C -> 0, remaining 150; B (200) covers it proportionally: B -> 50; returned 400
    got = s.A->RequestResource(s.lf, 400.0);
    CHECK(got == doctest::Approx(400.0).epsilon(eps));
    CHECK(s.Amount(*s.C) == 0.0);
    CHECK(s.Amount(*s.B) == doctest::Approx(50.0).epsilon(eps));
    CHECK(s.Amount(*s.A) == 100.0);
    // 3) demand 1000 exceeds everything: C is empty (available == 0: skipped), B and A are drained, the return is what was taken
    got = s.A->RequestResource(s.lf, 1000.0);
    CHECK(got == doctest::Approx(150.0).epsilon(eps));
    CHECK(s.Amount(*s.B) == 0.0);
    CHECK(s.Amount(*s.A) == 0.0);
    // 4) nothing left: 0
    CHECK(s.A->RequestResource(s.lf, 10.0) == 0.0);
    // demand 0 is 0 without touching anything
    CHECK(s.A->RequestResource(s.lf, 0.0) == 0.0);

    // pushing (negative demand) fills from the highest priority list down: lists use available = sum(amount - max) (negative).
    // All empty (0/400): C has room -400 which is < -50: proportional, C gets 50.
    got = s.A->RequestResource(s.lf, -50.0);
    CHECK(got == doctest::Approx(-50.0).epsilon(eps));
    CHECK(s.Amount(*s.C) == doctest::Approx(50.0).epsilon(eps));
    CHECK(s.Amount(*s.B) == 0.0);
    // room -350 in C < -2000? no: sequential fill of C (+350 -> 400 full), remaining -1650; B: room -400 not < -1650: sequential: B full,
    // remaining -1250; A full, remaining -850. Returned = what fitted = -1150.
    got = s.A->RequestResource(s.lf, -2000.0);
    CHECK(got == doctest::Approx(-1150.0).epsilon(eps));
    CHECK(s.Amount(*s.A) == 400.0);
    CHECK(s.Amount(*s.B) == 400.0);
    CHECK(s.Amount(*s.C) == 400.0);
}

TEST_CASE("WP-15 Part.RequestResource by ResourceFlowMode: NO_FLOW / ALL_VESSEL(_BALANCE) / STAGE_PRIORITY_FLOW(_BALANCE) / STACK_PRIORITY_SEARCH") {
    if (!GameAvailable()) return;
    Game15 g;
    Stack3 s(g);
    const double eps = 1e-12;
    // Part.cs:6480 requestResource: demand 0 -> 0. NO_FLOW and NULL (the switch default) use the part's own list only.
    CHECK(s.A->RequestResource(s.lf, 0.0, ResourceFlowMode::ALL_VESSEL) == 0.0);
    CHECK(s.A->RequestResource(s.lf, 30.0, ResourceFlowMode::NO_FLOW) == 30.0);
    CHECK(s.Amount(*s.A) == 70.0);
    CHECK(s.Amount(*s.B) == 200.0);
    CHECK(s.A->RequestResource(s.lf, 500.0, ResourceFlowMode::NULL_) == 70.0);  // only A's remaining 70
    CHECK(s.Amount(*s.A) == 0.0);
    CHECK(s.A->RequestResource(s.lf, 5.0, ResourceFlowMode::NO_FLOW) == 0.0);  // empty
    // a resource whose flowMode (None) is not flowing is skipped: GetFlowing(id, pulling) is null
    s.Set(100, 200, 300);
    s.A->GetResource("LiquidFuel")->flowMode = PartResourceFlowMode::In;
    CHECK(s.A->RequestResource(s.lf, 10.0, ResourceFlowMode::NO_FLOW) == 0.0);
    s.A->GetResource("LiquidFuel")->flowMode = PartResourceFlowMode::Both;

    // ALL_VESSEL and ALL_VESSEL_BALANCE: vessel.RequestResource(usePriority = false): PartSet.ProcessRequest(usePri = false) --
    // the sum over ALL lists decides: available 600 > 60 -> every resource gives 60 * amount / 600 (num11 = -0.1): 10, 20, 30.
    for (ResourceFlowMode mode : {ResourceFlowMode::ALL_VESSEL, ResourceFlowMode::ALL_VESSEL_BALANCE}) {
        s.Set(100, 200, 300);
        CHECK(s.A->RequestResource(s.lf, 60.0, mode) == doctest::Approx(60.0).epsilon(eps));
        CHECK(s.Amount(*s.A) == doctest::Approx(90.0).epsilon(eps));
        CHECK(s.Amount(*s.B) == doctest::Approx(180.0).epsilon(eps));
        CHECK(s.Amount(*s.C) == doctest::Approx(270.0).epsilon(eps));
    }
    // demand above the total: sequential over all lists from the last, each gives everything, return = total taken
    s.Set(100, 200, 300);
    CHECK(s.A->RequestResource(s.lf, 700.0, ResourceFlowMode::ALL_VESSEL) == doctest::Approx(600.0).epsilon(eps));
    CHECK(s.Amount(*s.A) + s.Amount(*s.B) + s.Amount(*s.C) == 0.0);

    // STAGE_PRIORITY_FLOW and _BALANCE: vessel set with usePriority = true: only the highest-priority list while it covers the demand
    for (ResourceFlowMode mode : {ResourceFlowMode::STAGE_PRIORITY_FLOW, ResourceFlowMode::STAGE_PRIORITY_FLOW_BALANCE}) {
        s.Set(100, 200, 300);
        CHECK(s.B->RequestResource(s.lf, 60.0, mode) == doctest::Approx(60.0).epsilon(eps));
        CHECK(s.Amount(*s.C) == doctest::Approx(240.0).epsilon(eps));  // C has priority 20
        CHECK(s.Amount(*s.B) == 200.0);
        CHECK(s.Amount(*s.A) == 100.0);
    }

    // STACK_PRIORITY_SEARCH / STAGE_STACK_FLOW / STAGE_STACK_FLOW_BALANCE: the part's crossfeed set with priorities
    for (ResourceFlowMode mode : {ResourceFlowMode::STACK_PRIORITY_SEARCH, ResourceFlowMode::STAGE_STACK_FLOW, ResourceFlowMode::STAGE_STACK_FLOW_BALANCE}) {
        s.Set(100, 200, 300);
        CHECK(s.A->RequestResource(s.lf, 60.0, mode) == doctest::Approx(60.0).epsilon(eps));
        CHECK(s.Amount(*s.C) == doctest::Approx(240.0).epsilon(eps));
    }

    // the string overloads and the default-flow-mode overloads: LiquidFuel's definition flowMode is STACK_PRIORITY_SEARCH
    s.Set(100, 200, 300);
    CHECK(s.A->RequestResource("LiquidFuel", 10.0) == doctest::Approx(10.0).epsilon(eps));
    CHECK(s.Amount(*s.C) == doctest::Approx(290.0).epsilon(eps));
    CHECK(s.A->RequestResource(s.lf, 10.0, false) == doctest::Approx(10.0).epsilon(eps));
    CHECK(s.A->RequestResource("LiquidFuel", 10.0, ResourceFlowMode::ALL_VESSEL, false) == doctest::Approx(10.0).epsilon(eps));
    // an unknown resource id has default flow mode NULL: 0, with an error log (PartResourceLibrary.GetDefaultFlowMode)
    LogCapture log;
    CHECK(s.A->RequestResource(Id("Unobtainium"), 10.0) == 0.0);
    CHECK(log.Has(log::Level::Error, "Resource System Error"));

    // a part with neither vessel nor ship: ALL_VESSEL / STAGE_PRIORITY_FLOW give 0; the crossfeed modes need the set (NullReference)
    auto lone = g.New("fuelTank");
    CHECK(lone->RequestResource(s.lf, 10.0, ResourceFlowMode::ALL_VESSEL) == 0.0);
    CHECK(lone->RequestResource(s.lf, 10.0, ResourceFlowMode::STAGE_PRIORITY_FLOW) == 0.0);
    CHECK_THROWS_AS(lone->RequestResource(s.lf, 10.0, ResourceFlowMode::STACK_PRIORITY_SEARCH), NullReferenceException);
    CHECK(lone->RequestResource(s.lf, 10.0, ResourceFlowMode::NO_FLOW) == 10.0);

    // CheatOptions.InfiniteElectricity: a positive ElectricCharge demand is returned in full without touching anything
    ResourceFlowEnvironment::InfiniteElectricity = true;
    CHECK(lone->RequestResource(Id("ElectricCharge"), 5.0, ResourceFlowMode::ALL_VESSEL) == 5.0);
    CHECK(lone->RequestResource(Id("ElectricCharge"), -5.0, ResourceFlowMode::ALL_VESSEL) == 0.0);  // pushes still go to the (empty) set
    ResourceFlowEnvironment::InfiniteElectricity = false;
}

TEST_CASE("WP-15 PartSet: flowState / flowMode / priority changes keep the lists in step; simulation twins; CanCrossfeed") {
    if (!GameAvailable()) return;
    Game15 g;
    Stack3 s(g);
    const double eps = 1e-12;
    PartSet& set = *s.A->crossfeedPartSet;
    auto lists = [&](bool pull) { return set.GetResourceList(s.lf, pull, false); };
    REQUIRE(lists(true)->lists.size() == 3);

    // OnFlowStateChange (PartSet.cs:906): flowState true -> false removes the resource from the pull and push lists (an emptied list
    // disappears); false -> true re-adds it into the list of its priority, creating the list at the right position
    s.C->GetResource("LiquidFuel")->SetFlowState(false);
    REQUIRE(lists(true)->lists.size() == 2);
    REQUIRE(lists(false)->lists.size() == 2);
    CHECK(lists(true)->lists[1][0] == s.B->GetResource("LiquidFuel"));
    CHECK(lists(true)->set.count(s.C->GetResource("LiquidFuel")) == 0);
    // draws now skip C: B is the highest priority
    CHECK(s.A->RequestResource(s.lf, 60.0) == doctest::Approx(60.0).epsilon(eps));
    CHECK(s.Amount(*s.B) == doctest::Approx(140.0).epsilon(eps));
    CHECK(s.Amount(*s.C) == 300.0);
    s.C->GetResource("LiquidFuel")->SetFlowState(true);
    REQUIRE(lists(true)->lists.size() == 3);
    CHECK(lists(true)->lists[2][0] == s.C->GetResource("LiquidFuel"));
    CHECK(lists(false)->lists.size() == 3);
    // adding twice is idempotent (value.set.Contains)
    s.C->GetResource("LiquidFuel")->SetFlowState(false);
    s.C->GetResource("LiquidFuel")->SetFlowState(true);
    CHECK(lists(true)->lists.size() == 3);

    // OnFlowModeChange (:930): Both -> In removes from the pull list only; In -> Both adds it back; Both -> None removes both
    PartResource& cl = *s.C->GetResource("LiquidFuel");
    cl.SetFlowMode(PartResourceFlowMode::In);
    CHECK(lists(true)->lists.size() == 2);
    CHECK(lists(false)->lists.size() == 3);
    cl.SetFlowMode(PartResourceFlowMode::Both);
    CHECK(lists(true)->lists.size() == 3);
    cl.SetFlowMode(PartResourceFlowMode::None);
    CHECK(lists(true)->lists.size() == 2);
    CHECK(lists(false)->lists.size() == 2);
    cl.SetFlowMode(PartResourceFlowMode::Out);  // None -> Out: pull only
    CHECK(lists(true)->lists.size() == 3);
    CHECK(lists(false)->lists.size() == 2);
    cl.SetFlowMode(PartResourceFlowMode::Both);  // Out -> Both: push added
    CHECK(lists(false)->lists.size() == 3);
    cl.SetFlowMode(PartResourceFlowMode::Out);
    cl.SetFlowMode(PartResourceFlowMode::In);  // Out -> In: push added, pull removed
    CHECK(lists(true)->lists.size() == 2);
    CHECK(lists(false)->lists.size() == 3);
    cl.SetFlowMode(PartResourceFlowMode::Both);

    // Part.ChangeResourcePriority (:7763) fires onPartPriorityChanged; the vessel rebuilds its sets (Vessel.cs:1577): A (inverseStage 0)
    // gets priority 0 + 30 = 30 and now leads
    s.Set(100, 200, 300);
    s.A->ChangeResourcePriority(30);
    CHECK(s.A->GetResourcePriority() == 30);
    PartSet& rebuilt = *s.A->crossfeedPartSet;
    PartSet::ResourcePrioritySet* l = rebuilt.GetResourceList(s.lf, true, false);
    REQUIRE(l->lists.size() == 3);
    CHECK(l->lists[2][0] == s.A->GetResource("LiquidFuel"));
    CHECK(l->lists[0][0] == s.B->GetResource("LiquidFuel"));
    CHECK(s.B->RequestResource(s.lf, 40.0) == doctest::Approx(40.0).epsilon(eps));  // from A (the highest priority list)
    CHECK(s.Amount(*s.A) == doctest::Approx(60.0).epsilon(eps));
    CHECK(s.Amount(*s.C) == 300.0);
    s.A->ResetPri();
    CHECK(s.A->GetResourcePriority() == 0);

    // simulate = true works on the simulation twins only (SimulationResources) and leaves the real amounts alone
    s.Set(100, 200, 300);
    CHECK(s.A->RequestResource(s.lf, 50.0, ResourceFlowMode::STACK_PRIORITY_SEARCH, true) == doctest::Approx(50.0).epsilon(eps));
    CHECK(s.Amount(*s.C) == 300.0);
    CHECK(s.Sim(*s.C) == doctest::Approx(250.0).epsilon(eps));
    CHECK(s.A->RequestResource(s.lf, 20.0, ResourceFlowMode::ALL_VESSEL, true) == doctest::Approx(20.0).epsilon(eps));
    // ResetSimulation copies the real amounts back
    s.A->ResetSimulation();
    s.C->ResetSimulation();
    CHECK(s.Sim(*s.C) == 300.0);

    // CanCrossfeed (Part.cs:6828): own part for NO_FLOW, same vessel for the vessel modes, the crossfeed set for the stack modes
    auto other = g.New("fuelTank");
    other->fields.persistentId = 999;
    CHECK(s.A->CanCrossfeed(s.C.get(), s.lf, ResourceFlowMode::NO_FLOW) == false);
    CHECK(s.A->CanCrossfeed(s.A.get(), s.lf, ResourceFlowMode::NO_FLOW) == true);
    CHECK(s.A->CanCrossfeed(s.C.get(), s.lf, ResourceFlowMode::ALL_VESSEL) == true);
    CHECK(s.A->CanCrossfeed(other.get(), s.lf, ResourceFlowMode::STAGE_PRIORITY_FLOW) == false);
    CHECK(s.A->CanCrossfeed(s.C.get(), s.lf) == true);  // flow = definition's STACK_PRIORITY_SEARCH -> set contains C
    CHECK(s.A->CanCrossfeed(other.get(), "LiquidFuel") == false);
}

TEST_CASE("WP-15 PartSet: constructors, rebuild functions, RemovePart, SetsEqual, AddPartToSet") {
    if (!GameAvailable()) return;
    Game15 g;
    Stack3 s(g);
    PartSet& set = *s.A->crossfeedPartSet;
    const int idBefore = set.setId;
    // PartSet(PartSet): a new id, vesselWide copied; the part set is copied (new HashSet)
    PartSet copy(set);
    CHECK(copy.setId != set.setId);
    CHECK(copy.GetParts() != set.GetParts());
    CHECK(copy.GetParts()->Count() == 3);
    CHECK(copy.SetsEqual(*set.GetParts()));
    // RemovePart rebuilds from the other persistentIds (parts created from prefabs have persistentId 0 unless assigned)
    s.A->fields.persistentId = 11;
    s.B->fields.persistentId = 12;
    s.C->fields.persistentId = 13;
    copy.RemovePart(s.B.get());
    CHECK(copy.GetParts()->Count() == 2);
    CHECK_FALSE(copy.GetParts()->Contains(s.B.get()));
    CHECK_FALSE(copy.SetsEqual(*set.GetParts()));
    CHECK(copy.ContainsPart(s.A.get()));
    CHECK_FALSE(copy.ContainsPart(s.B.get()));
    // RebuildInPlace clears the cached lists and gives a new id
    PartSet::ResourcePrioritySet* before = set.GetResourceList(s.lf, true, false);
    set.RebuildInPlace();
    CHECK(set.setId != idBefore);
    (void)before;
    CHECK(set.GetResourceList(s.lf, true, false)->lists.size() == 3);  // rebuilt on demand
    // PartSet(HashSet) set is never vessel wide: SetsEqual(vesselWide) is false, ContainsPart uses the persistentId test
    PartSet vesselSet(s.vessel.handle());
    CHECK(vesselSet.IsVesselWide());
    CHECK_FALSE(vesselSet.SetsEqual(*set.GetParts()));
    CHECK(vesselSet.ContainsPart(s.A.get()));  // p.vessel == vessel
    auto outsider = g.New("fuelTank");
    outsider->fields.persistentId = 999;
    CHECK_FALSE(vesselSet.ContainsPart(outsider.get()));
    // AddPartToSet (obsolete walk): from C it reaches B and A through the attach nodes
    PartHashSet walked;
    PartSet::AddPartToSet(walked, s.C.get(), s.vessel.handle());
    CHECK(walked.Count() == 3);
    CHECK(walked.Contains(s.A.get()));
    // fuelCrossFeed = false on B stops the walk beyond B (B is added but not expanded)
    s.B->fields.fuelCrossFeed = false;
    PartHashSet walked2;
    PartSet::AddPartToSet(walked2, s.C.get(), s.vessel.handle());
    CHECK(walked2.Count() == 2);
    CHECK_FALSE(walked2.Contains(s.A.get()));
    s.B->fields.fuelCrossFeed = true;
}

TEST_CASE("WP-15 flow graph: asparagus staging with fuel lines, decoupler crossfeed, NoCrossFeedNodeKey") {
    if (!GameAvailable()) return;
    Game15 g;
    // Core tank T0 with two outer tanks T1, T2 radially attached (no node between T0 and the outer tanks: the tank-to-tank connection
    // is a fuel line from T1 to T0 only). StackFlowGraph.UpdatePartVertex: T0.fuelLookupTargets holds the fuel line L1 (a CompoundPart
    // adds itself to its target, CModuleFuelLine.OnTargetSet); L1.parent = T1, so T0 depends on T1 in the request graph and T1 on T0 in
    // the delivery graph. T2 has no line: it stays alone.
    auto T0 = g.New("fuelTank");
    auto T1 = g.New("fuelTank");
    auto T2 = g.New("fuelTank");
    auto L1 = g.New("fuelLine");
    REQUIRE(L1->className == "CompoundPart");
    L1->setParent(T1.get());
    L1->srfAttachNode->attachedPart = T1.get();
    L1->fields.fuelCrossFeed = false;  // so the line's own surface link adds no edges
    for (PartModule* m : L1->Modules().GetModules([](PartModule& pm) { return dynamic_cast<CompoundPartModule*>(&pm) != nullptr; })) {
        auto* cm = dynamic_cast<CompoundPartModule*>(m);
        cm->compoundTarget = [t = T0.get()](const Part&) { return t; };
        cm->OnTargetSet(T0.get());
    }
    CHECK(T0->fuelLookupTargets.size() == 1);
    CHECK(T0->fuelLookupTargets[0] == L1.get());
    T0->fields.inverseStage = 0;
    T1->fields.inverseStage = 3;
    T2->fields.inverseStage = 3;

    std::vector<Part*> parts{T0.get(), T1.get(), T2.get(), L1.get()};
    SCCFlowGraph graph(parts);
    PartHashSetPtr reqT0 = graph.GetAllRequests(T0.get());
    CHECK(reqT0->Contains(T0.get()));
    CHECK(reqT0->Contains(T1.get()));
    CHECK_FALSE(reqT0->Contains(T2.get()));
    PartHashSetPtr reqT1 = graph.GetAllRequests(T1.get());
    CHECK(reqT1->Count() == 1);  // T1 draws from nobody else
    CHECK(graph.GetAllRequests(T2.get())->Count() == 1);
    // delivery: T1 can deliver to T0 (value.Dependencies.Add(vertex) in the delivery graph)
    CHECK(graph.GetAllDeliveries(T1.get())->Contains(T0.get()));
    CHECK_FALSE(graph.GetAllDeliveries(T0.get())->Contains(T1.get()));

    // and the flow follows: T0's engine-style request for LiquidFuel (STACK_PRIORITY_SEARCH) draws T1 first (priority 30 > T0's 0)
    TestVessel vessel;
    vessel.Adopt(parts);
    const int lf = Id("LiquidFuel");
    for (Part* p : {T0.get(), T1.get(), T2.get()}) Stack3::SetOne(*p, 100.0);
    REQUIRE(T0->crossfeedPartSet);
    CHECK(T0->RequestResource(lf, 40.0) == doctest::Approx(40.0).epsilon(1e-12));
    CHECK(T1->GetResource("LiquidFuel")->amount == doctest::Approx(60.0).epsilon(1e-12));
    CHECK(T0->GetResource("LiquidFuel")->amount == 100.0);
    CHECK(T2->GetResource("LiquidFuel")->amount == 100.0);
    // T1 is exhausted first, then T0 itself (priority 0): demand 90 -> T1's remaining 60 (sequential), then T0 gives 30 (proportional)
    CHECK(T0->RequestResource(lf, 90.0) == doctest::Approx(90.0).epsilon(1e-12));
    CHECK(T1->GetResource("LiquidFuel")->amount == 0.0);
    CHECK(T0->GetResource("LiquidFuel")->amount == doctest::Approx(70.0).epsilon(1e-12));
    CHECK(T2->GetResource("LiquidFuel")->amount == 100.0);
    // T2 never contributes to T0, T0's own request cannot reach it; T2's own request only sees T2
    CHECK(T2->RequestResource(lf, 500.0) == 100.0);
    CHECK(T0->GetResource("LiquidFuel")->amount == doctest::Approx(70.0).epsilon(1e-12));

    // closing the fuel line (CModuleFuelLine.CloseFuelLine) removes the lookup target and fires onPartFuelLookupStateChange: the vessel
    // is marked dirty (Vessel.cs:3943) and rebuilt on demand; then T0 is alone
    CompoundPartModule* fl = dynamic_cast<CompoundPartModule*>(L1->Modules().GetModule([](PartModule& pm) { return dynamic_cast<CModuleFuelLine*>(&pm) != nullptr; }));
    REQUIRE(fl != nullptr);
    dynamic_cast<CModuleFuelLine*>(fl)->OnTargetLost();
    CHECK(T0->fuelLookupTargets.empty());
    CHECK(vessel.sets.resourcesDirty);
    vessel.UpdateResourceSetsIfDirty();
    CHECK_FALSE(vessel.sets.resourcesDirty);
    CHECK(T0->crossfeedPartSet->GetParts()->Count() == 1);

    // NoCrossFeedNodeKey (Part.cs:629): an attach node whose id contains the key is not followed. Two tanks stacked: T3.bottom <-> T4.top.
    auto T3 = g.New("fuelTank");
    auto T4 = g.New("fuelTank");
    Attach(*T3, "bottom", *T4, "top");
    {
        SCCFlowGraph plain({T3.get(), T4.get()});
        CHECK(plain.GetAllRequests(T3.get())->Contains(T4.get()));
        CHECK(plain.GetAllRequests(T4.get())->Contains(T3.get()));
    }
    T3->fields.NoCrossFeedNodeKey = "bottom";  // T3 does not follow its "bottom" node; T4 still follows its "top" to T3
    {
        SCCFlowGraph keyed({T3.get(), T4.get()});
        CHECK_FALSE(keyed.GetAllRequests(T3.get())->Contains(T4.get()));
        CHECK(keyed.GetAllRequests(T4.get())->Contains(T3.get()));
        CHECK(keyed.GetAllRequests(T4.get())->Count() == 2);
    }
    // ResourceXFeed = false on a node: that side does not follow it; the opposing check (FindOpposingNode): the edge is also dropped
    // when the opposing node has ResourceXFeed false and AllowOneWayXFeed false
    T3->fields.NoCrossFeedNodeKey = "";
    T4->FindAttachNode("top")->ResourceXFeed = false;
    T4->FindAttachNode("top")->AllowOneWayXFeed = false;
    {
        SCCFlowGraph oneWay({T3.get(), T4.get()});
        // T3 -> T4: T3's node is ResourceXFeed but the opposing node refuses and does not allow one-way: no edge
        CHECK_FALSE(oneWay.GetAllRequests(T3.get())->Contains(T4.get()));
        // T4 -> T3: T4's own node has ResourceXFeed false: no edge
        CHECK_FALSE(oneWay.GetAllRequests(T4.get())->Contains(T3.get()));
    }
    T4->FindAttachNode("top")->AllowOneWayXFeed = true;  // one-way allowed: T3 -> T4 exists again, T4 -> T3 still refused
    {
        SCCFlowGraph oneWay({T3.get(), T4.get()});
        CHECK(oneWay.GetAllRequests(T3.get())->Contains(T4.get()));
        CHECK_FALSE(oneWay.GetAllRequests(T4.get())->Contains(T3.get()));
    }
    // fuelCrossFeed = false on a part: it adds no node edges (UpdatePartVertex returns after the fuel-lookup loop)
    T4->FindAttachNode("top")->ResourceXFeed = true;
    T4->fields.fuelCrossFeed = false;
    {
        SCCFlowGraph blocked({T3.get(), T4.get()});
        CHECK(blocked.GetAllRequests(T4.get())->Count() == 1);
        CHECK(blocked.GetAllRequests(T3.get())->Contains(T4.get()));  // T3 can still reach T4 (T4 is terminal)
    }
}

// ================================================== stock crafts ============================================================

namespace {

std::set<Part*> AsStdSet(const PartHashSet& s) {
    std::set<Part*> r;
    for (Part* p : s) r.insert(p);
    return r;
}

// breadth-first closure over the Dependencies of a vertex
template <class FlowGraphT>
std::set<Part*> Closure(FlowGraphT& graph, Part* start) {
    std::set<Part*> seen{start};
    std::vector<Part*> queue{start};
    while (!queue.empty()) {
        Part* p = queue.back();
        queue.pop_back();
        for (auto* dep : graph.lookup.At(p)->Dependencies)
            if (seen.insert(dep->Value).second) queue.push_back(dep->Value);
    }
    return seen;
}

struct DrawSnapshot {
    std::vector<std::vector<double>> amounts;
    double Sum() const {
        double s = 0;
        for (const auto& l : amounts)
            for (double a : l) s += a;
        return s;
    }
};
DrawSnapshot Snap(PartSet::ResourcePrioritySet* l) {
    DrawSnapshot d;
    for (const auto& list : l->lists) {
        std::vector<double> v;
        for (PartResource* r : list) v.push_back(r->amount);
        d.amounts.push_back(v);
    }
    return d;
}

}  // namespace

TEST_CASE("WP-15 flow graph and resource sets on every stock-part craft of harness/inputs/Ships") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;  // module / resource warnings of mods
    size_t crafts = 0, parts = 0, sccTotal = 0, linesLinked = 0, toggles = 0, terminalDecouplers = 0, drawsChecked = 0, totalsChecked = 0;
    size_t cratfsWithLines = 0;
    for (const char* dir : {"VAB", "SPH"}) {
        const fs::path d = fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / dir;
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(d))
            if (e.path().extension() == ".craft") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const fs::path& f : files) {
            ConfigNode::NodePtr root = ConfigNode::Load(f.string());
            REQUIRE(root);
            ShipConstruct ship;
            std::string err;
            if (!ship.LoadShip(*root, 0, true, err)) continue;  // modded parts or an old format (LoadShip refuses them as in the game)
            CraftTree tree = BuildCraftTree(g.data, ship);
            if (!tree.ok) continue;
            CAPTURE(f.filename().string());
            crafts++;
            const int lines = LinkFuelLines(tree, ship);
            int expectedLines = 0;  // the craft file's fuelLine parts whose PARTDATA names a target (CompoundPart.tgt)
            for (const auto& pd : ship.parts)
                if (pd.partInfoName == "fuelLine" && pd.partData && pd.partData->HasValue("tgt")) expectedLines++;
            CHECK(lines == expectedLines);
            linesLinked += static_cast<size_t>(lines);
            if (lines > 0) cratfsWithLines++;
            std::vector<Part*> raw = tree.Raw();
            parts += raw.size();
            TestVessel vessel;
            vessel.Adopt(raw);
            SCCFlowGraph graph(raw);

            // (1) the strongly connected components of the request graph partition the parts
            std::set<Part*> covered;
            size_t sccParts = 0;
            for (const PartHashSetPtr& scc : graph.GetConnectedComponentsInRequests()) {
                sccTotal++;
                for (Part* p : *scc) {
                    CHECK(covered.insert(p).second);
                    sccParts++;
                }
            }
            CHECK(sccParts == raw.size());
            CHECK(covered.size() == raw.size());

            // (2) GetAllRequests / GetAllDeliveries = the reachable set (closure) of the part's vertex, in both graphs
            for (Part* p : raw) {
                CHECK(AsStdSet(*graph.GetAllRequests(p)) == Closure(*graph.stackFlowGraph->request, p));
                CHECK(AsStdSet(*graph.GetAllDeliveries(p)) == Closure(*graph.stackFlowGraph->delivery, p));
                CHECK(graph.GetAllRequests(p)->Contains(p));
            }

            // (3) the delivery graph is the transpose of the request graph: every edge u -> v of one is v -> u of the other (surface links
            // are added in both directions to both graphs)
            std::multiset<std::pair<Part*, Part*>> eReq, eDelTransposed;
            for (auto* v : graph.stackFlowGraph->request->graph)
                for (auto* dep : v->Dependencies) eReq.insert({v->Value, dep->Value});
            for (auto* v : graph.stackFlowGraph->delivery->graph)
                for (auto* dep : v->Dependencies) eDelTransposed.insert({dep->Value, v->Value});
            CHECK(eReq == eDelTransposed);

            // (4) the vessel's crossfeed PartSets are the supersets of the graph; one PartSet per component of the condensation
            std::set<PartSet*> distinct;
            for (Part* p : raw) {
                REQUIRE(p->crossfeedPartSet);
                CHECK(AsStdSet(*p->crossfeedPartSet->GetParts()) == AsStdSet(*graph.GetAllRequests(p)));
                distinct.insert(p->crossfeedPartSet.get());
            }
            CHECK(distinct.size() == vessel.sets.crossfeedSets.size());
            CHECK(distinct.size() == graph.GetConnectedComponentsInRequests().size());
            CHECK(vessel.sets.simulationCrossfeedSets.size() == vessel.sets.crossfeedSets.size());

            // (5) ModuleToggleCrossfeed parts (stock decouplers / separators): fuelCrossFeed follows crossfeedStatus (OnLoad -> UpdateCrossfeed),
            // and a part that does not cross-feed and has no fuel line only reaches itself
            for (Part* p : raw) {
                PartModule* m = p->Modules().GetModule([](PartModule& pm) { return dynamic_cast<ModuleToggleCrossfeed*>(&pm) != nullptr; });
                if (m != nullptr) {
                    toggles++;
                    CHECK(p->fields.fuelCrossFeed == m->Get<bool>("crossfeedStatus"));
                }
                if (!p->fields.fuelCrossFeed && p->fuelLookupTargets.empty()) {
                    CHECK(graph.GetAllRequests(p)->Count() == 1);
                    terminalDecouplers++;
                }
            }

            // (6) fuel lines: the target requests from the line's parent
            for (size_t i = 0; i < raw.size(); i++) {
                Part* line = raw[i];
                if (line->className != "CompoundPart" || !ship.parts[i].partData || !ship.parts[i].partData->HasValue("tgt")) continue;
                if (line->Modules().GetModule([](PartModule& pm) { return dynamic_cast<CModuleFuelLine*>(&pm) != nullptr; }) == nullptr) continue;
                Part* target = nullptr;
                for (Part* q : raw)
                    for (Part* t : q->fuelLookupTargets)
                        if (t == line) target = q;
                REQUIRE(target != nullptr);
                REQUIRE(line->parent != nullptr);
                CHECK(graph.GetAllRequests(target)->Contains(line->parent));
                CHECK(graph.GetAllDeliveries(line->parent)->Contains(target));
            }

            // (7) connected totals equal the manual sum over the part's crossfeed set / the vessel, for the resources of the craft
            for (const char* res : {"LiquidFuel", "Oxidizer", "ElectricCharge", "MonoPropellant", "SolidFuel"}) {
                const int id = Id(res);
                const PartResourceDefinition* def = PartResourceLibrary::Instance()->GetDefinition(id);
                const ResourceFlowMode mode = AsFlowMode(def->resourceFlowMode());
                size_t checkedHere = 0;
                for (Part* p : raw) {
                    if (p->GetResource(res) == nullptr) continue;
                    if (++checkedHere > 4) break;
                    double amount = -1, maxAmount = -1;
                    p->GetConnectedResourceTotals(id, mode, amount, maxAmount, true);
                    double sum = 0, sumMax = 0;
                    const bool stack = mode == ResourceFlowMode::STACK_PRIORITY_SEARCH || mode == ResourceFlowMode::STAGE_STACK_FLOW ||
                                       mode == ResourceFlowMode::STAGE_STACK_FLOW_BALANCE;
                    const bool vesselWide = mode == ResourceFlowMode::ALL_VESSEL || mode == ResourceFlowMode::STAGE_PRIORITY_FLOW ||
                                            mode == ResourceFlowMode::ALL_VESSEL_BALANCE || mode == ResourceFlowMode::STAGE_PRIORITY_FLOW_BALANCE;
                    if (stack || vesselWide) {
                        for (Part* q : raw) {
                            if (stack && !p->crossfeedPartSet->GetParts()->Contains(q)) continue;
                            PartResource* r = q->GetResource(res);
                            if (r != nullptr && r->Flowing(true)) {
                                sum += r->amount;
                                sumMax += r->maxAmount;
                            }
                        }
                    } else {  // NO_FLOW: the part's own resource
                        PartResource* r = p->GetResource(res);
                        if (r->Flowing(true)) {
                            sum = r->amount;
                            sumMax = r->maxAmount;
                        }
                    }
                    CHECK(amount == doctest::Approx(sum).epsilon(1e-9));
                    CHECK(maxAmount == doctest::Approx(sumMax).epsilon(1e-9));
                    totalsChecked++;
                }
            }

            // (8) a draw of LiquidFuel from the first tank that has it obeys the solver's invariants (see the derivation in the
            // PartSet.RequestResource test): conservation, lists visited from the highest priority, proportional split in the last one
            for (Part* p : raw) {
                PartResource* r = p->GetResource("LiquidFuel");
                if (r == nullptr || !r->Flowing(true) || r->amount <= 0.0) continue;
                const int id = Id("LiquidFuel");
                for (double factor : {0.01, 0.37, 1.5}) {
                    PartSet::ResourcePrioritySet* l = p->crossfeedPartSet->GetResourceList(id, true, false);
                    DrawSnapshot before = Snap(l);
                    const double total = before.Sum();
                    const double demand = std::max(total * factor, 1e-3);
                    const double got = p->RequestResource(id, demand);
                    DrawSnapshot after = Snap(l);
                    CHECK(before.Sum() - after.Sum() == doctest::Approx(got).epsilon(1e-9).scale(1.0));
                    CHECK(got <= std::min(demand, total) * (1.0 + 1e-9) + 1e-9);
                    // walk the lists from the highest priority: fully drained lists, then at most one partially drained list,
                    // then untouched lists
                    int phase = 0;  // 0 drained, 1 after the partial list
                    for (size_t k = l->lists.size(); k-- > 0;) {
                        bool allZero = true, untouched = true;
                        for (size_t j = 0; j < after.amounts[k].size(); j++) {
                            if (after.amounts[k][j] != 0.0) allZero = false;
                            if (after.amounts[k][j] != before.amounts[k][j]) untouched = false;
                        }
                        double bsum = 0;
                        for (double a : before.amounts[k]) bsum += a;
                        if (phase == 0) {
                            if (allZero || bsum == 0.0) continue;
                            phase = 1;  // the partial list
                            // proportional split: every resource gives the same fraction of its amount
                            if (!untouched && before.amounts[k].size() > 1 && got < total * (1.0 - 1e-9)) {
                                double frac = -1;
                                for (size_t j = 0; j < before.amounts[k].size(); j++) {
                                    if (before.amounts[k][j] <= 0) continue;
                                    const double fr = (before.amounts[k][j] - after.amounts[k][j]) / before.amounts[k][j];
                                    if (frac < 0) frac = fr;
                                    else CHECK(fr == doctest::Approx(frac).epsilon(1e-9).scale(1.0));
                                }
                            }
                        } else {
                            CHECK(untouched);
                        }
                    }
                    drawsChecked++;
                    // refill for the next factor
                    for (Part* q : raw)
                        if (PartResource* qr = q->GetResource("LiquidFuel")) {
                            qr->amount = qr->maxAmount;
                            q->ResetSimulationResources();
                        }
                }
                break;
            }
        }
    }
    MESSAGE("crafts " << crafts << ", parts " << parts << ", request components " << sccTotal << ", fuel lines linked " << linesLinked << " in "
                      << cratfsWithLines << " crafts, ModuleToggleCrossfeed parts " << toggles << ", terminal non-crossfeed parts " << terminalDecouplers
                      << ", totals checked " << totalsChecked << ", draws checked " << drawsChecked);
    CHECK(crafts == 33);
    CHECK(parts == 1967);
    CHECK(linesLinked > 0);
    CHECK(toggles > 0);
    CHECK(drawsChecked > 0);
}

// ================================================== drain definitions, resource definitions ================================

TEST_CASE("WP-15 PartResourceDrainDefinition and the stock RESOURCE_DEFINITION nodes") {
    // PartResourceDrainDefinition.cs:20 constructor (drainable, showFX, priority, ISP, fx name)
    PartResourceDrainDefinition d;  // defaults :9-:17
    CHECK(d.isDrainable());
    CHECK(d.showDrainFX());
    CHECK(d.drainFXPriority() == 5);
    CHECK(d.drainForceISP() == 50.0f);
    CHECK(d.drainFXDefinition() == "gasDraining");
    PartResourceDrainDefinition e(false, false, 9, 12.5f, "particlesDraining");
    CHECK_FALSE(e.isDrainable());
    CHECK_FALSE(e.showDrainFX());
    CHECK(e.drainFXPriority() == 9);
    CHECK(e.drainForceISP() == 12.5f);
    CHECK(e.drainFXDefinition() == "particlesDraining");
    // Save writes the five values in order; Load reads them back
    ConfigNode n("RESOURCE_DRAIN_DEFINITION");
    e.Save(n);
    CHECK(n.values().Count() == 5);
    PartResourceDrainDefinition back;
    back.Load(n);
    CHECK(back.drainFXPriority() == 9);
    CHECK(back.drainForceISP() == 12.5f);
    CHECK_FALSE(back.isDrainable());

    if (!GameAvailable()) return;
    Game15 g;
    // every stock definition: the loaded flowMode / drain values equal the cfg node text (read directly from the RESOURCE_DEFINITION nodes)
    size_t checked = 0;
    for (ConfigNode* node : g.data.database.GetConfigNodes("RESOURCE_DEFINITION")) {
        const std::string name = *node->GetValue("name");
        const PartResourceDefinition* def = PartResourceLibrary::Instance()->GetDefinition(name);
        REQUIRE(def != nullptr);
        CAPTURE(name);
        if (auto fm = node->GetValue("flowMode")) CHECK(kEnum_ResourceFlowMode.ToString(def->resourceFlowMode()) == *fm);
        if (auto tm = node->GetValue("transfer")) CHECK(kEnum_ResourceTransferMode.ToString(def->resourceTransferMode()) == *tm);
        if (ConfigNode* dn = node->GetNode("RESOURCE_DRAIN_DEFINITION")) {
            if (auto v = dn->GetValue("isDrainable")) CHECK(def->partResourceDrainDefinition().isDrainable() == (*v == "true" || *v == "True"));
            if (auto v = dn->GetValue("drainFXPriority")) CHECK(def->partResourceDrainDefinition().drainFXPriority() == std::stoi(*v));
            if (auto v = dn->GetValue("drainFXDefinition")) CHECK(def->partResourceDrainDefinition().drainFXDefinition() == *v);
        }
        checked++;
    }
    CHECK(checked >= 10);
    // the flow modes the rest of the tests depend on (ResourcesGeneric.cfg)
    auto mode = [](const char* n) { return AsFlowMode(PartResourceLibrary::Instance()->GetDefinition(n)->resourceFlowMode()); };
    CHECK(mode("LiquidFuel") == ResourceFlowMode::STACK_PRIORITY_SEARCH);
    CHECK(mode("Oxidizer") == ResourceFlowMode::STACK_PRIORITY_SEARCH);
    CHECK(mode("SolidFuel") == ResourceFlowMode::NO_FLOW);
    CHECK(mode("ElectricCharge") == ResourceFlowMode::STAGE_PRIORITY_FLOW);
    CHECK(mode("MonoPropellant") == ResourceFlowMode::STAGE_PRIORITY_FLOW);
    CHECK(mode("IntakeAir") == ResourceFlowMode::ALL_VESSEL);
    CHECK(mode("Ore") == ResourceFlowMode::ALL_VESSEL);
    // PartResourceList.HasFlowable: SolidFuel (NO_FLOW) alone does not count
    auto srb = g.New("solidBooster.v2");
    REQUIRE(srb->GetResource("SolidFuel") != nullptr);
    CHECK_FALSE(srb->Resources.HasFlowable());
}
