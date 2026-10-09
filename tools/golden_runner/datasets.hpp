#pragma once

#include <string>
#include <vector>

#include "golden_json.hpp"

struct RunnerOptions {
    std::string harness;  // .../harness
    std::string out;      // candidate root directory (dataset/<file>.json is written below it)
    // KSP GameData directory read by the WP-05 part_database dataset (--gamedata). Default:
    // <harness>/../decompiled/KSP-dev/GameData (the dev copy; read-only).
    std::string gamedata;
    bool verbose = false;
};

// Write <out>/<relPath> and return 0 (non-zero on I/O failure).
int WriteCandidate(const RunnerOptions& opt, const golden::File& g, const std::string& relPath);

// Dataset registry: each ds_*.cpp registers itself with KSP_GOLDEN_DATASET(name, order, function). Adding a work
// package's golden gate = add a ds_<name>.cpp here and the dataset name to KSP_GOLDEN_DATASETS in tests/CMakeLists.txt.
using DatasetFn = int (*)(const RunnerOptions&);
struct DatasetEntry {
    const char* name;
    int order;
    DatasetFn fn;
};
const std::vector<DatasetEntry>& Datasets();
struct DatasetRegistrar {
    DatasetRegistrar(const char* name, int order, DatasetFn fn);
};
#define KSP_GOLDEN_DATASET(name, order, fn) static const DatasetRegistrar ksp_golden_registrar_##fn(name, order, fn)

std::string Sha256Hex(const std::string& bytes);
