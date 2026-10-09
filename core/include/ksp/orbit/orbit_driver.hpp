// OrbitDriver (OrbitDriver.cs, 363 lines). WP-10 ("time warp, rails propagation, orbit driver") owns the class; WP-08
// ports what patched conics and on-rails propagation need, which is the whole MonoBehaviour minus rendering:
//   Start            orbit.Init + updateFromParameters (UPDATE) or TrackRigidbody (TRACK_Phys / IDLE), then
//                    Planetarium.Orbits.Add(this) (the list PatchedConics.CalculatePatch iterates)
//   UpdateOrbit      UPDATE: updateFromParameters, then for a vessel CheckDominantBody(referenceBody.position + pos);
//                    TRACK_Phys / IDLE: CheckDominantBody(vessel.CoMD) / TrackRigidbody (needs the vessel hooks)
//   RecalculateOrbit SOI change: OnRailsSOITransition (UtilMath.BSPSolver on the last frame's interval) in UPDATE mode,
//                    TrackRigidbody otherwise; OnReferenceBodyChange, GameEvents.onVesselSOIChanged, frameShift lock
//                    released by Invoke("unlockFrameSwitch", 1f)
// Not ported: OrbitRendererBase Renderer and orbit.DrawOrbit() (rendering). Targetable() is there (ITargetable is
// ksp/orbit/targetable.hpp); CelestialBody is not an ITargetable in kspcore, the host supplies `celestialBodyTargetable`.
//
// C# reference semantics: `orbit` is a shared_ptr (KSP code replaces `orbitDriver.orbit` objects, and the patched-conic
// solver keeps the same object as its patches[0]). The GameObject components KSP reaches through GetComponent are
// pointers: `vessel` (IOrbitDriverVessel, implemented by the Vessel package), `celestialBody`; a driver with neither moves
// `driverTransformPosition` (Unity Transform.position, float).
//
// Unity's Invoke: RecalculateOrbit schedules unlockFrameSwitch at Time.time + 1; Unity runs delayed calls once per frame,
// the host does that by calling RunDelayedCalls() (kspcore has no frame loop).
#pragma once

#include <memory>
#include <string>

#include "ksp/core/delegate.hpp"
#include "ksp/math/vector3d.hpp"
#include "ksp/orbit/orbit.hpp"
#include "ksp/orbit/targetable.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {

class CelestialBody;
class Vessel;

// What OrbitDriver reads from and does to its Vessel (the members OrbitDriver.cs touches). The Vessel package
// (WP-14/WP-19) implements it; KSP's `vessel != null` is `vessel != nullptr`.
class IOrbitDriverVessel {
public:
    virtual ~IOrbitDriverVessel() = default;
    // The ksp::Vessel for GameEvents payloads (onVesselSOIChanged, onVesselOrbitClosed/Escaped); may be null for hosts
    // that are not a ksp::Vessel (test probes).
    virtual Vessel* AsVessel() = 0;
    virtual std::string GetDisplayName() const = 0;
    // updateFromParameters (OrbitDriver.cs:281): vessel.SetPosition(referenceBody.position + pos - offset) with
    // offset = (QuaternionD)driverTransform.rotation * (Vector3d)vessel.localCoM
    virtual Vector3d RotatedLocalCoM() const = 0;
    virtual void SetPosition(const Vector3d& position) = 0;
    // updateFromParameters, NaN orbit: vessel.Unload(); Object.Destroy(vessel.gameObject)
    virtual void UnloadAndDestroy() = 0;
    // driverTransform.position (the vessel's transform; Unity float)
    virtual Vector3f TransformPosition() const = 0;
    // TRACK_Phys / IDLE: vessel.rootPart != null && vessel.rootPart.rb != null; rb.isKinematic; vessel.CoMD;
    // vessel.velocityD
    virtual bool HasRootPartRigidbody() const = 0;
    virtual bool RootPartRigidbodyIsKinematic() const = 0;
    virtual Vector3d CoMD() const = 0;
    virtual Vector3d velocityD() const = 0;
    // RecalculateOrbit: ModuleTripLogger.ForceVesselLog(vessel, FlightLog.EntryType.Flyby, newReferenceBody)
    virtual void ForceVesselLogFlyby(CelestialBody* body) = 0;
    // The vessel as an ITargetable (OrbitDriver.Targetable, OrbitDriver.cs:75); null for hosts that are not targetable.
    virtual ITargetable* AsTargetable() { return nullptr; }
};

