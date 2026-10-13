// WP-32 unit tests, part 1: the KerbalEVA / RetractableLadder state-machine tables against the tables PARSED from the C# source
// (tools/wp32/eva_tables.py -> tests/data/wp32_reference.json): states, callback names in registration order, per-state event lists, event targets,
// update modes, timer expressions. Also the structure of the animation manager and the module registration.
#include "eva_fixture.hpp"

using namespace ksp;
using namespace ksp::eva32;

namespace {

std::string Mode(int m) { return std::to_string(m); }

std::vector<std::string> Names(const nlohmann::json& j) {
    std::vector<std::string> v;
    for (const auto& x : j) v.push_back(x.get<std::string>());
    return v;
}

}  // namespace

// KerbalEVA.cs:3268-4352 SetupFSM
TEST_CASE("WP-32 KerbalEVA.SetupFSM: states, callback lists and event lists equal the tables parsed from the C#") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef()["kerbal_eva_fsm"];
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    const auto& states = e.fsm->GetStates();
    REQUIRE(states.size() == ref["states"].size());
    const auto& wiring = e.StateWiring();
    for (size_t i = 0; i < states.size(); i++) {
        const auto& js = ref["states"][i];
        const std::string name = js["name"].get<std::string>();
        INFO("state " << name << " (KerbalEVA.cs:" << js["line"] << ")");
        CHECK(states[i]->name == name);
        CHECK(static_cast<int>(states[i]->updateMode) == js["updateMode"].get<int>());
        auto it = wiring.find(name);
        REQUIRE(it != wiring.end());
        const EvaStateWiring& sw = it->second;
        CHECK(sw.onEnter == (js["onEnter"].is_null() ? "" : js["onEnter"].get<std::string>()));
        CHECK(sw.onLeave == (js["onLeave"].is_null() ? "" : js["onLeave"].get<std::string>()));
        CHECK(sw.onUpdate == Names(js["onUpdate"]));
        CHECK(sw.onFixedUpdate == Names(js["onFixedUpdate"]));
        CHECK(sw.onLateUpdate == Names(js["onLateUpdate"]));
        std::vector<std::string> got;
        for (const KFSMEvent* ev : states[i]->StateEvents()) got.push_back(ev->name);
        CHECK(got == Names(js["events"]));
    }
    const auto& evw = e.EventWiring();
    REQUIRE(evw.size() == ref["events"].size());
    for (const auto& je : ref["events"]) {
        const std::string name = je["name"].get<std::string>();
        INFO("event " << name << " (KerbalEVA.cs:" << je["line"] << ")");
        auto it = evw.find(name);
        REQUIRE(it != evw.end());
        CHECK(it->second.gotoState == (je["goto"].is_null() ? "" : je["goto"].get<std::string>()));
        CHECK(it->second.updateMode == je["updateMode"].get<int>());
        CHECK(it->second.timed == je["timed"].get<bool>());
    }
}

// the timer durations of the timed events: literal expressions of the C# (the animation-length ones are checked against the synthetic clip lengths)
TEST_CASE("WP-32 KerbalEVA timed events: TimerDuration equals the parsed literal / the animation clip length") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef()["kerbal_eva_fsm"];
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    int checked = 0;
    for (const auto& je : ref["events"]) {
        if (!je["timed"].get<bool>()) continue;
        const std::string name = je["name"].get<std::string>();
        KFSMTimedEvent* found = nullptr;
        for (KFSMState* st : e.fsm->GetStates())
            for (KFSMEvent* ev : st->StateEvents())
                if (ev->name == name) found = dynamic_cast<KFSMTimedEvent*>(ev);
        REQUIRE(found != nullptr);
        const std::string expr = je["duration"].get<std::string>();
        INFO("event " << name << " duration " << expr);
        double want = 0.0;
        if (expr.find("Animations.flagPlant.State.length") != std::string::npos) want = e.Animations().flagPlant.State()->length;
        else if (expr.find("Animations.weld.State.length") != std::string::npos) want = e.Animations().weld.State()->length;
        else want = std::stod(expr);
        CHECK(found->TimerDuration == want);
        checked++;
    }
    CHECK(checked == 7);
}

// RetractableLadder.cs:126-244: the ladder FSM
TEST_CASE("WP-32 RetractableLadder.OnStart: states and event lists equal the tables parsed from the C#") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef()["ladder_fsm"];
    EvaWorld w;
    // a part that carries a RetractableLadder module (the crew-hatch ladders of the Mk1-2 pod etc.): take the real part
    std::unique_ptr<Part> part = w.g.New("telescopicLadder");  // the Telescopic Ladder (Squad/Parts/Utility/ladderTelescopic)
    RetractableLadder* lad = dynamic_cast<RetractableLadder*>(part->FindModuleOfClass("RetractableLadder"));
    REQUIRE(lad != nullptr);
    lad->rig.clip.length = 2.0f;  // SYNTHETIC clip length (the prefab animation)
    lad->OnStart(StartState::Landed);
    REQUIRE(lad->ladderFSM != nullptr);
    const auto& states = lad->ladderFSM->GetStates();
    REQUIRE(states.size() == ref["states"].size());
    for (size_t i = 0; i < states.size(); i++) {
        const auto& js = ref["states"][i];
        CHECK(states[i]->name == js["name"].get<std::string>());
        std::vector<std::string> got;
        for (const KFSMEvent* ev : states[i]->StateEvents()) got.push_back(ev->name);
        CHECK(got == Names(js["events"]));
    }
    for (const auto& je : ref["events"]) {
        KFSMEvent* found = nullptr;
        for (KFSMState* st : states)
            for (KFSMEvent* ev : st->StateEvents())
                if (ev->name == je["name"].get<std::string>()) found = ev;
        REQUIRE(found != nullptr);
        CHECK(found->GoToStateOnEvent->name == je["goto"].get<std::string>());
        CHECK(static_cast<int>(found->updateMode) == je["updateMode"].get<int>());
        CHECK((dynamic_cast<KFSMTimedEvent*>(found) != nullptr) == je["timed"].get<bool>());
    }
    CHECK(lad->ladderFSM->CurrentState()->name == "Retracted");
}

// KerbalAnimationManager.cs: the GetAllAnimations order is the field declaration order of the C# class (66 states)
TEST_CASE("WP-32 KerbalAnimationManager: GetAllAnimations count and jp_ variant rule") {
    if (!GameAvailable()) return;
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    CHECK(e.Animations().GetAllAnimations().size() == 66);
    KerbalAnimationState& a = e.Animations().idle;  // index 0 -> hasJpVariant (synthetic table)
    REQUIRE(a.hasJpVariant);
    CHECK(a.GetAnimationString() == "clip0");
    e.set_JetpackDeployed(true);
    CHECK(a.GetAnimationString() == "jp_clip0");
    KerbalAnimationState& b = e.Animations().suspendedIdle;  // index 1: no jp variant
    REQUIRE_FALSE(b.hasJpVariant);
    CHECK(b.GetAnimationString() == "clip1");
}
