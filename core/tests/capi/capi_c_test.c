/* Pure C consumer of the kspcore C ABI (include/kspcore/kspcore_c.h). argv[1] = harness/inputs/Physics.cfg. */
#include <stdio.h>
#include <string.h>

#include "kspcore/kspcore_c.h"

static int failures = 0;
#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            ++failures;                                                \
        }                                                              \
    } while (0)

static unsigned bits_of(float f) {
    unsigned u;
    memcpy(&u, &f, sizeof u);
    return u;
}

int main(int argc, char** argv) {
    const char* info = kspcore_build_info();
    printf("%s\n", info);
#ifdef _MSC_VER
    CHECK(strstr(info, "crt=MD ") != NULL);
#endif
    CHECK(strstr(info, "idl=0") != NULL);
    CHECK(kspcore_abi_version() == KSPCORE_ABI_VERSION);

    kspcore_sim* sim = kspcore_sim_create();
    CHECK(sim != NULL);

    /* ConfigNode: parse, query, save */
    kspcore_confignode* root = kspcore_confignode_parse("a = 1 // c\nPART (id7)\n{\nname = x\nkey = 0 0\nkey = 1 1\n}\n");
    CHECK(root != NULL);
    CHECK(strcmp(kspcore_confignode_get_value(root, "a", 0), "1") == 0);
    CHECK(kspcore_confignode_get_value(root, "missing", 0) == NULL);
    kspcore_confignode* part = kspcore_confignode_get_node(root, "PART", 0);
    CHECK(part != NULL);
    CHECK(strcmp(kspcore_confignode_id(part), "id7") == 0);
    {
        char buf[256];
        const size_t n = kspcore_confignode_save_to_buffer(root, buf, sizeof buf);
        CHECK(n == strlen("a = 1\r\nPART\r\n{\r\n\tname = x\r\n\tkey = 0 0\r\n\tkey = 1 1\r\n}\r\n"));
        CHECK(strcmp(buf, "a = 1\r\nPART\r\n{\r\n\tname = x\r\n\tkey = 0 0\r\n\tkey = 1 1\r\n}\r\n") == 0);
        CHECK(kspcore_confignode_save_to_buffer(root, NULL, 0) == n);
    }
    {
        int i = 0;
        CHECK(kspcore_confignode_try_get_int(root, "a", &i) == 1 && i == 1);
    }

    /* FloatCurve from a node */
    kspcore_floatcurve* c = kspcore_floatcurve_load(part);
    CHECK(c != NULL);
    CHECK(kspcore_floatcurve_evaluate(c, 0.5f) == 0.5f);
    kspcore_floatcurve_destroy(c);

    /* Physics.cfg DRAG_TIP curve: bit-exact against harness/golden/curves_eval (PhysicsGlobals.dragCurveTip) */
    if (argc > 1) {
        kspcore_confignode* phys = kspcore_confignode_load(argv[1]);
        CHECK(phys != NULL);
        if (phys) {
            kspcore_confignode* tip = kspcore_confignode_get_node(phys, "DRAG_TIP", 0);
            CHECK(tip != NULL);
            kspcore_floatcurve* t = kspcore_floatcurve_load(tip);
            CHECK(t != NULL);
            CHECK(bits_of(kspcore_floatcurve_evaluate(t, 1.1875f)) == 0x403923FAu); /* 2.8928208 */
            CHECK(bits_of(kspcore_floatcurve_evaluate(t, 1.0999999f)) == 0x40351EB6u);
            kspcore_floatcurve_destroy(t);
            kspcore_confignode_release(tip);
            kspcore_confignode_release(phys);
        }
    }
    /* errors do not escape */
    CHECK(kspcore_confignode_load("Z:/does/not/exist.cfg") == NULL);
    CHECK(strlen(kspcore_last_error()) > 0);

    kspcore_confignode_release(part);
    kspcore_confignode_release(root);
    kspcore_sim_destroy(sim);
    printf(failures ? "capi_c: %d FAILURES\n" : "capi_c: OK\n", failures);
    return failures ? 1 : 0;
}
