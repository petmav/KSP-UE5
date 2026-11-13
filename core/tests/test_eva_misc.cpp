// WP-32 unit tests, part 4: helmet safety, visor animation, jump / bound impulses, the low-g collider curve, clamber path points, camera bob, part
// placement mode, flag rules, CrewTransfer part rules and the ladder bookkeeping (trigger ordering with the runtime's List.Sort, ladder switching,
// ladder movement) against tools/wp32/eva_ref_misc.py.
#include "eva_fixture.hpp"

#include "ksp/celestial/flight_globals.hpp"
#include "ksp/career/game_variables.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/misc_util.hpp"
#include "ksp/localization/localizer.hpp"

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

// ===================================================================================================================================================
// wpfix9: FlightEVA.cs hatch obstruction, spawnEVA decisions and bookkeeping (:81-:277), from the C#. The engine answers (rays, overlap spheres,
// airlock colliders, fairing) are the FlightEvaEnvironment seams, scripted here; the decisions and the arguments they are asked with are the ported code.
namespace {

struct HatchScript {
    struct RayCall { Vector3f origin, dir; float distance; int mask; };
    std::vector<RayCall> rays;
    struct SphereCall { Vector3f center; float radius; int mask; };
    std::vector<SphereCall> spheres;
    std::vector<HatchHit> rayAnswers;                 // consumed in order, then "no hit"
    std::vector<HatchOverlap> overlapAnswer;
    HatchScript() {
        FlightEvaEnvironment::Reset();
        Install();
    }
    void Install() {
        FlightEvaEnvironment::raycast = [this](const Vector3f& o, const Vector3f& d, float dist, int mask) {
            rays.push_back({o, d, dist, mask});
            if (rays.size() <= rayAnswers.size()) return rayAnswers[rays.size() - 1];
            return HatchHit{};
        };
        FlightEvaEnvironment::overlapSphere = [this](const Vector3f& c, float r, int mask) {
            spheres.push_back({c, r, mask});
            return overlapAnswer;
        };
    }
    ~HatchScript() { FlightEvaEnvironment::Reset(); }
};

struct ShieldStub : IAirstreamShield {
    Part* part;
    explicit ShieldStub(Part* p) : part(p) {}
    bool ClosedAndLocked() override { return true; }
    Vessel* GetVessel() override { return nullptr; }
    Part* GetPart() override { return part; }
};

}  // namespace

