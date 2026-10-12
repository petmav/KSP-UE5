// WP-20d: the struct-of-arrays aerodynamics kernel of the FlightIntegrator tick (PHYSICS-SPIKE.md lever 2b, risk R11).
//
// What it computes: for every part of the Integrate order, exactly what the WP-21 exact port FlightIntegratorAero::UpdateAerodynamics
// (FlightIntegrator.cs:2260-2389, with DragCubeList.SetDrag / AddSurfaceDragDirection, DragCubeList.cs:352-419) computes, writing the
// same part fields, the same DragCubeList CubeData and the same force record, bit for bit (aero::MonoFloat, the game's float rule).
//
// What differs is only how often the inputs that do not depend on the part are evaluated and where the per-part constants live:
//   * the six Mach-only curves of the face loop (DRAG_TAIL / SURFACE / TIP / MULTIPLIER through PhysicsGlobals.DragCurveValue,
//     DRAG_CD_POWER and DRAG_MULTIPLIER for the exposed area, PhysicsGlobals.cs:2692) and the BodyLift liftMachCurve are evaluated
//     ONCE per tick at (float)mach instead of up to 36 + 1 times per part: the same FloatCurve, the same float argument, the same
//     result;
//   * the per-face arrays of SetDragWeights / SetPartOcclusion (areaOccluded, weightedDrag, weightedDepth) are copied into per-part
//     rows of contiguous floats, with the per-face DRAG_CD value of the weighted drag (DragCubeList.cs:395, a function of the
//     weighted drag only) and the 1 / weightedDrag of the exposed-area term (:401) precomputed; a row is rebuilt whenever any of its
//     three arrays changed bit-wise (SetDragWeights / SetPartOcclusion ran: cube weights, occlusion, staging), so it never goes stale;
//   * every other statement is the exact port's, in its order, on the same operands (the scalar half is shared with the capture
//     gate through `PartScalars`).
// A part the kernel does not cover takes the exact port (it is listed in `fallback`): no drag cubes, a drag model other than the cube
// model (DEFAULT, or CUBE without DragCubesUseSpherical), a rotated drag vector (DragCubeList.SetDragVectorRotation: robotic parts),
// or curves other than the shared PhysicsGlobals ones.
//
// Gate (tests/test_flight_integrator.cpp): AeroKernel::Both runs both kernels on the same inputs every tick and compares every output
// of every part (AeroPartOutputs) bit for bit; whole flights with the exact and with the SoA kernel end in the same Jolt state hash.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "ksp/aero/flight_integrator_aero.hpp"
#include "ksp/parts/part_data.hpp"
#include "ksp/unity/vector.hpp"

namespace ksp {
class FloatCurve;
class Part;
}  // namespace ksp

namespace ksp::physics {

// Everything UpdateAerodynamics + SetDrag write for one part (the part's fields, its DragCubeList CubeData and the force record).
struct AeroPartOutputs {
    double aerodynamicArea = 0.0, exposedArea = 0.0, submergedDynamicPressurekPa = 0.0, dynamicPressurekPa = 0.0;
    Vector3f dragVector{}, dragVectorDir{}, dragVectorDirLocal{};
    float dragVectorSqrMag = 0.0f, dragVectorMag = 0.0f, dragScalar = 0.0f, bodyLiftScalar = 0.0f;
    Vector3f bodyLiftLocalPosition{}, bodyLiftLocalVector{};
    DragCubeList::CubeData cubeData{};
    AeroForces forces{};

