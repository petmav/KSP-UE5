// WP-UE-05 unit tests: the .mu render reader and the texture header decode, through the C ABI (include/kspcore/kspcore_model.h).
//
// Gate: tools/wpue05/mu_render_reference.py is an independent Python parser written from PartReader.cs and the DatabaseLoaderTexture_*
// loaders; it shares no code with the C++. It writes tests/data/wpue05/*.mu (SYNTHETIC coverage models: labelled test fixtures with every
// render chunk layout and version branch, NOT game assets) and tests/data/wpue05_reference.json: for each model the full list of
// (key, value) rows (every float as its IEEE bit pattern), every prefix of a small model (accepted or refused + digest), six malformed
// inputs, the expected header record of 55 synthetic texture headers and of the GameData mirror's 897 header-only DDS / MBM stubs.
// The C++ side dumps the same rows through the C ABI (tests/mu_dump.hpp) and the tests compare value for value (bit patterns: 0 ULP).
// The real game files are swept locally (tools/wpue05/sweep_stock.py, STATUS.md "WP-UE-05").
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "kspcore/kspcore_model.h"
#include "mu_dump.hpp"
#include "nlohmann/json.hpp"

using nlohmann::json;

namespace {

const std::string kData = std::string(KSPCORE_TEST_DATA_DIR) + "/";

const json& Reference() {
    static const json ref = [] {
        std::ifstream f(kData + "wpue05_reference.json", std::ios::binary);
        REQUIRE(f.good());
        return json::parse(f);
    }();
    return ref;
}

std::vector<uint8_t> ReadAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    REQUIRE_MESSAGE(f.good(), path);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

std::vector<uint8_t> FromHex(const std::string& hex) {
    std::vector<uint8_t> out(hex.size() / 2);
    for (size_t i = 0; i < out.size(); ++i) out[i] = static_cast<uint8_t>(std::stoul(hex.substr(2 * i, 2), nullptr, 16));
    return out;
}

size_t g_compared = 0;

// Compares the dumped rows with the reference rows: same count, same key, same value at every position.
size_t CompareRows(const std::string& label, const mudump::Rows& got, const json& want) {
    size_t mismatches = 0;
    CHECK_MESSAGE(got.size() == want.size(), label << ": " << got.size() << " rows, reference " << want.size());
    const size_t n = std::min(got.size(), want.size());
    for (size_t i = 0; i < n; ++i) {
        ++g_compared;
        if (got[i].first != want[i][0].get<std::string>() || got[i].second != want[i][1].get<std::string>()) {
            if (++mismatches <= 5)
                MESSAGE(label << " row " << i << ": got " << got[i].first << "=" << got[i].second << ", reference " << want[i][0].get<std::string>()
                              << "=" << want[i][1].get<std::string>());
        }
    }
    CHECK_MESSAGE(mismatches == 0, label << ": " << mismatches << " mismatching rows");
    return mismatches;
}

struct ModelPtr {
    kspcore_model* m = nullptr;
    explicit ModelPtr(kspcore_model* p) : m(p) {}
    ~ModelPtr() { kspcore_model_release(m); }
    ModelPtr(const ModelPtr&) = delete;
};

std::string HexU64(uint64_t v) { return mudump::Hex64(v); }

}  // namespace

