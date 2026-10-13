// WP-52: ModuleRoboticServoHinge.cs (see include/ksp/serenity/servo_modules.hpp). Every function cites ModuleRoboticServoHinge.cs:<line> of
// decompiled/KSP-Decompiled/Assembly-CSharp/Expansions/Serenity/.
#include <algorithm>
#include <cfloat>

#include "ksp/editor/editor_types.hpp"
#include "ksp/save/save_enums.hpp"
#include "ksp/serenity/servo_modules.hpp"
#include "servo_internal.hpp"

namespace ksp::Expansions::Serenity {

using namespace detail;
using GE = GameEvents;

namespace {
std::vector<std::string> SplitComma(const std::string& s) {
    std::vector<std::string> out;
    size_t begin = 0;
    while (true) {
        const size_t comma = s.find(',', begin);
        out.push_back(s.substr(begin, comma == std::string::npos ? std::string::npos : comma - begin));
        if (comma == std::string::npos) break;
        begin = comma + 1;
    }
    return out;
}
}  // namespace

// ModuleRoboticServoHinge.cs:126
float ModuleRoboticServoHinge::JointTargetAngle() {
    const Vector2f soft = softMinMaxAngles();
    workingJointTargetAngle = Mathf::Clamp(targetAngle(), soft.x, soft.y);
    workingJointTargetAngle = mirrorRotation() ? F(0.0 - fx(workingJointTargetAngle)) : workingJointTargetAngle;
    workingJointTargetAngle = F(fx(workingJointTargetAngle) - fx(modelInitialAngle()));
    return workingJointTargetAngle;
}

// ModuleRoboticServoHinge.cs:152
void ModuleRoboticServoHinge::ToggleHingeAction(KSPActionParam&) {
    const Vector2f soft = softMinMaxAngles();
    if (fx(targetAngle()) - fx(soft.x) < (fx(soft.y) - fx(soft.x)) / 2.0)
        SetMaximumAngle();
    else
        SetMinimumAngle();
}

// ModuleRoboticServoHinge.cs:164
void ModuleRoboticServoHinge::SetMinimumAngle() {
    if (!servoIsLocked()) {
        Fields()["targetAngle"]->SetValue(ModuleValue(softMinMaxAngles().x));
        ModifyServo(nullptr);
    } else {
        ServoEnvironment::PostScreenMessage("#autoLOC_8002356");
    }
}

// ModuleRoboticServoHinge.cs:177
void ModuleRoboticServoHinge::SetMaximumAngle() {
    if (!servoIsLocked()) {
        Fields()["targetAngle"]->SetValue(ModuleValue(softMinMaxAngles().y));
        ModifyServo(nullptr);
    } else {
        ServoEnvironment::PostScreenMessage("#autoLOC_8002356");
    }
}

// ModuleRoboticServoHinge.cs:190
void ModuleRoboticServoHinge::CheckSymmetry() {
    Part& p = PartRef(*this);
    ModuleRoboticServoHinge* other = SymmetryModule(*this);
    if (other == nullptr) throw Nre();
    if (static_cast<SymmetryMethod>(p.fields.symMethod) == SymmetryMethod::Mirror) {
        const Vector3f vector = rig.modulePosition - other->rig.modulePosition;
        if (other->jointParent == nullptr || jointParent == nullptr) throw Nre();
        Vector3f lhs = other->jointParent->TransformDirection(other->GetMainAxis());
        const Vector3f rhs = jointParent->TransformDirection(GetMainAxis());
        const float s = F(2.0 * (fx(Vector3f::Dot(lhs, vector)) / fx(Vector3f::Dot(vector, vector))));
        lhs = lhs - s * vector;
        if (Vector3f::Dot(lhs, rhs) > 0.0f)
            Set<bool>("mirrorRotation", !other->mirrorRotation());
        else
            Set<bool>("mirrorRotation", other->mirrorRotation());
    } else {
        Set<bool>("mirrorRotation", other->mirrorRotation());
    }
}

// ModuleRoboticServoHinge.cs:216
void ModuleRoboticServoHinge::OnEditorPartEvent(ConstructionEventType evt, Part* p) {
    Part* parent = PartOf();
    while (parent != nullptr) {
        if (parent == p) return;  // `p` is this part or one of its ancestors: nothing to do
        parent = parent->parent;
    }
    Part& self = PartRef(*this);
    if (!self.symmetryCounterparts.empty() &&
        (evt == ConstructionEventType::PartAttached || evt == ConstructionEventType::PartOffsetting || evt == ConstructionEventType::PartRotating)) {
        CheckSymmetry();
        OnVisualizeServo(mirrorRotation() ? true : false);
    }
}

// ModuleRoboticServoHinge.cs:239
void ModuleRoboticServoHinge::OnStart(StartState state) {
    Actions()["ResetPosition"]->SetGuiName(KSP::Localization::Localizer::Format("#autoLOC_6012043"));
    Events()["ResetPosition"]->SetGuiName(KSP::Localization::Localizer::Format("#autoLOC_6012043"));
    BaseServo::OnStart(state);
    if (!ServoEnvironment::SceneIsFlight() && !ServoEnvironment::SceneIsEditor()) return;
    Part& p = PartRef(*this);
    if (checkSymmetry && !p.symmetryCounterparts.empty()) CheckSymmetry();
    checkSymmetry = false;
    if (ServoEnvironment::SceneIsEditor()) GE::onEditorPartEvent.Add(this, &ModuleRoboticServoHinge::OnEditorPartEvent);
    Fields()["servoMotorLimit"]->guiName = KSP::Localization::Localizer::Format("#autoLOC_8003238");
    targetAngleAxisField = dynamic_cast<BaseAxisField*>(Fields()["targetAngle"]);
    traverseVelocityAxisField = dynamic_cast<BaseAxisField*>(Fields()["traverseVelocity"]);
    Fields()["softMinMaxAngles"]->guiActiveEditor = !Get<bool>("hideUISoftMinMaxAngles");
    Fields()["traverseVelocity"]->guiActive = !Get<bool>("hideUITraverseVelocity");
    const bool showDamping = !Get<bool>("hideUIDamping");
    Fields()["hingeDamping"]->guiActiveEditor = showDamping;
    Fields()["hingeDamping"]->guiActive = showDamping;
    if (servoIsMotorized()) GE::onPartResourceNonemptyEmpty.Add(this, &ModuleRoboticServoHinge::OnResourceNonemptyEmpty);
    ModifyLimits(nullptr);
    ModifyTraverseLimits(nullptr);
    Fields()["softMinMaxAngles"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyLimits(&v); });
    Fields()["traverseVelocity"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyTraverseLimits(&v); });
    Fields()["targetAngle"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyServo(&v); });
    Fields()["hingeDamping"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyServo(&v); });
    TogglePistons(false);
    lastDragCubeAngle = FLT_MAX;
}

