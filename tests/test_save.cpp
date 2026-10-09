// WP-04 unit tests: GameParameters defaults and presets (GameParameters.cs constants, and the PARAMETERS / SCENARIO
// blocks of the original physprofile save), the save upgrade pipeline on synthetic old saves and crafts, save model
// quirks, System.Version / Enum.Parse, GamePersistence naming and backups.
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "ksp/config/config_node.hpp"
#include "ksp/dotnet/enum_parse.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/dotnet/text.hpp"
#include "ksp/dotnet/version.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_persistence.hpp"
#include "ksp/save/save_environment.hpp"
#include "ksp/save/ship_construct.hpp"
#include "ksp/save/upgrade_pipeline.hpp"
#include "ksp/util/ksp_util.hpp"

using namespace ksp;
using SaveUpgradePipeline::LoadContext;
namespace fs = std::filesystem;

namespace {

const std::string kProjectRoot = KSPCORE_PROJECT_ROOT;
const std::string kSaves = kProjectRoot + "/decompiled/KSP-dev/saves";

void FreshEnvironment() {
    SaveEnvironment::Reset();
    SaveEnvironment& env = SaveEnvironment::Current();
    env.newGuidHashCode = [] {
        static uint32_t s = 12345u;
        s = s * 1664525u + 1013904223u;
        return static_cast<int32_t>(s);
    };
    env.tickCount = [] { return 0; };
    // MainMenu.Start: GetDefaultParameters sorts the custom parameter types before any game is loaded
    GameParameters::GetDefaultParameters(Game_Modes::CAREER, GameParameters::Preset::Normal);
}

ConfigNode::NodePtr Parse(const std::string& text) { return ConfigNode::Parse(text); }

float F(const std::string& s) { return dotnet::ParseSingle(s); }

Vector3f V(const std::string& s) { return KSPUtil::ParseVector3(s); }

}  // namespace

// ======================================================= dotnet ======================================================

TEST_CASE("System.Version: components, comparison, ToString(fieldCount), parse errors") {
    using dotnet::Version;
    CHECK(Version("1.2") < Version("1.2.0"));  // Build -1 < 0
    CHECK(Version("1.12.5") > Version("1.12.2"));
    CHECK(Version(1, 12, 0) == Version("1.12.0"));
    CHECK(Version("1.12.5").ToString() == "1.12.5");
    CHECK(Version("0.21").ToString() == "0.21");
    CHECK(Version("1.2.3.4").ToString(3) == "1.2.3");
    CHECK_THROWS_AS(Version("1.2").ToString(3), ArgumentException);
    CHECK_THROWS_AS(Version("1"), ArgumentException);
    CHECK_THROWS_AS(Version("1.x.0"), dotnet::FormatException);
    CHECK_THROWS_AS(Version("1.-2.0"), dotnet::ArgumentOutOfRangeException);
    CHECK(Version(" 1. 2 .3").ToString() == "1.2.3");  // int.TryParse with NumberStyles.Integer trims each part
}

TEST_CASE("Enum.Parse full semantics (names, numbers, flags, whitespace)") {
    CHECK(dotnet::EnumParseFull<VesselSituations>(" ORBITING ") == VesselSituations::ORBITING);
    CHECK(static_cast<int>(dotnet::EnumParseFull<VesselSituations>("5")) == 5);  // any number, defined or not
    CHECK(dotnet::EnumParseFull<KSPActionGroup>("Stage, Gear") ==
          static_cast<KSPActionGroup>(static_cast<int>(KSPActionGroup::Stage) | static_cast<int>(KSPActionGroup::Gear)));
    CHECK(dotnet::EnumParseFull<KSPActionGroup>("-1") == KSPActionGroup::REPLACEWITHDEFAULT);
    CHECK_THROWS_AS(dotnet::EnumParseFull<KSPActionGroup>("Stage,Bogus"), ArgumentException);
    CHECK_THROWS_AS(dotnet::EnumParseFull<VesselType>(""), ArgumentException);
    CHECK_THROWS_AS(dotnet::EnumParseFull<VesselType>("ship"), ArgumentException);  // case-sensitive
}

// =================================================== GameParameters ==================================================

TEST_CASE("GameParameters defaults = GameParameters.cs field initialisers") {
    FreshEnvironment();
    GameParameters p;
    CHECK(p.preset == GameParameters::Preset::Normal);
    CHECK(p.Flight.CanQuickSave);
    CHECK(p.Flight.CanLeaveToMainMenu);
    CHECK(p.Editor.CanLeaveToMainMenu == false);
    CHECK(p.Editor.startUpMode == 0);
    CHECK(p.Editor.craftFileToLoad.empty());
    CHECK(p.TrackingStation.CanLeaveToMainMenu == false);
    CHECK(p.SpaceCenter.CanGoToRnD);
    CHECK(p.Difficulty.AutoHireCrews == false);
    CHECK(p.Difficulty.MissingCrewsRespawn);
    CHECK(p.Difficulty.RespawnTimer == 7200.0f);  // (float)GameSettings.DEFAULT_KERBAL_RESPAWN_TIMER
    CHECK(p.Difficulty.BypassEntryPurchaseAfterResearch);
    CHECK(p.Difficulty.AllowStockVessels == false);
    CHECK(p.Difficulty.ResourceAbundance == 1.0f);
    CHECK(p.Difficulty.ReentryHeatScale == 1.0f);
    CHECK(p.Difficulty.EnableCommNet == false);
    CHECK(p.Career.TechTreeUrl == "GameData/Squad/Resources/TechTree.cfg");
    CHECK(p.Career.StartingFunds == 25000.0f);
    CHECK(p.Career.RepLossDeclined == 1.0f);
    auto& adv = p.CustomParams<GameParameters::AdvancedParams>();
    CHECK(adv.KerbalGToleranceMult == 1.0f);
    CHECK(adv.BuildingImpactDamageMult == 0.05f);
    CHECK(adv.PartUpgradesInCareer);
    CHECK(adv.GetEnableKerbalExperience());  // unset, no current game -> true
    auto& cn = p.CustomParams<CommNet::CommNetParams>();
    CHECK(cn.occlusionMultiplierVac == 0.9f);
    CHECK(cn.occlusionMultiplierAtm == 0.75f);
    CHECK(cn.enableGroundStations);
    auto& mg = p.CustomParams<Expansions::Missions::MissionParamsGeneral>();
    CHECK(mg.startingFunds == 100000.0f);
    CHECK(mg.kerbalLevelPilot == 4);
    CHECK(p.CustomParams<Expansions::Missions::MissionParamsExtras>().astronautHiresAreFree);
    CHECK(p.CustomParams<Expansions::Missions::MissionParamsFacilities>().facilityLevelVAB == 3);
}

