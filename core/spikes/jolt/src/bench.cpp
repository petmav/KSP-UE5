#include "bench.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "craft_model.hpp"
#include "ksp_context.hpp"

namespace spike {

namespace fs = std::filesystem;

Args::Args(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        std::string s = argv[i];
        if (s.rfind("--", 0) == 0) {
            const std::string k = s.substr(2);
            if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) opts[k] = argv[++i];
            else opts[k] = "1";
        } else {
            positional.push_back(s);
        }
    }
}

std::string Args::Str(const std::string& k, const std::string& def) const {
    auto it = opts.find(k);
    return it == opts.end() ? def : it->second;
}

double Args::Num(const std::string& k, double def) const {
    auto it = opts.find(k);
    return it == opts.end() ? def : std::stod(it->second);
}

std::vector<std::string> Args::List(const std::string& k, const std::string& def) const {
    std::vector<std::string> out;
    std::stringstream ss(Str(k, def));
    std::string item;
    while (std::getline(ss, item, ','))
        if (!item.empty()) out.push_back(item);
    return out;
}

std::string CraftPath(const std::string& c) {
    if (fs::exists(c)) return c;
    for (const std::string& dir : {std::string(SPIKE_CRAFT_DIR), std::string(SPIKE_GEN_CRAFT_DIR)}) {
        const fs::path p = fs::path(dir) / (c + ".craft");
        if (fs::exists(p)) return p.string();
    }
    throw std::runtime_error("craft not found: " + c);
}

// Solver steps per model: the articulated model needs PhysX-like iteration counts to hold the PartJoint chain (KSP: 50 PhysX
// position iterations [runtime]; 10 Jolt velocity steps let the chain stretch by a metre on the pad); the reduced model has
// few or no joints and runs Jolt's defaults (10 / 2). --vel-steps / --pos-steps override both.
void SetModel(FlightConfig& c, const std::string& m, const Args& a) {
    c.model = m;
    c.world.velocitySteps = static_cast<int>(a.Num("vel-steps", m == "articulated" ? a.Num("art-vel-steps", 50) : 10));
    c.world.positionSteps = static_cast<int>(a.Num("pos-steps", 2));
}

FlightConfig ConfigFromArgs(const Args& a) {
    FlightConfig c;
    SetModel(c, a.Str("model", c.model), a);
    c.ticks = static_cast<int>(a.Num("ticks", c.ticks));
    c.settleTicks = static_cast<int>(a.Num("settle", c.settleTicks));
    c.dt = a.Num("dt", c.dt);
    if (a.Has("warp")) c.dt = 0.02 * a.Num("warp", 1.0);
    c.collisionSteps = static_cast<int>(a.Num("substeps", c.collisionSteps));
    c.throttle = static_cast<float>(a.Num("throttle", c.throttle));
    c.aero = !a.Has("no-aero");
    const std::string k = a.Str("kernel", "exact");
    c.kernel = k == "soa" ? PartPipeline::AeroKernel::SoA : k == "both" ? PartPipeline::AeroKernel::Both : PartPipeline::AeroKernel::Exact;
    c.terrain = !a.Has("no-terrain");
    c.terrainCfg.quadsPerSide = static_cast<int>(a.Num("quads", c.terrainCfg.quadsPerSide));
    c.rotatingFrame = !a.Has("inertial");
    c.originAtBody = !a.Has("origin-pad");
    c.perTickShift = a.Has("shift");
    c.gravityEase = !a.Has("no-ease");
    c.breaking = !a.Has("no-break");
    c.stress = !a.Has("no-stress");
    c.inertiaFloor = !a.Has("no-inertia-floor");
    c.fireDecouplersAtTick = static_cast<int>(a.Num("stage-at", -1));
    c.orbit = a.Has("orbit");
    c.orbitAltitude = a.Num("orbit-alt", c.orbitAltitude);
    c.sampleEvery = static_cast<int>(a.Num("sample-every", c.sampleEvery));
    c.debug = a.Has("debug");
    if (a.Has("pad")) c.throttle = 0.0f;
    c.world.threads = static_cast<int>(a.Num("threads", c.world.threads));
    return c;
}

