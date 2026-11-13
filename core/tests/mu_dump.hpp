// WP-UE-05: the (key, value) row dump of a parsed .mu model, produced ONLY through the C ABI (include/kspcore/kspcore_model.h).
// The row grammar is the one tools/wpue05/mu_render_reference.py writes from its own parse (see rows() there): ints in decimal, floats as
// 8 hex digits of their bits, strings with bytes outside 0x20..0x7e and the backslash as \xHH; `full` lists every array element, the
// compact mode lists an array as "<count>:<FNV-1a 64 of its raw little-endian bytes>". Used by tests/test_mu_render.cpp (the gate) and
// tests/capi/mu_sweep.cpp (the stock sweep).
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "kspcore/kspcore_model.h"

namespace mudump {

using Rows = std::vector<std::pair<std::string, std::string>>;

inline uint64_t Fnv(const void* data, size_t n, uint64_t h = 0xcbf29ce484222325ull) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < n; ++i) h = (h ^ p[i]) * 0x100000001b3ull;
    return h;
}

inline std::string Esc(const char* s) {
    std::string out;
    for (const char* p = s ? s : ""; *p; ++p) {
        const unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 0x20 && c <= 0x7e && c != '\\') {
            out += static_cast<char>(c);
        } else {
            char buf[8];
            std::snprintf(buf, sizeof buf, "\\x%02x", c);
            out += buf;
        }
    }
    return out;
}

inline std::string Hex(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%08x", u);
    return buf;
}
inline std::string Hex64(uint64_t v) {
    char buf[24];
    std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(v));
    return buf;
}
inline std::string Num(long long v) { return std::to_string(v); }

class Dumper {
public:
    Dumper(const kspcore_model* m, bool full) : m_(m), full_(full) {}

    Rows Run() {
        kspcore_model_info info = {};
        info.struct_size = sizeof info;
        if (!kspcore_model_get_info(m_, &info)) return rows_;
        R("model.file_type", Num(info.file_type));
        R("model.version", Num(info.version));
        R("model.header", Esc(info.header));
        R("model.node_count", Num(info.node_count));
        R("model.mesh_count", Num(info.mesh_count));
        R("model.renderer_count", Num(info.renderer_count));
        R("model.material_count", Num(info.material_count));
        R("model.texture_count", Num(info.texture_count));
        R("model.light_count", Num(info.light_count));
        R("model.camera_count", Num(info.camera_count));
        R("model.particle_emitter_count", Num(info.particle_emitter_count));
        R("model.animation_count", Num(info.animation_count));
        R("model.clip_count", Num(info.clip_count));
        R("model.collider_count", Num(info.collider_count));
        R("model.materials_chunk_count", Num(info.materials_chunk_count));
        R("model.textures_chunk_count", Num(info.textures_chunk_count));
        R("model.warning_count", Num(info.warning_count));
        R("model.ignored_entry_count", Num(info.ignored_entry_count));
        R("model.ignored_mesh_entry_count", Num(info.ignored_mesh_entry_count));
        for (int i = 0; i < info.warning_count; ++i) R("warning." + Num(i), Esc(kspcore_model_warning(m_, i)));
        for (int i = 0; i < info.ignored_entry_count; ++i) {
            int32_t v = 0, c = 0;
            kspcore_model_ignored_entry(m_, i, &v, &c);
            R("ignored." + Num(i), Num(v) + ":" + Num(c));
        }
        for (int i = 0; i < info.ignored_mesh_entry_count; ++i) {
            int32_t v = 0, c = 0;
            kspcore_model_ignored_mesh_entry(m_, i, &v, &c);
            R("ignoredmesh." + Num(i), Num(v) + ":" + Num(c));
        }
        for (int i = 0; i < info.node_count; ++i) Node(i);
        for (int i = 0; i < info.mesh_count; ++i) Mesh(i);
        for (int i = 0; i < info.renderer_count; ++i) Renderer(i);
        for (int i = 0; i < info.material_count; ++i) Material(i);
        for (int i = 0; i < info.texture_count; ++i) Texture(i);
        for (int i = 0; i < info.light_count; ++i) Light(i);
        for (int i = 0; i < info.camera_count; ++i) Camera(i);
        for (int i = 0; i < info.particle_emitter_count; ++i) R("particle." + Num(i) + ".material", Num(kspcore_model_particle_emitter_material(m_, i)));
        for (int i = 0; i < info.animation_count; ++i) Animation(i);
        for (int i = 0; i < info.clip_count; ++i) Clip(i);
        return rows_;
    }

private:
    void R(const std::string& k, const std::string& v) { rows_.emplace_back(k, v); }

