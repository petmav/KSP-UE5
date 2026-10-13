// WP-20e: the contact host of the physics bubble (see ksp/physics/contact_host.hpp). C# paths are relative to
// decompiled/KSP-Decompiled/Assembly-CSharp/. Engine glue: the KSP statements themselves are in src/parts/part_contact.cpp,
// src/vessel/vessel_ground.cpp, src/physics/part_buoyancy.cpp and src/physics/collision_enhancer.cpp.
#include "ksp/physics/contact_host.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <unordered_set>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "ksp/physics/part_forces.hpp"
#include "ksp/physics/physics_bubble.hpp"
#include "ksp/terrain/pqs.hpp"
#include "ksp/unity/matrix_native.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::physics {

using GE = events::GameEvents;
using Kind = PhysicsWorld::ContactEvent::Kind;
using UnityEngine::Collision;
using UnityEngine::ColliderRef;
using UnityEngine::ContactPoint;

namespace {
// the collider of a part as a dictionary key (static keys stay below 2^63)
uint64_t PartColliderId(const Part* p) { return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(p)) | (1ull << 63); }

bool Alive(const Part* p) {
    if (p == nullptr) return false;
    return std::find(Part::allParts.begin(), Part::allParts.end(), p) != Part::allParts.end();
}

Part* OwnerOf(Part* p) {
    Part* o = RigidBodyPart(p);
    return o != nullptr ? o : p;
}
}  // namespace

ContactHost::ContactHost(PhysicsBubble& bubble, const ContactHostOptions& opts) : options(opts), bubble_(&bubble) {}

ContactHost::~ContactHost() {
    Uninstall();
    terrain_.clear();
}

// ===================================================================================== installation

