// WP-29d: landing legs and landing gear on the wheel host with the parts' real models (core/data/wheel_models): the Two-Stage Lander's four
// LT-2 legs, the Aeris 3A's three LY-01 gear bays on a runway roll, the deployment animation (ModuleWheelDeployment over the rig), the
// stand-in sphere of a retracted gear.
//
// The craft are KSP's own (harness/inputs/Ships/VAB/Two-Stage Lander.craft, SPH/Aeris 3A.craft), loaded through the WP-18 / WP-14 path as the
// WP-29c rovers. References: closed forms (the suspension law's static compression, energy of the touchdown), the spike's measured numbers
// (WHEEL-SPIKE.md 4.4: legs at 3 m/s touchdown on Kerbin) as a cross-check. Not captures: no in-game gear tick exists (capture C11).
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>

#include "doctest/doctest.h"
#include "vessel_wheels_fixture.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp29c;

namespace {

constexpr double kRad = 3.14159265358979323846 / 180.0;
constexpr int kWheelsUp = 4;

struct GearRun {
    World w;
    LoadedRover r;
    std::unique_ptr<RoverScene> scene;
    explicit GearRun(const std::string& craft, double clearance = 0.0, double downSpeed = 0.0, WheelPhysicsOptions o = RealGeometry(), double heading = 0.0, bool airborne = false) {
        r = LoadRover(w, craft);
        scene = std::make_unique<RoverScene>(w, RoverOptions(r, o));
        scene->Place(r, heading, clearance, airborne);
        scene->pad.s.Start(*r.vessel);
        if (downSpeed != 0.0) Vp()->ChangeWorldVelocity(scene->pad.padUp * -downSpeed);
    }
    ~GearRun() {
        r.vessel.reset();
        scene.reset();
    }
    Vessel& V() { return *r.vessel; }
    VesselPhysics* Vp() { return scene->pad.s.bubble.Host().Find(V()); }
    Vector3d Up() const { return scene->pad.padUp; }
    void Tick() { scene->Tick(); }
    VesselWheels& Wheels() { return *scene->wheels.Find(V()); }
    Vector3d Com() { return VesselCoM(*Vp()); }
    Vector3d Vel() { return VesselVelocity(*Vp()); }
    double VerticalSpeed() { return Vector3d::Dot(Vel(), Up()); }
    double Altitude(const Vector3f& p) { return (Vector3d(p) - w.Kerbin().position).magnitude() - w.Kerbin().Radius; }
    double BodyUpDot() { return Vector3d::Dot(Vector3d(V().vesselTransform.rotation * r.craftUp).normalized(), Up()); }
    // the lowest tyre bottom (at full extension) of the set-up wheels above the ground sphere
    double LowestTyreBottom() {
        double lo = 1e9;
        for (auto& w : Wheels().Wheels()) {
            if (w->Controller() == nullptr || w->Controller()->wheelCollider == nullptr) continue;
            const auto& g = w->lastGeometry;
            const auto& wc = w->Controller()->wheelCollider->wheelColliderData;
            const Vector3d bottom = Vector3d(g.top) - Vector3d(g.up) * (static_cast<double>(wc.suspensionDistance) + static_cast<double>(wc.radius));
            lo = std::min(lo, Altitude(bottom.ToVector3f()));
        }
        return lo;
    }
};

bool Healthy(GearRun& run) {
    if (run.Vp() == nullptr) return false;
    const Vector3d v = run.Vel();
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && v.magnitude() < 200.0;
}

// A craft placed airborne with its lowest tyre bottom (at full extension) `fall` above the ground and `downSpeed` m/s down: the clearance comes from
// a measuring scene (Place puts the lowest drag-cube point at the clearance, the tyre bottoms hang below it)
std::unique_ptr<GearRun> Dropped(const std::string& craft, double fall, double downSpeed, const WheelPhysicsOptions& o = RealGeometry(), double heading = 0.0) {
    double bottom0 = 0.0;
    {
        GearRun m(craft, 0.0, 0.0, o, heading, true);
        for (int k = 0; k < 40; ++k) {
            m.Tick();
            if (k >= kWheelsUp && m.Vp() != nullptr && m.Wheels().SetUpCount() == static_cast<int>(m.Wheels().Wheels().size())) break;
        }
        m.Tick();
        bottom0 = m.LowestTyreBottom();
    }
    auto run = std::make_unique<GearRun>(craft, fall - bottom0, 0.0, o, heading, true);
    for (int k = 0; k < 10 && run->Vp() == nullptr; ++k) run->Tick();  // the vessel unpacks in the first ticks
    REQUIRE(run->Vp() != nullptr);
    if (downSpeed != 0.0) run->Vp()->ChangeWorldVelocity(run->Up() * -downSpeed);
    return run;
}

struct Touchdown {
    double touchSpeed = 0, maxCompression = 0, maxRebound = 0, settleTime = -1, finalCompression = 0, finalTilt = 0, finalSpeed = 0;
    int legsGrounded = 0, legs = 0, bottomedTicks = 0;
    bool healthy = true;
    double h0 = 0;
};

// The lander dropped so that its tyre bottoms reach the ground at `touch` m/s: the clearance comes from a measuring scene
Touchdown LanderTouchdown(double touch, WheelPhysicsOptions o = RealGeometry(), double fall = 0.03) {
    Touchdown t;
    // 1. the tyre-bottom height of the craft placed at clearance 0 (the proxy's lowest point on the ground)
    double bottom0 = 0.0;
    {
        GearRun m("Two-Stage Lander", 0.0, 0.0, o, 0.0, true);
        for (int k = 0; k < kWheelsUp; ++k) m.Tick();
        bottom0 = m.LowestTyreBottom();
    }
    // 2. the drop: the tyre bottom `fall` above the ground, the speed that reaches `touch` there (v^2 = v0^2 + 2 g h)
    GearRun run("Two-Stage Lander", fall - bottom0, 0.0, o, 0.0, true);
    const double gee = 9.81;  // Kerbin
    const double v0 = std::sqrt(std::max(0.0, touch * touch - 2.0 * gee * fall));
    for (int k = 0; k < 10 && run.Vp() == nullptr; ++k) run.Tick();  // the vessel unpacks in the first ticks
    REQUIRE(run.Vp() != nullptr);
    run.Vp()->ChangeWorldVelocity(run.Up() * -v0);
    t.h0 = fall;
    double lastMove = 0, prevVz = 0;
    for (int k = 0; k < 600; ++k) {
        prevVz = run.VerticalSpeed();
        run.Tick();
        if (!Healthy(run)) {
            t.healthy = false;
            break;
        }
        const double vz = run.VerticalSpeed();
        int grounded = 0;
        double comp = 0;
        for (auto& w : run.Wheels().Wheels()) {
            if (!w->last.grounded) continue;
            ++grounded;
            const auto& gm = w->lastGeometry;
            comp = std::max(comp, static_cast<double>((gm.maxDroop + w->last.jounce) / (gm.maxDroop + gm.maxCompression)));
            t.bottomedTicks += w->last.limitActive ? 1 : 0;
        }
        if (grounded > 0 && t.touchSpeed == 0.0) t.touchSpeed = -prevVz;  // the speed at the start of the tick that met the ground
        t.maxCompression = std::max(t.maxCompression, comp);
        if (t.touchSpeed > 0.0) {
            t.maxRebound = std::max(t.maxRebound, vz);
            if (std::fabs(vz) > 0.05) lastMove = (k + 1) * 0.02;
        }
        t.legsGrounded = grounded;
        t.finalCompression = comp;
        t.finalSpeed = run.Vel().magnitude();
    }
    t.settleTime = lastMove;
    t.legs = static_cast<int>(run.Wheels().Wheels().size());
    t.finalTilt = std::acos(std::max(-1.0, std::min(1.0, run.BodyUpDot()))) / kRad;
    return t;
}

}  // namespace

