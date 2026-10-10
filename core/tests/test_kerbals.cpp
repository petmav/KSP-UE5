// WP-31 unit tests: kerbal roster, experience, crew (Experience/*.cs, ProtoCrewMember.cs, KerbalRoster.cs, CrewGenerator.cs,
// PartCrewManifest.cs, VesselCrewManifest.cs, APSkillExtensions.cs, ModuleExperienceManagement.cs).
//
// No golden or capture exists for these classes; every expectation is derived from the decompiled C# (the derivation is in the comment
// above the check) or from the game's own data: GameData/Squad/Experience/Traits.cfg and Squad/Localization/dictionary.cfg (GameData
// mirror), the physprofile save (core/testdata/saves: the four original kerbals), the stock craft files (harness/inputs/Ships) and
// the stock part prefabs. The only synthetic inputs are the CrewGenerator name tables (the game keeps them in Unity TextAssets that the
// mirror does not have; STATUS.md "Needed captures") and EXPERIENCE_TRAIT nodes that exercise effects no stock trait uses.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/celestial/celestial_body.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/core/unity_time.hpp"
#include "ksp/dotnet/mono_float.hpp"
#include "ksp/dotnet/number.hpp"
#include "ksp/dotnet/random.hpp"
#include "ksp/kerbals/crew_generator.hpp"
#include "ksp/kerbals/crew_manifest.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/kerbals/kerbal_misc.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/orbit/time_warp.hpp"
#include "ksp/physics/physics_globals.hpp"
#include "ksp/save/game.hpp"
#include "ksp/save/game_parameters.hpp"
#include "ksp/unity/random.hpp"
#include "ksp/util/misc_util.hpp"
#include "ksp/vessel/vessel.hpp"
#include "resource_fixture.hpp"

namespace fs = std::filesystem;
using namespace ksp;
using namespace ksp::rtest;
using namespace ksp::Experience;
using GE = events::GameEvents;
using Localizer = KSP::Localization::Localizer;
using PCM = std::shared_ptr<ProtoCrewMember>;

namespace {

const std::string kHarness31 = KSPCORE_HARNESS_DIR;
const std::string kSaves31 = std::string(KSPCORE_TESTDATA_DIR) + "/saves";

std::string GameDataDir() { return ksp::testing::GameRoot() + "/GameData"; }

// Squad/Localization/dictionary.cfg as text: tag -> en-us value (the oracle for every description string)
const std::map<std::string, std::string>& Dictionary() {
    static const std::map<std::string, std::string> d = [] {
        std::map<std::string, std::string> m;
        ConfigNode::NodePtr root = ConfigNode::Load(GameDataDir() + "/Squad/Localization/dictionary.cfg");
        REQUIRE(root);
        ConfigNode* loc = root->GetNode("Localization");
        REQUIRE(loc);
        ConfigNode* lang = loc->GetNode("en-us");
        REQUIRE(lang);
        for (int i = 0; i < lang->values().Count(); ++i) m[lang->values()[i].name] = lang->values()[i].value;
        return m;
    }();
    return d;
}

// dictionary text with <<1>>, <<2>> ... replaced
std::string Dict(const std::string& tag, std::vector<std::string> args = {}) {
    auto it = Dictionary().find(tag);
    REQUIRE_MESSAGE(it != Dictionary().end(), tag);
    std::string s = it->second;
    for (size_t p = s.find("\\n"); p != std::string::npos; p = s.find("\\n", p + 1)) s.replace(p, 2, "\n");  // the dictionary stores a literal backslash-n
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string key = "<<" + std::to_string(i + 1) + ">>";
        for (size_t p = s.find(key); p != std::string::npos; p = s.find(key, p + args[i].size())) s.replace(p, key.size(), args[i]);
    }
    return s;
}

std::string F2(double v) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.2f", v);
    return buf;
}

void LoadLocalizer() {
    Localizer::Shutdown();
    Localizer::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(GameDataDir() + "/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    Localizer::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

// The stock game + PhysicsGlobals + Planetarium + the Kerbol system + the Squad Traits.cfg as GameDatabase.ExperienceConfigs, a
// CAREER game with a roster
struct World31 {
    Game15 g;
    PhysicsGlobals pg;
    Planetarium planetarium;
    std::unique_ptr<CelestialSystem> sys;
    Game game;
    World31() {
        FlightRegistry::Clear();
        SaveEnvironment& env = SaveEnvironment::Current();
        pg.physicsDatabaseFilename = kHarness31 + "/inputs/Physics.cfg";
        pg.Awake("");
        REQUIRE(pg.LoadDatabase());
        planetarium.Awake();
        sys = CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium);
        env.localBodies = sys->SpawnOrder();
        env.tickCount = [] { return 1234; };
        Vessel::env = VesselEnvironment{};
        VesselModuleManager::CompileModules();
        LoadLocalizer();
        std::vector<const ConfigNode*> traits;
        for (ConfigNode* n : g.data.database.GetConfigNodes("EXPERIENCE_TRAIT")) traits.push_back(n);
        REQUIRE(traits.size() == 4);
        env.experienceConfigs = std::make_shared<ExperienceSystemConfig>(traits);
        game.Mode = Game_Modes::CAREER;
        game.CrewRoster = std::make_shared<KerbalRoster>(Game_Modes::CAREER);
        env.currentGame = &game;
        CrewGenerator::Reset();
        ProtoCrewMember::PostScreenMessage = nullptr;
    }
    ~World31() {
        SaveEnvironment::Current().currentGame = nullptr;
        FlightRegistry::Clear();
        sys.reset();
        Planetarium::Zup = Planetarium::AwakeZup(0.0);
        Vessel::env = VesselEnvironment{};
        CrewGenerator::Reset();
        CrewGenerator::loadTextAsset = nullptr;
        ProtoCrewMember::PostScreenMessage = nullptr;
        ModuleExperienceManagement::postScreenMessage = nullptr;
    }
    CelestialBody& Body(const char* n) { return *sys->GetBodyByName(n); }
    ExperienceSystemConfig& Cfg() { return *SaveEnvironment::Current().experienceConfigs; }
};

PCM MakeKerbal(const std::string& name = "Testy Kerman", ProtoCrewMember::KerbalType type = ProtoCrewMember::KerbalType::Crew) {
    return std::make_shared<ProtoCrewMember>(type, name);
}

// the "modifiers" list of an effect config node as the test oracle reads it (strtof on each comma / space separated token)
std::vector<float> ParseModifiers(const std::string& text) {
    std::vector<float> out;
    std::string cur;
    for (char c : text + ",") {
        if (c == ',' || c == ' ') {
            if (!cur.empty()) out.push_back(std::strtof(cur.c_str(), nullptr));
            cur.clear();
        } else {
            cur += c;
        }
    }
    return out;
}

// n distinct equal-length synthetic first names (no one is a substring of another): the roster keeps 10+ applicants, so the tables must
// be large enough for GetRandomName's reject-substring loop to terminate
std::vector<std::string> SyntheticNames(char prefix, int n) {
    std::vector<std::string> v;
    for (int i = 0; i < n; ++i) v.push_back(std::string(1, prefix) + char('a' + i / 26) + char('a' + i % 26));
    return v;
}

float DefaultOf(const std::string& effect) {
    if (effect == "GeeForceTolerance" || effect == "VesselScienceReturn" || effect == "PartScienceReturn") return 1.0f;
    return 0.0f;
}

}  // namespace

// ====================================================== configs ======================================================

TEST_CASE("WP-31 Traits.cfg of the game and the built-in stock config describe the same four traits") {
    World31 w;
    ConfigNode::NodePtr root = ConfigNode::Load(GameDataDir() + "/Squad/Experience/Traits.cfg");
    REQUIRE(root);
    std::vector<const ConfigNode*> nodes;
    for (ConfigNode* n : root->GetNodes("EXPERIENCE_TRAIT")) nodes.push_back(n);
    REQUIRE(nodes.size() == 4);
    ExperienceSystemConfig fromCfg(nodes);
    auto builtIn = ExperienceSystemConfig::StockBuiltIn();
    REQUIRE(fromCfg.Categories().size() == 4);
    REQUIRE(builtIn->Categories().size() == 4);
    const char* names[4] = {"Pilot", "Engineer", "Scientist", "Tourist"};
    for (size_t i = 0; i < 4; ++i) {
        const auto& a = fromCfg.Categories()[i];
        const auto& b = builtIn->Categories()[i];
        CHECK(a->Name() == names[i]);
        CHECK(a->Name() == b->Name());
        CHECK(a->Title() == b->Title());
        CHECK(a->Description() == b->Description());
        REQUIRE(a->Effects().size() == b->Effects().size());
        for (size_t e = 0; e < a->Effects().size(); ++e) {
            CHECK(a->Effects()[e]->Name() == b->Effects()[e]->Name());
            CHECK(a->Effects()[e]->Config()->GetValue("modifiers") == b->Effects()[e]->Config()->GetValue("modifiers"));
            CHECK(a->Effects()[e]->Config()->GetValue("level") == b->Effects()[e]->Config()->GetValue("level"));
        }
    }
    CHECK(fromCfg.TraitNamesNoTourist() == std::vector<std::string>{"Pilot", "Engineer", "Scientist"});
    // the game database in the fixture loaded the same four nodes
    CHECK(w.Cfg().Categories().size() == 4);
    // every effect name used by the stock traits is a known ExperienceEffect subclass
    for (const auto& c : fromCfg.Categories())
        for (const auto& e : c->Effects()) CHECK_MESSAGE(ExperienceEffectFactory::Exists(e->Name()), e->Name());
    CHECK(fromCfg.GetTraitsNamesWithEffect("GeeForceTolerance").size() == 4);
    CHECK(fromCfg.GetTraitsNamesWithEffect("EVAChuteSkill").size() == 3);
}

TEST_CASE("WP-31 the effect registry knows the 20 ExperienceEffect subclasses of Assembly-CSharp") {
    const std::set<std::string> expected = {"AutopilotSkill", "ConverterSkill", "DeployedScienceExpSkill", "DeployedSciencePowerSkill",
                                            "DrillSkill", "EnginePower", "EVAChuteSkill", "ExternalExperimentSkill", "FailureRepairSkill",
                                            "FuelUsage", "FullVesselControlSkill", "GeeForceTolerance", "HeatProduction", "MaxThrottle",
                                            "PartScienceReturn", "RepairSkill", "ScienceResetSkill", "ScienceSkill",
                                            "SpecialExperimentSkill", "VesselScienceReturn"};
    CHECK(std::set<std::string>(ExperienceEffectFactory::TypeNames().begin(), ExperienceEffectFactory::TypeNames().end()) == expected);
    CHECK(ExperienceEffectFactory::TypeNames().size() == 20);
    for (const std::string& n : expected) {
        auto e = ExperienceEffectFactory::Create(n, nullptr);
        REQUIRE(e);
        CHECK(e->Name() == n);  // GetType().Name
    }
    CHECK(ExperienceEffectFactory::Create("NoSuchSkill", nullptr) == nullptr);
    // ExperienceEffect(parent, float[] modifiers): CopyTo(null LevelModifiers) throws, the parent is never stored
    CHECK_THROWS_AS(Effects::EnginePower(nullptr, std::vector<float>{1.0f}), ArgumentNullException);
    // ExperienceEffect(parent, int level) does not store the parent but sets Level
    Effects::EVAChuteSkill chute(nullptr, 3);
    CHECK(chute.Level() == 3);
    CHECK(chute.Parent() == nullptr);
    // LevelModifiers is null until a `modifiers` value was loaded (NullReferenceException, not an empty array)
    Effects::EnginePower ep(nullptr);
    CHECK_FALSE(ep.HasLevelModifiers());
    CHECK_THROWS_AS(ep.LevelModifiers(), NullReferenceException);
}

// ============================================ skill values per trait and level =======================================

