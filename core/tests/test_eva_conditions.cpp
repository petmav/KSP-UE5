// WP-32 unit tests, part 3: the event conditions and OnEvent target decisions of KerbalEVA.SetupFSM (KerbalEVA.cs:3767-4351) over input grids,
// against the Python predicates of tools/wp32/eva_ref_conditions.py (translated from the C# lambdas).
#include "eva_fixture.hpp"

using namespace ksp;
using namespace ksp::eva32;

namespace {

struct World {
    nlohmann::json w;
    std::vector<EvaCollider> colliders;
    Part otherPart;
};

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

// the world of the Python reference applied to a live kerbal (inputs only; the delegates run unmodified)
void Apply(EvaWorld& ew, EvaKerbal& k, World& wd, const nlohmann::json& ref) {
    KerbalEVA& e = *k.eva;
    const nlohmann::json& w = wd.w;
    e.tgtRpos = V3(w["tgt"]);
    e.deltaHdg = F(w["dh"]);
    e.lastDeltaHdg = F(w["ldh"]);
    ew.Kerbin().GeeASL = w["gee"].get<double>();
    e.CharacterFrameMode = w["cf"].get<bool>();
    k.vessel->Landed = w["landed"].get<bool>();
    k.vessel->Splashed = w["splashed"].get<bool>();
    k.vessel->packed = w["packed"].get<bool>();
    k.vessel->state = w["active"].get<bool>() ? Vessel::State::ACTIVE : Vessel::State::INACTIVE;
    k.vessel->heightFromTerrain = F(w["hft"]);
    k.vessel->heightFromSurface = F(w["hfs"]);
    Time::time = F(w["t"]);
    e.fsm->CurrentState()->TimeAtStateEnter = 0.0;
    Time::frameCount = w["frames"].get<int>();
    e.fsm->CurrentState()->FrameCountAtStateEnter = 0;
    e.set_lastBoundStep(F(w["lbs"]));
    e.set_JetpackDeployed(w["jet"].get<bool>());
    e.partPlacementMode = w["pm"].get<bool>();
    const nlohmann::json keys = w["keys"];
    EvaEnvironment::key = [keys](EvaKey kk, KeyQuery q) {
        static const char* names[] = {"Forward", "Back", "Left", "Right", "YawLeft", "YawRight", "PackForward", "PackBack", "PackLeft", "PackRight", "PackUp", "PackDown",
                                      "Jump", "Run", "TogglePack", "Use", "Board", "Orient", "Lights", "Helmet", "ChuteDeploy", "ToggleMovementMode"};
        const std::string n = names[static_cast<int>(kk)];
        if (!keys.contains(n)) return false;
        return keys[n][q == KeyQuery::Held ? "held" : (q == KeyQuery::Down ? "down" : "up")].get<bool>();
    };
    k.part->parent = w["noparent"].get<bool>() ? nullptr : &wd.otherPart;
    // ladder
    wd.colliders.assign(4, EvaCollider());
    e.currentLadderTriggers.clear();
    for (int i = 0; i < w["ntrig"].get<int>(); i++) e.currentLadderTriggers.push_back(&wd.colliders[static_cast<size_t>(i) + 1]);
    if (w["cl"].get<bool>()) {
        EvaCollider& c = wd.colliders[0];
        c.pose.rotation = Rot(ref, w["clrot"].get<int>());
        c.activeInHierarchy = w["lad_active"].get<bool>();
        c.enabled = w["lad_enabled"].get<bool>();
        e.currentLadder = &c;
    } else {
        e.currentLadder = nullptr;
    }
    e.invLadderAxis = w["inv"].get<bool>();
    e.ladderTgtRPos = V3(w["ltr"]);
    GameSettings::EVA_LADDER_CHECK_END = w["chk"].get<bool>();
    e.canClimb = w["ccl"].get<bool>();
    e.canDescend = w["cdn"].get<bool>();
    e.currentSpd = F(w["spd"]);
    // clamber
    if (w["cpath"].get<bool>()) e.clamberPath = std::make_unique<ClamberPath>(Vector3f(), Vector3f(), Vector3f(), Vector3f(), Vector3f(), 0.0f, 0.0f);
    else e.clamberPath.reset();
    e.Animations().clamber.State()->normalizedTime = F(w["cnt"]);
    e.Animations().clamber.end = F(w["cend"]);
    e.Animations().clamber.State()->length = F(w["clen"]);
    e.InConstructionMode = w["inconstr"].get<bool>();
    if (w["weldfx"].get<bool>()) {
        e.weldFX = std::make_unique<EvaWeldFx>();
        e.weldFX->meshEnabled = w["weldmesh"].get<bool>();
    } else {
        e.weldFX.reset();
    }
}

}  // namespace


TEST_CASE("WP-32 KerbalEVA event conditions: 2,880 worlds equal the Python predicates") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld ew;
    EvaKerbal k(ew, true);
    KerbalEVA& e = *k.eva;
    // the Python predicates read the clamber flag through the keys of the event; Ladder Grab Start / Boarding Part / Semi-Deploy post messages (checked elsewhere)
    World wd;
    int n = 0;
    std::map<std::string, int> perEvent;
    for (const auto& c : ref["conditions"]) {
        wd.w = c["world"];
        Apply(ew, k, wd, ref);
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
