// WP-29c: wheels on the reduced rigid-body model in the flight tick (ksp/physics/vessel_wheels.hpp): KSP's stock rovers on flat ground.
//
// The rovers are the stock Prospector Rover (19 parts, 4 x RoveMax M1, 2 motors) and Bug-E Buggy (26 parts, 8 x RoveMax S2, 6 motors)
// through the whole tick (WP-19b bubble, WP-20d FlightIntegrator, WP-20c loads, the managed wheel code of WP-29a / 29b, the native law of
// WP-29c) on the pad scene's terrain patch without a PQS (Kerbin's sea-level sphere). References, independent of the code under test:
//   * ride height: the tyre bottom predicted by the law's equilibrium compression (target D + F / k) lies on the analytic ground (the sphere),
//     and the wheel forces carry the vessel's weight (static equilibrium);
//   * drive / brake: the C# motor and brake models through wheels rolling without slip (the wheel spike's analytic references, re-coded
//     here from ModuleWheelMotor.cs / ModuleWheelBrakes.cs / KSPWheelController.cs), the tyre curve of locked wheels for the brake cap;
//   * steer: the kinematic bicycle model of the steered wheels' collider angles; the turn direction of KSP's keys (FlightInputHandler.cs:585);
//   * the spike's measured numbers (core/WHEEL-SPIKE.md section 4: the same law and geometry proxy on its own body model) as a cross-check.
// Not a capture: no in-game wheel tick exists (WHEEL-SPIKE.md section 5, capture C11).
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>

#include "doctest/doctest.h"
#include "ksp/curves/float_curve.hpp"
#include "vessel_wheels_fixture.hpp"

using namespace ksp;
using namespace ksp::physics;
using namespace wp29c;

