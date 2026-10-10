// WP-00: TimingManager phase order, add / remove during iteration, the coroutine scheduler (against the independent Python replay
// tools/wp00/coroutine_reference.py -> tests/data/wp00_reference.json) and the HierarchyUtil / TransformExtension / GameObjectExtension
// pure functions. No golden or capture covers scheduling; the phase order is checked against the in-game one-frame call trace
// analysis/physics/raw/run1/run1-L_calltrace.csv (read here when the analysis tree is present).
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/core/coroutine.hpp"
#include "ksp/core/coroutine_host.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/timing_manager.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/util/hierarchy_util.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using Stage = TimingManager::TimingStage;
using Action = TimingManager::UpdateAction;

namespace {

// Restores the process-wide Time statics the PlayerLoop writes.
struct TimeGuard {
    float time = Time::time, dt = Time::deltaTime, fdt = Time::fixedDeltaTime;
    int frames = Time::frameCount;
    ~TimeGuard() {
        Time::time = time;
        Time::deltaTime = dt;
        Time::fixedDeltaTime = fdt;
        Time::frameCount = frames;
    }
};

struct LogCapture {
    std::vector<std::string> errors;
    LogCapture() {
        log::SetSink([this](log::Level l, std::string_view m) {
            if (l == log::Level::Error) errors.emplace_back(m);
        });
    }
    ~LogCapture() { log::SetSink(nullptr); }
};

struct Owner {};  // delegate target

Action Recorder(Owner* o, std::vector<std::string>& out, std::string what) {
    return Action::Lambda(o, [&out, what] { out.push_back(what); });
}

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
float FromBits(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

const char* const kStageNames[9] = {"ObscenelyEarly", "Early", "Precalc", "Earlyish", "Normal", "FashionablyLate", "FlightIntegrator", "Late", "BetterLateThanNever"};

}  // namespace

// ---------------------------------------------------------------------------------------------------------------------------------
// Phase order
// ---------------------------------------------------------------------------------------------------------------------------------

TEST_CASE("WP-00 TimingManager: Awake / Instance / OnDestroy (TimingManager.cs:45, :64)") {
    LogCapture log;
    CHECK(TimingManager::Instance() == nullptr);
    {
        TimingManager a;
        CHECK(a.Awake());
        CHECK(TimingManager::Instance() == &a);
        TimingManager b;  // TimingManager.cs:47: a second manager logs and destroys itself
        CHECK_FALSE(b.Awake());
        CHECK(b.IsDestroyed());
        REQUIRE(log.errors.size() == 1);
        CHECK(log.errors[0] == "TimingManager: Instance already exists, destroying.");
        CHECK(TimingManager::Instance() == &a);
        b.OnDestroy();  // does not clear the Instance of another manager (:94)
        CHECK(TimingManager::Instance() == &a);
        a.OnDestroy();
        CHECK(TimingManager::Instance() == nullptr);
    }
    // no Instance: every *Add / *Remove is a silent no-op (:130)
    std::vector<std::string> out;
    Owner o;
    TimingManager::UpdateAdd(Stage::Normal, Recorder(&o, out, "x"));
    TimingManager::FixedUpdateRemove(Stage::Normal, Recorder(&o, out, "x"));
    CHECK(log.errors.size() == 1);
}

TEST_CASE("WP-00 TimingManager: stage -> host mapping (TimingManager.cs:128-185)") {
    TimingManager tm;
    REQUIRE(tm.Awake());
    const char* const hosts[9] = {"Timing0", "Timing1", "TimingPre", "Timing2", "", "Timing3", "TimingFI", "Timing4", "Timing5"};
    for (int i = 0; i < 9; i++) {
        const Stage st = static_cast<Stage>(i);
        if (st == Stage::Normal) {
            CHECK(tm.Phase(st) == nullptr);
        } else {
            REQUIRE(tm.Phase(st) != nullptr);
            CHECK(tm.Phase(st)->Name() == hosts[i]);
        }
    }
    CHECK(tm.Phase(static_cast<Stage>(42)) == nullptr);  // outside the enum: the C# switch matches nothing
    // UpdateAdd lands in the right delegate and only there
    std::vector<std::string> out;
    Owner o;
    for (int i = 0; i < 9; i++) TimingManager::UpdateAdd(static_cast<Stage>(i), Recorder(&o, out, kStageNames[i]));
    TimingManager::UpdateAdd(static_cast<Stage>(42), Recorder(&o, out, "bogus"));  // no-op
    tm.TimingManager::Update();
    CHECK(out == std::vector<std::string>{"Normal"});
    tm.Phase(Stage::Precalc)->Update();
    CHECK(out.back() == "Precalc");
    CHECK(out.size() == 2);
    // fixed / late delegates are separate
    CHECK(tm.Phase(Stage::Early)->onFixedUpdate.IsNull());
    CHECK(tm.onFixedUpdate.IsNull());
}

TEST_CASE("WP-00 PlayerLoop: the measured order of one fixed step and one frame (run1-L_calltrace.csv)") {
    TimeGuard tg;
    TimingManager tm;
    REQUIRE(tm.Awake());
    CoroutineScheduler sched;
    PlayerLoop loop(sched);
    loop.AddTimingManager(tm);
    loop.recordTrace = true;
    std::vector<std::string> callbacks;
    Owner o;
    for (int i = 0; i < 9; i++) {
        const Stage st = static_cast<Stage>(i);
        TimingManager::UpdateAdd(st, Recorder(&o, callbacks, std::string("U:") + kStageNames[i]));
        TimingManager::FixedUpdateAdd(st, Recorder(&o, callbacks, std::string("F:") + kStageNames[i]));
        TimingManager::LateUpdateAdd(st, Recorder(&o, callbacks, std::string("L:") + kStageNames[i]));
    }
    loop.physicsStep = Action::Lambda(&o, [&] { callbacks.push_back("physics"); });

    // one fixed step. Rows of run1-L_calltrace.csv: 4 FixedUpdate.ScriptRunBehaviourFixedUpdate, 5 Timing0, 8 TimingPre, 10 Timing1,
    // 13 Timing2, 14 TimingManager, 277 Timing3, 279 TimingFI, 283 Timing4, 286 Timing5, 291 PhysicsFixedUpdate, 297 ScriptRunDelayedFixedFrameRate
    loop.FixedStep();
    const std::vector<std::string> fixedTrace = {
        "FixedUpdate.ScriptRunBehaviourFixedUpdate", "Timing0.FixedUpdate", "TimingPre.FixedUpdate", "Timing1.FixedUpdate",
        "Timing2.FixedUpdate", "TimingManager.FixedUpdate", "Timing3.FixedUpdate", "TimingFI.FixedUpdate", "Timing4.FixedUpdate",
        "Timing5.FixedUpdate", "FixedUpdate.PhysicsFixedUpdate", "FixedUpdate.ScriptRunDelayedFixedFrameRate"};
    CHECK(loop.trace == fixedTrace);
    CHECK(callbacks == std::vector<std::string>{"F:ObscenelyEarly", "F:Precalc", "F:Early", "F:Earlyish", "F:Normal", "F:FashionablyLate",
                                                 "F:FlightIntegrator", "F:Late", "F:BetterLateThanNever", "physics"});

    // one frame without fixed steps. Rows: 299 PreUpdate.PhysicsUpdate, 309 ScriptRunBehaviourUpdate, 310 Timing0 ... 584 Timing5,
    // 585 Update.ScriptRunDelayedDynamicFrameRate, 621 PreLateUpdate.ScriptRunBehaviourLateUpdate, 622 Timing0.LateUpdate ... 897 Timing5.LateUpdate,
    // 901 PostLateUpdate.ScriptRunDelayedDynamicFrameRate, 929 PostLateUpdate.TriggerEndOfFrameCallbacks
    loop.trace.clear();
    callbacks.clear();
    loop.Frame(0.02f, 0);
    const std::vector<std::string> frameTrace = {
        "PreUpdate.PhysicsUpdate", "Update.ScriptRunBehaviourUpdate", "Timing0.Update", "TimingPre.Update", "Timing1.Update",
        "Timing2.Update", "TimingManager.Update", "Timing3.Update", "TimingFI.Update", "Timing4.Update", "Timing5.Update",
        "Update.ScriptRunDelayedDynamicFrameRate", "PreLateUpdate.ScriptRunBehaviourLateUpdate", "Timing0.LateUpdate",
        "TimingPre.LateUpdate", "Timing1.LateUpdate", "Timing2.LateUpdate", "TimingManager.LateUpdate", "Timing3.LateUpdate",
        "TimingFI.LateUpdate", "Timing4.LateUpdate", "Timing5.LateUpdate", "PostLateUpdate.ScriptRunDelayedDynamicFrameRate",
        "PostLateUpdate.TriggerEndOfFrameCallbacks"};
    CHECK(loop.trace == frameTrace);

    // a frame with 2 fixed steps: [step, step] precede PreUpdate.PhysicsUpdate
    loop.trace.clear();
    loop.Frame(0.04f, 2);
    REQUIRE(loop.trace.size() == 2 * fixedTrace.size() + frameTrace.size());
    CHECK(std::vector<std::string>(loop.trace.begin(), loop.trace.begin() + 12) == fixedTrace);
    CHECK(std::vector<std::string>(loop.trace.begin() + 12, loop.trace.begin() + 24) == fixedTrace);
    CHECK(loop.trace[24] == "PreUpdate.PhysicsUpdate");
}

TEST_CASE("WP-00 PlayerLoop: ExecutionOrder() == the order in run1-L_calltrace.csv (all three profiles' call traces)") {
    // Read the in-game traces when the analysis tree is next to the core (it is for a normal checkout and for the worktrees).
    const std::string root = KSPCORE_PROJECT_ROOT;
    int checkedFiles = 0;
    for (const char* prof : {"L", "M", "S"}) {
        std::ifstream in(root + "/analysis/physics/raw/run1/run1-" + prof + "_calltrace.csv");
        if (!in) continue;
        std::string line;
        std::getline(in, line);  // seq,kind,name,...
        std::map<std::string, std::vector<std::pair<int, std::string>>> byKind;  // "FixedUpdate" -> (seq, host)
        while (std::getline(in, line)) {
            std::stringstream ss(line);
            std::string seq, kind, name;
            std::getline(ss, seq, ',');
            std::getline(ss, kind, ',');
            std::getline(ss, name, ',');
            if (kind != "callback" || name.rfind("Timing", 0) != 0) continue;
            const size_t dot = name.find('.');
            byKind[name.substr(dot + 1)].push_back({std::stoi(seq), name.substr(0, dot)});
        }
        if (byKind.empty()) continue;
        checkedFiles++;
        const auto& order = TimingManager::ExecutionOrder();
        for (const char* method : {"FixedUpdate", "Update", "LateUpdate"}) {
            auto& rows = byKind[method];
            REQUIRE(rows.size() == 9);
            for (int i = 0; i < 9; i++) CHECK_MESSAGE(rows[i].second == order[i].name, prof << " " << method << " position " << i);
            if (std::string(prof) == "L")
                for (int i = 0; i < 9; i++) {
                    const int expect = std::string(method) == "FixedUpdate" ? order[i].traceSeqFixed : std::string(method) == "Update" ? order[i].traceSeqUpdate : order[i].traceSeqLate;
                    CHECK(rows[i].first == expect);
                }
        }
    }
    if (checkedFiles == 0) MESSAGE("analysis/physics/raw/run1 not present: trace cross-check skipped");
    // the Precalc position (before Early) differs from the enum order (TimingManager.cs:5-15): the measurement wins
    CHECK(PlayerLoop::TimingOrderKey(Stage::Precalc) < PlayerLoop::TimingOrderKey(Stage::Early));
}

TEST_CASE("WP-00 PlayerLoop: other scripts interleave by execution-order key; exceptions in scripts are logged and skipped") {
    TimeGuard tg;
    LogCapture log;
    TimingManager tm;
    REQUIRE(tm.Awake());
    CoroutineScheduler sched;
    PlayerLoop loop(sched);
    loop.AddTimingManager(tm);
    loop.recordTrace = true;
    Owner o;
    std::vector<std::string> hits;
    // run1-L_calltrace.csv rows 6 Planetarium (between Timing0 and TimingPre), 9 OrbitDriver (between TimingPre and Timing1)
    loop.AddScript("Planetarium", 50, nullptr, Recorder(&o, hits, "pl"), nullptr);
    loop.AddScript("OrbitDriver", 150, nullptr, Action::Lambda(&o, [] { throw Exception("boom"); }), nullptr);
    loop.FixedStep();
    std::vector<std::string> names;
    for (const auto& t : loop.trace) names.push_back(t);
    CHECK(names[1] == "Timing0.FixedUpdate");
    CHECK(names[2] == "Planetarium.FixedUpdate");
    CHECK(names[3] == "TimingPre.FixedUpdate");
    CHECK(names[4] == "OrbitDriver.FixedUpdate");
    CHECK(names[5] == "Timing1.FixedUpdate");  // the exception of OrbitDriver did not stop the loop
    REQUIRE(log.errors.size() == 1);
    CHECK(log.errors[0].find("OrbitDriver.FixedUpdate threw") == 0);
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Add / remove during iteration (System.Delegate semantics)
// ---------------------------------------------------------------------------------------------------------------------------------

TEST_CASE("WP-00 TimingManager: Add / Remove are Delegate.Combine / Remove (last occurrence), duplicates run twice") {
    TimingManager tm;
    REQUIRE(tm.Awake());
    Owner o;
    int a = 0, b = 0;
    Action fa = Action::Lambda(&o, [&] { a++; });
    Action fb = Action::Lambda(&o, [&] { b++; });
    TimingManager::UpdateAdd(Stage::Normal, fa);
    TimingManager::UpdateAdd(Stage::Normal, fb);
    TimingManager::UpdateAdd(Stage::Normal, fa);  // twice: it runs twice
    tm.TimingManager::Update();
    CHECK(a == 2);
    CHECK(b == 1);
    // Remove drops the LAST occurrence: the order is fa, fb afterwards (the first fa stays)
    TimingManager::UpdateRemove(Stage::Normal, fa);
    CHECK(tm.onUpdate.GetInvocationList().size() == 2);
    std::vector<std::string> order;
    Action ra = Recorder(&o, order, "a");
    Action rb = Recorder(&o, order, "b");
    tm.onUpdate = nullptr;
    TimingManager::UpdateAdd(Stage::Normal, ra);
    TimingManager::UpdateAdd(Stage::Normal, rb);
    TimingManager::UpdateAdd(Stage::Normal, ra);
    TimingManager::UpdateRemove(Stage::Normal, ra);
    tm.TimingManager::Update();
    CHECK(order == std::vector<std::string>{"a", "b"});
    // removing something absent, a null action, or from a null delegate: no change, no error
    Action stranger = Action::Lambda(&o, [] {});
    TimingManager::UpdateRemove(Stage::Normal, stranger);
    TimingManager::UpdateRemove(Stage::Late, stranger);  // Timing4 has a null delegate
    TimingManager::UpdateAdd(Stage::Normal, nullptr);
    CHECK(tm.onUpdate.GetInvocationList().size() == 2);
    CHECK(tm.Phase(Stage::Late)->onUpdate.IsNull());
}

TEST_CASE("WP-00 TimingManager: a callback that adds / removes listeners does not change the running invocation (snapshot)") {
    TimingManager tm;
    REQUIRE(tm.Awake());
    Owner o;
    std::vector<std::string> order;
    Action late = Recorder(&o, order, "late");
    Action second = Recorder(&o, order, "second");
    Action first = Action::Lambda(&o, [&] {
        order.push_back("first");
        TimingManager::FixedUpdateAdd(Stage::Early, late);      // added during the iteration: runs next time, not now
        TimingManager::FixedUpdateRemove(Stage::Early, second); // removed during the iteration: still runs now (C# invokes the old list)
    });
    TimingManager::FixedUpdateAdd(Stage::Early, first);
    TimingManager::FixedUpdateAdd(Stage::Early, second);
    tm.Phase(Stage::Early)->FixedUpdate();
    CHECK(order == std::vector<std::string>{"first", "second"});
    order.clear();
    tm.Phase(Stage::Early)->FixedUpdate();  // now: first, late (second was removed; first adds a second `late`)
    CHECK(order == std::vector<std::string>{"first", "late"});
    // a callback removing itself (OnDestroy-style) is invoked this time and not the next
    int selfRuns = 0;
    Action self;
    self = Action::Lambda(&o, [&] {
        selfRuns++;
        TimingManager::LateUpdateRemove(Stage::Late, self);
    });
    TimingManager::LateUpdateAdd(Stage::Late, self);
    tm.Phase(Stage::Late)->LateUpdate();
    tm.Phase(Stage::Late)->LateUpdate();
    CHECK(selfRuns == 1);
}

TEST_CASE("WP-00 Timing phases: an exception is logged and swallowed (Timing0.cs:12-26); the Normal stage lets it escape") {
    LogCapture log;
    TimingManager tm;
    REQUIRE(tm.Awake());
    Owner o;
    std::vector<std::string> order;
    TimingManager::UpdateAdd(Stage::Early, Recorder(&o, order, "e1"));
    TimingManager::UpdateAdd(Stage::Early, Action::Lambda(&o, [] { throw Exception("kaput"); }));
    TimingManager::UpdateAdd(Stage::Early, Recorder(&o, order, "e3"));  // skipped: the multicast delegate stops at the throw
    tm.Phase(Stage::Early)->Update();
    CHECK(order == std::vector<std::string>{"e1"});
    REQUIRE(log.errors.size() == 1);
    CHECK(log.errors[0].find("Timing1 threw during Update: ") == 0);
    TimingManager::FixedUpdateAdd(Stage::Precalc, Action::Lambda(&o, [] { throw Exception("x"); }));
    tm.Phase(Stage::Precalc)->FixedUpdate();
    TimingManager::LateUpdateAdd(Stage::BetterLateThanNever, Action::Lambda(&o, [] { throw Exception("y"); }));
    tm.Phase(Stage::BetterLateThanNever)->LateUpdate();
    REQUIRE(log.errors.size() == 3);
    CHECK(log.errors[1].find("TimingPre threw during FixedUpdate: ") == 0);
    CHECK(log.errors[2].find("Timing5 threw during LateUpdate: ") == 0);
    // TimingManager.Update has no try / catch (TimingManager.cs:104)
    TimingManager::UpdateAdd(Stage::Normal, Action::Lambda(&o, [] { throw Exception("z"); }));
    CHECK_THROWS_AS(tm.TimingManager::Update(), Exception);
}

TEST_CASE("WP-00 PlayerLoop: Time.time / Time.deltaTime inside a fixed step and in Update") {
    TimeGuard tg;
    Time::fixedDeltaTime = 0.02f;
    Time::time = 0.0f;
    Time::frameCount = 0;
    CoroutineScheduler sched;
    PlayerLoop loop(sched);
    Owner o;
    std::vector<float> fixedT, fixedD, updT, updD;
    loop.AddScript("S", 0, Action::Lambda(&o, [&] { updT.push_back(Time::time); updD.push_back(Time::deltaTime); }),
                   Action::Lambda(&o, [&] { fixedT.push_back(Time::time); fixedD.push_back(Time::deltaTime); }), nullptr);
    loop.Frame(0.0333f, 2);
    loop.Frame(0.0167f, 0);
    REQUIRE(fixedT.size() == 2);
    CHECK(fixedT[0] == 0.02f);
    CHECK(fixedT[1] == static_cast<float>(static_cast<double>(0.02f) + static_cast<double>(0.02f)));
    CHECK(fixedD[0] == 0.02f);
    REQUIRE(updT.size() == 2);
    CHECK(updT[0] == static_cast<float>(static_cast<double>(0.0333f)));
    CHECK(updD[0] == 0.0333f);
    CHECK(updT[1] == static_cast<float>(static_cast<double>(0.0333f) + static_cast<double>(0.0167f)));
    CHECK(updD[1] == 0.0167f);
    CHECK(Time::frameCount == 2);
    // outside the loop the statics are the frame's
    CHECK(Time::deltaTime == 0.0167f);
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Coroutine scheduler
// ---------------------------------------------------------------------------------------------------------------------------------

namespace {
Enumerator Counter(std::vector<std::string>& log, std::string name, int n, YieldInstruction y) {
    for (int i = 0; i < n; i++) {
        log.push_back(name + std::to_string(i));
        co_yield y;
    }
    log.push_back(name + "end");
}
}  // namespace

TEST_CASE("WP-00 coroutines: StartCoroutine runs to the first yield; yield null waits for a later frame") {
    TimeGuard tg;
    CoroutineScheduler s;
    PlayerLoop loop(s);
    std::vector<std::string> log;
    auto c = s.StartCoroutine(Counter(log, "a", 2, YieldInstruction::Null()));
    CHECK(log == std::vector<std::string>{"a0"});  // synchronously
    CHECK_FALSE(c->Finished());
    loop.Frame(0.02f, 0);                          // frame 1: resumes (yielded in frame 0)
    CHECK(log == std::vector<std::string>{"a0", "a1"});
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"a0", "a1", "aend"});
    CHECK(c->Finished());
}

TEST_CASE("WP-00 coroutines: yield-null routine started during Update resumes next frame") {
    TimeGuard tg;
    CoroutineScheduler s;
    PlayerLoop loop(s);
    Owner o;
    std::vector<std::string> log;
    bool started = false;
    loop.AddScript("starter", 0, Action::Lambda(&o, [&] {
        if (started) return;
        started = true;
        s.StartCoroutine(Counter(log, "b", 1, YieldInstruction::Null()));
    }), nullptr, nullptr);
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"b0"});
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"b0", "bend"});
}