void ContactHost::Install() {
    if (installed_) return;
    PhysicsWorld& world = bubble_->World();
    VesselEnvironment& env = Vessel::env;
    saved_ = env;
    world.SetContactEventTracking(options.collisions, options.contactOffset);
    if (options.groundProbes) {
        // Vessel.cs:2652 GetHeightFromTerrain: Physics.Raycast(vesselTransform.position, -up, out hit, num, 32768): the static layer
        env.heightFromTerrainProbe = [this](Vessel& v, float maxDistance, Vector3f& hitNormal) -> std::optional<float> {
            if (v.mainBody() == nullptr) return std::nullopt;
            const Vector3f pos = v.vesselTransform.position;
            const Vector3f up = FlightMath::getUpAxis(*v.mainBody(), Vector3d(pos)).ToVector3f();
            const PhysicsWorld::RayHit h = bubble_->World().RayCast(bubble_->StaticPhysicsFromWorld(Vector3d(pos)), Vector3d(-up),
                                                                    static_cast<double>(maxDistance), PhysicsWorld::kLayerStatic);
            if (!h.hit) return std::nullopt;
            hitNormal = h.normal.ToVector3f();
            v.objectUnderVesselTag = StaticInfo(h.userData).tag;
            return static_cast<float>(h.distance);
        };
        // Vessel.cs:2509: Physics.Raycast(.., 0x8000 | LayerUtil.DefaultEquivalent): terrain and parts
        env.groundRaycast = [this](Vessel&, const Vector3f& origin, const Vector3f& direction, float maxDistance) {
            VesselRayHit out;
            const PhysicsWorld::RayHit h = bubble_->World().RayCast(bubble_->StaticPhysicsFromWorld(Vector3d(origin)), Vector3d(direction),
                                                                    static_cast<double>(maxDistance), PhysicsWorld::kLayerAll);
            if (!h.hit) return out;
            out.hit = true;
            out.distance = static_cast<float>(h.distance);
            out.point = (h.point + bubble_->FrameOriginWorld()).ToVector3f();
            out.normal = h.normal.ToVector3f();
            if (bubble_->World().IsStatic(h.body)) {
                const StaticColliderInfo& info = StaticInfo(h.userData);
                out.tag = info.tag;
                out.isMobileLaunchPad = info.isMobileLaunchPad;
            } else {
                int cl = -1;
                out.isPart = BindingOfBody(h.body, cl) != nullptr;
            }
            return out;
        };
        // Vessel.cs:2596-2614: collider.ClosestPoint(measure).z of every proxy collider of the part, in the part's current pose
        env.colliderClosestPointsZ = [this](Part& p, const Vector3f& point) {
            std::vector<float> zs;
            const auto& fn = bubble_->Options().vessel.colliders;
            const std::vector<ColliderProxy> boxes = fn ? fn(p) : DragCubeColliderProxy(p);
            const QuaternionD pr(p.partTransform.rotation);
            const Vector3d pp(p.partTransform.position);
            for (const ColliderProxy& b : boxes) {
                const QuaternionD r = pr * QuaternionD(b.rotation);
                const Vector3d c = pp + pr * Vector3d(b.center);
                Vector3d local = QuaternionD::Inverse(r) * (Vector3d(point) - c);
                local.x = std::max(-static_cast<double>(b.halfExtents.x), std::min(static_cast<double>(b.halfExtents.x), local.x));
                local.y = std::max(-static_cast<double>(b.halfExtents.y), std::min(static_cast<double>(b.halfExtents.y), local.y));
                local.z = std::max(-static_cast<double>(b.halfExtents.z), std::min(static_cast<double>(b.halfExtents.z), local.z));
                zs.push_back(static_cast<float>((c + r * local).z));
            }
            return zs;
        };
        // Vessel.GoOffRails' landed placement. kspcore builds the bodies of an unpacking vessel at its rails state (CoMD - rotation * localCoM,
        // VesselPhysicsHost::DefaultUnpackPose); KSP's bodies are the transforms, which CheckGroundCollision moved: CoMD follows the transform.
        env.checkGroundCollision = [](Vessel& v) {
            const Vector3f p0 = v.vesselTransform.position;
            const Quaternionf r0 = v.vesselTransform.rotation;
            const bool ok = v.CheckGroundCollision();
            if (!(v.vesselTransform.position.Equals(p0)) || !(v.vesselTransform.rotation.x == r0.x && v.vesselTransform.rotation.y == r0.y &&
                                                              v.vesselTransform.rotation.z == r0.z && v.vesselTransform.rotation.w == r0.w))
                v.CoMD = Vector3d(v.vesselTransform.position) + QuaternionD(v.vesselTransform.rotation) * Vector3d(v.localCoM);
            return ok;
        };
    }
    if (options.rbAnchor) {
        env.rbAnchorCreate = [this](Vessel& v, float breakForce, float breakTorque) {
            VesselPhysics* vp = BindingOf(v);
            if (vp == nullptr || v.rootPart == nullptr) return false;
            const int cl = vp->ClusterOf(*OwnerOf(v.rootPart));
            if (cl < 0) return false;
            const BodyId body = vp->ClusterBody(static_cast<size_t>(cl));
            if (body == kInvalidBody) return false;
            // the anchor stops the vessel (v^2 < VESSEL_ANCHOR_VELOCITY_THRESHOLD: its jitter) at once, without a joint load: PhysX spread that
            // small impulse through the PartJoint springs; on the reduced model (no joint compliance, WP-20c's quasi-static loads) a one-step stop
            // of a 680 t vessel rocking at 3 cm/s read as kN m of bending and broke L2-AS's radial decouplers
            vp->StopMotion();
            const ConstraintId id = bubble_->World().CreateFixedToWorldConstraint(body, vp->ClusterCenterOfMass(static_cast<size_t>(cl)));
            if (id == kInvalidConstraint) return false;
            Anchor a;
            a.id = id;
            a.body = body;
            a.breakForce = breakForce;
            a.breakTorque = breakTorque;
            anchors_[&v] = a;
            return true;
        };
        env.rbAnchorDestroy = [this](Vessel& v) {
            auto it = anchors_.find(&v);
            if (it == anchors_.end()) return;
            bubble_->World().RemoveConstraint(it->second.id);
            anchors_.erase(it);
        };
        env.rbAnchorCurrentForce = [this](Vessel& v) { return AnchorForce(v); };
        env.rbAnchorSetBreakForce = [this](Vessel& v, float f) {
            auto it = anchors_.find(&v);
            if (it != anchors_.end()) it->second.breakForce = f;
        };
    }
    PartContactEnvironment::rigidbodyVelocity = [this](Part& p) {
        Part* o = OwnerOf(&p);
        VesselPhysics* vp = o->vessel != nullptr ? BindingOf(*o->vessel) : nullptr;
        return vp != nullptr ? vp->RigidbodyState(*o).velocity : Vector3f();
    };
    GE::onRotatingFrameTransition.Add(this, &ContactHost::OnRotatingFrameTransition);
    GE::onDominantBodyChange.Add(this, &ContactHost::OnDominantBodyChange);
    installed_ = true;
}