    // elem: 'f' float bits, 'i' int32, 'b' byte
    void Arr(const std::string& key, const void* data, size_t count, char elem) {
        if (full_) {
            for (size_t j = 0; j < count; ++j) {
                const std::string k = key + "." + Num(static_cast<long long>(j));
                if (elem == 'f') {
                    uint32_t u;
                    std::memcpy(&u, static_cast<const uint8_t*>(data) + 4 * j, 4);
                    char buf[16];
                    std::snprintf(buf, sizeof buf, "%08x", u);
                    R(k, buf);
                } else if (elem == 'i') {
                    int32_t v;
                    std::memcpy(&v, static_cast<const uint8_t*>(data) + 4 * j, 4);
                    R(k, Num(v));
                } else {
                    R(k, Num(static_cast<const uint8_t*>(data)[j]));
                }
            }
        } else {
            const size_t bytes = elem == 'b' ? count : 4 * count;
            R(key, Num(static_cast<long long>(count)) + ":" + Hex64(Fnv(data, bytes)));
        }
    }

    void Node(int i) {
        kspcore_model_node n = {};
        n.struct_size = sizeof n;
        kspcore_model_get_node(m_, i, &n);
        const std::string p = "node." + Num(i) + ".";
        R(p + "name", Esc(n.name));
        R(p + "parent", Num(n.parent));
        R(p + "child_count", Num(n.child_count));
        for (int j = 0; j < 3; ++j) R(p + "pos." + Num(j), Hex(n.position[j]));
        for (int j = 0; j < 4; ++j) R(p + "rot." + Num(j), Hex(n.rotation[j]));
        for (int j = 0; j < 3; ++j) R(p + "scale." + Num(j), Hex(n.scale[j]));
        R(p + "has_tag", Num(n.has_tag_and_layer));
        R(p + "tag", Esc(n.tag));
        R(p + "layer", Num(n.layer));
        R(p + "mesh_filter_count", Num(n.mesh_filter_count));
        R(p + "mesh_renderer_count", Num(n.mesh_renderer_count));
        R(p + "skinned_renderer_count", Num(n.skinned_renderer_count));
        R(p + "collider_count", Num(n.collider_count));
        R(p + "animation_count", Num(n.animation_count));
        R(p + "light_count", Num(n.light_count));
        R(p + "camera_count", Num(n.camera_count));
        R(p + "particle_emitter_count", Num(n.particle_emitter_count));
        for (int k = 0; k < n.child_count; ++k) R(p + "child." + Num(k), Num(kspcore_model_node_child(m_, i, k)));
    }

    void Mesh(int i) {
        kspcore_model_mesh me = {};
        me.struct_size = sizeof me;
        kspcore_model_get_mesh(m_, i, &me);
        const std::string p = "mesh." + Num(i) + ".";
        const size_t n = static_cast<size_t>(me.vertex_count);
        R(p + "node", Num(me.node));
        R(p + "source", Num(me.source));
        R(p + "vertex_count", Num(me.vertex_count));
        R(p + "header_int", Num(me.header_int));
        R(p + "flags", Num(me.flags));
        if (me.flags & KSPCORE_MESH_HAS_VERTICES) Arr(p + "vertices", me.vertices, 3 * n, 'f');
        if (me.flags & KSPCORE_MESH_HAS_NORMALS) Arr(p + "normals", me.normals, 3 * n, 'f');
        if (me.flags & KSPCORE_MESH_HAS_TANGENTS) Arr(p + "tangents", me.tangents, 4 * n, 'f');
        if (me.flags & KSPCORE_MESH_HAS_UV) Arr(p + "uv", me.uv, 2 * n, 'f');
        if (me.flags & KSPCORE_MESH_HAS_UV2) Arr(p + "uv2", me.uv2, 2 * n, 'f');
        if (me.flags & KSPCORE_MESH_HAS_COLORS) Arr(p + "colors32", me.colors32, 4 * n, 'b');
        R(p + "submesh_count", Num(me.submesh_count));
        for (int s = 0; s < me.submesh_count; ++s) {
            int32_t k = 0;
            const int32_t* t = kspcore_model_mesh_submesh(m_, i, s, &k);
            Arr(p + "submesh." + Num(s), t, static_cast<size_t>(k), 'i');
        }
        if (me.flags & KSPCORE_MESH_HAS_BONE_WEIGHTS) {
            Arr(p + "bone_indices", me.bone_indices, 4 * n, 'i');
            Arr(p + "bone_weights", me.bone_weights, 4 * n, 'f');
        }
        R(p + "bind_pose_count", Num(me.bind_pose_count));
        if (me.flags & KSPCORE_MESH_HAS_BIND_POSES) Arr(p + "bind_poses", me.bind_poses, 16 * static_cast<size_t>(me.bind_pose_count), 'f');
    }

