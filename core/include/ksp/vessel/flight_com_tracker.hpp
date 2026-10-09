// FlightCoMTracker.cs (105 lines): the targetable marker at a vessel's centre of mass. Unity's GameObject / Transform become the
// tracker's own ksp::UnityEngine::Transform (trf: name, position, identity rotation); FixedUpdate is an explicit call; MakeTarget
// goes through the host's SetVesselTarget.
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "ksp/modules/part_module.hpp"
#include "ksp/unity/transform.hpp"
#include "ksp/vessel/vessel_types.hpp"

namespace ksp {

class Vessel;

class FlightCoMTracker final : public ITargetable {
public:
    // FlightGlobals.fetch.SetVesselTarget(this) (FlightGlobals.ready gates it: MakeTarget retries until then)
    static inline std::function<void(ITargetable*)> setVesselTarget;

    // FlightCoMTracker.cs:13
    static std::unique_ptr<FlightCoMTracker> Create(PartModule* host, bool activeTargetable = false);

    void FixedUpdate();   // :22 transform.position = host.vessel.CurrentCoM
    void MakeTarget();    // :27 (the coroutine makeTgtWhenReady: RunPending after FlightGlobals.ready)
    void RunPending();    // :42 makeTgtWhenReady, one frame step
    bool pending() const { return pending_; }
    const std::string& name() const { return trf_.name; }  // GameObject name: host.name + " Vessel CoM Marker"

    UnityEngine::Transform* GetTransform() override { return &trf_; }  // :51
    Vector3f GetObtVelocity() override;                               // :56
    Vector3f GetSrfVelocity() override;                               // :61
    Vector3f GetFwdVector() override { return Vector3f(0.0f, 0.0f, 1.0f); }  // :66 trf.forward (identity rotation)
    Vessel* GetVessel() override;                                     // :71
    std::string GetName() override;                                   // :76
    std::string GetDisplayName() override { return GetName(); }       // :81
    Orbit* GetOrbit() override;                                       // :86
    OrbitDriver* GetOrbitDriver() override;                           // :91
    VesselTargetModes GetTargetingMode() override { return VesselTargetModes::DirectionAndVelocity; }  // :96
    bool GetActiveTargetable() override { return activeTargetable_; }  // :101

private:
    PartModule* host_ = nullptr;
    UnityEngine::Transform trf_;  // trf = transform (:9); never rotated
    bool activeTargetable_ = false;
    bool pending_ = false;
};

}  // namespace ksp
