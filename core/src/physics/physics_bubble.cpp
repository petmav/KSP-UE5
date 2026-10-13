// WP-19b: the physics bubble; see include/ksp/physics/physics_bubble.hpp. C# paths are relative to
// decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/physics/physics_bubble.hpp"

#include <stdexcept>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/physical_object.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/physics/reference_frame.hpp"
#include "ksp/vessel/flight_logger.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp::physics {

using GE = events::GameEvents;

PhysicsBubble::PhysicsBubble(PhysicsWorld& world, const PhysicsBubbleOptions& options)
    : world_(&world), options_(options), host_(world, options.vessel) {
    // one frame for the whole bubble: every binding gets the same physics origin; the transform frame is the world (floatOrigin 0)
    host_.frameOriginWorld = [this](const Vessel&) { return frameOriginWorld_; };
    host_.floatOrigin = Vector3d();
}

PhysicsBubble::~PhysicsBubble() {
    Uninstall();
    objects_.clear();
    host_.DestroyAll();
}

// ===================================================================================== installation

void PhysicsBubble::Install() {
    if (installed_) return;
    if (Krakensbane::Instance() != nullptr || FloatingOrigin::Instance() != nullptr)
        throw std::logic_error("PhysicsBubble::Install: another Krakensbane / FloatingOrigin instance is alive");
    host_.Install();
    VesselEnvironment& env = Vessel::env;
    savedSetWorldVelocity_ = env.setWorldVelocity;
    env.setWorldVelocity = [this](Vessel& v, const Vector3d& vel) {
        if (VesselPhysics* vp = host_.Find(v)) vp->SetLinearVelocity(vel);
    };
    // Vessel.GoOffRails of the active vessel: Krakensbane.AddFrameVelocity (Vessel.cs:2440)
    savedKrakensbaneAddFrameVelocity_ = env.krakensbaneAddFrameVelocity;
    env.krakensbaneAddFrameVelocity = [](const Vector3d& vel) { Krakensbane::AddFrameVelocity(vel); };
    savedOpm_ = OrbitPhysicsManager::env;
    OrbitPhysicsManager::env.setDominantBody = [this](OrbitPhysicsManager& opm, CelestialBody* body) { setDominantBody(opm, body); };
    OrbitPhysicsManager::env.setRotatingFrame = [this](OrbitPhysicsManager& opm, bool rotating) { setRotatingFrame(opm, rotating); };
    savedAfterFlightIntegrator_ = FlightScene::hooks.afterFlightIntegrator;
    FlightScene::hooks.afterFlightIntegrator = [this] { RunFlightIntegrators(); };

    // Krakensbane (Krakensbane.cs:31 Awake, :50 Start) and its host
    krakensbane_.Awake();
    krakensbane_.Start();
    krakensbane_.recentreSpeed = options_.krakensbaneRecentreSpeed;
    krakensbane_.host.snapshot = [this] { return KrakensbaneSnapshot(); };
    // Krakensbane.cs:136 / :160: vessel.ChangeWorldVelocity on FlightGlobals.VesselsLoaded[index]
    krakensbane_.host.changeWorldVelocity = [](size_t index, const Vector3d& dv) {
        if (index < FlightGlobals::VesselsLoaded.size() && FlightGlobals::VesselsLoaded[index] != nullptr)
            FlightGlobals::VesselsLoaded[index]->ChangeWorldVelocity(dv);
    };
    // Krakensbane.cs:139-147 / :163-171: physicalObject.rb.AddForce(dv, ForceMode.VelocityChange): the float dv added to the float
    // velocity (one float addition: the exact double sum of two floats rounded once)
    krakensbane_.host.physicalObjectsVelocityChange = [](const Vector3d& dv) {
        const Vector3d d(dv.ToVector3f());
        for (physicalObject* o : FlightGlobals::physicalObjects) {
            if (o == nullptr || !o->HasRigidbody()) continue;
            o->world->SetLinearVelocity(o->rb, o->world->GetLinearVelocity(o->rb) + d);
        }
    };

    // FloatingOrigin (FloatingOrigin.cs:102 Awake) and the WP-19b rebase hooks
    floatingOrigin_.Awake();
    floatingOrigin_.ready = [] { return FlightGlobals::ready; };
    floatingOrigin_.activeVesselPosition = [] {
        return FlightGlobals::ActiveVessel != nullptr ? FlightGlobals::ActiveVessel->vesselTransform.position : Vector3f();
    };
    // FloatingOrigin.cs:133
    floatingOrigin_.continuousCondition = [this] {
        const Vessel* a = FlightGlobals::ActiveVessel;
        if (a == nullptr) return false;
        return (TimeWarp::CurrentRate() > TimeWarp::MaxPhysicsRate() && TimeWarp::WarpMode() == TimeWarp::Modes::HIGH && !a->LandedOrSplashed()) ||
               FlightGlobals::ship_velocity().sqrMagnitude() > floatingOrigin_.velForContinuousSqr;
    };
    floatingOrigin_.shiftWorld = [this](const Vector3d& offset, const Vector3d& nonKrakensbane) { ShiftWorld(offset, nonKrakensbane); };
    // FloatingOrigin.cs:157
    floatingOrigin_.activeVesselLandedOrSlow = [] {
        const Vessel* a = FlightGlobals::ActiveVessel;
        return a != nullptr && (a->situation == Vessel::Situations::LANDED || FlightGlobals::ship_srfVelocity().magnitude() < 100.0);
    };
    floatingOrigin_.preciseUpdateQuadPositions = [this] {
        if (preciseUpdateQuadPositions) preciseUpdateQuadPositions();
    };

    GE::onVesselUnloaded.Add(this, &PhysicsBubble::OnVesselUnloaded);
    GE::onFlightGlobalsRemoveVessel.Add(this, &PhysicsBubble::OnRemoveVessel);
    installed_ = true;
}

