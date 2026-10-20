// WP-52b: the joint drive backend of the servos (include/ksp/serenity/servo_backend.hpp). Engine glue in double: the drive mapping reads
// the ConfigurableJointData the ported modules wrote (ModuleRoboticServoHinge.cs:442-469, ModuleRoboticRotationServo.cs:445-480,
// ModuleRoboticServoPiston.cs:396-431, ModuleRoboticServoRotor.cs:465-481 / :637-648); no managed KSP statement is ported here.
#include "ksp/serenity/servo_backend.hpp"

#include <cmath>

#include "ksp/parts/part.hpp"
#include "ksp/serenity/servo_modules.hpp"
#include "ksp/unity/quaternion_native.hpp"

namespace ksp::Expansions::Serenity {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg2Rad = kPi / 180.0;

BaseServo* ServoOf(const Part& part) { return dynamic_cast<BaseServo*>(const_cast<Part&>(part).FindModuleOfClass("BaseServo")); }

bool Ready(const BaseServo& s) { return s.servoJoint != nullptr && s.movingPartObject != nullptr; }

// the piston's travel origin (the joint's connected anchor in the joint parent's frame): HeadlessServoPlant's expression
Vector3f PistonOrigin(const BaseServo& s) { return MatrixMultiplyDirection(s.servoParentTransformInverse, s.servoJoint->connectedAnchor); }

Vector3d Col(const Matrix4x4f& m, int c) {
    switch (c) {
        case 0: return Vector3d(m.m00, m.m10, m.m20);
        case 1: return Vector3d(m.m01, m.m11, m.m21);
        case 2: return Vector3d(m.m02, m.m12, m.m22);
        default: return Vector3d(m.m03, m.m13, m.m23);
    }
}
}  // namespace

// The physical drive (the derivations of servo_joint.cpp): the hinge / rotation servo's target is the twist of targetRotation with the sign
// of SetTargetRotationInternal's inversion removed, its physical limits [-high, -low]; the piston's target is -targetPosition.x, limits
// +-linearLimit.limit; the rotor drives targetAngularVelocity.x. The drive is angularXDrive (slerpDrive in Slerp mode) or xDrive.
ServoDriveCommand ServoDriveLaw(const BaseServo& servo) {
    ServoDriveCommand c;
    if (!Ready(servo)) return c;
    const ConfigurableJointData& j = *servo.servoJoint;
    c.valid = true;
    if (dynamic_cast<const ModuleRoboticServoRotor*>(&servo) != nullptr) {
        // velocity drive about the main axis; the tiny limit spring and soft limits leave the rotor free (RatchetLock is the module's)
        c.rotor = true;
        c.drive = j.rotationDriveMode == RotationDriveMode::Slerp ? j.slerpDrive : j.angularXDrive;
        c.target = 0.0;
        c.targetVelocity = static_cast<double>(j.targetAngularVelocity.x);
        return c;
    }
    if (dynamic_cast<const ModuleRoboticServoPiston*>(&servo) != nullptr) {
        c.prismatic = true;
        c.drive = j.xDrive;
        c.target = -static_cast<double>(j.targetPosition.x);
        c.limited = j.xMotion == ConfigurableJointMotion::Limited;
        const double lim = j.linearLimit.limit;
        c.limitLo = -lim;
        c.limitHi = lim;
        c.limitSpring = j.linearLimitSpring.spring;
        c.limitDamper = j.linearLimitSpring.damper;
        return c;
    }
    c.target = PhysicalTargetAngleDegrees(j) * kDeg2Rad;
    // a Limited joint whose limit SPRING is vanishingly small (the rotation servo with allowFullRotation sets 1e-16) does not stop the body
    const bool softLimit = j.angularXLimitSpring.spring > 0.0f && j.angularXLimitSpring.spring <= 1e-8f;
    c.limited = j.angularXMotion == ConfigurableJointMotion::Limited && !softLimit;
    c.limitLo = -static_cast<double>(j.highAngularXLimit.limit) * kDeg2Rad;
    c.limitHi = -static_cast<double>(j.lowAngularXLimit.limit) * kDeg2Rad;
    c.limitSpring = j.angularXLimitSpring.spring;
    c.limitDamper = j.angularXLimitSpring.damper;
    c.drive = j.rotationDriveMode == RotationDriveMode::Slerp ? j.slerpDrive : j.angularXDrive;
    c.shortestArc = !c.limited;
    return c;
}

