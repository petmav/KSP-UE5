# CAPTURE-SPEC: values the harness cannot produce outside the game

The harness (`harness/run.ps1`) runs KSP's own IL on .NET Framework without Unity. Anything whose result depends on
Unity's **native** engine code (UnityPlayer.dll) or on live scene objects (Transforms, PQS, vessels) cannot be produced
that way. This file specifies in-game captures for those items, for the KSPAddon the physics agent maintains.

## General rules for every capture

- Run on the KSP-dev copy (KSP 1.12.5.3190, custom Assembly-CSharp is fine: the harness proved rebuilt IL == shipped IL
  bit-for-bit on all datasets). No mods besides the capture addon. Record `Versioning.version_major/minor/revision`,
  `Application.unityVersion`, OS, and `CultureInfo.CurrentCulture.Name` in every output.
- Write output in the harness schema **`ksp-golden/1`** (see harness/README.md "File format") so
  `python -I harness/compare/compare.py <harness golden> <capture>` works directly. Write floats with
  `f.ToString("R", CultureInfo.InvariantCulture)` **and** a hex-bits column (`BitConverter.SingleToInt32Bits` /
  `DoubleToInt64Bits`, formatted `X8`/`X16`); the compare tool uses the decimal column, the hex column is for bit audits.
- Output path: `<KSP-dev>/GameData/HarnessCapture/out/<dataset>.json`; inputs (harness golden files) are copied to
  `GameData/HarnessCapture/in/`. Do not write under `decompiled/` from the harness side; the physics agent copies files.
- Each item lists: scene and timing, exact calls, inputs, what to record, how the result is used.

Priority: **C1** (blocks bit-exact FloatCurve claims), **C7** (calibrates every tolerance), **C2**, **C3**, then the rest.

---

## C1. FloatCurve / AnimationCurve native verification (`curves_eval_native`)

