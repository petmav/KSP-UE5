# Phase-0 scale spike: Kerbin-scale rendering in UE 5.8 (results)

Run date 2026-10-09. Engine UE 5.8.3 (launcher, CL 58210709). Laptop: Ryzen AI 7 350 (8c/16t), 24 GB RAM, Radeon 860M iGPU,
DX12 SM6. 1280x720 windowed, low scalability (`sg.* 0`, 100% screen percentage), no Lumen / VSM / Nanite / shadows,
**no TAA/TSR** (temporal AA jitters the projection by design and would pollute the jitter proxy), no motion blur or bloom, fixed exposure.
Uncooked `UnrealEditor.exe -game` (Development editor binaries). Canonical data: tag `full2` (`data/full2_summary.csv`,
`data/full2_frames.csv`, `data/full2_tables.md`, `shots/full2_*.png`, `logs/full2.log` not committed). The first full run (`full`)
reproduced every image metric to the third decimal: the programme is deterministic (fixed simulated time step per frame).

## Verdict

**Jitter-free at Kerbin scale: yes, with Large World Coordinates alone, as long as every primitive's vertices are stored
relative to an origin near them.** A 1 m cube built that way showed 0.000 px of image jitter co-moving with the camera in a
100 km orbit 700 km from the world origin, 0.000 px at 1 m above the surface, 0.000 px co-moving 20 km above the Mun at
12,000 km from the origin, and at most 0.048 px frame-to-frame (sub-pixel rasterisation noise) while strafing 1 mm per frame.
That held with **no origin rebasing at all**, and equally under every rebasing variant. A forced origin shift with the scene
static moved it by 0.000 px.

What does jitter, in every variant: geometry whose primitive origin is far from its vertices. The same cube stored relative
to the planet centre ("B") jittered 7 to 11 px (3.6 to 5.5 cm) and popped 8.7 px on an origin shift. Rebasing does not cure
it; only small primitive-local coordinates do. For the planet this means a cubesphere of patch primitives with patch-centred
origins, which is what KSP's PQS does and what this spike built.

**Rebasing approach:** both work in 5.8. `UWorld::SetNewWorldOrigin` (manual call, no World Composition needed) is the cheaper
and complete one: 0.23 ms per shift in the bare scene, 5.9 ms with a KSP-sized scene of 457 rigid bodies + 1,325 colliders
(+9.1 ms game thread per frame when done every tick; 4.7 ms and +7.5 ms frame time in the packaged Development build). Moving every actor ("MoveWorld") costs 0.39 ms bare and 9.2 ms
(+13.0 ms) KSP-sized. KSP's own per-tick shift is 3.3 ms of script plus about 1.2 to 2.3 ms of extra PhysX
(`analysis/physics/PROFILE.md` section 4), so **UE's per-shift cost does not beat KSP's; don't shift every tick.** Chaos does not
support origin shifting (`FPhysScene_Chaos::SupportsOriginShifting()` returns false), so each shift teleports every body one by
one; physics cost is 80% of the shift (1.2 ms without physics state vs 5.9 ms with). With LWC, rendering needs no rebasing,
so rebasing (if kept) is for float-only subsystems and should be rare: a 100 km threshold gave 12 shifts in the whole programme,
each 0.36 ms on average in the bare scene.

## Scene and method

- **Kerbin**: 600 km radius cubesphere (`Source/KspUe5/Spike/SpikePlanet.*`, `SpikeMath.*`), 6 faces, distance-based quadtree to
  level 14 (57 m leaf patches, 16x16 quads, ~3.6 m quads), skirts against cracks, one `ProceduralMeshComponent` per patch with
  its origin at the patch centre. 450 patches seen from orbit, ~1,700 near the ground. Vertex colours (ocean/land/ice),
  `/Engine/EngineDebugMaterials/VertexColorMaterial` (lit). UE units: radius 6e7 cm; `WORLD_MAX` = 8.8e12 cm in 5.8
  (`EngineDefines.h`; `bEnableLargeWorlds` is deprecated, "all worlds are large" since 5.1; LWC cannot be switched off without
  an engine source build).
