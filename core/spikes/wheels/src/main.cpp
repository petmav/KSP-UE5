// wheel_spike: the wheel / landing-leg spike driver (core/WHEEL-SPIKE.md).
//
//   wheel_spike info  <craft>...                          wheel parameters, geometry proxies, masses
//   wheel_spike trace <craft> [--mode rest|drive|brake|drop] [--seconds s] [--every s] [options]
//                                                          one run, telemetry printed every `--every` seconds
//   wheel_spike bench rest|drive|brake|touchdown|cost [options]   the measurement sets (results/<set>.md|csv)
// <craft>: bug-e, prospector, crawler, twostage, munsplorer, skycrane (stock craft in harness/inputs/Ships) or a path.
// Options: --native explicit|jolt-vc|both, --ground pqs|flat, --friction smooth|lineal, --warp <rate>, --heading <deg>,
//          --east / --north <m> (spawn offset from the pad), --g <scale>, --no-inertia-floor, --affinity <hex mask>, --crafts a,b
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "jolt_world.hpp"
#include "ksp_context.hpp"
#include "rover.hpp"
#include "scenarios.hpp"

using namespace wspike;

namespace {

RoverConfig ConfigFrom(const Args& a) {
    RoverConfig c;
    c.native = a.Str("native", "explicit") == "jolt-vc" ? NativeModel::JoltVehicle : NativeModel::Explicit;
    c.ground = a.Str("ground", "pqs") == "flat" ? Ground::Flat : Ground::PQS;
    c.friction = a.Str("friction", "smooth") == "lineal" ? TireFriction::Model::Lineal : TireFriction::Model::Smooth;
    c.dt = 0.02 * a.Num("warp", 1.0);
    c.headingDeg = a.Num("heading", 90.0);
    c.startOffsetEast = a.Num("east", 0.0);
    c.startOffsetNorth = a.Num("north", 0.0);
    c.gravityScale = a.Num("g", 1.0);
    c.inertiaFloor = !a.Has("no-inertia-floor");
    c.legGroundOffset = a.Has("leg-offset");
    return c;
}

int Trace(spike::KspContext& ctx, const Args& a) {
    if (a.positional.empty()) throw std::runtime_error("trace: craft missing");
    auto craft = LoadCraft(ctx, a.positional[0]);
    RoverConfig cfg = ConfigFrom(a);
    const std::string mode = a.Str("mode", "rest");
    if (mode == "drop") {
        cfg.spawnClearance = 0.02f;
        cfg.spawnVerticalSpeed = -a.Num("v0", 3.0);
        cfg.gravityEase = false;
    }
    Rover rv(ctx, *craft, cfg);
    const double secs = a.Num("seconds", 10.0);
    const int every = std::max(1, static_cast<int>(std::lround(a.Num("every", 0.5) / cfg.dt)));
    const JPH::DVec3 up = rv.Up();
    const JPH::DVec3 x0 = rv.Com();
    for (int t = 0; t < static_cast<int>(secs / cfg.dt); t++) {
        Controls c;
        const double time = t * cfg.dt;
        if (mode == "drive" && time >= 2.0) c.wheelThrottle = 1.0f;
        if (mode == "brake") c.wheelThrottle = time >= 2.0 && time < a.Num("brake-at", 12.0) ? 1.0f : 0.0f;
        if (mode == "brake" && time >= a.Num("brake-at", 12.0)) c.brake = 1.0f;
        if (mode == "rest" && a.Has("brakes")) c.brake = 1.0f;
        rv.Tick(c);
        if (t % every != 0) continue;
        const JPH::Vec3 v = rv.Velocity();
        const JPH::DVec3 fw = rv.Forward();
        const double vf = v.GetX() * fw.GetX() + v.GetY() * fw.GetY() + v.GetZ() * fw.GetZ();
        const double vz = v.GetX() * up.GetX() + v.GetY() * up.GetY() + v.GetZ() * up.GetZ();
        const JPH::DVec3 d = rv.Com() - x0;
        std::printf("t %6.2f alt %7.3f dxH %8.4f vf %7.3f vz %7.3f w %6.3f gnd %d bot %d touch %d |", time + cfg.dt, rv.Altitude(),
                    (d - up * d.Dot(up)).Length(), vf, vz, rv.AngularVelocity().Length(), rv.GroundedCount(), rv.BottomedCount(),
                    rv.BodyTouchesGround() ? 1 : 0);
        for (int i = 0; i < rv.WheelCount(); i++) {
            const KspWheel& w = rv.Wheel(i);
            std::printf(" [c %.3f F %.2f k %.0f d %.0f tp %.2f om %.2f sl %.2f Fy %.2f]", w.hit.compression, w.hit.force, w.nativeSpring.spring,
                        w.nativeSpring.damper, w.nativeSpring.targetPosition, w.angularVelocity, w.tireSlip.y, w.tireForce.y);
            if (!a.Has("all-wheels") && i >= 1) break;
        }
        std::printf("\n");
        if (!std::isfinite(v.Length()) || v.Length() > 1000.0f) {
            std::printf("EXPLODED\n");
            break;
        }
    }
    const PhaseTimes& p = rv.LastTimes();
    std::printf("last tick ms: precalc %.4f managed %.4f nativePre %.4f step %.4f nativePost %.4f total %.4f\n", p.precalc, p.managed,
                p.nativePre, p.step, p.nativePost, p.total);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: wheel_spike info|trace|bench ...\n");
        return 2;
    }
    try {
        spike::JoltInit();
        const std::string cmd = argv[1];
        Args args(argc - 2, argv + 2);
#ifdef _WIN32
        if (args.Has("affinity")) {
            const DWORD_PTR mask = static_cast<DWORD_PTR>(std::stoull(args.Str("affinity", "1"), nullptr, 16));
            SetProcessAffinityMask(GetCurrentProcess(), mask);
            SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        }
#endif
        spike::KspContext ctx(true);
        int rc = 0;
        if (cmd == "info") {
            for (const std::string& c : args.positional) {
                auto craft = LoadCraft(ctx, c);
                PrintCraftInfo(ctx, *craft, ConfigFrom(args));
            }
        } else if (cmd == "trace") {
            rc = Trace(ctx, args);
        } else if (cmd == "bench") {
            if (args.positional.empty()) throw std::runtime_error("bench: name missing");
            rc = RunBench(ctx, args.positional[0], args);
        } else {
            throw std::runtime_error("unknown command " + cmd);
        }
        std::fflush(stdout);
        return rc;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
