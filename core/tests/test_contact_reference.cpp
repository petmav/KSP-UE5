// WP-20e: the KSP statements of contact, landing and water against the independent Python reference
// (tools/wp20e/contact_reference.py -> tests/data/wp20e_reference.json, from the decompiled C#, Unity's managed math and Physics.cfg only):
//   Part.HandleCollision's decision and its event (Part.cs:3613),
//   collision message sequences through Part.OnCollisionEnter / Stay / Exit, LandedCollisionChecks, CheckOtherGroundContact, CheckCollision and
//   the Vessel's checkLanded / checkSplashed / SetLandedAt / updateSituation (Part.cs:3355-3611, 3941; Vessel.cs:1015-1061, 1185, 3133),
//   PartBuoyancy.Start / FixedUpdate (PartBuoyancy.cs), CollisionEnhancer.FixedUpdate (CollisionEnhancer.cs:47), Vessel.AddRBAnchor (Vessel.cs:1105).
// Every value is compared bit for bit (0 ULP) except the anchor's force angle (Vector3.Angle's Math.Acos: the platform libm, 1e-4 deg).
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "contact_fixture.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/localization/localizer.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using namespace ksp::physics;
using json = nlohmann::json;
using UnityEngine::ColliderRef;
using UnityEngine::Collision;
using UnityEngine::ContactPoint;
using wp20b::World;
using wp20e::AnchorSettings;

namespace {

const json& Ref() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp20e_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

double D(const json& j) {
    const uint64_t u = std::stoull(j.get<std::string>(), nullptr, 16);
    double d;
    std::memcpy(&d, &u, sizeof d);
    return d;
}
float Fl(const json& j) {
    const uint32_t u = static_cast<uint32_t>(std::stoul(j.get<std::string>(), nullptr, 16));
    float f;
    std::memcpy(&f, &u, sizeof f);
    return f;
}
Vector3d V3(const json& j) { return {D(j[0]), D(j[1]), D(j[2])}; }
Vector3f V3f(const json& j) { return {Fl(j[0]), Fl(j[1]), Fl(j[2])}; }
std::string HexF(float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof u);
    char b[16];
    std::snprintf(b, sizeof b, "%08x", u);
    return b;
}
std::string HexD(double d) {
    uint64_t u;
    std::memcpy(&u, &d, sizeof u);
    char b[24];
    std::snprintf(b, sizeof b, "%016llx", static_cast<unsigned long long>(u));
    return b;
}
bool SameF3(const Vector3f& a, const json& j) {
    return HexF(a.x) == j[0].get<std::string>() && HexF(a.y) == j[1].get<std::string>() && HexF(a.z) == j[2].get<std::string>();
}

// The reference's body: the fixture's Kerbin moved there (position, radius, gravitational parameter, ocean density)
void UseReferenceBody(World& w) {
    const json& b = Ref().at("body");
    CelestialBody& k = w.Kerbin();
    k.position = V3(b.at("position"));
    k.Radius = D(b.at("radius"));
    k.gMagnitudeAtCenter = D(b.at("mu"));
    k.oceanDensity = D(b.at("oceanDensity"));
    k.ocean = true;
}

// a vessel's orbit around Kerbin (mainBody), without moving it
void OrbitKerbin(World& w, Vessel& v) {
    auto o = std::make_shared<Orbit>();
    o->referenceBody = &w.Kerbin();
    v.orbitDriver->orbit = o;
}

const char* SituationName(Vessel_Situations s) {
    switch (s) {
        case Vessel_Situations::LANDED: return "LANDED";
        case Vessel_Situations::SPLASHED: return "SPLASHED";
        case Vessel_Situations::PRELAUNCH: return "PRELAUNCH";
        case Vessel_Situations::FLYING: return "FLYING";
        default: return "OTHER";
    }
}
Vessel_Situations SituationOf(const std::string& s) {
    if (s == "LANDED") return Vessel_Situations::LANDED;
    if (s == "SPLASHED") return Vessel_Situations::SPLASHED;
    if (s == "PRELAUNCH") return Vessel_Situations::PRELAUNCH;
    return Vessel_Situations::FLYING;
}