- **Atmosphere**: `USkyAtmosphereComponent`, `PlanetCenterAtComponentTransform`, ground radius 600 km, height 70 km, Rayleigh
  scale height 5.6 km, Mie 0.84 km. Sun: directional light (10 lux, atmosphere sun light), 50 deg elevation over the test site.
  Real-time-capture sky light. Works at this radius (screenshots below).
- **Mun**: 200 km radius cubesphere (levels 2 to 5), unlit magenta, centre 12,000 km from Kerbin's centre.
- **Directions are generic on purpose.** The test site is at normalize(1,1,1) (latitude 35 deg), the orbit starts at 45 deg,
  the Mun is off-axis, and every motion has components on all axes. A site on an axis hides float32 loss, because the moving
  coordinate stays small; the first draft had exactly that flaw.
- **Camera paths** (`SpikeDirector.cpp`, fixed 1/60 s of simulated time per frame, independent of real frame time):
  - (a) `OrbitImg` / `OrbitTiming`: 100 km circular orbit at 2,246 m/s (Kerbin mu), 1,200 frames. `OrbitWarp`: the same orbit
    at 50x (1.87 km per frame), 300 frames.
  - (b) `Descent`: 100 km to 10 m over 600 frames (exponential), pitch easing to the horizon. `Horizon`: 10 m, 2 deg down,
    yaw sweep 0.1 deg/frame, 240 frames.
  - (c) `GroundStatic`: eye 1 m above the ground, two 1 m cubes resting 3 m ahead, 120 frames, with a forced origin shift
    at frame 60. `GroundStrafe`: the same, camera strafing 1 mm per frame, 300 frames.
  - Mun: `MunFromOrbit`: from the 100 km orbit, flying at 112 km/s (50x) with a fixed attitude on the Mun, 120 frames.
    `MunNear`: 20 km above the Mun's near side, moving 500 m/s, co-moving cubes, 240 frames.
- **Rebasing variants**, each running all paths: `None` (LWC only; origin fixed at Kerbin's centre, camera up to 700 km and
  11,780 km from the origin), `UE100k` / `UE10k` (`SetNewWorldOrigin` when the camera is more than 100 / 10 km from the
  origin), `MW100k` / `MW10k` (move every movable actor by the camera offset). Every shift is validated: Kerbin and the Mun
  must sit at their universe position minus the origin. The maximum error over the whole run was 0.0000 cm.
- **Image jitter proxy (the GPU output).** Each frame of the `OrbitImg`, `Ground*` and `Mun*` phases is read back through
  `UGameViewportClient::OnScreenshotCaptured`. The centroid of the red cube A, the yellow cube B and the magenta Mun (unlit,
  so exposure cannot move them) is computed inside a projected ROI. A and B are the same 1 m cube; A's vertices are relative
  to its own origin and B's to the body centre. Static and co-moving phases report deviation from the mean. Moving phases
  report the residual after a quadratic fit of the smooth motion. "f2f" is the worst frame-to-frame step of that residual,
  and "pop" is the step across the forced shift. At 3 m, 1 px is about 0.49 cm.
- **Numeric jitter proxy (emulation).** Every frame, static and co-moving reference vertices go through an op-for-op float
  emulation of UE 5.8's vertex path. That path (`Shaders/Private/DoubleFloat.ush` `DFTransformLocalToTranslatedWorld`,
  `DoubleFloatOperations.ush` `DFFastLocalSubtractDemote`, CPU split in `PrimitiveUniformShaderParametersBuilder.h`) is
  `((float(P) - float(C)) - float(C - float(C))) + float(P - float(P))`, then plus the float local position. The proxy also
  runs a UE4/Unity-style float32 world-space pipeline. Reported in screen pixels, on-screen points only. **Validated against
  the GPU** on cube B: worst frame-to-frame emulated vs measured 12.9 vs 10.5 px (orbit), 15.2 vs 13.4 px (ground strafe, no
  rebasing), 21.4 vs 19.4 px (strafe, UE100k), 3.2 vs 1.6 px (Mun). On cube A it predicts at most 0.0002 px and the GPU shows
  0.000 to 0.048 px.
