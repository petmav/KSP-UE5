// Jolt spike, lever 2: the per-part work of a physics tick, run natively on kspcore's exact ports.
//
//   transforms  the kspcore Part transforms are written in a VESSEL-LOCAL float frame (position = part - vessel CoM, rotation =
//               physics-frame rotation): the float maths of the ports sees small coordinates, the authoritative state stays double
//   mass        Vessel::UpdateMassStats (WP-14 port of FlightIntegrator.UpdateMassStats)
//   atmosphere  FlightIntegratorAtmosphere (WP-09 port: pressure, density, Mach, q, pseudo-Reynolds)
//   aero        FlightIntegratorAero::DragCubeSetupAndPartAeroStats + UpdateAerodynamics per part (WP-21 port, Mono float rule)
//   engines     thrust of every engine at full throttle along its part's axis (stand-in for WP-23 ModuleEngines.ThrustUpdate)
// Two aero kernels:
//   Exact   the WP-21 port as written: per part, per face, every FloatCurve evaluated where the C# evaluates it
//   SoA     the same float statements over struct-of-arrays part data, with the curves that depend only on the vessel's Mach
//           (DRAG_TAIL/SURFACE/TIP/MULTIPLIER, DRAG_CD_POWER, BodyLift liftMach) evaluated once per tick and the per-face Cd
//           curve of the (constant) weighted drag evaluated once per part. Same inputs, same operations: bit-identical results
//           (checked against Exact on every part and tick with --check-soa).
#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Math/DVec3.h>

#include <array>
#include <cstdint>
#include <vector>

#include "craft_model.hpp"
#include "ksp/aero/flight_integrator_aero.hpp"
#include "ksp/celestial/flight_atmosphere.hpp"
#include "vessel_physics.hpp"

namespace spike {

class KspContext;

struct PipelineTimes {
    double transformsMs = 0, massMs = 0, atmosphereMs = 0, aeroSetupMs = 0, aeroMs = 0, enginesMs = 0;
    double Total() const { return transformsMs + massMs + atmosphereMs + aeroSetupMs + aeroMs + enginesMs; }
};

class PartPipeline {
public:
    PartPipeline(KspContext& ctx, CraftModel& craft);
    enum class AeroKernel { Exact, SoA, Both };

    // One tick of per-part work; forces go to `model`. bodyPosOf(world) = frameOrigin + physics-frame position.
    void Tick(VesselPhysics& model, const JPH::DVec3& frameOrigin, bool aero, AeroKernel kernel, float throttle, PipelineTimes& t);
    // Count of parts whose SoA aero outputs differed in any bit from the exact kernel (AeroKernel::Both), cumulative
    long long soaMismatches = 0;
    long long soaCompared = 0;
    double altitude = 0.0;  // vessel CoM altitude (m) at the last tick
    double mach = 0.0, dynamicPressure = 0.0;

private:
    struct SoaPart {
        int part;
        // constant per flight (weights and occlusion do not change without a structural event)
        std::array<float, 6> areaOccluded, weightedDrag, weightedDepth;
        std::array<float, 6> cdCurve;  // DragCurveCd(weightedDrag) for faces with weightedDrag < 1
        std::array<float, 6> invDrag;  // the C# num5 after the 1/x rewrite
    };
    void SetTransforms(VesselPhysics& model);
    ksp::AeroForces AeroExactPart(VesselPhysics& model, size_t i);
    void AeroExact(VesselPhysics& model, std::vector<ksp::AeroForces>* out);
    void AeroSoA(VesselPhysics& model, std::vector<ksp::AeroForces>* out);
    void ApplyAero(VesselPhysics& model, int part, const ksp::AeroForces& f);
    void BuildSoa();

    KspContext& ctx_;
    CraftModel& craft_;
    ksp::FlightIntegratorAtmosphere atm_;
    ksp::FlightIntegratorAero fi_;
    std::vector<ksp::Part*> parts_;
    std::vector<SoaPart> soa_;
    std::vector<uint8_t> soaEligible_;
    JPH::RVec3 refPoint_;  // vessel CoM this tick (physics frame): origin of the vessel-local float frame
    JPH::DVec3 frameOrigin_;
    std::vector<ksp::AeroForces> exactOut_, soaOut_;
};

}  // namespace spike
