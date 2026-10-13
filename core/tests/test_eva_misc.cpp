// WP-32 unit tests, part 4: helmet safety, visor animation, jump / bound impulses, the low-g collider curve, clamber path points, camera bob, part
// placement mode, flag rules, CrewTransfer part rules and the ladder bookkeeping (trigger ordering with the runtime's List.Sort, ladder switching,
// ladder movement) against tools/wp32/eva_ref_misc.py.
#include "eva_fixture.hpp"

#include "ksp/events/game_events.hpp"

using namespace ksp;
using namespace ksp::eva32;

namespace {

struct VisorCount {
    int lowered = 0, raised = 0;
    void L(KerbalEVA*) { lowered++; }
    void R(KerbalEVA*) { raised++; }
    VisorCount() {
        GameEvents::OnVisorLowered.Add(this, &VisorCount::L);
        GameEvents::OnVisorRaised.Add(this, &VisorCount::R);
    }
    ~VisorCount() {
        GameEvents::OnVisorLowered.Remove(this, &VisorCount::L);
        GameEvents::OnVisorRaised.Remove(this, &VisorCount::R);
    }
};

struct CT : CrewTransfer {
    bool V(Part& p) { return IsValidPart(p); }
    bool S(Part& p) { return IsSemiValidPart(p); }
};

}  // namespace

// KerbalEVA.cs:7158
TEST_CASE("WP-32 CheckHelmetOffSafe: 600 pressure / temperature / body worlds give the Python verdict and reason") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    int n = 0;
    for (const auto& c : ref["helmet"]) {
        CelestialBody& b = w.Kerbin();
        b.atmosphere = c["atm"].get<bool>();
        b.atmosphereDepth = c["depth"].get<double>();
        b.atmosphereContainsOxygen = c["oxy"].get<bool>();
        k.vessel->altitude = c["alt"].get<double>();
        k.vessel->Splashed = c["splashed"].get<bool>();
        k.part->fields.staticPressureAtm = c["press"].get<double>();
        k.part->fields.skinTemperature = c["skin"].get<double>();
        k.part->fields.temperature = c["temp"].get<double>();
        const bool got = e.CheckHelmetOffSafe(c["margins"].get<bool>(), false);
        INFO("helmet case " << n);
        CHECK(got == c["ok"].get<bool>());
        CHECK(e.HelmetUnsafeReason() == c["why"].get<std::string>());
        // the three public wrappers: CanSafelyRemoveHelmet = with margins, WillDieWithoutHelmet = !without margins
        if (c["margins"].get<bool>()) CHECK(e.CanSafelyRemoveHelmet() == c["ok"].get<bool>());
        else CHECK(e.WillDieWithoutHelmet() == !c["ok"].get<bool>());
        n++;
    }
    CHECK(n == 600);
}

// KerbalEVA.cs:6164
TEST_CASE("WP-32 UpdateVisorPosition: the lerp series, the Lowered / Raised switch, events and the crew flag equal the Python reference") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    VisorCount cnt;
    e.set_isHelmetEnabled(true);
    e.Ready = false;
    for (const auto& c : ref["visor"]) {
        const bool lowering = c["lowering"].get<bool>();
        e.visorState = lowering ? VisorStates::Lowering : VisorStates::Raising;
        e.visorTextureOffsetY = lowering ? 0.0f : 0.5f;
        Time::deltaTime = F(c["dt"]);
        const int l0 = cnt.lowered, r0 = cnt.raised;
        size_t i = 0;
        for (; i < c["series"].size(); i++) {
            e.UpdateVisorPosition();
            CheckBits(e.visorTextureOffsetY, c["series"][i], "visor step " + std::to_string(i));
            if (e.visorState == VisorStates::Lowered || e.visorState == VisorStates::Raised) break;
        }
        CHECK(i + 1 == c["series"].size());
        CHECK((e.visorState == VisorStates::Lowered ? "Lowered" : "Raised") == c["final"].get<std::string>());
        CHECK(e.isVisorEnabled() == c["enabled"].get<bool>());
        CHECK(k.crew->hasVisorDown == c["enabled"].get<bool>());
        CHECK(cnt.lowered - l0 == (lowering ? 1 : 0));
        CHECK(cnt.raised - r0 == (lowering ? 0 : 1));
        // finished states do nothing
        const float before = e.visorTextureOffsetY;
        e.UpdateVisorPosition();
        CHECK(e.visorTextureOffsetY == before);
    }
    Time::deltaTime = 0.02f;
}

