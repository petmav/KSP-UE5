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

| 10. WP-05 UrlDir, GameDatabase (data), PartLoader (data), resource library (branch `wp05`) | done for the data side: UrlDir + GameDatabase load order identical to KSP.log (2,836 lines), PartLoader compiles all 353 GameData parts + the 9 prefab-part slots; `capture.part_database` gate bit-exact on every in-scope C9 row (parts, cfg values, modules, cfg-set module fields, propellants, resources, attach nodes, drag cubes, variants, asset paths). Module instantiation is WP-06, model contents the UE import. See "WP-05" below |

## Test results (all 20 ctest tests pass in RelWithDebInfo and in Debug)

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
| `capture.part_database` | WP-05: in-game `captures/out2/part_database.json` (C9), scope in "WP-05" below: parts 357, part_cfg_values 6,235, modules 1,312, module_fields 5,401, module_propellants 77, resources 166, attach_nodes 822, drag_cubes 531, variants 228, assets 743 | 15,872 | 0 ULP, all strings identical |
| `capture.gamedatabase_load_order` | WP-05: every `Load(Audio/Texture/Model): url` and `Config(TYPE) url` line of the capture run's KSP.log, in order | 2,836 lines | identical |
| `unit` | 125 doctest cases, 39,849 assertions (WP-05: 5 cases, 207 assertions in `test_partloader.cpp`; WP-11: 13 cases, 20,081 assertions in `test_terrain.cpp`; WP-08: 9 cases, 374 assertions in `test_patched_conics.cpp`; WP-09: 8 cases in `test_celestial.cpp`; WP-07: 18 cases in `test_orbit.cpp`, incl. native Quaternion.AngleAxis 84/84 bit-exact vs the in-game C3 capture; integration: 2 cases, 934 assertions in `test_celestial_orbit.cpp`; WP-03: `test_events.cpp`, `test_util.cpp`, `test_wp03_reference.cpp` with 11,032 assertions against 6,437 values from KSP's IL on .NET Framework 4.8, `tests/data/wp03_reference.tsv`), incl. 29,028 .NET Framework 4.8 reference results (`tests/data/netfx_reference.tsv`: parse 6,041 doubles + 6,041 floats, G17/G/G9 formatting 7,118, Sin/Cos/Tan 3,005 each, Ceiling/Round 400 each) | - | pass |

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

- **VS2013 CRT dependency**: Mono and .NET Framework 4.8 use the Visual Studio 2013 CRT libm. kspcore loads it at run
  time from `msvcr120_clr0400.dll` (installed with .NET Framework 4.x on every Windows 10/11) or `msvcr120.dll`
  (`dotnet::LibmBackend()` reports which; it is part of `kspcore_build_info()`). Without it (non-Windows), kspcore
  falls back to `<cmath>`, which differs in the last bit for cosh (130/3,093 inputs), sinh (40), pow (17), tanh (8),
  asin (3), atan (2), exp (1), atan2 (1). A portable bit-exact port of those VS2013 routines is future work.
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

## WP-05 UrlDir, GameDatabase (data side), PartLoader (data side), resource library (branch `wp05`)

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
| parts | 362 | 357 x 54 cols | 0 ULP, strings identical | 5 prefab parts (kerbalEVA, kerbalEVAfemale, flag, kerbalEVASlimSuit(+Female)): base values are Unity-serialized prefab data, not GameData. The 4 DLC-suit error rows ARE gated (name + "NullReferenceException: ..."). Columns partSize (renderer bounds of the model), module_mass / module_cost (IPartMassModifier / IPartCostModifier modules: 7 parts non-zero, WP-06) |
| part_cfg_values | 6,235 | 6,235 | identical (incl. 219 float-parsed doubles) | - |
| modules | 1,378 | 1,312 | identical (moduleName, class, field_count, isEnabled) | 66 rows of the 5 prefab parts |
| module_fields | 34,801 | 5,401 | identical (value text, value_num, Type.Name, isPersistant) | rows the part cfg does not set (KSPField defaults and module OnAwake/OnLoad results, WP-06; `tools/wp05/estimate_module_defaults.py`: 27,391 of those rows equal the field's literal C# initializer); 1 row listed in `tests/data/part_database_exclusions.json` (sepMotor1 stackableQuantity: ModuleCargoPart.OnLoad clamps it to 1 because the part has resources) |
| module_propellants | 77 | 77 | identical | - |
| resources | 166 | 166 | identical | - |
| attach_nodes | 827 | 822 | identical | 5 prefab parts |
| drag_cubes | 535 | 531 | identical incl. SaveToString | 4 EVA cubes built in code |
| variants | 228 | 228 | identical (name, displayName, cost, mass) | column materials (the model's materials) |
| assets | 3,978 | 743 (cfg 353, model 359, texture_replacement 15, internal 16) | identical; `idx` renumbered per part | 3,230 renderer texture-slot rows (2,497 GameData + 733 Unity assets: need the .mu contents / Unity assets), 5 prefab-part cfg rows |

Scope mechanics (all in `tests/CMakeLists.txt`, filters in `tools/filter_golden.py`): DROP_ROWS (prefab parts,
texture rows), KEEP_ROWS_FILE (`<candidate>/part_database/scope.json`, written by the runner from the inputs: the
module fields each MODULE node sets), DROP_KEYS_FILE (individual rows with reasons), RENUMBER (assets.idx),
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

`wp05` (from `f964447`) merged into main on top of WP-08 and WP-11.

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
matched.

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