TEST_CASE("WP-00 coroutines: WaitForFixedUpdate resumes after PhysicsFixedUpdate, in the same step when yielded in FixedUpdate") {
    TimeGuard tg;
    CoroutineScheduler s;
    PlayerLoop loop(s);
    loop.recordTrace = true;
    Owner o;
    std::vector<std::string> log;
    bool started = false;
    loop.AddScript("S", 0, nullptr, Action::Lambda(&o, [&] {
        if (started) return;
        started = true;
        s.StartCoroutine(Counter(log, "f", 2, YieldInstruction::WaitForFixedUpdate()));
    }), nullptr);
    loop.physicsStep = Action::Lambda(&o, [&] { log.push_back("physics"); });
    loop.FixedStep();  // starts in FixedUpdate (f0), physics, then the pass resumes it (f1)
    CHECK(log == std::vector<std::string>{"f0", "physics", "f1"});
    loop.FixedStep();
    CHECK(log == std::vector<std::string>{"f0", "physics", "f1", "physics", "fend"});
}

TEST_CASE("WP-00 coroutines: WaitForEndOfFrame, WaitUntil / WaitWhile, nested coroutines, StopCoroutine") {
    TimeGuard tg;
    CoroutineScheduler s;
    PlayerLoop loop(s);
    std::vector<std::string> log;
    s.StartCoroutine(Counter(log, "e", 2, YieldInstruction::WaitForEndOfFrame()));
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"e0", "e1"});  // resumed at the end of frame 1, yields again for frame 2
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"e0", "e1", "eend"});

    log.clear();
    int gate = 0;
    s.StartCoroutine(Counter(log, "u", 1, YieldInstruction::WaitUntil([&] { return gate >= 2; })));
    s.StartCoroutine(Counter(log, "w", 1, YieldInstruction::WaitWhile([&] { return gate < 1; })));
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"u0", "w0"});
    gate = 1;
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"u0", "w0", "wend"});
    gate = 2;
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"u0", "w0", "wend", "uend"});

    // nested: yield return StartCoroutine(child) resumes the parent when the child ends
    log.clear();
    struct Nest {
        static Enumerator Parent(CoroutineScheduler& s, std::vector<std::string>& log) {
            log.push_back("parent0");
            co_yield YieldInstruction::Wait(s.StartCoroutine(Counter(log, "child", 1, YieldInstruction::Null())));
            log.push_back("parent1");
        }
    };
    s.StartCoroutine(Nest::Parent(s, log));
    CHECK(log == std::vector<std::string>{"parent0", "child0"});
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"parent0", "child0", "childend", "parent1"});

    // StopCoroutine: never resumes
    log.clear();
    auto c = s.StartCoroutine(Counter(log, "s", 3, YieldInstruction::Null()));
    s.StopCoroutine(c);
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"s0"});
    CHECK(c->Finished());
    // an exception ends only that routine (logged)
    LogCapture lc;
    struct Thrower {
        static Enumerator Run() {
            co_yield YieldInstruction::Null();
            throw Exception("bad");
        }
    };
    s.StartCoroutine(Thrower::Run());
    loop.Frame(0.02f, 0);
    CHECK(lc.errors.size() == 1);
    CHECK(s.Pending() == 0);
}