TEST_CASE("WP-29d lander legs: the LT-2 geometry from the model (collider transforms, deployTgt, animation at the deployed state)") {
    if (!wp20b::GameAvailable()) return;
    GearRun run("Two-Stage Lander", 0.0, 0.0);
    for (int k = 0; k < kWheelsUp; ++k) run.Tick();
    auto& legs = run.Wheels().Wheels();
    REQUIRE(legs.size() == 4);
    for (auto& w : legs) {
        REQUIRE(w->rig != nullptr);
        CHECK(w->colliderNode >= 0);
        CHECK(w->base->IsWheelSetUp());
        CHECK(w->geometryLocked);
        // ModuleWheelDeployment.OnWheelSetup moved the collider transform to deployTgt: the collider host sits at deployTgt's pose
        VehiclePhysics::NativeTransform* tgt = WheelModuleEnvironment::findModelTransform(*w->part, "deployTgt");
        REQUIRE(tgt != nullptr);
        CHECK((Vector3d(tgt->position) - Vector3d(w->colliderHost.position)).magnitude() < 1e-4);
        // the deployed foot hangs below the leg's body: the suspension axis (collider up) is close to the vessel's up
        CHECK(Vector3d::Dot(Vector3d(w->colliderHost.up()), run.Up()) > 0.7);
    }
}

