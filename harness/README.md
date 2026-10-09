# KSP golden-output harness

Reference ("golden") outputs computed by **KSP 1.12.5's own code**, for diffing the C++ simulation core against. The
harness runs KSP's Assembly-CSharp on the .NET Framework 4.8 CLR (x64) outside Unity, builds the minimal game state the
way KSP does (prefab values from the shipped Unity assets, Physics.cfg, stock part cfgs), calls KSP's methods directly,
and writes typed, exact JSON tables plus a comparison tool.

## Run (one command)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\harness\run.ps1
#   -Resync          re-copy inputs\ and lib\ from decompiled\KSP-dev and the built DLL
#   -Extract         re-extract golden\serialized\ from the Unity asset files (needs the Python venv)
#   -SkipCrossCheck  skip the second run against the shipped IL
```

What it does: copies inputs if missing (`sync-inputs.ps1`), extracts prefab values if missing (`tools/extract_assets.py`),
builds (`build.ps1`), regenerates `golden\`, then regenerates the same datasets with KSP's **shipped** IL and diffs the
two with zero tolerance (`golden\CROSSCHECK-shipped-il.json`). Generation takes ~3 s; the first build needs network
access for two NuGet packages. Outputs are deterministic (byte-identical across runs).

Requirements: Windows with .NET Framework 4.8 (built in), a .NET 8 SDK (system `dotnet`, or the portable one at
`decompiled\tools\dotnet-sdk`), Python 3.12 (`python -I` for compare.py; a venv in `tools-cache\venv` with UnityPy and
TypeTreeGeneratorAPI only for `-Extract`). Nothing is installed system-wide; NuGet packages go to `tools-cache\nuget-packages`.

## Compare a C++ result

```powershell
python -I harness\compare\compare.py harness\golden <candidate_dir> --tol harness\compare\tolerances.json --report report.json
python -I harness\compare\compare.py harness\golden\orbit_propagation\orbit_propagation.json candidate.json --tol harness\compare\tolerances.json
```

The candidate writes the same schema (copy the golden file, replace output columns). Rows are joined on each table's
`key` columns. Per numeric column the tool prints n, max abs error, max relative error, max ULP distance (float32 ULPs for
`f32` columns, float64 for `f64`) and fails, with the worst row's key; strings/bools/ints must match exactly. Exit code 0 =
pass, 1 = failures or missing rows/columns.

## Datasets (golden\, 10 files, 71,566 rows)

| dataset | file | rows | trust | contents |
|---|---|---|---|---|
| curves_eval | `curves_eval/curves_eval.json` | 29,905 | direct + rehosted-native | 366 FloatCurves (PhysicsGlobals drag/lift sets, all body curves, every `key` node in 359 stock part cfgs, 14 synthetic edge cases): keys after load, 28,484 evaluations incl. key times, their float neighbours, midpoints, out-of-range |
| body_catalog | `body_catalog/body_catalog.json` | 1,370 | direct + rehosted | 17 bodies: radius, GeeASL, Mass, mu, SOI, Hill sphere, rotation (sidereal, tidal lock), atmosphere constants; 16 orbits (elements + UT 0 state); p/T/rho/c at 1 km steps for 7 atmospheres; all body curve keys |
| orbit_propagation | `orbit_propagation/orbit_propagation.json` | 2,422 | direct | 92 orbits around Kerbin/Mun/Kerbol/Jool/Eve/Minmus (circular to e=4.84, all solver branches, inc 0..180): anomalies, state vectors, UpdateFromUT values, time to Pe/Ap at 1,020 UTs; inverse via UpdateFromStateVectors; 9 hand-made/degenerate state vectors; 281-point Kepler solver grid |
| patched_conics | `patched_conics/patched_conics.json` | 11 | direct + rehosted | 5 trajectories through PatchedConics.CalculatePatch: LEO, Kerbin->Mun encounter->escape->Kerbin->Kerbol, Kerbin escape, Mun escape, Mun/Minmus-crossing orbit |
| atmosphere_curves | `atmosphere_curves/atmosphere_curves.json` | 5,200 | direct | 100 m (Kerbin) / 500 m profiles; 2,176 FlightIntegrator flight states (pressure, temperature with offset, density, q, c, Mach, solar air mass, shock temperature, convective coefficients, pseudo-Reynolds drag multiplier); solar effects |
| aero_forces | `aero_forces/aero_forces.json` | 25,694 | direct + rehosted | surface drag curve grid; 16 stock parts' drag cubes (+ per-cube variants) x 41 flow directions x 13 Mach: area, drag area, Cd, cross-section, exposed area, depth, taper, body-lift vector; Kerbin drag chain to force scalars; lifting surface lift/drag vs AoA/Mach |
| physics_constants | `physics_constants/physics_constants.json` | 213 | direct | every PhysicsGlobals value after load, with its source (Physics.cfg / prefab / static) |
| confignode_roundtrip | `confignode_roundtrip/` | 68 (+45 saved files, 45 trees) | direct | 14 stock craft, 12 stock cfgs (Physics, PartDatabase, TechTree, ResourcesGeneric, parts), 21 synthetic edge cases: exact Save bytes, ToString, parse tree, Parse==Load, resave stability |
| engine_thrust | `engine_thrust/engine_thrust.json` | 5,248 | direct + rehosted | all 41 stock engine modules: loaded parameters, fuel flows, Isp/thrust vs pressure, thrust in Kerbin flight (jets: atm/vel curves), rocket-equation dV in KSP's float form |
| math_shim | `math_shim/math_shim.json` | 1,435 | direct | managed Unity float math (Mathf, Vector3, Quaternion operators) and KSP double math (Vector3d, QuaternionD, UtilMath, CelestialFrame) |
| (inputs) | `serialized/*.serialized.json` | - | serialized | prefab values extracted from the asset files (see serialized/README.md) |

Every dataset directory has a generated `README.md` (inputs, units, frames, columns, exact KSP methods). `MANIFEST.json`
lists files and row counts; `_run.log` holds non-golden run notes.

## Trust levels

- **direct** (highest): the value is returned by KSP's own IL, executed on a copy of Assembly-CSharp. The only edits to
  that copy are (1) TypeRefs `UnityEngine.AnimationCurve`/`Gradient` retargeted to managed ports and (2) direct call
  sites of Unity InternalCalls redirected to throwing stubs (needed because the .NET Framework JIT rejects a whole method
  that merely references an InternalCall). No KSP method body logic is changed; `build/patched/patch-report.txt` lists
  every retarget and all 16,685 stubbed call sites. Each run is repeated on the **shipped** IL (de4dot of the stock DLL):
  the rebuilt-from-decompile IL and the shipped IL agree **bit for bit on all 518,141 compared values**.
- **rehosted** (high): a few KSP statements copied verbatim (with file:line citations) because the surrounding method
  begins with a Transform/scene access: CelestialBody Awake/Start/CBUpdate numeric lines, PhysicsGlobals.CalculateValues,
  ModuleEngines.OnLoad fuel-flow lines, PatchedConicSolver's patch loop, UpdateAerodynamics' force-scalar lines,
  ModuleLiftingSurface.SetupCoefficients' vector lines, DeltaVStageInfo's dV expression. Files: `src/KspGolden/Rehosted/`
  and the generators (search "re-hosted").
- **rehosted-native** (high, unverified against the engine): Unity's native `AnimationCurveTpl<float>` (under every
  FloatCurve) ported op-for-op from the disassembly of the shipped UnityPlayer.dll using Unity's public PDB
  (`tools/native/`). Bit-exact by construction but not yet executed against native code; CAPTURE-SPEC C1 closes this.
- **serialized**: read from the shipped asset files (golden/serialized).
- **capture-spec only** (not produced): see CAPTURE-SPEC.md (C1 FloatCurve native check, C2 Unity Random, C3 native
  quaternion maths, C4 latitude/sun temperature offsets, C5 stock delta-v readout, C6 thermal integration, C7 Mono libm and
  float32 mode, C8 in-game rails propagation, C9 loaded part database, C10 drag on an assembled vessel).

## Tolerances (compare/tolerances.json)

| quantity | recommended | why |
|---|---|---|
| curves_eval, drag_curve_value, cube_weighted, physics_constants, serialized body curves, ConfigNode bytes/strings | exact (0 ULP) | float32 +,*,/ in a fixed order, no libm; strings |
| body_catalog bodies | rel 1e-14 or 4 ULP | products/quotients and one pow |
| orbit positions (m) / velocities (m/s) | abs 1e-6 or rel 1e-11 | sin/cos/atan2/sinh in the CRT differ by an ulp between runtimes; Kepler iteration |
| anomalies (rad), times (s) | abs 1e-7 or rel 1e-11 | same |
| Kepler E | abs 1e-9 | solvers stop at abs(dE) <= 1e-7: a 1-ulp input change can change the final iterate |
| inverse (UpdateFromStateVectors) | abs 1e-7 or rel 1e-10 | ill-conditioned for e ~ 0 and i ~ 0/180 (argPe/LAN undefined) |
| patched conics UTs / elements | abs 1e-3 or rel 1e-8 | iterative SOI/closest-approach solvers, epsilon 1e-4 |
| atmosphere f64 | rel 1e-12 | pow/sqrt on top of exact curves |
| f32 outputs fed by double maths (aero, engines, atmosphere f32) | 2-4 ULP | double->float casts after libm |
| math_shim | 1 ULP (f32), 4 ULP (f64) | Mathf wraps System.Math |

Calibrate these once C7 (Mono libm, float32 mode) is captured; the bit-exact rows should stay at 0.

## KSP methods used as oracles

FloatCurve.Load/Add/Evaluate; CelestialBody.SetupConstants, GetPressure, GetPressureAtm, GetTemperature,
GetFullTemperature, GetDensity, GetSpeedOfSound, GetSolarAtmosphericEffects, GetSolarPowerFactor; Orbit (ctor/SetOrbit/Init,
UpdateFromUT, getObtAtUT, getMeanAnomalyAtUT, EccentricAnomalyAtUT, TrueAnomalyAtUT, getRelativePositionAtUT,
getOrbitalVelocityAtUT, GetOrbitalStateVectorsAtUT, GetTimeToPeriapsis, GetNextPeriapsisTime, GetNextApoapsisTime,
UpdateFromStateVectors, solveEccentricAnomaly, GetTrueAnomaly, GetEccentricAnomaly, GetMeanAnomaly);
PatchedConics.CalculatePatch (and CheckEncounter, EncountersBody, Orbit.FindClosestPoints/SolveClosestApproach/SolveSOI,
UpdateFromOrbitAtUT inside it); Planetarium.Awake, CelestialFrame.SetFrame/OrbitalFrame/PlanetaryFrame;
PhysicsGlobals.LoadDatabase, DragCurveValue, GetLiftingSurfaceCurve and static constants; FlightIntegrator.CalculatePressure,
CalculateConstantsAtmosphere, CalculateShockTemperature, CalculateConvectiveCoefficient(+Newtonian/Mach),
CalculateDensityThermalLerp, CalculateBackgroundRadiationTemperature, CalculateDragValue; DragCubeList.LoadCubes, SetPart,
SetCubeWeight, SetDragWeights, SetPartOcclusion, SetDrag (AddSurfaceDragDirection); ModuleLiftingSurface.GetLiftVector,
GetDragVector; BaseFieldList.Load; Propellant.Load; PartResourceDefinitionList.Add; ModuleEngines.SetupPropellant,
MaxThrustOutputAtm, MaxThrustOutputVac, GetEngineThrust, getExhaustVelocity, getMaxFuelFlow; ConfigNode.Load, Parse,
Save, ToString; Vector3d/QuaternionD/UtilMath; Unity managed Mathf/Vector3/Quaternion.

## How the world is built (KspEnv.cs)

1. Culture: `CultureInfo.CreateSpecificCulture("en")` (HighLogic.cs:125) with Mono's infinity/NaN symbols (below).
2. PhysicsGlobals: `new PhysicsGlobals()` -> level0 prefab fields -> Awake's SurfaceCurves -> real `LoadDatabase()` on
   `inputs/Physics.cfg` -> Start's bodyLiftCurve.
3. Planetarium: level9 fields -> real `Awake` (Zup, rotation).
4. Bodies: for each PSystemBody in spawn order: `new CelestialBody()` + prefab fields, `new OrbitDriver()` + prefab
   orbit, parent wiring as PSystemManager.SpawnBody; Awake (re-hosted), Start (re-hosted + real SetupConstants),
   real `Orbit.Init()` + `UpdateFromUT(0)`, CBUpdate in Planetarium.UpdateCBsRecursive order (re-hosted).
5. HighLogic.CurrentGame with default GameParameters (Normal: ReentryHeatScale 1); PartResourceLibrary from the stock
   RESOURCE_DEFINITION nodes.

Hosting tricks: Unity's fake-null is `m_CachedPtr == 0`, so hosted objects get a dummy non-zero pointer
(`U.Alive`); Unity's log handler is replaced by a capturing one.

## Surprises a C++ implementer must know

1. **FloatCurve is Unity's native AnimationCurve**, not textbook Hermite: per segment it builds a cached cubic in the
   absolute offset u = t - key.time, `((a*u+b)*u+c)*u+d`, in float32, with segment length clamped to >= 1e-4; only a
   `+Infinity` tangent makes a step; outside the key range it clamps (t >= last key returns the last value exactly);
   `Evaluate(+Inf)` is NaN. Exact formula in `golden/curves_eval/README.md`.
2. **2-value cfg keys get Unity's AddKey smoothing at insertion time**: the new key and both neighbours are recomputed
   (one-sided slope at the ends, average of left/right slopes in the middle, slope 0 if |dx| <= 1e-5). A later 2-value key
   overwrites the *explicit* tangents of its neighbours. Keys with a time that already exists are silently dropped. 3 or
   5 values on a key line are treated as 2 values.
3. **Mono culture**: KSP's Mono uses "Infinity"/"-Infinity"/"NaN" for en-US (verified in the shipped
   mono-2.0-bdwgc.dll string table); cfg values like `Infinity` parse in game. .NET on current Windows uses "∞".
4. **Body data are float-derived doubles** (e = 0.200000002980232, Kerbin sma 13599840256, adiabatic index
   1.39999997615814). mu = (R²·GeeASL·g0 / G)·G with G = 6.67408e-11 (Kerbin 3531599999999.9995, not 3.5316e12).
   Kerbin's 21600 s day is a *solar* period converted at Start to sidereal 21549.425183089825 s. Tidally locked bodies
   overwrite rotationPeriod with the orbital period every frame. Kerbol has an "atmosphere" (600 km). Kerbin's
   OrbitDriver has `reverse = true`.
5. **Kepler solver branches** at e < 0.8 (Newton, |dE| <= 1e-7, no iteration cap), 0.8 <= e < 1 (8 fixed iterations,
   no convergence test), e >= 1 (hyperbolic Newton). The solvers read the `eccentricity` field and the `ecc` argument
   inconsistently (initial guess vs iteration).
6. `Orbit.GetTimeToPeriapsis()` on an orbit that is not a solved patch (patchEndTransition INITIAL, EndUT 0) returns
   timeToPe - period (negative).
7. FlightIntegrator computes solar air-mass effects with the **previous frame's density**; first-frame values differ.
8. ConfigNode: `//` starts a comment anywhere (URLs truncate), comments are dropped, `NAME (id)` ids survive ToString but
   not Save, a stray `}` at root ends the file, bare tokens are dropped, Save writes CRLF and UTF-8 without BOM.
9. Float32 everywhere in aero/engine paths (DragCubeList, ModuleEngines thrust): e.g. MaxThrustOutputVac of
   `liquidEngine3_v2` (maxThrust 60) is 60.000004 kN.
10. QuaternionD.Euler/FromToRotation/Slerp/Lerp/AxisAngle are `extern` declarations in Assembly-CSharp that Unity never
    registers (dead in game); QuaternionD.Inverse is a plain conjugate (assumes unit quaternions).
11. On the .NET Framework, any KSP method that *references* a Unity InternalCall cannot be JIT-compiled (hence the stubs);
    FlightGlobals cannot be type-initialised at all (static `Quaternion.Euler`), GameParameters needs
    `AssemblyLoader.loadedAssemblies`.

## Not covered / limitations

- Thermal integration, delta-v readout on whole craft, latitude/sun temperature offsets, Unity Random and native
  quaternion maths: capture-spec only (CAPTURE-SPEC.md).
- The harness runs System.Math from the .NET Framework CRT, not Unity Mono's; transcendental-dependent values may differ
  from in-game values by an ulp (C7). Mono's float32 evaluation mode is unverified (C7).
- Drag cubes are evaluated on isolated parts (no attach-node occlusion); PQS/terrain-dependent values (minOrbitalDistance,
  terrain heights) are not produced.
- Running UnityPlayer.dll natively in-process to verify the AnimationCurve port was attempted and blocked by the
  permission system; the alternative is CAPTURE-SPEC C1.

## Layout

| path | what |
|---|---|
| `run.ps1`, `build.ps1`, `sync-inputs.ps1` | entry points |
| `src/KspUnityShim/` | managed ports of Unity's AnimationCurve (native port) and Gradient (stub) |
| `src/AsmPatcher/` | Mono.Cecil tool producing the patched Assembly-CSharp copies |
| `src/KspGolden/` | generator: `KspEnv.cs` (world), `Generators/`, `Rehosted/`, `Infra/` (exact numbers, JSON, tables, Unity hosting) |
| `compare/compare.py`, `compare/tolerances.json` | comparison tool and tolerance rules |
| `tools/extract_assets.py` | prefab extraction from the Unity assets |
| `tools/native/` | PDB public-symbol reader and capstone disassembler used to port Unity's AnimationCurve |
| `lib/` | copies of the game/Unity assemblies (rebuilt Assembly-CSharp sha256 6c9b58c6..., shipped de4dot IL c533ce20...) |
| `inputs/` | copies of Physics.cfg, PartDatabase.cfg, stock part/resource cfgs, Ships |
| `golden/` | outputs |
| `build/`, `tools-cache/` | regenerable (build outputs, venv, NuGet cache, Unity PDB) |
| `CAPTURE-SPEC.md` | in-game captures still needed |