TEST_CASE("WP-UE-05 synthetic coverage models: every row of the reader's output matches the independent reference bit for bit") {
    const json& ref = Reference();
    REQUIRE(ref["models"].size() == 7);
    size_t rows = 0;
    for (const json& mj : ref["models"]) {
        const std::string file = mj["file"];
        const std::string parent = mj["parent_url"];
        SUBCASE(file.c_str()) {
            const std::vector<uint8_t> bytes = ReadAll(kData + "wpue05/" + file);
            REQUIRE(bytes.size() == mj["size"].get<size_t>());
            ModelPtr h(kspcore_model_load_memory(bytes.data(), bytes.size(), parent.empty() ? nullptr : parent.c_str()));
            REQUIRE_MESSAGE(h.m != nullptr, file << ": " << kspcore_model_last_error());
            const mudump::Rows full = mudump::Dump(h.m, true);
            rows += full.size();
            CompareRows(file + " (full)", full, mj["rows"]);
            const mudump::Rows compact = mudump::Dump(h.m, false);
            CompareRows(file + " (compact)", compact, mj["compact"]);
            CHECK(HexU64(mudump::Digest(compact)) == mj["digest"].get<std::string>());
            // the same through ReadFile
            ModelPtr h2(kspcore_model_load_file((kData + "wpue05/" + file).c_str(), parent.empty() ? nullptr : parent.c_str()));
            REQUIRE(h2.m != nullptr);
            CHECK(HexU64(mudump::Digest(mudump::Dump(h2.m, false))) == mj["digest"].get<std::string>());
        }
    }
    MESSAGE("rows compared so far in this process: " << g_compared);
}

TEST_CASE("WP-UE-05 model facts read straight from the spec (not from the reference)") {
    // syn_v5_full.mu: version 5, 8 nodes; the layout is documented at the writer (tools/wpue05/mu_render_reference.py model_v5_full)
    const std::vector<uint8_t> bytes = ReadAll(kData + "wpue05/syn_v5_full.mu");
    ModelPtr h(kspcore_model_load_memory(bytes.data(), bytes.size(), "Squad/Parts/Syn"));
    REQUIRE(h.m);
    kspcore_model_info info = {};
    info.struct_size = sizeof info;
    REQUIRE(kspcore_model_get_info(h.m, &info));
    CHECK(info.file_type == 76543);
    CHECK(info.version == 5);
    CHECK(info.node_count == 8);        // model, body, a, a, b, empty, skin, ignored
    CHECK(info.mesh_count == 4);        // MeshFilters of body, empty, ignored and the skinned renderer's mesh (see the writer)
    CHECK(info.renderer_count == 4);
    CHECK(info.material_count == 4);
    CHECK(info.texture_count == 4);
    CHECK(info.light_count == 1);
    CHECK(info.camera_count == 1);
    CHECK(info.particle_emitter_count == 1);
    CHECK(info.collider_count == 2);
    CHECK(info.clip_count == 2);
    kspcore_model_node n = {};
    n.struct_size = sizeof n;
    REQUIRE(kspcore_model_get_node(h.m, 0, &n));
    CHECK(std::string(n.name) == "model");
    CHECK(n.parent == -1);
    CHECK(n.child_count == 5);
    REQUIRE(kspcore_model_get_node(h.m, 4, &n));
    CHECK(std::string(n.name) == "b");
    CHECK(n.parent == 3);
    // materials: shader names are what the file stores (version >= 4)
    kspcore_model_material mat = {};
    mat.struct_size = sizeof mat;
    REQUIRE(kspcore_model_get_material(h.m, 1, &mat));
    CHECK(std::string(mat.shader_name) == "KSP/Emissive/Bumped Specular");
    CHECK(mat.legacy_shader_type == -1);
    CHECK(mat.property_count == 6);
    kspcore_model_texture tex = {};
    tex.struct_size = sizeof tex;
    REQUIRE(kspcore_model_get_texture(h.m, 1, &tex));
    CHECK(std::string(tex.file) == "bodyNRM.png");
    CHECK(std::string(tex.name) == "bodyNRM");
    CHECK(std::string(tex.url) == "Squad/Parts/Syn/bodyNRM");
    CHECK(tex.type == 1);
    REQUIRE(kspcore_model_get_texture(h.m, 0, &tex));
    CHECK(std::string(tex.name) == "body");  // "Squad/Parts/body.dds": directory and extension stripped (Path.GetFileNameWithoutExtension)
    REQUIRE(kspcore_model_get_texture(h.m, 3, &tex));
    CHECK(std::string(tex.name) == "a.b.alpha");  // only the last extension goes
    // the skinned renderer: bone names b / body / missing / a resolve to the FIRST node of that name in preorder (PartReader.cs:1068)
    kspcore_model_renderer r = {};
    r.struct_size = sizeof r;
    int skinned = -1;
    for (int i = 0; i < info.renderer_count; ++i) {
        REQUIRE(kspcore_model_get_renderer(h.m, i, &r));
        if (r.kind == KSPCORE_RENDERER_SKINNED) skinned = i;
    }
    REQUIRE(skinned >= 0);
    REQUIRE(kspcore_model_get_renderer(h.m, skinned, &r));
    REQUIRE(r.bone_count == 4);
    CHECK(r.bone_nodes[0] == 4);   // b
    CHECK(r.bone_nodes[1] == 1);   // body
    CHECK(r.bone_nodes[2] == -1);  // missing
    CHECK(r.bone_nodes[3] == 2);   // a: nodes 2 and 3 are both "a"; the first wins
    // the animation: clip "lights" has a ParticleEmitter-free set of curves; both clips kept; default "open" applied
    kspcore_model_animation a = {};
    a.struct_size = sizeof a;
    REQUIRE(kspcore_model_get_animation(h.m, 0, &a));
    CHECK(a.clip_count == 2);
    CHECK(a.default_clip_applied == 1);
    kspcore_model_clip c = {};
    c.struct_size = sizeof c;
    REQUIRE(kspcore_model_get_clip(h.m, 0, &c));
    CHECK(std::string(c.name) == "open");
    CHECK(c.curve_count == 2);
    CHECK(c.dropped_by_ksp == 0);
    // lighting: version 5 stores the spot angle
    kspcore_model_light l = {};
    l.struct_size = sizeof l;
    REQUIRE(kspcore_model_get_light(h.m, 0, &l));
    CHECK(l.has_spot_angle == 1);
}

