/*
 * kspcore C ABI v2, domain header "frame": the per-frame bridge (ue5/DESIGN.md 1.3, decisions D3, D6, D12). WP-UE-02.
 *
 * One call sequence per rendered frame, on the game thread, before cameras and rendering:
 *
 *     kspcore_input_set_frame(sim, &input)      keys, mouse, axes, text, focus (Unity KeyCode numbers)
 *     kspcore_commands_submit(sim, cmds, n)     UI-originated actions, applied at the start of this frame's Update phase
 *     kspcore_frame(sim, dt_real)               core's PlayerLoop.Frame: n fixed steps (Time.fixedDeltaTime x warp, at most
 *                                               maximumDeltaTime per frame), then Update (commands), coroutines, LateUpdate
 *     snap = kspcore_snapshot(sim)              immutable view of the frame just run
 *
 * Snapshot rules:
 *   - Every position is a double RELATIVE TO THE CENTRE OF THE DOMINANT BODY (core's world position minus
 *     CelestialBody.position). That is invariant under core's own floating-origin rebases; the shell keeps its own render anchor.
 *   - The axes are core's world axes: Unity's left-handed Y-up axes, body-fixed while `frame_kind` is KSPCORE_FRAME_ROTATING and
 *     inertial otherwise. A frame change (rotating <-> inertial, SOI change) is an event carrying the old-to-new transform.
 *   - Rotations are quaternions (x, y, z, w) of Unity's convention.
 *   - Every array and `const char*` in the snapshot is owned by core and valid until the next kspcore_frame / kspcore_sim_destroy.
 *   - No callback from core into the shell happens during kspcore_frame.
 *
 * Every struct has a leading `uint32_t struct_size` (set by the producer to sizeof of its version); a consumer written against an
 * older header reads the fields it knows. Fields are only ever appended. Slots marked "reserved" are present and zero until the
 * package that owns them lands (camera: WP-UE-03, view-models: WP-UE-04, model handles: WP-UE-05, FX: WP-UE-16).
 */
#ifndef KSPCORE_FRAME_H
#define KSPCORE_FRAME_H

#include <stddef.h>
#include <stdint.h>

#include "kspcore_c.h"

#define KSPCORE_FRAME_ABI 1

