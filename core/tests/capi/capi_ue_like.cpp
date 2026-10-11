// C++ consumer compiled with Unreal's flags (/GR- /EHs-c- _HAS_EXCEPTIONS=0 /fp:fast), seeing only the C header.
// The numerics run inside kspcore.lib, so results are identical to the /fp:precise build. argv[1] = Physics.cfg.
#include <cstdio>
#include <cstring>

#include "kspcore/kspcore_c.h"

namespace {
int failures = 0;
void Check(bool ok, const char* what, int line) {
    if (!ok) {
        std::printf("FAILED line %d: %s\n", line, what);
        ++failures;
    }
}
unsigned Bits(float f) {
    unsigned u;
    std::memcpy(&u, &f, sizeof u);
    return u;
}
}  // namespace
#define CHECK(c) Check((c), #c, __LINE__)

int main(int argc, char** argv) {
    const char* info = kspcore_build_info();
    std::printf("%s\n", info);
#ifdef _MSC_VER
    CHECK(std::strstr(info, "crt=MD ") != nullptr);
#endif
    CHECK(std::strstr(info, "idl=0") != nullptr);
    kspcore_sim* sim = kspcore_sim_create();
    CHECK(sim != nullptr);

    kspcore_floatcurve* c = kspcore_floatcurve_create();
    kspcore_floatcurve_add_tangents(c, 0.0f, 1.0f, 0.0f, 0.0f);
    kspcore_floatcurve_add_tangents(c, 0.85f, 1.19f, 0.6960422f, 0.6960422f);
    kspcore_floatcurve_add_tangents(c, 1.1f, 2.83f, 0.730473f, 0.730473f);
    kspcore_floatcurve_add_tangents(c, 5.0f, 4.0f, 0.0f, 0.0f);
    CHECK(kspcore_floatcurve_key_count(c) == 4);
    // same keys as Physics.cfg DRAG_TIP: golden values (harness/golden/curves_eval PhysicsGlobals.dragCurveTip)
    CHECK(Bits(kspcore_floatcurve_evaluate(c, 1.1875f)) == 0x403923FAu);
    CHECK(Bits(kspcore_floatcurve_evaluate(c, 1.0999999f)) == 0x40351EB6u);
    kspcore_floatcurve_destroy(c);

    if (argc > 1) {
        kspcore_confignode* phys = kspcore_confignode_load(argv[1]);
        CHECK(phys != nullptr);
        double d = 0.0;
        CHECK(kspcore_confignode_try_get_double(phys, "dragMultiplier", &d) == 1 || d == 0.0);
        kspcore_confignode_release(phys);
    }
    // WP-12a terrain quads through the C ABI (argv[2] = core/data/terrain): the Mun, inline heights
    if (argc > 2) {
        kspcore_pqs* pqs = kspcore_pqs_create_stock("Mun", argv[2], 0);
        CHECK(pqs != nullptr);
        const double target[3] = {204000.0, 10000.0, 0.0};
        kspcore_pqs_set_target(pqs, target);
        const int n = kspcore_pqs_update(pqs, 1);
        CHECK(n > 50);
        kspcore_pqs_quad q{};
        CHECK(kspcore_pqs_get_quad(pqs, 0, &q) == 1);
        CHECK(q.vertex_count == 225);
        kspcore_pqs_destroy(pqs);
    }
    // a malformed key reports an error instead of throwing across the boundary
    kspcore_confignode* bad = kspcore_confignode_parse("key = 1\n");
    CHECK(kspcore_floatcurve_load(bad) == nullptr);
    CHECK(std::strlen(kspcore_last_error()) > 0);
    kspcore_confignode_release(bad);
    kspcore_sim_destroy(sim);
    std::printf(failures ? "capi_ue_like: %d FAILURES\n" : "capi_ue_like: OK\n", failures);
    return failures ? 1 : 0;
}
