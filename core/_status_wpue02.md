

## WP-UE-02: game host and C ABI v2 (branch `wpue02`, from main `da6db948`)

State: done for the kspcore side of the slice's bridge, with the exceptions under "Not done". Scope = `ue5/DESIGN.md` 1.3 / 6.2 / 6.3: the product-side
composition of the flight scene behind `kspcore_frame`, the two ABI headers (`kspcore_game.h`, `kspcore_frame.h`), the module lifecycle and the other
host items of WP-20g's "Host contract", quicksave / quickload / revert through the ABI, and the gates `capi.flight_slice` (pure C) and its C++ twin.
**Header freeze: commit `643355d8`** (the two headers alone); every later header change is additive and listed in `core/PROGRESS-wpue02.md`.

### What is in

| piece | files | KSP source |
|---|---|---|
| ABI v2 headers: process init, database load with progress, new game / load / save / list saves / quicksave / quickload / launch / revert / scene flow, settings; input frame, commands, `kspcore_frame`, snapshot, hash | `include/kspcore/kspcore_game.h`, `kspcore_frame.h`, `kspcore_c.h` (ABI_VERSION 2, includes the domain headers) | `ue5/DESIGN.md` 1.3 |
| C entry points (every one catches all C++ exceptions, `kspcore_last_error`) | `src/capi/kspcore_game.cpp`, `kspcore_frame.cpp`, `capi_internal.hpp`, `kspcore_c.cpp` (shared error / `kspcore_sim`) | |
| `GameProcess`: once per process (KSP's start-up scene): game root, GameDatabase, PartLoader data side, Physics.cfg, terrain maps, model ids | `include/ksp/host/game_host.hpp`, `src/host/game_process.cpp` | GameDatabase / PartLoader / PhysicsGlobals loading, `KSPUtil.ApplicationRootPath` (KSPUtil.cs:795) |
| `GameHost` (one per `kspcore_sim`): the celestial system, GameSettings.Awake (GameSettings.cs:1506), HighLogic (HighLogic.cs:112), the scheduler and PlayerLoop, the frame input device (`IInputDevice` over the input frame), commands, events, snapshot | `src/host/game_host.cpp` | |
| The flight scene composition, in WP-20g's host-contract order: TimeWarp (Awake / Start, `TimeWarp.cs:213`), PhysicsWorld with the game's PhysicMaterials (WP-12c), MuPartColliders, PhysicsBubble (`Install`), the rotating frame, engines' thrust transforms (WP-17b data), FlightInputHandler (Awake / Start / Update / FixedUpdate), Kerbin's PQS sphere with terrain colliders (`ContactHost::HostTerrain`), the KSC colliders (`SceneryCityHost`, every facility at its top level), `PSystemSetup`'s launch sites, FlightDriver (Awake / Start: `NEW_FROM_FILE` and `RESUME_SAVED_CACHE`), the vessel's entry into the scene (Part.Start with the module lifecycle, rigid bodies, `OrbitDriver.Start`), `OrbitPhysicsManager.Start`, StageManager (`BeginFlight`), QuickSaveLoad | `src/host/game_host.cpp` `Flight` | FlightDriver.cs:108-376, TimeWarp.cs, StageManager.cs:1209, QuickSaveLoad.cs; the test fixtures `PadScene` / `BubbleScene` / `KerbinScene` / `InputScene` / `test_vab_launch.cpp` promoted to product code |
| Frame: `PlayerLoop::Frame(dt)` (fixed steps by Unity's rule, `Time.*` plumbing), the script "KspHost.Flight" at the `Normal` slot: FixedUpdate = Planetarium.FixedUpdate, TimeWarp.FixedUpdate, the bubble's tick; Update = commands, TimeWarp.Update, FlightInputHandler.Update, QuickSaveLoad.Update, the bubble's Update, PQS update; LateUpdate = the bubble's LateUpdate | `src/host/game_host.cpp` | `core/timing_manager.hpp` measured order |
| Launch: `kspcore_game_launch_craft` runs the craft through KSPUpgradePipeline (`KSPUpgradePipeline.cs:35`: the stock Kerbal X has the renamed `liquidEngine1-2`), keeps the upgraded craft in the save's `Ships/VAB`, fills every crewable seat from the roster (`KerbalRoster.GetNextOrNewKerbal`, KerbalRoster.cs:281: stands for the launch dialog's default fill), then `FlightDriver.StartWithNewLaunch` (FlightDriver.cs:461) | `src/host/game_host.cpp` `GameHost::LaunchCraft` | |
| Scene flow: `HighLogic.LoadScene` -> `sceneLoader` seam -> event `SCENE_LOAD_REQUESTED`; the old scene goes at the next frame (Unity destroys it at the level switch); `kspcore_scene_loaded` composes the new one | `src/host/game_host.cpp` | HighLogic.cs:211, :310 |
| Settings by field name over `GameSettings::Fields()`, load / save through `ParseCfg` / `BuildCfg` | `src/host/settings.cpp` | GameSettings.cs:1619, :1691 |
| `HygieneProbe` (read-only version of the hygiene listener's checks) and `HygieneLeaks` | `tests/test_hygiene.cpp/.hpp` | |
| C10 states extracted bit for bit from the capture (origin path and sha256 in the file) | `tests/data/c10_states.csv`, `tools/wpue02/extract_c10_states.py` | `harness/captures/out2/aero_forces_vessel.json` |

### Gates

| test | what | result |
|---|---|---|
| `capi.flight_slice` (pure C, `tests/capi/capi_flight_slice.c`) | process init, load with progress, new game, launch PhysProf-M2 from the KSC LaunchPad, scene load, 100 settle frames, then KSP's own keys in the input frame (Z full throttle, T SAS, Space stage) as C10 did; 4,560 frames at dt 0.02 compared with KSP's C10 states (`c10_states.csv`) at WP-20f's tolerances; settings round trip; quicksave and the save list; destroy and re-create a sim | **pass**, [FILL] s |
| `capi.flight_slice_twin` (unit case "WP-UE-02 flight slice twin") | the same scripted run through the `GameHost` class directly and through the extern "C" entry points from C++, and the hash the pure-C program wrote: **bit for bit the same** snapshot hash; 0 hygiene leaks after each run | **pass**, hash `2c4ae66445a41c81` (all three) |
| unit "WP-UE-02 sim destroy / re-create" | two sims in one process: 0 hygiene leaks after each `kspcore_sim_destroy`, identical hash of the same scripted run | pass |
| unit "WP-UE-02 quicksave / quickload" | quicksave on the pad (KSP refuses it under thrust), fly, quickload: the scene load is requested (event), the old scene is gone at the next frame, `kspcore_scene_loaded` rebuilds it from the file: altitude 101.598 m = the saved one, 153 parts, UT = saved + the five frames run | pass |
| unit "WP-UE-02 revert to launch" | `FlightDriver.RevertToLaunch` through the ABI: back on the pad, PRELAUNCH, 153 parts | pass |
| unit "WP-UE-02 the stock Kerbal X" | the stock craft (73 parts) launches through the upgrade pipeline; throttle and stage keys lift it (92.8 m -> 304.2 m in 340 frames); the first stage leaves 70 parts | pass |

Ascent against C10 through the ABI (ours - KSP, relative; the same quantities and tolerances as WP-20f's C++ path, which are copied here for comparison):

| state | speed at the altitude | altitude at the Mach number | frames since the previous state |
|---|---|---|---|
| alt_1000 / mach_0.5 / alt_2500 / mach_1 / alt_5000 | -0.122 / - / -0.051 / - / -0.086 % | - / +0.252 / - / +0.048 / - % | +0.571 / -0.424 / 0.000 % |
| mach_1.5 / alt_10000 / alt_20000 | - / +0.036 / +0.054 % | +0.058 / - / - % | +0.251 / 0.000 / -0.107 % |
| final (KSP frame 4500: 36,672.5 m, 600.5 m/s) | 36,683.1 m (**+0.029 %**), 600.7 m/s (**+0.043 %**) | | |
| tolerances used (WP-20d / WP-20f, not loosened) | 1 % (max seen 0.122) | 1.5 % (max seen 0.252) | 2.5 % (max seen 0.571) |

(WP-20f's C++ fixture path: final +0.042 % / +0.084 %.) Our frames run about 26 ticks behind KSP's (the capture's frame 0 is not our ignition tick; WP-20f finding).
The pad is the real KSC LaunchPad (colliders and spawn transform), not WP-20f's 30 x 30 terrain patch.

### Deviations from `ue5/DESIGN.md` 1.3 (each with its reason)

- Functions that DESIGN lists without a handle (`kspcore_game_new / load / save / quicksave`) take the `kspcore_sim*` first: it names the owner (one sim per process), and a quickload needs the sim's scene.
- The snapshot type is `kspcore_snapshot_view`; `kspcore_snapshot(sim)` returns it (a C type and a function cannot share the name `kspcore_snapshot`).
- `kspcore_snapshot_hash`: the ids of vessels and parts are not hashed (process-wide GUID / instance counters differ between two sims of one process); `kspcore_vessel_state` got appended flight values (`altitude`, `surface_speed`, `mach`, `vertical_speed`) because no HUD view-model exists before WP-UE-04 and the C10 gate needs them.
- Part ids are `Part.flightID` (the port has no `Part.persistentId`); vessel ids `Vessel.persistentId`.
- Camera, animated node poses (gimbals), FX parameters, highlight / thermal glow, sun colour: present in the structs, zero, as DESIGN specifies for the packages that own them (`kspcore_camera_state.valid = 0`, `node_count = fx_count = 0`).
- Commands implemented: STAGE, THROTTLE, SAS_ENABLE, ACTION_GROUP, TIME_WARP_INDEX, PAUSE, QUICKSAVE, QUICKLOAD, REVERT_TO_LAUNCH, LOAD_SCENE. Accepted and ignored (documented in the header as later packages' work): SAS_MODE, SET_ACTIVE_VESSEL, MAP_VIEW, PART_EVENT.
- Events implemented: SCENE_LOAD_REQUESTED, SCENE_LOADED, VESSEL_CREATED / DESTROYED / SWITCHED, PART_DIED, PART_EXPLODED (no position), FLOATING_ORIGIN_REBASE, FRAME_CHANGE (the old-to-new rotation is the identity: UNVERIFIED what the shell needs), SCREEN_MESSAGE (FlightInputHandler's and QuickSaveLoad's). Not produced: STRUCTURE_CHANGED, SOUND_CUE.
- CommNet is off in the host's new games (`Difficulty.EnableCommNet = false`; KSP's sandbox default is on): with it on a probe core's control level comes from its CommNet connection (Vessel.cs GetControlLevel) and the KSC ground station / link graph is not part of this host (`CommNetScenario.Instance` is null then). M2 and Kerbal X need control to fly.

### Findings (a later implementer must know)

- **TimeWarp must be Awake'd and Start'ed**: a TimeWarp with `curr_rate` 0 makes `TimeWarp.fixedDeltaTime()` 0, so every engine's `requestedMassFlow * fixedDeltaTime` is 0, `RequestPropellant` returns 0 and the engines are "ignited" with zero thrust (found by bisecting why M2 sat on the pad with throttle 1 and 17 ignited engines).
- **KSP's sandbox new game has CommNet enabled**: see above; the probe core showed "Partial Control", control level 0, with `CommNetScenario.Instance` alive.
- **FlightDriver's statics are the NEXT scene's parameters** (`StartupBehaviour`, `StateFileToLoad`, `FlightStateCache`...): they are set by `StartWithNewLaunch` / `StartAndFocusVessel` before `HighLogic.LoadScene`, so tearing a scene down must not `ResetStatics()` them (the quickload started a NEW_FROM_FILE launch until this was fixed).
- **A craft from the harness may need the upgrade pipeline** (the stock Kerbal X: `liquidEngine1-2`): the VAB's craft browser runs it; `ShipConstruction.LoadShip(path)` does not.
- **KSP refuses a quicksave under thrust** (`FlightGlobals.ClearToSave`, QuickSaveClearToSave): the ABI returns 0 with the message; the gates quicksave on the pad.
- **A flight to orbit leaves process state behind** (`FlightGlobals` navball speed mode = Orbit, found by the hygiene probe after the 4,660-frame run): `GameHost` resets it; a scene that changed more would show the same way in `HygieneProbe::Leaks`.
- `GameData::Load` registers GameEvents listeners of the prefab part modules (36 on `onVesselSituationChange`...): they are the process level's, so the probe is taken after `kspcore_game_load_database`.
- Two sims of one process end the same scripted run in the same hash only because the GUID hash source is per sim (it was a function-local static).
- The stock Kerbal X lifts off with the throttle and stage keys through the whole ABI path; its crew seats are filled by the host (UNVERIFIED that the Kerbals board: no snapshot field shows crew).

### Not done

- Camera pose (WP-UE-03) and view-models (WP-UE-04): reserved slots only. Gimbal / animation node poses, FX parameters, thermal glow, sun colour: reserved.
- Commands SAS_MODE, SET_ACTIVE_VESSEL, MAP_VIEW, PART_EVENT; events STRUCTURE_CHANGED, SOUND_CUE.
- CommNet network (KSC ground station): off.
- The save dialog's UI side (QuickSaveLoad's dialogs) beyond `kspcore_game_save`, `kspcore_game_list_saves` and quicksave / quickload.
- `pqs_height_threads > 0` and `physics_threads > 0` are passed through (results are documented bit-identical, WP-12a / WP-20b) but no gate here runs with them.
- Non-FLIGHT scenes start nothing (MAINMENU / SPACECENTER / EDITOR: the scheduler and the HighLogic events run, no objects).
- Vessels that come into range later (a docked or decoupled vessel's physics) run through the bubble as in the fixtures; no gate here covers a second vessel.
- Kerbin only: the dominant body's PQS sphere is Kerbin's (the PQS of another body is not built when the SOI changes).