// ModuleRoboticServoHinge.cs:287
void ModuleRoboticServoHinge::TogglePistons(bool active) {
    const std::vector<std::string> names = SplitComma(pistonTransforms);
    for (const std::string& n : names) {
        if (std::shared_ptr<ServoNode> child = rig.Find(n)) {
            child->active = active;
            togglePistonsLog.push_back(n + (active ? "=1" : "=0"));
        }
    }
}

// ModuleRoboticServoHinge.cs:300
void ModuleRoboticServoHinge::OnInventoryModeDisable() {
    PartModule::OnInventoryModeDisable();
    TogglePistons(false);
}

// ModuleRoboticServoHinge.cs:306
void ModuleRoboticServoHinge::OnInventoryModeEnable() {
    PartModule::OnInventoryModeEnable();
    TogglePistons(false);
}

// ModuleRoboticServoHinge.cs:312
void ModuleRoboticServoHinge::OnDestroy() {
    BaseServo::OnDestroy();
    GE::onEditorPartEvent.Remove(this, &ModuleRoboticServoHinge::OnEditorPartEvent);
    GE::onPartResourceNonemptyEmpty.Remove(this, &ModuleRoboticServoHinge::OnResourceNonemptyEmpty);
}

// ModuleRoboticServoHinge.cs:328
void ModuleRoboticServoHinge::OnCopy(PartModule& fromModule) {
    if (ServoEnvironment::SceneIsEditor()) checkSymmetry = true;
    BaseServo::OnCopy(fromModule);
}