TEST_CASE("GetDefaultParameters(SANDBOX, Normal) writes the physprofile save's PARAMETERS block byte for byte") {
    FreshEnvironment();
    ConfigNode::NodePtr save = ConfigNode::Load(kSaves + "/physprofile/persistent.sfs");
    REQUIRE(save);
    ConfigNode* params = save->GetNode("GAME")->GetNode("PARAMETERS");
    REQUIRE(params);
    auto p = GameParameters::GetDefaultParameters(Game_Modes::SANDBOX, GameParameters::Preset::Normal);
    ConfigNode out("PARAMETERS");
    p->Save(out);
    CHECK(out.ToString() == params->ToString());
}

TEST_CASE("Custom parameter order: first GameParameters vs later ones (static ParameterTypes sort)") {
    FreshEnvironment();
    GameParameters::ResetParameterTypes();  // the state before the first construction (Assembly.GetTypes order)
    GameParameters first;
    std::vector<std::string> a;
    for (const auto& [n, _] : first.CustomParamsInOrder()) a.push_back(n);
    CHECK(a == std::vector<std::string>{"AdvancedParams", "CommNetParams", "MissionParamsGeneral",
                                        "MissionParamsFacilities", "MissionParamsExtras"});
    GameParameters later;
    std::vector<std::string> b;
    for (const auto& [n, _] : later.CustomParamsInOrder()) b.push_back(n);
    CHECK(b == std::vector<std::string>{"CommNetParams", "AdvancedParams", "MissionParamsGeneral",
                                        "MissionParamsExtras", "MissionParamsFacilities"});
}

TEST_CASE("Difficulty presets (GameParameters.cs:759 SetDifficultyPresets + SetDifficultyPreset overrides)") {
    FreshEnvironment();
    using P = GameParameters::Preset;
    auto easy = GameParameters::GetDefaultParameters(Game_Modes::CAREER, P::Easy);
    CHECK(easy->preset == P::Easy);
    CHECK(easy->Difficulty.AllowStockVessels);
    CHECK(easy->Difficulty.IndestructibleFacilities);
    CHECK(easy->Difficulty.ResourceAbundance == 1.2f);
    CHECK(easy->Difficulty.ReentryHeatScale == 0.5f);
    CHECK(easy->Difficulty.EnableCommNet == false);
    CHECK(easy->Difficulty.AllowOtherLaunchSites == false);  // career branch
    CHECK(easy->Career.StartingFunds == 250000.0f);
    CHECK(easy->Career.RepLossDeclined == 0.0f);
    CHECK(easy->CustomParams<GameParameters::AdvancedParams>().BuildingImpactDamageMult == 0.03f);
    CHECK(easy->CustomParams<GameParameters::AdvancedParams>().ImmediateLevelUp);
    CHECK(easy->CustomParams<GameParameters::AdvancedParams>().GetEnableKerbalExperience());
    CHECK(easy->CustomParams<CommNet::CommNetParams>().rangeModifier == 1.5f);
    CHECK(easy->CustomParams<CommNet::CommNetParams>().occlusionMultiplierVac == 0.0f);
    CHECK(easy->CustomParams<Expansions::Missions::MissionParamsGeneral>().startingFunds == 250000.0f);
    CHECK(easy->CustomParams<Expansions::Missions::MissionParamsExtras>().cheatsEnabled);

    auto normal = GameParameters::GetDefaultParameters(Game_Modes::SCIENCE_SANDBOX, P::Normal);
    CHECK(normal->Difficulty.EnableCommNet);  // set on the Normal preset after DifficultyPresets.Add (same object)
    CHECK(normal->Difficulty.AllowOtherLaunchSites);
    CHECK(normal->CustomParams<GameParameters::AdvancedParams>().GetEnableKerbalExperience() == false);

    auto moderate = GameParameters::GetDefaultParameters(Game_Modes::CAREER, P::Moderate);
    CHECK(moderate->Difficulty.MissingCrewsRespawn == false);
    CHECK(moderate->Difficulty.ResourceAbundance == 0.8f);
    CHECK(moderate->Career.StartingFunds == 15000.0f);
    CHECK(moderate->Career.FundsLossMultiplier == 1.5f);
    CHECK(moderate->CustomParams<GameParameters::AdvancedParams>().AllowNegativeCurrency);
    CHECK(moderate->CustomParams<CommNet::CommNetParams>().occlusionMultiplierAtm == 0.85f);
    CHECK(moderate->CustomParams<Expansions::Missions::MissionParamsFacilities>().facilityLevelAdmin == 2);

    auto hard = GameParameters::GetDefaultParameters(Game_Modes::SANDBOX, P::Hard);
    CHECK(hard->Difficulty.AllowStockVessels == false);  // sandbox: p != Hard
    CHECK(hard->Flight.CanRestart == false);
    CHECK(hard->Flight.CanQuickLoad == false);
    CHECK(hard->Career.RepLossDeclined == 3.0f);
    CHECK(hard->CustomParams<GameParameters::AdvancedParams>().BuildingImpactDamageMult == 0.2f);
    CHECK(hard->CustomParams<CommNet::CommNetParams>().rangeModifier == 0.65f);
    CHECK(hard->CustomParams<Expansions::Missions::MissionParamsGeneral>().preventVesselRecovery);
    CHECK(hard->CustomParams<Expansions::Missions::MissionParamsExtras>().astronautHiresAreFree == false);

    auto mission = GameParameters::GetDefaultParameters(Game_Modes::MISSION, P::Custom);
    CHECK(mission->preset == P::Custom);
    CHECK(mission->SpaceCenter.CanGoToRnD == false);
    CHECK(mission->SpaceCenter.CanGoToAdmin == false);
    CHECK(mission->Difficulty.BypassEntryPurchaseAfterResearch);
    CHECK(mission->CustomParams<Expansions::Missions::MissionParamsFacilities>().facilityLevelVAB == 3);  // Custom: no change
}

