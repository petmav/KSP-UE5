// WP-52 derived cases, part 2 (labelled): closed-loop servo motion on the reference plant, mirrored / symmetric servos, the rotor's brake and ratchet,
// the editor visualisation, and the controller's axis / action bookkeeping, persistence and curve helpers. Expectations are derived by hand from the cited C# lines
// or from closed forms of the documented drive model; every servo / controller value is a SYNTHETIC fixture.
#include "robotics_fixture.hpp"

using namespace wp52t;

namespace {

float F32(double v) { return f32store(v); }

void StartServo(BaseServo& s, const Matrix4x4f& parent = Matrix4x4f::identity()) {
    WireRig(s, parent);
    s.OnStart(StartState::None);
    s.OnStartBeforePartAttachJoint(StartState::None);
}

// a flight rig with a big charge, the warp at 1x
struct Flight {
    Rig rig;
    std::unique_ptr<Rig::One> one;
    Flight() {
        TimeWarp::fetch = &rig.env.tw;
        rig.env.tw.current_rate_index = 0;
        Time::fixedDeltaTime = 0.02f;
        Time::deltaTime = 0.02f;
        one = rig.MakeOne();
    }
    Part& part() { return *one->part; }
};

// one fixed tick of the game: FixedUpdate / Update of the module, then the plant step
void Tick(BaseServo& s, HeadlessServoPlant& plant, double dt = 0.02) {
    s.FixedUpdate();
    s.Update();
    plant.Step(dt);
}

}  // namespace

TEST_CASE("WP-52 closed loop on the reference plant: the hinge follows the MoveTowards ramp of traverseVelocity and settles on the target") {
    Flight f;
    auto* h = AddServo<ModuleRoboticServoHinge>(f.part(), "ModuleRoboticServoHinge");
    h->Set<Vector2f>("softMinMaxAngles", Vector2f(0.0f, 120.0f));
    h->Set<Vector2f>("traverseVelocityLimits", Vector2f(0.1f, 180.0f));
    h->Set<float>("traverseVelocity", 30.0f);
    StartServo(*h);
    HeadlessServoPlant plant(*h);
    plant.SetInitial(0.0);
    h->Fields()["targetAngle"]->SetValue(ModuleValue(90.0f));
    double worstLag = 0.0, lagAt2s = 0.0;
    for (int t = 1; t <= 600; t++) {
        Tick(*h, plant);
        // the drive target is the MoveTowards ramp 30 deg/s * 0.02 s per tick, capped at the (clamped) target
        const double ramp = std::min(30.0 * 0.02 * t, 90.0);
        CHECK(h->driveTargetAngle == doctest::Approx(ramp).epsilon(1e-5));
        // the joint data the plant reads back is that drive target (physical coordinate, derivation in servo_joint.cpp)
        CHECK(PhysicalTargetAngleDegrees(*h->servoJoint) == doctest::Approx(h->driveTargetAngle).epsilon(1e-4).scale(1.0));
        const double angle = h->currentTransformAngle();
        if (t <= 150) worstLag = std::max(worstLag, h->driveTargetAngle - angle);
        if (t == 100) lagAt2s = h->driveTargetAngle - angle;
        // the plant never leaves the soft limits [0, 120]
        CHECK(angle >= -0.01);
        CHECK(angle <= 120.01);
    }
    // the drive (k = motorOutput * 100, c = 20 * 100 * 1 * 1, m = 1 SYNTHETIC) is heavily overdamped: tau = c / k = 0.2 s, so the lag behind a 30 deg/s ramp is
    // about 6 degrees; it is steady by 2 s
    INFO("lag at 2 s " << lagAt2s << " worst " << worstLag);
    CHECK(lagAt2s > 4.0);
    CHECK(lagAt2s < 8.0);
    CHECK(std::fabs(h->currentTransformAngle() - 90.0f) < 0.05f);
    // the rate of motion measured from the Transform is 30 deg/s during the ramp and ~0 at rest (IsMoving threshold 0.049)
    CHECK(!h->IsMoving());
    // retarget beyond the soft maximum: the joint target is clamped to 120 (JointTargetAngle) and the plant stops there
    h->Fields()["targetAngle"]->SetValue(ModuleValue(500.0f));
    CHECK(h->targetAngle() == 120.0f);
    for (int t = 0; t < 700; t++) Tick(*h, plant);
    CHECK(std::fabs(h->currentTransformAngle() - 120.0f) < 0.05f);
    // the motor is now at rest: a lock is accepted, the drive target stays at the lock angle
    CHECK(h->EngageServoLock());
    for (int t = 0; t < 100; t++) Tick(*h, plant);
    CHECK(h->driveTargetAngle == doctest::Approx(120.0f).epsilon(1e-5));
    CHECK(std::fabs(h->currentTransformAngle() - 120.0f) < 0.05f);
    h->OnDestroy();
}

