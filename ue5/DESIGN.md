# UE5 shell design (KspUe5)

Date 2026-10-24. Author: the `optimizer` role (Opus), architecture package for NEXT-STEPS.md Session 5 "Next" item 6.
Status: design only. Nothing here is built yet; the next step is WP-UE-01 (rebuild the bridge against the Jolt-linked kspcore).

This document fixes how the Unreal Engine 5.8 shell in `ue5/` sits on top of kspcore (`core/`): what each side owns, the bridge
contract, scene flow, rendering, UI technology, input / cameras / audio / localization, and the first playable slice as a list of
bounded work packages. Decisions come first in each section, with the reason and the source. Open decisions for the user are in
section 8.

Sources read for this document (all paths relative to the repo root):

- `FEASIBILITY.md` (sections 2.2, 5, 6), `NEXT-STEPS.md` (Decisions, Sessions 4-5), `CLAUDE.md`, `CLOUD-TASKS.md`
- `analysis/tiering/TIERING.md` (sections 4-5, package details WP-41..58), `analysis/tiering/work_packages.csv`,
  `analysis/tiering/tiering.csv`, `analysis/tiering/progress.csv`, `analysis/tiering/unity-api-surface.md`
- `analysis/physics/PHYSICS-ENGINE-REVIEW.md` (sections 1, 5, 8, 11)
- `ue5/README.md`, `ue5/STATUS.md`, `ue5/PROGRESS.md`, `ue5/spike/RESULTS.md`, `ue5/Plugins/KspCoreBridge/**`,
  `ue5/Source/*.Target.cs`, `ue5/Config/DefaultEngine.ini`
