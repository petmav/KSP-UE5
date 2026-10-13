// WP-32 unit tests, part 3: the event conditions and OnEvent target decisions of KerbalEVA.SetupFSM (KerbalEVA.cs:3767-4351) over input grids,
// against the Python predicates of tools/wp32/eva_ref_conditions.py (translated from the C# lambdas).
#include "eva_fixture.hpp"

using namespace ksp;
using namespace ksp::eva32;

namespace {

KFSMEvent* FindEvent(KerbalEVA& e, const std::string& name) {
    for (KFSMState* st : e.fsm->GetStates())
        for (KFSMEvent* ev : st->StateEvents())
            if (ev->name == name) return ev;
    return nullptr;
}

KFSMState* FindState(KerbalEVA& e, const std::string& name) {
    for (KFSMState* st : e.fsm->GetStates())
        if (st->name == name) return st;
    return nullptr;
}

}  // namespace

TEST_CASE("WP-32 KerbalEVA event conditions: 2,880 worlds equal the Python predicates") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld ew;
    EvaKerbal k(ew, true);
    KerbalEVA& e = *k.eva;
    // the Python predicates read the clamber flag through the keys of the event; Ladder Grab Start / Boarding Part / Semi-Deploy post messages (checked elsewhere)
    WorldState wd;
    int n = 0;
    std::map<std::string, int> perEvent;
    for (const auto& c : ref["conditions"]) {
        ApplyWorld(ew, k, wd, ref, c["world"], true);
        const std::string name = c["event"].get<std::string>();
        KFSMEvent* ev = FindEvent(e, name);
        REQUIRE_MESSAGE(ev != nullptr, name);
        const bool got = ev->OnCheckCondition(e.fsm->CurrentState());
        INFO("event " << name << " case " << n);
        CHECK(got == c["expect"].get<bool>());
        perEvent[name]++;
        n++;
    }
    CHECK(n == 2880);
    CHECK(perEvent.size() == 36);
    EvaEnvironment::key = nullptr;
    GameSettings::EVA_LADDER_CHECK_END = false;
}

TEST_CASE("WP-32 KerbalEVA OnEvent target decisions (Landing, Feet Wet, Feet Dry, Recover End, Start / End Run, Low G bound land) equal the Python reference") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld ew;
    EvaKerbal k(ew, true);
    KerbalEVA& e = *k.eva;
    EvaEnvironment::universalTime = [] { return 1000.0; };
    int n = 0;
    for (const auto& c : ref["decisions"]) {
        k.part->partTransform.rotation = Rot(ref, c["rot"].get<int>());
        e.fUp = V3(c["fup"]);
        e.lastCollisionDirection = V3(c["lcd"]);
        e.lastCollisionNormal = V3(c["lcn"]);
        k.vessel->rb_velocity = V3(c["vrb"]);
        k.engine.velocity = V3(c["rbv"]);
        e.lastCollisionTime = c["lct"].get<double>();
        k.vessel->Landed = c["landed"].get<bool>();
        k.vessel->Splashed = c["splashed"].get<bool>();
        ew.Kerbin().GeeASL = c["gee"].get<double>();
        e.CharacterFrameMode = c["cf"].get<bool>();
        const std::string what = "decision " + std::to_string(n++);
        auto run = [&](const char* name) {
            KFSMEvent* ev = FindEvent(e, name);
            REQUIRE_MESSAGE(ev != nullptr, name);
            ev->OnEvent();
            return ev->GoToStateOnEvent != nullptr ? ev->GoToStateOnEvent->name : std::string("<none>");
        };
        INFO(what);
        CHECK(run("Landing") == c["landStart"].get<std::string>());
        CHECK(run("Feet Wet") == c["feetWet"].get<std::string>());
        CHECK(run("Feet Dry") == c["feetDry"].get<std::string>());
        CHECK(run("Recover End") == c["recoverComplete"].get<std::string>());
        KFSMEvent* sr = FindEvent(e, "Start Run");
        sr->GoToStateOnEvent = FindState(e, "Walk (FPS)");
        CHECK(run("Start Run") == c["startRun"].get<std::string>());
        CHECK(run("End Run") == c["endRun"].get<std::string>());
        e.boundLeftFoot = false;
        CHECK(run("Low G bound land") == c["boundLand"].get<std::string>());
        CHECK(e.boundLeftFoot);
    }
    CHECK(n == 300);
    EvaEnvironment::universalTime = nullptr;
}

// Scripted scenarios: the live FSM (callbacks, OnEvent bodies, timers, per-state event lists) against a second FSM runtime over the parsed tables
// (tools/wp32/eva_ref_scenarios.py). SYNTHETIC world: the inputs are scripted (identity rotation, tgtFwd = forward, fUp = up keep the callbacks inert).
TEST_CASE("WP-32 scenarios: walk, run, jump, swim, ladder, flag, ragdoll, low gravity, construction, jetpack traces equal the reference FSM runtime") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld ew;
    EvaEnvironment::upAxis = [] { return Vector3f(0.0f, 1.0f, 0.0f); };
    int total = 0;
    for (const auto& sc : ref["scenarios"]) {
        const std::string sname = sc["name"].get<std::string>();
        EvaKerbal k(ew, true);
        KerbalEVA& e = *k.eva;
        WorldState ws;
        Time::time = 100.0f;
        Time::frameCount = 1000;
        e.HasJetpack = true;
        e.heightFromSurfaceHitNormal = Vector3f(0.0f, 1.0f, 0.0f);
        e.tgtFwd = Vector3f(0.0f, 0.0f, 1.0f);
        ApplyWorld(ew, k, ws, ref, sc["base"], false);
        KFSMState* start = FindState(e, sc["start"].get<std::string>());
        REQUIRE_MESSAGE(start != nullptr, sname);
        e.fsm->StartFSM(start);
        size_t i = 0;
        for (const auto& st : sc["steps"]) {
            ApplyWorld(ew, k, ws, ref, st["set"], false);
            Time::time = f32store(fx(Time::time) + fx(F(st["dt"])));
            Time::frameCount++;
            if (!st["event"].is_null()) {
                KFSMEvent* ev = FindEvent(e, st["event"].get<std::string>());
                REQUIRE_MESSAGE(ev != nullptr, st["event"].get<std::string>());
                e.fsm->RunEvent(ev);
            } else {
                switch (st["mode"].get<int>()) {
                    case 1: e.fsm->UpdateFSM(); break;
                    case 2: e.fsm->FixedUpdateFSM(); break;
                    default: e.fsm->LateUpdateFSM(); break;
                }
            }
            INFO("scenario " << sname << " step " << i);
            CHECK(e.fsm->currentStateName == sc["trace"][i][0].get<std::string>());
            CHECK(e.fsm->lastEventName == sc["trace"][i][1].get<std::string>());
            i++;
            total++;
        }
    }
    CHECK(total == 83);
    EvaEnvironment::Reset();
    GameSettings::EVA_LADDER_CHECK_END = false;
}
