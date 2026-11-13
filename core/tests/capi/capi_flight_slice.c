/*
 * WP-UE-02 ctest capi.flight_slice: pure C consumer of the kspcore C ABI v2 (kspcore_game.h, kspcore_frame.h). The Unreal bridge makes
 * exactly these calls (ue5/DESIGN.md 1.3): process init, database load, new game, launch, scene loaded, then per frame
 * input / commands / kspcore_frame / snapshot.
 *
 * PhysProf-M2 is launched from the KSC LaunchPad the way capture C10 flew it (HarnessCapture2Flight.cs:759-764: full throttle, SAS on,
 * the first stage), here through KSP's own keys (Z, T, Space) in the input frame, and its ascent is compared with KSP's own C10 states
 * (harness/captures/out2/aero_forces_vessel.json, extracted bit for bit to tests/data/c10_states.csv) at the tolerances WP-20f gated the
 * C++ path with (speed at an altitude 1 %, altitude at a Mach number 1.5 %, frames between two states 2.5 %). The final state hash
 * (kspcore_snapshot_hash) is written to a file for the C++ twin (unit case "WP-UE-02 flight slice ...", ctest capi.flight_slice_twin).
 *
 * argv: 1 game_root  2 data_dir  3 reduced_model_dir  4 saves_dir  5 craft path  6 c10_states.csv  7 hash output file
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "kspcore/kspcore_c.h"

static int failures = 0;
#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            ++failures;                                                \
        }                                                              \
    } while (0)

#define KEY_SPACE 32
#define KEY_T 116
#define KEY_Z 122

static int log_lines = 0;
static void on_log(int level, const char* message, void* user) {
    (void)user;
    /* warnings and errors of the host and of KSP's own code paths, the first few only */
    if (level >= KSPCORE_LOG_WARNING && log_lines < 60) {
        ++log_lines;
        printf("[log %d] %s\n", level, message);
    }
}

static int progress_calls = 0;
static float progress_last = -1.0f;
static int progress_monotonic = 1;
static void on_progress(float fraction, const char* stage, void* user) {
    (void)stage;
    (void)user;
    ++progress_calls;
    if (fraction < progress_last) progress_monotonic = 0;
    progress_last = fraction;
}

typedef struct c10_state {
    char state[32];
    int frame;
    double altitude, spd, mach;
} c10_state;

static double hex_double(const char* h) {
    const uint64_t bits = (uint64_t)strtoull(h, NULL, 16);
    double d;
    memcpy(&d, &bits, sizeof d);
    return d;
}

static int load_c10(const char* path, const char* craft, c10_state* out, int cap) {
    FILE* f = fopen(path, "r");
    char line[512];
    int n = 0;
    if (!f) return -1;
    while (fgets(line, sizeof line, f)) {
        char* tok[6];
        int k = 0;
        char* p = line;
        if (line[0] == '#' || strncmp(line, "craft,", 6) == 0) continue;
        tok[k++] = p;
        while (*p && k < 6) {
            if (*p == ',') {
                *p = '\0';
                tok[k++] = p + 1;
            } else if (*p == '\n' || *p == '\r') {
                *p = '\0';
            }
            ++p;
        }
        if (k < 6 || strcmp(tok[0], craft) != 0 || n >= cap) continue;
        strncpy(out[n].state, tok[1], sizeof out[n].state - 1);
        out[n].state[sizeof out[n].state - 1] = '\0';
        out[n].frame = atoi(tok[2]);
        out[n].altitude = hex_double(tok[3]);
        out[n].spd = hex_double(tok[4]);
        out[n].mach = hex_double(tok[5]);
        ++n;
    }
    fclose(f);
    return n;
}

static const kspcore_vessel_state* active_vessel(const kspcore_snapshot_view* s) {
    uint32_t i;
    for (i = 0; i < s->vessel_count; ++i)
        if (s->vessels[i].flags & KSPCORE_VESSEL_ACTIVE) return &s->vessels[i];
    return NULL;
}

static int has_event(const kspcore_snapshot_view* s, int kind, int a) {
    uint32_t i;
    for (i = 0; i < s->event_count; ++i)
        if (s->events[i].kind == kind && s->events[i].a == a) return 1;
    return 0;
}