class OrbitDriver {
public:
    enum class UpdateMode : int { TRACK_Phys = 0, UPDATE = 1, IDLE = 2 };  // OrbitDriver.cs:6
    using CelestialBodyDelegate = Delegate<void(CelestialBody*)>;            // OrbitDriver.cs:13

    // ---- fields (OrbitDriver.cs:15-87, same names) ----------------------------------------------------------------
    Vector3d pos;
    Vector3d vel;
    std::shared_ptr<Orbit> orbit = std::make_shared<Orbit>();
    bool drawOrbit = false;
    bool reverse = false;
    bool frameShift = false;
    bool QueuedUpdate = false;
    bool QueueOnce = false;
    UpdateMode updateMode = UpdateMode::TRACK_Phys;
    IOrbitDriverVessel* vessel = nullptr;  // GetComponent<Vessel>() (Awake)
    CelestialBody* celestialBody = nullptr;  // GetComponent<CelestialBody>() (Awake)
    Vector3f driverTransformPosition;        // driverTransform.position when there is neither vessel nor body
    // The CelestialBody component as an ITargetable (CelestialBody : ITargetable needs bodyTransform, an engine object;
    // the host supplies it for Targetable()).
    ITargetable* celestialBodyTargetable = nullptr;
    std::string name;                        // base.name (log text only)
    CelestialBodyDelegate OnReferenceBodyChange;
    double lastTrackUT = 0.0;
    UpdateMode lastMode = UpdateMode::TRACK_Phys;

    OrbitDriver() = default;
    OrbitDriver(const OrbitDriver&) = delete;
    OrbitDriver& operator=(const OrbitDriver&) = delete;
    ~OrbitDriver();  // OnDestroy (OrbitDriver.cs:123): Planetarium.Orbits.Remove(this)

    bool Ready() const { return ready_; }                // OrbitDriver.cs:89
    // OrbitDriver.cs:75 Targetable: the vessel, else the celestial body, else null
    ITargetable* Targetable() const {
        if (vessel != nullptr) return vessel->AsTargetable();
        if (celestialBody != nullptr) return celestialBodyTargetable;
        return nullptr;
    }
    double UpdateUT() const { return updateUT_; }        // private updateUT (the C8 capture reads it by reflection)
    CelestialBody* referenceBody() const { return orbit->referenceBody; }  // OrbitDriver.cs:91 (=> orbit.referenceBody)
    void setReferenceBody(CelestialBody* body) { orbit->referenceBody = body; }

    void Start();                                    // OrbitDriver.cs:98
    void FixedUpdate();                              // OrbitDriver.cs:135
    void UpdateOrbit(bool offset = true);            // OrbitDriver.cs:150
    void SetOrbitMode(UpdateMode mode);              // OrbitDriver.cs:204
    bool CheckDominantBody(const Vector3d& refPos);  // OrbitDriver.cs:210
    void TrackRigidbody(CelestialBody* refBody, double fdtOffset);  // OrbitDriver.cs:221
    void updateFromParameters();                     // OrbitDriver.cs:251
    void updateFromParameters(bool setPosition);     // OrbitDriver.cs:256
    void RecalculateOrbit(CelestialBody* newReferenceBody);         // OrbitDriver.cs:301
    void OnRailsSOITransition(Orbit& ownOrbit, CelestialBody* to);  // OrbitDriver.cs:335

    // Unity's delayed-call pass for Invoke("unlockFrameSwitch", 1f): runs it once Time.time has reached the target.
    void RunDelayedCalls();
    bool FrameSwitchUnlockPending() const { return unlockPending_; }

private:
    void unlockFrameSwitch();  // OrbitDriver.cs:359

    bool isHyperbolic_ = false;
    bool ready_ = false;
    double fdtLast_ = 0.02;
    double updateUT_ = 0.0;
    bool unlockPending_ = false;
    float unlockAt_ = 0.0f;
};

const char* ToString(OrbitDriver::UpdateMode m);  // Enum.ToString()

}  // namespace ksp
