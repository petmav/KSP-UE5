// Test hygiene listener: see test_hygiene.hpp. Set KSP_HYGIENE_TRACE=1 to print what each test case left behind.
#include "test_hygiene.hpp"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/config/config_node.hpp"
#include "ksp/localization/localizer.hpp"
#include "ksp/celestial/celestial_system.hpp"
#include "ksp/contracts/templates.hpp"
#include "ksp/celestial/flight_globals.hpp"
#include "ksp/celestial/planetarium.hpp"
#include "ksp/physics/collision_enhancer.hpp"
#include "ksp/physics/flight_integrator.hpp"
#include "ksp/events/event_data.hpp"
#include "ksp/input/game_settings.hpp"
#include "ksp/input/input_lock_manager.hpp"
#include "ksp/inventory/construction.hpp"
#include "ksp/fineprint/compatibility.hpp"
#include "ksp/kerbals/experience.hpp"
#include "ksp/modules/part_module.hpp"
#include "ksp/science/science.hpp"
#include "ksp/save/save_environment.hpp"

namespace kspcore_tests {
namespace {

using namespace ksp;
namespace LC = ksp::Contracts::Lite;

struct PrefabBaseline {
    std::vector<Vector3d> positions;
    bool taken = false;
};

PrefabBaseline& Baseline() {
    static PrefabBaseline b;
    return b;
}

// Listeners that exist before the first test case (installed by static initialisers of the library) belong to the process
std::map<std::string, size_t>& EventBaseline() {
    static std::map<std::string, size_t> m;
    return m;
}

void TakeBaseline() {
    PrefabBaseline& b = Baseline();
    if (b.taken) return;
    for (const CelestialBodyPrefab& p : KerbolSystemPrefab().bodies) b.positions.push_back(p.body.position);
    b.taken = true;
    ksp::events::BaseGameEvent::ForEachRegistered([](ksp::events::BaseGameEvent& e) { EventBaseline()[e.EventName()] = e.ListenerCount(); });
}

bool Trace() {
    static const bool on = std::getenv("KSP_HYGIENE_TRACE") != nullptr;
    return on;
}

void Report(const char* what, const std::string& test) {
    if (Trace()) std::fprintf(stderr, "[hygiene] %s left by: %s\n", what, test.c_str());
}

// Compares the live state with the baseline; returns the number of differences found (and fixes them).
void Restore(const std::string& test) {
    TakeBaseline();
    const PrefabBaseline& b = Baseline();
    size_t i = 0;
    for (const CelestialBodyPrefab& p : KerbolSystemPrefab().bodies) {
        CelestialBody& body = const_cast<CelestialBody&>(p.body);
        if (i < b.positions.size() && !(body.position == b.positions[i])) {
            Report("KerbolSystemPrefab body position", test);
            body.position = b.positions[i];
        }
        ++i;
    }
    SaveEnvironment::Reset();
    // WP-20g: GameSettings at KSP's defaults in every case. KSP runs GameSettings.Awake (SetDefaultValues, GameSettings.cs:1529) once
    // when the game starts, so no code of the game sees the C# field initialisers (0 / false / null for most of the 447 fields); a host
    // does the same (GameSettings::Awake). Before this, the statics were 0 until a case ran SetDefaultValues (WP-50's InputFixture) and kept
    // whatever a case left: the WP-29c rover rode at 0.493 or 0.508 depending on the order, the anchor statics were 0 in the WP-20b / 19b /
    // 20d scenes. The input objects SetDefaultValues does not make (INPUT_DEVICES: GameSettings.Awake; the axis speed-multiplier list) and
    // the parsed keyboard layouts are reset to "not made yet", as WP-50's InputFixture does.
    GameSettings::SetDefaultValues();
    GameSettings::AXIS_INCREMENTAL_SPEED_MULTIPLIER_VALUES.reset();
    GameSettings::INPUT_DEVICES.reset();
    GameSettings::layoutConfigNodes.Clear();
    GameSettings::overrideSettings = false;
    GameSettings::saveOnGameSave = false;
    // EVAConstructionModeController::hooks is static and test lambdas capture their stack locals: a hook left behind is a
    // use-after-return in the next case that opens the construction panel (found as a g++ SIGSEGV in test_construction_flow.cpp)
    ksp::EVAConstructionModeController::hooks = ksp::EVAConstructionModeController::Hooks{};
    // AutopilotSkill / RepairSkill.SkillsReadable (C# static initialisers over Localizer.Format)
    ksp::Experience::Effects::ReadableSkillsCache::ResetForTests();
    // ResearchAndDevelopment.experiments (ResearchAndDevelopment.cs:21) is filled on first use from the GameDatabase of that moment:
    // a test that asked before its database was wired would leave an empty cache for the next one
    ResearchAndDevelopment::ResetStatics();
    KSP::Localization::Localizer::Shutdown();  // a test that formats #autoLOC tags loads the dictionary itself (LocalizerScope)
    // the per-module-class ReflectedAttributes.isStageable (PartModule.cs:25) is shared by every instance and written by
    // ModuleServiceModule / ModuleJettison.UpdateStaging and the decouplers (faithful to KSP, so a test must not leak it)
    PartModule::ResetReflectedAttributesForTests();
    // the contract template seams (FP::InstallAll() fills them, World38 only installs FP::Install(): a stale hook changes
    // what ExplorationContract unlocks)
    ksp::Contracts::Templates::TemplateEnvironment::Reset();
    if (FlightGlobals::ready || FlightGlobals::ActiveVessel != nullptr || !FlightGlobals::VesselsLoaded.empty() ||
        FlightGlobals::currentMainBody != nullptr || FlightGlobals::overrideOrbit) {
        Report("FlightGlobals state", test);
        FlightGlobals::ready = false;
        FlightGlobals::ActiveVessel = nullptr;
        FlightGlobals::VesselsLoaded.clear();
        FlightGlobals::currentMainBody = nullptr;
        FlightGlobals::overrideOrbit = false;
    }
    // FlightGlobals' navball speed modes (FlightGlobals.cs:128-130) follow the active vessel's altitude in every FlightGlobals.Update (a flight
    // scene with a vessel in orbit leaves Orbit; WP-19a's FlightGlobals tick case expects the Surface default); FlightIntegrator.ActiveVesselFI
    // (WP-20d) is a static pointer to a vessel module of the last scene
    if (FlightGlobals::speedDisplayMode() != FlightGlobals_SpeedDisplayModes::Surface) Report("FlightGlobals speed mode", test);
    FlightGlobals::ResetSpeedModesForTests();
    ksp::physics::FlightIntegrator::ActiveVesselFI = nullptr;
    // WP-20e: the collision statics (CheatOptions.NoCrashDamage, the FX / wheel-damage hooks, CollisionEnhancer.bypass)
    ksp::PartContactEnvironment::Reset();
    ksp::physics::CollisionEnhancer::bypass = false;
    // InputLockManager's lock stack and mask (InputLockManager.cs:8-10) are static: a lock a case leaves behind (WP-32's parachute-state walk
    // enters KerbalEVA states that set ControlPanelLock_<id> without their OnLeave) blocks VESSEL_SWITCHING and the ship controls in every
    // later case (WP-19a's VesselSwitching.Update returned nothing after it under --order-by=rand -rs=27)
    if (InputLockManager::lockMask != 0 || InputLockManager::lockStack.Count() != 0) {
        if (Trace()) std::fprintf(stderr, "[hygiene] InputLockManager locks %s left by: %s\n", InputLockManager::PrintLockStack().c_str(), test.c_str());
        InputLockManager::Reset();
    }
    // GameEvents / any named event: a listener holds a raw pointer to its target, so one left behind by a destroyed
    // object is a dangling call for every later test that fires the event (and a live one changes what it sees)
    ksp::events::BaseGameEvent::ForEachRegistered([&](ksp::events::BaseGameEvent& e) {
        const auto it = EventBaseline().find(e.EventName());
        const size_t keep = it == EventBaseline().end() ? 0 : it->second;
        if (e.ListenerCount() <= keep) return;
        if (Trace()) std::fprintf(stderr, "[hygiene] event %s: %zu listener(s) left by: %s\n", e.EventName().c_str(), e.ListenerCount() - keep, test.c_str());
        e.TruncateListenersForTests(keep);
    });
    // CompatibilityUtilities.majorVersion / minorVersion / revisionVersion (CompatibilityUtilities.cs:9-13) start at int.MaxValue
    // ("not an old save"); OldCareerSave overwrites them with the version of the save it was asked about
    if (LC::CompatibilityUtilities::majorVersion != 2147483647 || LC::CompatibilityUtilities::minorVersion != 2147483647 ||
        LC::CompatibilityUtilities::revisionVersion != 2147483647) {
        Report("CompatibilityUtilities version", test);
        LC::CompatibilityUtilities::majorVersion = LC::CompatibilityUtilities::minorVersion = LC::CompatibilityUtilities::revisionVersion = 2147483647;
    }
}

struct HygieneListener : doctest::IReporter {
    explicit HygieneListener(const doctest::ContextOptions&) {}
    std::string current;
    void report_query(const doctest::QueryData&) override {}
    void test_run_start() override { TakeBaseline(); }
    void test_run_end(const doctest::TestRunStats&) override {}
    void test_case_start(const doctest::TestCaseData& tc) override {
        current = tc.m_name;
        Restore("(before the test case) " + current);
    }
    void test_case_reenter(const doctest::TestCaseData&) override {}
    void test_case_end(const doctest::CurrentTestCaseStats&) override { Restore(current); }
    void test_case_exception(const doctest::TestCaseException&) override {}
    void subcase_start(const doctest::SubcaseSignature&) override {}
    void subcase_end() override {}
    void log_assert(const doctest::AssertData&) override {}
    void log_message(const doctest::MessageData&) override {}
    void test_case_skipped(const doctest::TestCaseData&) override {}
};

}  // namespace

void ResetSharedTestState() { Restore("(explicit)"); }

void LoadStockLocalizer() {
    KSP::Localization::Localizer::Shutdown();
    KSP::Localization::Localizer::Init();
    ConfigNode::NodePtr root = ConfigNode::Load(ksp::testing::GameRoot() + "/GameData/Squad/Localization/dictionary.cfg");
    REQUIRE(root);
    std::vector<ConfigNode*> nodes{root->GetNode("Localization")};
    KSP::Localization::Localizer::Instance()->AddTagValuesForLanguage(nodes, "en-us");
}

LocalizerScope::~LocalizerScope() { KSP::Localization::Localizer::Shutdown(); }

}  // namespace kspcore_tests

DOCTEST_REGISTER_LISTENER("hygiene", 1, kspcore_tests::HygieneListener);
