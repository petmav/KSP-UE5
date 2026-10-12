# KSP-UE5: state and next steps

Updated 2026-10-12 (session 4, PC; see "Session 4" below for the current state; the older sections are history).

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
- GitHub: PRIVATE repo https://github.com/petmav/KSP-UE5 (pushed 2026-10-09). Root is now ONE git repo (monorepo); core/, ue5/, decompiled/KSP-Decompiled histories imported via subtree; KSPBuildTools vendored. Repo is byte-exact (`* -text`, core.autocrlf=false). Local-only, ignored: decompiled/KSP-dev (game copy; symlink target decompiled/KSP-Decompiled/GameData/Assembly-CSharp-cleaned/ is ignored build output, rebuild with decompiled/tools/scripts/build.sh), decompiled/_archive/KSP-Decompiled.git (bare archive holding the `profiling` branch; worktree decompiled/KSP-Decompiled-profiling is checked out from it), harness/lib, harness/tools-cache, all build dirs. Never make the repo public. Gate filter options unified (README "Golden gate options").
- Follow-up: `harness/compare/compare.py` passes silently when the candidate has rows the golden lacks (641 such rows in part_database, all explained). Add an extra-rows check with an explicit allow-list before more capture gates land.
- Batch 3 originally (worktrees wp04, wp05, wp08, wp11): save model + SaveUpgradePipeline; UrlDir/GameDatabase/PartLoader data side; patched conics + maneuver nodes + Planetarium.Orbits; PQS height maths + LibNoise subset + original heightmap extraction. Merge order when done: wp08, wp11, wp05, wp04 (resolve CMake/STATUS unions; rerun fresh after each).

## Session 3 (2026-10-10, laptop, Fable orchestrating)

Wave 1 merged into `main` (one Sonnet `implementer` per package in a worktree `.worktrees/wp<NN>`, one Sonnet `gatekeeper` per branch, merge by the orchestrator with CMake/STATUS/README unions, fresh `run_tests.ps1` after every merge, push after every merge):

| package | merge | gate | notes |
|---|---|---|---|
| WP-22 thermal model, numeric part | `eea132a3` | `capture.atmosphere_thermal_offsets` (2,688 rows) and `capture.thermal_step` (vessel 2,994 x 13, parts 14,985 x 20, part_constants 15), 0 ULP | the C6 parts gate runs `UpdateThermodynamics` on a one-part graph with the capture's neighbour conduction, occlusion multipliers and incoming sun/body radiation as inputs (5 of 21 parts logged, no transforms); 7 ptd columns that could only be recovered from their own logged value are dropped as inputs, not compared (gatekeeper finding). Not verified: occlusion passes, GetSunArea/GetBodyArea, analytic branch, module FixedUpdate paths |
| WP-23 engines, RCS, gimbals, intakes, numeric part | `34dd0d3f` | `golden.engine_thrust` (5,248 rows, NetFloat policy for the .NET golden) and `capture.engine_deltav` (460 rows, MonoFloat), 0 ULP | game-facing virtuals are MonoFloat; MonoFloat differs from the .NET golden by 1-2 ULP, which is expected (Mono rule). ThrustUpdate returns forces for WP-20; FX, exhaust damage, PAW deferred |
| WP-28 resource-processing modules | `6844d1b5` | unit-gated (17 cases; `tools/wp28/processing_reference.py` independent Python oracle, 0 ULP on 420 catch-up steps) | gatekeeper found 54 wrong C# line citations, fixed; `tools/wp28/check_cites.py [--fix]` now checks `// File.cs:N` cites against the decompile: run it in every package |
| WP-26 fairings, cargo bays, jettison, shielding, launch clamps | `e87c5aab` | `capture.ship_construct` now 17 crafts / 1,225 parts with the ComSat Lx fairing panelMass computed from the craft's XSECTION nodes (exclusion file deleted) | FairingPanel mesh only unit-tested (needs a panel-mesh capture); CompoundPart is a Part subclass; `StrField` helper: null string KSPFields. Optional follow-up: `ModuleServiceModule.BuildJettisonPartsList` should throw on null JettisonZoneNames like the C# |

Main after wave 1: `d46be6e4`, 33 ctest tests, 355 unit cases / 696,649 assertions, every gate 0 ULP, CI green on Windows + Linux.

