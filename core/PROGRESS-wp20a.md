# PROGRESS WP-20a (physics world, frames, Jolt vendored)

Branch wp20a (from jolt-spike b3b70db5), origin/main merged (24 commits, no conflicts).

- [x] Jolt v5.6.0 vendored: third_party/jolt (Jolt/, Build/CMakeLists.txt, LICENSE, README.md, SHA256SUMS), cmake/ksp_jolt.cmake
- [x] PhysicsWorld (pimpl; glue TU physics_world.cpp built RTTI-free as OBJECT lib; physics_world_loop.cpp)
- [x] Krakensbane / FloatingOrigin (no shift) / ReferenceFrame ports; check_cites exact 15 / 0 mismatches
- [x] tests/test_physics_world.cpp: 14 cases / 147 assertions, thresholds from measurements (STATUS.md)
- [x] Windows full ctest 34/34, unit 718 / 1,094,123
- [x] Linux GCC 13.3 (WSL, own build tree ~/wp20a-gcc): builds, WP-20a tests pass. First attempt failed to link (typeinfo for JPH::GroupFilter) -> glue built with -fno-rtti
- [x] docs: third_party/jolt/README.md, VERSIONS.md, README.md, STATUS.md (section + row 39)
- [x] final: core/build deleted + run_tests.ps1 -Fresh: configure 2.1 s, build 169 s, ctest 34/34 pass (187 s)