TEST_CASE("WP-UE-05 version branches and PartReader quirks") {
    auto load = [](const char* file, const char* parent = nullptr) {
        const std::vector<uint8_t> bytes = ReadAll(kData + "wpue05/" + file);
        return kspcore_model_load_memory(bytes.data(), bytes.size(), parent);
    };
    SUBCASE("version 0: no shadow fields, no spot angle, legacy materials") {
        ModelPtr h(load("syn_v0_old.mu"));
        REQUIRE(h.m);
        kspcore_model_renderer r = {};
        r.struct_size = sizeof r;
        REQUIRE(kspcore_model_get_renderer(h.m, 0, &r));
        CHECK(r.has_shadow_fields == 0);
        CHECK(r.cast_shadows == 1);  // the AddComponent defaults stay
        CHECK(r.receive_shadows == 1);
        kspcore_model_light l = {};
        l.struct_size = sizeof l;
        REQUIRE(kspcore_model_get_light(h.m, 0, &l));
        CHECK(l.has_spot_angle == 0);
        CHECK(l.spot_angle == 30.0f);
        kspcore_model_material m = {};
        m.struct_size = sizeof m;
        REQUIRE(kspcore_model_get_material(h.m, 1, &m));
        CHECK(std::string(m.shader_name) == "KSP/Specular");  // ShaderType 2
        CHECK(m.legacy_shader_type == 2);
        CHECK(m.property_count == 3);  // _MainTex, _SpecColor, _Shininess
    }
    SUBCASE("version 1: shadow fields stored, light still without a spot angle") {
        ModelPtr h(load("syn_v1_shadow.mu"));
        REQUIRE(h.m);
        kspcore_model_renderer r = {};
        r.struct_size = sizeof r;
        REQUIRE(kspcore_model_get_renderer(h.m, 0, &r));
        CHECK(r.has_shadow_fields == 1);
        CHECK(r.cast_shadows == 0);
        CHECK(r.receive_shadows == 1);
        kspcore_model_light l = {};
        l.struct_size = sizeof l;
        REQUIRE(kspcore_model_get_light(h.m, 0, &l));
        CHECK(l.has_spot_angle == 0);
    }
    SUBCASE("version 3: all 17 ShaderTypes, Custom and unknown values map to KSP/Diffuse") {
        ModelPtr h(load("syn_v3_legacy.mu"));
        REQUIRE(h.m);
        const char* expect[19] = {"KSP/Diffuse", "KSP/Diffuse", "KSP/Specular", "KSP/Bumped", "KSP/Bumped Specular", "KSP/Emissive/Diffuse",
                                  "KSP/Emissive/Specular", "KSP/Emissive/Bumped Specular", "KSP/Alpha/Cutoff", "KSP/Alpha/Cutoff Bumped",
                                  "KSP/Alpha/Translucent", "KSP/Alpha/Translucent Specular", "KSP/Alpha/Unlit Transparent", "KSP/Unlit",
                                  "KSP/Particles/Alpha Blended", "KSP/Particles/Additive", "KSP/Bumped Specular (Mapped)", "KSP/Diffuse",
                                  "KSP/Diffuse"};
        for (int i = 0; i < 19; ++i) {
            kspcore_model_material m = {};
            m.struct_size = sizeof m;
            REQUIRE(kspcore_model_get_material(h.m, i, &m));
            CHECK_MESSAGE(std::string(m.shader_name) == expect[i], "material " << i);
        }
    }
    SUBCASE("version 2: a ParticleEmitter curve makes KSP skip its clip, every later clip and the default clip") {
        ModelPtr h(load("syn_v2_anim.mu"));
        REQUIRE(h.m);
        kspcore_model_info info = {};
        info.struct_size = sizeof info;
        REQUIRE(kspcore_model_get_info(h.m, &info));
        // component 0: clips ok / drop / after / "" ; the curve type 4 is in "drop"
        kspcore_model_clip c = {};
        c.struct_size = sizeof c;
        const int expectDropped[4] = {0, 1, 1, 1};
        for (int i = 0; i < 4; ++i) {
            REQUIRE(kspcore_model_get_clip(h.m, i, &c));
            CHECK_MESSAGE(c.dropped_by_ksp == expectDropped[i], "clip " << i);
        }
        kspcore_model_animation a = {};
        a.struct_size = sizeof a;
        REQUIRE(kspcore_model_get_animation(h.m, 0, &a));
        CHECK(std::string(a.default_clip) == "after");
        CHECK(a.default_clip_applied == 0);
        REQUIRE(kspcore_model_get_animation(h.m, 1, &a));
        CHECK(a.default_clip_applied == 1);
        CHECK(a.play_automatically == 0);
        REQUIRE(kspcore_model_get_animation(h.m, 2, &a));
        CHECK(std::string(a.default_clip).empty());
        CHECK(a.default_clip_applied == 0);
    }
    SUBCASE("version 4: accepted anomalies are recorded as warnings") {
        ModelPtr h(load("syn_v4_warn.mu"));
        REQUIRE(h.m);
        kspcore_model_info info = {};
        info.struct_size = sizeof info;
        REQUIRE(kspcore_model_get_info(h.m, &info));
        CHECK(info.warning_count == 3);
        CHECK(info.material_count == 3);
        kspcore_model_node n = {};
        n.struct_size = sizeof n;
        REQUIRE(kspcore_model_get_node(h.m, 0, &n));
        CHECK(std::string(n.name).size() == 200);  // a two-byte 7-bit length prefix
        kspcore_model_texture t = {};
        t.struct_size = sizeof t;
        REQUIRE(kspcore_model_get_texture(h.m, 0, &t));
        CHECK(std::string(t.name) == "x.y");  // a Windows path with two extensions
    }
}

