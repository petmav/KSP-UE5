// WP-05 part_database: the GameData tree, GameDatabase tables and PartLoader results vs the in-game capture C9
// (harness/captures/out2/part_database.json). Input: the real GameData (--gamedata, default
// <harness>/../decompiled/KSP-dev/GameData), read-only.
#include <filesystem>
#include <fstream>
#include <iostream>

#include "datasets.hpp"
#include "ksp/database/game_database.hpp"

namespace {

int RunPartDatabase(const RunnerOptions& opt) {
    namespace fs = std::filesystem;
    const fs::path gd = fs::absolute(fs::path(opt.gamedata)).lexically_normal();
    ksp::GameDatabase db;
    ksp::GameDatabase::LoadOptions lo;
    lo.gameRoot = gd.parent_path().string();
    lo.urlConfig = {ksp::io::ConfigDirectory{"", gd.string(), ksp::io::DirectoryType::GameData}};
    db.Load(lo);
    golden::MakeDirs(opt.out + "/part_database");
    std::ofstream log(opt.out + "/part_database/gamedatabase_load.log", std::ios::binary);
    for (const std::string& l : db.loadLog) log << l << "\n";
    std::cout << "part_database: " << db.databaseTexture.size() << " textures, " << db.databaseModel.size()
              << " models, " << db.databaseAudio.size() << " audio, " << db.root()->AllConfigs().size() << " configs\n";
    return 0;
}

}  // namespace

KSP_GOLDEN_DATASET("part_database", 120, RunPartDatabase);
