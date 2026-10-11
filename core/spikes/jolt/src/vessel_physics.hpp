// Jolt spike: the interface both rigid-body models implement (PHYSICS-SPIKE.md "Interfaces").
//
// Frames (lever 1, no per-tick world shift): every position handed in or out is in the PHYSICS FRAME = the main body's
// body-fixed (co-rotating) frame below the inverse-rotation threshold, relative to `Placement::frameOrigin` (a double the host
// moves only on a rare rebase, never per tick). With Jolt DOUBLE_PRECISION the origin can stay at the body centre forever.
// Units are KSP's: tonnes, metres, seconds, kN.
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Math/DVec3.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Real.h>

#include <string>
#include <vector>

#include "craft_model.hpp"
#include "jolt_world.hpp"

namespace spike {

// Craft frame (Unity axes, root at the origin) -> physics frame.
struct Placement {
    JPH::Quat rotation = JPH::Quat::sIdentity();  // craft axes -> physics axes
    JPH::DVec3 translation;                        // physics-frame position of the craft origin, relative to frameOrigin
    JPH::DVec3 frameOrigin;                        // body-fixed position of the physics-frame origin (0 = body centre)
    JPH::RVec3 ToPhysics(const JPH::DVec3& craftPoint) const {
        const JPH::Vec3 r = rotation * JPH::Vec3(static_cast<float>(craftPoint.GetX()), static_cast<float>(craftPoint.GetY()),
                                                 static_cast<float>(craftPoint.GetZ()));
        const JPH::DVec3 d = translation + JPH::DVec3(r);
        return JPH::RVec3(static_cast<JPH::Real>(d.GetX()), static_cast<JPH::Real>(d.GetY()), static_cast<JPH::Real>(d.GetZ()));
    }
    JPH::Vec3 DirToPhysics(JPH::Vec3Arg craftDir) const { return rotation * craftDir; }
};

struct BreakEvent {
    int joint = -1;   // CraftModel::joints index
    int tick = -1;
    float force = 0.0f, torque = 0.0f;  // per sub-joint (kN, kN m)
};

class VesselPhysics {
public:
    virtual ~VesselPhysics() = default;
    virtual const char* Name() const = 0;
    virtual void Build(World& world, const CraftModel& craft, const Placement& placement, unsigned vesselGroup) = 0;

    // ---- per tick ----
    // Bodies -> per-part state (pose of the part transform, velocity of the part CoM) and the vessel CoM / velocity.
    virtual void ReadState() = 0;
    // A force (kN) at a physics-frame point, on the body that carries `part` (Part.AddForceAtPosition semantics: buffered).
    virtual void AddForceAtPosition(int part, JPH::Vec3Arg force, JPH::RVec3Arg point) = 0;
    // The same acceleration on every body (gravity, Coriolis, centrifugal: VesselPrecalculate integrationAccel, ForceMode.Acceleration).
    virtual void AddUniformAcceleration(JPH::Vec3Arg accel) = 0;
    // rb.angularDrag from FlightIntegrator.UpdateAerodynamics (per part; the reduced model folds it into its cluster)
    virtual void SetAngularDrag(int part, float drag) = 0;
    // Mass changed (resource drain): rb.mass / inertia updates (FlightIntegrator.UpdateMassStats, Part.ValidateInertiaTensor)
    virtual void UpdateMass(const std::vector<float>& rbMass) = 0;
    // Push the buffered forces into Jolt (FlightIntegrator.Integrate).
    virtual void Flush() = 0;
    // After the Jolt step: joint loads and breaking. Returns the joints broken this tick.
    virtual std::vector<BreakEvent> AfterStep(float dt, int tick) = 0;
    // KSP's FloatingOrigin.setOffset applied to this vessel's bodies (measurement only: what the per-tick shift costs).
    virtual void ShiftOrigin(JPH::Vec3Arg delta) = 0;
    // Every body moves with velocity v (Part.ResumeVelocity on unpack / an orbital start).
    virtual void SetLinearVelocity(JPH::Vec3Arg v) = 0;

    // ---- one-off structural events ----
    // Fire every decoupler (staging): attach joints whose child is a decoupler are removed. Returns the new body count.
    virtual int FireDecouplers() = 0;

    // ---- stats ----
    virtual int BodyCount() const = 0;
    virtual int ConstraintCount() const = 0;  // Jolt constraints (sub-joints)
    // Largest anchor separation over the live constraints (m): joint stretch, the stability measure of the articulated model
    virtual float MaxJointError() const { return 0.0f; }

    // Joint breaking is suppressed while false (KSP: Vessel.ignoreCollisionsFrames = 60 after the ground placement,
    // PartJoint.OnJointBreak ignores breaks meanwhile, Vessel.cs:2543-2557, PartJoint.cs:397-401)
    bool breakAllowed = true;

    // ---- state (filled by ReadState) ----
    std::vector<JPH::RVec3> partPos;   // part transform position
    std::vector<JPH::Quat> partRot;    // part transform rotation
    std::vector<JPH::Vec3> partVel;    // rigid-body velocity at the part's CoM
    JPH::RVec3 com;                    // vessel CoM (physics frame)
    JPH::Vec3 comVel = JPH::Vec3::sZero();
    double mass = 0.0;
};

}  // namespace spike
