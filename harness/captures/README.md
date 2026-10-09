# In-game captures (CAPTURE-SPEC C1, C2, C3, C7; batch 2: C4, C5, C6, C8, C9, C10, terrain heights)

These are values produced **inside the running game**: KSP 1.12.5 build 03190, Unity 2019.4.18f1, Mono 5.11.0 ("Visual Studio built mono"), x64.
The dev copy `decompiled/KSP-dev` was used, never the Steam install. Each value is written in the harness schema `ksp-golden/1` so that `harness/compare/compare.py` can diff it.

The capture code is the **capture mode** of the profiling addon. It lives on branch `profiling` of `decompiled/KSP-Decompiled`, in `Assembly-CSharp/PhysProf/`:

| File | Contents |
|---|---|
| `HarnessCapture.cs` | C1, C2, C3, aero, the math_shim replica, the QuaternionD externs, and provenance |
| `HarnessCaptureC7.cs` | C7 kernel. Unity-free. |
| `HarnessCaptureJson.cs` | Writer. Unity-free. |

`PhysProfDriver` dispatches to the capture mode when the command line contains `-harness-capture <items>`. The capture runs at MAINMENU, writes `<dataset>.json` files, and quits the game. A full run takes about 45 s.

The commits are on `profiling`:

| Commit | Change |
|---|---|
| `ecd026a5` | capture mode |
| `ade40c4c` | C7 probes for argument, return and store rounding |
| `5fedcfab` | Random vector traces and the aero item |

The final data was produced by commit `5fedcfab`. The DLL was built with **Optimize=true** (`decompiled/tools/scripts/build_opt.sh`), sha256 `d87eee7b…`. That DLL is not JIT-debuggable: `IsJITOptimizerDisabled=False`, which is recorded in the provenance of every file.

## Results

| Item | Compared against | Verdict | Max error / notes |
|---|---|---|---|
| **C1** `curves_eval_native` | `harness/golden/curves_eval` (the harness's managed port of the native AnimationCurve) | **PASS, bit-exact** | 0 ULP on all 366 curves: key_count, minTime and maxTime; all 1,055 keys after load (time, value, in/out tangents, including Unity's AddKey smoothing); all 28,484 samples (including 58 NaN results). Evaluating in reverse order changes nothing (0 cache-order mismatches). `compare.py` PASS with max_abs = 0. |
| **C2** `unity_random` | Python model `tools/random_model.py` + `tools/random_vec_fit.py`. There is no harness golden. | **PASS, bit-exact model** | InitState 29/29 seeds; xorshift step 60/60 traced states; value 1856/1856; Range(float) 1856/1856; Range(int) 3712/3712 + 60 traced; 406/406 edge calls; onUnitSphere, insideUnitSphere, rotation and rotationUniform 100%. insideUnitCircle 95/96 components: the one miss is 1 ULP, where the CRT's `sinf` rounds a near-tie the other way. KSP `DiscoveryInfo.GetClassRadius` 45/45. |
| **C7** `libm_mono` | `out/libm_dotnet.json`: the same source run on .NET Framework 4.8 x64, the harness's runtime | **FAIL (real runtime differences)** | Of 25 libm functions × 3,093 inputs, only `Math.Tan` differs in value. Signed zero differs in `Ceiling` and `Atan2`. float32 expression evaluation differs fundamentally (see below). `(ulong)NaN`/Inf/overflow, `(double)ulong` rounding and one `double.ToString()` also differ. Parsing and System.Random are identical. |
| math_shim (`math_shim_mono`) | `harness/golden/math_shim`: MathGen re-run under Mono | **FAIL (float32 only)** | f64, all KSP double maths: 530/532 rows bit-exact. The 2 others are `Planetarium.Zup`, which is live rotated state at MAINMENU, not a maths difference. f32: 72/903 rows differ, up to 25 ULP (`Vector3.ProjectOnPlane`), and `Mathf.Repeat(1e20f, 360f)` is 0 on .NET but 360 on Mono. |
| aero (`aero_forces_mono`) | `harness/golden/aero_forces` (4 of 6 tables re-run) | **FAIL (float32 semantics)** | cube_inputs and cube_weighted are bit-exact. Differences: `drag_curve_value` 835/2870 values, up to 61 ULP (rel 4.0e-6). `cube_drag` up to 37 ULP (AreaDrag), 33 (DragCoeff), 14 (ExposedArea), 8 (Area), 1-3 (others). `CalculateDragValue` differs in all 18,122 rows (rel ≤ 3.2e-6), because Mono widens the float product exactly. |
| **C3** `math_shim_native` | `tools/c3_check.py`: double-precision textbook formulas. There is no harness golden; this file *is* the golden for the C++ port. | **captured, conventions confirmed** | See "C3" below. The native results are within 0 to 6.9 float32 eps (relative to magnitude) of the double references for the checked functions. |
| QuaternionD externs | expectation from harness README item 10 | **confirmed** | 11 of 12 methods throw `System.MissingMethodException` in game: Euler, FromToRotation, Slerp, Lerp, RotateTowards, eulerAngles, ToAngleAxis, EulerRotation, EulerAngles, ToEulerAngles and AxisAngle. `QuaternionD.Inverse`, a managed conjugate, works. |

Reports are in `reports/`:

| Report | Contents |
|---|---|
| `C1_compare.json` | `compare.py` output |
| `C1_bitcompare.json` | strict bit comparison |
| `C2_analysis.md` | scalar Random model |
| `C2_vector_fit.md` | vector-function fit |
| `C7_analysis.md` | per-function libm table, float32 model classification, conversion/format diffs |
| `C7_bitcompare.json` | |
| `mathshim_bitcompare.json` | |
| `mathshim_compare_tol.json` | with `tolerances.json` |
| `aero_compare.json`, `aero_bitcompare.json` | |
| `C3_check.md` | |

