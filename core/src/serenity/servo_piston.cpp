// WP-52: ModuleRoboticServoPiston.cs (see include/ksp/serenity/servo_modules.hpp). Every function cites ModuleRoboticServoPiston.cs:<line> of
// decompiled/KSP-Decompiled/Assembly-CSharp/Expansions/Serenity/.
#include <algorithm>
#include <cfloat>

#include "ksp/config/parse_extensions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/serenity/servo_modules.hpp"
#include "servo_internal.hpp"

namespace ksp::Expansions::Serenity {

using namespace detail;
using GE = GameEvents;

// ModuleRoboticServoPiston.cs:99
void ModuleRoboticServoPiston::OnStart(StartState state) {
    Actions()["ResetPosition"]->SetGuiName(KSP::Localization::Localizer::Format("#autoLOC_6012045"));
    Events()["ResetPosition"]->SetGuiName(KSP::Localization::Localizer::Format("#autoLOC_6012045"));
    // ModuleRoboticServoPiston.cs:103-:112 the soft-range control exists (UI_MinMaxRange on softMinMaxExtension): its limits clamp the stored soft range
    {
        const Vector2f hard = hardMinMaxLimits();
        const float step = F((fx(hard.y) - fx(hard.x)) / 100.0);
        const float minValueX = hard.x;
        const float minValueY = F(fx(hard.x) + fx(step));
        const float maxValueX = F(fx(hard.y) - fx(step));
        const float maxValueY = hard.y;
        Vector2f soft = softMinMaxExtension();
        soft.x = Mathf::Clamp(soft.x, minValueX, maxValueX);
        soft.y = Mathf::Clamp(soft.y, minValueY, maxValueY);
        Set<Vector2f>("softMinMaxExtension", soft);
    }
    targetExtensionAxisField = dynamic_cast<BaseAxisField*>(Fields()["targetExtension"]);
    if (targetExtensionAxisField != nullptr) {
        const Vector2f soft = softMinMaxExtension();
        const Vector2f hard = hardMinMaxLimits();
        targetExtensionAxisField->minValue = soft.x;
        targetExtensionAxisField->maxValue = soft.y;
        targetExtensionAxisField->incrementalSpeed = F((fx(hard.y) - fx(hard.x)) * fx(0.2f));
    }
    {
        const Vector2f soft = softMinMaxExtension();
        Set<float>("targetExtension", Mathf::Min(Mathf::Max(soft.x, targetExtension()), soft.y));
    }
    targetPosition = targetExtension();
    previousTargetPosition = targetPosition;
    driveTargetPosition = targetPosition;
    BaseServo::OnStart(state);
    UpdateTraverseVelocity(nullptr);
    UpdateTargetPosition(nullptr);
    if (servoIsMotorized()) GE::onPartResourceNonemptyEmpty.Add(this, &ModuleRoboticServoPiston::OnResourceNonemptyEmpty);
    Fields()["servoMotorLimit"]->guiName = KSP::Localization::Localizer::Format("#autoLOC_8003239");
    Fields()["softMinMaxExtension"]->guiActiveEditor = !Get<bool>("hideUISoftMinMaxExtension");
    const bool showVelocity = !Get<bool>("hideUITraverseVelocity");
    Fields()["traverseVelocity"]->guiActiveEditor = showVelocity;
    Fields()["traverseVelocity"]->guiActive = showVelocity;
    const bool showDamping = !Get<bool>("hideUIDamping");
    Fields()["pistonDamping"]->guiActiveEditor = showDamping;
    Fields()["pistonDamping"]->guiActive = showDamping;
    Fields()["softMinMaxExtension"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyLimits(&v); });
    Fields()["targetExtension"]->OnValueModified.push_back([this](const ModuleValue& v) { UpdateTargetPosition(&v); });
    Fields()["pistonDamping"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyServo(&v); });
    Fields()["traverseVelocity"]->OnValueModified.push_back([this](const ModuleValue& v) { UpdateTraverseVelocity(&v); });
    OutputUnit = KSP::Localization::Localizer::Format("#autoLOC_6013038");
    const std::string slaveTransformNames = Get<std::string>("slaveTransformNames");
    if (slaveTransformNames.empty()) return;
    if (movingPartObject == nullptr) throw Nre();
    const std::vector<std::string> array = ParseExtensions::ParseArray(slaveTransformNames);
    if (array.empty()) return;
    slaveTransforms.assign(array.size(), nullptr);
    slaveTransformsSet = true;
    for (size_t num = 0; num < array.size(); num++) {
        slaveTransforms[num] = rig.Find(array[num]);  // movingPartObject.transform.parent.Find(name)
        if (slaveTransforms[num] == nullptr) {
            log::Error("[ModuleRoboticServoPiston] No slave transform named " + array[num]);
            slaveTransforms.clear();
            slaveTransformsSet = false;
            return;
        }
    }
}

