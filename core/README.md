# kspcore

Engine-agnostic C++20 reimplementation of the Kerbal Space Program 1.12.5 simulation core (the Unreal Engine 5 shell
lives elsewhere and links this library). Private, local-only project; the behavioural spec is the decompiled game under
`..\decompiled\KSP-Decompiled\` and every number that matters is checked against the golden harness in `..\harness\`.

Status, per-dataset errors and open items: [STATUS.md](STATUS.md). Working notes: [PROGRESS.md](PROGRESS.md).

## Build, run, test: one command

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\core\run_tests.ps1
#   -Preset debug        Debug instead of RelWithDebInfo
#   -Fresh               reconfigure from scratch
#   -TestFilter golden   only tests whose name matches (ctest -R)
#   -BuildOnly           configure + build, no tests
```

`run_tests.ps1` locates Visual Studio 2022 Community with `vswhere`, imports its x64 developer environment
(`VC\Auxiliary\Build\vcvars64.bat`), then runs `cmake --preset <p>`, `cmake --build --preset <p>` and
`ctest --preset <p>`. Exit code 0 means every unit test and every golden gate passed.

Building from a git worktree (`core\.worktrees\<name>`) needs no flags: the top-level `CMakeLists.txt` walks up from the
source directory until it finds `harness/compare/compare.py` and uses that directory as `KSP_PROJECT_ROOT` (a cache
variable; `-DKSP_PROJECT_ROOT=<dir>` still overrides it). `run_tests.ps1 -Fresh` works in the main checkout and in
worktrees alike.

Manual equivalent from a "x64 Native Tools Command Prompt for VS 2022":

```bat
cd C:\Projects\KSP-UE5\core
cmake --preset relwithdebinfo
cmake --build --preset relwithdebinfo
ctest --preset relwithdebinfo
```

### Linux (and the cloud): one command

```bash
cd core
./run_tests.sh -f                  # preset linux-rel (Clang if installed, else the default compiler), fresh configure
./run_tests.sh -p linux-gcc-rel -f # GCC
#   -p <preset>  linux-rel | linux-dbg | linux-gcc-rel     -f fresh     -t <regex>  ctest -R     -b  build only
```

Needs cmake >= 3.28, ninja, a C++20 compiler (Clang 16+ or GCC 13+) and python3. Build trees: `build/<preset>/`.
Everything the gates read is in the repository: `harness/` (goldens, captures, inputs), `data/` (extracted terrain and body
assets), `testdata/GameData-mirror/` (see below). No KSP install, no Unreal, no Windows is needed.

**Portable libm.** `dotnet::LibmBackend()` (also in `kspcore_build_info()`, field `libm=`) says where the VS2013 CRT maths come
from: Windows loads `msvcr120_clr0400.dll` / `msvcr120.dll`; every other host, or Windows with `-DKSP_LIBM_PORTABLE=ON` (or env
`KSPCORE_LIBM_PORTABLE=1`), runs a bit-exact software lift of those DLLs' machine code (`src/dotnet/vs2013_libm_gen.cpp`,
`ucrt_libm_gen.cpp`; generator `tools/libm/gen_main.py`; details STATUS.md "WP-00L"). `libm.portable_vs_dll` (Windows only)
compares the two on 300,000 inputs per function.

**Game data for the tests.** The tests read `testdata/GameData-mirror/` (built from the game by `tools/mk_gamedata_mirror.py`; every
`.cfg` byte-identical, empty placeholders with the same paths for everything else, `.dds`/`.mbm` keep their headers, `MANIFEST.json`
lists counts and SHA-256s) and `testdata/logs/*.log`. Set env `KSP_GAMEDATA_ROOT=<game root>` (the directory with `GameData/`, e.g.
`decompiled/KSP-dev`) to run the same tests against a real install; `-DKSP_GAMEDATA_ROOT=...` changes the compiled-in default.

### Toolchain on this machine