### C1 verdict
The harness FloatCurve port (`harness/src/KspUnityShim/AnimationCurve.cs`, transcribed from UnityPlayer.dll) is **bit-exact** against native Unity on every input the harness uses. That covers:
- PhysicsGlobals drag and lift sets;
- all body curves;
- every `key` node in 214 stock part cfgs;
- the 14 synthetic edge cases: auto tangents, duplicate times, a single key, an empty curve, ±Infinity tangents, tiny dx, mixed delimiters and 3/5-value keys.

The 0-ULP tolerance for `curves_eval` stands. Other confirmations:
- Every curve has pre/post wrap mode 8 (ClampForever), including curves built by `FloatCurve.Load`, which creates a bare `new AnimationCurve()`.
- `weightedMode` is 0 everywhere.
- `inWeight`/`outWeight` are 1/3 for keys created through `Keyframe` and 0 for keys from `AddKey(t, v)`. This is irrelevant while weightedMode is 0.

### C2: UnityEngine.Random (native, Unity 2019.4)
All arithmetic below is float32 (each operation rounded), with `uint32` wrap-around.

**Generator**
- `InitState(seed)`: `s0 = seed; s1 = s0*1812433253+1; s2 = s1*1812433253+1; s3 = s2*1812433253+1`.
- `next()`: `t = s0 ^ (s0<<11); t ^= t>>8; s0=s1; s1=s2; s2=s3; s3 = s3 ^ (s3>>19) ^ t; return s3`.

**Scalar mappings**
- `value = (float)(next() & 0x7FFFFF) / 8388607f`. This is identical to `* (1f/8388607f)` for all 2^23 inputs. The range is [0, 1] inclusive.
- `Range(float min, float max) = v*min + (1-v)*max`, with one draw. **It is reversed compared with a textbook lerp:** `Range(0f,1f) == 1 - value`.
- `Range(int min, int max)`:
  - `max > min`: `min + (int)(next() % (uint)(max-min))`.
  - `max < min`: `min - (int)(next() % (uint)(min-max))`.
  - `max == min`: returns `min` and **consumes no draw**.

**Vector functions** (draws per call in brackets)

| Function | Formula |
|---|---|
| `onUnitSphere` [2] | `z = Range(-1,1); a = Range(0, 2π); r = sqrtf(1 - z*z); (r*cosf(a), r*sinf(a), z)` |
| `insideUnitSphere` [3] | `onUnitSphere * powf(value, 1/3)` |
| `rotation` [4] | `Normalize(Range(-1,1) x4)` as (x, y, z, w), divide by the norm, then negate all components if `w < 0` |
| `rotationUniform` [3] | `u1 = Range(0,1); u2 = Range(0,2π); u3 = Range(0,2π); a = sqrtf(1-u1); b = sqrtf(u1); (a sin u2, a cos u2, b sin u3, b cos u3)`, then make `w >= 0` |
| `insideUnitCircle` [2] | `θ = Range(0,2π); r = sqrtf(Range(0,1)); (r cosθ, r sinθ)` |
| `ColorHSV()` [4] | managed: 4 × value |

