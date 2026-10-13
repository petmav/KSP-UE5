// WP-52 derived cases (labelled): the C# control flow of the robotics modules that the scripted references do not drive. Nothing here claims game
// data: every servo / controller MODULE value is a SYNTHETIC fixture; the expectations are derived by hand from the cited C# lines.
#include "robotics_fixture.hpp"

#include "ksp/modules/part_module.hpp"
#include "ksp/physics/physics_world.hpp"
#include "vessel_physics_fixture.hpp"

using namespace wp52t;

namespace {

float F32(double v) { return f32store(v); }

// OnStart + OnStartBeforePartAttachJoint of a servo wired with the standard three nodes
void StartServo(BaseServo& s, const Matrix4x4f& parent = Matrix4x4f::identity()) {
    WireRig(s, parent);
    s.OnStart(StartState::None);
    s.OnStartBeforePartAttachJoint(StartState::None);
}

}  // namespace

TEST_CASE("WP-52 registration: the five module classes are the ported classes and carry the C# initialisers") {
    Rig rig;
    auto one = rig.MakeOne();
    Part& p = *one->part;
    CHECK(ModuleFactory::IsRegistered("ModuleRoboticServoHinge"));
    CHECK(ModuleFactory::IsRegistered("ModuleRoboticServoPiston"));
    CHECK(ModuleFactory::IsRegistered("ModuleRoboticServoRotor"));
    CHECK(ModuleFactory::IsRegistered("ModuleRoboticRotationServo"));
    CHECK(ModuleFactory::IsRegistered("ModuleRoboticController"));
    auto* h = AddServo<ModuleRoboticServoHinge>(p, "ModuleRoboticServoHinge");
    auto* pi = AddServo<ModuleRoboticServoPiston>(p, "ModuleRoboticServoPiston");
    auto* ro = AddServo<ModuleRoboticServoRotor>(p, "ModuleRoboticServoRotor");
    auto* rs = AddServo<ModuleRoboticRotationServo>(p, "ModuleRoboticRotationServo");
    auto* c = AddController(p);
    for (BaseServo* s : {static_cast<BaseServo*>(h), static_cast<BaseServo*>(pi), static_cast<BaseServo*>(ro), static_cast<BaseServo*>(rs)}) {
        CHECK(s->IsA("BaseServo"));
        // BaseServo.cs:18-139 initialisers
        CHECK(s->servoMotorSize() == 100.0f);
        CHECK(s->servoMotorLimit() == 100.0f);
        CHECK(s->maxMotorOutput() == 100.0f);
        CHECK(s->servoMotorIsEngaged());
        CHECK(s->servoIsMotorized());
        CHECK(!s->servoIsLocked());
        CHECK(s->lockPartOnPowerLoss());
        CHECK(s->mainAxis() == "Z");
        CHECK(s->Get<std::string>("jointParentName") == "JointParent");
        CHECK(s->hardMinMaxLimits().x == 0.0f);
        CHECK(s->hardMinMaxLimits().y == 180.0f);
        CHECK(s->traverseVelocityLimits().x == 0.1f);
        CHECK(s->traverseVelocityLimits().y == 10.0f);
        CHECK(s->Get<float>("servoMass") == 0.1f);
        CHECK(s->Get<float>("efficiency") == 1.0f);
        CHECK(s->Get<float>("motorizedMassPerKN") == 0.1f);
        CHECK(s->Get<float>("motorizedCostPerDriveUnit") == 1.0f);
        CHECK(s->Get<float>("baseResourceConsumptionRate") == 0.001f);
        CHECK(s->Get<float>("referenceConsumptionVelocity") == 90.0f);
        CHECK(s->resourceConsumptionMode() == BaseServo::ResourceConsumptionTypes::CurrentVelocity);
        CHECK(s->AsMassModifier() == s);
        CHECK(s->AsCostModifier() == s);
        CHECK(s->AsConstruction() == s);
        CHECK(dynamic_cast<IJointLockState*>(s) != nullptr);
        CHECK(dynamic_cast<IAxisFieldLimits*>(s) != nullptr);
    }
    // ModuleRoboticServoHinge.cs:10-71
    CHECK(h->softMinMaxAngles().y == 177.0f);
    CHECK(h->traverseVelocity() == 90.0f);
    CHECK(h->hingeDamping() == 100.0f);
    CHECK(h->Get<float>("driveDampingMutliplier") == 20.0f);
    CHECK(h->Get<float>("driveSpringMutliplier") == 100.0f);
    // ModuleRoboticServoPiston.cs:10-62
    CHECK(pi->softMinMaxExtension().y == 100.0f);
    CHECK(pi->traverseVelocity() == 1.0f);
    CHECK(pi->pistonDamping() == 100.0f);
    CHECK(pi->Get<float>("linearLimitBounce") == 0.2f);
    CHECK(pi->Get<float>("linearLimitSpringSpring") == 100000.0f);
    CHECK(pi->Get<float>("linearLimitContactDistance") == 1.0f);
    CHECK(pi->Get<float>("positionDampingMutliplier") == 0.1f);
    CHECK(pi->Get<float>("positionSpringMutliplier") == 100.0f);
    // ModuleRoboticServoRotor.cs:11-57
    CHECK(ro->rpmLimit() == 230.0f);
    CHECK(ro->brakePercentage() == 100.0f);
    CHECK(ro->Get<float>("rotationMatch") == 20.0f);
    CHECK(ro->Get<float>("rotorSpoolTime") == 3.0f);
    CHECK(ro->Get<float>("brakeTorque") == 1.0f);
    CHECK(ro->Get<float>("maxTorque") == 400.0f);
    CHECK(ro->Get<float>("angularPositionDamper") == 1.0f);
    CHECK(ro->Get<float>("angularPositionSpring") == 1E-10f);
    CHECK(ro->angularDriveMode() == "XYAndZ");
    // ModuleRoboticRotationServo.cs:11-78
    CHECK(rs->allowFullRotation());
    CHECK(rs->traverseVelocity() == 90.0f);
    CHECK(rs->invertedSoftMinMaxAngles.y == -177.0f);
    // ModuleRoboticController.cs:26-75
    CHECK(c->SequenceLength() == 5.0f);
    CHECK(c->SequencePlaySpeed() == 100.0f);
    CHECK(c->controllerEnabled());
    CHECK(!c->SequenceIsPlaying());  // sequencePlayingIndex = 1
    CHECK(c->SequenceDirection() == ModuleRoboticController_SequenceDirectionOptions::Forward);
    CHECK(c->SequenceLoop() == ModuleRoboticController_SequenceLoopOptions::Once);
    CHECK(c->Get<float>("priorityField") == 3.0f);
    // the part is robotic (Part.cs:1362) once Serenity is installed
    SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", false);
    CHECK(!p.isRobotic());
    SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", true);
    CHECK(p.isRobotic());
    // the actions and events of BaseServo and the controller are bound to handlers
    CHECK(h->Actions()["ToggleServoLockedAction"] != nullptr);
    CHECK(h->Actions()["ToggleServoLockedAction"]->onEvent);
    CHECK(h->Events()["ResetPosition"]->onEvent);
    CHECK(c->Actions()["PlaySequenceAction"]->onEvent);
    CHECK(c->Events()["OpenControllerEditor"]->onEvent);
}

