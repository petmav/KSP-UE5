// WP-03 event bus: Delegate (C# delegate model), EventVoid / EventData / EventDataModifier / Event*Max/Min /
// EventValueComparison / EventValueOperation / EventValueWrapper, BaseGameEvent.FindEvent, the GameEvents registry.
// Expected behaviour derived from the C# (EventData.cs, EventVoid.cs, ...); the call-order scenarios marked
// "netfx" are also checked against KSP's own IL in test_wp03_reference.cpp (tests/data/wp03_reference.tsv).
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/core/log.hpp"
#include "ksp/events/game_events.hpp"

using namespace ksp;
using namespace ksp::events;

namespace {

struct LogCapture {
    std::vector<std::pair<log::Level, std::string>> lines;
    LogCapture() {
        log::SetSink([this](log::Level l, std::string_view m) { lines.emplace_back(l, std::string(m)); });
    }
    ~LogCapture() { log::SetSink(nullptr); }
    size_t Count(log::Level l) const {
        size_t n = 0;
        for (auto& e : lines) n += e.first == l;
        return n;
    }
};

struct Listener {
    std::string id;
    std::vector<std::string>* trace;
    void OnVoid() { trace->push_back(id); }
    void OnInt(int v) { trace->push_back(id + ":" + std::to_string(v)); }
    void OnOther(int v) { trace->push_back(id + "/other:" + std::to_string(v)); }
    int Twice(int v, std::string) { return v * 2; }
};

struct Derived : Listener {
    virtual ~Derived() = default;
};

void StaticHandler() {}

}  // namespace

TEST_CASE("Delegate: C# equality, Target, multicast Combine/Remove, invocation") {
    std::vector<std::string> trace;
    Listener a{"a", &trace}, b{"b", &trace};
    using D = Delegate<void(int)>;
    const D da = D::Bind(&a, &Listener::OnInt);
    CHECK(da == D::Bind(&a, &Listener::OnInt));         // same target + method
    CHECK_FALSE(da == D::Bind(&b, &Listener::OnInt));   // other target
    CHECK_FALSE(da == D::Bind(&a, &Listener::OnOther)); // other method
    CHECK(da.Target() == &a);
    CHECK(da.TargetTypeName() == "Listener");
    Derived dd;
    dd.id = "d";
    dd.trace = &trace;
    Listener* asBase = &dd;
    CHECK(D::Bind(asBase, &Listener::OnInt).TargetTypeName() == "Listener");  // Listener is not polymorphic
    CHECK(D::Bind(&dd, &Listener::OnInt).TargetTypeName() == "Derived");

    // lambdas: a new identity per Lambda() call; copies stay equal
    const D l1 = D::Lambda(&a, [&](int) { trace.push_back("l"); });
    const D l1copy = l1;
    CHECK(l1 == l1copy);
    CHECK_FALSE(l1 == D::Lambda(&a, [&](int) { trace.push_back("l"); }));

    // static: Target is null -> TargetTypeName throws like evt.Target.GetType()
    const auto s = Delegate<void()>::Static(&StaticHandler);
    CHECK(s.Target() == nullptr);
    CHECK_THROWS_AS(s.TargetTypeName(), NullReferenceException);
    CHECK(s == Delegate<void()>::Static(&StaticHandler));

    // null delegate
    D none;
    CHECK(none == nullptr);
    CHECK_THROWS_AS(none(1), NullReferenceException);
    CHECK_THROWS_AS(none.Target(), NullReferenceException);

    // multicast: invocation order, Target = last, Remove = last occurrence of the sub-list
    D m = da + D::Bind(&b, &Listener::OnInt) + da;
    CHECK(m.GetInvocationList().size() == 3);
    CHECK(m.Target() == &a);
    m(7);
    CHECK(trace == std::vector<std::string>{"a:7", "b:7", "a:7"});
    trace.clear();
    D m2 = m - da;  // removes the LAST a
    m2(1);
    CHECK(trace == std::vector<std::string>{"a:1", "b:1"});
    CHECK(m2.Target() == &b);
    trace.clear();
    D m3 = m - (D::Bind(&b, &Listener::OnInt) + da);  // contiguous run [b, a] at the end
    m3(2);
    CHECK(trace == std::vector<std::string>{"a:2"});
    CHECK((m - (da + da)) == m);  // run not present: unchanged
    CHECK((da - da) == nullptr);
    D acc;
    acc += da;
    acc += da;
    acc -= da;
    CHECK(acc == da);

    // return value of a multicast delegate = last entry's result
    auto r = Delegate<int()>::Lambda(&a, [] { return 1; }) + Delegate<int()>::Lambda(&b, [] { return 2; });
    CHECK(r() == 2);
}

