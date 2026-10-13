// WP-20e scenarios: contact, landing and water through the whole flight tick (physics bubble + FlightIntegrator + contact host).
// See STATUS.md WP-20e. Fixtures: tests/contact_fixture.hpp (over the WP-20d pad scene). No golden or capture covers Jolt contacts; each
// scenario names its reference (KSP's rule evaluated on the reported contact, an analytic equilibrium, the scene's own geometry).
#include <algorithm>
#include <cmath>
#include <tuple>

#include "contact_fixture.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp20e;

namespace {
constexpr double kPadLat = wp20b::kPadLat, kPadLon = wp20b::kPadLon;

struct PadRun {
    int anchoredAt = -1;
    double maxSpeedAnchored = 0.0, maxDriftAnchored = 0.0;   // after anchoring (m/s, m)
    double maxSpeedLate = 0.0, maxDriftLate = 0.0;           // ticks 200-300 whatever the anchor
    uint64_t enters = 0;
    size_t groundContacts = 0;
    bool landed = false;
    Vessel::Situations situation = Vessel::Situations::PRELAUNCH;
    float anchorForce = 0.0f;
    double weight = 0.0;
    int anchorResets = 0;
};

PadRun RunPad(const std::string& craft, bool anchor, int ticks = 300) {
    PadRun r;
    AnchorSettings settings;
    World w(true);
    PhysicsBubbleOptions o;
    o.contacts.rbAnchor = anchor;
    PadScene pad(w, o);
    auto v = w.Craft(craft);
    pad.Place(*v);
    pad.s.Start(*v);
    ResetLevelClock();
    ContactHost& ch = pad.s.bubble.Contacts();
    Vector3d comAnchored, comLate;
    bool had = false;
    for (int k = 0; k < ticks; ++k) {
        Tick(pad);
        VesselPhysics* vp = pad.s.bubble.Host().Find(*v);
        if (vp == nullptr) continue;
        const VesselPhysicsState st = vp->ReadState();
        if (v->HasRBAnchor() && r.anchoredAt < 0) {
            r.anchoredAt = k;
            comAnchored = st.CoMD;
        }
        if (had && !v->HasRBAnchor()) ++r.anchorResets;
        had = v->HasRBAnchor();
        if (r.anchoredAt >= 0) {
            r.maxSpeedAnchored = std::max(r.maxSpeedAnchored, st.rb_velocityD.magnitude());
            r.maxDriftAnchored = std::max(r.maxDriftAnchored, (st.CoMD - comAnchored).magnitude());
        }
        if (k == 200) comLate = st.CoMD;
        if (k > 200) {
            r.maxSpeedLate = std::max(r.maxSpeedLate, st.rb_velocityD.magnitude());
            r.maxDriftLate = std::max(r.maxDriftLate, (st.CoMD - comLate).magnitude());
        }
    }
    r.enters = ch.EnterCount();
    r.groundContacts = v->GroundContacts.size();
    r.landed = v->Landed;
    r.situation = v->situation;
    r.anchorForce = ch.AnchorForce(*v).magnitude();
    r.weight = v->totalMass * v->graviticAcceleration.magnitude();
    return r;
}
}  // namespace

TEST_CASE("WP-20e pad: the contacts keep the craft landed, the RB anchor (KSP's 1.5 s rule) ends the pad rocking (S, M2-AS, L2-AS)") {
    if (!wp20b::GameAvailable()) return;
    for (const char* craft : {"PhysProf-S", "PhysProf-M2-AS", "PhysProf-L2-AS"}) {
        const PadRun a = RunPad(craft, true);
        const PadRun n = RunPad(craft, false);
        MESSAGE(std::string(craft) << ": " << a.enters << " contact enters, " << a.groundContacts << " ground-contact parts, landed " << a.landed << "; anchored at tick "
                      << a.anchoredAt << " (" << a.anchorResets << " resets), anchor force " << a.anchorForce << " kN vs weight " << a.weight
                      << " kN; after anchoring max speed " << a.maxSpeedAnchored << " m/s, drift " << a.maxDriftAnchored << " m; without the anchor (ticks 200-300) "
                      << n.maxSpeedLate << " m/s, " << n.maxDriftLate << " m");
        CHECK(a.enters > 0);
        CHECK(a.groundContacts > 0);
        CHECK(a.landed);
        CHECK(a.situation == Vessel::Situations::PRELAUNCH);
        // the anchor: KSP's rule (Vessel.cs:1159): landed, unpacked, not easing, v^2 < 0.05, 1.5 s -> 16 easing ticks + 75 ticks after them
        CHECK(a.anchoredAt > 0);
        CHECK(a.anchoredAt < 120);
        CHECK(a.anchorResets == 0);
        CHECK(std::fabs(a.anchorForce - a.weight) < 0.01 * a.weight);  // the anchor carries the support (see contact_host.cpp)
        CHECK(a.maxSpeedAnchored < 1e-4);
        CHECK(a.maxDriftAnchored < 1e-4);
    }
}

TEST_CASE("WP-20e launch: ignition releases the anchor of the active vessel, liftoff ends the ground contact (PRELAUNCH -> FLYING)") {
    if (!wp20b::GameAvailable()) return;
    AnchorSettings settings;
    World w(true);
    PadScene pad(w);
    EventLog log;
    auto v = w.Craft("PhysProf-S");
    pad.Place(*v);
    pad.s.Start(*v);
    v->MakeActive();
    ResetLevelClock();
    for (int k = 0; k < 120; ++k) Tick(pad);
    REQUIRE(v->HasRBAnchor());
    REQUIRE(v->situation == Vessel::Situations::PRELAUNCH);
    wp20d::IgniteFirstStage(*v);
    Tick(pad);
    CHECK_FALSE(v->HasRBAnchor());  // Vessel.cs:1125: ACTIVE and mainThrottle > 0
    int liftoff = -1;
    for (int k = 0; k < 300; ++k) {
        Tick(pad);
        if (liftoff < 0 && !v->Landed) liftoff = k;
    }
    const ContactHost& ch = pad.s.bubble.Contacts();
    MESSAGE("liftoff (not Landed) " << liftoff << " ticks after ignition; exits " << ch.ExitCount() << ", situation changes " << log.situations.size()
                                    << ", altitude " << pad.RootAltitude(*v) << " m, crashes " << log.crashes.size());
    CHECK(liftoff > 0);
    CHECK(ch.ExitCount() > 0);
    CHECK(v->GroundContacts.empty());
    CHECK(v->situation == Vessel::Situations::FLYING);
    REQUIRE_FALSE(log.situations.empty());
    CHECK(log.situations.back().first == Vessel::Situations::PRELAUNCH);
    CHECK(log.situations.back().second == Vessel::Situations::FLYING);
    CHECK(log.crashes.empty());
    CHECK(log.explosions == 0);
}

