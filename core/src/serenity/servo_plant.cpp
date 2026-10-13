// WP-52: HeadlessServoPlant, the reference plant of the servo joint seam (see include/ksp/serenity/servo_joint.hpp and servo_modules.hpp).
// A documented 1-DOF model of the D6 drive. NOT PhysX. SYNTHETIC inertia. Angles are radians and rad/s inside the plant (PhysX drives work in
// radians); the moving node's localRotation is written in the module's own Quaternion convention.
#include <cmath>

#include "ksp/serenity/servo_modules.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "servo_internal.hpp"

namespace ksp::Expansions::Serenity {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg2Rad = kPi / 180.0;
}  // namespace

void HeadlessServoPlant::Step(double dt) {
    using namespace detail;
    const std::shared_ptr<ConfigurableJointData>& j = servo_.servoJoint;
    if (j == nullptr || servo_.movingPartObject == nullptr) return;
    ServoNode& moving = *servo_.movingPartObject;
    BaseServo* s = &servo_;
    if (dynamic_cast<ModuleRoboticServoRotor*>(s) != nullptr) {
        // velocity drive about the main axis: torque = damper (targetAngularVelocity.x - omega), clamped; the tiny soft limits and the
        // 1e-10 spring make the rotor free to spin (RatchetLock is the module's, not the plant's)
        const JointDrive& drive = j->rotationDriveMode == RotationDriveMode::Slerp ? j->slerpDrive : j->angularXDrive;
        StepDrive1D(state, params_, drive, 0.0, static_cast<double>(j->targetAngularVelocity.x), false, 0.0, 0.0, dt);
        // the physical rotation is held in rad; the node gets AngleAxis(deg) of the wrapped angle
        double deg = std::fmod(state.position / kDeg2Rad, 360.0);
        if (deg > 180.0) deg -= 360.0;
        if (deg < -180.0) deg += 360.0;
        moving.localRotation = UnityNative::AngleAxis(static_cast<float>(deg), servo_.GetMainAxis());
    } else if (dynamic_cast<ModuleRoboticServoPiston*>(s) != nullptr) {
        // linear position drive along the main axis relative to the middle of the travel; the physical offset is -targetPosition.x
        const double target = -static_cast<double>(j->targetPosition.x);
        const bool limited = j->xMotion == ConfigurableJointMotion::Limited;
        const double lim = j->linearLimit.limit;
        StepDrive1D(state, params_, j->xDrive, target, 0.0, limited, -lim, lim, dt);
        const Vector3f origin = MatrixMultiplyDirection(servo_.servoParentTransformInverse, j->connectedAnchor);
        moving.localPosition = origin + servo_.GetMainAxis() * static_cast<float>(state.position);
    } else {
        // hinge / rotation servo: angular position drive. The physical angle the extension drives to is PhysicalTargetAngleDegrees; the joint
        // limits low / high are in the joint coordinate (the negative of the physical one, see servo_joint.cpp), so the physical range is
        // [-high, -low]. Hard limits (the limit springs are ignored by this model).
        const double target = PhysicalTargetAngleDegrees(*j) * kDeg2Rad;
        // a Limited joint whose limit SPRING is vanishingly small (the rotation servo with allowFullRotation sets 1e-16) does not stop the body
        const bool softLimit = j->angularXLimitSpring.spring > 0.0f && j->angularXLimitSpring.spring <= 1e-8f;
        const bool limited = j->angularXMotion == ConfigurableJointMotion::Limited && !softLimit;
        const double lo = -static_cast<double>(j->highAngularXLimit.limit) * kDeg2Rad;
        const double hi = -static_cast<double>(j->lowAngularXLimit.limit) * kDeg2Rad;
        const JointDrive& drive = j->rotationDriveMode == RotationDriveMode::Slerp ? j->slerpDrive : j->angularXDrive;
        // the D6 angular drive acts on the quaternion error, i.e. along the shortest arc: an unlimited joint (full rotation) takes the target
        // representation nearest to the current angle
        double goal = target;
        if (!limited) {
            const double twoPi = 2.0 * kPi;
            double diff = std::fmod(goal - state.position, twoPi);
            if (diff > kPi) diff -= twoPi;
            if (diff < -kPi) diff += twoPi;
            goal = state.position + diff;
        }
        StepDrive1D(state, params_, drive, goal, 0.0, limited, lo, hi, dt);
        moving.localRotation = UnityNative::AngleAxis(static_cast<float>(state.position / kDeg2Rad), servo_.GetMainAxis());
    }
}

}  // namespace ksp::Expansions::Serenity
