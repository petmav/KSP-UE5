// WP-12d: PQS_Collider.cs, PQS_ColliderAdvanced.cs, PQS_PartCollider.cs, PQSMod_CreateSphereCollider.cs (see the header).
#include "ksp/terrain/pqs_surface_colliders.hpp"

#include <algorithm>
#include <atomic>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/unity/quaternion_native.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {

// Transform.up = value (UnityEngine Transform.cs: rotation = Quaternion.FromToRotation(Vector3.up, value), native)
void SetUp(PQSBoxCollider& col, const Vector3f& value) {
    col.rotation = UnityNative::FromToRotation(Vector3f::up(), value);
    col.writes++;
}

PQSBoxCollider NewBox(const std::string& name, const Vector3f& localScale) {
    PQSBoxCollider b;
    b.id = NewSurfaceColliderId();
    b.name = name;
    b.localScale = localScale;
    return b;
}

Vector3f OwnerPosition(const SurfaceColliderOwner& o) { return o.position ? o.position() : Vector3f::zero(); }
Vector3f OwnerVelocity(const SurfaceColliderOwner& o) { return o.velocity ? o.velocity() : Vector3f::zero(); }
void SetOwnerPosition(SurfaceColliderOwner& o, const Vector3f& v) {
    if (o.setPosition) o.setPosition(v);
}
void SetOwnerVelocity(SurfaceColliderOwner& o, const Vector3f& v) {
    if (o.setVelocity) o.setVelocity(v);
}
bool HasRigidbody(const SurfaceColliderOwner& o) { return !o.hasRigidbody || o.hasRigidbody(); }
bool IsKinematic(const SurfaceColliderOwner& o) { return o.isKinematic && o.isKinematic(); }

}  // namespace

// ---- IgnoreCollisionLog ---------------------------------------------------------------------------------------------------------

void IgnoreCollisionLog::Record(int a, int b, bool ignore) {
    calls_.push_back({a, b, ignore});
    state_[{std::min(a, b), std::max(a, b)}] = ignore;
}

bool IgnoreCollisionLog::IsIgnored(int a, int b) const {
    const auto it = state_.find({std::min(a, b), std::max(a, b)});
    return it != state_.end() && it->second;
}

void IgnoreCollisionLog::Clear() {
    calls_.clear();
    state_.clear();
}

IgnoreCollisionLog& PhysicsIgnoreCollisions() {
    static IgnoreCollisionLog log;
    return log;
}

int NewSurfaceColliderId() {
    static std::atomic<int> next{1};
    return next.fetch_add(1, std::memory_order_relaxed);
}

PQSBoxWorldPose BoxWorldPose(const PQSBoxCollider& box, const PQS* sphere, const Vector3d& ownerPosition) {
    PQSBoxWorldPose p;
    switch (box.parent) {
        case PQSBoxCollider::Parent::Sphere:
            p.centre = sphere != nullptr ? sphere->GetWorldPosition(Vector3d(box.localPosition)) : Vector3d(box.localPosition);
            break;
        case PQSBoxCollider::Parent::Owner: p.centre = ownerPosition + Vector3d(box.localPosition); break;
        case PQSBoxCollider::Parent::Root: p.centre = Vector3d(box.position); break;
    }
    p.rotation = QuaternionD(box.rotation);
    p.halfExtents = Vector3f(box.localScale.x * 0.5f, box.localScale.y * 0.5f, box.localScale.z * 0.5f);
    return p;
}

// ---- PQS_Collider.cs -----------------------------------------------------------------------------------------------------------

double PQS_Collider::Height(const Vector3d& v) const { return surfaceHeight ? surfaceHeight(v) : sphere->GetSurfaceHeight(v); }

// PQS_Collider.cs:19
void PQS_Collider::Reset() {
    colliderDepth = 100.0f;
    colliderMaxDistance = 10.0f;
}

// PQS_Collider.cs:25 (collidersVisibleDEBUG: CreatePrimitive(Cube) carries the same unit BoxCollider; its renderer is not modelled)
void PQS_Collider::Start() {
    colliderObject = NewBox(std::string(), Vector3f::one());
    colliderObject.localScale = Vector3f(1.0f, colliderDepth, 1.0f);
    halfDepth = f32store(fx(colliderDepth) / 2.0);
    colliderObject.name = owner.name + "PQSCollider";
    colliderObject.activeSelf = false;
    isColliderActive = false;
    SetTargetSphere(sphere);
}

