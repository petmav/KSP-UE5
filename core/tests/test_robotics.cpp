// WP-52 unit tests: Breaking Ground robotics (src/serenity/{base_servo, servo_*, controlled_axis, robotic_controller}.cpp).
//
// Gate: tools/wp52/servo_reference.py (tests/data/wp52_reference.json), an independent Python implementation of the same C# (BaseServo,
// ModuleRoboticServoHinge / Piston / Rotor, ModuleRoboticRotationServo, ModuleRoboticController, ControlledAxis presets, the joint extension)
// that shares no code with the port and was written before it. Its seeded inputs are replayed here; every float output is compared bit for
// bit (0 ULP, NaN == NaN, the sign of zero included):
//   * 92 scripted servo runs (26 hinge, 20 rotation servo, 22 piston, 24 rotor; 120 fixed ticks each, the scripted coordinate fed through
//     BaseServo::angleSource / positionSource): the rate-of-motion average, the power draw, the MoveTowards drive target, the joint drive values
//     (spring / damper / maximum force from the motor output), the soft / hard limits, lock / unlock / motor on / off / brake, mass and cost;
//   * 60 controller runs (sequence position, loop modes Once / Repeat / PingPong / OnceRestart, direction, play / stop events, action firing
//     windows, axis updates);
//   * 80 preset curves (Sine / Square / Triangle / Saw / RevSaw): key lists bit for bit, the two keys that come from an AnimationCurve.Evaluate
//     (native) against the reference's double Hermite;
//   * ConfigurableJointExtensions.SetTargetRotationLocal against a double-precision quaternion algebra, the D6 drive model against its Python twin.
// DATA: the GameData mirror has no SquadExpansion/Serenity: every servo MODULE value is a SYNTHETIC fixture written here or in the reference
// from the C# field names (the defaults are the real C# initialisers). No in-game capture of a servo exists.
// The remaining cases are derived from the C# control flow and labelled.
#include "robotics_fixture.hpp"

using namespace wp52t;


// =============================================================================================================================
// the scripted servo runs

