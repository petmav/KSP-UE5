# KSP 1.12.5 decompilation - STATUS

Date: 2026-10-08. Guide: "How to decompile KSP" by SabrCat (`_reference\HowToDecompileKsp.md`), with GUI steps replaced by CLI.
**Outcome: steps 0-4 completed and verified; step 5 (stretch) completed.** The custom-compiled Assembly-CSharp.dll (Debug + portable PDB) builds with 0 errors. KSP-dev loads it and reaches the main menu with no exceptions. A process-memory MVID check confirms the custom DLL is the one loaded.
**Update (same day, after the folder rename to `decompiled`):** symlinks restored and the launch re-verified; every other managed assembly in the install (174 more) decompiled read-only and indexed in `KSP-Decompiled\INDEX.md`. See the **Post-rename** and **Full coverage** sections at the end.

## Tool versions

| Tool | Version / source | Used for |
|---|---|---|
| de4dot | **v3.2.0.0, kant2002 fork prebuilt release** `de4dot-net48.zip` (sha256 2267b72a...870d04), from github.com/kant2002/de4dot/releases/tag/v3.2.0 | Step 1. Worked on the first try, so the de4dot/de4dot source build was not needed. |
| ILSpy engine | **ICSharpCode.Decompiler 8.2.0.7535** (NuGet). This is the same engine version as the ILSpy 8.2.0.7535 GUI and ilspycmd 8.2.0.7535. | Step 2, through a small headless tool `tools\IlspyExport` |
| ilspycmd | 8.2.0.7535 (nupkg extracted, run on .NET runtime 6.0.36) | Cross-check of step 2, plus IL inspection |
| MSBuild | 17.14.40.60911 (VS 2022 Community 17.14.36930.0), Roslyn csc 4.14.0-3.25412.6 | Step 3 |
| .NET SDK | 8.0.425, **portable** (dotnet-install.ps1 into `tools\dotnet-sdk`, not added to PATH, nothing installed system-wide) | Building IlspyExport only |
| KSPBuildTools | git submodule, tag **1.1.1** (commit 2efb3f6) | Step 3 |
| Git | 2.53.0.windows.1 | local repo only, no remote |
| Game | KSP 1.12.5 build 03190 (Steam), Unity 2019.4.18f1 | |

## Step-by-step outcome

### 0. Dev install - DONE (verified)
- `robocopy <Steam KSP> C:\Projects\KSP-UE5\decompiled\KSP-dev /E /MT:16`: **3,783 files, 505 dirs, 5,216,582,711 bytes (4.86 GiB), 14.5 s**. Exit code 1 means "files copied, no failures".
- The Steam install was never written to. Its Assembly-CSharp.dll sha256 (d9e42483...) matches `backup\Assembly-CSharp.dll`, and no Steam files are newer than the start of the task.

### 1. de4dot - DONE (verified)
- `de4dot --dont-rename KSP-dev\KSP_x64_Data\Managed\Assembly-CSharp.dll` printed **"Detected Crypto Obfuscator"** and saved `Assembly-CSharp-cleaned.dll` (8,972,288 bytes; the original is 10,790,912 bytes) in about 4 s.
- A verbose run (`-v`, to a scratch output) gave these counts: **49,017 methods deobfuscated; 579,662 dead instructions removed in 27,802 methods; 58 unused locals removed in 52 methods; `SuppressIldasmAttribute` removed; empty .cctors removed.**
- Logs: `logs\de4dot.log`, `logs\de4dot-detect.log`, `logs\de4dot-verbose.log`.

### 2. Decompile (C# 7.3, legacy csproj, nested namespace dirs) - DONE (verified)
- ilspycmd 8.2 has no switch to turn off the SDK-style project format, so I used option (b). `tools\IlspyExport` runs `WholeProjectDecompiler` with `DecompilerSettings(LanguageVersion.CSharp7_3) { UseSdkStyleProjectFormat = false, UseNestedDirectoriesForNamespaces = true }`. All other settings are left at their defaults, the same as a fresh ILSpy GUI. The reference search directory is KSP-dev\Managed, and all 31 assembly references resolved there.
- Output: `KSP-Decompiled\Assembly-CSharp\` (empty beforehand) containing **3,299 .cs files, 627,871 lines** (raw), plus `Assembly-CSharp-cleaned.csproj` (legacy format, 3,299 `<Compile Include>` items). The export took 22 s. No embedded resources.
- Cross-check: ilspycmd 8.2.0.7535 (`-p -lv CSharp7_3 --nested-directories -r Managed`) produced the same 3,299 files. 22 of them differ only in the order of `using` lines (sorted contents are identical). Only the csproj format differs.

### 3. Project setup and build - DONE (verified)
- `git init` in `KSP-Decompiled\` (local only, no remote). Commits:
  - `9ca6e64` raw decompilation
  - `3dd2d6b` build setup + fixes
  - `3e98553` stretch decompilations
- `Assembly-CSharp-cleaned.sln` was hand-written one level above the project.
- `git submodule add https://github.com/KSPModdingLibs/KSPBuildTools.git` (1.1.1).
- csproj edits from guide 3.2 (applied by `tools\scripts\edit_csproj.py`):
  - Removed ILSpy's 30-item `<Reference>` ItemGroup.
  - Imported KSPCommon.targets after Microsoft.CSharp.targets.
  - Added the guide's reference block, `Reference Remove` of Assembly-CSharp.dll included.
  - OutputType=Library and LangVersion=7.3 were already in the ILSpy 8.2 output.