// the events of a replay, in firing order
struct Recorder {
    std::vector<std::string> out;
    std::function<std::string(Part*)> nameOf;
    std::function<std::string(EventReport*)> crashKind;
    float lastParam = 0.0f;      // the last crash / collision report: param
    std::string lastOther;       // the last collision report: other
    int hits = 0;                // OnCollisionEnhancerHit
    Recorder() {
        events::GameEvents::onPartExplodeGroundCollision.Add(this, &Recorder::OnGround);
        events::GameEvents::onCrash.Add(this, &Recorder::OnCrash);
        events::GameEvents::onCollision.Add(this, &Recorder::OnCollision);
        events::GameEvents::onVesselSituationChange.Add(this, &Recorder::OnSituation);
        events::GameEvents::OnCollisionEnhancerHit.Add(this, &Recorder::OnHit);
        PartContactEnvironment::explodeOverride = [this](Part& p, float) {
            out.push_back("explode:" + nameOf(&p));
            return true;
        };
    }
    ~Recorder() {
        events::GameEvents::onPartExplodeGroundCollision.Remove(this, &Recorder::OnGround);
        events::GameEvents::onCrash.Remove(this, &Recorder::OnCrash);
        events::GameEvents::onCollision.Remove(this, &Recorder::OnCollision);
        events::GameEvents::onVesselSituationChange.Remove(this, &Recorder::OnSituation);
        events::GameEvents::OnCollisionEnhancerHit.Remove(this, &Recorder::OnHit);
        PartContactEnvironment::explodeOverride = nullptr;
    }
    void OnGround(Part* p) { out.push_back("ground:" + nameOf(p)); }
    void OnCrash(EventReport* r) {
        lastParam = r->param;
        out.push_back(crashKind(r) + ":" + nameOf(r->origin));
    }
    void OnCollision(EventReport* r) {
        lastParam = r->param;
        lastOther = r->other;
        out.push_back("collision:" + nameOf(r->origin));
    }
    void OnHit(Part*, UnityEngine::RaycastHit) { ++hits; }
    bool Exploded() const {
        for (const std::string& x : out)
            if (x.rfind("explode", 0) == 0) return true;
        return false;
    }
    void OnSituation(events::GameEvents::HostedFromToAction<Vessel*, Vessel_Situations> a) {
        out.push_back(std::string("situation:") + SituationName(a.from) + ">" + SituationName(a.to));
    }
};

}  // namespace

// ===================================================================================================================== HandleCollision

TEST_CASE("WP-20e reference: Part.HandleCollision's crash-tolerance decision and event over 1,200 collisions incl. the float boundaries, 0 ULP") {
    if (!wp20b::GameAvailable()) return;
    World w;
    auto v = w.CraftHead("PhysProf-S", 1);
    auto o = w.CraftHead("PhysProf-S", 1);
    Part& p = *v->rootPart;
    Part& other = *o->rootPart;
    Recorder rec;
    rec.nameOf = [&](Part* x) { return x == &p ? std::string("P") : std::string("?"); };
    std::string lastName;
    rec.crashKind = [&](EventReport* r) {
        if (r->other == lastName) return std::string("crash_name");
        if (r->other == "Building") return std::string("crash_object");
        return std::string("crash_pq");
    };
    size_t n = 0, explodes = 0, mismatches = 0;
    for (const json& c : Ref().at("handle")) {
        p.fields.crashTolerance = Fl(c.at("tol"));
        PartContactEnvironment::noCrashDamage = c.at("noCrash").get<bool>();
        const std::string kind = c.at("other").get<std::string>();
        Collision col;
        col.relativeVelocity = V3f(c.at("v"));
        col.colliderEnabled = c.at("enabled").get<bool>();
        col.colliderActive = c.at("active").get<bool>();
        col.colliderIsTrigger = c.at("trigger").get<bool>();
        col.otherHasRigidbody = c.at("hasRb").get<bool>();
        col.otherRigidbodyMass = Fl(c.at("mass"));
        col.isPQ = kind == "pq";
        col.otherPart = kind == "part" ? &other : nullptr;
        col.name = "Collider" + std::to_string(n);
        lastName = col.name;
        p.crashObjectName = kind == "crashobject" ? "Building" : "";  // Part.crashObjectName (OnCollisionEnter sets it before CheckCollision)
        ContactPoint cp;
        cp.thisColliderPart = &p;
        cp.thisColliderIsWheelPiston = c.at("piston").get<bool>();
        col.contacts.push_back(cp);
        rec.out.clear();
        rec.lastParam = 0.0f;
        p.HandleCollision(col);
        const json& out = c.at("out");
        const bool exploded = std::find(rec.out.begin(), rec.out.end(), "explode:P") != rec.out.end();
        const bool ground = std::find(rec.out.begin(), rec.out.end(), "ground:P") != rec.out.end();
        std::string ev = "none";
        for (const std::string& s : rec.out)
            if (s.rfind("explode", 0) != 0 && s.rfind("ground", 0) != 0) ev = s.substr(0, s.find(':'));
        const bool ok = exploded == out.at("explode").get<bool>() && ground == out.at("ground").get<bool>() && ev == out.at("event").get<std::string>() &&
                        HexF(rec.lastParam) == out.at("param").get<std::string>();
        if (!ok && mismatches++ < 5)
            MESSAGE("case " << n << ": explode " << exploded << " ground " << ground << " event " << ev << " param " << HexF(rec.lastParam) << " vs " << out.dump());
        explodes += exploded;
        ++n;
    }
    PartContactEnvironment::noCrashDamage = false;
    MESSAGE(n << " collisions, " << explodes << " explode, " << mismatches << " mismatches");
    CHECK(n == 1200);
    CHECK(mismatches == 0);
}

// ===================================================================================================================== message sequences

