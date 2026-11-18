# WP-UE-07 progress (sim subsystem, frame loop, placement)

Branch `wpue07` from main `bd2364c3`. Done 2026-11-18, inline in the orchestrating session. Results and numbers: `STATUS.md`
"Sim subsystem, frame loop, placement"; core side: `core/STATUS.md` "WP-UE-07, core side".

## Steps

- [x] Core rebuilt in the main checkout (the old `core/build` dated from 2026-10-11 and had no link manifest).
- [x] Core: `kspcore_part_collider_proxies` (additive, `kspcore_frame.h`), checked in `capi_flight_slice.c`; hash unchanged `2c4ae66445a41c81`.
- [x] Bridge on ABI v2; `ue5/Stub` and `Scripts/build_stub.ps1` removed (the facade cannot compile against the v1 stub header).
- [x] Facade `KspSim.h`: `FKspAxis`, `FKspGame`, `FKspSim`.
- [x] Module `Source/KspShell`: `UKspSimSubsystem`, `FKspRenderAnchor`.
- [x] Tests `KspUe5.Sim.AxisConversion`, `KspUe5.Sim.RenderAnchor`, `KspUe5.Slice.HeadlessParity`: pass at the first run, 8 / 8 in the suite.
- [x] Debug view: `Scripts/fly.ps1`, screenshot `slice/shots/wpue07_pad.png`.
- [x] Development Game target builds, 0 warnings.

## Things that cost time (for the next shell package)

- A module without `Public/` needs `PublicIncludePaths.Add(ModuleDirectory)` for other modules to include its headers.
- `GConfig` is Unreal's global ini cache: do not name a file-scope variable that.
- A tick function runs once per `GFrameCounter` value: a test that ticks a world in a loop must advance it (`FKspTestWorld`).
- `FTestWorldWrapper` of the engine creates a game instance and with it the project's default game mode (the scale spike's): not usable here.
- Files under `ue5/` are CRLF in the working tree. `sed -i` in Git Bash rewrites them as LF, and a Python heredoc run through the Bash tool
  loses one level of backslashes: edit CRLF files and Windows paths with the Edit tool.
- Only one editor / game instance at a time: the automation run and `fly.ps1` cannot overlap.
- `decompiled/KSP-dev` is not readable from the session's shell (deny rule); the game process reads it without trouble.
