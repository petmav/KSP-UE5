// FlightCoMTracker.cs (105 lines): the targetable marker at a vessel's centre of mass. Unity's GameObject / Transform become the
// tracker's own position; FixedUpdate is an explicit call; MakeTarget goes through the host's SetVesselTarget.
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "ksp/modules/part_module.hpp"
#include "ksp/vessel/vessel_types.hpp"

namespace ksp {

class Vessel;

class FlightCoMTracker final : public ITargetable {
public:
    // FlightGlobals.fetch.SetVesselTarget(this) (FlightGlobals.ready gates it: MakeTarget retries until then)
    static inline std::function<void(ITargetable*)> setVesselTarget;

    // FlightCoMTracker.cs:14
    static std::unique_ptr<FlightCoMTracker> Create(PartModule* host, bool activeTargetable = false);

    void FixedUpdate();   // :23 transform.position = host.vessel.CurrentCoM
    void MakeTarget();    // :28 (the coroutine makeTgtWhenReady: RunPending after FlightGlobals.ready)
    void RunPending();    // :41 makeTgtWhenReady, one frame step
    bool pending() const { return pending_; }
    const std::string& name() const { return name_; }  // GameObject name: host.name + " Vessel CoM Marker"

    std::string GetTransformName() override { return name_; }        // :47 GetTransform().name
    Vector3f GetTransformPosition() override { return position_; }
    Vector3f GetObtVelocity() override;                               // :52
    Vector3f GetSrfVelocity() override;                               // :57
    Vector3f GetFwdVector() override { return Vector3f(0.0f, 0.0f, 1.0f); }  // :62 trf.forward (identity rotation)
    Vessel* GetVessel() override;                                     // :67
    std::string GetName() override;                                   // :72
    std::string GetDisplayName() override { return GetName(); }       // :77
    Orbit* GetOrbit() override;                                       // :82
    OrbitDriver* GetOrbitDriver() override;                           // :87
    VesselTargetModes GetTargetingMode() override { return VesselTargetModes::DirectionAndVelocity; }  // :92
    bool GetActiveTargetable() override { return activeTargetable_; }  // :97

private:
    PartModule* host_ = nullptr;
    std::string name_;
    Vector3f position_;
    bool activeTargetable_ = false;
    bool pending_ = false;
};

}  // namespace ksp