TEST_CASE("WP-29d lander legs: 3 m/s touchdown on Kerbin (no bounce-launch, compression 0.35-0.75 of the travel, settles within 3 s)") {
    if (!wp20b::GameAvailable()) return;
    const Touchdown t = LanderTouchdown(3.0);
    MESSAGE("lander: touch " << t.touchSpeed << " m/s, max compression " << t.maxCompression << " of the travel, max rebound " << t.maxRebound << " m/s, settled after "
                             << t.settleTime << " s, final compression " << t.finalCompression << ", legs grounded " << t.legsGrounded << "/" << t.legs << ", bottomed ticks "
                             << t.bottomedTicks << ", tilt " << t.finalTilt << " deg, speed " << t.finalSpeed);
    CHECK(t.healthy);
    CHECK(t.touchSpeed == doctest::Approx(3.0).epsilon(0.08));
    CHECK(t.maxCompression > 0.35);                         // WHEEL-SPIKE.md 4.4: 50 % with the proxy, 62 % with Jolt's VehicleConstraint
    CHECK(t.maxCompression < 0.75);
    CHECK(t.bottomedTicks == 0);                            // the travel limit is not reached
    CHECK(t.maxRebound < t.touchSpeed);                     // no bounce-launch
    CHECK(t.settleTime > 0.0);
    CHECK(t.settleTime < 3.0);                              // settles
    CHECK(t.legsGrounded == t.legs);
    CHECK(t.finalTilt < 0.5);
    CHECK(t.finalSpeed < 0.1);
}


namespace {
std::vector<ModuleWheels::ModuleWheelDeployment*> Deployments(Vessel& v, std::vector<Part*>* parts = nullptr) {
    std::vector<ModuleWheels::ModuleWheelDeployment*> out;
    for (Part* p : v.parts)
        for (size_t i = 0; i < p->Modules().Count(); ++i)
            if (auto* d = dynamic_cast<ModuleWheels::ModuleWheelDeployment*>(p->Modules()[i])) {
                out.push_back(d);
                if (parts) parts->push_back(p);
            }
    return out;
}
}  // namespace