TEST_CASE("WP-52 closed loop: the mirrored hinge (mirrorRotation) travels to the negated angle and its limits are the module's") {
    Flight f;
    auto* h = AddServo<ModuleRoboticServoHinge>(f.part(), "ModuleRoboticServoHinge");
    h->Set<Vector2f>("softMinMaxAngles", Vector2f(0.0f, 120.0f));
    h->Set<Vector2f>("traverseVelocityLimits", Vector2f(0.1f, 180.0f));
    h->Set<float>("traverseVelocity", 60.0f);
    h->Set<bool>("mirrorRotation", true);
    StartServo(*h);
    HeadlessServoPlant plant(*h);
    plant.SetInitial(0.0);
    h->Fields()["targetAngle"]->SetValue(ModuleValue(45.0f));
    CHECK(h->JointTargetAngle() == -45.0f);  // JointTargetAngle: mirrored
    for (int t = 0; t < 400; t++) Tick(*h, plant);
    CHECK(h->driveTargetAngle == doctest::Approx(-45.0f).epsilon(1e-6));
    // the joint limits of the module (OnJointInit: high = |soft.x - init| = 0, low = -(soft.y - init) = -120) allow [0, 120] physically in this model:
    // the mirrored target -45 is outside, so the plant stops at the 0 limit (the C# quirk that mirrored hinges are limited on the unmirrored side is kept)
    CHECK(std::fabs(h->currentTransformAngle()) < 0.05f);
    h->OnDestroy();
}

TEST_CASE("WP-52 closed loop: the rotor spools up by rotorSpoolTime, reaches workingRPM, brakes, and the measured rate of motion equals rpmLimit") {
    Flight f;
    auto* r = AddServo<ModuleRoboticServoRotor>(f.part(), "ModuleRoboticServoRotor");
    r->Set<Vector2f>("traverseVelocityLimits", Vector2f(0.1f, 460.0f));
    r->Set<float>("rpmLimit", 5.0f);
    StartServo(*r);
    HeadlessServoPlant plant(*r);
    // the motor limit is the 'throttle': OnStart zeroed it, the axis field raises it (ModifyServo through OnValueModified)
    CHECK(r->servoMotorLimit() == 0.0f);
    r->Fields()["servoMotorLimit"]->SetValue(ModuleValue(100.0f));
    const float want = F32(fx(5.0f) * fx(UtilMath::RPM2RadPerSec));
    CHECK(r->workingRPM == want);
    CHECK(r->totalTorque == F32(fx(r->Get<float>("maxTorque")) * (fx(100.0f) * fx(0.01f))));
    // the ramp: temp += rotorSpoolTime * fixedDeltaTime per tick until |temp| > |working|
    Tick(*r, plant);
    CHECK(r->tempAngularVelocity == F32(fx(r->Get<float>("rotorSpoolTime")) * fx(0.02f)));
    CHECK(r->servoJoint->targetAngularVelocity.x == r->tempAngularVelocity);
    for (int t = 0; t < 700; t++) Tick(*r, plant);
    CHECK(r->servoJoint->targetAngularVelocity.x == doctest::Approx(want).epsilon(0.05));
    // the plant reaches the target velocity (time constant 1 s: damper 1, inertia 1) and the Transform shows 5 rpm
    CHECK(plant.state.velocity == doctest::Approx(want).epsilon(1e-3));
    r->Update();
    CHECK(r->transformRateOfMotion == doctest::Approx(5.0f).epsilon(0.01));
    CHECK(r->IsMoving());
    CHECK(r->normalizedOutput() == doctest::Approx(1.0f).epsilon(0.01));
    // the reverse direction negates workingRPM
    r->Fields()["rotateCounterClockwise"]->SetValue(ModuleValue(true));
    CHECK(r->workingRPM == -want);
    // brake action: braking mode, drive torque = brakeTorque * brakePercentage * 0.01, the target velocity goes to zero
    KSPActionParam brake(0, KSPActionType::Activate);
    r->BrakeAction(brake);
    CHECK(r->brakingMode);
    CHECK(r->servoIsBraking);
    CHECK(r->brakingTorque == F32(fx(1.0f) * fx(100.0f) * fx(0.01f)));
    CHECK(r->workingRPM == 0.0f);
    for (int t = 0; t < 900; t++) Tick(*r, plant);
    CHECK(std::fabs(plant.state.velocity) < 1e-3);
    // toggle returns to free running
    KSPActionParam toggle(0, KSPActionType::Toggle);
    r->BrakeAction(toggle);
    CHECK(!r->brakingMode);
    CHECK(!r->servoIsBraking);
    // the motor power action zeroes / restores servoMotorLimit; refused while locked
    r->ToggleMotorPower();
    CHECK(r->servoMotorLimit() == 0.0f);
    r->ToggleMotorPower();
    CHECK(r->servoMotorLimit() == 100.0f);
    r->OnDestroy();
}

