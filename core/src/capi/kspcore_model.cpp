// C ABI of the model domain (include/kspcore/kspcore_model.h): the `.mu` render reader of ksp/parts/mu_model.hpp and the texture header
// decode of ksp/parts/texture_header.hpp. Every entry point catches all C++ exceptions; nothing but C types crosses the boundary.
#include "kspcore/kspcore_model.h"

#include <cstddef>
#include <cstring>
#include <exception>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "ksp/parts/mu_model.hpp"
#include "ksp/parts/texture_header.hpp"

using namespace ksp;

struct kspcore_model {
    mu::Model model;
    std::string parentUrl;
    std::vector<const mu::Node*> nodes;  // preorder
    std::vector<int32_t> parents;
    struct MeshRef {
        int32_t node, source;
        const mu::RenderMesh* mesh;
    };
    std::vector<MeshRef> meshes;
    struct RendererRef {
        int32_t node, kind, mesh;
        const mu::MeshRendererChunk* mr;
        const mu::SkinnedRendererChunk* sr;
    };
    std::vector<RendererRef> renderers;
    struct LightRef {
        int32_t node;
        const mu::LightChunk* c;
    };
    std::vector<LightRef> lights;
    struct CameraRef {
        int32_t node;
        const mu::CameraChunk* c;
    };
    std::vector<CameraRef> cameras;
    struct AnimRef {
        int32_t node, firstClip;
        const mu::AnimationChunk* a;
    };
    std::vector<AnimRef> animations;
    struct ClipRef {
        int32_t animation;
        const mu::AnimClip* c;
    };
    std::vector<ClipRef> clips;
    std::vector<int32_t> particleMaterials;
    std::vector<std::string> textureUrls;
    std::vector<std::pair<int32_t, int32_t>> ignored, ignoredMesh;
    int32_t colliderCount = 0;
};

namespace {

thread_local std::string t_modelError;

void SetError(const char* where, const char* what) { t_modelError = std::string(where) + ": " + what; }

// Runs `f`, converting any exception into `fallback` + kspcore_model_last_error.
template <class R, class F>
R Guard(const char* where, R fallback, F&& f) {
    try {
        return f();
    } catch (const std::exception& e) {
        SetError(where, e.what());
    } catch (...) {
        SetError(where, "non-standard exception");
    }
    return fallback;
}

// Copies as much of `v` as the caller's struct_size allows (the struct protocol of the header).
template <class T>
int Emit(const char* where, const T& v, T* out) {
    if (!out) {
        SetError(where, "null output struct");
        return 0;
    }
    const uint32_t size = out->struct_size;
    if (size < sizeof(uint32_t)) {
        SetError(where, "struct_size not set");
        return 0;
    }
    std::memcpy(out, &v, size < sizeof(T) ? size : sizeof(T));
    out->struct_size = size;
    return 1;
}

bool CheckIndex(const char* where, const kspcore_model* m, int index, size_t count, const char* what) {
    if (!m) {
        SetError(where, "null model");
        return false;
    }
    if (index < 0 || static_cast<size_t>(index) >= count) {
        SetError(where, (std::string(what) + " index out of range").c_str());
        return false;
    }
    return true;
}

const float* FloatsOrNull(const void* p, bool present) { return present ? static_cast<const float*>(p) : nullptr; }

void Build(kspcore_model& m, const mu::Node& n, int32_t parent) {
    const int32_t id = static_cast<int32_t>(m.nodes.size());
    m.nodes.push_back(&n);
    m.parents.push_back(parent);
    m.colliderCount += static_cast<int32_t>(n.colliders.size());
    for (const mu::RenderMesh& mesh : n.meshFilters) m.meshes.push_back({id, KSPCORE_MESH_SOURCE_FILTER, &mesh});
    const int32_t firstFilterMesh = n.meshFilters.empty() ? -1 : static_cast<int32_t>(m.meshes.size() - n.meshFilters.size());
    for (const mu::MeshRendererChunk& r : n.meshRenderers) m.renderers.push_back({id, KSPCORE_RENDERER_MESH, firstFilterMesh, &r, nullptr});
    for (const mu::SkinnedRendererChunk& s : n.skinnedRenderers) {
        m.meshes.push_back({id, KSPCORE_MESH_SOURCE_SKINNED, &s.mesh});
        m.renderers.push_back({id, KSPCORE_RENDERER_SKINNED, static_cast<int32_t>(m.meshes.size() - 1), nullptr, &s});
    }
    for (const mu::LightChunk& l : n.lights) m.lights.push_back({id, &l});
    for (const mu::CameraChunk& c : n.cameras) m.cameras.push_back({id, &c});
    for (const mu::ParticleEmitterChunk& p : n.particleEmitters) m.particleMaterials.push_back(p.materialSlot);
    for (const mu::AnimationChunk& a : n.animations) {
        const int32_t animIndex = static_cast<int32_t>(m.animations.size());
        m.animations.push_back({id, static_cast<int32_t>(m.clips.size()), &a});
        for (const mu::AnimClip& c : a.clips) m.clips.push_back({animIndex, &c});
    }
    for (const mu::Node& c : n.children) Build(m, c, id);
}

kspcore_model* Finish(kspcore_model* h, const char* parentUrl) {
    h->parentUrl = parentUrl ? parentUrl : "";
    Build(*h, h->model.root, -1);
    for (const mu::TextureRef& t : h->model.textures) h->textureUrls.push_back(h->parentUrl.empty() ? t.name : h->parentUrl + "/" + t.name);
    for (const auto& kv : h->model.ignoredEntries) h->ignored.emplace_back(kv.first, kv.second);
    for (const auto& kv : h->model.ignoredMeshEntries) h->ignoredMesh.emplace_back(kv.first, kv.second);
    return h;
}

int CountChildren(const mu::Node& n) { return static_cast<int>(n.children.size()); }

}  // namespace

