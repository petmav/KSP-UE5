/*
 * kspcore C ABI -- the only interface the Unreal shell (ue5/Plugins/KspCoreBridge) compiles against.
 *
 * Rules (Unreal compiles with /GR-, _HAS_EXCEPTIONS=0 and, for Game targets, /fp:fast):
 *   - only plain C types cross this boundary: no STL types, no exceptions, no RTTI;
 *   - no function lets a C++ exception escape: failures return NULL / 0 / NaN and set a thread-local error message
 *     (kspcore_last_error);
 *   - all strings are UTF-8, NUL-terminated; returned `const char*` pointers are owned by kspcore and stay valid until
 *     the object they came from is modified or released (or, for kspcore_last_error, until the next failing call on
 *     the same thread);
 *   - the numerics run inside kspcore.lib with kspcore's own compiler flags (/fp:precise); nothing numerically
 *     sensitive is inline in this header, so the consumer's /fp:fast cannot change results.
 *
 * Handles are opaque. Every *_create / *_load / *_parse / *_get_* function that returns a handle transfers ownership of
 * that handle to the caller, who releases it with the matching *_destroy / *_release function. A ConfigNode handle
 * keeps the whole tree it belongs to alive.
 *
 * ABI history: 1 = build info, sim context, ConfigNode (load/parse/save/query), FloatCurve (WP-01/02).
 */
#pragma once

#include <stddef.h>

#define KSPCORE_ABI_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

/* ---- build / version ------------------------------------------------------------------------------------------- */

/* Static string describing the build, e.g.
 * "kspcore abi=1 version=0.1.0 msc=1944 crt=MD idl=0 lang=202002 runtime=Mono libm=VS2013 CRT (msvcr120_clr0400.dll)".
 * The bridge checks "crt=MD " and "idl=0". Never freed. */
const char* kspcore_build_info(void);
/* "0.1.0" */
const char* kspcore_version(void);
/* KSPCORE_ABI_VERSION the library was built with */
int kspcore_abi_version(void);
/* Message of the last failed call on this thread ("" if none). */
const char* kspcore_last_error(void);

/* ---- simulation context -------------------------------------------------------------------------------------------- */

typedef struct kspcore_sim kspcore_sim;

/* Log levels passed to the log callback. */
#define KSPCORE_LOG_INFO 0
#define KSPCORE_LOG_WARNING 1
#define KSPCORE_LOG_ERROR 2
typedef void (*kspcore_log_fn)(int level, const char* message, void* user);

/* Creates the simulation context (will own the game state as work packages land). NULL on failure. */
kspcore_sim* kspcore_sim_create(void);
void kspcore_sim_destroy(kspcore_sim* sim);
/* Routes KSP's Debug.Log/LogWarning/LogError equivalents to `fn` (NULL to silence). Process-wide. */
void kspcore_set_log_callback(kspcore_log_fn fn, void* user);

/* ---- ConfigNode (WP-02) --------------------------------------------------------------------------------------------- */

typedef struct kspcore_confignode kspcore_confignode;

/* ConfigNode.Load(path): the file's tree under a node named "root". NULL if the file is missing or holds no data. */
kspcore_confignode* kspcore_confignode_load(const char* utf8_path);
/* ConfigNode.Parse(text). Never NULL for valid input (an empty text gives an empty "root"). */
kspcore_confignode* kspcore_confignode_parse(const char* utf8_text);
/* New empty node (ConfigNode(name)). */
kspcore_confignode* kspcore_confignode_create(const char* name);
void kspcore_confignode_release(kspcore_confignode* node);

const char* kspcore_confignode_name(const kspcore_confignode* node);
const char* kspcore_confignode_id(const kspcore_confignode* node);
int kspcore_confignode_value_count(const kspcore_confignode* node);
const char* kspcore_confignode_value_name(const kspcore_confignode* node, int index);
const char* kspcore_confignode_value_at(const kspcore_confignode* node, int index);
/* GetValue(name) / GetValue(name, index): NULL if there is no such value. */
const char* kspcore_confignode_get_value(const kspcore_confignode* node, const char* name, int index);
/* SetValue(name, value, createIfNotFound); returns 1 if an existing value was set (KSP returns 0 when it creates). */
int kspcore_confignode_set_value(kspcore_confignode* node, const char* name, const char* value, int create_if_not_found);
void kspcore_confignode_add_value(kspcore_confignode* node, const char* name, const char* value);
int kspcore_confignode_node_count(const kspcore_confignode* node);
/* Child by position / GetNode(name, index); returns a NEW handle (release it) or NULL. */
kspcore_confignode* kspcore_confignode_child_at(const kspcore_confignode* node, int index);
kspcore_confignode* kspcore_confignode_get_node(const kspcore_confignode* node, const char* name, int index);
/* AddNode(name); returns a NEW handle to the added child, or NULL for a blank name. */
kspcore_confignode* kspcore_confignode_add_node(kspcore_confignode* node, const char* name);
/* Typed getters with KSP's parsing rules (TryGetValue); return 1 and write *out on success, 0 otherwise. */
int kspcore_confignode_try_get_double(const kspcore_confignode* node, const char* name, double* out);
int kspcore_confignode_try_get_float(const kspcore_confignode* node, const char* name, float* out);
int kspcore_confignode_try_get_int(const kspcore_confignode* node, const char* name, int* out);
int kspcore_confignode_try_get_bool(const kspcore_confignode* node, const char* name, int* out);
int kspcore_confignode_try_get_vector3d(const kspcore_confignode* node, const char* name, double out_xyz[3]);
/* Save text (CRLF, UTF-8 without BOM, no root wrapper): writes up to `capacity` bytes (NUL-terminated when it fits)
 * and returns the full length in bytes (excluding the NUL). Call with buffer NULL to query the size. */
