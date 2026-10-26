// WP-19a: VesselPrecalculate (VesselPrecalculate.cs, 525 lines): the per-vessel physics-frame pre-pass.
//
// KSP runs one VesselPrecalculate per Vessel at Script Execution Order "Precalc": before the OrbitDriver and long before the
// FlightIntegrator, the rigid-body solver and Krakensbane / FloatingOrigin (measured order: analysis/physics/raw/run1/run1-L_calltrace.csv
// seq 6-285, see flight_scene.hpp). Each FixedUpdate it
//   1. moves an on-rails vessel along its orbit (OrbitDriver.UpdateOrbit in UPDATE mode) or places a landed packed vessel on the surface,
//   2. applies the previous tick's drift correction to the rigid bodies,
//   3. for an unpacked vessel computes the gravitational acceleration at the centre of mass (gAccelTrue), the rotating-frame
//      accelerations (Coriolis, centrifugal), the easing multiplier and the orbit drift compensation, and publishes the result as
//      `integrationAccel` / `gAccel` / `gravityForPos` (consumed by FlightIntegrator.FixedUpdate: WP-20d),
//   4. for a packed vessel (or in Update) refreshes CoM / velocity / angular velocity / MOI (`CalculatePhysicsStats`), the orbit state,
//      `UpdatePosVel`, `UpdateAcceleration`, then `Vessel.CheckKill`.
//
// kspcore split. The arithmetic is in `PrecalcMath` (free functions over plain values: the unit under the Python reference
// tools/wp19a/flight_reference.py at 0 ULP); the class is the C# state machine on top of the registry, the OrbitDriver and the vessel.
// Everything that needs a rigid body goes through the `VesselEnvironment` seams (PHYSICS-SPIKE.md 5.3 `VesselPhysics`, WP-20b):
//   * env.partRigidbody(part)          Part.rb: world CoM, velocity, angular velocity, inertia (default: PartRigidbodyState::FromPart, no motion)
//   * env.rootRigidbody(vessel, CoM)   rootPart.rb.GetPointVelocity(CoM) and rootPart.rb.angularVelocity (packed / mass-less branch)
//   * env.offsetVelocity(vessel, dv)   Vessel.OffsetVelocity (Vessel.cs:3742): add dv to every part rigid body (the drift correction)
//   * env.krakensbaneFrameVelocity     default Krakensbane::GetFrameVelocity()
// A vessel that is unpacked but has no physics body behind these hooks keeps zero velocity, as the C# does for rb == null.
//
// Deviations from the C# (all documented where they occur):
//   * UnityEngine components: no MonoBehaviour; the host (FlightScene) calls FixedUpdate / Update / OnDestroy.
//   * ScreenMessages / InputLockManager for the "PhysicsEase" message and control lock are the host's: the booleans `easingLockOn` and the
//     requested lock are kept (`InputLocks`), events fire as in the C#.
//   * `RunFirst`, `Awake` (GameEvents.onDockingComplete subscription) are `RunFirst()` and the constructor / `Subscribe()`.
#pragma once

#include <string>

#include <functional>

#include "ksp/events/game_events.hpp"
#include "ksp/math/quaterniond.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/orbit/orbit_driver.hpp"