namespace {
// One probeStackLarge (the root part of PhysProf-S: crashTolerance 9) released `clearance` above a site with a speed `v0` towards the ground
struct DropRun {
    std::vector<float> impacts;  // |Collision.relativeVelocity| of the part's Enter messages
    bool exploded = false, landed = false, splashed = false, anchored = false, enhancerHit = false;
    Vessel::Situations situation = Vessel::Situations::FLYING;
    std::string landedAt;
    double endSpeed = 0.0;
    double minHeight = 1e30;  // lowest root height above the site's surface over the run
    int explosions = 0, crashes = 0, splashCrashes = 0, groundExplosions = 0;
    uint64_t hash = 0;
    double depth = 0.0, submerged = 0.0;
    Quaternionf rotation;
    std::vector<std::pair<Vessel_Situations, Vessel_Situations>> situations;
    // physics-warp substeps (Drop's `substeps` > 1)
    int rawRootEnters = 0;       // the root's Enter events over every world substep (PhysicsWorld::ContactEvents after each)
    int lastStepRootEnters = 0;  // of those, the ones in a tick's last substep (all a last-step-only dispatch would have seen)
    int dispatchedRootEnters = 0;
    int stayDuplicates = 0;      // OnCollisionStay sent twice in one FixedUpdate for one collider pair
    size_t maxStepsPerDispatch = 0;
};

DropRun Drop(double lat, double lon, bool sea, double clearance, double v0, int ticks, const std::string& tag = "", int threads = 1,
             int substeps = 1) {
    DropRun r;
    AnchorSettings settings;
    World w(true);
    PhysicsWorldConfig cfg;
    cfg.threads = threads;
    PadScene pad(w, PhysicsBubbleOptions{}, cfg);
    if (!tag.empty()) {
        StaticColliderInfo info;
        info.tag = tag;  // FIXTURE: the KSC pad collider's tag (KSC scenery colliders are not extracted, WP-12b "Needs data" 2)
        info.name = tag;
        pad.s.bubble.Contacts().RegisterStaticRange(0, 900, info);
    }
    GameSettings::PHYSICS_EASE = false;  // the release speed is the impact speed's start, without the 16-tick gravity ramp
    EventLog log;
    auto v = w.CraftHead("PhysProf-S", 1);
    const Site site = SiteAt(pad, lat, lon, sea);
    PlaceAt(pad, *v, site, clearance, false);
    pad.s.Start(*v);
    v->MakeActive();
    ResetLevelClock();
    ContactHost& ch = pad.s.bubble.Contacts();
    ch.recordDispatch = true;
    REQUIRE(TickUntilUnpacked(pad, *v));
    VesselPhysics* vp = pad.s.bubble.Host().Find(*v);
    REQUIRE(vp != nullptr);
    vp->SetLinearVelocity(site.up * -v0);
    Part* root = v->rootPart;
    const float savedFixed = Time::fixedDeltaTime, savedDelta = Time::deltaTime;
    if (substeps > 1) {
        // physics warp (TimeWarp.cs:423: Time.fixedDeltaTime = 0.02 x rate) with WP-29c's 0.02 s substeps through PhysicsBubble::physicsStep, as
        // WheelPhysicsHost::PhysicsStep runs them for a wheeled vessel (src/physics/vessel_wheels.cpp: the tick's buffered forces held over
        // every substep); the raw events of every substep are counted here
        pad.s.tw.Mode = TimeWarp::Modes::LOW;
        pad.s.tw.curr_rate = static_cast<float>(substeps);
        Time::fixedDeltaTime = 0.02f * static_cast<float>(substeps);
        Time::deltaTime = Time::fixedDeltaTime;
        w.planetarium.fixedDeltaTime = static_cast<double>(Time::fixedDeltaTime);
        pad.s.bubble.physicsStep = [&, substeps](float dt) {
            VesselPhysicsHost& host = pad.s.bubble.Host();
            const float h = dt / static_cast<float>(substeps);
            std::vector<std::pair<VesselPhysics*, VesselPhysics::BufferedForces>> held;
            for (const auto& b : host.Bindings()) held.emplace_back(b.get(), b->PeekBufferedForces());
            for (int k = 0; k < substeps; ++k) {
                if (k > 0)
                    for (auto& [hp, f] : held)
                        for (const auto& b : host.Bindings())
                            if (b.get() == hp) hp->AddBufferedForces(f);
                host.Step(h);
                VesselPhysics* b = host.Find(*v);
                if (b == nullptr) continue;
                for (const PhysicsWorld::ContactEvent& e : pad.s.world.ContactEvents())
                    if (e.kind == PhysicsWorld::ContactEvent::Kind::Enter && e.otherStatic && b->ClusterOfBody(e.body1) >= 0 && b->PartAt(e.sub1) == root) {
                        ++r.rawRootEnters;
                        if (k == substeps - 1) ++r.lastStepRootEnters;
                    }
            }
        };
    }
    for (int k = 0; k < ticks; ++k) {
        Tick(pad);
        r.maxStepsPerDispatch = std::max(r.maxStepsPerDispatch, ch.SubstepsDispatched());
        std::vector<std::tuple<const Part*, const Part*, uint64_t>> stays;
        for (const ContactHost::DispatchRecord& d : ch.LastDispatch()) {
            if (d.kind == PhysicsWorld::ContactEvent::Kind::Enter && d.receiver == root) ++r.dispatchedRootEnters;
            if (d.kind != PhysicsWorld::ContactEvent::Kind::Stay) continue;
            const auto key = std::make_tuple(static_cast<const Part*>(d.receiver), static_cast<const Part*>(d.colliderPart), d.collision.collider.id);
            if (std::find(stays.begin(), stays.end(), key) != stays.end()) ++r.stayDuplicates;
            stays.push_back(key);
        }
        for (float s : ImpactSpeeds(ch, root)) r.impacts.push_back(s);
        if (CollisionEnhancer* e = ch.Enhancer(*root)) r.enhancerHit = r.enhancerHit || e->lastHit;
        if (root->State() == PartStates::DEAD) break;
        r.minHeight = std::min(r.minHeight, (Vector3d(root->partTransform.position) - w.Kerbin().position).magnitude() - site.surfaceRadius);
    }
    if (substeps > 1) {
        pad.s.bubble.physicsStep = nullptr;
        pad.s.tw.Mode = TimeWarp::Modes::HIGH;
        pad.s.tw.curr_rate = 1.0f;
        Time::fixedDeltaTime = savedFixed;
        Time::deltaTime = savedDelta;
        w.planetarium.fixedDeltaTime = static_cast<double>(savedFixed);
    }
    r.exploded = root->State() == PartStates::DEAD;
    r.landed = v->Landed;
    r.splashed = v->Splashed;
    r.anchored = v->HasRBAnchor();
    r.situation = v->situation;
    r.landedAt = v->landedAt;
    r.endSpeed = r.exploded ? 0.0 : VesselVelocity(pad, *v).magnitude();
    r.explosions = log.explosions;
    r.crashes = static_cast<int>(log.crashes.size());
    r.splashCrashes = static_cast<int>(log.splashCrashes.size());
    r.groundExplosions = static_cast<int>(log.groundExplosions.size());
    r.hash = pad.s.world.StateHash();
    r.depth = root->fields.depth;
    r.submerged = root->fields.submergedPortion;
    r.rotation = root->partTransform.rotation;
    r.situations = log.situations;
    return r;
}
}  // namespace

