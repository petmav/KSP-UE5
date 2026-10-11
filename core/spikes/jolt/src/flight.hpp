// Jolt spike: one flight scenario (pad -> ignition -> powered ascent, or a coast in orbit) with per-phase timing. The tick
// follows KSP's measured order (PHYSICS-ENGINE-REVIEW.md 1.2): VesselPrecalculate (state readback, gravity at the CoM with
// Coriolis / centrifugal in the rotating frame), the per-part work (mass, atmosphere, aero, engines), the force flush
// (FlightIntegrator.Integrate), the solver step, then joint loads / breaking. No FloatingOrigin / Krakensbane phase: the frame
// is body-fixed double (lever 1); `perTickShift` re-adds KSP's every-tick world shift for measurement only.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "craft_model.hpp"
#include "jolt_world.hpp"
#include "part_pipeline.hpp"
#include "terrain_patch.hpp"

namespace spike {

struct FlightConfig {
    std::string model = "articulated";  // articulated | reduced-whole | reduced-split
    int ticks = 1500;                   // 30 s at 0.02 s
    int settleTicks = 100;              // engines off on the pad first (KSP: 10 s settle, here 2 s)
    double dt = 0.02;                   // TimeWarp.fixedDeltaTime (0.02 x physics warp rate)
    int collisionSteps = 1;
    float throttle = 1.0f;
    bool aero = true;
    PartPipeline::AeroKernel kernel = PartPipeline::AeroKernel::Exact;
    bool terrain = true;
    TerrainConfig terrainCfg;
    bool rotatingFrame = true;          // KSP: Kerbin's inverse-rotation frame below 100 km [runtime]
    bool originAtBody = true;           // physics-frame origin at Kerbin's centre (needs double); false: at the pad
    bool perTickShift = false;          // emulate FloatingOrigin.setOffset every tick (measurement)
    bool gravityEase = true;            // VesselPrecalculate easing: gravity 0 -> 1 in 16 ticks after unpack
    bool breaking = true;
    // Galilean velocity frame for the inertial (orbit) case: the physics frame moves with frameVel (double bookkeeping, no body
    // moves); when the vessel's velocity in the frame exceeds the threshold the frame velocity takes it over (every body's
    // velocity changes once, positions never shift). Keeps Jolt's float velocities small for constrained parts.
    bool frameVelocity = false;
    double frameVelThreshold = 50.0;
    int breakGraceTicks = 60;           // Vessel.ignoreCollisionsFrames = 60 after ground placement
    bool stress = true;                 // reduced model: structural loads every tick
    bool inertiaFloor = true;
    bool graphSolve = false;            // reduced: indeterminate structural solve (struts share load) instead of the tree solve
    double graphKLinear = 1.0e8;
    bool graphDirect = true;
    int fireDecouplersAtTick = -1;
    bool orbit = false;                 // start in a circular equatorial-plane orbit (inertial frame, no terrain)
    double orbitAltitude = 80000.0;
    int loadWindow = 50;                // ticks at the end over which AttachJointLoads are averaged into FlightResult::jointLoads
    int sampleEvery = 10;               // trajectory / joint-error sample period (outside the timed phases)
    bool debug = false;                 // print break details and the worst joints (stderr)
    WorldConfig world;
};

struct Percentiles {
    double mean = 0, p50 = 0, p95 = 0, max = 0;
};

struct Sample {
    int tick;
    double t, altitude, speed;
    double comX, comY, comZ;  // body-fixed
    float jointError;
    double tiltDeg;  // angle between the root part's axis and the local vertical
};

struct FlightResult {
    std::string craft, model;
    int parts = 0, physicalParts = 0, bodies = 0, constraints = 0, statics = 0, triangles = 0;
    int bodiesAfterStaging = 0;
    double buildMs = 0, terrainMs = 0;
    // per tick (ms), over the measured ticks (after settle)
    Percentiles precalc, pipeline, flush, step, after, shift, total;
    PipelineTimes pipelineSum;  // summed over measured ticks
    int measuredTicks = 0;
    double altitudeEnd = 0, speedEnd = 0, machMax = 0, qMax = 0;
    float maxJointError = 0;    // over the samples
    int breaks = 0, firstBreakTick = -1;
    int frameRecenters = 0;
    std::string firstBreak;
    double graphMsMean = 0, graphItersMean = 0;  // reduced + graphSolve: per solve
    double graphFactorMs = 0;                    // last factorisation (direct method)
    size_t graphFactorBlocks = 0;
    double stagingMs = 0;       // cost of the FireDecouplers event
    float maxLoadRatio = 0;     // reduced: max load / capacity seen
    std::string maxLoadJoint;
    long long soaMismatches = 0, soaCompared = 0;
    bool exploded = false;      // NaN / runaway state
    std::vector<Sample> samples;
    std::vector<std::pair<float, float>> jointLoads;  // per attach joint at the last tick (VesselPhysics::AttachJointLoads)
};

FlightResult RunFlight(KspContext& ctx, CraftModel& craft, const FlightConfig& cfg);

}  // namespace spike
