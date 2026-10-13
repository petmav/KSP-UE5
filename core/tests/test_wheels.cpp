// WP-29a unit tests: the VehiclePhysics (VPP) wheel core and KSPWheelController (include/ksp/wheels/, src/wheels/).
//
// The numeric gate is tests/data/wp29a_reference.json, written by tools/wp29a/vpp_reference.py: an independent Python implementation of the
// same C# (VehiclePhysics/TireFriction.cs, Wheel.cs, DirectDrive.cs, Solver.cs, VehicleBase.cs, VPWheelCollider.cs, KSPWheelController.cs)
// that shares no code with the port and was written before it. It covers
//   * the five tyre curves (Flat, Lineal, Smooth, Parametric, Pacejka) over slip / load / grip / friction multiplier sweeps,
//   * the wheel / DirectDrive / Solver chain driven directly (Euler with 1 / 4 / 8 substeps and RK4, dt 0.005 .. 0.1, wheel inertia floor
//     0.01 t m^2, the 1000 kN m locking brake, motor ramps),
//   * the whole KSPWheelController.FixedUpdate / Update pipeline over 24 scripted scenes (11 families: flat drive, airborne transitions,
//     slopes, moving dynamic / kinematic surfaces, landing-leg lock with the 10 kN minimum downforce, steering, ground materials, the
//     reference-frame fix switched off, RK4 and substeps, the inertia floor, Lineal rebuilt / not rebuilt).
// Every output is compared bit for bit (0 ULP, NaN == NaN). The native seam (Transform / Rigidbody / WheelCollider) is scripted per tick from
// the same file; its per-op float32 formulations (Transform.up, InverseTransformPoint, Rigidbody.GetPointVelocity, Quaternion.AngleAxis) are
// written from Unity's documented formulation and are UNVERIFIED against the game (STATUS.md WP-29a).
// The remaining cases are derived from the C# control flow (lifecycle, errors, impacts, pause, gravity, properties); they are labelled.
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <limits>
#include <sstream>

#include "doctest/doctest.h"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/high_logic.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/wheels/vehicle_physics.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
using namespace ksp::VehiclePhysics;
using json = nlohmann::json;

namespace {

json LoadRef29() {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp29a_reference.json", std::ios::binary);
    REQUIRE(f.good());
    return json::parse(f);
}

// the reference writes a float as a number, or "nan" / "inf" / "-inf" / "-0"
float J(const json& v) {
    if (v.is_string()) {
        const std::string s = v.get<std::string>();
        if (s == "nan") return std::nanf("");
        if (s == "inf") return INFINITY;
        if (s == "-inf") return -INFINITY;
        if (s == "-0") return -0.0f;
        FAIL("bad float token " << s);
    }
    return v.get<float>();
}

bool Same(float a, float b) {
    if (std::isnan(a) && std::isnan(b)) return true;
    uint32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    return x == y;
}

int64_t UlpDiff(float a, float b) {
    int32_t x, y;
    std::memcpy(&x, &a, 4);
    std::memcpy(&y, &b, 4);
    if (x < 0) x = INT32_MIN - x;
    if (y < 0) y = INT32_MIN - y;
    return static_cast<int64_t>(x) - static_cast<int64_t>(y);
}

Vector3f V3j(const json& a) { return Vector3f(J(a[0]), J(a[1]), J(a[2])); }
Quaternionf Qj(const json& a) { return Quaternionf(J(a[0]), J(a[1]), J(a[2]), J(a[3])); }

struct Diff {
    int64_t bad = 0, total = 0;
    std::string first;
    void Check(float got, const json& exp, const std::string& what) {
        total++;
        const float e = J(exp);
        if (!Same(got, e)) {
            if (bad++ < 12) {
                std::ostringstream os;
                os.precision(9);
                os << what << ": got " << got << " expected " << e << " (" << UlpDiff(got, e) << " ULP)\n";
                first += os.str();
            }
        }
    }
};

// Time.deltaTime / fixedDeltaTime are process-wide: put them back
struct TimeScope {
    float dt = Time::deltaTime, fdt = Time::fixedDeltaTime;
    ~TimeScope() {
        Time::deltaTime = dt;
        Time::fixedDeltaTime = fdt;
    }
};

TireFriction::Settings SettingsOf(const json& d) {
    TireFriction::Settings s;
    if (d.contains("adherent")) s.adherent = Vector2f(J(d["adherent"][0]), J(d["adherent"][1]));
    if (d.contains("peak")) s.peak = Vector2f(J(d["peak"][0]), J(d["peak"][1]));
    if (d.contains("limit")) s.limit = Vector2f(J(d["limit"][0]), J(d["limit"][1]));
    if (d.contains("a")) s.a = J(d["a"]);
    if (d.contains("b")) s.b = J(d["b"]);
    if (d.contains("c")) s.c = J(d["c"]);
    if (d.contains("d")) s.d = J(d["d"]);
    if (d.contains("A")) s.A = J(d["A"]);
    if (d.contains("B")) s.B = J(d["B"]);
    if (d.contains("C")) s.C = J(d["C"]);
    if (d.contains("D")) s.D = J(d["D"]);
    if (d.contains("E")) s.E = J(d["E"]);
    return s;
}


// Quaternion.AngleAxis as the Python reference writes it (the native AxisAngleToQuaternionSafe formulation, cosf / sinf of the half angle taken
// from the correctly rounded double). The production binding (UCRT sinf / cosf) is bounded against it in its own case below.
Quaternionf RefAngleAxis(float angle, const Vector3f& axis) {
    float t = angle / 360.0f;
    t = t * 2.0f;
    const float rad = static_cast<float>(static_cast<double>(t) * 3.14159265358979323846);
    const float s2 = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
    const float mag = static_cast<float>(std::sqrt(static_cast<double>(s2)));
    if (mag > 1e-6f) {
        const float half = rad * 0.5f;
        const float w = static_cast<float>(std::cos(static_cast<double>(half)));
        const float sn = static_cast<float>(std::sin(static_cast<double>(half))) / mag;
        return Quaternionf(sn * axis.x, sn * axis.y, sn * axis.z, w);
    }
    return Quaternionf::identity();
}

struct AngleAxisScope {
    std::function<Quaternionf(float, const Vector3f&)> saved = NativeOps::AngleAxis;
    AngleAxisScope() { NativeOps::AngleAxis = RefAngleAxis; }
    ~AngleAxisScope() { NativeOps::AngleAxis = saved; }
};

TireFriction::Model ModelOf(const std::string& m) {
    if (m == "flat") return TireFriction::Model::Flat;
    if (m == "lineal") return TireFriction::Model::Lineal;
    if (m == "smooth") return TireFriction::Model::Smooth;
    if (m == "parametric") return TireFriction::Model::Parametric;
    return TireFriction::Model::Pacejka;
}

}  // namespace

