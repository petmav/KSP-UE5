// WP-20d: FlightIntegrator (FlightIntegrator.cs, a VesselModule) over the rigid bodies of the reduced model.
//
// One FlightIntegrator per loaded vessel runs FlightIntegrator.FixedUpdate (FlightIntegrator.cs:544-670) at the measured slot of the
// flight tick (seq 278, after the part-module FixedUpdates, before OrbitPhysicsManager / Krakensbane / FloatingOrigin and the physics
// step; ksp/vessel/flight_scene.hpp, PHYSICS-ENGINE-REVIEW.md 1.2), in the C#'s statement order:
//
//   Setup (:235) on the first tick       resource masses, the zeroed-root drag-cube reload, radiativeArea, UpdateMassStats
//   caches (:573-593)                    FlightIntegratorAero::CacheGlobals + FlightIntegratorThermal::CacheGlobals
//   UpdateMassStats (:452)               the thermal half (WP-22), Vessel::UpdateMassStats (WP-14: physicsMass), rb.mass / centre of mass
//                                        of the bodies through VesselPhysics::UpdateMassProperties (WP-20b; not while packed)
//   SmoothVelocity (:495)                the 10-sample velocity average with spike rejection, spd / nVel
//   atmosphere (:617-641)                altitude at CoMD, CalculatePressure, the vessel mirrors, the first-frame density quirk and
//                                        ThermoPrecalculate, CalculateConstantsAtmosphere / Vacuum (WP-09), densityThermalLerp,
//                                        background radiation temperatures
//   DragCubeSetupAndPartAeroStats (:642) WP-21, per-part static pressure at partTransform.position
//   thermal (:643-648)                   CheckThermalGraph, UpdateOcclusion(all) when flagged, UpdateThermodynamics (WP-22 port as is)
//   packed (:649-660)                    clear every part's force buffers and stop
//   IntegratePhysicalObjects (:661, :747)  gravity and drag of the physical objects, active vessel only
//   drift compensation (:665-668)        vessel.OffsetVelocity(precalc.preIntegrationVelocityOffset)
//   Integrate(partRef) (:669, :715)      depth first from the root over the attached children: integrationAccel (Acceleration) once
//                                        for the vessel's bodies, part.force / part.torque / part.forces (last first) of every part with
//                                        a Rigidbody (the WP-20b binding's public force calls, in the order IntegratePartBuffers uses),
//                                        the buffers cleared, then UpdateAerodynamics(part) (:2260) and its drag / lift / angular drag
//                                        applied (ApplyAeroDrag :2243, ApplyAeroLift :2249) before the children
// and FlightIntegrator.Update (:672: ThermoPrecalculate, the occlusion round-robin) once per frame.
//
// Aero kernels (PHYSICS-SPIKE.md lever 2b): Exact = the WP-21 port called inside the recursion as the C# does; SoA = ksp/physics/aero_soa.hpp
// evaluated for the whole Integrate order before the recursion and applied at each part's place in it (UpdateAerodynamics of a part
// reads nothing another part's UpdateAerodynamics or the force calls of the recursion write: the batch is the same computation);
// Both = the exact port and the SoA kernel on the same inputs, every output of every part compared bit for bit (`aeroStats`), the
// exact results applied.
//
// Fields: the C# names. The members FlightIntegrator shares with the WP-22 thermal half (fDeltaTime, firstFrame, needOcclusion,
// partCount, recreateThermalGraph, nVel, sunVector, the thermal graph ...) and the atmosphere state of WP-09 (`atm`: altitude, spd,
// density, mach, staticPressurekPa / Atm, dynamicPressurekPa, atmosphericTemperature, externalTemperature, convectiveCoefficient,
// pseudoReDragMult, densityThermalLerp ...) are the base class's: FlightIntegrator IS the C# class, split over three ports.
//
// Seams (the host's): `sunBodyFlux` (ScaledSpace positions and the sun raycast of CalculateSunBodyFlux; unset: DefaultSunBodyFlux, an
// analytic ray / sphere test against the bodies), `localToWorld` (part.partTransform.localToWorldMatrix; default: TRS of the part
// transform), `compoundTarget` (CompoundPart.target; unset: no compound links), HighLogic's ReentryHeatScale (`atm.reentryHeatScale`).
// Ground height / landed state / buoyancy (PartBuoyancy's submergedPortion) are WP-20e's.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ksp/aero/flight_integrator_aero.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/physics/aero_soa.hpp"
#include "ksp/physics/vessel_physics.hpp"
#include "ksp/thermal/flight_integrator_thermal.hpp"

