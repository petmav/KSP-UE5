// WP-20e: the Vessel's ground side: the landed RB anchor (Vessel.cs:1105-1183) and the landed placement CheckGroundCollision /
// getLowestPoint (Vessel.cs:2493-2650). C# paths are relative to decompiled/KSP-Decompiled/Assembly-CSharp/. The engine parts (the
// FixedJoint, Physics.Raycast, Collider.ClosestPoint) are the VesselEnvironment hooks the contact host (ksp/physics/contact_host.hpp)
// fills; float statements follow the Mono rule (ksp/dotnet/mono_float.hpp).
#include <cmath>
#include <limits>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/log.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/physics/collision_enhancer.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/unity/mathf.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {
bool KeyHeld(const std::shared_ptr<KeyBinding>& k) { return k != nullptr && k->GetKey(); }
}  // namespace

// Vessel.cs:1105 (joint = the anchor of env.rbAnchor*; Time.deltaTime = env.deltaTime, the frame time)
void Vessel::AddRBAnchor() {
    if (isEVA() || vesselType == VesselType::Flag || state == State::DEAD) return;
    for (size_t i = 0; i < GroundContacts.size(); i++)
        for (size_t j = 0; j < VesselAnchorExceptions.size(); j++)
            if (GroundContacts[i]->Modules().Contains(VesselAnchorExceptions[j])) {
                ResetRBAnchor();
                return;
            }
    anchorTimeCounter_ = f32store(fx(anchorTimeCounter_) + fx(env.deltaTime));
    if (rbAnchor_) {
        if (state == State::ACTIVE && (ctrlState.wheelThrottle != 0.0f || ctrlState.mainThrottle > 0.0f)) {
            ResetRBAnchor();
            return;
        }
        const Vector3f currentForce = env.rbAnchorCurrentForce ? env.rbAnchorCurrentForce(*this) : Vector3f();
        anchorForceAngle_ = Vector3f::Angle(currentForce, Vector3f::up());
        if (fx(anchorDeltaAngleTimeCounter_) >= fx(GameSettings::VESSEL_ANCHOR_ANGLE_TIME_THRESHOLD)) {
            if (prevAnchorForceAngle_ > 0.0f) deltaAngle_ = f32store(fx(anchorForceAngle_) - fx(prevAnchorForceAngle_));
            else deltaAngle_ = 0.0f;
            if (fx(Mathf::Abs(deltaAngle_)) >= fx(GameSettings::VESSEL_ANCHOR_ANGLE_CHANGE_THRESHOLD)) {
                ResetRBAnchor();
                return;
            }
            anchorDeltaAngleTimeCounter_ = 0.0f;
            prevAnchorForceAngle_ = anchorForceAngle_;
        }
        anchorDeltaAngleTimeCounter_ = f32store(fx(anchorDeltaAngleTimeCounter_) + fx(env.deltaTime));
        const Vector3f localUp = FlightMath::getUpAxis(*mainBody(), Vector3d(vesselTransform.position)).ToVector3f();
        if (Vector3f::Dot(currentForce, localUp) < 0.0f) {
            ResetRBAnchor();
        } else if (env.rbAnchorSetBreakForce) {
            env.rbAnchorSetBreakForce(*this, f32store(fx(Mathf::Max(static_cast<float>(totalMass), PhysicsGlobals::PartRBMassMin())) *
                                                      fx(static_cast<float>(graviticAcceleration.magnitude())) *
                                                      fx(GameSettings::VESSEL_ANCHOR_BREAK_FORCE_FACTOR)));
        }
    } else if (loaded && !packed && !precalc.isEasingGravity() && Landed &&
               fx(rb_velocity.sqrMagnitude()) < fx(GameSettings::VESSEL_ANCHOR_VELOCITY_THRESHOLD) &&
               fx(anchorTimeCounter_) >= fx(GameSettings::VESSEL_ANCHOR_TIME_THRESHOLD) && !KeyHeld(GameSettings::WHEEL_THROTTLE_DOWN) &&
               !KeyHeld(GameSettings::WHEEL_THROTTLE_UP)) {
        ResetRBAnchor();
        const float breakForce = f32store(fx(Mathf::Max(static_cast<float>(totalMass), PhysicsGlobals::PartRBMassMin())) *
                                          fx(static_cast<float>(graviticAcceleration.magnitude())) *
                                          fx(GameSettings::VESSEL_ANCHOR_BREAK_FORCE_FACTOR));
        // joint = AddComponent<FixedJoint>(); breakForce; breakTorque; enablePreprocessing = false
        if (env.rbAnchorCreate) rbAnchor_ = env.rbAnchorCreate(*this, breakForce, GameSettings::VESSEL_ANCHOR_BREAK_TORQUE);
    }
}