Wave 2 (branches `wp17`, `wp27`, `wp31`, from `135fb5aa`, told to merge `d46be6e4` before finishing): WP-17 delta-v (gate: the VesselDeltaV columns of `deltav_readout.json`), WP-27 parachutes / deployables / animation modules (unit + Python reference; replaces the WP-06 deployable stand-ins), WP-31 Kerbal roster / experience (gate: recompute every saved kerbal's XP from its flight log vs the saves). Results:

Process notes that paid off: gatekeeper briefs must ask for from-scratch builds (`-Fresh` is only `cmake --fresh`; delete `core/build` first), for a circularity check on any "recovered input" gate, and for a scripted citation check. Each implementer used ~500k tokens and 60-90 minutes; gatekeepers 60-100k tokens.


| package | merge | gate | notes |
|---|---|---|---|
| WP-27 parachutes, deployables, animation modules | `c9931cf8` (STATUS totals `2396200f`) | unit-gated: 16 cases, `tools/wp27/deploy_reference.py` (numpy float32, independent Hermite) regenerates `tests/data/wp27_reference.json` byte-identical; `capture.part_database` module_fields unchanged at 0 ULP (32,894 rows) once the real classes replaced the WP-06 stand-ins | Unity `Animation` semantics in `unity/animation.hpp` are modelled, not verified (needs a capture). Cargo-bay hook subscribes in OnStart, unsubscribes in the destructor through an `IScalarModule::IsAlive` registry (global, unlocked). Optional: tighten the solar `_efficMult` / `_flowRate` bound (4e-7 rel) to ULPs |
| WP-17 delta-v | `90de2da1` | `capture.deltav_stages` (stages 180 x 25, totals 60 x 10, parts 1,225 x 4): 0 ULP on every column except deltaVinVac / deltaVatASL / deltaVActual (<= 11 ULP, rel 1.02e-6) and TotalDeltaVASL / TotalDeltaVActual (rel 1.02e-6; TotalDeltaVVac 1.35e-7). Rules `rel 2e-6` in `tests/tolerances_capture.json`, within policy. 8 stage + 8 totals rows excluded (Dynawing, Learstar_A1 stage 0, omsEngine; Dynawing is +1.5 % off) | Cause (consistent, not proven): the runner stands in each engine's thrustTransform.forward as part -Y; the real transforms live in the `.mu` models, which worktrees and CI cannot read. Single-axis crafts (Kerbal_2, Jumping_Flea, ion probe) are 0 ULP. Flight branches (SimulateFlightScene, CalcLerpDeltaV, vectored exhaust) ported but ungated |
| WP-31 kerbal roster, experience, crew | `1fcad814` | `data.roster_experience` (1 save, 4 kerbals, 0 mismatches; weak: that save has `EnableKerbalExperience = False`, empty logs) + 20 unit cases (effects vs `dictionary.cfg`, CrewGenerator vs an independent KSPRandom replay with synthetic name tables) | real VesselCrewManifest replaces the WP-18 stub; `tools/wp31/extract_crew_names.py` written, never run (needs the game's TextAssets) |

Main after wave 2: `1fcad814`, 35 ctest tests, 401 unit cases / 710,877 assertions, every gate 0 ULP except the WP-17 delta-v columns above. Branches `wp17`, `wp27`, `wp31` pushed. Worktrees under `.worktrees/` are gitignored and can be deleted (`git worktree prune` after).

## Session 3, wave 3 (2026-10-10/11, laptop; the user stayed on the laptop)

Same protocol. Branches from `95ed0a16`; four implementers, four gatekeepers, merged in gate order:

| package | merge | gate | notes |
|---|---|---|---|
| WP-30 misc modules (ModuleGPS, FlagSite, ModulePartFirework) + small fixes | `8ee5a951` (merge before it) | unit (4 cases; `tools/wp30/firework_reference.py`, 0 ULP); part_database unchanged | fixes landed: crew.cpp / resource_modules.cpp cites, `check_cites.py` name aliases, `ModuleServiceModule.BuildJettisonPartsList` null throw, CrewGenerator mission-editor test. Kept a C# bug on purpose (`OnAwake` looks up `Events["LaunchShell"]`, event is `LaunchShellEvent`) |
| WP-25 docking, grapple, struts | `8ee5b6fb` | unit (15 cases; `tools/wp25/dock_reference.py`, 1,080 contact poses 0 ULP except 8 roll-dot cases near cos(pi/2) at abs 1e-15, test-only); part_database / ship_construct / staging_sequence / deltav_stages unchanged | joints, AddForce/AddTorque, pivots are WP-20 seams; WP-16 and WP-17 docking stand-ins removed. Needs a two-port approach capture and a claw capture |
| WP-00 runtime skeleton | `21c76c2a` | unit (17 + 1 cases; `tools/wp00/coroutine_reference.py` on 600 rows of `run1-L_ascent_frames.csv`, 0 ULP); every gate byte-identical | TimingManager + Timing* hosts, PlayerLoop model with the measured phase order, C++20 coroutine scheduler; VesselDeltaV.RunCalculations is a real coroutine. Coroutine resume rules are assumptions, no capture. Other `StartCoroutine` sites (thermal, staging, TimeWarp.CancelAutoWarp, CompoundPart, fairing) not migrated |
| WP-24 command, SAS, reaction wheels | `ef47ccf7` | unit (14 cases; `tools/wp24/sas_reference.py`: 1,100-step VesselSAS replay and 23 wheels x 48 steps, 0 ULP; 299 control-source scenes vs a decision table); gates unchanged | `Quaternion.eulerAngles` / `Euler` formulation unverified (needs a C3-style capture); `ModuleControlSurface.GetPotentialTorque` missing from the SAS torque sum until WP-21 exposes it; CommNet registration is WP-39. Fixed a destructor-order SIGSEGV (ModuleSAS vs dying Part) found by the Linux run; ASan clean |

Main after wave 3: `c172c84b`, 35 ctest tests, 453 unit cases / 731,498 assertions (Windows; Linux counts 16 fewer assertions in some pre-existing test, unexplained, see below). STATUS summary table renumbered 1-26 in order of appearance (rows for WP-06 / 13 / 14 / 15 are still missing from that table; their sections exist).

New follow-ups from wave 3:
- Linux unit total is 16 assertions below Windows (seen on wp24, wp25, wp30 trees alike, so it predates wave 3). Find the platform-conditional test and record it in STATUS.
- `check_cites.py` only checks the name within a +-2 line window; two gatekeepers found wrong line cites it missed. Make it verify the declaration line exactly (or report the distance).
- WSL gatekeeper logs collide on `/tmp/lin_*.log` when several run at once; gatekeeper briefs should name per-branch logs. Linux unit takes 45-50 min per toolchain on /mnt/c under contention.
- Optional test tightening: `tiny <= 16` to `== 8` in `test_docking.cpp`; add the remaining stage fields to the WP-00 scheduler-vs-driver comparison (it also skips silently without a game copy); `CHECK_THROWS_AS` for the firework missing-prefab path; solar `_efficMult` bound (WP-27).
- Agent memory files written by implementers running in a worktree land under `core/.claude/agent-memory/`; fold them into `.claude/agent-memory/` after each wave (done this time).

## Session 3, waves 4-5 (2026-10-11, laptop; the user stayed on the laptop after all)

Same protocol. Branches from `ad208205` (wave 4) and `7199d110` / `dcef6d17` (wave 5); Sonnet implementers and gatekeepers, merged in gate order:

| package | merge | gate | notes |
|---|---|---|---|
| WP-35 science, R&D, tech tree | `b05dfb92`-era (before WP-39) | unit (16 cases; `tools/wp35/science_reference.py`, 84,600 subject values 0 ULP); part_database unchanged | currency / CommNet / UI seams to WP-36 / WP-39 / WP-46 |
| WP-39 CommNet and KerbNet | `b05dfb92` | unit (25 cases; Python reference 0 ULP; `ModuleDataTransmitter` on the WP-00 scheduler) | first-packet gap was a test-driver bug (fixed in wpfix5) |
| WP-40 editor core | `fe7feda1` | unit (11 cases; Python reference 0 ULP) | one native `Euler` / `EulerAngles` / `LookRotation` (`quaternion_native.hpp`, UNVERIFIED vs game); nested `static inline` struct was MSVC-only, fixed |
| WP-36 funds, reputation, strategies, facilities, recovery, progress | `bfd968c8` | unit (31 cases; `tools/wp36/currency_reference.py` 0 ULP); physprofile SCENARIO nodes byte-exact | listener order is last-to-first (`EventData.Fire`); needs GameVariables prefab curves, facility level data, a career save |
| small fixes wave 5 (`wpfix5`) | `bb923bc2` | 35/35; unit +1 case / +49 assertions | Linux is 16 assertions below Windows because `WP-03 vs KSP IL` takes the `CellEquals` NaN fallback for 1 row on Linux vs 5 on MSVC (NaN sign bit): legitimate, documented. One `SqrtF`. `check_cites.py` exact-line mode is the default (`--window K` for the old check); audit `core/tools/wp28/cite_audit_2026-10-11.txt`: 668 mismatches repo-wide (463 off-by-k, 98 bare, 107 to review), nothing fixed outside the touched files |
| WP-37 contract system core and stock templates | `85b0085f` | unit (29 cases; `tools/wp37/contract_reference.py` 0 ULP / byte-exact, 480 generated contracts) | 5 of 8 templates; VesselRepair / OrbitalConstruction / RoverConstruction need WP-38; `check_cites.py --fix` snapped 116 cites after the merge (exact mode) |

Main after wave 5: `85b0085f`, 35 ctest tests, 566 unit cases / 935,159 assertions (Windows; Linux 16 fewer, explained).

In flight at the time of writing:
- `wp17b` (WP-17 to 0 ULP with real `.mu` thrust transforms): `core/tools/wp17/stage_engine_models.py` stages 19 engine parts (cfg + .mu + MANIFEST.json with origin and sha256) into gitignored `core/data/mu_staging/` (run it on a machine with `decompiled/KSP-dev`); `core/tools/wp17/mu_transforms.py` -> `core/data/engine_thrust_transforms.json`. 13 of 17 crafts are 0 ULP on every delta-v column; the Puff is tilted 10 degrees (explains Dynawing / Learstar). Blocked on `sepMotor1` staging. Residuals after that: Kerbal X (6 Swivels, 1 ULP) and Stearwing A300 (turboFanSize2, 2 ULP): to be documented exclusions, not tolerances.
- `wpfix6`: CI Linux clang `unit` SIGSEGV in `test_part.cpp:307` (WP-13 Part prefabs vs C9) on every main push since the wp39 merge; passes locally (WSL clang / gcc) and on Windows. ASan / UBSan hunt. CI `linux` job is red until this lands.
- `wp38` FinePrint (21,322 lines; may hand back a coherent subset) and `wp42` scene flow / KSC / launch sites (fills the WP-36 seams).

Follow-ups from waves 4-5:
- Repo-wide cite audit (668 mismatches, see wpfix5 row): fix per owning package or one sweep with `check_cites.py --fix` + hand review of the 107 "no symbol" cases.
- `corpus.saves` only runs on main with `decompiled/KSP-dev`; it was run after every merge here (35/35).
- Needs data (local): GameVariables prefab (`reputationAddition` / `reputationSubtraction` curves, contract / mentality factors), KSC facility `levelCost` / `KSCUpgradeableLevelText`, a career save with ContractSystem / Funding / Reputation / Facilities / Strategy nodes, the `SiteGeneratorInfo` TextAsset, a C3-style capture for `Quaternion.Euler` / `eulerAngles` / `LookRotation`.
- Next after WP-38 / WP-42: WP-33 inventory / EVA construction (7,547), WP-50 input bindings (7,026), WP-32 EVA controller (8,818, needs WP-20 for the rigidbody part), WP-53 Breaking Ground deployed science, WP-52 robotics (needs WP-20 joints). WP-19 / WP-20 / WP-12 wait for the Jolt spike (Opus `optimizer`). WP-29 wheels (VehiclePhysics) after WP-20.

## Session 4 (2026-10-11/12, PC; Sonnet orchestrating, Opus for the Jolt spike and WP-12a)

Main `dbb068f7`. All four branches left open at the end of session 3 are merged, and so is everything below. Every merge went through a PR with CI green on Windows + Linux (clang and gcc) and a Sonnet gatekeeper (user rule 2026-10-11: code and batched work reaches main only through a PR; docs-only updates may go straight to main).

### Environment on this machine (D:\Projects\KSP-UE5, a fresh clone)
- `C:\Projects\KSP-UE5` is a directory junction to `D:\Projects\KSP-UE5` (hundreds of scripts and csproj files hardcode the C: path). Use `/c/Projects/KSP-UE5` paths for `decompiled/tools/scripts/build.sh`.
- `decompiled/KSP-dev` was copied from the UNMODDED Steam install (`D:\SteamLibrary\steamapps\common\Kerbal Space Program`, build 03190; the user said never to use the modded KSP). Robocopy skipped Steam's preallocated all-zero files because size and time matched: always sha256-compare against the source after a copy (30 files had to be recopied). `Assembly-CSharp` was rebuilt (0 errors, 1,454 warnings) and the DLL/PDB are symlinked into KSP-dev. `core/data/mu_staging` was regenerated.
- NOT on this machine: the `profiling` branch (`decompiled/_archive` bare repo; not on the GitHub remote either), `harness/lib`, `harness/tools-cache`. Captures and profiling builds need the archive copied from the old C: drive. WSL has g++ only (no clang/cmake/ninja); Linux verification is CI (clang + gcc) plus ASan/UBSan with g++ (recipe in `core/README.md`, section 'Sanitizer run').
- The DLC (Breaking Ground, Making History) is not installed: `GameData/SquadExpansion/` is absent from KSP-dev and the Steam install. User decision 2026-10-11: real DLC data is a stretch goal, not needed for feature completion. WP-53/54/55 gates therefore use labelled synthetic fixtures (the mirror's stock mission files are 0 bytes).

### Merged this session
| PR / branch | package | gate |
|---|---|---|
| wpfix7 | sanitizer fixes (firework lambda, random.cpp seed wrap, TestVessel owns a real Vessel) | 34/34 |
| wp38 | FinePrint slice 1 | independent Python reference, CI green |
| wp42 | KSC / launch sites / scene flow (+ F1-F7 fixes) | unit-gated |
| wp17b | WP-17 delta-v with real `.mu` thrust transforms: 15 of 17 crafts 0 ULP; 38 listed exclusion cells (Dynawing, Stearwing A300); quaternion association is an empirical fit marked UNVERIFIED (circular: chosen against the gated rows) | `capture.deltav_stages` |
| #9 wp50 | input, bindings, input locks | independent reference 0 ULP, 32 cases |
| #10 wp53 | Breaking Ground deployed science, ROCs, Serenity contracts | independent numpy reference; fixtures synthetic |
| #11 wp54 | Making History mission runtime slice 1 | 220 synthetic graphs exact, 400 score cases 0 ULP |
| #12 wp33 | inventory and EVA construction (extends wp53's ModuleCargoPart) | 0 ULP numpy reference |
| #13 wp38b | FinePrint slice 2 (24 parameters, 8 contracts) | 4,830 generation cases byte-exact |
| #16 wp38c | FinePrint slice 3 (ContractDefs, Sentinel, CompatibilityUtilities, 3 templates, CometSample) | 0 ULP / byte-exact; 6.9 MB reference |
| #17 wp55 | mission node library slice A (25 tests, 12 actions, 39 adjusters) | 443 cases / 6,087 ops, 0 differences |
| #15 wp20a | physics world + vendored Jolt v5.6.0 + the Jolt spike | analytic tests; determinism at 1/2/4/8 threads |
| #18 wp12a | PQS quadtree and quad build | 24,728 quad rows + 13,051 mesh digests bit-identical; 140,522-point in-game terrain capture still 0 ULP |
| #19 wp19a | flight registry, on-rails propagation, VesselPrecalculate, vessel switching | new ctest `capture.vessel_rails` 0 ULP (6 probes, 1x-100,000x, 4 SOI changes) |
| #20 wp54b | mission slice 2 (data classes, VesselSituation, restrictions, 20 tests, 6 Create* actions) | 1,250 cases byte-exact (great-circle 1e-9 rel, flag rot 1e-5 rel) |
| #22 wpfix8 | whole unit binary clean under ASan + UBSan + LSan on g++ | 806 cases, 0 reports |
| #21 testorder | unit suite passes under `--order-by=rand`; hygiene listener `tests/test_hygiene.*`; ctest `unit.random_order.{11,27,42}` | 25+ seeds green |

Summary: about 42% of all source lines and about 78% of the simulation layer are merged (estimates; partly-done packages counted by guess).

### Decision recorded (Jolt spike, `core/PHYSICS-SPIKE.md`)
Recommendation: WP-20 builds a REDUCED rigid-body model on Jolt v5.6.0 inside kspcore (one body per rigid cluster + structural load graph with KSP's PartJoint capacity rule, double positions, no per-tick world shift, Krakensbane as a velocity offset). 457-part tick 0.33 ms vs 2.83 ms articulated; physics warp within 1% up to 10-25x. Known costs: no joint flex (tall unstrutted stacks 29 g vs 10.8 g first-break), load sharing in strutted craft is gated by scenario only, wheels and robotics not prototyped, Windows-vs-Linux determinism unmeasured. **The user has NOT yet confirmed this model; WP-20b-e are held until they do.** Package split: WP-20a (done), 20b vessel rigid bodies, 20c structural graph + decouple/couple, 20d FlightIntegrator tick (SoA == exact gate), 20e contact/landing/water, WP-19b frames + physics bubble, WP-12b terrain colliders.

### Process lessons (keep)
- Gatekeeper + CI in parallel; merge only on both green. After each merge the other open PRs go stale in STATUS/README: merge main into them, renumber the STATUS row, recompute the summary `unit` row, push, wait for CI again (~25 min per cycle).
- CI Linux runs BOTH clang and gcc. Several bugs showed up only on gcc or only in the combination of two branches (use-after-free in `~Vessel`, destruction order of test fixtures, the root `.gitignore` rule `**/build/` swallowing the vendored `third_party/jolt/Build/`). Use `git add -f` or a negation rule for vendored dirs, and check that every vendored file is tracked.
- Test fixtures must declare `vessels` last (destroyed first). A synthetic event fired with null parts now crashes (`VesselPrecalculate` subscribes to `onDockingComplete`).
- MSVC C1060 'out of heap' appears when several builds run at once: set `CMAKE_BUILD_PARALLEL_LEVEL=6`.
- Subagents' own pushes get blocked by the auto-mode classifier ('Out-of-Place Publication', especially chained `&&` commands): they commit locally, the orchestrator pushes with single simple commands.
- The 4-agent cap is the real throughput limit; the CPU is mostly idle during model waits. `unit` is ~186 s of ~230 s test time in one process; sharding it (`ctest -j`) was proposed and declined by the user.

### Open items
- Run-alone test independence (not memory errors): WP-39 `CommNetVessel control state`, WP-40 symmetry mode cycling, `PartLoader on the GameData mirror` (360 vs 362 parts when run alone).
- `ContractParameter::CreateID` hashes unconditionally but the C# has `if (hashString != null)`.
- `core/data/` needs: GameVariables prefab curves, KSC facility `levelCost`, `SiteGeneratorInfo`, CrewGenerator name tables, a career save, PQS LOD settings beyond min/max level (re-run `tools/wp11/extract_terrain.py` on KSP-dev), ocean spheres; in-game captures: per-tick VesselPrecalculate (gAccel, integrationAccel, drift), engine/part rotations + Quaternion products (WP-17b residual), quad set + one quad mesh at the pad (WP-12a), Euler/LookRotation (WP-24/40).
- `kspcore.lib` now links Jolt (AVX2 CPU needed): the Unreal bridge in `ue5/` has not been touched or rebuilt. `SHA256SUMS` of the vendored Jolt does not verify on a Windows checkout with `core.autocrlf=true`.
- Cite audit (`core/tools/wp28/cite_audit_2026-10-11.txt`) still has hundreds of off-by-k mismatches outside touched files; WP-11's `pqs_mods.cpp` has 4.
- A stray file `D:\scratchpad_run.log` outside the repo (left by an agent, harmless).

### Next (in order)
1. User confirms the reduced rigid-body model -> WP-20b, 20c, 20d, 20e, WP-19b, WP-12b (Opus `optimizer` for 20d/20e; Sonnet otherwise).
2. WP-29 wheels (VehiclePhysics subset, 29k lines), WP-32 EVA controller, WP-52 robotics (all after WP-20).
3. Leftovers: WP-54/55 deferred (TestDistance, ActionPartFailure/Repair/ResourceDrain, MissionSystem, MissionImporting, MissionEditorHistory, real BaseAPField, PositionMobileLaunchPad), WP-53 deferred bits, WP-38 ship spawning / CometManager seams.
4. The UE5 shell (about 289k lines of tier C: UI, rendering, cameras, audio, map view): needs its own design doc first (scene flow, what becomes Blueprints vs C++, first playable slice).

## Handoff to the PC (2026-10-11, end of session 3; all four listed branches are now merged, see Session 4 above)

### Moving the whole directory to another machine

The git remote holds only the repo; `decompiled/KSP-dev`, `decompiled/_archive/KSP-Decompiled.git`, `harness/lib`, `harness/tools-cache`, `core/data/mu_staging`, all build dirs and the Claude memory are local. Copy the directory with symlinks preserved, to the SAME path (`C:\Projects\KSP-UE5`) and the same Windows username if possible:

```
robocopy C:\Projects\KSP-UE5 <dest>\Projects\KSP-UE5 /E /SL /XJ /R:1 /W:1 /MT:16
robocopy C:\Users\<user>\.claude\projects\C--Projects-KSP-UE5 <dest>\Users\<user>\.claude\projects\C--Projects-KSP-UE5 /E
```

Absolute paths are baked into: `.worktrees/*/.git` + `.git/worktrees/*/gitdir`; `decompiled/KSP-Decompiled-profiling/` (worktree of the bare `_archive` repo) and the DLL symlink under `decompiled/KSP-Decompiled/GameData/`; every `core/build` (Windows CMake cache and the WSL `build/linux-*` dirs with `/mnt/c/...`). If the path or user differs: `git worktree repair` in the root and in `decompiled/KSP-Decompiled`; delete every `core/build` and rebuild (`run_tests.ps1 -Fresh`, `./run_tests.sh -f`); recreate the DLL symlink per `decompiled/STATUS.md`. The Claude memory dir is keyed by project path, so a different path starts with empty memory (this file is the handoff either way).

Prerequisites on the new machine: VS 2022 (MSVC x64), CMake >= 3.25, Ninja, Python 3 with numpy, `gh` logged in as petmav, WSL Ubuntu 24.04 with `clang g++ cmake ninja-build python3`, UE 5.8 only for `ue5/`. KSP dev copy is inside `decompiled/KSP-dev`; `decompiled/tools/scripts/launch_ksp.ps1` launches it for captures.


Main `13750b33`: 35 ctest tests, 566 unit cases / 935,159 assertions (Windows; Linux 16 fewer, explained), CI linux + windows green. Four branches are pushed but NOT merged; every agent was stopped mid-verification by the user. Pick each up with a gatekeeper (or implementer for FIX items) against the branch HEAD:

| branch | HEAD | state | what is left |
|---|---|---|---|
| `wp42` KSC / launch sites / scene flow | `3b594cd4` | gate ran all three toolchains green (34/34; unit 576 / 936,215 Windows, 936,199 Linux), CI run 38104664000 green, merge-tree into main clean | implementer FIX items in `core/PROGRESS-wp42-gate.md`: F1 call-site cites `psystem_setup.cpp` (IsFacility :681 -> :693, IsLaunchSite :686 -> :714); F2 `PQSCity::Orientate` / `PQSCity2::Orientate` omit `spaceCenterFacility.SetSpawnPointsLatLonAlt()` (PQSCity.cs:343); F3 `PQSCity2::Orientate` sets `inVisibleRange = false` with no C# counterpart; F4 `space_center_building.cpp:300` `fx(num) + 0.1` must be `0.1f` (SpaceCenterBuilding.cs:728); F5 `SetupLaunchSites` sets `forcePQSPosition = true` for every prefab (C#: only with a PQSMod_FlattenArea); F6 `space_center.hpp` cites 2-3 lines off; F7 optional: Moho/Mohole reference can use `harness/captures/out/libm_mono.json` row 9 (`Math.Cos(pi/2)` = 6.123031769111886e-17) instead of a 1e-9 comparison. Then re-gate the changed files and merge (STATUS row 32; renumber wp38's row to 33) |
| `wp38` FinePrint slice 1 (utilities, OrbitUtilities, waypoints, ProgressTrackingParameter, ExplorationContract) | `43271cde` | gate: Windows 34/34 (unit 579 / 963,143), clang green, STATUS corrections committed by the gatekeeper; gcc run and compare-report / diff-review steps cut off | finish the gate (gcc, compare reports, circularity of `wp37_reference.json` under the monkeypatch, the `GenerateOrbit` rel 1e-12 table: can the reference round-trip through state vectors to reach 0 ULP?), merge. Then WP-38 slice 2: the other 24 FinePrint parameters, 8 contracts (ARM, CometSample, ISRU, Satellite, Station, Survey, Tourism, Base), Sentinel, `VesselUtilities`, full `ContractDefs`, `ScenarioContractEvents`; afterwards WP-37's deferred VesselRepair / OrbitalConstruction / RoverConstruction and GrandTour seams |
| `wp17b` WP-17 delta-v with real `.mu` thrust transforms | `82cf2f34` (WIP) | 15 of 17 crafts 0 ULP on every column; 20 rows (Dynawing 14, Stearwing A300 6; 2-4 ULP) listed in `core/tests/data/deltav_stages_exclusions.json` with self-retiring runner logic in `ds_deltav_stages.cpp`; `rel 2e-6` rules removed from `tests/tolerances_capture.json`; the native quaternion w association `(lw*rw - lx*rx) - (ly*ry + lz*rz)` and the right-nested composition are empirical fits, marked UNVERIFIED | the WIP commit was never run: delete `core/build`, Windows `-Fresh` (34/34 expected), WSL clang; gatekeeper; merge. Local-only data: `core/data/mu_staging/` is gitignored, regenerate with `python -I core/tools/wp17/stage_engine_models.py` (needs `decompiled/KSP-dev`), then `python -I core/tools/wp17/mu_transforms.py`; `core/data/engine_thrust_transforms.json` (committed) is what the runner reads. What settles the residual: one editor-scene capture on the profiling branch of `part.transform.rotation`, `thrustTransform.rotation` / `.forward` (hex float32) for every engine of Dynawing and Stearwing A300, plus a Quaternion product capture (see STATUS WP-17b "Needs capture") |
| `wpfix7` sanitizer fixes | `fec99df4` | items 1-4 committed: firework test lambda lifetime, `random.cpp:29` seed arithmetic wraps defined (bit-identical), `TestVessel` owns a real `Vessel` (was `reinterpret_cast`), STATUS "Sanitizer run 2026-10-11"; last ASan finding was an intentional test-only leak (fix: shared static-lifetime log) | Windows `-Fresh` and WSL clang verification not completed; gatekeeper; merge |

After those merges: STATUS summary renumber (wp42 32, wp38 33), fold `core/.claude/agent-memory` files if any into `.claude/agent-memory/`, update this file.

Next packages (unchanged order): WP-38 slice 2; WP-33 inventory / EVA construction (7,547); WP-50 input (7,026); WP-53 Breaking Ground deployed science; WP-32 EVA controller and WP-52 robotics after WP-20; WP-19 / WP-20 / WP-12 after the Jolt spike (Opus `optimizer`); WP-29 wheels after WP-20; WP-54 / WP-55 Making History.

Repo-wide follow-ups: cite audit `core/tools/wp28/cite_audit_2026-10-11.txt` (668 mismatches, exact-line mode); needs-data list in the waves 4-5 section above (GameVariables prefab, KSC facility `levelCost`, career save, `SiteGeneratorInfo`, Euler / LookRotation / Lerp quaternion captures).

## Handoff to the PC (2026-10-10)

Local-only tasks (need `decompiled/KSP-dev`, which no worktree or cloud VM has):

1. Run `corpus.saves` on the merged main checkout (it only registers where `decompiled/KSP-dev/saves` exists; the laptop's from-scratch runs after each merge did run it, 35/35 green).
2. WP-17 to 0 ULP: extract, per engine part in the 17 crafts (JetEngine, MassiveBooster, SSME, Size3AdvancedEngine, ionEngine, liquidEngine2, liquidEngine2-2.v2, liquidEngine3.v2, liquidEngineMainsail.v2, microEngine.v2, miniJetEngine, omsEngine, radialLiquidEngine1-2, solidBooster.sm.v2, solidBooster.v2, toroidalAerospike, turboFanEngine, turboFanSize2), every transform named by `thrustVectorTransformName` from `decompiled/KSP-dev/GameData/Squad/Parts/Engine/**/*.mu`: local position / rotation / scale and the parent chain to the model root, plus the model-root transform PartReader applies, plus `thrustTransformMultipliers`. Store as byte-exact float32 in `core/data/` with the origin `.mu` path and sha256. Compose the world rotation with the native float quaternion chain (WP-07). Load it in `ds_deltav_stages.cpp` in place of `InstallThrustTransforms`, drop the two `rel 2e-6` rules and `tests/data/deltav_stages_exclusions.json`. Cheaper alternative: a profiling-branch capture of each engine's `thrustTransform.forward` as hex floats per craft (needs a KSP launch).
3. WP-31 data: run `python -I core/tools/wp31/extract_crew_names.py` against the dev copy (the `KerbalNames` / `KerbalSpecialNames` Male/Female TextAssets under `TextAssets/CrewGenerator/<lang>/`; `env.container` may not list `resources.assets`, untested), store under `core/data/` with origin recorded, and replace the synthetic tables in `tests/test_kerbals.cpp`. Then a career save with applicants and non-empty flight logs (or a capture of `KerbalRoster` after `ExperienceAddFlight`) to make `data.roster_experience` a real gate.
4. Captures still wanted: parachute / solar-panel / animation runtime tables (WP-27), in-flight delta-v (WP-17 flight branches), ablation thermal case (WP-22), FairingPanel mesh (WP-26).

Small fixes anyone can do:

- 7 WP-04 cite mismatches in `core/src/save/crew.cpp` (lines 246, 466, 474, 482, 505, 836, 845) and 1 WP-15 one at `core/src/modules/resource_modules.cpp:373` (`EVAResourceContainer.cs` has no `TransferStep`); `check_cites.py --fix` handles the line-number ones.
- `ModuleServiceModule.BuildJettisonPartsList` should throw on null `JettisonZoneNames` like the C# (WP-26).
- Name WP-24 in the `isControlSource` stand-in comments (`ds_deltav_stages.cpp`, `test_kerbals.cpp:1297`); add a test for the mission-editor branch of `CrewGenerator::RandomCrewMemberPrototype`.
- `compare.py` extra-rows check (CLOUD-TASKS item f).
- `core/STATUS.md` summary table: rows for WP-06 / WP-13 / WP-14 / WP-15 are missing and numbers repeat (13 x3, 14 x2, 17 x2); renumber once.

Next packages (dependency order; Sonnet `implementer`, Sonnet `gatekeeper`, same protocol):

1. (done in wave 3: WP-24, WP-25, WP-30, WP-00.)
2. WP-39 CommNet / KerbNet (4,534; WP-24 hooks wait for it), WP-35 science / R&D (6,225), WP-36 funds / reputation / progress (9,201), WP-40 editor core (10,865).
3. Still local-only: the WP-17 `.mu` thrust-transform extraction (the laptop's permission classifier blocked copying the 18 engine `.mu` files out of decompiled/KSP-dev; do it on the PC or allow the copy).
4. WP-19 vessel load / unload / flight registry (6,523) and WP-20 physics core (6,854) need the Jolt spike first (Opus `optimizer`; see item 3 of the old list below). WP-12 PQS quadtree / mesh build (9,326) is `optimizer` tier.
5. After those: WP-29 wheels (29,217, VehiclePhysics subset), WP-35 science, WP-36 funds / progress, WP-39 CommNet, WP-40 editor core.

Protocol that worked (keep): one worktree per package from the current main; implementer brief says the cap (4 concurrent), no children, PROGRESS-wp<NN>.md, compact near 600k tokens, merge main before finishing, `check_cites.py` on every ported file, no reads of `decompiled/KSP-dev/**`. Gatekeeper brief says: delete `core/build` then `-Fresh`, WSL clang + gcc, read every `compare_report_*.json`, circularity check on recovered inputs, scripted cite check, verify STATUS numbers. Orchestrator merges `--no-ff`, resolves STATUS / README / CMake as unions (keep both sides, renumber the summary row, bump the header count), from-scratch suite, measures unit totals with `./build/RelWithDebInfo/tests/kspcore_tests | grep -E "test cases|assertions"`, pushes main + branch. Worktrees show one fewer ctest test than main (`corpus.saves`).

## Older notes (session 1, kept for reference)


1. Next work packages in dependency order, one Opus agent each, 3-4 at a time, every one gated by core
un_tests.ps1: WP-03 event bus/utilities, WP-07 Orbit (golden orbit_propagation), WP-09 celestial bodies/atmospheres (body_catalog, atmosphere_curves), WP-08 patched conics (patched_conics), WP-11 terrain height maths (LibNoise subset; no golden yet, needs a capture), WP-04 save model, WP-05 GameDatabase/PartLoader + UrlDir, WP-06 module host + KSPField reflection mapping.
2. Captures still useful: terrain heights for the other 15 bodies; in-flight delta-v; ablation thermal case; WP-03's float32 helpers (Heading/Bearing, WrapAround, Color.Lerp) under Mono.
3. Physics spike in plain C++ with Jolt under core/ or a sibling: prototype levers 1 and 2 from PROFILE.md (no per-tick world shift; data-oriented per-part pipeline) then the reduced rigid-body model. Scale spike already proved rendering needs no rebasing.
4. ue5: production terrain path (static/instanced patches; spike is render-thread bound at ~1,800 dynamic meshes); keep Unreal code including only kspcore_c.h.
5. Implementer model from here on: SONNET (user decision 2026-10-09). Opus only as escalation after two failed gate attempts.
6. User intends a PRIVATE GitHub repo for cloud-credit work: exclude decompiled/KSP-dev, harness/lib, all build/ dirs; never public.
7. Keep agent rules: max 3 to 4 concurrent subagents, implementer children Haiku/Sonnet only, progress to PROGRESS.md, compact near 600k tokens.