TEST_CASE("WP-UE-05 every prefix of a model: accepted or refused exactly as the independent parse says") {
    const json& t = Reference()["truncation"];
    const std::vector<uint8_t> bytes = ReadAll(kData + "wpue05/" + t["file"].get<std::string>());
    size_t ok = 0, refused = 0;
    for (const json& row : t["prefixes"]) {
        const size_t n = row[0].get<size_t>();
        REQUIRE(n <= bytes.size());
        ModelPtr h(kspcore_model_load_memory(bytes.data(), n, nullptr));
        const bool wantOk = row[1].get<int>() == 1;
        ++g_compared;
        if (wantOk != (h.m != nullptr)) {
            FAIL_CHECK("prefix of " << n << " bytes: reference " << (wantOk ? "accepts" : "refuses") << ", reader " << (h.m ? "accepts" : "refuses"));
            continue;
        }
        if (h.m) {
            ++ok;
            CHECK_MESSAGE(HexU64(mudump::Digest(mudump::Dump(h.m, false))) == row[2].get<std::string>(), "prefix " << n);
        } else {
            ++refused;
            CHECK(std::string(kspcore_model_last_error()).size() > 0);
        }
    }
    MESSAGE(ok << " prefixes accepted, " << refused << " refused (of " << t["prefixes"].size() << ")");
    CHECK(ok >= 5);
    CHECK(refused > 100);
}