void RetractDeploy(const std::string& craft, size_t expectedParts, bool expectStandIn) {
    if (!wp20b::GameAvailable()) return;
    GearRun run(craft, 3000.0, 0.0, RealGeometry(), 0.0, true);  // high above the ground (24 s of free fall): only the gear is under test
    for (int k = 0; k < 8; ++k) run.Tick();
    std::vector<Part*> depParts;
    auto deps = Deployments(run.V(), &depParts);
    REQUIRE(deps.size() == expectedParts);
    for (auto* d : deps) {
        CHECK(d->Position() == doctest::Approx(1.0f));
        CHECK(!d->StandInColliderActive());
    }
    auto foot = [&](size_t i) {  // the deploy target in the part frame (the model rig answers in part space)
        VehiclePhysics::NativeTransform pt;
        pt.position = depParts[i]->partTransform.position;
        pt.rotation = depParts[i]->partTransform.rotation;
        return Vector3d(pt.InverseTransformPoint(deps[i]->DeployTarget()->position));
    };
    std::vector<Vector3d> footDeployed;
    for (size_t i = 0; i < deps.size(); ++i) footDeployed.push_back(foot(i));
    for (auto* d : deps) d->ActionToggle(KSPActionType::Deactivate);  // the Gear action group: retract
    bool standIn = false, wheelOffSeen = false, standInBuilt = false;
    double minPos = 1.0;
    for (int k = 0; k < 400; ++k) {  // 8 s
        run.Tick();
        for (auto* d : deps) {
            standIn = standIn || d->StandInColliderActive();
            minPos = std::min(minPos, static_cast<double>(d->Position()));
        }
        for (auto& w : run.Wheels().Wheels()) {
            wheelOffSeen = wheelOffSeen || !w->nativeOn;
            standInBuilt = standInBuilt || w->standInBuilt;
        }
    }
    MESSAGE("retracted: position " << deps[0]->Position() << " (min " << minPos << "), stand-in seen " << standIn << ", wheel off seen " << wheelOffSeen << ", state "
                                   << deps[0]->Get<std::string>("stateString") << ", foot y " << foot(0).y << " (deployed " << footDeployed[0].y << ")");
    CHECK(deps[0]->Position() < 0.05f);
    CHECK(standIn == expectStandIn);        // useStandInCollider: the LY gear bays have one, the LT-2 legs do not
    CHECK(standInBuilt == expectStandIn);   // ... and it is a sphere in the cluster compound while the gear is not deployed
    CHECK(wheelOffSeen);
    CHECK(deps[0]->Get<std::string>("stateString") == "Retracted");
    CHECK(foot(0).y > footDeployed[0].y + 0.3);  // the foot came up in the part frame
    for (auto* d : deps) d->ActionToggle(KSPActionType::Activate);  // deploy again
    for (int k = 0; k < 400; ++k) run.Tick();
    MESSAGE("deployed again: position " << deps[0]->Position() << ", state " << deps[0]->Get<std::string>("stateString") << ", foot y " << foot(0).y);
    CHECK(deps[0]->Position() > 0.95f);
    CHECK(!deps[0]->StandInColliderActive());
    CHECK(deps[0]->Get<std::string>("stateString") == "Deployed");
    for (auto& w : run.Wheels().Wheels()) CHECK(!w->standInBuilt);
    CHECK((foot(0) - footDeployed[0]).magnitude() < 0.01);
}

TEST_CASE("WP-29d lander legs retract and deploy: the animation over the model rig, TsubSys, the wheel on and off (no stand-in on the LT-2)") { RetractDeploy("Two-Stage Lander", 4, false); }

TEST_CASE("WP-29d gear bays retract and deploy: the animation, TsubSys, the stand-in sphere in the cluster compound, the wheel on and off (LY-01 on the VTOL craft)") { RetractDeploy("Rocket-power VTOL", 4, true); }