void ContactHost::Uninstall() {
    if (!installed_) return;
    GE::onRotatingFrameTransition.Remove(this, &ContactHost::OnRotatingFrameTransition);
    GE::onDominantBodyChange.Remove(this, &ContactHost::OnDominantBodyChange);
    VesselEnvironment& env = Vessel::env;
    env.heightFromTerrainProbe = saved_.heightFromTerrainProbe;
    env.groundRaycast = saved_.groundRaycast;
    env.colliderClosestPointsZ = saved_.colliderClosestPointsZ;
    env.checkGroundCollision = saved_.checkGroundCollision;
    env.rbAnchorCreate = saved_.rbAnchorCreate;
    env.rbAnchorDestroy = saved_.rbAnchorDestroy;
    env.rbAnchorCurrentForce = saved_.rbAnchorCurrentForce;
    env.rbAnchorSetBreakForce = saved_.rbAnchorSetBreakForce;
    PartContactEnvironment::rigidbodyVelocity = nullptr;
    for (auto& [v, a] : anchors_) bubble_->World().RemoveConstraint(a.id);
    anchors_.clear();
    bubble_->World().SetContactEventTracking(false);
    pairs_.clear();
    buoyancy_.clear();
    enhancers_.clear();
    installed_ = false;
}

// ===================================================================================== static colliders

void ContactHost::RegisterStatic(uint64_t key, const StaticColliderInfo& info) { statics_[key] = info; }

void ContactHost::RegisterStaticRange(uint64_t keyBase, uint64_t count, const StaticColliderInfo& info) {
    StaticRange r;
    r.base = keyBase;
    r.count = count;
    r.info = info;
    ranges_.push_back(r);
}

void ContactHost::UnregisterStatic(uint64_t key) { statics_.erase(key); }

const StaticColliderInfo& ContactHost::StaticInfo(uint64_t key) const {
    const auto it = statics_.find(key);
    if (it != statics_.end()) return it->second;
    for (const StaticRange& r : ranges_)
        if (key >= r.base && key - r.base < r.count) return r.info;
    return defaultInfo_;
}

// ===================================================================================== bindings

VesselPhysics* ContactHost::BindingOf(const Vessel& v) const { return bubble_->Host().Find(v); }

VesselPhysics* ContactHost::BindingOfBody(BodyId body, int& cluster) const {
    for (const auto& vp : bubble_->Host().Bindings()) {
        const int c = vp->ClusterOfBody(body);
        if (c >= 0) {
            cluster = c;
            return vp.get();
        }
    }
    cluster = -1;
    return nullptr;
}

Vector3d ContactHost::PartRbVelocity(VesselPhysics& vp, Part& part) const { return Vector3d(vp.RigidbodyState(part).velocity); }

Vector3f ContactHost::AnchorForce(const Vessel& v) const {
    const auto it = anchors_.find(&v);
    return it != anchors_.end() ? it->second.currentForce : Vector3f();
}

PartBuoyancy* ContactHost::Buoyancy(const Part& part) const {
    const auto it = buoyancy_.find(&part);
    return it != buoyancy_.end() ? it->second.second.get() : nullptr;
}

CollisionEnhancer* ContactHost::Enhancer(const Part& part) const {
    const auto it = enhancers_.find(&part);
    return it != enhancers_.end() ? it->second.second.get() : nullptr;
}

// ===================================================================================== collisions

