// WP-16 unit tests: staging (StageManager model, FindStageIndices / SetSeparationIndices), action groups (ActionGroupList.ToggleGroup,
// BaseAction statics), the decoupler modules, Part.decouple / Couple / Undock, DockedVesselInfo.
//
// The numerics that exist in a capture are gated by `capture.staging_sequence` (C5: Part.inverseStage and StageCount of 17 crafts).
// Everything else is derived from the decompiled source (the derivation is in the comment above each check) or taken from the game's
// own data: the .craft files of harness/inputs/Ships hold the staging the editor wrote (istg, dstg, sidx, sqor, sepI of every PART
// node = Part.inverseStage / defaultInverseStage / inStageIndex / manualStageOffset / separationIndex at save time), the part
// prefabs of the GameData mirror, the PhysProf crafts of analysis/physics/craft. No number is invented.
#include <algorithm>
#include <array>
#include <filesystem>
#include <map>
#include <set>
#include <fstream>

#include "nlohmann/json.hpp"

#include "editor_staging.hpp"
#include "resource_fixture.hpp"

#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/modules/vessel_module.hpp"
#include "ksp/save/game.hpp"
#include "ksp/modules/module_decouplers.hpp"
#include "ksp/parts/docked_vessel_info.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/resources/part_set.hpp"
#include "ksp/resources/resource_flow_host.hpp"
#include "ksp/staging/stage_manager.hpp"
#include "ksp/vessel/action_group_list.hpp"
#include "ksp/vessel/vessel.hpp"
#include "ksp/vessel/vessel_assembly.hpp"

using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::support;
namespace fs = std::filesystem;

namespace {

const std::string kHarness16 = KSPCORE_HARNESS_DIR;

// The stock game + PhysicsGlobals + Planetarium + the Kerbol system, as the flight scene has them (same set-up as test_vessel.cpp)
struct World16 {
    GameData data;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    World16() {
        SaveEnvironment::Reset();
        FlightRegistry::Clear();
        Vessel::SpawnedVessels().clear();
        GameData::Options o;
        o.gameRoot = ksp::testing::GameRoot();
        data.Load(o);
        SaveEnvironment& env = SaveEnvironment::Current();
        env.getPartInfoByName = [this](std::string_view n) { return data.parts.getPartInfoByName(n); };
        env.partExists = [this](std::string_view n) { return data.parts.getPartInfoByName(n) != nullptr; };
        env.newGuidHashCode = [] {
            static uint32_t s = 4242u;
            s = s * 1664525u + 1013904223u;
            return static_cast<int32_t>(s);
        };
        support::InstallUpgradeHooks(data);
        pg.physicsDatabaseFilename = kHarness16 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        Vessel::env = VesselEnvironment{};
        VesselModuleManager::CompileModules();
    }
    ~World16() {
        Vessel::SpawnedVessels().clear();
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
        SaveEnvironment::Reset();
        Vessel::env = VesselEnvironment{};
    }
    CelestialBody& Kerbin() { return *sys->GetBodyByName("Kerbin"); }
    Vector3f PlaceKerbinAtPad() {
        CelestialBody& k = Kerbin();
        k.position = Vector3d();
        const Vector3d pad = k.GetWorldSurfacePosition(-0.0972077486779459, -74.5576813534722, 75.0);
        k.position = Vector3d() - pad;
        return Vector3f();
    }
    std::function<const AvailablePart*(const std::string&)> Lookup() {
        return [this](const std::string& n) { return data.parts.getPartInfoByName(n); };
    }
    // A vessel assembled from one of the game's own craft files (analysis/physics/craft, harness/inputs/Ships)
    std::unique_ptr<Vessel> Craft(const std::string& path) {
        ShipConstruct ship;
        std::string err;
        REQUIRE_MESSAGE(support::LoadCraftFile(path, ship, err), err);  // through the upgrade pipeline like the game's loader
        AssembleOptions ao;
        ao.rootPosition = PlaceKerbinAtPad();
        auto v = AssembleVesselFromCraft(ship, Lookup(), ao);
        REQUIRE(v != nullptr);
        return v;
    }
    std::unique_ptr<Vessel> PhysProf(const char* name) { return Craft(kHarness16 + "/../analysis/physics/craft/" + name + ".craft"); }
};

// Stock crafts of harness/inputs/Ships (VAB, SPH) that load, sorted by file name, as (file, name)
std::vector<fs::path> StockCraftFiles() {
    std::vector<fs::path> files;
    for (const char* dir : {"VAB", "SPH"}) {
        const fs::path d = fs::path(KSPCORE_HARNESS_DIR) / "inputs" / "Ships" / dir;
        std::vector<fs::path> f;
        for (const auto& e : fs::directory_iterator(d))
            if (e.path().extension() == ".craft") f.push_back(e.path());
        std::sort(f.begin(), f.end());
        files.insert(files.end(), f.begin(), f.end());
    }
    return files;
}

}  // namespace


// ======================================================== small records, BaseAction statics, ActionGroupList

TEST_CASE("WP-16 KSPActionParam, KSPActionType, KSPActionGroup flags, PreferredInitialStage, DefaultIcons names") {
    // KSPActionType.cs: Activate, Deactivate, Toggle
    CHECK(static_cast<int>(KSPActionType::Activate) == 0);
    CHECK(static_cast<int>(KSPActionType::Deactivate) == 1);
    CHECK(static_cast<int>(KSPActionType::Toggle) == 2);
    // KSPActionParam.cs:21 ctor: group, type, cooldown 0; :17 Cooldown setter keeps the maximum (Mathf.Max)
    KSPActionParam p(static_cast<int64_t>(KSPActionGroup::Custom03), KSPActionType::Deactivate);
    CHECK(p.group == 0x200);
    CHECK(p.Type() == KSPActionType::Deactivate);
    CHECK(p.Cooldown() == 0.0f);
    p.SetCooldown(2.5f);
    CHECK(p.Cooldown() == 2.5f);
    p.SetCooldown(1.0f);  // lower: ignored
    CHECK(p.Cooldown() == 2.5f);
    p.SetCooldown(4.0f);
    CHECK(p.Cooldown() == 4.0f);
    p.SetCooldown(-3.0f);
    CHECK(p.Cooldown() == 4.0f);
    KSPActionParam q;  // default: Activate, no group
    CHECK(q.group == 0);
    CHECK(q.Type() == KSPActionType::Activate);

    // PreferredInitialStage.cs: FIRST, LAST, PARACHUTESTAGE, DEFAULT
    CHECK(static_cast<int>(PreferredInitialStage::FIRST) == 0);
    CHECK(static_cast<int>(PreferredInitialStage::LAST) == 1);
    CHECK(static_cast<int>(PreferredInitialStage::PARACHUTESTAGE) == 2);
    CHECK(static_cast<int>(PreferredInitialStage::DEFAULT) == 3);
    // ModifierStagingSituation.cs: CURRENT, UNSTAGED, STAGED
    CHECK(static_cast<int>(ModifierStagingSituation::CURRENT) == 0);
    CHECK(static_cast<int>(ModifierStagingSituation::UNSTAGED) == 1);
    CHECK(static_cast<int>(ModifierStagingSituation::STAGED) == 2);

    // BaseAction.cs:55 ActionGroupsLength = Enum.GetNames(KSPActionGroup).Length - 2 (19 names: REPLACEWITHDEFAULT, None, 17 groups)
    CHECK(BaseAction::ActionGroupsLength() == 17);
    // BaseAction.cs:394 GetGroupIndex: the bit index of a single flag below ActionGroupsLength; 0 for anything else
    for (int i = 0; i < 17; i++) CHECK(BaseAction::GetGroupIndex(int64_t{1} << i) == i);
    CHECK(BaseAction::GetGroupIndex(0) == 0);                    // None
    CHECK(BaseAction::GetGroupIndex(-1) == 0);                   // REPLACEWITHDEFAULT
    CHECK(BaseAction::GetGroupIndex(3) == 0);                    // Stage | Gear: not a single flag
    CHECK(BaseAction::GetGroupIndex(int64_t{1} << 17) == 0);     // beyond Custom10
    // KSPActionGroup values (KSPActionGroup.cs): Stage 1 ... Custom10 0x10000
    CHECK(static_cast<int>(KSPActionGroup::Stage) == 1);
    CHECK(static_cast<int>(KSPActionGroup::SAS) == 0x10);
    CHECK(static_cast<int>(KSPActionGroup::Custom01) == 0x80);
    CHECK(static_cast<int>(KSPActionGroup::Custom10) == 0x10000);

    // DefaultIcons.cs: the 23 names Part.Start parses stagingIcon into
    for (const char* n : {"CUSTOM", "MYSTERY_PART", "LIQUID_ENGINE", "SOLID_BOOSTER", "COMMAND_POD", "DECOUPLER_VERT", "DECOUPLER_HOR", "FUEL_TANK",
                          "PARACHUTES", "WINGLETS", "SAS", "STRUT", "STRUT_CONNECTOR", "ADV_SAS", "RCS_TANK", "RCS_MODULE", "FUEL_LINE", "LANDING_LEG",
                          "REACTION_WHEEL", "WHEEL", "LANDING_GEAR", "PROBE", "SCIENCE_GENERIC"})
        CHECK(IsDefaultIconName(n));
    CHECK_FALSE(IsDefaultIconName("liquid_engine"));  // Enum.Parse is case-sensitive
    CHECK_FALSE(IsDefaultIconName(""));
    CHECK_FALSE(IsDefaultIconName("BOGUS"));
}

namespace {
// Records the KSPActionParams a bound [KSPAction] receives
struct ActionLog {
    std::vector<std::pair<int64_t, KSPActionType>> calls;
    float cooldown = 0.0f;
    void Bind(BaseAction* a) {
        a->onEvent = [this](KSPActionParam& p) {
            calls.emplace_back(p.group, p.Type());
            p.SetCooldown(cooldown);
        };
    }
};
}  // namespace

TEST_CASE("WP-16 ActionGroupList.ToggleGroup / SetGroup: flag flipping, the KSPActionParam handed to the actions, cooldown, override groups") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    Planetarium planetarium;
    planetarium.Awake();
    Planetarium::SetUniversalTime(100.0);
    // Separator.0 / Separator.1 (cfg Separator_0 / _1) carry ModuleToggleCrossfeed (ToggleAction / EnableAction / DisableAction, group None by default)
    auto a = g.New("Separator.0");
    auto b = g.New("Separator.1");
    Vessel v;
    v.parts = {a.get(), b.get()};
    PartModule* ma = a->FindModuleOfClass("ModuleToggleCrossfeed");
    PartModule* mb = b->FindModuleOfClass("ModuleToggleCrossfeed");
    REQUIRE(ma != nullptr);
    REQUIRE(mb != nullptr);
    BaseAction* ta = ma->Actions()["ToggleAction"];
    BaseAction* tb = mb->Actions()["ToggleAction"];
    BaseAction* ea = ma->Actions()["EnableAction"];
    REQUIRE(ta != nullptr);
    REQUIRE(ta->actionGroup == 0);  // KSPAction default group None
    ActionLog la, lb, le;
    la.Bind(ta);
    lb.Bind(tb);
    le.Bind(ea);
    const int64_t c1 = static_cast<int64_t>(KSPActionGroup::Custom01);
    const int64_t gear = static_cast<int64_t>(KSPActionGroup::Gear);
    ta->actionGroup = c1;
    tb->actionGroup = c1 | gear;  // a bit mask: matches both groups (BaseAction.AddAction tests `(action.actionGroup & group) != 0`)
    ea->actionGroup = gear;

    ActionGroupList& ag = v.ActionGroups();
    CHECK_FALSE(ag[KSPActionGroup::Custom01]);
    // ActionGroupList.cs:50: first toggle -> true, Activate
    ag.ToggleGroup(KSPActionGroup::Custom01);
    CHECK(ag[KSPActionGroup::Custom01]);
    REQUIRE(la.calls.size() == 1);
    CHECK(la.calls[0].first == c1);
    CHECK(la.calls[0].second == KSPActionType::Activate);
    CHECK(lb.calls.size() == 1);
    CHECK(le.calls.empty());  // Gear action, other group
    // second toggle -> false, Deactivate
    ag.ToggleGroup(KSPActionGroup::Custom01);
    CHECK_FALSE(ag[KSPActionGroup::Custom01]);
    REQUIRE(la.calls.size() == 2);
    CHECK(la.calls[1].second == KSPActionType::Deactivate);
    // the Gear toggle reaches tb (mask) and ea
    ag.ToggleGroup(KSPActionGroup::Gear);
    CHECK(ag[KSPActionGroup::Gear]);
    CHECK(lb.calls.size() == 3);
    CHECK(la.calls.size() == 2);
    CHECK(le.calls.size() == 1);
    // SetGroup (:87): only toggles when the state differs
    ag.SetGroup(KSPActionGroup::Gear, true);
    CHECK(le.calls.size() == 1);
    ag.SetGroup(KSPActionGroup::Gear, false);
    CHECK_FALSE(ag[KSPActionGroup::Gear]);
    REQUIRE(le.calls.size() == 2);
    CHECK(le.calls[1].second == KSPActionType::Deactivate);

    // cooldown: cooldownTimes[idx] = UT + param.Cooldown; a toggle before it only logs
    la.cooldown = 3.0f;
    la.calls.clear();
    ag.ToggleGroup(KSPActionGroup::Custom01);
    CHECK(ag[KSPActionGroup::Custom01]);
    CHECK(ag.cooldownTimes[7] == 103.0);  // index 7 = Custom01 (bit 0x80)
    CHECK(ag.CooldownTimeRemaining(KSPActionGroup::Custom01) == 3.0);
    Planetarium::SetUniversalTime(101.0);
    CHECK(ag.CooldownTimeRemaining(KSPActionGroup::Custom01) == 2.0);
    ag.ToggleGroup(KSPActionGroup::Custom01);  // still cooling down: nothing happens
    CHECK(ag[KSPActionGroup::Custom01]);
    CHECK(la.calls.size() == 1);
    ag.SetGroup(KSPActionGroup::Custom01, false);  // SetGroup goes through ToggleGroup: refused as well
    CHECK(la.calls.size() == 1);
    Planetarium::SetUniversalTime(103.0);  // `UT < cooldown` is false at the boundary
    CHECK(ag.CooldownTimeRemaining(KSPActionGroup::Custom01) == 0.0);
    ag.ToggleGroup(KSPActionGroup::Custom01);
    CHECK_FALSE(ag[KSPActionGroup::Custom01]);
    CHECK(la.calls.size() == 2);

    // BaseAction.AddAction filters: disabled module, inactive action, advancedTweakable
    la.cooldown = 0.0f;
    Planetarium::SetUniversalTime(110.0);
    la.calls.clear();
    lb.calls.clear();
    ma->isEnabled = false;
    ag.ToggleGroup(KSPActionGroup::Custom01);
    CHECK(la.calls.empty());
    CHECK(lb.calls.size() == 1);
    ma->isEnabled = true;
    tb->active = false;
    ag.ToggleGroup(KSPActionGroup::Custom01);
    CHECK(lb.calls.size() == 1);
    tb->active = true;
    ta->advancedTweakable = true;  // hidden unless GameSettings.ADVANCED_TWEAKABLES
    la.calls.clear();
    ag.ToggleGroup(KSPActionGroup::Custom01);
    CHECK(la.calls.empty());
    GameSettings::ADVANCED_TWEAKABLES = true;
    ag.ToggleGroup(KSPActionGroup::Custom01);
    CHECK(la.calls.size() == 1);
    GameSettings::ADVANCED_TWEAKABLES = false;
    ta->advancedTweakable = false;

    // BaseAction.CreateActionList include / exclude (:236 AddAction): include=false lists the actions NOT in the group
    std::vector<BaseAction*> in = BaseAction::CreateActionList(v.parts, c1, 0, false, true);
    std::vector<BaseAction*> out = BaseAction::CreateActionList(v.parts, c1, 0, false, false);
    CHECK(std::find(in.begin(), in.end(), ta) != in.end());
    CHECK(std::find(in.begin(), in.end(), tb) != in.end());
    CHECK(std::find(out.begin(), out.end(), ta) == out.end());
    CHECK(std::find(out.begin(), out.end(), ea) != out.end());
    // CreateGroupList(parts) (:308): per group, does any module action sit in it
    std::vector<bool> gl = BaseAction::CreateGroupList(v.parts, 0);
    REQUIRE(gl.size() == 17);
    CHECK(gl[7]);   // Custom01
    CHECK(gl[1]);   // Gear
    CHECK_FALSE(gl[0]);  // Stage
    // FireAction (:379): invokes every listed action that is active, with fullOnly / editor filters
    la.calls.clear();
    BaseAction::FireAction(v.parts, c1, 0, KSPActionType::Toggle);
    REQUIRE(la.calls.size() == 1);
    CHECK(la.calls[0].second == KSPActionType::Toggle);
    la.calls.clear();
    ta->activeEditor = false;
    BaseAction::FireAction(v.parts, c1, 0, KSPActionType::Toggle, false, true);  // editor scene: activeEditor=false actions skipped
    CHECK(la.calls.empty());
    ta->activeEditor = true;
    ta->requireFullControl = true;
    BaseAction::FireAction(v.parts, c1, 0, KSPActionType::Toggle, true, false);  // TWEAKABLES_FULLONLY locked
    CHECK(la.calls.empty());
    ta->requireFullControl = false;

    // override groups (ADDITIONAL_ACTION_GROUPS): groupStates[o-1] is toggled; `groups` follows only when the override is neither
    // OverrideDefault nor action-controlling (:50 `if (num > 0 && !(flag || flag2)) groups[idx] = !groups[idx]`)
    GameSettings::ADDITIONAL_ACTION_GROUPS = true;
    Planetarium::SetUniversalTime(200.0);
    ag.cooldownTimes.assign(ag.cooldownTimes.size(), 0.0);
    la.calls.clear();
    ta->overrideGroups[0] = c1;  // override group 1 assignment
    v.GroupOverride = 1;
    const bool groupsBefore = ag[KSPActionGroup::Custom01];
    ag.ToggleGroup(KSPActionGroup::Custom01);
    CHECK(ag.groupStates[0][7]);
    CHECK(ag[KSPActionGroup::Custom01] == !groupsBefore);  // neither OverrideDefault nor OverrideActionControl: groups flips too
    CHECK(la.calls.size() == 1);
    // OverrideActionControl[0] contains the group: groups stays
    v.OverrideActionControl[0] = KSPActionGroup::Custom01;
    const bool g1 = ag[KSPActionGroup::Custom01];
    ag.ToggleGroup(KSPActionGroup::Custom01);
    CHECK_FALSE(ag.groupStates[0][7]);
    CHECK(ag[KSPActionGroup::Custom01] == g1);
    v.GroupOverride = 0;
    GameSettings::ADDITIONAL_ACTION_GROUPS = false;
    ta->overrideGroups[0] = 0;
}