- **Frame times** are the globals behind `stat unit` (`GGameThreadTime`, `GRenderThreadTime`, `GRHIThreadTime`,
  `RHIGetGPUFrameCycles`) plus the real frame delta. They were cross-checked with the engine's CSV profiler (`CsvProfile START/STOP`)
  on two steps and agree to 0.01 to 0.02 ms (`data/csvprofile/`):

  | step | source | frame | game | render | RHI | GPU |
  |---|---|---|---|---|---|---|
  | None/OrbitTiming | CsvProfiler | 3.09 | 2.05 | 3.02 | 1.44 | 1.83 |
  | None/OrbitTiming | director | 3.09 | 2.04 | 3.03 | 1.44 | 1.83 |
  | StressKsp/UEOrigin-every-tick | CsvProfiler | 13.87 | 13.86 | 4.17 | 1.73 | 1.92 |
  | StressKsp/UEOrigin-every-tick | director | 13.87 | 13.87 | 4.18 | 1.73 | 1.92 |

## Results

### Jitter at each path point (yes/no)

"Visible" means more than 0.5 px of image jitter. Values are the image proxy (worst frame-to-frame step, px) for A /
B. Where no image is captured, the emulated value for the planet's own patch vertices (G_patch) is given instead.

| path point | LWC only (`None`) | UE100k | UE10k | MW100k | MW10k |
|---|---|---|---|---|---|
| (a) 100 km orbit, cube co-moving | **no** (A 0.000; B 10.5) | no (A 0.000) | no | no | no |
| (a) orbit, planet surface vertices (emulated) | no (0.0000) | no | no | no | no |
| (a) orbit at 50x warp, planet vertices (emulated) | no (0.0000) | no | no | no | no |
| (b) descent 100 km to 10 m, planet vertices (emulated) | no (0.0001) | no | no | no | no |
| (b) 10 m, looking along the horizon (emulated) | no (0.0002) | no | no | no | no |
| (c) 1 m above ground, static, incl. forced shift | **no** (A 0.000, pop 0.000) | no (pop 0.000) | no | no | no |
| (c) 1 m, strafing 1 mm/frame | **no** (A 0.048) | no (A 0.048) | no | no | no |
| Mun from orbit at 12,000 km, camera at 112 km/s | **no** (Mun disc 0.112 max dev) | no | no | no | no |
| 20 km above the Mun, cube co-moving at 500 m/s | **no** (A 0.000; B 1.6) | no | no | no | no |

The failure modes, measured or emulated in the same frames:

| what | orbit (a) | ground strafe (c) | near Mun |
|---|---|---|---|
| B: vertices relative to the body centre, GPU image, worst f2f px | 10.5 | 13.4 (UE100k: 19.4) | 1.6 |
| G_planet: planet surface stored as one planet-centred mesh, emulated px | 0.0 (vertex far away) | 1.9 (UE10k 2.6) | n/a |
| UE4/Unity-style float32 world space, no rebasing, emulated px (cm) | 13.2 (5.7 cm) | 16.5 (5.6 cm) | **206.8 (71.6 cm)** |
| same float32 pipeline with the run's rebasing (10 km), emulated px | 0.21 | 0.0003 | 0.04 |

Reading: UE5's LWC rendering makes rebasing unnecessary *for rendering* at Kerbin and Mun scale. Without LWC (the float32 rows),
the classic failure is 5 to 6 cm at Kerbin's surface and 72 cm at the Mun, and rebasing fixes it, which is why KSP rebases. In UE 5.8 the
remaining failure mode is large primitive-local coordinates, which rebasing does not fix.

### Origin rebasing in 5.8: what works and what it costs

- `UWorld::SetNewWorldOrigin(FIntVector)` works when called manually. It does not need World Composition;
  `bEnableWorldOriginRebasing` only drives the automatic mode. The origin is `int32` cm, so it is limited to +-21,474 km from
  the start origin; the Mun at 12,000 km is fine, interplanetary positions are not. It shifts the render scene in bulk
  (`FScene::ApplyWorldOffset`), all level actors, navigation and line batchers, and logs two lines per shift.