namespace {

constexpr double kRad = 3.14159265358979323846 / 180.0;
constexpr int kWheelsUp = 3;  // ticks until the wheels are set up (unpack in LateUpdate, wheelSetup in the next Update's coroutine pass)

// one rover in the scene
struct Run {
    World w;
    LoadedRover r;
    std::unique_ptr<RoverScene> scene;
    double mass = 0.0;
    explicit Run(const std::string& craft, WheelPhysicsOptions o = {}, double heading = 0.0) {
        r = LoadRover(w, craft);
        scene = std::make_unique<RoverScene>(w, RoverOptions(r, o));
        scene->Place(r, heading);
        scene->pad.s.Start(*r.vessel);
    }
    ~Run() {
        r.vessel.reset();  // the vessel before the scene (fixture order)
        scene.reset();
    }
    Vessel& V() { return *r.vessel; }
    WheelPhysicsHost& Host() { return scene->wheels; }
    VesselPhysics* Vp() { return scene->pad.s.bubble.Host().Find(V()); }
    Vector3d Up() const { return scene->pad.padUp; }
    void Tick() { scene->Tick(); }
    void Ticks(int n, const std::function<void()>& each = nullptr) {
        for (int k = 0; k < n; ++k) {
            if (each) each();
            Tick();
        }
    }
    Vector3d Com() { return VesselCoM(*Vp()); }
    Vector3d Vel() { return VesselVelocity(*Vp()); }
    Vector3d Forward() { return Vector3d(V().vesselTransform.rotation * r.craftFwd).normalized(); }
    Vector3d BodyUp() { return Vector3d(V().vesselTransform.rotation * r.craftUp).normalized(); }
    double TiltDeg() { return std::acos(std::max(-1.0, std::min(1.0, Vector3d::Dot(BodyUp(), Up())))) / kRad; }
    double ForwardSpeed() { return Vector3d::Dot(Vel(), Forward()); }
    VesselWheels& Wheels() { return *Host().Find(V()); }
    double Mass() {
        double m = 0.0;
        for (size_t c = 0; c < Vp()->ClusterCount(); ++c) m += Vp()->ClusterMass(c);
        return m;
    }
    // height above Kerbin's sea-level sphere (the ground of the scene) of a transform-frame point
    double Altitude(const Vector3f& p) { return (Vector3d(p) - w.Kerbin().position).magnitude() - w.Kerbin().Radius; }
};

bool Healthy(Run& run) {
    const Vector3d v = run.Vel();
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && v.magnitude() < 200.0;
}

// ---- rest ---------------------------------------------------------------------------------------------------------------------------------
struct RestResult {
    double horizDrift = 0, vertDrift = 0, rmsVz = 0, maxW = 0, meanCompression = 0, alt = 0;
    double forceSum = 0, weight = 0;        // sum of the wheel forces along the vertical; mass x the tick's uniform acceleration
    double maxBottomError = 0;              // |predicted tyre bottom - ground| over the wheels (m)
    double minBodyClearance = 1e9;          // lowest non-wheel proxy point above the ground (m)
    double maxJointForceError = 0;          // |wheel attach joint load - its suspension force| / suspension force
    int grounded = 0, wheels = 0, bottomedTicks = 0;
    double msPerTick = 0;
    bool healthy = true;
};

RestResult Rest(const std::string& craft, double seconds, float warp, WheelPhysicsOptions o = {}) {
    Run run(craft, o);
    run.Ticks(kWheelsUp);
    SetBrakes(run.V(), true);
    run.Ticks(250 - kWheelsUp);  // 5 s: easing and the settle
    if (warp > 1.0f) run.scene->SetPhysicsWarp(warp);
    const float dt = Time::fixedDeltaTime;
    const int ticks = static_cast<int>(std::lround(seconds / dt));
    RestResult r;
    const Vector3d c0 = run.Com();
    double sum = 0;
    int n = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (int k = 0; k < ticks; ++k) {
        run.Tick();
        if (!Healthy(run)) {
            r.healthy = false;
            break;
        }
        const double vz = Vector3d::Dot(run.Vel(), run.Up());
        sum += vz * vz;
        ++n;
        for (size_t c = 0; c < run.Vp()->ClusterCount(); ++c) r.maxW = std::max(r.maxW, run.Vp()->ClusterAngularVelocity(c).magnitude());
        for (auto& w : run.Wheels().Wheels()) r.bottomedTicks += w->last.limitActive ? 1 : 0;
    }
    r.msPerTick = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / std::max(1, ticks);
    if (warp > 1.0f) run.scene->SetPhysicsWarp(1.0f);
    const Vector3d d = run.Com() - c0;
    r.vertDrift = Vector3d::Dot(d, run.Up());
    r.horizDrift = (d - run.Up() * r.vertDrift).magnitude();
    r.rmsVz = n > 0 ? std::sqrt(sum / n) : 0.0;
    r.alt = Vector3d::Dot(run.Com() - run.scene->pad.padWorld, run.Up());
    r.weight = run.Mass() * run.V().precalc.integrationAccel.magnitude();
    VesselPhysics& vp = *run.Vp();
    for (auto& w : run.Wheels().Wheels()) {
        ++r.wheels;
        const auto& g = w->lastGeometry;
        const auto& s = w->last;
        if (!s.grounded) continue;
        ++r.grounded;
        r.meanCompression += (g.maxDroop + s.jounce) / (g.maxDroop + g.maxCompression);
        r.forceSum += Vector3d::Dot(Vector3d(s.force), run.Up());
        // the tyre bottom at the law's equilibrium compression (target D + F / k from full extension; PhysX's gravity term is 0 in KSP)
        const auto& wc = w->Controller()->wheelCollider->wheelColliderData;
        const double D = wc.suspensionDistance, k = wc.suspensionSpring.spring, t = wc.suspensionSpring.targetPosition;
        const double comp = t * D + static_cast<double>(s.suspensionForce) / k;
        const Vector3d top(g.top), up(g.up);
        const Vector3d bottom = top - up * (D + static_cast<double>(wc.radius)) + up * comp;
        r.maxBottomError = std::max(r.maxBottomError, std::fabs(run.Altitude(bottom.ToVector3f())));
        // WP-20c: the wheel part's joints (its attach joint and the ForceHeaviest autostrut ModuleWheelBase gives it, ModuleWheelBase.cs:220)
        // carry its suspension force less the wheel part's own weight
        Vector3d jointSum;
        int joints = 0;
        for (size_t j = 0; j < vp.Joints().size(); ++j) {
            const PartJointSpec& js = vp.Joints()[j];
            if (js.child != w->part || !vp.JointLoads()[j].valid) continue;
            jointSum += vp.JointLoads()[j].force;
            ++joints;
        }
        const double wheelWeight = static_cast<double>(vp.PartMass(*w->part)) * run.V().precalc.integrationAccel.magnitude();
        const double expect = static_cast<double>(s.suspensionForce) - wheelWeight;
        if (joints > 0) r.maxJointForceError = std::max(r.maxJointForceError, std::fabs(jointSum.magnitude() - std::fabs(expect)) / std::fabs(expect));
        else r.maxJointForceError = 1e9;
    }
    r.meanCompression /= std::max(1, r.grounded);
    for (Part* p : run.V().parts) {
        if (!p->hasRigidbody || run.Wheels().Find(*p) != nullptr) continue;
        const PartPhysicsState st = vp.PartState(*p);
        for (const ColliderProxy& b : DragCubeColliderProxy(*p))
            for (int c = 0; c < 8; ++c) {
                const Vector3d corner = Vector3d(b.center) + Vector3d((c & 1) ? b.halfExtents.x : -b.halfExtents.x, (c & 2) ? b.halfExtents.y : -b.halfExtents.y,
                                                                      (c & 4) ? b.halfExtents.z : -b.halfExtents.z);
                const Vector3d world = vp.WorldFromPhysics(st.position + st.rotation * corner);
                r.minBodyClearance = std::min(r.minBodyClearance, (world - run.w.Kerbin().position).magnitude() - run.w.Kerbin().Radius);
            }
    }
    return r;
}

// ---- the analytic drive / brake references (the wheel spike's, core/spikes/wheels/src/scenarios.cpp RefForce): the C# motor model
// (ModuleWheelMotor.cs:131-176: rounded torque curve at |w r_cfg|, traction control, driveLimiter; KSPWheelController.cs:240-263: the drive /
// brake ramps, maxBrakeTorqueRest once brakeState >= 1) through wheels rolling without slip, accelerating the vessel mass plus the wheels'
// spin inertia (VPP: I = max(0.01, m r^2 / 2), Wheel.cs:76-84); no suspension, no slip, no aero --------------------------------------------
struct MotorRef {
    FloatCurve curve;
    float rCfg = 0, rTrue = 0, driveResponse = 0, tcs = 1, maxBrake = 0, brakeResponse = 0, brakeTweakable = 50;
    bool autoTorque = true, brakes = false, motor = false;
    float driveState = 0, brakeState = 0;
    double inertia = 0;
};

std::vector<MotorRef> MotorRefs(Run& run) {
    std::vector<MotorRef> out;
    for (auto& w : run.Wheels().Wheels()) {
        Part& p = *w->part;
        MotorRef r;
        r.rCfg = w->base->Get<float>("radius");
        r.rTrue = r.rCfg * p.fields.rescaleFactor;
        const double m = std::max(0.01, static_cast<double>(w->base->Get<float>("mass")));
        r.inertia = std::max(0.01, m * r.rTrue * r.rTrue / 2.0);
        for (size_t i = 0; i < p.Modules().Count(); ++i) {
            PartModule* mod = p.Modules()[i];
            if (auto* mo = dynamic_cast<ModuleWheels::ModuleWheelMotor*>(mod)) {
                r.motor = mo->Get<bool>("motorEnabled");
                r.curve = mo->Get<FloatCurve>("torqueCurve");
                r.driveResponse = mo->Get<float>("driveResponse");
                r.tcs = mo->Get<float>("tractionControlScale");
                r.autoTorque = mo->Get<bool>("autoTorque");
            } else if (auto* br = dynamic_cast<ModuleWheels::ModuleWheelBrakes*>(mod)) {
                r.brakes = true;
                r.maxBrake = br->Get<float>("maxBrakeTorque");
                r.brakeResponse = br->Get<float>("brakeResponse");
                r.brakeTweakable = br->Get<float>("brakeTweakable");
            }
        }
        out.push_back(r);
    }
    return out;
}

float MoveTowards(float cur, float target, float maxDelta) {
    if (std::fabs(target - cur) <= maxDelta) return target;
    return cur + (target - cur >= 0.0f ? 1.0f : -1.0f) * maxDelta;
}
float Lerp01(float a, float b, float t) { return a + (b - a) * std::min(1.0f, std::max(0.0f, t)); }

// the propulsive (+) / braking (-) force (kN) at speed v, one tick of dt
double RefForce(std::vector<MotorRef>& refs, double v, float throttle, float brake, float dt, double gee) {
    double f = 0.0;
    for (MotorRef& r : refs) {
        const float w = static_cast<float>(v) / r.rTrue;
        if (r.motor) {
            float maxT = r.curve.Evaluate(std::fabs(w * r.rCfg));
            maxT = static_cast<float>(std::nearbyint(static_cast<double>(maxT * 100.0f))) / 100.0f;
            float di = throttle;
            if (r.autoTorque) {
                const float slipDispY = (w * r.rCfg - static_cast<float>(v)) * dt;
                di *= 1.0f - std::fabs(slipDispY * r.tcs);
                di *= Lerp01(1.0f, static_cast<float>(gee), r.tcs / 2.0f);
            }
            r.driveState = MoveTowards(r.driveState, di, r.driveResponse * dt);
            f += static_cast<double>(r.driveState * maxT) / r.rTrue;
        }
        if (r.brakes) {
            const float num = std::min(1.0f, Lerp01(1.0f, static_cast<float>(gee), std::fabs(w)));
            const float bi = brake * r.brakeTweakable * 0.01f * num;
            r.brakeState = MoveTowards(r.brakeState, bi, (bi != 0.0f ? r.brakeResponse : 100.0f) * dt);
            const float tb = r.brakeState >= 1.0f ? r.brakeState * 1000.0f : r.brakeState * r.maxBrake;
            if (v > 1e-4) f -= static_cast<double>(tb) / r.rTrue;
        }
    }
    return f;
}

double EffectiveMass(Run& run, const std::vector<MotorRef>& refs) {
    double m = run.Mass();
    for (const MotorRef& r : refs) m += r.inertia / (static_cast<double>(r.rTrue) * r.rTrue);
    return m;
}

struct DriveResult {
    double vAt[2] = {}, vRef[2] = {};  // 10 s, 20 s
    double maxTilt = 0, mass = 0, massEff = 0;
    bool healthy = true;
};

DriveResult Drive(const std::string& craft, float warp = 1.0f, WheelPhysicsOptions o = {}) {
    Run run(craft, o);
    run.Ticks(100);  // 2 s settle at 1x
    if (warp > 1.0f) run.scene->SetPhysicsWarp(warp);
    const float dt = Time::fixedDeltaTime;
    auto refs = MotorRefs(run);
    DriveResult r;
    r.mass = run.Mass();
    r.massEff = EffectiveMass(run, refs);
    double va = 0.0;
    run.V().ctrlState.wheelThrottle = 1.0f;
    const int ticks = static_cast<int>(std::lround(20.0 / dt));
    for (int k = 0; k < ticks; ++k) {
        FillElectricCharge(run.V());
        run.Tick();
        if (!Healthy(run)) {
            r.healthy = false;
            break;
        }
        // the reference on the same clock: one 0.02 s tick of the C# model per physics substep
        const int sub = static_cast<int>(std::lround(dt / 0.02f));
        for (int s = 0; s < sub; ++s) va += RefForce(refs, va, 1.0f, 0.0f, 0.02f, 1.0) / r.massEff * 0.02;
        r.maxTilt = std::max(r.maxTilt, run.TiltDeg());
        const double t = (k + 1) * static_cast<double>(dt);
        for (int m = 0; m < 2; ++m)
            if (std::fabs(t - 10.0 * (m + 1)) < 0.5 * dt) {
                r.vAt[m] = run.ForwardSpeed();
                r.vRef[m] = va;
            }
    }
    if (warp > 1.0f) run.scene->SetPhysicsWarp(1.0f);
    return r;
}

}  // namespace

