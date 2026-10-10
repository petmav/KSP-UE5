// WP-23 engine_deltav: the engine numbers VesselDeltaV reads from ModuleEngines for the 17 crafts of the C5 delta-v capture
// (harness/captures/out2/deltav_readout.json, table `engines`, written by the profiling build's HarnessCapture2 C5 from
// DeltaVEngineInfo; part of in-game truth: Mono float semantics, aero::MonoFloat).
//
// What is ModuleEngines' and what is not. Per engine the capture holds ispVac / ispASL / ispActual, thrustVac / thrustASL /
// thrustActual, requiresAir and the propellant list. These come from DeltaVEngineInfo.CalculateISP (DeltaVEngineInfo.cs:610) and
// CalculateThrustVector / CalcThrustActual (:687, :535), whose EDITOR branches call exactly ModuleEngines.atmosphereCurve /
// atmCurveIsp .Evaluate, MaxThrustOutputVac, MaxThrustOutputAtm and read `propellants`; this runner re-hosts those few statements
// (DeltaVEngineInfo itself, the engine list order and the stage simulation are WP-17's). Not gated here (WP-17 / WP-16):
// `startBurnStage` (= engine.part.inverseStage, DeltaVEngineInfo.cs:88, gated by capture.staging_sequence as parts.inverseStage) and
// the row order `idx` is reproduced from VesselDeltaV.UpdateModuleEngines (VesselDeltaV.cs:1085-1147: ship parts in order, one entry per
// part, a MultiModeEngine part once with the engine of `runningPrimary`).
//
// Inputs (original game data only): the craft files of the capture (see ds_resource_mass.cpp), GameData of --gamedata, the bodies of
// WP-09 (Kerbin, Mun, Duna, Eve at altitude 0: DeltaVAppValues.atmPressure / atmDensity, DeltaVAppValues.cs:83-85).
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "celestial_env.hpp"
#include "craft_tree.hpp"
#include "datasets.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/modules/module_engines.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/resources/resource_broker.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"

