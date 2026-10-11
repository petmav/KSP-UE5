// Jolt spike: shape and body helpers shared by the two models.
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#include <algorithm>
#include <stdexcept>

#include "craft_model.hpp"

namespace spike {

// The collider proxy of a part in part space: its drag-cube box. The leaf box carries the part index as user data
// (Shape::GetSubShapeUserData of a contact returns it through compounds and decorated shapes).
inline JPH::Ref<JPH::Shape> PartBoxShape(const PartSpec& s, JPH::uint64 partIndex) {
    const float cr = std::min(0.05f, 0.5f * std::min({s.boxHalf.GetX(), s.boxHalf.GetY(), s.boxHalf.GetZ()}));
    JPH::Ref<JPH::Shape> box = new JPH::BoxShape(s.boxHalf, cr);
    box->SetUserData(partIndex);
    if (s.boxCenter.IsNearZero()) return box;
    JPH::RotatedTranslatedShapeSettings rts(s.boxCenter, JPH::Quat::sIdentity(), box);
    auto r = rts.Create();
    if (r.HasError()) throw std::runtime_error(r.GetError().c_str());
    return r.Get();
}

// Body settings KSP gives a part Rigidbody (Part.cs:1868-1883 and the PhysX defaults read at runtime, PROFILE.md section 6):
// no gravity, no linear drag, angular drag from FlightIntegrator, maxAngularVelocity 50 (Physics.cfg), discrete collision,
// no sleeping (KSP vessels are woken by forces every tick). Jolt's default 500 m/s linear speed cap is lifted (orbital speeds).
inline void KspBodyDefaults(JPH::BodyCreationSettings& b, float maxAngularVelocity) {
    b.mLinearDamping = 0.0f;
    b.mAngularDamping = 0.0f;
    b.mMaxAngularVelocity = maxAngularVelocity;
    b.mMaxLinearVelocity = 1.0e5f;
    b.mAllowSleeping = false;
    b.mGravityFactor = 0.0f;
    b.mFriction = 0.6f;
    b.mRestitution = 0.0f;
    b.mMotionQuality = JPH::EMotionQuality::Discrete;
    b.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
}

}  // namespace spike