// ModuleRoboticServoPiston.cs:190
void ModuleRoboticServoPiston::OnDestroy() {
    BaseServo::OnDestroy();
    GE::onPartResourceNonemptyEmpty.Remove(this, &ModuleRoboticServoPiston::OnResourceNonemptyEmpty);
}

// ModuleRoboticServoPiston.cs:205
void ModuleRoboticServoPiston::ModifyLimits(const ModuleValue* field) {
    const Vector2f soft0 = softMinMaxExtension();
    if (auto* baseAxisField = dynamic_cast<BaseAxisField*>(Fields()["TargetExtension"])) {  // sic: the C# looks up "TargetExtension", which no field has
        baseAxisField->minValue = soft0.x;
        baseAxisField->maxValue = soft0.y;
    }
    Set<float>("targetExtension", Mathf::Min(Mathf::Max(soft0.x, targetExtension()), soft0.y));
    if (HasAxisFieldLimit("targetExtension")) {
        UpdateAxisFieldLimit("targetExtension", hardMinMaxLimits(), soft0);
        if (LimitsChanged) LimitsChanged(GetAxisFieldLimit("targetExtension"));
    }
    UpdateTargetPosition(field);
    if (ServoEnvironment::SceneIsFlight()) {
        const Vector2f soft = softMinMaxExtension();
        const Vector3f vector = CalcTargetPosition(Vector3f::zero(), soft.x, false);
        const Vector3f vector2 = CalcTargetPosition(Vector3f::zero(), soft.y, false);
        const Vector3f vector3 = (vector + vector2) / 2.0f;
        if (servoJoint == nullptr) throw Nre();
        servoJoint->autoConfigureConnectedAnchor = false;
        servoJoint->connectedAnchor = MatrixMultiplyDirection(servoParentTransform, vector3);
        linearLimit.limit = F((fx(soft.y) - fx(soft.x)) * fx(1.01f) / 2.0);  // (a - b) * 1.01f / 2f: one double expression, stored once
        servoJoint->linearLimit = linearLimit;
    }
    ModifyServo(field);
}

// ModuleRoboticServoPiston.cs:241
void ModuleRoboticServoPiston::UpdateTargetPosition(const ModuleValue* field) {
    const Vector2f soft = softMinMaxExtension();
    Set<float>("targetExtension", Mathf::Min(Mathf::Max(soft.x, targetExtension()), soft.y));
    targetPosition = targetExtension();
    ModifyServo(field);
}

// ModuleRoboticServoPiston.cs:248
void ModuleRoboticServoPiston::UpdateTraverseVelocity(const ModuleValue* field) {
    currentVelocityLimit = traverseVelocity();
    ModifyServo(field);
}

// ModuleRoboticServoPiston.cs:254
void ModuleRoboticServoPiston::OnJointInit(bool) {
    const std::string servoTransformName = Get<std::string>("servoTransformName");
    const std::string baseTransformName = Get<std::string>("baseTransformName");
    if (movingPartObject == nullptr && !servoTransformName.empty()) movingPartObject = rig.Find(servoTransformName);
    if (basePartObject == nullptr && !baseTransformName.empty()) basePartObject = rig.Find(baseTransformName);
    if (!ServoEnvironment::SceneIsFlight()) {
        if (movingPartObject != nullptr) CreateServoRigidbody();
    } else if (movingPartObject != nullptr) {
        CreateServoRigidbody();
        servoJoint = std::make_shared<ConfigurableJointData>();
        servoJoint->anchor = Vector3f::zero();
        const Quaternionf localRotation = movingPartObject->localRotation;
        servoJoint->axis = UnityNative::Inverse(localRotation) * GetMainAxis();
        if (Vector3Equal(servoJoint->axis, Vector3f::up())) servoJoint->secondaryAxis = Vector3f::right();
        axis = servoJoint->axis;
        secAxis = servoJoint->secondaryAxis;
        servoJoint->xMotion = ConfigurableJointMotion::Limited;
        servoJoint->yMotion = ConfigurableJointMotion::Locked;
        servoJoint->zMotion = ConfigurableJointMotion::Locked;
        servoJoint->angularXMotion = ConfigurableJointMotion::Locked;
        servoJoint->angularYMotion = ConfigurableJointMotion::Locked;
        servoJoint->angularZMotion = ConfigurableJointMotion::Locked;
        linearLimitSpring = servoJoint->linearLimitSpring;
        servoJoint->targetVelocity = Vector3f::zero();
        linearLimit = servoJoint->linearLimit;
        linearLimit.limit = 0.0f;
        servoJoint->linearLimit = linearLimit;
    }
}

