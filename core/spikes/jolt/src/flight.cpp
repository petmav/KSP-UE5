#include "flight.hpp"

#include <Jolt/Math/Mat44.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <numeric>
#include <stdexcept>

#include "articulated.hpp"
#include "ksp_context.hpp"
#include "reduced.hpp"

namespace spike {

using namespace JPH;
using Clock = std::chrono::steady_clock;

namespace {

double Ms(Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); }

Percentiles Stats(std::vector<double> v) {
    Percentiles p;
    if (v.empty()) return p;
    p.mean = std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(v.size());
    std::sort(v.begin(), v.end());
    p.p50 = v[v.size() / 2];
    p.p95 = v[std::min(v.size() - 1, static_cast<size_t>(0.95 * static_cast<double>(v.size())))];
    p.max = v.back();
    return p;
}

std::unique_ptr<VesselPhysics> MakeModel(const FlightConfig& cfg) {
    if (cfg.model == "articulated") {
        auto m = std::make_unique<ArticulatedVessel>();
        m->breakingEnabled = cfg.breaking;
        return m;
    }
    std::unique_ptr<ReducedVessel> m;
    if (cfg.model == "reduced-whole") m = std::make_unique<ReducedVessel>(ReducedVessel::Clustering::Whole);
    else if (cfg.model == "reduced-split") m = std::make_unique<ReducedVessel>(ReducedVessel::Clustering::SplitAtDecouplers);
    else throw std::runtime_error("unknown model " + cfg.model);
    m->stressEnabled = cfg.stress;
    m->breakingEnabled = cfg.breaking;
    m->inertiaFloor = cfg.inertiaFloor;
    return m;
}

DVec3 ToD(RVec3Arg v) { return DVec3(v); }

}  // namespace