namespace ksp {
class Part;
class Vessel;
class physicalObject;
}  // namespace ksp

namespace ksp::physics {

enum class AeroKernel : uint8_t { Exact, SoA, Both };

// Part.Start in the flight scene (Part.cs:1663) for every part of a vessel entering it, the members the tick reads: the data block
// (Part::StartData with the flight context: physical significance, the external temperature of a part still at -1, resource and
// thermal masses, `started`), hasLiftModule (:1857-1866: an ILiftProvider with DisableBodyLift), CheckBodyLiftAttachment (:1867) and
// the Rigidbodies (WP-20b StartPartRigidbodies, :1868-1885). FlightIntegrator.Setup waits for the root part's `started` (:255).
void StartPartsInFlight(Vessel& vessel);

// WP-20g: Part.Start in the flight scene WITH the module lifecycle, for every part of a vessel entering the scene (what Unity runs between
// instantiating a craft or a loaded vessel and its first FixedUpdate): Part.ModulesOnStart (Part.cs:1727 -> :5615, the start state of
// GetModuleStartState :5569: PRELAUNCH = PreLaunch | Landed ...), the flight block above (StartPartsInFlight), Part.ModulesOnStartFinished
// (:1945 -> :5674), then the first frame's ModuleJettison.LateUpdate (ModuleJettison.cs:474: SetupJettisonState once) of every part. The
// module start-up sets the drag-cube weights KSP flies with (ModulePartVariants.RefreshVariant, ModuleJettison's SetFairingDragCube: STATUS
// WP-20f) and registers ModuleSAS's service level with the part's PartValues. A part already `started` is not started again (Unity starts a
// MonoBehaviour once). The physics bubble runs it on onVesselLoaded; a host that instantiates a vessel itself calls it before the first
// flight tick.
void StartVesselInFlight(Vessel& vessel);

// The SoA-vs-exact comparison of AeroKernel::Both, and the kernel's coverage.
struct AeroKernelStats {
    uint64_t ticks = 0;          // Integrate passes
    uint64_t partTicks = 0;      // UpdateAerodynamics evaluations (parts of the Integrate order, every tick)
    uint64_t compared = 0;       // part-ticks compared (Both)
    uint64_t mismatches = 0;     // part-ticks with any output differing in any bit
    uint64_t covered = 0;        // part-ticks evaluated by the SoA kernel proper (the rest fell back to the exact port)
    std::string firstMismatch;   // "<part> tick <n>: <member>"
    void Add(const AeroKernelStats& o);
};

class FlightIntegrator : public FlightIntegratorThermal {
public:
    explicit FlightIntegrator(Vessel& vessel);  // OnStart's HookVesselEvents (FlightIntegrator.cs:374, :400)
    ~FlightIntegrator() override;                // OnDestroy's UnhookVesselEvents (:395, :410)
    FlightIntegrator(const FlightIntegrator&) = delete;
    FlightIntegrator& operator=(const FlightIntegrator&) = delete;

    static inline FlightIntegrator* ActiveVesselFI = nullptr;  // FlightIntegrator.cs:7

    // ---- the C# fields that are not the thermal / atmosphere halves' (FlightIntegrator.cs:9-223) -------------------------------
    bool isRunning = false;
    bool isKerbal = false;
    Part* partRef = nullptr;  // GetComponent<Part>(): the vessel's root part (the VesselModule sits on its GameObject)
    Vector3f CoM{}, Vel{}, VelUnsmoothed{};
    Vector3d CoMd, Veld, rbVeld;
    static constexpr int VelSmoothLen = 10;  // :41
    std::array<Vector3f, VelSmoothLen> VelSmoother{};
    int VelIndex = 0;
    double VelSmoothLenRecip = 0.0;
    bool lastVelProvisional = false;
    bool VelSpiking = false;
    Vector3f lastVel{};
    int lastVelIndex = 0;
    double maxVelDeltaSqr = 1000000.0;
    Vector3f GraviticAcceleration{};
    bool setupRun = false;
    double fDeltaTimeRecip = 0.0;
    FlightIntegratorAero aero;  // :125-141 the aero caches, and mach / density / pseudoReDragMult as UpdateAerodynamics reads them