// =================================================================================================================================================
TEST_CASE("WP-29c rover rest on flat ground: the law's ride height, static equilibrium, no jitter, 60 s braked drift (Prospector, Bug-E)") {
    if (!wp20b::GameAvailable()) return;
    for (const char* craft : {"Prospector Rover", "Bug-E Buggy"}) {
        INFO(craft);
        const RestResult r = Rest(craft, 55.0, 1.0f);
        MESSAGE(std::string(craft) << ": drift " << r.horizDrift * 1e3 << " mm (vertical " << r.vertDrift * 1e3 << " mm), rms vz " << r.rmsVz * 1e3 << " mm/s, max w "
                      << r.maxW << " rad/s, compression " << r.meanCompression << " of the travel, wheel forces " << r.forceSum << " kN vs weight "
                      << r.weight << " kN, tyre-bottom error " << r.maxBottomError * 1e3 << " mm, body clearance " << r.minBodyClearance
                      << " m, joint-load error " << r.maxJointForceError << ", " << r.msPerTick << " ms/tick");
        CHECK(r.healthy);
        CHECK(r.grounded == r.wheels);
        CHECK(r.bottomedTicks == 0);
        CHECK(r.minBodyClearance > 0.05);                                      // the rover stands on its wheels
        CHECK(std::fabs(r.forceSum - r.weight) < 0.01 * r.weight);             // static equilibrium
        CHECK(r.maxBottomError < 0.005);                                        // the law's equilibrium tyre bottom is on the ground
        CHECK(r.horizDrift < 0.005);                                            // WHEEL-SPIKE.md 7: braked drift < 5 mm
        CHECK(std::fabs(r.vertDrift) < 0.001);
        CHECK(r.rmsVz < 0.001);                                                 // no jitter
        CHECK(r.maxW < 0.01);
        CHECK(r.maxJointForceError < 0.05);                                     // WP-20c sees the wheel loads on the wheel parts' joints
        // the spike's settled compression with the same law and proxy (WHEEL-SPIKE.md 4.1): M1 0.49, S2 0.53
        CHECK(r.meanCompression == doctest::Approx(std::string(craft) == "Prospector Rover" ? 0.49 : 0.53).epsilon(0.1));
    }
}