TEST_CASE("EventData: reverse registration order, Remove takes the last match, duplicates allowed") {
    std::vector<std::string> trace;
    Listener a{"a", &trace}, b{"b", &trace}, c{"c", &trace};
    EventData<int> ev("test.order");
    ev.Add(&a, &Listener::OnInt);
    ev.Add(&b, &Listener::OnInt);
    ev.Add(&c, &Listener::OnInt);
    ev.Add(&a, &Listener::OnInt);  // duplicate
    ev.Fire(5);
    CHECK(trace == std::vector<std::string>{"a:5", "c:5", "b:5", "a:5"});  // last added fires first
    trace.clear();
    ev.Remove(&a, &Listener::OnInt);  // removes the LAST registration of a
    ev.Fire(6);
    CHECK(trace == std::vector<std::string>{"c:6", "b:6", "a:6"});
    trace.clear();
    ev.Remove(&a, &Listener::OnOther);  // not registered: no-op
    ev.Remove(&a, &Listener::OnInt);
    ev.Remove(&a, &Listener::OnInt);  // nothing left for a: no-op
    ev.Fire(7);
    CHECK(trace == std::vector<std::string>{"c:7", "b:7"});
    CHECK(ev.Count() == 2);
    CHECK(ev.EventName() == "test.order");
}

TEST_CASE("EventData: Add of a static method or a null delegate throws NullReferenceException") {
    EventVoid ev("test.static");
    CHECK_THROWS_AS(ev.Add(Delegate<void()>::Static(&StaticHandler)), NullReferenceException);
    CHECK_THROWS_AS(ev.Add(Delegate<void()>()), NullReferenceException);
    CHECK(ev.Count() == 0);
}

TEST_CASE("EventData: a throwing listener is logged and the others still run") {
    LogCapture logs;
    std::vector<std::string> trace;
    Listener a{"a", &trace}, c{"c", &trace};
    EventData<int> ev("test.throw");
    ev.Add(&a, &Listener::OnInt);
    ev.Add(EventData<int>::OnEvent::Lambda(&c, [](int) { throw std::runtime_error("boom"); }));
    ev.Add(&c, &Listener::OnInt);
    ev.Fire(1);
    CHECK(trace == std::vector<std::string>{"c:1", "a:1"});
    REQUIRE(logs.Count(log::Level::Error) == 2);
    CHECK(logs.lines[0].second == "Exception handling event test.throw in class Listener:boom");
    CHECK(logs.lines[1].second == "boom");
}

TEST_CASE("EventData re-entrancy (netfx scenario R1): removal during Fire does not affect the current round") {
    std::vector<std::string> trace;
    Listener a{"a", &trace}, b{"b", &trace}, c{"c", &trace};
    EventData<int> ev("test.reentry1");
    ev.Add(&a, &Listener::OnInt);
    ev.Add(&b, &Listener::OnInt);
    // c removes a and itself, and adds b again, while firing
    ev.Add(EventData<int>::OnEvent::Lambda(&c, [&](int v) {
        trace.push_back("c:" + std::to_string(v));
        ev.Remove(&a, &Listener::OnInt);
        ev.Add(&b, &Listener::OnInt);
    }));
    ev.Fire(1);
    CHECK(trace == std::vector<std::string>{"c:1", "b:1", "a:1"});  // a still runs (snapshot), new b does not
    trace.clear();
    ev.Fire(2);
    CHECK(trace == std::vector<std::string>{"b:2", "c:2", "b:2"});
}

