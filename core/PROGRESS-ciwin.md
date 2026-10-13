# PROGRESS-ciwin: Windows CI build speed (branch ci-windows)

Local measurements (Windows, 6 build jobs, MSVC 19.44, RelWithDebInfo, build dir wiped before each; "build" = cmake --build only):

| configuration | build |
|---|---|
| baseline origin/main (c5a6fe33) | 207.6 s |
| PCH for kspcore_tests only | 184.7 s |
| PCH for kspcore + kspcore_tests (local default now) | 156.7 s (172.0 s in a second run, noise) |
| CI mode, ccache cold (-Ci; PCH off, no dev tools) | 242.7 s (+17 % on the very first run) |
| CI mode, ccache warm, build dir wiped | 16.4 s |
| local default (PCH), touch src/parts/part.cpp | 6.4 s |
| local default (PCH), touch tests/test_orbit.cpp | 2.3 s |

All 45 ctest tests pass (unit.shard.*, unit.random_order.*, golden, libm, capi) in both the PCH configuration and the -Ci/ccache configuration.

Notes
- /Z7 was already global (CMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded), so no change was needed for ccache; ccache 4.14.1 stats: 1096 cacheable of 1102 calls.
- PCH and ccache are mutually exclusive on MSVC (ccache: "Could not use precompiled header"); Jolt's own PCH (130 files) was the biggest uncacheable block until disabled for launcher builds.
- Skipped in CI: wp12_height_oracle, wp12_bench only. golden_runner, save_corpus, libm_check, capi_c, capi_ue_like are used by tests. No add_test or fixture references the two skipped targets.
- Risks: first CI run is cold (slower than before by ~15 %); cache key `windows-msvc-relwithdebinfo` plus the action's own restore prefix; a runner-image change (cl.exe mtime) invalidates the cache; a change to a widely included header rebuilds most TUs; link steps and ctest (266 s) are not sped up.
