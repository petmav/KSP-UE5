// WP-05 unit tests: UrlDir rules, GameDatabase tables, PartLoader data side (ApplyPartValue's float32-for-double
// rule, crew seat mass reduction, DRAG_CUBE precedence and the nested-PART rule of PartDatabase.cfg, the node_* attach
// node forms, MODEL nodes vs the legacy first-model-in-directory rule, prefab parts). Built on a synthetic GameData
// tree written to a temporary directory, plus checks against the real GameData of the dev copy when it is present.
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

#include "doctest/doctest.h"
#include "ksp/database/game_data.hpp"
#include "ksp/database/game_database.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/io/url_dir.hpp"
#include "ksp/parts/part_loader.hpp"
#include "ksp/resources/part_resource_library.hpp"

namespace fs = std::filesystem;
using namespace ksp;

namespace {

uint32_t Bits(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}

void WriteFile(const fs::path& p, const std::string& text) {
    fs::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::binary);
    f << text;
}

std::string Dds(const char* fourCC) {
    std::string h(128, '\0');
    std::memcpy(&h[0], "DDS ", 4);
    h[4] = 124;
    std::memcpy(&h[84], fourCC, 4);
    return h;
}

const char* kCube = "Default, 1,2,3, 1,2,3, 1,2,3, 1,2,3, 1,2,3, 1,2,3, 0,0,0, 1,1,1";