// ModuleRoboticServoPiston.cs:298
void ModuleRoboticServoPiston::OnPostStartJointInit() {
    if (ServoEnvironment::SceneIsFlight()) {
        initComplete = true;
        ModifyLimits(nullptr);
        if (servoIsLocked()) {
            Set<float>("currentExtension", targetPosition);
            lockAngle = currentExtension();
            SetJointTargetPosition();
        }
    }
}

// ModuleRoboticServoPiston.cs:317
void ModuleRoboticServoPiston::OnVisualizeServo(bool moveBase) {
    if (movingPartObject == nullptr) throw Nre();
    const Vector2f soft = softMinMaxExtension();
    const float num = Mathf::Clamp(targetPosition, soft.x, soft.y);
    if (moveBase) {
        const Quaternionf localRotation = rig.partLocalRotation;
        Vector3f startingPosition = UnityNative::Inverse(localRotation) * rig.partLocalPosition;
        const float num2 = F(fx(num) - fx(previousTargetPosition));
        rig.partLocalPosition = localRotation * CalcTargetPosition(startingPosition, F(0.0 - fx(num2)), true);
        startingPosition = movingPartObject->localPosition;
        movingPartObject->localPosition = CalcTargetPosition(Vector3f::zero(), num, false);
    } else {
        movingPartObject->localPosition = CalcTargetPosition(Vector3f::zero(), num, false);
    }
    if (previousTargetPosition != num) previousTargetPosition = num;
}

// ModuleRoboticServoPiston.cs:339
void ModuleRoboticServoPiston::LateUpdate() {
    if (slaveTransformsSet) {
        if (movingPartObject == nullptr || basePartObject == nullptr) throw Nre();
        const Vector3f localPosition = movingPartObject->localPosition;
        const Vector3f localPosition2 = basePartObject->localPosition;
        const float num = F(1.0 / fx(static_cast<float>(slaveTransforms.size() + 1)));
        for (size_t i = 0; i < slaveTransforms.size(); i++) {
            const float num2 = F(fx(static_cast<float>(i + 1)) * fx(num));
            slaveTransforms[i]->localPosition = localPosition * F(1.0 - fx(num2)) + localPosition2 * num2;
        }
    }
}

// ModuleRoboticServoPiston.cs:354
void ModuleRoboticServoPiston::ServoFixedUpdate() {
    const bool paused = ServoEnvironment::flightDriverPause && ServoEnvironment::flightDriverPause();
    if (!initComplete || !ServoEnvironment::SceneIsFlight() || paused) return;
    if (servoMotorIsEngaged() && !servoIsLocked()) {
        const float num = F(fx(traverseVelocity()) * fx(Time::fixedDeltaTime));
        if (driveTargetPosition < targetPosition)
            driveTargetPosition = Mathf::Min(F(fx(driveTargetPosition) + fx(num)), targetPosition);
        else if (driveTargetPosition > targetPosition)
            driveTargetPosition = Mathf::Max(F(fx(driveTargetPosition) - fx(num)), targetPosition);
    } else {
        if (!HasEnoughResources() && lockPartOnPowerLoss() && !servoIsLocked()) EngageServoLock();
        if (servoIsLocked()) driveTargetPosition = lockAngle;
    }
    previousDisplacement = currentTransformPosition();
    SetJointTargetPosition();
    const float num2 = currentExtension();
    UpdateCurrentExtension();
    if (num2 != currentExtension()) SetDragCubes(currentExtension());
}

// ModuleRoboticServoPiston.cs:393
void ModuleRoboticServoPiston::SetJointTargetPosition() {
    const Vector2f soft = softMinMaxExtension();
    const Vector3f vector = Vector3f::right() * F((fx(soft.y) + fx(soft.x)) / 2.0 - fx(driveTargetPosition));
    if (servoJoint == nullptr) throw Nre();
    servoJoint->targetPosition = vector;
}

