// confignode_roundtrip candidate: mirrors harness/src/KspGolden/Generators/ConfigNodeGen.cs, plus the byte-exact and
// tree-exact checks that compare.py cannot do (golden saved/<id>.cfg and trees/<id>.json are not table files).
#include <iostream>
#include <string>
#include <vector>

#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/text.hpp"
#include "nlohmann/json.hpp"
#include "confignode_tree.hpp"

using namespace ksp;
using ojson = nlohmann::ordered_json;

namespace {

const char* kCraft[] = {
    "Ships/VAB/Kerbal X.craft",   "Ships/VAB/Jumping Flea.craft", "Ships/VAB/Kerbal 1.craft", "Ships/VAB/Kerbal 2.craft",
    "Ships/VAB/Learstar A1.craft", "Ships/VAB/Ion-Powered Space Probe.craft", "Ships/VAB/Dynawing.craft",
    "Ships/VAB/ComSat Lx.craft",  "Ships/SPH/Aeris 3A.craft",     "Ships/SPH/Aeris 4A.craft", "Ships/SPH/Gull.craft",
    "Ships/SPH/Stearwing A300.craft", "Ships/SPH/Bug-E Buggy.craft", "Ships/SPH/Mallard.craft",
};

const char* kCfgs[] = {
    "Physics.cfg",
    "PartDatabase.cfg",
    "GameData/Squad/Resources/ResourcesGeneric.cfg",
    "GameData/Squad/Resources/TechTree.cfg",
    "GameData/Squad/Parts/Command/mk1pod_v2/mk1Pod_v2.cfg",
    "GameData/Squad/Parts/FuelTank/Size1_Tanks/fuelTankT400.cfg",
    "GameData/Squad/Parts/Engine/liquidEngineLV-T45_v2/liquidEngineLV-T45_v2.cfg",
    "GameData/Squad/Parts/Engine/liquidEngineMainsail_v2/liquidEngineMainsail_v2.cfg",
    "GameData/Squad/Parts/Engine/rapierEngine/rapierEngine.cfg",
    "GameData/Squad/Parts/Aero/wings/connector1.cfg",
    "GameData/Squad/Parts/Engine/jetEngines/jetEngineBasic.cfg",
    "GameData/Squad/Parts/Utility/parachuteMk1/parachuteMk1.cfg",
};

// ConfigNodeGen.cs Synthetic ids (the exact input bytes are read from golden/confignode_roundtrip/synthetic_inputs/)
const char* kSynthetic[] = {"basic_crlf",       "basic_lf",          "cr_only",           "utf8_bom",
                            "comments",         "braces_same_line",  "brace_after_value", "multiple_equals",
                            "empty_name_and_value", "whitespace_and_tabs", "bare_token_dropped", "node_id",
                            "duplicate_keys",   "unbalanced_close",  "unterminated_node", "anonymous_node",
                            "unicode",          "numbers",           "nested_deep",       "only_comment",
                            "empty_file"};

std::string FileStem(const std::string& p) {
    const size_t slash = p.find_last_of('/');
    std::string f = slash == std::string::npos ? p : p.substr(slash + 1);
    const size_t dot = f.find_last_of('.');
    return dot == std::string::npos ? f : f.substr(0, dot);
}

// ConfigNodeGen.cs San: letters, digits, '-' and '_' kept, everything else '_'
std::string San(const std::string& s) {
    std::string o;
    for (unsigned char c : s) o.push_back((std::isalnum(c) || c == '-' || c == '_') ? static_cast<char>(c) : '_');
    return o;
}

}  // namespace

static void Count(const ConfigNode& n, int64_t& v, int64_t& nn) {
    v += n.values().Count();
    nn += n.nodes().Count();
    for (const auto& c : n.nodes()) Count(*c, v, nn);
}

