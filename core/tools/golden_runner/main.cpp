// kspcore golden runner: regenerates the harness datasets with kspcore and writes candidate files in the ksp-golden/1
// schema, ready for harness/compare/compare.py.
//
//   golden_runner <dataset|all> --harness <harness dir> --out <candidate dir> [--gamedata <GameData dir>]
//   datasets: whatever the linked ds_*.cpp files register (math_shim, curves_eval, confignode_roundtrip, gamedata)
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>

#include "game_root.hpp"
#include "datasets.hpp"

int WriteCandidate(const RunnerOptions& opt, const golden::File& g, const std::string& relPath) {
    const std::string path = opt.out + "/" + relPath;
    const size_t slash = path.find_last_of('/');
    golden::MakeDirs(path.substr(0, slash));
    if (!golden::WriteTextFile(path, g.ToJson())) {
        std::cerr << "cannot write " << path << "\n";
        return 2;
    }
    if (opt.verbose) std::cout << "wrote " << path << "\n";
    return 0;
}

namespace {
std::vector<DatasetEntry>& Registry() {
    static std::vector<DatasetEntry> r;
    return r;
}
}  // namespace

DatasetRegistrar::DatasetRegistrar(const char* name, int order, DatasetFn fn) {
    auto& r = Registry();
    r.push_back({name, order, fn});
    std::sort(r.begin(), r.end(), [](const DatasetEntry& a, const DatasetEntry& b) { return a.order < b.order; });
}

const std::vector<DatasetEntry>& Datasets() { return Registry(); }

static int Usage() {
    std::cerr << "usage: golden_runner <dataset|all> --harness <dir> --out <dir> [-v]\n  datasets:";
    for (const auto& d : Datasets()) std::cerr << " " << d.name;
    std::cerr << "\n";
    return 64;
}

int main(int argc, char** argv) {
    if (argc < 2) return Usage();
    std::string dataset = argv[1];
    RunnerOptions opt;
    for (int i = 2; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--harness") && i + 1 < argc) opt.harness = argv[++i];
        else if (!std::strcmp(argv[i], "--out") && i + 1 < argc) opt.out = argv[++i];
        else if (!std::strcmp(argv[i], "--gamedata") && i + 1 < argc) opt.gamedata = argv[++i];
        else if (!std::strcmp(argv[i], "-v")) opt.verbose = true;
        else return Usage();
    }
    if (opt.harness.empty() || opt.out.empty()) return Usage();
    if (opt.gamedata.empty()) opt.gamedata = ksp::testing::GameRoot() + "/GameData";  // mirror, or env KSP_GAMEDATA_ROOT
    for (auto* s : {&opt.harness, &opt.out, &opt.gamedata})
        for (char& c : *s)
            if (c == '\\') c = '/';

    auto run = [&](const std::string& ds) -> int {
        const auto t0 = std::chrono::steady_clock::now();
        int rc;
        try {
            const DatasetEntry* e = nullptr;
            for (const auto& d : Datasets())
                if (ds == d.name) e = &d;
            if (!e) return Usage();
            rc = e->fn(opt);
        } catch (const std::exception& e) {
            std::cerr << ds << ": exception: " << e.what() << "\n";
            rc = 1;
        }
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        std::cout << ds << ": rc=" << rc << " (" << ms << " ms)\n";
        return rc;
    };
    if (dataset == "all") {
        int worst = 0;
        for (const auto& d : Datasets()) {
            const int rc = run(d.name);
            if (rc) worst = rc;
        }
        return worst;
    }
    return run(dataset);
}