TEST_CASE("AdvancedParams.EnableKerbalExperience follows HighLogic.CurrentGame when unset; OnLoad") {
    FreshEnvironment();
    Game career;
    career.Mode = Game_Modes::CAREER;
    Game sandbox;
    sandbox.Mode = Game_Modes::SANDBOX;
    GameParameters p;
    auto& adv = p.CustomParams<GameParameters::AdvancedParams>();
    SaveEnvironment::Current().currentGame = &sandbox;
    CHECK(adv.GetEnableKerbalExperience() == false);
    SaveEnvironment::Current().currentGame = &career;
    CHECK(adv.GetEnableKerbalExperience());
    CHECK(adv.KerbalExperienceEnabled(Game_Modes::MISSION));
    // OnLoad without the value: EnableKerbalExperience = (CurrentGame.Mode == CAREER)
    SaveEnvironment::Current().currentGame = &sandbox;
    GameParameters loaded(*Parse("PARAMETERS\n{\nAdvancedParams\n{\nGKerbalLimits = True\n}\n}")->GetNode("PARAMETERS"));
    CHECK(loaded.CustomParams<GameParameters::AdvancedParams>().enableKerbalExperience == std::optional<bool>(false));
    CHECK(loaded.CustomParams<GameParameters::AdvancedParams>().GKerbalLimits);
    SaveEnvironment::Current().currentGame = nullptr;
    // a value naming a method is a NullReferenceException in KSP (GetMember -> MethodInfo; field.FieldType)
    CHECK_THROWS_AS(GameParameters(*Parse("P\n{\nFLIGHT\n{\nToString = 1\n}\n}")->GetNode("P")), NullReferenceException);
    // unknown custom node: dropped with a warning; read-only property ignored
    GameParameters other(*Parse("P\n{\nModParams\n{\nx = 1\n}\nAdvancedParams\n{\nTitle = x\n}\n}")->GetNode("P"));
    ConfigNode out("P");
    other.Save(out);
    CHECK(out.GetNode("ModParams") == nullptr);
}

// ================================================= upgrade pipeline =================================================

TEST_CASE("Upgrade pipeline: stock scripts sorted by (TargetVersion, EarliestCompatibleVersion), stable") {
    FreshEnvironment();
    SaveUpgradePipeline::SaveUpgradePipeline pipe;
    pipe.Init(SaveUpgradePipeline::CreateStockUpgradeScripts());
    REQUIRE(pipe.upgradeScripts.size() == 40);
    std::vector<std::string> names;
    for (const auto& s : pipe.upgradeScripts) names.push_back(s->TypeName());
    CHECK(names[0] == "v110_LandingGearOffset");
    CHECK(names[3] == "v110_WheelModuleOverhaul");
    CHECK(names[4] == "v170_fuelTank1_2");
    CHECK(names[16] == "v170_toroidalFuelTank");
    CHECK(names[17] == "v180_ModuleAeroSurface");
    CHECK(names[18] == "v180_ModuleControlSurface");
    CHECK(names[19] == "probeCoreSphere");
    CHECK(names[20] == "v1120_engineLargeSkipper");
    CHECK(names[38] == "v1120_stackTriCoupler");
    CHECK(names[39] == "v1123_dockingNode");
    // PartReplace.OnInit registers PartLoader.AddPartReplacement for the 33 replacements
    CHECK(SaveEnvironment::Current().partReplacements.size() == 33);
    CHECK(pipe.upgradeScripts[1]->Name() == "1.1 LT1 Legs Offset\t\t");
}