// ModuleRoboticServoHinge.cs:337
void ModuleRoboticServoHinge::OnJointInit(bool) {
    const std::string servoTransformName = Get<std::string>("servoTransformName");
    const std::string baseTransformName = Get<std::string>("baseTransformName");
    if (movingPartObject == nullptr && !servoTransformName.empty()) movingPartObject = rig.Find(servoTransformName);
    if (basePartObject == nullptr && !baseTransformName.empty()) basePartObject = rig.Find(baseTransformName);
    if (!ServoEnvironment::SceneIsFlight()) {
        if (movingPartObject != nullptr) CreateServoRigidbody();
    } else if (movingPartObject != nullptr) {
        CreateServoRigidbody();
        servoJoint = std::make_shared<ConfigurableJointData>();
        servoJoint->axis = GetMainAxis();
        if (Vector3Equal(servoJoint->axis, Vector3f::up())) servoJoint->secondaryAxis = Vector3f::right();
        axis = servoJoint->axis;
        secAxis = servoJoint->secondaryAxis;
        servoJoint->xMotion = ConfigurableJointMotion::Locked;
        servoJoint->yMotion = ConfigurableJointMotion::Locked;
        servoJoint->zMotion = ConfigurableJointMotion::Locked;
        servoJoint->angularXMotion = ConfigurableJointMotion::Limited;
        servoJoint->angularYMotion = ConfigurableJointMotion::Locked;
        servoJoint->angularZMotion = ConfigurableJointMotion::Locked;
        xDrive = servoJoint->angularXDrive;
        xDrive.positionSpring = 0.0f;
        xDrive.positionDamper = 0.1f;
        xDrive.maximumForce = FLT_MAX;
        servoJoint->angularXDrive = xDrive;
        xSpringLimit = servoJoint->angularXLimitSpring;
        servoJoint->targetAngularVelocity = Vector3f::zero();
        lowerLimit = servoJoint->lowAngularXLimit;
        upperLimit = servoJoint->highAngularXLimit;
        const Vector2f soft = softMinMaxAngles();
        upperLimit.limit = F(fx(cachedStartingRotationOffset) + fx(Mathf::Abs(F(fx(soft.x) - fx(modelInitialAngle())))));
        lowerLimit.limit = F(fx(cachedStartingRotationOffset) - (fx(soft.y) - fx(modelInitialAngle())));
        servoJoint->highAngularXLimit = upperLimit;
        servoJoint->lowAngularXLimit = lowerLimit;
        servoJoint->targetRotation = Quaternionf::identity();
    }
}

// ModuleRoboticServoHinge.cs:388
void ModuleRoboticServoHinge::OnPostStartJointInit() {
    if (ServoEnvironment::SceneIsFlight()) {
        SetTargetRotationLocal(*servoJoint, targetRotation, cachedStartingRotation);
        Set<float>("previousTargetAngle", F(fx(modelInitialAngle()) - fx(currentTransformAngle())));
        driveTargetAngle = currentTransformAngle();
        OnServoLockApplied();
        ModifyLimits(nullptr);
        initComplete = true;
    }
}

// ModuleRoboticServoHinge.cs:401
void ModuleRoboticServoHinge::OnPreModifyServo() {
    const Vector2f soft = softMinMaxAngles();
    Set<float>("targetAngle", Mathf::Clamp(targetAngle(), soft.x, soft.y));
}