// ======================================================== module staging queries (PartModule.cs:272-345 and the per-class overrides)

namespace {
std::shared_ptr<PartModule> MakeModule(const char* cls) {
    const ModuleClassInfo* info = GetPartModuleClass(cls);
    REQUIRE_MESSAGE(info != nullptr, cls);
    return ModuleFactory::Create(*info);
}
}  // namespace

TEST_CASE("WP-16 PartModule.IsStageable / StagingEnabled / StagingToggleEnabled*: the reflected OnActive rule and the per-class overrides") {
    if (!GameAvailable()) return;
    Game15 g;
    // ReflectedAttributes.isStageable (PartModule.cs:27-50): the class (or a base) declares a public OnActive. Without an override of
    // IsStageable() the answer is that flag.
    for (const char* cls : {"ModuleParachute", "ModuleProceduralFairing", "ModuleOrbitalScanner", "ModuleServiceModule", "LaunchClamp"})
        CHECK_MESSAGE(MakeModule(cls)->IsStageable(), cls);
    for (const char* cls : {"ModuleCommand", "ModuleResourceIntake", "ModuleReactionWheel", "ModuleDataTransmitter", "ModuleToggleCrossfeed"})
        CHECK_FALSE_MESSAGE(MakeModule(cls)->IsStageable(), cls);
    // classes that override IsStageable(): ModuleEngines true (:2137), ModuleGimbal false (:326), ModuleTestSubject false (:200),
    // ModuleEvaChute false (:136) although they declare OnActive
    CHECK(MakeModule("ModuleEngines")->IsStageable());
    CHECK(MakeModule("ModuleEnginesFX")->IsStageable());  // inherits
    CHECK_FALSE(MakeModule("ModuleGimbal")->IsStageable());
    CHECK_FALSE(MakeModule("ModuleTestSubject")->IsStageable());
    CHECK_FALSE(MakeModule("ModuleEvaChute")->IsStageable());
    // ModuleRCS.cs:746 / ModuleJettison.cs:641: `if (!stagingEnabled && !stagingToggleEnabledEditor) return stagingToggleEnabledFlight; return true;`
    for (const char* cls : {"ModuleRCS", "ModuleRCSFX", "ModuleJettison"}) {
        auto m = MakeModule(cls);
        for (int mask = 0; mask < 8; mask++) {
            const bool en = (mask & 1) != 0, ed = (mask & 2) != 0, fl = (mask & 4) != 0;
            m->Set<bool>("stagingEnabled", en);
            m->Set<bool>("stagingToggleEnabledEditor", ed);
            m->Set<bool>("stagingToggleEnabledFlight", fl);
            const bool expected = (!en && !ed) ? fl : true;
            CHECK_MESSAGE(m->IsStageable() == expected, cls << " mask " << mask);
        }
    }
    // ModuleDockingNode.cs:1949-1970: stageable, enabled and togglable only while `staged`
    {
        auto m = MakeModule("ModuleDockingNode");
        for (int mask = 0; mask < 4; mask++) {
            const bool staged = (mask & 1) != 0, en = (mask & 2) != 0;
            m->Set<bool>("staged", staged);
            m->Set<bool>("stagingEnabled", en);
            CHECK(m->IsStageable() == staged);
            CHECK(m->StagingEnabled() == (en ? staged : false));
            CHECK(m->StagingToggleEnabledEditor() == staged);
            CHECK(m->StagingToggleEnabledFlight() == staged);
        }
    }
    // ModuleScienceExperiment.cs:1183-1195: `useStaging` gates both
    {
        auto m = MakeModule("ModuleScienceExperiment");
        for (int mask = 0; mask < 4; mask++) {
            const bool us = (mask & 1) != 0, en = (mask & 2) != 0;
            m->Set<bool>("useStaging", us);
            m->Set<bool>("stagingEnabled", en);
            CHECK(m->IsStageable() == us);
            CHECK(m->StagingEnabled() == (us ? en : false));
        }
    }
    // the base: StagingEnabled = the stagingEnabled field (default true); toggles = their fields
    {
        auto m = MakeModule("ModuleParachute");
        CHECK(m->StagingEnabled());
        m->Set<bool>("stagingEnabled", false);
        CHECK_FALSE(m->StagingEnabled());
        m->Set<bool>("stagingToggleEnabledEditor", false);
        CHECK_FALSE(m->StagingToggleEnabledEditor());
    }
}

TEST_CASE("WP-16 Part.Start staging block: the OnStart stagingIcon defaults, UpdateStageability, the stack icon") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    // module OnStart `if (part.stagingIcon == string.Empty && overrideStagingIconIfBlank) part.stagingIcon = <icon>`:
    // ModuleEngines.cs:1653 LIQUID_ENGINE, ModuleParachute.cs:310 PARACHUTES, LaunchClamp.cs:159 STRUT, ModuleDecouple.cs:46
    // DECOUPLER_VERT (these parts have no stagingIcon in their cfg; the engine, decoupler and docking-port cfgs below do or do not)
    struct Case { const char* part; const char* icon; bool stagingOn; };
    const Case cases[] = {
        {"liquidEngine2", "LIQUID_ENGINE", true},
        {"liquidEngineMainsail.v2", "LIQUID_ENGINE", true},
        {"solidBooster1-1", "SOLID_BOOSTER", true},   // the cfg's own stagingIcon wins
        {"parachuteSingle", "PARACHUTES", true},
        {"launchClamp1", "STRUT", true},
        {"Decoupler.1", "DECOUPLER_VERT", true},
        {"radialDecoupler1-2", "DECOUPLER_HOR", true},  // cfg; ModuleAnchoredDecoupler would default to DECOUPLER_HOR
        {"fuelTank", "", false},                        // no stageable module, no icon
        {"mk1pod.v2", "", false},
        {"dockingPort2", "DECOUPLER_VERT", false},      // cfg icon and `stagingEnabled = False` (Utility/dockingPort/dockingPort.cfg:34): no stagingOn
    };
    for (const Case& c : cases) {
        auto p = g.New(c.part);
        p->ModulesOnStart(StartState::Editor);
        CHECK_MESSAGE(p->fields.stagingIcon.value_or("<null>") == c.icon, c.part);
        p->StartStaging();
        // Part.cs:7777: with an icon, stagingOn = any stageable module that is enabled; without one the field keeps its default (true)
        if (std::string(c.icon).empty()) {
            CHECK_FALSE(p->hasStagingIcon());
            CHECK_FALSE(p->HasStageIcon());
        } else {
            CHECK_MESSAGE(p->fields.stagingOn == c.stagingOn, c.part);
            CHECK_MESSAGE(p->HasStageIcon() == c.stagingOn, c.part);
        }
    }
    // an unknown stagingIcon name: Enum.Parse throws, Part.Start logs and clears the field (Part.cs:1735)
    auto p = g.New("liquidEngine2");
    p->fields.stagingIcon = std::string("NOT_AN_ICON");
    p->StartStaging();
    CHECK(quiet.Has(log::Level::Error, "NOT_AN_ICON"));
    CHECK(p->fields.stagingIcon.value_or("<null>") == "");
    CHECK_FALSE(p->HasStageIcon());  // CreateIcon needs a stagingIcon
    // the docking port is stageable while `staged` (default true), enabled by stagingEnabled && staged
    auto d = g.New("dockingPort2");
    d->ModulesOnStart(StartState::Editor);
    PartModule* dn = d->FindModuleOfClass("ModuleDockingNode");
    CHECK(dn->IsStageable());
    CHECK_FALSE(dn->StagingEnabled());  // the cfg's stagingEnabled = False
    dn->Set<bool>("stagingEnabled", true);
    d->StartStaging();
    CHECK(d->fields.stagingOn);
    CHECK(d->HasStageIcon());
    // the stand-in IStageSeparators: LaunchClamp.cs:369 inverseStage, ModuleDockingNode.cs:1944 fallback + 1
    auto clamp = g.New("launchClamp1");
    clamp->fields.inverseStage = 5;
    auto* cs = dynamic_cast<IStageSeparator*>(clamp->FindModuleOfClass("LaunchClamp"));
    REQUIRE(cs != nullptr);
    CHECK(cs->GetStageIndex(-1) == 5);
    CHECK(cs->GetStageIndex(99) == 5);
    auto* ds = dynamic_cast<IStageSeparator*>(d->FindModuleOfClass("ModuleDockingNode"));
    REQUIRE(ds != nullptr);
    CHECK(ds->GetStageIndex(-1) == 0);
    CHECK(ds->GetStageIndex(4) == 5);
}

// ======================================================== the decoupler modules

namespace {
ConfigNode::NodePtr CfgModule(const char* relCfg, const char* moduleName) {
    ConfigNode::NodePtr root = ConfigNode::Load(std::string(ksp::testing::GameRoot()) + "/GameData/" + relCfg);
    REQUIRE(root);
    ConfigNode* part = root->GetNode("PART");
    REQUIRE(part != nullptr);
    for (const auto& n : part->nodes())
        if (n->name == "MODULE" && n->GetValue("name") && *n->GetValue("name") == moduleName) return n;
    FAIL("module " << moduleName);
    return nullptr;
}
}  // namespace

