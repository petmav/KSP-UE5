// WP-19b: the physics bubble: frames, Krakensbane, FloatingOrigin and several vessels in one PhysicsWorld on the reduced model.
//
// What KSP does and what the reduced model keeps (C# citations relative to decompiled/KSP-Decompiled/Assembly-CSharp):
//
// FRAME POLICY (resolves STATUS.md WP-20b risk R1, the floatOrigin-in-CalculatePhysicsStats frame mixing):
//   * ONE world frame for every float and double world position, as in KSP: part / vessel transforms, Vessel.CoM, rb.worldCenterOfMass,
//     Part.AddForceAtPosition's position (floats) and CelestialBody.position, Vessel.CoMD, worldSurfacePos (doubles). The transform frame
//     of VesselPhysics is this world frame (floatOrigin = 0, KSP's convention), so every ported statement that mixes them
//     (CalculatePhysicsStats' `CoM = CoMD; localCoM = vesselTransform.InverseTransformPoint(CoM)`, UpdatePosVel's
//     GetLatLonAlt(vesselTransform.position), OrbitDriver.TrackRigidbody's CoMD - body.position) runs unchanged.
//   * The world ORIGIN is kept near the active vessel by FloatingOrigin's own rule (FloatingOrigin.cs:131-145: `continuous`, or the
//     active vessel farther than `threshold` = 500 m, while canEngageThisFrame), so the floats have KSP's precision; the shift is
//     setOffset's non-physics half (FloatingOrigin.cs:181-206: every CelestialBody.position, every vessel's transform and CoMD / CoM),
//     ShiftWorld below. Jolt bodies NEVER move for it: they are tied to the world by one double, frameOriginWorld (the world position
//     of the physics origin), which the shift moves with everything else. Without this, kspcore's world has the dominant body at the
//     origin and a vessel at 680 km has float fields with a 6 cm ulp.
//   * Physics frame of a DYNAMIC body:  world = frameOriginWorld + p + FloatingOrigin::FrameOffset()  (Krakensbane's Galilean drift,
//     WP-20a). Physics frame of STATIC geometry (terrain quads, KSC):  world = frameOriginWorld + s  (the body frame). The two agree
//     while FrameOffset is zero. Krakensbane engages only when every loaded vessel is airborne (SafeToEngage, Krakensbane.cs:87-119:
//     the active vessel >= altThresholdAlone / altThreshold above the terrain, the others above altThreshold and not landed), so
//     while FrameOffset != 0 the static geometry is out of reach and does not collide (PhysicsWorld::SetStaticCollisions(false));
//     when the frame velocity is back to zero (Krakensbane.Zero, packed active vessel) the drift is FOLDED into the dynamic bodies
//     (one SetPosition per body, O(bodies), at disengagement only), FrameOffset is reset and static collisions come back.
//   * The physics origin itself is re-centred (every Jolt body, static ones included, moved once: O(bodies)) only when the active
//     vessel is farther than physicsOriginRebaseDistance (1,000 km) from it: Jolt's broad phase is float (PHYSICS-SPIKE.md R3).
//   * Rotating <-> inertial (OrbitPhysicsManager.cs:235 setRotatingFrame) and SOI change of the bubble (:155 setDominantBody) are ported
//     with their rigid-body effects (setRotatingFrame / setDominantBody below): the world frame's axes are KSP's (body-fixed while the
//     dominant body is in the rotating frame, inertial otherwise), world positions are continuous across both, only velocities change as
//     the C# changes them. The dominant body stays fixed in the world (the reversed orbit-driver chain, Planetarium.UpdateCBsRecursive), so
//     neither event moves the physics origin. (KSP's tick order switches after this tick's frame was set: the step of the switch tick runs
//     the new velocities in the old axes, which displaces the trajectory once by one tick of the frame's motion, -/+ (omega x r) dt or
//     -V_body dt; reproduced, STATUS.md WP-19b.)
//
// SEVERAL VESSELS: one VesselPhysics per loaded unpacked vessel in one PhysicsWorld (VesselPhysicsHost, WP-20b), all in this bubble's
// one frame; load / unload by Vessel.Update (VesselRanges, WP-19a), pack / unpack by OrbitPhysicsManager.LateUpdate (WP-19a) drive
// GoOffRails / GoOnRails -> the host builds / drops the bodies. A binding also goes when its vessel unloads (Vessel.Unload destroys the
// parts before GoOnRails, Vessel.cs:1882-1961) or leaves the registry (Die, destruction): onVesselUnloaded / onFlightGlobalsRemoveVessel.
// The parts a Vessel.Load instantiates get Part.Start's flight block (StartPartsInFlight) on onVesselLoaded (WP-20d).
//
// KRAKENSBANE HOST: the snapshot from FlightGlobals (ActiveVessel, VesselsLoaded: packed, DEAD, LandedOrSplashed, radarAltitude,
// velocityD, rb_velocityD, upAxis), Vessel.ChangeWorldVelocity (Vessel.cs:3197, with krakensbaneAcc) on VesselsLoaded[i], the
// physical objects' rb.AddForce(dv, VelocityChange). Hosts must run Krakensbane in orbit (WP-20b R3: without it an orbiting vessel
// drifts metres per orbit from Jolt's float velocities); the bubble's FixedUpdate does.
//
// PER TICK (FixedUpdate, the order measured in game, flight_scene.hpp): FlightScene::FixedUpdate (VesselPrecalculate, OrbitDriver,
// FlightGlobals, WP-20f: the input slot = FlightInputHandler.FixedUpdate -> Vessel.FeedInputFeed (SetFlightInputHandler / inputFeed), the
// part-module slot = PartModuleForces (WP-20d: engines / RCS / lifting surfaces / reaction wheels into the part force buffers; WP-20f: ModuleCommand,
// ModuleSAS, ModuleGimbal), the FlightIntegrator slot = FlightIntegrator.FixedUpdate of every loaded vessel (WP-20d, ksp/physics/flight_integrator.hpp;
// `flightIntegrator` overrides it per unpacked vessel), OrbitPhysicsManager.FixedUpdate -> setRotatingFrame / setDominantBody,
// Vessel.FixedUpdate), Krakensbane.FixedUpdate, FloatingOrigin.FixedUpdate (frame drift, world rebase), the frame-offset fold / static
// gate, the physics-origin rebase, then the physics step (Flush, PhysicsWorld::Step, AfterStep: transforms). Planetarium.FixedUpdate
// (body positions, UT) is the host's and runs before it. Update: FlightScene::Update with ModuleRCS.Update and FlightIntegrator.Update
// before Vessel.Update (the measured frame order).
#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include "ksp/math/vector3d.hpp"
#include "ksp/physics/contact_host.hpp"
#include "ksp/physics/flight_integrator.hpp"
#include "ksp/physics/floating_origin.hpp"
#include "ksp/physics/krakensbane.hpp"
#include "ksp/physics/orbit_physics_manager.hpp"
#include "ksp/physics/part_forces.hpp"
#include "ksp/physics/physics_world.hpp"
#include "ksp/physics/vessel_physics.hpp"
#include "ksp/resources/vessel_resource_host.hpp"
#include "ksp/vessel/flight_scene.hpp"