static int RunConfigNodeRoundtrip(const RunnerOptions& opt) {
    const std::string goldenDir = opt.harness + "/golden/confignode_roundtrip";
    const std::string outDir = opt.out + "/confignode_roundtrip";
    golden::MakeDirs(outDir + "/saved");
    golden::MakeDirs(outDir + "/trees");

    struct Input {
        std::string id, path, source;
        bool synthetic;
    };
    std::vector<Input> inputs;
    for (const char* c : kCraft) inputs.push_back({"craft_" + San(FileStem(c)), opt.harness + "/inputs/" + c, c, false});
    for (const char* c : kCfgs) inputs.push_back({"cfg_" + San(FileStem(c)), opt.harness + "/inputs/" + c, c, false});
    for (const char* s : kSynthetic)
        inputs.push_back({std::string("syn_") + s, goldenDir + "/synthetic_inputs/" + s + ".cfg",
                          std::string("synthetic_inputs/") + s + ".cfg", true});

    golden::File g;
    g.dataset = "confignode_roundtrip";
    g.description = "kspcore candidate for confignode_roundtrip.";
    golden::Table tc("corpus", "One row per input.");
    tc.Col("id", golden::ColType::Str).Col("source", golden::ColType::Str).Col("input_sha256", golden::ColType::Str)
        .Col("input_bytes", golden::ColType::I64).Col("loaded", golden::ColType::Bool)
        .Col("root_values", golden::ColType::I64).Col("root_nodes", golden::ColType::I64)
        .Col("total_values", golden::ColType::I64).Col("total_nodes", golden::ColType::I64)
        .Col("save_sha256", golden::ColType::Str).Col("save_bytes", golden::ColType::I64)
        .Col("tostring_sha256", golden::ColType::Str).Col("parse_tree_equals_load_tree", golden::ColType::Bool)
        .Col("resave_identical", golden::ColType::Bool);
    tc.key = {"id"};
    golden::Table ts("save_text", "Exact ConfigNode.Save output for the synthetic cases.");
    ts.Col("id", golden::ColType::Str).Col("save_text", golden::ColType::Str).Col("tostring_text", golden::ColType::Str);
    ts.key = {"id"};

    int savedMismatch = 0, treeMismatch = 0, savedMatch = 0, treeMatch = 0, missingInputs = 0;
    for (const Input& in : inputs) {
        auto bytes = dotnet::ReadAllBytes(in.path);
        if (!bytes) {
            std::cerr << "confignode: missing input " << in.source << "\n";
            ++missingInputs;
            continue;
        }
        auto node = ConfigNode::Load(in.path);
        std::optional<std::string> saved, tostr;
        bool parseEq = false, resaveSame = false;
        int64_t rv = 0, rn = 0, tv = 0, tn = 0;
        const std::string savePath = outDir + "/saved/" + in.id + ".cfg";
        if (node) {
            saved = node->SaveToString();
            golden::WriteTextFile(savePath, *saved);
            tostr = node->ToString();
            rv = node->values().Count();
            rn = node->nodes().Count();
            Count(*node, tv, tn);
            auto parsed = ConfigNode::Parse(*dotnet::ReadAllText(in.path));
            const ojson tree = TreeJson(*node);
            parseEq = TreeJson(*parsed) == tree;
            auto again = ConfigNode::Load(savePath);
            if (again) resaveSame = again->SaveToString() == *saved;
            golden::WriteTextFile(outDir + "/trees/" + in.id + ".json", tree.dump(1) + "\n");

            // byte-exact Save and tree-exact parse against the golden files
            auto gs = dotnet::ReadAllBytes(goldenDir + "/saved/" + in.id + ".cfg");
            if (gs && *gs == *saved) ++savedMatch;
            else {
                ++savedMismatch;
                std::cerr << "confignode: SAVE MISMATCH " << in.id << (gs ? "" : " (no golden file)") << "\n";
            }
            auto gt = dotnet::ReadAllText(goldenDir + "/trees/" + in.id + ".json");
            if (gt && ojson::parse(*gt) == tree) ++treeMatch;
            else {
                ++treeMismatch;
                std::cerr << "confignode: TREE MISMATCH " << in.id << (gt ? "" : " (no golden file)") << "\n";
            }
        } else {
            // golden has no saved/tree file for inputs that did not load
            if (dotnet::ReadAllBytes(goldenDir + "/saved/" + in.id + ".cfg")) {
                ++savedMismatch;
                std::cerr << "confignode: golden loaded " << in.id << " but kspcore returned null\n";
            }
        }
        tc.Add({in.id, in.source, Sha256Hex(*bytes), static_cast<int64_t>(bytes->size()), node != nullptr, rv, rn, tv, tn,
                saved ? Sha256Hex(*saved) : std::string(), saved ? static_cast<int64_t>(saved->size()) : int64_t(0),
                tostr ? Sha256Hex(*tostr) : std::string(), parseEq, resaveSame});
        if (in.synthetic)
            ts.Add({in.id, saved ? golden::Cell(*saved) : golden::Cell(nullptr), tostr ? golden::Cell(*tostr) : golden::Cell(nullptr)});
    }
    g.tables = {std::move(tc), std::move(ts)};
    std::cout << "confignode_roundtrip: saved files byte-identical " << savedMatch << "/" << (savedMatch + savedMismatch)
              << ", trees identical " << treeMatch << "/" << (treeMatch + treeMismatch) << ", missing inputs "
              << missingInputs << "\n";
    const int rc = WriteCandidate(opt, g, "confignode_roundtrip/confignode_roundtrip.json");
    if (rc) return rc;
    return (savedMismatch || treeMismatch || missingInputs) ? 4 : 0;
}

KSP_GOLDEN_DATASET("confignode_roundtrip", 30, RunConfigNodeRoundtrip);
