/*
 * kspcore C ABI, model domain (WP-UE-05): the render side of KSP's `.mu` part models and the texture headers. Header only for the
 * Unreal shell (UKspModelCache); pure C (compiles as C and C++), no STL across the boundary.
 *
 * Same rules as kspcore_c.h: plain C types, no exceptions escape, UTF-8 NUL-terminated strings, no inline maths. Failures return
 * NULL / 0 and set the model domain's own thread-local message (kspcore_model_last_error): kspcore_c.cpp is not touched by this
 * package, so this domain keeps its own error slot. Every pointer returned by an accessor points into the model handle and is
 * valid until kspcore_model_release; the shell copies what it keeps (one UStaticMesh per unique mesh).
 *
 * What a model is: PartReader.cs's `.mu` (decompiled/KSP-Decompiled/Assembly-CSharp/PartReader.cs). A tree of transforms in the
 * file's preorder (node 0 = the root; a node's children follow it in order) and component chunks on the nodes. The component tables
 * below (meshes, renderers, lights, cameras, animations, clips) are flat, ordered by node id then chunk order. Materials are indexed
 * by the material slots the renderers list; textures by the texture ids the material properties hold (-1 = none).
 *
 * Struct protocol: every info struct starts with `uint32_t struct_size`; the caller sets it to sizeof(the struct it compiled
 * against) before the call, kspcore fills the fields that fit in that size (older callers keep working when fields are appended).
 *
 * Units and layout: Unity's. Positions are metres in the parent's frame, rotations are quaternions (x, y, z, w), left-handed
 * Y-up. Triangle lists are Unity winding (clockwise seen from the front). UVs are Unity's (v up). Nothing here is converted to Unreal axes.
 *
 * ABI history: 1 = first version (WP-UE-05).
 */
#ifndef KSPCORE_MODEL_H
#define KSPCORE_MODEL_H

#include <stddef.h>
#include <stdint.h>

#define KSPCORE_ABI_MODEL 1