- `core/include/kspcore/kspcore_c.h`, `core/CMakeLists.txt`, `core/src/CMakeLists.txt`, `core/include/ksp/vessel/flight_scene.hpp`
- `core/STATUS.md` sections WP-00, WP-12a, WP-12c, WP-19b, WP-20b, WP-20d, WP-20e, WP-29c, WP-40, WP-42, WP-50, WP-mucol;
  WP-20f and WP-29d as they stand on their open PRs (#45 branch `wp20f`, #46 branch `wp29d`)
- Decompile, for facts only: `Assembly-CSharp/ScaledSpace.cs` (scaleFactor 6000), `Assembly-CSharp/GameScenes.cs`,
  `Assembly-CSharp/HighLogic.cs`, `Assembly-CSharp/KSP/UI/Screens/Flight/NavBall.cs`
- `core/data/kerbol_body_assets.json`, `core/data/terrain/manifest.json`, `core/testdata/GameData-mirror/MANIFEST.json`

---

## 0. Decisions at a glance

| # | Decision | Reason (short) | Section |
|---|---|---|---|
| D1 | **Every statement translated from the decompile lives in kspcore**, including the logic of tier C packages (camera controllers, navball maths, HUD readouts, map-view orbit sampling, FX curves, music logic). The shell holds only engine glue and presentation code that is not a translation of KSP code. | One place for ported code: cloud-testable, cite-checked by `pr_review.py`, compiled with core's float flags. The shell becomes small and replaceable. | 1.1 |
| D2 | **The bridge is the C ABI only** (`kspcore/kspcore_c.h`, grown into domain headers, ABI v2). The shell never includes `include/ksp/**`. | Core uses RTTI (122 files) and exceptions (224 files); UE compiles `/GR-`, `_HAS_EXCEPTIONS=0`, and the Game target `/fp:fast`: inline core code compiled under UE flags would be an ODR and numerics hazard. | 1.3 |
| D3 | **Snapshot + command API.** Once per rendered frame the shell pushes input and calls `kspcore_frame(dt)`; it then reads an immutable snapshot (poses, events, quad deltas) and queues commands for the next frame. No callbacks from core into UE during a frame. | Matches Unity's frame (input polled at frame start, scripts, render). Makes moving the sim to its own thread a later switch, not a rewrite. | 1.3, 1.4 |
| D4 | **kspcore runs on the game thread** in a UE tick function at `TG_PrePhysics`. Jolt runs with at most one worker thread inside the shell. | KSP's own model is single-threaded; core singletons are not thread-safe; measured core cost fits the frame (1.4). | 1.4 |
| D5 | **kspcore is compiled by CMake with its own flags** (`/fp:precise`, `/MD`, MSVC 14.44, no `/GL`) and linked as `kspcore.lib` + `Jolt.lib`. The bridge module sets precise FP semantics and guards MXCSR around every core call. A UE automation test proves bit-identity with the headless core run. | The Game target's `/fp:fast` must never touch ported maths; the caller's FP environment must not leak into core. | 1.5, 1.6 |
| D6 | **Snapshot positions are doubles in the dominant-body frame**; the shell places actors relative to its own render anchor (re-anchored past 100 km or on a frame change). No `SetNewWorldOrigin`, no per-tick shifting in UE. | Spike: LWC renders jitter-free when primitives have local origins; UE's rebasing costs ~9 ms per shift with physics. Core's own FloatingOrigin rebases stay invisible to the shell. | 1.3, 3.1 |
| D7 | **One persistent UE level.** KSP scenes (MAINMENU, SPACECENTER, EDITOR, FLIGHT, TRACKSTATION) are actor sets built by a scene director in response to core's `HighLogic::LoadScene`; no `OpenLevel`, no World Partition. | Every KSP scene is built at runtime from game data; UE levels would add load time and binary assets and buy nothing. | 2 |
| D8 | **Game assets are loaded at runtime** from `decompiled/KSP-dev` (loose GameData files) and from a local, gitignored extraction of the Unity-serialized files (`ue5/GameAssets/`, with origin + sha256 manifest). Nothing from the game is cooked into `.uasset` or committed. | Same model as KSP's GameDatabase (cfg-level mods keep working), no GBs of generated content in a public repo, no editor round-trip to add a part. Open decision O1. | 3.7 |
| D9 | **Planet meshes are built by kspcore** (WP-12a PQS, bit-exact) and drawn by a shell `UKspQuadComponent` with a static scene proxy, one primitive per quad (pooled), updated from per-frame quad deltas. | Spike: one ProceduralMeshComponent per quad is render-thread bound at ~1,800 dynamic meshes; per-quad primitives keep local origins (the only jitter failure mode). | 3.2 |
| D10 | **Scaled space in one world**: every body has an actor with the game's ScaledVersion mesh, placed camera-relative at true size when within 60,000 km, otherwise on an impostor shell with the angular size preserved. Map view switches the placement to KSP's scaled units (/6000). No scene-capture background. | WORLD_MAX is 87,960 km; interplanetary distances are not. One render, no second view. | 3.3 |
| D11 | **No Blueprints, no UMG assets.** Game UI is Slate (`SCompoundWidget`) in C++. The few `.uasset` files UE requires (master materials, the persistent map) are generated by editor commandlets from C++ / `.ush` sources and never hand-edited. | Text-only source is what agents can read, diff and review. Confirms and extends the README rule. | 4 |
| D12 | **KSP's own bindings drive input**: the shell implements core's `IInputDevice` (WP-50) from raw UE key / axis / mouse state. Enhanced Input is not used for KSP controls. | WP-50 ported 1,034 binding nodes, locks, double taps and `settings.cfg` bit-exactly; Enhanced Input would re-implement them. | 5.1 |
| D13 | **Chaos is unused.** All KSP actors have collision disabled; gameplay queries (picking, attach raycasts) go to core (Jolt). | Jolt in kspcore is the physics (user decision, reduced model adopted). | 1.2 |
| D14 | **First playable slice = pad to orbit + map view** with stock Kerbal X, plus PhysProf-M2 as the numeric parity gate, in 18 bounded packages WP-UE-01..18. | Exercises every layer (bridge, frames, terrain, parts, sky, HUD, map, input, scene flow) on core code that is already gated. | 6 |

---

## 1. Architecture

### 1.1 What lives where

kspcore already holds every engine-agnostic system (about 42% of all source lines and 78% of the simulation layer as of Session 4;
since then WP-19b, 20b-20e, 29a-29c, 32, 52, 12b, 12c merged; 20f and 29d are open PRs). It runs headless with deterministic tests
and is the authoritative state of the game. The UE5 shell renders that state, collects input, plays sound and draws UI.

Rule D1 extends this to tier C: a tier C package is split into a **core part** (the translated logic and view-model, with `// File.cs:N`
cites and the usual gates) and a **shell part** (widgets, components, materials). Tier C in `tiering.csv` is 254,825 lines of
KSP-authored code (NEXT-STEPS quotes ~289k, which also counts third-party rows such as TMPro); none of it is started
(`progress.csv` 0.0 for WP-34, 41, 43-47, 49, 51, 56-58; WP-48 0.03).

| package | lines (C) | core part (ported, cloud-capable) | shell part (UE, local) |
|---|---|---|---|
| WP-45 UI framework | 59,604 | DialogGUI / PopupDialog model, tooltip text, app-launcher state, KSP rich-text tag parser | Slate widget kit, style set from KSP sprites, rich-text decorators |
| WP-48 rendering, FX, asset import | 34,345 | `.mu` render chunks, texture header decode, EFFECTS / FXGroup / FXCurve evaluation, part highlight and thermal-glow values, ModuleLight state | runtime meshes, master materials, terrain proxy, sky, Niagara, lights |
| WP-56 Making History editor UI | 33,746 | MEGUI node / parameter models (runtime is WP-54, done) | node-graph canvas in Slate |
| WP-43 flight HUD | 29,718 | NavBall / NavBallVector maths, speed display modes, staging stack model, PAW field formatting, gauges, screen messages queue | navball, staging, PAW, gauges, portraits |
| WP-46 UI screens | 26,726 | main menu / settings / load-save / KSC building dialog models | the screens |
| WP-41 editor UI | 17,096 | part categorizer, search, craft browser lists, action-group editor model | part list, craft browser, action-group UI |
| WP-57 tutorials | 14,611 | tutorial page state machines | tutorial dialogs |
| WP-58 debug tools | 14,405 | cheat flags, console command table | console widget |
| WP-44 map view, tracking station | 14,344 | OrbitRendererBase / PatchRendering point sampling, MapNode positions and captions, OrbitTargeter, maneuver gizmo drag maths, SpaceTracking lists | lines, icons, labels, gizmo |
| WP-34 IVA (C rows) | 10,295 | internal props' value logic, portrait selection | IVA scene, portraits (render targets) |
| WP-47 cameras | 6,302 | FlightCamera, PlanetariumCamera, VAB / SPH and SpaceCenter cameras (modes, limits, input response) | apply the pose to a `UCameraComponent` |
| WP-49 audio | 2,496 | MusicLogic, AmbienceControl, AudioFX parameter curves | sound loading and playback |
| WP-51 localization | 2,047 | Localizer is in core (WP-03 key / format shim); language selection | fonts, text shaping |

Line counts: `analysis/tiering/TIERING.md` package table; WP-34 counts its 68 C files only.

### 1.2 Module layout in `ue5/`

| module / plugin | type | contents |
|---|---|---|
| `Plugins/KspCoreBridge` | runtime | links `kspcore.lib` + `Jolt.lib`; `FKspCore` facade: RAII handles, MXCSR guard, unit and axis conversion (Unity Y-up metres <-> UE Z-up centimetres), snapshot views. The only code that includes `kspcore_c.h`. |
| `Source/KspShell` (new; replaces the spike code in `Source/KspUe5`) | runtime | `UKspSimSubsystem` (owns the sim, the tick), `AKspSceneDirector` (scene actor sets), vessel / part / body / quad components, materials binding, asset loaders (mesh, texture, sound), input device, camera actor |
| `Source/KspUi` (new) | runtime | Slate widgets and the style set; reads view-models through the facade |
| `Source/KspUe5Editor` | editor | commandlets: `KspCreateMap` (the persistent level), `KspCreateMaterials` (master materials from `ue5/Shaders/*.ush`) |
| `Source/KspUe5/Tests` | runtime (dev) | automation tests `KspUe5.*` (kept), new tests per package |

Chaos (D13): the engine module stays (it cannot be removed from a launcher build), but nothing creates a physics body. Part,
terrain and scenery components use `NoCollision`. Picking a part under the mouse is a core raycast against the Jolt world (the
`Mouse.CheckHoveredPart` seam of WP-50); the editor's attach raycasts (WP-40, KFSM placement loop not ported yet) will query a core
editor collision world, not UE.

### 1.3 The bridge contract

**C only (D2).** `ue5/README.md` already requires a C ABI because UE builds `/GR-` and without exceptions. Two more facts settle
it: (a) core's headers carry inline float code (the Mono float rule helpers, Unity value types) that a UE translation unit would
compile under `/fp:fast`; (b) core throws and uses `dynamic_cast` across its API. The C header has no inline maths (its own header
comment says so). The ABI grows by domain headers included from the umbrella `kspcore_c.h`, with `KSPCORE_ABI_VERSION 2`:

| header | content | first user |
|---|---|---|
| `kspcore_c.h` (v1, kept) | build info, sim context, ConfigNode, FloatCurve, PQS quads (WP-12a) | existing tests |
| `kspcore_game.h` | process init (GameData root, extraction root, saves dir, threads), GameDatabase / PartLoader load with progress, new game / load / save / quicksave, settings | WP-UE-02 |
| `kspcore_frame.h` | input frame struct, `kspcore_frame(sim, dt)`, snapshot struct and accessors, event list, command queue | WP-UE-02 |
| `kspcore_model.h` | `.mu` model handles: node tree, mesh buffers, materials, texture references; texture decode | WP-UE-05 |
| `kspcore_view.h` | view-models: navball, speed / altitude, staging stack, resources, screen messages, map view (orbit samples, nodes) | WP-UE-04 |
| `kspcore_camera.h` | camera controller pose and mode (in the snapshot), camera commands | WP-UE-03 |

Every struct that crosses the boundary is plain data with a leading `uint32_t struct_size` (forward compatibility), doubles for
positions, floats where KSP stores floats. Arrays in a snapshot are core-owned and valid until the next `kspcore_frame`.

**Per-frame sequence (D3).** One call per rendered frame, on the game thread, from `UKspSimSubsystem`'s tick function
(`TG_PrePhysics`, before cameras and rendering):

```
1. kspcore_input_set_frame(sim, &input)   keys down / up since the last frame (Unity KeyCode numbers), held keys, joystick axes,
                                          mouse position / delta / buttons / wheel, typed text, screen size, focus, UI hover lock
2. kspcore_commands_submit(sim, cmds, n)  UI commands queued since the last frame (stage, SAS mode, warp, map toggle, PAW events ...)
3. kspcore_frame(sim, dt_real)            core's PlayerLoop::Frame (WP-00): n fixed steps (Time.fixedDeltaTime = 0.02 x warp,
                                          maximumDeltaTime 0.04 s per frame, GameSettings.cs:2016), each in the measured script
                                          order (FlightScene, PhysicsBubble, FlightIntegrator, contacts, Jolt step, WaitForFixedUpdate);
                                          then Update (commands applied where KSP's UI handlers ran), coroutines, LateUpdate
                                          (pack / unpack, camera controllers), end of frame
4. snap = kspcore_snapshot(sim)           immutable view (below)
5. shell                                  events -> scene director / actor create & destroy / re-anchor; then transforms of bodies,
                                          vessels, parts, animated nodes, quads, camera, sun, sky; UI view-models refreshed
6. UE renders the frame (render thread)
```

What the shell reads back (snapshot):

| block | content |
|---|---|
| frame | UT, frame count, warp rate and mode, paused, current scene, frame kind (rotating / inertial), dominant body id, frame-change serial |
| bodies | per body: id, position and rotation in the dominant-body frame (double), PQS active / planet fade (`kspcore_pqs_planet_fade`), scaled-space visibility |
| sun and sky | sun direction and colour from core's Sun state, planetarium rotation (the skybox turns with it in the rotating frame) |
| vessels | id, name, loaded, packed, situation, CoM, root pose, part range, active-vessel flag |
| parts | id, vessel, model id, pose (double position, float rotation) per part; sparse list of animated node local poses (gimbals re-posed by WP-20f, WP-27 animations, wheels WP-29); highlight and thermal glow values; FX parameters (sparse) |
| terrain | per active sphere: quads added / removed / rebuilt since the last frame (id, origin, build serial, buffers as in `kspcore_pqs_quad`) |
| camera | pose (double position, rotation), field of view, near clip, mode; map camera separately |
| events | scene load requested (HighLogic), vessel created / destroyed / switched, part died / exploded (FX, audio), structural change, frame change (old -> new transform), floating-origin rebase (informational), screen message, sound cues |

**Frames and the camera anchor (D6).** core's world frame follows KSP: one world frame whose origin FloatingOrigin moves with the
active vessel past 500 m, Jolt bodies tied to it through `FrameOriginWorld`, a physics-origin rebase past 1,000 km, axes body-fixed
below the rotating threshold (100 km on Kerbin) and inertial above (`core/STATUS.md` WP-19b "The frame policy"). The snapshot does not
expose that frame: it exports every position relative to the dominant body's centre (core world position minus
`CelestialBody.position`, double), which is invariant under core's rebases. A frame change (rotating <-> inertial, SOI change) is an
event carrying the old-to-new transform.

