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
| 4. WP-09 celestial bodies, atmospheres, PhysicsGlobals (branch `wp09`, merged) | done for the assigned scope: PhysicsGlobals (all fields/curves/properties, LoadDatabase/SaveDatabase), CelestialBody (data model, Awake/Start/CBUpdate/SetupConstants, atmosphere, frames, lat/lon), Planetarium, PSystem spawn + FlightGlobals list, Kerbol catalog of 17 bodies from the game's prefab + asset index, FlightIntegrator atmosphere helpers; 4 new gates bit-exact. Pending WP-07 integration and deferred items: see "WP-09" below |
| 5. WP-07 Orbit (branch `wp07`, merged) | done: Orbit.cs (everything but DrawOrbit), Targeting.cs (closest-approach intercept search behind `Orbit.FindClosestPoints`), OrbitSnapshot, OrbitUtil (but CalculateIntersections), FoRModes/VesselTargetModes; `CelestialBodyRef` / `PlanetariumFrames` interfaces for WP-09; golden `orbit_propagation` bit-exact (all 5 tables). TransferMath, DirectionTarget, ITargetable deferred (see "WP-07 Orbit") |
| 6. Integration WP-07/WP-09 (main) | done: Orbit on `ksp::CelestialBody` (CelestialBodyRef deleted), one Planetarium (PlanetariumFrames deleted), `CelestialBodyOrbitAdapter` (OrbitDriver for bodies over ksp::Orbit) attached by `CelestialSystem::Spawn` by default, `body_catalog/orbits` gated in full, integration unit test. See "Integration WP-07/WP-09" below |

## Test results (all 15 ctest tests pass in RelWithDebInfo and in Debug)

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
| `unit` | 66 doctest cases, 7,508 assertions (WP-09: 8 cases in `test_celestial.cpp`; WP-07: 18 cases in `test_orbit.cpp`, incl. native Quaternion.AngleAxis 84/84 bit-exact vs the in-game C3 capture; integration: 2 cases, 934 assertions in `test_celestial_orbit.cpp`), incl. 29,028 .NET Framework 4.8 reference results (`tests/data/netfx_reference.tsv`: parse 6,041 doubles + 6,041 floats, G17/G/G9 formatting 7,118, Sin/Cos/Tan 3,005 each, Ceiling/Round 400 each) | - | pass |

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
Still using `<cmath>` in WP-01 code that Orbit does not call (noted, not changed): `UtilMath.Flatten/RoundToPlaces`
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

## Integration WP-07/WP-09 (main)

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

### Deferred / for later packages

- `Planetarium.Orbits` (the OrbitDriver list patched conics read) is not kept; WP-08's `patched_conics` gate will need it.
- The harness's bodies never move (its OrbitDriver rehost only calls `Orbit.UpdateFromUT`), kspcore's spawn moves them
  (`updateFromParameters`); any golden column that depends on `CelestialBody.position` (patched_conics:
  getTruePositionAtUT, GetFrameVel chains) must take that into account in its runner.
- Vessel OrbitDrivers (TRACK_Phys / IDLE, CheckDominantBody, SOI transitions) are WP-10; the adapter throws for
  non-UPDATE modes (every stock body uses UPDATE).
- `GetAtmoThermalStats` now has its Orbit and native AngleAxis inputs; it still needs scaled-space transforms.
- Scene setup (`PSystemSetup.OnSceneChange` / `SetSpaceCentre` / `SetFlight`: clear inverseRotation/reverse, home body
  inverse rotation) is not ported; until then a spawned system keeps the prefab's Mun/Minmus inverse-rotation state.
