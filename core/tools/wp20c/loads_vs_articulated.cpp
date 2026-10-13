// WP-20c bench (not a test; built in the spike tree, Release): the PRODUCTION binding's structural loads (VesselPhysics::JointLoads: the
// tree solve on tree craft, the graph solve where autostruts share the load) against the spike's ARTICULATED model, the PhysX-equivalent
// reference of core/PHYSICS-SPIKE.md 2.8 (one Jolt body per part, every PartJoint sub-joint a SixDOF constraint with KSP's drives, 50
// velocity steps; loads = the sub-joint impulses / dt). The scenario is the spike's ascent (pad, 2 s settle, full thrust), 8 s; per attach
// joint the force and torque magnitudes averaged over the last 50 ticks, joints carrying >= 5 % of the largest load compared (the spike's
// `loads` bench). The gate of PHYSICS-SPIKE.md 7 (joint_loads): tree craft S / M2 within 1 % of the articulated impulses.
//
//   wp20c_loads [crafts...] [--time s]          (default PhysProf-S PhysProf-M2 PhysProf-M2-AS PhysProf-L2-AS, 8 s)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "bench.hpp"
#include "craft_model.hpp"
#include "flight.hpp"
#include "jolt_world.hpp"
#include "ksp_context.hpp"
#include "vessel_physics_fixture.hpp"

namespace {

struct Avg {
    double force = 0.0, torque = 0.0;
    int n = 0;
};

double Pct(std::vector<double> v, double q) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    return v[std::min(v.size() - 1, static_cast<size_t>(q * static_cast<double>(v.size())))];
}

}  // namespace

int main(int argc, char** argv) {
    std::vector<std::string> crafts;
    double seconds = 8.0;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--time" && i + 1 < argc) seconds = std::stod(argv[++i]);
        else crafts.push_back(a);
    }
    if (crafts.empty()) crafts = {"PhysProf-S", "PhysProf-M2", "PhysProf-M2-AS", "PhysProf-L2-AS"};
    const int ticks = static_cast<int>(std::lround(seconds / 0.02));
    const int window = 50;
    // ---- the articulated reference (spike), every craft first: its context owns the game data while it runs
    std::vector<std::vector<std::pair<float, float>>> art(crafts.size());
    std::vector<std::vector<int>> artChild(crafts.size());
    spike::JoltInit();  // the spike registers Jolt itself; kspcore's PhysicsWorld does its own (reference counted) afterwards
    {
        spike::KspContext ctx;
        for (size_t c = 0; c < crafts.size(); ++c) {
            spike::CraftModel craft = spike::BuildCraftModel(ctx, spike::CraftPath(crafts[c]), crafts[c]);
            spike::FlightConfig cfg;
            cfg.model = "articulated";
            cfg.world.velocitySteps = 50;
            cfg.world.positionSteps = 2;
            cfg.ticks = ticks;
            cfg.loadWindow = window;
            const spike::FlightResult r = spike::RunFlight(ctx, craft, cfg);
            art[c] = r.jointLoads;
            for (int j = 0; j < craft.attachJointCount; ++j) {
                artChild[c].push_back(craft.joints[static_cast<size_t>(j)].child);
            }
            std::fprintf(stderr, "%s: articulated done (%d breaks)\n", crafts[c].c_str(), r.breaks);
        }
    }
    spike::JoltShutdown();
    // ---- the production binding on the same scenario
    std::printf("| craft | autostruts | production solve | joints compared | force: median / p90 / max rel. diff | torque: median / p90 | max sub-joint "
                "load / capacity (production) | load solve ms per tick |\n|---|---|---|---|---|---|---|---|\n");
    wp20b::World w(true);
    for (size_t c = 0; c < crafts.size(); ++c) {
        auto v = w.Craft(crafts[c]);
        wp20b::AscentConfig cfg;
        cfg.ticks = ticks;
        cfg.angularDrag = ksp::physics::AngularDragModel::PerPart;
        std::vector<Avg> prod;
        std::vector<int> prodChild;  // vessel part index of each joint's child
        float maxRatio = 0.0f;
        cfg.onTick = [&](int tick, ksp::physics::VesselPhysics& vp) {
            if (tick < ticks - window) return;
            const auto& loads = vp.JointLoads();
            if (prod.empty()) {
                prod.assign(loads.size(), Avg{});
                for (const auto& j : vp.Joints()) prodChild.push_back(vp.PartIndex(*j.child));
            }
            for (size_t j = 0; j < loads.size() && j < prod.size(); ++j) {
                if (!loads[j].valid || vp.Joints()[j].kind != ksp::physics::JointKind::Attach) continue;
                prod[j].force += loads[j].force.magnitude();
                prod[j].torque += loads[j].moment.magnitude();
                prod[j].n++;
                maxRatio = std::max(maxRatio, loads[j].ratio);
            }
        };
        const wp20b::AscentResult res = wp20b::RunAscent(w, *v, cfg);
        // match by the child part (the same craft through the same assembly: the same part order)
        std::vector<double> df, dt;
        double fMax = 0.0, tMax = 0.0;
        for (size_t j = 0; j < art[c].size(); ++j) {
            fMax = std::max(fMax, static_cast<double>(art[c][j].first));
            tMax = std::max(tMax, static_cast<double>(art[c][j].second));
        }
        size_t compared = 0;
        for (size_t ja = 0; ja < art[c].size(); ++ja) {
            const auto it = std::find(prodChild.begin(), prodChild.end(), artChild[c][ja]);
            if (it == prodChild.end()) continue;
            const Avg& p = prod[static_cast<size_t>(it - prodChild.begin())];
            if (p.n == 0 || art[c][ja].first < 0.0f) continue;
            const double pf = p.force / p.n, pt = p.torque / p.n, af = art[c][ja].first, at = art[c][ja].second;
            if (std::max(af, pf) >= 0.05 * fMax) {
                df.push_back(std::fabs(af - pf) / std::max(af, pf));
                ++compared;
            }
            if (std::max(at, pt) >= 0.05 * tMax) dt.push_back(std::fabs(at - pt) / std::max(at, pt));
        }
        int autostruts = 0;
        for (ksp::Part* p : v->parts) autostruts += p->fields.autoStrutMode != 0 ? 1 : 0;
        std::printf("| %s | %d | %s | %zu | %.4f / %.4f / %.4f | %.3f / %.3f | %.4f | %.4f |\n", crafts[c].c_str(), autostruts,
                    res.lastSolve == ksp::physics::LoadSolve::Graph ? "graph" : "tree", compared, Pct(df, 0.5), Pct(df, 0.9), Pct(df, 1.0), Pct(dt, 0.5),
                    Pct(dt, 0.9), static_cast<double>(maxRatio), res.tLoads);
        std::fflush(stdout);
    }
    return 0;
}