// The moving transform at a physical coordinate (rad / m), as the reference plant writes it: the rotor's angle wrapped to [-180, 180]
// degrees, the hinge's as it is, the piston's offset along the main axis from its travel origin.
void WriteServoCoordinate(BaseServo& servo, double coordinate) {
    if (!Ready(servo)) return;
    ServoNode& moving = *servo.movingPartObject;
    if (dynamic_cast<ModuleRoboticServoRotor*>(&servo) != nullptr) {
        double deg = std::fmod(coordinate / kDeg2Rad, 360.0);  // std-math-ok: engine glue (the plant's own write-back, moved here unchanged): the PhysX-side joint angle, not a managed statement
        if (deg > 180.0) deg -= 360.0;
        if (deg < -180.0) deg += 360.0;
        moving.localRotation = UnityNative::AngleAxis(static_cast<float>(deg), servo.GetMainAxis());
    } else if (dynamic_cast<ModuleRoboticServoPiston*>(&servo) != nullptr) {
        moving.localPosition = PistonOrigin(servo) + servo.GetMainAxis() * static_cast<float>(coordinate);
    } else {
        moving.localRotation = UnityNative::AngleAxis(static_cast<float>(coordinate / kDeg2Rad), servo.GetMainAxis());
    }
}

// The servo body in part space: servoParentTransform (part.worldToLocal * jointParent.localToWorld, BaseServo.cs:168-171) places the joint
// parent; the moving transform sits in it (localPosition / localRotation), servoRb.centerOfMass in the moving transform. The coordinate is
// read back from the moving transform with the plant's convention (the inverse of WriteServoCoordinate).
bool ServoBodySpecOf(const Part& part, physics::ServoBodySpec& spec) {
    BaseServo* s = ServoOf(part);
    if (s == nullptr || !Ready(*s) || s->movingPartRB == nullptr || !ServoEnvironment::SceneIsFlight()) return false;
    const ServoNode& moving = *s->movingPartObject;
    const Matrix4x4f& m = s->servoParentTransform;
    const Vector3d c0 = Col(m, 0), c1 = Col(m, 1), c2 = Col(m, 2), t = Col(m, 3);
    const QuaternionD parentRot(c0.normalized(), c1.normalized(), c2.normalized());
    const Vector3d lp(moving.localPosition);
    spec.anchor = t + c0 * lp.x + c1 * lp.y + c2 * lp.z;
    const Vector3d a(s->GetMainAxis());
    const Vector3d ap = c0 * a.x + c1 * a.y + c2 * a.z;
    spec.axis = ap.normalized();
    spec.rotation = parentRot * QuaternionD(moving.localRotation);
    spec.centerOfMass = spec.anchor + spec.rotation * Vector3d(s->movingPartRB->centerOfMass);
    spec.prismatic = dynamic_cast<ModuleRoboticServoPiston*>(s) != nullptr;
    const Vector3d an = a.normalized();
    if (spec.prismatic) {
        spec.coordinate = Vector3d::Dot(lp - Vector3d(PistonOrigin(*s)), an);
    } else {
        const Quaternionf& q = moving.localRotation;
        const double dot = static_cast<double>(q.x) * an.x + static_cast<double>(q.y) * an.y + static_cast<double>(q.z) * an.z;
        double th = 2.0 * std::atan2(dot, static_cast<double>(q.w));  // std-math-ok: engine glue: reads the joint coordinate back from the moving transform, not a managed statement
        if (th > kPi) th -= 2.0 * kPi;
        if (th <= -kPi) th += 2.0 * kPi;
        spec.coordinate = th;
    }
    return true;
}

bool ServoJointDrive(Part& part, physics::PhysicsWorld::ArticulatedDrive& drive) {
    BaseServo* s = ServoOf(part);
    if (s == nullptr) return false;
    const ServoDriveCommand c = ServoDriveLaw(*s);
    if (!c.valid) return false;
    drive.spring = c.drive.positionSpring;
    drive.damper = c.drive.positionDamper;
    drive.maxForce = c.drive.maximumForce;
    drive.target = c.target;
    drive.targetVelocity = c.targetVelocity;
    drive.limited = c.limited;
    drive.limitLo = c.limitLo;
    drive.limitHi = c.limitHi;
    drive.limitSpring = c.limitSpring;
    drive.limitDamper = c.limitDamper;
    return true;
}

void ServoJointMoved(Part& part, double coordinate, double /*rate*/) {
    if (BaseServo* s = ServoOf(part)) WriteServoCoordinate(*s, coordinate);
}

void UseServoBackend(physics::VesselPhysicsOptions& o) {
    o.isArticulation = [](const physics::PartJointSpec& j) { return ServoArticulation(j); };
    o.servoBody = [](const Part& p, physics::ServoBodySpec& spec) { return ServoBodySpecOf(p, spec); };
    o.servoBodyEnd = [](const physics::PartJointSpec& j) { return ServoBodyEnd(j); };
    o.servoDrive = [](Part& p, physics::PhysicsWorld::ArticulatedDrive& d) { return ServoJointDrive(p, d); };
    o.servoMoved = [](Part& p, double c, double r) { ServoJointMoved(p, c, r); };
}

}  // namespace ksp::Expansions::Serenity
