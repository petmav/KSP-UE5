// WP-UE-02: the game host behind the C ABI v2. Two things the pure-C ctest capi.flight_slice cannot say:
//   * the C++ path (GameHost driven directly) and the C ABI path (the extern "C" entry points) end a scripted PhysProf-M2 ascent in the
//     same snapshot hash, bit for bit; the hash the pure-C program wrote to a file (ctest capi.flight_slice) is the same too;
//   * kspcore_sim_destroy leaves nothing behind the unit-test hygiene checks (tests/test_hygiene.*) would flag, and a new sim can be made.
// The heavy case (the whole scripted ascent, three runs) runs only when KSPCORE_SLICE_TWIN is set (ctest capi.flight_slice_twin);
// the short case runs in every unit shard.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "game_root.hpp"
#include "ksp/host/game_host.hpp"
#include "kspcore/kspcore_c.h"
#include "test_hygiene.hpp"

namespace fs = std::filesystem;

namespace {

constexpr int kKeySpace = 32, kKeyT = 116, kKeyZ = 122;

struct Paths {
    std::string gameRoot = ksp::testing::GameRoot();
    std::string dataDir = KSPCORE_DATA_DIR;
    std::string reduced = KSPCORE_TEST_DATA_DIR;
    std::string craft = std::string(KSPCORE_PROJECT_ROOT) + "/analysis/physics/craft/PhysProf-M2.craft";
    std::string saves;
    Paths() {
        const fs::path p = fs::temp_directory_path() / "kspcore_wpue02_twin" / "saves";
        fs::create_directories(p);
        saves = p.generic_string();
    }
};

bool GameAvailable() { return fs::exists(fs::path(ksp::testing::GameRoot()) / "GameData" / "Squad"); }

// the scripted input of the ascent: frame k carries the keys of C10's launch at k == settle
struct Script {
    int settle = 100;
    kspcore_input_frame Frame(int k, int32_t* down, int32_t* held) const {
        kspcore_input_frame in{};
        in.struct_size = sizeof in;
        in.screen_width = 1280;
        in.screen_height = 720;
        in.has_focus = 1;
        in.real_time_since_startup = static_cast<float>(k) * 0.02f;
        if (k == settle) {
            down[0] = kKeyZ;
            down[1] = kKeyT;
            down[2] = kKeySpace;
            in.keys_down = down;
            in.keys_down_count = 3;
            held[0] = kKeyZ;
            in.keys_held = held;
            in.keys_held_count = 1;
        }
        return in;
    }
};

bool InitProcess(const Paths& p) {
    kspcore_game_config cfg{};
    cfg.struct_size = sizeof cfg;
    cfg.game_root = p.gameRoot.c_str();
    cfg.data_dir = p.dataDir.c_str();
    cfg.reduced_model_dir = p.reduced.c_str();
    cfg.saves_dir = p.saves.c_str();
    return kspcore_game_init(&cfg) == 1 && kspcore_game_load_database(nullptr, nullptr) == 1;
}

// ---- the C ABI path ---------------------------------------------------------------------------------------------------------------
uint64_t RunAbi(const Paths& p, const char* save, int frames, uint32_t* parts = nullptr, double* altitude = nullptr) {
    kspcore_sim* sim = kspcore_sim_create();
    REQUIRE(sim != nullptr);
    REQUIRE(kspcore_game_new(sim, KSPCORE_MODE_SANDBOX, save) == 1);
    REQUIRE(kspcore_game_launch_craft(sim, p.craft.c_str(), "LaunchPad") == 1);
    REQUIRE(kspcore_frame(sim, 0.02) == 1);
    REQUIRE(kspcore_scene_loaded(sim, KSPCORE_SCENE_FLIGHT) == 1);
    const Script script;
    int32_t down[3], held[1];
    for (int k = 0; k < frames; ++k) {
        const kspcore_input_frame in = script.Frame(k, down, held);
        REQUIRE(kspcore_input_set_frame(sim, &in) == 1);
        REQUIRE(kspcore_frame(sim, 0.02) == 1);
    }
    const kspcore_snapshot_view* s = kspcore_snapshot(sim);
    REQUIRE(s != nullptr);
    const uint64_t h = kspcore_snapshot_hash(s);
    if (parts != nullptr || altitude != nullptr)
        for (uint32_t i = 0; i < s->vessel_count; ++i)
            if ((s->vessels[i].flags & KSPCORE_VESSEL_ACTIVE) != 0) {
                if (parts != nullptr) *parts = s->vessels[i].part_count;
                if (altitude != nullptr) *altitude = s->vessels[i].altitude;
            }
    kspcore_sim_destroy(sim);
    return h;
}

// ---- the C++ path: the host class, no C layer -------------------------------------------------------------------------------------
uint64_t RunCpp(const char* save, int frames) {
    ksp::host::GameHost host;
    REQUIRE(host.NewGame(KSPCORE_MODE_SANDBOX, save));
    const Paths p;
    REQUIRE(host.LaunchCraft(p.craft, "LaunchPad"));
    REQUIRE(host.Frame(0.02));
    REQUIRE(host.SceneLoaded(KSPCORE_SCENE_FLIGHT));
    const Script script;
    int32_t down[3], held[1];
    for (int k = 0; k < frames; ++k) {
        host.SetInput(script.Frame(k, down, held));
        REQUIRE(host.Frame(0.02));
    }
    return ksp::host::SnapshotHash(host.Snapshot());
}

std::vector<std::string>& LogLines() {
    static std::vector<std::string> v;
    return v;
}
void CaptureLog(int level, const char* message, void*) {
    (void)level;
    LogLines().emplace_back(message);
    if (std::getenv("KSPCORE_SLICE_LOG") != nullptr) std::fprintf(stderr, "[log] %s\n", message);
}

struct ProcessGuard {
    bool ok;
    explicit ProcessGuard(const Paths& p) : ok(InitProcess(p)) {}
    ~ProcessGuard() { kspcore_game_shutdown(); }
};

}  // namespace

