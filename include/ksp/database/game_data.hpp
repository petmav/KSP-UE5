// ksp::GameData: the data side of KSP's loading scene in one call (GameDatabase.cs:942 CreateDatabase followed by
// PartLoader.cs:161 CompileAll), reading the original GameData of a KSP install:
//
//   ksp::GameData gd;
//   gd.Load({.gameRoot = R"(C:\Projects\KSP-UE5\decompiled\KSP-dev)"});   // GameData = <gameRoot>\GameData
//   for (const auto& ap : gd.parts.loadedParts) ...                         // PartLoader.LoadedPartsList
//
// Steps: GameDatabase.Load (UrlDir, localisation, texture/model/audio tables, CompileConfig) -> PhysicsGlobals
// (prefab + <gameRoot>/Physics.cfg, only if no instance exists yet) -> PartResourceLibrary.LoadDefinitions ->
// PhysicMaterialLibrary.LoadDefinitions -> PartLoader.CompileAll (with <gameRoot>/PartDatabase.cfg). Experience traits,
// part upgrades,
// props and internal spaces are not compiled here (configs remain queryable through `database`).
// Nothing is ever written below gameRoot.
#pragma once

#include <memory>
#include <string>

#include "ksp/database/game_database.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/resources/part_resource_library.hpp"
#include "ksp/resources/physic_material_library.hpp"

namespace ksp {

class PhysicsGlobals;

class GameData {
public:
    struct Options {
        std::string gameRoot;            // KSP install directory (KSP: Application.dataPath + "/../")
        std::string gameDataDir;         // GameData directory; default <gameRoot>/GameData
        std::string language = "en-us";  // Localizer language
        bool translate = true;           // load the language's tags and translate every config (resets the Localizer)
    };
    GameData();
    ~GameData();
    GameData(const GameData&) = delete;
    GameData& operator=(const GameData&) = delete;

    void Load(const Options& options);

    GameDatabase database;
    PartResourceLibrary resources;
    PhysicMaterialLibrary physicMaterials;
    PartLoader parts;
    std::unique_ptr<PhysicsGlobals> physics;  // created when no PhysicsGlobals instance existed
};

}  // namespace ksp