// ---------------------------------------------------------------------------------------------------------------------------------------
TEST_CASE("WP-29a tyre friction: five curves vs the independent Python reference, 0 ULP") {
    const json ref = LoadRef29();
    const json& fr = ref["friction"];
    const json& models = fr["models"];
    Diff d;
    for (const json& row : fr["rows"]) {
        const json& sd = fr["sets"][row[0].get<size_t>()];
        TireFriction tf;
        *tf.settings = SettingsOf(sd);
        tf.model = ModelOf(models[row[1].get<size_t>()].get<std::string>());
        tf.frictionMultiplier = J(row[2]);
        tf.SetupFrictionCurves();
        TireFriction::ContactPatch cp;
        cp.slip = Vector2f(J(row[3]), J(row[4]));
        cp.load = J(row[5]);
        cp.groundGrip = J(row[6]);
        const Vector2f f = tf.GetForce(cp);
        d.Check(f.x, row[7], "force.x");
        d.Check(f.y, row[8], "force.y");
        d.Check(tf.IsAdherentSlip(cp) ? 1.0f : 0.0f, row[9], "adherent");
        d.Check(tf.GetAdherentSlipForward(cp), row[10], "adherentForward");
        d.Check(tf.GetAdherentSlipSideways(cp), row[11], "adherentSideways");
        const Vector2f af = tf.GetAdherentForce(cp);
        d.Check(af.x, row[12], "adherentForce.x");
        d.Check(af.y, row[13], "adherentForce.y");
        d.Check(tf.GetAdherentSlipBounds(cp).x, row[14], "adherentBound");
        d.Check(tf.GetPeakSlipBounds(cp).x, row[15], "peakBound");
        d.Check(tf.GetLimitSlipBounds(cp).x, row[16], "limitBound");
        d.Check(tf.curve()->EvaluateForce(cp.slip.magnitude(), cp), row[17], "EvaluateForce");
        if (d.bad) break;
    }
    INFO(d.first);
    MESSAGE("friction: " << fr["rows"].size() << " rows, " << d.total << " values compared");
    CHECK(d.bad == 0);
    CHECK(d.total == static_cast<int64_t>(fr["rows"].size()) * 11);
}

TEST_CASE("WP-29a wheel / DirectDrive / Solver chain vs the independent Python reference, 0 ULP (inertia floor, locking brake, Euler 1/4/8, RK4)") {
    const json ref = LoadRef29();
    const json& wr = ref["wheel"];
    const size_t nIn = wr["nInputs"].get<size_t>();
    Diff d;
    int64_t ticks = 0, floorCases = 0, lockCases = 0, rk4Cases = 0;
    for (const json& sc : wr["scenarios"]) {
        Wheel w;
        *w.tireFriction->settings = SettingsOf(sc["settings"]);
        w.tireFriction->model = ModelOf(sc["model"].get<std::string>());
        w.tireFriction->frictionMultiplier = J(sc["fm"]);
        w.tireFriction->SetupFrictionCurves();
        w.radius = J(sc["radius"]);
        w.mass = J(sc["mass"]);
        w.RecalculateConstants();
        w.RecalculateVars();
        DirectDrive dd;
        Block::Connect(w, 0, dd, 0);
        Solver solver;
        REQUIRE(solver.Initialize({&w}, true));
        const float dt = J(sc["dt"]);
        d.Check(w.inertia(), sc["I"], "inertia");
        if (w.inertia() == 0.01f && w.mass * w.radius * w.radius * 0.5f < 0.01f) floorCases++;
        solver.Integrate(0.0f, dt, 1, false);
        w.L = J(sc["L0"]);
        w.RecalculateVars();
        const int steps = sc["steps"].get<int>();
        const bool rk4 = sc["rk4"].get<int>() != 0;
        if (rk4) rk4Cases++;
        float t = 0.0f;
        for (const json& tk : sc["ticks"]) {
            w.load = J(tk[2]);
            w.grip = J(tk[3]);
            w.V = Vector2f(J(tk[0]), J(tk[1]));
            w.Fext = Vector2f(J(tk[4]), J(tk[5]));
            const float n3 = Mathf::Max(w.brakeTorque(), f32store(fx(w.load) * fx(w.radius)));
            w.Tr = Mathf::Clamp(J(tk[12]), f32store(0.0 - fx(n3)), n3);
            dd.motorInput = J(tk[8]);
            dd.maxMotorTorque = J(tk[9]);
            dd.maxRpm = J(tk[10]);
            dd.damping = J(tk[11]);
            if (J(tk[6]) >= 1000.0f) lockCases++;
            w.AddBrakeTorque(J(tk[6]));
            w.AddBrakeTorque(J(tk[7]));
            solver.Integrate(t, dt, steps, rk4);
            t = f32store(fx(t) + fx(dt));
            const size_t o = nIn;
            d.Check(w.L, tk[o + 0], "L");
            d.Check(w.angularVelocity(), tk[o + 1], "w");
            d.Check(w.T, tk[o + 2], "T");
            d.Check(w.Tr, tk[o + 3], "Tr");
            d.Check(w.F.x, tk[o + 4], "F.x");
            d.Check(w.F.y, tk[o + 5], "F.y");
            d.Check(w.m_Ty, tk[o + 6], "Ty");
            d.Check(w.m_tireForce.x, tk[o + 7], "tireForce.x");
            d.Check(w.m_tireForce.y, tk[o + 8], "tireForce.y");
            d.Check(w.m_isAdherent ? 1.0f : 0.0f, tk[o + 9], "isAdherent");
            d.Check(w.m_adherentSlip.x, tk[o + 10], "adherentSlip.x");
            d.Check(w.m_adherentSlip.y, tk[o + 11], "adherentSlip.y");
            d.Check(w.m_adherentForce.x, tk[o + 12], "adherentForce.x");
            d.Check(w.m_adherentForce.y, tk[o + 13], "adherentForce.y");
            d.Check(w.isResting ? 1.0f : 0.0f, tk[o + 14], "isResting");
            d.Check(w.driveTorque(), tk[o + 15], "inTd");
            d.Check(w.brakeTorque(), tk[o + 16], "inTb");
            d.Check(w.m_forwardSlip, tk[o + 17], "forwardSlip");
            d.Check(dd.torque(), tk[o + 18], "outTd");
            d.Check(Solver::time(), tk[o + 19], "Solver.time");
            d.Check(Solver::deltaTime(), tk[o + 20], "Solver.deltaTime");
            ticks++;
            if (d.bad) break;
        }
        if (d.bad) break;
    }
    INFO(d.first);
    MESSAGE("wheel chain: " << wr["scenarios"].size() << " scenarios, " << ticks << " ticks, " << d.total << " values compared; inertia-floor scenarios "
                            << floorCases << ", locking-brake ticks " << lockCases << ", RK4 scenarios " << rk4Cases);
    CHECK(d.bad == 0);
    CHECK(ticks >= 2000);
    CHECK(floorCases > 0);  // the 0.01 t m^2 inertia floor is exercised
    CHECK(lockCases > 0);   // and the 1000 kN m locking brake
    CHECK(rk4Cases > 0);
}

namespace {

// ---- the vehicle scenes ------------------------------------------------------------------------------------------------------------------
struct MatManager : GroundMaterialManagerBase {
    std::map<const PhysicMaterial*, GroundMaterial*> byMat;
    GroundMaterial* GetGroundMaterial(VehicleBase&, const GroundMaterialHit& hit) override {
        auto it = byMat.find(hit.physicMaterial.get());
        return it == byMat.end() ? nullptr : it->second;
    }
};

struct Scene {
    NativeRigidbody rb;
    NativeTransform hostTf, wcTf;
    std::shared_ptr<KSPWheelController> c;
    MatManager mgr;
    std::map<int, std::shared_ptr<PhysicMaterial>> mats;
    std::map<int, GroundMaterial> gm;
    std::vector<std::unique_ptr<NativeRigidbody>> surfs;
    std::vector<std::unique_ptr<Collider>> colliders;
    float rayDistance = -1.0f;