extern "C" {

const char* kspcore_model_last_error(void) { return t_modelError.c_str(); }

kspcore_model* kspcore_model_load_memory(const void* data, size_t size, const char* parent_url) {
    return Guard<kspcore_model*>("kspcore_model_load_memory", nullptr, [&]() -> kspcore_model* {
        if (!data && size) {
            SetError("kspcore_model_load_memory", "null data");
            return nullptr;
        }
        std::unique_ptr<kspcore_model> h(new kspcore_model());
        h->model = mu::ReadModel(static_cast<const uint8_t*>(data), size);
        return Finish(h.release(), parent_url);
    });
}

kspcore_model* kspcore_model_load_file(const char* utf8_path, const char* parent_url) {
    return Guard<kspcore_model*>("kspcore_model_load_file", nullptr, [&]() -> kspcore_model* {
        if (!utf8_path) {
            SetError("kspcore_model_load_file", "null path");
            return nullptr;
        }
        std::ifstream f(utf8_path, std::ios::binary);
        if (!f) {
            SetError("kspcore_model_load_file", (std::string("cannot open ") + utf8_path).c_str());
            return nullptr;
        }
        std::vector<char> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        std::unique_ptr<kspcore_model> h(new kspcore_model());
        h->model = mu::ReadModel(reinterpret_cast<const uint8_t*>(buf.data()), buf.size());
        return Finish(h.release(), parent_url);
    });
}

void kspcore_model_release(kspcore_model* model) { delete model; }

int kspcore_model_get_info(const kspcore_model* m, kspcore_model_info* out) {
    return Guard<int>("kspcore_model_get_info", 0, [&]() {
        if (!m) {
            SetError("kspcore_model_get_info", "null model");
            return 0;
        }
        kspcore_model_info v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.file_type = m->model.fileType;
        v.version = m->model.version;
        v.header = m->model.header.c_str();
        v.node_count = static_cast<int32_t>(m->nodes.size());
        v.mesh_count = static_cast<int32_t>(m->meshes.size());
        v.renderer_count = static_cast<int32_t>(m->renderers.size());
        v.material_count = static_cast<int32_t>(m->model.materials.size());
        v.texture_count = static_cast<int32_t>(m->model.textures.size());
        v.light_count = static_cast<int32_t>(m->lights.size());
        v.camera_count = static_cast<int32_t>(m->cameras.size());
        v.particle_emitter_count = static_cast<int32_t>(m->particleMaterials.size());
        v.animation_count = static_cast<int32_t>(m->animations.size());
        v.clip_count = static_cast<int32_t>(m->clips.size());
        v.collider_count = m->colliderCount;
        v.materials_chunk_count = m->model.materialsChunkCount;
        v.textures_chunk_count = m->model.texturesChunkCount;
        v.warning_count = static_cast<int32_t>(m->model.warnings.size());
        v.ignored_entry_count = static_cast<int32_t>(m->ignored.size());
        v.ignored_mesh_entry_count = static_cast<int32_t>(m->ignoredMesh.size());
        return Emit("kspcore_model_get_info", v, out);
    });
}

const char* kspcore_model_warning(const kspcore_model* m, int index) {
    return Guard<const char*>("kspcore_model_warning", nullptr, [&]() -> const char* {
        if (!CheckIndex("kspcore_model_warning", m, index, m ? m->model.warnings.size() : 0, "warning")) return nullptr;
        return m->model.warnings[static_cast<size_t>(index)].c_str();
    });
}

int kspcore_model_ignored_entry(const kspcore_model* m, int index, int32_t* value, int32_t* occurrences) {
    return Guard<int>("kspcore_model_ignored_entry", 0, [&]() {
        if (!CheckIndex("kspcore_model_ignored_entry", m, index, m ? m->ignored.size() : 0, "ignored entry") || !value || !occurrences) return 0;
        *value = m->ignored[static_cast<size_t>(index)].first;
        *occurrences = m->ignored[static_cast<size_t>(index)].second;
        return 1;
    });
}

int kspcore_model_ignored_mesh_entry(const kspcore_model* m, int index, int32_t* value, int32_t* occurrences) {
    return Guard<int>("kspcore_model_ignored_mesh_entry", 0, [&]() {
        if (!CheckIndex("kspcore_model_ignored_mesh_entry", m, index, m ? m->ignoredMesh.size() : 0, "ignored mesh entry") || !value || !occurrences) return 0;
        *value = m->ignoredMesh[static_cast<size_t>(index)].first;
        *occurrences = m->ignoredMesh[static_cast<size_t>(index)].second;
        return 1;
    });
}

int kspcore_model_get_node(const kspcore_model* m, int node, kspcore_model_node* out) {
    return Guard<int>("kspcore_model_get_node", 0, [&]() {
        if (!CheckIndex("kspcore_model_get_node", m, node, m ? m->nodes.size() : 0, "node")) return 0;
        const mu::Node& n = *m->nodes[static_cast<size_t>(node)];
        kspcore_model_node v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.name = n.name.c_str();
        v.parent = m->parents[static_cast<size_t>(node)];
        v.child_count = CountChildren(n);
        v.position[0] = n.position.x;
        v.position[1] = n.position.y;
        v.position[2] = n.position.z;
        v.rotation[0] = n.rotation.x;
        v.rotation[1] = n.rotation.y;
        v.rotation[2] = n.rotation.z;
        v.rotation[3] = n.rotation.w;
        v.scale[0] = n.scale.x;
        v.scale[1] = n.scale.y;
        v.scale[2] = n.scale.z;
        v.has_tag_and_layer = n.hasTagAndLayer ? 1 : 0;
        v.tag = n.tag.c_str();
        v.layer = n.layer;
        v.mesh_filter_count = static_cast<int32_t>(n.meshFilters.size());
        v.mesh_renderer_count = static_cast<int32_t>(n.meshRenderers.size());
        v.skinned_renderer_count = static_cast<int32_t>(n.skinnedRenderers.size());
        v.collider_count = static_cast<int32_t>(n.colliders.size());
        v.animation_count = static_cast<int32_t>(n.animations.size());
        v.light_count = static_cast<int32_t>(n.lights.size());
        v.camera_count = static_cast<int32_t>(n.cameras.size());
        v.particle_emitter_count = static_cast<int32_t>(n.particleEmitters.size());
        return Emit("kspcore_model_get_node", v, out);
    });
}

int kspcore_model_node_child(const kspcore_model* m, int node, int k) {
    return Guard<int>("kspcore_model_node_child", -1, [&]() {
        if (!CheckIndex("kspcore_model_node_child", m, node, m ? m->nodes.size() : 0, "node")) return -1;
        const mu::Node& n = *m->nodes[static_cast<size_t>(node)];
        if (k < 0 || static_cast<size_t>(k) >= n.children.size()) return -1;
        return n.children[static_cast<size_t>(k)].id;
    });
}

int kspcore_model_get_mesh(const kspcore_model* m, int mesh, kspcore_model_mesh* out) {
    return Guard<int>("kspcore_model_get_mesh", 0, [&]() {
        if (!CheckIndex("kspcore_model_get_mesh", m, mesh, m ? m->meshes.size() : 0, "mesh")) return 0;
        const kspcore_model::MeshRef& ref = m->meshes[static_cast<size_t>(mesh)];
        const mu::RenderMesh& r = *ref.mesh;
        kspcore_model_mesh v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.node = ref.node;
        v.source = ref.source;
        v.vertex_count = r.vertexCount;
        v.header_int = r.headerInt;
        v.flags = (r.hasVertices ? KSPCORE_MESH_HAS_VERTICES : 0u) | (r.hasNormals ? KSPCORE_MESH_HAS_NORMALS : 0u) |
                  (r.hasTangents ? KSPCORE_MESH_HAS_TANGENTS : 0u) | (r.hasUv ? KSPCORE_MESH_HAS_UV : 0u) |
                  (r.hasUv2 ? KSPCORE_MESH_HAS_UV2 : 0u) | (r.hasColors ? KSPCORE_MESH_HAS_COLORS : 0u) |
                  (r.hasBoneWeights ? KSPCORE_MESH_HAS_BONE_WEIGHTS : 0u) | (r.hasBindPoses ? KSPCORE_MESH_HAS_BIND_POSES : 0u);
        v.vertices = FloatsOrNull(r.vertices.data(), r.hasVertices);
        v.normals = FloatsOrNull(r.normals.data(), r.hasNormals);
        v.tangents = FloatsOrNull(r.tangents.data(), r.hasTangents);
        v.uv = FloatsOrNull(r.uv.data(), r.hasUv);
        v.uv2 = FloatsOrNull(r.uv2.data(), r.hasUv2);
        v.colors32 = r.hasColors ? r.colors32.data() : nullptr;
        v.submesh_count = static_cast<int32_t>(r.triangleLists.size());
        v.bone_indices = r.hasBoneWeights ? r.boneIndices.data() : nullptr;
        v.bone_weights = r.hasBoneWeights ? r.boneWeights.data() : nullptr;
        v.bind_pose_count = static_cast<int32_t>(r.BindPoseCount());
        v.bind_poses = r.hasBindPoses ? r.bindPoses.data() : nullptr;
        return Emit("kspcore_model_get_mesh", v, out);
    });
}

const int32_t* kspcore_model_mesh_submesh(const kspcore_model* m, int mesh, int sub, int32_t* index_count) {
    return Guard<const int32_t*>("kspcore_model_mesh_submesh", nullptr, [&]() -> const int32_t* {
        if (!CheckIndex("kspcore_model_mesh_submesh", m, mesh, m ? m->meshes.size() : 0, "mesh")) return nullptr;
        const mu::RenderMesh& r = *m->meshes[static_cast<size_t>(mesh)].mesh;
        if (sub < 0 || static_cast<size_t>(sub) >= r.triangleLists.size()) {
            SetError("kspcore_model_mesh_submesh", "submesh index out of range");
            return nullptr;
        }
        const std::vector<int32_t>& t = r.triangleLists[static_cast<size_t>(sub)];
        if (index_count) *index_count = static_cast<int32_t>(t.size());
        return t.data();
    });
}

int kspcore_model_get_renderer(const kspcore_model* m, int renderer, kspcore_model_renderer* out) {
    return Guard<int>("kspcore_model_get_renderer", 0, [&]() {
        if (!CheckIndex("kspcore_model_get_renderer", m, renderer, m ? m->renderers.size() : 0, "renderer")) return 0;
        const kspcore_model::RendererRef& ref = m->renderers[static_cast<size_t>(renderer)];
        kspcore_model_renderer v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.node = ref.node;
        v.kind = ref.kind;
        v.mesh = ref.mesh;
        if (ref.mr) {
            v.has_shadow_fields = ref.mr->hasShadowFields ? 1 : 0;
            v.cast_shadows = ref.mr->castShadows ? 1 : 0;
            v.receive_shadows = ref.mr->receiveShadows ? 1 : 0;
            v.material_slot_count = static_cast<int32_t>(ref.mr->materialSlots.size());
            v.material_slots = ref.mr->materialSlots.data();
        } else {
            const mu::SkinnedRendererChunk& s = *ref.sr;
            v.cast_shadows = 1;  // AddComponent<SkinnedMeshRenderer> defaults
            v.receive_shadows = 1;
            v.material_slot_count = static_cast<int32_t>(s.materialSlots.size());
            v.material_slots = s.materialSlots.data();
            for (int i = 0; i < 6; ++i) v.local_bounds[i] = s.localBounds[i];
            v.quality = s.quality;
            v.update_when_offscreen = s.updateWhenOffscreen ? 1 : 0;
            v.bone_count = static_cast<int32_t>(s.boneNames.size());
            v.bone_nodes = s.boneNodeIds.data();
        }
        return Emit("kspcore_model_get_renderer", v, out);
    });
}

const char* kspcore_model_renderer_bone_name(const kspcore_model* m, int renderer, int bone) {
    return Guard<const char*>("kspcore_model_renderer_bone_name", nullptr, [&]() -> const char* {
        if (!CheckIndex("kspcore_model_renderer_bone_name", m, renderer, m ? m->renderers.size() : 0, "renderer")) return nullptr;
        const mu::SkinnedRendererChunk* s = m->renderers[static_cast<size_t>(renderer)].sr;
        if (!s || bone < 0 || static_cast<size_t>(bone) >= s->boneNames.size()) {
            SetError("kspcore_model_renderer_bone_name", "bone index out of range");
            return nullptr;
        }
        return s->boneNames[static_cast<size_t>(bone)].c_str();
    });
}

int kspcore_model_get_material(const kspcore_model* m, int material, kspcore_model_material* out) {
    return Guard<int>("kspcore_model_get_material", 0, [&]() {
        if (!CheckIndex("kspcore_model_get_material", m, material, m ? m->model.materials.size() : 0, "material")) return 0;
        const mu::MaterialChunk& c = m->model.materials[static_cast<size_t>(material)];
        kspcore_model_material v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.name = c.name.c_str();
        v.shader_name = c.shaderName.c_str();
        v.legacy_shader_type = c.legacyShaderType;
        v.property_count = static_cast<int32_t>(c.properties.size());
        return Emit("kspcore_model_get_material", v, out);
    });
}

int kspcore_model_get_property(const kspcore_model* m, int material, int property, kspcore_model_property* out) {
    return Guard<int>("kspcore_model_get_property", 0, [&]() {
        if (!CheckIndex("kspcore_model_get_property", m, material, m ? m->model.materials.size() : 0, "material")) return 0;
        const mu::MaterialChunk& c = m->model.materials[static_cast<size_t>(material)];
        if (property < 0 || static_cast<size_t>(property) >= c.properties.size()) {
            SetError("kspcore_model_get_property", "property index out of range");
            return 0;
        }
        const mu::MaterialProperty& p = c.properties[static_cast<size_t>(property)];
        kspcore_model_property v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.name = p.name.c_str();
        v.kind = static_cast<int32_t>(p.kind);
        for (int i = 0; i < 4; ++i) v.value[i] = p.value[i];
        v.texture_id = p.textureId;
        v.tex_scale[0] = p.texScale[0];
        v.tex_scale[1] = p.texScale[1];
        v.tex_offset[0] = p.texOffset[0];
        v.tex_offset[1] = p.texOffset[1];
        return Emit("kspcore_model_get_property", v, out);
    });
}

int kspcore_model_get_texture(const kspcore_model* m, int texture, kspcore_model_texture* out) {
    return Guard<int>("kspcore_model_get_texture", 0, [&]() {
        if (!CheckIndex("kspcore_model_get_texture", m, texture, m ? m->model.textures.size() : 0, "texture")) return 0;
        const mu::TextureRef& t = m->model.textures[static_cast<size_t>(texture)];
        kspcore_model_texture v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.file = t.file.c_str();
        v.name = t.name.c_str();
        v.url = m->textureUrls[static_cast<size_t>(texture)].c_str();
        v.type = t.type;
        return Emit("kspcore_model_get_texture", v, out);
    });
}

int kspcore_model_get_light(const kspcore_model* m, int light, kspcore_model_light* out) {
    return Guard<int>("kspcore_model_get_light", 0, [&]() {
        if (!CheckIndex("kspcore_model_get_light", m, light, m ? m->lights.size() : 0, "light")) return 0;
        const kspcore_model::LightRef& r = m->lights[static_cast<size_t>(light)];
        kspcore_model_light v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.node = r.node;
        v.type = r.c->type;
        v.intensity = r.c->intensity;
        v.range = r.c->range;
        for (int i = 0; i < 4; ++i) v.color[i] = r.c->color[i];
        v.culling_mask = r.c->cullingMask;
        v.has_spot_angle = r.c->hasSpotAngle ? 1 : 0;
        v.spot_angle = r.c->spotAngle;
        return Emit("kspcore_model_get_light", v, out);
    });
}

int kspcore_model_get_camera(const kspcore_model* m, int camera, kspcore_model_camera* out) {
    return Guard<int>("kspcore_model_get_camera", 0, [&]() {
        if (!CheckIndex("kspcore_model_get_camera", m, camera, m ? m->cameras.size() : 0, "camera")) return 0;
        const kspcore_model::CameraRef& r = m->cameras[static_cast<size_t>(camera)];
        kspcore_model_camera v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.node = r.node;
        v.clear_flags = r.c->clearFlags;
        for (int i = 0; i < 4; ++i) v.background_color[i] = r.c->backgroundColor[i];
        v.culling_mask = r.c->cullingMask;
        v.orthographic = r.c->orthographic ? 1 : 0;
        v.field_of_view = r.c->fieldOfView;
        v.near_clip_plane = r.c->nearClipPlane;
        v.far_clip_plane = r.c->farClipPlane;
        v.depth = r.c->depth;
        return Emit("kspcore_model_get_camera", v, out);
    });
}

int kspcore_model_get_animation(const kspcore_model* m, int animation, kspcore_model_animation* out) {
    return Guard<int>("kspcore_model_get_animation", 0, [&]() {
        if (!CheckIndex("kspcore_model_get_animation", m, animation, m ? m->animations.size() : 0, "animation")) return 0;
        const kspcore_model::AnimRef& r = m->animations[static_cast<size_t>(animation)];
        kspcore_model_animation v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.node = r.node;
        v.first_clip = r.firstClip;
        v.clip_count = static_cast<int32_t>(r.a->clips.size());
        v.default_clip = r.a->defaultClip.c_str();
        v.default_clip_applied = r.a->defaultClipApplied ? 1 : 0;
        v.play_automatically = r.a->playAutomatically ? 1 : 0;
        return Emit("kspcore_model_get_animation", v, out);
    });
}

int kspcore_model_get_clip(const kspcore_model* m, int clip, kspcore_model_clip* out) {
    return Guard<int>("kspcore_model_get_clip", 0, [&]() {
        if (!CheckIndex("kspcore_model_get_clip", m, clip, m ? m->clips.size() : 0, "clip")) return 0;
        const kspcore_model::ClipRef& r = m->clips[static_cast<size_t>(clip)];
        kspcore_model_clip v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.animation = r.animation;
        v.name = r.c->name.c_str();
        for (int i = 0; i < 6; ++i) v.bounds[i] = r.c->bounds[i];
        v.wrap_mode = r.c->wrapMode;
        v.dropped_by_ksp = r.c->droppedByKsp ? 1 : 0;
        v.curve_count = static_cast<int32_t>(r.c->curves.size());
        v.length = r.c->Length();
        return Emit("kspcore_model_get_clip", v, out);
    });
}

static_assert(sizeof(kspcore_model_key) == sizeof(mu::AnimKey) && offsetof(kspcore_model_key, unused) == offsetof(mu::AnimKey, unused),
              "the key array is handed out as is");

int kspcore_model_get_curve(const kspcore_model* m, int clip, int curve, kspcore_model_curve* out) {
    return Guard<int>("kspcore_model_get_curve", 0, [&]() {
        if (!CheckIndex("kspcore_model_get_curve", m, clip, m ? m->clips.size() : 0, "clip")) return 0;
        const mu::AnimClip& c = *m->clips[static_cast<size_t>(clip)].c;
        if (curve < 0 || static_cast<size_t>(curve) >= c.curves.size()) {
            SetError("kspcore_model_get_curve", "curve index out of range");
            return 0;
        }
        const mu::AnimCurve& k = c.curves[static_cast<size_t>(curve)];
        kspcore_model_curve v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.path = k.path.c_str();
        v.property = k.property.c_str();
        v.type = k.type;
        v.pre_wrap_mode = k.preWrapMode;
        v.post_wrap_mode = k.postWrapMode;
        v.key_count = static_cast<int32_t>(k.keys.size());
        v.keys = reinterpret_cast<const kspcore_model_key*>(k.keys.data());
        return Emit("kspcore_model_get_curve", v, out);
    });
}

int kspcore_model_particle_emitter_material(const kspcore_model* m, int index) {
    return Guard<int>("kspcore_model_particle_emitter_material", -1, [&]() {
        if (!CheckIndex("kspcore_model_particle_emitter_material", m, index, m ? m->particleMaterials.size() : 0, "particle emitter")) return -1;
        return m->particleMaterials[static_cast<size_t>(index)];
    });
}

int kspcore_texture_decode_header(const char* name, const void* data, size_t size, uint64_t file_size, kspcore_texture_header* out) {
    return Guard<int>("kspcore_texture_decode_header", -1, [&]() {
        if (!name || (!data && size) || !out) {
            SetError("kspcore_texture_decode_header", "null argument");
            return -1;
        }
        const tex::Header h = tex::DecodeHeader(name, static_cast<const uint8_t*>(data), size, file_size);
        kspcore_texture_header v;
        std::memset(&v, 0, sizeof v);
        v.struct_size = sizeof v;
        v.container = static_cast<int32_t>(h.container);
        v.valid = h.valid ? 1 : 0;
        v.loadable = h.loadable ? 1 : 0;
        v.width = h.width;
        v.height = h.height;
        v.source_format = h.sourceFormat;
        v.unity_format = h.unityFormat;
        v.compressed = h.compressed ? 1 : 0;
        v.mip_chain = h.mipChain;
        v.mip_count = h.mipCount;
        v.is_normal_map = h.isNormalMap ? 1 : 0;
        v.is_readable = h.isReadable ? 1 : 0;
        v.is_compressed_flag = h.isCompressedFlag ? 1 : 0;
        v.nrm_suffix = h.nrmSuffix ? 1 : 0;
        v.has_alpha = h.hasAlpha ? 1 : 0;
        v.dds_header_flags = h.ddsHeaderFlags;
        v.dds_pixel_format_flags = h.ddsPixelFormatFlags;
        v.dds_fourcc = h.ddsFourCC;
        v.dds_caps = h.ddsCaps;
        v.dds_mip_map_count = h.ddsMipMapCount;
        v.dds_rgb_bit_count = h.ddsRgbBitCount;
        v.dds_dxgi_format = h.ddsDxgiFormat;
        v.mbm_texture_type = h.mbmTextureType;
        v.mbm_bits_per_pixel = h.mbmBitsPerPixel;
        v.png_color_type = h.pngColorType;
        v.png_bit_depth = h.pngBitDepth;
        v.tga_image_type = h.tgaImageType;
        v.tga_pixel_depth = h.tgaPixelDepth;
        v.data_offset = h.dataOffset;
        v.data_bytes = h.dataBytes;
        v.expected_data_bytes = h.expectedDataBytes;
        std::strncpy(v.error, h.error.c_str(), sizeof v.error - 1);
        if (!Emit("kspcore_texture_decode_header", v, out)) return -1;
        return h.valid ? 1 : 0;
    });
}

}  // extern "C"
