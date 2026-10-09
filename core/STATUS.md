# kspcore status

Updated 2026-10-09 (WP-05). One command: `powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\core\run_tests.ps1`
(details in README.md). Local git repository in `core/` only, never pushed.

## Summary

| deliverable | state |
|---|---|
| 1. Scaffold | done: CMake 3.28+/C++20/MSVC 14.44 presets (Debug, RelWithDebInfo; Ninja), `run_tests.ps1`, vendored doctest 2.4.12 + nlohmann/json 3.12.0 (sha256 in `third_party/VERSIONS.md`), golden gate (`golden_runner` + `harness/compare/compare.py`) as ctest tests, Unreal link settings (/MD, IDL 0, /Z7, no /GL) with `build/RelWithDebInfo/kspcore.lib`, C ABI `include/kspcore/kspcore_c.h` |
| 2. WP-01 maths + Unity shims + FloatCurve | done, all 19 package files (except the native-only Unity functions listed below); every gate bit-exact |
| 3. WP-02 ConfigNode | done: Load/Parse/Save/ToString, value/node API, typed getters/setters, ParseExtensions, Guid, KSPParseable, type parsers; byte-exact on the whole corpus. UrlDir (GameData tree) deferred, see below |
| extra | in-game (Mono) runtime semantics from the CAPTURE-SPEC C2/C7 captures: System.Math, float32 evaluation rule, conversions, formatting, System.Random, UnityEngine.Random, KSPRandom |
| 4. WP-09 celestial bodies, atmospheres, PhysicsGlobals (branch `wp09`, merged) | done for the assigned scope: PhysicsGlobals (all fields/curves/properties, LoadDatabase/SaveDatabase), CelestialBody (data model, Awake/Start/CBUpdate/SetupConstants, atmosphere, frames, lat/lon), Planetarium, PSystem spawn + FlightGlobals list, Kerbol catalog of 17 bodies from the game's prefab + asset index, FlightIntegrator atmosphere helpers; 4 new gates bit-exact. Pending WP-07 integration and deferred items: see "WP-09" below |
| 5. WP-07 Orbit (branch `wp07`, merged) | done: Orbit.cs (everything but DrawOrbit), Targeting.cs (closest-approach intercept search behind `Orbit.FindClosestPoints`), OrbitSnapshot, OrbitUtil (but CalculateIntersections), FoRModes/VesselTargetModes; `CelestialBodyRef` / `PlanetariumFrames` interfaces for WP-09; golden `orbit_propagation` bit-exact (all 5 tables). TransferMath, DirectionTarget, ITargetable deferred (see "WP-07 Orbit") |
| 6. Integration WP-07/WP-09 (main) | done: Orbit on `ksp::CelestialBody` (CelestialBodyRef deleted), one Planetarium (PlanetariumFrames deleted), `CelestialBodyOrbitAdapter` (OrbitDriver for bodies over ksp::Orbit) attached by `CelestialSystem::Spawn` by default, `body_catalog/orbits` gated in full, integration unit test. See "Integration WP-07/WP-09" below |
| 8. WP-08 patched conics, SOI transitions, maneuver nodes (branch `wp08`) | done: PatchedConics, PatchedConicSolver, ManeuverNode, Trajectory, OrbitDriver (minus rendering; vessel through `IOrbitDriverVessel`), Planetarium.Orbits; TimeWarp/FlightGlobals/GameSettings subsets; `golden.patched_conics` and the in-game C8 capture `capture.orbit_rails` bit-exact. See "WP-08" below |
| 7. WP-03 event bus, FSM, utilities (branch `wp03`, merged) | done: typed event bus + full GameEvents registry (469 events), KerbalFSM, KSPUtil engine-independent parts, Localizer + Lingoona fallback, UtilMath completed, .NET collection/enum semantics, Simplex; gated by unit tests incl. 6,437 values from KSP's IL (no golden dataset), see below |
| 9. WP-11 terrain height maths (branch `wp11`) | done: PQS height evaluation (SetupMods/List.Sort, radii, GetSurfaceHeight both variants), all 28 height PQSMods, MapSO, KSP's LibNoise subset, native Quaternion.FromToRotation, stock mod stacks for all 15 PQS bodies generated from the game's prefabs, the game's 16 height maps/decals extracted with a manifest; capture gate bit-exact on 140,522 Kerbin/Mun samples. See "WP-11" below |
| 11. WP-04 save model + upgrade pipeline (branch `wp04`) | done (data side): Game, GameParameters (all nodes, presets), FlightState, Proto* snapshots, ProtoCrewMember, KerbalRoster, ScenarioModule data, craft file form, SaveUpgradePipeline (framework + all 40 stock scripts), GamePersistence; gated by `corpus.saves` (314 original saves / crafts: R2 idempotent 314, R1 byte-identical 50 + 87 after removing mod-owned content, every other difference documented, 0 unexplained) and unit tests; no golden dataset. See "WP-04" |

| 10. WP-05 UrlDir, GameDatabase (data), PartLoader (data), resource library (branch `wp05`, merged in `bfb732f`) | done for the data side: UrlDir + GameDatabase load order identical to KSP.log (2,836 lines), PartLoader compiles all 353 GameData parts + the 9 prefab-part slots; `capture.part_database` gate bit-exact on every in-scope C9 row (parts, cfg values, modules, cfg-set module fields, propellants, resources, attach nodes, drag cubes, variants, asset paths). Module instantiation is WP-06, model contents the UE import. See "WP-05" below |

| 12. WP-10 time warp, rails propagation, orbit driver (branch `wp10`) | done: TimeWarp (whole class incl. rate limits, mode switching, autoWarpTo), WarpTransition(Calculator), PositionTarget + ITargetable, OrbitDriver finished (Targetable, TRACK_Phys/IDLE/UPDATE tests), CelestialBody.resetTimeWarpLimits/TerrainAltitude; `capture.orbit_rails` bit-exact incl. the newly gated `segments.requested_rate`; 17 unit cases (2,009 assertions); numerics beyond C8 need new captures (listed in "WP-10") |