TEST_CASE("WP-52 closed loop: the piston ramps its extension at traverseVelocity and the Transform position follows the drive target") {
    Flight f;
    auto* p = AddServo<ModuleRoboticServoPiston>(f.part(), "ModuleRoboticServoPiston");
    p->Set<Vector2f>("softMinMaxExtension", Vector2f(0.0f, 2.0f));
    p->Set<Vector2f>("hardMinMaxLimits", Vector2f(0.0f, 2.0f));
    p->Set<Vector2f>("traverseVelocityLimits", Vector2f(0.05f, 5.0f));
    p->Set<float>("traverseVelocity", 0.5f);
    StartServo(*p);
    // the travel is the soft range; the joint is centred in it (connectedAnchor = mid, linearLimit.limit = range * 1.01 / 2)
    CHECK(p->servoJoint->linearLimit.limit == F32(fx(2.0f) * fx(1.01f) / 2.0));
    CHECK(p->servoJoint->connectedAnchor.z == 1.0f);
    HeadlessServoPlant plant(*p);
    plant.SetInitial(-1.0);  // offset from the middle: the extension starts at 0
    p->Fields()["targetExtension"]->SetValue(ModuleValue(1.5f));
    double lagAt2s = 0.0;
    for (int t = 1; t <= 300; t++) {
        Tick(*p, plant);
        CHECK(p->driveTargetPosition == doctest::Approx(std::min(0.5 * 0.02 * t, 1.5)).epsilon(1e-5));
        if (t == 100) lagAt2s = p->driveTargetPosition - p->currentTransformPosition();
    }
    // tau = c / k = (0.1 * 100 * 1 * 1) / (100 * 100) = 1 ms: no visible lag
    CHECK(std::fabs(lagAt2s) < 0.02);
    CHECK(p->currentTransformPosition() == doctest::Approx(1.5f).epsilon(1e-3));
    // currentExtension (UpdateCurrentExtension) is read from the joint target, equal to the drive target
    CHECK(p->currentExtension() == doctest::Approx(1.5f).epsilon(1e-4));
    p->OnDestroy();
}

