// WP-52: ModuleRoboticServoRotor.cs (see include/ksp/serenity/servo_modules.hpp). Every function cites ModuleRoboticServoRotor.cs:<line> of
// decompiled/KSP-Decompiled/Assembly-CSharp/Expansions/Serenity/.
#include <algorithm>
#include <cfloat>
#include <cmath>

#include "ksp/core/unity_time.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/serenity/servo_modules.hpp"
#include "servo_internal.hpp"

namespace ksp::Expansions::Serenity {

using namespace detail;
using GE = GameEvents;

namespace {
// the `!(text == "XYAndZ") && text == "slerp"` test of the C#
bool IsSlerp(const std::string& text) { return text == "slerp"; }
}  // namespace

// ModuleRoboticServoRotor.cs:122
void ModuleRoboticServoRotor::OnStart(StartState state) {
    Actions()["ResetPosition"]->SetGuiName(KSP::Localization::Localizer::Format("#autoLOC_6012044"));
    Events()["ResetPosition"]->SetGuiName(KSP::Localization::Localizer::Format("#autoLOC_6012044"));
    BaseServo::OnStart(state);
    if (!ServoEnvironment::SceneIsFlight() && !ServoEnvironment::SceneIsEditor()) return;
    if (ServoEnvironment::SceneIsFlight()) Set<float>("servoMotorLimit", 0.0f);
    Fields()["servoMotorLimit"]->guiName = KSP::Localization::Localizer::Format("#autoLOC_8003238");
    rpmLimitAxisField = dynamic_cast<BaseAxisField*>(Fields()["rpmLimit"]);
    ModifyRPMLimits(nullptr);
    Fields()["rpmLimit"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyRPMLimits(&v); });
    Fields()["brakePercentage"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyServo(&v); });
    Fields()["rotateCounterClockwise"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyDirection(&v); });
    Fields()["ratcheted"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyServo(&v); });
    Fields()["inverted"]->OnValueModified.push_back([this](const ModuleValue& v) { ModifyServo(&v); });
    GE::onVesselChange.Add(this, &ModuleRoboticServoRotor::OnVesselChange);
    if (servoIsMotorized()) GE::onPartResourceNonemptyEmpty.Add(this, &ModuleRoboticServoRotor::OnResourceNonemptyEmpty);
}

// ModuleRoboticServoRotor.cs:153
void ModuleRoboticServoRotor::OnDestroy() {
    BaseServo::OnDestroy();
    GE::onVesselChange.Remove(this, &ModuleRoboticServoRotor::OnVesselChange);
    GE::onPartResourceNonemptyEmpty.Remove(this, &ModuleRoboticServoRotor::OnResourceNonemptyEmpty);
}

// ModuleRoboticServoRotor.cs:165
void ModuleRoboticServoRotor::ModifyRPMLimits(const ModuleValue* field) {
    const Vector2f tvl = traverseVelocityLimits();
    if (rpmLimitAxisField == nullptr) throw Nre();
    rpmLimitAxisField->minValue = tvl.x;
    rpmLimitAxisField->maxValue = tvl.y;
    Set<float>("rpmLimit", Mathf::Clamp(rpmLimit(), tvl.x, tvl.y));
    currentVelocityLimit = rpmLimit();
    ModifyServo(field);
}

// ModuleRoboticServoRotor.cs:179
void ModuleRoboticServoRotor::ModifyDirection(const ModuleValue* field) {
    if (ServoEnvironment::SceneIsFlight()) ModifyServo(field);
}

// ModuleRoboticServoRotor.cs:187
void ModuleRoboticServoRotor::OnUpdate() {
    if (ServoEnvironment::SceneIsFlight()) RatchetLock();
}

