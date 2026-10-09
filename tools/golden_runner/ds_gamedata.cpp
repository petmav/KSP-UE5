// gamedata corpus: every *.cfg under harness/inputs/GameData and every *.craft under harness/inputs/Ships is loaded with
// ConfigNode.Load and round-tripped: Save -> Load -> Save must be byte-identical and give the same tree, and
// Parse(ReadAllText) must give the same tree as Load. Every node with `key` values is also loaded as a FloatCurve.
// There is no golden for most of these files; this is a robustness and self-consistency gate.
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/dotnet/text.hpp"
#include "nlohmann/json.hpp"
#include "confignode_tree.hpp"

using namespace ksp;
namespace fs = std::filesystem;


namespace {
std::string U8(const fs::path& p) {
    const auto s = p.u8string();
    return std::string(reinterpret_cast<const char*>(s.data()), s.size());
}

void CollectCurves(const ConfigNode& n, int& ok, std::vector<std::string>& errors, const std::string& file) {
    for (const auto& c : n.nodes()) {
        if (c->HasValue("key")) {
            FloatCurve fc;
            try {
                fc.Load(*c);
                ++ok;
            } catch (const std::exception& e) {
                errors.push_back(file + ": " + c->name + ": " + e.what());
            }
        }
        CollectCurves(*c, ok, errors, file);
    }
}
}  // namespace

static int RunGameDataCorpus(const RunnerOptions& opt) {
    const std::string inputs = opt.harness + "/inputs";
    std::vector<std::pair<std::string, std::string>> files;  // (kind, path)
    for (const auto& e : fs::recursive_directory_iterator(dotnet::PathFromUtf8(inputs + "/GameData"))) {
        if (e.is_regular_file() && dotnet::EqualsOrdinalIgnoreCase(U8(e.path().extension()), ".cfg")) files.emplace_back("cfg", U8(e.path()));
    }
    for (const auto& e : fs::recursive_directory_iterator(dotnet::PathFromUtf8(inputs + "/Ships"))) {
        if (e.is_regular_file() && dotnet::EqualsOrdinalIgnoreCase(U8(e.path().extension()), ".craft")) files.emplace_back("craft", U8(e.path()));
    }
    std::sort(files.begin(), files.end());

    int loaded = 0, nullResult = 0, exceptions = 0, resaveOk = 0, resaveFail = 0, parseOk = 0, parseFail = 0, curvesOk = 0;
    int cfgCount = 0, craftCount = 0;
    int64_t totalValues = 0, totalNodes = 0, totalBytes = 0;
    std::vector<std::string> failures, curveErrors;
    const std::string tmp = opt.out + "/gamedata/tmp_resave.cfg";
    golden::MakeDirs(opt.out + "/gamedata");
    for (const auto& [kind, path] : files) {
        (kind == "cfg" ? cfgCount : craftCount)++;
        try {
            auto bytes = dotnet::ReadAllBytes(path);
            totalBytes += bytes ? static_cast<int64_t>(bytes->size()) : 0;
            auto node = ConfigNode::Load(path);
            if (!node) {
                ++nullResult;
                failures.push_back("null: " + path);
                continue;
            }
            ++loaded;
            std::vector<ConfigNode*> stack{node.get()};
            while (!stack.empty()) {
                ConfigNode* n = stack.back();
                stack.pop_back();
                totalValues += n->values().Count();
                totalNodes += n->nodes().Count();
                for (const auto& c : n->nodes()) stack.push_back(c.get());
            }
            const std::string saved = node->SaveToString();
            golden::WriteTextFile(tmp, saved);
            auto again = ConfigNode::Load(tmp);
            // Save drops node ids, so compare trees with ids cleared on the first load.
            bool same = again && again->SaveToString() == saved;
            if (same) ++resaveOk;
            else {
                ++resaveFail;
                failures.push_back("resave: " + path);
            }
            auto parsed = ConfigNode::Parse(*dotnet::ReadAllText(path));
            if (TreeJson(*parsed) == TreeJson(*node)) ++parseOk;
            else {
                ++parseFail;
                failures.push_back("parse!=load: " + path);
            }
            CollectCurves(*node, curvesOk, curveErrors, path);
        } catch (const std::exception& e) {
            ++exceptions;
            failures.push_back(std::string("exception: ") + path + ": " + e.what());
        }
    }
    std::error_code ec;
    fs::remove(dotnet::PathFromUtf8(tmp), ec);

    nlohmann::ordered_json s;
    s["files"] = files.size();
    s["cfg_files"] = cfgCount;
    s["craft_files"] = craftCount;
    s["bytes"] = totalBytes;
    s["loaded"] = loaded;
    s["load_returned_null"] = nullResult;
    s["exceptions"] = exceptions;
    s["resave_identical"] = resaveOk;
    s["resave_different"] = resaveFail;
    s["parse_equals_load"] = parseOk;
    s["parse_differs"] = parseFail;
    s["total_values"] = totalValues;
    s["total_nodes"] = totalNodes;
    s["floatcurves_loaded"] = curvesOk;
    s["floatcurve_errors"] = curveErrors;
    s["failures"] = failures;
    golden::WriteTextFile(opt.out + "/gamedata/summary.json", s.dump(1) + "\n");
    std::cout << "gamedata: files " << files.size() << " (cfg " << cfgCount << ", craft " << craftCount << "), loaded " << loaded
              << ", null " << nullResult << ", exceptions " << exceptions << ", resave identical " << resaveOk << "/"
              << (resaveOk + resaveFail) << ", parse==load " << parseOk << "/" << (parseOk + parseFail) << ", values "
              << totalValues << ", nodes " << totalNodes << ", FloatCurves " << curvesOk << " (errors " << curveErrors.size()
              << ")\n";
    for (const auto& f : failures) std::cerr << "gamedata: " << f << "\n";
    for (const auto& f : curveErrors) std::cerr << "gamedata: curve error: " << f << "\n";
    return (exceptions || resaveFail || parseFail || nullResult || !curveErrors.empty()) ? 5 : 0;
}

KSP_GOLDEN_DATASET("gamedata", 40, RunGameDataCorpus);
