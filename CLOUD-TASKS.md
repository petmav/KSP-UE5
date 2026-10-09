# CLOUD-TASKS: working on KSP-UE5 from a Claude Code cloud session

Private repo. Nothing here is distributed. Written by WP-00L (2026-10-09). Implementer model for package work: Sonnet
(Opus only as escalation after two failed gate attempts); Fable orchestrates and reviews; Haiku does research.

## (a) One command, build + test

Linux (cloud container, WSL, any Linux with cmake >= 3.28, ninja, Clang 16+ or GCC 13+, python3):

```bash
cd core
./run_tests.sh -f                      # linux-rel (Clang if installed), fresh configure, build, ctest
./run_tests.sh -p linux-gcc-rel -f     # same with GCC
./run_tests.sh -t golden               # only tests whose name matches the regex
./run_tests.sh -b                      # configure + build only
```

Windows (local, MSVC from VS 2022):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File core\run_tests.ps1 -Fresh
#   -Preset debug   -TestFilter golden   -BuildOnly
```

Both print `ALL TESTS PASSED` on success; every gate is bit-exact (max ULP 0). Currently 21 ctest tests (`ctest --preset
linux-rel -N` lists them). Build trees are under `core/build/<preset>/` (ignored by git). A package worktree
(`core/.worktrees/wp<NN>`) builds the same way; the top-level CMake walks up until it finds `harness/compare/compare.py`.

What a fresh container needs: nothing from the game. The tests read `core/testdata/GameData-mirror/` (cfg files byte-identical, empty
placeholders for textures/models/audio), `core/testdata/logs/`, `harness/golden`, `harness/captures`, `harness/inputs`, `core/data`.
Details: `core/STATUS.md` "WP-00L". Set env `KSP_GAMEDATA_ROOT=<game root>` to run against a real install (local only).

## (b) What the cloud can and cannot do

| can | cannot (local only) |
|---|---|
| build kspcore (Clang, GCC) and run every existing gate | launch KSP (never launch the game; the Steam install is never touched) |
| implement and gate any package whose gate is a golden file, a committed capture, or a unit test | in-game captures (`harness/captures/run_capture.sh`, needs the profiling build of the game in `decompiled/KSP-Decompiled-profiling/`) |
| read the decompiled source (`decompiled/KSP-Decompiled/`) as the spec, once it is in the repo (see "Open decisions") | regenerate harness goldens (`harness/run.ps1`: .NET Framework 4.8 plus the game DLLs in `harness/lib`, Windows only) |
| write Python/C++ tools, mirror data, docs | Unreal builds and the `ue5/` tests (UE 5.8 on Windows) |
| run `tools/libm/*` research scripts only with the DLLs (Windows) | regenerate the libm lift (needs `msvcr120_clr0400.dll`, `ucrtbase.dll`, capstone) |
| | the Jolt physics spike (WP-20 prerequisite: profiling on the dev laptop; see (c)) |
| | rebuild the GameData mirror or extract Unity assets (`tools/mk_gamedata_mirror.py`, `tools/wp11/extract_terrain.py` need the game install) |

Local-only task list (each needs the Windows machine with the game): new captures (terrain heights of the other 13 bodies, WP-03
float32 helpers under Mono, in-flight delta-v, ablation thermal case), harness golden regeneration or new harness datasets,
profiling-branch builds, Unreal work, anything that launches KSP.

## (c) Ordered package queue

Source of truth: `analysis/tiering/work_packages.csv` (file to package, per file) and `analysis/tiering/TIERING.md` (dependencies,
verification kind). Spec for every package = the decompiled files listed in `work_packages.csv` under `decompiled/KSP-Decompiled/Assembly-CSharp/`.
Done and merged: WP-01, 02, 03, 05, 07, 08, 09, 11 (WP-04 is on branch `wp04`, merged separately). "Unit-gated" = no golden or capture
exists: write doctest cases from the decompiled source and the cfg data, and say so in STATUS.md; ask for a new golden/capture (local) in the
"needs" column when numerics matter.

| order | package | key spec files | gate | needs / depends on |
|---|---|---|---|---|
| 1 | WP-06 Module host: PartModule, KSPField/Event/Action registry | `PartModule.cs`, `BaseField.cs`, `BaseAction.cs`, `PartModuleList.cs` | `capture.part_database`: widen the scope (module_fields currently only the fields a MODULE node sets; `module_mass`/`module_cost` columns are dropped) | WP-02, 03, 05 |
| 2 | WP-10 Time warp, rails propagation, orbit driver | `TimeWarp.cs`, `OrbitDriver.cs`, `WarpTransitionCalculator.cs`, `PositionTarget.cs` | `capture.orbit_rails` (C8): un-drop `world_rel_*` and `packed/loaded` once WP-14/19 exist; until then unit | WP-00, 07, 09 |
| 3 | WP-13 Part model and part tree | `Part.cs`, `PartVariant.cs`, `ModulePartVariants.cs`, `PartGeometryUtil.cs` | `capture.part_database` dropped columns (partSize, module_mass/cost, variants.materials); unit | WP-00, 05, 06 |
| 4 | WP-14 Vessel model, situations, mass properties | `Vessel.cs`, `FlightLogger.cs`, `FlightLog.cs`, `ModuleTripLogger.cs` | `harness/captures/out2/aero_forces_vessel.json` (assembled-vessel mass/drag inputs); unit | WP-13, 09 |
| 5 | WP-15 Resource flow | `PartSet.cs`, `ResourceMap.cs`, `ModuleResourceHandler.cs`, `PartResourceList.cs` | unit-gated | WP-13; a harness resource-flow dataset would be local work |
| 6 | WP-16 Staging, action groups, decouplers | `StageManager.cs`, `ModuleDecouplerBase.cs`, `ModuleDecouple.cs`, `ActionGroupList.cs` | unit-gated | WP-13, 15 |
| 7 | WP-17 Delta-v calculation | `VesselDeltaV.cs`, `DeltaVStageInfo.cs`, `DeltaVEngineInfo.cs`, `DeltaVEngineStageSet.cs` | `harness/captures/out2/deltav_readout.json` | WP-15, 16, 23 (engines: stub the engine data from cfg until WP-23) |
| 8 | WP-18 Craft files and ship construction | `ShipConstruction.cs`, `ShipConstruct.cs`, `ShipTemplate.cs` | craft round trip already in `corpus.gamedata` (86 crafts); load/construct is unit-gated on `harness/inputs/Ships` | WP-05, 13 |
| 9 | WP-12, 19, 20, 21, 22 (physics-dependent) | WP-12 `PQS.cs`, `PQ.cs`, `PQSLandControl.cs`; WP-19 `FlightGlobals.cs`, `ProtoVessel.cs`; WP-20 `PartJoint.cs`, `FloatingOrigin.cs`; WP-21 `FlightIntegrator.cs`, `DragCubeList.cs`; WP-22 `ModuleCoreHeat.cs`, `TemperatureModel.cs` | WP-21: `harness/golden/aero_forces`, `captures/out/aero_forces_mono.json`, `out2/aero_forces_vessel.json`; WP-22: `out2/thermal_step.json`, `out2/atmosphere_thermal_offsets.json`; WP-12/19/20: scenario | **WP-20 needs the Jolt spike first** (local: `analysis/physics/PHYSICS-ENGINE-REVIEW.md` levers 1 and 2; plain C++ under `core/` or a sibling). WP-19 needs WP-04 merged. WP-21/22 can start without Jolt on the numeric parts that do not need a rigid-body world |
| 10 | WP-23 Engines, RCS, gimbals, intakes | `ModuleEngines.cs`, `ModuleRCS.cs`, `ModuleGimbal.cs`, `MultiModeEngine.cs` | `harness/golden/engine_thrust` | WP-06, 09, 15, 20 |
| 11 | WP-24 Command, SAS, reaction wheels | `VesselAutopilot.cs`, `ModuleCommand.cs`, `ModuleReactionWheel.cs`, `FlightCtrlState.cs` | scenario / unit | WP-06, 14, 20 |
| 12 | WP-25 Docking, grapple, struts | `ModuleDockingNode.cs`, `ModuleGrappleNode.cs`, `CModuleStrut.cs`, `DockedVesselInfo.cs` | scenario | WP-13, 16, 20 |
| 13 | WP-26 Fairings, cargo bays | `ModuleProceduralFairing.cs`, `ModuleCargoBay.cs`, `FairingPanel.cs`, `CompoundPart.cs` | unit | WP-13, 21 |
| 14 | WP-27 Parachutes, deployables | `ModuleDeployablePart.cs`, `ModuleParachute.cs`, `ModuleAnimateGeneric.cs` | scenario | WP-00, 06, 21 |
| 15 | WP-28 Resource-processing modules | `BaseConverter.cs`, `ModuleResourceHarvester.cs`, `ModuleOrbitalScanner.cs` | unit | WP-06, 09, 15 |
| 16 | WP-29 Wheels and landing gear | `ModuleWheelBase.cs`, `VehicleBase.cs` (VehiclePhysics subset) | scenario | WP-06, 12, 20 |
| 17 | WP-30 Misc part modules; WP-31 Kerbal roster | `ModulePartFirework.cs`, `FlagSite.cs`; `KerbalRoster.cs`, `ProtoCrewMember.cs` | inspection; unit | WP-06; WP-03, 04 |
| later | WP-32 .. WP-59 | see `analysis/tiering/TIERING.md` (science, contracts, CommNet, editor core, UI, rendering: UI and rendering are C-tier, Unreal side) | per package | per TIERING.md |

## (d) Per-package brief template

Paste this into the cloud task, fill the angle brackets.

```
You are a Sonnet implementer on KSP-UE5 (private, local-only project; engine-agnostic C++ core kspcore + Unreal shell).
Read, in order: CLAUDE.md, NEXT-STEPS.md, CLOUD-TASKS.md, core/README.md, core/STATUS.md (WP-01 libm notes, WP-03, WP-05 GameData root,
WP-00L), analysis/tiering/work_packages.csv rows for <WP-NN>.

Task: implement <WP-NN: name> in core/ so that <gate> passes bit-exactly (max ULP 0) and every existing gate still passes.
Spec: decompiled/KSP-Decompiled/Assembly-CSharp/<files>. The decompile is authoritative; cite File.cs:line next to ported code.
Original game assets: use the game's own data (the GameData mirror in core/testdata, extracted data in core/data); never invent substitutes.

Rules (non-negotiable)
- Do not spawn subagents beyond 4 concurrent, and only model "haiku" for research; no implementer children.
- Context hygiene: write progress to PROGRESS-<wpNN>.md after every major step and work from it; compact near ~600k tokens; use
  tight grep/sed ranges, never dump large files.
- Branch per package: worktree core/.worktrees/wp<NN> on branch wp<NN> from main. Commit only there. Never push. Do not touch main
  or other worktrees.
- The gate must reach 0 ULP, or the failure is documented in STATUS.md with the mismatching inputs. Never loosen harness
  tolerances, never filter away rows to hide a mismatch (filters only for data outside the package's scope, documented next to the
  gate and in STATUS.md, README "Golden gate options").
- Float32 code paths follow the Mono rule (ksp/dotnet/mono_float.hpp: evaluate in double, round at stores); captures, not .NET goldens, are
  truth for float32. Use dotnet::Math for System.Math and dotnet::NativeSinf/Cosf/Powf for Unity-native float maths, never std:: directly.
- Paths: use dotnet::PathFromUtf8 for every filesystem path; output files stay CRLF where KSP writes CRLF.
- Add a STATUS.md section for the package (state, gate results per table/column, deviations, open items) and the README rows
  (tests table, layout). Run ./run_tests.sh -f on Linux (and run_tests.ps1 -Fresh on Windows if available) before the final commit.
Final message: gate results table, commits, anything not achieved.
```

## (e) Merge protocol

1. On the package branch: `./run_tests.sh -f` (and `run_tests.ps1 -Fresh` locally): all green.
2. In the main checkout of `core/` (not the worktree): `git merge --no-ff wp<NN>`. Resolve `CMakeLists.txt`/`tests/CMakeLists.txt`/
   `src/CMakeLists.txt` as a union, `STATUS.md` and `README.md` as a union (every section kept; header test counts and gate
   tables merged).
3. Rerun the full suite fresh on the merged tree (`./run_tests.sh -f`; locally also `run_tests.ps1 -Fresh`). If it fails, fix on
   `main` (or revert the merge) before the next package.
4. Record the merge hash in the package's STATUS.md section and in NEXT-STEPS.md. Merge one package at a time, in dependency order.
   `decompiled/KSP-Decompiled` stays on a clean `main`.

## (f) Known follow-ups

- `harness/compare/compare.py` passes silently when the candidate has rows the golden lacks (641 such rows in `part_database`, all
  explained). Add an extra-rows check with an explicit allow-list before more capture gates land.
- WP-03 float32 helpers (Heading/Bearing, WrapAround, Color.Lerp) need a Mono capture; today only the .NET golden covers them (local capture).
- Terrain goldens exist for Kerbin and Mun only (`captures/out2/terrain_height.json`); the other 13 bodies need a capture (local).
- Jolt physics spike (levers 1 and 2 from `analysis/physics/PROFILE.md`; then the reduced rigid-body model) is a local task and the
  prerequisite of WP-20; reconcile the review's ranking with WP-19 to WP-22 first.
- The libm lift tracks `msvcr120_clr0400.dll` with the FMA3 code path. If the capture machine class changes, rerun `libm_check` on
  Windows and the libm capture gates.
- Windows CI uses VS 2022 on `windows-latest` (Enterprise edition): `run_tests.ps1` falls back to any VS 2022 with the C++ tools.
  The workflow has never been run (WP-00L had no GitHub access).

## Repository set-up notes (before the first push)

- The project root is not a git repo yet. `core/` has its own history (main checkout plus `core/.worktrees/*`); decide whether to
  `git init` the root with `core/` as a plain directory (import the history with `git subtree`/`filter-repo`, or start fresh) before
  pushing. `.gitignore` already excludes `**/.worktrees/` and `**/build/`.
- `decompiled/KSP-Decompiled/` is a **separate git repo** (branch `main`, the spec every agent reads). A cloud session needs its
  contents: vendor it (delete its `.git`, commit the tree) or add it as a submodule. It is not in `.gitignore`.
  `decompiled/_reference` is not in `.gitignore` and was not reviewed by WP-00L (check its size before the first push).
- **LFS.** `.gitattributes` contains the Git LFS lines for `core/data/terrain/maps/*.bin` (55 MB) and the two 45 MB libm capture
  files, **commented out**. No file is above GitHub's 100 MB limit, so LFS is optional; the total is about 150 MB of large files. To enable:
  `git lfs install`, uncomment the three lines, `git add --renormalize .`, add `with: { lfs: true }` to the checkout steps in
  `.github/workflows/ci.yml`, and make sure the cloud environment can fetch LFS objects (an un-fetched LFS file is a text pointer and every
  gate that reads it fails). Otherwise leave the lines commented or delete them.
- Line endings: `.gitattributes` keeps `*.cfg`, `*.craft`, `*.sfs`, `*.json`, `*.tsv`, `core/testdata/**` byte-exact (`-text`); ConfigNode output is
  CRLF + UTF-8 without BOM on every platform because that is KSP's format.
- Never commit `decompiled/KSP-dev/`, `harness/lib/`, `harness/tools-cache/`, build trees, `*.pdb` (all in `.gitignore`).
