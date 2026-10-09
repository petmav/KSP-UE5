# KSP-UE5: state and next steps

Updated 2026-10-09 (session 2 in progress).

## Done

- `FEASIBILITY.md`: assessment. Conclusion: engine-agnostic C++ core + thin UE5 shell; physics model is the key decision.
- `decompiled/`: full decompilation of KSP 1.12.5 (180 managed assemblies, 3.26M lines). Main assembly recompiles (0 errors) and loads in the dev copy `decompiled/KSP-dev/`. Entry points: `decompiled/KSP-Decompiled/INDEX.md`, `decompiled/STATUS.md`. Game root is pinned to KSP-dev; the Steam install is never touched.
- `analysis/tiering/`: every KSP-authored file tiered (A transpile 1,109 files / B re-architect 238 / C rewrite-or-drop 1,520; 461 third-party). 60 work packages with dependency order in `TIERING.md` and `work_packages.csv`. First ten packages need no physics decision.
- Toolchain: Visual Studio 2022 Community with the Game Development with C++ workload (MSVC 14.44). Epic Games Launcher installed at `C:\Program Files\Epic Games\Launcher\`.

## Decisions made by the user

- Private, local-only project. Full scope: the entire game, both DLC code paths.
- Physics: faithful recreation of KSP's physics with significant performance improvements where possible. Kraken-style failure modes need not be preserved.
- No exhaustive feature testing; verify numerically critical systems via the golden harness, the rest by scenario or inspection.
- Install Unreal on this laptop now (24 GB RAM, integrated Radeon 860M); heavy shader/perf work deferred to a stronger machine.
- Agents: Fable orchestrates, Haiku researches, Opus 5.5 implements. Max 3 to 4 concurrent subagents; implementer children must be Haiku or Sonnet. Tell long agents to persist state to PROGRESS.md and compact around 600k tokens.

## Also done (landed at end of session 1)

- `analysis/physics/`: `PHYSICS-ENGINE-REVIEW.md` (1,067 lines; measured call order, 19 code quirks, ranked improvements) and `PROFILE.md` (real frame-time breakdown on this laptop). Headline: the joint solver is NOT the bottleneck. On a 457-part ascent tick: 35% floating-origin world shift, 21% FlightIntegrator, 12% PhysX joint solve, 11% per-part components, 6% engines. Top levers in order: (1) no per-tick world shift (exact, ~2x), (2) data-oriented per-part pipeline (exact, ~10x on that code), (3) reduced rigid-body model (approximate, ~10x on joint/PhysX work, unlocks physics warp > 4x). Live values: PhysX solver iterations 50, attach-joint stiffness x10, rotating-frame threshold 100 km, joint projection off. Instrumentation is on branch `profiling` (head 10006ec2, build with Optimize=true); test craft in `analysis/physics/craft/` and `decompiled/KSP-dev/Ships/VAB/PhysProf-*.craft`; a `physprofile` save exists in KSP-dev.
- `harness/`: golden-output harness running KSP's own IL on .NET 4.8 outside Unity. `run.ps1` regenerates everything in ~3 s. 10 datasets, 71,566 rows, dataset names match `work_packages.csv`. Rebuilt-from-decompile DLL agrees with the shipped DLL bit for bit on 518,141 values. `compare/compare.py` with `tolerances.json`. `CAPTURE-SPEC.md` lists 10 items that need the running game (native AnimationCurve check, Unity Random, native quaternion maths, Mono float semantics, on-rails propagation, assembled-vessel drag, etc.). FloatCurve's native evaluation was ported from UnityPlayer.dll disassembly and is the one "rehosted-native" item pending in-game confirmation.
- Must-know quirks for C++ implementers are listed in the harness final report and README: FloatCurve is a cached float32 cubic, not textbook Hermite; 2-value cfg keys get Unity tangent smoothing; Mono writes "Infinity"; body constants are float-derived doubles (Kerbin mu = 3531599999999.9995); Kepler solver switches method at e = 0.8; ConfigNode treats `//` as a comment anywhere and saves CRLF UTF-8 without BOM.

## Pending

- Unreal Engine 5.8.3 installed at `C:\Program Files\Epic Games\UE_5.8\` (2026-10-09).
- Session 2 (2026-10-09) DONE:
  - `core/`: kspcore scaffold (CMake, C++20, doctest, golden gate via compare.py). WP-01 (maths, Unity value types, FloatCurve, UnityRandom/KSPRandom/SystemRandom, Mono float rule `fx()`/`f32store()`, x87 FSIN/FCOS and VS2013 CRT libm emulation, .NET double.Parse port) and WP-02 (ConfigNode, byte-identical on 47/47 files, 368 cfg + 86 craft corpus) are bit-exact (0 ULP) on every gate. 10/10 tests. One command: `powershell -File core