TEST_CASE("WP-20e crash tolerance: the part hits the ground at 0.9x / 1.1x its crashTolerance: survives and lands / explodes (Part.HandleCollision)") {
    if (!wp20b::GameAvailable()) return;
    const double tol = 9.0;  // probeStackLarge.cfg crashTolerance
    const double g = 9.80;   // Kerbin at the pad, for the release speed only
    const double d = 0.30;
    for (const double f : {0.9, 1.1}) {
        const double vt = f * tol;
        const double v0 = std::sqrt(vt * vt - 2.0 * g * d);
        const DropRun r = Drop(kPadLat, kPadLon, false, d, v0, 300);
        REQUIRE_FALSE(r.impacts.empty());
        MESSAGE(f << " x crashTolerance: target " << vt << " m/s, reported impact " << r.impacts.front() << " m/s; exploded " << r.exploded << " (explosions "
                  << r.explosions << ", crash events " << r.crashes << "), landed " << r.landed << ", situation " << static_cast<int>(r.situation)
                  << ", anchored " << r.anchored << ", end speed " << r.endSpeed << " m/s, lowest root height " << r.minHeight << " m, landedAt '" << r.landedAt << "'");
        CHECK(std::fabs(r.impacts.front() - vt) < 0.03 * vt);  // the impact KSP's rule saw is the one aimed at
        CHECK(r.exploded == (f > 1.0));
        CHECK(r.exploded == (r.impacts.front() > tol));  // HandleCollision's own threshold (the reference covers the rule bit for bit)
        if (f < 1.0) {
            CHECK(r.landed);
            CHECK(r.situation == Vessel::Situations::LANDED);
            CHECK(r.landedAt.empty());  // untagged terrain (PQS quads are "Untagged")
            CHECK(r.endSpeed < 1e-3);
            CHECK(r.anchored);
            CHECK(r.explosions == 0);
            REQUIRE_FALSE(r.situations.empty());
            CHECK(r.situations.back().second == Vessel::Situations::LANDED);
        } else {
            CHECK(r.explosions == 1);
            CHECK(r.crashes == 1);
        }
    }
}

TEST_CASE("WP-20e landedAt: a tagged static collider names the landing site (Part.LandedCollisionChecks -> Vessel.SetLandedAt)") {
    if (!wp20b::GameAvailable()) return;
    const DropRun r = Drop(kPadLat, kPadLon, false, 0.2, 1.0, 150, "LaunchPad");
    MESSAGE("landedAt " << r.landedAt << ", landed " << r.landed);
    CHECK(r.landed);
    CHECK(r.landedAt == "LaunchPad");
}

namespace {
// A point of Kerbin's ocean east of the KSC: the first longitude (0.25 deg steps from the pad) whose PQS terrain is 50 m below sea level
bool OceanSite(World& w, double& lat, double& lon) {
    lat = kPadLat;
    for (double l = kPadLon + 0.5; l < kPadLon + 20.0; l += 0.25) {
        const Vector3d rel = terrain::GetRelSurfaceNVector(lat, l).normalized();
        if (w.pqs->GetSurfaceHeight(rel, true) < w.Kerbin().Radius - 50.0) {
            lon = l;
            return true;
        }
    }
    return false;
}

// KSP's buoyancy equilibrium for the part's settled attitude (PartBuoyancy.cs:171-229, 497-534 with the part's own cube data, independent of
// the port): the draft (depth of the lowest cube corner) at which the buoyant force equals the part's weight in the rotating frame
double EquilibriumDraft(Part& p, double mass, double gUp, double gBuoy, const Quaternionf& rot) {
    const DragCubeList& dc = p.DragCubes;
    const Vector3f sz = dc.WeightedSize(), c = dc.WeightedCenter();
    const std::array<float, 6>& a = dc.WeightedArea();
    const double xp = a[0] / (double(sz.y) * sz.z), yp = a[2] / (double(sz.x) * sz.z), zp = a[4] / (double(sz.x) * sz.y);
    const double xz = (std::min(xp, zp) + 2.0 * xp * zp) / 3.0;
    const double V = double(sz.x) * sz.y * sz.z * xz * yp;
    const double target = mass * gUp / (V * 1.0 /* oceanDensity */ * PhysicsGlobals::BuoyancyScalar() * p.fields.buoyancy * gBuoy);
    // corner heights above the centre of displacement (the part origin) along the local up, part attitude `rot` (craft up = world up here)
    double lo = 1e30, hi = -1e30;
    const QuaternionD q(rot);
    for (int i = 0; i < 8; ++i) {
        const Vector3d corner((i & 4) ? c.x + 0.5 * sz.x : c.x - 0.5 * sz.x, (i & 2) ? c.y + 0.5 * sz.y : c.y - 0.5 * sz.y,
                              (i & 1) ? c.z + 0.5 * sz.z : c.z - 0.5 * sz.z);
        const double up = (q * corner).y;  // the world up of the craft frame is +y of the attitude given (relative attitude)
        lo = std::min(lo, up);
        hi = std::max(hi, up);
    }
    // depth d of the centre of displacement: maxDepth = d - lo, minDepth = d - hi
    auto portion = [&](double d) {
        const double maxD = d - lo, minD = d - hi;
        if (!(minD < 0.0 && minD < maxD)) return 1.0;
        double s = (d <= 0.0 && d != maxD) ? maxD / (maxD - d) * 0.5 : 0.5 + d / (d - minD) * 0.5;
        s = std::sin(s * 3.14159265358979323846 - 3.14159265358979323846 / 2.0) * 0.5 + 0.5;
        const double ramp = maxD < 0.2 ? maxD / 0.2 : 1.0;
        return s * ramp;
    };
    double a0 = lo, b0 = hi;  // portion(a0) = 0 .. portion(b0) = 1
    for (int it = 0; it < 200; ++it) {
        const double m = 0.5 * (a0 + b0);
        if (portion(m) < target) a0 = m;
        else b0 = m;
    }
    return 0.5 * (a0 + b0) - lo;
}
}  // namespace