TEST_CASE("WP-31 every effect of Traits.cfg: LevelModifiers, defaults and the values registered in Part.PartValues, levels 0..5") {
    World31 w;
    auto part = w.g.New("mk1pod.v2");
    for (const auto& cfg : w.Cfg().Categories()) {
        for (int level = 0; level <= 5; ++level) {
            PCM pcm = MakeKerbal("Level Kerman");
            pcm->experienceLevel = level;
            pcm->trait = cfg->Name();
            pcm->experienceTrait = ExperienceTrait::Create(cfg->Name(), cfg.get(), pcm.get());
            REQUIRE(pcm->experienceTrait);
            ExperienceTrait& tr = *pcm->experienceTrait;
            REQUIRE(tr.Effects().size() == cfg->Effects().size());
            CHECK(tr.TypeName() == cfg->Title().value_or(""));  // sic: Config.Title, the raw "#autoLOC_.." tag
            for (size_t e = 0; e < tr.Effects().size(); ++e) {
                ExperienceEffect& eff = *tr.Effects()[e];
                const ConfigNode& ec = *cfg->Effects()[e]->Config();
                CHECK(eff.Name() == cfg->Effects()[e]->Name());
                if (auto mods = ec.GetValue("modifiers")) {
                    std::vector<float> expected{DefaultOf(eff.Name())};
                    for (float m : ParseModifiers(*mods)) expected.push_back(m);
                    REQUIRE(eff.HasLevelModifiers());
                    CHECK(eff.LevelModifiers() == expected);
                } else {
                    CHECK_FALSE(eff.HasLevelModifiers());
                }
                if (auto lv = ec.GetValue("level")) CHECK(eff.Level() == std::stoi(*lv));
            }
            // the trait's effects registered on a part
            pcm->RegisterExperienceTraits(*part);
            PartValues& pv = part->partValues;
            pv.AutopilotSkill.Update();
            pv.AutopilotKerbalSkill.Update();
            pv.RepairSkill.Update();
            pv.FailureRepairSkill.Update();
            pv.ScienceSkill.Update();
            pv.EVAChuteSkill.Update();
            pv.ScienceReturnMax.Update();
            pv.ScienceReturnSum.Update();
            const std::string t = cfg->Name();
            CHECK(pv.AutopilotSkill.value() == (t == "Pilot" ? level : -1));
            CHECK(pv.AutopilotKerbalSkill.value() == (t == "Pilot" ? level : -1));
            CHECK(pv.RepairSkill.value() == (t == "Engineer" ? level : -1));
            CHECK(pv.FailureRepairSkill.value() == (t == "Engineer" ? level : -1));
            CHECK(pv.ScienceSkill.value() == (t == "Scientist" ? level : -1));
            // EVAChuteSkill: the config `level`; PartValues default 0 for the Tourist
            CHECK(pv.EVAChuteSkill.value() == (t == "Pilot" ? 0 : (t == "Tourist" ? 0 : 1)));
            if (t == "Scientist") {
                const std::vector<float> m = ParseModifiers(*cfg->Effects()[0]->Config()->GetValue("modifiers"));
                const float mod = level == 0 ? 1.0f : m[static_cast<size_t>(level) - 1];  // [0] = default 1, then the five modifiers
                CHECK(pv.ScienceReturnMax.value() == mod);
                CHECK(pv.ScienceReturnSum.value() == 1.0f + mod);  // EventValueOperation: default 1, a + b
            } else {
                CHECK(pv.ScienceReturnMax.value() == 1.0f);
                CHECK(pv.ScienceReturnSum.value() == 1.0f);
            }
            // GeeTolerance: level 0 extrapolates down (m1 - (m2 - m1)), else the level's modifier (index capped at Length - 1)
            auto* gee = pcm->GetEffect<Effects::GeeForceTolerance>();
            REQUIRE(gee != nullptr);
            size_t geeIdx = 0;
            for (size_t i = 0; i < cfg->Effects().size(); ++i)
                if (cfg->Effects()[i]->Name() == "GeeForceTolerance") geeIdx = i;
            const std::vector<float> gm = ParseModifiers(*cfg->Effects()[geeIdx]->Config()->GetValue("modifiers"));
            if (level == 0)
                CHECK(gee->GeeTolerance() == static_cast<float>(double(gm[0]) - (double(gm[1]) - double(gm[0]))));
            else
                CHECK(gee->GeeTolerance() == gm[static_cast<size_t>(level) - 1]);
            // Unregister removes every delegate: back to the defaults
            pcm->UnregisterExperienceTraits(*part);
            pv.AutopilotSkill.Update();
            pv.EVAChuteSkill.Update();
            pv.ScienceReturnSum.Update();
            CHECK(pv.AutopilotSkill.value() == -1);
            CHECK(pv.EVAChuteSkill.value() == 0);
            CHECK(pv.ScienceReturnSum.value() == 1.0f);
        }
    }
}

TEST_CASE("WP-31 the best of several kerbals wins in PartValues; a negative level counts as 0; GetEffect searches from the end") {
    World31 w;
    auto part = w.g.New("mk1pod.v2");
    ExperienceTraitConfig* pilot = w.Cfg().GetExperienceTraitConfig("Pilot");
    REQUIRE(pilot);
    PCM a = MakeKerbal("A Kerman"), b = MakeKerbal("B Kerman");
    a->experienceLevel = 2;
    b->experienceLevel = 4;
    a->experienceTrait = ExperienceTrait::Create("Pilot", pilot, a.get());
    b->experienceTrait = ExperienceTrait::Create("Pilot", pilot, b.get());
    a->RegisterExperienceTraits(*part);
    b->RegisterExperienceTraits(*part);
    part->partValues.AutopilotSkill.Update();
    CHECK(part->partValues.AutopilotSkill.value() == 4);
    b->UnregisterExperienceTraits(*part);
    part->partValues.AutopilotSkill.Update();
    CHECK(part->partValues.AutopilotSkill.value() == 2);
    a->UnregisterExperienceTraits(*part);
    a->experienceLevel = -3;
    CHECK(a->experienceTrait->CrewMemberExperienceLevel() == 0);
    CHECK(a->experienceTrait->CrewMemberExperienceLevel(3) == 0);
    a->experienceLevel = 5;
    CHECK(a->experienceTrait->CrewMemberExperienceLevel(3) == 2);  // min(level, cap - 1)
    // GetEffect(name): last match first, by GetType().Name; HasEffect
    CHECK(a->HasEffect("GeeForceTolerance"));
    CHECK_FALSE(a->HasEffect("RepairSkill"));
    CHECK(a->GetEffect("EVAChuteSkill")->Level() == 0);
    // EVAChuteSkill.GetValue returns the first EVAChuteSkill of the parent's effects
    auto* chute = a->GetEffect<Effects::EVAChuteSkill>();
    REQUIRE(chute);
    CHECK(chute->GetValue() == 0);
    // no trait: NullReferenceException (experienceTrait.Effects)
    PCM none = MakeKerbal("None Kerman");
    CHECK_THROWS_AS(none->HasEffect("X"), NullReferenceException);
    // registering on a kerbal without a trait does nothing
    none->RegisterExperienceTraits(*part);
}

TEST_CASE("WP-31 effects of mods / DLC traits that no stock trait uses: EnginePower, FuelUsage, HeatProduction, MaxThrottle, Serenity skills") {
    World31 w;
    ConfigNode::NodePtr cfgRoot = ConfigNode::Parse(
        "EXPERIENCE_TRAIT\n{\nname = Synth\ntitle = #autoLOC_500101\ndesc = #autoLOC_500102\n"
        "EFFECT\n{\nname = EnginePower\nmodifiers = 1.1, 1.2, 1.3, 1.4, 1.5\n}\n"
        "EFFECT\n{\nname = FuelUsage\nmodifiers = 0.9, 0.8, 0.7, 0.6, 0.5\n}\n"
        "EFFECT\n{\nname = HeatProduction\nmodifiers = 0.95, 0.9, 0.85, 0.8, 0.75\n}\n"
        "EFFECT\n{\nname = MaxThrottle\nmodifiers = 1.01, 1.02, 1.03, 1.04, 1.05\n}\n"
        "EFFECT\n{\nname = DeployedScienceExpSkill\nmodifiers = 0.3, 0.35, 0.4, 0.45, 0.5\n}\n"
        "EFFECT\n{\nname = DeployedSciencePowerSkill\nmodifiers = 0.1, 0.2, 0.3, 0.4, 0.5\n}\n}\n");
    std::vector<const ConfigNode*> nodes{cfgRoot->GetNode("EXPERIENCE_TRAIT")};
    ExperienceSystemConfig cfg(nodes);
    ExperienceTraitConfig* synth = cfg.GetExperienceTraitConfig("Synth");
    REQUIRE(synth);
    auto part = w.g.New("mk1pod.v2");
    SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", true);
    for (int level = 0; level <= 5; ++level) {
        PCM pcm = MakeKerbal("Synth Kerman");
        pcm->experienceLevel = level;
        pcm->experienceTrait = ExperienceTrait::Create("Synth", synth, pcm.get());
        pcm->RegisterExperienceTraits(*part);
        PartValues& pv = part->partValues;
        pv.EnginePower.Update();
        pv.HeatProduction.Update();
        pv.FuelUsage.Update();
        pv.MaxThrottle.Update();
        // LevelModifiers[min(level, Length - 1)] with [0] = the effect's default (1 for these four)
        const float ep[6] = {1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f};
        const float fu[6] = {1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f};
        const float hp[6] = {1.0f, 0.95f, 0.9f, 0.85f, 0.8f, 0.75f};
        const float mt[6] = {1.0f, 1.01f, 1.02f, 1.03f, 1.04f, 1.05f};
        // EnginePower / MaxThrottle: the larger wins (default 1); FuelUsage / HeatProduction: the smaller wins (default 1)
        CHECK(pv.EnginePower.value() == std::max(1.0f, ep[level]));
        CHECK(pv.MaxThrottle.value() == std::max(1.0f, mt[level]));
        CHECK(pv.FuelUsage.value() == std::min(1.0f, fu[level]));
        CHECK(pv.HeatProduction.value() == std::min(1.0f, hp[level]));
        auto* exp = static_cast<Effects::DeployedScienceExpSkill*>(pcm->GetEffect("DeployedScienceExpSkill"));
        auto* power = static_cast<Effects::DeployedSciencePowerSkill*>(pcm->GetEffect("DeployedSciencePowerSkill"));
        REQUIRE(exp);
        REQUIRE(power);
        const float em[6] = {0.25f, 0.3f, 0.35f, 0.4f, 0.45f, 0.5f};  // [0] = 0.25 with Serenity installed
        CHECK(exp->GetValue() == em[level]);
        const float pm[6] = {1.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f};
        // level 0: GetDefaultValue() (1); else 1 + LevelModifiers[level]
        CHECK(power->GetValue() == (level == 0 ? 1.0f : 1.0f + pm[level]));
        CHECK(power->GetTraitBonus(level) == 1.0f + pm[level]);
        CHECK(power->GetTraitBonus(6) == 1.0f);  // level >= Length
        pcm->UnregisterExperienceTraits(*part);
    }
    // without Serenity both skills are 0 and have no description
    SaveEnvironment::Current().SetStockExpansionInstalled("Serenity", false);
    PCM pcm = MakeKerbal("Synth Kerman");
    pcm->experienceTrait = ExperienceTrait::Create("Synth", synth, pcm.get());
    CHECK(static_cast<Effects::DeployedScienceExpSkill*>(pcm->GetEffect("DeployedScienceExpSkill"))->GetValue() == 0.0f);
    CHECK(static_cast<Effects::DeployedSciencePowerSkill*>(pcm->GetEffect("DeployedSciencePowerSkill"))->GetValue() == 0.0f);
    CHECK(pcm->GetEffect("DeployedScienceExpSkill")->Description() == "");
}

