// WP-UE-05 local sweep tool: parses every .mu and decodes the header of every texture under <game root>/GameData through the C ABI
// (include/kspcore/kspcore_model.h) and writes DERIVED data only: per file the path, size, the parse result, the chunk counts, the
// compact row digest (tests/mu_dump.hpp) and the shader names; per texture the header record. It never prints file contents.
// Driver: tools/wpue05/sweep_stock.py (runs this tool and the independent python reference over the same files and cross-checks them).
//
//   mu_sweep --game <KSP root> --out <json>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "mu_dump.hpp"

namespace fs = std::filesystem;

namespace {

std::string J(const std::string& s) {
    std::string o = "\"";
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') {
            o += '\\';
            o += static_cast<char>(c);
        } else if (c < 0x20 || c >= 0x7f) {
            char b[8];
            std::snprintf(b, sizeof b, "\\u%04x", c);
            o += b;
        } else {
            o += static_cast<char>(c);
        }
    }
    return o + "\"";
}

std::vector<uint8_t> ReadAll(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::string Lower(std::string s) {
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return s;
}

}  // namespace

int main(int argc, char** argv) {
    std::string game, out;
    for (int i = 1; i + 1 < argc; i += 2) {
        if (!std::strcmp(argv[i], "--game")) game = argv[i + 1];
        else if (!std::strcmp(argv[i], "--out")) out = argv[i + 1];
    }
    if (game.empty() || out.empty()) {
        std::fprintf(stderr, "usage: mu_sweep --game <KSP root> --out <json>\n");
        return 2;
    }
    const fs::path gd = fs::path(game) / "GameData";
    std::vector<fs::path> mus, texs;
    for (const auto& e : fs::recursive_directory_iterator(gd)) {
        if (!e.is_regular_file()) continue;
        const std::string ext = Lower(e.path().extension().string());
        if (ext == ".mu") mus.push_back(e.path());
        else if (ext == ".dds" || ext == ".mbm" || ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".truecolor") texs.push_back(e.path());
    }
    std::sort(mus.begin(), mus.end());
    std::sort(texs.begin(), texs.end());
    std::string js = "{\"models\":[";
    bool first = true;
    for (const fs::path& p : mus) {
        const std::string rel = fs::relative(p, game).generic_string();
        const std::vector<uint8_t> bytes = ReadAll(p);
        const std::string parentUrl = fs::relative(p.parent_path(), gd).generic_string();
        kspcore_model* m = kspcore_model_load_memory(bytes.data(), bytes.size(), parentUrl.c_str());
        js += first ? "\n" : ",\n";
        first = false;
        js += "{\"path\":" + J(rel) + ",\"size\":" + std::to_string(bytes.size());
        if (!m) {
            js += ",\"ok\":false,\"error\":" + J(kspcore_model_last_error()) + "}";
            continue;
        }
        kspcore_model_info info = {};
        info.struct_size = sizeof info;
        kspcore_model_get_info(m, &info);
        const mudump::Rows rows = mudump::Dump(m, false);
        long long verts = 0, submeshes = 0, triIdx = 0, bindPoses = 0, keys = 0, curves = 0;
        for (int i = 0; i < info.mesh_count; ++i) {
            kspcore_model_mesh me = {};
            me.struct_size = sizeof me;
            kspcore_model_get_mesh(m, i, &me);
            verts += me.vertex_count;
            submeshes += me.submesh_count;
            bindPoses += me.bind_pose_count;
            for (int s = 0; s < me.submesh_count; ++s) {
                int32_t k = 0;
                kspcore_model_mesh_submesh(m, i, s, &k);
                triIdx += k;
            }
        }
        for (int i = 0; i < info.clip_count; ++i) {
            kspcore_model_clip c = {};
            c.struct_size = sizeof c;
            kspcore_model_get_clip(m, i, &c);
            curves += c.curve_count;
            for (int j = 0; j < c.curve_count; ++j) {
                kspcore_model_curve cv = {};
                cv.struct_size = sizeof cv;
                kspcore_model_get_curve(m, i, j, &cv);
                keys += cv.key_count;
            }
        }
        long long skinned = 0, filters = 0, renderers = 0, tags = 0;
        for (int i = 0; i < info.node_count; ++i) {
            kspcore_model_node n = {};
            n.struct_size = sizeof n;
            kspcore_model_get_node(m, i, &n);
            skinned += n.skinned_renderer_count;
            filters += n.mesh_filter_count;
            renderers += n.mesh_renderer_count;
            tags += n.has_tag_and_layer;
        }
        std::map<std::string, int> shaders;
        for (int i = 0; i < info.material_count; ++i) {
            kspcore_model_material mat = {};
            mat.struct_size = sizeof mat;
            kspcore_model_get_material(m, i, &mat);
            ++shaders[mat.shader_name];
        }
        js += ",\"ok\":true,\"version\":" + std::to_string(info.version) + ",\"digest\":" + J(mudump::Hex64(mudump::Digest(rows))) + ",\"rows\":" + std::to_string(rows.size());
        js += ",\"counts\":{\"nodes\":" + std::to_string(info.node_count) + ",\"colliders\":" + std::to_string(info.collider_count) +
              ",\"mesh_filters\":" + std::to_string(filters) + ",\"mesh_renderers\":" + std::to_string(renderers) +
              ",\"skinned_renderers\":" + std::to_string(skinned) + ",\"lights\":" + std::to_string(info.light_count) +
              ",\"cameras\":" + std::to_string(info.camera_count) + ",\"particle_emitters\":" + std::to_string(info.particle_emitter_count) +
              ",\"animations\":" + std::to_string(info.animation_count) + ",\"clips\":" + std::to_string(info.clip_count) +
              ",\"curves\":" + std::to_string(curves) + ",\"keys\":" + std::to_string(keys) + ",\"tag_and_layer\":" + std::to_string(tags) +
              ",\"vertices\":" + std::to_string(verts) + ",\"triangles\":" + std::to_string(triIdx / 3) + ",\"submeshes\":" + std::to_string(submeshes) +
              ",\"bind_poses\":" + std::to_string(bindPoses) + ",\"materials\":" + std::to_string(info.material_count) +
              ",\"textures\":" + std::to_string(info.texture_count) + "}";
        js += ",\"shaders\":{";
        bool f2 = true;
        for (const auto& kv : shaders) {
            js += (f2 ? "" : ",") + J(kv.first) + ":" + std::to_string(kv.second);
            f2 = false;
        }
        js += "},\"ignored\":[";
        for (int i = 0; i < info.ignored_entry_count; ++i) {
            int32_t v = 0, c = 0;
            kspcore_model_ignored_entry(m, i, &v, &c);
            js += (i ? "," : "") + std::string("[") + std::to_string(v) + "," + std::to_string(c) + "]";
        }
        js += "],\"ignored_mesh\":[";
        for (int i = 0; i < info.ignored_mesh_entry_count; ++i) {
            int32_t v = 0, c = 0;
            kspcore_model_ignored_mesh_entry(m, i, &v, &c);
            js += (i ? "," : "") + std::string("[") + std::to_string(v) + "," + std::to_string(c) + "]";
        }
        js += "],\"warnings\":[";
        for (int i = 0; i < info.warning_count; ++i) js += (i ? "," : "") + J(kspcore_model_warning(m, i));
        js += "]}";
        kspcore_model_release(m);
    }
    js += "\n],\"textures\":[";
    first = true;
    for (const fs::path& p : texs) {
        const std::string rel = fs::relative(p, game).generic_string();
        const std::vector<uint8_t> bytes = ReadAll(p);
        kspcore_texture_header h = {};
        h.struct_size = sizeof h;
        kspcore_texture_decode_header(rel.c_str(), bytes.data(), bytes.size(), bytes.size(), &h);
        js += first ? "\n" : ",\n";
        first = false;
        char buf[1024];
        std::snprintf(buf, sizeof buf,
                      "[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%llu,%llu,%llu",
                      h.container, h.valid, h.loadable, h.width, h.height, h.source_format, h.unity_format, h.compressed, h.mip_chain, h.mip_count,
                      h.is_normal_map, h.is_readable, h.is_compressed_flag, h.nrm_suffix, h.has_alpha, h.dds_header_flags, h.dds_pixel_format_flags,
                      h.dds_fourcc, h.dds_caps, h.dds_mip_map_count, h.dds_rgb_bit_count, h.dds_dxgi_format, h.mbm_texture_type, h.mbm_bits_per_pixel,
                      h.png_color_type, h.png_bit_depth, h.tga_image_type, h.tga_pixel_depth, static_cast<unsigned long long>(h.data_offset),
                      static_cast<unsigned long long>(h.data_bytes), static_cast<unsigned long long>(h.expected_data_bytes));
        js += "[" + J(rel) + "," + std::to_string(bytes.size()) + "," + buf + "," + J(h.error) + "]]";
    }
    js += "\n]}\n";
    std::ofstream o(out, std::ios::binary);
    o << js;
    std::printf("mu_sweep: %zu models, %zu textures -> %s\n", mus.size(), texs.size(), out.c_str());
    return 0;
}
