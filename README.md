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

| ctest name | what it checks |
|---|---|
| `unit` | doctest unit tests (`tests/`), including `test_netfx_reference.cpp`: 29,028 bit-exact reference results from the .NET Framework 4.8 runtime (number parsing and formatting, Math.Sin/Cos/Tan/Ceiling/Round) |
| `golden.math_shim` | WP-01: Unity Mathf/Vector3/Quaternion and KSP Vector3d/QuaternionD/UtilMath/CelestialFrame vs `harness/golden/math_shim` |
| `golden.confignode_roundtrip` | WP-02: ConfigNode Load/Save/ToString/Parse vs `harness/golden/confignode_roundtrip`, plus byte-exact `saved/*.cfg` and tree-exact `trees/*.json` |
| `golden.curves_eval` | WP-01: FloatCurve / Unity AnimationCurve vs `harness/golden/curves_eval` (366 curves, 28,484 evaluations) |
| `corpus.gamedata` | every `.cfg` under `harness/inputs/GameData` and every `.craft` under `harness/inputs/Ships`: load, Save->Load->Save byte-identical, Parse == Load, every `key` node loads as a FloatCurve |

### The golden gate

`ctest` runs `cmake -P cmake/RunGolden.cmake` per dataset:

1. `build/<preset>/tools/golden_runner/golden_runner.exe <dataset> --harness ..\harness --out build/<preset>/candidate`
   recomputes the dataset with kspcore and writes `candidate/<dataset>/<dataset>.json` in the harness's
   `ksp-golden/1` schema (same tables, keys and number text format as the golden file). Dataset-specific exact checks
   that are not tables (ConfigNode's saved bytes and parse trees) run inside the runner; any mismatch is a non-zero exit.
2. `python -I ..\harness\compare\compare.py <golden> <candidate> --tol ..\harness\compare\tolerances.json --report
   candidate/<dataset>/compare_report.json`; any out-of-tolerance column, missing row/column or table fails the test.

The runner never reads golden *outputs* as inputs: inputs come from the same sources the harness used
(`harness/inputs/**`, `harness/golden/serialized/*.serialized.json` for Unity-serialized prefab data, the
`synthetic_inputs/` files, and the generator's literal input lists mirrored from `harness/src/KspGolden/Generators/`).

## Layout

| path | contents |
|---|---|
| `include/ksp/dotnet/` | .NET runtime semantics KSP depends on: `number.hpp` (Mono/CLR number parsing incl. the runtime's NumberToDouble, .NET `G`/`G9`/`G17` formatting), `math.hpp` (System.Math Round/Ceiling/Sin/Cos/Tan with selectable runtime flavour), `text.hpp` (Trim/Split, UTF-8, File.ReadAllLines decoding) |
| `include/ksp/unity/` | Unity value types and maths: `mathf.hpp`, `vector.hpp` (Vector2f/3f/4f), `quaternion.hpp` (Quaternionf), `color.hpp` (Color, Color32, Rect, Matrix4x4f), `animation_curve.hpp` (Keyframe, AnimationCurve: port of the native AnimationCurveTpl<float>) |
| `include/ksp/math/` | KSP double maths: `vector3d.hpp` (Vector2d/3d/4d), `quaterniond.hpp`, `util_math.hpp` (UtilMath, Planetarium.CelestialFrame) |
| `include/ksp/curves/` | `float_curve.hpp` (FloatCurve) |
| `include/ksp/config/` | `config_node.hpp` (ConfigNode, Guid), `parse_extensions.hpp` (ParseExtensions, ColorUtility.TryParseHtmlString) |
| `include/ksp/core/` | `log.hpp` (Debug.Log replacement with an installable sink) |
| `src/` | implementations, same tree |
| `tests/` | doctest unit tests; `tests/data/netfx_reference.tsv` reference data |
| `tools/golden_runner/` | the golden runner (one `ds_<dataset>.cpp` per harness dataset) |
| `tools/netfx_reference/` | `gen_netfx_reference.ps1`, regenerates `tests/data/netfx_reference.tsv` |
| `cmake/RunGolden.cmake` | the per-dataset gate |
| `third_party/` | doctest 2.4.12, nlohmann/json 3.12.0 (pinned, see `third_party/VERSIONS.md`); used by tests/tools only |

## Conventions

- **Faithful port first.** Translate the decompiled C# operation for operation, including its quirks and bugs (they are
  marked `sic` or "KSP quirk" in comments). Each ported function carries a comment citing the C# source, e.g.
  `// ConfigNode.cs:3884 PreFormatConfig`; paths are relative to `decompiled/KSP-Decompiled/Assembly-CSharp/` unless
  they start with `Unity/`, `Mono/` or `harness/`. Performance work comes later and must keep the goldens green.
- **Precision.** `double` for world/orbital state (KSP's `Vector3d`, `QuaternionD`, `Orbit`...). `float` exactly where
  KSP/Unity use `float` (`Mathf`, `Vector3f`, `Quaternionf`, `FloatCurve`, most part/aero maths). Keep C#'s evaluation
  order and parenthesisation; C# `(float)Math.F(x)` becomes `static_cast<float>(dotnet::Math::F(double(x)))`.
  Compiler flags: `/fp:precise`, x64 SSE2, no FMA contraction (never `/fp:fast` or `/arch:AVX2` on kspcore).
- **Runtime semantics live in `ksp::dotnet`.** Use `dotnet::ParseSingle/TryParseDouble/...` instead of `strtod`,
  `dotnet::FormatSingleG/FormatDoubleG` instead of `printf`, `dotnet::Math::Round/Ceiling/Sin/Cos/Tan` instead of the
  CRT where C# calls System.Math, `dotnet::Trim/Split` for string ops, `dotnet::ConvToInt32` for `(int)double` casts.
- **Libm flavour.** `dotnet::SetLibm(Libm::NetFramework | Libm::Ucrt)`. Default `NetFramework` reproduces the harness
  oracle (x87 FSIN/FCOS/FPTAN semantics, `Ceiling(-0.5) == +0`). Which flavour KSP's Mono matches is CAPTURE-SPEC C7.
- **Unity value-type equality**: `operator==` on `Vector3f`/`Vector2f`/`Vector4f`/`Quaternionf` is Unity's approximate
  test; use `.Equals()` for exact comparison.
- **Errors**: where C# throws, kspcore throws (`dotnet::FormatException`, `OverflowException`, `ksp::NullReferenceException`,
  `std::out_of_range` for IndexOutOfRange). Where C# logs and continues, kspcore calls `ksp::log::Warning/Error`.
- **Nullable strings**: a C# `string` that can be null is `std::optional<std::string>` (getters) or `const std::string*`
  (`GetValuePtr`, no copy).
- **Reference semantics**: `ConfigNode` children are `std::shared_ptr<ConfigNode>`; `GetNode` returns a non-owning
  pointer into the tree; `ConfigNode` itself is move-only (use `CreateCopy()` for a deep copy).
- **Thread safety**: everything is single-threaded like KSP unless stated. `AnimationCurve::Evaluate` is const and
  stateless (thread-safe) for strictly increasing key times; `dotnet::SetLibm` and `log::SetSink` are process-wide.
- Namespaces: `ksp` (game types), `ksp::Mathf`, `ksp::UtilMath`, `ksp::ParseExtensions`, `ksp::dotnet`. Types keep their
  KSP names; Unity float types get an `f` suffix (`Vector3f`, `Quaternionf`) to sit next to KSP's `Vector3d`.

## Adding a work package with a golden gate

1. Port the C# into `include/ksp/<area>/` + `src/<area>/` and add the `.cpp` files to `src/CMakeLists.txt`.
2. Read the dataset's generator in `harness/src/KspGolden/Generators/` and its `harness/golden/<dataset>/README.md`.
3. Add `tools/golden_runner/ds_<dataset>.cpp`: rebuild the generator's inputs (from `harness/inputs`, the serialized
   prefab JSON or the generator's literal lists, never from the golden outputs), compute every column with kspcore and
   write the tables with `golden::File`/`golden::Table` (`golden_json.hpp`), then
   `return WriteCandidate(opt, g, "<dataset>/<dataset>.json");`. Register it at the end of the file with
   `KSP_GOLDEN_DATASET("<dataset>", <order>, RunMyDataset);` and add the file to `tools/golden_runner/CMakeLists.txt`.
4. Add `<dataset>` to `KSP_GOLDEN_DATASETS` in `tests/CMakeLists.txt`. `ctest` now fails until `compare.py` passes.
5. Run `run_tests.ps1`; inspect `build/<preset>/candidate/<dataset>/compare_report.json` for per-column max abs/rel/ULP.
6. Unit-test API behaviour the golden does not cover in `tests/test_<area>.cpp`.
7. Commit the package on its own (`git commit` in `core/` only; local repository, never pushed) and update STATUS.md.

Tolerances are owned by the harness (`harness/compare/tolerances.json`); do not loosen them from here. If a gate cannot
pass because the oracle runtime differs from the game (as with the x87 trig), reproduce the oracle in `ksp::dotnet`
behind a documented switch and record it in STATUS.md.

## Regenerating the .NET reference data

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File C:\Projects\KSP-UE5\core\tools\netfx_reference\gen_netfx_reference.ps1
```

Deterministic (fixed seed); needs Windows PowerShell 5.1 (CLR 4.0.30319, i.e. .NET Framework 4.8, the same runtime the
harness ran KSP's IL on).