TEST_CASE("Upgrade pipeline on a synthetic 1.0.0 save: iterations, version stamps, upgraded markers, part edits") {
    FreshEnvironment();
    // a 1.0.0 save with a pre-1.12 mk1pod (PartReplace), a SmallGearBay (PartOffset) and an LT-1 leg (PartOffset with
    // rotation) as children of the pod
    ConfigNode::NodePtr root = Parse(
        "GAME\n{\nversion = 1.0.0\nFLIGHTSTATE\n{\nversion = 1.0.0\nVESSEL\n{\nname = Test\n"
        "PART\n{\nname = mk1pod\nparent = 0\nposition = 0,1,0\nrotation = 0,0,0,1\nrTrf = mk1pod\n}\n"
        "PART\n{\nname = SmallGearBay\nparent = 0\nposition = 0,1,0\nrotation = 0,0,0,1\n}\n"
        "PART\n{\nname = landingLeg1\nparent = 0\nposition = 1,0,0\nrotation = 0,0,0,1\n}\n}\n}\n}\n");
    SaveUpgradePipeline::SaveUpgradePipeline pipe;
    pipe.Init(SaveUpgradePipeline::CreateStockUpgradeScripts());
    bool ok = false;
    std::string info;
    ConfigNode::NodePtr up = pipe.Run(root, LoadContext::SFS, dotnet::Version(1, 12, 5), ok, info);
    REQUIRE(ok);
    CHECK(up != root);  // the upgraded copy; the source tree is untouched
    CHECK(root->GetNode("GAME")->GetValue("version") == "1.0.0");
    // 1.0.0 -> (gear, legs, mk1pod upgraded; v1123 TooEarly) 1.1.0 -> (v1123 still TooEarly) 1.12.0 -> all pass
    CHECK(up->GetNode("GAME")->GetValue("version") == "1.12.0");
    CHECK(pipe.runLog.size() == 3);
    CHECK(info.rfind("Original File Version: 1.0.0\nUpgrades Completed. 3 iterations. Runtime: ", 0) == 0);
    // the scripts run from the end of the sorted list; each appends "upgraded = Name // Description" to the ROOT
    const std::vector<std::string> upgraded = up->GetValues("upgraded");
    REQUIRE(upgraded.size() == 3);
    CHECK(upgraded[0] == "1.12 Mk1 Command Pod replacement");
    CHECK(upgraded[1] == "1.1 LT1 Legs Offset  ");  // tabs -> spaces (CleanupInput)
    CHECK(upgraded[2] == "1.1 SmallGearBay Offset");
    CHECK(up->values()[0].comment == std::optional<std::string>("Replaces the old Mk1 Command Pod for the new Mk1 Command Pod."));
    const auto parts = up->GetNode("GAME")->GetNode("FLIGHTSTATE")->GetNode("VESSEL")->GetNodes("PART");
    CHECK(parts[0]->GetValue("name") == "mk1pod.v2");
    CHECK(parts[0]->GetValue("rTrf") == "mk1pod.v2");
    // SmallGearBay: position0 + rotation0 * (0, 0.1, 0)
    const Vector3f gear = V(*parts[1]->GetValue("position"));
    CHECK(gear.y == 1.0f + 0.1f);
    // LT-1: rotation0 * (AngleAxis(-90, up) * AngleAxis(-22, right)); position0 + rotation0 * (0.15, 0, 0)
    const Quaternionf leg = KSPUtil::ParseQuaternion(*parts[2]->GetValue("rotation"));
    CHECK(std::fabs(leg.x - (-0.1349220f)) < 1e-5f);
    CHECK(std::fabs(leg.y - (-0.6941150f)) < 1e-5f);
    CHECK(std::fabs(leg.z - (-0.1349220f)) < 1e-5f);
    CHECK(std::fabs(leg.w - 0.6941150f) < 1e-5f);
    CHECK(V(*parts[2]->GetValue("position")).x == 1.0f + 0.15f);
    // a second run on the result passes in one iteration and changes nothing
    bool ok2 = false;
    std::string info2;
    ConfigNode::NodePtr again = pipe.Run(up, LoadContext::SFS, dotnet::Version(1, 12, 5), ok2, info2);
    CHECK(ok2);
    CHECK(again == up);
}

TEST_CASE("Upgrade pipeline on a synthetic 1.6.0 craft: PartReplace renames links / attN, offsets attPos0 and attN") {
    FreshEnvironment();
    ConfigNode::NodePtr root = Parse(
        "ship = T\nversion = 1.6.0\n"
        "PART\n{\npart = fuelTank_100\npos = 0,2,0\nrot = 0,0,0,1\nlink = liquidEngine3_101\n"
        "attN = bottom,liquidEngine3_101_0|-0.5|0\n}\n"
        "PART\n{\npart = liquidEngine3_101\npos = 0,0,0\nrot = 0,0,0,1\nattPos0 = 0,0,0\n"
        "attN = top,fuelTank_100_0|0.5|0\n}\n");
    SaveUpgradePipeline::SaveUpgradePipeline pipe;
    pipe.Init(SaveUpgradePipeline::CreateStockUpgradeScripts());
    bool ok = false;
    std::string info;
    ConfigNode::NodePtr up = pipe.Run(root, LoadContext::Craft, dotnet::Version(1, 12, 5), ok, info);
    REQUIRE(ok);
    CHECK(up->GetValue("version") == "1.12.0");  // v1123 TooEarly until the stamp reaches 1.12.0
    const auto parts = up->GetNodes("PART");
    CHECK(parts[1]->GetValue("part") == "liquidEngine3.v2_101");
    CHECK(parts[0]->GetValue("link") == "liquidEngine3.v2_101");
    CHECK(parts[0]->GetValue("attN") == "bottom,liquidEngine3.v2_101_0|-0.5|0");
    // engine: parent found through its link, Dot(rot*up, parentPos - pos) > 0 -> no swap; pos += (0, 0.2875004, 0)
    CHECK(V(*parts[1]->GetValue("pos")).y == 0.0f + 0.2875004f);
    CHECK(V(*parts[1]->GetValue("attPos0")).y == 0.0f + 0.2875004f);
    // the last vector of each attN gets the node's offset (top: (0, -0.2875, 0))
    const std::string attN = *parts[1]->GetValue("attN");
    CHECK(attN.rfind("top,fuelTank_100_", 0) == 0);
    CHECK(KSPUtil::ParseVector3(attN.substr(attN.rfind('_') + 1), '|').y == 0.5f + -0.2875f);
}