FlightResult RunFlight(KspContext& ctx, CraftModel& craft, const FlightConfig& cfg) {
    FlightResult r;
    r.craft = craft.name;
    r.model = cfg.model;
    r.parts = static_cast<int>(craft.parts.size());
    r.physicalParts = craft.physicalCount;
    auto tb0 = Clock::now();
    World world(cfg.world);
    const SurfaceFrame pad = PadFrame(ctx);
    const ksp::CelestialBody& kerbin = ctx.Kerbin();
    const double mu = kerbin.gMagnitudeAtCenter;
    const DVec3 omega = cfg.rotatingFrame && !cfg.orbit ? DVec3(0.0, -2.0 * JPH_PI / kerbin.rotationPeriod, 0.0) : DVec3::sZero();

    // ---- where the craft starts (physics frame = body-fixed; inertial for the orbit case)
    double minY = 1e30;
    for (const PartSpec& s : craft.parts) {
        if (!s.physical) continue;
        for (int c = 0; c < 8; c++) {
            const Vec3 corner = s.boxCenter + Vec3((c & 1) ? s.boxHalf.GetX() : -s.boxHalf.GetX(), (c & 2) ? s.boxHalf.GetY() : -s.boxHalf.GetY(),
                                                   (c & 4) ? s.boxHalf.GetZ() : -s.boxHalf.GetZ());
            minY = std::min(minY, s.pos.GetY() + static_cast<double>((s.rot * corner).GetY()));
        }
    }
    DVec3 startPoint;  // body-fixed position of the craft origin
    if (cfg.orbit) {
        // the CoM on the circular orbit's radius (craft Y = up)
        DVec3 comCraft = DVec3::sZero();
        double mSum = 0.0;
        for (const PartSpec& s : craft.parts)
            if (s.physical) {
                comCraft += (s.pos + DVec3(s.rot * s.comLocal)) * static_cast<double>(s.rbMass);
                mSum += s.rbMass;
            }
        comCraft /= mSum;
        startPoint = pad.up * (kerbin.Radius + cfg.orbitAltitude - comCraft.GetY()) - pad.east * comCraft.GetX() - pad.north * comCraft.GetZ();
    } else {
        startPoint = pad.up * (pad.radius + 0.05 - minY);
    }
    DVec3 frameOrigin = cfg.originAtBody ? DVec3::sZero() : (cfg.orbit ? startPoint : pad.up * pad.radius);

    TerrainPatch terrain;
    if (cfg.terrain && !cfg.orbit) {
        terrain = BuildTerrainPatch(world, ctx, pad, frameOrigin, cfg.terrainCfg);
        r.statics = static_cast<int>(terrain.bodies.size());
        r.triangles = terrain.triangles;
        r.terrainMs = terrain.heightEvalMs + terrain.buildMs;
    }
    Placement pl;
    pl.frameOrigin = frameOrigin;
    const Vec3 e(static_cast<float>(pad.east.GetX()), static_cast<float>(pad.east.GetY()), static_cast<float>(pad.east.GetZ()));
    const Vec3 u(static_cast<float>(pad.up.GetX()), static_cast<float>(pad.up.GetY()), static_cast<float>(pad.up.GetZ()));
    const Vec3 n(static_cast<float>(pad.north.GetX()), static_cast<float>(pad.north.GetY()), static_cast<float>(pad.north.GetZ()));
    pl.rotation = Mat44(Vec4(e, 0), Vec4(u, 0), Vec4(n, 0), Vec4(0, 0, 0, 1)).GetQuaternion().Normalized();  // craft X,Y,Z -> east, up, north
    pl.translation = startPoint - frameOrigin;

    std::unique_ptr<VesselPhysics> model = MakeModel(cfg);
    model->Build(world, craft, pl, 1u);
    if (cfg.orbit) model->SetLinearVelocity(Vec3(pad.east * std::sqrt(mu / (kerbin.Radius + cfg.orbitAltitude))));
    world.System().OptimizeBroadPhase();
    r.bodies = model->BodyCount();
    r.constraints = model->ConstraintCount();
    r.buildMs = Ms(tb0, Clock::now()) - r.terrainMs;

    PartPipeline pipe(ctx, craft);
    ReducedVessel* reduced = dynamic_cast<ReducedVessel*>(model.get());
    std::vector<double> tPre, tPipe, tFlush, tStep, tAfter, tShift, tTotal;
    const float dt = static_cast<float>(cfg.dt);
    for (int tick = 0; tick < cfg.ticks; tick++) {
        const auto t0 = Clock::now();
        // ---- VesselPrecalculate: state from the bodies, gravity at the CoM (VesselPrecalculate.cs:281-368)
        model->ReadState();
        const DVec3 rBody = frameOrigin + ToD(model->com);
        const double rl = rBody.Length();
        DVec3 g = rBody * (-mu / (rl * rl * rl));
        if (!omega.IsNearZero()) {
            const DVec3 v(model->comVel);
            g += omega.Cross(v) * -2.0;                     // FlightGlobals.getCoriolisAcc
            g += omega.Cross(omega.Cross(-rBody));          // FlightGlobals.getCentrifugalAcc (pos = body - vessel)
        }
        const double ease = cfg.gravityEase && !cfg.orbit ? std::min(1.0, 0.0625 * (tick + 1)) : 1.0;
        g *= ease;
        const auto t1 = Clock::now();
        // ---- per-part work (FlightIntegrator: mass, atmosphere, aero; engine modules)
        PipelineTimes pt;
        pipe.Tick(*model, frameOrigin, cfg.aero, cfg.kernel, tick >= cfg.settleTicks ? cfg.throttle : 0.0f, pt);
        const auto t2 = Clock::now();
        // ---- FlightIntegrator.Integrate: gravity + buffered forces into the solver
        model->AddUniformAcceleration(Vec3(g));
        model->Flush();
        const auto t3 = Clock::now();
        model->breakAllowed = tick >= cfg.breakGraceTicks;
        world.Step(dt, cfg.collisionSteps);
        const auto t4 = Clock::now();
        const std::vector<BreakEvent> br = model->AfterStep(dt, tick);
        const auto t5 = Clock::now();
        if (cfg.perTickShift) {
            // FloatingOrigin.setOffset every tick while Krakensbane holds a frame velocity: every body and static collider moves
            model->ReadState();
            const Vec3 delta = -model->comVel * dt;
            model->ShiftOrigin(delta);
            if (!terrain.bodies.empty()) ShiftStatics(world, terrain, delta);
            frameOrigin -= DVec3(delta);
        }
        const auto t6 = Clock::now();
        if (tick == cfg.fireDecouplersAtTick) {
            const auto s0 = Clock::now();
            r.bodiesAfterStaging = model->FireDecouplers();
            r.stagingMs = Ms(s0, Clock::now());
        }
        if (reduced && tick >= cfg.settleTicks && reduced->maxLoadRatio > r.maxLoadRatio) {
            r.maxLoadRatio = reduced->maxLoadRatio;
            const JointSpec& js = craft.joints[reduced->maxLoadJoint];
            r.maxLoadJoint = craft.parts[js.child].name + "->" + craft.parts[js.parent].name + " t=" + std::to_string(tick * cfg.dt);
        }
        for (const BreakEvent& b : br) {
            if (cfg.debug) {
                const JointSpec& js = craft.joints[b.joint];
                std::fprintf(stderr, "break tick %d joint %d %s->%s mode %d size %d sub %d: F %.1f (cap %.1f) T %.1f (cap %.1f)\n", b.tick, b.joint,
                             craft.parts[js.child].name.c_str(), craft.parts[js.parent].name.c_str(), js.mode, js.nodeSize, js.internalJoints,
                             b.force, js.breakForceSub, b.torque, js.breakTorqueSub);
            }
            if (r.firstBreakTick < 0) {
                r.firstBreakTick = b.tick;
                const JointSpec& js = craft.joints[b.joint];
                r.firstBreak = craft.parts[js.child].name + "->" + craft.parts[js.parent].name;
            }
            r.breaks++;
        }
        if (tick >= cfg.settleTicks) {
            tPre.push_back(Ms(t0, t1));
            tPipe.push_back(Ms(t1, t2));
            tFlush.push_back(Ms(t2, t3));
            tStep.push_back(Ms(t3, t4));
            tAfter.push_back(Ms(t4, t5));
            tShift.push_back(Ms(t5, t6));
            tTotal.push_back(Ms(t0, t6));
            r.pipelineSum.transformsMs += pt.transformsMs;
            r.pipelineSum.massMs += pt.massMs;
            r.pipelineSum.atmosphereMs += pt.atmosphereMs;
            r.pipelineSum.aeroSetupMs += pt.aeroSetupMs;
            r.pipelineSum.aeroMs += pt.aeroMs;
            r.pipelineSum.enginesMs += pt.enginesMs;
            r.machMax = std::max(r.machMax, pipe.mach);
            r.qMax = std::max(r.qMax, pipe.dynamicPressure);
        }
        if (tick % cfg.sampleEvery == 0 || tick == cfg.ticks - 1) {
            model->ReadState();
            const DVec3 cb = frameOrigin + ToD(model->com);
            Sample s;
            s.tick = tick;
            s.t = tick * cfg.dt;
            s.altitude = cb.Length() - kerbin.Radius;
            s.speed = model->comVel.Length();
            s.comX = cb.GetX();
            s.comY = cb.GetY();
            s.comZ = cb.GetZ();
            s.jointError = model->MaxJointError();
            {
                const Vec3 axis = model->partRot[craft.root] * Vec3::sAxisY();
                const DVec3 upv = cb.Normalized();
                const double c = std::clamp(DVec3(axis).Dot(upv), -1.0, 1.0);
                s.tiltDeg = std::acos(c) * 180.0 / JPH_PI;
            }
            r.maxJointError = std::max(r.maxJointError, s.jointError);
            r.samples.push_back(s);
            if (!std::isfinite(s.altitude) || !std::isfinite(s.speed) || s.speed > 5.0e4) {
                r.exploded = true;
                break;
            }
        }
    }
    if (cfg.debug)
        if (auto* art = dynamic_cast<ArticulatedVessel*>(model.get())) {
            const auto errs = art->JointErrors();
            for (size_t i = 0; i < std::min<size_t>(8, errs.size()); i++) {
                const JointSpec& js = craft.joints[errs[i].second];
                std::fprintf(stderr, "joint err %.4f m: joint %d %s->%s mode %d size %d sub %d autostrut %d k %.0f\n", errs[i].first, errs[i].second,
                             craft.parts[js.child].name.c_str(), craft.parts[js.parent].name.c_str(), js.mode, js.nodeSize, js.internalJoints,
                             js.autostrut ? 1 : 0, js.stiffness);
            }
        }
    model->ReadState();
    const DVec3 cb = frameOrigin + ToD(model->com);
    r.altitudeEnd = cb.Length() - kerbin.Radius;
    r.speedEnd = model->comVel.Length();
    r.measuredTicks = static_cast<int>(tTotal.size());
    r.precalc = Stats(tPre);
    r.pipeline = Stats(tPipe);
    r.flush = Stats(tFlush);
    r.step = Stats(tStep);
    r.after = Stats(tAfter);
    r.shift = Stats(tShift);
    r.total = Stats(tTotal);
    r.soaMismatches = pipe.soaMismatches;
    r.soaCompared = pipe.soaCompared;
    if (r.bodiesAfterStaging == 0) r.bodiesAfterStaging = model->BodyCount();
    return r;
}

}  // namespace spike