// KerbalEVA.cs:3866 (jump launch) / :3805 (low-g bound)
TEST_CASE("WP-32 jump launch and low-g bound impulses equal the Python reference (0 ULP)") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    // StartEVA: maxJumpForce *= massMultiplier
    e.set_maxJumpForce(f32store(fx(e.maxJumpForce()) * fx(e.massMultiplier())));
    KFSMEvent* jump = nullptr;
    KFSMEvent* bound = nullptr;
    for (KFSMState* st : e.fsm->GetStates())
        for (KFSMEvent* ev : st->StateEvents()) {
            if (ev->name == "Jump Launch") jump = ev;
            if (ev->name == "Low G bound") bound = ev;
        }
    REQUIRE(jump != nullptr);
    REQUIRE(bound != nullptr);
    int n = 0;
    for (const auto& c : ref["impulse"]["jump"]) {
        k.engine.impulses.clear();
        k.part->partTransform.rotation = Rot(ref, c["rot"].get<int>());
        k.part->fields.mass = F(c["mass"]);
        e.tgtSpeed = F(c["tgtSpeed"]);
        jump->OnEvent();
        REQUIRE(k.engine.impulses.size() == 1);
        CheckV3(k.engine.impulses[0], c["impulse"], "jump impulse " + std::to_string(n++));
    }
    CHECK(n == 72);
    n = 0;
    for (const auto& c : ref["impulse"]["bound"]) {
        k.engine.impulses.clear();
        k.part->partTransform.rotation = Rot(ref, c["rot"].get<int>());
        k.part->fields.mass = F(c["mass"]);
        e.tgtRpos = V3(c["tgt"]);
        k.engine.velocity = V3(c["velIn"]);
        e.deltaHdg = 33.0f;
        k.engine.angularVelocity = Vector3f(5.0f, 5.0f, 5.0f);
        bound->OnEvent();
        const std::string what = "bound impulse " + std::to_string(n++);
        CheckBits(e.boundForce(), c["boundForce"], what + " boundForce");
        REQUIRE(k.engine.impulses.size() == 1);
        CheckV3(k.engine.impulses[0], c["impulse"], what);
        CHECK(e.deltaHdg == 0.0f);
        CHECK(k.engine.angularVelocity == Vector3f::zero());
    }
    CHECK(n == 72);
}

// KerbalEVA.cs:4820
TEST_CASE("WP-32 UpdateLowGBodyColliders: the piecewise collider height curve equals the Python reference") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    EvaAnimState st;
    e.tmpBoundState = &st;
    int n = 0;
    for (const auto& c : ref["lowg"]) {
        st.normalizedTime = F(c["nt"]);
        e.boundColliderModifierCounter = static_cast<float>(c["counter"].get<double>());
        k.engine.bodyColliderCenterY = 123.0f;
        e.UpdateLowGBodyColliders();
        const int idx_ = n++;
        INFO("lowg case " << idx_);
        if (c["ran"].get<bool>()) {
            CheckBits(e.normalizedBoundTime, c["nbt"], "normalizedBoundTime");
            CheckBits(e.colliderHeight, c["height"], "colliderHeight");
            CheckBits(k.engine.bodyColliderCenterY, c["height"], "body collider centre");
        } else {
            CHECK(k.engine.bodyColliderCenterY == 123.0f);
        }
    }
    e.tmpBoundState = nullptr;
    // no bound animation yet: nothing happens whatever the counter
    e.boundColliderModifierCounter = 5.0f;
    k.engine.bodyColliderCenterY = 123.0f;
    e.UpdateLowGBodyColliders();
    CHECK(k.engine.bodyColliderCenterY == 123.0f);
}

