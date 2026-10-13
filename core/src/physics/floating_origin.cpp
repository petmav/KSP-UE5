// WP-20a / WP-19b: FloatingOrigin without the shift of the physics bodies. Port of Assembly-CSharp/FloatingOrigin.cs; see
// ksp/physics/floating_origin.hpp.
#include "ksp/physics/floating_origin.hpp"

#include <cmath>

#include "ksp/events/game_events.hpp"
#include "ksp/math/mathfx.hpp"
#include "ksp/physics/collision_enhancer.hpp"

namespace ksp {

FloatingOrigin::~FloatingOrigin() { OnDestroy(); }

// FloatingOrigin.cs:102 (the event subscriptions of Awake are scene-reset hooks of the terrain shader offset: not ported)
void FloatingOrigin::Awake() {
    fetch_ = this;
    offset = Vector3d::zero();
    thresholdSqr = threshold * threshold;
    velForContinuousSqr = velForContinuous * velForContinuous;
}

// FloatingOrigin.cs:113
void FloatingOrigin::OnDestroy() {
    if (fetch_ != nullptr && fetch_ == this) fetch_ = nullptr;
}

// FloatingOrigin.cs:125. Two departures from the C# (see the header comment): the Krakensbane branch (:146-150, setOffset every tick
// while outOfFrameAdditional != 0) only accumulates the frame drift into FrameOffset(), and setOffset shifts the non-physics world
// through `shiftWorld` (never a physics body). Without the WP-19b hooks this is WP-20a's bookkeeping alone.
void FloatingOrigin::FixedUpdate() {
    if (!ready || !ready()) return;  // FlightGlobals.ready (unset = not ready)
    const bool setOffsetThisFrame = SetOffsetThisFrame_;
    SetOffsetThisFrame_ = false;
    if (!bypassAuto && continuousCondition) continuous = continuousCondition();
    const bool flag = !outOfFrameAdditional.IsZero();
    bool flag2 = false;
    if (canEngageThisFrame && activeVesselPosition) {
        // FlightGlobals.ActiveVessel.transform.position.sqrMagnitude > thresholdSqr: Vector3.sqrMagnitude (float) against a float
        flag2 = continuous || activeVesselPosition().sqrMagnitude() > thresholdSqr;
    }
    Vector3d consumed;
    if (canEngageThisFrame && flag) {
        // KSP moved the bodies by outOfFrameAdditional (offsetNonKrakensbane); here the same quantity accumulates in the frame offset
        frameOffset_ += outOfFrameAdditional;
        consumed = outOfFrameAdditional;
    }
    if (flag2) {
        physics::CollisionEnhancer::bypass = true;  // WP-20e (:138)
        setOffset(Vector3d(activeVesselPosition()), Vector3d::zero());  // C#: (position, outOfFrameAdditional): the drift is FrameOffset
        offsetNonKrakensbane = offset + consumed;
    } else {
        physics::CollisionEnhancer::bypass = false;  // WP-20e (:143 / :148)
        offset.Zero();
        reverseoffset.Zero();
        if (canEngageThisFrame && flag) offsetNonKrakensbane = consumed;
    }
    // FloatingOrigin.cs:157-160
    if (setOffsetThisFrame && !SetOffsetThisFrame_ && activeVesselLandedOrSlow && activeVesselLandedOrSlow() &&
        preciseUpdateQuadPositions) {
        preciseUpdateQuadPositions();
    }
    outOfFrameAdditional.Zero();
    canEngageThisFrame = true;
}

Vector3d FloatingOrigin::Offset() { return fetch_ ? fetch_->offset : Vector3d::zero(); }
Vector3d FloatingOrigin::ReverseOffset() { return fetch_ ? fetch_->reverseoffset : Vector3d::zero(); }
Vector3d FloatingOrigin::OffsetNonKrakensbane() { return fetch_ ? fetch_->offsetNonKrakensbane : Vector3d::zero(); }

// FloatingOrigin.cs:315
void FloatingOrigin::SetOffset(const Vector3d& refPos) {
    if (fetch_) fetch_->setOffset(refPos, Vector3d::zero());
}

// FloatingOrigin.cs:323
void FloatingOrigin::SetOutOfFrameOffset(const Vector3d& add) {
    if (fetch_) fetch_->outOfFrameAdditional += add;
}

// FloatingOrigin.cs:331
void FloatingOrigin::SetSafeToEngage(bool val) {
    if (fetch_) fetch_->canEngageThisFrame = val;
}

// FloatingOrigin.cs:166 (validation and the offset fields; the shift of Bodies / Vessels (:181-206) is `shiftWorld`, the particle /
// FX / terrain-shader offsets (:207-288) are the shell's)
void FloatingOrigin::setOffset(const Vector3d& refPos, const Vector3d& nonFrame) {
    if (VectorExtensions::IsInvalid(refPos)) return;
    if (std::isinf(refPos.sqrMagnitude())) return;  // double.IsInfinity (Debug.Break in the C#)
    SetOffsetThisFrame_ = true;
    offset = refPos;
    reverseoffset = Vector3d(0.0 - refPos.x, 0.0 - refPos.y, 0.0 - refPos.z);
    offsetNonKrakensbane = offset + nonFrame;
    if (shiftWorld) {
        shiftWorld(offset, offsetNonKrakensbane);
        ++shiftCount_;
        GameEvents::onFloatingOriginShift.Fire(offset, nonFrame);
    }
}

}  // namespace ksp