// PQS_Collider.cs:44
void PQS_Collider::SetTargetSphere(PQS* s) {
    sphere = s;
    if (sphere == nullptr) {
        colliderObject.parent = PQSBoxCollider::Parent::Owner;
        colliderObject.localPosition = Vector3f::zero();
        DeactivateCollider();
    } else {
        colliderObject.parent = PQSBoxCollider::Parent::Sphere;
        UpdateCollider();
    }
}

// PQS_Collider.cs:60
void PQS_Collider::FixedUpdate() { UpdateCollider(); }

// PQS_Collider.cs:65 (all float: Vector3 vector = (Vector3)sphere.GetRelativePosition(transform.position))
void PQS_Collider::UpdateCollider() {
    if (sphere == nullptr) return;
    const Vector3f vector = sphere->GetRelativePosition(Vector3d(OwnerPosition(owner))).ToVector3f();
    const Vector3f normalized = vector.normalized();
    const float magnitude = vector.magnitude();
    const float num = static_cast<float>(Height(Vector3d(vector)));
    if (fx(magnitude) - fx(num) < fx(colliderMaxDistance)) {
        ActivateCollider();
        colliderObject.localPosition = normalized * f32store(fx(num) - fx(halfDepth));
        SetUp(colliderObject, normalized);
    } else {
        DeactivateCollider();
    }
}

// PQS_Collider.cs:86
void PQS_Collider::ActivateCollider() {
    if (!isColliderActive) {
        colliderObject.activeSelf = true;
        isColliderActive = true;
    }
}

// PQS_Collider.cs:95
void PQS_Collider::DeactivateCollider() {
    if (isColliderActive) {
        colliderObject.activeSelf = false;
        isColliderActive = false;
    }
}

// ---- PQS_ColliderAdvanced.cs ---------------------------------------------------------------------------------------------------

PQS_ColliderAdvanced::PQS_ColliderAdvanced() = default;
PQS_ColliderAdvanced::~PQS_ColliderAdvanced() {
    if (started_) OnDestroy();
}

void PQS_ColliderAdvanced::ClearStatics() {
    sampleRotationsD.clear();
    colliderList.clear();
    colliderListCreated = false;
}

double PQS_ColliderAdvanced::Height(const Vector3d& v) const { return surfaceHeight ? surfaceHeight(v) : sphere->GetSurfaceHeight(v); }

// PQS_ColliderAdvanced.cs:86
void PQS_ColliderAdvanced::Reset() { colliderSize = Vector2f(1.0f, 1.0f); }

// PQS_ColliderAdvanced.cs:91
void PQS_ColliderAdvanced::RegisterColliders() {
    if (!colliderListCreated) {
        colliderList.clear();
        colliderListCreated = true;
    }
    IgnoreCollisionLog& physics = PhysicsIgnoreCollisions();
    for (PQS_ColliderAdvanced* collider : colliderList) {
        if (useGravityCollider) {
            physics.Record(collider->owner.colliderId, gravityCollider.id);
            if (collider->useGravityCollider) physics.Record(collider->gravityCollider.id, gravityCollider.id);
        }
        if (useVelocityCollider) {
            physics.Record(collider->owner.colliderId, velocityCollider.id);
            physics.Record(collider->owner.colliderId, velocityCliffCollider.id);
            if (collider->useVelocityCollider) {
                physics.Record(collider->velocityCollider.id, velocityCollider.id);
                physics.Record(collider->velocityCollider.id, velocityCliffCollider.id);
                physics.Record(collider->velocityCliffCollider.id, velocityCollider.id);
                physics.Record(collider->velocityCliffCollider.id, velocityCliffCollider.id);
            }
        }
    }
    if (useVelocityCollider) {
        physics.Record(velocityCollider.id, velocityCliffCollider.id);
        physics.Record(velocityCliffCollider.id, velocityCollider.id);
        if (useGravityCollider) {
            physics.Record(velocityCollider.id, gravityCollider.id);
            physics.Record(velocityCliffCollider.id, gravityCollider.id);
        }
    }
    colliderList.push_back(this);
}

// PQS_ColliderAdvanced.cs:133 (List.Remove: the first occurrence)
void PQS_ColliderAdvanced::UnregisterColliders() {
    const auto it = std::find(colliderList.begin(), colliderList.end(), this);
    if (it != colliderList.end()) colliderList.erase(it);
}