TEST_CASE("Upgrade pipeline: wheel overhaul, v180 control surface, v1123 module replace, too-early files") {
    FreshEnvironment();
    SaveEnvironment::Current().prefabModuleFloat = [](std::string_view part, std::string_view module, std::string_view field) {
        CHECK(part == "elevon1");
        CHECK(module == "ModuleControlSurface");
        CHECK(field == "ctrlSurfaceRange");
        return std::optional<float>(20.0f);
    };
    ConfigNode::NodePtr root = Parse(
        "GAME\n{\nversion = 1.0.0\nFLIGHTSTATE\n{\nVESSEL\n{\n"
        "PART\n{\nname = roverWheel1\nMODULE\n{\nname = ModuleWheel\nbrakesEngaged = True\nbrakesEnabled = True\n"
        "steeringLocked = False\ninvertSteering = True\nmotorEnabled = True\n}\n}\n"
        "PART\n{\nname = elevon1\nMODULE\n{\nname = ModuleControlSurface\nauthorityLimiter = 50\n"
        "AXISGROUPS\n{\nauthorityLimiter\n{\naxisGroup = Pitch\n}\n}\n}\n}\n}\n}\n}\n");
    SaveUpgradePipeline::SaveUpgradePipeline pipe;
    pipe.Init(SaveUpgradePipeline::CreateStockUpgradeScripts());
    bool ok = false;
    std::string info;
    ConfigNode::NodePtr up = pipe.Run(root, LoadContext::SFS, dotnet::Version(1, 12, 5), ok, info);
    REQUIRE(ok);
    const auto parts = up->GetNode("GAME")->GetNode("FLIGHTSTATE")->GetNode("VESSEL")->GetNodes("PART");
    const auto wheel = parts[0]->GetNodes("MODULE");
    REQUIRE(wheel.size() == 5);  // the old module stays; brakes, steering, motor-steering copy, motor are appended
    auto values = [](const ConfigNode* n) {
        std::string s;
        for (const auto& v : n->values()) s += v.name + "=" + v.value + ";";
        return s;
    };
    CHECK(values(wheel[1]) == "name=ModuleWheelBrakes;brakeInput=1;brakeTweak=100;");
    CHECK(values(wheel[2]) == "name=ModuleWheelSteering;steeringEnabled=True;steeringInvert=True;");
    CHECK(values(wheel[3]) == "name=ModuleWheelMotorSteering;steeringEnabled=True;steeringInvert=True;");
    CHECK(values(wheel[4]) == "name=ModuleWheelMotor;motorEnabled=True;");
    ConfigNode* surface = parts[1]->GetNode("MODULE");
    CHECK(surface->GetValue("deployAngle") == "10");  // 20 * 50 * 0.01 (float)
    CHECK(surface->GetNode("AXISGROUPS")->GetNode("deployAngle") != nullptr);

    // v1123: a 1.12.0 save with the JPLRepo module
    ConfigNode::NodePtr dock = Parse(
        "GAME\n{\nversion = 1.12.0\nFLIGHTSTATE\n{\nVESSEL\n{\nPART\n{\nname = dockingPort2\n"
        "MODULE\n{\nname = ModuleDockingNodeFixed\n}\n}\n}\n}\n}\n");
    ConfigNode::NodePtr dockUp = pipe.Run(dock, LoadContext::SFS, dotnet::Version(1, 12, 5), ok, info);
    REQUIRE(ok);
    CHECK(dockUp->GetNode("GAME")->GetValue("version") == "1.12.2");
    CHECK(dockUp->GetNode("GAME")->GetNode("FLIGHTSTATE")->GetNode("VESSEL")->GetNode("PART")->GetNode("MODULE")->GetValue(
              "name") == "ModuleDockingNode");

    // older than every script's EarliestCompatibleVersion: all TooEarly -> Fail
    ConfigNode::NodePtr ancient = Parse("GAME\n{\nversion = 0.20.0\n}\n");
    ConfigNode::NodePtr same = pipe.Run(ancient, LoadContext::SFS, dotnet::Version(1, 12, 5), ok, info);
    CHECK_FALSE(ok);
    CHECK(same == ancient);
    CHECK(info.find("Upgrades Failed. 1 iterations.") != std::string::npos);

    // KSPUpgradePipeline.Process: the fail dialog's answer goes to onFail
    KSPUpgradePipeline::Reset();
    int failed = 0;
    KSPUpgradePipeline::Process(
        ancient, "ancient", LoadContext::SFS, [](const ConfigNode::NodePtr&) { FAIL("onSucceed"); },
        [&](KSPUpgradePipeline::UpgradeFailOption opt, const ConfigNode::NodePtr& n) {
            CHECK(opt == KSPUpgradePipeline::UpgradeFailOption::LoadAnyway);
            CHECK(n == ancient);
            ++failed;
        },
        [](const std::string&) { return KSPUpgradePipeline::UpgradeFailOption::LoadAnyway; });
    CHECK(failed == 1);
    KSPUpgradePipeline::Reset();
}