// ModuleRoboticServoPiston.cs:399
void ModuleRoboticServoPiston::OnModifyServo() {
    if (HighWarp()) {
        jointDrive = servoJoint->xDrive;
        jointDrive.positionSpring = 0.0f;
        jointDrive.positionDamper = 0.0f;
        jointDrive.maximumForce = 0.0f;
        servoJoint->xDrive = jointDrive;
        return;
    }
    const Vector2f soft = softMinMaxExtension();
    linearLimit.limit = F((fx(soft.y) - fx(soft.x)) * fx(1.01f) / 2.0);
    linearLimit.bounciness = Get<float>("linearLimitBounce");
    linearLimit.contactDistance = Get<float>("linearLimitContactDistance");
    servoJoint->linearLimit = linearLimit;
    linearLimitSpring.spring = Get<float>("linearLimitSpringSpring");
    linearLimitSpring.damper = Get<float>("linearLimitSpringDamper");
    servoJoint->linearLimitSpring = linearLimitSpring;
    jointDrive = servoJoint->xDrive;
    if (!servoMotorIsEngaged() && !servoIsLocked()) {
        if (!lockPartOnPowerLoss()) {
            jointDrive.positionSpring = 0.0f;
            jointDrive.positionDamper = 0.0f;
            jointDrive.maximumForce = 0.0f;
        }
    } else {
        jointDrive.positionSpring = F(fx(motorOutput) * fx(Get<float>("positionSpringMutliplier")));
        jointDrive.positionDamper = F(fx(Get<float>("positionDampingMutliplier")) * fx(maxMotorOutput()) * (fx(servoMotorSize()) * fx(0.01f)) * (fx(pistonDamping()) * fx(0.01f)));
        jointDrive.maximumForce = motorOutput;
    }
    servoJoint->xDrive = jointDrive;
}

// ModuleRoboticServoPiston.cs:436
void ModuleRoboticServoPiston::UpdateCurrentExtension() {
    if (servoJoint != nullptr) {
        if (movingPartObject == nullptr) throw Nre();
        const Vector3f vector = movingPartObject->localRotation * (servoJoint->targetPosition.x * axis) + movingPartObject->localPosition;
        const Vector3f vector2 = MatrixMultiplyDirection(servoParentTransformInverse, servoJoint->connectedAnchor);
        Set<float>("currentExtension", F(fx(Vector3f::Dot(vector - vector2, GetMainAxis())) + fx(driveTargetPosition)));
    }
}

// ModuleRoboticServoPiston.cs:446
void ModuleRoboticServoPiston::OnServoLockApplied() {
    UpdateCurrentExtension();
    lockAngle = currentExtension();
    driveTargetPosition = currentExtension();
}

// ModuleRoboticServoPiston.cs:453
void ModuleRoboticServoPiston::OnServoLockRemoved() {
    UpdateCurrentExtension();
    Set<float>("currentExtension", lockAngle);
    driveTargetPosition = currentExtension();
}

// ModuleRoboticServoPiston.cs:460
void ModuleRoboticServoPiston::OnServoMotorEngaged() {
    UpdateCurrentExtension();
    driveTargetPosition = currentExtension();
    motorManualDisengaged = false;
}

// ModuleRoboticServoPiston.cs:477
float ModuleRoboticServoPiston::GetFrameDisplacement() {
    positionDiff = Mathf::Abs(F(fx(currentTransformPosition()) - fx(previousDisplacement)));
    return positionDiff;
}

// ModuleRoboticServoPiston.cs:488
void ModuleRoboticServoPiston::ResetLaunchPosition() {
    const Vector2f soft = softMinMaxExtension();
    Fields()["targetExtension"]->SetValue(ModuleValue(Mathf::Min(Mathf::Max(soft.x, launchPosition()), soft.y)));
    targetPosition = targetExtension();
}

// ModuleRoboticServoPiston.cs:494
void ModuleRoboticServoPiston::OnSaveShip() {
    SetLaunchPosition(targetExtension());
    BaseServo::OnSaveShip();
}

// ModuleRoboticServoPiston.cs:500
void ModuleRoboticServoPiston::UpdatePAWUI(UI_Scene currentScene) {
    if (initComplete) {
        if (servoJoint != nullptr) UpdateCurrentExtension();
        Fields()["softMinMaxExtension"]->guiActiveEditor = !Get<bool>("hideUISoftMinMaxExtension");
        const bool showVelocity = !Get<bool>("hideUITraverseVelocity");
        Fields()["traverseVelocity"]->guiActiveEditor = showVelocity;
        Fields()["traverseVelocity"]->guiActive = showVelocity;
        const bool showDamping = !Get<bool>("hideUIDamping");
        Fields()["pistonDamping"]->guiActiveEditor = showDamping;
        Fields()["pistonDamping"]->guiActive = showDamping;
        prevServoIsLocked = servoIsLocked();
        BaseServo::UpdatePAWUI(currentScene);
    }
}