TEST_CASE("WP-00 CallbackUtil.DelayedCallback and CoroutineHost (CallbackUtil.cs:26-45, CoroutineHost.cs)") {
    TimeGuard tg;
    CoroutineScheduler s;
    PlayerLoop loop(s);
    Owner o;
    int fired = 0;
    Callback<> cb = Callback<>::Lambda(&o, [&] { fired++; });
    s.StartCoroutine(CallbackUtil::DelayedCallback(3, cb));
    loop.Frame(0.02f, 0);
    loop.Frame(0.02f, 0);
    CHECK(fired == 0);
    loop.Frame(0.02f, 0);  // 3 yields: started in frame 0, resumed in frames 1, 2, 3: the callback runs in frame 3
    CHECK(fired == 1);
    loop.Frame(0.02f, 0);
    CHECK(fired == 1);

    fired = 0;
    s.StartCoroutine(CallbackUtil::DelayedCallback(0.05f, cb));
    loop.Frame(0.02f, 0);
    loop.Frame(0.02f, 0);
    CHECK(fired == 0);  // 0.04 < 0.05
    loop.Frame(0.02f, 0);
    CHECK(fired == 1);  // 0.06 >= 0.05

    fired = 0;
    auto inner = s.StartCoroutine(Counter(*new std::vector<std::string>(), "i", 1, YieldInstruction::Null()));  // intentionally leaked log
    s.StartCoroutine(CallbackUtil::DelayedCallback(inner, cb));
    loop.Frame(0.02f, 0);
    CHECK(fired == 1);

    // CoroutineHost: a disposable host terminates itself when its coroutine ends
    auto host = CoroutineHost::Create(s, "Host", true, true);
    CHECK(host->Persistent());
    std::vector<std::string> log;
    host->StartCoroutine(Counter(log, "h", 2, YieldInstruction::Null()));
    CHECK_FALSE(host->IsDestroyed());
    loop.Frame(0.02f, 0);
    CHECK_FALSE(host->IsDestroyed());
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"h0", "h1", "hend"});
    CHECK(host->IsDestroyed());
    CHECK(s.Pending() == 0);
    // a non-disposable host keeps living; destroying it stops its coroutines
    auto keep = CoroutineHost::Create(s, "Keep", false, false);
    log.clear();
    keep->StartCoroutine(Counter(log, "k", 3, YieldInstruction::Null()));
    loop.Frame(0.02f, 0);
    keep->Terminate();
    loop.Frame(0.02f, 0);
    CHECK(log == std::vector<std::string>{"k0", "k1"});
}