    explicit Scene(const json& spec) {
        const json& cs = spec["ctl"];
        const json& wcs = spec["wc"];
        const json& nat = spec["native"];
        rb.mass = J(spec["rbMass"]);
        Time::fixedDeltaTime = J(cs["fixedDeltaTime"]);  // read by VehicleBase.OnEnable's priming Integrate
        Time::deltaTime = Time::fixedDeltaTime;
        c = KSPWheelController::Create(&rb, &hostTf, &wcTf);
        c->maxDriveTorque = J(cs["maxDriveTorque"]);
        c->maxBrakeTorque = J(cs["maxBrakeTorque"]);
        c->maxBrakeTorqueRest = J(cs["maxBrakeTorqueRest"]);
        c->maxSteerAngle = J(cs["maxSteerAngle"]);
        c->maxRpm = J(cs["maxRpm"]);
        c->wheelDamping = J(cs["wheelDamping"]);
        c->driveResponse = J(cs["driveResponse"]);
        c->brakeResponse = J(cs["brakeResponse"]);
        c->steeringResponse = J(cs["steeringResponse"]);
        c->minimumdownForce = J(cs["minimumdownForce"]);
        c->tireForceSharpness = J(cs["tireForceSharpness"]);
        c->tireForceSmoothing = cs["tireForceSmoothing"].get<int>() != 0;
        c->tireSideDeflection = cs["tireSideDeflection"].get<int>() != 0;
        c->tireSideDeflectionRate = J(cs["tireSideDeflectionRate"]);
        c->tireImpulseRatio = J(cs["tireImpulseRatio"]);
        c->integrationSteps = cs["integrationSteps"].get<int>();
        c->integrationUseRK4 = cs["integrationUseRK4"].get<int>() != 0;
        c->gravity.Value(V3j(cs["gravity"]));
        // the module mutates the controller's own TireFriction settings and sets the model field; the curve only follows a rebuild
        const json& fr = spec["friction"];
        const TireFriction::Settings s = SettingsOf(fr["settings"]);
        TireFriction::Settings& live = *c->tireFriction->settings;
        if (fr["settings"].contains("adherent")) live.adherent = s.adherent;
        if (fr["settings"].contains("peak")) live.peak = s.peak;
        if (fr["settings"].contains("limit")) live.limit = s.limit;
        c->tireFriction->frictionMultiplier = J(fr["frictionMultiplier"]);
        c->tireFriction->model = ModelOf(fr["model"].get<std::string>());
        if (fr["rebuild"].get<int>() != 0) c->tireFriction->SetupFrictionCurves();
        VPWheelCollider& wc = *c->wheelCollider;
        wc.mass = J(wcs["mass"]);
        wc.radius = J(wcs["radius"]);
        wc.center = V3j(wcs["center"]);
        wc.suspensionDistance = J(wcs["suspensionDistance"]);
        wc.suspensionAnchor = J(wcs["suspensionAnchor"]);
        wc.springRate = J(wcs["springRate"]);
        wc.damperRate = J(wcs["damperRate"]);
        wc.groundPenetration = J(wcs["groundPenetration"]);
        wc.springSlerpRate = J(wcs["springSlerpRate"]);
        wc.suspensionOffset = J(wcs["suspensionOffset"]);
        wc.disableSuspensionMovement = wcs.contains("disableSuspensionMovement") && wcs["disableSuspensionMovement"].get<int>() != 0;
        VPWheelCollider::disableWheelReferenceFrameFix = wcs.contains("disableWheelReferenceFrameFix") && wcs["disableWheelReferenceFrameFix"].get<int>() != 0;
        VPWheelCollider::disableSteerAngleFix = wcs.contains("disableSteerAngleFix") && wcs["disableSteerAngleFix"].get<int>() != 0;
        NativeWheelCollider& nw = wc.wheelColliderData;
        nw.center = V3j(nat["center"]);
        nw.suspensionDistance = J(nat["suspensionDistance"]);
        nw.radius = J(nat["radius"]);
        nw.sprungMass = J(nat["sprungMass"]);
        nw.suspensionSpring = JointSpring{J(nat["spring"]), J(nat["damper"]), J(nat["target"])};
        wc.visualRaycast = [this](const Vector3f&, const Vector3f&, float) { return rayDistance; };
        if (!spec["materials"].is_null()) {
            for (auto it = spec["materials"].begin(); it != spec["materials"].end(); ++it) {
                const int id = std::stoi(it.key());
                mats[id] = std::make_shared<PhysicMaterial>();
                mats[id]->id = id;
                GroundMaterial g;
                g.grip = J(it.value()["grip"]);
                g.drag = J(it.value()["drag"]);
                gm[id] = g;
            }
            mats[0] = std::make_shared<PhysicMaterial>();
            for (auto& kv : gm) mgr.byMat[mats[kv.first].get()] = &gm[kv.first];
            c->groundMaterialManager = &mgr;
        }
        c->Initialize();     // ModuleWheelBase.wheelSetup: `if (!wheel.enabled) wheel.Initialize()` ...
        c->enabled(true);    // ... then `wheel.enabled = true` (InopUpdate) runs VehicleBase.OnEnable
    }
};

}  // namespace

