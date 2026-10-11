# Jolt Physics (vendored)

| | |
|---|---|
| project | Jolt Physics, https://github.com/jrouwe/JoltPhysics |
| version | v5.6.0 (2026-07-11), commit `e77f175595e64cb44218cc9d9d56fc365ad0e36a` |
| origin | GitHub release archive https://github.com/jrouwe/JoltPhysics/archive/refs/tags/v5.6.0.tar.gz |
| archive sha256 | `6e069ee0172478cc78182047aac87e5310ba14a67a53348ae14cc37801fd3f8e` (the value the physics spike pinned in its FetchContent; re-verified when vendoring, 2026-10-11) |
| licence | MIT, `LICENSE` (copyright Jorrit Rouwe) |
| vendored | `Jolt/` (the library: 153 .cpp, 367 .h, .inl, .hlsl, Jolt.cmake, Jolt.natvis; 5.6 MB), `Build/CMakeLists.txt` (the only file of `Build/` that `add_subdirectory` needs), `LICENSE`, `UPSTREAM-README.md` (upstream README.md). Not vendored: Docs, Samples, UnitTests, TestFramework, PerformanceTest, HelloWorld, JoltViewer, Assets |
| modifications | none. Files are byte-identical to the archive (`SHA256SUMS` lists every vendored file, verify with `cd core/third_party/jolt && sha256sum -c SHA256SUMS`) |

Why vendored: the physics spike fetched the archive at configure time. The production kspcore build (CI, cloud VMs, offline
machines) must not depend on the network, so the source lives here, as doctest and nlohmann/json do (`../VERSIONS.md`).

## How it is built

`core/cmake/ksp_jolt.cmake` (included by the root `CMakeLists.txt`) sets Jolt's options and `add_subdirectory`s
`third_party/jolt/Build`, which defines the static library target `Jolt` (not part of `all`; built when kspcore links it).
Configuration = the physics spike's (`core/spikes/jolt`, `core/PHYSICS-SPIKE.md`):

- `DOUBLE_PRECISION=ON` (`-DKSP_JOLT_DOUBLE=OFF` for single precision, spike comparisons only)
- `CROSS_PLATFORM_DETERMINISTIC=OFF` (`-DKSP_JOLT_DETERMINISTIC=ON` turns it on): deterministic for one binary and call order,
  independent of the thread count (measured in `tests/test_physics_world.cpp` at 1, 2, 4, 8 threads); not bit-identical across compilers / OSes
- `/MD` runtime, no IPO / LTO, no asserts, no debug renderer, no profiler, no object stream, no GPU compute back ends, FP exceptions off
- `ENABLE_ALL_WARNINGS=OFF`: no `/WX` or `-Werror` on third-party code; the glue that includes Jolt headers suppresses the header warnings
- ISA: Jolt's defaults (AVX2, BMI, LZCNT, F16C, FMA on x86-64). These flags are Jolt's PUBLIC compile options; kspcore applies them
  only to `src/physics/physics_world.cpp` (the `kspcore_physics_glue` object library) and links `Jolt` with `$<LINK_ONLY:Jolt>`,
  so the float32 code of the rest of kspcore keeps its SSE2-scalar, uncontracted flags. The machine must support AVX2.

To update Jolt: replace `Jolt/`, `Build/CMakeLists.txt`, `LICENSE` from the new release archive, regenerate `SHA256SUMS`, update this
file and `../VERSIONS.md`, re-run the physics tests.
