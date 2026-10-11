// jolt_spike: the Jolt physics spike driver (core/PHYSICS-SPIKE.md).
//
//   jolt_spike info   <craft>...                       craft model: parts, bodies, joints, sub-joints, autostruts, mass
//   jolt_spike flight <craft> [--model M] [options]    one scenario with per-phase timing
//   jolt_spike bench  <name> [options]                 the measurement sets of PHYSICS-SPIKE.md (see bench.cpp)
// <craft> is a name (PhysProf-S, -M2-AS, -L2-AS, ... from analysis/physics/craft; PhysProf-XL* generated in the build tree) or a path.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "bench.hpp"
#include "craft_model.hpp"
#include "flight.hpp"
#include "jolt_world.hpp"
#include "ksp_context.hpp"

using namespace spike;

namespace {

void PrintInfo(const CraftModel& m) {
    int rigid = 0, three = 0;
    for (int j = 0; j < m.attachJointCount; j++) {
        rigid += m.joints[j].rigid ? 1 : 0;
        three += m.joints[j].internalJoints == 3 ? 1 : 0;
    }
    int engines = 0, decouplers = 0;
    for (const PartSpec& s : m.parts) {
        engines += s.isEngine ? 1 : 0;
        decouplers += s.isDecoupler ? 1 : 0;
    }
    std::printf("%-18s parts %4zu  physical %4d  attach joints %4d (3-sub %3d, rigid %d)  autostruts %4d  sub-joints %5d  "
                "engines %3d  decouplers %3d  mass %.3f t\n",
                m.name.c_str(), m.parts.size(), m.physicalCount, m.attachJointCount, three, rigid, m.autostrutCount, m.SubJointCount(),
                engines, decouplers, m.totalMass);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: jolt_spike info|flight|bench ...\n");
        return 2;
    }
    try {
        JoltInit();
        const std::string cmd = argv[1];
        Args args(argc - 2, argv + 2);
        KspContext ctx(cmd != "info");
        int rc = 0;
        if (cmd == "info") {
            for (const std::string& c : args.positional) {
                CraftModel m = BuildCraftModel(ctx, CraftPath(c), c);
                PrintInfo(m);
            }
        } else if (cmd == "flight") {
            if (args.positional.empty()) throw std::runtime_error("flight: craft name missing");
            CraftModel m = BuildCraftModel(ctx, CraftPath(args.positional[0]), args.positional[0]);
            PrintInfo(m);
            FlightConfig cfg = ConfigFromArgs(args);
            FlightResult r = RunFlight(ctx, m, cfg);
            PrintResultHeader();
            PrintResult(r, cfg);
            if (args.Has("samples"))
                for (const Sample& s : r.samples)
                    std::printf("  t %7.2f  alt %10.3f  v %9.3f  jointErr %.5f\n", s.t, s.altitude, s.speed, s.jointError);
        } else if (cmd == "bench") {
            if (args.positional.empty()) throw std::runtime_error("bench: name missing");
            rc = RunBench(ctx, args.positional[0], args);
        } else {
            throw std::runtime_error("unknown command " + cmd);
        }
        std::fflush(stdout);
        // the Vessel / Part objects are torn down with the context; Jolt's factory last
        return rc;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