TEST_CASE("WP-20e splashdown: a part dropped into Kerbin's ocean floats at KSP's buoyancy equilibrium (within 2 %), SPLASHED; a fast one crashes") {
    if (!wp20b::GameAvailable()) return;
    double lat = 0.0, lon = 0.0;
    {
        World probe(true);
        REQUIRE(probe.Kerbin().ocean);
        REQUIRE(OceanSite(probe, lat, lon));
    }
    // gentle: 2 m/s from 1 m
    {
        AnchorSettings settings;
        World w(true);
        PadScene pad(w);
        GameSettings::PHYSICS_EASE = false;
        EventLog log;
        auto v = w.CraftHead("PhysProf-S", 1);
        const Site site = SiteAt(pad, lat, lon, true);
        PlaceAt(pad, *v, site, 1.0, false);
        pad.s.Start(*v);
        v->MakeActive();
        ResetLevelClock();
        REQUIRE(TickUntilUnpacked(pad, *v));
        VesselPhysics* vp = pad.s.bubble.Host().Find(*v);
        vp->SetLinearVelocity(site.up * -2.0);
        Part* root = v->rootPart;
        double draftSum = 0.0, speedMax = 0.0;
        int n = 0;
        const int ticks = 1000;
        for (int k = 0; k < ticks; ++k) {
            Tick(pad);
            if (k >= ticks - 100) {
                draftSum += root->fields.maxDepth;
                ++n;
                speedMax = std::max(speedMax, VesselVelocity(pad, *v).magnitude());
            }
        }
        const double draft = draftSum / n;
        const CelestialBody& k = w.Kerbin();
        const Vector3d com = vp->ReadState().CoMD;
        const Vector3d up = (com - k.position).normalized();
        const double gBuoy = -Vector3d::Dot(FlightGlobals::getGeeForceAtPosition(com, k), up);
        const double gUp = gBuoy - Vector3d::Dot(FlightGlobals::getCentrifugalAcc(com, k), up);
        // the part's attitude relative to the site's frame (craft x, y, z = east, up, north)
        const QuaternionD siteRot = wp20b::BasisRotation(site.east, site.up, site.north);
        const Quaternionf rel = (QuaternionD::Inverse(siteRot) * QuaternionD(root->partTransform.rotation)).ToQuaternionf();
        const double expected = EquilibriumDraft(*root, vp->PartMass(*root), gUp, gBuoy, rel);
        MESSAGE("splashdown at lon " << lon << ": draft (lowest cube corner depth) " << draft << " m vs KSP's equilibrium " << expected << " m ("
                                     << (draft - expected) / expected * 100.0 << " %), submerged portion " << root->fields.submergedPortion << ", max speed (last 2 s) "
                                     << speedMax << " m/s, situation " << static_cast<int>(v->situation) << ", WaterContact " << root->fields.WaterContact);
        CHECK(root->State() != PartStates::DEAD);
        CHECK(v->Splashed);
        CHECK_FALSE(v->Landed);
        CHECK(v->situation == Vessel::Situations::SPLASHED);
        CHECK(root->fields.WaterContact);
        CHECK(log.splashCrashes.empty());
        CHECK(std::fabs(draft - expected) < 0.02 * expected);
        CHECK(speedMax < 0.05);
    }
    // fast: 20 m/s into the water: max(BuoyancyMinCrashMult |v|, v_down) > crashTolerance * BuoyancyCrashToleranceMult = 10.8 m/s
    {
        AnchorSettings settings;
        World w(true);
        PadScene pad(w);
        GameSettings::PHYSICS_EASE = false;
        EventLog log;
        auto v = w.CraftHead("PhysProf-S", 1);
        const Site site = SiteAt(pad, lat, lon, true);
        PlaceAt(pad, *v, site, 1.0, false);
        pad.s.Start(*v);
        v->MakeActive();
        ResetLevelClock();
        REQUIRE(TickUntilUnpacked(pad, *v));
        pad.s.bubble.Host().Find(*v)->SetLinearVelocity(site.up * -20.0);
        Part* root = v->rootPart;
        for (int k = 0; k < 50 && root->State() != PartStates::DEAD; ++k) Tick(pad);
        MESSAGE("20 m/s splashdown: onCrashSplashdown " << log.splashCrashes.size() << ", part dead " << (root->State() == PartStates::DEAD));
        CHECK(log.splashCrashes.size() == 1);
        CHECK(root->State() == PartStates::DEAD);
    }
}

TEST_CASE("WP-20e high-speed crash: 80 m/s into the terrain, the part explodes and nothing tunnels through (CollisionEnhancer, HandleCollision)") {
    if (!wp20b::GameAvailable()) return;
    const DropRun r = Drop(kPadLat, kPadLon, false, 5.0, 80.0, 60);
    MESSAGE("exploded " << r.exploded << " (enhancer hit " << r.enhancerHit << ", contact impacts " << r.impacts.size() << "), lowest root height "
                        << r.minHeight << " m, explosions " << r.explosions);
    CHECK(r.exploded);
    CHECK(r.explosions == 1);
    CHECK(r.minHeight > -3.0);  // the root never got far below the surface: the step it crossed it, it died
}

TEST_CASE("WP-20e determinism: the drop's contact messages, impact speed and end state are the same at 1 and 4 Jolt threads") {
    if (!wp20b::GameAvailable()) return;
    const double v0 = std::sqrt(8.1 * 8.1 - 2.0 * 9.80 * 0.30);
    const DropRun a = Drop(kPadLat, kPadLon, false, 0.30, v0, 200, "", 1);
    const DropRun b = Drop(kPadLat, kPadLon, false, 0.30, v0, 200, "", 4);
    REQUIRE_FALSE(a.impacts.empty());
    CHECK(a.impacts.size() == b.impacts.size());
    CHECK(wp19b::SameBits(a.impacts.front(), b.impacts.front()));
    CHECK(a.hash == b.hash);
    CHECK(a.landed == b.landed);
}

