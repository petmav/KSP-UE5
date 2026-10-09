#include "ksp/dotnet/text.hpp"
#include "ksp/database/game_data.hpp"

#include <filesystem>

#include "ksp/physics/physics_globals.hpp"

namespace ksp {

GameData::GameData() = default;
GameData::~GameData() {
    if (physics && PhysicsGlobals::Instance() == physics.get()) PhysicsGlobals::SetInstance(nullptr);
}

void GameData::Load(const Options& options) {
    namespace fs = std::filesystem;
    const fs::path root = fs::absolute(dotnet::PathFromUtf8(options.gameRoot)).lexically_normal();
    const std::u8string r8 = root.u8string();
    std::string rootStr(r8.begin(), r8.end());
    while (!rootStr.empty() && (rootStr.back() == '\\' || rootStr.back() == '/')) rootStr.pop_back();

    GameDatabase::LoadOptions lo;
    lo.gameRoot = rootStr;
    lo.language = options.language;
    lo.translate = options.translate;
    if (!options.gameDataDir.empty()) lo.urlConfig = {io::ConfigDirectory{"", options.gameDataDir, io::DirectoryType::GameData}};
    database.Load(lo);

    if (!PhysicsGlobals::Instance()) {
        physics = std::make_unique<PhysicsGlobals>();
        physics->Awake(rootStr + "\\");
        physics->LoadDatabase();  // Physics.cfg (Start would SaveDatabase when missing: never write here)
        PhysicsGlobals::SetInstance(physics.get());
    }
    resources.LoadDefinitions(database);
    physicMaterials.LoadDefinitions(database);

    PartLoader::Options po;
    po.partDatabasePath = rootStr + "\\PartDatabase.cfg";
    parts.CompileAll(database, po);
}

}  // namespace ksp