TEST_CASE("WP-16 ModuleDecouple / ModuleAnchoredDecoupler: cfg fields, OnStart, IStageSeparator, staging queries, persistence") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    // --- Decoupler.1 (Squad/Parts/Coupling/Decoupler_1.cfg: ejectionForce = 100, isOmniDecoupler = false, explosiveNodeID = top)
    auto cfg = CfgModule("Squad/Parts/Coupling/Decoupler_1.cfg", "ModuleDecouple");
    auto dec = g.New("Decoupler.1");
    auto* d = dynamic_cast<ModuleDecouple*>(dec->FindModuleOfClass("ModuleDecouple"));
    REQUIRE(d != nullptr);
    CHECK(d->Get<float>("ejectionForce") == 100.0f);
    CHECK(*cfg->GetValue("ejectionForce") == "100");
    CHECK(d->Get<bool>("isOmniDecoupler") == false);
    CHECK(d->Get<std::string>("explosiveNodeID") == "top");
    // C# initialisers (ModuleDecouplerBase.cs:8-34, ModuleDecouple.cs:22-29)
    CHECK(d->Get<float>("ejectionForcePercent") == 100.0f);
    CHECK_FALSE(d->Get<bool>("isDecoupled"));
    CHECK(d->Get<bool>("staged"));
    CHECK(d->Get<bool>("partDecoupled"));
    CHECK_FALSE(d->Get<bool>("isEnginePlate"));
    CHECK(d->Get<std::string>("fxGroupName") == "decouple");
    CHECK(d->Get<bool>("automaticDir"));
    CHECK(d->Get<Vector3f>("explosiveDir").y == -1.0f);  // Vector3.down
    // OnAwake: the "decouple" FX group exists (cfg fx_gasBurst_white ..., decouple)
    // a stack: tank (parent) bottom <-> Decoupler.1 top
    auto tank = g.New("fuelTank");
    Attach(*tank, "bottom", *dec, "top");
    d->OnStart(StartState::Editor);
    REQUIRE(d->ExplosiveNode() != nullptr);
    CHECK(d->ExplosiveNode()->id == "top");  // OnStart: FindAttachNode(explosiveNodeID)
    CHECK(dec->fields.stagingIcon.value_or("") == "DECOUPLER_VERT");
    // IStageSeparatorChild.PartDetaches (:218): the part on the explosive node is what detaches; partDecoupled is the return value
    std::vector<Part*> detaches;
    CHECK(d->PartDetaches(detaches));
    REQUIRE(detaches.size() == 1);
    CHECK(detaches[0] == tank.get());
    CHECK_FALSE(d->IsEnginePlate());
    // IStageSeparator.GetStageIndex (:209): own inverseStage, the parent's while staging is disabled and a parent exists
    dec->fields.inverseStage = 3;
    tank->fields.inverseStage = 1;
    CHECK(d->GetStageIndex(-1) == 3);
    d->Set<bool>("stagingEnabled", false);
    CHECK(d->GetStageIndex(-1) == 1);
    d->Set<bool>("stagingEnabled", true);
    // staging queries (:259-280): stageable == staged, enabled == base && staged, toggle(editor) == staged
    CHECK(d->IsStageable());
    CHECK(d->StagingEnabled());
    d->Set<bool>("staged", false);
    CHECK_FALSE(d->IsStageable());
    CHECK_FALSE(d->StagingEnabled());
    CHECK_FALSE(d->StagingToggleEnabledEditor());
    d->Set<bool>("staged", true);
    d->Set<bool>("stagingEnabled", false);
    CHECK_FALSE(d->StagingEnabled());
    CHECK(d->IsStageable());
    d->Set<bool>("stagingEnabled", true);
    // texts: PartModule's stagingEnableText when set, else the localized default
    CHECK(d->GetStagingEnableText() == KSP::Localization::Localizer::Format("#autoLOC_240328"));
    CHECK(d->GetStagingDisableText() == KSP::Localization::Localizer::Format("#autoLOC_240329"));
    d->Set<std::string>("stagingEnableText", "Arm");
    CHECK(d->GetStagingEnableText() == "Arm");
    d->Set<std::string>("stagingEnableText", "");
    // module title: omni vs plain (:190)
    CHECK(d->GetModuleTitle() == KSP::Localization::Localizer::Format("#autoLOC_6001040"));
    d->Set<bool>("isOmniDecoupler", true);
    CHECK(d->GetModuleTitle() == KSP::Localization::Localizer::Format("#autoLOC_6001039"));
    CHECK(d->GetModuleDisplayName() == KSP::Localization::Localizer::Format("#autoLOC_6001039"));
    d->Set<bool>("isOmniDecoupler", false);
    // events / actions: Decouple [KSPEvent] guiActive, DecoupleAction [KSPAction] activeEditor = false
    BaseEvent* ev = d->Events()["Decouple"];
    REQUIRE(ev != nullptr);
    CHECK(ev->guiActive);
    BaseAction* act = d->Actions()["DecoupleAction"];
    REQUIRE(act != nullptr);
    CHECK_FALSE(act->activeEditor);
    // persistence: isDecoupled, ejectionForcePercent and stagingEnabled are [KSPField(isPersistant)]; ejectionForce and staged are not
    d->Set<bool>("isDecoupled", true);
    d->Set<float>("ejectionForcePercent", 55.0f);
    d->Set<bool>("stagingEnabled", false);
    ConfigNode saved("MODULE");
    d->Save(saved);
    CHECK(*saved.GetValue("isDecoupled") == "True");
    CHECK(*saved.GetValue("ejectionForcePercent") == "55");
    CHECK(*saved.GetValue("stagingEnabled") == "False");
    CHECK_FALSE(saved.HasValue("ejectionForce"));
    CHECK_FALSE(saved.HasValue("staged"));
    auto dec2 = g.New("Decoupler.1");
    auto* d2 = dynamic_cast<ModuleDecouple*>(dec2->FindModuleOfClass("ModuleDecouple"));
    d2->Load(saved);
    CHECK(d2->Get<bool>("isDecoupled"));
    CHECK(d2->Get<float>("ejectionForcePercent") == 55.0f);
    CHECK_FALSE(d2->Get<bool>("stagingEnabled"));
    // OnStart of an already decoupled module deactivates the Decouple event (ModuleDecouple.cs:48)
    auto tank2 = g.New("fuelTank");
    Attach(*tank2, "bottom", *dec2, "top");
    d2->OnStart(StartState::Editor);
    CHECK_FALSE(d2->Events()["Decouple"]->active);
    // GetInfo / GetPrimaryField: ejectionForce.ToString("0.0###")
    CHECK(d->GetInfo() == KSP::Localization::Localizer::Format("#autoLOC_240611", std::vector<std::string>{"100.0"}));
    CHECK(d->GetPrimaryField() == KSP::Localization::Localizer::Format("#autoLOC_240300", std::vector<std::string>{"100.0"}));

    // --- radialDecoupler1-2 (ModuleAnchoredDecoupler, cfg ejectionForce = 450, explosiveNodeID = srf, stagingIcon = DECOUPLER_HOR)
    auto cfgA = CfgModule("Squad/Parts/Utility/decouplerRadialHDM/decouplerRadialHDM.cfg", "ModuleAnchoredDecoupler");
    auto rad = g.New("radialDecoupler1-2");
    auto* a = dynamic_cast<ModuleAnchoredDecoupler*>(rad->FindModuleOfClass("ModuleAnchoredDecoupler"));
    REQUIRE(a != nullptr);
    CHECK(a->Get<float>("ejectionForce") == 450.0f);
    CHECK(*cfgA->GetValue("ejectionForce") == "450");
    CHECK(a->Get<std::string>("explosiveNodeID") == "srf");
    CHECK(a->Get<std::string>("anchorName") == "anchor");
    CHECK(a->Get<Vector3f>("explosiveDir").x == 1.0f);  // Vector3.right
    auto tank3 = g.New("fuelTank");
    rad->srfAttachNode->attachedPart = tank3.get();
    rad->setParent(tank3.get());
    a->OnStart(StartState::Editor);
    CHECK(a->ExplosiveNode() == rad->srfAttachNode.get());  // "srf" -> part.srfAttachNode
    std::vector<Part*> det2;
    CHECK(a->PartDetaches(det2));
    REQUIRE(det2.size() == 1);
    CHECK(det2[0] == tank3.get());
    CHECK(rad->fields.stagingIcon.value_or("") == "DECOUPLER_HOR");
    // an explosive node that does not exist: logged, no crash (ModuleDecouplerBase.cs:93)
    auto bad = g.New("Decoupler.1");
    auto* bd = dynamic_cast<ModuleDecouple*>(bad->FindModuleOfClass("ModuleDecouple"));
    bd->Set<std::string>("explosiveNodeID", "nope");
    bd->OnStart(StartState::Editor);
    CHECK(bd->ExplosiveNode() == nullptr);
    CHECK(quiet.Has(log::Level::Error, "No attachnode found with id nope"));
    std::vector<Part*> none;
    CHECK(bd->PartDetaches(none));
    CHECK(none.empty());
}

// ======================================================== the staging sequence of the stock crafts

namespace {
// Part.cs:7777 / StageManager.cs:1542: the stage a part without its own icon takes is the inverseStage of its nearest ancestor with an icon
// that carries over (inverseStageCarryover), -1 at the root (setSeparationIndices carriedInverseStage)
int CarriedStage(const Part& p) {
    for (const Part* a = p.parent; a != nullptr; a = a->parent)
        if ((a->hasStagingIcon() || a->HasStageIcon()) && a->fields.stagingOn && a->fields.inverseStageCarryover) return a->fields.inverseStage;
    return -1;
}
bool HasIcon(const Part& p) { return (p.hasStagingIcon() || p.HasStageIcon()) && p.fields.stagingOn; }
bool IsPhysProf(const fs::path& f) { return f.filename().string().rfind("PhysProf-", 0) == 0; }
}  // namespace

TEST_CASE("WP-16 staging sequence of the 51 stock-part crafts of harness/inputs/Ships: istg of the icon parts, dstg of every part, sepI of the separators") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    // 51 crafts load with stock parts only; 48 were written by the game's editor, 3 (PhysProf-S/M/L) by this project's tools: their
    // stage fields are not editor output (they are gated by capture.staging_sequence instead)
    size_t crafts = 0, written = 0, parts = 0, iconParts = 0, plainParts = 0, separators = 0, sepOk = 0;
    size_t dstgParts = 0, iconPartsWritten = 0;
    std::vector<std::string> sepExceptions, dstgCrafts;
    for (const fs::path& f : StockCraftFiles()) {
        ShipConstruct ship;
        std::string err;
        if (!LoadCraftFile(f.string(), ship, err)) continue;
        CraftTree tree = BuildCraftTree(g.data, ship);
        if (!tree.ok) continue;
        CAPTURE(f.filename().string());
        crafts++;
        const bool editorWritten = !IsPhysProf(f);
        written += editorWritten;
        parts += tree.parts.size();
        {
            EditorSession editor(tree);
            EditorSession::StartParts(tree);
            editor.Load();
            for (size_t i = 0; i < tree.parts.size(); i++) {
                Part& p = *tree.parts[i];
                const auto& d = ship.parts[i];
                if (HasIcon(p)) {
                    iconParts++;
                    // the part's own stage (istg) is what the editor wrote: SortIcons put the icon back into the stage group of its inverseStage
                    if (editorWritten) {
                        iconPartsWritten++;
                        CHECK_MESSAGE(p.fields.inverseStage == d.inverseStage, "part " << i << " " << p.gameObjectName);
                    }
                } else {
                    plainParts++;
                    // StageManager.cs:1550: no icon -> the carried stage of the nearest ancestor icon (-1 above the first)
                    CHECK_MESSAGE(p.fields.inverseStage == CarriedStage(p), "part " << i << " " << p.gameObjectName);
                }
                const bool sepr = p.FindModuleOfClass("ModuleDecouple") || p.FindModuleOfClass("ModuleAnchoredDecoupler") || p.FindModuleOfClass("LaunchClamp");
                if (sepr && editorWritten) {
                    separators++;
                    if (p.fields.separationIndex == d.separationIndex) sepOk++;
                    else sepExceptions.push_back(f.filename().string() + ":" + p.gameObjectName + ":" + std::to_string(i));
                }
            }
            // the pipeline is idempotent: the sort has nothing left to change
            std::vector<std::array<int, 3>> before;
            for (auto& p : tree.parts) before.push_back({p->fields.inverseStage, p->fields.separationIndex, p->fields.manualStageOffset});
            StageManager::SetSeparationIndices();
            editor.Load();
            for (size_t i = 0; i < tree.parts.size(); i++) {
                const std::array<int, 3> now{tree.parts[i]->fields.inverseStage, tree.parts[i]->fields.separationIndex, tree.parts[i]->fields.manualStageOffset};
                CHECK(now == before[i]);
            }
        }
        if (editorWritten) {
            // GenerateStagingSequence's first half (StageManager.cs:1408 FindStageIndices) computes defaultInverseStage = the dstg the
            // editor wrote, for every part, from stageOffset / stageBefore / stageAfter / childStageOffset alone
            CraftTree t2 = BuildCraftTree(g.data, ship);
            EditorSession e2(t2);
            EditorSession::StartParts(t2);
            StageManager::FindStageIndices(e2.host.root, -1, -1);
            size_t dstgBad = 0;
            for (size_t i = 0; i < t2.parts.size(); i++) {
                dstgParts++;
                dstgBad += t2.parts[i]->fields.defaultInverseStage != ship.parts[i].defaultInverseStage;
                // with the editor's manualStageOffset (sqor) an icon part's inverseStage is manualStageOffset, else dstg (:1431)
                if (HasIcon(*t2.parts[i]) && f.filename() != "Prospector Rover.craft") {
                    const int man = ship.parts[i].manualStageOffset;
                    CHECK_MESSAGE(t2.parts[i]->fields.inverseStage == (man != -1 ? man : ship.parts[i].defaultInverseStage), "inv part " << i);
                }
            }
            if (dstgBad != 0) dstgCrafts.push_back(f.filename().string() + " " + std::to_string(dstgBad) + "/" + std::to_string(t2.parts.size()));
        }
    }
    MESSAGE("crafts " << crafts << " (editor-written " << written << "), parts " << parts << ", icon parts " << iconParts << ", plain parts " << plainParts
                      << ", separators " << separators << " (sepI equal: " << sepOk << "), dstg parts compared " << dstgParts << ", icon parts of the editor-written crafts " << iconPartsWritten);
    CHECK(crafts == 51);
    CHECK(written == 48);
    // dstg of every part of 47 of the 48 editor-written crafts equals FindStageIndices' defaultInverseStage. The exception is
    // Prospector Rover.craft (KSP 1.2.0), a rover whose root part is the stack decoupler: its file keeps dstg 1 for the root and 0 for the
    // others, the values from before the decoupler became the root (the game does not recompute dstg when a craft loads)
    REQUIRE(dstgCrafts.size() == 1);
    CHECK(dstgCrafts[0] == "Prospector Rover.craft 19/19");
    // 120 of 122 separators carry the sepI the editor wrote (the part's own stage); the 2 exceptions are the two staging-disabled
    // smallHardpoints of Osprey.craft whose file keeps the value from before staging was switched off (their GetStageIndex is now the parent's)
    CHECK(separators == sepOk + sepExceptions.size());
    CHECK(sepExceptions.size() == 2);
    for (const auto& e : sepExceptions) CHECK(e.rfind("Osprey.craft:smallHardpoint", 0) == 0);
}