| tool | path / version |
|---|---|
| MSVC | VS 2022 Community 17.14, MSVC 14.44.35207 (`C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\HostX64\x64\cl.exe`) |
| CMake | 3.31.6-msvc6, bundled: `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe` (a system CMake 4.3.2 is also on PATH; either works, >= 3.28 required) |
| Ninja | 1.12.1, bundled: `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe` (the `ninja` on PATH is Strawberry Perl's 1.12.0; the script puts VS's first) |
| Python | 3.12.10 on PATH (`python -I` runs `harness\compare\compare.py`; found by CMake's `find_package(Python3)`) |
| Windows PowerShell | 5.1 on .NET Framework 4.8: only needed to regenerate `tests/data/netfx_reference.tsv` |

Presets (`CMakePresets.json`): `debug`, `relwithdebinfo` (x64, Ninja, cl.exe). Build trees: `build/Debug/` and
`build/RelWithDebInfo/`; the static library lands at `build/RelWithDebInfo/kspcore.lib` (the path the Unreal bridge
links).

## What the tests are

| ctest name | what it checks | reference |
|---|---|---|
| `capi.c`, `capi.ue_like` | the C ABI (`include/kspcore/kspcore_c.h`) from a pure C program and from a C++ program built with Unreal's flags (`/GR- /EHs-c- _HAS_EXCEPTIONS=0 /fp:fast`); checks `crt=MD`/`idl=0` and bit-exact FloatCurve results through the ABI | golden values |
| `unit` | doctest unit tests (`tests/`), including `test_netfx_reference.cpp`: 29,028 bit-exact reference results from the .NET Framework 4.8 runtime (parsing, formatting, Math.Sin/Cos/Tan/Ceiling/Round), and `test_wp03_reference.cpp`: 6,395 WP-03 values computed by KSP's own IL on .NET Framework 4.8 (UtilMath, KSPUtil time/date/SI/coordinate/string helpers, Localizer + Lingoona fallback, Simplex, event-bus call orders and re-entrancy, GameEvents registry, .NET Dictionary order / List.Sort / Enum formatting) | `tests/data/netfx_reference.tsv`, `tests/data/wp03_reference.tsv` |
| `unit` (WP-10) | `tests/test_timewarp.cpp`: TimeWarp (prefab tables vs 19 game runs and the C8 capture, rate selection per body/altitude, physics-warp vs rails mode switching, autoWarpTo), WarpTransitionCalculator, PositionTarget, OrbitDriver TRACK_Phys / IDLE / UPDATE transitions | the decompiled C# control flow + the game's rate tables and body prefabs (no golden or capture; see STATUS.md "WP-10") |
| `golden.math_shim.f64` | WP-01 KSP double maths: Vector3d, QuaternionD, UtilMath, CelestialFrame (runtime-independent) | `harness/golden/math_shim` (table f64) |
| `capture.math_shim.f32` | WP-01 Unity managed float maths (Mathf, Vector3, Quaternion) under Mono's float32 rule | in-game `harness/captures/out/math_shim_mono.json` (table f32) |
| `capture.libm_mono` | runtime layer: System.Math (25 functions x 3,093 inputs), float32 expression evaluation probes, numeric conversions, number formatting and parsing, System.Random (~312k rows) | in-game `harness/captures/out/libm_mono.json` |
| `capture.unity_random` | WP-01 UnityEngine.Random (xorshift128, value/Range/vector functions/ColorHSV, traced states) and KSPRandom | in-game `harness/captures/out/unity_random.json` |
| `golden.confignode_roundtrip` | WP-02 ConfigNode Load/Save/ToString/Parse; the runner also checks byte-exact `saved/*.cfg` and tree-exact `trees/*.json` | `harness/golden/confignode_roundtrip` |
| `golden.curves_eval` | WP-01 FloatCurve / Unity AnimationCurve: 366 curves, 28,484 evaluations | `harness/golden/curves_eval` (== native Unity, CAPTURE-SPEC C1) |
| `golden.orbit_propagation` | WP-07 Orbit: Init, propagation (positions, velocities, anomalies, UpdateFromUT, time to Pe/Ap), UpdateFromStateVectors, Kepler solvers in all three regimes (2,422 rows) | `harness/golden/orbit_propagation` |
| `libm.portable_vs_dll` | WP-00L (Windows only; prints "skipped" elsewhere): the portable VS2013-CRT / UCRT libm (`dotnet::vs2013::*`, `dotnet::ucrt::*`) against `msvcr120_clr0400.dll` / `ucrtbase.dll`, 300,000 inputs per function | the DLLs on the machine |
| `corpus.gamedata` | every `.cfg` under `harness/inputs/GameData` and `.craft` under `harness/inputs/Ships`: load, Save->Load->Save byte-identical, Parse == Load, every `key` node loads as a FloatCurve | self-consistency |
| `corpus.saves` | WP-04: every `.sfs` / `.craft` under `decompiled/KSP-dev/saves` and every stock `.craft` under `decompiled/KSP-dev/Ships`: `Game(ConfigNode)` / `ShipConstruct.LoadShip` -> Save (R1) -> again (R2); R2 == R1 for every file, R1 byte-identical or every difference explained by a documented KSP rule (`tools/save_corpus`), files written by KSP 1.12.5 byte-identical once the mod-owned elements KSP's loaders drop are removed | original game data |
| `golden.physics_constants` | WP-09 PhysicsGlobals: all 213 field rows (value text, numeric value, source layer) after level0 prefab + Physics.cfg | `harness/golden/physics_constants` |
| `capture.physics_drag_curve` | WP-09 PhysicsGlobals.DragCurveValue (managed float code, Mono rule) | in-game `harness/captures/out/aero_forces_mono.json` (table drag_curve_value) |
| `golden.body_catalog` | WP-09 Kerbol system at UT 0 with WP-07's run-time Orbit: bodies (all columns), orbits (all columns: elements, Init outputs, state at UT 0), 1 km atmosphere profiles, every body FloatCurve key | `harness/golden/body_catalog` |
| `golden.patched_conics` | WP-08 PatchedConics.CalculatePatch chains (encounter, escape, SOI transitions; 5 cases, patchLimit 6) on the harness's static bodies | `harness/golden/patched_conics` |
| `capture.orbit_rails` | WP-08 on-rails propagation through OrbitDriver in the rotating Planetarium frame, warp 1x-100,000x, 4 on-rails SOI transitions (BSP), TimeWarp warp rates (prefab table, WP-10) and TimeWarp.fixedDeltaTime | in-game `harness/captures/out2/orbit_rails_ingame.json` (C8) |
| `golden.atmosphere_curves` | WP-09 CelestialBody atmosphere + FlightIntegrator atmosphere helpers (pressure, temperature, density, speed of sound, Mach, solar attenuation, shock temperature, convection, pseudo-Reynolds) | `harness/golden/atmosphere_curves` |
| `capture.terrain_height` | WP-11 PQS.GetSurfaceHeight for Kerbin and Mun with the stock mod stacks (prefab data + the game's own height maps): 140,522 samples (1-degree grids, 0.01-degree KSC patch, the pad), public and overrideQuadBuildCheck variants, terrain altitude, per-scene probe heights, PQS radius/radiusMin/radiusMax | in-game `harness/captures/out2/terrain_height.json` |
| `capture.part_database` | WP-05 GameDatabase + PartLoader (data side) on the GameData mirror (`testdata/GameData-mirror`; env `KSP_GAMEDATA_ROOT` selects a real install; runner option `--gamedata`): parts, cfg values, modules, cfg-set module fields, propellants, resources, attach nodes, drag cubes, variants, asset paths; scope limits declared in `tests/CMakeLists.txt` | in-game `harness/captures/out2/part_database.json` (C9) |
| `capture.gamedatabase_load_order` | WP-05 GameDatabase load order: every `Load(Audio/Texture/Model): url` and `Config(TYPE) url` line | KSP.log of the C9 capture run (`decompiled/logs/`) |

All of them are bit-exact today (every numeric column max ULP 0, max abs 0).

### Which reference is truth

KSP runs on Unity's **Mono** (5.11, x64); the golden harness ran KSP's IL on **.NET Framework 4.8**. The in-game captures
(CAPTURE-SPEC C1/C2/C7, `harness/captures/README.md`) showed where they differ: Mono evaluates C# `float` expressions in
double and rounds only at stores, Mono's `Math.Tan`, `Ceiling(-0.5)`, `Atan2` zero sign, `Pow(-0, -3)`,
`(ulong)NaN` and `double.ToString()` differ from .NET. kspcore implements Mono (the game) by default; float32 columns
are therefore gated against the in-game capture files, double-precision datasets against the harness goldens
(runtime-independent). Tolerances for capture comparisons live in `tests/tolerances_capture.json` (currently 0 ULP
everywhere); the harness's `compare/tolerances.json` is used unchanged for harness goldens.

### The golden gate

`ctest` runs `cmake -P cmake/RunGolden.cmake` per gate:

1. `build/<Config>/tools/golden_runner/golden_runner.exe <dataset> --harness ..\harness --out build/<Config>/candidate`
   recomputes the dataset with kspcore and writes `candidate/<dataset>/<dataset>.json` in the harness's
   `ksp-golden/1` schema (same tables, keys and number text format as the golden file). Dataset-specific exact checks
   that are not tables (ConfigNode's saved bytes and parse trees) run inside the runner; any mismatch is a non-zero exit.
2. When a gate sets any filter option (below), `python -I tools/filter_golden.py` writes a filtered copy of the
   golden file into the build tree (`candidate/<dataset>/golden_<gate>.json`; harness/ is never modified).
3. `python -I ..\harness\compare\compare.py <golden> <candidate> --tol <tolerances> --report
   candidate/<dataset>/compare_report_<gate>.json`; any out-of-tolerance column, missing row/column or table fails.

The runner never reads golden *outputs* as inputs: inputs come from the same sources the harness used
(`harness/inputs/**`, `harness/golden/serialized/*.serialized.json` for Unity-serialized prefab data, the
`synthetic_inputs/` files, `harness/captures/in/*.tsv`, and the generators' literal input lists mirrored from
`harness/src/KspGolden/Generators/` and the capture code).

### Golden gate options

`ksp_golden_gate(<test name> <runner dataset> [options])` in `tests/CMakeLists.txt`; `cmake/RunGolden.cmake` passes
each filter option to the `tools/filter_golden.py` flag in brackets. One option set for every gate (WP-08's `ROWS`
became `KEEP_ROWS` when WP-05 was merged). Filters that remove golden data are allowed only for data outside the
package's scope (another package's code, objects that are not the experiment's inputs), each documented next to its
gate and in STATUS.md; never to hide a mismatch. A table or column named by a filter but absent from the golden file
fails the gate.

| option | meaning | gates using it |
|---|---|---|
| `GOLDEN <file>` | reference file (default `harness/golden/<dataset>/<dataset>.json`); captures: `${KSP_CAPTURES_DIR}` (`harness/captures/out`), `${KSP_CAPTURES2_DIR}` (`harness/captures/out2`) | all `capture.*`: math_shim.f32, libm_mono, unity_random, physics_drag_curve, orbit_rails, terrain_height, part_database |
| `TOL <file>` | tolerance file (default `harness/compare/tolerances.json`; captures use `${KSP_CAPTURE_TOL}` = `tests/tolerances_capture.json`, 0 ULP) | all `capture.*` (as above) |
| `TABLES t1,t2` [`--tables`] | compare only these tables | golden.math_shim.f64, capture.math_shim.f32, capture.unity_random, capture.physics_drag_curve, capture.terrain_height |
| `KEYS t:c1,c2/...` [`--key`] | set a table's key columns (capture tables written without a key; compare.py matches rows by key) | capture.orbit_rails (`soi_events:vessel,UT`) |
| `KEEP_ROWS rowspec` [`--keep-rows`] | keep only the rows matching rowspec (objects that are the experiment's inputs) | capture.orbit_rails (`soi_events:vessel^HC8`: the probes, not the asteroids) |
| `DROP_ROWS rowspec` [`--drop-rows`] | drop the rows matching rowspec (subject out of scope by a property of the golden row) | capture.part_database (the 5 prefab parts in every table; assets `kind=texture`) |
| `KEEP_ROWS_FILE <json>` [`--keep-rows-file`] | `{table: [[key values...], ...]}`: keep only the listed keys; a scope list the runner derives from its inputs (`<OUT>` = candidate dir) | capture.part_database (`<OUT>/part_database/scope.json`: the module fields each MODULE node sets) |
| `DROP_KEYS_FILE <json>` [`--drop-keys-file`] | `{table: [{"key": [...], "reason": "..."}, ...]}`: drop individually listed rows, each with its reason (`<OUT>` substituted too) | capture.part_database (`tests/data/part_database_exclusions.json`) |
| `RENUMBER t:col:group/...` [`--renumber`] | after the row filters, rewrite index column `col` as 0, 1, 2... within each value of `group` | capture.part_database (`assets:idx:part`) |
| `DROP_HEX` [`--drop-hex`] | drop the `<col>_hex` companion columns of in-game capture files (decimals are exact, so ULP compare is equivalent) | all `capture.*` (as above) |
| `COLUMNS t:c1,c2/...` [`--keep-columns`] | keep only the listed columns of the named tables (key columns must be listed) | capture.orbit_rails, capture.terrain_height |
| `DROP_COLUMNS t:c1,c2/...` [`--drop-columns`] | drop the listed columns (and their `_hex` companions) of the named tables | capture.part_database |

`rowspec` = `table:cond[+cond...][/table2:...]`; conditions on one table are OR'd. `col=v1|v2` matches when the
cell's text equals one of the values (strings as-is, null reads `null`, numbers/booleans as JSON text); `col^p1|p2`
matches when the cell is a string starting with one of the prefixes. Values cannot contain `/`, `+` or `|`.

Order inside `filter_golden.py`, per table: TABLES, KEYS, KEEP_ROWS then DROP_ROWS, KEEP_ROWS_FILE, DROP_KEYS_FILE
(both by the key after KEYS), RENUMBER, then the column filters DROP_HEX, COLUMNS, DROP_COLUMNS. Gates with no
filter option (golden.confignode_roundtrip, curves_eval, orbit_propagation, physics_constants, body_catalog,
atmosphere_curves, patched_conics) compare the harness file as is.

## Linking into Unreal (C ABI)

The UE 5.8 shell (`..\ue5\Plugins\KspCoreBridge`) links `build\RelWithDebInfo\kspcore.lib` and compiles against
`include\kspcore\kspcore_c.h` only (switch: `bUseRealCore = true` in `KspCoreBridge.Build.cs`; env `KSPCORE_ROOT` /
`KSPCORE_LIB` override the paths). Build it with `run_tests.ps1` (or `-BuildOnly`); the RelWithDebInfo preset writes the
library to exactly that path.

**C ABI** (`include/kspcore/kspcore_c.h`, `KSPCORE_ABI_VERSION 1`, implementation `src/capi/kspcore_c.cpp`): only C types
cross the boundary; every entry point catches all C++ exceptions and reports failures through return values plus
`kspcore_last_error()` (thread-local). Contents: `kspcore_build_info()` (contains `crt=MD ` and `idl=0`, which the
bridge's automation test checks), version/ABI queries, a simulation context (`kspcore_sim_create/destroy`), a log
callback, ConfigNode (load, parse, create, query values/children, typed getters, add/set, save, ToString) and FloatCurve
(create, load from a node, add keys, evaluate). Handles are opaque and owned by the caller; a ConfigNode handle keeps its
whole tree alive. Extend the ABI by appending functions and bumping `KSPCORE_ABI_VERSION` when anything changes
incompatibly (the bridge static_asserts the version).

**Build settings required by Unreal** (top-level `CMakeLists.txt`, every configuration):

| setting | value | why |
|---|---|---|
| CRT | `/MD` (`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL`), also in Debug | UE always uses the release CRT DLL; `/MDd` or `/MT` fails with LNK2038 |
| iterator debug level | `_ITERATOR_DEBUG_LEVEL=0` (explicit define) | LNK2038 otherwise |
| debug info | `/Z7` embedded (`CMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded`) | no PDB has to travel with the lib (LNK4099) |
| whole-program optimisation | off (no `/GL`, no LTCG) | `/GL` objects are toolset-version locked |
| language / toolset | C++20, MSVC 14.44 x64 | same family as UE 5.8 |
| floating point | `/fp:precise` on kspcore's own objects | bit-exact float32 and the Mono float rule |

The lib's linker directives confirm it: `/FAILIFMISMATCH:"RuntimeLibrary=MD_DynamicRelease"`,
`"_ITERATOR_DEBUG_LEVEL=0"`, no LTCG objects.

**Floating point and flag isolation.** kspcore's compiler options are attached to its own targets with
`ksp_set_build_flags()` as PRIVATE options, so they never propagate to consumers, and a consumer's options (UE's Game
target uses `/fp:fast`) never reach kspcore's objects: the numerics are compiled once, inside kspcore.lib. The C header
has no inline code, so including it from a `/fp:fast` translation unit cannot change any result (`capi.ue_like` proves it
bit-exactly). Do not let Unreal translation units include `include/ksp/**` C++ headers: they contain inline float code
(Mathf, Vector3f operators) that would be recompiled with UE's flags, and inline functions compiled under different
flags in different translation units violate the one-definition rule. `/fp:strict` is not needed: `/fp:precise` on
MSVC x64 already performs float32 operations in float32, double in double, honours every `(float)` store and does not
contract into FMA.

## Layout

| path | contents |
|---|---|
| `include/ksp/dotnet/` | .NET/Mono runtime semantics KSP depends on: `number.hpp` (Mono/CLR number parsing incl. the runtime's NumberToDouble; `G`/`F`/`R` formatting per runtime), `math.hpp` (System.Math with x87 Sin/Cos, VS2013-CRT-backed functions, Mono/.NET differences, C# numeric conversions), `mono_float.hpp` (Mono's float32 evaluation rule: `fx()` / `f32store()`), `random.hpp` (System.Random, KSPRandom), `text.hpp` (Trim/Split, UTF-8, File.ReadAllLines decoding), `utf16.hpp` (UTF-16 views, char.IsUpper/ToUpper), `collections.hpp` (List.Sort introsort, ordered Dictionary), `enum.hpp` (Enum.ToString/GetNames/Parse over declared metadata); `math.hpp` also has Math.Min/Max with .NET NaN/signed-zero rules |
| `include/ksp/unity/` | Unity value types and maths: `mathf.hpp`, `vector.hpp` (Vector2f/3f/4f), `quaternion.hpp` (Quaternionf), `color.hpp` (Color, Color32, Rect, Matrix4x4f), `animation_curve.hpp` (Keyframe, AnimationCurve: port of the native AnimationCurveTpl<float>), `random.hpp` (UnityEngine.Random, Color.HSVToRGB) |
| `include/ksp/math/` | KSP double maths: `vector3d.hpp` (Vector2d/3d/4d), `quaterniond.hpp`, `util_math.hpp` (UtilMath, Planetarium.CelestialFrame) |
| `include/ksp/curves/` | `float_curve.hpp` (FloatCurve) |
| `include/ksp/config/` | `config_node.hpp` (ConfigNode, Guid), `parse_extensions.hpp` (ParseExtensions, ColorUtility.TryParseHtmlString) |
| `include/ksp/core/` | `log.hpp` (Debug.Log replacement with an installable sink), `exceptions.hpp` (C# exception types), `delegate.hpp` (C# delegates: equality, Target, multicast; `Callback<...>`), `unity_time.hpp` (UnityEngine.Time values the host advances) |
| `include/ksp/orbit/` | WP-07: `orbit.hpp` (Orbit), `targeting.hpp` (Targeting: closest-approach search), `orbit_snapshot.hpp`, `orbit_util.hpp`, `targeting_modes.hpp`. WP-08: `patched_conics.hpp` (PatchedConics), `patched_conic_solver.hpp` (PatchedConicSolver), `maneuver_node.hpp` (ManeuverNode, ManeuverGizmoState), `trajectory.hpp` (Trajectory), `orbit_driver.hpp` (OrbitDriver, IOrbitDriverVessel), `time_warp.hpp` (TimeWarp, TimeWarpEnvironment, ClearToSaveStatus; WP-10). WP-10: `warp_transition.hpp` (WarpTransition, WarpTransitionCalculator), `targetable.hpp` (ITargetable), `position_target.hpp` (PositionTarget); `include/ksp/unity/transform.hpp` (Transform stand-in). Reference bodies are `ksp::CelestialBody`, frames come from `ksp::Planetarium`; `BodyOrbit(body)` is a body's own Orbit |
| `include/ksp/unity/quaternion_native.hpp` | native Unity functions ported bit-exact against in-game captures (Quaternion.AngleAxis; Quaternion.FromToRotation, WP-11) |
| `include/ksp/events/` | WP-03 event bus: `event_data.hpp` (BaseGameEvent, EventVoid, EventData, EventDataModifier, Event*Max/Min, EventValueComparison/Operation, EventValueWrapper), `game_events.hpp` + `game_event_types.hpp` (the whole GameEvents registry, generated by `tools/wp03/gen_game_events.py`) |
| `include/ksp/fsm/` | `kerbal_fsm.hpp` (KerbalFSM, KFSMState, KFSMEvent, KFSMTimedEvent) |
| `include/ksp/localization/` | `localizer.hpp` (KSP.Localization.Localizer tag table/Format, Lingoona managed fallback) |
| `include/ksp/util/` | `ksp_util.hpp` (KSPUtil, IDateTimeFormatter, DefaultDateTimeFormatter, GameSettings/Versioning shims), `collections.hpp` (ArrayExtensions, ListExtension, BiDictionaryOneToOne, DictionaryValueList, ListDictionary), `enum_extensions.hpp`, `simplex.hpp` (Simplex, SimplexNoise), `misc_util.hpp` (StringExtensions, StringBuilderCache.Format, MinMaxColor, IntersectInformation) |
| `include/kspcore/kspcore_c.h` | the C ABI used by the Unreal shell (implementation `src/capi/kspcore_c.cpp`) |
| `include/ksp/curves/double_curve.hpp`, `include/ksp/math/{matrix4x4d,mathfx}.hpp` | DoubleCurve, Vector3/4Curve, Matrix4x4D, Mathfx, VectorExtensions, QuaternionExtensions, MinMaxFloat, Vect, PIDclamp |
| `include/ksp/physics/` | WP-09 `physics_globals.hpp` (PhysicsGlobals: fields, static properties, LoadDatabase/SaveDatabase, lifting-surface curves, DragCurveValue; the `*.inc` files are generated), `vessel_ranges.hpp` (VesselRanges, Vessel.Situations) |
| `include/ksp/celestial/` | WP-09 `celestial_body.hpp` (CelestialBody, CBAttributeMapSO, MiniBiome, GameAssetRef, orbit elements + `ICelestialBodyOrbit` link to the run-time Orbit), `celestial_body_orbit.hpp` (`CelestialBodyOrbitAdapter`: OrbitDriver for bodies over ksp::Orbit), `planetarium.hpp` (Planetarium, the only one: also Orbit's Zup/up/right/ZupAtT), `celestial_system.hpp` (PSystem prefab, `KerbolSystemPrefab()`, spawn, FlightGlobals body list), `flight_globals.hpp` (WP-08: FlightGlobals.getMainBody/overrideOrbit subset), `flight_atmosphere.hpp` (FlightIntegrator atmosphere helpers) |
| `include/ksp/io/` | WP-05 `url_dir.hpp` (UrlDir, UrlFile, UrlConfig, UrlIdentifier: the GameData tree) |
| `include/ksp/database/` | WP-05 `game_database.hpp` (GameDatabase data side: configs, texture/model/audio metadata tables), `game_data.hpp` (`ksp::GameData`: the whole loading sequence on a KSP install; GameData root = `<gameRoot>/GameData`, plus `<gameRoot>/Physics.cfg` and `<gameRoot>/PartDatabase.cfg`) |
| `include/ksp/parts/` | WP-05 `part_loader.hpp` (PartLoader data side), `available_part.hpp` (AvailablePart, PartPrefab, PartVariant, model/asset refs), `part_data.hpp` (AttachRules, Part field table, AttachNode, DragCube/List, PartResource), `part_module_data.hpp` (PartModule class table, MODULE records, Propellant), `ksp_enums.hpp`; generated `part_fields.inc`, `compound_part_fields.inc`, `part_enums.inc`, `module_schema.inc` |
| `include/ksp/resources/` | WP-05 `part_resource_library.hpp` (PartResourceDefinition/List/Library), `physic_material_library.hpp` |
| `include/ksp/curves/serialized_float_curve.hpp` | FloatCurves built from Unity-serialized keyframes (prefab data) |
| `include/ksp/terrain/` | WP-11 terrain height: `pqs.hpp` (PQS height path: SetupMods, SetupBuildDelegates radii, GetSurfaceHeight, VertexBuildData, PQSMod base), `pqs_mods.hpp` (all 28 PQSMods that change height + PQSModPassive), `map_so.hpp` (MapSO sampling/decoding), `libnoise.hpp` (KSP's LibNoise subset), `pqs_prefab.hpp` (stock PQS factory, MapLibrary, `GetRelSurfaceNVector`, `TerrainAltitude`, `SurfaceHeightOverride`) |
| `data/terrain/` | WP-11 original terrain data from `decompiled/KSP-dev/KSP_x64_Data`: `maps/<name>.<PathID>.bin` (16 MapSO._data arrays, 55 MB: every height map/decal a height mod references), `manifest.json` (origin asset file + PathID, header, sha256 per map), `pqs_prefab.json` (every PQSMod of every body in GetChildMods order with all serialized fields) |
| `tools/wp11/` | `extract_terrain.py` (UnityPy extraction of the above; run with `harness/tools-cache/venv`), `gen_wp11_terrain.py` (generates `src/terrain/pqs_prefab_data.cpp` and `libnoise_random_vectors.inc`; regenerate, never hand-edit), `libnoise_reference.py` (independent Python LibNoise transcription for test expectations) |
| `include/ksp/save/` | WP-04 save model: `game.hpp` (Game, FlightState, ProtoScenarioModule, ScenarioModule data side, GameBackup), `game_parameters.hpp` (GameParameters + every stock parameter node, presets), `proto_vessel.hpp` (ProtoVessel, ProtoPartSnapshot, module / resource snapshots, AttachNodeSnapshot, VesselNaming, target / waypoint info), `crew.hpp` (FlightLog, ProtoCrewMember, KerbalRoster, experience configs), `ship_construct.hpp` (.craft file form), `upgrade_pipeline.hpp` (SaveUpgradePipeline + 40 stock scripts, KSPUpgradePipeline), `game_persistence.hpp` (GamePersistence, quicksave naming, KSPScenarioType), `save_environment.hpp` (the GameDatabase / ExpansionsLoader / HighLogic / FlightGlobals-id / GameSettings / PartLoader state the save model reads, one shim), `save_enums.hpp` |
| `include/ksp/dotnet/version.hpp`, `enum_parse.hpp` | System.Version; Enum.Parse with numbers, flags and whitespace |
| `tools/save_corpus/` | the `corpus.saves` gate (round trip + documented-difference classifier) |
| `tools/wp04/` | `gen_upgrade_scripts.py` (-> `src/save/upgrade_scripts_data.inc` from the decompiled SaveUpgradePipeline scripts), `gen_scenario_types.ps1` (-> `src/save/scenario_types_data.inc`, [KSPScenario] types of the shipped DLL via Mono.Cecil); regenerate, never hand-edit |
| `data/kerbol_body_assets.json` | index of the game's own body assets (biome maps with attribute tables, mini-biomes, PQS height/colour maps, scaled-space meshes/materials/textures) by asset file + PathID, from `decompiled/KSP-dev/KSP_x64_Data` (`tools/extract_body_assets.py`) |
| `tools/gen_wp09_data.py` | generates `physics_globals_*.inc` and `src/celestial/kerbol_system_data.cpp` from the decompiled C# (field lists, LoadDatabase/SaveDatabase order) and the game's serialized prefab data; regenerate, never hand-edit |
| `tools/extract_body_assets.py` | read-only UnityPy indexer of the body assets (run with `harness/tools-cache/venv`) |
| `src/` | implementations, same tree |
| `tests/capi/` | C ABI consumers (pure C; C++ with Unreal's flags) |
| `tests/` | doctest unit tests; `tests/data/netfx_reference.tsv` and `tests/data/wp03_reference.tsv` reference data; `tests/tolerances_capture.json` tolerances for in-game capture gates; `tests/CMakeLists.txt` declares every gate (`ksp_golden_gate`) |
| `tools/golden_runner/` | the golden runner (one `ds_<dataset>.cpp` per harness dataset) |
| `tools/netfx_reference/` | `gen_netfx_reference.ps1`, regenerates `tests/data/netfx_reference.tsv` |
| `tools/wp03_reference/` | `gen_wp03_reference.ps1` + `Wp03Ref.cs`, regenerates `tests/data/wp03_reference.tsv` from KSP's IL |
| `tools/wp03/` | `gen_game_events.py` (+ `cs_types.py`): regenerates the GameEvents registry from `GameEvents.cs` |
| `tools/filter_golden.py` | writes a table/column-filtered / `_hex`-stripped copy of a golden or capture file for compare.py |
| `tools/wp05/` | `gen_part_fields.py` and `gen_module_schema.py` (regenerate the `include/ksp/parts/*.inc` tables from the decompiled C#; never hand-edit), `check_load_log.py` (load-order gate), `estimate_module_defaults.py` (WP-06 diagnostic) |
| `tools/libm/`, `tools/libm_check/` | WP-00L: `gen_main.py` (+ `gen_lift.py`, `lift_discover.py`) lifts the x64 code of `msvcr120_clr0400.dll` / `ucrtbase.dll` into `src/dotnet/{vs2013,ucrt}_libm_gen.cpp`; `pe_tool.py`, `callgraph.py`, `census.py` research helpers; `libm_check` compares the result with the DLLs |
| `tools/mk_gamedata_mirror.py` | WP-00L: builds `testdata/GameData-mirror/` from a KSP install (`--verify` checks the manifest) |
| `testdata/` | WP-00L: `GameData-mirror/` (game root the tests read; `MANIFEST.json`), `logs/` (KSP.log of the C9 capture run) |
| `run_tests.sh` | the Linux one-command build + test (mirror of `run_tests.ps1`) |
| `cmake/RunGolden.cmake` | the per-dataset gate |
| `third_party/` | doctest 2.4.12, nlohmann/json 3.12.0 (pinned, see `third_party/VERSIONS.md`); used by tests/tools only |

## Conventions

- **Faithful port first.** Translate the decompiled C# operation for operation, including its quirks and bugs (they are
  marked `sic` or "KSP quirk" in comments). Each ported function carries a comment citing the C# source, e.g.
  `// ConfigNode.cs:3884 PreFormatConfig`; paths are relative to `decompiled/KSP-Decompiled/Assembly-CSharp/` unless
  they start with `Unity/`, `Mono/` or `harness/`. Performance work comes later and must keep the goldens green.
- **Precision.** `double` for world/orbital state (KSP's `Vector3d`, `QuaternionD`, `Orbit`...). `float` exactly where
  KSP/Unity use `float` (`Mathf`, `Vector3f`, `Quaternionf`, `FloatCurve`, most part/aero maths).
  Compiler flags: `/fp:precise`, x64 SSE2, no FMA contraction (never `/fp:fast` or `/arch:AVX2` on kspcore).
- **Mono float32 rule (managed float code).** Unity's Mono evaluates C# `float` expressions in double and rounds to
  float only where the source stores, passes, returns or casts (`include/ksp/dotnet/mono_float.hpp`). Port every managed
  float path as `f32store(<C# expression written in double with fx(operand)>)` at exactly those points, e.g.
  `return a + (b - a) * Clamp01(t);` -> `return f32store(fx(a) + (fx(b) - fx(a)) * fx(Clamp01(t)));`. Comparisons of
  float expressions happen in double. Native Unity code (AnimationCurve, Random, native Quaternion functions) is plain
  per-operation float32 and must not use the pattern. Double code is unaffected.
- **Runtime semantics live in `ksp::dotnet`.** Use `dotnet::ParseSingle/TryParseDouble/...` instead of `strtod`,
  `dotnet::FormatSingleG/FormatDoubleG/...F/...R` instead of `printf`, `dotnet::Math::*` instead of `<cmath>` wherever C#
  calls System.Math (Sin/Cos are x87-emulated, Tan/Exp/Pow/... come from the VS2013 CRT), `dotnet::Trim/Split` for
  string ops, `dotnet::ConvToInt32/ConvToUInt64/...` for C# numeric casts.
- **Runtime selection.** `dotnet::SetRuntime(Runtime::Mono | Runtime::NetFramework)`; Mono (the game) is the default,
  NetFramework reproduces the harness oracle (used by the .NET reference unit test). `dotnet::LibmBackend()` reports
  whether the VS2013 CRT (`msvcr120_clr0400.dll`, shipped with .NET Framework 4.x on every Windows 10/11, or
  `msvcr120.dll`) was loaded; without it kspcore falls back to `<cmath>` and can differ in the last bit.
- **Unity value-type equality**: `operator==` on `Vector3f`/`Vector2f`/`Vector4f`/`Quaternionf` is Unity's approximate
  test; use `.Equals()` for exact comparison.
- **Errors**: where C# throws, kspcore throws (`dotnet::FormatException`, `OverflowException`, `ksp::NullReferenceException`,
  `std::out_of_range` for IndexOutOfRange). Where C# logs and continues, kspcore calls `ksp::log::Warning/Error`.
- **Nullable strings**: a C# `string` that can be null is `std::optional<std::string>` (getters) or `const std::string*`
  (`GetValuePtr`, no copy).
- **Reference semantics**: `ConfigNode` children are `std::shared_ptr<ConfigNode>`; `GetNode` returns a non-owning
  pointer into the tree; `ConfigNode` itself is move-only (use `CreateCopy()` for a deep copy).
- **Bodies and orbits (WP-07 + WP-09).** `CelestialSystem::Spawn(KerbolSystemPrefab(), planetarium)` (an Awake
  `Planetarium`) spawns the 17 bodies and attaches each body's run-time `ksp::Orbit` (`CelestialBodyOrbitAdapter`, the
  default `OrbitFactory`); then `body->getPositionAtUT(UT)` (world position: orbit relative position `.xzy` + parent
  position), `getTruePositionAtUT(UT)` (chained through every parent), `GetFrameVelAtUT(UT)`, `BodyOrbit(body)` (the
  ksp::Orbit, `nullptr` for the Sun) and `Planetarium::FixedUpdate()` (advance time, CBUpdate every body, move them)
  work as in KSP. `Orbit::referenceBody` is a `CelestialBody*`. Every body's run-time orbit is driven by a real
  `ksp::OrbitDriver` (inside `CelestialBodyOrbitAdapter`), registered in `Planetarium::Orbits()` in spawn order (WP-08). There is one `Planetarium` (static `Zup`, `Rotation()`,
  `up()/right()`, `ZupAtT`): Orbit works without an instance (Awake(0)'s frame, see planetarium.hpp); the stock prefab's
  Mun/Minmus `inverseRotation = 1` makes the first CBUpdate pass turn `Zup` (InverseRotAngle 230), as KSP's code does.
- **Terrain (WP-11).** `terrain::MapLibrary maps("<core>/data/terrain"); auto pqs = terrain::LoadStockPQS("Kerbin",
  maps);` then `pqs->GetSurfaceHeight(terrain::GetRelSurfaceNVector(lat, lon))` (radius + terrain height, public
  variant) or `(dir, true)` (overrideQuadBuildCheck, vessel positioning) and `terrain::TerrainAltitude(pqs.get(), lat,
  lon, allowNegative)`. A PQS keeps KSP's mutable evaluation state (shared VertexBuildData, mod scratch fields,
  quad-dependent FlattenArea/MapDecal state driven by `Mod_OnQuadPreBuild/Mod_OnQuadBuilt`): one instance per thread.
  Building a PQS constructs Simplex objects, which reseed UnityEngine.Random and rewrite Simplex's static table, as
  in KSP.
- **Thread safety**: everything is single-threaded like KSP unless stated. `AnimationCurve::Evaluate` is const and
  stateless (thread-safe) for strictly increasing key times; `dotnet::SetRuntime`, `log::SetSink` and
  `UnityRandom::Global()` are process-wide.
- **Save model (WP-04).** Load = the C# constructor over a ConfigNode (`Game(root)`, `ProtoVessel(node, game)`, ...),
  Save = the C# Save method; KSP's quirks are kept (see STATUS.md WP-04). Everything the save code reads from other
  systems (GameDatabase envInfo / loaderInfo / modded, installed DLC, HighLogic.CurrentGame, the FlightGlobals
  persistentId registry, GameSettings, PartLoader queries, Guid / TickCount randomness) is in
  `SaveEnvironment::Current()`. A host must call `GameParameters::GetDefaultParameters(...)` (as MainMenu.Start does)
  before loading saves: the first GameParameters ever constructed has a different custom-parameter order.
- Namespaces: `ksp` (game types), `ksp::Mathf`, `ksp::UtilMath`, `ksp::KSPUtil`, `ksp::ParseExtensions`, `ksp::dotnet`,
  `ksp::events` (event bus; `ksp::GameEvents` is an alias), `ksp::util` (KSP's extension-method classes). C# namespaces
  map to nested namespaces under `ksp` (`KSP.Localization` -> `ksp::KSP::Localization`, `Contracts` -> `ksp::Contracts`).
- **C# delegates** are `ksp::Delegate<R(Args...)>` (`ksp/core/delegate.hpp`): `Bind(obj, &T::Method)` for method groups
  (equal when target and method are equal, so `Remove` works like C#), `Lambda(owner, fn)` for closures, `Static(fn)`.
  Objects must remove their listeners before they die (C# keeps destroyed objects alive; C++ does not).
- **Enums that KSP formats or parses** declare `ksp::dotnet::EnumTraits<E>` (declaration-order member table with optional
  `[Description]`); `dotnet::EnumToString` reproduces Enum.ToString including [Flags] and duplicate values.
- **Collections whose order shows**: `dotnet::Dictionary` (.NET enumeration order with free-slot reuse) and
  `dotnet::ListSort` (List.Sort's unstable introsort), `ksp/dotnet/collections.hpp`.
- Types keep their KSP names; Unity float types get an `f` suffix (`Vector3f`, `Quaternionf`) to sit next to KSP's
  `Vector3d`.

## Adding a work package with a golden gate

1. Port the C# into `include/ksp/<area>/` + `src/<area>/` and add the `.cpp` files to `src/CMakeLists.txt`. Follow the
   Mono float rule for managed float code and use `ksp::dotnet` for every runtime call.
2. Read the dataset's generator in `harness/src/KspGolden/Generators/` (or the capture code for in-game data) and its
   `README.md`.
3. Add `tools/golden_runner/ds_<dataset>.cpp`: rebuild the generator's inputs (never from the golden outputs), compute
   every column with kspcore and write the tables with `golden::File`/`golden::Table` (`golden_json.hpp`), then
   `return WriteCandidate(opt, g, "<dataset>/<dataset>.json");`. Register it at the end of the file with
   `KSP_GOLDEN_DATASET("<dataset>", <order>, RunMyDataset);` and add the file to `tools/golden_runner/CMakeLists.txt`.
4. Declare the gate in `tests/CMakeLists.txt`:
   `ksp_golden_gate(golden.<dataset> <dataset>)` for a harness golden, or
   `ksp_golden_gate(capture.<dataset> <dataset> GOLDEN ${KSP_CAPTURES_DIR}/<file>.json DROP_HEX TOL ${KSP_CAPTURE_TOL}
   [TABLES t1,t2])` for an in-game capture (batch 2 files: `${KSP_CAPTURES2_DIR}`). Float32 managed paths are judged
   against captures where they exist. Row/column scope filters (KEYS, KEEP_ROWS, DROP_ROWS, KEEP_ROWS_FILE,
   DROP_KEYS_FILE, RENUMBER, COLUMNS, DROP_COLUMNS): see "Golden gate options"; only for data another package's code
   produces or objects outside the experiment's inputs (document each in tests/CMakeLists.txt and STATUS.md), never to
   hide a mismatch.
   Candidate numbers are written like the harness (`.0` on integral values): compare.py matches row keys by their JSON
   text, so a key column written as `100` would not match the golden's `100.0`.
5. Run `run_tests.ps1`; inspect `build/<Config>/candidate/<dataset>/compare_report_<gate>.json` for per-column
   max abs/rel/ULP.
6. Unit-test API behaviour the golden does not cover in `tests/test_<area>.cpp`.
7. Commit the package on its own (`git commit` in `core/` only; local repository, never pushed) and update STATUS.md.

Tolerances for harness goldens are owned by the harness (`harness/compare/tolerances.json`); never loosen them from
here. Capture tolerances are in `tests/tolerances_capture.json`: float32 columns may go up to 64 ULP / rel 5e-6 if
bit-exactness is out of reach, and every such relaxation must be listed in STATUS.md.

## Regenerating the .NET reference data

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\core\tools\netfx_reference\gen_netfx_reference.ps1
```

Deterministic (fixed seed); needs Windows PowerShell 5.1 (CLR 4.0.30319, i.e. .NET Framework 4.8, the same runtime the
harness ran KSP's IL on).

WP-03 reference data (KSP's own IL from the golden harness's build, run on .NET Framework 4.8). Reads
`..\harness\build\KspGolden\*.dll` and `..\decompiled\KSP-dev\GameData\Squad\Localization\dictionary.cfg`, writes only
`build\wp03_reference\` and `tests\data\wp03_reference.tsv`; compiles with the .NET Framework's own `csc.exe` (no SDK):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\core\tools\wp03_reference\gen_wp03_reference.ps1
```

The GameEvents registry is generated from the decompiled source: `python -I tools\wp03\gen_game_events.py .` (run in
`core\`).