// ModuleRoboticServoPiston.cs:531
Vector3f ModuleRoboticServoPiston::CalcTargetPosition(const Vector3f& startingPosition, float offset, bool relative) {
    Vector3f vector;
    const std::string a = mainAxis();
    if (a == "Z")
        vector = Vector3f::forward();
    else if (a == "Y")
        vector = Vector3f::up();
    else
        vector = Vector3f::right();
    if (relative) return startingPosition + vector * offset;
    return vector * offset;
}

// ModuleRoboticServoPiston.cs:566
void ModuleRoboticServoPiston::TogglePistonAction(KSPActionParam&) {
    const Vector2f soft = softMinMaxExtension();
    if (fx(targetExtension()) - fx(soft.x) < (fx(soft.y) - fx(soft.x)) / 2.0)
        ExtendPiston();
    else
        RetractPiston();
}

// ModuleRoboticServoPiston.cs:578
void ModuleRoboticServoPiston::ExtendPiston() {
    if (!servoIsLocked()) {
        Fields()["targetExtension"]->SetValue(ModuleValue(softMinMaxExtension().y));
        UpdateTargetPosition(nullptr);
    } else {
        ServoEnvironment::PostScreenMessage("#autoLOC_8002356");
    }
}

// ModuleRoboticServoPiston.cs:591
void ModuleRoboticServoPiston::RetractPiston() {
    if (!servoIsLocked()) {
        Fields()["targetExtension"]->SetValue(ModuleValue(softMinMaxExtension().x));
        UpdateTargetPosition(nullptr);
    } else {
        ServoEnvironment::PostScreenMessage("#autoLOC_8002356");
    }
}

// ModuleRoboticServoPiston.cs:604
void ModuleRoboticServoPiston::OnResourceNonemptyEmpty(PartResource* resource) {
    if (!ResourceEventApplies(*this, resource)) return;
    for (size_t i = 0; i < resHandler.inputResources.size(); i++) {
        if (ResourceIsAnInput(*this, *resource, i)) {
            powerLossExtension = currentTransformPosition();
            ModifyServo(nullptr);
        }
    }
}

// ModuleRoboticServoPiston.cs:620
void ModuleRoboticServoPiston::InitAxisFieldLimits() {
    AxisFieldLimit l;
    l.limitedField = targetExtensionAxisField;
    l.softLimits = softMinMaxExtension();
    l.hardLimits = hardMinMaxLimits();
    AddAxisFieldLimit("targetExtension", l);
}

// ModuleRoboticServoPiston.cs:630
void ModuleRoboticServoPiston::UpdateAxisFieldHardLimit(const std::string& fieldName, const Vector2f& newlimits) {
    if (fieldName == "targetExtension") {
        Set<Vector2f>("hardMinMaxLimits", newlimits);
        ModifyLimits(nullptr);
    }
}

// ModuleRoboticServoPiston.cs:639
void ModuleRoboticServoPiston::UpdateAxisFieldSoftLimit(const std::string& fieldName, const Vector2f& newlimits) {
    if (fieldName == "targetExtension") {
        Set<Vector2f>("softMinMaxExtension", newlimits);
        ModifyLimits(nullptr);
    }
}

// ModuleRoboticServoPiston.cs:662
void ModuleRoboticServoPiston::SetDragCubes(float extension) {
    const Vector2f hard = hardMinMaxLimits();
    const float x = hard.x;
    const float y = hard.y;
    const float num = F((fx(x) + fx(y)) / 2.0);
    const float num2 = Mathf::Clamp01(F((fx(num) - fx(extension)) / (fx(num) - fx(x))));
    const float num3 = F(fx(Mathf::Clamp01(F(fx(extension) - fx(num)))) / (fx(y) - fx(num)));
    const float weight = F((1.0 - fx(num2)) * (1.0 - fx(num3)));
    Part& p = PartRef(*this);
    p.DragCubes.SetCubeWeight("0", num2);
    p.DragCubes.SetCubeWeight("50", weight);
    p.DragCubes.SetCubeWeight("100", num3);
}

}  // namespace ksp::Expansions::Serenity
