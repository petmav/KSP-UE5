// WP-18 ship_construct: ShipConstruct.GetShipMass(dryMass, fuelMass) and the per-part masses of the 17 crafts of the C5 delta-v capture
// (harness/captures/out2/deltav_readout.json: table `craft` columns parts, dry_mass, fuel_mass; table `parts` columns mass, module_mass).
//
// The editor ship of the capture is the craft file after KSPUpgradePipeline.Process (as ShipConstruction.CreateConstructFromTemplate runs
// it) and ShipConstruct.LoadShip; kspcore does the same (ShipConstruct::LoadShipWithParts). dry_mass / fuel_mass are the out parameters of
// GetShipMass (UpdateMass of every part, then sums of part.mass and density * amount in the C# order); `mass` is Part.mass after
// UpdateMass and `module_mass` the part's IPartMassModifier sum (Part.moduleMass). The capture's total_mass adds the editor crew
// (VesselCrewManifest, not ported): it is not gated here; tests/test_ship_construct.cpp checks that it is no-crew total + n * KerbalCrewMass.
//
// Inputs (original game data only): the craft files (harness/inputs/Ships; PhysProf-M2 / -L2-AS in analysis/physics/craft; each file's
// SHA-256 is the capture's craft.file_sha256) and the GameData of --gamedata. Part order is the craft file's PART order.
#include <filesystem>
#include <string>
#include <vector>

#include "craft_tree.hpp"
#include "datasets.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/resources/resource_broker.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/save/upgrade_pipeline.hpp"

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

std::string GameRootOf(const std::string& gamedata) {
    const fs::path gd = fs::absolute(dotnet::PathFromUtf8(gamedata)).lexically_normal();
    fs::path p = gd;
    if (!p.has_filename()) p = p.parent_path();
    const std::u8string s = p.parent_path().u8string();
    return std::string(s.begin(), s.end());
}

int RunShipConstruct(const RunnerOptions& opt) {
    SaveEnvironment::Reset();
    GameData data;
    GameData::Options go;
    go.gameRoot = GameRootOf(opt.gamedata);
    go.gameDataDir = fs::absolute(dotnet::PathFromUtf8(opt.gamedata)).lexically_normal().string();
    data.Load(go);
    SaveEnvironment& env = SaveEnvironment::Current();
    env.loadedScene = GameScenes::EDITOR;  // the C5 capture was taken in the VAB
    env.getPartInfoByName = [&data](std::string_view n) { return data.parts.getPartInfoByName(n); };
    env.partExists = [&data](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
    support::InstallUpgradeHooks(data);
    ResourceSetup::database = &data.database;
    ResourceSetup::ResetConfig();

    golden::File g;
    g.dataset = "ship_construct";
    g.description = "kspcore candidate for the ShipConstruct masses of the C5 delta-v capture crafts (GetShipMass, Part.mass, module mass).";
    golden::Table tc("craft", "Per craft: part count and GetShipMass(out dryMass, out fuelMass) (tonnes).");
    tc.Col("craft_id", ColType::Str).Col("parts", ColType::I64).Col("dry_mass", ColType::F32).Col("fuel_mass", ColType::F32);
    tc.key = {"craft_id"};
    golden::Table tp("parts", "Per part: Part.mass after UpdateMass and the module mass (tonnes), in craft-file part order.");
    tp.Col("craft_id", ColType::Str).Col("part_index", ColType::I64).Col("part", ColType::Str).Col("mass", ColType::F32).Col("module_mass", ColType::F32);
    tp.key = {"craft_id", "part_index"};

    const fs::path shipsDir = fs::path(dotnet::PathFromUtf8(opt.harness)) / "inputs" / "Ships";
    const fs::path repoRoot = fs::path(dotnet::PathFromUtf8(opt.harness)).parent_path();
    for (const CraftEntry& c : kCrafts) {
        const std::string rel = c.relPath;
        const fs::path file = rel[0] == '!' ? repoRoot / rel.substr(1) : shipsDir / rel;
        ConfigNode::NodePtr root = ConfigNode::Load(file.string());
        if (!root) throw std::runtime_error(std::string("cannot read ") + c.craftId);
        ConfigNode::NodePtr upgraded;
        const std::string* sn = root->GetValuePtr("ship");
        KSPUpgradePipeline::Process(root, sn ? *sn : std::string(), SaveUpgradePipeline::LoadContext::Craft,
                                    [&](const ConfigNode::NodePtr& n) { upgraded = n; },
                                    [&](KSPUpgradePipeline::UpgradeFailOption, const ConfigNode::NodePtr&) {});
        if (!upgraded) throw std::runtime_error(std::string("upgrade pipeline failed for ") + c.craftId);
        ShipConstruct ship;
        std::string err;
        if (!ship.LoadShipWithParts(*upgraded, 0, true, err)) throw std::runtime_error(std::string("LoadShip ") + c.craftId + ": " + err);
        // The capture is an editor session: Part.Start ran on every part (ModulesOnStart, Part.cs:1727), so ModuleProceduralFairing.OnStart
        // set `panelMass = GetFairingArea() * UnitAreaMass` from the XSECTION nodes the craft file stores (ModuleProceduralFairing.cs:443).
        for (Part* p : ship.liveParts) p->ModulesOnStart(StartState::Editor);
        float dryMass = 0.0f, fuelMass = 0.0f;
        ship.GetShipMass(dryMass, fuelMass);
        tc.Add({c.craftId, static_cast<int64_t>(ship.LiveCount()), dryMass, fuelMass});
        for (int i = 0; i < ship.LiveCount(); i++) {
            Part* p = ship.liveParts[static_cast<size_t>(i)];
            tp.Add({c.craftId, static_cast<int64_t>(i), p->gameObjectName, p->fields.mass, p->moduleMass});
        }
    }
    g.tables.push_back(std::move(tc));
    g.tables.push_back(std::move(tp));

    ResourceSetup::database = nullptr;
    ResourceSetup::ResetConfig();
    SaveEnvironment::Reset();
    return WriteCandidate(opt, g, "ship_construct/ship_construct.json");
}

}  // namespace

KSP_GOLDEN_DATASET("ship_construct", 131, RunShipConstruct);