TEST_CASE("EventData re-entrancy (netfx scenarios R2/R3): nested Fire of the same event shares the snapshot") {
    std::vector<std::string> trace;
    Listener a{"a", &trace}, b{"b", &trace}, c{"c", &trace}, d{"d", &trace};
    SUBCASE("R2: nested fire, unchanged list -> outer continues on the refilled list") {
        EventData<int> ev("test.reentry2");
        bool fired = false;
        ev.Add(&a, &Listener::OnInt);
        ev.Add(EventData<int>::OnEvent::Lambda(&b, [&](int v) {
            trace.push_back("b:" + std::to_string(v));
            if (!fired) {
                fired = true;
                ev.Fire(v + 10);
            }
        }));
        ev.Add(&c, &Listener::OnInt);
        ev.Fire(1);
        CHECK(trace == std::vector<std::string>{"c:1", "b:1", "c:11", "b:11", "a:11", "a:1"});
    }
    SUBCASE("R3: a listener removes earlier listeners then fires again -> the outer loop repeats listeners") {
        EventData<int> ev("test.reentry3");
        bool fired = false;
        ev.Add(&a, &Listener::OnInt);
        ev.Add(&b, &Listener::OnInt);
        ev.Add(EventData<int>::OnEvent::Lambda(&c, [&](int v) {
            trace.push_back("c:" + std::to_string(v));
            if (!fired) {
                fired = true;
                ev.Remove(&a, &Listener::OnInt);
                ev.Remove(&b, &Listener::OnInt);
                ev.Fire(v + 10);
            }
        }));
        ev.Add(&d, &Listener::OnInt);
        ev.Fire(1);
        // outer: d, c -> nested on [c, d]: d, c; outer continues at index 1 of [c, d]: d, then index 0: c
        CHECK(trace == std::vector<std::string>{"d:1", "c:1", "d:11", "c:11", "d:1", "c:1"});
    }
    SUBCASE("R4: the refilled list is shorter than the outer index -> ArgumentOutOfRange escapes Fire") {
        EventData<int> ev("test.reentry4");
        bool fired = false;
        ev.Add(&a, &Listener::OnInt);
        ev.Add(&b, &Listener::OnInt);
        ev.Add(EventData<int>::OnEvent::Lambda(&c, [&](int v) {
            trace.push_back("c:" + std::to_string(v));
            if (!fired) {
                fired = true;
                ev.Remove(&a, &Listener::OnInt);
                ev.Remove(&b, &Listener::OnInt);
                ev.Fire(v + 10);
            }
        }));
        CHECK_THROWS_AS(ev.Fire(1), std::out_of_range);
        CHECK(trace == std::vector<std::string>{"c:1", "c:11"});
        trace.clear();
        ev.Fire(2);  // still usable afterwards (numEventsFiring stays 1 internally; only the clone's lifetime differs)
        CHECK(trace == std::vector<std::string>{"c:2"});
    }
}

TEST_CASE("EventVoid and debug logging") {
    LogCapture logs;
    std::vector<std::string> trace;
    Listener a{"a", &trace};
    EventVoid ev("test.void");
    ev.debugEvent = true;
    ev.Add(&a, &Listener::OnVoid);
    ev.Fire();
    ev.Remove(&a, &Listener::OnVoid);
    ev.Remove(&a, &Listener::OnVoid);
    CHECK(trace == std::vector<std::string>{"a"});
    REQUIRE(logs.lines.size() == 4);
    CHECK(logs.lines[0].second == "EventManager: Adding event 'test.void' for object of type 'Listener'");
    CHECK(logs.lines[1].second == "EventManager: Firing event 'test.void'");
    CHECK(logs.lines[2].second == "EventManager: Removing event 'test.void' from object of type 'Listener'");
    CHECK(logs.lines[3].second == "EventManager: Removing event 'test.void' - Cannot find listed event");
    ev.debugEvent = false;
    logs.lines.clear();
    GameEventsBase::debugEvents = true;
    ev.Fire();
    GameEventsBase::debugEvents = false;
    CHECK(logs.lines.size() == 1);
}

TEST_CASE("EventDataModifier: listeners chain the value; DefaultModifier is registered first and runs last") {
    LogCapture logs;
    std::vector<std::string> trace;
    Listener a{"a", &trace}, b{"b", &trace};
    EventDataModifier<int, std::string> ev("test.modifier");
    CHECK(ev.Count() == 1);  // DefaultModifier
    CHECK(ev.Fire(3, "x") == 3);
    ev.Add(&a, &Listener::Twice);
    ev.Add(EventDataModifier<int, std::string>::OnEvent::Lambda(&b, [](int v, std::string s) {
        return v + static_cast<int>(s.size());
    }));
    CHECK(ev.Fire(3, "xy") == (3 + 2) * 2);  // b first, then a, then DefaultModifier
    ev.Add(EventDataModifier<int, std::string>::OnEvent::Lambda(&b, [](int, std::string) -> int {
        throw std::runtime_error("bad");
    }));
    CHECK(ev.Fire(3, "xy") == 10);  // the throwing listener leaves the value unchanged
    CHECK(logs.Count(log::Level::Error) == 2);
}