TEST_CASE("WP-32 ClamberPath constructor and Kerbal camera bob equal the Python reference (0 ULP)") {
    nlohmann::json ref = LoadRef();
    for (const auto& c : ref["clamber"]) {
        ClamberPath p(V3(c["e"]), V3(c["s"]), V3(c["p1"]), V3(c["p2"]), V3(c["p3"]), F(c["h"]), F(c["so"]));
        CheckV3(p.p1, c["q1"], "clamber p1");
        CheckV3(p.p2, c["q2"], "clamber p2");
        CheckV3(p.p3, c["q3"], "clamber p3");
        CheckBits(p.clamberHeight, c["h"], "clamberHeight");
        CheckBits(p.standOffDistance, c["so"], "standOffDistance");
        CheckV3(p.edgeFaceNormal, c["e"], "edgeFaceNormal");
        CheckV3(p.srfNormal, c["s"], "srfNormal");
    }
    for (const auto& c : ref["seatcam"]) {
        const Vector3f v = Kerbal::SeatCamBobEuler(F(c["px"]), F(c["py"]), F(c["pz"]), c["acc"].get<double>(), c["ang"].get<double>(), F(c["bob"]));
        CheckBits(v.x, c["x"], "bob x");
        CheckBits(v.y, c["y"], "bob y");
        CheckBits(v.z, c["z"], "bob z");
    }
}

TEST_CASE("WP-32 SetPartPlacementMode, CanPlantFlag, CrewTransfer part rules equal the Python reference") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    int n = 0;
    for (const auto& c : ref["rules"]["placement"]) {
        e.isRagdoll = c["rag"].get<bool>();
        k.vessel->state = c["uc"].get<bool>() ? Vessel::State::ACTIVE : Vessel::State::INACTIVE;
        k.vessel->packed = false;
        e.onLadder = c["lad"].get<bool>();
        k.vessel->Landed = c["landed"].get<bool>();
        e.partPlacementMode = c["pmIn"].get<bool>();
        const bool got = e.SetPartPlacementMode(c["mode"].get<bool>(), nullptr);
        const int idx_ = n++;
        INFO("placement case " << idx_);
        CHECK(got == c["result"].get<bool>());
        CHECK(e.PartPlacementMode() == c["pm"].get<bool>());
    }
    e.onLadder = false;
    k.vessel->Landed = true;
    k.vessel->state = Vessel::State::ACTIVE;
    n = 0;
    for (const auto& c : ref["rules"]["flag"]) {
        k.vessel->state = c["active"].get<bool>() ? Vessel::State::ACTIVE : Vessel::State::INACTIVE;
        k.part->fields.GroundContact = c["ground"].get<bool>();
        e.set__flags(c["items"].get<int>());
        e.isRagdoll = c["rag"].get<bool>();
        e.InConstructionMode = c["constr"].get<bool>();
        const int idx_ = n++;
        INFO("flag case " << idx_);
        CHECK(e.CanPlantFlag() == c["result"].get<bool>());
    }
    e.isRagdoll = false;
    e.InConstructionMode = false;
    n = 0;
    CT ct;
    for (const auto& c : ref["rules"]["transfer"]) {
        Part p;
        p.fields.CrewCapacity = c["cap"].get<int>();
        p.fields.crewTransferAvailable = c["avail"].get<bool>();
        for (int i = 0; i < c["n"].get<int>(); i++) p.protoModuleCrew.push_back(k.crew);
        const int idx_ = n++;
        INFO("transfer case " << idx_);
        CHECK(ct.S(p) == c["semi"].get<bool>());
        CHECK(ct.V(p) == c["valid"].get<bool>());
    }
}

// ladder bookkeeping --------------------------------------------------------------------------------------------------------------------------------
namespace {
std::vector<int> OrderOf(const std::vector<EvaCollider*>& lst, const std::vector<EvaCollider>& all) {
    std::vector<int> o;
    for (EvaCollider* c : lst) o.push_back(static_cast<int>(c - all.data()));
    return o;
}
std::vector<int> Ints(const nlohmann::json& j) {
    std::vector<int> v;
    for (const auto& x : j) v.push_back(x.get<int>());
    return v;
}
}  // namespace