TEST_CASE("WP-00 coroutine scheduler vs the independent numpy float32 replay (tests/data/wp00_reference.json, 0 ULP)") {
    std::ifstream in(std::string(KSPCORE_TEST_DATA_DIR) + "/wp00_reference.json", std::ios::binary);
    REQUIRE(in);
    const nlohmann::json ref = nlohmann::json::parse(in);
    TimeGuard tg;
    const int frames = ref["frames"].get<int>();
    Time::fixedDeltaTime = FromBits(ref["fixed_dt_bits"].get<uint32_t>());
    Time::time = 0.0f;
    Time::frameCount = 0;

    CoroutineScheduler s;
    PlayerLoop loop(s);
    using Rec = std::vector<std::array<uint32_t, 3>>;
    Rec w1, w2, w3, n, f, e;
    int fixedStepNo = 0;
    int curFrame = 0;

    struct Co {
        // WaitForSeconds chain: records (frame, bits(Time.time))
        static Enumerator Chain(Rec& rec, const int& frame, std::vector<float> secs) {
            for (float sec : secs) {
                co_yield YieldInstruction::WaitForSeconds(sec);
                rec.push_back({static_cast<uint32_t>(frame), Bits(Time::time), 0u});
            }
        }
        // yield null x8, accumulating Time.deltaTime in a float (Mono rule: f32store(fx(a) + fx(b)))
        static Enumerator Nulls(Rec& rec, const int& frame) {
            float acc = 0.0f;
            for (int i = 0; i < 8; i++) {
                co_yield YieldInstruction::Null();
                acc = f32store(fx(acc) + fx(Time::deltaTime));
                rec.push_back({static_cast<uint32_t>(frame), Bits(acc), 0u});
            }
        }
        static Enumerator Fixed(Rec& rec, const int& frame, int& step) {
            for (int i = 0; i < 6; i++) {
                co_yield YieldInstruction::WaitForFixedUpdate();
                rec.push_back({static_cast<uint32_t>(frame), Bits(Time::time), static_cast<uint32_t>(step)});
            }
        }
        static Enumerator EndOfFrame(Rec& rec, const int& frame) {
            for (int i = 0; i < 5; i++) {
                co_yield YieldInstruction::WaitForEndOfFrame();
                rec.push_back({static_cast<uint32_t>(frame), Bits(Time::time), 0u});
            }
        }
    };
    Owner o;
    loop.physicsStep = Action::Lambda(&o, [&] { fixedStepNo++; });  // the step counter is bumped in PhysicsFixedUpdate (before the delayed pass)
    s.StartCoroutine(Co::Chain(w1, curFrame, std::vector<float>(6, 0.1f)));
    s.StartCoroutine(Co::Chain(w2, curFrame, std::vector<float>(4, 0.25f)));
    s.StartCoroutine(Co::Chain(w3, curFrame, {1.5f, 0.016f, 0.016f, 0.016f, 0.016f, 0.016f}));
    s.StartCoroutine(Co::Nulls(n, curFrame));
    s.StartCoroutine(Co::Fixed(f, curFrame, fixedStepNo));
    s.StartCoroutine(Co::EndOfFrame(e, curFrame));
    for (int k = 1; k <= frames; k++) {
        curFrame = k;
        loop.Frame(FromBits(ref["dt_bits"][k - 1].get<uint32_t>()), ref["n_fixed"][k - 1].get<int>());
    }
    auto check = [&](const char* key, const Rec& got) {
        const auto& want = ref[key];
        REQUIRE_MESSAGE(got.size() == want.size(), key);
        for (size_t i = 0; i < got.size(); i++) {
            CHECK_MESSAGE(got[i][0] == want[i][0].get<uint32_t>(), key << " frame " << i);
            CHECK_MESSAGE(got[i][1] == want[i][1].get<uint32_t>(), key << " bits " << i);  // 0 ULP
            if (std::string(key) == "f") CHECK_MESSAGE(got[i][2] == want[i][2].get<uint32_t>(), key << " step " << i);
        }
    };
    check("w1", w1);
    check("w2", w2);
    check("w3", w3);
    check("n", n);
    check("f", f);
    check("e", e);
    CHECK(fixedStepNo == ref["total_fixed_steps"].get<int>());
    CHECK(Bits(static_cast<float>(s.CurTime())) == ref["clock_end_bits"][0].get<uint32_t>());
}

