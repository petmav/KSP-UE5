// WP-52: ModuleRoboticRotationServo.cs (see include/ksp/serenity/servo_modules.hpp). Every function cites ModuleRoboticRotationServo.cs:<line> of
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

// :121
float ModuleRoboticRotationServo::JointTargetAngle() {
    const Vector2f soft = softMinMaxAngles();
    workingJointTargetAngle = Mathf::Clamp(targetAngle(), soft.x, soft.y);
    workingJointTargetAngle = mirrorRotation() ? F(0.0 - fx(workingJointTargetAngle)) : workingJointTargetAngle;
    if (inverted()) workingJointTargetAngle = F(0.0 - fx(workingJointTargetAngle));
    return workingJointTargetAngle;
}

// :148
void ModuleRoboticRotationServo::ToggleServoAction(KSPActionParam&) {
    const Vector2f soft = softMinMaxAngles();
    if (fx(targetAngle()) - fx(soft.x) < (fx(soft.y) - fx(soft.x)) / 2.0)
        SetMaximumAngle();
    else
        SetMinimumAngle();
}

// :160
void ModuleRoboticRotationServo::SetMinimumAngle() {
    if (!servoIsLocked()) {
        Fields()["targetAngle"]->SetValue(ModuleValue(softMinMaxAngles().x));
        ModifyServo(nullptr);
    } else {
        ServoEnvironment::PostScreenMessage("#autoLOC_8002356");
    }
}

// :173
void ModuleRoboticRotationServo::SetMaximumAngle() {
    if (!servoIsLocked()) {
        Fields()["targetAngle"]->SetValue(ModuleValue(softMinMaxAngles().y));
        ModifyServo(nullptr);
    } else {
        ServoEnvironment::PostScreenMessage("#autoLOC_8002356");
    }
}

// :186
void ModuleRoboticRotationServo::CheckSymmetry() {
    Part& p = PartRef(*this);
    ModuleRoboticRotationServo* other = SymmetryModule(*this);
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

// :212
void ModuleRoboticRotationServo::OnEditorPartEvent(ConstructionEventType evt, Part* p) {
    Part* parent = PartOf();
    while (parent != nullptr) {
        if (parent == p) return;
        parent = parent->parent;
    }
    Part& self = PartRef(*this);
    if (!self.symmetryCounterparts.empty() && (evt == ConstructionEventType::PartOffsetting || evt == ConstructionEventType::PartRotating)) {
        CheckSymmetry();
        OnVisualizeServo(false);
    }
}

// :235
void ModuleRoboticRotationServo::OnStart(StartState state) {
    Actions()["ResetPosition"]->SetGuiName(KSP::Localization::Localizer::Format("#autoLOC_6012043"));
    Events()["ResetPosition"]->SetGuiName(KSP::Localization::Localizer::Format("#autoLOC_6012043"));
    BaseServo::OnStart(state);
    configHardMinMaxLimits = hardMinMaxLimits();
    prevSoftMinMaxAngles = softMinMaxAngles();
    invertedSoftMinMaxAngles.x = F(0.0 - fx(softMinMaxAngles().x));
    invertedSoftMinMaxAngles.y = F(0.0 - fx(softMinMaxAngles().y));
    if (!ServoEnvironment::SceneIsFlight() && !ServoEnvironment::SceneIsEditor()) return;
    Part& p = PartRef(*this);
    if (checkSymmetry && !p.symmetryCounterparts.empty()) CheckSymmetry();
    checkSymmetry = false;
    if (ServoEnvironment::SceneIsEditor()) GE::onEditorPartEvent.Add(this, &ModuleRoboticRotationServo::OnEditorPartEvent);
    Fields()["servoMotorLimit"]->guiName = KSP::Localization::Localizer::Format("#autoLOC_8003238");
    targetAngleAxisField = dynamic_cast<BaseAxisField*>(Fields()["targetAngle"]);
    traverseVelocityAxisField = dynamic_cast<BaseAxisField*>(Fields()["traverseVelocity"]);
    Fields()["softMinMaxAngles"]->guiActiveEditor = !Get<bool>("hideUISoftMinMaxAngles");
    Fields()["traverseVelocity"]->guiActive = !Get<bool>("hideUITraverseVelocity");
    const bool showDamping = !Get<bool>("hideUIDamping");
    Fields()["hingeDamping"]->guiActiveEditor = showDamping;
    Fields()["hingeDamping"]->guiActive = showDamping;
    if (servoIsMotorized()) GE::onPartResourceNonemptyEmpty.Add(this, &ModuleRoboticRotationServo::OnResourceNonemptyEmpty);
    ModifyLimits(nullptr);
    ModifyLimitsMode(nullptr);
    ModifyTraverseLimits(nullptr);
    Fields()["allowFullRotation"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyLimitsMode(&v); });
    Fields()["softMinMaxAngles"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyLimits(&v); });
    Fields()["traverseVelocity"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyTraverseLimits(&v); });
    Fields()["targetAngle"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyTargetAngle(&v); });
    Fields()["hingeDamping"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyServo(&v); });
    Fields()["inverted"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyInverted(&v); });
    for (const std::string& n : SplitComma(pistonTransforms))
        if (std::shared_ptr<ServoNode> child = rig.Find(n)) child->active = false;
}