TEST_CASE("WP-20e reference: collision message sequences (OnCollisionEnter / Stay / Exit -> landed, splashed, landedAt, situation), 300 sequences, exact") {
    if (!wp20b::GameAvailable()) return;
    World w;
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    Vessel::InstallPartHooks();
    auto V = w.CraftHead("PhysProf-S", 3);  // probeStackLarge -> Rockomax32 -> Mainsail
    REQUIRE(V->parts.size() == 3);
    Part* R = V->rootPart;
    REQUIRE(R->children.size() == 1);
    Part* T = R->children[0];
    REQUIRE(T->children.size() == 1);
    Part* M = T->children[0];
    T->fields.physicalSignificance = static_cast<int32_t>(Part_PhysicalSignificance::NONE);  // the physicsless middle part of the reference
    std::map<std::string, Part*> parts{{"R", R}, {"T", T}, {"M", M}};
    std::map<std::string, std::unique_ptr<Vessel>> others;
    std::map<std::string, Part*> otherPart;
    struct OtherDef {
        bool landed, splashed, eva;
        const char* landedAt;
        const char* display;
    };
    const std::map<std::string, OtherDef> defs{{"O1", {true, false, false, "Runway", "Runway"}},
                                               {"O2", {false, true, false, "", ""}},
                                               {"O3", {false, false, false, "", ""}},
                                               {"O4", {true, true, true, "KSC_Grounds", "KSC"}}};
    for (const auto& [name, d] : defs) {
        others[name] = w.CraftHead("PhysProf-S", 1);
        Vessel& o = *others[name];
        o.Landed = d.landed;
        o.Splashed = d.splashed;
        o.landedAt = d.landedAt;
        o.displaylandedAt = d.display;
        if (d.eva) o.vesselType = VesselType::EVA;
        otherPart[name] = o.rootPart;
    }
    struct StaticDef {
        uint64_t id;
        int layer;
        const char* tag;
        bool pq;
    };
    const std::map<std::string, StaticDef> statics{{"S1", {1, 15, "Untagged", false}},
                                                   {"S2", {2, 15, "LaunchPad", false}},
                                                   {"S3", {3, 15, "KSC_Grounds_Pond", false}},
                                                   {"S4", {4, 15, "Untagged", true}},
                                                   {"S5", {5, 0, "Untagged", false}}};
    auto colliderName = [&](uint64_t id) -> std::string {
        for (const auto& [n, s] : statics)
            if (s.id == id) return n;
        for (const auto& [n, p] : otherPart)
            if ((static_cast<uint64_t>(reinterpret_cast<uintptr_t>(p)) | 1ull) == id) return n;
        return "?";
    };
    Recorder rec;
    rec.nameOf = [&](Part* x) {
        for (const auto& [n, p] : parts)
            if (p == x) return n;
        return std::string("?");
    };
    rec.crashKind = [&](EventReport* r) { return statics.count(r->other) != 0 ? std::string("crash_name") : std::string("crash_pq"); };
    size_t seqs = 0, msgs = 0, mismatches = 0;
    for (const json& sq : Ref().at("sequences")) {
        // the vessel's start state
        const json& st = sq.at("start");
        V->loaded = true;
        V->packed = false;
        V->precalc.set_isEasingGravity(false);
        V->vesselSpawning = false;
        V->srfSpeed = 0.0;
        V->staticPressurekPa = 1.0;
        V->Landed = st.at("landed").get<bool>();
        V->Splashed = false;
        V->landedAt = st.at("landedAt").get<std::string>();
        V->displaylandedAt = V->landedAt;
        V->situation = SituationOf(st.at("situation").get<std::string>());
        V->updateSituation();  // lastSituation = situation (no event: unchanged)
        V->SetIgnoreCollisionsFrames(0);
        V->GroundContacts.clear();
        for (auto& [n, p] : parts) {
            p->fields.GroundContact = false;
            p->fields.PermanentGroundContact = false;
            p->fields.WaterContact = false;
            p->currentCollisions.clear();
            p->checkAfterResetCollisions = false;
            p->fields.crashTolerance = Fl(sq.at("tol").at(n));
        }
        R->fields.PermanentGroundContact = sq.at("permRoot").get<bool>();
        for (auto& [n, p] : otherPart) p->set_State(PartStates::IDLE);
        rec.out.clear();
        size_t k = 0;
        for (const json& ev : sq.at("events")) {
            const json& pre = ev.at("pre");
            if (pre.contains("ignore")) V->SetIgnoreCollisionsFrames(pre.at("ignore").get<int>());
            if (pre.contains("afterReset")) parts.at(pre.at("afterReset")[0].get<std::string>())->checkAfterResetCollisions = pre.at("afterReset")[1].get<bool>();
            if (pre.contains("otherState")) {
                const std::string s = pre.at("otherState")[1].get<std::string>();
                otherPart.at(pre.at("otherState")[0].get<std::string>())->set_State(s == "PLACEMENT" ? PartStates::PLACEMENT : s == "CARGO" ? PartStates::CARGO : PartStates::IDLE);
            }
            const std::string kind = ev.at("kind").get<std::string>();
            Part* recv = parts.at(ev.at("recv").get<std::string>());
            const std::string other = ev.at("other").get<std::string>();
            Collision c;
            c.relativeVelocity = V3f(ev.at("v"));
            ContactPoint cp;
            cp.thisColliderPart = parts.at(ev.at("this").get<std::string>());
            if (statics.count(other) != 0) {
                const StaticDef& s = statics.at(other);
                c.collider = ColliderRef{s.id, s.layer};
                c.layer = s.layer;
                c.tag = s.tag;
                c.name = other;
                c.isPQ = s.pq;
            } else {
                Part* op = otherPart.at(other);
                c.collider = ColliderRef{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(op)) | 1ull, 0};
                c.layer = 0;
                c.name = other;
                c.otherPart = op;
                c.otherHasRigidbody = true;
                c.otherRigidbodyMass = Fl(ev.at("mass"));
                cp.otherColliderPart = op;
            }
            c.contacts.push_back(cp);
            rec.out.clear();
            if (kind == "enter") recv->OnCollisionEnter(c);
            else if (kind == "stay") recv->OnCollisionStay(c);
            else recv->OnCollisionExit(c);
            // the snapshot
            const json& a = ev.at("after");
            std::vector<std::string> gc;
            for (Part* p : V->GroundContacts) gc.push_back(rec.nameOf(p));
            json cur = json::object();
            for (const char* rn : {"R", "M"}) {
                json l = json::array();
                for (const auto& [ref, count] : parts.at(rn)->currentCollisions) l.push_back(json::array({colliderName(ref.id), count}));
                cur[rn] = l;
            }
            std::vector<bool> ground{R->fields.GroundContact, T->fields.GroundContact, M->fields.GroundContact};
            // the reference names a collision event with the other collider (collision:R:O1); the C++ record has the receiver only
            std::vector<std::string> expOut;
            for (const json& s : a.at("out")) {
                std::string x = s.get<std::string>();
                if (x.rfind("collision:", 0) == 0) x = x.substr(0, x.rfind(':'));
                expOut.push_back(x);
            }
            const bool ok = V->Landed == a.at("landed").get<bool>() && V->Splashed == a.at("splashed").get<bool>() &&
                            V->landedAt == a.at("landedAt").get<std::string>() && V->displaylandedAt == a.at("display").get<std::string>() &&
                            std::string(SituationName(V->situation)) == a.at("situation").get<std::string>() &&
                            gc == a.at("groundContacts").get<std::vector<std::string>>() && ground == a.at("ground").get<std::vector<bool>>() &&
                            cur == a.at("current") && rec.out == expOut;
            if (!ok && mismatches++ < 5) {
                json got{{"landed", V->Landed}, {"splashed", V->Splashed}, {"landedAt", V->landedAt}, {"display", V->displaylandedAt},
                         {"situation", SituationName(V->situation)}, {"groundContacts", gc}, {"ground", ground}, {"current", cur}, {"out", rec.out}};
                MESSAGE("sequence " << seqs << " message " << k << " (" << kind << " " << ev.at("recv") << "/" << ev.at("this") << " x " << other << "): got "
                                    << got.dump() << " expected " << a.dump());
            }
            ++k;
            ++msgs;
        }
        ++seqs;
    }
    MESSAGE(seqs << " sequences, " << msgs << " messages, " << mismatches << " mismatches");
    CHECK(seqs == 300);
    CHECK(mismatches == 0);
}