// ModuleRoboticServoRotor.cs:201
void ModuleRoboticServoRotor::BrakeAction(KSPActionParam& kPar) {
    KSPActionType kSPActionType = kPar.Type();
    if (kSPActionType == KSPActionType::Toggle) kSPActionType = brakingMode ? KSPActionType::Deactivate : KSPActionType::Activate;
    switch (kSPActionType) {
    case KSPActionType::Deactivate:
        brakingMode = false;
        servoIsBraking = false;
        ModifyServo(nullptr);
        break;
    case KSPActionType::Activate:
        brakingMode = true;
        servoIsBraking = true;
        ModifyServo(nullptr);
        break;
    default:
        break;
    }
    ModifyServo(nullptr);
}

// ModuleRoboticServoRotor.cs:236
void ModuleRoboticServoRotor::ToggleMotorPower() {
    if (!servoIsLocked()) {
        if (servoMotorLimit() > 0.0f)
            Fields()["servoMotorLimit"]->SetValue(ModuleValue(0.0f));
        else
            Fields()["servoMotorLimit"]->SetValue(ModuleValue(100.0f));
        ModifyServo(nullptr);
    } else {
        ServoEnvironment::PostScreenMessage("#autoLOC_8002356");
    }
}

// ModuleRoboticServoRotor.cs:256
void ModuleRoboticServoRotor::ToggleMotorDirection() {
    if (!servoIsLocked()) {
        Fields()["rotateCounterClockwise"]->SetValue(ModuleValue(!rotateCounterClockwise()));
        ModifyServo(nullptr);
    } else {
        ServoEnvironment::PostScreenMessage("#autoLOC_8002356");
    }
}

// ModuleRoboticServoRotor.cs:269
void ModuleRoboticServoRotor::OnJointInit(bool) {
    const std::string servoTransformName = Get<std::string>("servoTransformName");
    if (movingPartObject == nullptr && !servoTransformName.empty()) movingPartObject = rig.Find(servoTransformName);
    if (!ServoEnvironment::SceneIsFlight()) {
        if (movingPartObject != nullptr) CreateServoRigidbody();
    } else if (movingPartObject != nullptr) {
        CreateServoRigidbody();
        UpdateLoadMass();
        Part& p = PartRef(*this);
        movingPartRB->isKinematic = p.fields.packed;
        movingPartRB->maxAngularVelocity = PhysicsGlobals::MaxAngularVelocity();
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
        highLimits.limit = 0.1f;
        lowLimits.limit = 0.01f;
        servoJoint->highAngularXLimit = highLimits;
        servoJoint->lowAngularXLimit = lowLimits;
        SetJointDrive(0.0f);
        xSpringLimit = servoJoint->angularXLimitSpring;
        xSpringLimit.spring = 1E-08f;
        xSpringLimit.damper = 1E-08f;
        servoJoint->angularXLimitSpring = xSpringLimit;
        servoJoint->targetAngularVelocity = Vector3f::zero();
        Vector3f unusedAxis;
        ToAngleAxis(movingPartObject->localRotation, freespinCurrentRotation, unusedAxis);
    }
}

// ModuleRoboticServoRotor.cs:316
void ModuleRoboticServoRotor::OnPostStartJointInit() {
    if (ServoEnvironment::SceneIsFlight()) {
        OnServoLockApplied();
        ModifyServo(nullptr);
        initComplete = true;
    }
}

// ModuleRoboticServoRotor.cs:330
void ModuleRoboticServoRotor::OnVisualizeServo(bool) {
    if (servoIsMotorized() && initComplete) {
        if (movingPartObject == nullptr) throw Nre();
        editorRotation = movingPartObject->localRotation;
        const float match = Get<float>("rotationMatch");
        const double a = fx(rpmLimit()) * fx(UtilMath::RPM2RadPerSec) * fx(match);
        const float lim = Mathf::Min(servoMotorLimit(), servoMotorSize());
        if ((rotateCounterClockwise() && !inverted()) || (!rotateCounterClockwise() && inverted()))
            movingPartObject->localRotation = RotateOnMainAxis(editorRotation, F(a * fx(-1.0f) * fx(lim)));
        else
            movingPartObject->localRotation = RotateOnMainAxis(editorRotation, F(a * fx(lim)));
    }
}