TEST_CASE("WP-29a KSPWheelController.FixedUpdate / Update pipeline vs the independent Python reference, 0 ULP (11 scene families)") {
    const json ref = LoadRef29();
    const json& vh = ref["vehicle"];
    const json& cols = vh["cols"];
    TimeScope timeScope;
    AngleAxisScope angleAxisScope;
    const bool savedFix = VPWheelCollider::disableWheelReferenceFrameFix, savedSteer = VPWheelCollider::disableSteerAngleFix;
    Diff d;
    int64_t ticks = 0, grounded = 0, airborne = 0, families = 0;
    std::map<std::string, int> famSeen;
    for (const json& sc : vh["scenarios"]) {
        const json& spec = sc["spec"];
        famSeen[spec["family"].get<std::string>()]++;
        Scene sn(spec);
        REQUIRE(sn.c->enabled());
        REQUIRE(sn.c->initialized());
        const size_t nTicks = sc["ticks"].size();
        for (size_t k = 0; k < nTicks; k++) {
            const json& t = sc["ticks"][k];
            const json& expect = sc["rows"][k];
            const float dt = J(t["dt"]);
            Time::fixedDeltaTime = dt;
            Time::deltaTime = dt;
            sn.rb.mass = J(t["rb"]["mass"]);
            sn.rb.velocity = V3j(t["rb"]["vel"]);
            sn.rb.angularVelocity = V3j(t["rb"]["ang"]);
            sn.rb.worldCenterOfMass = V3j(t["rb"]["com"]);
            sn.rb.forces.clear();
            sn.hostTf.position = V3j(t["rb"]["pos"]);
            sn.hostTf.rotation = Qj(t["rbq"]);
            sn.wcTf.position = V3j(t["wcpos"]);
            sn.wcTf.rotation = Qj(t["wcq"]);
            VPWheelCollider& wc = *sn.c->wheelCollider;
            wc.wheelColliderData.sprungMass = J(t["sprungMass"]);
            if (!t["runtime"].is_null()) {
                wc.runtimeSpringRate = J(t["runtime"][0]);
                wc.runtimeDamperRate = J(t["runtime"][1]);
                wc.runtimeSuspensionTravel = J(t["runtime"][2]);
                wc.runtimeExtraDownforce = J(t["runtime"][3]);
            }
            sn.c->driveInput = J(t["drive"]);
            sn.c->brakeInput = J(t["brake"]);
            sn.c->steerInput = J(t["steer"]);
            NativeRigidbody* surf = nullptr;
            sn.surfs.clear();
            sn.colliders.clear();
            if (!t["hit"].is_null()) {
                const json& h = t["hit"];
                WheelHit wh;
                wh.point = V3j(h["point"]);
                wh.normal = V3j(h["normal"]);
                wh.forwardDir = V3j(h["fwd"]);
                wh.sidewaysDir = V3j(h["side"]);
                wh.force = J(h["force"]);
                if (!h["surf"].is_null()) {
                    const json& s = h["surf"];
                    auto sr = std::make_unique<NativeRigidbody>();
                    sr->mass = J(s["mass"]);
                    sr->isKinematic = s["kinematic"].get<int>() != 0;
                    sr->velocity = V3j(s["vel"]);
                    sr->angularVelocity = V3j(s["ang"]);
                    sr->worldCenterOfMass = V3j(s["com"]);
                    surf = sr.get();
                    sn.surfs.push_back(std::move(sr));
                }
                auto col = std::make_unique<Collider>();
                col->attachedRigidbody = surf;
                const int matId = h["mat"].get<int>();
                col->sharedMaterial = sn.mats.empty() ? std::make_shared<PhysicMaterial>() : sn.mats[matId];
                wh.collider = col.get();
                sn.colliders.push_back(std::move(col));
                wc.wheelColliderData.groundHitValid = true;
                wc.wheelColliderData.groundHit = wh;
            } else {
                wc.wheelColliderData.groundHitValid = false;
                wc.wheelColliderData.groundHit = WheelHit();
            }
            for (const json& fr : t["frames"]) {
                Time::deltaTime = J(fr[0]);
                sn.rayDistance = fr[1].is_null() ? -1.0f : J(fr[1]);
                sn.c->Update();
            }
            Time::deltaTime = dt;
            sn.c->FixedUpdate();

            VehicleBase::WheelState& w = *sn.c->wheelState()[0];
            Wheel& wh = *sn.c->wheelAt(0);
            const NativeWheelCollider& col = wc.wheelColliderData;
            std::vector<float> row;
            auto push = [&row](float v) { row.push_back(v); };
            auto push3 = [&row](const Vector3f& v) {
                row.push_back(v.x);
                row.push_back(v.y);
                row.push_back(v.z);
            };
            auto push2 = [&row](const Vector2f& v) {
                row.push_back(v.x);
                row.push_back(v.y);
            };
            push(w.grounded ? 1.0f : 0.0f);
            push(w.contactDepth);
            push(w.suspensionCompression);
            push(w.contactSpeed);
            push(w.damperForce);
            push(w.downforce);
            push(w.normalizedLoad);
            push(w.contactAngle);
            push3(w.wheelVelocity);
            push2(w.localWheelVelocity);
            push3(w.surfaceForce);
            push2(w.localSurfaceForce);
            push2(w.externalTireForce);
            push(w.angularVelocity);
            push2(w.tireForce);
            push(w.reactionTorque);
            push2(w.tireSlip);
            push(w.combinedTireSlip);
            push2(w.lastTireForce);
            push(w.driveTorque);
            push(w.brakeTorque);
            push(wh.L);
            push(wh.Tr);
            push(wh.F.x);
            push(wh.F.y);
            push(wh.isResting ? 1.0f : 0.0f);
            push(wh.angularVelocity());
            push(wh.load);
            push(wh.grip);
            push(sn.c->speed());
            push(sn.c->speedAngle());
            push3(sn.c->localAcceleration());
            push(sn.rb.maxDepenetrationVelocity);
            push3(col.center);
            push(col.suspensionDistance);
            push(col.radius);
            push(col.motorTorque);
            push(col.suspensionSpring.spring);
            push(col.suspensionSpring.damper);
            push(col.suspensionSpring.targetPosition);
            push(col.forceAppPointDistance);
            push(col.steerAngle);
            push(wc.canSleep ? 1.0f : 0.0f);
            push(sn.c->driveState);
            push(sn.c->brakeState);
            push(sn.c->steerState);
            push(w.steerAngle);
            push(Solver::time());
            if (w.grounded) {
                push3(w.hit.point);
                push3(w.hit.forwardDir);
                push3(w.hit.sidewaysDir);
            } else {
                for (int i = 0; i < 9; i++) push(0.0f);
            }
            push(wc.angularPosition());
            push(wc.contactDistance());
            push3(wc.suspensionPosition());
            push3(sn.c->localDragVelocity());
            push(wc.lastRuntimeSpringRate());
            push(wc.lastRuntimeDamperRate());
            push(wc.lastRuntimeSuspensionTravel());
            push(wc.steerAngle());
            push(static_cast<float>(sn.rb.forces.size()));
            if (!sn.rb.forces.empty()) {
                push3(sn.rb.forces[0].force);
                push3(sn.rb.forces[0].position);
            } else {
                for (int i = 0; i < 6; i++) push(0.0f);
            }
            if (surf != nullptr) {
                push(static_cast<float>(surf->forces.size()));
                if (!surf->forces.empty()) {
                    push3(surf->forces[0].force);
                    push3(surf->forces[0].position);
                } else {
                    for (int i = 0; i < 6; i++) push(0.0f);
                }
            } else {
                push(-1.0f);
                for (int i = 0; i < 6; i++) push(0.0f);
            }
            REQUIRE(row.size() == cols.size());
            REQUIRE(expect.size() == cols.size());
            for (size_t i = 0; i < row.size(); i++) {
                d.Check(row[i], expect[i], spec["family"].get<std::string>() + " tick " + std::to_string(k) + " " + cols[i].get<std::string>());
            }
            ticks++;
            if (w.grounded) grounded++; else airborne++;
            if (d.bad) break;
        }
        if (d.bad) break;
    }
    VPWheelCollider::disableWheelReferenceFrameFix = savedFix;
    VPWheelCollider::disableSteerAngleFix = savedSteer;
    INFO(d.first);
    MESSAGE("vehicle: " << vh["scenarios"].size() << " scenes, " << ticks << " ticks (" << grounded << " grounded, " << airborne << " airborne), " << d.total
                        << " values compared");
    CHECK(d.bad == 0);
    CHECK(ticks >= 3000);
    CHECK(grounded > 1000);
    CHECK(airborne > 50);
    families = static_cast<int64_t>(famSeen.size());
    CHECK(families == 12);
}

// ---------------------------------------------------------------------------------------------------------------------------------------
namespace {
// a bare controller for the derived (non-reference) cases
struct Bare {
    NativeRigidbody rb;
    NativeTransform host, wcT;
    std::shared_ptr<KSPWheelController> c;
    Bare() {
        c = KSPWheelController::Create(&rb, &host, &wcT);
        c->wheelCollider->radius = 0.4f;
        c->wheelCollider->mass = 0.1f;
        c->wheelCollider->suspensionDistance = 0.5f;
        c->wheelCollider->wheelColliderData.sprungMass = 0.1f;
        c->wheelCollider->wheelColliderData.suspensionDistance = 0.5f;
        rb.mass = 1.0f;
    }
};
}  // namespace