// ===================================================================================================================== PartBuoyancy

namespace {
// the reference's localToWorld matrix of a pose (tools/wp20e/contact_reference.py trs_matrix: not under test)
Matrix4x4f PoseMatrix(const Vector3f& pos, const Quaternionf& q) {
    const double x = q.x, y = q.y, z = q.z, w = q.w;
    Matrix4x4f m;
    m.m00 = static_cast<float>(1 - 2 * (y * y + z * z));
    m.m01 = static_cast<float>(2 * (x * y - z * w));
    m.m02 = static_cast<float>(2 * (x * z + y * w));
    m.m03 = pos.x;
    m.m10 = static_cast<float>(2 * (x * y + z * w));
    m.m11 = static_cast<float>(1 - 2 * (x * x + z * z));
    m.m12 = static_cast<float>(2 * (y * z - x * w));
    m.m13 = pos.y;
    m.m20 = static_cast<float>(2 * (x * z - y * w));
    m.m21 = static_cast<float>(2 * (y * z + x * w));
    m.m22 = static_cast<float>(1 - 2 * (x * x + y * y));
    m.m23 = pos.z;
    m.m33 = 1.0f;
    return m;
}
}  // namespace

TEST_CASE("WP-20e reference: PartBuoyancy.Start / FixedUpdate (depths, submersion, buoyant force, water drag, splashdown, crash), 110 parts, 0 ULP") {
    if (!wp20b::GameAvailable()) return;
    World w;
    UseReferenceBody(w);
    FlightGlobals::ready = true;
    TimeWarp tw;
    TimeWarpEnvironment twEnv;
    twEnv.LoadedSceneValue = GameScenes::FLIGHT;
    TimeWarp::environment = &twEnv;
    TimeWarp::fetch = &tw;
    tw.warpRates = {1.0f, 5.0f, 10.0f, 50.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f};
    tw.physicsWarpRates = {1.0f, 2.0f, 3.0f, 4.0f};
    const float savedDt = Time::fixedDeltaTime;
    size_t cases = 0, ticks = 0, mismatches = 0, forces = 0, crashes = 0, fx = 0, libmCases = 0, libmTicks = 0;
    std::string first;
    for (const json& c : Ref().at("buoyancy")) {
        auto v = w.CraftHead("PhysProf-S", 1);
        OrbitKerbin(w, *v);
        REQUIRE(FlightGlobals::getMainBody(Vector3d(V3f(c.at("ticks")[0].at("pos")))) == &w.Kerbin());
        Part& p = *v->rootPart;
        const json& pj = c.at("part");
        p.fields.CenterOfBuoyancy = V3f(pj.at("cob"));
        p.fields.CenterOfDisplacement = V3f(pj.at("cod"));
        p.fields.buoyancy = Fl(pj.at("buoyancy"));
        p.fields.buoyancyUseSine = pj.at("useSine").get<bool>();
        p.fields.crashTolerance = Fl(pj.at("crashTolerance"));
        p.DragCubes.SetWeightedBounds(V3f(pj.at("wcenter")), V3f(pj.at("wsize")));
        for (int i = 0; i < 6; ++i) p.DragCubes.WeightedArea()[static_cast<size_t>(i)] = Fl(pj.at("warea")[static_cast<size_t>(i)]);
        DragCubeList::CubeData cd = p.DragCubes.GetCubeData();
        cd.areaDrag = Fl(pj.at("areaDrag"));
        p.DragCubes.SetCubeData(cd);
        p.DragCubes.SetNone(pj.at("none").get<bool>());
        p.set_ShieldedFromAirstream(pj.at("shielded").get<bool>());
        p.fields.buoyancyUseCubeNamed = std::string("");
        if (!pj.at("override").is_null()) {
            DragCube cube("WP20E_REFERENCE");
            cube.SetCenter(V3f(pj.at("override").at("center")));
            cube.SetSize(V3f(pj.at("override").at("size")));
            for (int i = 0; i < 6; ++i) cube.Area()[static_cast<size_t>(i)] = Fl(pj.at("override").at("area")[static_cast<size_t>(i)]);
            p.DragCubes.Cubes().push_back(cube);
            p.fields.buoyancyUseCubeNamed = std::string("WP20E_REFERENCE");
        }
        const json& vj = c.at("vessel");
        v->loaded = true;
        v->waterOffset = D(vj.at("waterOffset"));
        v->terrainAltitude = D(vj.at("terrainAltitude"));
        v->srf_vel_direction = V3(vj.at("srfVelDir"));
        v->srfSpeed = D(vj.at("srfSpeed"));
        v->ctrlState.pitch = Fl(vj.at("ctrl")[0]);
        v->ctrlState.yaw = Fl(vj.at("ctrl")[1]);
        v->ctrlState.roll = Fl(vj.at("ctrl")[2]);
        v->gravityMultiplier = D(vj.at("gravityMultiplier"));
        v->Splashed = false;
        tw.curr_rate = Fl(c.at("rate"));
        tw.Mode = c.at("low").get<bool>() ? TimeWarp::Modes::LOW : TimeWarp::Modes::HIGH;
        Time::fixedDeltaTime = Fl(c.at("dt"));
        // the Unity state of the part
        float rbDrag = 0.0f;
        bool angularSet = false;
        float rbAngular = 0.0f;
        size_t tick = 0;
        bool everSet = false;
        bool tolerant = false;  // the runtime's Math.Sin differed from the reference's libm in this case (see below)
        bool applied = false;
        Vector3d appliedF, appliedP;
        bool splashFx = false;
        Vector3f fxPos;
        float fxPower = 0.0f;
        PartBuoyancy::Env env;
        env.hasRigidbody = [](Part&) { return true; };
        env.rbVelocity = [&](Part&) { return V3f(c.at("ticks")[tick].at("rbv")); };
        env.rbMass = [&](Part&) { return Fl(c.at("mass")); };
        env.rbDrag = [&](Part&) { return rbDrag; };
        env.setRbDrag = [&](Part&, float d) { rbDrag = d; };
        env.setRbAngularDrag = [&](Part&, float d) {
            angularSet = true;
            rbAngular = d;
        };
        env.localToWorld = [](Part& x) { return PoseMatrix(x.partTransform.position, x.partTransform.rotation); };
        env.frameVelocity = [&] { return V3(c.at("frameVel")); };
        env.cameraPosition = [&] { return V3(c.at("camera")); };
        env.splashFx = [&](const Vector3f& pos, float power) {
            splashFx = true;
            fxPos = pos;
            fxPower = power;
        };
        env.addForceAtPosition = [&](Part& x, const Vector3d& f, const Vector3d& pos) {
            applied = &x == &p;
            appliedF = f;
            appliedP = pos;
        };
        env.colliderBoundsSize = [&](Part&) { return V3f(pj.at("colliderSize")); };
        auto pose = [&](size_t k) {
            const json& t = c.at("ticks")[k];
            p.partTransform.position = V3f(t.at("pos"));
            p.partTransform.rotation = Quaternionf(Fl(t.at("rot")[0]), Fl(t.at("rot")[1]), Fl(t.at("rot")[2]), Fl(t.at("rot")[3]));
        };
        pose(0);
        PartBuoyancy b;
        b.Start(p, env);
        for (tick = 0; tick < c.at("out").size(); ++tick) {
            pose(tick);
            applied = false;
            splashFx = false;
            angularSet = false;
            b.FixedUpdate(env);
            const json& o = c.at("out")[tick];
            std::string what;
            // Math.Sin of the submerged portion (PartBuoyancy.cs:513): dotnet::Math::Sin (KSP's runtime) vs the reference's libm. Where they differ
            // (an ulp), the values that follow from the portion (portion, buoyant force, its point, water drag) are compared to 1e-12 relative /
            // 4 float ulps from then on; every other value stays bit for bit
            if (o.contains("sinArg") && HexD(dotnet::Math::Sin(D(o.at("sinArg")))) != o.at("sin").get<std::string>()) {
                if (!tolerant) ++libmCases;
                tolerant = true;
                ++libmTicks;
            }
            auto closeD = [](double a, double e) { return std::fabs(a - e) <= 1e-12 * std::max(1.0, std::fabs(e)); };
            auto closeF = [](float a, float e) { return std::fabs(static_cast<double>(a) - e) <= 4.0 * std::fabs(static_cast<double>(std::nextafter(e, 2.0f * e + 1.0f)) - e) + 1e-30; };
            auto closeF3 = [&](const Vector3f& a, const json& j) {
                const Vector3f e = V3f(j);
                return closeF(a.x, e.x) && closeF(a.y, e.y) && closeF(a.z, e.z);
            };
            auto chkD = [&](const char* name, double got) {
                if (what.empty() && HexD(got) != o.at(name).get<std::string>()) what = std::string(name) + " " + HexD(got) + " vs " + o.at(name).get<std::string>();
            };
            auto chkDsin = [&](const char* name, double got) {
                if (!tolerant) return chkD(name, got);
                if (what.empty() && !closeD(got, D(o.at(name)))) what = std::string(name) + " (tolerant)";
            };
            auto sameF3sin = [&](const Vector3f& a, const json& j) { return tolerant ? closeF3(a, j) : SameF3(a, j); };
            auto chkB = [&](const char* name, bool got) {
                if (what.empty() && got != o.at(name).get<bool>()) what = name;
            };
            chkD("depth", p.fields.depth);
            chkD("minDepth", p.fields.minDepth);
            chkD("maxDepth", p.fields.maxDepth);
            chkD("lvm", b.localVelocityMag);
            chkDsin("submerged", p.fields.submergedPortion);
            chkD("dragScalar", p.fields.submergedDragScalar);
            chkD("liftScalar", p.fields.submergedLiftScalar);
            chkD("splashedCounter", b.splashedCounter);
            chkD("displacement", b.displacement);
            chkB("splashed", b.splashed);
            chkB("waterContact", p.fields.WaterContact);
            chkB("vesselSplashed", v->Splashed);
            chkB("angularDragByFI", p.fields.angularDragByFI);
            chkB("crash", b.lastCrash);
            chkB("dead", b.dead);
            if (what.empty() && (tolerant ? !closeF(rbDrag, Fl(o.at("rbDrag"))) : HexF(rbDrag) != o.at("rbDrag").get<std::string>())) what = "rbDrag";
            if (what.empty() && HexF(b.maxDimension) != o.at("maxDimension").get<std::string>()) what = "maxDimension";
            // rb.angularDrag keeps its last value: the reference reports the last one set
            everSet = everSet || angularSet;
            if (what.empty() && o.at("rbAngular").is_null() == everSet) what = "rbAngular set";
            if (what.empty() && everSet && HexF(rbAngular) != o.at("rbAngular").get<std::string>()) what = "rbAngular";
            if (o.contains("buoyantGeeForce")) chkDsin("buoyantGeeForce", b.buoyantGeeForce);
            if (what.empty() && o.contains("effectiveForce") && !sameF3sin(b.effectiveForce, o.at("effectiveForce"))) what = "effectiveForce";
            if (what.empty() && !sameF3sin(b.lastForcePosition, o.at("lastForcePosition"))) what = "lastForcePosition";
            const json& ap = o.at("applied");
            if (what.empty() && ap.is_null() == applied) what = "applied";
            if (what.empty() && applied &&
                (!sameF3sin(appliedF.ToVector3f(), ap[1]) || !sameF3sin(appliedP.ToVector3f(), ap[2]) || ap[0].get<std::string>() != "self"))
                what = "applied force";
            const json& fj = o.at("fx");
            if (what.empty() && fj.is_null() == splashFx) what = "fx";
            if (what.empty() && splashFx && (!SameF3(fxPos, fj[0]) || HexF(fxPower) != fj[1].get<std::string>())) what = "fx values";
            if (!what.empty()) {
                if (mismatches == 0) first = "case " + std::to_string(cases) + " tick " + std::to_string(tick) + ": " + what;
                ++mismatches;
            }
            forces += applied;
            crashes += b.lastCrash;
            fx += splashFx;
            ++ticks;
            if (b.dead) break;
        }
        ++cases;
    }
    Time::fixedDeltaTime = savedDt;
    TimeWarp::fetch = nullptr;
    TimeWarp::environment = nullptr;
    FlightGlobals::ready = false;
    MESSAGE(cases << " parts, " << ticks << " FixedUpdates (" << forces << " buoyant forces applied, " << crashes << " splashdown crashes, " << fx
                  << " splash FX), " << mismatches << " mismatches" << (first.empty() ? "" : "; first: " + first) << "; Math.Sin differed from the reference libm in "
                  << libmTicks << " ticks of " << libmCases << " parts (their portion-dependent values to 1e-12 / 4 ulps)");
    CHECK(cases == 110);
    CHECK(mismatches == 0);
}