TEST_CASE("Event{Double,Float,Int}{Max,Min}: fold with Math.Max (double) / Mathf.Max (float, int)") {
    Listener o{"o", nullptr};
    EventDoubleMax dmax("test.dmax", -1.0);
    CHECK(dmax.Fire() == -1.0);
    dmax.Add(EventDoubleMax::OnEvent::Lambda(&o, [] { return 2.0; }));
    dmax.Add(EventDoubleMax::OnEvent::Lambda(&o, [] { return 5.0; }));
    CHECK(dmax.Fire() == 5.0);
    // NaN: Math.Max(num, NaN) = NaN and Math.Max(NaN, x) = NaN -> NaN sticks (fires: NaN-returner first)
    dmax.Add(EventDoubleMax::OnEvent::Lambda(&o, [] { return std::numeric_limits<double>::quiet_NaN(); }));
    CHECK(std::isnan(dmax.Fire()));
    // float: Mathf.Max(num, NaN) = NaN, then Mathf.Max(NaN, x) = x -> the NaN is dropped by the next listener
    EventFloatMax fmax("test.fmax");
    fmax.Add(EventFloatMax::OnEvent::Lambda(&o, [] { return 2.0f; }));
    fmax.Add(EventFloatMax::OnEvent::Lambda(&o, [] { return std::numeric_limits<float>::quiet_NaN(); }));
    CHECK(fmax.Fire() == 2.0f);
    EventFloatMin fmin("test.fmin", 10.0f);
    fmin.Add(EventFloatMin::OnEvent::Lambda(&o, [] { return 3.0f; }));
    CHECK(fmin.Fire() == 3.0f);
    EventIntMin imin("test.imin", 4);
    imin.Add(EventIntMin::OnEvent::Lambda(&o, [] { return 9; }));
    CHECK(imin.Fire() == 4);
    EventIntMax imax("test.imax");
    imax.Add(EventIntMax::OnEvent::Lambda(&o, [] { return -3; }));
    CHECK(imax.Fire() == 0);
    EventDoubleMin dmin("test.dmin", 1.0);
    dmin.Add(EventDoubleMin::OnEvent::Lambda(&o, [] { return -0.0; }));
    dmin.Add(EventDoubleMin::OnEvent::Lambda(&o, [] { return 0.0; }));
    // Math.Min(1, 0) = 0, then Math.Min(0, -0): 0 < -0 false -> -0 (val2)
    CHECK(std::signbit(dmin.Fire()));
}

TEST_CASE("EventValueComparison / EventValueOperation: Update folds, no exception handling (sic)") {
    LogCapture logs;
    Listener o{"o", nullptr};
    auto greater = EventValueComparison<int>::OnCompare::Lambda(&o, [](int a, int b) { return a > b; });
    EventValueComparison<int> cmp("PilotSkill", -1, greater);
    CHECK(cmp.value() == -1);
    cmp.Add(EventValueComparison<int>::OnEvent::Lambda(&o, [] { return 3; }));
    cmp.Add(EventValueComparison<int>::OnEvent::Lambda(&o, [] { return 1; }));
    cmp.Update();
    CHECK(cmp.value() == 3);
    CHECK(cmp.EventName() == "PilotSkill");
    cmp.debugEvent = true;
    cmp.Update();
    REQUIRE(!logs.lines.empty());
    CHECK(logs.lines.back().second == "EventManager: Firing event ''");  // hidden, never-assigned eventName (sic)
    cmp.debugEvent = false;
    cmp.Add(EventValueComparison<int>::OnEvent::Lambda(&o, []() -> int { throw std::runtime_error("x"); }));
    CHECK_THROWS_AS(cmp.Update(), std::runtime_error);
    CHECK(cmp.NumEventsFiring() == 1);  // no finally: the nesting counter stays incremented

    auto sum = EventValueOperation<float>::OnOperation::Lambda(&o, [](float a, float b) { return a + b; });
    EventValueOperation<float> op("ScienceReturn", 1.0f, sum);
    op.Add(EventValueOperation<float>::OnEvent::Lambda(&o, [] { return 0.5f; }));
    op.Add(EventValueOperation<float>::OnEvent::Lambda(&o, [] { return 0.25f; }));
    op.Update();
    CHECK(op.value() == 1.75f);
    op.Update();
    CHECK(op.value() == 1.75f);  // restarts from defaultValue
    CHECK(op.gameEventName().empty());
}

