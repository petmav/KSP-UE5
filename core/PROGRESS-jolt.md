# PROGRESS: Jolt physics spike (branch `jolt-spike`, worktree `core/.worktrees/jolt`, from main `af3798c6`)

Agent: optimizer (Opus). Brief: de-risk the physics decision for WP-19 / WP-20 / WP-12. Deliverables: plain-C++ spike under
`core/spikes/jolt/` (own CMake target, Jolt via FetchContent at a pinned tag), measurements on this machine (Ryzen 9 7950X3D),
recommendation in `core/PHYSICS-SPIKE.md`.

## Decisions so far

- Jolt Physics **v5.6.0** (2026-07-11, commit `e77f175595e64cb44218cc9d9d56fc365ad0e36a`), fetched as the GitHub release
  archive with `URL_HASH SHA256=6e069ee0172478cc78182047aac87e5310ba14a67a53348ae14cc37801fd3f8e` (network works).
- Spike is opt-in: `KSPCORE_BUILD_SPIKES` (default OFF) in `core/CMakeLists.txt`; separate presets build it in its own
  build tree, so `run_tests.ps1` / CI / existing presets are untouched.
- Jolt options: no static CRT (kspcore is /MD), no IPO, no /WX, no debug symbols; DOUBLE_PRECISION is a per-preset switch
  (float and double builds are measured side by side).

## Steps

1. [ ] Baseline: `run_tests.ps1` in the worktree before any change (CMAKE_BUILD_PARALLEL_LEVEL=6).
2. [ ] CMake skeleton + FetchContent, hello-world Jolt world builds and runs.
3. [ ] Craft extraction through kspcore (GameData mirror, ShipConstruct.LoadShip, AssembleVesselFromCraft): parts, masses,
       CoM, attach nodes, PartJoint rules (node choice, 1/3 sub-joints, stiffness, break force), autostruts.
4. [ ] Model A: PhysX-equivalent articulated model in Jolt (one body per FULL part, SixDOF joints as PartJoint builds them).
5. [ ] Model B: reduced rigid-body model (one body per rigid cluster, structural load graph, break check, split).
6. [ ] Lever 1: double-precision body-centred frame, no per-tick shift; measure the per-tick shift it replaces.
7. [ ] Lever 2: per-part pipeline natively (exact kspcore ports, AoS) and SoA; per-part µs.
8. [ ] Measurements: tick vs part count (A vs B), solver iterations, threads, warp 1x..>4x (large dt and substeps).
9. [ ] PHYSICS-SPIKE.md recommendation, packages, gates, risks.
10. [ ] Full test suite green, commit, push `jolt-spike`.

## Log

- 2026-10-11: step 1 done (baseline 34/34 green in the worktree, build 129 s, test 130 s). Steps 2-3 done: spike builds
  (VS 2022 Community MSVC 14.44, same as run_tests.ps1; build_spike.ps1); `jolt_spike info` reproduces the profile's runtime
  sub-joint counts exactly (S 24, S-AS 44, M2 168, M2-AS 320, L2-AS 960) and masses (L2-AS 677.3 t). Committed 6d7279b6,
  pushed `jolt-spike`. Resumed after an API rate-limit cut; main has moved (wp50, wp53): merge before finishing.
- First flight (S, articulated) runs; radial decouplers break on the pad at 1.26 s and the remaining joints show 0.3-0.5 m
  anchor separation: investigating (joint error measure, break threshold, mass ratios).

- 2026-10-11: read optimizer.md, CLAUDE.md, PHYSICS-ENGINE-REVIEW.md, PROFILE.md, TIERING.md (WP-12/18/19/20/21/22),
  core/README.md, core/STATUS.md (WP-11/14/18/21 sections), NEXT-STEPS.md. Network reachable; Jolt v5.6.0 pinned.