Why: every FloatCurve in KSP evaluates through Unity's native `AnimationCurveTpl<float>`. The harness uses a managed port
(`harness/src/KspUnityShim/AnimationCurve.cs`) transcribed op-for-op from the disassembly of UnityPlayer.dll 2019.4.18f1
(PDB from Unity's public symbol server, GUID 41E8D80F828B4CEC840B0E4C0496BCCF age 1). It has **not been executed against
native code**; this capture closes that gap.

- Scene: MAINMENU (KSPAddon `Startup.MainMenu`, `once = true`), after PhysicsGlobals has loaded (it has by then).
- Input: `in/curves_eval.json` (tables `curves`, `keys`, `samples`).
- For each row of `curves`:
  - `origin` = "part cfg" or "synthetic: ...": build `var n = new ConfigNode("CURVE"); foreach (k in cfg_keys.Split(" | ")) n.AddValue("key", k); var fc = new FloatCurve(); fc.Load(n);`
    (exactly the same call sequence as the harness; for `synthetic.keyframe_ctor_dragTip` use `new FloatCurve(Keyframe[])` with the 4 keys listed in FloatCurveGen.cs).
  - `PhysicsGlobals.<f>`: use the live object, e.g. `PhysicsGlobals.DragCurveTip`; `LiftingSurface.<set>.<curve>`: `PhysicsGlobals.GetLiftingSurfaceCurve(set).<curve>Curve`.
  - `Body.<name>.<field>`: `FlightGlobals.Bodies.Find(b => b.bodyName == name).<field>` (FloatCurve field by reflection).
- Record table `keys`: `fc.Curve.keys[i]` time/value/inTangent/outTangent (+ weightedMode, inWeight, outWeight) for every curve.
- Record table `samples`: for every `(curve_id, i, t)` in the input `samples` table, `fc.Evaluate(t)`, evaluated in the
  input order; then evaluate the same list in reverse order and record any bitwise difference (expected: none).
- Use: `compare.py golden/curves_eval/curves_eval.json out/curves_eval_native.json` must pass with **0 ULP**. Any
  mismatch identifies an error in the port (fix KspUnityShim, regenerate).
- Alternative without launching KSP (was attempted and **blocked by the permission system**; the user can allow it):
  load a copy of UnityPlayer.dll into a harness process and call the native functions directly at these RVAs:
  `AnimationCurveTpl<float>::Evaluate` 0x75A6A0, `AddKey` 0x758840, `AddKeySmoothTangents` 0x750780,
  `AnimationCurveTpl<float>::AnimationCurveTpl(MemLabelId)` 0xDEB70 (object layout: cache 0x00-0x37, keys ptr 0x38, label 0x40,
  size 0x48, capacity<<1 0x50, pre/post infinity 0x58/0x5C). Pre-allocate the key buffer and set capacity so no Unity
  allocator call is needed.

## C2. Unity Random (`unity_random`)

Why: `UnityEngine.Random` is native (xorshift128 in UnityPlayer.dll; `Random_CUSTOM_InitState` 0x962470,
`Random_Get_Custom_PropValue` 0x9627A0, `Random_CUSTOM_Range` 0x962560, `RandomRangeInt` 0x9624B0). KSP seeds it for
contracts, comets, asteroids and surface features (InitState callers include CelestialBody.cs, CometManager.cs,
Contracts/Contract.cs, Contracts/ContractSystem.cs, Contracts/Templates/*).

- Scene: any (MAINMENU). Save and restore `Random.state` around the capture.
- Seeds: 0, 1, 2, 42, 12345, -1, int.MinValue, int.MaxValue, 0x5EED, plus 20 seeds from `new System.Random(20261008).Next()`.
- For each seed: `Random.InitState(seed)`; record `Random.state` (struct `Random.State`, private int fields s0..s3 via
  reflection); then 64 x `Random.value`; re-init; 64 x `Random.Range(-10f, 10f)`; re-init; 64 x `Random.Range(0, 7)`
  and 64 x `Random.Range(-1000, 1000)`; re-init; 16 x `Random.insideUnitSphere`, 16 x `Random.onUnitSphere`,
  16 x `Random.rotation`, 16 x `Random.insideUnitCircle`; record `Random.state` after each block.
- Also record one KSP-level consumer end to end: for 5 seeds, the comet/asteroid orbit and size class that
  `CometManager`/`ScenarioDiscoverableObjects` generate when given that seed (call path and fields per the decompile).
- Use: specifies the xorshift128 state transition and float conversion for the C++ shim.

## C3. Unity native vector/quaternion maths (`math_shim_native`)

Why: these are InternalCalls; `harness/golden/math_shim` covers only managed code.

- Calls (float32): `Quaternion.Euler(x,y,z)`, `Quaternion.AngleAxis(a, axis)`, `Quaternion.LookRotation(fwd, up)`,
  `Quaternion.LookRotation(fwd)`, `Quaternion.FromToRotation(a,b)`, `Quaternion.Slerp(a,b,t)`, `Quaternion.SlerpUnclamped`,
  `Quaternion.Lerp`, `Quaternion.Inverse(q)`, `q.eulerAngles`, `Quaternion.RotateTowards`, `Vector3.Slerp`,
  `Vector3.RotateTowards`, `Vector3.OrthoNormalize(ref a, ref b)`, `Mathf.PerlinNoise(x,y)`, `Mathf.ClosestPowerOfTwo`,
  `Matrix4x4.TRS(p,q,s)` and `.inverse`, and a Transform with given position/rotation/scale:
  `TransformPoint`, `InverseTransformPoint`, `TransformDirection`, `InverseTransformDirection` (FlightIntegrator uses
  `partTransform.InverseTransformDirection` to build dragVectorDirLocal).
- Inputs: angles {0, 1e-6, 0.5, 30, 45, 89.9, 90, 135, 179.99, 180, 270, 359, 720, -45}; axes {X, Y, Z, (1,1,0), (0.3,-0.4,0.5), (1e-6,0,1)};
  vectors from `harness/golden/math_shim` (`f32` table inputs); t in {0, 0.25, 0.5, 1, 1.5}.
- Also KSP's `QuaternionD.Euler/FromToRotation/Slerp/Lerp/AxisAngle`: these are `extern` declarations inside
  Assembly-CSharp itself (QuaternionD.cs:189-434) that Unity does not register. Call each once inside try/catch and
  record whether it throws (expected: MissingMethodException / SecurityException), so the port knows they are dead.
- Record float bits of every component.

## C4. Latitude/sun atmospheric temperature (`atmosphere_thermal_offsets`)

Why: `CelestialBody.GetAtmoThermalStats` reads `scaledBody.transform`, `bodyTransform.up` and calls `Quaternion.AngleAxis`;
the harness takes `atmosphereTemperatureOffset` as an input instead.

- Scene: FLIGHT (any vessel), physics running, so FlightIntegrator.sunBody and ScaledSpace are valid.
- For bodies Kerbin, Eve, Duna, Laythe, Jool; lat {-90,-60,-30,0,30,60,90}; lon {0,90,180,270}; alt {0, 10000, 30000, 0.9*atmosphereDepth}:
  - `pos = body.GetWorldSurfacePosition(lat, lon, alt)`; compute exactly as `FlightIntegrator.CalculateSunBodyFlux`:
    `scaledPos = ScaledSpace.LocalToScaledSpace(pos)`, `sunVector = (sun.scaledBody.transform.position - scaledPos).normalized`,
    `upAxis = FlightGlobals.getUpAxis(body, pos)`, `sunDot = Vector3d.Dot(sunVector, upAxis)`.
  - Call `body.GetAtmoThermalStats(true, sun, sunVector, sunDot, upAxis, alt, out offset, out emissive, out albedo)` and
    `body.GetFullTemperature(alt, offset)`, `body.GetFullTemperature(pos)`.
  - Record inputs (pos, sunVector, upAxis, sunDot, `body.rotationAngle`, `body.bodyTransform.up`, home-planet `orbit.trueAnomaly`,
    UT) and outputs.
- Repeat at UT, UT + 0.25 solar day, UT + 0.5 day, UT + 0.25 year (use `Planetarium.SetUniversalTime` then wait 2 physics frames).

## C5. Stock delta-v readout (`deltav_readout`)

Why: `VesselDeltaV`/`DeltaVStageInfo` simulate a whole ShipConstruct (resources, crossfeed, staging) with float masses;
the harness only re-hosts the per-burn expression (`engine_thrust/rocket_equation`).

- Scene: EDITOR (VAB/SPH). For each stock craft in `harness/golden/confignode_roundtrip/corpus` (rows with id `craft_*`):
  `EditorLogic.LoadShipFromFile(path)`; wait until `EditorLogic.fetch.ship.vesselDeltaV.IsReady`; then for each
  `DeltaVStageInfo s in ship.vesselDeltaV.OperatingStageInfo`: record stage, deltaVinVac, deltaVatASL, deltaVActual, TWRVac,
  TWRASL, ispVac, ispASL, thrustVac, thrustASL, startMass, endMass, dryMass, fuelMass, stageBurnTime.
- Repeat with `DeltaVGlobals.DeltaVAppValues` body = Mun, Duna, Eve and altitude 0.

## C6. Part thermal integration (`thermal_step`)

Why: FlightIntegrator's thermal graph (conduction, convection, radiation per PartThermalData) needs a live vessel.

- Scene: FLIGHT. Case A: Mk1 pod + heat shield 1.25 m, on rails at Kerbin PeA 25 km / ApA 500 km, released at 80 km
  descending. Case B: LV-T45 + FL-T800 clamped on the launch pad at full throttle for 60 s.
- Per physics frame for 3000 frames (case A) / 3000 frames (case B): `Time.fixedDeltaTime`, vessel altitude, srfSpeed,
  mach, `FlightIntegrator` fields (density, externalTemperature, convectiveCoefficient, solarFlux, bodyEmissiveFlux,
  bodyAlbedoFlux), and per part: temperature, skinTemperature, skinUnexposedTemperature, thermalInternalFlux,
  thermalConductionFlux, thermalConvectionFlux, thermalRadiationFlux, thermalSkinFlux, skinExposedAreaFrac, radiativeArea,
  exposedArea, ptd.convectionArea (via `FlightIntegrator.partThermalDataList`).

## C7. Mono vs .NET libm (`libm_mono`)

Why: the harness runs on the .NET Framework CLR; KSP runs on Unity's Mono, whose System.Math calls a different C
runtime. Results can differ by an ulp, which is why some tolerances are not 0. This capture measures it.

- Scene: any. For x in a fixed list (all inputs of `orbit_propagation/kepler`, the `state_at_ut` UTs times meanMotion, and
  1000 values from `new System.Random(7).NextDouble()*200-100`): record bits of Math.Sin, Cos, Tan, Asin, Acos, Atan,
  Atan2(x, 0.7), Sinh, Cosh, Tanh, Exp, Log, Log10, Sqrt, Pow(x, 0.4), Pow(|x|, 1/3), Pow(x, 2.0), Math.Floor, Math.Round.
- Use: diff against the same table produced by the harness (`KspGolden` can be extended to emit it); any non-zero diff
  explains residual differences in orbit/atmosphere datasets.
- **float32 arithmetic mode** (equally important): Mono can evaluate C# `float` expressions in double precision unless
  its float32 optimisation is on; the .NET Framework x64 JIT always rounds to float32 per operation. Record bits of
  `float r = ((x * 1.1f + 0.3f) * x - 0.7f) / 3.3f;` and of `float r = a * b + c * d - e;` for 10,000 pseudo-random
  float inputs (`System.Random(11)`), in a method compiled into the capture addon (so it runs under KSP's Mono JIT).
  If any result differs from the same code run by the harness, managed float32 code in KSP (DragCubeList, FloatCurve
  wrappers, ModuleEngines float paths) is not bit-reproducible by a float32 C++ port and the f32 tolerances must be
  widened accordingly. Also compare `harness/golden/aero_forces/cube_drag` against C10 for the same inputs.

## C8. On-rails propagation through the full game loop (`orbit_rails_ingame`)

Why: confirms OrbitDriver/Planetarium/Krakensbane/floating origin add nothing on top of `Orbit` math.

- Scene: FLIGHT. Spawn 5 probes on rails with the elements of orbit_propagation cases `kerbin_leo_circ_equ_a`,
  `kerbin_gto_a`, `kerbin_e081_a`, `kerbin_hyp12_a`, `mun_incl_ell_a` (use `Orbit` constructor + `OrbitDriver.orbit`).
- At warp 1x, 100x and 10000x, every 50 physics frames for 2 orbits: record UT, `vessel.orbit.pos`, `vessel.orbit.vel`,
  `orbit.getRelativePositionAtUT(UT)` and `vessel.GetWorldPos3D() - mainBody.position` (swizzled .xzy).

## C9. Part database and loaded drag cubes (`part_database`)

- Scene: MAINMENU. For every `PartLoader.LoadedPartsList` entry: name, partUrl, mass, cost, crashTolerance, maxTemp,
  skinMaxTemp, module names in order, resources (name, amount, maxAmount), `partPrefab.DragCubes.Cubes` (name, weight,
  area[6], drag[6], depth[6], center, size), `partPrefab.DragCubes.None/Procedural`.
- Use: verifies that the harness's PartDatabase.cfg-based cubes equal what KSP actually loads.

## C10. Drag on an assembled vessel (`aero_forces_vessel`)

Why: the harness evaluates drag cubes on isolated parts; occlusion through attach nodes (`SetPartOcclusion` with
attached parts) needs a real vessel.

- Scene: FLIGHT, Kerbal X launched, with `PhysicsGlobals.AeroDataDisplay` on. At altitudes 1, 5, 10, 20, 30 km during a
  scripted vertical ascent and at 10 deg AoA: for every part record dragVectorDirLocal, mach, dynamicPressurekPa,
  DragCubes.AreaOccluded[6], WeightedDrag[6], AreaDrag, Area, ExposedArea, dragScalar, bodyLiftScalar,
  DragCubes.LiftForce, and FlightIntegrator.pseudoReDragMult.