// :298
void ModuleRoboticRotationServo::OnDestroy() {
    BaseServo::OnDestroy();
    GE::onEditorPartEvent.Remove(this, &ModuleRoboticRotationServo::OnEditorPartEvent);
    GE::onPartResourceNonemptyEmpty.Remove(this, &ModuleRoboticRotationServo::OnResourceNonemptyEmpty);
}

// :316
void ModuleRoboticRotationServo::OnCopy(PartModule& fromModule) {
    if (ServoEnvironment::SceneIsEditor()) checkSymmetry = true;
    BaseServo::OnCopy(fromModule);
}

// :325
void ModuleRoboticRotationServo::OnJointInit(bool) {
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
        xDrive = servoJoint->xDrive;  // sic: the linear drive is read, the angular one is written
        xDrive.positionSpring = 0.0f;
        xDrive.positionDamper = 0.1f;
        xDrive.maximumForce = FLT_MAX;
        servoJoint->angularXDrive = xDrive;
        xSpringLimit = servoJoint->angularXLimitSpring;
        servoJoint->targetAngularVelocity = Vector3f::zero();
        SetJointHighLowLimits();
        servoJoint->targetRotation = Quaternionf::identity();
    }
}

// :371
void ModuleRoboticRotationServo::SetJointHighLowLimits() {
    if (servoJoint == nullptr) throw Nre();
    lowerLimit = servoJoint->lowAngularXLimit;
    upperLimit = servoJoint->highAngularXLimit;
    const Vector2f soft = softMinMaxAngles();
    const bool mirror = mirrorRotation();
    if (inverted()) {
        invertedSoftMinMaxAngles.x = mirror ? F(0.0 - fx(soft.y)) : F(0.0 - fx(soft.x));
        invertedSoftMinMaxAngles.y = mirror ? F(0.0 - fx(soft.x)) : F(0.0 - fx(soft.y));
        upperLimit.limit = mirror ? soft.x : soft.y;
        lowerLimit.limit = mirror ? F(0.0 - fx(soft.y)) : soft.x;
    } else {
        upperLimit.limit = F(fx(cachedStartingRotationOffset) + fx(mirror ? Mathf::Abs(soft.y) : Mathf::Abs(soft.x)));
        lowerLimit.limit = F(fx(cachedStartingRotationOffset) - (mirror ? (0.0 - fx(soft.x)) : fx(soft.y)));
    }
    servoJoint->highAngularXLimit = upperLimit;
    servoJoint->lowAngularXLimit = lowerLimit;
}