The shell keeps a render anchor `A` (double, dominant-body frame) and places every actor at `(p - A)`, converted to UE axes and
centimetres. `A` moves to the camera when the camera is more than 100 km from it, and on every frame-change event. Because all
local-space transforms are recomputed from the snapshot every frame anyway, re-anchoring costs only the re-placement of static
primitives (terrain quads, KSC meshes). This is the spike's recommendation ("rebase rarely, about a 100 km threshold, only where a
float-only subsystem needs it", `ue5/spike/RESULTS.md` "Implications" 2) without `UWorld::SetNewWorldOrigin`, whose `int32` origin
stops at +-21,474 km. Axis conversion: UE (X, Y, Z) = Unity (Z, X, Y) x 100; both systems are left-handed, so it is a permutation and
rotations keep their sense. The conversion lives in one place (the facade) with an automation test.

**Commands.** UI actions are queued and applied at the start of the next frame's Update phase, where KSP's UI event handlers would
have run. Most controls do not need commands: they reach core as keys through the input device (D12), exactly as in KSP.

**Lifetime.** One sim per process. Core has process-wide singletons and static hooks (`FlightScene::hooks`, FlightGlobals,
InputLockManager, GameSettings); `kspcore_sim_destroy` must reset all of them the way the unit tests' hygiene listener does
(`core/tests/test_hygiene.*`). UE runs the game with `-game`; play-in-editor with two worlds is not supported.

### 1.4 Threading and budgets

Measured core costs (MSVC RelWithDebInfo, one thread, laptop):

| what | cost | source |
|---|---|---|
| whole bubble tick, PhysProf-S (21 parts) / M2-AS (153) / L2-AS (457), SoA kernel, no SAS | 0.10 / 0.39 / 1.11 ms | `core/STATUS.md` WP-20d "Measurements" |
| L2-AS whole tick with gimbals, ModuleCommand, ModuleSAS, FlightInputHandler and SAS | 3.51 / 3.77 / 4.25 ms (other runs 5.5-7.6 ms, laptop shared with builds) | WP-20f (PR #45) "Measurements" |
| PQS steady frame at rest | 0.018-0.032 ms | WP-12a "Performance" |
| PQS quad build (heights dominate) | 0.64-1.03 ms per quad | WP-12a |
| PQS StartSphere (scene load), 1 / 15 height workers | 308 / 108 ms (Kerbin) | WP-12a |
| UE frame, 100 km orbit, 456 patches (spike, iGPU, low) | 3.09 ms (game 2.04, render 3.03, GPU 1.83) | `ue5/spike/RESULTS.md` |

A frame runs 0, 1 or 2 fixed steps (PHYSICS-ENGINE-REVIEW.md 1.1). At 60 FPS the average is 0.83 steps per frame; the worst frame is
2 steps. For the slice's Kerbal X (73 parts) the tick is well under 1 ms; for an L2-AS-class craft the worst frame is about 2 x 4 ms
plus PQS builds. That fits a 16.7 ms frame on this laptop, so D4: game thread.

- Jolt: `PhysicsWorld` owns a `JobSystemThreadPool`. The reduced model has few bodies (one per cluster) and most tick time is the
  per-part pipeline, so the shell configures 0-1 Jolt workers (determinism is gated at 1/2/4/8 threads, WP-20b) and leaves the
  cores to UE's task graph and render thread.
- PQS height workers (`height_threads`, bit-identical results): on at scene load (StartSphere 308 -> ~110 ms), 2 workers in flight.
- Moving the sim to its own thread later: D3's snapshot / command split makes this mechanical (double-buffered snapshot, the
  command queue already exists). Trigger: the game thread exceeds budget on the target PC with core > 50% of it. Not before.

### 1.5 Floating point (the `/fp:fast` Game target)

The UE Game target compiles with `/fp:fast` (UBT `VCToolChain.cs`, BuildSettingsVersion V7), the Editor target with `/fp:precise`
(`ue5/README.md`, `ue5/PROGRESS.md`). Decisions:

1. **Core is compiled separately** by its own CMake (`/fp:precise /EHsc`, `core/CMakeLists.txt` `ksp_set_build_flags`), never by UBT.
   The objects in `kspcore.lib` keep their flags whatever target links them.
2. **The shell never does ported float maths** (D1). Shell maths is presentation only: double subtraction for placement, axis and unit
   conversion, interpolation of display values. Nothing the shell computes flows back into core state, except raw input and the
   screen-space mouse ray for picking (KSP's equivalents are float Unity values that no gate covers).
3. **`KspCoreBridge` sets `FPSemantics = Precise`** (README "UE floating-point note") so the facade's conversions are the same in
   Editor and Game builds.
4. **MXCSR guard.** The facade saves MXCSR, sets 0x1F80 (round to nearest, exceptions masked, no FTZ / DAZ: the state the ctest
   binaries run in) before each core call and restores it after. Jolt's own scoped denormal flushing inside its update stays as it is.
5. **Parity gate.** An automation test drives the same scripted input sequence and frame-dt sequence through the UE shell and through
   a core ctest (`capi.flight_slice`, WP-UE-02); the final state hashes must be bit-identical. This catches any flag, FP-environment or
   ordering leak at once.

### 1.6 Linking

- `kspcore.lib` does not contain Jolt: `core/src/CMakeLists.txt` links Jolt `LINK_ONLY` so Jolt's `/arch:AVX2` stays out of
  kspcore's own objects. The bridge must link `Jolt.lib` too. Core will emit a small link manifest (`build/<cfg>/kspcore_link.txt`:
  the libraries and their order) that `KspCoreBridge.Build.cs` reads, so a new dependency never needs a Build.cs edit.
- Jolt is built with AVX2: the shell needs an AVX2 CPU (both machines have one).
- Toolset and CRT as in `ue5/README.md` "Linking the real core": MSVC 14.4x, `/MD`, IDL 0, C++20, no `/GL`, `/Z7`.
- `bUseRealCore` is already `true` in `KspCoreBridge.Build.cs`, but the bridge has not been rebuilt since Jolt was vendored
  (NEXT-STEPS Session 5 open items): WP-UE-01.

---

## 2. Scene flow