// ModuleRoboticServoHinge.cs:415
void ModuleRoboticServoHinge::OnVisualizeServo(bool rotateBase) {
    if (movingPartObject == nullptr) throw Nre();
    const float jointTargetAngle = JointTargetAngle();
    if (rotateBase) {
        Part& p = PartRef(*this);
        editorRotation = p.partTransform.rotation;
        const float num = F(fx(jointTargetAngle) - fx(previousTargetAngle()));
        p.partTransform.rotation = SetTargetRotation(editorRotation, F(0.0 - fx(num)), false);
        editorRotation = movingPartObject->localRotation;
        movingPartObject->localRotation = SetTargetRotation(editorRotation, jointTargetAngle, true);
    } else {
        editorRotation = movingPartObject->localRotation;
        movingPartObject->localRotation = SetTargetRotation(editorRotation, jointTargetAngle, true);
    }
    if (previousTargetAngle() != jointTargetAngle) Set<float>("previousTargetAngle", jointTargetAngle);
}

// ModuleRoboticServoHinge.cs:437
void ModuleRoboticServoHinge::OnModifyServo() {
    if (HighWarp()) {
        xDrive = servoJoint->xDrive;
        xDrive.positionSpring = 0.0f;
        xDrive.positionDamper = 0.0f;
        xDrive.maximumForce = 0.0f;
        servoJoint->angularXDrive = xDrive;
        return;
    }
    if ((!servoMotorIsEngaged() && !lockPartOnPowerLoss()) || !servoIsMotorized()) {
        if (movingPartObject == nullptr) throw Nre();
        SetTargetRotationLocal(*servoJoint, movingPartObject->localRotation, cachedStartingRotation);
    }
    xSpringLimit.spring = Get<float>("angularXLimitSpring");
    xSpringLimit.damper = Get<float>("angularXLimitDamper");
    servoJoint->angularXLimitSpring = xSpringLimit;
    xDrive = servoJoint->angularXDrive;
    if (!servoMotorIsEngaged() && !servoIsLocked()) {
        if (!lockPartOnPowerLoss()) {
            xDrive.positionSpring = 0.0f;
            xDrive.positionDamper = 0.0f;
            xDrive.maximumForce = 0.0f;
        }
    } else {
        xDrive.positionSpring = F(fx(motorOutput) * fx(Get<float>("driveSpringMutliplier")));
        xDrive.positionDamper = F(fx(Get<float>("driveDampingMutliplier")) * fx(maxMotorOutput()) * (fx(servoMotorSize()) * fx(0.01f)) * (fx(hingeDamping()) * fx(0.01f)));
        xDrive.maximumForce = motorOutput;
    }
    servoJoint->angularXDrive = xDrive;
}

// ModuleRoboticServoHinge.cs:474
void ModuleRoboticServoHinge::ResetLaunchPosition() {
    Fields()["targetAngle"]->SetValue(ModuleValue(launchPosition()));
    ModifyServo(nullptr);
}

// ModuleRoboticServoHinge.cs:480
void ModuleRoboticServoHinge::ServoFixedUpdate() {
    const bool paused = ServoEnvironment::flightDriverPause && ServoEnvironment::flightDriverPause();
    if (!ServoEnvironment::SceneIsFlight() || paused) return;
    if (servoMotorIsEngaged()) {
        if (!servoIsLocked()) {
            maxAnglePerFrame = F(fx(traverseVelocity()) * fx(Time::fixedDeltaTime));
            const float jta = JointTargetAngle();
            if (!FloatEquals(driveTargetAngle, jta)) driveTargetAngle = Mathf::MoveTowards(driveTargetAngle, JointTargetAngle(), maxAnglePerFrame);
            if (movingPartObject == nullptr) throw Nre();
            targetRotation = SetTargetRotation(movingPartObject->localRotation, driveTargetAngle, true);
            SetTargetRotationLocal(*servoJoint, targetRotation, cachedStartingRotation);
        }
    } else {
        if (!HasEnoughResources() && lockPartOnPowerLoss() && !servoIsLocked()) EngageServoLock();
        if (servoIsLocked()) driveTargetAngle = lockAngle;
    }
    previousDisplacement = currentTransformAngle();
    if (useMultipleDragCubes()) SetDragCubes(currentTransformAngle());
}