TEST_CASE("WP-32 ladder triggers: OnTriggerStay distance ordering (List.Sort, equal distances) equals the Python reference") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    int n = 0;
    for (const auto& c : ref["ladder"]["distance"]) {
        k.engine.ladderPivot = V3(c["pivot"]);
        const size_t cnt = c["pts"].size();
        std::vector<EvaCollider> cols(cnt);
        e.currentLadderTriggers.clear();
        for (size_t i = 0; i < cnt; i++) {
            cols[i].tag = "Ladder";
            cols[i].boundsClosestPoint = V3(c["pts"][i]);
            e.OnTriggerStay(&cols[i]);
            e.OnTriggerStay(&cols[i]);  // already listed: nothing changes
        }
        const int idx_ = n++;
        INFO("distance case " << idx_);
        CHECK(OrderOf(e.currentLadderTriggers, cols) == Ints(c["order"]));
    }
    CHECK(n == 120);
    // OnTriggerExit removes and re-sorts; the airlock trigger sets / clears the boarding part
    std::vector<EvaCollider> cols(3);
    for (auto& c : cols) c.tag = "Ladder";
    e.currentLadderTriggers.clear();
    for (auto& c : cols) e.OnTriggerStay(&c);
    e.OnTriggerExit(&cols[1]);
    CHECK(e.currentLadderTriggers.size() == 2);
    Part airlockPart;
    EvaCollider air;
    air.tag = "Airlock";
    air.part = &airlockPart;
    e.OnTriggerStay(&air);
    CHECK(e.currentAirlockPart == &airlockPart);
    CHECK(e.currentAirlockTrigger == &air);
    e.OnTriggerExit(&air);
    CHECK(e.currentAirlockPart == nullptr);
    e.currentLadderTriggers.clear();
}

TEST_CASE("WP-32 UpdateCurrentLadder / UpdateCurrentLadderIdle: alignment ordering, ladder switching, invLadderAxis, ladderTransition equal the Python reference") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    int n = 0;
    for (const auto& c : ref["ladder"]["alignment"]) {
        const size_t cnt = c["rots"].size();
        std::vector<EvaCollider> cols(cnt);
        e.currentLadderTriggers.clear();
        for (size_t i = 0; i < cnt; i++) {
            cols[i].pose.rotation = Rot(ref, c["rots"][i].get<int>());
            e.currentLadderTriggers.push_back(&cols[i]);
        }
        e.currentLadder = &cols[static_cast<size_t>(c["curIdx"].get<int>())];
        e.secondaryLadder = nullptr;
        e.ladderTgtRPos = V3(c["ltr"]);
        e.tgtSpeed = F(c["tgtSpeed"]);
        e.invLadderAxis = c["invIn"].get<bool>();
        e.ladderTransition = c["transIn"].get<bool>();
        const bool lh = c["leftHeld"].get<bool>(), rh = c["rightHeld"].get<bool>(), lu = c["leftUp"].get<bool>(), ru = c["rightUp"].get<bool>();
        EvaEnvironment::key = [=](EvaKey kk, KeyQuery q) {
            if (kk == EvaKey::Left) return q == KeyQuery::Held ? lh : (q == KeyQuery::Up ? lu : false);
            if (kk == EvaKey::Right) return q == KeyQuery::Held ? rh : (q == KeyQuery::Up ? ru : false);
            return false;
        };
        e.UpdateCurrentLadder();
        const int idx_ = n++;
        INFO("alignment case " << idx_);
        CHECK(OrderOf(e.currentLadderTriggers, cols) == Ints(c["order"]));
        CHECK(static_cast<int>(e.currentLadder - cols.data()) == c["cur"].get<int>());
        CHECK(e.invLadderAxis == c["inv"].get<bool>());
        CHECK(e.ladderTransition == c["trans"].get<bool>());
    }
    CHECK(n == 150);
    n = 0;
    for (const auto& c : ref["ladder"]["idle"]) {
        const size_t cnt = c["rots"].size();
        std::vector<EvaCollider> cols(cnt);
        e.currentLadderTriggers.clear();
        for (size_t i = 0; i < cnt; i++) {
            cols[i].pose.rotation = Rot(ref, c["rots"][i].get<int>());
            e.currentLadderTriggers.push_back(&cols[i]);
        }
        k.part->partTransform.rotation = Rot(ref, c["frot"].get<int>());
        e.currentLadder = &cols[0];
        e.secondaryLadder = nullptr;
        e.ladderTgtRPos = V3(c["ltr"]);
        e.invLadderAxis = c["invIn"].get<bool>();
        e.ladderTransition = c["transIn"].get<bool>();
        const bool lu = c["leftUp"].get<bool>(), ru = c["rightUp"].get<bool>();
        EvaEnvironment::key = [=](EvaKey kk, KeyQuery q) { return q == KeyQuery::Up && ((kk == EvaKey::Left && lu) || (kk == EvaKey::Right && ru)); };
        e.UpdateCurrentLadderIdle();
        const int idx_ = n++;
        INFO("idle case " << idx_);
        CHECK(OrderOf(e.currentLadderTriggers, cols) == Ints(c["order"]));
        CHECK(static_cast<int>(e.currentLadder - cols.data()) == c["cur"].get<int>());
        CHECK(e.invLadderAxis == c["inv"].get<bool>());
        CHECK(e.ladderTransition == c["trans"].get<bool>());
        CheckV3(e.ladderTgtRPos, c["ltrOut"], "ladderTgtRPos");
    }
    CHECK(n == 150);
    EvaEnvironment::key = nullptr;
    e.currentLadderTriggers.clear();
    e.currentLadder = nullptr;
}