TEST_CASE("WP-29a tyre curves: Smooth vs Lineal, constraints, the Lineal-never-rebuilt quirk (UNVERIFIED pending capture C11)") {
    // derived from TireFriction.cs: SmoothFriction.CubicLerp is the cubic Hermite blend, LinealFriction.FastLerp the chord
    TireFriction smooth, lineal;
    lineal.model = TireFriction::Model::Lineal;
    lineal.SetupFrictionCurves();
    TireFriction::ContactPatch cp;
    cp.load = 1.0f;
    cp.groundGrip = 1.0f;
    // below the adherent slip both are the adherent plateau
    CHECK(smooth.curve()->EvaluateForce(0.25f, cp) == doctest::Approx(0.95f));
    CHECK(lineal.curve()->EvaluateForce(0.25f, cp) == doctest::Approx(0.95f));
    // midway between adherent (0.5, 0.95) and peak (1.5, 1.1): the cubic Hermite blend of two points is symmetric, so both give 1.025
    CHECK(smooth.curve()->EvaluateForce(1.0f, cp) == doctest::Approx(1.025f));
    CHECK(lineal.curve()->EvaluateForce(1.0f, cp) == doctest::Approx(1.025f));
    // at a quarter they differ (smoothstep(0.25) = 0.15625 vs 0.25)
    CHECK(smooth.curve()->EvaluateForce(0.75f, cp) == doctest::Approx(0.95f + 0.15f * 0.15625f));
    CHECK(lineal.curve()->EvaluateForce(0.75f, cp) == doctest::Approx(0.95f + 0.15f * 0.25f));
    // beyond the limit slip the limit plateau
    CHECK(smooth.curve()->EvaluateForce(9.0f, cp) == doctest::Approx(0.8f));
    CHECK(lineal.curve()->EvaluateForce(9.0f, cp) == doctest::Approx(0.8f));
    // the field assignment alone changes nothing: ModuleWheelBase.cs:361 sets model = Lineal and never rebuilds
    TireFriction stock;
    stock.model = TireFriction::Model::Lineal;
    CHECK(dynamic_cast<TireFriction::SmoothFriction*>(stock.curve()) != nullptr);
    CHECK(dynamic_cast<TireFriction::LinealFriction*>(stock.curve()) == nullptr);
    stock.OnAfterDeserialize();  // OnAfterDeserialize / SetupFrictionCurves rebuild from the model field
    CHECK(dynamic_cast<TireFriction::LinealFriction*>(stock.curve()) != nullptr);
    // the curves hold the settings object by reference: a module mutating the fields after construction is seen by the curve,
    // and ApplyAPLConstraints ran once, at construction
    TireFriction mutated;
    mutated.settings->adherent.x = 0.3f;
    mutated.settings->peak.x = 0.2f;  // violates adherent <= peak: not re-constrained
    cp.slip = Vector2f(0.0f, 0.25f);
    CHECK(mutated.IsAdherentSlip(cp) == true);
    CHECK(mutated.GetAdherentSlipBounds(cp).x == 0.3f);
    CHECK(mutated.GetPeakSlipBounds(cp).x == 0.2f);
    TireFriction rebuilt;
    rebuilt.settings->adherent.x = 0.3f;
    rebuilt.settings->peak.x = 0.2f;
    rebuilt.SetupFrictionCurves();  // constructing the curve runs ApplyAPLConstraints: peak.x >= adherent.x
    CHECK(rebuilt.settings->peak.x == 0.3f);
    // GetForce: zero below 1e-6 slip, opposes the slip, scales with frictionMultiplier
    cp.slip = Vector2f(0.0f, 5e-7f);
    CHECK(smooth.GetForce(cp).x == 0.0f);
    CHECK(smooth.GetForce(cp).y == 0.0f);
    cp.slip = Vector2f(0.0f, 2.0f);
    cp.load = 10.0f;
    // slip 2.0 is 0.2 of the way from the peak (1.5, 1.1) to the limit (4, 0.8): smoothstep(0.2) = 0.104 -> mu = 1.1 - 0.3 * 0.104 = 1.0688
    CHECK(smooth.GetForce(cp).y == doctest::Approx(-1.0688f * 10.0f).epsilon(0.0001));
    smooth.frictionMultiplier = 0.5f;
    CHECK(smooth.GetForce(cp).y == doctest::Approx(-1.0688f * 10.0f * 0.5f).epsilon(0.0001));
    // Flat: adherent.y * load * grip regardless of slip, all three bounds are adherent.x
    TireFriction flat;
    flat.model = TireFriction::Model::Flat;
    flat.SetupFrictionCurves();
    CHECK(flat.curve()->EvaluateForce(7.0f, cp) == doctest::Approx(9.5f));
    CHECK(flat.GetPeakSlipBounds(cp).x == flat.GetAdherentSlipBounds(cp).x);
}

TEST_CASE("WP-29a Wheel: inertia floor 0.01 t m^2, brake torque accumulation, locking brake holds the wheel") {
    // derived from Wheel.cs:RecalculateConstants / AddBrakeTorque / EvaluateTorqueDownstream
    Wheel w;
    w.radius = 0.4f;
    w.mass = 0.04f;  // m r^2 / 2 = 0.0032
    w.RecalculateConstants();
    CHECK(w.inertia() == 0.01f);
    w.mass = 20.0f;
    w.RecalculateConstants();
    CHECK(w.inertia() == doctest::Approx(1.6f));
    CHECK(w.AddBrakeTorque(5.0f) == 5.0f);
    CHECK(w.AddBrakeTorque(-9.0f) == 0.0f);  // clamped at zero
    w.mass = 0.04f;
    w.RecalculateConstants();
    // spin a floored wheel up on a stationary ground contact with the 1000 kN m brake on: it stops within a few ticks and stays locked
    w.tireFriction->model = TireFriction::Model::Lineal;
    w.tireFriction->SetupFrictionCurves();
    DirectDrive dd;
    Block::Connect(w, 0, dd, 0);
    Solver solver;
    REQUIRE(solver.Initialize({&w}, true));
    solver.Integrate(0.0f, 0.02f, 1, false);
    w.L = 5.0f * w.inertia();  // 5 rad/s
    w.RecalculateVars();
    float t = 0.0f;
    for (int i = 0; i < 20; i++) {
        w.load = 5.0f;
        w.grip = 1.0f;
        w.V = Vector2f(0.0f, 0.0f);
        w.Fext = Vector2f(0.0f, 0.0f);
        w.Tr = 0.0f;
        dd.motorInput = 0.0f;
        w.AddBrakeTorque(1000.0f);
        solver.Integrate(t, 0.02f, 8, false);
        t += 0.02f;
    }
    CHECK(std::fabs(w.angularVelocity()) < 1e-9f);  // locked: the brake holds the wheel (the integrator leaves a ~1e-15 residue)
    CHECK(w.isResting);
}

TEST_CASE("WP-29a Block / Solver wiring: connect, disconnect, aborted chain falls back to the wheels") {
    Wheel w;
    DirectDrive dd;
    CHECK(Block::Connect(w, 0, dd, 0));
    CHECK(w.connectedInputs() == 1);
    CHECK(dd.connectedOutputs() == 1);
    CHECK(w.inputs()[0]->output == &dd);
    CHECK(!Block::Connect(w, 1, dd, 0));  // the wheel has one input slot
    CHECK(!Block::Connect(dd, 0, w, 0));  // a DirectDrive has no inputs
    w.DisconnectInput(0);
    CHECK(w.connectedInputs() == 0);
    CHECK(dd.connectedOutputs() == 0);
    // a Wheel whose input is connected to a DirectDrive with a different connection: chain is [wheel, directDrive]
    Block::Connect(w, dd);
    w.radius = 0.3f;
    w.mass = 1.0f;
    w.RecalculateConstants();
    Solver ok;
    CHECK(ok.Initialize({&w}, true));
    REQUIRE(ok.GetBlockArray().size() == 2);
    CHECK(ok.GetBlockArray()[0] == &w);
    CHECK(ok.GetBlockArray()[1] == &dd);
    CHECK(ok.resultMessage() == "Initializing | Ok.");
    // DirectDrive.CheckConnections fails when its output was disconnected behind the solver's back: Initialize aborts and keeps the wheels
    Wheel w2;
    DirectDrive dd2;
    Block::Connect(w2, dd2);
    dd2.outputs()[0] = nullptr;
    Solver bad;
    CHECK(!bad.Initialize({&w2}, false));
    CHECK(bad.GetBlockArray().size() == 1);
    CHECK(bad.resultMessage().find("Aborted.") != std::string::npos);
    CHECK(w.angularVelocity() == 0.0f);
}

