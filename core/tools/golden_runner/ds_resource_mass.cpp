// WP-15 resource_mass: Part.GetResourceMass() of every part of the 17 crafts of the C5 delta-v capture
// (harness/captures/out2/deltav_readout.json, table `parts`, column resource_mass; written by the profiling build's
// HarnessCapture2 C5 from DeltaVPartInfo, which reads Part.GetResourceMass()).
//
// Inputs (original game data only): the craft files of the capture (harness/inputs/Ships/{VAB,SPH}/*.craft; the PhysProf-M2 and
// PhysProf-L2-AS crafts only exist in analysis/physics/craft/; each file's SHA-256 is the capture's `craft.file_sha256`), the
// GameData of --gamedata. Part order is the craft file's PART order (ShipConstruct.parts), key (craft_id, part_index).
// Output: part_resource table with the capture's key and the columns craft_id, part_index, part, resource_mass.
#include <filesystem>
#include <string>
#include <vector>

#include "craft_tree.hpp"
#include "datasets.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/database/game_data.hpp"
#include "ksp/dotnet/text.hpp"
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

std::string GameRootOf(const std::string& gamedata) {
    const fs::path gd = fs::absolute(dotnet::PathFromUtf8(gamedata)).lexically_normal();
    fs::path p = gd;
    if (!p.has_filename()) p = p.parent_path();
    const std::u8string s = p.parent_path().u8string();
    return std::string(s.begin(), s.end());
}

int RunResourceMass(const RunnerOptions& opt) {
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
    g.dataset = "resource_mass";
    g.description = "kspcore candidate for the part resource masses of the C5 delta-v capture crafts (Part.GetResourceMass).";
    golden::Table t("parts", "Per-part Part.GetResourceMass() (tonnes) of the capture's crafts, in craft-file part order.");
    t.Col("craft_id", ColType::Str).Col("part_index", ColType::I64).Col("part", ColType::Str).Col("resource_mass", ColType::F32);
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
        for (size_t i = 0; i < tree.parts.size(); i++)
            t.Add({c.craftId, static_cast<int64_t>(i), tree.parts[i]->gameObjectName, tree.parts[i]->GetResourceMass()});
    }
    g.tables.push_back(std::move(t));

    ResourceSetup::database = nullptr;
    ResourceSetup::ResetConfig();
    SaveEnvironment::Reset();
    return WriteCandidate(opt, g, "resource_mass/resource_mass.json");
}

}  // namespace

KSP_GOLDEN_DATASET("resource_mass", 130, RunResourceMass);
