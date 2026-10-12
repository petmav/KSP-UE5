// WP-20a: FloatingOrigin (Assembly-CSharp/FloatingOrigin.cs) without the shift of the PHYSICS bodies; WP-19b: the rebase of the
// world origin for the non-physics state.
//
// KSP keeps float Transform positions near the active vessel by shifting EVERY object (bodies, vessels, particles, FX, scene
// roots) by the vessel's position whenever it drifts past `threshold` (500 m), or every tick while Krakensbane's frame velocity
// is nonzero (`setOffset`, FloatingOrigin.cs:166-290; cost: 3.3 ms script + ~2.25 ms PhysX per tick on the 457-part ascent,
// PHYSICS-SPIKE.md). kspcore never shifts a physics body: positions are doubles in the physics frame (physics_world.hpp). What this
// class keeps:
//
//   * FrameOffset(): the accumulated `outOfFrameAdditional` (= sum of Krakensbane.FrameVel * fixedDeltaTime), i.e. how far
//     the Galilean frame has drifted. A body's TRUE position in the physics frame is  p_world + FrameOffset  (its velocity is
//     v_world + Krakensbane::GetFrameVelocity()). One double vector, O(1), replaces the per-tick O(objects) shift. The physics
//     bubble (WP-19b, physics_bubble.hpp) folds it into the dynamic bodies when the frame velocity is back to zero (ResetFrameOffset).
//   * Offset() / ReverseOffset() / OffsetNonKrakensbane(): the values the C# exposes.
//   * canEngageThisFrame / SetSafeToEngage, SetOutOfFrameOffset: unchanged semantics.
//   * WP-19b: the world-origin rebase. With the hooks `activeVesselPosition` and `shiftWorld` set (the physics bubble sets them),
//     FixedUpdate applies KSP's own decision (FloatingOrigin.cs:131-140: `continuous`, or the active vessel farther than `threshold`
//     from the origin, while canEngageThisFrame) and setOffset hands the offset to `shiftWorld`, which moves the NON-physics world
//     state (CelestialBody.position, vessel transforms, CoMD / CoM) and the bubble's physics-origin anchor: float fields stay near
//     the origin as in KSP, the Jolt bodies stay where they are. Krakensbane's per-tick branch (FloatingOrigin.cs:146-150:
//     setOffset every tick while outOfFrameAdditional != 0) is NOT a trigger: its position consequence is FrameOffset.
//     SetOffset (PSystemSetup / FlightDriver / ResetOffset) shifts through the same hook. Unset hooks = WP-20a's behaviour (no shift).
//
// Dropped on purpose: CollisionEnhancer.bypass, particle and FX offsets, TerrainShaderOffset (the shell's camera-relative rendering
// owns those).
#pragma once

#include <functional>

#include "ksp/math/vector3d.hpp"

namespace ksp {

class FloatingOrigin {
public:
    // FloatingOrigin.cs:6-34 (public fields, original defaults)
    float threshold = 500.0f;
    float thresholdSqr = 0.0f;  // Awake: threshold * threshold
    double velForContinuous = 5000.0;
    double velForContinuousSqr = 0.0;  // Awake
    double CoMRecalcOffsetMaxSqr = 100000000.0;
    bool continuous = false;
    bool bypassAuto = false;
    Vector3d offset;
    Vector3d offsetNonKrakensbane;
    Vector3d reverseoffset;
    Vector3d outOfFrameAdditional;
    bool canEngageThisFrame = true;

    // kspcore additions
    // Sum of every out-of-frame offset consumed by FixedUpdate (see the header comment).
    Vector3d FrameOffset() const { return frameOffset_; }
    // The physics bubble folded the frame drift into the dynamic bodies: FrameOffset() is zero again.
    void ResetFrameOffset() { frameOffset_ = Vector3d(); }

    // Host hooks (unset: not ready / nothing to do)
    std::function<bool()> ready;  // FlightGlobals.ready
    // FixedUpdate:  FlightGlobals.ActiveVessel.orbit.referenceBody.PreciseUpdateQuadPositions() after a SetOffset
    // (FloatingOrigin.cs:157-160), condition `situation == LANDED || ship_srfVelocity.magnitude < 100` evaluated by the host.
    std::function<void()> preciseUpdateQuadPositions;
    std::function<bool()> activeVesselLandedOrSlow;
    // WP-19b. FlightGlobals.ActiveVessel.transform.position (the Unity float world position; unset = no rebase decision at all,
    // WP-20a's behaviour)
    std::function<Vector3f()> activeVesselPosition;
    // FloatingOrigin.cs:133 `continuous = (TimeWarp.CurrentRate > TimeWarp.MaxPhysicsRate && WarpMode == HIGH &&
    // !ActiveVessel.LandedOrSplashed) || FlightGlobals.ship_velocity.sqrMagnitude > velForContinuousSqr` (unset: continuous stays)
    std::function<bool()> continuousCondition;
    // the shift of setOffset (FloatingOrigin.cs:181-288) for the non-physics state: subtract `offsetNonKrakensbane` from every world
    // position the host keeps (unset: nothing moves). `offset` and `nonFrame` are setOffset's arguments' results.
    std::function<void(const Vector3d& offset, const Vector3d& offsetNonKrakensbane)> shiftWorld;

    FloatingOrigin() = default;
    ~FloatingOrigin();
    FloatingOrigin(const FloatingOrigin&) = delete;
    FloatingOrigin& operator=(const FloatingOrigin&) = delete;

    // FloatingOrigin.cs:102 Awake (offset.Zero; thresholds squared; `fetch` = this) / :113 OnDestroy
    void Awake();
    void OnDestroy();
    static FloatingOrigin* Instance() { return fetch_; }

    // FloatingOrigin.cs:125 FixedUpdate: consumes outOfFrameAdditional into FrameOffset(); with the WP-19b hooks set, rebases the
    // world origin by KSP's threshold / continuous rule (see the header comment).
    void FixedUpdate();
    // the number of world rebases so far (setOffset calls that reached shiftWorld)
    size_t ShiftCount() const { return shiftCount_; }

    // FloatingOrigin.cs:46-80 (static properties; zero without an instance, like the C# `if (!fetch)`)
    static Vector3d Offset();
    static Vector3d ReverseOffset();
    static Vector3d OffsetNonKrakensbane();
    // FloatingOrigin.cs:315 SetOffset / :323 SetOutOfFrameOffset / :331 SetSafeToEngage
    static void SetOffset(const Vector3d& refPos);
    static void SetOutOfFrameOffset(const Vector3d& add);
    static void SetSafeToEngage(bool val);

private:
    // FloatingOrigin.cs:166 setOffset: validation, the offset fields, the "set this frame" flag, then the shift through `shiftWorld`
    // and onFloatingOriginShift (only when a shift happened).
    void setOffset(const Vector3d& refPos, const Vector3d& nonFrame);
    static inline FloatingOrigin* fetch_ = nullptr;
    bool SetOffsetThisFrame_ = false;
    Vector3d frameOffset_;
    size_t shiftCount_ = 0;
};

}  // namespace ksp