TEST_CASE("WP-29a KSPWheelController lifecycle: Awake in a game scene, Initialize, enabled(true) runs OnEnable, errors disable") {
    TimeScope ts;
    Time::fixedDeltaTime = 0.02f;
    Time::deltaTime = 0.02f;
    {
        // Create() outside a game scene: Awake leaves it enabled, OnEnable runs at once and finds no wheel collider yet (Missing WheelCollider)
        Bare b;
        CHECK(!b.c->enabled());
        CHECK(!b.c->initialized());
        bool missing = false;
        for (const auto& l : b.c->log) missing = missing || l.find("Missing WheelCollider") != std::string::npos;
        CHECK(missing);
        // the ModuleWheelBase sequence: Initialize() then enabled = true
        b.c->Initialize();
        CHECK(!b.c->initialized());
        b.c->enabled(true);
        CHECK(b.c->enabled());
        CHECK(b.c->initialized());
        CHECK(b.rb.maxAngularVelocity == 14.0f);
        CHECK(b.c->wheelCount() == 1);
        CHECK(b.c->wheelAt(0)->radius == 0.4f);
        CHECK(b.c->wheelAt(0)->mass == 0.1f);
        // VPWheelCollider.OnEnableVehicle ran: runtime values come from the fields, the native collider is enabled with 1 substep
        CHECK(b.c->wheelCollider->runtimeSpringRate == b.c->wheelCollider->springRate);
        CHECK(b.c->wheelCollider->wheelColliderData.enabled);
        CHECK(b.c->wheelCollider->wheelColliderData.substepsBelow == 1);
        CHECK(b.c->wheelCollider->wheelColliderData.substepSpeedThreshold == 1000.0f);
        CHECK(b.c->wheelCollider->GetWheelCollider()->mass == 1.0f);
        CHECK(b.c->wheelCollider->GetUpdateOrder() == 100);
        // disabling runs OnDisable: the native collider is switched off, the solver dropped
        b.c->enabled(false);
        CHECK(!b.c->initialized());
        CHECK(!b.c->wheelCollider->wheelColliderData.enabled);
    }
    {
        // an invalid radius disables the vehicle with the C# message
        Bare b;
        b.c->wheelCollider->radius = 0.005f;
        b.c->Initialize();
        b.c->enabled(true);
        CHECK(!b.c->enabled());
        bool invalid = false;
        for (const auto& l : b.c->log) invalid = invalid || l.find("Invalid wheel properties (radius and/or mass) for wheel[0]") != std::string::npos;
        CHECK(invalid);
    }
    {
        // no rigidbody
        NativeTransform host, wcT;
        auto c = KSPWheelController::Create(nullptr, &host, &wcT);
        c->enabled(true);
        CHECK(!c->enabled());
        bool noRb = false;
        for (const auto& l : c->log) noRb = noRb || l.find("Rigidbody not found") != std::string::npos;
        CHECK(noRb);
    }
    {
        // IsGrounded / currentState / SuspensionAxis before and after init
        Bare b;
        CHECK(b.c->currentState() == nullptr);
        CHECK(!b.c->IsGrounded());
        CHECK(b.c->SuspensionAxis().y == 1.0f);
        b.c->Initialize();
        b.c->enabled(true);
        REQUIRE(b.c->currentState() != nullptr);
        b.c->IsGrounded(true);
        CHECK(b.c->IsGrounded());
        CHECK(b.c->RbTgt() == &b.rb);
        NativeRigidbody other;
        b.c->SetRigidbodyTarget(&other);
        CHECK(b.c->RbTgt() == &other);
        b.c->SetRigidbodyTarget(nullptr);
        CHECK(b.c->RbTgt() == &b.rb);
    }
}

TEST_CASE("WP-29a KSPWheelController: defaults of Create, brake ramp (>= 1 uses maxBrakeTorqueRest = 1000), drive ramp, keyboard modes, properties") {
    TimeScope ts;
    Time::fixedDeltaTime = 0.02f;
    Time::deltaTime = 0.02f;
    Bare b;
    CHECK(b.c->maxRpm == std::numeric_limits<float>::max());
    CHECK(b.c->maxBrakeTorqueRest == 1000.0f);
    CHECK(b.c->wheelDamping == 0.95f);
    CHECK(b.c->maxDriveTorque == 0.0f);
    b.c->Initialize();
    b.c->enabled(true);
    NativeWheelCollider& nw = b.c->wheelCollider->wheelColliderData;
    nw.groundHitValid = true;
    nw.groundHit.point = Vector3f(0.0f, -0.4f, 0.0f);
    nw.groundHit.normal = Vector3f(0.0f, 1.0f, 0.0f);
    nw.groundHit.forwardDir = Vector3f(0.0f, 0.0f, 1.0f);
    nw.groundHit.sidewaysDir = Vector3f(1.0f, 0.0f, 0.0f);
    nw.groundHit.force = 1.0f;
    Collider ground;
    nw.groundHit.collider = &ground;
    b.c->maxBrakeTorque = 20.0f;
    b.c->brakeResponse = 100.0f;
    b.c->driveResponse = 10.0f;
    b.c->maxDriveTorque = 4.0f;
    b.c->brakeInput = 0.5f;
    b.c->driveInput = 1.0f;
    b.c->FixedUpdate();
    // brakeState moves 100 * 0.02 = 2 per tick towards 0.5 -> reaches 0.5; 0.5 < 1 uses maxBrakeTorque: the wheel saw 10
    CHECK(b.c->brakeState == 0.5f);
    CHECK(b.c->wheelAt(0)->brakeTorque() == doctest::Approx(10.0f));
    // driveState ramps by 10 * 0.02 = 0.2 per tick
    CHECK(b.c->driveState == doctest::Approx(0.2f));
    b.c->brakeInput = 1.0f;
    b.c->FixedUpdate();
    CHECK(b.c->brakeState == 1.0f);
    CHECK(b.c->wheelAt(0)->brakeTorque() == 1000.0f);  // brakeState >= 1: brake * maxBrakeTorqueRest (KSPWheelController.cs:256-263)
    CHECK(b.c->driveState == doctest::Approx(0.4f));
    // keyboard modes read Input.GetKey through the host seam
    b.c->keyboardBrakes = true;
    b.c->keyboardDrive = true;
    b.c->getKey = [](int k) { return k == KSPWheelController::KeyW; };
    b.c->FixedUpdate();
    CHECK(b.c->driveInput == 1.0f);
    CHECK(b.c->brakeInput == 0.0f);
    b.c->getKey = [](int k) { return k == KSPWheelController::KeyS || k == KSPWheelController::KeySpace; };
    b.c->FixedUpdate();
    CHECK(b.c->driveInput == -1.0f);
    CHECK(b.c->brakeInput == 1.0f);
    // steering is a per-frame ramp (OnUpdate), written to the wheel state's steerAngle
    b.c->maxSteerAngle = 30.0f;
    b.c->steeringResponse = 10.0f;
    b.c->steerInput = 1.0f;
    Time::deltaTime = 0.05f;
    b.c->Update();
    CHECK(b.c->steerState == doctest::Approx(0.5f));
    CHECK(b.c->wheelState()[0]->steerAngle == doctest::Approx(15.0f));
    b.c->keyboardSteering = true;
    b.c->getKey = [](int k) { return k == KSPWheelController::KeyA; };
    b.c->Update();
    CHECK(b.c->steerInput == -1.0f);
    // properties with identity transforms: WheelDown, WheelRadius, WheelCenter = TransformPoint(center + up * dist * (1 - anchor)) + down * contactDepth
    b.wcT.position = Vector3f(1.0f, 2.0f, 3.0f);
    CHECK(b.c->WheelRadius() == 0.4f);
    CHECK(b.c->WheelDown().y == -1.0f);
    b.c->currentState()->contactDepth = 0.1f;
    const Vector3f wcenter = b.c->WheelCenter();
    CHECK(wcenter.x == doctest::Approx(1.0f));
    CHECK(wcenter.y == doctest::Approx(2.0f + 0.5f * 0.5f - 0.1f));
    CHECK(b.c->WheelBase().y == doctest::Approx(2.0f + 0.25f - 0.1f - 0.4f));
    // SetWheelTransform writes the wheel collider host transform
    b.c->SetWheelTransform(Vector3f(5.0f, 6.0f, 7.0f), Quaternionf::identity());
    CHECK(b.wcT.position.y == 6.0f);
    // LegacyWheelLoad = max(0, contactDepth * effectiveSpringRate / g) when grounded
    b.c->currentState()->grounded = true;
    CHECK(b.c->LegacyWheelLoad() == doctest::Approx(0.1f * b.c->wheelCollider->effectiveSpringRate() / 9.80665f).epsilon(0.001));
    b.c->currentState()->grounded = false;
    CHECK(b.c->LegacyWheelLoad() == 0.0f);
}