namespace ksp {
class CelestialBody;
class FlightInputHandler;
class Vessel;
class physicalObject;
}  // namespace ksp

namespace ksp::physics {

struct PhysicsBubbleOptions {
    VesselPhysicsOptions vessel;
    // re-centre the physics origin when the active vessel's physics-frame centre of mass is farther than this from it (0: never)
    double physicsOriginRebaseDistance = 1.0e6;
    // fold FrameOffset into the dynamic bodies once the frame velocity is zero, static collisions only while FrameOffset == 0
    bool foldFrameOffset = true;
    // Krakensbane::recentreSpeed (0 = KSP's rule: the whole rb velocity moves into FrameVel every tick above MaxV)
    double krakensbaneRecentreSpeed = 0.0;
    // WP-20e: contact, landing and water (ksp/physics/contact_host.hpp): on by default (KSP's behaviour); `contacts` selects the pieces
    bool contactHost = true;
    ContactHostOptions contacts;
    // WP-20g: the flight scene's resource-flow host (ksp/resources/vessel_resource_host.hpp): every Vessel's PartSets behind
    // ResourceFlowEnvironment::vesselHost and Vessel::env.updateResourceSets, as KSP's Vessel holds them (Vessel.cs:451-464, :3913)
    bool resourceHost = true;
};

class PhysicsBubble {
public:
    explicit PhysicsBubble(PhysicsWorld& world, const PhysicsBubbleOptions& options = {});
    ~PhysicsBubble();  // Uninstall, bindings and physical objects removed
    PhysicsBubble(const PhysicsBubble&) = delete;
    PhysicsBubble& operator=(const PhysicsBubble&) = delete;

    PhysicsWorld& World() const { return *world_; }
    VesselPhysicsHost& Host() { return host_; }
    Krakensbane& KrakensbaneInstance() { return krakensbane_; }
    FloatingOrigin& FloatingOriginInstance() { return floatingOrigin_; }
    const PhysicsBubbleOptions& Options() const { return options_; }