size_t kspcore_confignode_save_to_buffer(const kspcore_confignode* node, char* buffer, size_t capacity);
/* ConfigNode.Save(path). 1 on success. */
int kspcore_confignode_save(const kspcore_confignode* node, const char* utf8_path);
/* ToString() text ('\n' line ends, root wrapper and node ids included); same buffer protocol as above. */
size_t kspcore_confignode_to_string(const kspcore_confignode* node, char* buffer, size_t capacity);

/* ---- FloatCurve (WP-01) --------------------------------------------------------------------------------------------- */

typedef struct kspcore_floatcurve kspcore_floatcurve;

kspcore_floatcurve* kspcore_floatcurve_create(void);
/* FloatCurve.Load(node) over the node's `key` values; NULL (and kspcore_last_error) if a key is malformed. */
kspcore_floatcurve* kspcore_floatcurve_load(const kspcore_confignode* node);
void kspcore_floatcurve_destroy(kspcore_floatcurve* curve);
/* FloatCurve.Add(time, value): Unity smooth tangents on the key and its neighbours */
void kspcore_floatcurve_add(kspcore_floatcurve* curve, float time, float value);
/* FloatCurve.Add(time, value, inTangent, outTangent) */
void kspcore_floatcurve_add_tangents(kspcore_floatcurve* curve, float time, float value, float in_tangent, float out_tangent);
/* FloatCurve.Evaluate(time): bit-exact with the game */
float kspcore_floatcurve_evaluate(const kspcore_floatcurve* curve, float time);
int kspcore_floatcurve_key_count(const kspcore_floatcurve* curve);
float kspcore_floatcurve_min_time(const kspcore_floatcurve* curve);
float kspcore_floatcurve_max_time(const kspcore_floatcurve* curve);

/* ---- PQS terrain quads (WP-12a; additive, ABI version unchanged: KSPCORE_ABI_TERRAIN marks it) ----------------------
 *
 * A stock sphere (the game's prefab mod stack, WP-11 heights) with its own quad pool, driven like KSP's PQS in flight:
 * the target is the active vessel (or camera) position in the body-fixed frame (metres from the body centre, the
 * sphere's local axes; no floating origin). Each update returns the quads to draw; their data stays valid until the
 * next kspcore_pqs_update / kspcore_pqs_destroy. Positions are floats relative to the quad's double `origin`
 * (camera-relative rendering: subtract the camera's double position from origin, then add the float positions). */
#define KSPCORE_ABI_TERRAIN 1

typedef struct kspcore_pqs kspcore_pqs;

typedef struct kspcore_pqs_quad {
    int id;                 /* PQ.id (unique per sphere start) */
    int subdivision;        /* quadtree level */
    int vertex_count;       /* 225 (PQS cacheSideVertCount^2) */
    int index_count;        /* triangle indices (1176): the edge-lerp index set of this quad */
    double origin[3];       /* the quad's local origin (body-fixed, metres) */
    const float* positions; /* vertex_count x 3, relative to origin */
    const float* normals;   /* vertex_count x 3 */
    const float* tangents;  /* vertex_count x 4 (NULL if the stack builds none) */
    const float* uv;        /* vertex_count x 2 (NULL if never assigned) */
    const float* uv2;
    const float* uv3;
    const float* uv4;
    const float* colors;    /* vertex_count x 4 (r, g, b, a) */
    const int* indices;     /* index_count */
    int collider;           /* 1: PQSMod_QuadMeshColliders gave this quad a MeshCollider (WP-12b) */
} kspcore_pqs_quad;

/* `body_name` e.g. "Kerbin"; `terrain_data_dir` = <core>/data/terrain; `height_threads` > 0 evaluates the height pass
 * of quads about to be built on that many worker threads (bit-identical results). NULL on failure. */
kspcore_pqs* kspcore_pqs_create_stock(const char* body_name, const char* terrain_data_dir, int height_threads);
void kspcore_pqs_destroy(kspcore_pqs* pqs);
void kspcore_pqs_set_target(kspcore_pqs* pqs, const double position_xyz[3]);
/* The first call starts the sphere (PQS.Start); every call then runs `fixed_updates` physics ticks (PQS.FixedUpdate)
 * and one frame (the UpdateSphere coroutine) and collects the quads to draw. Returns their number, -1 on failure. */
int kspcore_pqs_update(kspcore_pqs* pqs, int fixed_updates);
/* Quad `index` (0 .. count-1) of the last update; 1 on success. */
int kspcore_pqs_get_quad(const kspcore_pqs* pqs, int index, kspcore_pqs_quad* out);
/* PQS.radius, PQS.isActive, and the planet fade value PQSMod_CelestialBodyTransform applies to the surface material */
double kspcore_pqs_radius(const kspcore_pqs* pqs);
int kspcore_pqs_is_active(const kspcore_pqs* pqs);
float kspcore_pqs_planet_fade(const kspcore_pqs* pqs);

#ifdef __cplusplus
}
#endif
