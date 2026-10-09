// curves_eval candidate: mirrors harness/src/KspGolden/Generators/FloatCurveGen.cs.
// Curve inputs are rebuilt independently of the golden output:
//   PhysicsGlobals.* / LiftingSurface.*  : harness/inputs/Physics.cfg through kspcore ConfigNode + FloatCurve.Load
//                                          (PhysicsGlobals.LoadDatabase / LiftingSurfaceCurve.Load, PhysicsGlobals.cs)
//   Body.*                               : harness/golden/serialized/celestial_bodies.serialized.json (Unity-serialized
//                                          keyframes, wrap modes and _minTime/_maxTime, as Unity deserializes them)
//   cfg:*                                : every node with `key` values in harness/inputs/GameData/Squad/Parts/**.cfg
//   synthetic.*                          : the generator's synthetic key sets
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/curves/float_curve.hpp"
#include "ksp/dotnet/text.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;
namespace fs = std::filesystem;

namespace {

struct CurveSource {
    std::string id, origin, path;
    std::shared_ptr<FloatCurve> curve;
    bool hasNode = false;
    std::vector<std::string> nodeKeys;  // node.GetValues("key")
};

// C# float.CompareTo ordering (NaN first, -0 == +0) for SortedSet<float>.
struct FloatCompare {
    bool operator()(float a, float b) const {
        if (std::isnan(a)) return !std::isnan(b);
        if (std::isnan(b)) return false;
        return a < b;
    }
};

float BitsToFloat(int32_t b) {
    float f;
    std::memcpy(&f, &b, sizeof f);
    return f;
}
int32_t FloatToBits(float f) {
    int32_t b;
    std::memcpy(&b, &f, sizeof b);
    return b;
}

// FloatCurveGen.cs NextUp / NextDown
float NextUp(float f) {
    if (std::isnan(f) || (std::isinf(f) && f > 0)) return f;
    if (f == 0) return 1.401298E-45f;  // float.Epsilon
    int32_t b = FloatToBits(f);
    b += f > 0 ? 1 : -1;
    return BitsToFloat(b);
}
float NextDown(float f) { return -NextUp(-f); }

// FloatCurveGen.cs SampleTimes
std::vector<float> SampleTimes(const std::vector<Keyframe>& keys) {
    std::set<float, FloatCompare> set;
    if (keys.empty()) {
        for (float t : {-1.0f, 0.0f, 1.0f}) set.insert(t);
        return {set.begin(), set.end()};
    }
    const float tmin = keys.front().time, tmax = keys.back().time;
    float span = tmax - tmin;
    if (span <= 0) span = std::max(1.0f, std::fabs(tmin));
    for (const Keyframe& k : keys) {
        set.insert(k.time);
        set.insert(NextDown(k.time));
        set.insert(NextUp(k.time));
    }
    for (size_t i = 0; i + 1 < keys.size(); ++i) {
        const float a = keys[i].time, b = keys[i + 1].time;
        set.insert(a + (b - a) * 0.5f);
        set.insert(a + (b - a) * 0.25f);
        set.insert(a + (b - a) * 0.75f);
    }
    constexpr int N = 64;
    const float lo = tmin - 0.1f * span, hi = tmax + 0.1f * span;
    for (int i = 0; i <= N; ++i) set.insert(lo + (hi - lo) * static_cast<float>(i) / static_cast<float>(N));
    std::vector<float> out;
    for (float f : set)
        if (!std::isnan(f) && !std::isinf(f)) out.push_back(f);
    return out;
}

// FloatCurveGen.cs Walk: label = name[+"[name value]"][+"#k"], path joined with '/'
void Walk(const ConfigNode& n, const std::string& path, std::vector<std::pair<const ConfigNode*, std::string>>& out) {
    std::map<std::string, int> counts;
    for (const auto& cp : n.nodes()) {
        const ConfigNode& c = *cp;
        std::string label = c.name;
        const std::string* nameVal = c.GetValuePtr("name");
        if (nameVal && !nameVal->empty()) label += "[" + *nameVal + "]";
        const int k = counts[label]++;
        if (k > 0) label += "#" + std::to_string(k + 1);
        const std::string p = path.empty() ? label : path + "/" + label;
        out.emplace_back(&c, p);
        Walk(c, p, out);
    }
}

std::string U8(const fs::path& p) {
    const auto s = p.u8string();
    return std::string(reinterpret_cast<const char*>(s.data()), s.size());
}

// C# string.CompareOrdinal on UTF-16; equal to byte order for the ASCII paths used here.
bool OrdinalLess(const std::string& a, const std::string& b) { return a < b; }

void PhysicsGlobalsCurves(const std::string& inputs, std::vector<CurveSource>& out) {
    auto cfg = ConfigNode::Load(inputs + "/Physics.cfg");
    if (!cfg) throw std::runtime_error("cannot load Physics.cfg");
    const std::pair<const char*, const char*> drag[] = {
        {"dragCurveTip", "DRAG_TIP"},           {"dragCurveSurface", "DRAG_SURFACE"},
        {"dragCurveTail", "DRAG_TAIL"},         {"dragCurveMultiplier", "DRAG_MULTIPLIER"},
        {"dragCurveCd", "DRAG_CD"},             {"dragCurveCdPower", "DRAG_CD_POWER"},
        {"dragCurvePseudoReynolds", "DRAG_PSEUDOREYNOLDS"}};
    // PhysicsGlobals.LoadDatabase (PhysicsGlobals.cs:3382-3416)
    for (const auto& [field, nodeName] : drag) {
        auto fc = std::make_shared<FloatCurve>();
        if (ConfigNode* n = cfg->GetNode(nodeName)) fc->Load(*n);
        out.push_back({std::string("PhysicsGlobals.") + field, "PhysicsGlobals (prefab, then Physics.cfg via LoadDatabase)",
                       field, fc, false, {}});
    }
    // LoadLiftingSurfaceCurves (PhysicsGlobals.cs:3758) + LiftingSurfaceCurve.Load (PhysicsGlobals.cs:27)
    struct LSC {
        std::shared_ptr<FloatCurve> lift = std::make_shared<FloatCurve>(), liftMach = std::make_shared<FloatCurve>(),
                                    dragC = std::make_shared<FloatCurve>(), dragMach = std::make_shared<FloatCurve>();
    };
    std::map<std::string, LSC> sets;  // ordered by key (generator: OrderBy(k.Key, StringComparer.Ordinal))
    if (ConfigNode* lsc = cfg->GetNode("LIFTING_SURFACE_CURVES")) {
        for (ConfigNode* ls : lsc->GetNodes("LIFTING_SURFACE")) {
            LSC c;
            std::string name;
            if (auto v = ls->GetValue("name")) name = *v;
            if (ConfigNode* n = ls->GetNode("lift")) c.lift->Load(*n);
            if (ConfigNode* n = ls->GetNode("liftMach")) c.liftMach->Load(*n);
            if (ConfigNode* n = ls->GetNode("drag")) c.dragC->Load(*n);
            if (ConfigNode* n = ls->GetNode("dragMach")) c.dragMach->Load(*n);
            sets[name] = c;
        }
    }
    for (auto& [name, c] : sets) {
        const std::string o = "PhysicsGlobals.LiftingSurfaceCurves";
        out.push_back({"LiftingSurface." + name + ".lift", o, name + "/lift", c.lift, false, {}});
        out.push_back({"LiftingSurface." + name + ".liftMach", o, name + "/liftMach", c.liftMach, false, {}});
        out.push_back({"LiftingSurface." + name + ".drag", o, name + "/drag", c.dragC, false, {}});
        out.push_back({"LiftingSurface." + name + ".dragMach", o, name + "/dragMach", c.dragMach, false, {}});
    }
}

void BodyCurves(const std::string& serializedPath, std::vector<CurveSource>& out) {
    auto text = dotnet::ReadAllText(serializedPath);
    if (!text) throw std::runtime_error("cannot read " + serializedPath);
    const auto j = nlohmann::ordered_json::parse(*text);
    std::vector<const nlohmann::ordered_json*> bodies;
    for (const auto& b : j.at("bodies")) bodies.push_back(&b);
    std::stable_sort(bodies.begin(), bodies.end(), [](auto* a, auto* b) {
        return a->at("spawnOrder").template get<int>() < b->at("spawnOrder").template get<int>();
    });
    for (const auto* b : bodies) {
        const auto& cb = b->at("celestialBody");
        const std::string bodyName = cb.at("bodyName").get<std::string>();
        for (const auto& [field, val] : cb.items()) {
            if (!val.is_object() || !val.contains("fCurve")) continue;
            const auto& fcj = val.at("fCurve");
            std::vector<Keyframe> keys;
            for (const auto& k : fcj.at("m_Curve")) {
                Keyframe kf;
                kf.time = static_cast<float>(k.at("time").get<double>());
                kf.value = static_cast<float>(k.at("value").get<double>());
                kf.inTangent = static_cast<float>(k.at("inSlope").get<double>());
                kf.outTangent = static_cast<float>(k.at("outSlope").get<double>());
                kf.weightedMode = static_cast<WeightedMode>(k.value("weightedMode", 0));
                kf.inWeight = static_cast<float>(k.value("inWeight", 0.0));
                kf.outWeight = static_cast<float>(k.value("outWeight", 0.0));
                keys.push_back(kf);
            }
            if (keys.empty()) continue;  // generator: if (fc.Curve.length == 0) continue
            auto fc = std::make_shared<FloatCurve>();
            fc->SetCurve(AnimationCurve::CreateFromSerialized(keys, fcj.value("m_PreInfinity", 2), fcj.value("m_PostInfinity", 2)));
            fc->SetSerializedRange(static_cast<float>(val.at("_minTime").get<double>()),
                                   static_cast<float>(val.at("_maxTime").get<double>()));
            out.push_back({"Body." + bodyName + "." + field, "CelestialBody (sharedassets9 prefab, deserialized)", field, fc,
                           false, {}});
        }
    }
}

void PartCfgCurves(const std::string& inputs, std::vector<CurveSource>& out, std::vector<std::string>& errors) {
    const fs::path root = dotnet::PathFromUtf8(inputs) / "GameData" / "Squad" / "Parts";
    std::vector<std::string> files;  // full paths with '\' separators (C# Directory.GetFiles + OrderBy ordinal)
    for (const auto& e : fs::recursive_directory_iterator(root)) {
        if (!e.is_regular_file()) continue;
        const std::string ext = U8(e.path().extension());
        // "*.cfg": a 3-character extension pattern also matches longer extensions starting with ".cfg"
        if (ext.size() < 4 || !dotnet::EqualsOrdinalIgnoreCase(ext.substr(0, 4), ".cfg")) continue;
        std::string p = U8(e.path());
        std::replace(p.begin(), p.end(), '/', '\\');
        files.push_back(p);
    }
    std::sort(files.begin(), files.end(), OrdinalLess);
    std::string inputsBs = inputs;
    std::replace(inputsBs.begin(), inputsBs.end(), '/', '\\');
    std::set<std::string> seen;
    for (const std::string& file : files) {
        std::string rel = file.substr(inputsBs.size() + 1);
        std::replace(rel.begin(), rel.end(), '\\', '/');
        auto cn = ConfigNode::Load(file);
        if (!cn) {
            errors.push_back(rel + ": ConfigNode.Load returned null");
            continue;
        }
        std::vector<std::pair<const ConfigNode*, std::string>> nodes;
        Walk(*cn, "", nodes);
        for (const auto& [node, path] : nodes) {
            if (!node->HasValue("key")) continue;
            auto fc = std::make_shared<FloatCurve>();
            try {
                fc->Load(*node);
            } catch (const std::exception& e) {
                errors.push_back(rel + ":" + path + ": " + e.what());
                continue;
            }
            std::string id = "cfg:" + rel + ":" + path;
            if (!seen.insert(id).second) {
                int n = 2;
                while (!seen.insert(id + "#" + std::to_string(n)).second) ++n;
                id += "#" + std::to_string(n);
            }
            out.push_back({id, "part cfg", rel + ":" + path, fc, true, node->GetValues("key")});
        }
    }
}

void Synthetic(std::vector<CurveSource>& out) {
    struct Def {
        const char* id;
        std::vector<const char*> keys;
        const char* why;
    };
    const Def defs[] = {
        {"auto_tangents_sorted", {"0 0", "1 1", "2 4", "3 9"}, "2-value keys: Unity AddKeySmoothTangents recomputes neighbour slopes"},
        {"auto_tangents_unsorted", {"3 9", "0 0", "2 4", "1 1"}, "insertion order changes which neighbours get recalculated"},
        {"mixed_explicit_and_auto", {"0 1 0 0", "10 2", "20 0 0 0"}, "a 2-value key overwrites the explicit tangents of its neighbours"},
        {"duplicate_time", {"0 0 0 0", "1 1 0 0", "1 5 0 0", "2 2 0 0"}, "AddKey rejects a key whose time already exists"},
        {"single_key", {"5 3"}, "1 key: Evaluate returns its value everywhere"},
        {"empty", {}, "no keys: Evaluate returns 0"},
        {"stepped_infinite_tangent", {"0 0 0 Infinity", "1 1 Infinity 0", "2 3 0 0"}, "+Infinity out/in tangent -> constant segment"},
        {"negative_infinite_tangent", {"0 0 0 -Infinity", "1 1 0 0"}, "-Infinity is NOT treated as stepped"},
        {"tiny_dx", {"0 0 1 1", "0.00001 1 1 1", "1 2 0 0"}, "segment shorter than 1e-4: length clamped to 1e-4 in CalculateCacheData"},
        {"comma_semicolon_tab", {"0,1,0,0", "1;2;0;0", "2\t3\t0\t0"}, "FloatCurve.Load delimiters ' ' ',' ';' '\\t'"},
        {"three_and_five_values", {"0 1 2", "1 2 3 4 5", "2 0 0 0"}, "only exactly 4 values are read as tangents; 3 or 5 values -> Add(t,v)"},
        {"large_magnitude", {"0 101325 0 -12", "35000 0.5 -0.0001 -0.0001", "70000 0 0 0"}, "large values: float rounding in the cubic"},
        {"isp_like", {"0 320", "1 270", "9 0.001"}, "typical atmosphereCurve with auto tangents"},
    };
    for (const Def& d : defs) {
        ConfigNode node("CURVE");
        for (const char* k : d.keys) node.AddValue("key", k);
        auto fc = std::make_shared<FloatCurve>();
        fc->Load(node);
        out.push_back({std::string("synthetic.") + d.id, std::string("synthetic: ") + d.why, d.id, fc, true, node.GetValues("key")});
    }
    // FloatCurve(Keyframe[]) path
    auto kc = std::make_shared<FloatCurve>(std::vector<Keyframe>{Keyframe(0.0f, 1.0f, 0.0f, 0.0f),
                                                                 Keyframe(0.85f, 1.19f, 0.6960422f, 0.6960422f),
                                                                 Keyframe(1.1f, 2.83f, 0.730473f, 0.730473f),
                                                                 Keyframe(5.0f, 4.0f, 0.0f, 0.0f)});
    out.push_back({"synthetic.keyframe_ctor_dragTip", "synthetic: new FloatCurve(Keyframe[]) -> Add(t,v,in,out) per key",
                   "keyframe_ctor", kc, false, {}});
}

}  // namespace

