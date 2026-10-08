// physics_constants candidate: mirrors harness/src/KspGolden/Generators/ThermalGen.cs. One row per PhysicsGlobals field
// (instance and static, sorted ordinal) after the in-game load sequence (CelestialEnv), the VesselRanges fields expanded,
// plus the five computed static properties. Inputs: harness/inputs/Physics.cfg (cfg key set for the `source` column),
// harness/golden/serialized/physicsglobals.serialized.json (prefab key set); values come from kspcore's PhysicsGlobals.
#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "celestial_env.hpp"
#include "datasets.hpp"
#include "golden_json.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/text.hpp"
#include "nlohmann/json.hpp"

using namespace ksp;

namespace {

int RunPhysicsConstants(const RunnerOptions& opt) {
    CelestialEnv& env = CelestialEnv::Get(opt);
    const PhysicsGlobals& pg = *env.physics;

    auto cfg = ConfigNode::Load(env.physicsCfgPath);
    if (!cfg) throw std::runtime_error("cannot load " + env.physicsCfgPath);
    std::set<std::string> cfgKeys;
    for (const auto& v : cfg->values()) cfgKeys.insert(v.name);
    const auto text = dotnet::ReadAllText(opt.harness + "/golden/serialized/physicsglobals.serialized.json");
    if (!text) throw std::runtime_error("cannot read physicsglobals.serialized.json");
    std::set<std::string> serKeys;
    const nlohmann::ordered_json ser = nlohmann::ordered_json::parse(*text);
    for (const auto& [k, v] : ser.at("PhysicsGlobals").items()) serKeys.insert(k);

    golden::File g;
    g.dataset = "physics_constants";
    g.description = "kspcore candidate for physics_constants (PhysicsGlobals level0 prefab + Physics.cfg LoadDatabase).";
    golden::Table t("physicsglobals", "One row per field (instance and static).");
    t.Col("field", golden::ColType::Str).Col("static", golden::ColType::Bool).Col("type", golden::ColType::Str);
    t.Col("value", golden::ColType::Str).Col("value_f64", golden::ColType::F64).Col("source", golden::ColType::Str);
    t.key = {"field"};

    std::vector<const PhysicsGlobals::FieldInfo*> fields;
    for (const auto& f : PhysicsGlobals::Fields()) fields.push_back(&f);
    std::sort(fields.begin(), fields.end(), [](auto* a, auto* b) { return std::string(a->name) < std::string(b->name); });
    for (const PhysicsGlobals::FieldInfo* f : fields) {
        const std::string name = f->name;
        std::string source = f->isConst ? "const"
                             : f->isStatic ? "static initializer"
                             : cfgKeys.count(name) ? "Physics.cfg"
                             : serKeys.count(name) ? "prefab (level0)"
                                                   : "constructor";
        if (name == "solarLuminosity") source = "CalculateValues (re-hosted)";
        const void* p = f->get(pg);
        switch (f->kind) {
            case PhysicsGlobals::FieldKind::Double: {
                const double d = *static_cast<const double*>(p);
                t.Add({name, f->isStatic, "double", ExactFormatDouble(d), d, source});
                break;
            }
            case PhysicsGlobals::FieldKind::Float: {
                const float x = *static_cast<const float*>(p);
                t.Add({name, f->isStatic, "float", ExactFormatFloat(x), static_cast<double>(x), source});
                break;
            }
            case PhysicsGlobals::FieldKind::Int: {
                const int32_t i = *static_cast<const int32_t*>(p);
                t.Add({name, f->isStatic, "int", std::to_string(i), static_cast<double>(i), source});
                break;
            }
            case PhysicsGlobals::FieldKind::Bool: {
                const bool b = *static_cast<const bool*>(p);
                t.Add({name, f->isStatic, "bool", b ? "True" : "False", b ? 1.0 : 0.0, source});
                break;
            }
            case PhysicsGlobals::FieldKind::String:
                t.Add({name, f->isStatic, "string", *static_cast<const std::string*>(p), nullptr, source});
                break;
            case PhysicsGlobals::FieldKind::Enum:
                t.Add({name, f->isStatic, f->csType, PhysicsGlobals::EnumName(*f, pg),
                       static_cast<double>(PhysicsGlobals::EnumValue(*f, pg)), source});
                break;
            case PhysicsGlobals::FieldKind::Curve:
                t.Add({name, f->isStatic, "FloatCurve", "see curves_eval PhysicsGlobals." + name, nullptr,
                       cfgKeys.count(name) ? std::string("Physics.cfg") : source});
                break;
            case PhysicsGlobals::FieldKind::LiftingSurfaces: {
                std::vector<std::string> keys;
                for (const auto& [k, v] : pg.liftingSurfaceCurves.Entries()) keys.push_back(k);
                std::sort(keys.begin(), keys.end());
                std::string joined;
                for (size_t i = 0; i < keys.size(); ++i) joined += (i ? " " : "") + keys[i];
                t.Add({name, f->isStatic, "Dictionary<string,LiftingSurfaceCurve>", joined, nullptr,
                       "LoadDefaultLiftingSurfaceCurves + Physics.cfg LIFTING_SURFACE_CURVES"});
                break;
            }
            case PhysicsGlobals::FieldKind::Ranges: {
                const VesselRanges& r = *static_cast<const VesselRanges*>(p);
                const std::pair<const char*, const VesselRanges::Situation*> sits[] = {
                    {"prelaunch", &r.prelaunch}, {"landed", &r.landed}, {"splashed", &r.splashed}, {"flying", &r.flying},
                    {"subOrbital", &r.subOrbital}, {"orbit", &r.orbit}, {"escaping", &r.escaping}};
                for (const auto& [sn, s] : sits) {
                    const std::pair<const char*, float> vals[] = {{"load", s->load}, {"unload", s->unload}, {"pack", s->pack},
                                                                  {"unpack", s->unpack}};
                    for (const auto& [vn, v] : vals)
                        t.Add({name + "." + sn + "." + vn, false, "float", ExactFormatFloat(v), static_cast<double>(v),
                               "Physics.cfg VesselRanges"});
                }
                break;
            }
            case PhysicsGlobals::FieldKind::Other:
                break;  // Gradient, SurfaceCurvesList, LiftingSurfaceCurve: not physics constants (generator: default)
        }
    }
    const double sl = PhysicsGlobals::SolarLuminosity();
    t.Add({"SolarLuminosity(property)", true, "double", ExactFormatDouble(sl), sl, "CalculateValues (re-hosted)"});
    const std::pair<const char*, double> props[] = {{"IdealGasConstant(property)", PhysicsGlobals::IdealGasConstant()},
                                                    {"StefanBoltzmanConstant(property)", PhysicsGlobals::StefanBoltzmanConstant()},
                                                    {"BoltzmannConstant(property)", PhysicsGlobals::BoltzmannConstant()},
                                                    {"AvogadroConstant(property)", PhysicsGlobals::AvogadroConstant()}};
    for (const auto& [n, v] : props) t.Add({n, true, "double", ExactFormatDouble(v), v, "static initializer"});
    g.tables.push_back(std::move(t));
    return WriteCandidate(opt, g, "physics_constants/physics_constants.json");
}

// physics_drag_curve: PhysicsGlobals.DragCurveValue(SurfaceCurves, dotNormalized, mach) under Mono, judged against the
// in-game capture harness/captures/out/aero_forces_mono.json table drag_curve_value (inputs: AeroGen.cs:72-78).
int RunPhysicsDragCurve(const RunnerOptions& opt) {
    CelestialEnv::Get(opt);
    golden::File g;
    g.dataset = "aero_forces";
    g.description = "kspcore candidate for PhysicsGlobals.DragCurveValue (Mono float rule).";
    golden::Table t("drag_curve_value", "PhysicsGlobals.DragCurveValue(SurfaceCurves, dotNormalized, mach).");
    t.Col("dotNormalized", golden::ColType::F32).Col("mach", golden::ColType::F32).Col("value", golden::ColType::F32);
    t.key = {"dotNormalized", "mach"};
    std::vector<float> machGrid;
    for (int i = 0; i < 61; ++i) machGrid.push_back(static_cast<float>(i) * 0.05f);
    for (float m : {3.5f, 4.0f, 5.0f, 7.0f, 10.0f, 15.0f, 20.0f, 25.0f, 30.0f}) machGrid.push_back(m);
    for (int i = 0; i <= 40; ++i)
        for (float m : machGrid) {
            const float dn = static_cast<float>(i) / 40.0f;
            t.Add({dn, m, PhysicsGlobals::DragCurveValue(PhysicsGlobals::SurfaceCurves, dn, m)});
        }
    g.tables.push_back(std::move(t));
    return WriteCandidate(opt, g, "physics_drag_curve/physics_drag_curve.json");
}

}  // namespace

KSP_GOLDEN_DATASET("physics_constants", 60, RunPhysicsConstants);
KSP_GOLDEN_DATASET("physics_drag_curve", 61, RunPhysicsDragCurve);
