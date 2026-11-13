// C ABI v2, "game" domain (include/kspcore/kspcore_game.h). Every entry point catches all C++ exceptions.
#include <cstring>
#include <stdexcept>
#include <string>

#include "capi_internal.hpp"
#include "ksp/host/game_host.hpp"
#include "ksp/input/game_settings.hpp"
#include "kspcore/kspcore_game.h"

namespace {

using kspcore_capi::Guard;
using kspcore_capi::SetError;

// The sim's host, made on first use (it needs the database kspcore_game_load_database loads); NULL with kspcore_last_error otherwise
ksp::host::GameHost* Host(kspcore_sim* sim, const char* where) {
    if (sim == nullptr) {
        SetError(where, "sim is NULL");
        return nullptr;
    }
    if (!sim->host) {
        if (!ksp::host::GameProcess::DatabaseLoaded()) {
            SetError(where, "the game database is not loaded (kspcore_game_init, kspcore_game_load_database)");
            return nullptr;
        }
        sim->host = std::make_unique<ksp::host::GameHost>();
    }
    return sim->host.get();
}

template <class F>
int HostCall(kspcore_sim* sim, const char* where, F&& f) {
    return Guard(where, 0, [&]() -> int {
        ksp::host::GameHost* h = Host(sim, where);
        if (h == nullptr) return 0;
        if (f(*h)) return 1;
        SetError(where, h->Error().c_str());
        return 0;
    });
}

std::string& SettingsText() {
    static std::string s;
    return s;
}

}  // namespace