// ===================================================== save model ===================================================

TEST_CASE("Game(): new game defaults and the GAME block order of Game.Save") {
    FreshEnvironment();
    Game g;
    CHECK(g.Mode == Game_Modes::SANDBOX);
    CHECK(g.Status == Game_GameStatus::ONGOING);
    CHECK(g.startScene == GameScenes::SPACECENTER);
    CHECK(g.compatible);
    CHECK(g.versionFull == "1.12.5.3190 (WindowsPlayer x64)");
    CHECK(g.versionCreated == g.versionFull);
    // Seed = new KSPRandom(Environment.TickCount ^ Guid.NewGuid().GetHashCode()).Next() (hooks of this test)
    CHECK(g.Seed >= 0);
    ConfigNode root;
    g.Save(root);
    const ConfigNode* game = root.GetNode("GAME");
    std::vector<std::string> names;
    for (const auto& v : game->values()) names.push_back(v.name);
    CHECK(names == std::vector<std::string>{"version", "Title", "Description", "linkURL", "linkCaption", "Mode", "Status",
                                            "Seed", "scene", "editor", "flag", "launchID", "defaultVABLaunchSite",
                                            "defaultSPHLaunchSite", "modded", "envInfo", "versionFull", "versionCreated"});
    std::vector<std::string> nodes;
    for (const auto& n : game->nodes()) nodes.push_back(n->name);
    CHECK(nodes == std::vector<std::string>{"CometNames", "PARAMETERS", "LoaderInfo", "ROSTER"});
    CHECK(game->GetValue("Mode") == "SANDBOX");
    CHECK(game->GetValue("Status") == "1");
    CHECK(game->GetValue("scene") == "5");
}

TEST_CASE("Game(ConfigNode) quirks: numeric Mode, Description '^', launch sites, ROSTER CREW/APPLICANTS, FlightLog") {
    FreshEnvironment();
    ConfigNode::NodePtr root = Parse(
        "GAME\n{\nversion = 1.12.5\nTitle = T\nDescription = a^b\nMode = 1\nSeed = 7\ndefaultVABLaunchSite = Woomerang\n"
        "ROSTER\n{\nCREW\n{\nname = Jebediah Kerman\ntrait = Pilot\nCAREER_LOG\n{\nflight = 3\n0 = Orbit,Kerbin\n}\n}\n"
        "APPLICANTS\n{\nRECRUIT\n{\nname = Bob Kerman\n}\n}\n}\n}\n");
    Game g(root.get());
    CHECK(g.Mode == Game_Modes::CAREER);
    CHECK(g.Description == std::optional<std::string>("a\nb"));
    CHECK(g.defaultVABLaunchSite == "LaunchPad");  // reset outside SANDBOX / SCIENCE_SANDBOX
    REQUIRE(g.CrewRoster->Count() == 2);
    auto jeb = (*g.CrewRoster)["Jebediah Kerman"];
    REQUIRE(jeb);
    CHECK(jeb->veteran);  // no "veteran" value: the four heroes become veterans and heroes
    CHECK(jeb->isHero);
    CHECK(jeb->careerLog.Flight() == 3);
    CHECK(jeb->careerLog.Count() == 1);
    // FlightLog.Load removed "flight" from the tree it read
    CHECK_FALSE(root->GetNode("GAME")->GetNode("ROSTER")->GetNode("CREW")->GetNode("CAREER_LOG")->HasValue("flight"));
    CHECK((*g.CrewRoster)["Bob Kerman"]->type() == ProtoCrewMember::KerbalType::Applicant);
    ConfigNode out;
    g.Save(out);
    const ConfigNode* game = out.GetNode("GAME");
    CHECK(game->GetValue("Mode") == "CAREER");
    CHECK(game->GetValue("Description") == "ab");  // '^' -> '\n' -> stripped by AddValue's CleanupInput
    CHECK(game->GetNode("ROSTER")->GetNodes("KERBAL").size() == 2);
    CHECK(game->GetNode("ROSTER")->GetNode("KERBAL", 1)->GetValue("type") == "Applicant");
}