// A Collision for the part `thisPart` (its collider touched) against the other side of `e` (mirrored: the event seen from body2)
Collision ContactHost::MakeCollision(const PhysicsWorld::ContactEvent& e, bool mirrored, Part* thisPart, Part* otherPart, VesselPhysics* vpThis,
                                     VesselPhysics* vpOther) const {
    Collision c;
    const bool otherStatic = !mirrored && e.otherStatic;
    if (otherStatic) {
        const StaticColliderInfo& info = StaticInfo(e.other);
        c.collider = ColliderRef{e.other, info.layer};
        c.layer = info.layer;
        c.tag = info.tag;
        c.name = info.name;
        c.isPQ = info.isPQ;
        c.pqBody = info.pqBody;
        c.crashObjectName = info.crashObjectName;
    } else {
        c.collider = ColliderRef{PartColliderId(otherPart), 0};
        c.layer = 0;
        c.tag = "Untagged";
        c.name = otherPart != nullptr ? otherPart->gameObjectName : std::string();
        c.otherPart = otherPart;
        Part* owner = otherPart != nullptr ? RigidBodyPart(otherPart) : nullptr;
        c.otherHasRigidbody = owner != nullptr && vpOther != nullptr;
        c.otherRigidbodyMass = c.otherHasRigidbody ? vpOther->PartMass(*owner) : 0.0f;
    }
    // Collision.relativeVelocity: the two parts' rb velocities (at their centres of mass) at the contact, before the solve
    auto pointVelocity = [](const Vector3d& v, const Vector3d& w, const Vector3d& com, const Vector3d& p) { return v + Vector3d::Cross(w, p - com); };
    const Vector3d vB1 = mirrored ? e.v2 : e.v1, wB1 = mirrored ? e.w2 : e.w1, comB1 = mirrored ? e.com2 : e.com1;
    const Vector3d vB2 = mirrored ? e.v1 : e.v2, wB2 = mirrored ? e.w1 : e.w2, comB2 = mirrored ? e.com1 : e.com2;
    Vector3d vThis, vOther;
    if (vpThis != nullptr && thisPart != nullptr) {
        Part* o = OwnerOf(thisPart);
        const int cl = vpThis->ClusterOf(*o);
        const Vector3d arm = vpThis->PartState(*o).centerOfMass - (cl >= 0 ? vpThis->ClusterCenterOfMass(static_cast<size_t>(cl)) : comB1);
        vThis = pointVelocity(vB1, wB1, Vector3d(), arm);
    }
    if (!otherStatic && vpOther != nullptr && otherPart != nullptr) {
        Part* o = OwnerOf(otherPart);
        const int cl = vpOther->ClusterOf(*o);
        const Vector3d arm = vpOther->PartState(*o).centerOfMass - (cl >= 0 ? vpOther->ClusterCenterOfMass(static_cast<size_t>(cl)) : comB2);
        vOther = pointVelocity(vB2, wB2, Vector3d(), arm);
    } else if (!otherStatic) {
        vOther = vB2;  // a body that is no vessel's (a physical object): its centre-of-mass velocity
    }
    c.relativeVelocity = (vThis - vOther).ToVector3f();
    ContactPoint cp;
    cp.thisColliderPart = thisPart;
    cp.otherColliderPart = otherStatic ? nullptr : otherPart;
    cp.point = bubble_->WorldFromPhysics(e.point).ToVector3f();
    cp.normal = (mirrored ? -e.normal : e.normal).ToVector3f();
    c.contacts.push_back(cp);
    return c;
}

