/* WP-UE-05: kspcore_model.h is a C header. This program is compiled as C (no C++ anywhere in the translation unit), links kspcore and
 * walks one model and a few texture headers through the ABI only. Usage: capi_model_c <model.mu> <expected node count> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kspcore/kspcore_model.h"

int main(int argc, char** argv) {
    kspcore_model_info info;
    kspcore_model_node node;
    kspcore_model_mesh mesh;
    kspcore_model_material mat;
    kspcore_model_property prop;
    kspcore_texture_header th;
    unsigned char dds[128];
    kspcore_model* m;
    int i, j, props = 0, expect;
    if (argc < 3) {
        fprintf(stderr, "usage: capi_model_c <model.mu> <node count>\n");
        return 2;
    }
    expect = atoi(argv[2]);
    m = kspcore_model_load_file(argv[1], "Squad/Parts/Syn");
    if (!m) {
        fprintf(stderr, "load failed: %s\n", kspcore_model_last_error());
        return 1;
    }
    memset(&info, 0, sizeof info);
    info.struct_size = sizeof info;
    if (!kspcore_model_get_info(m, &info) || info.node_count != expect) {
        fprintf(stderr, "node count %d, expected %d\n", info.node_count, expect);
        return 1;
    }
    for (i = 0; i < info.node_count; ++i) {
        node.struct_size = sizeof node;
        if (!kspcore_model_get_node(m, i, &node)) return 1;
    }
    for (i = 0; i < info.mesh_count; ++i) {
        mesh.struct_size = sizeof mesh;
        if (!kspcore_model_get_mesh(m, i, &mesh) || mesh.vertex_count < 0) return 1;
    }
    for (i = 0; i < info.material_count; ++i) {
        mat.struct_size = sizeof mat;
        if (!kspcore_model_get_material(m, i, &mat)) return 1;
        for (j = 0; j < mat.property_count; ++j) {
            prop.struct_size = sizeof prop;
            if (!kspcore_model_get_property(m, i, j, &prop)) return 1;
            ++props;
        }
    }
    kspcore_model_release(m);
    /* a DXT5 DDS header: 128 bytes, 64 x 32, no mips */
    memset(dds, 0, sizeof dds);
    memcpy(dds, "DDS ", 4);
    dds[4] = 124;
    dds[12] = 32;
    dds[16] = 64;
    dds[76] = 32;
    dds[80] = 4;
    memcpy(dds + 84, "DXT5", 4);
    th.struct_size = sizeof th;
    if (kspcore_texture_decode_header("a/b.dds", dds, sizeof dds, 0, &th) != 1 || th.width != 64 || th.height != 32 ||
        th.unity_format != KSPCORE_TEXFMT_DXT5 || !th.loadable) {
        fprintf(stderr, "dds header wrong\n");
        return 1;
    }
    printf("capi_model_c ok: %d nodes, %d meshes, %d materials, %d properties\n", info.node_count, info.mesh_count, info.material_count, props);
    return 0;
}