TEST_CASE("WP-31 effect descriptions against the game's localization dictionary") {
    World31 w;
    for (const auto& cfg : w.Cfg().Categories()) {
        for (int level = 0; level <= 5; ++level) {
            PCM pcm = MakeKerbal("Desc Kerman");
            pcm->experienceLevel = level;
            pcm->experienceTrait = ExperienceTrait::Create(cfg->Name(), cfg.get(), pcm.get());
            ExperienceTrait& tr = *pcm->experienceTrait;
            // GKerbalLimits off: GeeForceTolerance has no description; CommNet on: FullVesselControlSkill has one
            w.game.Parameters->CustomParams<GameParameters::AdvancedParams>().GKerbalLimits = false;
            w.game.Parameters->Difficulty.EnableCommNet = true;
            std::string joined;
            for (const auto& e : tr.Effects()) {
                const std::string n = e->Name();
                std::string expected;
                if (n == "AutopilotSkill") {
                    const char* tags[4] = {"#autoLOC_18284", "#autoLOC_18285", "#autoLOC_18286", "#autoLOC_18287"};
                    for (int i = 0; i < std::min(level + 1, 4); ++i) expected += (i ? "\n" : "") + Dict(tags[i]);
                } else if (n == "FullVesselControlSkill") {
                    expected = Dict("#autoLOC_18556");
                } else if (n == "ConverterSkill") {
                    expected = Dict("#autoLOC_18401");
                } else if (n == "DrillSkill") {
                    expected = Dict("#autoLOC_18426");
                } else if (n == "ExternalExperimentSkill") {
                    expected = Dict("#autoLOC_18491");
                } else if (n == "ScienceResetSkill") {
                    expected = Dict("#autoLOC_18818");
                } else if (n == "RepairSkill" && level > 0) {
                    expected = Dict("#autoLOC_18766");
                    const char* tags[3] = {"#autoLOC_18738", "#autoLOC_18739", "#autoLOC_18740"};  // Parachutes 1, LandingLegs 2, Wheels 3
                    for (int i = 0; i < 3; ++i)
                        if (level >= i + 1) expected += "\n" + Dict(tags[i]);
                } else if (n == "FailureRepairSkill" && level > 0) {
                    expected = Dict("#autoLOC_8003452") + "\n" + "Repair Failures";
                } else if (n == "VesselScienceReturn" || n == "PartScienceReturn") {
                    const std::vector<float> m = ParseModifiers(*e->Parent()->Config()->Effects()[0]->Config()->GetValue("modifiers"));
                    const float v = level == 0 ? 1.0f : m[static_cast<size_t>(level) - 1];
                    expected = Dict(n == "VesselScienceReturn" ? "#autoLOC_18908" : "#autoLOC_18706", {F2(double(v * 100.0f))});
                }
                CHECK_MESSAGE(e->Description() == expected, cfg->Name() << " " << n << " level " << level);
                if (!expected.empty()) joined += (joined.empty() ? "" : "\n") + expected;
            }
            CHECK(tr.DescriptionEffects() == joined);
            // GKerbalLimits on: GeeForceTolerance describes its tolerance with F2
            w.game.Parameters->CustomParams<GameParameters::AdvancedParams>().GKerbalLimits = true;
            auto* gee = pcm->GetEffect<Effects::GeeForceTolerance>();
            REQUIRE(gee);
            CHECK(gee->Description() == Dict("#autoLOC_18586", {F2(gee->GeeTolerance())}));
            // CommNet off: no FullVesselControlSkill text
            w.game.Parameters->Difficulty.EnableCommNet = false;
            if (auto* fv = pcm->GetEffect("FullVesselControlSkill")) CHECK(fv->Description() == "");
            CHECK(tr.Title() == cfg->Title().value_or(""));
            CHECK(tr.Description() == cfg->Description().value_or(""));
        }
    }
}

// ======================================================= APSkill ====================================================

TEST_CASE("WP-31 APSkillExtensions: required skill per mode and AvailableAtLevel in every game mode") {
    using M = VesselAutopilot_AutopilotMode;
    const std::pair<M, int> req[] = {{M::StabilityAssist, 0}, {M::Prograde, 1},  {M::Retrograde, 1}, {M::Normal, 2}, {M::Antinormal, 2},
                                     {M::RadialIn, 2},        {M::RadialOut, 2}, {M::Target, 3},     {M::AntiTarget, 3}, {M::Maneuver, 3}};
    for (auto [m, n] : req) CHECK(APSkillExtensions::GetRequiredSkill(m) == n);
    // the AutopilotSkill enum of Effects/AutopilotSkill.cs mirrors the same table
    CHECK(Effects::AutopilotSkill::Prograde == 1);
    CHECK(Effects::AutopilotSkill::Normal == 2);
    CHECK(Effects::AutopilotSkill::Maneuver == 3);

    World31 w;
    auto& adv = w.game.Parameters->CustomParams<GameParameters::AdvancedParams>();
    Vessel v;
    auto part = w.g.New("mk1pod.v2");
    part->vessel = &v;
    v.parts.push_back(part.get());
    // set the part's three skill values by hand (they are the EventValue results the effects would register)
    auto setSkills = [&](int pilot, int kerbal, int sas, int eng, int sci) {
        part->partValues.AutopilotSkill.Add(Delegate<int()>::Lambda(&v, [=] { return pilot; }));
        part->partValues.AutopilotKerbalSkill.Add(Delegate<int()>::Lambda(&v, [=] { return kerbal; }));
        part->partValues.AutopilotSASSkill.Add(Delegate<int()>::Lambda(&v, [=] { return sas; }));
        part->partValues.RepairSkill.Add(Delegate<int()>::Lambda(&v, [=] { return eng; }));
        part->partValues.ScienceSkill.Add(Delegate<int()>::Lambda(&v, [=] { return sci; }));
        part->partValues.AutopilotSkill.Update();
        part->partValues.AutopilotKerbalSkill.Update();
        part->partValues.AutopilotSASSkill.Update();
        part->partValues.RepairSkill.Update();
        part->partValues.ScienceSkill.Update();
        Time::frameCount++;  // VesselValues caches per frame
    };
    setSkills(2, 2, -1, -1, -1);
    // CAREER with experience on: the pilot level decides
    w.game.Mode = Game_Modes::CAREER;
    CHECK(APSkillExtensions::AvailableAtLevel(M::Prograde, v));
    CHECK(APSkillExtensions::AvailableAtLevel(M::RadialIn, v));
    CHECK_FALSE(APSkillExtensions::AvailableAtLevel(M::Target, v));
    CHECK(APSkillExtensions::AvailableAtLevel(M::StabilityAssist, v));
    // SANDBOX, experience off, no EnableFullSASInSandbox: a crewed vessel (any skill > -1) is unrestricted
    w.game.Mode = Game_Modes::SANDBOX;
    adv.SetEnableKerbalExperience(false);
    adv.EnableFullSASInSandbox = false;
    CHECK(APSkillExtensions::AvailableAtLevel(M::Target, v));
    // SANDBOX, experience on, full SAS off: AutopilotSkill decides
    adv.SetEnableKerbalExperience(true);
    CHECK_FALSE(APSkillExtensions::AvailableAtLevel(M::Target, v));
    CHECK(APSkillExtensions::AvailableAtLevel(M::Normal, v));
    // SANDBOX, experience on, full SAS on: with a kerbal the pilot skill decides, uncrewed the SAS module skill does
    adv.EnableFullSASInSandbox = true;
    CHECK_FALSE(APSkillExtensions::AvailableAtLevel(M::Target, v));
    // MISSION with Making History: EnableFullSASInMissions short-circuits
    SaveEnvironment::Current().SetStockExpansionInstalled("MakingHistory", true);
    w.game.Mode = Game_Modes::MISSION;
    adv.EnableFullSASInMissions = true;
    CHECK(APSkillExtensions::AvailableAtLevel(M::Target, v));
    adv.EnableFullSASInMissions = false;
    CHECK_FALSE(APSkillExtensions::AvailableAtLevel(M::Target, v));
    // the obsolete int overload
    w.game.Mode = Game_Modes::CAREER;
    CHECK(APSkillExtensions::AvailableAtLevel(M::Target, 3));
    CHECK_FALSE(APSkillExtensions::AvailableAtLevel(M::Target, 2));
    adv.SetEnableKerbalExperience(false);
    w.game.Mode = Game_Modes::SANDBOX;
    CHECK(APSkillExtensions::AvailableAtLevel(M::Target, 0));
    part->vessel = nullptr;
    v.parts.clear();
}

// ================================================ experience points ================================================

TEST_CASE("WP-31 experience points from the flight logs: per-target maximum, home vs away values, Die clears, levels") {
    World31 w;
    const float rvKerbin = w.Body("Kerbin").scienceValues.RecoveryValue;
    const float rvMun = w.Body("Mun").scienceValues.RecoveryValue;
    const float rvMinmus = w.Body("Minmus").scienceValues.RecoveryValue;
    REQUIRE(w.Body("Kerbin").isHomeWorld);
    REQUIRE_FALSE(w.Body("Mun").isHomeWorld);
    // KerbalRoster.cs:1225 InitializeExperienceTypes: home values / away values per entry type
    //   Flight 1.0 / 2.0, Suborbit 1.0 / (none), Flyby 1.5 / 1.0, Orbit 2.0 / 1.5, Land (none) / 2.3, PlantFlag (none) / 2.5, Escape (none) / 1.0
    PCM k = MakeKerbal("Xp Kerman");
    // career log: flight 0 = Flight, Suborbit, Orbit at Kerbin; Land, Flyby at Mun
    k->careerLog.AddEntry("Flight", std::string("Kerbin"));
    k->careerLog.AddEntry("Suborbit", std::string("Kerbin"));
    k->careerLog.AddEntry("Orbit", std::string("Kerbin"));
    k->careerLog.AddEntry("Land", std::string("Mun"));
    k->careerLog.AddEntry("Flyby", std::string("Mun"));
    // Kerbin: max(1, 1, 2) * rv; Mun: max(2.3, 1.0) * rv (the log sum order is target insertion order)
    float expected = 0.0f;
    expected += rvKerbin * 2.0f;
    expected += rvMun * 2.3f;
    CHECK(k->CalculateExperiencePoints(*w.game.Parameters, Game_Modes::CAREER) == expected);
    // a second flight adds a better value at Minmus and a lower one at Mun
    k->careerLog.AddFlight();
    k->careerLog.AddEntry("PlantFlag", std::string("Minmus"));
    k->careerLog.AddEntry("Orbit", std::string("Mun"));
    expected += rvMinmus * 2.5f;
    CHECK(k->CalculateExperiencePoints(*w.game.Parameters, Game_Modes::CAREER) == expected);
    // a flight whose last entry is Die is rejected and clears everything gathered so far (sic)
    PCM dead = MakeKerbal("Dead Kerman");
    dead->careerLog.AddEntry("Orbit", std::string("Kerbin"));
    dead->careerLog.AddFlight();
    dead->careerLog.AddEntry("Flight", std::string("Kerbin"));
    dead->careerLog.AddEntry("Die", std::string("Kerbin"));
    CHECK(dead->CalculateExperiencePoints(*w.game.Parameters, Game_Modes::CAREER) == 0.0f);
    // unknown bodies and empty targets add nothing
    PCM odd = MakeKerbal("Odd Kerman");
    odd->careerLog.AddEntry("Orbit", std::string("Nowhere"));
    odd->careerLog.AddEntry("Orbit");
    CHECK(odd->CalculateExperiencePoints(*w.game.Parameters, Game_Modes::CAREER) == 0.0f);
    // the flight log counts only with ImmediateLevelUp; the career log alone otherwise
    PCM imm = MakeKerbal("Imm Kerman");
    imm->flightLog.AddEntry("Orbit", std::string("Kerbin"));
    CHECK(imm->CalculateExperiencePoints(*w.game.Parameters, Game_Modes::CAREER) == 0.0f);
    w.game.Parameters->CustomParams<GameParameters::AdvancedParams>().ImmediateLevelUp = true;
    CHECK(imm->CalculateExperiencePoints(*w.game.Parameters, Game_Modes::CAREER) == rvKerbin * 2.0f);
    w.game.Parameters->CustomParams<GameParameters::AdvancedParams>().ImmediateLevelUp = false;
    // experience disabled (SANDBOX default): 99999
    CHECK(imm->CalculateExperiencePoints(*w.game.Parameters, Game_Modes::SANDBOX) == 99999.0f);
    // levels: the thresholds 2, 8, 16, 32, 64
    const std::pair<float, int> lv[] = {{0.0f, 0}, {1.99f, 0}, {2.0f, 1}, {7.99f, 1}, {8.0f, 2}, {16.0f, 3}, {32.0f, 4}, {63.9f, 4}, {64.0f, 5}, {99999.0f, 5}};
    for (auto [xp, level] : lv) CHECK(KerbalRoster::CalculateExperienceLevel(xp) == level);
    CHECK(KerbalRoster::GetExperienceMaxLevel() == 5);
    CHECK(KerbalRoster::GetExperienceLevelRequirement(-1) == 0.0f);
    CHECK(KerbalRoster::GetExperienceLevelRequirement(0) == 2.0f);
    CHECK(KerbalRoster::GetExperienceLevelRequirement(4) == 64.0f);
    CHECK(KerbalRoster::GetExperienceLevelRequirement(9) == 64.0f);
    // UpdateExperience / ArchiveFlightLog: levels, the level-up event, ExperienceLevelDelta, extra experience
    struct Ev {
        int levelUps = 0;
        void Up(ProtoCrewMember*) { levelUps++; }
    } ev;
    GE::onKerbalLevelUp.Add(&ev, &Ev::Up);
    PCM p = MakeKerbal("Lv Kerman");
    p->flightLog.AddEntry("Orbit", std::string("Mun"));  // 1.5 * rvMun
    p->flightLog.AddEntry("Land", std::string("Mun"));   // 2.3 * rvMun
    p->ArchiveFlightLog();
    CHECK(p->careerLog.Count() == 2);
    CHECK(p->flightLog.Count() == 0);
    CHECK(p->careerLog.Flight() == 1);
    CHECK(p->flightLog.Flight() == 1);
    CHECK(p->experience == rvMun * 2.3f);
    CHECK(p->experienceLevel == KerbalRoster::CalculateExperienceLevel(rvMun * 2.3f));
    CHECK(ev.levelUps == (p->experienceLevel != 0 ? 1 : 0));
    p->SetExtraExperience(100.0f);  // property setter: UpdateExperience
    CHECK(p->ExtraExperience() == 100.0f);
    CHECK(p->ExperienceLevelDelta() >= 0.0f);
    GE::onKerbalLevelUp.Remove(&ev, &Ev::Up);
    // SetExperienceLevel: one Training entry at the home body; level 0 only clears; sandbox grants the maximum
    PCM t = MakeKerbal("Train Kerman");
    KerbalRoster::SetExperienceLevel(*t, 3);
    CHECK(t->careerLog.Count() == 1);
    CHECK(t->careerLog[0].type == "Training3");
    CHECK(t->careerLog[0].target == "Kerbin");
    CHECK(t->experience == 16.0f * rvKerbin);
    CHECK(t->experienceLevel == KerbalRoster::CalculateExperienceLevel(16.0f * rvKerbin));
    KerbalRoster::SetExperienceLevel(*t, 0);
    CHECK(t->careerLog.Count() == 0);
    KerbalRoster::SetExperienceLevel(*t, 99);  // clamped to 5
    CHECK(t->careerLog[0].type == "Training5");
}