// Vessel.CheckGroundCollision / getLowestPoint (Vessel.cs:2578-2640) on a landed craft with legs: the lowest collider decides, a WheelCollider
// component by its centre minus its radius minus 0.15. The lander is placed landed (PRELAUNCH) and the host moves it to the ground: its foot
// colliders (the piston colliders ModuleWheelSuspension disables are enabled for the measure) rest within a few centimetres of the ground,
// where the first port of the C# (no collider roles, no disabled colliders) left the tyre bottoms 0.53 m below it.
TEST_CASE("WP-29d landed placement: the lander is put on the ground by its foot colliders (getLowestPoint), the tyre bottom a few centimetres below it") {
    if (!wp20b::GameAvailable()) return;
    GearRun run("Two-Stage Lander", 0.0, 0.0);
    for (int k = 0; k < kWheelsUp + 4; ++k) run.Tick();
    const double low = run.LowestTyreBottom();
    MESSAGE("landed placement: tyre bottom at full extension " << low << " m from the ground, speed " << run.Vel().magnitude());
    CHECK(low < 0.0);
    CHECK(low > -0.15);
    CHECK(run.Wheels().GroundedCount() == 4);
}


// WP-21b: WP-29d reported an "attitude jump" of the Rocket-power VTOL (its wheel colliders rotating ~25 degrees in the first ticks while the cluster
// barely rotates). Measured: the VESSEL does not rotate (the root part's rotation moves by < 0.01 degrees over the first 40 ticks, airborne and on
// the ground); the wheel colliders' part-space pose changes ONCE, between tick 0 and tick 1, by the gear-bay deployment pose: the LY-01 model's
// collider node is posed by the Animation sample ModuleWheelDeployment.OnStart applies (the deployed state; the prefab pose is the stowed one), 5.0
// degrees and 0.1 m for all four bays. The pose is then constant, in the part frame and in the world.
TEST_CASE("WP-21b Rocket-power VTOL spawn: the vessel keeps its attitude, the wheel colliders take the deployed gear pose once (tick 0 -> 1)") {
    if (!wp20b::GameAvailable()) return;
    for (const bool airborne : {true, false}) {
        GearRun run("Rocket-power VTOL", airborne ? 3000.0 : 0.0, 0.0, RealGeometry(), 0.0, airborne);
        Vessel& v = run.V();
        const Quaternionf rot0 = v.vesselTransform.rotation;
        const auto angleDeg = [](const Quaternionf& a, const Quaternionf& b) {  // the angle of a^-1 b: 2 atan2(|vector part|, |w|) (exact for tiny angles)
            const double aw = a.w, ax = -static_cast<double>(a.x), ay = -static_cast<double>(a.y), az = -static_cast<double>(a.z);
            const double w = aw * b.w - ax * b.x - ay * b.y - az * b.z;
            const double x = aw * b.x + ax * b.w + ay * b.z - az * b.y;
            const double y = aw * b.y - ax * b.z + ay * b.w + az * b.x;
            const double z = aw * b.z + ax * b.y - ay * b.x + az * b.w;
            return 2.0 * std::atan2(std::sqrt(x * x + y * y + z * z), std::fabs(w)) * 180.0 / 3.14159265358979323846;
        };
        std::vector<Vector3d> up0;
        double maxVessel = 0.0, changeAt1 = 0.0, changeAfter1 = 0.0, shiftAt1 = 0.0;
        std::vector<Vector3d> pos0;
        for (int k = 0; k < 40; ++k) {
            run.Tick();
            maxVessel = std::max(maxVessel, angleDeg(rot0, v.vesselTransform.rotation));
            size_t i = 0;
            for (auto& w : run.Wheels().Wheels()) {
                const Vector3d u(w->geometry.rotation * Vector3f(0, 1, 0));
                const Vector3d p(w->geometry.position);
                if (k == 0) {
                    up0.push_back(u);
                    pos0.push_back(p);
                } else {
                    const double d = std::acos(std::clamp(Vector3d::Dot(u, up0[i]), -1.0, 1.0)) * 180.0 / 3.14159265358979323846;
                    if (k == 1) {
                        changeAt1 = std::max(changeAt1, d);
                        shiftAt1 = std::max(shiftAt1, (p - pos0[i]).magnitude());
                        up0[i] = u;  // the reference for the later ticks is the deployed pose
                    } else {
                        changeAfter1 = std::max(changeAfter1, d);
                    }
                }
                ++i;
            }
        }
        MESSAGE(std::string(airborne ? "airborne" : "on the ground") << ": vessel rotation over 40 ticks " << maxVessel << " deg; wheel collider pose tick 0 -> 1: " << changeAt1 << " deg, " << shiftAt1 << " m; ticks 2-39: "
                                                      << changeAfter1 << " deg");
        CHECK(maxVessel < 0.05);
        CHECK(changeAt1 > 4.9);
        CHECK(changeAt1 < 5.1);
        CHECK(shiftAt1 > 0.05);
        CHECK(changeAfter1 < 0.05);
    }
}

