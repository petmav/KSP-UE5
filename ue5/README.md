# ue5/ : KspUe5, the Unreal Engine 5.8 shell

Private, local-only. Thin UE 5.8 C++ shell over the engine-agnostic simulation core (`..\core`, built separately with CMake).
No Blueprints: everything is C++ so agents can read and change it.

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

`Source/KspUe5/Tests/KspUe5Tests.cpp`, all under the `KspUe5.` prefix:

- `KspUe5.Bridge.LinksCore`: calls `kspcore_build_info()` through the bridge, checks /MD CRT, `_ITERATOR_DEBUG_LEVEL=0`, ABI v1.
- `KspUe5.Precision.Float32VsLwc`: float32 world-space vs UE5 LWC vertex pipeline emulation at Kerbin radius.
- `KspUe5.Spike.CubeSpherePatch`: cubesphere patch geometry, float-local precision and winding.

Results: `Saved/Logs/automation.log` and the JSON report in `Saved/Automation/`.

## Linking the real core (the one-line switch)

`Plugins/KspCoreBridge/Source/KspCoreBridge/KspCoreBridge.Build.cs`:

```csharp
private static readonly bool bUseRealCore = false;   // -> true
```

With `true` the bridge uses `<KSPCORE_ROOT or C:\Projects\KSP-UE5\core>\include` and
`<root>\build\RelWithDebInfo\kspcore.lib` (override the lib path with env `KSPCORE_LIB`). Then rebuild the editor target.
The build fails fast with a clear message if the lib is missing.

What the core must provide:
- `include/kspcore/kspcore_c.h` with `#define KSPCORE_ABI_VERSION 1` and `extern "C" const char* kspcore_build_info(void);`
  (copy `Stub/include/kspcore/kspcore_c.h`). Only C types cross this boundary.
- `kspcore.lib`, a static library built so that it links into UE 5.8:

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

## Spike

See `spike/RESULTS.md`. Options for `spike.ps1` / the game command line: `-SpikeVariants=None,UE100k,UE10k,MW100k,MW10k`,
`-SpikeStress=0|1`, `-SpikeQuick`, `-SpikeTag=<name>`, `-SpikeSunLux=<lux>`, `-SpikeParts=457`, `-SpikeColliders=1325`,
`-SpikeFlipWinding`, `-SpikePlanetCentred`, `-SpikeNoSkyLight`. Outputs: `spike/data/<tag>_summary.csv` (one row per
variant x phase), `spike/data/<tag>_frames.csv` (one row per frame), `spike/shots/<tag>_*.png`, `spike/logs/<tag>.log`.