void ContactHost::DispatchContacts() {
    if (recordDispatch) lastDispatch_.clear();
    const std::vector<PhysicsWorld::ContactEvent>& events = bubble_->World().ContactEvents();
    if (events.empty()) return;
    // new contacts, kept ones, then lost ones (see the header comment)
    for (const Kind pass : {Kind::Enter, Kind::Stay, Kind::Exit}) {
        for (const PhysicsWorld::ContactEvent& e : events) {
            if (e.kind != pass) continue;
            const auto key = std::make_tuple(e.body1, e.sub1, e.body2, e.other);
            Part *c1 = nullptr, *c2 = nullptr;
            VesselPhysics *vp1 = nullptr, *vp2 = nullptr;
            if (pass != Kind::Exit) {
                int cl1 = -1, cl2 = -1;
                vp1 = BindingOfBody(e.body1, cl1);
                if (e.otherStatic) {
                    if (vp1 == nullptr) continue;  // a physical object on the ground: no part
                    c1 = vp1->PartAt(e.sub1);
                } else {
                    vp2 = BindingOfBody(e.body2, cl2);
                    if (vp1 != nullptr) c1 = vp1->PartAt(e.sub1);
                    if (vp2 != nullptr) c2 = vp2->PartAt(static_cast<size_t>(e.other));
                    if (c1 == nullptr && c2 == nullptr) continue;
                }
                PairMemory& m = pairs_[key];
                m.colliderPart1 = c1;
                m.colliderPart2 = c2;
                m.otherKey = e.other;
                m.otherStatic = e.otherStatic;
            } else {
                const auto it = pairs_.find(key);
                if (it == pairs_.end()) continue;
                c1 = it->second.colliderPart1;
                c2 = it->second.colliderPart2;
                pairs_.erase(it);
                // Unity sends no OnCollisionExit for a collider that was destroyed or disabled: a dead part / a removed static collider
                if (c1 != nullptr && !Alive(c1)) c1 = nullptr;
                if (c2 != nullptr && !Alive(c2)) c2 = nullptr;
                if (e.otherStatic && bubble_->World().StaticBody(e.other) == kInvalidBody) continue;
                if (!e.otherStatic && (c1 == nullptr || c2 == nullptr)) continue;
                if (c1 != nullptr && c1->vessel != nullptr) vp1 = BindingOf(*c1->vessel);
                if (c2 != nullptr && c2->vessel != nullptr) vp2 = BindingOf(*c2->vessel);
            }
            auto send = [&](Part* receiver, const Collision& col, Part* colliderPart) {
                if (receiver == nullptr || receiver->vessel == nullptr) return;
                if (recordDispatch) lastDispatch_.push_back(DispatchRecord{pass, receiver, colliderPart, col});
                switch (pass) {
                    case Kind::Enter: ++enters_; receiver->OnCollisionEnter(col); break;
                    case Kind::Stay: ++stays_; receiver->OnCollisionStay(col); break;
                    case Kind::Exit: ++exits_; receiver->OnCollisionExit(col); break;
                }
            };
            if (c1 != nullptr && c1->State() != PartStates::DEAD) send(OwnerOf(c1), MakeCollision(e, false, c1, c2, vp1, vp2), c1);
            if (!e.otherStatic && c2 != nullptr && Alive(c2) && c2->State() != PartStates::DEAD)
                send(OwnerOf(c2), MakeCollision(e, true, c2, c1, vp2, vp1), c2);
        }
    }
}

// ===================================================================================== the RB anchor

