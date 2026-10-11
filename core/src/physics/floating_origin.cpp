// WP-20a: FloatingOrigin without the world shift. Port of Assembly-CSharp/FloatingOrigin.cs; see ksp/physics/floating_origin.hpp.
#include "ksp/physics/floating_origin.hpp"

#include <limits>

#include "ksp/math/mathfx.hpp"

namespace ksp {

FloatingOrigin::~FloatingOrigin() { OnDestroy(); }

// FloatingOrigin.cs:102 (the event subscriptions of Awake are scene-reset hooks of the terrain shader offset: not ported)
void FloatingOrigin::Awake() {
    fetch_ = this;
    offset = Vector3d::zero();
}

// FloatingOrigin.cs:113
void FloatingOrigin::OnDestroy() {
    if (fetch_ != nullptr && fetch_ == this) fetch_ = nullptr;
}

// FloatingOrigin.cs:125. The decision `flag2 = continuous || ActiveVessel.transform.position.sqrMagnitude > thresholdSqr`
// and the two setOffset(ActiveVessel.transform.position, outOfFrameAdditional) calls are the shift and are not ported; what
// remains is the bookkeeping of the frame drift (frameOffset_) and the per-tick reset of the engage flag.
void FloatingOrigin::FixedUpdate() {
    if (!ready || !ready()) return;  // FlightGlobals.ready (unset = not ready)
    bool setOffsetThisFrame = SetOffsetThisFrame_;
    SetOffsetThisFrame_ = false;
    // C#: setOffset(ActiveVessel position, outOfFrameAdditional) when a shift engages, else offset.Zero(). Nothing shifts here,
    // so the offset the host set with SetOffset is a one-tick value, as in the C#.
    offset.Zero();
    reverseoffset.Zero();
    if (canEngageThisFrame && !outOfFrameAdditional.IsZero()) {
        // KSP moved the bodies by outOfFrameAdditional (offsetNonKrakensbane); the same quantity accumulates in the frame offset.
        frameOffset_ += outOfFrameAdditional;
        offsetNonKrakensbane = outOfFrameAdditional;
    }
    // FloatingOrigin.cs:146-149
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

// FloatingOrigin.cs:166 (validation and the offset fields; the shift of Bodies / Vessels / particles is not ported)
void FloatingOrigin::setOffset(const Vector3d& refPos, const Vector3d& nonFrame) {
    if (VectorExtensions::IsInvalid(refPos)) return;
    if (refPos.sqrMagnitude() == std::numeric_limits<double>::infinity()) return;  // double.IsInfinity (Debug.Break in the C#)
    SetOffsetThisFrame_ = true;
    offset = refPos;
    reverseoffset = Vector3d(0.0 - refPos.x, 0.0 - refPos.y, 0.0 - refPos.z);
    offsetNonKrakensbane = offset + nonFrame;
}

}  // namespace ksp