**Decision D7: one persistent level, scenes are actor sets.** The level `/Game/Maps/Ksp` (generated by `KspCreateMap`) contains
only the scene director, a default pawn-less player controller and post-process / exposure settings. Scene content is spawned at
runtime from core state and game assets. Reasons: KSP's scenes are themselves built at runtime from data (planets, vessels, parts)
or are Unity level files that we read as data (the KSC and the editors' buildings); UE levels would be empty shells, `OpenLevel`
tears down the world (seconds of load), and World Partition targets authored open worlds while our terrain is procedural.

| KSP scene (`GameScenes.cs`) | shell scene controller | content |
|---|---|---|
| LOADING | startup, before the persistent level's first frame | `kspcore_game_init`: GameDatabase, PartLoader data side (WP-05), localization; shell asset index; Slate progress bar |
| LOADINGBUFFER | loading screen while a controller builds | Slate overlay |
| MAINMENU | `FKspMainMenuScene` | slice: buttons only; later the Kerbin / Mun menu set from the extracted MainMenu scene (WP-46) |
| SPACECENTER | `FKspSpaceCenterScene` | Kerbin PQS around the KSC, KSC scenery render meshes (WP-12c placement), facility picking (WP-42 entry rules), SpaceCenterCamera2 |
| EDITOR | `FKspEditorScene` | VAB / SPH building meshes, editor camera, part list UI (WP-41), placement through WP-40 (KFSM loop still to port) |
| FLIGHT | `FKspFlightScene` | PQS of the dominant body, scaled bodies, sky, vessels and parts, KSC when near, HUD; map view is a mode of this scene, not a scene |
| TRACKSTATION | `FKspTrackingScene` | map presentation only (D10) with SpaceTracking lists (WP-44) |
| PSYSTEM, MISSIONBUILDER, SETTINGS, CREDITS | later | MISSIONBUILDER with WP-56 |

Flow (WP-42 host contract, `core/STATUS.md` WP-42 "Host contract"):

1. Core decides a scene change (`HighLogic::LoadScene`: `onGameSceneLoadRequested`, `onGameSceneSwitchRequested`, flags). The
   `HighLogic::sceneLoader` seam, implemented in core's game host, records a "scene load requested" event.
2. The shell's scene director reads the event, shows the loading screen, tears down the previous controller's actors, builds the new
   one over as many frames as it needs (assets, PQS start, scenery), then calls `kspcore_scene_loaded(sim, scene)`.
3. Core calls `HighLogic::OnSceneLoaded`; `onLevelWasLoaded` / `onLevelWasLoadedGUIReady` fire one frame later (the WP-42 timing).
   FlightDriver / EditorDriver start in core and spawn vessels; the shell sees them as snapshot events.

Map view is not a KSP scene (it is `MapView` inside FLIGHT); the flight controller switches presentation (D10). IVA is a camera mode.

**Save / load entry points.** All persistence is core: the save model and upgrade pipeline (WP-04), ProtoVessel load / unload and
vessel switching (WP-19a), FlightDriver persistence load and revert state (WP-42). The shell calls `kspcore_game_new(mode, name)`,
`kspcore_game_load(name, file)`, `kspcore_game_save(name, file)`; F5 / F9 reach core's QuickSaveLoad through the input device (the
QuickSaveLoad port is a core gap, WP-19b "Seams": host / later; it is part of WP-UE-02). Save directory: `ue5/Saved/ksp/saves/`
(gitignored); `decompiled/KSP-dev/saves` and `core/testdata/saves` are read-only sources for imports and tests.

---

## 3. Rendering

### 3.1 Coordinates and the LWC policy (from the spike)

- LWC is always on in UE 5.8 launcher builds (`WORLD_MAX` 8.796e12 cm = 87,960 km; `bEnableLargeWorlds` deprecated since 5.1;
  `ue5/PROGRESS.md` "Engine facts"). It is not a switch we can turn off.
- The one jitter failure mode is geometry stored far from its primitive origin (7-19 px; `ue5/spike/RESULTS.md` "Verdict"). Rule:
  every primitive's vertices are local to an origin near them: terrain quads on the quad's origin, parts on the part (more exactly on
  their model node), KSC meshes on their object.
- No UE world-origin shifting (D6). Measured cost with a KSP-sized Chaos scene: ~9 ms per shift in the editor, +7.5 ms frame in the
  cooked build, against KSP's ~5.5 ms (spike "Origin rebasing in 5.8").
- Untested at scale and therefore a risk (section 8): Niagara world-space particles, shadows (CSM / VSM), TSR, translucency far from
  the anchor. The 100 km anchor bound limits the exposure.

### 3.2 Planets (PQS)

**Who builds the meshes: kspcore.** WP-12a ported the PQ / PQS / PQSCache quadtree and the quad build, bit-identical to an
independent reference over every built quad, and the live sphere matches the 140,522-point in-game terrain capture at 0 ULP. Heights
come from WP-11's LibNoise + PQSMod stack on the game's own heightmaps, extracted from `sharedassets9.assets` into
`core/data/terrain/maps/*.bin` with origin and sha256 in `core/data/terrain/manifest.json`. Vertex colours are the PQS colour mods'
output (`kspcore_pqs_quad.colors`). Nothing about terrain shape or colour is re-created in UE.

**How the shell draws them (D9).**
- `UKspQuadComponent : UPrimitiveComponent` with a static-relevance scene proxy (cached mesh draw commands), its own vertex / index
  buffers and a transform at the quad's origin. One component per visible quad, pooled like KSP's quad pool.
- The ABI gains a delta form of `kspcore_pqs_update`: added / removed / rebuilt quad ids with a build serial (today it returns every
  quad each update). Only rebuilt quads re-upload buffers.
- Body rotation: in the rotating frame the quads are static; in the inertial frame they hang under one scene component per body that
  takes the body's rotation from the snapshot (one transform update propagates; ~450 children).
- The PQS target is set by core's game host from the active vessel (or camera in the space center), as KSP does; the shell never
  computes LOD.
- Material: one master material "PQS ground" (vertex colour, KSP's detail textures by altitude / slope, the planet fade as opacity
  cross-fade with the scaled body). Detail textures, their tiling and blend parameters come from the PQS material of each body in the
  extraction (WP-UE-06). The look is an approximation of KSP's terrain shaders; the shape and colours are exact.
- Oceans: the ocean child spheres (KerbinOcean ...) are not extracted yet (WP-12a "Needs data" 3); the slice uses a sea-level sphere
  shell with KSP's ocean colour as a stand-in, labelled as such, until the extraction lands.

### 3.3 Scaled space, sky, sun

- **Scaled bodies (D10).** Each body gets an actor with its ScaledVersion mesh and materials as indexed in
  `core/data/kerbol_body_assets.json` (`scaledVersion.mesh`, `materials`, transform scale). In flight it is drawn at true size
  (ScaledVersion scale x 6000, `ScaledSpace.cs` `scaleFactor`) camera-relative at its true position when within 60,000 km of the
  camera; beyond that it sits on an impostor shell at 60,000-85,000 km (distance monotonic in true distance, so nearer bodies stay in
  front), scaled to keep the true angular size. The dominant body's scaled actor cross-fades with its PQS by the planet fade.
- **Map view and the tracking station** place the same actors in KSP's scaled units: position = (p - map focus) / 6000, as KSP's
  ScaledSpace / PlanetariumCamera do. Eeloo's orbit (~90 Gm) becomes ~15,000 km: inside `WORLD_MAX`.