TEST_CASE("WP-31 GenerateExperienceLog and CheatExperience") {
    World31 w;
    FlightLog log;
    log.AddEntry("Orbit", std::string("Kerbin"));
    log.AddEntry("Land", std::string("Mun"));
    const std::string text = KerbalRoster::GenerateExperienceLog(log);
    // per target: Localizer.Format(typeName, displayName) + " = " + xp.ToString("F0"), joined by '\n'
    const float rvKerbin = w.Body("Kerbin").scienceValues.RecoveryValue;
    const float rvMun = w.Body("Mun").scienceValues.RecoveryValue;
    auto disp = [&](const char* n) {
        const std::string d = w.Body(n).displayName();
        std::string r = d.rfind("#", 0) == 0 ? Dict(d) : d;  // Localizer.Format localizes the argument too
        const size_t caret = r.find('^');                       // ... and strips the Lingoona gender suffix
        return caret == std::string::npos ? r : r.substr(0, caret);
    };
    const std::string l1 = Dict("#autoLOC_6002264", {disp("Kerbin")}) + " = " + F2(double(rvKerbin * 2.0f)).substr(0, F2(double(rvKerbin * 2.0f)).find('.'));
    const std::string l2 = Dict("#autoLOC_286262", {disp("Mun")}) + " = " + std::to_string(int(std::lround(double(rvMun * 2.3f))));
    CHECK(text == l1 + "\n" + l2);
    // CheatExperience: every Crew kerbal to the maximum level, only when experience is enabled
    PCM a = MakeKerbal("Cheat Kerman");
    PCM tourist = MakeKerbal("Tourist Kerman", ProtoCrewMember::KerbalType::Tourist);
    REQUIRE(w.game.CrewRoster->AddCrewMember(a));
    REQUIRE(w.game.CrewRoster->AddCrewMember(tourist));
    KerbalRoster::CheatExperience();
    CHECK(a->careerLog.Count() == 1);
    CHECK(a->careerLog[0].type == "Training5");
    CHECK(tourist->careerLog.Count() == 0);
    w.game.Mode = Game_Modes::SANDBOX;
    w.game.Parameters->CustomParams<GameParameters::AdvancedParams>().SetEnableKerbalExperience(false);
    a->careerLog.Clear();
    KerbalRoster::CheatExperience();
    CHECK(a->careerLog.Count() == 0);
}

// ================================================ ProtoCrewMember ==================================================

namespace {
struct PcmEvents {
    std::vector<std::string> log;
    void St(ProtoCrewMember*, ProtoCrewMember_RosterStatus f, ProtoCrewMember_RosterStatus t) { log.push_back("status " + std::to_string(int(f)) + ">" + std::to_string(int(t))); }
    void Sd(ProtoCrewMember*, ProtoCrewMember_RosterStatus f, ProtoCrewMember_RosterStatus t) { log.push_back("statusd " + std::to_string(int(f)) + ">" + std::to_string(int(t))); }
    void Ty(ProtoCrewMember*, ProtoCrewMember_KerbalType f, ProtoCrewMember_KerbalType t) { log.push_back("type " + std::to_string(int(f)) + ">" + std::to_string(int(t))); }
    void In(ProtoCrewMember*, bool f, bool t) { log.push_back(std::string("inactive ") + (f ? "1" : "0") + ">" + (t ? "1" : "0")); }
    PcmEvents() {
        GE::onKerbalStatusChange.Add(this, &PcmEvents::St);
        GE::onKerbalStatusChanged.Add(this, &PcmEvents::Sd);
        GE::onKerbalTypeChange.Add(this, &PcmEvents::Ty);
        GE::onKerbalInactiveChange.Add(this, &PcmEvents::In);
    }
    ~PcmEvents() {
        GE::onKerbalStatusChange.Remove(this, &PcmEvents::St);
        GE::onKerbalStatusChanged.Remove(this, &PcmEvents::Sd);
        GE::onKerbalTypeChange.Remove(this, &PcmEvents::Ty);
        GE::onKerbalInactiveChange.Remove(this, &PcmEvents::In);
    }
};
}  // namespace

TEST_CASE("WP-31 ProtoCrewMember state machine: status / type / inactive transitions, respawn, Die") {
    World31 w;
    PcmEvents ev;
    PCM k = MakeKerbal("State Kerman");
    // rosterStatus: Change fires before the assignment, Changed after (ProtoCrewMember.cs:226)
    k->set_rosterStatus(ProtoCrewMember::RosterStatus::Assigned);
    CHECK(ev.log == std::vector<std::string>{"status 0>1", "statusd 0>1"});
    // type: onKerbalTypeChange from > to
    ev.log.clear();
    k->set_type(ProtoCrewMember::KerbalType::Tourist);
    CHECK(ev.log == std::vector<std::string>{"type 0>3"});
    k->set_type(ProtoCrewMember::KerbalType::Crew);
    // inactive: fires only when the value changes
    ev.log.clear();
    k->set_inactive(true);
    k->set_inactive(true);
    CHECK(ev.log == std::vector<std::string>{"inactive 0>1"});
    k->set_inactive(false);
    // SetInactive(time, additive): additive extends the end time, non-additive resets it from UT (Planetarium)
    ProtoCrewMember::PostScreenMessage = nullptr;
    const double ut = SaveUniversalTime();
    CHECK_FALSE(k->SetInactive(100.0, false));  // returns the previous _inactive
    CHECK(k->inactive());
    CHECK(k->inactiveTimeEnd == ut + 100.0);
    CHECK(k->SetInactive(50.0, true));  // already inactive: additive
    CHECK(k->inactiveTimeEnd == ut + 150.0);
    // CheckActive: still inactive before the end, active after (and the flag clears)
    CHECK_FALSE(k->CheckActive(ut + 10.0));
    CHECK(k->inactive());
    CHECK(k->CheckActive(ut + 151.0));
    CHECK_FALSE(k->inactive());
    // Update(UT): CheckActive while inactive, then the static callback
    double cbUT = -1.0;
    ProtoCrewMember::CallbackUpdate = [&](ProtoCrewMember&, double u) { cbUT = u; };
    k->SetInactive(10.0, false);
    k->Update(ut + 500.0);
    CHECK_FALSE(k->inactive());
    CHECK(cbUT == ut + 500.0);
    ProtoCrewMember::CallbackUpdate = nullptr;
    // Missing + respawn timer: StartRespawnPeriod uses the difficulty RespawnTimer, CheckRespawnTimer respawns or kills
    k->set_rosterStatus(ProtoCrewMember::RosterStatus::Assigned);
    k->StartRespawnPeriod(200.0);
    CHECK(k->rosterStatus() == ProtoCrewMember::RosterStatus::Missing);
    CHECK(k->UTaR == ut + 200.0);
    w.game.Parameters->Difficulty.MissingCrewsRespawn = true;
    k->CheckRespawnTimer(ut + 100.0, *w.game.Parameters);
    CHECK(k->rosterStatus() == ProtoCrewMember::RosterStatus::Missing);
    k->CheckRespawnTimer(ut + 200.0, *w.game.Parameters);
    CHECK(k->rosterStatus() == ProtoCrewMember::RosterStatus::Available);
    CHECK(k->flightLog.HasEntry("Spawn"));
    k->StartRespawnPeriod(-1.0);
    CHECK(k->UTaR == ut + w.game.Parameters->Difficulty.RespawnTimer);
    w.game.Parameters->Difficulty.MissingCrewsRespawn = false;
    k->CheckRespawnTimer(k->UTaR + 1.0, *w.game.Parameters);  // never respawns: Die
    CHECK(k->rosterStatus() == ProtoCrewMember::RosterStatus::Dead);
    CHECK(k->flightLog.Count() == 0);   // Die archived the flight log into the career log ...
    CHECK(k->careerLog.HasEntry("Die"));
    // a Tourist who dies leaves the roster
    PCM tr = MakeKerbal("Tour Kerman", ProtoCrewMember::KerbalType::Tourist);
    REQUIRE(w.game.CrewRoster->AddCrewMember(tr));
    tr->Die();
    CHECK_FALSE(w.game.CrewRoster->Exists("Tour Kerman"));
}