// PQS_ColliderAdvanced.cs:138
void PQS_ColliderAdvanced::ActivateGravityCollider() {
    if (!isGravityColliderActive) {
        isGravityColliderActive = true;
        PhysicsIgnoreCollisions().Record(owner.colliderId, gravityCollider.id, false);
    }
}

// PQS_ColliderAdvanced.cs:147
void PQS_ColliderAdvanced::DeactivateGravityCollider() {
    if (isGravityColliderActive) {
        isGravityColliderActive = false;
        PhysicsIgnoreCollisions().Record(owner.colliderId, gravityCollider.id, true);
    }
}

// PQS_ColliderAdvanced.cs:156
void PQS_ColliderAdvanced::ActivateVelocityCollider() {
    if (!isVelocityColliderActive) {
        isVelocityColliderActive = true;
        PhysicsIgnoreCollisions().Record(owner.colliderId, velocityCollider.id, false);
        PhysicsIgnoreCollisions().Record(owner.colliderId, velocityCliffCollider.id, false);
    }
}

// PQS_ColliderAdvanced.cs:166
void PQS_ColliderAdvanced::DeactivateVelocityCollider() {
    if (isVelocityColliderActive) {
        isVelocityColliderActive = false;
        PhysicsIgnoreCollisions().Record(owner.colliderId, velocityCollider.id, true);
        PhysicsIgnoreCollisions().Record(owner.colliderId, velocityCliffCollider.id, true);
    }
}

// PQS_ColliderAdvanced.cs:176 (the debug cubes of collidersVisibleDEBUG are renderers: not modelled)
void PQS_ColliderAdvanced::Start() {
    if (sampleRotationsD.empty()) {
        sampleRotationsD = {QuaternionD::AngleAxis(sampleAngle, Vector3d(Vector3f::right())),
                            QuaternionD::AngleAxis(sampleAngle, Vector3d(Vector3f::left())),
                            QuaternionD::AngleAxis(sampleAngle, Vector3d(Vector3f::forward()))};
    }
    if (useGravityCollider) {
        gravityCollider = NewBox(owner.name + "ColGrav", Vector3f(colliderSize.x, f32store(fx(halfColliderHeight) * 2.0), colliderSize.y));
        hasGravityCollider = true;
        isGravityColliderActive = true;
    }
    if (useVelocityCollider) {
        velocityCollider = NewBox(owner.name + "ColVel", Vector3f(colliderSize.x, f32store(fx(halfColliderHeight) * 2.0), colliderSize.y));
        velocityCliffCollider = NewBox(owner.name + "ColVelCliff", Vector3f(colliderSize.x, f32store(fx(halfCliffHeight) * 2.0), colliderSize.y));
        hasVelocityCollider = true;
        isVelocityColliderActive = true;
    }
    debugSamples[0].assign(3, Vector3d::zero());
    debugSamples[1].assign(3, Vector3d::zero());
    RegisterColliders();
    for (Vector3d& s : samplesD) s = Vector3d::zero();
    started_ = true;
    SetTargetSphere(sphere);
}

// PQS_ColliderAdvanced.cs:257
void PQS_ColliderAdvanced::OnDestroy() {
    UnregisterColliders();
    hasGravityCollider = false;
    hasVelocityCollider = false;
    started_ = false;
}

// PQS_ColliderAdvanced.cs:274 (the C# dereferences the collider objects: before Start that is a NullReferenceException)
void PQS_ColliderAdvanced::SetTargetSphere(PQS* s) {
    sphere = s;
    if ((useGravityCollider && !hasGravityCollider) || (useVelocityCollider && !hasVelocityCollider)) throw NullReferenceException();
    if (sphere == nullptr) {
        if (useGravityCollider) {
            gravityCollider.parent = PQSBoxCollider::Parent::Owner;
            gravityCollider.localPosition = Vector3f::zero();
            DeactivateGravityCollider();
        }
        if (useVelocityCollider) {
            velocityCollider.parent = PQSBoxCollider::Parent::Owner;
            velocityCollider.localPosition = Vector3f::zero();
            velocityCliffCollider.parent = PQSBoxCollider::Parent::Owner;
            velocityCliffCollider.localPosition = Vector3f::zero();
            DeactivateVelocityCollider();
        }
    } else {
        if (useGravityCollider) {
            gravityCollider.parent = PQSBoxCollider::Parent::Sphere;
            gravityCollider.localPosition = Vector3f::zero();
        }
        if (useVelocityCollider) {
            velocityCollider.parent = PQSBoxCollider::Parent::Sphere;
            velocityCollider.localPosition = Vector3f::zero();
            velocityCliffCollider.parent = PQSBoxCollider::Parent::Sphere;
            velocityCliffCollider.localPosition = Vector3f::zero();
        }
        UpdateColliders();
    }
}