TEST_CASE("wpfix9 FlightEVA.HatchIsObstructed / HatchIsObstructedMore: ray, inward ray and overlap sphere arguments and decisions (FlightEVA.cs:81-120)") {
    HatchScript s;
    Part p, other, frozen;
    p.partTransform.position = Vector3f(0.0f, 0.0f, 0.0f);
    p.fields.hatchObstructionCheckOutwardDistance = 1.5f;
    p.fields.hatchObstructionCheckInwardOffset = 0.25f;
    p.fields.hatchObstructionCheckInwardDistance = 2.0f;
    p.fields.hatchObstructionCheckSphereRadius = 0.5f;
    p.fields.hatchObstructionCheckSphereOffset = 0.125f;
    frozen.fields.frozen = true;
    PartTransform airlock;
    airlock.position = Vector3f(0.0f, 2.0f, 0.0f);  // 2 m above the part: outward = +Y
    // HatchIsObstructed: the ray starts 0.5 m inside the airlock, along the outward direction, for the outward distance + 0.5
    CHECK_FALSE(FlightEVA::HatchIsObstructed(p, airlock));
    REQUIRE(s.rays.size() == 1);
    CHECK(s.rays[0].origin == Vector3f(0.0f, 1.5f, 0.0f));
    CHECK(s.rays[0].dir == Vector3f(0.0f, 1.0f, 0.0f));
    CHECK(s.rays[0].distance == 2.0f);
    CHECK(s.rays[0].mask != 0);
    s.rays.clear();
    s.rayAnswers = {HatchHit{true, nullptr}};  // anything hit (the C# does not look at what)
    CHECK(FlightEVA::HatchIsObstructed(p, airlock));
    // no raycast seam: nothing obstructs
    FlightEvaEnvironment::raycast = nullptr;
    CHECK_FALSE(FlightEVA::HatchIsObstructed(p, airlock));
    s.rays.clear();
    s.Install();

    // HatchIsObstructedMore: the outward ray first (obstructed -> true without the other two)
    s.rayAnswers = {HatchHit{true, nullptr}};
    CHECK(FlightEVA::HatchIsObstructedMore(p, airlock));
    CHECK(s.rays.size() == 1);
    CHECK(s.spheres.empty());
    // outward clear; the inward ray starts hatchObstructionCheckInwardOffset back along the inward direction, length InwardDistance
    s.rays.clear();
    s.rayAnswers = {HatchHit{}, HatchHit{true, &other}};
    CHECK(FlightEVA::HatchIsObstructedMore(p, airlock));  // another loose part inside
    REQUIRE(s.rays.size() == 2);
    CHECK(s.rays[1].dir == Vector3f(0.0f, -1.0f, 0.0f));
    CHECK(s.rays[1].origin == Vector3f(0.0f, 2.25f, 0.0f));  // airlock - inward * 0.25
    CHECK(s.rays[1].distance == 2.0f);
    // ... but the part itself, a frozen part, or a hit without a part do not count
    for (Part* hit : {&p, &frozen, static_cast<Part*>(nullptr)}) {
        s.rays.clear();
        s.spheres.clear();
        s.rayAnswers = {HatchHit{}, HatchHit{true, hit}};
        CHECK_FALSE(FlightEVA::HatchIsObstructedMore(p, airlock));
        REQUIRE(s.spheres.size() == 1);
        // the sphere: centre = airlock + inward * SphereOffset, radius SphereRadius, the fixed overlap mask
        CHECK(s.spheres[0].center == Vector3f(0.0f, 2.0f - 0.125f, 0.0f));
        CHECK(s.spheres[0].radius == 0.5f);
        CHECK(s.spheres[0].mask == 67665921);
    }
    // overlap sphere: a solid collider of another loose part blocks; triggers, the part itself, frozen parts and colliders without a part do not
    s.rayAnswers.clear();
    s.overlapAnswer = {HatchOverlap{&p, false}, HatchOverlap{&frozen, false}, HatchOverlap{&other, true}, HatchOverlap{nullptr, false}};
    CHECK_FALSE(FlightEVA::HatchIsObstructedMore(p, airlock));
    s.overlapAnswer.push_back(HatchOverlap{&other, false});
    CHECK(FlightEVA::HatchIsObstructedMore(p, airlock));
}

TEST_CASE("wpfix9 FlightEVA.hatchInsideFairing: only a shielded part whose first shield is a capped fairing without an interstage") {
    HatchScript s;
    Part p, shieldPart;
    ShieldStub shield(&shieldPart);
    int asked = 0;
    FlightEvaEnvironment::fairingCappedNoInterstage = [&](Part& x) {
        ++asked;
        CHECK(&x == &shieldPart);
        return true;
    };
    CHECK_FALSE(FlightEVA::hatchInsideFairing(p));  // not shielded, no shields
    p.airstreamShields.push_back(&shield);
    CHECK_FALSE(FlightEVA::hatchInsideFairing(p));  // shields but not ShieldedFromAirstream
    CHECK(asked == 0);
    p.set_ShieldedFromAirstream(true);
    CHECK(FlightEVA::hatchInsideFairing(p));
    CHECK(asked == 1);
    FlightEvaEnvironment::fairingCappedNoInterstage = [&](Part&) { return false; };  // an interstage or an open fairing
    CHECK_FALSE(FlightEVA::hatchInsideFairing(p));
    p.airstreamShields.clear();
    CHECK_FALSE(FlightEVA::hatchInsideFairing(p));
}