TEST_CASE("WP-UE-05 malformed models are refused with a message") {
    for (const json& e : Reference()["errors"]) {
        const std::vector<uint8_t> bytes = FromHex(e["hex"].get<std::string>());
        ModelPtr h(kspcore_model_load_memory(bytes.data(), bytes.size(), nullptr));
        CHECK_MESSAGE(h.m == nullptr, e["label"].get<std::string>());
        CHECK_MESSAGE(std::string(kspcore_model_last_error()).size() > 0, e["label"].get<std::string>());
    }
    ModelPtr missing(kspcore_model_load_file((kData + "wpue05/does_not_exist.mu").c_str(), nullptr));
    CHECK(missing.m == nullptr);
    CHECK(std::string(kspcore_model_last_error()).find("cannot open") != std::string::npos);
    ModelPtr nullpath(kspcore_model_load_file(nullptr, nullptr));
    CHECK(nullpath.m == nullptr);
}

TEST_CASE("WP-UE-05 C ABI protocol: struct_size, index checks, null handles") {
    const std::vector<uint8_t> bytes = ReadAll(kData + "wpue05/syn_v5_full.mu");
    ModelPtr h(kspcore_model_load_memory(bytes.data(), bytes.size(), nullptr));
    REQUIRE(h.m);
    kspcore_model_node n = {};
    std::memset(&n, 0xAB, sizeof n);
    // an older caller with a shorter struct gets the leading fields only and keeps its struct_size
    n.struct_size = static_cast<uint32_t>(offsetof(kspcore_model_node, child_count));
    REQUIRE(kspcore_model_get_node(h.m, 1, &n));
    CHECK(n.struct_size == offsetof(kspcore_model_node, child_count));
    CHECK(std::string(n.name) == "body");
    CHECK(n.parent == 0);
    uint8_t pattern[sizeof(n) - offsetof(kspcore_model_node, child_count)];
    std::memset(pattern, 0xAB, sizeof pattern);
    CHECK(std::memcmp(reinterpret_cast<const uint8_t*>(&n) + offsetof(kspcore_model_node, child_count), pattern, sizeof pattern) == 0);
    n.struct_size = 0;
    CHECK(kspcore_model_get_node(h.m, 1, &n) == 0);
    n.struct_size = sizeof n;
    CHECK(kspcore_model_get_node(h.m, -1, &n) == 0);
    CHECK(kspcore_model_get_node(h.m, 8, &n) == 0);
    CHECK(kspcore_model_get_node(nullptr, 0, &n) == 0);
    CHECK(kspcore_model_get_node(h.m, 0, nullptr) == 0);
    CHECK(kspcore_model_node_child(h.m, 0, 5) == -1);
    CHECK(kspcore_model_node_child(h.m, 0, 4) == 7);
    int32_t count = 0;
    CHECK(kspcore_model_mesh_submesh(h.m, 0, 3, &count) == nullptr);
    const int32_t* t = kspcore_model_mesh_submesh(h.m, 0, 0, &count);
    REQUIRE(t != nullptr);
    CHECK(count == 6);
    CHECK(kspcore_model_renderer_bone_name(h.m, 0, 0) == nullptr);  // not a skinned renderer
    CHECK(kspcore_model_warning(h.m, 0) == nullptr);
    kspcore_model_release(nullptr);  // no-op
    kspcore_model_info info = {};
    info.struct_size = sizeof info;
    CHECK(kspcore_model_get_info(nullptr, &info) == 0);
    // a mesh without normals reports NULL arrays and clear flags
    kspcore_model_mesh mesh = {};
    mesh.struct_size = sizeof mesh;
    REQUIRE(kspcore_model_get_mesh(h.m, 0, &mesh));
    CHECK((mesh.flags & KSPCORE_MESH_HAS_NORMALS) != 0);
    CHECK(mesh.vertex_count == 7);
    CHECK(mesh.submesh_count == 3);
    CHECK(mesh.bone_indices == nullptr);
    CHECK(mesh.bind_poses == nullptr);
}