// ---------------------------------------------------------------------------------------------------------------------------------
// HierarchyUtil / TransformExtension / GameObjectExtension
// ---------------------------------------------------------------------------------------------------------------------------------

namespace {
using UnityEngine::Transform;
struct Tree {
    // root / (a / (a1, a2 / a2x), b)
    Transform root, a, a1, a2, a2x, b;
    Tree() {
        root.name = "SpaceCenter";
        a.name = "Launch Pad";
        a1.name = "Flag Pole";
        a2.name = "Crew Building";
        a2x.name = "Door";
        b.name = "VAB";
        a.SetParent(&root);
        b.SetParent(&root);
        a1.SetParent(&a);
        a2.SetParent(&a);
        a2x.SetParent(&a2);
    }
};
}  // namespace

TEST_CASE("WP-00 HierarchyUtil: depth, comparisons, CompileID (HierarchyUtil.cs:22-125)") {
    Tree t;
    CHECK(HierarchyUtil::GetHierarchyDepth(&t.root) == 0);
    CHECK(HierarchyUtil::GetHierarchyDepth(&t.b) == 1);
    CHECK(HierarchyUtil::GetHierarchyDepth(&t.a2x) == 3);
    CHECK(HierarchyUtil::CompareHierarchyDepths(&t.root, &t.a2x) == -1);
    CHECK(HierarchyUtil::CompareHierarchyDepths(&t.a2x, &t.b) == 1);
    CHECK(HierarchyUtil::CompareHierarchyDepths(5, 5) == 0);
    CHECK(HierarchyUtil::isAncestor(&t.root, &t.a));
    CHECK(HierarchyUtil::isSameLevel(&t.a, &t.b));
    CHECK(HierarchyUtil::isDescendant(&t.a2x, &t.a));
    // the generic accessor forms (parentAccessor(o) == null ends the walk)
    const std::function<Transform*(Transform*)> acc = [](Transform* x) { return x->parent(); };
    CHECK(HierarchyUtil::GetHierarchyDepth<Transform*>(&t.a2x, acc) == 3);
    CHECK(HierarchyUtil::GetHierarchyDepth<Transform*>(&t.root, acc) == 0);
    CHECK(HierarchyUtil::isAncestor<Transform*>(&t.a, &t.a1, acc));
    CHECK(HierarchyUtil::isSameLevel<Transform*>(&t.a1, &t.a2, acc));
    CHECK(HierarchyUtil::isDescendant<Transform*>(&t.a2x, &t.b, acc));
    // CompileID: path from the named root, spaces removed
    CHECK(HierarchyUtil::CompileID(&t.a2x, "SpaceCenter") == "SpaceCenter/LaunchPad/CrewBuilding/Door");
    CHECK(HierarchyUtil::CompileID(&t.root, "SpaceCenter") == "SpaceCenter");
    // the root name is never found: logs once and returns the path from the scene root
    LogCapture lc;
    CHECK(HierarchyUtil::CompileID(&t.b, "Nowhere") == "SpaceCenter/VAB");
    CHECK(lc.errors.size() == 1);
}