TEST_CASE("WP-52 BaseServo lifecycle: OnStart builds the joint, the servo body and the attach node transforms; OnSave / OnLoad keep the transform") {
    Rig rig;
    auto one = rig.MakeOne();
    Part& p = *one->part;
    TimeWarp::fetch = &rig.env.tw;
    auto* h = AddServo<ModuleRoboticServoHinge>(p, "ModuleRoboticServoHinge");
    h->Set<std::string>("servoAttachNodes", "bottom");
    h->Set<std::string>("servoSrfMeshNames", "meshA,meshB");
    Time::fixedDeltaTime = 0.02f;
    WireRig(*h);
    h->OnStart(StartState::None);
    CHECK(h->movingPartObject != nullptr);
    CHECK(h->basePartObject != nullptr);
    CHECK(h->jointParent != nullptr);
    CHECK(h->movingPartRB != nullptr);                // CreateServoRigidbody in the flight scene
    CHECK(h->movingPartRB->mass == 0.1f);             // servoMass
    CHECK(h->movingPartRB->useGravity == false);
    CHECK(h->movingPartRB->isKinematic == false);     // part.packed == false
    REQUIRE(h->servoJoint != nullptr);
    CHECK(h->servoJoint->angularXMotion == ConfigurableJointMotion::Limited);
    CHECK(h->servoJoint->xMotion == ConfigurableJointMotion::Locked);
    CHECK(h->servoJoint->axis.z == 1.0f);             // GetMainAxis() for "Z"
    CHECK(h->attachNodes.size() == 1);
    CHECK(h->attachNodes[0]->id == "bottom");
    REQUIRE(h->attachNodeTransforms.count("bottom") == 1);
    CHECK(h->attachNodeTransforms["bottom"]->parent == h->movingPartObject);  // SetParent(movingPartObject)
    CHECK(h->servoSrfMeshes.size() == 2);
    CHECK(h->ServoTransformCollider("meshB"));
    CHECK(!h->ServoTransformCollider("meshC"));
    CHECK(h->AttachServoRigidBody(*h->attachNodes[0]) == BaseServo::BodyOwner::Servo);
    CHECK(h->NodeRigidBody(*h->attachNodes[0]) == BaseServo::BodyOwner::Servo);
    AttachNode* top = p.FindAttachNode("top");
    REQUIRE(top != nullptr);
    CHECK(h->AttachServoRigidBody(*top) == BaseServo::BodyOwner::Part);
    h->OnStartBeforePartAttachJoint(StartState::None);
    CHECK(h->ServoInitComplete());
    CHECK(h->servoJoint->hasConnectedBody);
    // PostStartInitJoint (BaseServo.cs:1132-1175): cachedStartingRotation = identity, targetRotation = the moving transform's rotation
    CHECK(h->cachedStartingRotation.w == 1.0f);
    // Save: the moving transform is written (BaseServo.cs:564-585)
    h->movingPartObject->localPosition = Vector3f(0.0f, 0.5f, 0.0f);
    h->movingPartObject->localRotation = UnityNative::AngleAxis(30.0f, Vector3f::forward());
    h->jointParent->localRotation = UnityNative::AngleAxis(10.0f, Vector3f::up());
    ConfigNode node("MODULE");
    h->Save(node);
    CHECK(node.HasValue("servoTransformPosition"));
    CHECK(node.HasValue("servoTransformRotation"));
    CHECK(node.HasValue("jointParentRotation"));
    CHECK(node.HasValue("servoIsLocked"));
    // a second module loads that node: OnLoad flags the loaded transforms, OnStart restores the joint parent rotation, PostStartInitJoint the moving transform
    auto one2 = rig.MakeOne();
    auto* h2 = AddServo<ModuleRoboticServoHinge>(*one2->part, "ModuleRoboticServoHinge");
    h2->Load(node);
    CHECK(h2->servoTransformPosLoaded);
    CHECK(h2->servoTransformRotLoaded);
    CHECK(h2->jointParentRotationLoaded);
    WireRig(*h2);
    h2->OnStart(StartState::None);
    CHECK(h2->jointParent->localRotation.y == doctest::Approx(h->jointParent->localRotation.y).epsilon(1e-6));
    h2->OnStartBeforePartAttachJoint(StartState::None);
    CHECK(h2->movingPartObject->localPosition.y == 0.5f);
    CHECK(h2->movingPartObject->localRotation.z == doctest::Approx(h->movingPartObject->localRotation.z).epsilon(1e-6));
    h->OnDestroy();
    h2->OnDestroy();
}

