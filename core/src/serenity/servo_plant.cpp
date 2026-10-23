// WP-52: HeadlessServoPlant, the reference plant of the servo joint seam (see include/ksp/serenity/servo_joint.hpp and servo_modules.hpp).
// A documented 1-DOF model of the D6 drive. NOT PhysX. SYNTHETIC inertia. Angles are radians and rad/s inside the plant (PhysX drives work in
// radians); the moving node's localRotation is written in the module's own Quaternion convention.
// WP-52b: the mapping from the joint data to the drive (ServoDriveLaw) and the write-back (WriteServoCoordinate) are shared with the Jolt
// backend (servo_backend.cpp); the plant's arithmetic is unchanged.
#include <cmath>

#include "ksp/serenity/servo_backend.hpp"
#include "ksp/serenity/servo_modules.hpp"

namespace ksp::Expansions::Serenity {

namespace {
constexpr double kPi = 3.14159265358979323846;
}  // namespace

void HeadlessServoPlant::Step(double dt) {
    const ServoDriveCommand c = ServoDriveLaw(servo_);
    if (!c.valid) return;
    if (c.rotor) {
        // velocity drive about the main axis: torque = damper (targetAngularVelocity.x - omega), clamped; the tiny soft limits and the
        // 1e-10 spring make the rotor free to spin (RatchetLock is the module's, not the plant's)
        StepDrive1D(state, params_, c.drive, 0.0, c.targetVelocity, false, 0.0, 0.0, dt);
    } else if (c.prismatic) {
        // linear position drive along the main axis relative to the middle of the travel; the physical offset is -targetPosition.x
        StepDrive1D(state, params_, c.drive, c.target, 0.0, c.limited, c.limitLo, c.limitHi, dt);
    } else {
        // hinge / rotation servo: angular position drive. Hard limits (the limit springs are ignored by this model). The D6 angular drive
        // acts on the quaternion error, i.e. along the shortest arc: an unlimited joint (full rotation) takes the target representation
        // nearest to the current angle
        double goal = c.target;
        if (c.shortestArc) {
            const double twoPi = 2.0 * kPi;
            double diff = std::fmod(goal - state.position, twoPi);
            if (diff > kPi) diff -= twoPi;
            if (diff < -kPi) diff += twoPi;
            goal = state.position + diff;
        }
        StepDrive1D(state, params_, c.drive, goal, 0.0, c.limited, c.limitLo, c.limitHi, dt);
    }
    WriteServoCoordinate(servo_, state.position);
}

}  // namespace ksp::Expansions::Serenity
