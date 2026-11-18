/*
 * WP-UE-10 ctest capi.part_rig: pure C consumer of the "part models" calls of kspcore_game.h, as the Unreal shell uses them: for every
 * part of the database it reads the rig (PartLoader.CompileModel's hierarchy), loads the rig's .mu files through kspcore_model.h and
 * checks that every rig node is the .mu node it claims to be. The two sides number the nodes independently (the rig walks the model
 * tree while it builds, kspcore_model numbers a preorder walk of its own load of the file), so agreement of name, parent and local
 * transform at every node checks the mapping the shell relies on.
 *
 * On the repository's GameData mirror only the parts with a reduced model (reduced_model_dir, the stock engines of WP-mucol) have a rig.
 *
 * argv: 1 game_root  2 data_dir  3 reduced_model_dir  4 saves_dir
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kspcore/kspcore_c.h"
#include "kspcore/kspcore_model.h"

static int failures = 0;
#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            ++failures;                                                \
        }                                                              \
    } while (0)

static int same_floats(const float* a, const float* b, int n) { return memcmp(a, b, (size_t)n * sizeof(float)) == 0; }

int main(int argc, char** argv) {
    kspcore_game_config cfg;
    int parts, rigs = 0, nodes_total = 0, files_total = 0, replacements = 0, textures = 0, textures_resolved = 0;
    uint32_t id;

    if (argc < 5) {
        printf("usage: capi_part_rig game_root data_dir reduced_model_dir saves_dir\n");
        return 2;
    }
    memset(&cfg, 0, sizeof cfg);
    cfg.struct_size = sizeof cfg;
    cfg.game_root = argv[1];
    cfg.data_dir = argv[2];
    cfg.reduced_model_dir = argv[3];
    cfg.saves_dir = argv[4];

    /* before the database: no rig, no texture, no crash */
    CHECK(kspcore_game_part_rig_node_count(1) == 0);
    CHECK(kspcore_game_texture_file("Squad/Parts/Engine/liquidEngineLV-T45/model000") == NULL);

    CHECK(kspcore_game_init(&cfg) == 1);
    CHECK(kspcore_game_load_database(NULL, NULL) == 1);
    parts = kspcore_game_part_count();
    CHECK(parts > 100);

    /* model ids are 1 .. part count (0 = unknown) */
    CHECK(kspcore_game_part_rig_node_count(0) == 0);
    CHECK(kspcore_game_part_model_file_count(0) == 0);
    for (id = 1; id <= (uint32_t)parts; ++id) {
        const int n = kspcore_game_part_rig_node_count(id);
        const int nfiles = kspcore_game_part_model_file_count(id);
        kspcore_model** models;
        int* used;
        int i, k;
        if (n == 0) {
            CHECK(nfiles == 0);
            continue;
        }
        ++rigs;
        nodes_total += n;
        files_total += nfiles;
        CHECK(nfiles >= 1);
        models = (kspcore_model**)calloc((size_t)nfiles, sizeof(kspcore_model*));
        used = (int*)calloc((size_t)nfiles, sizeof(int));
        for (k = 0; k < nfiles; ++k) {
            kspcore_part_model_file f;
            kspcore_model_info info;
            int t;
            memset(&f, 0, sizeof f);
            f.struct_size = sizeof f;
            CHECK(kspcore_game_part_model_file(id, k, &f) == 1);
            CHECK(f.path != NULL && f.parent_url != NULL);
            if (!f.path) continue;
            models[k] = kspcore_model_load_file(f.path, f.parent_url);
            CHECK(models[k] != NULL);
            if (!models[k]) {
                printf("%s: %s: %s\n", kspcore_game_model_name(id), f.path, kspcore_model_last_error());
                continue;
            }
            for (t = 0; t < f.texture_replacement_count; ++t) {
                const char *original = NULL, *url = NULL;
                CHECK(kspcore_game_part_texture_replacement(id, k, t, &original, &url) == 1);
                CHECK(original != NULL && url != NULL && original[0] != '\0' && url[0] != '\0');
                /* the loader accepts a replacement only when the new texture exists */
                CHECK(url != NULL && kspcore_game_texture_file(url) != NULL);
                ++replacements;
            }
            CHECK(kspcore_game_part_texture_replacement(id, k, f.texture_replacement_count, NULL, NULL) == 0);
            memset(&info, 0, sizeof info);
            info.struct_size = sizeof info;
            CHECK(kspcore_model_get_info(models[k], &info) == 1);
            for (t = 0; t < info.texture_count; ++t) {
                kspcore_model_texture tex;
                memset(&tex, 0, sizeof tex);
                tex.struct_size = sizeof tex;
                CHECK(kspcore_model_get_texture(models[k], t, &tex) == 1);
                ++textures;
                if (tex.url && kspcore_game_texture_file(tex.url) != NULL) ++textures_resolved;
            }
        }
        for (i = 0; i < n; ++i) {
            kspcore_part_rig_node r;
            memset(&r, 0, sizeof r);
            r.struct_size = sizeof r;
            CHECK(kspcore_game_part_rig_node(id, i, &r) == 1);
            CHECK(r.struct_size == sizeof r && r.name != NULL);
            if (i == 0) {
                /* "model": the child of the part transform, scaled by rescaleFactor */
                CHECK(r.parent == -1 && r.name && strcmp(r.name, "model") == 0);
                CHECK(r.position[0] == 0.0f && r.position[1] == 0.0f && r.position[2] == 0.0f);
                CHECK(r.scale[0] == r.scale[1] && r.scale[1] == r.scale[2] && r.scale[0] > 0.0f);
            } else {
                CHECK(r.parent >= 0 && r.parent < i); /* preorder: a parent comes first */
            }
            if (r.file < 0) {
                CHECK(i == 0 && r.file_node == -1);
                continue;
            }
            CHECK(r.file < nfiles);
            if (r.file >= nfiles || !models[r.file]) continue;
            ++used[r.file];
            {
                kspcore_model_node m;
                memset(&m, 0, sizeof m);
                m.struct_size = sizeof m;
                CHECK(kspcore_model_get_node(models[r.file], r.file_node, &m) == 1);
                if (m.parent < 0) {
                    /* the root of a .mu: placed by the loader (name, position, rotation and scale come from the part cfg) */
                    CHECK(r.file_node == 0);
                } else {
                    kspcore_part_rig_node p;
                    memset(&p, 0, sizeof p);
                    p.struct_size = sizeof p;
                    CHECK(r.parent >= 0 && kspcore_game_part_rig_node(id, r.parent, &p) == 1);
                    CHECK(p.file == r.file && p.file_node == m.parent);
                    CHECK(m.name != NULL && r.name != NULL && strcmp(m.name, r.name) == 0);
                    CHECK(same_floats(m.position, r.position, 3));
                    CHECK(same_floats(m.rotation, r.rotation, 4));
                    CHECK(same_floats(m.scale, r.scale, 3));
                }
            }
        }
        {
            kspcore_part_rig_node r;
            r.struct_size = sizeof r;
            CHECK(kspcore_game_part_rig_node(id, n, &r) == 0);
            CHECK(kspcore_game_part_rig_node(id, -1, &r) == 0);
        }
        for (k = 0; k < nfiles; ++k) {
            if (models[k]) {
                kspcore_model_info info;
                memset(&info, 0, sizeof info);
                info.struct_size = sizeof info;
                kspcore_model_get_info(models[k], &info);
                /* an instance the rig uses appears whole, exactly once per instance; a file it does not use has no node */
                CHECK(used[k] == 0 || used[k] == info.node_count);
                kspcore_model_release(models[k]);
            }
        }
        free(models);
        free(used);
    }
    printf("%d parts, %d with a rig: %d rig nodes, %d model files, %d texture replacements, %d of %d model textures resolved to a file\n", parts, rigs,
           nodes_total, files_total, replacements, textures_resolved, textures);
    /* the reduced models of the mirror (19 staged stock engines) or, on a real game root, every part */
    CHECK(rigs >= 10);
    CHECK(kspcore_game_texture_file("no/such/texture") == NULL);
    CHECK(kspcore_game_texture_file(NULL) == NULL);
    kspcore_game_shutdown();

    if (failures) {
        printf("%d check(s) FAILED\n", failures);
        return 1;
    }
    printf("capi.part_rig OK\n");
    return 0;
}