static kspcore_sim* start_flight(const char* craft, const char* save_name) {
    kspcore_sim* sim = kspcore_sim_create();
    const kspcore_snapshot_view* s;
    CHECK(sim != NULL);
    if (!sim) return NULL;
    CHECK(kspcore_game_new(sim, KSPCORE_MODE_SANDBOX, save_name) == 1);
    CHECK(kspcore_game_launch_craft(sim, craft, "LaunchPad") == 1);
    /* the launch queued the FLIGHT scene load: the shell sees the event in the next frame's snapshot, builds, answers */
    CHECK(kspcore_frame(sim, 0.02) == 1);
    s = kspcore_snapshot(sim);
    CHECK(s != NULL && has_event(s, KSPCORE_EVENT_SCENE_LOAD_REQUESTED, KSPCORE_SCENE_FLIGHT));
    CHECK(kspcore_scene_loaded(sim, KSPCORE_SCENE_FLIGHT) == 1);
    if (failures) printf("last error: %s\n", kspcore_last_error());
    CHECK(kspcore_game_current_scene(sim) == KSPCORE_SCENE_FLIGHT);
    s = kspcore_snapshot(sim);
    CHECK(s != NULL && has_event(s, KSPCORE_EVENT_SCENE_LOADED, KSPCORE_SCENE_FLIGHT));
    return sim;
}