namespace {
struct Values : EventValueWrapper {
    std::vector<std::string>* trace;
    struct Item {
        std::string id;
        std::vector<std::string>* trace;
        void Update() { trace->push_back(id); }
    };
    Item first{"first", nullptr}, second{"second", nullptr};
    explicit Values(std::vector<std::string>* t) : trace(t) {
        first.trace = second.trace = t;
        RegisterUpdates(first, second);
    }
};
}  // namespace

TEST_CASE("EventValueWrapper: Update calls the registered members' Update in declaration order") {
    std::vector<std::string> trace;
    Values v(&trace);
    v.Update();
    CHECK(trace == std::vector<std::string>{"first", "second"});
}

TEST_CASE("FindEvent: by name, type mismatch logs and returns null, later events replace earlier names") {
    LogCapture logs;
    EventData<int> e1("test.find");
    CHECK(BaseGameEvent::FindEvent<EventData<int>>("test.find") == &e1);
    CHECK(BaseGameEvent::FindEvent<BaseGameEvent>("test.find") == &e1);
    CHECK(BaseGameEvent::FindEvent<EventVoid>("test.find") == nullptr);
    CHECK(logs.Count(log::Level::Error) == 1);
    CHECK(logs.lines[0].second.rfind("Type mismatch when searching for event with name test.find.  Expected type ", 0) == 0);
    CHECK(BaseGameEvent::FindEvent<EventVoid>("test.nothing") == nullptr);
    {
        EventVoid e2("test.find");  // same name: replaces the entry
        CHECK(GameEvents::FindEvent<EventVoid>("test.find") == &e2);
    }
    // C++ only: a destroyed event unregisters itself (C# keeps it alive in the dictionary)
    CHECK(GameEvents::FindEvent<BaseGameEvent>("test.find") == nullptr);
}

TEST_CASE("GameEvents registry: names, nesting, duplicate names, modifiers") {
    CHECK(GameEvents::onVesselChange.EventName() == "onVesselChange");
    CHECK(GameEvents::VesselSituation::onLand.EventName() == "VesselSituation.OnLand");
    CHECK(GameEvents::Contract::onOffered.EventName() == "Contract.Generated");
    CHECK(GameEvents::Modifiers.FundsGain.EventName() == "Modify.FundsGain");
    CHECK(GameEvents::Modifiers.FundsGain.Count() == 1);  // DefaultModifier
    // duplicate names: the later declaration owns the name
    CHECK(GameEvents::onPartDeCouple.EventName() == "onPartCouple");
    CHECK(GameEvents::FindEvent<EventData<Part*>>("onPartCouple") == &GameEvents::onPartDeCouple);
    {
        LogCapture logs;
        using Couple = EventData<GameEvents::FromToAction<Part*, Part*>>;
        CHECK(GameEvents::FindEvent<Couple>("onPartCouple") == nullptr);  // type mismatch (sic, KSP behaves the same)
        CHECK(logs.Count(log::Level::Error) == 1);
    }
    CHECK(GameEvents::FindEvent<EventData<Vessel*, Part*>>("OnEVAConstructionModePartAttached") ==
          &GameEvents::OnEVAConstructionModePartDetached);
    CHECK(GameEvents::FindEvent<EventData<FlightState*>>("OnFlightState") == &GameEvents::OnRevertToPrelaunchFlightState);
    // events carrying value structs
    std::vector<std::string> trace;
    Listener a{"a", &trace};
    using HT = GameEvents::HostTargetAction<Part*, Part*>;
    auto& ev = GameEvents::onPartAttach;
    const auto listener = EventData<HT>::OnEvent::Lambda(
        &a, [&](HT h) { trace.push_back(h.host == nullptr && h.target == nullptr ? "null" : "set"); });
    ev.Add(listener);
    ev.Fire(HT(nullptr, nullptr));
    CHECK(trace == std::vector<std::string>{"null"});
    // the registry is process-wide: remove what this test added
    ev.Remove(listener);
    CHECK(ev.Count() == 0);
}
