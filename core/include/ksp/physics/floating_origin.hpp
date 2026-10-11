// WP-20a: FloatingOrigin (Assembly-CSharp/FloatingOrigin.cs) without the world shift.
//
// KSP keeps float Transform positions near the active vessel by shifting EVERY object (bodies, vessels, particles, FX, scene
// roots) by the vessel's position whenever it drifts past `threshold` (500 m), or every tick while Krakensbane's frame velocity
// is nonzero (`setOffset`, FloatingOrigin.cs:166-290; cost: 3.3 ms script + ~2.25 ms PhysX per tick on the 457-part ascent,
// PHYSICS-SPIKE.md). kspcore does not shift: positions are doubles in the physics frame (physics_world.hpp), so there is nothing
// to keep small. What this class keeps is the API the rest of KSP reads and the one piece of state the shift used to carry:
//
//   * FrameOffset(): the accumulated `outOfFrameAdditional` (= sum of Krakensbane.FrameVel * fixedDeltaTime), i.e. how far
//     the Galilean frame has drifted. A body's TRUE position in the physics frame is  p_world + FrameOffset  (its velocity is
//     v_world + Krakensbane::GetFrameVelocity()). One double vector, O(1), replaces the per-tick O(objects) shift.
//   * Offset() / ReverseOffset() / OffsetNonKrakensbane(): the values the C# exposes. The core never applies them to bodies;
//     they record the last origin the host asked for with SetOffset (the rendering shell's camera-relative origin, scene
//     focus changes, FloatingOrigin.SetOffset at PSystemSetup / FlightDriver) and read as zero otherwise.
//   * canEngageThisFrame / SetSafeToEngage, SetOutOfFrameOffset: unchanged semantics.
//
// Dropped on purpose: the threshold / continuous / bypassAuto decision (nothing ever shifts), CollisionEnhancer.bypass,
// Vessel.SetPosition / Part transform shifts, particle and FX offsets, TerrainShaderOffset (the shell's camera-relative
// rendering owns those), and the onFloatingOriginShift event (nothing shifts, so no subscriber has anything to do).
#pragma once

#include <functional>

#include "ksp/math/vector3d.hpp"

namespace ksp {

class FloatingOrigin {
public:
    // FloatingOrigin.cs:22-30 (public fields)
    Vector3d offset;
    Vector3d offsetNonKrakensbane;
    Vector3d reverseoffset;
    Vector3d outOfFrameAdditional;
    bool canEngageThisFrame = true;

    // kspcore additions
    // Sum of every out-of-frame offset consumed by FixedUpdate (see the header comment).
    Vector3d FrameOffset() const { return frameOffset_; }

    // Host hooks (unset: not ready / nothing to do)
    std::function<bool()> ready;  // FlightGlobals.ready
    // FixedUpdate:  FlightGlobals.ActiveVessel.orbit.referenceBody.PreciseUpdateQuadPositions() after a SetOffset
    // (FloatingOrigin.cs:146-149), condition `situation == LANDED || ship_srfVelocity.magnitude < 100` evaluated by the host.
    std::function<void()> preciseUpdateQuadPositions;
    std::function<bool()> activeVesselLandedOrSlow;

    FloatingOrigin() = default;
    ~FloatingOrigin();
    FloatingOrigin(const FloatingOrigin&) = delete;
    FloatingOrigin& operator=(const FloatingOrigin&) = delete;

    // FloatingOrigin.cs:102 Awake (offset.Zero; `fetch` = this) / :113 OnDestroy
    void Awake();
    void OnDestroy();
    static FloatingOrigin* Instance() { return fetch_; }

    // FloatingOrigin.cs:125 FixedUpdate, minus the shift (see the header comment): consumes outOfFrameAdditional into FrameOffset().
    void FixedUpdate();

    // FloatingOrigin.cs:46-80 (static properties; zero without an instance, like the C# `if (!fetch)`)
    static Vector3d Offset();
    static Vector3d ReverseOffset();
    static Vector3d OffsetNonKrakensbane();
    // FloatingOrigin.cs:315 SetOffset / :323 SetOutOfFrameOffset / :331 SetSafeToEngage
    static void SetOffset(const Vector3d& refPos);
    static void SetOutOfFrameOffset(const Vector3d& add);
    static void SetSafeToEngage(bool val);

private:
    // FloatingOrigin.cs:166 setOffset without the shift of bodies / vessels / particles: records the offsets and the
    // "set this frame" flag.
    void setOffset(const Vector3d& refPos, const Vector3d& nonFrame);
    static inline FloatingOrigin* fetch_ = nullptr;
    bool SetOffsetThisFrame_ = false;
    Vector3d frameOffset_;
};

}  // namespace ksp