namespace ksp {

class Vessel;
class Part;
class CelestialBody;

namespace PrecalcMath {

// ---- FlightGlobals.cs:1347 getGeeForceAtPosition(pos, mainBody) over plain values -----------------------------------------------------
// `pos -= mainBody.position; sqrMagnitude = pos.sqrMagnitude; sqrMagnitude *= Math.Sqrt(sqrMagnitude); return pos * (0.0 - g) / sqrMagnitude`
Vector3d GeeForceAtPosition(const Vector3d& pos, const Vector3d& bodyPosition, double gMagnitudeAtCenter);

// ---- the drift decision (VesselPrecalculate.cs:278-290) ----------------------------------------------------------------------------------
enum class DriftAction : int { ResetWait = 0, CountDown = 1, Correct = 2 };
struct DriftCheck {
    DriftAction action = DriftAction::ResetWait;
    int framesUntilCorrect = 0;  // the new value of the field
};
// Inputs are the swizzled vectors exactly as the C# holds them: `cposMinusCoM = cpos - (CoMD - referenceBody.position)`,
// `cvelRel = cvel - curRotFrameVel - velocityD` (after the three `Swizzle()` calls).
// `if (!driftAlwaysPassThreshChecks && (!(sqr(cposMinusCoM) < T) || sqr(cvelRel) >= T)) wait; else if (frames > 0) frames--; else correct`
DriftCheck CheckDrift(bool driftAlwaysPassThreshChecks, const Vector3d& cposMinusCoM, const Vector3d& cvelRel, double sqrThreshold,
                      int framesToWait, int framesUntilCorrect);

// ---- drift compensation (VesselPrecalculate.cs:293-314) ------------------------------------------------------------------------------------
struct DriftInputs {
    Vector3d railsPosNext, railsVelNext, rotFrameNext;  // after Swizzle()
    Vector3d curRotFrameVel;                            // after Swizzle()
    Vector3d referenceBodyPosition, CoMD, velocityD;
    double fDeltaTime = 0.02, fDeltaTimeRecip = 50.0;
    double GraviticForceMultiplier = 1.0, gravityMultiplier = 1.0;
};
struct DriftResult {
    Vector3d preIntegrationVelocityOffset, postIntegrationVelocityCorrection, integrationAccel, gAccel, gravityForPos;
    bool invalid = false;  // IsInvalid(pre) || IsInvalid(post): the C# zeroes both and clears wasCorrectingDrift
};
DriftResult ApplyDriftCompensation(const DriftInputs& in);

// ---- gravity integration without drift compensation (VesselPrecalculate.cs:317-343) -------------------------------------------------------
struct GravityInputs {
    Vector3d gAccelTrue;
    bool inverseRotation = false;
    Vector3d coriolis, centrifugal;  // FlightGlobals.getCoriolisAcc / getCentrifugalAcc (only read when inverseRotation)
    double GraviticForceMultiplier = 1.0, gravityMultiplier = 1.0;
    Vector3d CoMD, velocityD, mainBodyPosition;
    double fDeltaTime = 0.02;
    Vector3d rotFrameVel;  // vessel.orbit.GetRotFrameVel(mainBody), after Swizzle()
};
struct GravityResult {
    Vector3d integrationAccel, gAccel, gravityForPos;
};
// `rotFrameVelAtPos(refPos)` is vessel.orbit.GetRotFrameVelAtPos(mainBody, refPos) for the SWIZZLED refPos, returned UNswizzled (the function
// swizzles the result itself, VesselPrecalculate.cs:333-335). `coriolis` / `centrifugal` are filled in the result of the call.
GravityResult IntegrationGravity(const GravityInputs& in, const std::function<Vector3d(const Vector3d& swizzledRefPos)>& rotFrameVelAtPos);

}  // namespace PrecalcMath

class VesselPrecalculate {
public:
    // ---- fields (VesselPrecalculate.cs:7-98, same names) --------------------------------------------------------------------------
    Vessel* vessel = nullptr;
    bool physStatsNotDoneInUpdate = true;
    bool ranFixedThisFrame = true;
    bool packedInFixed = true;
    double fDeltaTime = 0.0;
    double lastUT = 0.0;
    Vector3d worldSurfacePos;
    QuaternionD worldSurfaceRot{0.0, 0.0, 0.0, 0.0};  // `default(QuaternionD)`: all zero until SetLandedPosRot runs
    Vector3d gAccel, gAccelTrue, coriolis, centrifugal, integrationAccel;
    Vector3d postIntegrationVelocityCorrection;
    Vector3d preIntegrationVelocityOffset;
    static inline bool allowDriftCompensation = true;
    static inline bool driftAlwaysPassThreshChecks = false;
    static inline bool disableRunInUpdate = false;
    double easingFrameIncrease = 0.0625;
    bool wasCorrectingDrift = false;
    bool easeDocking = false;
    bool firstStatsRunComplete = false;
    OrbitDriver::UpdateMode lastMode = OrbitDriver::UpdateMode::TRACK_Phys;
    bool railsSetPosRot = true;
    bool updateOrbit = true;
    bool calculateGravity = true;

    // The control lock / screen message the C# sets when easing starts on the active vessel (InputLockManager / ScreenMessages are the
    // host's): the request is recorded so the host can show it.
    bool easingLockOn = false;
    std::string easingMessage;  // WP-19c: its text, cacheAutoLOC_6003101 (VesselPrecalculate.cs:80 / :521: Localizer #autoLOC_6003101)
    float easingMessageSeconds = 0.0f;  // (float)(1.0 / easingFrameIncrease * 0.02 + 0.5) of the last StartEasing on the active vessel

    explicit VesselPrecalculate(Vessel* v = nullptr);
    ~VesselPrecalculate();
    void OnDestroy();  // VesselPrecalculate.cs:109 (the Vessel destructor calls it first)
    VesselPrecalculate(const VesselPrecalculate&) = delete;
    VesselPrecalculate& operator=(const VesselPrecalculate&) = delete;

    // VesselPrecalculate.Vessel
    Vessel* GetVessel() const { return vessel; }
    // isEasingGravity (cs:103-124)
    bool isEasingGravity() const { return easing_; }
    void set_isEasingGravity(bool value);

    void RunFirst();                          // cs:132
    void FixedUpdate();                       // cs:155
    void MainPhysics(bool doKillChecks);      // cs:160
    void Update();                            // cs:221
    void ApplyVelocityCorrection();           // cs:240
    void GoOnRails();                         // cs:251
    void GoOffRails();                        // cs:264
    void CalculateGravity();                  // cs:293 (named CalculateGravity in the C#)
    void SetLandedPosRot();                   // cs:376
    void CalculatePhysicsStats();             // cs:387 (Vessel::CalculatePhysicsStats with the part-body hooks of Vessel::env)

    // The same statistics step used by Vessel.GetWorldPos3D before the first run
    double fDeltaTimeRecip() const { return fDeltaTimeRecip_; }

private:
    void onDockingComplete(events::GameEvents::FromToAction<Part*, Part*> fromTo);  // cs:513 (GameEvents.onDockingComplete)
    void StartEasing();  // cs:269
    void StopEasing();   // cs:283

    bool easing_ = false;
    bool easingLockOn_ = false;
    bool correctDriftThisFrame_ = false;
    double fDeltaTimeRecip_ = 0.0;
    Vector3d railsPosNext_, railsVelNext_, cpos_, cvel_, curRotFrameVel_, rotFrameNext_;
    int framesUntilCorrect_ = 0;
};

}  // namespace ksp