TEST_CASE("WP-52 closed loop: the rotation servo with allowFullRotation takes the short way through 180 degrees (MoveTowardsAngle)") {
    Flight f;
    auto* rs = AddServo<ModuleRoboticRotationServo>(f.part(), "ModuleRoboticRotationServo");
    rs->Set<Vector2f>("traverseVelocityLimits", Vector2f(0.1f, 360.0f));
    rs->Set<float>("traverseVelocity", 100.0f);
    StartServo(*rs);
    CHECK(rs->allowFullRotation());
    CHECK(rs->softMinMaxAngles().x == -179.99f);  // ModifyLimitsMode set the full range
    CHECK(rs->softMinMaxAngles().y == 180.0f);
    HeadlessServoPlant plant(*rs);
    plant.SetInitial(0.0);
    rs->Fields()["targetAngle"]->SetValue(ModuleValue(170.0f));
    for (int t = 0; t < 400; t++) Tick(*rs, plant);
    CHECK(std::fabs(rs->currentTransformAngle() - 170.0f) < 0.5f);
    rs->Fields()["targetAngle"]->SetValue(ModuleValue(-170.0f));
    float maxAbs = 0.0f;
    int ticksToArrive = 0;
    for (int t = 0; t < 400; t++) {
        Tick(*rs, plant);
        maxAbs = std::max(maxAbs, std::fabs(rs->currentTransformAngle()));
        if (ticksToArrive == 0 && std::fabs(rs->driveTargetAngle - (-170.0f)) < 1e-3f) ticksToArrive = t + 1;
    }
    // 170 -> -170 is 20 degrees the short way: 20 / (100 * 0.02) = 10 ticks; the angle goes through +-180, never back through 0
    CHECK(ticksToArrive >= 10);
    CHECK(ticksToArrive <= 12);
    CHECK(maxAbs > 178.0f);
    CHECK(std::fabs(rs->currentTransformAngle() - (-170.0f)) < 0.5f);
    // allowFullRotation off restores the config limits and the previous soft range (ModifyLimitsMode)
    rs->Fields()["allowFullRotation"]->SetValue(ModuleValue(false));
    CHECK(rs->hardMinMaxLimits().x == 0.0f);
    CHECK(rs->hardMinMaxLimits().y == 180.0f);
    CHECK(rs->softMinMaxAngles().y == 177.0f);
    CHECK(!rs->Actions()["MinimumAngleAction"]->active == false);
    rs->OnDestroy();
}

TEST_CASE("WP-52 symmetric servos: CheckSymmetry decides mirrorRotation from the mirrored joint axis (ModuleRoboticServoHinge.cs:190-214)") {
    Flight f;
    Part& p1 = f.part();
    Part& p2 = *f.rig.AddPart(*f.one);
    auto* h1 = AddServo<ModuleRoboticServoHinge>(p1, "ModuleRoboticServoHinge");
    auto* h2 = AddServo<ModuleRoboticServoHinge>(p2, "ModuleRoboticServoHinge");
    p1.symmetryCounterparts = {&p2};
    p2.symmetryCounterparts = {&p1};
    StartServo(*h1);
    StartServo(*h2);
    h1->rig.modulePosition = Vector3f(1.0f, 0.0f, 0.0f);
    h2->rig.modulePosition = Vector3f(-1.0f, 0.0f, 0.0f);
    // radial symmetry: copy the counterpart's flag
    p1.fields.symMethod = static_cast<int>(SymmetryMethod::Radial);
    h2->Set<bool>("mirrorRotation", true);
    h1->CheckSymmetry();
    CHECK(h1->mirrorRotation());
    h2->Set<bool>("mirrorRotation", false);
    h1->CheckSymmetry();
    CHECK(!h1->mirrorRotation());
    // mirror symmetry, joint parents with the same orientation: the reflected axis is parallel to this one, so the flag is the opposite of the counterpart's
    p1.fields.symMethod = static_cast<int>(SymmetryMethod::Mirror);
    h1->jointParent->worldRotation = Quaternionf::identity();
    h2->jointParent->worldRotation = Quaternionf::identity();
    h2->Set<bool>("mirrorRotation", false);
    h1->CheckSymmetry();
    CHECK(h1->mirrorRotation());
    h2->Set<bool>("mirrorRotation", true);
    h1->CheckSymmetry();
    CHECK(!h1->mirrorRotation());
    // the counterpart turned 180 degrees about Y: its axis points the other way, the reflection is anti-parallel to ours: same flag
    h2->jointParent->worldRotation = UnityNative::AngleAxis(180.0f, Vector3f::up());
    h2->Set<bool>("mirrorRotation", false);
    h1->CheckSymmetry();
    CHECK(!h1->mirrorRotation());
    h2->Set<bool>("mirrorRotation", true);
    h1->CheckSymmetry();
    CHECK(h1->mirrorRotation());
    // no counterpart: the C# indexes symmetryCounterparts[0] (ArgumentOutOfRange)
    p1.symmetryCounterparts.clear();
    CHECK_THROWS(h1->CheckSymmetry());
    h1->OnDestroy();
    h2->OnDestroy();
}

