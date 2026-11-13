// WP-UE-02: the game host behind the C ABI v2. Two things the pure-C ctest capi.flight_slice cannot say:
//   * the C++ path (GameHost driven directly) and the C ABI path (the extern "C" entry points) end a scripted PhysProf-M2 ascent in the
//     same snapshot hash, bit for bit; the hash the pure-C program wrote to a file (ctest capi.flight_slice) is the same too;
//   * kspcore_sim_destroy leaves nothing behind the unit-test hygiene checks (tests/test_hygiene.*) would flag, and a new sim can be made.
// The heavy case (the whole scripted ascent, three runs) runs only when KSPCORE_SLICE_TWIN is set (ctest capi.flight_slice_twin);
// the short case runs in every unit shard.
#include <cstdint>
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
    kspcore_tests::HygieneLeaks("(before)");
    uint32_t parts1 = 0, parts2 = 0;
    double alt1 = 0.0, alt2 = 0.0;
    const uint64_t h1 = RunAbi(p, "wpue02_a", 160, &parts1, &alt1);
    const size_t leaks1 = kspcore_tests::HygieneLeaks("after kspcore_sim_destroy #1");
    const uint64_t h2 = RunAbi(p, "wpue02_b", 160, &parts2, &alt2);
    const size_t leaks2 = kspcore_tests::HygieneLeaks("after kspcore_sim_destroy #2");
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
    const int frames = 100 + 4560;  // the C test's settle + C10 window
    const uint64_t viaAbi = RunAbi(p, "wpue02_twin_abi", frames);
    CHECK(kspcore_tests::HygieneLeaks("after the ABI run") == 0);
    const uint64_t viaCpp = RunCpp("wpue02_twin_cpp", frames);
    CHECK(kspcore_tests::HygieneLeaks("after the C++ run") == 0);
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