- Chaos is **not** shifted in bulk (`SupportsOriginShifting()` is false). Each physics body gets a teleport through
  `USceneComponent::ApplyWorldOffset`, and this works: 457 spinning bodies kept their universe positions through 722
  consecutive shifts (drift 0.0000 cm). MoveWorld (`SetActorLocation(..., TeleportPhysics)` on every movable actor) also works,
  with the same drift of 0.
- Sparse shifts during the paths, bare scene (stopwatch around the shift call):

  | variant | shifts in programme | avg ms | max ms |
  |---|---|---|---|
  | UE100k | 12 | 0.36 | 0.59 |
  | UE10k | 87 | 0.31 | 0.67 |
  | MW100k | 12 | 0.49 | 1.09 |
  | MW10k | 87 | 0.47 | 1.07 |

- **Shift every tick for 10 s** (camera in a 100 km orbit at 2,246 m/s, so every tick moves 37 m), compared with KSP's 3.3 ms
  `FloatingOrigin.FixedUpdate` plus about 1.2 to 2.3 ms of extra PhysX per shifted tick on the 457-part L2-AS craft. "KSP-like"
  = 457 awake, simulating rigid bodies (vessel stand-in) + 1,325 hidden static box colliders (PQS/KSC collider stand-in,
  matching PROFILE.md's scene census). "No physics" = the same 1,782 actors with physics and collision disabled.

  | scene / mode | frames in 10 s | frame ms | game ms | game delta vs no-shift | shift ms avg (p50 / max) |
  |---|---|---|---|---|---|
  | bare / none | 3,426 | 2.92 | 1.95 | 0 | - |
  | bare / SetNewWorldOrigin | 3,333 | 3.00 | 2.27 | +0.33 | 0.233 (0.229 / 0.615) |
  | bare / MoveWorld | 3,173 | 3.15 | 2.89 | +0.94 | 0.395 (0.387 / 0.779) |
  | KSP-like / none | 2,081 | 4.81 | 4.81 | 0 | - |
  | KSP-like / SetNewWorldOrigin | 722 | 13.87 | 13.87 | **+9.06** | **5.95** (5.91 / 7.64) |
  | KSP-like / MoveWorld | 561 | 17.86 | 17.81 | +13.00 | 9.21 (9.17 / 10.93) |
  | KSP-like no physics / none | 3,409 | 2.93 | 1.98 | 0 | - |
  | KSP-like no physics / SetNewWorldOrigin | 2,935 | 3.41 | 3.40 | +1.42 | 1.208 (1.192 / 2.451) |
  | KSP-like no physics / MoveWorld | 2,022 | 4.95 | 4.95 | +2.97 | 1.576 (1.555 / 2.583) |

  The stopwatch catches about two thirds of the cost. The rest (about 3 ms per shift) lands later in the same game-thread frame,
  in deferred transform and physics updates, so the game-thread delta is the honest total: **about 9 ms per shift for UE's
  built-in rebasing versus about 5.5 ms for KSP**. Every-tick rebasing is not a win in UE. Rare rebasing is cheap.

### Mun at 12,000 km

Renders: a 397 px disc, which is the expected 21.3 px diameter for 200 km at 12,000 km with a 90 deg FOV at 1280 px
(`shots/full2_None_MunFromOrbit.png`). With LWC only (camera 700 km from the origin, the Mun 12,000 km away) and the camera
flying at 112 km/s, the disc centroid stays within 0.112 px of a smooth parallax fit over 120 frames. That is rasterisation
noise on a 22 px disc. Near the Mun (camera 11,780 km from the origin, no rebasing), cube A co-moving at 500 m/s shows 0.000 px.
Depth, culling and the sky atmosphere showed no problems at that distance.

### Frame times (stat unit numbers, ms, `None` variant; full per-step tables in `data/full2_tables.md`)

| phase | frame avg | frame p95 | game | render | GPU | patches |
|---|---|---|---|---|---|---|
| orbit 100 km (no readback) | 3.09 | 3.81 | 2.04 | 3.03 | 1.83 | 456 |
| orbit 50x warp | 3.41 | 4.27 | 2.60 | 3.04 | 1.90 | 480 |
| descent 100 km to 10 m | 4.38 | 6.73 | 3.27 | 3.98 | 2.40 | up to 1,803 |
| 10 m horizon | 3.57 | 4.37 | 2.51 | 3.52 | 1.89 | 1,803 |
| image phases (with per-frame GPU readback) | 11 to 12 | 12 to 13 | 3.0 | 8.3 to 9.7 | 1.0 to 2.1 | |

The iGPU is not the limit here: GPU time is 1.0 to 2.5 ms at 1280x720 low. The render thread is the largest cost because each of
the ~1,800 ProceduralMeshComponents takes the dynamic-mesh path every frame. A production terrain should use a static or instanced
patch path, or RealtimeMesh. Quadtree LOD rebuilds cost 369 ms in total over the 600-frame descent (worst frame 29 ms). The worst
spike was 141 ms in warm-up, building ~1,350 patches in one frame.

### Shader compile time

- First-ever launch of the project (`smoke1`): 38.7 s from process start to "warm-up done" (all shader jobs finished and 120 quiet
  frames). The launcher build ships a 1.67 GB engine DDC pak (`Engine/DerivedDataCache/Compressed.ddp`) with the engine's global
  and default-material shaders, so the only project-side compile was a 7.2 s first-frame hitch for the vertex-colour materials
  on the procedural-mesh vertex factory.
- Warm launches: 8.4 s to `BeginPlay`, 11.0 s to warm-up done.
- Truly cold (`-DDC=Cold`: an empty standalone Zen cache with no engine pak, so everything compiles on 8 local
  ShaderCompileWorkers; tag `coldddc`): **351.7 s to `BeginPlay`**, all blocking global and default-material shaders. Then
  19.2 s of warm-up for the project materials, including one 17.1 s first-frame hitch. **Total 371.6 s (6.2 min)** to a fully
  compiled scene, against 11.0 s warm. This is the worst case for a fresh machine or a wiped DDC. The log does not report shader
  counts.

### Memory

Process working set (uncooked, editor binaries in `-game`): 2.54 GB after warm-up, 3.26 GB maximum at a step end, peak 3.28 GB.
RHI texture memory (streaming + non-streaming): 118 MB. The KSP-like scene's 1,782 actors added about 30 MB.

### Cooked Development game (packaged)

`Scripts/cook.ps1` (`RunUAT BuildCookRun -cook -stage -pak -skipbuild`) took 322.9 s: 283 s of cook (549 packages; engine
assets the spike loads by path had to be added to `DirectoriesToAlwaysCook`), then pak and stage. The staged build is 972 MB.
The staged `KspUe5.exe` ran the full programme (tag `cooked`, `data/cooked_summary.csv`, `data/cooked_tables.md`): 1.7 s to
`BeginPlay`, 4.7 s to warm-up done, working set 2.0 GB (3.3 GB for editor `-game`). **Every image and numeric jitter metric is
identical to the editor run**, so the rendering path is the same.

The first cooked full run's frame times came out about 35% slower than the editor run. A controlled A/B (quick programme, cooked
and editor alternated twice back to back, `data/ab_*_summary.csv`) did not reproduce that, so it was machine load from other
work at the time. The A/B shows:

| quick A/B, ms | cooked run 1 / run 2 | editor run 1 / run 2 |
|---|---|---|
| orbit frame (render, GPU) | 3.57 (3.31, 1.88) / 3.17 (2.93, 1.85) | 3.71 (3.43, 1.84) / 3.08 (2.83, 1.76) |
| KSP-like scene, no shift: frame | 3.95 / 3.83 | 4.87 / 4.51 |
| KSP-like, SetNewWorldOrigin every tick: frame, shift stopwatch | 11.37, 4.64 / 11.47, 4.75 | 13.88, 5.91 / 14.01, 5.95 |
| KSP-like, MoveWorld every tick: frame, shift stopwatch | 14.04, 7.07 / 14.08, 7.21 | 18.30, 9.19 / 19.33, 9.54 |

So in the packaged build UE's every-tick origin shift with a KSP-sized scene costs about 4.7 ms on the stopwatch and about
**+7.5 ms of frame time**, against KSP's ~5.5 ms. Same conclusion: not cheaper than KSP. Runs vary by roughly 10 to 20% on this
shared laptop.

### Screenshots (`shots/`)

`full2_None_OrbitImg.png` (100 km orbit, atmosphere limb, cubes A red / B yellow), `full2_None_OrbitWarp_mid.png`,
`full2_None_Descent_alt{100,30,10,1,0.1,0.01}km.png`, `full2_None_Horizon.png` (10 m, horizon), `full2_None_GroundStatic.png`
(1 m; B's 4 cm vertex quantisation is visible as a skewed cube), `full2_None_GroundStrafe.png`, `full2_None_MunFromOrbit.png`,
`full2_None_MunNear.png`, `full2_<variant>_GroundStatic.png` and `full2_<variant>_MunNear.png` for every variant, and
`full2_warmup_view0..4.png`. Captured through the same screenshot pipeline as `Shot`/`HighResShot`
(`FScreenshotRequest` + `OnScreenshotCaptured`), saved as PNG by the director.

## Implications for the port

1. Build every renderable with small primitive-local coordinates: terrain patches centred on the patch, parts on the part.
   Never store a planet-centred or world-space mesh. This is the only thing that jittered.
2. Rendering does not need floating-origin rebasing in UE 5.8 at Kerbin or Mun scale. Keep the simulation in the core's own
   doubles, as FEASIBILITY section 5 plans, and treat UE world space as a camera-local view. If a float-only subsystem
   (Niagara, which was not tested here) or Chaos's local bubble needs small coordinates, rebase rarely (about a 100 km
   threshold) with `SetNewWorldOrigin`. Do not rebase every tick: with Chaos bodies it costs about 9 ms per shift.