// After the step: each anchor's joint.currentForce, then PhysX's break rule (force or torque over the limit -> the joint goes,
// OnJointBreak). An anchor whose body went (a rebind, the vessel packed) is gone: ResetRBAnchor (the vessel anchors again once still).
void ContactHost::UpdateAnchors(float dt) {
    PhysicsWorld& world = bubble_->World();
    std::vector<Vessel*> gone, broke;
    std::vector<float> brokeForce;
    for (auto& [vc, a] : anchors_) {
        Vessel* v = const_cast<Vessel*>(vc);
        const bool registered = std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), v) != FlightGlobals::Vessels.end();
        VesselPhysics* vp = registered ? BindingOf(*v) : nullptr;
        bool valid = vp != nullptr && world.IsValid(a.body) && v->rootPart != nullptr;
        if (valid) {
            const int cl = vp->ClusterOf(*OwnerOf(v->rootPart));
            valid = cl >= 0 && vp->ClusterBody(static_cast<size_t>(cl)) == a.body;
        }
        if (!valid) {
            gone.push_back(v);
            continue;
        }
        // joint.currentForce: PhysX solved the joint before the contacts (warm started), so the anchor carried the vessel's support; Jolt keeps
        // it in the warm-started contacts. The joint's force is therefore taken as the whole support of the root cluster in the step (the
        // anchor's own impulse plus the contacts'): m dv / dt - F_applied (VesselPhysics::ClusterSupportForce)
        const int rc = vp->ClusterOf(*OwnerOf(v->rootPart));
        const Vector3d force = vp->ClusterSupportForce(static_cast<size_t>(rc)), torque = vp->ClusterSupportTorque(static_cast<size_t>(rc));
        (void)dt;
        a.currentForce = force.ToVector3f();
        if (force.magnitude() > static_cast<double>(a.breakForce) || torque.magnitude() > static_cast<double>(a.breakTorque)) {
            broke.push_back(v);
            brokeForce.push_back(static_cast<float>(force.magnitude()));
        }
    }
    for (Vessel* v : gone) {
        auto it = anchors_.find(v);
        if (it != anchors_.end()) {
            world.RemoveConstraint(it->second.id);
            anchors_.erase(it);
        }
        if (std::find(FlightGlobals::Vessels.begin(), FlightGlobals::Vessels.end(), v) != FlightGlobals::Vessels.end()) v->OnJointBreak(0.0f);
    }
    for (size_t i = 0; i < broke.size(); ++i) {
        auto it = anchors_.find(broke[i]);
        if (it == anchors_.end()) continue;
        world.RemoveConstraint(it->second.id);
        anchors_.erase(it);
        broke[i]->OnJointBreak(brokeForce[i]);  // Vessel.cs:1180 (the C#'s OnJointBreak message of the FixedJoint)
    }
}

void ContactHost::AfterPhysicsStep(float dt) {
    UpdateAnchors(dt);
    if (options.collisions) DispatchContacts();
}

// ===================================================================================== PartBuoyancy

PartBuoyancy::Env ContactHost::BuoyancyEnv() {
    PartBuoyancy::Env env;
    auto vpOf = [this](Part& p) -> VesselPhysics* { return p.vessel != nullptr ? BindingOf(*p.vessel) : nullptr; };
    env.hasRigidbody = [vpOf](Part& p) {
        VesselPhysics* vp = vpOf(p);
        if (vp == nullptr) return false;
        const int i = vp->PartIndex(p);
        return i >= 0 && vp->PartHasRigidbody(static_cast<size_t>(i));
    };
    env.rbVelocity = [vpOf](Part& p) {
        VesselPhysics* vp = vpOf(p);
        return vp != nullptr ? vp->RigidbodyState(p).velocity : Vector3f();
    };
    env.rbMass = [vpOf](Part& p) {
        VesselPhysics* vp = vpOf(p);
        return vp != nullptr ? vp->PartMass(p) : 0.0f;
    };
    env.rbDrag = [vpOf](Part& p) {
        VesselPhysics* vp = vpOf(p);
        return vp != nullptr ? vp->Drag(p) : 0.0f;
    };
    env.setRbDrag = [vpOf](Part& p, float d) {
        if (VesselPhysics* vp = vpOf(p)) vp->SetDrag(p, d);
    };
    env.setRbAngularDrag = [vpOf](Part& p, float d) {
        if (VesselPhysics* vp = vpOf(p)) vp->SetAngularDrag(p, d);
    };
    env.localToWorld = [](Part& p) { return UnityNative::TRS(p.partTransform.position, p.partTransform.rotation, p.partTransform.localScale); };
    env.frameVelocity = [] { return Krakensbane::GetFrameVelocity(); };
    env.cameraPosition = [] { return FlightGlobals::camera_position; };
    env.splashFx = [this](const Vector3f& pos, float power) {
        if (splashFx) splashFx(pos, power);
    };
    env.addForceAtPosition = [](Part& p, const Vector3d& f, const Vector3d& pos) { PartAddForceAtPosition(p, f, pos); };
    return env;
}