**KSP consumers**
- `DiscoveryInfo.GetClassRadius(cls, seed) = base[cls] * Range(0.75f, 1.25f)` after `InitState(seed)`. Reproduced 45/45.
- The `ScenarioDiscoverableObjects.SpawnHomeAsteroid` random sequence was captured for 5 seeds (table `ksp_asteroid`): duration, name, size class, lifetime, and a heliocentric flyby orbit from `Orbit.CreateRandomOrbitFlyBy`. At MAINMENU, UT is 0.
- `KSPRandom` (a copy of .NET's Random) and `System.Random` are identical in Mono and .NET.
- Comets need `CometManager.Instance`, a scenario in a loaded game. They were **not** captured.

### C7: Mono vs .NET Framework

**libm (double).** These are bit-identical on all 3,093 inputs: Sin, Cos, Asin, Acos, Atan, Atan2, Sinh, Cosh, Tanh, Exp, Log, Log10, Sqrt, Pow (x^0.4, |x|^(1/3), x^2, 1.0000001^x), Floor, Round (both modes), IEEERemainder and `%`. The inputs are:
- kepler e/M/E and state_at_ut UT×meanMotion and eccentricAnomaly from `orbit_propagation`;
- 1000 × `System.Random(7)`;
- 40 specials.

The differences:
- **`Math.Tan`** differs in 244 values.
  - Where the two differ, Mono's tan is within 1 ULP of exact in every case (checked with mpmath).
  - .NET's tan is more than 1 ULP off in 94 of those cases. Near kπ it has a relative error of up to 4e-5, and for the tested |x| ≥ 1e22 it returns NaN.
  - KSP uses Tan only in camera, UI, occlusion and wheel code, not in orbit code.
- **Signed zero.**
  - `Math.Ceiling(-0.5)` is `-0.0` on Mono (IEEE) and `+0.0` on .NET.
  - `Math.Atan2(-0.0, 0.7)` is `+0.0` on Mono and `-0.0` on .NET (IEEE).
  - `Mathf.Pow(-0f, -3f)` is `+∞` on Mono and `-∞` on .NET (IEEE).

**float32 expression evaluation (the big one).** Unity's Mono runs **without** float32 mode. Under Mono:
- C# `float` arithmetic is evaluated in **double**.
- The result is rounded to float32 only when it is stored to a float32 location. Locals, arguments, return values, fields, statics, struct fields and array elements all count, as does an explicit `(float)` (`conv.r4`).
- Temporaries on the expression stack are never rounded. So `a*b + c*d - e` is rounded once.
- `double r = a*b` gives the **exact** product.

This was established on 10,000 random inputs per probe. Every .NET row matches per-operation float32. Every Mono row of 17 of the 18 probes matches this model. For details, see the f32ops tables in `reports/C7_analysis.md`.
- The exception is `locals` (`t1=a*b; t2=c*d; t1+t2-e`; Roslyn keeps `t1` on the IL stack). There, all Mono rows match only if `a*b` is also rounded, which is consistent with the JIT spilling that stack value to an R4 temporary. Treat "rounded at stores" as the rule and expect occasional spill-induced roundings that depend on IL shape.

**Consequence for KSP's own float code.** KSP's managed float32 code (DragCubeList, PhysicsGlobals.DragCurveValue, Vector3/Quaternion operators, Mathf.Lerp and others) produces **different bits in game** than in the .NET harness:
- typically 1 to 8 ULP, up to 61 ULP in drag_curve_value;
- occasionally a qualitatively different result, for example `Mathf.Repeat(1e20f, 360f)`.

Only native code (AnimationCurve, C1) and double code are runtime-independent.

**Conversions** (unchecked)
- `(ulong)d` for NaN, ±Inf and values ≥ 2^64 gives `0x8000000000000000` on Mono and 0 on .NET. All other tested conversions are identical:
  - `(int)NaN = int.MinValue`;
  - `(uint)NaN = 0`;
  - `Convert.ToInt32` overflow throws on both.
- `(double)ulong` for 2^63+1025: Mono rounds correctly to 9223372036854777856, while .NET is 1 ULP low.

**Formatting and parsing**
- `float.Parse` and `double.Parse` are identical for all 296 tokens: every numeric token of the curves_eval cfg keys, plus edge cases including subnormals, overflow and `Infinity`.
- Formatting with "R", "G9", "G17" and "F6" is identical.
- Mono's `double.ToString()` (G15) is mis-rounded once in 600: for 5.997450259893145 it prints `…315`, while the correct output is `…314`, which .NET prints.

### C3: native maths conventions
Checked in `reports/C3_check.md`:
- `Quaternion.Euler(x,y,z)` = qY·qX·qZ, i.e. Z first, then X, then Y.
- `AngleAxis` normalises the axis.
- **`Quaternion.Inverse` is a plain conjugate.** It does not divide by |q|²; this was checked on the non-unit (0.1, -0.2, 0.3, 0.9).
- `Quaternion.Lerp` flips the sign of the second quaternion when the dot product is negative, then normalises.
- `Matrix4x4.TRS` is column-major with the scale applied to the rotation columns.
- `Transform.TransformPoint = R·(S∘p) + T` and `InverseTransformPoint = (R⁻¹·(p−T)) / S`.
- The round trip `Euler(q.eulerAngles)` reproduces q to within 5.4e-4.

The remaining functions are captured raw for the C++ port, but no reference formula is fitted for them: LookRotation, FromToRotation, Slerp/SlerpUnclamped, RotateTowards, Vector3.Slerp/RotateTowards/OrthoNormalize, PerlinNoise, Gamma/Linear, Half and the PowerOfTwo helpers.

### Recommendations (harness files were not edited; the harness owner decides)
1. Keep `curves_eval/*` at 0 ULP.
2. **float32 managed paths** cannot be bit-exact against the .NET golden if the C++ port is meant to match the game. These are `aero_forces/*` f32, `engine_thrust/*` f32, `atmosphere_curves/*` f32 and `math_shim/f32`. Either:
   - emulate Mono's rule in C++ (compute float expressions in double and round at every assignment, argument, return or cast) and compare against in-game captures; or
   - widen the tolerances to at least the observed values: drag_curve_value about 64 ULP or rel 5e-6, cube_drag about 40 ULP or rel 5e-6, and f64 values built from float products rel 5e-6.

   The current 0 to 4 ULP rules for these tables fail on the in-game data: 7 FAIL columns in aero (drag_curve_value.value, Area, AreaDrag, DragCoeff, ExposedArea, TaperDot, CalculateDragValue) and 3 f32 columns in math_shim. math_shim also has 2 f64 failures, but those come from the live Zup rows.
3. Double-precision datasets that use only Sin, Cos, Atan2, Sqrt, Exp, Log, Pow and Sinh are runtime-independent on the tested inputs, so the "1-ULP CRT" allowance in the orbit and atmosphere tolerances is not needed for the Mono/.NET difference. Iterative solvers still justify their own tolerances.
4. A C++ port should use correctly rounded or UCRT `tan`; Mono's matches exact to within 1 ULP. It should also reproduce Mono's signed-zero behaviour for Ceiling, Atan2 and Pow if bit-exactness matters.

## Rerun (one command each)
Prerequisite: the profiling-branch DLL must be in the KSP-dev symlink target. `main` is normally checked out, so:

```bash
git -C /c/Projects/KSP-UE5/decompiled/KSP-Decompiled checkout profiling
bash /c/Projects/KSP-UE5/decompiled/tools/scripts/build_opt.sh capture-NN          # Optimize=true; plain build.sh also works (unoptimised)
```

Then run one of the following. Each command regenerates the inputs, launches KSP-dev in capture mode, then normalizes, verifies and compares:

```bash
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh c1         # C1  -> out/curves_eval_native.json, reports/C1_*.json
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh c2         # C2  -> out/unity_random.json, reports/C2_*.md
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh c7         # C7  -> out/libm_mono.json (+ out/libm_dotnet.json), reports/C7_*
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh mathshim   # math_shim under Mono
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh aero       # aero_forces drag-cube tables under Mono
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh c3         # C3  -> out/math_shim_native.json, reports/C3_check.md
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh qdextern   # QuaternionD extern check
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh all        # everything in one ~45 s launch
```

Afterwards, restore main:

```bash
git -C /c/Projects/KSP-UE5/decompiled/KSP-Decompiled checkout main
bash /c/Projects/KSP-UE5/decompiled/tools/scripts/build.sh <tag> Rebuild
```

The launcher behind the wrapper is `decompiled/tools/scripts/capture_ksp.ps1 -Tag <t> -Items <list>`. It launches windowed at 640×360 with `-popupwindow`, waits for the addon to quit the game, kills the game on timeout, and copies the logs to `decompiled/logs/ksp-capture-<t>-{player,KSP}.log`.

The underlying command-line switches are:
- `-harness-capture c1,c7,mathshim,c2,aero,c3,qdextern|all`
- `-harness-capture-in <dir>`
- `-harness-capture-out <dir>`
- `-harness-capture-commit <sha>`

Runs are deterministic: two launches gave bit-identical outputs for every file.

## Files

| Path | What |
|---|---|
| `in/curves_eval_in.tsv` | curves (id, origin, path, raw cfg keys) and sample times (float32 hex) from `golden/curves_eval` |
| `in/libm_base_inputs.tsv`, `in/libm_inputs.tsv` | doubles (hex) from `golden/orbit_propagation`, plus Specials and 1000 × System.Random(7) (`libm_inputs` is written by LibmRef `gen`) |
| `in/f32_inputs.tsv` | 10,000 × (a, b, c, d, e, x) float32 hex from System.Random(11) |
| `in/parse_tokens.tsv` | numeric tokens of all curve keys, plus edge cases |
| `in/aero_in.tsv` | drag_curve_value inputs and cube_drag flow/Mach rows from `golden/aero_forces` |
| `out/curves_eval_native.json` | C1 (dataset `curves_eval`) |
| `out/unity_random.json` | C2 |
| `out/libm_mono.json`, `out/libm_dotnet.json` | C7, Mono and .NET Framework 4.8 x64 |
| `out/math_shim_mono.json` | harness math_shim re-run under Mono (dataset `math_shim`) |
| `out/aero_forces_mono.json` | harness aero_forces drag-cube tables re-run under Mono (dataset `aero_forces`) |
| `out/math_shim_native.json` | C3 |
| `out/quaterniond_externs.json` | QuaternionD extern check |
| `reports/` | comparison and analysis outputs (see Results) |
| `run_capture.sh` | one-command wrapper |
| `tools/prep_inputs.py` | writes `in/` from the harness golden files |
| `tools/netfx/` | `LibmRef.cs` + `build.sh`: .NET Framework 4.8 x64 reference (`gen`, `run`, `fmt`). `src/` holds verbatim copies of `HarnessCaptureC7.cs` and `HarnessCaptureJson.cs` from the profiling branch; `SOURCE.txt` records the commit. |
| `tools/normalize.py` | rewrites the decimals from the `_hex` bits in the harness's exact number format (needed so compare.py joins float keys) |
| `tools/verify_hex.py` | checks that every decimal equals its hex bits |
| `tools/bitcompare.py` | strict bit-level comparer, joining on keys and using the hex bits |
| `tools/analyze_c7.py`, `tools/random_model.py`, `tools/random_vec_fit.py`, `tools/c3_check.py` | analyses |
| `tools/hcutil.py` | shared helpers (exact decimal to float32, ULP distance) |
| `PROGRESS.md` | work log |

## Schema notes
- These are standard `ksp-golden/1` files.
  - The `trust` field is `capture`.
  - Where a harness dataset is mirrored, the `dataset` name equals the harness name: `curves_eval`, `math_shim`, `aero_forces`. That lets compare.py match the tables and tolerance rules.
  - Table and key names match the golden files.
  - Extra columns are allowed, and compare.py ignores them. In the C1 `curves` table these are `built_from`, `preWrapMode`, `postWrapMode` and `error`; in the `keys` table they are `weightedMode`, `inWeight` and `outWeight`.
- **Every f32/f64 column has a companion `<col>_hex` string column** holding the raw IEEE bits (`X8`/`X16`). These are authoritative.
  - The decimals are rewritten from the hex by `normalize.py`, using .NET Framework `ToString("G6".."G9")` / `("G15".."G17")` and the harness's `ExactNumber` round-trip rule. The text is therefore identical to what the harness would write.
  - This matters because .NET's float G8/G9 output is not always correctly rounded (e.g. `0.050000005`), and compare.py joins rows on the JSON text of float keys.
  - NaN and ±Infinity are written as the strings `"NaN"`, `"Infinity"` and `"-Infinity"`.
- Provenance records:
  - `ksp_version`, `unity_version`, `runtime` (Mono version), `os`, `cpu`, `culture` (`en-US`);
  - Assembly-CSharp path, sha256 and MVID, `assembly_csharp_jit` (Optimize), and `profiling_branch_commit`;
  - `scene` (MAINMENU), `ut` (0 at MAINMENU), frame and UTC;
  - item-specific counters, for example `cache_order_mismatches` and `system_random7_regenerated_mismatches`.

## Not verified / limitations
- **C1** covers exactly the harness's curves and sample times. ±Infinity sample times are not in the harness sample set, so `Evaluate(±Inf)` is not checked natively.
- **C2**:
  - Comet generation (`CometManager.GenerateDefinition`, which needs a loaded game) was not captured.
  - The asteroid sequence was captured with an inactive `ScenarioDiscoverableObjects` whose `OnAwake()` was called by hand, giving the default sizeCurve with 4 keys. `ProtoVessel` spawning was not exercised.
- **C3** has no fitted reference for LookRotation, FromToRotation, Slerp, RotateTowards, PerlinNoise and the others. The raw native values are the golden.
- **C7**:
  - The `locals` probe shows that Mono's rounding also happens at JIT stack spills. That depends on IL shape, so an exact emulation of Mono for arbitrary code needs care.
  - The probes were compiled with Roslyn 4.14 `/optimize+`, the same compiler as the decompiled-DLL build. The shipped DLL was compiled by Squad's compiler, whose IL may place locals differently.
- **aero**: only drag_curve_value, cube_inputs, cube_weighted and cube_drag were re-run. drag_chain_kerbin, lifting_surface, engine_thrust and atmosphere f32 were not, but they are expected to show the same kind of float32 drift.
- C4, C5, C6, C8, C9 and C10 (flight/editor scenes) were captured in batch 2; see "Batch 2" below.

---

# Batch 2: C4, C5, C6, C8, C9, C10 and PQS terrain heights

Captured 2026-10-09 on the same KSP-dev copy (KSP 1.12.5.3190, Unity 2019.4.18f1, Mono 5.11.0, no DLC installed), with the profiling-branch DLL built `Optimize=true` from commit `36ef51bd` (DLL sha256 `42d4b00d…`, clean tree; recorded in every file's provenance). Outputs are in `out2/`, reports in `reports2/`; batch-1 files in `out/` and `reports/` were not touched.

Code (branch `profiling`, `Assembly-CSharp/PhysProf/`):

| File | Contents |
|---|---|
| `HarnessCapture2.cs` | scene orchestration, C9, terrain, C5, helpers (`Safe()` coroutine wrapper, instance GameEvent handlers) |
| `HarnessCapture2Flight.cs` | launches, C4, C6, C8, C10 |

| Commit | Change |
|---|---|
| `c23196a1` | batch-2 capture mode |
| `fdde6a30` | C10 ascent ends at apex (keeps the vessel alive for C6) |
| `36ef51bd` | build fix (string quoting); the final data was produced by this commit |

One launch does everything (`-harness-capture batch2`, ~8 min):
1. MAINMENU: C9 and the PQS state probe.
2. A new sandbox game `physprofile`, created the same way as the profiling driver; it overwrites that save.
3. EDITOR: C5.
4. FLIGHT with PhysProf-S on the pad: terrain, C6 pad, C4, C8, C10, then C6 orbit and reentry.
5. FLIGHT with PhysProf-M2: C10.
6. FLIGHT with PhysProf-L2-AS: C10.
7. SPACECENTER, then quit.

The window is 640×360 with `-popupwindow`. Logs are `decompiled/logs/ksp-capture-capture2-final2-{player,KSP}.log`. The final run logged no game exceptions. The player log does have 4 `Exception` lines, but they are the capture's own handled messages for the 4 DLC-suit EVA parts (see C9).

## Batch 2 results

| Item | File (`out2/`) | Rows | Tables (rows) | Check | Result |
|---|---|---|---|---|---|
| **C9** part database | `part_database.json` (6.0 MB) | 48,587 | parts 362, part_cfg_values 6,235, modules 1,378, module_fields 34,801, module_propellants 77, resources 166, attach_nodes 827, drag_cubes 535, variants 228, assets 3,978 | Loaded cubes vs the DRAG_CUBE text the game read (part cfg if present, else top-level PartDatabase.cfg); cfg value vs resolved field | **531/531 cubes bit-exact** (67 from part cfgs; 4 EVA cubes are built in code). 3,069 numeric cfg values resolve to the parsed value; 235 are changed by the loader (two rules, see below). |
| **Terrain** (WP-11) | `terrain_height.json` (28.4 MB) | 144,035 | bodies 2, mods 69, mod_params 3,430, map_assets 8, scene_probe 4, samples 140,522 | MAINMENU vs FLIGHT; `GetSurfaceHeight` public vs `overrideQuadBuildCheck: true` | **Heights identical in MAINMENU and FLIGHT.** Public == override everywhere except 3 Mun points. KSC pad terrain is 64.7847 m. |
| **C5** delta-v | `deltav_readout.json` (0.4 MB) | 1,958 | craft 17, totals 68, stages 188, engines 460, parts 1,225 | Rocket equation per stage; craft sha256 vs harness corpus | 14/14 stock craft files byte-identical to the corpus. 20/24 single-phase stages equal Isp·g0·ln(m0/m1) to 6.6e-7. The other 4 are multi-phase stages, which differ by design. |
| **C4** atmo temperature offsets | `atmosphere_thermal_offsets.json` (2.8 MB) | 2,716 | epochs 4, body_epoch 24, samples 2,688 | `GetFullTemperature(pos)` vs FlightIntegrator path | Inputs and outputs captured for 6 bodies × 7 lat × 4 lon × 4 alt × 4 UTs. `GetFullTemperature(pos)` differs from the FI-path offset in 2,240/2,688 rows (quirk, below). |
| **C6** thermal integration | `thermal_step.json` (19.2 MB) | 18,015 | vessel 3,000, parts 15,000, part_constants 15 | none (golden) | Pad 500 frames, orbit 1,000, reentry 1,500. Reentry goes from 80 to 66 km at up to Mach 8.1, with T_ext up to 7,110 K and the Mainsail skin up to 490 K. FI is never in analytic mode. |
| **C8** on-rails propagation | `orbit_rails_ingame.json` (3.5 MB) | 3,932 | initial 6, samples 3,465, frames 450, segments 5, soi_events 6 | orbit.pos vs getRelativePositionAtUT; world vs pos.xzy; independent Kepler propagation through the captured `Planetarium.Zup` | In-game consistency ≤ 3.4e-15 relative. **Kepler agreement ≤ 4.7e-13 relative** (≤ 0.7 µm). 4 SOI transitions of our probes (Kerbin→Mun→Kerbin→Mun, Kerbin→Sun) plus 2 asteroid captures. |
| **C10** assembled-vessel drag | `aero_forces_vessel.json` (16.8 MB) | 30,219 | structure 631, states 20, parts 4,224, aoa_sweep 25,344 | AoA-0 sweep vs live FlightIntegrator values | **4,224/4,224 bit-identical.** 20 flight states over 3 craft: Mach 0.02–1.82, q up to 57 kPa, including a tumbling S at AoA 173°. |

All 7 files pass `verify_hex.py` (0 decimal/hex mismatches, 1.56 M float values) and were normalised with `normalize.py`. Summaries: `reports2/{c4,c5,c6,c8,c9,c10,terrain}_summary.md` (written by `tools/batch2_report.py`).

## Batch 2 notes per item

**C9 (`part_database`).**
- One row per `PartLoader.LoadedPartsList` entry: 362 entries, including EVA kerbals and the flag.
- **Asset origins.** The `assets` table gives the GameData-relative origin of every file the game resolved for each part:
  - the part cfg (`AvailablePart.configFileFullName`);
  - the model `.mu`, resolved like `PartLoader.CompileModel` does it (`MODEL{model=}`, otherwise the first model in the part's directory) via `GameDatabase.GetModelFile`;
  - the textures, read from every renderer/material/texture property of the prefab and mapped through `GameDatabase.databaseTexture` to their `.dds`/`.png`/`.mbm` file;
  - MODEL `texture =` replacements.
- **Counts.** 353 cfgs, 359 models and 2,497 texture slots resolve to files under `KSP-dev/GameData`. 733 texture slots are Unity assets that are not in GameData (`source = unity_asset`): EVA jetpack and backpack, FX textures such as `fairydust`, `smoke` and `expl_005`, and the parachute canopy. The UE5 import has to pull these from `KSP_x64_Data`. Five parts have no cfg because they are built in code: the 4 EVA kerbals and the flag.
- **Other tables.** `part_cfg_values` maps every top-level cfg key to the field it lands in. `module_fields` holds every KSPField value, with FloatCurves as their keys.

**Terrain (`terrain_height`).**
- **Samples:**
  - Kerbin and Mun on a 1° global grid (65,160 samples each);
  - a 0.01° patch around KSC (101×101: lat −0.60…0.40, lon −75.06…−74.06);
  - the pad point.
- **Heights:** `h_surface` (`pqsController.GetSurfaceHeight`, the `TerrainAltitude` path), `h_override` (the internal `overrideQuadBuildCheck: true` variant used for vessel ground positioning) and `terrain_alt = h_surface − pqs.radius`.
- **Mod stacks.**
  - Kerbin: 46 mods in the stack, 9 of which change height: VertexHeightMap, VertexRidgedAltitudeCurve, VertexSimplexHeightAbsolute, VertexHeightNoiseVertHeightCurve2, two MapDecalTangent, MapDecal, FlattenArea and PQSLandControl.
  - Mun: 23 mods, with VertexHeightMap, two VoronoiCraters and two FlattenArea.
- **Mod parameters** are dumped by reflection in `mod_params`.
- **Map assets** (`map_assets`), all Unity assets in `KSP_x64_Data` rather than GameData files:

  | Asset | Size and format | Used by |
  |---|---|---|
  | `kerbin_height_new` | 4096×2048, 8 bpp | VertexHeightMap and PQSLandControl |
  | `ksc_decal_heightMap` | 1024², 16 bpp | MapDecalTangent |
  | `decal_spaceport_0_height` | 1024², 16 bpp | MapDecal |
  | `island_runway_decal_heightmap` | 256², 16 bpp | MapDecalTangent |
  | `munHeightMap2` | 4096×2048, 8 bpp | VertexHeightMap (Mun) |
  | `munColorMap2` | 4096×2048, 24 bpp | VertexColorMap (Mun) |
- **Cost.** 281,044 height evaluations took about 2.5 s in game.

**C5 (`deltav_readout`).**
- **Craft.** The 3 PhysProf craft and the 14 stock craft of the harness corpus. The stock craft come from `KSP-dev/Ships`, and their sha256 matches the corpus.
- **Method.** Each craft is loaded with `EditorDriver.StartAndLoadVessel` or `EditorLogic.LoadShipFromFile`. For each body (Kerbin, Mun, Duna, Eve), `DeltaVGlobals.DeltaVAppValues.body` is set with altitude 0, and the values are read after `onDeltaVCalcsCompleted`.
- **Contents.** Per stage, every `DeltaVStageInfo` field: dV vac/ASL/actual, TWR, Isp, thrust, start/end/dry/fuel/stage/decoupled mass and burn time. Per engine, `DeltaVEngineInfo`. Per part, mass, module mass, resource mass and decouple/activation stage.

**C4 (`atmosphere_thermal_offsets`).**
- **Inputs** are built exactly as in `FlightIntegrator.CalculateSunBodyFlux`: scaled-space sun vector, `upAxis`, `sunDot`.
- **Outputs:** `GetAtmoThermalStats` (offset, emissive flux, albedo flux, and the `bodyTemperature` side effect), `GetTemperature`, `GetFullTemperature(alt, offset)`, `GetFullTemperature(pos)`, pressure, density and `GetSolarAtmosphericEffects`.
- **Bodies:** Kerbin, Eve, Duna, Laythe, Jool and Mun (Mun exercises the airless body-flux branch).
- **Epochs:** UT0, +¼ and +½ Kerbin solar day, and +¼ Kerbin year, set with `Planetarium.SetUniversalTime` followed by 2 physics frames.
- **Per-epoch state:** body rotation, `bodyTransform.up` and the referencing body's true anomaly are in `body_epoch`.

**C6 (`thermal_step`).**
- **Setup.** PhysProf-S, logged every physics frame after FlightIntegrator ran. Five parts are tracked: probe root, Rockomax32, Mainsail, LV-T45 and nose cone.
- **Cases:**
  - pad: right after load, engines off;
  - orbit: `SetShipOrbit` to a 100 km circular orbit;
  - reentry: `SetShipOrbit` to PeA 25 km / ApA 500 km, placed at 80 km descending.
- **Vessel table:** the FI environment terms (density, temperatures, convective coefficient, solar/body fluxes, `densityThermalLerp`, `isAnalytical`).
- **Parts table:** temperatures, skin temperatures and every flux term of `Part` and `PartThermalData`.

**C8 (`orbit_rails_ingame`).**
- **Probes.** Six `probeCoreOcto` probes are spawned with `ProtoVessel.CreateVesselNode` + `Game.AddVessel` (orbit elements go through G17 text, so they are exact):
  - 4 harness `orbit_propagation` cases;
  - `kerbin_e081` with its sma raised to 4,000 km, because 3,000 km puts its periapsis inside Kerbin;
  - a Mun encounter constructed backwards from a Mun-relative hyperbola (50 km periapsis, v∞ 362 m/s).
- **Epoch.** All epochs are the in-game UT at spawn, 2,300,908 s (after C4's +¼-year jump), not 0.
- **Warp.** `TimeWarp.SetRate(instant)` through 1×, 100×, 1000×, 10⁴× and 10⁵×, with the active vessel landed on the pad.
- **Sampling.** Every 10th physics frame: `orbit.pos/vel`, `getRelativePositionAtUT(UT)`, `GetWorldPos3D() − mainBody.position`, elements and `updateUT`; Mun and Minmus likewise.
- **Frames table.** `Planetarium.Zup` and `inverseRotAngle` per sample.
- **Destroyed probe.** The Mun-encounter probe is destroyed on its second Mun encounter: its periapsis is 7.9 km from the Mun's centre, below the surface. Its samples stop at 315.

**C10 (`aero_forces_vessel`).**
- **Ascent.** Scripted vertical ascent: stage 1, full throttle, SAS. Snapshots are taken at altitude 1/2.5/5/10/20/40 km, Mach 0.5/1/1.5, and at the last frame. The last frame is `apex` when vertical speed turns negative, otherwise `final` (90 s cap).
- **Per-part data:** the cube arrays (`AreaOccluded`, `WeightedArea/Drag/Depth` per face), the `SetDrag` outputs, `dragScalar`, `bodyLiftScalar` and `CalculateDragValue`.
- **Structure table:** parent part, attach node and the `AttachNode.contactArea` values set by `SetPartOcclusion`.
- **`aoa_sweep`** (0/5/10/20/45/90°) is synthetic. `SetDrag` is run on the live occluded cubes, with the live part-local flow vector rotated about the vessel pitch axis.
- **Outcomes per craft:**
  - **S:** went transonic, tumbled (AoA 173° at 2.5 km) and reached apex at 3.4 km.
  - **M2:** reached 36.7 km at Mach 1.82.
  - **L2-AS:** reached apex at 11.5 km.
- **ElectricCharge.** EC is refilled before each ascent. After C8's 1.3e6 s warp the S probe core had 0/30 EC and no control authority, so the throttle stayed at 0.

## Surprises for the C++ port

- **WP-05 (part database and loader):**
  1. **Double-typed Part fields are parsed with `float.Parse`.** `PartLoader.ApplyPartValue` tests `FieldType == typeof(double)` twice, and the first branch parses as float. So `emissiveConstant = 0.7` loads as 0.699999988079071; 219 values are affected (emissiveConstant 158, heatConductivity 60, radiatorMax 1).
  2. **Crewed parts lose mass at load.** They get `mass -= PhysicsGlobals.PerSeatReduction × CrewCapacity` (`PerCommandSeatReduction` for command parts): 0.094 t per seat on 11 parts, 0.045 t on 5, e.g. mk1pod.v2 0.8 → 0.706.
  3. **Part-cfg DRAG_CUBE overrides PartDatabase.cfg.** 67 cubes come from a `DRAG_CUBE` node in the part's own cfg (the intakes, Potato asteroids/comets, some probes) and take precedence over PartDatabase.cfg.
  4. **PartDatabase.cfg has a malformed nesting.** It contains a PART nested inside another PART (MpoProbe inside Size5Flag). Only top-level PART nodes count. With that rule, the loaded cubes are bit-exact with the cfg text.
  5. **DLC-suit EVA parts are listed without their DLC.** `kerbalEVAVintage/Future` and the female variants appear in `LoadedPartsList` even though neither DLC is installed. Reading their prefab module list throws NullReferenceException, so they are recorded as error rows.
  6. **Some textures are not in GameData.** 733 texture slots on parts come from Unity assets, not GameData.
- **WP-11 (terrain):**
  - **The stack evaluates the same in every scene.** It is set up in MAINMENU, even though `isStarted` is false there (as it is for the Mun in FLIGHT), and gives bit-identical heights in MAINMENU and FLIGHT. So a headless C++ port can be checked against MAINMENU captures.
  - **Flatten/decal mods depend on active quads.** FlattenArea and MapDecal(Tangent) depend on `quadActive` unless `overrideQuadBuildCheck` is set. With KSC loaded, all Kerbin points agree, but 3 Mun points differ, so pick the override variant as the golden.
  - **Heightmaps are Unity assets, not GameData files.** `kerbin_height_new` and `munHeightMap2` are 8-bit 4096×2048 MapSO; the decals are 16-bit.
- **WP-17 (delta-v):**
  - **"ASL" follows the selected body.** In the editor, `deltaVatASL` and `deltaVActual` use the pressure of `DeltaVAppValues.body` at `DeltaVAppValues.altitude`, so "ASL" on the Mun equals vacuum.
  - **Kerbin sea level is not exactly 1 atm.** `atmPressure` at Kerbin sea level is float `0.99999994`.
  - **Air-breathing craft:** vacuum dV is 0, but ASL dV is 8.9–38 km/s on every body including the Mun (intake air is not checked).
  - **Learstar A1 Mun "ASL" exceeds its vacuum figure:** 4618.62 vs 4618.08.
  - **No engines, no ready flag.** Bug-E Buggy never sets `IsReady`, but `onDeltaVCalcsCompleted` still fires.
  - **Multi-engine stages are integrated per phase.** Stages whose engines differ in Isp or burn time do not reduce to one rocket equation (S stage 1: −0.28 %).
- **WP-07/08 (orbits):**
  - **`getRelativePositionAtUT` uses the live `Planetarium.Zup` frame.** The same goes for `Orbit.pos`. `Zup` rotates with `inverseRotAngle` while the reference frame is rotating (active vessel below 100 km or landed). An inertial C++ propagation must apply `Zup.WorldToLocal` to match; it then agrees to 4.7e-13.
  - **SOI transitions under warp happen in the expected rails path:** `CheckDominantBody` → `OnRailsSOITransition`, with the BSP solver locating the crossing UT.
- **WP-21/22 (aero/thermal):**
  - **Occlusion acts only through stack attach nodes.** `SetPartOcclusion` only uses attach nodes with an attached part: `areaOccluded = max(0, weightedArea − neighbour facing area)`. Surface attachment only sets `contactArea`. About 27–29 % of the summed cube area is occluded on these craft.
  - **`GetFullTemperature(Vector3d pos)` returns the temperature *offset*.** It does not return a temperature, and it uses an unnormalised sun vector (2,240/2,688 rows differ from the FI path).
  - **FI stays in integrated mode in all three C6 cases.** `isAnalytical` is false throughout, so the per-frame integrator is the reference.

## Batch 2 rerun (one command each)

Prerequisite: the same as batch 1. Check out `profiling` and build it (`bash decompiled/tools/scripts/build_opt.sh capture2-NN`). Then run one of the following. Batch-2 items may be combined with commas; they share one launch, and flight items always launch PhysProf-S.

```bash
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh c9        # MAINMENU only, ~1 min  -> out2/part_database.json
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh terrain   # + PhysProf-S flight, ~1.5 min -> out2/terrain_height.json
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh c5        # editor, ~1.5 min -> out2/deltav_readout.json
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh c4        # -> out2/atmosphere_thermal_offsets.json
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh c6        # -> out2/thermal_step.json
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh c8        # -> out2/orbit_rails_ingame.json
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh c10       # three launches, ~6 min -> out2/aero_forces_vessel.json
bash /c/Projects/KSP-UE5/harness/captures/run_capture.sh batch2    # everything, ~8 min (the published data)
```

Each command launches KSP via `capture_ksp.ps1` with `-OutDir out2` and a 1,800 s timeout. It then runs `normalize.py`, `verify_hex.py` and `batch2_report.py <item>` on each file. The default tag is `capture2-<items>-<date>`; the logs are `decompiled/logs/ksp-capture-<tag>-{player,KSP}.log`.

**Results depend on order.** C4 jumps UT by a quarter year and C8 warps 1.3e6 s; C10 and C6 run after both. Running a single item gives a different UT and a different vessel history than the published `batch2` run.

## Batch 2 files

| Path | What |
|---|---|
| `out2/part_database.json` | C9 |
| `out2/terrain_height.json` | terrain heights (WP-11) |
| `out2/deltav_readout.json` | C5 |
| `out2/atmosphere_thermal_offsets.json` | C4 |
| `out2/thermal_step.json` | C6 |
| `out2/orbit_rails_ingame.json` | C8 |
| `out2/aero_forces_vessel.json` | C10 |
| `reports2/*_summary.md` | per-item checks and summaries |
| `tools/batch2_report.py` | the checks |
| `PROGRESS-batch2.md` | work log |

## Batch 2: not verified or not achievable

- **C4 grid is smaller than the spec's for Mun.** The UT jumps use `Planetarium.SetUniversalTime` with the vessel landed. For the airless Mun, the 4th altitude is 60 km instead of 0.9 × atmosphereDepth.
- **C5 skips the Mun, Duna and Eve altitude-0 *vessel* (flight) mode.** Only the editor (Ship mode) readout was captured. Flight-mode `VesselDeltaV` uses `vessel.mainBody` and live pressure instead.
- **C6 is shorter than the spec's 3,000 frames, and spec case B was replaced.** Frames are 500 (pad), 1,000 (orbit) and 1,500 (reentry). Spec case B (LV-T45 + FL-T800 clamped at full throttle) was replaced by the brief's parked-on-pad case. The Mk1 pod + heat shield of spec case A was replaced by PhysProf-S, so there is no heat-shield ablation data.
- **C8 samples every 10 frames, not every 50, and does not cover 2 orbits at every warp rate.** The 1× segment is 10 s. The 100× and 1000× segments cover 2,000 s and 20,000 s.
- **C10 is not the spec's Kerbal X, and AoA states are synthetic.** It uses the three PhysProf craft. AoA states are synthetic (`aoa_sweep`), except S's natural tumble. `AeroDataDisplay` was not switched on: it is not needed, because the values are read from `Part`/`DragCubeList` directly.
- **Terrain covers only Kerbin and Mun at 1°.** The map assets are named but not exported; extraction needs AssetStudio/UnityPy on `KSP_x64_Data`.
- **Determinism was not re-checked by a second identical `batch2` run.** In three different runs, C9 cube values, terrain probe heights (`h(KSC)=600064.78470541444`) and C5 totals were identical in the logs and summaries. Flight items depend on physics and frame timing.
