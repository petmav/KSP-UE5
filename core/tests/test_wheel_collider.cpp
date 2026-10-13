// WP-29c unit tests: the native WheelCollider (include/ksp/wheels/native_wheel_collider.hpp, src/wheels/native_wheel_collider.cpp): the PhysX
// 4.1 suspension / tyre law under KSP's wheels with Unity's mapping.
//
// The numeric gate is tests/data/wp29c_reference.json, written by tools/wp29c/suspension_reference.py: an independent Python implementation of
// the same PhysX statements (PxVehicleUpdate.cpp) that shares no code with the C++. 2,000 synthetic cases (random WheelCollider properties,
// poses, chassis motion, previous PhysX state; a ground plane at a chosen jounce: below max droop, inside the travel, past full compression,
// tilted, rejected for a zero distance / a normal along the travel / a ray start behind the plane, static, dynamic or kinematic actors; KSP's
// all-zero friction curves and random ones) and 400 friction-curve evaluations: every output 0 ULP (NaN == NaN, the sign of zero included).
// The remaining cases are derived from the C# (VPWheelCollider.SetupWheelCollider's curves) and the composition with WP-29a's
// VPWheelCollider.GetGroundHit; they are labelled.
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "doctest/doctest.h"
#include "ksp/core/unity_time.hpp"
#include "ksp/wheels/native_wheel_collider.hpp"
#include "ksp/wheels/vehicle_physics.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using namespace ksp::VehiclePhysics;
using json = nlohmann::json;