// ===================================================================================================================== CollisionEnhancer

TEST_CASE("WP-20e reference: CollisionEnhancer.FixedUpdate (the 0.1 m rule, linecast, explode / translate back, reflected velocity), 400 parts, 0 ULP") {
    if (!wp20b::GameAvailable()) return;
    World w;
    auto v = w.CraftHead("PhysProf-S", 1);
    Part& p = *v->rootPart;
    Recorder rec;
    rec.nameOf = [](Part*) { return std::string("P"); };
    rec.crashKind = [](EventReport*) { return std::string("crash"); };
    const std::string terrainName = Localizer::Format("#autoLOC_204427");
    size_t cases = 0, ticks = 0, mismatches = 0, linecasts = 0;
    std::string first;
    for (const json& c : Ref().at("enhancer")) {
        p.fields.crashTolerance = Fl(c.at("tol"));
        CollisionEnhancer ce;
        const std::string beh = c.at("behaviour").get<std::string>();
        ce.OnTerrainPunchThrough = beh == "EXPLODE" ? CollisionEnhancerBehaviour::EXPLODE
                                   : beh == "TRANSLATE_BACK" ? CollisionEnhancerBehaviour::TRANSLATE_BACK
                                                             : CollisionEnhancerBehaviour::TRANSLATE_BACK_SPLAT;
        ce.instanceMinDistSqr = Fl(c.at("instMinDistSqr"));
        size_t tick = 0;
        Vector3f pos;
        bool linecast = false, setPos = false, setVel = false;
        Vector3f lcFrom, lcTo, posSet, velSet;
        CollisionEnhancer::Env env;
        env.packed = [&](Part&) { return c.at("ticks")[tick].at("packed").get<bool>(); };
        env.position = [&](Part&) { return pos; };
        env.setPosition = [&](Part&, const Vector3f& x) {
            setPos = true;
            posSet = x;
            pos = x;
        };
        env.velocity = [&](Part&) { return V3f(c.at("ticks")[tick].at("rbv")); };
        env.setVelocity = [&](Part&, const Vector3f& x) {
            setVel = true;
            velSet = x;
        };
        env.linecast = [&](const Vector3f& from, const Vector3f& to) {
            linecast = true;
            lcFrom = from;
            lcTo = to;
            CollisionEnhancer::LinecastHit h;
            const json& hj = c.at("ticks")[tick].at("hit");
            if (hj.is_null()) return h;
            h.hit = true;
            h.point = V3f(hj.at("point"));
            h.normal = V3f(hj.at("normal"));
            h.hasRigidbody = hj.at("hasRb").get<bool>();
            h.rigidbodyVelocity = V3f(hj.at("rbv"));
            h.rigidbodyMass = Fl(hj.at("mass"));
            h.rigidbodyName = "Rigidbody";
            return h;
        };
        env.upAxis = [&](const Vector3f&) { return V3f(c.at("ticks")[tick].at("up")); };
        env.frameVelocity = [&] { return V3(c.at("ticks")[tick].at("frameVel")); };
        env.worldShift = [&] { return V3(c.at("ticks")[tick].at("shift")); };
        pos = V3f(c.at("state").at("lastPos"));
        ce.Start(p, env);
        ce.lastPos = V3f(c.at("state").at("lastPos"));
        ce.wasPacked = c.at("state").at("wasPacked").get<bool>();
        ce.framesToSkip = c.at("state").at("framesToSkip").get<int>();
        for (tick = 0; tick < c.at("ticks").size(); ++tick) {
            const json& t = c.at("ticks")[tick];
            pos = V3f(t.at("pos"));
            CollisionEnhancer::bypass = t.at("bypass").get<bool>();
            PartContactEnvironment::noCrashDamage = t.at("noCrash").get<bool>();
            p.set_State(t.at("dead").get<bool>() ? PartStates::DEAD : PartStates::IDLE);
            linecast = setPos = setVel = false;
            rec.out.clear();
            rec.lastOther.clear();
            rec.hits = 0;
            ce.FixedUpdate(env);
            const json& o = c.at("out")[tick];
            std::string what;
            if (o.at("linecast").is_null() == linecast) what = "linecast";
            else if (linecast && (!SameF3(lcFrom, o.at("linecast")[0]) || !SameF3(lcTo, o.at("linecast")[1]))) what = "linecast args";
            const bool exploded = rec.Exploded();
            if (what.empty() && exploded != o.at("explode").get<bool>()) what = "explode";
            if (what.empty() && o.at("setPos").is_null() == setPos) what = "setPos";
            if (what.empty() && setPos && !SameF3(posSet, o.at("setPos"))) what = "setPos value";
            if (what.empty() && o.at("setVel").is_null() == setVel) what = "setVel";
            if (what.empty() && setVel && !SameF3(velSet, o.at("setVel"))) what = "setVel value";
            if (what.empty() && (rec.hits > 0) != o.at("hitEvent").get<bool>()) what = "hitEvent";
            const std::string coll = rec.lastOther.empty() ? "" : (rec.lastOther == terrainName ? "terrain" : "rb");
            if (what.empty() && (o.at("collision").is_null() ? std::string() : o.at("collision").get<std::string>()) != coll) what = "collision";
            if (what.empty() && !SameF3(ce.lastPos, o.at("lastPos"))) what = "lastPos";
            if (what.empty() && ce.framesToSkip != o.at("framesToSkip").get<int>()) what = "framesToSkip";
            if (what.empty() && ce.wasPacked != o.at("wasPacked").get<bool>()) what = "wasPacked";
            if (!what.empty()) {
                if (mismatches == 0) first = "case " + std::to_string(cases) + " tick " + std::to_string(tick) + ": " + what;
                ++mismatches;
            }
            linecasts += linecast;
            ++ticks;
        }
        ++cases;
    }
    p.set_State(PartStates::IDLE);
    CollisionEnhancer::bypass = false;
    PartContactEnvironment::noCrashDamage = false;
    MESSAGE(cases << " parts, " << ticks << " FixedUpdates, " << linecasts << " linecasts, " << mismatches << " mismatches"
                  << (first.empty() ? "" : "; first: " + first));
    CHECK(cases == 400);
    CHECK(mismatches == 0);
}

