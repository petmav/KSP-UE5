// WP-52b gate, part 1: the articulated joints of PhysicsWorld (the servos' ConfigurableJoint on Jolt) against the independent Python
// reference of the drive law (tools/wp52b/drive_reference.py -> tests/data/wp52b_reference.json) and the constraint impulse against the
// bodies' momentum. Every drive value is a SYNTHETIC fixture (no servo part data exists locally); the sign conventions and the limit /
// spring behaviour of the drive are UNVERIFIED against PhysX (no capture).
#include <cmath>
#include <fstream>
#include <string>

#include "doctest/doctest.h"
#include "nlohmann/json.hpp"

#include "ksp/physics/physics_world.hpp"

using namespace ksp;
using namespace ksp::physics;
using json = nlohmann::json;

namespace {

const json& Ref() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp52b_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

constexpr double kPi = 3.14159265358979323846;

// a static base and one dynamic body of mass m and isotropic inertia I (explicit: the drive sees exactly I about any axis through the centre
// of mass), the joint through the body's centre of mass along `axis` (+ `offset`: the lever arm)
struct Rig {
    PhysicsWorld world;
    BodyId base = kInvalidBody, body = kInvalidBody;
    ConstraintId joint = kInvalidConstraint;
    explicit Rig(PhysicsWorldConfig cfg = {}) : world(cfg) {}
    void Build(PhysicsWorld::ArticulatedJointDesc::Kind kind, double m, double I, const Vector3d& axis, const Vector3d& offset = {},
               double coordinate0 = 0.0, const Vector3d& at = Vector3d(0, 10, 0)) {
        BodyDesc b;
        b.shape = ShapeDesc::Box(Vector3f(0.5f, 0.5f, 0.5f));
        b.motion = MotionType::Static;
        base = world.CreateBody(b);
        CompoundBodyDesc d;
        CompoundChild ch;
        ch.shape = ShapeDesc::Box(Vector3f(0.2f, 0.2f, 0.2f));
        d.children.push_back(ch);
        d.position = at;
        d.mass = m;
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c) d.inertia[r][c] = r == c ? I : 0.0;
        d.maxAngularVelocity = 100.0f;
        body = world.CreateCompoundBody(d);
        PhysicsWorld::ArticulatedJointDesc j;
        j.kind = kind;
        j.point = at + offset;
        j.axis = axis;
        j.normal = std::fabs(axis.y) < 0.9 ? Vector3d(0, 1, 0) : Vector3d(1, 0, 0);
        j.coordinate0 = coordinate0;
        joint = world.CreateArticulatedConstraint(base, body, j);
        REQUIRE(joint != kInvalidConstraint);
    }
};

PhysicsWorld::ArticulatedDrive DriveOf(const json& c, double target) {
    PhysicsWorld::ArticulatedDrive d;
    d.spring = c["k"].get<double>();
    d.damper = c["c"].get<double>();
    d.maxForce = c["F"].get<double>();
    d.target = target;
    d.targetVelocity = c["vt"].get<double>();
    return d;
}

double Wrap(double a) {
    while (a > kPi) a -= 2.0 * kPi;
    while (a <= -kPi) a += 2.0 * kPi;
    return a;
}

}  // namespace

TEST_CASE("WP-52b articulated joint: a hinge / slider reads its creation coordinate and keeps the five locked degrees of freedom") {
    for (double c0 : {0.0, 0.7, -2.5}) {
        Rig r;
        r.Build(PhysicsWorld::ArticulatedJointDesc::Kind::Hinge, 1.0, 1.0, Vector3d(0, 0, 1), Vector3d(0.3, 0, 0), c0);
        CHECK(r.world.ArticulatedCoordinate(r.joint) == doctest::Approx(c0).epsilon(1e-6));
        Rig s;
        s.Build(PhysicsWorld::ArticulatedJointDesc::Kind::Slider, 1.0, 1.0, Vector3d(0, 1, 0), Vector3d(), c0);
        CHECK(s.world.ArticulatedCoordinate(s.joint) == doctest::Approx(c0).epsilon(1e-6));
    }
    // positive coordinate = body b turned about +axis: an angular velocity +z of the body reads as a positive rate and a growing angle
    Rig r;
    r.Build(PhysicsWorld::ArticulatedJointDesc::Kind::Hinge, 1.0, 1.0, Vector3d(0, 0, 1));
    r.world.SetAngularVelocity(r.body, Vector3d(0, 0, 1.0));
    CHECK(r.world.ArticulatedRate(r.joint) == doctest::Approx(1.0));
    for (int i = 0; i < 10; ++i) r.world.Step(0.02f);
    CHECK(r.world.ArticulatedCoordinate(r.joint) == doctest::Approx(0.2).epsilon(1e-4));
    // a push across the axis is held (no drift of the pivot, no rotation off the axis)
    const Vector3d p0 = r.world.GetPosition(r.body);
    for (int i = 0; i < 50; ++i) {
        r.world.AddForce(r.body, Vector3d(5.0, 3.0, -4.0));
        r.world.AddTorque(r.body, Vector3d(2.0, -1.0, 0.0));
        r.world.Step(0.02f);
    }
    const Vector3d w = r.world.GetAngularVelocity(r.body);
    CHECK(std::fabs(w.x) < 1e-3);
    CHECK(std::fabs(w.y) < 1e-3);
    CHECK((r.world.GetPosition(r.body) - p0).magnitude() < 1e-3);
}