- **Atmosphere.** UE renders one `USkyAtmosphereComponent`; it is assigned to the dominant body when it has an atmosphere (the spike
  verified Kerbin's radius and 70 km height). Its Rayleigh / Mie parameters are fitted once per body to KSP's `AtmosphereFromGround`
  values (wavelength, Kr, Km, scale depth) by screenshot comparison; other bodies' atmospheres are the rim material of their scaled
  mesh. Approximate by design (visual only; the physics atmosphere is core's WP-09 curves).
- **Sun.** One directional light; direction from core's Sun / body positions; colour and intensity fitted to KSP's sunlight curves.
  Fixed exposure (as the spike). The Sun's scaled mesh plus a lens-flare stand-in from KSP's flare textures.
- **Skybox.** KSP's GalaxyCube textures (extraction) on an unlit cube centred on the camera, rotated by the planetarium rotation from
  the snapshot (GalaxyCubeControl's role).

### 3.4 Part models

- **Reader in core.** WP-mucol's `.mu` reader (`core/include/ksp/parts/mu_model.hpp`) already consumes every PartReader chunk and keeps
  the colliders. WP-UE-05 makes it keep the render chunks too (MeshFilter meshes with normals / tangents / uv / uv2 / colours / submeshes,
  MeshRenderer and SkinnedMeshRenderer, materials with shader name and properties, texture references, lights, animation clips) and
  exposes them through `kspcore_model.h`. Core needs the transform tree and the animation curves anyway (WP-27 animation sampling,
  WP-20f gimbal chains, WP-29d wheel geometry), so one reader serves both.
- **Mesh build in the shell.** At first use of a model the shell builds one `UStaticMesh` per unique mesh (runtime
  `BuildFromMeshDescriptions`, no Nanite) and per part instance a tree of scene components mirroring the model's nodes, with a
  `UStaticMeshComponent` per renderer. Animated node poses come from the snapshot (sparse). Skinned meshes (few stock parts) use a
  `USkeletalMesh` built at runtime; they are not in the slice.
- **Textures.** DDS (895 in Squad's GameData), PNG, MBM and TRUECOLOR are read at runtime. DDS BC1 / BC3 / BC5 blocks upload as
  `PF_DXT1 / PF_DXT5 / PF_BC5` without transcoding; KSP's DXT5nm normal maps are swizzled in the material; header decode is core
  code (testable on the mirror's header-only DDS stubs), the GPU upload is shell code.
- **Materials.** A fixed table maps each KSP shader name found in the `.mu` material chunks (`KSP/Bumped Specular`, `KSP/Specular`,
  `KSP/Diffuse`, `KSP/Emissive/*`, `KSP/Alpha/*`, `KSP/Unlit` ...) to a master material; each part material becomes a
  `UMaterialInstanceDynamic` with the chunk's colours, textures and tiling. Unknown shaders log a count and fall back to the nearest
  master (the slice requires the count to be 0). Thermal glow and highlight are scalar parameters fed from the snapshot.
- Origin recording: each loaded asset carries its GameData path; the shell logs it on load errors and the model cache keys on
  (path, sha256).

### 3.5 KSC and other scenery

WP-12c extracted the collider side of all 59 PQSCity / PQSCity2 objects (`core/data/scenery/`), placed bit-exactly with WP-42's
`PQSCity::Orientate`, including facility levels. The render side reuses the same object tree: WP-UE-06 extracts the render meshes,
materials and textures of the same GameObjects (same pathIDs) into the local staging dir; the shell builds them like part models and
places them with the same city pose, so render and collision cannot drift apart. Facility level (career level vs sandbox highest) is
chosen by core (`SceneryLevels`). Destruction states switch subtrees as `CollapsibleObject` does in core (WP-42); the collapse
animation is presentation (shell).

### 3.6 FX

- **Model plumes first.** Most stock engines' exhaust is `MODEL` / `MODEL_MULTI_PARTICLE` effects in their EFFECTS nodes. Core evaluates
  EFFECTS (FXGroup, FXCurve power / volume / pitch, the per-module FX state WP-23 deferred) and publishes per-effect parameters in the
  snapshot; `MODEL` effects are `.mu` meshes whose scale / alpha the shell sets: no particles needed for the slice.
- **Particles.** `MODEL_MULTI_PARTICLE` and `PREFAB_PARTICLE` (prefabs in `squadcorefx.ksp`) map to a small set of generic Niagara
  systems parametrised from C++ (texture, emission rate, speed, size and colour curves sampled by core). The Niagara assets are
  generated by a commandlet from an engine template. Niagara is float-only in local space; world-space smoke trails depend on
  Niagara's LWC support, untested (risk R3). After the slice.
- Explosions, splashes, reentry streaks, the Kerbal flag: after the slice; core already emits the events (`PartContactEnvironment::explodeFx`,
  `ContactHost::splashFx`).

### 3.7 Asset pipeline (D8)

| source (in `decompiled/KSP-dev`) | how it reaches the shell |
|---|---|
| loose GameData files: `.cfg` (core's GameDatabase), `.mu`, `.dds`, `.png`, `.mbm`, `.truecolor`, `.wav`, `.ogg` | read at runtime from `KspGameRoot` (config, default `<repo>/decompiled/KSP-dev`); models lazily on first use in the slice, all at LOADING once the editor needs icons |
| Unity-serialized assets (`KSP_x64_Data/*.assets`, `squadcore.ksp`, `squadcorefx.ksp`): scaled-space meshes and textures, PQS materials, GalaxyCube, KSC / VAB / SPH / main-menu scenes, UI sprites, fonts, FX prefabs, music | extracted once per machine by `ue5/Tools/extract_game_assets.py` (UnityPy, like `core/tools/wp12c/`) into `ue5/GameAssets/` (gitignored) as raw formats (mesh binary + JSON, DDS / PNG, TTF, WAV / OGG), with a manifest (origin file, pathID, sha256) and a second independent verification route (the WP-12c pattern); read at runtime like the loose files |
| derived data core already commits | `core/data/terrain`, `core/data/scenery`, `core/data/kerbol_body_assets.json`, `core/data/engine_thrust_transforms.json` (used through core) |

Never: hand-authored substitutes, assets cooked into `.uasset`, or extracted bulk committed (CLAUDE.md "Original game assets": derived
data that tests need may be committed; bulky staging stays gitignored). A packaged build reads the same paths; nothing from the game is
staged into the pak.

### 3.8 Render settings

Two profiles in config: **Laptop** (the spike's settings: no Lumen, no VSM, no distance fields, no TAA / TSR for measurement runs,
CSM shadows on parts only within ~200 m, 1280x720) and **PC** (TSR, VSM, higher PQS detail) for the stronger machine (NEXT-STEPS
user decision: heavy shader and perf work later). The visual target is "KSP's look", not a re-lit UE5 look (open decision O7).
Shader compile: cold DDC is 6.2 minutes on the laptop (spike "Shader compile time"); master materials keep permutations few.

---

## 4. Blueprints, UMG and C++ (D11)

`ue5/README.md`: "No Blueprints: everything is C++ so agents can read and change it." **Confirmed, and extended to UI and assets:**

| asset kind | decision | reason |
|---|---|---|
| gameplay and shell logic | C++ only | as README; most of it is core anyway (D1) |
| game UI (HUD WP-43, map WP-44, screens WP-46, editor UI WP-41, dialogs WP-45, mission editor WP-56) | **Slate** (`SCompoundWidget`, declarative C++), hosted with `UGameViewportClient::AddViewportWidgetContent` | plain text, diffable, reviewable; Slate builds UIs as complex as the UE editor itself (part list, tech tree, node graph); no designer-only state; UMG built from C++ is clumsier than Slate and its widget blueprints are binary |
| styles and images | an `FSlateStyleSet` in C++; brushes from KSP's extracted UI sprites (`FSlateDynamicImageBrush` on runtime textures) | KSP's own look, no re-drawn art |
| in-world labels (map view node captions, vessel markers) | Slate overlay at projected screen positions | no `UWidgetComponent` needed |
| materials | generated by `KspCreateMaterials` from C++ and `ue5/Shaders/*.ush` (Custom nodes and material functions) | shader logic stays in text files; the generated `.uasset` is a build artefact |
| the persistent map | generated by `KspCreateMap` (as `KspCreateSpikeMap` does today) | |
| Niagara systems (after the slice) | generated by a commandlet from an engine template, parameters from C++ | keeps FX reproducible; risk R3 |
| input actions / mapping contexts | none (D12) | |

Generated `.uasset` files are committed (UE needs them to cook) and must be reproducible by `ue5/Scripts/gen_content.ps1`; nobody
edits them in the editor. A Blueprint or widget blueprint in a PR is a review failure.

---

## 5. Input, cameras, audio, localization

### 5.1 Input (WP-50 done in core)

- **D12.** Core's WP-50 port reads keys and axes through `InputEnvironment::device` (`IInputDevice`), with GameSettings' 456 fields,
  `SetDefaultValues`, `settings.cfg` read / write, the nine shipped keyboard layouts, InputLockManager and FlightInputHandler's
  `Update` / `FixedUpdate` maths, all gated bit-exactly (`core/STATUS.md` WP-50). The shell implements the device: an
  `IInputProcessor` records key down / up events and mouse state each frame; a generated table maps UE `FKey` to Unity `KeyCode`
  numbers (from core's `keycode.hpp`); joysticks come through UE's raw input as Unity's "joystick N axis M" indices.
- Enhanced Input is not used for KSP controls: its triggers / modifiers would duplicate what the port already does exactly, and its
  assets would be one more binary format. Shell-only debug keys may use it.
- **UI vs game input.** When the pointer is over a Slate widget, the shell reports it in the input frame; core applies the KSP.UI lock
  helpers (`CanvasGroupInputLock`, `KSPGraphicRaycaster` semantics, ported in WP-50) so the same InputLockManager masks apply as in KSP.
- Focus loss: the input frame carries focus; core's `OnApplicationFocus` handling (`refocusThrottle`, WP-50 findings) runs.

### 5.2 Cameras (WP-47, ported into core)

FlightCamera (auto / free / orbital / chase / locked, mouse orbit, zoom limits, terrain clamp), PlanetariumCamera (map view and
tracking station), the VAB / SPH cameras and SpaceCenterCamera2 are ported into core as `core/src/cameras/` (D1), driven by the input
device and the frame clock, and their pose (double position in the dominant-body frame, rotation, FOV, near clip) is in the snapshot.
The shell has one `ACameraActor`-like pawn whose `UCameraComponent` takes that pose every frame. UE's camera uses a reversed-Z infinite
far plane, so KSP's two stacked local cameras (near and far) collapse into one; the scaled-space camera disappears with D10. IVA
(WP-34) is a later camera mode. Reason for porting the controllers instead of writing UE camera code: their behaviour is KSP's
(including how the camera follows frame changes and vessel switches), they need the core's frames, and in core they are testable
headless against a Python reference.

### 5.3 Audio (WP-49)

- Sounds are the game's: `GameData/Squad/Sounds/*.wav`, the alarm and other loose files, and music from the Unity assets (extraction).
  The shell decodes WAV itself and OGG with UE's bundled Vorbis into PCM-backed `USoundWave`s at load.
- What plays, how loud and at what pitch is core's: EFFECTS AUDIO entries evaluated with FXCurves per engine / module (WP-UE-16), sound
  cue events (staging, decouple, explosion, alarm), MusicLogic and AmbienceControl ported into core. The shell owns `UAudioComponent`s
  (3D for parts, 2D for UI and music) and applies volume / pitch / play state from the snapshot.
- Mixing: KSP's settings volumes (master, music, ship, ambient, UI) from GameSettings map to five `USoundClass`es set from C++.

### 5.4 Localization (WP-51)

- Strings: core's Localizer (WP-03 key lookup and format shim; tables from `GameData/Squad/Localization/*.cfg`). The shell never uses
  UE's localization tables or `.po` files; every UI string is `kspcore_localize(key, args...)` -> `FText::FromString`.
- Fonts: KSP's own fonts from the extraction (TTF payloads of its font assets) loaded at runtime into `FCompositeFont`s; language
  selection is KSP's setting. KSP rich-text tags (TMP subset: colour, bold, size, sprite) become `SRichTextBlock` decorators; the tag
  parser is core (WP-45 core part).

---

## 6. The first playable slice

### 6.1 Definition

**S1 "pad to orbit".** From a cold start of `KspUe5.exe` (or editor `-game`) on this laptop:

1. Minimal main menu (Slate): "Start pad-to-orbit sandbox", "Resume quicksave", "Quit".
2. Core creates a sandbox game (GameSettings defaults, sandbox facility levels) and launches the stock **Kerbal X**
   (the stock file, committed as `harness/inputs/Ships/VAB/Kerbal X.craft`; 73 parts, asparagus, 8 stages; already flown stage by
   stage in WP-16 and used by WP-17 / WP-18 tests) onto the KSC LaunchPad through core's launch path (LaunchSiteFacility / FlightDriver / ShipConstruction: the "VAB launch
   sequence onto the pad" host item of branch `wp20g`).
3. FLIGHT scene: Kerbin terrain from WP-12a with KSP's colours, the KSC pad and its neighbours rendered from KSP's meshes, sky
   atmosphere, skybox, sun, Mun and Minmus as scaled bodies, Kerbal X with KSP's meshes and textures, engine plumes (MODEL effects)
   and engine sound.
4. The player flies with KSP's default keys: throttle (Shift / Ctrl / Z / X), SAS (T), stage (Space), pitch / yaw / roll
   (W S A D Q E), time warp (, .), camera (mouse, V). HUD: navball with prograde / retrograde / normal / radial / SAS markers, SAS
   mode buttons, throttle gauge, altitude and speed (surface / orbit modes), staging stack, resource totals (LF / OX / SF / EC),
   screen messages.
5. Reaches a stable orbit (periapsis above 70 km). Map view (M): Kerbin, Mun, Minmus, the vessel's orbit line with Ap / Pe markers and
   values; focus cycling (Tab); rails time warp in orbit.
6. Quicksave / quickload (F5 / F9), revert to launch and quit to the menu (Esc menu).

**Not in S1:** KSC scene, VAB / SPH, tracking station, EVA, IVA, part action windows, career / science / contracts UI, settings screen
(settings come from `settings.cfg`), particles, explosions, music.

### 6.2 What it needs from core

Core packages (all merged or on open PRs): WP-01-05 (data, ConfigNode, GameDatabase), WP-06 module host, WP-07-10 (orbits, patched
conics, bodies, warp, rails), WP-11 / 12a / 12b / 12c (terrain, colliders, KSC colliders), WP-13-18 (part tree, vessel, resources,
staging, delta-v, craft files), WP-19a / 19b (registry, bubble, frames), WP-20a-e (Jolt reduced model, structure, FlightIntegrator,
contacts), WP-21-24 (aero, thermal, engines, SAS), WP-26 / 27 (fairings, deployables), WP-42 (scene flow, launch sites), WP-50 (input).
**WP-20f (PR #45) is required** (SAS and FeedInputFeed in the tick; M2 matches KSP's C10 ascent to 36.7 km at 0.1%).

**Host contract items in progress (branch `wp20g`)**, needed before WP-UE-07 can fly anything: `Part.packed` hosted through
GoOnRails / GoOffRails, `GameSettings.SetDefaultValues` at startup, the flight-scene resource-flow host (`ResourceFlowEnvironment::vesselHost`,
`Vessel::env.updateResourceSets`; WP-20d finding), servo modules in the module slot, the VAB launch sequence onto the pad. Also listed
by WP-20d / 20f findings and to be covered by the game host (WP-UE-02): the module lifecycle before the first flight tick
(ModulesOnStart, OnStartFinished, first ModuleJettison.LateUpdate: it sets the drag-cube weights KSP flies with), Part.Update's
PartValues refresh for all parts (a probe's SAS needs it), `loadedScene == FLIGHT`.

**Core gaps the slice adds** (ported code, so they go to core per D1; all cloud-capable): the production game host (the composition
the test fixtures do today: `PadScene`, `physics_bubble_fixture.hpp`), the C ABI v2, QuickSaveLoad, camera controllers, HUD and map
view-models, the `.mu` render reader, EFFECTS evaluation.

### 6.3 Work packages

Sizes are rough: new lines of code (not counting tests) and agent-days at the observed pace (an implementer package ~60-90 min of
agent time per 1,000 ported lines plus gating; UE packages slower because of the 27 s build-test loop and visual checks). "Cloud"
means it can be implemented and gated in a cloud session (Ubuntu, `core/run_tests.sh -f`, no game data beyond the mirror); "local"
needs UE 5.8 and / or `decompiled/KSP-dev`.

| WP | name | side | where | size | depends on | acceptance (measurable) |
|---|---|---|---|---|---|---|
| WP-UE-01 | Bridge rebuild on the Jolt-linked core | shell | local | ~300 lines, 1 day | - | core RelWithDebInfo `/MD` builds; the bridge links `kspcore.lib` + `Jolt.lib` via the link manifest in Editor and Game targets with 0 warnings; MXCSR guard; `KspUe5.Bridge.LinksCore` passes (ABI, CRT, IDL); new `KspUe5.Bridge.JoltStep`: a core physics scenario stepped through a test ABI call gives the same state hash as its ctest twin |
| WP-UE-02 | Game host and C ABI v2 (`kspcore_game.h`, `kspcore_frame.h`) | core | cloud | ~2,500 lines, 4-6 days | wp20g items, WP-20f | product-side composition of the flight scene (PlayerLoop + FlightScene + PhysicsBubble + FlightIntegratorHost + ContactHost + SceneryCityHost + PQS + FlightInputHandler) behind `kspcore_frame`; snapshot as in 1.3; QuickSaveLoad; ctest `capi.flight_slice` (C only, through the ABI): M2 launched as C10 did reproduces WP-20f's C10 states at the same tolerances and the C++ path's state hash bit for bit; `kspcore_sim_destroy` + re-create passes the hygiene checks |
| WP-UE-03 | Camera controllers in core (WP-47 slice: FlightCamera, PlanetariumCamera) | core | cloud | ~2,000 lines, 3-4 days | UE-02 | Python reference over 20 scripted mouse / key sequences per mode (auto, free, orbital, chase, locked, map): camera pose 0 ULP; pose in the snapshot |
| WP-UE-04 | HUD and map view-models (NavBall, NavBallVector, speed / altitude display modes, staging stack, resource totals, screen messages; orbit patch sampling, Ap / Pe / SOI nodes) | core | cloud | ~2,000 lines, 3-4 days | UE-02 | Python reference 0 ULP for navball rotation and marker vectors and displayed numbers over recorded flight states; orbit samples equal `Orbit` evaluations at the sampled anomalies exactly |
| WP-UE-05 | `.mu` render reader, texture headers (`kspcore_model.h`) | core | cloud (synthetic) + local sweep | ~1,500 lines, 3 days | - | independent Python reference on a synthetic coverage model with every render chunk layout (as WP-mucol did for colliders): every value bit for bit; local sweep over all 551 stock `.mu`: 0 parse errors, chunk counts per part recorded; DDS / MBM / PNG header decode on the mirror's stubs and a local sweep of 895 DDS |
| WP-UE-06 | Asset extraction tool (`ue5/Tools/extract_game_assets.py`) | data | local | ~1,500 lines Python, 3-4 days | - | outputs for the slice: 17 scaled bodies (mesh + textures), Kerbin PQS material textures, GalaxyCube, KSC render meshes for the pad and its neighbours, KSP fonts, slice UI sprites, music not needed; manifest with origin + pathID + sha256; second independent route (`verify_*`) 0 mismatches; extraction twice gives identical sha256 |
| WP-UE-07 | Sim subsystem, frame loop, placement | shell | local | ~1,500 lines, 3-4 days | UE-01, UE-02 | `UKspSimSubsystem` + tick function + render anchor + axis conversion; vessels drawn as their collider proxies (debug); `KspUe5.Sim.AxisConversion` (round trips, handedness); **`KspUe5.Slice.HeadlessParity`**: M2's scripted C10 ascent for 90 s with a fixed dt sequence through UE (`-nullrhi`) ends in the same state hash as `capi.flight_slice`; re-anchor count and cost logged |
| WP-UE-08 | Input device | shell | local | ~1,000 lines, 2 days | UE-07 | generated FKey -> KeyCode table covers all 326 KeyCodes or lists the unmapped ones; injected key / mouse sequences produce the same FlightInputHandler state as the WP-50 reference rows (bit for bit); UI hover lock blocks flight keys |
| WP-UE-09 | Camera application | shell | local | ~400 lines, 1 day | UE-03, UE-07 | `UCameraComponent` pose equals the snapshot pose after conversion (1e-6 m relative, 1e-6 rad); mode switch (V) and map camera work |
| WP-UE-10 | Part rendering | shell | local | ~2,500 lines + master materials, 5-7 days | UE-05, UE-07 | Kerbal X and M2 render with their meshes and textures; log count of unknown shaders / missing textures = 0; animated nodes follow the snapshot (gimbals visibly deflect); model load time for Kerbal X measured (target < 2 s warm); screenshot `slice/shots/parts_pad.png` |
| WP-UE-11 | Terrain | shell | local | ~2,000 lines, 4-5 days | UE-07, UE-06 | `UKspQuadComponent` pool from quad deltas; visible quad count equals core's every frame; descent from 100 km to 10 m: render thread average <= the spike's 3.98 ms and no frame > 30 ms from quad uploads (laptop, low); screenshots at 6 altitudes |
| WP-UE-12 | Scaled space, sky, sun, skybox | shell | local | ~1,500 lines, 3-4 days | UE-06, UE-07 | Mun disc diameter at a known distance within 1 px of geometry (the spike's method); planet fade crossover without a visible pop (max luminance step at the crossover < 2%); screenshots pad / 70 km / orbit |
| WP-UE-13 | KSC pad scenery rendering | shell | local | ~800 lines, 2 days | UE-06, UE-10 | render meshes placed by the same `PQSCity::Orientate` pose as WP-12c's colliders; 41 vertical rays on the pad: render-surface vs collider-surface height within 1 cm |
| WP-UE-14 | Flight HUD slice (Slate) | shell | local | ~3,000 lines, 5-7 days | UE-04, UE-08 | every displayed number equals the core view-model on 300 recorded frames (automation reads the widget text); staging by Space updates the stack the same frame core does; screenshot next to a KSP screenshot of the same state for layout review |
| WP-UE-15 | Map view slice | shell | local | ~2,000 lines, 4-5 days | UE-04, UE-09, UE-12 | M toggles presentation; orbit line vertices equal core samples after the /6000 conversion (1e-6 relative); Ap / Pe labels equal core numbers; Tab cycles focus; screenshot |
| WP-UE-16 | EFFECTS evaluation and audio (core part + shell part) | core + shell | cloud (core) / local | ~1,200 + ~800 lines, 3-4 days | UE-02, UE-10 | core: Python reference for FXCurve / FXGroup power -> scale / alpha / volume / pitch over a throttle sweep, 0 ULP; shell: MODEL plumes and engine loops follow the snapshot; listening check by the user |
| WP-UE-17 | Scene flow slice | shell | local | ~1,200 lines, 2-3 days | UE-07..15 | menu -> flight -> F5 -> F9 -> Esc -> revert -> menu in an automation test; HighLogic event order as WP-42's test; quickload state hash equals the quicksaved one; cold start to "flight ready" timed (target <= 30 s warm DDC) |
| WP-UE-18 | Slice integration and playtest | shell | local | ~500 lines + notes, 2 days | all | **`KspUe5.Slice.KerbalXToOrbit`**: a scripted pitch program and staging through the input device (`-nullrhi`) reaches Pe > 70 km; with rendering at 1280x720 low on this laptop, average frame <= 16.7 ms on the pad and in the ascent, worst 1% frame and core share logged; the user's playtest checklist (6.1 items 1-6) signed off in `ue5/STATUS.md` |

Totals: about 26,000 lines (core ~9,200, shell ~16,000, tools ~1,500) and 50-65 agent-days; the critical path is
01 -> 02 -> 07 -> 10 -> 11 -> 14 -> 17 -> 18.

### 6.4 Order and parallelism

```
local:  UE-01 ─┬─────────────── UE-07 ── UE-08 ── UE-09 ─┬─ UE-10 ── UE-13 ─┬─ UE-14 ── UE-15 ── UE-17 ── UE-18
               │                  ▲                       ├─ UE-11            │                    ▲
        UE-06 ─┼──────────────────┼───────────────────────┴─ UE-12 ───────────┘        UE-16(shell)┘
cloud:  UE-02 ─┴─ UE-03 ── UE-04 ─┴─ UE-05 ── UE-16(core)
```

- Cloud sessions take UE-02, 03, 04, 05 (synthetic part) and the core half of UE-16 in that order; at most one cloud package at a time
  touches `src/capi/` (ABI conflicts).
- Locally, UE-01 and UE-06 start at once (no dependencies); UE-07 starts when UE-02's ABI header is frozen (its first commit), not
  when UE-02 is finished.
- UE packages cannot run in CI (GitHub runners have no UE). Their PRs attach the automation report (`Saved/Automation/*.json`
  summary), the timings line from `Scripts/timings.log` and the screenshots; CI still gates the core side and `pr_review.py`.
  Branch names `wpue01` ... `wpue18`; `progress.csv` gets rows `WP-UE-01` ... (or the shell part counts towards the owning tier C
  package).

### 6.5 After the slice

In order of user value: KSC scene (WP-42 shell part, facility picking), VAB / SPH (WP-40's KFSM placement loop into core, then WP-41
UI; editor collision world in core), PAW (WP-43), tracking station (WP-44), particles and explosions (WP-48), EVA (WP-32 render side),
IVA (WP-34), career screens (WP-46, R&D tree), settings screen, KSPedia, then WP-56 / 57 / 58.

---

## 7. Interfaces to keep stable

- `kspcore_frame` semantics: one call per rendered frame with the real frame `dt`; core owns the fixed-step accumulator and all of
  Time.* (WP-00). The shell never steps physics on its own.
- Snapshot positions in the dominant-body frame (double); the shell's anchor is the shell's business.
- Commands applied at the next frame's Update phase.
- Every KSP control through the input device; commands only for UI-originated actions.
- Asset access through paths relative to `KspGameRoot` / `GameAssets`, keyed by (path, sha256).

---

## 8. Risks and open decisions

### 8.1 Open decisions for the user (recommendation first)

| # | question | recommendation | alternative and its cost |
|---|---|---|---|
| O1 | Game assets at runtime or imported at build time? | **Runtime** (D8): loose GameData files from `decompiled/KSP-dev`, Unity-serialized assets from a local extraction, nothing cooked or committed. | Import into `.uasset` by a commandlet: faster loads, texture streaming, Nanite; but GBs of generated content outside git, a re-import for every data change, the editor in the loop, mods only after re-import. |
| O2 | UI technology | **Slate in C++** (D11); amend the README rule to "no Blueprints, no widget blueprints; generated `.uasset` only from commandlets". | UMG widget blueprints: designer convenience, opaque to agents. UMG from C++: possible, clumsier than Slate. |
| O3 | Threading of kspcore | **Game thread now** (D4), snapshot / command API so a sim thread is a later switch. | A sim thread from day one: +1 frame latency, more bridge code, no measured need on the slice's crafts. |
| O4 | Interpolate physics poses for rendering? | **No** (KSP renders the latest tick). Revisit if 144 Hz monitors show stepping. | Interpolation in the shell (presentation maths, allowed by D1): smoother at high FPS, one tick of visual lag. |
| O5 | Scaled space | **One world, true-size bodies within 60,000 km, impostor shell beyond** (D10). | KSP's two-camera model with a scene-capture background: closer to KSP, one extra scene render per frame on the iGPU. |
| O6 | Enhanced Input for KSP bindings? | **No** (D12): KSP's ported bindings through a raw device. | Enhanced Input: UE-native remapping UI, but a second binding model to keep consistent with `settings.cfg`. |
| O7 | Visual target | **KSP's look first** (KSP shaders emulated, no Lumen / Nanite), a PC quality profile later. | A re-lit UE5 look (Lumen, PBR conversion of KSP textures): prettier, not KSP, and costly on the laptop iGPU. |
| O8 | Slice craft | **Kerbal X** for orbit, **PhysProf-M2** for the numeric parity gate. | M2 / L2-AS only: already gated to C10, but they are test rockets, not the stock playable experience. |
| O9 | LWC on / off | **Not a decision**: always on in 5.8 launcher builds; the policy is local-origin primitives and a camera anchor (D6). | Turning it off needs an engine source build and brings back KSP's float32 jitter (5.6 cm at Kerbin, 72 cm at the Mun, spike). |
| O10 | Chaos | **Unused** (D13): Jolt in core is the physics; UE collision off on every KSP actor. | Chaos for picking / UI traces only: a second collision world to keep in sync. |
| O11 | `CLAUDE.md` still says "This private build ..." while the repo is public | Keep extracted bulk and imported assets out of git (D8); commit only code, manifests, small derived data and review screenshots. | Commit extracted assets: makes cloud sessions able to render tests, but publishes KSP's art. |

### 8.2 Risks

| # | risk | likelihood | mitigation |
|---|---|---|---|
| R1 | Core singletons make one sim per process; a missed reset leaks state between automation tests or scene restarts | medium | `kspcore_sim_destroy` reuses the unit-test hygiene resets; WP-UE-02 gate re-creates the sim |
| R2 | Runtime model and texture loading is slow (551 `.mu`, 895 DDS) | medium | lazy loading in the slice; a derived cache under `Saved/` keyed by sha256 later; measure in UE-10 and UE-17 |
| R3 | Niagara / shadows / TSR / translucency untested far from the anchor (spike "Unverified") | medium | 100 km anchor bound; particles after the slice; a spike-style jitter test when UE-16 adds particles |
| R4 | One `USkyAtmosphereComponent` per scene: the second atmospheric body in view (Laythe from Jool) uses a rim material only | certain | accepted (visual only); revisit with a custom atmosphere shader if it matters |
| R5 | The host items on `wp20g` and the game host (UE-02) are larger than they look (module lifecycle, resource flow host, launch path) | medium | UE-02 starts from the test fixtures that already fly M2 / L2-AS (`flight_integrator_fixture.hpp`) and promotes them to product code |
| R6 | UE iteration loop for visual work (27 s build + test; 6.2 min cold shader compile) slows agents | medium | keep logic in core (D1); shell packages small; one UE instance at a time |
| R7 | UE sets a different FP environment (FTZ / DAZ) on the game thread | low | MXCSR guard (1.5) and the parity test |
| R8 | Kerbal X behaves differently on the reduced rigid-body model (no joint flex) and is hard to fly | low | WP-20c / 20f gates cover similar stacks; M2 parity gate isolates shell bugs from model effects |
| R9 | The ocean spheres, PQS LOD fields and some PQS material data are not extracted yet (WP-12a "Needs data") | certain | labelled stand-ins in the slice; WP-UE-06 extends the extraction |
| R10 | Linking: `Jolt.lib` and future core dependencies, AVX2 | low | link manifest from CMake; build-info string reports the instruction set |

---

## 9. Reproduce / where things are

- Current shell: `ue5/README.md` (one-liners, test list, linking contract), `ue5/STATUS.md` (what works, timings), `ue5/spike/RESULTS.md`.
- Core host-facing contracts: `core/STATUS.md` sections WP-00 (PlayerLoop, phase order), WP-19b (frame policy), WP-20b / 20d / 20e (bodies,
  tick, contacts), WP-29c (wheels in the tick), WP-12a / 12c (terrain, scenery hosting API), WP-42 (host contract), WP-50 (input seams);
  WP-20f and WP-29d on PRs #45 / #46.
- This document's package list feeds NEXT-STEPS.md Session 5 "Next" item 6.