TEST_CASE("WP-16 separationIndex and inverseStage reproduce the capture's decoupleStage / activationStage of the non-engine, non-decoupler parts") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    // C5 `parts` (VesselDeltaV, Kerbin): for a part that is neither an engine nor a decoupler DeltaVPartInfo.CalculateStagingValues
    // (DeltaVPartInfo.cs:231-258) is
    //     activationStage = part.inverseStage
    //     num = min(part.separationIndex, activationStage)
    //     decoupleStage = max{ s in S : s <= num } (or -1)             [GetSeparationStage, :168]
    //     if (activationStage < decoupleStage) decoupleStage = activationStage
    // with S = for every decoupler (ModuleDecouple / ModuleAnchoredDecoupler): separationIndex, and inverseStage when
    // PartDetaches() is false (VesselDeltaV.cs:557 CalculateSeparationStageIndexes). The capture is the game's, so this ties the
    // port's separationIndex to values it computed with its own.
    std::ifstream fin(kHarness16 + "/captures/out2/deltav_readout.json", std::ios::binary);
    REQUIRE(fin.good());
    const nlohmann::json cap = nlohmann::json::parse(fin);
    const nlohmann::json* craftT = nullptr;
    const nlohmann::json* partsT = nullptr;
    for (const auto& t : cap.at("tables")) {
        if (t.at("name") == "craft") craftT = &t;
        if (t.at("name") == "parts") partsT = &t;
    }
    REQUIRE(craftT != nullptr);
    REQUIRE(partsT != nullptr);
    auto col = [](const nlohmann::json& t, const char* c) {
        size_t i = 0;
        for (const auto& cc : t.at("columns")) {
            if (cc.at("name") == c) return i;
            i++;
        }
        FAIL("column " << c);
        return size_t(0);
    };
    const size_t cCraft = col(*partsT, "craft_id"), cIdx = col(*partsT, "part_index"), cDec = col(*partsT, "decoupleStage"), cAct = col(*partsT, "activationStage"),
                 cEng = col(*partsT, "isEngine"), cIsDec = col(*partsT, "isDecoupler");
    const size_t cPath = col(*craftT, "path");
    size_t checked = 0, crafts = 0;
    for (const auto& cr : craftT->at("rows")) {
        const std::string id = cr[0].get<std::string>();
        const std::string rel = cr[cPath].get<std::string>();
        fs::path file = fs::path(KSPCORE_HARNESS_DIR) / "inputs" / rel;
        if (!fs::exists(file)) file = fs::path(KSPCORE_HARNESS_DIR) / ".." / "analysis" / "physics" / "craft" / fs::path(rel).filename();
        ShipConstruct ship;
        std::string err;
        REQUIRE_MESSAGE(LoadCraftFile(file.string(), ship, err), err);
        CraftTree tree = BuildCraftTree(g.data, ship);
        REQUIRE(tree.ok);
        EditorSession editor(tree);
        EditorSession::StartParts(tree);
        editor.Load();
        crafts++;
        std::vector<int> S;
        auto add = [&](int v) {
            if (std::find(S.begin(), S.end(), v) == S.end()) S.push_back(v);
        };
        for (auto& p : tree.parts) {
            if (p->FindModuleOfClass("ModuleDecouple") == nullptr && p->FindModuleOfClass("ModuleAnchoredDecoupler") == nullptr) continue;
            std::vector<Part*> dec;
            IStageSeparatorChild* c = nullptr;
            for (size_t m = 0; m < p->Modules().Count(); m++)
                if (auto* sc = dynamic_cast<IStageSeparatorChild*>(p->Modules()[m])) c = sc;
            if (c != nullptr && !c->PartDetaches(dec)) add(p->fields.inverseStage);
            add(p->fields.separationIndex);
        }
        for (const auto& r : partsT->at("rows")) {
            if (r[cCraft].get<std::string>() != id) continue;
            if (r[cEng].is_null() || r[cEng].get<bool>() || r[cIsDec].get<bool>()) continue;  // clamps (null), engines, decouplers: DeltaVPartInfo does more
            const size_t i = r[cIdx].get<size_t>();
            Part& p = *tree.parts[i];
            const int act = p.fields.inverseStage;
            const int num = std::min(p.fields.separationIndex, act);
            int best = -1;
            for (int s : S)
                if (s > best && s <= num) best = s;
            const int dec = std::min(best, act);
            checked++;
            CHECK_MESSAGE(act == r[cAct].get<int>(), id << " part " << i << " activationStage");
            CHECK_MESSAGE(dec == r[cDec].get<int>(), id << " part " << i << " " << p.gameObjectName << " decoupleStage");
        }
    }
    CHECK(crafts == 17);
    CHECK(checked == 1001);
}

// ======================================================== FindStageIndices / SetSeparationIndices on a hand-built stack

namespace {
// R(tank) - D1(Decoupler.1) - T1(tank) - D2(Decoupler.1) - T2(tank) - E(liquidEngine2), a chain of parts from the game's prefabs
struct Chain {
    CraftTree tree;
    Part *R, *D1, *T1, *D2, *T2, *E;
    explicit Chain(Game15& g) {
        auto add = [&](const char* name) {
            tree.parts.push_back(g.New(name));
            return tree.parts.back().get();
        };
        R = add("fuelTank");
        D1 = add("Decoupler.1");
        T1 = add("fuelTank");
        D2 = add("Decoupler.1");
        T2 = add("fuelTank");
        E = add("liquidEngine2");
        Attach(*R, "bottom", *D1, "top");
        Attach(*D1, "bottom", *T1, "top");
        Attach(*T1, "bottom", *D2, "top");
        Attach(*D2, "bottom", *T2, "top");
        Attach(*T2, "bottom", *E, "top");
        tree.ok = true;
        uint32_t id = 100;
        for (auto& p : tree.parts) {
            p->fields.inverseStage = 0;          // the prefab value
            p->fields.persistentId = ++id;       // setSeparationIndices compares children by persistentId
        }
    }
};
}  // namespace

TEST_CASE("WP-16 StageManager.FindStageIndices / GenerateStagingSequence / RecalculateVesselStaging: offsets, manual stage, carry-over, separation") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    Chain c(g);
    for (auto& p : c.tree.parts) p->ModulesOnStart(StartState::Editor);  // stagingIcon defaults (Decoupler.1: DECOUPLER_VERT, engine: LIQUID_ENGINE)
    // Decoupler.1 / Separator cfgs: stageOffset = 1 for the decouplers, none for the engine (Decoupler_1.cfg:32)
    CHECK(c.D1->fields.stageOffset == 1);
    CHECK(c.E->fields.stageOffset == 0);
    CHECK_FALSE(c.R->hasStagingIcon());
    CHECK(c.D1->hasStagingIcon());
    CHECK(c.E->hasStagingIcon());
    // D2: stageBefore, stageAfter, stageOffset 2, childStageOffset 1
    c.D2->fields.stageOffset = 2;
    c.D2->fields.stageBefore = true;
    c.D2->fields.stageAfter = true;
    c.D2->fields.childStageOffset = 1;

    struct Counter {
        int n = 0;
        void On(Part*) { n++; }
    } changed;
    GameEvents::onPartPriorityChanged.Add(&changed, &Counter::On);
    // StageManager.cs:1408, thisStage starts at -1; derivation per part (num = "has an icon"; Decoupler_1.cfg: stageOffset = 1 AND
    // childStageOffset = 1; the engine has neither):
    //   R  no icon : thisStage = max(0, -1 + 0) = 0; default 0; inverseStage = carried = -1; children get max(0, 0 + 0) = 0
    //   D1 icon    : thisStage = max(0, 0 + stageOffset 1) = 1; default 1; inverse 1; carried 1; children max(0, 1 + childStageOffset 1) = 2
    //   T1 no icon : max(0, 2) = 2; default 2; inverse = carried 1
    //   D2 icon    : max(0, 2 + 2) = 4; stageBefore -> 5; default 5; inverse 5; carried 5; stageAfter -> 6; childStageOffset 1 -> 7
    //   T2 no icon : max(0, 7) = 7; default 7; inverse = carried 5
    //   E  icon    : max(0, 7 + 0) = 7; default 7; inverse = 7 (no manualStageOffset)
    StageManager::FindStageIndices(c.R, -1, -1);
    const int dflt[] = {0, 1, 2, 5, 7, 7};
    const int inv[] = {-1, 1, 1, 5, 5, 7};
    for (size_t i = 0; i < 6; i++) {
        CHECK_MESSAGE(c.tree.parts[i]->fields.defaultInverseStage == dflt[i], "default " << i);
        CHECK_MESSAGE(c.tree.parts[i]->fields.inverseStage == inv[i], "inverse " << i);
    }
    CHECK(changed.n == 6);  // every inverseStage changed from 0 (onPartPriorityChanged fires per change)
    changed.n = 0;
    StageManager::FindStageIndices(c.R, -1, -1);  // nothing changes the second time
    CHECK(changed.n == 0);
    // manualStageOffset on the engine: inverse = manual, default unchanged
    c.E->fields.manualStageOffset = 2;
    StageManager::FindStageIndices(c.R, -1, -1);
    CHECK(c.E->fields.inverseStage == 2);
    CHECK(c.E->fields.defaultInverseStage == 7);
    CHECK(changed.n == 1);
    c.E->fields.manualStageOffset = -1;
    // inverseStageCarryover = false on D2: T2 (no icon) now carries D1's stage
    c.D2->fields.inverseStageCarryover = false;
    StageManager::FindStageIndices(c.R, -1, -1);
    CHECK(c.D2->fields.inverseStage == 5);
    CHECK(c.T2->fields.inverseStage == 1);
    c.D2->fields.inverseStageCarryover = true;
    StageManager::FindStageIndices(c.R, -1, -1);
    CHECK(c.T2->fields.inverseStage == 5);
    GameEvents::onPartPriorityChanged.Remove(&changed, &Counter::On);

    // SetSeparationIndices (:1512): R -1; D1 = its own stage 1 (GetStageIndex), inherited by T1; D2 5, inherited by T2 and E
    StageManager::SetSeparationIndices(c.R, -1);
    const int sep[] = {-1, 1, 1, 5, 5, 5};
    for (size_t i = 0; i < 6; i++) CHECK_MESSAGE(c.tree.parts[i]->fields.separationIndex == sep[i], "sep " << i);
    // a decoupler whose partDecoupled is false (it detaches only its explosive node's part): its own separationIndex is the parent's,
    // and only the child on the explosive node gets the decoupler's stage
    auto* d2 = dynamic_cast<ModuleDecouple*>(c.D2->FindModuleOfClass("ModuleDecouple"));
    REQUIRE(d2 != nullptr);
    d2->Set<bool>("partDecoupled", false);
    d2->Set<std::string>("explosiveNodeID", "bottom");  // the node that holds T2 (a child)
    d2->SetExplosiveNode(nullptr);
    StageManager::SetSeparationIndices(c.R, -1);
    CHECK(c.D2->fields.separationIndex == 1);  // `part.separationIndex = sepIndex` (the parent's 1) when PartDetaches() is false
    CHECK(c.T2->fields.separationIndex == 5);  // the child named by decoupledChildParts: setSeparationIndices(child, num = D2's stage)
    CHECK(c.E->fields.separationIndex == 5);
    d2->Set<std::string>("explosiveNodeID", "top");  // now the node holds T1 (the parent), not a child
    d2->SetExplosiveNode(nullptr);
    StageManager::SetSeparationIndices(c.R, -1);
    CHECK(c.T2->fields.separationIndex == 1);  // not listed: the children get part.separationIndex
    CHECK(c.E->fields.separationIndex == 1);
    d2->Set<bool>("partDecoupled", true);
    d2->Set<std::string>("explosiveNodeID", "top");
    d2->SetExplosiveNode(nullptr);
    // GenerateStagingSequence = FindStageIndices(root, -1, -1) + SetSeparationIndices(root, -1)
    for (auto& p : c.tree.parts) p->fields.inverseStage = 0;
    StageManager::GenerateStagingSequence(c.R);
    for (size_t i = 0; i < 6; i++) {
        CHECK(c.tree.parts[i]->fields.inverseStage == inv[i]);
        CHECK(c.tree.parts[i]->fields.separationIndex == sep[i]);
    }
    // GetStageCount (:1038) = max inverseStage (>= 0); SetStageCount computes the same maximum and discards it
    std::vector<Part*> raw = c.tree.Raw();
    CHECK(StageManager::GetStageCount(raw) == 7);
    CHECK(StageManager::GetStageCount({c.R}) == 0);  // R is -1, the maximum starts at 0
    StageManager::SetStageCount(raw);
    // RecalculateVesselStaging (:1477): highest inverseStage of the tree (>= 0), and the separation indices are rebuilt
    Vessel v;
    v.rootPart = c.R;
    for (auto& p : c.tree.parts) p->fields.separationIndex = 99;
    CHECK(StageManager::RecalculateVesselStaging(&v) == 7);
    for (size_t i = 0; i < 6; i++) CHECK(c.tree.parts[i]->fields.separationIndex == sep[i]);
    v.rootPart = nullptr;
    v.parts.clear();
}

TEST_CASE("WP-16 StageManager.FindPartsWithModuleSeparatingBeforeOtherPartsWithModule") {
    if (!GameAvailable()) return;
    Game15 g;
    // parts sorted by separationIndex; walk the groups of equal index: engines (After) of a group are collected when the NEXT group starts,
    // and the walk stops at the group in which a decoupler (Before) is seen (StageManager.cs:1063)
    auto e0 = g.New("liquidEngine2"), e1 = g.New("liquidEngine2"), d = g.New("Decoupler.1"), e3 = g.New("liquidEngine2"), e4 = g.New("liquidEngine2");
    e0->fields.separationIndex = 0;
    e1->fields.separationIndex = 1;
    d->fields.separationIndex = 2;
    e3->fields.separationIndex = 2;
    e4->fields.separationIndex = 5;
    auto isDecouple = [](Part& p) { return p.FindModuleOfClass("ModuleDecouple") != nullptr; };
    auto isEngine = [](Part& p) { return p.FindModuleOfClass("ModuleEngines") != nullptr; };
    // sorted: e0(0) e1(1) d(2) e3(2) e4(5). num starts 0: e0 -> After, group {e0}; e1: new index -> flag2 so list=[e0], group {e1};
    // d: new index -> list=[e0,e1], Before seen (flag); e3: same index, flag set so not collected; e4: new index with flag -> break
    std::vector<Part*> r = StageManager::FindPartsWithModuleSeparatingBeforeOtherPartsWithModule({e4.get(), e3.get(), d.get(), e1.get(), e0.get()}, isDecouple, isEngine);
    REQUIRE(r.size() == 2);
    CHECK(r[0] == e0.get());
    CHECK(r[1] == e1.get());
    // no decoupler at all: the pending group at the very end is not appended (the loop ends without the flush)
    r = StageManager::FindPartsWithModuleSeparatingBeforeOtherPartsWithModule({e0.get(), e1.get(), e4.get()}, isDecouple, isEngine);
    REQUIRE(r.size() == 2);
    CHECK(r[0] == e0.get());
    CHECK(r[1] == e1.get());
}

// ======================================================== the SortIcons pipeline, stage groups, symmetry groups

namespace {
// R(tank, root) with the radial decouplers rad1 / rad2 (symmetry counterparts) and, below, Dm(Decoupler.1) - T1(tank) - E(engine)
struct Ship {
    CraftTree tree;
    Part *R, *rad1, *rad2, *Dm, *T1, *E;
    explicit Ship(Game15& g) {
        auto add = [&](const char* name) {
            tree.parts.push_back(g.New(name));
            return tree.parts.back().get();
        };
        R = add("fuelTank");
        rad1 = add("radialDecoupler1-2");
        rad2 = add("radialDecoupler1-2");
        Dm = add("Decoupler.1");
        T1 = add("fuelTank");
        E = add("liquidEngine2");
        rad1->setParent(R);
        rad2->setParent(R);
        rad1->symmetryCounterparts.push_back(rad2);
        rad2->symmetryCounterparts.push_back(rad1);
        Attach(*R, "bottom", *Dm, "top");
        Attach(*Dm, "bottom", *T1, "top");
        Attach(*T1, "bottom", *E, "top");
        tree.ok = true;
        uint32_t id = 200;
        for (auto& p : tree.parts) p->fields.persistentId = ++id;
        // as a loaded craft would have them (istg)
        for (auto& p : tree.parts) p->fields.inverseStage = 0;
        rad1->fields.inverseStage = 2;
        rad2->fields.inverseStage = 2;
        Dm->fields.inverseStage = 1;
        E->fields.inverseStage = 0;
    }
};

std::vector<int> InverseStages(const std::vector<StageIcon*>& icons) {
    std::vector<int> v;
    for (const StageIcon* i : icons) v.push_back(i->InverseStage());
    return v;
}
}  // namespace