TEST_CASE("WP-UE-02 sim destroy / re-create: the C ABI flight scene leaves nothing behind the hygiene checks, a second sim flies") {
    if (!GameAvailable()) return;
    const Paths p;
    ProcessGuard pg(p);
    REQUIRE(pg.ok);
    uint32_t parts1 = 0, parts2 = 0;
    double alt1 = 0.0, alt2 = 0.0;
    const kspcore_tests::HygieneProbe probe;  // after kspcore_game_init / load_database: the process-level GameData's own listeners are the baseline
    const uint64_t h1 = RunAbi(p, "wpue02_a", 160, &parts1, &alt1);
    std::string what1, what2;
    const size_t leaks1 = probe.Leaks(&what1);
    const uint64_t h2 = RunAbi(p, "wpue02_b", 160, &parts2, &alt2);
    const size_t leaks2 = probe.Leaks(&what2);
    MESSAGE("sim #1: hash " << std::hex << h1 << std::dec << ", " << parts1 << " parts, altitude " << alt1 << " m, leaks " << leaks1 << "; sim #2: hash " << std::hex
                            << h2 << std::dec << ", " << parts2 << " parts, altitude " << alt2 << " m, leaks " << leaks2);
    CHECK(leaks1 == 0);
    CHECK(leaks2 == 0);
    CHECK(parts1 == 153);
    CHECK(parts2 == 153);
    // the same scripted run in a new sim in the same process ends in the same state (nothing leaked between the sims)
    CHECK(h1 == h2);
}