// A small GameData tree exercising the loader rules.
struct SyntheticGame {
    fs::path root;
    SyntheticGame() {
        std::random_device rd;
        root = fs::temp_directory_path() / ("kspcore_wp05_" + std::to_string(rd()));
        const fs::path gd = root / "GameData";
        WriteFile(gd / "Test/Resources/res.cfg",
                  "RESOURCE_DEFINITION\n{\n name = LiquidFuel\n density = 0.005\n unitCost = 0.8\n flowMode = STACK_PRIORITY_SEARCH\n}\n"
                  "RESOURCE_DEFINITION\n{\n name = MonoPropellant\n density = 0.004\n unitCost = 1.2\n}\n");
        // legacy part: no MODEL node -> first model in the part's directory
        WriteFile(gd / "Test/Parts/PodA/podA.cfg",
                  "PART\n{\n name = pod_A\n module = Part\n author = me\n title = Pod A\n cost = 100\n entryCost = 1200\n"
                  " category = Pods\n mass = 0.8\n CrewCapacity = 1\n emissiveConstant = 0.7\n maxTemp = 1200\n"
                  " vesselType = Ship\n attachRules = 1,0,1,1,0\n"
                  " MODULE\n {\n  name = ModuleCommand\n }\n INTERNAL\n {\n  name = podIVA\n }\n}\n");
        WriteFile(gd / "Test/Parts/PodA/model.mu", "");
        WriteFile(gd / "Test/Parts/PodA/podA_tex.dds", Dds("DXT1"));
        // MODEL nodes + texture replacement, a cfg DRAG_CUBE, a resource, attach node forms
        WriteFile(gd / "Test/Parts/Tank/tank.cfg",
                  std::string("PART\n{\n name = tank\n module = Part\n mass = 0.5\n rescaleFactor = 2\n"
                              " node_stack_early = 0.0, 1.0, 0.0, 0.0, 1.0, 0.0\n"
                              " scale = 3\n"
                              " node_stack_top = 0.0, 1.0, 0.0, 0.0, 1.0, 0.0\n"
                              " node_stack_bottom = 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 2\n"
                              " node_stack_side = 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0, 1\n"
                              " node_stack_x = 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1, 0, 0\n"
                              " node_stack_y = 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 1, 0, 1, 1\n"
                              " node_dock_z = 0.5, 0.0, 0.0, 1.0, 0.0, 0.0, 3, 0, 1, 0, 0\n"
                              " node_stack_bad = 1, 2, 3\n"
                              " node_attach = 0.5, 0.0, 0.0, 1.0, 0.0, 0.0\n"
                              " MODEL\n {\n  model = Test/Parts/Shared/shared\n  texture = orig : Test/Parts/Shared/newtex\n }\n"
                              " MODEL\n {\n  model = Test/Parts/Shared/missing\n }\n"
                              " RESOURCE\n {\n  name = LiquidFuel\n  amount = 10\n  maxAmount = 20\n }\n"
                              " DRAG_CUBE\n {\n  cube = ") + "CfgCube" + std::string(kCube).substr(7) + "\n }\n}\n");
        WriteFile(gd / "Test/Parts/Shared/shared.mu", "");
        WriteFile(gd / "Test/Parts/Shared/orig.png", "png");
        WriteFile(gd / "Test/Parts/Shared/newtex.png", "png");
        // only in a NESTED PART of PartDatabase.cfg: gets no cubes
        WriteFile(gd / "Test/Parts/Nested/nested.cfg", "PART\n{\n name = nestedPart\n module = Part\n}\n");
        WriteFile(gd / "Test/Parts/Nested/nested.mu", "");
        // the prefab part kerbalEVA (applied onto PartLoader's code-built prefab)
        WriteFile(gd / "Test/Parts/Prebuilt/kerbalEVA.cfg", "PART\n{\n name = kerbalEVA\n crashTolerance = 50\n}\n");
        // skipped directories and a non-config extension
        WriteFile(gd / "Test/zDeprecated/old.cfg", "PART\n{\n name = old\n module = Part\n}\n");
        WriteFile(gd / "Test/PluginData/plugin.cfg", "PART\n{\n name = plug\n module = Part\n}\n");
        WriteFile(gd / "Test/Upper/UPPER.CFG", "PART\n{\n name = upper\n module = Part\n}\n");
        WriteFile(root / "PartDatabase.cfg",
                  std::string("PART_DATABASE\n{\n version = test-version\n}\n") +
                      "PART\n{\n url = Test/Parts/PodA/podA/pod_A\n DRAG_CUBE\n {\n  cube = " + kCube + "\n }\n"
                      " PART\n {\n  url = Test/Parts/Nested/nested/nestedPart\n  DRAG_CUBE\n  {\n   cube = " + kCube + "\n  }\n }\n}\n"
                      "PART\n{\n url = Test/Parts/Tank/tank/tank\n DRAG_CUBE\n {\n  cube = DbCube" + std::string(kCube).substr(7) + "\n }\n}\n");
    }
    ~SyntheticGame() {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
    std::string Root() const { return root.string(); }
};

struct LoadedSynthetic {
    SyntheticGame game;
    GameDatabase db;
    PartResourceLibrary lib;
    PartLoader loader;
    LoadedSynthetic() {
        GameDatabase::LoadOptions lo;
        lo.gameRoot = game.Root();
        lo.translate = false;
        db.Load(lo);
        lib.LoadDefinitions(db);
        PartLoader::Options po;
        po.partDatabasePath = (game.root / "PartDatabase.cfg").string();
        po.versionString = "test-version";
        po.usePhysicsGlobalsInstance = false;
        po.perSeatReduction = 0.045f;
        po.perCommandSeatReduction = 0.094f;
        loader.CompileAll(db, po);
    }
    const AvailablePart& Part(const char* name) const {
        const AvailablePart* p = loader.getPartInfoByName(name);
        REQUIRE(p != nullptr);
        return *p;
    }
};

}  // namespace