// ModuleRoboticServoHinge.cs:522
float ModuleRoboticServoHinge::GetFrameDisplacement() {
    angleDiff = Mathf::Abs(F(fx(currentTransformAngle()) - fx(previousDisplacement)));
    if (angleDiff < displacementLimitIgnoreFrameDistance) {
        const Vector2f soft = softMinMaxAngles();
        lowerDisplacementLimit = F(fx(soft.x) + fx(displacementLimitIgnoreRange));
        upperDisplacementLimit = F(fx(soft.y) - fx(displacementLimitIgnoreRange));
        if (targetAngle() < lowerDisplacementLimit && currentAngle() < lowerDisplacementLimit)
            angleDiff = 0.0f;
        else if (targetAngle() > upperDisplacementLimit && currentAngle() > upperDisplacementLimit)
            angleDiff = 0.0f;
    }
    return angleDiff;
}

// ModuleRoboticServoHinge.cs:546 (the part-action-window controls are UI; the state writes stay)
void ModuleRoboticServoHinge::UpdatePAWUI(UI_Scene currentScene) {
    const float num = F(fx(currentTransformAngle()) + fx(modelInitialAngle()));
    Set<float>("currentAngle", num);
    if ((prevServoMotorIsEngaged == servoMotorIsEngaged() || currentScene != UI_Scene::Flight) &&
        (prevServoIsMotorized == servoIsMotorized() || currentScene != UI_Scene::Editor) && prevServoIsLocked == servoIsLocked()) {
        BaseServo::UpdatePAWUI(currentScene);
        return;
    }
    Fields()["softMinMaxAngles"]->guiActiveEditor = !Get<bool>("hideUISoftMinMaxAngles");
    const bool showDamping = !Get<bool>("hideUIDamping");
    Fields()["hingeDamping"]->guiActiveEditor = showDamping;
    Fields()["hingeDamping"]->guiActive = showDamping;
    prevServoMotorIsEngaged = servoMotorIsEngaged();
    prevServoIsMotorized = servoIsMotorized();
    prevServoIsLocked = servoIsLocked();
    BaseServo::UpdatePAWUI(currentScene);
}

// ModuleRoboticServoHinge.cs:573
void ModuleRoboticServoHinge::OnServoLockApplied() { lockAngle = F(fx(currentTransformAngle()) + fx(modelInitialAngle())); }

// ModuleRoboticServoHinge.cs:578
void ModuleRoboticServoHinge::OnServoLockRemoved() { driveTargetAngle = currentTransformAngle(); }

// ModuleRoboticServoHinge.cs:583
void ModuleRoboticServoHinge::OnServoMotorEngaged() {
    driveTargetAngle = currentTransformAngle();
    motorManualDisengaged = false;
}

// ModuleRoboticServoHinge.cs:594
void ModuleRoboticServoHinge::OnSaveShip() {
    SetLaunchPosition(targetAngle());
    BaseServo::OnSaveShip();
}

// ModuleRoboticServoHinge.cs:607
void ModuleRoboticServoHinge::ModifyLimits(const ModuleValue* field) {
    const Vector2f soft = softMinMaxAngles();
    if (targetAngleAxisField == nullptr) throw Nre();
    targetAngleAxisField->minValue = soft.x;
    targetAngleAxisField->maxValue = soft.y;
    Set<float>("targetAngle", Mathf::Clamp(targetAngle(), soft.x, soft.y));
    if (HasAxisFieldLimit("targetAngle")) {
        UpdateAxisFieldLimit("targetAngle", hardMinMaxLimits(), soft);
        if (LimitsChanged) LimitsChanged(GetAxisFieldLimit("targetAngle"));
    }
    ModifyServo(field);
}

