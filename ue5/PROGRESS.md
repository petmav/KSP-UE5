# ue5/ PROGRESS (Opus implementer, session 2026-10-09)

Live log. Work from this file after any context compaction.

## Task (from Fable brief)
D1: UE 5.8 C++ project `KspUe5` + plugin `KspCoreBridge` (stub lib now, one-line switch to core\kspcore.lib RelWithDebInfo),
    git init + Unreal .gitignore, CLI builds (Dev Editor + Dev Game Win64) with timings, automation test run headless + timing.
D2: phase-0 scale spike: map ScaleSpike, 600 km cubesphere (ProceduralMesh, LODs), SkyAtmosphere planet-centred, sun,
    camera paths a) orbit 100 km b) descend to 10 m look at horizon c) 1 m above ground with 1 m cube; rebasing
    (SetNewWorldOrigin vs move-world, thresholds 10/100 km), per-shift cost; jitter proxy; Mun-distance actor (12,000 km);
    shift-every-tick for 10 s vs KSP 3.3 ms (PROFILE.md). Outputs: spike/RESULTS.md, spike/shots/, README, STATUS.
Rules: write only under ue5\. No subagents. One UE instance at a time. Low scalability, 1280x720.

## Environment facts
- UE 5.8.3 CL 58210709 at C:\Program Files\Epic Games\UE_5.8. Installed build has UnrealGame precompiled libs (game target OK).
- MSVC 14.44.35207, WinSDK 10.0.26100 + 22621. git 2.53. Python 3.12.10 with PIL 12.1.1 + numpy 2.4.2.
- Disk C: 316 GB free at start. 16 logical cores.
- core\ exists but only has PROGRESS.md (core agent mid-scaffold). Do not depend on it.

