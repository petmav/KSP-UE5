// FlightCoMTracker.cs (see include/ksp/vessel/flight_com_tracker.hpp)
#include "ksp/vessel/flight_com_tracker.hpp"

#include "ksp/localization/localizer.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/vessel/vessel.hpp"

namespace ksp {

namespace {
Vessel* VesselOf(PartModule* host) { return host->part != nullptr ? static_cast<Part*>(host->part)->vessel : nullptr; }
}  // namespace

// FlightCoMTracker.cs:13
std::unique_ptr<FlightCoMTracker> FlightCoMTracker::Create(PartModule* host, bool activeTargetable) {
    auto t = std::make_unique<FlightCoMTracker>();
    t->host_ = host;
    t->name_ = (host->part != nullptr ? host->part->gameObjectName : std::string()) + " Vessel CoM Marker";
    t->activeTargetable_ = activeTargetable;
    return t;
}

// FlightCoMTracker.cs:22
void FlightCoMTracker::FixedUpdate() { position_ = VesselOf(host_)->CurrentCoM(); }

// FlightCoMTracker.cs:27
void FlightCoMTracker::MakeTarget() {
    if (!Vessel::env.LoadedSceneIsFlight) return;
    if (FlightRegistry::ready) {
        if (setVesselTarget) setVesselTarget(this);
    } else {
        pending_ = true;
    }
}

// FlightCoMTracker.cs:42
void FlightCoMTracker::RunPending() {
    if (pending_ && FlightRegistry::ready) {
        pending_ = false;
        if (setVesselTarget) setVesselTarget(this);
    }
}

Vector3f FlightCoMTracker::GetObtVelocity() { return VesselOf(host_)->obt_velocity.ToVector3f(); }
Vector3f FlightCoMTracker::GetSrfVelocity() { return VesselOf(host_)->srf_velocity.ToVector3f(); }
Vessel* FlightCoMTracker::GetVessel() { return VesselOf(host_); }
std::string FlightCoMTracker::GetName() { return VesselOf(host_)->GetDisplayName() + " " + KSP::Localization::Localizer::Format("#autoLOC_6002484"); }
Orbit* FlightCoMTracker::GetOrbit() { return &VesselOf(host_)->orbit(); }
OrbitDriver* FlightCoMTracker::GetOrbitDriver() { return VesselOf(host_)->GetOrbitDriver(); }

}  // namespace ksp