TEST_CASE("Proto snapshots: unknown values, persistentId duplicates, missing ACTIONGROUPS, EVA cleanup") {
    FreshEnvironment();
    ConfigNode::NodePtr root = Parse(
        "GAME\n{\nversion = 1.12.5\nROSTER\n{\n}\nFLIGHTSTATE\n{\nversion = 1.12.5\nactiveVessel = 2\n"
        "VESSEL\n{\nname = A\ntype = Ship\npersistentId = 42\nmodValue = 1,2,3\nORBIT\n{\nREF = 1\n}\nACTIONGROUPS\n{\n}\n"
        "PART\n{\nname = mk1pod.v2\npersistentId = 99\nmodPart = x\nconnected = True\n}\n"
        "PART\n{\nname = mk1pod.v2\npersistentId = 99\n}\n}\n"
        "VESSEL\n{\nname = Kerbal\ntype = EVA\nPART\n{\nname = kerbalEVA\n}\n}\n"
        "VESSEL\n{\nname = B\ntype = Ship\npersistentId = 42\nORBIT\n{\nREF = 1\n}\n}\n}\n}\n");
    Game g(root.get());
    REQUIRE(g.flightState);
    // CleanUpUnsanitaryEVAKerbals removed the crewless EVA vessel; it was index 1 < activeVessel 2, but the fix-up
    // only decrements for vessels AFTER the active one (sic): activeVessel stays 2
    REQUIRE(g.flightState->protoVessels.size() == 2);
    CHECK(g.flightState->activeVesselIdx == 2);
    ProtoVessel& a = *g.flightState->protoVessels[0];
    ProtoVessel& b = *g.flightState->protoVessels[1];
    CHECK(a.persistentId == 42);
    CHECK(b.persistentId != 42);  // duplicate vessel id re-generated
    CHECK(a.protoPartSnapshots[0]->persistentId == 99);
    CHECK(a.protoPartSnapshots[1]->persistentId != 99);
    ConfigNode va;
    a.Save(va);
    CHECK(va.GetValue("modValue") == "1");  // KSPParseable.value: text before the first ','
    ConfigNode pa;
    a.protoPartSnapshots[0]->Save(pa);
    CHECK(pa.GetValue("modPart") == "x, UNKNOWN");  // KSPParseable.Save
    CHECK_FALSE(pa.HasValue("connected"));
    CHECK_FALSE(pa.HasValue("PreFailState"));  // Making History not installed
    ConfigNode vb;
    CHECK_THROWS_AS(b.Save(vb), NullReferenceException);  // no ACTIONGROUPS node (KSP: actionGroups.CopyTo -> NRE)
}

TEST_CASE("ProtoCrewMember: trait assignment, suits, Save order") {
    FreshEnvironment();
    ConfigNode::NodePtr n = Parse("KERBAL\n{\nname = Zed Kerman\nsuit = Vintage\ntype = Crew\n}\n");
    ProtoCrewMember pcm(Game_Modes::SANDBOX, *n->GetNode("KERBAL"));
    // empty trait: TraitNamesNoTourist[abs(name.GetHashCode_Net35()) % 3] (Pilot, Engineer, Scientist)
    REQUIRE(pcm.trait);
    CHECK((*pcm.trait == "Pilot" || *pcm.trait == "Engineer" || *pcm.trait == "Scientist"));
    CHECK(pcm.suit() == ProtoCrewMember::KerbalSuit::Default);  // Vintage needs Making History
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", true);
    ProtoCrewMember pcm2(Game_Modes::SANDBOX, *Parse("KERBAL\n{\nname = Zed Kerman\nsuit = Vintage\n}\n")->GetNode("KERBAL"));
    CHECK(pcm2.suit() == ProtoCrewMember::KerbalSuit::Vintage);
    CHECK(pcm2.GetKerbalEVAPartName() == "kerbalEVAVintage");
    ConfigNode out("KERBAL");
    pcm2.Save(out);
    CHECK(out.values()[0].name == "name");
    CHECK(out.GetValue("suit") == "Vintage");
    CHECK(out.GetValue("inactiveTimeEnd") == "0");
    CHECK(out.GetValue("lightG") == "0.5176");  // float "G9" (0.517600000095...)
    CHECK(out.nodes()[0]->name == "CAREER_LOG");
}

TEST_CASE("ShipConstruct: craft file form (description U+00A8, srfN position twice, link resolution)") {
    FreshEnvironment();
    ConfigNode::NodePtr craft = Parse(
        "ship = S\nversion = 1.12.5\ndescription = a\xC2\xA8" "b\ntype = SPH\nsize = 1,2,3\n"
        "PART\n{\npart = a_1\npartName = Part\npersistentId = 5\nlink = b_2\n}\n"
        "PART\n{\npart = b_2\npartName = Part\npersistentId = 6\n"
        "srfN = srfAttach,a_1,,0|1|0,0|0|1,0|0|0\nattN = top,Null_0_0|1|0_0|1|0_0|1|0_0|1|0\n}\n");
    ShipConstruct ship;
    std::string err;
    REQUIRE(ship.LoadShip(*craft, 0, true, err));
    CHECK(ship.shipDescription == "a\nb");
    CHECK(ship.parts[1].symMethod == SymmetryMethod::Mirror);  // SPH default
    CHECK(ship.parts[0].children == std::vector<int>{1});
    ConfigNode::NodePtr out = ship.SaveShip();
    CHECK(out->GetValue("description") == "a\xC2\xA8" "b");
    CHECK(out->GetNode("PART", 1)->GetValue("srfN") == "srfAttach,a_1,,0|1|0,0|0|1,0|1|0");  // position, orientation, position
    CHECK(out->GetNode("PART", 0)->GetValue("link") == "b_2");
    // a link to a part that is not in the craft: "File corrupted", LoadShip returns false
    ConfigNode::NodePtr broken = Parse("ship = S\nversion = 1.12.5\nPART\n{\npart = a_1\nlink = c_9\n}\n");
    ShipConstruct ship2;
    CHECK_FALSE(ship2.LoadShip(*broken, 0, true, err));
    // too old for ShipConstruct (0.18.0 is the last compatible)
    ConfigNode::NodePtr old = Parse("ship = S\nversion = 0.17.0\n");
    ShipConstruct ship3;
    CHECK_FALSE(ship3.LoadShip(*old, 0, true, err));
}

// ================================================== GamePersistence ==================================================

