// WP-19b: the physics bubble; see include/ksp/physics/physics_bubble.hpp. C# paths are relative to
// decompiled/KSP-Decompiled/Assembly-CSharp/.
#include "ksp/physics/physics_bubble.hpp"

#include <stdexcept>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/input/flight_input_handler.hpp"
#include "ksp/modules/module_reaction_wheel.hpp"
#include "ksp/orbit/orbit_driver.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physical_object.hpp"
#include "ksp/physics/reference_frame.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/space_center/scene_drivers.hpp"
#include "ksp/vessel/flight_logger.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_autopilot.hpp"

namespace ksp::physics {

using GE = events::GameEvents;

PhysicsBubble::PhysicsBubble(PhysicsWorld& world, const PhysicsBubbleOptions& options)
    : world_(&world), options_(options), host_(world, options.vessel), contacts_(*this, options.contacts) {
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
    // WP-20d: the part-module FixedUpdates before FlightIntegrator (seq 22-276), the frame's ModuleRCS.Update / FlightIntegrator.Update
    // before Vessel.Update
    savedBeforeFlightIntegrator_ = FlightScene::hooks.beforeFlightIntegrator;
    // seq 18 (FlightInputHandler.FixedUpdate -> Vessel.FeedInputFeed: the seam `inputFeed` or the host's FlightInputHandler, WP-20f),
    // seq 22-276 (the part modules), then PartBuoyancy
    FlightScene::hooks.beforeFlightIntegrator = [this] {
        InputFeed();
        moduleForces_.FixedUpdate(host_);
        if (options_.contactHost) contacts_.BuoyancyFixedUpdate();
    };
    // WP-20e: seq 7 CollisionEnhancer.FixedUpdate (after VesselPrecalculate / OrbitDriver)
    savedAfterPrecalc_ = FlightScene::hooks.afterPrecalc;
    FlightScene::hooks.afterPrecalc = [this] {
        if (options_.contactHost) contacts_.CollisionEnhancerFixedUpdate();
    };
    if (options_.contactHost) contacts_.Install();
    if (options_.resourceHost) resources_.Install();  // WP-20g
    // WP-20f: the scene state VesselSAS and ModuleReactionWheel read (see SetFlightInputHandler); the saved hook answers while no
    // FlightInputHandler is set
    savedKillAngularVelocity_ = AutopilotEnvironment::killAngularVelocity;
    // VesselAutopilot.cs:672 KillAngularVelocity: rb.angularVelocity = Vector3.zero on every part (count-- order) except a robotic rotor
    // and a control surface below one (Part.isRoboticRotor / isChildOfRoboticRotor, Part.cs:1373 / :1441: false without Serenity)
    AutopilotEnvironment::killAngularVelocity = [this](Vessel& v) {
        VesselPhysics* vp = host_.Find(v);
        if (vp == nullptr) return;
        const bool serenity = SaveEnvironment::Current().IsExpansionInstalled("Serenity");
        size_t count = v.parts.size();
        while (count-- > 0) {
            Part* part = v.parts[count];
            if (serenity) {
                if (part->FindModuleOfClass("ModuleRoboticServoRotor") != nullptr) continue;
                bool childOfRotor = false;
                for (Part* p = part->parent; p != nullptr && !childOfRotor; p = p->parent)
                    childOfRotor = p->FindModuleOfClass("ModuleRoboticServoRotor") != nullptr;
                if (childOfRotor && part->isControlSurface()) continue;
            }
            vp->SetPartAngularVelocity(*part, Vector3d());
        }
    };
    savedActiveVesselPYR_ = ReactionWheelEnvironment::activeVesselPYR;
    ReactionWheelEnvironment::activeVesselPYR = [this]() -> Vector3f {
        if (flightInputHandler_ != nullptr) return FlightInputHandler::state.GetPYR();  // ModuleReactionWheel.cs:165
        return savedActiveVesselPYR_ ? savedActiveVesselPYR_() : Vector3f::zero();
    };
    savedPitchInput_ = AutopilotEnvironment::pitchInput;
    savedRollInput_ = AutopilotEnvironment::rollInput;
    savedYawInput_ = AutopilotEnvironment::yawInput;
    AutopilotEnvironment::pitchInput = [this](float threshold) {
        if (flightInputHandler_ != nullptr) return AutopilotPitchInput(threshold);
        return savedPitchInput_ ? savedPitchInput_(threshold) : false;
    };
    AutopilotEnvironment::rollInput = [this](float threshold) {
        if (flightInputHandler_ != nullptr) return AutopilotRollInput(threshold);
        return savedRollInput_ ? savedRollInput_(threshold) : false;
    };
    AutopilotEnvironment::yawInput = [this](float threshold) {
        if (flightInputHandler_ != nullptr) return AutopilotYawInput(threshold);
        return savedYawInput_ ? savedYawInput_(threshold) : false;
    };
    savedPrecisionMode_ = AutopilotEnvironment::precisionMode;
    AutopilotEnvironment::precisionMode = [this]() {
        if (flightInputHandler_ != nullptr) return flightInputHandler_->precisionMode;  // ModuleRCS.GetPotentialTorque reads it
        return savedPrecisionMode_ ? savedPrecisionMode_() : true;
    };
    savedBeforeVesselUpdate_ = FlightScene::hooks.beforeVesselUpdate;
    FlightScene::hooks.beforeVesselUpdate = [this] {
        PartValuesUpdate();  // WP-20f
        moduleForces_.Update();
        fiHost_.Update();
    };

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
    GE::onVesselLoaded.Add(this, &PhysicsBubble::OnVesselLoaded);
    installed_ = true;
}

void PhysicsBubble::Uninstall() {
    if (!installed_) return;
    GE::onVesselUnloaded.Remove(this, &PhysicsBubble::OnVesselUnloaded);
    GE::onFlightGlobalsRemoveVessel.Remove(this, &PhysicsBubble::OnRemoveVessel);
    GE::onVesselLoaded.Remove(this, &PhysicsBubble::OnVesselLoaded);
    floatingOrigin_.OnDestroy();
    krakensbane_.OnDestroy();
    contacts_.Uninstall();
    resources_.Uninstall();  // WP-20g
    AutopilotEnvironment::killAngularVelocity = savedKillAngularVelocity_;
    ReactionWheelEnvironment::activeVesselPYR = savedActiveVesselPYR_;
    AutopilotEnvironment::pitchInput = savedPitchInput_;
    AutopilotEnvironment::rollInput = savedRollInput_;
    AutopilotEnvironment::yawInput = savedYawInput_;
    AutopilotEnvironment::precisionMode = savedPrecisionMode_;
    FlightScene::hooks.afterPrecalc = savedAfterPrecalc_;
    FlightScene::hooks.afterFlightIntegrator = savedAfterFlightIntegrator_;
    FlightScene::hooks.beforeFlightIntegrator = savedBeforeFlightIntegrator_;
    FlightScene::hooks.beforeVesselUpdate = savedBeforeVesselUpdate_;
    fiHost_.Clear();
    OrbitPhysicsManager::env = savedOpm_;
    Vessel::env.krakensbaneAddFrameVelocity = savedKrakensbaneAddFrameVelocity_;
    Vessel::env.setWorldVelocity = savedSetWorldVelocity_;
    host_.Uninstall();
    installed_ = false;
}

// WP-20f: seq 18. The gimbal transforms first follow their parts (the physics step moved them; Unity's hierarchy keeps a child on its
// parent) where VesselSAS will read them (UpdateVesselTorque -> ModuleGimbal.GetPotentialTorque in FeedInputFeed; ModuleGimbal.FixedUpdate
// poses its own), then the pilot input and FeedInputFeed
void PhysicsBubble::InputFeed() {
    for (Vessel* v : FlightGlobals::VesselsLoaded)
        if (v != nullptr && !v->packed && v->Autopilot().SAS().FBWconnected()) moduleForces_.PoseGimbalTransforms(*v);
    if (inputFeed) {
        inputFeed();
        return;
    }
    if (flightInputHandler_ == nullptr) return;
    flightInputHandler_->FixedUpdate();  // FlightInputHandler.cs:377
    // FlightInputHandler.fetch.precisionMode, read by ModuleRCS.FixedUpdate (seq 22-276)
    moduleForces_.hooks.precisionMode = flightInputHandler_->precisionMode;
}

// Part.cs:4307 Update, its PartValues refresh (:4328-4331) for the parts of every loaded vessel: the skills VesselValues folds over the
// parts (a probe's ModuleSAS service level for VesselAutopilot) are the values of the last refresh. The rest of Part.Update (editor,
// mouse over, temperature renderer, onPartUpdate, ModulesOnUpdate) is not this package's.
void PhysicsBubble::PartValuesUpdate() {
    if (FlightDriver::Pause()) return;
    if (!ResourceFlowEnvironment::LoadedSceneIsFlight() || !FlightGlobals::ready) return;
    for (Vessel* v : FlightGlobals::Vessels) {
        if (v == nullptr || !v->loaded) continue;
        for (Part* p : v->parts) {
            const PartStates st = p->State();
            if (st == PartStates::DEAD || st == PartStates::PLACEMENT || st == PartStates::CARGO || !p->fields.started) continue;
            if (p->fields.overrideSkillUpdate || p->fields.CrewCapacity > 0) p->partValues.Update();
        }
    }
}

// A binding leaves with its vessel's parts (Vessel.Unload destroys them before GoOnRails, Vessel.cs:1882-1961) or with the vessel
// (FlightGlobals.RemoveVessel: Die, destruction)
void PhysicsBubble::OnVesselUnloaded(Vessel* v) {
    if (v == nullptr) return;
    host_.Destroy(*v);
    fiHost_.OnVesselUnloaded(*v);  // FlightIntegrator.OnUnloadVessel (FlightIntegrator.cs:390): Setup again on the next load
}

// Vessel.Load (Vessel.cs:1813) instantiated the parts: Unity runs their Part.Start before the next FixedUpdate (WP-20d), the module
// lifecycle included (WP-20g: ModulesOnStart, the flight block, ModulesOnStartFinished, the first ModuleJettison.LateUpdate)
void PhysicsBubble::OnVesselLoaded(Vessel* v) {
    if (v != nullptr) StartVesselInFlight(*v);
}

void PhysicsBubble::OnRemoveVessel(Vessel* v) {
    if (v == nullptr) return;
    host_.Destroy(*v);
    fiHost_.Remove(*v);  // the VesselModule goes with its vessel
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
    contacts_.AddWorldShift(offsetNonKrakensbane);  // WP-20e: CollisionEnhancer's `lastPos -= FloatingOrigin.Offset`
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
    contacts_.RefreshTerrain();  // WP-20e: the hosted terrain colliders' frame moved with the static bodies
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

// seq 278: FlightIntegrator.FixedUpdate (FlightIntegrator.cs:544) of every loaded vessel (WP-20d), or the host's override per unpacked vessel
void PhysicsBubble::RunFlightIntegrators() {
    if (!flightIntegrator) {
        fiHost_.FixedUpdate(host_);
        return;
    }
    const std::vector<Vessel*> vessels = FlightGlobals::Vessels;
    for (Vessel* v : vessels) {
        if (v == nullptr || v->packed) continue;
        VesselPhysics* vp = host_.Find(*v);
        if (vp == nullptr) continue;
        flightIntegrator(*v, *vp);
    }
}

void PhysicsBubble::FixedUpdate() {
    FlightScene::FixedUpdate();  // seq 7-281 (FlightIntegrator slot: RunFlightIntegrators)
    FrameUpdate();               // seq 284-285
    PhysicsStep();               // seq 291
}

void PhysicsBubble::FrameUpdate() {
    krakensbane_.FixedUpdate();     // seq 284
    const Vector3d drift0 = floatingOrigin_.FrameOffset();
    floatingOrigin_.FixedUpdate();  // seq 285: frame drift; world rebase by KSP's threshold rule
    // WP-20e: KSP's world moved with the Galilean frame every tick (setOffset), so CollisionEnhancer measured the part's motion in that
    // frame: the frame drift of the tick is part of its FloatingOrigin.Offset
    contacts_.AddWorldShift(floatingOrigin_.FrameOffset() - drift0);
    if (options_.foldFrameOffset) {
        if (krakensbane_.FrameVel.IsZero()) FoldFrameOffset();
        world_->SetStaticCollisions(floatingOrigin_.FrameOffset().IsZero());
    }
    MaybeRebasePhysicsOrigin();
}

// seq 291 PhysicsFixedUpdate (Time.fixedDeltaTime: TimeWarp sets it during physical warp)
void PhysicsBubble::PhysicsStep() {
    if (physicsStep) physicsStep(Time::fixedDeltaTime);  // WP-29c: the wheels' native step and physics-warp substeps
    else host_.Step(Time::fixedDeltaTime);
    // WP-20e: PhysX's callbacks after simulate (the RB anchor's force and break, OnCollisionEnter / Stay / Exit), then the structural
    // changes they caused (an explosion's Part.Die). With WP-29c's substeps the world stepped several times: the contact host dispatches
    // the events of every one of them (PhysicsWorld::TakeContactEventLog)
    if (options_.contactHost && contacts_.Installed()) {
        contacts_.AfterPhysicsStep(Time::fixedDeltaTime);
        host_.ProcessStructuralChanges();
    }
}

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