namespace {
double HighestProxyY(const Vessel& v) {
    double maxY = -1e30;
    for (Part* p : v.parts) {
        if (!p->hasRigidbody) continue;
        for (const ColliderProxy& b : DragCubeColliderProxy(*p))
            for (int c = 0; c < 8; ++c) {
                const Vector3f corner = b.center + Vector3f((c & 1) ? b.halfExtents.x : -b.halfExtents.x, (c & 2) ? b.halfExtents.y : -b.halfExtents.y,
                                                            (c & 4) ? b.halfExtents.z : -b.halfExtents.z);
                maxY = std::max(maxY, static_cast<double>(p->fields.orgPos.y) + static_cast<double>((p->fields.orgRot * corner).y));
            }
    }
    return maxY;
}

struct StackRun {
    bool aDead = false, bDead = false, bLanded = false, aLanded = false;
    std::string aLandedAt, bLandedAt;
    float impactB = 0.0f, impactA = 0.0f, massSeenByB = 0.0f;
    int collisions = 0, layer = -1;
    Part* otherPart = nullptr;
    Part* aRoot = nullptr;
};

// vessel A (one probeStackLarge) standing on the pad, vessel B (another) dropped onto it at v0
StackRun DropOnVessel(double v0) {
    StackRun r;
    AnchorSettings settings;
    World w(true);
    PadScene pad(w);
    GameSettings::PHYSICS_EASE = false;
    EventLog log;
    auto a = w.CraftHead("PhysProf-S", 1);
    auto b = w.CraftHead("PhysProf-S", 1);
    const Site site = SiteAt(pad, kPadLat, kPadLon, false);
    PlaceAt(pad, *a, site, 0.0, true);
    PlaceAt(pad, *b, site, (HighestProxyY(*a) - LowestProxyY(*a)) + 0.30, false);
    pad.s.Start(*b);
    b->MakeActive();
    ResetLevelClock();
    ContactHost& ch = pad.s.bubble.Contacts();
    ch.recordDispatch = true;
    REQUIRE(TickUntilUnpacked(pad, *b));
    REQUIRE(TickUntilUnpacked(pad, *a));
    pad.s.bubble.Host().Find(*b)->SetLinearVelocity(site.up * -v0);
    r.aRoot = a->rootPart;
    Part* aRoot = a->rootPart;
    Part* bRoot = b->rootPart;
    for (int k = 0; k < 150; ++k) {
        Tick(pad);
        for (const ContactHost::DispatchRecord& d : ch.LastDispatch()) {
            if (d.kind != PhysicsWorld::ContactEvent::Kind::Enter || d.collision.layer != 0) continue;
            if (d.receiver == bRoot && r.impactB == 0.0f) {
                r.impactB = d.collision.relativeVelocity.magnitude();
                r.massSeenByB = d.collision.otherRigidbodyMass;
                r.layer = d.collision.layer;
                r.otherPart = d.collision.otherPart;
            }
            if (d.receiver == aRoot && r.impactA == 0.0f) r.impactA = d.collision.relativeVelocity.magnitude();
        }
        if (aRoot->State() == PartStates::DEAD || bRoot->State() == PartStates::DEAD) {
            Tick(pad);
            break;
        }
    }
    r.aDead = aRoot->State() == PartStates::DEAD;
    r.bDead = bRoot->State() == PartStates::DEAD;
    r.aLanded = a->Landed;
    r.bLanded = b->Landed;
    r.aLandedAt = a->landedAt;
    r.bLandedAt = b->landedAt;
    r.collisions = static_cast<int>(log.collisions.size());
    return r;
}
}  // namespace

TEST_CASE("WP-20e vessel on vessel: a part landing on a landed vessel is landed (LandedCollisionChecks' part branch); at speed both parts explode") {
    if (!wp20b::GameAvailable()) return;
    {
        const StackRun r = DropOnVessel(1.0);
        MESSAGE("1 m/s: B's impact " << r.impactB << " m/s (layer " << r.layer << ", other rb mass " << r.massSeenByB << " t), A's " << r.impactA << "; B landed "
                                     << r.bLanded << " at '" << r.bLandedAt << "' (A at '" << r.aLandedAt << "'), dead " << r.aDead << " / " << r.bDead);
        CHECK(r.layer == 0);
        CHECK(r.otherPart == r.aRoot);
        CHECK(r.massSeenByB == doctest::Approx(0.5).epsilon(1e-6));
        CHECK(r.impactB == doctest::Approx(r.impactA).epsilon(1e-6));  // the two sides of one collision
        CHECK(r.bLanded);
        CHECK(r.bLandedAt == r.aLandedAt);
        CHECK_FALSE(r.aDead);
        CHECK_FALSE(r.bDead);
        CHECK(r.collisions == 0);
    }
    {
        const StackRun r = DropOnVessel(25.0);
        MESSAGE("25 m/s: B's impact " << r.impactB << " m/s x A's rb mass " << r.massSeenByB << " t; dead " << r.aDead << " / " << r.bDead << ", onCollision "
                                      << r.collisions);
        CHECK(r.impactB > 20.0f);  // 25 m/s at release, the drag of the 2.5 m disc on the way
        CHECK(r.aDead);
        CHECK(r.bDead);
        CHECK(r.collisions == 2);
    }
}

TEST_CASE("WP-20e CheckGroundCollision: a landed craft spawned 2 m above / 0.4 m into the pad is put on it at GoOffRails (Vessel.cs:2493)") {
    if (!wp20b::GameAvailable()) return;
    for (const double clearance : {2.0, -0.4}) {
        AnchorSettings settings;
        World w(true);
        PadScene pad(w);
        EventLog log;
        auto v = w.Craft("PhysProf-S");
        const Site site = SiteAt(pad, kPadLat, kPadLon, false);
        PlaceAt(pad, *v, site, clearance, true);
        REQUIRE(v->vesselSpawning);
        pad.s.Start(*v);
        ResetLevelClock();
        ContactHost& ch = pad.s.bubble.Contacts();
        ch.recordDispatch = true;
        REQUIRE(TickUntilUnpacked(pad, *v));
        const int ignore = v->IgnoreCollisionsFrames();
        // the lowest proxy corner of the craft over the terrain right after the unpack (rays from 5 m above every lowest corner)
        double gap = 1e30;
        for (Part* p : v->parts) {
            if (!p->hasRigidbody) continue;
            for (const ColliderProxy& b : DragCubeColliderProxy(*p))
                for (int c = 0; c < 8; ++c) {
                    const Vector3f corner = b.center + Vector3f((c & 1) ? b.halfExtents.x : -b.halfExtents.x, (c & 2) ? b.halfExtents.y : -b.halfExtents.y,
                                                                (c & 4) ? b.halfExtents.z : -b.halfExtents.z);
                    const Vector3d world = Vector3d(p->partTransform.TransformPoint(corner));
                    const PhysicsWorld::RayHit h = pad.s.world.RayCast(pad.s.bubble.StaticPhysicsFromWorld(world + site.up * 5.0), site.up * -1.0, 20.0,
                                                                       PhysicsWorld::kLayerStatic);
                    if (h.hit) gap = std::min(gap, h.distance - 5.0);
                }
        }
        float maxImpact = 0.0f;
        for (int k = 0; k < 100; ++k) {
            Tick(pad);
            for (const ContactHost::DispatchRecord& d : ch.LastDispatch())
                if (d.kind == PhysicsWorld::ContactEvent::Kind::Enter) maxImpact = std::max(maxImpact, d.collision.relativeVelocity.magnitude());
        }
        MESSAGE("spawned " << clearance << " m from the pad: after CheckGroundCollision the lowest corner is " << gap << " m above the terrain; ignore-collision frames "
                           << ignore << "; largest impact speed " << maxImpact << " m/s; joint breaks " << pad.s.bubble.Host().LastBreaks().size() << ", parts "
                           << v->parts.size() << ", landed " << v->Landed);
        CHECK(std::fabs(gap) < 0.05);
        CHECK(maxImpact < 0.5f);
        CHECK(v->parts.size() == 21);
        CHECK(v->Landed);
        CHECK(log.explosions == 0);
        // moved up out of the ground: CollisionEnhancer.bypass and 60 frames without collision messages (Vessel.cs:2542-2543)
        if (clearance < 0.0) CHECK(ignore >= 58);
        else CHECK(ignore == 0);
    }
}