TEST_CASE("WP-UE-02 flight slice twin: the C++ path and the C ABI path end the scripted M2 ascent in the same state hash; so does the pure-C ctest") {
    if (!GameAvailable()) return;
    if (std::getenv("KSPCORE_SLICE_TWIN") == nullptr) return;  // the heavy case: ctest capi.flight_slice_twin
    const Paths p;
    ProcessGuard pg(p);
    REQUIRE(pg.ok);
    const kspcore_tests::HygieneProbe probe;
    const int frames = 100 + 4560;  // the C test's settle + C10 window
    const uint64_t viaAbi = RunAbi(p, "wpue02_twin_abi", frames);
    {
        std::string what;
        const size_t n = probe.Leaks(&what);
        INFO("after the ABI run: " << what);
        CHECK(n == 0);
    }
    const uint64_t viaCpp = RunCpp("wpue02_twin_cpp", frames);
    {
        std::string what;
        const size_t n = probe.Leaks(&what);
        INFO("after the C++ run: " << what);
        CHECK(n == 0);
    }
    MESSAGE("state hash after " << frames << " frames: ABI " << std::hex << viaAbi << ", C++ " << viaCpp << std::dec);
    CHECK(viaAbi == viaCpp);
    if (const char* f = std::getenv("KSPCORE_SLICE_HASH_FILE")) {
        std::ifstream in(f);
        std::string line;
        REQUIRE(static_cast<bool>(std::getline(in, line)));
        const uint64_t viaC = std::strtoull(line.c_str(), nullptr, 16);
        MESSAGE("the pure-C ctest wrote " << line);
        CHECK(viaC == viaCpp);
    }
}

TEST_CASE("WP-UE-02 quicksave / quickload through the ABI: F5 state saved, the flight scene torn down and rebuilt from the file, the vessel back where it was") {
    if (!GameAvailable()) return;
    const Paths p;
    ProcessGuard pg(p);
    REQUIRE(pg.ok);
    const kspcore_tests::HygieneProbe probe;
    kspcore_sim* sim = kspcore_sim_create();
    REQUIRE(sim != nullptr);
    std::error_code ec;
    fs::remove_all(fs::path(p.saves) / "wpue02_qs", ec);  // a clean save folder: no quicksave from an earlier run
    REQUIRE(kspcore_game_new(sim, KSPCORE_MODE_SANDBOX, "wpue02_qs") == 1);
    CHECK(kspcore_game_has_quicksave(sim) == 0);
    REQUIRE(kspcore_game_launch_craft(sim, p.craft.c_str(), "LaunchPad") == 1);
    REQUIRE(kspcore_frame(sim, 0.02) == 1);
    REQUIRE(kspcore_scene_loaded(sim, KSPCORE_SCENE_FLIGHT) == 1);
    Script script;
    script.settle = 130;
    int32_t down[3], held[1];
    auto activeOf = [](const kspcore_snapshot_view* s) -> const kspcore_vessel_state* {
        for (uint32_t i = 0; i < s->vessel_count; ++i)
            if ((s->vessels[i].flags & KSPCORE_VESSEL_ACTIVE) != 0) return &s->vessels[i];
        return nullptr;
    };
    // on the pad at rest (KSP refuses a quicksave under thrust, FlightGlobals.ClearToSave)
    for (int k = 0; k < 120; ++k) {
        const kspcore_input_frame in = script.Frame(k, down, held);
        REQUIRE(kspcore_input_set_frame(sim, &in) == 1);
        REQUIRE(kspcore_frame(sim, 0.02) == 1);
    }
    const kspcore_snapshot_view* s = kspcore_snapshot(sim);
    const kspcore_vessel_state* v = activeOf(s);
    REQUIRE(v != nullptr);
    const double altSaved = v->altitude;
    const double utSaved = s->frame.ut;
    CHECK(v->situation == 4);  // PRELAUNCH
    REQUIRE(kspcore_game_quicksave(sim) == 1);
    CHECK(kspcore_game_has_quicksave(sim) == 1);
    for (int k = 120; k < 320; ++k) {
        const kspcore_input_frame in = script.Frame(k, down, held);
        REQUIRE(kspcore_input_set_frame(sim, &in) == 1);
        REQUIRE(kspcore_frame(sim, 0.02) == 1);
    }
    v = activeOf(kspcore_snapshot(sim));
    REQUIRE(v != nullptr);
    CHECK(v->altitude > altSaved + 20.0);  // lifted off after the quicksave
    // quickload: the scene load is requested (event), the old scene goes at the next frame, the shell answers kspcore_scene_loaded
    LogLines().clear();
    kspcore_set_log_callback(&CaptureLog, nullptr);
    REQUIRE(kspcore_game_quickload(sim) == 1);
    REQUIRE(kspcore_frame(sim, 0.02) == 1);
    s = kspcore_snapshot(sim);
    bool requested = false;
    for (uint32_t i = 0; i < s->event_count; ++i) requested = requested || (s->events[i].kind == KSPCORE_EVENT_SCENE_LOAD_REQUESTED && s->events[i].a == KSPCORE_SCENE_FLIGHT);
    CHECK(requested);
    CHECK(s->vessel_count == 0);
    const int loaded = kspcore_scene_loaded(sim, KSPCORE_SCENE_FLIGHT);
    kspcore_set_log_callback(nullptr, nullptr);
    INFO(std::string(kspcore_last_error()));
    for (const std::string& l : LogLines()) INFO(l);
    REQUIRE(loaded == 1);
    for (int k = 0; k < 5; ++k) REQUIRE(kspcore_frame(sim, 0.02) == 1);
    s = kspcore_snapshot(sim);
    v = activeOf(s);
    REQUIRE(v != nullptr);
    MESSAGE("quicksaved on the pad at altitude " << altSaved << " m, UT " << utSaved << "; reloaded: altitude " << v->altitude << " m, UT " << s->frame.ut << ", " << v->part_count
                                      << " parts, " << s->vessel_count << " vessel(s)");
    CHECK(v->part_count == 153);
    CHECK(std::fabs(s->frame.ut - (utSaved + 5 * 0.02)) < 1e-3);  // the saved UT plus the five frames run since the reload
    CHECK(std::fabs(v->altitude - altSaved) < 1.0);  // back on the pad
    kspcore_sim_destroy(sim);
    std::string what;
    const size_t n = probe.Leaks(&what);
    INFO(what);
    CHECK(n == 0);
}