#ifdef __cplusplus
extern "C" {
#endif

/* ---- model handle ---------------------------------------------------------------------------------------------------- */

typedef struct kspcore_model kspcore_model;

/* Message of the last failed model call on this thread ("" if none). */
const char* kspcore_model_last_error(void);

/* PartReader.Read: parses the `.mu` file / buffer. `parent_url` is the GameDatabase url of the directory the file is in
 * (e.g. "Squad/Parts/Engine/liquidEngine"): the texture urls are `<parent_url>/<texture name>` (PartReader.cs:397); NULL = bare
 * names. NULL + kspcore_model_last_error on a missing file, a wrong file type or a malformed / truncated file. */
kspcore_model* kspcore_model_load_file(const char* utf8_path, const char* parent_url);
kspcore_model* kspcore_model_load_memory(const void* data, size_t size, const char* parent_url);
void kspcore_model_release(kspcore_model* model);

typedef struct kspcore_model_info {
    uint32_t struct_size;
    int32_t file_type;  /* 76543 */
    int32_t version;    /* .mu file version (0 to 5 in the stock game) */
    const char* header; /* the string after the version (unused by KSP) */
    int32_t node_count, mesh_count, renderer_count, material_count, texture_count;
    int32_t light_count, camera_count, particle_emitter_count; /* particle emitters: count only, their values are not kept */
    int32_t animation_count, clip_count;                       /* Animation components / legacy clips */
    int32_t collider_count;                                    /* collider chunks (kept by core, not exposed here) */
    int32_t materials_chunk_count, textures_chunk_count;
    int32_t warning_count;
    int32_t ignored_entry_count;      /* distinct node-level entry values PartReader has no case for */
    int32_t ignored_mesh_entry_count; /* the same inside meshes */
} kspcore_model_info;
int kspcore_model_get_info(const kspcore_model* model, kspcore_model_info* out);

/* Things PartReader would have thrown on that were accepted (e.g. a Materials chunk longer than the referenced slots). */
const char* kspcore_model_warning(const kspcore_model* model, int index);
/* Entry values the reader consumed nothing for: value and how often. 1 on success. */
int kspcore_model_ignored_entry(const kspcore_model* model, int index, int32_t* value, int32_t* occurrences);
int kspcore_model_ignored_mesh_entry(const kspcore_model* model, int index, int32_t* value, int32_t* occurrences);

/* ---- node tree ------------------------------------------------------------------------------------------------------- */

typedef struct kspcore_model_node {
    uint32_t struct_size;
    const char* name;
    int32_t parent; /* -1 for the root */
    int32_t child_count;
    float position[3];
    float rotation[4]; /* x, y, z, w as stored (not normalised) */
    float scale[3];
    int32_t has_tag_and_layer; /* a TagAndLayer chunk was present */
    const char* tag;           /* "Untagged" when absent */
    int32_t layer;
    int32_t mesh_filter_count, mesh_renderer_count, skinned_renderer_count;
    int32_t collider_count, animation_count, light_count, camera_count, particle_emitter_count;
} kspcore_model_node;
int kspcore_model_get_node(const kspcore_model* model, int node, kspcore_model_node* out);
/* Node id of child `k` (0 .. child_count-1) of `node`, -1 if out of range. */
int kspcore_model_node_child(const kspcore_model* model, int node, int k);

/* ---- meshes (MeshFilter and SkinnedMeshRenderer meshes) -------------------------------------------------------------- */

#define KSPCORE_MESH_HAS_VERTICES (1 << 0)
#define KSPCORE_MESH_HAS_NORMALS (1 << 1)
#define KSPCORE_MESH_HAS_TANGENTS (1 << 2)
#define KSPCORE_MESH_HAS_UV (1 << 3)
#define KSPCORE_MESH_HAS_UV2 (1 << 4)
#define KSPCORE_MESH_HAS_COLORS (1 << 5)
#define KSPCORE_MESH_HAS_BONE_WEIGHTS (1 << 6)
#define KSPCORE_MESH_HAS_BIND_POSES (1 << 7)

#define KSPCORE_MESH_SOURCE_FILTER 0
#define KSPCORE_MESH_SOURCE_SKINNED 1

typedef struct kspcore_model_mesh {
    uint32_t struct_size;
    int32_t node;   /* the node that owns the MeshFilter / SkinnedMeshRenderer */
    int32_t source; /* KSPCORE_MESH_SOURCE_* */
    int32_t vertex_count;
    int32_t header_int; /* the second int of the mesh header (read and dropped by PartReader) */
    uint32_t flags;     /* KSPCORE_MESH_HAS_*: which arrays the file carried (a pointer is NULL when its flag is clear or the array is empty) */
    const float* vertices;  /* 3 per vertex */
    const float* normals;   /* 3 */
    const float* tangents;  /* 4 (x, y, z, w) */
    const float* uv;        /* 2 */
    const float* uv2;       /* 2 */
    const uint8_t* colors32; /* 4 per vertex: r, g, b, a (Color32) */
    int32_t submesh_count;   /* one triangle list per MeshTriangles chunk, in order */
    const int32_t* bone_indices; /* 4 per vertex (boneIndex0..3) */
    const float* bone_weights;   /* 4 per vertex (weight0..3) */
    int32_t bind_pose_count;
    const float* bind_poses; /* 16 per pose, Matrix4x4 m00 m01 m02 m03 m10 ... m33 */
} kspcore_model_mesh;
int kspcore_model_get_mesh(const kspcore_model* model, int mesh, kspcore_model_mesh* out);
/* Triangle list `sub` of a mesh (indices into the vertices, 3 per triangle); *index_count receives the length. NULL if out of range. */
const int32_t* kspcore_model_mesh_submesh(const kspcore_model* model, int mesh, int sub, int32_t* index_count);

/* ---- renderers ------------------------------------------------------------------------------------------------------- */

#define KSPCORE_RENDERER_MESH 0
#define KSPCORE_RENDERER_SKINNED 1

typedef struct kspcore_model_renderer {
    uint32_t struct_size;
    int32_t node;
    int32_t kind; /* KSPCORE_RENDERER_* */
    /* index into the mesh table: a MeshRenderer draws the first MeshFilter mesh of its node (-1 if none), a skinned renderer its own */
    int32_t mesh;
    int32_t has_shadow_fields; /* fileVersion >= 1 stores the two flags; otherwise the Unity defaults below */
    int32_t cast_shadows;      /* 1 = ShadowCastingMode.On (default) */
    int32_t receive_shadows;   /* 1 (default) */
    int32_t material_slot_count;
    const int32_t* material_slots; /* material ids (index into the material table), one per submesh */
    float local_bounds[6];         /* skinned: centre then size */
    int32_t quality;               /* SkinQuality */
    int32_t update_when_offscreen;
    int32_t bone_count;
    const int32_t* bone_nodes; /* node id per bone name (first node with that name from the root), -1 if not found */
} kspcore_model_renderer;
int kspcore_model_get_renderer(const kspcore_model* model, int renderer, kspcore_model_renderer* out);
const char* kspcore_model_renderer_bone_name(const kspcore_model* model, int renderer, int bone);

/* ---- materials and textures ------------------------------------------------------------------------------------------ */

typedef struct kspcore_model_material {
    uint32_t struct_size;
    const char* name;
    const char* shader_name; /* e.g. "KSP/Bumped Specular", "KSP/Emissive/Bumped Specular" (the Shader.Find name) */
    int32_t legacy_shader_type; /* version < 4: PartToolsLib.ShaderType as stored; -1 for version >= 4 */
    int32_t property_count;
} kspcore_model_material;
int kspcore_model_get_material(const kspcore_model* model, int material, kspcore_model_material* out);

#define KSPCORE_PROP_COLOR 0
#define KSPCORE_PROP_VECTOR 1
#define KSPCORE_PROP_FLOAT 2
#define KSPCORE_PROP_RANGE 3
#define KSPCORE_PROP_TEXTURE 4

typedef struct kspcore_model_property {
    uint32_t struct_size;
    const char* name; /* "_MainTex", "_BumpMap", "_SpecColor", "_Shininess", "_EmissiveColor" ... */
    int32_t kind;     /* KSPCORE_PROP_* */
    float value[4];   /* colour / vector: 4; float / range: [0] */
    int32_t texture_id; /* texture: index into the texture table, -1 = none */
    float tex_scale[2], tex_offset[2];
} kspcore_model_property;
int kspcore_model_get_property(const kspcore_model* model, int material, int property, kspcore_model_property* out);

typedef struct kspcore_model_texture {
    uint32_t struct_size;
    const char* file; /* as stored in the model */
    const char* name; /* Path.GetFileNameWithoutExtension(file) */
    const char* url;  /* "<parent_url>/<name>": the GameDatabase texture url (no extension) */
    int32_t type;     /* 0 Texture, 1 NormalMap (the model asks GetTexture(url, asNormalMap)) */
} kspcore_model_texture;
int kspcore_model_get_texture(const kspcore_model* model, int texture, kspcore_model_texture* out);

/* ---- lights, cameras ------------------------------------------------------------------------------------------------- */

typedef struct kspcore_model_light {
    uint32_t struct_size;
    int32_t node;
    int32_t type; /* UnityEngine.LightType */
    float intensity, range;
    float color[4];
    int32_t culling_mask;
    int32_t has_spot_angle; /* fileVersion > 1 stores it */
    float spot_angle;       /* 30 (Unity default) when absent */
} kspcore_model_light;
int kspcore_model_get_light(const kspcore_model* model, int light, kspcore_model_light* out);

typedef struct kspcore_model_camera {
    uint32_t struct_size;
    int32_t node;
    int32_t clear_flags;
    float background_color[4];
    int32_t culling_mask;
    int32_t orthographic;
    float field_of_view, near_clip_plane, far_clip_plane, depth;
} kspcore_model_camera;
int kspcore_model_get_camera(const kspcore_model* model, int camera, kspcore_model_camera* out);

/* ---- animations (legacy clips; WP-27 samples them in core, the shell needs the poses from the snapshot) -------------------- */

typedef struct kspcore_model_animation {
    uint32_t struct_size;
    int32_t node;       /* the transform that owns the Animation component */
    int32_t first_clip; /* index of its first clip in the clip table */
    int32_t clip_count;
    const char* default_clip;     /* "" = none */
    int32_t default_clip_applied; /* KSP set Animation.clip from it (default_clip != "" and no curve type had to be dropped) */
    int32_t play_automatically;
} kspcore_model_animation;
int kspcore_model_get_animation(const kspcore_model* model, int animation, kspcore_model_animation* out);

typedef struct kspcore_model_clip {
    uint32_t struct_size;
    int32_t animation;
    const char* name;
    float bounds[6]; /* localBounds centre, size */
    int32_t wrap_mode; /* UnityEngine.WrapMode */
    int32_t dropped_by_ksp; /* a curve with AnimationType.ParticleEmitter (4) made KSP skip this and every later clip (PartReader.cs:851-861) */
    int32_t curve_count;
    float length; /* AnimationClip.length: the latest key time */
} kspcore_model_clip;
int kspcore_model_get_clip(const kspcore_model* model, int clip, kspcore_model_clip* out);

typedef struct kspcore_model_key {
    float time, value, in_tangent, out_tangent;
    int32_t unused; /* the int after each keyframe (read and dropped by KSP) */
} kspcore_model_key;

#define KSPCORE_CURVE_TRANSFORM 0
#define KSPCORE_CURVE_MATERIAL 1
#define KSPCORE_CURVE_LIGHT 2
#define KSPCORE_CURVE_AUDIOSOURCE 3
#define KSPCORE_CURVE_PARTICLE_EMITTER 4

typedef struct kspcore_model_curve {
    uint32_t struct_size;
    const char* path;     /* relative to the animation's node ("" = the node itself) */
    const char* property; /* "m_LocalRotation.w", "m_LocalPosition.x", ... */
    int32_t type;         /* KSPCORE_CURVE_* */
    int32_t pre_wrap_mode, post_wrap_mode;
    int32_t key_count;
    const kspcore_model_key* keys;
} kspcore_model_curve;
int kspcore_model_get_curve(const kspcore_model* model, int clip, int curve, kspcore_model_curve* out);

/* Particle emitters: the material slot only. */
int kspcore_model_particle_emitter_material(const kspcore_model* model, int index);

/* ---- texture headers ------------------------------------------------------------------------------------------------- */

#define KSPCORE_TEX_UNKNOWN 0
#define KSPCORE_TEX_DDS 1
#define KSPCORE_TEX_MBM 2
#define KSPCORE_TEX_PNG 3
#define KSPCORE_TEX_JPEG 4
#define KSPCORE_TEX_TGA 5
#define KSPCORE_TEX_TRUECOLOR 6

/* UnityEngine.TextureFormat values */
#define KSPCORE_TEXFMT_NONE 0
#define KSPCORE_TEXFMT_ALPHA8 1
#define KSPCORE_TEXFMT_RGB24 3
#define KSPCORE_TEXFMT_RGBA32 4
#define KSPCORE_TEXFMT_ARGB32 5
#define KSPCORE_TEXFMT_DXT1 10
#define KSPCORE_TEXFMT_DXT5 12

typedef struct kspcore_texture_header {
    uint32_t struct_size;
    int32_t container;      /* KSPCORE_TEX_* (picked by the extension of the name, as DatabaseLoaderAttrib does) */
    int32_t valid;          /* the header parsed: width / height are meaningful */
    int32_t loadable;       /* the game's loader would return a Texture2D (DXT2 / DXT3 / DXT4 / DX10 DDS and MBM with other bit depths do not) */
    int32_t width, height;
    int32_t source_format;  /* UnityEngine.TextureFormat the loader creates first (DDS: the stored DXT1 / DXT5) */
    int32_t unity_format;   /* after the loader's compression step */
    int32_t compressed;     /* unity_format is a DXT format the loader produced (DDS: stored so; others: Compress on multiples of 4) */
    int32_t mip_chain;      /* Texture2D mipChain 1 / 0; -1 = decided inside Unity's decoder (inferred, see texture_header.hpp) */
    int32_t mip_count;      /* levels of the texture when mip_chain is 1 (full chain), 1 when 0, 0 if unknown */
    int32_t is_normal_map;  /* GameDatabase.TextureInfo.isNormalMap (DDS: pixel-format flags 0x80000 / 0x80000000; MBM: type 1; "...NRM" files) */
    int32_t is_readable;    /* TextureInfo.isReadable */
    int32_t is_compressed_flag; /* TextureInfo.isCompressed as the loader passes it */
    int32_t nrm_suffix;     /* the file name ends with NRM: GameDatabase.BitmapToUnityNormalMap (RGBA32, mips, channels swizzled) */
    int32_t has_alpha;
    uint32_t dds_header_flags, dds_pixel_format_flags, dds_fourcc, dds_caps, dds_mip_map_count, dds_rgb_bit_count, dds_dxgi_format;
    int32_t mbm_texture_type, mbm_bits_per_pixel; /* -1 when not an MBM */
    int32_t png_color_type, png_bit_depth;        /* -1 when not a PNG / TRUECOLOR PNG */
    int32_t tga_image_type, tga_pixel_depth;      /* -1 when not a TGA */
    uint64_t data_offset;          /* first byte after the header(s) */
    uint64_t data_bytes;           /* file_size - data_offset */
    uint64_t expected_data_bytes;  /* the pixel bytes Unity needs (DDS blocks of the whole chain, MBM / TGA raw pixels), 0 = not applicable */
    char error[160];               /* "" when loadable; otherwise why not (the loader's message where it has one) */
} kspcore_texture_header;

/* Decodes the header of `data[0..size)` = the start (or all) of the file `name` that is `file_size` bytes long (0 = size). 1 if
 * the header is valid, 0 otherwise (the struct is filled either way; -1 for a NULL argument, with kspcore_model_last_error). */
int kspcore_texture_decode_header(const char* name, const void* data, size_t size, uint64_t file_size, kspcore_texture_header* out);

#ifdef __cplusplus
}
#endif

#endif /* KSPCORE_MODEL_H */