namespace {
struct SpawnProbe {
    AvailablePart info;
    Part part;
    std::shared_ptr<ProtoCrewMember> crew = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Test Kerman");
    PartTransform main, alt;
    SpawnProbe() {
        info.name = "testPod";
        part.partInfo = &info;
        part.protoModuleCrew.push_back(crew);
        main.position = Vector3f(0.0f, 2.0f, 0.0f);
        alt.position = Vector3f(2.0f, 0.0f, 0.0f);
        FlightEvaEnvironment::messages.clear();
    }
    // the posted text is Localizer.Format(tag) (the tag itself when no dictionary is loaded)
    bool Said(const char* tag) const {
        const std::string want = KSP::Localization::Localizer::Format(tag);
        for (const std::string& m : FlightEvaEnvironment::messages)
            if (m == want) return true;
        return false;
    }
};
}  // namespace

TEST_CASE("wpfix9 FlightEVA.spawnEVA: the refusals (crew not in the part, no airlock, obstructed hatch, fairing, all hatches blocked) and the abort event (FlightEVA.cs:135-195)") {
    HatchScript s;
    FlightEVA fe;
    SpawnProbe t;
    // the kerbal is not in that part
    auto stranger = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Stranger Kerman");
    CHECK(fe.spawnEVA(stranger, &t.part, &t.main) == nullptr);
    CHECK(FlightEvaEnvironment::messages.empty());
    CHECK(s.rays.empty());  // refused before any hatch test
    // no airlock transform: message 111934
    CHECK(fe.spawnEVA(t.crew, &t.part, nullptr) == nullptr);
    CHECK(FlightEvaEnvironment::messages.size() == 1);
    CHECK(t.Said("#autoLOC_111934"));
    // obstructed hatch (not tryAllHatches): message 111978, no spawn
    FlightEvaEnvironment::messages.clear();
    s.rayAnswers = {HatchHit{true, nullptr}};
    CHECK(fe.spawnEVA(t.crew, &t.part, &t.main) == nullptr);
    CHECK(FlightEvaEnvironment::messages.size() == 1);
    CHECK(t.Said("#autoLOC_111978"));
    // (without a loaded dictionary every tag formats to the same text: the branch is told apart by the ray calls and the message count)
    // inside a capped fairing: the same message
    FlightEvaEnvironment::messages.clear();
    s.rayAnswers.clear();
    Part shieldPart;
    ShieldStub shield(&shieldPart);
    t.part.airstreamShields.push_back(&shield);
    t.part.set_ShieldedFromAirstream(true);
    FlightEvaEnvironment::fairingCappedNoInterstage = [](Part&) { return true; };
    CHECK(fe.spawnEVA(t.crew, &t.part, &t.main) == nullptr);
    CHECK(t.Said("#autoLOC_111978"));
    // tryAllHatches: the given hatch is blocked (outward ray), the second airlock collider is clear but the part sits inside a fairing: message 111971
    FlightEvaEnvironment::messages.clear();
    FlightEvaEnvironment::airlockTransforms = [&](Part&) { return std::vector<const PartTransform*>{&t.main, &t.alt}; };
    s.rays.clear();
    s.rayAnswers = {HatchHit{true, nullptr}};
    CHECK(fe.spawnEVA(t.crew, &t.part, &t.main, true) == nullptr);
    CHECK(t.Said("#autoLOC_111971"));
    // tryAllHatches with every other hatch blocked as well (the given hatch itself is skipped without a ray): message 111971
    FlightEvaEnvironment::messages.clear();
    t.part.airstreamShields.clear();
    t.part.set_ShieldedFromAirstream(false);
    s.rays.clear();
    s.rayAnswers = {HatchHit{true, nullptr}, HatchHit{true, nullptr}};  // HatchIsObstructedMore's first ray, then the alt hatch's ray
    CHECK(fe.spawnEVA(t.crew, &t.part, &t.main, true) == nullptr);
    CHECK(t.Said("#autoLOC_111971"));
    CHECK(s.rays.size() == 2);  // `fromAirlock == collider.transform` is skipped: no ray for it
    // onAttemptEva: a listener that sets overrideEVA vetoes the spawn
    struct Veto {
        FlightEVA* fe;
        int fired = 0;
        void On(ProtoCrewMember*, Part*, UnityEngine::Transform*) { ++fired; fe->overrideEVA = true; }
    } veto{&fe};
    GameEvents::onAttemptEva.Add(&veto, &Veto::On);
    s.rays.clear();
    s.rayAnswers.clear();
    CHECK(fe.spawnEVA(t.crew, &t.part, &t.main) == nullptr);
    CHECK(veto.fired == 1);
    CHECK(fe.overrideEVA);
    CHECK(t.part.protoModuleCrew.size() == 1);  // the kerbal is still in the part
    GameEvents::onAttemptEva.Remove(&veto, &Veto::On);
}