TEST_CASE("WP-UE-02 revert to launch through the ABI: the flight is torn down and the vessel is on the pad again") {
    if (!GameAvailable()) return;
    const Paths p;
    ProcessGuard pg(p);
    REQUIRE(pg.ok);
    const kspcore_tests::HygieneProbe probe;
    kspcore_sim* sim = kspcore_sim_create();
    REQUIRE(sim != nullptr);
    REQUIRE(kspcore_game_new(sim, KSPCORE_MODE_SANDBOX, "wpue02_rv") == 1);
    REQUIRE(kspcore_game_launch_craft(sim, p.craft.c_str(), "LaunchPad") == 1);
    REQUIRE(kspcore_frame(sim, 0.02) == 1);
    REQUIRE(kspcore_scene_loaded(sim, KSPCORE_SCENE_FLIGHT) == 1);
    Script script;
    script.settle = 40;  // FlightDriver.PostInit saves the revert state after 15 frames (FlightDriver.cs:376)
    int32_t down[3], held[1];
    for (int k = 0; k < 160; ++k) {
        const kspcore_input_frame in = script.Frame(k, down, held);
        REQUIRE(kspcore_input_set_frame(sim, &in) == 1);
        REQUIRE(kspcore_frame(sim, 0.02) == 1);
    }
    const kspcore_snapshot_view* s = kspcore_snapshot(sim);
    const kspcore_vessel_state* v = nullptr;
    for (uint32_t i = 0; i < s->vessel_count; ++i)
        if ((s->vessels[i].flags & KSPCORE_VESSEL_ACTIVE) != 0) v = &s->vessels[i];
    REQUIRE(v != nullptr);
    CHECK(v->altitude > 110.0);  // in the air
    REQUIRE(kspcore_game_revert_to_launch(sim) == 1);
    REQUIRE(kspcore_frame(sim, 0.02) == 1);
    CHECK(kspcore_snapshot(sim)->vessel_count == 0);
    const int loaded = kspcore_scene_loaded(sim, KSPCORE_SCENE_FLIGHT);
    INFO(std::string(kspcore_last_error()));
    REQUIRE(loaded == 1);
    for (int k = 0; k < 30; ++k) REQUIRE(kspcore_frame(sim, 0.02) == 1);
    s = kspcore_snapshot(sim);
    v = nullptr;
    for (uint32_t i = 0; i < s->vessel_count; ++i)
        if ((s->vessels[i].flags & KSPCORE_VESSEL_ACTIVE) != 0) v = &s->vessels[i];
    REQUIRE(v != nullptr);
    MESSAGE("after the revert: altitude " << v->altitude << " m, situation " << v->situation << ", " << v->part_count << " parts, UT " << s->frame.ut);
    CHECK(v->part_count == 153);
    CHECK(v->altitude < 103.0);
    CHECK(v->situation == 4);  // PRELAUNCH
    kspcore_sim_destroy(sim);
    std::string what;
    const size_t n = probe.Leaks(&what);
    INFO(what);
    CHECK(n == 0);
}