void PhysicsBubble::Uninstall() {
    if (!installed_) return;
    GE::onVesselUnloaded.Remove(this, &PhysicsBubble::OnVesselUnloaded);
    GE::onFlightGlobalsRemoveVessel.Remove(this, &PhysicsBubble::OnRemoveVessel);
    floatingOrigin_.OnDestroy();
    krakensbane_.OnDestroy();
    FlightScene::hooks.afterFlightIntegrator = savedAfterFlightIntegrator_;
    OrbitPhysicsManager::env = savedOpm_;
    Vessel::env.krakensbaneAddFrameVelocity = savedKrakensbaneAddFrameVelocity_;
    Vessel::env.setWorldVelocity = savedSetWorldVelocity_;
    host_.Uninstall();
    installed_ = false;
}

// A binding leaves with its vessel's parts (Vessel.Unload destroys them before GoOnRails, Vessel.cs:1882-1961) or with the vessel
// (FlightGlobals.RemoveVessel: Die, destruction)
void PhysicsBubble::OnVesselUnloaded(Vessel* v) {
    if (v != nullptr) host_.Destroy(*v);
}

void PhysicsBubble::OnRemoveVessel(Vessel* v) {
    if (v != nullptr) host_.Destroy(*v);
}

// ===================================================================================== frames

void PhysicsBubble::SetFrameOriginWorld(const Vector3d& w) {
    frameOriginWorld_ = w;
    RefreshBindings();
}

Vector3d PhysicsBubble::WorldFromPhysics(const Vector3d& p) const { return frameOriginWorld_ + ReferenceFrame::TruePosition(p); }
Vector3d PhysicsBubble::PhysicsFromWorld(const Vector3d& w) const { return ReferenceFrame::PhysicsPosition(w - frameOriginWorld_); }
Vector3d PhysicsBubble::StaticPhysicsFromWorld(const Vector3d& w) const { return w - frameOriginWorld_; }

void PhysicsBubble::RefreshBindings() {
    host_.RefreshFrames();
    for (const auto& vp : host_.Bindings()) vp->AfterStep();  // the cluster caches and the part transforms from the bodies
}

// FloatingOrigin.cs:166 setOffset, its shift of the bodies and the vessels (lines 181-206). kspcore's offsets are equal (the Galilean drift is
// FrameOffset, never a `nonFrame` part). The vessels with bodies are re-derived from the unchanged Jolt bodies (their transforms
// are a function of frameOriginWorld): the same shift as the C#'s `SetPosition(transform.position - offset)`, from doubles.
void PhysicsBubble::ShiftWorld(const Vector3d& offset, const Vector3d& offsetNonKrakensbane) {
    for (CelestialBody* body : FlightGlobals::Bodies()) body->position -= offsetNonKrakensbane;
    const bool flag = offset.sqrMagnitude() > floatingOrigin_.CoMRecalcOffsetMaxSqr;
    for (Vessel* vessel : FlightGlobals::Vessels) {
        if (vessel == nullptr || vessel->state == Vessel::State::DEAD) continue;
        const Vector3d v2 = (!vessel->loaded || vessel->packed || vessel->LandedOrSplashed()) ? offsetNonKrakensbane : offset;
        if (host_.Find(*vessel) == nullptr) vessel->SetPosition(Vector3d(vessel->vesselTransform.position) - v2);
        if (flag && vessel->packed) {
            vessel->precalc.CalculatePhysicsStats();
        } else {
            vessel->CoMD -= v2;
            vessel->CoM = vessel->CoMD.ToVector3f();
        }
    }
    // the physics bodies' anchor moves with the world (FloatingOrigin.cs:278-287 moves the physical objects' transforms; here their
    // world position derives from frameOriginWorld like every other body's)
    frameOriginWorld_ -= offsetNonKrakensbane;
    RefreshBindings();
}