// FlightEVA.onGoForEVA (:197-:277) over two real kerbalEVA parts: the source part (holds the crew member) and the spawned EVA kerbal
// (instantiateKerbalEVA hands out the second one). The velocity the new kerbal gets is that of the SOURCE PART's own airlock point
// (`fromPart.airlock.position`, :220), also when another hatch was chosen.
TEST_CASE("wpfix9 FlightEVA.spawnEVA: the new kerbal's placement, velocity at the part's airlock, crew move, ids, events and the switch to the EVA vessel") {
    if (!GameAvailable()) return;
    EvaWorld w;
    EvaKerbal src(w);
    EvaKerbal dst(w);
    dst.part->protoModuleCrew.clear();  // the spawned kerbal part starts empty (the Spawn delegate hands out the prefab instance)
    dst.part->fields.CrewCapacity = 1;  // SYNTHETIC: the kerbalEVA prefab's serialized CrewCapacity (the mirror cfg has no such key)
    FlightRegistry::ActiveVessel = src.vessel.get();
    ScienceEnvironment::scheduler = &w.sched;
    FlightEvaEnvironment::Reset();
    src.part->partTransform.position = Vector3f(10.0f, 20.0f, 30.0f);
    src.part->fields.missionID = 4321u;
    src.part->fields.launchID = 17;
    src.part->fields.flagURL = "Squad/Flags/test";
    PartTransform hatch, partAirlock;
    hatch.position = Vector3f(10.0f, 22.0f, 30.0f);
    hatch.rotation = Quaternionf(0.0f, 0.0f, 0.0f, 1.0f);
    partAirlock.position = Vector3f(11.0f, 20.0f, 30.0f);  // the part's own airlock is not the hatch that is used
    Vector3f askedPoint(-1.0f, -1.0f, -1.0f);
    int bailed = 0, neutral = 0;
    FlightEvaEnvironment::instantiateKerbalEVA = [&](ProtoCrewMember&) { return dst.eva; };
    FlightEvaEnvironment::addVessel = [&](KerbalEVA&) { return dst.vessel.get(); };
    FlightEvaEnvironment::newGuid = [] { return Guid::Empty(); };
    FlightEvaEnvironment::partAirlock = [&](Part&) { return &partAirlock; };
    FlightEvaEnvironment::pointVelocity = [&](Part&, const Vector3f& p) {
        askedPoint = p;
        return Vector3f(1.0f, 2.0f, 3.0f);
    };
    FlightEvaEnvironment::partAngularVelocity = [](Part&) { return Vector3f(0.0f, 0.5f, 0.0f); };
    FlightEvaEnvironment::uniqueFlightID = [] { return 777u; };
    FlightEvaEnvironment::kerbalBailedOut = [&](ProtoCrewMember&) { ++bailed; };
    FlightEvaEnvironment::setNeutralControls = [&] { ++neutral; };
    FlightEvaEnvironment::mainBodyAt = [&](const Vector3f&) { return &w.Kerbin(); };
    struct Listen {
        int onEva = 0, transferred = 0;
        Part* from = nullptr;
        Part* to = nullptr;
        void E(const GameEvents::FromToAction<Part*, Part*>& a) { ++onEva; from = a.from; to = a.to; }
        void T(const GameEvents::HostedFromToAction<ProtoCrewMember*, Part*>&) { ++transferred; }
    } listen;
    GameEvents::onCrewOnEva.Add(&listen, &Listen::E);
    GameEvents::onCrewTransferred.Add(&listen, &Listen::T);

    FlightEVA fe;
    KerbalEVA* spawned = fe.spawnEVA(src.crew, src.part.get(), &hatch);
    REQUIRE(spawned == dst.eva);
    // placement: rotation of the hatch, position = hatch + rotation * (back * airlockStandoff)
    CHECK(dst.part->partTransform.rotation == hatch.rotation);
    CHECK(dst.part->partTransform.position == Vector3f(10.0f, 22.0f, 30.0f) + hatch.rotation * (Vector3f::back() * 0.18f));
    // the velocity comes from the part's airlock point, not from the hatch used (the bug this test found: it used the hatch)
    CHECK(askedPoint == partAirlock.position);
    CHECK(dst.engine.velocity == Vector3f(1.0f, 2.0f, 3.0f));
    CHECK(dst.engine.angularVelocity == Vector3f(0.0f, 0.5f, 0.0f));
    CHECK(FlightEvaEnvironment::collisionEnhancerBypass);
    // crew move, EVA vessel identity and the ids inherited from the source part
    CHECK(src.part->protoModuleCrew.empty());
    REQUIRE(dst.part->protoModuleCrew.size() == 1);
    CHECK(dst.part->protoModuleCrew[0] == src.crew);
    CHECK(dst.vessel->vesselType == VesselType::EVA);
    CHECK(dst.vessel->vesselName == "Valentina Kerman");
    CHECK(dst.part->fields.flightID == 777u);
    CHECK(dst.part->fields.missionID == 4321u);
    CHECK(dst.part->fields.launchID == 17);
    CHECK(dst.part->fields.flagURL == std::optional<std::string>("Squad/Flags/test"));
    CHECK(bailed == 1);
    CHECK(listen.onEva == 1);
    CHECK(listen.transferred == 1);
    CHECK(listen.from == src.part.get());
    CHECK(listen.to == dst.part.get());
    // the pending state is cleared: spawning the same kerbal again fails the crew check (it left the part)
    CHECK(fe.spawnEVA(src.crew, src.part.get(), &hatch) == nullptr);
    // SwitchToEVAVesselWhenReady: waits for eva.Ready, then makes the EVA vessel the active one and neutralises the controls
    REQUIRE(fe.lastSwitch != nullptr);
    dst.eva->Ready = false;
    Pump(w.sched, 3);
    CHECK_FALSE(fe.lastSwitch->Finished());
    CHECK(neutral == 0);
    dst.eva->Ready = true;
    Pump(w.sched, 3);
    CHECK(fe.lastSwitch->Finished());
    CHECK(neutral == 1);  // ForceSetActiveVessel(eva.vessel) succeeded: FlightInputHandler.SetNeutralControls
    GameEvents::onCrewOnEva.Remove(&listen, &Listen::E);
    GameEvents::onCrewTransferred.Remove(&listen, &Listen::T);
    FlightEVA::Spawn = nullptr;  // the next FlightEVA installs the default again
}