TEST_CASE("GamePersistence: file names, APPEND / BACKUP rules, quicksave naming, new game scenarios") {
    FreshEnvironment();
    const fs::path tmp = fs::temp_directory_path() / "kspcore_wp04_persistence";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "data");
    KSPUtil::ApplicationDataPath = (tmp / "data").generic_string();  // ApplicationRootPath = data + "/../"
    CHECK(GamePersistence::ReplaceInvalidFileNameChars("a:b*c?d/e\\f\"g<h>i|j") == "a_b_c_d_e_f_g_h_i_j");
    std::tm t{};
    t.tm_year = 126;
    t.tm_mon = 7;
    t.tm_mday = 17;
    t.tm_hour = 19;
    t.tm_min = 46;
    t.tm_sec = 3;
    CHECK(GamePersistence::BackupFileName("persistent", t) == "persistent (2026_08_17_19_46_03).sfs");

    std::string folder;
    auto game = GamePersistence::CreateNewGame("unit", Game_Modes::SANDBOX,
                                               GameParameters::GetDefaultParameters(Game_Modes::SANDBOX, GameParameters::Preset::Normal),
                                               "Squad/Flags/default", GameScenes::SPACECENTER, EditorFacility::None,
                                               KSPScenarioType::GetAllScenarioTypesInAssemblies(), folder);
    CHECK(folder == "unit");
    CHECK(fs::exists(tmp / "saves" / "unit" / "persistent.sfs"));
    CHECK(fs::exists(tmp / "saves" / "unit" / "Ships" / "SPH"));
    // the scenario list of a new sandbox game = the physprofile save's (KSP 1.12.5 sandbox) names and scenes, in order
    ConfigNode::NodePtr phys = ConfigNode::Load(kSaves + "/physprofile/persistent.sfs");
    REQUIRE(phys);
    std::vector<std::string> expected, actual;
    for (ConfigNode* s : phys->GetNode("GAME")->GetNodes("SCENARIO"))
        expected.push_back(*s->GetValue("name") + "|" + *s->GetValue("scene"));
    for (const auto& s : game->scenarios) actual.push_back(*s->moduleName + "|" + *s->GetData().GetValue("scene"));
    CHECK(actual == expected);
    CHECK(game->CrewRoster->Count() == 4);
    CHECK(game->Title == std::optional<std::string>("unit (SANDBOX)"));

    // APPEND: name + "000", "001", ...
    CHECK(GamePersistence::SaveGame(*game, "auto", folder, SaveMode::APPEND) == "auto000");
    CHECK(GamePersistence::SaveGame(*game, "auto", folder, SaveMode::APPEND) == "auto001");
    // ABORT when the file exists
    CHECK(GamePersistence::SaveGame(*game, "auto000", folder, SaveMode::ABORT).empty());
    // BACKUP: copies the existing file into Backup with a timestamp, keeps SAVE_BACKUPS files
    SaveEnvironment::Current().SAVE_BACKUPS = 2;
    int second = 0;
    GamePersistence::DateTimeNow = [&] {
        std::tm x = t;
        x.tm_sec = second++;
        return x;
    };
    // call 1: no quicksave yet (no backup); 2: Backup created with a copy (_01); 3: 1 < 2 backups -> copy (_02);
    // 4: copy (_03), then the oldest LastWriteTime (_01) is deleted
    for (int i = 0; i < 4; ++i) {
        GamePersistence::SaveGame(*game, "quicksave", folder, SaveMode::BACKUP);
        // The pruning sorts by LastWriteTime. Backups land within one mtime tick on coarse filesystems (WSL DrvFs),
        // which ties the sort; pin each backup's mtime to the second encoded in its name so the order is the one
        // the wall clock would give on NTFS.
        const fs::path backupDir = tmp / "saves" / "unit" / "Backup";
        if (fs::exists(backupDir))
            for (const auto& b : fs::directory_iterator(backupDir)) {
                const std::string name = b.path().stem().string();  // "quicksave (2026_08_17_19_46_SS)"
                const int sec = std::stoi(name.substr(name.size() - 3, 2));
                fs::last_write_time(b.path(), fs::file_time_type::clock::now() - std::chrono::hours(1) +
                                                  std::chrono::seconds(sec));
            }
    }
    int backups = 0;
    for (const auto& b : fs::directory_iterator(tmp / "saves" / "unit" / "Backup")) {
        (void)b;
        ++backups;
    }
    CHECK(backups == 2);
    CHECK_FALSE(fs::exists(tmp / "saves" / "unit" / "Backup" / "quicksave (2026_08_17_19_46_01).sfs"));
    CHECK(fs::exists(tmp / "saves" / "unit" / "Backup" / "quicksave (2026_08_17_19_46_02).sfs"));
    CHECK(fs::exists(tmp / "saves" / "unit" / "Backup" / "quicksave (2026_08_17_19_46_03).sfs"));
    GamePersistence::DateTimeNow = nullptr;
    // quicksave naming: "quicksave", then "quicksave #1", ...
    CHECK(GamePersistence::NextQuicksaveFilename(folder) == "quicksave #1");
    CHECK(GamePersistence::CheckFilename("") == GamePersistence::FilenameCheckResults::Empty);
    // the saved persistent.sfs loads back
    auto loaded = GamePersistence::LoadGame("persistent", folder, true, true);
    REQUIRE(loaded);
    CHECK(loaded->Title == game->Title);
    KSPUtil::ApplicationDataPath.clear();
    fs::remove_all(tmp);
}
