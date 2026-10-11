#include "articulated.hpp"

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>

#include <algorithm>
#include <cmath>

#include "ksp/physics/physics_globals.hpp"
#include "shapes.hpp"

namespace spike {

using namespace JPH;

void ArticulatedVessel::Build(World& world, const CraftModel& craft, const Placement& pl, unsigned vesselGroup) {
    world_ = &world;
    craft_ = &craft;
    const size_t n = craft.parts.size();
    BodyInterface& bi = world.Bodies();
    bodyOfPart_.assign(n, -1);
    partLocalPos_.assign(n, Vec3::sZero());
    partLocalRot_.assign(n, Quat::sIdentity());
    const float maxAng = ksp::PhysicsGlobals::MaxAngularVelocity();
    // ---- bodies: one per FULL part (Part.cs:1868-1895)
    for (size_t i = 0; i < n; i++) {
        const PartSpec& s = craft.parts[i];
        if (!s.physical) continue;
        Ref<Shape> box = PartBoxShape(s, static_cast<uint64>(i));
        OffsetCenterOfMassShapeSettings oc(s.comLocal - box->GetCenterOfMass(), box);
        auto shapeRes = oc.Create();
        BodyCreationSettings b(shapeRes.Get(), pl.ToPhysics(s.pos), (pl.rotation * s.rot).Normalized(), EMotionType::Dynamic,
                               Layers::MOVING);
        KspBodyDefaults(b, maxAng);
        b.mMassPropertiesOverride.mMass = s.rbMass;
        b.mMassPropertiesOverride.mInertia = Mat44::sScale(s.inertiaPerMass * std::max(1.0f, s.rbMass));  // Part.cs:4263
        b.mCollisionGroup = CollisionGroup(world.VesselFilter(), vesselGroup, s.part->fields.sameVesselCollision ? 1u : 0u);
        Body* body = bi.CreateBody(b);
        bi.AddBody(body->GetID(), EActivation::Activate);
        bodyOfPart_[i] = static_cast<int>(bodies_.size());
        bodies_.push_back(body);
        partOfBody_.push_back(static_cast<int>(i));
        inertiaPerMass_.push_back(s.inertiaPerMass);
    }
    // physicsless parts ride on their owner's body (a Transform child in KSP)
    for (size_t i = 0; i < n; i++) {
        const PartSpec& s = craft.parts[i];
        if (s.physical) continue;
        const PartSpec& o = craft.parts[s.rbOwner];
        bodyOfPart_[i] = bodyOfPart_[s.rbOwner];
        const Quat inv = o.rot.Conjugated();
        const DVec3 d = s.pos - o.pos;
        partLocalPos_[i] = inv * Vec3(static_cast<float>(d.GetX()), static_cast<float>(d.GetY()), static_cast<float>(d.GetZ()));
        partLocalRot_[i] = (inv * s.rot).Normalized();
    }
    force_.assign(bodies_.size(), Vec3::sZero());
    torque_.assign(bodies_.size(), Vec3::sZero());
    angularDrag_.assign(bodies_.size(), 0.0f);
    // ---- joints: every PartJoint sub-joint (PartJoint.cs:182-210, SetJointLimits :359-395)
    jointBroken_.assign(craft.joints.size(), false);
    for (size_t j = 0; j < craft.joints.size(); j++) {
        const JointSpec& js = craft.joints[j];
        Body* b1 = bodies_[bodyOfPart_[js.host]];
        Body* b2 = bodies_[bodyOfPart_[js.target]];
        if (b1 == b2) continue;
        for (int k = 0; k < js.internalJoints; k++) {
            SixDOFConstraintSettings s;
            s.mSpace = EConstraintSpace::WorldSpace;
            const RVec3 anchor = pl.ToPhysics(js.anchors[k]);
            s.mPosition1 = s.mPosition2 = anchor;
            s.mAxisX1 = s.mAxisX2 = pl.DirToPhysics(js.axis);
            s.mAxisY1 = s.mAxisY2 = pl.DirToPhysics(js.secAxis);
            s.MakeFixedAxis(SixDOFConstraintSettings::TranslationX);
            s.MakeFixedAxis(SixDOFConstraintSettings::TranslationY);
            s.MakeFixedAxis(SixDOFConstraintSettings::TranslationZ);
            if (js.rigid) {
                s.MakeFixedAxis(SixDOFConstraintSettings::RotationX);
                s.MakeFixedAxis(SixDOFConstraintSettings::RotationY);
                s.MakeFixedAxis(SixDOFConstraintSettings::RotationZ);
            } else {
                for (int a = SixDOFConstraintSettings::RotationX; a <= SixDOFConstraintSettings::RotationZ; a++)
                    s.mMotorSettings[a] = MotorSettings(ESpringMode::StiffnessAndDamping, js.stiffness, 0.0f);
            }
            TwoBodyConstraint* c = static_cast<TwoBodyConstraint*>(s.Create(*b1, *b2));
            if (!js.rigid) {
                auto* six = static_cast<SixDOFConstraint*>(c);
                for (int a = SixDOFConstraintSettings::RotationX; a <= SixDOFConstraintSettings::RotationZ; a++)
                    six->SetMotorState(static_cast<SixDOFConstraintSettings::EAxis>(a), EMotorState::Position);
                six->SetTargetOrientationCS(Quat::sIdentity());
            }
            world.System().AddConstraint(c);
            SubJoint sj;
            sj.c = c;
            sj.joint = static_cast<int>(j);
            sj.localAnchor1 = Vec3(b1->GetInverseCenterOfMassTransform() * anchor);
            sj.localAnchor2 = Vec3(b2->GetInverseCenterOfMassTransform() * anchor);
            subJoints_.push_back(sj);
        }
    }
    partPos.assign(n, RVec3::sZero());
    partRot.assign(n, Quat::sIdentity());
    partVel.assign(n, Vec3::sZero());
    mass = 0.0;
    for (Body* b : bodies_) mass += 1.0 / b->GetMotionProperties()->GetInverseMass();
}

void ArticulatedVessel::ReadState() {
    const size_t n = craft_->parts.size();
    DVec3 comSum = DVec3::sZero();
    DVec3 velSum = DVec3::sZero();
    double m = 0.0;
    for (Body* b : bodies_) {
        const double bm = 1.0 / b->GetMotionProperties()->GetInverseMass();
        const RVec3 c = b->GetCenterOfMassPosition();
        comSum += DVec3(c) * bm;
        velSum += DVec3(b->GetLinearVelocity()) * bm;
        m += bm;
    }
    mass = m;
    const DVec3 cd = comSum / m;
    com = RVec3(static_cast<Real>(cd.GetX()), static_cast<Real>(cd.GetY()), static_cast<Real>(cd.GetZ()));
    comVel = (velSum / m).ToVec3RoundDown();
    for (size_t i = 0; i < n; i++) {
        const Body* b = bodies_[bodyOfPart_[i]];
        const Quat r = b->GetRotation();
        partPos[i] = b->GetPosition() + r * partLocalPos_[i];
        partRot[i] = r * partLocalRot_[i];
        const RVec3 partCom = partPos[i] + partRot[i] * craft_->parts[i].comLocal;
        partVel[i] = b->GetPointVelocity(partCom);
    }
}

void ArticulatedVessel::AddForceAtPosition(int part, Vec3Arg f, RVec3Arg point) {
    const int bIdx = bodyOfPart_[part];
    force_[bIdx] += f;
    torque_[bIdx] += Vec3(point - bodies_[bIdx]->GetCenterOfMassPosition()).Cross(f);
}

void ArticulatedVessel::AddUniformAcceleration(Vec3Arg a) { uniformAccel_ += a; }

void ArticulatedVessel::SetAngularDrag(int part, float drag) {
    if (craft_->parts[part].physical) angularDrag_[bodyOfPart_[part]] = drag;
}

void ArticulatedVessel::UpdateMass(const std::vector<float>& rbMass) {
    for (size_t b = 0; b < bodies_.size(); b++) {
        const float m = rbMass[partOfBody_[b]];
        MotionProperties* mp = bodies_[b]->GetMotionProperties();
        mp->SetInverseMass(1.0f / m);
        mp->SetInverseInertia(Vec3::sReplicate(1.0f) / (inertiaPerMass_[b] * std::max(1.0f, m)), Quat::sIdentity());
    }
}

void ArticulatedVessel::Flush() {
    for (size_t b = 0; b < bodies_.size(); b++) {
        Body* body = bodies_[b];
        const float m = 1.0f / body->GetMotionProperties()->GetInverseMass();
        body->AddForce(force_[b] + uniformAccel_ * m);
        body->AddTorque(torque_[b]);
        body->GetMotionProperties()->SetAngularDamping(angularDrag_[b]);
        force_[b] = Vec3::sZero();
        torque_[b] = Vec3::sZero();
    }
    uniformAccel_ = Vec3::sZero();
}

std::vector<BreakEvent> ArticulatedVessel::AfterStep(float dt, int tick) {
    std::vector<BreakEvent> out;
    if (!breakingEnabled || !breakAllowed) return out;
    const float inv = 1.0f / dt;
    for (const SubJoint& sj : subJoints_) {
        if (jointBroken_[sj.joint]) continue;
        const JointSpec& js = craft_->joints[sj.joint];
        if (js.autostrut) continue;  // autostruts stay unbreakable (PHYSICS-ENGINE-REVIEW.md 2.3)
        const auto* six = static_cast<const SixDOFConstraint*>(sj.c.GetPtr());
        const float f = six->GetTotalLambdaPosition().Length() * inv;
        const float t = (six->GetTotalLambdaRotation() + six->GetTotalLambdaMotorRotation()).Length() * inv;
        if (f > js.breakForceSub || t > js.breakTorqueSub) {
            jointBroken_[sj.joint] = true;
            out.push_back({sj.joint, tick, f, t});
        }
    }
    if (!out.empty()) DropCrossingAutostruts();
    return out;
}

void ArticulatedVessel::ShiftOrigin(Vec3Arg delta) {
    BodyInterface& bi = world_->Bodies();
    for (Body* b : bodies_) bi.SetPosition(b->GetID(), b->GetPosition() + delta, EActivation::DontActivate);
}

int ArticulatedVessel::FireDecouplers() {
    for (SubJoint& sj : subJoints_) {
        const JointSpec& js = craft_->joints[sj.joint];
        if (js.autostrut || jointBroken_[sj.joint]) continue;
        if (!craft_->parts[js.child].isDecoupler) continue;
        jointBroken_[sj.joint] = true;
    }
    DropCrossingAutostruts();
    return BodyCount();
}

void ArticulatedVessel::DropCrossingAutostruts() {
    // KSP splits the vessel and re-creates autostruts per new vessel (onVesselWasModified -> CycleAutoStrut, Part.cs:8023-8041):
    // an autostrut whose ends are no longer connected through attach joints disappears
    const size_t n = craft_->parts.size();
    std::vector<int> uf(n);
    for (size_t i = 0; i < n; i++) uf[i] = static_cast<int>(i);
    auto find = [&](int x) {
        while (uf[x] != x) x = uf[x] = uf[uf[x]];
        return x;
    };
    for (size_t i = 0; i < n; i++)
        if (!craft_->parts[i].physical && craft_->parts[i].parent >= 0) uf[find(static_cast<int>(i))] = find(craft_->parts[i].parent);
    for (int j = 0; j < craft_->attachJointCount; j++)
        if (!jointBroken_[j]) uf[find(craft_->joints[j].child)] = find(craft_->joints[j].parent);
    for (size_t j = craft_->attachJointCount; j < craft_->joints.size(); j++)
        if (!jointBroken_[j] && find(craft_->joints[j].child) != find(craft_->joints[j].parent)) jointBroken_[j] = true;
    for (SubJoint& sj : subJoints_)
        if (jointBroken_[sj.joint] && sj.c->GetEnabled()) {
            world_->System().RemoveConstraint(sj.c);
            sj.c->SetEnabled(false);
        }
}

std::vector<std::pair<float, float>> ArticulatedVessel::AttachJointLoads(float dt) const {
    // sum the sub-joint impulses of each PartJoint (force on body 2 = +lambda / dt at the sub-joint anchor); torque about the
    // centroid of the anchors = constraint torques + moments of the ring forces
    const int na = craft_->attachJointCount;
    std::vector<Vec3> f(na, Vec3::sZero()), t(na, Vec3::sZero());
    std::vector<RVec3> centroid(na, RVec3::sZero());
    std::vector<int> cnt(na, 0);
    std::vector<RVec3> anchor(subJoints_.size());
    for (size_t i = 0; i < subJoints_.size(); i++) {
        const SubJoint& sj = subJoints_[i];
        anchor[i] = sj.c->GetBody2()->GetCenterOfMassTransform() * sj.localAnchor2;
        if (sj.joint < na && sj.c->GetEnabled()) {
            centroid[sj.joint] += anchor[i];
            cnt[sj.joint]++;
        }
    }
    const float inv = 1.0f / dt;
    for (size_t i = 0; i < subJoints_.size(); i++) {
        const SubJoint& sj = subJoints_[i];
        if (sj.joint >= na || !sj.c->GetEnabled()) continue;
        const auto* six = static_cast<const SixDOFConstraint*>(sj.c.GetPtr());
        const Vec3 fi = six->GetTotalLambdaPosition() * inv;
        const RVec3 c = centroid[sj.joint] / static_cast<Real>(cnt[sj.joint]);
        f[sj.joint] += fi;
        t[sj.joint] += (six->GetTotalLambdaRotation() + six->GetTotalLambdaMotorRotation()) * inv + Vec3(anchor[i] - c).Cross(fi);
    }
    std::vector<std::pair<float, float>> out(na);
    for (int j = 0; j < na; j++) out[j] = cnt[j] ? std::make_pair(f[j].Length(), t[j].Length()) : std::make_pair(-1.0f, -1.0f);
    return out;
}

int ArticulatedVessel::ConstraintCount() const {
    int c = 0;
    for (const SubJoint& sj : subJoints_) c += sj.c->GetEnabled() ? 1 : 0;
    return c;
}

std::vector<std::pair<float, int>> ArticulatedVessel::JointErrors() const {
    std::vector<std::pair<float, int>> out;
    for (const SubJoint& sj : subJoints_) {
        if (!sj.c->GetEnabled()) continue;
        const RVec3 p1 = sj.c->GetBody1()->GetCenterOfMassTransform() * sj.localAnchor1;
        const RVec3 p2 = sj.c->GetBody2()->GetCenterOfMassTransform() * sj.localAnchor2;
        out.push_back({static_cast<float>(Vec3(p1 - p2).Length()), sj.joint});
    }
    std::sort(out.begin(), out.end(), [](auto& a, auto& b) { return a.first > b.first; });
    return out;
}

float ArticulatedVessel::MaxJointError() const {
    float e = 0.0f;
    for (const SubJoint& sj : subJoints_) {
        if (!sj.c->GetEnabled()) continue;
        const Body* b1 = sj.c->GetBody1();
        const Body* b2 = sj.c->GetBody2();
        const RVec3 p1 = b1->GetCenterOfMassTransform() * sj.localAnchor1;
        const RVec3 p2 = b2->GetCenterOfMassTransform() * sj.localAnchor2;
        e = std::max(e, static_cast<float>(Vec3(p1 - p2).Length()));
    }
    return e;
}

}  // namespace spike