#ifdef __cplusplus
extern "C" {
#endif

/* ---- input (D12: KSP's own bindings drive input; the shell reports raw device state) -------------------------------------- */

/* Mouse buttons: bit 0 left, bit 1 right, bit 2 middle, bits 3.. further buttons (Input.GetMouseButton index = bit index). */
typedef struct kspcore_axis_value {
    const char* name; /* Unity input-manager axis name: "Mouse X", "Mouse Y", "Mouse ScrollWheel", "Joystick1 Axis 1" ... */
    float value;
} kspcore_axis_value;

typedef struct kspcore_input_frame {
    uint32_t struct_size;
    /* UnityEngine.KeyCode numbers (326 values; mouse buttons and joystick buttons are KeyCodes too). */
    const int32_t* keys_down; /* went down since the last frame (Input.GetKeyDown) */
    uint32_t keys_down_count;
    const int32_t* keys_up; /* went up since the last frame (Input.GetKeyUp) */
    uint32_t keys_up_count;
    const int32_t* keys_held; /* currently down (Input.GetKey) */
    uint32_t keys_held_count;
    const kspcore_axis_value* axes; /* Input.GetAxis(name); an axis not listed reads 0 */
    uint32_t axis_count;
    double mouse_position[2]; /* pixels, origin bottom-left as Unity (Input.mousePosition) */
    float mouse_delta[2];
    uint32_t mouse_buttons_held;
    uint32_t mouse_buttons_down;
    uint32_t mouse_buttons_up;
    float mouse_wheel;
    const char* text_utf8; /* characters typed this frame (Input.inputString), NULL or "" for none */
    int32_t screen_width;
    int32_t screen_height;
    int32_t has_focus;      /* Application focus (FlightInputHandler.OnApplicationFocus) */
    int32_t ui_hover_lock;  /* the pointer is over shell UI: mouse input is not delivered to flight controls */
    float real_time_since_startup; /* Time.realtimeSinceStartup */
} kspcore_input_frame;

/* Stores the input for the next kspcore_frame. The pointers need only live until this call returns (core copies). 1 on success. */
int kspcore_input_set_frame(kspcore_sim* sim, const kspcore_input_frame* input);

/* ---- commands (UI actions; most controls are keys through the input device) ----------------------------------------------- */

#define KSPCORE_CMD_STAGE 1               /* StageManager.ActivateNextStage */
#define KSPCORE_CMD_SAS_ENABLE 2          /* i0: 0 off, 1 on (VesselAutopilot.SetMode / Enable) */
#define KSPCORE_CMD_SAS_MODE 3            /* i0: VesselAutopilot.AutopilotMode (0 StabilityAssist .. 9 Maneuver) */
#define KSPCORE_CMD_THROTTLE 4            /* d0: 0..1 (FlightInputHandler.state.mainThrottle) */
#define KSPCORE_CMD_TIME_WARP_INDEX 5     /* i0: rate index, i1: 0 = on-rails (high) / 1 = physics (low) mode */
#define KSPCORE_CMD_ACTION_GROUP 6        /* i0: KSPActionGroup bit value to toggle */
#define KSPCORE_CMD_SET_ACTIVE_VESSEL 7   /* i0: vessel id of the snapshot */
#define KSPCORE_CMD_MAP_VIEW 8            /* i0: 1 enter, 0 leave (MapView.EnterMapView / ExitMapView) */
#define KSPCORE_CMD_QUICKSAVE 9
#define KSPCORE_CMD_QUICKLOAD 10
#define KSPCORE_CMD_REVERT_TO_LAUNCH 11
#define KSPCORE_CMD_LOAD_SCENE 12         /* i0: GameScenes (quit to the main menu, space centre ...) */
#define KSPCORE_CMD_PAUSE 13              /* i0: 1 pause, 0 resume (FlightDriver.SetPause) */
#define KSPCORE_CMD_PART_EVENT 14         /* i0: part id, text: event name (Part Action Window buttons) */

typedef struct kspcore_command {
    uint32_t struct_size;
    int32_t kind; /* KSPCORE_CMD_* */
    int32_t i0;
    int32_t i1;
    double d0;
    double d1;
    const char* text; /* NULL when unused; copied by kspcore_commands_submit */
} kspcore_command;

/* Queues `count` commands; they are applied in order at the start of the next frame's Update phase. Returns the number queued. */
size_t kspcore_commands_submit(kspcore_sim* sim, const kspcore_command* commands, size_t count);

/* ---- the frame ------------------------------------------------------------------------------------------------------------ */

/* core's PlayerLoop.Frame (WP-00). `dt_real` = the frame's real delta in seconds (converted to the float Time.deltaTime inside).
 * Core owns the fixed-step accumulator and all of Time.*; the shell never steps physics. 1 on success, 0 on failure
 * (kspcore_last_error). Does nothing (returns 1) while no scene is running. */
int kspcore_frame(kspcore_sim* sim, double dt_real);

/* ---- snapshot ------------------------------------------------------------------------------------------------------------- */

#define KSPCORE_FRAME_ROTATING 0 /* world axes body-fixed (below the rotating-frame threshold) */
#define KSPCORE_FRAME_INERTIAL 1

typedef struct kspcore_frame_state {
    uint32_t struct_size;
    double ut;                    /* Planetarium.GetUniversalTime */
    uint64_t frame_count;         /* kspcore_frame calls since the scene started */
    uint64_t fixed_steps;         /* fixed steps run since the scene started */
    uint32_t fixed_steps_last;    /* fixed steps of the last frame (0, 1 or 2) */
    float warp_rate;              /* TimeWarp.CurrentRate */
    int32_t warp_index;           /* TimeWarp.CurrentRateIndex */
    int32_t warp_mode;            /* 0 = rails (high), 1 = physics (low) */
    int32_t paused;               /* FlightDriver.Pause */
    int32_t scene;                /* GameScenes of core's HighLogic.LoadedScene */
    int32_t frame_kind;           /* KSPCORE_FRAME_* */
    int32_t dominant_body_id;     /* CelestialBody.flightGlobalsIndex of the bubble's dominant body, -1 without a flight scene */
    uint32_t frame_change_serial; /* increments at every frame change event */
    int32_t active_vessel_id;     /* vessel id of the active vessel, 0 when none */
    float fixed_delta_time;       /* Time.fixedDeltaTime of the last frame */
    float reserved0;
} kspcore_frame_state;

#define KSPCORE_BODY_PQS_ACTIVE 1
#define KSPCORE_BODY_SCALED_VISIBLE 2
#define KSPCORE_BODY_DOMINANT 4

typedef struct kspcore_body_state {
    uint32_t struct_size;
    int32_t id;          /* CelestialBody.flightGlobalsIndex */
    const char* name;
    uint32_t flags;      /* KSPCORE_BODY_* */
    float planet_fade;   /* kspcore_pqs_planet_fade of the body's sphere (1 = scaled version fully faded in), 1 without a sphere */
    double position[3];  /* relative to the dominant body's centre (0 for the dominant body) */
    double rotation[4];  /* the body's rotation in core's world axes (x, y, z, w) */
    double radius;
} kspcore_body_state;

typedef struct kspcore_sun_state {
    uint32_t struct_size;
    double direction[3];              /* unit vector from the dominant body's centre to the sun, core's world axes */
    double planetarium_rotation[4];   /* Planetarium.Rotation: the skybox turns with it in the rotating frame (x, y, z, w) */
    float color[4];                   /* reserved: zero (the sun light colour is shell material data, not core state) */
} kspcore_sun_state;

#define KSPCORE_VESSEL_LOADED 1
#define KSPCORE_VESSEL_PACKED 2
#define KSPCORE_VESSEL_ACTIVE 4
#define KSPCORE_VESSEL_LANDED 8
#define KSPCORE_VESSEL_SPLASHED 16

typedef struct kspcore_vessel_state {
    uint32_t struct_size;
    int32_t id;          /* Vessel.persistentId reinterpreted (stable across frames and quicksave / quickload) */
    const char* name;
    uint32_t flags;      /* KSPCORE_VESSEL_* */
    int32_t situation;   /* Vessel.Situations (LANDED 1, SPLASHED 2, PRELAUNCH 4, FLYING 8, SUB_ORBITAL 16, ORBITING 32, ESCAPING 64, DOCKED 128) */
    double com[3];       /* Vessel.CoMD relative to the dominant body */
    double root_position[3];
    float root_rotation[4];
    uint32_t first_part; /* index into parts[] of the vessel's first part */
    uint32_t part_count; /* parts[first_part .. first_part + part_count) belong to this vessel (loaded vessels only) */
    /* Appended after the header freeze (additive, WP-UE-02): the flight quantities the C10 parity gate and the HUD read; 0 for a vessel
     * without a FlightIntegrator (not loaded). */
    double altitude;       /* the root part's altitude above sea level: |root_position| - radius of the dominant body */
    float surface_speed;   /* FlightIntegrator atm.spd (m/s, relative to the rotating atmosphere) */
    float mach;            /* FlightIntegrator atm.mach */
    float vertical_speed;  /* Vessel.verticalSpeed (m/s) */
    float reserved1;
} kspcore_vessel_state;

typedef struct kspcore_part_state {
    uint32_t struct_size;
    uint32_t id;          /* Part.persistentId (stable) */
    int32_t vessel_id;
    uint32_t model_id;    /* kspcore_game_model_name(model_id) = the part's AvailablePart name; WP-UE-05 keys model handles by it */
    uint32_t flags;       /* bit 0: dead */
    double position[3];   /* relative to the dominant body's centre (the part transform's position) */
    float rotation[4];
    float highlight[4];   /* reserved: zero */
    float thermal_glow;   /* reserved: zero */
    float reserved0;
} kspcore_part_state;

/* A re-posed local transform below a part (a gimbal, an animation, a wheel): sparse list. Reserved until WP-UE-16 / the animation
 * packages: node_count is 0 now. */
typedef struct kspcore_node_pose {
    uint32_t struct_size;
    uint32_t part_id;
    const char* node_name;
    float local_position[3];
    float local_rotation[4];
} kspcore_node_pose;

/* Sparse FX parameter (engine power, throttle-driven scale / volume / pitch). Reserved until WP-UE-16: fx_count is 0 now. */
typedef struct kspcore_fx_param {
    uint32_t struct_size;
    uint32_t part_id;
    const char* name;
    float value;
} kspcore_fx_param;

/* Terrain changes of one sphere since the last frame. `added` quads are new or rebuilt (a rebuilt quad has the same id and a new
 * `build_serial`); positions are floats relative to the quad's double origin (body-fixed sphere axes). */
typedef struct kspcore_terrain_quad {
    kspcore_pqs_quad quad;     /* kspcore_c.h; pointers valid until the next kspcore_frame */
    uint32_t build_serial;
    int32_t body_id;
} kspcore_terrain_quad;

typedef struct kspcore_terrain_removed {
    int32_t body_id;
    int32_t quad_id;
} kspcore_terrain_removed;

/* Camera pose and mode: reserved for WP-UE-03 (FlightCamera, PlanetariumCamera). `valid` is 0 now. */
typedef struct kspcore_camera_state {
    uint32_t struct_size;
    int32_t valid;
    int32_t mode;           /* reserved */
    double position[3];     /* relative to the dominant body's centre */
    double rotation[4];
    float field_of_view;    /* degrees */
    float near_clip;
    int32_t map_valid;      /* map camera, reserved */
    double map_position[3];
    double map_rotation[4];
} kspcore_camera_state;

/* Events of the frame (since the previous kspcore_frame). a / b / id / d are the payload named per kind. */
#define KSPCORE_EVENT_SCENE_LOAD_REQUESTED 1 /* a = GameScenes requested (HighLogic.LoadScene: shell builds, then kspcore_scene_loaded) */
#define KSPCORE_EVENT_VESSEL_CREATED 2       /* id = vessel id */
#define KSPCORE_EVENT_VESSEL_DESTROYED 3     /* id = vessel id */
#define KSPCORE_EVENT_VESSEL_SWITCHED 4      /* id = new active vessel id, a = previous vessel id */
#define KSPCORE_EVENT_PART_DIED 5            /* id = part id, a = vessel id (FX, audio) */
#define KSPCORE_EVENT_PART_EXPLODED 6        /* id = part id, a = vessel id, d[0..2] = position relative to the dominant body */
#define KSPCORE_EVENT_STRUCTURE_CHANGED 7    /* id = vessel id (decouple, dock, part lost): the shell rebuilds its part actors */
#define KSPCORE_EVENT_FRAME_CHANGE 8         /* a = old kind, b = new kind, d[0..2] = old-to-new origin offset, d[3..6] = rotation (x y z w) */
#define KSPCORE_EVENT_FLOATING_ORIGIN_REBASE 9 /* informational, d[0..2] = the offset core moved its world by */
#define KSPCORE_EVENT_SCREEN_MESSAGE 10      /* text, d[0] = duration seconds, a = position (ScreenMessageStyle) */
#define KSPCORE_EVENT_SOUND_CUE 11           /* text = cue name, id = part id, d[0] = volume */
#define KSPCORE_EVENT_SCENE_LOADED 12        /* a = GameScenes core finished starting */

typedef struct kspcore_event {
    uint32_t struct_size;
    int32_t kind; /* KSPCORE_EVENT_* */
    int32_t a;
    int32_t b;
    int32_t id;
    double d[7];
    const char* text; /* NULL when unused */
} kspcore_event;

typedef struct kspcore_snapshot_view {
    uint32_t struct_size;
    kspcore_frame_state frame;
    kspcore_sun_state sun;
    kspcore_camera_state camera;
    const kspcore_body_state* bodies;
    uint32_t body_count;
    const kspcore_vessel_state* vessels;
    uint32_t vessel_count;
    const kspcore_part_state* parts;
    uint32_t part_count;
    const kspcore_node_pose* nodes;
    uint32_t node_count;
    const kspcore_fx_param* fx;
    uint32_t fx_count;
    const kspcore_terrain_quad* quads_added;
    uint32_t quads_added_count;
    const kspcore_terrain_removed* quads_removed;
    uint32_t quads_removed_count;
    const kspcore_event* events;
    uint32_t event_count;
} kspcore_snapshot_view;

/* The snapshot of the frame just run (the state before the first frame when no frame ran). NULL without a sim. Not a copy: valid
 * until the next kspcore_frame. */
const kspcore_snapshot_view* kspcore_snapshot(kspcore_sim* sim);

/* State hash for the parity gates (DESIGN 1.5): FNV-1a 64 over the bytes of the snapshot's frame (ut, frame_count, fixed_steps),
 * then every body (id, position, rotation), vessel (flags, situation, com, root pose) and part (model_id, position, rotation) in list
 * order. Vessel and part ids, pointers, names and quad / event data are not hashed (ids come from process-wide counters that differ
 * between two sims of one process). The shell and the headless ctest compute it the same way. [doc corrected after the freeze: the first
 * text listed the ids; see core/PROGRESS-wpue02.md] */
uint64_t kspcore_snapshot_hash(const kspcore_snapshot_view* snapshot);

#ifdef __cplusplus
}
#endif

#endif /* KSPCORE_FRAME_H */
