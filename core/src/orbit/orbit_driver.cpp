// OrbitDriver (OrbitDriver.cs); see include/ksp/orbit/orbit_driver.hpp.
#include "ksp/orbit/orbit_driver.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/exceptions.hpp"
#include "ksp/core/log.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/math/util_math.hpp"
#include "ksp/orbit/time_warp.hpp"

namespace ksp {

namespace {

namespace M = dotnet::Math;

// Vector3d.cs:329 ToString: "[" + x + ", " + y + ", " + z + "]"
std::string VecText(const Vector3d& v) {
    return "[" + dotnet::FormatDoubleG(v.x) + ", " + dotnet::FormatDoubleG(v.y) + ", " + dotnet::FormatDoubleG(v.z) + "]";
}

// double.ToString("0.###") for the log line of OnRailsSOITransition (log text only): F3 without trailing zeros.
std::string Fmt3(double v) {
    std::string s = dotnet::FormatDoubleF(v, 3);
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    if (s == "-0") s = "0";
    return s;
}

std::vector<OrbitDriver*>& RequireOrbits() {
    std::vector<OrbitDriver*>* orbits = Planetarium::Orbits();
    if (orbits == nullptr) throw NullReferenceException("OrbitDriver.Start: Planetarium.Orbits (no Planetarium)");
    return *orbits;
}

}  // namespace

const char* ToString(OrbitDriver::UpdateMode m) {
    switch (m) {
        case OrbitDriver::UpdateMode::TRACK_Phys: return "TRACK_Phys";
        case OrbitDriver::UpdateMode::UPDATE: return "UPDATE";
        case OrbitDriver::UpdateMode::IDLE: return "IDLE";
    }
    return "?";
}

// OrbitDriver.cs:123 OnDestroy: if (Planetarium.Orbits != null) { Planetarium.Orbits.Remove(this); Destroy(Renderer); }
OrbitDriver::~OrbitDriver() {
    if (std::vector<OrbitDriver*>* orbits = Planetarium::Orbits()) {
        auto it = std::find(orbits->begin(), orbits->end(), this);  // List.Remove: first occurrence
        if (it != orbits->end()) orbits->erase(it);
    }
}

// OrbitDriver.cs:98
void OrbitDriver::Start() {
    switch (updateMode) {
        case UpdateMode::UPDATE:
            orbit->Init();
            updateFromParameters();
            break;
        case UpdateMode::TRACK_Phys:
        case UpdateMode::IDLE:
            if (referenceBody() == nullptr) {
                const Vector3f tp = vessel != nullptr ? vessel->TransformPosition() : driverTransformPosition;
                setReferenceBody(FlightGlobals::getMainBody(Vector3d(tp)));
            }
            TrackRigidbody(referenceBody(), 0.0 - fdtLast_);
            break;
    }
    ready_ = true;
    RequireOrbits().push_back(this);  // Planetarium.Orbits.Add(this)
    if (OnReferenceBodyChange) OnReferenceBodyChange(referenceBody());
}

// OrbitDriver.cs:135
void OrbitDriver::FixedUpdate() {
    if (!QueuedUpdate) {
        if (QueueOnce)
            QueueOnce = false;
        else
            UpdateOrbit();
    }
}

// OrbitDriver.cs:150
void OrbitDriver::UpdateOrbit(bool offset) {
    if (!ready_) return;
    lastMode = updateMode;
    switch (updateMode) {
        case UpdateMode::UPDATE:
            updateFromParameters();
            if (vessel != nullptr) CheckDominantBody(referenceBody()->position + pos);
            break;
        case UpdateMode::TRACK_Phys:
        case UpdateMode::IDLE:
            if (vessel != nullptr && vessel->HasRootPartRigidbody()) {
                if (!offset) fdtLast_ = -0.0;
                if (!CheckDominantBody(vessel->CoMD())) TrackRigidbody(referenceBody(), 0.0 - fdtLast_);
            }
            break;
    }
    fdtLast_ = static_cast<double>(TimeWarp::fixedDeltaTime());
    if (isHyperbolic_ && orbit->eccentricity < 1.0) {
        isHyperbolic_ = false;
        if (vessel != nullptr) GameEvents::onVesselOrbitClosed.Fire(vessel->AsVessel());
    }
    if (!isHyperbolic_ && orbit->eccentricity > 1.0) {
        isHyperbolic_ = true;
        if (vessel != nullptr) GameEvents::onVesselOrbitEscaped.Fire(vessel->AsVessel());
    }
    // if (drawOrbit) orbit.DrawOrbit(): Debug.DrawLine gizmos, not ported
}

// OrbitDriver.cs:204
void OrbitDriver::SetOrbitMode(UpdateMode mode) {
    updateMode = mode;
    fdtLast_ = static_cast<double>(TimeWarp::fixedDeltaTime());
}

// OrbitDriver.cs:210
bool OrbitDriver::CheckDominantBody(const Vector3d& refPos) {
    CelestialBody* mainBody = FlightGlobals::getMainBody(refPos);
    if (referenceBody() != mainBody && !FlightGlobals::overrideOrbit) {
        RecalculateOrbit(mainBody);
        return true;
    }
    return false;
}

// OrbitDriver.cs:221
void OrbitDriver::TrackRigidbody(CelestialBody* refBody, double fdtOffset) {
    updateUT_ = Planetarium::GetUniversalTime();
    CelestialBody* rb = referenceBody();
    if (vessel != nullptr) pos = (vessel->CoMD() - rb->position).xzy();
    if (vessel != nullptr && vessel->HasRootPartRigidbody() && !vessel->RootPartRigidbodyIsKinematic()) {
        updateUT_ += fdtOffset;
        vel = vessel->velocityD().xzy() + orbit->GetRotFrameVelAtPos(*rb, pos);
    } else if (updateMode == UpdateMode::IDLE) {
        vel = orbit->GetRotFrameVel(*rb);
    }
    if (refBody != rb) {
        if (vessel != nullptr) pos = (vessel->CoMD() - refBody->position).xzy();
        vel += rb->GetFrameVel() - refBody->GetFrameVel();
    }
    lastTrackUT = updateUT_;
    orbit->UpdateFromStateVectors(pos, vel, refBody, updateUT_);
    pos.Swizzle();
    vel.Swizzle();
}

// OrbitDriver.cs:251
void OrbitDriver::updateFromParameters() { updateFromParameters(true); }

// OrbitDriver.cs:256
void OrbitDriver::updateFromParameters(bool setPosition) {
    updateUT_ = Planetarium::GetUniversalTime();
    orbit->UpdateFromUT(updateUT_);
    pos = orbit->pos;
    vel = orbit->vel;
    pos.Swizzle();
    vel.Swizzle();
    if (std::isnan(pos.x)) {
        log::Info("ObT : " + dotnet::FormatDoubleG(orbit->ObT) + "\nM : " + dotnet::FormatDoubleG(orbit->meanAnomaly) +
                  "\nE : " + dotnet::FormatDoubleG(orbit->eccentricAnomaly) + "\nV : " + dotnet::FormatDoubleG(orbit->trueAnomaly) +
                  "\nRadius: " + dotnet::FormatDoubleG(orbit->radius) + "\nvel: " + VecText(vel) + "\nAN: " + VecText(orbit->an) +
                  "\nperiod: " + dotnet::FormatDoubleG(orbit->period) + "\n");
        if (vessel != nullptr) {
            log::Warning("[OrbitDriver Warning!]: " + vessel->GetDisplayName() + " had a NaN Orbit and was removed.");
            vessel->UnloadAndDestroy();
            return;
        }
    }
    if (!setPosition) return;
    CelestialBody* rb = referenceBody();
    if (!reverse) {
        if (vessel != nullptr) {
            const Vector3d vector3d = vessel->RotatedLocalCoM();
            vessel->SetPosition(rb->position + pos - vector3d);
        } else if (celestialBody != nullptr) {
            celestialBody->position = rb->position + pos;
        } else {
            driverTransformPosition = (rb->position + pos).ToVector3f();
        }
    } else {
        const Vector3d own = celestialBody != nullptr ? celestialBody->position
                                                      : Vector3d(vessel != nullptr ? vessel->TransformPosition() : driverTransformPosition);
        rb->position = own - pos;
    }
}

// OrbitDriver.cs:301
void OrbitDriver::RecalculateOrbit(CelestialBody* newReferenceBody) {
    if (frameShift) return;
    frameShift = true;
    CelestialBody* celestialBody0 = referenceBody();
    if (updateMode == UpdateMode::UPDATE && Time::timeScale > 0.0f) {
        OnRailsSOITransition(*orbit, newReferenceBody);
    } else {
        log::Info("recalculating orbit for " + name + ": " + referenceBody()->bodyName + " ( Update mode " +
                  ToString(updateMode) + " )\nrPos: " + VecText(pos) + "   rVel: " + VecText(vel) + " |" +
                  dotnet::FormatDoubleG(vel.magnitude()) + "|");
        TrackRigidbody(newReferenceBody, 0.0 - fdtLast_);
        // The second Debug.Log (OrbitDriver.cs:318) prints drift deltas through vessel.CoMD, rb_velocityD and
        // Krakensbane.GetFrameVelocity (WP-20); only the header is logged here.
        if (newReferenceBody != nullptr && orbit->referenceBody != nullptr && vessel != nullptr && celestialBody != nullptr)
            log::Info("recalculated orbit for " + name + ": " + newReferenceBody->bodyName + " ( UT: " +
                      dotnet::FormatDoubleG(Planetarium::GetUniversalTime()) + " )");
    }
    if (OnReferenceBodyChange) OnReferenceBodyChange(newReferenceBody);
    if (vessel != nullptr) {
        GameEvents::onVesselSOIChanged.Fire(
            GameEvents::HostedFromToAction<Vessel*, CelestialBody*>(vessel->AsVessel(), celestialBody0, newReferenceBody));
        vessel->ForceVesselLogFlyby(newReferenceBody);
    }
    // Invoke("unlockFrameSwitch", 1f)
    unlockPending_ = true;
    unlockAt_ = Time::time + 1.0f;
}

// OrbitDriver.cs:335
void OrbitDriver::OnRailsSOITransition(Orbit& ownOrbit, CelestialBody* to) {
    CelestialBody* celestialBody0 = referenceBody();
    const double universalTime = Planetarium::GetUniversalTime();
    const double num = universalTime - static_cast<double>(TimeWarp::fixedDeltaTime());
    const double num2 = universalTime - num;
    const double epsilon = num2 * 1E-07;
    double v = universalTime;
    double SOIsqr = 0.0;
    int num3 = 0;
    if (orbit->referenceBody->HasChild(to)) {
        SOIsqr = to->sphereOfInfluence * to->sphereOfInfluence;
        num3 = UtilMath::BSPSolver(
            v, num2,
            [&](double t) { return M::Abs((ownOrbit.getPositionAtUT(t) - to->getPositionAtUT(t)).sqrMagnitude() - SOIsqr); },
            num, universalTime, epsilon, 64);
    } else if (to->HasChild(orbit->referenceBody)) {
        SOIsqr = orbit->referenceBody->sphereOfInfluence * orbit->referenceBody->sphereOfInfluence;
        num3 = UtilMath::BSPSolver(
            v, num2, [&](double t) { return M::Abs(ownOrbit.getRelativePositionAtUT(t).sqrMagnitude() - SOIsqr); }, num,
            universalTime, epsilon, 64);
    }
    ownOrbit.UpdateFromOrbitAtUT(ownOrbit, v, to);
    log::Info("[OrbitDriver]: On-Rails SOI Transition from " + celestialBody0->bodyName + " to " + to->bodyName +
              ".\n Transition UT Range: " + Fmt3(num) + " - " + Fmt3(universalTime) + ".\n Transition UT: " + Fmt3(v) +
              ". Iterations: " + std::to_string(num3) + ".");
}

void OrbitDriver::RunDelayedCalls() {
    if (unlockPending_ && Time::time >= unlockAt_) {
        unlockPending_ = false;
        unlockFrameSwitch();
    }
}

// OrbitDriver.cs:359
void OrbitDriver::unlockFrameSwitch() { frameShift = false; }

}  // namespace ksp
