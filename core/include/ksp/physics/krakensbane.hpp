// WP-20a: Krakensbane (Assembly-CSharp/Krakensbane.cs), ported as the Galilean FRAME VELOCITY bookkeeping of the physics world.
//
// What KSP does (Krakensbane.cs:55-85, 121-173): when the active vessel's velocity passes MaxV (5 m/s) and it is safe, the
// excess velocity is subtracted from every unpacked vessel's rigidbodies (`Vessel.ChangeWorldVelocity(-excess)`, Vessel.cs:3197)
// and from the free physical objects, and accumulated in FrameVel; the true velocity of anything is `rb.velocity + FrameVel`
// (`velocityD = rb_velocity + Krakensbane.GetFrameVelocity()`, Vessel.cs UpdatePosVel). FrameVel * fixedDeltaTime is then
// handed to FloatingOrigin (`SetOutOfFrameOffset`, Krakensbane.cs:83), which shifts the whole scene back by it every tick.
//
// What kspcore keeps and what it drops (PHYSICS-SPIKE.md 5.1):
//   * KEPT, ported statement by statement: the engage / disengage rule (FixedUpdate, SafeToEngage), AddExcess, Zero,
//     GetFrameVelocity / GetFrameVelocityV3f / GetLastCorrection / ResetVelocityFrame / AddFrameVelocity (39 call sites in 30
//     files read GetFrameVelocity and port unchanged), the onKrakensbaneEngage / onKrakensbaneDisengage events.
//   * DROPPED: the per-tick world shift. FrameVel is a velocity offset only: nothing moves because of it. The position
//     consequence of a nonzero FrameVel (the world "drifting" by FrameVel * dt per tick) is one accumulated double vector,
//     FloatingOrigin::FrameOffset() (floating_origin.hpp), not a write to every object.
//   * Every dependency on Unity objects (FlightGlobals.ready / ActiveVessel / VesselsLoaded / physicalObjects, Vessel
//     velocities, Rigidbody.AddForce) is the KrakensbaneHost below; the physics world implements the velocity write
//     (PhysicsWorld::AddVelocityChangeToAllMoving) and WP-19/WP-20b wire the vessels.
//
// Everything here is double arithmetic (the C# is double: Vector3d, `(double)Time.fixedDeltaTime`); there is no float32 statement
// in Krakensbane.cs, so the Mono float rule has nothing to apply to.
#pragma once

#include <cstddef>
#include <functional>
#include <limits>
#include <vector>

#include "ksp/math/vector3d.hpp"

namespace ksp {

// The per-vessel facts Krakensbane reads (Vessel.packed, Vessel.state == DEAD, Vessel.LandedOrSplashed, Vessel.radarAltitude).
struct FrameVesselState {
    bool isActive = false;  // vessel == FlightGlobals.ActiveVessel
    bool packed = false;
    bool dead = false;      // state == Vessel.State.DEAD
    bool landedOrSplashed = false;
    double radarAltitude = 0.0;
};

// One read of the flight state, taken at the top of Krakensbane.FixedUpdate / AddFrameVelocity.
struct FrameSnapshot {
    bool ready = false;                 // FlightGlobals.ready
    bool hasActiveVessel = false;       // FlightGlobals.ActiveVessel != null
    bool activeHasRigidbody = false;    // ActiveVessel.GetComponent<Rigidbody>()
    FrameVesselState active;            // the active vessel's own state
    Vector3d velocityD;                 // ActiveVessel.velocityD
    Vector3d rb_velocityD;              // ActiveVessel.rb_velocityD
    Vector3d upAxis;                    // ActiveVessel.upAxis
    std::vector<FrameVesselState> loaded;  // FlightGlobals.VesselsLoaded, in list order
};

struct KrakensbaneHost {
    // unset = FlightGlobals not ready (FixedUpdate returns, SafeToEngage is false)
    std::function<FrameSnapshot()> snapshot;
    // Vessel.ChangeWorldVelocity(dv) on VesselsLoaded[loadedIndex] (Vessel.cs:3197): every part rigidbody velocity += dv.
    std::function<void(size_t loadedIndex, const Vector3d& dv)> changeWorldVelocity;
    // FlightGlobals.physicalObjects: rb.AddForce(dv, ForceMode.VelocityChange) on each (Krakensbane.cs:139-147, 163-171).
    std::function<void(const Vector3d& dv)> physicalObjectsVelocityChange;
};

class Krakensbane {
public:
    // Krakensbane.cs:7-25 (public fields, original defaults)
    double MaxV = 5.0;
    double MaxVSqr = std::numeric_limits<double>::max();
    double extraAltOffsetForVel = 100.0;
    double altThreshold = 200.0;
    double altThresholdAlone = 50.0;
    Vector3d FrameVel;
    Vector3d totalVel;
    Vector3d RBVel;
    Vector3d excessV;
    Vector3d lastCorrection;

    // kspcore option, 0 = off = KSP's rule. KSP moves the whole rigidbody velocity of the active vessel into FrameVel on EVERY
    // tick while the vessel is faster than MaxV (FixedUpdate:67-70). With recentreSpeed > 0 an engaged frame velocity is only
    // re-centred (AddExcess) when the vessel's physics-frame speed |rb_velocityD| exceeds it (PHYSICS-SPIKE.md used 50 m/s), so
    // a coasting vessel costs no velocity writes; the first engagement and every disengagement are unchanged. True velocities are
    // identical either way (FrameVel + rb velocity), only the split between the two differs.
    double recentreSpeed = 0.0;

    KrakensbaneHost host;

    Krakensbane() = default;
    ~Krakensbane();
    Krakensbane(const Krakensbane&) = delete;
    Krakensbane& operator=(const Krakensbane&) = delete;

    // Krakensbane.cs:31 Awake: first instance becomes `fetch` and sets MaxVSqr; a second one is destroyed (returns false).
    bool Awake();
    // Krakensbane.cs:42 OnDestroy
    void OnDestroy();
    static Krakensbane* Instance() { return fetch_; }
    // Krakensbane.cs:50 Start
    void Start();
    // Krakensbane.cs:55 FixedUpdate. Runs in the FixedUpdate group (Timing slot of the Krakensbane component; the caller places it).
    void FixedUpdate();
    // Krakensbane.cs:87
    bool SafeToEngage(const FrameSnapshot& s, bool& safeForFloatingOrigin);
    // Krakensbane.cs:121 (uses the snapshot of the current tick for the loaded-vessel list)
    void AddExcess(const Vector3d& vel);
    // Krakensbane.cs:150
    void Zero();

    // Krakensbane.cs:29, 175-208 (static API; all require an instance, like `fetch` in the C#)
    static double SqrThreshold();
    static Vector3d GetFrameVelocity();
    static Vector3f GetFrameVelocityV3f();
    static Vector3d GetLastCorrection();
    static void ResetVelocityFrame(bool resetObjects);
    static void AddFrameVelocity(const Vector3d& vel);

private:
    FrameSnapshot Snapshot() const;
    static inline Krakensbane* fetch_ = nullptr;
    size_t loadedVesselsCount_ = 0;
    FrameSnapshot lastSnapshot_;
};

}  // namespace ksp