TEST_CASE("WP-31 ProtoCrewMember: names, suits, EVA / IVA names, icons, Equals / GetHashCode, localized strings") {
    World31 w;
    PCM k = MakeKerbal("Bob Kerman");
    CHECK(k->nameWithGender() == "Bob Kerman^M");  // LocalizeName(Male): "^M"
    k->set_gender(ProtoCrewMember::Gender::Female);
    CHECK(k->nameWithGender() == "Bob Kerman^F");
    CHECK(k->GetKerbalEVAPartName() == "kerbalEVAfemale");
    CHECK(k->GetKerbalIVAName() == "kerbalFemale");
    k->set_gender(ProtoCrewMember::Gender::Male);
    CHECK(k->GetKerbalEVAPartName() == "kerbalEVA");
    CHECK(k->GetKerbalIVAName() == "kerbalMale");
    // DLC suits fall back to the default one without the expansion; Slim needs none
    k->set_suit(ProtoCrewMember::KerbalSuit::Slim);
    CHECK(k->GetKerbalEVAPartName() == "kerbalEVASlimSuit");
    CHECK(k->GetKerbalIVAName() == "slimSuitIVAMale");
    CHECK(k->GetKerbalIconSuitSuffix() == "_slim");
    CHECK(k->GetKerbalIconSuitPath() == "kerbalicon_suit_slim");
    k->set_suit(ProtoCrewMember::KerbalSuit::Default);
    CHECK(k->GetKerbalIconSuitSuffix() == "");
    CHECK(k->GetKerbalIconSuitPath() == "kerbalicon_suit");
    k->veteran = true;
    CHECK(k->GetKerbalIconSuitPath() == "kerbalicon_suit_orange");
    k->veteran = false;
    k->set_type(ProtoCrewMember::KerbalType::Applicant);
    CHECK(k->GetKerbalIconSuitPath() == "kerbalicon_recruit");
    k->set_type(ProtoCrewMember::KerbalType::Tourist);
    CHECK(k->GetKerbalIconSuitPath() == "kerbalicon_tourist");
    k->SpritePath = "mod/sprite";
    CHECK(k->GetKerbalIconSuitPath() == "mod/sprite");
    // GetLocalizedType / Status return the enum [Description] tag, GetLocalizedTrait the trait name string
    k->set_type(ProtoCrewMember::KerbalType::Crew);
    CHECK(k->GetLocalizedType() == "#autoLoc_6002189");
    CHECK(k->GetLocalizedStatus() == "#autoLOC_901072");
    k->trait = "Pilot";
    CHECK(k->GetLocalizedTrait() == Dict("#autoLOC_900433"));
    // Equals / GetHashCode: equal fields (hasToured.Equals(hasToured) is a self-comparison, sic)
    PCM a = MakeKerbal("Same Kerman"), b = MakeKerbal("Same Kerman");
    a->courage = b->courage = 0.25f;
    a->stupidity = b->stupidity = 0.75f;
    CHECK(a->Equals(*b));
    CHECK(a->GetHashCode() == b->GetHashCode());
    b->hasToured = true;
    CHECK(a->Equals(*b));  // not compared
    b->courage = 0.5f;
    CHECK_FALSE(a->Equals(*b));
    // GetHashCode: name hash ^ float bits ^ bool 0/1 ^ double hash
    PCM c = MakeKerbal("Hash Kerman");
    c->courage = 0.5f;
    c->stupidity = 0.0f;
    c->isBadass = true;
    c->UTaR = 0.0;
    const int32_t fh = 0x3F000000;  // 0.5f bits; stupidity 0 -> 0
    CHECK(c->GetHashCode() == (StringExtensions::GetHashCode_Net35("Hash Kerman") ^ 0 ^ fh ^ 1 ^ 0 ^ 0 ^ 0));
}

TEST_CASE("WP-31 g-force: GToleranceMult, MaxSustainedG, ActiveFixedUpdate (LOC, warning, recovery)") {
    World31 w;
    auto& adv = w.game.Parameters->CustomParams<GameParameters::AdvancedParams>();
    PCM k = MakeKerbal("Gee Kerman");
    ExperienceTraitConfig* pilot = w.Cfg().GetExperienceTraitConfig("Pilot");
    k->experienceTrait = ExperienceTrait::Create("Pilot", pilot, k.get());
    k->experienceLevel = 2;  // GeeForceTolerance modifiers 1.3 1.45 1.6 ...: level 2 -> 1.45
    k->courage = 0.5f;
    k->isBadass = true;
    const double brave = PhysicsGlobals::KerbalGBraveMult();
    const double expected = (1.0 + (brave - 1.0) * double(0.5f)) * double(1.45f) * PhysicsGlobals::KerbalGBadMult();
    CHECK(ProtoCrewMember::GToleranceMult(*k) == expected);
    CHECK(ProtoCrewMember::MaxSustainedG(*k) == dotnet::Math::Pow(PhysicsGlobals::KerbalGOffset() * expected, 1.0 / PhysicsGlobals::KerbalGPower()));
    // ActiveFixedUpdate needs the part's vessel
    Vessel v;
    auto part = w.g.New("mk1pod.v2");
    part->vessel = &v;
    v.parts.push_back(part.get());
    std::vector<std::string> msgs;
    ProtoCrewMember::PostScreenMessage = [&](const std::string& t, float) { msgs.push_back(t); };
    // GKerbalLimits off: gExperienced resets to 0
    adv.GKerbalLimits = false;
    k->gExperienced = 5.0;
    v.geeForce = 3.0;
    k->ActiveFixedUpdate(*part);
    CHECK(k->geeForce == 3.0);
    CHECK(k->gExperienced == 0.0);
    // GKerbalLimits on, stock calculation: a pulled gIncrement accumulates; with doStockGCalcs the increment carries the random
    // 1 +- 5 % factor (UnityEngine.Random.Range(0, 1) * 0.1 - 0.05)
    adv.GKerbalLimits = true;
    adv.KerbalGToleranceMult = 1.0f;
    UnityRandom::Global().InitState(77);
    UnityRandom shadow(77);
    k->courage = 0.0f;
    k->isBadass = false;
    v.geeForce = PhysicsGlobals::KerbalGClamp();
    const double dt = double(TimeWarp::fixedDeltaTime());
    const double num2 = ProtoCrewMember::GToleranceMult(*k) * double(adv.KerbalGToleranceMult);
    double inc = (dotnet::Math::Pow(dotnet::Math::Min(PhysicsGlobals::KerbalGClamp(), PhysicsGlobals::KerbalGClamp()), PhysicsGlobals::KerbalGPower()) - PhysicsGlobals::KerbalGOffset() * num2) * dt;
    REQUIRE(inc > 0.0);
    inc *= 1.0 + (double(shadow.Range(0.0f, 1.0f)) * 0.1 - 0.05);
    k->gExperienced = 0.0;
    k->ActiveFixedUpdate(*part);
    CHECK(k->gIncrement == inc);
    CHECK(k->gExperienced == inc);
    // a gentle load decays gExperienced: negative increment -> -(|inc|^decayPower), floor at 0
    v.geeForce = 0.0;
    k->gExperienced = 0.0;
    k->ActiveFixedUpdate(*part);
    CHECK(k->gExperienced == 0.0);
    // over the LOC threshold: the kerbal passes out (inactive, outDueToG) and the event fires once
    int passedOut = 0;
    struct PO {
        int* n;
        void F(ProtoCrewMember*) { ++*n; }
    } po{&passedOut};
    GE::onKerbalPassedOutFromGeeForce.Add(&po, &PO::F);
    ProtoCrewMember::doStockGCalcs = false;
    k->gExperienced = PhysicsGlobals::KerbalGThresholdLOC() * num2 + 1.0;
    FlightRegistry::ActiveVessel = &v;
    k->ActiveFixedUpdate(*part);
    CHECK(passedOut == 1);
    CHECK(k->outDueToG);
    CHECK(k->inactive());
    CHECK(k->inactiveTimeEnd == SaveUniversalTime() + PhysicsGlobals::KerbalGLOCBaseTime());
    REQUIRE_FALSE(msgs.empty());
    CHECK(msgs.back() == Localizer::Format("#autoLOC_143389", {"Gee Kerman"}));
    if (PhysicsGlobals::KerbalGClampGExperienced()) CHECK(k->gExperienced == PhysicsGlobals::KerbalGThresholdLOC() * num2);
    ProtoCrewMember::doStockGCalcs = true;
    GE::onKerbalPassedOutFromGeeForce.Remove(&po, &PO::F);
    // active callback hooks
    int updates = 0;
    ProtoCrewMember::CallbackActiveUpdate = [&](ProtoCrewMember&, Part&) { ++updates; };
    k->ActiveUpdate(*part);
    CHECK(updates == 1);
    ProtoCrewMember::CallbackActiveUpdate = nullptr;
    FlightRegistry::ActiveVessel = nullptr;
    ProtoCrewMember::PostScreenMessage = nullptr;
    part->vessel = nullptr;
    v.parts.clear();
}

// ===================================================== KerbalRoster =================================================

TEST_CASE("WP-31 the four original kerbals: GenerateInitialCrewRoster against the physprofile save (hero values are game constants)") {
    World31 w;
    ConfigNode::NodePtr save = ConfigNode::Load(kSaves31 + "/physprofile/persistent.sfs");
    REQUIRE(save);
    ConfigNode* roster = save->GetNode("GAME")->GetNode("ROSTER");
    REQUIRE(roster);
    auto initial = KerbalRoster::GenerateInitialCrewRoster(Game_Modes::SANDBOX);
    REQUIRE(initial->Count() == 4);
    for (int i = 0; i < 4; ++i) {
        ConfigNode* kn = roster->GetNode("KERBAL", i);
        REQUIRE(kn);
        PCM p = (*initial)[i];
        const std::string name = *kn->GetValue("name");
        CHECK(p->name().value_or("") == name);
        CHECK(p->trait == kn->GetValue("trait"));
        CHECK(p->gender() == (*kn->GetValue("gender") == "Female" ? ProtoCrewMember::Gender::Female : ProtoCrewMember::Gender::Male));
        CHECK(p->courage == dotnet::ParseSingle(*kn->GetValue("brave")));
        CHECK(p->stupidity == dotnet::ParseSingle(*kn->GetValue("dumb")));
        CHECK(p->isBadass == (*kn->GetValue("badS") == "True"));
        CHECK(p->veteran == (*kn->GetValue("veteran") == "True"));
        CHECK(p->isHero == (*kn->GetValue("hero") == "True"));
        CHECK(p->seatIdx == std::stoi(*kn->GetValue("idx")));
        CHECK(p->type() == ProtoCrewMember::KerbalType::Crew);
        // the trait object was created with the config's effects
        REQUIRE(p->experienceTrait);
        CHECK(p->experienceTrait->Effects().size() == w.Cfg().GetExperienceTraitConfig(*p->trait)->Effects().size());
    }
    // loading the save recomputes experience (never stored): 0 xp / level 0 for the empty logs, and the roster saves back identically
    Game g(save.get());
    REQUIRE(g.CrewRoster->Count() == 4);
    for (int i = 0; i < 4; ++i) {
        PCM p = (*g.CrewRoster)[i];
        CHECK(p->experience == 0.0f);
        CHECK(p->experienceLevel == 0);
        CHECK(p->careerLog.Count() == 0);
    }
}