// Vessel.cs:1169 (Object.Destroy(joint) acts at the end of the frame in Unity; the anchor is gone at once here)
void Vessel::ResetRBAnchor() {
    if (rbAnchor_) {
        if (env.rbAnchorDestroy) env.rbAnchorDestroy(*this);
        rbAnchor_ = false;
    }
    anchorTimeCounter_ = 0.0f;
    anchorDeltaAngleTimeCounter_ = 0.0f;
    prevAnchorForceAngle_ = -1.0f;
}

// Vessel.cs:1180 (PhysX removed the joint itself: the joint object is gone before ResetRBAnchor runs)
void Vessel::OnJointBreak(float /*breakForce*/) {
    rbAnchor_ = false;
    ResetRBAnchor();
}

// Vessel.cs:2493
bool Vessel::CheckGroundCollision() {
    if (skipGroundPositioning) return true;
    CelestialBody& body = *mainBody();
    const Vector3f position = vesselTransform.position;
    const Vector3d vector3d(position);
    const Vector3d vector3d2 = FlightMath::getUpAxis(body, Vector3d(vesselTransform.position));
    const Vector3d relSurfaceNVector = body.GetRelSurfaceNVector(latitude, longitude);
    GetHeightFromTerrain(true);
    // pqsController.GetSurfaceHeight(relSurfaceNVector, overrideQuadBuildCheck: true)
    const double surface = env.pqsSurfaceHeight ? env.pqsSurfaceHeight(body, relSurfaceNVector) : body.Radius;
    const double num = surface - body.Radius - body.GetAltitude(Vector3d(vesselTransform.position));
    const float num2 = Mathf::Max(500.0f, f32store(fx(static_cast<float>(num)) + 500.0));
    const Vector3f vector = (Vector3d(position) + vector3d2 * static_cast<double>(num2)).ToVector3f();
    IgnoreGForces(1);
    Translate(vector3d2 * 5000.0);
    const float maxDistance = static_cast<float>((Vector3d(vector) - body.position).magnitude());
    groundCollisionHit_ = env.groundRaycast ? env.groundRaycast(*this, vector, (-vector3d2).ToVector3f(), maxDistance) : VesselRayHit{};
    if (groundCollisionHit_.hit) {
        SetPosition(vector3d);
        double num3;
        if (groundCollisionHit_.isPart) {
            num3 = HeightFromPartOffsetGlobal + heightFromPartOffsetLocal;
            // num3.ToString("0.000") (Vessel.cs:2519; WP-20g: was std::to_string, six decimals): F3 rounding, the log line of KSP.log
            log::Info("[" + GetDisplayName() + "]: ground contact from a Part! Moving upwards " + dotnet::FormatDoubleF(num3, 3) + "m");
        } else {
            if (heightFromTerrain < 0.0f) heightFromTerrain = 0.0f;
            num3 = static_cast<double>(heightFromTerrain) - (static_cast<double>(groundCollisionHit_.distance) - static_cast<double>(num2));
            float num4 = Mathf::Abs(getLowestPoint());
            if (!std::isinf(num4)) {
                const float angle = Mathf::Abs(static_cast<float>(Vector3d::Angle(Vector3d(groundCollisionHit_.normal), vector3d2)));
                const float num5 = f32store(fx(Mathf::Cos(f32store(fx(angle) * fx(static_cast<float>(3.14159265358979323846) / 180.0f)))) * fx(num4));
                if (fx(Mathf::Abs(f32store(fx(num4) - fx(num5)))) > fx(0.1f)) num4 = num5;
                if (heightFromTerrain > num4) {
                    num4 = f32store(fx(heightFromTerrain) - fx(num4));
                    num3 -= static_cast<double>(num4);
                } else {
                    num4 = f32store(fx(num4) - fx(heightFromTerrain));
                    num3 += static_cast<double>(num4);
                    physics::CollisionEnhancer::bypass = true;
                    ignoreCollisionsFrames = 60;
                }
            }
            if (groundCollisionHit_.isMobileLaunchPad) {
                const Quaternionf rotation = UnityNative::FromToRotation(groundCollisionHit_.normal, upAxis.ToVector3f()) * vesselTransform.rotation;
                SetRotation(rotation);
                physics::CollisionEnhancer::bypass = true;
                ignoreCollisionsFrames = 60;
            } else if (Vector3f::Angle(vesselTransform.up(), groundCollisionHit_.normal) > 1.0f) {
                SetRotation(vesselTransform.rotation *
                            UnityNative::FromToRotation(terrainNormal, vesselTransform.InverseTransformDirection(groundCollisionHit_.normal)));
                physics::CollisionEnhancer::bypass = true;
                ignoreCollisionsFrames = 60;
            }
        }
        const bool groundPart = rootPart != nullptr && rootPart->FindModuleOfClass("ModuleGroundPart") != nullptr;
        if (std::fabs(num3) < 0.10000000149011612 && !groundPart) {
            SetPosition(vector3d);
        } else {
            log::Info("[" + GetDisplayName() + "]: ground contact! - error. Moving Vessel " + (num3 < 0.0 ? " down " : " up ") + dotnet::FormatDoubleF(num3, 3) +
                      "m");  // Vessel.cs:2564 num3.ToString("0.000")
            SetPosition(vector3d + vector3d2 * num3);
        }
        physicsHoldLock_ = false;  // RemovePhysicsHoldLock
        IgnoreGForces(20 * static_cast<int>(Mathf::Max(static_cast<float>(Mathf::RoundToInt(static_cast<float>(num3))), 1.0f)));
        return true;
    }
    SetPosition(vector3d);
    return false;
}

