# kspcore status

Updated 2026-10-09. One command: `powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\core\run_tests.ps1`
(details in README.md). Local git repository in `core/` only, never pushed.

## Summary

| deliverable | state |
|---|---|
| 1. Scaffold | done: CMake 3.28+/C++20/MSVC 14.44 presets (Debug, RelWithDebInfo; Ninja), `run_tests.ps1`, vendored doctest 2.4.12 + nlohmann/json 3.12.0 (sha256 in `third_party/VERSIONS.md`), golden gate (`golden_runner` + `harness/compare/compare.py`) as ctest tests, Unreal link settings (/MD, IDL 0, /Z7, no /GL) with `build/RelWithDebInfo/kspcore.lib`, C ABI `include/kspcore/kspcore_c.h` |
| 2. WP-01 maths + Unity shims + FloatCurve | done, all 19 package files (except the native-only Unity functions listed below); every gate bit-exact |
| 3. WP-02 ConfigNode | done: Load/Parse/Save/ToString, value/node API, typed getters/setters, ParseExtensions, Guid, KSPParseable, type parsers; byte-exact on the whole corpus. UrlDir (GameData tree) deferred, see below |
| 4. WP-07 Orbit (branch `wp07`) | done: Orbit.cs (everything but DrawOrbit), Targeting.cs (closest-approach intercept search behind `Orbit.FindClosestPoints`), OrbitSnapshot, OrbitUtil (but CalculateIntersections), FoRModes/VesselTargetModes; `CelestialBodyRef` / `PlanetariumFrames` interfaces for WP-09; golden `orbit_propagation` bit-exact (all 5 tables). TransferMath, DirectionTarget, ITargetable deferred (see "WP-07 Orbit") |
| extra | in-game (Mono) runtime semantics from the CAPTURE-SPEC C2/C7 captures: System.Math, float32 evaluation rule, conversions, formatting, System.Random, UnityEngine.Random, KSPRandom |

## Test results (all 11 ctest tests pass in RelWithDebInfo and in Debug)

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
| `unit` | 56 doctest cases, 819 assertions (WP-07: 18 cases in `tests/test_orbit.cpp`, incl. native Quaternion.AngleAxis 84/84 bit-exact vs the in-game C3 capture), incl. 29,028 .NET Framework 4.8 reference results (`tests/data/netfx_reference.tsv`: parse 6,041 doubles + 6,041 floats, G17/G/G9 formatting 7,118, Sin/Cos/Tan 3,005 each, Ceiling/Round 400 each) | - | pass |

Not gated any more by design: the harness's .NET-generated float32 math_shim rows. kspcore follows Mono (the game) and
differs from that .NET golden in 46 cells outside its tolerance (r0..r2): up to 6 ULP (Vector3.Project/ProjectOnPlane) and
`Mathf.Repeat(1e20f, 360f)` = 360 (Mono) vs 0 (.NET); kspcore equals the in-game capture in all 903 rows.

## WP-07 Orbit (branch `wp07`)

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

## Build and test times on this machine (laptop, MSVC 14.44, Ninja)

| | configure | build (from clean) | ctest (10 tests) |
|---|---|---|---|
| RelWithDebInfo | 2.0 s | 12.6 s | 12.9 s (capture.libm_mono 8.8 s, corpus 2.4 s) |
| Debug | 1.9 s | 6.4 s | 33.2 s (capture.libm_mono 20.1 s, corpus 9.8 s) |

With WP-07 (branch `wp07`, 11 tests): RelWithDebInfo ctest 13.0 s (golden.orbit_propagation 0.4 s, unit 0.2 s), Debug
33.9 s (golden.orbit_propagation 0.9 s, unit 0.5 s).

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

Branch `wp07` (from `c051578`, not merged): `431b44d` WP-01 fixes needed by Orbit (Math.Min/Max, System.Math in Angle
helpers, golden number text); `03e2310` Orbit + CelestialBodyRef/PlanetariumFrames + orbit_propagation gate;
`6073a3f` Orbit unit tests, native AngleAxis bit-exact; `d982b62` Targeting; `7e5a33a` test speed-up; `10d3aea`
OrbitSnapshot, OrbitUtil, enums; plus the STATUS commit.

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
- A worktree build (`core/.worktrees/<wp>`) must be configured with `-DKSP_PROJECT_ROOT=C:/Projects/KSP-UE5` (the
  default `<source>/..` points at `.worktrees/`); the value stays in the CMake cache for later `run_tests.ps1` runs.

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