TEST_CASE("WP-29a VehicleBase: gravity cache, pause / single step, state vars, advanced damper (static vehicle is null: NullReferenceException), accessors") {
    TimeScope ts;
    Time::fixedDeltaTime = 0.02f;
    Time::deltaTime = 0.02f;
    // KSPWheelGravity: default (0, -9.80665, 0); Value() assigns only when the new vector differs by more than the approximate ==
    KSPWheelGravity g;
    CHECK(g.Value().y == -9.80665f);
    CHECK(g.Magnitude() == 9.80665f);
    CHECK(g.Up().y == 1.0f);
    g.Value(Vector3f(0.0f, -9.80665f, 1e-7f));  // within the Vector3 == epsilon: ignored
    CHECK(g.Value().z == 0.0f);
    g.Value(Vector3f(0.0f, -1.63f, 0.0f));  // Mun
    CHECK(g.Magnitude() == 1.63f);
    // pause / single step
    Bare b;
    b.c->Initialize();
    b.c->enabled(true);
    NativeWheelCollider& nw = b.c->wheelCollider->wheelColliderData;
    nw.groundHitValid = false;
    b.c->paused(true);
    const float t0 = b.c->time();
    b.c->FixedUpdate();
    CHECK(b.c->time() == t0);  // paused: no step
    b.c->SingleStep();
    b.c->FixedUpdate();
    CHECK(b.c->time() == doctest::Approx(t0 + 0.02f));
    b.c->FixedUpdate();  // the single step was consumed
    CHECK(b.c->time() == doctest::Approx(t0 + 0.02f));
    b.c->paused(false);
    b.c->FixedUpdate();
    CHECK(b.c->time() == doctest::Approx(t0 + 0.04f));
    // state vars
    const auto vs = b.c->GetVehicleStateVars();
    const auto ws = b.c->GetWheelStateVars();
    b.c->SetWheelStateVars({VehicleBase::WheelStateVars{3.0f, 4.0f, 0.5f, Vector2f(1.0f, 2.0f)}});
    CHECK(b.c->wheelAt(0)->L == 3.0f);
    CHECK(b.c->wheelState()[0]->reactionTorque == 4.0f);
    b.c->SetWheelStateVars(ws);
    CHECK(b.c->wheelAt(0)->L == ws[0].L);
    b.c->SetVehicleStateVars(vs);
    CHECK(b.c->time() == vs.time);
    // accessors by index
    CHECK(b.c->GetWheelRadius(0) == 0.4f);
    CHECK(b.c->GetWheelRadius(3) == 0.0f);
    b.c->SetWheelRadius(0, 0.001f);
    CHECK(b.c->GetWheelRadius(0) == 0.01f);  // clamped
    CHECK(b.c->wheelCollider->radius == 0.01f);
    b.c->SetWheelTireFrictionMultiplier(0, -3.0f);
    CHECK(b.c->GetWheelTireFriction(0)->frictionMultiplier == 0.0f);
    CHECK(b.c->GetWheelIndex(0) == 0);
    CHECK(b.c->GetWheelIndex(1) == -1);
    CHECK(b.c->GetAxleCount() == 1);
    CHECK(b.c->GetWheelsInAxle(0) == 1);
    CHECK(b.c->GetWheelsInAxle(1) == 0);
    CHECK(b.c->GetWheelAngularVelocityForSlip(0, 1.0f) == 0.0f);  // not grounded
    CHECK(b.c->data.Get(0).size() == 10);
    CHECK(b.c->data.Get(1).size() == 22);
    CHECK(b.c->data.Get(2).size() == 10);
    b.c->AddWheelBrakeTorque(0, 7.0f);
    CHECK(b.c->wheelAt(0)->AddBrakeTorque(0.0f) == 7.0f);
    // the advanced suspension damper dereferences the static VehicleBase.vehicle, which is null outside the impact callbacks
    nw.groundHitValid = true;
    nw.groundHit.normal = Vector3f(0.0f, 1.0f, 0.0f);
    nw.groundHit.forwardDir = Vector3f(0.0f, 0.0f, 1.0f);
    nw.groundHit.sidewaysDir = Vector3f(1.0f, 0.0f, 0.0f);
    nw.groundHit.force = 2.0f;
    Collider ground;
    nw.groundHit.collider = &ground;
    b.c->advancedSuspensionDamper = true;
    CHECK_THROWS_AS(b.c->FixedUpdate(), NullReferenceException);
}