namespace {

const char* const kFields[] = {"container", "valid", "loadable", "width", "height", "source_format", "unity_format", "compressed", "mip_chain",
                               "mip_count", "is_normal_map", "is_readable", "is_compressed_flag", "nrm_suffix", "has_alpha",
                               "dds_header_flags", "dds_pixel_format_flags", "dds_fourcc", "dds_caps", "dds_mip_map_count",
                               "dds_rgb_bit_count", "dds_dxgi_format", "mbm_texture_type", "mbm_bits_per_pixel", "png_color_type",
                               "png_bit_depth", "tga_image_type", "tga_pixel_depth", "data_offset", "data_bytes", "expected_data_bytes"};

// the struct's value of a named field (as a signed 64 bit number: the reference stores unsigned 32 bit values as numbers)
long long FieldOf(const kspcore_texture_header& h, const std::string& f) {
    if (f == "container") return h.container;
    if (f == "valid") return h.valid;
    if (f == "loadable") return h.loadable;
    if (f == "width") return h.width;
    if (f == "height") return h.height;
    if (f == "source_format") return h.source_format;
    if (f == "unity_format") return h.unity_format;
    if (f == "compressed") return h.compressed;
    if (f == "mip_chain") return h.mip_chain;
    if (f == "mip_count") return h.mip_count;
    if (f == "is_normal_map") return h.is_normal_map;
    if (f == "is_readable") return h.is_readable;
    if (f == "is_compressed_flag") return h.is_compressed_flag;
    if (f == "nrm_suffix") return h.nrm_suffix;
    if (f == "has_alpha") return h.has_alpha;
    if (f == "dds_header_flags") return h.dds_header_flags;
    if (f == "dds_pixel_format_flags") return h.dds_pixel_format_flags;
    if (f == "dds_fourcc") return h.dds_fourcc;
    if (f == "dds_caps") return h.dds_caps;
    if (f == "dds_mip_map_count") return h.dds_mip_map_count;
    if (f == "dds_rgb_bit_count") return h.dds_rgb_bit_count;
    if (f == "dds_dxgi_format") return h.dds_dxgi_format;
    if (f == "mbm_texture_type") return h.mbm_texture_type;
    if (f == "mbm_bits_per_pixel") return h.mbm_bits_per_pixel;
    if (f == "png_color_type") return h.png_color_type;
    if (f == "png_bit_depth") return h.png_bit_depth;
    if (f == "tga_image_type") return h.tga_image_type;
    if (f == "tga_pixel_depth") return h.tga_pixel_depth;
    if (f == "data_offset") return static_cast<long long>(h.data_offset);
    if (f == "data_bytes") return static_cast<long long>(h.data_bytes);
    if (f == "expected_data_bytes") return static_cast<long long>(h.expected_data_bytes);
    return -999999;
}

size_t CompareHeader(const std::string& label, const kspcore_texture_header& h, const json& expect) {
    size_t bad = 0;
    for (const char* f : kFields) {
        ++g_compared;
        if (FieldOf(h, f) != expect[f].get<long long>()) {
            ++bad;
            if (bad <= 3) MESSAGE(label << ": " << f << " got " << FieldOf(h, f) << ", reference " << expect[f].get<long long>());
        }
    }
    ++g_compared;
    if (std::string(h.error) != expect["error"].get<std::string>()) {
        ++bad;
        MESSAGE(label << ": error '" << h.error << "', reference '" << expect["error"].get<std::string>() << "'");
    }
    return bad;
}

}  // namespace

