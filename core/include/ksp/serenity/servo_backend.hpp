// WP-52b: the joint drive backend of the robotic servos on the reduced rigid-body model (Jolt).
//
// KSP gives every servo a second Rigidbody on its moving transform (Part.servoRb, BaseServo.CreateServoRigidbody, BaseServo.cs:1196) and a
// PhysX ConfigurableJoint from it to part.Rigidbody (BaseServo.OnJointInit in each servo, BaseServo.PostStartInitJoint :1158); the parts
// attached to the servo's moving nodes are jointed to the servo body (AttachServoRigidBody / NodeRigidBody, :1602 / :1627). The module
// logic (WP-52) writes the joint's drive into ConfigurableJointData; this file is what replaces PhysX for it:
//
//   * ServoDriveLaw: the ONE mapping ConfigurableJointData -> a one-degree-of-freedom drive in the physical coordinate (rad / m), shared
//     by HeadlessServoPlant (the reference plant, servo_plant.cpp) and the Jolt backend, so the two cannot drift apart.
//   * The VesselPhysicsOptions hooks (UseServoBackend): the servo body is a mass element of the cluster of the servo's moving side, the
//     servo joint a PhysicsWorld articulated constraint (Jolt hinge / slider with a PositionAndVelocity motor) that reads the drive every
//     step (servoDrive) and moves the moving transform after it (servoMoved: what the plant's Step wrote, now from the bodies).
//
// UNVERIFIED (no capture of a servo exists; the SquadExpansion parts are not installed): the sign convention between targetRotation /
// targetPosition / targetAngularVelocity and the physical coordinate (servo_joint.cpp's derivation), PhysX's limit and limit-spring
// behaviour, the drive law itself (PhysX's implicit D6 drive as documented; tools/wp52b/drive_reference.py). Every servo MODULE value in the
// tests is a labelled SYNTHETIC fixture.
#pragma once

#include "ksp/physics/vessel_physics.hpp"
#include "ksp/serenity/servo_joint.hpp"

namespace ksp {
class Part;
}

namespace ksp::Expansions::Serenity {

class BaseServo;

// The drive a servo's ConfigurableJointData asks for, in the PHYSICAL coordinate of the moving transform (revolute: rad, the angle of
// movingPartObject.localRotation about GetMainAxis(); prismatic: m, the offset of its localPosition along GetMainAxis() from the middle of
// the travel). Hinge / rotation servo: angular position drive; rotor: angular velocity drive; piston: linear position drive.
struct ServoDriveCommand {
    bool valid = false;            // a servo with a joint and a moving transform
    bool prismatic = false;        // the piston
    bool rotor = false;
    JointDrive drive;              // positionSpring k, positionDamper c, maximumForce (angularXDrive / slerpDrive by rotationDriveMode, xDrive)
    double target = 0.0;           // rad / m
    double targetVelocity = 0.0;   // rad/s / m/s
    bool limited = false;          // a Limited joint motion whose limit spring does not vanish (0 < spring <= 1e-8 means no limit)
    double limitLo = 0.0, limitHi = 0.0;  // physical (the joint limits are in the joint coordinate, its negative: servo_joint.cpp)
    float limitSpring = 0.0f, limitDamper = 0.0f;  // angularXLimitSpring / linearLimitSpring (0: a hard limit)
    bool shortestArc = false;      // an unlimited revolute position drive acts on the quaternion error: the nearest representative
};
ServoDriveCommand ServoDriveLaw(const BaseServo& servo);
// The moving transform at a physical coordinate (the plant's and the backend's write-back): localRotation / localPosition of movingPartObject
void WriteServoCoordinate(BaseServo& servo, double coordinate);

// ---- the VesselPhysicsOptions hooks ------------------------------------------------------------------------------------------------
// servoBody: the servo body of a part with a BaseServo in the flight scene (a joint, a moving transform and its Rigidbody), part space
bool ServoBodySpecOf(const Part& part, physics::ServoBodySpec& spec);
// servoBodyEnd: the part whose servo body an attach joint uses (ServoArticulation's test, base_servo.cpp), nullptr: none
const Part* ServoBodyEnd(const physics::PartJointSpec& j);
// servoDrive: ServoDriveLaw of the part's servo as an articulated drive
bool ServoJointDrive(Part& part, physics::PhysicsWorld::ArticulatedDrive& drive);
// servoMoved: the moving transform at the joint coordinate (HeadlessServoPlant::Step's write-back)
void ServoJointMoved(Part& part, double coordinate, double rate);
// Installs the four hooks and isArticulation = ServoArticulation
void UseServoBackend(physics::VesselPhysicsOptions& options);

}  // namespace ksp::Expansions::Serenity
