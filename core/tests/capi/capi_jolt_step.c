/* ctest twin of the Unreal automation test KspUe5.Bridge.JoltStep (WP-UE-01). A pure C consumer of the test ABI
 * (include/kspcore/kspcore_test.h): steps the physics scenario through kspcore_test_physics_scenario and checks the hash is
 * non-zero, identical run to run and at 1 / 2 / 4 threads, that the process MXCSR is the guard's 0x1F80 and that Jolt is the
 * double-precision build. It then writes the hash to argv[1] ("key=value" lines) for the Unreal test to compare with.
 * Usage: capi_jolt_step <out_file> [steps]   (steps default 200) */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kspcore/kspcore_c.h"
#include "kspcore/kspcore_test.h"

static int failures = 0;
#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            ++failures;                                                \
        }                                                              \
    } while (0)

int main(int argc, char** argv) {
    const char* outFile = argc > 1 ? argv[1] : NULL;
    const int steps = argc > 2 ? atoi(argv[2]) : 200;
    uint64_t h1 = 0, h1b = 0, h2 = 0, h4 = 0;

    CHECK(kspcore_test_abi_version() == KSPCORE_TEST_ABI_VERSION);
    /* control bits only (0xFFC0: DAZ, exception masks, rounding, FTZ); the low bits are sticky exception flags */
    printf("mxcsr=0x%04X\n", kspcore_test_mxcsr());
    CHECK((kspcore_test_mxcsr() & 0xFFC0u) == 0x1F80u);
    CHECK(strstr(kspcore_test_jolt_config(), "double_precision=1") != NULL);
    CHECK(kspcore_test_physics_scenario(KSPCORE_TEST_SCENARIO_PILE, 1, steps, &h1) == 1);
    CHECK(kspcore_test_physics_scenario(KSPCORE_TEST_SCENARIO_PILE, 1, steps, &h1b) == 1);
    CHECK(kspcore_test_physics_scenario(KSPCORE_TEST_SCENARIO_PILE, 2, steps, &h2) == 1);
    CHECK(kspcore_test_physics_scenario(KSPCORE_TEST_SCENARIO_PILE, 4, steps, &h4) == 1);
    CHECK(h1 != 0);
    CHECK(h1 == h1b);
    CHECK(h1 == h2);
    CHECK(h1 == h4);
    CHECK(kspcore_test_physics_scenario(99, 1, steps, &h1b) == 0);   /* unknown scenario fails cleanly */
    CHECK(kspcore_test_physics_scenario(KSPCORE_TEST_SCENARIO_PILE, 1, steps, NULL) == 0);

    printf("pile steps=%d hash=0x%016" PRIx64 "\n", steps, h1);
    if (outFile) {
        FILE* f = fopen(outFile, "wb");
        CHECK(f != NULL);
        if (f) {
            fprintf(f, "build_info=%s\n", kspcore_build_info());
            fprintf(f, "pile_steps=%d\n", steps);
            fprintf(f, "pile_hash=0x%016" PRIx64 "\n", h1);
            fclose(f);
        }
    }
    return failures ? 1 : 0;
}