namespace {

const json& LoadRef29c() {
    static const json j = [] {
        std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp29c_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return j;
}

float J(const json& v) {
    if (v.is_string()) {
        const std::string s = v.get<std::string>();
        if (s == "nan") return std::nanf("");
        if (s == "inf") return INFINITY;
        if (s == "-inf") return -INFINITY;
        if (s == "-0") return -0.0f;
        FAIL("bad float token " << s);
    }
    return static_cast<float>(v.get<double>());
}
Vector3f V3(const json& a) { return Vector3f(J(a[0]), J(a[1]), J(a[2])); }
bool Same(float a, float b) {
    if (std::isnan(a) && std::isnan(b)) return true;
    uint32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    return x == y;
}

struct Diff {
    long bad = 0, total = 0;
    std::string first;
    void F(float got, const json& exp, const std::string& what) {
        ++total;
        if (!Same(got, J(exp))) {
            if (bad++ < 12) {
                std::ostringstream os;
                os.precision(9);
                os << what << ": got " << got << " expected " << J(exp) << "\n";
                first += os.str();
            }
        }
    }
    void V(const Vector3f& got, const json& exp, const std::string& what) {
        F(got.x, exp[0], what + ".x");
        F(got.y, exp[1], what + ".y");
        F(got.z, exp[2], what + ".z");
    }
    void B(bool got, const json& exp, const std::string& what) {
        ++total;
        if (got != exp.get<bool>()) {
            if (bad++ < 12) first += what + ": got " + std::to_string(got) + "\n";
        }
    }
};

WheelFrictionCurve Curve(const json& c) {
    WheelFrictionCurve w;
    w.extremumSlip = J(c["es"]);
    w.extremumValue = J(c["ev"]);
    w.asymptoteSlip = J(c["as"]);
    w.asymptoteValue = J(c["av"]);
    w.stiffness = J(c["stiff"]);
    return w;
}

}  // namespace

TEST_CASE("WP-29c native WheelCollider: geometry and the PhysX suspension / tyre law vs the independent Python reference, 0 ULP") {
    const json& ref = LoadRef29c();
    Diff d;
    Collider ground, actor;
    int grounded = 0, limits = 0, reactions = 0, tyres = 0;
    for (size_t i = 0; i < ref["cases"].size(); ++i) {
        const json& c = ref["cases"][i];
        const std::string where = "case " + std::to_string(i);
        const json& w = c["wc"];
        NativeWheelCollider wc;
        wc.center = V3(w["center"]);
        wc.radius = J(w["radius"]);
        wc.suspensionDistance = J(w["D"]);
        wc.suspensionSpring = JointSpring{J(w["spring"]), J(w["damper"]), J(w["t"])};
        wc.sprungMass = J(w["sprungMass"]);
        wc.mass = J(w["mass"]);
        wc.forceAppPointDistance = J(w["fapd"]);
        wc.motorTorque = J(w["motor"]);
        wc.brakeTorque = J(w["brake"]);
        wc.steerAngle = J(w["steer"]);
        wc.wheelDampingRate = J(w["damping"]);
        wc.forwardFriction = Curve(w["fwd"]);
        wc.sidewaysFriction = Curve(w["side"]);
        NativeTransform pose;
        pose.position = V3(c["pose"]["pos"]);
        pose.rotation = Quaternionf(J(c["pose"]["rot"][0]), J(c["pose"]["rot"][1]), J(c["pose"]["rot"][2]), J(c["pose"]["rot"][3]));
        const NativeWheelGeometry g = ComputeNativeWheelGeometry(wc, pose);
        const json& eg = c["geo"];
        d.V(g.top, eg["top"], where + " top");
        d.V(g.up, eg["up"], where + " up");
        d.V(g.right, eg["right"], where + " right");
        d.V(g.forward, eg["forward"], where + " forward");
        d.V(g.w, eg["w"], where + " w");
        d.F(g.maxCompression, eg["maxCompression"], where + " maxCompression");
        d.F(g.maxDroop, eg["maxDroop"], where + " maxDroop");
        d.F(g.moi, eg["moi"], where + " moi");
        d.V(g.centreRest, eg["centreRest"], where + " centreRest");
        d.V(g.forceAppPoint, eg["forceAppPoint"], where + " forceAppPoint");
        d.V(g.rayOrigin, eg["rayOrigin"], where + " rayOrigin");
        d.F(g.rayLength, eg["rayLength"], where + " rayLength");

        const json& qq = c["query"];
        NativeGroundQuery q;
        q.hit = qq["hit"].get<bool>();
        q.distance = J(qq["distance"]);
        q.point = V3(qq["point"]);
        q.normal = V3(qq["normal"]);
        const std::string kind = qq["actor"].get<std::string>();
        q.collider = kind == "none" || kind == "static" ? &ground : &actor;
        q.rigidbodyActor = kind == "dynamic" || kind == "kinematic";
        q.kinematicActor = kind == "kinematic";
        q.actorMass = J(qq["actorMass"]);
        q.actorVelocity = V3(qq["actorVel"]);
        q.actorAngularVelocity = V3(qq["actorAngVel"]);
        q.actorCenterOfMass = V3(qq["actorCom"]);
        NativeChassis ch;
        ch.centerOfMass = V3(c["chassis"]["com"]);
        ch.velocity = V3(c["chassis"]["vel"]);
        ch.angularVelocity = V3(c["chassis"]["angvel"]);
        PhysxWheelDyn prev;
        prev.jounce = J(c["prev"]["jounce"]);
        prev.wheelSpeed = J(c["prev"]["wheelSpeed"]);
        const NativeWheelStep s = SimulateNativeWheel(wc, g, q, ch, prev, J(c["dt"]), V3(c["gravity"]));
        const json& o = c["out"];
        d.B(s.grounded, o["grounded"], where + " grounded");
        d.F(s.dx, o["dx"], where + " dx");
        d.F(s.jounce, o["jounce"], where + " jounce");
        d.F(s.jounceSpeed, o["jounceSpeed"], where + " jounceSpeed");
        d.F(s.suspensionForce, o["suspensionForce"], where + " suspensionForce");
        d.B(s.limitActive, o["limitActive"], where + " limitActive");
        d.F(s.limitError, o["limitError"], where + " limitError");
        d.B(s.hitActorForceValid, o["hitActorForceValid"], where + " hitActorForceValid");
        for (const char* k : {"longSpeed", "latSpeed", "longSlip", "latSlip", "tireLongForce", "tireLatForce", "tireTorque", "wheelSpeed"}) {
            const float got = std::string(k) == "longSpeed"       ? s.longSpeed
                              : std::string(k) == "latSpeed"      ? s.latSpeed
                              : std::string(k) == "longSlip"      ? s.longSlip
                              : std::string(k) == "latSlip"       ? s.latSlip
                              : std::string(k) == "tireLongForce" ? s.tireLongForce
                              : std::string(k) == "tireLatForce"  ? s.tireLatForce
                              : std::string(k) == "tireTorque"    ? s.tireTorque
                                                                  : s.wheelSpeed;
            d.F(got, o[k], where + " " + k);
        }
        if (s.grounded) {
            d.V(s.force, o["force"], where + " force");
            d.V(s.forcePoint, o["forcePoint"], where + " forcePoint");
            d.V(s.limitDir, o["limitDir"], where + " limitDir");
            d.V(s.limitPoint, o["limitPoint"], where + " limitPoint");
            d.V(s.wheelBottom, o["wheelBottom"], where + " wheelBottom");
            d.V(s.tireLongDir, o["tireLongDir"], where + " tireLongDir");
            d.V(s.tireLatDir, o["tireLatDir"], where + " tireLatDir");
            d.V(s.tireForce, o["tireForce"], where + " tireForce");
            d.V(s.hit.point, o["hitPoint"], where + " hit.point");
            d.V(s.hit.normal, o["hitNormal"], where + " hit.normal");
            d.F(s.hit.force, o["suspensionForce"], where + " hit.force");
            d.F(s.hit.forwardSlip, o["longSlip"], where + " hit.forwardSlip");
            d.F(s.hit.sidewaysSlip, o["latSlip"], where + " hit.sidewaysSlip");
            d.V(s.hit.forwardDir, o["tireLongDir"], where + " hit.forwardDir");
            d.V(s.hit.sidewaysDir, o["tireLatDir"], where + " hit.sidewaysDir");
            CHECK(s.hit.collider == q.collider);
            ++grounded;
        }
        if (s.hitActorForceValid) {
            d.V(s.hitActorForce, o["hitActorForce"], where + " hitActorForce");
            d.V(s.hitActorForcePoint, o["hitActorForcePoint"], where + " hitActorForcePoint");
            ++reactions;
        }
        limits += s.limitActive ? 1 : 0;
        tyres += (s.tireLongForce != 0.0f || s.tireLatForce != 0.0f) ? 1 : 0;
        // the component after the step (what WheelCollider.isGrounded / rpm / GetGroundHit report)
        CommitNativeWheel(wc, s);
        d.F(wc.rpm, o["rpm"], where + " rpm");
        CHECK(wc.isGrounded == s.grounded);
        CHECK(wc.groundHitValid == s.grounded);
        if (d.bad > 0) break;
    }
    INFO(d.first);
    CHECK(d.bad == 0);
    CHECK(d.total > 100000);
    const json& st = ref["stats"];
    CHECK(grounded == st["grounded"].get<int>());
    CHECK(limits == st["limit"].get<int>());
    CHECK(reactions == st["actorForce"].get<int>());
    CHECK(tyres == st["tireForce"].get<int>());
    CHECK(grounded > 500);
    CHECK(limits > 100);
}

TEST_CASE("WP-29c native WheelCollider: friction curve evaluations vs the independent Python reference, 0 ULP") {
    const json& ref = LoadRef29c();
    Diff d;
    for (const json& r : ref["curves"]) d.F(EvaluateFrictionCurve(Curve(r["curve"]), J(r["slip"])), r["value"], "curve");
    INFO(d.first);
    CHECK(d.bad == 0);
    CHECK(d.total == 400);
}

// Derived from VPWheelCollider.SetupWheelCollider (VPWheelCollider.cs:397-412): KSP's WheelCollider has all-zero friction curves (default
// WheelFrictionCurve, stiffness 0) and mass 1, so the PhysX tyre force is exactly zero whatever the slip: the tyre is VPP's.
TEST_CASE("WP-29c native WheelCollider: KSP's WheelCollider (VPWheelCollider.SetupWheelCollider) gives no PhysX tyre force") {
    NativeRigidbody rb;
    NativeTransform host, wheelHost;
    auto ctl = KSPWheelController::Create(&rb, &host, &wheelHost);
    NativeWheelCollider& wc = ctl->wheelCollider->wheelColliderData;
    CHECK(wc.mass == 1.0f);
    for (const WheelFrictionCurve* c : {&wc.forwardFriction, &wc.sidewaysFriction}) {
        CHECK(c->stiffness == 0.0f);
        CHECK(c->extremumSlip == 0.0f);
        CHECK(c->extremumValue == 0.0f);
        CHECK(c->asymptoteSlip == 0.0f);
        CHECK(c->asymptoteValue == 0.0f);
    }
    wc.radius = 0.35f;
    wc.suspensionDistance = 0.2f;
    wc.suspensionSpring = JointSpring{150.0f, 12.0f, 0.3f};
    wc.sprungMass = 0.05f;
    wc.motorTorque = 1e-5f;
    wheelHost.position = Vector3f(0.0f, 0.5f, 0.0f);
    const NativeWheelGeometry g = ComputeNativeWheelGeometry(wc, wheelHost);
    for (int k = 0; k < 50; ++k) {
        NativeGroundQuery q;
        Collider ground;
        q.hit = true;
        q.collider = &ground;
        q.normal = Vector3f(0.0f, 1.0f, 0.0f);
        q.point = Vector3f(0.0f, 0.0f, 0.0f);
        q.distance = g.rayOrigin.y;
        NativeChassis ch;
        ch.velocity = Vector3f(static_cast<float>(k) - 25.0f, 0.0f, 0.3f * static_cast<float>(k));
        ch.angularVelocity = Vector3f(0.1f, -0.2f * static_cast<float>(k), 0.0f);
        PhysxWheelDyn prev;
        prev.wheelSpeed = 3.0f * static_cast<float>(k);
        const NativeWheelStep s = SimulateNativeWheel(wc, g, q, ch, prev, 0.02f);
        REQUIRE(s.grounded);
        CHECK(s.tireLongForce == 0.0f);
        CHECK(s.tireLatForce == 0.0f);
        CHECK(s.tireTorque == 0.0f);
        CHECK(s.longSlip == s.longSlip);  // finite slips: WheelHit.sidewaysSlip is read by ModuleWheelSteering.cs:283
    }
}

// The composition with WP-29a: the native step writes the WheelHit of GetGroundHit; VPWheelCollider.GetGroundHit (VPWheelCollider.cs:313) adds
// rb.velocity * fixedDeltaTime to the point and rebuilds forwardDir / sidewaysDir from the steer angle (its reference-frame fix), and
// VehicleBase.CalculateSuspensionTravel (VehicleBase.cs:697) turns it into the contact depth: here the compression from full extension.
TEST_CASE("WP-29c native WheelCollider: the WheelHit through VPWheelCollider.GetGroundHit and the contact depth") {
    const float savedFdt = Time::fixedDeltaTime;
    Time::fixedDeltaTime = 0.02f;
    NativeRigidbody rb;
    rb.velocity = Vector3f(1.5f, 0.0f, -2.0f);
    NativeTransform host, wheelHost;
    auto ctl = KSPWheelController::Create(&rb, &host, &wheelHost);
    ctl->Initialize();
    ctl->enabled(true);  // ModuleWheelBase.wheelSetup -> InopUpdate(force) -> wheel.enabled = true (VehicleBase.OnEnable enables the collider)
    VPWheelCollider& vpw = *ctl->wheelCollider;
    NativeWheelCollider& wc = vpw.wheelColliderData;
    vpw.radius = 0.4f;
    vpw.suspensionDistance = 0.3f;
    vpw.suspensionAnchor = 0.5f;
    wc.radius = 0.4f;
    wc.suspensionDistance = 0.3f;
    wc.center = vpw.center + Vector3f(0.0f, 0.15f, 0.0f);  // VPWheelCollider.UpdateWheelCollider: center + up D (1 - anchor)
    wc.suspensionSpring = JointSpring{200.0f, 10.0f, 0.25f};
    wheelHost.position = Vector3f(0.0f, 1.0f, 0.0f);
    const NativeWheelGeometry g = ComputeNativeWheelGeometry(wc, wheelHost);
    // the ground 0.1 m into the travel from full extension: wheel bottom at full extension is top - D - r = 1.15 - 0.3 - 0.4 = 0.45
    const float groundY = 0.55f;
    NativeGroundQuery q;
    Collider ground;
    q.hit = true;
    q.collider = &ground;
    q.normal = Vector3f(0.0f, 1.0f, 0.0f);
    q.point = Vector3f(0.0f, groundY, 0.0f);
    q.distance = g.rayOrigin.y - groundY;
    NativeChassis ch;
    ch.velocity = rb.velocity;
    const NativeWheelStep s = SimulateNativeWheel(wc, g, q, ch, PhysxWheelDyn{}, 0.02f);
    REQUIRE(s.grounded);
    // jounce relative to the rest point (t D = 0.075 above full extension): 0.1 - 0.075
    CHECK(s.jounce == doctest::Approx(0.025).epsilon(1e-5));
    CHECK(s.suspensionForce == doctest::Approx(200.0 * 0.025).epsilon(1e-4));
    CommitNativeWheel(wc, s);
    WheelHit hit;
    REQUIRE(vpw.GetGroundHit(hit));
    CHECK(hit.point.x == doctest::Approx(1.5 * 0.02));
    CHECK(hit.point.y == doctest::Approx(groundY));
    CHECK(hit.point.z == doctest::Approx(-2.0 * 0.02));
    CHECK(hit.force == s.suspensionForce);
    // VPP's contact depth: compression from full extension (GetContactDepth, VPWheelCollider.cs:351) = 0.1 m
    CHECK(vpw.GetContactDepth(hit.point, 0.3f) == doctest::Approx(0.1).epsilon(1e-4));
    // a disabled collider is reset: no hit, no previous jounce (Unity drops the PhysX wheel)
    ResetNativeWheel(wc);
    CHECK_FALSE(vpw.GetGroundHit(hit));
    CHECK(wc.physx.jounce == PhysxWheelDyn::kNoJounce);
    Time::fixedDeltaTime = savedFdt;
}