void PrintResultHeader() {
    std::printf("| craft | parts | model | bodies | constr. | vel/pos | thr | dt | tick mean | p95 | step | pipeline | flush | after | "
                "precalc | shift | alt end (m) | v end | jointErr max | breaks | first break | load ratio max |\n");
    std::printf("|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n");
}

void PrintResult(const FlightResult& r, const FlightConfig& c) {
    std::printf("| %s | %d | %s | %d | %d | %d/%d | %d | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f | %.1f | %.1f | %.4f | %d | "
                "%s | %.3f |%s\n",
                r.craft.c_str(), r.parts, r.model.c_str(), r.bodies, r.constraints, c.world.velocitySteps, c.world.positionSteps,
                c.world.threads, c.dt, r.total.mean, r.total.p95, r.step.mean, r.pipeline.mean, r.flush.mean, r.after.mean,
                r.precalc.mean, r.shift.mean, r.altitudeEnd, r.speedEnd, r.maxJointError, r.breaks,
                r.firstBreakTick >= 0 ? (r.firstBreak + " @" + std::to_string(r.firstBreakTick)).c_str() : "-", r.maxLoadRatio,
                r.exploded ? " EXPLODED" : "");
    std::fflush(stdout);
}

namespace {

struct Csv {
    std::ofstream f;
    explicit Csv(const std::string& path) {
        if (path.empty()) return;
        f.open(path, std::ios::binary);
        f << "bench,craft,parts,physical,model,bodies,constraints,vel_steps,pos_steps,threads,dt,ticks,tick_mean_ms,tick_p50_ms,tick_p95_ms,"
             "tick_max_ms,step_ms,pipeline_ms,flush_ms,after_ms,precalc_ms,shift_ms,transforms_ms,mass_ms,atmosphere_ms,aero_setup_ms,aero_ms,"
             "engines_ms,alt_end,v_end,mach_max,q_max,joint_err_max,breaks,first_break_tick,load_ratio_max,staging_ms,bodies_after_staging,"
             "soa_compared,soa_mismatches,exploded,extra\n";
    }
    void Row(const std::string& bench, const FlightResult& r, const FlightConfig& c, const std::string& extra = "") {
        if (!f.is_open()) return;
        const double n = std::max(1, r.measuredTicks);
        f << bench << ',' << r.craft << ',' << r.parts << ',' << r.physicalParts << ',' << r.model << ',' << r.bodies << ',' << r.constraints << ','
          << c.world.velocitySteps << ',' << c.world.positionSteps << ',' << c.world.threads << ',' << c.dt << ',' << c.ticks << ','
          << r.total.mean << ',' << r.total.p50 << ',' << r.total.p95 << ',' << r.total.max << ',' << r.step.mean << ',' << r.pipeline.mean << ','
          << r.flush.mean << ',' << r.after.mean << ',' << r.precalc.mean << ',' << r.shift.mean << ',' << r.pipelineSum.transformsMs / n << ','
          << r.pipelineSum.massMs / n << ',' << r.pipelineSum.atmosphereMs / n << ',' << r.pipelineSum.aeroSetupMs / n << ','
          << r.pipelineSum.aeroMs / n << ',' << r.pipelineSum.enginesMs / n << ',' << r.altitudeEnd << ',' << r.speedEnd << ',' << r.machMax << ','
          << r.qMax << ',' << r.maxJointError << ',' << r.breaks << ',' << r.firstBreakTick << ',' << r.maxLoadRatio << ',' << r.stagingMs << ','
          << r.bodiesAfterStaging << ',' << r.soaCompared << ',' << r.soaMismatches << ',' << (r.exploded ? 1 : 0) << ',' << extra << '\n';
        f.flush();
    }
};

const std::vector<std::string> kModels = {"articulated", "reduced-whole", "reduced-split"};

}  // namespace