TEST_CASE("WP-UE-02 the stock Kerbal X launches from the pad through the ABI: 73 parts, the throttle key and the stage key lift it") {
    if (!GameAvailable()) return;
    const Paths p;
    ProcessGuard pg(p);
    REQUIRE(pg.ok);
    const std::string kerbalX = std::string(KSPCORE_PROJECT_ROOT) + "/harness/inputs/Ships/VAB/Kerbal X.craft";
    if (!fs::exists(kerbalX)) return;
    kspcore_sim* sim = kspcore_sim_create();
    REQUIRE(sim != nullptr);
    REQUIRE(kspcore_game_new(sim, KSPCORE_MODE_SANDBOX, "wpue02_kx") == 1);
    const int launched = kspcore_game_launch_craft(sim, kerbalX.c_str(), "LaunchPad");
    INFO(std::string(kspcore_last_error()));
    REQUIRE(launched == 1);
    REQUIRE(kspcore_frame(sim, 0.02) == 1);
    LogLines().clear();
    kspcore_set_log_callback(&CaptureLog, nullptr);
    const int loaded = kspcore_scene_loaded(sim, KSPCORE_SCENE_FLIGHT);
    kspcore_set_log_callback(nullptr, nullptr);
    INFO(std::string(kspcore_last_error()));
    for (const std::string& l : LogLines()) INFO(l);
    REQUIRE(loaded == 1);
    Script script;
    script.settle = 60;
    int32_t down[3], held[1];
    double alt0 = 0.0, alt1 = 0.0;
    uint32_t parts = 0, partsBefore = 0;
    for (int k = 0; k < 400; ++k) {
        const kspcore_input_frame in = script.Frame(k, down, held);
        REQUIRE(kspcore_input_set_frame(sim, &in) == 1);
        REQUIRE(kspcore_frame(sim, 0.02) == 1);
        const kspcore_snapshot_view* s = kspcore_snapshot(sim);
        for (uint32_t i = 0; i < s->vessel_count; ++i)
            if ((s->vessels[i].flags & KSPCORE_VESSEL_ACTIVE) != 0) {
                if (k == 59) {
                    alt0 = s->vessels[i].altitude;
                    partsBefore = s->vessels[i].part_count;
                }
                if (k == 399) alt1 = s->vessels[i].altitude;
                parts = s->vessels[i].part_count;
            }
    }
    MESSAGE("Kerbal X: " << partsBefore << " parts on the pad, " << parts << " after the first stage, altitude " << alt0 << " m before the launch keys, " << alt1 << " m 340 frames after");
    CHECK(partsBefore == 73);
    CHECK(parts == 70);  // observed: StageManager's first stage drops three parts (UNVERIFIED which; the launch clamps are the likely ones)
    CHECK(alt1 > alt0 + 20.0);
    kspcore_sim_destroy(sim);
}