3. KSP's 100 km rotating-frame threshold (`PROFILE.md`, `CelestialBody.inverseRotThresholdAltitude`) is a physics-frame
   choice and is independent of this result. A 100 km rebasing threshold means at most one shift per ~45 s in low orbit.
4. The `int32` origin limits `SetNewWorldOrigin` to +-21,474 km. Interplanetary scale needs the camera-local approach anyway:
   place UE actors at (body position - camera position) from the core's doubles every frame.

## Not verified / caveats

- The canonical data is from editor binaries in `-game`. The cooked Development game reproduced every jitter metric exactly
  (see "Cooked Development game"). Shipping was not tested.
- Low scalability, no shadows, no TAA/TSR. Shadow (CSM/VSM) stability and TSR at this scale were not measured.
- Planet rotation (KSP's rotating frame below 100 km) was not modelled; the planet was static.
- No terrain heights or PQS-style displacement. Niagara/particles, decals and translucency were not tested.
- Chaos simulation far from the origin was only exercised as free bodies without gravity (teleport correctness and cost),
  not joints or contacts.
- The image proxy covers the cubes and the Mun. Planet-surface jitter rests on the emulator, which was validated against
  the GPU on cube B (same arithmetic as a planet-centred mesh) and cube A (same arithmetic as a patch-centred mesh).
- Numbers come from one laptop with other work running (UBT reported only ~6 GB of free RAM), so expect run-to-run noise of a
  few percent. Two full runs gave identical jitter metrics and stress costs within 3%.

## Reproduce

```powershell
.\Scripts\build.ps1                      # editor target
.\Scripts\create_map.ps1                 # Content/Maps/ScaleSpike.umap (already committed)
.\Scripts\spike.ps1 -Tag full2           # ~3.5 min including startup; writes spike/data, spike/shots, spike/logs
python -I spike\analyze.py full2 > spike\data\full2_tables.md
.\Scripts\spike.ps1 -Tag coldddc -Quick -Variants None -Stress 0 -Extra "-DDC=Cold"   # cold shader compile
```

## Appendix: generated tables (`data/full2_tables.md`)

See `data/full2_tables.md` for every variant x phase: image proxy, numeric proxy (all six references), frame times, shifts,
LOD cost, patches and camera-to-origin distance, plus the full stress table.
