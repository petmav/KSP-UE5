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

Manual equivalent from a "x64 Native Tools Command Prompt for VS 2022":

```bat
cd C:\Projects\KSP-UE5\core
cmake --preset relwithdebinfo
cmake --build --preset relwithdebinfo
ctest --preset relwithdebinfo
```

### Toolchain on this machine

| tool | path / version |
|---|---|
| MSVC | VS 2022 Community 17.14, MSVC 14.44.35207 (`C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\HostX64\x64\cl.exe`) |
| CMake | 3.31.6-msvc6, bundled: `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe` (a system CMake 4.3.2 is also on PATH; either works, >= 3.28 required) |
| Ninja | 1.12.1, bundled: `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe` (the `ninja` on PATH is Strawberry Perl's 1.12.0; the script puts VS's first) |
| Python | 3.12.10 on PATH (`python -I` runs `harness\compare\compare.py`; found by CMake's `find_package(Python3)`) |
| Windows PowerShell | 5.1 on .NET Framework 4.8: only needed to regenerate `tests/data/netfx_reference.tsv` |

Presets (`CMakePresets.json`): `debug`, `relwithdebinfo` (x64, Ninja, cl.exe). Build trees: `build/<preset>/`.

## What the tests are

| ctest name | what it checks | reference |
|---|---|---|
| `unit` | doctest unit tests (`tests/`), including `test_netfx_reference.cpp`: 29,028 bit-exact reference results from the .NET Framework 4.8 runtime (parsing, formatting, Math.Sin/Cos/Tan/Ceiling/Round) | `tests/data/netfx_reference.tsv` |
| `golden.math_shim.f64` | WP-01 KSP double maths: Vector3d, QuaternionD, UtilMath, CelestialFrame (runtime-independent) | `harness/golden/math_shim` (table f64) |
| `capture.math_shim.f32` | WP-01 Unity managed float maths (Mathf, Vector3, Quaternion) under Mono's float32 rule | in-game `harness/captures/out/math_shim_mono.json` (table f32) |
| `capture.libm_mono` | runtime layer: System.Math (25 functions x 3,093 inputs), float32 expression evaluation probes, numeric conversions, number formatting and parsing, System.Random (~312k rows) | in-game `harness/captures/out/libm_mono.json` |
| `capture.unity_random` | WP-01 UnityEngine.Random (xorshift128, value/Range/vector functions/ColorHSV, traced states) and KSPRandom | in-game `harness/captures/out/unity_random.json` |
| `golden.confignode_roundtrip` | WP-02 ConfigNode Load/Save/ToString/Parse; the runner also checks byte-exact `saved/*.cfg` and tree-exact `trees/*.json` | `harness/golden/confignode_roundtrip` |
| `golden.curves_eval` | WP-01 FloatCurve / Unity AnimationCurve: 366 curves, 28,484 evaluations | `harness/golden/curves_eval` (== native Unity, CAPTURE-SPEC C1) |
| `corpus.gamedata` | every `.cfg` under `harness/inputs/GameData` and `.craft` under `harness/inputs/Ships`: load, Save->Load->Save byte-identical, Parse == Load, every `key` node loads as a FloatCurve | self-consistency |

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

1. `build/<preset>/tools/golden_runner/golden_runner.exe <dataset> --harness ..\harness --out build/<preset>/candidate`
   recomputes the dataset with kspcore and writes `candidate/<dataset>/<dataset>.json` in the harness's
   `ksp-golden/1` schema (same tables, keys and number text format as the golden file). Dataset-specific exact checks
   that are not tables (ConfigNode's saved bytes and parse trees) run inside the runner; any mismatch is a non-zero exit.
2. For gates that use only some tables of a golden file, or an in-game capture file (which carries `<col>_hex`
   companion columns), `python -I tools/filter_golden.py` writes a filtered copy of the golden file into the build tree.
3. `python -I ..\harness\compare\compare.py <golden> <candidate> --tol <tolerances> --report
   candidate/<dataset>/compare_report_<gate>.json`; any out-of-tolerance column, missing row/column or table fails.

The runner never reads golden *outputs* as inputs: inputs come from the same sources the harness used
(`harness/inputs/**`, `harness/golden/serialized/*.serialized.json` for Unity-serialized prefab data, the
`synthetic_inputs/` files, `harness/captures/in/*.tsv`, and the generators' literal input lists mirrored from
`harness/src/KspGolden/Generators/` and the capture code).

## Layout

| path | contents |
|---|---|
| `include/ksp/dotnet/` | .NET/Mono runtime semantics KSP depends on: `number.hpp` (Mono/CLR number parsing incl. the runtime's NumberToDouble; `G`/`F`/`R` formatting per runtime), `math.hpp` (System.Math with x87 Sin/Cos, VS2013-CRT-backed functions, Mono/.NET differences, C# numeric conversions), `mono_float.hpp` (Mono's float32 evaluation rule: `fx()` / `f32store()`), `random.hpp` (System.Random, KSPRandom), `text.hpp` (Trim/Split, UTF-8, File.ReadAllLines decoding) |
| `include/ksp/unity/` | Unity value types and maths: `mathf.hpp`, `vector.hpp` (Vector2f/3f/4f), `quaternion.hpp` (Quaternionf), `color.hpp` (Color, Color32, Rect, Matrix4x4f), `animation_curve.hpp` (Keyframe, AnimationCurve: port of the native AnimationCurveTpl<float>), `random.hpp` (UnityEngine.Random, Color.HSVToRGB) |
| `include/ksp/math/` | KSP double maths: `vector3d.hpp` (Vector2d/3d/4d), `quaterniond.hpp`, `util_math.hpp` (UtilMath, Planetarium.CelestialFrame) |
| `include/ksp/curves/` | `float_curve.hpp` (FloatCurve) |
| `include/ksp/config/` | `config_node.hpp` (ConfigNode, Guid), `parse_extensions.hpp` (ParseExtensions, ColorUtility.TryParseHtmlString) |
| `include/ksp/core/` | `log.hpp` (Debug.Log replacement with an installable sink) |
| `src/` | implementations, same tree |
| `tests/` | doctest unit tests; `tests/data/netfx_reference.tsv` reference data; `tests/tolerances_capture.json` tolerances for in-game capture gates; `tests/CMakeLists.txt` declares every gate (`ksp_golden_gate`) |
| `tools/golden_runner/` | the golden runner (one `ds_<dataset>.cpp` per harness dataset) |
| `tools/netfx_reference/` | `gen_netfx_reference.ps1`, regenerates `tests/data/netfx_reference.tsv` |
| `tools/filter_golden.py` | writes a table-filtered / `_hex`-stripped copy of a golden or capture file for compare.py |
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
- **Thread safety**: everything is single-threaded like KSP unless stated. `AnimationCurve::Evaluate` is const and
  stateless (thread-safe) for strictly increasing key times; `dotnet::SetRuntime`, `log::SetSink` and
  `UnityRandom::Global()` are process-wide.
- Namespaces: `ksp` (game types), `ksp::Mathf`, `ksp::UtilMath`, `ksp::ParseExtensions`, `ksp::dotnet`. Types keep their
  KSP names; Unity float types get an `f` suffix (`Vector3f`, `Quaternionf`) to sit next to KSP's `Vector3d`.

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
   [TABLES t1,t2])` for an in-game capture. Float32 managed paths are judged against captures where they exist.
5. Run `run_tests.ps1`; inspect `build/<preset>/candidate/<dataset>/compare_report_<gate>.json` for per-column
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
