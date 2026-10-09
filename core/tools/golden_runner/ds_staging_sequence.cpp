// WP-16 staging_sequence: the staging model of the 17 crafts of the C5 delta-v capture (harness/captures/out2/deltav_readout.json).
//
// What the capture holds of staging: `craft.stage_count` and, per part, `inverseStage` (Part.inverseStage as VesselDeltaV saw it after
// the editor loaded the craft: the result of StageManager's SortIcons / UpdateStageGroups / SetSeparationIndices pipeline, which
// rewrites the file's istg for the parts without a stage icon) and `isDecoupler` (DeltaVPartInfo: ModuleDecouple or
// ModuleAnchoredDecoupler on the part). decoupleStage / activationStage / the stages table are VesselDeltaV's (WP-17).
//
// Inputs (original game data only): the craft files of the capture (see ds_resource_mass.cpp), GameData of --gamedata. The run
// replays the editor: every part is built (Part.Instantiate + the craft's values), its modules' OnStart and the staging block of
// Part.Start run (icons are created and held), then onEditorLoad + the deferred SortIcons run. Outputs: the golden's table shapes
// (`parts`: craft_id, part_index, part, inverseStage, isDecoupler; `craft`: craft_id, stage_count).
#include <filesystem>
#include <string>
#include <vector>

#include "craft_tree.hpp"
#include "editor_staging.hpp"
#include "datasets.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/resources/planetary_resources.hpp"
#include "ksp/resources/resource_broker.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/staging/stage_manager.hpp"

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

int RunStagingSequence(const RunnerOptions& opt) {
    SaveEnvironment::Reset();
    GameData data;
    GameData::Options go;
    go.gameRoot = GameRootOf(opt.gamedata);
    go.gameDataDir = fs::absolute(dotnet::PathFromUtf8(opt.gamedata)).lexically_normal().string();
    data.Load(go);
    SaveEnvironment& env = SaveEnvironment::Current();
    env.getPartInfoByName = [&data](std::string_view n) { return data.parts.getPartInfoByName(n); };
    env.partExists = [&data](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
    support::InstallUpgradeHooks(data);
    ResourceSetup::database = &data.database;
    ResourceSetup::ResetConfig();

    golden::File g;
    g.dataset = "staging_sequence";
    g.description = "kspcore candidate for the staging results of the C5 delta-v capture crafts (StageManager, ModuleDecouple).";
    golden::Table tc("craft", "Per craft: StageManager.StageCount after the editor loaded it.");
    tc.Col("craft_id", ColType::Str).Col("stage_count", ColType::I64);
    tc.key = {"craft_id"};
    golden::Table t("parts", "Per-part Part.inverseStage after the staging sequence was rebuilt, and whether the part is a decoupler.");
    t.Col("craft_id", ColType::Str).Col("part_index", ColType::I64).Col("part", ColType::Str).Col("inverseStage", ColType::I64).Col("isDecoupler", ColType::Bool);
    t.key = {"craft_id", "part_index"};

    const fs::path shipsDir = fs::path(dotnet::PathFromUtf8(opt.harness)) / "inputs" / "Ships";
    const fs::path repoRoot = fs::path(dotnet::PathFromUtf8(opt.harness)).parent_path();
    for (const CraftEntry& c : kCrafts) {
        const std::string rel = c.relPath;
        const fs::path file = rel[0] == '!' ? repoRoot / rel.substr(1) : shipsDir / rel;
        ShipConstruct ship;
        std::string err;
        if (!support::LoadCraftFile(file.string(), ship, err)) throw std::runtime_error(std::string("LoadShip ") + c.craftId + ": " + err);
        support::CraftTree tree = support::BuildCraftTree(data, ship);
        if (!tree.ok) throw std::runtime_error(std::string("missing part ") + tree.missing + " in " + c.craftId);

        support::EditorSession editor(tree);
        support::EditorSession::StartParts(tree);
        editor.Load();

        tc.Add({c.craftId, static_cast<int64_t>(StageManager::StageCount())});
        for (size_t i = 0; i < tree.parts.size(); i++) {
            Part& p = *tree.parts[i];
            const bool isDecoupler = p.FindModuleOfClass("ModuleDecouple") != nullptr || p.FindModuleOfClass("ModuleAnchoredDecoupler") != nullptr;
            // VesselDeltaV.cs:636 builds no DeltaVPartInfo for launch clamps: the capture's isDecoupler / isEngine are null there
            t.Add({c.craftId, static_cast<int64_t>(i), p.gameObjectName, static_cast<int64_t>(p.fields.inverseStage),
                   p.isLaunchClamp() ? Cell(nullptr) : Cell(isDecoupler)});
        }
    }
    g.tables.push_back(std::move(tc));
    g.tables.push_back(std::move(t));

    ResourceSetup::database = nullptr;
    ResourceSetup::ResetConfig();
    SaveEnvironment::Reset();
    return WriteCandidate(opt, g, "staging_sequence/staging_sequence.json");
}

}  // namespace

KSP_GOLDEN_DATASET("staging_sequence", 135, RunStagingSequence);
