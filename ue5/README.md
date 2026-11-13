# ue5/ : KspUe5, the Unreal Engine 5.8 shell

Private, local-only. Thin UE 5.8 C++ shell over the engine-agnostic simulation core (`..\core`, built separately with CMake).
No Blueprints: everything is C++ so agents can read and change it.

| Document | What |
|---|---|
| `DESIGN.md` | UE5 shell design: core vs shell split, the C ABI bridge contract, scene flow, rendering, UI (Slate), input / cameras / audio / localization, the first playable slice (WP-UE-01..18), open decisions |
| `STATUS.md` | what works today, build and iteration timings |
| `spike/RESULTS.md` | phase-0 scale spike (LWC, rebasing, frame times) |

```
KspUe5.uproject
Source/KspUe5/            game module: spike code (Spike/), automation tests (Tests/)
Source/KspUe5Editor/      editor-only module: KspCreateSpikeMap commandlet (generates Content/Maps/ScaleSpike.umap)
Plugins/KspCoreBridge/    links kspcore.lib through a C ABI (kspcore/kspcore_c.h)
Stub/                     placeholder kspcore.lib (CMake, /MD) + header, until core/ ships the real lib
Scripts/                  one-liner wrappers; every run appends wall-clock time to timings.log
spike/                    phase-0 scale spike: RESULTS.md, data/*.csv, shots/*.png, logs/
```

Engine: `C:\Program Files\Epic Games\UE_5.8` (launcher build 5.8.3). Toolchain: VS 2022, MSVC 14.44. All commands are PowerShell,
run from anywhere (scripts resolve paths themselves). Do not run more than one editor/game instance at a time.

## One-liners

| What | Command | Typical time on this laptop |
|---|---|---|
| Build the stub core lib (only if `Stub/` changed) | `.\Scripts\build_stub.ps1` | 6 s |
| Generate VS solution (`UnrealBuildTool -projectfiles` via Build.bat) | `.\Scripts\genproj.ps1` | 14 s |
| Build editor (Development Editor Win64) | `.\Scripts\build.ps1` | 64 s clean, 10 s after one .cpp, 1 s no-op |
| Build game (Development Win64) | `.\Scripts\build.ps1 -Target KspUe5` | 52 s first build |
| Cook + stage + pak the game (after building it) | `.\Scripts\cook.ps1` | 323 s |
| Rebuild only the bridge plugin module | `.\Scripts\build.ps1 -Module KspCoreBridge` | 2.5 s |
| (Re)generate the ScaleSpike map | `.\Scripts\create_map.ps1` | 29 s |
| Run automation tests headless | `.\Scripts\test.ps1` | 16 s warm (42 s first ever) |
| Run the full scale spike | `.\Scripts\spike.ps1 -Tag full2` | 3.5 min |
| Quick spike smoke run | `.\Scripts\spike.ps1 -Tag smoke -Quick -Variants None,UE10k -Stress 0` | 46 s |
| Spike CSVs to Markdown tables | `python -I spike\analyze.py full2` | 1 s |

Raw equivalents (what the scripts run):

```powershell
$UE = 'C:\Program Files\Epic Games\UE_5.8'; $P = 'C:\Projects\KSP-UE5\ue5\KspUe5.uproject'
& "$UE\Engine\Build\BatchFiles\Build.bat" -projectfiles "-project=$P" -game -rocket -progress
& "$UE\Engine\Build\BatchFiles\Build.bat" KspUe5Editor Win64 Development "-Project=$P" -WaitMutex
& "$UE\Engine\Build\BatchFiles\Build.bat" KspUe5 Win64 Development "-Project=$P" -WaitMutex
& "$UE\Engine\Build\BatchFiles\Build.bat" KspUe5Editor Win64 Development "-Project=$P" -WaitMutex -Module=KspCoreBridge
& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $P -run=KspCreateSpikeMap -unattended -nopause -nullrhi
& "$UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" $P "-ExecCmds=Automation RunTests KspUe5; Quit" -unattended -nopause -nullrhi -log
& "$UE\Engine\Binaries\Win64\UnrealEditor.exe" $P /Game/Maps/ScaleSpike -game -windowed -ResX=1280 -ResY=720 -NoSound -unattended -SpikeTag=full
```

Notes:
- `UnrealBuildTool.exe` cannot be called directly on this machine: it needs the .NET 10 runtime, which is only present as the
  engine-bundled copy (`Engine\Binaries\ThirdParty\DotNet\10.0`). `Build.bat` uses that copy, so all scripts go through it.
- Live Coding is disabled for the project (`Config/DefaultEditorPerProjectUserSettings.ini`): command-line UBT builds are the loop.
  UBT refuses to build while an editor with Live Coding holds the DLLs.
