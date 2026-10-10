// WP-23 engine_thrust: the .NET harness golden harness/golden/engine_thrust (harness/src/KspGolden/Generators/EnginesGen.cs):
// every ModuleEngines / ModuleEnginesFX of the stock part cfgs under the .NET Framework float policy (aero::NetFloat).
//
// Inputs (original game data only): the stock part cfgs of --gamedata (GameData/Squad/Parts/**/*.cfg, the files EnginesGen reads
// from harness/inputs), compiled by kspcore's own PartLoader into prefabs whose ModuleEngines objects (KSPFields, propellants,
// atmosphereCurve, OnLoad) are the module under test, and Kerbin's atmosphere (WP-09). The engine order, ids and cfg paths follow
// the generator: files in ordinal order, PART nodes, MODULE nodes named ModuleEngines / ModuleEnginesFX, id
// `part/Module[engineID]` with `#n` for a second engine without an engineID.
//
// Policy: the generator ran KSP's IL on .NET Framework 4.8 (every float operation rounds to float32); the module's float maths
// is a template over the policy, so this runner instantiates it with NetFloat and re-hosts the OnLoad statements
// (ModuleEngines.cs:636-647) exactly as the generator does (`maxThrust / (atmosphereCurve.Evaluate(0f) * g)` under NetFloat).
// The in-game values of the same expressions (MonoFloat, the production policy) are gated by capture.engine_deltav.
//
// Output: tables engines, isp_thrust_vs_pressure, kerbin_flight, rocket_equation.
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>

#include "celestial_env.hpp"
#include "datasets.hpp"
#include "ksp/aero/float_eval.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/dotnet/math.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/parts/cs_parse.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/part_resource_library.hpp"

namespace {

using namespace ksp;
using golden::Cell;
using golden::ColType;
namespace fs = std::filesystem;
using P = aero::NetFloat;
using E = aero::FE<P>;

const double kPressuresAtm[] = {0, 0.001, 0.01, 0.05, 0.1, 0.2, 0.3, 0.5, 0.75, 1.0, 1.25, 1.5, 2, 3, 5, 7, 10, 15};

struct Eng {
    std::string id, file;
    ModuleEngines* m = nullptr;
    ConfigNode* node = nullptr;
    Part* part = nullptr;
};

const CelestialBody* FindKerbin(CelestialEnv& env) {
    for (CelestialBody* b : env.system->SpawnOrder())
        if (b->bodyName == "Kerbin") return b;
    throw std::runtime_error("engine_thrust: no Kerbin");
}

// EnginesGen.cs:19-67 LoadEngines
std::vector<Eng> LoadEngines(GameData& data, const std::string& gameRoot, std::vector<ConfigNode::NodePtr>& keepAlive) {
    std::vector<Eng> list;
    const fs::path root = dotnet::PathFromUtf8(gameRoot) / "GameData" / "Squad" / "Parts";
    std::vector<fs::path> files;
    for (const auto& de : fs::recursive_directory_iterator(root))
        if (de.is_regular_file() && de.path().extension() == ".cfg") files.push_back(de.path());
    std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) { return a.generic_string() < b.generic_string(); });
    for (const fs::path& file : files) {
        ConfigNode::NodePtr cn = ConfigNode::Load(file.string());
        if (!cn) continue;
        keepAlive.push_back(cn);
        for (ConfigNode* partNode : cn->GetNodes("PART")) {
            const std::string partName = *partNode->GetValue("name");
            std::string lookup = partName;
            std::replace(lookup.begin(), lookup.end(), '_', '.');  // PartLoader: AvailablePart.name
            const AvailablePart* ap = data.parts.getPartInfoByName(lookup);
            if (ap == nullptr) ap = data.parts.getPartInfoByName(partName);
            int idx = 0;
            int moduleIndex = -1;
            for (ConfigNode* mn : partNode->GetNodes("MODULE")) {
                moduleIndex++;
                const std::string mname = *mn->GetValue("name");
                if (mname != "ModuleEngines" && mname != "ModuleEnginesFX") continue;
                idx++;
                if (ap == nullptr || !ap->partPrefab) throw std::runtime_error("engine_thrust: part not loaded: " + partName);
                PartPrefab& prefab = *ap->partPrefab;
                if (static_cast<size_t>(moduleIndex) >= prefab.ModuleData.size()) throw std::runtime_error("engine_thrust: module index " + partName);
                auto* m = dynamic_cast<ModuleEngines*>(prefab.ModuleData[static_cast<size_t>(moduleIndex)].module.get());
                if (m == nullptr) throw std::runtime_error("engine_thrust: not a ModuleEngines: " + partName + " " + mname);
                const auto eid = mn->GetValue("engineID");
                std::string rel = std::filesystem::relative(file, dotnet::PathFromUtf8(gameRoot)).generic_string();
                Eng e;
                e.id = partName + "/" + mname + ((eid && !eid->empty()) ? "[" + *eid + "]" : "") + ((idx > 1 && !(eid && !eid->empty())) ? "#" + std::to_string(idx) : "");
                e.file = rel;
                e.m = m;
                e.node = mn;
                e.part = ap->partPrefab.get();
                list.push_back(std::move(e));
            }
        }
    }
    return list;
}