TEST_CASE("WP-16 SortIcons: stage groups from the held icons, symmetry grouping, current stage, separation indices, events") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    Ship s(g);
    EditorSession ed(s.tree);
    StageManager& sm = *ed.sm;
    CHECK(StageManager::Instance == &sm);
    int sequenceModified = 0, separationEvents = 0;
    struct Counter {
        int* n;
        void On() { (*n)++; }
    } c1{&sequenceModified}, c2{&separationEvents};
    GameEvents::StageManager::OnGUIStageSequenceModified.Add(&c1, &Counter::On);
    GameEvents::StageManager::OnStagingSeparationIndices.Add(&c2, &Counter::On);
    EditorSession::StartParts(s.tree);
    // Part.Start created and HELD the icons of the four staging parts (CreateIcon -> HoldIcon, no stage group yet)
    CHECK(sm.IconsInHold().size() == 4);
    CHECK(sm.IconOf(s.R) == nullptr);
    CHECK(sm.IconOf(s.T1) == nullptr);
    REQUIRE(sm.IconOf(s.rad1) != nullptr);
    CHECK(StageManager::StageCount() == 0);
    CHECK(sm.Stages().empty());
    for (Part* p : {s.rad1, s.rad2, s.Dm, s.E}) CHECK(p->fields.stagingOn);

    // --- SortIcons(instant: false) is a 3-frame coroutine (StageManager.cs:1131 + :1150: `for (cfw = 2; cfw >= 0; cfw--) yield`)
    sm.OnEditorLoad();  // rebuildIndexes = true
    sm.SortIcons(false);
    CHECK(sm.SortPending());
    sm.Tick();
    sm.Tick();
    CHECK(sm.SortPending());
    sm.SortIcons(false);  // a request while waiting restarts the wait (coroutineFramesToWait = 2)
    sm.Tick();
    sm.Tick();
    CHECK(sm.SortPending());
    sm.SortIcons(true);   // instant while the coroutine runs: ignored (`sortRoutine == null` test)
    CHECK(StageManager::StageCount() == 0);
    sm.Tick();
    CHECK_FALSE(sm.SortPending());
    CHECK(sequenceModified == 1);  // rebuildIndexes -> OnGUIStageSequenceModified once
    CHECK_FALSE(sm.rebuildIndexesValue());
    CHECK(separationEvents >= 1);  // SetSeparationIndices fires OnStagingSeparationIndices

    // --- the groups. AddNewStageGroupsIfNeeded (:373): max InverseStage over the held icons = 2 -> 3 groups; every group is renumbered by
    // UpdateStageGroups (inverseStageIndex = index). The editor's current stage ends at count - 1 (:1160).
    REQUIRE(StageManager::StageCount() == 3);
    CHECK(StageManager::LastStage() == 2);
    CHECK(StageManager::CurrentStage() == 2);
    std::vector<StageGroup*> st = sm.Stages();
    for (int i = 0; i < 3; i++) CHECK(st[static_cast<size_t>(i)]->inverseStageIndex == i);
    CHECK(st[0]->defaultStage == 0);  // groups made by AddNewStageGroupsIfNeeded: defaultStage = their index
    // icons: AddHeldIconsToStages processes the hold list from its first element: rad1 + rad2 are symmetry counterparts with the same
    // InverseStage -> one group (list), lead = the first of the parent's children among them (rad1); Dm and E are singles (list2)
    REQUIRE(st[2]->Icons().size() == 1);
    StageIcon* lead = st[2]->Icons()[0];
    CHECK(lead == sm.IconOf(s.rad1));
    REQUIRE(lead->groupedIcons.size() == 1);
    CHECK(lead->groupedIcons[0] == sm.IconOf(s.rad2));
    CHECK(sm.IconOf(s.rad2)->groupLead == lead);
    CHECK(lead->groupLead == lead);
    CHECK(lead->grouped);
    CHECK_FALSE(lead->expanded);  // CollapseGroup
    CHECK_FALSE(sm.IconOf(s.rad2)->active);  // collapsed members are deactivated
    CHECK(s.rad1->fields.inverseStage == 2);
    CHECK(s.rad2->fields.inverseStage == 2);
    CHECK(s.rad1->fields.manualStageOffset == 2);  // SetInverseSequenceIndex(seqOverride: true) of AddIcon, AddToGroup (:1000)
    CHECK(s.rad2->fields.manualStageOffset == 2);
    CHECK(s.rad1->fields.inStageIndex == 0);
    CHECK(s.rad2->fields.inStageIndex == 0);  // grouped icons share the lead's index
    REQUIRE(st[1]->Icons().size() == 1);
    CHECK(st[1]->Icons()[0] == sm.IconOf(s.Dm));
    REQUIRE(st[0]->Icons().size() == 1);
    CHECK(st[0]->Icons()[0] == sm.IconOf(s.E));
    CHECK(sm.IconsInHold().empty());
    // inverse stages of the parts without icons: the carried stage of the nearest icon ancestor (-1 above the first)
    CHECK(s.R->fields.inverseStage == -1);
    CHECK(s.T1->fields.inverseStage == 1);  // below Dm (stage 1)
    // separation indices (setSeparationIndices): the anchored decouplers' own stage, Dm's stage inherited by T1 and the engine
    CHECK(s.R->fields.separationIndex == -1);
    CHECK(s.rad1->fields.separationIndex == 2);
    CHECK(s.rad2->fields.separationIndex == 2);
    CHECK(s.Dm->fields.separationIndex == 1);
    CHECK(s.T1->fields.separationIndex == 1);
    CHECK(s.E->fields.separationIndex == 1);
    // the stage-group listing reproduces the inverse stages
    CHECK(InverseStages(st[0]->Icons()) == std::vector<int>{0});
    CHECK(InverseStages(st[1]->Icons()) == std::vector<int>{1});

    // --- UpdateStageGroups / AddStageAt / DeleteStage (:949-1032): inserting an empty stage renumbers the groups above it and every
    // icon's Part.inverseStage / manualStageOffset follows (SetPartIndices with seqOverride)
    sm.AddStageAt(1);
    REQUIRE(StageManager::StageCount() == 4);
    st = sm.Stages();
    for (int i = 0; i < 4; i++) CHECK(st[static_cast<size_t>(i)]->inverseStageIndex == i);
    CHECK(st[1]->Icons().empty());
    CHECK(st[1]->defaultStage == -1);  // a manually added stage
    CHECK(s.E->fields.inverseStage == 0);
    CHECK(s.Dm->fields.inverseStage == 2);
    CHECK(s.rad1->fields.inverseStage == 3);
    CHECK(s.rad2->fields.inverseStage == 3);  // grouped icons follow the lead
    CHECK(s.Dm->fields.manualStageOffset == 2);
    CHECK(s.T1->fields.inverseStage == 2);    // plain parts follow: SetInverseSequenceIndex of a decoupler icon runs SetSeparationIndices (:784), which re-carries Dm's stage
    // DeleteEmptyStages (:1010): a manual (defaultStage == -1) stage survives, a normal empty one does not
    sm.DeleteEmptyStages();
    CHECK(StageManager::StageCount() == 4);
    sm.DeleteEmptyStages(true);  // deleteManualStages
    CHECK(StageManager::StageCount() == 3);
    CHECK(s.Dm->fields.inverseStage == 1);
    CHECK(s.rad1->fields.inverseStage == 2);
    // IncrementCurrentStage / DecrementCurrentStage: the editor's current stage follows added / removed stages
    const int cur = StageManager::CurrentStage();
    sm.IncrementCurrentStage();
    CHECK(StageManager::CurrentStage() == cur + 1);
    sm.DecrementCurrentStage();
    sm.DecrementCurrentStage();
    CHECK(StageManager::CurrentStage() == cur - 1);

    // --- OnPartUpdateStageability (:294): the clamp of a negative inverseStage, rebuildIndexes, an instant sort
    s.E->fields.inverseStage = -1;
    sm.OnPartUpdateStageability(s.E);
    CHECK(s.E->fields.inverseStage >= 0);
    CHECK_FALSE(sm.rebuildIndexesValue());  // the instant SortIcons ran and consumed it
    GameEvents::StageManager::OnGUIStageSequenceModified.Remove(&c1, &Counter::On);
    GameEvents::StageManager::OnStagingSeparationIndices.Remove(&c2, &Counter::On);
}

TEST_CASE("WP-16 StageGroup.MoveAllIconsInto / MoveIconsInto, Reset, RemoveIcon, DisableIcon, symmetry markers") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    Ship s(g);
    EditorSession ed(s.tree);
    StageManager& sm = *ed.sm;
    EditorSession::StartParts(s.tree);
    ed.Load();
    std::vector<StageGroup*> st = sm.Stages();
    REQUIRE(st.size() == 3);
    // StageGroup.cs:817 MoveAllIconsInto(group 2 -> group 0): the symmetry group moves as one (MoveIconsInto(lead, its group)); both
    // decouplers take the target stage as inverseStage and manualStageOffset; the lead is inserted at index 0
    StageIcon* lead = st[2]->Icons()[0];
    st[2]->MoveAllIconsInto(st[0]);
    CHECK(st[2]->Icons().empty());
    REQUIRE(st[0]->Icons().size() == 2);
    CHECK(st[0]->Icons()[0] == lead);
    CHECK(st[0]->Icons()[1] == sm.IconOf(s.E));
    CHECK(lead->groupedIcons.size() == 1);
    CHECK(sm.IconOf(s.rad2)->groupLead == lead);
    CHECK(s.rad1->fields.inverseStage == 0);
    CHECK(s.rad2->fields.inverseStage == 0);
    CHECK(s.rad1->fields.manualStageOffset == 0);
    CHECK(s.rad2->fields.manualStageOffset == 0);
    CHECK(s.rad1->fields.inStageIndex == 0);
    CHECK(s.E->fields.inStageIndex == 0);  // AddIconAt does not re-index the icons already in the group (:570) ...
    sm.UpdateStageGroups();                 // ... UpdateStageGroup -> SetPartIndices does (:657)
    CHECK(s.E->fields.inStageIndex == 1);
    CHECK(s.rad1->fields.inStageIndex == 0);
    // ResetAvailable (:705): an icon with manualStageOffset != -1 whose inverseStage differs from its defaultInverseStage
    s.rad1->fields.defaultInverseStage = 2;
    CHECK(lead->ResetAvailable());
    CHECK(st[0]->ResetAvailable());
    // Reset (the editor's reset button, :1579): icons back on hold with inverseStage = defaultInverseStage and manualStageOffset = default, stages cleared
    s.rad1->fields.defaultInverseStage = 2;
    s.rad2->fields.defaultInverseStage = 2;
    s.Dm->fields.defaultInverseStage = 1;
    s.E->fields.defaultInverseStage = 0;
    sm.Reset();
    CHECK(StageManager::StageCount() == 0);
    CHECK(StageManager::CurrentStage() == 0);
    CHECK(s.rad1->fields.inverseStage == 2);
    CHECK(s.rad2->fields.inverseStage == 2);
    CHECK(s.Dm->fields.inverseStage == 1);
    CHECK(s.rad1->fields.manualStageOffset == 2);
    CHECK(sm.SortPending());
    sm.FlushSort();
    REQUIRE(StageManager::StageCount() == 3);
    st = sm.Stages();
    CHECK(st[2]->Icons().size() == 1);  // the group is re-formed
    CHECK(st[2]->Icons()[0]->groupedIcons.size() == 1);
    // RemoveIcon of a symmetry lead in the editor (:881): its group goes back on hold and the icon leaves its stage
    StageIcon* l2 = st[2]->Icons()[0];
    Part* ownerPart = l2->part;
    ownerPart->RemoveStageIcon(false);
    CHECK(sm.IconOf(ownerPart) == nullptr);
    CHECK_FALSE(ownerPart->HasStageIcon());
    CHECK(sm.IconsInHold().size() == 1);  // the former group member
    // Selection (:162) is empty here
    CHECK(StageManager::Selection().empty());
}

TEST_CASE("WP-16 editor placement: preferredStage FIRST / LAST / PARACHUTESTAGE and the clamp of a held icon's stage") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    // AvailablePart.preferredStage (PartLoader): launchClamp1 LAST, the parachutes PARACHUTESTAGE, everything else DEFAULT
    CHECK(g.Info("launchClamp1").preferredStage == static_cast<int>(PreferredInitialStage::LAST));
    CHECK(g.Info("parachuteSingle").preferredStage == static_cast<int>(PreferredInitialStage::PARACHUTESTAGE));
    CHECK(g.Info("liquidEngine2").preferredStage == static_cast<int>(PreferredInitialStage::DEFAULT));
    Ship s(g);
    EditorSession ed(s.tree);
    StageManager& sm = *ed.sm;
    EditorSession::StartParts(s.tree);
    ed.Load();
    REQUIRE(StageManager::StageCount() == 3);
    // a launch clamp placed on the ship: preferredStage LAST -> the last stage group (index 2) whatever its own stage is
    auto clamp = g.New("launchClamp1");
    clamp->fields.persistentId = 900;
    clamp->setParent(s.T1);
    clamp->ModulesOnStart(StartState::Editor);
    clamp->fields.inverseStage = 0;
    clamp->StartStaging();  // the icon is created and held
    REQUIRE(sm.IconOf(clamp.get()) != nullptr);
    CHECK_FALSE(clamp->fields.editorFirstTimePlaced);
    sm.OnEditorPartPlaced(clamp.get());  // SortIcons(instant: true, part, false, false)
    std::vector<StageGroup*> st = sm.Stages();
    REQUIRE(st.size() == 3);
    CHECK(st[2]->Icons().back() == sm.IconOf(clamp.get()));
    CHECK(clamp->fields.inverseStage == 2);
    CHECK(clamp->fields.editorFirstTimePlaced);  // set once placed
    // a parachute (PARACHUTESTAGE): stage 0 holds only the engine (no parachute, no decoupler) so GetParachuteStageIndex(0) opens a new
    // top stage, shifts every icon up one stage and places the parachute in stage 0 (:637)
    auto chute = g.New("parachuteSingle");
    chute->fields.persistentId = 901;
    chute->setParent(s.T1);
    chute->ModulesOnStart(StartState::Editor);
    chute->fields.inverseStage = 0;
    chute->StartStaging();
    REQUIRE(sm.IconOf(chute.get()) != nullptr);
    const int enginesBefore = s.E->fields.inverseStage;
    sm.OnEditorPartPlaced(chute.get());
    CHECK(StageManager::StageCount() == 4);
    CHECK(chute->fields.inverseStage == 0);
    CHECK(s.E->fields.inverseStage == enginesBefore + 1);
    CHECK(s.Dm->fields.inverseStage == 2);
    CHECK(clamp->fields.editorFirstTimePlaced);
    // the held icon of a part is released with the part: ProtoStageIcon.RemoveIcon
    chute->RemoveStageIcon(false);
    clamp->RemoveStageIcon(false);
    CHECK(sm.IconOf(chute.get()) == nullptr);
}

// ======================================================== flight: stage activation, decouple, couple, undock