TEST_CASE("WP-52 BaseServo.Start and the construction vetoes") {
    Rig rig;
    auto one = rig.MakeOne();
    auto* h = AddServo<ModuleRoboticServoHinge>(*one->part, "ModuleRoboticServoHinge");
    // BaseServo.cs:865-871: Serenity absent in a game scene destroys the module
    SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", false);
    SaveEnvironment::Current().loadedSceneIsGame = true;
    h->Start();
    CHECK(h->destroyed);
    CHECK(!h->behaviourEnabled);
    SaveEnvironment::Current().loadedSceneIsGame = false;
    // BaseServo.cs:1795-1807: CanBeDetached / CanBeOffset / CanBeRotated are false while the servo moves
    h->transformRateOfMotion = 0.0f;
    CHECK(h->CanBeDetached());
    h->transformRateOfMotion = 1.0f;
    CHECK(!h->CanBeDetached());
    CHECK(!h->CanBeOffset());
    CHECK(!h->CanBeRotated());
    // the mass / cost modifiers (BaseServo.cs:1671-1699) follow servoIsMotorized
    h->Set<float>("servoMotorSize", 50.0f);
    h->Set<float>("maxMotorOutput", 40.0f);
    // GetModuleMass reads driveUnit, which only SetDriveUnit (GetModuleCost, ModifyServo, GetInfo) refreshes: stale (0) until then
    CHECK(h->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == 0.0f);
    CHECK(h->GetModuleCost(0.0f, ModifierStagingSituation::CURRENT) == F32(fx(1.0f) * fx(F32(fx(50.0f) * fx(0.01f) * fx(40.0f)))));
    CHECK(h->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == F32(fx(0.1f) * fx(F32(fx(50.0f) * fx(0.01f) * fx(40.0f)))));
    h->Set<bool>("servoIsMotorized", false);
    CHECK(h->GetModuleMass(0.0f, ModifierStagingSituation::CURRENT) == 0.0f);
    CHECK(h->GetModuleCost(0.0f, ModifierStagingSituation::CURRENT) == 0.0f);
    CHECK(h->GetModuleMassChangeWhen() == ModifierChangeWhen::CONSTANTLY);
}

