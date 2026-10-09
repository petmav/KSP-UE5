# Harness progress (working memory; not a deliverable)

## Layout (all under C:\Projects\KSP-UE5\harness)
- lib/ : copies of Assembly-CSharp.dll(+pdb, rebuilt, sha 6c9b58c6...), Assembly-CSharp.de4dot-original.dll (backup\Assembly-CSharp-cleaned.dll), UnityEngine*/Unity.*/firstpass/KSPAssets/KSPTrackIR/Ionic.Zip/Mono.Cecil.
- inputs/ : Physics.cfg, PartDatabase.cfg, GameData/Squad/{Parts,Resources}/**.cfg, Ships/{VAB,SPH}/*.craft (sync-inputs.ps1, robocopy from KSP-dev).
- src/KspUnityShim (net48): managed port of Unity native AnimationCurveTpl<float> (from UnityPlayer.dll disassembly + Unity public PDB) + Gradient stub.
- src/AsmPatcher (net8, Mono.Cecil 0.11.6): copies Assembly-CSharp, retargets UnityEngine.AnimationCurve/Gradient TypeRefs -> KspUnityShim, redirects 16,685 direct InternalCall call sites (500 natives) to throwing stubs (KspHarness.UnityNativeStubs). Report: build/patched/patch-report.txt.
- src/KspGolden (net48 x64 exe): KspEnv (world setup), Infra (ExactNumber, Json reader/writer, Golden tables, Unity helpers, SerializedOverlay), Rehosted/CelestialBodyLifecycle.cs, Generators/*.
- tools/extract_assets.py (UnityPy + TypeTreeGeneratorAPI in tools-cache/venv) -> golden/serialized/*.serialized.json (17 bodies, PhysicsGlobals level0, Planetarium/FlightGlobals/PSystemManager level9).
- tools/native/pdb_publics.py, disasm.py (capstone) : PDB symbol dump + disassembly of UnityPlayer.dll. tools-cache/unity-symbols has the PDB (229 MB) + publics.tsv.
- build.ps1 (builds shim, patcher, patches both DLLs, builds KspGolden, makes build/KspGolden-original bound to shipped IL), sync-inputs.ps1.
- Run: build/KspGolden/KspGolden.exe --harness . [--datasets a,b] [--out dir] [--ksp-il label] [--input-sha sha]

## STATUS: COMPLETE (all deliverables written; run.ps1 verified end to end, deterministic, cross-check PASS 518,141 values)

## Datasets status (final names: curves_eval, body_catalog, orbit_propagation, patched_conics, atmosphere_curves, aero_forces, physics_constants, confignode_roundtrip, engine_thrust, math_shim)
- floatcurve: DONE golden/floatcurve/floatcurves.json (366 curves, ~30k rows). trust direct+rehosted-native.
- bodies: DONE golden/bodies/celestial_bodies.json (bodies 17, orbits 16, atm profile 1km, body_curves). direct+rehosted.
- orbits: DONE golden/orbits/orbits.json (cases 92, state_at_ut, from_state_vectors, state_vector_cases, kepler, patches incl. Kerbin->Mun encounter). direct.
- atmosphere: DONE golden/atmosphere/atmosphere.json (profile_100m, flight_state via real FlightIntegrator.CalculatePressure/CalculateConstantsAtmosphere, solar_effects). direct.
- aero: DONE golden/aero/aero.json (drag_curve_value, cube_inputs/weighted/drag for 16 stock parts + multi-cube variants, drag_chain_kerbin, lifting_surface). direct+rehosted.
- thermal: TODO (PhysicsGlobals values table incl. static constants, source cfg vs prefab; curves already in floatcurve).
- confignode: TODO (Load -> Save via real ConfigNode.Save to temp, ToString, JSON tree; craft files (stock subset), part cfgs, Physics.cfg, PartDatabase.cfg).
- engines: TODO (stock engine cfgs; atmosphereCurve Isp sweep; host ModuleEngines via new(); fields: try BaseFieldList(this).Load(node) else set manually; maxFuelFlow rehosted from OnLoad; MaxThrustOutputAtm/Vac, GetEngineThrust direct; Tsiolkovsky table rehosted from DeltaVStageInfo.StoreInterimDeltaV/CalculateISP).
- Then: compare/compare.py, README.md, CAPTURE-SPEC.md, golden/*/README.md (or one per dataset), run.ps1 (sync-if-missing, extract-if-missing, build, run rebuilt + original IL, diff the two), final report.

## Key findings (for final report)
1. FloatCurve sits on Unity native AnimationCurve: evaluation = cached cubic ((a t + b) t + c) t + d in float32 with t = time - key.time (not Hermite basis); segment length clamped to >=1e-4; only +Inf tangent = stepped; clamp outside range; Evaluate(+inf) -> NaN.
2. AddKey(t,v) (2-value cfg keys) = AddKeySmoothTangents: inserts and recomputes slopes of key-1, key, key+1 (bias 0); first/last key slopes = one-sided; middle = 0.5*m_right + 0.5*m_left, |dx|<=1e-5 -> 0. Explicit tangents of neighbours get overwritten by a later 2-value key. Duplicate times rejected (AddKey returns -1, key dropped). Keys with 3 or 5 values -> treated as 2-value.
3. Unity Keyframe weights default; KSP never weighted.
4. Mono culture: en-US uses "Infinity"/"-Infinity"/"NaN" (scanned mono-2.0-bdwgc.dll strings); .NET Framework on Win11 uses U+221E -> harness patches NumberFormat. C++ parser must accept "Infinity".
5. Body serialized values are float-derived doubles (e.g. ecc 0.200000002980232, Kerbin sma 13599840256, adiabatic index 1.39999997615814). gravParameter = (R^2 g0 GeeASL / G) * G with G = 6.67408e-11 (Kerbin mu = 3531599999999.9995 not 3.5316e12). Kerbin solarRotationPeriod -> sidereal 21549.425183089825. Tidally locked bodies rotationPeriod = orbit.period each CBUpdate. Kerbin OrbitDriver reverse=1. Sun has atmosphere (600 km).
6. On .NET Framework any KSP method directly referencing a Unity InternalCall fails to JIT entirely -> patcher stubs.
7. FlightGlobals cctor calls Quaternion.Euler (native) -> FlightGlobals not hostable; GetHomeBody/CalculateValues re-hosted. GameParameters cctor needs AssemblyLoader.loadedAssemblies (set empty).
8. SetupConstants' last line (minOrbitalDistance) needs PQS -> caught, earlier statements kept.
9. Orbit.GetTimeToPeriapsis on a fresh orbit (patchEndTransition INITIAL, EndUT 0) returns timeToPe - period.
10. CalculateConstantsAtmosphere uses previous-frame density for solar effects (call twice).
11. Native UnityPlayer.dll in-process load (to execute AnimationCurve natively) was DENIED by the permission classifier; only disassembly used. Mention in report; capture spec covers in-game check.
12. No subagents spawned (0).

## Conventions
- Golden schema ksp-golden/1: {schema,dataset,trust,description,oracles,provenance,tables:[{name,description,key,columns:[{name,type(f64|f32|i64|str|bool),unit,note}],row_count,rows}]}; NaN/Inf as strings.
- Doubles formatted shortest of G15..G17 that round-trips via exact BigInteger parser; floats G6..G9.
- Use Edit tool or python io.open(...,encoding='utf-8') for edits (cp1252 default broke a file once).