namespace {
// Records the GameEvents the staging and tree operations fire
struct EventTrace {
    std::vector<std::string> log;
    int stageSeparationStage = -99;
    std::vector<int> separationStages;
    EventTrace() {
        GameEvents::onStageActivate.Add(this, &EventTrace::Activate);
        GameEvents::onStageSeparation.Add(this, &EventTrace::Separation);
        GameEvents::onLaunch.Add(this, &EventTrace::Launch);
        GameEvents::onPartDeCouple.Add(this, &EventTrace::DeCouple);
        GameEvents::onPartDeCoupleComplete.Add(this, &EventTrace::DeCoupleComplete);
        GameEvents::onPartDeCoupleNewVesselComplete.Add(this, &EventTrace::NewVessel);
        GameEvents::onPartCouple.Add(this, &EventTrace::Couple);
        GameEvents::onPartCoupleComplete.Add(this, &EventTrace::CoupleComplete);
        GameEvents::onPartUndock.Add(this, &EventTrace::Undock);
        GameEvents::onPartUndockComplete.Add(this, &EventTrace::UndockComplete);
        GameEvents::onVesselsUndocking.Add(this, &EventTrace::Undocking);
        GameEvents::onVesselPersistentIdChanged.Add(this, &EventTrace::PidChanged);
    }
    ~EventTrace() {
        GameEvents::onStageActivate.Remove(this, &EventTrace::Activate);
        GameEvents::onStageSeparation.Remove(this, &EventTrace::Separation);
        GameEvents::onLaunch.Remove(this, &EventTrace::Launch);
        GameEvents::onPartDeCouple.Remove(this, &EventTrace::DeCouple);
        GameEvents::onPartDeCoupleComplete.Remove(this, &EventTrace::DeCoupleComplete);
        GameEvents::onPartDeCoupleNewVesselComplete.Remove(this, &EventTrace::NewVessel);
        GameEvents::onPartCouple.Remove(this, &EventTrace::Couple);
        GameEvents::onPartCoupleComplete.Remove(this, &EventTrace::CoupleComplete);
        GameEvents::onPartUndock.Remove(this, &EventTrace::Undock);
        GameEvents::onPartUndockComplete.Remove(this, &EventTrace::UndockComplete);
        GameEvents::onVesselsUndocking.Remove(this, &EventTrace::Undocking);
        GameEvents::onVesselPersistentIdChanged.Remove(this, &EventTrace::PidChanged);
    }
    void Activate(int stage) { log.push_back("activate:" + std::to_string(stage)); }
    void Separation(EventReport* r) {
        log.push_back("separation:" + std::to_string(r->stage));
        separationStages.push_back(r->stage);
    }
    void Launch(EventReport*) { log.push_back("launch"); }
    void DeCouple(Part* p) { log.push_back("decouple:" + p->gameObjectName); }
    void DeCoupleComplete(Part* p) { log.push_back("decoupleComplete:" + p->gameObjectName); }
    void NewVessel(Vessel*, Vessel*) { log.push_back("newVessel"); }
    void Couple(GameEvents::FromToAction<Part*, Part*> d) { log.push_back("couple:" + d.from->gameObjectName + ">" + d.to->gameObjectName); }
    void CoupleComplete(GameEvents::FromToAction<Part*, Part*>) { log.push_back("coupleComplete"); }
    void Undock(Part* p) { log.push_back("undock:" + p->gameObjectName); }
    void UndockComplete(Part* p) { log.push_back("undockComplete:" + p->gameObjectName); }
    void Undocking(Vessel*, Vessel*) { log.push_back("undocking"); }
    void PidChanged(uint32_t, uint32_t) { log.push_back("pidChanged"); }
    int Count(const std::string& prefix) const {
        int n = 0;
        for (const auto& l : log) n += l.rfind(prefix, 0) == 0;
        return n;
    }
};

// A VesselResourceSets per Vessel, wired like Vessel.cs:3913 (Vessel.env.updateResourceSets) so the crossfeed sets are rebuilt
struct ResourceSetsHook : IResourceVessel {
    std::map<Vessel*, VesselResourceSets> sets;
    std::map<Vessel*, std::unique_ptr<IResourceVessel>> hosts;
    struct Host : IResourceVessel {
        Vessel* v;
        VesselResourceSets* s;
        Host(Vessel* vv, VesselResourceSets* ss) : v(vv), s(ss) {}
        const std::vector<Part*>& ResourceParts() const override { return v->parts; }
        double RequestResource(Part* part, int id, double demand, bool usePriority, bool simulate) override { return s->RequestResource(part, id, demand, usePriority, simulate); }
        void GetConnectedResourceTotals(int id, bool simulate, double& a, double& m, bool pulling) override { s->GetConnectedResourceTotals(id, simulate, a, m, pulling); }
        void GetConnectedResourceTotals(int id, bool simulate, double& a, double& m, double th, bool pulling) override { s->GetConnectedResourceTotals(id, simulate, a, m, th, pulling); }
    };
    ResourceSetsHook() {
        ResourceFlowEnvironment::vesselHost = [this](Vessel* v) -> IResourceVessel* {
            auto it = hosts.find(v);
            if (it == hosts.end()) it = hosts.emplace(v, std::make_unique<Host>(v, &sets[v])).first;
            return it->second.get();
        };
        Vessel::env.updateResourceSets = [this](Vessel& v) { sets[&v].UpdateResourceSets(&v, v.parts); };
    }
    ~ResourceSetsHook() {
        ResourceFlowEnvironment::vesselHost = nullptr;
        Vessel::env.updateResourceSets = nullptr;
    }
    const std::vector<Part*>& ResourceParts() const override { static std::vector<Part*> none; return none; }
    double RequestResource(Part*, int, double, bool, bool) override { return 0.0; }
    void GetConnectedResourceTotals(int, bool, double&, double&, bool) override {}
    void GetConnectedResourceTotals(int, bool, double&, double&, double, bool) override {}
};

size_t SubtreeSize(const Part* p) {
    size_t n = 1;
    for (const Part* c : p->children) n += SubtreeSize(c);
    return n;
}

std::set<Part*> CrossfeedParts(Part* p) {
    std::set<Part*> out;
    REQUIRE(p->crossfeedPartSet != nullptr);
    auto hs = p->crossfeedPartSet->GetParts();
    for (Part* q : *hs) out.insert(q);
    return out;
}
}  // namespace

TEST_CASE("WP-16 flight staging of PhysProf-S: BeginFlight, ActivateNextStage, the anchored decouplers split the vessel into three") {
    if (!GameAvailable()) return;
    World16 w;
    LogCapture quiet;
    ResourceSetsHook rs;
    EventTrace ev;
    auto v = w.PhysProf("PhysProf-S");
    REQUIRE(v->parts.size() == 21);
    // the tree of PhysProf-S (capture C5/C10 `structure`, parent_index): the radial decouplers are parts 3 and 12 (children of the
    // Rockomax tank 1); each carries a booster stack of 9 parts incl. itself
    Part* R = v->rootPart;
    Part* tank = v->parts[1];
    Part* dec3 = v->parts[3];
    Part* dec12 = v->parts[12];
    REQUIRE(dec3->parent == tank);
    REQUIRE(dec12->parent == tank);
    CHECK(SubtreeSize(dec3) == 9);
    CHECK(SubtreeSize(dec12) == 9);
    Part* p4 = v->parts[4];   // the tank below the first decoupler; its children are the engine (5) and the chain 6 - 7 - 8 - 9 - 10 - 11
    Part* p5 = v->parts[5];
    Part* p6 = v->parts[6];
    Part* p11 = v->parts[11];
    REQUIRE(p4->children.size() == 2);
    REQUIRE(p4->children[0] == p5);
    REQUIRE(p4->children[1] == p6);
    REQUIRE(p11->parent == v->parts[10]);
    FlightRegistry::ActiveVessel = v.get();
    v->updateResourcesOnEvent = true;
    v->UpdateResourceSets();
    for (size_t i = 0; i < v->parts.size(); i++) v->parts[i]->fields.flightID = static_cast<uint32_t>(1000 + i);

    FlightStagingHost host;
    StageManager sm(&host);
    REQUIRE(sm.Awake());
    for (Part* p : v->parts) p->ModulesOnStart(StartState::Flying);  // explosive nodes, stagingIcon defaults
    v->ResumeStaging();                                               // Vessel.cs:2140: UpdateStageability(false, false) + CreateIcon
    CHECK(sm.IconsInHold().size() == 5);                              // 3 engines + 2 decouplers (parts 2, 3, 5, 12, 14)
    StageManager::BeginFlight();                                      // StageManager.cs:1209
    CHECK(StageManager::CanSeparate());
    sm.FlushSort();
    // BeginFlight: currentStage = highest inverseStage + 1 = 2; the sort made two stage groups
    CHECK(StageManager::StageCount() == 2);
    CHECK(StageManager::CurrentStage() == 2);
    CHECK(v->currentStage == 2);  // SetCurrentStage writes the active vessel's currentStage
    std::vector<StageGroup*> st = sm.Stages();
    REQUIRE(st.size() == 2);
    // flight AddHeldIconsToStages: stages[min(InverseStage, currentStage)]; hold order = part order
    CHECK(st[1]->Icons().size() == 3);  // mainsail + two LV-909 style engines
    CHECK(st[0]->Icons().size() == 2);  // the decouplers
    CHECK(st[0]->Icons()[0] == sm.IconOf(dec3));
    CHECK(st[0]->Icons()[1] == sm.IconOf(dec12));
    // the carried stages of the booster tanks (parts 4, 6-11, 13, 15-20) = the decoupler's 0; root, Rockomax tank: -1
    CHECK(R->fields.inverseStage == -1);
    CHECK(tank->fields.inverseStage == -1);
    CHECK(v->parts[4]->fields.inverseStage == 0);
    CHECK(v->parts[20]->fields.inverseStage == 0);
    CHECK(v->parts[2]->fields.inverseStage == 1);
    for (size_t i : {2u, 5u, 14u}) CHECK(v->parts[i]->State() == PartStates::IDLE);

    // --- stage 1: the engines. StageManager.cs:1244 ActivateStage
    ev.log.clear();
    StageManager::ActivateNextStage();
    REQUIRE(ev.log.size() >= 2);
    CHECK(ev.log[0] == "activate:1");
    CHECK(ev.log[1] == "launch");  // FlightLogger.LiftOff is false: onLaunch
    for (size_t i : {2u, 5u, 14u}) CHECK(v->parts[i]->State() == PartStates::ACTIVE);
    for (size_t i : {3u, 12u}) CHECK(v->parts[i]->State() == PartStates::IDLE);
    CHECK(StageManager::CurrentStage() == 1);
    CHECK(v->currentStage == 1);
    CHECK_FALSE(StageManager::CanSeparate());  // cooldownSeparation
    CHECK(v->parts.size() == 21);              // nothing separated yet
    // a second request during the cooldown is refused without a trace
    ev.log.clear();
    StageManager::ActivateNextStage();
    CHECK(ev.log.empty());
    CHECK(StageManager::CurrentStage() == 1);
    sm.Tick(PhysicsGlobals::StagingCooldownTimer() * 0.5);
    CHECK_FALSE(StageManager::CanSeparate());
    sm.Tick(PhysicsGlobals::StagingCooldownTimer());
    CHECK(StageManager::CanSeparate());

    // --- stage 0: both radial decouplers fire. force_activate runs on the parts with inverseStage == 0 in reverse part order, so the
    // decoupler of part 12 goes first. ModuleAnchoredDecoupler.OnActive (staged && stagingEnabled) -> OnDecouple -> part.decouple()
    const std::string idOld = v->id.ToString();
    ev.log.clear();
    StageManager::ActivateNextStage();
    CHECK(ev.log[0] == "activate:0");
    CHECK(ev.Count("decouple:") == 2);
    CHECK(ev.Count("decoupleComplete:") == 2);
    CHECK(ev.Count("newVessel") == 2);
    CHECK(ev.Count("separation:") == 2);
    for (int s : ev.separationStages) CHECK(s == 1);  // EventReport.stage = StageManager.CurrentStage at the time (SetCurrentStage(0) comes after)
    // onLaunch (FlightLogger.LiftOff is still false) fires after the activations (:1280), so after the decouple events
    REQUIRE(ev.Count("launch") == 1);
    CHECK(ev.log.back() == "launch");
    CHECK(StageManager::CurrentStage() == 0);
    CHECK(v->currentStage == 0);
    // three vessels: the original keeps the root, the Rockomax tank and the mainsail; each booster is a new vessel rooted at its decoupler
    REQUIRE(Vessel::SpawnedVessels().size() == 2);
    Vessel* b12 = dec12->vessel;
    Vessel* b3 = dec3->vessel;
    REQUIRE(b12 != nullptr);
    REQUIRE(b3 != nullptr);
    CHECK(b12 != v.get());
    CHECK(b3 != v.get());
    CHECK(b3 != b12);
    CHECK(b12->rootPart == dec12);
    CHECK(b3->rootPart == dec3);
    CHECK(b12->parts.size() == 9);
    CHECK(b3->parts.size() == 9);
    CHECK(v->parts.size() == 3);  // root pod, Rockomax tank, mainsail
    CHECK(v->parts[0] == R);
    CHECK(v->parts[1] == tank);
    CHECK(v->parts[2] == v->rootPart->children[0]->children[0]);
    CHECK(tank->children.size() == 1);  // only the mainsail is left below the tank
    CHECK(dec3->parent == nullptr);
    CHECK(dec12->parent == nullptr);
    for (Part* p : b3->parts) CHECK(p->vessel == b3);
    for (Part* p : b12->parts) CHECK(p->vessel == b12);
    for (Part* p : v->parts) CHECK(p->vessel == v.get());
    // Vessel.Initialize order: findVesselParts DFS from the root, the decoupler first
    CHECK(b3->parts[0] == dec3);
    CHECK(b3->parts[1]->parent == dec3);
    // ownership followed the parts
    CHECK(v->OwnedPartCount() == 3);
    CHECK(b3->OwnedPartCount() == 9);
    CHECK(b12->OwnedPartCount() == 9);
    // currentStage of a new vessel = RecalculateVesselStaging = highest inverseStage of its parts (>= 0): the booster engine's stage 1
    CHECK(b3->currentStage == 1);
    CHECK(b12->currentStage == 1);
    // inverse stages renumbering: nothing is renumbered by a decouple, the parts keep their stage
    CHECK(dec3->fields.inverseStage == 0);
    CHECK(b3->parts[2]->fields.inverseStage == 1);  // the engine (part 5)
    // the old vessel's decoupled parts: vessel name / type of the new ones (UpdateVesselNaming or AutoRename)
    CHECK_FALSE(b3->vesselName.empty());
    CHECK(b3->launchedFrom == v->launchedFrom);
    CHECK(b3->landedAt == v->landedAt);
    CHECK(b3->id.ToString() != idOld);
    CHECK(b3->id.ToString() != b12->id.ToString());
    CHECK(FlightRegistry::FindVessel(b3->id) == b3);
    // Decoupling: the explosive node (srf) of the decoupler no longer holds the tank, nor does the tank know it
    CHECK(dec3->srfAttachNode->attachedPart == nullptr);
    // the decoupler modules recorded the state: isDecoupled persists, the Decouple event is hidden, staging flipped off in LateUpdate
    auto* m3 = dynamic_cast<ModuleAnchoredDecoupler*>(dec3->FindModuleOfClass("ModuleAnchoredDecoupler"));
    REQUIRE(m3 != nullptr);
    CHECK(m3->Get<bool>("isDecoupled"));
    CHECK_FALSE(m3->Events()["Decouple"]->active);
    CHECK(m3->Get<bool>("stagingEnabled"));  // until LateUpdate
    ModuleDecouplerBase::TickAll();           // LateUpdate: refreshStaging -> stagingEnabled = false; UpdateStageability
    CHECK_FALSE(m3->Get<bool>("stagingEnabled"));
    CHECK_FALSE((m3->IsStageable() && m3->StagingEnabled()));
    CHECK_FALSE(dec3->fields.stagingOn);
    // ejection forces are applied two frames after the decouple (the coroutine): addForce(part, force)
    std::vector<std::pair<Part*, Vector3f>> forces;
    ModuleDecouplerBase::addForce = [&](Part& p, const Vector3f& f) { forces.emplace_back(&p, f); };
    ModuleDecouplerBase::addForceAtPosition = [&](Part& p, const Vector3f& f, const Vector3f&) { forces.emplace_back(&p, f); };
    ModuleDecouplerBase::TickAll();
    CHECK(forces.size() == 4);  // 2 decouplers x (-force on the decoupler, +force on the other part)
    ModuleDecouplerBase::addForce = nullptr;
    ModuleDecouplerBase::addForceAtPosition = nullptr;
    // the crossfeed sets were rebuilt for every vessel by onVesselPartCountChanged / Initialize (Vessel.cs:3892, 1766)
    {
        std::set<Part*> oldParts(v->parts.begin(), v->parts.end());
        for (Part* p : v->parts)
            if (p->fields.fuelCrossFeed) {
                std::set<Part*> cf = CrossfeedParts(p);
                for (Part* q : cf) CHECK_MESSAGE(oldParts.count(q) == 1, "old vessel set contains a part of another vessel");
            }
        for (Vessel* nb : {b3, b12}) {
            std::set<Part*> np(nb->parts.begin(), nb->parts.end());
            for (Part* p : nb->parts) {
                if (!p->fields.fuelCrossFeed || p->crossfeedPartSet == nullptr) continue;
                std::set<Part*> cf = CrossfeedParts(p);
                for (Part* q : cf) CHECK_MESSAGE(np.count(q) == 1, "booster set contains a foreign part");
            }
        }
        // the Rockomax tank no longer shares a set with the boosters: its set holds at most root + tank + mainsail
        CHECK(CrossfeedParts(tank).size() <= 3);
    }
    // the stage manager follows: SortIcons (instant) after the activation; the icons of the decoupled parts left with them
    CHECK(StageManager::StageCount() >= 1);

    // --- Couple (Part.cs:3171): the booster re-joins the tank. dec3 becomes a child of `tank` again, its vessel is destroyed
    ev.log.clear();
    const size_t spawnedBefore = Vessel::SpawnedVessels().size();
    Vessel* b3Before = b3;
    (void)b3Before;
    dec3->Couple(tank);
    CHECK(ev.log.front() == "couple:radialDecoupler1-2>Rockomax32.BW");
    CHECK(ev.Count("coupleComplete") == 1);
    CHECK(ev.Count("pidChanged") == 1);
    CHECK(Vessel::SpawnedVessels().size() == spawnedBefore - 1);  // DestroyImmediate(vessel)
    CHECK(dec3->parent == tank);
    CHECK(tank->children.size() == 2);
    CHECK(v->parts.size() == 3 + 9);
    for (Part* p : v->parts) CHECK(p->vessel == v.get());
    CHECK(v->OwnedPartCount() == 12);
    // Vessel.currentStage = RecalculateVesselStaging + 1 (:3217): highest inverseStage 1 + 1
    CHECK(v->currentStage == 2);
    for (Part* p : v->parts) CHECK(p->isAttached());
    CHECK(FlightRegistry::FindVessel(v->id) == v.get());

    // --- Undock (Part.cs:3274) without a DockedVesselInfo: the part and its subtree leave as a new vessel, named by AutoRename
    ev.log.clear();
    Part* sub = v->parts[1]->children.back();  // whichever child of the tank is last (dec3 after the couple)
    REQUIRE(sub == dec3);
    Part* tankBelow = dec3->children[0];
    const size_t sizeSub = SubtreeSize(tankBelow);
    Vessel* nv = tankBelow->Undock(nullptr);
    CHECK(ev.log[0] == "undock:" + tankBelow->gameObjectName);
    CHECK(ev.Count("undocking") == 1);
    CHECK(ev.Count("undockComplete") == 1);
    REQUIRE(nv != nullptr);
    CHECK(nv->rootPart == tankBelow);
    CHECK(nv->parts.size() == sizeSub);
    CHECK(tankBelow->parent == nullptr);
    CHECK(dec3->children.empty());
    CHECK(v->parts.size() == 12 - sizeSub);
    CHECK(nv->currentStage == StageManager::RecalculateVesselStaging(nv) + 1);
    CHECK(nv->launchedFrom == v->launchedFrom);
    // the old vessel resumes staging one frame later (CallbackUtil.DelayedCallback(1, ResumeStaging))
    REQUIRE(v->delayedFrameCalls.size() == 1);
    int resumed = 0;
    struct R { int* n; void On(Vessel*) { (*n)++; } } rr{&resumed};
    GameEvents::onVesselResumeStaging.Add(&rr, &R::On);
    v->RunDelayedFrameCalls();
    CHECK(resumed >= 1);
    GameEvents::onVesselResumeStaging.Remove(&rr, &R::On);
    CHECK(v->delayedFrameCalls.empty());
    // a DockedVesselInfo naming a part below the undocked one: that part becomes the root of the new vessel (SetHierarchyRoot reverses
    // the chain 6 - 7 - 8 - 9 - 10 - 11) and the vessel takes the info's name and type (Part.cs:3283-3319)
    DockedVesselInfo info;
    info.name = "Booster Probe";
    info.vesselType = VesselType::Probe;
    info.rootPartUId = p11->fields.flightID;
    REQUIRE(nv->parts.size() == 8);
    REQUIRE(p6->hasIndirectChild(p11));
    Vessel* nv2 = p6->Undock(&info);
    CHECK(nv2->rootPart == p11);
    CHECK(nv2->parts.size() == 6);
    CHECK(nv2->parts[0] == p11);
    CHECK(p11->parent == nullptr);
    CHECK(p6->parent != nullptr);          // 6 hangs below 7 now
    CHECK(p6->children.empty());           // the old top of the chain is now its leaf
    CHECK(nv2->vesselName == "Booster Probe");
    CHECK(nv2->vesselType == VesselType::Probe);
    CHECK(nv->parts.size() == 2);          // parts 4 and 5 stay behind
    CHECK(p4->children.size() == 1);       // the chain left (setParent())
    CHECK(nv2->currentStage == StageManager::RecalculateVesselStaging(nv2) + 1);
    // Vessel::ClearStaging in Couple / Undock removed the stack icons of the vessels involved
    CHECK(sm.IconOf(p5) == nullptr);                // nv was never resumed
    CHECK(sm.IconOf(v->parts[2]) != nullptr);       // v was: RunDelayedFrameCalls -> ResumeStaging recreated the mainsail's icon
    Vessel::SpawnedVessels().clear();
    FlightRegistry::ActiveVessel = nullptr;
}