// ModuleRoboticServoRotor.cs:346
void ModuleRoboticServoRotor::OnModifyServo() {
    Set<float>("maxTorque", F(fx(maxMotorOutput()) * (fx(servoMotorSize()) * fx(0.01f))));
    if (!servoIsBraking) brakingMode = false;
    brakingTorque = brakingMode ? F(fx(Get<float>("brakeTorque")) * fx(brakePercentage()) * fx(0.01f)) : 0.0f;
    totalTorque = Mathf::Max(0.0f, F(fx(Get<float>("maxTorque")) * (fx(servoMotorLimit()) * fx(0.01f))));
    workingRPM = brakingMode ? 0.0f : Mathf::Max(0.0f, F(fx(rpmLimit()) * fx(UtilMath::RPM2RadPerSec)));
    const std::string text = angularDriveMode();
    if (IsSlerp(text)) {
        if ((!rotateCounterClockwise() && !inverted()) || (rotateCounterClockwise() && inverted())) workingRPM = F(fx(workingRPM) * fx(-1.0f));
    } else if ((rotateCounterClockwise() && !inverted()) || (!rotateCounterClockwise() && inverted())) {
        workingRPM = F(fx(workingRPM) * fx(-1.0f));
    }
    if (servoMotorIsEngaged()) {
        if (HighWarp()) {
            servoJoint->targetAngularVelocity = Vector3f::zero();
            SetJointDrive(0.0f);
        } else if (hasEnoughResources) {
            servoJoint->angularXMotion = ConfigurableJointMotion::Limited;
            if (brakingMode)
                SetJointDrive(brakingTorque);
            else
                SetJointDrive(totalTorque);
        } else {
            servoJoint->targetAngularVelocity = Vector3f::zero();
            SetJointDrive(1.0f);
        }
    } else {
        servoJoint->targetAngularVelocity = Vector3f::zero();
        if (brakingMode)
            SetJointDrive(brakingTorque);
        else
            SetJointDrive(1.0f);
    }
}

// ModuleRoboticServoRotor.cs:426
void ModuleRoboticServoRotor::ServoFixedUpdate() {
    if (!ServoEnvironment::SceneIsFlight() || !initComplete) return;
    timeSinceRPMRefresh = F(fx(timeSinceRPMRefresh) + fx(Time::deltaTime));
    if (timeSinceRPMRefresh > rpmRefreshSeconds) {
        timeSinceRPMRefresh = 0.0f;
        refreshRPMReadouts = true;
    }
    if (servoMotorIsEngaged() && !servoIsLocked()) {
        if (servoMotorLimit() > 0.0f) {
            resourceStateChange = false;
            if (Mathf::Abs(tempAngularVelocity) <= Mathf::Abs(workingRPM)) {
                const std::string text = angularDriveMode();
                const float step = F(fx(Get<float>("rotorSpoolTime")) * fx(Time::fixedDeltaTime));
                if (IsSlerp(text)) {
                    if ((!rotateCounterClockwise() && !inverted()) || (rotateCounterClockwise() && inverted()))
                        tempAngularVelocity = F(fx(tempAngularVelocity) - fx(step));
                    else
                        tempAngularVelocity = F(fx(tempAngularVelocity) + fx(step));
                } else if ((rotateCounterClockwise() && !inverted()) || (!rotateCounterClockwise() && inverted())) {
                    tempAngularVelocity = F(fx(tempAngularVelocity) - fx(step));
                } else {
                    tempAngularVelocity = F(fx(tempAngularVelocity) + fx(step));
                }
                servoJoint->targetAngularVelocity = Vector3f::right() * tempAngularVelocity;
            } else {
                servoJoint->targetAngularVelocity = Vector3f::right() * workingRPM;
            }
        } else {
            tempAngularVelocity = 0.0f;
            servoJoint->targetAngularVelocity = Vector3f::zero();
        }
    } else {
        tempAngularVelocity = 0.0f;
        servoJoint->targetAngularVelocity = Vector3f::zero();
        if (lockPartOnPowerLoss()) {
            if (!servoIsLocked() && resourceStateChange) {
                brakingMode = true;
                servoIsBraking = true;
                EngageServoLock();
            }
            SetJointDrive(Get<float>("brakeTorque"));
        }
        if (servoIsLocked()) {
            targetRotation = UnityNative::AngleAxis(lockAngle, GetMainAxis());
            SetTargetRotationLocal(*servoJoint, targetRotation, cachedStartingRotation);
            SetJointDrive(1.0f);
        }
    }
    previousDisplacement = currentTransformAngle();
}