int main(int argc, char** argv) {
    kspcore_game_config cfg;
    kspcore_sim* sim;
    const kspcore_snapshot_view* snap;
    const kspcore_vessel_state* v;
    c10_state ksp[32];
    int n_ksp, frames_total, settle = 100, window = 4560, k, i, j;
    double *alt, *spd, *mach;
    int ok_flight = 1;
    clock_t t_loop;

    if (argc < 8) {
        printf("usage: capi_flight_slice game_root data_dir reduced_model_dir saves_dir craft c10_states.csv hash_out\n");
        return 2;
    }
    memset(&cfg, 0, sizeof cfg);
    cfg.struct_size = sizeof cfg;
    cfg.game_root = argv[1];
    cfg.data_dir = argv[2];
    cfg.reduced_model_dir = argv[3];
    cfg.saves_dir = argv[4];
    cfg.pqs_height_threads = 0;
    cfg.physics_threads = 0;

    CHECK(kspcore_abi_version() == 2);
    /* calls before init fail with a message, never crash */
    CHECK(kspcore_game_load_database(NULL, NULL) == 0);
    CHECK(strlen(kspcore_last_error()) > 0);
    CHECK(kspcore_game_init(&cfg) == 1);
    CHECK(kspcore_game_init(&cfg) == 0); /* one process, one init */
    CHECK(kspcore_game_load_database(on_progress, NULL) == 1);
    CHECK(progress_calls >= 2 && progress_monotonic && progress_last == 1.0f);
    kspcore_set_log_callback(on_log, NULL); /* after the database load: PartLoader logs hundreds of cfg warnings */
    CHECK(kspcore_game_part_count() > 100);
    CHECK(kspcore_game_resource_count() > 5);

    n_ksp = load_c10(argv[6], "PhysProf-M2", ksp, 32);
    CHECK(n_ksp == 9);
    if (n_ksp <= 0) return 1;

    sim = start_flight(argv[5], "flight_slice");
    if (!sim) return 1;
    snap = kspcore_snapshot(sim);
    CHECK(snap->body_count >= 17);
    CHECK(snap->frame.dominant_body_id >= 0);
    CHECK(snap->frame.frame_kind == KSPCORE_FRAME_ROTATING);
    v = active_vessel(snap);
    CHECK(v != NULL);
    if (!v) return 1;
    CHECK(v->part_count == 153);
    CHECK(v->flags & KSPCORE_VESSEL_LOADED);
    printf("M2 on the pad: %u parts, altitude %.3f m, situation %d, %u terrain quads\n", v->part_count, v->altitude, v->situation, snap->quads_added_count);

    frames_total = settle + window;
    t_loop = clock();
    alt = (double*)calloc((size_t)window, sizeof(double));
    spd = (double*)calloc((size_t)window, sizeof(double));
    mach = (double*)calloc((size_t)window, sizeof(double));
    for (k = 0; k < frames_total; ++k) {
        kspcore_input_frame in;
        int32_t down[3];
        int32_t held[1];
        memset(&in, 0, sizeof in);
        in.struct_size = sizeof in;
        in.screen_width = 1280;
        in.screen_height = 720;
        in.has_focus = 1;
        in.real_time_since_startup = (float)k * 0.02f;
        if (k == settle) {
            /* what C10 did, through KSP's own bindings: full throttle (Z), SAS on (T), the first stage (Space) */
            down[0] = KEY_Z;
            down[1] = KEY_T;
            down[2] = KEY_SPACE;
            in.keys_down = down;
            in.keys_down_count = 3;
            held[0] = KEY_Z;
            in.keys_held = held;
            in.keys_held_count = 1;
        }
        CHECK(kspcore_input_set_frame(sim, &in) == 1);
        if (!kspcore_frame(sim, 0.02)) {
            printf("kspcore_frame failed at %d: %s\n", k, kspcore_last_error());
            ++failures;
            ok_flight = 0;
            break;
        }
        snap = kspcore_snapshot(sim);
        v = active_vessel(snap);
        if (!v) {
            printf("no active vessel at frame %d\n", k);
            ++failures;
            ok_flight = 0;
            break;
        }
        if (k >= settle) {
            alt[k - settle] = v->altitude;
            spd[k - settle] = (double)v->surface_speed;
            mach[k - settle] = (double)v->mach;
        }
        if (k == settle) CHECK(snap->frame.fixed_steps_last == 1);
        if (k % 1000 == 0 || k == settle + 1)
            printf("frame %d: alt %.1f m, spd %.1f m/s, mach %.3f, vertical %.1f m/s, parts %u, quads +%u -%u, UT %.2f\n", k, v->altitude,
                   (double)v->surface_speed, (double)v->mach, (double)v->vertical_speed, v->part_count, snap->quads_added_count,
                   snap->quads_removed_count, snap->frame.ut);
        if (snap->vessel_count != 1) {
            printf("vessel count %u at frame %d (a break or decouple?)\n", snap->vessel_count, k);
        }
    }

    printf("%d frames (kspcore_input_set_frame + kspcore_frame + kspcore_snapshot): %.3f ms per frame on average (CPU time)\n", frames_total,
           1000.0 * (double)(clock() - t_loop) / (double)CLOCKS_PER_SEC / (double)frames_total);
    if (ok_flight) {
        const double tol_spd = 0.01, tol_alt = 0.015, tol_frames = 0.025;
        int prev_ksp = -1, prev_ours = -1, n = 0;
        double max_alt_err = 0.0, max_spd_err = 0.0, max_frame_err = 0.0;
        for (i = 0; i < n_ksp; ++i) {
            const c10_state* s = &ksp[i];
            const int is_alt = strncmp(s->state, "alt_", 4) == 0, is_mach = strncmp(s->state, "mach_", 5) == 0;
            double threshold, d_spd, d_alt;
            int at = -1;
            if (!(is_alt || is_mach) || s->altitude > 21000.0) continue;
            threshold = is_alt ? atof(s->state + 4) : atof(s->state + 5);
            for (j = 0; j < window && at < 0; ++j)
                if ((is_alt ? alt[j] : mach[j]) >= threshold) at = j;
            if (at < 0) {
                printf("state %s not reached\n", s->state);
                ++failures;
                continue;
            }
            d_spd = spd[at] / s->spd - 1.0;
            d_alt = alt[at] / s->altitude - 1.0;
            printf("%s: KSP frame %d alt %.1f spd %.1f mach %.3f | ours frame %d alt %.1f spd %.1f mach %.3f | speed %+.3f %% altitude %+.3f %%\n", s->state,
                   s->frame, s->altitude, s->spd, s->mach, at, alt[at], spd[at], mach[at], d_spd * 100.0, d_alt * 100.0);
            if (is_alt) {
                CHECK(fabs(d_spd) < tol_spd);
                if (fabs(d_spd) > max_spd_err) max_spd_err = fabs(d_spd);
            } else {
                CHECK(fabs(d_alt) < tol_alt);
                if (fabs(d_alt) > max_alt_err) max_alt_err = fabs(d_alt);
            }
            if (prev_ksp >= 0 && s->frame - prev_ksp >= 100) {
                const double d_frames = (double)(at - prev_ours) / (double)(s->frame - prev_ksp) - 1.0;
                printf("    frames since the previous state: KSP %d, ours %d (%+.3f %%)\n", s->frame - prev_ksp, at - prev_ours, d_frames * 100.0);
                CHECK(fabs(d_frames) < tol_frames);
                if (fabs(d_frames) > max_frame_err) max_frame_err = fabs(d_frames);
            }
            prev_ksp = s->frame;
            prev_ours = at;
            ++n;
        }
        CHECK(n >= 3);
        for (i = 0; i < n_ksp; ++i) {
            const c10_state* s = &ksp[i];
            if (strcmp(s->state, "final") == 0 && prev_ksp >= 0) {
                const int jj = prev_ours + (s->frame - prev_ksp);
                double d_alt, d_spd;
                CHECK(jj >= 0 && jj < window);
                if (jj < 0 || jj >= window) continue;
                d_alt = alt[jj] / s->altitude - 1.0;
                d_spd = spd[jj] / s->spd - 1.0;
                printf("final (KSP frame %d): KSP alt %.1f spd %.1f | ours frame %d alt %.1f spd %.1f | altitude %+.3f %% speed %+.3f %%\n", s->frame,
                       s->altitude, s->spd, jj, alt[jj], spd[jj], d_alt * 100.0, d_spd * 100.0);
                CHECK(fabs(d_alt) < tol_alt);
                CHECK(fabs(d_spd) < tol_spd);
            }
        }
        printf("max |speed at altitude| %.3f %% (tol 1), max |altitude at Mach| %.3f %% (tol 1.5), max |frames| %.3f %% (tol 2.5)\n", max_spd_err * 100.0,
               max_alt_err * 100.0, max_frame_err * 100.0);
    }

    /* the final state hash for the C++ twin */
    snap = kspcore_snapshot(sim);
    {
        const uint64_t h = kspcore_snapshot_hash(snap);
        FILE* f = fopen(argv[7], "w");
        printf("STATE_HASH %016llx\n", (unsigned long long)h);
        CHECK(f != NULL);
        if (f) {
            fprintf(f, "%016llx\n", (unsigned long long)h);
            fclose(f);
        }
    }
    free(alt);
    free(spd);
    free(mach);

    /* settings surface */
    {
        const char* v0 = kspcore_settings_get("FLT_CAMERA_ORBIT_SENS");
        CHECK(v0 != NULL);
        CHECK(kspcore_settings_set("FLT_CAMERA_ORBIT_SENS", "0.5") == 1);
        v0 = kspcore_settings_get("FLT_CAMERA_ORBIT_SENS");
        CHECK(v0 != NULL && atof(v0) == 0.5);
        {
            /* settings.cfg round trip: save, change, load */
            char cfg_path[1024];
            snprintf(cfg_path, sizeof cfg_path, "%s/../settings_roundtrip.cfg", argv[4]);
            CHECK(kspcore_settings_save(cfg_path) == 1);
            CHECK(kspcore_settings_set("FLT_CAMERA_ORBIT_SENS", "0.25") == 1);
            CHECK(kspcore_settings_load(cfg_path) == 1);
            v0 = kspcore_settings_get("FLT_CAMERA_ORBIT_SENS");
            CHECK(v0 != NULL && atof(v0) == 0.5);
        }
        CHECK(kspcore_settings_get("NO_SUCH_SETTING") == NULL);
        CHECK(kspcore_settings_set("NO_SUCH_SETTING", "1") == 0);
    }

    /* quicksave / quickload and the save list */
    CHECK(kspcore_game_quicksave(sim) == 1);
    CHECK(kspcore_game_has_quicksave(sim) == 1);
    {
        char names[256];
        const size_t need = kspcore_game_list_saves(sim, "flight_slice", names, sizeof names);
        CHECK(need > 0 && strstr(names, "quicksave") != NULL && strstr(names, "persistent") != NULL);
    }

    /* destroy and re-create: a second sim in the same process flies (the hygiene resets are checked by the C++ twin) */
    kspcore_sim_destroy(sim);
    sim = start_flight(argv[5], "flight_slice2");
    if (sim) {
        for (k = 0; k < 150; ++k) CHECK(kspcore_frame(sim, 0.02) == 1);
        snap = kspcore_snapshot(sim);
        v = active_vessel(snap);
        CHECK(v != NULL && v->part_count == 153);
        kspcore_sim_destroy(sim);
    }
    kspcore_game_shutdown();

    if (failures) {
        printf("%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("capi.flight_slice OK\n");
    return 0;
}