bool PhysicsBubble::FoldFrameOffset() {
    const Vector3d f = floatingOrigin_.FrameOffset();
    if (f.IsZero()) return false;
    world_->TranslateBodies(f, true, false);
    floatingOrigin_.ResetFrameOffset();
    RefreshBindings();
    ++folds_;
    return true;
}

void PhysicsBubble::RebasePhysicsOrigin(const Vector3d& delta) {
    world_->TranslateBodies(-delta, true, true);
    frameOriginWorld_ += delta;
    RefreshBindings();
    ++physicsRebases_;
}

void PhysicsBubble::MaybeRebasePhysicsOrigin() {
    const double d = options_.physicsOriginRebaseDistance;
    if (!(d > 0.0)) return;
    const Vessel* a = FlightGlobals::ActiveVessel;
    if (a == nullptr || host_.Find(*a) == nullptr) return;
    const Vector3d p = PhysicsFromWorld(a->CoMD);
    if (p.sqrMagnitude() > d * d) RebasePhysicsOrigin(p);
}

// ===================================================================================== OrbitPhysicsManager with the bodies

// OrbitPhysicsManager.cs:235
void PhysicsBubble::setRotatingFrame(OrbitPhysicsManager& opm, bool rotatingFrameState) {
    FlightLogger::IgnoreGeeForces(0.5f);                    // :237
    opm.dominantBody->inverseRotation = rotatingFrameState;  // :238
    for (Vessel* vessel : FlightGlobals::Vessels) {          // :240-285
        if (vessel == nullptr || vessel->state == Vessel::State::DEAD || vessel->packed) continue;
        const Vector3d rFrmVel = opm.dominantBody->getRFrmVel(Vector3d(vessel->vesselTransform.position));
        // C# lines 248-276: every rb (and servoRb) of the parts: rb.velocity = (Vector3d)rb.velocity -/+ rFrmVel, one rigid-body change per cluster
        if (VesselPhysics* vp = host_.Find(*vessel)) vp->ChangeWorldVelocity(rotatingFrameState ? -rFrmVel : rFrmVel);
        if (rotatingFrameState) vessel->krakensbaneAcc -= rFrmVel;  // lines 277-284 (rFrmVel itself, not divided by dt: as the C#)
        else vessel->krakensbaneAcc += rFrmVel;
    }
    for (physicalObject* o : FlightGlobals::physicalObjects) {  // :286-300
        if (o == nullptr || !o->HasRigidbody()) continue;
        const Vector3d rFrmVel = opm.dominantBody->getRFrmVel(Vector3d(PhysicalObjectPosition(*o).ToVector3f()));
        const Vector3d v = o->world->GetLinearVelocity(o->rb);
        o->world->SetLinearVelocity(o->rb, rotatingFrameState ? v - rFrmVel : v + rFrmVel);
    }
    GE::onRotatingFrameTransition.Fire(GE::HostTargetAction<CelestialBody*, bool>(opm.dominantBody, rotatingFrameState));  // :301
}

namespace {
// OrbitPhysicsManager.cs:226 SetReverseOrbit, over the body orbit drivers (KSP's are OrbitDrivers of the body GameObjects)
void SetReverseOrbit(CelestialBody& body) {
    body.orbitDriver->reverse = true;
    CelestialBody* parent = body.orbitDriver->referenceBody;
    if (parent != nullptr && parent->orbitDriver) SetReverseOrbit(*parent);
}
}  // namespace