TEST_CASE("UrlDir: NTFS order, skipped directories, case-sensitive extensions") {
    CHECK(io::UrlDir::NtfsLess("B", "_a"));   // '_' (0x5F) sorts after upper-cased letters
    CHECK(io::UrlDir::NtfsLess("a", "B"));    // case-insensitive
    CHECK(io::UrlDir::NtfsLess("ab", "abc"));
    SyntheticGame g;
    GameDatabase db;
    GameDatabase::LoadOptions lo;
    lo.gameRoot = g.Root();
    lo.translate = false;
    db.Load(lo);
    // the GameData UrlDir is named "" (urlRoot): GetDirectory / GetFile from the root never descend into it (sic;
    // GetConfig does, UrlDir.cs:803), so look up from the GameData directory itself
    CHECK(db.root()->GetDirectory("Test") == nullptr);
    io::UrlDir* gd = db.root()->children().at(0).get();
    CHECK(gd->name().empty());
    CHECK(gd->GetDirectory("Test/Upper") != nullptr);
    CHECK(gd->GetDirectory("Test/zDeprecated") == nullptr);
    CHECK(gd->GetDirectory("Test/PluginData") == nullptr);
    std::vector<std::string> partUrls;
    for (io::UrlConfig* c : db.GetConfigs("PART")) partUrls.push_back(c->url());
    // "UPPER.CFG" is not a config (extension compare is case-sensitive)
    CHECK(partUrls == std::vector<std::string>{"Test/Parts/Nested/nested/nestedPart", "Test/Parts/PodA/podA/pod_A",
                                               "Test/Parts/Prebuilt/kerbalEVA/kerbalEVA", "Test/Parts/Tank/tank/tank"});
    // UrlDir.GetFile treats the last url element as a config name: the file is element urlDepth - 1 (UrlDir.cs:742)
    CHECK(gd->GetFile("Test/Upper/UPPER") == nullptr);
    io::UrlFile* upper = gd->GetFile("Test/Upper/UPPER/anything");
    REQUIRE(upper != nullptr);
    CHECK(upper->fileType() == io::FileType::Unknown);
    CHECK(upper->fileExtension() == "CFG");
    // texture / model tables (url = directory url + file name without extension)
    CHECK(db.ExistsTexture(std::string_view("test/parts/shared/ORIG")));  // OrdinalIgnoreCase
    CHECK(db.GetTextureInfo(std::string_view("Test/Parts/PodA/podA_tex"))->loader == "DDS");
    CHECK(db.GetModelPrefab("Test/Parts/Shared/shared") != nullptr);
    CHECK(db.GetModelPrefabIn("Test/Parts/PodA")->name == "Test/Parts/PodA/model");
    CHECK(db.GetConfigNode("Test/Parts/PodA/podA/pod_A")->GetValue("title") == std::optional<std::string>("Pod A"));
}

TEST_CASE("PartLoader.ApplyPartValue: double fields are parsed with float.Parse") {
    PartPrefab p;
    CHECK(PartLoader::ApplyPartValue(p, ConfigNode::Value("emissiveConstant", "0.7")));
    CHECK(p.fields.emissiveConstant == static_cast<double>(0.7f));
    CHECK(dotnet::FormatDoubleR(p.fields.emissiveConstant) == "0.699999988079071");
    CHECK(PartLoader::ApplyPartValue(p, ConfigNode::Value("maxTemp", "2400")));
    CHECK(p.fields.maxTemp == 2400.0);
    CHECK(PartLoader::ApplyPartValue(p, ConfigNode::Value("heatConductivity", "0.12")));
    CHECK(p.fields.heatConductivity == static_cast<double>(0.12f));
    CHECK(p.fields.heatConductivity != 0.12);
    // float fields go through float.Parse via the generic Parse(string) branch; Vector3 / Quaternion / enums / bool
    CHECK(PartLoader::ApplyPartValue(p, ConfigNode::Value("mass", "0.8")));
    CHECK(Bits(p.fields.mass) == Bits(0.8f));
    CHECK(PartLoader::ApplyPartValue(p, ConfigNode::Value("CoMOffset", "0.0, 0.5, 0.0")));
    CHECK(p.fields.CoMOffset.y == 0.5f);
    CHECK(PartLoader::ApplyPartValue(p, ConfigNode::Value("fuelCrossFeed", "False")));
    CHECK(p.fields.fuelCrossFeed == false);
    CHECK(PartLoader::ApplyPartValue(p, ConfigNode::Value("dragModelType", "override")));
    CHECK(p.fields.dragModel == 5);  // NONE
    CHECK(PartLoader::ApplyPartValue(p, ConfigNode::Value("attachRules", "1,0,1,1,0")));
    REQUIRE(p.fields.attachRules.has_value());
    CHECK(p.fields.attachRules->String() == "1,0,1,1,0,0,0");
    CHECK(p.fields.attachRules->allowRoot);  // fewer than 8 values: allowRoot = true
    // not a Part field: consumed with a warning; special names are left to ParsePart
    CHECK(PartLoader::ApplyPartValue(p, ConfigNode::Value("noSuchField", "1")));
    CHECK_FALSE(PartLoader::ApplyPartValue(p, ConfigNode::Value("node_stack_top", "0,0,0,0,1,0")));
    CHECK_FALSE(PartLoader::ApplyPartValue(p, ConfigNode::Value("scale", "1")));
    CHECK_THROWS_AS(PartLoader::ApplyPartValue(p, ConfigNode::Value("maxTemp", "hot")), dotnet::FormatException);
}