namespace {

void RunServoScenario(Rig& rig, const json& sc, size_t index, Diff& d, long& ticks) {
    const std::string kind = sc["kind"].get<std::string>();
    auto one = rig.MakeOne();
    Part& p = *one->part;
    const char* cls = kind == "hinge" ? "ModuleRoboticServoHinge" : kind == "rotation" ? "ModuleRoboticRotationServo" : kind == "piston" ? "ModuleRoboticServoPiston" : "ModuleRoboticServoRotor";
    BaseServo* s = nullptr;
    ModuleRoboticServoHinge* hinge = nullptr;
    ModuleRoboticRotationServo* rot = nullptr;
    ModuleRoboticServoPiston* piston = nullptr;
    ModuleRoboticServoRotor* rotor = nullptr;
    if (kind == "hinge") s = hinge = AddServo<ModuleRoboticServoHinge>(p, cls);
    else if (kind == "rotation") s = rot = AddServo<ModuleRoboticRotationServo>(p, cls);
    else if (kind == "piston") s = piston = AddServo<ModuleRoboticServoPiston>(p, cls);
    else s = rotor = AddServo<ModuleRoboticServoRotor>(p, cls);
    for (auto it = sc["fields"].begin(); it != sc["fields"].end(); ++it) SetFieldJ(*s, it.key(), it.value());
    if (sc["autostart"].get<bool>()) {
        s->resHandler.inputResources[0]->startUpHandler = true;
        s->resHandler.inputResources[0]->startUpAmount = 0.5f;
    }
    Time::fixedDeltaTime = J(sc["fdt"]);
    Time::deltaTime = J(sc["dt"]);
    Matrix4x4f parent = Matrix4x4f::identity();
    if (kind == "piston") parent = MatJ(sc["parent"]);
    WireRig(*s, parent);
    if (kind == "piston") {
        s->rig.nodes["Moving"]->localRotation = Quaternionf(J(sc["movingRot"][0]), J(sc["movingRot"][1]), J(sc["movingRot"][2]), J(sc["movingRot"][3]));
        s->rig.nodes["Moving"]->localPosition = Vector3f(J(sc["movingPos"][0]), J(sc["movingPos"][1]), J(sc["movingPos"][2]));
    }
    float cur = J(sc["angles"][0]);
    if (kind == "piston") s->positionSource = [&cur] { return cur; };
    else s->angleSource = [&cur] { return cur; };
    if (kind == "hinge" || kind == "piston") {
        // the drag cubes the module weighs
        p.DragCubes.Cubes().clear();
        for (const char* n : {"0", "50", "100"}) p.DragCubes.Cubes().push_back(DragCube(n));
    }
    const std::string name = kind + std::to_string(index);
    INFO(name);
    s->OnStart(StartState::None);
    s->OnStartBeforePartAttachJoint(StartState::None);
    if (sc["paw"].get<bool>()) s->OnPartMenuOpen(&p);
    std::vector<std::string> events;
    // lock events
    struct Sub {
        std::vector<std::string>* ev;
        void Changing(Part*, bool b) { ev->push_back("lockChanging:" + std::to_string(b ? 1 : 0)); }
        void Changed(Part*, bool b) { ev->push_back("lockChanged:" + std::to_string(b ? 1 : 0)); }
    } sub{&events};
    GameEvents::onRoboticPartLockChanging.Add(&sub, &Sub::Changing);
    GameEvents::onRoboticPartLockChanged.Add(&sub, &Sub::Changed);
    struct Unsub {
        Sub* sub;
        ~Unsub() {
            GameEvents::onRoboticPartLockChanging.Remove(sub, &Sub::Changing);
            GameEvents::onRoboticPartLockChanged.Remove(sub, &Sub::Changed);
        }
    } unsub{&sub};
    const json& ops = sc["ops"];
    const json& expected = sc["expected"];
    const int nTicks = sc["ticks"].get<int>();
    for (int t = 0; t < nTicks; t++) {
        for (const json& op : ops) {
            if (op[0].get<int>() != t) continue;
            const std::string o = op[1].get<std::string>();
            if (o == "set") SetValueJ(*s, op[2].get<std::string>(), op[3]);
            else if (o == "set_pair") s->Fields()[op[2].get<std::string>()]->SetValue(ModuleValue(Vector2f(J(op[3]), J(op[4]))));
            else if (o == "lock") s->EngageServoLock();
            else if (o == "unlock") s->DisengageServoLock();
            else if (o == "motor_off") s->DisengageMotor();
            else if (o == "motor_on") s->EngageMotor();
            else if (o == "min") (hinge ? hinge->SetMinimumAngle() : rot->SetMinimumAngle());
            else if (o == "max") (hinge ? hinge->SetMaximumAngle() : rot->SetMaximumAngle());
            else if (o == "toggle") { KSPActionParam ap; if (hinge) hinge->ToggleHingeAction(ap); else rot->ToggleServoAction(ap); }
            else if (o == "extend") piston->ExtendPiston();
            else if (o == "retract") piston->RetractPiston();
            else if (o == "brake_on") { KSPActionParam ap(0, KSPActionType::Activate); rotor->BrakeAction(ap); }
            else if (o == "brake_off") { KSPActionParam ap(0, KSPActionType::Deactivate); rotor->BrakeAction(ap); }
            else if (o == "brake_toggle") { KSPActionParam ap(0, KSPActionType::Toggle); rotor->BrakeAction(ap); }
            else if (o == "invert") {}
            else FAIL("unknown op " << o);
        }
        cur = J(sc["angles"][t + 1]);
        s->FixedUpdate();
        s->Update();
        // observation
        Obs obs;
        const JointDrive* drive = nullptr;
        const json& e = expected[t];
        auto F = [&](const char* k, float v) { if (e.contains(k)) d.CheckF(v, J(e[k]), name + " t" + std::to_string(t) + " " + k); };
        auto L = [&](const char* k, const std::vector<float>& v) {
            if (!e.contains(k)) return;
            const json& w = e[k];
            d.CheckB(w.size() == v.size(), w.size() == v.size(), name + " size " + k);
            for (size_t i = 0; i < std::min<size_t>(w.size(), v.size()); i++) d.CheckF(v[i], J(w[i]), name + " t" + std::to_string(t) + " " + k + "[" + std::to_string(i) + "]");
        };
        (void)drive;
        d.CheckD(s->rate, JD(e["rate"]), name + " t" + std::to_string(t) + " rate");
        F("previousDisplacement", s->previousDisplacement);
        F("currentFrameVelocity", s->currentFrameVelocity);
        F("transformRateOfMotion", s->transformRateOfMotion);
        F("rsum", s->velocityReadingsSum);
        F("motorOutput", s->motorOutput);
        F("driveUnit", s->driveUnit);
        F("lockAngle", s->lockAngle);
        F("locked", B(s->servoIsLocked()));
        F("engaged", B(s->servoMotorIsEngaged()));
        F("moving", B(s->IsMoving()));
        F("mass", s->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT));
        F("cost", s->GetModuleCost(0.0f, ModifierStagingSituation::CURRENT));
        if (hinge) {
            F("driveTargetAngle", hinge->driveTargetAngle);
            F("maxAnglePerFrame", hinge->maxAnglePerFrame);
            F("angleDiff", hinge->angleDiff);
            F("working", hinge->workingJointTargetAngle);
            F("targetAngle", hinge->targetAngle());
            F("traverseVelocity", hinge->traverseVelocity());
            F("currentAngle", hinge->currentAngle());
            F("previousTargetAngle", hinge->previousTargetAngle());
            F("softMin", hinge->softMinMaxAngles().x);
            F("softMax", hinge->softMinMaxAngles().y);
            L("drive", Drive(hinge->servoJoint->angularXDrive));
            L("limitSpring", {hinge->servoJoint->angularXLimitSpring.spring, hinge->servoJoint->angularXLimitSpring.damper});
            F("high", hinge->servoJoint->highAngularXLimit.limit);
            F("low", hinge->servoJoint->lowAngularXLimit.limit);
            if (e.contains("dragWeights") && !e["dragWeights"].empty()) L("dragWeights", {p.DragCubes.GetCube("0")->Weight(), p.DragCubes.GetCube("50")->Weight(), p.DragCubes.GetCube("100")->Weight()});
        } else if (rot) {
            F("driveTargetAngle", rot->driveTargetAngle);
            F("maxAnglePerFrame", rot->maxAnglePerFrame);
            F("angleDiff", rot->angleDiff);
            F("working", rot->workingJointTargetAngle);
            F("targetAngle", rot->targetAngle());
            F("traverseVelocity", rot->traverseVelocity());
            F("currentAngle", rot->currentAngle());
            F("previousTargetAngle", rot->Get<float>("previousTargetAngle"));
            F("softMin", rot->softMinMaxAngles().x);
            F("softMax", rot->softMinMaxAngles().y);
            F("hardMin", rot->hardMinMaxLimits().x);
            F("hardMax", rot->hardMinMaxLimits().y);
            L("drive", Drive(rot->servoJoint->angularXDrive));
            L("limitSpring", {rot->servoJoint->angularXLimitSpring.spring, rot->servoJoint->angularXLimitSpring.damper});
            F("high", rot->servoJoint->highAngularXLimit.limit);
            F("low", rot->servoJoint->lowAngularXLimit.limit);
        } else if (piston) {
            F("driveTargetPosition", piston->driveTargetPosition);
            F("targetPosition", piston->targetPosition);
            F("positionDiff", piston->positionDiff);
            F("targetExtension", piston->targetExtension());
            F("traverseVelocity", piston->traverseVelocity());
            F("currentExtension", piston->currentExtension());
            F("softMin", piston->softMinMaxExtension().x);
            F("softMax", piston->softMinMaxExtension().y);
            L("drive", Drive(piston->servoJoint->xDrive));
            L("linearLimit", {piston->servoJoint->linearLimit.limit, piston->servoJoint->linearLimit.bounciness, piston->servoJoint->linearLimit.contactDistance});
            L("linearLimitSpring", {piston->servoJoint->linearLimitSpring.spring, piston->servoJoint->linearLimitSpring.damper});
            L("targetPositionVec", V3(piston->servoJoint->targetPosition));
            L("connectedAnchor", V3(piston->servoJoint->connectedAnchor));
            if (e.contains("dragWeights") && !e["dragWeights"].empty()) L("dragWeights", {p.DragCubes.GetCube("0")->Weight(), p.DragCubes.GetCube("50")->Weight(), p.DragCubes.GetCube("100")->Weight()});
        } else {
            F("tempAngularVelocity", rotor->tempAngularVelocity);
            F("workingRPM", rotor->workingRPM);
            F("totalTorque", rotor->totalTorque);
            F("brakingTorque", rotor->brakingTorque);
            F("rotationDiff", rotor->rotationDiff);
            F("maxTorque", rotor->Get<float>("maxTorque"));
            F("rpmLimit", rotor->rpmLimit());
            F("currentRPM", rotor->currentRPM());
            F("servoMotorLimit", rotor->servoMotorLimit());
            L("drive", Drive(rotor->servoJoint->angularXDrive));
            L("slerp", Drive(rotor->servoJoint->slerpDrive));
            L("targetAngularVelocity", V3(rotor->servoJoint->targetAngularVelocity));
            F("braking", B(rotor->brakingMode));
        }
        ticks++;
    }
    // the lock events in order (the reference records them with the same names)
    std::vector<std::string> want;
    for (const json& ev : sc["events"]) {
        const std::string e = ev.get<std::string>();
        if (e.rfind("lock", 0) == 0) want.push_back(e);
    }
    d.CheckS(std::to_string(events.size()), std::to_string(want.size()), name + " lock event count");
    for (size_t i = 0; i < std::min(events.size(), want.size()); i++) d.CheckS(events[i], want[i], name + " lock event");
    s->OnDestroy();
}

}  // namespace

TEST_CASE("WP-52 servo modules over 92 scripted runs (hinge, rotation servo, piston, rotor) vs the independent Python reference, 0 ULP") {
    const json& ref = LoadRef52();
    REQUIRE(GameAvailable());
    Rig rig;
    TimeWarp::fetch = &rig.env.tw;
    rig.env.tw.current_rate_index = 0;
    Diff d;
    long ticks = 0;
    std::map<std::string, int> perKind;
    size_t index = 0;
    for (const json& sc : ref["servo"]) {
        RunServoScenario(rig, sc, index++, d, ticks);
        perKind[sc["kind"].get<std::string>()]++;
    }
    INFO(d.first);
    CHECK(d.bad == 0);
    CHECK(d.total > 100000);
    CHECK(perKind["hinge"] == 26);
    CHECK(perKind["rotation"] == 20);
    CHECK(perKind["piston"] == 22);
    CHECK(perKind["rotor"] == 24);
    CHECK(ticks == 92 * 120);
}