    static std::string Join(const int32_t* v, int n) {
        std::string s;
        for (int i = 0; i < n; ++i) s += (i ? "," : "") + Num(v[i]);
        return s;
    }

    void Renderer(int i) {
        kspcore_model_renderer r = {};
        r.struct_size = sizeof r;
        kspcore_model_get_renderer(m_, i, &r);
        const std::string p = "renderer." + Num(i) + ".";
        R(p + "node", Num(r.node));
        R(p + "kind", Num(r.kind));
        R(p + "mesh", Num(r.mesh));
        R(p + "has_shadow_fields", Num(r.has_shadow_fields));
        R(p + "cast_shadows", Num(r.cast_shadows));
        R(p + "receive_shadows", Num(r.receive_shadows));
        R(p + "material_slots", Join(r.material_slots, r.material_slot_count));
        if (r.kind == KSPCORE_RENDERER_SKINNED) {
            for (int j = 0; j < 6; ++j) R(p + "bounds." + Num(j), Hex(r.local_bounds[j]));
            R(p + "quality", Num(r.quality));
            R(p + "update_when_offscreen", Num(r.update_when_offscreen));
            R(p + "bone_count", Num(r.bone_count));
            R(p + "bone_nodes", Join(r.bone_nodes, r.bone_count));
            for (int j = 0; j < r.bone_count; ++j) R(p + "bone." + Num(j), Esc(kspcore_model_renderer_bone_name(m_, i, j)));
        }
    }

    void Material(int i) {
        kspcore_model_material c = {};
        c.struct_size = sizeof c;
        kspcore_model_get_material(m_, i, &c);
        const std::string p = "material." + Num(i) + ".";
        R(p + "name", Esc(c.name));
        R(p + "shader", Esc(c.shader_name));
        R(p + "legacy_shader_type", Num(c.legacy_shader_type));
        R(p + "property_count", Num(c.property_count));
        for (int j = 0; j < c.property_count; ++j) {
            kspcore_model_property pr = {};
            pr.struct_size = sizeof pr;
            kspcore_model_get_property(m_, i, j, &pr);
            const std::string q = p + "prop." + Num(j) + ".";
            R(q + "name", Esc(pr.name));
            R(q + "kind", Num(pr.kind));
            for (int k = 0; k < 4; ++k) R(q + "value." + Num(k), Hex(pr.value[k]));
            R(q + "texture_id", Num(pr.texture_id));
            for (int k = 0; k < 2; ++k) R(q + "scale." + Num(k), Hex(pr.tex_scale[k]));
            for (int k = 0; k < 2; ++k) R(q + "offset." + Num(k), Hex(pr.tex_offset[k]));
        }
    }

    void Texture(int i) {
        kspcore_model_texture t = {};
        t.struct_size = sizeof t;
        kspcore_model_get_texture(m_, i, &t);
        const std::string p = "texture." + Num(i) + ".";
        R(p + "file", Esc(t.file));
        R(p + "name", Esc(t.name));
        R(p + "url", Esc(t.url));
        R(p + "type", Num(t.type));
    }