// Vessel.cs:2578. The colliders are the hook's: Vessel::env.colliderClosestPoints with the role of each collider (the wheel host's scan of the wheel parts'
// real model colliders, WheelPhysicsHost::PlacementPoints: the radius and 0.15 of a WheelCollider, the 0.15 of a collisionEnhancer, :2622-2645), else
// colliderClosestPointsZ (plain proxies). ModuleWheelBase.onPartUnpack (:2591-2595, InopSystems.RemoveSubsystem(inopOnRails)) is what Part.Unpack does
// next anyway (the wheel host mirrors it), so it has no effect of its own here.
float Vessel::getLowestPoint() {
    float num = std::numeric_limits<float>::infinity();
    const Quaternionf rotation = vesselTransform.rotation;
    const Quaternionf rotation2 = UnityNative::FromToRotation(groundCollisionHit_.normal, Vector3f::forward()) * rotation;
    SetRotation(rotation2);
    Vector3f measure = vesselTransform.position;  // gameObject.transform.position, then += -Vector3.forward * 1000f
    measure = measure + (-Vector3f::forward()) * 1000.0f;
    // WP-29d: Vessel.cs:2599-2637: the lowest collider decides the wheel branch (a WheelCollider: minus its radius and 0.15, a collisionEnhancer:
    // minus 0.15). Without the role-aware scan every collider is Plain.
    bool haveCollider = false;
    VesselEnvironment::ColliderRole role = VesselEnvironment::ColliderRole::Plain;
    float roleRadius = 0.0f;
    for (Part* p : parts) {
        if (env.colliderClosestPoints) {
            for (const VesselEnvironment::ColliderPoint& cp : env.colliderClosestPoints(*p, measure)) {
                if (cp.z < num) {
                    haveCollider = true;
                    role = cp.role;
                    roleRadius = cp.radius;
                }
                num = Mathf::Min(num, cp.z);
            }
        } else if (env.colliderClosestPointsZ) {
            for (float z : env.colliderClosestPointsZ(*p, measure)) num = Mathf::Min(num, z);
        }
    }
    if (haveCollider) {
        if (role == VesselEnvironment::ColliderRole::WheelCollider) {
            num = f32store(fx(num) - fx(roleRadius));
            num = f32store(fx(num) - fx(0.15f));
        } else if (role == VesselEnvironment::ColliderRole::CollisionEnhancer) {
            num = f32store(fx(num) - fx(0.15f));
        }
    }
    const float result = f32store(fx(vesselTransform.position.z) - fx(num));
    SetRotation(rotation);
    return result;
}

}  // namespace ksp