// seq 13: PartBuoyancy.FixedUpdate of every part with a Rigidbody of every loaded vessel (registry order, vessel part order); a part's
// component is made (Start) the first tick it is seen and dropped with the part
void ContactHost::BuoyancyFixedUpdate() {
    if (!options.buoyancy) return;
    const PartBuoyancy::Env env = BuoyancyEnv();
    std::unordered_set<const Part*> seen;
    const std::vector<Vessel*> vessels = FlightGlobals::Vessels;
    for (Vessel* v : vessels) {
        if (v == nullptr || !v->loaded || v->state == Vessel::State::DEAD) continue;
        const std::vector<Part*> parts = v->parts;
        for (Part* p : parts) {
            if (!p->hasRigidbody || p->State() == PartStates::DEAD || p->vessel != v) continue;
            seen.insert(p);
            auto& slot = buoyancy_[p];
            const uint32_t id = p->fields.flightID;
            if (!slot.second || slot.first != id) {
                slot.first = id;
                slot.second = std::make_unique<PartBuoyancy>();
                slot.second->Start(*p, env);
            }
            PartBuoyancy& b = *slot.second;
            b.enabled = !v->packed;  // Part.Pack / Unpack (Part.cs:3756, :3790)
            if (b.enabled) b.TickInvoke(static_cast<double>(TimeWarp::fixedDeltaTime()));
            b.FixedUpdate(env);
        }
    }
    for (auto it = buoyancy_.begin(); it != buoyancy_.end();) {
        if (seen.count(it->first) == 0) it = buoyancy_.erase(it);
        else ++it;
    }
}

// ===================================================================================== CollisionEnhancer

CollisionEnhancer::Env ContactHost::EnhancerEnv() {
    CollisionEnhancer::Env env;
    auto vpOf = [this](Part& p) -> VesselPhysics* { return p.vessel != nullptr ? BindingOf(*p.vessel) : nullptr; };
    env.packed = [vpOf](Part& p) { return p.vessel == nullptr || p.vessel->packed || vpOf(p) == nullptr; };
    env.position = [](Part& p) { return p.partTransform.position; };
    env.setPosition = [vpOf](Part& p, const Vector3f& pos) {
        if (VesselPhysics* vp = vpOf(p)) vp->TranslatePartCluster(p, Vector3d(pos) - Vector3d(p.partTransform.position));
    };
    env.velocity = [vpOf](Part& p) {
        VesselPhysics* vp = vpOf(p);
        return vp != nullptr ? vp->RigidbodyState(p).velocity : Vector3f();
    };
    env.setVelocity = [vpOf](Part& p, const Vector3f& v) {
        if (VesselPhysics* vp = vpOf(p)) vp->SetPartVelocity(p, Vector3d(v));
    };
    env.linecast = [this](const Vector3f& from, const Vector3f& to) {
        CollisionEnhancer::LinecastHit out;
        const Vector3d d = Vector3d(to) - Vector3d(from);
        const double len = d.magnitude();
        if (!(len > 0.0)) return out;
        const PhysicsWorld::RayHit h = bubble_->World().RayCast(bubble_->StaticPhysicsFromWorld(Vector3d(from)), d, len, PhysicsWorld::kLayerStatic);
        if (!h.hit) return out;
        out.hit = true;
        out.point = (h.point + bubble_->FrameOriginWorld()).ToVector3f();
        out.normal = h.normal.ToVector3f();
        return out;  // terrain colliders have no Rigidbody
    };
    env.upAxis = [](const Vector3f& point) {
        const CelestialBody* b = FlightGlobals::currentMainBody;
        return b != nullptr ? FlightMath::getUpAxis(*b, Vector3d(point)).ToVector3f() : Vector3f::up();
    };
    env.frameVelocity = [] { return Krakensbane::GetFrameVelocity(); };
    const Vector3d shift = pendingShift_;
    env.worldShift = [shift] { return shift; };
    return env;
}