TEST_CASE("WP-00 TransformExtension / GameObjectExtension: NestToParent, FindChild / FindParent, layers and tags") {
    Tree t;
    // FindChild: depth-first including the object itself
    CHECK(TransformExtension::FindChild(&t.root, "Door") == &t.a2x);
    CHECK(TransformExtension::FindChild(&t.root, "SpaceCenter") == &t.root);
    CHECK(TransformExtension::FindChild(&t.root, "Nope") == nullptr);
    CHECK(TransformExtension::FindChild(&t.a, "VAB") == nullptr);
    t.a2.activeSelf = false;  // an inactive ancestor hides its subtree from the active search
    CHECK(TransformExtension::FindChild(&t.root, "Door", true) == nullptr);
    CHECK(TransformExtension::FindChild(&t.root, "Door", false) == &t.a2x);
    CHECK(TransformExtension::FindParent(&t.a2x, "SpaceCenter", false) == &t.root);
    CHECK(TransformExtension::FindParent(&t.a2x, "Launch Pad", true) == &t.a);
    CHECK(TransformExtension::FindParent(&t.a2x, "Crew Building", true) == nullptr);  // inactive
    CHECK(TransformExtension::FindParent(&t.a2x, "Nope", false) == nullptr);
    CHECK_THROWS_AS(TransformExtension::FindChild(nullptr, "x"), NullReferenceException);
    // GameObject.GetChild: direct children first, then each subtree
    CHECK(GameObjectExtension::GetChild(&t.root, "Door") == &t.a2x);
    CHECK(GameObjectExtension::GetChild(&t.root, "VAB") == &t.b);
    CHECK(GameObjectExtension::GetChild(&t.root, "SpaceCenter") == nullptr);  // not its own child
    // NestToParent
    t.a1.localPosition = {1.0f, 2.0f, 3.0f};
    t.a1.localScale = {2.0f, 2.0f, 2.0f};
    TransformExtension::NestToParent(&t.a1, &t.b);
    CHECK(t.a1.parent() == &t.b);
    CHECK(t.a.childCount() == 1);
    CHECK(t.b.childCount() == 1);
    CHECK(t.a1.localPosition.x == 0.0f);
    CHECK(t.a1.localScale.x == 2.0f);
    TransformExtension::NestToParent(&t.a1, &t.a1);  // onto itself: no-op
    CHECK(t.a1.parent() == &t.b);
    TransformExtension::NestToParent(&t.a1, &t.root, true);  // resetParent: detach, unit scale, then nest
    CHECK(t.a1.parent() == &t.root);
    CHECK(t.a1.localScale.x == 1.0f);
    TransformExtension::NestToParent(&t.a1, nullptr);
    CHECK(t.a1.parent() == nullptr);
    // layers
    TransformExtension::SetLayerRecursive(&t.root, 9);
    CHECK(t.a2x.layer == 9);
    CHECK(t.b.layer == 9);
    t.a2.layer = 3;
    GameObjectExtension::SetLayerRecursive(&t.root, 12, 1 << 9);  // everything is on layer 9 (ignored) except a2 (3)
    CHECK(t.root.layer == 9);
    CHECK(t.a2.layer == 12);
    CHECK(t.a2x.layer == 9);
    GameObjectExtension::SetLayerRecursive(&t.root, 5, true, 0, [](const Transform& x) { return x.name == "Door"; });
    CHECK(t.a2x.layer == 9);  // translucent: skipped
    CHECK(t.b.layer == 5);
    // tags
    t.b.tag = "Keep";
    GameObjectExtension::SetTagsRecursive(&t.root, "Foo", {"Keep", "Other"});
    CHECK(t.b.tag == "Keep");
    CHECK(t.a2x.tag == "Foo");
}