TEST_CASE("PartLoader: synthetic GameData") {
    LoadedSynthetic L;
    // loaded list: the 9 prefab parts first, then the compiled configs in GameDatabase order
    REQUIRE(L.loader.loadedParts.size() == 9 + 3);
    CHECK(L.loader.loadedParts[0]->name == "kerbalEVA");
    CHECK(L.loader.loadedParts[9]->name == "nestedPart");
    CHECK(L.loader.loadedParts[10]->name == "pod.A");  // '_' -> '.'
    CHECK(L.loader.loadedParts[11]->name == "tank");

    SUBCASE("crew seat mass reduction (ModuleCommand: PerCommandSeatReduction per seat)") {
        const AvailablePart& pod = L.Part("pod.A");
        // mass = (float)((double)0.8f - (double)(float)(0.094f * 1)): mk1pod.v2's 0.8 -> 0.706 (0x3F34BC6B in C9)
        CHECK(Bits(pod.partPrefab->fields.mass) == 0x3F34BC6Bu);
        CHECK(Bits(pod.partPrefab->fields.partSeatMassReduction) == Bits(0.094f));
        CHECK(pod.partPrefab->fields.emissiveConstant == static_cast<double>(0.7f));
        CHECK(pod.partPrefab->fields.skinMaxTemp == 1200.0);  // skinMaxTemp <= 0 -> maxTemp
    }
    SUBCASE("LoadObjectFromConfig strips the AvailablePart keys from partConfig") {
        const AvailablePart& pod = L.Part("pod.A");
        CHECK(pod.title == "Pod A");
        CHECK(pod.cost == 100.0f);
        CHECK(pod.entryCost == 1200);
        CHECK(kEnum_PartCategories.ToString(pod.category) == "Pods");
        for (const char* k : {"title", "cost", "entryCost", "category", "author", "name"})
            CHECK_FALSE(pod.partConfig->HasValue(k));
        CHECK(pod.partConfig->HasValue("mass"));
        CHECK(pod.partConfig->HasValue("module"));
        CHECK(pod.showVesselNaming);  // ModuleCommand
        // vesselType Ship > Unknown -> ModuleTripLogger appended
        REQUIRE(pod.partPrefab->Modules.size() == 2);
        CHECK(pod.partPrefab->Modules[1].moduleName == "ModuleTripLogger");
        CHECK(pod.partPrefab->Modules[1].addedInCode);
        CHECK(pod.internalConfig->GetValue("name") == std::optional<std::string>("podIVA"));
    }
    SUBCASE("drag cubes: part cfg DRAG_CUBE wins, PartDatabase.cfg otherwise, nested PART ignored") {
        const AvailablePart& tank = L.Part("tank");
        REQUIRE(tank.partPrefab->DragCubes.Cubes().size() == 1);
        CHECK(tank.partPrefab->DragCubes.Cubes()[0].Name() == "CfgCube");
        const AvailablePart& pod = L.Part("pod.A");
        REQUIRE(pod.partPrefab->DragCubes.Cubes().size() == 1);
        CHECK(pod.partPrefab->DragCubes.Cubes()[0].Name() == "Default");
        CHECK(pod.partPrefab->DragCubes.Cubes()[0].SaveToString() == kCube);
        CHECK(L.Part("nestedPart").partPrefab->DragCubes.Cubes().empty());
        CHECK(L.loader.cubesNeedRendering == std::vector<std::string>{"nestedPart"});
    }
    SUBCASE("attach node forms") {
        const PartPrefab& t = *L.Part("tank").partPrefab;
        // rescaleFactor 2; `scale = 3` changes the factor only for the node values AFTER it (value order)
        REQUIRE(t.attachNodes.size() == 7);
        const AttachNode& early = t.attachNodes[0];
        CHECK(early.id == "early");
        CHECK(early.position.y == 2.0f);
        CHECK(early.size == 1);
        CHECK(early.attachMethod == AttachNodeMethod::FIXED_JOINT);
        const AttachNode& top = t.attachNodes[1];
        CHECK(top.position.y == 6.0f);
        CHECK(top.orientation.y == 6.0f);  // orientation is scaled too (sic)
        CHECK(Bits(t.fields.scaleFactor) == Bits(3.0f / 2.0f));
        CHECK(t.attachNodes[2].size == 2);                                     // 7 values
        CHECK(t.attachNodes[3].attachMethod == AttachNodeMethod::HINGE_JOINT);  // 8 values
        CHECK(t.attachNodes[3].size == 0);
        CHECK_FALSE(t.attachNodes[4].ResourceXFeed);                           // 9 values
        CHECK(t.attachNodes[5].rigid);                                         // 10 values
        const AttachNode& dock = t.attachNodes[6];                             // 11 values, dock
        CHECK(dock.id == "z");
        CHECK(dock.nodeType == AttachNodeType::Dock);
        CHECK(dock.size == 3);
        CHECK_FALSE(dock.AllowOneWayXFeed);
        REQUIRE(t.srfAttachNode.has_value());
        CHECK(t.srfAttachNode->id == "srfAttach");
        CHECK(t.srfAttachNode->nodeType == AttachNodeType::Surface);
        CHECK(t.srfAttachNode->attachMethod == AttachNodeMethod::HINGE_JOINT);
        CHECK(t.srfAttachNode->position.x == 3.0f);
        // a part without node_attach still has a default srfAttachNode (Unity serializer instance)
        const PartPrefab& n = *L.Part("nestedPart").partPrefab;
        REQUIRE(n.srfAttachNode.has_value());
        CHECK(n.srfAttachNode->id.empty());
        CHECK(n.srfAttachNode->radius == 0.4f);
    }
    SUBCASE("MODEL nodes vs the legacy first-model rule; asset paths") {
        const AvailablePart& tank = L.Part("tank");
        REQUIRE(tank.partPrefab->models.size() == 1);  // the missing model is skipped (error logged)
        const PartModelRef& m = tank.partPrefab->models[0];
        CHECK(m.fromModelNode);
        CHECK(m.url == "Test/Parts/Shared/shared");
        REQUIRE(m.textureReplacements.size() == 1);
        CHECK(m.textureReplacements[0].original == "orig");
        CHECK(m.textureReplacements[0].url == "Test/Parts/Shared/newtex");
        REQUIRE(tank.assets.size() == 4);
        CHECK(tank.assets[0].kind == "cfg");
        CHECK(tank.assets[0].file == "GameData/Test/Parts/Tank/tank.cfg");
        CHECK(tank.assets[1].slot == "MODEL[0]");
        CHECK(tank.assets[1].file == "GameData/Test/Parts/Shared/shared.mu");
        CHECK(tank.assets[2].kind == "texture_replacement");
        CHECK(tank.assets[2].file == "GameData/Test/Parts/Shared/newtex.png");
        CHECK(tank.assets[3].slot == "MODEL[1]");
        CHECK(tank.assets[3].source == "unresolved");
        const AvailablePart& pod = L.Part("pod.A");
        REQUIRE(pod.partPrefab->models.size() == 1);
        CHECK_FALSE(pod.partPrefab->models[0].fromModelNode);
        CHECK(pod.partPrefab->models[0].url == "Test/Parts/PodA/model");
        CHECK(pod.assets[1].slot == "mesh (first model in Test/Parts/PodA)");
        CHECK(pod.assets[1].file == "GameData/Test/Parts/PodA/model.mu");
        CHECK(pod.assets.back().kind == "internal");
        CHECK(pod.fileTimes.size() == 2);  // cfg + model (AddFileTime)
    }
    SUBCASE("resources and the resource library") {
        const PartPrefab& t = *L.Part("tank").partPrefab;
        REQUIRE(t.Resources.size() == 1);
        CHECK(t.Resources[0].amount == 10.0);
        CHECK(t.Resources[0].maxAmount == 20.0);
        CHECK(t.Resources[0].flowState);
        CHECK(t.Resources[0].flowMode == PartResourceFlowMode::Both);
        const PartResourceDefinition* lf = L.lib.GetDefinition("LiquidFuel");
        REQUIRE(lf != nullptr);
        CHECK(lf->density() == 0.005f);
        CHECK(kEnum_ResourceFlowMode.ToString(lf->resourceFlowMode()) == "STACK_PRIORITY_SEARCH");
        CHECK(lf->displayName() == "Liquid Fuel");  // KSPUtil.PrintModuleName
        CHECK(lf->id() == MonoStringHash("LiquidFuel"));
    }
    SUBCASE("prefab parts: cfg applied onto kerbalEVA; the others never compiled") {
        const AvailablePart& eva = *L.loader.loadedParts[0];
        CHECK(eva.codeBuilt);
        CHECK(eva.prefabCompiled);
        CHECK(eva.loadError.empty());
        CHECK(eva.partPrefab->fields.crashTolerance == 50.0f);
        for (size_t i = 1; i < 9; i++) {
            CHECK_FALSE(L.loader.loadedParts[i]->prefabCompiled);
            CHECK(L.loader.loadedParts[i]->loadError ==
                  "NullReferenceException: Object reference not set to an instance of an object");
        }
    }
}