    void Light(int i) {
        kspcore_model_light l = {};
        l.struct_size = sizeof l;
        kspcore_model_get_light(m_, i, &l);
        const std::string p = "light." + Num(i) + ".";
        R(p + "node", Num(l.node));
        R(p + "type", Num(l.type));
        R(p + "intensity", Hex(l.intensity));
        R(p + "range", Hex(l.range));
        for (int k = 0; k < 4; ++k) R(p + "color." + Num(k), Hex(l.color[k]));
        R(p + "culling_mask", Num(l.culling_mask));
        R(p + "has_spot_angle", Num(l.has_spot_angle));
        R(p + "spot_angle", Hex(l.spot_angle));
    }

    void Camera(int i) {
        kspcore_model_camera c = {};
        c.struct_size = sizeof c;
        kspcore_model_get_camera(m_, i, &c);
        const std::string p = "camera." + Num(i) + ".";
        R(p + "node", Num(c.node));
        R(p + "clear_flags", Num(c.clear_flags));
        for (int k = 0; k < 4; ++k) R(p + "background." + Num(k), Hex(c.background_color[k]));
        R(p + "culling_mask", Num(c.culling_mask));
        R(p + "orthographic", Num(c.orthographic));
        R(p + "fov", Hex(c.field_of_view));
        R(p + "near", Hex(c.near_clip_plane));
        R(p + "far", Hex(c.far_clip_plane));
        R(p + "depth", Hex(c.depth));
    }

    void Animation(int i) {
        kspcore_model_animation a = {};
        a.struct_size = sizeof a;
        kspcore_model_get_animation(m_, i, &a);
        const std::string p = "animation." + Num(i) + ".";
        R(p + "node", Num(a.node));
        R(p + "first_clip", Num(a.first_clip));
        R(p + "clip_count", Num(a.clip_count));
        R(p + "default_clip", Esc(a.default_clip));
        R(p + "default_clip_applied", Num(a.default_clip_applied));
        R(p + "play_automatically", Num(a.play_automatically));
    }

    void Clip(int i) {
        kspcore_model_clip c = {};
        c.struct_size = sizeof c;
        kspcore_model_get_clip(m_, i, &c);
        const std::string p = "clip." + Num(i) + ".";
        R(p + "animation", Num(c.animation));
        R(p + "name", Esc(c.name));
        for (int k = 0; k < 6; ++k) R(p + "bounds." + Num(k), Hex(c.bounds[k]));
        R(p + "wrap_mode", Num(c.wrap_mode));
        R(p + "dropped_by_ksp", Num(c.dropped_by_ksp));
        R(p + "curve_count", Num(c.curve_count));
        R(p + "length", Hex(c.length));
        for (int j = 0; j < c.curve_count; ++j) {
            kspcore_model_curve cv = {};
            cv.struct_size = sizeof cv;
            kspcore_model_get_curve(m_, i, j, &cv);
            const std::string q = p + "curve." + Num(j) + ".";
            R(q + "path", Esc(cv.path));
            R(q + "property", Esc(cv.property));
            R(q + "type", Num(cv.type));
            R(q + "pre_wrap_mode", Num(cv.pre_wrap_mode));
            R(q + "post_wrap_mode", Num(cv.post_wrap_mode));
            R(q + "key_count", Num(cv.key_count));
            if (full_) {
                for (int k = 0; k < cv.key_count; ++k) {
                    const kspcore_model_key& key = cv.keys[k];
                    R(q + "key." + Num(k), Hex(key.time) + "," + Hex(key.value) + "," + Hex(key.in_tangent) + "," + Hex(key.out_tangent) + "," + Num(key.unused));
                }
            } else {
                R(q + "keys", Num(cv.key_count) + ":" + Hex64(Fnv(cv.keys, sizeof(kspcore_model_key) * static_cast<size_t>(cv.key_count))));
            }
        }
    }

    const kspcore_model* m_;
    bool full_;
    Rows rows_;
};

inline Rows Dump(const kspcore_model* m, bool full) { return Dumper(m, full).Run(); }

inline uint64_t Digest(const Rows& rows) {
    uint64_t h = 0xcbf29ce484222325ull;
    for (const auto& kv : rows) {
        h = Fnv(kv.first.data(), kv.first.size(), h);
        h = Fnv("=", 1, h);
        h = Fnv(kv.second.data(), kv.second.size(), h);
        h = Fnv("\n", 1, h);
    }
    return h;
}

}  // namespace mudump