TEST_CASE("WP-52 axis field limits (IAxisFieldLimits): hard / soft limits, clamps, the unknown-field answers") {
    Rig rig;
    auto one = rig.MakeOne();
    Part& p = *one->part;
    TimeWarp::fetch = &rig.env.tw;
    auto* h = AddServo<ModuleRoboticServoHinge>(p, "ModuleRoboticServoHinge");
    StartServo(*h);
    CHECK(h->HasAxisFieldLimits());
    CHECK(h->HasAxisFieldLimit("targetAngle"));
    CHECK(!h->HasAxisFieldLimit("other"));
    CHECK(h->GetAxisFieldLimit("other") == nullptr);
    // BaseServo.cs:1772-1788: Vector2.one for an unknown field
    CHECK(h->GetHardLimits("other").x == 1.0f);
    CHECK(h->GetSoftLimits("other").y == 1.0f);
    CHECK(h->GetHardLimits("targetAngle").y == 180.0f);
    // SetSoftLimits clamps to the hard limits (BaseServo.cs:1721-1730), then the hinge applies it (ModifyLimits)
    h->SetSoftLimits("targetAngle", Vector2f(-20.0f, 400.0f));
    CHECK(h->softMinMaxAngles().x == 0.0f);
    CHECK(h->softMinMaxAngles().y == 180.0f);
    CHECK(h->GetSoftLimits("targetAngle").y == 180.0f);
    CHECK(h->GetAxisFieldLimits().size() == 1);
    // SetHardLimits writes hardMinMaxLimits and re-runs ModifyLimits
    h->SetHardLimits("targetAngle", Vector2f(10.0f, 100.0f));
    CHECK(h->hardMinMaxLimits().y == 100.0f);
    h->SetSoftLimits("targetAngle", Vector2f(0.0f, 150.0f));
    CHECK(h->softMinMaxAngles().x == 10.0f);
    CHECK(h->softMinMaxAngles().y == 100.0f);
    // the axis field's own limits follow the soft ones (ModifyLimits), and targetAngle is clamped into them
    BaseAxisField* ta = dynamic_cast<BaseAxisField*>(h->Fields()["targetAngle"]);
    REQUIRE(ta != nullptr);
    CHECK(ta->minValue == 10.0f);
    CHECK(ta->maxValue == 100.0f);
    h->Fields()["targetAngle"]->SetValue(ModuleValue(500.0f));
    CHECK(h->targetAngle() == 100.0f);
    // LimitsChanged fires with the limit record
    int calls = 0;
    h->LimitsChanged = Delegate<void(AxisFieldLimit*)>::Lambda(&calls, [&calls](AxisFieldLimit*) { calls++; });
    h->SetSoftLimits("targetAngle", Vector2f(20.0f, 90.0f));
    CHECK(calls == 1);
    // a limit entry added twice is the C# Dictionary.Add exception
    CHECK_THROWS(h->AddAxisFieldLimit("targetAngle", AxisFieldLimit{}));
    h->OnDestroy();
}

