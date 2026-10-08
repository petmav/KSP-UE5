# kspcore status

Updated 2026-10-09. One command: `powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\core\run_tests.ps1`
(details in README.md). Local git repository in `core/` only, never pushed.

## Summary

| deliverable | state |
|---|---|
| 1. Scaffold | done: CMake 3.28+/C++20/MSVC 14.44 presets (Debug, RelWithDebInfo; Ninja), `run_tests.ps1`, vendored doctest 2.4.12 + nlohmann/json 3.12.0 (sha256 in `third_party/VERSIONS.md`), golden gate (`golden_runner` + `harness/compare/compare.py`) as ctest tests, Unreal link settings (/MD, IDL 0, /Z7, no /GL) with `build/RelWithDebInfo/kspcore.lib`, C ABI `include/kspcore/kspcore_c.h` |
| 2. WP-01 maths + Unity shims + FloatCurve | done, all 19 package files (except the native-only Unity functions listed below); every gate bit-exact |
| 3. WP-02 ConfigNode | done: Load/Parse/Save/ToString, value/node API, typed getters/setters, ParseExtensions, Guid, KSPParseable, type parsers; byte-exact on the whole corpus. UrlDir (GameData tree) deferred, see below |
| extra | in-game (Mono) runtime semantics from the CAPTURE-SPEC C2/C7 captures: System.Math, float32 evaluation rule, conversions, formatting, System.Random, UnityEngine.Random, KSPRandom |
| 4. WP-09 celestial bodies, atmospheres, PhysicsGlobals (branch `wp09`) | done for the assigned scope: PhysicsGlobals (all fields/curves/properties, LoadDatabase/SaveDatabase), CelestialBody (data model, Awake/Start/CBUpdate/SetupConstants, atmosphere, frames, lat/lon), Planetarium, PSystem spawn + FlightGlobals list, Kerbol catalog of 17 bodies from the game's prefab + asset index, FlightIntegrator atmosphere helpers; 4 new gates bit-exact. Pending WP-07 integration and deferred items: see "WP-09" below |

## Test results (all 14 ctest tests pass in RelWithDebInfo and in Debug)

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
| `corpus.gamedata` | self-consistency: 368 GameData `.cfg` + 86 `.craft` (8.55 MB, 241,219 values, 77,117 nodes): all load, Save->Load->Save byte-identical, Parse == Load, 226 FloatCurve nodes load | 454 files | 0 failures |
| `capi.c`, `capi.ue_like` | C ABI from pure C and from C++ with UE flags (/GR- /EHs-c- _HAS_EXCEPTIONS=0 /fp:fast); FloatCurve values bit-exact vs golden through the ABI | - | pass |
| `golden.physics_constants` | harness golden `physics_constants` (WP-09): every PhysicsGlobals field, exact value text + source layer | 213 | exact (0 ULP, all strings identical) |
| `capture.physics_drag_curve` | in-game `captures/out/aero_forces_mono.json` table drag_curve_value: PhysicsGlobals.DragCurveValue under Mono | 2,870 | 0 ULP |
| `golden.body_catalog` | harness golden `body_catalog` (WP-09): bodies 17 x 51 cols, orbits 16 x 9 (elements only), atmosphere_profile_1km 1,072, body_curves 265 | 1,370 | 0 ULP |
| `golden.atmosphere_curves` | harness golden `atmosphere_curves` (WP-09): profile_100m 2,704, flight_state 2,176 x 19 cols, solar_effects 320 | 5,200 | 0 ULP |
| `unit` | 46 doctest cases, 6,221 assertions (WP-09: 8 cases, 5,744 assertions in `test_celestial.cpp`), incl. 29,028 .NET Framework 4.8 reference results (`tests/data/netfx_reference.tsv`: parse 6,041 doubles + 6,041 floats, G17/G/G9 formatting 7,118, Sin/Cos/Tan 3,005 each, Ceiling/Round 400 each) | - | pass |

Not gated any more by design: the harness's .NET-generated float32 math_shim rows. kspcore follows Mono (the game) and
differs from that .NET golden in 46 cells outside its tolerance (r0..r2): up to 6 ULP (Vector3.Project/ProjectOnPlane) and
`Mathf.Repeat(1e20f, 360f)` = 360 (Mono) vs 0 (.NET); kspcore equals the in-game capture in all 903 rows.

## Build and test times on this machine (laptop, MSVC 14.44, Ninja)

| | configure | build (from clean) | ctest (10 tests) |
|---|---|---|---|
| RelWithDebInfo | 2.0 s | 12.6 s | 12.9 s (capture.libm_mono 8.8 s, corpus 2.4 s) |
| Debug | 1.9 s | 6.4 s | 33.2 s (capture.libm_mono 20.1 s, corpus 9.8 s) |

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

## Incomplete, and why

- **WP-02 UrlDir/UrlFile/UrlConfig (GameData tree, UrlDir.cs, 927 lines)**: not ported. It crawls GameData with
  selective-expansion / limited-parts options, file-type tables and Localizer hooks that belong with GameDatabase
  (WP-05), and there is no golden for it yet; the `part_database` golden of WP-05 is the natural gate. ConfigNode,
  which it uses, is complete.
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

## WP-09: celestial bodies, atmospheres, PhysicsGlobals (branch `wp09`, not merged)

Spec: `CelestialBody.cs`, `PhysicsGlobals.cs`, `Planetarium.cs`, `PSystemManager.cs` (SpawnBody/PostSetup),
`PSystemBody.cs`, `CBAttributeMapSO.cs`, `CelestialBodyScienceParams.cs`, `VesselRanges.cs`, `FlightGlobals.cs`
(GetHomeBody/GetBodyByName), `FlightIntegrator.cs:808-985`; every ported function cites its line.

### Gates (RelWithDebInfo and Debug; all bit-exact)

| gate / table | rows | numeric columns (f32) | max abs | max ULP | notes |
|---|---|---|---|---|---|
| `golden.physics_constants` / physicsglobals | 213 | value_f64 (+ exact `value` text, `type`, `source`) | 0 | 0 | prefab + Physics.cfg + CalculateValues' solarLuminosity |
| `capture.physics_drag_curve` / drag_curve_value | 2,870 | value (f32) | 0 | 0 | in-game Mono capture; inputs AeroGen.cs:72-78 |
| `golden.body_catalog` / bodies | 17 | 34 (inverseRotThresholdAltitude f32) | 0 | 0 | tidal-lock `rotationPeriod` and `solarDayLength` use the gate-only `StandInOrbitPeriod` (Orbit.cs:334/370 period formula) |
| `golden.body_catalog` / orbits | 16 | 7 | 0 | 0 | serialized elements only (COLUMNS filter) |
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

### Pending WP-07 integration (excluded from the gate on purpose)

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
| next commit | unit tests `tests/test_celestial.cpp`, STATUS/README |