    // Installs every seam (and saves what it replaces; Uninstall / the destructor restore it): the VesselPhysicsHost hooks of
    // Vessel::env plus setWorldVelocity, OrbitPhysicsManager::env (setRotatingFrame / setDominantBody), FlightScene::hooks
    // .beforeFlightIntegrator / .afterFlightIntegrator / .beforeVesselUpdate, the Krakensbane and FloatingOrigin singletons (owned; Awake / Start) with their host hooks, and the
    // lifecycle listeners. Throws std::logic_error when another Krakensbane or FloatingOrigin instance is alive.
    void Install();
    void Uninstall();
    bool Installed() const { return installed_; }

    // Overrides the FlightIntegrator slot of one unpacked vessel (FlightIntegrator.FixedUpdate, FlightIntegrator.cs:544). Unset (the
    // default): WP-20d's FlightIntegrator of every loaded vessel (FlightIntegrators(); the WP-19b stand-in it replaces is gone).
    std::function<void(Vessel&, VesselPhysics&)> flightIntegrator;
    // WP-20d: the FlightIntegrator of every loaded vessel and the part-module force slot (kernel, hooks, statistics)
    FlightIntegratorHost& FlightIntegrators() { return fiHost_; }
    PartModuleForces& ModuleForces() { return moduleForces_; }
    // WP-20e: contacts -> Part.OnCollision*, landed / splashed, PartBuoyancy, CollisionEnhancer, the RB anchor, ground probes, terrain per body
    ContactHost& Contacts() { return contacts_; }
    // WP-20g: the resource-flow host (installed with the bubble when PhysicsBubbleOptions::resourceHost)
    VesselResourceHost& Resources() { return resources_; }
    // WP-20e seam: the slot of FlightInputHandler.FixedUpdate in the measured order (run1-L_calltrace.csv seq 18: after FlightGlobals
    // (15), before TimeWarp (19) and the part modules (22-276)). When set it replaces the slot; unset: the FlightInputHandler below.
    std::function<void()> inputFeed;
    // WP-20f: the flight scene's FlightInputHandler (a MonoBehaviour of the scene: the host makes it and runs Awake / Start / Update /
    // OnDestroy). The bubble runs its FixedUpdate (FlightInputHandler.cs:377: the pilot state into the active vessel, then
    // Vessel.FeedInputFeed of every unpacked loaded vessel: OnPreAutopilotUpdate, OnAutopilotUpdate = VesselSAS, OnPostAutopilotUpdate,
    // OnFlyByWire, Part.propagateControlUpdate) in the slot above, before the part modules read vessel.ctrlState. While one is set, the
    // scene inputs the C# reads from it are wired: ReactionWheelEnvironment::activeVesselPYR (FlightInputHandler.state.GetPYR()),
    // AutopilotEnvironment::pitchInput / rollInput / yawInput (VesselSAS.CheckPitch / Roll / YawInput through GameSettings),
    // AutopilotEnvironment::precisionMode and the module slot's precisionMode (FlightInputHandler.fetch.precisionMode).
    // nullptr (default): no input feed (vessel.ctrlState is what the host / test writes).
    void SetFlightInputHandler(FlightInputHandler* handler) { flightInputHandler_ = handler; }
    FlightInputHandler* GetFlightInputHandler() const { return flightInputHandler_; }
    // FloatingOrigin's PreciseUpdateQuadPositions after a rebase (the PQS host, WP-12): unset = nothing
    std::function<void()> preciseUpdateQuadPositions;
    // WP-29c: replaces the physics step (seq 291) of one tick of `dt` seconds; unset: Host().Step(dt). The wheels' host
    // (ksp/physics/vessel_wheels.hpp) puts its native suspension step and the physics-warp substeps here (each substep is a Host().Step).
    std::function<void(float dt)> physicsStep;

    // ---- frames (see the header comment) -----------------------------------------------------------------------------------
    Vector3d FrameOriginWorld() const { return frameOriginWorld_; }
    // Places the physics origin in the world (before bodies exist; later changes go through ShiftWorld / RebasePhysicsOrigin)
    void SetFrameOriginWorld(const Vector3d& w);
    Vector3d WorldFromPhysics(const Vector3d& p) const;          // dynamic bodies
    Vector3d PhysicsFromWorld(const Vector3d& w) const;
    Vector3d StaticPhysicsFromWorld(const Vector3d& w) const;    // static geometry (terrain / KSC meshes): w - frameOriginWorld
    // FloatingOrigin.setOffset's shift (FloatingOrigin.cs:181-206) of the non-physics world state by -offsetNonKrakensbane / -offset,
    // and of frameOriginWorld; the bindings' transforms are re-derived from the unchanged bodies. FloatingOrigin calls it.
    void ShiftWorld(const Vector3d& offset, const Vector3d& offsetNonKrakensbane);
    // FrameOffset into every dynamic body (TranslateBodies), FrameOffset reset, the bindings' caches refreshed; false when it was zero
    bool FoldFrameOffset();
    // every Jolt body moved by -delta, frameOriginWorld += delta: the world positions do not change
    void RebasePhysicsOrigin(const Vector3d& delta);
    size_t FoldCount() const { return folds_; }
    size_t PhysicsRebaseCount() const { return physicsRebases_; }

