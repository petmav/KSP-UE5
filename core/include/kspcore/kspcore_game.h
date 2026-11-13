/*
 * kspcore C ABI v2, domain header "game": process initialisation, game data load, new game / load / save / quicksave, scene
 * flow, settings (ue5/DESIGN.md 1.3, 2). WP-UE-02.
 *
 * Same rules as kspcore_c.h: plain C types only, no exception crosses the boundary (failures return 0 / NULL and set
 * kspcore_last_error), UTF-8 NUL-terminated strings, returned `const char*` stay valid until the object they came from changes.
 * Every struct that crosses the boundary starts with `uint32_t struct_size` (the caller sets it to sizeof of ITS version of the
 * struct; kspcore reads only that many bytes and leaves the rest of its own defaults). Later versions only append fields.
 *
 * Lifetime: ONE game per process (core has process-wide singletons: FlightGlobals, InputLockManager, GameSettings, GameEvents ...).
 *
 *     kspcore_game_init(&cfg)                      once per process
 *     kspcore_game_load_database(progress, user)   GameDatabase + PartLoader data side + Physics.cfg + celestial system
 *     sim = kspcore_sim_create()                   the simulation context that owns the game and its scene
 *       kspcore_game_new(sim, mode, name)          or kspcore_game_load(sim, name, "persistent")
 *       kspcore_game_launch_craft(sim, ...)        FlightDriver.StartWithNewLaunch: queues a "scene load requested" event
 *       (shell builds the scene's actors) kspcore_scene_loaded(sim, KSPCORE_SCENE_FLIGHT)
 *       per rendered frame: kspcore_input_set_frame, kspcore_commands_submit, kspcore_frame, kspcore_snapshot   (kspcore_frame.h)
 *     kspcore_sim_destroy(sim)                     resets every process-wide singleton the sim touched; a new sim can be created
 *     kspcore_game_shutdown()
 *
 * The functions below take the sim handle (DESIGN 1.3 lists them without it: one sim per process, so it only names the owner).
 */
#ifndef KSPCORE_GAME_H
#define KSPCORE_GAME_H

#include <stddef.h>
#include <stdint.h>

#include "kspcore_c.h"

#define KSPCORE_GAME_ABI 1