TEST_CASE("WP-31 KerbalRoster: applicants, hire / sack / MIA / dead, GetNextOrNewKerbal, Update (applicant group), PCMUpdate") {
    World31 w;
    // name tables: synthetic (the game's are Unity TextAssets, see STATUS.md); last name "Kerman"
    CrewGenerator::SetNameTables(SyntheticNames('M', 60), SyntheticNames('F', 60), SyntheticNames('S', 3), SyntheticNames('T', 3), "Kerman");
    KerbalRoster& r = *w.game.CrewRoster;
    UnityRandom::Global().InitState(5);
    r.Update(1000.0);  // empty roster: num == 0 -> add 10 + Range(-3, 3) applicants
    UnityRandom shadow(5);
    const int b = 10 + shadow.Range(-3, 3);
    // some synthetic names may collide (GetRandomName avoids name substrings of existing kerbals) -> exactly b applicants
    CHECK(r.Applicants().size() == static_cast<size_t>(b));
    for (const PCM& ap : r.Applicants()) {
        CHECK(ap->type() == ProtoCrewMember::KerbalType::Applicant);
        CHECK(ap->UTaR > 1000.0);                   // UT + Range(1, 3) * 86400
        CHECK(ap->UTaR <= 1000.0 + 3.0 * 86400.0);
        CHECK(ap->rosterStatus() == ProtoCrewMember::RosterStatus::Available);
    }
    // expired applicants are removed and replaced up to the target
    const size_t before = r.Applicants().size();
    r.Update(1000.0 + 4.0 * 86400.0);
    CHECK(r.Applicants().size() >= static_cast<size_t>(7));
    CHECK(r.Applicants().size() <= static_cast<size_t>(13));
    (void)before;
    // hire: Applicant -> Crew with the OnCrewmemberHired event
    struct Hire {
        int hired = 0, sacked = 0, dead = 0;
        void H(ProtoCrewMember*, int) { hired++; }
        void S(ProtoCrewMember*, int) { sacked++; }
        void D(ProtoCrewMember*, int) { dead++; }
    } h;
    GE::OnCrewmemberHired.Add(&h, &Hire::H);
    GE::OnCrewmemberSacked.Add(&h, &Hire::S);
    GE::OnCrewmemberLeftForDead.Add(&h, &Hire::D);
    PCM ap = r.GetNextApplicant();
    REQUIRE(ap);
    r.HireApplicant(ap);
    CHECK(ap->type() == ProtoCrewMember::KerbalType::Crew);
    CHECK(h.hired == 1);
    r.HireApplicant(ap);  // not an applicant any more: logged, no event
    CHECK(h.hired == 1);
    r.SackAvailable(ap);
    CHECK(ap->type() == ProtoCrewMember::KerbalType::Applicant);
    CHECK(h.sacked == 1);
    r.HireApplicant(ap);
    ap->set_rosterStatus(ProtoCrewMember::RosterStatus::Missing);
    r.RemoveMIA(ap);
    CHECK(ap->rosterStatus() == ProtoCrewMember::RosterStatus::Dead);
    CHECK(h.dead == 1);
    CHECK(r.GetKIACrewCount() == 1);
    r.RemoveDead(ap);
    CHECK_FALSE(r.Exists(ap->name().value_or("")));
    GE::OnCrewmemberHired.Remove(&h, &Hire::H);
    GE::OnCrewmemberSacked.Remove(&h, &Hire::S);
    GE::OnCrewmemberLeftForDead.Remove(&h, &Hire::D);
    // GetNextOrNewKerbal: an available Crew kerbal first, else a new one (added to the roster)
    const int count0 = r.Count();
    PCM nk = r.GetNextOrNewKerbal();
    CHECK(nk->type() == ProtoCrewMember::KerbalType::Crew);
    CHECK(r.Count() == count0 + 1);
    CHECK(r.GetNextOrNewKerbal() == nk);
    // RepopulateCrewRoster: tops the AVAILABLE count up
    int avail = 0;
    for (int i = 0; i < r.Count(); ++i)
        if (r[i]->rosterStatus() == ProtoCrewMember::RosterStatus::Available) avail++;
    r.RepopulateCrewRoster(avail + 3);
    int avail2 = 0;
    for (int i = 0; i < r.Count(); ++i)
        if (r[i]->rosterStatus() == ProtoCrewMember::RosterStatus::Available) avail2++;
    CHECK(avail2 == avail + 3);
    // PCMUpdate calls Update(UT) of every kerbal
    int calls = 0;
    ProtoCrewMember::CallbackUpdate = [&](ProtoCrewMember&, double) { ++calls; };
    r.PCMUpdate(5.0);
    CHECK(calls == r.Count());
    ProtoCrewMember::CallbackUpdate = nullptr;
    // UpdateExperience: every kerbal
    r.UpdateExperience();
}

// =================================================== CrewGenerator ==================================================

TEST_CASE("WP-31 CrewGenerator: the name draw sequence, prototype attributes and the seeds, against an independent KSPRandom replay") {
    World31 w;
    const std::vector<std::string> male = {"Alan", "Bart", "Cal", "Dud", "Edz", "Fil", "Gus", "Hal"};
    const std::vector<std::string> female = {"Ada", "Bea", "Cyd", "Dot"};
    const std::vector<std::string> sMale = {"Spec", "Spud"};
    const std::vector<std::string> sFemale = {"Fspec"};
    CrewGenerator::SetNameTables(male, female, sMale, sFemale, "Kerman");
    w.game.CrewRoster = std::make_shared<KerbalRoster>(Game_Modes::CAREER);  // empty roster: no rejection
    SaveEnvironment::Current().currentGame = &w.game;
    for (int32_t seed : {1, 2, 3, 42, 1000, 123456789, -77, 2147483647}) {
        // independent replay of CrewGenerator.cs:37-95 (draw order: gender, special?, index)
        KSPRandom rng(seed);
        const bool female_ = rng.Next(2) > 0;
        std::string first;
        const auto& names = female_ ? female : male;
        const auto& special = female_ ? sFemale : sMale;
        if (rng.Next(20) != 0)
            first = names[static_cast<size_t>(rng.Next(static_cast<int32_t>(names.size())))];
        else
            first = special[static_cast<size_t>(rng.Next(static_cast<int32_t>(special.size())))];
        const std::string full = first + " Kerman";
        KSPRandom attr(StringExtensions::GetHashCode_Net35(full));
        const float courage = static_cast<float>(attr.NextDouble());
        const float stupidity = static_cast<float>(attr.NextDouble());
        const bool badass = attr.Next(10) == 0;
        PCM p = CrewGenerator::RandomCrewMemberPrototypeSeeded(seed, ProtoCrewMember::KerbalType::Applicant);
        CHECK(p->name().value_or("") == full);
        CHECK(p->gender() == (female_ ? ProtoCrewMember::Gender::Female : ProtoCrewMember::Gender::Male));
        CHECK(p->courage == courage);
        CHECK(p->stupidity == stupidity);
        CHECK(p->isBadass == badass);
        CHECK(p->hasToured == false);
        CHECK(p->type() == ProtoCrewMember::KerbalType::Applicant);
        CHECK(p->rosterStatus() == ProtoCrewMember::RosterStatus::Available);
        // SetExperienceTrait: |hash| % 3 of the stock non-tourist traits
        const int32_t hash = StringExtensions::GetHashCode_Net35(full);
        const char* traits[3] = {"Pilot", "Engineer", "Scientist"};
        CHECK(p->trait == std::string(traits[std::abs(hash) % 3]));
        REQUIRE(p->experienceTrait);
        // the Environment.TickCount ^ Guid hash seeding of RandomCrewMemberPrototype(): the two hooks
        SaveEnvironment::Current().tickCount = [] { return 0x1234; };
        SaveEnvironment::Current().newGuidHashCode = [seed] { return seed ^ 0x1234; };
        PCM q = CrewGenerator::RandomCrewMemberPrototype(ProtoCrewMember::KerbalType::Applicant);
        CHECK(q->name().value_or("") == full);
    }
    // an existing roster member whose name is a substring of the draw rejects it (the loop draws again)
    KSPRandom probe(9);
    const bool f9 = probe.Next(2) > 0;
    KSPRandom probe2(9);
    probe2.Next(2);
    const bool special9 = probe2.Next(20) == 0;
    const std::string first9 = special9 ? (f9 ? sFemale : sMale)[static_cast<size_t>(probe2.Next(f9 ? 1 : 2))]
                                        : (f9 ? female : male)[static_cast<size_t>(probe2.Next(f9 ? 4 : 8))];
    auto blocker = MakeKerbal(first9 + " Kerman");
    REQUIRE(w.game.CrewRoster->AddCrewMember(blocker));
    PCM p9 = CrewGenerator::RandomCrewMemberPrototypeSeeded(9, ProtoCrewMember::KerbalType::Crew);
    CHECK(p9->name().value_or("").find(first9) == std::string::npos);
    // GetRandomName(gender, null) seeds a new KSPRandom with Environment.TickCount
    SaveEnvironment::Current().tickCount = [] { return 31337; };
    const std::string viaNull = CrewGenerator::GetRandomName(ProtoCrewMember::Gender::Male, static_cast<KSPRandom*>(nullptr));
    KSPRandom rng(31337);
    const bool special = rng.Next(20) == 0;
    const std::string expectedFirst = special ? sMale[static_cast<size_t>(rng.Next(2))] : male[static_cast<size_t>(rng.Next(8))];
    CHECK((viaNull == expectedFirst + " Kerman" || w.game.CrewRoster->Exists(expectedFirst + " Kerman")));
    // a System.Random generator is not a KSPRandom: its own Next is used
    dotnet::SystemRandom sys(5);
    dotnet::SystemRandom sys2(5);
    const bool sp = sys2.Next(20) == 0;
    const std::string sysFirst = sp ? sMale[static_cast<size_t>(sys2.Next(2))] : male[static_cast<size_t>(sys2.Next(8))];
    w.game.CrewRoster->Remove(first9 + " Kerman");
    const std::string viaSys = CrewGenerator::GetRandomName(ProtoCrewMember::Gender::Male, sys);
    CHECK(viaSys == sysFirst + " Kerman");
    // GetFullName ignores its lastName argument (uses the static crewLastName), "ja" has no space
    CHECK(CrewGenerator::GetFullName("Abc", "ignored") == "Abc Kerman");
    CrewGenerator::languageIdFromFile = [] { return std::string("ja"); };
    CHECK(CrewGenerator::GetFullName("Abc", "x") == "AbcKerman");
    CrewGenerator::languageIdFromFile = nullptr;
    // RemoveLastName strips " Kerman"
    CHECK(CrewGenerator::RemoveLastName("Jebediah Kerman") == "Jebediah");
    // without Initialize / tables: NullReferenceException
    CrewGenerator::Reset();
    CHECK_THROWS_AS(CrewGenerator::GetRandomName(ProtoCrewMember::Gender::Male, &rng), NullReferenceException);
}

TEST_CASE("WP-31 CrewGenerator.Initialize reads the four TextAssets (ConfigNode text with Element values)") {
    World31 w;
    std::vector<std::string> requested;
    CrewGenerator::loadTextAsset = [&](const std::string& path) -> std::optional<std::string> {
        requested.push_back(path);
        const bool female = path.find("Female") != std::string::npos;
        const bool special = path.find("Special") != std::string::npos;
        const std::string prefix = special ? "KerbalSpecialNames" : "KerbalNames";
        std::string body = prefix + (female ? "Female" : "Male") + "\n{\n";
        body += female ? "Element = Fa\nElement = Fb\n" : "Element = Ma\nElement = Mb\nElement = Mc\n";
        body += "}\n";
        return body;
    };
    CrewGenerator::Initialize();
    REQUIRE(requested.size() == 4);
    CHECK(requested[0] == "TextAssets/CrewGenerator/en-us/KerbalNamesMale");
    CHECK(requested[1] == "TextAssets/CrewGenerator/en-us/KerbalNamesFemale");
    CHECK(requested[2] == "TextAssets/CrewGenerator/en-us/KerbalSpecialNamesMale");
    CHECK(requested[3] == "TextAssets/CrewGenerator/en-us/KerbalSpecialNamesFemale");
    REQUIRE(CrewGenerator::KerbalNamesMale().has_value());
    CHECK(*CrewGenerator::KerbalNamesMale() == std::vector<std::string>{"Ma", "Mb", "Mc"});
    CHECK(*CrewGenerator::KerbalNamesFemale() == std::vector<std::string>{"Fa", "Fb"});
    CHECK(CrewGenerator::GetFullName("Ma", "") == "Ma " + Dict("#autoLoc_6003018"));  // crewLastName = Localizer.Format("#autoLoc_6003018")
    CrewGenerator::loadTextAsset = nullptr;
    CHECK_THROWS_AS(CrewGenerator::Initialize(), NullReferenceException);
}

// ============================================ manifests over the stock crafts =======================================

namespace {
std::vector<fs::path> StockCrafts() {
    std::vector<fs::path> out;
    const fs::path ships = fs::path(kHarness31) / "inputs" / "Ships";
    if (fs::exists(ships))
        for (const auto& e : fs::recursive_directory_iterator(ships))
            if (e.is_regular_file() && e.path().extension() == ".craft") out.push_back(e.path());
    std::sort(out.begin(), out.end());
    return out;
}
}  // namespace