// ModuleRoboticServoRotor.cs:502
void ModuleRoboticServoRotor::RatchetLock() {
    if (servoMotorIsEngaged()) return;
    if (servoJoint == nullptr || movingPartRB == nullptr || movingPartObject == nullptr) throw Nre();
    servoJoint->angularXMotion = ConfigurableJointMotion::Free;
    servoJoint->targetAngularVelocity = Vector3f::zero();
    Set<std::string>("resourceConsumption", "0.0");
    Set<std::string>("motorState", "Unpowered");
    movingPartRB->centerOfMass = Vector3f::zero();
    movingPartRB->inertiaTensorRotation = Quaternionf::identity();
    ratchetLockRotation = Quaternionf::identity();
    float num = 0.0f;
    Vector3f vector;
    ToAngleAxis(movingPartObject->localRotation, freespinCurrentRotation, vector);
    num = F(fx(freespinCurrentRotation) - fx(freespinPreviousRotation));
    int constraints = 0;  // RigidbodyConstraints.None
    const std::string a = mainAxis();
    if (a == "Y") {
        constraints = kFreezeRotationY;
        num = F(fx(num) * fx(vector.y));
        ratchetLockRotation = UnityNative::AngleAxis(freespinPreviousRotation, Vector3f::right());
    } else if (a == "X") {
        constraints = kFreezeRotationX;
        num = F(fx(num) * fx(vector.x));
        ratchetLockRotation = UnityNative::AngleAxis(freespinPreviousRotation, Vector3f::up());
    } else if (a == "Z") {
        constraints = kFreezeRotationZ;
        num = F(fx(num) * fx(vector.z));
        ratchetLockRotation = UnityNative::AngleAxis(freespinPreviousRotation, Vector3f::forward());
    }
    if (num > 0.0f && num > ratchetTolerance) {
        if (ratcheted() == 2) {
            movingPartRB->constraints = constraints;
            movingPartObject->localRotation = ratchetLockRotation;
        } else {
            movingPartRB->constraints = 0;
        }
    } else if (num < 0.0f && Mathf::Abs(num) > ratchetTolerance) {
        if (ratcheted() == 1) {
            movingPartRB->constraints = constraints;
            movingPartObject->localRotation = ratchetLockRotation;
        } else {
            movingPartRB->constraints = 0;
        }
    } else {
        movingPartRB->constraints = 0;
    }
    freespinPreviousRotation = freespinCurrentRotation;
}

// ModuleRoboticServoRotor.cs:573
float ModuleRoboticServoRotor::GetFrameDisplacement() {
    rotationDiff = F(fx(currentTransformAngle()) - fx(previousDisplacement));
    if (rotationDiff >= 180.0f)
        rotationDiff = F(fx(rotationDiff) - 360.0);
    else if (rotationDiff < -180.0f)
        rotationDiff = F(fx(rotationDiff) + 360.0);
    rotationDiff = F(fx(Mathf::Abs(rotationDiff)) * 60.0 / 360.0);
    return rotationDiff;
}

// ModuleRoboticServoRotor.cs:593
void ModuleRoboticServoRotor::OnSaveShip() {
    SetLaunchPosition(rpmLimit());
    BaseServo::OnSaveShip();
}

// ModuleRoboticServoRotor.cs:599
void ModuleRoboticServoRotor::ResetLaunchPosition() {
    Fields()["rpmLimit"]->SetValue(ModuleValue(launchPosition()));
    ModifyServo(nullptr);
}