// ===================================================================================================================== Vessel.AddRBAnchor

TEST_CASE("WP-20e reference: Vessel.AddRBAnchor timelines (the 1.5 s rule, release on throttle / angle change / downward force, break force), 90 vessels") {
    if (!wp20b::GameAvailable()) return;
    AnchorSettings settings;
    World w;
    UseReferenceBody(w);
    size_t cases = 0, frames = 0, mismatches = 0, angleMismatch = 0, creates = 0;
    double maxAngle = 0.0;
    std::string first;
    for (const json& c : Ref().at("anchor")) {
        auto v = w.CraftHead("PhysProf-S", 1);
        OrbitKerbin(w, *v);
        v->vesselTransform.position = V3f(c.at("pos"));
        v->totalMass = D(c.at("mass"));
        v->graviticAcceleration = V3(c.at("gacc"));
        v->state = c.at("active").get<bool>() ? Vessel::State::ACTIVE : Vessel::State::INACTIVE;
        v->vesselType = VesselType::Probe;
        size_t f = 0;
        bool created = false, destroyed = false, bfSet = false;
        float cbf = 0.0f, cbt = 0.0f, bf = 0.0f;
        Vessel::env.rbAnchorCreate = [&](Vessel&, float a, float b) {
            created = true;
            cbf = a;
            cbt = b;
            return true;
        };
        Vessel::env.rbAnchorDestroy = [&](Vessel&) { destroyed = true; };
        Vessel::env.rbAnchorCurrentForce = [&](Vessel&) { return V3f(c.at("frames")[f].at("force")); };
        Vessel::env.rbAnchorSetBreakForce = [&](Vessel&, float x) {
            bfSet = true;
            bf = x;
        };
        for (f = 0; f < c.at("frames").size(); ++f) {
            const json& fr = c.at("frames")[f];
            Vessel::env.deltaTime = Fl(fr.at("dt"));
            v->loaded = fr.at("loaded").get<bool>();
            v->packed = fr.at("packed").get<bool>();
            v->precalc.set_isEasingGravity(fr.at("easing").get<bool>());
            v->Landed = fr.at("landed").get<bool>();
            v->rb_velocity = V3f(fr.at("rbv"));
            v->ctrlState.mainThrottle = Fl(fr.at("throttle"));
            v->ctrlState.wheelThrottle = 0.0f;
            created = destroyed = bfSet = false;
            v->AddRBAnchor();
            const json& o = fr.at("out");
            std::string what;
            if (o.at("create").is_null() == created) what = "create";
            else if (created && (HexF(cbf) != o.at("create")[0].get<std::string>() || HexF(cbt) != o.at("create")[1].get<std::string>())) what = "create args";
            if (what.empty() && destroyed != o.at("destroy").get<bool>()) what = "destroy";
            if (what.empty() && o.at("breakForce").is_null() == bfSet) what = "breakForce";
            if (what.empty() && bfSet && HexF(bf) != o.at("breakForce").get<std::string>()) what = "breakForce value";
            if (what.empty() && v->HasRBAnchor() != o.at("joint").get<bool>()) what = "joint";
            if (what.empty() && HexF(v->AnchorTimeCounter()) != o.at("timer").get<std::string>()) what = "timer";
            if (what.empty() && HexF(v->AnchorDeltaAngleTimeCounter()) != o.at("dtimer").get<std::string>()) what = "dtimer";
            // the angles come through Math.Acos (the platform libm): a stated tolerance
            const double dAngle = std::fabs(static_cast<double>(v->AnchorForceAngle()) - Fl(o.at("angle")));
            const double dPrev = std::fabs(static_cast<double>(v->PrevAnchorForceAngle()) - Fl(o.at("prev")));
            const double dDelta = std::fabs(static_cast<double>(v->AnchorDeltaAngle()) - Fl(o.at("delta")));
            maxAngle = std::max(maxAngle, std::max(dAngle, std::max(dPrev, dDelta)));
            if (dAngle > 1e-4 || dPrev > 1e-4 || dDelta > 1e-4) ++angleMismatch;
            if (!what.empty()) {
                if (mismatches == 0) first = "case " + std::to_string(cases) + " frame " + std::to_string(f) + ": " + what;
                ++mismatches;
            }
            creates += created;
            ++frames;
        }
        v->ResetRBAnchor();
        ++cases;
    }
    Vessel::env.rbAnchorCreate = nullptr;
    Vessel::env.rbAnchorDestroy = nullptr;
    Vessel::env.rbAnchorCurrentForce = nullptr;
    Vessel::env.rbAnchorSetBreakForce = nullptr;
    MESSAGE(cases << " vessels, " << frames << " LateUpdates, " << creates << " anchors made, " << mismatches << " mismatches"
                  << (first.empty() ? "" : "; first: " + first) << "; largest angle difference " << maxAngle << " deg");
    CHECK(cases == 90);
    CHECK(mismatches == 0);
    CHECK(angleMismatch == 0);
}