namespace {
// Kerbin's stock PQS sphere built around the pad as KSP's flight scene does (PQS.Start, two frames; the WP-12b kerbin_ksc scenario's actions)
struct KerbinSphere {
    PQSCache cache;
    std::unique_ptr<PQS> sphere;
    PQSTarget target;
    explicit KerbinSphere(World& w) {
        cache.cachePQInitialCount = 64;
        sphere = terrain::CreateStockPQS("Kerbin", *w.maps);
        for (auto& m : sphere->childMods)
            if (auto* cb = dynamic_cast<PQSMod_CelestialBodyTransform*>(m.get())) {
                cb->host.hasBody = [] { return true; };
                cb->host.flightGlobalsExists = [] { return true; };
                cb->host.activeVessel = [this] { return &target; };
                cb->host.isCurrentMainBody = [] { return true; };
                cb->host.loadedScene = [] { return static_cast<int>(GameScenes::FLIGHT); };
                cb->host.mapIsEnabled = [] { return false; };
            }
        PQSCache::CreateDefaultPresetList();
        sphere->cache = &cache;
        target.position = terrain::GetRelSurfaceNVector(kPadLat, kPadLon).normalized() * (w.Kerbin().Radius + 70.0);
        sphere->SetTarget(&target);
        sphere->Start();
        for (int f = 0; f < 2; ++f) {
            sphere->FixedUpdate();
            sphere->FrameUpdate();
        }
    }
    ~KerbinSphere() { PQSCache::PresetList.reset(); }
};
}  // namespace

TEST_CASE("WP-20e terrain colliders per body: the bubble hosts Kerbin's PQS quads (WP-12b) in its frame, a part lands on a PQ, frame changes rebuild them") {
    if (!wp20b::GameAvailable()) return;
    AnchorSettings settings;
    World w(true);
    KerbinSphere ks(w);
    KerbinScene sc(w);
    GameSettings::PHYSICS_EASE = false;
    EventLog log;
    CelestialBody& kerbin = w.Kerbin();
    ContactHost& ch = sc.s.bubble.Contacts();
    TerrainColliders* tc = ch.HostTerrain(kerbin, *ks.sphere);
    REQUIRE(tc != nullptr);
    const size_t colliders = tc->CookedCount();
    // the frame: body.rotation maps the sphere's local frame to the world as GetWorldSurfacePosition does
    const Vector3d rel = terrain::GetRelSurfaceNVector(kPadLat, kPadLon).normalized();
    const Vector3d viaFrame = kerbin.GetWorldSurfacePosition(kPadLat, kPadLon, 70.0);
    const Vector3d viaRotation = kerbin.position + kerbin.rotation * (rel * (kerbin.Radius + 70.0));
    // the pad from above: the ray hits a PQ collider of Kerbin at the WP-11 height
    const double wp11 = w.pqs->GetSurfaceHeight(rel, true);
    const Vector3d padWorld = kerbin.GetWorldSurfacePosition(kPadLat, kPadLon, wp11 - kerbin.Radius);
    const Vector3d up = (padWorld - kerbin.position).normalized();
    const PhysicsWorld::RayHit h = sc.s.world.RayCast(sc.s.bubble.StaticPhysicsFromWorld(padWorld + up * 50.0), up * -1.0, 100.0, PhysicsWorld::kLayerStatic);
    REQUIRE(h.hit);
    const Vector3d hitWorld = h.point + sc.s.bubble.FrameOriginWorld();
    const StaticColliderInfo& info = ch.StaticInfo(h.userData);
    MESSAGE(colliders << " PQ colliders hosted; frame: rotation vs BodyFrame " << (viaFrame - viaRotation).magnitude() << " m; the pad ray hits " << info.name
                      << " (PQ " << info.isPQ << ") " << (hitWorld - padWorld).magnitude() << " m from the WP-11 surface point");
    CHECK(colliders >= 9);
    CHECK((viaFrame - viaRotation).magnitude() < 1e-6);
    CHECK(info.isPQ);
    CHECK(info.pqBody == &kerbin);
    CHECK((hitWorld - padWorld).magnitude() < 1.51);  // WP-12b's measured collider-vs-WP-11 bound within 200 m of the pad
    // a part dropped on it lands on the PQ
    Site site = SiteAt(sc, kPadLat, kPadLon, false);
    site.surfaceRadius = (hitWorld - kerbin.position).magnitude();
    auto v = w.CraftHead("PhysProf-S", 1);
    PlaceAt(sc, *v, site, 0.3, false);
    sc.s.Start(*v);
    v->MakeActive();
    ResetLevelClock();
    ch.recordDispatch = true;
    REQUIRE(TickUntilUnpacked(sc, *v));
    sc.s.bubble.Host().Find(*v)->SetLinearVelocity(site.up * -1.0);
    bool onPQ = false;
    for (int k = 0; k < 150; ++k) {
        Tick(sc);
        for (const ContactHost::DispatchRecord& d : ch.LastDispatch())
            if (d.kind == PhysicsWorld::ContactEvent::Kind::Enter && d.collision.isPQ && d.collision.pqBody == &kerbin) onPQ = true;
    }
    MESSAGE("landed on a PQ " << onPQ << ", Landed " << v->Landed << ", situation " << static_cast<int>(v->situation));
    CHECK(onPQ);
    CHECK(v->Landed);
    CHECK(v->situation == Vessel::Situations::LANDED);
    // rotating -> inertial: the body turns under the quads, they go; back: rebuilt where they were
    sc.s.bubble.setRotatingFrame(*sc.s.opm, false);
    CHECK(ch.Terrain(kerbin) == nullptr);
    size_t inRange = 0;
    for (uint64_t key : sc.s.world.StaticKeys())
        if (ch.StaticInfo(key).pqBody == &kerbin) ++inRange;
    CHECK(inRange == 0);
    sc.s.bubble.setRotatingFrame(*sc.s.opm, true);
    REQUIRE(ch.Terrain(kerbin) != nullptr);
    CHECK(ch.Terrain(kerbin)->CookedCount() == colliders);
    ch.DropTerrain(kerbin);
}