TEST_CASE("WP-52 editor: OnEditorPartEvent re-checks the symmetry of a servo that is not an ancestor of the event's part and re-poses the model") {
    Flight f;
    Part& p1 = f.part();
    Part& p2 = *f.rig.AddPart(*f.one);
    auto* h1 = AddServo<ModuleRoboticServoHinge>(p1, "ModuleRoboticServoHinge");
    auto* h2 = AddServo<ModuleRoboticServoHinge>(p2, "ModuleRoboticServoHinge");
    p1.symmetryCounterparts = {&p2};
    p2.symmetryCounterparts = {&p1};
    SceneScope editor(GameScenes::EDITOR);
    for (auto* h : {h1, h2}) {
        WireRig(*h);
        h->movingPartObject = h->rig.Find("Moving");
        h->basePartObject = h->rig.Find("Base");
        h->jointParent = h->rig.Find("JointParent");
    }
    h1->rig.modulePosition = Vector3f(1.0f, 0.0f, 0.0f);
    h2->rig.modulePosition = Vector3f(-1.0f, 0.0f, 0.0f);
    p1.fields.symMethod = static_cast<int>(SymmetryMethod::Mirror);
    h2->Set<bool>("mirrorRotation", false);
    h1->Set<float>("targetAngle", 30.0f);
    // PartAttached for a part that is neither this one nor an ancestor: symmetry check + OnVisualizeServo(mirrorRotation)
    h1->OnEditorPartEvent(ConstructionEventType::PartAttached, &p2);
    CHECK(h1->mirrorRotation());  // anti-... parallel axes in Mirror mode: opposite of the counterpart's
    // OnVisualizeServo(rotateBase = true): the part turns by -(jointTarget - previousTarget), the moving transform takes the joint target angle
    const float jt = -30.0f;  // mirrored
    CHECK(h1->movingPartObject->localRotation.z == doctest::Approx(UnityNative::AngleAxis(jt, Vector3f::forward()).z).epsilon(1e-6));
    CHECK(p1.partTransform.rotation.z == doctest::Approx(UnityNative::AngleAxis(30.0f, Vector3f::forward()).z).epsilon(1e-6));
    CHECK(h1->previousTargetAngle() == jt);
    // an event for the part itself (or an ancestor) is ignored
    const float before = h1->previousTargetAngle();
    h1->Set<float>("targetAngle", 10.0f);
    h1->OnEditorPartEvent(ConstructionEventType::PartAttached, &p1);
    CHECK(h1->previousTargetAngle() == before);
    // PartOffsetting / PartRotating also re-check; PartDeleted does not
    h1->Set<bool>("mirrorRotation", false);
    h1->OnEditorPartEvent(ConstructionEventType::PartDeleted, &p2);
    CHECK(!h1->mirrorRotation());
    h1->OnEditorPartEvent(ConstructionEventType::PartRotating, &p2);
    CHECK(h1->mirrorRotation());
    // the rotation servo ignores PartAttached (ModuleRoboticRotationServo.cs:226)
    auto* rs = AddServo<ModuleRoboticRotationServo>(p1, "ModuleRoboticRotationServo");
    auto* rs2 = AddServo<ModuleRoboticRotationServo>(p2, "ModuleRoboticRotationServo");
    for (auto* r : {rs, rs2}) {
        WireRig(*r);
        r->movingPartObject = r->rig.Find("Moving");
        r->jointParent = r->rig.Find("JointParent");
    }
    rs->rig.modulePosition = Vector3f(1.0f, 0.0f, 0.0f);
    rs2->rig.modulePosition = Vector3f(-1.0f, 0.0f, 0.0f);
    rs->Set<bool>("mirrorRotation", false);
    rs->OnEditorPartEvent(ConstructionEventType::PartAttached, &p2);
    CHECK(!rs->mirrorRotation());
    rs->OnEditorPartEvent(ConstructionEventType::PartOffsetting, &p2);
    CHECK(rs->mirrorRotation());
    h1->OnDestroy();
    h2->OnDestroy();
    rs->OnDestroy();
    rs2->OnDestroy();
}