TEST_CASE("WP-29c rover drive at full throttle vs the C# motor model without slip (Prospector within 2 %, Bug-E within 10 %)") {
    if (!wp20b::GameAvailable()) return;
    for (const char* craft : {"Prospector Rover", "Bug-E Buggy"}) {
        INFO(craft);
        const DriveResult r = Drive(craft);
        MESSAGE(std::string(craft) << ": mass " << r.mass << " t (effective " << r.massEff << " t): v(10 s) " << r.vAt[0] << " (reference " << r.vRef[0] << "), v(20 s) "
                      << r.vAt[1] << " (" << r.vRef[1] << ") m/s, max tilt " << r.maxTilt << " deg");
        CHECK(r.healthy);
        const double tol = std::string(craft) == "Prospector Rover" ? 0.02 : 0.10;  // WHEEL-SPIKE.md 7 (WP-29c wheel_drive)
        for (int m = 0; m < 2; ++m) CHECK(r.vAt[m] == doctest::Approx(r.vRef[m]).epsilon(tol));
        CHECK(r.maxTilt < 10.0);
    }
}

TEST_CASE("WP-29c rover brakes from 8 m/s: stops within 25 % of the C# brake model capped by the locked-tyre curve, no flip") {
    if (!wp20b::GameAvailable()) return;
    for (const char* craft : {"Prospector Rover", "Bug-E Buggy"}) {
        INFO(craft);
        Run run(craft);
        run.Ticks(100);
        run.V().ctrlState.wheelThrottle = 1.0f;
        for (int k = 0; k < 3000 && run.ForwardSpeed() < 8.0; ++k) {
            FillElectricCharge(run.V());
            run.Tick();
        }
        run.V().ctrlState.wheelThrottle = 0.0f;
        const double v0 = run.ForwardSpeed();
        const Vector3d x0 = run.Com(), f0 = run.Forward();
        SetBrakes(run.V(), true);
        double tilt = 0, dist = -1, t = 0;
        for (int j = 0; j < 3000; ++j) {
            run.Tick();
            REQUIRE(Healthy(run));
            tilt = std::max(tilt, run.TiltDeg());
            if (Vector3d::Dot(run.Vel(), f0) < 0.05) {
                t = (j + 1) * 0.02;
                dist = Vector3d::Dot(run.Com() - x0, f0);
                break;
            }
        }
        // the reference: KSP's brake torque through rolling wheels, capped by the locked tyres sliding on their curve at slip = v
        // (VPP's force = curve(|slip|) x load, the loads summing to m g; no load transfer)
        auto refs = MotorRefs(run);
        const double meff = EffectiveMass(run, refs), m = run.Mass(), g = run.V().precalc.integrationAccel.magnitude();
        VehiclePhysics::TireFriction& tf = *run.Wheels().Wheels()[0]->Controller()->tireFriction;
        VehiclePhysics::TireFriction::ContactPatch cp;
        double va = v0, xa = 0, ta = 0;
        for (int j = 0; j < 3000 && va > 0.05; ++j) {
            const double fb = -RefForce(refs, va, 0.0f, 1.0f, 0.02f, 1.0);
            const double fSlide = static_cast<double>(tf.curve()->EvaluateForce(static_cast<float>(va), cp)) * m * g;
            va -= fb < fSlide ? fb / meff * 0.02 : fSlide / m * 0.02;
            xa += std::max(0.0, va) * 0.02;
            ta = (j + 1) * 0.02;
        }
        MESSAGE(std::string(craft) << ": from " << v0 << " m/s: " << dist << " m in " << t << " s (reference " << xa << " m in " << ta << " s), max tilt " << tilt << " deg");
        CHECK(dist > 0.0);
        CHECK(dist == doctest::Approx(xa).epsilon(0.25));
        CHECK(tilt < 30.0);
    }
}