// ModuleRoboticServoHinge.cs:628
void ModuleRoboticServoHinge::ModifyTraverseLimits(const ModuleValue* field) {
    const Vector2f tvl = traverseVelocityLimits();
    if (traverseVelocityAxisField == nullptr) throw Nre();
    traverseVelocityAxisField->minValue = tvl.x;
    traverseVelocityAxisField->maxValue = tvl.y;
    Set<float>("traverseVelocity", Mathf::Clamp(traverseVelocity(), tvl.x, tvl.y));
    currentVelocityLimit = traverseVelocity();
    ModifyServo(field);
}

// ModuleRoboticServoHinge.cs:642
Quaternionf ModuleRoboticServoHinge::SetTargetRotation(const Quaternionf& startingRotation, float rotationAngle, bool setRotation) {
    const Quaternionf quaternion = startingRotation;
    if (setRotation) return UnityNative::AngleAxis(rotationAngle, GetMainAxis());
    return quaternion * UnityNative::AngleAxis(rotationAngle, GetMainAxis());
}

// ModuleRoboticServoHinge.cs:652
void ModuleRoboticServoHinge::OnResourceNonemptyEmpty(PartResource* resource) {
    if (!ResourceEventApplies(*this, resource)) return;
    for (size_t i = 0; i < resHandler.inputResources.size(); i++) {
        if (ResourceIsAnInput(*this, *resource, i)) {
            powerLossLockAngle = currentTransformAngle();
            if (movingPartObject == nullptr) throw Nre();
            targetRotation = SetTargetRotation(movingPartObject->localRotation, powerLossLockAngle, true);
            SetTargetRotationLocal(*servoJoint, targetRotation, cachedStartingRotation);
        }
    }
}

// ModuleRoboticServoHinge.cs:669 (limitedField is the axis field the module has found by now: null in OnAwake, as in C#)
void ModuleRoboticServoHinge::InitAxisFieldLimits() {
    AxisFieldLimit l;
    l.limitedField = targetAngleAxisField;
    l.softLimits = softMinMaxAngles();
    l.hardLimits = hardMinMaxLimits();
    AddAxisFieldLimit("targetAngle", l);
}

// ModuleRoboticServoHinge.cs:679
void ModuleRoboticServoHinge::UpdateAxisFieldHardLimit(const std::string& fieldName, const Vector2f& newlimits) {
    if (fieldName == "targetAngle") {
        Set<Vector2f>("hardMinMaxLimits", newlimits);
        ModifyLimits(nullptr);
    }
}

// ModuleRoboticServoHinge.cs:688
void ModuleRoboticServoHinge::UpdateAxisFieldSoftLimit(const std::string& fieldName, const Vector2f& newlimits) {
    if (fieldName == "targetAngle") {
        Set<Vector2f>("softMinMaxAngles", newlimits);
        ModifyLimits(nullptr);
    }
}

// ModuleRoboticServoHinge.cs:697
std::vector<std::string> ModuleRoboticServoHinge::GetDragCubeNames() const {
    if (useMultipleDragCubes()) return {"0", "50", "100"};
    return {};
}

// ModuleRoboticServoHinge.cs:719
void ModuleRoboticServoHinge::SetDragCubes(float angle) {
    if (!FloatEquals(angle, lastDragCubeAngle)) {
        float num = angle;
        if (angle < -90.0f)
            num = F(-180.0 - fx(angle));
        else if (angle > 90.0f)
            num = F(180.0 - fx(angle));
        const float weight = Mathf::Max(F((0.0 - fx(num)) * fx(0.01f)), 0.0f);
        const float weight2 = F(1.0 - fx(Mathf::Max(F(fx(Mathf::Abs(F(0.0 - fx(num)))) * fx(0.01f)), 0.0f)));
        const float weight3 = Mathf::Max(F(fx(num) * fx(0.01f)), 0.0f);
        Part& p = PartRef(*this);
        p.DragCubes.SetCubeWeight("0", weight);
        p.DragCubes.SetCubeWeight("50", weight2);
        p.DragCubes.SetCubeWeight("100", weight3);
        lastDragCubeAngle = angle;
    }
}

}  // namespace ksp::Expansions::Serenity
