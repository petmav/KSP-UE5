// WP-18 unit tests: ShipConstruct over live parts, ShipTemplate, ShipConstruction.
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "resource_fixture.hpp"
#include "ksp/ship/ship_construction.hpp"
#include "ksp/ship/ship_template.hpp"
#include "ksp/vessel/vessel_assembly.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::support;
namespace fs = std::filesystem;

namespace {

struct CraftFile {
    std::string label;
    fs::path path;
};

// every .craft of harness/inputs/Ships (VAB, SPH) and analysis/physics/craft
std::vector<CraftFile> AllCrafts() {
    std::vector<CraftFile> out;
    for (const char* dir : {"VAB", "SPH"}) {
        const fs::path d = fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / dir;
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(d))
            if (e.path().extension() == ".craft") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const auto& f : files) out.push_back({std::string(dir) + "/" + f.filename().string(), f});
    }
    const fs::path physDir = fs::path(KSPCORE_PROJECT_ROOT) / "analysis" / "physics" / "craft";
    if (fs::exists(physDir)) {
        std::vector<fs::path> files;
        for (const auto& e : fs::directory_iterator(physDir))
            if (e.path().extension() == ".craft") files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const auto& f : files) out.push_back({"physics/" + f.filename().string(), f});
    }
    return out;
}

// KSPUpgradePipeline.Process as CreateConstructFromTemplate runs it; nullptr when the pipeline fails
ConfigNode::NodePtr Upgraded(const fs::path& file) {
    ConfigNode::NodePtr root = ConfigNode::Load(file.string());
    if (!root) return nullptr;
    const std::string* sn = root->GetValuePtr("ship");
    ConfigNode::NodePtr up;
    KSPUpgradePipeline::Process(root, sn ? *sn : std::string(), SaveUpgradePipeline::LoadContext::Craft,
                                [&](const ConfigNode::NodePtr& n) { up = n; }, [&](KSPUpgradePipeline::UpgradeFailOption, const ConfigNode::NodePtr&) {});
    return up;
}

std::string FirstDiff(const std::string& a, const std::string& b) {
    std::istringstream sa(a), sb(b);
    std::string la, lb;
    int n = 0;
    while (true) {
        const bool ga = static_cast<bool>(std::getline(sa, la));
        const bool gb = static_cast<bool>(std::getline(sb, lb));
        n++;
        if (!ga && !gb) return "";
        if (!ga || !gb || la != lb) return "line " + std::to_string(n) + ": [" + (ga ? la : "<eof>") + "] vs [" + (gb ? lb : "<eof>") + "]";
    }
}

}  // namespace

TEST_CASE("WP-18 explore: per-key differences of live SaveShip vs the (upgraded) file") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    std::map<std::string, int> diffByKey, total;
    std::map<std::string, std::string> example;
    for (const CraftFile& cf : AllCrafts()) {
        ConfigNode::NodePtr up = Upgraded(cf.path);
        if (!up) continue;
        ShipConstruct ship;
        std::string err;
        if (!ship.LoadShip(*up, 0, true, err)) continue;
        if (!ship.InstantiateParts(true, err)) continue;
        ConfigNode::NodePtr live = ship.SaveShip();
        for (int i = 0; i < up->nodes().Count(); i++) {
            const ConfigNode& a = *up->nodes()[i];
            const ConfigNode& b = *live->nodes()[i];
            std::map<std::string, std::vector<std::string>> va, vb;
            for (int k = 0; k < a.values().Count(); k++) va[a.values()[k].name].push_back(a.values()[k].value);
            for (int k = 0; k < b.values().Count(); k++) vb[b.values()[k].name].push_back(b.values()[k].value);
            for (auto& [key, list] : va) {
                total[key]++;
                if (!vb.count(key) || vb[key] != list) {
                    diffByKey[key]++;
                    if (!example.count(key)) example[key] = cf.label + " part " + std::to_string(i) + ": [" + list[0] + "] vs [" + (vb.count(key) ? vb[key][0] : "<none>") + "]";
                }
            }
            for (auto& [key, list] : vb)
                if (!va.count(key)) { diffByKey[key + " (only live)"]++; if (!example.count(key + " (only live)")) example[key + " (only live)"] = cf.label + " " + std::to_string(i) + ": " + list[0]; }
            for (const char* key : {"modCost", "modMass"}) {
                if (va.count(key) && vb.count(key) && va[key] != vb[key]) {
                    float fa = 0, fb = 0;
                    dotnet::TryParseSingle(va[key][0], fa);
                    dotnet::TryParseSingle(vb[key][0], fb);
                    if (std::fabs(fa - fb) > 1e-6f) {
                        const std::string* pn = a.GetValuePtr("part");
                        MESSAGE("  " << key << " " << cf.label << " " << (pn ? *pn : "?") << " file " << va[key][0] << " live " << vb[key][0]);
                    }
                }
            }
            std::map<std::string, int> na, nb;
            for (int k = 0; k < a.nodes().Count(); k++) na[a.nodes()[k]->name]++;
            for (int k = 0; k < b.nodes().Count(); k++) nb[b.nodes()[k]->name]++;
            if (na != nb) { diffByKey["<node counts>"]++; if (diffByKey["<node counts>"] < 4) { std::string d; for (auto& [k2, n2] : na) d += k2 + ":" + std::to_string(n2) + " "; d += " | "; for (auto& [k2, n2] : nb) d += k2 + ":" + std::to_string(n2) + " "; MESSAGE("  nodes " << d); } if (!example.count("<node counts>")) example["<node counts>"] = cf.label + " part " + std::to_string(i); }
        }
    }
    for (auto& [k, n] : diffByKey) MESSAGE(k << ": " << n << " / " << total[k] << "  e.g. " << example[k]);
}