    static AeroPartOutputs Capture(const Part& part, const AeroForces& f);
    // bit-wise equality of every member; `firstDiff` names the first differing member
    bool BitEqual(const AeroPartOutputs& o, std::string* firstDiff = nullptr) const;
};

// The curve values of the face loop that depend only on the vessel's Mach number (evaluated once per tick).
struct AeroMachCurves {
    float mach = 0.0f;   // (float)FlightIntegrator.mach
    float tail = 0.0f, surface = 0.0f, tip = 0.0f, multiplier = 0.0f;  // SurfaceCurves (DragCurveValue)
    float cdPower = 0.0f;   // DragCurveCdPower.Evaluate(mach)
    float exposedDiv = 0.0f;  // DragCurveMultiplier.Evaluate(mach)
    float liftMach = 0.0f;  // PhysicsGlobals.BodyLiftCurve.liftMachCurve.Evaluate((float)mach)
    // the curves they were evaluated from (a part is covered only when its DragCubeList points at the same ones)
    const FloatCurve* cTail = nullptr;
    const FloatCurve* cSurface = nullptr;
    const FloatCurve* cTip = nullptr;
    const FloatCurve* cMultiplier = nullptr;
    const FloatCurve* cCd = nullptr;
    const FloatCurve* cCdPower = nullptr;
    const FloatCurve* cDragMultiplier = nullptr;
    // PhysicsGlobals' curves at (float)mach
    static AeroMachCurves Evaluate(float mach);
};

// One part's per-face constants (copied from its DragCubeList) and the derived per-face values.
struct AeroFaceRow {
    std::array<float, 6> areaOccluded{}, weightedDrag{}, weightedDepth{};
    std::array<float, 6> cd{};      // DragCurveCd.Evaluate(weightedDrag) where weightedDrag < 1 (else the weighted drag itself)
    std::array<float, 6> invDrag{};  // DragCubeList.cs:401: weightedDrag in (0.01, 1) -> 1 / weightedDrag, else 1
    void Build(const DragCubeList& cubes);  // copy + derive (DragCurveCd of the list)
    bool Matches(const DragCubeList& cubes) const;  // the three arrays are bit-identical to the list's
};

class AeroSoaKernel {
public:
    // DragCubeList.AddSurfaceDragDirection(-dirLocal, mach) on a row with the hoisted curves (DragCubeList.cs:367-419). `liftCurve` is
    // the part's BodyLiftCurve.liftCurve (evaluated per windward face).
    static DragCubeList::CubeData SurfaceDrag(const AeroMachCurves& m, const AeroFaceRow& row, const FloatCurve& liftCurve,
                                              const Vector3f& dragVectorDirLocal);
    // FlightIntegrator.cs:2371-2380 for the cube model: num6 = CalculateDragValue_Cube (AreaDrag * cacheDragCubeMultiplier, :2429) *
    // pseudoReDragMult; dragScalar = (float)(q * num6 * num2) * cacheDragMultiplier, 0 when num6 is NaN or 0. `num6` is returned too.
    static float DragScalar(const FlightIntegratorAero& fi, float areaDrag, double q, double num2, double& num6);
    // FlightIntegrator.cs:2383: (float)q * bodyLiftMultiplier * cacheBodyLiftMultiplier * liftMach
    static float BodyLiftScalar(const FlightIntegratorAero& fi, const AeroMachCurves& m, float bodyLiftMultiplier, double q);
    // The scalar statements of UpdateAerodynamics after SetDrag (FlightIntegrator.cs:2309-2389) for a covered part, given the CubeData
    // SetDrag produced: writes the part's fields (aerodynamicArea, the dynamic pressures, dragScalar, bodyLiftScalar, the body-lift
    // locals) and returns the force record. `flag3` = dragVectorSqrMag != 0; `out` already holds the angular-drag reset.
    static void PartScalars(const FlightIntegratorAero& fi, const AeroMachCurves& m, Part& part, const AeroRigidbody& rb, bool flag3,
                            AeroForces& out);

    // One tick over `parts` (FlightIntegrator.Integrate's order) with their rigid bodies; out[i] is the force record of parts[i].
    // Covered parts run the kernel, the others FlightIntegratorAero::UpdateAerodynamics<MonoFloat> (listed in Fallback()).
    void Run(const FlightIntegratorAero& fi, const std::vector<Part*>& parts, const std::vector<AeroRigidbody>& rbs,
             std::vector<AeroForces>& out);
    size_t CoveredParts() const { return covered_; }
    const std::vector<const Part*>& Fallback() const { return fallback_; }
    size_t RowRebuilds() const { return rebuilds_; }  // rows rebuilt over the kernel's life (first build included)
    void Clear();

private:
    struct Slot {
        const Part* part = nullptr;
        AeroFaceRow row;
        bool built = false;
    };
    std::vector<Slot> slots_;  // parallel to the last `parts`
    size_t covered_ = 0, rebuilds_ = 0;
    std::vector<const Part*> fallback_;
};

}  // namespace ksp::physics
