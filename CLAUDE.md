# KSP-UE5

Private, local-only project. Nothing here is ever published.

## Decompiled KSP reference (use it)

`decompiled/` holds a full decompilation of Kerbal Space Program 1.12.5 (build 03190). Treat it as the authoritative behavioural reference for anything about how KSP works. Read the real code there instead of guessing or searching the web.

- `decompiled/KSP-Decompiled/INDEX.md` — table of every assembly and where the core gameplay classes live. Start here.
- `decompiled/KSP-Decompiled/Assembly-CSharp/` — the game itself (deobfuscated with de4dot, decompiled with ILSpy 8.2, C# 7.3). Root-level files are the core classes (`Part.cs`, `Vessel.cs`, `Orbit.cs`, `FlightIntegrator.cs`, `PQS.cs`, `Krakensbane.cs`, `FloatingOrigin.cs`, `ConfigNode.cs` ...). Namespaced code is in nested folders (`KSP/`, `Expansions/`, `CommNet/`, `Contracts/`, `FinePrint/` ...). Embedded third-party code: `TMPro/`, `Smooth/`, `VehiclePhysics/`, `EdyCommonTools/`, `LibNoise/`, `SoftMasking/`, `UniLinq/`.
- `decompiled/KSP-Decompiled/Assembly-CSharp-firstpass/`, `KSPAssets/`, `KSPTrackIR/`, `KSPSteamCtrlr/`, `Unity/`, `Mono/`, `ThirdParty/`, `Launcher/` — the other shipped assemblies, read-only.
- `decompiled/KSP-Decompiled/FIXES.md` — every edit made to get the main assembly compiling again.
- `decompiled/STATUS.md` — full procedure log, tool versions, verification results.

The main assembly is buildable: `bash decompiled/tools/scripts/build.sh <tag>` runs MSBuild and writes logs to `decompiled/logs/`. The built DLL is symlinked into the dev copy at `decompiled/KSP-dev/` (a non-Steam copy; never modify the Steam install). `decompiled/tools/scripts/launch_ksp.ps1` launches the dev copy for a load test.

## Original game assets (use them)

This private build uses KSP's own assets, not placeholders or re-creations: part meshes (.mu via the PartTools format in the decompile), textures (.dds/.png/.mbm), part and resource .cfg files, planet heightmaps, biome maps, colour maps, scaled-space textures, sounds, fonts, UI sprites, and the Unity asset bundles (extract with AssetStudio or UnityPy; the harness `tools/extract_assets.py` already does this for prefab values). Source of truth is the dev copy at `decompiled/KSP-dev/` (GameData/, KSP_x64_Data/). Load from there at runtime or import from there at build time; never hand-author a substitute for something the game ships. Record each asset's origin path next to where it is consumed. Nothing derived from these assets is ever distributed.

Rules:
- Cite decompiled files by relative path when explaining KSP behaviour.
- Do not copy decompiled code into any artefact intended for distribution. This project is private; keep it that way.
- Keep all decompilation work inside `decompiled/`.
- `decompiled/KSP-Decompiled/` must always have `main` checked out and clean, because every agent reads it as the spec. Profiling and capture instrumentation lives on branch `profiling`, checked out in the worktree `decompiled/KSP-Decompiled-profiling/`. Build there (`tools/scripts/build_opt.sh`) and copy the DLL over the symlink target; never switch branches in the main checkout.
- In-game captures: `harness/captures/` (batch 1: `out/`, batch 2: `out2/`; one-command reruns via `run_capture.sh`). For float32 code paths the captures, not the .NET harness goldens, are truth.

## Other files

- `FEASIBILITY.md` — the UE5 port feasibility assessment (2026-10-08).

## Agent roles

Fable orchestrates and reviews. Haiku does research and web lookups. Sonnet is the implementer for build, decompile and code tasks (Opus 5.5 was used through batch 3; escalate to Opus only after two failed gate attempts).