// OrbitPhysicsManager.cs:155
void PhysicsBubble::setDominantBody(OrbitPhysicsManager& opm, CelestialBody* body) {
    opm.dominantBody = body;  // :157
    // C# line 158: `Vector3[] array`: the physical objects' velocities relative to the active vessel's orbit, stored as floats
    std::vector<Vector3f> array(FlightGlobals::physicalObjects.size());
    // C# lines 160-164: Planetarium.Orbits[i].reverse = false: the vessels' OrbitDrivers and the bodies' (kspcore keeps the bodies' drivers on
    // the CelestialBody, not in Planetarium.Orbits)
    if (std::vector<OrbitDriver*>* orbits = Planetarium::Orbits())
        for (OrbitDriver* od : *orbits)
            if (od != nullptr) od->reverse = false;
    for (CelestialBody* b : FlightGlobals::Bodies())
        if (b->orbitDriver) b->orbitDriver->reverse = false;
    if (opm.dominantBody->orbitDriver) SetReverseOrbit(*opm.dominantBody);  // :165-168
    FlightLogger::IgnoreGeeForces(0.5f);                                     // :169
    const Vessel* active = FlightGlobals::ActiveVessel;
    const CelestialBody* activeRef = active != nullptr ? active->mainBody() : nullptr;
    // C# lines 170-180 (FlightGlobals.ActiveVessel.orbit: the C# throws without an active vessel; here the objects are then left alone)
    const size_t count2 = active != nullptr ? FlightGlobals::physicalObjects.size() : 0;
    for (size_t j = 0; j < count2; ++j) {
        physicalObject* o = FlightGlobals::physicalObjects[j];
        if (o == nullptr || !o->HasRigidbody()) continue;
        array[j] = (o->world->GetLinearVelocity(o->rb) - (active->orbit().GetVel(activeRef) - Krakensbane::GetFrameVelocity())).ToVector3f();
    }
    const Vector3d frameVelocity = Krakensbane::GetFrameVelocity();  // :181
    for (Vessel* vessel : FlightGlobals::VesselsLoaded) {            // :182-204
        if (vessel == nullptr || vessel->state == Vessel::State::DEAD || vessel->packed) continue;
        vessel->IgnoreGForces(20);
        const Vector3d vector3d = vessel->orbit().GetVel(activeRef) - frameVelocity;
        // C# lines 191-202: every part rb (and servoRb) = vector3d: every cluster's centre-of-mass velocity (angular velocities kept)
        if (VesselPhysics* vp = host_.Find(*vessel)) vp->SetLinearVelocity(vector3d);
    }
    for (size_t m = 0; m < count2; ++m) {  // :205-214
        physicalObject* o = FlightGlobals::physicalObjects[m];
        if (o == nullptr || !o->HasRigidbody()) continue;
        o->world->SetLinearVelocity(o->rb, active->orbit().GetVel(activeRef) - Krakensbane::GetFrameVelocity() + Vector3d(array[m]));
    }
    // C# line 215 (both arguments are the new body: dominantBody was assigned first)
    GE::onDominantBodyChange.Fire(GE::FromToAction<CelestialBody*, CelestialBody*>(opm.dominantBody, body));
}

// ===================================================================================== Krakensbane host

FrameSnapshot PhysicsBubble::KrakensbaneSnapshot() const {
    FrameSnapshot s;
    s.ready = FlightGlobals::ready;
    const Vessel* a = FlightGlobals::ActiveVessel;
    s.hasActiveVessel = a != nullptr;
    auto stateOf = [a](const Vessel& v) {
        FrameVesselState f;
        f.isActive = &v == a;
        f.packed = v.packed;
        f.dead = v.state == Vessel::State::DEAD;
        f.landedOrSplashed = v.LandedOrSplashed();
        f.radarAltitude = v.radarAltitude;
        return f;
    };
    if (a != nullptr) {
        // ActiveVessel.GetComponent<Rigidbody>(): the Vessel component sits on the root part's GameObject, whose Rigidbody Part.Start made
        // (kept, kinematic, while packed)
        s.activeHasRigidbody = a->loaded && a->rootPart != nullptr && a->rootPart->hasRigidbody;
        s.active = stateOf(*a);
        s.velocityD = a->velocityD;
        s.rb_velocityD = a->rb_velocityD;
        s.upAxis = a->upAxis;
    }
    for (const Vessel* v : FlightGlobals::VesselsLoaded) s.loaded.push_back(v != nullptr ? stateOf(*v) : FrameVesselState{});
    return s;
}

// ===================================================================================== physical objects

physicalObject& PhysicsBubble::CreatePhysicalObject(BodyDesc desc, const Vector3d& worldPosition, const Vector3d& trueVelocity) {
    desc.position = PhysicsFromWorld(worldPosition);
    desc.linearVelocity = ReferenceFrame::PhysicsVelocity(trueVelocity);
    if (desc.motion == MotionType::Static) desc.motion = MotionType::Dynamic;
    const BodyId id = world_->CreateBody(desc);
    objects_.push_back(std::make_unique<physicalObject>(*world_, id));
    return *objects_.back();
}

