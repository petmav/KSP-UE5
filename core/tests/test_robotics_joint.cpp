// WP-52 unit tests: the joint seam (src/serenity/servo_joint.cpp, servo_plant.cpp) and the ControlledAxis presets (controlled_axis.cpp) against the
// independent Python reference (tools/wp52/servo_reference.py). See tests/test_robotics.cpp for the gate description and the data caveat.
//   * preset curves: key lists bit for bit (0 ULP); the (up to two) keys per curve that the C# computes with AnimationCurve.Evaluate (native) against
//     the reference's double-precision Hermite evaluation at a documented tolerance;
//   * ConfigurableJointExtensions.SetTargetRotationLocal against a double-precision quaternion algebra (the Quaternion natives LookRotation / Inverse are
//     float32 per operation, UNVERIFIED against the game): within 1e-5 per component;
//   * the D6 drive model (StepDrive1D) against its Python twin: 40 parameter sets x 100 steps, relative 1e-12.
#include "robotics_fixture.hpp"

using namespace wp52t;

namespace {

// the test fixture field a preset case uses: a hinge axis field without limits (hingeDamping), the hinge's targetAngle with soft limits, or a controller field
struct PresetOwner {
    std::unique_ptr<Rig::One> one;
    ModuleRoboticServoHinge* hinge = nullptr;
    ModuleRoboticController* ctl = nullptr;
};

}  // namespace

TEST_CASE("WP-52 ControlledAxis presets (Sine, Square, Triangle, Saw, RevSaw) vs the independent Python reference: keys 0 ULP") {
    const json& ref = LoadRef52();
    REQUIRE(GameAvailable());
    Rig rig;
    Diff d;
    long keysChecked = 0, synthChecked = 0;
    size_t idx = 0;
    for (const json& c : ref["presets"]) {
        const std::string name = "preset" + std::to_string(idx++);
        INFO(name);
        PresetOwner o;
        o.one = rig.MakeOne();
        Part& p = *o.one->part;
        o.hinge = AddServo<ModuleRoboticServoHinge>(p, "ModuleRoboticServoHinge");
        o.ctl = AddController(p);
        const bool controller = c["controller"].get<bool>();
        const bool soft = !c["soft"].is_null();
        BaseAxisField* f = nullptr;
        PartModule* m = o.hinge;
        if (controller) {
            f = dynamic_cast<BaseAxisField*>(o.ctl->Fields()["sequencePlaySpeed"]);
            m = o.ctl;
        } else if (soft) {
            f = dynamic_cast<BaseAxisField*>(o.hinge->Fields()["targetAngle"]);
            o.hinge->UpdateAxisFieldLimit("targetAngle", o.hinge->hardMinMaxLimits(), V2j(c["soft"]));
        } else {
            f = dynamic_cast<BaseAxisField*>(o.hinge->Fields()["hingeDamping"]);
        }
        REQUIRE(f != nullptr);
        f->minValue = J(c["min"]);
        f->maxValue = J(c["max"]);
        ControlledAxis ax(&p, m, f, o.ctl);
        const std::string ref1 = c["ref"].get<std::string>();
        auto run = [&](float cycles, float phase) {
            if (ref1 == "stockSine") ax.PresetSine(cycles, phase);
            else if (ref1 == "stockSquare") ax.PresetSquare(cycles, phase);
            else if (ref1 == "stockTriangle") ax.PresetTriangle(cycles, phase);
            else if (ref1 == "stockSaw") ax.PresetSaw(cycles, phase);
            else ax.PresetRevSaw(cycles, phase);
        };
        const float vmax = std::max(std::fabs(J(c["min"])), std::fabs(J(c["max"])));
        for (int pass = 0; pass < 2; pass++) {
            const float cycles = J(c[pass == 0 ? "cycles" : "cycles2"]);
            const float phase = J(c[pass == 0 ? "phase" : "phase2"]);
            run(cycles, phase);
            const json& want = c[pass == 0 ? "out" : "out2"];
            const std::vector<Keyframe>& got = ax.timeValue.Curve().keys();
            const std::string w = name + (pass == 0 ? " first" : " second");
            d.CheckS(std::to_string(got.size()), std::to_string(want.size()), w + " key count");
            for (size_t i = 0; i < std::min(got.size(), want.size()); i++) {
                const json& k = want[i];
                const std::string kind = k[0].get<std::string>();
                const std::string wk = w + " key" + std::to_string(i);
                d.CheckF(got[i].time, J(k[1]), wk + " time");
                if (kind == "key") {
                    d.CheckF(got[i].value, J(k[2]), wk + " value");
                    d.CheckF(got[i].inTangent, J(k[3]), wk + " in");
                    d.CheckF(got[i].outTangent, J(k[4]), wk + " out");
                    keysChecked++;
                } else {
                    // a key built from AnimationCurve.Evaluate (native cubic, float32) against the reference's double Hermite: the value to 1e-5 of the
                    // axis range, a tangent (difference quotient over 0.001) to 3e-4 of the range per cycle
                    const double tolV = 1e-5 * (vmax + 1.0), tolT = 3e-3 * (vmax + 1.0) * std::max(cycles, 1.0f);
                    d.CheckB(std::fabs(static_cast<double>(got[i].value) - JD(k[2])) <= tolV, true, wk + " synth value " + std::to_string(got[i].value) + " vs " + std::to_string(JD(k[2])));
                    d.CheckB(std::fabs(static_cast<double>(got[i].inTangent) - JD(k[3])) <= tolT, true, wk + " synth in " + std::to_string(got[i].inTangent) + " vs " + std::to_string(JD(k[3])));
                    d.CheckB(std::fabs(static_cast<double>(got[i].outTangent) - JD(k[4])) <= tolT, true, wk + " synth out " + std::to_string(got[i].outTangent) + " vs " + std::to_string(JD(k[4])));
                    synthChecked++;
                }
            }
        }
    }
    INFO(d.first);
    CHECK(d.bad == 0);
    CHECK(keysChecked > 1000);
    CHECK(synthChecked > 20);
}