- KSP install location: `Assembly-CSharp\Assembly-CSharp-cleaned.csproj.user` sets `ReferencePath=C:\Projects\KSP-UE5\decompiled\KSP-dev\`. This is what the guide's VS "Reference Paths" step writes. The build log confirms "Chosen KSPBT_GameRoot: ...\KSP-dev\ in reference path" (not Steam).
- Checked on the csc command line: Assembly-CSharp.dll is **not** referenced, every reference comes from KSP-dev\Managed, `/langversion:7.3 /debug:portable`, and there are no CS0436 type-conflict warnings. No extra Managed DLLs were needed. I18N, Mono.Posix, TDx, Timeline and RenderPipelines are not referenced by Assembly-CSharp.
- Build iterations, all Debug:
  1. First build: 22 errors (20x CS0683, 1x CS0542, 1x CS0535), exactly the guide's first-pass list.
  2. After the first-pass fixes: 1 error (CS0019, bool != int).
  3. After that fix: **0 errors**. A full Rebuild also gives 0 errors.
- **Build output:** `Assembly-CSharp.dll` **9,866,240 bytes**, `Assembly-CSharp.pdb` **4,849,024 bytes** (portable PDB, because KSPBuildTools forces `DebugType=portable`).
- **1,454 warnings:** 1162 CS0649, 219 CS0169, 29 CS0219, 20 CS0618, 15 CS0414, 5 CS0252, 3 CS1522, 1 CS1717. All are benign: unassigned or unused fields, obsolete APIs.
- Sanity check against the input DLL (type counts from ilspycmd -l):
  - classes 4560 = 4560, interfaces 92 = 92, delegates 186 = 186, enums 577 = 577.
  - structs 182 vs 179: the 3 extra are compiler-generated `<PrivateImplementationDetails>` array-init types.
- The 20 deleted accessor stubs were checked in the built IL: Roslyn regenerated `.override` bridge methods for them (IKSPCamera.get_enabled, ICommNetControlSource.get_name, ICanvasElement.get_transform, ITooltipController.*). The restored `IDesignConcern.add_testResultChanged` override is present.

### 4. Load KSP-dev with the custom assembly - DONE (verified)
- Moved `Assembly-CSharp.dll` (stock) and `Assembly-CSharp-cleaned.dll` from KSP-dev\Managed to `C:\Projects\KSP-UE5\decompiled\backup\`.
- Symlinks were created with **unelevated `mklink`**. This works because Developer Mode is on (AllowDevelopmentWithoutDevLicense=1); no copy fallback was needed. Links:
  - `KSP-dev\KSP_x64_Data\Managed\Assembly-CSharp.dll` -> `KSP-Decompiled\GameData\Assembly-CSharp-cleaned\Assembly-CSharp.dll`
  - `KSP-dev\KSP_x64_Data\Managed\Assembly-CSharp.pdb` -> `KSP-Decompiled\GameData\Assembly-CSharp-cleaned\Assembly-CSharp.pdb`
- A Rebuild after symlinking still has 0 errors, KSPBuildTools still finds KSP-dev, and the build does not reference itself.
- Launch command: `KSP_x64.exe -popupwindow -screen-fullscreen 0 -screen-width 1280 -screen-height 720 -logFile logs\ksp-<tag>-player.log`. Then wait for the main menu, let it settle 15 s, and kill (`tools\scripts\launch_ksp.ps1`).

| Run | Main menu ("Scene Change : From LOADING to MAINMENU") | Exception / MissingMethod / TypeLoad / MissingField / BadImageFormat / ReflectionTypeLoad / "Crash!!!" | ERR/EXC lines | WRN lines |
|---|---|---|---|---|
| Baseline, **stock** DLL (first run, builds part DB and drag cubes) | 39 s | 0 / 0 / 0 / 0 / 0 / 0 / 0 | 0 | 48 |
| **Custom** DLL | 21 s | 0 / 0 / 0 / 0 / 0 / 0 / 0 | 0 | 48 |

- Both logs show `Stock assembly: Assembly-CSharp v0.0.0.0` and the `[AddonLoader]: Instantiating addon 'KSPSteamController' ...` / `'ContractDefs' from assembly 'KSP'` lines. Mono path[0] is KSP-dev\Managed.
- The normalized Player.log diff shows **no lines unique to the custom run** apart from the command-line echo. The only baseline-only lines are first-run work (part DB, drag cubes, keyboard layout detection).
- The custom run reaches the same final main-menu state: `[ApplicationLauncher] ... [MessageSystem] OnAppInitialized`. The process was still responding when killed (about 2.6 GB working set).
- The custom run's KSP.log copy ends shortly after the scene change. That is because KSP buffers KSP.log and the process was killed; Player.log has the full sequence.
- **Proof the custom DLL is what got loaded:** I scanned the running KSP process's memory for module MVIDs after the main menu came up.
  - Custom build MVID 0daddb13-... : **found 1 time**.
  - Stock MVID 4b449f28-... : **0**.
  - de4dot-cleaned MVID 745e2a8f-... : **0**.
  - Script: `tools\scripts\scan_mvid.ps1`.
- In total, 4 full launches with the custom DLL all reached the main menu in about 21 s (the log run, a file-lock test, and two MVID scans). All processes were killed; no KSP is running.
- Not tested: gameplay beyond the main menu (scene changes into the editor or flight, loading a save), attaching the Unity debugger, and PDB line info in stack traces (there were no exceptions to produce any).

### 5. Stretch - DONE
These were decompiled read-only with the same settings into `KSP-Decompiled\<Name>\`. They are not in the .sln and were not built. de4dot -d found no known obfuscator in any of them, so no deobfuscation was needed.

| Assembly | Files | Lines | Notes |
|---|---|---|---|
| Assembly-CSharp-firstpass | 413 | 33,052 | Mostly third-party: Steamworks.NET (364 files), Vectrosity (20), Lingoona (2 + LingoonaGrammarFormatter), SpaceNavigator, OBJ/DAE loaders, ProgMesh, SmoothFollow2 (Unity Standard Assets), and a few small global-namespace files of uncertain origin |
| KSPAssets | 11 | 2,502 | Squad |
| KSPAssets.XmlSerializers | 11 | 1,769 | sgen-generated from KSPAssets |
| KSPTrackIR | 3 | 481 | Squad |
| KSPSteamCtrlr (`GameData\Squad\Plugins`) | 7 | 481 | Squad (extra, not in the original request) |

**Managed DLL classification** (KSP_x64_Data\Managed):
- **KSP/Squad-authored:**
  - Assembly-CSharp. It also contains embedded third-party source: TMPro (121 files), Smooth.* (104), VehiclePhysics + EdyCommonTools (Edy's Vehicle Physics, 143), LibNoise (45), SoftMasking (22), UniLinq (13), Highlighting, UnityEngine.UI.BoxSlider, and the Crypto Obfuscator runtime class `A._0003`.
  - KSPAssets, KSPAssets.XmlSerializers, KSPTrackIR.
  - Outside Managed: GameData\Squad\Plugins\KSPSteamCtrlr.dll.
- **Unity:** UnityEngine.dll, UnityEngine.*Module.dll, UnityEngine.UI, Unity.Timeline, Unity.RenderPipelines.* (Core.Runtime, Core.ShaderLibrary, ShaderGraph.ShaderGraphLibrary), Unity.Analytics.DataPrivacy / StandardEvents / Tracker.
- **Mono BCL** (Unity's Mono): mscorlib, System, System.Core, System.Xml, System.Configuration, System.Security, Mono.Security, Mono.Posix, I18N, I18N.CJK/MidEast/Other/Rare/West.
- **Other third-party:** Mono.Cecil 0.9.6, Ionic.Zip (DotNetZip 1.9.1.8), TDx.TDxInput (3Dconnexion interop). Assembly-CSharp-firstpass is mostly third-party (see above).
- **Native, not .NET:** UnityPlayer.dll, KSP_x64_Data\Plugins\x86_64\lingoona_grammar_kerbal.dll, steam_api64.dll.
- ~~**Not decompiled:** the KSP launcher's `Launcher_Data\Managed\Assembly-CSharp.dll`.~~ Superseded: now decompiled, together with every other managed DLL (see **Full coverage**).

## FIXES (full list; also in `KSP-Decompiled\FIXES.md`)
Line numbers refer to the raw decompilation (commit 9ca6e64).

| # | File | Line | Error | Change |
|---|---|---|---|---|
| 1 | FlightCamera.cs | 1184 | CS0683 'FlightCamera.get_enabled()' ... cannot implement 'IKSPCamera.enabled.get' because it is an accessor | Deleted `[SpecialName] bool IKSPCamera.get_enabled() { return base.enabled; }` |
| 2 | KSP/UI/PinnableTooltipController.cs | 62 | CS0683 get_name() / ITooltipController.name.get | Deleted `[SpecialName] string ITooltipController.get_name() { return base.name; }` |
| 3 | KSP/UI/PinnableTooltipController.cs | 68 | CS0683 set_name(string) / ITooltipController.name.set | Deleted `[SpecialName] void ITooltipController.set_name(string value) { base.name = value; }` |
| 4 | KSP/UI/TooltipController.cs | 116 | CS0683 get_name() / ITooltipController.name.get | Deleted stub (as #2) |
| 5 | KSP/UI/TooltipController.cs | 122 | CS0683 set_name(string) / ITooltipController.name.set | Deleted stub (as #3) |
| 6 | KSP/UI/UIRadioButton.cs | 296 | CS0683 get_name() / IRadioButton.name.get | Deleted `[SpecialName] string IRadioButton.get_name() { return base.name; }` |
| 7 | KSP/UI/UIRadioButton.cs | 302 | CS0683 set_name(string) / IRadioButton.name.set | Deleted `[SpecialName] void IRadioButton.set_name(string value) { base.name = value; }` |
| 8 | KSP/UI/UIRadioButtonColorChanger.cs | 162 | CS0683 get_name() / IRadioButton.name.get | Deleted stub (as #6) |
| 9 | KSP/UI/UIRadioButtonColorChanger.cs | 168 | CS0683 set_name(string) / IRadioButton.name.set | Deleted stub (as #7) |
| 10 | KerbalSeat.cs | 315 | CS0683 get_name() / ICommNetControlSource.name.get | Deleted `[SpecialName] string ICommNetControlSource.get_name() { return base.name; }` |
| 11 | ModuleCommand.cs | 710 | CS0683 get_name() / ICommNetControlSource.name.get | Deleted stub (as #10) |
| 12 | ModuleGroundExpControl.cs | 406 | CS0683 get_name() / ICommNetControlSource.name.get | Deleted stub (as #10) |
| 13 | PlanetariumCamera.cs | 587 | CS0683 get_enabled() / IKSPCamera.enabled.get | Deleted stub (as #1) |
| 14 | SPHCamera.cs | 317 | CS0683 get_enabled() / IKSPCamera.enabled.get | Deleted stub (as #1) |
| 15 | SpaceCenterBuilding.cs | 913 | CS0683 get_name() / ITooltipController.name.get | Deleted stub (as #2) |
| 16 | SpaceCenterBuilding.cs | 919 | CS0683 set_name(string) / ITooltipController.name.set | Deleted stub (as #3) |
| 17 | SpaceCenterCamera2.cs | 442 | CS0683 get_enabled() / IKSPCamera.enabled.get | Deleted stub (as #1) |
| 18 | TMPro/TMP_InputField.cs | 3188 | CS0683 get_transform() / ICanvasElement.transform.get | Deleted `[SpecialName] Transform ICanvasElement.get_transform() { return base.transform; }` |
| 19 | UnityEngine/UI/BoxSlider.cs | 411 | CS0683 get_transform() / ICanvasElement.transform.get | Deleted stub (as #18) |
| 20 | VABCamera.cs | 290 | CS0683 get_enabled() / IKSPCamera.enabled.get | Deleted stub (as #1) |
| 21 | A/-.cs | 24 | CS0542 '_0003': member names cannot be the same as their enclosing type | Deleted the empty `internal static void _0003() { }` in obfuscator class `A._0003`. It has no callers: no C# reference, and `<Module>` has no .cctor in either the original or the cleaned DLL. |
| 22 | PreFlightTests/DesignConcernBase.cs | 5 (decl. 13) | CS0535 'DesignConcernBase' does not implement interface member 'IDesignConcern.testResultChanged' | `private event Callback<bool> _0003 {add;remove}` -> `event Callback<bool> IDesignConcern.testResultChanged {add;remove}`. This restores the explicit implementation the obfuscator had renamed; the accessor bodies are unchanged. |
| 23 | Expansions/Missions/Editor/MissionEditorLogic.cs | 1856 | CS0019 Operator '!=' cannot be applied to operands of type 'bool' and 'int' | `if (num3 != 0)` -> `if (num3 != false)` |

- Fixes 1-20 were applied by `tools\scripts\fix_accessor_stubs.py` (guide 3.3). Each deleted body was asserted to be a single `return base.X;` or `base.X = value;`.
- **Not encountered with ILSpy 8.2:** `Cannot implicitly convert type 'T' to 'Smooth.Algebraics.Option<T>'` (the guide says ILSpy 9.0 preview only) and `Cannot implicitly convert type 'int' to 'char'` (the gist comment says ILSpy 9.1).
- **Totals:** 23 source fixes in 18 files, plus the project-file changes listed above. No functionality was deleted beyond what the guide prescribes; every deleted member is either regenerated by the compiler or provably unused.

## Where everything lives (root `C:\Projects\KSP-UE5\decompiled\`)

| Path | Contents |
|---|---|
| `KSP-dev\` | Dev copy of KSP. `KSP_x64_Data\Managed\Assembly-CSharp.dll/.pdb` are **symlinks** to the build output. |
| `backup\` | Stock `Assembly-CSharp.dll` (sha256 d9e42483...), de4dot `Assembly-CSharp-cleaned.dll` |
| `KSP-Decompiled\` | Git repo |
| `KSP-Decompiled\Assembly-CSharp-cleaned.sln` | Solution |
| `KSP-Decompiled\FIXES.md` | Fix log |
| `KSP-Decompiled\KSPBuildTools\` | Submodule |
| `KSP-Decompiled\Assembly-CSharp\` | Decompiled project: `Assembly-CSharp-cleaned.csproj`, plus `.csproj.user` (not committed) |
| `KSP-Decompiled\GameData\Assembly-CSharp-cleaned\` | Built `Assembly-CSharp.dll` + `.pdb`; gitignored |
| `KSP-Decompiled\{Assembly-CSharp-firstpass, KSPAssets, KSPAssets.XmlSerializers, KSPTrackIR, KSPSteamCtrlr}\` | Stretch decompilations |
| `KSP-Decompiled\{Unity, Mono, ThirdParty, Launcher}\<Assembly>\` | Full-coverage read-only decompilations (174 assemblies) |
| `KSP-Decompiled\INDEX.md` | Index of all 188 .dll/.exe files in the install + where-to-find guide |
| `tools\de4dot\net48\` | de4dot (kant2002 v3.2.0) |
| `tools\ilspycmd\pkg\` | ilspycmd 8.2.0.7535 |
| `tools\IlspyExport\` | Headless legacy-csproj exporter |
| `tools\dotnet-sdk\` | Portable .NET 8.0.425 SDK |
| `tools\scripts\` | `edit_csproj.py`, `fix_accessor_stubs.py`, `build.sh`, `launch_ksp.ps1`, `scan_mvid.ps1`, `decompile_all.ps1`, `make_index.py` |
| `tools\scratch\ilspycmd-crosscheck\` | Cross-check output; can be deleted |
| `logs\` | robocopy, de4dot, ILSpy export, `build-01..05` (`.log` / `.errors.log` / `.warnings.log`), KSP launch logs `ksp-baseline-stock-*`, `ksp-custom-dll-*` |

Rebuild: `tools\scripts\build.sh <tag> [Build|Rebuild]` (MSBuild on the .sln, Debug). A build writes straight into the files KSP-dev loads, through the symlinks.
Restore stock KSP-dev: delete the two symlinks in `KSP-dev\KSP_x64_Data\Managed\` and copy `backup\Assembly-CSharp.dll` back.

## Deviations from the guide
1. **de4dot:** I used the kant2002 fork's prebuilt v3.2.0 (net48) instead of building de4dot/de4dot in VS. The guide mentions this fork as an untested alternative.
2. **ILSpy:** no GUI. I used the identical engine (ICSharpCode.Decompiler 8.2.0.7535) through a small console tool, with the GUI's three choices set: C# 7.3, SDK-style off, nested directories on. ilspycmd 8.2 cannot turn off SDK-style projects. Its .cs output matched apart from `using` order.
3. **KSPBuildTools has changed since the guide** (now 1.1.1):
   - Import line from the current docs: `$(MSBuildThisFileDirectory)../KSPBuildTools/KSPCommon.targets` instead of `$(SolutionDir)KSPBuildTools\KSPCommon.targets`. Both resolve to the same file, and the new one also works without a SolutionDir.
   - `$(ManagedPath)` no longer exists, so I used `$(KSPBT_ManagedPath)`.
   - The Remove item uses `/` to match KSPBuildTools' item spec exactly.
   - KSPBuildTools now references System.Core.dll and firstpass itself, so the guide's `System.Core` and `Assembly-CSharp-firstpass` entries are duplicates. I kept them as the guide shows; they caused no warnings.
   - KSPBuildTools forces `DebugType=portable`, so the PDB is portable rather than "full".
4. **csproj edits:** OutputType Library and LangVersion 7.3 were already present in the ILSpy 8.2 legacy output. The guide implies OutputType started as Exe.
5. **Hand-written / CLI equivalents of VS steps:** the .sln (instead of VS "Save All"), the `.csproj.user` ReferencePath (instead of the "Reference Paths" property page), and git via the CLI with a `.gitignore` (bin, obj, .vs, *.user, GameData).
6. **Build and launch:** built with MSBuild CLI instead of VS. KSP_x64.exe was launched directly with window/log arguments instead of F5. Unity debugger attach was not tried.
7. **DesignConcernBase fix:** the guide only shows this as screenshots, which I could not see. I restored the explicit interface event implementation, which is the only semantics-preserving fix for that error.
8. **Symlinks:** created unelevated (Developer Mode) rather than from an Administrator CMD.
9. **KSPCommunityFixes `kspcf.cfg`:** not applicable, because KSPCF and ModuleManager are not installed in KSP-dev (GameData contains only Squad).
10. **KSPBuildTools side effect:** it also copies the guide-referenced DLLs (KSPAssets, Ionic.Zip, Mono.Cecil, ...) into `GameData\Assembly-CSharp-cleaned\`, because the guide's block has no `<Private>False</Private>`. This is harmless; only the .dll and .pdb are symlinked.
11. **Log names:** Unity 2019.4 writes Player.log, not `KSP_x64_Data\output_log.txt`. I redirected it with `-logFile` into `logs\` so it does not mix with the Steam install's LocalLow Player.log.

## Not done / limitations (verified vs assumed)
- **Verified:**
  - de4dot output.
  - Decompilation counts and the ilspycmd equivalence.
  - 0-error build and the reference list.
  - The regenerated bridge stubs in the IL.
  - The main menu loads with the custom DLL and no exceptions.
  - The custom DLL is the one in memory (MVID scan).
- **Not verified:**
  - Gameplay beyond the main menu (VAB/SPH, flight, saves, Missions editor).
  - Debugger attach and PDB line mapping at runtime.
  - Full semantic equivalence of decompiled method bodies. The decompiler can still introduce subtle behavioural differences in untested paths, for example goto-heavy code like `MissionEditorLogic.Update`.
- The stretch decompilations are read-only and were never compiled.
- ~~The launcher's Assembly-CSharp was not decompiled.~~ Done (Full coverage).

## Post-rename (2026-10-08, after `decompile` -> `decompiled`)

### Old-path references - DONE (verified)
- `grep` for `KSP-UE5\decompile` (not followed by `d`) over the whole tree, excluding `logs\` and `.git`, initially found one live reference: **`tools\scripts\launch_ksp.ps1` still had `$root = 'C:\Projects\KSP-UE5\decompile'`** (the launch script would have failed). Fixed.
- The other hits were not live config: `KSP-dev\KSP.log` (runtime log, since overwritten by the launches below) and `tools\IlspyExport\obj\*` (NuGet/MSBuild intermediates, regenerated when IlspyExport was rebuilt). A final grep finds **no** old-path reference.
- The rename patch had converted `KSP-Decompiled\FIXES.md` from LF to CRLF. I restored LF, so the committed diff is the single path line. Committed with the stretch csproj HintPath updates as `90fbdc02`.

### Symlinks - DONE (verified)
- `cmd.exe /c mklink <link> <target>`, run from PowerShell, unelevated (Developer Mode). Both succeeded on the first try, so no copy fallback was needed.
- `dir` shows:
  - `<SYMLINK> Assembly-CSharp.dll [C:\Projects\KSP-UE5\decompiled\KSP-Decompiled\GameData\Assembly-CSharp-cleaned\Assembly-CSharp.dll]`
  - `<SYMLINK> Assembly-CSharp.pdb [...\Assembly-CSharp-cleaned\Assembly-CSharp.pdb]`
- `Get-Item` reports LinkType=SymbolicLink. The SHA-256 read through each link equals that of its target.

### Finding: build-06 (the post-rename build) compiled against the **Steam** install
- `logs\build-06-after-rename.log` says `Chosen KSPBT_GameRoot: C:\Program Files (x86)\Steam\steamapps\common\Kerbal Space Program in steam`, not KSP-dev.
- Cause: KSPBuildTools accepts a game-root candidate only if `<root>/KSP_x64_Data/Managed/Assembly-CSharp.dll` exists (`KSPCommon.targets`, `KSPBT_GameIdentifier`). The symlinks had been deleted during the rename, so both the solution-dir and the ReferencePath (KSP-dev) candidates failed, and it silently fell back to Steam.
- Steam was only **read**:
  - Every "Copying file" destination in build-06 is inside `KSP-Decompiled\`.
  - 0 files in the Steam install are newer than `logs\robocopy.log`.
  - Steam's `Assembly-CSharp.dll` sha256 is still `d9e42483...`, the same as `backup\`.
- The Steam and KSP-dev Managed DLLs are identical copies, so the build-06 output was equivalent. To restore the intended setup I rebuilt after recreating the symlinks:
  - `tools\scripts\build.sh 07-post-full-coverage Build`
  - Result: `Chosen KSPBT_GameRoot: C:\Projects\KSP-UE5\decompiled\KSP-dev\ in reference path`, **0 errors**, 1,454 warnings (same as before).
  - New DLL sha256 `b5962288...` (build-06's was `1a66d022...`).
- **Caveat for the future:** whenever `KSP-dev\KSP_x64_Data\Managed\Assembly-CSharp.dll` is missing or dangling, builds silently use the Steam install as their reference source. Optional hardening, not applied: set `<KSPBT_GameRoot>` (or the `KSP_ROOT` env var) explicitly in `Assembly-CSharp-cleaned.csproj.user`.

### Launch sanity check - DONE (verified, twice)
Both runs used `tools\scripts\launch_ksp.ps1` (main menu, settle 15 s, kill).

| Run (tag) | DLL loaded via symlink | Main menu | Exception / MissingMethod / TypeLoad / MissingField / BadImageFormat / ReflectionTypeLoad / Crash!!! | ERR/EXC | WRN | OnAppInitialized |
|---|---|---|---|---|---|---|
| `post-rename` | build-06 (`1a66d022...`) | 24 s | 0 / 0 / 0 / 0 / 0 / 0 / 0 (KSP.log and Player.log) | 0 | 48 | yes |
| `post-rename-build07` | build-07 (`b5962288...`, the current one) | 30 s | 0 / 0 / 0 / 0 / 0 / 0 / 0 (KSP.log and Player.log) | 0 | 48 | yes |

- Logs: `logs\ksp-post-rename-{KSP,player}.log`, `logs\ksp-post-rename-launch.txt`, and the same three files for `post-rename-build07`.
- Player.log `Mono path[0] = 'C:/Projects/KSP-UE5/decompiled/KSP-dev/KSP_x64_Data/Managed'`.
- 48 WRN is the same as the pre-rename custom-DLL run.
- Both processes were alive and responding at kill time (about 2.5-2.7 GB working set), and both were killed. No KSP process is running.
- Not re-done: the MVID memory scan. The loaded file is established by the symlink target hash and the Mono path, not by a process-memory scan.

## Full coverage (2026-10-08) - DONE (verified)

### Tooling
- **`tools\IlspyExport`** gained `--lang 7.3|latest`.
  - The default is still C# 7.3, so earlier behaviour is unchanged. Regression test: a KSPTrackIR re-export is identical to the committed one except the random `<ProjectGuid>`.
  - `latest` = `LanguageVersion.Latest`, which is C# 11.0 in ICSharpCode.Decompiler 8.2.0.7535.
  - Rebuilt with the portable SDK; `obj\` was deleted first, which cleared its stale old-path content.
- **`tools\scripts\decompile_all.ps1`** (reproducible driver). Run it as `powershell -NoProfile -ExecutionPolicy Bypass -File tools\scripts\decompile_all.ps1 [-Group Unity,Mono,ThirdParty,Launcher,Expansions] [-Force] [-TimeoutMin 20]`. What it does:
  - Enumerates and classifies the DLLs and checks each is managed.
  - Runs `de4dot -d` on the whole group, and `de4dot --dont-rename` first for any assembly with a known obfuscator (none were found).
  - Runs IlspyExport per assembly under a 20-minute wall-clock kill limit. Partial output is removed on timeout or failure.
  - Counts `.cs` files and lines (newline count, same as `wc -l`).
  - Writes `logs\decompile-all-<group>.log` (full exporter output) and `logs\decompile-all-<group>.csv` (status, exit code, seconds, peak working set, unresolved refs, files, lines, sha256, version).
  - With `-Inventory` it writes `logs\decompile-all-inventory.csv` instead: every .dll/.exe in KSP-dev with managed/native, origin, version, sha256, de4dot result and launcher-vs-game duplicates.
- **`tools\scripts\make_index.py`** generates `KSP-Decompiled\INDEX.md` from those CSVs plus live counts.
- Language choice: C# 7.3 for Squad and third-party assemblies (the same as the earlier decompilations); C# latest for Unity and Mono assemblies.

### Results (all verified from the logs/CSVs)

| Group (output dir) | Source | Assemblies | .cs files | Lines | Wall time | Notes |
|---|---|---|---|---|---|---|
| `Unity\` | game `Managed\` UnityEngine*, Unity.* | 68/68 | 2,575 | 250,554 | 48 s | largest CoreModule (2.9 s) |
| `Mono\` | game `Managed\` mscorlib, System, System.Core/Xml/Configuration/Security, Mono.Security, Mono.Posix, I18N (6) | 14/14 | 6,342 | 933,692 | 38 s | mscorlib 2,292 files / 310,085 lines in 10.8 s, peak about 200 MB |
| `ThirdParty\` | Ionic.Zip 1.9.1.8, Mono.Cecil 0.9.6.0, TDx.TDxInput 1.1.0.0 | 3/3 | 366 | 49,461 | 3 s | |
| `Launcher\` | every DLL in `Launcher_Data\Managed\` (flat) | 89/89 | 9,874 | 1,361,960 | 80 s | 60 are byte-identical to the game's copies |
| `Expansions\` | `GameData\SquadExpansion\**\Plugins`, extra `GameData\Squad\Plugins` DLLs | 0 | - | - | - | none exist (see below); no directory created |
| **New total** | | **174** | **19,157** | **2,595,667** | | |

- **Nothing skipped and nothing failed.** The slowest single assembly was 10.8 s, far below the 20-minute limit, and the peak working set per export was at most about 220 MB.
- **All references resolved inside the intended directory:** 410 / 332 / 33 / 5 resolution lines for Launcher / Unity / Mono / ThirdParty, and 0 point elsewhere (no GAC or Steam fallback).
- 0 `Error decompiling` markers in any output. Embedded resources were exported alongside the code: mscorlib collation `.bin`, I18N.CJK `.table`, System `.wav`, System.Drawing `.ico`, `.resx`, and the Ionic.Zip `.zip`.
- **Obfuscation:** `de4dot -d` was run on all 180 managed files. Only the game's stock Assembly-CSharp reports a known obfuscator (Crypto Obfuscator); all others report "Unknown Obfuscator", which means none was detected.
  - **The launcher's Assembly-CSharp is not obfuscated:** de4dot found nothing and the names are readable (`News`, `Settings`, `GraphicsProfiles`, `UpdateKSP`, ...). 23 files, 3,163 lines.
  - The launcher's Assembly-CSharp-firstpass is the EZ GUI / SpriteManager 2 UI library (124 files, 29,710 lines).
- **Inventory corrections to the task brief:**
  - `lingoona_grammar_kerbal.dll` is **not** a launcher managed DLL. It is native, in `KSP_x64_Data\Plugins\x86_64\`.
  - `Launcher_Data\Managed\` also contains System.Data, System.Drawing, System.Net.Http, System.Numerics, System.Runtime.Serialization, System.ServiceModel.Internals, System.Transactions, System.Xml.Linq, netstandard, Unity.TextMeshPro and others. All were decompiled.
- **No DLC in the install:** `GameData\SquadExpansion\` exists neither in KSP-dev nor in the Steam install (checked read-only). `GameData\Squad\Plugins\` contains only KSPSteamCtrlr.dll, which was already decompiled.
  - The Making History / Breaking Ground code is in `Assembly-CSharp\Expansions\{Missions,Serenity}\`.
  - Assumed, not verified: the DLC packages themselves ship no extra DLLs.
- **Native (listed in INDEX, not decompilable):** KSP_x64.exe, Launcher.exe, UnityPlayer.dll, UnityCrashHandler64.exe, MonoBleedingEdge\EmbedRuntime\{mono-2.0-bdwgc,MonoPosixHelper}.dll, KSP_x64_Data\Plugins\x86_64\{steam_api64,lingoona_grammar_kerbal}.dll. Root `Plugins\` is empty.

### Index - DONE
- `KSP-Decompiled\INDEX.md` has a table of all 188 .dll/.exe files: 180 managed, 8 native. Columns: assembly, origin, version, obfuscated, decompiled path, .cs files, lines, buildable, notes.
- It also has a "where to find things" section with paths and line counts. Examples: Part.cs 9,023, Vessel.cs 4,664, EditorLogic.cs 5,490, ConfigNode.cs 4,344, PQS.cs 3,168, Orbit.cs 1,854. It covers 66 PQSMod*.cs files (6,400 lines), CommNet, Contracts, FinePrint, Expansions, VehiclePhysics, LibNoise, TMPro, Smooth, SoftMasking, UniLinq, EdyCommonTools and Highlighting.
- Caveat in the INDEX: the Unity assemblies are mostly `InternalCall` bindings (2,741 InternalCall attributes in CoreModule alone), so they show the API surface, not engine behaviour.

### Totals (whole install)
- **180 managed assemblies, all decompiled: 22,901 .cs files, 3,261,699 lines.**
  - By origin: Squad 8 (3,891 files / 698,905 lines), third-party 4 (435 / 61,358), Mono 37 (13,565 / 1,986,490), Unity 131 (5,010 / 514,946).
  - Squad counts the game's Assembly-CSharp as built (with fixes, 627,747 lines) and includes the launcher's two Assembly-CSharp* assemblies.
- **8 native binaries.**
- **Buildable:** only `Assembly-CSharp\`.

### Git (local only, never pushed)
- Commits added this session:
  - `90fbdc02` path updates after the rename
  - `798e6655` Unity/Mono/ThirdParty/Launcher decompilations (19,368 files incl. csproj and resources)
  - `1fe33b73` INDEX.md
- `.gitignore` already covered `bin/`, `obj/`, `.vs/`, `*.user`, `GameData/`. None of the new trees contains a bin/obj-named directory, and `git status --ignored` shows 0 ignored files in them.
- **Repo size after `git gc`:**
  - Object store 15 MB: one pack, 14.13 MiB, 17,612 objects. Identical launcher/game blobs are stored once.
  - `.git` total 37 MB, including the 20 MB KSPBuildTools submodule clone in `.git\modules`.
  - 23,124 tracked files, 92.1 MB uncompressed.