TEST_CASE("WP-31 VesselCrewManifest.FromConfigNode over every stock craft: part manifests, capacities, crew assignment") {
    World31 w;
    const std::vector<fs::path> crafts = StockCrafts();
    REQUIRE(crafts.size() >= 80);
    SaveEnvironment::Current().loadedScene = GameScenes::EDITOR;
    int withCrew = 0, crewableTotal = 0, partsTotal = 0;
    for (const fs::path& path : crafts) {
        ConfigNode::NodePtr craft = ConfigNode::Load(path.string());
        REQUIRE(craft);
        auto vcm = VesselCrewManifest::FromConfigNode(*craft);
        // oracle: the PART nodes whose part exists, each with a manifest of the prefab's CrewCapacity seats and PartID = the craftID
        size_t known = 0;
        std::vector<std::pair<uint32_t, int>> expected;
        for (ConfigNode* p : craft->GetNodes("PART")) {
            const std::string part = *p->GetValue("part");
            const size_t us = part.find('_');
            const AvailablePart* info = w.g.data.parts.getPartInfoByName(part.substr(0, us));
            if (info == nullptr) continue;
            known++;
            expected.emplace_back(static_cast<uint32_t>(std::stoul(part.substr(us + 1))), info->partPrefab->fields.CrewCapacity);
        }
        // duplicate craftIDs replace each other (SetPartManifest on an existing id): compare the last one per id
        std::map<uint32_t, int> byId;
        for (auto [id, cap] : expected) byId[id] = cap;
        partsTotal += static_cast<int>(known);
        CHECK_MESSAGE(vcm->PartCount() == static_cast<int>(byId.size()), path.string());
        CHECK(vcm->PartManifests().size() == byId.size());
        int crewable = 0;
        for (auto& pm : vcm->PartManifests()) {
            REQUIRE(byId.count(pm->PartID()) == 1);
            CHECK(static_cast<int>(pm->partCrew.size()) == byId[pm->PartID()]);
            for (const std::string& s : pm->partCrew) CHECK(s.empty());
            CHECK(pm->vcm == vcm.get());
            if (!pm->partCrew.empty()) crewable++;
        }
        CHECK(vcm->GetCrewableParts().size() == static_cast<size_t>(crewable));
        crewableTotal += crewable;
        CHECK(vcm->CrewCount() == 0);
        CHECK_FALSE(vcm->HasAnyCrew(*w.game.CrewRoster));
    }
    CHECK(partsTotal > 1000);
    CHECK(crewableTotal > 50);
    // assign crew to one crewed craft: seat bookkeeping, crew lookup and the cloned / merged manifests
    KerbalRoster& roster = *w.game.CrewRoster;
    PCM a = MakeKerbal("Seat A Kerman"), b = MakeKerbal("Seat B Kerman"), c = MakeKerbal("Seat C Kerman");
    for (const PCM& k : {a, b, c}) REQUIRE(roster.AddCrewMember(k));
    ConfigNode::NodePtr craft;
    std::shared_ptr<VesselCrewManifest> vcm;
    for (const fs::path& path : crafts) {
        craft = ConfigNode::Load(path.string());
        vcm = VesselCrewManifest::FromConfigNode(*craft);
        if (vcm->GetCrewableParts().size() >= 2) {
            int seats = 0;
            for (auto& p : vcm->GetCrewableParts()) seats += static_cast<int>(p->partCrew.size());
            if (seats >= 3) break;
        }
        vcm.reset();
    }
    REQUIRE(vcm);
    auto parts = vcm->GetCrewableParts();
    PartCrewManifest& p0 = *parts[0];
    PartCrewManifest& p1 = *parts[1];
    int editorEvents = 0;
    struct Ed {
        int* n;
        void F(VesselCrewManifest*) { ++*n; }
    } ed{&editorEvents};
    GE::onEditorShipCrewModified.Add(&ed, &Ed::F);
    p0.AddCrewToSeat(a, 0);
    CHECK(editorEvents == 1);  // fired in the editor scene
    CHECK(vcm->CrewCount() == 1);
    CHECK(vcm->CrewCountOptimizedForFloat() == 1.0f);
    CHECK(vcm->Contains(a));
    CHECK(p0.Contains(a));
    CHECK(p0.GetCrewSeat(*a) == 0);
    CHECK(vcm->GetPartForCrew(*a).get() == &p0);
    CHECK(p0.AnySeatTaken(roster));
    CHECK_FALSE(p0.AllSeatsEmpty(roster));
    CHECK(p0.CountCrewType(ProtoCrewMember::KerbalType::Crew, roster) == 1);
    CHECK(p0.CountCrewNotType(ProtoCrewMember::KerbalType::Crew, roster) == 0);
    // moving a kerbal to another part removes it from the first (SetCrewMember -> RemoveCrew)
    p1.AddCrewToSeat(a, 0);
    CHECK(p0.GetCrewSeat(*a) == -1);
    CHECK(p1.GetCrewSeat(*a) == 0);
    CHECK(vcm->CrewCount() == 1);
    // GetPartCrew / GetAllCrew
    CHECK(p1.GetPartCrew().size() == p1.partCrew.size());
    CHECK(p1.GetPartCrew()[0] == a);
    std::vector<PCM> allCrew = vcm->GetAllCrew(false, roster);
    REQUIRE(allCrew.size() == 1);
    CHECK(allCrew[0] == a);
    CHECK(vcm->GetAllCrew(true, roster).size() == [&] {
        size_t n = 0;
        for (auto& pm : vcm->PartManifests()) n += pm->partCrew.size();
        return n;
    }());
    // state queries
    CHECK(vcm->HasAnyCrew(roster));
    a->set_rosterStatus(ProtoCrewMember::RosterStatus::Assigned);
    CHECK(vcm->AnyCrewInState(ProtoCrewMember::RosterStatus::Assigned, roster));
    CHECK_FALSE(vcm->AnyCrewInState(ProtoCrewMember::RosterStatus::Dead, roster));
    CHECK(vcm->AnyCrewInState(ProtoCrewMember::RosterStatus::Dead, roster, true));  // notInState
    a->trait = "Pilot";
    CHECK(vcm->AnyCrewWithTrait("Pilot", roster));
    // sic: with no match the loop falls through to `return !noCrewWithTrait` -- true for the default, false when noCrewWithTrait is set
    CHECK(vcm->AnyCrewWithTrait("Engineer", roster));
    CHECK_FALSE(vcm->AnyCrewWithTrait("Pilot", roster, true));
    CHECK_FALSE(vcm->AnyCrewWithTrait("Engineer", roster, true));
    // an unknown name in a seat is dropped by HasAnyCrew / AnyCrewInState (RemoveCrewFromSeat)
    p0.partCrew[0] = "Ghost Kerman";
    CHECK(vcm->AnyCrewInState(ProtoCrewMember::RosterStatus::Dead, roster) == false);
    CHECK(p0.partCrew[0].empty());
    // RemoveCrewFromSeat
    p1.RemoveCrewFromSeat(0);
    CHECK(vcm->CrewCount() == 0);
    // clone (blank / full) and merge
    p0.AddCrewToSeat(b, 0);
    auto full = VesselCrewManifest::CloneOf(*vcm, false);
    auto blank = VesselCrewManifest::CloneOf(*vcm, true);
    // CloneOf(blank = false) calls SetCrewMember(partCrew[i], ...) for EVERY seat, so the empty seat name "" becomes a crew key too (sic)
    CHECK(full->Contains(b));
    CHECK(full->GetPartForCrew(*b)->PartID() == p0.PartID());
    CHECK(blank->CrewCount() == 0);
    CHECK(full->PartCount() == vcm->PartCount());
    // MergeInto with no filter does nothing (sic)
    VesselCrewManifest::MergeInto(*blank, *full);
    CHECK(blank->CrewCount() == 0);
    VesselCrewManifest::MergeInto(*blank, *full, [](PartCrewManifest&) { return true; });
    CHECK(blank->CrewCount() == 1);
    // out-of-range seat: the capacity bound is inclusive (sic), so seat == capacity reaches the array
    CHECK_THROWS_AS(p0.AddCrewToSeat(c, static_cast<int>(p0.partCrew.size())), std::out_of_range);
    CHECK_THROWS_AS(p0.AddCrewToSeat(c, 99), std::out_of_range);
    // crew inventory costs / mass / resource mass: 0 for tourists, the hooks otherwise
    ProtoCrewMember::kerbalModuleMass = [](ProtoCrewMember&) { return 0.5f; };
    CHECK(vcm->GetCrewInventoryMass(&roster) == 0.5f);
    ProtoCrewMember::kerbalModuleMass = nullptr;
    CHECK(vcm->GetCrewInventoryMass(&roster) == 0.0f);
    GE::onEditorShipCrewModified.Remove(&ed, &Ed::F);
    SaveEnvironment::Current().loadedScene = GameScenes::MAINMENU;
}

TEST_CASE("WP-31 KerbalRoster.DefaultCrewForVessel: pilot first, auto-hire, inactive kerbals skipped, previous manifests") {
    World31 w;
    KerbalRoster& roster = *w.game.CrewRoster;
    CrewGenerator::SetNameTables(SyntheticNames('M', 60), SyntheticNames('F', 60), SyntheticNames('S', 3), SyntheticNames('T', 3), "Kerman");
    // a craft whose first control-source part is the Mk1 pod (1 seat) and one with a 3-seat Mk1-3 pod
    ConfigNode::NodePtr one = ConfigNode::Parse("craft\n{\nPART\n{\npart = mk1pod.v2_4294767000\n}\n}\n");
    ConfigNode::NodePtr three = ConfigNode::Parse("craft\n{\nPART\n{\npart = mk1-3pod_4294767000\n}\n}\n");
    const AvailablePart* mk13 = w.g.data.parts.getPartInfoByName("mk1-3pod");
    const AvailablePart* mk1 = w.g.data.parts.getPartInfoByName("mk1pod.v2");
    REQUIRE(mk13);
    REQUIRE(mk1);
    // ModuleCommand.OnAwake (ModuleCommand.cs:378) sets part.isControlSource = FULL on the prefab; ModuleCommand is not ported yet, so
    // the test applies that one assignment itself
    mk13->partPrefab->fields.isControlSource = static_cast<int>(ControlLevel::FULL);
    mk1->partPrefab->fields.isControlSource = static_cast<int>(ControlLevel::FULL);
    REQUIRE(mk13->partPrefab->fields.CrewCapacity == 3);
    PCM eng = MakeKerbal("Eng Kerman"), pilot = MakeKerbal("Pil Kerman"), sci = MakeKerbal("Sci Kerman");
    eng->trait = "Engineer";
    pilot->trait = "Pilot";
    sci->trait = "Scientist";
    for (const PCM& k : {eng, pilot, sci}) {
        REQUIRE(roster.AddCrewMember(k));
        KerbalRoster::SetExperienceTrait(*k);
    }
    // 1 seat, no previous manifest: the first Pilot is chosen
    auto m1 = roster.DefaultCrewForVessel(*one->GetNode("craft"));
    REQUIRE(m1->PartManifests().size() == 1);
    CHECK(m1->PartManifests()[0]->partCrew[0] == "Pil Kerman");
    // 3 seats: pilot in seat 0, then the available kerbals in roster order
    auto m3 = roster.DefaultCrewForVessel(*three->GetNode("craft"));
    auto& pc = m3->PartManifests()[0]->partCrew;
    REQUIRE(pc.size() == 3);
    CHECK(pc[0] == "Pil Kerman");
    CHECK(pc[1] == "Eng Kerman");
    CHECK(pc[2] == "Sci Kerman");
    // an inactive kerbal is skipped
    eng->set_inactive(true);
    auto m3b = roster.DefaultCrewForVessel(*three->GetNode("craft"));
    auto& pcb = m3b->PartManifests()[0]->partCrew;
    CHECK(pcb[0] == "Pil Kerman");
    CHECK(pcb[1] == "Sci Kerman");
    CHECK(pcb[2].empty());  // the inactive engineer is not used and no more kerbals exist (auto-hire off: not enough funds hooks)
    eng->set_inactive(false);
    // usePreviousVCMToFill takes the seats of the previous manifest
    std::vector<std::string> msgs;
    CrewAssignmentEnvironment::postScreenMessage = [&](const std::string& t, float) { msgs.push_back(t); };
    auto prev = VesselCrewManifest::FromConfigNode(*three->GetNode("craft"));
    prev->PartManifests()[0]->AddCrewToSeat(sci, 1);
    auto m3c = roster.DefaultCrewForVessel(*three->GetNode("craft"), prev.get(), true, true);
    CHECK(m3c->PartManifests()[0]->partCrew[1] == "Sci Kerman");
    // auto-hire: only when the difficulty option is on and the astronaut complex allows it
    w.game.Parameters->Difficulty.AutoHireCrews = true;
    CrewAssignmentEnvironment::activeCrewLimit = [] { return 20; };
    CrewAssignmentEnvironment::recruitHireCost = [](int n) { return 2000.0f + float(n); };
    CrewAssignmentEnvironment::canAfford = [](float c) { return c < 100000.0f; };
    CrewAssignmentEnvironment::fundingPresent = [] { return true; };
    UnityRandom::Global().InitState(3);
    PCM lone = MakeKerbal("Only Kerman");
    KerbalRoster r2(Game_Modes::CAREER);
    w.game.CrewRoster = std::make_shared<KerbalRoster>(Game_Modes::CAREER);
    SaveEnvironment::Current().currentGame = &w.game;
    REQUIRE(w.game.CrewRoster->AddCrewMember(lone));
    auto mh = w.game.CrewRoster->DefaultCrewForVessel(*three->GetNode("craft"));
    int filled = 0;
    for (auto& s : mh->PartManifests()[0]->partCrew) filled += s.empty() ? 0 : 1;
    CHECK(filled == 3);  // two applicants were hired
    CHECK(w.game.CrewRoster->GetActiveCrewCount() == 3);
    CHECK(msgs.size() >= 2);
    CrewAssignmentEnvironment::activeCrewLimit = nullptr;
    CrewAssignmentEnvironment::recruitHireCost = nullptr;
    CrewAssignmentEnvironment::canAfford = nullptr;
    CrewAssignmentEnvironment::fundingPresent = nullptr;
    CrewAssignmentEnvironment::postScreenMessage = nullptr;
}