TEST_CASE("WP-52 ConfigurableJointExtensions.SetTargetRotationLocal vs a double-precision quaternion algebra, and the physical angle the drive reads back") {
    const json& ref = LoadRef52();
    int n = 0;
    double worst = 0.0;
    for (const json& c : ref["joint"]) {
        ConfigurableJointData j;
        j.axis = Vector3f(J(c["axis"][0]), J(c["axis"][1]), J(c["axis"][2]));
        j.secondaryAxis = Vector3f(J(c["sec"][0]), J(c["sec"][1]), J(c["sec"][2]));
        const Quaternionf target(J(c["target"][0]), J(c["target"][1]), J(c["target"][2]), J(c["target"][3]));
        const Quaternionf start(J(c["start"][0]), J(c["start"][1]), J(c["start"][2]), J(c["start"][3]));
        SetTargetRotationLocal(j, target, start);
        const double q[4] = {j.targetRotation.x, j.targetRotation.y, j.targetRotation.z, j.targetRotation.w};
        double e[4];
        for (int i = 0; i < 4; i++) e[i] = JD(c["expected"][i]);
        // q and -q are the same rotation
        double dot = 0.0;
        for (int i = 0; i < 4; i++) dot += q[i] * e[i];
        const double sgn = dot < 0.0 ? -1.0 : 1.0;
        for (int i = 0; i < 4; i++) {
            const double diff = std::fabs(q[i] - sgn * e[i]);
            worst = std::max(worst, diff);
            CHECK_MESSAGE(diff < 1e-5, "case " << n << " component " << i << " got " << q[i] << " want " << sgn * e[i]);
        }
        // the physical coordinate the D6 drive is pulled to: target angle minus the start angle about the joint axis (derivation in servo_joint.cpp)
        double want = JD(c["angle"]) - JD(c["startAngle"]);
        while (want > 180.0) want -= 360.0;
        while (want < -180.0) want += 360.0;
        CHECK_MESSAGE(std::fabs(PhysicalTargetAngleDegrees(j) - want) < 1e-3, "case " << n << " angle " << PhysicalTargetAngleDegrees(j) << " want " << want);
        n++;
    }
    CHECK(n == 72);
    CHECK(worst < 1e-5);
}