#ifdef __cplusplus
extern "C" {
#endif

/* ---- process init ---------------------------------------------------------------------------------------------------- */

typedef struct kspcore_game_config {
    uint32_t struct_size;
    /* The game root: the folder holding GameData/ (decompiled/KSP-dev at runtime; the mirror in tests). KSPUtil.ApplicationRootPath. */
    const char* game_root;
    /* kspcore's own derived data (the repository's core/data: terrain maps, scenery colliders, engine thrust transforms). */
    const char* data_dir;
    /* Local extraction of the Unity-serialised game files (ue5/GameAssets). May be NULL: kspcore only passes the path on. */
    const char* extraction_root;
    /* Saves root: a save is <saves_dir>/<save name>/<file>.sfs (ue5/Saved/ksp/saves). Created when missing. */
    const char* saves_dir;
    /* Physics.cfg to load (PhysicsGlobals.physicsDatabaseFilename). NULL: <game_root>/Physics.cfg. */
    const char* physics_cfg;
    /* Optional directory of reduced model data (collider reference files: mucollider_reference.json and the .mu files it lists) used
     * for a part whose own .mu in the game root is a zero-byte stub (the repository's GameData mirror). NULL: none. */
    const char* reduced_model_dir;
    /* PQS height-pass worker threads (bit-identical results; 0 = none, 1..16). */
    int32_t pqs_height_threads;
    /* Jolt worker threads of the physics world (0 = the calling thread; DESIGN 1.4: 0 or 1). */
    int32_t physics_threads;
} kspcore_game_config;

/* Process init. Returns 1 on success. A second call before kspcore_game_shutdown fails. */
int kspcore_game_init(const kspcore_game_config* config);
/* Releases the game database and every process-level object kspcore_game_init made. All sims must be destroyed first. */
void kspcore_game_shutdown(void);

/* Load progress: `fraction` 0..1 over the whole load, `stage` names the step ("GameDatabase", "PartLoader", ...). Called on the
 * calling thread, between steps. May be NULL. */
typedef void (*kspcore_progress_fn)(float fraction, const char* stage, void* user);

/* GameDatabase (cfg files), PartLoader's data side (part prefabs without models), localisation, Physics.cfg, the celestial
 * system prefab. Returns 1 on success. Part and resource counts are available afterwards. */
int kspcore_game_load_database(kspcore_progress_fn progress, void* user);
int kspcore_game_part_count(void);
int kspcore_game_resource_count(void);
/* Part name of a model id (a snapshot part's `model_id`), "" when unknown. Valid for the process lifetime. */
const char* kspcore_game_model_name(uint32_t model_id);

/* ---- game ------------------------------------------------------------------------------------------------------------ */

/* Game_Modes */
#define KSPCORE_MODE_SANDBOX 0
#define KSPCORE_MODE_CAREER 1
#define KSPCORE_MODE_SCIENCE_SANDBOX 4

/* GameScenes */
#define KSPCORE_SCENE_LOADING 0
#define KSPCORE_SCENE_MAINMENU 2
#define KSPCORE_SCENE_SETTINGS 3
#define KSPCORE_SCENE_SPACECENTER 5
#define KSPCORE_SCENE_EDITOR 6
#define KSPCORE_SCENE_FLIGHT 7
#define KSPCORE_SCENE_TRACKSTATION 8

/* New game: GameParameters at the mode's Normal preset, sandbox facility levels, saved as <save>/persistent.sfs. 1 on success. */
int kspcore_game_new(kspcore_sim* sim, int mode, const char* save_name);
/* Loads <saves_dir>/<save_name>/<file>.sfs (file without extension; "persistent" for the main save), through the upgrade pipeline. */
int kspcore_game_load(kspcore_sim* sim, const char* save_name, const char* file);
/* GamePersistence.SaveGame of the current game as <saves_dir>/<save_name>/<file>.sfs. 1 on success. */
int kspcore_game_save(kspcore_sim* sim, const char* save_name, const char* file);
/* Number of .sfs files of a save folder and their names (without extension), '\n' separated, same buffer protocol as
 * kspcore_confignode_save_to_buffer: the save dialog's list. */
size_t kspcore_game_list_saves(kspcore_sim* sim, const char* save_name, char* buffer, size_t capacity);

/* QuickSaveLoad.QuickSave / quickLoad (QuickSaveLoad.cs:189 / :252) of the current game: file "quicksave". 1 on success; a
 * quickload queues the FLIGHT scene load like the game (event KSPCORE_EVENT_SCENE_LOAD_REQUESTED). F5 / F9 also reach
 * QuickSaveLoad through the input device. */
int kspcore_game_quicksave(kspcore_sim* sim);
int kspcore_game_quickload(kspcore_sim* sim);
int kspcore_game_has_quicksave(kspcore_sim* sim);

/* FlightDriver.StartWithNewLaunch (FlightDriver.cs:461): launch the craft file (.craft, KSP's own format) from `launch_site`
 * ("LaunchPad", "Runway") in the current game. Queues the FLIGHT scene load. 1 on success. */
int kspcore_game_launch_craft(kspcore_sim* sim, const char* craft_path, const char* launch_site);
/* FlightDriver.RevertToLaunch (FlightDriver.cs:475): back to the pre-launch state. Queues the FLIGHT scene load. */
int kspcore_game_revert_to_launch(kspcore_sim* sim);
/* HighLogic.LoadScene(scene): queues a scene load (the main menu, the space centre ...). 1 if accepted. */
int kspcore_game_load_scene(kspcore_sim* sim, int scene);

/* The shell has built the actors of `scene`: core calls HighLogic.OnSceneLoaded and starts the scene's drivers (for FLIGHT:
 * the flight scene is composed here, the vessels are spawned; the first frame follows). 1 on success. */
int kspcore_scene_loaded(kspcore_sim* sim, int scene);
/* The scene core is in (HighLogic.LoadedScene); -1 without a sim. */
int kspcore_game_current_scene(kspcore_sim* sim);

/* ---- settings -------------------------------------------------------------------------------------------------------- */

/* GameSettings (settings.cfg, GameSettings.cs:1506 Awake / SetDefaultValues / SaveSettings). Names are the C# field names
 * ("MASTER_VOLUME", "CAMERA_FOV" ...). Values are strings in the cfg's own format. */
int kspcore_settings_load(const char* settings_cfg_path);
int kspcore_settings_save(const char* settings_cfg_path);
/* The value as text, NULL for an unknown name. */
const char* kspcore_settings_get(const char* name);
/* 1 if the name is a known setting and the text parsed. */
int kspcore_settings_set(const char* name, const char* value);

#ifdef __cplusplus
}
#endif

#endif /* KSPCORE_GAME_H */