TEST_CASE("WP-52 lock logic: EngageServoLock is refused while moving, ModifyLocked runs twice through the field event, events and autostrut cycling") {
    Rig rig;
    auto one = rig.MakeOne();
    Part& p = *one->part;
    TimeWarp::fetch = &rig.env.tw;
    auto* h = AddServo<ModuleRoboticServoHinge>(p, "ModuleRoboticServoHinge");
    float cur = 12.0f;
    h->angleSource = [&cur] { return cur; };
    StartServo(*h);
    std::vector<std::string> ev;
    struct Sub {
        std::vector<std::string>* ev;
        void A(Part*, bool b) { ev->push_back(b ? "changing:locked" : "changing:unlocked"); }
        void B(Part*, bool b) { ev->push_back(b ? "changed:locked" : "changed:unlocked"); }
    } sub{&ev};
    GameEvents::onRoboticPartLockChanging.Add(&sub, &Sub::A);
    GameEvents::onRoboticPartLockChanged.Add(&sub, &Sub::B);
    h->transformRateOfMotion = 5.0f;
    CHECK(!h->EngageServoLock());  // BaseServo.cs:966: IsMoving() is true
    CHECK(!h->servoIsLocked());
    h->transformRateOfMotion = 0.0f;
    ServoEnvironment::cycleAllAutoStrutCalls = 0;
    CHECK(h->EngageServoLock());
    CHECK(h->servoIsLocked());
    // the SetValue fires ModifyLocked (subscribed in OnStart) and EngageServoLock calls it again: prevServoIsLocked has not changed in between
    CHECK(ev == std::vector<std::string>{"changing:locked", "changed:locked", "changing:locked", "changed:locked"});
    CHECK(ServoEnvironment::cycleAllAutoStrutCalls == 2);
    CHECK(h->lockAngle == F32(fx(cur) + fx(h->modelInitialAngle())));
    CHECK(h->EngageServoLock());  // already locked: true, no events
    CHECK(ev.size() == 4);
    // Update copies the lock flag into prevServoIsLocked (PAW closed), after which ModifyLocked(unlock) is the removal branch
    h->Update();
    CHECK(h->prevServoIsLocked);
    ev.clear();
    h->DisengageServoLock();
    CHECK(!h->servoIsLocked());
    CHECK(ev.size() == 4);  // twice again
    CHECK(ev[0] == "changing:unlocked");
    CHECK(h->driveTargetAngle == cur);  // OnServoLockRemoved: driveTargetAngle = currentTransformAngle()
    // the toggle action: a refused lock posts the screen message
    ServoEnvironment::screenMessages.clear();
    h->transformRateOfMotion = 5.0f;
    KSPActionParam ap;
    h->ToggleServoLockedAction(ap);
    CHECK(ServoEnvironment::screenMessages == std::vector<std::string>{"#autoLOC_8003335"});
    // the editor scene cycles every part's autostrut instead
    GameEvents::onRoboticPartLockChanging.Remove(&sub, &Sub::A);
    GameEvents::onRoboticPartLockChanged.Remove(&sub, &Sub::B);
    h->OnDestroy();
}

TEST_CASE("WP-52 motor engage / disengage and power loss: the charge running out disengages the motor and locks the part") {
    Rig rig;
    auto one = rig.MakeOne("mk1pod.v2", 1.0e6);
    Part& p = *one->part;
    TimeWarp::fetch = &rig.env.tw;
    Time::fixedDeltaTime = 0.02f;
    Time::deltaTime = 0.02f;
    auto* h = AddServo<ModuleRoboticServoHinge>(p, "ModuleRoboticServoHinge", 1.0);
    h->Set<float>("baseResourceConsumptionRate", 0.5f);
    float cur = 10.0f;
    h->angleSource = [&cur] { return cur; };
    StartServo(*h);
    CHECK(h->servoMotorIsEngaged());
    // moving at 1 deg/tick -> 50 deg/s: power draw rate = 0.5 * motorOutput * (rate / 90)
    for (int i = 0; i < 40; i++) {
        cur += 1.0f;
        h->FixedUpdate();
    }
    CHECK(h->rate > 0.001);
    CHECK(h->HasEnoughResources());
    const double used = 1.0e6 - p.GetResource("ElectricCharge")->amount;
    CHECK(used > 0.0);
    // the charge runs out while the servo is still moving: CalculateResourceDrain disengages the motor (BaseServo.cs:428-433) and zeroes the drive
    p.GetResource("ElectricCharge")->amount = 0.0;
    p.ResetSimulationResources();
    one->host->Update();
    cur += 1.0f;
    h->FixedUpdate();
    CHECK(!h->HasEnoughResources());
    CHECK(!h->servoMotorIsEngaged());
    CHECK(h->rate == 0.0);
    // not engaged, not locked, lockPartOnPowerLoss: once the part stops (rate of motion below 0.049) the next tick locks it (ModuleRoboticServoHinge.cs:499-504)
    for (int i = 0; i < 60 && !h->servoIsLocked(); i++) h->FixedUpdate();
    CHECK(h->servoIsLocked());
    CHECK(h->driveTargetAngle == h->lockAngle);
    // with the lock on and no charge the motor stays off: HasEnoughResourcesToAutoStart is false (no start-up handler)
    for (int i = 0; i < 5; i++) h->FixedUpdate();
    CHECK(!h->servoMotorIsEngaged());
    // the drive of an unlocked, un-engaged, lockPartOnPowerLoss servo is left as it was; with lockPartOnPowerLoss false it is zeroed (:458-465)
    h->Set<bool>("lockPartOnPowerLoss", false);
    h->DisengageServoLock();
    h->ModifyServo(nullptr);
    CHECK(h->servoJoint->angularXDrive.positionSpring == 0.0f);
    CHECK(h->servoJoint->angularXDrive.maximumForce == 0.0f);
    h->OnDestroy();
}

