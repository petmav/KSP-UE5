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

namespace {

// struct_size rule: fill what fits in the caller's struct
template <class T>
void CopyOut(const T& value, T* out) {
    const uint32_t size = out->struct_size < sizeof(T) ? out->struct_size : static_cast<uint32_t>(sizeof(T));
    std::memcpy(out, &value, size);
    out->struct_size = size;
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

// ---- part models (WP-UE-10) ----

int kspcore_game_part_rig_node_count(uint32_t model_id) {
    return Guard("kspcore_game_part_rig_node_count", 0, [&]() -> int { return static_cast<int>(ksp::host::GameProcess::Rig(model_id).nodes.size()); });
}

int kspcore_game_part_rig_node(uint32_t model_id, int node, kspcore_part_rig_node* out) {
    return Guard("kspcore_game_part_rig_node", 0, [&]() -> int {
        const ksp::host::GameProcess::PartRig& rig = ksp::host::GameProcess::Rig(model_id);
        if (out == nullptr || node < 0 || static_cast<size_t>(node) >= rig.nodes.size()) throw std::invalid_argument("node out of range or out is NULL");
        const ksp::host::GameProcess::PartRigNode& n = rig.nodes[static_cast<size_t>(node)];
        kspcore_part_rig_node v{};
        v.struct_size = sizeof(v);
        v.name = n.name.c_str();
        v.parent = n.parent;
        std::memcpy(v.position, n.position, sizeof(v.position));
        std::memcpy(v.rotation, n.rotation, sizeof(v.rotation));
        std::memcpy(v.scale, n.scale, sizeof(v.scale));
        v.file = n.file;
        v.file_node = n.fileNode;
        CopyOut(v, out);
        return 1;
    });
}

int kspcore_game_part_model_file_count(uint32_t model_id) {
    return Guard("kspcore_game_part_model_file_count", 0, [&]() -> int { return static_cast<int>(ksp::host::GameProcess::Rig(model_id).files.size()); });
}

int kspcore_game_part_model_file(uint32_t model_id, int file, kspcore_part_model_file* out) {
    return Guard("kspcore_game_part_model_file", 0, [&]() -> int {
        const ksp::host::GameProcess::PartRig& rig = ksp::host::GameProcess::Rig(model_id);
        if (out == nullptr || file < 0 || static_cast<size_t>(file) >= rig.files.size()) throw std::invalid_argument("file out of range or out is NULL");
        const ksp::host::GameProcess::PartRigFile& f = rig.files[static_cast<size_t>(file)];
        kspcore_part_model_file v{};
        v.struct_size = sizeof(v);
        v.path = f.path.c_str();
        v.parent_url = f.parentUrl.c_str();
        v.texture_replacement_count = static_cast<int32_t>(f.textureReplacements.size());
        CopyOut(v, out);
        return 1;
    });
}

int kspcore_game_part_texture_replacement(uint32_t model_id, int file, int index, const char** original, const char** url) {
    return Guard("kspcore_game_part_texture_replacement", 0, [&]() -> int {
        const ksp::host::GameProcess::PartRig& rig = ksp::host::GameProcess::Rig(model_id);
        if (file < 0 || static_cast<size_t>(file) >= rig.files.size()) throw std::invalid_argument("file out of range");
        const auto& list = rig.files[static_cast<size_t>(file)].textureReplacements;
        if (index < 0 || static_cast<size_t>(index) >= list.size() || original == nullptr || url == nullptr) throw std::invalid_argument("index out of range or NULL output");
        *original = list[static_cast<size_t>(index)].first.c_str();
        *url = list[static_cast<size_t>(index)].second.c_str();
        return 1;
    });
}

const char* kspcore_game_texture_file(const char* url) {
    return Guard("kspcore_game_texture_file", static_cast<const char*>(nullptr), [&]() -> const char* {
        return url != nullptr ? ksp::host::GameProcess::TextureFile(url) : nullptr;
    });
}

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