## Engine facts verified from 5.8 headers/source
- WORLD_MAX = UE_LARGE_WORLD_MAX = 8.796e12 cm (EngineDefines.h:41-53); bEnableLargeWorlds is DEPRECATED ("As of UE 5.1 all
  worlds are large", WorldSettings.h:1087). LWC cannot be turned off without an engine source rebuild (UE_USE_UE4_WORLD_MAX).
- UWorld::SetNewWorldOrigin(FIntVector) (World.cpp:8071): int32 cm => max +-21,474 km origin. Logs "WORLD TRANSLATION END ... took X ms".
  Shifts render scene (Scene->ApplyWorldOffset), levels, nav, ownerless components, line batchers.
- FPhysScene_Chaos::SupportsOriginShifting() returns false (PhysScene_Chaos.cpp:2262) => Chaos scene is NOT shifted by
  SetNewWorldOrigin. (Check how simulating bodies follow: UPrimitiveComponent::ApplyWorldOffset.)
- bEnableWorldOriginRebasing (WorldSettings.h:442) only drives *automatic* rebasing with World Composition. Manual call works.
- UBT VCToolChain.cs: CRT /MD (release CRT even for Debug editor unless bUseDebugCRT) ; /std:c++20 default; /GR- (no RTTI);
  exceptions off => _HAS_EXCEPTIONS=0 unless bEnableExceptions; Editor target /fp:precise, Game target default /fp:fast
  (BuildSettingsVersion V7). /permissive- and /Zc:__cplusplus.
- Templates: TP_Blank uses BuildSettingsVersion.V7, EngineIncludeOrderVersion.Unreal5_8.
- Engine materials usable from C++: /Engine/EngineDebugMaterials/VertexColorMaterial (DefaultLit, vertex colour) and
  /Engine/EngineDebugMaterials/VertexColorViewMode_ColorOnly (Unlit, vertex colour).

## Status
- [x] D1 project shell files (uproject, targets, Config, game module KspUe5, editor module KspUe5Editor w/ commandlet)
- [x] D1 stub lib (CMake, /MD, /Z7) + bridge plugin (one-line switch bUseRealCore in KspCoreBridge.Build.cs)
- [x] D1 git init + first commit (1229344)
- [x] D1 builds + timings (see timings.log): genproj 14.1 s; clean full editor 64.4 s; no-op 1.2 s; incremental (1 cpp)
      10.1 s; plugin-only 2.5 s; game Development 51.7 s; commandlet map 29.0 s. UBT limited to 3-4 parallel actions
      (only ~6 GB RAM free; 1.5 GB/action).
- [x] D1 automation tests: 3/3 pass. Headless run cold 41.8 s, warm 15.7 s; touch+build+test loop 27.2 s.
- [x] commit 8c15f5a
- [x] D2 spike code (compiles, runs end to end: 55 steps, ~210 s per full run incl. startup)
- [x] smoke1/2/3 + full + full2 (canonical) done. CsvProfiler cross-check agrees within 0.02 ms.
- [x] cold-DDC shader compile: 351.7 s to BeginPlay, 371.6 s to all shaders ready (warm: 11.0 s)
- [x] RESULTS.md, STATUS.md, README.md written; game target rebuilt clean (22.3 s); kspcore.lib change -> bridge relink 1.6 s
- [x] cook + stage + pak 322.9 s (549 pkgs, staged 972 MB); staged exe ran full spike (tag cooked): jitter identical,
      2.0 GB working set, 1.7 s to BeginPlay. A/B (quick, alternating twice): cooked ~= editor on render/GPU, ~20% faster on
      shift cost (4.7 vs 5.9 ms). Disk: ue5 7.8 GB after cook.
- [x] all docs updated; final commit pending
- [ ] D2 runs + RESULTS.md
- [ ] README / STATUS

## Timings (wall clock)
(fill in)

## Findings so far (full run, tag full)
- A (small primitive-local coords): image jitter 0.000 px static/co-moving, <=0.048 px f2f under 1 mm/frame strafe, in ALL
  variants incl. no rebasing at 700 km and at 12,000 km (Mun). Forced origin shift pop 0.000 px. => jitter-free with LWC alone.
- B (vertices relative to body centre): 7-11 px max dev, 10-19 px f2f; pop 8.6 px on shift. Rebasing does not fix it.
- Emulator (op-for-op DF path) validated vs GPU on B: 12.9 vs 10.5 px (orbit), 15.2 vs 13.4 (strafe None), 21.4 vs 19.4 (UE strafe).
- Float32 world (UE4/Unity) emulation: 13 px orbit, 16.5 px ground strafe, 207 px at Mun; with rebasing 0.2 px / 0.0003 px.
- Shift cost (10 s every tick, KSP-like 457 bodies + 1325 colliders): UE SetNewWorldOrigin 5.93 ms/shift (+8.9 ms game),
  MoveWorld 9.13 ms (+13.0). No physics: 1.19 / 1.60 ms. Bare: 0.23 / 0.39 ms. KSP: 3.3 ms script + ~2.25 ms PhysX.
- Chaos does not support origin shifting (SupportsOriginShifting()=false) -> per-body teleport; drift check 0.0000 cm.
- Frame times at 1280x720 low scalability: 2.9-4.8 ms (non-readback phases), GPU 1.7-2.5 ms. Memory ~2.5-3.2 GB working set.

## Decisions
- Spike runs uncooked via `UnrealEditor.exe KspUe5.uproject /Game/Maps/ScaleSpike -game` (no cook needed); game target built but
  running it needs a cook (optional, record if done).
- Map ScaleSpike is generated by a C++ commandlet (reproducible from code), content spawned at runtime by C++.
- Jitter: (1) numeric proxy = emulated float32 vs UE5 DF vs double camera-space position of a static vertex, per-frame delta;
  (2) image proxy = per-frame ReadPixels centroid of an unlit red 1 m cube that is static in camera space (or moving linearly).
- TAA/TSR, motion blur, auto exposure off for measurement (TAA jitter is intentional sub-pixel jitter and would pollute the proxy).

## Gotchas hit
- UnrealBuildTool.exe direct call fails (needs .NET 10 runtime, not installed system-wide). Build.bat uses the
  engine-bundled dotnet (Engine/Binaries/ThirdParty/DotNet/10.0). genproj.ps1 therefore calls Build.bat -projectfiles.
- UE front-face winding: cross(B-A, C-A) points AWAY from viewer (verified from KismetProceduralMeshLibrary GenerateBoxMesh).
- An axis-aligned test site hides float32 jitter (moving coordinate stays small). Site = normalize(1,1,1), orbit start
  theta 45 deg, Mun dir normalize(0.35,0.85,0.40), Mun motion tangential with X/Y components.
- Bash tool: heredocs > ~30 KB fail (ENAMETOOLONG); use the Write tool for big files.

## Scripts (ue5/Scripts)
genproj.ps1, build.ps1 [-Target -Module -Label], build_stub.ps1, create_map.ps1, test.ps1, spike.ps1 [-Tag -Variants -Quick -Stress].
All append wall-clock timings to ue5/timings.log.

## Next steps (for a future session)
- Switch KspCoreBridge to the real core once core/ ships include/kspcore/kspcore_c.h + build/RelWithDebInfo/kspcore.lib (/MD).
- Terrain: replace per-patch ProceduralMeshComponents (dynamic path, render-thread bound at ~1,800 patches) with a static or
  instanced patch path or RealtimeMesh; keep patch-centred primitive origins.
- Untested at scale: shadows/TSR, Niagara, planet rotation (rotating frame < 100 km), Chaos joints far from origin.