TEST_CASE("WP-29c rover steers: wheelSteer +1 (KSP's left key) turns left at the kinematic rate of the steered wheels (Prospector), Bug-E turns left") {
    if (!wp20b::GameAvailable()) return;
    for (const char* craft : {"Prospector Rover", "Bug-E Buggy"}) {
        INFO(craft);
        Run run(craft);
        run.Ticks(100);
        // to 2.5 m/s, then coast: the kinematic model holds while the lateral acceleration stays low (the tyres in their adherent range)
        run.V().ctrlState.wheelThrottle = 0.4f;
        for (int k = 0; k < 1000 && run.ForwardSpeed() < 2.5; ++k) {
            FillElectricCharge(run.V());
            run.Tick();
        }
        run.V().ctrlState.wheelThrottle = 0.0f;
        const Vector3d f0 = run.Forward();
        // KSP's left key WHEEL_STEER_LEFT -> +1 (FlightInputHandler.cs:585); a quarter of it for the Prospector: a wider turn
        const bool prospector = std::string(craft) == "Prospector Rover";
        run.V().ctrlState.wheelSteer = prospector ? 0.25f : 1.0f;
        run.Ticks(prospector ? 150 : 100, [&] { FillElectricCharge(run.V()); });
        const Vector3d f = run.Forward();
        const double heading = std::atan2(Vector3d::Dot(Vector3d::Cross(f0, f), run.Up()), Vector3d::Dot(f0, f)) / kRad;  // Unity: + = right
        const double yawRate = Vector3d::Dot(run.Vp()->ClusterAngularVelocity(0), run.Up());
        const double v = run.ForwardSpeed();
        double tilt = run.TiltDeg();
        CHECK(Healthy(run));
        CHECK(heading < -5.0);  // a left turn
        CHECK(tilt < 20.0);
        if (std::string(craft) == "Prospector Rover") {
            // kinematic bicycle: yaw rate = v tan(delta) / L, delta the mean collider steer angle of the steered (front) wheels, L the
            // distance between the steered and the fixed axle along the drive direction
            double sumDelta = 0, front = 0, rear = 0;
            int nf = 0, nr = 0;
            const Vector3d com = Vector3d(run.V().CoM);
            for (auto& w : run.Wheels().Wheels()) {
                const float steer = w->Controller()->wheelCollider->steerAngle();
                const double along = Vector3d::Dot(Vector3d(w->colliderHost.position) - com, f);
                if (steer != 0.0f) {
                    sumDelta += steer;
                    front += along;
                    ++nf;
                } else {
                    rear += along;
                    ++nr;
                }
            }
            REQUIRE(nf > 0);
            REQUIRE(nr > 0);
            const double delta = sumDelta / nf * kRad, L = front / nf - rear / nr;
            const double kinematic = -v * std::tan(std::fabs(delta)) / L;  // a left turn: negative about the Unity up axis
            MESSAGE(std::string(craft) << ": " << heading << " deg in 3 s at " << v << " m/s, yaw rate " << yawRate << " rad/s vs kinematic " << kinematic
                          << " (steer " << delta / kRad << " deg, wheelbase " << L << " m), tilt " << tilt << " deg");
            CHECK(yawRate == doctest::Approx(kinematic).epsilon(0.15));
        } else {
            MESSAGE(std::string(craft) << ": " << heading << " deg in 2 s at " << v << " m/s, yaw rate " << yawRate << " rad/s (all-wheel steering), tilt " << tilt << " deg");
        }
    }
}