un_tests.ps1`. C ABI at `core/include/kspcore/kspcore_c.h`, lib at `core/build/RelWithDebInfo/kspcore.lib`.
  - `harness/captures/`: C1 FloatCurve bit-exact vs native Unity; C2 Unity Random pinned bit-exact; C7 found Mono evaluates float in double and rounds at stores (harness .NET float32 goldens are not game truth; captures are). Rerun: `bash harness/captures/run_capture.sh all` with the profiling build.
  - `ue5/`: UE 5.8 C++ project KspUe5 + KspCoreBridge plugin, no Blueprints, 3 headless tests. Scale spike: jitter-free at Kerbin scale with LWC and per-patch mesh origins, no rebasing needed; per-tick origin shift costs ~5-6 ms (Chaos teleports bodies one by one) so do not do it. Agent loop: touch + build + headless tests = 27 s. Real kspcore linked (bUseRealCore=true, committed); bridge test reports runtime=Mono, libm=VS2013 CRT.

- In-game captures per `harness/CAPTURE-SPEC.md` (C1 to C10). The profiling addon on the `profiling` branch is the natural host for a capture mode; not implemented yet.
- Reconcile the physics review's ranking with tiering packages WP-19 to WP-22 before starting physics work.

## Session 2, later (2026-10-09)

- Merged into `core/main` (f964447, 15/15 tests, 41 gate tables at 0 ULP): WP-03 event bus/FSM/KSPUtil/Localizer (469 GameEvents generated from source; events fire in REVERSE registration order), WP-07 Orbit + Targeting (bit-exact incl. native AngleAxis), WP-09 bodies/atmospheres/PhysicsGlobals (17-body catalog from prefabs; asset index of real biome/height/colour maps), and the Orbit/CelestialBody integration (one Planetarium; `CelestialSystem::Spawn` attaches real orbits; harness frame reproduced via `CelestialEnv::UseHarnessFrame()` because Mun/Minmus ship with inverseRotation=1 and the harness skipped the 230-degree frame turn).
- Captures batch 2 done (`harness/captures/out2/`): part database (362 parts, asset paths per part), terrain heights (140k samples Kerbin+Mun), delta-v readout, thermal steps, on-rails orbits (4 SOI changes), assembled-vessel drag, temperature offsets. Loader quirks recorded for WP-05 (double fields parsed as float32; crew mass reduction; in-cfg DRAG_CUBE override).
- Repo hygiene: `decompiled/KSP-Decompiled` stays on clean `main`; profiling/capture code lives in worktree `decompiled/KSP-Decompiled-profiling` (branch `profiling`, head 36ef51bd). Core packages are developed in `core/.worktrees/<wp>` on branch `<wp>` and merged into main by an integration step that reruns the full suite.
- Batch 3 complete: wp08, wp11, wp05, wp04 merged into core/main. WP-00L (Sonnet) merged too: Linux build (Clang/GCC presets, run_tests.sh), portable bit-exact libm (machine-code lift of msvcr120_clr0400.dll + ucrtbase sinf/cosf/powf, 0 mismatches in 2M inputs/function), GameData mirror core/testdata/GameData-mirror (6.4 MB; KSP_GAMEDATA_ROOT overrides), CI workflow at .github/workflows/ci.yml (never run yet). Main: 22/22 ctest on Windows MSVC and WSL Clang, every gate 0 ULP. 11 packages done: WP-00L, 01, 02, 03, 04, 05, 07, 08, 09, 11 + integration.
- Cloud session instructions: CLOUD-TASKS.md at project root (queue starts WP-06). Implementer model is Sonnet.
- GitHub push (pending user's word): root is not yet a git repo; core/ and decompiled/KSP-Decompiled are separate repos (import via subtree or vendor). Root .gitignore/.gitattributes written. Private only. Gate filter options unified (README "Golden gate options").
- Follow-up: `harness/compare/compare.py` passes silently when the candidate has rows the golden lacks (641 such rows in part_database, all explained). Add an extra-rows check with an explicit allow-list before more capture gates land.
- Batch 3 originally (worktrees wp04, wp05, wp08, wp11): save model + SaveUpgradePipeline; UrlDir/GameDatabase/PartLoader data side; patched conics + maneuver nodes + Planetarium.Orbits; PQS height maths + LibNoise subset + original heightmap extraction. Merge order when done: wp08, wp11, wp05, wp04 (resolve CMake/STATUS unions; rerun fresh after each).

## Next session, in order

1. Next work packages in dependency order, one Opus agent each, 3-4 at a time, every one gated by core
un_tests.ps1: WP-03 event bus/utilities, WP-07 Orbit (golden orbit_propagation), WP-09 celestial bodies/atmospheres (body_catalog, atmosphere_curves), WP-08 patched conics (patched_conics), WP-11 terrain height maths (LibNoise subset; no golden yet, needs a capture), WP-04 save model, WP-05 GameDatabase/PartLoader + UrlDir, WP-06 module host + KSPField reflection mapping.
2. Captures still useful: terrain heights for the other 15 bodies; in-flight delta-v; ablation thermal case; WP-03's float32 helpers (Heading/Bearing, WrapAround, Color.Lerp) under Mono.
3. Physics spike in plain C++ with Jolt under core/ or a sibling: prototype levers 1 and 2 from PROFILE.md (no per-tick world shift; data-oriented per-part pipeline) then the reduced rigid-body model. Scale spike already proved rendering needs no rebasing.
4. ue5: production terrain path (static/instanced patches; spike is render-thread bound at ~1,800 dynamic meshes); keep Unreal code including only kspcore_c.h.
5. Implementer model from here on: SONNET (user decision 2026-10-09). Opus only as escalation after two failed gate attempts.
6. User intends a PRIVATE GitHub repo for cloud-credit work: exclude decompiled/KSP-dev, harness/lib, all build/ dirs; never public.
7. Keep agent rules: max 3 to 4 concurrent subagents, implementer children Haiku/Sonnet only, progress to PROGRESS.md, compact near 600k tokens.