// PQS_ColliderAdvanced.cs:312
void PQS_ColliderAdvanced::FixedUpdate() { UpdateColliders(); }

// PQS_ColliderAdvanced.cs:317 (relPos / relVel are float Vector3 fields: the Vector3d results are narrowed)
void PQS_ColliderAdvanced::UpdateColliders() {
    if (sphere == nullptr) return;
    relPos = sphere->GetRelativePosition(Vector3d(OwnerPosition(owner))).ToVector3f();
    relVel = sphere->GetRelativeDirection(Vector3d(OwnerVelocity(owner))).ToVector3f();
    if (useGravityCollider) UpdateGravityCollider();
    if (useVelocityCollider) UpdateVelocityCollider();
}

// PQS_ColliderAdvanced.cs:334 (sic: GetSurfaceHeight is evaluated twice; `altitude + verticalSpeed < sphere.radiusMax` compares an
// altitude with a radius)
void PQS_ColliderAdvanced::UpdateGravityCollider() {
    relDir = Vector3d(relPos.normalized());
    radDist = static_cast<double>(relPos.magnitude());
    surfaceHeightValue = Height(Vector3d(relPos));
    altitude = radDist - Height(Vector3d(relPos));
    if (altitude < -0.25) {
        ActivateGravityCollider();
        UpdateVerticalCollider(gravityCollider, surfaceHeightValue - static_cast<double>(halfColliderHeight), relDir);
        SetOwnerPosition(owner, (relDir * (surfaceHeightValue + 1.0)).ToVector3f());
        SetOwnerVelocity(owner, OwnerVelocity(owner) * 0.8f);
        return;
    }
    radSpeed = static_cast<double>((relPos + relVel).magnitude()) - radDist;
    if (radDist > sphere->radiusMax && radDist + radSpeed > sphere->radiusMax) {
        DeactivateGravityCollider();
        return;
    }
    planeVelocity = Vector3d(UnityNative::FromToRotation(relDir.ToVector3f(), Vector3d::up().ToVector3f()) * relVel);
    verticalSpeed = planeVelocity.y;
    if (verticalSpeed < 100.0 && altitude + verticalSpeed < sphere->radiusMax) {
        ActivateGravityCollider();
        UpdateNormalCollider(gravityCollider, static_cast<double>(halfColliderHeight), relDir);
        ShowDebugNormals(0, Vector3d(relPos));
    } else {
        DeactivateGravityCollider();
    }
}

// PQS_ColliderAdvanced.cs:368 (speed = relVel.magnitude * velocityThinkAhead: a float product widened by conv.r8, Mono keeps it in
// double; the cliff depth `(float)surfHeight - halfCliffHeight - 1f` likewise reaches the double parameter unrounded)
void PQS_ColliderAdvanced::UpdateVelocityCollider() {
    vel = Vector3d(relVel.normalized());
    speed = fx(relVel.magnitude()) * fx(velocityThinkAhead);
    hitPoint = Vector3d(Vector3f::zero());
    surfHeight = 0.0;
    if (speed > 0.25 && IterateVelocity(Vector3d(relPos), vel, speed, 0, maxVelocityIteration)) {
        ActivateVelocityCollider();
        ShowDebugNormals(1, hitPoint);
        hitPoint = hitPoint.normalized();
        UpdateNormalCollider(velocityCollider, static_cast<double>(halfColliderHeight), hitPoint);
        UpdateVerticalCollider(velocityCliffCollider, fx(static_cast<float>(surfHeight)) - fx(halfCliffHeight) - 1.0, hitPoint);
    } else {
        DeactivateVelocityCollider();
    }
}

// PQS_ColliderAdvanced.cs:388 (0.1f as a double literal)
bool PQS_ColliderAdvanced::IterateVelocity(const Vector3d& start, const Vector3d& v, double length, int itr, int maxItr) {
    hitPoint = start + v * length;
    surfHeight = Height(hitPoint);
    if (hitPoint.magnitude() <= surfHeight + 0.10000000149011612) {
        if (itr < maxItr) return IterateVelocity(start, v, length / 2.0, itr + 1, maxItr);
        return true;
    }
    return false;
}