// :391
void ModuleRoboticRotationServo::OnPostStartJointInit() {
    if (ServoEnvironment::SceneIsFlight()) {
        SetTargetRotationLocal(*servoJoint, targetRotation, cachedStartingRotation);
        Set<float>("previousTargetAngle", currentTransformAngle());
        driveTargetAngle = currentTransformAngle();
        OnServoLockApplied();
        ModifyServo(nullptr);
        initComplete = true;
    }
}

// :404
void ModuleRoboticRotationServo::OnPreModifyServo() {
    const Vector2f soft = softMinMaxAngles();
    Set<float>("targetAngle", Mathf::Clamp(targetAngle(), soft.x, soft.y));
}

// :418
void ModuleRoboticRotationServo::OnVisualizeServo(bool rotateBase) {
    if (movingPartObject == nullptr) throw Nre();
    const float jointTargetAngle = JointTargetAngle();
    const float previousTargetAngle = Get<float>("previousTargetAngle");
    if (rotateBase) {
        Part& p = PartRef(*this);
        editorRotation = p.partTransform.rotation;
        const float num = F(fx(jointTargetAngle) - fx(previousTargetAngle));
        p.partTransform.rotation = SetTargetRotation(editorRotation, F(0.0 - fx(num)), false);
        editorRotation = movingPartObject->localRotation;
        movingPartObject->localRotation = SetTargetRotation(editorRotation, jointTargetAngle, true);
    } else {
        editorRotation = movingPartObject->localRotation;
        movingPartObject->localRotation = SetTargetRotation(editorRotation, jointTargetAngle, true);
    }
    if (previousTargetAngle != jointTargetAngle) Set<float>("previousTargetAngle", jointTargetAngle);
}

// :440
void ModuleRoboticRotationServo::OnModifyServo() {
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
    if (!allowFullRotation()) {
        xSpringLimit.spring = Get<float>("angularXLimitSpring");
        xSpringLimit.damper = Get<float>("angularXLimitDamper");
    } else {
        xSpringLimit.spring = 1E-16f;
        xSpringLimit.damper = 1E-16f;
    }
    servoJoint->angularXLimitSpring = xSpringLimit;
    xDrive = servoJoint->angularXDrive;
    if (!servoMotorIsEngaged() && !servoIsLocked()) {
        if (!lockPartOnPowerLoss()) {
            xDrive.positionSpring = 0.0f;
            xDrive.positionDamper = 1000.0f;
            xDrive.maximumForce = 1.0f;
        }
    } else {
        xDrive.positionSpring = F(fx(motorOutput) * fx(Get<float>("driveSpringMutliplier")));
        xDrive.positionDamper = F(fx(Get<float>("driveDampingMutliplier")) * fx(maxMotorOutput()) * (fx(servoMotorSize()) * fx(0.01f)) * (fx(hingeDamping()) * fx(0.01f)));
        xDrive.maximumForce = motorOutput;
    }
    servoJoint->angularXDrive = xDrive;
}

// :485
void ModuleRoboticRotationServo::ServoFixedUpdate() {
    const bool paused = ServoEnvironment::flightDriverPause && ServoEnvironment::flightDriverPause();
    if (!ServoEnvironment::SceneIsFlight() || paused) return;
    if (!servoIsLocked() && servoMotorIsEngaged()) {
        maxAnglePerFrame = F(fx(traverseVelocity()) * fx(Time::fixedDeltaTime));
        if (!FloatEquals(driveTargetAngle, JointTargetAngle())) {
            if (!allowFullRotation())
                driveTargetAngle = Mathf::MoveTowards(driveTargetAngle, JointTargetAngle(), maxAnglePerFrame);
            else
                driveTargetAngle = Mathf::MoveTowardsAngle(driveTargetAngle, JointTargetAngle(), maxAnglePerFrame);
        }
        if (movingPartObject == nullptr) throw Nre();
        targetRotation = SetTargetRotation(movingPartObject->localRotation, driveTargetAngle, true);
        SetTargetRotationLocal(*servoJoint, targetRotation, cachedStartingRotation);
    } else if (!HasEnoughResources() && lockPartOnPowerLoss() && !servoIsLocked()) {
        EngageServoLock();
    } else {
        if (movingPartObject == nullptr) throw Nre();
        targetRotation = SetTargetRotation(movingPartObject->localRotation, lockAngle, true);
        SetTargetRotationLocal(*servoJoint, targetRotation, cachedStartingRotation);
    }
    previousDisplacement = currentTransformAngle();
}