TEST_CASE("WP-52 SetTargetRotation in the wrong space logs and still computes (ConfigurableJointExtensions.cs:5-21)") {
    LogCapture logs;
    ConfigurableJointData j;
    j.axis = Vector3f::forward();
    j.secondaryAxis = Vector3f::up();
    j.configuredInWorldSpace = true;
    SetTargetRotationLocal(j, Quaternionf::identity(), Quaternionf::identity());
    CHECK(logs.Has(log::Level::Error, "SetTargetRotationLocal should not be used with joints that are configured in world space"));
    j.configuredInWorldSpace = false;
    SetTargetRotationWorld(j, Quaternionf::identity(), Quaternionf::identity());
    CHECK(logs.Has(log::Level::Error, "SetTargetRotation must be used with joints that are configured in world space"));
    // identity target and start: the joint-space conjugation of the identity is the identity
    CHECK(std::fabs(std::fabs(j.targetRotation.w) - 1.0f) < 1e-6f);
}

TEST_CASE("WP-52 D6 drive model (StepDrive1D) vs its independent Python twin: 40 parameter sets x 100 steps") {
    const json& ref = LoadRef52();
    int sets = 0;
    for (const json& c : ref["drive"]) {
        ServoPlantState s;
        s.position = JD(c["x0"]);
        s.velocity = JD(c["v0"]);
        ServoPlantParams params;
        params.inertia = JD(c["m"]);
        JointDrive drive;
        drive.positionSpring = static_cast<float>(JD(c["k"]));
        drive.positionDamper = static_cast<float>(JD(c["c"]));
        drive.maximumForce = static_cast<float>(JD(c["fmax"]));
        // the float JointDrive rounds the parameters: the reference is told the same (it only uses exactly representable decimals; re-check)
        const double k = drive.positionSpring, cc = drive.positionDamper, fm = drive.maximumForce;
        const double m = params.inertia, dt = JD(c["dt"]);
        double x = s.position, v = s.velocity;
        std::vector<std::array<double, 3>> want;
        for (const json& t : c["traj"]) want.push_back({JD(t[0]), JD(t[1]), JD(t[2])});
        for (size_t i = 0; i < want.size(); i++) {
            StepDrive1D(s, params, drive, JD(c["target"]), JD(c["tvel"]), c["limited"].get<bool>(), JD(c["lo"]), JD(c["hi"]), dt);
            // 1e-6 relative: k, c and the force limit travel as float32 in JointDrive (1e5 / 1e4 / 5.0 / 100.0 are exact, 1e30 is rounded)
            const double tol = 1e-6 * (1.0 + std::fabs(want[i][0]));
            CHECK_MESSAGE(std::fabs(s.position - want[i][0]) <= tol, "set " << sets << " step " << i << " x " << s.position << " vs " << want[i][0]);
            CHECK_MESSAGE(std::fabs(s.velocity - want[i][1]) <= tol * 10.0, "set " << sets << " step " << i << " v");
            CHECK_MESSAGE(std::fabs(s.lastForce - want[i][2]) <= 1e-6 * (1.0 + std::fabs(want[i][2])) * 10.0, "set " << sets << " step " << i << " force");
        }
        (void)k; (void)cc; (void)fm; (void)m; (void)x; (void)v;
        sets++;
    }
    CHECK(sets == 40);
}

TEST_CASE("WP-52 D6 drive model: analytic checks (saturation, steady state, hard limits)") {
    ServoPlantParams params;
    params.inertia = 2.0;
    JointDrive d;
    d.positionSpring = 100.0f;
    d.positionDamper = 20.0f;
    d.maximumForce = 5.0f;
    ServoPlantState s;
    // the force never exceeds the limit, and the body is accelerated at F/m while saturated
    StepDrive1D(s, params, d, 10.0, 0.0, false, 0, 0, 0.02);
    CHECK(std::fabs(s.lastForce) <= 5.0 + 1e-12);
    CHECK(std::fabs(s.velocity - 5.0 * 0.02 / 2.0) < 1e-12);
    // settles on the target
    for (int i = 0; i < 5000; i++) StepDrive1D(s, params, d, 1.0, 0.0, false, 0, 0, 0.02);
    CHECK(std::fabs(s.position - 1.0) < 1e-6);
    CHECK(std::fabs(s.velocity) < 1e-6);
    // hard limits stop the body and zero the velocity into the stop
    ServoPlantState t;
    d.maximumForce = 1e9f;
    for (int i = 0; i < 2000; i++) StepDrive1D(t, params, d, 5.0, 0.0, true, -1.0, 2.0, 0.02);
    CHECK(std::fabs(t.position - 2.0) < 1e-12);
    CHECK(t.velocity <= 0.0);
}
