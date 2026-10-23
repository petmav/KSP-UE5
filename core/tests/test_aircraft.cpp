// WP-21b: stock aircraft in the headless flight bubble: the stock SPH crafts through the craft path (upgrade pipeline, ShipConstruct.LoadShip,
// AssembleVesselFromCraft), control surfaces in the module slot, the gear on the real KSC runway.
//
// The crafts are KSP's own (harness/inputs/Ships/SPH). No capture covers an aircraft flight: the scenario values are analytic / reference-gated
// (labelled where they are asserted).
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/aero/lifting_surface.hpp"
#include "vessel_wheels_fixture.hpp"

using namespace ksp;
using namespace ksp::physics;

namespace {

struct LoadResult {
    bool ok = false;
    std::string error;
    size_t partNodes = 0, parts = 0;
    size_t controlSurfaces = 0, liftingSurfaces = 0, wheels = 0, engines = 0;
};

LoadResult TryLoad(wp20b::World& w, const std::string& path) {
    LoadResult r;
    try {
        ConfigNode::NodePtr root = ConfigNode::Load(path);
        if (!root) {
            r.error = "unreadable";
            return r;
        }
        const std::string* sn = root->GetValuePtr("ship");
        ConfigNode::NodePtr up;
        KSPUpgradePipeline::Process(root, sn ? *sn : std::string(), SaveUpgradePipeline::LoadContext::Craft, [&](const ConfigNode::NodePtr& n) { up = n; },
                                    [&](KSPUpgradePipeline::UpgradeFailOption, const ConfigNode::NodePtr&) {});
        if (!up) {
            r.error = "upgrade pipeline failed";
            return r;
        }
        r.partNodes = up->GetNodes("PART").size();
        std::unique_ptr<Vessel> v = w.Assemble(*up, path);
        r.parts = v->parts.size();
        for (Part* p : v->parts)
            for (size_t i = 0; i < p->Modules().Count(); ++i) {
                PartModule* m = p->Modules().GetModule(i);
                if (dynamic_cast<ModuleControlSurface*>(m)) ++r.controlSurfaces;
                else if (dynamic_cast<ModuleLiftingSurface*>(m)) ++r.liftingSurfaces;
                else if (dynamic_cast<ModuleEngines*>(m)) ++r.engines;
                else if (dynamic_cast<ModuleWheelBase*>(m)) ++r.wheels;
            }
        r.ok = true;
    } catch (const std::exception& e) {
        r.error = e.what();
    }
    return r;
}

}  // namespace

TEST_CASE("WP-21b the stock SPH crafts load through the craft path (control surfaces included)") {
    if (!wp20b::GameAvailable()) return;
    wp20b::World w;
    std::vector<std::string> names;
    for (const auto& e : std::filesystem::directory_iterator(std::string(KSPCORE_HARNESS_DIR) + "/inputs/Ships/SPH"))
        if (e.path().extension() == ".craft") names.push_back(e.path().stem().string());
    std::sort(names.begin(), names.end());
    size_t loaded = 0;
    for (const std::string& n : names) {
        const LoadResult r = TryLoad(w, std::string(KSPCORE_HARNESS_DIR) + "/inputs/Ships/SPH/" + n + ".craft");
        std::printf("| %s | %s | %zu | %zu | %zu | %zu | %zu | %zu | %s |\n", n.c_str(), r.ok ? "loads" : "FAILS", r.partNodes, r.parts, r.controlSurfaces,
                    r.liftingSurfaces, r.engines, r.wheels, r.error.c_str());
        loaded += r.ok ? 1 : 0;
    }
    CHECK(loaded > 0);
}