extern "C" {

int kspcore_game_init(const kspcore_game_config* config) {
    return Guard("kspcore_game_init", 0, [&]() -> int {
        if (config == nullptr) throw std::invalid_argument("config is NULL");
        auto str = [](const char* s) { return s != nullptr ? std::string(s) : std::string(); };
        ksp::host::GameConfig c;
        c.gameRoot = str(config->game_root);
        c.dataDir = str(config->data_dir);
        c.extractionRoot = str(config->extraction_root);
        c.savesDir = str(config->saves_dir);
        c.physicsCfg = str(config->physics_cfg);
        c.reducedModelDir = str(config->reduced_model_dir);
        // fields after struct_size are read only when the caller's struct has them (forward compatibility)
        c.pqsHeightThreads = config->pqs_height_threads;
        c.physicsThreads = config->physics_threads;
        std::string err;
        if (!ksp::host::GameProcess::Init(c, err)) {
            SetError("kspcore_game_init", err.c_str());
            return 0;
        }
        return 1;
    });
}

void kspcore_game_shutdown(void) {
    Guard("kspcore_game_shutdown", 0, [] {
        ksp::host::GameProcess::Shutdown();
        return 0;
    });
}

int kspcore_game_load_database(kspcore_progress_fn progress, void* user) {
    return Guard("kspcore_game_load_database", 0, [&]() -> int {
        std::string err;
        ksp::host::ProgressFn fn;
        if (progress != nullptr) fn = [progress, user](float f, const char* stage) { progress(f, stage, user); };
        if (!ksp::host::GameProcess::LoadDatabase(fn, err)) {
            SetError("kspcore_game_load_database", err.c_str());
            return 0;
        }
        return 1;
    });
}

int kspcore_game_part_count(void) { return ksp::host::GameProcess::PartCount(); }
int kspcore_game_resource_count(void) { return ksp::host::GameProcess::ResourceCount(); }
const char* kspcore_game_model_name(uint32_t model_id) { return ksp::host::GameProcess::ModelName(model_id); }

int kspcore_game_new(kspcore_sim* sim, int mode, const char* save_name) {
    return HostCall(sim, "kspcore_game_new", [&](ksp::host::GameHost& h) { return h.NewGame(mode, save_name != nullptr ? save_name : ""); });
}

int kspcore_game_load(kspcore_sim* sim, const char* save_name, const char* file) {
    return HostCall(sim, "kspcore_game_load", [&](ksp::host::GameHost& h) { return h.LoadGame(save_name != nullptr ? save_name : "", file != nullptr ? file : "persistent"); });
}

int kspcore_game_save(kspcore_sim* sim, const char* save_name, const char* file) {
    return HostCall(sim, "kspcore_game_save", [&](ksp::host::GameHost& h) { return h.SaveGame(save_name != nullptr ? save_name : "", file != nullptr ? file : "persistent"); });
}

size_t kspcore_game_list_saves(kspcore_sim* sim, const char* save_name, char* buffer, size_t capacity) {
    return Guard("kspcore_game_list_saves", static_cast<size_t>(0), [&]() -> size_t {
        ksp::host::GameHost* h = Host(sim, "kspcore_game_list_saves");
        return h != nullptr ? h->ListSaves(save_name != nullptr ? save_name : "", buffer, capacity) : 0;
    });
}

int kspcore_game_quicksave(kspcore_sim* sim) { return HostCall(sim, "kspcore_game_quicksave", [](ksp::host::GameHost& h) { return h.QuickSave(); }); }
int kspcore_game_quickload(kspcore_sim* sim) { return HostCall(sim, "kspcore_game_quickload", [](ksp::host::GameHost& h) { return h.QuickLoad(); }); }
int kspcore_game_has_quicksave(kspcore_sim* sim) {
    return Guard("kspcore_game_has_quicksave", 0, [&]() -> int {
        ksp::host::GameHost* h = Host(sim, "kspcore_game_has_quicksave");
        return h != nullptr && h->HasQuickSave() ? 1 : 0;
    });
}

int kspcore_game_launch_craft(kspcore_sim* sim, const char* craft_path, const char* launch_site) {
    return HostCall(sim, "kspcore_game_launch_craft", [&](ksp::host::GameHost& h) {
        return h.LaunchCraft(craft_path != nullptr ? craft_path : "", launch_site != nullptr ? launch_site : "LaunchPad");
    });
}

int kspcore_game_revert_to_launch(kspcore_sim* sim) { return HostCall(sim, "kspcore_game_revert_to_launch", [](ksp::host::GameHost& h) { return h.RevertToLaunch(); }); }
int kspcore_game_load_scene(kspcore_sim* sim, int scene) { return HostCall(sim, "kspcore_game_load_scene", [&](ksp::host::GameHost& h) { return h.LoadScene(scene); }); }
int kspcore_scene_loaded(kspcore_sim* sim, int scene) { return HostCall(sim, "kspcore_scene_loaded", [&](ksp::host::GameHost& h) { return h.SceneLoaded(scene); }); }
int kspcore_game_current_scene(kspcore_sim* sim) { return sim != nullptr && sim->host ? sim->host->CurrentScene() : -1; }

// ---- settings ------------------------------------------------------------------------------------------------------------------

int kspcore_settings_load(const char* settings_cfg_path) {
    return Guard("kspcore_settings_load", 0, [&]() -> int {
        if (settings_cfg_path == nullptr) throw std::invalid_argument("path is NULL");
        return ksp::host::SettingsLoad(settings_cfg_path) ? 1 : 0;
    });
}

int kspcore_settings_save(const char* settings_cfg_path) {
    return Guard("kspcore_settings_save", 0, [&]() -> int {
        if (settings_cfg_path == nullptr) throw std::invalid_argument("path is NULL");
        return ksp::host::SettingsSave(settings_cfg_path) ? 1 : 0;
    });
}

const char* kspcore_settings_get(const char* name) {
    return Guard("kspcore_settings_get", static_cast<const char*>(nullptr), [&]() -> const char* {
        if (name == nullptr) return nullptr;
        std::string v;
        if (!ksp::host::SettingsGet(name, v)) return nullptr;
        SettingsText() = v;
        return SettingsText().c_str();
    });
}

int kspcore_settings_set(const char* name, const char* value) {
    return Guard("kspcore_settings_set", 0, [&]() -> int { return name != nullptr && value != nullptr && ksp::host::SettingsSet(name, value) ? 1 : 0; });
}

}  // extern "C"