// PQS_ColliderAdvanced.cs:403
void PQS_ColliderAdvanced::UpdateVerticalCollider(PQSBoxCollider& col, double sh, const Vector3d& radianNormalized) {
    col.localPosition = (radianNormalized * sh).ToVector3f();
    SetUp(col, radianNormalized.ToVector3f());
}

// PQS_ColliderAdvanced.cs:409
void PQS_ColliderAdvanced::UpdateNormalCollider(PQSBoxCollider& col, double depth, const Vector3d& radianNormalized) {
    normal = GetNormalAtPoint(radianNormalized);
    col.localPosition = (radianNormalized * Height(radianNormalized) - normal * depth).ToVector3f();
    SetUp(col, normal.ToVector3f());
}

// PQS_ColliderAdvanced.cs:416 (the cubes' positions; their rotations / renderers are debug drawing)
void PQS_ColliderAdvanced::ShowDebugNormals(int set, const Vector3d& radPos) {
    if (!showNormalsDEBUG) return;
    const Vector3d normalized = radPos.normalized();
    for (int i = 0; i < 3; i++) {
        samplesD[i] = sampleRotationsD[static_cast<size_t>(i)] * normalized;
        samplesD[i] = samplesD[i] * Height(samplesD[i]);
        const Vector3d normalized2 = samplesD[i].normalized();
        debugSamples[set][static_cast<size_t>(i)] = samplesD[i] + normalized2;
    }
}

// PQS_ColliderAdvanced.cs:432
Vector3d PQS_ColliderAdvanced::GetNormalAtPoint(const Vector3d& radian) {
    for (int i = 0; i < 3; i++) {
        samplesD[i] = sampleRotationsD[static_cast<size_t>(i)] * radian;
        samplesD[i] = samplesD[i] * Height(samplesD[i]);
    }
    edgeABd = samplesD[1] - samplesD[0];
    edgeACd = samplesD[2] - samplesD[0];
    if (radian.y >= 0.0) {
        return Vector3d(edgeABd.y * edgeACd.z - edgeABd.z * edgeACd.y, edgeABd.z * edgeACd.x - edgeABd.x * edgeACd.z,
                        edgeABd.x * edgeACd.y - edgeABd.y * edgeACd.x)
            .normalized();
    }
    return Vector3d(edgeABd.z * edgeACd.y - edgeABd.y * edgeACd.z, edgeABd.x * edgeACd.z - edgeABd.z * edgeACd.x,
                    edgeABd.y * edgeACd.x - edgeABd.x * edgeACd.y)
        .normalized();
}

// PQS_ColliderAdvanced.cs:448
void PQS_ColliderAdvanced::OnReferenceBodyChange(CelestialBody& body) { SetTargetSphere(body.pqsController); }

// ---- PQS_PartCollider.cs -------------------------------------------------------------------------------------------------------

PQS_PartCollider::PQS_PartCollider() = default;
PQS_PartCollider::~PQS_PartCollider() {
    if (started_) OnDestroy();
}

void PQS_PartCollider::ClearStatics() {
    sampleRotationsD.clear();
    colliderList.clear();
    colliderListCreated = false;
    edgeABd = edgeACd = normal = Vector3d::zero();
}

double PQS_PartCollider::Height(const Vector3d& v) const { return surfaceHeight ? surfaceHeight(v) : sphere->GetSurfaceHeight(v); }

void PQS_PartCollider::BindPart(Part* p) {
    partPresent = p != nullptr;
    if (p == nullptr) {
        part = PartHooks();
        return;
    }
    part.packed = [p] { return p->fields.packed; };
    part.hasVessel = [p] { return p->vessel != nullptr; };
    part.vesselLanded = [p] { return p->vessel->Landed; };
    part.referenceBodySphere = [p]() -> PQS* {
        if (p->vessel == nullptr || !p->vessel->orbitDriver) throw NullReferenceException();
        CelestialBody* body = p->vessel->orbitDriver->referenceBody();
        if (body == nullptr) throw NullReferenceException();
        return body->pqsController;
    };
}

// PQS_PartCollider.cs:95
void PQS_PartCollider::Reset() {
    colliderSize = Vector2f(1.0f, 1.0f);
    useVelocityCollider = true;
    useGravityCollider = true;
}