TEST_CASE("WP-16 Part.decouple: breakForce, omni decoupler, decoupler facing away, DockedVesselInfo") {
    if (!GameAvailable()) return;
    World16 w;
    LogCapture quiet;
    // DockedVesselInfo.cs:31 Load / :96 Save, including the pre-1.2 numeric VesselType values (OldVesselType)
    {
        DockedVesselInfo d;
        ConfigNode n("DOCKEDVESSEL");
        n.AddValue("vesselName", "Station One");
        n.AddValue("rootUId", "123456");
        n.AddValue("vesselType", "Station");
        d.Load(n);
        CHECK(d.name == "Station One");
        CHECK(d.rootPartUId == 123456u);
        CHECK(d.vesselType == VesselType::Station);
        ConfigNode out("DOCKEDVESSEL");
        d.Save(out);
        CHECK(*out.GetValue("vesselName") == "Station One");
        CHECK(*out.GetValue("vesselType") == "Station");
        CHECK(*out.GetValue("rootUId") == "123456");
        // OldVesselType numbers: Debris 0, SpaceObject 1, Unknown 2, Probe 3, Rover 4, Lander 5, Ship 6, Station 7, Base 8, EVA 9, Flag 10,
        // Plane 11, Relay 12, DeployedScienceController 13, DeployedSciencePart 14; anything else -> Ship
        const std::pair<int, VesselType> old[] = {{0, VesselType::Debris}, {1, VesselType::SpaceObject}, {2, VesselType::Unknown}, {3, VesselType::Probe},
                                                   {4, VesselType::Rover}, {5, VesselType::Lander}, {6, VesselType::Ship}, {7, VesselType::Station},
                                                   {8, VesselType::Base}, {9, VesselType::EVA}, {10, VesselType::Flag}, {11, VesselType::Plane},
                                                   {12, VesselType::Relay}, {13, VesselType::DeployedScienceController}, {14, VesselType::DeployedSciencePart},
                                                   {15, VesselType::Ship}, {99, VesselType::Ship}};
        for (const auto& [num, vt] : old) {
            DockedVesselInfo o;
            ConfigNode m("DOCKEDVESSEL");
            m.AddValue("vesselType", std::to_string(num));
            o.Load(m);
            CHECK_MESSAGE(o.vesselType == vt, "old type " << num);
        }
        DockedVesselInfo empty;  // missing values keep the defaults
        ConfigNode none("DOCKEDVESSEL");
        empty.Load(none);
        CHECK(empty.name.empty());
        CHECK(empty.rootPartUId == 0u);
    }
    // Decouple of a vessel part with breakForce > 0 (a broken joint): disconnect(false) keeps the stack icons (:3084)
    auto v = w.PhysProf("PhysProf-S");
    Part* dec3 = v->parts[3];
    FlightRegistry::ActiveVessel = v.get();
    FlightStagingHost host;
    StageManager sm(&host);
    REQUIRE(sm.Awake());
    for (Part* p : v->parts) p->ModulesOnStart(StartState::Flying);
    v->ResumeStaging();
    sm.FlushSort();
    REQUIRE(sm.IconOf(dec3) != nullptr);
    Vessel* nb = dec3->decouple(5.0f);
    CHECK(nb->rootPart == dec3);
    CHECK(sm.IconOf(dec3) != nullptr);        // controlledSeparation == (breakForce == 0): false, icon stays
    Part* dec12 = v->parts[v->parts.size() - 9];  // not used further
    (void)dec12;
    // Part.decouple() with breakForce == 0: stackIcon.RemoveIcon for the part and its subtree
    Part* d12 = nullptr;
    for (Part* p : v->parts)
        if (p->gameObjectName == "radialDecoupler1-2") d12 = p;
    REQUIRE(d12 != nullptr);
    REQUIRE(sm.IconOf(d12) != nullptr);
    Vessel* nb2 = d12->decouple();
    CHECK(nb2->rootPart == d12);
    CHECK(sm.IconOf(d12) == nullptr);
    Vessel::SpawnedVessels().clear();
    FlightRegistry::ActiveVessel = nullptr;
}

TEST_CASE("WP-16 Kerbal X flown stage by stage: every ModuleDecouple / ModuleAnchoredDecoupler splits the vessel, parts and ownership are conserved") {
    if (!GameAvailable()) return;
    World16 w;
    LogCapture quiet;
    ResourceSetsHook rs;
    EventTrace ev;
    auto v = w.Craft(kHarness16 + "/inputs/Ships/VAB/Kerbal X.craft");
    const size_t total = v->parts.size();
    REQUIRE(total == 73);  // C5 craft table
    FlightRegistry::ActiveVessel = v.get();
    v->UpdateResourceSets();
    // decouplers in the tree (ModuleDecouple: stack decouplers and the omni separators, ModuleAnchoredDecoupler: radial)
    size_t decouplers = 0, clamps = 0;
    for (Part* p : v->parts) {
        decouplers += p->FindModuleOfClass("ModuleDecouple") != nullptr || p->FindModuleOfClass("ModuleAnchoredDecoupler") != nullptr;
        clamps += p->isLaunchClamp();
    }
    CHECK(decouplers > 0);
    CHECK(clamps == 3);
    FlightStagingHost host;
    StageManager sm(&host);
    REQUIRE(sm.Awake());
    for (Part* p : v->parts) p->ModulesOnStart(StartState::Flying);
    v->ResumeStaging();
    StageManager::BeginFlight();
    sm.FlushSort();
    const int stages = StageManager::StageCount();
    CHECK(stages == 8);  // C5: stage_count of craft_Kerbal_X
    CHECK(StageManager::CurrentStage() == 8);
    // the control-from part switches to the vessel that keeps the pod (WaitAndSwitchFocus -> FlightGlobals.ForceSetActiveVessel)
    std::vector<Vessel*> switched;
    ModuleDecouplerBase::forceSetActiveVessel = [&](Vessel* nv) { switched.push_back(nv); };
    auto check = [&](const char* when) {
        size_t sum = 0;
        std::set<Part*> seen;
        std::vector<Vessel*> all{v.get()};
        for (auto& sp : Vessel::SpawnedVessels()) all.push_back(sp.get());
        for (Vessel* x : all) {
            sum += x->parts.size();
            REQUIRE_MESSAGE(x->rootPart != nullptr, when);
            CHECK_MESSAGE(x->rootPart->parent == nullptr, when);
            CHECK_MESSAGE(x->parts[0] == x->rootPart, when);
            CHECK_MESSAGE(x->OwnedPartCount() == x->parts.size(), when);
            for (Part* p : x->parts) {
                CHECK_MESSAGE(p->vessel == x, when);
                CHECK_MESSAGE(seen.insert(p).second, when << ": a part in two vessels");
                // every non-root part's parent is in the same vessel
                if (p != x->rootPart) {
                    REQUIRE(p->parent != nullptr);
                    CHECK_MESSAGE(p->parent->vessel == x, when);
                }
            }
            // findVesselParts order: the DFS from the root visits exactly the parts list
            std::vector<Part*> dfs;
            std::function<void(Part*)> walk = [&](Part* p) {
                dfs.push_back(p);
                for (Part* c : p->children) walk(c);
            };
            walk(x->rootPart);
            CHECK_MESSAGE(dfs.size() == x->parts.size(), when);
        }
        CHECK_MESSAGE(sum == total, when << ": parts lost or duplicated");
    };
    check("before staging");
    int activated = 0;
    for (int s = stages - 1; s >= 0; s--) {
        const size_t vesselsBefore = 1 + Vessel::SpawnedVessels().size();
        ev.log.clear();
        sm.Tick(10.0);  // past the staging cooldown
        StageManager::ActivateNextStage();
        activated++;
        REQUIRE(ev.log.size() >= 1);
        CHECK(ev.log[0] == "activate:" + std::to_string(s));
        CHECK(StageManager::CurrentStage() == s);
        CHECK(v->currentStage == s);
        const std::string when = "after stage " + std::to_string(s);
        check(when.c_str());
        // each decouple event created exactly one new vessel
        CHECK(1 + Vessel::SpawnedVessels().size() == vesselsBefore + static_cast<size_t>(ev.Count("newVessel")));
        ModuleDecouplerBase::TickAll();
        ModuleDecouplerBase::TickAll();
    }
    CHECK(activated == stages);
    // every activated decoupler that had a part on its explosive node separated something
    size_t decoupled = 0;
    for (Part* p : [&] {
             std::vector<Part*> all = v->parts;
             for (auto& sp : Vessel::SpawnedVessels()) all.insert(all.end(), sp->parts.begin(), sp->parts.end());
             return all;
         }()) {
        PartModule* m = p->FindModuleOfClass("ModuleDecouple");
        if (m == nullptr) m = p->FindModuleOfClass("ModuleAnchoredDecoupler");
        if (m != nullptr && m->Get<bool>("isDecoupled")) decoupled++;
    }
    CHECK(decoupled > 0);
    CHECK(Vessel::SpawnedVessels().size() >= decoupled);  // at least one new vessel per decoupler that fired (an omni decoupler makes several)
    MESSAGE("Kerbal X: " << decouplers << " decoupler parts, " << decoupled << " fired, " << Vessel::SpawnedVessels().size() << " new vessels, " << switched.size() << " focus switches");
    ModuleDecouplerBase::forceSetActiveVessel = nullptr;
    Vessel::SpawnedVessels().clear();
    FlightRegistry::ActiveVessel = nullptr;
}