    // ---- OrbitPhysicsManager with the rigid bodies (OrbitPhysicsManager::env) ---------------------------------------------------
    // (the C# names of the private methods they port; OrbitPhysicsManager::env calls them)
    void setRotatingFrame(OrbitPhysicsManager& opm, bool rotatingFrameState);  // OrbitPhysicsManager.cs:235
    void setDominantBody(OrbitPhysicsManager& opm, CelestialBody* body);       // OrbitPhysicsManager.cs:155

    // ---- Krakensbane host ---------------------------------------------------------------------------------------------------
    FrameSnapshot KrakensbaneSnapshot() const;

    // ---- physical objects (FlightGlobals.physicalObjects) ---------------------------------------------------------------------
    // physicalObject.ConvertToPhysicalObject's rigid body: a Moving body of this world at a WORLD position, with a TRUE velocity
    physicalObject& CreatePhysicalObject(BodyDesc desc, const Vector3d& worldPosition, const Vector3d& trueVelocity);
    void DestroyPhysicalObject(physicalObject* o);
    size_t PhysicalObjectCount() const { return objects_.size(); }
    Vector3d PhysicalObjectPosition(const physicalObject& o) const;  // transform.position (world)

    // ---- per tick ----------------------------------------------------------------------------------------------------------
    // FlightScene::FixedUpdate, FrameUpdate, PhysicsStep (Planetarium.FixedUpdate first is the host's)
    void FixedUpdate();
    // seq 284-285 and the frame bookkeeping: Krakensbane.FixedUpdate, FloatingOrigin.FixedUpdate (frame drift, world rebase), the
    // frame-offset fold and the static-collision gate, the physics-origin rebase
    void FrameUpdate();
    // seq 291 PhysicsFixedUpdate: Flush of every binding, PhysicsWorld::Step(Time.fixedDeltaTime), AfterStep (transforms)
    void PhysicsStep();
    void Update();       // FlightScene::Update (with ModuleRCS.Update and FlightIntegrator.Update), physicalObject.Update
    void LateUpdate();   // FlightScene::LateUpdate (pack / unpack, situations)

private:
    void OnVesselUnloaded(Vessel* v);
    void OnVesselLoaded(Vessel* v);
    void OnRemoveVessel(Vessel* v);
    void RunFlightIntegrators();
    void InputFeed();         // WP-20f: the seq 18 slot
    void PartValuesUpdate();  // WP-20f: Part.Update's PartValues refresh, before Vessel.Update
    void RefreshBindings();  // host frames + every binding's caches / transforms
    void MaybeRebasePhysicsOrigin();

    PhysicsWorld* world_;
    PhysicsBubbleOptions options_;
    VesselPhysicsHost host_;
    FlightIntegratorHost fiHost_;
    PartModuleForces moduleForces_;
    ContactHost contacts_;
    VesselResourceHost resources_;  // WP-20g
    Krakensbane krakensbane_;
    FloatingOrigin floatingOrigin_;
    Vector3d frameOriginWorld_;
    std::vector<std::unique_ptr<physicalObject>> objects_;
    bool installed_ = false;
    size_t folds_ = 0, physicsRebases_ = 0;
    FlightInputHandler* flightInputHandler_ = nullptr;  // WP-20f
    // saved seams
    std::function<void(Vessel&, const Vector3d&)> savedSetWorldVelocity_;
    std::function<void(const Vector3d&)> savedKrakensbaneAddFrameVelocity_;
    OrbitPhysicsManager::Environment savedOpm_;
    std::function<void()> savedAfterFlightIntegrator_;
    std::function<void()> savedBeforeFlightIntegrator_;
    std::function<void()> savedBeforeVesselUpdate_;
    std::function<void()> savedAfterPrecalc_;
    // WP-20f
    std::function<void(Vessel&)> savedKillAngularVelocity_;
    std::function<Vector3f()> savedActiveVesselPYR_;
    std::function<bool(float)> savedPitchInput_, savedRollInput_, savedYawInput_;
    std::function<bool()> savedPrecisionMode_;
};

}  // namespace ksp::physics