// ===================================================================================================================================================
// wpfix9: CrewTransfer.cs flows (:24-:115) and Kerbal.cs start / die / situation change (:143-:329), from the C#. Parts and vessel are bare objects;
// the engine-side calls (Vessel.DespawnCrew / SpawnCrew, the portrait gallery) are the seams.
namespace {

// one frame of dt seconds with the dynamic delayed-call pass (WaitForSeconds wakes in it)
void Frame(CoroutineScheduler& s, float dt) {
    s.AdvanceFrame(dt);
    s.RunDelayedDynamicFrameRate();
}

struct TransferWorld {
    Vessel vessel;
    AvailablePart info;
    Part src, dest, full, none;
    std::shared_ptr<ProtoCrewMember> crew = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Transfer Kerman");
    std::shared_ptr<ProtoCrewMember> other = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Other Kerman");
    CoroutineScheduler sched;
    std::vector<std::string> log;
    TransferWorld() {
        vessel.loaded = true;  // RebuildCrewList reads the parts of a loaded vessel
        info.title = "Test Pod";
        for (Part* p : {&src, &dest, &full, &none}) {
            p->partInfo = &info;
            p->vessel = &vessel;
            p->fields.crewTransferAvailable = true;
            vessel.parts.push_back(p);
        }
        src.fields.CrewCapacity = 1;
        dest.fields.CrewCapacity = 2;
        full.fields.CrewCapacity = 1;
        none.fields.CrewCapacity = 0;
        src.protoModuleCrew.push_back(crew);
        full.protoModuleCrew.push_back(other);
        FlightRegistry::ActiveVessel = &vessel;
        ScienceEnvironment::scheduler = &sched;
        CrewTransfer::despawnCrew = [this](Vessel& v) { log.push_back(&v == &vessel ? "despawn" : "despawn?"); };
        CrewTransfer::spawnCrew = [this](Vessel& v) { log.push_back(&v == &vessel ? "spawn" : "spawn?"); };
    }
    ~TransferWorld() {
        PartItemTransfer::DismissActive();
        CrewTransfer::despawnCrew = nullptr;
        CrewTransfer::spawnCrew = nullptr;
        ScienceEnvironment::scheduler = nullptr;
        FlightRegistry::ActiveVessel = nullptr;
        for (Part* p : {&src, &dest, &full, &none}) p->vessel = nullptr;
        vessel.parts.clear();
    }
};

}  // namespace