// PQS_PartCollider.cs:102
void PQS_PartCollider::RegisterColliders() {
    if (!colliderListCreated) {
        colliderList.clear();
        colliderListCreated = true;
    }
    IgnoreCollisionLog& physics = PhysicsIgnoreCollisions();
    for (PQS_PartCollider* collider2 : colliderList) {
        if (!collider2->collidersSet) throw NullReferenceException();   // foreach over a null `colliders`
        if (useGravityCollider) {
            for (int c : collider2->colliders) physics.Record(c, gravityCollider.id);
            if (collider2->useGravityCollider) physics.Record(collider2->gravityCollider.id, gravityCollider.id);
        }
        if (useVelocityCollider) {
            for (int c : collider2->colliders) {
                physics.Record(c, velocityCollider.id);
                physics.Record(c, velocityCliffCollider.id);
            }
            if (collider2->useVelocityCollider) {
                physics.Record(collider2->velocityCollider.id, velocityCollider.id);
                physics.Record(collider2->velocityCollider.id, velocityCliffCollider.id);
                physics.Record(collider2->velocityCliffCollider.id, velocityCollider.id);
                physics.Record(collider2->velocityCliffCollider.id, velocityCliffCollider.id);
            }
        }
    }
    if (useVelocityCollider) {
        physics.Record(velocityCollider.id, velocityCliffCollider.id);
        physics.Record(velocityCliffCollider.id, velocityCollider.id);
        if (useGravityCollider) {
            physics.Record(velocityCollider.id, gravityCollider.id);
            physics.Record(velocityCliffCollider.id, gravityCollider.id);
        }
    }
    colliderList.push_back(this);
}

// PQS_PartCollider.cs:152
void PQS_PartCollider::UnregisterColliders() {
    const auto it = std::find(colliderList.begin(), colliderList.end(), this);
    if (it != colliderList.end()) colliderList.erase(it);
    colliders.clear();
    collidersSet = false;
}

// PQS_PartCollider.cs:158 (a collider id 0 is a null Collider)
void PQS_PartCollider::ActivateGravityCollider() {
    if (isGravityColliderActive) return;
    isGravityColliderActive = true;
    for (int c : colliders)
        if (c != 0) PhysicsIgnoreCollisions().Record(c, gravityCollider.id, false);
}

// PQS_PartCollider.cs:175
void PQS_PartCollider::DeactivateGravityCollider() {
    if (!isGravityColliderActive) return;
    isGravityColliderActive = false;
    for (int c : colliders)
        if (c != 0) PhysicsIgnoreCollisions().Record(c, gravityCollider.id, true);
}

// PQS_PartCollider.cs:192
void PQS_PartCollider::ActivateVelocityCollider() {
    if (isVelocityColliderActive) return;
    isVelocityColliderActive = true;
    for (int c : colliders) {
        if (c != 0) {
            PhysicsIgnoreCollisions().Record(c, velocityCollider.id, false);
            PhysicsIgnoreCollisions().Record(c, velocityCliffCollider.id, false);
        }
    }
}

// PQS_PartCollider.cs:210
void PQS_PartCollider::DeactivateVelocityCollider() {
    if (!isVelocityColliderActive) return;
    isVelocityColliderActive = false;
    for (int c : colliders) {
        if (c != 0) {
            PhysicsIgnoreCollisions().Record(c, velocityCollider.id, true);
            PhysicsIgnoreCollisions().Record(c, velocityCliffCollider.id, true);
        }
    }
}

// PQS_PartCollider.cs:228
void PQS_PartCollider::Awake() {
    if (sampleRotationsD.empty()) {
        sampleRotationsD = {QuaternionD::AngleAxis(sampleAngle, Vector3d::right()), QuaternionD::AngleAxis(sampleAngle, Vector3d::left()),
                            QuaternionD::AngleAxis(sampleAngle, Vector3d::forward())};
    }
    samplesD.assign(3, Vector3d::zero());
    colliders.clear();
    collidersSet = false;
}