// ModuleRoboticServoRotor.cs:612
void ModuleRoboticServoRotor::OnVesselChange(Vessel* vsl) {
    Part* p = PartOf();
    if (p != nullptr && p->vessel != nullptr && vsl != nullptr && vsl->persistentId == p->vessel->persistentId) UpdateLoadMass();
}

// ModuleRoboticServoRotor.cs:620
void ModuleRoboticServoRotor::UpdateLoadMass() {
    loadMass = 0.0f;
    Part& p = PartRef(*this);
    for (size_t i = 0; i < p.children.size(); i++) {
        p.children[i]->UpdateMass();
        loadMass = F(fx(loadMass) + fx(p.children[i]->fields.mass));
    }
}

// ModuleRoboticServoRotor.cs:630
void ModuleRoboticServoRotor::SetJointDrive(float torque) {
    if (servoJoint == nullptr) throw Nre();
    const std::string text = angularDriveMode();
    if (IsSlerp(text)) {
        servoJoint->rotationDriveMode = RotationDriveMode::Slerp;
        xDrive = servoJoint->slerpDrive;
        xDrive.positionSpring = Get<float>("angularPositionSpring");
        xDrive.positionDamper = Get<float>("angularPositionDamper");
        xDrive.maximumForce = torque;
        servoJoint->slerpDrive = xDrive;
    } else {
        servoJoint->rotationDriveMode = RotationDriveMode::XYAndZ;
        xDrive = servoJoint->xDrive;
        xDrive.positionSpring = Get<float>("angularPositionSpring");
        xDrive.positionDamper = (servoMotorIsEngaged() || brakingMode) ? Get<float>("angularPositionDamper") : 1E-11f;
        xDrive.maximumForce = torque;
        servoJoint->angularXDrive = xDrive;
    }
}

// ModuleRoboticServoRotor.cs:653
Quaternionf ModuleRoboticServoRotor::RotateOnMainAxis(const Quaternionf& originalRotation, float rotationAngle) {
    const float half = F(fx(rotationAngle) * 0.5 * fx(Mathf::Deg2Rad));
    const Vector3f vector = GetMainAxis() * Mathf::Sin(half);
    Quaternionf quaternion;
    quaternion.x = vector.x;
    quaternion.y = vector.y;
    quaternion.z = vector.z;
    quaternion.w = Mathf::Cos(half);
    return originalRotation * quaternion;
}

// ModuleRoboticServoRotor.cs:664 (the field visibility is UI; the readout refresh and the prev* bookkeeping stay)
void ModuleRoboticServoRotor::UpdatePAWUI(UI_Scene currentScene) {
    const bool num = (prevServoMotorIsEngaged != servoMotorIsEngaged() && currentScene == UI_Scene::Flight) ||
                     (prevServoIsMotorized != servoIsMotorized() && currentScene == UI_Scene::Editor) || prevServoIsLocked != servoIsLocked();
    if (ServoEnvironment::SceneIsFlight() && refreshRPMReadouts) {
        refreshRPMReadouts = false;
        Set<float>("currentRPM", transformRateOfMotion);
    }
    if (!num) {
        BaseServo::UpdatePAWUI(currentScene);
        return;
    }
    prevServoMotorIsEngaged = servoMotorIsEngaged();
    prevServoIsMotorized = servoIsMotorized();
    prevServoIsLocked = servoIsLocked();
    BaseServo::UpdatePAWUI(currentScene);
}

// ModuleRoboticServoRotor.cs:721
void ModuleRoboticServoRotor::OnResourceNonemptyEmpty(PartResource* resource) {
    if (!ResourceEventApplies(*this, resource)) return;
    for (size_t i = 0; i < resHandler.inputResources.size(); i++)
        if (ResourceIsAnInput(*this, *resource, i)) resourceStateChange = true;
}

}  // namespace ksp::Expansions::Serenity