TEST_CASE("WP-29c physics-warp substeps: the policy (a tick longer than 0.02 s with a set-up wheel in the bubble runs ceil(dt / 0.02) substeps)") {
    if (!wp20b::GameAvailable()) return;
    Run run("Prospector Rover");
    WheelPhysicsHost& h = run.Host();
    // packed, no wheel set up yet: no substeps whatever the step
    CHECK(h.SubstepCount(0.08f) == 1);
    run.Ticks(kWheelsUp);
    REQUIRE(run.Wheels().SetUpCount() == 4);
    CHECK(h.SubstepCount(0.02f) == 1);
    CHECK(h.SubstepCount(0.04f) == 2);  // physics warp 2x (TimeWarp.cs:423: Time.fixedDeltaTime = 0.02 x rate)
    CHECK(h.SubstepCount(0.06f) == 3);
    CHECK(h.SubstepCount(0.08f) == 4);
    CHECK(h.SubstepCount(0.1f) == 5);
    CHECK(h.SubstepCount(0.03f) == 2);  // not a multiple: two substeps of 0.015 s
    h.options.substeps = false;
    CHECK(h.SubstepCount(0.08f) == 1);
    h.options.substeps = true;
    run.scene->SetPhysicsWarp(4.0f);
    run.Tick();
    CHECK(h.LastSubsteps() == 4);
    run.scene->SetPhysicsWarp(1.0f);
    run.Tick();
    CHECK(h.LastSubsteps() == 1);
}