TEST_CASE("DragCube: load / save round trip and the 24-value quirk") {
    DragCubeList l;
    ConfigNode n("DRAG_CUBE");
    n.AddValue("cube", "A, 1,2,3, 4,5,6, 7,8,9, 1,2,3, 4,5,6, 7,8,9, 0.5,0.25,0, 1.5,2.5,3.5");
    n.AddValue("cube", "B, 1,2,3, 4,5,6, 7,8,9, 1,2,3, 4,5,6, 7,8,9, 0.5,0.25,0, 1.5,2.5,3.5");
    REQUIRE(l.LoadCubes(n));
    REQUIRE(l.Cubes().size() == 2);
    CHECK(l.Cubes()[0].Name() == "B");  // read last to first
    const ConfigNode::NodePtr saved = l.SaveCubes();
    CHECK(saved->GetValues("cube") == n.GetValues("cube"));  // written last to first again
    ConfigNode bad("DRAG_CUBE");
    bad.AddValue("cube", "1,2,3, 4,5,6, 7,8,9, 1,2,3, 4,5,6, 7,8,9, 0.5,0.25,0, 1.5,2.5,3.5");  // 24 values, no name
    CHECK_THROWS_AS(l.LoadCubes(bad), std::out_of_range);
}

TEST_CASE("PartLoader on the real GameData (decompiled/KSP-dev)") {
    const fs::path root = fs::path(KSPCORE_PROJECT_ROOT) / "decompiled" / "KSP-dev";
    if (!fs::exists(root / "GameData" / "Squad")) {
        MESSAGE("decompiled/KSP-dev/GameData not present: skipped");
        return;
    }
    GameData data;
    GameData::Options o;
    o.gameRoot = root.string();
    o.translate = false;
    data.Load(o);
    const AvailablePart* pod = data.parts.getPartInfoByName("mk1pod.v2");
    REQUIRE(pod != nullptr);
    CHECK(Bits(pod->partPrefab->fields.mass) == 0x3F34BC6Bu);  // 0.8 -> 0.706 (C9)
    CHECK(data.parts.loadedParts.size() == 362);
    CHECK(data.parts.cubesNeedRendering.empty());
    int errors = 0;
    for (const auto& ap : data.parts.loadedParts) errors += !ap->loadError.empty();
    CHECK(errors == 4);  // the DLC suits
    // no PHYSICMATERIAL_DEFINITION in stock: the code-built Default
    REQUIRE(data.physicMaterials.DefaultMaterial != nullptr);
    CHECK(data.physicMaterials.physicMaterials.empty());
    CHECK(data.physicMaterials.DefaultMaterial->dynamicFriction() == 0.4f);
    CHECK(data.physicMaterials.DefaultMaterial->displayName() == "#autoLOC_6011085");
    CHECK(data.resources.resourceDefinitions.Count() == 10);  // the 10 "Resource X added to database" lines of KSP.log
}