// ExactNumber.FormatFloat (the `str` columns of the harness)
std::string F(float f) { return ExactFormatFloat(f); }

int RunEngineThrust(const RunnerOptions& opt) {
    CelestialEnv& env = CelestialEnv::Get(opt);
    const CelestialBody* kerbin = FindKerbin(env);
    GameData data;
    GameData::Options go;
    namespace fsys = std::filesystem;
    const fsys::path gd = fsys::absolute(dotnet::PathFromUtf8(opt.gamedata)).lexically_normal();
    fsys::path p = gd;
    if (!p.has_filename()) p = p.parent_path();
    const std::u8string rootU8 = p.parent_path().u8string();
    const std::string gameRoot(rootU8.begin(), rootU8.end());
    go.gameRoot = gameRoot;
    go.gameDataDir = gd.string();
    data.Load(go);

    std::vector<ConfigNode::NodePtr> keepAlive;
    std::vector<Eng> engines = LoadEngines(data, gameRoot, keepAlive);

    golden::File g;
    g.dataset = "engine_thrust";
    g.description = "kspcore candidate for the EnginesGen tables under the .NET Framework float policy (aero::NetFloat).";
    golden::Table te("engines", "Per engine module: loaded parameters.");
    te.Col("engine_id", ColType::Str).Col("file", ColType::Str).Col("maxThrust", ColType::F32).Col("minThrust", ColType::F32).Col("g", ColType::F32)
        .Col("maxFuelFlow", ColType::F32).Col("minFuelFlow", ColType::F32).Col("throttleMin", ColType::F32).Col("isp_vac", ColType::F32)
        .Col("isp_asl", ColType::F32).Col("exhaustVelocity_vac", ColType::F32).Col("MaxThrustOutputVac", ColType::F32).Col("atmChangeFlow", ColType::Bool)
        .Col("useAtmCurve", ColType::Bool).Col("useVelCurve", ColType::Bool).Col("useThrustCurve", ColType::Bool).Col("flowMultCap", ColType::F32)
        .Col("multIsp", ColType::F32).Col("multFlow", ColType::F32).Col("mixtureDensity", ColType::F32).Col("ratioSum", ColType::F64)
        .Col("propellant_maxFuelFlow", ColType::Str).Col("atmosphereCurve_keys", ColType::Str);
    te.key = {"engine_id"};
    golden::Table tp("isp_thrust_vs_pressure", "atmosphereCurve.Evaluate(p) and MaxThrustOutputAtm at pressure p, machNumber 0.");
    tp.Col("engine_id", ColType::Str).Col("pressure_atm", ColType::F32).Col("isp", ColType::F32).Col("MaxThrustOutputAtm", ColType::F32)
        .Col("GetEngineThrust_full", ColType::F32).Col("GetEngineThrust_half", ColType::F32);
    tp.key = {"engine_id", "pressure_atm"};
    golden::Table tk("kerbin_flight", "Engines in Kerbin atmosphere: altitude x mach.");
    tk.Col("engine_id", ColType::Str).Col("altitude", ColType::F64).Col("mach", ColType::F64).Col("pressure_atm", ColType::F32).Col("density", ColType::F64)
        .Col("isp", ColType::F32).Col("MaxThrustOutputAtm", ColType::F32);
    tk.key = {"engine_id", "altitude", "mach"};
    golden::Table tr("rocket_equation", "dV per DeltaVStageInfo.StoreInterimDeltaV for a single +Y engine.");
    tr.Col("engine_id", ColType::Str).Col("situation", ColType::Str).Col("m0", ColType::F32).Col("m1", ColType::F32).Col("thrust", ColType::F32)
        .Col("isp", ColType::F64).Col("dV", ColType::F32).Col("burnTime_fullThrottle", ColType::F64);
    tr.key = {"engine_id", "situation", "m0", "m1"};

    for (Eng& e : engines) {
        ModuleEngines& m = *e.m;
        // EnginesGen.cs:51-55: the re-hosted OnLoad statements under the .NET policy (the module's own OnLoad ran in the loader under
        // MonoFloat: its values are replaced by the NetFloat ones here, as the generator computes them)
        const float ispVac0 = m.Get<FloatCurve>("atmosphereCurve").Evaluate(0.0f);
        if (e.node->HasValue("maxThrust")) {
            m.maxThrust = cs::FloatParse(*e.node->GetValue("maxThrust"));
            m.maxFuelFlow = ModuleEngines::FuelFlowFromThrust<P>(m.maxThrust, ispVac0, m.g);
        }
        if (e.node->HasValue("minThrust")) {
            m.minThrust = cs::FloatParse(*e.node->GetValue("minThrust"));
            m.minFuelFlow = ModuleEngines::FuelFlowFromThrust<P>(m.minThrust, ispVac0, m.g);
        }
        m.throttleMin = aero::S(E(m.minFuelFlow) / E(m.maxFuelFlow));
        m.SetupPropellantT<P>();

        const FloatCurve& atm = m.Get<FloatCurve>("atmosphereCurve");
        const float ispVac = atm.Evaluate(0.0f), ispAsl = atm.Evaluate(1.0f);
        const float vac = m.MaxThrustOutputVacT<P>(true);
        std::string props, keys;
        for (const Propellant& pp : m.propellants) {
            if (!props.empty()) props += " ";
            props += pp.name + ":" + F(pp.ratio) + ":" + F(m.getMaxFuelFlow(pp));
        }
        if (ConfigNode* ac = e.node->GetNode("atmosphereCurve")) {
            for (const std::string& k : ac->GetValues("key")) {
                if (!keys.empty()) keys += " | ";
                keys += k;
            }
        }
        te.Add({e.id, e.file, m.maxThrust, m.minThrust, m.g, m.maxFuelFlow, m.minFuelFlow, m.throttleMin, ispVac, ispAsl, m.getExhaustVelocity(ispVac), vac,
                m.Get<bool>("atmChangeFlow"), m.Get<bool>("useAtmCurve"), m.Get<bool>("useVelCurve"), m.Get<bool>("useThrustCurve"),
                m.Get<float>("flowMultCap"), m.Get<float>("multIsp"), m.Get<float>("multFlow"), m.mixtureDensity, m.ratioSum, props, keys});

        for (double pr : kPressuresAtm) {
            const float pf = static_cast<float>(pr);
            const float isp = atm.Evaluate(pf);
            tp.Add({e.id, pf, isp, m.MaxThrustOutputAtmT<P>(false, true, pf, 310.0, 1.225 * pr, 0.0), m.GetEngineThrustT<P>(isp, 1.0f),
                    m.GetEngineThrustT<P>(isp, 0.5f)});
        }
        for (double alt : {0.0, 2000.0, 5000.0, 10000.0, 15000.0, 20000.0, 25000.0, 30000.0, 40000.0, 50000.0, 69000.0})
            for (double mach : {0.0, 0.3, 0.8, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0}) {
                const double pk = kerbin->GetPressure(alt), T = kerbin->GetTemperature(alt), rho = kerbin->GetDensity(pk, T);
                const float patm = static_cast<float>(pk * 0.009869232667160128);
                tk.Add({e.id, alt, mach, patm, rho, atm.Evaluate(patm), m.MaxThrustOutputAtmT<P>(false, true, patm, T, rho, mach)});
            }
        // EnginesGen.cs:125-139 (DeltaVStageInfo.cs:342: v = enginesThrust * (float)(Math.Log(startingMass / currentMass) * isp * g0 / (double)thrust))
        for (const char* sit : {"vacuum", "kerbin_asl"}) {
            const float pf = std::string(sit) == "vacuum" ? 0.0f : 1.0f;
            const float thrust = pf == 0.0f ? vac : m.MaxThrustOutputAtmT<P>(false, true, pf, 310.0, 1.225, 0.0);
            const double isp = static_cast<double>(atm.Evaluate(pf));
            const std::pair<float, float> masses[] = {{10.0f, 5.0f}, {20.0f, 8.0f}, {3.5f, 1.2f}, {100.0f, 40.0f}, {1.5f, 1.4999f}};
            for (const auto& [m0, m1] : masses) {
                const Vector3f thrustVec{0.0f, thrust, 0.0f};
                const double logv = dotnet::Math::Log(static_cast<double>(aero::S(E(m0) / E(m1))));
                const float s = static_cast<float>(logv * isp * PhysicsGlobals::GravitationalAcceleration / static_cast<double>(thrust));
                const Vector3f v = thrustVec * s;
                tr.Add({e.id, sit, m0, m1, thrust, isp, aero::U<P>::Magnitude(v),
                        static_cast<double>(aero::S(E(m0) - E(m1))) / static_cast<double>(m.maxFuelFlow)});
            }
        }
    }
    g.tables.push_back(std::move(te));
    g.tables.push_back(std::move(tp));
    g.tables.push_back(std::move(tk));
    g.tables.push_back(std::move(tr));
    std::cout << "engine_thrust: " << engines.size() << " engine modules\n";
    return WriteCandidate(opt, g, "engine_thrust/engine_thrust.json");
}

}  // namespace

KSP_GOLDEN_DATASET("engine_thrust", 150, RunEngineThrust);