TEST_CASE("WP-52 high physics warp zeroes the joint drives (OnModifyServo warp branches)") {
    Rig rig;
    auto one = rig.MakeOne();
    Part& p = *one->part;
    TimeWarp::fetch = &rig.env.tw;
    auto* h = AddServo<ModuleRoboticServoHinge>(p, "ModuleRoboticServoHinge");
    auto* ro = AddServo<ModuleRoboticServoRotor>(p, "ModuleRoboticServoRotor");
    auto* pi = AddServo<ModuleRoboticServoPiston>(p, "ModuleRoboticServoPiston");
    auto* rs = AddServo<ModuleRoboticRotationServo>(p, "ModuleRoboticRotationServo");
    float cur = 0.0f;
    for (BaseServo* s : {static_cast<BaseServo*>(h), static_cast<BaseServo*>(ro), static_cast<BaseServo*>(pi), static_cast<BaseServo*>(rs)}) {
        s->angleSource = [&cur] { return cur; };
        s->positionSource = [&cur] { return cur; };
        StartServo(*s);
    }
    ro->Fields()["servoMotorLimit"]->SetValue(ModuleValue(100.0f));
    rig.env.tw.current_rate_index = 0;
    h->ModifyServo(nullptr);
    CHECK(h->servoJoint->angularXDrive.positionSpring > 0.0f);
    CHECK(pi->servoJoint->xDrive.positionSpring > 0.0f);
    CHECK(rs->servoJoint->angularXDrive.maximumForce > 0.0f);
    CHECK(ro->servoJoint->angularXDrive.maximumForce > 0.0f);
    rig.env.tw.current_rate_index = 3;  // TimeWarp.CurrentRateIndex > 0 && WarpMode == HIGH
    rig.env.tw.Mode = TimeWarp::Modes::HIGH;
    for (BaseServo* s : {static_cast<BaseServo*>(h), static_cast<BaseServo*>(ro), static_cast<BaseServo*>(pi), static_cast<BaseServo*>(rs)}) s->ModifyServo(nullptr);
    CHECK(h->servoJoint->angularXDrive.positionSpring == 0.0f);
    CHECK(h->servoJoint->angularXDrive.positionDamper == 0.0f);
    CHECK(h->servoJoint->angularXDrive.maximumForce == 0.0f);
    CHECK(rs->servoJoint->angularXDrive.maximumForce == 0.0f);
    CHECK(pi->servoJoint->xDrive.positionSpring == 0.0f);
    CHECK(pi->servoJoint->xDrive.maximumForce == 0.0f);
    CHECK(ro->servoJoint->angularXDrive.maximumForce == 0.0f);  // SetJointDrive(0)
    CHECK(ro->servoJoint->targetAngularVelocity.x == 0.0f);
    // low warp (rails warp is not physics warp): the drives come back
    rig.env.tw.Mode = TimeWarp::Modes::LOW;
    h->ModifyServo(nullptr);
    CHECK(h->servoJoint->angularXDrive.positionSpring > 0.0f);
    rig.env.tw.current_rate_index = 0;
    rig.env.tw.Mode = TimeWarp::Modes::HIGH;
    for (BaseServo* s : {static_cast<BaseServo*>(h), static_cast<BaseServo*>(ro), static_cast<BaseServo*>(pi), static_cast<BaseServo*>(rs)}) s->OnDestroy();
}