namespace {

using namespace ksp;
using golden::Cell;
using golden::ColType;
namespace fs = std::filesystem;

struct CraftEntry {
    const char* craftId;
    const char* relPath;  // under harness/inputs/Ships, or "!analysis/..." under the repository root
};

// deltav_readout.json `craft` table (craft_id, path)
const CraftEntry kCrafts[] = {
    {"PhysProf-S", "VAB/PhysProf-S.craft"},
    {"PhysProf-M2", "!analysis/physics/craft/PhysProf-M2.craft"},
    {"PhysProf-L2-AS", "!analysis/physics/craft/PhysProf-L2-AS.craft"},
    {"craft_Kerbal_X", "VAB/Kerbal X.craft"},
    {"craft_Jumping_Flea", "VAB/Jumping Flea.craft"},
    {"craft_Kerbal_1", "VAB/Kerbal 1.craft"},
    {"craft_Kerbal_2", "VAB/Kerbal 2.craft"},
    {"craft_Learstar_A1", "VAB/Learstar A1.craft"},
    {"craft_Ion-Powered_Space_Probe", "VAB/Ion-Powered Space Probe.craft"},
    {"craft_Dynawing", "VAB/Dynawing.craft"},
    {"craft_ComSat_Lx", "VAB/ComSat Lx.craft"},
    {"craft_Aeris_3A", "SPH/Aeris 3A.craft"},
    {"craft_Aeris_4A", "SPH/Aeris 4A.craft"},
    {"craft_Gull", "SPH/Gull.craft"},
    {"craft_Stearwing_A300", "SPH/Stearwing A300.craft"},
    {"craft_Bug-E_Buggy", "SPH/Bug-E Buggy.craft"},
    {"craft_Mallard", "SPH/Mallard.craft"},
};
const char* const kBodies[] = {"Kerbin", "Mun", "Duna", "Eve"};

// the capture's R(float) (HarnessCapture2 `R`): the shortest round-trip text, an integral value without the ".0" the golden writer adds
std::string FormatR(float f) {
    std::string s = ExactFormatFloat(f);
    if (s.size() > 2 && s.compare(s.size() - 2, 2, ".0") == 0) s.resize(s.size() - 2);
    return s;
}

std::string GameRootOf(const std::string& gamedata) {
    const fs::path gd = fs::absolute(dotnet::PathFromUtf8(gamedata)).lexically_normal();
    fs::path p = gd;
    if (!p.has_filename()) p = p.parent_path();
    const std::u8string s = p.parent_path().u8string();
    return std::string(s.begin(), s.end());
}

const CelestialBody* FindBody(CelestialEnv& env, const std::string& name) {
    for (CelestialBody* b : env.system->SpawnOrder())
        if (b->bodyName == name) return b;
    throw std::runtime_error("engine_deltav: no body " + name);
}

// VesselDeltaV.cs:1085-1147 UpdateModuleEngines (Ship mode) + DeltaVEngineInfo's constructor: the engine of a part's entry
ModuleEngines* EngineOfEntry(Part& part, bool& skipPart) {
    skipPart = false;
    std::vector<ModuleEngines*> engines;  // Part.isEngine(out engines): the part's ModuleEngines in module order
    for (size_t i = 0; i < part.Modules().Count(); i++)
        if (auto* e = dynamic_cast<ModuleEngines*>(part.Modules().GetModule(i))) engines.push_back(e);
    if (engines.empty()) {
        skipPart = true;
        return nullptr;
    }
    MultiModeEngine* multi = nullptr;
    for (size_t i = 0; i < part.Modules().Count(); i++)
        if (auto* mm = dynamic_cast<MultiModeEngine*>(part.Modules().GetModule(i))) {
            multi = mm;
            break;
        }
    for (ModuleEngines* e : engines) {
        if (e->Get<bool>("nonThrustMotor") && !e->Get<bool>("includeinDVCalcs")) continue;
        if (multi != nullptr) return multi->runningPrimary() ? multi->PrimaryEngine() : multi->SecondaryEngine();
        return e;
    }
    skipPart = true;
    return nullptr;
}

int RunEngineDeltaV(const RunnerOptions& opt) {
    SaveEnvironment::Reset();
    CelestialEnv& env = CelestialEnv::Get(opt);
    GameData data;
    GameData::Options go;
    go.gameRoot = GameRootOf(opt.gamedata);
    go.gameDataDir = fs::absolute(dotnet::PathFromUtf8(opt.gamedata)).lexically_normal().string();
    data.Load(go);
    SaveEnvironment& senv = SaveEnvironment::Current();
    senv.getPartInfoByName = [&data](std::string_view n) { return data.parts.getPartInfoByName(n); };
    senv.partExists = [&data](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
    support::InstallUpgradeHooks(data);
    ResourceSetup::database = &data.database;
    ResourceSetup::ResetConfig();

    golden::File g;
    g.dataset = "engine_deltav";
    g.description = "kspcore candidate for the engine rows of the C5 delta-v capture (DeltaVEngineInfo's editor-situation reads of ModuleEngines).";
    golden::Table t("engines", "DeltaVEngineInfo values per engine of the craft, per situation body (altitude 0).");
    t.Col("craft_id", ColType::Str).Col("body", ColType::Str).Col("idx", ColType::I64).Col("part", ColType::Str).Col("part_index", ColType::I64)
        .Col("ispVac", ColType::F64).Col("ispASL", ColType::F64).Col("ispActual", ColType::F64).Col("thrustVac", ColType::F32)
        .Col("thrustASL", ColType::F32).Col("thrustActual", ColType::F32).Col("requiresAir", ColType::Bool).Col("propellants", ColType::Str);
    t.key = {"craft_id", "body", "idx"};

    const fs::path shipsDir = fs::path(dotnet::PathFromUtf8(opt.harness)) / "inputs" / "Ships";
    const fs::path repoRoot = fs::path(dotnet::PathFromUtf8(opt.harness)).parent_path();
    size_t engineRows = 0;
    for (const CraftEntry& c : kCrafts) {
        const std::string rel = c.relPath;
        const fs::path file = rel[0] == '!' ? repoRoot / rel.substr(1) : shipsDir / rel;
        ShipConstruct ship;
        std::string err;
        if (!support::LoadCraftFile(file.string(), ship, err)) throw std::runtime_error(std::string("LoadShip ") + c.craftId + ": " + err);
        support::CraftTree tree = support::BuildCraftTree(data, ship);
        if (!tree.ok) throw std::runtime_error(std::string("missing part ") + tree.missing + " in " + c.craftId);
        // the editor starts every module: MultiModeEngine finds its two engines (MultiModeEngine.cs:326 OnStart)
        for (auto& p : tree.parts)
            for (size_t i = 0; i < p->Modules().Count(); i++)
                if (auto* mm = dynamic_cast<MultiModeEngine*>(p->Modules().GetModule(i))) mm->OnStart(StartState::Editor);
        for (const char* bodyName : kBodies) {
            const CelestialBody* body = FindBody(env, bodyName);
            // DeltaVAppValues.atmPressure / atmDensity at altitude 0 (DeltaVAppValues.cs:83-85); body.atmPressureASL / atmDensityASL (CelestialBody.cs:370)
            const float appPressure = static_cast<float>(body->GetPressure(0.0) * 0.009869232667160128);
            const double appDensity = body->GetDensity(body->GetPressure(0.0), body->GetTemperature(0.0));
            const float time2 = static_cast<float>(body->atmPressureASL);
            const float time3 = static_cast<float>(body->atmDensityASL);
            int idx = 0;
            for (size_t pi = 0; pi < tree.parts.size(); pi++) {
                Part& part = *tree.parts[pi];
                bool skip = false;
                ModuleEngines* engine = EngineOfEntry(part, skip);
                if (skip) continue;
                if (engine == nullptr) throw std::runtime_error("engine_deltav: multimode engine without engines in " + std::string(c.craftId));
                const FloatCurve atm = engine->Get<FloatCurve>("atmosphereCurve");
                const bool useAtmCurveIsp = engine->Get<bool>("useAtmCurveIsp");
                const FloatCurve atmCurveIsp = engine->Get<FloatCurve>("atmCurveIsp");
                // DeltaVEngineInfo.cs:625-631
                double ispASL = atm.Evaluate(time2);
                if (useAtmCurveIsp) ispASL *= static_cast<double>(atmCurveIsp.Evaluate(time3));
                const double ispVac = atm.Evaluate(0.0f);
                // :659-665 (editor branch)
                double ispActual = atm.Evaluate(appPressure);
                if (useAtmCurveIsp) ispActual *= static_cast<double>(atmCurveIsp.Evaluate(static_cast<float>(appDensity * 0.8163265306122448)));
                // DeltaVEngineInfo.cs:689-701, 561 (editor branches)
                const float thrustVac = engine->MaxThrustOutputVac();
                const float thrustASL = engine->MaxThrustOutputAtm(false, true, time2, body->atmosphereTemperatureSeaLevel, body->atmDensityASL);
                const float thrustActual = engine->MaxThrustOutputAtm(false, true, appPressure, 310.0, appDensity);
                bool requiresAir = false;
                std::string props;
                for (const Propellant& pr : engine->propellants) {
                    if (pr.name == "IntakeAir") requiresAir = true;
                    if (!props.empty()) props += ",";
                    props += pr.name + ":" + FormatR(pr.ratio);
                }
                t.Add({c.craftId, bodyName, idx++, part.gameObjectName, static_cast<int64_t>(pi), ispVac, ispASL, ispActual, thrustVac, thrustASL,
                       thrustActual, requiresAir, props});
                engineRows++;
            }
        }
    }
    g.tables.push_back(std::move(t));
    std::cout << "engine_deltav: " << engineRows << " engine rows\n";
    ResourceSetup::database = nullptr;
    ResourceSetup::ResetConfig();
    SaveEnvironment::Reset();
    return WriteCandidate(opt, g, "engine_deltav/engine_deltav.json");
}

}  // namespace

KSP_GOLDEN_DATASET("engine_deltav", 151, RunEngineDeltaV);