void PhysicsBubble::DestroyPhysicalObject(physicalObject* o) {
    for (auto it = objects_.begin(); it != objects_.end(); ++it)
        if (it->get() == o) {
            objects_.erase(it);
            return;
        }
}

Vector3d PhysicsBubble::PhysicalObjectPosition(const physicalObject& o) const {
    return o.HasRigidbody() ? WorldFromPhysics(o.world->GetPosition(o.rb)) : Vector3d();
}

// ===================================================================================== per tick

void PhysicsBubble::RunFlightIntegrators() {
    const std::vector<Vessel*> vessels = FlightGlobals::Vessels;
    for (Vessel* v : vessels) {
        if (v == nullptr || v->packed) continue;
        VesselPhysics* vp = host_.Find(*v);
        if (vp == nullptr) continue;
        if (flightIntegrator) flightIntegrator(*v, *vp);
        else DefaultFlightIntegrator(*v, *vp);
    }
}

// FlightIntegrator.cs:544 FixedUpdate, its lines 661-669 (the rest of FlightIntegrator.FixedUpdate is WP-20d's)
void PhysicsBubble::DefaultFlightIntegrator(Vessel& v, VesselPhysics& vp) {
    if (FlightGlobals::ActiveVessel == &v && !FlightGlobals::physicalObjects.empty()) {
        // IntegratePhysicalObjects (:747), its gravity (:752, :762): the acceleration at the vessel's CoM on every object (the drag
        // half needs FlightIntegrator's pseudoReDragMult: WP-20d)
        const CelestialBody& body = *v.mainBody();
        const Vector3f force = ((FlightGlobals::getGeeForceAtPosition(v.CoMD, body) + FlightGlobals::getCoriolisAcc(v.velocityD, body) +
                                 FlightGlobals::getCentrifugalAcc(v.CoMD, body)) *
                                PhysicsGlobals::GraviticForceMultiplier)
                                   .ToVector3f();
        size_t count = FlightGlobals::physicalObjects.size();
        while (count-- > 0) {
            physicalObject* o = FlightGlobals::physicalObjects[count];
            if (o == nullptr || !o->HasRigidbody()) continue;
            o->world->AddForce(o->rb, Vector3d(force) * static_cast<double>(o->world->GetMass(o->rb)));  // rb.AddForce(force, Acceleration)
        }
    }
    if (!v.precalc.preIntegrationVelocityOffset.IsZero()) v.OffsetVelocity(v.precalc.preIntegrationVelocityOffset);  // :665-668
    vp.IntegratePartBuffers(v.precalc.integrationAccel);                                                             // :669 Integrate
}

void PhysicsBubble::FixedUpdate() {
    FlightScene::FixedUpdate();  // seq 7-281 (FlightIntegrator slot: RunFlightIntegrators)
    FrameUpdate();               // seq 284-285
    PhysicsStep();               // seq 291
}

void PhysicsBubble::FrameUpdate() {
    krakensbane_.FixedUpdate();     // seq 284
    floatingOrigin_.FixedUpdate();  // seq 285: frame drift; world rebase by KSP's threshold rule
    if (options_.foldFrameOffset) {
        if (krakensbane_.FrameVel.IsZero()) FoldFrameOffset();
        world_->SetStaticCollisions(floatingOrigin_.FrameOffset().IsZero());
    }
    MaybeRebasePhysicsOrigin();
}

// seq 291 PhysicsFixedUpdate (Time.fixedDeltaTime: TimeWarp sets it during physical warp)
void PhysicsBubble::PhysicsStep() { host_.Step(Time::fixedDeltaTime); }

void PhysicsBubble::Update() {
    FlightScene::Update();
    // physicalObject.cs:48 Update: an object destroys itself at high warp or beyond maxDistance from the active vessel
    const Vessel* a = FlightGlobals::ActiveVessel;
    const Vector3f activePos = a != nullptr ? a->vesselTransform.position : Vector3f();
    std::vector<physicalObject*> doomed;
    for (const auto& o : objects_)
        if (o->UpdateWantsDestroy(PhysicalObjectPosition(*o).ToVector3f(), a != nullptr, activePos)) doomed.push_back(o.get());
    for (physicalObject* o : doomed) DestroyPhysicalObject(o);
}

void PhysicsBubble::LateUpdate() { FlightScene::LateUpdate(); }

}  // namespace ksp::physics
