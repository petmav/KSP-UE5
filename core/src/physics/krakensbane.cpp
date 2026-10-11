// WP-20a: Krakensbane. Port of Assembly-CSharp/Krakensbane.cs; see ksp/physics/krakensbane.hpp for what is kept and dropped.
#include "ksp/physics/krakensbane.hpp"

#include "ksp/core/unity_time.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/physics/floating_origin.hpp"

namespace ksp {

Krakensbane::~Krakensbane() { OnDestroy(); }

// Krakensbane.cs:31
bool Krakensbane::Awake() {
    if (fetch_ != nullptr) return false;  // Object.Destroy(this)
    fetch_ = this;
    MaxVSqr = MaxV * MaxV;
    return true;
}

// Krakensbane.cs:42
void Krakensbane::OnDestroy() {
    if (fetch_ != nullptr && fetch_ == this) fetch_ = nullptr;
}

// Krakensbane.cs:50
void Krakensbane::Start() { FrameVel = Vector3d::zero(); }

FrameSnapshot Krakensbane::Snapshot() const { return host.snapshot ? host.snapshot() : FrameSnapshot{}; }

// Krakensbane.cs:55
void Krakensbane::FixedUpdate() {
    FrameSnapshot s = Snapshot();
    if (!s.ready || !s.hasActiveVessel || !s.activeHasRigidbody) return;
    lastCorrection.Zero();
    bool safeForFloatingOrigin;
    bool flag = SafeToEngage(s, safeForFloatingOrigin);
    FloatingOrigin::SetSafeToEngage(safeForFloatingOrigin);
    if (!s.active.packed) {
        if (flag && s.velocityD.sqrMagnitude() > MaxVSqr) {
            if (recentreSpeed <= 0.0 || FrameVel.IsZero() || s.rb_velocityD.sqrMagnitude() > recentreSpeed * recentreSpeed)
                AddExcess(s.rb_velocityD);
        } else if (!FrameVel.IsZero()) {
            Zero();
        }
    } else {
        FrameVel.Zero();
        excessV.Zero();
    }
    if (!FrameVel.IsZero()) {
        FloatingOrigin::SetOutOfFrameOffset(FrameVel * static_cast<double>(Time::fixedDeltaTime));
    }
}

// Krakensbane.cs:87. The C# do { ... continue; } while (cond) is unrolled: `continue` jumps to the loop condition, which keeps
// iterating while the vessel is the active one or is airborne above altThreshold; the first other vessel that is landed /
// splashed or low ends the loop with "not safe for the floating origin".
bool Krakensbane::SafeToEngage(const FrameSnapshot& s, bool& safeForFloatingOrigin) {
    safeForFloatingOrigin = false;
    lastSnapshot_ = s;
    loadedVesselsCount_ = s.loaded.size();
    if (s.active.dead) return false;  // ActiveVessel.state == Vessel.State.DEAD
    safeForFloatingOrigin = true;
    size_t index = loadedVesselsCount_;
    for (;;) {
        if (index > 0) {
            --index;
            const FrameVesselState& vessel = s.loaded[index];
            if (vessel.isActive || (!vessel.landedOrSplashed && vessel.radarAltitude >= altThreshold)) continue;
            safeForFloatingOrigin = false;
            return false;
        }
        if (s.active.landedOrSplashed) return false;
        double num = s.active.radarAltitude - extraAltOffsetForVel;
        if (!(Vector3d::Dot(s.velocityD, -s.upAxis) * static_cast<double>(Time::fixedDeltaTime) > num) &&
            num >= ((loadedVesselsCount_ > 1) ? altThreshold : altThresholdAlone)) {
            return true;
        }
        return false;
    }
}

// Krakensbane.cs:121
void Krakensbane::AddExcess(const Vector3d& vel) {
    excessV = vel;
    lastCorrection = -excessV;
    if (FrameVel.IsZero()) GameEvents::onKrakensbaneEngage.Fire(-excessV);
    FrameVel += excessV;
    size_t index = loadedVesselsCount_;
    while (index-- > 0) {
        if (index >= lastSnapshot_.loaded.size()) continue;
        const FrameVesselState& vessel = lastSnapshot_.loaded[index];
        if (!vessel.packed && !vessel.dead && host.changeWorldVelocity) host.changeWorldVelocity(index, -excessV);
    }
    if (host.physicalObjectsVelocityChange) host.physicalObjectsVelocityChange(-excessV);
}

// Krakensbane.cs:150
void Krakensbane::Zero() {
    lastCorrection = FrameVel;
    GameEvents::onKrakensbaneDisengage.Fire(FrameVel);
    FrameSnapshot s = Snapshot();
    const size_t count = s.loaded.size();
    for (size_t i = 0; i < count; ++i) {
        const FrameVesselState& vessel = s.loaded[i];
        if (!vessel.packed && !vessel.dead && (!vessel.landedOrSplashed || !(!vessel.isActive))) {
            if (host.changeWorldVelocity) host.changeWorldVelocity(i, FrameVel);
        }
    }
    if (host.physicalObjectsVelocityChange) host.physicalObjectsVelocityChange(FrameVel);
    FrameVel.Zero();
}

// Krakensbane.cs:29, 175-208. `fetch` is null without an instance: the C# would throw NullReferenceException; here the
// readers return zero / do nothing (documented deviation: a headless caller without a Krakensbane has no frame velocity).
double Krakensbane::SqrThreshold() { return fetch_ ? fetch_->MaxVSqr : std::numeric_limits<double>::max(); }
Vector3d Krakensbane::GetFrameVelocity() { return fetch_ ? fetch_->FrameVel : Vector3d::zero(); }
Vector3f Krakensbane::GetFrameVelocityV3f() { return GetFrameVelocity().ToVector3f(); }
Vector3d Krakensbane::GetLastCorrection() { return fetch_ ? fetch_->lastCorrection : Vector3d::zero(); }

void Krakensbane::ResetVelocityFrame(bool resetObjects) {
    if (!fetch_) return;
    if (resetObjects) fetch_->Zero();
    else fetch_->FrameVel.Zero();
}

void Krakensbane::AddFrameVelocity(const Vector3d& vel) {
    if (!fetch_) return;
    bool unused;
    if (fetch_->SafeToEngage(fetch_->Snapshot(), unused)) fetch_->AddExcess(vel);
}

}  // namespace ksp
