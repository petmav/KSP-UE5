// WP-30 unit tests: ModuleGPS.cs, FlagSite.cs, ModulePartFirework.cs (src/modules/misc_modules.cpp).
//
// The float arithmetic (shell rigidbody numbers, mass modifier, ModuleGPS number text) is checked against tools/wp30/firework_reference.py
// (numpy float32, independent of kspcore; tests/data/wp30_reference.json) at 0 ULP. The inputs are the stock launcher parts' own MODULE
// values (core/testdata/GameData-mirror). The state machines (FlagSite's KerbalFSM, ModuleGPS's asteroid / comet / update-rate logic,
// the firework shot counter) are checked against the C# line by line.
#include <cmath>
#include <fstream>
#include <map>
#include <string>

#include "doctest/doctest.h"
#include "nlohmann/json.hpp"
#include "resource_fixture.hpp"

#include "ksp/core/exceptions.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/events/game_events.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/modules/misc_modules.hpp"
#include "ksp/modules/resource_processing.hpp"
#include "ksp/parts/part.hpp"
#include "ksp/parts/part_module_data.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/save/save_environment.hpp"

using namespace ksp;
using namespace ksp::rtest;
using Localizer = KSP::Localization::Localizer;

namespace {

nlohmann::json LoadRef() {
    std::ifstream f(std::string(KSPCORE_TEST_DATA_DIR) + "/wp30_reference.json", std::ios::binary);
    REQUIRE(f.good());
    return nlohmann::json::parse(f);
}

void LoadLocalizer30() {
    Localizer::Shutdown();
    Localizer::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(ksp::testing::GameRoot() + "/GameData/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    Localizer::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

struct Reset30 {
    Reset30() { Clear(); }
    ~Reset30() { Clear(); }
    static void Clear() {
        ModuleGPSEnvironment::Reset();
        FlagSiteEnvironment::Reset();
        FireworkEnvironment::Reset();
        ResourceProcessingEnvironment::Reset();
        ResourceFlowEnvironment::InfinitePropellant = false;
        Time::time = 0.0f;
        Time::deltaTime = 0.02f;
        Time::fixedDeltaTime = 0.02f;
    }
};

template <class T>
T* Mod(Part& p, const char* cls) {
    auto* m = dynamic_cast<T*>(p.FindModuleOfClass(cls));
    REQUIRE(m != nullptr);
    return m;
}

FireworkFXDefinition Trail() {
    FireworkFXDefinition d;
    d.name = "bareTrail";
    d.fwType = FireworkEffectType::TRAIL;
    d.prefabName = "fx_fwTrail1";
    d.displayName = "Bare";
    d.minTrailLifetime = 0.5f;
    d.maxTrailLifetime = 1.5f;
    return d;
}
FireworkFXDefinition Burst(const char* display, const char* color3) {
    FireworkFXDefinition d;
    d.name = display;
    d.fwType = FireworkEffectType::BURST;
    d.prefabName = std::string("fx_burst_") + display;
    d.displayName = display;
    d.color1Name = "Core";
    d.color2Name = "Rim";
    d.color3Name = color3;
    d.crackleSFX = "Squad/Sounds/crackle";
    d.randomizeBurstOrientation = true;
    return d;
}

}  // namespace

// ModulePartFirework.cs:473 LaunchShell numbers, :769 GetModuleMass, :624 label: against the Python float32 reference
TEST_CASE("WP-30 ModulePartFirework: shell numbers, mass modifier and shot label against the float32 reference") {
    Reset30 r;
    const nlohmann::json ref = LoadRef();
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    std::map<std::string, std::unique_ptr<Part>> parts;
    for (const auto& pj : ref["parts"]) parts[pj["part"]] = g.New(pj["part"].get<std::string>().c_str());
    FireworkShellLaunch last;
    int launches = 0;
    FireworkEnvironment::launchShell = [&](PartModule&, const FireworkShellLaunch& l) {
        last = l;
        ++launches;
    };
    for (const auto& c : ref["launch"]) {
        Part& p = *parts[c["part"]];
        auto* m = Mod<ModulePartFirework>(p, "ModulePartFirework");
        m->Set<float>("shellMass", c["shellMass"].get<float>());
        m->Set<float>("shellVelocity", c["shellVelocity"].get<float>());
        m->Set<bool>("variationOnShellDirection", false);
        m->Set<float>("fireworkShots", 32.0f);
        Time::fixedDeltaTime = c["fixedDeltaTime"].get<float>();
        const Vector3f dir(c["dir"][0].get<float>(), c["dir"][1].get<float>(), c["dir"][2].get<float>());
        FireworkEnvironment::cannon = [dir](PartModule&, const std::string&) {
            return std::optional<FireworkEnvironment::Cannon>(FireworkEnvironment::Cannon{Vector3f(1.0f, 2.0f, 3.0f), dir});
        };
        m->shellPrefabPresent = true;
        m->availableFXPresent = true;
        m->availableFX = {Trail(), Burst("Peony", "none")};
        m->trailOptions = {"Bare"};
        m->burstOptions = {"Peony"};
        m->Set<int32_t>("trailType", 0);
        m->Set<int32_t>("burstType", 0);
        m->fireworkColors.assign(5, Color(0.25f, 0.5f, 0.75f));
        const auto& vf = c["vesselForce"];
        for (size_t k = 0; k < vf.size(); ++k) {
            const Vector3d vel(vf[k]["vel"][0].get<double>(), vf[k]["vel"][1].get<double>(), vf[k]["vel"][2].get<double>());
            FireworkEnvironment::vesselRbVelocityD = [vel](PartModule&) { return vel; };
            FireworkEnvironment::krakensbaneFrameSpeed = [] { return 0.0; };
            m->LaunchShell();
            REQUIRE(last.vesselVelocityForce.has_value());
            CHECK(last.vesselVelocityForce->x == vf[k]["force"][0].get<float>());
            CHECK(last.vesselVelocityForce->y == vf[k]["force"][1].get<float>());
            CHECK(last.vesselVelocityForce->z == vf[k]["force"][2].get<float>());
            CHECK(last.rbMass == c["rbMass"].get<float>());
            CHECK(last.force.x == c["force"][0].get<float>());
            CHECK(last.force.y == c["force"][1].get<float>());
            CHECK(last.force.z == c["force"][2].get<float>());
            CHECK(last.recoil.x == c["recoil"][0].get<float>());
            CHECK(last.recoil.y == c["recoil"][1].get<float>());
            CHECK(last.recoil.z == c["recoil"][2].get<float>());
            CHECK(last.position.x == 1.0f);
            CHECK(last.shellForceDir.x == dir.x);
            m->Set<float>("fireworkShots", 32.0f);
        }
    }
    CHECK(launches == static_cast<int>(ref["launch"].size()) * 3);
    // GetModuleMass + the label
    for (const auto& mj : ref["mass"]) {
        auto* m = Mod<ModulePartFirework>(*parts[mj["part"]], "ModulePartFirework");
        m->Set<float>("fireworkShots", mj["fireworkShots"].get<float>());
        m->Set<float>("shellMass", mj["shellMass"].get<float>());
        CHECK(m->GetModuleMass(mj["defaultMass"].get<float>(), ModifierStagingSituation::CURRENT) == mj["mass"].get<float>());
        m->updateFireworkShotLabel();
        CHECK(m->Get<std::string>("fireworkShotsDisplay") == mj["label"].get<std::string>());
    }
}

// ModulePartFirework.cs:167 OnAwake with a missing shell prefab: the C# looks up base.Events["LaunchShell"] (the event is named LaunchShellEvent), the
// lookup is null and the guiActive write throws NullReferenceException (the sic path of OnAwake)
TEST_CASE("WP-30 ModulePartFirework.OnAwake with shellPrefabExists == false throws NullReferenceException (ModulePartFirework.cs:167)") {
    Reset30 r;
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    LoadLocalizer30();
    auto p = g.New("fireworksLauncherSmall");
    auto* m = Mod<ModulePartFirework>(*p, "ModulePartFirework");
    FireworkEnvironment::shellPrefabExists = [](const std::string&) { return false; };
    CHECK_THROWS_AS(m->OnAwake(), NullReferenceException);
    CHECK_FALSE(m->shellPrefabPresent);
    FireworkEnvironment::shellPrefabExists = [](const std::string&) { return true; };
    CHECK_NOTHROW(m->OnAwake());
    CHECK(m->shellPrefabPresent);
}

// ModulePartFirework.cs:261 OnStart (clamp, option lists, color field names), :473 shot counter, :230 / :250 colors
TEST_CASE("WP-30 ModulePartFirework: OnStart clamp and option lists, shot counter, color save / load, OnColorChanged") {
    Reset30 r;
    const nlohmann::json ref = LoadRef();
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    LoadLocalizer30();
    for (const auto& cj : ref["clamp"]) {
        auto p = g.New(cj["part"].get<std::string>().c_str());
        auto* m = Mod<ModulePartFirework>(*p, "ModulePartFirework");
        CHECK(m->Get<float>("maxShots") == cj["maxShots"].get<float>());
        m->trailChooseOptionPresent = m->burstChooseOptionPresent = m->fireworkShotsFloatRangePresent = m->tertiaryFieldPresent = true;
        m->availableFXPresent = true;
        m->availableFX.clear();
        FireworkEnvironment::fxDefinitions = {Trail(), Burst("Peony", "none"), Burst("Willow", "Glitter")};
        m->Set<float>("fireworkShots", cj["fireworkShots"].get<float>());
        m->OnStart(StartState::Editor);
        CHECK(m->Get<float>("fireworkShots") == cj["after"].get<float>());
        CHECK(m->fireworkShotsRangeMax == cj["maxShots"].get<float>());
        CHECK(m->trailOptions == std::vector<std::string>{"Bare"});
        CHECK(m->burstOptions == (std::vector<std::string>{"Peony", "Willow"}));
        CHECK(m->trailDisplay == m->trailOptions);
        // configureColorFields (burstType 0 = Peony): color names, no third color
        CHECK(m->Fields()["primaryBurstColorChanger"]->guiName == "Core");
        CHECK(m->Fields()["secondaryBurstColorChanger"]->guiName == "Rim");
        // :639 with a third color: the Willow burst names it
        bool tertiaryVisible = false;
        FireworkEnvironment::setTertiaryColorVisible = [&](PartModule&, bool v) { tertiaryVisible = v; };
        m->Set<int32_t>("burstType", 1);
        m->configureColorFields();
        CHECK(tertiaryVisible);
        CHECK(m->Fields()["tertiaryBurstColorChanger"]->guiName == "Glitter");
        m->Set<int32_t>("burstType", 0);
        m->configureColorFields();
        CHECK_FALSE(tertiaryVisible);
        // the burstType OnValueModified handler OnStart added
        m->fieldModifiedLog.clear();
        for (auto& h : m->Fields()["burstType"]->OnValueModified) h(ModuleValue{});
        CHECK(m->fieldModifiedLog == std::vector<std::string>{"burstType"});
    }
    // :473 shots: decrements, stops at 0, InfinitePropellant leaves it; the missing prefab returns
    auto p = g.New("fireworksLauncherSmall");
    auto* m = Mod<ModulePartFirework>(*p, "ModulePartFirework");
    int launches = 0;
    FireworkEnvironment::launchShell = [&](PartModule&, const FireworkShellLaunch&) { ++launches; };
    m->shellPrefabPresent = true;
    m->availableFXPresent = true;
    m->availableFX = {Trail(), Burst("Peony", "none")};
    m->trailOptions = {"Bare"};
    m->burstOptions = {"Peony"};
    m->fireworkColors.assign(5, Color());
    m->Set<bool>("variationOnShellDirection", false);
    m->Set<float>("fireworkShots", 2.0f);
    m->LaunchShell();
    CHECK(m->Get<float>("fireworkShots") == 1.0f);
    CHECK(m->Get<std::string>("fireworkShotsDisplay") == "1");
    m->LaunchShell();
    m->LaunchShell();
    CHECK(launches == 2);
    CHECK(m->Get<float>("fireworkShots") == 0.0f);
    ResourceFlowEnvironment::InfinitePropellant = true;
    m->LaunchShell();
    CHECK(launches == 3);
    CHECK(m->Get<float>("fireworkShots") == 0.0f);
    m->shellPrefabPresent = false;
    m->LaunchShell();
    CHECK(launches == 3);
    // the OnValueModified handler of fireworkShots keeps the label current (OnAwake :187)
    m->Set<float>("fireworkShots", 7.0f);
    for (auto& h : m->Fields()["fireworkShots"]->OnValueModified) h(ModuleValue{});
    CHECK(m->Get<std::string>("fireworkShotsDisplay") == "7");
    // OnColorChanged: the five picker names map to fireworkColors[] and the R / G / B fields
    struct Pick {
        const char* name;
        size_t idx;
        const char* r;
    };
    const Pick picks[] = {{"primaryBurstColorChanger", 0, "burstColor1R"}, {"secondaryBurstColorChanger", 1, "burstColor2R"},
                          {"tertiaryBurstColorChanger", 2, "burstColor3R"}, {"primaryTrailColorChanger", 3, "trailColor1R"},
                          {"secondaryTrailColorChanger", 4, "trailColor2R"}};
    float v = 0.1f;
    for (const Pick& pk : picks) {
        const Color c(v, v + 0.05f, v + 0.1f);
        m->OnColorChanged(c, pk.name);
        CHECK(m->fireworkColors[pk.idx].Equals(c));
        CHECK(m->Get<float>(pk.r) == c.r);
        CHECK(m->GetCurrentColor(pk.name).Equals(c));
        v += 0.15f;
    }
    m->OnColorChanged(Color(0.9f, 0.9f, 0.9f), "");  // empty picker name: ignored
    m->OnColorChanged(Color(0.9f, 0.9f, 0.9f), "nope");
    CHECK(m->fireworkColors[0].r != 0.9f);
    // OnSave writes the five colors; OnLoad rebuilds them from the fields
    ConfigNode node("MODULE");
    m->OnSave(node);
    CHECK(node.GetValue("burstColor1R") == dotnet::FormatSingleG(m->fireworkColors[0].r, 9));
    CHECK(node.GetValue("trailColor2B") == dotnet::FormatSingleG(m->fireworkColors[4].b, 9));
    m->Set<float>("burstColor2G", 0.375f);
    m->OnLoad(node);
    CHECK(m->fireworkColors[1].g == 0.375f);
    // OnPAWInit sets the yellowish orange on the open pickers only
    m->pickerIds = {{"primaryBurstColorChanger", "pb"}};
    m->OnPAWInit(p.get());
    CHECK(m->Get<float>("burstColor1G") == 0.6705883f);
    CHECK(m->Get<float>("burstColor1B") == 1.0f / 17.0f);
    // a missing cannon child is a NullReferenceException (GetChild(...).transform)
    m->shellPrefabPresent = true;
    ResourceFlowEnvironment::InfinitePropellant = false;
    m->Set<float>("fireworkShots", 3.0f);
    FireworkEnvironment::cannon = [](PartModule&, const std::string&) { return std::optional<FireworkEnvironment::Cannon>(); };
    CHECK_THROWS_AS(m->LaunchShell(), NullReferenceException);
}

// ModuleGPS.cs:85 Update, :37 / :61 asteroid and comet connection, :139-:161 IAnimatedModule
TEST_CASE("WP-30 ModuleGPS: update rate, lat / lon text against the reference, asteroid and comet connection") {
    Reset30 r;
    const nlohmann::json ref = LoadRef();
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    LoadLocalizer30();
    auto p = g.New("OrbitalScanner");
    auto rock = g.New("OrbitalScanner");
    rock->AddModule("ModuleAsteroid", true);
    auto comet = g.New("OrbitalScanner");
    comet->AddModule("ModuleComet", true);
    support::TestVessel tv;  // gives the parts a vessel (VesselOf answers only for parts that have one)
    tv.Adopt({p.get(), rock.get(), comet.get()});
    auto* gps = Mod<ModuleGPS>(*p, "ModuleGPS");
    REQUIRE(gps->AsAnimatedModule() != nullptr);
    CHECK(gps->Get<std::string>("body") == "???");
    CHECK(gps->IsSituationValid());
    gps->DisableModule();
    CHECK_FALSE(gps->ModuleIsActive());
    gps->EnableModule();
    CHECK(gps->ModuleIsActive());
    CHECK(gps->GetModuleDisplayName() == Localizer::Format("#autoLOC_8004214"));

    ProcessingVessel pv;
    pv.mainBody.displayName = "Kerbin";
    pv.SituationString = "Landed";
    ResourceProcessingEnvironment::vesselOf = [&](const Part&) { return &pv; };
    ModuleGPSEnvironment::itemListContains = [](Part&) { return true; };
    // the refresh only runs once secondsUpdateRate (0.1 s) has accumulated; the accumulator resets
    pv.latitude = -0.0972;
    pv.longitude = -74.5575;
    Time::deltaTime = 0.04f;
    gps->Update();
    gps->Update();
    CHECK(gps->Get<std::string>("lat") == "???");
    gps->Update();  // 0.12 >= 0.1
    CHECK(gps->secondsSinceLastDisplayUpdate == 0.0f);
    CHECK(gps->Get<std::string>("body") == Localizer::Format("#autoLOC_7001301", {"Kerbin"}));
    CHECK(gps->Get<std::string>("bioName") == "Landed");  // no biome: the situation string
    // with a biome
    ResourceProcessingEnvironment::biomeAt = [](double, double, const Part&) {
        return std::optional<ResourceProcessingEnvironment::Biome>(ResourceProcessingEnvironment::Biome{"Shores", "The Shores"});
    };
    Time::deltaTime = 0.2f;
    for (const auto& c : ref["gps"]) {
        pv.latitude = c["lat"].get<double>();
        pv.longitude = c["lon"].get<double>();
        gps->Update();
        CHECK(gps->Get<std::string>("lat") == Localizer::Format("#autoLOC_6001052", {c["latAbs"].get<std::string>(), std::to_string(c["latSign"].get<int>()),
                                                                                     c["latRad"].get<std::string>()}));
        CHECK(gps->Get<std::string>("lon") == Localizer::Format("#autoLOC_6001053", {c["lonAbs"].get<std::string>(), std::to_string(c["lonSign"].get<int>()),
                                                                                     c["lonRad"].get<std::string>()}));
        CHECK(gps->Get<std::string>("bioName") == Localizer::Format("#autoLOC_7001301", {"The Shores"}));
    }
    // no PAW open: nothing is rewritten
    gps->Set<std::string>("lat", "keep");
    ModuleGPSEnvironment::itemListContains = [](Part&) { return false; };
    gps->Update();
    CHECK(gps->Get<std::string>("lat") == "keep");
    ModuleGPSEnvironment::itemListContains = [](Part&) { return true; };
    // not in the flight scene: nothing happens
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    gps->Update();
    CHECK(gps->Get<std::string>("lat") == "keep");
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    // an error inside the refresh (no vessel record) is caught and printed
    ResourceProcessingEnvironment::vesselOf = [](const Part&) -> const ProcessingVessel* { return nullptr; };
    gps->Update();
    CHECK(ModuleGPSEnvironment::printed == std::vector<std::string>{"[RESOURCES] Error updating ModuleGPS"});
    ResourceProcessingEnvironment::vesselOf = [&](const Part&) { return &pv; };
    // CheckForAsteroidConnect / CheckForCometConnect: a part with the module on the vessel
    pv.parts = {p.get(), rock.get()};
    gps->Update();
    CHECK(gps->isConnectedToAsteroid);
    CHECK_FALSE(gps->IsSituationValid());
    CHECK(gps->Get<std::string>("body") == Localizer::Format("#autoLOC_258910"));
    CHECK(gps->Get<std::string>("lat") == Localizer::Format("#autoLOC_258912"));
    CHECK(gps->Get<std::string>("lon") == Localizer::Format("#autoLOC_258913"));
    CHECK(gps->Get<std::string>("bioName") == Localizer::Format("#autoLOC_258911"));
    pv.parts = {p.get(), comet.get()};
    gps->Update();  // the asteroid flag is recomputed (false) and the comet found
    CHECK_FALSE(gps->isConnectedToAsteroid);
    CHECK(gps->isConnectedToComet);
    CHECK(gps->Get<std::string>("body") == Localizer::Format("#autoLOC_6005065"));
    CHECK_FALSE(gps->IsSituationValid());
}

// FlagSite.cs:137 SetupFSM: Placing -> Placed (timed event), Toppled on a joint break, OnSave / OnLoad of the state, placement failure
TEST_CASE("WP-30 FlagSite: the KerbalFSM, joint, locks, plaque dialog bookkeeping, OnSave / OnLoad") {
    Reset30 r;
    Game15 g;
    SaveEnvironment::Current().loadedScene = GameScenes::FLIGHT;
    LoadLocalizer30();
    auto p = g.New("flag");
    auto* f = Mod<FlagSite>(*p, "FlagSite");
    CHECK(f->Get<float>("deployVisibilityDelay") == 1.0f);
    CHECK(f->Get<float>("breakForce") == 180.0f);
    CHECK(f->Get<std::string>("placedBy") == "Unknown");
    int visible = 0, invisible = 0, joints = 0, destroyed = 0;
    float brokenAt = 0.0f;
    FlagSiteEnvironment::setVisible = [&](FlagSite&, bool v) { (v ? visible : invisible)++; };
    FlagSiteEnvironment::createJoint = [&](FlagSite&) {
        ++joints;
        return true;
    };
    FlagSiteEnvironment::destroyJoint = [&](FlagSite&) { ++destroyed; };
    FlagSiteEnvironment::setJointBreakForce = [&](FlagSite&, float force, float torque) {
        brokenAt = force;
        CHECK(torque == force);
    };
    FlagSiteEnvironment::flagDeployAnimationLength = 3.0;
    f->Start();
    REQUIRE(f->Fsm() != nullptr);
    CHECK(f->Fsm()->currentStateName == "Placing");
    CHECK(f->joint);
    CHECK(joints == 1);
    CHECK(invisible == 1);
    REQUIRE(f->pendingInvokes.size() == 1);
    CHECK(f->pendingInvokes[0].first == "MakeVisible");
    CHECK(f->pendingInvokes[0].second == 1.0f);
    CHECK(FlagSiteEnvironment::controlLocks.count("Flag_NoInterruptWhileDeploying") == 1);
    CHECK_FALSE(f->Events()["TakeDown"]->active);
    CHECK(f->jointBreakPending);
    f->ApplyJointBreakForce();  // the DelayedCallback after unbreakablePeriodLength
    CHECK(brokenAt == 180.0f);
    // time passes: the timed event fires after the animation length
    Time::time = 2.0f;
    f->Update();
    CHECK(f->Fsm()->currentStateName == "Placing");
    Time::time = 3.5f;
    f->Update();
    CHECK(f->Fsm()->currentStateName == "Placed");
    CHECK(visible == 1);    // OnLeave of Placing: MakeVisible
    CHECK(destroyed == 0);  // the last event was onPlaceComplete: the joint stays
    CHECK(FlagSiteEnvironment::controlLocks.count("Flag_NoInterruptWhileDeploying") == 0);
    CHECK(f->Events()["TakeDown"]->active);
    CHECK(p->fields.PermanentGroundContact);
    // OnSave stores the state name; a joint break topples the flag
    ConfigNode node("MODULE");
    f->OnSave(node);
    CHECK(node.GetValue("state") == "Placed");
    f->OnJointBreak(250.0f);
    CHECK(f->Fsm()->currentStateName == "Toppled");
    CHECK_FALSE(f->Events()["TakeDown"]->active);
    CHECK(f->Events()["PickUp"]->active);
    CHECK_FALSE(p->fields.PermanentGroundContact);
    CHECK(destroyed == 1);  // OnLeave of Placed: joint destroyed
    CHECK_FALSE(f->joint);
    // a toppled flag cannot topple again (the event excludes the Toppled state): RunEvent logs and stays
    f->OnJointBreak(250.0f);
    CHECK(f->Fsm()->currentStateName == "Toppled");
    // unpacking in the Toppled state does not create a joint
    f->OnVesselUnpack();
    CHECK(joints == 1);
    // OnLoad("state") restarts the FSM in that state (and Placed re-creates the joint)
    auto p2 = g.New("flag");
    auto* f2 = Mod<FlagSite>(*p2, "FlagSite");
    ConfigNode saved("MODULE");
    saved.AddValue("state", "Placed");
    f2->OnLoad(saved);
    f2->Start();
    CHECK(f2->Fsm()->currentStateName == "Placed");
    CHECK(f2->joint);
    CHECK(joints == 2);
    f2->OnVesselUnpack();
    CHECK(joints == 2);  // the joint exists
    // placement failure inside deployFailRevertThreshold: PickUp (no EVA kerbal: nothing dies), the state is Toppled
    auto p3 = g.New("flag");
    auto* f3 = Mod<FlagSite>(*p3, "FlagSite");
    Time::time = 4.0f;
    f3->Start();
    int pickups = 0;
    FlagSiteEnvironment::evaAddFlag = [&] {
        ++pickups;
        return false;
    };
    Time::time = 5.0f;
    f3->OnPlacementFail();
    CHECK(pickups == 1);
    CHECK(f3->Fsm()->currentStateName == "Toppled");
    f3->TakeDown();
    CHECK(pickups == 2);
    // no animationRoot: no FSM, and the Unity messages need one
    FlagSiteEnvironment::hasAnimationRoot = false;
    auto p4 = g.New("flag");
    auto* f4 = Mod<FlagSite>(*p4, "FlagSite");
    f4->Start();
    CHECK(f4->Fsm() == nullptr);
    CHECK_THROWS_AS(f4->Update(), NullReferenceException);
    ConfigNode empty("MODULE");
    f4->OnSave(empty);
    CHECK_FALSE(empty.HasValue("state"));
    // site rename dialog bookkeeping: lock, text inputs, enabled condition, the afterDialog callback of both buttons
    int after = 0, spawned = 0, dismissed = 0;
    FlagSiteEnvironment::spawnSiteRenameDialog = [&](FlagSite&) { ++spawned; };
    FlagSiteEnvironment::dismissSiteRenameDialog = [&](FlagSite&) { ++dismissed; };
    f4->Set<std::string>("PlaqueText", "Hello");
    f4->RenameSite([&] { ++after; });
    CHECK(spawned == 1);
    CHECK(f4->siteRenameDialogOpen);
    CHECK(f4->siteName.empty());
    CHECK(f4->newPlaqueText == "Hello");
    CHECK(FlagSiteEnvironment::controlLocks.count("siteRenameDialog") == 1);
    CHECK_FALSE(f4->SiteRenameAcceptEnabled());  // an empty name is not a valid vessel name
    f4->SiteRenameDismissPressed();
    CHECK(after == 1);
    CHECK(dismissed == 1);
    CHECK_FALSE(f4->siteRenameDialogOpen);
    CHECK(FlagSiteEnvironment::controlLocks.count("siteRenameDialog") == 0);
    CHECK_THROWS_AS(f4->DismissSiteRename(), NullReferenceException);  // SiteRenameDialog is null now
    // onGameSceneLoadRequested closes both dialogs
    f4->RenameSite(nullptr);
    f4->flagPlaqueDialogOpen = true;
    FlagSiteEnvironment::controlLocks["FlagPlaqueDialog"] = "x";
    int terminated = 0;
    FlagSiteEnvironment::terminatePlaqueDialog = [&](FlagSite&) { ++terminated; };
    f4->onGameSceneLoadRequested(GameScenes::MAINMENU);
    CHECK_FALSE(f4->siteRenameDialogOpen);
    CHECK_FALSE(f4->flagPlaqueDialogOpen);
    CHECK(terminated == 1);
    CHECK(FlagSiteEnvironment::controlLocks.empty());
    // ReadPlaque needs the vessel's name: without a vessel it is a NullReferenceException
    CHECK_THROWS_AS(f4->ReadPlaque(), NullReferenceException);
}