    // ---- options and seams ------------------------------------------------------------------------------------------------
    AeroKernel kernel = AeroKernel::Exact;
    std::function<SunBodyFluxInput(const FlightIntegrator&)> sunBodyFlux;
    SunBodyFluxInput DefaultSunBodyFlux() const;

    // ---- the C# methods ---------------------------------------------------------------------------------------------------
    void Setup();                          // :235
    void OnLoadVessel();                   // :381
    void OnUnloadVessel() { setupRun = false; }  // :390
    // :544. `binding`: the vessel's rigid bodies (null while the vessel is packed or has none)
    void FixedUpdate(VesselPhysics* binding);
    void Update();                         // :672
    void ThermoPrecalculate();             // :695 (CalculateSunBodyFlux with the sun input of the seam)
    using FlightIntegratorThermal::ThermoPrecalculate;
    void UpdateMassStats();                // :452
    void SmoothVelocity();                 // :495
    float GetPhysicslessChildMass(const Part& part) const;  // :700
    void IntegratePhysicalObjects(const std::vector<physicalObject*>& pObjs, double atmDensity);  // :747
    // its two expressions: :752 the acceleration of every object (gravity, Coriolis and centrifugal at CoMd / Veld, float), and :767-780
    // the drag of one object with float velocity `rbVelocity` (false: none applied: drag off, no atmosphere, NaN / infinity)
    Vector3f PhysicalObjectGravity() const;
    bool PhysicalObjectDrag(const Vector3f& rbVelocity, const Vector3d& frameVelocity, float origDrag, double atmDensity,
                            Vector3f& drag) const;

    // :669 Integrate(partRef) (:715 recursion) over `binding` (FixedUpdate calls it; public for the gates)
    void Integrate(VesselPhysics& binding);

    // :420-450: a part or vessel event of this vessel rebuilds the thermal graph (recreateThermalGraph)
    void OnPartEvent(Part* part);
    void OnVesselEvent(Vessel* v);
    void OnPartEventTargetAction(events::GameEvents::HostTargetAction<Part*, Part*> data);
    void OnPartEventFromToAction(events::GameEvents::FromToAction<Part*, Part*> data);

    // ---- results ----------------------------------------------------------------------------------------------------------
    AeroKernelStats aeroStats;
    const std::vector<Part*>& IntegrateOrder() const { return order_; }  // the last Integrate's parts, recursion order
    const AeroSoaKernel& Soa() const { return soa_; }
    VesselPhysics* Binding() const { return vp_; }
    uint64_t FixedUpdates() const { return fixedUpdates_; }  // FixedUpdates that reached Integrate

private:
    void ClearPartBuffers();
    AeroRigidbody RigidbodyOf(Part& part) const;
    void AeroBatch();  // SoA / Both before the recursion
    void SetupReloadRootCubes(Part& part);

    VesselPhysics* vp_ = nullptr;
    AeroSoaKernel soa_;
    std::vector<Part*> order_;
    std::vector<AeroRigidbody> rbs_;
    std::vector<AeroForces> aeroOut_, exactOut_;
    std::vector<AeroPartOutputs> exactSnap_;
    std::vector<DragCubeList::CubeData> prevCube_;
    uint64_t fixedUpdates_ = 0;
};

// One FlightIntegrator per loaded vessel (the VesselModule's life: made on the first tick a vessel is loaded, Setup again after an
// unload, dropped with the vessel).
class FlightIntegratorHost {
public:
    AeroKernel kernel = AeroKernel::Exact;
    std::function<SunBodyFluxInput(const FlightIntegrator&)> sunBodyFlux;
    std::function<void(FlightIntegrator&)> configure;  // run once on every new FlightIntegrator (hooks of a host / test)

    FlightIntegrator& For(Vessel& vessel);
    FlightIntegrator* Find(const Vessel& vessel) const;
    void Remove(const Vessel& vessel);
    void Clear();
    size_t Count() const { return fis_.size(); }
    // seq 278: FlightIntegrator.FixedUpdate of every loaded vessel (FlightGlobals.Vessels order) with its binding from `host`
    void FixedUpdate(VesselPhysicsHost& host);
    // FlightIntegrator.Update of every loaded vessel (the frame)
    void Update();
    void OnVesselUnloaded(const Vessel& vessel);  // OnUnloadVessel
    AeroKernelStats TotalAeroStats() const;

private:
    std::vector<std::pair<const Vessel*, std::unique_ptr<FlightIntegrator>>> fis_;  // creation order
};

}  // namespace ksp::physics