TEST_CASE("WP-20e craft touchdown: PhysProf-S set down on the terrain at 1 / 3 / 6 m/s (the Mainsail bell first): lands or crashes by its crashTolerance") {
    if (!wp20b::GameAvailable()) return;
    for (const double v0 : {1.0, 3.0, 6.0}) {
        AnchorSettings settings;
        World w(true);
        PadScene pad(w);
        GameSettings::PHYSICS_EASE = false;
        EventLog log;
        auto v = w.Craft("PhysProf-S");
        const Site site = SiteAt(pad, kPadLat, kPadLon, false);
        PlaceAt(pad, *v, site, 0.05, false);
        pad.s.Start(*v);
        v->MakeActive();
        ResetLevelClock();
        ContactHost& ch = pad.s.bubble.Contacts();
        ch.recordDispatch = true;
        REQUIRE(TickUntilUnpacked(pad, *v));
        pad.s.bubble.Host().Find(*v)->SetLinearVelocity(site.up * -v0);
        float impact = 0.0f, tolerance = 0.0f;
        size_t breaks = 0;
        float maxLoad = 0.0f;
        for (int k = 0; k < 200; ++k) {
            Tick(pad);
            breaks += pad.s.bubble.Host().LastBreaks().size();
            if (VesselPhysics* vp = pad.s.bubble.Host().Find(*v)) maxLoad = std::max(maxLoad, vp->MaxLoadRatio());
            for (const ContactHost::DispatchRecord& d : ch.LastDispatch())
                if (d.kind == PhysicsWorld::ContactEvent::Kind::Enter && impact == 0.0f) {
                    impact = d.collision.relativeVelocity.magnitude();
                    tolerance = d.colliderPart->fields.crashTolerance;
                }
        }
        MESSAGE("touchdown at " << v0 << " m/s: first contact " << impact << " m/s on a part of crashTolerance " << tolerance << "; explosions " << log.explosions
                                << ", joint breaks " << breaks << " (largest sub-joint load / capacity " << maxLoad << "), landed " << v->Landed << ", parts "
                                << v->parts.size() << ", situation " << static_cast<int>(v->situation));
        CHECK(impact > 0.0f);
        CHECK((log.explosions > 0) == (impact > tolerance));  // Part.HandleCollision on the touching part
        if (impact <= tolerance) {
            CHECK(v->Landed);
            CHECK(v->situation == Vessel::Situations::LANDED);
        }
    }
}

TEST_CASE("WP-20e input seam: PhysicsBubble::inputFeed runs at FlightInputHandler.FixedUpdate's slot, before the part modules and the FlightIntegrator") {
    if (!wp20b::GameAvailable()) return;
    World w(true);
    PadScene pad(w);
    auto v = w.Craft("PhysProf-S");
    pad.Place(*v);
    pad.s.Start(*v);
    std::vector<std::string> order;
    pad.s.bubble.inputFeed = [&] { order.push_back("input"); };
    const std::function<void()> fi = FlightScene::hooks.afterFlightIntegrator;
    FlightScene::hooks.afterFlightIntegrator = [&] {
        order.push_back("fi");
        fi();
    };
    // the module slot ran after the input: an engine ignited by the input slot produces thrust in the same tick
    for (int k = 0; k < 3; ++k) Tick(pad);
    wp20d::IgniteFirstStage(*v);
    v->ctrlState.mainThrottle = 0.0f;  // the input slot sets it
    pad.s.bubble.inputFeed = [&] {
        order.push_back("input");
        v->ctrlState.mainThrottle = 1.0f;  // what FlightInputHandler.FixedUpdate / FeedInputFeed would have put there
    };
    order.clear();
    Tick(pad);
    MESSAGE("slot order of one tick: " << (order.size() > 1 ? order[0] + ", " + order[1] : std::string("?")) << "; engine forces handed to the buffers "
                                       << pad.s.bubble.ModuleForces().engineForces);
    REQUIRE(order.size() == 2);
    CHECK(order[0] == "input");
    CHECK(order[1] == "fi");
    CHECK(pad.s.bubble.ModuleForces().engineForces > 0);
    FlightScene::hooks.afterFlightIntegrator = fi;
    pad.s.bubble.inputFeed = nullptr;
}


TEST_CASE("WP-20e collider closest points: getLowestPoint's Collider.ClosestPoint over box / sphere / capsule / hull proxies (WP-mucol shapes)") {
    if (!wp20b::GameAvailable()) return;
    // the part proxies of WP-mucol's real .mu colliders (VesselPhysicsOptions::colliders); synthetic shapes with analytic answers
    ColliderProxy box;  // kind Box
    box.center = Vector3f(0.0f, 1.0f, 0.0f);
    box.halfExtents = Vector3f(0.5f, 0.5f, 0.5f);
    ColliderProxy sphere;
    sphere.kind = ShapeKind::Sphere;
    sphere.center = Vector3f(0.0f, 0.0f, 2.0f);
    sphere.radius = 0.7f;
    ColliderProxy capsule;  // its local Y (the cylinder axis) turned onto the part's Z
    capsule.kind = ShapeKind::Capsule;
    capsule.center = Vector3f(0.0f, 0.0f, -2.0f);
    capsule.rotation = Quaternionf(0.70710678f, 0.0f, 0.0f, 0.70710678f);
    capsule.radius = 0.4f;
    capsule.halfHeight = 1.0f;
    ColliderProxy hull;
    hull.kind = ShapeKind::ConvexHull;
    hull.center = Vector3f(0.0f, 0.0f, -5.0f);
    auto pts = std::make_shared<std::vector<Vector3f>>();
    for (int c = 0; c < 8; ++c) pts->push_back(Vector3f((c & 1) ? 0.5f : -0.5f, (c & 2) ? 0.5f : -0.5f, (c & 4) ? 0.5f : -0.5f));
    hull.hull = pts;
    World w(true);
    PhysicsBubbleOptions o;
    o.vessel.colliders = [&](const Part&) { return std::vector<ColliderProxy>{box, sphere, capsule, hull}; };
    PadScene pad(w, o);
    auto v = w.Craft("PhysProf-S");
    Part& p = *v->rootPart;
    p.partTransform.position = Vector3f(10.0f, 20.0f, 30.0f);
    p.partTransform.rotation = Quaternionf::identity();
    // getLowestPoint's measure point: 1,000 m along -forward from the vessel (Vessel.cs:2587), here slightly off the part's axis
    const Vector3f m(10.3f, 19.8f, -970.0f);
    REQUIRE(Vessel::env.colliderClosestPointsZ);
    const std::vector<float> zs = Vessel::env.colliderClosestPointsZ(p, m);
    REQUIRE(zs.size() == 4);
    const Vector3d M(m), P(p.partTransform.position);
    auto towards = [&](const Vector3d& c, double r) { return c.z + r * (M.z - c.z) / (M - c).magnitude(); };
    const double expected[4] = {
        P.z - 0.5,                                    // the box's bottom face (M is below it)
        towards(P + Vector3d(0.0, 0.0, 2.0), 0.7),     // the sphere's point towards M
        towards(P + Vector3d(0.0, 0.0, -3.0), 0.4),    // the lower hemisphere of the capsule (axis end at -2 - 1)
        P.z - 5.5,                                    // the hull's bottom face (M projects inside it)
    };
    for (int i = 0; i < 4; ++i) {
        MESSAGE("proxy " << i << ": ClosestPoint z " << zs[i] << " vs " << expected[i]);
        CHECK(std::fabs(zs[i] - expected[i]) < 1e-5);
    }
    // a point inside a collider is its own closest point (Unity's Collider.ClosestPoint)
    const std::vector<float> inside = Vessel::env.colliderClosestPointsZ(p, Vector3f(10.0f, 20.0f, 32.1f));
    CHECK(inside[1] == doctest::Approx(32.1).epsilon(1e-6));
}