// :525
float ModuleRoboticRotationServo::GetFrameDisplacement() {
    angleDiff = F(fx(currentTransformAngle()) - fx(previousDisplacement));
    if (angleDiff >= 180.0f)
        angleDiff = F(fx(angleDiff) - 360.0);
    else if (angleDiff < -180.0f)
        angleDiff = F(fx(angleDiff) + 360.0);
    angleDiff = Mathf::Abs(angleDiff);
    return angleDiff;
}

// :545
void ModuleRoboticRotationServo::UpdatePAWUI(UI_Scene currentScene) {
    const float num = currentTransformAngle();
    Set<float>("currentAngle", num);
    if ((prevServoMotorIsEngaged == servoMotorIsEngaged() || currentScene != UI_Scene::Flight) &&
        (prevServoIsMotorized == servoIsMotorized() || currentScene != UI_Scene::Editor) && prevServoIsLocked == servoIsLocked()) {
        BaseServo::UpdatePAWUI(currentScene);
        return;
    }
    Fields()["softMinMaxAngles"]->guiActiveEditor = !allowFullRotation() && !Get<bool>("hideUIDamping");
    const bool showDamping = !Get<bool>("hideUIDamping");
    Fields()["hingeDamping"]->guiActiveEditor = showDamping;
    Fields()["hingeDamping"]->guiActive = showDamping;
    prevServoMotorIsEngaged = servoMotorIsEngaged();
    prevServoIsMotorized = servoIsMotorized();
    prevServoIsLocked = servoIsLocked();
    BaseServo::UpdatePAWUI(currentScene);
}

// :582
void ModuleRoboticRotationServo::OnServoMotorEngaged() {
    driveTargetAngle = currentTransformAngle();
    motorManualDisengaged = false;
}

// :593
void ModuleRoboticRotationServo::ResetLaunchPosition() {
    Fields()["targetAngle"]->SetValue(ModuleValue(launchPosition()));
    ModifyServo(nullptr);
}

// :599
void ModuleRoboticRotationServo::OnSaveShip() {
    SetLaunchPosition(currentAngle());
    BaseServo::OnSaveShip();
}

// :612
void ModuleRoboticRotationServo::ModifyTargetAngle(const ModuleValue* field) {
    const Vector2f soft = softMinMaxAngles();
    if (!allowFullRotation()) {
        Set<float>("targetAngle", Mathf::Clamp(targetAngle(), soft.x, soft.y));
    } else if (targetAngle() < -179.99f) {
        Set<float>("targetAngle", F(360.0 + fx(targetAngle())));
    } else if (targetAngle() > 180.0f) {
        Set<float>("targetAngle", F(fx(targetAngle()) - 360.0));
    }
    ModifyServo(field);
}

// :629
void ModuleRoboticRotationServo::ModifyLimitsMode(const ModuleValue* field) {
    if (!allowFullRotation()) {
        SetHardLimits("targetAngle", configHardMinMaxLimits);
        SetSoftLimits("targetAngle", prevSoftMinMaxAngles);
    } else {
        prevSoftMinMaxAngles = softMinMaxAngles();
        SetHardLimits("targetAngle", Vector2f(-179.99f, 180.0f));
        SetSoftLimits("targetAngle", Vector2f(-179.99f, 180.0f));
    }
    Fields()["softMinMaxAngles"]->guiActiveEditor = !allowFullRotation() && !Get<bool>("hideUIDamping");
    const bool active = !allowFullRotation();
    if (BaseAction* a = Actions()["MinimumAngleAction"]) a->active = active;
    if (BaseAction* a = Actions()["MaximumAngleAction"]) a->active = active;
    if (BaseAction* a = Actions()["ToggleServoAction"]) a->active = active;
    ModifyServo(field);
}