TEST_CASE("WP-32 UpdateLadderMovement and correctLadderPosition equal the Python reference (0 ULP)") {
    if (!GameAvailable()) return;
    nlohmann::json ref = LoadRef();
    EvaWorld w;
    EvaKerbal k(w, true);
    KerbalEVA& e = *k.eva;
    int n = 0;
    for (const auto& c : ref["ladder"]["movement"]) {
        EvaCollider col;
        col.hasRigidbody = c["hasRb"].get<bool>();
        col.rbVelocity = V3(c["rbv"]);
        col.rbAngularVelocity = V3(c["rba"]);
        col.rbWorldCenterOfMass = V3(c["rbc"]);
        e.currentLadder = &col;
        k.engine.worldCenterOfMass = V3(c["com"]);
        e.ladderUp = V3(c["lup"]);
        e.fsm->CurrentState()->TimeAtStateEnter = c["enter"].get<double>();
        Time::time = F(c["time"]);
        e.lastTgtSpeed = F(c["lastTgt"]);
        e.tgtSpeed = F(c["tgt"]);
        e.canClimb = c["canClimb"].get<bool>();
        e.canDescend = c["canDesc"].get<bool>();
        GameSettings::EVA_LADDER_CHECK_END = c["chk"].get<bool>();
        e.UpdateLadderMovement();
        const std::string what = "ladder movement " + std::to_string(n++);
        CheckBits(e.currentSpd, c["spd"], what + " currentSpd");
        CheckV3(k.engine.velocity, c["vel"], what + " velocity");
    }
    CHECK(n == 120);
    n = 0;
    e.isLadderJointed = false;
    for (const auto& c : ref["ladder"]["position"]) {
        EvaCollider col;
        col.pose.rotation = Rot(ref, c["rot"].get<int>());
        col.pose.position = V3(c["lpos"]);
        col.hasRigidbody = c["hasRb"].get<bool>();
        col.rbVelocity = V3(c["rbv"]);
        col.rbAngularVelocity = V3(c["rba"]);
        col.rbWorldCenterOfMass = V3(c["rbc"]);
        e.currentLadder = &col;
        k.engine.worldCenterOfMass = V3(c["com"]);
        k.engine.ladderPivot = V3(c["pivot"]);
        k.part->partTransform.position = V3(c["pos"]);
        Time::deltaTime = F(c["dt"]);
        e.correctLadderPosition();
        const std::string what = "ladder position " + std::to_string(n++);
        CheckV3(k.engine.velocity, c["vtgt"], what + " velocity");
        CheckV3(e.Vtgt, c["vtgt"], what + " Vtgt");
        CheckV3(e.ladderPos, c["ladderPos"], what + " ladderPos");
        CheckV3(k.part->partTransform.position, c["newPos"], what + " position");
        CHECK(e.onLadder);
    }
    CHECK(n == 100);
    Time::deltaTime = 0.02f;
    GameSettings::EVA_LADDER_CHECK_END = false;
    e.currentLadder = nullptr;
}