// ---- WP-29e: getLowestPoint's wheel scan (Vessel.cs:2589-2637) ----------------------------------------------------------------------------------
// The wheel parts' share of the scan against the independent float64 reference tools/wp29e/lowest_point_reference.py (tests/data/wp29e_lowest_point_reference.json):
// per stock wheel / gear / leg model, a random world pose, the gear state (ModuleWheelDeployment's stateString / position), the damaged wheel and the hidden
// wheel; the reference lists every collider getLowestPoint reads (z = bounds.min.z for the layers 26 / 27 / 30, the role from the node name, the WheelCollider
// chunk's radius). The parts are those of KSP's own stock crafts (the model of each type comes from the craft that carries it).
#include <fstream>
#include <map>

#include "nlohmann/json.hpp"

namespace {
const nlohmann::json& LowestRef() {
    static const nlohmann::json ref = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp29e_lowest_point_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return nlohmann::json::parse(f);
    }();
    return ref;
}
}  // namespace

TEST_CASE("WP-29e getLowestPoint's wheel scan: the collider z, roles and radii of every stock wheel / gear / leg model vs the independent float64 reference") {
    if (!wp20b::GameAvailable()) return;
    using nlohmann::json;
    const json& ref = LowestRef();
    // the craft that carries each part type (one World alive at a time: the scene keeps global state)
    const std::vector<std::pair<std::string, std::vector<std::string>>> crafts = {
        {"Prospector Rover", {"roverWheel1"}}, {"Bug-E Buggy", {"roverWheel2"}},    {"Crater Crawler", {"wheelMed"}}, {"Two-Stage Lander", {"landingLeg1-2"}},
        {"PT Series Munsplorer", {"landingLeg1"}}, {"Aeris 3A", {"SmallGearBay"}}, {"Mallard", {"GearMedium"}},      {"Albatross 3", {"GearSmall", "GearFree"}}};
    std::map<std::string, int> compared;
    double maxZ = 0.0, maxRadius = 0.0;
    size_t points = 0, wheelRole = 0, enhancerRole = 0;
    struct P {
        double z;
        int role;
        double radius;
    };
    for (const auto& cr : crafts) {
        GearRun run(cr.first, 3000.0, 0.0, RealGeometry(), 0.0, true);
        run.Tick();  // the wheel records exist, the wheels are not set up yet (the placement's situation: ModuleWheelBase.wheelSetup waits for an unpacked part)
        for (const json& c : ref["cases"]) {
            const std::string type = c["part"].get<std::string>();
            if (std::find(cr.second.begin(), cr.second.end(), type) == cr.second.end()) continue;
            Part* part = nullptr;
            for (Part* p : run.V().parts)
                if (p->partInfo != nullptr && p->partInfo->name == type) {
                    part = p;
                    break;
                }
            REQUIRE(part != nullptr);
            INFO(type);
            CHECK(part->fields.rescaleFactor == doctest::Approx(c["rescale"].get<double>()));
            // the case's world pose, gear state, damage and hidden wheel
            const json& pp = c["partPose"];
            part->partTransform.position = Vector3f(pp[0].get<float>(), pp[1].get<float>(), pp[2].get<float>());
            part->partTransform.rotation = Quaternionf(pp[3].get<float>(), pp[4].get<float>(), pp[5].get<float>(), pp[6].get<float>());
            if (PartModule* m = part->FindModuleOfClass("ModuleWheelDeployment")) {
                m->Set<std::string>("stateString", c["stateString"].get<std::string>());
                static_cast<ModuleWheels::ModuleWheelDeployment*>(m)->position = c["position"].get<float>();
            }
            if (PartModule* m = part->FindModuleOfClass("ModuleWheelDamage")) m->Set<bool>("isDamaged", c["damaged"].get<bool>());
            WheelPart* rec = run.Wheels().Find(*part);
            REQUIRE(rec != nullptr);
            CHECK(!rec->base->IsWheelSetUp());
            rec->wheelHidden = c["hidden"].get<bool>();
            const std::vector<VesselEnvironment::ColliderPoint> got = run.scene->wheels.PlacementPoints(*part, Vector3f());
            const auto& exp = c["expect"];
            REQUIRE(got.size() == exp.size());
            // order-free: sort both by (z, role, radius); VesselEnvironment::ColliderRole: Plain 0, WheelCollider 1, CollisionEnhancer 2
            auto roleId = [](const std::string& s) { return s == "wheel" ? 1 : (s == "enhancer" ? 2 : 0); };
            std::vector<P> a, b;
            for (const auto& g : got) a.push_back({g.z, static_cast<int>(g.role), g.radius});
            for (const auto& e : exp) b.push_back({e["z"].get<double>(), roleId(e["role"].get<std::string>()), e["radius"].get<double>()});
            auto cmp = [](const P& x, const P& y) { return x.z != y.z ? x.z < y.z : (x.role != y.role ? x.role < y.role : x.radius < y.radius); };
            std::sort(a.begin(), a.end(), cmp);
            std::sort(b.begin(), b.end(), cmp);
            for (size_t i = 0; i < a.size(); ++i) {
                CHECK(a[i].role == b[i].role);
                maxZ = std::max(maxZ, std::fabs(a[i].z - b[i].z));
                maxRadius = std::max(maxRadius, std::fabs(a[i].radius - b[i].radius));
                ++points;
                wheelRole += a[i].role == 1;
                enhancerRole += a[i].role == 2;
            }
            compared[type]++;
        }
    }
    MESSAGE("compared " << points << " collider points (" << wheelRole << " WheelCollider role, " << enhancerRole << " collisionEnhancer role) of " << compared.size() << " part types; max |z| " << maxZ
                        << " m, max |radius| " << maxRadius << " m");
    CHECK(compared.size() == 9);
    CHECK(points > 200);
    CHECK(wheelRole > 40);
    CHECK(enhancerRole > 20);
    CHECK(maxZ < 2.0e-5);       // float32 corner / endpoint maths vs the float64 extremes, poses of up to 8 m
    CHECK(maxRadius < 1.0e-6);
}