TEST_CASE("WP-52 rotor: RatchetLock frees the joint and freezes rotation about the main axis in the ratcheted direction") {
    Flight f;
    auto* r = AddServo<ModuleRoboticServoRotor>(f.part(), "ModuleRoboticServoRotor");
    StartServo(*r);
    r->DisengageMotor();
    CHECK(!r->servoMotorIsEngaged());
    r->Set<int32_t>("ratcheted", 2);
    // free spin: positive rotation about Z by 5 degrees beyond the previous reading is above the 0.2 tolerance
    r->movingPartObject->localRotation = UnityNative::AngleAxis(5.0f, Vector3f::forward());
    r->OnUpdate();
    CHECK(r->servoJoint->angularXMotion == ConfigurableJointMotion::Free);
    CHECK(r->Get<std::string>("motorState") == "Unpowered");
    CHECK(r->Get<std::string>("resourceConsumption") == "0.0");
    CHECK(r->movingPartRB->constraints == kFreezeRotationZ);       // ratcheted == 2 locks the positive direction
    CHECK(std::fabs(r->movingPartObject->localRotation.z) < 1e-6f);  // snapped back to the previous angle (0)
    CHECK(r->freespinPreviousRotation == doctest::Approx(5.0f).epsilon(1e-5));
    // ratcheted == 1 locks the negative direction only; a positive step leaves the body free
    r->Set<int32_t>("ratcheted", 1);
    r->movingPartObject->localRotation = UnityNative::AngleAxis(12.0f, Vector3f::forward());
    r->OnUpdate();
    CHECK(r->movingPartRB->constraints == 0);
    // a step below the tolerance changes nothing
    r->OnUpdate();
    CHECK(r->movingPartRB->constraints == 0);
    // engaged: RatchetLock returns at once and leaves the joint alone
    r->EngageMotor();
    r->servoJoint->angularXMotion = ConfigurableJointMotion::Limited;
    r->OnUpdate();
    CHECK(r->servoJoint->angularXMotion == ConfigurableJointMotion::Limited);
    r->OnDestroy();
}

TEST_CASE("WP-52 rotor: power loss (resource event) engages braking and the lock through the next fixed tick") {
    Flight f;
    auto* r = AddServo<ModuleRoboticServoRotor>(f.part(), "ModuleRoboticServoRotor");
    StartServo(*r);
    r->Fields()["servoMotorLimit"]->SetValue(ModuleValue(100.0f));
    PartResource* ec = f.part().GetResource("ElectricCharge");
    REQUIRE(ec != nullptr);
    r->resourceStateChange = false;
    // the empty event for another resource or another vessel changes nothing; for the servo's input on this vessel it flags the state change
    PartResource other;
    other.resourceName = "LiquidFuel";
    other.part = &f.part();
    r->OnResourceNonemptyEmpty(&other);
    CHECK(!r->resourceStateChange);
    ec->amount = 0.0;
    r->resHandler.inputResources[0]->currentAmount = 1.0;  // IsResourceBelowShutOffLimit (ModuleResourceHandler.cs:348): no shut-off handler -> true while currentAmount >= 0.01
    r->OnResourceNonemptyEmpty(ec);
    CHECK(r->resourceStateChange);
    // not engaged now (power gone), lockPartOnPowerLoss: the tick brakes and locks the rotor (ModuleRoboticServoRotor.cs:478-497)
    r->DisengageMotor();
    r->transformRateOfMotion = 0.0f;
    r->FixedUpdate();
    CHECK(r->brakingMode);
    CHECK(r->servoIsBraking);
    CHECK(r->servoIsLocked());
    CHECK(r->servoJoint->angularXDrive.maximumForce == 1.0f);  // SetJointDrive(1) after the lock re-poses the target rotation
    r->OnDestroy();
}