// ============================================ ModuleExperienceManagement ===========================================

TEST_CASE("WP-31 ModuleExperienceManagement: OnStart enable rules and FinalizeExperience (archive, funds, CommNet)") {
    World31 w;
    // the stock part that carries the module (GameData/Squad/Parts/Science/LargeCrewedLab/largeCrewedLab.cfg: costPerKerbal = 0)
    auto part = w.g.New("Large.Crewed.Lab");
    PartModule* m = part->FindModuleOfClass("ModuleExperienceManagement");
    REQUIRE(m);
    auto* mem = dynamic_cast<ModuleExperienceManagement*>(m);
    REQUIRE(mem);
    CHECK(mem->costPerKerbal() == 0.0f);
    CHECK(mem->GetInfo() == Dict("#autoLOC_230278"));
    CHECK(mem->GetModuleDisplayName() == Dict("#autoLoc_6003041"));
    auto& adv = w.game.Parameters->CustomParams<GameParameters::AdvancedParams>();
    // enabled when experience is on, ImmediateLevelUp off, not a mission
    mem->OnStart(StartState::Editor);
    CHECK(mem->enabled);
    adv.ImmediateLevelUp = true;
    mem->OnStart(StartState::Editor);
    CHECK_FALSE(mem->enabled);
    adv.ImmediateLevelUp = false;
    mem->enabled = true;
    adv.SetEnableKerbalExperience(false);
    mem->OnStart(StartState::Editor);
    CHECK_FALSE(mem->enabled);
    adv.SetEnableKerbalExperience(true);
    mem->enabled = true;
    w.game.Mode = Game_Modes::MISSION;
    mem->OnStart(StartState::Editor);
    CHECK_FALSE(mem->enabled);
    w.game.Mode = Game_Modes::CAREER;
    // FinalizeExperience: a vessel with two kerbals, one with unarchived experience
    Vessel v;
    part->vessel = &v;
    v.parts.push_back(part.get());
    PCM rookie = MakeKerbal("Rookie Kerman"), vet = MakeKerbal("Vet Kerman");
    KerbalRoster::SetExperienceTrait(*vet, "Pilot");
    KerbalRoster::SetExperienceTrait(*rookie, "Pilot");
    const float rvMun = w.Body("Mun").scienceValues.RecoveryValue;
    vet->flightLog.AddEntry("Land", std::string("Mun"));
    vet->flightLog.AddEntry("PlantFlag", std::string("Mun"));
    v.loaded = true;
    part->protoModuleCrew = {rookie, vet};
    v.RebuildCrewList();
    REQUIRE(v.GetVesselCrew().size() == 2);
    std::vector<std::string> msgs;
    ModuleExperienceManagement::postScreenMessage = [&](const std::string& t, float) { msgs.push_back(t); };
    // CommNet enabled and no signal: refused
    ModuleExperienceManagement::commNetHasNoSignal = [](Vessel&) { return true; };
    mem->FinalizeExperience();
    REQUIRE(msgs.size() == 1);
    CHECK(msgs[0] == Dict("#autoLOC_230288"));
    ModuleExperienceManagement::commNetHasNoSignal = nullptr;
    msgs.clear();
    mem->FinalizeExperience();
    // the vet's flight log was archived into the career log; the rookie had nothing new
    CHECK(vet->flightLog.Count() == 0);
    CHECK(vet->careerLog.Count() == 2);
    CHECK(vet->experience == rvMun * 2.5f);
    REQUIRE_FALSE(msgs.empty());
    ModuleExperienceManagement::postScreenMessage = nullptr;
    part->vessel = nullptr;
    v.parts.clear();
}


// ================================================== data gate ======================================================

namespace {
// Independent XP oracle over the RAW CAREER_LOG / FLIGHT_LOG nodes of a KERBAL (not the FlightLog class): KerbalRoster.cs:1126-1372
//   per log (last to first), per flight (in order): the flight counts only when its last entry is not "Die" (otherwise everything gathered so
//   far is dropped); per target body the maximum of home / away value x RecoveryValue; the sum over targets in first-seen order.
struct RawEntry {
    int flight;
    std::string type, target;
};
std::vector<RawEntry> RawLog(const ConfigNode* log) {
    std::vector<RawEntry> out;
    if (log == nullptr) return out;
    for (int i = 0; i < log->values().Count(); ++i) {
        const auto& v = log->values()[i];
        if (v.name == "flight") continue;
        const size_t us = v.name.find('_');  // "<flight>_<index>" in KSP 1.12.5 saves
        RawEntry e;
        e.flight = us == std::string::npos ? 0 : std::stoi(v.name.substr(0, us));
        const size_t comma = v.value.find(',');
        e.type = v.value.substr(0, comma);
        e.target = comma == std::string::npos ? "" : v.value.substr(comma + 1);
        out.push_back(e);
    }
    return out;
}
float OracleXp(const std::vector<std::vector<RawEntry>>& logs, World31& w) {
    struct V {
        float home, away;
    };
    const std::map<std::string, V> table = {{"Training1", {2.0f, 2.0f}}, {"Training2", {8.0f, 8.0f}}, {"Training3", {16.0f, 16.0f}},
                                            {"Training4", {32.0f, 32.0f}}, {"Training5", {64.0f, 64.0f}}, {"PlantFlag", {0.0f, 2.5f}},
                                            {"Land", {0.0f, 2.3f}}, {"Flight", {1.0f, 2.0f}}, {"Suborbit", {1.0f, 0.0f}},
                                            {"Flyby", {1.5f, 1.0f}}, {"Escape", {0.0f, 1.0f}}, {"Orbit", {2.0f, 1.5f}}};
    std::vector<std::string> targets;
    std::vector<float> xp;
    for (int l = static_cast<int>(logs.size()) - 1; l >= 0; --l) {
        std::map<int, std::vector<RawEntry>> flights;
        for (const RawEntry& e : logs[static_cast<size_t>(l)]) flights[e.flight].push_back(e);
        for (auto& [fl, entries] : flights) {
            if (entries.empty() || entries.back().type == "Die") {
                targets.clear();
                xp.clear();
                continue;
            }
            for (const RawEntry& e : entries) {
                if (e.target.empty()) continue;
                CelestialBody* body = nullptr;
                for (CelestialBody* b : SaveEnvironment::Current().localBodies)
                    if (b->name() == e.target || b->displayName() == e.target) body = b;
                auto it = table.find(e.type);
                if (body == nullptr || it == table.end()) continue;
                const float mult = body->isHomeWorld ? it->second.home : it->second.away;
                if (mult == 0.0f) continue;
                const float value = body->scienceValues.RecoveryValue * mult;
                auto pos = std::find(targets.begin(), targets.end(), e.target);
                if (pos == targets.end()) {
                    targets.push_back(e.target);
                    xp.push_back(value);
                } else if (xp[static_cast<size_t>(pos - targets.begin())] < value) {
                    xp[static_cast<size_t>(pos - targets.begin())] = value;
                }
            }
        }
    }
    float sum = 0.0f;
    for (float v : xp) sum += v;
    return sum;
}
}  // namespace

TEST_CASE("WP-31 data gate: every KERBAL of every save in testdata and harness/inputs, experience recomputed from its flight logs") {
    World31 w;
    std::vector<fs::path> saves;
    for (const std::string& root : {std::string(KSPCORE_TESTDATA_DIR) + "/saves", kHarness31 + "/inputs"})
        if (fs::exists(root))
            for (const auto& e : fs::recursive_directory_iterator(root))
                if (e.is_regular_file() && e.path().extension() == ".sfs") saves.push_back(e.path());
    std::sort(saves.begin(), saves.end());
    REQUIRE_FALSE(saves.empty());
    int kerbals = 0, mismatches = 0, storedFields = 0;
    for (const fs::path& path : saves) {
        ConfigNode::NodePtr root = ConfigNode::Load(path.string());
        REQUIRE(root);
        ConfigNode* gameNode = root->GetNode("GAME");
        REQUIRE(gameNode);
        ConfigNode* rosterNode = gameNode->GetNode("ROSTER");
        if (rosterNode == nullptr) continue;
        // the raw logs, before FlightLog.Load removes `flight` from the tree
        struct Raw {
            std::string name;
            std::vector<std::vector<RawEntry>> logs;
            bool hasStored;
        };
        std::vector<Raw> raws;
        for (ConfigNode* k : rosterNode->GetNodes("KERBAL")) {
            Raw r;
            r.name = k->GetValue("name").value_or("");
            r.logs = {RawLog(k->GetNode("CAREER_LOG")), RawLog(k->GetNode("FLIGHT_LOG"))};
            r.hasStored = k->HasValue("experience") || k->HasValue("experienceLevel");
            raws.push_back(r);
        }
        SaveEnvironment::Current().currentGame = nullptr;
        Game g(root.get());
        SaveEnvironment::Current().currentGame = &g;
        GE::onGameStateCreated.Fire(&g);  // KerbalRoster/ProtoCrewMember: experience is computed once the game exists
        REQUIRE(g.CrewRoster);
        REQUIRE(static_cast<size_t>(g.CrewRoster->Count()) == raws.size());
        auto& adv = g.Parameters->CustomParams<GameParameters::AdvancedParams>();
        for (const Raw& r : raws) {
            PCM p = (*g.CrewRoster)[r.name];
            REQUIRE_MESSAGE(p, r.name);
            kerbals++;
            float expected;
            if (!adv.KerbalExperienceEnabled(g.Mode))
                expected = 99999.0f;
            else if (adv.ImmediateLevelUp)
                expected = OracleXp({r.logs[0], r.logs[1]}, w);
            else
                expected = OracleXp({r.logs[0]}, w);
            const int level = KerbalRoster::CalculateExperienceLevel(expected);
            if (p->experience != expected || p->experienceLevel != level) mismatches++;
            CHECK_MESSAGE(p->experience == expected, path.string() << " " << r.name);
            CHECK_MESSAGE(p->experienceLevel == level, path.string() << " " << r.name);
            if (r.hasStored) storedFields++;
            // a trait object exists for every kerbal and its trait name is the saved one
            REQUIRE(p->experienceTrait);
        }
        // the ROSTER saves back identically (WP-04 round trip, now through the extended classes)
        ConfigNode out;
        g.Save(out);
        ConfigNode::NodePtr reload = ConfigNode::Load(path.string());
        CHECK(out.GetNode("GAME")->GetNode("ROSTER")->ToString() == reload->GetNode("GAME")->GetNode("ROSTER")->ToString());
        SaveEnvironment::Current().currentGame = &w.game;
    }
    MESSAGE("kerbals " << kerbals << ", mismatches " << mismatches << ", KERBAL nodes carrying a stored experience value: " << storedFields);
    CHECK(kerbals >= 4);
    CHECK(mismatches == 0);
}