TEST_CASE("WP-29c physics warp 2x / 3x / 4x with 0.02 s substeps: the 1x rest and drive; without them the rovers are not held (the spike's finding)") {
    if (!wp20b::GameAvailable()) return;
    for (const char* craft : {"Prospector Rover", "Bug-E Buggy"}) {
        INFO(craft);
        const RestResult r1 = Rest(craft, 55.0, 1.0f);
        for (float warp : {2.0f, 3.0f, 4.0f}) {
            const RestResult r = Rest(craft, 55.0, warp);
            MESSAGE(std::string(craft) << " rest at " << warp << "x with substeps: drift " << r.horizDrift * 1e3 << " mm, compression " << r.meanCompression
                                       << " (1x " << r1.meanCompression << "), " << r.msPerTick << " ms per tick");
            CHECK(r.healthy);
            CHECK(r.grounded == r.wheels);
            CHECK(r.bottomedTicks == 0);
            CHECK(r.horizDrift < 0.005);
            CHECK(std::fabs(r.vertDrift) < 0.001);
            CHECK(r.rmsVz < 0.001);
            CHECK(r.meanCompression == doctest::Approx(r1.meanCompression).epsilon(1e-3));
        }
        WheelPhysicsOptions off;
        off.substeps = false;
        const RestResult r4 = Rest(craft, 55.0, 4.0f, off);
        MESSAGE(std::string(craft) << " rest at 4x WITHOUT substeps (KSP's single 0.08 s step): drift " << r4.horizDrift << " m, rms vz " << r4.rmsVz
                                   << " m/s, bottomed ticks " << r4.bottomedTicks);
        CHECK(r4.horizDrift > 0.1);  // not held: the explicit suspension / VPP's brake model at 0.08 s (WHEEL-SPIKE.md 2.3)
        // the drive: identical to 1x with substeps (the frame-level forces held over the substeps), far off without them
        const DriveResult d1 = Drive(craft);
        const DriveResult d4 = Drive(craft, 4.0f);
        const DriveResult d4off = Drive(craft, 4.0f, off);
        MESSAGE(std::string(craft) << " drive at 4x: v(10 s) " << d4.vAt[0] << " / v(20 s) " << d4.vAt[1] << " m/s with substeps (1x " << d1.vAt[0] << " / " << d1.vAt[1]
                                   << "), " << d4off.vAt[0] << " / " << d4off.vAt[1] << " without");
        CHECK(d4.healthy);
        for (int m = 0; m < 2; ++m) CHECK(d4.vAt[m] == doctest::Approx(d1.vAt[m]).epsilon(0.005));
        CHECK(d4off.vAt[0] < 0.9 * d1.vAt[0]);
    }
}