TEST_CASE("WP-52b drive law: Jolt's PositionAndVelocity motor matches the independent implicit D6 drive reference (hinge, slider, rotor, hold)") {
    const json& ref = Ref();
    const float dt = static_cast<float>(ref["dt"].get<double>());
    double worstX = 0.0, worstV = 0.0;
    int cases = 0;
    for (const json& c : ref["cases"]) {
        const std::string kind = c["kind"].get<std::string>();
        for (int slider = 0; slider < 2; ++slider) {
            if (slider == 1 && kind == "velocity") continue;  // the rotor is a hinge
            INFO(c["name"].get<std::string>() << (slider ? " slider" : " hinge"));
            Rig r;
            const double I = c["I"].get<double>();
            // hinge: inertia I about the axis (mass irrelevant, the axis passes the centre of mass); slider: mass I
            r.Build(slider ? PhysicsWorld::ArticulatedJointDesc::Kind::Slider : PhysicsWorld::ArticulatedJointDesc::Kind::Hinge, slider ? I : 1.0,
                    slider ? 1.0 : I, Vector3d(0.6, 0.0, 0.8));
            const std::vector<double> xs = c["x"].get<std::vector<double>>(), vs = c["v"].get<std::vector<double>>();
            const double load = c["load"].get<double>();
            const double rate = c.contains("rate") ? c["rate"].get<double>() : 0.0;
            double unwrapped = 0.0, last = 0.0, lagAtRampEnd = 0.0, spoolTime = -1.0;
            for (size_t n = 0; n < xs.size(); ++n) {
                double target = c["target"].get<double>();
                if (kind == "ramp") target = std::min(c["x0"].get<double>() + rate * static_cast<double>(dt) * static_cast<double>(n + 1), target);
                r.world.SetArticulatedDrive(r.joint, DriveOf(c, target));
                if (load != 0.0) {
                    if (slider) r.world.AddForce(r.body, Vector3d(0.6, 0.0, 0.8) * load);
                    else r.world.AddTorque(r.body, Vector3d(0.6, 0.0, 0.8) * load);
                }
                r.world.Step(dt);
                const double x = r.world.ArticulatedCoordinate(r.joint);
                unwrapped += slider ? x - last : Wrap(x - last);
                last = x;
                const double v = r.world.ArticulatedRate(r.joint);
                const double scaleX = std::max(1.0, std::fabs(xs[n]));
                const double scaleV = std::max(1.0, std::fabs(vs[n]));
                worstX = std::max(worstX, std::fabs(unwrapped - xs[n]) / scaleX);
                worstV = std::max(worstV, std::fabs(v - vs[n]) / scaleV);
                REQUIRE(std::fabs(unwrapped - xs[n]) <= 2e-4 * scaleX);
                REQUIRE(std::fabs(v - vs[n]) <= 2e-4 * scaleV);
                if (kind == "ramp" && target < c["target"].get<double>()) lagAtRampEnd = target - unwrapped;
                if (kind == "velocity" && spoolTime < 0.0 && v >= 0.99 * c["vt"].get<double>()) spoolTime = static_cast<double>(dt) * static_cast<double>(n + 1);
            }
            // the analytic markers of the reference (continuous-time closed forms of the drive law)
            if (kind == "ramp") CHECK(lagAtRampEnd == doctest::Approx(c["lag"].get<double>()).epsilon(0.05));
            if (kind == "velocity") CHECK(spoolTime == doctest::Approx(0.99 * c["spool"].get<double>()).epsilon(0.03));
            if (kind == "hold") CHECK(unwrapped == doctest::Approx(c["offset"].get<double>()).epsilon(1e-3));
            ++cases;
        }
    }
    MESSAGE("drive law: " << cases << " runs, worst |dx| / max(1, |x|) " << worstX << ", worst |dv| / max(1, |v|) " << worstV);
}

TEST_CASE("WP-52b solver characterisation: a velocity-level constraint solve dissipates (1 - cos w dt) of an arm's tangential speed per step") {
    // A body spinning about a pivot off its centre of mass: each step the first-order integrator moves the centre of mass along the old
    // tangent and the point constraint removes the radial part of its velocity, i.e. the tangential speed loses (1 - cos(w dt)) per step.
    // PhysX solves at the same level (KSP's servos see the same kind of loss). With a velocity drive (k ~ 0, damper c) the steady rate then
    // falls short of the target by about m r^2 w (1 - cos w dt) / (c dt); without an arm it reaches the target. (Characterisation of the
    // solver, not a KSP rule: the tolerance is the model's accuracy, 20 %.)
    for (double off : {0.0, 2.4}) {
        Rig r;
        const double m = 18.0, I = 30.0, c = 1.0e4, vt = 3.14159;
        r.Build(PhysicsWorld::ArticulatedJointDesc::Kind::Hinge, m, I, Vector3d(1, 0, 0), Vector3d(0, off, 0));
        PhysicsWorld::ArticulatedDrive d;
        d.spring = 1e-10;
        d.damper = c;
        d.maxForce = 100.0;
        d.targetVelocity = vt;
        for (int t = 1; t <= 400; ++t) {
            r.world.SetArticulatedDrive(r.joint, d);
            r.world.Step(0.02f);
        }
        const double w = r.world.ArticulatedRate(r.joint);
        const double loss = m * off * off * w * (1.0 - std::cos(w * 0.02)) / (c * 0.02);
        MESSAGE("arm " << off << " m: steady rate " << w << " (target " << vt << ", predicted shortfall " << loss << ")");
        if (off == 0.0) CHECK(w == doctest::Approx(vt).epsilon(1e-5));
        else CHECK(vt - w == doctest::Approx(loss).epsilon(0.2));
    }
}