TEST_CASE("WP-29a VehicleBase impacts: OnCollision classification, ProcessImpacts averaging and the callbacks (derived from VehicleBase.cs:1255-1361)") {
    TimeScope ts;
    Time::fixedDeltaTime = 0.02f;
    Time::deltaTime = 0.02f;
    Bare b;
    b.c->Initialize();
    b.c->enabled(true);
    b.c->wheelCollider->wheelColliderData.groundHitValid = false;
    NativeRigidbody other;
    Collider otherCol;
    otherCol.attachedRigidbody = &other;
    ContactPoint cpHard;
    cpHard.point = Vector3f(1.0f, 0.0f, 0.0f);
    cpHard.normal = Vector3f(1.0f, 0.0f, 0.0f);
    cpHard.thisRigidbody = &b.rb;
    cpHard.otherCollider = &otherCol;
    Collision hard;
    hard.contacts = {cpHard, cpHard};
    hard.relativeVelocity = Vector3f(5.0f, 0.0f, 0.0f);  // |v|^2 = 25 > impactMinSpeed^2 = 4 on an Enter
    b.c->OnCollisionEnter(hard);
    // 2 contacts with a dot(pointVelocity, normal) = 0: since relativeVelocity is hard the contacts are impact contacts
    CHECK(b.c->localImpactPosition().x == doctest::Approx(1.0f));  // the average of the two points, to local space (identity)
    CHECK(b.c->localImpactVelocity().x == doctest::Approx(5.0f));
    CHECK(b.c->isHardImpact());  // hardness sum 0 >= 0 (no ground material manager)
    CHECK(b.c->lastContactedCollider() == &otherCol);
    // ProcessImpacts runs at the end of FixedUpdate once m_time - lastImpactTime >= impactInterval
    int callbacks = 0;
    VehicleBase* seen = nullptr;
    b.c->onImpact = [&]() {
        callbacks++;
        seen = VehicleBase::vehicle;
    };
    b.c->impactInterval = 0.0f;
    const UnityRandom::State savedRandom = UnityRandom::Global().state();
    UnityRandom::Global().InitState(42);
    UnityRandom shadow(42);
    b.c->FixedUpdate();
    CHECK(callbacks == 1);
    CHECK(seen == b.c.get());
    CHECK(VehicleBase::vehicle == nullptr);
    // the next impact time was drawn with one Random.Range(-0.4, 0.4) * interval(0) -> still consumes exactly one draw
    CHECK(UnityRandom::Global().state().s3 == (shadow.Range(-0.4f, 0.4f), shadow.state().s3));
    CHECK(b.c->localImpactPosition().x == 0.0f);  // the sums were cleared
    UnityRandom::Global().SetState(savedRandom);
    // a soft contact: slow relative velocity and |dot| < threshold feeds the drag state, not the impact sums
    Collision soft;
    ContactPoint cps = cpHard;
    cps.normal = Vector3f(0.0f, 1.0f, 0.0f);
    soft.contacts = {cps};
    soft.relativeVelocity = Vector3f(0.1f, 0.0f, 0.0f);
    b.rb.velocity = Vector3f(2.0f, 0.0f, 0.0f);
    b.c->OnCollisionStay(soft);
    CHECK(b.c->localImpactPosition().x == 0.0f);
    CHECK(b.c->localDragVelocity().x == doctest::Approx(0.8f));
    CHECK(b.c->localDragPosition().x == doctest::Approx(0.2f));  // Lerp(0, 1, 10 * 0.02)
    // OnCollision is ignored while not initialized or when contact processing is disabled
    b.c->disableContactProcessing = true;
    b.c->OnCollisionEnter(hard);
    CHECK(b.c->localImpactPosition().x == 0.0f);
    // onRawCollision sees the static collision state during the callback
    b.c->disableContactProcessing = false;
    bool rawSeen = false;
    b.c->onRawCollision = [&]() { rawSeen = VehicleBase::vehicle == b.c.get() && VehicleBase::currentCollision == &hard && VehicleBase::isCollisionEnter; };
    b.c->OnCollisionEnter(hard);
    CHECK(rawSeen);
    CHECK(VehicleBase::currentCollision == nullptr);
}

TEST_CASE("WP-29a VPWheelCollider: GetGroundHit is false when the component is disabled, the native GetGroundHit passes through, scale factor") {
    TimeScope ts;
    Time::fixedDeltaTime = 0.02f;
    Time::deltaTime = 0.02f;
    Bare b;
    b.c->Initialize();
    b.c->enabled(true);
    VPWheelCollider& wc = *b.c->wheelCollider;
    wc.wheelColliderData.groundHitValid = true;
    wc.wheelColliderData.groundHit.point = Vector3f(1.0f, 0.0f, 0.0f);
    wc.wheelColliderData.groundHit.normal = Vector3f(0.0f, 1.0f, 0.0f);
    WheelHit h;
    b.rb.velocity = Vector3f(0.0f, 0.0f, 10.0f);
    CHECK(wc.GetGroundHit(h));
    CHECK(h.point.z == doctest::Approx(0.2f));  // + velocity * fixedDeltaTime
    CHECK(h.forwardDir.z == doctest::Approx(1.0f));
    CHECK(h.sidewaysDir.x == doctest::Approx(1.0f));  // Cross(up, forward)
    wc.SetEnabled(false);
    WheelHit h2;
    CHECK(!wc.GetGroundHit(h2));
    CHECK(!wc.isActiveAndEnabled());
    // steerAngle(): a disabled component keeps m_steerAngle
    wc.steerAngle(10.0f);
    CHECK(wc.steerAngle() == 0.0f);
    wc.SetEnabled(true);
    wc.steerAngle(10.0f);
    CHECK(wc.steerAngle() == 10.0f);
    // with identity transforms the fixed steer angle is the input angle (FixSteerAngle projects the rotated forward on the body plane)
    CHECK(wc.wheelColliderData.steerAngle == doctest::Approx(10.0f).epsilon(1e-5));
    // GetContactDepth with identity transforms: travel + radius - (-y + dist * (1 - anchor))
    b.wcT.position = Vector3f(0.0f, 1.0f, 0.0f);
    const float cd = wc.GetContactDepth(Vector3f(0.0f, 0.5f, 0.0f), 0.5f);
    CHECK(cd == doctest::Approx(0.5f + 0.4f - (0.5f + 0.5f * 0.5f)));
    // a dynamic surface body feeds back the clamped mass-ratio share of the (negated) force; a kinematic one does not
    NativeRigidbody surf;
    surf.mass = 4.0f;
    b.rb.mass = 2.0f;
    b.rb.forces.clear();
    wc.ApplyForce(Vector3f(0.0f, 8.0f, 0.0f), Vector3f::zero(), &surf);
    REQUIRE(surf.forces.size() == 1);
    CHECK(surf.forces[0].force.y == -8.0f);  // Clamp01(4 / 2) = 1
    surf.forces.clear();
    surf.isKinematic = true;
    wc.ApplyForce(Vector3f(0.0f, 8.0f, 0.0f), Vector3f::zero(), &surf);
    CHECK(surf.forces.empty());
    CHECK(b.rb.forces.size() == 2);
}

TEST_CASE("WP-29a Quaternion.AngleAxis: the production (UCRT sinf / cosf) binding stays within 2 ULP of the reference formulation") {
    // the wheel gate rebinds NativeOps::AngleAxis to RefAngleAxis (the reference cannot reproduce the UCRT single-precision sinf / cosf);
    // this bounds what that substitution hides
    int64_t worst = 0, differing = 0, n = 0;
    const Vector3f axes[] = {Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, 0.99498743f, 0.1f), Vector3f(0.6f, 0.0f, 0.8f)};
    for (const Vector3f& ax : axes) {
        for (float a = -90.0f; a <= 90.0f; a += 0.37f) {
            const Quaternionf p = UnityNative::AngleAxis(a, ax), q = RefAngleAxis(a, ax);
            const float pv[4] = {p.x, p.y, p.z, p.w}, qv[4] = {q.x, q.y, q.z, q.w};
            for (int i = 0; i < 4; i++) {
                const int64_t u = std::llabs(UlpDiff(pv[i], qv[i]));
                worst = std::max(worst, u);
                if (u != 0) differing++;
                n++;
            }
        }
    }
    INFO("worst " << worst << " ULP, " << differing << " of " << n << " components differ");
    CHECK(worst <= 2);
    CHECK(n > 1000);
}