// PQS_PartCollider.cs:243 (sic: the cliff box takes halfColliderHeight, not halfCliffHeight)
void PQS_PartCollider::Start() {
    if (useGravityCollider) {
        gravityCollider = NewBox(owner.name + "ColGrav", Vector3f(colliderSize.x, static_cast<float>(halfColliderHeight * 2.0), colliderSize.y));
        gravityCollider.layer = layerID;
        hasGravityCollider = true;
        isGravityColliderActive = true;
    }
    if (useVelocityCollider) {
        velocityCollider = NewBox(owner.name + "ColVel", Vector3f(colliderSize.x, static_cast<float>(halfColliderHeight * 2.0), colliderSize.y));
        velocityCollider.layer = layerID;
        velocityCliffCollider = NewBox(owner.name + "ColVelCliff", Vector3f(colliderSize.x, static_cast<float>(halfColliderHeight * 2.0), colliderSize.y));
        velocityCliffCollider.layer = layerID;
        hasVelocityCollider = true;
        isVelocityColliderActive = true;
    }
    colliders = owner.colliderIds;
    collidersSet = true;
    RegisterColliders();
    started_ = true;
    SetTargetSphere(sphere);
    if (partPresent) UpdateColliders();
}

// PQS_PartCollider.cs:274
void PQS_PartCollider::OnDestroy() {
    UnregisterColliders();
    hasGravityCollider = false;
    hasVelocityCollider = false;
    started_ = false;
}

// PQS_PartCollider.cs:291
void PQS_PartCollider::SetTargetSphere(PQS* s) {
    sphere = s;
    if (!collidersSet) return;
    if (sphere == nullptr) {
        if (useGravityCollider) {
            gravityCollider.position = safeSpot;
            DeactivateGravityCollider();
        }
        if (useVelocityCollider) {
            velocityCollider.position = safeSpot;
            velocityCliffCollider.position = safeSpot;
            DeactivateVelocityCollider();
        }
    } else {
        if (useGravityCollider) gravityCollider.position = safeSpot;
        if (useVelocityCollider) {
            velocityCollider.position = safeSpot;
            velocityCliffCollider.position = safeSpot;
        }
    }
}

// PQS_PartCollider.cs:326 (part.vessel.orbitDriver.referenceBody is dereferenced without a null check, as in the C#)
void PQS_PartCollider::FixedUpdate() {
    if (!partPresent) {
        if (!HasRigidbody(owner) || IsKinematic(owner)) return;
    } else {
        const bool packed = part.packed && part.packed();
        const bool hasVessel = part.hasVessel && part.hasVessel();
        if (packed || !HasRigidbody(owner) || IsKinematic(owner) || (hasVessel && part.vesselLanded && part.vesselLanded())) return;
        if (!hasVessel || !part.referenceBodySphere) throw NullReferenceException();
        PQS* referenceSphere = part.referenceBodySphere();
        if (referenceSphere != sphere) SetTargetSphere(referenceSphere);
    }
    UpdateColliders();
}

// PQS_PartCollider.cs:349
void PQS_PartCollider::UpdateColliders() {
    if (sphere == nullptr) return;
    relPos = sphere->GetRelativePosition(Vector3d(OwnerPosition(owner)));
    relVel = sphere->GetRelativeDirection(Vector3d(OwnerVelocity(owner)));
    if (useGravityCollider) UpdateGravityCollider();
    if (useVelocityCollider) UpdateVelocityCollider();
}

// PQS_PartCollider.cs:366 (q * relVel: Unity's Quaternion * Vector3, relVel narrowed: C# picks the operator of an operand type)
void PQS_PartCollider::UpdateGravityCollider() {
    relDir = relPos.normalized();
    radDist = relPos.magnitude();
    surfaceHeightValue = Height(relPos);
    altitude = radDist - surfaceHeightValue;
    if (altitude < -0.25) {
        ActivateGravityCollider();
        UpdateVerticalCollider(gravityCollider, surfaceHeightValue - halfColliderHeight, relDir);
        SetOwnerPosition(owner, sphere->GetWorldPosition(relDir * (surfaceHeightValue + 1.0)).ToVector3f());
        SetOwnerVelocity(owner, OwnerVelocity(owner) * 0.8f);
        return;
    }
    radSpeed = (relPos + relVel).magnitude() - radDist;
    if (radDist > sphere->radiusMax && radDist + radSpeed > sphere->radiusMax) {
        DeactivateGravityCollider();
        return;
    }
    q = UnityNative::FromToRotation(relDir.ToVector3f(), Vector3d::up().ToVector3f());
    planeVelocity = Vector3d(q * relVel.ToVector3f());
    verticalSpeed = planeVelocity.y;
    if (verticalSpeed < 0.0 && altitude + verticalSpeed < sphere->radiusMax) {
        ActivateGravityCollider();
        UpdateNormalCollider(gravityCollider, halfColliderHeight, relDir);
    } else {
        DeactivateGravityCollider();
    }
}