// Vessel.getLowestPoint's correction of the lowest collider (Vessel.cs:2616-2637), the statements on their own: a collider on a node whose name contains
// "WheelCollider" has its radius and then 0.15 subtracted from `bounds.min.z`, one named "collisionEnhancer" 0.15, any other nothing. A probe collider far
// below the Two-Stage Lander (z = -1000 in the frame the vessel was rotated into) is lowest in all three runs, so the result differs by exactly the correction.
TEST_CASE("WP-29e Vessel.getLowestPoint: the wheel collider branch subtracts radius + 0.15, the collisionEnhancer branch 0.15, a plain collider nothing") {
    if (!wp20b::GameAvailable()) return;
    GearRun run("Two-Stage Lander");
    run.Tick();   // unpacked and placed: CheckGroundCollision has set the ground normal getLowestPoint rotates the vessel into
    REQUIRE(run.Vp() != nullptr);
    const auto base = Vessel::env.colliderClosestPoints;
    REQUIRE(static_cast<bool>(base));
    auto lowest = [&](VesselEnvironment::ColliderRole role, float radius) {
        Part* root = run.V().rootPart;
        Vessel::env.colliderClosestPoints = [&, role, radius](Part& p, const Vector3f& point) {
            std::vector<VesselEnvironment::ColliderPoint> v = base(p, point);
            if (&p == root) v.push_back({-1000.0f, role, radius});
            return v;
        };
        const float r = run.V().getLowestPoint();
        Vessel::env.colliderClosestPoints = base;
        return r;
    };
    const float plain = lowest(VesselEnvironment::ColliderRole::Plain, 0.0f);
    const float enhancer = lowest(VesselEnvironment::ColliderRole::CollisionEnhancer, 0.0f);
    const float wheel = lowest(VesselEnvironment::ColliderRole::WheelCollider, 0.3f);
    const float wheelNoRadius = lowest(VesselEnvironment::ColliderRole::WheelCollider, 0.0f);   // a "...WheelCollider..." node without a WheelCollider component
    MESSAGE("getLowestPoint with a probe 1000 m below: plain " << plain << ", collisionEnhancer " << enhancer << " (+" << enhancer - plain << "), WheelCollider r 0.3 " << wheel << " (+" << wheel - plain
                                                                << "), WheelCollider-named without a component " << wheelNoRadius << " (+" << wheelNoRadius - plain << ")");
    CHECK(plain > 900.0f);
    CHECK(enhancer - plain == doctest::Approx(0.15).epsilon(0.002));
    CHECK(wheel - plain == doctest::Approx(0.45).epsilon(0.002));
    CHECK(wheelNoRadius - plain == doctest::Approx(0.15).epsilon(0.002));
    // and the placement of the lander itself: its legs are not WheelColliders (the LT-2 has no chunk), the lowest collider is a foot piston: the origin
    // is as high above the ground as getLowestPoint says
    const Vector3d org = Vector3d(run.V().vesselTransform.position);
    const PhysicsWorld::RayHit h = run.scene->pad.s.world.RayCast(run.scene->pad.s.bubble.StaticPhysicsFromWorld(org + run.Up() * 50.0), run.Up() * -1.0, 200.0, PhysicsWorld::kLayerStatic);
    REQUIRE(h.hit);
    const float gl = run.V().getLowestPoint();
    MESSAGE("lander: getLowestPoint " << gl << " m, the origin is " << h.distance - 50.0 << " m above the ground");
    CHECK(std::fabs(gl - (h.distance - 50.0)) < 0.002);
}