TEST_CASE("wpfix9 CrewTransfer: part lists, selection veto, MoveCrewTo, the delayed completion and the interruptions (CrewTransfer.cs:24-115)") {
    if (!GameAvailable()) return;
    EvaWorld world;  // the environment a Vessel needs
    inv33::LoadLocalizer();  // PartItemTransfer.Setup formats a message with arguments
    struct LocalizerGuard {
        ~LocalizerGuard() { KSP::Localization::Localizer::Shutdown(); }  // other test files assume no Localizer instance
    } localizerGuard;
    TransferWorld t;
    std::vector<std::pair<PartItemTransfer::DismissAction, Part*>> dismissed;
    auto onDismiss = [&](PartItemTransfer::DismissAction a, Part* p) { dismissed.emplace_back(a, p); };
    struct Listen {
        int created = 0, selected = 0, transferred = 0;
        bool allow = true;
        Part* destSeen = nullptr;
        std::vector<Part*>* valid = nullptr;
        std::vector<Part*>* semi = nullptr;
        void Created(const events::GameEvents::HostedFromToAction<Part*, std::vector<Part*>*>& a) { ++created; valid = a.from; semi = a.to; }
        void Selected(CrewTransfer::CrewTransferData* d) { ++selected; destSeen = d->destPart; d->canTransfer = allow; }
        void Transferred(const events::GameEvents::HostedFromToAction<ProtoCrewMember*, Part*>&) { ++transferred; }
    } l;
    events::GameEvents::onCrewTransferPartListCreated.Add(&l, &Listen::Created);
    events::GameEvents::onCrewTransferSelected.Add(&l, &Listen::Selected);
    events::GameEvents::onCrewTransferred.Add(&l, &Listen::Transferred);

    std::unique_ptr<CrewTransfer> ct = CrewTransfer::Create(t.src, t.crew, onDismiss);
    // the lists: a part with free seats is valid, a full one is semi-valid, one without seats is neither; the source itself is skipped
    REQUIRE(l.created == 1);
    CHECK(l.valid->size() == 1);
    CHECK((*l.valid)[0] == &t.dest);
    CHECK(l.semi->size() == 1);
    CHECK((*l.semi)[0] == &t.full);
    // a part without seats is in neither list
    CT probe;  // IsValidPart / IsSemiValidPart are protected
    CHECK_FALSE(probe.V(t.none));
    CHECK_FALSE(probe.S(t.none));
    // the selection event can veto the transfer
    l.allow = false;
    ct->OnPartSelect(&t.dest);
    CHECK(l.selected == 1);
    CHECK(l.destSeen == &t.dest);
    CHECK(t.src.protoModuleCrew.size() == 1);
    CHECK(t.dest.protoModuleCrew.empty());
    CHECK(l.transferred == 0);
    CHECK(t.log.empty());
    // allowed: the crew member moves, the message names the kerbal and the destination part, the event fires, the crew is despawned
    l.allow = true;
    ct->OnPartSelect(&t.dest);
    CHECK(l.selected == 2);
    CHECK(t.src.protoModuleCrew.empty());
    REQUIRE(t.dest.protoModuleCrew.size() == 1);
    CHECK(t.dest.protoModuleCrew[0] == t.crew);
    CHECK(l.transferred == 1);
    CHECK(ct->tgtPart == &t.dest);
    CHECK(t.log == std::vector<std::string>{"despawn"});
    REQUIRE_FALSE(ct->screenMessages.empty());
    CHECK(ct->screenMessages.back() == KSP::Localization::Localizer::Format("#autoLOC_111636", std::vector<std::string>{"Transfer Kerman", "Test Pod"}));
    // the completion is CallbackUtil.DelayedCallback(1, ...): ONE FRAME (the int overload counts frames), not one second. Nothing happens before it
    CHECK(dismissed.empty());
    CHECK(t.log.size() == 1);
    Frame(t.sched, 0.02f);
    CHECK(t.log == std::vector<std::string>{"despawn", "spawn"});
    REQUIRE(dismissed.size() == 1);
    CHECK(dismissed[0].first == PartItemTransfer::DismissAction::ItemMoved);
    CHECK(dismissed[0].second == &t.dest);
    CHECK(PartItemTransfer::Instance == nullptr);  // dismissed: the singleton is free again
    events::GameEvents::onCrewTransferPartListCreated.Remove(&l, &Listen::Created);
    events::GameEvents::onCrewTransferSelected.Remove(&l, &Listen::Selected);
    events::GameEvents::onCrewTransferred.Remove(&l, &Listen::Transferred);

    // interruptions: the kerbal going inactive, but not another kerbal and not the kerbal coming back
    dismissed.clear();
    ct = CrewTransfer::Create(t.dest, t.crew, onDismiss);
    REQUIRE(PartItemTransfer::Instance == ct.get());
    ct->onChangeInactive(t.other.get(), false, true);
    ct->onChangeInactive(t.crew.get(), true, false);
    CHECK(dismissed.empty());
    ct->onChangeInactive(t.crew.get(), false, true);
    REQUIRE(dismissed.size() == 1);
    CHECK(dismissed[0].first == PartItemTransfer::DismissAction::Interrupted);
    CHECK(dismissed[0].second == nullptr);
    // a crew transfer of the same kerbal to a part that is not a KerbalEVA does not interrupt (CrewTransfer.cs:82-88)
    dismissed.clear();
    ct = CrewTransfer::Create(t.dest, t.crew, onDismiss);
    ct->onGoForEva(events::GameEvents::HostedFromToAction<ProtoCrewMember*, Part*>(t.crew.get(), &t.dest, &t.full));
    CHECK(dismissed.empty());
}