int RunBench(KspContext& ctx, const std::string& name, const Args& args) {
    Csv csv(args.Str("csv", ""));
    const std::vector<std::string> models = args.List("models", "articulated,reduced-whole,reduced-split");
    std::map<std::string, CraftModel> crafts;
    auto craft = [&](const std::string& c) -> CraftModel& {
        auto it = crafts.find(c);
        if (it == crafts.end()) it = crafts.emplace(c, BuildCraftModel(ctx, CraftPath(c), c)).first;
        return it->second;
    };
    // NOTE: a CraftModel's kspcore Parts carry run state (drag cube data, aero fields) between runs; every run starts from the
    // same assembled pose (the bodies are rebuilt from the craft-frame transforms), so the runs are independent for the physics.
    if (name == "scaling") {
        // tick cost vs part count, both models; pad settle + powered ascent with aero
        const auto list = args.List("crafts", "PhysProf-S,PhysProf-M2-AS,PhysProf-L2-AS,PhysProf-XL1-AS,PhysProf-XL2-AS");
        PrintResultHeader();
        for (const std::string& c : list)
            for (const std::string& m : models) {
                FlightConfig cfg = ConfigFromArgs(args);
                SetModel(cfg, m, args);
                FlightResult r = RunFlight(ctx, craft(c), cfg);
                PrintResult(r, cfg);
                csv.Row(name, r, cfg);
            }
        return 0;
    }
    if (name == "iterations") {
        const auto list = args.List("crafts", "PhysProf-L2-AS");
        const auto steps = args.List("steps", "4/1,10/2,20/2,50/2,50/10");
        PrintResultHeader();
        for (const std::string& c : list)
            for (const std::string& m : models)
                for (const std::string& s : steps) {
                    FlightConfig cfg = ConfigFromArgs(args);
                    SetModel(cfg, m, args);
                    const size_t slash = s.find('/');
                    cfg.world.velocitySteps = std::stoi(s.substr(0, slash));
                    cfg.world.positionSteps = std::stoi(s.substr(slash + 1));
                    FlightResult r = RunFlight(ctx, craft(c), cfg);
                    PrintResult(r, cfg);
                    csv.Row(name, r, cfg, s);
                }
        return 0;
    }
    if (name == "threads") {
        const auto list = args.List("crafts", "PhysProf-L2-AS,PhysProf-XL2-AS");
        const auto threads = args.List("thread-list", "1,2,4,8,16");
        PrintResultHeader();
        for (const std::string& c : list)
            for (const std::string& m : models)
                for (const std::string& t : threads) {
                    FlightConfig cfg = ConfigFromArgs(args);
                    SetModel(cfg, m, args);
                    cfg.world.threads = std::stoi(t);
                    FlightResult r = RunFlight(ctx, craft(c), cfg);
                    PrintResult(r, cfg);
                    csv.Row(name, r, cfg);
                }
        return 0;
    }
    if (name == "warp") {
        // physics warp: the same game time at dt = 0.02 x rate; deviation from the 1x run and stability
        const auto list = args.List("crafts", "PhysProf-L2-AS");
        const auto rates = args.List("rates", "1,2,3,4,6,8,10,16,25,50");
        const double gameTime = args.Num("game-time", 30.0);
        const double settle = args.Num("settle-time", 2.0);
        std::printf("| craft | model | warp | dt | ticks | tick mean ms | step ms | alt end | v end | d alt vs 1x | d v vs 1x | jointErr max | breaks | "
                    "real-time warp by substeps (x) | state |\n|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n");
        for (const std::string& c : list)
            for (const std::string& m : models) {
                double alt1 = 0, v1 = 0, tick1 = 0;
                for (const std::string& rate : rates) {
                    FlightConfig cfg = ConfigFromArgs(args);
                    SetModel(cfg, m, args);
                    const double w = std::stod(rate);
                    cfg.dt = 0.02 * w;
                    cfg.ticks = static_cast<int>(std::lround(gameTime / cfg.dt));
                    cfg.settleTicks = static_cast<int>(std::lround(settle / cfg.dt));
                    cfg.sampleEvery = std::max(1, static_cast<int>(std::lround(0.2 / cfg.dt)));
                    FlightResult r = RunFlight(ctx, craft(c), cfg);
                    if (w == 1.0) {
                        alt1 = r.altitudeEnd;
                        v1 = r.speedEnd;
                        tick1 = r.total.mean;
                    }
                    // substepping: w ticks of 0.02 s per 0.02 s of wall time; real time sustainable while w * tick1 <= 20 ms
                    const double subWarp = tick1 > 0 ? 20.0 / tick1 : 0.0;
                    std::printf("| %s | %s | %g | %.2f | %d | %.3f | %.3f | %.1f | %.2f | %+.2f | %+.3f | %.4f | %d | %.0f | %s |\n", c.c_str(), m.c_str(),
                                w, cfg.dt, cfg.ticks, r.total.mean, r.step.mean, r.altitudeEnd, r.speedEnd, r.altitudeEnd - alt1, r.speedEnd - v1,
                                r.maxJointError, r.breaks, subWarp, r.exploded ? "EXPLODED" : (r.breaks ? "broke" : "ok"));
                    std::fflush(stdout);
                    csv.Row(name, r, cfg, rate);
                }
            }
        return 0;
    }
    if (name == "shift") {
        // KSP's every-tick FloatingOrigin shift re-added (bodies + static colliders) vs none (lever 1)
        const auto list = args.List("crafts", "PhysProf-S,PhysProf-M2-AS,PhysProf-L2-AS");
        PrintResultHeader();
        for (const std::string& c : list)
            for (const std::string& m : models)
                for (int s = 0; s < 2; s++) {
                    FlightConfig cfg = ConfigFromArgs(args);
                    SetModel(cfg, m, args);
                    cfg.perTickShift = s == 1;
                    FlightResult r = RunFlight(ctx, craft(c), cfg);
                    PrintResult(r, cfg);
                    csv.Row(name, r, cfg, s ? "shift" : "noshift");
                }
        return 0;
    }
    if (name == "pipeline") {
        // lever 2: per-part work, exact AoS kernel vs SoA kernel (and both with the bit comparison)
        const auto list = args.List("crafts", "PhysProf-S,PhysProf-M2-AS,PhysProf-L2-AS,PhysProf-XL2-AS");
        std::printf("| craft | parts | kernel | pipeline ms/tick | us/part | transforms | mass | atmosphere | aero setup | aero | engines | compared | "
                    "mismatches |\n|---|---|---|---|---|---|---|---|---|---|---|---|---|\n");
        for (const std::string& c : list)
            for (const std::string& k : args.List("kernels", "exact,soa,both")) {
                FlightConfig cfg = ConfigFromArgs(args);
                SetModel(cfg, args.Str("model", "reduced-whole"), args);
                cfg.kernel = k == "soa" ? PartPipeline::AeroKernel::SoA : k == "both" ? PartPipeline::AeroKernel::Both : PartPipeline::AeroKernel::Exact;
                FlightResult r = RunFlight(ctx, craft(c), cfg);
                const double n = std::max(1, r.measuredTicks);
                const auto& p = r.pipelineSum;
                std::printf("| %s | %d | %s | %.4f | %.3f | %.4f | %.4f | %.4f | %.4f | %.4f | %.4f | %lld | %lld |\n", c.c_str(), r.parts, k.c_str(),
                            r.pipeline.mean, 1000.0 * r.pipeline.mean / r.parts, p.transformsMs / n, p.massMs / n, p.atmosphereMs / n,
                            p.aeroSetupMs / n, p.aeroMs / n, p.enginesMs / n, r.soaCompared, r.soaMismatches);
                std::fflush(stdout);
                csv.Row(name, r, cfg, k);
            }
        return 0;
    }
    if (name == "staging") {
        // fire every decoupler 5 s into the ascent: one-off cost and the tick cost after
        const auto list = args.List("crafts", "PhysProf-M2-AS,PhysProf-L2-AS,PhysProf-XL2-AS");
        std::printf("| craft | model | bodies before | bodies after | staging ms | tick mean ms | alt end | breaks |\n|---|---|---|---|---|---|---|---|\n");
        for (const std::string& c : list)
            for (const std::string& m : models) {
                FlightConfig cfg = ConfigFromArgs(args);
                SetModel(cfg, m, args);
                cfg.fireDecouplersAtTick = cfg.settleTicks + 250;
                FlightResult r = RunFlight(ctx, craft(c), cfg);
                std::printf("| %s | %s | %d | %d | %.3f | %.3f | %.1f | %d |\n", c.c_str(), m.c_str(), r.bodies, r.bodiesAfterStaging, r.stagingMs,
                            r.total.mean, r.altitudeEnd, r.breaks);
                std::fflush(stdout);
                csv.Row(name, r, cfg);
            }
        return 0;
    }
    if (name == "precision") {
        // lever 1: a settled vessel on the pad, physics frame origin at Kerbin's centre vs at the pad
        const auto list = args.List("crafts", "PhysProf-S,PhysProf-L2-AS");
        std::printf("| craft | model | origin | double build | max speed after settle (m/s) | CoM drift (m) | tick mean ms |\n|---|---|---|---|---|---|---|\n");
        for (const std::string& c : list)
            for (const std::string& m : models)
                for (int o = 0; o < 2; o++) {
                    FlightConfig cfg = ConfigFromArgs(args);
                    SetModel(cfg, m, args);
                    cfg.originAtBody = o == 0;
                    cfg.throttle = 0.0f;
                    cfg.aero = false;
                    cfg.ticks = static_cast<int>(args.Num("ticks", 750));
                    cfg.settleTicks = 250;
                    cfg.sampleEvery = 5;
                    FlightResult r = RunFlight(ctx, craft(c), cfg);
                    double vmax = 0, x0 = 0, y0 = 0, z0 = 0, drift = 0;
                    bool first = true;
                    for (const Sample& s : r.samples) {
                        if (s.tick < cfg.settleTicks) continue;
                        if (first) {
                            x0 = s.comX, y0 = s.comY, z0 = s.comZ;
                            first = false;
                        }
                        vmax = std::max(vmax, s.speed);
                        drift = std::max(drift, std::sqrt((s.comX - x0) * (s.comX - x0) + (s.comY - y0) * (s.comY - y0) + (s.comZ - z0) * (s.comZ - z0)));
                    }
#ifdef JPH_DOUBLE_PRECISION
                    const char* dbl = "yes";
#else
                    const char* dbl = "no";
#endif
                    std::printf("| %s | %s | %s | %s | %.6f | %.6f | %.3f |\n", c.c_str(), m.c_str(), o == 0 ? "Kerbin centre" : "pad", dbl, vmax, drift,
                                r.total.mean);
                    std::fflush(stdout);
                    csv.Row(name, r, cfg, o == 0 ? "body" : "pad");
                }
        return 0;
    }
    if (name == "orbit") {
        // a coasting rigid vessel in an 80 km orbit for one period: radius / speed error of plain integration vs the circular orbit
        const auto list = args.List("crafts", "PhysProf-L2-AS");
        const auto rates = args.List("rates", "1,4,10,50");
        std::printf("| craft | model | warp | dt | ticks | tick mean ms | radius error max (m) | speed error max (m/s) |\n|---|---|---|---|---|---|---|---|\n");
        for (const std::string& c : list)
            for (const std::string& m : models)
                for (const std::string& rate : rates) {
                    FlightConfig cfg = ConfigFromArgs(args);
                    SetModel(cfg, m, args);
                    cfg.orbit = true;
                    cfg.terrain = false;
                    cfg.aero = false;
                    cfg.throttle = 0.0f;
                    cfg.settleTicks = 0;
                    cfg.dt = 0.02 * std::stod(rate);
                    const double R = ctx.Kerbin().Radius + cfg.orbitAltitude;
                    const double mu = ctx.Kerbin().gMagnitudeAtCenter;
                    const double period = 2.0 * 3.14159265358979323846 * std::sqrt(R * R * R / mu);
                    cfg.ticks = static_cast<int>(std::lround(args.Num("periods", 1.0) * period / cfg.dt));
                    cfg.sampleEvery = std::max(1, cfg.ticks / 400);
                    FlightResult r = RunFlight(ctx, craft(c), cfg);
                    const double v0 = std::sqrt(mu / R);
                    double rErr = 0, vErr = 0;
                    for (const Sample& s : r.samples) {
                        const double rad = std::sqrt(s.comX * s.comX + s.comY * s.comY + s.comZ * s.comZ);
                        rErr = std::max(rErr, std::abs(rad - R));
                        vErr = std::max(vErr, std::abs(s.speed - v0));
                    }
                    std::printf("| %s | %s | %s | %.2f | %d | %.3f | %.3f | %.4f |\n", c.c_str(), m.c_str(), rate.c_str(), cfg.dt, cfg.ticks, r.total.mean,
                                rErr, vErr);
                    std::fflush(stdout);
                    csv.Row(name, r, cfg, rate);
                }
        return 0;
    }
    throw std::runtime_error("unknown bench " + name);
}

}  // namespace spike