// PQS_PartCollider.cs:400
void PQS_PartCollider::UpdateVelocityCollider() {
    relVelNorm = relVel.normalized();
    speed = relVel.magnitude() * velocityThinkAhead;
    hitPoint = Vector3d(Vector3f::zero());
    surfHeight = 0.0;
    itr = 0;
    if (speed > 0.25 && IterateVelocity()) {
        ActivateVelocityCollider();
        hitPoint = hitPoint.normalized();
        UpdateNormalCollider(velocityCollider, halfColliderHeight, hitPoint);
        UpdateVerticalCollider(velocityCliffCollider, surfHeight - halfCliffHeight - 1.0, hitPoint);
    } else {
        DeactivateVelocityCollider();
    }
}

// PQS_PartCollider.cs:420 (speed is halved in place)
bool PQS_PartCollider::IterateVelocity() {
    hitPoint = relPos + relVelNorm * speed;
    surfHeight = Height(hitPoint);
    if (hitPoint.magnitude() <= surfHeight + 0.1) {
        if (itr < maxVelocityIteration) {
            itr++;
            speed *= 0.5;
            return IterateVelocity();
        }
        return true;
    }
    return false;
}

// PQS_PartCollider.cs:437
void PQS_PartCollider::UpdateVerticalCollider(PQSBoxCollider& col, double sh, const Vector3d& radianNormalized) {
    col.position = sphere->GetWorldPosition(radianNormalized * sh).ToVector3f();
    SetUp(col, radianNormalized.ToVector3f());
}

// PQS_PartCollider.cs:443
void PQS_PartCollider::UpdateNormalCollider(PQSBoxCollider& col, double depth, const Vector3d& radianNormalized) {
    GetNormalAtPoint(radianNormalized);
    col.position = sphere->GetWorldPosition(radianNormalized * Height(radianNormalized) - normal * depth).ToVector3f();
    SetUp(col, normal.ToVector3f());
}

// PQS_PartCollider.cs:450 (the loop counter is the `itr` field)
void PQS_PartCollider::GetNormalAtPoint(const Vector3d& radian) {
    for (itr = 0; itr < 3; itr++) {
        samplesD[static_cast<size_t>(itr)] = sampleRotationsD[static_cast<size_t>(itr)] * radian;
        samplesD[static_cast<size_t>(itr)] = samplesD[static_cast<size_t>(itr)] * Height(samplesD[static_cast<size_t>(itr)]);
    }
    edgeABd = samplesD[1] - samplesD[0];
    edgeACd = samplesD[2] - samplesD[0];
    if (radian.y >= 0.0) {
        normal = Vector3d(edgeABd.y * edgeACd.z - edgeABd.z * edgeACd.y, edgeABd.z * edgeACd.x - edgeABd.x * edgeACd.z,
                          edgeABd.x * edgeACd.y - edgeABd.y * edgeACd.x)
                     .normalized();
    } else {
        normal = Vector3d(edgeABd.z * edgeACd.y - edgeABd.y * edgeACd.z, edgeABd.x * edgeACd.z - edgeABd.z * edgeACd.x,
                          edgeABd.y * edgeACd.x - edgeABd.x * edgeACd.y)
                     .normalized();
    }
}

// PQS_PartCollider.cs:469
void PQS_PartCollider::OnReferenceBodyChange(CelestialBody& body) { SetTargetSphere(body.pqsController); }

// ---- PQSMod_CreateSphereCollider.cs --------------------------------------------------------------------------------------------

// PQSMod_CreateSphereCollider.cs:15 (hideFlags: editor only)
void PQSMod_CreateSphereCollider::OnSetup() {
    hasSphereCollider = true;   // GetComponent<SphereCollider>() ?? AddComponent<SphereCollider>()
    sphereColliderRadius = static_cast<float>(sphere->radius + static_cast<double>(radiusOffset));
}

void PQSMod_CreateSphereCollider::VisitSerialized(const SerializedFieldVisitor& v) const {
    PQSMod::VisitSerialized(v);
    v("radiusOffset", SerializedValue::Of(radiusOffset));
}

}  // namespace ksp