TEST_CASE("WP-UE-05 texture headers: 55 synthetic headers (DDS, MBM, PNG, JPEG, TGA, TRUECOLOR) match the independent reference") {
    size_t bad = 0;
    for (const json& c : Reference()["textures"]) {
        const std::string name = c["name"];
        const std::vector<uint8_t> bytes = FromHex(c["hex"].get<std::string>());
        kspcore_texture_header h = {};
        h.struct_size = sizeof h;
        const int rc = kspcore_texture_decode_header(name.c_str(), bytes.data(), bytes.size(), c["file_size"].get<uint64_t>(), &h);
        CHECK_MESSAGE(rc == c["expect"]["valid"].get<int>(), name);
        bad += CompareHeader(name, h, c["expect"]);
    }
    CHECK(bad == 0);
    // argument checks
    kspcore_texture_header h = {};
    h.struct_size = sizeof h;
    CHECK(kspcore_texture_decode_header(nullptr, nullptr, 0, 0, &h) == -1);
    CHECK(kspcore_texture_decode_header("a.dds", nullptr, 4, 0, &h) == -1);
    CHECK(kspcore_texture_decode_header("a.dds", nullptr, 0, 0, nullptr) == -1);
}

TEST_CASE("WP-UE-05 texture headers: the GameData mirror's 895 DDS and 2 MBM header stubs") {
    const json& m = Reference()["mirror_textures"];
    const std::vector<std::string> fields = m["fields"].get<std::vector<std::string>>();
    REQUIRE(fields.size() == 3 + sizeof(kFields) / sizeof(kFields[0]) + 1);
    size_t dds = 0, mbm = 0, bad = 0, dxt1 = 0, dxt5 = 0, normal = 0, mips = 0, unloadable = 0;
    for (const json& row : m["rows"]) {
        const std::string path = row[0];
        const std::vector<uint8_t> bytes = ReadAll(std::string(KSPCORE_TESTDATA_DIR) + "/GameData-mirror/" + path);
        REQUIRE_MESSAGE(bytes.size() == row[2].get<size_t>(), path);
        kspcore_texture_header h = {};
        h.struct_size = sizeof h;
        kspcore_texture_decode_header(path.c_str(), bytes.data(), bytes.size(), bytes.size(), &h);
        json expect = json::object();
        for (size_t i = 3; i < fields.size(); ++i) expect[fields[i]] = row[i];
        bad += CompareHeader(path, h, expect);
        if (h.container == KSPCORE_TEX_DDS) {
            ++dds;
            dxt1 += h.unity_format == KSPCORE_TEXFMT_DXT1;
            dxt5 += h.unity_format == KSPCORE_TEXFMT_DXT5;
            normal += h.is_normal_map;
            mips += h.mip_chain == 1;
            unloadable += !h.loadable;
        } else if (h.container == KSPCORE_TEX_MBM) {
            ++mbm;
        }
    }
    MESSAGE("mirror stubs: " << dds << " DDS (" << dxt1 << " DXT1, " << dxt5 << " DXT5, " << unloadable << " not loadable, " << normal << " flagged normal map, " << mips
                             << " with a mip chain), " << mbm << " MBM; " << bad << " mismatches");
    CHECK(bad == 0);
    CHECK(dds == 895);
    CHECK(mbm == 2);
    CHECK(dxt1 + dxt5 + unloadable == dds);
}