| 13. WP-18 craft files and ship construction (branch `wp18`) | done for the engine-agnostic part: ShipConstruct over live `Part`s (`LoadShipWithParts` / `SaveShip` from the parts, costs, masses, naming, ship PartSet), ShipTemplate (whole class), ShipConstruction (paths, backups, load / save, `AssembleForLaunch` over WP-14's assembly, templates through the upgrade pipeline, `GetPartCosts*`, launch-site queries, craft size); new gate `capture.ship_construct` (C5: `GetShipMass` and the per-part masses of 17 crafts, 0 ULP); 26 unit cases (274,042 assertions): round trip of 57 crafts, agreement with the original files, templates vs ships. See "WP-18" |

## Test results (all 24 ctest tests pass: Linux Clang/GCC and Windows MSVC in CI; WP-00L added libm.portable_vs_dll, WP-14 capture.vessel_state, WP-15 capture.resource_mass, WP-18 capture.ship_construct)

Every numeric column of every gate: **max ULP 0, max abs 0** (bit-exact, and the same decimal text). Tolerance files:
harness goldens use `harness/compare/tolerances.json` unchanged; in-game capture gates use `tests/tolerances_capture.json`
(0 ULP everywhere; no relaxation was needed).

| ctest | reference | rows compared | result |
|---|---|---|---|
| `golden.math_shim.f64` | harness golden `math_shim` table f64 (Vector3d, QuaternionD, UtilMath, CelestialFrame, Zup) | 532 x 10 cols | 0 ULP |
| `capture.math_shim.f32` | in-game `captures/out/math_shim_mono.json` table f32 (Mathf, Vector3, Quaternion under Mono) | 903 x 10 cols | 0 ULP |
| `capture.libm_mono` | in-game `captures/out/libm_mono.json`: f64 (25 System.Math fns x 3,093 inputs), f32 (15 x 3,093), f32ops (100,000), f32ops_wide (80,000), conv (390), iconv (72), tostring (4,200), parse (296), sysrand (288) | 312,066 | 0 ULP, all strings identical |
| `capture.unity_random` | in-game `captures/out/unity_random.json`: seeds, states, value, range_f, range_i, vec, edge, trace, trace_int, trace_vec, ksp_random | 8,676 | 0 ULP, all ints identical |
| `golden.confignode_roundtrip` | harness golden: corpus (47) + save_text (21); runner: 47/47 saved files byte-identical, 45/45 parse trees identical | 68 + 47 files + 45 trees | exact |
| `golden.curves_eval` | harness golden (== native Unity per C1): curves 366, keys 1,055, samples 28,484 | 29,905 | 0 ULP |
| `golden.orbit_propagation` | WP-07 harness golden `orbit_propagation` (KSP's Orbit on .NET 4.8): cases 92, state_at_ut 1,020, from_state_vectors 1,020, state_vector_cases 9, kepler 281; every row matched, 46,977 f64 values | 2,422 rows | 0 ULP |
| `corpus.gamedata` | self-consistency: 368 GameData `.cfg` + 86 `.craft` (8.55 MB, 241,219 values, 77,117 nodes): all load, Save->Load->Save byte-identical, Parse == Load, 226 FloatCurve nodes load | 454 files | 0 failures |
| `capi.c`, `capi.ue_like` | C ABI from pure C and from C++ with UE flags (/GR- /EHs-c- _HAS_EXCEPTIONS=0 /fp:fast); FloatCurve values bit-exact vs golden through the ABI | - | pass |
| `golden.physics_constants` | harness golden `physics_constants` (WP-09): every PhysicsGlobals field, exact value text + source layer | 213 | exact (0 ULP, all strings identical) |
| `capture.physics_drag_curve` | in-game `captures/out/aero_forces_mono.json` table drag_curve_value: PhysicsGlobals.DragCurveValue under Mono | 2,870 | 0 ULP |
| `golden.body_catalog` | harness golden `body_catalog` (WP-09 + WP-07 Orbit): bodies 17 x 51 cols, orbits 16 x 26 cols (all: elements, Init outputs, state at UT 0), atmosphere_profile_1km 1,072, body_curves 265 | 1,370 | 0 ULP |
| `golden.atmosphere_curves` | harness golden `atmosphere_curves` (WP-09): profile_100m 2,704, flight_state 2,176 x 19 cols, solar_effects 320 | 5,200 | 0 ULP |
| `golden.patched_conics` | WP-08 harness golden `patched_conics` (PatchedConics.CalculatePatch chains, 5 cases) | 11 | 0 ULP |
| `capture.orbit_rails` | WP-08 in-game `captures/out2/orbit_rails_ingame.json` (C8): initial 6, samples 3,465, frames 450, segments 5, soi_events 4 (probes); world_rel/packed/loaded/inputs excluded (see WP-08) | 3,930 | 0 ULP |
| `capture.terrain_height` | in-game `captures/out2/terrain_height.json` (WP-11): samples 140,522 x (h_surface, h_override, terrain_alt), bodies 2 (radii), scene_probe 4 | 140,528 | 0 ULP, no exclusions |
| `capture.part_database` | WP-05 + WP-06: in-game `captures/out2/part_database.json` (C9), scope in "WP-05" and "WP-06" below: parts 357 (module_mass / module_cost included), part_cfg_values 6,235, modules 1,312, module_fields 32,894, module_propellants 77, resources 166, attach_nodes 822, drag_cubes 531, variants 228, assets 743 | 43,365 | 0 ULP, all strings identical |
| `capture.resource_mass` | WP-15: in-game `captures/out2/deltav_readout.json` (C5) table `parts`, column `resource_mass` (`Part.GetResourceMass()`): 1,225 parts of 17 crafts; scope in "WP-15" | 1,225 | 0 ULP |
| `capture.ship_construct` | WP-18: in-game `captures/out2/deltav_readout.json` (C5) tables `craft` (16 crafts: parts, dry_mass, fuel_mass of `ShipConstruct.GetShipMass`) and `parts` (1,224 parts: mass, module_mass); `craft_ComSat_Lx` (fairing panelMass, WP-26) excluded per `tests/data/ship_construct_exclusions.json`; `total_mass` (editor crew) checked in the unit test; scope in "WP-18" | 1,240 | 0 ULP |
| `capture.gamedatabase_load_order` | WP-05: every `Load(Audio/Texture/Model): url` and `Config(TYPE) url` line of the capture run's KSP.log, in order | 2,836 lines | identical |
| `capture.vessel_state` | WP-14: in-game `captures/out2/aero_forces_vessel.json` (C10): structure 631 (part tree of the three PhysProf crafts assembled into a `Vessel`), states 20 (surface speed and the FlightIntegrator atmosphere mirror); cube_names, node_contact_areas and engine/input columns excluded (see WP-14) | 651 | 0 ULP, strings/ints identical |
| `unit` | 262 doctest cases, 452,479 assertions (WP-18: 26 cases, 274,042 assertions in `test_ship_construct.cpp`; WP-14: 24 cases, 6,901 assertions in `test_vessel.cpp`; WP-15: 11 cases, 21,194 assertions in `test_resource_flow.cpp` and 11 cases, 7,316 assertions in `test_resource_modules.cpp`; WP-13: 17 cases, 100,738 assertions in `test_part.cpp`; WP-10: 17 cases, 2,009 assertions in `test_timewarp.cpp`; WP-06: 14 cases, 204 assertions in `test_modules.cpp`; WP-05: 5 cases, 207 assertions in `test_partloader.cpp`; WP-11: 13 cases, 20,081 assertions in `test_terrain.cpp`; WP-08: 9 cases, 374 assertions in `test_patched_conics.cpp`; WP-09: 8 cases in `test_celestial.cpp`; WP-07: 18 cases in `test_orbit.cpp`, incl. native Quaternion.AngleAxis 84/84 bit-exact vs the in-game C3 capture; integration: 2 cases, 934 assertions in `test_celestial_orbit.cpp`; WP-03: `test_events.cpp`, `test_util.cpp`, `test_wp03_reference.cpp` with 11,032 assertions against 6,437 values from KSP's IL on .NET Framework 4.8, `tests/data/wp03_reference.tsv`), incl. 29,028 .NET Framework 4.8 reference results (`tests/data/netfx_reference.tsv`: parse 6,041 doubles + 6,041 floats, G17/G/G9 formatting 7,118, Sin/Cos/Tan 3,005 each, Ceiling/Round 400 each) | - | pass |

Not gated any more by design: the harness's .NET-generated float32 math_shim rows. kspcore follows Mono (the game) and
differs from that .NET golden in 46 cells outside its tolerance (r0..r2): up to 6 ULP (Vector3.Project/ProjectOnPlane) and
`Mathf.Repeat(1e20f, 360f)` = 360 (Mono) vs 0 (.NET); kspcore equals the in-game capture in all 903 rows.

## Build and test times on this machine (laptop, MSVC 14.44, Ninja)

| | configure | build (from clean) | ctest (10 tests) |
|---|---|---|---|
| RelWithDebInfo | 2.0 s | 12.6 s | 12.9 s (capture.libm_mono 8.8 s, corpus 2.4 s) |
| Debug | 1.9 s | 6.4 s | 33.2 s (capture.libm_mono 20.1 s, corpus 9.8 s) |

With WP-07 (branch `wp07`, 11 tests): RelWithDebInfo ctest 13.0 s (golden.orbit_propagation 0.4 s, unit 0.2 s), Debug
33.9 s (golden.orbit_propagation 0.9 s, unit 0.5 s). Main after the WP-07/WP-09 integration (15 tests, fresh): RelWithDebInfo
ctest 12.5 s (orbit_propagation 0.4 s, body_catalog 0.1 s), Debug 26.1 s.

Incremental rebuild after a one-file change: ~1-2 s. `build/RelWithDebInfo/kspcore.lib` is 9.6 MB (/Z7 debug info
embedded).

## Commits (core/, branch main)

| hash | content |
|---|---|
| `d69e30f` | scaffold: CMake/presets, golden gate, .NET runtime layer, vendored test/JSON libs |
| `cd4366d` | WP-01 maths: Mathf/Vector/Quaternion, Vector3d/QuaternionD/UtilMath/CelestialFrame |
| `83d761c` | WP-02 ConfigNode |
| `c75c6c2` | WP-01 curves (AnimationCurve, FloatCurve) + GameData corpus test |
| `c4cf5a8` | WP-01 in-game Mono semantics (C2/C7), UnityRandom, KSPRandom, System.Random, capture gates |
| `bf43535` | WP-01 remaining files: DoubleCurve, Vector3/4Curve, Matrix4x4D, Mathfx, extensions, PIDclamp |
| `8992c77` | scaffold: Unreal link settings + C ABI |
| `854766d` | WP-02 KSPParseable, KSPUtil vector helpers, type parsers |
| `d229056` | docs: STATUS.md, PROGRESS.md |

The first four commits were each built and tested in a clean worktree (each passes the gates it contains).

Branch `wp07` (from `c051578`, merged in `d2e66e7`): `431b44d` WP-01 fixes needed by Orbit (Math.Min/Max, System.Math in Angle
helpers, golden number text); `03e2310` Orbit + CelestialBodyRef/PlanetariumFrames + orbit_propagation gate;
`6073a3f` Orbit unit tests, native AngleAxis bit-exact; `d982b62` Targeting; `7e5a33a` test speed-up; `10d3aea`
OrbitSnapshot, OrbitUtil, enums; `1804b39` STATUS; `304729a` README; plus the BodyOrbit/integration-notes commit.

## Incomplete, and why

- **WP-02 UrlDir/UrlFile/UrlConfig**: done in WP-05 (`include/ksp/io/url_dir.hpp`), see "WP-05".
- **ConfigNode reflection mapping** (CreateConfigFromObject, LoadObjectFromConfig, [Persistent], links): WP-06 by plan.
  `ConfigNode.Load(path, bypassLocalization: false)` would call `Localizer.TranslateBranch` (not ported; `Load(path)`
  bypasses it, as in KSP). `TryGetEnum` takes an explicit member table (no reflection in C++).
- **Native Unity maths** (Quaternion.Euler/AngleAxis/LookRotation/FromToRotation/Slerp/RotateTowards,
  Vector3.Slerp/RotateTowards/OrthoNormalize, PerlinNoise): not ported; C3 captured them raw
  (`captures/out/math_shim_native.json`), which is the golden for the next step. `Quaternion.Inverse` (a plain
  conjugate, confirmed by C3) is provided. QuaternionD's extern members are dead in game (MissingMethodException) and
  are deliberately absent.
- **AnimationCurve weighted (Bezier) segments and Loop/PingPong wrap modes**: throw `std::logic_error`, as in the
  harness shim; KSP never produces them (C1: weightedMode 0 and ClampForever everywhere).
- **UnityRandom KSP consumers** (`ksp_class_radius`, `ksp_asteroid` tables of the C2 capture) need DiscoveryInfo / Orbit
  (later packages); the tables are excluded from the gate.

## Known approximations and risks

- **VS2013 CRT dependency**: Mono and .NET Framework 4.8 use the Visual Studio 2013 CRT libm. On Windows kspcore loads it at
  run time from `msvcr120_clr0400.dll` (or `msvcr120.dll`); everywhere else (and with `KSP_LIBM_PORTABLE=ON`) it runs the
  bit-exact software lift of that DLL's own code (WP-00L: `src/dotnet/vs2013_libm_gen.cpp`; `dotnet::LibmBackend()` reports
  which backend is active and is part of `kspcore_build_info()`). The `<cmath>` fallback is gone.
- **Mono number formatting**: Mono builds 17 digits from a 64-bit approximation (MantissaBitsTable) before rounding;
  kspcore uses the correctly rounded 17 digits, then Mono's half-up rounding. This matched all 4,200 captured strings
  (including the known Mono/.NET G15 difference), but the approximation itself is not ported.
- **Mono Pow special cases**: only `Pow(+-0, negative) = +Infinity` is evidenced by the capture and implemented; other
  Mono wrapper rules (sysmath.c) are not verified.
- **Mono float32 rule at JIT spills**: Mono can also round where its JIT spills a stack temporary; following the C#
  source's own locals reproduced every probe and all 903 math_shim rows, but arbitrary code may hit IL shapes not yet
  seen (C7 `locals` probe).
- **.NET string semantics on UTF-8**: culture-sensitive `IndexOf`/`StartsWith` are ordinal here (identical for the ASCII
  syntax of cfg files); `ValueList.SortByName` uses an en-US-like comparison with a stable sort (List.Sort is unstable);
  invalid UTF-8 bytes become U+FFFD one byte at a time.
- **Thread safety**: `SetRuntime`, the log sink and `UnityRandom::Global()` are process-wide; AnimationCurve evaluation
  is stateless except for curves with duplicate key times (faithful per-curve cache, not thread-safe).

## Surprises a later implementer must know

WP-07 (Orbit):
- `solveEccentricAnomaly(M, ecc)` ignores `ecc`: the branch (Newton below 0.8, eight fixed Laguerre-type iterations
  0.8..1, hyperbolic Newton above) and the value the solvers use are the `eccentricity` field. The 0.8..1 solver calls
  Math.Sign, which throws on NaN; KSP catches it and returns NaN. The Newton loops are unbounded (|dE| <= 1e-7).
- `GetTimeToPeriapsis()` returns `timeToPe - period` whenever e < 1, the patch is not FINAL and
  `StartUT + timeToPe > EndUT`, i.e. always for a fresh Orbit (StartUT = EndUT = 0).
- `Init()` sets eccVec and h in the zup frame but `an` in the world-like frame; `UpdateFromFixedVectors` leaves eccVec
  world-like. pos/vel/h end up zup-local. Zup is the identity only up to ~6e-17 cross terms, so the frames differ in
  the last bits.
- `GetOrbitalStateVectorsAtUT` and `UpdateFromUT` use `Planetarium.ZupAtT(UT, referenceBody)`: for a body with
  `inverseRotation` set they return vectors in that body's ROTATING frame (getRelativePositionAtUT does not). The Mun
  and Minmus prefabs ship with `inverseRotation = 1` and the harness never cleared it, so the golden's sv_*/u_* columns
  for Mun/Minmus orbits are rotated; the runner rebuilds rotPeriodRecip (Mun tidally locked: 1/orbit.period).
- `UpdateFromStateVectors`' NaN-argPe fallback (Quaternion.AngleAxis, native) stores Acos(...) in radians into the
  degrees field; it only runs on overflowing input (an infinite eccentricity vector).
- `TrueAnomalyAtRadius` clamps R to [PeR, ApR] and can return NaN at the ends (the acos argument rounds past -1/1).
- `GetRadiusAtPhaseAngle` / `PhaseAngle` use the float Deg2Rad 0.01745329238474369.
- `OrbitUtil.GetSmaDistanceToSun` double-counts: the running sum is passed down and added again.
- Unity's native `Deg2Rad` is `deg / 360 * 2 * kPI`, not `deg * (pi/180)` (18 of 84 AngleAxis rows differ otherwise).
- `Targeting.Sample.get_normal_intercept_frames` reports the intercept count even when it zeroed the frames because the
  points were not real (hyperbola far branch): such samples are "valid" with zero frames.
- (Superseded on main by WP-09's CMake walk-up: worktree builds need no `-DKSP_PROJECT_ROOT` any more.)

1. **The harness's float32 goldens are .NET, not the game.** Mono evaluates C# float expressions in double and rounds
   only at stores/arguments/returns/casts. Port managed float code with `f32store(<double expression with fx()>)` at
   exactly the C# store points (`include/ksp/dotnet/mono_float.hpp`); never for native Unity code. Gate float32 paths
   against `harness/captures/out/*_mono.json` where they exist.
2. **Math.Sin/Cos are x87 FSIN/FCOS** on both runtimes (66-bit pi reduction: `Cos(Math.PI/2) = 6.1230317691118863E-17`,
   `Sin(x) = x` for |x| >= 2^63). kspcore emulates them exactly (192-bit reduction, double-double kernel); do not call
   `std::sin` in ported code, call `dotnet::Math::Sin`. Tan differs between the runtimes (Mono = CRT tan).
3. **double.Parse is not correctly rounded** in KSP's runtime: it is the CLR/Mono NumberToDouble (64-bit mantissa,
   power-of-ten tables read from mono-2.0-bdwgc.dll). Use `dotnet::ParseDouble/TryParseDouble`, never `strtod`.
   "-0" parses to +0; "1,000" parses as 1000 (AllowThousands); float overflow is a parse failure.
4. **double.ToString() on Mono rounds twice** (17 digits, then half-up to 15): `5.997450259893145` prints
   `5.99745025989315`. G17/G9/R match .NET on everything seen so far.
5. **FloatCurve/AnimationCurve is native code**: per-operation float32, bit-exact against native Unity (C1). Its
   evaluation is stateless in kspcore (identical to Unity's cached evaluation for strictly increasing key times).
6. **ConfigNode**: `//` starts a comment anywhere, a root-level `}` ends the file, ids survive ToString but not Save,
   `SetValue(..., createIfNotFound)` returns false when it creates and does not sanitize, `RemoveValue` removes the last
   match, `RemoveValues(string)` matches whole names, `AddValue` sanitizes `{}` (so a braced GUID cannot be stored),
   copying a tree with an anonymous child throws (NullReferenceException in KSP).
7. **Unreal**: link only through `kspcore_c.h`; never include `include/ksp/**` from UE code (inline float code would be
   recompiled with UE's /fp:fast and break the one-definition rule). The lib is built /MD, IDL 0, /Z7, no /GL.

## WP-11: terrain height maths (branch `wp11`)

Spec: `PQS.cs` (height path only: SetupMods :2109, GetChildMods :2161, SetupBuildDelegates :2177, Mod_OnSetup :1827,
Mod_GetVertexMax/MinHeight :1892/:1902, Mod_OnVertexBuildHeight :1956, BuildVertexMapCoords :2294, GetSurfaceHeight
:3088/:3093, VertexBuildData :77), `PQSMod.cs`, every class that overrides OnVertexBuildHeight (28: `PQSLandControl.cs`
and `PQSMod_{FlattenArea, FlattenAreaTangential, FlattenOcean, MapDecal, MapDecalTangent, SmoothLatitudeRange,
VertexDefineCoastLine, VertexHeightMap, VertexHeightMapStep, VertexHeightNoise, VertexHeightNoiseHeightMap,
VertexHeightNoiseVertHeight, VertexHeightNoiseVertHeightCurve, -Curve2, -Curve3, VertexHeightOblate, VertexHeightOffset,
VertexNoise, VertexPlanet, VertexRidgedAltitudeCurve, VertexSimplexHeight, VertexSimplexHeightAbsolute,
VertexSimplexHeightFlatten, VertexSimplexHeightMap, VertexVoronoi, VoronoiCraters, VoronoiCraters2}.cs`), `MapSO.cs`,
the reachable LibNoise subset (`LibNoise/{Math, GradientNoiseBasis, ValueNoiseBasis, Perlin, Billow, RidgedMultifractal,
Voronoi, IModule, NoiseQuality, Modifiers/ScaleBiasOutput, Modifiers/Select}.cs`), `CelestialBody.cs:656/:831`
(GetRelSurfaceNVector, TerrainAltitude), native `Quaternion.FromToRotation` (Unity 2019.4). WP-03's Simplex is reused.
Every ported function cites its line.

### Gate (RelWithDebInfo and Debug, fresh; `tests/tolerances_capture.json` unchanged: 0 ULP)

| gate / table | rows | columns | max abs | max ULP | notes |
|---|---|---|---|---|---|
| `capture.terrain_height` / samples | 140,522 | h_surface, h_override, terrain_alt (f64) | 0 | 0 | Kerbin + Mun 1-degree grids (65,160 each), Kerbin 0.01-degree KSC patch (10,201), the pad; **no exclusions** |
| `capture.terrain_height` / bodies | 2 | Radius, pqs_radius, radiusMin, radiusMax, ocean, pqs_name | 0 | 0 | Kerbin 598015 / 608735, Mun 199600 / 208300 |
| `capture.terrain_height` / scene_probe | 4 | h_ksc, h_override_ksc, h_override_0_0 (MAINMENU and FLIGHT) | 0 | 0 | |

Golden = the in-game capture `harness/captures/out2/terrain_height.json` (FLIGHT, Mono). Both the public
`GetSurfaceHeight` (h_surface) and the `overrideQuadBuildCheck` variant (h_override) are bit-exact. The 3 Mun points
where the game's two variants differ ((-82, 102), (-82, 103), (-82, 104): FlattenArea "Monolith01/02" with no quad
built) are reproduced, not excluded: the runner emulates the capture's scene state (Kerbin's sphere active with quads
built = `PQS::Mod_OnQuadBuilt`, the Mun's never built). KSC pad: `TerrainAltitude` at the active vessel's lat/lon =
**64.78470540174749 m** (capture 64.7847). Columns not compared (kept list in tests/CMakeLists.txt): the capture's
bodies.mod_count/isStarted/isActive and scene_probe.has_pqs/isStarted/isActive/mod_count/ut/go_active are scene state
(Kerbin's runtime stack holds 9 PQSCity2 set-dressing objects spawned by space-centre code, not by the PQS prefab);
the mods/mod_params/map_assets tables are covered by unit tests. Gate time 4.5 s RelWithDebInfo, 14 s Debug.

### Unit tests (`tests/test_terrain.cpp`: 13 cases, 20,081 assertions)

- Stock prefab data equals `data/terrain/pqs_prefab.json` bit for bit: 15 bodies, every mod's class/GameObject/
  activeSelf/height flag and 3,653 serialized values (doubles, floats, ints, bools, strings, Vector3, Color, curves
  key by key, map keys).
- Extracted maps: size and sha256 against the manifest, origin asset file present in `decompiled/KSP-dev`.
- Evaluated stack vs the capture's `mods` table: Mun identical (23 mods incl. List.Sort's permutation of the
  order-100 group); Kerbin height mods identical as multisets per `order` (see quirk 5), plus a commutation test that
  evaluates the game's and kspcore's order-99999 permutation at 6 x 41 x 41 points around each decal/flatten centre
  and antipode: bit-identical.
- KSC pad 64.78470540174749 m, scene-probe h(KSC) `0x4122500191C4E878`, radii; TerrainAltitude clamp/allowNegative/no
  PQS, the override variant; the Mun FlattenArea quad dependency; every stock body evaluates finite heights.
- MapSO: Byte2Float, u = 1.0 and negative wrap, exact-integer coordinates, maxX/maxY wrap, double vs float bilinear
  overloads (different bits), HeightAlpha channels and lerp, channel averaging on 3-byte maps, the GetPixelByte
  negative-x bug, bad sizes.
- LibNoise: IntValueNoise/ValueNoise, GradientCoherentNoise (Low/Standard/High, lattice points, the (int)0 - 1 quirk),
  Perlin, Billow, RidgedMultifractal (seed wrap, spectral weights), Voronoi (distance on/off), Select/ScaleBiasOutput:
  expectations from `tools/wp11/libnoise_reference.py`, an independent Python transcription of the C# (IEEE doubles in
  source order; two values also derived by hand).
- Native `Quaternion.FromToRotation` vs all 49 rows of the C3 capture (`captures/out/math_shim_native.json`): bit-exact.
- FlattenAreaTangential quad state; VertexHeightMapStep throws without a Texture2D sampler.

### Game assets (CLAUDE.md "Original game assets")

`tools/wp11/extract_terrain.py` (run with `harness/tools-cache/venv`, read-only on `decompiled/KSP-dev/KSP_x64_Data`,
same UnityPy 1.25.4 + IL type-tree method as WP-09's `extract_body_assets.py`; sharedassets9.assets sha256
`09e9525a...81c3c`) walks every body's PQS GameObject like `PQS.GetChildMods`, dumps every PQSMod's serialized fields to
`data/terrain/pqs_prefab.json` (436 KB) and copies `MapSO._data` verbatim for every MapSO a height mod references
(colour-map fields excepted) to `data/terrain/maps/<name>.<PathID>.bin`, with `data/terrain/manifest.json` (origin
file + PathID, Unity object name, MapSO header, byte size, sha256, consumers). No texture is decoded: MapSO stores its
pixel bytes in the asset itself. 16 maps, 56,786,944 bytes, all from `KSP_x64_Data/sharedassets9.assets`:

| map key | origin | size, depth | bytes | consumers |
|---|---|---|---|---|
| `decal_spaceport_0_height.19927` | sharedassets9.assets PathID 19927 | 1024x1024, 2 B/px | 2,097,152 | Kerbin MapDecal |
| `desertplanet_height.19917` | sharedassets9.assets PathID 19917 | 4096x2048, 1 B/px | 8,388,608 | Duna PQSLandControl, Duna VertexHeightMap |
| `dwarfplanet1_height.19916` | sharedassets9.assets PathID 19916 | 2048x1024, 1 B/px | 2,097,152 | Dres VertexHeightMap |
| `eve_height_new.19923` | sharedassets9.assets PathID 19923 | 4096x2048, 1 B/px | 8,388,608 | Eve PQSLandControl, Eve VertexHeightMap |
| `genericoffset.19987` | sharedassets9.assets PathID 19987 | 256x128, 1 B/px | 32,768 | Vall VertexHeightMap |
| `icemoon_mountains.19988` | sharedassets9.assets PathID 19988 | 1024x512, 1 B/px | 524,288 | Vall PQSLandControl, Vall VertexHeightMap |
| `island_runway_decal_heightmap.19928` | sharedassets9.assets PathID 19928 | 256x256, 2 B/px | 131,072 | Kerbin MapDecalTangent |
| `kerbin_height_new.19929` | sharedassets9.assets PathID 19929 | 4096x2048, 1 B/px | 8,388,608 | Kerbin PQSLandControl, Kerbin VertexHeightMap |
| `ksc_decal_heightMap.19930` | sharedassets9.assets PathID 19930 | 1024x1024, 2 B/px | 2,097,152 | Kerbin MapDecalTangent |
| `moho_height.19981` | sharedassets9.assets PathID 19981 | 2048x1024, 1 B/px | 2,097,152 | Moho VertexHeightMap |
| `munHeightMap2.19983` | sharedassets9.assets PathID 19983 | 4096x2048, 1 B/px | 8,388,608 | Mun VertexHeightMap |
| `oceanmoon_height.19931` | sharedassets9.assets PathID 19931 | 4096x2048, 1 B/px | 8,388,608 | Laythe PQSLandControl, VertexHeightMap, VertexSimplexHeightMap |
| `pyramid.19919` | sharedassets9.assets PathID 19919 | 1024x1024, 1 B/px | 1,048,576 | Duna MapDecal |
| `rockymoon_height.19985` | sharedassets9.assets PathID 19985 | 2048x1024, 1 B/px | 2,097,152 | Tylo VertexHeightMap |
| `saganface.19986` | sharedassets9.assets PathID 19986 | 512x512, 2 B/px | 524,288 | Tylo MapDecal |
| `snowydwarfplanet_height.19921` | sharedassets9.assets PathID 19921 | 2048x1024, 1 B/px | 2,097,152 | Eeloo PQSLandControl, Eeloo VertexHeightMap |

Referenced but not extracted (colour data, later packages): KerbinTextureAtlasMap (8192x4096x3, 100 MB),
munColorMap2, rockymoon_color, duna/eve/gilly_coloraddition, desertplanetmoon_coloroverlay, pyramidcolor, bigcrater
(Bop's two MapDecals use it as colorMap only and have no heightMap). `MapSO::GetOrigin()` carries file + PathID + name +
key next to every loaded map. The C++ mod stacks are generated: `tools/wp11/gen_wp11_terrain.py` ->
`src/terrain/pqs_prefab_data.cpp` (15 factories, 225 KB) and `src/terrain/libnoise_random_vectors.inc` (the 1024
RandomVectors literals copied from `GradientNoiseBasis.cs`); the generator fails if any prefab field of a height mod is
neither consumed nor listed as ignored (sphere back-reference, Gradient colour ramps, FlattenArea.body,
PQSLandControl.scatters and per-class scatter objects) and cross-checks the passive mods' OnSetup requirements against
the capture (0 differences).

### API (for WP-12/19/20)

`include/ksp/terrain/pqs_prefab.hpp`: `terrain::MapLibrary(dataDir)`, `terrain::LoadStockPQS(body, maps)` (all 15
bodies with a PQS), `PQS::GetSurfaceHeight(dir[, overrideQuadBuildCheck])`, `PQS::radius/radiusMin/radiusMax/
radiusDelta`, `terrain::GetRelSurfaceNVector(lat, lon)`, `terrain::TerrainAltitude(pqs, lat, lon, allowNegative)`
(CelestialBody.TerrainAltitude: pqs.radius, ignores `ocean`, which `StockPQSInfo::ocean` carries for callers),
`terrain::SurfaceHeightOverride(pqs, lat, lon, bodyRadius)` (Vessel.cs:2717 form), `PQS::Mod_OnQuadPreBuild/
Mod_OnQuadBuilt(PQ)` for quad state. Custom/modded stacks: fill `PQS::childMods` with `PQSMod_*` objects and call
`SetupForHeight()`.

### Not done in this package (and why)

- Quads, meshes, normals, UVs, colliders, the vertex build delegates and PQ (WP-12). The `PQ` struct here is a stub
  with the two fields the height mods read in OnQuadPreBuild.
- Colour output: every mod's OnVertexBuild, MapSO.GetPixelColor*/Color32 sampling, Gradient ramps, the texture atlas,
  biome colour and CBAttributeMapSO lookup; scatter (LandClassScatter, PQSLandControl scatter setup and quads).
- `PQSMod_VertexHeightMapStep` and `PQSMod_VertexHeightNoiseHeightMap` (no stock body uses them) read a Texture2D
  through Unity's native `GetPixelBilinear(...).grayscale`; not ported: they throw `std::logic_error` unless the caller
  sets `heightMapSampler`.
- Bodies other than Kerbin and Mun: the port and data are generic and every stock body evaluates, but the capture has
  no golden for them (recommend a capture run over all 15 bodies; the runner and gate extend by adding body names).
- `CelestialBody.pqsController` is not wired into WP-09's `CelestialBody` (the helpers take a `PQS*`): integration step.
- PQSCity/PQSCity2 (positioning, KSC set dressing), PQS lifecycle beyond setup (OnSphereStart/Active, PostSetup),
  QuadEnhanceCoast and other non-height mods (kept as `PQSModPassive`).

### Surprises for later implementers

1. Height maps are `MapSO` ScriptableObjects whose `_data` byte array is in the asset: no texture decode. The "16-bit"
   decals are 2 bytes per pixel = (height, alpha) as two 8-bit channels; there is no 16-bit height decode in 1.12.5.
   Byte -> float is `0.003921569f * b` (not /255; 255 still rounds to 1.0f).
2. MapSO bilinear: the double overload (VertexHeightMap, PQSLandControl) and the float overload (MapDecal*) round
   differently (different bits for the same u, v); u = 1.0 samples texel 0; the upper texel wraps only when it equals
   the width; `GetPixelByte(x < 0)` reads `Width - x` (sic, a texel of the next row).
3. MapDecal/MapDecalTangent have no hemisphere test: outside quad building, `GetSurfaceHeight` applies a decal at the
   antipode of its centre as well (unit-tested on Kerbin's KSC decal).
4. Quad state decides the public `GetSurfaceHeight`: MapDecal* apply until a quad build starts (quadActive and
   buildHeight both start false), FlattenArea applies only with overrideQuadBuildCheck or after OnQuadBuilt,
   FlattenAreaTangential never until a quad was built. The 3 differing Mun points of the capture are this.
5. `PQS.SetupMods` uses List.Sort (unstable): the permutation of equal-`order` mods depends on the whole list. The
   game's Kerbin list has 9 more PQSCity2 (runtime set dressing), so kspcore's order-99999 permutation differs from the
   game's; those three mods commute (tested), heights are bit-exact. A future mod that overlaps another equal-order
   mod would need the runtime list.
6. Building a PQS constructs many `Simplex` objects; each reseeds `UnityEngine.Random` and rewrites Simplex's static
   table (also in KSP): anything drawing from UnityEngine.Random afterwards sees a different state (the terrain tests
   restore it so WP-03's tests stay order-independent).
7. Without Breaking Ground, `PQSROCControl.Awake` sets modExpansionDisabled and SetupMods drops it; Kerbin's
   AltitudeAlpha is modEnabled = false in the prefab.
8. `PQSMod_VertexHeightMap.OnVertexBuildHeight` uses `heightMapDeformity`, while GetVertexMaxHeight uses the
   radius-scaled `heightDeformity` (sic). `VertexRidgedAltitudeCurve`, `-Curve2/3` and `VertexHeightNoiseVertHeight`
   read `sphere.radiusMin`/`radiusDelta`, so every mod's GetVertexMin/MaxHeight feeds the terrain.
9. MapDecal's `rot = AngleAxis(angle, up) * FromToRotation(posNorm, up)` mixes native (AngleAxis, FromToRotation) and
   managed (Quaternion * Quaternion) code; `rot * vbData.directionFromCenter` resolves to Unity's float
   `Quaternion * Vector3` (Vector3d -> Vector3 implicit). FromToRotation is Moller-Hughes + Shoemake in float32,
   `h = (1 - e) / |v|^2` (the `1 / (1 + e)` variant misses 2 of 49 captured rows).
10. Managed float code in height mods (VoronoiCraters, MapDecal smoothing, MapSO) follows the Mono rule: e.g.
    `(double)(heightEnd - heightStart)` and `1f / x` passed to a double are not rounded to float.
11. `TerrainAltitude` subtracts `pqs.radius` and ignores `ocean`; the vessel-positioning calls subtract
    `CelestialBody.Radius` and use overrideQuadBuildCheck. MAINMENU and FLIGHT give identical heights except for
    quad state, so the headless port needs no scene.
12. PQSMod_TextureAtlas assigns MeshUV3 only under `CanTextureAtlasModBeUsed()` (graphics settings); the generator
    ignores conditional requirement assignments (matches the capture).

### Commits (branch `wp11`)

| hash | content |
|---|---|
| `9ed17fb` | WP-11 code, generated stack data, extracted terrain data (data/terrain), extraction/generator tools, capture gate, unit tests, README/STATUS |
| (this commit) | STATUS: commit hash, PROGRESS-wp11.md final notes |

## WP-05 UrlDir, GameDatabase (data side), PartLoader (data side), resource library (branch `wp05`, merged in `bfb732f`)

State: done for the data side. Every ported function cites `File.cs:line` (Assembly-CSharp). Input is the game's own
GameData, read in place and never written.

**GameData root convention.** `ksp::GameData::Load({.gameRoot = <KSP install>})` reads `<gameRoot>/GameData` (the
GameDatabase urlConfig: urlRoot "", type GameData), `<gameRoot>/Physics.cfg` (only when no PhysicsGlobals instance
exists) and `<gameRoot>/PartDatabase.cfg`. `gameDataDir` overrides the GameData directory. The golden runner takes
`--gamedata <dir>` (default `<harness>/../decompiled/KSP-dev/GameData`, the dev copy) and uses its parent as the game
root. Unit tests write a synthetic GameData tree to the temp directory and also check the dev copy when present.

| area | ported (C++ location) | verification |
|---|---|---|
| UrlDir (UrlDir.cs, all) | `ksp::io::UrlDir/UrlFile/UrlConfig/UrlIdentifier` (`io/url_dir.hpp`): directory walk in NTFS order (explicit sort: upper-cased UTF-16 ordinal), `.svn`/`PluginData`/`zDeprecated` skipped, case-sensitive extensions, cfg parsing, Parts-type whole-file PARTs, url construction, all lookups | load order == KSP.log (2,836 lines); unit |
| GameDatabase (data) | `ksp::GameDatabase` (`database/game_database.hpp`): LoadObjects (UrlDir with the loaders' extension table, Localizer tags of the language + TranslateBranch on every config, audio/texture/model tables with the OUT-OF-DATE replacement rule, DDS/MBM header metadata for the loader flags and DDS format failures), CompileConfig, every config/texture/model/audio lookup, ToString | `capture.gamedatabase_load_order`; unit |
| Orchestration | `ksp::GameData` (`database/game_data.hpp`): CreateDatabase order (GameDatabase, PhysicsGlobals, PartResourceLibrary, PhysicMaterialLibrary, PartLoader) | runner, unit |
| Resources | `PartResourceDefinition/List/Library`, `PartResourceDrainDefinition` (`resources/part_resource_library.hpp`); `PhysicMaterialDefinition/Library` (`resources/physic_material_library.hpp`) | gate (density/unitCost/flowMode columns), unit |
| PartLoader (data) | `ksp::PartLoader` (`parts/part_loader.hpp`): CompileAll/CompileParts (9 prefab-part slots, Prebuilt overlays, ParsePart, cubes from cfg or PartDatabase.cfg incl. version and timestamp checks, CompilePartInfo seat reduction), ApplyPartValue (generated reflection table of all 257 Part + 9 CompoundPart public fields), LoadObjectFromConfig for AvailablePart, SanitizePartCosts, node_* parsing, CompileModel resolution (MODEL nodes, texture replacements, first model of the directory), AddModule (class lookup, Cargo/Inventory exclusivity, ModuleTripLogger), AddResource, ModulePartVariants base/variant list, GeneratePartAutoTags, InferBulkheadProfiles (dead code in KSP) | gate, unit |
| Module data | `PartModuleData` / module class table (`parts/part_module_data.hpp`, generated `module_schema.inc`: 151 PartModule classes, 1,595 [KSPField]/[KSPAxisField] members with C# type, Type.Name, isPersistant, initializer text, 13 enum tables): class resolution, BaseFieldList.Load of the MODULE node with BaseField.ReadPvt typing, FloatCurve/DoubleCurve nodes, Propellant (ModuleEngines/ModuleRCS incl. RCS's synthesised PROPELLANT) | schema == C9 for all 1,378 captured modules (field names, field_count, Type.Name, isPersistant); gate |
| Data records | `AvailablePart`, `PartPrefab`, `PartFields` (generated), `AttachRules`, `AttachNode`, `DragCube`, `DragCubeList`, `PartResource`, `PartVariant`, `PartModelRef`, `PartAssetRef` (`parts/available_part.hpp`, `parts/part_data.hpp`) | gate |

Generators (regenerate, never hand-edit): `tools/wp05/gen_part_fields.py` writes `include/ksp/parts/{part_fields,
compound_part_fields,part_enums}.inc`; `tools/wp05/gen_module_schema.py` writes `include/ksp/parts/module_schema.inc`.

### Gate `capture.part_database` (RelWithDebInfo 0.8 s, Debug 1.4 s; bit-exact, no tolerance)

| table | C9 rows | gated | result | not gated, and why |
|---|---|---|---|---|
| parts | 362 | 357 x 54 cols | 0 ULP, strings identical | 5 prefab parts (kerbalEVA, kerbalEVAfemale, flag, kerbalEVASlimSuit(+Female)): base values are Unity-serialized prefab data, not GameData. The 4 DLC-suit error rows ARE gated (name + "NullReferenceException: ..."). Column partSize (renderer bounds of the model). module_mass / module_cost: gated since WP-06 |
| part_cfg_values | 6,235 | 6,235 | identical (incl. 219 float-parsed doubles) | - |
| modules | 1,378 | 1,312 | identical (moduleName, class, field_count, isEnabled) | 66 rows of the 5 prefab parts |
| module_fields | 34,801 | 32,894 (WP-06; WP-05 gated 5,401) | identical (value text, value_num, Type.Name, isPersistant) | 1,889 rows of the 5 prefab parts, 18 solar panel pivot rotations (WP-06 section) |
| module_propellants | 77 | 77 | identical | - |
| resources | 166 | 166 | identical | - |
| attach_nodes | 827 | 822 | identical | 5 prefab parts |
| drag_cubes | 535 | 531 | identical incl. SaveToString | 4 EVA cubes built in code |
| variants | 228 | 228 | identical (name, displayName, cost, mass) | column materials (the model's materials) |
| assets | 3,978 | 743 (cfg 353, model 359, texture_replacement 15, internal 16) | identical; `idx` renumbered per part | 3,230 renderer texture-slot rows (2,497 GameData + 733 Unity assets: need the .mu contents / Unity assets), 5 prefab-part cfg rows |

Scope mechanics (all in `tests/CMakeLists.txt`, filters in `tools/filter_golden.py`): DROP_ROWS (prefab parts,
texture rows), KEEP_ROWS_FILE (WP-05 only: `<candidate>/part_database/scope.json`, the module fields each MODULE node sets;
removed by WP-06), DROP_KEYS_FILE (individual rows with reasons), RENUMBER (assets.idx),
DROP_COLUMNS. Investigation record: the first gate run differed in 3 places, all explained and resolved without
relaxation: srfAttachNode on parts without node_attach (Unity's serializer instantiates [Serializable] class fields of
a new component: now modelled), fuelCrossFeed on 8 parts (ModuleToggleCrossfeed.OnLoad writes part.fuelCrossFeed:
ported, the only module effect on Part prefab data the gate found), sepMotor1 stackableQuantity (module-internal,
excluded with reason).

### Public shape (how WP-06 and the UE importer consume it)

- `GameData gd; gd.Load({.gameRoot = ...});` then `gd.parts.loadedParts` (LoadedPartsList order),
  `gd.parts.getPartInfoByName(name)`, `gd.database` (configs, `databaseTexture` / `databaseModel` / `databaseAudio`
  metadata with `UrlFile::fullPath()`), `gd.resources.GetDefinition(name)`.
- `AvailablePart`: KSP's fields (title, category, cost, entryCost, TechRequired, tags, bulkheadProfiles, partUrl,
  partConfig after ParsePart, internalConfig, fileTimes, ...), `partPrefab`, `assets` (GameData-relative
  cfg / model / texture_replacement / internal paths), `codeBuilt` / `prefabCompiled` / `loadError`.
- `PartPrefab`: `fields` (every public Part field by its C# name and default; `PartFieldTable()` / `FindPartField()`
  give reflection), `compoundFields`, `attachNodes`, `srfAttachNode`, `DragCubes`, `Resources`, `Modules`,
  `effects` (EFFECTS nodes), `fxDefinitions` / `soundDefinitions` (fx_* / sound_* values), `models` (`PartModelRef`:
  url, `.mu` UrlFile, MODEL parent/name/scale/position/rotation/iconHidden, accepted texture replacements),
  `materialOverrides` (texture/normalmap/alphaCutoff/specPower/rimFalloff values), `baseVariant` / `variantList`.
- WP-06: `PartModuleData` = `moduleClass` (`ModuleClassInfo`: name, base chain, `Fields()` in BaseFieldList order with
  types, persistence and initializer text), `config` (the MODULE node as loaded), `cfgFields` (typed values
  BaseFieldList.Load assigned), `isEnabled`, `propellants`. Instantiate the module class, apply the defaults, then
  `cfgFields`, then run the module's OnAwake/OnLoad.
- UE importer: `PartPrefab::models[i].file->fullPath()` (.mu), `GameDatabase::databaseTexture` (url to file, normal-map
  flag), `PartAssetRef::file`, MODEL transforms, texture replacements and material overrides; the importer reads the
  .mu itself (renderers, materials, texture slots, bounds).

### Not done in this package (and why)

- Module instantiation: KSPField defaults, OnAwake/OnLoad behaviour, GetInfo/moduleInfos/resourceInfo texts,
  IPartMassModifier/IPartCostModifier, UPGRADES/ADJUSTERS/EVENTS/ACTIONS nodes: WP-06.
- Model contents (.mu): renderer texture slots, partSize, variant materials, NODE{} attach nodes (position from a model
  transform: recorded with the transform name and a warning; no stock part uses NODE), icons: UE import pipeline.
- The 9 prefab parts' Unity-serialized base data (EVA kerbals, flag): UE import / Unity asset extraction (the
  harness's `tools/extract_assets.py` approach). PartLoader models the slots, applies the Prebuilt cfg overlays and
  reproduces the DLC-suit error state.
- Asset bundles (`*.ksp`) are not read (their models/textures/shaders are not part of any stock part's resolution).
  FX prefabs (`Resources.Load("Effects/...")`) and sounds are recorded as definitions only.
- Not compiled: PROP and INTERNAL spaces, VARIANTTHEME, part upgrades (C9 did not capture them), ExpansionsLoader; their
  configs stay queryable through GameDatabase.
- Texture loads other than DDS headers are assumed to succeed (KSP.log of the capture run shows no load errors).
- Resource ids use Mono's String.GetHashCode (h * 31 + c); not verified against a capture (ids are never saved).

### Surprises for later implementers

1. `ApplyPartValue` tests `typeof(double)` twice; the first branch uses `float.Parse`, so every double Part field from a
   cfg holds a float32 value (`emissiveConstant = 0.7` gives 0.699999988079071). float fields go through the generic
   `Parse(string)` reflection tail (no `typeof(float)` case).
2. Crew mass: CompilePartInfo subtracts `PerCommandSeatReduction` (any module that `is ModuleCommand`) or
   `PerSeatReduction` per seat from `mass` (Mono float rule: one rounding), only for compiled parts (prefab parts never
   go through CompilePartInfo). mk1pod.v2: 0.8 to 0x3F34BC6B (0.70600003).
3. `LoadObjectFromConfig(AvailablePart, removeAfterUse)` strips name/title/manufacturer/author/description/category/
   TechRequired/cost/tags/bulkheadProfiles/... from `partConfig`; `entryCost` is removed explicitly; an invalid
   `category` (Enum.Parse) or a RESOURCE with `amount` but no `maxAmount` (SanitizePartCosts) drops the part.
4. Drag cubes: the part cfg's DRAG_CUBE wins (and replaces the in-memory PartDatabase entry); otherwise only TOP-LEVEL
   `PART{url=}` nodes of PartDatabase.cfg count (Size5Flag contains a nested MpoProbe PART: ignored); the file's
   `PART_DATABASE.version` must equal "1.12.5.3190 (WindowsPlayer x64)" or the whole file is discarded; `LoadCubes`
   reads cube lines last to first and `SaveCubes` writes them back last to first; `ClearCubes` resets `procedural` but
   not `none`; a 24-value cube line throws IndexOutOfRange.
5. `node_*` values are scaled by rescaleFactor x `scale`, in VALUE ORDER: a `scale` after a node does not affect it;
   orientation is scaled too; a name without '_' throws; 7th value size, 8th method, 9th crossfeed, 10th rigid, 11th
   one-way crossfeed.
6. Every compiled part has a `srfAttachNode` (default AttachNode: id "", Stack, radius 0.4) and non-null `attachRules`
   even when its cfg defines none: Unity's serializer instantiates [Serializable] class fields of a new component.
7. The GameData UrlDir is named "" (urlRoot): `UrlDir.GetDirectory/GetFile` from the root never descend into it (only
   `GetConfig` handles unnamed directories), so `GameDatabase.GetConfigNodes(baseUrl, type)` finds nothing for GameData
   paths; `GetFile` treats the last url element as a config name.
8. Texture lookups are OrdinalIgnoreCase with 7 hard-coded flag-url swaps; model lookups are ordinal; the legacy
   `mesh =` rule takes the FIRST model (load order) of the part's directory; a second file with the same url (e.g.
   `a.png` + `a.dds`) replaces the first at the end of the table on the first load.
9. Module OnLoad effects on Part prefab data exist: ModuleToggleCrossfeed sets `part.fuelCrossFeed = crossfeedStatus`
   (KSPField, default false) on 8 stock parts. ModuleRCS without PROPELLANT adds one to the live MODULE node.
10. AddModule: ModuleCargoPart is refused after ModuleInventoryPart; ModuleInventoryPart removes an earlier
    ModuleCargoPart whose packedVolume >= 0. vesselType > Unknown appends ModuleTripLogger.
11. The 9 prefab parts come first in LoadedPartsList; a PART cfg with their name is applied onto the prefab
    (`__OVERLOAD = True` modules reload the prefab's module). The DLC suits have no cfg without the DLC: their prefab is
    never Awake'd and reading it throws NullReferenceException (C9 error rows).
12. `InferBulkheadProfiles` is never called: parts without `bulkheadProfiles` have null.
13. CompileConfig logs the literal "CloneURL: System.String[]".

### Commits (branch `wp05`, from main `f964447`)

`987cae2` UrlDir, GameDatabase, resource library, schema generators; `f0c53e1` PartLoader data side, part_database
candidate, golden row filters; `32f8f60` capture.part_database gate + load-order check; `da22d65` unit tests;
`c0dc966` PhysicMaterialLibrary; then the docs commit (STATUS.md, README.md, PROGRESS-wp05.md).

Shared files touched (merge notes): `src/CMakeLists.txt`, `tests/CMakeLists.txt` (gate options DROP_COLUMNS, DROP_ROWS,
KEEP_ROWS_FILE, DROP_KEYS_FILE, RENUMBER), `cmake/RunGolden.cmake`, `tools/filter_golden.py` (row/column filters),
`tools/golden_runner/{main.cpp,datasets.hpp,CMakeLists.txt}` (`--gamedata`), README.md, STATUS.md.

### Merge into main (after WP-11)

`wp05` (from `f964447`) merged into main on top of WP-08 and WP-11: merge commit `bfb732f`.

| file | resolution |
|---|---|
| `src/CMakeLists.txt`, `tools/golden_runner/CMakeLists.txt`, `tests/CMakeLists.txt` (sources) | union (WP-11 terrain + WP-05 io/database/resources/parts sources; ds_patched_conics/orbit_rails/terrain_height + ds_part_database; test_patched_conics/test_terrain/sha256 + test_partloader) |
| `tools/filter_golden.py` | rewritten as one option set: WP-08's `--rows t:col^prefix` and WP-05's `--drop-rows t:col=v` share one row-condition grammar (`--keep-rows` / `--drop-rows`, conditions `col=v1\|v2` or `col^p1\|p2`); fixed step order (tables, key, keep/drop rows, keep-rows-file, drop-keys-file, renumber, column filters); a table/column named by a filter but absent from the golden is an error. The textual auto-merge had a bug: WP-05's keep-rows-file loop reused the name `keys` and overwrote WP-08's `--key` table, so a gate using both would silently lose its key override |
| `cmake/RunGolden.cmake` | one table option -> flag; `<OUT>` substituted in KEEP_ROWS_FILE and DROP_KEYS_FILE |
| `tests/CMakeLists.txt` (`ksp_golden_gate`) | options GOLDEN TOL TABLES KEYS KEEP_ROWS DROP_ROWS KEEP_ROWS_FILE DROP_KEYS_FILE RENUMBER DROP_HEX COLUMNS DROP_COLUMNS; `capture.orbit_rails` uses `KEEP_ROWS` (was `ROWS`, same filter); WP-05's second `set(KSP_CAPTURES2_DIR ...)` dropped (main already sets it) |
| `tools/golden_runner/{main.cpp,datasets.hpp}` | auto-merged, no overlap: `--gamedata` (default `<harness>/../decompiled/KSP-dev/GameData`) is read only by `part_database` |
| `README.md` | tests and layout tables unioned (main's `include/ksp/celestial/` row incl. WP-08 `flight_globals.hpp`); new "Golden gate options" table |
| `STATUS.md` | every section kept, WP-05 after WP-11; summary, test table, header (20 tests) and `unit` totals merged |

Check of the unified filter: for every filtered gate, the filtered golden it writes is JSON-identical to the one the
old main filter (math_shim.f64/f32, libm_mono, unity_random, physics_drag_curve, orbit_rails, terrain_height) or the
old wp05 filter (part_database) writes with the same options. Gate results after the merge (fresh, RelWithDebInfo and
Debug): 20/20 ctest, every gate max ULP 0 / max abs 0; doctest 125 cases, 39,849 assertions.

Observation (unchanged by the merge): compare.py does not fail on extra candidate rows. The `capture.part_database`
report lists 641 candidate rows absent from the filtered golden (parts 5, modules 66, module_fields 565, assets 5:
all but one are rows of the 5 prefab parts the gate drops from the golden; the other is the excluded
sepMotor1 `stackableQuantity` row, which the runner writes as the cfg value 2 while the game holds 1); every golden row
matched. (Historical: with WP-06 the runner writes every module field and the sepMotor1 row matches; see the WP-06 section.)
## WP-04: save model and upgrade pipeline (branch `wp04`, data side)

Spec: `Game.cs`, `GameParameters.cs` (+ `CommNet/CommNetParams.cs`, `Expansions/Missions/MissionParams{General,Extras,Facilities}.cs`),
`FlightState.cs`, `ProtoVessel.cs`, `ProtoPartSnapshot.cs`, `ProtoPartModuleSnapshot.cs`, `ProtoPartResourceSnapshot.cs`,
`AttachNodeSnapshot.cs`, `VesselNaming.cs`, `ProtoTargetInfo.cs`, `ProtoWaypointInfo.cs`, `ProtoCrewMember.cs`,
`KerbalRoster.cs`, `FlightLog.cs`, `Experience/Experience{System,Trait,Effect}Config.cs`, `ProtoScenarioModule.cs`,
`ScenarioModule.cs` (data side), `GameBackup.cs`, `ShipConstruct.cs` (craft load/save), the whole `SaveUpgradePipeline/`
folder (51 files), `KSPUpgradePipeline.cs`, `GamePersistence.cs`, `QuickSaveLoad.cs` naming, `KSPScenarioType.cs` /
`ScenarioCreationOptions.cs`, `Versioning.cs` strings, `GameDatabase.cs` loader info, `ROCManager.cs` removed-ROC list,
`FlightGlobals.cs` persistentId checks, `FinePrint/Utilities/CompatibilityUtilities.cs` (EVA cleanup). Every ported
function cites `File.cs:line`. OrbitSnapshot is WP-07's.

**No `KSP_GOLDEN_DATASET`**: no dataset was produced; the package is gated by the round trip of original game data
(`corpus.saves`) and unit tests (`tests/test_save.cpp`).

### Gate: `corpus.saves` (all original saves and crafts of `decompiled/KSP-dev`)

`tools/save_corpus`: `ConfigNode.Load` -> `Game(root)` / `ShipConstruct.LoadShip` -> Save (R1) -> load + save again (R2).
The environment is the one that wrote each file (raw `envInfo` line, LoaderInfo flags, Breaking Ground if `ROCSeed` or a
`DeployedScience` scenario exists, Making History if `PreFailState` exists, a ROCManager if `RemovedROCs` exists, the GAME
values / nodes other systems own fed back through `GameEvents.onGameStateSave` as MessageSystem and mods do; a
`GetDefaultParameters` call first, as `MainMenu.Start` does). Pass = R2 == R1 for every file, and R1 == file or every
structural difference matches a documented rule; a file written by KSP 1.12.5 must additionally be byte-identical
once the mod-owned elements KSP's loaders drop are removed. 36 s (RelWithDebInfo).

| files | count | R1 byte-identical | R1 identical after removing dropped mod-owned elements | differences all documented | unexplained | R2 idempotent |
|---|---|---|---|---|---|---|
| `.sfs` written by KSP 1.12.5 (physprofile, default/*, RP-1*) | 61 | 1 (physprofile, the only unmodded one) | 60 (all the modded ones) | 61 | 0 | 61 |
| `.sfs` older (0.21.0 .. 1.11.2: scenarios, training, gamer, gaming2, 1111111111) | 44 | 0 | - | 44 | 0 | 44 |
| `.craft` written by KSP 1.12.5 | 80 | 49 | 27 | 80 (4: srfN stale-mesh quirk) | 0 | 80 |
| `.craft` older / hand-made layouts | 129 | 0 | - | 129 | 0 | 129 |
| total | 314 | 50 | 87 | 314 | 0 | 314 |

Differences of KSP-1.12.5 files: only mod content KSP's loaders drop without the mod (unknown `PARAMETERS` custom nodes,
`ORBIT/IDENT` from Kopernicus, `KERBAL/KerbalExt`, craft `_modVersions`) and, in 4 modded crafts, the srfN quirk below.
Differences of older files (all rules in `ExplainValue` / `CompareNodes`, counted per category in the test output): fields
KSP's Save writes that the file lacked (defaults: e.g. `VESSEL.persistentId` regenerated, `KERBAL.suit`,
`PARAMETERS/*` members added since), `CometNames` / `LoaderInfo` / stock custom parameter nodes added, numbers and
floats re-formatted with the same value (older KSP wrote floats with `ToString()`, today `G9`), enums written by name
(`Mode = 1` -> `CAREER`, `state = 0` -> `Available`), `ROSTER/CREW` and `APPLICANTS/RECRUIT` -> `KERBAL`, values the
typed loaders ignore, `PART.connected` dropped, duplicate persistentIds re-generated, craft `version` stamped with the
running game's, craft attach nodes of the old one-vector layout completed (prefab vectors, see approximations), values
re-ordered into KSP's fixed Save order. Mutation check: swapping two `PART` fields in `ProtoPartSnapshot::Save` makes
458 differences in physprofile unexplained and the gate fails.

### Unit tests (`tests/test_save.cpp`, 17 cases, 261 assertions)

- `GetDefaultParameters(SANDBOX, Normal)` saves byte-for-byte the `PARAMETERS` block of the original physprofile save;
  field defaults against `GameParameters.cs` initialisers; every preset (Easy / Normal / Moderate / Hard / Custom) x
  mode branch against `SetDifficultyPresets` and the five `SetDifficultyPreset` overrides; the first-instance vs later
  custom-parameter order; `EnableKerbalExperience` / `OnLoad` with and without `HighLogic.CurrentGame`; a value named
  like a method is a NullReferenceException; unknown custom nodes dropped.
- Upgrade pipeline: sorted script order (40, stable within each target version), 33 `PartLoader.AddPartReplacement`
  calls; a synthetic 1.0.0 save (mk1pod / SmallGearBay / LT-1 leg): 3 iterations, GAME version stamped 1.0.0 -> 1.1.0 ->
  1.12.0 (the TooEarly v1123 script drives the last stamp), `upgraded = <Name> // <Description>` on the ROOT in run
  order (mk1pod, LT-1, gear), the part renames / offsets / LT-1 rotation, source tree untouched, a second run is a no-op;
  a synthetic 1.6.0 craft (liquidEngine3 under a tank): link / attN renames, pos / attPos0 / attN offsets; wheel
  overhaul module salvage, v180 control surface (`deployAngle = range * limiter * 0.01f`, AXISGROUPS rename, prefab
  range through the hook), v1123 module replace stamping 1.12.2, a 0.20.0 file failing (all TooEarly), the
  KSPUpgradePipeline fail callback.
- Save model: `Game()` defaults and GAME order; `Game(root)` quirks (numeric Mode, `^` in Description, launch-site
  reset, CREW / APPLICANTS conversion, veteran heroes, FlightLog removing `flight` from the tree); proto snapshots
  (unknown vessel values truncated at ',', unknown part values gaining ", UNKNOWN", duplicate persistentIds, missing
  ACTIONGROUPS -> NullReferenceException, the EVA clean-up index quirk); ProtoCrewMember trait / DLC suits / Save order;
  ShipConstruct (U+00A8 description, srfN position written twice, broken links, too-old craft); GamePersistence
  (invalid file-name chars, APPEND `name000`, ABORT, BACKUP pruning with SAVE_BACKUPS, `quicksave #1`, CreateNewGame's
  scenario list == the physprofile save's 14 SCENARIO names and scenes in order, load back).
- System.Version and full Enum.Parse semantics.

All tests: ctest 16/16 (unit 115 cases / 19,427 assertions; corpus.saves new), RelWithDebInfo.

### Public shape (namespace `ksp`)

`Game` (`Game(ConfigNode* root)`, `Save(ConfigNode&)`, `Updated()`, fields named as in C#: `Title, Description, Mode,
Status, Seed, ROCSeed, startScene, editorFacility, flightState, Parameters, CrewRoster, scenarios, additionalSystems,
config, file_version_*, versionFull, versionCreated, flagURL, launchID, loaderInfo, cometNames ...`), `FlightState`
(`protoVessels`, `universalTime`, `activeVesselIdx`, ...), `ProtoVessel` / `ProtoPartSnapshot` /
`ProtoPartModuleSnapshot` / `ProtoPartResourceSnapshot` (C# field names; ConfigNode-typed members are shared `NodePtr`s
aliasing the loaded tree like C# references), `ProtoCrewMember` (shared_ptr; C# properties as `name()`, `rosterStatus()`
/ `set_rosterStatus()` firing the GameEvents), `KerbalRoster`, `GameParameters` (+ `AdvancedParams`,
`CommNet::CommNetParams`, `Expansions::Missions::MissionParams*`; `CustomParams<T>()`), `ShipConstruct` (`LoadShip`,
`SaveShip`, `PartData`), `SaveUpgradePipeline::{SaveUpgradePipeline, UpgradeScript, PartOffset, PartReplace,
ModuleReplace, NodeUtil, CreateStockUpgradeScripts}`, `KSPUpgradePipeline::Process`, `GamePersistence`,
`KSPScenarioType`, `SaveEnvironment` (all cross-system inputs), `dotnet::Version`, `dotnet::EnumParseFull`.

### Quirks a later implementer must know

1. KSP rewrites on every save: GAME `envInfo` / `modded` / LoaderInfo flags (the running game's GameDatabase),
   `ROCSeed` / `RemovedROCs` only with Breaking Ground, `PART.PreFailState` only with Making History, DLC suits reset to
   Default without the DLC, MESSAGESYSTEM and mods' GAME values / nodes come from `onGameStateSave` listeners (not from
   the loaded file; `Game::additionalSystems` is filled once and reused), FLIGHTSTATE `commNetUIModeTracking/Flight`
   are the CommNetUI statics (`SaveEnvironment`), the version stamp only changes in `Updated()` (crafts: always the
   running version).
2. Load mutates the source tree: `CompatibilityUtilities.CleanUpUnsanitaryEVAKerbals` removes crewless EVA vessels and
   rewrites `activeVessel` (its fix-up only decrements for vessels after the active one -- sic); `FlightLog.Load`
   removes `flight`. ProtoVessel's DISCOVERY / FLIGHTPLAN / CTRLSTATE / ACTIONGROUPS / VESSELMODULES members alias the
   loaded nodes.
3. Unknown values: VESSEL keeps `KSPParseable(value).value` (text before the first ','), PART writes
   `KSPParseable.Save()` ("x" becomes "x, UNKNOWN"), GAME / FLIGHTSTATE / ORBIT / KERBAL / PARAMETERS drop them,
   duplicate unknown names throw ArgumentException (Dictionary.Add). A missing ACTIONGROUPS or ORBIT node makes
   `ProtoVessel.Save` throw NullReferenceException (as in KSP).
4. GameParameters: Save order = Mono `GetMembers` (r/w properties, then fields); floats with `float.ToString()` (G7);
   custom nodes in the order of a static type list that the FIRST `GameParameters()` sorts by localized Title -- KSP's
   `MainMenu.Start` constructs one before any load, a host must too; the order also depends on the game language
   (Advanced / CommNet titles are localized, the Mission titles are raw tags). Unknown custom nodes (mods) are dropped.
   The Normal preset gets `EnableCommNet = true` after it is added to the dictionary (same object).
5. `Description`: '^' is read as '\n' and `AddValue`'s CleanupInput strips '\n' on save: line breaks are lost.
6. persistentIds go through the FlightGlobals registry (`SaveEnvironment::persistentIds`): clear it before each load
   (`GamePersistence.LoadGame` does); a duplicate gets a new random id (`Guid.NewGuid().GetHashCode()`, a hook).
7. Upgrade pipeline: every pre-1.12 save is stamped at least 1.12.0 (the v1123 script is TooEarly until then), the
   stamp is the lowest target among the scripts involved, not the game version; `upgraded` markers go to the root node
   (dropped again by `Game.Save`); RecurseNodes visits PART nodes in reverse; PartOffset keeps `rotation0` from the
   previous part when a part has no rotation; v180 needs the prefab's `ctrlSurfaceRange` (NullReferenceException when
   the part is unknown).
8. Crafts: `GetAttachNodeInfo` parses srfN mesh / vectors only for the id "srfAttach"; other ids keep the previous
   part's `attachMeshName` (a variable shared across the whole LoadShip -- sic) and the prefab's vectors; srfN writes
   the position in its third vector slot; attach nodes with duplicate ids: only the first is updated
   (`FindAttachNode`); `description` uses U+00A8 for line breaks.
9. ProtoCrewMember registers an `onGameStateCreated` listener when there is no current game (removed in the destructor,
   a C++ necessity); KerbalRoster appends its experience tables on every construction (static lists grow);
   `AddCrewMember` rejects a kerbal whose first name is a substring of an existing name; `FlightLog.GetEntries` always
   returns nothing.

### Known approximations (data side) and deferred

- Craft file form: Part.partName, modCost / modMass / modSize and the attach-node list come from the part prefab /
  module host in KSP; kspcore carries them from the file (old one-vector attN layouts are completed with the file's
  position and a zero orientation). MODULE / RESOURCE / EVENTS / ACTIONS / PARTDATA nodes of crafts and part snapshots
  are written back as loaded (KSP regenerates them from live PartModules: WP-06 / WP-13). `ShipConstruct.size` is kept
  (CalculateCraftSize needs live parts). `shipPartsUnlocked` (R&D) is not evaluated.
- `MISSIONTOSTART` is kept as the loaded node (Mission.Load/Save is the Making History package); adjuster bookkeeping
  needing AdjusterPartModuleBase objects is deferred (RemovePartModuleAdjuster(Guid) / ProtoPartModuleRepair ported).
- Live paths (WP-13 / WP-14): ProtoVessel.Load / ProtoPartSnapshot.Load / constructors from live Vessel / Part,
  FlightState() capturing live vessels (`SaveEnvironment::captureFlightState` hook), ScenarioRunner modules
  (`getUpdatedProtoModules` hook), SystemUtilities.ExpungeKerbal (logged), ProtoVessel.CreateVesselNode / CreatePartNode,
  GetSituationString, Clean.
- Not ported: FlightAutoSave (coroutine timer), ClearToSaveStatus / FlightGlobals.ClearToSave (flight scene),
  CraftProfileInfo and the `.loadmeta` files (UI metadata), `[Persistent]` attributes (WP-06 reflection),
  VesselCrewManifest / DefaultCrewForVessel (editor), CrewGenerator random names (GetNewKerbal / AddApplicant /
  roster Update), ExperienceTrait objects / effects (only the trait configs), PartResourceLibrary definitions,
  GameParameters preset colours (XKCDColors), the popup dialogs (logged).
- `SaveEnvironment` stands in for WP-05 (GameDatabase / ExpansionsLoader / PartLoader), WP-19 (FlightGlobals),
  WP-42 (HighLogic), WP-50 (GameSettings): integrations should point it at the real systems, not duplicate them.

### Commits (branch `wp04`, from main `f964447`)

| hash | content |
|---|---|
| `383104f` | save model, upgrade pipeline, craft file form, corpus.saves gate |
| `42eb750` | GamePersistence, scenario-type table, unit tests |
| (this) | docs: README, STATUS |

## WP-09: celestial bodies, atmospheres, PhysicsGlobals (branch `wp09`, merged in `29b71f6`)

Spec: `CelestialBody.cs`, `PhysicsGlobals.cs`, `Planetarium.cs`, `PSystemManager.cs` (SpawnBody/PostSetup),
`PSystemBody.cs`, `CBAttributeMapSO.cs`, `CelestialBodyScienceParams.cs`, `VesselRanges.cs`, `FlightGlobals.cs`
(GetHomeBody/GetBodyByName), `FlightIntegrator.cs:808-985`; every ported function cites its line.

### Gates (RelWithDebInfo and Debug; all bit-exact)

| gate / table | rows | numeric columns (f32) | max abs | max ULP | notes |
|---|---|---|---|---|---|
| `golden.physics_constants` / physicsglobals | 213 | value_f64 (+ exact `value` text, `type`, `source`) | 0 | 0 | prefab + Physics.cfg + CalculateValues' solarLuminosity |
| `capture.physics_drag_curve` / drag_curve_value | 2,870 | value (f32) | 0 | 0 | in-game Mono capture; inputs AeroGen.cs:72-78 |
| `golden.body_catalog` / bodies | 17 | 34 (inverseRotThresholdAltitude f32) | 0 | 0 | tidal-lock `rotationPeriod` and `solarDayLength` from the bodies' run-time Orbit (since the integration; WP-09 used a gate-only period stand-in) |
| `golden.body_catalog` / orbits | 16 | 23 | 0 | 0 | all columns since the integration (WP-09 alone: the 7 serialized elements) |
| `golden.body_catalog` / atmosphere_profile_1km | 1,072 | 5 | 0 | 0 | |
| `golden.body_catalog` / body_curves | 265 | 4 (all f32) | 0 | 0 | keys read back from the spawned bodies |
| `golden.atmosphere_curves` / profile_100m | 2,704 | 7 (tempSunMult f32) | 0 | 0 | |
| `golden.atmosphere_curves` / flight_state | 2,176 | 19 (pseudoReDragMult f32) | 0 | 0 | two CalculateConstantsAtmosphere calls per row (previous-frame density quirk) |
| `golden.atmosphere_curves` / solar_effects | 320 | 3 | 0 | 0 | |

Float32 columns: tempSunMult, pseudoReDragMult, the body_curves keys and inverseRotThresholdAltitude are native FloatCurve
evaluations or plain float stores, identical on Mono and .NET, so the harness golden is game truth for them. The managed
float code of this package (Mathf.Lerp in normalised pressure curves, CBAttributeMapSO.RGBColorSqrMag, DragCurveValue)
follows the Mono rule; DragCurveValue is gated against the in-game capture. No stock body uses normalised curves, so that
GetPressure branch has no golden. The double columns use only Pow/Sqrt/Log/Sin, which agree between Mono and .NET
Framework (VS2013 CRT, x87 Sin).

### WP-07 integration (done on main, see "Integration WP-07/WP-09"; the notes below are the pre-integration contract)

- `body_catalog/orbits` columns meanMotion, period, PeR, ApR, ObT_UT0, meanAnomaly_UT0, eccentricAnomaly_UT0,
  trueAnomaly_UT0, radius_UT0, orbitalSpeed_UT0, pos_x/y/z, vel_x/y/z: need `ksp::Orbit` (Orbit.Init/UpdateFromUT).
  Re-gate the whole table (drop the COLUMNS argument in tests/CMakeLists.txt) once WP-07 lands.
- Replace the runner's `StandInOrbitPeriod` (tools/golden_runner/celestial_env.cpp) with an adapter over `ksp::Orbit`
  passed to `CelestialSystem::Spawn(prefab, planetarium, orbitFactory)`; bodies then get the real run-time Orbit.
- What CelestialBody reads from its Orbit (`ICelestialBodyOrbit`, celestial_body.hpp): `period()`, `trueAnomaly()`,
  `radius()`, `PeR()`, `ApR()`, `vel()`, `getRelativePositionAtUT(UT)`, `getOrbitalVelocityAtUT(UT)`, `UpdateOrbit()`.
  An Orbit is built from `body.orbitDriver->orbit` (`CelestialOrbitElements`: inclination, eccentricity,
  semiMajorAxis, LAN, argumentOfPeriapsis, meanAnomalyAtEpoch, epoch, objectType) and `body.orbitDriver->referenceBody`.
- What an Orbit reads from its reference body (getters on CelestialBody): `GetGravParameter()`,
  `GetSphereOfInfluence()`, `GetRadius()`, `GetMass()`, `GetAtmosphereDepth()`, `GetName()`, `GetReferenceBody()`
  (itself for the Sun, like KSP's `referenceBody`), `GetParent()` (null for the Sun), `GetOrbitElements()`,
  `GetPosition()`, `GetPositionAtUT(UT)` (KSP getPositionAtUT, delegates to the Orbit link), `IsRotating()`,
  `IsInverseRotation()`, `GetAngularVelocity()`, `GetZUpAngularVelocity()`. The public fields keep KSP's names too
  (`gravParameter`, `sphereOfInfluence`, `Radius`, `orbitingBodies`, ...).

### Game assets (CLAUDE.md "Original game assets")

The catalog uses only the game's own data: the serialized PSystem prefab (harness extraction of
`KSP_x64_Data/sharedassets9.assets`) and `data/kerbol_body_assets.json`, produced read-only from
`decompiled/KSP-dev/KSP_x64_Data` by `tools/extract_body_assets.py` (UnityPy 1.25.4 + type trees from KSP's IL; loads
sharedassets9, its externals resources.assets / sharedassets0/2/7 and globalgamemanagers; sha256s recorded). Per body:
biome map (CBAttributeMapSO header + attribute table, 15 bodies; the bitmap stays in the asset), mini-biomes (Kerbin 49),
the PQS component (radius, mapMaxHeight) and every MapSO / CBTextureAtlasSO / Texture2D it references (height maps,
colour maps, decals, ocean textures), the scaled-space GameObject, mesh, materials (shader name) and textures (size,
format, `.resS` offset/size). `CelestialBody::assets`, `BiomeMap->origin` and `MiniBiome::origin` carry file + PathID +
name next to the consumer; a unit test checks every referenced file exists in `decompiled/KSP-dev`. PQS surface-material
textures are in the JSON index only (terrain package). Nothing is decoded yet and nothing is substituted.

### Not done in this package (and why)

- `CelestialBody.GetAtmoThermalStats` (latitude/sun temperature offset, body flux): needs native `Quaternion.AngleAxis`
  (not in kspcore; the C3 capture has it), scaled-space positions and the run-time Orbit; reference capture is
  CAPTURE-SPEC C4. Callers pass `atmosphereTemperatureOffset` in (as the harness did).
- `SetupConstants`' last statement `minOrbitalDistance = CelestialUtilities.GetMinimumOrbitalDistance(this, 1f)`
  (FinePrint, contracts only) needs the PQS highest peak and TimeWarp limits; the serialized value is kept (the harness
  skipped it as well).
- Start's TimeWarp limit-count check / `resetTimeWarpLimits`, localisation (`Localizer.Format` of names and biome
  display names), DiscoveryInfo, PQS/AFG/scaled-space wiring, `TerrainAltitude`, `GetRandomLatitudeAndLongitude`:
  engine/UI or later packages. Biome lookup (`CBAttributeMapSO.GetAtt`, MapSO pixel access) waits for the map decoder.
- The other WP-09 rows of work_packages.csv (comets, asteroids, discovery, ScaledSpace, SphereBaseSO, ...) were not in
  this assignment.
- PhysicsGlobals' Unity-only members: `blackBodyRadiation` is kept as raw Gradient keys (no native Gradient.Evaluate),
  `kerbalEVAPhysicMaterial` is a PhysicMaterial asset reference (comment with its level0 PPtr), Shader.PropertyToID.

### Surprises for later implementers

1. `PhysicsGlobals.LoadDatabase` reads the double `constructionWeightLimit` with `float.Parse`
   (PhysicsGlobals.cs:3330): in game it is 588.3989868164063, not 588.399. Ported as is.
2. Prefab values win over C# initialisers for every `[SerializeField]` (e.g. kerbalGOffset 900 not 1500,
   conductionFactor 120 not 20); `physics_globals_members.inc` therefore initialises from the level0 prefab.
3. `CelestialBody.GetTemperature` has no `atmosphere` check: an airless body returns
   atmosphereTemperatureSeaLevel - lapse * altitude below 0 m.
4. `FlightIntegrator.CalculateConstantsAtmosphere` computes the solar air mass from the density left by the previous
   frame; the first frame after entering an atmosphere uses 0.
5. compare.py matches row keys by `json.dumps` of the parsed value, so candidate files must write integral doubles as
   `100.0` (golden_json.cpp now applies the harness normalisation to every number).
6. The Sun has an atmosphere (600 km, pressure and temperature curves) and its inverseRotThresholdAltitude is raised to
   600000 by SetupConstants.
7. KSP's `CelestialBody.Awake` registers the body with the *prefab* parent's `orbitingBodies` (it runs inside
   Instantiate, before SpawnBody rewires the parent); the spawned hierarchy comes from SpawnBody only.
8. The decompiled `Planetarium.FixedUpdate` contains `PhysProf` hooks from the profiling build; they are not KSP code.
9. Bash heredocs through the agent tooling break on apostrophes; write helper scripts to files instead.

### Commits (branch `wp09`)

| hash | content |
|---|---|
| `663d4c2` | WP-09 code, generated data, asset index, 4 gates, worktree-safe CMake root |
| `13f4d07` | unit tests `tests/test_celestial.cpp`, STATUS/README |

## WP-07 Orbit (branch `wp07`, merged; `CelestialBodyRef` / `PlanetariumFrames` below were replaced by the integration)

Files: `include/ksp/orbit/{orbit,celestial_body_ref,planetarium_frames,targeting,orbit_snapshot,orbit_util,targeting_modes}.hpp`,
`src/orbit/*.cpp`, `include/ksp/unity/quaternion_native.hpp` + `src/unity/quaternion_native.cpp` (native
Quaternion.AngleAxis), `tools/golden_runner/ds_orbit_propagation.cpp`, `tests/test_orbit.cpp`.

| golden table (`golden.orbit_propagation`) | rows | f64 values | max abs | max rel | max ULP |
|---|---|---|---|---|---|
| `cases` (elements, Init outputs: meanMotion, period, ObTAtEpoch, h, eccVec, an, body mu) | 92 | 1,840 | 0 | 0 | 0 |
| `state_at_ut` (ObT, M, E, tA, pos/vel, GetOrbitalStateVectorsAtUT, UpdateFromUT fields, GetTimeToPeriapsis x2, next Pe/Ap) | 1,020 | 28,560 | 0 | 0 | 0 |
| `from_state_vectors` (UpdateFromStateVectors of the propagated state) | 1,020 | 15,300 | 0 | 0 | 0 |
| `state_vector_cases` (hand-made vectors incl. zero/radial velocity, exact escape) | 9 | 153 | 0 | 0 | 0 |
| `kepler` (solveEccentricAnomaly in all three regimes, anomaly conversions) | 281 | 1,124 | 0 | 0 | 0 |

Bit-exact in RelWithDebInfo and Debug, under kspcore's default runtime (Mono); harness tolerances unchanged. Nothing
excluded: the `patched_conics` file written by the same generator is a separate golden and WP-08's gate.

What is ported: all of Orbit.cs except `DrawOrbit` (Debug.DrawLine gizmos), including the patched-conic fields as data,
the static closest-approach / SOI solvers (`_SolveClosestApproach`, `_SolveSOI`, `_SolveSOI_BSP`,
`FindClosestPoints_old`, `RelativeStateAtUT`, `CASolutionState`) behind settable delegates, `CreateRandomOrbit*`
(UnityRandom), node and inclination helpers; Targeting.cs in full (`Orbit::FindClosestPoints` defaults to KSP's
Targeting.Intercepts path, `Orbit::LegacyOrbitTargeting` selects `FindClosestPoints_old`); OrbitSnapshot (save-file
ORBIT block); OrbitUtil minus `CalculateIntersections`. Calls that read global game state take it as a parameter:
`GetVel/GetWorldSpaceVel(activeVesselReferenceBody)` (FlightGlobals.ActiveVessel), `CreateRandomOrbitFlyBy(...,
universalTime)` (Planetarium.GetUniversalTime), `OrbitSnapshot(..., bodies)` / `Load(bodies)` (FlightGlobals.Bodies).

Interface for WP-09 (`include/ksp/orbit/celestial_body_ref.hpp`): `struct CelestialBodyRef` carries exactly what
Orbit/Targeting/OrbitSnapshot/OrbitUtil read, named as in KSP: `bodyName, Radius, gravParameter, sphereOfInfluence,
atmosphereDepth, rotates, rotationPeriod, rotPeriodRecip, initialRotation, directRotAngle, angularVelocity,
zUpAngularVelocity, inverseRotation, position, orbit (Orbit*, nullptr = no OrbitDriver), orbitDriverEnabled,
orbitingBodies, flightGlobalsIndex`, plus the CelestialBody.cs methods Orbit calls (`referenceBody()`, `GetFrameVel`,
`GetFrameVelAtUT`, `getRFrmVel`, `getRFrmVelOrbit`, `getTruePositionAtUT`, `getPositionAtUT`, `HasChild`). WP-09's
`ksp::CelestialBody` should derive publicly from it; `Orbit::referenceBody` is a `CelestialBodyRef*`. Planetarium's
frame statics are `PlanetariumFrames` (`Zup()`, `Rotation()`, `InverseRotAngle()`, `up/right/forward()`,
`ZupAtT()`, `Awake()`, `SetInverseRotAngle()` for CBUpdate's inverse-rotation branch); WP-09's Planetarium should
forward to them.

Integration with WP-09 as merged on main (`29b71f6`, which landed after this branch started from `c051578`): WP-09
defines `ksp::CelestialBody` (same member names as above, `orbit()` returning `ICelestialBodyOrbit*`,
`orbitDriver->enabled`), `ksp::Planetarium` (static `Zup`, `Rotation()`/`up()`/`right()` through `fetch`,
`ZupAtT(UT, const CelestialBody&, CelestialFrame&)`) and leaves `body_catalog/orbits` derived columns ungated pending
WP-07. The names do not collide (`CelestialBodyRef`, `PlanetariumFrames` vs `CelestialBody`, `Planetarium`), so the
merge compiles side by side; the follow-up glue is small: (1) `Orbit::referenceBody` and the helpers take
`CelestialBody*` (delete `CelestialBodyRef`); (2) `BodyOrbit()` (the single access point to a body's own orbit, 5 call
sites) returns the `ksp::Orbit` behind the body's `ICelestialBodyOrbit` link; (3) an `ICelestialBodyOrbit` adapter
owning a `ksp::Orbit` (Orbit's fields `period`, `radius`, `vel`, `trueAnomaly` clash with the interface's method names,
so Orbit cannot implement it directly), passed as WP-09's `orbitFactory`; (4) `PlanetariumFrames` forwards to
`Planetarium::Zup` / `Rotation()` / `ZupAtT` (6 call sites; keep the Awake(0) default when `Planetarium::fetch` is
null); (5) re-gate `body_catalog/orbits` in full and drop the runner's `StandInOrbitPeriod`; (6) on the merge keep
main's `golden_json.cpp` (WP-09 made the same Normalize fix under another name).

Shared fixes made for WP-07 (WP-01/runner files): `dotnet::Math::Min/Max` (NaN-propagating like .NET); `UtilMath.AngleBetween`
and `Vector3d/Vector2d.Angle/AngleBetween` now call `dotnet::Math` (were `std::atan2/std::acos`, which is the UCRT, not
the VS2013 CRT KSP uses); the golden writer normalises number text like the harness (`ExactNumber.Normalize`: ".0"
on integral values, lower-case "e"), without which float key columns (`kepler` e/M, `UT`) did not match.
(Since the WP-03 merge only `QuaternionD.Angle` (std::acos/std::min) of this list remains; WP-03 moved the others to System.Math.) Still using `<cmath>` in WP-01 code that Orbit does not call (noted, not changed): `UtilMath.Flatten/RoundToPlaces`
(std::pow), `UtilMath.ASinh/ACosh/ATanh/Sech/Csch/Coth` (std::log/cosh/sinh), `QuaternionD.Angle` (std::acos).

Not done in WP-07, and why:
- **TransferMath.cs** (1,870 lines; Lambert solvers, Hohmann/ejection/correction burns): most entry points take
  `TransferDataSimple` (WP-43, tier C UI) and `SafeAbortBackgroundWorker` (WP-03); the pure-maths part (LambertSolver,
  LambertRHG/LamGXL/LamGTL, Root/Bisect, DV helpers) can be ported on its own later. No golden exists.
- **OrbitUtil.CalculateIntersections**: returns `IntersectInformation`, a WP-03 type (being ported in parallel); a
  short wrapper around FindClosestPoints/GetDTforTrueAnomaly once it lands.
- **DirectionTarget, ITargetable**: need PositionTarget (WP-10), Vessel and FlightGlobals.
- **C ABI for orbits**: not added. A useful ABI hands out orbits that reference body handles of the simulation context;
  that needs WP-09/WP-10. A standalone mu/radius stopgap would have to be deprecated, so ABI v1 is unchanged.

Known deviations and risks (WP-07):
- Targeting's object pools: KSP's Interval pool is process-wide; an interval list of 2 or 3 entries makes KSP release an
  interval twice, so two later borrows can alias. kspcore reproduces that inside one search (per-search LIFO pool)
  but not across searches (a double release in one search could alias two intervals in the next one in KSP). Other
  pooled state (stale Sample info, Interval min/max flags) is never read on any path.
- Targeting.Interval.FindRoot / find_region_edge loop forever in KSP when a NaN reaches the bisection; kspcore throws
  `std::runtime_error` after 10,000 iterations instead.
- Targeting and OrbitUtil have no golden: they are covered by unit tests (FindClosestPoints agrees with a brute-force
  minimum distance within 1e-6 on four conic pairs) and will be exercised by WP-08's `patched_conics` golden.
- Native `Quaternion.AngleAxis` (two rare Orbit paths) is bit-exact on the 84 captured rows (MSVC CRT sinf/cosf).
- Debug.Log / Console.WriteLine in Orbit go to `ksp::log::Info` without the C# stack traces; KSP's
  `!Thread.CurrentThread.IsBackground` guards are treated as "main thread" (always log).

## WP-03 Event bus, FSM, misc core utilities (branch `wp03`, merged)

State: done (40 of the 42 package files fully or partly ported; see "skipped"). **No `KSP_GOLDEN_DATASET`**: there is
no harness golden for WP-03 and none was added, so the package is **gated by unit tests**, including
`test_wp03_reference.cpp`, which replays 6,437 values computed by **KSP's own IL** (the harness's Assembly-CSharp build)
on .NET Framework 4.8 (`tools/wp03_reference`, data `tests/data/wp03_reference.tsv`): 0 mismatches. A mutation check
(20 corrupted cells) confirmed every row kind is really compared. Every ported function cites `File.cs:line`.

| area | ported (C++ location) | verification |
|---|---|---|
| C# delegates | `ksp::Delegate<R(Args...)>`: Bind/Lambda/Static, C# equality, Target, multicast `+ - += -=`, invocation (`core/delegate.hpp`); `Callback<...>` | unit |
| Event bus | BaseGameEvent (+FindEvent), GameEventsBase, EventVoid, EventData<1..4>, EventDataModifier, Event{Double,Float,Int}{Max,Min}, EventValueComparison, EventValueOperation, EventValueWrapper (`events/event_data.hpp`) | unit + KSP IL (call orders R1-R4, exception isolation, modifier chain, NaN/signed-zero folds, hidden eventName) |
| GameEvents registry | all **469 events** (383 EventData, 80 EventVoid, 6 EventDataModifier: 3 static + 3 in `Modifiers`), 7 nested classes, the 5 nested structs, FindEvent; generated by `tools/wp03/gen_game_events.py`; 109 forward-declared KSP/Unity types (`events/game_event_types.hpp`) | KSP IL: member path, event-name string and C# type of all 469 match (`GameEventsRegistry()`) |
| FSM | KerbalFSM, KFSMState, KFSMEvent, KFSMTimedEvent, KFSMUpdateMode/Callback/EventCondition/StateChange (`fsm/kerbal_fsm.hpp`); UnityEngine.Time shim (`core/unity_time.hpp`) | unit (Time.time is native: cannot run on .NET) |
| KSPUtil | DefaultDateTimeFormatter (all 16 IDateTimeFormatter members + 10 GetUT*/GetTimeSpan*), IDateFormatterExtensions, dateTimeFormatter get/set, CacheLocalStrings, LocalizeNumber, PrintSI, Print{Coordinates,Latitude,Longitude}, the 12 Print* forwarders, PrintModuleName, PrintLocalizedModuleName, PrintSpacedStringFromCamelcase, GetOrCreatePath, GenerateFilePathWithDate, MoveFile, StripFileExtension, SanitizeString/Filename/InstanceName, CheckDOSName, CalculateFolderSize, CheckVersion x2, ReplaceString, Heading/Bearing x4, PrintCollection, AppendValueToString, Matrix4x4 Add/Diag/ToDiagonalMatrix/OuterProduct (9), DeepCompare, GetPartName, GetPartInfo, GetAttachNodeInfo x4, GetLinkID, GenerateSuperSeed, WriteArray, ParseArray, WriteVector(Vector3d)/WriteQuaternion(QuaternionD), ParseVector3d x2, ParseQuaternionD x2, per-component ParseVector2/3/4/ParseQuaternion (`util/ksp_util.hpp`) | KSP IL: 3,776 date/time strings (Kerbin + Earth calendars, 30 UTs incl. negative/NaN/Inf/1e300, every option), cache sequences, shared date holder, SI, coordinates, LocalizeNumber, strings, versions, craft ids, seeds, vectors |
| Localizer | KSP.Localization.Localizer tag table, Format (0/params), GetStringByTag, TryGetStringByTag, TranslateBranch, AddTagValuesFromNode/ForLanguage, unescaping; Lingoona.Grammar managed fallback (`localization/localizer.hpp`) | KSP IL: Format/Lingoona rows; the full dictionary.cfg loads to the same 12,030 tags; the built-in English subset equals dictionary.cfg |
| UtilMath (WP-01 file, completed) | added Max/Min(params), MaxFrom/MinFrom (8 overloads), WrapAround(int) exceptions; fixed to System.Math semantics: ASinh..Coth (CRT Log/Cosh/Sinh), Flatten (Pow, Sign(NaN) throws), RoundToPlaces(double) (Pow), AngleBetween (Atan2), BSPSolver (Math.Min NaN/-0 rules); citations corrected to the real lines | KSP IL: 852 UtilMath values; golden.math_shim.f64 still 0 ULP |
| .NET semantics | `dotnet::ListSort` (ArraySortHelper introsort), `dotnet::Dictionary` (.NET enumeration order, free-slot reuse), `dotnet::EnumToString/GetName/GetNames/Parse` (+[Flags], duplicates), `Math::Min/Max`, UTF-16 helpers | KSP IL / .NET: sort permutations (n = 17..1000 with ties), dictionary orders, enum strings |
| Collections / extensions | ArrayExtensions.IndexOf, ListExtension (Shuffle, AddUnique(Range), MaxAt/MinAt, SetRange x4, ContainsId x3, OrderByAlphaNumeric x2), BiDictionaryOneToOne, DictionaryValueList, ListDictionary, EnumExtensions (Description, displayDescription, GetDescriptions, ToStringCached), StringExtensions.GetHashCode_Net35, StringBuilderCache.Format, MinMaxColor, IntersectInformation, DelegateExtensions/CallbackUtil.Clear | unit + KSP IL (hash, Format, DVL, OrderByAlphaNumeric, enums) |
| Simplex | Simplex (instance, octaves, seed), SimplexNoise (static) (`util/simplex.hpp`) | KSP IL: 88 noise values (default tables); seeding (UnityEngine.Random is native) by reasoning on the bit-exact UnityRandom |

Tests: `unit` now 70 doctest cases / 12,156 assertions (WP-03: test_events 17 cases, test_util 14, test_wp03_reference 1
with 6,437 compared values). All 10 ctest tests pass in RelWithDebInfo (13.6 s) and Debug (27.1 s).

**Mono-rule float32 paths, verified by reasoning only** (the .NET reference cannot judge them; candidates for a future
in-game capture): KSPUtil.HeadingRadians/HeadingDegrees/BearingRadians/BearingDegrees, UtilMath.WrapAround(float),
RoundToPlaces(float), BSPSolver(float), MaxFrom/MinFrom float overloads (no arithmetic), MinMaxColor.GetLerp
(Color.Lerp), CalculateFolderSize (float sum), EventFloatMax/Min (comparisons only), LocalizeNumber(float) text.

**Skipped, and where it goes**
- Unity-bound KSPUtil members -> shell / later packages: FindInPartModel, recurseModels, GetTransformPathToRoot,
  GetTransformIndexPathToRoot, FindTransformAtIndexPath, HasAncestorTransform, HasDescendantTransform,
  FindComponentsImplementing, FindTagsInChildren (Transform/GameObject: UE5 shell, tier C), RemoveNonHighlightableRenderers
  (rendering, WP-48), ClampRectToScreen (UI), GenerateTexture2DArray (rendering), GetPartByCraftID (needs Part: WP-13),
  ApplicationFileProtocol (platform), GetActivator/ObjectActivator (reflection; WP-06 generated factories),
  SystemDateTime.DateTimeNow (only used by GenerateFilePathWithDate, which reads the local clock directly),
  GetRelativePath (System.Uri escaping; only the craft-thumbnail UI uses it).
- CallbackUtil coroutines (DelayedCallback x12, WaitUntil, DoUntil, HoldUntil, LazyCallback): Unity coroutines ->
  the game-loop scheduler (tier B, with the scene/loop package). Only CallbackUtil.Clear is ported.
- SafeAbortBackgroundWorker: .NET BackgroundWorker + Thread.Abort -> a C++ job/stop_token design where it is used
  (tier B).
- LocalizeNumber with formats other than G/F/R ("N0", "0.0", "0.###E+0", ...): throws std::logic_error until ksp::dotnet
  ports Mono's NumberFormatter (custom/N/E/P formats; needed by PartModule GetInfo strings, WP-06+).
- Localizer language switching from GameDatabase / buildID64.txt, mission tags and the native Lingoona engine: WP-51.

**Surprises (a later implementer must know)**
1. **Fire order is REVERSE registration order** (not registration order): every event class walks its snapshot from
   the end. EventDataModifier's DefaultModifier is registered first and therefore runs last.
2. **Re-entrancy**: each event has ONE shared snapshot list. Listeners removed or added during Fire do not affect the
   current round, but a listener that fires the same event again refills the shared snapshot; the outer loop then
   continues on the new list (can repeat or skip listeners), and if the new list is shorter the outer index throws
   ArgumentOutOfRangeException out of Fire (confirmed on KSP's IL: R2-R4).
3. Remove takes the LAST matching registration; Add of a static-method delegate throws NullReferenceException
   (`evt.Target.GetType()`), so the `originator == null` cleanup branch in Fire is dead code.
4. EventValueComparison/Operation.Update have no try/catch (an exception escapes and leaves the nesting counter at 1);
   their debug logs print an empty name (`new string eventName` / `gameEventName` are never assigned; KSP IL shows null).
5. GameEvents has duplicate event names (the later declaration wins in FindEvent): onPartCouple/onPartDeCouple,
   onPartCoupleComplete/onPartDeCoupleComplete, OnFlightState x2, OnApplicantHired (OnCrewmemberHired/Sacked),
   OnEVAConstructionModePartAttached (Attached/Detached), "Active Node Changed" x2.
6. KerbalFSM.updateFSM keeps its loop index across a state change, so it continues on the NEW state's event list.
7. KSPUtil date formatting: PrintTimeLong has no space before units ("1Hour"); PrintTimeCompact never shows years;
   PrintTime(0, n, explicitPositive: true) is "+ "; a fresh formatter's PrintDate(t, false) for t in [0, 60) and
   PrintDateCompact/PrintTimeStampCompact for floor(t) == -1 return null (kspcore ""); a UT too large for (int)
   (e.g. 1e300) throws OverflowException from Math.Abs(int.MinValue).
8. UtilMath.BSPSolver does not take |f| of the starting value: a negative start never moves (KSP IL confirms).
9. ListExtension.MaxAt/MinAt always throw for non-empty lists (loop starts at list[Count]); IntersectInformation's
   constructor swaps the two distances; DelegateExtensions.Clear / CallbackUtil.Clear do nothing.
10. Simplex.seed rewrites a STATIC permutation table shared by every Simplex and reseeds UnityEngine.Random globally;
    fastfloor(0) = -1.
11. SanitizeInstanceName("mk1pod (Clone)") = "mk1pod " (the "(clone" match overrides the " (" match).

**Deviations (documented, deliberate)**: C# null strings returned by the formatter caches are ""; FindEvent finds
nested-class events without the C# lazy class initialisation; a destroyed C++ event unregisters its name; log texts use
`e.what()` and C++ type names where C# prints Exception.ToString() / Type names; char.IsUpper/ToUpper cover Latin-1,
Greek and Cyrillic only; OrderByAlphaNumeric uses `dotnet::CompareCulture` (exact for ASCII); `int.MinValue % -1`
throws OverflowException as on .NET (unverified on Mono); List.Sort's depth limit uses Count unless the caller passes
the List capacity.

**Shared files touched** (merge notes): `CMakeLists.txt` (KSP_PROJECT_ROOT searches upwards for harness/, so git
worktrees under core/.worktrees build), `include/ksp/config/config_node.hpp` (NullReferenceException moved to
`core/exceptions.hpp`), `include/ksp/dotnet/math.hpp` (Math.Min/Max), `include/ksp/math/util_math.hpp` +
`src/math/util_math.cpp` (completion and System.Math fixes above), `src/config/kspparseable.cpp` (ParseVector4 /
ParseQuaternion warning text with the actual separator), `src/CMakeLists.txt`, `tests/CMakeLists.txt`, README.md.

Commits on `wp03` (from main `c051578`, not merged): `05e84fc` delegates + event bus + GameEvents registry, `072e37e`
KerbalFSM, `ecd9075` KSPUtil/Localizer/UtilMath/.NET semantics/collections/Simplex, `b0f9ba3` KSP-IL reference run,
`2a2bbb7` KSPUtil double vector helpers + README, then the docs commit (STATUS.md, PROGRESS-wp03.md).

## Integration WP-07/WP-09, then the WP-03 merge (main)

`wp07` merged into main (`d2e66e7`, on top of the WP-09 merge `29b71f6`), then the six integration steps WP-07 listed.

### Merge conflicts and resolutions

| file | resolution |
|---|---|
| `src/CMakeLists.txt`, `tests/CMakeLists.txt`, `tools/golden_runner/CMakeLists.txt` | union of both packages' sources/tests; in tests/ main's `KSPCORE_PROJECT_ROOT` definition kept (WP-07 had only re-indented that line) |
| `tools/golden_runner/golden_json.cpp` | main's (WP-09) kept: its `Normalize` and WP-07's `NormalizeNumber` were the same `ExactNumber.Normalize` (lower-case "e", ".0" on integral text). Both gates bit-exact with it |
| `STATUS.md` | both sections kept, WP-07 after WP-09; test table merged |
| `README.md` (auto-merged) | WP-07's worktree note (`-DKSP_PROJECT_ROOT`) replaced: WP-09's root CMake walk-up (first parent containing `harness/compare/compare.py`) is the one mechanism; the main checkout and `.worktrees/<wp>` configure without flags, `-DKSP_PROJECT_ROOT` still overrides |

### What changed

- **Orbit on `ksp::CelestialBody`.** `Orbit::referenceBody`, `closestEncounterBody` and every Orbit/OrbitSnapshot/OrbitUtil
  parameter are `CelestialBody*`; `CelestialBodyRef` (and its .cpp) is deleted: WP-09's CelestialBody already had every
  member Orbit reads under KSP's names, plus the frame methods (GetFrameVel(AtUT), getRFrmVel, getTruePositionAtUT,
  getPositionAtUT, HasChild, referenceBody). Added `CelestialBody::getRFrmVelOrbit(const Orbit&)` (CelestialBody.cs:633).
  `orbitDriverEnabled` is `orbitDriver->enabled`.
- **`BodyOrbit(body)`** (orbit.hpp) is `body->orbit()->GetOrbit()`: `ICelestialBodyOrbit` gained `virtual Orbit* GetOrbit()`
  (test doubles return null). Still the single access point (5 call sites).
- **Adapter.** `CelestialBodyOrbitAdapter` (`include/ksp/celestial/celestial_body_orbit.hpp`) owns the body's `ksp::Orbit`
  and implements `ICelestialBodyOrbit` (needed because Orbit's fields period/radius/vel/trueAnomaly clash with the
  interface's method names). It is the OrbitDriver as far as bodies use it: construction = OrbitDriver.Start in UPDATE
  mode (deserialized elements + referenceBody, `Init()`, `updateFromParameters()`), `UpdateOrbit()` =
  `updateFromParameters()` (`UpdateFromUT(Planetarium UT)`, then `body.position = parent.position + pos.xzy`, or for the
  reversed Kerbin `parent.position = body.position - pos.xzy`). `MakeCelestialBodyOrbit` is
  `CelestialSystem::Spawn`'s default factory (a factory may still return null to test KSP's `orbit == null` branches).
- **One Planetarium.** `PlanetariumFrames` is deleted; Orbit calls `Planetarium::Zup`, `up()`, `right()`, `ZupAtT()` (KSP's
  names). Kept WP-07's standalone behaviour: `Zup` starts at Awake(0)'s frame (KSP: zero until the first Awake), and
  `Rotation()`/`InverseRotAngle()` answer with Awake(0)'s values when `fetch` is null (KSP: NullReferenceException).
  The setters throw `NullReferenceException` without an instance (WP-09 dereferenced null). `Planetarium::AwakeZup(a)` /
  `AwakeRotation(a)` expose Awake's frame statements.
- **Golden runner.** `StandInOrbitPeriod` removed; `CelestialEnv` spawns with the real Orbit and then calls
  `UseHarnessFrame()` (below); `orbit_propagation` now uses the shared `CelestialEnv` (the harness's OrbitGen uses the same
  KspEnv as BodiesGen) instead of its own body rebuild from the serialized JSON; `body_catalog` writes every orbits column
  from `BodyOrbit(cb)`; the `COLUMNS` exclusion on `body_catalog/orbits` is gone.
- **Tests.** test_orbit ported to CelestialBody (an `OrbitLink` test double attaches test-owned Orbits) and to the one
  Planetarium; test_celestial spawns with real orbits; new `tests/test_celestial_orbit.cpp` (below).

How a caller gets a body's position now: `Planetarium p; p.Awake(); auto sys = CelestialSystem::Spawn(KerbolSystemPrefab(),
p); CelestialBody* mun = sys->GetBodyByName("Mun");` then `mun->getPositionAtUT(UT)` (orbit position relative to Kerbin,
Unity axes, + Kerbin's current position), `mun->getTruePositionAtUT(UT)` (chained Mun -> Kerbin -> Sun),
`mun->GetFrameVelAtUT(UT)`, `BodyOrbit(mun)->getRelativePositionAtUT(UT)`; `p.FixedUpdate()` advances time and moves
every body (`mun->position`).

### Gate results after the integration (RelWithDebInfo and Debug, fresh; harness tolerances unchanged)

| gate / table | rows | compared columns | max abs | max ULP |
|---|---|---|---|---|
| `golden.body_catalog` / bodies | 17 | 51 | 0 | 0 |
| `golden.body_catalog` / orbits | 16 | 25 + name key (parent, reverse, 7 elements, meanMotion, period, PeR, ApR, ObT/M/E/v at UT 0, radius, orbitalSpeed, pos_xyz, vel_xyz) | 0 | 0 |
| `golden.body_catalog` / atmosphere_profile_1km | 1,072 | 5 | 0 | 0 |
| `golden.body_catalog` / body_curves | 265 | 7 | 0 | 0 |
| `golden.orbit_propagation` / cases | 92 | 22 | 0 | 0 |
| `golden.orbit_propagation` / state_at_ut | 1,020 | 28 | 0 | 0 |
| `golden.orbit_propagation` / from_state_vectors | 1,020 | 15 | 0 | 0 |
| `golden.orbit_propagation` / state_vector_cases | 9 | 18 | 0 | 0 |
| `golden.orbit_propagation` / kepler | 281 | 4 | 0 | 0 |

All 15 ctest tests pass; every other gate (WP-01/02/09: math_shim f64/f32, libm_mono, unity_random, confignode,
curves_eval, physics_constants, physics_drag_curve, atmosphere_curves, corpus) unchanged at 0 ULP / exact.

### The harness frame (why the runner calls `UseHarnessFrame`)

The PSystem prefab ships the Mun and Minmus with `inverseRotation = 1`. KSP's `CelestialBody.CBUpdate` (ported
faithfully) then takes its inverse-rotation branch in the first CBUpdate pass: `Planetarium.InverseRotAngle = (230 - 0) % 360`,
`Zup = PlanetaryFrame(0, 90, 230)`, `Rotation = ...`. The harness's CBUpdate rehost
(`harness/src/KspGolden/Rehosted/CelestialBodyLifecycle.cs:106-147`) leaves that branch out ("frame bookkeeping"), so
every golden orbit state is in the Awake frame. With the faithful spawn, `pos_x/pos_y/vel_x/vel_y` of every body
updated after the Mun (15 of 16; Kerbin, the root of the pass, was updated first) came out turned by exactly 130 degrees
(= -230), and orbit_propagation's x/y columns and dependent elements failed the same way. `CelestialEnv::UseHarnessFrame()`
restores the Awake frame (serialized inverseRotAngle 0) and redoes `UpdateOrbit()` for every body in the CBUpdate order;
nothing else changes, no tolerance was touched. The library keeps KSP's behaviour (Spawn leaves InverseRotAngle at 230);
the real game clears these flags on scene setup (`PSystemSetup.OnSceneChange` / `SetSpaceCentre`, not ported yet).

### Integration unit test (`tests/test_celestial_orbit.cpp`, 2 cases, 934 assertions)

1. Faithful spawn: every body with an OrbitDriver has its adapter and `BodyOrbit`, parented like the driver; InverseRotAngle
   is 230 and Zup = PlanetaryFrame(0, 90, 230) bit for bit; frame-free orbit state (elements, meanMotion, period, PeR,
   ApR, ObT, anomalies, radius, pos_z, vel_z) equals the golden bit for bit (orbitalSpeed within 4e-16 relative: |vel| of
   rotated components); Kerbin's pos/vel bit for bit, the others' after undoing the frame within 8e-15 relative; all 7
   tidally locked bodies (Mun, Ike, Laythe, Vall, Tylo, Bop, Pol): rotationPeriod == orbit period == golden; rotPeriodRecip
   and solarDayLength == golden for every body; OrbitDriver positions (Kerbin at the origin, Sun = -Kerbin's orbit
   position, every other body = parent + orbit pos.xzy); Mun getPositionAtUT / getTruePositionAtUT / GetFrameVelAtUT equal
   the explicit Mun -> Kerbin -> Sun sums at UT 0, 1234.5, half a Mun period, 3e6, 1e8; one Planetarium.FixedUpdate moves
   time to 0.02 and the Mun with it.
2. Harness frame (as the runner): every orbits column of every body bit for bit; world positions implied by the golden at
   UT 0 (Kerbin, Sun, Mun, Minmus, Duna, Ike, Laythe) bit for bit; the Mun at UT 0 through getPositionAtUT /
   getTruePositionAtUT within 4e-15 relative of the golden-derived values; after 1, 2, 10 periods back at the golden UT-0 position,
   opposite after half a period and a quarter turn prograde after a quarter (1e-9 relative).

### Commits (main)

| hash | content |
|---|---|
| `d2e66e7` | merge `wp07` (conflict resolutions above) |
| `f93380a` | Orbit on CelestialBody, one Planetarium, CelestialBodyOrbitAdapter, Spawn attaches real orbits, runner (StandIn removed, shared env, harness frame), ported unit tests |
| `2fae513` | gate the whole body_catalog orbits table |
| `93cd8ff` | integration unit test |
| `e4d982c` | docs: this section, README bodies/orbits API |
| `b9d4578` | merge `wp03` (see "WP-03 merge") |

### WP-03 merge

`wp03` (6 commits on `c051578`) merged after the integration. Conflicts and resolutions:

| file | resolution |
|---|---|
| `CMakeLists.txt` | one root mechanism: main's walk-up (WP-09; WP-03 had written the same loop) to the first parent containing `harness/compare/compare.py`; main checkout and worktrees configure without flags; `-DKSP_PROJECT_ROOT` overrides |
| `include/ksp/dotnet/math.hpp` | one `Math::Max/Min` implementation: WP-03's (double, float and int overloads; double code identical to WP-07's: NaN first argument returned, `val1 > val2 ? val1 : val2` otherwise, so NaN in either argument gives NaN and ties return val2). Orbit/Targeting (double) and WP-03 callers compile unchanged |
| `src/math/util_math.cpp` | union: WP-03's completion (WrapAround, Max/Min(params), System.Math in Flatten/RoundToPlaces/ASinh..Coth, BSPSolver with Math.Min) plus WP-07's `AngleBetween` via `dotnet::Math::Atan2` (both sides had made that same change) |
| `include/ksp/config/config_node.hpp` (auto) | `NullReferenceException` now lives in `ksp/core/exceptions.hpp` (WP-03); config_node.hpp includes it. Orbit, CelestialBody and Planetarium now include `core/exceptions.hpp` instead of config_node.hpp |
| `src/CMakeLists.txt`, `tests/CMakeLists.txt` | union; the unit-test target keeps `KSPCORE_HARNESS_DIR` / `KSPCORE_PROJECT_ROOT` and gains WP-03's `KSP_DICTIONARY_CFG` |
| `README.md`, `STATUS.md` | union (WP-03 rows/sections kept, WP-03 section after WP-07's) |
| `tools/wp03/__pycache__/*.pyc` | dropped from the tree (a Python cache file was committed on wp03); `__pycache__/` added to .gitignore |

After the merge (fresh, RelWithDebInfo and Debug): 15/15 ctest tests pass; `unit` 98 cases / 19,187 assertions including
the WP-03 KSP-IL reference test (11,032 assertions); all 41 compared golden/capture tables 0 ULP / max abs 0, incl.
`golden.math_shim.f64` (UtilMath / Vector3d angle code touched by both WP-03 and WP-07).

### Deferred / for later packages

- `Planetarium.Orbits` (the OrbitDriver list patched conics read): done in WP-08 (see "WP-08").
- The harness's bodies never move (its OrbitDriver rehost only calls `Orbit.UpdateFromUT`), kspcore's spawn moves them
  (`updateFromParameters`); any golden column that depends on `CelestialBody.position` (patched_conics:
  getTruePositionAtUT, GetFrameVel chains) must take that into account in its runner.
- Vessel OrbitDrivers (TRACK_Phys / IDLE, CheckDominantBody, SOI transitions) are WP-10; the adapter throws for
  non-UPDATE modes (every stock body uses UPDATE).
- `GetAtmoThermalStats` now has its Orbit and native AngleAxis inputs; it still needs scaled-space transforms.
- Scene setup (`PSystemSetup.OnSceneChange` / `SetSpaceCentre` / `SetFlight`: clear inverseRotation/reverse, home body
  inverse rotation) is not ported; until then a spawned system keeps the prefab's Mun/Minmus inverse-rotation state.

## WP-08 patched conics, SOI transitions, maneuver nodes (branch `wp08`)

Spec: `PatchedConics.cs`, `PatchedConicSolver.cs`, `ManeuverNode.cs`, `Trajectory.cs`, `OrbitDriver.cs`,
`Planetarium.cs` (orbits), `FlightGlobals.cs:1311-1330` (getMainBody/inSOI), `TimeWarp.cs` (fixedDeltaTime, CurrentRate,
updateRate, assumeWarpRate), `PatchedConicRenderer.cs:200-204` (nodeRotation), `ManeuverGizmo.cs:94-112` (patch state),
`GameSettings.cs:999-1000`; every ported function cites its line.

### What is ported

| file | contents |
|---|---|
| `include/ksp/orbit/patched_conics.hpp` | `PatchedConics`: SolverParameters, the four settable delegates (`CalculatePatch`, `CheckEncounter`, `EncountersBody`, `GetClosestApproach`) with KSP defaults, `AngleWrap`, `TAIsWithinPatchBounds`; `IsBackgroundThread` (thread_local) for KSP's `Thread.CurrentThread.IsBackground` branches |
| `include/ksp/orbit/patched_conic_solver.hpp` | `PatchedConicSolver` (whole class): Awake (constructor), OnDestroy, Start coroutine (`StartMoveNext`), Update patch loop, UpdateFlightPlan / CheckNextManeuver, FindPatchContainingUT, FindManeuverIndex, Add/RemoveManeuverNode, Increase/DecreasePatchLimit, OnGameSettingsApplied, OnReferenceBodyChange, Load/Save, LastActivePatch |
| `include/ksp/orbit/maneuver_node.hpp` | `ManeuverNode` (whole class) + `ManeuverGizmoState` (ManeuverGizmo's patchBefore/patchAhead/prevUT/rotation as plain data); `UpdateNodeRotation` = PatchedConicRenderer's nodeRotation |
| `include/ksp/orbit/orbit_driver.hpp` | `OrbitDriver` (OrbitDriver.cs minus the renderer/DrawOrbit): Start (Planetarium.Orbits.Add), OnDestroy (Remove), FixedUpdate, UpdateOrbit (UPDATE / TRACK_Phys / IDLE), SetOrbitMode, CheckDominantBody, TrackRigidbody, updateFromParameters, RecalculateOrbit, OnRailsSOITransition, Invoke("unlockFrameSwitch") as `RunDelayedCalls`; the vessel is reached through `IOrbitDriverVessel` |
| `include/ksp/orbit/trajectory.hpp` | `Trajectory` (whole class, logic only; `GetColors` included) |
| `include/ksp/orbit/time_warp.hpp` | TimeWarp subset (WP-10 owns the class): Modes, curr_rate, warpRates, WarpMode, CurrentRate, fixedDeltaTime (Mono float rule), updateRate |
| `include/ksp/celestial/flight_globals.hpp` | FlightGlobals subset (WP-19 owns the class): `bodies` (registered by `CelestialSystem::Spawn`), `overrideOrbit`, `getMainBody` |
| `include/ksp/core/unity_time.hpp`, `include/ksp/util/ksp_util.hpp` | `Time::timeScale`, `Time::fixedDeltaTime`; `GameSettings::CONIC_PATCH_LIMIT = 3`, `ALWAYS_SHOW_TARGET_APPROACH_MARKERS = false` (SetDefaultValues; settings.cfg loading is the GameSettings port's) |

**Planetarium.Orbits** (deferred by the integration, done here): `Planetarium::orbits` (`std::vector<OrbitDriver*>`,
Planetarium.cs:60) and the static `Planetarium::Orbits()` (null without an instance, Planetarium.cs:84).
`OrbitDriver::Start` appends, `~OrbitDriver` (OnDestroy) removes. Bodies now have a real `OrbitDriver`:
`CelestialBodyOrbitAdapter` owns one (`driver`; `orbit` is a reference to `*driver.orbit`), sets `QueuedUpdate = true`
(CelestialBody.cs:483) and calls `driver.Start()` in its constructor, so `CelestialSystem::Spawn` registers the 16 body
drivers in spawn order exactly as KSP's OrbitDriver.Start and the harness's KspEnv do. The adapter's `UpdateOrbit` is
`driver.UpdateOrbit()`; the scene-visible `reverse` flag stays in `CelestialBodyOrbitDriver` (FindRootBody reads it,
scene setup writes it) and is copied into the driver before each update. `CelestialSystem::Spawn` also registers
`FlightGlobals::bodies`. WP-07/WP-09 gates and tests are unchanged (bit-exact).

### Gates (RelWithDebInfo and Debug; harness tolerances unchanged; capture tolerance 0 ULP)

| gate / table | rows | compared columns | max abs | max ULP | missing / extra rows |
|---|---|---|---|---|---|
| `golden.patched_conics` / patches | 11 (5 cases) | 19 + key (referenceBody, transitions, StartUT/EndUT/UTsoi, 7 elements, PeR, ApR, closestEncounterLevel/Body, ClAppr, UTappr) | 0 | 0 | 0 / 0 |
| `capture.orbit_rails` / initial | 6 | 13 (body, 7 elements, meanMotion, period, ObTAtEpoch, mu, spawned) | 0 | 0 | 0 / 0 |
| `capture.orbit_rails` / samples | 3,465 | 25 (segment, UT, warp_rate f32, ref_body, updateUT, pos/vel/relpos_at_UT xyz, 7 elements, ObT, meanAnomaly, trueAnomaly, update_mode) | 0 | 0 | 0 / 0 |
| `capture.orbit_rails` / frames | 450 | 12 (UT, Planetarium.Zup X/Y/Z, inverseRotAngle, frame_is_rotating) | 0 | 0 | 0 / 0 |
| `capture.orbit_rails` / segments | 5 | 8 (rate_index, achieved_rate, frames, UT_start/end, Time.fixedDeltaTime, TimeWarp.fixedDeltaTime, timeScale) | 0 | 0 | 0 / 0 |
| `capture.orbit_rails` / soi_events | 4 (the "HC8" probes) | 10 (from, to, 7 elements of the new patch, warp_rate) | 0 | 0 | 0 / 0 |

`capture.orbit_rails` covers 6 probes and the Mun/Minmus orbits over 1.3e6 game seconds (warp 1x, 100x, 1,000x,
10,000x, 100,000x) in the rotating Planetarium frame, the 4 on-rails SOI transitions of the probes (Kerbin->Mun at
1,000x, Mun->Kerbin, Kerbin->Sun at 10,000x, Kerbin->Mun again) with their BSP-located transition UTs, and the
Mun-encounter probe's destruction after sample 315: all bit-exact.

### What the runners freeze or take from the capture

`golden.patched_conics` (`tools/golden_runner/ds_patched_conics.cpp`): the harness's KspEnv (CelestialEnv: spawn at UT 0,
`UseHarnessFrame`, Mun/Minmus prefab `inverseRotation = 1` kept as the harness kept it), Planetarium time frozen at 0
(no FixedUpdate), and **every CelestialBody.position set to (0,0,0)**: the harness never writes a body position (bodies
are created uninitialised, its OrbitDriver rehost only calls Orbit.UpdateFromUT), kspcore's spawn moves them. Measured:
for these five cases the result is bit-identical even without the freeze (every position-dependent sum involves only
Kerbin, which kspcore's reversed root keeps at the origin; escapes to the Sun go through GetOrbitalStateVectorsAtUT);
the freeze stays for apples-to-apples. Loop and inputs: OrbitGen.cs PatchCases/SolvePatches (patchLimit 6, fresh
SolverParameters per case, targetBody null).

`capture.orbit_rails` (`tools/golden_runner/ds_orbit_rails.cpp`, capture code `HarnessCapture2Flight.cs:444-625`):
- inputs read from the capture file (never gated): provenance `ut0`, the requested probe elements `initial.req_*`
  (exact bits from `_hex`; the Mun-encounter orbit was constructed in game) and `initial.body`, the warp rates
  `segments.requested_rate` (TimeWarp prefab data, not extracted elsewhere);
- hidden game state: **Kerbin.directRotAngle = -140.0008910891089**, frozen since the flight scene switched Kerbin to the
  rotating frame long before the capture. Recovered from the first frames row as (rotationAngle(UT) - inverseRotAngle)
  % 360; the runner checks that it reproduces that row bit for bit; the other 449 rows (and every orbit column) are
  predictions;
- scene state: PSystemSetup.cs:978-990 (all bodies inverseRotation = false / reverse = false, then Kerbin true/true;
  this clears the prefab's Mun/Minmus flag), Planetarium time = ut0, Planetarium.fixedDeltaTime 0.02, TimeWarp HIGH,
  `TimeWarp.SetRate(i, instant)` = assumeWarpRate (curr_rate = rate; updateRate), the capture's frame schedule
  (5 frames, then per segment 1 + {500, 1000, 1000, 1500, 500} frames, a sample every 10th frame). The game clock
  reproduces bit for bit: UT += 0.02 * timeScale per frame (450/450 sample UTs);
- frame loop: Planetarium.FixedUpdate (time, CBUpdate pass from Kerbin), then every probe's OrbitDriver.UpdateOrbit
  (UPDATE mode, `vessel != null`: CheckDominantBody -> RecalculateOrbit -> OnRailsSOITransition), then a runner-only
  stand-in for Vessel.CheckKill (Vessel.cs:2746: unloaded vessel destroyed above 1 kPa or below altitude -0.1 m);
- probes go through ProtoVessel.CreateOrbitNode's G17 text (OrbitSnapshot Save/Load) into an UPDATE-mode OrbitDriver;
  body positions move as in kspcore (Kerbin, the reversed root, stays at its spawn position; the game keeps it near the
  floating origin; only CheckDominantBody and the Kerbin->Mun BSP read world positions, through differences);
- excluded columns (COLUMNS in tests/CMakeLists.txt): `samples.world_rel_*` (vessel.GetWorldPos3D() - mainBody.position:
  floating-origin world positions and the vessel transform / localCoM, WP-14/WP-20; in the capture itself
  |world_rel - pos.xzy| <= 3.7e-9 m for the bodies and <= 1.0e-9 m for the probes, <= 3.3e-15 relative: floating-origin
  rounding), `samples.packed/loaded` (Vessel state, WP-14/WP-19), `initial.note` (free text) and the echoed inputs
  `initial.req_*`, `segments.requested_rate`. Excluded rows (KEEP_ROWS, `ROWS` before the WP-05 merge): the two asteroid SOI captures in `soi_events`
  (spawned by the game's discovery scenario, not capture inputs). KEYS gives the keyless `soi_events` the key (vessel, UT).

Gate plumbing: `tools/filter_golden.py` gained `--rows t:col^prefix` and `--key t:c1,c2`, `ksp_golden_gate` /
`cmake/RunGolden.cmake` the matching `ROWS` / `KEYS` arguments; `KSP_CAPTURES2_DIR` = `harness/captures/out2`.
Since the WP-05 merge `ROWS`/`--rows` is `KEEP_ROWS`/`--keep-rows` (same filter, the unified row-condition grammar;
README.md "Golden gate options").

### Unit tests (`tests/test_patched_conics.cpp`, 9 cases, 374 assertions)

Planetarium.Orbits registration (16 body drivers in spawn order, vessel driver added/removed, cleared with the system);
Kerbin -> Mun encounter -> Mun escape -> Kerbin -> Sun chain (transition on the SOI sphere, position/velocity continuity,
UTsoi vs the golden); hyperbolic escape from Kerbin to Kerbol; degenerate cases (LEO: one FINAL patch, Sun-centred
hyperbola with infinite SOI, debug_disableEscapeCheck, no Planetarium -> caught exception, AngleWrap /
TAIsWithinPatchBounds); PatchedConicSolver patch list, patch limit (GameSettings at Awake, Increase/Decrease, clamp to
149, OnGameSettingsApplied), IDLE / timeScale 0; maneuver nodes (burn-vector conventions, GetPartialDv, nodeRotation,
gizmo state, Save/Load + Start coroutine, two-node chains, RemoveSelf); OrbitDriver on-rails SOI transition under
1,000x warp vs the patch prediction (OnReferenceBodyChange, onVesselSOIChanged, frameShift lock and Invoke release);
Trajectory sampling (closed FINAL, non-final with faded ends, hyperbola) and the Reframe quirk.

### Quirks and conventions a later implementer must know

1. **ManeuverNode.DeltaV axes**: z = prograde, y = normal (Unity-axis `Cross(-pos, vel)`), x = radial out:
   `QuaternionD.LookRotation(vel.xzy, Cross(-pos.xzy, vel.xzy)) * DeltaV`, applied to the world-like state vectors of
   `GetOrbitalStateVectorsAtUT(UT, out State)` (no Zup) and fed to `UpdateFromFixedVectors`.
2. **The solver's own iteration fields are unused by KSP**: `PatchedConicSolver.maxGeometrySolverIterations` etc. are
   never copied into its `SolverParameters` (C# defaults 25/50 always apply); ported as is.
3. **Patch limit** is read once at Awake (`Mathf.Max(CONIC_PATCH_LIMIT, 1)`), on OnGameSettingsApplied only when it
   differs, and clamped to [1, 149] at each Update; Update keeps going past the limit until the first maneuver node's
   UT is covered (it can index past patch 149: `std::out_of_range`, KSP's ArgumentOutOfRangeException).
4. **On-rails SOI transition order**: in the transition frame `updateFromParameters` has already moved the vessel and
   set `pos` relative to the old body; the orbit is switched afterwards (OnRailsSOITransition at the BSP crossing in
   [UT - TimeWarp.fixedDeltaTime, UT]); pos becomes relative to the new body at the next update. `frameShift` blocks
   another SOI change until Unity's `Invoke("unlockFrameSwitch", 1f)`: kspcore hosts must call
   `OrbitDriver::RunDelayedCalls()` once per frame (Time.time based).
5. **CalculatePatch only checks drivers with a `celestialBody`** in Planetarium.Orbits whose reference body equals the
   patch's; a later driver in the list overwrites an earlier encounter's nextPatch (order = spawn order).
6. **Trajectory**: the Reframe* methods always throw NullReferenceException in KSP (they fill the new Trajectory's null
   array); tTimes gain one period for every sample with E > 0 once fromE went negative (non-final patches starting near
   periapsis); `Vs` is not written for hyperbolas. All kept.
7. **CheckEncounter's target-approach marker** (`ClAppr` for a targeted body outside the SOI) is computed only with
   GameSettings.ALWAYS_SHOW_TARGET_APPROACH_MARKERS or on a background thread (`PatchedConics::IsBackgroundThread`).

### Deferred (and why)

- **Vessel side of OrbitDriver** (WP-10/WP-14/WP-19): `IOrbitDriverVessel` is the seam; TRACK_Phys / IDLE need a real
  Vessel (CoMD, rigidbody, velocityD), the NaN-orbit removal needs Vessel.Unload; GameEvents fire with
  `IOrbitDriverVessel::AsVessel()` (null for non-Vessel hosts). The C8 runner's CheckKill stand-in belongs to Vessel.
- **TimeWarp / FlightGlobals / GameSettings** are minimal subsets (WP-10 / WP-19 / WP-50 replace them): no rate changes,
  altitude limits or SOI-rate caps; FlightGlobals has only getMainBody/overrideOrbit/bodies; settings.cfg is not read.
- **Scene setup** (PSystemSetup / OrbitPhysicsManager rotating-frame switches, Kerbin.directRotAngle history) is set by
  the C8 runner only; the library still spawns with the prefab's Mun/Minmus inverseRotation flags.
- **UI and rendering**: PatchedConicRenderer (except nodeRotation), PatchRendering, ManeuverGizmo input handling,
  ScreenCast picking, MapObject/MapNode, ScreenMessages / maneuver-editor refresh (static hooks on PatchedConicSolver and
  ManeuverNode), orbit.DrawOrbit.
- **Alarm clock and transfer planner** (the AlarmClock*/AlarmType* rows of WP-08 in work_packages.csv): not in this
  assignment.
- `world_rel` of the C8 capture: needs the floating-origin state (FloatingOrigin/Krakensbane, WP-20) and Vessel transforms.

### Commits (branch `wp08`)

| hash | content |
|---|---|
| `04d00cd` | OrbitDriver, Planetarium.Orbits, PatchedConics, PatchedConicSolver, ManeuverNode, TimeWarp/FlightGlobals/GameSettings subsets, both gates, filter ROWS/KEYS |
| `3028ccb` | Trajectory, `tests/test_patched_conics.cpp`, STATUS/README |

## WP-00L: Linux build, portable bit-exact libm, game-copy-free tests, CI (branch `wp00l`)

Goal: a Claude Code cloud session (Linux container, no KSP install, no Unreal, no Windows) clones the repo, builds kspcore
and runs every gate. State: **done**; every gate is bit-exact (0 ULP) on Windows and on Linux with no dependency on the
game copy. Cloud-session instructions: `../CLOUD-TASKS.md`. Generator and research tools: `tools/libm/`,
`tools/mk_gamedata_mirror.py`.

**Results (2026-10-09)**

| platform / configuration | ctest | notes |
|---|---|---|
| Windows MSVC 14.44 RelWithDebInfo (`run_tests.ps1 -Fresh`), default: libm from `msvcr120_clr0400.dll`, mirror | 21/21 | the new test is `libm.portable_vs_dll` |
| Windows, `KSP_GAMEDATA_ROOT=decompiled/KSP-dev` (real install instead of the mirror) | 21/21 | |
| Windows, `-DKSP_LIBM_PORTABLE=ON` (software libm forced; `build/portable`) | 21/21 | every capture gate 0 ULP with the software path |
| WSL Ubuntu 24.04, Clang 18 (`run_tests.sh -f`, preset `linux-rel`) | 21/21 | `libm.portable_vs_dll` prints "skipped" (no DLL) |
| WSL Ubuntu 24.04, GCC 13 (`run_tests.sh -p linux-gcc-rel -f`) | 21/21 | |
| Windows Debug (`run_tests.ps1 -Preset debug -Fresh`) | 21/21 | |
| WSL Clang 18 Debug (`run_tests.sh -p linux-dbg -f`) | 21/21 | |

**1. Linux build.** Presets `linux-rel`, `linux-dbg` (Clang if installed, else the default compiler) and `linux-gcc-rel`;
`run_tests.sh` mirrors `run_tests.ps1` (`-f` fresh, `-t <regex>`, `-b` build only, `-p <preset>`). Same C++20 and warnings
(`-Wall -Wextra -ffp-contract=off -fno-fast-math`); MSVC flags untouched. Portability fixes: `CMAKE_CXX_SCAN_FOR_MODULES OFF`
(no clang-scan-deps needed); Clang rejects a nested type's default member initializers while the enclosing class is still
incomplete (`PhysicsGlobals::SurfaceCurvesList` lost its `= nullptr`, always value-initialised; the `MapSO::Origin origin = {}`
default arguments became overloads); paths: KSP and the harness build backslash-separated Windows paths, so every filesystem
entry goes through `dotnet::PathFromUtf8` (identity on Windows, backslash to slash elsewhere) and `Utf8ToWide` is a real UTF-8
decoder off Windows; `crt=host` in `kspcore_build_info()` off MSVC (the C ABI tests check `crt=MD` only under MSVC);
`unsigned __int128` / `__builtin_*` replaced by `MulWide` / `std::popcount` / `std::countl_zero` in portable code. ConfigNode
output stays CRLF on all platforms (the ConfigNode tests pass unchanged on Linux). Two fidelity fixes found on the way:
`QuaternionD.Angle` and `Math.Log(a, base)` used `std::acos` / `std::log` instead of the CRT path; Unity's native
`sinf/cosf/powf` (Random.onUnitSphere / insideUnitCircle, Quaternion.AngleAxis) went through `std::sin(float)` (host libm,
which differs from UCRT for about 0.1% of inputs) and now use `dotnet::NativeSinf/NativeCosf/NativePowf`.

**2. Portable bit-exact libm.** Instead of hand-porting algorithms, `tools/libm/gen_main.py` *lifts the CRT's own x64 machine
code*: capstone decodes the functions reachable from the 12 exports of `msvcr120_clr0400.dll` (acos asin atan atan2 cosh exp log
log10 pow sinh tan tanh; WP-01's note named 8 that visibly differ from UCRT, but tan, acos and log10 differ from glibc too, so
all 12 are lifted) and from `ucrtbase.dll` (sinf, cosf, powf, used by Unity's native code): 4,024 + 1,733 instructions. The
generator emits one C++ statement block per instruction over an emulated register file, flags and stack
(`src/dotnet/vs2013_lift.hpp`; generated `vs2013_libm_gen.cpp`, `ucrt_libm_gen.cpp` and their `_data.inc`, 63 KB and 14 KB of
read-only data the code reads). CRT infrastructure that cannot change a result (FP status word, errno, `_matherr` hook) is
stubbed; `_handle_error` returns its retval argument. The CRT picks FMA3 or SSE2 code at start-up (`_set_FMA3_enable`); the
portable path defaults to FMA3 (what the capture machine and this one use; `vs2013::SetFma3(false)` exists and demonstrably
differs for exp, tan and pow). `ucrtbase` needs `__isa_available` >= 2 (patched in the data image). Selection: Windows loads the
DLL unless `KSP_LIBM_PORTABLE=ON` / env `KSPCORE_LIBM_PORTABLE=1`; everywhere else the software path;
`LibmBackend()` / `kspcore_build_info()` name the backend.

| function | portable vs the DLL on Windows (`libm_check`: random bit patterns, per-domain ranges, near-integer, specials) | capture.libm_mono on Linux |
|---|---|---|
| acos asin atan atan2 cosh sinh tanh exp log log10 pow tan | 0 mismatches in 2,000,000 inputs each | 0 ULP |
| sinf cosf powf (UCRT) | 0 mismatches in 1,000,000 inputs each | via capture.unity_random, 0 ULP |

glibc against the capture before this package (of 3,093 inputs each): sinh 540, cosh 407, log10 246, tanh 183, tan 130, atan 115,
asin 21, exp 19, acos 18, atan2 10+5, pow 3-8, log 0; the software path is what makes the gate pass off Windows.
Regenerate (needs the Windows `System32` DLLs and `harness/tools-cache/venv` for capstone):
`python tools/libm/gen_main.py msvcr120 C:/Windows/System32/msvcr120_clr0400.dll <out>` and
`python tools/libm/gen_main.py ucrt C:/Windows/System32/ucrtbase.dll <out>`; copy the four generated files to `src/dotnet/`.
`msvcr120.dll` (the non-CLR copy, second in the Windows load order) is a different build and was not lifted; the lift tracks the
`_clr0400` copy that the gate used. `tools/libm/pe_tool.py`, `callgraph.py`, `census.py` are the research helpers.

**3. No dependency on the game copy.** Inputs outside the repo, before: `KSP_DICTIONARY_CFG`
(`decompiled/KSP-dev/GameData/Squad/Localization/dictionary.cfg`), the `golden_runner --gamedata` default (`capture.part_database`;
also the Physics.cfg / PartDatabase.cfg root), `test_partloader.cpp` (real GameData), `test_celestial.cpp` and `test_terrain.cpp`
(existence of the `KSP_x64_Data/*.assets|.resS` files the asset indexes name), `capture.gamedatabase_load_order`
(`decompiled/logs/ksp-capture-capture2-final2-KSP.log`). Already in the repo, no change: `corpus.gamedata`
(`harness/inputs/GameData` 368 cfg and `harness/inputs/Ships` 86 craft), the WP-11 maps (`data/terrain`). No test reads the
game's `saves` or `Ships`. After: `testdata/GameData-mirror/` (built by `tools/mk_gamedata_mirror.py`, `MANIFEST.json`) and
`testdata/logs/ksp-capture-capture2-final2-KSP.log`; the tests take the game root from env `KSP_GAMEDATA_ROOT`, else the mirror
(`tools/golden_runner/game_root.hpp`; CMake cache variable `KSP_GAMEDATA_ROOT`).
Mirror: 2,804 files under `GameData/` (533 cfg byte-identical with SHA-256 per file in the manifest; 2,271 placeholders), plus
`Physics.cfg` and `PartDatabase.cfg` verbatim, plus 10 zero-byte `KSP_x64_Data/*.assets|.resS` placeholders. By extension: .dds 895,
.mu 551, .cfg 533, .png 430, .ksp 196, .loadmeta 54, .craft 52, .wav 50, .truecolor 27, no extension 3, .mission 3, .txt 3, ._cfg 2,
.mbm 2, .bac 1, .dll 1, .ogg 1. Total 6.4 MB. One deliberate deviation from "zero bytes for every non-cfg file": **.dds and .mbm
placeholders keep their original header bytes** (128 bytes; length-prefixed name plus 16 bytes). `GameDatabase` reads exactly those
headers to decide whether a texture loads and whether it is a normal map (`ReadDdsHeader` / `ReadMbmHeader`); with empty files 14
asset rows of `capture.part_database` became "unresolved". The stock `.craft` files the tests use are `harness/inputs/Ships` (in the
repo); the 52 `.craft` under `GameData` are empty placeholders. `physics_constants` records `physicsDatabaseFilename` as the
author's absolute path; the runner re-roots that one string to `C:\Projects\KSP-UE5\harness\inputs\Physics.cfg` (the golden's text),
so the gate is location independent.

**4. CI** `../.github/workflows/ci.yml` (ubuntu: Clang and GCC via `run_tests.sh -f`; windows-latest: `run_tests.ps1 -Fresh`).
**5. Root files** `../.gitignore`, `../.gitattributes` (LFS lines present but commented out). **6.** `../CLOUD-TASKS.md`.

**Not achieved / limits.** The CI workflow itself was not run (no GitHub access from here). The portable libm reproduces the
FMA3 code path; on a CPU without FMA3 the DLL takes the SSE2 path and gives different bits for exp, tan and pow, the software path
still returns the FMA3 results (the capture's truth). Not tested: macOS; non-x86 hosts (the lift is host independent C++, but
`std::fma` is software-emulated where there is no FMA instruction, which is slow but exact). Large files in the repo:
`harness/captures/out/libm_mono.json` and `libm_dotnet.json` (45 MB each) and `data/terrain/maps/*.bin` (55 MB total); no LFS is
enabled (CLOUD-TASKS.md "LFS").

## WP-06: module host (PartModule / VesselModule / ScenarioModule, KSPField / KSPEvent / KSPAction registry) (branch `wp06`)

State: done. `capture.part_database` now gates **every KSPField of every module instance** (32,894 `module_fields` rows, was 5,401)
and the `parts.module_mass` / `parts.module_cost` columns (357 rows), all at max ULP 0 / identical strings. Every other gate is
unchanged and green. Every ported function cites `File.cs:line` of `decompiled/KSP-Decompiled/Assembly-CSharp/`.

### What is ported

| area | C++ | verification |
|---|---|---|
| KSPField reflection (BaseFieldList.ReflectedData, FieldAttribute/KSPField/KSPAxisField, UI_Control tree) | schema `parts/module_schema.inc` (PartModule classes, WP-05) and `parts/host_schema.inc` (ScenarioModule / VesselModule classes); attribute data `modules/module_members.inc` (generated by `tools/wp06/gen_module_members.py`): per field the `[KSPField(...)]` argument text, the `[UI_*(...)]` attributes, per class the `[KSPEvent]` / `[KSPAction]` methods; 18 UI_* class definitions | gate (names, types, isPersistant, values), unit |
| C# constant evaluator | `modules/cs_expr.{hpp,cpp}`: field initialisers (`1f / 17f`, `new Vector2(..)`, `Vector3.up`, `double.NaN`, `string.Empty`, `Localizer.Format("#autoLOC_..")`, enum members, `new FloatCurve()`) and attribute argument lists (named + positional); float arithmetic follows the Mono rule | gate (27,254 initialiser rows), unit |
| FieldStore / BaseField / BaseAxisField / BaseFieldList (BaseField.cs, BaseFieldList.cs, BaseAxisField.cs, BasePAWGroup.cs) | `modules/base_field.{hpp,cpp}`: per-instance typed storage indexed like the BaseFieldList; `Read` = ReadPvt (WP-05's `ReadModuleFieldValue`), SetValue type check, `GetStringValue` (G9 / G17 / vector forms / gui forms), `Load` (values then IConfigNode nodes, `_UIFlight` / `_UIEditor` nodes, SetOriginalValue), `Save`, UI control setup (Flight / Editor slot rules, UI_Label default) | gate, unit |
| BaseEvent / BaseEventList / BaseEventDetails / BaseAction / BaseActionList (KSPEvent.cs, KSPAction.cs) | `modules/base_event.{hpp,cpp}`: attribute instances with the C# constructor defaults, `Localizer.Format` of guiName, name hash ids (Mono `GetHashCode`), `Send` / `SendDefault` (disabled module rule), `OnLoad` / `OnSave` including the C# quirk that BaseEvent.OnSave writes `category` from `guiIcon`, `ContainsNonDefaultActions`, override groups; handlers are bound by the owning module (`Events().Bind`, `Actions().Bind`) like the delegates | unit |
| PartModule (PartModule.cs) | `modules/part_module.{hpp,cpp}`: constructor chain = FieldStore (initialisers), `Awake` (ModularSetup, OnAwake), `Load` (isEnabled, persistentId, UPGRADES, Fields.Load, EVENTS, ACTIONS, AXISGROUPS, OnLoad, UpdateStagingToggle), `Save`, `Copy`, staging toggle, the virtual hooks (OnStart ... GetInfo) | gate, unit |
| PartModuleList, ScalarModuleSetHandler, IScalarModule / IModuleInfo / IPartMassModifier / IPartCostModifier | `modules/part_module.{hpp,cpp}` (`PartGetModuleMass` / `PartGetModuleCosts` = Part.GetModuleMass / GetModuleCosts with the Mono float rule) | gate (module_mass / module_cost), unit |
| ModuleFactory (AssemblyLoader.GetClassByName + AddComponent) | `ModuleFactory::Register(className, creator)`; unregistered classes are the generic PartModule; `module_behaviours.cpp` registers the stock behaviours below | gate, unit |
| PartLoader integration (Part.AddModule, PartLoader.ParsePart) | `PartLoader::AddModule`: AddComponent (constructor chain) -> `Awake()` -> `Load(node)`; `PartModuleData::module` holds the object, `cfgFields` is the Fields.Load trace (unchanged contents, WP-05's tests untouched) | gate |
| VesselModule, VesselModuleManager (VesselModule.cs, VesselModuleManager.cs) | `modules/vessel_module.{hpp,cpp}`: lifecycle, Load/Save through the BaseFieldList, Activation flags and `ShouldBeActive`, wrapper list built from the 5 VesselModule classes of the schema (AxisGroupsModule, CometVessel, FlightIntegrator, SuspensionLoadBalancer, CommNetVessel) (order from a temporary instance: FlightIntegrator 0, default 999), `AddModulesToVessel` | unit |
| ScenarioModule host fields, ScenarioRunner, ProtoScenarioModule.Load (ScenarioModule.cs, ScenarioRunner.cs, ProtoScenarioModule.cs) | `save/game.hpp` ScenarioModule gained `Awake` / `Fields()` / `Events()` / `Store()` (default `SaveFields`/`LoadFields` = BaseFieldList); `modules/scenario_runner.{hpp,cpp}`: ScenarioRunner (Instance, scene events, `AddModule`, proto-module loading in scene order with MissionSystem first, `GetUpdatedProtoModules` wired into `Game.Updated()` through `SaveEnvironment::getUpdatedProtoModules`), ScenarioModuleFactory | unit |

Generators (regenerate, never hand-edit): `tools/wp05/gen_module_schema.py` (extended: also writes `host_schema.inc`; the
`module_schema.inc` it writes is byte-identical to WP-05's), `tools/wp06/gen_module_members.py` (`module_members.inc`).

### Gate `capture.part_database` after WP-06 (Linux release 1.7 s; bit-exact, no tolerance)

| table | capture rows | gated | excluded, and why | max ULP |
|---|---|---|---|---|
| parts | 362 | 357 x 55 cols (+ module_mass, module_cost) | 5 prefab parts (WP-05); column partSize (model bounds), variants.materials (model) stay dropped | 0 |
| part_cfg_values | 6,235 | 6,235 | - | 0 |
| modules | 1,378 | 1,312 | 66 rows of the 5 prefab parts | 0 |
| module_fields | 34,801 | **32,894** | 1,889 rows of the 5 prefab parts (their modules are Unity-serialized prefab data); 18 rows: `originalRotation` / `currentRotation` of 9 solar panels (ModuleDeployablePart.cs:542 OnAwake reads the pivot transform's localRotation from the .mu model); the exclusion file lists them by module class with the decompiled line | 0 |
| module_propellants | 77 | 77 | - | 0 |
| resources, attach_nodes, drag_cubes, variants, assets | 166, 827, 535, 228, 3,978 | 166, 822, 531, 228, 743 | unchanged from WP-05 | 0 |

`module_fields` provenance of the 32,912 compiled-part rows (printed by the runner): **5,402** are set by the part's MODULE node,
**256** are results of the ported OnAwake/OnLoad logic below, **27,254** equal the C# initialiser (WP-05's estimate was 27,391
against the whole capture, before the cfg-set rows were separated). The first run with only initialisers and the loader's
`Load` differed in 403 rows (58 module-class/field groups): the host bugs found and fixed on the way were SetValue rejecting a string
into a null string field, `new`-hidden duplicate field names and the FloatCurve null/empty default; the rest are the ported behaviours.

`module_mass` / `module_cost`: the 7 non-zero parts are fairingSize1..3 (Mathf.Epsilon clamp), PotatoComet, PotatoRoid
(-149 = asteroidMass 1 - part mass 150), fireworksLauncherBig / Small; all other parts are 0. Un-dropped at 0 ULP.

### Module behaviours ported (`src/modules/module_behaviours.cpp`)

Rule: only what changes a [KSPField] (or Part prefab data) between `AddComponent` and the end of `Load` while a prefab compiles
is ported, and only that part of OnAwake / OnLoad. Classes with the same behaviour through inheritance are registered under each name.

| class (decompiled line) | effect |
|---|---|
| ModuleToggleCrossfeed (:116 OnAwake, :127 OnLoad, :155 UpdateCrossfeed) | `part.fuelCrossFeed = crossfeedStatus` (moved here from WP-05's PartLoader special case), toggle event/action gui names |
| ModuleCargoPart (:96) | `stackableQuantity = 1` when the part has resources (was the single WP-05 exclusion) |
| ModuleAnimationSetter (:40), ModuleAnimateHeat (:22) | default `animStateCurve`; `ThermalAnim` -> `animName` |
| BaseConverter (:460, :342) for BaseDrill, ModuleAsteroidDrill, ModuleCometDrill, ModuleResourceConverter, ModuleResourceHarvester, ModuleScienceConverter | default ThermalEfficiency / TemperatureModifier curves; `status` text |
| ModuleDeployablePart (:732) and Antenna / Radiator / SolarPanel | `subPartName` / `partType` localisation, `stateString` -> deployState, `sunTracking`, SolarPanel default curves and `type` -> panelType |
| ModuleDataTransmitter (:107, :133) | default range/science DoubleCurves, `powerText` (KSPUtil.PrintSI), event gui name |
| ModuleGimbal (:296) | gimbalRange{XP,YP,XN,YN} fallbacks |
| ModuleCoreHeat (:416), ModuleResourceIntake (:75, :134), MultiModeEngine (:283), FlagDecalBackground (:57, :563), ModulePartVariants (:380, via WP-05's variant loader) | default curves, `autoSwitch`, `sizeLocked`, `displayingPortrait`, `variantIndex` |
| ModuleColorChanger, ModuleEngines(FX), ModuleRCS(FX), ModuleWheelSteering, ModuleWheelMotor(+Steering) | `if (curve == null) curve = new FloatCurve()` in OnAwake (BaseFieldList.Load skips null IConfigNode fields, so the node can only fill a curve OnAwake created) |
| IPartMassModifier / IPartCostModifier: ModuleComet, ModuleAsteroid, ModulePartFirework, ModuleProceduralFairing, ModuleJettison, ModuleInventoryPart, ModulePartVariants, FlagDecalBackground, PartStatsUpgradeModule | `GetModuleMass` / `GetModuleCost` (+ ChangeWhen); private state (cometMass, panelMass, ...) is a C++ member |

### Module behaviours not ported (left to the owning packages)

Everything else a module does: OnStart ... OnFixedUpdate, the bodies of [KSPEvent] / [KSPAction] methods (events and actions exist
with their attribute data and are bindable, but have no handlers), GetInfo / GetModuleDisplayName, ModuleResourceHandler (WP-15),
resource-ratio lists of converters (WP-28), propellant gauges, GameEvents subscriptions, FX / animation / transform lookups from the
model (ModuleDeployablePart.OnAwake pivot rotations, ModuleDataTransmitter DeployFxModules, ...), AdjusterPartModuleBase lists
(`ADJUSTERS` nodes), PartUpgradeManager (`ApplyUpgradeNode`, `FindUpgrades`, `LoadUpgradesDel`: WP-32), BaseServo mass/cost (Serenity
robotics: no stock part in the capture has a non-zero value), ModuleInventoryPart stored-part mass/cost (prefabs have none),
ModuleProceduralFairing area/mass of built fairings, `PartModuleUtil.PrintResourceRequirements` (needs ModuleResource, WP-15),
`BaseAxisField.CreateAxisList` / `AddAxis` and `BaseAction.CreateActionList` (Part-level collections, WP-13/16), UI_Control
subclass Load/Save (UI), KSPModule / PartInfo attributes (no stock class uses them), `ReflectedAttributes.isStageable`
(`PartModule::isStageable` is set by the registrant: a class overriding OnActive).

### Surprises a later implementer must know

1. **FloatCurve, DoubleCurve and string KSPFields are null after AddComponent** (no Unity-serializer instantiation for the
   runtime-added component; the capture proves it: OnAwake bodies guarded by `if (curve == null)` produce their defaults).
   `BaseFieldList.Load` skips null IConfigNode fields, so a MODULE `curve { key = ... }` node only loads into a curve OnAwake created.
   A module package that forgets the OnAwake creation silently drops the cfg curve (the gate catches it for stock parts).
2. A `new`-hidden field (ModuleControlSurface.transformName shadows ModuleLiftingSurface's) appears twice in the BaseFieldList; Load
   and the C9 table take the first (own class first, then bases: reflection order); kspcore keeps both fields and the runner
   writes the first.
3. Awake order: constructor chain -> Awake (ModularSetup builds Fields/Events/Actions, then OnAwake) -> Load. OnAwake therefore sees
   initialiser values only; a module reading cfg values must do it in OnLoad.
4. `Localizer.Format` runs in field initialisers, attribute guiName/guiUnits and module code: the Localizer must be initialised before
   any module is created (GameData::Load does; unit tests use `InitWithBuiltInEnglish`). Without an instance Format returns "".
5. BaseEvent.OnSave writes `category` = guiIcon; BaseAxisField.CopyField copies overrideIncremental from the source's overrideInverted;
   PartModule's own [KSPField]s (stagingEnabled ... showUpgradesInModuleInfo) and its `ToggleStaging` event / action exist on every
   module, so `Fields().Count()` is 10 + the class's own.
6. Event ids are Mono `string.GetHashCode()` (`MonoStringHash`), the same hash the resource library uses.
7. `ModulePartVariants.variantIndex` is a private [KSPField]: reflection with FlattenHierarchy returns the module's own private
   fields, so it is in the list (private fields of base classes are not: `ModuleFieldInfo::isPrivate`, WP-05 rule).
8. `ModuleFactory` registers the built-in behaviours lazily on first use (no static-initialiser registration: a static library would
   drop the translation unit).

### How a module package plugs in

Subclass `PartModule` (or a ported base), override `OnAwake` / `OnLoad` / `OnStart` ..., read and write fields with
`Get<T>(name)` / `Set<T>(name, v)` (T is the C# field type: float -> float, double, int -> int32_t, bool, std::string, Vector3f,
FloatCurve, `EnumValue` via `SetEnumByName`), bind `[KSPEvent]` / `[KSPAction]` bodies in OnAwake
(`Events().Bind("Activate", handler)`), implement `AsMassModifier()` / `AsScalarModule()` for the C# interfaces, and register with
`ModuleFactory::Register("ModuleXyz", creator)`. VesselModules and ScenarioModules register with `VesselModuleFactory` /
`ScenarioModuleFactory`. WP-13 (Part) should turn `PartModule::part` (a `PartPrefab*` today) into the real Part and let
`PartModuleList` live on it; `PartModuleData::module` is the bridge.

### Deviations, approximations

- `PartModule::GetPersistentId` uses a counter hash instead of `Guid.NewGuid().GetHashCode()` (ids are random in the game).
- GameEvents the C# fires inside OnLoad (`onPartCrossfeedStateChange`) are not fired while a prefab compiles (no listener exists).
- `UI_Control` subclasses keep the attribute fields as evaluated constants (`Param("maxValue")`); only the base class
  `Load` / `Save` (controlEnabled) are ported.
- Reflection order of events / actions / fields is "own class first, then bases, declaration order"; Mono's `GetMethods` order is
  unspecified, only the field order is verified by the capture (`field_count`, names; WP-05).
- Windows: not built here (cloud VM, Clang 18 and GCC 13 only); the code uses no platform headers, but `run_tests.ps1` was not run.

### Not done / open

- The 5 code-built prefab parts' modules (1,889 `module_fields` rows) need the Unity-serialized prefab data.
- Pivot `localRotation` of solar panels (18 rows) needs the .mu model reader (UE import).
- compare.py still ignores extra candidate rows (NEXT-STEPS follow-up); the gate's `missing_rows` is 0 for every table.

### Commits (branch `wp06`, from main `f50d6121`)

See `git log --oneline wp06` (module host skeleton and generators; instantiation through PartLoader + gate widening; VesselModule /
ScenarioRunner / unit tests; docs).

## WP-10 time warp, rails propagation, orbit driver (branch `wp10`)

Spec: `TimeWarp.cs` (all 865 lines), `WarpTransition.cs`, `WarpTransitionCalculator.cs`, `PositionTarget.cs`, `OrbitDriver.cs`
(finished on top of WP-08), `ITargetable.cs`, `ClearToSaveStatus.cs`; consumers read for the integration: `Planetarium.cs:201`
(`time += fixedDeltaTime * timeScale`), `FlightGlobals.cs:196/1406/1471`, `CelestialBody.cs:499/831/1013`, `GameSettings.cs:999-1011`.
Every ported function cites its line.

### What is ported

| C# file | kspcore | notes |
|---|---|---|
| `TimeWarp.cs` | `include/ksp/orbit/time_warp.hpp`, `src/orbit/time_warp.cpp` | the whole MonoBehaviour: Awake/OnDestroy singleton, Start, Update (mode switching by pressure/surface speed, the TIME_WARP keys, the physics-warp warning flag), FixedUpdate (rails warp forced down inside atmosphere, `Mathf.Lerp` rate ramp), `updateRate`, `SetRate`/`setRate`/`assumeWarpRate`/`setMode`, `getMaxOnRailsRateIdx` (ladder, atmosphere, g-force, surface motion, altitude look-ahead in both `MaxRailsRateMode`s, SOI clamp), `GetMaxRateForAltitude`, `GetAltitudeLimit`, `ClampRateToOrbitTransitions`, `WarpTo`/`CancelAutoWarp`/`autoWarpTo` (coroutine as an explicit state machine), `getMaxWarpRateForTravel`, `getWarpAccel`, `getRateChangeTravel`, all static properties (`WarpMode`, `CurrentRate`, `CurrentRateIndex`, `CurrentRateIsTargetRate`, `MaxRate`, `MaxPhysicsRate`, `deltaTime`, `fixedDeltaTime`) |
| `WarpTransition.cs`, `WarpTransitionCalculator.cs` | `warp_transition.hpp/.cpp` | whole classes |
| `PositionTarget.cs` | `position_target.hpp/.cpp`, `targetable.hpp` (ITargetable), `unity/transform.hpp` | whole class; the GameObject is a `Transform` stand-in (float position) |
| `OrbitDriver.cs` | `orbit_driver.hpp/.cpp` (WP-08) | `Targetable` getter added (vessel via `IOrbitDriverVessel::AsTargetable()`, body via `celestialBodyTargetable`); TRACK_Phys / IDLE / UPDATE transitions, `lastMode`, `lastTrackUT`, `fdtLast` back-dating and the SOI re-reference are unit-tested (they were only reachable through the UPDATE path before). `orbitColor`, `Renderer`, `DrawOrbit` are rendering: skipped |
| `CelestialBody.cs` | `resetTimeWarpLimits()` (:1013), the limit-count check in `Start()` (:499), `TerrainAltitude()` (:831, over `pqsController`) | WP-09 deferrals that needed TimeWarp / PQS |

Prefab data: `TimeWarp::LoadStockPrefab()` = the scene object's serialized arrays as the running game reports them (warpRates
1 5 10 50 100 1000 10000 100000, physicsWarpRates 1 2 3 4, altitudeLimits 0 0.05 0.05 0.1 0.2 0.4 0.8 1 (fractions of the body
radius), maxPhysicsRate_index 0, maxModeSwitchRate_index 0): `analysis/physics/raw/run*/*_summary.csv` rows `rt_TimeWarp.*`,
19 runs, all identical (not in any harness golden: the flight-scene TimeWarp is not in the level9 singleton dump). Checked by the unit test
against all 19 runs and, for `warpRates`, against the C8 capture (`segments.requested_rate` for rate indices 0,4,5,6,7 bit for bit). `Mode = HIGH` and `textDuration = 3f` are not in the dump; they are the C# field initialisers (TimeWarp.cs:30, :32).

Additive subsets other packages will replace: `FlightGlobals` (`IFlightVessel` seam = the Vessel members TimeWarp reads,
`ActiveVessel`, `VesselsLoaded`, `ready`, `currentMainBody`, `ship_srfVelocity`, `getAltitudeAtPos` (double and the float
overload `getMaxOnRailsRateIdx` uses), `getStaticPressure(alt, body)`; WP-19), `GameSettings::ORBIT_WARP_*` / `SHOW_PWARP_WARNING`
defaults (GameSettings.cs:1003-1011), `Time::deltaTime`, `PQS::meshVertMin/meshVertMax` (reset by SetupBuildDelegates; raised by
quad builds, which are WP-12: they stay at the reset value, so `minPeAllowed`'s third term is inert until then),
`CelestialBody::pqsController`. `VesselTargetModes` was defined twice (targeting_modes.hpp, physics_globals.hpp); there is one
definition now (`int32_t`-based, in targeting_modes.hpp).

`TimeWarpEnvironment` carries everything of TimeWarp.cs that is scene/UI/input state with no numerics: `HighLogic.LoadedScene`,
`FlightDriver.flightStarted`, `Flight.CanTimeWarpHigh/Low`, `InputLockManager` (lock names), this frame's key presses and the
modifier key, and the recorded outputs (screen-message texts, control locks, pause, `FlightLogger.IgnoreGeeForces`, the settings
save). Host protocol per frame: `Update()` then `ResumeAfterUpdate()`; per physics step `FixedUpdate()` then
`ResumeAfterFixedUpdate()` (the autoWarpTo coroutine's `WaitForFixedUpdate` / `yield return null` points).

### Gate results (RelWithDebInfo-equivalent Linux build; all harness/capture tolerances unchanged, 0 ULP)

| gate / table | rows | compared columns | max abs | max ULP | missing / extra |
|---|---|---|---|---|---|
| `capture.orbit_rails` / segments | 5 | 9 (rate_index, **requested_rate** (new: now `TimeWarp.warpRates[rate_index]` from the prefab table, was an echoed input), achieved_rate, frames, UT_start, UT_end, Time.fixedDeltaTime, TimeWarp.fixedDeltaTime, timeScale) | 0 | 0 | 0 / 0 |
| `capture.orbit_rails` / samples, frames, initial, soi_events | 3,465 / 450 / 6 / 4 | unchanged from WP-08 | 0 | 0 | 0 / 0 |

The C8 runner now drives the warp through the real code: `TimeWarp::LoadStockPrefab`, `Awake`, `Start` (flight scene, `SetRate(0)`),
then `TimeWarp::SetRate(rateIdx, instant: true)` per segment (flight scene, HIGH mode: `setRate` -> `getMaxOnRailsRateIdx` ->
`assumeWarpRate` -> `updateRate`), and `TimeWarp::FixedUpdate` every frame. The active vessel is the runner stand-in `PadVessel` (landed
on the pad, no surface speed, no patched conic solver attached): its answers are not capture outputs, the capture's achieved rates equal
the requested ones in all 5 segments, which is what `getMaxOnRailsRateIdx` must return for a landed still vessel. Columns still
excluded and why: `samples.world_rel_*` (floating-origin world position, WP-14/WP-20), `samples.packed/loaded` (Vessel state: constants
`true/false` for every unloaded probe, null for the bodies; computing them here would be gating a stand-in's constants; WP-14/WP-19),
`initial.note` (free text) and `initial.req_*` (echoed inputs); `soi_events` of the two asteroids. Nothing else of C8 was computable
without a Vessel; the capture carries no other TimeWarp columns (no physics-warp segment, no rate ramp, no warp-limit probe).

### Unit tests (`tests/test_timewarp.cpp`: 17 cases, 2,009 assertions)

Prefab tables vs 19 game runs and the C8 capture; `resetTimeWarpLimits` vs the 17 body prefabs (finding: only Kerbin, Minmus and
Tylo have limits equal to `altitudeLimits * Radius`; the other 14 are hand-tuned per body, e.g. Mun 0/5000/5000/10000/25000/50000/100000/200000);
`GetMaxRateForAltitude` boundaries on Kerbin and Mun and monotonicity on all bodies; `Start()` resetting limit arrays of the wrong
length; singleton/properties/`updateRate` in HIGH and LOW mode with UT advance per tick = `TimeWarp.fixedDeltaTime`; `SetRate` over the
C8 indices, control locks, clamping, the 1 s `Mathf.Lerp` ramp; `getMaxOnRailsRateIdx` (atmosphere with the real Kerbin PQS: `minPeAllowed`
= 70 km atmosphere since `radiusMax - radius` = 8,735 m, g-force, ladder, surface motion, outside the flight scene), the altitude
look-ahead in both `MaxRailsRateMode`s (100 km circular -> 7 / 3; 20x100 km orbit at apoapsis -> 3 with `ABOUT_TO_CRASH`), the SOI
clamp boundary (75.5 s = 25.5 s ramp + 50 s), `FixedUpdate` dropping out of rails warp inside the atmosphere; `Update` mode switching
(atmosphere / surface speed against `GThreshold`), keys, the physics-warp warning, difficulty parameters and the input lock;
`getMaxWarpRateForTravel` (including WarpTo's swapped min/max argument order); a full `WarpTo` simulation (plan, ramp up, decelerate,
UT error < 200 s, locks released) and `CancelAutoWarp` with the delayed lock release; `WarpTransitionCalculator` (stock rates:
UTToRateDown/Up, UTTo1Times, UTToRateDown(from,to), SafeRateToUTPeriod, table-change detection); `PositionTarget` (including terrain
altitude at the KSC from the real PQS = capture `h_ksc`); OrbitDriver TRACK_Phys / IDLE / UPDATE (back-dating by `fdtLast`, `offset=false`,
kinematic rigidbody, no rigidbody, hand-over to rails at a later UT, SOI re-reference through `TrackRigidbody`, `overrideOrbit`);
`OrbitDriver::Targetable`. The expected values are derived from the C# control flow and the game's tables, worked out in comments
next to each case.

### Quirks a later implementer must know

1. `WarpTo`'s call `getMaxWarpRateForTravel(dt, warpToMaxWarping, warpToMinWarping, ...)` passes the max where the callee's signature
   has `minTimeInWarp` (defaults 8.0 / 2.5): the argument order is swapped in the game; ported as is.
2. `GetMaxRateForAltitude` / the SOI clamp / `setRate` clamp to `warpRates.Length` in the public `SetRate` (not `Length - 1`) and rely on
   the later `Mathf.Clamp` in `setRate`.
3. `getMaxOnRailsRateIdx` converts the vessel altitude through the *float* `getAltitudeAtPos(Vector3, body)` overload: the altitude has
   float32 precision.
4. Only 3 of 17 bodies have prefab warp limits equal to the `resetTimeWarpLimits` formula.
5. `TimeWarp.CurrentRateIndex` is 1 (not 0) without an instance; `CurrentRateIsTargetRate` compares the float rate with the int index.
6. The warp-down ramp is discretised by the physics tick: `autoWarpTo` ends within a tick-worth of UT of the target (`(rate + 1) / 2 * 0.02`
   at most), not exactly on it.

### Deferred / needs (local work)

| needs | what for | how |
|---|---|---|
| **a warp-limit capture** (new C11, in flight, profiling build) | gate `getMaxOnRailsRateIdx` / `GetMaxRateForAltitude` / `minPeAllowed` / `ClampRateToOrbitTransitions` numerically | call `TimeWarp.fetch.getMaxOnRailsRateIdx(i, true, out reason)` by reflection for i = 1..7 in both `ORBIT_WARP_MAXRATE_MODE`s for a table of vessels (100 km circular, 20x100 km at Ap/Pe, 10 km flight, landed still/moving, ladder EVA, patch end in 30/60/90 s) recording altitude, orbit elements, `timeToPe`, the result index, reason, `minPeAllowed`, `pqsController.meshVertMax`, `Planetarium.TimeScale` |
| a rate-ramp / physics-warp capture | gate `FixedUpdate`'s lerp and LOW mode numerics (`Time.timeScale`, `Time.fixedDeltaTime` = 0.02f * ts, `curr_rate` per frame under the real `Time.time`) and `autoWarpTo` end to end | per frame for 10 s after `SetRate(i, false)` in LOW and HIGH: Time.time, curr_rate, tgt_rate, Planetarium.TimeScale, Time.timeScale, Time.fixedDeltaTime, UT; and a `WarpTo(UT)` run recording the rate index sequence and the final UT error |
| the flight-scene TimeWarp prefab as extracted asset | replace the runtime-dump provenance of `LoadStockPrefab` | UnityPy on the flight scene's TimeWarp MonoBehaviour (`tools/extract_*`) |
| Vessel (WP-14/WP-19) | `samples.packed/loaded`, TRACK_Phys with a real rigidbody CoMD, `IFlightVessel`/`IOrbitDriverVessel` for the real Vessel (adapters `Vessel::FlightView()` / `OrbitDriverView()` exist since the wp14 merge; one registry: WP-19), packing at the physics-warp boundary | WP-14 / WP-19 / WP-20 |
| WP-12 quad builder | `PQS.meshVertMax` raised by built quads (third term of `minPeAllowed`) | WP-12 |
| UI | `UITimeWarpController`, the warp scrubber, screen-message widgets, the `PhysWarpWarning` dialog | Unreal side; `TimeWarpEnvironment::screenMessages` / `physWarpWarningShown` are the hooks |
| `CelestialBody : ITargetable` | `OrbitDriver::Targetable()` for bodies without a host-supplied target | needs `bodyTransform` (engine) |

Not in this assignment although in `work_packages.csv` for WP-10: nothing (all five WP-10 files are ported).

### Test totals (fresh `./run_tests.sh -f`, Linux Clang 18, linux-rel)

21/21 ctest tests pass (every gate max ULP 0); `unit`: 159 doctest cases, 42,082 assertions (WP-10: 17 cases, 2,009).

### Commits (branch `wp10`)

| hash | content |
|---|---|
| `e266c472` | TimeWarp full port, WarpTransition(Calculator), ITargetable/PositionTarget, FlightGlobals/GameSettings/Time/PQS/CelestialBody seams, OrbitDriver::Targetable |
| `3b0529dd` | `tests/test_timewarp.cpp`, C8 runner drives `TimeWarp::SetRate` and gates `segments.requested_rate`, STATUS/README |
| (docs) | test totals and commit table |

## WP-13: Part model and part tree (branch `wp13`, from `wp06` e27bc88b)

State: done for the engine-agnostic part. `Part` is now a real object (data model, part tree, attach nodes, module list, resources, mass/cost,
crew, copy semantics, states, live proto snapshots) and the WP-05 loader builds it. Gates unchanged and green: `capture.part_database`
max ULP 0 (same columns as after WP-06, `parts:partSize` and `variants:materials` still dropped, reasons below), `corpus.gamedata` (86 crafts)
and the saves corpus tests inside `unit` unchanged and green. Every ported function cites `File.cs:line` of `decompiled/KSP-Decompiled/Assembly-CSharp/`; the line numbers were
checked by script against the decompile (a function name must appear within three lines of the cited line).

### What is ported (by C# file)

| C# file | C++ | ported | not ported (and why) |
|---|---|---|---|
| Part.cs (9,023 lines) | `parts/part.hpp`, `src/parts/part.cpp` (tree, nodes, resources, mass, crew, states, StartData), `src/parts/part_modules.cpp` (modules, Instantiate, copy callbacks) | fields and properties (`Part::fields` holds the PartFields row, WP-05); constructor / `Awake`; tree: `setParent`, `addChild`, `removeChild`, `clearParent`, `hasIndirectParent/Child`, `localRoot`, `RigidBodyPart`, `FindChildPart`, `FindNonPhysicslessChildren/Parent`, `GetPartsOutTo`, `SetHierarchyRoot`, `onAttach`, `onDetach`, `OnDelete`; symmetry (`getSymmetryCounterPart`, `isSymmetryCounterPart`, `RemoveFromSymmetry`, `CleanSymmetry*`, `SetMirror`); attach nodes (`AddAttachNode`, `SetupAttachNodes`, `UpdateAttachNodes`, `UpdateSrfAttachNode`, `FindAttachNode*`, `FindPartThroughNodes`, `UpdateOrgPosAndRot`); `PartCanBeDetached/Offset/Rotated`, `isAttachable`, the `isXxx` module-class queries; resources (`AddResource`, `SetResource`, `RemoveResource`, `GetResource`, `GetResourceMass`, priority, simulation reset/setup); mass (`UpdateMass`, `GetModuleMass/Costs/Size`, `GetPhysicslessChildMass`); crew (`AddCrewmember(At)`, `RemoveCrewmember`); `AddModule`, `LoadModule`, `RemoveModule(s)`, `FindModule*`, `HasModuleImplementing`, `InitializeModules`, `GetModuleStartState`, `Modules{OnStart,OnStartFinished,OnUpdate,OnFixedUpdate,OnActivate,OnDeactivate,BeforePartAttachJoint}`; states (`set_State`, `CanFail`, `activate`, `deactivate`, `force_activate`, `UpdateStageability`, `ShieldedFromAirstream`); `Instantiate` (Object.Instantiate clone), `OnWillBeCopied`, `OnCopy`, `OnWasCopied`, `OnWillBeMirrored`; `OnLoad`/`LoadEffects`/`SaveEffects`, `SendEvent`, `StartData` (the data block of `Start()`) | rendering, highlighting, colliders and joints (`PartJoint`, autostrut `7904-8660`, `Couple`/`Undock`/`decouple`/`Decoupling`/`disconnect`), collision handling, pack/unpack, thermal update and `FixedUpdate`, FX/audio/UI (`fxGroups` playing, stage icons), `RequestResource`/`TransferResource` family (WP-15), shields/airstream flow, `GetBoundsPoints`, the `PartToVesselSpace*` statics, Part's own BaseFieldList/BaseEventList/BaseActionList (EVENTS/ACTIONS nodes of the part are kept and written back unevaluated), ExperienceTrait registration, the Start coroutine's Unity component work |
| PartVariant.cs, ModulePartVariants.cs | `available_part.hpp` (`PartVariant`), `src/parts/part_variant.cpp`, `modules/module_part_variants.hpp`, `src/modules/module_part_variants.cpp` | constructors incl. `PartVariant(PartVariant baseVariant)`, `Load`, `GetAttachedNodeIndex`, `GetExtraInfoValue`, `ModelVisibility` (the GAMEOBJECTS list `UpdateModel` applies), variant lists, `RefreshVariant`, `SetVariant`, `ApplyVariant` data part (attach nodes, mass, cost, drag cubes, extra info), themes, `GetModuleMass/Cost` | Material/texture/shader switching, mesh enable/disable on a GameObject, PAW rebuild |
| PartGeometryUtil.cs | `parts/part_geometry_util.hpp`, `src/parts/part_geometry_util.cpp` | `GetRendererBounds`, `GetColliderBounds`, `GetPartRendererBounds`, `MergeBounds` (3 overloads), `FindBoundsCentroid`, `PartSize` (PartLoader.cs:937) over stored bounds (`Part::modelRenderers`, `Part::colliderBounds`) | reading the bounds from a mesh (`.mu` renderers are not read, see Needs) |
| AttachNode.cs, AttachNodeSnapshot.cs, AttachRules.cs, Attachment.cs, AttachModes.cs, AttachNodeMethod.cs | `parts/part_data.hpp/.cpp` (AttachNode gained `owner`, `attachedPart`, `FindAttachedPart`, `FromTransform`, `Clone`, `FindOpposingNode`, `ReverseSrfNodeDirection`, `ChangeSrfNodePosition`), `parts/part_types.hpp`, `save/proto_vessel.hpp/.cpp` (`AttachNodeSnapshot(AttachNode)`, `findAttachedPart`) | all | node icon (`DestroyNodeIcon`) |
| AxisGroupsManager.cs, KSPAxisGroup.cs, AxisFieldLimit.cs, IAxisFieldLimits.cs, KSPAxisMode.cs | `modules/axis_groups_manager.{hpp,cpp}`, `parts/part_types.hpp`; `PartModule::Save/Load` call it for AXISGROUPS | all (`KSPAxisGroup` itself was WP-04's, reused) | input binding of axis groups (WP-27) |
| PartValues.cs, PartStates.cs, PartPointer.cs, PartRawInfo.cs, PartGameObjectInfo.cs, PartHeightQuery.cs, PartSpaceMode.cs, ModifierChangeWhen.cs, IPartSizeModifier.cs, KSPPart.cs, KSPModelEvent.cs | `parts/part_types.hpp` | all (`PartValues` with the extremum rules of its constructor, `PartPointer` index/Equals) | none |
| ModuleDisplaceTweak.cs, ModuleMirrorPlacement.cs, CompoundPartModule.cs, ICompoundPartAnchorMethod.cs | `modules/part_structural_modules.{hpp,cpp}` | all four (`CompoundPart.target` is read through the interface the CompoundPart implementation will provide) | the PAW fields' GUI side |
| FlagNameManager.cs | `parts/flag_name_manager.hpp`, `src/parts/part_misc.cpp` | all; `FlagOrientation` / `FlagMesh` data records defined here because the manager needs them (owned later by WP-48) | flag texture loading |
| ProtoPartSnapshot / ProtoPartModuleSnapshot / ProtoPartResourceSnapshot live paths | `src/save/proto_part_live.cpp`, `save/proto_vessel.hpp` | `ProtoPartSnapshot(Part, ProtoVessel, ...)`, `storePartRefs`, `ConfigurePart`, `CreatePart`, `Load(vessel parts)`, `Init`, `ProtoVessel::LoadObjects`, snapshots from live modules/resources (`moduleMass` quirk below) | `ProtoVessel(Vessel)` and the Vessel-bound members (WP-14) |
| IPartMassModifier, IPartCostModifier | WP-06's `modules/part_module.hpp`, reused | | |
| Other | `unity/bounds.hpp` (Bounds with the managed Unity maths), `parts/available_variant_theme.hpp` | | |

Also changed (WP-05/06 code the Part model replaced): `PartPrefab` is now the data base of `Part` (virtual destructor; `PartPrefab::Modules`
renamed `ModuleData`; `srfAttachNode` and `baseVariant` are `shared_ptr` because the C# objects are shared); `AvailablePart` owns a
`Part` prefab, `variant`, `GetVariant`, `Variants()`; `PartLoader` builds `Part` objects and runs `Awake` / `OnLoad` at `CreatePart`;
the old stand-in `ModulePartVariants` of `module_behaviours.cpp` and the loader's private `AddModule` / `AddResource` / `PartVariant::Load`
were removed; `PartModule` gained the virtual hooks `OnStartBeforePartAttachJoint`, `OnWillBeCopied`, `OnWasCopied`, `OnWillBeMirrored`,
`PromoteToPhysicalPart`, `DemoteToPhysicslessPart`, `CopySerializedFrom`, `AsSizeModifier`, `AsConstruction`; `FieldStore::CopyValuesFrom`
(Object.Instantiate copies non-private serialised fields), `BaseFieldList::Set`; `SaveEnvironment::getPartInfoByName` hook.

### Gate results (Linux release, fresh build; harness tolerances unchanged)

| gate | rows | max abs | max rel | max ULP | notes |
|---|---|---|---|---|---|
| `capture.part_database` | same as after WP-06 (353 parts, 32,894 `module_fields`, 357 module_mass / module_cost) | 0 | 0 | 0 | now produced by the `Part` objects of the loader (`ds_part_database.cpp` reads `Part&`); row set and values unchanged |
| `corpus.gamedata` | 86 crafts | unchanged | | | untouched |
| `unit` (includes the saves corpus tests) | all cases | | | | green |

Excluded columns of `capture.part_database` (unchanged, `DROP_COLUMNS` in `tests/CMakeLists.txt`):
- `parts:partSize` is `PartGeometryUtil.MergeBounds(GetRendererBounds()).size.magnitude` over the renderer bounds of the instantiated prefab. The
  formula is ported (`PartGeometryUtil::PartSize`, unit-tested over given bounds in `tests/test_part.cpp`) but the bounds come from the `.mu`
  meshes (`KSP_x64_Data` / GameData models), which no package reads yet. It cannot be computed without mesh data, so it stays dropped.
- `variants:materials` is the `Material.name` list of the variant renderers (shader/texture names after `Renderer.material` instancing), again
  mesh/renderer data.

### Measured facts (unit tests in `tests/test_part.cpp`)

- 353 stock prefabs (362 cfg parts minus 4 DLC error rows and 5 prefab-only parts) build as `Part`: module counts, resource counts and mass sums equal the C9
  capture rows; `Instantiate` of every prefab followed by `UpdateMass` reproduces the C9 mass plus module mass.
- `physprofile` save: 458 parts / 1337 modules / 623 resources build through `ProtoVessel.LoadObjects` and write back byte-identically
  (`ProtoPartSnapshot(Part)`), except the hand-built asteroid part (skipped for srfN / rTrf / modMass).
- 33 of the 86 corpus crafts (stock parts only; 53 contain modded parts or an old format and are refused by `LoadShip`, as in the game) become
  consistent trees: 1,967 parts, one root each, parent/child links equal the `parent` indices, attach-node pairs mutual (except 3 pairs of one old craft). `modCost` / `modMass` written by the game equal `GetModuleCosts` / `GetModuleMass(partInfo.cost)` for 1,061 of 1,063 checked
  parts; the two differences are the fairings (`fairingSize1`, `fairingSize2`: their mass and cost come from built fairing panels, state of
  ModuleProceduralFairing not in the prefab).

### Unit tests (`tests/test_part.cpp`, 17 cases, 100,738 assertions)

enums and records; FlagNameManager and themes; Bounds and PartGeometryUtil; prefabs vs C9; Instantiate + UpdateMass of all prefabs; attach nodes;
part tree; surface node inversion; AddModule/LoadModule/RemoveModule rules; module lifecycle and error isolation; resources, crew, state, StartData;
variants and ModulePartVariants against the C9 variants table; copy semantics over a part tree; structural modules; physprofile round trip;
craft trees; activation / detach / delete state machine. Expected values come from the decompiled source, the C9 capture, the game's own saves and
crafts; no number is invented.

### Quirks reproduced (a later implementer must keep them)

- `hasIndirectParent` recurses with the part itself (Part.cs, not `parent`); `PartCanBeRotated` calls `CanBeOffset`.
- `RefreshVariant` uses `>` against `Count` (an index equal to Count is accepted); `baseVariant` with an unknown name selects index 0 (skipper
  `baseVariant = Full` -> Shroud); `new PartVariant(base)` is not memberwise and shares `srfAttachNode`.
- ProtoPartSnapshot constructor passes the part cost as the default mass of `GetModuleMass` (`moduleMass` field).
- `ModulePartVariants.useVariantMass` is set persistently in the EDITOR scene.
- A clone's private `variantIndex` resets to 0 (Object.Instantiate does not copy private fields): call `SetVariant(partInfo.variant.Name)` after `Instantiate`.
- The `OnEditorAttach/Detach/Destroy` and `OnJustAboutToDie/BeDestroyed` Callbacks start as empty delegates, never null.

### Deviations

- The Part object is a plain C++ class (no GameObject): `PartTransform` stands for the Transform fields used (position, rotation, scale), `PartModelRenderer` and
  `colliderBounds` are the renderer/collider inputs that a model reader will fill. Native Unity maths in them (quaternion, matrix) is plain float.
- `Part::Instantiate` approximates Object.Instantiate: public/serialised fields copied via `FieldStore::CopyValuesFrom`, variant lists deep-copied,
  `srfAttachNode`/`baseVariant` re-aliased; private state resets exactly as in C#.
- Hooks for the Vessel (WP-14) in `Part::vesselHooks` (isEVA, partCount, removeCrew, crewListSetDirty, snapshotSavesModules); `Part::vessel` is an opaque pointer.
- Part's own PAW lists (BaseFieldList/Events/Actions) are not modelled (listed above).

### Needs a new (local) capture or data

- `partSize` and `variants.materials` of every part: needs the `.mu` model reader (renderer bounds, material names) or a capture of
  `PartGeometryUtil.GetRendererBounds` after `Instantiate`; then un-drop the two columns.
- Native Transform maths for `PartToVesselSpace*`, `GetBoundsPoints`, `transform.position` of a parent chain (Quaternion/Matrix4x4 float outputs).
- `Part.Start` thermal values in flight (`skinTemperature`, `thermalMass`, `skinThermalMass`, `skinInternalConductionMult`) and the `FixedUpdate` temperature step: needs an in-flight capture.
- Fairing mass / cost (ModuleProceduralFairing panel state) for the two craft rows above.
- `module_persistence` (Part.Save of an instantiated, started part in flight).

Fresh run (`./run_tests.sh -f`, Linux release): 21/21 ctest tests passed, including `unit`, `corpus.gamedata` and `capture.part_database`.

### Commits (branch `wp13`)

see `git log --oneline wp06..wp13`.

## WP-15: Resource flow (branch `wp15`, from `wp13` 688c5711; wp13 merged again at the end)

State: done. The resource flow of KSP is ported: `PartSet` (the request/delivery solver and the crossfeed set builder) over the SCC flow
graphs, `PartResourceList`, the `Part.RequestResource` / `TransferResource` / `GetConnectedResourceTotals` family, `ModuleResource` /
`ModuleResourceHandler` (hosted by every `PartModule`), the crossfeed and fuel-line modules, the resource broker / converter, and the planetary
resource layer (`ResourceMap`, `ResourceCache`, `ResourceScenario`, `ResourceGameSettings`, `NoiseGenerator`). Every ported function cites
`File.cs:line` of `decompiled/KSP-Decompiled/Assembly-CSharp/` (checked by script: a name from the C# must appear within three lines of the cited
line; the ~25 remaining reports of the checker are prose references, names that exist only in C++, or constructors cited against Awake/OnDestroy).
Existing gates unchanged and green (`capture.part_database` 0 ULP, `corpus.gamedata`, `corpus.saves`, all WP-13 tests).
There is no in-game capture of the flow solver yet: its gate is the unit suite (expectations derived from the C# control flow and from
self-consistency properties over the real stock crafts) plus one capture gate for `Part.GetResourceMass()`; the capture the solver needs is
specified under "Needs".

### What is ported (by C# file)

| C# file | C++ | notes |
|---|---|---|
| PartSet.cs | `resources/part_set.hpp`, `src/resources/part_set.cpp` | all: constructors (vessel / part list / SCC graph variants), `GetConnectedResourceTotals` x4, `RequestResource` (priority lists, proportional split, sequential drain with the over-drain quirk), `GetResourceList`, `Rebuild*`, `RemovePart`, `AddPartToSet`, static `BuildPartSets` / `BuildPartSimulationSets`, `SetsEqual`, `OnFlowStateChange` / `OnFlowModeChange` / priority handlers. `_BALANCE` flow modes are not distinguished by the C#, nor here |
| PartResourceList.cs, PartResource.cs | `resources/part_resource_list.hpp`, `src/resources/part_resource_list.cpp`; `PartResource` extended in `parts/part_data.hpp` | all (`Add` x3, `Remove` x3, `Get`, `GetAll`, `GetAllFlowing`, `GetFlowingTotals`, `GetFlowing`, `HasFlowable(Unhidden)`, `RefreshSimulationListAmounts`; `PartResource` constructors, `SetFlowState` / `SetFlowMode` (event-firing properties), `SetInfo`, `CanProvide`, `Flowing`, `Copy`, `Load` / `Save`) |
| Part.cs (members WP-13 skipped) | `src/parts/part_resource_flow.cpp` | `RequestResource` x10, `requestResource`, `TransferResource`, `GetConnectedResourceTotals` x6, `CanCrossfeed` x2, `RequestFuel`, `FindFuel`, `fuelLookupTargets`, `crossfeedPartSet`, `simulationCrossfeedPartSet`, `resourceTargets`, `ResetSimulationResources`. `DrainFuel` is WP-13's stub (`part.hpp`, returns false as Part.cs:4825). Not ported: the obsolete `RequestRCS` (Part.cs:4831; no callers anywhere in the assembly) |
| RUI/Algorithms (FlowGraph, StackFlowGraph, SCCFlowGraph, SCCFlowGraphUCFinder, StronglyConnectedComponentFinder, Vertex) | `resources/flow_graph.hpp`, `src/resources/flow_graph.cpp`; `dotnet/hash_set.hpp` (HashSet with .NET entry order) | all graph and Tarjan code except the constructors `Vertex(IEnumerable<Vertex>)` and `Vertex(T, IEnumerable<Vertex>)` (Vertex.cs:32, 38; unused in the assembly); `Vertex.transformGuide` / `StackFlowGraph.BuildTransformGuides` (Unity transforms of the debug lines) not ported |
| ModuleResource.cs, ModuleResourceHandler.cs, ModuleResourceAutoShiftState.cs | `modules/module_resource.hpp`, `modules/resource_modules.hpp`, `src/modules/module_resource.cpp`, `resource_modules.cpp` | all but the PAW text (`PrintRate`, `PrintModuleResources`); `PartModule::resHandler` is wired into `Awake` / `Load` like `PartModule.resHandler` |
| ModuleToggleCrossfeed.cs, IToggleCrossfeed.cs, CompoundParts/CModuleFuelLine.cs, EVAResourceContainer.cs | `modules/resource_modules.hpp`, `src/modules/resource_modules.cpp` | full ports; the WP-06 `ModuleToggleCrossfeed` stand-in was removed. The fuel line's `CompoundPart.target` link is test support only (`tools/support/craft_tree.hpp`, `LinkFuelLines`); `CompoundPart` itself is outside the package |
| ResourceBroker / IResourceBroker, ResourceConverter, ResourceSetup, ConversionRecipe, ResourceRatio, ConverterResults, ResourceFlowMode, ResourceTransferMode, HarvestTypes, PartResourceDrainDefinition and the small records | `resources/resource_broker.hpp`, `resources/resource_types.hpp`, `src/resources/resource_broker.cpp`; `PartResourceDrainDefinition` extended in `part_resource_library.hpp` | all |
| ResourceMap, ResourceCache, ResourceScenario, ResourceGameSettings, ResourceUtilities, NoiseGenerator, PlanetaryResource, DistributionData, ResourceData, DepletionData, BiomeLockData, PlanetScanData | `resources/planetary_resources.hpp`, `src/resources/planetary_resources.cpp` | all. `ResourceMap.GetAbundance` reads the biome map: `CBAttributeMapSO.GetAtt` is not ported (needs the biome textures), so the biome lookup is the host hook `ResourceEnvironment::getBiomeAtt`; the altitude / latitude / longitude noise is ported |
| Vessel.cs:451-464, 3913-4040, ShipConstruct.cs:46, 916-970 (the PartSet owners) | `resources/resource_flow_host.hpp`, `src/resources/resource_flow_host.cpp` | the seam to WP-14 / WP-18, see below |
| ProtoPartResourceSnapshot.cs | WP-13's `proto_vessel.hpp` (unchanged) | `GetCurrentResourceInfo` returns `AvailablePart.ResourceInfo`, a UI record WP-05 does not have; deferred |
| ModuleAlternator `moduleResourceBasedPrimaryIsInput` | - | the field exists on the handler; the alternator module does not exist yet (WP-06 solar panel sets it) |

Changed WP-05/06/13 code (all behaviour-preserving for the existing gates): `PartPrefab::Resources` and `Part::simulationResources_` are
`PartResourceList` (was `vector<PartResource>`; `resourcesSetUp_` / `simulationSetUp_` removed), `Part::Instantiate` clones the list
(`CloneFrom`), `PartModule` owns a `ModuleResourceHandler`, the part.cpp resource block works over the list. Forward declarations were
checked against the definitions (`struct PartResource`, `class PartResourceList`, `class PartSet`; MSVC mangles the keyword).

### The Vessel seam (for WP-14, WP-18)

`resources/resource_flow_host.hpp`. A Vessel (or ShipConstruct) is seen through `IResourceVessel` (`ResourceParts()`, `RequestResource`,
`GetConnectedResourceTotals` x2); `ResourceFlowEnvironment::vesselHost` / `shipHost` map `Part::vessel` / `Part::ship` to it. The PartSet
owners `VesselResourceSets` (Vessel.cs:3913-4040: `UpdateResourceSets`, `BuildCrossfeedPartSets`, `RequestResource`,
`GetConnectedResourceTotals`; `resourcePartSet`, `simulationResourcePartSet`, `crossfeedSets`, `simulationCrossfeedSets`, `resourcesDirty`) and
`ShipResourceSets` (ShipConstruct.cs:916-970) are members the Vessel / ShipConstruct forward to. The three GameEvents subscriptions of
Vessel.cs:1574-1577 (`onPartResourceListChange`, `onPartCrossfeedStateChange`, `onPartPriorityChanged` -> `UpdateResourceSetsEventCheckPart`)
stay in the Vessel (they test `p.vessel == this`); `tools/support/craft_tree.hpp::TestVessel` is the reference implementation WP-14 can copy.

### Gate and unit results (Linux release; harness tolerances unchanged)

| gate | rows | max abs | max rel | max ULP | notes |
|---|---|---|---|---|---|
| `capture.resource_mass` (new) | `deltav_readout.json` table `parts`, column `resource_mass`: 1,225 parts of 17 crafts (key craft_id + part_index) | 0 | 0 | 0 | `Part.GetResourceMass()` of every part of the C5 craft set: the craft files go through `KSPUpgradePipeline.Process` (as `ShipConstruction.CreateConstructFromTemplate` does; renames `liquidEngine1-2` and the v180 control surfaces; the v180 scripts read the prefab's `ctrlSurfaceRange` through the existing `SaveEnvironment::prefabModuleFloat` hook, which WP-15 only installs from the loaded parts in `InstallUpgradeHooks`, `tools/support/craft_tree.hpp`), `LoadShip`, the part tree; dataset `tools/golden_runner/ds_resource_mass.cpp`. Only `resource_mass` is gated (`mass` / `module_mass` are WP-13's, the staging columns WP-18's) |
| `capture.part_database` | unchanged | 0 | 0 | 0 | green |
| `corpus.gamedata`, `corpus.saves`, all WP-13 tests | unchanged | | | | green |

Unit tests: `tests/test_resource_flow.cpp` (11 cases, 21,194 assertions) and `tests/test_resource_modules.cpp` (11 cases, 7,316 assertions).
Expected values come from the decompiled control flow, the real stock configs, the physprofile save and the stock crafts; no number is invented.

- Flow: predicates and property events; `PartResourceList`; `TransferResource` clamping and thermal mixing; PartSet totals and requests (priority order,
  proportional inside a list, sequential across lists); every `ResourceFlowMode`; state / mode / priority changes with simulation twins; constructors and
  `AddPartToSet`; asparagus staging with fuel lines, decoupler crossfeed and `NoCrossFeedNodeKey`; drain definitions.
- Stock crafts (`harness/inputs/Ships`, `tests/test_resource_flow.cpp`, through the upgrade pipeline): 51 of the 86 files load (the others contain FAR,
  Pwings and similar mod parts and are refused by LoadShip as in the game); 3,020 parts, 671 request-graph components. For every craft: the SCCs of
  the request graph partition the parts; the BFS reachability agrees with the condensed DAG and with the transposed delivery graph; 29 fuel lines in 12
  crafts link their targets (equal to the PARTDATA `tgt` count); 153 `ModuleToggleCrossfeed` parts; connected-resource totals equal the sum over the
  part's set (514 checks); draws on LiquidFuel of three sizes (1 %, 37 %, 150 % of the set total) obey the solver invariants: conservation (drawn == amount removed from the set), lists drained from the highest priority, at most one partially drained list, the rest untouched, proportional split inside the partial list (138 draws).
- Modules: `ModuleResourceHandler.OnLoad` equals the cfg on all 353 prefabs (74 with module resources); handler runtime
  (`UpdateModuleResourceInputs` / `Outputs`, `HasEnoughResourcesToAutoStart`); broker / converter / `ResourceSetup`; `ModuleToggleCrossfeed` (toggle,
  symmetry propagation, OnStart, Instantiate); `CModuleFuelLine`; `EVAResourceContainer`; `ModuleResourceAutoShiftState`; `ResourceGameSettings` from
  the physprofile save (GameSeed 1985366484) with the Save round trip; `ResourceUtilities` / `NoiseGenerator`.
- `ResourceMap.GetAbundance`: bit-exact against an independent Python implementation (`tools/wp15/abundance_reference.py`, numpy float32 stores,
  KSPRandom int32 wrap), 1,896 cases plus the body item lists (`tests/data/wp15_abundance_reference.json`). The reference and the C++ share only the
  decompiled C# as source.

### C# quirks reproduced (later code must keep them)

- The 4-argument `PartResourceList` constructor never sets `simulationSet`: simulation twins loaded by `Add(node)` have `simulationResource == false` until `RefreshSimulationListAmounts` runs. Two WP-13 test expectations were corrected to this behaviour (`tests/test_part.cpp`: the sim twin flag; `OnCopy` leaves a stale twin, so the simulation sizes after OnCopy are 2 and 3).
- `PartResourceList.Add(name, ...)` fires `onPartResourceListChange`; `Add` / `Remove` fire it only for the non-simulation list.
- `PartResource.flowState` / `flowMode` setters fire the events whenever the value changes; `UpdateCrossfeed` fires the state-change event when the value changed regardless of `fireEvent`.
- `PartSet.GetConnectedResourceTotals` with a threshold: the pulling check is `maxAmount * (1 - maxAmount * threshold)` (sic).
- `ModuleResourceHandler.IsResourceBelowShutOffLimit` is inverted relative to its name.
- `Part.RequestFuel` can loop forever on a part tree that never reaches its target (the C# loop has no cycle guard); kept, not guarded.
- `Simplex.fastfloor` (Simplex.cs:195, used by the noise generator) is `(int)x - 1` for x <= 0, so `fastfloor(-2.0)` is -3.
- Sequential drain across lists: the last list's `TransferResource(r, -demand)` can over-drain by the unfilled remainder; returns are `demand - (demand - x)` rounded, tests compare with an approximate check there.
- Null references the C# would throw as NullReferenceException (a PART without `part`, v180 upgrade without a prefab module) throw `NullReferenceException` here.
- Set order is `HashSet<Part>` entry order (`dotnet::HashSet`: entries array, LIFO free list), which decides which part drains first among equal priorities.

### Deviations

- `PartResource` keeps public fields (`flowState`, `flowMode` are the C# `_flowState` / `_flowMode`); the property semantics are `SetFlowState` / `SetFlowMode`.
- Ownership: `PartSet`s are held by `shared_ptr` and compared by identity (C# reference semantics); GameEvents listeners of a PartSet are registered in its constructor and removed in its destructor (C# relies on `OnDestroy` / the vessel doing it).
- `GetAtt` of the biome maps and the PAW / Localizer display text are host hooks or not modelled (see the table).

### Needs a new (local) capture or data

- Harness dataset `resource_flow` (in-game, EDITOR and FLIGHT): for the 17 C5 crafts plus an asparagus craft with fuel lines and a craft with `ModuleToggleCrossfeed`, per physics tick of a scripted burn: (a) per part the amounts of every resource before / after each `Part.RequestResource` of the engines (key craft, tick, part_index, resource), (b) the `crossfeedSets` composition (part indices per set, set order) after `Vessel.UpdateResourceSets`, (c) the same after toggling crossfeed on one decoupler / fuel line and after changing a `resourcePriorityOffset`, (d) `Vessel.GetConnectedResourceTotals(id, simulate)` for LiquidFuel / Oxidizer / ElectricCharge. With it the unit-level properties here become a 0 ULP gate: the per-part draw split is the one place where order (HashSet enumeration, priority lists) shows in the result.
- `ResourceMap.GetAbundance` in flight: needs the biome map reader (`CBAttributeMapSO.GetAtt`, the biome PNGs / MapSO data) and a capture of `ResourceMap.Instance.GetAbundance(request)` for a set of bodies / lat / lon / altitudes.
- `ProtoPartResourceSnapshot.GetCurrentResourceInfo` needs `AvailablePart.ResourceInfo` (part-info panel records).
- The WP-13 craft statistics ("33 of 86 craft load", 1,967 parts) in `tests/test_part.cpp` use `LoadShip` without the upgrade pipeline; with the pipeline 51 of the 86 files load.

Fresh run (`./run_tests.sh -f`, Linux release): 22/22 ctest tests passed (`unit` 195 cases, `capture.resource_mass` new, `capture.part_database`, `corpus.gamedata`, all golden / capture gates); `corpus.saves` is registered only where `decompiled/KSP-dev/saves` exists (not on this VM, as before WP-15).

### Commits (branch `wp15`)

see `git log --oneline wp13..wp15`.

## WP-14: Vessel model, situations, mass properties (branch `wp14`, from `wp13` 688c5711, merged `wp13` ae3e81fa)

State: done for the engine-agnostic part. `Vessel` is a plain C++ class (no GameObject; created with `std::make_unique` so its address is
stable): part list and ownership, state flags, situations (`checkLanded` / `checkSplashed` / `updateSituation` / `SetLandedAt`), crew list and
control level, control state and action-group blocking, kinematics (`UpdatePosVel`, `UpdateSurfaceSpeeds`, `UpdateAcceleration`), mass /
CoM / MOI (`GetTotalMass`, `UpdateMassStats`, `CalculatePhysicsStats`), `Initialize` / `Load` / `Unload` / `GoOnRails` / `GoOffRails` /
`Die` / `BackupVessel`, naming rules, ITargetable / IDiscoverable, the `ProtoVessel(Vessel&)` and `ProtoVessel::Load` live paths and the
FlightRegistry (the FlightGlobals lists). Also ported: FlightCtrlState, ActionGroupList, DiscoveryInfo (data side), VesselValues, the
flight logger family (FlightLogger, ModuleTripLogger, VesselTripLog, FlightCoMTracker) and AxisGroupsModule. Every ported function cites
`File.cs:line` of `decompiled/KSP-Decompiled/Assembly-CSharp/` (checked by script: a cited line never exceeds the file length and the
function name appears within three lines of it). Gate `capture.vessel_state` (new): max ULP 0. All earlier gates unchanged and green.

### What is ported (by C# file)

| C# file | C++ | ported | not ported (and why) |
|---|---|---|---|
| Vessel.cs (4,664 lines) | `vessel/vessel.hpp`, `src/vessel/vessel.cpp` (registry, ctor/dtor, properties, reference transform, situations, crew, control, modules, naming, ITargetable / IDiscoverable, the `VesselOrbitDriverView` adapter), `vessel_motion.cpp` (mass, CalculatePhysicsStats, positions, kinematics, atmosphere mirror), `vessel_transitions.cpp` (Initialize, Load / Unload, rails, Die, BackupVessel, ProtoVessel live paths) | fields, `State` / `ControlLevel` (12, 45), `Situations` use (VesselSituations of WP-08), properties (53-475), `findVesselParts` / `Initialize` / `StartFromBackup` / `Load` / `Unload` / `GoOnRails` / `GoOffRails` / `Die` / `BackupVessel` (538-905, 1583-2000 in part), `checkLanded` / `checkSplashed` / `updateSituation` / `SetLandedAt` (1015-1062, 1185, 3133), crew (1234-1485), control state and overrides (1994-2035, 4045-4076), the srf_velocity block inside `UpdatePosVel` (2845-2865: `srfSpeed`, `horizontalSrfSpeed`, `srf_vel_direction`; factored out as `Vessel::UpdateSurfaceSpeeds`, it is not a function of its own in the C#), `UpdateAcceleration` (2892), `UpdatePosVel`, mass and CoM (3104-3240, 3376-3441), naming (3761-3815, `FindDefaultVesselType`, `UpdateVesselNaming`, `IsValidVesselName`, auto clean), positioning (`SetPosition` / `Translate` / `SetRotation`), vessel size and ranges | rigidbody / joint anchoring and ground-contact printing (about 1064-1180: physics objects, WP-21); ground raycasts `CheckGroundCollision` / `getLowestPoint` / `GetHeightFromSurface` / `GetHitFromSurface` / `GetGroundLevelAngle` (about 2493-2712: reached through `VesselEnvironment` hooks); `FixedUpdate` / `Update` / `LateUpdate` integration (2730-2745, 2991-3103: WP-21); Krakensbane / FloatingOrigin (WP-20); screen messages, input locks, rename dialog (4078-4136), IVA / camera; internal-space, Serenity and Making History helpers (4217-4300, 4429-4660); resource sets / PartSet (3908-4044: WP-15, `updateResourceSets` hook); autopilot, CommNet connection (WP-39), VesselDeltaV, map / orbit renderers, patched-conic attach / detach (2262-2355), target / waypoint resume coroutines (2155-2233), contract objectives (4308-4363) |
| FlightCtrlState.cs | `vessel/flight_ctrl_state.hpp`, `src/vessel/flight_ctrl_state.cpp` | the full class: shared `custom_axes`, Save / Load of the 7 CTRLSTATE values, `MaskedCopyFrom`, Neutralize, copy | input-handler ownership (WP-24) |
| ActionGroupList.cs | `vessel/action_group_list.hpp`, `src/vessel/action_group_list.cpp` | constructor shape, indexer, Load / Save, CopyFrom | `ToggleGroup` / part action dispatch (WP-16) |
| DiscoveryInfo.cs, IDiscoverable | `vessel/discovery_info.hpp`, `src/vessel/discovery_info.cpp` | levels, lifetimes, signal strength, Load / Save, size classes | UI strings, the KSC discovery manager (assigned to WP-09, absent there, so ported here) |
| VesselValues.cs | `vessel/vessel_values.hpp`, `src/vessel/vessel_values.cpp` | `PartValuesComparison<T>` / `PartValuesOperation<T>`, extremum over the parts' PartValues, default start value, per-frame cache | |
| FlightEvents.cs, FlightEndModes.cs, VesselActuatorMode.cs, EventReport.cs, IVesselAutoRename.cs, VesselEngiUtil.cs | `vessel/vessel_types.hpp` | all | |
| ITargetable.cs (implemented here, defined by WP-10) | `orbit/targetable.hpp` (WP-10); `Vessel::GetTransform`, `FlightCoMTracker::GetTransform` | `Vessel` (Vessel.cs:3761-3815) and `FlightCoMTracker` implement the WP-10 interface; `GetTransform()` hands out the `ksp/unity/transform.hpp` stand-in (name, position, rotation) | |
| ProtoTargetInfo.cs | `src/vessel/vessel_transitions.cpp` (extras of `ProtoTargetInfo` in `save/proto_vessel.hpp`) | `ProtoTargetInfo(ITargetable)`, `FindTarget` through the registry | |
| FlightLogger.cs, FlightLog.cs | `vessel/flight_logger.hpp`, `src/vessel/flight_logger.cpp` | statistics, the event log, mission end, `Invoke` delayed calls as `RunDelayedCalls`, coroutines as pending state | UI, `FlightResultsDialog`; `FlightLog` / `FlightLog.Entry` already existed in `save/crew.hpp` (WP-04) and are reused |
| ModuleTripLogger.cs, VesselTripLog.cs | `vessel/module_trip_logger.hpp`, `src/vessel/module_trip_logger.cpp` | Log / Save / Load of the FLIGHTLOG and LOG nodes, deprecated nodes, delayed situation logging, forced logs | |
| FlightCoMTracker.cs | `vessel/flight_com_tracker.hpp`, `src/vessel/flight_com_tracker.cpp` | marker follows the vessel CoM (its own `UnityEngine::Transform trf`), targets once FlightGlobals is ready | rendering |
| AxisGroupsModule.cs | `vessel/axis_groups_module.hpp`, `src/vessel/axis_groups_module.cpp` | axis groups built from the vessel's `[KSPAxisField]` floats, `BaseAxisField_IncrementAxis` / `UpdateAxis` | |
| ShipConstruction.cs (part half only) | `vessel/vessel_assembly.hpp`, `src/vessel/vessel_assembly.cpp` | `BuildCraftParts`, `AssembleVesselFromCraft` (the part half of `LoadShip` / `AssembleForLaunch`: parts instantiated at the craft's `pos` / `rot`, tree linked, `Vessel.Initialize`) | launch-site placement, kerbal crew assignment, editor |

Also changed in earlier WPs' files (small, additive): `physics/vessel_ranges.hpp` (`Vessel_Situations` is an alias of `VesselSituations`),
`save/proto_vessel.hpp/.cpp` (live `ProtoVessel(Vessel&)` / `Load` declarations; the unloaded `ProtoPartResourceSnapshot` node constructor
sets `definition`; `CheckPartpersistentId` now evaluates `flag2` as in the C# instead of the former constant false),
`save/save_environment.cpp`, `modules/vessel_module.cpp`, `modules/module_behaviours.cpp` (the trip logger module registration through a
local forward declaration).

### Gate results (Linux release, fresh build; harness tolerances unchanged, `tests/tolerances_capture.json` 0 ULP)

`capture.vessel_state` (dataset `vessel_state`, golden `harness/captures/out2/aero_forces_vessel.json` = C10; DROP_HEX plus the column
drops below):

| table | rows | gated columns | max abs | max rel | max ULP | inputs vs outputs |
|---|---|---|---|---|---|---|
| `structure` | 631 (PhysProf-L2-AS 457 parts, -M2 153, -S 21) | `part_name`, `parent_index`, `attach_node`, `attachMode`, `dragModel` | exact | exact | 0 | input: the crafts (the game's own `analysis/physics/craft/*.craft`, as launched by C10). Output: the whole tree, built by `BuildCraftParts` + `Vessel.Initialize` (`findVesselParts` DFS order, parent index, which node attaches to the parent, attach mode, drag model) |
| `states` outputs | 20 snapshots (3 crafts) | `srfSpeed`, `density`, `mach`, `dynamicPressurekPa`, `pseudoReynolds`, `pseudoReDragMult`, `cacheDragCubeMultiplier` (f32), `parts` (i64, exact) | 0 | 0 | 0 | `srfSpeed` is the srf_velocity block of `UpdatePosVel` (Vessel.cs:2845-2865) over the captured `srf_velocity_*`. `density`, `mach`, `dynamicPressurekPa`, `pseudoReynolds`, `pseudoReDragMult` are functions of the captured static pressure, temperature and `spd` (FlightIntegrator formulas mirrored into the Vessel by `ApplyFlightIntegratorAtmosphere`). `cacheDragCubeMultiplier` is `PhysicsGlobals.DragCubeMultiplier`, `parts` the vessel's part count |
| `states` consistency columns | 20 | `staticPressurekPa`, `staticPressureAtm`, `atmosphericTemperature` | 0 | 0 | 0 | consistency checks, NOT independent gates. The runner recovers an altitude by inverting the captured static pressure and a temperature offset by inverting the captured temperature (the capture's `altitude` is `Vessel.altitude` of the root part, not FlightIntegrator's CoM altitude: a 457-part stack differs by about 100 m; `GetAtmoThermalStats` is not ported), then checks that the ported curves reproduce the captured P and T bit for bit. These three columns therefore only prove the inversion is consistent: the altitude-to-pressure and altitude-to-temperature models are not gated here (`golden.atmosphere_curves` gates them). The captured `altitude` column is used only as the fallback when the inversion fails (it did not fail on any of the 20 rows) |

`states.spd` is not gated (see below): the gated `states` columns are `srfSpeed`, `density`, `mach`, `dynamicPressurekPa`, `pseudoReynolds`,
`pseudoReDragMult`, `cacheDragCubeMultiplier`, `parts` and the three consistency columns (11 columns, 20 rows each; 12 before `spd` was dropped).

Dropped columns (`DROP_COLUMNS` in `tests/CMakeLists.txt`, each with its reason):
- `structure:cube_names` and `structure:node_contact_areas`: cube weights are set by module OnStart logic and `DragCubeList.ForceUpdate`,
  contact areas by `DragCubeList.SetPartOcclusion` (WP-21 / module packages). `tests/test_vessel.cpp` still checks the cube NAMES.
- `states:spd`: input. `Vessel.speed` is FlightIntegrator's `spd` copied through `ApplyFlightIntegratorAtmosphere` and the runner feeds it the
  captured `spd`, so comparing it would compare the capture with itself.
- `states:frame`, `UT`: engine state. `altitude` (the root part's, only the inversion fallback), `aoa_deg`, `srf_velocity_*` (inputs of `srfSpeed`),
  `ref_up_*`: inputs of the dataset or values that need the Krakensbane / reference-transform frame state of WP-20 / WP-21. No row is filtered.

### Unit tests (`tests/test_vessel.cpp`, 24 cases, 6,901 assertions)

small types; FlightCtrlState; ActionGroupList; DiscoveryInfo; vessels assembled from the three PhysProf crafts vs the C10 structure table
(counts, order, tree, cube names); the `physprofile` save round trip (`ProtoVessel -> Vessel -> BackupVessel`); situation determination;
mass properties (`GetTotalMass`, `UpdateMassStats`, `CalculatePhysicsStats` CoM / MOI of PhysProf-S); surface speeds; `UpdatePosVel` on a
circular equatorial Kerbin orbit; `UpdateAcceleration`; the atmosphere mirror; crew list, capacity, control level; control state and group
overrides; naming; state transitions (GoOffRails / GoOnRails / Unload / Load); reference transform, `PartsContain`, ranges, size;
positioning; VesselValues; FlightRegistry and ProtoTargetInfo; ModuleTripLogger (FLIGHTLOG round trip of the physprofile modules);
FlightLogger; FlightCoMTracker; AxisGroupsModule. Expected values come from the decompiled source, the C10 capture, the game's own saves and
crafts; no number is invented.

### Measured facts

- `physprofile` save (458 parts): 1335 of 1337 MODULE nodes write back identically through `ProtoVessel.Load` -> `Vessel.Load` ->
  `BackupVessel`. The two exceptions are `ModuleReactionWheel` and `ModuleDataTransmitter` `OnSave` (not ported: WP-24 / WP-39); the test asserts
  the exact difference. `cPch` / `cHdg` reload as 0 (KSPParseable returns 0 from `value_float()` for an UNKNOWN-typed value: faithful).
  VESSELMODULES: the test does not compare the VESSELMODULES node at all (`continue` in the node loop of the physprofile round-trip case). The
  save's node lists `FlightIntegrator`, `AxisGroupsModule`, `SuspensionLoadBalancer` (and `CommNetVessel` with `controlState` / `canComm` on the L2-AS
  vessel); the port regenerates the node from the VesselModules `VesselModuleManager::CompileModules` found, whose set depends on other packages
  (FlightIntegrator: WP-21; CommNetVessel: WP-39; SuspensionLoadBalancer: not ported; the game never wrote CometVessel into this save), so a
  comparison would test the module set of those packages, not WP-14. The CometVessel / CommNetVessel wrappers are additionally deactivated so
  the vessels load with the same module types as the asteroid.
- `CheckPartpersistentId` flag2 is evaluated as in the C#; the physprofile round trip keeps the `persistentId`s.
- `Vessel.parts` is the DFS order of `findVesselParts`; the three C10 crafts reproduce all 631 rows.

### Quirks reproduced (a later implementer must keep them)

- `GetTotalMass` accumulates in double: `totalMass += fx(part.mass) + fx(part.GetResourceMass())` with no intermediate float rounding (Mono rule).
- CoM summation runs from the last part to the first (test: summation order matters in the last bit).
- A landed vessel stays packed in a non-rotating frame; `GoOffRails` uses `FlightGlobals.RefFrameIsRotating` (body.rotates && inverseRotation),
  not `Planetarium.FrameIsRotating`.
- The naming quirk of `UpdateVesselNaming` (a part compared with itself) is kept.
- `OnFlightLogRecorded` fires on every `LogSituation`.
- The srf_velocity block of `UpdatePosVel` (Vessel.cs:2845-2865, `Vessel::UpdateSurfaceSpeeds`) computes `srfSpeed` and the horizontal / vertical split in the C# order; the gate reproduces `srfSpeed` bit for bit.

### Deviations and seams

- `VesselEnvironment` (`Vessel::env`): optional `std::function` hooks for everything that is Unity or another package: `pqsLevels`,
  `pqsSurfaceHeight`, `heightFromTerrainProbe`, `checkGroundCollision`, `packPart`, `unpackPart`, `resumePartVelocity`,
  `precalcGoOnRails` / `precalcGoOffRails`, `commNetControlLevel`, `updateResourceSets`, `cleanVessel`, `fullTemperatureAtPosition`,
  `krakensbaneFrameVelocity` / `krakensbaneAddFrameVelocity`, `calculateCraftSize`, `isFacilityOrLaunchSite`, `biomeDisplayName`,
  `miniBiomeDisplayNameByUnityTag`. A missing hook gives the C# behaviour for "no such object". `CelestialBody` has no PQS controller, so PQS
  data comes through these hooks.
- `FlightRegistry` (static): `Vessels`, `VesselsLoaded`, `VesselsUnloaded`, `ActiveVessel`, `ready`, Add / Remove / SortVessel, FindVessel,
  FindPartByID, Clear. Persistent-id dictionaries stay SaveEnvironment's. It is the FlightGlobals subset WP-14 needs; WP-19 / WP-10 own the
  rest. `IOrbitDriverVessel` is implemented by the adapter `VesselOrbitDriverView` (the interface's `CoMD()` / `velocityD()` clash with
  Vessel's fields); its `AsTargetable()` returns the Vessel, so `OrbitDriver::Targetable()` (OrbitDriver.cs:75) is the vessel.
- `IFlightVessel` (WP-10, `celestial/flight_globals.hpp`, the Vessel members TimeWarp.cs reads) is implemented by the adapter
  `VesselFlightView` (`Vessel::FlightView()`), not by `Vessel` itself: the interface's `altitude()` / `geeForce()` / `srf_velocity()` clash
  with Vessel's fields of those names and its `orbit()` returns `Orbit*` where `Vessel::orbit()` returns `Orbit&`. `OnALadder` is
  `IEvaController::OnALadder` (KerbalEVA.cs:1205; default false, and false when the vessel has no KerbalEVA, where the C# would throw).
  **Left for WP-19:** there are two registries. `FlightRegistry` (WP-14: `Vessels`, `VesselsLoaded`, `VesselsUnloaded`, `ActiveVessel`,
  `ready`, typed `Vessel*`) and `FlightGlobals` (WP-08/WP-10: `bodies`, `ready`, `ActiveVessel` / `VesselsLoaded` typed `IFlightVessel*`,
  `currentMainBody`). Nothing keeps them in step: a host that runs TimeWarp with real vessels must put `&v->FlightView()` into
  `FlightGlobals::ActiveVessel` / `VesselsLoaded` and set both `ready` flags itself. WP-19 ports FlightGlobals.cs and makes it the one
  registry (FlightRegistry's lists typed `Vessel*`, `IFlightVessel` / `IOrbitDriverVessel` then retired or kept only for test stubs).
- `ITargetable` is WP-10's (`orbit/targetable.hpp`); WP-14's earlier copy (with `GetTransformName()` / `GetTransformPosition()`) was removed in
  the wp13 merge. `Vessel::GetTransform()` (Vessel.cs:3761, `return ReferenceTransform`) returns a `UnityEngine::Transform` owned by the
  Vessel and refreshed from `ReferenceTransform()` on every call: name = the Vessel's `gameObjectName` (vesselTransform) or the reference
  part's `GetReferenceTransformName()`, position and rotation copied. Exact for a caller that reads it after the call; a caller that keeps
  the pointer (KSP's `FlightGlobals.fetch.vesselTargetTransform`) sees the values of the last `GetTransform()` call, not the live transform,
  and after `SetReferenceTransform` it re-targets on the next call where KSP would keep the old part's transform. Unifying `PartTransform`
  with `UnityEngine::Transform` (so the pointer is the live reference transform) is WP-19 / WP-20. `ProtoTargetInfo(ITargetable)` for a
  generic target reads `GetTransform()->name` (ProtoTargetInfo.cs:64; a null transform throws `NullReferenceException`); a
  `PositionTarget`'s GameObject is now named "New Game Object" (Unity's name for `new GameObject()`), so a saved ground target writes
  `tgtId = New Game Object` as in KSP.
- `VesselPrecalculate` is a seam struct (isEasingGravity, mass maths of CalculatePhysicsStats): WP-19. `PartSet` / resource sets: WP-15 (hook).
  `ActionGroupList.ToggleGroup`: WP-16. `FlightCtrlState` ownership with the input handler: WP-24.
- Part-side state that `Vessel` reads goes through `Part::vesselHooks` (WP-13) and `Part::vessel` stays the opaque `Vessel*`.
- `ProtoVessel` ownership: `Vessel::protoVesselOwner_` is a `shared_ptr`; `protoVessel` is a raw pointer valid until the next backup.
- Unity `Invoke` / coroutines become `RunDelayedCalls` / host callbacks and pending state; `Vessel` has no MonoBehaviour life cycle.
- `Vessel_Situations` is an alias of `VesselSituations`; `ClearToSaveStatus` stays an opaque enum (merge-safe with WP-10).
- Native Unity matrix / quaternion inverses on the MOI path of `CalculatePhysicsStats` are plain per-op float32 and are not verified (no capture).
- `GameSettings` namespace in `vessel.hpp` carries only the two values Vessel reads (`ADDITIONAL_ACTION_GROUPS = false`,
  `RADAR_ALTIMETER_EXTENDED_CALCS = true`), both defaults of GameSettings.cs.
- `CalculatePhysicsStats`, flag branch (`vessel_motion.cpp` 207-231, `VesselPrecalculate.cs:469-510`), two deviations:
  1. packed and `LandedOrSplashed`: C# is `CoMD = worldSurfacePos + worldSurfaceRot * localCoM` (`:475`); the port uses
     `vesselTransform.TransformPoint(localCoM)` because `worldSurfacePos` / `worldSurfaceRot` live in `VesselPrecalculate`, which is a seam.
     The two agree only while the vessel transform holds the same world-surface placement. Resolved by WP-19 (VesselPrecalculate) with WP-20 (floating origin).
  2. unpacked: C# reads `rootPart.rb` (`rb_velocity = rb.GetPointVelocity(CoM)`, `velocityD = rb_velocity + Krakensbane.GetFrameVelocity()`,
     `angularVelocity = Quaternion.Inverse(ReferenceTransform.rotation) * rb.angularVelocity`, `:492-497`); the port always takes the `else`
     (zero) branch of `:499-505`, because parts have no rigidbody. Resolved by WP-21 (part rigidbody / joints) with WP-19.
  `MOI` and `angularMomentum` are zeroed as in the C#.
- The live game also attaches the VesselModules `CommNetVessel` and `CometVessel` (and others); this port's set is whatever
  `VesselModuleManager::CompileModules` found, so VESSELMODULES nodes of those do not round trip (deactivated in the test).

### Needs a new (local) capture or data

- A `vessel_state` in-game capture with mass / CoM / MOI columns (not the existing C10 dataset): CoM, total mass, MOI (principal moments and tensor), situation, landed-at, control level, `Vessel.altitude`,
  `heightFromTerrain`, `GetHeightFromSurface` and FlightIntegrator's CoM altitude for sample vessels (PhysProf-S, -M2, -L2-AS on the pad, in
  orbit, and flying). Mass / CoM / MOI have no capture column at all today; they are unit-tested against the source only, and the MOI path
  (native Unity Matrix4x4 / Quaternion maths) is therefore the least verified part of this package. Un-drop `states:altitude` etc. when it exists.
- `DragCubeList` cube weights and `SetPartOcclusion` contact areas of an assembled vessel (the two dropped `structure` columns).
- `GetAtmoThermalStats` inputs (sun direction, body temperature offsets) so FlightIntegrator's `atmosphereTemperatureOffset` is computed, not recovered.

### Fresh run

Before the wp13 (main) merge: `./run_tests.sh -f` (Linux release, clang 18): 22/22 ctest tests passed (21 earlier tests unchanged plus `capture.vessel_state`); `unit` 197 cases / 147,884 assertions
(24 cases / 6,869 assertions of them are `test_vessel.cpp`); ctest total 40 s. Not run here: Windows (MSVC) and Debug presets; the class/struct
forward-declaration rule of wp13 ae3e81fa was applied (no `class PartVariant|PartResource|AvailablePart;` declaration exists).

After merging `wp13` (with main: WP-06, WP-10) and unifying ITargetable: `./run_tests.sh -f` (linux-rel, clang 18) and
`./run_tests.sh -p linux-gcc-rel -f` (GCC 13): 22/22 each, every gate column max ULP 0 (429 compared columns); `unit` 214 cases /
149,925 assertions (24 cases / 6,901 of them in `test_vessel.cpp`). Not run here: Windows (MSVC).
After the later merge of main with WP-15 (`capture.resource_mass`): 23/23 ctest, every gate 0 ULP; `unit` 236 cases / 178,437 assertions
(the `unit` summary row above). Windows (MSVC) is covered by CI.

### Commits (branch `wp14`)

see `git log --oneline wp13..wp14`.

## WP-18: Craft files and ship construction (branch `wp18`, from `wp14` 7bc606d6)

State: done for the engine-agnostic part. `ShipConstruct` now also holds live `Part` objects (it is the editor ship of ShipConstruct.cs:
`parts` is a `List<Part>`), `ShipTemplate` is the craft browser's view of a .craft file, and `ShipConstruction` is the static helper class
around both (paths, backups, load / save, AssembleForLaunch, templates, costs, launch-site queries). Every ported function cites
`File.cs:line` of `decompiled/KSP-Decompiled/Assembly-CSharp/` (checked by script: the name of the C++ function, or of the C# function it
ports, appears within three lines of the cited line; the reports that remain are C++-only names such as `InstantiateParts`, which is half of
`LoadShip`). New gate `capture.ship_construct` (max ULP 0). All earlier gates unchanged and green.

### The two halves of ShipConstruct (read this before using it)

- WP-04's `ShipConstruct::parts` (`PartData`) is the **file-side record** of the PART nodes (LoadShip's parse). It is untouched; ships that
  were only `LoadShip`'d behave as before (`corpus.saves`, the WP-04 / WP-13 / WP-15 helpers `support::LoadCraftFile` +
  `BuildCraftTree`, WP-14's `AssembleVesselFromCraft`).
- The WP-18 half: C# `parts` (`List<Part>`) is **`liveParts`** (raw `Part*`), owned by `ownedParts` when the ship created the parts.
  `LoadShipWithParts(root, persistentID, returnErrors, err)` = file-side `LoadShip` + `InstantiateParts` (ShipConstruct.cs:378-700: Instantiate
  the prefab of every PART through WP-14's `BuildCraftParts`, assign the file's values, MODULE / RESOURCE / PARTDATA / EVENTS / ACTIONS /
  EFFECTS / VESSELNAMING, parent / children / symmetry / attach nodes, `orgPos` / `orgRot`, `InitializeModules`, the persistentId check,
  `UpdateVesselNaming`). `SaveShip()` dispatches to `SaveShipFromParts()` when the ship has live parts: every value is then read from the
  `Part` / `PartModule` / `PartResource` objects (modCost, modMass, modSize, the full attN list of the prefab, the module nodes), not
  carried from the file.
- `ShipConstruction` takes ships by pointer (`unique_ptr<ShipConstruct>` returns), `AssembleForLaunch` moves the ship's parts into the new
  `Vessel` (`ship.ownedParts` is emptied; `Part.ship` is cleared), as the Unity scene hands the GameObjects over.

### What is ported (by C# file)

| C# file | C++ | ported | not ported (and why) |
|---|---|---|---|
| ShipConstruct.cs (971 lines) | `save/ship_construct.hpp`, `src/save/ship_construct.cpp` (WP-04 file side; WP-18: `changedPersistentIds_`, the deferred part-id check), `src/save/ship_construct_live.cpp` (WP-18) | constructors (76-150), `SaveShip` over live parts (153-293), `LoadShip` part half 378-700 (`InstantiateParts`, `LoadShipWithParts`), `AreAllPartsConnected` (709), `isControllable` (727, [Obsolete]), `Clear` / `Add` / `Remove` / `Contains` / `AddToConstruct` / enumeration (742-784), `GetShipCosts` x2 (786, 791), `GetShipMass` x2 (822, 827), `GetTotalMass` (859), `UpdateVesselNaming` / `RunVesselNamingUpdates` (866, 878), `GetConnectedResourceTotals` (916), `RequestResource` (927), `UpdateResourceSets` x2 (936, 941: WP-15's `ShipResourceSets`), the IResourceVessel host, `IShipConstructIDChanges` hand-over | `vesselDeltaV` creation in the editor (lines 690-700, VesselDeltaV is WP-17): hook `ShipConstruct::onEditorShipLoaded`; the popup dialogs (error text is returned or logged); `ScreenMessages.PostScreenMessage` (hook `postScreenMessage`) and `Part.RefreshVesselNamingPAWDisplay` (UI); `StageManager.SetStageCount(parts)` (:688) computes the highest `inverseStage` into a local and discards it: no call; `FlightGlobals.PersistentLoadedPartIds.Remove(part.persistentId)` after Instantiate (the port's `Part::Instantiate` never registers an id); `gameObject.SetActive`, `transform.parent`, `SetLayerRecursive` (Unity hierarchy) |
| ShipConstruction.cs (1,349 lines) | `ship/ship_construction.hpp`, `src/ship/ship_construction.cpp` | statics `backups`, `ShipConfig`, `ShipManifest`, `ShipType`; `ClearBackups`, `CreateBackup`, `ShiftAndCreateBackup`, `RestoreBackup`, `DebugBackup` (28-72); `LoadShip` x2, `SaveShip` x2, `SaveShipToPath` x2, `GetPlayerCraftThumbnailName` x2, `GetSavePath`, `GetCurrentGameShipsPathFor`, `GetShipsPathFor` x2, `GetRootCraftSavePath`, `CheckCraftFileType`, `GetShipsSubfolderFor` (73-289); `AssembleForLaunch` x2 (291-434, over WP-14's `AssembleVesselFromParts`), `GetUniqueFlightID`, `findFirstCrewablePart`, `findFirstPod_Placeholder`, `findFirstControlSource`, `PutShipToGround` (436-553); `CalculateCraftSize` x3, `FindCraftCenter` x2, `FindCraftMOI` x2 (555-652); `CheckLaunchSiteClear` x2, `FindVesselsLandedAt` x2, `FindVesselsAtLaunchSite`, `RecoverVesselFromFlight` x2 (654-777); `CaptureStockThumbnail` / `CaptureThumbnail` (hooks); `LoadTemplate`, `SaveSubassembly`, `SubassemblyExists`, `LoadSubassembly` (937-985); `CreateConstructFromTemplate`, `OnPipelineFailed`, `OnPipelineFinished`, `SanitizeCraftIDs` (1113-1195); `GetPartCosts` x2, `GetPartCostsAndMass`, `SanitizePartCosts`, `FindPartWithCraftID`, `AllPartsFound`, `IsCompatible` (1197-1349) | `AutoGenerateThumbnail`, `ThumbnailIsOutdated`, `GetThumbnail` x3, `LoadThumbnail` (coroutine), `CreateSubassemblyIcon`, `RenderCamera`, `StripPartComponents`, `GetSABounds`, `SetLayerRecursively` (779-935 and 987-1111 except the two Capture functions): camera / RenderTexture / UnityWebRequest UI. In `AssembleForLaunch`: `PSystemSetup.GetSpaceCenterFacility` / `IsStockLaunchSite` (a lookup whose result is discarded) and `AnalyticsUtil.LogVesselLaunched` |
| ShipTemplate.cs (251 lines) | `ship/ship_template.hpp`, `src/ship/ship_template.cpp` | the whole class: fields, `GetShipSize`, both `LoadShip`, `GetPartName`, `GetPartId`, `SetIfControllable` | the Making History mission part filter and R&D tech are hooks (`ShipTemplateEnvironment::missionExcludedParts` / `isExperimentalPart` / `partTechAvailable`): no `MissionSystem` / `ResearchAndDevelopment` yet |
| ShipConstruction.AssembleForLaunch, the part half (WP-14) | `vessel/vessel_assembly.hpp`, `src/vessel/vessel_assembly.cpp` | refactored, not duplicated: `AssembleVesselFromCraft` = `BuildCraftParts` + the new `AssembleVesselFromParts` (the rest of AssembleForLaunch: orbit driver, Initialize flags, naming, landed state, flight / mission / launch ids, crew manifest hook, preCreate crew copy, reference transform); `BuildCraftParts` gained `shipOwner` (`Part.ship`), the EVENTS / ACTIONS / EFFECTS / VESSELNAMING nodes and an `onPartLoaded` callback (the persistentId check) | |

Names in the task text that do **not** exist in KSP 1.12.5's three files (so nothing was ported for them): `ShipConstruction.GetPartTotalMass`,
`ShipConstruction.GetShipCosts` / `GetShipMass` (they are `ShipConstruct.GetShipCosts` / `GetShipMass`, ported), `ExtractShipName` /
`ExtractShipTag`, `PlaceOnLaunchSite` (the equivalent is `PutShipToGround` + the placement passed to `AssembleForLaunch`), a ShipConstruct overload
of `GetPartCosts` (the overloads take a `ProtoPartSnapshot`; the ship-level formula is `GetShipCosts`), `GetDistanceToLaunchSite`.

### Gate results (Linux release, fresh build; harness tolerances unchanged, `tests/tolerances_capture.json` 0 ULP)

| gate | rows | max abs | max rel | max ULP | notes |
|---|---|---|---|---|---|
| `capture.ship_construct` (new, dataset `ship_construct`, `tools/golden_runner/ds_ship_construct.cpp`) | `deltav_readout.json` (C5) table `craft`: 16 crafts x (`parts`, `dry_mass`, `fuel_mass`); table `parts`: 1,224 parts x (`part`, `mass`, `module_mass`) | 0 | 0 | 0 | the 17 craft files of the C5 capture go through `KSPUpgradePipeline.Process` (as `CreateConstructFromTemplate` does) and `ShipConstruct.LoadShip`; `GetShipMass(out dryMass, out fuelMass)` (UpdateMass of every part, Mono-rule float sums in the C# order) and `Part.mass` / `Part.moduleMass` per part. Mutation check: adding 1e-6 to the dry-mass accumulator fails 16 of 16 rows |
| `capture.resource_mass`, `capture.part_database`, `capture.vessel_state`, `corpus.gamedata`, every earlier gate | unchanged | 0 | 0 | 0 | green |

Excluded from the gate (and why):
- `craft_ComSat_Lx` (craft row) and its part 10 (parts row), via `tests/data/ship_construct_exclusions.json` (DROP_KEYS_FILE, with the reason per
  row): `fairingSize1`'s module mass is `ModuleProceduralFairing.panelMass`, the mass of the fairing panels the module builds (FairingPanel mesh
  generation, WP-26); until then `panelMass` is 0 and `GetModuleMass` returns `Mathf.Epsilon`. The craft's `dry_mass` includes it; the 16 other
  crafts, the other 17 parts of this craft and its `fuel_mass` are exact (the filter drops the whole craft row, so `fuel_mass` / `parts` of this one
  craft are not compared; compare.py reports its candidate rows as "extra", the known follow-up of NEXT-STEPS).
- `craft.total_mass`: the capture's editor session adds the crew (VesselCrewManifest: `(float)CrewCount * KerbalCrewMass`, the crew's inventory
  mass, its resource mass; ShipConstruct.cs:841-849), which is the editor package's. `tests/test_ship_construct.cpp` checks the formula
  instead: for each of the 16 crafts with a part-complete mass, exactly one crew count `n` makes `GetShipMass(manifest{n crew, n x per-kerbal
  inventory})` equal the captured total bit for bit (11 crewed crafts with 1-4 crew, 5 uncrewed). The per-kerbal inventory mass is computed from
  the part database, not a literal: kerbalEVA.cfg's `ModuleInventoryPart` DEFAULTPARTS are `evaChute` (mass 0.004) and `evaJetpack` (mass 0.02 plus
  its RESOURCE `EVA Propellant` 5 units at density 0.005), summed as `ModuleInventoryPart.GetModuleMass` does (ModuleInventoryPart.cs:1928-1959:
  `num += snapshot.mass * (float)Math.Max(1, quantity)`, then the resources last to first, `density * (float)amount`, times the quantity) =
  0.049 t (`ProtoCrewMember.InventoryMass`, ProtoCrewMember.cs:775), and the manifest adds it once per crew member (VesselCrewManifest.cs:131).
- `craft.stage_count`, `loaded`, `error`; `parts.inverseStage` / `decoupleStage` / `activationStage` / `isEngine` / `isDecoupler`: staging
  (WP-16), VesselDeltaV (WP-17), engines (WP-23). `craft.parts` is gated.

### Unit tests (`tests/test_ship_construct.cpp`: 26 cases, 274,042 assertions)

All expectations are derived from the decompiled control flow or are properties of original game data; no number is invented.
- Live load: all 57 loadable crafts (51 of `harness/inputs/Ships` + the 6 `analysis/physics/craft` PhysProf crafts; the other 35 files contain FAR /
  Pwings / MLP parts and are refused as in the game) give 4,282 `Part`s whose tree, links, symmetry, attach-node partners, ids and ownership equal
  the file-side record; one root each; `AreAllPartsConnected`, `isControllable`; refusal paths (unknown part, version too old, dangling link);
  `partTechAvailable`, id re-issue on a second load of the same ids, the editor hook.
- Round trip: R1 = `SaveShip(LoadShip(file))`; `LoadShip(R1)` -> `SaveShip` == R1 byte for byte for all 57 crafts (CRLF, no BOM).
- Agreement with the original files: for every craft, `pos` / `rot` / `attPos` / `attPos0` / `attRot` / `attRot0` / `mir` (as floats), `istg` /
  `dstg` / `sidx` / `sqor` / `sepI` / `attm` / `resPri` / `symMethod` / `autostrutMode` / `rigidAttachment` / `sameVesselCollision` / `cData` /
  `persistentId`, `link` and `sym` craftIDs, `srfN` (id, partner, vectors), every `attN` the file names (partner craftID), the file's RESOURCE
  amounts, 26,278 module values that the live module also saves, and `modCost` / `modMass` / `modSize` for the 3,807 parts without a FAR module or a
  procedural fairing are equal. The 452 FAR / fairing parts are where the live values differ (FAR's wing mass modifiers are a mod, the fairing
  mass is WP-26); the live part saves exactly the prefab's module list (the file's modules the prefab does not define are skipped by
  `Part.LoadModule`, prefab modules the file lacks keep their prefab values).
- ShipTemplate over every craft (name, description with U+00A8, type, size, part count, stage count = 1 + max `istg`, root part = the one
  unlinked PART, `isControllable`, flags) and the modded crafts (locked), duplicate parts, unknown part (logged, excluded from the totals), R&D /
  experimental / mission-filter hooks, a node without `part`, the first `type` wins.
- Costs and masses: `GetShipCosts` / `GetShipMass` equal the part sums in the C# order (dry and fuel accumulated separately from the last part to
  the first, Mono float rule); `GetTotalMass` with and without `ShipManifest`; `ShipTemplate` totals over the saved node agree with the ship's
  exactly: the template's `totalCost` / `fuelCost` / `dryCost` / `totalMass` equal the forward float32 sum of the per-part values
  (ShipTemplate.cs:129-143, Mono rule) bit for bit for all 57 crafts, and `GetPartCostsAndMass` per part equals the live computation exactly. The
  ship's own sum (dry and fuel separately, last part to first) differs from the template's only by association order: bit-equal for 37 of 57
  `totalCost` and 11 of 57 `totalMass`; `GetPartCostsAndMass` / `SanitizePartCosts` on a hand-built PART node (expected values from the C# order of
  operations and the stock resource definitions); `GetPartCosts(ProtoPartSnapshot)` on all 458 parts of the physprofile save (double arithmetic,
  dry / fuel split, `includeModuleCosts`, null `AvailablePart` = NullReferenceException, unknown resource).
- Costs against the part database (TIERING's acceptance "costs match part_database"): for all 4,282 parts of the 57 crafts, `AvailablePart.cost` and
  the live part's `GetModuleCosts(cost)` equal the C9 `parts.cost` / `parts.module_cost` bit for bit; the part's own resources give the C9
  `resource_cost` for 4,204 of them (the other 78 have tank sizes the craft changed); `GetShipCosts` (dry, fuel, total) equals the sum of the C9 values
  with the craft's resource amounts within the accumulation rounding.
- Names and paths: `KSPUtil.SanitizeString` table (20 cases derived from KSPUtil.cs:1505 incl. DOS device names and white-space), `GetSavePath` in
  every mode (sandbox, MISSION with and without Making History / a mission, MISSION_BUILDER), `GetShipsPathFor`, `GetPlayerCraftThumbnailName`
  both forms (the 2-argument form on a scratch directory tree), `CheckCraftFileType`.
- Files: `LoadShip(path)` / `SaveShip` / `SaveShipToPath` x2 / backups (`CreateBackup`, `ShiftAndCreateBackup`, `RestoreBackup`) / subassemblies /
  `LoadTemplate` / thumbnail hooks on a scratch directory; `CreateConstructFromTemplate` on a copy of Kerbal X: the pipeline upgrades the node,
  the file is rewritten and the original kept as `.original`, every part's `craftID` becomes its instance id.
- `AssembleForLaunch` for the three PhysProf crafts: part count, order, parents and attach modes equal the C10 `structure` table (631 rows) and
  WP-14's `AssembleVesselFromCraft`; launch bookkeeping (`launchID++`, unique flight ids, one `missionID`, flag, `landedAt`, `launchedFrom`,
  situation PRELAUNCH, crew manifest called once); splashed / not landed / orbit / default overload; `findFirst*`.
- Launch-site queries on the physprofile save (asteroid + probe on the LaunchPad): `FindVesselsLandedAt` both forms (substring match, `""` matches
  everything), `CheckLaunchSiteClear` (index, cleanup, `onVesselRecovered`), `RecoverVesselFromFlight`, `FindVesselsAtLaunchSite` (see Quirks).
- `AllPartsFound` / `IsCompatible` / `FindPartWithCraftID` / `SanitizeCraftIDs`; the `IShipConstructIDChanges` hand-over through a real part
  module; list operations and the four part constructors; vessel naming (priority, events, the editor root part hook); the ship's PartSet
  (`UpdateResourceSets`, totals through `Part.ship`, cache); craft size / centre / MOI over given renderer bounds and `PutShipToGround` over
  given collider bounds.

### Measured facts

- `GetShipMass` of the 17 C5 crafts equals the capture bit for bit; the capture's `total_mass` adds `n * 0.045 + n * 0.049` (see above).
- The old (1.6 era) stock crafts of `harness/inputs/Ships` carry `attN` entries with the prefab's node lists, `modCost` / `modMass` / `modSize`
  that the live parts reproduce exactly: the live `SaveShip` is therefore a real check of WP-13's mass / cost modifiers and attach-node lists.
- The cloud has no craft written by KSP 1.12.5 itself (`core/testdata`'s `.craft` files are zero-byte mirror placeholders; the PhysProf crafts
  are generated). Byte-identity of the live `SaveShip` against a 1.12.5-written craft is therefore not testable here; it is covered by the
  round trip and by the typed agreement above. `corpus.saves` (WP-04, local) keeps testing the file-side form.

### Quirks reproduced (a later implementer must keep them)

- A second load of the same craft re-issues every part id and the vessel id: `FlightGlobals.CheckVesselpersistentId` registers an id it has not
  seen even with `addNewId = false`, `CheckPartpersistentId` compares the part object. Clear `SaveEnvironment::persistentIds` between loads of
  the same file (a scene change does). `LoadShipWithParts` runs the part check with the real `Part*` (the file-side `LoadShip` alone registers
  stand-in keys, as WP-04 did).
- `ShipConstruct(EditorFacility)` / the part-list constructors leave `rotation` at `default(Quaternion)` = (0,0,0,0); only the default constructor
  sets identity. `SaveShip` always recomputes `size` (here: from renderer bounds when a part has any, else the loaded value is kept).
- `GetShipMass` adds the crew as `(float)CrewCount * KerbalCrewMass`, then `GetCrewInventoryMass()`, then `GetCrewResourceMass()`, each a float store.
- `GetPartCosts(ProtoPartSnapshot, ...)` reads `aP.cost` before it tests `aP == null`: a null `AvailablePart` is a NullReferenceException; the
  arithmetic is double (`(double)unitCost * amount`) with one float store at the start and at the end.
- `ShipTemplate.LoadShip`: `Dictionary.ContainsKey(null)` throws ArgumentNullException for a PART node without `part`; a duplicate `part` value
  aborts the whole read (`duplicatedParts`, nothing else is set); a non-PART node without `part` sets `partCount = 0`; unknown parts `continue`
  before their `istg` is read (stage count ignores them); the first `type` value of the file wins (the loop runs backwards).
- `ShipConstruction.AllPartsFound`: a part with ANY registered replacement counts as found, even when the replacement does not exist.
  `CheckCraftFileType`: every `type` other than `SPH` is `VAB`; no `type` is `None`.
- `GetPlayerCraftThumbnailName(game, "", ship)` names the empty sub-directory "Unnamed" (`SanitizeFilename` uses `replaceEmpty: true`); outside the
  mission modes the file's facility only decides which `Ships/<facility>` folder the root is measured from.
- `FindVesselsAtLaunchSite` ([Obsolete]) calls itself: the game dies with a StackOverflowException; the port throws `ksp::Exception`.
- `FindVesselsLandedAt(state, "")` matches every vessel (`"x".Contains("")`).
- A saved craft lists all the prefab's modules and attach nodes and keeps prefab resources the file lacks; it drops file modules the prefab does
  not define; `partName` is the prefab's (`Part`), not the 1.6-era value.
- `CreateConstructFromTemplate` rewrites the craft file with the upgraded node and keeps the original as `<file>.original` (`template.filename`);
  tests run it on a copy.
- `ShipConstruction.SaveShip(filename)` sanitises the file name but passes the raw name to the thumbnail.

### Deviations and seams

- `VesselCrewManifest` (editor, Making History-era crew UI) is an abstract seam (`CrewCount`, `GetCrewInventoryCosts` / `Mass` / `ResourceMass`,
  `AssignCrewToVessel`) the editor package implements; `AssembleForLaunch` requires a manifest (null = NullReferenceException, as in C#).
- `ShipConstructionEnvironment` (`ShipConstruction::env`) stands for HighLogic.SaveFolder / CurrentGame, EditorDriver, MissionSystem, Steam and
  the thumbnail UI; `ShipTemplateEnvironment` for ResearchAndDevelopment and the mission part filter; defaults are a sandbox game in the VAB.
- Part events / actions are the nodes the Part was loaded with (WP-13: `Part.Events` / `Actions` are not modelled), written back unevaluated.
- `Part.OnSave` is WP-13's empty virtual: PARTDATA of parts whose class saves data (CompoundPart `tgt`, struts) is lost on a live save until WP-25 /
  WP-26 port those classes (the file-side `SaveShip` still carries it). The 57 crafts contain such parts (fuel lines).
- Module `OnSave` values of modules that are not ported (docking node state, wheel state, reaction wheel `WheelState`, ...) are not written:
  see WP-06 "Module behaviours not ported". The typed agreement test only compares values the live module saves.
- `CalculateCraftSize` / `FindCraftCenter` need the renderer bounds a model reader fills (`Part::modelRenderers`); without them the size is
  zero and `SaveShip` keeps the loaded size. `FindCraftMOI` uses `Part.WCoM` without a rigidbody (Part.cs:1014) and the local root's transform
  rotation for `GetReferenceTransform().rotation`.
- `PutShipToGround`: collider bounds are `Part::colliderBounds` (world space, enabled non-layer-21 colliders of the part and its descendants);
  `Transform.Translate` / `RotateAround` / `Quaternion.ToAngleAxis` are native Unity and plain float32 here, not verified against a capture.
- `RecoverVesselFromFlight` / `CheckLaunchSiteClear` move the removed `ProtoVessel`s into a retained list (the Vessel's `protoVessel` pointer must not
  dangle; `Object.DestroyImmediate(vessel.gameObject)` is the host's hook `ShipConstruction::destroyVesselGameObject`).
- `AssembleVesselFromParts` calls `ShipConstruction::GetUniqueFlightID` for the per-part flight ids; `findFirstCrewablePart` starts from the ship's
  first part (`ship.parts[0]`, as the C# does) and `findFirstControlSource` scans `vessel.parts`; for the unchanged WP-14 route that first part is
  the root of the DFS, i.e. the old `vessel->parts[0]`.
- `ShipConstruct` is neither copyable nor movable (parts and PartSets point back at it); the GameEvents `onAboutToSaveShip` fires with the raw pointer.

### Needs a new (local) capture or scenario

- `launch_craft` scenario (local, needs the game): every stock craft spawned at the pad / runway; the launch-site transforms
  (`PSystemSetup` spawn points, `PutShipToGround`'s `spawnPoint`, `LaunchSite` GameObjects) are engine data, so `AssembleForLaunch`'s placement
  (`LaunchPlacement`) and `PutShipToGround` are unit-tested with given transforms only. A capture of the root part's world position / rotation and
  `Vessel.landedAt` / situation after `AssembleForLaunch` + physics start for 3-4 crafts would let the placement maths be gated.
- A craft file written by KSP 1.12.5 for each of the three `PhysProf` crafts (`EditorLogic` `SaveShip`) to compare the live `SaveShip` byte for byte
  (the PhysProf files in `analysis/physics/craft` are script-generated: one-vector attN lines).
- Fairing mass / cost (`ModuleProceduralFairing.panelMass`, WP-26): un-drop `craft_ComSat_Lx`.
- The editor crew manifest (default crew assignment, crew inventory): `craft.total_mass` could then be gated directly.
- `CalculateCraftSize` needs the part renderer bounds (the `.mu` model reader / UE import); the crafts' `size` lines are the reference (the
  harness corpus has them).
- Costs: no capture reports `ShipConstruct.GetShipCosts` or `ShipTemplate.totalCost`; the part-level costs (`AvailablePart.cost`, `module_cost`) are
  gated by `capture.part_database` and the ship level is unit-tested against them (TIERING's "costs match part_database"). A C5 variant that also
  logged `GetShipCosts` / `ShipTemplate.totalCost` / `dryCost` / `fuelCost` per craft would gate them.

### Fresh run

`./run_tests.sh -f` (Linux release, clang 18): 24/24 ctest tests passed (the 23 of WP-14 plus `capture.ship_construct`; `corpus.saves` is registered only
where `decompiled/KSP-dev/saves` exists, not on this VM); `unit` 262 cases / 452,479 assertions (26 cases / 274,042 assertions of them are
`test_ship_construct.cpp`); ctest total 63 s. GCC 13 (`./run_tests.sh -p linux-gcc-rel -f`): 24/24 as well (build 437 s cold). Not run here: Windows (MSVC) and Debug presets; the
class / struct forward-declaration rule was applied (`struct AvailablePart`, `struct PartHeightQuery`, `class VesselCrewManifest` is defined here, not
forward-declared differently from `game_event_types.hpp`).

### Commits (branch `wp18`)

see `git log --oneline wp14..wp18`.