TEST_CASE("WP-52 piston: slave transforms interpolate between the moving and base objects; the 'TargetExtension' lookup of ModifyLimits finds no field") {
    Flight f;
    auto* p = AddServo<ModuleRoboticServoPiston>(f.part(), "ModuleRoboticServoPiston");
    p->Set<std::string>("slaveTransformNames", "SlaveA, SlaveB");
    auto a = std::make_shared<ServoNode>();
    auto b = std::make_shared<ServoNode>();
    p->rig.nodes["SlaveA"] = a;
    p->rig.nodes["SlaveB"] = b;
    StartServo(*p);
    REQUIRE(p->slaveTransformsSet);
    CHECK(p->slaveTransforms.size() == 2);
    p->movingPartObject->localPosition = Vector3f(0.0f, 0.0f, 3.0f);
    p->basePartObject->localPosition = Vector3f(0.0f, 0.0f, 0.0f);
    p->LateUpdate();
    // slave i sits at moving * (1 - t) + base * t with t = (i + 1) / 3
    const float third = F32(1.0 / fx(3.0f));                 // 1f / (float)(slaveTransforms.Length + 1)
    const float t1 = F32(fx(1.0f) * fx(third)), t2 = F32(fx(2.0f) * fx(third));
    CHECK(a->localPosition.z == F32(fx(F32(fx(3.0f) * fx(F32(1.0 - fx(t1))))) + fx(F32(fx(0.0f) * fx(t1)))));
    CHECK(b->localPosition.z == F32(fx(F32(fx(3.0f) * fx(F32(1.0 - fx(t2))))) + fx(F32(fx(0.0f) * fx(t2)))));
    // an unknown slave name logs and drops the list
    LogCapture logs;
    auto one2 = f.rig.MakeOne();
    auto* p2 = AddServo<ModuleRoboticServoPiston>(*one2->part, "ModuleRoboticServoPiston");
    p2->Set<std::string>("slaveTransformNames", "Nope");
    StartServo(*p2);
    CHECK(!p2->slaveTransformsSet);
    CHECK(logs.Has(log::Level::Error, "[ModuleRoboticServoPiston] No slave transform named Nope"));
    // with 'hard' range 0..100 and a 25 % target the extension actions work: ExtendPiston / RetractPiston / TogglePistonAction
    p->ExtendPiston();
    CHECK(p->targetExtension() == p->softMinMaxExtension().y);
    p->RetractPiston();
    CHECK(p->targetExtension() == p->softMinMaxExtension().x);
    KSPActionParam ap;
    p->TogglePistonAction(ap);
    CHECK(p->targetExtension() == p->softMinMaxExtension().y);
    // locked: the extension actions post the refusal message
    p->Set<bool>("servoIsLocked", true);
    ServoEnvironment::screenMessages.clear();
    p->ExtendPiston();
    CHECK(ServoEnvironment::screenMessages == std::vector<std::string>{"#autoLOC_8002356"});
    p->OnDestroy();
    p2->OnDestroy();
}

TEST_CASE("WP-52 attach nodes of a servo follow the moving transform every fixed tick (RecurseAttachNodeUpdate)") {
    Flight f;
    auto* h = AddServo<ModuleRoboticServoHinge>(f.part(), "ModuleRoboticServoHinge");
    h->Set<std::string>("servoAttachNodes", "bottom");
    float cur = 0.0f;
    h->angleSource = [&cur] { return cur; };
    StartServo(*h);
    REQUIRE(h->attachNodes.size() == 1);
    AttachNode* n = h->attachNodes[0];
    // the node transform moves with the model: its world pose becomes the node's part-space position / orientation (Transform.InverseTransformPoint / Direction)
    f.part().partTransform.position = Vector3f(10.0f, 0.0f, 0.0f);
    f.part().partTransform.rotation = Quaternionf::identity();
    ServoNode& t = *h->attachNodeTransforms["bottom"];
    t.worldPosition = Vector3f(10.0f, -2.0f, 0.5f);
    t.worldRotation = UnityNative::AngleAxis(90.0f, Vector3f::right());
    h->FixedUpdate();
    CHECK(n->position.y == doctest::Approx(-2.0f).epsilon(1e-6));
    CHECK(n->position.z == doctest::Approx(0.5f).epsilon(1e-6));
    CHECK(n->position.x == doctest::Approx(0.0f).scale(1.0).epsilon(1e-6));
    // forward (0,0,1) rotated 90 degrees about X is (0,-1,0)
    CHECK(n->orientation.y == doctest::Approx(-1.0f).epsilon(1e-5));
    CHECK(std::fabs(n->orientation.z) < 1e-5f);
    h->OnDestroy();
}