// seq 7: CollisionEnhancer.FixedUpdate of every part with a Rigidbody of every loaded vessel
void ContactHost::CollisionEnhancerFixedUpdate() {
    if (!options.collisionEnhancer) {
        pendingShift_ = Vector3d();
        return;
    }
    const CollisionEnhancer::Env env = EnhancerEnv();
    pendingShift_ = Vector3d();
    std::unordered_set<const Part*> seen;
    const std::vector<Vessel*> vessels = FlightGlobals::Vessels;
    for (Vessel* v : vessels) {
        if (v == nullptr || !v->loaded || v->state == Vessel::State::DEAD) continue;
        const std::vector<Part*> parts = v->parts;
        for (Part* p : parts) {
            if (!p->hasRigidbody || p->State() == PartStates::DEAD || p->vessel != v) continue;
            seen.insert(p);
            auto& slot = enhancers_[p];
            const uint32_t id = p->fields.flightID;
            if (!slot.second || slot.first != id) {
                slot.first = id;
                slot.second = std::make_unique<CollisionEnhancer>();
                slot.second->Start(*p, env);
            }
            slot.second->FixedUpdate(env);
        }
    }
    for (auto it = enhancers_.begin(); it != enhancers_.end();) {
        if (seen.count(it->first) == 0) it = enhancers_.erase(it);
        else ++it;
    }
}

// ===================================================================================== terrain colliders per body

Vector3d ContactHost::TerrainOrigin(const CelestialBody& body) const { return bubble_->StaticPhysicsFromWorld(body.position); }
QuaternionD ContactHost::TerrainRotation(const CelestialBody& body) const { return body.rotation; }

void ContactHost::BuildTerrain(HostedTerrain& t) {
    t.colliders.reset();
    // valid in the body's rotating frame only (PHYSICS-SPIKE.md 5.5: above the inverse-rotation threshold no vessel is near terrain)
    if (!t.body->inverseRotation) return;
    TerrainCollidersOptions opt = t.opt;
    opt.origin = TerrainOrigin(*t.body);
    opt.rotation = TerrainRotation(*t.body);
    t.colliders = std::make_unique<TerrainColliders>(bubble_->World(), *t.sphere, opt);
    t.colliders->AdoptExisting();
}

TerrainColliders* ContactHost::HostTerrain(CelestialBody& body, PQS& sphere, TerrainCollidersOptions opt) {
    DropTerrain(body);
    if (opt.keyBase == TerrainCollidersOptions{}.keyBase) {
        size_t index = 0;
        const auto& bodies = FlightGlobals::Bodies();
        for (size_t i = 0; i < bodies.size(); ++i)
            if (bodies[i] == &body) index = i;
        opt.keyBase = 0x5454000000000000ull + (static_cast<uint64_t>(index) << 32);
    }
    HostedTerrain t;
    t.body = &body;
    t.sphere = &sphere;
    t.opt = opt;
    StaticColliderInfo info;
    info.layer = 15;
    info.tag = "Untagged";
    info.name = body.bodyName;
    info.isPQ = true;
    info.pqBody = &body;
    RegisterStaticRange(opt.keyBase, 1ull << 32, info);
    terrain_.push_back(std::move(t));
    BuildTerrain(terrain_.back());
    return terrain_.back().colliders.get();
}

void ContactHost::DropTerrain(CelestialBody& body) {
    for (auto it = terrain_.begin(); it != terrain_.end(); ++it)
        if (it->body == &body) {
            it->colliders.reset();
            ranges_.erase(std::remove_if(ranges_.begin(), ranges_.end(), [&](const StaticRange& r) { return r.base == it->opt.keyBase; }),
                          ranges_.end());
            terrain_.erase(it);
            return;
        }
}

TerrainColliders* ContactHost::Terrain(const CelestialBody& body) const {
    for (const HostedTerrain& t : terrain_)
        if (t.body == &body) return t.colliders.get();
    return nullptr;
}

void ContactHost::RefreshTerrain() {
    for (HostedTerrain& t : terrain_) BuildTerrain(t);
    ++terrainRefreshes_;
}

void ContactHost::OnRotatingFrameTransition(events::GameEvents::HostTargetAction<CelestialBody*, bool>) { RefreshTerrain(); }
void ContactHost::OnDominantBodyChange(events::GameEvents::FromToAction<CelestialBody*, CelestialBody*>) { RefreshTerrain(); }

}  // namespace ksp::physics