TEST_CASE("WP-16 ModuleDecouple.OnDecouple: explosive node on the parent, omni decoupler, focus switch to the vessel that holds the control-from part") {
    if (!GameAvailable()) return;
    World16 w;
    LogCapture quiet;
    ResourceSetsHook rs;
    EventTrace ev;
    auto v = w.Craft(kHarness16 + "/inputs/Ships/VAB/Kerbal X.craft");
    FlightRegistry::ActiveVessel = v.get();
    v->UpdateResourceSets();
    for (Part* p : v->parts) p->ModulesOnStart(StartState::Flying);
    // A plain stack decoupler: its explosive node holds its parent, so decouple() splits the decoupler and everything below it off
    ModuleDecouple* stack = nullptr;
    ModuleDecouple* omni = nullptr;
    for (Part* p : v->parts) {
        auto* d = dynamic_cast<ModuleDecouple*>(p->FindModuleOfClass("ModuleDecouple"));
        if (d == nullptr) continue;
        if (d->Get<bool>("isOmniDecoupler")) {
            if (omni == nullptr) omni = d;
        } else if (stack == nullptr && d->ExplosiveNode() != nullptr && d->ExplosiveNode()->attachedPart == p->parent && p->parent != nullptr && p->children.size() == 1) {
            stack = d;
        }
    }
    REQUIRE(stack != nullptr);
    Part* sp = dynamic_cast<Part*>(stack->part);
    Part* below = sp->children[0];
    // the control-from part is below the decoupler: it leaves with the lower stage
    v->SetReferenceTransform(below);
    v->ctrlState.mainThrottle = 0.75f;
    v->ActionGroups().Set(KSPActionGroup::Light, true);
    v->currentStage = 5;
    const size_t belowSize = SubtreeSize(sp);
    const size_t before = v->parts.size();
    std::vector<Vessel*> switched;
    ModuleDecouplerBase::forceSetActiveVessel = [&](Vessel* nv) { switched.push_back(nv); };
    stack->OnDecouple();
    REQUIRE(Vessel::SpawnedVessels().size() == 1);
    Vessel* lower = Vessel::SpawnedVessels()[0].get();
    CHECK(lower->rootPart == sp);
    CHECK(lower->parts.size() == belowSize);
    CHECK(v->parts.size() == before - belowSize);
    CHECK(stack->Get<bool>("isDecoupled"));
    CHECK_FALSE(stack->Events()["Decouple"]->active);
    CHECK(ev.Count("separation:") == 1);
    // Part.decouple already copied the control state and the action groups (Part.cs:2820) and gave the new vessel the stage of its own
    // highest inverseStage; WaitAndSwitchFocus (one frame later) overwrites the stage with the old vessel's and switches focus
    CHECK(lower->ctrlState.mainThrottle == 0.75f);
    CHECK(lower->ActionGroups()[KSPActionGroup::Light]);
    CHECK(lower->currentStage == StageManager::RecalculateVesselStaging(lower));
    CHECK(switched.empty());
    ModuleDecouplerBase::TickAll();
    CHECK(switched.size() == 1);
    CHECK(switched[0] == lower);
    CHECK(lower->currentStage == 5);                        // currentStage = oldVessel.currentStage
    CHECK(lower->GetReferenceTransformPart() == below);     // SetReferenceTransform(p)
    // the old vessel is now debris-or-default by its remaining parts (FindDefaultVesselType) and got a name
    CHECK(v->vesselType == v->FindDefaultVesselType());
    ModuleDecouplerBase::forceSetActiveVessel = nullptr;
    Vessel::SpawnedVessels().clear();
    FlightRegistry::ActiveVessel = nullptr;
}

TEST_CASE("WP-16 omni decoupler (Separator.x): every child leaves, then the separator itself; the ejection force is split") {
    if (!GameAvailable()) return;
    World16 w;
    LogCapture quiet;
    ResourceSetsHook rs;
    EventTrace ev;
    auto v = w.Craft(kHarness16 + "/inputs/Ships/SPH/Satellite Launcher.craft");
    FlightRegistry::ActiveVessel = v.get();
    v->UpdateResourceSets();
    for (Part* p : v->parts) p->ModulesOnStart(StartState::Flying);
    ModuleDecouple* omni = nullptr;
    for (Part* p : v->parts) {
        auto* d = dynamic_cast<ModuleDecouple*>(p->FindModuleOfClass("ModuleDecouple"));
        if (d != nullptr && d->Get<bool>("isOmniDecoupler")) {
            omni = d;
            break;
        }
    }
    REQUIRE(omni != nullptr);
    Part* op = dynamic_cast<Part*>(omni->part);
    const size_t total = v->parts.size();
    std::vector<size_t> childSizes;
    const std::vector<Part*> childParts(op->children);
    for (Part* c : op->children) childSizes.push_back(SubtreeSize(c));
    const bool hasParent = op->parent != nullptr;
    const size_t kids = childSizes.size();
    REQUIRE(kids >= 1);
    const float ej = omni->Get<float>("ejectionForce");
    omni->OnDecouple();
    // ModuleDecouple.cs:74-97: `list2 = new List<Part>(children)`, each child.decouple(); then, with a parent, part.decouple()
    CHECK(Vessel::SpawnedVessels().size() == kids + (hasParent ? 1u : 0u));
    size_t sum = v->parts.size();
    for (auto& sp : Vessel::SpawnedVessels()) sum += sp->parts.size();
    CHECK(sum == total);
    for (size_t i = 0; i < kids; i++) CHECK(Vessel::SpawnedVessels()[i]->parts.size() == childSizes[i]);
    if (hasParent) {
        CHECK(Vessel::SpawnedVessels().back()->rootPart == op);
        CHECK(Vessel::SpawnedVessels().back()->parts.size() == 1);  // the separator alone: its children left first
    }
    CHECK(op->children.empty());
    CHECK(omni->Get<bool>("isDecoupled"));
    // the ejection list: a pair per child (child gets -v, the separator +v, v = dir * ejectionForce * percent * 0.01 / (children + 1)),
    // then a pair for the parent with the force halved (ejectionForce * percent * 0.01 * 0.5)
    const auto& e = omni->LastEjections();
    REQUIRE(e.size() == 2 * kids + (hasParent ? 2u : 0u));
    const float percent = omni->Get<float>("ejectionForcePercent");
    const float perChild = f32store(fx(ej) * (fx(percent) * fx(0.01f)) / fx(static_cast<float>(kids + 1)));
    for (size_t i = 0; i < kids; i++) {
        CHECK(e[2 * i].part == childParts[i]);
        CHECK(e[2 * i + 1].part == op);
        // automaticDir: Normalize(separator.position - child.position) * perChild; opposite signs on the pair
        CHECK(e[2 * i].ejectionVector.x == doctest::Approx(-e[2 * i + 1].ejectionVector.x));
        CHECK(e[2 * i].ejectionVector.y == doctest::Approx(-e[2 * i + 1].ejectionVector.y));
        CHECK(e[2 * i + 1].ejectionVector.magnitude() <= perChild * 1.0001f);
    }
    ModuleDecouplerBase::TickAll();
    Vessel::SpawnedVessels().clear();
    FlightRegistry::ActiveVessel = nullptr;
}

// ======================================================== ToggleStaging, ClearStaging / ResumeStaging, manager life cycle

TEST_CASE("WP-16 PartModule.ToggleStaging / SetStagingState: staging switched off and on again for a symmetry pair") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    Ship s(g);
    EditorSession ed(s.tree);
    StageManager& sm = *ed.sm;
    EditorSession::StartParts(s.tree);
    ed.Load();
    REQUIRE(StageManager::StageCount() == 3);
    PartModule* m1 = s.rad1->FindModuleOfClass("ModuleAnchoredDecoupler");
    PartModule* m2 = s.rad2->FindModuleOfClass("ModuleAnchoredDecoupler");
    REQUIRE(m1 != nullptr);
    REQUIRE(m2 != nullptr);
    CHECK(m1->Get<bool>("stagingEnabled"));
    CHECK(m1->GetStagingDisableText() == KSP::Localization::Localizer::Format("#autoLOC_240329"));
    // the [KSPEvent] ToggleStaging (PartModule.cs:303) is bound on every module
    BaseEvent* ev = m1->Events()["ToggleStaging"];
    REQUIRE(ev != nullptr);
    ev->Invoke();
    // :303 + :326: m1 and the ModuleDecouplerBase of every symmetry counterpart switch; each part's stageability follows (Part.cs:7777),
    // the icons leave the stack, OnPartUpdateStageability re-sorts
    CHECK_FALSE(m1->Get<bool>("stagingEnabled"));
    CHECK_FALSE(m2->Get<bool>("stagingEnabled"));
    CHECK_FALSE(s.rad1->fields.stagingOn);
    CHECK_FALSE(s.rad2->fields.stagingOn);
    CHECK(sm.IconOf(s.rad1) == nullptr);
    CHECK(sm.IconOf(s.rad2) == nullptr);
    CHECK(m1->Events()["ToggleStaging"]->guiName() == m1->GetStagingEnableText());  // UpdateStagingToggle
    // the emptied top stage was auto-created (defaultStage != -1) and is deleted by DeleteEmptyStages
    CHECK(StageManager::StageCount() == 2);
    // no icon: the decoupler's inverseStage is the carried stage of the root (-1) and its GetStageIndex is the parent's (stagingEnabled
    // is false) -> separationIndex -1
    CHECK(s.rad1->fields.inverseStage == -1);
    CHECK(s.rad1->fields.separationIndex == -1);
    CHECK(dynamic_cast<IStageSeparator*>(m1)->GetStageIndex(7) == -1);
    // on again: the icons return and are placed at max(0, inverseStage) = stage 0 (OnPartUpdateStageability / AddHeldIconsToStages clamp)
    ev->Invoke();
    CHECK(m1->Get<bool>("stagingEnabled"));
    CHECK(m2->Get<bool>("stagingEnabled"));
    CHECK(s.rad1->fields.stagingOn);
    CHECK(s.rad2->fields.stagingOn);
    CHECK(sm.IconOf(s.rad1) != nullptr);
    CHECK(sm.IconOf(s.rad2) != nullptr);
    CHECK(s.rad1->fields.inverseStage == 0);
    CHECK(s.rad2->fields.inverseStage == 0);
    // the two icons form one symmetry group in stage 0
    StageIcon* a = sm.IconOf(s.rad1);
    StageIcon* b = sm.IconOf(s.rad2);
    CHECK(((a->groupLead == a && b->groupLead == a) || (b->groupLead == b && a->groupLead == b)));
}

TEST_CASE("WP-16 StageManager life cycle: Instance, a second manager is refused, hooks, Vessel.ClearStaging / ResumeStaging") {
    if (!GameAvailable()) return;
    Game15 g;
    LogCapture quiet;
    // without an Instance: the C# statics dereference null
    CHECK(StageManager::Instance == nullptr);
    CHECK_THROWS_AS(StageManager::StageCount(), NullReferenceException);
    CHECK_THROWS_AS(StageManager::CurrentStage(), NullReferenceException);
    CHECK(StageManager::Selection().empty());  // a fresh list, not a throw (:162)
    CHECK_FALSE(Part::stagingHooks.stageCount != nullptr);
    {
        EditorStagingHost host;
        StageManager sm(&host);
        REQUIRE(sm.Awake());
        CHECK(StageManager::Instance == &sm);
        CHECK(Part::stagingHooks.stageCount != nullptr);
        StageManager other(&host);
        CHECK_FALSE(other.Awake());  // "Only one instance of this class may exist per scene"
        CHECK(quiet.Has(log::Level::Error, "Only one instance"));
        CHECK(StageManager::Instance == &sm);
        CHECK(StageManager::StageCount() == 0);
        // the manager listens to GameEvents.onEditorLoad (:352): rebuildIndexes = true
        CHECK_FALSE(sm.rebuildIndexesValue());
        GameEvents::onEditorLoad.Fire(nullptr, static_cast<KSP::UI::Screens::CraftBrowserDialog_LoadType>(0));
        CHECK(sm.rebuildIndexesValue());
        CHECK(StageManager::LastStage() == 0);   // :146 `Count - 1 < 0 ? 0`
        CHECK(StageManager::CurrentStage() == 0);
    }
    CHECK(StageManager::Instance == nullptr);
    CHECK(Part::stagingHooks.stageCount == nullptr);  // OnDestroy: the part hooks are gone

    // Vessel.ClearStaging / ResumeStaging (Vessel.cs:2130-2153) on a vessel in flight
    World16 w;
    auto v = w.PhysProf("PhysProf-S");
    FlightRegistry::ActiveVessel = v.get();
    FlightStagingHost host;
    StageManager sm(&host);
    REQUIRE(sm.Awake());
    for (Part* p : v->parts) p->ModulesOnStart(StartState::Flying);
    int cleared = 0, resumed = 0;
    struct Counter {
        int* c;
        int* r;
        void Clear(Vessel*) { (*c)++; }
        void Resume(Vessel*) { (*r)++; }
    } cnt{&cleared, &resumed};
    GameEvents::onVesselClearStaging.Add(&cnt, &Counter::Clear);
    GameEvents::onVesselResumeStaging.Add(&cnt, &Counter::Resume);
    v->ResumeStaging();  // creates the icons of the five staging parts; fires onVesselResumeStaging
    CHECK(resumed == 1);
    size_t icons = 0;
    for (Part* p : v->parts) icons += sm.IconOf(p) != nullptr;
    CHECK(icons == 5);
    sm.FlushSort();
    v->ClearStaging();   // every stack icon removed, event fired, the manager schedules a sort
    CHECK(cleared == 1);
    icons = 0;
    for (Part* p : v->parts) icons += sm.IconOf(p) != nullptr;
    CHECK(icons == 0);
    CHECK(sm.SortPending());
    sm.FlushSort();
    GameEvents::onVesselClearStaging.Remove(&cnt, &Counter::Clear);
    GameEvents::onVesselResumeStaging.Remove(&cnt, &Counter::Resume);
    FlightRegistry::ActiveVessel = nullptr;
}