// After the set-up (ModuleWheelBase.wheelSetup) `GetComponent<VPWheelCollider>()` exists: getLowestPoint subtracts ITS radius (the cfg radius x rescaleFactor)
// and the WheelCollider's bounds centre is the native collider's center under the wheel collider transform.
TEST_CASE("WP-29e getLowestPoint after the wheel set-up: the VPWheelCollider's radius and centre") {
    if (!wp20b::GameAvailable()) return;
    GearRun run("Prospector Rover", 3000.0, 0.0, RealGeometry(), 0.0, true);
    for (int k = 0; k < 8; ++k) run.Tick();
    REQUIRE(run.Wheels().SetUpCount() == 4);
    size_t checked = 0;
    for (auto& w : run.Wheels().Wheels()) {
        const std::vector<VesselEnvironment::ColliderPoint> pts = run.scene->wheels.PlacementPoints(*w->part, Vector3f());
        const VehiclePhysics::VPWheelCollider& vc = *w->Controller()->wheelCollider;
        const float expectRadius = w->base->Get<float>("radius") * static_cast<float>(w->part->fields.rescaleFactor);
        const float expectZ = w->colliderHost.TransformPoint(vc.wheelColliderData.center).z;
        int found = 0;
        for (const auto& p : pts)
            if (p.role == VesselEnvironment::ColliderRole::WheelCollider) {
                ++found;
                CHECK(p.radius == doctest::Approx(expectRadius).epsilon(1e-6));
                CHECK(p.z == doctest::Approx(expectZ).epsilon(1e-6));
                CHECK(p.radius == vc.radius);
            }
        CHECK(found == 1);
        ++checked;
    }
    CHECK(checked == 4);
}