TEST_CASE("wpfix9 Kerbal.Start / die / onVesselSitChange (Kerbal.cs:143-183, :306-323)") {
    CoroutineScheduler sched;
    ScienceEnvironment::scheduler = &sched;
    KerbalEnvironment::Reset();
    int registered = 0, unregistered = 0, deaths = 0;
    KerbalEnvironment::portraitGallery = [&](Kerbal&, bool reg) { (reg ? registered : unregistered)++; };
    KerbalEnvironment::portraitCrewDie = [&](Kerbal&) { ++deaths; };
    auto pcm = std::make_shared<ProtoCrewMember>(ProtoCrewMember::KerbalType::Crew, "Start Kerman");
    {
        Kerbal k;
        k.crewMemberName = "Start Kerman";
        k.protoCrewMember = pcm;
        k.textureTargets = 3;
        pcm->veteran = true;
        k.Start();
        CHECK(k.running);
        CHECK(k.state == KerbalStates::NO_SIGNAL);  // until the static overlay time has passed
        CHECK(registered == 1);
        CHECK(k.portraitRegistered);
        // SetupKerbalModel: every texture target takes the veteran texture of a veteran
        REQUIRE(k.textureTargetUsesVeteran.size() == 3);
        for (bool v : k.textureTargetUsesVeteran) CHECK(v);
        CHECK(k.capturedInitial);
        // randomizeOnStartup: InitState(name.GetHashCode_Net35()), then Range(0.1f, 0.15f) and Range(0, 20) in that order
        UnityRandom::Global().InitState(StringExtensions::GetHashCode_Net35("Start Kerman"));
        const float wantInterval = UnityRandom::Global().Range(0.1f, 0.15f);
        const float wantSeed = static_cast<float>(UnityRandom::Global().Range(0, 20));
        CHECK(k.updateInterval == wantInterval);
        CHECK(k.noiseSeed == wantSeed);
        CHECK(k.updateInterval >= 0.1f);
        CHECK(k.updateInterval <= 0.15f);
        // staticOverlayDuration (3 s): then ALIVE
        Frame(sched, 1.0f);
        CHECK(k.state == KerbalStates::NO_SIGNAL);
        for (int i = 0; i < 4; ++i) Frame(sched, 1.0f);
        CHECK(k.state == KerbalStates::ALIVE);
        // die(): the portrait is told, the kerbal and its roster entry are dead
        k.die();
        CHECK(deaths == 1);
        CHECK(k.state == KerbalStates::DEAD);
        CHECK(k.rosterStatus == ProtoCrewMember::RosterStatus::Dead);
        // onVesselSitChange: only the active vessel's situation changes matter; with GameVariables missing the C# throws on the active one
        Vessel other, active;
        FlightRegistry::ActiveVessel = &active;
        GameVariables* savedGv = GameVariables::Instance;
        GameVariables::Instance = nullptr;
        CHECK_NOTHROW(k.onVesselSitChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations>(&other, Vessel_Situations::LANDED, Vessel_Situations::FLYING)));
        CHECK_THROWS_AS(k.onVesselSitChange(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations>(&active, Vessel_Situations::LANDED, Vessel_Situations::FLYING)),
                        NullReferenceException);
        GameVariables::Instance = savedGv;
        FlightRegistry::ActiveVessel = nullptr;
    }
    CHECK(unregistered == 1);  // OnDestroy: KerbalPortraitGallery.UnregisterActiveCrew
    // a kerbal that is already dead when it starts stays DEAD until the overlay timer, which (as in the C#) then sets ALIVE if it still runs
    {
        Kerbal k;
        k.crewMemberName = "Dead Kerman";
        k.protoCrewMember = pcm;
        k.rosterStatus = ProtoCrewMember::RosterStatus::Dead;
        k.randomizeOnStartup = false;
        k.noiseSeed = 5.0f;
        k.Start();
        CHECK(k.state == KerbalStates::DEAD);
        CHECK(k.noiseSeed == 5.0f);  // randomizeOnStartup off: the seed is kept
        for (int i = 0; i < 4; ++i) Frame(sched, 1.0f);
        CHECK(k.state == KerbalStates::ALIVE);
    }
    KerbalEnvironment::Reset();
    ScienceEnvironment::scheduler = nullptr;
}