TEST_CASE("WP-29c determinism: the same rover run twice gives the same Jolt state, bit for bit") {
    if (!wp20b::GameAvailable()) return;
    uint64_t hash[2] = {0, 0};
    double speed[2] = {0, 0};
    for (int k = 0; k < 2; ++k) {
        Run run("Bug-E Buggy");
        run.Ticks(100);
        run.V().ctrlState.wheelThrottle = 1.0f;
        run.V().ctrlState.wheelSteer = 0.5f;
        run.Ticks(300, [&] { FillElectricCharge(run.V()); });
        hash[k] = run.scene->pad.s.world.StateHash();
        speed[k] = run.ForwardSpeed();
    }
    CHECK(hash[0] == hash[1]);
    CHECK(speed[0] == speed[1]);
}

TEST_CASE("WP-29c the Smooth-vs-Lineal tyre curve switch (UNVERIFIED pending capture C11b): AsDecompiled keeps SmoothFriction, Lineal rebuilds") {
    if (!wp20b::GameAvailable()) return;
    for (TyreCurve c : {TyreCurve::AsDecompiled, TyreCurve::Lineal}) {
        WheelPhysicsOptions o;
        o.tyreCurve = c;
        Run run("Prospector Rover", o);
        run.Ticks(kWheelsUp + 1);
        for (auto& w : run.Wheels().Wheels()) {
            VehiclePhysics::TireFriction& tf = *w->Controller()->tireFriction;
            // ModuleWheelBase.cs:361 assigns the Lineal model either way; the curve object is the question
            CHECK(tf.model == VehiclePhysics::TireFriction::Model::Lineal);
            const bool smooth = dynamic_cast<VehiclePhysics::TireFriction::SmoothFriction*>(tf.curve()) != nullptr;
            const bool lineal = dynamic_cast<VehiclePhysics::TireFriction::LinealFriction*>(tf.curve()) != nullptr;
            CHECK(smooth == (c == TyreCurve::AsDecompiled));
            CHECK(lineal == (c == TyreCurve::Lineal));
        }
    }
}

TEST_CASE("WP-29c the wheel parts in the cluster: welded with their rb.mass, sprungMass = the wheel part's rb.mass, the collider radius x rescaleFactor") {
    if (!wp20b::GameAvailable()) return;
    Run run("Prospector Rover");
    run.Ticks(kWheelsUp + 2);
    VesselPhysics& vp = *run.Vp();
    REQUIRE(vp.ClusterCount() == 1);
    double partSum = 0.0;
    for (Part* p : run.V().parts) partSum += vp.PartMass(*p);
    CHECK(vp.ClusterMass(0) == doctest::Approx(partSum));
    for (auto& w : run.Wheels().Wheels()) {
        CHECK(vp.ClusterOf(*w->part) == 0);
        CHECK(vp.PartMass(*w->part) > 0.0f);
        const auto& wc = w->Controller()->wheelCollider->wheelColliderData;
        CHECK(wc.sprungMass == vp.PartMass(*w->part));  // Unity: Rigidbody.mass over its one WheelCollider
        CHECK(wc.radius == doctest::Approx(w->base->Get<float>("radius") * w->part->fields.rescaleFactor));  // ModuleWheelBase.cs:333
        CHECK(wc.mass == 1.0f);                                                                             // VPWheelCollider.cs:409
        CHECK(wc.forwardFriction.stiffness == 0.0f);
        CHECK(w->Controller()->wheelCollider->springSlerpRate == w->base->Get<float>("springSlerpRate"));
    }
}