// :653
void ModuleRoboticRotationServo::ModifyLimits(const ModuleValue* field) {
    const Vector2f soft = softMinMaxAngles();
    if (targetAngleAxisField == nullptr) throw Nre();
    targetAngleAxisField->minValue = soft.x;
    targetAngleAxisField->maxValue = soft.y;
    Set<float>("targetAngle", Mathf::Clamp(targetAngle(), soft.x, soft.y));
    if (HasAxisFieldLimit("targetAngle")) {
        UpdateAxisFieldLimit("targetAngle", hardMinMaxLimits(), soft);
        if (LimitsChanged) LimitsChanged(GetAxisFieldLimit("targetAngle"));
    }
    if (ServoEnvironment::SceneIsFlight()) SetJointHighLowLimits();
    ModifyServo(field);
}

// :678
void ModuleRoboticRotationServo::ModifyInverted(const ModuleValue*) {
    if (ServoEnvironment::SceneIsFlight()) SetJointHighLowLimits();
}

// :686
void ModuleRoboticRotationServo::ModifyTraverseLimits(const ModuleValue* field) {
    const Vector2f tvl = traverseVelocityLimits();
    if (traverseVelocityAxisField == nullptr) throw Nre();
    traverseVelocityAxisField->minValue = tvl.x;
    traverseVelocityAxisField->maxValue = tvl.y;
    Set<float>("traverseVelocity", Mathf::Clamp(traverseVelocity(), tvl.x, tvl.y));
    currentVelocityLimit = traverseVelocity();
    ModifyServo(field);
}

// :700
Quaternionf ModuleRoboticRotationServo::SetTargetRotation(const Quaternionf& startingRotation, float rotationAngle, bool setRotation) {
    const Quaternionf quaternion = startingRotation;
    if (setRotation) return UnityNative::AngleAxis(rotationAngle, GetMainAxis());
    return quaternion * UnityNative::AngleAxis(rotationAngle, GetMainAxis());
}

// :710
void ModuleRoboticRotationServo::OnResourceNonemptyEmpty(PartResource* resource) {
    if (!ResourceEventApplies(*this, resource)) return;
    for (size_t i = 0; i < resHandler.inputResources.size(); i++) {
        if (ResourceIsAnInput(*this, *resource, i)) {
            if (movingPartObject == nullptr) throw Nre();
            targetRotation = SetTargetRotation(movingPartObject->localRotation, currentTransformAngle(), true);
            SetTargetRotationLocal(*servoJoint, targetRotation, cachedStartingRotation);
        }
    }
}

// :726
void ModuleRoboticRotationServo::InitAxisFieldLimits() {
    AxisFieldLimit l;
    l.limitedField = targetAngleAxisField;
    l.softLimits = softMinMaxAngles();
    l.hardLimits = hardMinMaxLimits();
    AddAxisFieldLimit("targetAngle", l);
}

// :736
void ModuleRoboticRotationServo::UpdateAxisFieldHardLimit(const std::string& fieldName, const Vector2f& newlimits) {
    if (fieldName == "targetAngle") {
        Set<Vector2f>("hardMinMaxLimits", newlimits);
        ModifyLimits(nullptr);
    }
}

// :745
void ModuleRoboticRotationServo::UpdateAxisFieldSoftLimit(const std::string& fieldName, const Vector2f& newlimits) {
    if (fieldName == "targetAngle") {
        Set<Vector2f>("softMinMaxAngles", newlimits);
        invertedSoftMinMaxAngles.x = F(0.0 - fx(newlimits.x));
        invertedSoftMinMaxAngles.y = F(0.0 - fx(newlimits.y));
        ModifyLimits(nullptr);
    }
}

}  // namespace ksp::Expansions::Serenity