TEST_CASE("WP-52 PartJoint rules: a servo is an IJointLockState and an articulation point of the reduced model") {
    Rig rig;
    // pod (parent) - fuel tank (child) with a servo on the tank; the child's "top" node is the one attached to the pod
    Game15& g = rig.g;
    auto pod = g.New("mk1pod.v2");
    auto tank = g.New("fuelTank");
    auto pod2 = g.New("mk1pod.v2");
    pod->fields.persistentId = 11;
    tank->fields.persistentId = 12;
    pod2->fields.persistentId = 13;
    Attach(*pod, "bottom", *tank, "top");
    Attach(*tank, "bottom", *pod2, "bottom");
    auto vessel = rig.env.MakeVessel("chain", rig.env.Body("Kerbin"), 700000.0, 0.0, true, false, OrbitDriver::UpdateMode::UPDATE, false);
    for (Part* q : {pod.get(), tank.get(), pod2.get()}) {
        q->vessel = vessel;
        vessel->parts.push_back(q);
    }
    vessel->rootPart = pod.get();
    TestVesselHost host(vessel);
    host.Update();
    TimeWarp::fetch = &rig.env.tw;
    auto* hinge = AddServo<ModuleRoboticServoHinge>(*tank, "ModuleRoboticServoHinge");
    hinge->Set<std::string>("servoAttachNodes", "top");
    StartServo(*hinge);
    // physics::PartJointSpec of the pod's child (the tank) and of the pod2's child relation
    auto spec = [](Part* child, Part* parent) {
        physics::PartJointSpec s;
        s.kind = physics::JointKind::Attach;
        s.child = child;
        s.parent = parent;
        return s;
    };
    // BaseServo.AttachServoRigidBody(top) is the servo body: the joint between the tank (a servo whose moving side is its top node) and the pod
    CHECK(ServoArticulation(spec(tank.get(), pod.get())));
    // the other end of the tank (bottom) is the servo's static side: no articulation
    CHECK(!ServoArticulation(spec(pod2.get(), tank.get())));
    // a joint with no servo at either end, a strut and an autostrut are never articulation points
    CHECK(!ServoArticulation(spec(pod.get(), pod2.get())));
    physics::PartJointSpec strut = spec(tank.get(), pod.get());
    strut.kind = physics::JointKind::Strut;
    CHECK(!ServoArticulation(strut));
    // IJointLockState: IsJointUnlocked == !servoIsLocked
    CHECK(hinge->IsJointUnlocked());
    hinge->Set<bool>("servoIsLocked", true);
    CHECK(!hinge->IsJointUnlocked());
    hinge->OnDestroy();
    vessel->parts.clear();
    for (Part* q : {pod.get(), tank.get(), pod2.get()}) q->vessel = nullptr;
}

TEST_CASE("WP-52 reduced rigid-body model: the servo hook turns the servo's moving-side joint into an articulation point (two clusters, one constraint)") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w;
    SceneScope flight(GameScenes::FLIGHT);
    SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", true);
    auto v = w.Craft("PhysProf-S");
    Part* mainsail = nullptr;
    for (Part* p : v->parts)
        if (p->partInfo != nullptr && p->partInfo->name == "liquidEngineMainsail.v2") mainsail = p;
    REQUIRE(mainsail != nullptr);
    // a servo module on the Mainsail whose moving side is its "top" node (SYNTHETIC: no real servo part is installed)
    ConfigNode node("MODULE");
    node.AddValue("name", "ModuleRoboticServoHinge");
    REQUIRE(mainsail->AddModule(node, true) != nullptr);
    auto* hinge = dynamic_cast<ModuleRoboticServoHinge*>(mainsail->FindModuleOfClass("ModuleRoboticServoHinge"));
    REQUIRE(hinge != nullptr);
    hinge->Set<std::string>("servoAttachNodes", "top");
    TimeWarp::fetch = nullptr;
    StartServo(*hinge);
    CHECK(hinge->attachNodes.size() == 1);
    for (int policy = 0; policy < 2; ++policy) {
        physics::VesselPhysicsOptions opt;
        if (policy == 1) opt.isArticulation = [](const physics::PartJointSpec& j) { return ServoArticulation(j); };
        physics::PhysicsWorld world;
        physics::VesselPhysics vp(world, *v, physics::VesselPose{}, opt);
        INFO("policy " << policy);
        CHECK(vp.ClusterCount() == static_cast<size_t>(policy + 1));
        CHECK(vp.ConstraintCount() == static_cast<size_t>(policy));
    }
    hinge->OnDestroy();
}
