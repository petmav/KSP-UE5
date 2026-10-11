// Jolt spike, model A: the PhysX-equivalent articulated vessel (PHYSICS-ENGINE-REVIEW.md section 2).
// One dynamic body per FULL part; every PartJoint sub-joint is a Jolt SixDOFConstraint with the ConfigurableJoint's settings:
//   linear     Limited 1 m with drive positionSpring = JointForce (1e20)  -> translation locked (fixed axes)
//   angular    Limited 180 deg (Locked if rigid) with drives positionSpring = stiffness, damper 0  -> free rotation axes with
//              position motors (Jolt soft springs, StiffnessAndDamping k = stiffness, c = 0), or fixed when rigid
//   break      PhysX breakForce / breakTorque after SetUnbreakable(false) -> constraint impulse / dt compared per sub-joint
// Solver: Jolt's sequential impulses (velocity steps) + position steps. KSP's PhysX runs 50 position / 1 velocity iterations.
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Constraints/TwoBodyConstraint.h>

#include <vector>

#include "vessel_physics.hpp"

namespace spike {

class ArticulatedVessel final : public VesselPhysics {
public:
    const char* Name() const override { return "articulated"; }
    void Build(World& world, const CraftModel& craft, const Placement& placement, unsigned vesselGroup) override;
    void ReadState() override;
    void AddForceAtPosition(int part, JPH::Vec3Arg force, JPH::RVec3Arg point) override;
    void AddUniformAcceleration(JPH::Vec3Arg accel) override;
    void SetAngularDrag(int part, float drag) override;
    void UpdateMass(const std::vector<float>& rbMass) override;
    void Flush() override;
    std::vector<BreakEvent> AfterStep(float dt, int tick) override;
    void ShiftOrigin(JPH::Vec3Arg delta) override;
    void SetLinearVelocity(JPH::Vec3Arg v) override {
        for (JPH::Body* b : bodies_) b->SetLinearVelocity(v);
    }
    void AddLinearVelocity(JPH::Vec3Arg dv) override {
        for (JPH::Body* b : bodies_) b->SetLinearVelocity(b->GetLinearVelocity() + dv);
    }
    int FireDecouplers() override;
    int BodyCount() const override { return static_cast<int>(bodies_.size()); }
    int ConstraintCount() const override;
    std::vector<std::pair<float, float>> AttachJointLoads(float dt) const override;
    float MaxJointError() const override;
    // (error m, joint index) of every live sub-joint, largest first
    std::vector<std::pair<float, int>> JointErrors() const;
    bool breakingEnabled = true;

private:
    struct SubJoint {
        JPH::Ref<JPH::TwoBodyConstraint> c;
        int joint = -1;  // CraftModel::joints index
        JPH::Vec3 localAnchor1, localAnchor2;  // body-local anchor points (for the stretch measure)
    };
    void DropCrossingAutostruts();  // also removes the constraints of every broken joint
    World* world_ = nullptr;
    const CraftModel* craft_ = nullptr;
    std::vector<JPH::Body*> bodies_;        // per physical body
    std::vector<int> bodyOfPart_;           // part -> body index (rbOwner's body)
    std::vector<int> partOfBody_;
    std::vector<JPH::Vec3> partLocalPos_;   // part transform in its body's frame (physicsless parts: relative to the owner)
    std::vector<JPH::Quat> partLocalRot_;
    std::vector<JPH::Vec3> inertiaPerMass_; // per body
    std::vector<JPH::Vec3> force_, torque_; // buffered per body, about the body CoM
    std::vector<float> angularDrag_;
    JPH::Vec3 uniformAccel_ = JPH::Vec3::sZero();
    std::vector<SubJoint> subJoints_;
    std::vector<bool> jointBroken_;
};

}  // namespace spike