- `spike.ps1` runs uncooked with editor binaries (`UnrealEditor.exe -game`). For a packaged run, use `.\Scripts\cook.ps1`
  (`RunUAT BuildCookRun -cook -stage -pak -skipbuild`, about 5.5 min) after building the game target, then
  `Saved\StagedBuilds\Windows\KspUe5.exe /Game/Maps/ScaleSpike -windowed -ResX=1280 -ResY=720 -SpikeTag=cooked`. Its spike
  output lands under `Saved\StagedBuilds\Windows\KspUe5\spike\`. Engine assets that are loaded by path must be listed in
  `DirectoriesToAlwaysCook` (`Config/DefaultGame.ini`).

## Tests

`Source/KspUe5/Tests/` (`KspBridgeTests.cpp`, `KspUe5Tests.cpp`), all under the `KspUe5.` prefix:

- `KspUe5.Bridge.LinksCore`: calls `kspcore_build_info()` through the bridge, checks /MD CRT, `_ITERATOR_DEBUG_LEVEL=0`, C++20, ABI v1 (compiled
  and reported), real core not the stub, manifest lists both libraries, manifest `JPH_*` definitions equal the Jolt configuration of the
  linked library.
- `KspUe5.Bridge.FpGuard`: MXCSR control bits inside core are 0x1F80 whatever the caller's state (FTZ / DAZ set), restored after; nested guards.
- `KspUe5.Bridge.JoltStep`: a core Jolt scenario stepped through the test ABI (`kspcore_test.h`) gives the same state hash as its ctest twin
  `capi.jolt_step` (which writes `core\build\RelWithDebInfo\kspcore_jolt_step.txt`; `Scripts\test.ps1` runs the twin first), run to run, at 4
  threads, and under a hostile caller MXCSR.
- `KspUe5.Precision.Float32VsLwc`: float32 world-space vs UE5 LWC vertex pipeline emulation at Kerbin radius.
- `KspUe5.Spike.CubeSpherePatch`: cubesphere patch geometry, float-local precision and winding.

Results: `Saved/Logs/automation.log` and the JSON report in `Saved/Automation/`.

## Linking the real core (the one-line switch)

`Plugins/KspCoreBridge/Source/KspCoreBridge/KspCoreBridge.Build.cs`:

```csharp
private static readonly bool bUseRealCore = true;   // false = the placeholder in Stub/
```

With `true` the bridge reads the **link manifest** the core build writes: `<KSPCORE_ROOT or <repo>\core>\build\RelWithDebInfo\kspcore_link.txt`
(override with env `KSPCORE_LINK_MANIFEST`). It lists `kspcore.lib` and `Jolt.lib` in link order, the include directory and the
definitions that must match (`JPH_*`, Jolt's configuration). Build the core first: `powershell core\run_tests.ps1 -BuildOnly`.
The build fails fast with a clear message if the manifest or a listed file is missing. The bridge module compiles with
`FPSemantics = Precise`, and every call into core goes through `FKspFpGuard` (MXCSR 0x1F80).

What the core must provide:
- `include/kspcore/kspcore_c.h` (`KSPCORE_ABI_VERSION`, `kspcore_build_info`) and the test-only `kspcore_test.h`. Only C types cross this boundary.
- `kspcore.lib` (+ `Jolt.lib`), static libraries built so that they link into UE 5.8:

| Setting | Required value | Why |
|---|---|---|
| Compiler | MSVC 14.4x (VS 2022), x64 | same toolset family as UE 5.8 (UBT picks 14.44 here) |
| CRT | `/MD` in every configuration (`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL`) | UE uses the release CRT DLL even for Debug editor builds; `/MDd` or `/MT` fails with LNK2038 |
| Iterator debug level | `_ITERATOR_DEBUG_LEVEL=0` (implied by `/MD`) | LNK2038 mismatch otherwise |
| Language | C++20 | UE 5.8 default (`/std:c++20`) |
| Whole-program optimisation | off (no `/GL`, no LTCG) | `/GL` objects are toolset-version locked |
| Debug info | `/Z7` (`CMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded`) or ship the PDB | avoids LNK4099 |
| Config | RelWithDebInfo | `/O2 /Ob1 /MD`, matches the expected path |
| ABI surface | C only | UE compiles with `/GR-` (no RTTI) and `_HAS_EXCEPTIONS=0`; STL/exception types must not cross |

UE floating-point note: the Editor target compiles with `/fp:precise`, but the Game target defaults to `/fp:fast`
(UBT `VCToolChain.cs`, BuildSettingsVersion V7). The core's own objects keep the core's flags, so keep numerically sensitive
code out of headers that UE translation units compile, or set `FPSemantics = FPSemanticsMode.Precise` in the module that
includes them.

## Game assets (WP-UE-06)

The Unity-serialized assets (scaled-space bodies, Kerbin PQS materials, GalaxyCube, KSC render meshes, fonts, flight HUD sprites) are
extracted once per machine into `GameAssets/` (gitignored) by `Tools/extract_game_assets.py`; `Tools/verify_game_assets.py` checks the
result by an independent route. Output layout, manifest and conventions: `Tools/README.md`.

```
harness\tools-cache\venv\Scripts\python.exe -I ue5\Tools\extract_game_assets.py --game decompiled\KSP-dev --out ue5\GameAssets   # 45 s
harness\tools-cache\venv\Scripts\python.exe -I ue5\Tools\verify_game_assets.py --game decompiled\KSP-dev --assets ue5\GameAssets  # 115 s
```

## Spike

See `spike/RESULTS.md`. Options for `spike.ps1` / the game command line: `-SpikeVariants=None,UE100k,UE10k,MW100k,MW10k`,
`-SpikeStress=0|1`, `-SpikeQuick`, `-SpikeTag=<name>`, `-SpikeSunLux=<lux>`, `-SpikeParts=457`, `-SpikeColliders=1325`,
`-SpikeFlipWinding`, `-SpikePlanetCentred`, `-SpikeNoSkyLight`. Outputs: `spike/data/<tag>_summary.csv` (one row per
variant x phase), `spike/data/<tag>_frames.csv` (one row per frame), `spike/shots/<tag>_*.png`, `spike/logs/<tag>.log`.