TEST_CASE("WP-20e contact event log: several world steps between two dispatches keep every step's events (PhysicsWorld::TakeContactEventLog)") {
    // a box moving onto a static floor, four steps of 5 ms per "FixedUpdate" (no gravity: the world has none): the contact begins in the
    // tick's first step and the last step reports only its Stay; the log holds the Enter of the step it happened in
    PhysicsWorld world;
    world.SetContactEventTracking(true);
    REQUIRE(world.AddStaticShape(7, ShapeDesc::Box(Vector3f(10.0f, 0.5f, 10.0f)), Vector3d(0.0, -0.5, 0.0), QuaternionD::identity()) != kInvalidBody);
    BodyDesc d;
    d.shape = ShapeDesc::Box(Vector3f(0.5f, 0.5f, 0.5f));
    d.position = Vector3d(0.0, 0.5 + 0.005, 0.0);  // 5 mm above the floor: within Unity's 0.01 m contact offset at the first step
    d.linearVelocity = Vector3d(0.0, -3.0, 0.0);
    d.mass = 1.0f;
    const BodyId box = world.CreateBody(d);
    REQUIRE(box != kInvalidBody);
    std::vector<std::vector<PhysicsWorld::ContactEvent::Kind>> perStep;
    for (int k = 0; k < 4; ++k) {
        world.Step(0.005f);
        perStep.emplace_back();
        for (const PhysicsWorld::ContactEvent& e : world.ContactEvents()) perStep.back().push_back(e.kind);
    }
    const std::vector<std::vector<PhysicsWorld::ContactEvent>> log = world.TakeContactEventLog();
    int enterStep = -1;
    for (int k = 0; k < 4 && enterStep < 0; ++k)
        for (PhysicsWorld::ContactEvent::Kind kind : perStep[static_cast<size_t>(k)])
            if (kind == PhysicsWorld::ContactEvent::Kind::Enter) enterStep = k;
    size_t logged = 0;
    bool logEnter = false;
    for (const auto& step : log) {
        logged += step.size();
        for (const PhysicsWorld::ContactEvent& e : step) logEnter = logEnter || (e.kind == PhysicsWorld::ContactEvent::Kind::Enter && e.body1 == box && e.otherStatic && e.other == 7);
    }
    size_t raw = 0;
    for (const auto& s : perStep) raw += s.size();
    MESSAGE("contact begins in step " << enterStep << " of 4; the last step reports " << perStep.back().size() << " event(s); the log holds " << log.size()
                                     << " step(s), " << logged << " event(s)");
    REQUIRE(enterStep >= 0);
    CHECK(enterStep < 3);  // inside the tick: a last-step-only dispatch would never send its OnCollisionEnter
    for (PhysicsWorld::ContactEvent::Kind kind : perStep.back()) CHECK(kind != PhysicsWorld::ContactEvent::Kind::Enter);
    CHECK(logEnter);
    CHECK(logged == raw);  // every step's events, none twice
    CHECK(world.TakeContactEventLog().empty());  // taken
    // the log keeps the last 64 steps that reported events, so an undrained world stays bounded
    for (int k = 0; k < 100; ++k) world.Step(0.005f);
    CHECK(world.TakeContactEventLog().size() == 64);
    world.SetContactEventTracking(false);
    world.Step(0.005f);
    CHECK(world.TakeContactEventLog().empty());
}

TEST_CASE("WP-20e physics-warp substeps: every substep's contact messages reach the parts (WP-29c's PhysicsBubble::physicsStep, 4x)") {
    if (!wp20b::GameAvailable()) return;
    const double tol = 9.0;  // probeStackLarge.cfg crashTolerance
    const double g = 9.80;
    const double d = 0.30;
    for (const double f : {0.9, 1.1}) {
        const double vt = f * tol;
        const double v0 = std::sqrt(vt * vt - 2.0 * g * d);
        const DropRun r = Drop(kPadLat, kPadLon, false, d, v0, 100, "", 1, 4);
        MESSAGE(f << " x crashTolerance at 4x physics warp (4 substeps of 0.02 s): the root's Enter events over all substeps " << r.rawRootEnters
                  << " (in a tick's last substep " << r.lastStepRootEnters << "), dispatched " << r.dispatchedRootEnters << "; steps per dispatch up to "
                  << r.maxStepsPerDispatch << ", duplicate Stays " << r.stayDuplicates << "; impact " << (r.impacts.empty() ? 0.0f : r.impacts.front())
                  << " m/s (aimed " << vt << "), exploded " << r.exploded << ", landed " << r.landed);
        REQUIRE(r.rawRootEnters > 0);
        // every substep's Enter reaches the part (none lost, none twice); at 1.1x the first explodes it and the part, DEAD, takes no more
        // messages (the same rule as at 1x). Here the contact begins inside the tick: a last-substep-only dispatch would have sent none.
        if (f < 1.0) CHECK(r.dispatchedRootEnters == r.rawRootEnters);
        else CHECK(r.dispatchedRootEnters >= 1);
        CHECK(r.maxStepsPerDispatch > 1);
        CHECK(r.stayDuplicates == 0);                       // OnCollisionStay once per FixedUpdate and pair
        REQUIRE_FALSE(r.impacts.empty());
        CHECK(std::fabs(r.impacts.front() - vt) < 0.03 * vt);
        CHECK(r.exploded == (f > 1.0));
        if (f < 1.0) {
            CHECK(r.landed);
            CHECK(r.situation == Vessel::Situations::LANDED);
        }
    }
}