static int RunCurvesEval(const RunnerOptions& opt) {
    const std::string inputs = opt.harness + "/inputs";
    std::vector<CurveSource> sources;
    std::vector<std::string> errors;
    PhysicsGlobalsCurves(inputs, sources);
    BodyCurves(opt.harness + "/golden/serialized/celestial_bodies.serialized.json", sources);
    PartCfgCurves(inputs, sources, errors);
    Synthetic(sources);
    for (const auto& e : errors) std::cerr << "curves_eval: cfg error: " << e << "\n";

    golden::File g;
    g.dataset = "curves_eval";
    g.description = "kspcore candidate for curves_eval (FloatCurve/AnimationCurve port).";
    golden::Table tc("curves", "One row per curve.");
    tc.Col("curve_id", golden::ColType::Str).Col("origin", golden::ColType::Str).Col("path", golden::ColType::Str)
        .Col("key_count", golden::ColType::I64).Col("minTime", golden::ColType::F32).Col("maxTime", golden::ColType::F32)
        .Col("cfg_keys", golden::ColType::Str);
    tc.key = {"curve_id"};
    golden::Table tk("keys", "Keyframes after loading (FloatCurve.Curve.keys).");
    tk.Col("curve_id", golden::ColType::Str).Col("index", golden::ColType::I64).Col("time", golden::ColType::F32)
        .Col("value", golden::ColType::F32).Col("inTangent", golden::ColType::F32).Col("outTangent", golden::ColType::F32);
    tk.key = {"curve_id", "index"};
    golden::Table ts("samples", "FloatCurve.Evaluate(t).");
    ts.Col("curve_id", golden::ColType::Str).Col("i", golden::ColType::I64).Col("t", golden::ColType::F32)
        .Col("value", golden::ColType::F32);
    ts.key = {"curve_id", "i"};

    for (const CurveSource& s : sources) {
        const auto& keys = s.curve->Curve().keys();
        std::string raw;
        if (s.hasNode)
            for (size_t i = 0; i < s.nodeKeys.size(); ++i) raw += (i ? " | " : "") + s.nodeKeys[i];
        tc.Add({s.id, s.origin, s.path, static_cast<int64_t>(keys.size()), s.curve->minTime(), s.curve->maxTime(), raw});
        for (size_t i = 0; i < keys.size(); ++i)
            tk.Add({s.id, static_cast<int64_t>(i), keys[i].time, keys[i].value, keys[i].inTangent, keys[i].outTangent});
        const std::vector<float> times = SampleTimes(keys);
        for (size_t i = 0; i < times.size(); ++i) ts.Add({s.id, static_cast<int64_t>(i), times[i], s.curve->Evaluate(times[i])});
    }
    g.tables = {std::move(tc), std::move(tk), std::move(ts)};
    std::cout << "curves_eval: " << sources.size() << " curves, cfg errors " << errors.size() << "\n";
    const int rc = WriteCandidate(opt, g, "curves_eval/curves_eval.json");
    return rc != 0 ? rc : (errors.empty() ? 0 : 3);
}

KSP_GOLDEN_DATASET("curves_eval", 20, RunCurvesEval);
